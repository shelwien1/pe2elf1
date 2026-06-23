/*
 * mine32 - exhaustively mine the 40-bit (5-byte) opcode space for every
 *          decodable x86-32 instruction, using the Zydis decoder.
 *
 * The "40-bit space" is the set of all 2^40 distinct 5-byte sequences.  A naive
 * sweep would need 1.1 trillion decode calls, so this tool instead walks the
 * space while skipping over "don't-care" bytes:
 *
 *   - On a SUCCESSFUL decode of length L, the bytes after L are not part of the
 *     instruction and can be skipped.  By default we additionally treat the
 *     displacement and immediate bytes as wildcards (see --no-skip-imm), which
 *     collapses e.g. all 2^32 encodings of "mov eax, imm32" into a single form.
 *
 *   - On an INVALID decode we probe with increasing buffer lengths to find the
 *     shortest prefix that already determines the failure; once a buffer is
 *     rejected, appending more bytes can never make that same prefix valid, so
 *     the trailing bytes are skippable too.
 *
 *   - On a TRUNCATED decode (NO_MORE_DATA: the instruction needs more than the
 *     window) we re-probe with a full 15-byte buffer to learn the structural
 *     length and skip the trailing displacement/immediate bytes.
 *
 * What remains is enumerated exactly once per distinct instruction "form".
 *
 * The 40-bit value is laid out big-endian: byte 0 (the first instruction byte)
 * is the most significant.  Work is partitioned across threads by the first
 * byte (256 chunks handed out from an atomic counter), so every form is visited
 * by exactly one thread with no double counting.
 */

#include <Zydis/Zydis.h>

#include <inttypes.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_BYTES   8       /* window must fit in a 64-bit counter           */
#define PROBE_LEN   15      /* full x86 instruction length for re-probing    */
#define BUF_LEN     16

/* ---- configuration (set once in main, read-only afterwards) ------------- */
static ZydisDecoder    g_decoder;
static ZydisFormatter  g_fmt;                   /* read-only, shared         */
static int          g_nbytes    = 5;            /* window size (40 bit = 5)  */
static int          g_skip_imm  = 1;            /* treat disp/imm as wildcard*/
static int          g_canon     = 0;            /* collapse register variants*/
static int          g_quiet     = 0;
static const char  *g_dump      = NULL;         /* dump file prefix or NULL  */
static unsigned     g_b0_start  = 0;
static unsigned     g_b0_end    = 256;

/* ===================== canonical-form de-duplication ======================
 * In --canon mode we collapse instructions that differ only in their register
 * operands: every register is mapped to the base register of its class (GPR ->
 * eax/ax/al, XMM -> xmm0, YMM -> ymm0, ...), so e.g. "add ecx, edx" and
 * "add esi, edi" become the single form "add eax, eax".  Displacement/immediate
 * *sizes*, the SIB scale, the addressing structure and a few semantic prefixes
 * are kept, so genuinely different encodings/addressing modes stay distinct. */

static char *dupstr(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}


/* register -> base register of its class (eax, ax, al, xmm0, ymm0, ...) */
static inline ZydisRegister reg_base(ZydisRegister r)
{
    if (r == ZYDIS_REGISTER_NONE) return r;
    ZydisRegister b = ZydisRegisterEncode(ZydisRegisterGetClass(r), 0);
    return (b == ZYDIS_REGISTER_NONE) ? r : b;
}

/* Format the instruction with every register replaced by its class base, into
 * `out`. The resulting text is the dedup key, so what you see is what is
 * counted. Returns the string length, or -1 on failure. */
static int format_canonical(const ZydisDecodedInstruction *insn,
                            const ZydisDecodedOperand *ops, char *out, size_t outsz)
{
    ZydisDecodedOperand norm[ZYDIS_MAX_OPERAND_COUNT];
    memcpy(norm, ops, sizeof(norm));
    for (int i = 0; i < insn->operand_count_visible; i++) {
        if (norm[i].type == ZYDIS_OPERAND_TYPE_REGISTER)
            norm[i].reg.value = reg_base(norm[i].reg.value);
        else if (norm[i].type == ZYDIS_OPERAND_TYPE_MEMORY) {
            norm[i].mem.base  = reg_base(norm[i].mem.base);
            norm[i].mem.index = reg_base(norm[i].mem.index);
        }
    }
    /* A relative branch's printed target is runtime_address + length + rel.
     * The rel is wildcarded (0), but the length varies with prefix padding, so
     * the same branch would otherwise print different targets and fail to
     * merge. Setting runtime_address = -length makes the target compute to the
     * (wildcarded) rel = 0 for every encoding, independent of length. */
    ZyanU64 runtime = (ZyanU64)0 - (ZyanU64)insn->length;
    if (!ZYAN_SUCCESS(ZydisFormatterFormatInstruction(
            &g_fmt, insn, norm, insn->operand_count_visible,
            out, outsz, runtime, ZYAN_NULL)))
        return -1;
    return (int)strlen(out);
}

/* ---- open-addressing hash set of canonical keys (power-of-two capacity) -- */
typedef struct {
    uint64_t  hash;
    uint8_t  *key;        /* canonical disassembly string (the dedup key)   */
    uint64_t  minv;       /* smallest window value seen => "first instance" */
    uint16_t  klen;
    uint16_t  mnemonic;
    uint8_t   oplen;      /* length of the representative (shortest) encoding*/
    uint8_t   opcut;      /* where structural bytes end (the | marker)      */
    uint8_t   opbytes[MAX_BYTES];  /* first-instance opcode bytes           */
} Slot;

typedef struct { Slot *s; size_t cap, cnt; } HMap;

/* order canonical-form slots by their disassembly text (for the dump) */
static int cmp_slot(const void *a, const void *b)
{
    const Slot *x = *(const Slot *const *)a;
    const Slot *y = *(const Slot *const *)b;
    return strcmp((const char *)x->key, (const char *)y->key);
}

static uint64_t fnv1a(const uint8_t *p, int n)
{
    uint64_t h = 1469598103934665603ULL;
    for (int i = 0; i < n; i++) { h ^= p[i]; h *= 1099511628211ULL; }
    return h;
}

static void hmap_init(HMap *m, size_t cap)
{
    m->cap = cap; m->cnt = 0;
    m->s = calloc(cap, sizeof(Slot));
}

static Slot *hmap_slot(HMap *m, const uint8_t *key, int klen, uint64_t h)
{
    size_t mask = m->cap - 1, i = h & mask;
    for (;;) {
        Slot *sl = &m->s[i];
        if (!sl->key) return sl;                            /* empty */
        if (sl->hash == h && sl->klen == klen &&
            memcmp(sl->key, key, klen) == 0) return sl;     /* match */
        i = (i + 1) & mask;
    }
}

static void hmap_grow(HMap *m)
{
    size_t ncap = m->cap * 2, nmask = ncap - 1;
    Slot *ns = calloc(ncap, sizeof(Slot));
    for (size_t i = 0; i < m->cap; i++) {
        Slot *o = &m->s[i];
        if (!o->key) continue;
        size_t j = o->hash & nmask;
        while (ns[j].key) j = (j + 1) & nmask;
        ns[j] = *o;
    }
    free(m->s);
    m->s = ns; m->cap = ncap;
}

/* Merge a source slot into `m`: insert a copy if new, otherwise keep the
 * shortest length and the smallest-value (first-instance) opcode bytes. */
static void hmap_merge_slot(HMap *m, const Slot *src)
{
    if ((m->cnt + 1) * 10 >= m->cap * 7) hmap_grow(m);
    Slot *d = hmap_slot(m, src->key, src->klen, src->hash);
    if (!d->key) {
        *d = *src;
        d->key = malloc(src->klen + 1);
        memcpy(d->key, src->key, src->klen);
        d->key[src->klen] = 0;
        m->cnt++;
        return;
    }
    if (src->oplen < d->oplen ||
        (src->oplen == d->oplen && src->minv < d->minv)) {
        d->minv = src->minv;
        d->oplen = src->oplen;
        d->opcut = src->opcut;
        memcpy(d->opbytes, src->opbytes, sizeof(d->opbytes));
    }
}

static void hmap_free(HMap *m)
{
    if (!m->s) return;
    for (size_t i = 0; i < m->cap; i++) free(m->s[i].key);
    free(m->s); m->s = NULL; m->cap = m->cnt = 0;
}

/* ---- work distribution -------------------------------------------------- */
static atomic_uint  g_next_chunk;               /* next first-byte to claim  */
static atomic_uint  g_done_chunks;

/* ---- per-thread statistics --------------------------------------------- */
#define NMNEM (ZYDIS_MNEMONIC_MAX_VALUE + 1)
typedef struct {
    int        tid;
    uint64_t   forms;                  /* distinct decodable forms found     */
    uint64_t   raw;                    /* every decodable form (pre-dedup)   */
    uint64_t   decodes;                /* Zydis decode calls issued          */
    uint64_t   len_hist[MAX_BYTES + 1];/* forms by instruction length        */
    uint64_t  *mnem;                   /* forms by mnemonic (heap, NMNEM)    */
    HMap       map;                    /* canonical-form set (--canon)       */
    FILE      *dump;                   /* per-thread dump file or NULL       */
} ThreadStats;

/* Compute the "structural" length: the offset of the first displacement or
 * immediate byte, i.e. everything before it (prefixes/opcode/modrm/sib) that
 * actually selects the instruction form.  Falls back to the full length. */
static int structural_len(const ZydisDecodedInstruction *insn)
{
    int keep = insn->length;
    if (insn->raw.disp.size && insn->raw.disp.offset < keep)
        keep = insn->raw.disp.offset;
    if (insn->raw.imm[0].size && insn->raw.imm[0].offset < keep)
        keep = insn->raw.imm[0].offset;
    if (insn->raw.imm[1].size && insn->raw.imm[1].offset < keep)
        keep = insn->raw.imm[1].offset;
    if (keep < 1) keep = 1;
    return keep;
}

static void record_form(ThreadStats *ts, const ZydisDecodedInstruction *insn,
                        const ZydisDecodedOperand *ops, const uint8_t *buf,
                        uint64_t v)
{
    ts->raw++;

    if (g_canon) {
        /* `ops` is valid here (process_chunk used DecodeFull). The dedup key is
         * the register-normalised disassembly string itself, so two forms that
         * differ only in registers (and thus print identically) collapse. We
         * keep the opcode bytes of the *shortest* encoding seen (ties broken by
         * smallest value), which is the cleanest concrete instance of the form
         * (no redundant prefixes, base registers). */
        char text[256];
        int klen = format_canonical(insn, ops, text, sizeof(text));
        if (klen < 0) return;
        uint64_t h = fnv1a((const uint8_t *)text, klen);
        if ((ts->map.cnt + 1) * 10 >= ts->map.cap * 7) hmap_grow(&ts->map);
        Slot *sl = hmap_slot(&ts->map, (const uint8_t *)text, klen, h);
        if (sl->key) {                       /* seen this form already       */
            if (insn->length < sl->oplen ||
                (insn->length == sl->oplen && v < sl->minv)) {
                sl->minv = v;
                sl->oplen = (uint8_t)insn->length;
                sl->opcut = (uint8_t)structural_len(insn);
                memcpy(sl->opbytes, buf, insn->length);
            }
            return;
        }
        sl->key = (uint8_t *)dupstr(text);   /* the string is the key        */
        sl->klen = (uint16_t)klen; sl->hash = h;
        sl->mnemonic = (uint16_t)insn->mnemonic;
        sl->minv = v;
        sl->oplen = (uint8_t)insn->length;
        sl->opcut = (uint8_t)structural_len(insn);
        memcpy(sl->opbytes, buf, insn->length);
        ts->map.cnt++;
        return;
    }

    /* default mode: count every structural form */
    ts->forms++;
    ts->len_hist[insn->length]++;
    ts->mnem[insn->mnemonic]++;

    if (!ts->dump)
        return;

    /* Re-decode with operands so the formatter can print a readable form.
     * Wildcard (disp/imm) bytes appear as their current value (usually 0). */
    ZydisDecodedInstruction full;
    ZydisDecodedOperand     fops[ZYDIS_MAX_OPERAND_COUNT];
    char text[256];
    if (ZYAN_SUCCESS(ZydisDecoderDecodeFull(&g_decoder, buf, g_nbytes,
                                            &full, fops)) &&
        ZYAN_SUCCESS(ZydisFormatterFormatInstruction(
            &g_fmt, &full, fops, full.operand_count_visible,
            text, sizeof(text), 0, ZYAN_NULL)))
    {
        int sk = structural_len(insn);
        for (int i = 0; i < insn->length; i++)
            fprintf(ts->dump, "%s%02x", i == sk ? "|" : (i ? " " : ""), buf[i]);
        fprintf(ts->dump, "\t%s\n", text);
    }
}

static inline ZyanStatus decode_n(ThreadStats *ts, const uint8_t *buf, int n,
                                  ZydisDecodedInstruction *insn)
{
    ts->decodes++;
    return ZydisDecoderDecodeInstruction(&g_decoder, ZYAN_NULL, buf, n, insn);
}

static void process_chunk(unsigned b0, ThreadStats *ts)
{
    const int      N          = g_nbytes;
    const uint64_t inner_bits = (uint64_t)(N - 1) * 8;
    const uint64_t inner_end  = (inner_bits >= 64) ? 0 : (1ULL << inner_bits);

    uint8_t buf[BUF_LEN];
    memset(buf, 0, sizeof(buf));
    buf[0] = (uint8_t)b0;

    ZydisDecodedInstruction insn;
    ZydisDecodedOperand     ops[ZYDIS_MAX_OPERAND_COUNT];

    uint64_t lo = 0;
    while (lo < inner_end) {
        for (int i = 1; i < N; i++)
            buf[i] = (uint8_t)(lo >> ((N - 1 - i) * 8));

        /* canon mode needs operands, so decode fully; otherwise stay lean */
        ZyanStatus st;
        if (g_canon) {
            ts->decodes++;
            st = ZydisDecoderDecodeFull(&g_decoder, buf, N, &insn, ops);
        } else {
            st = decode_n(ts, buf, N, &insn);
        }

        int keep;
        if (ZYAN_SUCCESS(st)) {
            keep = g_skip_imm ? structural_len(&insn) : insn.length;
            uint64_t v = ((uint64_t)b0 << inner_bits) | lo;
            record_form(ts, &insn, ops, buf, v);
        } else if (st == ZYDIS_STATUS_NO_MORE_DATA) {
            /* Needs more than the window: probe full length to find where the
             * structural part ends, then skip the trailing value bytes. */
            ZydisDecodedInstruction p;
            if (ZYAN_SUCCESS(decode_n(ts, buf, PROBE_LEN, &p))) {
                keep = structural_len(&p);
                if (keep > N) keep = N;
            } else {
                keep = N;                       /* cannot prove a skip       */
            }
        } else {
            /* Hard decode error: find the shortest prefix that already fails.
             * Once rejected, longer completions of that prefix stay invalid. */
            keep = N;
            for (int k = 1; k < N; k++) {
                ZydisDecodedInstruction t;
                if (decode_n(ts, buf, k, &t) != ZYDIS_STATUS_NO_MORE_DATA) {
                    keep = k;
                    break;
                }
            }
        }

        if (keep < 1) keep = 1;
        if (keep > N) keep = N;

        int shift = (N - keep) * 8;             /* low inner bits to wildcard */
        if (shift >= (int)inner_bits)
            break;                              /* whole rest of chunk skipped*/
        uint64_t step = 1ULL << shift;
        lo = (lo & ~(step - 1)) + step;
    }
}

static void *worker(void *arg)
{
    ThreadStats *ts = (ThreadStats *)arg;
    if (g_dump && !g_canon) {           /* canon writes one merged file later */
        char name[512];
        snprintf(name, sizeof(name), "%s.%d", g_dump, ts->tid);
        ts->dump = fopen(name, "w");
        if (!ts->dump)
            fprintf(stderr, "warning: cannot open dump file %s\n", name);
    }

    for (;;) {
        unsigned b0 = atomic_fetch_add(&g_next_chunk, 1);
        if (b0 >= g_b0_end)
            break;
        process_chunk(b0, ts);
        unsigned done = atomic_fetch_add(&g_done_chunks, 1) + 1;
        if (!g_quiet)
            fprintf(stderr, "\rchunks %3u/%-3u", done,
                    g_b0_end - g_b0_start);
    }

    if (ts->dump)
        fclose(ts->dump);
    return NULL;
}

static double now_sec(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

static void usage(const char *p)
{
    fprintf(stderr,
        "usage: %s [options]\n"
        "  -t, --threads N     worker threads (default: 4)\n"
        "  -b, --bytes N       window size in bytes, 1..8 (default: 5 = 40 bit)\n"
        "  -m, --mode M        16 | 32 | 64 (default: 32)\n"
        "      --no-skip-imm   count every disp/imm value as a distinct form\n"
        "      --canon         merge instructions that differ only in registers\n"
        "                      (every register -> class base: eax, xmm0, ...)\n"
        "      --dump PREFIX   write found forms to PREFIX.<tid> (PREFIX in --canon)\n"
        "      --range A B     only first-bytes [A,B) (for testing; default 0 256)\n"
        "  -q, --quiet         no progress output\n"
        "  -h, --help          this help\n", p);
}

int main(int argc, char **argv)
{
    int threads = 4;
    int mode    = 32;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if ((!strcmp(a, "-t") || !strcmp(a, "--threads")) && i + 1 < argc)
            threads = atoi(argv[++i]);
        else if ((!strcmp(a, "-b") || !strcmp(a, "--bytes")) && i + 1 < argc)
            g_nbytes = atoi(argv[++i]);
        else if ((!strcmp(a, "-m") || !strcmp(a, "--mode")) && i + 1 < argc)
            mode = atoi(argv[++i]);
        else if (!strcmp(a, "--no-skip-imm"))
            g_skip_imm = 0;
        else if (!strcmp(a, "--canon"))
            g_canon = 1;
        else if (!strcmp(a, "--dump") && i + 1 < argc)
            g_dump = argv[++i];
        else if (!strcmp(a, "--range") && i + 2 < argc) {
            g_b0_start = (unsigned)strtoul(argv[++i], NULL, 0);
            g_b0_end   = (unsigned)strtoul(argv[++i], NULL, 0);
        } else if (!strcmp(a, "-q") || !strcmp(a, "--quiet"))
            g_quiet = 1;
        else if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "unknown option: %s\n", a);
            usage(argv[0]);
            return 1;
        }
    }

    if (g_nbytes < 1 || g_nbytes > MAX_BYTES) {
        fprintf(stderr, "bytes must be 1..%d\n", MAX_BYTES);
        return 1;
    }
    if (threads < 1) threads = 1;
    if (g_b0_end > 256) g_b0_end = 256;
    if (g_b0_start > g_b0_end) g_b0_start = g_b0_end;

    ZydisMachineMode mm;
    ZydisStackWidth  sw;
    switch (mode) {
        case 16: mm = ZYDIS_MACHINE_MODE_LEGACY_16; sw = ZYDIS_STACK_WIDTH_16; break;
        case 32: mm = ZYDIS_MACHINE_MODE_LEGACY_32; sw = ZYDIS_STACK_WIDTH_32; break;
        case 64: mm = ZYDIS_MACHINE_MODE_LONG_64;   sw = ZYDIS_STACK_WIDTH_64; break;
        default: fprintf(stderr, "mode must be 16, 32 or 64\n"); return 1;
    }
    if (!ZYAN_SUCCESS(ZydisDecoderInit(&g_decoder, mm, sw))) {
        fprintf(stderr, "ZydisDecoderInit failed\n");
        return 1;
    }
    ZydisFormatterInit(&g_fmt, ZYDIS_FORMATTER_STYLE_INTEL);

    fprintf(stderr,
        "mining %d-bit space (mode=%d, window=%d bytes, threads=%d, "
        "skip-imm=%d, canon=%d, first-bytes=[%u,%u))\n",
        g_nbytes * 8, mode, g_nbytes, threads, g_skip_imm, g_canon,
        g_b0_start, g_b0_end);

    atomic_store(&g_next_chunk, g_b0_start);
    atomic_store(&g_done_chunks, 0);

    ThreadStats *stats = calloc(threads, sizeof(*stats));
    pthread_t   *tids  = calloc(threads, sizeof(*tids));
    for (int i = 0; i < threads; i++) {
        stats[i].tid  = i;
        stats[i].mnem = calloc(NMNEM, sizeof(uint64_t));
        if (g_canon)
            hmap_init(&stats[i].map, 4096);
    }

    double t0 = now_sec();
    for (int i = 0; i < threads; i++)
        pthread_create(&tids[i], NULL, worker, &stats[i]);
    for (int i = 0; i < threads; i++)
        pthread_join(tids[i], NULL);
    double dt = now_sec() - t0;
    if (!g_quiet) fputc('\n', stderr);

    /* merge */
    uint64_t total_forms = 0, total_raw = 0, total_decodes = 0;
    uint64_t len_hist[MAX_BYTES + 1] = {0};
    uint64_t *mnem = calloc(NMNEM, sizeof(uint64_t));
    for (int i = 0; i < threads; i++) {
        total_raw     += stats[i].raw;
        total_decodes += stats[i].decodes;
    }

    if (g_canon) {
        /* Union the per-thread canonical sets: the same form (e.g. "push eax")
         * can be produced from different first-byte chunks owned by different
         * threads, so cross-thread de-duplication is required for a true count. */
        HMap g;
        hmap_init(&g, 1u << 16);
        for (int i = 0; i < threads; i++) {
            HMap *lm = &stats[i].map;
            for (size_t j = 0; j < lm->cap; j++)
                if (lm->s[j].key)
                    hmap_merge_slot(&g, &lm->s[j]);
            hmap_free(lm);
        }
        total_forms = g.cnt;
        for (size_t j = 0; j < g.cap; j++)
            if (g.s[j].key) {
                len_hist[g.s[j].oplen]++;
                mnem[g.s[j].mnemonic]++;
            }

        /* dump the merged forms, sorted by disassembly, each with the opcode
         * bytes of its first (smallest-valued) instance; "|" marks where the
         * structural bytes end and wildcard disp/imm bytes begin. */
        if (g_dump) {
            Slot **arr = malloc(g.cnt * sizeof(Slot *));
            size_t nl = 0;
            for (size_t j = 0; j < g.cap; j++)
                if (g.s[j].key) arr[nl++] = &g.s[j];
            qsort(arr, nl, sizeof(Slot *), cmp_slot);
            FILE *f = fopen(g_dump, "w");
            if (f) {
                for (size_t j = 0; j < nl; j++) {
                    const Slot *sl = arr[j];
                    for (int b = 0; b < sl->oplen; b++)
                        fprintf(f, "%s%02x",
                                b == sl->opcut ? "|" : (b ? " " : ""),
                                sl->opbytes[b]);
                    fprintf(f, "\t%s\n", (char *)sl->key);
                }
                fclose(f);
                fprintf(stderr, "wrote canonical forms to %s\n", g_dump);
            } else {
                fprintf(stderr, "warning: cannot open dump file %s\n", g_dump);
            }
            free(arr);
        }
        hmap_free(&g);
    } else {
        for (int i = 0; i < threads; i++) {
            total_forms += stats[i].forms;
            for (int l = 0; l <= MAX_BYTES; l++) len_hist[l] += stats[i].len_hist[l];
            for (int m = 0; m < NMNEM; m++)      mnem[m]     += stats[i].mnem[m];
        }
    }
    for (int i = 0; i < threads; i++) free(stats[i].mnem);

    printf("\n===== results =====\n");
    if (g_canon) {
        printf("canonical forms : %" PRIu64 "   (register variants merged)\n",
               total_forms);
        printf("raw forms       : %" PRIu64 "   (before register merge)\n",
               total_raw);
    } else {
        printf("decodable forms : %" PRIu64 "\n", total_forms);
    }
    printf("decode calls    : %" PRIu64 "\n", total_decodes);
    printf("elapsed         : %.2f s  (%.2f M decodes/s)\n",
           dt, total_decodes / dt / 1e6);

    printf("\nforms by length (bytes)%s:\n", g_canon ? ", shortest encoding" : "");
    for (int l = 1; l <= g_nbytes; l++)
        printf("  %d : %" PRIu64 "\n", l, len_hist[l]);

    /* count distinct mnemonics and list them sorted by frequency */
    int distinct = 0;
    for (int m = 0; m < NMNEM; m++) if (mnem[m]) distinct++;
    printf("\ndistinct mnemonics: %d\n", distinct);

    printf("\ntop mnemonics by form count:\n");
    for (int shown = 0; shown < 40; shown++) {
        int best = -1;
        for (int m = 0; m < NMNEM; m++)
            if (mnem[m] && (best < 0 || mnem[m] > mnem[best]))
                best = m;
        if (best < 0) break;
        printf("  %-16s %" PRIu64 "\n",
               ZydisMnemonicGetString((ZydisMnemonic)best), mnem[best]);
        mnem[best] = 0;
    }

    free(mnem);
    free(stats);
    free(tids);
    return 0;
}
