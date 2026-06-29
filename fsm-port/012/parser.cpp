// parser.cpp -- demo + self-test for the corpus-driven x86-32 (dis)assembler.
//
// The decoder (run_fsm, decode_insn, ...) and the encoder live in the shared
// headers x86dec.hpp / x86enc.hpp; this file is just the NASM-ish renderer
// (fmt_insn) plus a table of hand-picked instructions that it decodes, renders,
// and round-trips (decode -> x86insn_t -> encode) to prove both directions
// agree byte-for-byte. See asm32.cpp for the file-to-file tool.

#include "x86dec.hpp"   // shared decoder: run_fsm .. decode_insn, x86dec_t, byte/uint
#include <stdio.h>
// register name at the effective width: regnum + (8 if 16-bit) into greg[].
// addresses use the address size (67), the reg operand uses the operand size (66).
static const char* reg_name(int regnum, int sz16) {
  return greg[regnum + (sz16 ? 8 : 0)];
}

// render a register operand through its file (which name table it indexes)
static const char* reg_operand(int n, int file, int opsize) {
  switch (file) {
    case OPF_RGB:    return rgb[n & 7];
    case OPF_XMM:    return ssereg[8 + (n & 7)];
    case OPF_MM:     return ssereg[n & 7];
    case OPF_SREG:   return sreg[n % (int)sreg_size];
    case OPF_SSE_OS: return ssereg[(opsize ? 8 : 0) + (n & 7)];   // mm / xmm by opsize
    default:         return reg_name(n, opsize);                  // OPF_GREG
  }
}


// compose "seg:[base+index*scale+disp]" in NASM-ish form from the captures.
static void fmt_mem(const x86insn_t* in, int segidx, char* out) {
  char* p = out;
  int a16 = in->addr;                             // 16-bit address registers?
  int si = in->mem_seg + segidx;
  const char* sg = (si >= 0 && (size_t)si < seg_size) ? seg[si] : "";
  p += sprintf(p, "%s[", sg);
  int wrote = 0;
  if (in->mem_base != GREG_NONE) { p += sprintf(p, "%s", reg_name(in->mem_base, a16)); wrote = 1; }
  if (in->mem_index != GREG_NONE) {
    int sc = 1 << in->mem_scale;
    p += sprintf(p, "%s%s", wrote ? "+" : "", reg_name(in->mem_index, a16));
    if (sc != 1) p += sprintf(p, "*%d", sc);
    wrote = 1;
  }
  if (!wrote) {                                   // absolute: bare address
    p += sprintf(p, "0x%X", (uint)in->disp);
  } else if (in->disp != 0) {                     // signed offset
    int32_t d = in->disp;
    p += sprintf(p, d < 0 ? "-0x%X" : "+0x%X", (uint)(d < 0 ? -d : d));
  }
  sprintf(p, "]");
}



// compose the assembly text from the decoded captures, per operand FORM.
static void fmt_insn(const x86dec_t* d, size_t end, char* out) {
  const x86insn_t* in = &d->insn;
  const uint64_t* c = d->cap;
  if (in->mnem == 0xFFFF) { sprintf(out, "(bad)"); return; }
  int os = in->opsize, rf = (int)c[CAP_RFILE], mf = (int)c[CAP_MFILE];
  int is_mem = (c[CAP_MODE] == RM_MEM);                 // r/m is memory (a ModR/M ran)
  // mnemonic, with the memory-form size suffix when applicable (groups fold it in)
  char* p = out + sprintf(out, "%s%s", mnem_tab[in->mnem],
                          (is_mem && c[CAP_SFX]) ? sfx[c[CAP_SFX]] : "");
  switch ((int)c[CAP_FORM]) {
    case FORM_NONE:
      break;
    case FORM_MODRM: {
      char rm[64];
      if (c[CAP_MODE] == RM_REG) snprintf(rm, sizeof rm, "%s", reg_operand((int)c[CAP_RM], mf, os));
      else                       fmt_mem(in, (int)c[VAR_SEGIDX], rm);
      const char* reg = reg_operand((int)c[CAP_REG], rf, os);
      if (c[CAP_DIR]) p += sprintf(p, " %s, %s", rm, reg);    // r/m, reg
      else            p += sprintf(p, " %s, %s", reg, rm);    // reg, r/m
      break;
    }
    case FORM_RM: {                                    // single r/m operand (e.g. setcc)
      char rm[64];
      if (c[CAP_MODE] == RM_REG) snprintf(rm, sizeof rm, "%s", reg_operand((int)c[CAP_RM], mf, os));
      else                       fmt_mem(in, (int)c[VAR_SEGIDX], rm);
      p += sprintf(p, " %s", rm);
      break;
    }
    case FORM_REG:
      p += sprintf(p, " %s", reg_operand((int)c[CAP_REG], rf, os));
      break;
    case FORM_REG_IMM:
      p += sprintf(p, " %s, 0x%X", reg_operand((int)c[CAP_REG], rf, os), (uint)(uint32_t)c[CAP_IMM]);
      break;
    case FORM_IMM:
      p += sprintf(p, " 0x%X", (uint)(uint32_t)c[CAP_IMM]);
      break;
    case FORM_REL: {
      uint target = (uint)((int)end + (int32_t)c[CAP_REL]);
      p += sprintf(p, " 0x%X", target);
      break;
    }
    case FORM_PTR:
      p += sprintf(p, " 0x%X:0x%X", (uint)(uint32_t)c[CAP_DISP], (uint)(uint32_t)c[CAP_IMM]);
      break;
    case FORM_GROUP: {                                 // mnemonic (with size suffix) already printed
      char rm[64];
      if (c[CAP_MODE] == RM_REG) snprintf(rm, sizeof rm, "%s", reg_name((int)c[CAP_RM], os));
      else                       fmt_mem(in, (int)c[VAR_SEGIDX], rm);
      p += sprintf(p, " %s", rm);
      if (c[CAP_IMK] != IMK_NONE) p += sprintf(p, ", 0x%X", (uint)(uint32_t)c[CAP_IMM]);
      break;
    }
  }
  if (c[CAP_CC])                                       // setcc/cmovcc/jcc: condition as operand
    p += sprintf(p, ", %d", (int)c[CAP_CC] - 1);
}

static int g_fail = 0;
static int g_rtfail = 0;

static void hexstr(const byte* b, size_t n, char* out) {
  char* p = out;
  for (size_t i = 0; i < n; ++i) p += sprintf(p, "%s%02X", i ? " " : "", b[i]);
  if (!n) *p = 0;
}

static void show(const char* hexbytes, const char* want, const byte* s, size_t n) {
  x86dec_t d = {};
  size_t total = decode_insn(s, n, &d);
  char asm_[96];
  fmt_insn(&d, total, asm_);
  int ok = (strcmp(asm_, want) == 0);
  if (!ok) g_fail++;

  // round-trip: re-encode the decoded x86insn_t and compare to the input bytes.
  const char* rt;
  byte enc[16]; char encx[64];
  if (d.insn.mnem == 0xFFFF) {
    rt = "  rt: n/a";                                    // (bad): nothing to encode
  } else {
    size_t m = encode_insn(&d.insn, enc);
    if (m == total && memcmp(enc, s, total) == 0) {
      rt = "  rt: ok";
    } else {
      hexstr(enc, m, encx);
      rt = "  rt: FAIL";
      g_rtfail++;
    }
  }

  printf("  %-26s %-30s %s%s\n", hexbytes, asm_, rt,
         (!ok ? "  (decode != expected)" : ""));
  if (!ok)
    printf("%30s expected: %s\n", "", want);
  if (d.insn.mnem != 0xFFFF) {
    size_t m = encode_insn(&d.insn, enc);
    if (!(m == total && memcmp(enc, s, total) == 0)) {
      hexstr(enc, m, encx);
      printf("%30s re-encoded: %s\n", "", encx[0] ? encx : "(empty)");
    }
  }
}

int main() {
  printf("single-byte instruction decode (prefix -> opcode -> modrm/sib/disp):\n\n");

  { byte b[] = {0x90};                         show("90", "nop", b, sizeof b); }
  { byte b[] = {0xc3};                         show("C3", "ret", b, sizeof b); }
  { byte b[] = {0xf4};                         show("F4", "hlt", b, sizeof b); }
  { byte b[] = {0x40};                         show("40", "inc eax", b, sizeof b); }
  { byte b[] = {0x47};                         show("47", "inc edi", b, sizeof b); }
  { byte b[] = {0x53};                         show("53", "push ebx", b, sizeof b); }
  { byte b[] = {0x89, 0xd8};                   show("89 D8", "mov eax, ebx", b, sizeof b); }
  { byte b[] = {0x8b, 0x4c, 0xc3, 0x10};       show("8B 4C C3 10", "mov ecx, [ebx+eax*8+0x10]", b, sizeof b); }
  { byte b[] = {0x2e, 0x8b, 0x44, 0x9d, 0x08}; show("2E 8B 44 9D 08", "mov eax, cs:[ebp+ebx*4+0x8]", b, sizeof b); }
  { byte b[] = {0x01, 0xc8};                   show("01 C8", "add eax, ecx", b, sizeof b); }
  { byte b[] = {0x03, 0x06};                   show("03 06", "add eax, [esi]", b, sizeof b); }
  { byte b[] = {0x8d, 0x04, 0x88};             show("8D 04 88", "lea eax, [eax+ecx*4]", b, sizeof b); }
  { byte b[] = {0xb8, 0x78, 0x56, 0x34, 0x12}; show("B8 78 56 34 12", "mov eax, 0x12345678", b, sizeof b); }
  { byte b[] = {0x66, 0xb9, 0x34, 0x12};       show("66 B9 34 12", "mov cx, 0x1234", b, sizeof b); }
  { byte b[] = {0x05, 0x0a, 0x00, 0x00, 0x00}; show("05 0A 00 00 00", "add eax, 0xA", b, sizeof b); }
  { byte b[] = {0xeb, 0x10};                   show("EB 10", "jmp 0x12", b, sizeof b); }
  { byte b[] = {0x74, 0xfa};                   show("74 FA", "jcc 0xFFFFFFFC, 4", b, sizeof b); }
  { byte b[] = {0xe9, 0x05, 0x00, 0x00, 0x00}; show("E9 05 00 00 00", "jmp 0xA", b, sizeof b); }
  { byte b[] = {0xcd, 0x80};                   show("CD 80", "int 0x80", b, sizeof b); }
  { byte b[] = {0x67, 0x8b, 0x07};             show("67 8B 07", "mov eax, [bx]", b, sizeof b); }

  // opcode-extension groups (reg field /digit picks the mnemonic)
  { byte b[] = {0xff, 0xc0};                   show("FF C0", "inc eax", b, sizeof b); }
  { byte b[] = {0xff, 0x30};                   show("FF 30", "push.d [eax]", b, sizeof b); }
  { byte b[] = {0xff, 0xe3};                   show("FF E3", "jmp ebx", b, sizeof b); }
  { byte b[] = {0xf7, 0xe1};                   show("F7 E1", "mul ecx", b, sizeof b); }
  { byte b[] = {0xc1, 0xe0, 0x04};             show("C1 E0 04", "shl eax, 0x4", b, sizeof b); }
  { byte b[] = {0x83, 0xc0, 0x10};             show("83 C0 10", "add eax, 0x10", b, sizeof b); }
  { byte b[] = {0x83, 0xc3, 0xf0};             show("83 C3 F0", "add ebx, 0xFFFFFFF0", b, sizeof b); }
  { byte b[] = {0x81, 0xc1, 0x34, 0x12, 0x00, 0x00}; show("81 C1 34 12 00 00", "add ecx, 0x1234", b, sizeof b); }
  { byte b[] = {0xc7, 0x00, 0x01, 0x00, 0x00, 0x00}; show("C7 00 01 00 00 00", "mov.d [eax], 0x1", b, sizeof b); }
  { byte b[] = {0xc6, 0x42, 0x04, 0x7b};       show("C6 42 04 7B", "mov.b [edx+0x4], 0x7B", b, sizeof b); }
  { byte b[] = {0xff, 0x74, 0xcb, 0x08};       show("FF 74 CB 08", "push.d [ebx+ecx*8+0x8]", b, sizeof b); }
  { byte b[] = {0xff, 0xc8};                   show("FF C8", "dec eax", b, sizeof b); }   // ff /1 = dec

  // 8-bit r/m forms now carry the size suffix and rgb operands
  { byte b[] = {0x88, 0x00};                   show("88 00", "mov.b [eax], al", b, sizeof b); }
  // two-byte 0F map (movzx/movsx, setcc, cmov, cpuid, SSE/MMX)
  { byte b[] = {0x0f, 0xb6, 0xc1};             show("0F B6 C1", "movzx eax, cl", b, sizeof b); }
  { byte b[] = {0x0f, 0xb6, 0x00};             show("0F B6 00", "movzx.b eax, [eax]", b, sizeof b); }
  { byte b[] = {0x0f, 0xbe, 0xd3};             show("0F BE D3", "movsx edx, bl", b, sizeof b); }
  { byte b[] = {0x0f, 0xa2};                   show("0F A2", "cpuid", b, sizeof b); }
  { byte b[] = {0x0f, 0x90, 0xc1};             show("0F 90 C1", "setcc cl, 0", b, sizeof b); }
  { byte b[] = {0x0f, 0x95, 0x02};             show("0F 95 02", "setcc.b [edx], 5", b, sizeof b); }
  { byte b[] = {0x0f, 0x44, 0xc1};             show("0F 44 C1", "cmovcc eax, ecx, 4", b, sizeof b); }
  { byte b[] = {0x0f, 0x28, 0xc1};             show("0F 28 C1", "movaps xmm0, xmm1", b, sizeof b); }
  { byte b[] = {0x0f, 0x6f, 0xc1};             show("0F 6F C1", "movq mm0, mm1", b, sizeof b); }
  { byte b[] = {0x0f, 0x2e, 0xc1};             show("0F 2E C1", "ucomiss xmm0, xmm1", b, sizeof b); }
  { byte b[] = {0x66, 0x0f, 0x2e, 0xc1};       show("66 0F 2E C1", "ucomisd xmm0, xmm1", b, sizeof b); }
  { byte b[] = {0x0f, 0xfc, 0xc1};             show("0F FC C1", "paddb mm0, mm1", b, sizeof b); }
  { byte b[] = {0x66, 0x0f, 0xfc, 0xc1};       show("66 0F FC C1", "paddb xmm0, xmm1", b, sizeof b); }
  { byte b[] = {0x0f, 0xff, 0xc1};             show("0F FF C1", "ud0 eax, ecx", b, sizeof b); }   // ud0 r,r/m

  printf("\n  decode: %s\n", g_fail ? "  FAILURES present" : "all match expected");
  printf("  encode (lossless round-trip): %s\n",
         g_rtfail ? "  FAILURES present" : "all valid decodes re-encode to identical bytes");
  return (g_fail || g_rtfail) ? 1 : 0;
}

