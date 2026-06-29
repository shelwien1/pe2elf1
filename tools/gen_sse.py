#!/usr/bin/env python3
# gen_sse.py -- emit the x86-64 SSE/SSE2/SSE3 0F-map rules for corpus64.p.
# The rules are mechanical; generating them keeps the REX/mm-xmm conventions
# uniform.  Conventions:
#   xmm reg field  : xreg[8*$rexr+$g]      xmm rm field : xreg[8*$rexb+$r]
#   mm/xmm-by-66   : simd[8*$opsiz+8*$rexr+$g]  (mm when 66 absent, else xmm)
#   GP for cvt/movd: greg[32*$rexw+8*$rexX+f] (32- or 64-bit by REX.W)
G = "8*$rexr+$g"          # xmm reg field
Rr = "8*$rexb+$r"         # xmm rm field
SG = "8*$opsiz+16*$rexr+$g"   # mm/xmm reg field (66 -> xmm)
SR = "8*$opsiz+16*$rexb+$r"   # mm/xmm rm field
out = []


def emit(s):
  out.append("  " + s)


# --- mov-family (10-17, 28-2B) -------------------------------------------
emit('0x0f 0x10 11 ggg rrr => movu[$reptype*2+$opsiz] " " xreg[%s] "," xreg[%s] ;' % (G, Rr))
emit('0x0f 0x10 @addr      => movu[$reptype*2+$opsiz] sfx[movusz[$reptype*2+$opsiz]] " " xreg[%s] "," $addr ;' % G)
emit('0x0f 0x11 11 ggg rrr => movu[$reptype*2+$opsiz] " " xreg[%s] "," xreg[%s] wit("alt") ;' % (Rr, G))
emit('0x0f 0x11 @addr      => movu[$reptype*2+$opsiz] sfx[movusz[$reptype*2+$opsiz]] " " $addr "," xreg[%s] ;' % G)
emit('0x0f 0x12 11 ggg rrr [$reptype] => m12f[$reptype] " " xreg[%s] "," xreg[%s] ;' % (G, Rr))
emit('0x0f 0x12 @addr      [$reptype] => m12f[$reptype] " " xreg[%s] "," $addr ;' % G)
emit('0x0f 0x12 11 ggg rrr => "movhlps " xreg[%s] "," xreg[%s] ;' % (G, Rr))
emit('0x0f 0x12 @addr      => ml12[$opsiz] " " xreg[%s] "," $addr ;' % G)
emit('0x0f 0x13 @addr      => ml12[$opsiz] " " $addr "," xreg[%s] ;' % G)
emit('0x0f 0x14 11 ggg rrr => unpl[$opsiz] " " xreg[%s] "," xreg[%s] ;' % (G, Rr))
emit('0x0f 0x14 @addr      => unpl[$opsiz] " " xreg[%s] "," $addr ;' % G)
emit('0x0f 0x15 11 ggg rrr => unph[$opsiz] " " xreg[%s] "," xreg[%s] ;' % (G, Rr))
emit('0x0f 0x15 @addr      => unph[$opsiz] " " xreg[%s] "," $addr ;' % G)
emit('0x0f 0x16 11 ggg rrr [$reptype] => m16f[$reptype] " " xreg[%s] "," xreg[%s] ;' % (G, Rr))
emit('0x0f 0x16 @addr      [$reptype] => m16f[$reptype] " " xreg[%s] "," $addr ;' % G)
emit('0x0f 0x16 11 ggg rrr => "movlhps " xreg[%s] "," xreg[%s] ;' % (G, Rr))
emit('0x0f 0x16 @addr      => mh16[$opsiz] " " xreg[%s] "," $addr ;' % G)
emit('0x0f 0x17 @addr      => mh16[$opsiz] " " $addr "," xreg[%s] ;' % G)
emit('0x0f 0x28 11 ggg rrr => mova2[$opsiz] " " xreg[%s] "," xreg[%s] ;' % (G, Rr))
emit('0x0f 0x28 @addr      => mova2[$opsiz] " " xreg[%s] "," $addr ;' % G)
emit('0x0f 0x29 11 ggg rrr => mova2[$opsiz] " " xreg[%s] "," xreg[%s] wit("alt") ;' % (Rr, G))
emit('0x0f 0x29 @addr      => mova2[$opsiz] " " $addr "," xreg[%s] ;' % G)
emit('0x0f 0x2b @addr      => mnt4[$reptype*2+$opsiz] " " $addr "," xreg[%s] ;' % G)

# --- cvt (2A/2C/2D) : GP <-> xmm/mm ---------------------------------------
emit('0x0f 0x2a 11 ggg rrr [$reptype] => cvt2a[$reptype*2+$opsiz] " " xreg[%s] "," greg[32*$rexw+%s] ;' % (G, Rr))
emit('0x0f 0x2a 11 ggg rrr => cvt2a[$reptype*2+$opsiz] " " simd[%s] "," simd[%s] ;' % (SG, SR))
emit('0x0f 0x2a @addr [$reptype] => cvt2a[$reptype*2+$opsiz] " " xreg[%s] "," $addr ;' % G)
emit('0x0f 0x2a @addr      => cvt2a[$reptype*2+$opsiz] " " simd[%s] "," $addr ;' % SG)
emit('0x0f 0x2c 11 ggg rrr [$reptype] => cvt2c[$reptype*2+$opsiz] " " greg[32*$rexw+%s] "," xreg[%s] ;' % (G, Rr))
emit('0x0f 0x2c 11 ggg rrr => cvt2c[$reptype*2+$opsiz] " " mmreg[$g] "," xreg[%s] ;' % Rr)
emit('0x0f 0x2c @addr [$reptype] => cvt2c[$reptype*2+$opsiz] " " greg[32*$rexw+%s] "," $addr ;' % G)
emit('0x0f 0x2d 11 ggg rrr [$reptype] => cvt2d[$reptype*2+$opsiz] " " greg[32*$rexw+%s] "," xreg[%s] ;' % (G, Rr))
emit('0x0f 0x2d 11 ggg rrr => cvt2d[$reptype*2+$opsiz] " " mmreg[$g] "," xreg[%s] ;' % Rr)
emit('0x0f 0x2d @addr [$reptype] => cvt2d[$reptype*2+$opsiz] " " greg[32*$rexw+%s] "," $addr ;' % G)

# --- ucomi/comi (2E/2F) ---------------------------------------------------
emit('0x0f 0x2e 11 ggg rrr => uc[$opsiz] " " xreg[%s] "," xreg[%s] ;' % (G, Rr))
emit('0x0f 0x2e @addr      => uc[$opsiz] " " xreg[%s] "," $addr ;' % G)
emit('0x0f 0x2f 11 ggg rrr => comi[$opsiz] " " xreg[%s] "," xreg[%s] ;' % (G, Rr))
emit('0x0f 0x2f @addr      => comi[$opsiz] " " xreg[%s] "," $addr ;' % G)

# --- packed/scalar float ops (50-5F) --------------------------------------
emit('0x0f 0x50 11 ggg rrr => "movmsk" elt[$opsiz] " " greg[32*$rexw+%s] "," xreg[%s] ;' % (G, Rr))
fl = {0x51: "sqrt", 0x52: "rsqrt", 0x53: "rcp", 0x54: "and", 0x55: "andn",
      0x56: "or", 0x57: "xor", 0x58: "add", 0x59: "mul", 0x5c: "sub",
      0x5d: "min", 0x5e: "div", 0x5f: "max"}
for op in sorted(fl):
  mn = fl[op]
  emit('0x0f 0x%02x 11 ggg rrr => "%s" elt[$reptype*2+$opsiz] " " xreg[%s] "," xreg[%s] ;' % (op, mn, G, Rr))
  emit('0x0f 0x%02x @addr      => "%s" elt[$reptype*2+$opsiz] " " xreg[%s] "," $addr ;' % (op, mn, G))
emit('0x0f 0x5a 11 ggg rrr => cvt5a[$reptype*2+$opsiz] " " xreg[%s] "," xreg[%s] ;' % (G, Rr))
emit('0x0f 0x5a @addr      => cvt5a[$reptype*2+$opsiz] " " xreg[%s] "," $addr ;' % G)
emit('0x0f 0x5b 11 ggg rrr => cvt5b[$reptype*2+$opsiz] " " xreg[%s] "," xreg[%s] ;' % (G, Rr))
emit('0x0f 0x5b @addr      => cvt5b[$reptype*2+$opsiz] " " xreg[%s] "," $addr ;' % G)

# --- mm/xmm-by-66 integer ops (60-6D, 74-76, D1-FE minus specials) --------
integ = {
  0x60: "punpcklbw", 0x61: "punpcklwd", 0x62: "punpckldq", 0x63: "packsswb",
  0x64: "pcmpgtb", 0x65: "pcmpgtw", 0x66: "pcmpgtd", 0x67: "packuswb",
  0x68: "punpckhbw", 0x69: "punpckhwd", 0x6a: "punpckhdq", 0x6b: "packssdw",
  0x6c: "punpcklqdq", 0x6d: "punpckhqdq",
  0x74: "pcmpeqb", 0x75: "pcmpeqw", 0x76: "pcmpeqd",
  0xd1: "psrlw", 0xd2: "psrld", 0xd3: "psrlq", 0xd4: "paddq", 0xd5: "pmullw",
  0xd8: "psubusb", 0xd9: "psubusw", 0xda: "pminub", 0xdb: "pand",
  0xdc: "paddusb", 0xdd: "paddusw", 0xde: "pmaxub", 0xdf: "pandn",
  0xe0: "pavgb", 0xe1: "psraw", 0xe2: "psrad", 0xe3: "pavgw",
  0xe4: "pmulhuw", 0xe5: "pmulhw", 0xe8: "psubsb", 0xe9: "psubsw",
  0xea: "pminsw", 0xeb: "por", 0xec: "paddsb", 0xed: "paddsw",
  0xee: "pmaxsw", 0xef: "pxor", 0xf1: "psllw", 0xf2: "pslld", 0xf3: "psllq",
  0xf4: "pmuludq", 0xf5: "pmaddwd", 0xf6: "psadbw", 0xf8: "psubb",
  0xf9: "psubw", 0xfa: "psubd", 0xfb: "psubq", 0xfc: "paddb",
  0xfd: "paddw", 0xfe: "paddd",
}
for op in sorted(integ):
  mn = integ[op]
  emit('0x0f 0x%02x 11 ggg rrr => "%s " simd[%s] "," simd[%s] ;' % (op, mn, SG, SR))
  emit('0x0f 0x%02x @addr      => "%s " simd[%s] "," $addr ;' % (op, mn, SG))

# --- movd/movq (6E/7E) ----------------------------------------------------
emit('0x0f 0x6e 11 ggg rrr => mdq2[$rexw] " " simd[%s] "," greg[32*$rexw+%s] ;' % (SG, Rr))
emit('0x0f 0x6e @addr      => mdq2[$rexw] sfx[$rexw? 8 : 4] " " simd[%s] "," $addr ;' % SG)
emit('0x0f 0x7e 11 ggg rrr [$reptype==1] => "movq " xreg[%s] "," xreg[%s] ;' % (G, Rr))
emit('0x0f 0x7e @addr      [$reptype==1] => "movq " xreg[%s] "," $addr ;' % G)
emit('0x0f 0x7e 11 ggg rrr => mdq2[$rexw] " " greg[32*$rexw+%s] "," simd[%s] ;' % (Rr, SG))
emit('0x0f 0x7e @addr      => mdq2[$rexw] sfx[$rexw? 8 : 4] " " $addr "," simd[%s] ;' % SG)

# --- movdqa/movdqu/movq (6F/7F) -------------------------------------------
emit('0x0f 0x6f 11 ggg rrr [$reptype==1] => "movdqu " xreg[%s] "," xreg[%s] ;' % (G, Rr))
emit('0x0f 0x6f @addr      [$reptype==1] => "movdqu " xreg[%s] "," $addr ;' % G)
emit('0x0f 0x6f 11 ggg rrr [$opsiz] => "movdqa " xreg[%s] "," xreg[%s] ;' % (G, Rr))
emit('0x0f 0x6f @addr      [$opsiz] => "movdqa " xreg[%s] "," $addr ;' % G)
emit('0x0f 0x6f 11 ggg rrr => "movq " mmreg[$g] "," mmreg[$r] ;')
emit('0x0f 0x6f @addr      => "movq " mmreg[$g] "," $addr ;')
emit('0x0f 0x7f 11 ggg rrr [$reptype==1] => "movdqu " xreg[%s] "," xreg[%s] wit("alt") ;' % (Rr, G))
emit('0x0f 0x7f @addr      [$reptype==1] => "movdqu " $addr "," xreg[%s] ;' % G)
emit('0x0f 0x7f 11 ggg rrr [$opsiz] => "movdqa " xreg[%s] "," xreg[%s] wit("alt") ;' % (Rr, G))
emit('0x0f 0x7f @addr      [$opsiz] => "movdqa " $addr "," xreg[%s] ;' % G)
emit('0x0f 0x7f 11 ggg rrr => "movq " mmreg[$r] "," mmreg[$g] wit("alt") ;')
emit('0x0f 0x7f @addr      => "movq " $addr "," mmreg[$g] ;')

# --- pshuf (70) -----------------------------------------------------------
emit('0x0f 0x70 11 ggg rrr @imm8 [$opsiz] => "pshufd " xreg[%s] "," xreg[%s] "," hex($imm8) ;' % (G, Rr))
emit('0x0f 0x70 @addr @imm8 [$opsiz] => "pshufd " xreg[%s] "," $addr "," hex($imm8) ;' % G)
emit('0x0f 0x70 11 ggg rrr @imm8 [$reptype==1] => "pshufhw " xreg[%s] "," xreg[%s] "," hex($imm8) ;' % (G, Rr))
emit('0x0f 0x70 @addr @imm8 [$reptype==1] => "pshufhw " xreg[%s] "," $addr "," hex($imm8) ;' % G)
emit('0x0f 0x70 11 ggg rrr @imm8 [$reptype==2] => "pshuflw " xreg[%s] "," xreg[%s] "," hex($imm8) ;' % (G, Rr))
emit('0x0f 0x70 @addr @imm8 [$reptype==2] => "pshuflw " xreg[%s] "," $addr "," hex($imm8) ;' % G)
emit('0x0f 0x70 11 ggg rrr @imm8 => "pshufw " mmreg[$g] "," mmreg[$r] "," hex($imm8) ;')
emit('0x0f 0x70 @addr @imm8 => "pshufw " mmreg[$g] "," $addr "," hex($imm8) ;')

# --- shift-by-imm (71/72/73) ----------------------------------------------
for op, dig, mn in [(0x71, 2, "psrlw"), (0x71, 4, "psraw"), (0x71, 6, "psllw"),
                    (0x72, 2, "psrld"), (0x72, 4, "psrad"), (0x72, 6, "pslld"),
                    (0x73, 2, "psrlq"), (0x73, 3, "psrldq"), (0x73, 6, "psllq"),
                    (0x73, 7, "pslldq")]:
  d = format(dig, "03b")
  emit('0x0f 0x%02x 11 %s rrr @imm8 => "%s " simd[%s] "," hex($imm8) ;' % (op, d, mn, SR))

# --- misc SSE2/SSE3 (C2-C6, D0, 7C/7D, D6, D7, E6/E7, F7, F0) --------------
emit('0x0f 0xc2 11 ggg rrr @imm8 => "cmp" elt[$reptype*2+$opsiz] " " xreg[%s] "," xreg[%s] "," hex($imm8) ;' % (G, Rr))
emit('0x0f 0xc2 @addr @imm8 => "cmp" elt[$reptype*2+$opsiz] " " xreg[%s] "," $addr "," hex($imm8) ;' % G)
emit('0x0f 0xc3 @addr => "movnti " $addr "," greg[32*$rexw+%s] ;' % G)
emit('0x0f 0xc4 11 ggg rrr @imm8 => "pinsrw " simd[%s] "," greg[32*$rexw+%s] "," hex($imm8) ;' % (SG, Rr))
emit('0x0f 0xc4 @addr @imm8 => "pinsrw " simd[%s] "," $addr "," hex($imm8) ;' % SG)
emit('0x0f 0xc5 11 ggg rrr @imm8 => "pextrw " greg[32*$rexw+%s] "," simd[%s] "," hex($imm8) ;' % (G, SR))
emit('0x0f 0xc6 11 ggg rrr @imm8 => "shuf" elt[$opsiz] " " xreg[%s] "," xreg[%s] "," hex($imm8) ;' % (G, Rr))
emit('0x0f 0xc6 @addr @imm8 => "shuf" elt[$opsiz] " " xreg[%s] "," $addr "," hex($imm8) ;' % G)
emit('0x0f 0xd0 11 ggg rrr => d0t[$reptype*2+$opsiz] " " xreg[%s] "," xreg[%s] ;' % (G, Rr))
emit('0x0f 0xd0 @addr      => d0t[$reptype*2+$opsiz] " " xreg[%s] "," $addr ;' % G)
emit('0x0f 0x7c 11 ggg rrr => h7c[$reptype*2+$opsiz] " " xreg[%s] "," xreg[%s] ;' % (G, Rr))
emit('0x0f 0x7c @addr      => h7c[$reptype*2+$opsiz] " " xreg[%s] "," $addr ;' % G)
emit('0x0f 0x7d 11 ggg rrr => h7d[$reptype*2+$opsiz] " " xreg[%s] "," xreg[%s] ;' % (G, Rr))
emit('0x0f 0x7d @addr      => h7d[$reptype*2+$opsiz] " " xreg[%s] "," $addr ;' % G)
emit('0x0f 0xd6 11 ggg rrr [$reptype==1] => "movq2dq " xreg[%s] "," mmreg[$r] ;' % G)
emit('0x0f 0xd6 11 ggg rrr [$reptype==2] => "movdq2q " mmreg[$g] "," xreg[%s] ;' % Rr)
emit('0x0f 0xd6 11 ggg rrr => "movq " xreg[%s] "," xreg[%s] wit("alt") ;' % (Rr, G))
emit('0x0f 0xd6 @addr      => "movq " $addr "," xreg[%s] ;' % G)
emit('0x0f 0xd7 11 ggg rrr => "pmovmskb " greg[32*$rexw+%s] "," simd[%s] ;' % (G, SR))
emit('0x0f 0xe6 11 ggg rrr => me6[$reptype*2+$opsiz] " " xreg[%s] "," xreg[%s] ;' % (G, Rr))
emit('0x0f 0xe6 @addr      => me6[$reptype*2+$opsiz] " " xreg[%s] "," $addr ;' % G)
emit('0x0f 0xe7 @addr      => me7[$opsiz] " " $addr "," simd[%s] ;' % SG)
emit('0x0f 0xf7 11 ggg rrr => mf7[$opsiz] " " simd[%s] "," simd[%s] ;' % (SG, SR))
emit('0x0f 0xf0 @addr [$reptype==2] => "lddqu " xreg[%s] "," $addr ;' % G)

print("\n".join(out))
