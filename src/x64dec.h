// x64dec.h - x86-64 instruction layout decoder.
//
// Splits an instruction into "structural" bytes (prefixes, REX/REX2,
// VEX/EVEX payload, opcode, ModRM, SIB) and operand fields (displacement,
// immediates).  The layout is computed from the structural bytes only, so an
// inverse transform that stores operands elsewhere can re-parse the
// structural stream and get exactly the same answer.  Any byte sequence is
// accepted: invalid opcodes simply decode as one-byte instructions.
#pragma once
#include <stdint.h>
#include <stddef.h>

namespace x64 {

// immediate operand kinds
enum ImmKind : uint8_t {
  I_NONE = 0,
  I_B,      // imm8
  I_W,      // imm16
  I_Z,      // imm16/32 (66 prefix without REX.W -> 16)
  I_V,      // imm16/32/64 (mov r,imm)
  I_MOFFS,  // moffs64 (moffs32 with 67)
  I_ENTER,  // imm16 + imm8
  I_J8,     // rel8 (jcc short, jmp short, loop, jrcxz)
  I_CALL,   // rel32 call
  I_JMP,    // rel32 jmp
  I_JCC,    // rel32 jcc (0F 80..8F)
  I_G3,     // F6/F7: imm only for /0 and /1
  I_BB,     // imm8 + imm8 (EXTRQ/INSERTQ)
};

// operand field classes (what the transform does with a field)
enum FieldClass : uint8_t {
  F_NONE = 0,
  F_D8,      // disp8
  F_D32,     // disp32 with base register
  F_DABS,    // disp32 without base (SIB base=101, mod=00): absolute address
  F_RIP,     // RIP-relative disp32
  F_I8,      // imm8
  F_I16,     // imm16
  F_I32,     // imm32
  F_I64,     // imm64 / moffs64
  F_J8,      // rel8
  F_CALL,    // call rel32
  F_JMP,     // jmp rel32
  F_JCC,     // jcc rel32
  F_COUNT
};

struct Insn {
  uint8_t nstruct;   // number of structural bytes
  uint8_t len;       // total length (nstruct + operand bytes)
  uint8_t nfield;    // number of operand fields (0..3)
  uint8_t trunc;     // instruction does not fit into the available bytes
  uint8_t fsize[3];  // field sizes in order
  uint8_t fclass[3]; // field classes in order
  uint16_t op;       // map<<8 | opcode byte
  uint8_t modrm;     // valid if hasmodrm
  uint8_t hasmodrm;
};

// map0 table: low nibble = ImmKind, 0x10 = ModRM, 0x20 = prefix, 0x40 = special
enum { T_M = 0x10, T_P = 0x20, T_S = 0x40 };

static const uint8_t map0[256] = {
// 0        1        2        3        4        5        6        7        8        9        A        B        C        D        E        F
  T_M,     T_M,     T_M,     T_M,     I_B,     I_Z,     0,       0,       T_M,     T_M,     T_M,     T_M,     I_B,     I_Z,     0,       T_S,     // 0
  T_M,     T_M,     T_M,     T_M,     I_B,     I_Z,     0,       0,       T_M,     T_M,     T_M,     T_M,     I_B,     I_Z,     0,       0,       // 1
  T_M,     T_M,     T_M,     T_M,     I_B,     I_Z,     T_P,     0,       T_M,     T_M,     T_M,     T_M,     I_B,     I_Z,     T_P,     0,       // 2
  T_M,     T_M,     T_M,     T_M,     I_B,     I_Z,     T_P,     0,       T_M,     T_M,     T_M,     T_M,     I_B,     I_Z,     T_P,     0,       // 3
  T_P,     T_P,     T_P,     T_P,     T_P,     T_P,     T_P,     T_P,     T_P,     T_P,     T_P,     T_P,     T_P,     T_P,     T_P,     T_P,     // 4
  0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       // 5
  0,       0,       T_S,     T_M,     T_P,     T_P,     T_P,     T_P,     I_Z,     T_M|I_Z, I_B,     T_M|I_B, 0,       0,       0,       0,       // 6
  I_J8,    I_J8,    I_J8,    I_J8,    I_J8,    I_J8,    I_J8,    I_J8,    I_J8,    I_J8,    I_J8,    I_J8,    I_J8,    I_J8,    I_J8,    I_J8,    // 7
  T_M|I_B, T_M|I_Z, T_M|I_B, T_M|I_B, T_M,     T_M,     T_M,     T_M,     T_M,     T_M,     T_M,     T_M,     T_M,     T_M,     T_M,     T_M,     // 8
  0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       0,       // 9
  I_MOFFS, I_MOFFS, I_MOFFS, I_MOFFS, 0,       0,       0,       0,       I_B,     I_Z,     0,       0,       0,       0,       0,       0,       // A
  I_B,     I_B,     I_B,     I_B,     I_B,     I_B,     I_B,     I_B,     I_V,     I_V,     I_V,     I_V,     I_V,     I_V,     I_V,     I_V,     // B
  T_M|I_B, T_M|I_B, I_W,     0,       T_S,     T_S,     T_M|I_B, T_M|I_Z, I_ENTER, 0,       I_W,     0,       0,       I_B,     0,       0,       // C
  T_M,     T_M,     T_M,     T_M,     0,       T_S,     0,       0,       T_M,     T_M,     T_M,     T_M,     T_M,     T_M,     T_M,     T_M,     // D
  I_J8,    I_J8,    I_J8,    I_J8,    I_B,     I_B,     I_B,     I_B,     I_CALL,  I_JMP,   0,       I_J8,    0,       0,       0,       0,       // E
  T_P,     0,       T_P,     T_P,     0,       0,       T_M|I_G3,T_M|I_G3,0,       0,       0,       0,       0,       0,       T_M,     T_M,     // F
};

// 0F map: bit0 = ModRM, bit1 = imm8
static inline uint8_t map1_info(unsigned o) {
  switch (o) {
    case 0x04: case 0x05: case 0x06: case 0x07: case 0x08: case 0x09: case 0x0A:
    case 0x0B: case 0x0C: case 0x0E:
    case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
    case 0x39: case 0x3B: case 0x3C: case 0x3D: case 0x3E: case 0x3F:
    case 0x77:
    case 0xA0: case 0xA1: case 0xA2: case 0xA8: case 0xA9: case 0xAA:
    case 0xC8: case 0xC9: case 0xCA: case 0xCB: case 0xCC: case 0xCD: case 0xCE: case 0xCF:
      return 0;
    case 0x0F:  // 3DNow!: ModRM + imm8 suffix opcode
    case 0x70: case 0x71: case 0x72: case 0x73:
    case 0xA4: case 0xAC: case 0xBA:
    case 0xC2: case 0xC4: case 0xC5: case 0xC6:
      return 3;
    default:
      return (o >= 0x80 && o <= 0x8F) ? 0 : 1;
  }
}

static inline bool vex_map1_imm(unsigned o) {
  return (o >= 0x70 && o <= 0x73) || o == 0xC2 || o == 0xC4 || o == 0xC5 || o == 0xC6;
}

// Decode the instruction at p, of which `avail` bytes exist.
// Only structural bytes are read.  If the structure itself or the operands
// do not fit, trunc is set; then the instruction is `avail` bytes long and
// the caller must treat all of it as structural.
static inline void decode(const uint8_t* p, size_t avail, Insn& I) {
  size_t i = 0;
  unsigned p66 = 0, p67 = 0, rexw = 0, npfx = 0;
  unsigned imm = I_NONE, hasm = 0, map = 0, op = 0, forcereg = 0;
  I.nfield = 0; I.trunc = 0; I.hasmodrm = 0; I.modrm = 0;

  #define NEED(k) if (i + (k) > avail) goto truncated
  for (;;) {
    NEED(1);
    unsigned b = p[i];
    if (npfx < 14 && (map0[b] & T_P)) {
      if ((b & 0xF0) == 0x40) rexw = b & 8;
      else {
        rexw = 0;  // REX must be the last prefix
        if (b == 0x66) p66 = 1;
        if (b == 0x67) p67 = 1;
      }
      i++; npfx++;
      continue;
    }
    break;
  }
  op = p[i++];
  {
    unsigned t = map0[op];
    if (t & T_P) {
      // too many prefixes: the excess prefix byte is a 1-byte instruction
      imm = I_NONE; hasm = 0;
    } else if (t & T_S) {
      if (op == 0x0F) {
        NEED(1);
        op = p[i++];
        if (op == 0x38 || op == 0x3A) {
          map = (op == 0x38) ? 2 : 3;
          NEED(1);
          op = p[i++];
          hasm = 1; imm = (map == 3) ? I_B : I_NONE;
        } else {
          map = 1;
          uint8_t info = map1_info(op);
          hasm = info & 1; imm = (info & 2) ? I_B : I_NONE;
          if (op >= 0x80 && op <= 0x8F) imm = I_JCC;
          if (op >= 0x20 && op <= 0x23) forcereg = 1;  // mov cr/dr: mod is ignored
        }
      } else if (op == 0xC4 || op == 0xC5) {
        // VEX
        unsigned mm;
        if (op == 0xC4) { NEED(2); mm = p[i] & 31; i += 2; }
        else            { NEED(1); mm = 1; i += 1; }
        NEED(1);
        unsigned o = p[i++];
        map = mm; op = o;
        if (mm == 1) { hasm = (o != 0x77); imm = vex_map1_imm(o) ? I_B : I_NONE; }
        else if (mm == 3) { hasm = 1; imm = I_B; }
        else { hasm = 1; imm = I_NONE; }
      } else if (op == 0x62) {
        // EVEX
        NEED(3);
        unsigned mm = p[i] & 7;
        i += 3;
        NEED(1);
        unsigned o = p[i++];
        map = mm; op = o;
        hasm = 1;
        if (mm == 1) imm = vex_map1_imm(o) ? I_B : I_NONE;
        else if (mm == 3) imm = I_B;
        else if (mm == 4) {
          // APX promoted legacy instructions: follow map0 immediates
          unsigned t4 = map0[o];
          imm = (t4 & (T_P | T_S)) ? (unsigned)I_NONE : (t4 & 15u);
          if (imm != I_B && imm != I_Z && imm != I_G3) imm = I_NONE;
        } else imm = I_NONE;
      } else {
        // 0xD5: REX2 (APX)
        NEED(2);
        unsigned pl = p[i];
        i += 1;
        rexw = pl & 8;
        unsigned o = p[i++];
        if (pl & 0x80) {
          map = 1; op = o;
          uint8_t info = map1_info(o);
          hasm = info & 1; imm = (info & 2) ? I_B : I_NONE;
          if (o >= 0x80 && o <= 0x8F) imm = I_JCC;
        } else {
          op = o;
          unsigned t2 = map0[o];
          if (t2 & (T_P | T_S)) { hasm = 0; imm = I_NONE; }
          else { hasm = (t2 & T_M) ? 1 : 0; imm = t2 & 15; }
        }
      }
    } else {
      hasm = (t & T_M) ? 1 : 0;
      imm = t & 15;
    }
  }
  if (map == 1 && op == 0x78 && hasm) {
    // EXTRQ (66 0F 78 /0 ib ib), INSERTQ (F2 0F 78 /r ib ib)
    // prefix check: last legacy prefix scan
    for (size_t k = 0; k < (size_t)npfx; k++)
      if (p[k] == 0x66 || p[k] == 0xF2) { imm = I_BB; break; }
  }

  {
    unsigned dsz = 0, dcls = F_NONE;
    if (hasm) {
      NEED(1);
      unsigned m = p[i++];
      I.modrm = (uint8_t)m; I.hasmodrm = 1;
      unsigned mod = m >> 6, rm = m & 7;
      if (forcereg) mod = 3;
      if (mod != 3) {
        if (rm == 4) {
          NEED(1);
          unsigned sib = p[i++];
          if (mod == 0 && (sib & 7) == 5) { dsz = 4; dcls = F_DABS; }
        } else if (mod == 0 && rm == 5) {
          dsz = 4; dcls = F_RIP;
        }
        if (mod == 1) { dsz = 1; dcls = F_D8; }
        else if (mod == 2) { dsz = 4; dcls = F_D32; }
      }
      if (imm == I_G3) {
        unsigned reg = (m >> 3) & 7;
        imm = (reg < 2) ? ((op == 0xF6) ? I_B : I_Z) : I_NONE;
      }
    } else if (imm == I_G3) {
      imm = I_NONE;
    }
    I.nstruct = (uint8_t)i;
    I.op = (uint16_t)(map << 8 | op);
    if (dsz) { I.fsize[0] = (uint8_t)dsz; I.fclass[0] = (uint8_t)dcls; I.nfield = 1; }
  }
  {
    unsigned n = I.nfield;
    switch (imm) {
      case I_NONE: break;
      case I_B:  I.fsize[n] = 1; I.fclass[n++] = F_I8; break;
      case I_W:  I.fsize[n] = 2; I.fclass[n++] = F_I16; break;
      case I_Z:
        if (p66 && !rexw) { I.fsize[n] = 2; I.fclass[n++] = F_I16; }
        else { I.fsize[n] = 4; I.fclass[n++] = F_I32; }
        break;
      case I_V:
        if (rexw) { I.fsize[n] = 8; I.fclass[n++] = F_I64; }
        else if (p66) { I.fsize[n] = 2; I.fclass[n++] = F_I16; }
        else { I.fsize[n] = 4; I.fclass[n++] = F_I32; }
        break;
      case I_MOFFS:
        if (p67) { I.fsize[n] = 4; I.fclass[n++] = F_I32; }
        else { I.fsize[n] = 8; I.fclass[n++] = F_I64; }
        break;
      case I_ENTER:
        I.fsize[n] = 2; I.fclass[n++] = F_I16;
        I.fsize[n] = 1; I.fclass[n++] = F_I8;
        break;
      case I_BB:
        I.fsize[n] = 1; I.fclass[n++] = F_I8;
        I.fsize[n] = 1; I.fclass[n++] = F_I8;
        break;
      case I_J8:   I.fsize[n] = 1; I.fclass[n++] = F_J8; break;
      case I_CALL: I.fsize[n] = 4; I.fclass[n++] = F_CALL; break;
      case I_JMP:  I.fsize[n] = 4; I.fclass[n++] = F_JMP; break;
      case I_JCC:  I.fsize[n] = 4; I.fclass[n++] = F_JCC; break;
      default: break;
    }
    I.nfield = (uint8_t)n;
    unsigned len = i;
    for (unsigned k = 0; k < n; k++) len += I.fsize[k];
    if (len > avail) goto truncated;
    I.len = (uint8_t)len;
  }
  return;
truncated:
  #undef NEED
  I.trunc = 1;
  I.nfield = 0;
  I.nstruct = (uint8_t)(avail < 255 ? avail : 255);
  I.len = I.nstruct;
  I.op = 0;
}

}  // namespace x64
