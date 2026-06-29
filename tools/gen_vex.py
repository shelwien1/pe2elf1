#!/usr/bin/env python3
# gen_vex.py -- emit the x86-64 VEX submatches (C4 = `vex`, C5 = `vex2`) for a
# representative set of AVX/AVX2 instructions.  Conventions (see corpus64.p):
#   byte1(C4) = R-bar X-bar B-bar mmmmm   byte2 = W vvvv L pp
#   byte1(C5) = R-bar vvvv L pp           (W=0, X=B=0 implied, map 0F)
#   reg  = vreg[16*L + 8*R-bar + reg]     rm(reg) = vreg[16*L + 8*B-bar + rm]
#   vvvv = vvv[16*L + vvvv]               element suffix = velt[pp]
# Memory rm: C4 sets $rexb/$rexx (= 1 - the byte1 bit) for the shared addr
# matcher; the assembler bridges REX.B/X back (asm._vexfix).  C5 memory keeps
# $rexb/$rexx at their 0 default (no base/index extension encodable in C5).
v = []
def E(s): v.append("  " + s)

DST = "vreg[16*$y+8*$h+$g]"
SRC1 = "vvv[16*$y+$v]"
RM_C4 = "vreg[16*$y+8*$b+$r]"
RM_C5 = "vreg[16*$y+8+$r]"
ACT = "{$rexb=1-$b;$rexx=1-$k}"

def velt3(op, mn):
  # 3-operand, element by pp: dest, vvvv, rm
  E('h k b 00001 0 vvvv y pp 0x%02x 11 ggg rrr => "%s" velt[$p] " " %s "," %s "," %s ;' % (op, mn, DST, SRC1, RM_C4))
  E('h k b 00001 0 vvvv y pp 0x%02x @addr %s => "%s" velt[$p] " " %s "," %s "," $addr ;' % (op, ACT, mn, DST, SRC1))

def velt3_c5(op, mn):
  E('h vvvv y pp 0x%02x 11 ggg rrr => "%s" velt[$p] " " %s "," %s "," %s ;' % (op, mn, DST, SRC1, RM_C5))
  E('h vvvv y pp 0x%02x @addr => "%s" velt[$p] " " %s "," %s "," $addr ;' % (op, mn, DST, SRC1))

def int3(op, mn, pp):  # 3-operand integer, fixed pp (binary string e.g. "01")
  E('h k b 00001 0 vvvv y %s 0x%02x 11 ggg rrr => "%s " %s "," %s "," %s ;' % (pp, op, mn, DST, SRC1, RM_C4))
  E('h k b 00001 0 vvvv y %s 0x%02x @addr %s => "%s " %s "," %s "," $addr ;' % (pp, op, ACT, mn, DST, SRC1))

def int3_c5(op, mn, pp):
  E('h vvvv y %s 0x%02x 11 ggg rrr => "%s " %s "," %s "," %s ;' % (pp, op, mn, DST, SRC1, RM_C5))
  E('h vvvv y %s 0x%02x @addr => "%s " %s "," %s "," $addr ;' % (pp, op, mn, DST, SRC1))

def mov2(op, tbl):  # 2-operand move, vvvv=1111, mnemonic from tbl[pp]
  E('h k b 00001 0 1111 y pp 0x%02x 11 ggg rrr => %s[$p] " " %s "," %s ;' % (op, tbl, DST, RM_C4))
  E('h k b 00001 0 1111 y pp 0x%02x @addr %s => %s[$p] " " %s "," $addr ;' % (op, ACT, tbl, DST))

def mov2_c5(op, tbl):
  E('h 1111 y pp 0x%02x 11 ggg rrr => %s[$p] " " %s "," %s ;' % (op, tbl, DST, RM_C5))
  E('h 1111 y pp 0x%02x @addr => %s[$p] " " %s "," $addr ;' % (op, tbl, DST))

def store2(op, tbl):  # 2-operand store form (reg is source): rm, reg
  E('h k b 00001 0 1111 y pp 0x%02x @addr %s => %s[$p] " " $addr "," %s ;' % (op, ACT, tbl, DST))

def store2_c5(op, tbl):
  E('h 1111 y pp 0x%02x @addr => %s[$p] " " $addr "," %s ;' % (op, tbl, DST))

def imm3(op, mn_expr):  # 3-operand + imm8 (vcmp/vshuf): dest, vvvv, rm, imm8
  E('h k b 00001 0 vvvv y pp 0x%02x 11 ggg rrr @imm8 => %s " " %s "," %s "," %s "," hex($imm8) ;' % (op, mn_expr, DST, SRC1, RM_C4))
  E('h k b 00001 0 vvvv y pp 0x%02x @addr %s @imm8 => %s " " %s "," %s "," $addr "," hex($imm8) ;' % (op, ACT, mn_expr, DST, SRC1))

def imm3_c5(op, mn_expr):
  E('h vvvv y pp 0x%02x 11 ggg rrr @imm8 => %s " " %s "," %s "," %s "," hex($imm8) ;' % (op, mn_expr, DST, SRC1, RM_C5))
  E('h vvvv y pp 0x%02x @addr @imm8 => %s " " %s "," %s "," $addr "," hex($imm8) ;' % (op, mn_expr, DST, SRC1))

ARITH = [(0x54, "vand"), (0x55, "vandn"), (0x56, "vor"), (0x57, "vxor"),
         (0x58, "vadd"), (0x59, "vmul"), (0x5c, "vsub"), (0x5d, "vmin"),
         (0x5e, "vdiv"), (0x5f, "vmax")]
# integer ops: (opcode, mnemonic) -- all VEX.66.0F, pp = 01
INT = [(0xfc, "vpaddb"), (0xfd, "vpaddw"), (0xfe, "vpaddd"), (0xd4, "vpaddq"),
       (0xf8, "vpsubb"), (0xf9, "vpsubw"), (0xfa, "vpsubd"), (0xfb, "vpsubq"),
       (0xdb, "vpand"), (0xdf, "vpandn"), (0xeb, "vpor"), (0xef, "vpxor"),
       (0x74, "vpcmpeqb"), (0x75, "vpcmpeqw"), (0x76, "vpcmpeqd"),
       (0x64, "vpcmpgtb"), (0x65, "vpcmpgtw"), (0x66, "vpcmpgtd"),
       (0xd5, "vpmullw"), (0xe5, "vpmulhw"), (0xf5, "vpmaddwd"),
       (0x60, "vpunpcklbw"), (0x61, "vpunpcklwd"), (0x62, "vpunpckldq"),
       (0x6c, "vpunpcklqdq"), (0x68, "vpunpckhbw"), (0x6d, "vpunpckhqdq"),
       (0xdc, "vpaddusb"), (0xde, "vpmaxub"), (0xda, "vpminub"),
       (0xe0, "vpavgb"), (0xe3, "vpavgw"), (0x63, "vpacksswb"), (0x67, "vpackuswb")]

def extras_c5():
  # vsqrt: ps/pd 2-operand (pp00/01), ss/sd 3-operand (pp10/11)
  E('h 1111 y 00 0x51 11 ggg rrr => "vsqrtps " %s "," %s ;' % (DST, RM_C5))
  E('h 1111 y 00 0x51 @addr => "vsqrtps " %s "," $addr ;' % DST)
  E('h 1111 y 01 0x51 11 ggg rrr => "vsqrtpd " %s "," %s ;' % (DST, RM_C5))
  E('h 1111 y 01 0x51 @addr => "vsqrtpd " %s "," $addr ;' % DST)
  E('h vvvv y 10 0x51 11 ggg rrr => "vsqrtss " %s "," %s "," %s ;' % (DST, SRC1, RM_C5))
  E('h vvvv y 10 0x51 @addr => "vsqrtss " %s "," %s "," $addr ;' % (DST, SRC1))
  E('h vvvv y 11 0x51 11 ggg rrr => "vsqrtsd " %s "," %s "," %s ;' % (DST, SRC1, RM_C5))
  E('h vvvv y 11 0x51 @addr => "vsqrtsd " %s "," %s "," $addr ;' % (DST, SRC1))
  # scalar int<->fp converts, W0 (32-bit GP)
  E('h vvvv y 11 0x2a 11 ggg rrr => "vcvtsi2sd " %s "," %s "," greg[$r] ;' % (DST, SRC1))
  E('h vvvv y 10 0x2a 11 ggg rrr => "vcvtsi2ss " %s "," %s "," greg[$r] ;' % (DST, SRC1))
  E('h 1111 y 11 0x2d 11 ggg rrr => "vcvtsd2si " greg[$g] "," %s ;' % RM_C5)
  E('h 1111 y 11 0x2d @addr => "vcvtsd2si " greg[$g] "," $addr ;')
  E('h 1111 y 10 0x2d 11 ggg rrr => "vcvtss2si " greg[$g] "," %s ;' % RM_C5)
  E('h 1111 y 10 0x2d @addr => "vcvtss2si " greg[$g] "," $addr ;')
  E('h 1111 y 11 0x2c 11 ggg rrr => "vcvttsd2si " greg[$g] "," %s ;' % RM_C5)
  E('h 1111 y 11 0x2c @addr => "vcvttsd2si " greg[$g] "," $addr ;')
  E('h 1111 y 10 0x2c 11 ggg rrr => "vcvttss2si " greg[$g] "," %s ;' % RM_C5)
  E('h 1111 y 10 0x2c @addr => "vcvttss2si " greg[$g] "," $addr ;')
  # same-width packed converts (5B): dq2ps/ps2dq/ttps2dq
  E('h 1111 y 00 0x5b 11 ggg rrr => "vcvtdq2ps " %s "," %s ;' % (DST, RM_C5))
  E('h 1111 y 00 0x5b @addr => "vcvtdq2ps " %s "," $addr ;' % DST)
  E('h 1111 y 01 0x5b 11 ggg rrr => "vcvtps2dq " %s "," %s ;' % (DST, RM_C5))
  E('h 1111 y 01 0x5b @addr => "vcvtps2dq " %s "," $addr ;' % DST)
  E('h 1111 y 10 0x5b 11 ggg rrr => "vcvttps2dq " %s "," %s ;' % (DST, RM_C5))
  E('h 1111 y 10 0x5b @addr => "vcvttps2dq " %s "," $addr ;' % DST)
  # width-changing converts: one operand fixed xmm, the other sized by L.
  # dest-by-L (src always xmm): @addr ok (dest reveals L)
  E('h 1111 y 00 0x5a 11 ggg rrr => "vcvtps2pd " %s "," vreg[8+$r] ;' % DST)
  E('h 1111 y 00 0x5a @addr => "vcvtps2pd " %s "," $addr ;' % DST)
  E('h 1111 y 10 0xe6 11 ggg rrr => "vcvtdq2pd " %s "," vreg[8+$r] ;' % DST)
  E('h 1111 y 10 0xe6 @addr => "vcvtdq2pd " %s "," $addr ;' % DST)
  # src-by-L (dest always xmm): reg-only (a mem src would be L-ambiguous)
  E('h 1111 y 01 0x5a 11 ggg rrr => "vcvtpd2ps " vreg[8*$h+$g] "," %s ;' % RM_C5)
  E('h 1111 y 11 0xe6 11 ggg rrr => "vcvtpd2dq " vreg[8*$h+$g] "," %s ;' % RM_C5)
  E('h 1111 y 01 0xe6 11 ggg rrr => "vcvttpd2dq " vreg[8*$h+$g] "," %s ;' % RM_C5)
  # 0x10/0x11 by pp: vmovups/upd (2-op, pp00/01) and vmovss/sd (pp10/11)
  E('h 1111 y 00 0x10 11 ggg rrr => "vmovups " %s "," %s ;' % (DST, RM_C5))
  E('h 1111 y 00 0x10 @addr => "vmovups " %s "," $addr ;' % DST)
  E('h 1111 y 00 0x11 @addr => "vmovups " $addr "," %s ;' % DST)
  E('h 1111 y 01 0x10 11 ggg rrr => "vmovupd " %s "," %s ;' % (DST, RM_C5))
  E('h 1111 y 01 0x10 @addr => "vmovupd " %s "," $addr ;' % DST)
  E('h 1111 y 01 0x11 @addr => "vmovupd " $addr "," %s ;' % DST)
  # scalar moves: reg-reg 3-operand, mem load/store 2-operand
  E('h vvvv y 10 0x10 11 ggg rrr => "vmovss " %s "," %s "," %s ;' % (DST, SRC1, RM_C5))
  E('h 1111 y 10 0x10 @addr => "vmovss " %s "," $addr ;' % DST)
  E('h 1111 y 10 0x11 @addr => "vmovss " $addr "," %s ;' % DST)
  E('h vvvv y 11 0x10 11 ggg rrr => "vmovsd " %s "," %s "," %s ;' % (DST, SRC1, RM_C5))
  E('h 1111 y 11 0x10 @addr => "vmovsd " %s "," $addr ;' % DST)
  E('h 1111 y 11 0x11 @addr => "vmovsd " $addr "," %s ;' % DST)
  # vmovd (66.0F 6E/7E, W0): xmm<->r32
  E('h 1111 y 01 0x6e 11 ggg rrr => "vmovd " %s "," greg[$r] ;' % DST)
  E('h 1111 y 01 0x6e @addr => "vmovd " %s "," $addr ;' % DST)
  E('h 1111 y 01 0x7e 11 ggg rrr => "vmovd " greg[$r] "," %s ;' % DST)
  E('h 1111 y 01 0x7e @addr => "vmovd " $addr "," %s ;' % DST)
  # vpshufd (imm), vpshufb (3-op)
  E('h 1111 y 01 0x70 11 ggg rrr @imm8 => "vpshufd " %s "," %s "," hex($imm8) ;' % (DST, RM_C5))
  E('h 1111 y 01 0x70 @addr @imm8 => "vpshufd " %s "," $addr "," hex($imm8) ;' % DST)
  # shift-by-imm8 (/digit): dest=vvvv, src=rm
  for op, dig, mn in [(0x72, 6, "vpslld"), (0x72, 2, "vpsrld"), (0x72, 4, "vpsrad"),
                      (0x73, 6, "vpsllq"), (0x73, 2, "vpsrlq"),
                      (0x71, 6, "vpsllw"), (0x71, 2, "vpsrlw"), (0x71, 4, "vpsraw")]:
    d = format(dig, "03b")
    E('h vvvv y 01 0x%02x 11 %s rrr @imm8 => "%s " %s "," %s "," hex($imm8) ;' % (op, d, mn, SRC1, RM_C5))

def extras_c4():
  # W1 (64-bit GP) scalar converts + vmovq  (C4, 0F map; h/k/b capture reg/rm ext)
  E('h k b 00001 1 vvvv y 11 0x2a 11 ggg rrr => "vcvtsi2sd " %s "," %s "," greg[32+$r] ;' % (DST, SRC1))
  E('h k b 00001 1 vvvv y 10 0x2a 11 ggg rrr => "vcvtsi2ss " %s "," %s "," greg[32+$r] ;' % (DST, SRC1))
  E('h k b 00001 1 1111 y 11 0x2d 11 ggg rrr => "vcvtsd2si " greg[32+$g] "," %s ;' % RM_C4)
  E('h k b 00001 1 1111 y 11 0x2d @addr %s => "vcvtsd2si " greg[32+$g] "," $addr ;' % ACT)
  E('h k b 00001 1 1111 y 10 0x2d 11 ggg rrr => "vcvtss2si " greg[32+$g] "," %s ;' % RM_C4)
  E('h k b 00001 1 1111 y 10 0x2d @addr %s => "vcvtss2si " greg[32+$g] "," $addr ;' % ACT)
  E('h k b 00001 1 1111 y 11 0x2c 11 ggg rrr => "vcvttsd2si " greg[32+$g] "," %s ;' % RM_C4)
  E('h k b 00001 1 1111 y 10 0x2c 11 ggg rrr => "vcvttss2si " greg[32+$g] "," %s ;' % RM_C4)
  E('h k b 00001 1 1111 y 01 0x6e 11 ggg rrr => "vmovq " %s "," greg[32+$r] ;' % DST)
  E('h k b 00001 1 1111 y 01 0x7e 11 ggg rrr => "vmovq " greg[32+$g] "," %s ;' % RM_C4)
  # vbroadcastss/sd (0F38, 2-op mem)
  E('h k b 00010 0 1111 y 01 0x18 @addr %s => "vbroadcastss " %s "," $addr ;' % (ACT, DST))
  E('h k b 00010 0 1111 y 01 0x19 @addr %s => "vbroadcastsd " %s "," $addr ;' % (ACT, DST))
  # vpshufb (0F38, 3-op)
  E('h k b 00010 0 vvvv y 01 0x00 11 ggg rrr => "vpshufb " %s "," %s "," %s ;' % (DST, SRC1, RM_C4))
  E('h k b 00010 0 vvvv y 01 0x00 @addr %s => "vpshufb " %s "," %s "," $addr ;' % (ACT, DST, SRC1))
  # vmovups/upd C4 (extended regs / extended mem base)
  E('h k b 00001 0 1111 y 00 0x10 11 ggg rrr => "vmovups " %s "," %s ;' % (DST, RM_C4))
  E('h k b 00001 0 1111 y 00 0x10 @addr %s => "vmovups " %s "," $addr ;' % (ACT, DST))
  E('h k b 00001 0 1111 y 00 0x11 @addr %s => "vmovups " $addr "," %s ;' % (ACT, DST))
  E('h k b 00001 0 1111 y 01 0x10 11 ggg rrr => "vmovupd " %s "," %s ;' % (DST, RM_C4))
  E('h k b 00001 0 1111 y 01 0x10 @addr %s => "vmovupd " %s "," $addr ;' % (ACT, DST))
  E('h k b 00001 0 1111 y 01 0x11 @addr %s => "vmovupd " $addr "," %s ;' % (ACT, DST))

def kops_c4():
  # AVX-512 mask ops needing C4: kmovq (W1.F2.0F) and kshift (W1.66.0F3A).
  E('1 1 1 00001 1 1111 0 11 0x92 11 ggg rrr => "kmovq " kreg[$g] "," greg[32+$r] ;')
  E('1 1 1 00001 1 1111 0 11 0x93 11 ggg rrr => "kmovq " greg[32+$g] "," kreg[$r] ;')
  E('1 1 1 00001 1 1111 0 11 0x90 11 ggg rrr => "kmovq " kreg[$g] "," kreg[$r] ;')
  E('1 1 1 00011 1 1111 0 01 0x32 11 ggg rrr @imm8 => "kshiftlw " kreg[$g] "," kreg[$r] "," hex($imm8) ;')
  E('1 1 1 00011 1 1111 0 01 0x30 11 ggg rrr @imm8 => "kshiftrw " kreg[$g] "," kreg[$r] "," hex($imm8) ;')

def kops_c5():
  # C5-encodable mask ops (W0).  pp: 00=w 01=b 11=d.
  E('1 1111 0 pp 0x90 11 ggg rrr => kmov_t[$p] " " kreg[$g] "," kreg[$r] ;')
  E('1 1111 0 pp 0x90 @addr => kmov_t[$p] " " kreg[$g] "," $addr ;')
  E('1 1111 0 pp 0x91 @addr => kmov_t[$p] " " $addr "," kreg[$g] ;')
  E('1 1111 0 pp 0x92 11 ggg rrr => kmov_t[$p] " " kreg[$g] "," greg[$r] ;')
  E('1 1111 0 pp 0x93 11 ggg rrr => kmov_t[$p] " " greg[$g] "," kreg[$r] ;')
  for op, mn in [(0x41, "kand"), (0x42, "kandn"), (0x45, "kor"), (0x47, "kxor"), (0x46, "kxnor"), (0x4a, "kadd")]:
    E('1 vvvv 1 00 0x%02x 11 ggg rrr => "%sw " kreg[$g] "," kreg[15-$v] "," kreg[$r] ;' % (op, mn))
    E('1 vvvv 1 01 0x%02x 11 ggg rrr => "%sb " kreg[$g] "," kreg[15-$v] "," kreg[$r] ;' % (op, mn))
  E('1 vvvv 1 01 0x4b 11 ggg rrr => "kunpckbw " kreg[$g] "," kreg[15-$v] "," kreg[$r] ;')
  E('1 1111 0 00 0x44 11 ggg rrr => "knotw " kreg[$g] "," kreg[$r] ;')
  E('1 1111 0 01 0x44 11 ggg rrr => "knotb " kreg[$g] "," kreg[$r] ;')
  E('1 1111 0 00 0x98 11 ggg rrr => "kortestw " kreg[$g] "," kreg[$r] ;')
  E('1 1111 0 00 0x99 11 ggg rrr => "ktestw " kreg[$g] "," kreg[$r] ;')

# ---------------- C4 (vex) ----------------
v.append("submatch vex {")
for op, mn in ARITH:
  velt3(op, mn)
mov2(0x28, "vmova"); store2(0x29, "vmova")
mov2(0x6f, "vmdq"); store2(0x7f, "vmdq")
for op, mn in INT:
  int3(op, mn, "01")
imm3(0xc2, '"vcmp" velt[$p]')
imm3(0xc6, '"vshuf" velt[$p]')
extras_c4()
kops_c4()
v.append("}")

# ---------------- C5 (vex2) ----------------
v.append("submatch vex2 {")
for op, mn in ARITH:
  velt3_c5(op, mn)
mov2_c5(0x28, "vmova"); store2_c5(0x29, "vmova")
mov2_c5(0x6f, "vmdq"); store2_c5(0x7f, "vmdq")
for op, mn in INT:
  int3_c5(op, mn, "01")
imm3_c5(0xc2, '"vcmp" velt[$p]')
imm3_c5(0xc6, '"vshuf" velt[$p]')
extras_c5()
kops_c5()
# vzeroupper / vzeroall : C5 only, no operands
E('1 1111 0 00 0x77 => "vzeroupper" ;')
E('1 1111 1 00 0x77 => "vzeroall" ;')
v.append("}")

print("\n".join(v))
