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
static int          g_quiet     = 0;
static const char  *g_dump      = NULL;         /* dump file prefix or NULL  */
static unsigned     g_b0_start  = 0;
static unsigned     g_b0_end    = 256;

/* ---- work distribution -------------------------------------------------- */
static atomic_uint  g_next_chunk;               /* next first-byte to claim  */
static atomic_uint  g_done_chunks;

/* ---- per-thread statistics --------------------------------------------- */
#define NMNEM (ZYDIS_MNEMONIC_MAX_VALUE + 1)
typedef struct {
    int        tid;
    uint64_t   forms;                  /* distinct decodable forms found     */
    uint64_t   decodes;                /* Zydis decode calls issued          */
    uint64_t   len_hist[MAX_BYTES + 1];/* forms by instruction length        */
    uint64_t  *mnem;                   /* forms by mnemonic (heap, NMNEM)    */
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
                        const uint8_t *buf)
{
    ts->forms++;
    ts->len_hist[insn->length]++;
    ts->mnem[insn->mnemonic]++;

    if (!ts->dump)
        return;

    /* Re-decode with operands so the formatter can print a readable form.
     * Wildcard (disp/imm) bytes appear as their current value (usually 0). */
    ZydisDecodedInstruction full;
    ZydisDecodedOperand     ops[ZYDIS_MAX_OPERAND_COUNT];
    char text[256];
    if (ZYAN_SUCCESS(ZydisDecoderDecodeFull(&g_decoder, buf, g_nbytes,
                                            &full, ops)) &&
        ZYAN_SUCCESS(ZydisFormatterFormatInstruction(
            &g_fmt, &full, ops, full.operand_count_visible,
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

    uint64_t lo = 0;
    while (lo < inner_end) {
        for (int i = 1; i < N; i++)
            buf[i] = (uint8_t)(lo >> ((N - 1 - i) * 8));

        ZydisDecodedInstruction insn;
        ZyanStatus st = decode_n(ts, buf, N, &insn);

        int keep;
        if (ZYAN_SUCCESS(st)) {
            keep = g_skip_imm ? structural_len(&insn) : insn.length;
            record_form(ts, &insn, buf);
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
    if (g_dump) {
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
        "      --dump PREFIX   write found forms to PREFIX.<tid> files\n"
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
        "skip-imm=%d, first-bytes=[%u,%u))\n",
        g_nbytes * 8, mode, g_nbytes, threads, g_skip_imm,
        g_b0_start, g_b0_end);

    atomic_store(&g_next_chunk, g_b0_start);
    atomic_store(&g_done_chunks, 0);

    ThreadStats *stats = calloc(threads, sizeof(*stats));
    pthread_t   *tids  = calloc(threads, sizeof(*tids));
    for (int i = 0; i < threads; i++) {
        stats[i].tid  = i;
        stats[i].mnem = calloc(NMNEM, sizeof(uint64_t));
    }

    double t0 = now_sec();
    for (int i = 0; i < threads; i++)
        pthread_create(&tids[i], NULL, worker, &stats[i]);
    for (int i = 0; i < threads; i++)
        pthread_join(tids[i], NULL);
    double dt = now_sec() - t0;
    if (!g_quiet) fputc('\n', stderr);

    /* merge */
    uint64_t total_forms = 0, total_decodes = 0;
    uint64_t len_hist[MAX_BYTES + 1] = {0};
    uint64_t *mnem = calloc(NMNEM, sizeof(uint64_t));
    for (int i = 0; i < threads; i++) {
        total_forms   += stats[i].forms;
        total_decodes += stats[i].decodes;
        for (int l = 0; l <= MAX_BYTES; l++) len_hist[l] += stats[i].len_hist[l];
        for (int m = 0; m < NMNEM; m++)      mnem[m]     += stats[i].mnem[m];
        free(stats[i].mnem);
    }

    printf("\n===== results =====\n");
    printf("decodable forms : %" PRIu64 "\n", total_forms);
    printf("decode calls    : %" PRIu64 "\n", total_decodes);
    printf("elapsed         : %.2f s  (%.2f M decodes/s)\n",
           dt, total_decodes / dt / 1e6);

    printf("\nforms by length (bytes):\n");
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
