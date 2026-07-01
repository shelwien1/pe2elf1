// asm32.cpp -- corpus-driven x86-32 (dis)assembler, file to file.
//
//   asm32 d  input.bin   insns.dat    decode machine code -> x86insn_t records
//   asm32 c  insns.dat   output.bin   encode x86insn_t records -> machine code
//
// The x86insn_t record is the bijection endpoint: it carries the semantic
// instruction PLUS the encoding witness (the `enc` twin choice and the
// addressing delta disp_w/sib/sscale), so  c(d(bytes)) == bytes  even for
// non-canonical encodings. Records are written as raw fixed-size structs, so
// the count is filesize/sizeof(x86insn_t) and the two modes are exact inverses.
//
// Decoding is only as complete as corpus.p: on an opcode the tables don't model
// (mnem == 0xFFFF) decode stops -- a stream can't be resynchronised past an
// instruction of unknown length -- and the stop offset is reported on stderr.

#include "x86dec.hpp"
#include "x86enc.hpp"
#include <stdio.h>
#include <stdlib.h>

static byte* read_file(const char* path, size_t* len) {
  FILE* f = fopen(path, "rb");
  if (!f) { fprintf(stderr, "asm32: cannot open %s\n", path); return 0; }
  fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
  byte* buf = (byte*)malloc(n > 0 ? n : 1);
  *len = fread(buf, 1, n, f);
  fclose(f);
  return buf;
}

// short operand-shape tag for the listing, e.g. "r,m" or "r,i"
static char opclass_ch(const x86op_t* o) {
  switch (o->type) {
    case T_GPR: case T_GPR8: case T_GPRdq: case T_GPRq: case T_GPRw: case T_SREG: return 'r';
    case T_XMM: case T_MMX:               return 'x';
    case T_YMM:                           return 'y';
    case T_ZMM:                           return 'z';
    case T_KREG: case T_MASK:             return 'k';
    case T_BND:                           return 'b';
    case T_MEM:                           return 'm';
    case T_IMM: case T_PTR:               return 'i';
    case T_REL:                           return 'l';
    default:                              return '-';
  }
}

// ---------------------------------------------------------------------------
// mode d: decode machine code into a stream of x86insn_t records
// ---------------------------------------------------------------------------
static int decode_mode(const char* inpath, const char* outpath) {
  size_t len = 0;
  byte* s = read_file(inpath, &len);
  if (!s) return 1;
  FILE* out = fopen(outpath, "wb");
  if (!out) { fprintf(stderr, "asm32: cannot write %s\n", outpath); free(s); return 1; }

  size_t ip = 0, ninsn = 0;
  while (ip < len) {
    x86dec_t d = {};
    size_t l = decode_insn(s + ip, len - ip, &d);
    if (d.insn.mnem == 0xFFFF || l == 0) {
      fprintf(stderr, "asm32: stop at offset %zu (byte 0x%02X): opcode not in corpus\n",
              ip, s[ip]);
      break;
    }
    // listing line: offset, length, mnemonic, operand-shape, addressing witness
    char ops[8]; int k = 0;
    for (int i = 0; i < d.insn.n_ops && k < 6; ++i) {
      if (i) ops[k++] = ',';
      ops[k++] = opclass_ch(&d.insn.op[i]);
    }
    ops[k] = 0;
    int has_mem = 0;
    for (int i = 0; i < d.insn.n_ops; ++i) if (d.insn.op[i].type == T_MEM) has_mem = 1;
    fprintf(stderr, "  %4zu  %2zuB  %-8s %-6s",
            ip, l, mnem_tab[d.insn.mnem], ops);
    if (has_mem)
      fprintf(stderr, "  [disp_w=%u sib=%u ss=%u%s]",
              d.insn.disp_w, d.insn.sib, d.insn.sscale, d.insn.enc ? " enc!=0" : "");
    else if (d.insn.enc)
      fprintf(stderr, "  [enc=%u]", d.insn.enc);
    fprintf(stderr, "\n");

    fwrite(&d.insn, sizeof(x86insn_t), 1, out);
    ip += l; ninsn++;
  }
  fclose(out);
  fprintf(stderr, "asm32: decoded %zu instruction(s), %zu/%zu byte(s) -> %s\n",
          ninsn, ip, len, outpath);
  free(s);
  return 0;
}

// ---------------------------------------------------------------------------
// mode c: encode a stream of x86insn_t records back into machine code
// ---------------------------------------------------------------------------
static int encode_mode(const char* inpath, const char* outpath) {
  size_t len = 0;
  byte* raw = read_file(inpath, &len);
  if (!raw) return 1;
  if (len % sizeof(x86insn_t)) {
    fprintf(stderr, "asm32: %s is not a whole number of x86insn_t records\n", inpath);
    free(raw); return 1;
  }
  FILE* out = fopen(outpath, "wb");
  if (!out) { fprintf(stderr, "asm32: cannot write %s\n", outpath); free(raw); return 1; }

  size_t cnt = len / sizeof(x86insn_t), total = 0, bad = 0;
  const x86insn_t* insns = (const x86insn_t*)raw;
  for (size_t i = 0; i < cnt; ++i) {
    byte buf[16];
    size_t m = encode_insn(&insns[i], buf);
    if (m == 0) {
      fprintf(stderr, "asm32: record %zu (mnem %u) could not be encoded\n", i, insns[i].mnem);
      bad++;
      continue;
    }
    fwrite(buf, 1, m, out);
    total += m;
  }
  fclose(out);
  fprintf(stderr, "asm32: encoded %zu record(s)%s, %zu byte(s) -> %s\n",
          cnt, bad ? " (some failed)" : "", total, outpath);
  free(raw);
  return bad ? 1 : 0;
}

int main(int argc, char** argv) {
  if (argc != 4 || (argv[1][0] != 'c' && argv[1][0] != 'd') || argv[1][1]) {
    fprintf(stderr, "usage: asm32 c|d input output\n"
                    "  d  decode machine code (input) into x86insn_t records (output)\n"
                    "  c  encode x86insn_t records (input) into machine code (output)\n");
    return 2;
  }
  return argv[1][0] == 'd' ? decode_mode(argv[2], argv[3])
                           : encode_mode(argv[2], argv[3]);
}
