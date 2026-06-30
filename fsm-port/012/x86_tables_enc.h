// Generated from corpus.p by gasm.py. Do not edit.
// Encode side: x86insn_t -> bytes (inverse of x86_tables.h's decode FSM).
#ifndef X86_TABLES_ENC_H
#define X86_TABLES_ENC_H

#include "x86_tables.h"   // shares FORM_*/IMK_*/OPF_*/GREG_* enum values

enum EmbKind { EMB_NONE = 0, EMB_REG, EMB_CC };
// operand classes for candidate signature matching (derived from x86op_t.type)
enum OpClass { C_NONE = 0, C_GREG, C_RGB, C_XMM, C_MM, C_SREG, C_MEM, C_IMM, C_REL };

// one encoding candidate: opcode byte(s) + embedded-field program + signature.
struct EncCand {
  uint16_t mnem;            // index into mnem_tab (matches decode)
  uint8_t  tb;              // opcode map: 0 one-byte, 1 0F, 2 0F 38, 3 0F 3A
  uint8_t  op;              // base opcode byte (embedded field bits are 0)
  uint8_t  emb;             // EmbKind: embed reg / cc into the opcode byte
  uint8_t  emb_pos, emb_w;  // bit position / width of the embedded field
  uint8_t  digit;           // group /digit (ModR/M reg field), or 0xFF
  uint8_t  form;            // InsnForm
  uint8_t  imk;             // ImmKind of the trailing immediate
  uint8_t  dir;             // ModR/M order: 0 = reg,r/m   1 = r/m,reg
  uint8_t  rfile, mfile;    // OperandFile of the reg / r-m-direct operand
  uint8_t  sfx;             // memory-form size suffix (unused by encoder, kept for parity)
  uint8_t  reg0;            // implicit accumulator (reg operand must be index 0)
  uint8_t  sup_reg, sup_mem;// r/m slot accepts register-direct / memory
};

static const struct EncCand enc_cand[] = {
  {0,0,0x00,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  //   0  add
  {0,0,0x01,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  //   1  add
  {0,0,0x02,EMB_NONE,0,0,255,1,0,0,OPF_RGB,OPF_RGB,0,0,1,1},  //   2  add
  {0,0,0x03,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  //   3  add
  {0,0,0x05,EMB_NONE,0,0,255,10,4,0,OPF_GREG,OPF_GREG,0,1,0,0},  //   4  add
  {0,0,0x80,EMB_NONE,0,0,0,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  //   5  add
  {0,0,0x81,EMB_NONE,0,0,0,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  //   6  add
  {0,0,0x82,EMB_NONE,0,0,0,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  //   7  add
  {0,0,0x83,EMB_NONE,0,0,0,7,8,0,OPF_GREG,OPF_GREG,0,0,1,0},  //   8  add
  {1,0,0x04,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  //   9  add al,
  {2,0,0x08,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  //  10  or
  {2,0,0x09,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  //  11  or
  {2,0,0x0a,EMB_NONE,0,0,255,1,0,0,OPF_RGB,OPF_RGB,0,0,1,1},  //  12  or
  {2,0,0x0b,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  //  13  or
  {2,0,0x0d,EMB_NONE,0,0,255,10,4,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  14  or
  {2,0,0x80,EMB_NONE,0,0,1,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  //  15  or
  {2,0,0x81,EMB_NONE,0,0,1,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  16  or
  {2,0,0x82,EMB_NONE,0,0,1,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  //  17  or
  {2,0,0x83,EMB_NONE,0,0,1,7,8,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  18  or
  {3,0,0x0c,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  19  or al,
  {4,0,0x10,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  //  20  adc
  {4,0,0x11,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  //  21  adc
  {4,0,0x12,EMB_NONE,0,0,255,1,0,0,OPF_RGB,OPF_RGB,0,0,1,1},  //  22  adc
  {4,0,0x13,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  //  23  adc
  {4,0,0x15,EMB_NONE,0,0,255,10,4,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  24  adc
  {4,0,0x80,EMB_NONE,0,0,2,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  //  25  adc
  {4,0,0x81,EMB_NONE,0,0,2,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  26  adc
  {4,0,0x82,EMB_NONE,0,0,2,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  //  27  adc
  {4,0,0x83,EMB_NONE,0,0,2,7,8,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  28  adc
  {5,0,0x14,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  29  adc al,
  {6,0,0x18,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  //  30  sbb
  {6,0,0x19,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  //  31  sbb
  {6,0,0x1a,EMB_NONE,0,0,255,1,0,0,OPF_RGB,OPF_RGB,0,0,1,1},  //  32  sbb
  {6,0,0x1b,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  //  33  sbb
  {6,0,0x1d,EMB_NONE,0,0,255,10,4,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  34  sbb
  {6,0,0x80,EMB_NONE,0,0,3,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  //  35  sbb
  {6,0,0x81,EMB_NONE,0,0,3,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  36  sbb
  {6,0,0x82,EMB_NONE,0,0,3,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  //  37  sbb
  {6,0,0x83,EMB_NONE,0,0,3,7,8,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  38  sbb
  {7,0,0x1c,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  39  sbb al,
  {8,0,0x20,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  //  40  and
  {8,0,0x21,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  //  41  and
  {8,0,0x22,EMB_NONE,0,0,255,1,0,0,OPF_RGB,OPF_RGB,0,0,1,1},  //  42  and
  {8,0,0x23,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  //  43  and
  {8,0,0x25,EMB_NONE,0,0,255,10,4,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  44  and
  {8,0,0x80,EMB_NONE,0,0,4,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  //  45  and
  {8,0,0x81,EMB_NONE,0,0,4,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  46  and
  {8,0,0x82,EMB_NONE,0,0,4,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  //  47  and
  {8,0,0x83,EMB_NONE,0,0,4,7,8,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  48  and
  {9,0,0x24,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  49  and al,
  {10,0,0x28,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  //  50  sub
  {10,0,0x29,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  //  51  sub
  {10,0,0x2a,EMB_NONE,0,0,255,1,0,0,OPF_RGB,OPF_RGB,0,0,1,1},  //  52  sub
  {10,0,0x2b,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  //  53  sub
  {10,0,0x2d,EMB_NONE,0,0,255,10,4,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  54  sub
  {10,0,0x80,EMB_NONE,0,0,5,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  //  55  sub
  {10,0,0x81,EMB_NONE,0,0,5,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  56  sub
  {10,0,0x82,EMB_NONE,0,0,5,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  //  57  sub
  {10,0,0x83,EMB_NONE,0,0,5,7,8,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  58  sub
  {11,0,0x2c,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  59  sub al,
  {12,0,0x30,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  //  60  xor
  {12,0,0x31,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  //  61  xor
  {12,0,0x32,EMB_NONE,0,0,255,1,0,0,OPF_RGB,OPF_RGB,0,0,1,1},  //  62  xor
  {12,0,0x33,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  //  63  xor
  {12,0,0x35,EMB_NONE,0,0,255,10,4,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  64  xor
  {12,0,0x80,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  //  65  xor
  {12,0,0x81,EMB_NONE,0,0,6,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  66  xor
  {12,0,0x82,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  //  67  xor
  {12,0,0x83,EMB_NONE,0,0,6,7,8,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  68  xor
  {13,0,0x34,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  69  xor al,
  {14,0,0x38,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  //  70  cmp
  {14,0,0x39,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  //  71  cmp
  {14,0,0x3a,EMB_NONE,0,0,255,1,0,0,OPF_RGB,OPF_RGB,0,0,1,1},  //  72  cmp
  {14,0,0x3b,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  //  73  cmp
  {14,0,0x3d,EMB_NONE,0,0,255,10,4,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  74  cmp
  {14,0,0x80,EMB_NONE,0,0,7,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  //  75  cmp
  {14,0,0x81,EMB_NONE,0,0,7,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  76  cmp
  {14,0,0x82,EMB_NONE,0,0,7,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  //  77  cmp
  {14,0,0x83,EMB_NONE,0,0,7,7,8,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  78  cmp
  {15,0,0x3c,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  79  cmp al,
  {16,0,0x80,EMB_NONE,0,0,0,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  //  80  add.b
  {16,0,0x82,EMB_NONE,0,0,0,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  //  81  add.b
  {17,0,0x81,EMB_NONE,0,0,0,7,4,0,OPF_GREG,OPF_GREG,0,0,0,1},  //  82  add.d
  {17,0,0x83,EMB_NONE,0,0,0,7,8,0,OPF_GREG,OPF_GREG,0,0,0,1},  //  83  add.d
  {18,0,0x80,EMB_NONE,0,0,1,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  //  84  or.b
  {18,0,0x82,EMB_NONE,0,0,1,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  //  85  or.b
  {19,0,0x81,EMB_NONE,0,0,1,7,4,0,OPF_GREG,OPF_GREG,0,0,0,1},  //  86  or.d
  {19,0,0x83,EMB_NONE,0,0,1,7,8,0,OPF_GREG,OPF_GREG,0,0,0,1},  //  87  or.d
  {20,0,0x80,EMB_NONE,0,0,2,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  //  88  adc.b
  {20,0,0x82,EMB_NONE,0,0,2,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  //  89  adc.b
  {21,0,0x81,EMB_NONE,0,0,2,7,4,0,OPF_GREG,OPF_GREG,0,0,0,1},  //  90  adc.d
  {21,0,0x83,EMB_NONE,0,0,2,7,8,0,OPF_GREG,OPF_GREG,0,0,0,1},  //  91  adc.d
  {22,0,0x80,EMB_NONE,0,0,3,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  //  92  sbb.b
  {22,0,0x82,EMB_NONE,0,0,3,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  //  93  sbb.b
  {23,0,0x81,EMB_NONE,0,0,3,7,4,0,OPF_GREG,OPF_GREG,0,0,0,1},  //  94  sbb.d
  {23,0,0x83,EMB_NONE,0,0,3,7,8,0,OPF_GREG,OPF_GREG,0,0,0,1},  //  95  sbb.d
  {24,0,0x80,EMB_NONE,0,0,4,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  //  96  and.b
  {24,0,0x82,EMB_NONE,0,0,4,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  //  97  and.b
  {25,0,0x81,EMB_NONE,0,0,4,7,4,0,OPF_GREG,OPF_GREG,0,0,0,1},  //  98  and.d
  {25,0,0x83,EMB_NONE,0,0,4,7,8,0,OPF_GREG,OPF_GREG,0,0,0,1},  //  99  and.d
  {26,0,0x80,EMB_NONE,0,0,5,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 100  sub.b
  {26,0,0x82,EMB_NONE,0,0,5,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 101  sub.b
  {27,0,0x81,EMB_NONE,0,0,5,7,4,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 102  sub.d
  {27,0,0x83,EMB_NONE,0,0,5,7,8,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 103  sub.d
  {28,0,0x80,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 104  xor.b
  {28,0,0x82,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 105  xor.b
  {29,0,0x81,EMB_NONE,0,0,6,7,4,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 106  xor.d
  {29,0,0x83,EMB_NONE,0,0,6,7,8,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 107  xor.d
  {30,0,0x80,EMB_NONE,0,0,7,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 108  cmp.b
  {30,0,0x82,EMB_NONE,0,0,7,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 109  cmp.b
  {31,0,0x81,EMB_NONE,0,0,7,7,4,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 110  cmp.d
  {31,0,0x83,EMB_NONE,0,0,7,7,8,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 111  cmp.d
  {32,0,0x84,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  // 112  test
  {32,0,0x85,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 113  test
  {32,0,0xa9,EMB_NONE,0,0,255,10,4,0,OPF_GREG,OPF_GREG,0,1,0,0},  // 114  test
  {32,0,0xf6,EMB_NONE,0,0,0,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 115  test
  {32,0,0xf6,EMB_NONE,0,0,1,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 116  test
  {32,0,0xf7,EMB_NONE,0,0,0,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 117  test
  {32,0,0xf7,EMB_NONE,0,0,1,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 118  test
  {33,0,0xa8,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  // 119  test al,
  {34,0,0xc0,EMB_NONE,0,0,0,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 120  rol
  {34,0,0xc1,EMB_NONE,0,0,0,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 121  rol
  {34,0,0xd0,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 122  rol
  {34,0,0xd1,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 123  rol
  {34,0,0xd2,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 124  rol
  {34,0,0xd3,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 125  rol
  {35,0,0xc0,EMB_NONE,0,0,0,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 126  rol.b
  {35,0,0xd0,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 127  rol.b
  {35,0,0xd2,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 128  rol.b
  {36,0,0xc1,EMB_NONE,0,0,0,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 129  rol.d
  {36,0,0xd1,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 130  rol.d
  {36,0,0xd3,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 131  rol.d
  {37,0,0xc0,EMB_NONE,0,0,1,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 132  ror
  {37,0,0xc1,EMB_NONE,0,0,1,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 133  ror
  {37,0,0xd0,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 134  ror
  {37,0,0xd1,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 135  ror
  {37,0,0xd2,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 136  ror
  {37,0,0xd3,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 137  ror
  {38,0,0xc0,EMB_NONE,0,0,1,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 138  ror.b
  {38,0,0xd0,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 139  ror.b
  {38,0,0xd2,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 140  ror.b
  {39,0,0xc1,EMB_NONE,0,0,1,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 141  ror.d
  {39,0,0xd1,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 142  ror.d
  {39,0,0xd3,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 143  ror.d
  {40,0,0xc0,EMB_NONE,0,0,2,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 144  rcl
  {40,0,0xc1,EMB_NONE,0,0,2,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 145  rcl
  {40,0,0xd0,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 146  rcl
  {40,0,0xd1,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 147  rcl
  {40,0,0xd2,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 148  rcl
  {40,0,0xd3,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 149  rcl
  {41,0,0xc0,EMB_NONE,0,0,2,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 150  rcl.b
  {41,0,0xd0,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 151  rcl.b
  {41,0,0xd2,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 152  rcl.b
  {42,0,0xc1,EMB_NONE,0,0,2,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 153  rcl.d
  {42,0,0xd1,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 154  rcl.d
  {42,0,0xd3,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 155  rcl.d
  {43,0,0xc0,EMB_NONE,0,0,3,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 156  rcr
  {43,0,0xc1,EMB_NONE,0,0,3,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 157  rcr
  {43,0,0xd0,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 158  rcr
  {43,0,0xd1,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 159  rcr
  {43,0,0xd2,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 160  rcr
  {43,0,0xd3,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 161  rcr
  {44,0,0xc0,EMB_NONE,0,0,3,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 162  rcr.b
  {44,0,0xd0,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 163  rcr.b
  {44,0,0xd2,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 164  rcr.b
  {45,0,0xc1,EMB_NONE,0,0,3,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 165  rcr.d
  {45,0,0xd1,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 166  rcr.d
  {45,0,0xd3,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 167  rcr.d
  {46,0,0xc0,EMB_NONE,0,0,4,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 168  shl
  {46,0,0xc1,EMB_NONE,0,0,4,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 169  shl
  {46,0,0xd0,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 170  shl
  {46,0,0xd1,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 171  shl
  {46,0,0xd2,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 172  shl
  {46,0,0xd3,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 173  shl
  {47,0,0xc0,EMB_NONE,0,0,4,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 174  shl.b
  {47,0,0xd0,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 175  shl.b
  {47,0,0xd2,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 176  shl.b
  {48,0,0xc1,EMB_NONE,0,0,4,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 177  shl.d
  {48,0,0xd1,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 178  shl.d
  {48,0,0xd3,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 179  shl.d
  {49,0,0xc0,EMB_NONE,0,0,5,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 180  shr
  {49,0,0xc1,EMB_NONE,0,0,5,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 181  shr
  {49,0,0xd0,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 182  shr
  {49,0,0xd1,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 183  shr
  {49,0,0xd2,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 184  shr
  {49,0,0xd3,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 185  shr
  {50,0,0xc0,EMB_NONE,0,0,5,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 186  shr.b
  {50,0,0xd0,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 187  shr.b
  {50,0,0xd2,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 188  shr.b
  {51,0,0xc1,EMB_NONE,0,0,5,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 189  shr.d
  {51,0,0xd1,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 190  shr.d
  {51,0,0xd3,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 191  shr.d
  {52,0,0xc0,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 192  sal
  {52,0,0xc1,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 193  sal
  {52,0,0xd0,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 194  sal
  {52,0,0xd1,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 195  sal
  {52,0,0xd2,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 196  sal
  {52,0,0xd3,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 197  sal
  {53,0,0xc0,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 198  sal.b
  {53,0,0xd0,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 199  sal.b
  {53,0,0xd2,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 200  sal.b
  {54,0,0xc1,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 201  sal.d
  {54,0,0xd1,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 202  sal.d
  {54,0,0xd3,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 203  sal.d
  {55,0,0xc0,EMB_NONE,0,0,7,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 204  sar
  {55,0,0xc1,EMB_NONE,0,0,7,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 205  sar
  {55,0,0xd0,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 206  sar
  {55,0,0xd1,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 207  sar
  {55,0,0xd2,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 208  sar
  {55,0,0xd3,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 209  sar
  {56,0,0xc0,EMB_NONE,0,0,7,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 210  sar.b
  {56,0,0xd0,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 211  sar.b
  {56,0,0xd2,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 212  sar.b
  {57,0,0xc1,EMB_NONE,0,0,7,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 213  sar.d
  {57,0,0xd1,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 214  sar.d
  {57,0,0xd3,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 215  sar.d
  {58,0,0xf6,EMB_NONE,0,0,0,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 216  test.b
  {58,0,0xf6,EMB_NONE,0,0,1,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 217  test.b
  {59,0,0xf7,EMB_NONE,0,0,0,7,4,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 218  test.d
  {59,0,0xf7,EMB_NONE,0,0,1,7,4,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 219  test.d
  {60,0,0xf6,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 220  not
  {60,0,0xf7,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 221  not
  {61,0,0xf6,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 222  not.b
  {62,0,0xf7,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 223  not.d
  {63,0,0xf6,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 224  neg
  {63,0,0xf7,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 225  neg
  {64,0,0xf6,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 226  neg.b
  {65,0,0xf7,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 227  neg.d
  {66,0,0xf6,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 228  mul
  {66,0,0xf7,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 229  mul
  {67,0,0xf6,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 230  mul.b
  {68,0,0xf7,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 231  mul.d
  {69,0,0x69,EMB_NONE,0,0,255,1,4,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 232  imul
  {69,0,0x6b,EMB_NONE,0,0,255,1,8,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 233  imul
  {69,0,0xf6,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 234  imul
  {69,0,0xf7,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 235  imul
  {69,1,0xaf,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 236  imul
  {70,0,0xf6,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 237  imul.b
  {71,0,0xf7,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 238  imul.d
  {72,0,0xf6,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 239  div
  {72,0,0xf7,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 240  div
  {73,0,0xf6,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 241  div.b
  {74,0,0xf7,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 242  div.d
  {75,0,0xf6,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 243  idiv
  {75,0,0xf7,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 244  idiv
  {76,0,0xf6,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 245  idiv.b
  {77,0,0xf7,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 246  idiv.d
  {78,0,0x40,EMB_REG,0,3,255,2,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 247  inc
  {78,0,0xfe,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 248  inc
  {78,0,0xff,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 249  inc
  {79,0,0x48,EMB_REG,0,3,255,2,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 250  dec
  {79,0,0xfe,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 251  dec
  {79,0,0xff,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 252  dec
  {80,0,0xfe,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 253  inc.b
  {81,0,0xfe,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 254  dec.b
  {82,0,0xff,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 255  inc.d
  {83,0,0xff,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 256  dec.d
  {84,0,0xe8,EMB_NONE,0,0,255,5,6,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 257  call
  {84,0,0xff,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 258  call
  {85,0,0xff,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 259  call.d
  {86,0,0x9a,EMB_NONE,0,0,255,6,7,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 260  callf
  {86,0,0xff,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 261  callf
  {87,0,0xff,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 262  callf.d
  {88,0,0xe9,EMB_NONE,0,0,255,5,6,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 263  jmp
  {88,0,0xeb,EMB_NONE,0,0,255,5,5,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 264  jmp
  {88,0,0xff,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 265  jmp
  {89,0,0xff,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 266  jmp.d
  {90,0,0xea,EMB_NONE,0,0,255,6,7,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 267  jmpf
  {90,0,0xff,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 268  jmpf
  {91,0,0xff,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 269  jmpf.d
  {92,0,0x50,EMB_REG,0,3,255,2,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 270  push
  {92,0,0x68,EMB_NONE,0,0,255,4,4,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 271  push
  {92,0,0x6a,EMB_NONE,0,0,255,4,8,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 272  push
  {92,0,0xff,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 273  push
  {93,0,0xff,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 274  push.d
  {94,0,0x8d,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 275  lea
  {95,0,0x88,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  // 276  mov
  {95,0,0x89,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 277  mov
  {95,0,0x8a,EMB_NONE,0,0,255,1,0,0,OPF_RGB,OPF_RGB,0,0,1,1},  // 278  mov
  {95,0,0x8b,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 279  mov
  {95,0,0x8c,EMB_NONE,0,0,255,1,0,1,OPF_SREG,OPF_GREG,0,0,1,1},  // 280  mov
  {95,0,0x8e,EMB_NONE,0,0,255,1,0,0,OPF_SREG,OPF_GREG,0,0,1,1},  // 281  mov
  {95,0,0xb0,EMB_REG,0,3,255,3,1,0,OPF_RGB,OPF_GREG,0,0,0,0},  // 282  mov
  {95,0,0xb8,EMB_REG,0,3,255,3,4,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 283  mov
  {95,0,0xc6,EMB_NONE,0,0,0,7,1,0,OPF_GREG,OPF_RGB,0,0,1,0},  // 284  mov
  {95,0,0xc7,EMB_NONE,0,0,0,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 285  mov
  {96,0,0xc6,EMB_NONE,0,0,7,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 286  xabort
  {97,0,0xc6,EMB_NONE,0,0,0,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 287  mov.b
  {98,0,0xc7,EMB_NONE,0,0,7,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 288  xbegin
  {99,0,0xc7,EMB_NONE,0,0,0,7,4,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 289  mov.d
  {100,1,0xb6,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_RGB,1,0,1,1},  // 290  movzx
  {100,1,0xb7,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,2,0,1,1},  // 291  movzx
  {101,1,0xbe,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_RGB,1,0,1,1},  // 292  movsx
  {101,1,0xbf,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,2,0,1,1},  // 293  movsx
  {102,0,0x86,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  // 294  xchg
  {102,0,0x87,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 295  xchg
  {102,0,0x90,EMB_REG,0,3,255,2,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 296  xchg
  {103,0,0xd7,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 297  xlat
  {104,1,0xc8,EMB_REG,0,3,255,2,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 298  bswap
  {105,0,0x58,EMB_REG,0,3,255,2,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 299  pop
  {105,0,0x8f,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 300  pop
  {106,0,0x8f,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 301  pop.d
  {107,0,0x06,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 302  push es
  {108,0,0x0e,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 303  push cs
  {109,0,0x16,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 304  push ss
  {110,0,0x1e,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 305  push ds
  {111,0,0x07,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 306  pop es
  {112,0,0x17,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 307  pop ss
  {113,0,0x1f,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 308  pop ds
  {114,1,0xa0,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 309  push fs
  {115,1,0xa8,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 310  push gs
  {116,1,0xa1,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 311  pop fs
  {117,1,0xa9,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 312  pop gs
  {118,0,0x60,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 313  pusha
  {119,0,0x61,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 314  popa
  {120,0,0x9c,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 315  pushf
  {121,0,0x9d,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 316  popf
  {122,0,0x70,EMB_CC,0,4,255,5,5,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 317  jcc
  {122,1,0x80,EMB_CC,0,4,255,5,6,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 318  jcc
  {123,0,0xe0,EMB_NONE,0,0,255,5,5,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 319  loopne
  {124,0,0xe1,EMB_NONE,0,0,255,5,5,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 320  loope
  {125,0,0xe2,EMB_NONE,0,0,255,5,5,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 321  loop
  {126,0,0xe3,EMB_NONE,0,0,255,5,5,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 322  jecxz
  {127,0,0xc2,EMB_NONE,0,0,255,4,2,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 323  ret
  {127,0,0xc3,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 324  ret
  {128,0,0xca,EMB_NONE,0,0,255,4,2,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 325  retf
  {128,0,0xcb,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 326  retf
  {129,0,0xcd,EMB_NONE,0,0,255,4,1,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 327  int
  {130,0,0xcc,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 328  int3
  {131,0,0xce,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 329  into
  {132,0,0xcf,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 330  iret
  {133,0,0xc9,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 331  leave
  {134,0,0xc8,EMB_NONE,0,0,255,6,9,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 332  enter
  {135,1,0x90,EMB_CC,0,4,255,9,0,0,OPF_GREG,OPF_RGB,1,0,1,1},  // 333  setcc
  {136,1,0x40,EMB_CC,0,4,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 334  cmovcc
  {137,1,0xa3,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 335  bt
  {137,1,0xba,EMB_NONE,0,0,4,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 336  bt
  {138,1,0xab,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 337  bts
  {138,1,0xba,EMB_NONE,0,0,5,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 338  bts
  {139,1,0xb3,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 339  btr
  {139,1,0xba,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 340  btr
  {140,1,0xba,EMB_NONE,0,0,7,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 341  btc
  {140,1,0xbb,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 342  btc
  {141,1,0xbc,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 343  bsf
  {142,1,0xbd,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 344  bsr
  {143,1,0xba,EMB_NONE,0,0,4,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 345  bt.d
  {144,1,0xba,EMB_NONE,0,0,5,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 346  bts.d
  {145,1,0xba,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 347  btr.d
  {146,1,0xba,EMB_NONE,0,0,7,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 348  btc.d
  {147,1,0xa4,EMB_NONE,0,0,255,1,1,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 349  shld
  {147,1,0xa5,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 350  shld
  {148,1,0xac,EMB_NONE,0,0,255,1,1,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 351  shrd
  {148,1,0xad,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 352  shrd
  {149,0,0xc4,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 353  les
  {150,0,0xc5,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 354  lds
  {151,1,0xb2,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 355  lss
  {152,1,0xb4,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 356  lfs
  {153,1,0xb5,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 357  lgs
  {154,0,0x62,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 358  bound
  {155,0,0x63,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 359  arpl
  {156,0,0xf5,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 360  cmc
  {157,0,0xf8,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 361  clc
  {158,0,0xf9,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 362  stc
  {159,0,0xfa,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 363  cli
  {160,0,0xfb,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 364  sti
  {161,0,0xfc,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 365  cld
  {162,0,0xfd,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 366  std
  {163,0,0x9e,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 367  sahf
  {164,0,0x9f,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 368  lahf
  {165,0,0x27,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 369  daa
  {166,0,0x2f,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 370  das
  {167,0,0x37,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 371  aaa
  {168,0,0x3f,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 372  aas
  {169,0,0x90,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 373  nop
  {169,1,0x18,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 374  nop
  {169,1,0x1e,EMB_NONE,0,0,255,9,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 375  nop
  {169,1,0x1f,EMB_NONE,0,0,255,9,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 376  nop
  {170,0,0xf4,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 377  hlt
  {171,0,0x9b,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 378  fwait
  {172,0,0xd4,EMB_NONE,0,0,255,4,1,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 379  aam
  {173,0,0xd5,EMB_NONE,0,0,255,4,1,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 380  aad
  {174,1,0xa2,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 381  cpuid
  {175,1,0x31,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 382  rdtsc
  {176,1,0x32,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 383  rdmsr
  {177,1,0x30,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 384  wrmsr
  {178,1,0x0b,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 385  ud2
  {179,0,0x99,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 386  cdq
  {180,0,0x99,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 387  cwd
  {181,0,0x98,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 388  cwde
  {182,0,0x98,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 389  cbw
  {183,0,0xa4,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 390  movsb
  {184,0,0xa6,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 391  cmpsb
  {185,0,0xaa,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 392  stosb
  {186,0,0xac,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 393  lodsb
  {187,0,0xae,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 394  scasb
  {188,0,0xa5,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 395  movsd
  {189,0,0xa5,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 396  movsw
  {190,0,0xa7,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 397  cmpsd
  {191,0,0xa7,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 398  cmpsw
  {192,0,0xab,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 399  stosd
  {193,0,0xab,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 400  stosw
  {194,0,0xad,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 401  lodsd
  {195,0,0xad,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 402  lodsw
  {196,0,0xaf,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 403  scasd
  {197,0,0xaf,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 404  scasw
  {198,0,0xe4,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  // 405  in al,
  {199,0,0xe5,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  // 406  in
  {199,0,0xed,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 407  in
  {200,0,0xe6,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  // 408  out
  {200,0,0xe7,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  // 409  out
  {201,0,0xec,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 410  in al,dx
  {202,0,0xee,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 411  out dx,al
  {203,0,0xef,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 412  out dx,
  {204,0,0x6c,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 413  insb
  {205,0,0x6d,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 414  ins
  {206,0,0x6e,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 415  outsb
  {207,0,0x6f,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 416  outs
  {208,0,0xd8,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 417  fadd st(
  {208,0,0xdc,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 418  fadd st(
  {209,0,0xd8,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 419  fadd.d
  {210,0,0xd8,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 420  fmul st(
  {210,0,0xdc,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 421  fmul st(
  {211,0,0xd8,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 422  fmul.d
  {212,0,0xd8,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 423  fcom st(
  {212,0,0xdc,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 424  fcom st(
  {213,0,0xd8,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 425  fcom.d
  {214,0,0xd8,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 426  fcomp st(
  {214,0,0xdc,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 427  fcomp st(
  {214,0,0xde,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 428  fcomp st(
  {215,0,0xd8,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 429  fcomp.d
  {216,0,0xd8,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 430  fsub st(
  {216,0,0xdc,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 431  fsub st(
  {217,0,0xd8,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 432  fsub.d
  {218,0,0xd8,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 433  fsubr st(
  {218,0,0xdc,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 434  fsubr st(
  {219,0,0xd8,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 435  fsubr.d
  {220,0,0xd8,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 436  fdiv st(
  {220,0,0xdc,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 437  fdiv st(
  {221,0,0xd8,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 438  fdiv.d
  {222,0,0xd8,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 439  fdivr st(
  {222,0,0xdc,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 440  fdivr st(
  {223,0,0xd8,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 441  fdivr.d
  {224,0,0xd9,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 442  fld st(
  {224,0,0xdb,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 443  fld st(
  {224,0,0xdd,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 444  fld st(
  {225,0,0xd9,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 445  fld.d
  {225,0,0xdb,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 446  fld.d
  {226,0,0xd9,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 447  fxch st(
  {227,0,0xd9,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 448  fxch.d
  {228,0,0xd9,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 449  fst st(
  {228,0,0xdd,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 450  fst st(
  {229,0,0xd9,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 451  fst.d
  {230,0,0xd9,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 452  fstp st(
  {230,0,0xdb,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 453  fstp st(
  {230,0,0xdd,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 454  fstp st(
  {231,0,0xd9,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 455  fstp.d
  {231,0,0xdb,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 456  fstp.d
  {232,0,0xd9,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 457  fldenv st(
  {233,0,0xd9,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 458  fldenv.d
  {234,0,0xd9,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 459  fldcw st(
  {235,0,0xd9,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 460  fldcw.d
  {236,0,0xd9,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 461  fnstenv st(
  {237,0,0xd9,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 462  fnstenv.d
  {238,0,0xd9,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 463  fnstcw st(
  {239,0,0xd9,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 464  fnstcw.d
  {240,0,0xda,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 465  fcmovb st(
  {241,0,0xda,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 466  fcmovb.d
  {242,0,0xda,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 467  fcmove st(
  {243,0,0xda,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 468  fcmove.d
  {244,0,0xda,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 469  fcmovbe st(
  {245,0,0xda,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 470  fcmovbe.d
  {246,0,0xda,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 471  fcmovu st(
  {247,0,0xda,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 472  fcmovu.d
  {248,0,0xda,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 473  fx_a st(
  {248,0,0xda,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 474  fx_a st(
  {248,0,0xda,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 475  fx_a st(
  {249,0,0xda,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 476  fx_a.d
  {249,0,0xda,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 477  fx_a.d
  {249,0,0xda,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 478  fx_a.d
  {250,0,0xda,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 479  fucompp st(
  {251,0,0xda,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 480  fucompp.d
  {252,0,0xdb,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 481  fild st(
  {252,0,0xdf,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 482  fild st(
  {252,0,0xdf,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 483  fild st(
  {253,0,0xdb,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 484  fild.d
  {254,0,0xdb,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 485  fisttp st(
  {254,0,0xdd,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 486  fisttp st(
  {254,0,0xdf,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 487  fisttp st(
  {255,0,0xdb,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 488  fisttp.d
  {256,0,0xdb,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 489  fist st(
  {256,0,0xdf,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 490  fist st(
  {257,0,0xdb,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 491  fist.d
  {258,0,0xdb,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 492  fistp st(
  {258,0,0xdf,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 493  fistp st(
  {258,0,0xdf,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 494  fistp st(
  {259,0,0xdb,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 495  fistp.d
  {260,0,0xdb,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 496  fx_b st(
  {260,0,0xdb,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 497  fx_b st(
  {261,0,0xdb,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 498  fx_b.d
  {261,0,0xdb,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 499  fx_b.d
  {262,0,0xdc,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 500  fadd.q
  {263,0,0xdc,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 501  fmul.q
  {264,0,0xdc,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 502  fcom.q
  {265,0,0xdc,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 503  fcomp.q
  {266,0,0xdc,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 504  fsub.q
  {267,0,0xdc,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 505  fsubr.q
  {268,0,0xdc,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 506  fdiv.q
  {269,0,0xdc,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 507  fdivr.q
  {270,0,0xdd,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 508  fld.q
  {271,0,0xdd,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 509  fisttp.q
  {272,0,0xdd,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 510  fst.q
  {273,0,0xdd,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 511  fstp.q
  {274,0,0xdd,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 512  frstor st(
  {275,0,0xdd,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 513  frstor.q
  {276,0,0xdd,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 514  fx_d st(
  {277,0,0xdd,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 515  fx_d.q
  {278,0,0xdd,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 516  fnsave st(
  {279,0,0xdd,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 517  fnsave.q
  {280,0,0xdd,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 518  fnstsw st(
  {281,0,0xdd,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 519  fnstsw.q
  {282,0,0xde,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 520  faddp st(
  {283,0,0xde,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 521  faddp.w
  {284,0,0xde,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 522  fmulp st(
  {285,0,0xde,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 523  fmulp.w
  {286,0,0xde,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 524  fcomp.w
  {287,0,0xde,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 525  fcompp st(
  {288,0,0xde,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 526  fcompp.w
  {289,0,0xde,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 527  fsubrp st(
  {290,0,0xde,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 528  fsubrp.w
  {291,0,0xde,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 529  fsubp st(
  {292,0,0xde,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 530  fsubp.w
  {293,0,0xde,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 531  fdivrp st(
  {294,0,0xde,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 532  fdivrp.w
  {295,0,0xde,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 533  fdivp st(
  {296,0,0xde,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 534  fdivp.w
  {297,0,0xdf,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 535  fild.w
  {297,0,0xdf,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 536  fild.w
  {298,0,0xdf,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 537  fisttp.w
  {299,0,0xdf,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 538  fist.w
  {300,0,0xdf,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 539  fistp.w
  {300,0,0xdf,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 540  fistp.w
  {301,0,0xdf,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 541  fbld st(
  {302,0,0xdf,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 542  fbld.w
  {303,0,0xdf,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 543  fbstp st(
  {304,0,0xdf,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 544  fbstp.w
  {305,1,0x6f,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 545  movq
  {305,1,0xd6,EMB_NONE,0,0,255,1,0,1,OPF_XMM,OPF_XMM,0,0,1,1},  // 546  movq
  {306,1,0xfc,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 547  paddb
  {307,1,0xfd,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 548  paddw
  {308,1,0xfe,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 549  paddd
  {309,1,0xf8,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 550  psubb
  {310,1,0xdb,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 551  pand
  {311,1,0xeb,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 552  por
  {312,1,0xef,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 553  pxor
  {313,1,0x74,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 554  pcmpeqb
  {314,1,0x63,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 555  packsswb
  {315,1,0x60,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 556  punpcklbw
  {316,1,0x71,EMB_NONE,0,0,2,7,1,0,OPF_GREG,OPF_SSE_OS,0,0,1,0},  // 557  psrlw
  {316,1,0xd1,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 558  psrlw
  {317,1,0x72,EMB_NONE,0,0,2,7,1,0,OPF_GREG,OPF_SSE_OS,0,0,1,0},  // 559  psrld
  {317,1,0xd2,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 560  psrld
  {318,1,0x73,EMB_NONE,0,0,2,7,1,0,OPF_GREG,OPF_SSE_OS,0,0,1,0},  // 561  psrlq
  {318,1,0xd3,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 562  psrlq
  {319,1,0x71,EMB_NONE,0,0,4,7,1,0,OPF_GREG,OPF_SSE_OS,0,0,1,0},  // 563  psraw
  {319,1,0xe1,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 564  psraw
  {320,1,0x72,EMB_NONE,0,0,4,7,1,0,OPF_GREG,OPF_SSE_OS,0,0,1,0},  // 565  psrad
  {320,1,0xe2,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 566  psrad
  {321,1,0x71,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_SSE_OS,0,0,1,0},  // 567  psllw
  {321,1,0xf1,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 568  psllw
  {322,1,0x72,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_SSE_OS,0,0,1,0},  // 569  pslld
  {322,1,0xf2,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 570  pslld
  {323,1,0x73,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_SSE_OS,0,0,1,0},  // 571  psllq
  {323,1,0xf3,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 572  psllq
  {324,1,0x28,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 573  movaps
  {324,1,0x29,EMB_NONE,0,0,255,1,0,1,OPF_XMM,OPF_XMM,0,0,1,1},  // 574  movaps
  {325,1,0x10,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 575  movups
  {325,1,0x11,EMB_NONE,0,0,255,1,0,1,OPF_XMM,OPF_XMM,0,0,1,1},  // 576  movups
  {326,1,0x58,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 577  addps
  {327,1,0x59,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 578  mulps
  {328,1,0x5c,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 579  subps
  {329,1,0x2f,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 580  comiss
  {330,1,0x57,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 581  xorps
  {331,1,0x54,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 582  andps
  {332,1,0x2e,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 583  ucomiss
  {333,1,0x2e,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 584  ucomisd
  {334,1,0x6e,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_GREG,0,0,1,1},  // 585  movd
  {334,1,0x7e,EMB_NONE,0,0,255,1,0,1,OPF_SSE_OS,OPF_GREG,0,0,1,1},  // 586  movd
  {335,1,0x2a,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_GREG,0,0,1,1},  // 587  cvtpi2ps
  {336,1,0x2c,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_XMM,0,0,1,1},  // 588  cvttps2pi
  {337,1,0x2d,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_XMM,0,0,1,1},  // 589  cvtps2pi
  {338,1,0x70,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 590  pshufw
  {339,1,0x77,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 591  emms
  {340,1,0xae,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 592  ptwrite
  {341,1,0xae,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 593  lfence
  {342,1,0xae,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 594  mfence
  {343,1,0xae,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 595  sfence
  {344,1,0xae,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 596  clflush.b
  {345,1,0x18,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 597  prefetchnta.b
  {346,1,0x18,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 598  prefetcht0.b
  {347,1,0x18,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 599  prefetcht1.b
  {348,1,0x18,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 600  prefetcht2.b
  {349,1,0x18,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 601  nop18
  {349,1,0x18,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 602  nop18
  {349,1,0x18,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 603  nop18
  {349,1,0x18,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 604  nop18
  {349,1,0x18,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 605  nop18
  {349,1,0x18,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 606  nop18
  {349,1,0x18,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 607  nop18
  {349,1,0x18,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 608  nop18
  {349,1,0x18,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 609  nop18
  {350,1,0x18,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 610  prefetchit1.b
  {351,1,0x18,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 611  prefetchit0.b
  {352,1,0xc0,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  // 612  xadd
  {352,1,0xc1,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 613  xadd
  {353,1,0xb0,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  // 614  cmpxchg
  {353,1,0xb1,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 615  cmpxchg
  {354,1,0xc7,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 616  cmpxchg8b
  {355,1,0xc7,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 617  xrstors
  {356,1,0xc7,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 618  xsavec
  {357,1,0xc7,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 619  xsaves
  {358,1,0xc7,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 620  vmptrld
  {359,1,0xc7,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 621  vmptrst
  {360,1,0xc7,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 622  rdrand
  {361,1,0xc7,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 623  rdseed
  {362,1,0x37,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 624  getsec
  {363,1,0x02,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 625  lar
  {364,1,0x03,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 626  lsl
  {365,1,0x06,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 627  clts
  {366,1,0x08,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 628  invd
  {367,1,0x09,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 629  wbinvd
  {368,1,0x00,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 630  sldt
  {369,1,0x00,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 631  sldt.w
  {370,1,0x00,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 632  str
  {371,1,0x00,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 633  str.w
  {372,1,0x00,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 634  lldt
  {373,1,0x00,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 635  lldt.w
  {374,1,0x00,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 636  ltr
  {375,1,0x00,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 637  ltr.w
  {376,1,0x00,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 638  verr
  {377,1,0x00,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 639  verr.w
  {378,1,0x00,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 640  verw
  {379,1,0x00,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 641  verw.w
  {380,1,0x01,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 642  sgdt
  {381,1,0x01,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 643  sidt
  {382,1,0x01,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 644  lgdt
  {383,1,0x01,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 645  lidt
  {384,1,0x01,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 646  smsw
  {384,1,0x01,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 647  smsw
  {385,1,0x01,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 648  lmsw
  {385,1,0x01,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 649  lmsw
  {386,1,0x01,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 650  invlpg
  {387,1,0x01,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 651  rstorssp
  {387,1,0x01,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 652  rstorssp
  {388,1,0xec,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 653  paddsb
  {389,1,0xed,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 654  paddsw
  {390,1,0xdc,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 655  paddusb
  {391,1,0xdd,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 656  paddusw
  {392,1,0xf9,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 657  psubw
  {393,1,0xfa,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 658  psubd
  {394,1,0xe8,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 659  psubsb
  {395,1,0xe9,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 660  psubsw
  {396,1,0xd8,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 661  psubusb
  {397,1,0xd9,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 662  psubusw
  {398,1,0xd5,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 663  pmullw
  {399,1,0xe5,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 664  pmulhw
  {400,1,0xf5,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 665  pmaddwd
  {401,1,0xdf,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 666  pandn
  {402,1,0x64,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 667  pcmpgtb
  {403,1,0x65,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 668  pcmpgtw
  {404,1,0x66,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 669  pcmpgtd
  {405,1,0x67,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 670  packuswb
  {406,1,0x6b,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 671  packssdw
  {407,1,0x68,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 672  punpckhbw
  {408,1,0x69,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 673  punpckhwd
  {409,1,0x6a,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 674  punpckhdq
  {410,1,0x61,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 675  punpcklwd
  {411,1,0x62,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 676  punpckldq
  {412,1,0xf4,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 677  pmuludq
  {413,1,0x75,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 678  pcmpeqw
  {414,1,0x76,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 679  pcmpeqd
  {415,1,0xd4,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 680  paddq
  {416,1,0xfb,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 681  psubq
  {417,1,0xe0,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 682  pavgb
  {418,1,0xe3,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 683  pavgw
  {419,1,0xda,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 684  pminub
  {420,1,0xde,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 685  pmaxub
  {421,1,0xea,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 686  pminsw
  {422,1,0xee,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 687  pmaxsw
  {423,1,0xe4,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 688  pmulhuw
  {424,1,0xf6,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 689  psadbw
  {425,1,0xd7,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_SSE_OS,0,0,1,0},  // 690  pmovmskb
  {426,1,0xc4,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_GREG,0,0,1,1},  // 691  pinsrw
  {427,1,0xc5,EMB_NONE,0,0,255,1,1,0,OPF_GREG,OPF_SSE_OS,0,0,1,0},  // 692  pextrw
  {427,3,0x15,EMB_NONE,0,0,255,1,1,1,OPF_SSE_OS,OPF_GREG,0,0,1,1},  // 693  pextrw
  {428,1,0x73,EMB_NONE,0,0,3,7,1,0,OPF_GREG,OPF_SSE_OS,0,0,1,0},  // 694  psrldq
  {429,1,0x73,EMB_NONE,0,0,7,7,1,0,OPF_GREG,OPF_SSE_OS,0,0,1,0},  // 695  pslldq
  {430,1,0x51,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 696  sqrtps
  {431,1,0x52,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 697  rsqrtps
  {432,1,0x53,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 698  rcpps
  {433,1,0x55,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 699  andnps
  {434,1,0x56,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 700  orps
  {435,1,0x5d,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 701  minps
  {436,1,0x5e,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 702  divps
  {437,1,0x5f,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 703  maxps
  {438,1,0x14,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 704  unpcklps
  {439,1,0x15,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 705  unpckhps
  {440,1,0x12,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 706  movlps
  {440,1,0x13,EMB_NONE,0,0,255,1,0,1,OPF_XMM,OPF_GREG,0,0,0,1},  // 707  movlps
  {441,1,0x16,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 708  movhps
  {441,1,0x17,EMB_NONE,0,0,255,1,0,1,OPF_XMM,OPF_GREG,0,0,0,1},  // 709  movhps
  {442,1,0x50,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_XMM,0,0,1,0},  // 710  movmskps
  {443,1,0xc2,EMB_NONE,0,0,255,1,1,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 711  cmpps
  {444,1,0xc6,EMB_NONE,0,0,255,1,1,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 712  shufps
  {445,1,0xd0,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 713  addsubps
  {446,1,0x7c,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 714  haddps
  {447,1,0x7d,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 715  hsubps
  {448,1,0x5a,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 716  cvtps2pd
  {449,1,0x5b,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 717  cvtdq2ps
  {450,1,0xe6,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 718  cvtpd2dq
  {451,1,0x6c,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 719  punpcklqdq
  {452,1,0x6d,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 720  punpckhqdq
  {453,1,0xf0,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_GREG,0,0,0,1},  // 721  lddqu
  {454,1,0x2b,EMB_NONE,0,0,255,1,0,1,OPF_XMM,OPF_GREG,0,0,0,1},  // 722  movntps
  {455,1,0xe7,EMB_NONE,0,0,255,1,0,1,OPF_SSE_OS,OPF_GREG,0,0,0,1},  // 723  movntq
  {456,1,0xc3,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,0,1},  // 724  movnti
  {457,1,0xf7,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,0},  // 725  maskmovq
  {458,1,0xae,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 726  fxsave
  {459,1,0xae,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 727  fxrstor
  {460,1,0xae,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 728  ldmxcsr
  {461,1,0xae,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 729  stmxcsr
  {462,1,0xae,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 730  xsave
  {463,1,0xae,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 731  xrstor
  {464,1,0xae,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 732  xsaveopt
  {465,2,0x00,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 733  pshufb
  {466,2,0x01,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 734  phaddw
  {467,2,0x02,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 735  phaddd
  {468,2,0x03,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 736  phaddsw
  {469,2,0x04,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 737  pmaddubsw
  {470,2,0x05,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 738  phsubw
  {471,2,0x06,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 739  phsubd
  {472,2,0x07,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 740  phsubsw
  {473,2,0x08,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 741  psignb
  {474,2,0x09,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 742  psignw
  {475,2,0x0a,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 743  psignd
  {476,2,0x0b,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 744  pmulhrsw
  {477,2,0x1c,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 745  pabsb
  {478,2,0x1d,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 746  pabsw
  {479,2,0x1e,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 747  pabsd
  {480,2,0x10,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 748  pblendvb
  {481,2,0x14,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 749  blendvps
  {482,2,0x15,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 750  blendvpd
  {483,2,0x17,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 751  ptest
  {484,2,0x20,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 752  pmovsxbw
  {485,2,0x21,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 753  pmovsxbd
  {486,2,0x22,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 754  pmovsxbq
  {487,2,0x23,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 755  pmovsxwd
  {488,2,0x24,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 756  pmovsxwq
  {489,2,0x25,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 757  pmovsxdq
  {490,2,0x28,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 758  pmuldq
  {491,2,0x29,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 759  pcmpeqq
  {492,2,0x2b,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 760  packusdw
  {493,2,0x30,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 761  pmovzxbw
  {494,2,0x31,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 762  pmovzxbd
  {495,2,0x32,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 763  pmovzxbq
  {496,2,0x33,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 764  pmovzxwd
  {497,2,0x34,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 765  pmovzxwq
  {498,2,0x35,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 766  pmovzxdq
  {499,2,0x37,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 767  pcmpgtq
  {500,2,0x38,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 768  pminsb
  {501,2,0x39,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 769  pminsd
  {502,2,0x3a,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 770  pminuw
  {503,2,0x3b,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 771  pminud
  {504,2,0x3c,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 772  pmaxsb
  {505,2,0x3d,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 773  pmaxsd
  {506,2,0x3e,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 774  pmaxuw
  {507,2,0x3f,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 775  pmaxud
  {508,2,0x40,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 776  pmulld
  {509,2,0x41,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 777  phminposuw
  {510,2,0x2a,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_GREG,0,0,0,1},  // 778  movntdqa
  {511,2,0xdb,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 779  aesimc
  {512,2,0xdc,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 780  aesenc
  {513,2,0xdd,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 781  aesenclast
  {514,2,0xde,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 782  aesdec
  {515,2,0xdf,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 783  aesdeclast
  {516,2,0xc8,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 784  sha1nexte
  {517,2,0xc9,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 785  sha1msg1
  {518,2,0xca,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 786  sha1msg2
  {519,2,0xcb,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 787  sha256rnds2
  {520,2,0xcc,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 788  sha256msg1
  {521,2,0xcd,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 789  sha256msg2
  {522,3,0x08,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 790  roundps
  {523,3,0x09,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 791  roundpd
  {524,3,0x0a,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 792  roundss
  {525,3,0x0b,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 793  roundsd
  {526,3,0x0c,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 794  blendps
  {527,3,0x0d,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 795  blendpd
  {528,3,0x0e,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 796  pblendw
  {529,3,0x0f,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 797  palignr
  {530,3,0x21,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 798  insertps
  {531,3,0x40,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 799  dpps
  {532,3,0x41,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 800  dppd
  {533,3,0x42,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 801  mpsadbw
  {534,3,0x44,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 802  pclmulqdq
  {535,3,0x60,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 803  pcmpestrm
  {536,3,0x61,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 804  pcmpestri
  {537,3,0x62,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 805  pcmpistrm
  {538,3,0x63,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 806  pcmpistri
  {539,3,0xcc,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 807  sha1rnds4
  {540,3,0xdf,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 808  aeskeygenassist
  {541,3,0x14,EMB_NONE,0,0,255,1,1,1,OPF_SSE_OS,OPF_GREG,0,0,1,1},  // 809  pextrb
  {542,3,0x16,EMB_NONE,0,0,255,1,1,1,OPF_SSE_OS,OPF_GREG,0,0,1,1},  // 810  pextrd
  {543,3,0x17,EMB_NONE,0,0,255,1,1,1,OPF_SSE_OS,OPF_GREG,0,0,1,1},  // 811  extractps
  {544,3,0x20,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_GREG,0,0,1,1},  // 812  pinsrb
  {545,3,0x22,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_GREG,0,0,1,1},  // 813  pinsrd
  {546,1,0x34,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 814  sysenter
  {547,1,0x35,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 815  sysexit
  {548,1,0xaa,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 816  rsm
  {549,1,0x33,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 817  rdpmc
  {550,1,0x01,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 818  monitor
  {551,1,0x01,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 819  rdtscp
  {552,1,0xb8,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 820  popcnt
  {553,2,0xf0,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_RGB,0,0,1,1},  // 821  crc32
  {553,2,0xf1,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 822  crc32
  {554,1,0x0d,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 823  prefetch
  {555,1,0x0d,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 824  prefetchw
  {556,1,0x0d,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 825  prefetchwt1
  {557,1,0x0d,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 826  prefetch3
  {558,1,0x0d,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 827  prefetch4
  {559,1,0x0d,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 828  prefetch5
  {560,1,0x0d,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 829  prefetch6
  {561,1,0x0d,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 830  prefetch7
  {562,1,0x0d,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 831  nop0d
  {562,1,0x0d,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 832  nop0d
  {562,1,0x0d,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 833  nop0d
  {562,1,0x0d,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 834  nop0d
  {562,1,0x0d,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 835  nop0d
  {562,1,0x0d,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 836  nop0d
  {562,1,0x0d,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 837  nop0d
  {562,1,0x0d,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 838  nop0d
  {563,1,0x01,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 839  vmcall
  {564,1,0x01,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 840  xgetbv
  {565,1,0x01,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 841  vmrun
  {566,1,0xb9,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 842  ud1
  {567,0,0xf1,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 843  int1
  {568,1,0x0f,EMB_NONE,0,0,255,1,1,0,OPF_MM,OPF_MM,0,0,1,1},  // 844  _3dnow
  {569,0,0xd6,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 845  salc
  {570,1,0x05,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 846  syscall
  {571,1,0x07,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 847  sysret
  {572,1,0x0e,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 848  femms
  {573,1,0x7f,EMB_NONE,0,0,255,1,0,1,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 849  movdqa
  {574,1,0x78,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 850  vmread
  {575,1,0x79,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 851  vmwrite
  {576,1,0xff,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 852  ud0
  {577,1,0x19,EMB_NONE,0,0,255,9,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 853  nop19
  {578,1,0x1a,EMB_NONE,0,0,255,9,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 854  nop1a
  {579,1,0x1b,EMB_NONE,0,0,255,9,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 855  nop1b
  {580,1,0x1d,EMB_NONE,0,0,255,9,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 856  nop1d
  {581,1,0x1c,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 857  cldemote
  {582,1,0x1c,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 858  nop1c
  {582,1,0x1c,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 859  nop1c
  {582,1,0x1c,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 860  nop1c
  {582,1,0x1c,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 861  nop1c
  {582,1,0x1c,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 862  nop1c
  {582,1,0x1c,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 863  nop1c
  {582,1,0x1c,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 864  nop1c
  {582,1,0x1c,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 865  nop1c
  {582,1,0x1c,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 866  nop1c
  {582,1,0x1c,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 867  nop1c
  {582,1,0x1c,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 868  nop1c
  {582,1,0x1c,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 869  nop1c
  {582,1,0x1c,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 870  nop1c
  {582,1,0x1c,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 871  nop1c
  {582,1,0x1c,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 872  nop1c
  {583,1,0xa6,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 873  montmul
  {584,1,0xa6,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 874  xsha1
  {585,1,0xa6,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 875  xsha256
  {586,1,0xa7,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 876  xstore
  {587,1,0xa7,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 877  xcryptecb
  {588,1,0xa7,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 878  xcryptcbc
  {589,1,0xa7,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 879  xcryptctr
  {590,1,0xa7,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 880  xcryptcfb
  {591,1,0xa7,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 881  xcryptofb
  {592,2,0x80,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 882  invept
  {593,2,0x81,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 883  invvpid
  {594,2,0x82,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 884  invpcid
  {595,2,0xcf,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 885  gf2p8mulb
  {596,2,0xf6,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 886  adcx
  {597,2,0xf5,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,0,1},  // 887  wrssd
  {598,2,0xf8,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 888  movdir64b
  {599,2,0xf9,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,0,1},  // 889  movdiri
  {600,2,0xfc,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,0,1},  // 890  aadd
  {601,2,0xfa,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 891  encodekey128
  {602,2,0xfb,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 892  encodekey256
  {603,2,0xd8,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 893  aesencwide128kl
  {604,2,0xd8,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 894  aesdecwide128kl
  {605,2,0xd8,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 895  aesencwide256kl
  {606,2,0xd8,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 896  aesdecwide256kl
  {607,3,0xce,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 897  gf2p8affineqb
  {608,3,0xcf,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 898  gf2p8affineinvqb
  {609,3,0xf0,EMB_NONE,0,0,255,9,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 899  hreset
};
static const size_t enc_cand_count = sizeof(enc_cand)/sizeof(enc_cand[0]);

// per-mnemonic bucket: candidates for mnem M are enc_cand[start..start+count).
static const uint16_t enc_bucket_start[] = {
    0, 9, 10, 19, 20, 29, 30, 39, 40, 49, 50, 59, 60, 69, 70, 79, 80, 82, 84, 86, 88, 90, 92, 94, 96, 98, 100, 102, 104, 106, 108, 110, 112, 119, 120, 126, 129, 132, 138, 141, 144, 150, 153, 156, 162, 165, 168, 174, 177, 180, 186, 189, 192, 198, 201, 204, 210, 213, 216, 218, 220, 222, 223, 224, 226, 227, 228, 230, 231, 232, 237, 238, 239, 241, 242, 243, 245, 246, 247, 250, 253, 254, 255, 256, 257, 259, 260, 262, 263, 266, 267, 269, 270, 274, 275, 276, 286, 287, 288, 289, 290, 292, 294, 297, 298, 299, 301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313, 314, 315, 316, 317, 319, 320, 321, 322, 323, 325, 327, 328, 329, 330, 331, 332, 333, 334, 335, 337, 339, 341, 343, 344, 345, 346, 347, 348, 349, 351, 353, 354, 355, 356, 357, 358, 359, 360, 361, 362, 363, 364, 365, 366, 367, 368, 369, 370, 371, 372, 373, 377, 378, 379, 380, 381, 382, 383, 384, 385, 386, 387, 388, 389, 390, 391, 392, 393, 394, 395, 396, 397, 398, 399, 400, 401, 402, 403, 404, 405, 406, 408, 410, 411, 412, 413, 414, 415, 416, 417, 419, 420, 422, 423, 425, 426, 429, 430, 432, 433, 435, 436, 438, 439, 441, 442, 445, 447, 448, 449, 451, 452, 455, 457, 458, 459, 460, 461, 462, 463, 464, 465, 466, 467, 468, 469, 470, 471, 472, 473, 476, 479, 480, 481, 484, 485, 488, 489, 491, 492, 495, 496, 498, 500, 501, 502, 503, 504, 505, 506, 507, 508, 509, 510, 511, 512, 513, 514, 515, 516, 517, 518, 519, 520, 521, 522, 523, 524, 525, 526, 527, 528, 529, 530, 531, 532, 533, 534, 535, 537, 538, 539, 541, 542, 543, 544, 545, 547, 548, 549, 550, 551, 552, 553, 554, 555, 556, 557, 559, 561, 563, 565, 567, 569, 571, 573, 575, 577, 578, 579, 580, 581, 582, 583, 584, 585, 587, 588, 589, 590, 591, 592, 593, 594, 595, 596, 597, 598, 599, 600, 601, 610, 611, 612, 614, 616, 617, 618, 619, 620, 621, 622, 623, 624, 625, 626, 627, 628, 629, 630, 631, 632, 633, 634, 635, 636, 637, 638, 639, 640, 641, 642, 643, 644, 645, 646, 648, 650, 651, 653, 654, 655, 656, 657, 658, 659, 660, 661, 662, 663, 664, 665, 666, 667, 668, 669, 670, 671, 672, 673, 674, 675, 676, 677, 678, 679, 680, 681, 682, 683, 684, 685, 686, 687, 688, 689, 690, 691, 692, 694, 695, 696, 697, 698, 699, 700, 701, 702, 703, 704, 705, 706, 708, 710, 711, 712, 713, 714, 715, 716, 717, 718, 719, 720, 721, 722, 723, 724, 725, 726, 727, 728, 729, 730, 731, 732, 733, 734, 735, 736, 737, 738, 739, 740, 741, 742, 743, 744, 745, 746, 747, 748, 749, 750, 751, 752, 753, 754, 755, 756, 757, 758, 759, 760, 761, 762, 763, 764, 765, 766, 767, 768, 769, 770, 771, 772, 773, 774, 775, 776, 777, 778, 779, 780, 781, 782, 783, 784, 785, 786, 787, 788, 789, 790, 791, 792, 793, 794, 795, 796, 797, 798, 799, 800, 801, 802, 803, 804, 805, 806, 807, 808, 809, 810, 811, 812, 813, 814, 815, 816, 817, 818, 819, 820, 821, 823, 824, 825, 826, 827, 828, 829, 830, 831, 839, 840, 841, 842, 843, 844, 845, 846, 847, 848, 849, 850, 851, 852, 853, 854, 855, 856, 857, 858, 873, 874, 875, 876, 877, 878, 879, 880, 881, 882, 883, 884, 885, 886, 887, 888, 889, 890, 891, 892, 893, 894, 895, 896, 897, 898, 899
};
static const uint16_t enc_bucket_count[] = {
    9, 1, 9, 1, 9, 1, 9, 1, 9, 1, 9, 1, 9, 1, 9, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 7, 1, 6, 3, 3, 6, 3, 3, 6, 3, 3, 6, 3, 3, 6, 3, 3, 6, 3, 3, 6, 3, 3, 6, 3, 3, 2, 2, 2, 1, 1, 2, 1, 1, 2, 1, 1, 5, 1, 1, 2, 1, 1, 2, 1, 1, 3, 3, 1, 1, 1, 1, 2, 1, 2, 1, 3, 1, 2, 1, 4, 1, 1, 10, 1, 1, 1, 1, 2, 2, 3, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 2, 1, 3, 1, 2, 1, 2, 1, 2, 1, 2, 1, 3, 2, 1, 1, 2, 1, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 1, 1, 3, 1, 3, 1, 2, 1, 3, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 9, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 15, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};
static const size_t enc_nmnem = 610;

#endif // X86_TABLES_ENC_H
