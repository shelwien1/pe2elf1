#ifndef X86INSN_HPP
#define X86INSN_HPP

#include <stdint.h>
#include "x86_tables.h"   /* VAR_COUNT, ARCH_MAXLEN (prefix bookkeeping sizes) */

/* operand type : a register file, or one of the hoisted singletons.
 * The bulk payload (memory descriptor, immediate) lives ONCE in the
 * instruction - there is at most one memory operand and one immediate -
 * so an operand is only which-kind + which-register. */
enum {
  T_NONE = 0,
  /* register files - index is the register number */
  T_GPR,
  /* (e)ax..(e)di, 16/32 per opsize */ T_GPR8, /* al..bh           */
  T_GPRdq, /* r32/r64, never 16: SSE-op GPR where 66 is a mandatory prefix, not an
            * operand-size override (pmovmskb, pextrb/w/d, movd/movq r/m) */
  T_GPRq,  /* r64 always (fixed 64-bit operand in long mode): vmread/vmwrite */
  T_GPRw,  /* r16 always: the r/m16 source of movzx/movsx r32|r64, r/m16 */
  T_SREG,
  /* es,cs,ss,ds,fs,gs */ T_CREG, /* cr0..            */
  T_DREG,
  /* dr0..dr7 */ T_TREG, /* tr3..tr7 (legacy)*/
  T_MMX,
  T_XMM,
  T_YMM,
  T_ZMM,
  T_KREG, /* k0..k7 as a positional operand (kmov, vpcmp dst) */
  T_MASK, /* k0..k7 as the EVEX writemask {k}  -> EVEX.aaa    */
  T_BND,
  /* bnd0..bnd3 */ T_ST, /* st0..st7         */
  /* hoisted singletons - index unused; payload is in the instruction */
  T_MEM,  /* the one memory operand  -> disp + mem_base/index/scale/seg */
  T_IMM,  /* the one full immediate  -> imm                            */
  T_IMM2, /* enter's 2nd immediate   -> disp (free; enter has no mem)   */
  T_REL,  /* branch displacement     -> imm                            */
  T_PTR,  /* far ptr16:32   off -> imm,  sel -> disp                    */
  T_TMM   /* AMX tile register tmm0..tmm7 (VEX-encoded, not sized by L) */
};

typedef struct {
  uint16_t type : 5;  /* T_*                                          */
  uint16_t index : 5; /* register number 0..31 (GP/xmm 0..15, zmm 0..31) */
} x86op_t;            /* 2 bytes */

typedef struct {
  uint16_t mnem;   /* enum of mnemonics (ADD, MOV, VADDPS, ...)    */
  uint8_t n_ops;   /* 0..5 (the writemask counts as one)           */
  uint8_t rep : 2; /* rep/repne, when semantic (rep movs)          */
  uint8_t lock : 1;
  uint8_t zero : 1; /* {z}                                          */
  uint8_t bcst : 1; /* broadcast {1toN}                             */
  uint8_t rc : 3;   /* rounding mode / SAE (0 = none)               */

  /* the single memory operand (target of an op[] of type T_MEM) */
  int32_t disp;            /* mem displacement; OR imm2 / far selector     */
  uint32_t mem_base : 7;   /* 0..31 (APX r16-31 via REX2) or GREG_NONE/GREG_RIP */
  uint32_t mem_index : 7;  /* 0..31 or GREG_NONE                       */
  uint32_t mem_scale : 2;
  uint32_t mem_seg : 3;
  uint32_t opsize : 2;  /* operand size: 0=default(32) 1=16 (66) 2=64 (REX.W) */
  uint32_t addr : 1;    /* address size: 0=native (64) 1=32 (67); x86-32: 0=32 1=16 */
  uint32_t has_pfx : 1; /* preceding record is a raw-prefix sidecar */
  uint32_t rip : 1;     /* RIP-relative memory operand: [rip+disp32]  */
  uint32_t rex : 1;     /* a REX prefix byte was present (W via opsize, R/X/B re-derived) */
  uint32_t rex2 : 1;    /* APX REX2 (D5) prefix present; its payload (incl. map bit M0)
                         * is replayed from pfx[], so the encoder drops the implied 0F */
  uint32_t moffs : 1;   /* mov accumulator<->[moffs] (A0-A3): no ModR/M, the operand is an
                         * absolute address held in `imm` (64-bit, or 32 under a 67); the
                         * opcode byte is in vex_op, width follows `addr` */
  uint32_t apx : 1;     /* APX EVEX-promoted legacy (62, map 4): GPR ops, r0-31, NDD/NF.
                         * vex==3 (the EVEX prefix bytes are replayed); apx flags the
                         * semantic GPR lowering so the renderer/encoder treat it as legacy */
  uint32_t nf : 1;      /* APX no-flags ({nf}): the status-flag write is suppressed */

  /* encoding witness: the residual that makes  bytes <-> x86insn_t  a byte-exact
   * bijection. The semantic fields say WHAT the instruction is; these say WHICH of
   * the redundant encodings produced it. All-zero == canonical, so a synthesized or
   * operand-edited insn (witness stale) just re-encodes canonically. The decoder
   * must populate these (parse_addr/select currently discard them); the encoder
   * replays them, and a plain assembler ignores them. See FULL_ENCODE.md sec 11. */
  uint8_t disp_w : 2;  /* actual disp width 0/1/2 = none/disp8/disp16-32 (ENCODE_ADDR delta) */
  uint8_t sib    : 1;  /* a redundant SIB byte was present (not the esp/index-forced one)     */
  uint8_t sscale : 2;  /* dead SIB scale bits when index == NONE                              */
  uint8_t enc    : 3;  /* opcode-choice deviation: 0=canonical, else dir/long-imm/acc/moffs   */
  uint8_t reg_w  : 4;  /* dead ModR/M reg field for single-r/m forms (setcc), REX.R-extended  */

  /* the single full immediate (target of T_IMM / T_REL / T_PTR). 64-bit so the
   * REX.W mov r64,imm64 (movabs) immediate fits; narrower immediates occupy the
   * low bytes (the encoder writes only as many bytes as the operand size needs). */
  int64_t imm;

  uint8_t cc;          /* condition code 0..15 for jcc/setcc/cmovcc; 0xFF = none */

  /* VEX witness: a VEX instruction is  [C4|C5] <vex bytes> <opcode> <modrm...>.
   * The prefix bytes are captured raw and replayed (they hold vvvv/L/W/pp/RXB,
   * none of which change the tail byte layout); only the map + opcode are decoded,
   * to route and to size the modrm/immediate tail. vex == 0 for legacy insns. */
  uint8_t vex;         /* 0 = legacy, 1 = C5 (2B VEX), 2 = C4 (3B VEX), 3 = 62 (EVEX) */
  uint8_t vex1;        /* VEX byte 1 / EVEX P0 (R.X.B.R'.00.mm map bits)            */
  uint8_t vex2;        /* VEX byte 2 / EVEX P1 (W.vvvv.1.pp)                        */
  uint8_t vex3;        /* EVEX P2 (z.L'L.b.V'.aaa); 0 for VEX                       */
  uint8_t vex_op;      /* the opcode byte following the VEX/EVEX prefix            */
  uint8_t vex_modrm;   /* raw ModR/M byte of a VEX/EVEX insn (0 if none). Replayed
                        * verbatim by the encoder so the byte-exact round-trip is
                        * independent of how vex_decode reorders op[] for display. */

  /* prefix bookkeeping (filled by parse_prefixes) */
  uint8_t n_pfx;                 /* number of prefix bytes (prefix string len) */
  uint8_t pfx[ARCH_MAXLEN];      /* the raw prefix bytes, in order             */
  uint8_t pfx_off[VAR_COUNT];    /* last offset per prefix group; 0xFF = none  */

  x86op_t op[5]; /* operands incl. the writemask (T_MASK)        */
} x86insn_t;

#endif // X86INSN_HPP
