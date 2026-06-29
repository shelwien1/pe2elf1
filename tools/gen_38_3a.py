#!/usr/bin/env python3
# gen_38_3a.py -- emit x86-64 0F38 (SSSE3/SSE4/AES/SHA) and 0F3A (imm8) rules.
SG = "8*$opsiz+16*$rexr+$g"   # mm/xmm reg field
SR = "8*$opsiz+16*$rexb+$r"   # mm/xmm rm field
XG = "8*$rexr+$g"             # xmm-only reg field
XR = "8*$rexb+$r"             # xmm-only rm field
GR = "32*$rexw+8*$rexb+$r"    # GP rm field (32/64 by REX.W)
out = []
def emit(s): out.append("  " + s)

# ---- 0F38 : mm/xmm-by-66 two-operand ops (SSSE3 + SSE4) ----
ops38 = {
  0x00: "pshufb", 0x01: "phaddw", 0x02: "phaddd", 0x03: "phaddsw",
  0x04: "pmaddubsw", 0x05: "phsubw", 0x06: "phsubd", 0x07: "phsubsw",
  0x08: "psignb", 0x09: "psignw", 0x0a: "psignd", 0x0b: "pmulhrsw",
  0x1c: "pabsb", 0x1d: "pabsw", 0x1e: "pabsd",
  0x10: "pblendvb", 0x14: "blendvps", 0x15: "blendvpd", 0x17: "ptest",
  0x20: "pmovsxbw", 0x21: "pmovsxbd", 0x22: "pmovsxbq", 0x23: "pmovsxwd",
  0x24: "pmovsxwq", 0x25: "pmovsxdq", 0x28: "pmuldq", 0x29: "pcmpeqq",
  0x2b: "packusdw", 0x30: "pmovzxbw", 0x31: "pmovzxbd", 0x32: "pmovzxbq",
  0x33: "pmovzxwd", 0x34: "pmovzxwq", 0x35: "pmovzxdq", 0x37: "pcmpgtq",
  0x38: "pminsb", 0x39: "pminsd", 0x3a: "pminuw", 0x3b: "pminud",
  0x3c: "pmaxsb", 0x3d: "pmaxsd", 0x3e: "pmaxuw", 0x3f: "pmaxud",
  0x40: "pmulld", 0x41: "phminposuw",
}
for op in sorted(ops38):
  mn = ops38[op]
  emit('0x0f 0x38 0x%02x 11 ggg rrr => "%s " simd[%s] "," simd[%s] ;' % (op, mn, SG, SR))
  emit('0x0f 0x38 0x%02x @addr      => "%s " simd[%s] "," $addr ;' % (op, mn, SG))
emit('0x0f 0x38 0x2a @addr      => "movntdqa " simd[%s] "," $addr ;' % SG)

# ---- 0F38 : AES (always xmm) ----
for op, mn in [(0xdb, "aesimc"), (0xdc, "aesenc"), (0xdd, "aesenclast"),
               (0xde, "aesdec"), (0xdf, "aesdeclast")]:
  emit('0x0f 0x38 0x%02x 11 ggg rrr => "%s " xreg[%s] "," xreg[%s] ;' % (op, mn, XG, XR))
  emit('0x0f 0x38 0x%02x @addr      => "%s " xreg[%s] "," $addr ;' % (op, mn, XG))

# ---- 0F38 : SHA (always xmm) ----
for op, mn in [(0xc8, "sha1nexte"), (0xc9, "sha1msg1"), (0xca, "sha1msg2"),
               (0xcb, "sha256rnds2"), (0xcc, "sha256msg1"), (0xcd, "sha256msg2")]:
  emit('0x0f 0x38 0x%02x 11 ggg rrr => "%s " xreg[%s] "," xreg[%s] ;' % (op, mn, XG, XR))
  emit('0x0f 0x38 0x%02x @addr      => "%s " xreg[%s] "," $addr ;' % (op, mn, XG))

# ---- 0F38 : GP-interfacing (crc32, movbe, adcx/adox) ----
emit('0x0f 0x38 0xf0 11 ggg rrr [$reptype==2] => "crc32" sfx[1] " " greg[32*$rexw+8*$rexr+$g] "," rgb[16*$rex+%s] ;' % XR)
emit('0x0f 0x38 0xf0 @addr      [$reptype==2] => "crc32" sfx[1] " " greg[32*$rexw+8*$rexr+$g] "," $addr ;')
emit('0x0f 0x38 0xf1 11 ggg rrr [$reptype==2] => "crc32 " greg[32*$rexw+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+%s] ;' % XR)
emit('0x0f 0x38 0xf1 @addr      [$reptype==2] => "crc32" sfx[$rexw? 8 : (4>>$opsiz)] " " greg[32*$rexw+8*$rexr+$g] "," $addr ;')
emit('0x0f 0x38 0xf6 11 ggg rrr [$reptype==1] => "adox " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+%s] ;' % XR)
emit('0x0f 0x38 0xf6 11 ggg rrr [$opsiz] => "adcx " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+%s] ;' % XR)
emit('0x0f 0x38 0xf0 11 ggg rrr => "movbe " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+%s] ;' % XR)
emit('0x0f 0x38 0xf0 @addr      => "movbe " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;')
emit('0x0f 0x38 0xf1 @addr      => "movbe " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;')

# ---- 0F3A : imm8 ops, mm/xmm-by-66 ----
ops3a = {
  0x08: "roundps", 0x09: "roundpd", 0x0a: "roundss", 0x0b: "roundsd",
  0x0c: "blendps", 0x0d: "blendpd", 0x0e: "pblendw", 0x0f: "palignr",
  0x21: "insertps", 0x40: "dpps", 0x41: "dppd", 0x42: "mpsadbw",
  0x44: "pclmulqdq", 0x60: "pcmpestrm", 0x61: "pcmpestri",
  0x62: "pcmpistrm", 0x63: "pcmpistri", 0xdf: "aeskeygenassist",
}
for op in sorted(ops3a):
  mn = ops3a[op]
  emit('0x0f 0x3a 0x%02x 11 ggg rrr @imm8 => "%s " simd[%s] "," simd[%s] "," hex($imm8) ;' % (op, mn, SG, SR))
  emit('0x0f 0x3a 0x%02x @addr      @imm8 => "%s " simd[%s] "," $addr "," hex($imm8) ;' % (op, mn, SG))
emit('0x0f 0x3a 0xcc 11 ggg rrr @imm8 => "sha1rnds4 " xreg[%s] "," xreg[%s] "," hex($imm8) ;' % (XG, XR))
emit('0x0f 0x3a 0xcc @addr      @imm8 => "sha1rnds4 " xreg[%s] "," $addr "," hex($imm8) ;' % XG)

# ---- 0F3A : extract/insert (GP-interfacing) ----
# pextrb r/m8, xmm ; pextrw r/m16 ; pextrd/pextrq r/m32/64 ; extractps r/m32
emit('0x0f 0x3a 0x14 11 ggg rrr @imm8 => "pextrb " greg[%s] "," xreg[%s] "," hex($imm8) ;' % (GR, XG))
emit('0x0f 0x3a 0x14 @addr      @imm8 => "pextrb" sfx[1] " " $addr "," xreg[%s] "," hex($imm8) ;' % XG)
emit('0x0f 0x3a 0x15 11 ggg rrr @imm8 => "pextrw " greg[%s] "," xreg[%s] "," hex($imm8) wit("long") ;' % (GR, XG))
emit('0x0f 0x3a 0x15 @addr      @imm8 => "pextrw" sfx[2] " " $addr "," xreg[%s] "," hex($imm8) ;' % XG)
emit('0x0f 0x3a 0x16 11 ggg rrr @imm8 => pextrdq[$rexw] " " greg[%s] "," xreg[%s] "," hex($imm8) ;' % (GR, XG))
emit('0x0f 0x3a 0x16 @addr      @imm8 => pextrdq[$rexw] " " $addr "," xreg[%s] "," hex($imm8) ;' % XG)
emit('0x0f 0x3a 0x17 11 ggg rrr @imm8 => "extractps " greg[%s] "," xreg[%s] "," hex($imm8) ;' % (GR, XG))
emit('0x0f 0x3a 0x17 @addr      @imm8 => "extractps " $addr "," xreg[%s] "," hex($imm8) ;' % XG)
emit('0x0f 0x3a 0x20 11 ggg rrr @imm8 => "pinsrb " xreg[%s] "," greg[%s] "," hex($imm8) ;' % (XG, GR))
emit('0x0f 0x3a 0x20 @addr      @imm8 => "pinsrb " xreg[%s] "," $addr "," hex($imm8) ;' % XG)
emit('0x0f 0x3a 0x22 11 ggg rrr @imm8 => pinsrdq[$rexw] " " xreg[%s] "," greg[%s] "," hex($imm8) ;' % (XG, GR))
emit('0x0f 0x3a 0x22 @addr      @imm8 => pinsrdq[$rexw] " " xreg[%s] "," $addr "," hex($imm8) ;' % XG)

print("\n".join(out))
