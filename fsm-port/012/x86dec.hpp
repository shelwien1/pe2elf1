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
static inline size_t run_fsm(uint16_t base, const byte* s, size_t len,
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
static inline int file_to_T(int opf, int opsize) {
  switch (opf) {
    case OPF_RGB:    return T_GPR8;
    case OPF_XMM:    return T_XMM;
    case OPF_MM:     return T_MMX;
    case OPF_SREG:   return T_SREG;
    case OPF_SSE_OS: return opsize ? T_XMM : T_MMX;
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
static inline void finalize_insn(x86dec_t* d, const byte* s, size_t op_at, int tb) {
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
  // REX is the last prefix byte (it must immediately precede the opcode), captured
  // raw in pfx[] by the prefix run. Extract W/R/X/B here rather than spending five
  // scarce capture slots on it. REX.R extends a ModR/M.reg operand, REX.B an
  // embedded (40+r/B8+r) reg or rm/base, REX.X the SIB index.
  // For a REX2 (D5) instruction the last prefix byte is the REX2 payload, not a
  // 0x4x REX -- its bits are replayed raw, so skip the REX extraction (operand
  // size already came from VAR_OPSIZ via REX2.W; register extension is not needed
  // for the byte-exact round-trip, the high bits ride in the replayed payload).
  int rexbyte = (!in->rex2 && in->n_pfx && (in->pfx[in->n_pfx - 1] & 0xF0) == 0x40)
                  ? in->pfx[in->n_pfx - 1] : 0;
  int xr = (rexbyte >> 2) & 1, xx = (rexbyte >> 1) & 1;
  xb = rexbyte & 1;
  in->rex = rexbyte ? 1 : 0;
  if (rexbyte & 8) { in->opsize = 2; os = 2; }   // REX.W -> 64-bit operand
  // mov r64,imm64 (movabs): B8+r at 64-bit operand size carries a full 64-bit
  // immediate. Key on os (== VAR_OPSIZ == 2), the same source append_imm/enc_imm
  // size the immediate from, so they agree even on pathological multi-REX input
  // where the last REX byte's W differs from the effective $opsiz.
  imm64_w = (c[CAP_IMK] == IMK_IMMV && os == 2) ? 1 : 0;
  // embedded reg (40+r/B8+r) extends via REX.B; ModR/M.reg via REX.R; the
  // implicit accumulator (FORM_ACC) is not encoded, so it is never extended.
  reg_rex = (form == FORM_REG || form == FORM_REG_IMM) ? xb
          : (form == FORM_ACC) ? 0 : xr;
#endif

  for (int i = 0; i < 5; ++i) { in->op[i].type = T_NONE; in->op[i].index = 0; }
  in->cc = c[CAP_CC] ? (uint8_t)(c[CAP_CC] - 1) : 0xFF;

  int n = 0;
  #define SETREG(slot, file, idx) do { in->op[slot].type = file_to_T(file, os); \
                                       in->op[slot].index = ((idx) & 7) + 8 * reg_rex; } while (0)
  #define SETRM(slot) do { if (mode == RM_REG) { in->op[slot].type = file_to_T(mf, os); \
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
      }
      break;
    case FORM_RM:      SETRM(0); n = 1;
      if (c[CAP_IMK] != IMK_NONE) { in->op[1].type = T_IMM; in->imm = (int32_t)c[CAP_IMM]; n = 2; }
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
  enc_stamp(in, tb, opbyte, digit);
}

static inline size_t parse_addr(const byte* s, size_t len, size_t ip, x86dec_t* d) {
  uint16_t start = (uint16_t)(FSM_MODRM + d->cap[VAR_ADRSIZ] * 256);
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
// One VEX operand from a VEX file code (0=vec by L, 1=gpr32, 2=gpr64, 3=kreg).
static inline x86op_t vex_mkop(x86insn_t* in, int fc, int idx, int vt) {
  x86op_t o; o.index = 0; o.type = T_NONE;
  switch (fc) {
    case 1: o.type = T_GPR;  o.index = idx; break;              // 32-bit GPR
    case 2: o.type = T_GPR;  o.index = idx; in->opsize = 2; break;  // 64-bit GPR (W=1)
    case 3: o.type = T_KREG; o.index = idx & 7; break;          // mask register
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
  in->vex = (p0 == 0xC5) ? 1 : (p0 == 0xC4) ? 2 : 3;     // C5 / C4 / 62 (EVEX)
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
  } else {                                               // VEX C4/C5
    if (in->vex == 1) { in->vex1 = s[q]; in->vex2 = in->vex3 = 0; q += 1; }
    else              { in->vex1 = s[q]; in->vex2 = s[q + 1]; in->vex3 = 0; q += 2; }
    int rxb = (int)c[CAP_RXB];
    if (in->vex == 2) { R = 1 - ((rxb >> 2) & 1); X = 1 - ((rxb >> 1) & 1); B = 1 - (rxb & 1); }
    else              { R = 1 - (rxb & 1); X = 0; B = 0; }
    LL = (int)c[CAP_VL]; vvvv = (~(int)c[CAP_VVVV]) & 0xf;
  }
  size_t opc_at = q; in->vex_op = s[q++];
  bool has_modrm = (form != FORM_VEX_NONE);
  in->vex_modrm = has_modrm ? s[q] : 0;
  bool is_reg = (c[CAP_MODE] == RM_REG);
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

  x86op_t regop  = vex_mkop(in, rf, mreg + 8 * R + 16 * Rp, vt);
  x86op_t vvvvop = vex_mkop(in, rf, vvvv, vt);
  x86op_t rmreg  = vex_mkop(in, mf, mrm + 8 * B + 16 * X, vt);
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
    case FORM_VEX_VMI:  in->op[nn++]=vvvvop; in->op[nn++]=rmop; in->op[nn++]=immop; break;
    case FORM_VEX_VMG:  // shift-by-imm group: dest=vvvv, src=r/m, imm8; mnem by /digit
      in->op[nn++]=vvvvop; in->op[nn++]=rmop; in->op[nn++]=immop;
      in->mnem = (uint16_t)vexgrp[(int)c[CAP_GRP]][mreg];     // mreg == ModR/M reg = /digit
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
  vex_structure(map, op, &has_modrm, &imm_len);
  in->n_ops = 0;
  in->opsize = 0;
  in->vex_modrm = 0;
  in->addr = (uint16_t)d->cap[VAR_ADRSIZ];         // needed by capture_addr_witness
  if (has_modrm) {
    if (ip >= n) { in->mnem = 0xFFFF; return ip; }
    in->vex_modrm = s[ip];                         // raw ModR/M (replayed by the encoder)
    uint16_t mstart = (uint16_t)(FSM_MODRM + d->cap[VAR_ADRSIZ] * 256);
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
  if (ip < n && s[ip] == 0x8F && ip + 1 < n && (s[ip + 1] & 0x1f) >= 8)
    return vex_decode(d, s, n, ip);
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
        rf == OPF_XMM || rf == OPF_MM || rf == OPF_SSE_OS ||
        mf == OPF_XMM || mf == OPF_MM || mf == OPF_SSE_OS) {
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
      uint16_t b3 = (d->cap[CAP_TBL3] == 1) ? FSM_OP3_38 : FSM_OP3_3A;
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
      uint16_t mstart = (uint16_t)(FSM_MODRM + d->cap[VAR_ADRSIZ] * 256);
      size_t before = ip;
      ip = run_fsm(mstart, s, n, ip, d->cap);
      if (ip == before) { d->insn.mnem = 0xFFFF; return ip; }   // truncated ModR/M
      fill_insn(d);
      // opcode-extension group with a /digit the corpus doesn't define: fall back
      // to the structural placeholder so the byte-exact round-trip still holds
      // (e.g. VEX 0F 73 /3 vpsrldq / /7 vpslldq if absent from the group).
      if (form == FORM_VEX_VMG && vexgrp[d->cap[CAP_GRP]][d->cap[CAP_REG]] < 0)
        return vex_decode(d, s, n, op_at);
    }
    d->insn.addr = (uint16_t)d->cap[VAR_ADRSIZ];
    ip = append_imm(d->cap, s, n, ip, (int)d->cap[VAR_OPSIZ]);   // imm8 if CAP_IMK set
    d->insn.mnem = (uint16_t)d->cap[CAP_MNEM];
    vex_finalize(d, s, op_at);                       // build operands from the captures
    return ip;
  }
#endif
  if (form == FORM_MODRM || form == FORM_RM) {
    uint16_t mstart = (uint16_t)(FSM_MODRM + d->cap[VAR_ADRSIZ] * 256);
    ip = run_fsm(mstart, s, n, ip, d->cap);
    int req = (int)d->cap[CAP_RMREQ];                 // 0 any, 1 reg-only, 2 mem-only
    int is_reg = (d->cap[CAP_MODE] == RM_REG);
    if ((req == 1 && !is_reg) || (req == 2 && is_reg)) {   // illegal mod for this opcode
      d->insn.mnem = 0xFFFF; return ip;
    }
    fill_insn(d);
    ip = append_imm(d->cap, s, n, ip, (int)d->cap[VAR_OPSIZ]);
  } else if (form == FORM_GROUP) {
    int gid = (int)d->cap[CAP_GRP];
    uint16_t gstart = (uint16_t)(FSM_GROUPS + (gid * 2 + (int)d->cap[VAR_ADRSIZ]) * 256);
    size_t before = ip;
    ip = run_fsm(gstart, s, n, ip, d->cap);               // reg field -> mnemonic + r/m operand
    if (ip == before) { d->insn.mnem = 0xFFFF; return ip; }   // undefined /digit extension
    fill_insn(d);
    ip = append_imm(d->cap, s, n, ip, (int)d->cap[VAR_OPSIZ]);
  } else {
    ip = append_imm(d->cap, s, n, ip, (int)d->cap[VAR_OPSIZ]);
  }
  if (d->cap[CAP_MNSEL]) d->cap[CAP_MNEM] += d->cap[VAR_OPSIZ];   // movs/cdqw by opsize
  d->insn.mnem   = (uint16_t)d->cap[CAP_MNEM];
  d->insn.opsize = (uint16_t)d->cap[VAR_OPSIZ];
  d->insn.addr   = (uint16_t)d->cap[VAR_ADRSIZ];
  if (d->insn.mnem != 0xFFFF) finalize_insn(d, s, op_at, tb);   // faithful x86insn_t + enc
  return ip;
}

#endif // X86DEC_HPP
