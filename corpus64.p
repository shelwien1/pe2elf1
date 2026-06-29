# corpus64.p (v1, x86-64 long-mode disassembly) -- bit-pattern -> string rewriter.
#
# x86-64 companion to corpus.p.  Same engine (parsergen.py to disassemble,
# asm.py to assemble), same conventions (size-paired register tables, mod=11 +
# @addr per ModR/M op, prefix-indexed mnemonic tables, inline segment display,
# wit() annotations for non-canonical encodings).  The long-mode additions over
# the 32-bit file are:
#
#   * REX prefix (0x40..0x4F = 0100 WRXB).  Handled as a prefix frame that sets
#     $rexw/$rexr/$rexx/$rexb/$rex and emits NO token: its bits are recovered
#     directly from the register names (a 64-bit name reveals W, an r8..r15 name
#     reveals the R/X/B extension, an spl/bpl/sil/dil name reveals REX presence),
#     exactly as the 66 prefix is recovered from a 16-bit register in corpus.p.
#     The assembler re-derives the REX byte from the operands (see asm.py).
#   * Register files widened to 16 (r8..r15) and to 64-bit (rax..r15).  The GP
#     index is the clean bit-pack 32*W + 16*opsiz + 8*hi + lo so the assembler's
#     reverse solver can invert it; `hi` is REX.R for a reg field, REX.B for an
#     rm / base / opcode-reg field, REX.X for a SIB index.
#   * 64-bit addressing: mod=00 rm=101 is RIP-relative ([rip+disp32]); absolute
#     [disp32] is the SIB base=101 index=none form; bases/indexes are rax..r15
#     (REX.B/REX.X extended).  The 0x67 form switches to 32-bit addressing.
#   * Default-64 operand size for the stack / near-branch group (push/pop/call/
#     jmp/Jcc/...): 64-bit unless 66, REX.W ignored -- a separate `dreg` table.
#   * movabs (B8+r under REX.W: a true 8-byte imm64); movsxd (0x63); the 1-byte
#     inc/dec (0x40..0x4F) and the removed legacy opcodes are gone (their bytes
#     are REX / new meanings in long mode).

arch  $mode=64 $endian=le $bitorder=msb $maxlen=15
vars  $opsiz=0 $adrsiz=0 $reptype=0 $segidx=0 $lock=0 $rexw=0 $rexr=0 $rexx=0 $rexb=0 $rex=0

# ===========================================================================
# register / addressing tables
# ===========================================================================
# GP registers, W-sensitive ops.  index = 32*$rexw + 16*$opsiz + 8*hi + lo,
# hi = REX.{R|B|X}, lo = the 3-bit field.  Four 16-register banks:
#   rexw=0 opsiz=0 : 32-bit    rexw=0 opsiz=1 : 16-bit
#   rexw=1 opsiz=0 : 64-bit    rexw=1 opsiz=1 : 64-bit (redundant 66)
table greg {
  eax,ecx,edx,ebx,esp,ebp,esi,edi,r8d,r9d,r10d,r11d,r12d,r13d,r14d,r15d,
  ax,cx,dx,bx,sp,bp,si,di,r8w,r9w,r10w,r11w,r12w,r13w,r14w,r15w,
  rax,rcx,rdx,rbx,rsp,rbp,rsi,rdi,r8,r9,r10,r11,r12,r13,r14,r15,
  rax,rcx,rdx,rbx,rsp,rbp,rsi,rdi,r8,r9,r10,r11,r12,r13,r14,r15
}
# Default-64 GP (stack / near branches): 64-bit unless 66->16; REX.W ignored.
# index = 16*$opsiz + 8*hi + lo.
table dreg {
  rax,rcx,rdx,rbx,rsp,rbp,rsi,rdi,r8,r9,r10,r11,r12,r13,r14,r15,
  ax,cx,dx,bx,sp,bp,si,di,r8w,r9w,r10w,r11w,r12w,r13w,r14w,r15w
}
# 8-bit GP.  index = 16*$rex + 8*hi + lo.  Without REX: ah/ch/dh/bh at 4..7;
# with REX: spl/bpl/sil/dil and r8b..r15b.  The rex=0,hi=1 bank is unreachable.
table rgb {
  al,cl,dl,bl,ah,ch,dh,bh,
  "","","","","","","","",
  al,cl,dl,bl,spl,bpl,sil,dil,
  r8b,r9b,r10b,r11b,r12b,r13b,r14b,r15b
}
# Addressing-base registers: always full width regardless of opsiz/REX.W.
table areg   { rax,rcx,rdx,rbx,rsp,rbp,rsi,rdi,r8,r9,r10,r11,r12,r13,r14,r15 }   # adrsiz=0
table areg32 { eax,ecx,edx,ebx,esp,ebp,esi,edi,r8d,r9d,r10d,r11d,r12d,r13d,r14d,r15d } # adrsiz=1
# MMX (never extended) and XMM (REX-extended, 16 regs).
table mmreg  { mm0,mm1,mm2,mm3,mm4,mm5,mm6,mm7 }
table xreg   { xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7,xmm8,xmm9,xmm10,xmm11,xmm12,xmm13,xmm14,xmm15 }
# MMX/XMM merged for the 66-split integer ops: index 8*$opsiz + 16*REX.{R|B} + f
# selects mm0..7 (no 66) or xmm0..15 (66 -> xmm).  The REX high bit is weight 16
# so opsiz and the REX bit stay separable by the assembler's solver; the
# opsiz=0 + REX-bit=1 bank (16..23) is unreachable (REX on an MMX form).
table simd {
  mm0,mm1,mm2,mm3,mm4,mm5,mm6,mm7,
  xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7,
  "","","","","","","","",
  xmm8,xmm9,xmm10,xmm11,xmm12,xmm13,xmm14,xmm15
}
table sreg   { es,cs,ss,ds,fs,gs }
table cond   { o,no,b,ae,e,ne,be,a,s,ns,p,np,l,ge,le,g }
table crreg  { cr0,cr1,cr2,cr3,cr4,cr5,cr6,cr7,cr8,cr9,cr10,cr11,cr12,cr13,cr14,cr15 }
table drreg  { dr0,dr1,dr2,dr3,dr4,dr5,dr6,dr7,dr8,dr9,dr10,dr11,dr12,dr13,dr14,dr15 }

# size suffix : the dot is part of the entry; index = operand size in bytes.
table sfx    { "",".b",".w","",".d","","","",".q","",".t","","","","","","" }

# segment display : index = sbo(base)+segidx.  ss is the default for rsp/rbp
# (and r12/r13, same SIB base encoding); ds elsewhere.
table seg    { "","es:","cs:","ss:","","fs:","gs:",  "ss:","es:","cs:","ss:","ds:","fs:","gs:" }
table sbo    { 0,0,0,0,7,7,0,0, 0,0,0,0,7,7,0,0 }
table sc     { 1, 2, 4, 8 }

# prefix-indexed / op-indexed mnemonic tables
table alu    { add,or,adc,sbb,and,sub,xor,cmp }
table shift  { rol,ror,rcl,rcr,shl,shr,shl,sar }
table grp3   { test,test,not,neg,mul,imul,div,idiv }
table movs   { movsd,movsw,movsq }
table stos   { stosd,stosw,stosq }
table lods   { lodsd,lodsw,lodsq }
table cmps   { cmpsd,cmpsw,cmpsq }
table scas   { scasd,scasw,scasq }
table insx   { insd,insw }
table outsx  { outsd,outsw }
table cbw_t  { cwde,cbw,cdqe }         # 0x98 by ($rexw?2:$opsiz): cwde/cbw/cdqe
table cwd_t  { cdq,cwd,cqo }           # 0x99 by ($rexw?2:$opsiz): cdq/cwd/cqo
table d8r    { fadd,fmul,fcom,fcomp,fsub,fsubr,fdiv,fdivr }
table dcr    { fadd,fmul,fcom,fcomp,fsubr,fsub,fdivr,fdiv }
table dar    { fcmovb,fcmove,fcmovbe,fcmovu }
table dbr    { fcmovnb,fcmovne,fcmovnbe,fcmovnu }
table der    { faddp,fmulp,fcompp,fcompp,fsubrp,fsubp,fdivrp,fdivp }
table pause_t { "",pause,"" }          # F3 90 (pause absorbs the F3)
table pcnt   { "",popcnt,"" }          # F3 0F B8
table tzt    { "",tzcnt,"" }           # F3 0F BC
table lzt    { "",lzcnt,"" }           # F3 0F BD
table grpba  { "","","","",bt,bts,btr,btc }   # 0F BA /op
table c7r7   { rdseed,rdpid,rdseed,rdseed }   # 0F C7 /7 by reptype
table c7r6   { rdrand,rdrand,vmxon,rdrand }   # 0F C7 /6 reg by reptype
table cx16   { cmpxchg8b, cmpxchg16b }        # 0F C7 /1 by REX.W (m64 / m128)
table e1efa  { "",endbr64,"" }                 # F3 0F 1E FA
table e1efb  { "",endbr32,"" }                 # F3 0F 1E FB

# ---- SSE / MMX mnemonic tables (indexed by reptype*2+opsiz or opsiz) ----
table elt    { ps,pd,ss,ss,sd,sd }     # packed/scalar element by reptype*2+opsiz
table uc     { ucomiss,ucomisd }       # 0F 2E by opsiz
table comi   { comiss,comisd }         # 0F 2F by opsiz
table mova2  { movaps,movapd }         # 0F 28/29 by opsiz
table movu   { movups,movupd,movss,movss,movsd,movsd }  # 0F 10/11 reptype*2+opsiz
table movusz { 16,16,4,4,8,8 }         # 0F 10/11 mem size
table mnt4   { movntps,movntpd,movntss,movntss,movntsd,movntsd } # 0F 2B
table ml12   { movlps,movlpd }         # 0F 12/13 mem by opsiz
table mh16   { movhps,movhpd }         # 0F 16/17 mem by opsiz
table m12f   { "",movsldup,movddup }   # 0F 12 by reptype
table m16f   { "",movshdup,"" }        # 0F 16 by reptype
table unpl   { unpcklps,unpcklpd }     # 0F 14 by opsiz
table unph   { unpckhps,unpckhpd }     # 0F 15 by opsiz
table mdq    { movq,movdqa,movdqu }    # 0F 6F/7F reptype*2+opsiz (idx 0/1/2)
table mdq2   { movd,movq }             # 0F 6E/7E GP move by REX.W
table cvt2a  { cvtpi2ps,cvtpi2pd,cvtsi2ss,cvtsi2ss,cvtsi2sd,cvtsi2sd } # 0F 2A
table cvt2c  { cvttps2pi,cvttpd2pi,cvttss2si,cvttss2si,cvttsd2si,cvttsd2si } # 0F 2C
table cvt2d  { cvtps2pi,cvtpd2pi,cvtss2si,cvtss2si,cvtsd2si,cvtsd2si } # 0F 2D
table cvt5a  { cvtps2pd,cvtpd2ps,cvtss2sd,cvtss2sd,cvtsd2ss,cvtsd2ss } # 0F 5A
table cvt5b  { cvtdq2ps,cvtps2dq,cvttps2dq,cvttps2dq } # 0F 5B
table me6    { "",cvttpd2dq,cvtdq2pd,"",cvtpd2dq,"" } # 0F E6
table me7    { movntq,movntdq }        # 0F E7 by opsiz
table mf7    { maskmovq,maskmovdqu }   # 0F F7 by opsiz
table md6    { "",movq2dq,movdq2q }    # 0F D6 reptype (F3/F2)
table d0t    { "",addsubpd,"","",addsubps,"" } # 0F D0
table h7c    { "",haddpd,"","",haddps,"" }      # 0F 7C
table h7d    { "",hsubpd,"","",hsubps,"" }      # 0F 7D
table pshuf  { pshufw,pshufd,pshufhw,pshufhw,pshuflw,pshuflw } # 0F 70
table m7e    { "",movq }               # 0F 7E F3 (reload)
table ldqt   { "","",lddqu }           # F2 0F F0
table pextrdq { pextrd, pextrq }       # 0F3A 16 by REX.W
table pinsrdq { pinsrd, pinsrq }       # 0F3A 22 by REX.W

# ---- VEX/AVX register files (REX/VEX bits stored inverted in the prefix) ----
# vreg : index 16*L + 8*(R-bar|B-bar) + field.  L=0 xmm, L=1 ymm.  The high bit
# is the INVERTED VEX.R/B, so ext=0 -> xmm8..15, ext=1 -> xmm0..7.
table vreg {
  xmm8,xmm9,xmm10,xmm11,xmm12,xmm13,xmm14,xmm15, xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7,
  ymm8,ymm9,ymm10,ymm11,ymm12,ymm13,ymm14,ymm15, ymm0,ymm1,ymm2,ymm3,ymm4,ymm5,ymm6,ymm7
}
# vvvv source: index 16*L + vvvv (vvvv stored inverted: 0000->reg15 .. 1111->reg0)
table vvv {
  xmm15,xmm14,xmm13,xmm12,xmm11,xmm10,xmm9,xmm8, xmm7,xmm6,xmm5,xmm4,xmm3,xmm2,xmm1,xmm0,
  ymm15,ymm14,ymm13,ymm12,ymm11,ymm10,ymm9,ymm8, ymm7,ymm6,ymm5,ymm4,ymm3,ymm2,ymm1,ymm0
}
table velt { ps,pd,ss,sd }            # VEX pp -> element suffix
table kreg  { k0,k1,k2,k3,k4,k5,k6,k7 }   # AVX-512 mask registers
table kmov_t { kmovw,kmovb,"",kmovd }      # k-move width by pp (VEX W0)
table klog_t { kandw,kandb }               # placeholder (klog uses explicit mnemonics)

# ===========================================================================
# EVEX / AVX-512 register & decoration tables
# ===========================================================================
# 32 registers (zmm0-31) selected by the INVERTED prefix bits, laid out so the
# assembler's positional solver inverts them with positive coefficients.
#   reg     : ereg[32*L'L + 16*R-bar' + 8*R-bar + modrm.reg]
#   rm(reg) : ereg[32*L'L + 16*X-bar  + 8*B-bar + modrm.rm ]
#   vvvv    : evvv[32*L'L + 16*V-bar' + vvvv]
# L'L: 0=xmm 1=ymm 2=zmm.
table ereg {
  xmm24, xmm25, xmm26, xmm27, xmm28, xmm29, xmm30, xmm31, xmm16, xmm17, xmm18, xmm19, xmm20, xmm21, xmm22, xmm23, xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7,
  ymm24, ymm25, ymm26, ymm27, ymm28, ymm29, ymm30, ymm31, ymm16, ymm17, ymm18, ymm19, ymm20, ymm21, ymm22, ymm23, ymm8, ymm9, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15, ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7,
  zmm24, zmm25, zmm26, zmm27, zmm28, zmm29, zmm30, zmm31, zmm16, zmm17, zmm18, zmm19, zmm20, zmm21, zmm22, zmm23, zmm8, zmm9, zmm10, zmm11, zmm12, zmm13, zmm14, zmm15, zmm0, zmm1, zmm2, zmm3, zmm4, zmm5, zmm6, zmm7
}
table evvv {
  xmm31, xmm30, xmm29, xmm28, xmm27, xmm26, xmm25, xmm24, xmm23, xmm22, xmm21, xmm20, xmm19, xmm18, xmm17, xmm16, xmm15, xmm14, xmm13, xmm12, xmm11, xmm10, xmm9, xmm8, xmm7, xmm6, xmm5, xmm4, xmm3, xmm2, xmm1, xmm0,
  ymm31, ymm30, ymm29, ymm28, ymm27, ymm26, ymm25, ymm24, ymm23, ymm22, ymm21, ymm20, ymm19, ymm18, ymm17, ymm16, ymm15, ymm14, ymm13, ymm12, ymm11, ymm10, ymm9, ymm8, ymm7, ymm6, ymm5, ymm4, ymm3, ymm2, ymm1, ymm0,
  zmm31, zmm30, zmm29, zmm28, zmm27, zmm26, zmm25, zmm24, zmm23, zmm22, zmm21, zmm20, zmm19, zmm18, zmm17, zmm16, zmm15, zmm14, zmm13, zmm12, zmm11, zmm10, zmm9, zmm8, zmm7, zmm6, zmm5, zmm4, zmm3, zmm2, zmm1, zmm0
}
# mask {k1..k7} + zeroing {z}: index z*8 + aaa
table kzdec { "", " {k1}", " {k2}", " {k3}", " {k4}", " {k5}", " {k6}", " {k7}", "", " {k1} {z}", " {k2} {z}", " {k3} {z}", " {k4} {z}", " {k5} {z}", " {k6} {z}", " {k7} {z}" }
# mask only (no zeroing form): index aaa
table kdec  { "", " {k1}", " {k2}", " {k3}", " {k4}", " {k5}", " {k6}", " {k7}" }
# broadcast suffix by L'L (32-/64-bit element)
table bcst32 { " {1to4}", " {1to8}", " {1to16}", "" }
table bcst64 { " {1to2}", " {1to4}", " {1to8}", "" }
# embedded-rounding suffix by L'L (b=1, reg-reg)
table rcdec { " {rn-sae}", " {rd-sae}", " {ru-sae}", " {rz-sae}" }
table vmova { vmovaps,vmovapd,"","" }   # VEX 0F 28/29 by pp
table vmovu { vmovups,vmovupd,"","" }   # VEX 0F 10/11 (packed) by pp
table vmdq  { "",vmovdqa,vmovdqu,"" }   # VEX 0F 6F/7F by pp

# ===========================================================================
# immediates / displacements
# ===========================================================================
submatch imm8  { iiiiiiii                              => $i }
submatch imm16 { iiiiiiii iiiiiiii                     => $i }
submatch imm32 { iiiiiiii iiiiiiii iiiiiiii iiiiiiii   => $i }
submatch imm64 { iiiiiiii iiiiiiii iiiiiiii iiiiiiii iiiiiiii iiiiiiii iiiiiiii iiiiiiii => $i }
submatch disp8 { dddddddd                              => sx8($d) }
submatch disp32{ dddddddd dddddddd dddddddd dddddddd   => $d }
submatch rel8  { dddddddd                              => $E + sx8($d) }
# "z" immediate: 16-bit under 66, else 32-bit.  REX.W keeps it 32-bit (the 66 is
# ignored once W is set), so the dispatch is ($rexw ? 0 : $opsiz).
submatch immz1($opsiz) { <0> @imm32 => $imm32 ;  <1> @imm16 => $imm16 }
submatch immz { @immz1($rexw ? 0 : $opsiz) => $immz1 }
submatch relz1($opsiz) { <0> @imm32 => $E+sx32($imm32) ;  <1> @imm16 => $E+sx16($imm16) }
submatch relz { @relz1($opsiz) => $relz1 }
# moffs absolute address: 64-bit unless 0x67 (then 32-bit).
submatch immadr1($adrsiz) { <0> @imm64 => $imm64 ;  <1> @imm32 => $imm32 }
submatch immadr { @immadr1($adrsiz) => $immadr1 }

# ===========================================================================
# prefix run : legacy prefixes recurse through @pfx; REX goes straight to @insn
# (it must be the last prefix before the opcode).  REX emits no token.
# ===========================================================================
submatch pfx($d) {
  [$d >= $maxlen] => "" ;
  0x66 @pfx($d+1) {$opsiz=1}   => "opsiz "  $pfx ;
  0x67 @pfx($d+1) {$adrsiz=1}  => "adrsiz " $pfx ;
  0x26 @pfx($d+1) {$segidx=1}  => "seges "  $pfx ;
  0x2e @pfx($d+1) {$segidx=2}  => "segcs "  $pfx ;
  0x36 @pfx($d+1) {$segidx=3}  => "segss "  $pfx ;
  0x3e @pfx($d+1) {$segidx=4}  => "segds "  $pfx ;
  0x64 @pfx($d+1) {$segidx=5}  => "segfs "  $pfx ;
  0x65 @pfx($d+1) {$segidx=6}  => "seggs "  $pfx ;
  0xf3 @pfx($d+1) {$reptype=1} => "rep "    $pfx ;
  0xf2 @pfx($d+1) {$reptype=2} => "repnz "  $pfx ;
  0xf0 @pfx($d+1) {$lock=1}    => "lock "   $pfx ;
  0100 wrxb {$rexw=$w; $rexr=$r; $rexx=$x; $rexb=$b; $rex=1} @insn => $insn ;
  @insn => $insn ;
}

# ===========================================================================
# addressing : MEMORY ONLY (mod != 11).  addr() injects current adrsiz.
# ===========================================================================
submatch addr($g) { @addr1($adrsiz, $g) => $addr1 }
submatch addr1($adrsiz, $g) {
  # ---- 64-bit addressing (adrsiz = 0) ----
  <0> 00 ggg 100 @sib0          => seg[sbo[$sbase]+$segidx] $sib0 ;
  <0> 00 ggg 101 @disp32        => seg[$segidx] "[rip" sgn($disp32) "]" ;
  <0> 00 ggg rrr                => seg[sbo[8*$rexb+$r]+$segidx] "[" areg[8*$rexb+$r] "]" ;
  <0> 01 ggg 100 @sib1 @disp8   => wit($disp8==0 ? ($sbase!=5 ? "disp8" : "") : "") seg[sbo[$sbase]+$segidx] "[" $sib1 sgn($disp8) "]" ;
  <0> 01 ggg rrr @disp8         => wit($disp8==0 ? "disp8" : "") seg[sbo[8*$rexb+$r]+$segidx] "[" areg[8*$rexb+$r] sgn($disp8) "]" ;
  <0> 10 ggg 100 @sib1 @disp32  => wit(sx32($disp32)>=-128 ? (sx32($disp32)<=127 ? "disp32" : "") : "") seg[sbo[$sbase]+$segidx] "[" $sib1 sgn($disp32) "]" ;
  <0> 10 ggg rrr @disp32        => wit(sx32($disp32)>=-128 ? (sx32($disp32)<=127 ? "disp32" : "") : "") seg[sbo[8*$rexb+$r]+$segidx] "[" areg[8*$rexb+$r] sgn($disp32) "]" ;
  # ---- 32-bit addressing (adrsiz = 1, the 0x67 form); mod=00 rm=101 = EIP-rel ----
  <1> 00 ggg 100 @sib0          => seg[sbo[$sbase]+$segidx] $sib0 ;
  <1> 00 ggg 101 @disp32        => seg[$segidx] "[eip" sgn($disp32) "]" ;
  <1> 00 ggg rrr                => seg[sbo[8*$rexb+$r]+$segidx] "[" areg32[8*$rexb+$r] "]" ;
  <1> 01 ggg 100 @sib1 @disp8   => wit($disp8==0 ? ($sbase!=5 ? "disp8" : "") : "") seg[sbo[$sbase]+$segidx] "[" $sib1 sgn($disp8) "]" ;
  <1> 01 ggg rrr @disp8         => wit($disp8==0 ? "disp8" : "") seg[sbo[8*$rexb+$r]+$segidx] "[" areg32[8*$rexb+$r] sgn($disp8) "]" ;
  <1> 10 ggg 100 @sib1 @disp32  => wit(sx32($disp32)>=-128 ? (sx32($disp32)<=127 ? "disp32" : "") : "") seg[sbo[$sbase]+$segidx] "[" $sib1 sgn($disp32) "]" ;
  <1> 10 ggg rrr @disp32        => wit(sx32($disp32)>=-128 ? (sx32($disp32)<=127 ? "disp32" : "") : "") seg[sbo[8*$rexb+$r]+$segidx] "[" areg32[8*$rexb+$r] sgn($disp32) "]" ;
}
# SIB, mod=00.  No-index requires REX.X=0 & index=100; no-base is base=101.
submatch sib0 {
  ss 100 101 @disp32 [$rexx==0] {$sbase=none} => "[" hex($disp32) "]" ;
  ss iii 101 @disp32 {$sbase=none}            => "[" areg[8*$rexx+$i] "*" dec(sc[$s]) "+" hex($disp32) "]" ;
  ss 100 bbb [$rexx==0] {$sbase=8*$rexb+$b}   => "[" areg[8*$rexb+$b] "]" ;
  ss iii bbb {$sbase=8*$rexb+$b}              => "[" areg[8*$rexb+$b] "+" areg[8*$rexx+$i] "*" dec(sc[$s]) "]" ;
}
submatch sib1 {
  ss 100 bbb [$rexx==0] {$sbase=8*$rexb+$b} => areg[8*$rexb+$b] ;
  ss iii bbb {$sbase=8*$rexb+$b}            => areg[8*$rexb+$b] "+" areg[8*$rexx+$i] "*" dec(sc[$s]) ;
}

# ===========================================================================
# instruction decoder
# ===========================================================================
submatch insn {
  0xc4 @vex => $vex ;
  0xc5 @vex2 => $vex2 ;
  0x62 @evex => $evex ;
  # --- lea / mov family -----------------------------------------------------
  0x8d @addr => "lea " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x88 11 ggg rrr => "mov " rgb[16*$rex+8*$rexb+$r] "," rgb[16*$rex+8*$rexr+$g] ;
  0x88 @addr      => "mov" sfx[1] " " $addr "," rgb[16*$rex+8*$rexr+$g] ;
  0x89 11 ggg rrr => "mov " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x89 @addr      => "mov " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x8b 11 ggg rrr => wit("alt") "mov " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x8b @addr      => "mov " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x8a 11 ggg rrr => wit("alt") "mov " rgb[16*$rex+8*$rexr+$g] "," rgb[16*$rex+8*$rexb+$r] ;
  0x8a @addr      => "mov" sfx[1] " " rgb[16*$rex+8*$rexr+$g] "," $addr ;
  0x8c 11 ggg rrr => "mov " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," sreg[$g] ;
  0x8c @addr      => "mov " $addr "," sreg[$g] ;
  0x8e 11 ggg rrr => "mov " sreg[$g] "," greg[16+8*$rexb+$r] ;
  0x8e @addr      => "mov " sreg[$g] "," $addr ;
  0xa0 @immadr => "mov al," seg[$segidx] "[@" hex($immadr) "]" ;
  0xa1 @immadr => "mov " greg[32*$rexw+16*$opsiz+0] "," seg[$segidx] "[@" hex($immadr) "]" ;
  0xa2 @immadr => "mov " seg[$segidx] "[@" hex($immadr) "],al" ;
  0xa3 @immadr => "mov " seg[$segidx] "[@" hex($immadr) "]," greg[32*$rexw+16*$opsiz+0] ;
  10110 bbb @imm8 => "mov " rgb[16*$rex+8*$rexb+$b] "," hex($imm8) ;
  10111 bbb @imm64 [$rexw==1] => "movabs " greg[32*$rexw+8*$rexb+$b] "," hex($imm64) ;
  10111 bbb @immz => "mov " greg[16*$opsiz+8*$rexb+$b] "," hex($immz) ;
  0xc6 11 000 rrr @imm8 => wit("long") "mov " rgb[16*$rex+8*$rexb+$r] "," hex($imm8) ;
  0xc6 @addr(0)   @imm8 => "mov" sfx[1] " " $addr "," hex($imm8) ;
  0xc7 11 000 rrr @immz => wit("long") "mov " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," hex($immz) ;
  0xc7 @addr(0)   @immz => "mov" sfx[$rexw? 8 : (4>>$opsiz)] " " $addr "," hex($immz) ;
  0x63 11 ggg rrr => "movsxd " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[8*$rexb+$r] ;
  0x63 @addr      => "movsxd " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;

  # --- ALU group (00..3D) : op table; Eb/Ev, both directions, AL/eAX imm -----
  00 fff 000 11 ggg rrr => alu[$f] " " rgb[16*$rex+8*$rexb+$r] "," rgb[16*$rex+8*$rexr+$g] ;
  00 fff 000 @addr      => alu[$f] sfx[1] " " $addr "," rgb[16*$rex+8*$rexr+$g] ;
  00 fff 010 11 ggg rrr => wit("alt") alu[$f] " " rgb[16*$rex+8*$rexr+$g] "," rgb[16*$rex+8*$rexb+$r] ;
  00 fff 010 @addr      => alu[$f] sfx[1] " " rgb[16*$rex+8*$rexr+$g] "," $addr ;
  00 fff 001 11 ggg rrr => alu[$f] " " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  00 fff 001 @addr      => alu[$f] " " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  00 fff 011 11 ggg rrr => wit("alt") alu[$f] " " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  00 fff 011 @addr      => alu[$f] " " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  00 fff 100 @imm8 => alu[$f] " al," hex($imm8) ;
  00 fff 101 @immz => alu[$f] " " greg[32*$rexw+16*$opsiz+0] "," hex($immz) wit(sx32($immz)>=-128 ? (sx32($immz)<=127 ? "imm32" : "") : "") ;

  # --- group 1 (80/81/83): 8 ALU ops via alu[$f] ----------------------------
  0x80 11 fff rrr @imm8 => wit($r==0 ? "long" : "") alu[$f] " " rgb[16*$rex+8*$rexb+$r] "," hex($imm8) ;
  0x80 @addr      @imm8 => alu[$g] sfx[1] " " $addr "," hex($imm8) ;
  0x81 11 fff rrr @immz => alu[$f] " " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," hex($immz) wit($r==0 ? "long" : "") wit(sx32($immz)>=-128 ? (sx32($immz)<=127 ? "imm32" : "") : "") ;
  0x81 @addr      @immz => alu[$g] sfx[$rexw? 8 : (4>>$opsiz)] " " $addr "," hex($immz) wit(sx32($immz)>=-128 ? (sx32($immz)<=127 ? "imm32" : "") : "") ;
  0x83 11 fff rrr @imm8 => alu[$f] " " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," hex(sx8($imm8)) ;
  0x83 @addr      @imm8 => alu[$g] sfx[$rexw? 8 : (4>>$opsiz)] " " $addr "," hex(sx8($imm8)) ;

  # --- inc/dec/call/jmp/push group FE/FF; unary F6/F7; shifts; test ---------
  0xff 11 000 rrr => "inc " greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0xff @addr(0)   => "inc" sfx[$rexw? 8 : (4>>$opsiz)] " " $addr ;
  0xff 11 001 rrr => "dec " greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0xff @addr(1)   => "dec" sfx[$rexw? 8 : (4>>$opsiz)] " " $addr ;
  0xff 11 010 rrr => "call " dreg[16*$opsiz+8*$rexb+$r] ;
  0xff @addr(2)   => "call" sfx[8] " " $addr ;
  0xff 11 100 rrr => "jmp " dreg[16*$opsiz+8*$rexb+$r] ;
  0xff @addr(4)   => "jmp" sfx[8] " " $addr ;
  0xff @addr(3)   => "call far" sfx[$rexw? 8 : (4>>$opsiz)] " " $addr ;
  0xff @addr(5)   => "jmp far" sfx[$rexw? 8 : (4>>$opsiz)] " " $addr ;
  0xff 11 110 rrr => "push " dreg[16*$opsiz+8*$rexb+$r] ;
  0xff @addr(6)   => "push" sfx[8] " " $addr ;
  0xfe 11 000 rrr => "inc " rgb[16*$rex+8*$rexb+$r] ;
  0xfe @addr(0)   => "inc" sfx[1] " " $addr ;
  0xfe 11 001 rrr => "dec " rgb[16*$rex+8*$rexb+$r] ;
  0xfe @addr(1)   => "dec" sfx[1] " " $addr ;
  0xf7 11 000 rrr @immz => wit($r==0 ? "long" : "") "test " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," hex($immz) ;
  0xf7 11 fff rrr => grp3[$f] " " greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0xf7 @addr(0) @immz => "test" sfx[$rexw? 8 : (4>>$opsiz)] " " $addr "," hex($immz) ;
  0xf7 @addr      => grp3[$g] sfx[$rexw? 8 : (4>>$opsiz)] " " $addr ;
  0xf6 11 000 rrr @imm8 => wit($r==0 ? "long" : "") "test " rgb[16*$rex+8*$rexb+$r] "," hex($imm8) ;
  0xf6 11 fff rrr => grp3[$f] " " rgb[16*$rex+8*$rexb+$r] ;
  0xf6 @addr(0) @imm8 => "test" sfx[1] " " $addr "," hex($imm8) ;
  0xf6 @addr      => grp3[$g] sfx[1] " " $addr ;
  0x84 11 ggg rrr => "test " rgb[16*$rex+8*$rexb+$r] "," rgb[16*$rex+8*$rexr+$g] ;
  0x84 @addr      => "test" sfx[1] " " $addr "," rgb[16*$rex+8*$rexr+$g] ;
  0x85 11 ggg rrr => "test " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x85 @addr      => "test " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0xa8 @imm8 => "test al," hex($imm8) ;
  0xa9 @immz => "test " greg[32*$rexw+16*$opsiz+0] "," hex($immz) ;
  0xc1 11 fff rrr @imm8 => shift[$f] " " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," hex($imm8) ;
  0xc1 @addr @imm8 => shift[$g] sfx[$rexw? 8 : (4>>$opsiz)] " " $addr "," hex($imm8) ;
  0xd1 11 fff rrr => shift[$f] " " greg[32*$rexw+16*$opsiz+8*$rexb+$r] ",1" ;
  0xd1 @addr => shift[$g] sfx[$rexw? 8 : (4>>$opsiz)] " " $addr ",1" ;
  0xd3 11 fff rrr => shift[$f] " " greg[32*$rexw+16*$opsiz+8*$rexb+$r] ",cl" ;
  0xd3 @addr => shift[$g] sfx[$rexw? 8 : (4>>$opsiz)] " " $addr ",cl" ;
  0xc0 11 fff rrr @imm8 => shift[$f] " " rgb[16*$rex+8*$rexb+$r] "," hex($imm8) ;
  0xc0 @addr @imm8 => shift[$g] sfx[1] " " $addr "," hex($imm8) ;
  0xd0 11 fff rrr => shift[$f] " " rgb[16*$rex+8*$rexb+$r] ",1" ;
  0xd0 @addr => shift[$g] sfx[1] " " $addr ",1" ;
  0xd2 11 fff rrr => shift[$f] " " rgb[16*$rex+8*$rexb+$r] ",cl" ;
  0xd2 @addr => shift[$g] sfx[1] " " $addr ",cl" ;

  # --- imul Gv,Ev,imm -------------------------------------------------------
  0x69 11 ggg rrr @immz => "imul " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," hex($immz) wit(sx32($immz)>=-128 ? (sx32($immz)<=127 ? "imm32" : "") : "") ;
  0x69 @addr @immz      => "imul " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr "," hex($immz) wit(sx32($immz)>=-128 ? (sx32($immz)<=127 ? "imm32" : "") : "") ;
  0x6b 11 ggg rrr @imm8 => "imul " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," hex($imm8) ;
  0x6b @addr @imm8      => "imul " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr "," hex($imm8) ;

  # --- xchg / stack / nop ---------------------------------------------------
  0x90 [$reptype==1] => pause_t[$reptype] ;
  0x90 [$rexb==0] => "nop" ;
  10010 bbb => "xchg " greg[32*$rexw+16*$opsiz+8*$rexb+$b] "," greg[32*$rexw+16*$opsiz+0] ;
  0x87 11 ggg rrr => "xchg " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x87 @addr      => "xchg " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x86 11 ggg rrr => "xchg " rgb[16*$rex+8*$rexb+$r] "," rgb[16*$rex+8*$rexr+$g] ;
  0x86 @addr      => "xchg" sfx[1] " " $addr "," rgb[16*$rex+8*$rexr+$g] ;
  0x68 @immz => "push " hex($immz) wit(sx32($immz)>=-128 ? (sx32($immz)<=127 ? "imm32" : "") : "") ;
  0x6a @imm8 => "push " hex($imm8) ;
  01010 bbb => "push " dreg[16*$opsiz+8*$rexb+$b] ;
  01011 bbb => "pop " dreg[16*$opsiz+8*$rexb+$b] ;
  0x8f 11 000 rrr => wit("long") "pop " dreg[16*$opsiz+8*$rexb+$r] ;
  0x8f @addr(0) => "pop" sfx[8] " " $addr ;
  0x9c => "pushf" ;
  0x9d => "popf" ;

  # --- branches / ret / int / enter / leave ---------------------------------
  0xeb @rel8 => "jmp " hex($rel8) ;
  0xe9 @relz => wit("long") "jmp " hex($relz) ;
  0xe8 @relz => "call " hex($relz) ;
  0111 cccc @rel8 => "j" cond[$c] " " hex($rel8) ;
  0xe3 @rel8 => "jrcxz " hex($rel8) ;
  0xe2 @rel8 => "loop " hex($rel8) ;
  0xe1 @rel8 => "loope " hex($rel8) ;
  0xe0 @rel8 => "loopne " hex($rel8) ;
  0xc3 => "ret" ;
  0xc2 @imm16 => "ret " hex($imm16) ;
  0xcb => "retf" ;
  0xca @imm16 => "retf " hex($imm16) ;
  0xcd @imm8 => "int " hex($imm8) ;
  0xcc => "int3" ;
  0xcf => "iret" ;
  0xc9 => "leave" ;
  0xc8 @imm16 @imm8 => "enter " hex($imm16) "," hex($imm8) ;
  0xf1 => "int1" ;

  # --- string ops -----------------------------------------------------------
  0xa4 => "movsb" ;
  0xa5 => movs[$rexw? 2 : $opsiz] ;
  0xaa => "stosb" ;
  0xab => stos[$rexw? 2 : $opsiz] ;
  0xac => "lodsb" ;
  0xad => lods[$rexw? 2 : $opsiz] ;
  0xa6 => "cmpsb" ;
  0xae => "scasb" ;
  0xa7 => cmps[$rexw? 2 : $opsiz] ;
  0xaf => scas[$rexw? 2 : $opsiz] ;

  # --- flags / misc / size-varying accumulators -----------------------------
  0xf8 => "clc" ;
  0xf9 => "stc" ;
  0xf5 => "cmc" ;
  0xfc => "cld" ;
  0xfd => "std" ;
  0xfa => "cli" ;
  0xfb => "sti" ;
  0x9f => "lahf" ;
  0x9e => "sahf" ;
  0xf4 => "hlt" ;
  0x99 => cwd_t[$rexw? 2 : $opsiz] ;
  0x98 => cbw_t[$rexw? 2 : $opsiz] ;
  0x9b => "fwait" ;
  0xd7 => "xlatb" ;

  # --- I/O ------------------------------------------------------------------
  0xe4 @imm8 => "in al," hex($imm8) ;
  0xe5 @imm8 => "in " greg[16*$opsiz+0] "," hex($imm8) ;
  0xed => "in " greg[16*$opsiz+0] ",dx" ;
  0xe6 @imm8 => "out " hex($imm8) ",al" ;
  0xe7 @imm8 => "out " hex($imm8) "," greg[16*$opsiz+0] ;
  0xec => "in al,dx" ;
  0xee => "out dx,al" ;
  0xef => "out dx," greg[16*$opsiz+0] ;
  0x6c => "insb" ;
  0x6e => "outsb" ;
  0x6d => insx[$opsiz] ;
  0x6f => outsx[$opsiz] ;

  # ====================== two-byte 0F : GP / system ========================
  # --- jcc near (0F 80..8F, rel32) / setcc / cmovcc -------------------------
  0x0f 1000 cccc @relz => wit("long") "j" cond[$c] " " hex($relz) ;
  0x0f 1001 cccc 11 ggg rrr => "set" cond[$c] " " rgb[16*$rex+8*$rexb+$r] ;
  0x0f 1001 cccc @addr      => "set" cond[$c] sfx[1] " " $addr ;
  0x0f 0100 cccc 11 ggg rrr => "cmov" cond[$c] " " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0100 cccc @addr      => "cmov" cond[$c] " " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;

  # --- movzx / movsx --------------------------------------------------------
  0x0f 0xb6 11 ggg rrr => "movzx " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," rgb[16*$rex+8*$rexb+$r] ;
  0x0f 0xb6 @addr      => "movzx" sfx[1] " " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x0f 0xb7 11 ggg rrr => "movzx " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[16+8*$rexb+$r] ;
  0x0f 0xb7 @addr      => "movzx" sfx[2] " " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x0f 0xbe 11 ggg rrr => "movsx " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," rgb[16*$rex+8*$rexb+$r] ;
  0x0f 0xbe @addr      => "movsx" sfx[1] " " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x0f 0xbf 11 ggg rrr => "movsx " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[16+8*$rexb+$r] ;
  0x0f 0xbf @addr      => "movsx" sfx[2] " " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;

  # --- imul / bit string / shld / shrd --------------------------------------
  0x0f 0xaf 11 ggg rrr => "imul " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0xaf @addr      => "imul " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x0f 0xa3 11 ggg rrr => "bt " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xa3 @addr      => "bt " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xab 11 ggg rrr => "bts " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xab @addr      => "bts " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xb3 11 ggg rrr => "btr " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xb3 @addr      => "btr " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xbb 11 ggg rrr => "btc " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xbb @addr      => "btc " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xba 11 fff rrr @imm8 => grpba[$f] " " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," hex($imm8) ;
  0x0f 0xba @addr(4) @imm8 => "bt" sfx[$rexw? 8 : (4>>$opsiz)] " " $addr "," hex($imm8) ;
  0x0f 0xba @addr(5) @imm8 => "bts" sfx[$rexw? 8 : (4>>$opsiz)] " " $addr "," hex($imm8) ;
  0x0f 0xba @addr(6) @imm8 => "btr" sfx[$rexw? 8 : (4>>$opsiz)] " " $addr "," hex($imm8) ;
  0x0f 0xba @addr(7) @imm8 => "btc" sfx[$rexw? 8 : (4>>$opsiz)] " " $addr "," hex($imm8) ;
  0x0f 0xa4 11 ggg rrr @imm8 => "shld " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," hex($imm8) ;
  0x0f 0xa4 @addr @imm8 => "shld " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," hex($imm8) ;
  0x0f 0xa5 11 ggg rrr => "shld " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ",cl" ;
  0x0f 0xa5 @addr => "shld " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ",cl" ;
  0x0f 0xac 11 ggg rrr @imm8 => "shrd " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," hex($imm8) ;
  0x0f 0xac @addr @imm8 => "shrd " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," hex($imm8) ;
  0x0f 0xad 11 ggg rrr => "shrd " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ",cl" ;
  0x0f 0xad @addr => "shrd " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ",cl" ;

  # --- bsf / bsr / tzcnt / lzcnt / popcnt -----------------------------------
  0x0f 0xbc 11 ggg rrr [$reptype==1] => "tzcnt " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0xbc 11 ggg rrr => "bsf " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0xbc @addr      => "bsf " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x0f 0xbd 11 ggg rrr [$reptype==1] => "lzcnt " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0xbd 11 ggg rrr => "bsr " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0xbd @addr      => "bsr " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x0f 0xb8 11 ggg rrr [$reptype==1] => "popcnt " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0xb8 @addr [$reptype==1]      => "popcnt " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;

  # --- cmpxchg / xadd / cmpxchg8b16b / movnti / bswap -----------------------
  0x0f 0xb0 11 ggg rrr => "cmpxchg " rgb[16*$rex+8*$rexb+$r] "," rgb[16*$rex+8*$rexr+$g] ;
  0x0f 0xb0 @addr      => "cmpxchg" sfx[1] " " $addr "," rgb[16*$rex+8*$rexr+$g] ;
  0x0f 0xb1 11 ggg rrr => "cmpxchg " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xb1 @addr      => "cmpxchg " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xc0 11 ggg rrr => "xadd " rgb[16*$rex+8*$rexb+$r] "," rgb[16*$rex+8*$rexr+$g] ;
  0x0f 0xc0 @addr      => "xadd" sfx[1] " " $addr "," rgb[16*$rex+8*$rexr+$g] ;
  0x0f 0xc1 11 ggg rrr => "xadd " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xc1 @addr      => "xadd " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0xc7 @addr(1) => cx16[$rexw] " " $addr ;
  0x0f 0xc7 @addr(6) => "vmptrld " $addr ;
  0x0f 0xc7 @addr(7) => "vmptrst " $addr ;
  0x0f 0xc7 11 110 rrr => c7r6[$reptype] " " greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0xc7 11 111 rrr => c7r7[$reptype] " " greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0xc3 @addr      => "movnti " $addr "," greg[32*$rexw+8*$rexr+$g] ;
  0x0f 11001 bbb => "bswap " greg[32*$rexw+16*$opsiz+8*$rexb+$b] ;

  # --- segment push/pop, cpuid, msr, sys ------------------------------------
  0x0f 0xa0 => "push fs" ;
  0x0f 0xa1 => "pop fs" ;
  0x0f 0xa8 => "push gs" ;
  0x0f 0xa9 => "pop gs" ;
  0x0f 0xaa => "rsm" ;
  0x0f 0xa2 => "cpuid" ;
  0x0f 0x05 => "syscall" ;
  0x0f 0x07 => "sysret" ;
  0x0f 0x06 => "clts" ;
  0x0f 0x08 => "invd" ;
  0x0f 0x09 => "wbinvd" ;
  0x0f 0x0b => "ud2" ;
  0x0f 0x30 => "wrmsr" ;
  0x0f 0x31 => "rdtsc" ;
  0x0f 0x32 => "rdmsr" ;
  0x0f 0x33 => "rdpmc" ;
  0x0f 0x34 => "sysenter" ;
  0x0f 0x35 => "sysexit" ;
  0x0f 0x37 => "getsec" ;
  0x0f 0x77 => "emms" ;
  0x0f 0xb9 11 ggg rrr => "ud1 " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0xb9 @addr      => "ud1 " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x0f 0xff 11 ggg rrr => "ud0 " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;

  # --- 0F 00 (sldt/str/lldt/ltr/verr/verw) / 0F 01 descriptor + system ------
  0x0f 0x00 11 000 rrr => "sldt " greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0x00 11 001 rrr => "str " greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0x00 11 010 rrr => "lldt " greg[16+8*$rexb+$r] ;
  0x0f 0x00 11 011 rrr => "ltr " greg[16+8*$rexb+$r] ;
  0x0f 0x00 11 100 rrr => "verr " greg[16+8*$rexb+$r] ;
  0x0f 0x00 11 101 rrr => "verw " greg[16+8*$rexb+$r] ;
  0x0f 0x00 @addr(0) => "sldt " $addr ;
  0x0f 0x00 @addr(1) => "str " $addr ;
  0x0f 0x00 @addr(2) => "lldt " $addr ;
  0x0f 0x00 @addr(3) => "ltr " $addr ;
  0x0f 0x00 @addr(4) => "verr " $addr ;
  0x0f 0x00 @addr(5) => "verw " $addr ;
  0x0f 0x01 @addr(0) => "sgdt " $addr ;
  0x0f 0x01 @addr(1) => "sidt " $addr ;
  0x0f 0x01 @addr(2) => "lgdt " $addr ;
  0x0f 0x01 @addr(3) => "lidt " $addr ;
  0x0f 0x01 @addr(4) => "smsw " $addr ;
  0x0f 0x01 @addr(6) => "lmsw " $addr ;
  0x0f 0x01 @addr(7) => "invlpg " $addr ;
  0x0f 0x01 11 100 rrr => "smsw " greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0x01 11 110 rrr => "lmsw " greg[16+8*$rexb+$r] ;
  0x0f 0x01 0xc8 => "monitor" ;
  0x0f 0x01 0xc9 => "mwait" ;
  0x0f 0x01 0xca => "clac" ;
  0x0f 0x01 0xcb => "stac" ;
  0x0f 0x01 0xd0 => "xgetbv" ;
  0x0f 0x01 0xd1 => "xsetbv" ;
  0x0f 0x01 0xf8 => "swapgs" ;
  0x0f 0x01 0xf9 => "rdtscp" ;

  # --- lar / lsl / mov cr,dr ------------------------------------------------
  0x0f 0x02 11 ggg rrr => "lar " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0x02 @addr      => "lar " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x0f 0x03 11 ggg rrr => "lsl " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0x03 @addr      => "lsl " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x0f 0x20 11 ggg rrr => "mov " greg[32+8*$rexb+$r] "," crreg[8*$rexr+$g] ;
  0x0f 0x22 11 ggg rrr => "mov " crreg[8*$rexr+$g] "," greg[32+8*$rexb+$r] ;
  0x0f 0x21 11 ggg rrr => "mov " greg[32+8*$rexb+$r] "," drreg[8*$rexr+$g] ;
  0x0f 0x23 11 ggg rrr => "mov " drreg[8*$rexr+$g] "," greg[32+8*$rexb+$r] ;

  # --- CET / reserved-nop space (0F 1E/1C/1D/19/1F) -------------------------
  0x0f 0x1e 0xfa [$reptype==1] => e1efa[$reptype] ;
  0x0f 0x1e 0xfb [$reptype==1] => e1efb[$reptype] ;
  0x0f 0x1e 11 ggg rrr => "nop " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0x1e @addr      => "nop " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0x1c @addr(0) => "cldemote " $addr ;
  0x0f 0x1c 11 ggg rrr => "nop " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0x1c @addr      => "nop " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0x1d 11 ggg rrr => "nop " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0x1d @addr      => "nop " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0x19 11 ggg rrr => "nop " greg[32*$rexw+16*$opsiz+8*$rexb+$r] "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0x19 @addr      => "nop " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  # --- nop / prefetch / fences / fxsave group / clflush ---------------------
  0x0f 0x1f 11 ggg rrr => "nop " greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0x1f @addr      => "nop" sfx[$rexw? 8 : (4>>$opsiz)] " " $addr ;
  0x0f 0x0d @addr(1) => "prefetchw" sfx[1] " " $addr ;
  0x0f 0x18 @addr(0) => "prefetchnta" sfx[1] " " $addr ;
  0x0f 0x18 @addr(1) => "prefetcht0" sfx[1] " " $addr ;
  0x0f 0x18 @addr(2) => "prefetcht1" sfx[1] " " $addr ;
  0x0f 0x18 @addr(3) => "prefetcht2" sfx[1] " " $addr ;
  0x0f 0xae 0xe8 => "lfence" ;
  0x0f 0xae 0xf0 => "mfence" ;
  0x0f 0xae 0xf8 => "sfence" ;
  0x0f 0xae @addr(0) => "fxsave " $addr ;
  0x0f 0xae @addr(1) => "fxrstor " $addr ;
  0x0f 0xae @addr(2) => "ldmxcsr " $addr ;
  0x0f 0xae @addr(3) => "stmxcsr " $addr ;
  0x0f 0xae @addr(4) => "xsave " $addr ;
  0x0f 0xae @addr(5) => "xrstor " $addr ;
  0x0f 0xae @addr(6) => "xsaveopt " $addr ;
  0x0f 0xae @addr(7) => "clflush" sfx[1] " " $addr ;

  # ====================== two-byte 0F : SSE / SSE2 / SSE3 ==================
  0x0f 0x10 11 ggg rrr => movu[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x10 @addr      => movu[$reptype*2+$opsiz] sfx[movusz[$reptype*2+$opsiz]] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x11 11 ggg rrr => movu[$reptype*2+$opsiz] " " xreg[8*$rexb+$r] "," xreg[8*$rexr+$g] wit("alt") ;
  0x0f 0x11 @addr      => movu[$reptype*2+$opsiz] sfx[movusz[$reptype*2+$opsiz]] " " $addr "," xreg[8*$rexr+$g] ;
  0x0f 0x12 11 ggg rrr [$reptype] => m12f[$reptype] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x12 @addr      [$reptype] => m12f[$reptype] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x12 11 ggg rrr => "movhlps " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x12 @addr      => ml12[$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x13 @addr      => ml12[$opsiz] " " $addr "," xreg[8*$rexr+$g] ;
  0x0f 0x14 11 ggg rrr => unpl[$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x14 @addr      => unpl[$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x15 11 ggg rrr => unph[$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x15 @addr      => unph[$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x16 11 ggg rrr [$reptype] => m16f[$reptype] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x16 @addr      [$reptype] => m16f[$reptype] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x16 11 ggg rrr => "movlhps " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x16 @addr      => mh16[$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x17 @addr      => mh16[$opsiz] " " $addr "," xreg[8*$rexr+$g] ;
  0x0f 0x28 11 ggg rrr => mova2[$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x28 @addr      => mova2[$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x29 11 ggg rrr => mova2[$opsiz] " " xreg[8*$rexb+$r] "," xreg[8*$rexr+$g] wit("alt") ;
  0x0f 0x29 @addr      => mova2[$opsiz] " " $addr "," xreg[8*$rexr+$g] ;
  0x0f 0x2b @addr      => mnt4[$reptype*2+$opsiz] " " $addr "," xreg[8*$rexr+$g] ;
  0x0f 0x2a 11 ggg rrr [$reptype] => cvt2a[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," greg[32*$rexw+8*$rexb+$r] ;
  0x0f 0x2a 11 ggg rrr => cvt2a[$reptype*2+$opsiz] " " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x2a @addr [$reptype] => cvt2a[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x2a @addr      => cvt2a[$reptype*2+$opsiz] " " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x2c 11 ggg rrr [$reptype] => cvt2c[$reptype*2+$opsiz] " " greg[32*$rexw+8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x2c 11 ggg rrr => cvt2c[$reptype*2+$opsiz] " " mmreg[$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x2c @addr [$reptype] => cvt2c[$reptype*2+$opsiz] " " greg[32*$rexw+8*$rexr+$g] "," $addr ;
  0x0f 0x2d 11 ggg rrr [$reptype] => cvt2d[$reptype*2+$opsiz] " " greg[32*$rexw+8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x2d 11 ggg rrr => cvt2d[$reptype*2+$opsiz] " " mmreg[$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x2d @addr [$reptype] => cvt2d[$reptype*2+$opsiz] " " greg[32*$rexw+8*$rexr+$g] "," $addr ;
  0x0f 0x2e 11 ggg rrr => uc[$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x2e @addr      => uc[$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x2f 11 ggg rrr => comi[$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x2f @addr      => comi[$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x50 11 ggg rrr => "movmsk" elt[$opsiz] " " greg[32*$rexw+8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x51 11 ggg rrr => "sqrt" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x51 @addr      => "sqrt" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x52 11 ggg rrr => "rsqrt" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x52 @addr      => "rsqrt" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x53 11 ggg rrr => "rcp" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x53 @addr      => "rcp" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x54 11 ggg rrr => "and" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x54 @addr      => "and" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x55 11 ggg rrr => "andn" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x55 @addr      => "andn" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x56 11 ggg rrr => "or" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x56 @addr      => "or" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x57 11 ggg rrr => "xor" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x57 @addr      => "xor" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x58 11 ggg rrr => "add" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x58 @addr      => "add" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x59 11 ggg rrr => "mul" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x59 @addr      => "mul" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x5c 11 ggg rrr => "sub" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x5c @addr      => "sub" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x5d 11 ggg rrr => "min" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x5d @addr      => "min" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x5e 11 ggg rrr => "div" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x5e @addr      => "div" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x5f 11 ggg rrr => "max" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x5f @addr      => "max" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x5a 11 ggg rrr => cvt5a[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x5a @addr      => cvt5a[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x5b 11 ggg rrr => cvt5b[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x5b @addr      => cvt5b[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x60 11 ggg rrr => "punpcklbw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x60 @addr      => "punpcklbw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x61 11 ggg rrr => "punpcklwd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x61 @addr      => "punpcklwd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x62 11 ggg rrr => "punpckldq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x62 @addr      => "punpckldq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x63 11 ggg rrr => "packsswb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x63 @addr      => "packsswb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x64 11 ggg rrr => "pcmpgtb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x64 @addr      => "pcmpgtb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x65 11 ggg rrr => "pcmpgtw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x65 @addr      => "pcmpgtw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x66 11 ggg rrr => "pcmpgtd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x66 @addr      => "pcmpgtd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x67 11 ggg rrr => "packuswb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x67 @addr      => "packuswb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x68 11 ggg rrr => "punpckhbw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x68 @addr      => "punpckhbw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x69 11 ggg rrr => "punpckhwd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x69 @addr      => "punpckhwd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x6a 11 ggg rrr => "punpckhdq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x6a @addr      => "punpckhdq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x6b 11 ggg rrr => "packssdw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x6b @addr      => "packssdw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x6c 11 ggg rrr => "punpcklqdq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x6c @addr      => "punpcklqdq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x6d 11 ggg rrr => "punpckhqdq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x6d @addr      => "punpckhqdq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x74 11 ggg rrr => "pcmpeqb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x74 @addr      => "pcmpeqb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x75 11 ggg rrr => "pcmpeqw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x75 @addr      => "pcmpeqw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x76 11 ggg rrr => "pcmpeqd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x76 @addr      => "pcmpeqd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xd1 11 ggg rrr => "psrlw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xd1 @addr      => "psrlw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xd2 11 ggg rrr => "psrld " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xd2 @addr      => "psrld " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xd3 11 ggg rrr => "psrlq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xd3 @addr      => "psrlq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xd4 11 ggg rrr => "paddq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xd4 @addr      => "paddq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xd5 11 ggg rrr => "pmullw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xd5 @addr      => "pmullw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xd8 11 ggg rrr => "psubusb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xd8 @addr      => "psubusb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xd9 11 ggg rrr => "psubusw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xd9 @addr      => "psubusw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xda 11 ggg rrr => "pminub " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xda @addr      => "pminub " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xdb 11 ggg rrr => "pand " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xdb @addr      => "pand " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xdc 11 ggg rrr => "paddusb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xdc @addr      => "paddusb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xdd 11 ggg rrr => "paddusw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xdd @addr      => "paddusw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xde 11 ggg rrr => "pmaxub " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xde @addr      => "pmaxub " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xdf 11 ggg rrr => "pandn " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xdf @addr      => "pandn " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xe0 11 ggg rrr => "pavgb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xe0 @addr      => "pavgb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xe1 11 ggg rrr => "psraw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xe1 @addr      => "psraw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xe2 11 ggg rrr => "psrad " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xe2 @addr      => "psrad " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xe3 11 ggg rrr => "pavgw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xe3 @addr      => "pavgw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xe4 11 ggg rrr => "pmulhuw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xe4 @addr      => "pmulhuw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xe5 11 ggg rrr => "pmulhw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xe5 @addr      => "pmulhw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xe8 11 ggg rrr => "psubsb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xe8 @addr      => "psubsb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xe9 11 ggg rrr => "psubsw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xe9 @addr      => "psubsw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xea 11 ggg rrr => "pminsw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xea @addr      => "pminsw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xeb 11 ggg rrr => "por " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xeb @addr      => "por " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xec 11 ggg rrr => "paddsb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xec @addr      => "paddsb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xed 11 ggg rrr => "paddsw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xed @addr      => "paddsw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xee 11 ggg rrr => "pmaxsw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xee @addr      => "pmaxsw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xef 11 ggg rrr => "pxor " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xef @addr      => "pxor " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xf1 11 ggg rrr => "psllw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xf1 @addr      => "psllw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xf2 11 ggg rrr => "pslld " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xf2 @addr      => "pslld " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xf3 11 ggg rrr => "psllq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xf3 @addr      => "psllq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xf4 11 ggg rrr => "pmuludq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xf4 @addr      => "pmuludq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xf5 11 ggg rrr => "pmaddwd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xf5 @addr      => "pmaddwd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xf6 11 ggg rrr => "psadbw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xf6 @addr      => "psadbw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xf8 11 ggg rrr => "psubb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xf8 @addr      => "psubb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xf9 11 ggg rrr => "psubw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xf9 @addr      => "psubw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xfa 11 ggg rrr => "psubd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xfa @addr      => "psubd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xfb 11 ggg rrr => "psubq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xfb @addr      => "psubq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xfc 11 ggg rrr => "paddb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xfc @addr      => "paddb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xfd 11 ggg rrr => "paddw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xfd @addr      => "paddw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0xfe 11 ggg rrr => "paddd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xfe @addr      => "paddd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x6e 11 ggg rrr => mdq2[$rexw] " " simd[8*$opsiz+16*$rexr+$g] "," greg[32*$rexw+8*$rexb+$r] ;
  0x0f 0x6e @addr      => mdq2[$rexw] sfx[$rexw? 8 : 4] " " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x7e 11 ggg rrr [$reptype==1] => "movq " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x7e @addr      [$reptype==1] => "movq " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x7e 11 ggg rrr => mdq2[$rexw] " " greg[32*$rexw+8*$rexb+$r] "," simd[8*$opsiz+16*$rexr+$g] ;
  0x0f 0x7e @addr      => mdq2[$rexw] sfx[$rexw? 8 : 4] " " $addr "," simd[8*$opsiz+16*$rexr+$g] ;
  0x0f 0x6f 11 ggg rrr [$reptype==1] => "movdqu " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x6f @addr      [$reptype==1] => "movdqu " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x6f 11 ggg rrr [$opsiz] => "movdqa " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x6f @addr      [$opsiz] => "movdqa " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x6f 11 ggg rrr => "movq " mmreg[$g] "," mmreg[$r] ;
  0x0f 0x6f @addr      => "movq " mmreg[$g] "," $addr ;
  0x0f 0x7f 11 ggg rrr [$reptype==1] => "movdqu " xreg[8*$rexb+$r] "," xreg[8*$rexr+$g] wit("alt") ;
  0x0f 0x7f @addr      [$reptype==1] => "movdqu " $addr "," xreg[8*$rexr+$g] ;
  0x0f 0x7f 11 ggg rrr [$opsiz] => "movdqa " xreg[8*$rexb+$r] "," xreg[8*$rexr+$g] wit("alt") ;
  0x0f 0x7f @addr      [$opsiz] => "movdqa " $addr "," xreg[8*$rexr+$g] ;
  0x0f 0x7f 11 ggg rrr => "movq " mmreg[$r] "," mmreg[$g] wit("alt") ;
  0x0f 0x7f @addr      => "movq " $addr "," mmreg[$g] ;
  0x0f 0x70 11 ggg rrr @imm8 [$opsiz] => "pshufd " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] "," hex($imm8) ;
  0x0f 0x70 @addr @imm8 [$opsiz] => "pshufd " xreg[8*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x70 11 ggg rrr @imm8 [$reptype==1] => "pshufhw " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] "," hex($imm8) ;
  0x0f 0x70 @addr @imm8 [$reptype==1] => "pshufhw " xreg[8*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x70 11 ggg rrr @imm8 [$reptype==2] => "pshuflw " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] "," hex($imm8) ;
  0x0f 0x70 @addr @imm8 [$reptype==2] => "pshuflw " xreg[8*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x70 11 ggg rrr @imm8 => "pshufw " mmreg[$g] "," mmreg[$r] "," hex($imm8) ;
  0x0f 0x70 @addr @imm8 => "pshufw " mmreg[$g] "," $addr "," hex($imm8) ;
  0x0f 0x71 11 010 rrr @imm8 => "psrlw " simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x71 11 100 rrr @imm8 => "psraw " simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x71 11 110 rrr @imm8 => "psllw " simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x72 11 010 rrr @imm8 => "psrld " simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x72 11 100 rrr @imm8 => "psrad " simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x72 11 110 rrr @imm8 => "pslld " simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x73 11 010 rrr @imm8 => "psrlq " simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x73 11 011 rrr @imm8 => "psrldq " simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x73 11 110 rrr @imm8 => "psllq " simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x73 11 111 rrr @imm8 => "pslldq " simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0xc2 11 ggg rrr @imm8 => "cmp" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] "," hex($imm8) ;
  0x0f 0xc2 @addr @imm8 => "cmp" elt[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0xc3 @addr => "movnti " $addr "," greg[32*$rexw+8*$rexr+$g] ;
  0x0f 0xc4 11 ggg rrr @imm8 => "pinsrw " simd[8*$opsiz+16*$rexr+$g] "," greg[32*$rexw+8*$rexb+$r] "," hex($imm8) ;
  0x0f 0xc4 @addr @imm8 => "pinsrw " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0xc5 11 ggg rrr @imm8 => "pextrw " greg[32*$rexw+8*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0xc6 11 ggg rrr @imm8 => "shuf" elt[$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] "," hex($imm8) ;
  0x0f 0xc6 @addr @imm8 => "shuf" elt[$opsiz] " " xreg[8*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0xd0 11 ggg rrr => d0t[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0xd0 @addr      => d0t[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x7c 11 ggg rrr => h7c[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x7c @addr      => h7c[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x7d 11 ggg rrr => h7d[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x7d @addr      => h7d[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0xd6 11 ggg rrr [$reptype==1] => "movq2dq " xreg[8*$rexr+$g] "," mmreg[$r] ;
  0x0f 0xd6 11 ggg rrr [$reptype==2] => "movdq2q " mmreg[$g] "," xreg[8*$rexb+$r] ;
  0x0f 0xd6 11 ggg rrr => "movq " xreg[8*$rexb+$r] "," xreg[8*$rexr+$g] wit("alt") ;
  0x0f 0xd6 @addr      => "movq " $addr "," xreg[8*$rexr+$g] ;
  0x0f 0xd7 11 ggg rrr => "pmovmskb " greg[32*$rexw+8*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xe6 11 ggg rrr => me6[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0xe6 @addr      => me6[$reptype*2+$opsiz] " " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0xe7 @addr      => me7[$opsiz] " " $addr "," simd[8*$opsiz+16*$rexr+$g] ;
  0x0f 0xf7 11 ggg rrr => mf7[$opsiz] " " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0xf0 @addr [$reptype==2] => "lddqu " xreg[8*$rexr+$g] "," $addr ;

  # ================= three-byte 0F38 / 0F3A : SSSE3 / SSE4 / AES / SHA =====
  0x0f 0x38 0x00 11 ggg rrr => "pshufb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x00 @addr      => "pshufb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x01 11 ggg rrr => "phaddw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x01 @addr      => "phaddw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x02 11 ggg rrr => "phaddd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x02 @addr      => "phaddd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x03 11 ggg rrr => "phaddsw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x03 @addr      => "phaddsw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x04 11 ggg rrr => "pmaddubsw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x04 @addr      => "pmaddubsw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x05 11 ggg rrr => "phsubw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x05 @addr      => "phsubw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x06 11 ggg rrr => "phsubd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x06 @addr      => "phsubd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x07 11 ggg rrr => "phsubsw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x07 @addr      => "phsubsw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x08 11 ggg rrr => "psignb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x08 @addr      => "psignb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x09 11 ggg rrr => "psignw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x09 @addr      => "psignw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x0a 11 ggg rrr => "psignd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x0a @addr      => "psignd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x0b 11 ggg rrr => "pmulhrsw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x0b @addr      => "pmulhrsw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x10 11 ggg rrr => "pblendvb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x10 @addr      => "pblendvb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x14 11 ggg rrr => "blendvps " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x14 @addr      => "blendvps " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x15 11 ggg rrr => "blendvpd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x15 @addr      => "blendvpd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x17 11 ggg rrr => "ptest " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x17 @addr      => "ptest " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x1c 11 ggg rrr => "pabsb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x1c @addr      => "pabsb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x1d 11 ggg rrr => "pabsw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x1d @addr      => "pabsw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x1e 11 ggg rrr => "pabsd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x1e @addr      => "pabsd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x20 11 ggg rrr => "pmovsxbw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x20 @addr      => "pmovsxbw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x21 11 ggg rrr => "pmovsxbd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x21 @addr      => "pmovsxbd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x22 11 ggg rrr => "pmovsxbq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x22 @addr      => "pmovsxbq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x23 11 ggg rrr => "pmovsxwd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x23 @addr      => "pmovsxwd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x24 11 ggg rrr => "pmovsxwq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x24 @addr      => "pmovsxwq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x25 11 ggg rrr => "pmovsxdq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x25 @addr      => "pmovsxdq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x28 11 ggg rrr => "pmuldq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x28 @addr      => "pmuldq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x29 11 ggg rrr => "pcmpeqq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x29 @addr      => "pcmpeqq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x2b 11 ggg rrr => "packusdw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x2b @addr      => "packusdw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x30 11 ggg rrr => "pmovzxbw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x30 @addr      => "pmovzxbw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x31 11 ggg rrr => "pmovzxbd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x31 @addr      => "pmovzxbd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x32 11 ggg rrr => "pmovzxbq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x32 @addr      => "pmovzxbq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x33 11 ggg rrr => "pmovzxwd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x33 @addr      => "pmovzxwd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x34 11 ggg rrr => "pmovzxwq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x34 @addr      => "pmovzxwq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x35 11 ggg rrr => "pmovzxdq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x35 @addr      => "pmovzxdq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x37 11 ggg rrr => "pcmpgtq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x37 @addr      => "pcmpgtq " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x38 11 ggg rrr => "pminsb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x38 @addr      => "pminsb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x39 11 ggg rrr => "pminsd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x39 @addr      => "pminsd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x3a 11 ggg rrr => "pminuw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x3a @addr      => "pminuw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x3b 11 ggg rrr => "pminud " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x3b @addr      => "pminud " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x3c 11 ggg rrr => "pmaxsb " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x3c @addr      => "pmaxsb " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x3d 11 ggg rrr => "pmaxsd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x3d @addr      => "pmaxsd " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x3e 11 ggg rrr => "pmaxuw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x3e @addr      => "pmaxuw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x3f 11 ggg rrr => "pmaxud " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x3f @addr      => "pmaxud " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x40 11 ggg rrr => "pmulld " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x40 @addr      => "pmulld " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x41 11 ggg rrr => "phminposuw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] ;
  0x0f 0x38 0x41 @addr      => "phminposuw " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0x2a @addr      => "movntdqa " simd[8*$opsiz+16*$rexr+$g] "," $addr ;
  0x0f 0x38 0xdb 11 ggg rrr => "aesimc " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x38 0xdb @addr      => "aesimc " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xdc 11 ggg rrr => "aesenc " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x38 0xdc @addr      => "aesenc " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xdd 11 ggg rrr => "aesenclast " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x38 0xdd @addr      => "aesenclast " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xde 11 ggg rrr => "aesdec " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x38 0xde @addr      => "aesdec " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xdf 11 ggg rrr => "aesdeclast " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x38 0xdf @addr      => "aesdeclast " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xc8 11 ggg rrr => "sha1nexte " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x38 0xc8 @addr      => "sha1nexte " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xc9 11 ggg rrr => "sha1msg1 " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x38 0xc9 @addr      => "sha1msg1 " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xca 11 ggg rrr => "sha1msg2 " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x38 0xca @addr      => "sha1msg2 " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xcb 11 ggg rrr => "sha256rnds2 " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x38 0xcb @addr      => "sha256rnds2 " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xcc 11 ggg rrr => "sha256msg1 " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x38 0xcc @addr      => "sha256msg1 " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xcd 11 ggg rrr => "sha256msg2 " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] ;
  0x0f 0x38 0xcd @addr      => "sha256msg2 " xreg[8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xf0 11 ggg rrr [$reptype==2] => "crc32" sfx[1] " " greg[32*$rexw+8*$rexr+$g] "," rgb[16*$rex+8*$rexb+$r] ;
  0x0f 0x38 0xf0 @addr      [$reptype==2] => "crc32" sfx[1] " " greg[32*$rexw+8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xf1 11 ggg rrr [$reptype==2] => "crc32 " greg[32*$rexw+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0x38 0xf1 @addr      [$reptype==2] => "crc32" sfx[$rexw? 8 : (4>>$opsiz)] " " greg[32*$rexw+8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xf6 11 ggg rrr [$reptype==1] => "adox " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0x38 0xf6 11 ggg rrr [$opsiz] => "adcx " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0x38 0xf0 11 ggg rrr => "movbe " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," greg[32*$rexw+16*$opsiz+8*$rexb+$r] ;
  0x0f 0x38 0xf0 @addr      => "movbe " greg[32*$rexw+16*$opsiz+8*$rexr+$g] "," $addr ;
  0x0f 0x38 0xf1 @addr      => "movbe " $addr "," greg[32*$rexw+16*$opsiz+8*$rexr+$g] ;
  0x0f 0x3a 0x08 11 ggg rrr @imm8 => "roundps " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x08 @addr      @imm8 => "roundps " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x09 11 ggg rrr @imm8 => "roundpd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x09 @addr      @imm8 => "roundpd " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0a 11 ggg rrr @imm8 => "roundss " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0a @addr      @imm8 => "roundss " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0b 11 ggg rrr @imm8 => "roundsd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0b @addr      @imm8 => "roundsd " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0c 11 ggg rrr @imm8 => "blendps " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0c @addr      @imm8 => "blendps " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0d 11 ggg rrr @imm8 => "blendpd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0d @addr      @imm8 => "blendpd " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0e 11 ggg rrr @imm8 => "pblendw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0e @addr      @imm8 => "pblendw " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0f 11 ggg rrr @imm8 => "palignr " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0f @addr      @imm8 => "palignr " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x21 11 ggg rrr @imm8 => "insertps " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x21 @addr      @imm8 => "insertps " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x40 11 ggg rrr @imm8 => "dpps " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x40 @addr      @imm8 => "dpps " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x41 11 ggg rrr @imm8 => "dppd " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x41 @addr      @imm8 => "dppd " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x42 11 ggg rrr @imm8 => "mpsadbw " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x42 @addr      @imm8 => "mpsadbw " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x44 11 ggg rrr @imm8 => "pclmulqdq " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x44 @addr      @imm8 => "pclmulqdq " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x60 11 ggg rrr @imm8 => "pcmpestrm " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x60 @addr      @imm8 => "pcmpestrm " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x61 11 ggg rrr @imm8 => "pcmpestri " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x61 @addr      @imm8 => "pcmpestri " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x62 11 ggg rrr @imm8 => "pcmpistrm " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x62 @addr      @imm8 => "pcmpistrm " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x63 11 ggg rrr @imm8 => "pcmpistri " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x63 @addr      @imm8 => "pcmpistri " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0xdf 11 ggg rrr @imm8 => "aeskeygenassist " simd[8*$opsiz+16*$rexr+$g] "," simd[8*$opsiz+16*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0xdf @addr      @imm8 => "aeskeygenassist " simd[8*$opsiz+16*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0xcc 11 ggg rrr @imm8 => "sha1rnds4 " xreg[8*$rexr+$g] "," xreg[8*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0xcc @addr      @imm8 => "sha1rnds4 " xreg[8*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x14 11 ggg rrr @imm8 => "pextrb " greg[32*$rexw+8*$rexb+$r] "," xreg[8*$rexr+$g] "," hex($imm8) ;
  0x0f 0x3a 0x14 @addr      @imm8 => "pextrb" sfx[1] " " $addr "," xreg[8*$rexr+$g] "," hex($imm8) ;
  0x0f 0x3a 0x15 11 ggg rrr @imm8 => "pextrw " greg[32*$rexw+8*$rexb+$r] "," xreg[8*$rexr+$g] "," hex($imm8) wit("long") ;
  0x0f 0x3a 0x15 @addr      @imm8 => "pextrw" sfx[2] " " $addr "," xreg[8*$rexr+$g] "," hex($imm8) ;
  0x0f 0x3a 0x16 11 ggg rrr @imm8 => pextrdq[$rexw] " " greg[32*$rexw+8*$rexb+$r] "," xreg[8*$rexr+$g] "," hex($imm8) ;
  0x0f 0x3a 0x16 @addr      @imm8 => pextrdq[$rexw] " " $addr "," xreg[8*$rexr+$g] "," hex($imm8) ;
  0x0f 0x3a 0x17 11 ggg rrr @imm8 => "extractps " greg[32*$rexw+8*$rexb+$r] "," xreg[8*$rexr+$g] "," hex($imm8) ;
  0x0f 0x3a 0x17 @addr      @imm8 => "extractps " $addr "," xreg[8*$rexr+$g] "," hex($imm8) ;
  0x0f 0x3a 0x20 11 ggg rrr @imm8 => "pinsrb " xreg[8*$rexr+$g] "," greg[32*$rexw+8*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x20 @addr      @imm8 => "pinsrb " xreg[8*$rexr+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x22 11 ggg rrr @imm8 => pinsrdq[$rexw] " " xreg[8*$rexr+$g] "," greg[32*$rexw+8*$rexb+$r] "," hex($imm8) ;
  0x0f 0x3a 0x22 @addr      @imm8 => pinsrdq[$rexw] " " xreg[8*$rexr+$g] "," $addr "," hex($imm8) ;

  # ============================ x87 FPU (D8-DF) =========================
  # --- x87 FPU (D8-DF) ---
  0xd8 @addr(0)   => "fadd.d " $addr ;
  0xd8 @addr(1)   => "fmul.d " $addr ;
  0xd8 @addr(2)   => "fcom.d " $addr ;
  0xd8 @addr(3)   => "fcomp.d " $addr ;
  0xd8 @addr(4)   => "fsub.d " $addr ;
  0xd8 @addr(5)   => "fsubr.d " $addr ;
  0xd8 @addr(6)   => "fdiv.d " $addr ;
  0xd8 @addr(7)   => "fdivr.d " $addr ;
  0xd9 @addr(0)   => "fld.d " $addr ;
  0xd9 @addr(2)   => "fst.d " $addr ;
  0xd9 @addr(3)   => "fstp.d " $addr ;
  0xd9 @addr(4)   => "fldenv " $addr ;
  0xd9 @addr(5)   => "fldcw.w " $addr ;
  0xd9 @addr(6)   => "fnstenv " $addr ;
  0xd9 @addr(7)   => "fnstcw.w " $addr ;
  0xda @addr(0)   => "fiadd.d " $addr ;
  0xda @addr(1)   => "fimul.d " $addr ;
  0xda @addr(2)   => "ficom.d " $addr ;
  0xda @addr(3)   => "ficomp.d " $addr ;
  0xda @addr(4)   => "fisub.d " $addr ;
  0xda @addr(5)   => "fisubr.d " $addr ;
  0xda @addr(6)   => "fidiv.d " $addr ;
  0xda @addr(7)   => "fidivr.d " $addr ;
  0xdb @addr(0)   => "fild.d " $addr ;
  0xdb @addr(1)   => "fisttp.d " $addr ;
  0xdb @addr(2)   => "fist.d " $addr ;
  0xdb @addr(3)   => "fistp.d " $addr ;
  0xdb @addr(5)   => "fld.t " $addr ;
  0xdb @addr(7)   => "fstp.t " $addr ;
  0xdc @addr(0)   => "fadd.q " $addr ;
  0xdc @addr(1)   => "fmul.q " $addr ;
  0xdc @addr(2)   => "fcom.q " $addr ;
  0xdc @addr(3)   => "fcomp.q " $addr ;
  0xdc @addr(4)   => "fsub.q " $addr ;
  0xdc @addr(5)   => "fsubr.q " $addr ;
  0xdc @addr(6)   => "fdiv.q " $addr ;
  0xdc @addr(7)   => "fdivr.q " $addr ;
  0xdd @addr(0)   => "fld.q " $addr ;
  0xdd @addr(1)   => "fisttp.q " $addr ;
  0xdd @addr(2)   => "fst.q " $addr ;
  0xdd @addr(3)   => "fstp.q " $addr ;
  0xdd @addr(4)   => "frstor " $addr ;
  0xdd @addr(6)   => "fnsave " $addr ;
  0xdd @addr(7)   => "fnstsw " $addr ;
  0xde @addr(0)   => "fiadd.w " $addr ;
  0xde @addr(1)   => "fimul.w " $addr ;
  0xde @addr(2)   => "ficom.w " $addr ;
  0xde @addr(3)   => "ficomp.w " $addr ;
  0xde @addr(4)   => "fisub.w " $addr ;
  0xde @addr(5)   => "fisubr.w " $addr ;
  0xde @addr(6)   => "fidiv.w " $addr ;
  0xde @addr(7)   => "fidivr.w " $addr ;
  0xdf @addr(0)   => "fild.w " $addr ;
  0xdf @addr(1)   => "fisttp.w " $addr ;
  0xdf @addr(2)   => "fist.w " $addr ;
  0xdf @addr(3)   => "fistp.w " $addr ;
  0xdf @addr(4)   => "fbld.t " $addr ;
  0xdf @addr(5)   => "fild.q " $addr ;
  0xdf @addr(6)   => "fbstp.t " $addr ;
  0xdf @addr(7)   => "fistp.q " $addr ;
  # D8 reg
  0xd8 11 010 rrr => "fcom st(" dec($r) ")" ;
  0xd8 11 011 rrr => "fcomp st(" dec($r) ")" ;
  0xd8 11 ddd rrr => d8r[$d] " st(0),st(" dec($r) ")" ;
  # D9 reg
  0xd9 11 000 rrr => "fld st(" dec($r) ")" ;
  0xd9 11 001 rrr => "fxch st(" dec($r) ")" ;
  0xd9 11 011 rrr => "fstpnce st(" dec($r) ")" ;
  0xd9 0xd0 => "fnop" ;
  0xd9 0xe0 => "fchs" ;
  0xd9 0xe1 => "fabs" ;
  0xd9 0xe4 => "ftst" ;
  0xd9 0xe5 => "fxam" ;
  0xd9 0xe8 => "fld1" ;
  0xd9 0xe9 => "fldl2t" ;
  0xd9 0xea => "fldl2e" ;
  0xd9 0xeb => "fldpi" ;
  0xd9 0xec => "fldlg2" ;
  0xd9 0xed => "fldln2" ;
  0xd9 0xee => "fldz" ;
  0xd9 0xf0 => "f2xm1" ;
  0xd9 0xf1 => "fyl2x" ;
  0xd9 0xf2 => "fptan" ;
  0xd9 0xf3 => "fpatan" ;
  0xd9 0xf4 => "fxtract" ;
  0xd9 0xf5 => "fprem1" ;
  0xd9 0xf6 => "fdecstp" ;
  0xd9 0xf7 => "fincstp" ;
  0xd9 0xf8 => "fprem" ;
  0xd9 0xf9 => "fyl2xp1" ;
  0xd9 0xfa => "fsqrt" ;
  0xd9 0xfb => "fsincos" ;
  0xd9 0xfc => "frndint" ;
  0xd9 0xfd => "fscale" ;
  0xd9 0xfe => "fsin" ;
  0xd9 0xff => "fcos" ;
  # DA reg
  0xda 0xe9 => "fucompp" ;
  0xda 11 0dd rrr => dar[$d] " st(0),st(" dec($r) ")" ;
  # DB reg
  0xdb 0xe0 => "feni8087_nop" ;
  0xdb 0xe1 => "fdisi8087_nop" ;
  0xdb 0xe2 => "fnclex" ;
  0xdb 0xe3 => "fninit" ;
  0xdb 0xe4 => "fsetpm287_nop" ;
  0xdb 11 0dd rrr => dbr[$d] " st(0),st(" dec($r) ")" ;
  0xdb 11 101 rrr => "fucomi st(0),st(" dec($r) ")" ;
  0xdb 11 110 rrr => "fcomi st(0),st(" dec($r) ")" ;
  # DC reg
  0xdc 11 010 rrr => "fcom st(" dec($r) ")" wit("alt") ;
  0xdc 11 011 rrr => "fcomp st(" dec($r) ")" wit("alt") ;
  0xdc 11 ddd rrr => dcr[$d] " st(" dec($r) "),st(0)" wit($r==0 ? "alt" : "") ;
  # DD reg
  0xdd 11 000 rrr => "ffree st(" dec($r) ")" ;
  0xdd 11 001 rrr => "fxch st(" dec($r) ")" wit("alt") ;
  0xdd 11 010 rrr => "fst st(" dec($r) ")" ;
  0xdd 11 011 rrr => "fstp st(" dec($r) ")" ;
  0xdd 11 100 rrr => "fucom st(" dec($r) ")" ;
  0xdd 11 101 rrr => "fucomp st(0),st(" dec($r) ")" ;
  # DE reg
  0xde 0xd9 => "fcompp" ;
  0xde 11 010 rrr => "fcomp st(" dec($r) ")" wit("long") ;
  0xde 11 ddd rrr => der[$d] " st(" dec($r) "),st(0)" ;
  # DF reg
  0xdf 0xe0 => "fnstsw" ;
  0xdf 11 000 rrr => "ffreep st(" dec($r) ")" ;
  0xdf 11 001 rrr => "fxch st(" dec($r) ")" wit("long") ;
  0xdf 11 010 rrr => "fstp st(" dec($r) ")" wit("alt") ;
  0xdf 11 011 rrr => "fstp st(" dec($r) ")" wit("long") ;
  0xdf 11 101 rrr => "fucomip st(0),st(" dec($r) ")" ;
  0xdf 11 110 rrr => "fcomip st(0),st(" dec($r) ")" ;
}

# ============================ VEX (C4 / C5) ================================
# byte1(C4) = R-bar X-bar B-bar mmmmm ; byte2 = W vvvv L pp.  Register operands
# read the inverted R/B bits straight from the prefix via vreg; memory operands
# set $rexb/$rexx (= 1 - the byte1 bit) so the shared addr matcher renders the
# base/index, and the assembler bridges REX.B/X back to byte1 (asm._vexfix).
submatch vex {
  h k b 00001 0 vvvv y pp 0x54 11 ggg rrr => "vand" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y pp 0x54 @addr {$rexb=1-$b;$rexx=1-$k} => "vand" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y pp 0x55 11 ggg rrr => "vandn" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y pp 0x55 @addr {$rexb=1-$b;$rexx=1-$k} => "vandn" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y pp 0x56 11 ggg rrr => "vor" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y pp 0x56 @addr {$rexb=1-$b;$rexx=1-$k} => "vor" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y pp 0x57 11 ggg rrr => "vxor" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y pp 0x57 @addr {$rexb=1-$b;$rexx=1-$k} => "vxor" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y pp 0x58 11 ggg rrr => "vadd" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y pp 0x58 @addr {$rexb=1-$b;$rexx=1-$k} => "vadd" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y pp 0x59 11 ggg rrr => "vmul" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y pp 0x59 @addr {$rexb=1-$b;$rexx=1-$k} => "vmul" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y pp 0x5c 11 ggg rrr => "vsub" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y pp 0x5c @addr {$rexb=1-$b;$rexx=1-$k} => "vsub" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y pp 0x5d 11 ggg rrr => "vmin" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y pp 0x5d @addr {$rexb=1-$b;$rexx=1-$k} => "vmin" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y pp 0x5e 11 ggg rrr => "vdiv" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y pp 0x5e @addr {$rexb=1-$b;$rexx=1-$k} => "vdiv" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y pp 0x5f 11 ggg rrr => "vmax" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y pp 0x5f @addr {$rexb=1-$b;$rexx=1-$k} => "vmax" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 1111 y pp 0x28 11 ggg rrr => vmova[$p] " " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 1111 y pp 0x28 @addr {$rexb=1-$b;$rexx=1-$k} => vmova[$p] " " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00001 0 1111 y pp 0x29 @addr {$rexb=1-$b;$rexx=1-$k} => vmova[$p] " " $addr "," vreg[16*$y+8*$h+$g] ;
  h k b 00001 0 1111 y pp 0x10 11 ggg rrr => vmovu[$p] " " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 1111 y pp 0x10 @addr {$rexb=1-$b;$rexx=1-$k} => vmovu[$p] " " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00001 0 1111 y pp 0x11 @addr {$rexb=1-$b;$rexx=1-$k} => vmovu[$p] " " $addr "," vreg[16*$y+8*$h+$g] ;
  h k b 00001 0 1111 y pp 0x6f 11 ggg rrr => vmdq[$p] " " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 1111 y pp 0x6f @addr {$rexb=1-$b;$rexx=1-$k} => vmdq[$p] " " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00001 0 1111 y pp 0x7f @addr {$rexb=1-$b;$rexx=1-$k} => vmdq[$p] " " $addr "," vreg[16*$y+8*$h+$g] ;
  h k b 00001 0 vvvv y 01 0xfc 11 ggg rrr => "vpaddb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xfc @addr {$rexb=1-$b;$rexx=1-$k} => "vpaddb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xfd 11 ggg rrr => "vpaddw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xfd @addr {$rexb=1-$b;$rexx=1-$k} => "vpaddw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xfe 11 ggg rrr => "vpaddd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xfe @addr {$rexb=1-$b;$rexx=1-$k} => "vpaddd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xd4 11 ggg rrr => "vpaddq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xd4 @addr {$rexb=1-$b;$rexx=1-$k} => "vpaddq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xf8 11 ggg rrr => "vpsubb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xf8 @addr {$rexb=1-$b;$rexx=1-$k} => "vpsubb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xf9 11 ggg rrr => "vpsubw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xf9 @addr {$rexb=1-$b;$rexx=1-$k} => "vpsubw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xfa 11 ggg rrr => "vpsubd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xfa @addr {$rexb=1-$b;$rexx=1-$k} => "vpsubd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xfb 11 ggg rrr => "vpsubq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xfb @addr {$rexb=1-$b;$rexx=1-$k} => "vpsubq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xdb 11 ggg rrr => "vpand " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xdb @addr {$rexb=1-$b;$rexx=1-$k} => "vpand " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xdf 11 ggg rrr => "vpandn " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xdf @addr {$rexb=1-$b;$rexx=1-$k} => "vpandn " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xeb 11 ggg rrr => "vpor " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xeb @addr {$rexb=1-$b;$rexx=1-$k} => "vpor " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xef 11 ggg rrr => "vpxor " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xef @addr {$rexb=1-$b;$rexx=1-$k} => "vpxor " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0x74 11 ggg rrr => "vpcmpeqb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0x74 @addr {$rexb=1-$b;$rexx=1-$k} => "vpcmpeqb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0x75 11 ggg rrr => "vpcmpeqw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0x75 @addr {$rexb=1-$b;$rexx=1-$k} => "vpcmpeqw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0x76 11 ggg rrr => "vpcmpeqd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0x76 @addr {$rexb=1-$b;$rexx=1-$k} => "vpcmpeqd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0x64 11 ggg rrr => "vpcmpgtb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0x64 @addr {$rexb=1-$b;$rexx=1-$k} => "vpcmpgtb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0x65 11 ggg rrr => "vpcmpgtw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0x65 @addr {$rexb=1-$b;$rexx=1-$k} => "vpcmpgtw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0x66 11 ggg rrr => "vpcmpgtd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0x66 @addr {$rexb=1-$b;$rexx=1-$k} => "vpcmpgtd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xd5 11 ggg rrr => "vpmullw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xd5 @addr {$rexb=1-$b;$rexx=1-$k} => "vpmullw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xe5 11 ggg rrr => "vpmulhw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xe5 @addr {$rexb=1-$b;$rexx=1-$k} => "vpmulhw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xf5 11 ggg rrr => "vpmaddwd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xf5 @addr {$rexb=1-$b;$rexx=1-$k} => "vpmaddwd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0x60 11 ggg rrr => "vpunpcklbw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0x60 @addr {$rexb=1-$b;$rexx=1-$k} => "vpunpcklbw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0x61 11 ggg rrr => "vpunpcklwd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0x61 @addr {$rexb=1-$b;$rexx=1-$k} => "vpunpcklwd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0x62 11 ggg rrr => "vpunpckldq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0x62 @addr {$rexb=1-$b;$rexx=1-$k} => "vpunpckldq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0x6c 11 ggg rrr => "vpunpcklqdq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0x6c @addr {$rexb=1-$b;$rexx=1-$k} => "vpunpcklqdq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0x68 11 ggg rrr => "vpunpckhbw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0x68 @addr {$rexb=1-$b;$rexx=1-$k} => "vpunpckhbw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0x6d 11 ggg rrr => "vpunpckhqdq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0x6d @addr {$rexb=1-$b;$rexx=1-$k} => "vpunpckhqdq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xdc 11 ggg rrr => "vpaddusb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xdc @addr {$rexb=1-$b;$rexx=1-$k} => "vpaddusb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xde 11 ggg rrr => "vpmaxub " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xde @addr {$rexb=1-$b;$rexx=1-$k} => "vpmaxub " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xda 11 ggg rrr => "vpminub " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xda @addr {$rexb=1-$b;$rexx=1-$k} => "vpminub " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xe0 11 ggg rrr => "vpavgb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xe0 @addr {$rexb=1-$b;$rexx=1-$k} => "vpavgb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xe3 11 ggg rrr => "vpavgw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xe3 @addr {$rexb=1-$b;$rexx=1-$k} => "vpavgw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0x63 11 ggg rrr => "vpacksswb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0x63 @addr {$rexb=1-$b;$rexx=1-$k} => "vpacksswb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0x67 11 ggg rrr => "vpackuswb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0x67 @addr {$rexb=1-$b;$rexx=1-$k} => "vpackuswb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y pp 0xc2 11 ggg rrr @imm8 => "vcmp" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00001 0 vvvv y pp 0xc2 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vcmp" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h k b 00001 0 vvvv y pp 0xc6 11 ggg rrr @imm8 => "vshuf" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00001 0 vvvv y pp 0xc6 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vshuf" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  1 1 1 00001 1 1111 0 11 0x92 11 ggg rrr => "kmovq " kreg[$g] "," greg[32+$r] ;
  1 1 1 00001 1 1111 0 11 0x93 11 ggg rrr => "kmovq " greg[32+$g] "," kreg[$r] ;
  1 1 1 00001 1 1111 0 11 0x90 11 ggg rrr => "kmovq " kreg[$g] "," kreg[$r] ;
  1 1 1 00011 1 1111 0 01 0x32 11 ggg rrr @imm8 => "kshiftlw " kreg[$g] "," kreg[$r] "," hex($imm8) ;
  1 1 1 00011 1 1111 0 01 0x30 11 ggg rrr @imm8 => "kshiftrw " kreg[$g] "," kreg[$r] "," hex($imm8) ;
}
submatch vex2 {
  h vvvv y pp 0x54 11 ggg rrr => "vand" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y pp 0x54 @addr => "vand" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y pp 0x55 11 ggg rrr => "vandn" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y pp 0x55 @addr => "vandn" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y pp 0x56 11 ggg rrr => "vor" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y pp 0x56 @addr => "vor" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y pp 0x57 11 ggg rrr => "vxor" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y pp 0x57 @addr => "vxor" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y pp 0x58 11 ggg rrr => "vadd" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y pp 0x58 @addr => "vadd" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y pp 0x59 11 ggg rrr => "vmul" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y pp 0x59 @addr => "vmul" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y pp 0x5c 11 ggg rrr => "vsub" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y pp 0x5c @addr => "vsub" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y pp 0x5d 11 ggg rrr => "vmin" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y pp 0x5d @addr => "vmin" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y pp 0x5e 11 ggg rrr => "vdiv" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y pp 0x5e @addr => "vdiv" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y pp 0x5f 11 ggg rrr => "vmax" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y pp 0x5f @addr => "vmax" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h 1111 y pp 0x28 11 ggg rrr => vmova[$p] " " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y pp 0x28 @addr => vmova[$p] " " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y pp 0x29 @addr => vmova[$p] " " $addr "," vreg[16*$y+8*$h+$g] ;
  h 1111 y pp 0x10 11 ggg rrr => vmovu[$p] " " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y pp 0x10 @addr => vmovu[$p] " " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y pp 0x11 @addr => vmovu[$p] " " $addr "," vreg[16*$y+8*$h+$g] ;
  h 1111 y pp 0x6f 11 ggg rrr => vmdq[$p] " " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y pp 0x6f @addr => vmdq[$p] " " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y pp 0x7f @addr => vmdq[$p] " " $addr "," vreg[16*$y+8*$h+$g] ;
  h vvvv y 01 0xfc 11 ggg rrr => "vpaddb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xfc @addr => "vpaddb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xfd 11 ggg rrr => "vpaddw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xfd @addr => "vpaddw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xfe 11 ggg rrr => "vpaddd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xfe @addr => "vpaddd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xd4 11 ggg rrr => "vpaddq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xd4 @addr => "vpaddq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xf8 11 ggg rrr => "vpsubb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xf8 @addr => "vpsubb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xf9 11 ggg rrr => "vpsubw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xf9 @addr => "vpsubw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xfa 11 ggg rrr => "vpsubd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xfa @addr => "vpsubd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xfb 11 ggg rrr => "vpsubq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xfb @addr => "vpsubq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xdb 11 ggg rrr => "vpand " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xdb @addr => "vpand " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xdf 11 ggg rrr => "vpandn " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xdf @addr => "vpandn " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xeb 11 ggg rrr => "vpor " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xeb @addr => "vpor " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xef 11 ggg rrr => "vpxor " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xef @addr => "vpxor " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x74 11 ggg rrr => "vpcmpeqb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0x74 @addr => "vpcmpeqb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x75 11 ggg rrr => "vpcmpeqw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0x75 @addr => "vpcmpeqw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x76 11 ggg rrr => "vpcmpeqd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0x76 @addr => "vpcmpeqd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x64 11 ggg rrr => "vpcmpgtb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0x64 @addr => "vpcmpgtb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x65 11 ggg rrr => "vpcmpgtw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0x65 @addr => "vpcmpgtw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x66 11 ggg rrr => "vpcmpgtd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0x66 @addr => "vpcmpgtd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xd5 11 ggg rrr => "vpmullw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xd5 @addr => "vpmullw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xe5 11 ggg rrr => "vpmulhw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xe5 @addr => "vpmulhw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xf5 11 ggg rrr => "vpmaddwd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xf5 @addr => "vpmaddwd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x60 11 ggg rrr => "vpunpcklbw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0x60 @addr => "vpunpcklbw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x61 11 ggg rrr => "vpunpcklwd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0x61 @addr => "vpunpcklwd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x62 11 ggg rrr => "vpunpckldq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0x62 @addr => "vpunpckldq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x6c 11 ggg rrr => "vpunpcklqdq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0x6c @addr => "vpunpcklqdq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x68 11 ggg rrr => "vpunpckhbw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0x68 @addr => "vpunpckhbw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x6d 11 ggg rrr => "vpunpckhqdq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0x6d @addr => "vpunpckhqdq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xdc 11 ggg rrr => "vpaddusb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xdc @addr => "vpaddusb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xde 11 ggg rrr => "vpmaxub " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xde @addr => "vpmaxub " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xda 11 ggg rrr => "vpminub " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xda @addr => "vpminub " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xe0 11 ggg rrr => "vpavgb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xe0 @addr => "vpavgb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xe3 11 ggg rrr => "vpavgw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xe3 @addr => "vpavgw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x63 11 ggg rrr => "vpacksswb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0x63 @addr => "vpacksswb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x67 11 ggg rrr => "vpackuswb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0x67 @addr => "vpackuswb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y pp 0xc2 11 ggg rrr @imm8 => "vcmp" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] "," hex($imm8) ;
  h vvvv y pp 0xc2 @addr @imm8 => "vcmp" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h vvvv y pp 0xc6 11 ggg rrr @imm8 => "vshuf" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] "," hex($imm8) ;
  h vvvv y pp 0xc6 @addr @imm8 => "vshuf" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  1 1111 0 pp 0x90 11 ggg rrr => kmov_t[$p] " " kreg[$g] "," kreg[$r] ;
  1 1111 0 pp 0x90 @addr => kmov_t[$p] " " kreg[$g] "," $addr ;
  1 1111 0 pp 0x91 @addr => kmov_t[$p] " " $addr "," kreg[$g] ;
  1 1111 0 pp 0x92 11 ggg rrr => kmov_t[$p] " " kreg[$g] "," greg[$r] ;
  1 1111 0 pp 0x93 11 ggg rrr => kmov_t[$p] " " greg[$g] "," kreg[$r] ;
  1 vvvv 1 00 0x41 11 ggg rrr => "kandw " kreg[$g] "," kreg[15-$v] "," kreg[$r] ;
  1 vvvv 1 01 0x41 11 ggg rrr => "kandb " kreg[$g] "," kreg[15-$v] "," kreg[$r] ;
  1 vvvv 1 00 0x42 11 ggg rrr => "kandnw " kreg[$g] "," kreg[15-$v] "," kreg[$r] ;
  1 vvvv 1 01 0x42 11 ggg rrr => "kandnb " kreg[$g] "," kreg[15-$v] "," kreg[$r] ;
  1 vvvv 1 00 0x45 11 ggg rrr => "korw " kreg[$g] "," kreg[15-$v] "," kreg[$r] ;
  1 vvvv 1 01 0x45 11 ggg rrr => "korb " kreg[$g] "," kreg[15-$v] "," kreg[$r] ;
  1 vvvv 1 00 0x47 11 ggg rrr => "kxorw " kreg[$g] "," kreg[15-$v] "," kreg[$r] ;
  1 vvvv 1 01 0x47 11 ggg rrr => "kxorb " kreg[$g] "," kreg[15-$v] "," kreg[$r] ;
  1 vvvv 1 00 0x46 11 ggg rrr => "kxnorw " kreg[$g] "," kreg[15-$v] "," kreg[$r] ;
  1 vvvv 1 01 0x46 11 ggg rrr => "kxnorb " kreg[$g] "," kreg[15-$v] "," kreg[$r] ;
  1 vvvv 1 00 0x4a 11 ggg rrr => "kaddw " kreg[$g] "," kreg[15-$v] "," kreg[$r] ;
  1 vvvv 1 01 0x4a 11 ggg rrr => "kaddb " kreg[$g] "," kreg[15-$v] "," kreg[$r] ;
  1 vvvv 1 01 0x4b 11 ggg rrr => "kunpckbw " kreg[$g] "," kreg[15-$v] "," kreg[$r] ;
  1 1111 0 00 0x44 11 ggg rrr => "knotw " kreg[$g] "," kreg[$r] ;
  1 1111 0 01 0x44 11 ggg rrr => "knotb " kreg[$g] "," kreg[$r] ;
  1 1111 0 00 0x98 11 ggg rrr => "kortestw " kreg[$g] "," kreg[$r] ;
  1 1111 0 00 0x99 11 ggg rrr => "ktestw " kreg[$g] "," kreg[$r] ;
  1 1111 0 00 0x77 => "vzeroupper" ;
  1 1111 1 00 0x77 => "vzeroall" ;
}

submatch evex {
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x58 11 ggg rrr => wit("evex") "vaddps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 00 z ll 1 u aaa 0x58 11 ggg rrr => wit("evex") "vaddps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x58 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vaddps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 00 z ll 1 u aaa 0x58 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vaddps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x58 11 ggg rrr => wit("evex") "vaddpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0x58 11 ggg rrr => wit("evex") "vaddpd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x58 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vaddpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0x58 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vaddpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x59 11 ggg rrr => wit("evex") "vmulps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 00 z ll 1 u aaa 0x59 11 ggg rrr => wit("evex") "vmulps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x59 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmulps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 00 z ll 1 u aaa 0x59 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmulps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x59 11 ggg rrr => wit("evex") "vmulpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0x59 11 ggg rrr => wit("evex") "vmulpd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x59 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmulpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0x59 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmulpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x5c 11 ggg rrr => wit("evex") "vsubps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 00 z ll 1 u aaa 0x5c 11 ggg rrr => wit("evex") "vsubps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x5c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vsubps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 00 z ll 1 u aaa 0x5c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vsubps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x5c 11 ggg rrr => wit("evex") "vsubpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0x5c 11 ggg rrr => wit("evex") "vsubpd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x5c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vsubpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0x5c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vsubpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x5e 11 ggg rrr => wit("evex") "vdivps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 00 z ll 1 u aaa 0x5e 11 ggg rrr => wit("evex") "vdivps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x5e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vdivps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 00 z ll 1 u aaa 0x5e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vdivps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x5e 11 ggg rrr => wit("evex") "vdivpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0x5e 11 ggg rrr => wit("evex") "vdivpd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x5e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vdivpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0x5e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vdivpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x5d 11 ggg rrr => wit("evex") "vminps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x5d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vminps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 00 z ll 1 u aaa 0x5d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vminps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x5d 11 ggg rrr => wit("evex") "vminpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x5d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vminpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0x5d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vminpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x5f 11 ggg rrr => wit("evex") "vmaxps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x5f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmaxps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 00 z ll 1 u aaa 0x5f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmaxps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x5f 11 ggg rrr => wit("evex") "vmaxpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x5f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmaxpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0x5f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmaxpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x54 11 ggg rrr => wit("evex") "vandps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x54 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vandps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 00 z ll 1 u aaa 0x54 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vandps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x54 11 ggg rrr => wit("evex") "vandpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x54 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vandpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0x54 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vandpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x55 11 ggg rrr => wit("evex") "vandnps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x55 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vandnps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 00 z ll 1 u aaa 0x55 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vandnps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x55 11 ggg rrr => wit("evex") "vandnpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x55 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vandnpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0x55 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vandnpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x56 11 ggg rrr => wit("evex") "vorps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x56 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vorps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 00 z ll 1 u aaa 0x56 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vorps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x56 11 ggg rrr => wit("evex") "vorpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x56 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vorpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0x56 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vorpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x57 11 ggg rrr => wit("evex") "vxorps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x57 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vxorps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 00 z ll 1 u aaa 0x57 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vxorps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x57 11 ggg rrr => wit("evex") "vxorpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x57 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vxorpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0x57 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vxorpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 01 0 vvvv 1 10 z ll 0 u aaa 0x58 11 ggg rrr => wit("evex") "vaddss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 10 z ll 1 u aaa 0x58 11 ggg rrr => wit("evex") "vaddss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 0 vvvv 1 10 z ll 0 u aaa 0x58 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vaddss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 11 z ll 0 u aaa 0x58 11 ggg rrr => wit("evex") "vaddsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 11 z ll 1 u aaa 0x58 11 ggg rrr => wit("evex") "vaddsd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 1 vvvv 1 11 z ll 0 u aaa 0x58 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vaddsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 10 z ll 0 u aaa 0x59 11 ggg rrr => wit("evex") "vmulss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 10 z ll 1 u aaa 0x59 11 ggg rrr => wit("evex") "vmulss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 0 vvvv 1 10 z ll 0 u aaa 0x59 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmulss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 11 z ll 0 u aaa 0x59 11 ggg rrr => wit("evex") "vmulsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 11 z ll 1 u aaa 0x59 11 ggg rrr => wit("evex") "vmulsd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 1 vvvv 1 11 z ll 0 u aaa 0x59 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmulsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 10 z ll 0 u aaa 0x5c 11 ggg rrr => wit("evex") "vsubss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 10 z ll 1 u aaa 0x5c 11 ggg rrr => wit("evex") "vsubss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 0 vvvv 1 10 z ll 0 u aaa 0x5c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vsubss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 11 z ll 0 u aaa 0x5c 11 ggg rrr => wit("evex") "vsubsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 11 z ll 1 u aaa 0x5c 11 ggg rrr => wit("evex") "vsubsd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 1 vvvv 1 11 z ll 0 u aaa 0x5c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vsubsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 10 z ll 0 u aaa 0x5e 11 ggg rrr => wit("evex") "vdivss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 10 z ll 1 u aaa 0x5e 11 ggg rrr => wit("evex") "vdivss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 0 vvvv 1 10 z ll 0 u aaa 0x5e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vdivss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 11 z ll 0 u aaa 0x5e 11 ggg rrr => wit("evex") "vdivsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 11 z ll 1 u aaa 0x5e 11 ggg rrr => wit("evex") "vdivsd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 1 vvvv 1 11 z ll 0 u aaa 0x5e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vdivsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x28 11 ggg rrr => wit("evex") "vmovaps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x28 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovaps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x29 11 ggg rrr => wit("evex") wit("alt") "vmovaps " ereg[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x29 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovaps " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x28 11 ggg rrr => wit("evex") "vmovapd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x28 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovapd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x29 11 ggg rrr => wit("evex") wit("alt") "vmovapd " ereg[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x29 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovapd " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x10 11 ggg rrr => wit("evex") "vmovups " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x10 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovups " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x11 11 ggg rrr => wit("evex") wit("alt") "vmovups " ereg[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x11 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovups " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x10 11 ggg rrr => wit("evex") "vmovupd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x10 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovupd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x11 11 ggg rrr => wit("evex") wit("alt") "vmovupd " ereg[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x11 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovupd " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 0 1111 1 01 z ll 0 1 aaa 0x6f 11 ggg rrr => wit("evex") "vmovdqa32 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 01 z ll 0 1 aaa 0x6f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovdqa32 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 01 z ll 0 1 aaa 0x7f 11 ggg rrr => wit("evex") wit("alt") "vmovdqa32 " ereg[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 0 1111 1 01 z ll 0 1 aaa 0x7f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovdqa32 " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x6f 11 ggg rrr => wit("evex") "vmovdqa64 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x6f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovdqa64 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x7f 11 ggg rrr => wit("evex") wit("alt") "vmovdqa64 " ereg[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x7f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovdqa64 " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 0 1111 1 10 z ll 0 1 aaa 0x6f 11 ggg rrr => wit("evex") "vmovdqu32 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 10 z ll 0 1 aaa 0x6f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovdqu32 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 10 z ll 0 1 aaa 0x7f 11 ggg rrr => wit("evex") wit("alt") "vmovdqu32 " ereg[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 0 1111 1 10 z ll 0 1 aaa 0x7f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovdqu32 " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 1 1111 1 10 z ll 0 1 aaa 0x6f 11 ggg rrr => wit("evex") "vmovdqu64 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 1111 1 10 z ll 0 1 aaa 0x6f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovdqu64 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 1 1111 1 10 z ll 0 1 aaa 0x7f 11 ggg rrr => wit("evex") wit("alt") "vmovdqu64 " ereg[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 1 1111 1 10 z ll 0 1 aaa 0x7f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovdqu64 " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 0 1111 1 11 z ll 0 1 aaa 0x6f 11 ggg rrr => wit("evex") "vmovdqu8 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 11 z ll 0 1 aaa 0x6f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovdqu8 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 11 z ll 0 1 aaa 0x7f 11 ggg rrr => wit("evex") wit("alt") "vmovdqu8 " ereg[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 0 1111 1 11 z ll 0 1 aaa 0x7f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovdqu8 " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 1 1111 1 11 z ll 0 1 aaa 0x6f 11 ggg rrr => wit("evex") "vmovdqu16 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 1111 1 11 z ll 0 1 aaa 0x6f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovdqu16 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 1 1111 1 11 z ll 0 1 aaa 0x7f 11 ggg rrr => wit("evex") wit("alt") "vmovdqu16 " ereg[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 1 1111 1 11 z ll 0 1 aaa 0x7f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovdqu16 " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xfc 11 ggg rrr => wit("evex") "vpaddb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xfc @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpaddb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xfd 11 ggg rrr => wit("evex") "vpaddw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xfd @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpaddw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xfe 11 ggg rrr => wit("evex") "vpaddd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xfe @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpaddd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 1 u aaa 0xfe @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpaddd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0xd4 11 ggg rrr => wit("evex") "vpaddq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0xd4 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpaddq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0xd4 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpaddq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xf8 11 ggg rrr => wit("evex") "vpsubb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xf8 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsubb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xf9 11 ggg rrr => wit("evex") "vpsubw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xf9 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsubw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xfa 11 ggg rrr => wit("evex") "vpsubd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xfa @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsubd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 1 u aaa 0xfa @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsubd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0xfb 11 ggg rrr => wit("evex") "vpsubq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0xfb @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsubq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0xfb @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsubq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xdb 11 ggg rrr => wit("evex") "vpandd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xdb @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpandd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 1 u aaa 0xdb @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpandd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0xdb 11 ggg rrr => wit("evex") "vpandq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0xdb @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpandq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0xdb @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpandq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xdf 11 ggg rrr => wit("evex") "vpandnd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xdf @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpandnd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 1 u aaa 0xdf @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpandnd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0xdf 11 ggg rrr => wit("evex") "vpandnq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0xdf @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpandnq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0xdf @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpandnq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xeb 11 ggg rrr => wit("evex") "vpord " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xeb @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpord " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 1 u aaa 0xeb @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpord " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0xeb 11 ggg rrr => wit("evex") "vporq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0xeb @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vporq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0xeb @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vporq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xef 11 ggg rrr => wit("evex") "vpxord " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xef @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpxord " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 1 u aaa 0xef @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpxord " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0xef 11 ggg rrr => wit("evex") "vpxorq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0xef @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpxorq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0xef @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpxorq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x40 11 ggg rrr => wit("evex") "vpmulld " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x40 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmulld " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x40 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmulld " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xd5 11 ggg rrr => wit("evex") "vpmullw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xd5 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmullw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xa8 11 ggg rrr => wit("evex") "vfmadd213ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xa8 11 ggg rrr => wit("evex") "vfmadd213ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xa8 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd213ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xa8 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd213ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xa8 11 ggg rrr => wit("evex") "vfmadd213pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xa8 11 ggg rrr => wit("evex") "vfmadd213pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xa8 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd213pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xa8 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd213pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xb8 11 ggg rrr => wit("evex") "vfmadd231ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xb8 11 ggg rrr => wit("evex") "vfmadd231ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xb8 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd231ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xb8 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd231ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xb8 11 ggg rrr => wit("evex") "vfmadd231pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xb8 11 ggg rrr => wit("evex") "vfmadd231pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xb8 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd231pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xb8 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd231pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x98 11 ggg rrr => wit("evex") "vfmadd132ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x98 11 ggg rrr => wit("evex") "vfmadd132ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x98 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd132ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x98 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd132ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x98 11 ggg rrr => wit("evex") "vfmadd132pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x98 11 ggg rrr => wit("evex") "vfmadd132pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x98 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd132pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x98 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd132pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xaa 11 ggg rrr => wit("evex") "vfmsub213ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xaa 11 ggg rrr => wit("evex") "vfmsub213ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xaa @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub213ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xaa @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub213ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xaa 11 ggg rrr => wit("evex") "vfmsub213pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xaa 11 ggg rrr => wit("evex") "vfmsub213pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xaa @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub213pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xaa @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub213pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xac 11 ggg rrr => wit("evex") "vfnmadd213ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xac 11 ggg rrr => wit("evex") "vfnmadd213ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xac @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd213ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xac @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd213ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xac 11 ggg rrr => wit("evex") "vfnmadd213pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xac 11 ggg rrr => wit("evex") "vfnmadd213pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xac @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd213pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xac @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd213pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  1 k b 1 00 01 0 vvvv 1 01 0 ll 0 u aaa 0x76 11 ggg rrr => wit("evex") "vpcmpeqd " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  1 k b 1 00 01 0 vvvv 1 01 0 ll 0 u aaa 0x76 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpcmpeqd " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  1 k b 1 00 01 0 vvvv 1 01 0 ll 0 u aaa 0x66 11 ggg rrr => wit("evex") "vpcmpgtd " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  1 k b 1 00 01 0 vvvv 1 01 0 ll 0 u aaa 0x66 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpcmpgtd " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  1 k b 1 00 01 0 vvvv 1 00 0 ll 0 u aaa 0xc2 11 ggg rrr @imm8 => wit("evex") "vcmpps " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  1 k b 1 00 01 0 vvvv 1 00 0 ll 0 u aaa 0xc2 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vcmpps " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  1 k b 1 00 01 1 vvvv 1 01 0 ll 0 u aaa 0xc2 11 ggg rrr @imm8 => wit("evex") "vcmppd " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  1 k b 1 00 01 1 vvvv 1 01 0 ll 0 u aaa 0xc2 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vcmppd " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  1 k b 1 00 11 0 vvvv 1 01 0 ll 0 u aaa 0x1f 11 ggg rrr @imm8 => wit("evex") "vpcmpd " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  1 k b 1 00 11 0 vvvv 1 01 0 ll 0 u aaa 0x1f @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpcmpd " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  1 k b 1 00 11 0 vvvv 1 01 0 ll 0 u aaa 0x1e 11 ggg rrr @imm8 => wit("evex") "vpcmpud " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  1 k b 1 00 11 0 vvvv 1 01 0 ll 0 u aaa 0x1e @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpcmpud " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  1 k b 1 00 11 1 vvvv 1 01 0 ll 0 u aaa 0x1f 11 ggg rrr @imm8 => wit("evex") "vpcmpq " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  1 k b 1 00 11 1 vvvv 1 01 0 ll 0 u aaa 0x1f @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpcmpq " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  1 k b 1 00 11 1 vvvv 1 01 0 ll 0 u aaa 0x1e 11 ggg rrr @imm8 => wit("evex") "vpcmpuq " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  1 k b 1 00 11 1 vvvv 1 01 0 ll 0 u aaa 0x1e @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpcmpuq " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  1 k b 1 00 10 0 vvvv 1 01 0 ll 0 u aaa 0x27 11 ggg rrr => wit("evex") "vptestmd " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  1 k b 1 00 10 0 vvvv 1 01 0 ll 0 u aaa 0x27 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vptestmd " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  1 k b 1 00 10 1 vvvv 1 01 0 ll 0 u aaa 0x27 11 ggg rrr => wit("evex") "vptestmq " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  1 k b 1 00 10 1 vvvv 1 01 0 ll 0 u aaa 0x27 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vptestmq " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
}

submatch main { @pfx(0) => $pfx }
