/*
 * mine32cs - Capstone-based variant of mine32: mine the 40-bit (5-byte) opcode
 *            space for every decodable x86 instruction, using Capstone instead
 *            of Zydis.
 *
 * Same approach as mine32 (see mine32.c): enumerate the window as the first N
 * bytes of an instruction followed by zero padding, decode it, and skip the
 * don't-care bytes (those past the opcode/modrm/sib, plus disp/immediate with
 * --skip-imm) so each distinct form is visited once.  --canon collapses forms
 * that differ only in registers / index scale / segment / writemask.
 *
 * Capstone difference: on a decode failure Capstone cannot report how many
 * leading bytes are decisively invalid, so invalid candidates are stepped one
 * at a time (Zydis could skip them).  To keep that cheap the validity/length
 * probe uses a no-detail handle; a detail handle is used only on success (for
 * the structural offsets and operand/disasm text).
 */
#include <capstone/capstone.h>

#include <ctype.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_BYTES   8       /* enumeration window must fit in a 64-bit counter*/
#define PROBE_LEN   15      /* full x86 instruction length / decode buffer    */
#define BUF_LEN     16
#define MAX_ILEN    15      /* max instruction length                         */

/* ---- configuration (set once in main, read-only afterwards) ------------- */
static cs_mode      g_mode      = CS_MODE_32;
static int          g_modebits  = 32;
static int          g_nbytes    = 5;
static int          g_skip_imm  = 1;
static int          g_canon     = 0;
static int          g_quiet     = 0;
static const char  *g_dump      = NULL;
static unsigned     g_b0_start  = 0;
static unsigned     g_b0_end    = 256;
static csh          g_names;                    /* handle for cs_insn_name    */

/* ---- work distribution -------------------------------------------------- */
static atomic_uint  g_next_chunk;
static atomic_uint  g_done_chunks;

/* ===================== canonical-form normalisation ======================
 * Capstone hands back the disassembly as text, so --canon normalises that
 * text: every register name is rewritten to the base register of its class,
 * the SIB scale is folded to *1, segment overrides are dropped and the EVEX
 * writemask {k1..k7} is folded to {k1}. */

static char *dupstr(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

static int cmp_slot(const void *a, const void *b);  /* fwd */

static int is_seg(const char *t)
{
    return !strcmp(t, "es") || !strcmp(t, "cs") || !strcmp(t, "ss") ||
           !strcmp(t, "ds") || !strcmp(t, "fs") || !strcmp(t, "gs");
}

/* Map a register-name token to the base register of its class, or NULL if the
 * token is not a register name. */
static const char *reg_base_name(const char *t)
{
    static const char *g8[]  = {"al","bl","cl","dl","ah","bh","ch","dh",
                                "spl","bpl","sil","dil",0};
    static const char *g16[] = {"ax","bx","cx","dx","sp","bp","si","di",0};
    static const char *g32[] = {"eax","ebx","ecx","edx","esp","ebp","esi","edi",0};
    static const char *g64[] = {"rax","rbx","rcx","rdx","rsp","rbp","rsi","rdi",0};
    for (int i = 0; g8[i];  i++) if (!strcmp(t, g8[i]))  return "al";
    for (int i = 0; g16[i]; i++) if (!strcmp(t, g16[i])) return "ax";
    for (int i = 0; g32[i]; i++) if (!strcmp(t, g32[i])) return "eax";
    for (int i = 0; g64[i]; i++) if (!strcmp(t, g64[i])) return "rax";
    if (is_seg(t)) return "es";
    size_t n = strlen(t);
    /* r8..r15 (+ b/w/d suffix) extended GPRs */
    if (t[0] == 'r' && isdigit((unsigned char)t[1])) {
        char suf = t[n - 1];
        if (suf == 'b') return "al";
        if (suf == 'w') return "ax";
        if (suf == 'd') return "eax";
        if (isdigit((unsigned char)suf)) return "rax";
    }
    if (!strncmp(t, "xmm", 3)) return "xmm0";
    if (!strncmp(t, "ymm", 3)) return "ymm0";
    if (!strncmp(t, "zmm", 3)) return "zmm0";
    if (t[0] == 'm' && t[1] == 'm' && isdigit((unsigned char)t[2])) return "mm0";
    if (t[0] == 'k' && t[1] >= '0' && t[1] <= '7' && n == 2) return "k0";
    if (!strncmp(t, "cr", 2) && isdigit((unsigned char)t[2])) return "cr0";
    if (!strncmp(t, "dr", 2) && isdigit((unsigned char)t[2])) return "dr0";
    if (!strncmp(t, "bnd", 3)) return "bnd0";
    if (!strncmp(t, "tmm", 3)) return "tmm0";
    if (!strcmp(t, "st"))      return "st0";
    return NULL;
}

/* Rewrite `in` (an Intel op_str) into `out` with the canon normalisations. */
static void normalize_opstr(const char *in, char *out, size_t outsz)
{
    size_t o = 0, i = 0;
#define PUT(c) do { if (o + 1 < outsz) out[o++] = (c); } while (0)
#define PUTS(s) do { for (const char *p = (s); *p; p++) PUT(*p); } while (0)
    while (in[i]) {
        char c = in[i];
        if (c == '{') {                         /* {k3}/{z}/{1to8}/{rn-sae}  */
            size_t j = i + 1;
            char inner[32]; size_t k = 0;
            while (in[j] && in[j] != '}') { if (k + 1 < sizeof inner) inner[k++] = in[j]; j++; }
            inner[k] = 0;
            if (inner[0] == 'k' && inner[1] >= '1' && inner[1] <= '7' && !inner[2])
                PUTS("{k1}");
            else { PUT('{'); PUTS(inner); PUT('}'); }
            i = in[j] ? j + 1 : j;
            continue;
        }
        if (c == '*' && (in[i+1]=='1'||in[i+1]=='2'||in[i+1]=='4'||in[i+1]=='8')) {
            i += 2; continue;       /* drop "*N": Capstone omits *1, so this
                                     * folds [r+r*2] and [r+r] together       */
        }
        if (isalpha((unsigned char)c) || c == '_') {
            char tok[32]; size_t t = 0;
            while ((isalnum((unsigned char)in[i]) || in[i]=='_') && t+1 < sizeof tok)
                tok[t++] = in[i++];
            tok[t] = 0;
            if (is_seg(tok) && in[i] == ':') {  /* drop segment override     */
                i++;                            /* also skip the ':'         */
                continue;
            }
            if (!strcmp(tok, "st") && in[i] == '(') {   /* x87 st(N) -> st0  */
                while (in[i] && in[i] != ')') i++;
                if (in[i]) i++;
                PUTS("st0");
                continue;
            }
            const char *b = reg_base_name(tok);
            PUTS(b ? b : tok);
            continue;
        }
        PUT(c); i++;
    }
    out[o] = 0;
#undef PUT
#undef PUTS
}

/* Build the canon dedup key for a (detailed) instruction into `out`. */
static void canon_key(csh hd, const cs_insn *in, char *out, size_t outsz)
{
    /* relative branch: the target is position-dependent (addr+len+rel); the rel
     * is wildcarded, so collapse the operand to a fixed 0 to merge encodings. */
    if (in->op_str[0] &&
        cs_insn_group((csh)hd, in, X86_GRP_BRANCH_RELATIVE)) {
        snprintf(out, outsz, "%s 0", in->mnemonic);
        return;
    }
    if (!in->op_str[0]) {
        snprintf(out, outsz, "%s", in->mnemonic);
        return;
    }
    char norm[256];
    normalize_opstr(in->op_str, norm, sizeof norm);
    snprintf(out, outsz, "%s %s", in->mnemonic, norm);
}

/* ---- open-addressing hash set of canonical keys ------------------------- */
typedef struct {
    uint64_t  hash;
    uint8_t  *key;
    uint64_t  minv;
    uint16_t  klen;
    uint16_t  mnemonic;          /* x86_insn id                              */
    uint8_t   oplen;
    uint8_t   opcut;
    uint8_t   opbytes[MAX_ILEN];
} Slot;
typedef struct { Slot *s; size_t cap, cnt; } HMap;

static int cmp_slot(const void *a, const void *b)
{
    const Slot *x = *(const Slot *const *)a, *y = *(const Slot *const *)b;
    return strcmp((const char *)x->key, (const char *)y->key);
}
static uint64_t fnv1a(const uint8_t *p, int n)
{
    uint64_t h = 1469598103934665603ULL;
    for (int i = 0; i < n; i++) { h ^= p[i]; h *= 1099511628211ULL; }
    return h;
}
static void hmap_init(HMap *m, size_t cap){ m->cap=cap; m->cnt=0; m->s=calloc(cap,sizeof(Slot)); }
static Slot *hmap_slot(HMap *m, const uint8_t *key, int klen, uint64_t h)
{
    size_t mask = m->cap-1, i = h & mask;
    for (;;) {
        Slot *sl = &m->s[i];
        if (!sl->key) return sl;
        if (sl->hash==h && sl->klen==klen && !memcmp(sl->key,key,klen)) return sl;
        i = (i+1) & mask;
    }
}
static void hmap_grow(HMap *m)
{
    size_t nc = m->cap*2, nmask = nc-1; Slot *ns = calloc(nc,sizeof(Slot));
    for (size_t i=0;i<m->cap;i++){ Slot*o=&m->s[i]; if(!o->key)continue;
        size_t j=o->hash&nmask; while(ns[j].key) j=(j+1)&nmask; ns[j]=*o; }
    free(m->s); m->s=ns; m->cap=nc;
}
static void hmap_merge_slot(HMap *m, const Slot *src)
{
    if ((m->cnt+1)*10 >= m->cap*7) hmap_grow(m);
    Slot *d = hmap_slot(m, src->key, src->klen, src->hash);
    if (!d->key) { *d=*src; d->key=malloc(src->klen+1);
        memcpy(d->key,src->key,src->klen); d->key[src->klen]=0; m->cnt++; return; }
    if (src->oplen < d->oplen || (src->oplen==d->oplen && src->minv < d->minv)) {
        d->minv=src->minv; d->oplen=src->oplen; d->opcut=src->opcut;
        memcpy(d->opbytes, src->opbytes, sizeof d->opbytes);
    }
}
static void hmap_free(HMap *m){
    if(!m->s)return;
    for(size_t i=0;i<m->cap;i++) free(m->s[i].key);
    free(m->s); m->s=NULL; m->cap=m->cnt=0;
}

/* ---- per-thread statistics --------------------------------------------- */
#define NMNEM (X86_INS_ENDING)
typedef struct {
    int        tid;
    uint64_t   forms, raw, decodes;
    uint64_t   len_hist[MAX_ILEN + 1];
    uint64_t  *mnem;
    HMap       map;
    FILE      *dump;
    csh        hf, hd;          /* fast (no detail) + detail handles         */
    cs_insn   *inf, *ind;
} ThreadStats;

/* legacy prefix bytes (group 1-4) -- a leading run of these precedes the
 * opcode.  We detect them from the actual instruction bytes because Capstone's
 * cs_x86.prefix[] is inconsistent for redundant/mandatory prefixes. */
static int is_prefix_byte(uint8_t b)
{
    switch (b) {
    case 0xF0: case 0xF2: case 0xF3:                        /* lock / rep(ne)  */
    case 0x2E: case 0x36: case 0x3E: case 0x26:             /* cs/ss/ds/es     */
    case 0x64: case 0x65:                                   /* fs/gs           */
    case 0x66: case 0x67:                                   /* opsize/addrsize */
        return 1;
    }
    return 0;
}

/* Length of the structural part of an instruction -- everything up to the
 * first wildcard (displacement / immediate) byte.  Capstone's
 * encoding.imm_offset marks only the *last* immediate, so any form with two
 * immediates -- enter imm16,imm8 (0xC8); far ljmp/lcall ptr16:16|32
 * (0xEA/0x9A); SSE4a extrq/insertq xmm,imm8,imm8 (66/F2 0F 78 /r ib ib) --
 * would leave the earlier immediate(s) un-skipped and explode the sweep.  We
 * instead compute where the wildcard region begins directly. */
static int structural_len(const cs_insn *in)
{
    const cs_x86          *x = &in->detail->x86;
    const cs_x86_encoding *e = &x->encoding;
    int keep = in->size;
    if (e->disp_offset && e->disp_offset < keep) keep = e->disp_offset;
    if (e->imm_offset  && e->imm_offset  < keep) keep = e->imm_offset;

    int first;
    if (e->modrm_offset) {
        /* ModRM present: the wildcard region starts after modrm(+sib); any
         * displacement is already folded in above via disp_offset. */
        int mod = (x->modrm >> 6) & 3, rm = x->modrm & 7;
        int has_sib = (g_modebits != 16 && mod != 3 && rm == 4);
        first = e->modrm_offset + 1 + has_sib;
    } else {
        /* No ModRM: the wildcard region starts right after the opcode.  Count
         * the leading legacy prefixes, then 1 opcode byte (2 for the 0F
         * escape, 3 for 0F 38 / 0F 3A). */
        int p = 0;
        while (p < in->size && is_prefix_byte(in->bytes[p])) p++;
        int oplen = 1;
        if (p < in->size && in->bytes[p] == 0x0F) {
            oplen = 2;
            if (p + 1 < in->size &&
                (in->bytes[p+1] == 0x38 || in->bytes[p+1] == 0x3A)) oplen = 3;
        }
        first = p + oplen;
    }
    if (first && first < keep) keep = first;

    if (keep < 1) keep = 1;
    return keep;
}

static void record_form(ThreadStats *ts, const cs_insn *in, int have_detail,
                        const uint8_t *buf, uint64_t v)
{
    ts->raw++;

    if (g_canon) {
        char key[384];
        canon_key(ts->hd, in, key, sizeof key);
        int klen = (int)strlen(key);
        uint64_t h = fnv1a((const uint8_t *)key, klen);
        if ((ts->map.cnt+1)*10 >= ts->map.cap*7) hmap_grow(&ts->map);
        Slot *sl = hmap_slot(&ts->map, (const uint8_t *)key, klen, h);
        int cut = structural_len(in);
        if (sl->key) {
            if (in->size < sl->oplen ||
                (in->size == sl->oplen && v < sl->minv)) {
                sl->minv = v; sl->oplen = in->size; sl->opcut = cut;
                memcpy(sl->opbytes, buf, in->size);
            }
            return;
        }
        sl->key = (uint8_t *)dupstr(key); sl->klen = klen; sl->hash = h;
        sl->mnemonic = in->id; sl->minv = v;
        sl->oplen = in->size; sl->opcut = cut; memcpy(sl->opbytes, buf, in->size);
        ts->map.cnt++;
        return;
    }

    ts->forms++;
    ts->len_hist[in->size]++;
    ts->mnem[in->id]++;
    if (!ts->dump) return;
    int cut = have_detail ? structural_len(in) : in->size;
    for (int i = 0; i < in->size; i++)
        fprintf(ts->dump, "%s%02x", i==cut ? "|" : (i?" ":""), buf[i]);
    if (in->op_str[0])
        fprintf(ts->dump, "\t%s %s\n", in->mnemonic, in->op_str);
    else
        fprintf(ts->dump, "\t%s\n", in->mnemonic);
}

static void process_chunk(unsigned b0, ThreadStats *ts)
{
    const int N = g_nbytes;
    const uint64_t inner_bits = (uint64_t)(N-1)*8;
    const uint64_t inner_end  = (inner_bits >= 64) ? 0 : (1ULL << inner_bits);
    int want_detail = g_skip_imm || g_canon || (ts->dump != NULL);

    uint8_t buf[BUF_LEN];
    memset(buf, 0, sizeof buf);
    buf[0] = (uint8_t)b0;

    uint64_t lo = 0;
    while (lo < inner_end) {
        for (int i = 1; i < N; i++)
            buf[i] = (uint8_t)(lo >> ((N-1-i)*8));

        const uint8_t *c = buf; size_t s = PROBE_LEN; uint64_t a = 0;
        ts->decodes++;
        int keep;
        if (cs_disasm_iter(ts->hf, &c, &s, &a, ts->inf)) {
            const cs_insn *use = ts->inf; int have = 0;
            if (want_detail) {
                const uint8_t *cd = buf; size_t sd = PROBE_LEN; uint64_t ad = 0;
                ts->decodes++;
                if (cs_disasm_iter(ts->hd, &cd, &sd, &ad, ts->ind)) {
                    use = ts->ind; have = 1;
                }
            }
            keep = (g_skip_imm && have) ? structural_len(ts->ind) : use->size;
            if (keep > N) keep = N;
            uint64_t v = ((uint64_t)b0 << inner_bits) | lo;
            record_form(ts, use, have, buf, v);
        } else {
            keep = N;                            /* invalid: step by one      */
        }

        if (keep < 1) keep = 1;
        if (keep > N) keep = N;
        int shift = (N - keep) * 8;
        if (shift >= (int)inner_bits) break;
        uint64_t step = 1ULL << shift;
        lo = (lo & ~(step-1)) + step;
    }
}

static void *worker(void *arg)
{
    ThreadStats *ts = (ThreadStats *)arg;
    if (cs_open(CS_ARCH_X86, g_mode, &ts->hf) ||
        cs_open(CS_ARCH_X86, g_mode, &ts->hd)) {
        fprintf(stderr, "cs_open failed\n"); return NULL;
    }
    cs_option(ts->hf, CS_OPT_DETAIL, CS_OPT_OFF);
    cs_option(ts->hf, CS_OPT_SYNTAX, CS_OPT_SYNTAX_INTEL);
    cs_option(ts->hd, CS_OPT_DETAIL, CS_OPT_ON);
    cs_option(ts->hd, CS_OPT_SYNTAX, CS_OPT_SYNTAX_INTEL);
    ts->inf = cs_malloc(ts->hf);
    ts->ind = cs_malloc(ts->hd);

    if (g_dump && !g_canon) {
        char nm[512]; snprintf(nm, sizeof nm, "%s.%d", g_dump, ts->tid);
        ts->dump = fopen(nm, "w");
    }

    for (;;) {
        unsigned b0 = atomic_fetch_add(&g_next_chunk, 1);
        if (b0 >= g_b0_end) break;
        process_chunk(b0, ts);
        unsigned done = atomic_fetch_add(&g_done_chunks, 1) + 1;
        if (!g_quiet) fprintf(stderr, "\rchunks %3u/%-3u", done, g_b0_end-g_b0_start);
    }

    if (ts->dump) fclose(ts->dump);
    cs_free(ts->inf, 1); cs_free(ts->ind, 1);
    cs_close(&ts->hf); cs_close(&ts->hd);
    return NULL;
}

static double now_sec(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
    return t.tv_sec + t.tv_nsec*1e-9; }

static void usage(const char *p)
{
    fprintf(stderr,
        "usage: %s [options]   (Capstone-based mine32)\n"
        "  -t, --threads N     worker threads (default: 4)\n"
        "  -b, --bytes N       window size in bytes, 1..8 (default: 5)\n"
        "  -m, --mode M        16 | 32 | 64 (default: 32)\n"
        "      --no-skip-imm   count every disp/imm value as a distinct form\n"
        "      --canon         merge forms differing only in registers\n"
        "      --dump PREFIX   write found forms (PREFIX.<tid>; PREFIX in --canon)\n"
        "      --range A B     only first-bytes [A,B) (default 0 256)\n"
        "  -q, --quiet         no progress output\n", p);
}

int main(int argc, char **argv)
{
    int threads = 4, mode = 32;
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if ((!strcmp(a,"-t")||!strcmp(a,"--threads")) && i+1<argc) threads = atoi(argv[++i]);
        else if ((!strcmp(a,"-b")||!strcmp(a,"--bytes")) && i+1<argc) g_nbytes = atoi(argv[++i]);
        else if ((!strcmp(a,"-m")||!strcmp(a,"--mode")) && i+1<argc) mode = atoi(argv[++i]);
        else if (!strcmp(a,"--no-skip-imm")) g_skip_imm = 0;
        else if (!strcmp(a,"--canon")) g_canon = 1;
        else if (!strcmp(a,"--dump") && i+1<argc) g_dump = argv[++i];
        else if (!strcmp(a,"--range") && i+2<argc) {
            g_b0_start = (unsigned)strtoul(argv[++i],0,0);
            g_b0_end   = (unsigned)strtoul(argv[++i],0,0);
        } else if (!strcmp(a,"-q")||!strcmp(a,"--quiet")) g_quiet = 1;
        else if (!strcmp(a,"-h")||!strcmp(a,"--help")) { usage(argv[0]); return 0; }
        else { fprintf(stderr,"unknown option: %s\n",a); usage(argv[0]); return 1; }
    }
    if (g_nbytes < 1 || g_nbytes > MAX_BYTES) { fprintf(stderr,"bytes 1..%d\n",MAX_BYTES); return 1; }
    if (threads < 1) threads = 1;
    if (g_b0_end > 256) g_b0_end = 256;
    if (g_b0_start > g_b0_end) g_b0_start = g_b0_end;
    switch (mode) {
        case 16: g_mode = CS_MODE_16; break;
        case 32: g_mode = CS_MODE_32; break;
        case 64: g_mode = CS_MODE_64; break;
        default: fprintf(stderr,"mode must be 16,32,64\n"); return 1;
    }
    g_modebits = mode;
    if (cs_open(CS_ARCH_X86, g_mode, &g_names)) { fprintf(stderr,"cs_open failed\n"); return 1; }

    fprintf(stderr, "mining %d-bit space via Capstone (mode=%d, window=%d, threads=%d, "
        "skip-imm=%d, canon=%d, first-bytes=[%u,%u))\n",
        g_nbytes*8, mode, g_nbytes, threads, g_skip_imm, g_canon, g_b0_start, g_b0_end);

    atomic_store(&g_next_chunk, g_b0_start);
    atomic_store(&g_done_chunks, 0);
    ThreadStats *st = calloc(threads, sizeof *st);
    pthread_t *tid = calloc(threads, sizeof *tid);
    for (int i = 0; i < threads; i++) {
        st[i].tid = i; st[i].mnem = calloc(NMNEM, sizeof(uint64_t));
        if (g_canon) hmap_init(&st[i].map, 4096);
    }
    double t0 = now_sec();
    for (int i = 0; i < threads; i++) pthread_create(&tid[i], 0, worker, &st[i]);
    for (int i = 0; i < threads; i++) pthread_join(tid[i], 0);
    double dt = now_sec() - t0;
    if (!g_quiet) fputc('\n', stderr);

    uint64_t total=0, raw=0, decs=0, len_hist[MAX_ILEN+1]={0};
    uint64_t *mnem = calloc(NMNEM, sizeof(uint64_t));
    for (int i=0;i<threads;i++){ raw+=st[i].raw; decs+=st[i].decodes; }

    if (g_canon) {
        HMap g; hmap_init(&g, 1u<<16);
        for (int i=0;i<threads;i++){ HMap*lm=&st[i].map;
            for(size_t j=0;j<lm->cap;j++) if(lm->s[j].key) hmap_merge_slot(&g,&lm->s[j]);
            hmap_free(lm); }
        total = g.cnt;
        for(size_t j=0;j<g.cap;j++) if(g.s[j].key){ len_hist[g.s[j].oplen]++; mnem[g.s[j].mnemonic]++; }
        if (g_dump) {
            Slot **arr = malloc(g.cnt*sizeof(Slot*)); size_t nl=0;
            for(size_t j=0;j<g.cap;j++) if(g.s[j].key) arr[nl++]=&g.s[j];
            qsort(arr, nl, sizeof(Slot*), cmp_slot);
            FILE *f = fopen(g_dump,"w");
            if (f){ for(size_t j=0;j<nl;j++){ const Slot*sl=arr[j];
                for(int b=0;b<sl->oplen;b++) fprintf(f,"%s%02x", b==sl->opcut?"|":(b?" ":""), sl->opbytes[b]);
                fprintf(f,"\t%s\n",(char*)sl->key); } fclose(f);
                fprintf(stderr,"wrote canonical forms to %s\n", g_dump);
            } else fprintf(stderr,"cannot open %s\n", g_dump);
            free(arr);
        }
        hmap_free(&g);
    } else {
        for (int i=0;i<threads;i++){ total+=st[i].forms;
            for(int l=0;l<=MAX_ILEN;l++) len_hist[l]+=st[i].len_hist[l];
            for(int m=0;m<NMNEM;m++) mnem[m]+=st[i].mnem[m]; }
    }
    for (int i=0;i<threads;i++) free(st[i].mnem);

    printf("\n===== results (Capstone) =====\n");
    if (g_canon) {
        printf("canonical forms : %" PRIu64 "   (register variants merged)\n", total);
        printf("raw forms       : %" PRIu64 "\n", raw);
    } else printf("decodable forms : %" PRIu64 "\n", total);
    printf("decode calls    : %" PRIu64 "\n", decs);
    printf("elapsed         : %.2f s  (%.2f M decodes/s)\n", dt, decs/dt/1e6);

    printf("\nforms by length (bytes)%s:\n", g_canon?", shortest encoding":"");
    for (int l=1;l<=MAX_ILEN;l++) if (len_hist[l]) printf("  %2d : %" PRIu64 "\n", l, len_hist[l]);

    int distinct=0; for(int m=0;m<NMNEM;m++) if(mnem[m]) distinct++;
    printf("\ndistinct mnemonics: %d\n", distinct);
    printf("\ntop mnemonics by form count:\n");
    for (int shown=0;shown<40;shown++){ int best=-1;
        for(int m=0;m<NMNEM;m++) if(mnem[m] && (best<0||mnem[m]>mnem[best])) best=m;
        if(best<0) break;
        printf("  %-16s %" PRIu64 "\n", cs_insn_name(g_names,best), mnem[best]);
        mnem[best]=0;
    }
    free(mnem); free(st); free(tid); cs_close(&g_names);
    return 0;
}
