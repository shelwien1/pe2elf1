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
  {0,0,0x80,EMB_NONE,0,0,0,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  //   5  add
  {0,0,0x81,EMB_NONE,0,0,0,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  //   6  add
  {0,0,0x82,EMB_NONE,0,0,0,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  //   7  add
  {0,0,0x83,EMB_NONE,0,0,0,7,8,0,OPF_GREG,OPF_GREG,0,0,1,0},  //   8  add
  {1,0,0x04,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  //   9  add al,
  {2,0,0x08,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  //  10  or
  {2,0,0x09,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  //  11  or
  {2,0,0x0a,EMB_NONE,0,0,255,1,0,0,OPF_RGB,OPF_RGB,0,0,1,1},  //  12  or
  {2,0,0x0b,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  //  13  or
  {2,0,0x0d,EMB_NONE,0,0,255,10,4,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  14  or
  {2,0,0x80,EMB_NONE,0,0,1,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  15  or
  {2,0,0x81,EMB_NONE,0,0,1,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  16  or
  {2,0,0x82,EMB_NONE,0,0,1,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  17  or
  {2,0,0x83,EMB_NONE,0,0,1,7,8,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  18  or
  {3,0,0x0c,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  19  or al,
  {4,0,0x10,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  //  20  adc
  {4,0,0x11,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  //  21  adc
  {4,0,0x12,EMB_NONE,0,0,255,1,0,0,OPF_RGB,OPF_RGB,0,0,1,1},  //  22  adc
  {4,0,0x13,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  //  23  adc
  {4,0,0x15,EMB_NONE,0,0,255,10,4,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  24  adc
  {4,0,0x80,EMB_NONE,0,0,2,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  25  adc
  {4,0,0x81,EMB_NONE,0,0,2,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  26  adc
  {4,0,0x82,EMB_NONE,0,0,2,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  27  adc
  {4,0,0x83,EMB_NONE,0,0,2,7,8,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  28  adc
  {5,0,0x14,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  29  adc al,
  {6,0,0x18,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  //  30  sbb
  {6,0,0x19,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  //  31  sbb
  {6,0,0x1a,EMB_NONE,0,0,255,1,0,0,OPF_RGB,OPF_RGB,0,0,1,1},  //  32  sbb
  {6,0,0x1b,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  //  33  sbb
  {6,0,0x1d,EMB_NONE,0,0,255,10,4,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  34  sbb
  {6,0,0x80,EMB_NONE,0,0,3,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  35  sbb
  {6,0,0x81,EMB_NONE,0,0,3,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  36  sbb
  {6,0,0x82,EMB_NONE,0,0,3,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  37  sbb
  {6,0,0x83,EMB_NONE,0,0,3,7,8,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  38  sbb
  {7,0,0x1c,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  39  sbb al,
  {8,0,0x20,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  //  40  and
  {8,0,0x21,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  //  41  and
  {8,0,0x22,EMB_NONE,0,0,255,1,0,0,OPF_RGB,OPF_RGB,0,0,1,1},  //  42  and
  {8,0,0x23,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  //  43  and
  {8,0,0x25,EMB_NONE,0,0,255,10,4,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  44  and
  {8,0,0x80,EMB_NONE,0,0,4,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  45  and
  {8,0,0x81,EMB_NONE,0,0,4,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  46  and
  {8,0,0x82,EMB_NONE,0,0,4,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  47  and
  {8,0,0x83,EMB_NONE,0,0,4,7,8,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  48  and
  {9,0,0x24,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  49  and al,
  {10,0,0x28,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  //  50  sub
  {10,0,0x29,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  //  51  sub
  {10,0,0x2a,EMB_NONE,0,0,255,1,0,0,OPF_RGB,OPF_RGB,0,0,1,1},  //  52  sub
  {10,0,0x2b,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  //  53  sub
  {10,0,0x2d,EMB_NONE,0,0,255,10,4,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  54  sub
  {10,0,0x80,EMB_NONE,0,0,5,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  55  sub
  {10,0,0x81,EMB_NONE,0,0,5,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  56  sub
  {10,0,0x82,EMB_NONE,0,0,5,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  57  sub
  {10,0,0x83,EMB_NONE,0,0,5,7,8,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  58  sub
  {11,0,0x2c,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  59  sub al,
  {12,0,0x30,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  //  60  xor
  {12,0,0x31,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  //  61  xor
  {12,0,0x32,EMB_NONE,0,0,255,1,0,0,OPF_RGB,OPF_RGB,0,0,1,1},  //  62  xor
  {12,0,0x33,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  //  63  xor
  {12,0,0x35,EMB_NONE,0,0,255,10,4,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  64  xor
  {12,0,0x80,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  65  xor
  {12,0,0x81,EMB_NONE,0,0,6,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  66  xor
  {12,0,0x82,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  67  xor
  {12,0,0x83,EMB_NONE,0,0,6,7,8,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  68  xor
  {13,0,0x34,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  69  xor al,
  {14,0,0x38,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  //  70  cmp
  {14,0,0x39,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  //  71  cmp
  {14,0,0x3a,EMB_NONE,0,0,255,1,0,0,OPF_RGB,OPF_RGB,0,0,1,1},  //  72  cmp
  {14,0,0x3b,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  //  73  cmp
  {14,0,0x3d,EMB_NONE,0,0,255,10,4,0,OPF_GREG,OPF_GREG,0,1,0,0},  //  74  cmp
  {14,0,0x80,EMB_NONE,0,0,7,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  75  cmp
  {14,0,0x81,EMB_NONE,0,0,7,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  76  cmp
  {14,0,0x82,EMB_NONE,0,0,7,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  //  77  cmp
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
  {32,0,0xf6,EMB_NONE,0,0,0,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 115  test
  {32,0,0xf6,EMB_NONE,0,0,1,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 116  test
  {32,0,0xf7,EMB_NONE,0,0,0,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 117  test
  {32,0,0xf7,EMB_NONE,0,0,1,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 118  test
  {33,0,0xa8,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  // 119  test al,
  {34,0,0xc0,EMB_NONE,0,0,0,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 120  rol
  {34,0,0xc1,EMB_NONE,0,0,0,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 121  rol
  {34,0,0xd0,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 122  rol
  {34,0,0xd1,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 123  rol
  {34,0,0xd2,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 124  rol
  {34,0,0xd3,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 125  rol
  {35,0,0xc0,EMB_NONE,0,0,0,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 126  rol.b
  {35,0,0xd0,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 127  rol.b
  {35,0,0xd2,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 128  rol.b
  {36,0,0xc1,EMB_NONE,0,0,0,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 129  rol.d
  {36,0,0xd1,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 130  rol.d
  {36,0,0xd3,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 131  rol.d
  {37,0,0xc0,EMB_NONE,0,0,1,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 132  ror
  {37,0,0xc1,EMB_NONE,0,0,1,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 133  ror
  {37,0,0xd0,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 134  ror
  {37,0,0xd1,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 135  ror
  {37,0,0xd2,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 136  ror
  {37,0,0xd3,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 137  ror
  {38,0,0xc0,EMB_NONE,0,0,1,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 138  ror.b
  {38,0,0xd0,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 139  ror.b
  {38,0,0xd2,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 140  ror.b
  {39,0,0xc1,EMB_NONE,0,0,1,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 141  ror.d
  {39,0,0xd1,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 142  ror.d
  {39,0,0xd3,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 143  ror.d
  {40,0,0xc0,EMB_NONE,0,0,2,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 144  rcl
  {40,0,0xc1,EMB_NONE,0,0,2,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 145  rcl
  {40,0,0xd0,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 146  rcl
  {40,0,0xd1,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 147  rcl
  {40,0,0xd2,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 148  rcl
  {40,0,0xd3,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 149  rcl
  {41,0,0xc0,EMB_NONE,0,0,2,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 150  rcl.b
  {41,0,0xd0,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 151  rcl.b
  {41,0,0xd2,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 152  rcl.b
  {42,0,0xc1,EMB_NONE,0,0,2,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 153  rcl.d
  {42,0,0xd1,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 154  rcl.d
  {42,0,0xd3,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 155  rcl.d
  {43,0,0xc0,EMB_NONE,0,0,3,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 156  rcr
  {43,0,0xc1,EMB_NONE,0,0,3,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 157  rcr
  {43,0,0xd0,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 158  rcr
  {43,0,0xd1,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 159  rcr
  {43,0,0xd2,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 160  rcr
  {43,0,0xd3,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 161  rcr
  {44,0,0xc0,EMB_NONE,0,0,3,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 162  rcr.b
  {44,0,0xd0,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 163  rcr.b
  {44,0,0xd2,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 164  rcr.b
  {45,0,0xc1,EMB_NONE,0,0,3,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 165  rcr.d
  {45,0,0xd1,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 166  rcr.d
  {45,0,0xd3,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 167  rcr.d
  {46,0,0xc0,EMB_NONE,0,0,4,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 168  shl
  {46,0,0xc1,EMB_NONE,0,0,4,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 169  shl
  {46,0,0xd0,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 170  shl
  {46,0,0xd1,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 171  shl
  {46,0,0xd2,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 172  shl
  {46,0,0xd3,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 173  shl
  {47,0,0xc0,EMB_NONE,0,0,4,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 174  shl.b
  {47,0,0xd0,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 175  shl.b
  {47,0,0xd2,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 176  shl.b
  {48,0,0xc1,EMB_NONE,0,0,4,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 177  shl.d
  {48,0,0xd1,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 178  shl.d
  {48,0,0xd3,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 179  shl.d
  {49,0,0xc0,EMB_NONE,0,0,5,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 180  shr
  {49,0,0xc1,EMB_NONE,0,0,5,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 181  shr
  {49,0,0xd0,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 182  shr
  {49,0,0xd1,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 183  shr
  {49,0,0xd2,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 184  shr
  {49,0,0xd3,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 185  shr
  {50,0,0xc0,EMB_NONE,0,0,5,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 186  shr.b
  {50,0,0xd0,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 187  shr.b
  {50,0,0xd2,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 188  shr.b
  {51,0,0xc1,EMB_NONE,0,0,5,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 189  shr.d
  {51,0,0xd1,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 190  shr.d
  {51,0,0xd3,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 191  shr.d
  {52,0,0xc0,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 192  sal
  {52,0,0xc1,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 193  sal
  {52,0,0xd0,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 194  sal
  {52,0,0xd1,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 195  sal
  {52,0,0xd2,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 196  sal
  {52,0,0xd3,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 197  sal
  {53,0,0xc0,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 198  sal.b
  {53,0,0xd0,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 199  sal.b
  {53,0,0xd2,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 200  sal.b
  {54,0,0xc1,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 201  sal.d
  {54,0,0xd1,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 202  sal.d
  {54,0,0xd3,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 203  sal.d
  {55,0,0xc0,EMB_NONE,0,0,7,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 204  sar
  {55,0,0xc1,EMB_NONE,0,0,7,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 205  sar
  {55,0,0xd0,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 206  sar
  {55,0,0xd1,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 207  sar
  {55,0,0xd2,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 208  sar
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
  {60,0,0xf6,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 220  not
  {60,0,0xf7,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 221  not
  {61,0,0xf6,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 222  not.b
  {62,0,0xf7,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 223  not.d
  {63,0,0xf6,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 224  neg
  {63,0,0xf7,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 225  neg
  {64,0,0xf6,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 226  neg.b
  {65,0,0xf7,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 227  neg.d
  {66,0,0xf6,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 228  mul
  {66,0,0xf7,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 229  mul
  {67,0,0xf6,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 230  mul.b
  {68,0,0xf7,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 231  mul.d
  {69,0,0x69,EMB_NONE,0,0,255,1,4,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 232  imul
  {69,0,0x6b,EMB_NONE,0,0,255,1,8,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 233  imul
  {69,0,0xf6,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 234  imul
  {69,0,0xf7,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 235  imul
  {69,1,0xaf,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 236  imul
  {70,0,0xf6,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 237  imul.b
  {71,0,0xf7,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 238  imul.d
  {72,0,0xf6,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 239  div
  {72,0,0xf7,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 240  div
  {73,0,0xf6,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 241  div.b
  {74,0,0xf7,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 242  div.d
  {75,0,0xf6,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 243  idiv
  {75,0,0xf7,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 244  idiv
  {76,0,0xf6,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 245  idiv.b
  {77,0,0xf7,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 246  idiv.d
  {78,0,0x40,EMB_REG,0,3,255,2,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 247  inc
  {78,0,0xfe,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 248  inc
  {78,0,0xff,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 249  inc
  {79,0,0x48,EMB_REG,0,3,255,2,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 250  dec
  {79,0,0xfe,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 251  dec
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
  {95,0,0xa1,EMB_NONE,0,0,255,10,3,0,OPF_GREG,OPF_GREG,0,1,0,0},  // 282  mov
  {95,0,0xa2,EMB_NONE,0,0,255,10,3,0,OPF_GREG,OPF_GREG,0,1,0,0},  // 283  mov
  {95,0,0xa3,EMB_NONE,0,0,255,10,3,0,OPF_GREG,OPF_GREG,0,1,0,0},  // 284  mov
  {95,0,0xb0,EMB_REG,0,3,255,3,1,0,OPF_RGB,OPF_GREG,0,0,0,0},  // 285  mov
  {95,0,0xb8,EMB_REG,0,3,255,3,4,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 286  mov
  {95,0,0xc6,EMB_NONE,0,0,0,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 287  mov
  {95,0,0xc7,EMB_NONE,0,0,0,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 288  mov
  {96,0,0xa0,EMB_NONE,0,0,255,10,3,0,OPF_GREG,OPF_GREG,0,1,0,0},  // 289  mov al,
  {97,0,0xc6,EMB_NONE,0,0,7,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 290  xabort
  {98,0,0xc6,EMB_NONE,0,0,0,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 291  mov.b
  {99,0,0xc7,EMB_NONE,0,0,7,7,4,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 292  xbegin
  {100,0,0xc7,EMB_NONE,0,0,0,7,4,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 293  mov.d
  {101,1,0xb6,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_RGB,1,0,1,1},  // 294  movzx
  {101,1,0xb7,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,2,0,1,1},  // 295  movzx
  {102,1,0xbe,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_RGB,1,0,1,1},  // 296  movsx
  {102,1,0xbf,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,2,0,1,1},  // 297  movsx
  {103,0,0x86,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  // 298  xchg
  {103,0,0x87,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 299  xchg
  {103,0,0x90,EMB_REG,0,3,255,2,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 300  xchg
  {104,0,0xd7,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 301  xlat
  {105,1,0xc8,EMB_REG,0,3,255,2,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 302  bswap
  {106,0,0x58,EMB_REG,0,3,255,2,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 303  pop
  {106,0,0x8f,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 304  pop
  {107,0,0x8f,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 305  pop.d
  {108,0,0x06,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 306  push es
  {109,0,0x0e,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 307  push cs
  {110,0,0x16,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 308  push ss
  {111,0,0x1e,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 309  push ds
  {112,0,0x07,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 310  pop es
  {113,0,0x17,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 311  pop ss
  {114,0,0x1f,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 312  pop ds
  {115,1,0xa0,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 313  push fs
  {116,1,0xa8,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 314  push gs
  {117,1,0xa1,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 315  pop fs
  {118,1,0xa9,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 316  pop gs
  {119,0,0x60,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 317  pusha
  {120,0,0x61,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 318  popa
  {121,0,0x9c,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 319  pushf
  {122,0,0x9d,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 320  popf
  {123,0,0x70,EMB_CC,0,4,255,5,5,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 321  jcc
  {123,1,0x80,EMB_CC,0,4,255,5,6,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 322  jcc
  {124,0,0xe0,EMB_NONE,0,0,255,5,5,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 323  loopne
  {125,0,0xe1,EMB_NONE,0,0,255,5,5,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 324  loope
  {126,0,0xe2,EMB_NONE,0,0,255,5,5,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 325  loop
  {127,0,0xe3,EMB_NONE,0,0,255,5,5,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 326  jecxz
  {128,0,0xc2,EMB_NONE,0,0,255,4,2,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 327  ret
  {128,0,0xc3,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 328  ret
  {129,0,0xca,EMB_NONE,0,0,255,4,2,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 329  retf
  {129,0,0xcb,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 330  retf
  {130,0,0xcd,EMB_NONE,0,0,255,4,1,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 331  int
  {131,0,0xcc,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 332  int3
  {132,0,0xce,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 333  into
  {133,0,0xcf,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 334  iret
  {134,0,0xc9,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 335  leave
  {135,0,0xc8,EMB_NONE,0,0,255,6,9,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 336  enter
  {136,1,0x90,EMB_CC,0,4,255,9,0,0,OPF_GREG,OPF_RGB,1,0,1,1},  // 337  setcc
  {137,1,0x40,EMB_CC,0,4,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 338  cmovcc
  {138,1,0xa3,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 339  bt
  {138,1,0xba,EMB_NONE,0,0,4,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 340  bt
  {139,1,0xab,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 341  bts
  {139,1,0xba,EMB_NONE,0,0,5,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 342  bts
  {140,1,0xb3,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 343  btr
  {140,1,0xba,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 344  btr
  {141,1,0xba,EMB_NONE,0,0,7,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 345  btc
  {141,1,0xbb,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 346  btc
  {142,1,0xbc,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 347  bsf
  {143,1,0xbd,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 348  bsr
  {144,1,0xba,EMB_NONE,0,0,4,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 349  bt.d
  {145,1,0xba,EMB_NONE,0,0,5,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 350  bts.d
  {146,1,0xba,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 351  btr.d
  {147,1,0xba,EMB_NONE,0,0,7,7,1,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 352  btc.d
  {148,1,0xa4,EMB_NONE,0,0,255,1,1,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 353  shld
  {148,1,0xa5,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 354  shld
  {149,1,0xac,EMB_NONE,0,0,255,1,1,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 355  shrd
  {149,1,0xad,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 356  shrd
  {150,0,0xc4,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 357  les
  {151,0,0xc5,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 358  lds
  {152,1,0xb2,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 359  lss
  {153,1,0xb4,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 360  lfs
  {154,1,0xb5,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 361  lgs
  {155,0,0x62,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 362  bound
  {156,0,0x63,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 363  arpl
  {157,0,0xf5,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 364  cmc
  {158,0,0xf8,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 365  clc
  {159,0,0xf9,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 366  stc
  {160,0,0xfa,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 367  cli
  {161,0,0xfb,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 368  sti
  {162,0,0xfc,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 369  cld
  {163,0,0xfd,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 370  std
  {164,0,0x9e,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 371  sahf
  {165,0,0x9f,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 372  lahf
  {166,0,0x27,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 373  daa
  {167,0,0x2f,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 374  das
  {168,0,0x37,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 375  aaa
  {169,0,0x3f,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 376  aas
  {170,0,0x90,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 377  nop
  {170,1,0x18,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 378  nop
  {170,1,0x1e,EMB_NONE,0,0,255,9,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 379  nop
  {170,1,0x1f,EMB_NONE,0,0,255,9,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 380  nop
  {171,0,0xf4,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 381  hlt
  {172,0,0x9b,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 382  fwait
  {173,0,0xd4,EMB_NONE,0,0,255,4,1,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 383  aam
  {174,0,0xd5,EMB_NONE,0,0,255,4,1,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 384  aad
  {175,1,0xa2,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 385  cpuid
  {176,1,0x31,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 386  rdtsc
  {177,1,0x32,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 387  rdmsr
  {178,1,0x30,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 388  wrmsr
  {179,1,0x0b,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 389  ud2
  {180,0,0x99,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 390  cdq
  {181,0,0x99,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 391  cwd
  {182,0,0x98,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 392  cwde
  {183,0,0x98,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 393  cbw
  {184,0,0xa4,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 394  movsb
  {185,0,0xa6,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 395  cmpsb
  {186,0,0xaa,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 396  stosb
  {187,0,0xac,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 397  lodsb
  {188,0,0xae,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 398  scasb
  {189,0,0xa5,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 399  movsd
  {190,0,0xa5,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 400  movsw
  {191,0,0xa7,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 401  cmpsd
  {192,0,0xa7,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 402  cmpsw
  {193,0,0xab,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 403  stosd
  {194,0,0xab,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 404  stosw
  {195,0,0xad,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 405  lodsd
  {196,0,0xad,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 406  lodsw
  {197,0,0xaf,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 407  scasd
  {198,0,0xaf,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 408  scasw
  {199,0,0xe4,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  // 409  in al,
  {200,0,0xe5,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  // 410  in
  {200,0,0xed,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 411  in
  {201,0,0xe6,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  // 412  out
  {201,0,0xe7,EMB_NONE,0,0,255,10,1,0,OPF_GREG,OPF_GREG,0,1,0,0},  // 413  out
  {202,0,0xec,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 414  in al,dx
  {203,0,0xee,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 415  out dx,al
  {204,0,0xef,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 416  out dx,
  {205,0,0x6c,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 417  insb
  {206,0,0x6d,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 418  ins
  {207,0,0x6e,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 419  outsb
  {208,0,0x6f,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 420  outs
  {209,0,0xd8,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 421  fadd st(
  {209,0,0xdc,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 422  fadd st(
  {210,0,0xd8,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 423  fadd.d
  {211,0,0xd8,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 424  fmul st(
  {211,0,0xdc,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 425  fmul st(
  {212,0,0xd8,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 426  fmul.d
  {213,0,0xd8,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 427  fcom st(
  {213,0,0xdc,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 428  fcom st(
  {214,0,0xd8,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 429  fcom.d
  {215,0,0xd8,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 430  fcomp st(
  {215,0,0xdc,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 431  fcomp st(
  {215,0,0xde,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 432  fcomp st(
  {216,0,0xd8,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 433  fcomp.d
  {217,0,0xd8,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 434  fsub st(
  {217,0,0xdc,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 435  fsub st(
  {218,0,0xd8,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 436  fsub.d
  {219,0,0xd8,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 437  fsubr st(
  {219,0,0xdc,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 438  fsubr st(
  {220,0,0xd8,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 439  fsubr.d
  {221,0,0xd8,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 440  fdiv st(
  {221,0,0xdc,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 441  fdiv st(
  {222,0,0xd8,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 442  fdiv.d
  {223,0,0xd8,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 443  fdivr st(
  {223,0,0xdc,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 444  fdivr st(
  {224,0,0xd8,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 445  fdivr.d
  {225,0,0xd9,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 446  fld st(
  {225,0,0xdb,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 447  fld st(
  {225,0,0xdd,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 448  fld st(
  {226,0,0xd9,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 449  fld.d
  {226,0,0xdb,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 450  fld.d
  {227,0,0xd9,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 451  fxch st(
  {228,0,0xd9,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 452  fxch.d
  {229,0,0xd9,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 453  fst st(
  {229,0,0xdd,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 454  fst st(
  {230,0,0xd9,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 455  fst.d
  {231,0,0xd9,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 456  fstp st(
  {231,0,0xdb,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 457  fstp st(
  {231,0,0xdd,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 458  fstp st(
  {232,0,0xd9,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 459  fstp.d
  {232,0,0xdb,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 460  fstp.d
  {233,0,0xd9,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 461  fldenv st(
  {234,0,0xd9,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 462  fldenv.d
  {235,0,0xd9,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 463  fldcw st(
  {236,0,0xd9,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 464  fldcw.d
  {237,0,0xd9,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 465  fnstenv st(
  {238,0,0xd9,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 466  fnstenv.d
  {239,0,0xd9,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 467  fnstcw st(
  {240,0,0xd9,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 468  fnstcw.d
  {241,0,0xda,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 469  fcmovb st(
  {242,0,0xda,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 470  fcmovb.d
  {243,0,0xda,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 471  fcmove st(
  {244,0,0xda,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 472  fcmove.d
  {245,0,0xda,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 473  fcmovbe st(
  {246,0,0xda,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 474  fcmovbe.d
  {247,0,0xda,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 475  fcmovu st(
  {248,0,0xda,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 476  fcmovu.d
  {249,0,0xda,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 477  fx_a st(
  {249,0,0xda,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 478  fx_a st(
  {249,0,0xda,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 479  fx_a st(
  {250,0,0xda,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 480  fx_a.d
  {250,0,0xda,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 481  fx_a.d
  {250,0,0xda,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 482  fx_a.d
  {251,0,0xda,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 483  fucompp st(
  {252,0,0xda,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 484  fucompp.d
  {253,0,0xdb,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 485  fild st(
  {253,0,0xdf,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 486  fild st(
  {253,0,0xdf,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 487  fild st(
  {254,0,0xdb,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 488  fild.d
  {255,0,0xdb,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 489  fisttp st(
  {255,0,0xdd,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 490  fisttp st(
  {255,0,0xdf,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 491  fisttp st(
  {256,0,0xdb,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 492  fisttp.d
  {257,0,0xdb,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 493  fist st(
  {257,0,0xdf,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 494  fist st(
  {258,0,0xdb,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 495  fist.d
  {259,0,0xdb,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 496  fistp st(
  {259,0,0xdf,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 497  fistp st(
  {259,0,0xdf,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 498  fistp st(
  {260,0,0xdb,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 499  fistp.d
  {261,0,0xdb,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 500  fx_b st(
  {261,0,0xdb,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 501  fx_b st(
  {262,0,0xdb,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 502  fx_b.d
  {262,0,0xdb,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 503  fx_b.d
  {263,0,0xdc,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 504  fadd.q
  {264,0,0xdc,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 505  fmul.q
  {265,0,0xdc,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 506  fcom.q
  {266,0,0xdc,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 507  fcomp.q
  {267,0,0xdc,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 508  fsub.q
  {268,0,0xdc,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 509  fsubr.q
  {269,0,0xdc,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 510  fdiv.q
  {270,0,0xdc,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 511  fdivr.q
  {271,0,0xdd,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 512  fld.q
  {272,0,0xdd,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 513  fisttp.q
  {273,0,0xdd,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 514  fst.q
  {274,0,0xdd,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 515  fstp.q
  {275,0,0xdd,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 516  frstor st(
  {276,0,0xdd,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 517  frstor.q
  {277,0,0xdd,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 518  fx_d st(
  {278,0,0xdd,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 519  fx_d.q
  {279,0,0xdd,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 520  fnsave st(
  {280,0,0xdd,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 521  fnsave.q
  {281,0,0xdd,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 522  fnstsw st(
  {282,0,0xdd,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 523  fnstsw.q
  {283,0,0xde,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 524  faddp st(
  {284,0,0xde,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 525  faddp.w
  {285,0,0xde,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 526  fmulp st(
  {286,0,0xde,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 527  fmulp.w
  {287,0,0xde,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 528  fcomp.w
  {288,0,0xde,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 529  fcompp st(
  {289,0,0xde,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 530  fcompp.w
  {290,0,0xde,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 531  fsubrp st(
  {291,0,0xde,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 532  fsubrp.w
  {292,0,0xde,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 533  fsubp st(
  {293,0,0xde,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 534  fsubp.w
  {294,0,0xde,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 535  fdivrp st(
  {295,0,0xde,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 536  fdivrp.w
  {296,0,0xde,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 537  fdivp st(
  {297,0,0xde,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 538  fdivp.w
  {298,0,0xdf,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 539  fild.w
  {298,0,0xdf,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 540  fild.w
  {299,0,0xdf,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 541  fisttp.w
  {300,0,0xdf,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 542  fist.w
  {301,0,0xdf,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 543  fistp.w
  {301,0,0xdf,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 544  fistp.w
  {302,0,0xdf,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 545  fbld st(
  {303,0,0xdf,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 546  fbld.w
  {304,0,0xdf,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 547  fbstp st(
  {305,0,0xdf,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 548  fbstp.w
  {306,1,0x6f,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 549  movq
  {306,1,0xd6,EMB_NONE,0,0,255,1,0,1,OPF_XMM,OPF_XMM,0,0,1,1},  // 550  movq
  {307,1,0xfc,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 551  paddb
  {308,1,0xfd,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 552  paddw
  {309,1,0xfe,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 553  paddd
  {310,1,0xf8,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 554  psubb
  {311,1,0xdb,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 555  pand
  {312,1,0xeb,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 556  por
  {313,1,0xef,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 557  pxor
  {314,1,0x74,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 558  pcmpeqb
  {315,1,0x63,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 559  packsswb
  {316,1,0x60,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 560  punpcklbw
  {317,1,0x71,EMB_NONE,0,0,2,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 561  psrlw
  {317,1,0xd1,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 562  psrlw
  {318,1,0x72,EMB_NONE,0,0,2,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 563  psrld
  {318,1,0xd2,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 564  psrld
  {319,1,0x73,EMB_NONE,0,0,2,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 565  psrlq
  {319,1,0xd3,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 566  psrlq
  {320,1,0x71,EMB_NONE,0,0,4,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 567  psraw
  {320,1,0xe1,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 568  psraw
  {321,1,0x72,EMB_NONE,0,0,4,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 569  psrad
  {321,1,0xe2,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 570  psrad
  {322,1,0x71,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 571  psllw
  {322,1,0xf1,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 572  psllw
  {323,1,0x72,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 573  pslld
  {323,1,0xf2,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 574  pslld
  {324,1,0x73,EMB_NONE,0,0,6,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 575  psllq
  {324,1,0xf3,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 576  psllq
  {325,1,0x28,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 577  movaps
  {325,1,0x29,EMB_NONE,0,0,255,1,0,1,OPF_XMM,OPF_XMM,0,0,1,1},  // 578  movaps
  {326,1,0x10,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 579  movups
  {326,1,0x11,EMB_NONE,0,0,255,1,0,1,OPF_XMM,OPF_XMM,0,0,1,1},  // 580  movups
  {327,1,0x58,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 581  addps
  {328,1,0x59,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 582  mulps
  {329,1,0x5c,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 583  subps
  {330,1,0x2f,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 584  comiss
  {331,1,0x57,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 585  xorps
  {332,1,0x54,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 586  andps
  {333,1,0x2e,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 587  ucomiss
  {334,1,0x2e,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 588  ucomisd
  {335,1,0x6e,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_GREG,0,0,1,1},  // 589  movd
  {335,1,0x7e,EMB_NONE,0,0,255,1,0,1,OPF_SSE_OS,OPF_GREG,0,0,1,1},  // 590  movd
  {336,1,0x2a,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_GREG,0,0,1,1},  // 591  cvtpi2ps
  {337,1,0x2c,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_XMM,0,0,1,1},  // 592  cvttps2pi
  {338,1,0x2d,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_XMM,0,0,1,1},  // 593  cvtps2pi
  {339,1,0x70,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 594  pshufw
  {340,1,0x77,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 595  emms
  {341,1,0xae,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 596  ptwrite
  {342,1,0xae,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 597  lfence
  {343,1,0xae,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 598  mfence
  {344,1,0xae,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 599  sfence
  {345,1,0xae,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 600  clflush.b
  {346,1,0x18,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 601  prefetchnta.b
  {347,1,0x18,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 602  prefetcht0.b
  {348,1,0x18,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 603  prefetcht1.b
  {349,1,0x18,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 604  prefetcht2.b
  {350,1,0x18,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 605  nop18
  {350,1,0x18,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 606  nop18
  {350,1,0x18,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 607  nop18
  {350,1,0x18,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 608  nop18
  {350,1,0x18,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 609  nop18
  {350,1,0x18,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 610  nop18
  {350,1,0x18,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 611  nop18
  {350,1,0x18,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 612  nop18
  {350,1,0x18,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 613  nop18
  {351,1,0x18,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 614  prefetchit1.b
  {352,1,0x18,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 615  prefetchit0.b
  {353,1,0xc0,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  // 616  xadd
  {353,1,0xc1,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 617  xadd
  {354,1,0xb0,EMB_NONE,0,0,255,1,0,1,OPF_RGB,OPF_RGB,1,0,1,1},  // 618  cmpxchg
  {354,1,0xb1,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 619  cmpxchg
  {355,1,0xc7,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 620  cmpxchg8b
  {356,1,0xc7,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 621  xrstors
  {357,1,0xc7,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 622  xsavec
  {358,1,0xc7,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 623  xsaves
  {359,1,0xc7,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 624  vmptrld
  {360,1,0xc7,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 625  vmptrst
  {361,1,0xc7,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 626  rdrand
  {362,1,0xc7,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 627  rdseed
  {363,1,0x37,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 628  getsec
  {364,1,0x02,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 629  lar
  {365,1,0x03,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 630  lsl
  {366,1,0x06,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 631  clts
  {367,1,0x08,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 632  invd
  {368,1,0x09,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 633  wbinvd
  {369,1,0x00,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 634  sldt
  {370,1,0x00,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 635  sldt.w
  {371,1,0x00,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 636  str
  {372,1,0x00,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 637  str.w
  {373,1,0x00,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 638  lldt
  {374,1,0x00,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 639  lldt.w
  {375,1,0x00,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 640  ltr
  {376,1,0x00,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 641  ltr.w
  {377,1,0x00,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 642  verr
  {378,1,0x00,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 643  verr.w
  {379,1,0x00,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 644  verw
  {380,1,0x00,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 645  verw.w
  {381,1,0x01,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 646  sgdt
  {382,1,0x01,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 647  sidt
  {383,1,0x01,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 648  lgdt
  {384,1,0x01,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 649  lidt
  {385,1,0x01,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 650  smsw
  {385,1,0x01,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 651  smsw
  {386,1,0x01,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 652  lmsw
  {386,1,0x01,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 653  lmsw
  {387,1,0x01,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 654  invlpg
  {388,1,0x01,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 655  rstorssp
  {388,1,0x01,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 656  rstorssp
  {389,1,0xec,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 657  paddsb
  {390,1,0xed,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 658  paddsw
  {391,1,0xdc,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 659  paddusb
  {392,1,0xdd,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 660  paddusw
  {393,1,0xf9,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 661  psubw
  {394,1,0xfa,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 662  psubd
  {395,1,0xe8,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 663  psubsb
  {396,1,0xe9,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 664  psubsw
  {397,1,0xd8,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 665  psubusb
  {398,1,0xd9,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 666  psubusw
  {399,1,0xd5,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 667  pmullw
  {400,1,0xe5,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 668  pmulhw
  {401,1,0xf5,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 669  pmaddwd
  {402,1,0xdf,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 670  pandn
  {403,1,0x64,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 671  pcmpgtb
  {404,1,0x65,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 672  pcmpgtw
  {405,1,0x66,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 673  pcmpgtd
  {406,1,0x67,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 674  packuswb
  {407,1,0x6b,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 675  packssdw
  {408,1,0x68,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 676  punpckhbw
  {409,1,0x69,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 677  punpckhwd
  {410,1,0x6a,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 678  punpckhdq
  {411,1,0x61,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 679  punpcklwd
  {412,1,0x62,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 680  punpckldq
  {413,1,0xf4,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 681  pmuludq
  {414,1,0x75,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 682  pcmpeqw
  {415,1,0x76,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 683  pcmpeqd
  {416,1,0xd4,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 684  paddq
  {417,1,0xfb,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 685  psubq
  {418,1,0xe0,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 686  pavgb
  {419,1,0xe3,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 687  pavgw
  {420,1,0xda,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 688  pminub
  {421,1,0xde,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 689  pmaxub
  {422,1,0xea,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 690  pminsw
  {423,1,0xee,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 691  pmaxsw
  {424,1,0xe4,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 692  pmulhuw
  {425,1,0xf6,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 693  psadbw
  {426,1,0xd7,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_SSE_OS,0,0,1,0},  // 694  pmovmskb
  {427,1,0xc4,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_GREG,0,0,1,1},  // 695  pinsrw
  {428,1,0xc5,EMB_NONE,0,0,255,1,1,0,OPF_GREG,OPF_SSE_OS,0,0,1,0},  // 696  pextrw
  {428,3,0x15,EMB_NONE,0,0,255,1,1,1,OPF_SSE_OS,OPF_GREG,0,0,1,1},  // 697  pextrw
  {429,1,0x73,EMB_NONE,0,0,3,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 698  psrldq
  {430,1,0x73,EMB_NONE,0,0,7,7,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 699  pslldq
  {431,1,0x51,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 700  sqrtps
  {432,1,0x52,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 701  rsqrtps
  {433,1,0x53,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 702  rcpps
  {434,1,0x55,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 703  andnps
  {435,1,0x56,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 704  orps
  {436,1,0x5d,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 705  minps
  {437,1,0x5e,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 706  divps
  {438,1,0x5f,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 707  maxps
  {439,1,0x14,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 708  unpcklps
  {440,1,0x15,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 709  unpckhps
  {441,1,0x12,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 710  movlps
  {441,1,0x13,EMB_NONE,0,0,255,1,0,1,OPF_XMM,OPF_GREG,0,0,0,1},  // 711  movlps
  {442,1,0x16,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 712  movhps
  {442,1,0x17,EMB_NONE,0,0,255,1,0,1,OPF_XMM,OPF_GREG,0,0,0,1},  // 713  movhps
  {443,1,0x50,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_XMM,0,0,1,0},  // 714  movmskps
  {444,1,0xc2,EMB_NONE,0,0,255,1,1,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 715  cmpps
  {445,1,0xc6,EMB_NONE,0,0,255,1,1,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 716  shufps
  {446,1,0xd0,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 717  addsubps
  {447,1,0x7c,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 718  haddps
  {448,1,0x7d,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 719  hsubps
  {449,1,0x5a,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 720  cvtps2pd
  {450,1,0x5b,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 721  cvtdq2ps
  {451,1,0xe6,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 722  cvtpd2dq
  {452,1,0x6c,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 723  punpcklqdq
  {453,1,0x6d,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_XMM,0,0,1,1},  // 724  punpckhqdq
  {454,1,0xf0,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_GREG,0,0,0,1},  // 725  lddqu
  {455,1,0x2b,EMB_NONE,0,0,255,1,0,1,OPF_XMM,OPF_GREG,0,0,0,1},  // 726  movntps
  {456,1,0xe7,EMB_NONE,0,0,255,1,0,1,OPF_SSE_OS,OPF_GREG,0,0,0,1},  // 727  movntq
  {457,1,0xc3,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,0,1},  // 728  movnti
  {458,1,0xf7,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,0},  // 729  maskmovq
  {459,1,0xae,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 730  fxsave
  {460,1,0xae,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 731  fxrstor
  {461,1,0xae,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 732  ldmxcsr
  {462,1,0xae,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 733  stmxcsr
  {463,1,0xae,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 734  xsave
  {464,1,0xae,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 735  xrstor
  {465,1,0xae,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 736  xsaveopt
  {466,2,0x00,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 737  pshufb
  {467,2,0x01,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 738  phaddw
  {468,2,0x02,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 739  phaddd
  {469,2,0x03,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 740  phaddsw
  {470,2,0x04,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 741  pmaddubsw
  {471,2,0x05,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 742  phsubw
  {472,2,0x06,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 743  phsubd
  {473,2,0x07,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 744  phsubsw
  {474,2,0x08,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 745  psignb
  {475,2,0x09,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 746  psignw
  {476,2,0x0a,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 747  psignd
  {477,2,0x0b,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 748  pmulhrsw
  {478,2,0x1c,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 749  pabsb
  {479,2,0x1d,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 750  pabsw
  {480,2,0x1e,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 751  pabsd
  {481,2,0x10,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 752  pblendvb
  {482,2,0x14,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 753  blendvps
  {483,2,0x15,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 754  blendvpd
  {484,2,0x17,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 755  ptest
  {485,2,0x20,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 756  pmovsxbw
  {486,2,0x21,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 757  pmovsxbd
  {487,2,0x22,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 758  pmovsxbq
  {488,2,0x23,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 759  pmovsxwd
  {489,2,0x24,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 760  pmovsxwq
  {490,2,0x25,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 761  pmovsxdq
  {491,2,0x28,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 762  pmuldq
  {492,2,0x29,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 763  pcmpeqq
  {493,2,0x2b,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 764  packusdw
  {494,2,0x30,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 765  pmovzxbw
  {495,2,0x31,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 766  pmovzxbd
  {496,2,0x32,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 767  pmovzxbq
  {497,2,0x33,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 768  pmovzxwd
  {498,2,0x34,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 769  pmovzxwq
  {499,2,0x35,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 770  pmovzxdq
  {500,2,0x37,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 771  pcmpgtq
  {501,2,0x38,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 772  pminsb
  {502,2,0x39,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 773  pminsd
  {503,2,0x3a,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 774  pminuw
  {504,2,0x3b,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 775  pminud
  {505,2,0x3c,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 776  pmaxsb
  {506,2,0x3d,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 777  pmaxsd
  {507,2,0x3e,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 778  pmaxuw
  {508,2,0x3f,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 779  pmaxud
  {509,2,0x40,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 780  pmulld
  {510,2,0x41,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 781  phminposuw
  {511,2,0x2a,EMB_NONE,0,0,255,1,0,0,OPF_XMM,OPF_GREG,0,0,0,1},  // 782  movntdqa
  {512,2,0xdb,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 783  aesimc
  {513,2,0xdc,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 784  aesenc
  {514,2,0xdd,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 785  aesenclast
  {515,2,0xde,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 786  aesdec
  {516,2,0xdf,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 787  aesdeclast
  {517,2,0xc8,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 788  sha1nexte
  {518,2,0xc9,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 789  sha1msg1
  {519,2,0xca,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 790  sha1msg2
  {520,2,0xcb,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 791  sha256rnds2
  {521,2,0xcc,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 792  sha256msg1
  {522,2,0xcd,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 793  sha256msg2
  {523,3,0x08,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 794  roundps
  {524,3,0x09,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 795  roundpd
  {525,3,0x0a,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 796  roundss
  {526,3,0x0b,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 797  roundsd
  {527,3,0x0c,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 798  blendps
  {528,3,0x0d,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 799  blendpd
  {529,3,0x0e,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 800  pblendw
  {530,3,0x0f,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 801  palignr
  {531,3,0x21,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 802  insertps
  {532,3,0x40,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 803  dpps
  {533,3,0x41,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 804  dppd
  {534,3,0x42,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 805  mpsadbw
  {535,3,0x44,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 806  pclmulqdq
  {536,3,0x60,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 807  pcmpestrm
  {537,3,0x61,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 808  pcmpestri
  {538,3,0x62,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 809  pcmpistrm
  {539,3,0x63,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 810  pcmpistri
  {540,3,0xcc,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 811  sha1rnds4
  {541,3,0xdf,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 812  aeskeygenassist
  {542,3,0x14,EMB_NONE,0,0,255,1,1,1,OPF_SSE_OS,OPF_GREG,0,0,1,1},  // 813  pextrb
  {543,3,0x16,EMB_NONE,0,0,255,1,1,1,OPF_SSE_OS,OPF_GREG,0,0,1,1},  // 814  pextrd
  {544,3,0x17,EMB_NONE,0,0,255,1,1,1,OPF_SSE_OS,OPF_GREG,0,0,1,1},  // 815  extractps
  {545,3,0x20,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_GREG,0,0,1,1},  // 816  pinsrb
  {546,3,0x22,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_GREG,0,0,1,1},  // 817  pinsrd
  {547,1,0x34,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 818  sysenter
  {548,1,0x35,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 819  sysexit
  {549,1,0xaa,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 820  rsm
  {550,1,0x33,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 821  rdpmc
  {551,1,0x01,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 822  monitor
  {552,1,0x01,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 823  rdtscp
  {553,1,0xb8,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 824  popcnt
  {554,2,0xf0,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_RGB,0,0,1,1},  // 825  crc32
  {554,2,0xf1,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 826  crc32
  {555,1,0x0d,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 827  prefetch
  {556,1,0x0d,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 828  prefetchw
  {557,1,0x0d,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 829  prefetchwt1
  {558,1,0x0d,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 830  prefetch3
  {559,1,0x0d,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 831  prefetch4
  {560,1,0x0d,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 832  prefetch5
  {561,1,0x0d,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 833  prefetch6
  {562,1,0x0d,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 834  prefetch7
  {563,1,0x0d,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 835  nop0d
  {563,1,0x0d,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 836  nop0d
  {563,1,0x0d,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 837  nop0d
  {563,1,0x0d,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 838  nop0d
  {563,1,0x0d,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 839  nop0d
  {563,1,0x0d,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 840  nop0d
  {563,1,0x0d,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 841  nop0d
  {563,1,0x0d,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 842  nop0d
  {564,1,0x01,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 843  vmcall
  {565,1,0x01,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 844  xgetbv
  {566,1,0x01,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 845  vmrun
  {567,1,0xb9,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 846  ud1
  {568,0,0xf1,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 847  int1
  {569,1,0x0f,EMB_NONE,0,0,255,1,1,0,OPF_MM,OPF_MM,0,0,1,1},  // 848  _3dnow
  {570,0,0xd6,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 849  salc
  {571,1,0x05,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 850  syscall
  {572,1,0x07,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 851  sysret
  {573,1,0x0e,EMB_NONE,0,0,255,0,0,0,OPF_GREG,OPF_GREG,0,0,0,0},  // 852  femms
  {574,1,0x7f,EMB_NONE,0,0,255,1,0,1,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 853  movdqa
  {575,1,0x78,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 854  vmread
  {576,1,0x79,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 855  vmwrite
  {577,1,0xff,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 856  ud0
  {578,1,0x19,EMB_NONE,0,0,255,9,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 857  nop19
  {579,1,0x1a,EMB_NONE,0,0,255,9,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 858  nop1a
  {580,1,0x1b,EMB_NONE,0,0,255,9,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 859  nop1b
  {581,1,0x1d,EMB_NONE,0,0,255,9,0,1,OPF_GREG,OPF_GREG,0,0,1,1},  // 860  nop1d
  {582,1,0x1c,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 861  cldemote
  {583,1,0x1c,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 862  nop1c
  {583,1,0x1c,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 863  nop1c
  {583,1,0x1c,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 864  nop1c
  {583,1,0x1c,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 865  nop1c
  {583,1,0x1c,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 866  nop1c
  {583,1,0x1c,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 867  nop1c
  {583,1,0x1c,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 868  nop1c
  {583,1,0x1c,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 869  nop1c
  {583,1,0x1c,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 870  nop1c
  {583,1,0x1c,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 871  nop1c
  {583,1,0x1c,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 872  nop1c
  {583,1,0x1c,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 873  nop1c
  {583,1,0x1c,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 874  nop1c
  {583,1,0x1c,EMB_NONE,0,0,6,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 875  nop1c
  {583,1,0x1c,EMB_NONE,0,0,7,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 876  nop1c
  {584,1,0xa6,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 877  montmul
  {585,1,0xa6,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 878  xsha1
  {586,1,0xa6,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 879  xsha256
  {587,1,0xa7,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 880  xstore
  {588,1,0xa7,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 881  xcryptecb
  {589,1,0xa7,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 882  xcryptcbc
  {590,1,0xa7,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 883  xcryptctr
  {591,1,0xa7,EMB_NONE,0,0,4,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 884  xcryptcfb
  {592,1,0xa7,EMB_NONE,0,0,5,7,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 885  xcryptofb
  {593,2,0x80,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 886  invept
  {594,2,0x81,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 887  invvpid
  {595,2,0x82,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 888  invpcid
  {596,2,0xcf,EMB_NONE,0,0,255,1,0,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 889  gf2p8mulb
  {597,2,0xf6,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,1},  // 890  adcx
  {598,2,0xf5,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,0,1},  // 891  wrssd
  {599,2,0xf8,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 892  movdir64b
  {600,2,0xf9,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,0,1},  // 893  movdiri
  {601,2,0xfc,EMB_NONE,0,0,255,1,0,1,OPF_GREG,OPF_GREG,0,0,0,1},  // 894  aadd
  {602,2,0xfa,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 895  encodekey128
  {603,2,0xfb,EMB_NONE,0,0,255,1,0,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 896  encodekey256
  {604,2,0xd8,EMB_NONE,0,0,0,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 897  aesencwide128kl
  {605,2,0xd8,EMB_NONE,0,0,1,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 898  aesdecwide128kl
  {606,2,0xd8,EMB_NONE,0,0,2,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 899  aesencwide256kl
  {607,2,0xd8,EMB_NONE,0,0,3,7,0,0,OPF_GREG,OPF_GREG,0,0,0,1},  // 900  aesdecwide256kl
  {608,3,0xce,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 901  gf2p8affineqb
  {609,3,0xcf,EMB_NONE,0,0,255,1,1,0,OPF_SSE_OS,OPF_SSE_OS,0,0,1,1},  // 902  gf2p8affineinvqb
  {610,3,0xf0,EMB_NONE,0,0,255,9,1,0,OPF_GREG,OPF_GREG,0,0,1,0},  // 903  hreset
};
static const size_t enc_cand_count = sizeof(enc_cand)/sizeof(enc_cand[0]);

// per-mnemonic bucket: candidates for mnem M are enc_cand[start..start+count).
static const uint16_t enc_bucket_start[] = {
    0, 9, 10, 19, 20, 29, 30, 39, 40, 49, 50, 59, 60, 69, 70, 79, 80, 82, 84, 86, 88, 90, 92, 94, 96, 98, 100, 102, 104, 106, 108, 110, 112, 119, 120, 126, 129, 132, 138, 141, 144, 150, 153, 156, 162, 165, 168, 174, 177, 180, 186, 189, 192, 198, 201, 204, 210, 213, 216, 218, 220, 222, 223, 224, 226, 227, 228, 230, 231, 232, 237, 238, 239, 241, 242, 243, 245, 246, 247, 250, 253, 254, 255, 256, 257, 259, 260, 262, 263, 266, 267, 269, 270, 274, 275, 276, 289, 290, 291, 292, 293, 294, 296, 298, 301, 302, 303, 305, 306, 307, 308, 309, 310, 311, 312, 313, 314, 315, 316, 317, 318, 319, 320, 321, 323, 324, 325, 326, 327, 329, 331, 332, 333, 334, 335, 336, 337, 338, 339, 341, 343, 345, 347, 348, 349, 350, 351, 352, 353, 355, 357, 358, 359, 360, 361, 362, 363, 364, 365, 366, 367, 368, 369, 370, 371, 372, 373, 374, 375, 376, 377, 381, 382, 383, 384, 385, 386, 387, 388, 389, 390, 391, 392, 393, 394, 395, 396, 397, 398, 399, 400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 412, 414, 415, 416, 417, 418, 419, 420, 421, 423, 424, 426, 427, 429, 430, 433, 434, 436, 437, 439, 440, 442, 443, 445, 446, 449, 451, 452, 453, 455, 456, 459, 461, 462, 463, 464, 465, 466, 467, 468, 469, 470, 471, 472, 473, 474, 475, 476, 477, 480, 483, 484, 485, 488, 489, 492, 493, 495, 496, 499, 500, 502, 504, 505, 506, 507, 508, 509, 510, 511, 512, 513, 514, 515, 516, 517, 518, 519, 520, 521, 522, 523, 524, 525, 526, 527, 528, 529, 530, 531, 532, 533, 534, 535, 536, 537, 538, 539, 541, 542, 543, 545, 546, 547, 548, 549, 551, 552, 553, 554, 555, 556, 557, 558, 559, 560, 561, 563, 565, 567, 569, 571, 573, 575, 577, 579, 581, 582, 583, 584, 585, 586, 587, 588, 589, 591, 592, 593, 594, 595, 596, 597, 598, 599, 600, 601, 602, 603, 604, 605, 614, 615, 616, 618, 620, 621, 622, 623, 624, 625, 626, 627, 628, 629, 630, 631, 632, 633, 634, 635, 636, 637, 638, 639, 640, 641, 642, 643, 644, 645, 646, 647, 648, 649, 650, 652, 654, 655, 657, 658, 659, 660, 661, 662, 663, 664, 665, 666, 667, 668, 669, 670, 671, 672, 673, 674, 675, 676, 677, 678, 679, 680, 681, 682, 683, 684, 685, 686, 687, 688, 689, 690, 691, 692, 693, 694, 695, 696, 698, 699, 700, 701, 702, 703, 704, 705, 706, 707, 708, 709, 710, 712, 714, 715, 716, 717, 718, 719, 720, 721, 722, 723, 724, 725, 726, 727, 728, 729, 730, 731, 732, 733, 734, 735, 736, 737, 738, 739, 740, 741, 742, 743, 744, 745, 746, 747, 748, 749, 750, 751, 752, 753, 754, 755, 756, 757, 758, 759, 760, 761, 762, 763, 764, 765, 766, 767, 768, 769, 770, 771, 772, 773, 774, 775, 776, 777, 778, 779, 780, 781, 782, 783, 784, 785, 786, 787, 788, 789, 790, 791, 792, 793, 794, 795, 796, 797, 798, 799, 800, 801, 802, 803, 804, 805, 806, 807, 808, 809, 810, 811, 812, 813, 814, 815, 816, 817, 818, 819, 820, 821, 822, 823, 824, 825, 827, 828, 829, 830, 831, 832, 833, 834, 835, 843, 844, 845, 846, 847, 848, 849, 850, 851, 852, 853, 854, 855, 856, 857, 858, 859, 860, 861, 862, 877, 878, 879, 880, 881, 882, 883, 884, 885, 886, 887, 888, 889, 890, 891, 892, 893, 894, 895, 896, 897, 898, 899, 900, 901, 902, 903
};
static const uint16_t enc_bucket_count[] = {
    9, 1, 9, 1, 9, 1, 9, 1, 9, 1, 9, 1, 9, 1, 9, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 7, 1, 6, 3, 3, 6, 3, 3, 6, 3, 3, 6, 3, 3, 6, 3, 3, 6, 3, 3, 6, 3, 3, 6, 3, 3, 2, 2, 2, 1, 1, 2, 1, 1, 2, 1, 1, 5, 1, 1, 2, 1, 1, 2, 1, 1, 3, 3, 1, 1, 1, 1, 2, 1, 2, 1, 3, 1, 2, 1, 4, 1, 1, 13, 1, 1, 1, 1, 1, 2, 2, 3, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 2, 1, 3, 1, 2, 1, 2, 1, 2, 1, 2, 1, 3, 2, 1, 1, 2, 1, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 1, 1, 3, 1, 3, 1, 2, 1, 3, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 9, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 15, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};
static const size_t enc_nmnem = 611;

#endif // X86_TABLES_ENC_H
