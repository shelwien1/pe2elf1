#!/usr/bin/env python3
# gen_evex.py -- emit the x86-64 EVEX submatch (0x62) for a representative set of
# AVX-512 instructions.  EVEX prefix (after the 0x62 lead):
#   P0 = R-bar X-bar B-bar R-bar' 0 0 mm   (mm: 01=0F 10=0F38 11=0F3A)
#   P1 = W vvvv 1 pp                       (pp: 00=none 01=66 10=F3 11=F2)
#   P2 = z L'L b V-bar' aaa
# field letters: h=R-bar k=X-bar b=B-bar e=R-bar' ; v=vvvv ; z l(=L'L) c(=b-bit)
# u=V-bar' a=aaa.  Registers via inverted-layout tables (ereg/evvv); memory bridges
# REX.B/X into k/b (asm._vexfix).  Displacement is the raw disp8 byte (corpus.p
# convention -- disp8*N scaling is not applied, but round-trips byte-exact).
v = []
def E(s): v.append("  " + s)

RG = "ereg[32*$l+16*$e+8*$h+$g]"      # reg (R'R)
RM = "ereg[32*$l+16*$k+8*$b+$r]"      # rm-register (XB)
VV = "evvv[32*$l+16*$u+$v]"           # vvvv (V')
ZG = "ereg[64+16*$e+8*$h+$g]"         # reg forced zmm (embedded rounding)
ZV = "evvv[64+16*$u+$v]"
ZM = "ereg[64+16*$k+8*$b+$r]"
KZ = "kzdec[$z*8+$a]"
ACT = "{$rexb=1-$b;$rexx=1-$k}"
P0 = lambda mm: "h k b e 00 %s" % mm
P1 = lambda w, pp: "%s vvvv 1 %s" % (w, pp)
P1F = lambda w, pp: "%s 1111 1 %s" % (w, pp)   # vvvv unused (2-operand)

def fp3(mn, op, w, pp, bcst, er=True, mm="01"):
  # 3-operand packed FP: reg-reg, reg-reg{er}, reg-mem, reg-mem{1toN}
  E('%s %s z ll 0 u aaa 0x%02x 11 ggg rrr => wit("evex") "%s " %s %s "," %s "," %s ;' % (P0(mm), P1(w, pp), op, mn, RG, KZ, VV, RM))
  if er:
    E('%s %s z ll 1 u aaa 0x%02x 11 ggg rrr => wit("evex") "%s " %s %s "," %s "," %s rcdec[$l] ;' % (P0(mm), P1(w, pp), op, mn, ZG, KZ, ZV, ZM))
  E('%s %s z ll 0 u aaa 0x%02x @addr %s => wit("evex") "%s " %s %s "," %s "," $addr ;' % (P0(mm), P1(w, pp), op, ACT, mn, RG, KZ, VV))
  E('%s %s z ll 1 u aaa 0x%02x @addr %s => wit("evex") "%s " %s %s "," %s "," $addr %s[$l] ;' % (P0(mm), P1(w, pp), op, ACT, mn, RG, KZ, VV, bcst))

def scal3(mn, op, w, pp, mm="01"):
  # 3-operand scalar: reg-reg, reg-reg{er}, reg-mem (no broadcast)
  E('%s %s z ll 0 u aaa 0x%02x 11 ggg rrr => wit("evex") "%s " %s %s "," %s "," %s ;' % (P0(mm), P1(w, pp), op, mn, RG, KZ, VV, RM))
  E('%s %s z ll 1 u aaa 0x%02x 11 ggg rrr => wit("evex") "%s " %s %s "," %s "," %s rcdec[$l] ;' % (P0(mm), P1(w, pp), op, mn, ZG, KZ, ZV, ZM))
  E('%s %s z ll 0 u aaa 0x%02x @addr %s => wit("evex") "%s " %s %s "," %s "," $addr ;' % (P0(mm), P1(w, pp), op, ACT, mn, RG, KZ, VV))

def int3(mn, op, w, pp, bcst=None, mm="01"):
  # 3-operand integer (66.0F / 0F38): reg-reg, reg-mem, [reg-mem{1toN}]
  E('%s %s z ll 0 u aaa 0x%02x 11 ggg rrr => wit("evex") "%s " %s %s "," %s "," %s ;' % (P0(mm), P1(w, pp), op, mn, RG, KZ, VV, RM))
  E('%s %s z ll 0 u aaa 0x%02x @addr %s => wit("evex") "%s " %s %s "," %s "," $addr ;' % (P0(mm), P1(w, pp), op, ACT, mn, RG, KZ, VV))
  if bcst:
    E('%s %s z ll 1 u aaa 0x%02x @addr %s => wit("evex") "%s " %s %s "," %s "," $addr %s[$l] ;' % (P0(mm), P1(w, pp), op, ACT, mn, RG, KZ, VV, bcst))

def mov2(mn, op, w, pp, store=False, mm="01"):
  # 2-operand move (vvvv=1111, V'=1).  store=True: rm is the destination.
  if store:
    E('%s %s z ll 0 1 aaa 0x%02x 11 ggg rrr => wit("evex") wit("alt") "%s " %s %s "," %s ;' % (P0(mm), P1F(w, pp), op, mn, RM, KZ, RG))
    E('%s %s z ll 0 1 aaa 0x%02x @addr %s => wit("evex") "%s " $addr %s "," %s ;' % (P0(mm), P1F(w, pp), op, ACT, mn, KZ, RG))
  else:
    E('%s %s z ll 0 1 aaa 0x%02x 11 ggg rrr => wit("evex") "%s " %s %s "," %s ;' % (P0(mm), P1F(w, pp), op, mn, RG, KZ, RM))
    E('%s %s z ll 0 1 aaa 0x%02x @addr %s => wit("evex") "%s " %s %s "," $addr ;' % (P0(mm), P1F(w, pp), op, ACT, mn, RG, KZ))

v.append("submatch evex {")
# --- FP packed arithmetic (W0.0F=ps / W1.66.0F=pd) with embedded rounding ---
for mn, op in [("vadd", 0x58), ("vmul", 0x59), ("vsub", 0x5c), ("vdiv", 0x5e)]:
  fp3(mn + "ps", op, "0", "00", "bcst32", er=True)
  fp3(mn + "pd", op, "1", "01", "bcst64", er=True)
# min/max: packed, no {er} here
for mn, op in [("vmin", 0x5d), ("vmax", 0x5f)]:
  fp3(mn + "ps", op, "0", "00", "bcst32", er=False)
  fp3(mn + "pd", op, "1", "01", "bcst64", er=False)
# FP logical (ps/pd), no {er}
for mn, op in [("vand", 0x54), ("vandn", 0x55), ("vor", 0x56), ("vxor", 0x57)]:
  fp3(mn + "ps", op, "0", "00", "bcst32", er=False)
  fp3(mn + "pd", op, "1", "01", "bcst64", er=False)
# --- FP scalar (W0.F3=ss / W1.F2=sd) with embedded rounding ---
for mn, op in [("vadd", 0x58), ("vmul", 0x59), ("vsub", 0x5c), ("vdiv", 0x5e)]:
  scal3(mn + "ss", op, "0", "10")
  scal3(mn + "sd", op, "1", "11")
# --- aligned/unaligned moves ---
mov2("vmovaps", 0x28, "0", "00"); mov2("vmovaps", 0x29, "0", "00", store=True)
mov2("vmovapd", 0x28, "1", "01"); mov2("vmovapd", 0x29, "1", "01", store=True)
mov2("vmovups", 0x10, "0", "00"); mov2("vmovups", 0x11, "0", "00", store=True)
mov2("vmovupd", 0x10, "1", "01"); mov2("vmovupd", 0x11, "1", "01", store=True)
mov2("vmovdqa32", 0x6f, "0", "01"); mov2("vmovdqa32", 0x7f, "0", "01", store=True)
mov2("vmovdqa64", 0x6f, "1", "01"); mov2("vmovdqa64", 0x7f, "1", "01", store=True)
mov2("vmovdqu32", 0x6f, "0", "10"); mov2("vmovdqu32", 0x7f, "0", "10", store=True)
mov2("vmovdqu64", 0x6f, "1", "10"); mov2("vmovdqu64", 0x7f, "1", "10", store=True)
mov2("vmovdqu8", 0x6f, "0", "11"); mov2("vmovdqu8", 0x7f, "0", "11", store=True)
mov2("vmovdqu16", 0x6f, "1", "11"); mov2("vmovdqu16", 0x7f, "1", "11", store=True)
# --- packed integer (66.0F) : d/q broadcast, b/w none ---
int3("vpaddb", 0xfc, "0", "01"); int3("vpaddw", 0xfd, "0", "01")
int3("vpaddd", 0xfe, "0", "01", "bcst32"); int3("vpaddq", 0xd4, "1", "01", "bcst64")
int3("vpsubb", 0xf8, "0", "01"); int3("vpsubw", 0xf9, "0", "01")
int3("vpsubd", 0xfa, "0", "01", "bcst32"); int3("vpsubq", 0xfb, "1", "01", "bcst64")
int3("vpandd", 0xdb, "0", "01", "bcst32"); int3("vpandq", 0xdb, "1", "01", "bcst64")
int3("vpandnd", 0xdf, "0", "01", "bcst32"); int3("vpandnq", 0xdf, "1", "01", "bcst64")
int3("vpord", 0xeb, "0", "01", "bcst32"); int3("vporq", 0xeb, "1", "01", "bcst64")
int3("vpxord", 0xef, "0", "01", "bcst32"); int3("vpxorq", 0xef, "1", "01", "bcst64")
int3("vpmulld", 0x40, "0", "01", "bcst32", mm="10")   # 0F38
int3("vpmullw", 0xd5, "0", "01")
# --- FMA (0F38) : ps/pd, {er}, broadcast ---
for mn, op in [("vfmadd213", 0xa8), ("vfmadd231", 0xb8), ("vfmadd132", 0x98),
               ("vfmsub213", 0xaa), ("vfnmadd213", 0xac)]:
  fp3(mn + "ps", op, "0", "01", "bcst32", er=True, mm="10")
  fp3(mn + "pd", op, "1", "01", "bcst64", er=True, mm="10")
v.append("}")
print("\n".join(v))
