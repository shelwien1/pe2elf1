// x86dec.hpp -- the decode half of the corpus-driven (dis)assembler, factored
// out of parser.cpp so both the demo (parser.cpp) and the tool (asm32.cpp) share
// one decoder. bytes -> x86insn_t. Pure: the renderer (fmt_*) lives in parser.cpp.
//
// One decode primitive: run_fsm() walks the flat generated FSM (x86_tables.h),
// running each state's actions into the capture array. decode_insn drives the
// prefix -> opcode -> ModR/M | immediate sequence; finalize_insn lowers the
// captures into a complete x86insn_t (operands in semantic order, immediate and
// condition hoisted) and records the encoding witness -- the `enc` twin choice
// and the addressing delta (disp width, redundant SIB, dead scale) -- so the
// encoder reproduces the exact input bytes, canonical or not.

#ifndef X86DEC_HPP
#define X86DEC_HPP

#include "x86_tables.h"
#include "x86insn.hpp"
#include "x86enc.hpp"
#include <stdint.h>
#include <string.h>

typedef uint8_t  byte;
typedef uint32_t uint;

// Decode context: the capture array (vars + addressing + per-group prefix
// offsets, one index space) plus the architectural result.
typedef struct {
  uint64_t  cap[NCAPS];
  x86insn_t insn;
} x86dec_t;

// ---------------------------------------------------------------------------
// the entire decoder: a generic flat-table walker. zero architecture inside.
// ---------------------------------------------------------------------------
static inline size_t run_fsm(uint32_t base, const byte* s, size_t len,
                             size_t ip, uint64_t* cap) {
  while (base != FSM_HALT && ip < len) {
    byte b = s[ip];
    const struct DState* st = &FSM_AT(base + b);
    if (st->act[0].op == ACT_NONE && st->next == FSM_HALT) break;   // dead state: not ours
    size_t off = ip;                                     // offset of this byte
    ip++;                                                // consume the index byte
    for (uint k = 0; k < FSM_MAX_ACT && st->act[k].op != ACT_NONE; ++k) {
      const struct Action* a = &st->act[k];
      switch (a->op) {
        case ACT_FIELD:
          cap[a->dst] = (b >> a->arg1) & ((1u << (a->arg0 - a->arg1 + 1)) - 1);
          break;
        case ACT_CONST:
          cap[a->dst] = (uint32_t)a->arg0 | ((uint32_t)a->arg1 << 5);  // 8-bit const (GRP > 31)
          break;
        case ACT_MARK:
          cap[a->dst] = (uint64_t)off;                   // remember this byte's offset
          break;
        case ACT_MNEM:
          cap[CAP_MNEM] = a->argx;                        // 13-bit wide const -> fixed cell
          break;
        default: {                                       // ACT_APPEND
          byte n = a->arg0;
          uint64_t v = 0;
          if (ip + n <= len) {
            if (ARCH_ENDIAN == 0)
              for (int j = (int)n - 1; j >= 0; --j) v = (v << 8) | s[ip + j];
            else
              for (byte j = 0; j < n; ++j) v = (v << 8) | s[ip + j];
            if (n < 8) { uint64_t sgn = (uint64_t)1 << (n * 8 - 1);
                         if (v & sgn) v |= ~((sgn << 1) - 1); }   // sign-extend to 64
            ip += n;
          }
          cap[a->dst] = v;
        }
      }
    }
    base = st->next;
  }
  return ip;
}

// ---------------------------------------------------------------------------
// thin entry points: pick a start table, run the same machine.
// ---------------------------------------------------------------------------
static inline size_t parse_prefixes(const byte* s, size_t len, x86dec_t* d) {
  for (int g = 0; g < VAR_COUNT; ++g) d->cap[CAP_POFF + g] = 0xFF;  // 'none'
  size_t lim = len < ARCH_MAXLEN ? len : ARCH_MAXLEN;              // corpus bound
  size_t ip = run_fsm(FSM_PREFIX, s, lim, 0, d->cap);

  d->insn.n_pfx = (uint8_t)ip;
  memcpy(d->insn.pfx, s, ip);
  for (int g = 0; g < VAR_COUNT; ++g)
    d->insn.pfx_off[g] = (uint8_t)d->cap[CAP_POFF + g];
  d->insn.has_pfx = ip ? 1 : 0;
  return ip;
}

// map captures -> instruction (the one spot that interprets capture meaning)
static inline void fill_insn(x86dec_t* d) {
  uint64_t* c = d->cap;
  x86insn_t* in = &d->insn;
  in->op[0].type  = T_GPR;
  in->op[0].index = c[CAP_REG];
  in->n_ops       = 2;
  if (c[CAP_MODE] == RM_REG) {            // register-direct r/m
    in->op[1].type  = T_GPR;
    in->op[1].index = c[CAP_RM];
    return;
  }
  in->op[1].type = T_MEM;
  in->mem_base   = c[CAP_BASE];
  in->mem_index  = c[CAP_INDEX];
  in->mem_scale  = c[CAP_SCALE];
  in->mem_seg    = c[CAP_SEG];            // seg[] base row; + segidx at render
  in->disp       = (int32_t)c[CAP_DISP];  // low 32 bits of the sign-extended value
}

// register file (OperandFile) -> x86op_t.type, resolving SSE_OS by operand size.
static inline int file_to_T(int opf, int opsize, int reptype) {
  switch (opf) {
    case OPF_RGB:    return T_GPR8;
    case OPF_XMM:    return T_XMM;
    case OPF_MM:     return T_MMX;
    case OPF_SREG:   return T_SREG;
    // mm/xmm by 66; F3/F2 (mandatory prefix on the xmm form: movdqu, pshufhw/lw,
    // ...) also selects the xmm bank even though opsize stays 0. Kept symmetric
    // with the encoder's enc_file_class so the bijection holds.
    case OPF_SSE_OS: return (opsize || reptype) ? T_XMM : T_MMX;
    // cvtpi2ps/cvttps2pi/...: mm at NP/66, GPR at F3/F2 (cvtsi2ss/cvttss2si).
    // The GPR width follows opsize (REX.W -> r64); mm is always 64-bit.
    case OPF_MMG:    return reptype ? T_GPR : T_MMX;
    case OPF_GREGd:  return T_GPRdq;                      // r32/r64 (never 16 on mandatory 66)
    case OPF_GREGq:  return T_GPRq;                       // r64 always (vmread/vmwrite)
    case OPF_GREGw:  return T_GPRw;                       // r16 always (movzx/movsx word src)
    case OPF_BND:    return T_BND;                        // MPX bound register (bnd0-3)
    default:         return T_GPR;                       // OPF_GREG
  }
}

// Read the addressing witness (disp width, redundant SIB, dead scale) straight
// from the ModR/M (+ SIB) bytes the decoder just consumed. These are the points
// where the byte->operand map is many-to-one; capturing them lets the encoder
// reproduce non-canonical addressing (disp8/disp32 of zero, gratuitous SIB,
// dead scale bits) byte-for-byte. modrm sits at op_at+tb+1 (the byte after the
// opcode, for both ModR/M and group forms).
static inline void capture_addr_witness(x86insn_t* in, const byte* s, size_t op_at, int tb) {
  in->disp_w = 0; in->sib = 0; in->sscale = 0;
  bool has_mem = false;
  for (int i = 0; i < in->n_ops; ++i) if (in->op[i].type == T_MEM) has_mem = true;
  if (!has_mem) return;

  // bytes before the final opcode. REX2 folds the 0F into its map bit (no escape
  // byte in the stream), so the opcode sits at op_at and the modrm at op_at+1.
  int oplen = in->rex2 ? 0 : (tb < 2 ? tb : 2);
  size_t mrm_at = op_at + oplen + 1;
  uint8_t modrm = s[mrm_at];
  int mod = modrm >> 6, rm = modrm & 7;
  // 64-bit (and 32-bit) use SIB/disp32 addressing; only x86-32 has the 16-bit form.
  // In 64-bit, mod=00 rm=101 is [rip+disp32] (still a 4-byte disp), so the same
  // disp-width rules apply.
  if (ARCH_MODE == 64 || in->addr == 0) {                // 32/64-bit addressing
    if (mod == 0)      in->disp_w = (rm == 5) ? 2 : 0;   // rm=101 -> [disp32]/[rip+disp32]
    else if (mod == 1) in->disp_w = 1;
    else               in->disp_w = 2;
    if (rm == 4) {                                       // a SIB byte is present
      uint8_t sib = s[mrm_at + 1];
      int sbase = sib & 7, sindex = (sib >> 3) & 7, sscl = sib >> 6;
      in->sib = 1;
      if (mod == 0 && sbase == 5) in->disp_w = 2;        // SIB no-base -> disp32
      if (sindex == 4) in->sscale = sscl;                // index absent: scale bits are dead
    }
  } else {                                               // 16-bit addressing
    if (mod == 0)      in->disp_w = (rm == 6) ? 2 : 0;   // rm=110 -> [disp16]
    else if (mod == 1) in->disp_w = 1;
    else               in->disp_w = 2;
  }
}

// Lower the faithful decode (in cap[]) into a complete x86insn_t -- the form the
// encoder round-trips. Operands are placed in SEMANTIC order (op[0] = first as
// written), the immediate/condition are hoisted, the addressing witness is read,
// and `enc` is stamped with which encoding twin the bytes used. cap[] is
// untouched, so the cap-based renderer (fmt_insn) is unaffected.
static inline void finalize_insn(x86dec_t* d, const byte* s, size_t slen, size_t op_at, int tb) {
  const uint64_t* c = d->cap;
  x86insn_t* in = &d->insn;
  int os = (int)in->opsize, form = (int)c[CAP_FORM];
  int rf = (int)c[CAP_RFILE], mf = (int)c[CAP_MFILE];
  int mode = (int)c[CAP_MODE], dir = (int)c[CAP_DIR];

  // x86-64: REX adds a 4th register bit. REX.R extends a ModR/M.reg operand,
  // REX.B an embedded (40+r/B8+r) reg or the rm/base, REX.X the SIB index. REX is
  // replayed from pfx[] on encode, so only the operand *numbers* are widened here.
  int xb = 0, reg_rex = 0, imm64_w = 0;
#if ARCH_MODE == 64
  int xr = 0, xx = 0;
  // The register-extension prefix is the last prefix byte (it must immediately
  // precede the opcode). Extract its bits here rather than spending scarce FSM
  // capture slots. Two encodings:
  //   * legacy REX (0x4x):  ....W R X B           -> reg/rm/index extend by 1 bit
  //   * APX  REX2 (D5 + payload): M0 R4 X4 B4 W R X B -> 2 bits each (-> r16-r31)
  // REX.R / R4 extend a ModR/M.reg operand; REX.B / B4 an embedded (40+r/B8+r) reg
  // or the rm/base; REX.X / X4 the SIB index. The byte is replayed on encode, so
  // only the operand register *numbers* are widened here.
  if (in->rex2) {
    int p = in->n_pfx ? in->pfx[in->n_pfx - 1] : 0;        // REX2 payload
    xr = ((p >> 2) & 1) | (((p >> 6) & 1) << 1);           // R | R4<<1  -> 0..3
    xx = ((p >> 1) & 1) | (((p >> 5) & 1) << 1);           // X | X4<<1
    xb = ( p       & 1) | (((p >> 4) & 1) << 1);           // B | B4<<1
    in->rex = 1;
    if (p & 8) { in->opsize = 2; os = 2; }                 // REX2.W -> 64-bit operand
  } else {
    int rexbyte = (in->n_pfx && (in->pfx[in->n_pfx - 1] & 0xF0) == 0x40)
                    ? in->pfx[in->n_pfx - 1] : 0;
    xr = (rexbyte >> 2) & 1; xx = (rexbyte >> 1) & 1; xb = rexbyte & 1;
    in->rex = rexbyte ? 1 : 0;
    if (rexbyte & 8) { in->opsize = 2; os = 2; }   // REX.W -> 64-bit operand
  }
  // mov r64,imm64 (movabs): B8+r at 64-bit operand size carries a full 64-bit
  // immediate. Key on os (== VAR_OPSIZ == 2), the same source append_imm/enc_imm
  // size the immediate from, so they agree even on pathological multi-REX input.
  imm64_w = (c[CAP_IMK] == IMK_IMMV && os == 2) ? 1 : 0;
  // embedded reg (40+r/B8+r) extends via REX.B; ModR/M.reg via REX.R; the
  // implicit accumulator (FORM_ACC) is not encoded, so it is never extended.
  reg_rex = (form == FORM_REG || form == FORM_REG_IMM) ? xb
          : (form == FORM_ACC) ? 0 : xr;
  // Default operand size 64 in long mode: push/pop (r64 and r/m64), near
  // indirect call/jmp, and pop r/m operate on 64-bit operands without REX.W.
  // Reflect that in the rendered operand width; the encoder replays REX from
  // pfx[], so widening opsize here does not perturb the round-tripped bytes.
  {
    int oplen_d = in->rex2 ? 0 : (tb < 2 ? tb : 2);
    uint8_t opb = s[op_at + oplen_d];
    int dig = (form == FORM_GROUP) ? ((s[op_at + oplen_d + 1] >> 3) & 7) : -1;
    bool d64 = tb == 0 && (                                          // one-byte map only:
               (opb >= 0x50 && opb <= 0x5f)                         // push/pop r64
            || (opb == 0x8f && dig == 0)                             // pop r/m64
            || (opb == 0xff && (dig == 2 || dig == 4 || dig == 6))); // call/jmp/push r/m64
    if (d64 && in->opsize == 0) { in->opsize = 2; os = 2; }
  }
#endif

  for (int i = 0; i < 5; ++i) { in->op[i].type = T_NONE; in->op[i].index = 0; }
  in->cc = c[CAP_CC] ? (uint8_t)(c[CAP_CC] - 1) : 0xFF;

  int n = 0;
  int rept = (int)c[VAR_REPTYPE];
  #define SETREG(slot, file, idx) do { in->op[slot].type = file_to_T(file, os, rept); \
                                       in->op[slot].index = ((idx) & 7) + 8 * reg_rex; } while (0)
  #define SETRM(slot) do { if (mode == RM_REG) { in->op[slot].type = file_to_T(mf, os, rept); \
                                                 in->op[slot].index = ((int)c[CAP_RM] & 7) + 8 * xb; } \
                           else { in->op[slot].type = T_MEM; in->op[slot].index = 0; } } while (0)
  switch (form) {
    case FORM_NONE: n = 0; break;
    case FORM_MODRM:
      if (dir) { SETRM(0); SETREG(1, rf, (int)c[CAP_REG]); }
      else     { SETREG(0, rf, (int)c[CAP_REG]); SETRM(1); }
      n = 2;
      if (c[CAP_IMK] != IMK_NONE) {                    // imul r,r/m,imm ; shld r/m,r,imm
        in->op[2].type = T_IMM; in->imm = (int32_t)c[CAP_IMM]; n = 3;
        if (c[CAP_IMK] == IMK_IMM8X2) {                // insertq xmm,xmm,imm8,imm8
          in->op[3].type = T_IMM2; in->disp = (int32_t)c[CAP_DISP]; n = 4;
        }
      }
      break;
    case FORM_RM:      SETRM(0); n = 1;
      if (c[CAP_IMK] != IMK_NONE) { in->op[1].type = T_IMM; in->imm = (int32_t)c[CAP_IMM]; n = 2;
        if (c[CAP_IMK] == IMK_IMM8X2) {                // extrq xmm,imm8,imm8
          in->op[2].type = T_IMM2; in->disp = (int32_t)c[CAP_DISP]; n = 3;
        }
      }
      break;
    case FORM_REG:     SETREG(0, rf, (int)c[CAP_REG]); n = 1; break;
    case FORM_ACC:                                  // implicit eAX/al + imm (reg_rex==0)
    case FORM_REG_IMM: SETREG(0, rf, (int)c[CAP_REG]);
                       in->op[1].type = T_IMM;
                       in->imm = imm64_w ? (int64_t)c[CAP_IMM] : (int32_t)c[CAP_IMM];
                       n = 2; break;
    case FORM_IMM:     in->op[0].type = T_IMM; in->imm = (int32_t)c[CAP_IMM]; n = 1; break;
    case FORM_REL:     in->op[0].type = T_REL; in->imm = (int32_t)c[CAP_REL]; n = 1; break;
    case FORM_GROUP:
      SETRM(0); n = 1;
      if (c[CAP_IMK] != IMK_NONE) { in->op[1].type = T_IMM; in->imm = (int32_t)c[CAP_IMM]; n = 2; }
      break;
    case FORM_PTR:     in->op[0].type = T_PTR; in->imm = (int32_t)c[CAP_IMM];
                       in->disp = (int32_t)c[CAP_DISP]; n = 1; break;
    default: n = 0;
  }
  in->n_ops = (uint8_t)n;
  #undef SETREG
  #undef SETRM

  // x86-64 memory operand: widen base/index by REX.B/REX.X, flag RIP-relative.
#if ARCH_MODE == 64
  in->rip = 0;
  bool fin_mem = false;
  for (int i = 0; i < in->n_ops; ++i) if (in->op[i].type == T_MEM) fin_mem = true;
  if (fin_mem) {
    if (in->mem_base == GREG_RIP) in->rip = 1;
    else if (in->mem_base < 8)    in->mem_base += 8 * xb;
    if (in->mem_index < 8)        in->mem_index += 8 * xx;
  }
#endif

  // Mandatory-66 GPR width: on an SSE op the 66 is a mnemonic/bank selector, not an
  // operand-size override, so a GPR operand stays 32-bit (64 under REX.W) -- movd,
  // movmskpd, pmovmskb, pextrb/w/d. Detect "66 alongside a vector register" and drop
  // opsize 1->0 so the GPR renders at 32-bit. Safe because: the vector operands are
  // already resolved to concrete T_XMM/T_MMX (file_to_T ran with the original os), the
  // 66 byte is replayed from pfx[] (not derived from opsize), and we exclude SSE_OS
  // operands (whose mm/xmm class the encoder rebuilds from opsize) and opsize-width
  // immediates (already consumed at the original size) so the bijection is untouched.
  if (os == 1 && rf != OPF_SSE_OS && mf != OPF_SSE_OS &&
      (c[CAP_IMK] == IMK_NONE || c[CAP_IMK] == IMK_IMM8 || c[CAP_IMK] == IMK_IMM8SX)) {
    bool has_vec = false;
    for (int i = 0; i < in->n_ops; ++i) {
      int t = in->op[i].type;
      if (t == T_XMM || t == T_MMX || t == T_YMM || t == T_ZMM) has_vec = true;
    }
    if (has_vec) { in->opsize = 0; os = 0; }
  }

  capture_addr_witness(in, s, op_at, tb);

  // FORM_RM has no register operand, so the ModR/M reg field is dead -- but it is
  // present in the byte and must be replayed (e.g. non-canonical setcc reg bits).
  in->reg_w = 0;
  if (form == FORM_RM) {
    int oplen2 = in->rex2 ? 0 : (tb < 2 ? tb : 2);
    in->reg_w = (s[op_at + oplen2 + 1] >> 3) & 7;
  }

  // stamp enc: which matching candidate the decoded opcode/digit corresponds to
  int oplen = in->rex2 ? 0 : (tb < 2 ? tb : 2);          // REX2: no 0F escape byte
  uint8_t opbyte = s[op_at + oplen];
  int digit = 0xFF;
  if (form == FORM_GROUP) digit = (s[op_at + oplen + 1] >> 3) & 7;
  // the ModR/M byte (if any) distinguishes same-opcode fixed-ModR/M ops (endbr64 vs
  // endbr32, monitor vs mwait); bounded so a no-ModR/M op at the buffer end can't OOB.
  uint8_t modrm = (op_at + oplen + 1 < slen) ? s[op_at + oplen + 1] : 0;
  enc_stamp(in, tb, opbyte, digit, modrm);
}

static inline size_t parse_addr(const byte* s, size_t len, size_t ip, x86dec_t* d) {
  uint32_t start = (uint32_t)(FSM_MODRM + d->cap[VAR_ADRSIZ] * 256);
  ip = run_fsm(start, s, len, ip, d->cap);
  fill_insn(d);
  d->insn.opsize = (uint16_t)d->cap[VAR_OPSIZ];   // 66 effect (operand width)
  d->insn.addr   = (uint16_t)d->cap[VAR_ADRSIZ];  // 67 effect (address width)
  return ip;
}

// append a trailing immediate per CAP_IMK into CAP_IMM / CAP_REL (far-pointer
// selector -> CAP_DISP). immz/relz width follows opsize.
static inline size_t append_imm(uint64_t* cap, const byte* s, size_t len, size_t ip, int opsiz) {
  // opsiz: 0=32-bit, 1=16-bit (66), 2=64-bit (REX.W). immz/relz are 2 bytes only
  // at 16-bit operand size; at 32- and 64-bit they are a 32-bit (sign-extended)
  // immediate. immv (mov r,imm) is the full operand-size immediate: 64 under REX.W.
  int w = 0, dst = CAP_IMM, sext = 0;
  switch ((int)cap[CAP_IMK]) {
    case IMK_NONE:  return ip;
    case IMK_IMM8:  w = 1; break;
    case IMK_IMM8SX: w = 1; sext = 1; break;
    case IMK_IMM16: w = 2; break;
    case IMK_IMM32: w = 4; break;
    case IMK_IMMZ:  w = (opsiz == 1) ? 2 : 4; break;
    case IMK_IMMV:  w = (opsiz == 1) ? 2 : (opsiz == 2) ? 8 : 4; break;  // mov r,imm: 64 under REX.W
    case IMK_REL8:  w = 1; dst = CAP_REL; sext = 1; break;
    case IMK_RELZ:  w = (opsiz == 1) ? 2 : 4; dst = CAP_REL; sext = 1; break;
    case IMK_PTR:                                   // imm32 offset : imm16 selector
      if (ip + 6 <= len) {
        uint64_t off = 0, sel = 0;
        for (int j = 3; j >= 0; --j) off = (off << 8) | s[ip + j];
        for (int j = 1; j >= 0; --j) sel = (sel << 8) | s[ip + 4 + j];
        cap[CAP_IMM] = off; cap[CAP_DISP] = sel; ip += 6;
      }
      return ip;
    case IMK_ENTER:                                 // enter: imm16 frame : imm8 level
      if (ip + 3 <= len) {
        uint64_t a = s[ip] | (s[ip + 1] << 8);
        cap[CAP_IMM] = a; cap[CAP_DISP] = s[ip + 2]; ip += 3;
      }
      return ip;
    case IMK_IMM8X2:                                // extrq/insertq: imm8 : imm8
      if (ip + 2 <= len) { cap[CAP_IMM] = s[ip]; cap[CAP_DISP] = s[ip + 1]; ip += 2; }
      return ip;
  }
  if (ip + w <= len) {
    uint64_t v = 0;
    for (int j = w - 1; j >= 0; --j) v = (v << 8) | s[ip + j];
    if (sext && w < 8) { uint64_t sgn = (uint64_t)1 << (w * 8 - 1); if (v & sgn) v |= ~((sgn << 1) - 1); }
    cap[dst] = v; ip += w;
  }
  return ip;
}

// vector operand type from the vector-length field (VEX L: 0/1; EVEX L'L: 0/1/2)
static inline int vex_vec_type(int L) { return (L >= 2) ? T_ZMM : L ? T_YMM : T_XMM; }

#if ARCH_MODE == 64
// One VEX operand from a VEX file code: 0=vec(by vt), 1=gpr32, 2=gpr64, 3=kreg,
// 4=vec one size class down (eregh: zmm->ymm else xmm), 5=xmm (eregx). LL is the
// EVEX L'L (0/1/2) used to size the narrowed forms.
static inline x86op_t vex_mkop(x86insn_t* in, int fc, int idx, int vt, int LL) {
  x86op_t o; o.index = 0; o.type = T_NONE;
  switch (fc) {
    case 1: o.type = T_GPR;  o.index = idx; break;              // 32-bit GPR
    case 2: o.type = T_GPR;  o.index = idx; in->opsize = 2; break;  // 64-bit GPR (W=1)
    case 3: o.type = T_KREG; o.index = idx & 7; break;          // mask register
    case 4: o.type = (LL >= 2) ? T_YMM : T_XMM; o.index = idx; break;  // half width
    case 5: o.type = T_XMM;  o.index = idx; break;              // quarter/eighth -> xmm
    default: o.type = vt;    o.index = idx; break;              // vector (xmm/ymm/zmm by L)
  }
  return o;
}

// Lower an FSM-decoded VEX insn (C4/C5) into operands, from the captures the
// vexp1/vexp2/vexop states wrote: CAP_FORM (a VEX_* shape), CAP_RFILE/CAP_MFILE
// (reg/rm file codes), CAP_RXB (R'X'B'), CAP_VVVV (raw vvvv), CAP_VL (L). The
// modrm/SIB/disp FSM already ran (fill_insn populated mem_*). The raw prefix +
// opcode + modrm are snapshotted into vex1/2/op/modrm so the byte-exact replay
// encoder is unchanged -- reordering op[] for display cannot affect the bytes.
static inline void vex_finalize(x86dec_t* d, const byte* s, size_t op_at) {
  x86insn_t* in = &d->insn;
  const uint64_t* c = d->cap;
  int form = (int)c[CAP_FORM];
  byte p0 = s[op_at];
  in->vex = (p0 == 0xC5) ? 1 : (p0 == 0xC4) ? 2 : (p0 == 0x8F) ? 4 : 3;  // C5/C4/XOP/EVEX
  in->rip = 0; in->rex = 0; in->zero = 0; in->bcst = 0; in->rc = 0;
  // snapshot the raw prefix bytes for the replay encoder; de-invert the
  // extension bits + vvvv + length, all from the captures the FSM wrote.
  int R, X, B, Rp = 0, Vp = 0, vvvv, LL, aaa = 0, z = 0, bbit = 0;
  size_t q = op_at + 1;
  if (in->vex == 3) {                                    // EVEX: 62 P0 P1 P2 opcode
    in->vex1 = s[q]; in->vex2 = s[q + 1]; in->vex3 = s[q + 2]; q += 3;
    int rxb = (int)c[CAP_RXB];                           // P0 bits 7:4 = R'X'B'R''
    R = 1 - ((rxb >> 3) & 1); X = 1 - ((rxb >> 2) & 1);
    B = 1 - ((rxb >> 1) & 1); Rp = 1 - (rxb & 1);
    int p2 = (int)c[CAP_VL];                             // whole EVEX P2 byte
    z = (p2 >> 7) & 1; LL = (p2 >> 5) & 3; bbit = (p2 >> 4) & 1;
    Vp = 1 - ((p2 >> 3) & 1); aaa = p2 & 7;
    vvvv = ((~(int)c[CAP_VVVV]) & 0xf) | (Vp << 4);      // 5-bit vvvv (V' high bit)
  } else {                                               // VEX C4/C5 or XOP (8F)
    if (in->vex == 1) { in->vex1 = s[q]; in->vex2 = in->vex3 = 0; q += 1; }
    else              { in->vex1 = s[q]; in->vex2 = s[q + 1]; in->vex3 = 0; q += 2; }
    int rxb = (int)c[CAP_RXB];
    if (in->vex == 2 || in->vex == 4) {                  // C4 / XOP: R'X'B' (3 bits)
      R = 1 - ((rxb >> 2) & 1); X = 1 - ((rxb >> 1) & 1); B = 1 - (rxb & 1);
    } else            { R = 1 - (rxb & 1); X = 0; B = 0; }  // C5: R' only
    LL = (int)c[CAP_VL]; vvvv = (~(int)c[CAP_VVVV]) & 0xf;
  }
  size_t opc_at = q; in->vex_op = s[q++];
  bool has_modrm = (form != FORM_VEX_NONE);
  in->vex_modrm = has_modrm ? s[q] : 0;
  bool is_reg = (c[CAP_MODE] == RM_REG);
  // VEX 0F 12/16 NP: the mem form loads (vmovlps/vmovhps); the reg-direct form is
  // vmovhlps/vmovlhps. One VEX descriptor per opcode, so swap here (encode replays
  // raw vex bytes, not the mnemonic, so this is decode-display only).
  if (in->vex != 3) {
    if (in->mnem == MNEM_VMOVLPS && is_reg) in->mnem = MNEM_VMOVHLPS;
    else if (in->mnem == MNEM_VMOVHPS && is_reg) in->mnem = MNEM_VMOVLHPS;
  }
  // EVEX decorations: b=1 on a reg-reg form is embedded rounding {er} (L'L picks
  // the mode and the operands are zmm); on a memory form it is broadcast {1toN}.
  int vt;
  if (in->vex == 3 && bbit && is_reg) { in->rc = LL + 1; vt = T_ZMM; }
  else                                 vt = vex_vec_type(LL);
  if (in->vex == 3) { in->zero = z; if (bbit && !is_reg) in->bcst = 1; }
  in->opsize = 0;
  in->imm = (int64_t)(int32_t)c[CAP_IMM];                // any imm8 appended into CAP_IMM
  int mreg = (int)c[CAP_REG], mrm = (int)c[CAP_RM];
  int rf = (int)c[CAP_RFILE], mf = (int)c[CAP_MFILE];

  // memory r/m: fold the high extension bits into base/index (the encoder masks
  // to 3 bits, so this stays byte-exact) + capture the addressing witness.
  if (has_modrm && !is_reg) {
    if (in->mem_base == GREG_RIP) in->rip = 1;
    else if (in->mem_base < 8)    in->mem_base += 8 * B;
    if (in->mem_index < 8)        in->mem_index += 8 * X;
    capture_addr_witness(in, s, opc_at, 0);
  }

  x86op_t regop  = vex_mkop(in, rf, mreg + 8 * R + 16 * Rp, vt, LL);
  x86op_t vvvvop = vex_mkop(in, rf, vvvv, vt, LL);
  x86op_t rmreg  = vex_mkop(in, mf, mrm + 8 * B + 16 * X, vt, LL);
  x86op_t memop; memop.type = T_MEM; memop.index = 0;
  x86op_t rmop   = is_reg ? rmreg : memop;
  x86op_t immop; immop.type = T_IMM; immop.index = 0;
  x86op_t is4op; is4op.type = vt; is4op.index = (int)((in->imm >> 4) & 0xf);

  int nn = 0;
  switch (form) {
    case FORM_VEX_RVM:  in->op[nn++]=regop; in->op[nn++]=vvvvop; in->op[nn++]=rmop; break;
    case FORM_VEX_RVMI: in->op[nn++]=regop; in->op[nn++]=vvvvop; in->op[nn++]=rmop; in->op[nn++]=immop; break;
    case FORM_VEX_RVMR: in->op[nn++]=regop; in->op[nn++]=vvvvop; in->op[nn++]=rmop; in->op[nn++]=is4op; break;
    case FORM_VEX_RM:   in->op[nn++]=regop; in->op[nn++]=rmop; break;
    case FORM_VEX_RMI:  in->op[nn++]=regop; in->op[nn++]=rmop; in->op[nn++]=immop; break;
    case FORM_VEX_MR:   in->op[nn++]=rmop;  in->op[nn++]=regop; break;
    case FORM_VEX_MRI:  in->op[nn++]=rmop;  in->op[nn++]=regop; in->op[nn++]=immop; break;
    case FORM_VEX_VM:   in->op[nn++]=vvvvop; in->op[nn++]=rmop; break;
    case FORM_VEX_R:    in->op[nn++]=regop; break;
    case FORM_VEX_M:    in->op[nn++]=rmop; break;
    case FORM_VEX_RVMV: in->op[nn++]=regop; in->op[nn++]=vvvvop; in->op[nn++]=rmop; in->op[nn++]=vvvvop; break;
    case FORM_VEX_RMV:  in->op[nn++]=regop; in->op[nn++]=rmop; in->op[nn++]=vvvvop; break;   // XOP vprot/vpsh W0
    case FORM_VEX_RVRM: in->op[nn++]=regop; in->op[nn++]=vvvvop; in->op[nn++]=is4op; in->op[nn++]=rmop; break; // XOP vpcmov/vpperm W1
    case FORM_VEX_VMI:  in->op[nn++]=vvvvop; in->op[nn++]=rmop; in->op[nn++]=immop; break;
    case FORM_VEX_VMG:  // shift-by-imm group: dest=vvvv, src=r/m, imm; mnem by /digit
      in->op[nn++]=vvvvop; in->op[nn++]=rmop; in->op[nn++]=immop;
      in->mnem = (uint16_t)vexgrp[(int)c[CAP_GRP]][mreg];     // mreg == ModR/M reg = /digit
      break;
    case FORM_VEX_MG:   // XOP LWP llwpcb/slwpcb: r/m only; mnem by /digit
      in->op[nn++]=rmop;
      in->mnem = (uint16_t)vexgrp[(int)c[CAP_GRP]][mreg];
      break;
    case FORM_VEX_VMG0: // XOP TBM blc*/bls*/tzmsk/t1mskc: dest=vvvv, src=r/m; mnem by /digit
      in->op[nn++]=vvvvop; in->op[nn++]=rmop;
      in->mnem = (uint16_t)vexgrp[(int)c[CAP_GRP]][mreg];
      break;
    default: break;                                            // FORM_VEX_NONE
  }
  // EVEX writemask {kN}: a positional operand right after the destination.
  if (in->vex == 3 && aaa && nn < 5) {
    for (int i = nn; i > 1; --i) in->op[i] = in->op[i - 1];
    in->op[1].type = T_MASK; in->op[1].index = aaa;
    nn++;
  }
  in->n_ops = (uint8_t)nn;
}

// One APX GPR operand: file 6 (rgb) -> 8-bit; otherwise an opsize-sized GPR.
static inline x86op_t apx_gpr(int file, int idx) {
  x86op_t o; o.index = (uint16_t)(idx & 31);
  // file: 6=rgb (8-bit GPR), 0=ssereg (xmm, for the SHA/AES map-4 ops), else opsize GPR
  o.type = (file == 6) ? T_GPR8 : (file == 0) ? T_XMM : T_GPR;
  return o;
}

// Lower an APX EVEX-promoted-legacy insn (62, map 4) into GPR operands. The FSM
// captured P0[7:3]=R3.X3.B3.R4.B4 (CAP_RXB), P1[6:2]=vvvv<<1|X4 (CAP_VVVV), the
// whole P2 byte (CAP_VL), the legacy operand shape (CAP_FORM) + GPR files, and set
// VAR_OPSIZ from (pp,W).  We fold the r0-31 extension, apply ND (the new-data dest
// rides vvvv -> prepended) and NF.  The EVEX prefix bytes are snapshotted for the
// byte-exact replay encoder (vex==3); apx==1 marks the GPR (legacy) lowering.
static inline void apx_finalize(x86dec_t* d, const byte* s, size_t op_at) {
  x86insn_t* in = &d->insn;
  const uint64_t* c = d->cap;
  int form = (int)c[CAP_FORM];
  in->vex = 3; in->apx = 1; in->rip = 0; in->rex = 0;
  in->zero = 0; in->bcst = 0; in->rc = 0; in->cc = 0xFF;
  in->opsize = (uint16_t)c[VAR_OPSIZ];                 // 0=32 1=16 2=64, set by apxp1 (pp,W)
  size_t q = op_at + 1;
  in->vex1 = s[q]; in->vex2 = s[q + 1]; in->vex3 = s[q + 2]; q += 3;
  int rxb = (int)c[CAP_RXB];                            // P0[7:3] = R3 X3 B3 R4 B4
  int R3 = 1 - ((rxb >> 4) & 1), X3 = 1 - ((rxb >> 3) & 1), B3 = 1 - ((rxb >> 2) & 1);
  int R4 = 1 - ((rxb >> 1) & 1);
  int B4 = rxb & 1;                                     // B4 (P0.3) is NOT inverted (1 -> +16)
  int vv = (int)c[CAP_VVVV];                            // P1[6:2] = vvvv<<1 | X4 (inverted)
  int X4 = 1 - (vv & 1);
  int p2 = (int)c[CAP_VL];                              // whole P2 byte
  int ND = (p2 >> 4) & 1, V4 = 1 - ((p2 >> 3) & 1);
  int ndd = ((~(vv >> 1)) & 0xf) | (V4 << 4);           // NDD dest register 0..31
  in->nf = (uint8_t)((p2 >> 2) & 1);                    // NF (no-flags)
  in->vex_op = s[q]; size_t opc_at = q; q++;
  in->vex_modrm = s[q];
  bool is_reg = (c[CAP_MODE] == RM_REG);
  int mreg = (int)c[CAP_REG], mrm = (int)c[CAP_RM];
  int rf = (int)c[CAP_RFILE], mf = (int)c[CAP_MFILE];
  in->imm = (int64_t)(int32_t)c[CAP_IMM];
  // memory r/m: base uses B3(8s)+B4(16s); index uses X3(8s)+X4(16s).
  if (!is_reg) {
    if (in->mem_base == GREG_RIP)      in->rip = 1;
    else if (in->mem_base != GREG_NONE) in->mem_base += 8 * B3 + 16 * B4;
    if (in->mem_index != GREG_NONE)     in->mem_index += 8 * X3 + 16 * X4;
    capture_addr_witness(in, s, opc_at, 0);
  }
  int reg = mreg + 8 * R3 + 16 * R4;                    // ModR/M.reg -> r0..31
  // reg-direct r/m: 8s = B3 always; the 16s bit differs by file -- GPR uses B4 (P0.3);
  // vector (SHA/AES xmm) accepts either X3 (AVX-512 reg-direct convention) or B4, matching
  // binutils' loose APX decode (objdump 2.42 extends an xmm r/m on either bit).
  int rmr = mrm + 8 * B3 + 16 * ((mf == 0) ? (X3 | B4) : B4);
  x86op_t regop = apx_gpr(rf, reg);
  x86op_t rmreg = apx_gpr(mf, rmr);
  x86op_t memop; memop.type = T_MEM; memop.index = 0;
  x86op_t rmop = is_reg ? rmreg : memop;
  x86op_t immop; immop.type = T_IMM; immop.index = 0;
  // NDD dest size = the reg file when present, else the r/m file (group forms have
  // no reg operand, so the dest follows the r/m width: 8-bit for 80/c0/f6/fe, etc.)
  x86op_t nddop = apx_gpr(rf ? rf : (mf ? mf : 1), ndd);
  // push2/pop2 (ff /6, 8f /0): a 64-bit register pair (vvvv, r/m). ND is a required
  // encoding marker here, not a new-data dest, so vvvv is a direct operand (not
  // prepended) and the pair is always 64-bit regardless of W (W picks the {,p} variant).
  bool is_v2 = is_reg && ((in->vex_op == 0xff && mreg == 6) || (in->vex_op == 0x8f && mreg == 0));
  int nn = 0;
  // NDD prepends the vvvv dest -- but only for NDD-capable ops (CAP_MNSEL flags the
  // non-NDD map-4 tail: movbe/crc32/atomics/MSR/CET/VMX/SHA/AES) and not the register pair.
  if (ND && !is_v2 && !c[CAP_MNSEL]) in->op[nn++] = nddop;
  switch (form) {
    case FORM_APX_MR:  in->op[nn++] = rmop;  in->op[nn++] = regop;
                       // shld/shrd by cl (a5/ad): r/m, reg, cl
                       if (in->vex_op == 0xa5 || in->vex_op == 0xad) {
                         x86op_t o; o.type = T_GPR8; o.index = 1; in->op[nn++] = o;   // cl
                       }
                       break;
    case FORM_APX_RM:  in->op[nn++] = regop; in->op[nn++] = rmop;
                       if (in->vex_op == 0xdb && rf == 0) {  // sha256rnds2 (xmm): implicit <xmm0>
                         x86op_t o; o.type = T_XMM; o.index = 0; in->op[nn++] = o;
                       }
                       break;
    case FORM_APX_RMI: in->op[nn++] = regop; in->op[nn++] = rmop; in->op[nn++] = immop; break;
    case FORM_APX_R:   in->op[nn++] = regop; break;
    case FORM_APX_MI:  in->op[nn++] = rmop; in->op[nn++] = immop;
                       in->mnem = (uint16_t)vexgrp[(int)c[CAP_GRP]][mreg]; break;
    case FORM_APX_M:
                       if (is_v2) {                       // push2/pop2: 64-bit reg pair
                         in->opsize = 2;
                         x86op_t v; v.type = T_GPR; v.index = (uint16_t)(ndd & 31); in->op[nn++] = v;
                         x86op_t m; m.type = T_GPR; m.index = (uint16_t)(rmr & 31); in->op[nn++] = m;
                       } else {
                         in->op[nn++] = rmop;
                         // shift groups d0/d1 (by 1) and d2/d3 (by cl) carry an implicit
                         // count operand; not/neg/inc/dec (f6/f7/fe/ff) take only r/m.
                         if (in->vex_op == 0xd0 || in->vex_op == 0xd1) {
                           in->imm = 1; x86op_t o; o.type = T_IMM; o.index = 0; in->op[nn++] = o;
                         } else if (in->vex_op == 0xd2 || in->vex_op == 0xd3) {
                           x86op_t o; o.type = T_GPR8; o.index = 1; in->op[nn++] = o;   // cl
                         }
                       }
                       in->mnem = (uint16_t)vexgrp[(int)c[CAP_GRP]][mreg]; break;
    case FORM_APX_MRI: in->op[nn++] = rmop; in->op[nn++] = regop; in->op[nn++] = immop; break;
    default: break;
  }
  // f8 pp2/pp3 swap the mnemonic by mod: reg-direct is uwrmsr/urdmsr, a memory r/m is
  // enqcmds/enqcmd (reg, m -- RM order regardless of the reg-direct form).
  if (in->vex_op == 0xf8 && !is_reg &&
      (in->mnem == MNEM_UWRMSR || in->mnem == MNEM_URDMSR)) {
    in->mnem = (in->mnem == MNEM_UWRMSR) ? MNEM_ENQCMDS : MNEM_ENQCMD;
    in->op[0] = regop; in->op[1] = memop; nn = 2;
  }
  in->n_ops = (uint8_t)nn;
}
#endif // ARCH_MODE == 64

// Structural decode of EVEX (62) and XOP (8F): the prefix bytes are captured raw
// for replay and the map+opcode size the ModR/M+immediate tail, but no real
// mnemonic/operands are produced yet (mnem = MNEM_VEX). VEX (C4/C5) no longer
// reaches here in 64-bit mode -- the FSM decodes it via vex_finalize.
static inline size_t vex_decode(x86dec_t* d, const byte* s, size_t n, size_t ip) {
  x86insn_t* in = &d->insn;
  byte p0 = s[ip];
  in->vex = (p0 == 0xC5) ? 1 : (p0 == 0xC4) ? 2 : (p0 == 0x62) ? 3 : 4;  // C5/C4/62/8F
  ip++;
  int map, L;
  if (in->vex == 1) {                              // C5: one payload byte, implied 0F
    if (ip >= n) { in->mnem = 0xFFFF; return ip; }
    in->vex1 = s[ip]; in->vex2 = in->vex3 = 0; ip++;
    map = 1; L = (in->vex1 >> 2) & 1;
  } else if (in->vex == 2 || in->vex == 4) {       // C4 (VEX) / 8F (XOP): two payload bytes
    if (ip + 1 >= n) { in->mnem = 0xFFFF; return ip; }
    in->vex1 = s[ip]; in->vex2 = s[ip + 1]; in->vex3 = 0; ip += 2;
    map = in->vex1 & 0x1f; L = (in->vex2 >> 2) & 1;
  } else {                                          // 62: EVEX, three payload bytes
    if (ip + 2 >= n) { in->mnem = 0xFFFF; return ip; }
    in->vex1 = s[ip]; in->vex2 = s[ip + 1]; in->vex3 = s[ip + 2]; ip += 3;
    map = in->vex1 & 0x07; L = (in->vex3 >> 5) & 3;       // L'L: 0=128 1=256 2=512
  }
  if (ip >= n) { in->mnem = 0xFFFF; return ip; }
  byte op = s[ip]; in->vex_op = op; size_t op_at = ip; ip++;
  int has_modrm, imm_len;
  vex_structure(map, op, (int)in->opsize, &has_modrm, &imm_len);
  in->n_ops = 0;
  in->opsize = 0;
  in->vex_modrm = 0;
  in->addr = (uint16_t)d->cap[VAR_ADRSIZ];         // needed by capture_addr_witness
  if (has_modrm) {
    if (ip >= n) { in->mnem = 0xFFFF; return ip; }
    in->vex_modrm = s[ip];                         // raw ModR/M (replayed by the encoder)
    uint32_t mstart = (uint32_t)(FSM_MODRM + d->cap[VAR_ADRSIZ] * 256);
    ip = run_fsm(mstart, s, n, ip, d->cap);
    fill_insn(d);
    int rt = vex_vec_type(L);                            // L picks xmm/ymm/zmm (display)
    in->op[0].type = rt;
    if (in->op[1].type == T_GPR) in->op[1].type = rt;
    capture_addr_witness(in, s, op_at, 0);         // modrm sits at op_at + 1
#if ARCH_MODE == 64
    if (in->mem_base == GREG_RIP) in->rip = 1;
#endif
  }
  if (imm_len) {
    int32_t v = 0;
    for (int i = 0; i < imm_len && ip < n; ++i) v |= (int32_t)s[ip++] << (8 * i);
    in->imm = v;
    in->op[in->n_ops].type = T_IMM; in->n_ops++;
  }
  in->mnem = MNEM_VEX;                             // VEX placeholder (opcode in vex_op)
  return ip;
}

// decode one full instruction: prefixes -> opcode -> ModR/M | immediate.
// returns the byte length consumed; sets insn.mnem = 0xFFFF on an unknown opcode.
static inline size_t decode_insn(const byte* s, size_t n, x86dec_t* d) {
  size_t ip = parse_prefixes(s, n, d);             // vars + prefix bookkeeping
  d->insn.vex = 0;
  d->insn.rex2 = 0;
  d->insn.moffs = 0;
#if ARCH_MODE == 64
  // 64-bit: C4/C5 (VEX) and 62 (EVEX) are decoded by the FSM -- op1[C4/C5/62]
  // route into the capture-based vexp*/evexp* stages -> vex_finalize -- so they
  // fall through to the opcode FSM below. An opcode the corpus doesn't cover (or
  // a truncated prefix) falls back to the C++ structural path from there. Only
  // XOP (8F) still starts on the structural path (the 8F/POP ambiguity).
#else
  // 32-bit: C4/C5/62 are VEX/EVEX only when the next byte has mod==11 (LES/LDS/
  // BOUND require a memory operand, so mod==11 is free for the VEX disambiguation).
  if (ip < n && (s[ip] == 0xC4 || s[ip] == 0xC5 || s[ip] == 0x62) &&
      ip + 1 < n && (s[ip + 1] & 0xC0) == 0xC0)
    return vex_decode(d, s, n, ip);
#endif
  // XOP (8F): map field (byte1[4:0]) >= 8 distinguishes it from legacy POP (/0).
  if (ip < n && s[ip] == 0x8F && ip + 1 < n && (s[ip + 1] & 0x1f) >= 8) {
#if ARCH_MODE == 64
    // The C++ does the POP/XOP split (the FSM can't, 8F being a legacy group), then
    // the capture-based xop tables decode it -- same shape as the VEX FSM path.
    size_t xop_at = ip;
    ip = run_fsm(FSM_XOPP1, s, n, ip + 1, d->cap);   // consume byte1, byte2, opcode
    int form = (int)d->cap[CAP_FORM];
    if (form < FORM_VEX_NONE) return vex_decode(d, s, n, xop_at);  // uncovered -> structural
    if (form != FORM_VEX_NONE) {
      uint32_t mstart = (uint32_t)(FSM_MODRM + d->cap[VAR_ADRSIZ] * 256);
      size_t before = ip;
      ip = run_fsm(mstart, s, n, ip, d->cap);
      if (ip == before) { d->insn.mnem = 0xFFFF; return ip; }
      fill_insn(d);
    }
    d->insn.addr = (uint16_t)d->cap[VAR_ADRSIZ];
    ip = append_imm(d->cap, s, n, ip, (int)d->cap[VAR_OPSIZ]);
    d->insn.mnem = (uint16_t)d->cap[CAP_MNEM];
    vex_finalize(d, s, xop_at);
    return ip;
#else
    return vex_decode(d, s, n, ip);                  // 32-bit: structural placeholder
#endif
  }
  // mov to/from cr/dr (0F 20-23): the ModR/M mod field is ignored by hardware
  // (always register-direct, no SIB/disp), so capture the raw ModR/M byte and
  // replay it -- this round-trips the non-canonical mod != 11 encodings too.
  if (ip + 2 < n && s[ip] == 0x0F && s[ip + 1] >= 0x20 && s[ip + 1] <= 0x23) {
    x86insn_t* in = &d->insn;
    in->vex = 5; in->vex_op = s[ip + 1]; in->vex1 = s[ip + 2];
    in->vex2 = in->vex3 = 0;
    int reg = (in->vex1 >> 3) & 7, rm = in->vex1 & 7;
    int crdr = (in->vex_op & 1) ? T_DREG : T_CREG;       // 20/22 = cr, 21/23 = dr
    in->n_ops = 2; in->mnem = MNEM_MOV; in->opsize = 0;
    if (in->vex_op >= 0x22) {                             // 22/23: mov cr/dr, r32
      in->op[0].type = crdr;   in->op[0].index = reg;
      in->op[1].type = T_GPR;  in->op[1].index = rm;
    } else {                                              // 20/21: mov r32, cr/dr
      in->op[0].type = T_GPR;  in->op[0].index = rm;
      in->op[1].type = crdr;   in->op[1].index = reg;
    }
    return ip + 3;
  }
#if ARCH_MODE == 64
  int rex2_0f = 0;
  // APX REX2 (D5 + payload): a 2-byte prefix carrying R4/X4/B4 (the 4th register
  // bit -> r16-r31) and W.R.X.B, plus a map bit M0 (payload bit 7: 0 = 1-byte
  // opcode map, 1 = 0F map with NO 0F escape byte). Capture it raw in pfx[] and
  // replay it; W drives operand size like REX.W; the high register bits ride in
  // the replayed payload, so the modrm low 3 bits round-trip unchanged.
  if (ip + 1 < n && s[ip] == 0xD5 && d->insn.n_pfx + 2 <= (int)sizeof(d->insn.pfx)) {
    byte payload = s[ip + 1];
    d->insn.rex2 = 1;
    rex2_0f = (payload >> 7) & 1;                       // M0
    if (payload & 8) d->cap[VAR_OPSIZ] = 2;             // REX2.W -> 64-bit operand
    d->insn.pfx[d->insn.n_pfx++] = 0xD5;
    d->insn.pfx[d->insn.n_pfx++] = payload;
    d->insn.has_pfx = 1;
    ip += 2;
  }
#endif
  // mov accumulator <-> [moffs] (A0-A3), both modes: no ModR/M -- the operand is an
  // absolute address that follows the opcode directly, sized by the ADDRESS size
  // (64-bit: 8 bytes, or 4 under a 67; 32-bit: 4 bytes, or 2 under a 67). It is held
  // in the 64-bit imm for re-encode and mirrored into disp for the renderer; the
  // opcode byte rides in vex_op. No compiler emits these, but completeness and the
  // bijection require them, address-size-correct in both modes.
  if (ip < n && s[ip] >= 0xA0 && s[ip] <= 0xA3) {
    x86insn_t* in = &d->insn;
#if ARCH_MODE == 64
    int aw = d->cap[VAR_ADRSIZ] ? 4 : 8;
#else
    int aw = d->cap[VAR_ADRSIZ] ? 2 : 4;
#endif
    if (ip + 1 + (size_t)aw > n) { in->mnem = 0xFFFF; return ip; }   // truncated address
    uint64_t a = 0;
    for (int j = aw - 1; j >= 0; --j) a = (a << 8) | s[ip + 1 + j];
    in->moffs = 1; in->vex_op = s[ip]; in->imm = (int64_t)a; in->disp = (int32_t)a;
    in->mnem = MNEM_MOV;
    in->opsize = (uint16_t)d->cap[VAR_OPSIZ];
    in->addr   = (uint16_t)d->cap[VAR_ADRSIZ];
    int store = (s[ip] >= 0xA2);                          // A2/A3: [moffs] <- accumulator
    int rs = store ? 1 : 0, ms = store ? 0 : 1;
    in->op[rs].type = T_GPR;  in->op[rs].index = 0;       // al / eAX / rAX
    in->op[ms].type = T_MEM;  in->op[ms].index = 0;       // [abs]
    in->mem_base = GREG_NONE; in->mem_index = GREG_NONE; in->mem_seg = 0; in->n_ops = 2;
    return ip + 1 + aw;
  }
  size_t op_at = ip;
  int tb = 0;                                       // escape: 0 none, 1 0F, 2 0F 38, 3 0F 3A
#if ARCH_MODE == 64
  if (d->insn.rex2 && rex2_0f) {                    // REX2 M0=1: opcode is a 0F-map byte
    tb = 1;
    ip = run_fsm(FSM_OP2, s, n, ip, d->cap);
    if (ip == op_at) { d->insn.mnem = 0xFFFF; return ip; }
    // REX2 reaches only the 1-byte and 0F maps -- a further escape (0F 38/3A, or
    // 0F 0F) is #UD under REX2 (the encoder would drop the implied 0F and lose
    // the escape byte). And REX2 encodes only legacy GPR instructions; legacy
    // SSE/vector ops in the 0F map are #UD too. Reject both.
    int rf = (int)d->cap[CAP_RFILE], mf = (int)d->cap[CAP_MFILE];
    if (d->cap[CAP_FORM] == FORM_ESC || d->cap[CAP_TBL3] ||
        rf == OPF_XMM || rf == OPF_MM || rf == OPF_SSE_OS || rf == OPF_MMG ||
        mf == OPF_XMM || mf == OPF_MM || mf == OPF_SSE_OS || mf == OPF_MMG) {
      d->insn.mnem = 0xFFFF; return ip;
    }
  } else
#endif
  {
  ip = run_fsm(FSM_OP1, s, n, ip, d->cap);          // opcode: mnemonic, form, embedded reg
  if (ip == op_at) { d->insn.mnem = 0xFFFF; return ip; }   // undefined / unsupported opcode
  // REX2 with M0=0 selects the 1-byte map; an explicit 0F escape byte is then #UD
  // (a 0F-map op must use M0=1). Reject so the encoder never has to drop a 0F.
  if (d->insn.rex2 && d->cap[CAP_FORM] == FORM_ESC) { d->insn.mnem = 0xFFFF; return ip; }
  if (d->cap[CAP_FORM] == FORM_ESC) {               // 0F escape: the real opcode is byte 2
    tb = 1;
    size_t o2 = ip;
    ip = run_fsm(FSM_OP2, s, n, ip, d->cap);
    if (ip == o2) { d->insn.mnem = 0xFFFF; return ip; }     // undefined 0F opcode
    if (d->cap[CAP_FORM] == FORM_ESC) d->cap[CAP_FORM] = FORM_NONE;   // op2 set no form
    if (d->cap[CAP_TBL3]) {                          // 0F 38 / 0F 3A: a third opcode byte
      uint32_t b3 = (d->cap[CAP_TBL3] == 1) ? FSM_OP3_38 : FSM_OP3_3A;
      tb = (d->cap[CAP_TBL3] == 1) ? 2 : 3;
      size_t o3 = ip;
      ip = run_fsm(b3, s, n, ip, d->cap);
      if (ip == o3) { d->insn.mnem = 0xFFFF; return ip; }   // undefined three-byte opcode
      if (d->cap[CAP_FORM] == FORM_ESC) d->cap[CAP_FORM] = FORM_NONE;
    }
  }
  }
  int form = (int)d->cap[CAP_FORM];
#if ARCH_MODE == 64
  if (s[op_at] == 0xC4 || s[op_at] == 0xC5 || s[op_at] == 0x62) {  // VEX/EVEX via the FSM
    // a complete VEX decode reaches a vexop state, which sets CAP_FORM to a VEX_*
    // form. If not -- the opcode is not (yet) in the corpus, or the prefix/opcode
    // was truncated -- fall back to the structural path (MNEM_VEX placeholder),
    // which round-trips any VEX bytes and rejects truncation. So uncovered VEX
    // opcodes still decode losslessly; covered ones get real mnemonics/operands.
    if (form < FORM_VEX_NONE) return vex_decode(d, s, n, op_at);
    if (form != FORM_VEX_NONE) {                     // run the ModR/M + SIB + disp stage
      uint32_t mstart = (uint32_t)(FSM_MODRM + d->cap[VAR_ADRSIZ] * 256);
      size_t before = ip;
      ip = run_fsm(mstart, s, n, ip, d->cap);
      if (ip == before) { d->insn.mnem = 0xFFFF; return ip; }   // truncated ModR/M
      fill_insn(d);
      // opcode-extension group with a /digit the corpus doesn't define: fall back
      // to the structural placeholder so the byte-exact round-trip still holds
      // (e.g. VEX 0F 73 /3 vpsrldq / /7 vpslldq if absent from the group).
      if ((form == FORM_VEX_VMG || form == FORM_APX_MI || form == FORM_APX_M) &&
          vexgrp[d->cap[CAP_GRP]][d->cap[CAP_REG]] < 0)
        return vex_decode(d, s, n, op_at);
    }
    d->insn.addr = (uint16_t)d->cap[VAR_ADRSIZ];
    ip = append_imm(d->cap, s, n, ip, (int)d->cap[VAR_OPSIZ]);   // imm width per CAP_IMK
    d->insn.mnem = (uint16_t)d->cap[CAP_MNEM];
    // map 4 is APX-promoted legacy: lower to GPR operands + r0-31 + NDD/NF; the rest
    // of the EVEX prefixes (maps 1-3) are AVX-512 and lower via vex_finalize.
    if (s[op_at] == 0x62 && (s[op_at + 1] & 7) == 4) apx_finalize(d, s, op_at);
    else                                             vex_finalize(d, s, op_at);
    return ip;
  }
#endif
  // Per-prefix full descriptor (MNSEL mode 3): the mandatory prefix selects not just
  // the mnemonic but the whole form (movd/movq 7E; movq/movq2dq/movdq2q D6). The
  // prefix run is decoded by now, so resolve pp and override mnem/form/dir/files/imk/
  // rmreq from ppdesc before the ModR/M + immediate stage. A 0xFFFF slot is #UD.
  if (d->cap[CAP_MNSEL] == 3) {
    int pp = d->cap[VAR_REPTYPE] ? (int)d->cap[VAR_REPTYPE] + 1
                                 : (d->cap[VAR_OPSIZ] == 1 ? 1 : 0);
    const struct PpDesc* pd = &ppdesc[d->cap[CAP_GRP]][pp];
    if (pd->mnem == 0xFFFF) { d->insn.mnem = 0xFFFF; return ip; }
    d->cap[CAP_MNEM]  = pd->mnem;   d->cap[CAP_FORM] = pd->form;   form = pd->form;
    d->cap[CAP_DIR]   = pd->dir;    d->cap[CAP_RFILE] = pd->rfile;
    d->cap[CAP_MFILE] = pd->mfile;  d->cap[CAP_IMK] = pd->imk;
    d->cap[CAP_RMREQ] = pd->rmreq;   // MNSEL stays 3: the reg-form mnemonic (mreg) is
  }                                  // resolved after ModR/M, once reg-vs-mem is known

  if (form == FORM_MODRM || form == FORM_RM) {
    uint32_t mstart = (uint32_t)(FSM_MODRM + d->cap[VAR_ADRSIZ] * 256);
    ip = run_fsm(mstart, s, n, ip, d->cap);
    int req = (int)d->cap[CAP_RMREQ];                 // 0 any, 1 reg-only, 2 mem-only
    int is_reg = (d->cap[CAP_MODE] == RM_REG);
    if ((req == 1 && !is_reg) || (req == 2 && is_reg)) {   // illegal mod for this opcode
      d->insn.mnem = 0xFFFF; return ip;
    }
    // ppdesc reg-form structural override: MPX 0F 1A/1B NP have mem=bndldx/bndstx
    // (bnd,mem) but reg=nop (r/m) -- a different form/file, resolved now that the
    // ModR/M mod is known (the mnemonic swap rides the MNSEL==3 mreg pass below).
    if (d->cap[CAP_MNSEL] == 3 && is_reg) {
      int pp = d->cap[VAR_REPTYPE] ? (int)d->cap[VAR_REPTYPE] + 1
                                   : (d->cap[VAR_OPSIZ] == 1 ? 1 : 0);
      const struct PpDesc* pd = &ppdesc[d->cap[CAP_GRP]][pp];
      if (pd->rform != 0xFF) { d->cap[CAP_FORM] = pd->rform; d->cap[CAP_MFILE] = pd->rmf; }
      d->cap[CAP_DIR] = pd->dir_reg;   // reg-form ModR/M order (urdmsr r/m,reg vs enqcmd reg,mem)
    }
    fill_insn(d);
    ip = append_imm(d->cap, s, n, ip, (int)d->cap[VAR_OPSIZ]);
    if (d->cap[CAP_MNSEL] == 4) {                    // 3DNow!: a trailing opcode byte after
      if (ip >= n) { d->insn.mnem = 0xFFFF; return ip; }   // ModR/M selects the mnemonic
      d->cap[CAP_MNEM] = tdnow_tab[s[ip++]];         // (pfadd/pfmul/...); 0xFFFF -> #UD
    }
  } else if (form == FORM_GROUP) {
    int gid = (int)d->cap[CAP_GRP];
    if (ppgroup[gid]) {                                   // pp-variant group (0F AE / 0F 1E):
      int pp = d->cap[VAR_REPTYPE] ? (int)d->cap[VAR_REPTYPE] + 1  // pick the mandatory-prefix
                                   : (d->cap[VAR_OPSIZ] == 1 ? 1 : 0);  // slot before ModR/M
      gid += pp;                                          // (base gid + pp; 4 consecutive gids)
    }
    uint32_t gstart = (uint32_t)(FSM_GROUPS + (gid * 2 + (int)d->cap[VAR_ADRSIZ]) * 256);
    size_t before = ip;
    ip = run_fsm(gstart, s, n, ip, d->cap);               // reg field -> mnemonic + r/m operand
    if (ip == before) { d->insn.mnem = 0xFFFF; return ip; }   // undefined /digit extension
    fill_insn(d);
    ip = append_imm(d->cap, s, n, ip, (int)d->cap[VAR_OPSIZ]);
  } else {
    ip = append_imm(d->cap, s, n, ip, (int)d->cap[VAR_OPSIZ]);
  }
  if (d->cap[CAP_MNSEL] == 1) d->cap[CAP_MNEM] += d->cap[VAR_OPSIZ];   // movs/cdqw by opsize
  else if (d->cap[CAP_MNSEL] == 2) {                              // legacy SSE: mnemonic by mandatory prefix
    int pp = d->cap[VAR_REPTYPE] ? (int)d->cap[VAR_REPTYPE] + 1   // F3 -> 2, F2 -> 3
                                 : (d->cap[VAR_OPSIZ] == 1 ? 1 : 0);  // 66 -> 1, else NP -> 0
    d->cap[CAP_MNEM] = ppvtab[d->cap[CAP_GRP]][pp];              // CAP_GRP carries the vtab index
  }
  else if (d->cap[CAP_MNSEL] == 3) {                              // ppdesc: reg-form mnemonic
    int pp = d->cap[VAR_REPTYPE] ? (int)d->cap[VAR_REPTYPE] + 1   // movhlps (reg) vs movlps (mem),
                                 : (d->cap[VAR_OPSIZ] == 1 ? 1 : 0);  // movlhps vs movhps, ...
    uint16_t mr = ppdesc[d->cap[CAP_GRP]][pp].mreg;              // 0xFFFF -> use CAP_MNEM for both
    if (mr != 0xFFFF && d->cap[CAP_MODE] == RM_REG) d->cap[CAP_MNEM] = mr;
  }
  d->insn.mnem   = (uint16_t)d->cap[CAP_MNEM];
  d->insn.opsize = (uint16_t)d->cap[VAR_OPSIZ];
  d->insn.addr   = (uint16_t)d->cap[VAR_ADRSIZ];
  if (d->insn.mnem != 0xFFFF) finalize_insn(d, s, n, op_at, tb);   // faithful x86insn_t + enc
  return ip;
}

#endif // X86DEC_HPP
