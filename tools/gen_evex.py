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

def kcmp(mn, op, w, pp, mm, imm=False):
  # EVEX compare -> mask register: dest=k (reg field, no extension so R-bar/
  # R-bar'=1), optional {k} mask via aaa, src1=vvvv, src2=rm[, imm8].  z=0.
  ip = " @imm8" if imm else ""
  it = ' "," hex($imm8)' if imm else ""
  E('1 k b 1 00 %s %s 0 ll 0 u aaa 0x%02x 11 ggg rrr%s => wit("evex") "%s " kreg[$g] kdec[$a] "," %s "," %s%s ;' % (mm, P1(w, pp), op, ip, mn, VV, RM, it))
  E('1 k b 1 00 %s %s 0 ll 0 u aaa 0x%02x @addr %s%s => wit("evex") "%s " kreg[$g] kdec[$a] "," %s "," $addr%s ;' % (mm, P1(w, pp), op, ACT, ip, mn, VV, it))

# --- additional operand shapes -------------------------------------------------
XG = "ereg[16*$e+8*$h+$g]"     # scalar reg (xmm, LIG)
XV = "evvv[16*$u+$v]"          # scalar vvvv (xmm)
XM = "ereg[16*$k+8*$b+$r]"     # scalar/broadcast-source rm (xmm)
GRm = "greg[32*$w+8*$rexb+$r]"  # GP in modrm.rm (W picks 32/64; $rexb=1-B-bar)
GRg = "greg[32*$w+8*$rexr+$g]"  # GP in modrm.reg (W picks 32/64; $rexr=1-R-bar)

def cvt2(mn, op, w, pp, bcst=None, rnd=None, mm="01"):
  # 2-operand same-width convert / unary (vpabs): reg<-rm, vvvv unused(1111,V'=1),
  # masking ok.  rnd in {None,'er','sae'} adds the reg-reg embedded-rounding form.
  E('%s %s z ll 0 1 aaa 0x%02x 11 ggg rrr => wit("evex") "%s " %s %s "," %s ;' % (P0(mm), P1F(w, pp), op, mn, RG, KZ, RM))
  if rnd == "er":
    E('%s %s z ll 1 1 aaa 0x%02x 11 ggg rrr => wit("evex") "%s " %s %s "," %s rcdec[$l] ;' % (P0(mm), P1F(w, pp), op, mn, ZG, KZ, ZM))
  elif rnd == "sae":
    E('%s %s z 00 1 1 aaa 0x%02x 11 ggg rrr => wit("evex") "%s " %s %s "," %s " {sae}" ;' % (P0(mm), P1F(w, pp), op, mn, ZG, KZ, ZM))
  E('%s %s z ll 0 1 aaa 0x%02x @addr %s => wit("evex") "%s " %s %s "," $addr ;' % (P0(mm), P1F(w, pp), op, ACT, mn, RG, KZ))
  if bcst:
    E('%s %s z ll 1 1 aaa 0x%02x @addr %s => wit("evex") "%s " %s %s "," $addr %s[$l] ;' % (P0(mm), P1F(w, pp), op, ACT, mn, RG, KZ, bcst))

def scal_cvt(mn, op, w, pp, rnd, mm="01"):
  # scalar convert (xmm), 3-operand reg/vvvv/rm.  EVEX form is selected only when
  # masked or with {er}/{sae}; the plain form is shorter as VEX (asm picks that).
  E('%s %s z ll 0 u aaa 0x%02x 11 ggg rrr => wit("evex") "%s " %s %s "," %s "," %s ;' % (P0(mm), P1(w, pp), op, mn, XG, KZ, XV, XM))
  if rnd == "er":
    E('%s %s z ll 1 u aaa 0x%02x 11 ggg rrr => wit("evex") "%s " %s %s "," %s "," %s rcdec[$l] ;' % (P0(mm), P1(w, pp), op, mn, ZG, KZ, ZV, ZM))
  elif rnd == "sae":
    E('%s %s z 00 1 u aaa 0x%02x 11 ggg rrr => wit("evex") "%s " %s %s "," %s "," %s " {sae}" ;' % (P0(mm), P1(w, pp), op, mn, ZG, KZ, ZV, ZM))
  E('%s %s z ll 0 u aaa 0x%02x @addr %s => wit("evex") "%s " %s %s "," %s "," $addr ;' % (P0(mm), P1(w, pp), op, ACT, mn, XG, KZ, XV))

def cvtsi(mn, op, pp, mm="01"):
  # vcvtsi2sd/ss: dest=xmm vvvv=xmm src2=GP(W-driven).  EVEX form carries only the
  # embedded rounding ({er}, reg-reg); plain/mem forms are emitted as VEX.
  E('%s w vvvv 1 %s z ll 1 u aaa 0x%02x 11 ggg rrr {$rexb=1-$b} => wit("evex") "%s " %s %s "," %s "," %s rcdec[$l] ;' % (P0(mm), pp, op, mn, ZG, KZ, ZV, GRm))

def cvt2si(mn, op, pp, rnd, mm="01"):
  # vcvt[t]sd2si/ss2si: dest=GP(W-driven) src=xmm.  EVEX only for {er}/{sae}.
  if rnd == "er":
    E('%s w 1111 1 %s z ll 1 u aaa 0x%02x 11 ggg rrr {$rexr=1-$h} => wit("evex") "%s " %s "," %s rcdec[$l] ;' % (P0(mm), pp, op, mn, GRg, ZM))
  else:
    E('%s w 1111 1 %s z 00 1 u aaa 0x%02x 11 ggg rrr {$rexr=1-$h} => wit("evex") "%s " %s "," %s " {sae}" ;' % (P0(mm), pp, op, mn, GRg, ZM))

def bcast(mn, op, w, pp, mm="10"):
  # broadcast a low element (xmm) or m32/m64 across all lanes.  dest=full(reg).
  E('%s %s z ll 0 1 aaa 0x%02x 11 ggg rrr => wit("evex") "%s " %s %s "," %s ;' % (P0(mm), P1F(w, pp), op, mn, RG, KZ, XM))
  E('%s %s z ll 0 1 aaa 0x%02x @addr %s => wit("evex") "%s " %s %s "," $addr ;' % (P0(mm), P1F(w, pp), op, ACT, mn, RG, KZ))

def bcast_gp(mn, op, w, pp, bank, mm="10"):
  # vpbroadcastd/q from a GP register (W fixes element width and mnemonic).
  E('%s %s z ll 0 1 aaa 0x%02x 11 ggg rrr {$rexb=1-$b} => wit("evex") "%s " %s %s "," greg[%d+8*$rexb+$r] ;' % (P0(mm), P1F(w, pp), op, mn, RG, KZ, bank))

def shimm(mn, op, digit, w, pp, bcst, mm="01"):
  # shift-by-imm8: dest=vvvv(NDD) src=rm, /digit in modrm.reg, masking on dest.
  db = format(digit, "03b")
  E('%s %s z ll 0 u aaa 0x%02x 11 %s rrr @imm8 => wit("evex") "%s " %s %s "," %s "," hex($imm8) ;' % (P0(mm), P1(w, pp), op, db, mn, VV, KZ, RM))
  E('%s %s z ll 0 u aaa 0x%02x @addr(%d) %s @imm8 => wit("evex") "%s " %s %s "," $addr "," hex($imm8) ;' % (P0(mm), P1(w, pp), op, digit, ACT, mn, VV, KZ))
  if bcst:
    E('%s %s z ll 1 u aaa 0x%02x @addr(%d) %s @imm8 => wit("evex") "%s " %s %s "," $addr %s[$l] "," hex($imm8) ;' % (P0(mm), P1(w, pp), op, digit, ACT, mn, VV, KZ, bcst))

def perm_imm(mn, op, w, pp, bcst, mm="11"):
  # 2-op + imm8 (vpermq/vpermpd): dest=reg src=rm, vvvv unused, broadcast on mem.
  E('%s %s z ll 0 1 aaa 0x%02x 11 ggg rrr @imm8 => wit("evex") "%s " %s %s "," %s "," hex($imm8) ;' % (P0(mm), P1F(w, pp), op, mn, RG, KZ, RM))
  E('%s %s z ll 0 1 aaa 0x%02x @addr %s @imm8 => wit("evex") "%s " %s %s "," $addr "," hex($imm8) ;' % (P0(mm), P1F(w, pp), op, ACT, mn, RG, KZ))
  if bcst:
    E('%s %s z ll 1 1 aaa 0x%02x @addr %s @imm8 => wit("evex") "%s " %s %s "," $addr %s[$l] "," hex($imm8) ;' % (P0(mm), P1F(w, pp), op, ACT, mn, RG, KZ, bcst))

def int3i(mn, op, w, pp, bcst, mm="11"):
  # 3-source + imm8 (vpternlog): dest=reg src1=vvvv src2=rm, masking, broadcast.
  E('%s %s z ll 0 u aaa 0x%02x 11 ggg rrr @imm8 => wit("evex") "%s " %s %s "," %s "," %s "," hex($imm8) ;' % (P0(mm), P1(w, pp), op, mn, RG, KZ, VV, RM))
  E('%s %s z ll 0 u aaa 0x%02x @addr %s @imm8 => wit("evex") "%s " %s %s "," %s "," $addr "," hex($imm8) ;' % (P0(mm), P1(w, pp), op, ACT, mn, RG, KZ, VV))
  if bcst:
    E('%s %s z ll 1 u aaa 0x%02x @addr %s @imm8 => wit("evex") "%s " %s %s "," %s "," $addr %s[$l] "," hex($imm8) ;' % (P0(mm), P1(w, pp), op, ACT, mn, RG, KZ, VV, bcst))

def rndi(mn, op, w, pp, bcst, mm="11"):
  # 2-op + imm8 with optional {sae} (vrndscale/vgetmant): dest=reg src=rm, vvvv unused.
  E('%s %s z ll 0 1 aaa 0x%02x 11 ggg rrr @imm8 => wit("evex") "%s " %s %s "," %s "," hex($imm8) ;' % (P0(mm), P1F(w, pp), op, mn, RG, KZ, RM))
  E('%s %s z 00 1 1 aaa 0x%02x 11 ggg rrr @imm8 => wit("evex") "%s " %s %s "," %s " {sae}" "," hex($imm8) ;' % (P0(mm), P1F(w, pp), op, mn, ZG, KZ, ZM))
  E('%s %s z ll 0 1 aaa 0x%02x @addr %s @imm8 => wit("evex") "%s " %s %s "," $addr "," hex($imm8) ;' % (P0(mm), P1F(w, pp), op, ACT, mn, RG, KZ))
  if bcst:
    E('%s %s z ll 1 1 aaa 0x%02x @addr %s @imm8 => wit("evex") "%s " %s %s "," $addr %s[$l] "," hex($imm8) ;' % (P0(mm), P1F(w, pp), op, ACT, mn, RG, KZ, bcst))

EGH = "eregh[32*$l+16*$e+8*$h+$g]"   # reg, half width (L'L selects size)
EMH = "eregh[32*$l+16*$k+8*$b+$r]"   # rm,  half width
ZGH = "eregh[64+16*$e+8*$h+$g]"      # reg, half-at-512 (ymm), rounding form
ZMH = "eregh[64+16*$k+8*$b+$r]"      # rm,  half-at-512 (ymm)

def wcvt(mn, op, w, pp, half, rnd=None, mm="01"):
  # width-changing convert: one operand is full-width, the other a size class
  # smaller (eregh).  half='src': dest full(reg) / src half(rm).  half='dst':
  # dest half(reg) / src full(rm).  Broadcast count is the qword count (bcst64).
  if half == "src":
    rg, rm, zrg, zrm = RG, EMH, ZG, ZMH
  else:
    rg, rm, zrg, zrm = EGH, RM, ZGH, ZM
  E('%s %s z ll 0 1 aaa 0x%02x 11 ggg rrr => wit("evex") "%s " %s %s "," %s ;' % (P0(mm), P1F(w, pp), op, mn, rg, KZ, rm))
  if rnd == "er":
    E('%s %s z ll 1 1 aaa 0x%02x 11 ggg rrr => wit("evex") "%s " %s %s "," %s rcdec[$l] ;' % (P0(mm), P1F(w, pp), op, mn, zrg, KZ, zrm))
  elif rnd == "sae":
    E('%s %s z 00 1 1 aaa 0x%02x 11 ggg rrr => wit("evex") "%s " %s %s "," %s " {sae}" ;' % (P0(mm), P1F(w, pp), op, mn, zrg, KZ, zrm))
  E('%s %s z ll 0 1 aaa 0x%02x @addr %s => wit("evex") "%s " %s %s "," $addr ;' % (P0(mm), P1F(w, pp), op, ACT, mn, rg, KZ))
  E('%s %s z ll 1 1 aaa 0x%02x @addr %s => wit("evex") "%s " %s %s "," $addr bcst64[$l] ;' % (P0(mm), P1F(w, pp), op, ACT, mn, rg, KZ))

EXM = "eregx[32*$l+16*$k+8*$b+$r]"   # rm, quarter/eighth width (always xmm)

def pmovx(mn, op, narrow):
  # vpmovzx/sx widening: dest full(reg), src narrower(rm); 66.0F38.W0, masking.
  rm = EXM if narrow == "x" else EMH
  E('%s %s z ll 0 1 aaa 0x%02x 11 ggg rrr => wit("evex") "%s " %s %s "," %s ;' % (P0("10"), P1F("0", "01"), op, mn, RG, KZ, rm))
  E('%s %s z ll 0 1 aaa 0x%02x @addr %s => wit("evex") "%s " %s %s "," $addr ;' % (P0("10"), P1F("0", "01"), op, ACT, mn, RG, KZ))

def pmovt(mn, op, narrow):
  # vpmov truncate/saturate: store-form, dest narrower(rm), src full(reg);
  # F3.0F38.W0, masking on the destination.
  rm = EXM if narrow == "x" else EMH
  E('%s %s z ll 0 1 aaa 0x%02x 11 ggg rrr => wit("evex") wit("alt") "%s " %s %s "," %s ;' % (P0("10"), P1F("0", "10"), op, mn, rm, KZ, RG))
  E('%s %s z ll 0 1 aaa 0x%02x @addr %s => wit("evex") "%s " $addr %s "," %s ;' % (P0("10"), P1F("0", "10"), op, ACT, mn, KZ, RG))

def scal_imm(mn, op, w, pp, mm="11"):
  # scalar 3-op + imm8 with optional {sae} (vrndscaless/sd): all xmm.
  E('%s %s z ll 0 u aaa 0x%02x 11 ggg rrr @imm8 => wit("evex") "%s " %s %s "," %s "," %s "," hex($imm8) ;' % (P0(mm), P1(w, pp), op, mn, XG, KZ, XV, XM))
  E('%s %s z 00 1 u aaa 0x%02x 11 ggg rrr @imm8 => wit("evex") "%s " %s %s "," %s "," %s " {sae}" "," hex($imm8) ;' % (P0(mm), P1(w, pp), op, mn, ZG, KZ, ZV, ZM))
  E('%s %s z ll 0 u aaa 0x%02x @addr %s @imm8 => wit("evex") "%s " %s %s "," %s "," $addr "," hex($imm8) ;' % (P0(mm), P1(w, pp), op, ACT, mn, XG, KZ, XV))

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
# --- compares / tests writing a mask register (k) ---
kcmp("vpcmpeqd", 0x76, "0", "01", "01")
kcmp("vpcmpgtd", 0x66, "0", "01", "01")
kcmp("vcmpps", 0xc2, "0", "00", "01", imm=True)
kcmp("vcmppd", 0xc2, "1", "01", "01", imm=True)
kcmp("vpcmpd", 0x1f, "0", "01", "11", imm=True)
kcmp("vpcmpud", 0x1e, "0", "01", "11", imm=True)
kcmp("vpcmpq", 0x1f, "1", "01", "11", imm=True)
kcmp("vpcmpuq", 0x1e, "1", "01", "11", imm=True)
kcmp("vptestmd", 0x27, "0", "01", "10")
kcmp("vptestmq", 0x27, "1", "01", "10")
# --- same-width converts (int<->fp, 32-bit elements): {er}/{sae}, broadcast ---
cvt2("vcvtdq2ps", 0x5b, "0", "00", "bcst32", rnd="er")
cvt2("vcvtps2dq", 0x5b, "0", "01", "bcst32", rnd="er")
cvt2("vcvttps2dq", 0x5b, "0", "10", "bcst32", rnd="sae")
# --- scalar converts (xmm), 3-operand: masked + {er}/{sae} forms ---
scal_cvt("vcvtss2sd", 0x5a, "0", "10", "sae")
scal_cvt("vcvtsd2ss", 0x5a, "1", "11", "er")
# --- GP<->scalar converts: EVEX carries only embedded rounding ---
cvtsi("vcvtsi2sd", 0x2a, "11"); cvtsi("vcvtsi2ss", 0x2a, "10")
cvt2si("vcvtsd2si", 0x2d, "11", "er"); cvt2si("vcvtss2si", 0x2d, "10", "er")
cvt2si("vcvttsd2si", 0x2c, "11", "sae"); cvt2si("vcvttss2si", 0x2c, "10", "sae")
# --- broadcasts (0F38) ---
bcast("vbroadcastss", 0x18, "0", "01"); bcast("vbroadcastsd", 0x19, "1", "01")
bcast("vpbroadcastd", 0x58, "0", "01"); bcast("vpbroadcastq", 0x59, "1", "01")
bcast_gp("vpbroadcastd", 0x7c, "0", "01", 0); bcast_gp("vpbroadcastq", 0x7c, "1", "01", 32)
# --- integer min/max (0F38: d/q broadcast; 0F + b/w none) ---
int3("vpmaxsd", 0x3d, "0", "01", "bcst32", mm="10"); int3("vpmaxsq", 0x3d, "1", "01", "bcst64", mm="10")
int3("vpminsd", 0x39, "0", "01", "bcst32", mm="10"); int3("vpminsq", 0x39, "1", "01", "bcst64", mm="10")
int3("vpmaxud", 0x3f, "0", "01", "bcst32", mm="10"); int3("vpmaxuq", 0x3f, "1", "01", "bcst64", mm="10")
int3("vpminud", 0x3b, "0", "01", "bcst32", mm="10"); int3("vpminuq", 0x3b, "1", "01", "bcst64", mm="10")
int3("vpmaxsb", 0x3c, "0", "01", mm="10"); int3("vpminsb", 0x38, "0", "01", mm="10")
int3("vpmaxuw", 0x3e, "0", "01", mm="10"); int3("vpminuw", 0x3a, "0", "01", mm="10")
int3("vpmaxsw", 0xee, "0", "01"); int3("vpminsw", 0xea, "0", "01")
int3("vpmaxub", 0xde, "0", "01"); int3("vpminub", 0xda, "0", "01")
# --- integer abs (0F38) : d/q broadcast, b/w none ---
cvt2("vpabsd", 0x1e, "0", "01", "bcst32", mm="10"); cvt2("vpabsq", 0x1f, "1", "01", "bcst64", mm="10")
cvt2("vpabsb", 0x1c, "0", "01", mm="10"); cvt2("vpabsw", 0x1d, "0", "01", mm="10")
# --- variable (per-element) shifts (0F38) ---
int3("vpsllvd", 0x47, "0", "01", "bcst32", mm="10"); int3("vpsllvq", 0x47, "1", "01", "bcst64", mm="10")
int3("vpsrlvd", 0x45, "0", "01", "bcst32", mm="10"); int3("vpsrlvq", 0x45, "1", "01", "bcst64", mm="10")
int3("vpsravd", 0x46, "0", "01", "bcst32", mm="10"); int3("vpsravq", 0x46, "1", "01", "bcst64", mm="10")
# --- shifts by imm8 (0F 71/72/73 /digit) ---
shimm("vpslld", 0x72, 6, "0", "01", "bcst32"); shimm("vpsrld", 0x72, 2, "0", "01", "bcst32")
shimm("vpsrad", 0x72, 4, "0", "01", "bcst32"); shimm("vpsraq", 0x72, 4, "1", "01", "bcst64")
shimm("vpsllq", 0x73, 6, "1", "01", "bcst64"); shimm("vpsrlq", 0x73, 2, "1", "01", "bcst64")
shimm("vpsllw", 0x71, 6, "0", "01", None); shimm("vpsrlw", 0x71, 2, "0", "01", None)
shimm("vpsraw", 0x71, 4, "0", "01", None)
# --- permutes ---
int3("vpermd", 0x36, "0", "01", "bcst32", mm="10"); int3("vpermps", 0x16, "0", "01", "bcst32", mm="10")
perm_imm("vpermq", 0x00, "1", "01", "bcst64"); perm_imm("vpermpd", 0x01, "1", "01", "bcst64")
# --- vpternlog (0F3A): 3-source bitwise LUT + imm8 ---
int3i("vpternlogd", 0x25, "0", "01", "bcst32"); int3i("vpternlogq", 0x25, "1", "01", "bcst64")
# --- sqrt (0F 51): packed {er} + scalar {er} ---
cvt2("vsqrtps", 0x51, "0", "00", "bcst32", rnd="er"); cvt2("vsqrtpd", 0x51, "1", "01", "bcst64", rnd="er")
scal_cvt("vsqrtss", 0x51, "0", "10", "er"); scal_cvt("vsqrtsd", 0x51, "1", "11", "er")
# --- integer multiply (d/q) ---
int3("vpmuldq", 0x28, "1", "01", "bcst64", mm="10"); int3("vpmuludq", 0xf4, "1", "01", "bcst64")
int3("vpmullq", 0x40, "1", "01", "bcst64", mm="10")
# --- round-to-scale (0F3A) : packed imm8+{sae} + scalar imm8 ---
rndi("vrndscaleps", 0x08, "0", "01", "bcst32"); rndi("vrndscalepd", 0x09, "1", "01", "bcst64")
scal_imm("vrndscaless", 0x0a, "0", "01"); scal_imm("vrndscalesd", 0x0b, "1", "01")
# --- scale by 2^x (0F38) : packed {er} + scalar {er} ---
fp3("vscalefps", 0x2c, "0", "01", "bcst32", er=True, mm="10"); fp3("vscalefpd", 0x2c, "1", "01", "bcst64", er=True, mm="10")
scal_cvt("vscalefss", 0x2d, "0", "01", "er", mm="10"); scal_cvt("vscalefsd", 0x2d, "1", "01", "er", mm="10")
# --- reciprocal / rsqrt approximations (0F38) : packed + scalar ---
cvt2("vrcp14ps", 0x4c, "0", "01", "bcst32", mm="10"); cvt2("vrcp14pd", 0x4c, "1", "01", "bcst64", mm="10")
cvt2("vrsqrt14ps", 0x4e, "0", "01", "bcst32", mm="10"); cvt2("vrsqrt14pd", 0x4e, "1", "01", "bcst64", mm="10")
scal_cvt("vrcp14ss", 0x4d, "0", "01", None, mm="10"); scal_cvt("vrcp14sd", 0x4d, "1", "01", None, mm="10")
scal_cvt("vrsqrt14ss", 0x4f, "0", "01", None, mm="10"); scal_cvt("vrsqrt14sd", 0x4f, "1", "01", None, mm="10")
# --- exponent / mantissa extraction ---
cvt2("vgetexpps", 0x42, "0", "01", "bcst32", rnd="sae", mm="10"); cvt2("vgetexppd", 0x42, "1", "01", "bcst64", rnd="sae", mm="10")
rndi("vgetmantps", 0x26, "0", "01", "bcst32"); rndi("vgetmantpd", 0x26, "1", "01", "bcst64")
# --- merge-blend by mask (0F38) ---
int3("vpblendmd", 0x64, "0", "01", "bcst32", mm="10"); int3("vpblendmq", 0x64, "1", "01", "bcst64", mm="10")
int3("vblendmps", 0x65, "0", "01", "bcst32", mm="10"); int3("vblendmpd", 0x65, "1", "01", "bcst64", mm="10")
# --- test-not-mask -> k (F3.0F38) ---
kcmp("vptestnmd", 0x27, "0", "10", "10"); kcmp("vptestnmq", 0x27, "1", "10", "10")
# --- width-changing converts (operands differ in vector width) ---
wcvt("vcvtps2pd", 0x5a, "0", "00", "src", rnd="sae")   # SP->DP, src half
wcvt("vcvtpd2ps", 0x5a, "1", "01", "dst", rnd="er")    # DP->SP, dest half
wcvt("vcvtdq2pd", 0xe6, "0", "10", "src")              # i32->DP, src half
wcvt("vcvtudq2pd", 0x7a, "0", "10", "src")             # u32->DP, src half
wcvt("vcvtpd2dq", 0xe6, "1", "11", "dst", rnd="er")    # DP->i32, dest half
wcvt("vcvttpd2dq", 0xe6, "1", "01", "dst", rnd="sae")  # DP->i32 trunc, dest half
# --- integer width converts: zero/sign-extend widen (66.0F38) ---
for mn, op, nw in [("vpmovzxbw", 0x30, "h"), ("vpmovzxbd", 0x31, "x"), ("vpmovzxbq", 0x32, "x"),
                   ("vpmovzxwd", 0x33, "h"), ("vpmovzxwq", 0x34, "x"), ("vpmovzxdq", 0x35, "h"),
                   ("vpmovsxbw", 0x20, "h"), ("vpmovsxbd", 0x21, "x"), ("vpmovsxbq", 0x22, "x"),
                   ("vpmovsxwd", 0x23, "h"), ("vpmovsxwq", 0x24, "x"), ("vpmovsxdq", 0x25, "h")]:
  pmovx(mn, op, nw)
# --- integer width converts: narrowing truncate / s-saturate / us-saturate (F3.0F38) ---
for mn, op, nw in [("vpmovwb", 0x30, "h"), ("vpmovdb", 0x31, "x"), ("vpmovqb", 0x32, "x"),
                   ("vpmovdw", 0x33, "h"), ("vpmovqw", 0x34, "x"), ("vpmovqd", 0x35, "h"),
                   ("vpmovswb", 0x20, "h"), ("vpmovsdb", 0x21, "x"), ("vpmovsqb", 0x22, "x"),
                   ("vpmovsdw", 0x23, "h"), ("vpmovsqw", 0x24, "x"), ("vpmovsqd", 0x25, "h"),
                   ("vpmovuswb", 0x10, "h"), ("vpmovusdb", 0x11, "x"), ("vpmovusqb", 0x12, "x"),
                   ("vpmovusdw", 0x13, "h"), ("vpmovusqw", 0x14, "x"), ("vpmovusqd", 0x15, "h")]:
  pmovt(mn, op, nw)
v.append("}")
print("\n".join(v))
