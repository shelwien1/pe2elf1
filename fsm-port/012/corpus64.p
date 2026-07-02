# corpus64.p -- x86-64 (long mode) bit-pattern -> string rewriter.
#
# Companion to corpus.p (x86-32), in the SAME DSL (corpus-p-syntax.md), driven by
# the SAME generators:  gen.py corpus64.p x86_tables.h   /   gasm.py corpus64.p x86_tables_enc.h
#
# x86-64 deltas over corpus.p:
#  * REX prefix (0x40..0x4F): a no-token prefix frame that sets $rexw/r/x/b/$rex.
#    Like every prefix it is captured raw in pfx[] and replayed by the encoder, so
#    the REX byte (and its R/X/B high register bits) round-trips byte-exactly; the
#    decoder also adds the high bit into the operand register *number* (fill_insn).
#  * 16 GP registers: register operands carry only the low 3 bits here; the 4th
#    bit (REX.R/X/B) is folded in by the C++ fill_insn, so a template just names
#    the register file (greg/rgb/ssereg) -- the rendered index is a render concern.
#  * 64-bit addressing: mod=00 rm=101 is [rip+disp32]; absolute is the SIB no-base
#    form; no 16-bit addressing ($adrsiz=1 via 67 is 32-bit).
#  * movsxd (0x63), movabs (B8+r REX.W -> imm64), default-64 stack/branch group.
#
# Coverage: the core integer ISA (ALU, mov/lea, push/pop, inc/dec, test/xchg,
# shifts, imul, jmp/call/jcc, ret, movzx/movsx/movsxd, setcc/cmovcc, groups,
# string ops, bit ops/bsf/bsr/bswap, cmpxchg/xadd, cpuid/rdtsc, fences/prefetch,
# CET endbr) plus the legacy SSE family -- SSE, SSE2, SSE3, SSSE3, SSE4.1/4.2:
# data movement, packed/scalar float arithmetic, the full packed-integer set
# (add/sub/mul/min/max/avg/sad/cmp/pack/unpack/shift incl. the 71-73 imm groups),
# movd/movq, cvt*, movmsk/pmovmskb, pinsr/pextr, and the 0F38/0F3A maps (pshufb,
# palignr, pmovsx/zx, roundsd, blends, pcmpistri, pclmulqdq, AES), plus x87
# (D8-DF). The mandatory 66/F3/F2 SSE selector and rep/lock ride the prefix run
# (replayed verbatim), so prefix variants round-trip byte-exact.
#
# VEX (AVX/AVX2) and EVEX (AVX-512) are now decoded SEMANTICALLY, not just byte-
# round-tripped: the vex/vex2/evex submatches below are compiled by gen.py into
# capture-based FSM stages (op1[C4/C5/62] -> vexp*/evexp* -> per-(map,pp,W)
# opcode tables). The VEX/EVEX prefix fields ride the SAME 32 capture slots as
# the legacy decode -- no slot is live on more than one path, so they alias onto
# the unused ones (vvvv->REL, L/L'L->CC, R'X'B'->TBL3) -- and the C++ vex_finalize
# folds in the R/X/B/R'/V' register extensions + the EVEX mask/zeroing/broadcast/
# rounding decorations. XOP (8F) is decoded the same FSM-native way (op1[0x8F] ->
# xopp1/xopp2/xopop after the C++ POP/XOP split): the full SIMD XOP set is covered
# -- map 8 (vpmacs*/vpmadcs*/vpcmov/vpperm/vpcom*) and the vector map 9 (vfrcz*,
# vprot/vpsh[al] register forms incl. the W=1 operand swap, vphadd*/vphsub*).
# Coverage: VEX 100% of the rules, EVEX ~98% (the rare EVEX memory-source
# shift-by-imm and the 256-bit vextract* lane ops remain structural), XOP 100%.
# The XOP GPR families -- TBM (blc*/bls*/tzmsk/t1mskc on 09.01/02), LWP
# (llwpcb/slwpcb on 09.12, lwpins/lwpval on 0A.12) and the XOP-encoded bextr
# (0A.10) -- are AMD-only (Bulldozer-family), but real silicon and objdump both
# decode them, so they are lowered semantically too: the reg-fixed group cells
# reuse the VEX /digit mechanism (VEX_MG / VEX_VMG0 for the no-imm and single-
# operand shapes) with GPR dest riding vvvv/reg and a control imm32. APX EVEX-promoted legacy (62 with
# the 3-bit map == 4) is decoded the same FSM-native way (evexp0 -> apxp1/apxp2/
# apxop; the C++ apx_finalize folds the r0-31 extension + sizing + the ND new-data-
# destination and NF no-flags modifiers). The full integer set is covered: the ALU
# (add/or/adc/sbb/and/sub/xor/cmp, 8/16/32/64, MR/RM/MI), cmovcc, the shift/rotate
# and inc/dec/not/neg groups, imul, shld/shrd and the push2/pop2 register pair --
# all with NDD/NF and r0-31. The remaining map-4 tail uses pp as a MANDATORY-PREFIX
# mnemonic selector (not the size override the promoted ALU uses) and/or SSE-domain
# operands -- adcx/adox, movbe, crc32, the aadd/aand/axor/aor atomics, movdiri/
# movdir64b, sha*/aes*kl, invept/invvpid/invpcid, u{rd,wr}msr, wrssq/wrussq -- so it
# does not fit the uniform model and stays structural. The APX REX2 (D5) prefix now
# decodes SEMANTICALLY over the full 0F map (M0=1 routes to FSM_OP2): SSE/MMX, mov
# cr/dr, cpuid/bt, push/pop fs/gs, pinsrw/pextrw/punpckldq -- all with r0-31 registers,
# via a unified C++ mov-cr/dr handler and the R3.R4/B3.B4 extension in fill_insn. An
# opcode the corpus does not cover and the moffs mov (A0-A3) still round-trip via the
# C++ structural path (raw-byte replay). The
# fuzzer enforces lossless round-trip across the whole accepted byte space (0
# failures) -- the bijection holds for all of x64, and the covered VEX/EVEX/XOP/APX
# now lower to real mnemonics + operands.

arch  $mode=64 $endian=le $bitorder=msb $maxlen=15
# REX is NOT a var: the capture index is only 5 bits (32 slots) and the legacy
# var set already fills it. The REX byte is consumed into pfx[] and the C++
# extracts W/R/X/B from it (the last prefix byte).
vars  $opsiz=0 $adrsiz=0 $reptype=0 $segidx=0 $lock=0

# ---- register files. Operands store a size-independent NUMBER (0..15); the size
# bank (8/16/32/64) is the render's concern, picked from $rexw/$opsiz. The tables
# carry one name per number per size so the enum/renderer have names; the decode
# captures only the low 3 bits and fill_insn adds 8*REX.{R,X,B}. ----------------
table greg {
  eax,ecx,edx,ebx,esp,ebp,esi,edi,r8d,r9d,r10d,r11d,r12d,r13d,r14d,r15d,r16d,r17d,r18d,r19d,r20d,r21d,r22d,r23d,r24d,r25d,r26d,r27d,r28d,r29d,r30d,r31d,
  ax,cx,dx,bx,sp,bp,si,di,r8w,r9w,r10w,r11w,r12w,r13w,r14w,r15w,r16w,r17w,r18w,r19w,r20w,r21w,r22w,r23w,r24w,r25w,r26w,r27w,r28w,r29w,r30w,r31w,
  rax,rcx,rdx,rbx,rsp,rbp,rsi,rdi,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31
}
table rgb { al,cl,dl,bl,spl,bpl,sil,dil,r8b,r9b,r10b,r11b,r12b,r13b,r14b,r15b,r16b,r17b,r18b,r19b,r20b,r21b,r22b,r23b,r24b,r25b,r26b,r27b,r28b,r29b,r30b,r31b }
table ssereg { mm0,mm1,mm2,mm3,mm4,mm5,mm6,mm7,
  xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7,xmm8,xmm9,xmm10,xmm11,xmm12,xmm13,xmm14,xmm15 }
table sreg { es,cs,ss,ds,fs,gs }
table bndreg { bnd0,bnd1,bnd2,bnd3 }                     # MPX bound registers (0F 1A/1B)
table cond { o,no,b,ae,e,ne,be,a,s,ns,p,np,l,ge,le,g }

# size suffix : the dot is part of the entry; index = operand size in bytes
table sfx { "",".b",".w","",".d","","","",".q" }

# segment display : index = sbo(base)+segidx. Row sbo=0 is a DS-default base, row
# sbo=7 an SS-default base (rsp/rbp). 64-bit mostly ignores segments but fs/gs.
table seg { "","es:","cs:","ss:","","fs:","gs:",  "ss:","es:","cs:","ss:","ds:","fs:","gs:" }
table sbo { 0,0,0,0,7,7,0,0 }      # base reg number (low 3) -> seg row

# legacy-SSE mandatory-prefix mnemonic tables: entries are {NP, 66, F3, F2}.
# A rule `=> sseNN[$pp] ...` picks the entry by the mandatory prefix; invalid
# prefix slots repeat the NP mnemonic (lenient collapse, still round-trips).
table sse58 { addps, addpd, addss, addsd }
table sse2b { movntps, movntpd, movntss, movntsd }       # 0F 2B non-temporal store
table sret  { sysretd, sysretd~w, sysretq }              # 0F 07: d at 32-bit/data16, q at REX.W
table sxit  { sysexitd, sysexitd~w, sysexitq }           # 0F 35  (~w = distinct idx, "sysexitd" display)
table rdssp { rdsspd, rdsspd~w, rdsspq }                 # F3 0F 1E /1: CET read shadow-stack ptr
table incssp { incsspd, incsspd~w, incsspq }             # F3 0F AE /5: CET increment shadow-stack
# push/pop fs/gs (0F A0/A1/A8/A9): 66 -> 16-bit (pushw/popw); REX.W ignored (~q = "push" display)
table pushfs { "push fs", "pushw fs", "push fs~q" }
table popfs  { "pop fs", "popw fs", "pop fs~q" }
table pushgs { "push gs", "pushw gs", "push gs~q" }
table popgs  { "pop gs", "popw gs", "pop gs~q" }
table ssebc { bsf, bsf, tzcnt, bsf }
table ssebd { bsr, bsr, lzcnt, bsr }
table ssee6 { cvtpd2dq, cvttpd2dq, cvtdq2pd, cvtpd2dq }
# int<->float converts: NP/66 use an mm operand, F3/F2 a GPR (the mmxg operand
# file resolves mm-or-GPR by the mandatory prefix, mirroring cvtpi2ps vs cvtsi2ss).
table sse2a { cvtpi2ps, cvtpi2pd, cvtsi2ss, cvtsi2sd }
table sse2c { cvttps2pi, cvttpd2pi, cvttss2si, cvttsd2si }
table sse2d { cvtps2pi, cvtpd2pi, cvtss2si, cvtsd2si }
# mm/xmm data-movement + shuffle: NP is the MMX form, 66/F3/F2 the xmm forms
# (the SSE_OS operand picks mm/xmm; a rep prefix forces the xmm bank).
table sse6f { movq, movdqa, movdqu, movdqa }
table sse7f { movq, movdqa, movdqu, movdqa }
table sse70 { pshufw, pshufd, pshufhw, pshuflw }
table ssee7 { movntq, movntdq, movntq, movntq }
table ssef7 { maskmovq, maskmovdqu, maskmovq, maskmovq }  # 0F F7: mm at NP, xmm at 66
table sse10 { movups, movupd, movss, movsd }
table sse11 { movups, movupd, movss, movsd }
table sse14 { unpcklps, unpcklpd, unpcklps, unpcklps }
table sse15 { unpckhps, unpckhpd, unpckhps, unpckhps }
table sse28 { movaps, movapd, movaps, movaps }
table sse29 { movaps, movapd, movaps, movaps }
table sse2e { ucomiss, ucomisd, ucomiss, ucomiss }
table sse2f { comiss, comisd, comiss, comiss }
table sse50 { movmskps, movmskpd, movmskps, movmskps }
table sse51 { sqrtps, sqrtpd, sqrtss, sqrtsd }
table sse52 { rsqrtps, rsqrtps, rsqrtss, rsqrtps }
table sse53 { rcpps, rcpps, rcpss, rcpps }
table sse54 { andps, andpd, andps, andps }
table sse55 { andnps, andnpd, andnps, andnps }
table sse56 { orps, orpd, orps, orps }
table sse57 { xorps, xorpd, xorps, xorps }
table sse59 { mulps, mulpd, mulss, mulsd }
table sse5a { cvtps2pd, cvtpd2ps, cvtss2sd, cvtsd2ss }
table sse5b { cvtdq2ps, cvtps2dq, cvttps2dq, cvtdq2ps }
table sse5c { subps, subpd, subss, subsd }
table sse5d { minps, minpd, minss, minsd }
table sse5e { divps, divpd, divss, divsd }
table sse5f { maxps, maxpd, maxss, maxsd }
table sse7c { haddps, haddpd, haddps, haddps }
table sse7d { hsubps, hsubpd, hsubps, hsubps }
table ssec2 { cmpps, cmppd, cmpss, cmpsd }
table ssec6 { shufps, shufpd, shufps, shufps }
table ssed0 { addsubps, addsubpd, addsubps, addsubps }

# ---- immediates / displacements ----------------------------------------------
submatch imm8  { iiiiiiii                              => $i }
submatch imm16 { iiiiiiii iiiiiiii                     => $i }
submatch imm32 { iiiiiiii iiiiiiii iiiiiiii iiiiiiii   => $i }
submatch imm64 { iiiiiiii iiiiiiii iiiiiiii iiiiiiii iiiiiiii iiiiiiii iiiiiiii iiiiiiii => $i }
submatch disp8 { dddddddd                              => sx8($d) }
submatch disp32{ dddddddd dddddddd dddddddd dddddddd   => $d }
submatch rel8  { dddddddd                              => $E + sx8($d) }
submatch immz1($opsiz) { <0> @imm32 => $imm32 ;  <1> @imm16 => $imm16 }
submatch relz1($opsiz) { <0> @imm32 => $E+sx32($imm32) ;  <1> @imm16 => $E+sx16($imm16) }
submatch immz { @immz1($opsiz) => $immz1 }
submatch relz { @relz1($opsiz) => $relz1 }
submatch immq { @imm64 => $imm64 }
# mov r,imm operand-size immediate: imm16/imm32 here, and imm64 under REX.W
# (movabs). REX.W is not an FSM var, so the imm64 case is sized C++-side
# (append_imm/enc_imm key on IMK_IMMV); this body covers the non-REX.W widths.
submatch immv { @immz1($opsiz) => $immz1 }

# ---- prefix run : legacy prefixes + REX (REX last; a no-token frame) ----------
submatch pfx($d) {
  [$d >= $maxlen] => "" ;
  0x66 @pfx($d+1) {$opsiz=1}   => "opsiz "  $pfx ;
  0x67 @pfx($d+1) {$adrsiz=1}  => "adrsiz " $pfx ;
  0x2e @pfx($d+1) {$segidx=2}  => "segcs "  $pfx ;
  0x36 @pfx($d+1) {$segidx=3}  => "segss "  $pfx ;
  0x3e @pfx($d+1) {$segidx=4}  => "segds "  $pfx ;
  0x26 @pfx($d+1) {$segidx=1}  => "seges "  $pfx ;
  0x64 @pfx($d+1) {$segidx=5}  => "segfs "  $pfx ;
  0x65 @pfx($d+1) {$segidx=6}  => "seggs "  $pfx ;
  0xf3 @pfx($d+1) {$reptype=1} => "rep "    $pfx ;
  0xf2 @pfx($d+1) {$reptype=2} => "repnz "  $pfx ;
  0xf0 @pfx($d+1) {$lock=1}    => "lock "   $pfx ;
  # REX (0x40..0x4F): consumed raw into pfx[]; W/R/X/B re-read in C++. REX.W is
  # split out because it overrides the 66 prefix for operand size: $opsiz becomes
  # 3-valued (0=32, 1=16 via 66, 2=64 via REX.W). REX is the last prefix, so a
  # REX.W after a 66 lands $opsiz at 2 (REX.W wins). The C++ keys immediate width
  # on $opsiz==1 (-> 2 bytes) and reads the 64-bit operand/register bank for
  # $opsiz==2, so movabs and SSE GPR/xmm-bank ops size correctly.
  0100 0 r x b @pfx($d+1)            => $pfx ;
  0100 1 r x b @pfx($d+1) {$opsiz=2} => $pfx ;
       @insn => $insn ;
}

# ---- addressing : MEMORY ONLY (mod != 11). $adrsiz=0 -> 64-bit, =1 -> 32-bit
# (the 67 prefix). mod=00 rm=101 is RIP-relative; absolute is the SIB no-base
# form. Registers render through greg (number captured; size/REX folded later). --
submatch addr($g) { @addr1($adrsiz, $g) => $addr1 }
submatch addr1($adrsiz, $g) {
  <0> 00 ggg 100 @sib0          => seg[sbo[$sbase]+$segidx] $sib0 ;
  <0> 00 ggg 101 @disp32        => seg[$segidx] "[rip" sgn($disp32) "]" ;
  <0> 00 ggg rrr                => seg[sbo[$r]+$segidx] "[" greg[$r] "]" ;
  <0> 01 ggg 100 @sib1 @disp8   => seg[sbo[$sbase]+$segidx] "[" $sib1 sgn($disp8) "]" ;
  <0> 01 ggg rrr @disp8         => seg[sbo[$r]+$segidx] "[" greg[$r] sgn($disp8) "]" ;
  <0> 10 ggg 100 @sib1 @disp32  => seg[sbo[$sbase]+$segidx] "[" $sib1 sgn($disp32) "]" ;
  <0> 10 ggg rrr @disp32        => seg[sbo[$r]+$segidx] "[" greg[$r] sgn($disp32) "]" ;
  <1> 00 ggg 100 @sib0          => seg[sbo[$sbase]+$segidx] $sib0 ;
  <1> 00 ggg 101 @disp32        => seg[$segidx] "[eip" sgn($disp32) "]" ;
  <1> 00 ggg rrr                => seg[sbo[$r]+$segidx] "[" greg[$r] "]" ;
  <1> 01 ggg 100 @sib1 @disp8   => seg[sbo[$sbase]+$segidx] "[" $sib1 sgn($disp8) "]" ;
  <1> 01 ggg rrr @disp8         => seg[sbo[$r]+$segidx] "[" greg[$r] sgn($disp8) "]" ;
  <1> 10 ggg 100 @sib1 @disp32  => seg[sbo[$sbase]+$segidx] "[" $sib1 sgn($disp32) "]" ;
  <1> 10 ggg rrr @disp32        => seg[sbo[$r]+$segidx] "[" greg[$r] sgn($disp32) "]" ;
}
submatch sib0 {                              # mod=00; owns its disp32 (base==5)
  ss 100 101 @disp32 {$sbase=none} => "[" hex($disp32) "]" ;
  ss iii 101 @disp32 {$sbase=none} => "[" greg[$i] "*" dec(1<<$s) "+" hex($disp32) "]" ;
  ss 100 bbb         {$sbase=$b}   => "[" greg[$b] "]" ;
  ss iii bbb         {$sbase=$b}   => "[" greg[$b] "+" greg[$i] "*" dec(1<<$s) "]" ;
}
submatch sib1 {                              # mod=01/10; returns CORE, addr adds disp
  ss 100 bbb {$sbase=$b} => greg[$b] ;
  ss iii bbb {$sbase=$b} => greg[$b] "+" greg[$i] "*" dec(1<<$s) ;
}

# ============================ instruction decoder ===========================
submatch insn {
  # ===== ALU: add/or/adc/sbb/and/sub/xor/cmp =====================================
  # each: r/m8,r8 | r/m,r | r8,r/m8 | r,r/m | al,imm8 | eAX,immz
  0x00 11 ggg rrr => "add " rgb[$r] "," rgb[$g] ;
  0x00 @addr      => "add" sfx[1] " " $addr "," rgb[$g] ;
  0x01 11 ggg rrr => "add " greg[$r] "," greg[$g] ;
  0x01 @addr      => "add" sfx[4] " " $addr "," greg[$g] ;
  0x02 11 ggg rrr => "add " rgb[$g] "," rgb[$r] ;
  0x02 @addr      => "add " rgb[$g] "," $addr ;
  0x03 11 ggg rrr => "add " greg[$g] "," greg[$r] ;
  0x03 @addr      => "add " greg[$g] "," $addr ;
  0x04 @imm8      => "add al," hex($imm8) ;
  0x05 @immz      => "add eax," hex($immz) ;
  0x08 11 ggg rrr => "or " rgb[$r] "," rgb[$g] ;
  0x08 @addr      => "or" sfx[1] " " $addr "," rgb[$g] ;
  0x09 11 ggg rrr => "or " greg[$r] "," greg[$g] ;
  0x09 @addr      => "or" sfx[4] " " $addr "," greg[$g] ;
  0x0a 11 ggg rrr => "or " rgb[$g] "," rgb[$r] ;
  0x0a @addr      => "or " rgb[$g] "," $addr ;
  0x0b 11 ggg rrr => "or " greg[$g] "," greg[$r] ;
  0x0b @addr      => "or " greg[$g] "," $addr ;
  0x0c @imm8      => "or al," hex($imm8) ;
  0x0d @immz      => "or eax," hex($immz) ;
  0x10 11 ggg rrr => "adc " rgb[$r] "," rgb[$g] ;
  0x10 @addr      => "adc" sfx[1] " " $addr "," rgb[$g] ;
  0x11 11 ggg rrr => "adc " greg[$r] "," greg[$g] ;
  0x11 @addr      => "adc" sfx[4] " " $addr "," greg[$g] ;
  0x12 11 ggg rrr => "adc " rgb[$g] "," rgb[$r] ;
  0x12 @addr      => "adc " rgb[$g] "," $addr ;
  0x13 11 ggg rrr => "adc " greg[$g] "," greg[$r] ;
  0x13 @addr      => "adc " greg[$g] "," $addr ;
  0x14 @imm8      => "adc al," hex($imm8) ;
  0x15 @immz      => "adc eax," hex($immz) ;
  0x18 11 ggg rrr => "sbb " rgb[$r] "," rgb[$g] ;
  0x18 @addr      => "sbb" sfx[1] " " $addr "," rgb[$g] ;
  0x19 11 ggg rrr => "sbb " greg[$r] "," greg[$g] ;
  0x19 @addr      => "sbb" sfx[4] " " $addr "," greg[$g] ;
  0x1a 11 ggg rrr => "sbb " rgb[$g] "," rgb[$r] ;
  0x1a @addr      => "sbb " rgb[$g] "," $addr ;
  0x1b 11 ggg rrr => "sbb " greg[$g] "," greg[$r] ;
  0x1b @addr      => "sbb " greg[$g] "," $addr ;
  0x1c @imm8      => "sbb al," hex($imm8) ;
  0x1d @immz      => "sbb eax," hex($immz) ;
  0x20 11 ggg rrr => "and " rgb[$r] "," rgb[$g] ;
  0x20 @addr      => "and" sfx[1] " " $addr "," rgb[$g] ;
  0x21 11 ggg rrr => "and " greg[$r] "," greg[$g] ;
  0x21 @addr      => "and" sfx[4] " " $addr "," greg[$g] ;
  0x22 11 ggg rrr => "and " rgb[$g] "," rgb[$r] ;
  0x22 @addr      => "and " rgb[$g] "," $addr ;
  0x23 11 ggg rrr => "and " greg[$g] "," greg[$r] ;
  0x23 @addr      => "and " greg[$g] "," $addr ;
  0x24 @imm8      => "and al," hex($imm8) ;
  0x25 @immz      => "and eax," hex($immz) ;
  0x28 11 ggg rrr => "sub " rgb[$r] "," rgb[$g] ;
  0x28 @addr      => "sub" sfx[1] " " $addr "," rgb[$g] ;
  0x29 11 ggg rrr => "sub " greg[$r] "," greg[$g] ;
  0x29 @addr      => "sub" sfx[4] " " $addr "," greg[$g] ;
  0x2a 11 ggg rrr => "sub " rgb[$g] "," rgb[$r] ;
  0x2a @addr      => "sub " rgb[$g] "," $addr ;
  0x2b 11 ggg rrr => "sub " greg[$g] "," greg[$r] ;
  0x2b @addr      => "sub " greg[$g] "," $addr ;
  0x2c @imm8      => "sub al," hex($imm8) ;
  0x2d @immz      => "sub eax," hex($immz) ;
  0x30 11 ggg rrr => "xor " rgb[$r] "," rgb[$g] ;
  0x30 @addr      => "xor" sfx[1] " " $addr "," rgb[$g] ;
  0x31 11 ggg rrr => "xor " greg[$r] "," greg[$g] ;
  0x31 @addr      => "xor" sfx[4] " " $addr "," greg[$g] ;
  0x32 11 ggg rrr => "xor " rgb[$g] "," rgb[$r] ;
  0x32 @addr      => "xor " rgb[$g] "," $addr ;
  0x33 11 ggg rrr => "xor " greg[$g] "," greg[$r] ;
  0x33 @addr      => "xor " greg[$g] "," $addr ;
  0x34 @imm8      => "xor al," hex($imm8) ;
  0x35 @immz      => "xor eax," hex($immz) ;
  0x38 11 ggg rrr => "cmp " rgb[$r] "," rgb[$g] ;
  0x38 @addr      => "cmp" sfx[1] " " $addr "," rgb[$g] ;
  0x39 11 ggg rrr => "cmp " greg[$r] "," greg[$g] ;
  0x39 @addr      => "cmp" sfx[4] " " $addr "," greg[$g] ;
  0x3a 11 ggg rrr => "cmp " rgb[$g] "," rgb[$r] ;
  0x3a @addr      => "cmp " rgb[$g] "," $addr ;
  0x3b 11 ggg rrr => "cmp " greg[$g] "," greg[$r] ;
  0x3b @addr      => "cmp " greg[$g] "," $addr ;
  0x3c @imm8      => "cmp al," hex($imm8) ;
  0x3d @immz      => "cmp eax," hex($immz) ;

  # ===== movsxd (0x63, replaces 32-bit arpl), push imm, imul imm ===============
  0x63 11 ggg rrr => "movsxd " greg[$g] "," greg[$r] ;
  0x63 @addr      => "movsxd " greg[$g] "," $addr ;
  0x68 @immz      => "push " hex($immz) ;
  0x6a @imm8      => "push " hex(sx8($imm8)) ;
  0x69 11 ggg rrr @immz => "imul " greg[$g] "," greg[$r] "," hex($immz) ;
  0x69 @addr      @immz => "imul " greg[$g] "," $addr "," hex($immz) ;
  0x6b 11 ggg rrr @imm8 => "imul " greg[$g] "," greg[$r] "," hex(sx8($imm8)) ;
  0x6b @addr      @imm8 => "imul " greg[$g] "," $addr "," hex(sx8($imm8)) ;

  # ===== Jcc rel8 (0111 cccc) ===================================================
  0111 cccc @rel8 => "j" cond[$c] " " hex($rel8) ;

  # ===== group 1 (80/81/83) : add/or/adc/sbb/and/sub/xor/cmp r/m, imm ==========
  0x80 11 000 rrr @imm8 => "add " rgb[$r] "," hex($imm8) ;
  0x80 @addr(0)   @imm8 => "add" sfx[1] " " $addr "," hex($imm8) ;
  0x80 11 001 rrr @imm8 => "or " rgb[$r] "," hex($imm8) ;
  0x80 @addr(1)   @imm8 => "or" sfx[1] " " $addr "," hex($imm8) ;
  0x80 11 010 rrr @imm8 => "adc " rgb[$r] "," hex($imm8) ;
  0x80 @addr(2)   @imm8 => "adc" sfx[1] " " $addr "," hex($imm8) ;
  0x80 11 011 rrr @imm8 => "sbb " rgb[$r] "," hex($imm8) ;
  0x80 @addr(3)   @imm8 => "sbb" sfx[1] " " $addr "," hex($imm8) ;
  0x80 11 100 rrr @imm8 => "and " rgb[$r] "," hex($imm8) ;
  0x80 @addr(4)   @imm8 => "and" sfx[1] " " $addr "," hex($imm8) ;
  0x80 11 101 rrr @imm8 => "sub " rgb[$r] "," hex($imm8) ;
  0x80 @addr(5)   @imm8 => "sub" sfx[1] " " $addr "," hex($imm8) ;
  0x80 11 110 rrr @imm8 => "xor " rgb[$r] "," hex($imm8) ;
  0x80 @addr(6)   @imm8 => "xor" sfx[1] " " $addr "," hex($imm8) ;
  0x80 11 111 rrr @imm8 => "cmp " rgb[$r] "," hex($imm8) ;
  0x80 @addr(7)   @imm8 => "cmp" sfx[1] " " $addr "," hex($imm8) ;
  0x81 11 000 rrr @immz => "add " greg[$r] "," hex($immz) ;
  0x81 @addr(0)   @immz => "add" sfx[4] " " $addr "," hex($immz) ;
  0x81 11 001 rrr @immz => "or " greg[$r] "," hex($immz) ;
  0x81 @addr(1)   @immz => "or" sfx[4] " " $addr "," hex($immz) ;
  0x81 11 010 rrr @immz => "adc " greg[$r] "," hex($immz) ;
  0x81 @addr(2)   @immz => "adc" sfx[4] " " $addr "," hex($immz) ;
  0x81 11 011 rrr @immz => "sbb " greg[$r] "," hex($immz) ;
  0x81 @addr(3)   @immz => "sbb" sfx[4] " " $addr "," hex($immz) ;
  0x81 11 100 rrr @immz => "and " greg[$r] "," hex($immz) ;
  0x81 @addr(4)   @immz => "and" sfx[4] " " $addr "," hex($immz) ;
  0x81 11 101 rrr @immz => "sub " greg[$r] "," hex($immz) ;
  0x81 @addr(5)   @immz => "sub" sfx[4] " " $addr "," hex($immz) ;
  0x81 11 110 rrr @immz => "xor " greg[$r] "," hex($immz) ;
  0x81 @addr(6)   @immz => "xor" sfx[4] " " $addr "," hex($immz) ;
  0x81 11 111 rrr @immz => "cmp " greg[$r] "," hex($immz) ;
  0x81 @addr(7)   @immz => "cmp" sfx[4] " " $addr "," hex($immz) ;
  0x83 11 000 rrr @imm8 => "add " greg[$r] "," hex(sx8($imm8)) ;
  0x83 @addr(0)   @imm8 => "add" sfx[4] " " $addr "," hex(sx8($imm8)) ;
  0x83 11 001 rrr @imm8 => "or " greg[$r] "," hex(sx8($imm8)) ;
  0x83 @addr(1)   @imm8 => "or" sfx[4] " " $addr "," hex(sx8($imm8)) ;
  0x83 11 010 rrr @imm8 => "adc " greg[$r] "," hex(sx8($imm8)) ;
  0x83 @addr(2)   @imm8 => "adc" sfx[4] " " $addr "," hex(sx8($imm8)) ;
  0x83 11 011 rrr @imm8 => "sbb " greg[$r] "," hex(sx8($imm8)) ;
  0x83 @addr(3)   @imm8 => "sbb" sfx[4] " " $addr "," hex(sx8($imm8)) ;
  0x83 11 100 rrr @imm8 => "and " greg[$r] "," hex(sx8($imm8)) ;
  0x83 @addr(4)   @imm8 => "and" sfx[4] " " $addr "," hex(sx8($imm8)) ;
  0x83 11 101 rrr @imm8 => "sub " greg[$r] "," hex(sx8($imm8)) ;
  0x83 @addr(5)   @imm8 => "sub" sfx[4] " " $addr "," hex(sx8($imm8)) ;
  0x83 11 110 rrr @imm8 => "xor " greg[$r] "," hex(sx8($imm8)) ;
  0x83 @addr(6)   @imm8 => "xor" sfx[4] " " $addr "," hex(sx8($imm8)) ;
  0x83 11 111 rrr @imm8 => "cmp " greg[$r] "," hex(sx8($imm8)) ;
  0x83 @addr(7)   @imm8 => "cmp" sfx[4] " " $addr "," hex(sx8($imm8)) ;

  # ===== test / xchg ============================================================
  0x84 11 ggg rrr => "test " rgb[$r] "," rgb[$g] ;
  0x84 @addr      => "test" sfx[1] " " $addr "," rgb[$g] ;
  0x85 11 ggg rrr => "test " greg[$r] "," greg[$g] ;
  0x85 @addr      => "test" sfx[4] " " $addr "," greg[$g] ;
  0x86 11 ggg rrr => "xchg " rgb[$g] "," rgb[$r] ;
  0x86 @addr      => "xchg " rgb[$g] "," $addr ;
  0x87 11 ggg rrr => "xchg " greg[$g] "," greg[$r] ;
  0x87 @addr      => "xchg " greg[$g] "," $addr ;

  # ===== mov ====================================================================
  0x88 11 ggg rrr => "mov " rgb[$r] "," rgb[$g] ;
  0x88 @addr      => "mov" sfx[1] " " $addr "," rgb[$g] ;
  0x89 11 ggg rrr => "mov " greg[$r] "," greg[$g] ;
  0x89 @addr      => "mov " $addr "," greg[$g] ;
  0x8a 11 ggg rrr => "mov " rgb[$g] "," rgb[$r] ;
  0x8a @addr      => "mov " rgb[$g] "," $addr ;
  0x8b 11 ggg rrr => "mov " greg[$g] "," greg[$r] ;
  0x8b @addr      => "mov " greg[$g] "," $addr ;
  0x8d @addr      => "lea " greg[$g] "," $addr ;

  # ===== xchg eAX,r / nop / pause ; cwde/cdqe ; cdq/cqo =========================
  0x90 => "nop" ;
  0x91 => "xchg eax," greg[1] ;
  0x92 => "xchg eax," greg[2] ;
  0x93 => "xchg eax," greg[3] ;
  0x94 => "xchg eax," greg[4] ;
  0x95 => "xchg eax," greg[5] ;
  0x96 => "xchg eax," greg[6] ;
  0x97 => "xchg eax," greg[7] ;
  0x98 => "cwde" ;
  0x99 => "cdq" ;

  # ===== string ops (no operands; rep/repnz + 66/REX.W ride in the prefix run,
  # so the byte stream round-trips for every operand size; the .w/.q sizes share
  # the rendered .d mnemonic since size is carried by the replayed prefix) =======
  0xa4 => "movsb" ;
  0xa5 => "movsd" ;
  0xa6 => "cmpsb" ;
  0xa7 => "cmpsd" ;
  0xaa => "stosb" ;
  0xab => "stosd" ;
  0xac => "lodsb" ;
  0xad => "lodsd" ;
  0xae => "scasb" ;
  0xaf => "scasd" ;

  # ===== test al/eax,imm ; mov r,imm ; movabs ===================================
  0xa8 @imm8 => "test al," hex($imm8) ;
  0xa9 @immz => "test eax," hex($immz) ;
  10110 bbb @imm8 => "mov " rgb[$b] "," hex($imm8) ;
  10111 bbb @immv => "mov " greg[$b] "," hex($immv) ;   # mov r,imm (imm64 under REX.W = movabs)

  # ===== group 2 shifts (C0/C1 imm8 ; D0-D3) ====================================
  0xc0 11 000 rrr @imm8 => "rol " rgb[$r] "," hex($imm8) ;
  0xc0 @addr(0)   @imm8 => "rol" sfx[1] " " $addr "," hex($imm8) ;
  0xc0 11 001 rrr @imm8 => "ror " rgb[$r] "," hex($imm8) ;
  0xc0 @addr(1)   @imm8 => "ror" sfx[1] " " $addr "," hex($imm8) ;
  0xc0 11 010 rrr @imm8 => "rcl " rgb[$r] "," hex($imm8) ;
  0xc0 @addr(2)   @imm8 => "rcl" sfx[1] " " $addr "," hex($imm8) ;
  0xc0 11 011 rrr @imm8 => "rcr " rgb[$r] "," hex($imm8) ;
  0xc0 @addr(3)   @imm8 => "rcr" sfx[1] " " $addr "," hex($imm8) ;
  0xc0 11 100 rrr @imm8 => "shl " rgb[$r] "," hex($imm8) ;
  0xc0 11 110 rrr @imm8 => "shl " rgb[$r] "," hex($imm8) ;   # /6 = undocumented shl alias
  0xc0 @addr(4)   @imm8 => "shl" sfx[1] " " $addr "," hex($imm8) ;
  0xc0 @addr(6)     @imm8 => "shl" sfx[1] " " $addr "," hex($imm8) ;   # /6 = undocumented shl alias
  0xc0 11 101 rrr @imm8 => "shr " rgb[$r] "," hex($imm8) ;
  0xc0 @addr(5)   @imm8 => "shr" sfx[1] " " $addr "," hex($imm8) ;
  0xc0 11 111 rrr @imm8 => "sar " rgb[$r] "," hex($imm8) ;
  0xc0 @addr(7)   @imm8 => "sar" sfx[1] " " $addr "," hex($imm8) ;
  0xc1 11 000 rrr @imm8 => "rol " greg[$r] "," hex($imm8) ;
  0xc1 @addr(0)   @imm8 => "rol" sfx[4] " " $addr "," hex($imm8) ;
  0xc1 11 001 rrr @imm8 => "ror " greg[$r] "," hex($imm8) ;
  0xc1 @addr(1)   @imm8 => "ror" sfx[4] " " $addr "," hex($imm8) ;
  0xc1 11 010 rrr @imm8 => "rcl " greg[$r] "," hex($imm8) ;
  0xc1 @addr(2)   @imm8 => "rcl" sfx[4] " " $addr "," hex($imm8) ;
  0xc1 11 011 rrr @imm8 => "rcr " greg[$r] "," hex($imm8) ;
  0xc1 @addr(3)   @imm8 => "rcr" sfx[4] " " $addr "," hex($imm8) ;
  0xc1 11 100 rrr @imm8 => "shl " greg[$r] "," hex($imm8) ;
  0xc1 11 110 rrr @imm8 => "shl " greg[$r] "," hex($imm8) ;   # /6 = undocumented shl alias
  0xc1 @addr(4)   @imm8 => "shl" sfx[4] " " $addr "," hex($imm8) ;
  0xc1 @addr(6)     @imm8 => "shl" sfx[4] " " $addr "," hex($imm8) ;   # /6 = undocumented shl alias
  0xc1 11 101 rrr @imm8 => "shr " greg[$r] "," hex($imm8) ;
  0xc1 @addr(5)   @imm8 => "shr" sfx[4] " " $addr "," hex($imm8) ;
  0xc1 11 111 rrr @imm8 => "sar " greg[$r] "," hex($imm8) ;
  0xc1 @addr(7)   @imm8 => "sar" sfx[4] " " $addr "," hex($imm8) ;
  0xd0 11 000 rrr => "rol " rgb[$r] ",1" ;
  0xd0 @addr(0)   => "rol" sfx[1] " " $addr ",1" ;
  0xd0 11 001 rrr => "ror " rgb[$r] ",1" ;
  0xd0 @addr(1)   => "ror" sfx[1] " " $addr ",1" ;
  0xd0 11 010 rrr => "rcl " rgb[$r] ",1" ;
  0xd0 @addr(2)   => "rcl" sfx[1] " " $addr ",1" ;
  0xd0 11 011 rrr => "rcr " rgb[$r] ",1" ;
  0xd0 @addr(3)   => "rcr" sfx[1] " " $addr ",1" ;
  0xd0 11 100 rrr => "shl " rgb[$r] ",1" ;
  0xd0 11 110 rrr => "shl " rgb[$r] ",1" ;   # /6 = undocumented shl alias
  0xd0 @addr(4)   => "shl" sfx[1] " " $addr ",1" ;
  0xd0 @addr(6)     => "shl" sfx[1] " " $addr ",1" ;   # /6 = undocumented shl alias
  0xd0 11 101 rrr => "shr " rgb[$r] ",1" ;
  0xd0 @addr(5)   => "shr" sfx[1] " " $addr ",1" ;
  0xd0 11 111 rrr => "sar " rgb[$r] ",1" ;
  0xd0 @addr(7)   => "sar" sfx[1] " " $addr ",1" ;
  0xd1 11 000 rrr => "rol " greg[$r] ",1" ;
  0xd1 @addr(0)   => "rol" sfx[4] " " $addr ",1" ;
  0xd1 11 001 rrr => "ror " greg[$r] ",1" ;
  0xd1 @addr(1)   => "ror" sfx[4] " " $addr ",1" ;
  0xd1 11 010 rrr => "rcl " greg[$r] ",1" ;
  0xd1 @addr(2)   => "rcl" sfx[4] " " $addr ",1" ;
  0xd1 11 011 rrr => "rcr " greg[$r] ",1" ;
  0xd1 @addr(3)   => "rcr" sfx[4] " " $addr ",1" ;
  0xd1 11 100 rrr => "shl " greg[$r] ",1" ;
  0xd1 11 110 rrr => "shl " greg[$r] ",1" ;   # /6 = undocumented shl alias
  0xd1 @addr(4)   => "shl" sfx[4] " " $addr ",1" ;
  0xd1 @addr(6)     => "shl" sfx[4] " " $addr ",1" ;   # /6 = undocumented shl alias
  0xd1 11 101 rrr => "shr " greg[$r] ",1" ;
  0xd1 @addr(5)   => "shr" sfx[4] " " $addr ",1" ;
  0xd1 11 111 rrr => "sar " greg[$r] ",1" ;
  0xd1 @addr(7)   => "sar" sfx[4] " " $addr ",1" ;
  0xd2 11 000 rrr => "rol " rgb[$r] ",cl" ;
  0xd2 @addr(0)   => "rol" sfx[1] " " $addr ",cl" ;
  0xd2 11 001 rrr => "ror " rgb[$r] ",cl" ;
  0xd2 @addr(1)   => "ror" sfx[1] " " $addr ",cl" ;
  0xd2 11 010 rrr => "rcl " rgb[$r] ",cl" ;
  0xd2 @addr(2)   => "rcl" sfx[1] " " $addr ",cl" ;
  0xd2 11 011 rrr => "rcr " rgb[$r] ",cl" ;
  0xd2 @addr(3)   => "rcr" sfx[1] " " $addr ",cl" ;
  0xd2 11 100 rrr => "shl " rgb[$r] ",cl" ;
  0xd2 11 110 rrr => "shl " rgb[$r] ",cl" ;   # /6 = undocumented shl alias
  0xd2 @addr(4)   => "shl" sfx[1] " " $addr ",cl" ;
  0xd2 @addr(6)     => "shl" sfx[1] " " $addr ",cl" ;   # /6 = undocumented shl alias
  0xd2 11 101 rrr => "shr " rgb[$r] ",cl" ;
  0xd2 @addr(5)   => "shr" sfx[1] " " $addr ",cl" ;
  0xd2 11 111 rrr => "sar " rgb[$r] ",cl" ;
  0xd2 @addr(7)   => "sar" sfx[1] " " $addr ",cl" ;
  0xd3 11 000 rrr => "rol " greg[$r] ",cl" ;
  0xd3 @addr(0)   => "rol" sfx[4] " " $addr ",cl" ;
  0xd3 11 001 rrr => "ror " greg[$r] ",cl" ;
  0xd3 @addr(1)   => "ror" sfx[4] " " $addr ",cl" ;
  0xd3 11 010 rrr => "rcl " greg[$r] ",cl" ;
  0xd3 @addr(2)   => "rcl" sfx[4] " " $addr ",cl" ;
  0xd3 11 011 rrr => "rcr " greg[$r] ",cl" ;
  0xd3 @addr(3)   => "rcr" sfx[4] " " $addr ",cl" ;
  0xd3 11 100 rrr => "shl " greg[$r] ",cl" ;
  0xd3 11 110 rrr => "shl " greg[$r] ",cl" ;   # /6 = undocumented shl alias
  0xd3 @addr(4)   => "shl" sfx[4] " " $addr ",cl" ;
  0xd3 @addr(6)     => "shl" sfx[4] " " $addr ",cl" ;   # /6 = undocumented shl alias
  0xd3 11 101 rrr => "shr " greg[$r] ",cl" ;
  0xd3 @addr(5)   => "shr" sfx[4] " " $addr ",cl" ;
  0xd3 11 111 rrr => "sar " greg[$r] ",cl" ;
  0xd3 @addr(7)   => "sar" sfx[4] " " $addr ",cl" ;

  # ===== ret / mov r/m,imm / leave / int =======================================
  0xc2 @imm16 => "ret " hex($imm16) ;
  0xc3 => "ret" ;
  0xc6 11 000 rrr @imm8 => "mov " rgb[$r] "," hex($imm8) ;
  0xc6 @addr(0)   @imm8 => "mov" sfx[1] " " $addr "," hex($imm8) ;
  0xc7 11 000 rrr @immz => "mov " greg[$r] "," hex($immz) ;
  0xc7 @addr(0)   @immz => "mov" sfx[4] " " $addr "," hex($immz) ;
  # TSX/RTM: C6 /7 with ModR/M F8 is xabort imm8; C7 /7 with ModR/M F8 is xbegin rel16/32
  # (the 66 form is rel16, matching the near-branch convention). Both are fully-fixed
  # ModR/M (11 111 000), distinct from the C6/C7 /0 mov group above.
  0xc6 11 111 000 @imm8 => "xabort " hex($imm8) ;
  0xc7 11 111 000 @relz => "xbegin " hex($relz) ;
  0xc9 => "leave" ;
  0xcc => "int3" ;
  0xcd @imm8 => "int " hex($imm8) ;

  # ===== legacy 1-byte completeness: flags, system, far ret, enter =============
  0x9b => "fwait" ;
  0x9c => "pushf" ;
  0x9d => "popf" ;
  0x9e => "sahf" ;
  0x9f => "lahf" ;
  0xd7 => "xlat" ;
  0xf1 => "int1" ;
  0xf4 => "hlt" ;
  0xf5 => "cmc" ;
  0xf8 => "clc" ;
  0xf9 => "stc" ;
  0xfa => "cli" ;
  0xfb => "sti" ;
  0xfc => "cld" ;
  0xfd => "std" ;
  0xcf => "iret" ;
  0xcb => "retf" ;
  0xca @imm16 => "retf " hex($imm16) ;
  0xc8 @imm16 @imm8 => "enter " hex($imm16) "," hex($imm8) ;

  # ===== loop / jrcxz (rel8) ===================================================
  0xe0 @rel8 => "loopne " hex($rel8) ;
  0xe1 @rel8 => "loope " hex($rel8) ;
  0xe2 @rel8 => "loop " hex($rel8) ;
  0xe3 @rel8 => "jrcxz " hex($rel8) ;

  # ===== port I/O (in/out, imm8 or dx) + string I/O (ins/outs) =================
  0xe4 @imm8 => "in al," hex($imm8) ;
  0xe5 @imm8 => "in " greg[0] "," hex($imm8) ;
  0xe6 @imm8 => "out " hex($imm8) ",al" ;
  0xe7 @imm8 => "out " hex($imm8) "," greg[0] ;
  0xec => "in al,dx" ;
  0xed => "in " greg[0] ",dx" ;
  0xee => "out dx,al" ;
  0xef => "out dx," greg[0] ;
  0x6c => "insb" ;
  0x6d => "ins" ;
  0x6e => "outsb" ;
  0x6f => "outs" ;

  # ===== mov to/from segment register (8C / 8E) ================================
  0x8c 11 ggg rrr => "mov " greg[$r] "," sreg[$g] ;
  0x8c @addr      => "mov " $addr "," sreg[$g] ;
  0x8e 11 ggg rrr => "mov " sreg[$g] "," greg[$r] ;
  0x8e @addr      => "mov " sreg[$g] "," $addr ;

  # ===== call/jmp rel ; group 5 indirect ; group 3 F6/F7 =======================
  0xe8 @relz => "call " hex($relz) ;
  0xe9 @relz => "jmp " hex($relz) ;
  0xeb @rel8 => "jmp " hex($rel8) ;
  0xf6 11 000 rrr @imm8 => "test " rgb[$r] "," hex($imm8) ;
  0xf6 11 001 rrr @imm8 => "test " rgb[$r] "," hex($imm8) ;   # /1 = undocumented test alias
  0xf6 @addr(0)   @imm8 => "test" sfx[1] " " $addr "," hex($imm8) ;
  0xf6 @addr(1)     @imm8 => "test" sfx[1] " " $addr "," hex($imm8) ;   # /1 = undocumented test alias
  0xf6 11 010 rrr => "not " rgb[$r] ;
  0xf6 @addr(2)   => "not" sfx[1] " " $addr ;
  0xf6 11 011 rrr => "neg " rgb[$r] ;
  0xf6 @addr(3)   => "neg" sfx[1] " " $addr ;
  0xf6 11 100 rrr => "mul " rgb[$r] ;
  0xf6 @addr(4)   => "mul" sfx[1] " " $addr ;
  0xf6 11 101 rrr => "imul " rgb[$r] ;
  0xf6 @addr(5)   => "imul" sfx[1] " " $addr ;
  0xf6 11 110 rrr => "div " rgb[$r] ;
  0xf6 @addr(6)   => "div" sfx[1] " " $addr ;
  0xf6 11 111 rrr => "idiv " rgb[$r] ;
  0xf6 @addr(7)   => "idiv" sfx[1] " " $addr ;
  0xf7 11 000 rrr @immz => "test " greg[$r] "," hex($immz) ;
  0xf7 11 001 rrr @immz => "test " greg[$r] "," hex($immz) ;   # /1 = undocumented test alias
  0xf7 @addr(0)   @immz => "test" sfx[4] " " $addr "," hex($immz) ;
  0xf7 @addr(1)     @immz => "test" sfx[4] " " $addr "," hex($immz) ;   # /1 = undocumented test alias
  0xf7 11 010 rrr => "not " greg[$r] ;
  0xf7 @addr(2)   => "not" sfx[4] " " $addr ;
  0xf7 11 011 rrr => "neg " greg[$r] ;
  0xf7 @addr(3)   => "neg" sfx[4] " " $addr ;
  0xf7 11 100 rrr => "mul " greg[$r] ;
  0xf7 @addr(4)   => "mul" sfx[4] " " $addr ;
  0xf7 11 101 rrr => "imul " greg[$r] ;
  0xf7 @addr(5)   => "imul" sfx[4] " " $addr ;
  0xf7 11 110 rrr => "div " greg[$r] ;
  0xf7 @addr(6)   => "div" sfx[4] " " $addr ;
  0xf7 11 111 rrr => "idiv " greg[$r] ;
  0xf7 @addr(7)   => "idiv" sfx[4] " " $addr ;

  # ===== group 4/5 (FE inc/dec r/m8 ; FF inc/dec/call/jmp/push) =================
  0xfe 11 000 rrr => "inc " rgb[$r] ;
  0xfe @addr(0)   => "inc" sfx[1] " " $addr ;
  0xfe 11 001 rrr => "dec " rgb[$r] ;
  0xfe @addr(1)   => "dec" sfx[1] " " $addr ;
  0xff 11 000 rrr => "inc " greg[$r] ;
  0xff @addr(0)   => "inc" sfx[4] " " $addr ;
  0xff 11 001 rrr => "dec " greg[$r] ;
  0xff @addr(1)   => "dec" sfx[4] " " $addr ;
  0xff 11 010 rrr => "call " greg[$r] ;
  0xff @addr(2)   => "call " $addr ;
  0xff @addr(3)   => "callf " $addr ;               # /3 call far m16:16/32/64 (memory only)
  0xff 11 100 rrr => "jmp " greg[$r] ;
  0xff @addr(4)   => "jmp " $addr ;
  0xff @addr(5)   => "jmpf " $addr ;                # /5 jmp far m16:16/32/64 (memory only)
  0xff 11 110 rrr => "push " greg[$r] ;
  0xff @addr(6)   => "push " $addr ;

  # ===== push/pop r64 (default-64; embedded register 50+r / 58+r) ==============
  01010 bbb => "push " greg[$b] ;
  01011 bbb => "pop " greg[$b] ;
  # 8F /0: pop r/m64 (the non-XOP form; map<8 falls through to the FSM, the
  # enc-stamp picks 8F vs the 58+r encoding so both round-trip).
  0x8f 11 000 rrr => "pop " greg[$r] ;
  0x8f @addr(0)   => "pop" sfx[4] " " $addr ;

  # ===== 0F two-byte map ========================================================
  0x0f 0x05 => "syscall" ;
  0x0f 0x0b => "ud2" ;
  0x0f 0x1f 11 000 rrr => "nop " greg[$r] ;
  0x0f 0x1f 11 001 rrr => "nop " greg[$r] ;
  0x0f 0x1f 11 010 rrr => "nop " greg[$r] ;
  0x0f 0x1f 11 011 rrr => "nop " greg[$r] ;
  0x0f 0x1f 11 100 rrr => "nop " greg[$r] ;
  0x0f 0x1f 11 101 rrr => "nop " greg[$r] ;
  0x0f 0x1f 11 110 rrr => "nop " greg[$r] ;
  0x0f 0x1f 11 111 rrr => "nop " greg[$r] ;
  0x0f 0x1f @addr(0)   => "nop" sfx[4] " " $addr ;
  0x0f 0x1f @addr(1)   => "nop" sfx[4] " " $addr ;
  0x0f 0x1f @addr(2)   => "nop" sfx[4] " " $addr ;
  0x0f 0x1f @addr(3)   => "nop" sfx[4] " " $addr ;
  0x0f 0x1f @addr(4)   => "nop" sfx[4] " " $addr ;
  0x0f 0x1f @addr(5)   => "nop" sfx[4] " " $addr ;
  0x0f 0x1f @addr(6)   => "nop" sfx[4] " " $addr ;
  0x0f 0x1f @addr(7)   => "nop" sfx[4] " " $addr ;
  0x0f 1000 cccc @relz => "j" cond[$c] " " hex($relz) ;
  0x0f 1001 cccc 11 ggg rrr => "set" cond[$c] " " rgb[$r] ;
  0x0f 1001 cccc @addr      => "set" cond[$c] sfx[1] " " $addr ;
  0x0f 0100 cccc 11 ggg rrr => "cmov" cond[$c] " " greg[$g] "," greg[$r] ;
  0x0f 0100 cccc @addr      => "cmov" cond[$c] " " greg[$g] "," $addr ;
  0x0f 0xaf 11 ggg rrr => "imul " greg[$g] "," greg[$r] ;
  0x0f 0xaf @addr      => "imul " greg[$g] "," $addr ;
  0x0f 0xb6 11 ggg rrr => "movzx " greg[$g] "," rgb[$r] ;
  0x0f 0xb6 @addr      => "movzx " greg[$g] "," $addr ;
  0x0f 0xb7 11 ggg rrr => "movzx " greg[$g] "," gregw[$r] ;   # r/m16 source: always 16-bit
  0x0f 0xb7 @addr      => "movzx " greg[$g] "," $addr ;
  0x0f 0xbe 11 ggg rrr => "movsx " greg[$g] "," rgb[$r] ;
  0x0f 0xbe @addr      => "movsx " greg[$g] "," $addr ;
  0x0f 0xbf 11 ggg rrr => "movsx " greg[$g] "," gregw[$r] ;   # r/m16 source: always 16-bit
  0x0f 0xbf @addr      => "movsx " greg[$g] "," $addr ;

  # ===== 0F bit ops / scans / bswap / atomics / cpuid+rdtsc =====================
  # F3-prefixed twins (popcnt = F3 0F B8, tzcnt = F3 0F BC, lzcnt = F3 0F BD) and
  # cmpxchg16b (REX.W 0F C7 /1) round-trip via the replayed prefix; only the base
  # opcode needs a rule. Bit-test reg/reg puts the r/m first (r/m,reg direction).
  0x0f 0xa2 => "cpuid" ;
  0x0f 0x31 => "rdtsc" ;
  0x0f 0xa3 11 ggg rrr => "bt " greg[$r] "," greg[$g] ;
  0x0f 0xa3 @addr      => "bt " $addr "," greg[$g] ;
  0x0f 0xab 11 ggg rrr => "bts " greg[$r] "," greg[$g] ;
  0x0f 0xab @addr      => "bts " $addr "," greg[$g] ;
  0x0f 0xb3 11 ggg rrr => "btr " greg[$r] "," greg[$g] ;
  0x0f 0xb3 @addr      => "btr " $addr "," greg[$g] ;
  0x0f 0xbb 11 ggg rrr => "btc " greg[$r] "," greg[$g] ;
  0x0f 0xbb @addr      => "btc " $addr "," greg[$g] ;
  0x0f 0xba 11 100 rrr @imm8 => "bt " greg[$r] "," hex($imm8) ;
  0x0f 0xba @addr(4)   @imm8 => "bt" sfx[4] " " $addr "," hex($imm8) ;
  0x0f 0xba 11 101 rrr @imm8 => "bts " greg[$r] "," hex($imm8) ;
  0x0f 0xba @addr(5)   @imm8 => "bts" sfx[4] " " $addr "," hex($imm8) ;
  0x0f 0xba 11 110 rrr @imm8 => "btr " greg[$r] "," hex($imm8) ;
  0x0f 0xba @addr(6)   @imm8 => "btr" sfx[4] " " $addr "," hex($imm8) ;
  0x0f 0xba 11 111 rrr @imm8 => "btc " greg[$r] "," hex($imm8) ;
  0x0f 0xba @addr(7)   @imm8 => "btc" sfx[4] " " $addr "," hex($imm8) ;
  0x0f 0xbc 11 ggg rrr => ssebc[$pp] greg[$g] "," greg[$r] ;
  0x0f 0xbc @addr      => ssebc[$pp] greg[$g] "," $addr ;
  0x0f 0xbd 11 ggg rrr => ssebd[$pp] greg[$g] "," greg[$r] ;
  0x0f 0xbd @addr      => ssebd[$pp] greg[$g] "," $addr ;
  0x0f 0xb8 11 ggg rrr => "popcnt " greg[$g] "," greg[$r] ;
  0x0f 0xb8 @addr      => "popcnt " greg[$g] "," $addr ;
  0x0f 0xb0 11 ggg rrr => "cmpxchg " rgb[$r] "," rgb[$g] ;
  0x0f 0xb0 @addr      => "cmpxchg" sfx[1] " " $addr "," rgb[$g] ;
  0x0f 0xb1 11 ggg rrr => "cmpxchg " greg[$r] "," greg[$g] ;
  0x0f 0xb1 @addr      => "cmpxchg " $addr "," greg[$g] ;
  0x0f 0xc0 11 ggg rrr => "xadd " rgb[$r] "," rgb[$g] ;
  0x0f 0xc0 @addr      => "xadd" sfx[1] " " $addr "," rgb[$g] ;
  0x0f 0xc1 11 ggg rrr => "xadd " greg[$r] "," greg[$g] ;
  0x0f 0xc1 @addr      => "xadd " $addr "," greg[$g] ;
  0x0f 0xc7 @addr(1)   => "cmpxchg8b " $addr ;
  # 0F C7 group tail (/3-/7): XSAVE-supervisor + VMX pointers + rdrand/rdseed/rdpid/
  # senduipi. pp-variant: /6,/7 select by mandatory prefix (NP/66/F3). REX.W -> the
  # *64 forms / r64 operand (rides the prefix witness; base mnemonic shown).
  0x0f 0xc7 @addr(3)   => "xrstors " $addr ;            # /3 mem: xrstors/xrstors64
  0x0f 0xc7 @addr(4)   => "xsavec " $addr ;             # /4 mem: xsavec/xsavec64
  0x0f 0xc7 @addr(5)   => "xsaves " $addr ;             # /5 mem: xsaves/xsaves64
  0x0f 0xc7 @addr(6) [$pp==0] => "vmptrld " $addr ;     # /6 mem: NP vmptrld
  0x0f 0xc7 @addr(6) [$pp==1] => "vmclear " $addr ;     #         66 vmclear
  0x0f 0xc7 @addr(6) [$pp==2] => "vmxon " $addr ;       #         F3 vmxon
  0x0f 0xc7 @addr(7) [$pp==0] => "vmptrst " $addr ;     # /7 mem: NP vmptrst
  0x0f 0xc7 11 110 rrr [$pp==0] => "rdrand " greg[$r] ;    # /6 reg: NP rdrand r32/r64
  0x0f 0xc7 11 110 rrr [$pp==1] => "rdrand " greg[$r] ;    #          66 rdrand r16
  0x0f 0xc7 11 110 rrr [$pp==2] => "senduipi " gregq[$r] ; #          F3 senduipi r64
  0x0f 0xc7 11 111 rrr [$pp==0] => "rdseed " greg[$r] ;    # /7 reg: NP rdseed r32/r64
  0x0f 0xc7 11 111 rrr [$pp==1] => "rdseed " greg[$r] ;    #          66 rdseed r16
  0x0f 0xc7 11 111 rrr [$pp==2] => "rdpid " gregq[$r] ;    #          F3 rdpid r64
  0x0f 11001 bbb       => "bswap " greg[$b] ;

  # ===== 0F AE: fences / fxsave-xsave / clflush + CET/base-reg/wait (pp-variant grp)
  # The reg (mod=11) forms are almost entirely mandatory-prefix-specific; the mem
  # (mod!=11) fxsave/fxrstor/ldmxcsr/stmxcsr ignore F3/F2/66. This is a pp-variant
  # group: unguarded rules seed all 4 prefix slots, [$pp==N] rules add/override a
  # slot. (pp: 0=NP, 1=66, 2=F3, 3=F2.) ========================================
  0x0f 0xae @addr(0)   => "fxsave " $addr ;             # /0-/3 mem: prefix-agnostic
  0x0f 0xae @addr(1)   => "fxrstor " $addr ;
  0x0f 0xae @addr(2)   => "ldmxcsr " $addr ;
  0x0f 0xae @addr(3)   => "stmxcsr " $addr ;
  0x0f 0xae 11 111 000 => "sfence" ;                    # /7 reg: sfence (fixmodrm F8; no operand)
  0x0f 0xae 11 101 000 [$pp==0] => "lfence" ;           # /5,/6 reg NP-only (fixmodrm E8/F0)
  0x0f 0xae 11 110 000 [$pp==0] => "mfence" ;
  0x0f 0xae @addr(5) [$pp==0] => "xrstor " $addr ;      # NP-only mem: xrstor/xsaveopt
  0x0f 0xae @addr(6) [$pp==0] => "xsaveopt " $addr ;
  0x0f 0xae @addr(7) [$pp==0] => "clflush" sfx[1] " " $addr ;
  0x0f 0xae @addr(6) [$pp==1] => "clwb" sfx[1] " " $addr ;         # 66: clwb / clflushopt mem
  0x0f 0xae @addr(7) [$pp==1] => "clflushopt" sfx[1] " " $addr ;
  0x0f 0xae 11 110 rrr [$pp==1] => "tpause " gregd[$r] ;          # 66: tpause reg
  0x0f 0xae 11 000 rrr [$pp==2] => "rdfsbase " gregd[$r] ;        # F3: rd/wr fs/gs base reg
  0x0f 0xae 11 001 rrr [$pp==2] => "rdgsbase " gregd[$r] ;
  0x0f 0xae 11 010 rrr [$pp==2] => "wrfsbase " gregd[$r] ;
  0x0f 0xae 11 011 rrr [$pp==2] => "wrgsbase " gregd[$r] ;
  0x0f 0xae @addr(4) [$pp==2] => "ptwrite " $addr ;               # F3: ptwrite mem/reg
  0x0f 0xae 11 100 rrr [$pp==2] => "ptwrite " gregd[$r] ;
  0x0f 0xae 11 101 rrr [$pp==2] => incssp[$opsiz] gregd[$r] ;      # F3: incsspd/incsspq
  0x0f 0xae @addr(6) [$pp==2] => "clrssbsy" sfx[8] " " $addr ;     # F3: clrssbsy qword mem
  0x0f 0xae 11 110 rrr [$pp==2] => "umonitor " gregq[$r] ;        # F3: umonitor (addr-size reg)
  0x0f 0xae 11 110 rrr [$pp==3] => "umwait " gregd[$r] ;          # F2: umwait reg
  0x0f 0xae @addr(4) [$pp==0] => "xsave " $addr ;
  0x0f 0x18 @addr(0)   => "prefetchnta" sfx[1] " " $addr ;
  0x0f 0x18 @addr(1)   => "prefetcht0" sfx[1] " " $addr ;
  0x0f 0x18 @addr(2)   => "prefetcht1" sfx[1] " " $addr ;
  0x0f 0x18 @addr(3)   => "prefetcht2" sfx[1] " " $addr ;
  0x0f 0x18 @addr(4)   => "nop~18 " $addr ;
  0x0f 0x18 @addr(5)   => "nop~18 " $addr ;
  0x0f 0x18 @addr(6)   => "nop~18 " $addr ;
  0x0f 0x18 @addr(7)   => "nop~18 " $addr ;
  0x0f 0x18 11 000 rrr => "nop~18 " greg[$r] ;
  0x0f 0x18 11 001 rrr => "nop~18 " greg[$r] ;
  0x0f 0x18 11 010 rrr => "nop~18 " greg[$r] ;
  0x0f 0x18 11 011 rrr => "nop~18 " greg[$r] ;
  0x0f 0x18 11 100 rrr => "nop~18 " greg[$r] ;
  0x0f 0x18 11 101 rrr => "nop~18 " greg[$r] ;
  0x0f 0x18 11 110 rrr => "nop~18 " greg[$r] ;
  0x0f 0x18 11 111 rrr => "nop~18 " greg[$r] ;
  # 0F 0D: prefetch group -- /1 prefetchw, /2 prefetchwt1, all other digits prefetch
  # (memory forms). The reg-direct forms are the Intel reserved multi-byte NOP (Zydis
  # decodes them; objdump calls them (bad)) -- rendered "nop" via the ~0d disambiguator
  # so the encoder keeps them distinct from the 0F 1F / 0F 18-1E nop-space opcodes.
  0x0f 0x0d @addr(0)   => "prefetch " $addr ;
  0x0f 0x0d @addr(1)   => "prefetchw " $addr ;
  0x0f 0x0d @addr(2)   => "prefetchwt1 " $addr ;
  0x0f 0x0d @addr(3)   => "prefetch " $addr ;
  0x0f 0x0d @addr(4)   => "prefetch " $addr ;
  0x0f 0x0d @addr(5)   => "prefetch " $addr ;
  0x0f 0x0d @addr(6)   => "prefetch " $addr ;
  0x0f 0x0d @addr(7)   => "prefetch " $addr ;
  0x0f 0x0d 11 000 rrr => "nop~0d " greg[$r] ;
  0x0f 0x0d 11 001 rrr => "nop~0d " greg[$r] ;
  0x0f 0x0d 11 010 rrr => "nop~0d " greg[$r] ;
  0x0f 0x0d 11 011 rrr => "nop~0d " greg[$r] ;
  0x0f 0x0d 11 100 rrr => "nop~0d " greg[$r] ;
  0x0f 0x0d 11 101 rrr => "nop~0d " greg[$r] ;
  0x0f 0x0d 11 110 rrr => "nop~0d " greg[$r] ;
  0x0f 0x0d 11 111 rrr => "nop~0d " greg[$r] ;
  # 0F 1C cldemote (/0 mem) + reserved-nop; 0F 19/1D reserved multi-byte nop
  0x0f 0x1c @addr(0) [$pp==0] => "cldemote " $addr ;   # cldemote is NP-only; 66/F3/F2 -> nop
  0x0f 0x1c @addr(0)   => "nop~1c " $addr ;
  0x0f 0x1c @addr(1)   => "nop~1c " $addr ;
  0x0f 0x1c @addr(2)   => "nop~1c " $addr ;
  0x0f 0x1c @addr(3)   => "nop~1c " $addr ;
  0x0f 0x1c @addr(4)   => "nop~1c " $addr ;
  0x0f 0x1c @addr(5)   => "nop~1c " $addr ;
  0x0f 0x1c @addr(6)   => "nop~1c " $addr ;
  0x0f 0x1c @addr(7)   => "nop~1c " $addr ;
  0x0f 0x1c 11 000 rrr => "nop~1c " greg[$r] ;
  0x0f 0x1c 11 001 rrr => "nop~1c " greg[$r] ;
  0x0f 0x1c 11 010 rrr => "nop~1c " greg[$r] ;
  0x0f 0x1c 11 011 rrr => "nop~1c " greg[$r] ;
  0x0f 0x1c 11 100 rrr => "nop~1c " greg[$r] ;
  0x0f 0x1c 11 101 rrr => "nop~1c " greg[$r] ;
  0x0f 0x1c 11 110 rrr => "nop~1c " greg[$r] ;
  0x0f 0x1c 11 111 rrr => "nop~1c " greg[$r] ;
  0x0f 0x19 11 ggg rrr => "nop~19 " greg[$r] ;         # reserved nop r/m (reg field dead)
  0x0f 0x19 @addr      => "nop~19 " $addr ;
  0x0f 0x1d 11 ggg rrr => "nop~1d " greg[$r] ;
  0x0f 0x1d @addr      => "nop~1d " $addr ;
  # 0F 1A/1B: MPX bnd* (prefix-selected). Punted as reserved-nop placeholders
  # like the 32-bit corpus; the mandatory 66/F2/F3 rides the prefix run so all
  # bndldx/bndstx/bndmov/bndmk/bndc* encodings round-trip byte-exact.
  # 0F 1A MPX: NP=bndldx (reg-form is a reserved nop); 66=bndmov bnd,bnd|[mem];
  # F3=bndcl bnd,r/m64; F2=bndcu bnd,r/m64. The mandatory prefix picks the op (ppdesc).
  0x0f 0x1a 11 ggg rrr [$pp==0] => "nop~1a " greg[$r] ;
  0x0f 0x1a @addr      [$pp==0] => "bndldx " bndreg[$g] "," $addr ;
  0x0f 0x1a 11 ggg rrr [$pp==1] => "bndmov " bndreg[$g] "," bndreg[$r] ;
  0x0f 0x1a @addr      [$pp==1] => "bndmov " bndreg[$g] "," $addr ;
  0x0f 0x1a 11 ggg rrr [$pp==2] => "bndcl " bndreg[$g] "," gregq[$r] ;
  0x0f 0x1a @addr      [$pp==2] => "bndcl " bndreg[$g] "," $addr ;
  0x0f 0x1a 11 ggg rrr [$pp==3] => "bndcu " bndreg[$g] "," gregq[$r] ;
  0x0f 0x1a @addr      [$pp==3] => "bndcu " bndreg[$g] "," $addr ;
  # 0F 1B MPX: NP=bndstx [mem],bnd (reg=nop); 66=bndmov [mem]|bnd,bnd; F3=bndmk bnd,[mem]
  # (reg=nop); F2=bndcn bnd,r/m64.
  0x0f 0x1b 11 ggg rrr [$pp==0] => "nop~1b " greg[$r] ;
  0x0f 0x1b @addr      [$pp==0] => "bndstx " $addr "," bndreg[$g] ;
  0x0f 0x1b 11 ggg rrr [$pp==1] => "bndmov " bndreg[$r] "," bndreg[$g] ;
  0x0f 0x1b @addr      [$pp==1] => "bndmov " $addr "," bndreg[$g] ;
  0x0f 0x1b 11 ggg rrr [$pp==2] => "nop~1b " greg[$r] ;
  0x0f 0x1b @addr      [$pp==2] => "bndmk " bndreg[$g] "," $addr ;
  0x0f 0x1b 11 ggg rrr [$pp==3] => "bndcn " bndreg[$g] "," gregq[$r] ;
  0x0f 0x1b @addr      [$pp==3] => "bndcn " bndreg[$g] "," $addr ;
  # 0F 1E: reserved multi-byte NOP r/m (all mod), EXCEPT under F3, where reg=1 is
  # rdsspd/rdsspq and ModR/M FA/FB are endbr64/endbr32 (pp-variant group). Without
  # F3 those bytes are plain nop (nop edx / nop ebx / nop eax). pp: 0=NP ... 2=F3.
  0x0f 0x1e 11 111 010 [$pp==2] => "endbr64" ;         # F3 0F 1E FA (fixmodrm)
  0x0f 0x1e 11 111 011 [$pp==2] => "endbr32" ;         # F3 0F 1E FB
  0x0f 0x1e 11 001 rrr [$pp==2] => rdssp[$opsiz] gregd[$r] ;   # F3 0F 1E /1: rdsspd/rdsspq
  0x0f 0x1e 11 000 rrr => "nop~1e " greg[$r] ;
  0x0f 0x1e 11 001 rrr => "nop~1e " greg[$r] ;
  0x0f 0x1e 11 010 rrr => "nop~1e " greg[$r] ;
  0x0f 0x1e 11 011 rrr => "nop~1e " greg[$r] ;
  0x0f 0x1e 11 100 rrr => "nop~1e " greg[$r] ;
  0x0f 0x1e 11 101 rrr => "nop~1e " greg[$r] ;
  0x0f 0x1e 11 110 rrr => "nop~1e " greg[$r] ;
  0x0f 0x1e 11 111 rrr => "nop~1e " greg[$r] ;
  0x0f 0x1e @addr(0)   => "nop~1e " $addr ;
  0x0f 0x1e @addr(1)   => "nop~1e " $addr ;
  0x0f 0x1e @addr(2)   => "nop~1e " $addr ;
  0x0f 0x1e @addr(3)   => "nop~1e " $addr ;
  0x0f 0x1e @addr(4)   => "nop~1e " $addr ;
  0x0f 0x1e @addr(5)   => "nop~1e " $addr ;
  0x0f 0x1e @addr(6)   => "nop~1e " $addr ;
  0x0f 0x1e @addr(7)   => "nop~1e " $addr ;

  # ===== legacy system / misc 0F: double-shift, far-ptr load, vmx, ud, movnti ==
  0x0f 0x0e => "femms" ;
  # 3DNow! (AMD): the mnemonic is a trailing opcode byte after ModR/M; operands mm, mm/m64.
  0x0f 0x0f 11 ggg rrr @suf(0x0c) => "pi2fw " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0x0c) => "pi2fw " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0x0d) => "pi2fd " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0x0d) => "pi2fd " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0x1c) => "pf2iw " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0x1c) => "pf2iw " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0x1d) => "pf2id " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0x1d) => "pf2id " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0x8a) => "pfnacc " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0x8a) => "pfnacc " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0x8e) => "pfpnacc " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0x8e) => "pfpnacc " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0x90) => "pfcmpge " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0x90) => "pfcmpge " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0x94) => "pfmin " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0x94) => "pfmin " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0x96) => "pfrcp " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0x96) => "pfrcp " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0x97) => "pfrsqrt " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0x97) => "pfrsqrt " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0x9a) => "pfsub " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0x9a) => "pfsub " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0x9e) => "pfadd " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0x9e) => "pfadd " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0xa0) => "pfcmpgt " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0xa0) => "pfcmpgt " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0xa4) => "pfmax " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0xa4) => "pfmax " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0xa6) => "pfrcpit1 " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0xa6) => "pfrcpit1 " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0xa7) => "pfrsqit1 " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0xa7) => "pfrsqit1 " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0xaa) => "pfsubr " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0xaa) => "pfsubr " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0xae) => "pfacc " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0xae) => "pfacc " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0xb0) => "pfcmpeq " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0xb0) => "pfcmpeq " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0xb4) => "pfmul " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0xb4) => "pfmul " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0xb6) => "pfrcpit2 " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0xb6) => "pfrcpit2 " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0xb7) => "pmulhrw " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0xb7) => "pmulhrw " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0xbb) => "pswapd " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0xbb) => "pswapd " ssereg[$g] "," $addr ;
  0x0f 0x0f 11 ggg rrr @suf(0xbf) => "pavgusb " ssereg[$g] "," ssereg[$r] ;
  0x0f 0x0f @addr      @suf(0xbf) => "pavgusb " ssereg[$g] "," $addr ;
  0x0f 0x37 => "getsec" ;
  0x0f 0xaa => "rsm" ;
  0x0f 0xa4 11 ggg rrr @imm8 => "shld " greg[$r] "," greg[$g] "," hex($imm8) ;
  0x0f 0xa4 @addr      @imm8 => "shld " $addr "," greg[$g] "," hex($imm8) ;
  0x0f 0xa5 11 ggg rrr => "shld " greg[$r] "," greg[$g] ",cl" ;
  0x0f 0xa5 @addr      => "shld " $addr "," greg[$g] ",cl" ;
  0x0f 0xac 11 ggg rrr @imm8 => "shrd " greg[$r] "," greg[$g] "," hex($imm8) ;
  0x0f 0xac @addr      @imm8 => "shrd " $addr "," greg[$g] "," hex($imm8) ;
  0x0f 0xad 11 ggg rrr => "shrd " greg[$r] "," greg[$g] ",cl" ;
  0x0f 0xad @addr      => "shrd " $addr "," greg[$g] ",cl" ;
  0x0f 0xb2 @addr => "lss " greg[$g] "," $addr ;
  0x0f 0xb4 @addr => "lfs " greg[$g] "," $addr ;
  0x0f 0xb5 @addr => "lgs " greg[$g] "," $addr ;
  # 0F 78/79: NP = vmread/vmwrite r/m64,r64 (fixed 64-bit in long mode); the mandatory
  # 66/F2 prefixes select the AMD SSE4a extrq/insertq. 0F 78 carries two imm8 (the
  # immediate form); 0F 79 is the register form. F3 is #UD.
  0x0f 0x78 11 ggg rrr [$pp==0] => "vmread " gregq[$r] "," gregq[$g] ;
  0x0f 0x78 @addr      [$pp==0] => "vmread " $addr "," gregq[$g] ;
  0x0f 0x78 11 ggg rrr [$pp==1] @imm8 @imm8 => "extrq " ssereg[8+$r] "," hex($imm8) "," hex($imm8) ;
  0x0f 0x78 11 ggg rrr [$pp==3] @imm8 @imm8 => "insertq " ssereg[8+$g] "," ssereg[8+$r] "," hex($imm8) "," hex($imm8) ;
  0x0f 0x79 11 ggg rrr [$pp==0] => "vmwrite " gregq[$g] "," gregq[$r] ;
  0x0f 0x79 @addr      [$pp==0] => "vmwrite " gregq[$g] "," $addr ;
  0x0f 0x79 11 ggg rrr [$pp==1] => "extrq " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x79 11 ggg rrr [$pp==3] => "insertq " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0xb9 11 ggg rrr => "ud1 " greg[$g] "," greg[$r] ;
  0x0f 0xb9 @addr      => "ud1 " greg[$g] "," $addr ;
  0x0f 0xff 11 ggg rrr => "ud0 " greg[$g] "," greg[$r] ;
  0x0f 0xff @addr      => "ud0 " greg[$g] "," $addr ;
  0x0f 0xc3 @addr => "movnti " $addr "," greg[$g] ;

  # ===== SSE/SSE2 (legacy-encoded, xmm-only). The mandatory 66/F3/F2 prefix
  # selects the pd/ss/sd variant and rides in the prefix run (shown as a prefix
  # token), so one base rule per opcode round-trips all four variants byte-exact.
  # Operands draw from the fixed XMM bank (ssereg[8+..]); the reg<->mem direction
  # rides the enc/dir witness, the mod field rides the modrm capture. ===========
  0x0f 0x10 11 ggg rrr => sse10[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x10 @addr      => sse10[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0x11 11 ggg rrr => sse11[$pp] ssereg[8+$r] "," ssereg[8+$g] ;
  0x0f 0x11 @addr      => sse11[$pp] $addr "," ssereg[8+$g] ;
  # 0F 12: NP reg=movhlps / mem=movlps; 66 mem=movlpd (reg #UD); F3 movsldup; F2 movddup.
  # The mnemonic depends on the mandatory prefix AND (at NP) on mod (reg vs mem).
  0x0f 0x12 11 ggg rrr [$pp==0] => "movhlps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x12 @addr      [$pp==0] => "movlps " ssereg[8+$g] "," $addr ;
  0x0f 0x12 @addr      [$pp==1] => "movlpd " ssereg[8+$g] "," $addr ;
  0x0f 0x12 11 ggg rrr [$pp==2] => "movsldup " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x12 @addr      [$pp==2] => "movsldup " ssereg[8+$g] "," $addr ;
  0x0f 0x12 11 ggg rrr [$pp==3] => "movddup " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x12 @addr      [$pp==3] => "movddup " ssereg[8+$g] "," $addr ;
  # 0F 13: movlps/movlpd store (mem-only; reg form and F3/F2 are #UD).
  0x0f 0x13 @addr      [$pp==0] => "movlps " $addr "," ssereg[8+$g] ;
  0x0f 0x13 @addr      [$pp==1] => "movlpd " $addr "," ssereg[8+$g] ;
  0x0f 0x14 11 ggg rrr => sse14[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x14 @addr      => sse14[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0x15 11 ggg rrr => sse15[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x15 @addr      => sse15[$pp] ssereg[8+$g] "," $addr ;
  # 0F 16: NP reg=movlhps / mem=movhps; 66 mem=movhpd (reg #UD); F3 movshdup; F2 #UD.
  0x0f 0x16 11 ggg rrr [$pp==0] => "movlhps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x16 @addr      [$pp==0] => "movhps " ssereg[8+$g] "," $addr ;
  0x0f 0x16 @addr      [$pp==1] => "movhpd " ssereg[8+$g] "," $addr ;
  0x0f 0x16 11 ggg rrr [$pp==2] => "movshdup " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x16 @addr      [$pp==2] => "movshdup " ssereg[8+$g] "," $addr ;
  # 0F 17: movhps/movhpd store (mem-only; reg form and F3/F2 are #UD).
  0x0f 0x17 @addr      [$pp==0] => "movhps " $addr "," ssereg[8+$g] ;
  0x0f 0x17 @addr      [$pp==1] => "movhpd " $addr "," ssereg[8+$g] ;
  0x0f 0x28 11 ggg rrr => sse28[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x28 @addr      => sse28[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0x29 11 ggg rrr => sse29[$pp] ssereg[8+$r] "," ssereg[8+$g] ;
  0x0f 0x29 @addr      => sse29[$pp] $addr "," ssereg[8+$g] ;
  0x0f 0x2e 11 ggg rrr => sse2e[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x2e @addr      => sse2e[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0x2f 11 ggg rrr => sse2f[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x2f @addr      => sse2f[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0x51 11 ggg rrr => sse51[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x51 @addr      => sse51[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0x52 11 ggg rrr => sse52[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x52 @addr      => sse52[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0x53 11 ggg rrr => sse53[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x53 @addr      => sse53[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0x54 11 ggg rrr => sse54[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x54 @addr      => sse54[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0x55 11 ggg rrr => sse55[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x55 @addr      => sse55[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0x56 11 ggg rrr => sse56[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x56 @addr      => sse56[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0x57 11 ggg rrr => sse57[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x57 @addr      => sse57[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0x58 11 ggg rrr => sse58[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x58 @addr      => sse58[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0x59 11 ggg rrr => sse59[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x59 @addr      => sse59[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0x5a 11 ggg rrr => sse5a[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5a @addr      => sse5a[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0x5b 11 ggg rrr => sse5b[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5b @addr      => sse5b[$pp] ssereg[8+$g] "," $addr ;
  # 0F E6: cvt(t)pd2dq / cvtdq2pd -- mandatory 66/F2/F3 selects the variant and
  # rides the prefix run (one base rule round-trips all three byte-exact).
  0x0f 0xe6 11 ggg rrr => ssee6[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0xe6 @addr      => ssee6[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0x5c 11 ggg rrr => sse5c[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5c @addr      => sse5c[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0x5d 11 ggg rrr => sse5d[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5d @addr      => sse5d[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0x5e 11 ggg rrr => sse5e[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5e @addr      => sse5e[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0x5f 11 ggg rrr => sse5f[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5f @addr      => sse5f[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0xc2 11 ggg rrr @imm8 => ssec2[$pp] ssereg[8+$g] "," ssereg[8+$r] "," hex($imm8) ;
  0x0f 0xc2 @addr      @imm8 => ssec2[$pp] ssereg[8+$g] "," $addr "," hex($imm8) ;
  0x0f 0xc6 11 ggg rrr @imm8 => ssec6[$pp] ssereg[8+$g] "," ssereg[8+$r] "," hex($imm8) ;
  0x0f 0xc6 @addr      @imm8 => ssec6[$pp] ssereg[8+$g] "," $addr "," hex($imm8) ;

  # ===== SSE2 GPR<->xmm moves, movdq, packed integer. ssereg[$opsiz*8+..] =
  # OPF_SSE_OS (mm at opsiz 0, xmm at 1/2); greg[$r] takes REX.W as a 64-bit GPR
  # (movq / cvtsi2sd r64). The 66/F3/F2 selector rides the prefix run, so one base
  # rule per opcode covers movd/movq, movdqa/movdqu, cvtsi2ss/sd etc. byte-exact. =
  0x0f 0x6e 11 ggg rrr => "movd " ssereg[$opsiz*8+$g] "," gregd[$r] ;
  0x0f 0x6e @addr      => "movd " ssereg[$opsiz*8+$g] "," $addr ;
  # 0F 7E: NP/66 = movd/q r/m,mm-or-xmm (rm-first); F3 = movq xmm,xmm/m64 (reg-first,
  # a different form); F2 = #UD. Per-prefix descriptor selects mnemonic + operand form.
  0x0f 0x7e 11 ggg rrr [$pp==0] => "movd " gregd[$r] "," ssereg[$opsiz*8+$g] ;
  0x0f 0x7e @addr      [$pp==0] => "movd " $addr "," ssereg[$opsiz*8+$g] ;
  0x0f 0x7e 11 ggg rrr [$pp==1] => "movd " gregd[$r] "," ssereg[$opsiz*8+$g] ;
  0x0f 0x7e @addr      [$pp==1] => "movd " $addr "," ssereg[$opsiz*8+$g] ;
  0x0f 0x7e 11 ggg rrr [$pp==2] => "movq " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x7e @addr      [$pp==2] => "movq " ssereg[8+$g] "," $addr ;
  0x0f 0x6f 11 ggg rrr => sse6f[$pp] ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x6f @addr      => sse6f[$pp] ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x7f 11 ggg rrr => sse7f[$pp] ssereg[$opsiz*8+$r] "," ssereg[$opsiz*8+$g] ;
  0x0f 0x7f @addr      => sse7f[$pp] $addr "," ssereg[$opsiz*8+$g] ;
  # 0F D6: 66 = movq xmm/m64,xmm; F3 = movq2dq xmm,mm; F2 = movdq2q mm,xmm; NP = #UD.
  # movq2dq/movdq2q bridge the MMX and XMM banks (reg-only) -- a per-prefix form change.
  0x0f 0xd6 11 ggg rrr [$pp==1] => "movq " ssereg[8+$r] "," ssereg[8+$g] ;
  0x0f 0xd6 @addr      [$pp==1] => "movq " $addr "," ssereg[8+$g] ;
  0x0f 0xd6 11 ggg rrr [$pp==2] => "movq2dq " ssereg[8+$g] "," ssereg[$r] ;
  0x0f 0xd6 11 ggg rrr [$pp==3] => "movdq2q " ssereg[$g] "," ssereg[8+$r] ;
  # cvt int<->float: reg is always xmm on 2A / GPR-or-mm on 2C/2D; the r/m operand
  # is the reciprocal. mmxg[..] = OPF_MMG (mm at NP/66, GPR at F3/F2). The mnemonic
  # rides the $pp selector; the prefix run reproduces the exact bytes on encode.
  0x0f 0x2a 11 ggg rrr => sse2a[$pp] ssereg[8+$g] "," mmxg[$r] ;
  0x0f 0x2a @addr      => sse2a[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0x2c 11 ggg rrr => sse2c[$pp] mmxg[$g] "," ssereg[8+$r] ;
  0x0f 0x2c @addr      => sse2c[$pp] mmxg[$g] "," $addr ;
  0x0f 0x2d 11 ggg rrr => sse2d[$pp] mmxg[$g] "," ssereg[8+$r] ;
  0x0f 0x2d @addr      => sse2d[$pp] mmxg[$g] "," $addr ;
  0x0f 0xfc 11 ggg rrr => "paddb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xfc @addr      => "paddb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xfd 11 ggg rrr => "paddw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xfd @addr      => "paddw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xfe 11 ggg rrr => "paddd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xfe @addr      => "paddd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd4 11 ggg rrr => "paddq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd4 @addr      => "paddq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf8 11 ggg rrr => "psubb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf8 @addr      => "psubb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf9 11 ggg rrr => "psubw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf9 @addr      => "psubw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xfa 11 ggg rrr => "psubd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xfa @addr      => "psubd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xfb 11 ggg rrr => "psubq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xfb @addr      => "psubq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xdb 11 ggg rrr => "pand " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xdb @addr      => "pand " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xdf 11 ggg rrr => "pandn " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xdf @addr      => "pandn " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xeb 11 ggg rrr => "por " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xeb @addr      => "por " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xef 11 ggg rrr => "pxor " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xef @addr      => "pxor " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x74 11 ggg rrr => "pcmpeqb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x74 @addr      => "pcmpeqb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x75 11 ggg rrr => "pcmpeqw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x75 @addr      => "pcmpeqw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x76 11 ggg rrr => "pcmpeqd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x76 @addr      => "pcmpeqd " ssereg[$opsiz*8+$g] "," $addr ;

  # ===== SSE2 packed integer: unpack/pack/compare, shifts (by xmm), multiply,
  # add/sub saturating, min/max/avg/sad, and the GPR-result / imm8 / store forms.
  # All SSE_OS unless noted; 66 selects xmm and rides the prefix run. ============
  0x0f 0x60 11 ggg rrr => "punpcklbw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x60 @addr      => "punpcklbw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x61 11 ggg rrr => "punpcklwd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x61 @addr      => "punpcklwd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x62 11 ggg rrr => "punpckldq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x62 @addr      => "punpckldq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x63 11 ggg rrr => "packsswb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x63 @addr      => "packsswb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x64 11 ggg rrr => "pcmpgtb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x64 @addr      => "pcmpgtb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x65 11 ggg rrr => "pcmpgtw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x65 @addr      => "pcmpgtw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x66 11 ggg rrr => "pcmpgtd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x66 @addr      => "pcmpgtd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x67 11 ggg rrr => "packuswb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x67 @addr      => "packuswb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x68 11 ggg rrr => "punpckhbw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x68 @addr      => "punpckhbw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x69 11 ggg rrr => "punpckhwd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x69 @addr      => "punpckhwd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x6a 11 ggg rrr => "punpckhdq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x6a @addr      => "punpckhdq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x6b 11 ggg rrr => "packssdw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x6b @addr      => "packssdw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x6c 11 ggg rrr => "punpcklqdq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x6c @addr      => "punpcklqdq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x6d 11 ggg rrr => "punpckhqdq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x6d @addr      => "punpckhqdq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x70 11 ggg rrr @imm8 => sse70[$pp] ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x70 @addr      @imm8 => sse70[$pp] ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x50 11 ggg rrr => sse50[$pp] gregd[$g] "," ssereg[8+$r] ;
  0x0f 0xc4 11 ggg rrr @imm8 => "pinsrw " ssereg[$opsiz*8+$g] "," gregd[$r] "," hex($imm8) ;
  0x0f 0xc4 @addr      @imm8 => "pinsrw " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0xc5 11 ggg rrr @imm8 => "pextrw " gregd[$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0xd1 11 ggg rrr => "psrlw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd1 @addr      => "psrlw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd2 11 ggg rrr => "psrld " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd2 @addr      => "psrld " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd3 11 ggg rrr => "psrlq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd3 @addr      => "psrlq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd5 11 ggg rrr => "pmullw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd5 @addr      => "pmullw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd7 11 ggg rrr => "pmovmskb " gregd[$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd8 11 ggg rrr => "psubusb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd8 @addr      => "psubusb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd9 11 ggg rrr => "psubusw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd9 @addr      => "psubusw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xda 11 ggg rrr => "pminub " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xda @addr      => "pminub " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xdc 11 ggg rrr => "paddusb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xdc @addr      => "paddusb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xdd 11 ggg rrr => "paddusw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xdd @addr      => "paddusw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xde 11 ggg rrr => "pmaxub " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xde @addr      => "pmaxub " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe0 11 ggg rrr => "pavgb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe0 @addr      => "pavgb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe1 11 ggg rrr => "psraw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe1 @addr      => "psraw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe2 11 ggg rrr => "psrad " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe2 @addr      => "psrad " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe3 11 ggg rrr => "pavgw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe3 @addr      => "pavgw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe4 11 ggg rrr => "pmulhuw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe4 @addr      => "pmulhuw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe5 11 ggg rrr => "pmulhw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe5 @addr      => "pmulhw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe7 @addr      => ssee7[$pp] $addr "," ssereg[$opsiz*8+$g] ;
  0x0f 0xe8 11 ggg rrr => "psubsb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe8 @addr      => "psubsb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe9 11 ggg rrr => "psubsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe9 @addr      => "psubsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xea 11 ggg rrr => "pminsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xea @addr      => "pminsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xec 11 ggg rrr => "paddsb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xec @addr      => "paddsb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xed 11 ggg rrr => "paddsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xed @addr      => "paddsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xee 11 ggg rrr => "pmaxsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xee @addr      => "pmaxsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf1 11 ggg rrr => "psllw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf1 @addr      => "psllw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf2 11 ggg rrr => "pslld " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf2 @addr      => "pslld " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf3 11 ggg rrr => "psllq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf3 @addr      => "psllq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf4 11 ggg rrr => "pmuludq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf4 @addr      => "pmuludq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf5 11 ggg rrr => "pmaddwd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf5 @addr      => "pmaddwd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf6 11 ggg rrr => "psadbw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf6 @addr      => "psadbw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf7 11 ggg rrr => ssef7[$pp] ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;   # maskmovq/maskmovdqu
  0x0f 0x2b @addr      => sse2b[$pp] $addr "," ssereg[8+$g] ;

  # shift packed by imm8 (0F 71/72/73 groups; the /digit picks the shift, the r/m
  # is the shifted reg -- reg-direct only, no memory form). psrldq/pslldq (73 /3,
  # /7) are the 128-bit byte shifts.
  0x0f 0x71 11 010 rrr @imm8 => "psrlw " ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x71 11 100 rrr @imm8 => "psraw " ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x71 11 110 rrr @imm8 => "psllw " ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x72 11 010 rrr @imm8 => "psrld " ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x72 11 100 rrr @imm8 => "psrad " ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x72 11 110 rrr @imm8 => "pslld " ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x73 11 010 rrr @imm8 => "psrlq " ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x73 11 011 rrr @imm8 => "psrldq " ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x73 11 110 rrr @imm8 => "psllq " ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x73 11 111 rrr @imm8 => "pslldq " ssereg[$opsiz*8+$r] "," hex($imm8) ;

  # ===== SSSE3 / SSE4 three-byte maps (0F 38 / 0F 3A). Same SSE_OS model; the
  # mandatory 66 rides the prefix run; 0F 3A ops take a trailing imm8. ===========
  0x0f 0x38 0x00 11 ggg rrr => "pshufb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x00 @addr      => "pshufb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x17 11 ggg rrr => "ptest " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x17 @addr      => "ptest " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x29 11 ggg rrr => "pcmpeqq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x29 @addr      => "pcmpeqq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x30 11 ggg rrr => "pmovzxbw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x30 @addr      => "pmovzxbw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x3a 0x0b 11 ggg rrr @imm8 => "roundsd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0b @addr      @imm8 => "roundsd " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0f 11 ggg rrr @imm8 => "palignr " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0f @addr      @imm8 => "palignr " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  # -- 0F 38 (no imm8): SSSE3 horizontal/sign/abs, SSE4.1 pmovsx/zx, min/max,
  #    multiply, compare, AES. All SSE_OS xmm,xmm/mem.
  0x0f 0x38 0x01 11 ggg rrr => "phaddw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x01 @addr      => "phaddw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x02 11 ggg rrr => "phaddd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x02 @addr      => "phaddd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x03 11 ggg rrr => "phaddsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x03 @addr      => "phaddsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x04 11 ggg rrr => "pmaddubsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x04 @addr      => "pmaddubsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x05 11 ggg rrr => "phsubw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x05 @addr      => "phsubw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x06 11 ggg rrr => "phsubd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x06 @addr      => "phsubd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x07 11 ggg rrr => "phsubsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x07 @addr      => "phsubsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x08 11 ggg rrr => "psignb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x08 @addr      => "psignb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x09 11 ggg rrr => "psignw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x09 @addr      => "psignw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x0a 11 ggg rrr => "psignd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x0a @addr      => "psignd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x0b 11 ggg rrr => "pmulhrsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x0b @addr      => "pmulhrsw " ssereg[$opsiz*8+$g] "," $addr ;
  # SSE4.1 variable blend (66; implicit <xmm0> 3rd operand not shown)
  0x0f 0x38 0x10 11 ggg rrr => "pblendvb " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x38 0x10 @addr      => "pblendvb " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0x14 11 ggg rrr => "blendvps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x38 0x14 @addr      => "blendvps " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0x15 11 ggg rrr => "blendvpd " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x38 0x15 @addr      => "blendvpd " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0x1c 11 ggg rrr => "pabsb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x1c @addr      => "pabsb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x1d 11 ggg rrr => "pabsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x1d @addr      => "pabsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x1e 11 ggg rrr => "pabsd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x1e @addr      => "pabsd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x20 11 ggg rrr => "pmovsxbw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x20 @addr      => "pmovsxbw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x21 11 ggg rrr => "pmovsxbd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x21 @addr      => "pmovsxbd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x22 11 ggg rrr => "pmovsxbq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x22 @addr      => "pmovsxbq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x23 11 ggg rrr => "pmovsxwd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x23 @addr      => "pmovsxwd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x24 11 ggg rrr => "pmovsxwq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x24 @addr      => "pmovsxwq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x25 11 ggg rrr => "pmovsxdq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x25 @addr      => "pmovsxdq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x28 11 ggg rrr => "pmuldq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x28 @addr      => "pmuldq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x2a @addr      => "movntdqa " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x2b 11 ggg rrr => "packusdw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x2b @addr      => "packusdw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x31 11 ggg rrr => "pmovzxbd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x31 @addr      => "pmovzxbd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x32 11 ggg rrr => "pmovzxbq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x32 @addr      => "pmovzxbq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x33 11 ggg rrr => "pmovzxwd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x33 @addr      => "pmovzxwd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x34 11 ggg rrr => "pmovzxwq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x34 @addr      => "pmovzxwq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x35 11 ggg rrr => "pmovzxdq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x35 @addr      => "pmovzxdq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x37 11 ggg rrr => "pcmpgtq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x37 @addr      => "pcmpgtq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x38 11 ggg rrr => "pminsb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x38 @addr      => "pminsb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x39 11 ggg rrr => "pminsd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x39 @addr      => "pminsd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x3a 11 ggg rrr => "pminuw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x3a @addr      => "pminuw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x3b 11 ggg rrr => "pminud " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x3b @addr      => "pminud " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x3c 11 ggg rrr => "pmaxsb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x3c @addr      => "pmaxsb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x3d 11 ggg rrr => "pmaxsd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x3d @addr      => "pmaxsd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x3e 11 ggg rrr => "pmaxuw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x3e @addr      => "pmaxuw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x3f 11 ggg rrr => "pmaxud " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x3f @addr      => "pmaxud " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x40 11 ggg rrr => "pmulld " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x40 @addr      => "pmulld " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x41 11 ggg rrr => "phminposuw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x41 @addr      => "phminposuw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0xdb 11 ggg rrr => "aesimc " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0xdb @addr      => "aesimc " ssereg[$opsiz*8+$g] "," $addr ;
  # legacy GFNI multiply (66 0F38 CF) + VMX invept/invvpid/invpcid (66 0F38 80/81/82,
  # r64,m128 -- memory only). All are 66-mandatory (pp==1).
  0x0f 0x38 0xcf 11 ggg rrr [$pp==1] => "gf2p8mulb " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x38 0xcf @addr      [$pp==1] => "gf2p8mulb " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0x80 @addr [$pp==1] => "invept " gregq[$g] "," $addr ;
  0x0f 0x38 0x81 @addr [$pp==1] => "invvpid " gregq[$g] "," $addr ;
  0x0f 0x38 0x82 @addr [$pp==1] => "invpcid " gregq[$g] "," $addr ;
  # AES Key Locker wide (F3 0F 38 D8 /0-3 mem): a pp-variant group, F3 slot only.
  0x0f 0x38 0xd8 @addr(0) [$pp==2] => "aesencwide128kl " $addr ;
  0x0f 0x38 0xd8 @addr(1) [$pp==2] => "aesdecwide128kl " $addr ;
  0x0f 0x38 0xd8 @addr(2) [$pp==2] => "aesencwide256kl " $addr ;
  0x0f 0x38 0xd8 @addr(3) [$pp==2] => "aesdecwide256kl " $addr ;
  # AES-NI (66) vs AES Key Locker (F3), per-prefix descriptor. NP/F2 -> #UD.
  0x0f 0x38 0xdc 11 ggg rrr [$pp==1] => "aesenc " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x38 0xdc @addr      [$pp==1] => "aesenc " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0xdc 11 ggg rrr [$pp==2] => "loadiwkey " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x38 0xdc @addr      [$pp==2] => "aesenc128kl " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0xdd 11 ggg rrr [$pp==1] => "aesenclast " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x38 0xdd @addr      [$pp==1] => "aesenclast " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0xdd @addr      [$pp==2] => "aesdec128kl " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0xde 11 ggg rrr [$pp==1] => "aesdec " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x38 0xde @addr      [$pp==1] => "aesdec " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0xde @addr      [$pp==2] => "aesenc256kl " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0xdf 11 ggg rrr [$pp==1] => "aesdeclast " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x38 0xdf @addr      [$pp==1] => "aesdeclast " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0xdf @addr      [$pp==2] => "aesdec256kl " ssereg[8+$g] "," $addr ;
  # SHA-NI (NP; sha256rnds2 has an implicit <xmm0> 3rd operand not shown)
  0x0f 0x38 0xc8 11 ggg rrr => "sha1nexte " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x38 0xc8 @addr      => "sha1nexte " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0xc9 11 ggg rrr => "sha1msg1 " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x38 0xc9 @addr      => "sha1msg1 " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0xca 11 ggg rrr => "sha1msg2 " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x38 0xca @addr      => "sha1msg2 " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0xcb 11 ggg rrr => "sha256rnds2 " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x38 0xcb @addr      => "sha256rnds2 " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0xcc 11 ggg rrr => "sha256msg1 " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x38 0xcc @addr      => "sha256msg1 " ssereg[8+$g] "," $addr ;
  0x0f 0x38 0xcd 11 ggg rrr => "sha256msg2 " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x38 0xcd @addr      => "sha256msg2 " ssereg[8+$g] "," $addr ;
  # -- 0F 38 F0-FC: mnemonic flips with the mandatory prefix (per-prefix descriptor).
  #    F0/F1 movbe (NP/66, mem<->GPR) vs crc32 (F2); F6 wrssd (NP) / adcx (66) /
  #    adox (F3); F9 movdiri (NP); FC RAO-INT aadd/aand/axor/aor. pp: 0=NP..3=F2.
  0x0f 0x38 0xf0 @addr      [$pp==0] => "movbe " greg[$g] "," $addr ;         # movbe r,m (load)
  0x0f 0x38 0xf0 @addr      [$pp==1] => "movbe " greg[$g] "," $addr ;
  0x0f 0x38 0xf0 11 ggg rrr [$pp==3] => "crc32 " gregd[$g] "," rgb[$r] ;      # crc32 r32/64,r/m8
  0x0f 0x38 0xf0 @addr      [$pp==3] => "crc32 " gregd[$g] "," $addr ;
  0x0f 0x38 0xf1 @addr      [$pp==0] => "movbe " $addr "," greg[$g] ;         # movbe m,r (store)
  0x0f 0x38 0xf1 @addr      [$pp==1] => "movbe " $addr "," greg[$g] ;
  0x0f 0x38 0xf1 11 ggg rrr [$pp==3] => "crc32 " gregd[$g] "," greg[$r] ;     # crc32 r32/64,r/m
  0x0f 0x38 0xf1 @addr      [$pp==3] => "crc32 " gregd[$g] "," $addr ;
  0x0f 0x38 0xf5 @addr      [$pp==1] => "wrussd " $addr "," gregd[$g] ;       # CET write user shadow
  0x0f 0x38 0xf6 @addr      [$pp==0] => "wrssd " $addr "," gregd[$g] ;        # CET write shadow stk
  0x0f 0x38 0xf6 11 ggg rrr [$pp==1] => "adcx " gregd[$g] "," gregd[$r] ;
  0x0f 0x38 0xf6 @addr      [$pp==1] => "adcx " gregd[$g] "," $addr ;
  0x0f 0x38 0xf6 11 ggg rrr [$pp==2] => "adox " gregd[$g] "," gregd[$r] ;
  0x0f 0x38 0xf6 @addr      [$pp==2] => "adox " gregd[$g] "," $addr ;
  0x0f 0x38 0xf8 @addr      [$pp==1] => "movdir64b " gregq[$g] "," $addr ;    # movdir64b/enqcmd(s)
  0x0f 0x38 0xf8 @addr      [$pp==2] => "enqcmds " gregq[$g] "," $addr ;
  0x0f 0x38 0xf8 11 ggg rrr [$pp==2] => "uwrmsr " gregq[$g] "," gregq[$r] ;   # USER_MSR (reg form)
  0x0f 0x38 0xf8 @addr      [$pp==3] => "enqcmd " gregq[$g] "," $addr ;
  0x0f 0x38 0xf8 11 ggg rrr [$pp==3] => "urdmsr " gregq[$r] "," gregq[$g] ;    # USER_MSR read (reg form: r/m,reg; ppdesc dir_reg keeps enqcmd's mem order intact)
  0x0f 0x38 0xf9 @addr      [$pp==0] => "movdiri " $addr "," gregd[$g] ;      # movdiri m,r32/64
  0x0f 0x38 0xfa 11 ggg rrr [$pp==2] => "encodekey128 " gregd[$g] "," gregd[$r] ;  # Key Locker
  0x0f 0x38 0xfb 11 ggg rrr [$pp==2] => "encodekey256 " gregd[$g] "," gregd[$r] ;
  0x0f 0x38 0xfc @addr      [$pp==0] => "aadd " $addr "," gregd[$g] ;         # RAO-INT
  0x0f 0x38 0xfc @addr      [$pp==1] => "aand " $addr "," gregd[$g] ;
  0x0f 0x38 0xfc @addr      [$pp==2] => "axor " $addr "," gregd[$g] ;
  0x0f 0x38 0xfc @addr      [$pp==3] => "aor " $addr "," gregd[$g] ;
  # -- 0F 3A (imm8): rounds, blends, pextr/pinsr (GPR + imm8), dp, mpsadbw,
  #    pclmulqdq, the SSE4.2 string compares.
  0x0f 0x3a 0x08 11 ggg rrr @imm8 => "roundps " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x08 @addr      @imm8 => "roundps " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x09 11 ggg rrr @imm8 => "roundpd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x09 @addr      @imm8 => "roundpd " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0a 11 ggg rrr @imm8 => "roundss " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0a @addr      @imm8 => "roundss " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0c 11 ggg rrr @imm8 => "blendps " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0c @addr      @imm8 => "blendps " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0d 11 ggg rrr @imm8 => "blendpd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0d @addr      @imm8 => "blendpd " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0e 11 ggg rrr @imm8 => "pblendw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0e @addr      @imm8 => "pblendw " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x14 11 ggg rrr @imm8 => "pextrb " gregd[$r] "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x14 @addr      @imm8 => "pextrb " $addr "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x16 11 ggg rrr @imm8 => "pextrd " gregd[$r] "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x16 @addr      @imm8 => "pextrd " $addr "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  # SSE4.1 pextrw r/m16 form (0F3A 15) -- distinct from the 0F C5 pextrw r32,xmm form
  0x0f 0x3a 0x15 11 ggg rrr @imm8 => "pextrw " gregd[$r] "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x15 @addr      @imm8 => "pextrw " $addr "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  # legacy AES key-assist + GFNI affine (66 0F3A DF/CE/CF ib); 66-mandatory (pp==1)
  0x0f 0x3a 0xdf 11 ggg rrr @imm8 [$pp==1] => "aeskeygenassist " ssereg[8+$g] "," ssereg[8+$r] "," hex($imm8) ;
  0x0f 0x3a 0xdf @addr      @imm8 [$pp==1] => "aeskeygenassist " ssereg[8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0xce 11 ggg rrr @imm8 [$pp==1] => "gf2p8affineqb " ssereg[8+$g] "," ssereg[8+$r] "," hex($imm8) ;
  0x0f 0x3a 0xce @addr      @imm8 [$pp==1] => "gf2p8affineqb " ssereg[8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0xcf 11 ggg rrr @imm8 [$pp==1] => "gf2p8affineinvqb " ssereg[8+$g] "," ssereg[8+$r] "," hex($imm8) ;
  0x0f 0x3a 0xcf @addr      @imm8 [$pp==1] => "gf2p8affineinvqb " ssereg[8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x17 11 ggg rrr @imm8 => "extractps " gregd[$r] "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x17 @addr      @imm8 => "extractps " $addr "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x20 11 ggg rrr @imm8 => "pinsrb " ssereg[$opsiz*8+$g] "," gregd[$r] "," hex($imm8) ;
  0x0f 0x3a 0x20 @addr      @imm8 => "pinsrb " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x21 11 ggg rrr @imm8 => "insertps " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x21 @addr      @imm8 => "insertps " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x22 11 ggg rrr @imm8 => "pinsrd " ssereg[$opsiz*8+$g] "," gregd[$r] "," hex($imm8) ;
  0x0f 0x3a 0x22 @addr      @imm8 => "pinsrd " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x40 11 ggg rrr @imm8 => "dpps " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x40 @addr      @imm8 => "dpps " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x41 11 ggg rrr @imm8 => "dppd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x41 @addr      @imm8 => "dppd " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x42 11 ggg rrr @imm8 => "mpsadbw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x42 @addr      @imm8 => "mpsadbw " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x44 11 ggg rrr @imm8 => "pclmulqdq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x44 @addr      @imm8 => "pclmulqdq " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0xcc 11 ggg rrr @imm8 => "sha1rnds4 " ssereg[8+$g] "," ssereg[8+$r] "," hex($imm8) ;  # SHA-NI (NP)
  0x0f 0x3a 0xcc @addr      @imm8 => "sha1rnds4 " ssereg[8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x60 11 ggg rrr @imm8 => "pcmpestrm " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x60 @addr      @imm8 => "pcmpestrm " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x61 11 ggg rrr @imm8 => "pcmpestri " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x61 @addr      @imm8 => "pcmpestri " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x62 11 ggg rrr @imm8 => "pcmpistrm " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x62 @addr      @imm8 => "pcmpistrm " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x63 11 ggg rrr @imm8 => "pcmpistri " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x63 @addr      @imm8 => "pcmpistri " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;

  # ===== SSE3 horizontal add/sub, addsub, lddqu (xmm; 66/F2 ride the prefix run).
  # movddup/movsldup/movshdup (F2/F3 0F 12/16) round-trip via the movlps/movhps
  # rules + prefix replay, so only these distinct opcodes need rules. ============
  0x0f 0x7c 11 ggg rrr => sse7c[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x7c @addr      => sse7c[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0x7d 11 ggg rrr => sse7d[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x7d @addr      => sse7d[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0xd0 11 ggg rrr => ssed0[$pp] ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0xd0 @addr      => ssed0[$pp] ssereg[8+$g] "," $addr ;
  0x0f 0xf0 @addr      => "lddqu " ssereg[8+$g] "," $addr ;

  # ===== x87 FPU (D8-DF). Each opcode is a group: 8 memory forms (/digit) +
  # 8 register forms (mod==11, st(i) in rm). The operand-size suffix and the
  # control-region mnemonics are cosmetic; the modrm/sib/disp round-trips via
  # the addressing witness (mem) or the captured rm (reg), so every D8-DF byte
  # sequence is byte-exact. Ported verbatim from the 32-bit corpus.
  # ---- D8 ----
  0xd8 11 000 rrr => "fadd st(" dec($r) ")" ;
  0xd8 11 001 rrr => "fmul st(" dec($r) ")" ;
  0xd8 11 010 rrr => "fcom st(" dec($r) ")" ;
  0xd8 11 011 rrr => "fcomp st(" dec($r) ")" ;
  0xd8 11 100 rrr => "fsub st(" dec($r) ")" ;
  0xd8 11 101 rrr => "fsubr st(" dec($r) ")" ;
  0xd8 11 110 rrr => "fdiv st(" dec($r) ")" ;
  0xd8 11 111 rrr => "fdivr st(" dec($r) ")" ;
  0xd8 @addr(0)   => "fadd.d " $addr ;
  0xd8 @addr(1)   => "fmul.d " $addr ;
  0xd8 @addr(2)   => "fcom.d " $addr ;
  0xd8 @addr(3)   => "fcomp.d " $addr ;
  0xd8 @addr(4)   => "fsub.d " $addr ;
  0xd8 @addr(5)   => "fsubr.d " $addr ;
  0xd8 @addr(6)   => "fdiv.d " $addr ;
  0xd8 @addr(7)   => "fdivr.d " $addr ;
  # ---- D9 ----
  0xd9 11 010 000 => "fnop" ;
  0xd9 11 100 000 => "fchs" ;
  0xd9 11 100 001 => "fabs" ;
  0xd9 11 100 100 => "ftst" ;
  0xd9 11 100 101 => "fxam" ;
  0xd9 11 101 000 => "fld1" ;
  0xd9 11 101 001 => "fldl2t" ;
  0xd9 11 101 010 => "fldl2e" ;
  0xd9 11 101 011 => "fldpi" ;
  0xd9 11 101 100 => "fldlg2" ;
  0xd9 11 101 101 => "fldln2" ;
  0xd9 11 101 110 => "fldz" ;
  0xd9 11 110 000 => "f2xm1" ;
  0xd9 11 110 001 => "fyl2x" ;
  0xd9 11 110 010 => "fptan" ;
  0xd9 11 110 011 => "fpatan" ;
  0xd9 11 110 100 => "fxtract" ;
  0xd9 11 110 101 => "fprem1" ;
  0xd9 11 110 110 => "fdecstp" ;
  0xd9 11 110 111 => "fincstp" ;
  0xd9 11 111 000 => "fprem" ;
  0xd9 11 111 001 => "fyl2xp1" ;
  0xd9 11 111 010 => "fsqrt" ;
  0xd9 11 111 011 => "fsincos" ;
  0xd9 11 111 100 => "frndint" ;
  0xd9 11 111 101 => "fscale" ;
  0xd9 11 111 110 => "fsin" ;
  0xd9 11 111 111 => "fcos" ;
  0xd9 11 000 rrr => "fld st(" dec($r) ")" ;
  0xd9 11 001 rrr => "fxch st(" dec($r) ")" ;
  0xd9 @addr(0)   => "fld.d " $addr ;
  0xd9 @addr(2)   => "fst.d " $addr ;
  0xd9 @addr(3)   => "fstp.d " $addr ;
  0xd9 @addr(4)   => "fldenv " $addr ;
  0xd9 @addr(5)   => "fldcw " $addr ;
  0xd9 @addr(6)   => "fnstenv " $addr ;
  0xd9 @addr(7)   => "fnstcw " $addr ;
  # ---- DA ----
  0xda 11 101 001 => "fucompp" ;
  0xda 11 000 rrr => "fcmovb st(" dec($r) ")" ;
  0xda 11 001 rrr => "fcmove st(" dec($r) ")" ;
  0xda 11 010 rrr => "fcmovbe st(" dec($r) ")" ;
  0xda 11 011 rrr => "fcmovu st(" dec($r) ")" ;
  0xda @addr(0)   => "fiadd.d " $addr ;
  0xda @addr(1)   => "fimul.d " $addr ;
  0xda @addr(2)   => "ficom.d " $addr ;
  0xda @addr(3)   => "ficomp.d " $addr ;
  0xda @addr(4)   => "fisub.d " $addr ;
  0xda @addr(5)   => "fisubr.d " $addr ;
  0xda @addr(6)   => "fidiv.d " $addr ;
  0xda @addr(7)   => "fidivr.d " $addr ;
  # ---- DB ----
  0xdb 11 100 000 => "fneni" ;
  0xdb 11 100 001 => "fndisi" ;
  0xdb 11 100 010 => "fnclex" ;
  0xdb 11 100 011 => "fninit" ;
  0xdb 11 100 100 => "fnsetpm" ;
  0xdb 11 100 101 => "frstpm" ;
  0xdb 11 000 rrr => "fcmovnb st(" dec($r) ")" ;
  0xdb 11 001 rrr => "fcmovne st(" dec($r) ")" ;
  0xdb 11 010 rrr => "fcmovnbe st(" dec($r) ")" ;
  0xdb 11 011 rrr => "fcmovnu st(" dec($r) ")" ;
  0xdb 11 101 rrr => "fucomi st(" dec($r) ")" ;
  0xdb 11 110 rrr => "fcomi st(" dec($r) ")" ;
  0xdb @addr(0)   => "fild.d " $addr ;
  0xdb @addr(1)   => "fisttp.d " $addr ;
  0xdb @addr(2)   => "fist.d " $addr ;
  0xdb @addr(3)   => "fistp.d " $addr ;
  0xdb @addr(5)   => "fld.t " $addr ;
  0xdb @addr(7)   => "fstp.t " $addr ;
  # ---- DC ----
  0xdc 11 000 rrr => "fadd st(" dec($r) ")" ;
  0xdc 11 001 rrr => "fmul st(" dec($r) ")" ;
  0xdc 11 100 rrr => "fsubr st(" dec($r) ")" ;
  0xdc 11 101 rrr => "fsub st(" dec($r) ")" ;
  0xdc 11 110 rrr => "fdivr st(" dec($r) ")" ;
  0xdc 11 111 rrr => "fdiv st(" dec($r) ")" ;
  # ---- undocumented x87 reg-direct aliases (real on silicon; objdump rejects, other
  # decoders accept). Redundant encodings of fcom/fcomp/fxch/fstp/fstpnce. ----------
  0xdc 11 010 rrr => "fcom st(" dec($r) ")" ;      # DC /2 = fcom st(i)  (alt of D8 /2)
  0xdc 11 011 rrr => "fcomp st(" dec($r) ")" ;     # DC /3 = fcomp st(i) (alt of D8 /3)
  0xde 11 010 rrr => "fcomp st(" dec($r) ")" ;     # DE /2 = fcomp st(i)
  0xdd 11 001 rrr => "fxch st(" dec($r) ")" ;      # DD /1 = fxch st(i)  (alt of D9 /1)
  0xdf 11 001 rrr => "fxch st(" dec($r) ")" ;      # DF /1 = fxch st(i)
  0xdf 11 010 rrr => "fstp st(" dec($r) ")" ;      # DF /2 = fstp st(i)  (alt of DD /3)
  0xdf 11 011 rrr => "fstp st(" dec($r) ")" ;      # DF /3 = fstp st(i)
  0xd9 11 011 rrr => "fstpnce st(" dec($r) ")" ;   # D9 /3 = fstpnce st(i) (undocumented)
  0xdc @addr(0)   => "fadd.q " $addr ;
  0xdc @addr(1)   => "fmul.q " $addr ;
  0xdc @addr(2)   => "fcom.q " $addr ;
  0xdc @addr(3)   => "fcomp.q " $addr ;
  0xdc @addr(4)   => "fsub.q " $addr ;
  0xdc @addr(5)   => "fsubr.q " $addr ;
  0xdc @addr(6)   => "fdiv.q " $addr ;
  0xdc @addr(7)   => "fdivr.q " $addr ;
  # ---- DD ----
  0xdd 11 000 rrr => "ffree st(" dec($r) ")" ;
  0xdd 11 010 rrr => "fst st(" dec($r) ")" ;
  0xdd 11 011 rrr => "fstp st(" dec($r) ")" ;
  0xdd 11 100 rrr => "fucom st(" dec($r) ")" ;
  0xdd 11 101 rrr => "fucomp st(" dec($r) ")" ;
  0xdd @addr(0)   => "fld.q " $addr ;
  0xdd @addr(1)   => "fisttp.q " $addr ;
  0xdd @addr(2)   => "fst.q " $addr ;
  0xdd @addr(3)   => "fstp.q " $addr ;
  0xdd @addr(4)   => "frstor " $addr ;
  0xdd @addr(6)   => "fnsave " $addr ;
  0xdd @addr(7)   => "fnstsw " $addr ;
  # ---- DE ----
  0xde 11 011 001 => "fcompp" ;
  0xde 11 000 rrr => "faddp st(" dec($r) ")" ;
  0xde 11 001 rrr => "fmulp st(" dec($r) ")" ;
  0xde 11 100 rrr => "fsubrp st(" dec($r) ")" ;
  0xde 11 101 rrr => "fsubp st(" dec($r) ")" ;
  0xde 11 110 rrr => "fdivrp st(" dec($r) ")" ;
  0xde 11 111 rrr => "fdivp st(" dec($r) ")" ;
  0xde @addr(0)   => "fiadd.w " $addr ;
  0xde @addr(1)   => "fimul.w " $addr ;
  0xde @addr(2)   => "ficom.w " $addr ;
  0xde @addr(3)   => "ficomp.w " $addr ;
  0xde @addr(4)   => "fisub.w " $addr ;
  0xde @addr(5)   => "fisubr.w " $addr ;
  0xde @addr(6)   => "fidiv.w " $addr ;
  0xde @addr(7)   => "fidivr.w " $addr ;
  # ---- DF ----
  0xdf 11 100 000 => "fnstsw ax" ;
  0xdf 11 000 rrr => "ffreep st(" dec($r) ")" ;
  0xdf 11 101 rrr => "fucomip st(" dec($r) ")" ;
  0xdf 11 110 rrr => "fcomip st(" dec($r) ")" ;
  0xdf @addr(0)   => "fild.w " $addr ;
  0xdf @addr(1)   => "fisttp.w " $addr ;
  0xdf @addr(2)   => "fist.w " $addr ;
  0xdf @addr(3)   => "fistp.w " $addr ;
  0xdf @addr(4)   => "fbld " $addr ;
  0xdf @addr(5)   => "fild.q " $addr ;
  0xdf @addr(6)   => "fbstp " $addr ;
  0xdf @addr(7)   => "fistp.q " $addr ;


  # ===== legacy / system 0F ops (sldt/str/lldt/ltr/verr/verw, sgdt/lgdt/.../
  # invlpg + reg sub-ops, lar/lsl, clts/invd/wbinvd, sysret/sysenter/sysexit,
  # wrmsr/rdmsr/rdpmc, emms, push/pop fs/gs). The 0F 01 register sub-ops
  # (monitor/xgetbv/swapgs/rdtscp/...) capture the rm so each fixed modrm
  # round-trips; mnemonics are cosmetic. =======================================
  0x0f 0x00 11 000 rrr => "sldt " greg[$r] ;
  0x0f 0x00 @addr(0)   => "sldt" sfx[2] " " $addr ;
  0x0f 0x00 11 001 rrr => "str " greg[$r] ;
  0x0f 0x00 @addr(1)   => "str" sfx[2] " " $addr ;
  0x0f 0x00 11 010 rrr => "lldt " greg[$r] ;
  0x0f 0x00 @addr(2)   => "lldt" sfx[2] " " $addr ;
  0x0f 0x00 11 011 rrr => "ltr " greg[$r] ;
  0x0f 0x00 @addr(3)   => "ltr" sfx[2] " " $addr ;
  0x0f 0x00 11 100 rrr => "verr " greg[$r] ;
  0x0f 0x00 @addr(4)   => "verr" sfx[2] " " $addr ;
  0x0f 0x00 11 101 rrr => "verw " greg[$r] ;
  0x0f 0x00 @addr(5)   => "verw" sfx[2] " " $addr ;
  0x0f 0x00 11 110 rrr [$pp==3] => "lkgs " greg[$r] ;   # /6 reg F2: FRED lkgs r/m16
  0x0f 0x00 @addr(6)   [$pp==3] => "lkgs " $addr ;
  0x0f 0x01 @addr(0)   => "sgdt " $addr ;
  0x0f 0x01 @addr(1)   => "sidt " $addr ;
  0x0f 0x01 @addr(2)   => "lgdt " $addr ;
  0x0f 0x01 @addr(3)   => "lidt " $addr ;
  0x0f 0x01 @addr(4)   => "smsw " $addr ;
  0x0f 0x01 @addr(5)   => "rstorssp " $addr ;
  0x0f 0x01 @addr(6)   => "lmsw " $addr ;
  0x0f 0x01 @addr(7)   => "invlpg " $addr ;
  # 0F 01 Cx-Fx: fully-fixed ModR/M system ops (no operand). reg=4 (smsw) and reg=6
  # (lmsw) stay group reg-forms; every other reg-form byte is one of these or #UD.
  0x0f 0x01 11 000 000 => "enclv" ;
  0x0f 0x01 11 000 001 => "vmcall" ;
  0x0f 0x01 11 000 010 => "vmlaunch" ;
  0x0f 0x01 11 000 011 => "vmresume" ;
  0x0f 0x01 11 000 100 => "vmxoff" ;
  0x0f 0x01 11 000 101 => "pconfig" ;
  0x0f 0x01 11 000 110 => "wrmsrns" ;
  0x0f 0x01 11 000 111 => "pbndkb" ;
  0x0f 0x01 11 001 000 => "monitor" ;
  0x0f 0x01 11 001 001 => "mwait" ;
  0x0f 0x01 11 001 010 => "clac" ;
  0x0f 0x01 11 001 011 => "stac" ;
  0x0f 0x01 11 001 111 => "encls" ;
  0x0f 0x01 11 010 000 => "xgetbv" ;
  0x0f 0x01 11 010 001 => "xsetbv" ;
  0x0f 0x01 11 010 100 => "vmfunc" ;
  0x0f 0x01 11 010 101 => "xend" ;
  0x0f 0x01 11 010 110 => "xtest" ;
  0x0f 0x01 11 010 111 => "enclu" ;
  0x0f 0x01 11 011 000 => "vmrun" ;
  0x0f 0x01 11 011 001 => "vmmcall" ;
  0x0f 0x01 11 011 010 => "vmload" ;
  0x0f 0x01 11 011 011 => "vmsave" ;
  0x0f 0x01 11 011 100 => "stgi" ;
  0x0f 0x01 11 011 101 => "clgi" ;
  0x0f 0x01 11 011 110 => "skinit" ;
  0x0f 0x01 11 011 111 => "invlpga" ;
  0x0f 0x01 11 100 rrr => "smsw " greg[$r] ;
  0x0f 0x01 11 101 000 => "serialize" ;
  0x0f 0x01 11 101 000 [$pp==3] => "xsusldtrk" ;        # F2 0F01 E8: TSXLDTRK suspend
  0x0f 0x01 11 101 001 [$pp==3] => "xresldtrk" ;        # F2 0F01 E9: TSXLDTRK resume
  0x0f 0x01 11 101 110 => "rdpkru" ;
  0x0f 0x01 11 101 111 => "wrpkru" ;
  0x0f 0x01 11 110 rrr => "lmsw " gregw[$r] ;
  0x0f 0x01 11 111 000 => "swapgs" ;
  0x0f 0x01 11 111 001 => "rdtscp" ;
  0x0f 0x01 11 111 010 => "monitorx" ;
  0x0f 0x01 11 111 011 => "mwaitx" ;
  0x0f 0x01 11 111 100 => "clzero" ;
  0x0f 0x01 11 111 101 => "rdpru" ;
  0x0f 0x01 11 111 110 => "invlpgb" ;
  0x0f 0x01 11 111 111 => "tlbsync" ;
  0x0f 0x02 11 ggg rrr => "lar " greg[$g] "," greg[$r] ;
  0x0f 0x02 @addr      => "lar " greg[$g] "," $addr ;
  0x0f 0x03 11 ggg rrr => "lsl " greg[$g] "," greg[$r] ;
  0x0f 0x03 @addr      => "lsl " greg[$g] "," $addr ;
  0x0f 0x06 => "clts" ;
  0x0f 0x07 => sret[$opsiz] ;
  0x0f 0x08 => "invd" ;
  0x0f 0x09 [$pp==0] => "wbinvd" ;
  0x0f 0x09 [$pp==2] => "wbnoinvd" ;                    # F3 selects wbnoinvd
  0x0f 0x09 [$pp==1] => "wbinvd" ;                      # 66 has no meaning here -> ignored (wbinvd)
  0x0f 0x09 [$pp==3] => "wbinvd" ;                      # F2 likewise ignored (real HW decodes wbinvd)
  # VIA PadLock (0F A6/A7): fully-fixed ModR/M crypto ops; the rep (F3) prefix that
  # accompanies them in practice rides the prefix run and replays.
  0x0f 0xa6 11 000 000 => "montmul" ;
  0x0f 0xa6 11 001 000 => "xsha1" ;
  0x0f 0xa6 11 010 000 => "xsha256" ;
  0x0f 0xa7 11 000 000 => "xstore-rng" ;
  0x0f 0xa7 11 001 000 => "xcrypt-ecb" ;
  0x0f 0xa7 11 010 000 => "xcrypt-cbc" ;
  0x0f 0xa7 11 011 000 => "xcrypt-ctr" ;
  0x0f 0xa7 11 100 000 => "xcrypt-cfb" ;
  0x0f 0xa7 11 101 000 => "xcrypt-ofb" ;
  0x0f 0x30 => "wrmsr" ;
  0x0f 0x32 => "rdmsr" ;
  0x0f 0x33 => "rdpmc" ;
  0x0f 0x34 => "sysenter" ;
  0x0f 0x35 => sxit[$opsiz] ;
  0x0f 0x77 => "emms" ;
  0x0f 0xa0 => pushfs[$opsiz] ;                        # 66 -> pushw fs
  0x0f 0xa1 => popfs[$opsiz] ;
  0x0f 0xa8 => pushgs[$opsiz] ;
  0x0f 0xa9 => popgs[$opsiz] ;
}

# ===========================================================================
# VEX (AVX/AVX2) -- ported from the root corpus64.p. These submatches are NOT
# walked by the decode FSM (the VEX prefix fields don't fit the 32 capture
# slots; REX has the same problem and is likewise handled in C++). gen.py
# compiles them into a (map,pp,W,opcode) -> descriptor table (mnemonic +
# operand roles); the C++ vex_decode reads the raw VEX bytes, looks up the
# descriptor, and fills the real mnemonic + operands. Byte-exact round-trip is
# unchanged (the encoder still replays the raw prefix + witnessed tail).
# ===========================================================================

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
table vregd { xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7,xmm8,xmm9,xmm10,xmm11,xmm12,xmm13,xmm14,xmm15, ymm0,ymm1,ymm2,ymm3,ymm4,ymm5,ymm6,ymm7,ymm8,ymm9,ymm10,ymm11,ymm12,ymm13,ymm14,ymm15 }   # XOP is4 4th operand (direct)
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
# half-width reg/rm for width-changing converts (one size class smaller than the
# full operand, which is what selects L'L): L'L=0->xmm, L'L=1->xmm, L'L=2->ymm.
table eregh {
  xmm24, xmm25, xmm26, xmm27, xmm28, xmm29, xmm30, xmm31, xmm16, xmm17, xmm18, xmm19, xmm20, xmm21, xmm22, xmm23, xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7,
  xmm24, xmm25, xmm26, xmm27, xmm28, xmm29, xmm30, xmm31, xmm16, xmm17, xmm18, xmm19, xmm20, xmm21, xmm22, xmm23, xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7,
  ymm24, ymm25, ymm26, ymm27, ymm28, ymm29, ymm30, ymm31, ymm16, ymm17, ymm18, ymm19, ymm20, ymm21, ymm22, ymm23, ymm8, ymm9, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15, ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6, ymm7
}
# quarter/eighth-width reg/rm for vpmovzx/sx (b->d/q, w->q) and the matching
# vpmov*b / vpmovqw truncates: the narrow operand is xmm regardless of L'L.
table eregx {
  xmm24, xmm25, xmm26, xmm27, xmm28, xmm29, xmm30, xmm31, xmm16, xmm17, xmm18, xmm19, xmm20, xmm21, xmm22, xmm23, xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7,
  xmm24, xmm25, xmm26, xmm27, xmm28, xmm29, xmm30, xmm31, xmm16, xmm17, xmm18, xmm19, xmm20, xmm21, xmm22, xmm23, xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7,
  xmm24, xmm25, xmm26, xmm27, xmm28, xmm29, xmm30, xmm31, xmm16, xmm17, xmm18, xmm19, xmm20, xmm21, xmm22, xmm23, xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7
}
# mask {k1..k7} + zeroing {z}: index z*8 + aaa
table kzdec { "", " {k1}", " {k2}", " {k3}", " {k4}", " {k5}", " {k6}", " {k7}", "", " {k1} {z}", " {k2} {z}", " {k3} {z}", " {k4} {z}", " {k5} {z}", " {k6} {z}", " {k7} {z}" }
# mask only (no zeroing form): index aaa
table kdec  { "", " {k1}", " {k2}", " {k3}", " {k4}", " {k5}", " {k6}", " {k7}" }
# broadcast suffix by L'L (16-/32-/64-bit element; 16-bit = AVX512-FP16)
table bcst16 { " {1to8}", " {1to16}", " {1to32}", "" }
table bcst32 { " {1to4}", " {1to8}", " {1to16}", "" }
table bcst64 { " {1to2}", " {1to4}", " {1to8}", "" }
# embedded-rounding suffix by L'L (b=1, reg-reg)
table rcdec { " {rn-sae}", " {rd-sae}", " {ru-sae}", " {rz-sae}" }
table vmova { vmovaps,vmovapd,"","" }   # VEX 0F 28/29 by pp
table vmovu { vmovups,vmovupd,"","" }   # VEX 0F 10/11 (packed) by pp
table vmdq  { "",vmovdqa,vmovdqu,"" }   # VEX 0F 6F/7F by pp

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
  # ==== VEX map1 completion: NP/F3/F2 + 66-remainder (moves/compares/msk/scalar) ====
  h k b 00001 0 vvvv y 00 0x14 11 ggg rrr => "vunpcklps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h vvvv y 00 0x14 11 ggg rrr => "vunpcklps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h k b 00001 0 vvvv y 00 0x14 @addr {$rexb=1-$b;$rexx=1-$k} => "vunpcklps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 00 0x14 @addr => "vunpcklps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 00 0x15 11 ggg rrr => "vunpckhps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h vvvv y 00 0x15 11 ggg rrr => "vunpckhps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h k b 00001 0 vvvv y 00 0x15 @addr {$rexb=1-$b;$rexx=1-$k} => "vunpckhps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 00 0x15 @addr => "vunpckhps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 w 1111 y 00 0x2e 11 ggg rrr => "vucomiss " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 w 1111 y 00 0x2e @addr {$rexb=1-$b;$rexx=1-$k} => "vucomiss " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y 00 0x2e 11 ggg rrr => "vucomiss " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 00 0x2e @addr => "vucomiss " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00001 0 1111 y 00 0x2f 11 ggg rrr => "vcomiss " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 1111 y 00 0x2f @addr {$rexb=1-$b;$rexx=1-$k} => "vcomiss " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y 00 0x2f 11 ggg rrr => "vcomiss " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 00 0x2f @addr => "vcomiss " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00001 0 1111 y 00 0x50 11 ggg rrr => "vmovmskps " greg[$g] "," vreg[16*$y+8*$b+$r] ;
  h 1111 y 00 0x50 11 ggg rrr => "vmovmskps " greg[$g] "," vreg[16*$y+8+$r] ;
  h k b 00001 0 1111 y 00 0x52 11 ggg rrr => "vrsqrtps " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 1111 y 00 0x52 @addr {$rexb=1-$b;$rexx=1-$k} => "vrsqrtps " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y 00 0x52 11 ggg rrr => "vrsqrtps " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 00 0x52 @addr => "vrsqrtps " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00001 0 1111 y 00 0x53 11 ggg rrr => "vrcpps " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 1111 y 00 0x53 @addr {$rexb=1-$b;$rexx=1-$k} => "vrcpps " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y 00 0x53 11 ggg rrr => "vrcpps " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 00 0x53 @addr => "vrcpps " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00001 w 1111 y 01 0x2e 11 ggg rrr => "vucomisd " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 w 1111 y 01 0x2e @addr {$rexb=1-$b;$rexx=1-$k} => "vucomisd " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y 01 0x2e 11 ggg rrr => "vucomisd " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 01 0x2e @addr => "vucomisd " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00001 0 1111 y 01 0x2f 11 ggg rrr => "vcomisd " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 1111 y 01 0x2f @addr {$rexb=1-$b;$rexx=1-$k} => "vcomisd " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y 01 0x2f 11 ggg rrr => "vcomisd " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 01 0x2f @addr => "vcomisd " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00001 0 1111 y 01 0x50 11 ggg rrr => "vmovmskpd " greg[$g] "," vreg[16*$y+8*$b+$r] ;
  h 1111 y 01 0x50 11 ggg rrr => "vmovmskpd " greg[$g] "," vreg[16*$y+8+$r] ;
  h k b 00001 0 1111 y 01 0xd7 11 ggg rrr => "vpmovmskb " greg[$g] "," vreg[16*$y+8*$b+$r] ;
  h 1111 y 01 0xd7 11 ggg rrr => "vpmovmskb " greg[$g] "," vreg[16*$y+8+$r] ;
  h k b 00001 0 1111 y 10 0x12 11 ggg rrr => "vmovsldup " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 1111 y 10 0x12 @addr {$rexb=1-$b;$rexx=1-$k} => "vmovsldup " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y 10 0x12 11 ggg rrr => "vmovsldup " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 10 0x12 @addr => "vmovsldup " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00001 w 1111 y 10 0x16 11 ggg rrr => "vmovshdup " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 w 1111 y 10 0x16 @addr {$rexb=1-$b;$rexx=1-$k} => "vmovshdup " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y 10 0x16 11 ggg rrr => "vmovshdup " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 10 0x16 @addr => "vmovshdup " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00001 0 vvvv y 10 0x52 11 ggg rrr => "vrsqrtss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h vvvv y 10 0x52 11 ggg rrr => "vrsqrtss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h k b 00001 0 vvvv y 10 0x52 @addr {$rexb=1-$b;$rexx=1-$k} => "vrsqrtss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 10 0x52 @addr => "vrsqrtss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 10 0x53 11 ggg rrr => "vrcpss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h vvvv y 10 0x53 11 ggg rrr => "vrcpss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h k b 00001 0 vvvv y 10 0x53 @addr {$rexb=1-$b;$rexx=1-$k} => "vrcpss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 10 0x53 @addr => "vrcpss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 10 0x5a 11 ggg rrr => "vcvtss2sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h vvvv y 10 0x5a 11 ggg rrr => "vcvtss2sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h k b 00001 0 vvvv y 10 0x5a @addr {$rexb=1-$b;$rexx=1-$k} => "vcvtss2sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 10 0x5a @addr => "vcvtss2sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 1111 y 10 0x70 11 ggg rrr @imm8 => "vpshufhw " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00001 0 1111 y 10 0x70 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vpshufhw " vreg[16*$y+8*$h+$g] "," $addr "," hex($imm8) ;
  h 1111 y 10 0x70 11 ggg rrr @imm8 => "vpshufhw " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] "," hex($imm8) ;
  h 1111 y 10 0x70 @addr @imm8 => "vpshufhw " vreg[16*$y+8*$h+$g] "," $addr "," hex($imm8) ;
  h k b 00001 0 1111 y 11 0x12 11 ggg rrr => "vmovddup " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 1111 y 11 0x12 @addr {$rexb=1-$b;$rexx=1-$k} => "vmovddup " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y 11 0x12 11 ggg rrr => "vmovddup " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 11 0x12 @addr => "vmovddup " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00001 0 vvvv y 11 0x5a 11 ggg rrr => "vcvtsd2ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h vvvv y 11 0x5a 11 ggg rrr => "vcvtsd2ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h k b 00001 0 vvvv y 11 0x5a @addr {$rexb=1-$b;$rexx=1-$k} => "vcvtsd2ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 11 0x5a @addr => "vcvtsd2ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 1111 y 11 0x70 11 ggg rrr @imm8 => "vpshuflw " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00001 0 1111 y 11 0x70 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vpshuflw " vreg[16*$y+8*$h+$g] "," $addr "," hex($imm8) ;
  h 1111 y 11 0x70 11 ggg rrr @imm8 => "vpshuflw " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] "," hex($imm8) ;
  h 1111 y 11 0x70 @addr @imm8 => "vpshuflw " vreg[16*$y+8*$h+$g] "," $addr "," hex($imm8) ;
  h k b 00001 0 vvvv y 11 0x7c 11 ggg rrr => "vhaddps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h vvvv y 11 0x7c 11 ggg rrr => "vhaddps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h k b 00001 0 vvvv y 11 0x7c @addr {$rexb=1-$b;$rexx=1-$k} => "vhaddps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 11 0x7c @addr => "vhaddps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 11 0x7d 11 ggg rrr => "vhsubps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h vvvv y 11 0x7d 11 ggg rrr => "vhsubps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h k b 00001 0 vvvv y 11 0x7d @addr {$rexb=1-$b;$rexx=1-$k} => "vhsubps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 11 0x7d @addr => "vhsubps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 11 0xd0 11 ggg rrr => "vaddsubps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h vvvv y 11 0xd0 11 ggg rrr => "vaddsubps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h k b 00001 0 vvvv y 11 0xd0 @addr {$rexb=1-$b;$rexx=1-$k} => "vaddsubps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 11 0xd0 @addr => "vaddsubps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  # ==== VEX map1 specials: movl/h loads+stores, movnt stores, vlddqu, vpinsrw/vpextrw, vmaskmovdqu ====
  h k b 00001 0 vvvv y 01 0x12 @addr {$rexb=1-$b;$rexx=1-$k} => "vmovlpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x12 @addr => "vmovlpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0x16 @addr {$rexb=1-$b;$rexx=1-$k} => "vmovhpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x16 @addr => "vmovhpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 1111 y 00 0x13 @addr {$rexb=1-$b;$rexx=1-$k} => "vmovlps " $addr "," vreg[16*$y+8*$h+$g] ;
  h 1111 y 00 0x13 @addr => "vmovlps " $addr "," vreg[16*$y+8*$h+$g] ;
  h k b 00001 0 1111 y 00 0x17 @addr {$rexb=1-$b;$rexx=1-$k} => "vmovhps " $addr "," vreg[16*$y+8*$h+$g] ;
  h 1111 y 00 0x17 @addr => "vmovhps " $addr "," vreg[16*$y+8*$h+$g] ;
  h k b 00001 0 1111 y 01 0x13 @addr {$rexb=1-$b;$rexx=1-$k} => "vmovlpd " $addr "," vreg[16*$y+8*$h+$g] ;
  h 1111 y 01 0x13 @addr => "vmovlpd " $addr "," vreg[16*$y+8*$h+$g] ;
  h k b 00001 0 1111 y 01 0x17 @addr {$rexb=1-$b;$rexx=1-$k} => "vmovhpd " $addr "," vreg[16*$y+8*$h+$g] ;
  h 1111 y 01 0x17 @addr => "vmovhpd " $addr "," vreg[16*$y+8*$h+$g] ;
  h k b 00001 0 1111 y 00 0x2b @addr {$rexb=1-$b;$rexx=1-$k} => "vmovntps " $addr "," vreg[16*$y+8*$h+$g] ;
  h 1111 y 00 0x2b @addr => "vmovntps " $addr "," vreg[16*$y+8*$h+$g] ;
  h k b 00001 0 1111 y 01 0x2b @addr {$rexb=1-$b;$rexx=1-$k} => "vmovntpd " $addr "," vreg[16*$y+8*$h+$g] ;
  h 1111 y 01 0x2b @addr => "vmovntpd " $addr "," vreg[16*$y+8*$h+$g] ;
  h k b 00001 0 1111 y 01 0xe7 @addr {$rexb=1-$b;$rexx=1-$k} => "vmovntdq " $addr "," vreg[16*$y+8*$h+$g] ;
  h 1111 y 01 0xe7 @addr => "vmovntdq " $addr "," vreg[16*$y+8*$h+$g] ;
  h k b 00001 0 1111 y 11 0xf0 @addr {$rexb=1-$b;$rexx=1-$k} => "vlddqu " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y 11 0xf0 @addr => "vlddqu " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00001 0 vvvv y 01 0xc4 11 ggg rrr @imm8 => "vpinsrw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," greg[$r] "," hex($imm8) ;
  h vvvv y 01 0xc4 11 ggg rrr @imm8 => "vpinsrw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," greg[$r] "," hex($imm8) ;
  h k b 00001 0 vvvv y 01 0xc4 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vpinsrw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h vvvv y 01 0xc4 @addr @imm8 => "vpinsrw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h k b 00001 0 1111 y 01 0xc5 11 ggg rrr @imm8 => "vpextrw " greg[$g] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h 1111 y 01 0xc5 11 ggg rrr @imm8 => "vpextrw " greg[$g] "," vreg[16*$y+8+$r] "," hex($imm8) ;
  h k b 00001 w 1111 y 01 0xf7 11 ggg rrr => "vmaskmovdqu " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h 1111 y 01 0xf7 11 ggg rrr => "vmaskmovdqu " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] ;
  # ==== VEX map2 66 width-changers (vpmovsx/zx, vcvtph2ps, vpbroadcast*), C4-only ====
  h k b 00010 0 1111 y 01 0x13 11 ggg rrr => "vcvtph2ps " vreg[16*$y+8*$h+$g] "," vreg[8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x13 @addr {$rexb=1-$b;$rexx=1-$k} => "vcvtph2ps " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0x20 11 ggg rrr => "vpmovsxbw " vreg[16*$y+8*$h+$g] "," vreg[8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x20 @addr {$rexb=1-$b;$rexx=1-$k} => "vpmovsxbw " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0x21 11 ggg rrr => "vpmovsxbd " vreg[16*$y+8*$h+$g] "," vreg[8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x21 @addr {$rexb=1-$b;$rexx=1-$k} => "vpmovsxbd " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0x22 11 ggg rrr => "vpmovsxbq " vreg[16*$y+8*$h+$g] "," vreg[8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x22 @addr {$rexb=1-$b;$rexx=1-$k} => "vpmovsxbq " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0x23 11 ggg rrr => "vpmovsxwd " vreg[16*$y+8*$h+$g] "," vreg[8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x23 @addr {$rexb=1-$b;$rexx=1-$k} => "vpmovsxwd " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0x24 11 ggg rrr => "vpmovsxwq " vreg[16*$y+8*$h+$g] "," vreg[8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x24 @addr {$rexb=1-$b;$rexx=1-$k} => "vpmovsxwq " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0x25 11 ggg rrr => "vpmovsxdq " vreg[16*$y+8*$h+$g] "," vreg[8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x25 @addr {$rexb=1-$b;$rexx=1-$k} => "vpmovsxdq " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0x30 11 ggg rrr => "vpmovzxbw " vreg[16*$y+8*$h+$g] "," vreg[8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x30 @addr {$rexb=1-$b;$rexx=1-$k} => "vpmovzxbw " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0x31 11 ggg rrr => "vpmovzxbd " vreg[16*$y+8*$h+$g] "," vreg[8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x31 @addr {$rexb=1-$b;$rexx=1-$k} => "vpmovzxbd " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0x32 11 ggg rrr => "vpmovzxbq " vreg[16*$y+8*$h+$g] "," vreg[8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x32 @addr {$rexb=1-$b;$rexx=1-$k} => "vpmovzxbq " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0x33 11 ggg rrr => "vpmovzxwd " vreg[16*$y+8*$h+$g] "," vreg[8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x33 @addr {$rexb=1-$b;$rexx=1-$k} => "vpmovzxwd " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0x34 11 ggg rrr => "vpmovzxwq " vreg[16*$y+8*$h+$g] "," vreg[8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x34 @addr {$rexb=1-$b;$rexx=1-$k} => "vpmovzxwq " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0x35 11 ggg rrr => "vpmovzxdq " vreg[16*$y+8*$h+$g] "," vreg[8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x35 @addr {$rexb=1-$b;$rexx=1-$k} => "vpmovzxdq " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0x58 11 ggg rrr => "vpbroadcastd " vreg[16*$y+8*$h+$g] "," vreg[8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x58 @addr {$rexb=1-$b;$rexx=1-$k} => "vpbroadcastd " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0x59 11 ggg rrr => "vpbroadcastq " vreg[16*$y+8*$h+$g] "," vreg[8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x59 @addr {$rexb=1-$b;$rexx=1-$k} => "vpbroadcastq " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0x78 11 ggg rrr => "vpbroadcastb " vreg[16*$y+8*$h+$g] "," vreg[8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x78 @addr {$rexb=1-$b;$rexx=1-$k} => "vpbroadcastb " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0x79 11 ggg rrr => "vpbroadcastw " vreg[16*$y+8*$h+$g] "," vreg[8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x79 @addr {$rexb=1-$b;$rexx=1-$k} => "vpbroadcastw " vreg[16*$y+8*$h+$g] "," $addr ;
  # ==== VEX map2 BMI1/BMI2 (GPR): andn/bextr/bzhi/shlx/sarx/shrx/pdep/pext/mulx (W0=r32,W1=r64) ====
  h k b 00010 0 vvvv y 00 0xf2 11 ggg rrr => "andn " greg[$g] "," greg[$v] "," greg[$r] ;
  h k b 00010 0 vvvv y 00 0xf2 @addr {$rexb=1-$b;$rexx=1-$k} => "andn " greg[$g] "," greg[$v] "," $addr ;
  h k b 00010 1 vvvv y 00 0xf2 11 ggg rrr => "andn " greg[32+$g] "," greg[32+$v] "," greg[32+$r] ;
  h k b 00010 1 vvvv y 00 0xf2 @addr {$rexb=1-$b;$rexx=1-$k} => "andn " greg[32+$g] "," greg[32+$v] "," $addr ;
  h k b 00010 0 vvvv y 00 0xf5 11 ggg rrr => "bzhi " greg[$g] "," greg[$r] "," greg[$v] ;
  h k b 00010 0 vvvv y 00 0xf5 @addr {$rexb=1-$b;$rexx=1-$k} => "bzhi " greg[$g] "," $addr "," greg[$v] ;
  h k b 00010 1 vvvv y 00 0xf5 11 ggg rrr => "bzhi " greg[32+$g] "," greg[32+$r] "," greg[32+$v] ;
  h k b 00010 1 vvvv y 00 0xf5 @addr {$rexb=1-$b;$rexx=1-$k} => "bzhi " greg[32+$g] "," $addr "," greg[32+$v] ;
  h k b 00010 0 vvvv y 10 0xf5 11 ggg rrr => "pext " greg[$g] "," greg[$v] "," greg[$r] ;
  h k b 00010 0 vvvv y 10 0xf5 @addr {$rexb=1-$b;$rexx=1-$k} => "pext " greg[$g] "," greg[$v] "," $addr ;
  h k b 00010 1 vvvv y 10 0xf5 11 ggg rrr => "pext " greg[32+$g] "," greg[32+$v] "," greg[32+$r] ;
  h k b 00010 1 vvvv y 10 0xf5 @addr {$rexb=1-$b;$rexx=1-$k} => "pext " greg[32+$g] "," greg[32+$v] "," $addr ;
  h k b 00010 0 vvvv y 11 0xf5 11 ggg rrr => "pdep " greg[$g] "," greg[$v] "," greg[$r] ;
  h k b 00010 0 vvvv y 11 0xf5 @addr {$rexb=1-$b;$rexx=1-$k} => "pdep " greg[$g] "," greg[$v] "," $addr ;
  h k b 00010 1 vvvv y 11 0xf5 11 ggg rrr => "pdep " greg[32+$g] "," greg[32+$v] "," greg[32+$r] ;
  h k b 00010 1 vvvv y 11 0xf5 @addr {$rexb=1-$b;$rexx=1-$k} => "pdep " greg[32+$g] "," greg[32+$v] "," $addr ;
  h k b 00010 0 vvvv y 11 0xf6 11 ggg rrr => "mulx " greg[$g] "," greg[$v] "," greg[$r] ;
  h k b 00010 0 vvvv y 11 0xf6 @addr {$rexb=1-$b;$rexx=1-$k} => "mulx " greg[$g] "," greg[$v] "," $addr ;
  h k b 00010 1 vvvv y 11 0xf6 11 ggg rrr => "mulx " greg[32+$g] "," greg[32+$v] "," greg[32+$r] ;
  h k b 00010 1 vvvv y 11 0xf6 @addr {$rexb=1-$b;$rexx=1-$k} => "mulx " greg[32+$g] "," greg[32+$v] "," $addr ;
  h k b 00010 0 vvvv y 00 0xf7 11 ggg rrr => "bextr " greg[$g] "," greg[$r] "," greg[$v] ;
  h k b 00010 0 vvvv y 00 0xf7 @addr {$rexb=1-$b;$rexx=1-$k} => "bextr " greg[$g] "," $addr "," greg[$v] ;
  h k b 00010 1 vvvv y 00 0xf7 11 ggg rrr => "bextr " greg[32+$g] "," greg[32+$r] "," greg[32+$v] ;
  h k b 00010 1 vvvv y 00 0xf7 @addr {$rexb=1-$b;$rexx=1-$k} => "bextr " greg[32+$g] "," $addr "," greg[32+$v] ;
  # VEX map 7 (APX USER_MSR immediate forms): urdmsr r64,imm32 (F2 /0) ; uwrmsr imm32,r64 (F3 /0).
  # r64 is the ModR/M r/m (reg field = /0); imm32 follows. Register-only (no memory form).
  h k b 00111 0 1111 y 11 0xf8 11 ggg rrr @imm32 => "urdmsr " greg[32+$r] "," hex($imm32) ;
  h k b 00111 0 1111 y 10 0xf8 11 ggg rrr @imm32 => "uwrmsr " hex($imm32) "," greg[32+$r] ;
  h k b 00010 0 vvvv y 01 0xf7 11 ggg rrr => "shlx " greg[$g] "," greg[$r] "," greg[$v] ;
  h k b 00010 0 vvvv y 01 0xf7 @addr {$rexb=1-$b;$rexx=1-$k} => "shlx " greg[$g] "," $addr "," greg[$v] ;
  h k b 00010 1 vvvv y 01 0xf7 11 ggg rrr => "shlx " greg[32+$g] "," greg[32+$r] "," greg[32+$v] ;
  h k b 00010 1 vvvv y 01 0xf7 @addr {$rexb=1-$b;$rexx=1-$k} => "shlx " greg[32+$g] "," $addr "," greg[32+$v] ;
  h k b 00010 0 vvvv y 10 0xf7 11 ggg rrr => "sarx " greg[$g] "," greg[$r] "," greg[$v] ;
  h k b 00010 0 vvvv y 10 0xf7 @addr {$rexb=1-$b;$rexx=1-$k} => "sarx " greg[$g] "," $addr "," greg[$v] ;
  h k b 00010 1 vvvv y 10 0xf7 11 ggg rrr => "sarx " greg[32+$g] "," greg[32+$r] "," greg[32+$v] ;
  h k b 00010 1 vvvv y 10 0xf7 @addr {$rexb=1-$b;$rexx=1-$k} => "sarx " greg[32+$g] "," $addr "," greg[32+$v] ;
  h k b 00010 0 vvvv y 11 0xf7 11 ggg rrr => "shrx " greg[$g] "," greg[$r] "," greg[$v] ;
  h k b 00010 0 vvvv y 11 0xf7 @addr {$rexb=1-$b;$rexx=1-$k} => "shrx " greg[$g] "," $addr "," greg[$v] ;
  h k b 00010 1 vvvv y 11 0xf7 11 ggg rrr => "shrx " greg[32+$g] "," greg[32+$r] "," greg[32+$v] ;
  h k b 00010 1 vvvv y 11 0xf7 @addr {$rexb=1-$b;$rexx=1-$k} => "shrx " greg[32+$g] "," $addr "," greg[32+$v] ;
  # ==== VEX map3 (0F3A) 66 imm8 2-src / 2-op (blend/round/dp/mpsadbw/pclmul/pcmpstr/perm), C4-only ====
  h k b 00011 0 vvvv y 01 0x02 11 ggg rrr @imm8 => "vpblendd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x02 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vpblendd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x0a 11 ggg rrr @imm8 => "vroundss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x0a @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vroundss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x0b 11 ggg rrr @imm8 => "vroundsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x0b @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vroundsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x0c 11 ggg rrr @imm8 => "vblendps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x0c @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vblendps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x0d 11 ggg rrr @imm8 => "vblendpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x0d @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vblendpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h k b 00011 w vvvv y 01 0x0e 11 ggg rrr @imm8 => "vpblendw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 w vvvv y 01 0x0e @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vpblendw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h k b 00011 w vvvv y 01 0x0f 11 ggg rrr @imm8 => "vpalignr " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 w vvvv y 01 0x0f @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vpalignr " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x21 11 ggg rrr @imm8 => "vinsertps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x21 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vinsertps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x40 11 ggg rrr @imm8 => "vdpps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x40 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vdpps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x41 11 ggg rrr @imm8 => "vdppd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x41 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vdppd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x42 11 ggg rrr @imm8 => "vmpsadbw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x42 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vmpsadbw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x44 11 ggg rrr @imm8 => "vpclmulqdq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x44 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vpclmulqdq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0xde 11 ggg rrr @imm8 => "vsm3rnds2 " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0xde @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vsm3rnds2 " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x04 11 ggg rrr @imm8 => "vpermilps " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x04 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vpermilps " vreg[16*$y+8*$h+$g] "," $addr "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x05 11 ggg rrr @imm8 => "vpermilpd " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x05 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vpermilpd " vreg[16*$y+8*$h+$g] "," $addr "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x08 11 ggg rrr @imm8 => "vroundps " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x08 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vroundps " vreg[16*$y+8*$h+$g] "," $addr "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x09 11 ggg rrr @imm8 => "vroundpd " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x09 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vroundpd " vreg[16*$y+8*$h+$g] "," $addr "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x60 11 ggg rrr @imm8 => "vpcmpestrm " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x60 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vpcmpestrm " vreg[16*$y+8*$h+$g] "," $addr "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x61 11 ggg rrr @imm8 => "vpcmpestri " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x61 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vpcmpestri " vreg[16*$y+8*$h+$g] "," $addr "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x62 11 ggg rrr @imm8 => "vpcmpistrm " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x62 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vpcmpistrm " vreg[16*$y+8*$h+$g] "," $addr "," hex($imm8) ;
  h k b 00011 w 1111 y 01 0x63 11 ggg rrr @imm8 => "vpcmpistri " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 w 1111 y 01 0x63 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vpcmpistri " vreg[16*$y+8*$h+$g] "," $addr "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0xdf 11 ggg rrr @imm8 => "vaeskeygenassist " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0xdf @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vaeskeygenassist " vreg[16*$y+8*$h+$g] "," $addr "," hex($imm8) ;
  # ==== VEX map3 is4 4-operand: FMA4 (W0/W1 swap r/m<->is4) + vblendv* (W0) ====
  h k b 00011 0 vvvv y 01 0x5c 11 ggg rrr qqqq 0000 => "vfmaddsubps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x5c @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmaddsubps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  h k b 00011 1 vvvv y 01 0x5c 11 ggg rrr qqqq 0000 => "vfmaddsubps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," vreg[16*$y+8*$b+$r] ;
  h k b 00011 1 vvvv y 01 0x5c @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmaddsubps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," $addr ;
  h k b 00011 0 vvvv y 01 0x5d 11 ggg rrr qqqq 0000 => "vfmaddsubpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x5d @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmaddsubpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  h k b 00011 1 vvvv y 01 0x5d 11 ggg rrr qqqq 0000 => "vfmaddsubpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," vreg[16*$y+8*$b+$r] ;
  h k b 00011 1 vvvv y 01 0x5d @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmaddsubpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," $addr ;
  h k b 00011 0 vvvv y 01 0x5e 11 ggg rrr qqqq 0000 => "vfmsubaddps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x5e @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmsubaddps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  h k b 00011 1 vvvv y 01 0x5e 11 ggg rrr qqqq 0000 => "vfmsubaddps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," vreg[16*$y+8*$b+$r] ;
  h k b 00011 1 vvvv y 01 0x5e @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmsubaddps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," $addr ;
  h k b 00011 0 vvvv y 01 0x5f 11 ggg rrr qqqq 0000 => "vfmsubaddpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x5f @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmsubaddpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  h k b 00011 1 vvvv y 01 0x5f 11 ggg rrr qqqq 0000 => "vfmsubaddpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," vreg[16*$y+8*$b+$r] ;
  h k b 00011 1 vvvv y 01 0x5f @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmsubaddpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," $addr ;
  h k b 00011 0 vvvv y 01 0x68 11 ggg rrr qqqq 0000 => "vfmaddps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x68 @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmaddps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  h k b 00011 1 vvvv y 01 0x68 11 ggg rrr qqqq 0000 => "vfmaddps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," vreg[16*$y+8*$b+$r] ;
  h k b 00011 1 vvvv y 01 0x68 @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmaddps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," $addr ;
  h k b 00011 0 vvvv y 01 0x69 11 ggg rrr qqqq 0000 => "vfmaddpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x69 @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmaddpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  h k b 00011 1 vvvv y 01 0x69 11 ggg rrr qqqq 0000 => "vfmaddpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," vreg[16*$y+8*$b+$r] ;
  h k b 00011 1 vvvv y 01 0x69 @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmaddpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," $addr ;
  h k b 00011 0 vvvv y 01 0x6a 11 ggg rrr qqqq 0000 => "vfmaddss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x6a @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmaddss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  h k b 00011 1 vvvv y 01 0x6a 11 ggg rrr qqqq 0000 => "vfmaddss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," vreg[16*$y+8*$b+$r] ;
  h k b 00011 1 vvvv y 01 0x6a @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmaddss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," $addr ;
  h k b 00011 0 vvvv y 01 0x6b 11 ggg rrr qqqq 0000 => "vfmaddsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x6b @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmaddsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  h k b 00011 1 vvvv y 01 0x6b 11 ggg rrr qqqq 0000 => "vfmaddsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," vreg[16*$y+8*$b+$r] ;
  h k b 00011 1 vvvv y 01 0x6b @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmaddsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," $addr ;
  h k b 00011 0 vvvv y 01 0x6c 11 ggg rrr qqqq 0000 => "vfmsubps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x6c @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmsubps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  h k b 00011 1 vvvv y 01 0x6c 11 ggg rrr qqqq 0000 => "vfmsubps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," vreg[16*$y+8*$b+$r] ;
  h k b 00011 1 vvvv y 01 0x6c @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmsubps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," $addr ;
  h k b 00011 0 vvvv y 01 0x6d 11 ggg rrr qqqq 0000 => "vfmsubpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x6d @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmsubpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  h k b 00011 1 vvvv y 01 0x6d 11 ggg rrr qqqq 0000 => "vfmsubpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," vreg[16*$y+8*$b+$r] ;
  h k b 00011 1 vvvv y 01 0x6d @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmsubpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," $addr ;
  h k b 00011 0 vvvv y 01 0x6e 11 ggg rrr qqqq 0000 => "vfmsubss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x6e @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmsubss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  h k b 00011 1 vvvv y 01 0x6e 11 ggg rrr qqqq 0000 => "vfmsubss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," vreg[16*$y+8*$b+$r] ;
  h k b 00011 1 vvvv y 01 0x6e @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmsubss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," $addr ;
  h k b 00011 0 vvvv y 01 0x6f 11 ggg rrr qqqq 0000 => "vfmsubsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x6f @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmsubsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  h k b 00011 1 vvvv y 01 0x6f 11 ggg rrr qqqq 0000 => "vfmsubsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," vreg[16*$y+8*$b+$r] ;
  h k b 00011 1 vvvv y 01 0x6f @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfmsubsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," $addr ;
  h k b 00011 0 vvvv y 01 0x78 11 ggg rrr qqqq 0000 => "vfnmaddps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x78 @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfnmaddps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  h k b 00011 1 vvvv y 01 0x78 11 ggg rrr qqqq 0000 => "vfnmaddps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," vreg[16*$y+8*$b+$r] ;
  h k b 00011 1 vvvv y 01 0x78 @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfnmaddps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," $addr ;
  h k b 00011 0 vvvv y 01 0x79 11 ggg rrr qqqq 0000 => "vfnmaddpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x79 @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfnmaddpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  h k b 00011 1 vvvv y 01 0x79 11 ggg rrr qqqq 0000 => "vfnmaddpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," vreg[16*$y+8*$b+$r] ;
  h k b 00011 1 vvvv y 01 0x79 @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfnmaddpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," $addr ;
  h k b 00011 0 vvvv y 01 0x7a 11 ggg rrr qqqq 0000 => "vfnmaddss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x7a @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfnmaddss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  h k b 00011 1 vvvv y 01 0x7a 11 ggg rrr qqqq 0000 => "vfnmaddss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," vreg[16*$y+8*$b+$r] ;
  h k b 00011 1 vvvv y 01 0x7a @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfnmaddss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," $addr ;
  h k b 00011 0 vvvv y 01 0x7b 11 ggg rrr qqqq 0000 => "vfnmaddsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x7b @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfnmaddsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  h k b 00011 1 vvvv y 01 0x7b 11 ggg rrr qqqq 0000 => "vfnmaddsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," vreg[16*$y+8*$b+$r] ;
  h k b 00011 1 vvvv y 01 0x7b @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfnmaddsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," $addr ;
  h k b 00011 0 vvvv y 01 0x7c 11 ggg rrr qqqq 0000 => "vfnmsubps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x7c @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfnmsubps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  h k b 00011 1 vvvv y 01 0x7c 11 ggg rrr qqqq 0000 => "vfnmsubps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," vreg[16*$y+8*$b+$r] ;
  h k b 00011 1 vvvv y 01 0x7c @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfnmsubps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," $addr ;
  h k b 00011 0 vvvv y 01 0x7d 11 ggg rrr qqqq 0000 => "vfnmsubpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x7d @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfnmsubpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  h k b 00011 1 vvvv y 01 0x7d 11 ggg rrr qqqq 0000 => "vfnmsubpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," vreg[16*$y+8*$b+$r] ;
  h k b 00011 1 vvvv y 01 0x7d @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfnmsubpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," $addr ;
  h k b 00011 0 vvvv y 01 0x7e 11 ggg rrr qqqq 0000 => "vfnmsubss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x7e @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfnmsubss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  h k b 00011 1 vvvv y 01 0x7e 11 ggg rrr qqqq 0000 => "vfnmsubss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," vreg[16*$y+8*$b+$r] ;
  h k b 00011 1 vvvv y 01 0x7e @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfnmsubss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," $addr ;
  h k b 00011 0 vvvv y 01 0x7f 11 ggg rrr qqqq 0000 => "vfnmsubsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x7f @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfnmsubsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  h k b 00011 1 vvvv y 01 0x7f 11 ggg rrr qqqq 0000 => "vfnmsubsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," vreg[16*$y+8*$b+$r] ;
  h k b 00011 1 vvvv y 01 0x7f @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vfnmsubsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," $addr ;
  h k b 00011 0 vvvv y 01 0x4a 11 ggg rrr qqqq 0000 => "vblendvps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x4a @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vblendvps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x4b 11 ggg rrr qqqq 0000 => "vblendvpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x4b @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vblendvpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x4c 11 ggg rrr qqqq 0000 => "vpblendvb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 00011 0 vvvv y 01 0x4c @addr {$rexb=1-$b;$rexx=1-$k} qqqq 0000 => "vpblendvb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," vregd[16*$y+$q] ;
  # ==== VEX map3 GPR extract/insert (vpextrb/w/d/q, vextractps, vpinsrb/d/q) ====
  h k b 00011 0 1111 y 01 0x14 11 ggg rrr @imm8 => "vpextrb " greg[$r] "," vreg[16*$y+8*$h+$g] "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x14 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vpextrb " $addr "," vreg[16*$y+8*$h+$g] "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x15 11 ggg rrr @imm8 => "vpextrw " greg[$r] "," vreg[16*$y+8*$h+$g] "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x15 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vpextrw " $addr "," vreg[16*$y+8*$h+$g] "," hex($imm8) ;
  h k b 00011 w 1111 y 01 0x17 11 ggg rrr @imm8 => "vextractps " greg[$r] "," vreg[16*$y+8*$h+$g] "," hex($imm8) ;
  h k b 00011 w 1111 y 01 0x17 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vextractps " $addr "," vreg[16*$y+8*$h+$g] "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x16 11 ggg rrr @imm8 => "vpextrd " greg[$r] "," vreg[16*$y+8*$h+$g] "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x16 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vpextrd " $addr "," vreg[16*$y+8*$h+$g] "," hex($imm8) ;
  h k b 00011 1 1111 y 01 0x16 11 ggg rrr @imm8 => "vpextrq " greg[32+$r] "," vreg[16*$y+8*$h+$g] "," hex($imm8) ;
  h k b 00011 1 1111 y 01 0x16 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vpextrq " $addr "," vreg[16*$y+8*$h+$g] "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x20 11 ggg rrr @imm8 => "vpinsrb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," greg[$r] "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x20 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vpinsrb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x22 11 ggg rrr @imm8 => "vpinsrd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," greg[$r] "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x22 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vpinsrd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h k b 00011 1 vvvv y 01 0x22 11 ggg rrr @imm8 => "vpinsrq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," greg[32+$r] "," hex($imm8) ;
  h k b 00011 1 vvvv y 01 0x22 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vpinsrq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  # ==== VEX tail: vmovss/vmovsd C4 (W1), vperm2f128, vpermd, vpermps ====
  h k b 00001 w vvvv y 10 0x10 11 ggg rrr => "vmovss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 w vvvv y 10 0x11 11 ggg rrr => "vmovss " vreg[16*$y+8*$b+$r] "," vvv[16*$y+$v] "," vreg[16*$y+8*$h+$g] ;
  h k b 00001 w 1111 y 10 0x10 @addr {$rexb=1-$b;$rexx=1-$k} => "vmovss " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00001 w 1111 y 10 0x11 @addr {$rexb=1-$b;$rexx=1-$k} => "vmovss " $addr "," vreg[16*$y+8*$h+$g] ;
  h k b 00001 w vvvv y 11 0x10 11 ggg rrr => "vmovsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 w vvvv y 11 0x11 11 ggg rrr => "vmovsd " vreg[16*$y+8*$b+$r] "," vvv[16*$y+$v] "," vreg[16*$y+8*$h+$g] ;
  h k b 00001 w 1111 y 11 0x10 @addr {$rexb=1-$b;$rexx=1-$k} => "vmovsd " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00001 w 1111 y 11 0x11 @addr {$rexb=1-$b;$rexx=1-$k} => "vmovsd " $addr "," vreg[16*$y+8*$h+$g] ;
  h k b 00011 0 vvvv y 01 0x06 11 ggg rrr @imm8 => "vperm2f128 " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x06 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vperm2f128 " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h k b 00010 0 vvvv y 01 0x36 11 ggg rrr => "vpermd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x36 @addr {$rexb=1-$b;$rexx=1-$k} => "vpermd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x16 11 ggg rrr => "vpermps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x16 @addr {$rexb=1-$b;$rexx=1-$k} => "vpermps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  # vsha512rnds2 (VEX.256.F2.0F38.W0 CB): dst/vvvv ymm, src xmm
  h k b 00010 0 vvvv y 11 0xcb 11 ggg rrr => "vsha512rnds2 " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[8*$b+$r] ;
  h k b 00010 0 vvvv y 11 0xcb @addr {$rexb=1-$b;$rexx=1-$k} => "vsha512rnds2 " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  # ==== VEX map2 NP/F3/F2 tail: AVX-VNNI-INT, NE-CONVERT bf16/fp16, SM3/SM4, AMX ====
  h k b 00010 0 vvvv y 00 0x50 11 ggg rrr => "vpdpbuud " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 00 0x50 @addr {$rexb=1-$b;$rexx=1-$k} => "vpdpbuud " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 00 0x51 11 ggg rrr => "vpdpbuuds " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 00 0x51 @addr {$rexb=1-$b;$rexx=1-$k} => "vpdpbuuds " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 00 0xd2 11 ggg rrr => "vpdpwuud " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 00 0xd2 @addr {$rexb=1-$b;$rexx=1-$k} => "vpdpwuud " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 00 0xd3 11 ggg rrr => "vpdpwuuds " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 00 0xd3 @addr {$rexb=1-$b;$rexx=1-$k} => "vpdpwuuds " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 00 0xda 11 ggg rrr => "vsm3msg1 " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 00 0xda @addr {$rexb=1-$b;$rexx=1-$k} => "vsm3msg1 " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 10 0x50 11 ggg rrr => "vpdpbsud " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 10 0x50 @addr {$rexb=1-$b;$rexx=1-$k} => "vpdpbsud " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 10 0x51 11 ggg rrr => "vpdpbsuds " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 10 0x51 @addr {$rexb=1-$b;$rexx=1-$k} => "vpdpbsuds " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 10 0xd2 11 ggg rrr => "vpdpwsud " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 10 0xd2 @addr {$rexb=1-$b;$rexx=1-$k} => "vpdpwsud " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 10 0xd3 11 ggg rrr => "vpdpwsuds " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 10 0xd3 @addr {$rexb=1-$b;$rexx=1-$k} => "vpdpwsuds " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 10 0xda 11 ggg rrr => "vsm4key4 " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 10 0xda @addr {$rexb=1-$b;$rexx=1-$k} => "vsm4key4 " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 11 0x50 11 ggg rrr => "vpdpbssd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 11 0x50 @addr {$rexb=1-$b;$rexx=1-$k} => "vpdpbssd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 11 0x51 11 ggg rrr => "vpdpbssds " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 11 0x51 @addr {$rexb=1-$b;$rexx=1-$k} => "vpdpbssds " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 11 0xda 11 ggg rrr => "vsm4rnds4 " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 11 0xda @addr {$rexb=1-$b;$rexx=1-$k} => "vsm4rnds4 " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 1111 y 00 0xb0 11 ggg rrr => "vcvtneoph2ps " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 1111 y 00 0xb0 @addr {$rexb=1-$b;$rexx=1-$k} => "vcvtneoph2ps " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 10 0xb0 11 ggg rrr => "vcvtneebf162ps " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 1111 y 10 0xb0 @addr {$rexb=1-$b;$rexx=1-$k} => "vcvtneebf162ps " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 11 0xb0 11 ggg rrr => "vcvtneobf162ps " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 1111 y 11 0xb0 @addr {$rexb=1-$b;$rexx=1-$k} => "vcvtneobf162ps " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 10 0xb1 @addr {$rexb=1-$b;$rexx=1-$k} => "vbcstnebf162ps " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 00 0x49 @addr {$rexb=1-$b;$rexx=1-$k} => "ldtilecfg " $addr ;
  # AVX-VNNI (66 0F38): vpdpbusd/vpdpbusds/vpdpwssd/vpdpwssds
  h k b 00010 0 vvvv y 01 0x50 11 ggg rrr => "vpdpbusd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x50 @addr {$rexb=1-$b;$rexx=1-$k} => "vpdpbusd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x51 11 ggg rrr => "vpdpbusds " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x51 @addr {$rexb=1-$b;$rexx=1-$k} => "vpdpbusds " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x52 11 ggg rrr => "vpdpwssd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x52 @addr {$rexb=1-$b;$rexx=1-$k} => "vpdpwssd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x53 11 ggg rrr => "vpdpwssds " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x53 @addr {$rexb=1-$b;$rexx=1-$k} => "vpdpwssds " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  # 0F 12/16 NP: mem = vmovlps/vmovhps (load, listed first = descriptor mnem); reg-direct
  # form = vmovhlps/vmovlhps, swapped in vex_finalize (VEX descriptor is per-opcode).
  h k b 00001 w vvvv y 00 0x12 @addr {$rexb=1-$b;$rexx=1-$k} => "vmovlps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 w vvvv y 00 0x12 11 ggg rrr => "vmovhlps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h vvvv y 00 0x12 @addr => "vmovlps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 00 0x12 11 ggg rrr => "vmovhlps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h k b 00001 w vvvv y 00 0x16 @addr {$rexb=1-$b;$rexx=1-$k} => "vmovhps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 w vvvv y 00 0x16 11 ggg rrr => "vmovlhps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h vvvv y 00 0x16 @addr => "vmovhps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 00 0x16 11 ggg rrr => "vmovlhps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  # ==== VEX map2 (0F38) 66 same-width AVX2/AVX-VNNI/GFNI/AES (2-src / 2-op), C4-only ====
  h k b 00010 0 vvvv y 01 0x01 11 ggg rrr => "vphaddw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x01 @addr {$rexb=1-$b;$rexx=1-$k} => "vphaddw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x02 11 ggg rrr => "vphaddd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x02 @addr {$rexb=1-$b;$rexx=1-$k} => "vphaddd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x03 11 ggg rrr => "vphaddsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x03 @addr {$rexb=1-$b;$rexx=1-$k} => "vphaddsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x04 11 ggg rrr => "vpmaddubsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x04 @addr {$rexb=1-$b;$rexx=1-$k} => "vpmaddubsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x05 11 ggg rrr => "vphsubw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x05 @addr {$rexb=1-$b;$rexx=1-$k} => "vphsubw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 w vvvv y 01 0x06 11 ggg rrr => "vphsubd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 w vvvv y 01 0x06 @addr {$rexb=1-$b;$rexx=1-$k} => "vphsubd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x07 11 ggg rrr => "vphsubsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x07 @addr {$rexb=1-$b;$rexx=1-$k} => "vphsubsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x08 11 ggg rrr => "vpsignb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x08 @addr {$rexb=1-$b;$rexx=1-$k} => "vpsignb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x09 11 ggg rrr => "vpsignw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x09 @addr {$rexb=1-$b;$rexx=1-$k} => "vpsignw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x0a 11 ggg rrr => "vpsignd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x0a @addr {$rexb=1-$b;$rexx=1-$k} => "vpsignd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x0b 11 ggg rrr => "vpmulhrsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x0b @addr {$rexb=1-$b;$rexx=1-$k} => "vpmulhrsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x0c 11 ggg rrr => "vpermilps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x0c @addr {$rexb=1-$b;$rexx=1-$k} => "vpermilps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x0d 11 ggg rrr => "vpermilpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x0d @addr {$rexb=1-$b;$rexx=1-$k} => "vpermilpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x28 11 ggg rrr => "vpmuldq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x28 @addr {$rexb=1-$b;$rexx=1-$k} => "vpmuldq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x29 11 ggg rrr => "vpcmpeqq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x29 @addr {$rexb=1-$b;$rexx=1-$k} => "vpcmpeqq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x2b 11 ggg rrr => "vpackusdw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x2b @addr {$rexb=1-$b;$rexx=1-$k} => "vpackusdw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x37 11 ggg rrr => "vpcmpgtq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x37 @addr {$rexb=1-$b;$rexx=1-$k} => "vpcmpgtq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x38 11 ggg rrr => "vpminsb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x38 @addr {$rexb=1-$b;$rexx=1-$k} => "vpminsb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x39 11 ggg rrr => "vpminsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x39 @addr {$rexb=1-$b;$rexx=1-$k} => "vpminsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x3a 11 ggg rrr => "vpminuw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x3a @addr {$rexb=1-$b;$rexx=1-$k} => "vpminuw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x3b 11 ggg rrr => "vpminud " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x3b @addr {$rexb=1-$b;$rexx=1-$k} => "vpminud " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x3c 11 ggg rrr => "vpmaxsb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x3c @addr {$rexb=1-$b;$rexx=1-$k} => "vpmaxsb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x3d 11 ggg rrr => "vpmaxsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x3d @addr {$rexb=1-$b;$rexx=1-$k} => "vpmaxsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 w vvvv y 01 0x3e 11 ggg rrr => "vpmaxuw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 w vvvv y 01 0x3e @addr {$rexb=1-$b;$rexx=1-$k} => "vpmaxuw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x3f 11 ggg rrr => "vpmaxud " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x3f @addr {$rexb=1-$b;$rexx=1-$k} => "vpmaxud " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x45 11 ggg rrr => "vpsrlvd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x45 @addr {$rexb=1-$b;$rexx=1-$k} => "vpsrlvd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x46 11 ggg rrr => "vpsravd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x46 @addr {$rexb=1-$b;$rexx=1-$k} => "vpsravd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x47 11 ggg rrr => "vpsllvd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x47 @addr {$rexb=1-$b;$rexx=1-$k} => "vpsllvd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xcf 11 ggg rrr => "vgf2p8mulb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xcf @addr {$rexb=1-$b;$rexx=1-$k} => "vgf2p8mulb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xdc 11 ggg rrr => "vaesenc " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xdc @addr {$rexb=1-$b;$rexx=1-$k} => "vaesenc " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xdd 11 ggg rrr => "vaesenclast " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xdd @addr {$rexb=1-$b;$rexx=1-$k} => "vaesenclast " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xde 11 ggg rrr => "vaesdec " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xde @addr {$rexb=1-$b;$rexx=1-$k} => "vaesdec " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xdf 11 ggg rrr => "vaesdeclast " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xdf @addr {$rexb=1-$b;$rexx=1-$k} => "vaesdeclast " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 1111 y 01 0x0e 11 ggg rrr => "vtestps " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x0e @addr {$rexb=1-$b;$rexx=1-$k} => "vtestps " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0x0f 11 ggg rrr => "vtestpd " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x0f @addr {$rexb=1-$b;$rexx=1-$k} => "vtestpd " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0x17 11 ggg rrr => "vptest " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x17 @addr {$rexb=1-$b;$rexx=1-$k} => "vptest " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0x1c 11 ggg rrr => "vpabsb " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x1c @addr {$rexb=1-$b;$rexx=1-$k} => "vpabsb " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0x1d 11 ggg rrr => "vpabsw " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x1d @addr {$rexb=1-$b;$rexx=1-$k} => "vpabsw " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 w 1111 y 01 0x1e 11 ggg rrr => "vpabsd " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 w 1111 y 01 0x1e @addr {$rexb=1-$b;$rexx=1-$k} => "vpabsd " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0x41 11 ggg rrr => "vphminposuw " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 1111 y 01 0x41 @addr {$rexb=1-$b;$rexx=1-$k} => "vphminposuw " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0xdb 11 ggg rrr => "vaesimc " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 1111 y 01 0xdb @addr {$rexb=1-$b;$rexx=1-$k} => "vaesimc " vreg[16*$y+8*$h+$g] "," $addr ;
  # ==== VEX map1 66 AVX2 integer/FP (2-src) -- VEX forms of EVEX-covered ops ====
  h k b 00001 0 vvvv y 01 0x14 11 ggg rrr => "vunpcklpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0x14 @addr {$rexb=1-$b;$rexx=1-$k} => "vunpcklpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x14 11 ggg rrr => "vunpcklpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0x14 @addr => "vunpcklpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0x15 11 ggg rrr => "vunpckhpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0x15 @addr {$rexb=1-$b;$rexx=1-$k} => "vunpckhpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x15 11 ggg rrr => "vunpckhpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0x15 @addr => "vunpckhpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 w vvvv y 01 0x69 11 ggg rrr => "vpunpckhwd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 w vvvv y 01 0x69 @addr {$rexb=1-$b;$rexx=1-$k} => "vpunpckhwd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x69 11 ggg rrr => "vpunpckhwd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0x69 @addr => "vpunpckhwd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0x6a 11 ggg rrr => "vpunpckhdq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0x6a @addr {$rexb=1-$b;$rexx=1-$k} => "vpunpckhdq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x6a 11 ggg rrr => "vpunpckhdq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0x6a @addr => "vpunpckhdq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0x6b 11 ggg rrr => "vpackssdw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0x6b @addr {$rexb=1-$b;$rexx=1-$k} => "vpackssdw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x6b 11 ggg rrr => "vpackssdw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0x6b @addr => "vpackssdw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0x7c 11 ggg rrr => "vhaddpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0x7c @addr {$rexb=1-$b;$rexx=1-$k} => "vhaddpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x7c 11 ggg rrr => "vhaddpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0x7c @addr => "vhaddpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0x7d 11 ggg rrr => "vhsubpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0x7d @addr {$rexb=1-$b;$rexx=1-$k} => "vhsubpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0x7d 11 ggg rrr => "vhsubpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0x7d @addr => "vhsubpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xd0 11 ggg rrr => "vaddsubpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xd0 @addr {$rexb=1-$b;$rexx=1-$k} => "vaddsubpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xd0 11 ggg rrr => "vaddsubpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xd0 @addr => "vaddsubpd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xd1 11 ggg rrr => "vpsrlw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xd1 @addr {$rexb=1-$b;$rexx=1-$k} => "vpsrlw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xd1 11 ggg rrr => "vpsrlw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[8+$r] ;
  h vvvv y 01 0xd1 @addr => "vpsrlw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xd2 11 ggg rrr => "vpsrld " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xd2 @addr {$rexb=1-$b;$rexx=1-$k} => "vpsrld " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xd2 11 ggg rrr => "vpsrld " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[8+$r] ;
  h vvvv y 01 0xd2 @addr => "vpsrld " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xd3 11 ggg rrr => "vpsrlq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xd3 @addr {$rexb=1-$b;$rexx=1-$k} => "vpsrlq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xd3 11 ggg rrr => "vpsrlq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[8+$r] ;
  h vvvv y 01 0xd3 @addr => "vpsrlq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xd8 11 ggg rrr => "vpsubusb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xd8 @addr {$rexb=1-$b;$rexx=1-$k} => "vpsubusb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xd8 11 ggg rrr => "vpsubusb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xd8 @addr => "vpsubusb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xd9 11 ggg rrr => "vpsubusw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xd9 @addr {$rexb=1-$b;$rexx=1-$k} => "vpsubusw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xd9 11 ggg rrr => "vpsubusw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xd9 @addr => "vpsubusw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xdd 11 ggg rrr => "vpaddusw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xdd @addr {$rexb=1-$b;$rexx=1-$k} => "vpaddusw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xdd 11 ggg rrr => "vpaddusw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xdd @addr => "vpaddusw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xe1 11 ggg rrr => "vpsraw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xe1 @addr {$rexb=1-$b;$rexx=1-$k} => "vpsraw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xe1 11 ggg rrr => "vpsraw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[8+$r] ;
  h vvvv y 01 0xe1 @addr => "vpsraw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xe2 11 ggg rrr => "vpsrad " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xe2 @addr {$rexb=1-$b;$rexx=1-$k} => "vpsrad " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xe2 11 ggg rrr => "vpsrad " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[8+$r] ;
  h vvvv y 01 0xe2 @addr => "vpsrad " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xe4 11 ggg rrr => "vpmulhuw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xe4 @addr {$rexb=1-$b;$rexx=1-$k} => "vpmulhuw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xe4 11 ggg rrr => "vpmulhuw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xe4 @addr => "vpmulhuw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xe8 11 ggg rrr => "vpsubsb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xe8 @addr {$rexb=1-$b;$rexx=1-$k} => "vpsubsb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xe8 11 ggg rrr => "vpsubsb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xe8 @addr => "vpsubsb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 w vvvv y 01 0xe9 11 ggg rrr => "vpsubsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 w vvvv y 01 0xe9 @addr {$rexb=1-$b;$rexx=1-$k} => "vpsubsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xe9 11 ggg rrr => "vpsubsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xe9 @addr => "vpsubsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xea 11 ggg rrr => "vpminsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xea @addr {$rexb=1-$b;$rexx=1-$k} => "vpminsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xea 11 ggg rrr => "vpminsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xea @addr => "vpminsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xec 11 ggg rrr => "vpaddsb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xec @addr {$rexb=1-$b;$rexx=1-$k} => "vpaddsb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xec 11 ggg rrr => "vpaddsb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xec @addr => "vpaddsb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xed 11 ggg rrr => "vpaddsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xed @addr {$rexb=1-$b;$rexx=1-$k} => "vpaddsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xed 11 ggg rrr => "vpaddsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xed @addr => "vpaddsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xee 11 ggg rrr => "vpmaxsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xee @addr {$rexb=1-$b;$rexx=1-$k} => "vpmaxsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xee 11 ggg rrr => "vpmaxsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xee @addr => "vpmaxsw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xf1 11 ggg rrr => "vpsllw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xf1 @addr {$rexb=1-$b;$rexx=1-$k} => "vpsllw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xf1 11 ggg rrr => "vpsllw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[8+$r] ;
  h vvvv y 01 0xf1 @addr => "vpsllw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xf2 11 ggg rrr => "vpslld " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xf2 @addr {$rexb=1-$b;$rexx=1-$k} => "vpslld " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xf2 11 ggg rrr => "vpslld " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[8+$r] ;
  h vvvv y 01 0xf2 @addr => "vpslld " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xf3 11 ggg rrr => "vpsllq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xf3 @addr {$rexb=1-$b;$rexx=1-$k} => "vpsllq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xf3 11 ggg rrr => "vpsllq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[8+$r] ;
  h vvvv y 01 0xf3 @addr => "vpsllq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xf4 11 ggg rrr => "vpmuludq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xf4 @addr {$rexb=1-$b;$rexx=1-$k} => "vpmuludq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xf4 11 ggg rrr => "vpmuludq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xf4 @addr => "vpmuludq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0xf6 11 ggg rrr => "vpsadbw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0xf6 @addr {$rexb=1-$b;$rexx=1-$k} => "vpsadbw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 01 0xf6 11 ggg rrr => "vpsadbw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 01 0xf6 @addr => "vpsadbw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
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
  h k b 00001 w vvvv y 01 0x6c 11 ggg rrr => "vpunpcklqdq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 w vvvv y 01 0x6c @addr {$rexb=1-$b;$rexx=1-$k} => "vpunpcklqdq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
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
  h k b 00001 w vvvv y 01 0x63 11 ggg rrr => "vpacksswb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 w vvvv y 01 0x63 @addr {$rexb=1-$b;$rexx=1-$k} => "vpacksswb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y 01 0x67 11 ggg rrr => "vpackuswb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 vvvv y 01 0x67 @addr {$rexb=1-$b;$rexx=1-$k} => "vpackuswb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 0 vvvv y pp 0xc2 11 ggg rrr @imm8 => "vcmp" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00001 0 vvvv y pp 0xc2 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vcmp" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h k b 00001 w vvvv y pp 0xc6 11 ggg rrr @imm8 => "vshuf" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 00001 w vvvv y pp 0xc6 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vshuf" velt[$p] " " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h k b 00001 1 vvvv y 11 0x2a 11 ggg rrr => "vcvtsi2sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," greg[32+$r] ;
  h k b 00001 1 vvvv y 10 0x2a 11 ggg rrr => "vcvtsi2ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," greg[32+$r] ;
  h k b 00001 1 1111 y 11 0x2d 11 ggg rrr => "vcvtsd2si " greg[32+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 1 1111 y 11 0x2d @addr {$rexb=1-$b;$rexx=1-$k} => "vcvtsd2si " greg[32+$g] "," $addr ;
  h k b 00001 1 1111 y 10 0x2d 11 ggg rrr => "vcvtss2si " greg[32+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 1 1111 y 10 0x2d @addr {$rexb=1-$b;$rexx=1-$k} => "vcvtss2si " greg[32+$g] "," $addr ;
  h k b 00001 1 1111 y 11 0x2c 11 ggg rrr => "vcvttsd2si " greg[32+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 1 1111 y 10 0x2c 11 ggg rrr => "vcvttss2si " greg[32+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 1 1111 y 01 0x6e 11 ggg rrr => "vmovq " vreg[16*$y+8*$h+$g] "," greg[32+$r] ;
  h k b 00001 1 1111 y 01 0x7e 11 ggg rrr => "vmovq " greg[32+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 1111 y 10 0x7e 11 ggg rrr => "vmovq " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 0 1111 y 10 0x7e @addr {$rexb=1-$b;$rexx=1-$k} => "vmovq " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00001 0 1111 y 01 0xd6 11 ggg rrr => "vmovq " vreg[16*$y+8*$b+$r] "," vreg[16*$y+8*$h+$g] ;
  h k b 00001 0 1111 y 01 0xd6 @addr {$rexb=1-$b;$rexx=1-$k} => "vmovq " $addr "," vreg[16*$y+8*$h+$g] ;
  h k b 00010 0 1111 y 01 0x18 @addr {$rexb=1-$b;$rexx=1-$k} => "vbroadcastss " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 1111 y 01 0x19 @addr {$rexb=1-$b;$rexx=1-$k} => "vbroadcastsd " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00010 0 vvvv y 01 0x00 11 ggg rrr => "vpshufb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x00 @addr {$rexb=1-$b;$rexx=1-$k} => "vpshufb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00001 w 1111 y 00 0x10 11 ggg rrr => "vmovups " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 w 1111 y 00 0x10 @addr {$rexb=1-$b;$rexx=1-$k} => "vmovups " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00001 w 1111 y 00 0x11 @addr {$rexb=1-$b;$rexx=1-$k} => "vmovups " $addr "," vreg[16*$y+8*$h+$g] ;
  h k b 00001 w 1111 y 01 0x10 11 ggg rrr => "vmovupd " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 00001 w 1111 y 01 0x10 @addr {$rexb=1-$b;$rexx=1-$k} => "vmovupd " vreg[16*$y+8*$h+$g] "," $addr ;
  h k b 00001 w 1111 y 01 0x11 @addr {$rexb=1-$b;$rexx=1-$k} => "vmovupd " $addr "," vreg[16*$y+8*$h+$g] ;
  1 1 1 00001 1 1111 0 11 0x92 11 ggg rrr => "kmovq " kreg[$g] "," greg[32+$r] ;
  1 1 1 00001 1 1111 0 11 0x93 11 ggg rrr => "kmovq " greg[32+$g] "," kreg[$r] ;
  1 1 1 00001 1 1111 0 11 0x90 11 ggg rrr => "kmovq " kreg[$g] "," kreg[$r] ;
  1 1 1 00011 1 1111 0 01 0x32 11 ggg rrr @imm8 => "kshiftlw " kreg[$g] "," kreg[$r] "," hex($imm8) ;
  1 1 1 00011 1 1111 0 01 0x30 11 ggg rrr @imm8 => "kshiftrw " kreg[$g] "," kreg[$r] "," hex($imm8) ;
  # ---- VEX FMA (0F38, pp=66, W picks ps/pd or ss/sd) + vpmulld -- added for coverage
  h k b 00010 0 vvvv y 01 0x96 11 ggg rrr => "vfmaddsub132ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x96 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmaddsub132ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0x96 11 ggg rrr => "vfmaddsub132pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0x96 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmaddsub132pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x97 11 ggg rrr => "vfmsubadd132ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x97 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmsubadd132ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0x97 11 ggg rrr => "vfmsubadd132pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0x97 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmsubadd132pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x98 11 ggg rrr => "vfmadd132ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x98 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmadd132ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0x98 11 ggg rrr => "vfmadd132pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0x98 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmadd132pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x99 11 ggg rrr => "vfmadd132ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x99 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmadd132ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0x99 11 ggg rrr => "vfmadd132sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0x99 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmadd132sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x9a 11 ggg rrr => "vfmsub132ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x9a @addr {$rexb=1-$b;$rexx=1-$k} => "vfmsub132ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0x9a 11 ggg rrr => "vfmsub132pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0x9a @addr {$rexb=1-$b;$rexx=1-$k} => "vfmsub132pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x9b 11 ggg rrr => "vfmsub132ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x9b @addr {$rexb=1-$b;$rexx=1-$k} => "vfmsub132ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0x9b 11 ggg rrr => "vfmsub132sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0x9b @addr {$rexb=1-$b;$rexx=1-$k} => "vfmsub132sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x9c 11 ggg rrr => "vfnmadd132ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x9c @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmadd132ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0x9c 11 ggg rrr => "vfnmadd132pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0x9c @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmadd132pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x9d 11 ggg rrr => "vfnmadd132ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x9d @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmadd132ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0x9d 11 ggg rrr => "vfnmadd132sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0x9d @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmadd132sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x9e 11 ggg rrr => "vfnmsub132ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x9e @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmsub132ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0x9e 11 ggg rrr => "vfnmsub132pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0x9e @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmsub132pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x9f 11 ggg rrr => "vfnmsub132ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x9f @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmsub132ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0x9f 11 ggg rrr => "vfnmsub132sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0x9f @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmsub132sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xa6 11 ggg rrr => "vfmaddsub213ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xa6 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmaddsub213ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0xa6 11 ggg rrr => "vfmaddsub213pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0xa6 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmaddsub213pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xa7 11 ggg rrr => "vfmsubadd213ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xa7 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmsubadd213ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0xa7 11 ggg rrr => "vfmsubadd213pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0xa7 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmsubadd213pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xa8 11 ggg rrr => "vfmadd213ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xa8 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmadd213ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0xa8 11 ggg rrr => "vfmadd213pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0xa8 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmadd213pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xa9 11 ggg rrr => "vfmadd213ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xa9 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmadd213ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0xa9 11 ggg rrr => "vfmadd213sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0xa9 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmadd213sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xaa 11 ggg rrr => "vfmsub213ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xaa @addr {$rexb=1-$b;$rexx=1-$k} => "vfmsub213ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0xaa 11 ggg rrr => "vfmsub213pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0xaa @addr {$rexb=1-$b;$rexx=1-$k} => "vfmsub213pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xab 11 ggg rrr => "vfmsub213ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xab @addr {$rexb=1-$b;$rexx=1-$k} => "vfmsub213ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0xab 11 ggg rrr => "vfmsub213sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0xab @addr {$rexb=1-$b;$rexx=1-$k} => "vfmsub213sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xac 11 ggg rrr => "vfnmadd213ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xac @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmadd213ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0xac 11 ggg rrr => "vfnmadd213pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0xac @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmadd213pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xad 11 ggg rrr => "vfnmadd213ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xad @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmadd213ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0xad 11 ggg rrr => "vfnmadd213sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0xad @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmadd213sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xae 11 ggg rrr => "vfnmsub213ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xae @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmsub213ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0xae 11 ggg rrr => "vfnmsub213pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0xae @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmsub213pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xaf 11 ggg rrr => "vfnmsub213ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xaf @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmsub213ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0xaf 11 ggg rrr => "vfnmsub213sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0xaf @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmsub213sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xb6 11 ggg rrr => "vfmaddsub231ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xb6 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmaddsub231ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0xb6 11 ggg rrr => "vfmaddsub231pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0xb6 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmaddsub231pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xb7 11 ggg rrr => "vfmsubadd231ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xb7 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmsubadd231ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0xb7 11 ggg rrr => "vfmsubadd231pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0xb7 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmsubadd231pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xb8 11 ggg rrr => "vfmadd231ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xb8 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmadd231ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0xb8 11 ggg rrr => "vfmadd231pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0xb8 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmadd231pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xb9 11 ggg rrr => "vfmadd231ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xb9 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmadd231ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0xb9 11 ggg rrr => "vfmadd231sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0xb9 @addr {$rexb=1-$b;$rexx=1-$k} => "vfmadd231sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xba 11 ggg rrr => "vfmsub231ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xba @addr {$rexb=1-$b;$rexx=1-$k} => "vfmsub231ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0xba 11 ggg rrr => "vfmsub231pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0xba @addr {$rexb=1-$b;$rexx=1-$k} => "vfmsub231pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xbb 11 ggg rrr => "vfmsub231ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xbb @addr {$rexb=1-$b;$rexx=1-$k} => "vfmsub231ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0xbb 11 ggg rrr => "vfmsub231sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0xbb @addr {$rexb=1-$b;$rexx=1-$k} => "vfmsub231sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xbc 11 ggg rrr => "vfnmadd231ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xbc @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmadd231ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0xbc 11 ggg rrr => "vfnmadd231pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0xbc @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmadd231pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xbd 11 ggg rrr => "vfnmadd231ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xbd @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmadd231ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0xbd 11 ggg rrr => "vfnmadd231sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0xbd @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmadd231sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xbe 11 ggg rrr => "vfnmsub231ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xbe @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmsub231ps " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0xbe 11 ggg rrr => "vfnmsub231pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0xbe @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmsub231pd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0xbf 11 ggg rrr => "vfnmsub231ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0xbf @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmsub231ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 1 vvvv y 01 0xbf 11 ggg rrr => "vfnmsub231sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 1 vvvv y 01 0xbf @addr {$rexb=1-$b;$rexx=1-$k} => "vfnmsub231sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h k b 00010 0 vvvv y 01 0x40 11 ggg rrr => "vpmulld " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 00010 0 vvvv y 01 0x40 @addr {$rexb=1-$b;$rexx=1-$k} => "vpmulld " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  # ---- VEX lane insert/extract (vinsert/vextract f128/i128) --------------------
  h k b 00011 0 vvvv y 01 0x18 11 ggg rrr @imm8 => "vinsertf128 " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[8+$r] "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x18 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vinsertf128 " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x38 11 ggg rrr @imm8 => "vinserti128 " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[8+$r] "," hex($imm8) ;
  h k b 00011 0 vvvv y 01 0x38 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vinserti128 " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x19 11 ggg rrr @imm8 => "vextractf128 " vreg[8+$r] "," vreg[16*$y+8*$h+$g] "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x19 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vextractf128 " $addr "," vreg[16*$y+8*$h+$g] "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x39 11 ggg rrr @imm8 => "vextracti128 " vreg[8+$r] "," vreg[16*$y+8*$h+$g] "," hex($imm8) ;
  h k b 00011 0 1111 y 01 0x39 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => "vextracti128 " $addr "," vreg[16*$y+8*$h+$g] "," hex($imm8) ;
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
  h 1111 y 00 0x51 11 ggg rrr => "vsqrtps " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 00 0x51 @addr => "vsqrtps " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y 01 0x51 11 ggg rrr => "vsqrtpd " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 01 0x51 @addr => "vsqrtpd " vreg[16*$y+8*$h+$g] "," $addr ;
  h vvvv y 10 0x51 11 ggg rrr => "vsqrtss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 10 0x51 @addr => "vsqrtss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 11 0x51 11 ggg rrr => "vsqrtsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h vvvv y 11 0x51 @addr => "vsqrtsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," $addr ;
  h vvvv y 11 0x2a 11 ggg rrr => "vcvtsi2sd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," greg[$r] ;
  h vvvv y 10 0x2a 11 ggg rrr => "vcvtsi2ss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," greg[$r] ;
  h 1111 y 11 0x2d 11 ggg rrr => "vcvtsd2si " greg[$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 11 0x2d @addr => "vcvtsd2si " greg[$g] "," $addr ;
  h 1111 y 10 0x2d 11 ggg rrr => "vcvtss2si " greg[$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 10 0x2d @addr => "vcvtss2si " greg[$g] "," $addr ;
  h 1111 y 11 0x2c 11 ggg rrr => "vcvttsd2si " greg[$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 11 0x2c @addr => "vcvttsd2si " greg[$g] "," $addr ;
  h 1111 y 10 0x2c 11 ggg rrr => "vcvttss2si " greg[$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 10 0x2c @addr => "vcvttss2si " greg[$g] "," $addr ;
  h 1111 y 00 0x5b 11 ggg rrr => "vcvtdq2ps " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 00 0x5b @addr => "vcvtdq2ps " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y 01 0x5b 11 ggg rrr => "vcvtps2dq " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 01 0x5b @addr => "vcvtps2dq " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y 10 0x5b 11 ggg rrr => "vcvttps2dq " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 10 0x5b @addr => "vcvttps2dq " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y 00 0x5a 11 ggg rrr => "vcvtps2pd " vreg[16*$y+8*$h+$g] "," vreg[8+$r] ;
  h 1111 y 00 0x5a @addr => "vcvtps2pd " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y 10 0xe6 11 ggg rrr => "vcvtdq2pd " vreg[16*$y+8*$h+$g] "," vreg[8+$r] ;
  h 1111 y 10 0xe6 @addr => "vcvtdq2pd " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y 01 0x5a 11 ggg rrr => "vcvtpd2ps " vreg[8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 11 0xe6 11 ggg rrr => "vcvtpd2dq " vreg[8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 01 0xe6 11 ggg rrr => "vcvttpd2dq " vreg[8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 00 0x10 11 ggg rrr => "vmovups " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 00 0x10 @addr => "vmovups " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y 00 0x11 @addr => "vmovups " $addr "," vreg[16*$y+8*$h+$g] ;
  h 1111 y 01 0x10 11 ggg rrr => "vmovupd " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 01 0x10 @addr => "vmovupd " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y 01 0x11 @addr => "vmovupd " $addr "," vreg[16*$y+8*$h+$g] ;
  h vvvv y 10 0x10 11 ggg rrr => "vmovss " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h 1111 y 10 0x10 @addr => "vmovss " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y 10 0x11 @addr => "vmovss " $addr "," vreg[16*$y+8*$h+$g] ;
  h vvvv y 11 0x10 11 ggg rrr => "vmovsd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8+$r] ;
  h 1111 y 11 0x10 @addr => "vmovsd " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y 11 0x11 @addr => "vmovsd " $addr "," vreg[16*$y+8*$h+$g] ;
  h 1111 y 01 0x6e 11 ggg rrr => "vmovd " vreg[16*$y+8*$h+$g] "," greg[$r] ;
  h 1111 y 01 0x6e @addr => "vmovd " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y 01 0x7e 11 ggg rrr => "vmovd " greg[$r] "," vreg[16*$y+8*$h+$g] ;
  h 1111 y 01 0x7e @addr => "vmovd " $addr "," vreg[16*$y+8*$h+$g] ;
  h 1111 y 10 0x7e 11 ggg rrr => "vmovq " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] ;
  h 1111 y 10 0x7e @addr => "vmovq " vreg[16*$y+8*$h+$g] "," $addr ;
  h 1111 y 01 0xd6 11 ggg rrr => "vmovq " vreg[16*$y+8+$r] "," vreg[16*$y+8*$h+$g] ;
  h 1111 y 01 0xd6 @addr => "vmovq " $addr "," vreg[16*$y+8*$h+$g] ;
  h 1111 y 01 0x70 11 ggg rrr @imm8 => "vpshufd " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8+$r] "," hex($imm8) ;
  h 1111 y 01 0x70 @addr @imm8 => "vpshufd " vreg[16*$y+8*$h+$g] "," $addr "," hex($imm8) ;
  h vvvv y 01 0x72 11 110 rrr @imm8 => "vpslld " vvv[16*$y+$v] "," vreg[16*$y+8+$r] "," hex($imm8) ;
  h vvvv y 01 0x72 11 010 rrr @imm8 => "vpsrld " vvv[16*$y+$v] "," vreg[16*$y+8+$r] "," hex($imm8) ;
  h vvvv y 01 0x72 11 100 rrr @imm8 => "vpsrad " vvv[16*$y+$v] "," vreg[16*$y+8+$r] "," hex($imm8) ;
  h vvvv y 01 0x73 11 110 rrr @imm8 => "vpsllq " vvv[16*$y+$v] "," vreg[16*$y+8+$r] "," hex($imm8) ;
  h vvvv y 01 0x73 11 010 rrr @imm8 => "vpsrlq " vvv[16*$y+$v] "," vreg[16*$y+8+$r] "," hex($imm8) ;
  h vvvv y 01 0x73 11 111 rrr @imm8 => "vpslldq " vvv[16*$y+$v] "," vreg[16*$y+8+$r] "," hex($imm8) ;
  h vvvv y 01 0x73 11 011 rrr @imm8 => "vpsrldq " vvv[16*$y+$v] "," vreg[16*$y+8+$r] "," hex($imm8) ;
  h vvvv y 01 0x71 11 110 rrr @imm8 => "vpsllw " vvv[16*$y+$v] "," vreg[16*$y+8+$r] "," hex($imm8) ;
  h vvvv y 01 0x71 11 010 rrr @imm8 => "vpsrlw " vvv[16*$y+$v] "," vreg[16*$y+8+$r] "," hex($imm8) ;
  h vvvv y 01 0x71 11 100 rrr @imm8 => "vpsraw " vvv[16*$y+$v] "," vreg[16*$y+8+$r] "," hex($imm8) ;
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


# ===========================================================================
# EVEX (AVX-512) + XOP -- ported from the root corpus64.p. Same FSM-native
# capture treatment as VEX: op1[0x62]/op1[0x8F] route through evex/xop prefix
# stages that FIELD-capture the payload bytes into the reused VEX slots, then
# per-(map,pp,W) opcode tables set the mnemonic + operand shape. Decorations
# (mask, zeroing, broadcast, rounding) come from the EVEX P2 byte at finalize.
# ===========================================================================

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
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtdq2ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 00 z ll 1 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtdq2ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x5b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtdq2ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 00 z ll 1 1 aaa 0x5b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtdq2ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  h k b e 00 01 0 1111 1 01 z ll 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtps2dq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 01 z ll 1 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtps2dq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 0 1111 1 01 z ll 0 1 aaa 0x5b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtps2dq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 01 z ll 1 1 aaa 0x5b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtps2dq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  h k b e 00 01 0 1111 1 10 z ll 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvttps2dq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 10 z 00 1 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvttps2dq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 01 0 1111 1 10 z ll 0 1 aaa 0x5b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttps2dq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 10 z ll 1 1 aaa 0x5b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttps2dq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  h k b e 00 01 0 vvvv 1 10 z ll 0 u aaa 0x5a 11 ggg rrr => wit("evex") "vcvtss2sd " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," ereg[16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 10 z 00 1 u aaa 0x5a 11 ggg rrr => wit("evex") "vcvtss2sd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 01 0 vvvv 1 10 z ll 0 u aaa 0x5a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtss2sd " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 11 z ll 0 u aaa 0x5a 11 ggg rrr => wit("evex") "vcvtsd2ss " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," ereg[16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 11 z ll 1 u aaa 0x5a 11 ggg rrr => wit("evex") "vcvtsd2ss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 1 vvvv 1 11 z ll 0 u aaa 0x5a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtsd2ss " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," $addr ;
  h k b e 00 01 w vvvv 1 11 z ll 1 u aaa 0x2a 11 ggg rrr {$rexb=1-$b} => wit("evex") "vcvtsi2sd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," greg[32*$w+8*$rexb+$r] rcdec[$l] ;
  h k b e 00 01 w vvvv 1 10 z ll 1 u aaa 0x2a 11 ggg rrr {$rexb=1-$b} => wit("evex") "vcvtsi2ss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," greg[32*$w+8*$rexb+$r] rcdec[$l] ;
  h k b e 00 01 w 1111 1 11 z ll 1 u aaa 0x2d 11 ggg rrr {$rexr=1-$h} => wit("evex") "vcvtsd2si " greg[32*$w+8*$rexr+$g] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 w 1111 1 10 z ll 1 u aaa 0x2d 11 ggg rrr {$rexr=1-$h} => wit("evex") "vcvtss2si " greg[32*$w+8*$rexr+$g] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 w 1111 1 11 z 00 1 u aaa 0x2c 11 ggg rrr {$rexr=1-$h} => wit("evex") "vcvttsd2si " greg[32*$w+8*$rexr+$g] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 01 w 1111 1 10 z 00 1 u aaa 0x2c 11 ggg rrr {$rexr=1-$h} => wit("evex") "vcvttss2si " greg[32*$w+8*$rexr+$g] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x18 11 ggg rrr => wit("evex") "vbroadcastss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x18 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vbroadcastss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x19 11 ggg rrr => wit("evex") "vbroadcastsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[16*$k+8*$b+$r] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x19 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vbroadcastsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x58 11 ggg rrr => wit("evex") "vpbroadcastd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x58 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpbroadcastd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x59 11 ggg rrr => wit("evex") "vpbroadcastq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[16*$k+8*$b+$r] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x59 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpbroadcastq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x7c 11 ggg rrr {$rexb=1-$b} => wit("evex") "vpbroadcastd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," greg[0+8*$rexb+$r] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x7c 11 ggg rrr {$rexb=1-$b} => wit("evex") "vpbroadcastq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," greg[32+8*$rexb+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x3d 11 ggg rrr => wit("evex") "vpmaxsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x3d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmaxsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x3d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmaxsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x3d 11 ggg rrr => wit("evex") "vpmaxsq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x3d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmaxsq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x3d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmaxsq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x39 11 ggg rrr => wit("evex") "vpminsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x39 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpminsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x39 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpminsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x39 11 ggg rrr => wit("evex") "vpminsq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x39 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpminsq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x39 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpminsq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x3f 11 ggg rrr => wit("evex") "vpmaxud " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x3f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmaxud " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x3f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmaxud " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x3f 11 ggg rrr => wit("evex") "vpmaxuq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x3f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmaxuq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x3f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmaxuq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x3b 11 ggg rrr => wit("evex") "vpminud " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x3b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpminud " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x3b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpminud " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x3b 11 ggg rrr => wit("evex") "vpminuq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x3b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpminuq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x3b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpminuq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x3c 11 ggg rrr => wit("evex") "vpmaxsb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x3c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmaxsb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x38 11 ggg rrr => wit("evex") "vpminsb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x38 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpminsb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x3e 11 ggg rrr => wit("evex") "vpmaxuw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x3e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmaxuw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x3a 11 ggg rrr => wit("evex") "vpminuw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x3a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpminuw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xee 11 ggg rrr => wit("evex") "vpmaxsw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xee @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmaxsw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xea 11 ggg rrr => wit("evex") "vpminsw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xea @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpminsw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xde 11 ggg rrr => wit("evex") "vpmaxub " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xde @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmaxub " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xda 11 ggg rrr => wit("evex") "vpminub " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xda @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpminub " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x1e 11 ggg rrr => wit("evex") "vpabsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x1e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpabsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 1 1 aaa 0x1e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpabsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x1f 11 ggg rrr => wit("evex") "vpabsq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x1f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpabsq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 1 1111 1 01 z ll 1 1 aaa 0x1f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpabsq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x1c 11 ggg rrr => wit("evex") "vpabsb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x1c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpabsb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x1d 11 ggg rrr => wit("evex") "vpabsw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x1d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpabsw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x47 11 ggg rrr => wit("evex") "vpsllvd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x47 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsllvd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x47 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsllvd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x47 11 ggg rrr => wit("evex") "vpsllvq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x47 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsllvq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x47 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsllvq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x45 11 ggg rrr => wit("evex") "vpsrlvd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x45 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsrlvd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x45 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsrlvd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x45 11 ggg rrr => wit("evex") "vpsrlvq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x45 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsrlvq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x45 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsrlvq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x46 11 ggg rrr => wit("evex") "vpsravd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x46 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsravd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x46 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsravd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x46 11 ggg rrr => wit("evex") "vpsravq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x46 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsravq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x46 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsravq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x72 11 110 rrr @imm8 => wit("evex") "vpslld " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x72 @addr(6) {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpslld " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 01 0 vvvv 1 01 z ll 1 u aaa 0x72 @addr(6) {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpslld " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," $addr bcst32[$l] "," hex($imm8) ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x72 11 010 rrr @imm8 => wit("evex") "vpsrld " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x72 @addr(2) {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpsrld " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 01 0 vvvv 1 01 z ll 1 u aaa 0x72 @addr(2) {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpsrld " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," $addr bcst32[$l] "," hex($imm8) ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x72 11 100 rrr @imm8 => wit("evex") "vpsrad " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x72 @addr(4) {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpsrad " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 01 0 vvvv 1 01 z ll 1 u aaa 0x72 @addr(4) {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpsrad " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," $addr bcst32[$l] "," hex($imm8) ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x72 11 100 rrr @imm8 => wit("evex") "vpsraq " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x72 @addr(4) {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpsraq " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0x72 @addr(4) {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpsraq " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," $addr bcst64[$l] "," hex($imm8) ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x73 11 110 rrr @imm8 => wit("evex") "vpsllq " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x73 @addr(6) {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpsllq " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0x73 @addr(6) {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpsllq " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," $addr bcst64[$l] "," hex($imm8) ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x73 11 010 rrr @imm8 => wit("evex") "vpsrlq " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x73 @addr(2) {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpsrlq " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0x73 @addr(2) {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpsrlq " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," $addr bcst64[$l] "," hex($imm8) ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x71 11 110 rrr @imm8 => wit("evex") "vpsllw " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x71 @addr(6) {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpsllw " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x71 11 010 rrr @imm8 => wit("evex") "vpsrlw " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x71 @addr(2) {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpsrlw " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x71 11 100 rrr @imm8 => wit("evex") "vpsraw " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x71 @addr(4) {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpsraw " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x36 11 ggg rrr => wit("evex") "vpermd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x36 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x36 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x16 11 ggg rrr => wit("evex") "vpermps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x16 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x16 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 11 1 1111 1 01 z ll 0 1 aaa 0x00 11 ggg rrr @imm8 => wit("evex") "vpermq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z ll 0 1 aaa 0x00 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpermq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z ll 1 1 aaa 0x00 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpermq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z ll 0 1 aaa 0x01 11 ggg rrr @imm8 => wit("evex") "vpermpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z ll 0 1 aaa 0x01 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpermpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z ll 1 1 aaa 0x01 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpermpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x25 11 ggg rrr @imm8 => wit("evex") "vpternlogd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x25 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpternlogd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 1 u aaa 0x25 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpternlogd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x25 11 ggg rrr @imm8 => wit("evex") "vpternlogq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x25 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpternlogq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 1 u aaa 0x25 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpternlogq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] "," hex($imm8) ;
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x51 11 ggg rrr => wit("evex") "vsqrtps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 00 z ll 1 1 aaa 0x51 11 ggg rrr => wit("evex") "vsqrtps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x51 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vsqrtps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 00 z ll 1 1 aaa 0x51 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vsqrtps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x51 11 ggg rrr => wit("evex") "vsqrtpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 1111 1 01 z ll 1 1 aaa 0x51 11 ggg rrr => wit("evex") "vsqrtpd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x51 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vsqrtpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 1 1111 1 01 z ll 1 1 aaa 0x51 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vsqrtpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 01 0 vvvv 1 10 z ll 0 u aaa 0x51 11 ggg rrr => wit("evex") "vsqrtss " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," ereg[16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 10 z ll 1 u aaa 0x51 11 ggg rrr => wit("evex") "vsqrtss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 0 vvvv 1 10 z ll 0 u aaa 0x51 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vsqrtss " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 11 z ll 0 u aaa 0x51 11 ggg rrr => wit("evex") "vsqrtsd " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," ereg[16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 11 z ll 1 u aaa 0x51 11 ggg rrr => wit("evex") "vsqrtsd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 1 vvvv 1 11 z ll 0 u aaa 0x51 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vsqrtsd " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x28 11 ggg rrr => wit("evex") "vpmuldq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x28 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmuldq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x28 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmuldq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0xf4 11 ggg rrr => wit("evex") "vpmuludq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0xf4 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmuludq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0xf4 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmuludq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x40 11 ggg rrr => wit("evex") "vpmullq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x40 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmullq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x40 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmullq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 11 0 1111 1 01 z ll 0 1 aaa 0x08 11 ggg rrr @imm8 => wit("evex") "vrndscaleps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 1111 1 01 z 00 1 1 aaa 0x08 11 ggg rrr @imm8 => wit("evex") "vrndscaleps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" "," hex($imm8) ;
  h k b e 00 11 0 1111 1 01 z ll 0 1 aaa 0x08 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vrndscaleps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 1111 1 01 z ll 1 1 aaa 0x08 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vrndscaleps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z ll 0 1 aaa 0x09 11 ggg rrr @imm8 => wit("evex") "vrndscalepd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z 00 1 1 aaa 0x09 11 ggg rrr @imm8 => wit("evex") "vrndscalepd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z ll 0 1 aaa 0x09 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vrndscalepd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z ll 1 1 aaa 0x09 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vrndscalepd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x0a 11 ggg rrr @imm8 => wit("evex") "vrndscaless " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," ereg[16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z 00 1 u aaa 0x0a 11 ggg rrr @imm8 => wit("evex") "vrndscaless " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x0a @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vrndscaless " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x0b 11 ggg rrr @imm8 => wit("evex") "vrndscalesd " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," ereg[16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z 00 1 u aaa 0x0b 11 ggg rrr @imm8 => wit("evex") "vrndscalesd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x0b @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vrndscalesd " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x2c 11 ggg rrr => wit("evex") "vscalefps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x2c 11 ggg rrr => wit("evex") "vscalefps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x2c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vscalefps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x2c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vscalefps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x2c 11 ggg rrr => wit("evex") "vscalefpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x2c 11 ggg rrr => wit("evex") "vscalefpd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x2c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vscalefpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x2c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vscalefpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x2d 11 ggg rrr => wit("evex") "vscalefss " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," ereg[16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x2d 11 ggg rrr => wit("evex") "vscalefss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x2d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vscalefss " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x2d 11 ggg rrr => wit("evex") "vscalefsd " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," ereg[16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x2d 11 ggg rrr => wit("evex") "vscalefsd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x2d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vscalefsd " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x4c 11 ggg rrr => wit("evex") "vrcp14ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x4c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrcp14ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 1 1 aaa 0x4c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrcp14ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x4c 11 ggg rrr => wit("evex") "vrcp14pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x4c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrcp14pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 1 1111 1 01 z ll 1 1 aaa 0x4c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrcp14pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x4e 11 ggg rrr => wit("evex") "vrsqrt14ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x4e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrsqrt14ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 1 1 aaa 0x4e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrsqrt14ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x4e 11 ggg rrr => wit("evex") "vrsqrt14pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x4e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrsqrt14pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 1 1111 1 01 z ll 1 1 aaa 0x4e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrsqrt14pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x4d 11 ggg rrr => wit("evex") "vrcp14ss " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," ereg[16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x4d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrcp14ss " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x4d 11 ggg rrr => wit("evex") "vrcp14sd " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," ereg[16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x4d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrcp14sd " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x4f 11 ggg rrr => wit("evex") "vrsqrt14ss " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," ereg[16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x4f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrsqrt14ss " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x4f 11 ggg rrr => wit("evex") "vrsqrt14sd " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," ereg[16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x4f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrsqrt14sd " ereg[16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[16*$u+$v] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x42 11 ggg rrr => wit("evex") "vgetexpps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z 00 1 1 aaa 0x42 11 ggg rrr => wit("evex") "vgetexpps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x42 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vgetexpps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 1 1 aaa 0x42 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vgetexpps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x42 11 ggg rrr => wit("evex") "vgetexppd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 1111 1 01 z 00 1 1 aaa 0x42 11 ggg rrr => wit("evex") "vgetexppd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x42 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vgetexppd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 1 1111 1 01 z ll 1 1 aaa 0x42 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vgetexppd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 11 0 1111 1 01 z ll 0 1 aaa 0x26 11 ggg rrr @imm8 => wit("evex") "vgetmantps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 1111 1 01 z 00 1 1 aaa 0x26 11 ggg rrr @imm8 => wit("evex") "vgetmantps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" "," hex($imm8) ;
  h k b e 00 11 0 1111 1 01 z ll 0 1 aaa 0x26 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vgetmantps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 1111 1 01 z ll 1 1 aaa 0x26 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vgetmantps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z ll 0 1 aaa 0x26 11 ggg rrr @imm8 => wit("evex") "vgetmantpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z 00 1 1 aaa 0x26 11 ggg rrr @imm8 => wit("evex") "vgetmantpd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z ll 0 1 aaa 0x26 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vgetmantpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z ll 1 1 aaa 0x26 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vgetmantpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] "," hex($imm8) ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x64 11 ggg rrr => wit("evex") "vpblendmd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x64 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpblendmd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x64 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpblendmd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x64 11 ggg rrr => wit("evex") "vpblendmq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x64 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpblendmq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x64 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpblendmq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x65 11 ggg rrr => wit("evex") "vblendmps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x65 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vblendmps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x65 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vblendmps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x65 11 ggg rrr => wit("evex") "vblendmpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x65 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vblendmpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x65 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vblendmpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  1 k b 1 00 10 0 vvvv 1 10 0 ll 0 u aaa 0x27 11 ggg rrr => wit("evex") "vptestnmd " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  1 k b 1 00 10 0 vvvv 1 10 0 ll 0 u aaa 0x27 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vptestnmd " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  1 k b 1 00 10 1 vvvv 1 10 0 ll 0 u aaa 0x27 11 ggg rrr => wit("evex") "vptestnmq " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  1 k b 1 00 10 1 vvvv 1 10 0 ll 0 u aaa 0x27 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vptestnmq " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtps2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 00 z 00 1 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtps2pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x5a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtps2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 00 z ll 1 1 aaa 0x5a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtps2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtpd2ps " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 1111 1 01 z ll 1 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtpd2ps " eregh[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x5a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtpd2ps " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 1 1111 1 01 z ll 1 1 aaa 0x5a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtpd2ps " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 01 0 1111 1 10 z ll 0 1 aaa 0xe6 11 ggg rrr => wit("evex") "vcvtdq2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 10 z ll 0 1 aaa 0xe6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtdq2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 10 z ll 1 1 aaa 0xe6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtdq2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 01 0 1111 1 10 z ll 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtudq2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 10 z ll 0 1 aaa 0x7a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtudq2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 10 z ll 1 1 aaa 0x7a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtudq2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 01 1 1111 1 11 z ll 0 1 aaa 0xe6 11 ggg rrr => wit("evex") "vcvtpd2dq " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 1111 1 11 z ll 1 1 aaa 0xe6 11 ggg rrr => wit("evex") "vcvtpd2dq " eregh[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 1 1111 1 11 z ll 0 1 aaa 0xe6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtpd2dq " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 1 1111 1 11 z ll 1 1 aaa 0xe6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtpd2dq " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0xe6 11 ggg rrr => wit("evex") "vcvttpd2dq " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 1111 1 01 z 00 1 1 aaa 0xe6 11 ggg rrr => wit("evex") "vcvttpd2dq " eregh[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0xe6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttpd2dq " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 1 1111 1 01 z ll 1 1 aaa 0xe6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttpd2dq " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x30 11 ggg rrr => wit("evex") "vpmovzxbw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x30 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovzxbw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x31 11 ggg rrr => wit("evex") "vpmovzxbd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x31 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovzxbd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x32 11 ggg rrr => wit("evex") "vpmovzxbq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x32 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovzxbq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x33 11 ggg rrr => wit("evex") "vpmovzxwd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x33 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovzxwd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x34 11 ggg rrr => wit("evex") "vpmovzxwq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x34 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovzxwq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x35 11 ggg rrr => wit("evex") "vpmovzxdq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x35 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovzxdq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x20 11 ggg rrr => wit("evex") "vpmovsxbw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x20 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovsxbw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x21 11 ggg rrr => wit("evex") "vpmovsxbd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x21 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovsxbd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x22 11 ggg rrr => wit("evex") "vpmovsxbq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x22 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovsxbq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x23 11 ggg rrr => wit("evex") "vpmovsxwd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x23 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovsxwd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x24 11 ggg rrr => wit("evex") "vpmovsxwq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x24 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovsxwq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x25 11 ggg rrr => wit("evex") "vpmovsxdq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x25 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovsxdq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x30 11 ggg rrr => wit("evex") wit("alt") "vpmovwb " eregh[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x30 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovwb " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x31 11 ggg rrr => wit("evex") wit("alt") "vpmovdb " eregx[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x31 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovdb " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x32 11 ggg rrr => wit("evex") wit("alt") "vpmovqb " eregx[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x32 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovqb " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x33 11 ggg rrr => wit("evex") wit("alt") "vpmovdw " eregh[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x33 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovdw " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x34 11 ggg rrr => wit("evex") wit("alt") "vpmovqw " eregx[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x34 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovqw " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x35 11 ggg rrr => wit("evex") wit("alt") "vpmovqd " eregh[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x35 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovqd " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x20 11 ggg rrr => wit("evex") wit("alt") "vpmovswb " eregh[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x20 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovswb " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x21 11 ggg rrr => wit("evex") wit("alt") "vpmovsdb " eregx[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x21 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovsdb " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x22 11 ggg rrr => wit("evex") wit("alt") "vpmovsqb " eregx[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x22 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovsqb " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x23 11 ggg rrr => wit("evex") wit("alt") "vpmovsdw " eregh[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x23 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovsdw " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x24 11 ggg rrr => wit("evex") wit("alt") "vpmovsqw " eregx[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x24 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovsqw " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x25 11 ggg rrr => wit("evex") wit("alt") "vpmovsqd " eregh[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x25 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovsqd " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x10 11 ggg rrr => wit("evex") wit("alt") "vpmovuswb " eregh[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x10 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovuswb " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x11 11 ggg rrr => wit("evex") wit("alt") "vpmovusdb " eregx[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x11 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovusdb " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x12 11 ggg rrr => wit("evex") wit("alt") "vpmovusqb " eregx[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x12 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovusqb " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x13 11 ggg rrr => wit("evex") wit("alt") "vpmovusdw " eregh[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x13 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovusdw " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x14 11 ggg rrr => wit("evex") wit("alt") "vpmovusqw " eregx[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x14 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovusqw " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x15 11 ggg rrr => wit("evex") wit("alt") "vpmovusqd " eregh[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x15 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmovusqd " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0x90 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpgatherdd " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0x90 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpgatherdd " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0x90 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpgatherdd " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0x90 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpgatherdd " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0x90 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpgatherdd " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0x90 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpgatherdd " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0x90 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpgatherdd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0x90 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpgatherdd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0x90 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpgatherdd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0x90 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpgatherdq " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0x90 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpgatherdq " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0x90 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpgatherdq " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0x90 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpgatherdq " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0x90 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpgatherdq " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0x90 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpgatherdq " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0x90 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpgatherdq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0x90 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpgatherdq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0x90 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpgatherdq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0x91 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpgatherqd " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0x91 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpgatherqd " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0x91 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpgatherqd " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0x91 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpgatherqd " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0x91 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpgatherqd " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0x91 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpgatherqd " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0x91 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpgatherqd " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0x91 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpgatherqd " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0x91 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpgatherqd " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0x91 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpgatherqq " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0x91 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpgatherqq " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0x91 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpgatherqq " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0x91 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpgatherqq " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0x91 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpgatherqq " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0x91 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpgatherqq " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0x91 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpgatherqq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0x91 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpgatherqq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0x91 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpgatherqq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0x92 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vgatherdps " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0x92 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vgatherdps " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0x92 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vgatherdps " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0x92 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vgatherdps " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0x92 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vgatherdps " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0x92 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vgatherdps " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0x92 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vgatherdps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0x92 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vgatherdps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0x92 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vgatherdps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0x92 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vgatherdpd " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0x92 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vgatherdpd " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0x92 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vgatherdpd " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0x92 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vgatherdpd " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0x92 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vgatherdpd " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0x92 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vgatherdpd " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0x92 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vgatherdpd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0x92 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vgatherdpd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0x92 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vgatherdpd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0x93 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vgatherqps " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0x93 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vgatherqps " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0x93 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vgatherqps " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0x93 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vgatherqps " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0x93 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vgatherqps " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0x93 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vgatherqps " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0x93 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vgatherqps " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0x93 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vgatherqps " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0x93 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vgatherqps " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0x93 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vgatherqpd " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0x93 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vgatherqpd " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0x93 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vgatherqpd " ereg[0+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0x93 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vgatherqpd " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0x93 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vgatherqpd " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0x93 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vgatherqpd " ereg[32+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0x93 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vgatherqpd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] "]" ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0x93 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vgatherqpd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0x93 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vgatherqpd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0xa0 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpscatterdd " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0xa0 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpscatterdd " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0xa0 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpscatterdd " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0xa0 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpscatterdd " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0xa0 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpscatterdd " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0xa0 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpscatterdd " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0xa0 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpscatterdd " "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[64+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0xa0 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpscatterdd " "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[64+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0xa0 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpscatterdd " "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[64+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0xa0 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpscatterdq " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0xa0 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpscatterdq " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0xa0 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpscatterdq " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0xa0 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpscatterdq " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0xa0 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpscatterdq " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0xa0 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpscatterdq " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0xa0 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpscatterdq " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[64+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0xa0 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpscatterdq " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[64+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0xa0 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpscatterdq " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[64+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0xa1 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpscatterqd " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0xa1 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpscatterqd " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0xa1 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpscatterqd " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0xa1 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpscatterqd " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0xa1 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpscatterqd " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0xa1 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpscatterqd " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0xa1 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpscatterqd " "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0xa1 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpscatterqd " "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0xa1 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpscatterqd " "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0xa1 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpscatterqq " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0xa1 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpscatterqq " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0xa1 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpscatterqq " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0xa1 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpscatterqq " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0xa1 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpscatterqq " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0xa1 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpscatterqq " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0xa1 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vpscatterqq " "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[64+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0xa1 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vpscatterqq " "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[64+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0xa1 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vpscatterqq " "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[64+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0xa2 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vscatterdps " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0xa2 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vscatterdps " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0xa2 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vscatterdps " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0xa2 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vscatterdps " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0xa2 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vscatterdps " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0xa2 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vscatterdps " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0xa2 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vscatterdps " "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[64+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0xa2 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vscatterdps " "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[64+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0xa2 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vscatterdps " "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[64+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0xa2 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vscatterdpd " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0xa2 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vscatterdpd " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0xa2 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vscatterdpd " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0xa2 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vscatterdpd " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0xa2 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vscatterdpd " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0xa2 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vscatterdpd " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0xa2 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vscatterdpd " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[64+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0xa2 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vscatterdpd " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[64+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0xa2 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vscatterdpd " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[64+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0xa3 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vscatterqps " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0xa3 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vscatterqps " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 00 0 u aaa 0xa3 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vscatterqps " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0xa3 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vscatterqps " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0xa3 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vscatterqps " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 01 0 u aaa 0xa3 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vscatterqps " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0xa3 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vscatterqps " "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0xa3 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vscatterqps " "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z 10 0 u aaa 0xa3 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vscatterqps " "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0xa3 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vscatterqpd " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0xa3 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vscatterqpd " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 00 0 u aaa 0xa3 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vscatterqpd " "[" areg[8*$rexb+$d] "+" ereg[0+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[0+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0xa3 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vscatterqpd " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0xa3 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vscatterqpd " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 01 0 u aaa 0xa3 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vscatterqpd " "[" areg[8*$rexb+$d] "+" ereg[32+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[32+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0xa3 00 ggg 100 ss iii ddd {$rexb=1-$b} => wit("evex") "vscatterqpd " "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] "]" kzdec[$z*8+$a] "," ereg[64+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0xa3 01 ggg 100 ss iii ddd @disp8 {$rexb=1-$b} => wit("evex") "vscatterqpd " "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp8) "]" kzdec[$z*8+$a] "," ereg[64+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z 10 0 u aaa 0xa3 10 ggg 100 ss iii ddd @disp32 {$rexb=1-$b} => wit("evex") "vscatterqpd " "[" areg[8*$rexb+$d] "+" ereg[64+16*$u+8*$k+$i] "*" vscale[$s] sgn($disp32) "]" kzdec[$z*8+$a] "," ereg[64+16*$e+8*$h+$g] ;
  # ---- EVEX lane extract (vextract{f,i}{32,64}x{2,4,8}) -- MRI, narrow dst ----
  h k b e 00 11 0 1111 1 01 z ll 0 u aaa 0x19 11 ggg rrr @imm8 => wit("evex") "vextractf32x4 " eregx[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] "," hex($imm8) ;
  h k b e 00 11 0 1111 1 01 z ll 0 u aaa 0x19 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vextractf32x4 " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z ll 0 u aaa 0x19 11 ggg rrr @imm8 => wit("evex") "vextractf64x2 " eregx[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z ll 0 u aaa 0x19 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vextractf64x2 " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] "," hex($imm8) ;
  h k b e 00 11 0 1111 1 01 z ll 0 u aaa 0x1b 11 ggg rrr @imm8 => wit("evex") "vextractf32x8 " eregh[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] "," hex($imm8) ;
  h k b e 00 11 0 1111 1 01 z ll 0 u aaa 0x1b @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vextractf32x8 " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z ll 0 u aaa 0x1b 11 ggg rrr @imm8 => wit("evex") "vextractf64x4 " eregh[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z ll 0 u aaa 0x1b @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vextractf64x4 " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] "," hex($imm8) ;
  h k b e 00 11 0 1111 1 01 z ll 0 u aaa 0x39 11 ggg rrr @imm8 => wit("evex") "vextracti32x4 " eregx[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] "," hex($imm8) ;
  h k b e 00 11 0 1111 1 01 z ll 0 u aaa 0x39 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vextracti32x4 " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z ll 0 u aaa 0x39 11 ggg rrr @imm8 => wit("evex") "vextracti64x2 " eregx[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z ll 0 u aaa 0x39 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vextracti64x2 " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] "," hex($imm8) ;
  h k b e 00 11 0 1111 1 01 z ll 0 u aaa 0x3b 11 ggg rrr @imm8 => wit("evex") "vextracti32x8 " eregh[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] "," hex($imm8) ;
  h k b e 00 11 0 1111 1 01 z ll 0 u aaa 0x3b @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vextracti32x8 " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z ll 0 u aaa 0x3b 11 ggg rrr @imm8 => wit("evex") "vextracti64x4 " eregh[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z ll 0 u aaa 0x3b @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vextracti64x4 " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] "," hex($imm8) ;
  # ==== EVEX FMA completion: scalar vfmadd/msub/nmadd/nmsub + packed addsub/subadd/msub/nmadd/nmsub ====
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x99 11 ggg rrr => wit("evex") "vfmadd132sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x99 11 ggg rrr => wit("evex") "vfmadd132sd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x99 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd132sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x99 11 ggg rrr => wit("evex") "vfmadd132ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x99 11 ggg rrr => wit("evex") "vfmadd132ss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x99 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd132ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xa9 11 ggg rrr => wit("evex") "vfmadd213sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xa9 11 ggg rrr => wit("evex") "vfmadd213sd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xa9 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd213sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xa9 11 ggg rrr => wit("evex") "vfmadd213ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xa9 11 ggg rrr => wit("evex") "vfmadd213ss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xa9 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd213ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xb9 11 ggg rrr => wit("evex") "vfmadd231sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xb9 11 ggg rrr => wit("evex") "vfmadd231sd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xb9 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd231sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xb9 11 ggg rrr => wit("evex") "vfmadd231ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xb9 11 ggg rrr => wit("evex") "vfmadd231ss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xb9 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd231ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x96 11 ggg rrr => wit("evex") "vfmaddsub132pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x96 11 ggg rrr => wit("evex") "vfmaddsub132pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x96 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmaddsub132pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x96 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmaddsub132pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x96 11 ggg rrr => wit("evex") "vfmaddsub132ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x96 11 ggg rrr => wit("evex") "vfmaddsub132ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x96 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmaddsub132ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x96 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmaddsub132ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xa6 11 ggg rrr => wit("evex") "vfmaddsub213pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xa6 11 ggg rrr => wit("evex") "vfmaddsub213pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xa6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmaddsub213pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xa6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmaddsub213pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xa6 11 ggg rrr => wit("evex") "vfmaddsub213ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xa6 11 ggg rrr => wit("evex") "vfmaddsub213ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xa6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmaddsub213ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xa6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmaddsub213ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xb6 11 ggg rrr => wit("evex") "vfmaddsub231pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xb6 11 ggg rrr => wit("evex") "vfmaddsub231pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xb6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmaddsub231pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xb6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmaddsub231pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xb6 11 ggg rrr => wit("evex") "vfmaddsub231ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xb6 11 ggg rrr => wit("evex") "vfmaddsub231ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xb6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmaddsub231ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xb6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmaddsub231ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x9a 11 ggg rrr => wit("evex") "vfmsub132pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x9a 11 ggg rrr => wit("evex") "vfmsub132pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x9a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub132pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x9a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub132pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x9a 11 ggg rrr => wit("evex") "vfmsub132ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x9a 11 ggg rrr => wit("evex") "vfmsub132ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x9a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub132ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x9a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub132ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x9b 11 ggg rrr => wit("evex") "vfmsub132sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x9b 11 ggg rrr => wit("evex") "vfmsub132sd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x9b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub132sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x9b 11 ggg rrr => wit("evex") "vfmsub132ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x9b 11 ggg rrr => wit("evex") "vfmsub132ss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x9b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub132ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xab 11 ggg rrr => wit("evex") "vfmsub213sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xab 11 ggg rrr => wit("evex") "vfmsub213sd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xab @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub213sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xab 11 ggg rrr => wit("evex") "vfmsub213ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xab 11 ggg rrr => wit("evex") "vfmsub213ss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xab @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub213ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xba 11 ggg rrr => wit("evex") "vfmsub231pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xba 11 ggg rrr => wit("evex") "vfmsub231pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xba @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub231pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xba @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub231pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xba 11 ggg rrr => wit("evex") "vfmsub231ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xba 11 ggg rrr => wit("evex") "vfmsub231ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xba @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub231ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xba @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub231ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xbb 11 ggg rrr => wit("evex") "vfmsub231sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xbb 11 ggg rrr => wit("evex") "vfmsub231sd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xbb @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub231sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xbb 11 ggg rrr => wit("evex") "vfmsub231ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xbb 11 ggg rrr => wit("evex") "vfmsub231ss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xbb @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub231ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x97 11 ggg rrr => wit("evex") "vfmsubadd132pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x97 11 ggg rrr => wit("evex") "vfmsubadd132pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x97 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsubadd132pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x97 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsubadd132pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x97 11 ggg rrr => wit("evex") "vfmsubadd132ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x97 11 ggg rrr => wit("evex") "vfmsubadd132ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x97 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsubadd132ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x97 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsubadd132ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xa7 11 ggg rrr => wit("evex") "vfmsubadd213pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xa7 11 ggg rrr => wit("evex") "vfmsubadd213pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xa7 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsubadd213pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xa7 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsubadd213pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xa7 11 ggg rrr => wit("evex") "vfmsubadd213ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xa7 11 ggg rrr => wit("evex") "vfmsubadd213ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xa7 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsubadd213ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xa7 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsubadd213ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xb7 11 ggg rrr => wit("evex") "vfmsubadd231pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xb7 11 ggg rrr => wit("evex") "vfmsubadd231pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xb7 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsubadd231pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xb7 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsubadd231pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xb7 11 ggg rrr => wit("evex") "vfmsubadd231ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xb7 11 ggg rrr => wit("evex") "vfmsubadd231ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xb7 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsubadd231ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xb7 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsubadd231ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x9c 11 ggg rrr => wit("evex") "vfnmadd132pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x9c 11 ggg rrr => wit("evex") "vfnmadd132pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x9c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd132pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x9c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd132pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x9c 11 ggg rrr => wit("evex") "vfnmadd132ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x9c 11 ggg rrr => wit("evex") "vfnmadd132ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x9c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd132ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x9c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd132ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x9d 11 ggg rrr => wit("evex") "vfnmadd132sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x9d 11 ggg rrr => wit("evex") "vfnmadd132sd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x9d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd132sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x9d 11 ggg rrr => wit("evex") "vfnmadd132ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x9d 11 ggg rrr => wit("evex") "vfnmadd132ss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x9d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd132ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xad 11 ggg rrr => wit("evex") "vfnmadd213sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xad 11 ggg rrr => wit("evex") "vfnmadd213sd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xad @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd213sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xad 11 ggg rrr => wit("evex") "vfnmadd213ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xad 11 ggg rrr => wit("evex") "vfnmadd213ss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xad @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd213ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xbc 11 ggg rrr => wit("evex") "vfnmadd231pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xbc 11 ggg rrr => wit("evex") "vfnmadd231pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xbc @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd231pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xbc @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd231pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xbc 11 ggg rrr => wit("evex") "vfnmadd231ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xbc 11 ggg rrr => wit("evex") "vfnmadd231ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xbc @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd231ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xbc @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd231ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xbd 11 ggg rrr => wit("evex") "vfnmadd231sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xbd 11 ggg rrr => wit("evex") "vfnmadd231sd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xbd @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd231sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xbd 11 ggg rrr => wit("evex") "vfnmadd231ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xbd 11 ggg rrr => wit("evex") "vfnmadd231ss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xbd @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd231ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x9e 11 ggg rrr => wit("evex") "vfnmsub132pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x9e 11 ggg rrr => wit("evex") "vfnmsub132pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x9e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub132pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x9e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub132pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x9e 11 ggg rrr => wit("evex") "vfnmsub132ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x9e 11 ggg rrr => wit("evex") "vfnmsub132ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x9e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub132ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x9e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub132ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x9f 11 ggg rrr => wit("evex") "vfnmsub132sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x9f 11 ggg rrr => wit("evex") "vfnmsub132sd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x9f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub132sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x9f 11 ggg rrr => wit("evex") "vfnmsub132ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x9f 11 ggg rrr => wit("evex") "vfnmsub132ss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x9f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub132ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xae 11 ggg rrr => wit("evex") "vfnmsub213pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xae 11 ggg rrr => wit("evex") "vfnmsub213pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xae @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub213pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xae @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub213pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xae 11 ggg rrr => wit("evex") "vfnmsub213ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xae 11 ggg rrr => wit("evex") "vfnmsub213ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xae @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub213ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xae @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub213ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xaf 11 ggg rrr => wit("evex") "vfnmsub213sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xaf 11 ggg rrr => wit("evex") "vfnmsub213sd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xaf @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub213sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xaf 11 ggg rrr => wit("evex") "vfnmsub213ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xaf 11 ggg rrr => wit("evex") "vfnmsub213ss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xaf @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub213ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xbe 11 ggg rrr => wit("evex") "vfnmsub231pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xbe 11 ggg rrr => wit("evex") "vfnmsub231pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xbe @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub231pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xbe @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub231pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xbe 11 ggg rrr => wit("evex") "vfnmsub231ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xbe 11 ggg rrr => wit("evex") "vfnmsub231ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xbe @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub231ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xbe @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub231ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xbf 11 ggg rrr => wit("evex") "vfnmsub231sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xbf 11 ggg rrr => wit("evex") "vfnmsub231sd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xbf @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub231sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xbf 11 ggg rrr => wit("evex") "vfnmsub231ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0xbf 11 ggg rrr => wit("evex") "vfnmsub231ss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xbf @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub231ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  # ==== EVEX packed-integer completion: arith(bw) / pack / unpack / blendm / palignr ====
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xd8 11 ggg rrr => wit("evex") "vpsubusb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xd8 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsubusb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xd9 11 ggg rrr => wit("evex") "vpsubusw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xd9 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsubusw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xdc 11 ggg rrr => wit("evex") "vpaddusb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xdc @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpaddusb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xdd 11 ggg rrr => wit("evex") "vpaddusw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xdd @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpaddusw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xe8 11 ggg rrr => wit("evex") "vpsubsb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xe8 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsubsb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xe9 11 ggg rrr => wit("evex") "vpsubsw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xe9 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsubsw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xec 11 ggg rrr => wit("evex") "vpaddsb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xec @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpaddsb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xed 11 ggg rrr => wit("evex") "vpaddsw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xed @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpaddsw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xe0 11 ggg rrr => wit("evex") "vpavgb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xe0 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpavgb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xe3 11 ggg rrr => wit("evex") "vpavgw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xe3 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpavgw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xe4 11 ggg rrr => wit("evex") "vpmulhuw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xe4 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmulhuw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xe5 11 ggg rrr => wit("evex") "vpmulhw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xe5 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmulhw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xf5 11 ggg rrr => wit("evex") "vpmaddwd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xf5 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmaddwd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xf6 11 ggg rrr => wit("evex") "vpsadbw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xf6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsadbw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x04 11 ggg rrr => wit("evex") "vpmaddubsw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x04 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmaddubsw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x0b 11 ggg rrr => wit("evex") "vpmulhrsw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x0b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmulhrsw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x63 11 ggg rrr => wit("evex") "vpacksswb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x63 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpacksswb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x6b 11 ggg rrr => wit("evex") "vpackssdw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x6b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpackssdw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 1 u aaa 0x6b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpackssdw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x67 11 ggg rrr => wit("evex") "vpackuswb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x67 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpackuswb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x2b 11 ggg rrr => wit("evex") "vpackusdw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x2b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpackusdw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x2b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpackusdw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x60 11 ggg rrr => wit("evex") "vpunpcklbw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x60 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpunpcklbw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x61 11 ggg rrr => wit("evex") "vpunpcklwd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x61 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpunpcklwd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x62 11 ggg rrr => wit("evex") "vpunpckldq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x62 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpunpckldq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 1 u aaa 0x62 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpunpckldq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x6c 11 ggg rrr => wit("evex") "vpunpcklqdq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x6c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpunpcklqdq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0x6c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpunpcklqdq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x68 11 ggg rrr => wit("evex") "vpunpckhbw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x68 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpunpckhbw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x69 11 ggg rrr => wit("evex") "vpunpckhwd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x69 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpunpckhwd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x6a 11 ggg rrr => wit("evex") "vpunpckhdq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x6a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpunpckhdq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 1 u aaa 0x6a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpunpckhdq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x6d 11 ggg rrr => wit("evex") "vpunpckhqdq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x6d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpunpckhqdq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0x6d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpunpckhqdq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x66 11 ggg rrr => wit("evex") "vpblendmb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x66 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpblendmb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x66 11 ggg rrr => wit("evex") "vpblendmw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x66 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpblendmw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x0f 11 ggg rrr @imm8 => wit("evex") "vpalignr " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x0f @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpalignr " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  # ==== EVEX shift completion: by-count-xmm (D1-F3) + variable word (vpsllvw/vpsrlvw/vpsravw) ====
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xd1 11 ggg rrr => wit("evex") "vpsrlw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xd1 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsrlw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xd2 11 ggg rrr => wit("evex") "vpsrld " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xd2 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsrld " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0xd3 11 ggg rrr => wit("evex") "vpsrlq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0xd3 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsrlq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xe1 11 ggg rrr => wit("evex") "vpsraw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xe1 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsraw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xe2 11 ggg rrr => wit("evex") "vpsrad " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xe2 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsrad " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0xe2 11 ggg rrr => wit("evex") "vpsraq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0xe2 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsraq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xf1 11 ggg rrr => wit("evex") "vpsllw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xf1 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsllw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xf2 11 ggg rrr => wit("evex") "vpslld " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0xf2 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpslld " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0xf3 11 ggg rrr => wit("evex") "vpsllq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0xf3 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsllq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x10 11 ggg rrr => wit("evex") "vpsrlvw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x10 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsrlvw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x11 11 ggg rrr => wit("evex") "vpsravw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x11 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsravw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x12 11 ggg rrr => wit("evex") "vpsllvw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x12 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpsllvw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  # ==== EVEX crypto: VAES / GFNI / PCLMULQDQ ====
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xdc 11 ggg rrr => wit("evex") "vaesenc " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xdc @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vaesenc " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xdd 11 ggg rrr => wit("evex") "vaesenclast " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xdd @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vaesenclast " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xde 11 ggg rrr => wit("evex") "vaesdec " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xde @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vaesdec " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xdf 11 ggg rrr => wit("evex") "vaesdeclast " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xdf @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vaesdeclast " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xcf 11 ggg rrr => wit("evex") "vgf2p8mulb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xcf @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vgf2p8mulb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0xce 11 ggg rrr @imm8 => wit("evex") "vgf2p8affineqb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0xce @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vgf2p8affineqb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 1 u aaa 0xce @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vgf2p8affineqb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0xcf 11 ggg rrr @imm8 => wit("evex") "vgf2p8affineinvqb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0xcf @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vgf2p8affineinvqb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 1 u aaa 0xcf @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vgf2p8affineinvqb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x44 11 ggg rrr @imm8 => wit("evex") "vpclmulqdq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x44 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpclmulqdq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  # ==== EVEX VPMISC: variable rotates/funnel-shifts, madd52, multishift (RVM) + lzcnt/popcnt/conflict (RM) ====
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x44 11 ggg rrr => wit("evex") "vplzcntd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x44 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vplzcntd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 1 1 aaa 0x44 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vplzcntd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x44 11 ggg rrr => wit("evex") "vplzcntq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x44 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vplzcntq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 1 1111 1 01 z ll 1 1 aaa 0x44 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vplzcntq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0xc4 11 ggg rrr => wit("evex") "vpconflictd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0xc4 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpconflictd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 1 1 aaa 0xc4 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpconflictd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0xc4 11 ggg rrr => wit("evex") "vpconflictq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0xc4 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpconflictq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 1 1111 1 01 z ll 1 1 aaa 0xc4 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpconflictq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x54 11 ggg rrr => wit("evex") "vpopcntb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x54 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpopcntb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x54 11 ggg rrr => wit("evex") "vpopcntw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x54 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpopcntw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x55 11 ggg rrr => wit("evex") "vpopcntd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x55 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpopcntd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 1 1 aaa 0x55 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpopcntd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x55 11 ggg rrr => wit("evex") "vpopcntq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x55 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpopcntq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 1 1111 1 01 z ll 1 1 aaa 0x55 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpopcntq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x14 11 ggg rrr => wit("evex") "vprorvd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x14 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vprorvd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x14 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vprorvd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x14 11 ggg rrr => wit("evex") "vprorvq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x14 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vprorvq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x14 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vprorvq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x15 11 ggg rrr => wit("evex") "vprolvd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x15 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vprolvd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x15 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vprolvd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x15 11 ggg rrr => wit("evex") "vprolvq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x15 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vprolvq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x15 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vprolvq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x70 11 ggg rrr => wit("evex") "vpshldvw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x70 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpshldvw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x71 11 ggg rrr => wit("evex") "vpshldvd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x71 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpshldvd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x71 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpshldvd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x71 11 ggg rrr => wit("evex") "vpshldvq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x71 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpshldvq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x71 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpshldvq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x72 11 ggg rrr => wit("evex") "vpshrdvw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x72 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpshrdvw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x73 11 ggg rrr => wit("evex") "vpshrdvd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x73 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpshrdvd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x73 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpshrdvd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x73 11 ggg rrr => wit("evex") "vpshrdvq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x73 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpshrdvq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x73 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpshrdvq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xb4 11 ggg rrr => wit("evex") "vpmadd52luq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xb4 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmadd52luq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xb4 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmadd52luq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xb5 11 ggg rrr => wit("evex") "vpmadd52huq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xb5 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmadd52huq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0xb5 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmadd52huq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x83 11 ggg rrr => wit("evex") "vpmultishiftqb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x83 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmultishiftqb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x83 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpmultishiftqb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  # ==== EVEX dot-product / bf16: vpdp* (VNNI, VNNI-INT8) + vdpbf16ps (RVM, bcst32) ====
  h k b e 00 10 0 vvvv 1 11 z ll 0 u aaa 0x50 11 ggg rrr => wit("evex") "vpdpbssd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 11 z ll 0 u aaa 0x50 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpdpbssd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 11 z ll 1 u aaa 0x50 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpdpbssd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 0 vvvv 1 11 z ll 0 u aaa 0x51 11 ggg rrr => wit("evex") "vpdpbssds " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 11 z ll 0 u aaa 0x51 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpdpbssds " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 11 z ll 1 u aaa 0x51 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpdpbssds " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 0 vvvv 1 10 z ll 0 u aaa 0x50 11 ggg rrr => wit("evex") "vpdpbsud " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 10 z ll 0 u aaa 0x50 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpdpbsud " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 10 z ll 1 u aaa 0x50 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpdpbsud " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 0 vvvv 1 10 z ll 0 u aaa 0x51 11 ggg rrr => wit("evex") "vpdpbsuds " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 10 z ll 0 u aaa 0x51 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpdpbsuds " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 10 z ll 1 u aaa 0x51 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpdpbsuds " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x50 11 ggg rrr => wit("evex") "vpdpbusd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x50 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpdpbusd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x50 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpdpbusd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x51 11 ggg rrr => wit("evex") "vpdpbusds " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x51 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpdpbusds " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x51 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpdpbusds " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 0 vvvv 1 00 z ll 0 u aaa 0x50 11 ggg rrr => wit("evex") "vpdpbuud " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 00 z ll 0 u aaa 0x50 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpdpbuud " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 00 z ll 1 u aaa 0x50 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpdpbuud " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 0 vvvv 1 00 z ll 0 u aaa 0x51 11 ggg rrr => wit("evex") "vpdpbuuds " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 00 z ll 0 u aaa 0x51 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpdpbuuds " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 00 z ll 1 u aaa 0x51 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpdpbuuds " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x52 11 ggg rrr => wit("evex") "vpdpwssd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x52 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpdpwssd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x52 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpdpwssd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x53 11 ggg rrr => wit("evex") "vpdpwssds " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x53 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpdpwssds " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x53 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpdpwssds " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 0 vvvv 1 10 z ll 0 u aaa 0x52 11 ggg rrr => wit("evex") "vdpbf16ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 10 z ll 0 u aaa 0x52 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vdpbf16ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 10 z ll 1 u aaa 0x52 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vdpbf16ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  # ==== EVEX integer compare -> mask: vpcmpeq/gt {b,w,q}, vptestm/nm {b,w} (kreg dest) ====
  1 k b 1 00 01 0 vvvv 1 01 0 ll 0 u aaa 0x74 11 ggg rrr => wit("evex") "vpcmpeqb " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  1 k b 1 00 01 0 vvvv 1 01 0 ll 0 u aaa 0x74 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpcmpeqb " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  1 k b 1 00 01 0 vvvv 1 01 0 ll 0 u aaa 0x75 11 ggg rrr => wit("evex") "vpcmpeqw " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  1 k b 1 00 01 0 vvvv 1 01 0 ll 0 u aaa 0x75 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpcmpeqw " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  1 k b 1 00 10 1 vvvv 1 01 0 ll 0 u aaa 0x29 11 ggg rrr => wit("evex") "vpcmpeqq " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  1 k b 1 00 10 1 vvvv 1 01 0 ll 0 u aaa 0x29 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpcmpeqq " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  1 k b 1 00 10 1 vvvv 1 01 0 ll 1 u aaa 0x29 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpcmpeqq " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  1 k b 1 00 01 0 vvvv 1 01 0 ll 0 u aaa 0x64 11 ggg rrr => wit("evex") "vpcmpgtb " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  1 k b 1 00 01 0 vvvv 1 01 0 ll 0 u aaa 0x64 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpcmpgtb " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  1 k b 1 00 01 0 vvvv 1 01 0 ll 0 u aaa 0x65 11 ggg rrr => wit("evex") "vpcmpgtw " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  1 k b 1 00 01 0 vvvv 1 01 0 ll 0 u aaa 0x65 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpcmpgtw " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  1 k b 1 00 10 1 vvvv 1 01 0 ll 0 u aaa 0x37 11 ggg rrr => wit("evex") "vpcmpgtq " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  1 k b 1 00 10 1 vvvv 1 01 0 ll 0 u aaa 0x37 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpcmpgtq " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  1 k b 1 00 10 1 vvvv 1 01 0 ll 1 u aaa 0x37 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpcmpgtq " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  1 k b 1 00 10 0 vvvv 1 01 0 ll 0 u aaa 0x26 11 ggg rrr => wit("evex") "vptestmb " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  1 k b 1 00 10 0 vvvv 1 01 0 ll 0 u aaa 0x26 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vptestmb " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  1 k b 1 00 10 1 vvvv 1 01 0 ll 0 u aaa 0x26 11 ggg rrr => wit("evex") "vptestmw " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  1 k b 1 00 10 1 vvvv 1 01 0 ll 0 u aaa 0x26 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vptestmw " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  1 k b 1 00 10 0 vvvv 1 10 0 ll 0 u aaa 0x26 11 ggg rrr => wit("evex") "vptestnmb " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  1 k b 1 00 10 0 vvvv 1 10 0 ll 0 u aaa 0x26 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vptestnmb " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  1 k b 1 00 10 1 vvvv 1 10 0 ll 0 u aaa 0x26 11 ggg rrr => wit("evex") "vptestnmw " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  1 k b 1 00 10 1 vvvv 1 10 0 ll 0 u aaa 0x26 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vptestnmw " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  # ==== EVEX broadcasts: lane f/i 32/64 x2/x4/x8, vpbroadcast{b,w}, vpbroadcastm{b2q,w2d} ====
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x19 11 ggg rrr => wit("evex") "vbroadcastf32x2 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x19 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vbroadcastf32x2 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x1a 11 ggg rrr => wit("evex") "vbroadcastf32x4 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x1a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vbroadcastf32x4 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x1b 11 ggg rrr => wit("evex") "vbroadcastf32x8 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x1b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vbroadcastf32x8 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x1a 11 ggg rrr => wit("evex") "vbroadcastf64x2 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x1a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vbroadcastf64x2 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x1b 11 ggg rrr => wit("evex") "vbroadcastf64x4 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x1b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vbroadcastf64x4 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x59 11 ggg rrr => wit("evex") "vbroadcasti32x2 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x59 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vbroadcasti32x2 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x5a 11 ggg rrr => wit("evex") "vbroadcasti32x4 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x5a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vbroadcasti32x4 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vbroadcasti32x8 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x5b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vbroadcasti32x8 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x5a 11 ggg rrr => wit("evex") "vbroadcasti64x2 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x5a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vbroadcasti64x2 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vbroadcasti64x4 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x5b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vbroadcasti64x4 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vpbroadcastb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x78 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpbroadcastb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vpbroadcastw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x79 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpbroadcastw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 1 1111 1 10 z ll 0 1 aaa 0x2a 11 ggg rrr => wit("evex") "vpbroadcastmb2q " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," kreg[$r] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x3a 11 ggg rrr => wit("evex") "vpbroadcastmw2d " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," kreg[$r] ;
  # ==== EVEX scalar<->GPR unsigned converts (mirror vcvtsd2si/vcvtsi2sd templates) ====
  h k b e 00 01 w 1111 1 11 z ll 1 u aaa 0x79 11 ggg rrr {$rexr=1-$h} => wit("evex") "vcvtsd2usi " greg[32*$w+8*$rexr+$g] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 w 1111 1 10 z ll 1 u aaa 0x79 11 ggg rrr {$rexr=1-$h} => wit("evex") "vcvtss2usi " greg[32*$w+8*$rexr+$g] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 w 1111 1 11 z 00 1 u aaa 0x78 11 ggg rrr {$rexr=1-$h} => wit("evex") "vcvttsd2usi " greg[32*$w+8*$rexr+$g] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 01 w 1111 1 10 z 00 1 u aaa 0x78 11 ggg rrr {$rexr=1-$h} => wit("evex") "vcvttss2usi " greg[32*$w+8*$rexr+$g] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 01 w vvvv 1 11 z ll 1 u aaa 0x7b 11 ggg rrr {$rexb=1-$b} => wit("evex") "vcvtusi2sd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," greg[32*$w+8*$rexb+$r] rcdec[$l] ;
  h k b e 00 01 w vvvv 1 10 z ll 1 u aaa 0x7b 11 ggg rrr {$rexb=1-$b} => wit("evex") "vcvtusi2ss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," greg[32*$w+8*$rexb+$r] rcdec[$l] ;
  # ==== EVEX packed converts: unsigned/qq lattice (same / narrow / widen width classes) ====
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x7b 11 ggg rrr => wit("evex") "vcvtpd2qq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 1111 1 01 z ll 1 1 aaa 0x7b 11 ggg rrr => wit("evex") "vcvtpd2qq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x7b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtpd2qq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 1 1111 1 01 z ll 1 1 aaa 0x7b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtpd2qq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 01 1 1111 1 00 z ll 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtpd2udq " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 1111 1 00 z ll 1 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtpd2udq " eregh[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 1 1111 1 00 z ll 0 1 aaa 0x79 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtpd2udq " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 1 1111 1 00 z ll 1 1 aaa 0x79 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtpd2udq " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtpd2uqq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 1111 1 01 z ll 1 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtpd2uqq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x79 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtpd2uqq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 1 1111 1 01 z ll 1 1 aaa 0x79 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtpd2uqq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 01 0 1111 1 01 z ll 0 1 aaa 0x7b 11 ggg rrr => wit("evex") "vcvtps2qq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 01 z ll 1 1 aaa 0x7b 11 ggg rrr => wit("evex") "vcvtps2qq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 0 1111 1 01 z ll 0 1 aaa 0x7b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtps2qq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 01 z ll 1 1 aaa 0x7b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtps2qq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtps2udq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 00 z ll 1 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtps2udq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x79 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtps2udq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 00 z ll 1 1 aaa 0x79 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtps2udq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  h k b e 00 01 0 1111 1 01 z ll 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtps2uqq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 01 z ll 1 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtps2uqq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 0 1111 1 01 z ll 0 1 aaa 0x79 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtps2uqq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 01 z ll 1 1 aaa 0x79 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtps2uqq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  h k b e 00 01 1 1111 1 10 z ll 0 1 aaa 0xe6 11 ggg rrr => wit("evex") "vcvtqq2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 1111 1 10 z ll 1 1 aaa 0xe6 11 ggg rrr => wit("evex") "vcvtqq2pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 1 1111 1 10 z ll 0 1 aaa 0xe6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtqq2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 1 1111 1 10 z ll 1 1 aaa 0xe6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtqq2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 01 1 1111 1 00 z ll 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtqq2ps " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 1111 1 00 z ll 1 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtqq2ps " eregh[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 1 1111 1 00 z ll 0 1 aaa 0x5b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtqq2ps " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 1 1111 1 00 z ll 1 1 aaa 0x5b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtqq2ps " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 01 0 1111 1 11 z ll 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtudq2ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 11 z ll 1 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtudq2ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 0 1111 1 11 z ll 0 1 aaa 0x7a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtudq2ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 11 z ll 1 1 aaa 0x7a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtudq2ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  h k b e 00 01 1 1111 1 10 z ll 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtuqq2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 1111 1 10 z ll 1 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtuqq2pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 1 1111 1 10 z ll 0 1 aaa 0x7a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtuqq2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 1 1111 1 10 z ll 1 1 aaa 0x7a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtuqq2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 01 1 1111 1 11 z ll 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtuqq2ps " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 1111 1 11 z ll 1 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtuqq2ps " eregh[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 01 1 1111 1 11 z ll 0 1 aaa 0x7a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtuqq2ps " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 1 1111 1 11 z ll 1 1 aaa 0x7a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtuqq2ps " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvttpd2qq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 1111 1 01 z 00 1 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvttpd2qq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x7a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttpd2qq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 1 1111 1 01 z ll 1 1 aaa 0x7a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttpd2qq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 01 1 1111 1 00 z ll 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttpd2udq " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 1111 1 00 z 00 1 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttpd2udq " eregh[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 01 1 1111 1 00 z ll 0 1 aaa 0x78 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttpd2udq " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 1 1111 1 00 z ll 1 1 aaa 0x78 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttpd2udq " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttpd2uqq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 1111 1 01 z 00 1 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttpd2uqq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x78 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttpd2uqq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 1 1111 1 01 z ll 1 1 aaa 0x78 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttpd2uqq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 01 0 1111 1 01 z ll 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvttps2qq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 01 z 00 1 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvttps2qq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 01 0 1111 1 01 z ll 0 1 aaa 0x7a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttps2qq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 01 z ll 1 1 aaa 0x7a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttps2qq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttps2udq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 00 z 00 1 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttps2udq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x78 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttps2udq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 00 z ll 1 1 aaa 0x78 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttps2udq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  h k b e 00 01 0 1111 1 01 z ll 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttps2uqq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 01 z 00 1 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttps2uqq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 01 0 1111 1 01 z ll 0 1 aaa 0x78 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttps2uqq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 01 z ll 1 1 aaa 0x78 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttps2uqq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  # ==== EVEX perm RVM: vpermb/w, vperm{i2,t2}{b,w,d,q,ps,pd}, vpermil{ps,pd} (variable) ====
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x8d 11 ggg rrr => wit("evex") "vpermb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x8d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x8d 11 ggg rrr => wit("evex") "vpermw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x8d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x75 11 ggg rrr => wit("evex") "vpermi2b " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x75 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermi2b " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x75 11 ggg rrr => wit("evex") "vpermi2w " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x75 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermi2w " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x76 11 ggg rrr => wit("evex") "vpermi2d " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x76 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermi2d " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x76 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermi2d " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x76 11 ggg rrr => wit("evex") "vpermi2q " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x76 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermi2q " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x76 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermi2q " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x77 11 ggg rrr => wit("evex") "vpermi2ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x77 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermi2ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x77 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermi2ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x77 11 ggg rrr => wit("evex") "vpermi2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x77 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermi2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x77 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermi2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x7d 11 ggg rrr => wit("evex") "vpermt2b " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x7d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermt2b " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x7d 11 ggg rrr => wit("evex") "vpermt2w " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x7d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermt2w " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x7e 11 ggg rrr => wit("evex") "vpermt2d " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x7e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermt2d " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x7e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermt2d " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x7e 11 ggg rrr => wit("evex") "vpermt2q " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x7e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermt2q " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x7e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermt2q " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x7f 11 ggg rrr => wit("evex") "vpermt2ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x7f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermt2ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x7f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermt2ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x7f 11 ggg rrr => wit("evex") "vpermt2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x7f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermt2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x7f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermt2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x0c 11 ggg rrr => wit("evex") "vpermilps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x0c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermilps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 1 u aaa 0x0c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermilps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x0d 11 ggg rrr => wit("evex") "vpermilpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x0d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermilpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 1 u aaa 0x0d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpermilpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  # ==== EVEX map3 imm8 RVMI: vpshld/vpshrd{d,q,w}, vfixupimm{ps,pd,ss,sd} ====
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x70 11 ggg rrr @imm8 => wit("evex") "vpshldw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x70 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpshldw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x71 11 ggg rrr @imm8 => wit("evex") "vpshldd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x71 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpshldd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 1 u aaa 0x71 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpshldd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x71 11 ggg rrr @imm8 => wit("evex") "vpshldq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x71 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpshldq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 1 u aaa 0x71 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpshldq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x72 11 ggg rrr @imm8 => wit("evex") "vpshrdw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x72 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpshrdw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x73 11 ggg rrr @imm8 => wit("evex") "vpshrdd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x73 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpshrdd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 1 u aaa 0x73 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpshrdd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x73 11 ggg rrr @imm8 => wit("evex") "vpshrdq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x73 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpshrdq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 1 u aaa 0x73 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpshrdq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x54 11 ggg rrr @imm8 => wit("evex") "vfixupimmps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x54 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vfixupimmps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 1 u aaa 0x54 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vfixupimmps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x54 11 ggg rrr @imm8 => wit("evex") "vfixupimmpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x54 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vfixupimmpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 1 u aaa 0x54 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vfixupimmpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x55 11 ggg rrr @imm8 => wit("evex") "vfixupimmss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x55 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vfixupimmss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x55 11 ggg rrr @imm8 => wit("evex") "vfixupimmsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x55 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vfixupimmsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  # ==== EVEX vprord/vprorq (0F72 group /0) + vcmpsd/vcmpss (scalar cc -> kreg) ====
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x72 11 000 rrr @imm8 => wit("evex") "vprord " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 01 0 vvvv 1 01 z ll 0 u aaa 0x72 @addr(0) {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vprord " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 01 0 vvvv 1 01 z ll 1 u aaa 0x72 @addr(0) {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vprord " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," $addr bcst32[$l] "," hex($imm8) ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x72 11 000 rrr @imm8 => wit("evex") "vprorq " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x72 @addr(0) {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vprorq " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0x72 @addr(0) {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vprorq " evvv[32*$l+16*$u+$v] kzdec[$z*8+$a] "," $addr bcst64[$l] "," hex($imm8) ;
  1 k b 1 00 01 1 vvvv 1 11 0 ll 0 u aaa 0xc2 11 ggg rrr @imm8 => wit("evex") "vcmpsd " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  1 k b 1 00 01 1 vvvv 1 11 0 ll 0 u aaa 0xc2 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vcmpsd " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  1 k b 1 00 01 0 vvvv 1 10 0 ll 0 u aaa 0xc2 11 ggg rrr @imm8 => wit("evex") "vcmpss " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  1 k b 1 00 01 0 vvvv 1 10 0 ll 0 u aaa 0xc2 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vcmpss " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  # ==== EVEX compress / expand / align: valign[dq], v(p)expand*, v(p)compress* ====
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x03 11 ggg rrr @imm8 => wit("evex") "valignd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x03 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "valignd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 1 u aaa 0x03 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "valignd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x03 11 ggg rrr @imm8 => wit("evex") "valignq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x03 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "valignq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 1 u aaa 0x03 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "valignq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] "," hex($imm8) ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x88 11 ggg rrr => wit("evex") "vexpandps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x88 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vexpandps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x88 11 ggg rrr => wit("evex") "vexpandpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x88 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vexpandpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x89 11 ggg rrr => wit("evex") "vpexpandd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x89 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpexpandd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x89 11 ggg rrr => wit("evex") "vpexpandq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x89 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpexpandq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x62 11 ggg rrr => wit("evex") "vpexpandb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x62 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpexpandb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x62 11 ggg rrr => wit("evex") "vpexpandw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x62 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpexpandw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x8a 11 ggg rrr => wit("evex") "vcompressps " ereg[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x8a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcompressps " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x8a 11 ggg rrr => wit("evex") "vcompresspd " ereg[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x8a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcompresspd " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x8b 11 ggg rrr => wit("evex") "vpcompressd " ereg[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x8b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpcompressd " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x8b 11 ggg rrr => wit("evex") "vpcompressq " ereg[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x8b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpcompressq " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x63 11 ggg rrr => wit("evex") "vpcompressb " ereg[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x63 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpcompressb " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x63 11 ggg rrr => wit("evex") "vpcompressw " ereg[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0x63 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpcompressw " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  # ==== EVEX transcendental/range/fpclass: getexp/getmant/range/reduce/rcp28/rsqrt28/fpclass ====
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x43 11 ggg rrr => wit("evex") "vgetexpsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z 00 1 u aaa 0x43 11 ggg rrr => wit("evex") "vgetexpsd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0x43 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vgetexpsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x43 11 ggg rrr => wit("evex") "vgetexpss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z 00 1 u aaa 0x43 11 ggg rrr => wit("evex") "vgetexpss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x43 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vgetexpss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xcb 11 ggg rrr => wit("evex") "vrcp28sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z 00 1 u aaa 0xcb 11 ggg rrr => wit("evex") "vrcp28sd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xcb @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrcp28sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xcb 11 ggg rrr => wit("evex") "vrcp28ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z 00 1 u aaa 0xcb 11 ggg rrr => wit("evex") "vrcp28ss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xcb @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrcp28ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xcd 11 ggg rrr => wit("evex") "vrsqrt28sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 vvvv 1 01 z 00 1 u aaa 0xcd 11 ggg rrr => wit("evex") "vrsqrt28sd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 10 1 vvvv 1 01 z ll 0 u aaa 0xcd @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrsqrt28sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xcd 11 ggg rrr => wit("evex") "vrsqrt28ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z 00 1 u aaa 0xcd 11 ggg rrr => wit("evex") "vrsqrt28ss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0xcd @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrsqrt28ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x27 11 ggg rrr @imm8 => wit("evex") "vgetmantsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z 00 1 u aaa 0x27 11 ggg rrr @imm8 => wit("evex") "vgetmantsd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x27 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vgetmantsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x27 11 ggg rrr @imm8 => wit("evex") "vgetmantss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z 00 1 u aaa 0x27 11 ggg rrr @imm8 => wit("evex") "vgetmantss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x27 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vgetmantss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x51 11 ggg rrr @imm8 => wit("evex") "vrangesd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z 00 1 u aaa 0x51 11 ggg rrr @imm8 => wit("evex") "vrangesd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x51 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vrangesd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x51 11 ggg rrr @imm8 => wit("evex") "vrangess " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z 00 1 u aaa 0x51 11 ggg rrr @imm8 => wit("evex") "vrangess " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x51 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vrangess " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x57 11 ggg rrr @imm8 => wit("evex") "vreducesd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z 00 1 u aaa 0x57 11 ggg rrr @imm8 => wit("evex") "vreducesd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x57 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vreducesd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x57 11 ggg rrr @imm8 => wit("evex") "vreducess " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z 00 1 u aaa 0x57 11 ggg rrr @imm8 => wit("evex") "vreducess " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x57 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vreducess " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x50 11 ggg rrr @imm8 => wit("evex") "vrangepd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z 00 1 u aaa 0x50 11 ggg rrr @imm8 => wit("evex") "vrangepd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x50 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vrangepd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 1 u aaa 0x50 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vrangepd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x50 11 ggg rrr @imm8 => wit("evex") "vrangeps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z 00 1 u aaa 0x50 11 ggg rrr @imm8 => wit("evex") "vrangeps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x50 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vrangeps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 1 u aaa 0x50 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vrangeps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] "," hex($imm8) ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0xca 11 ggg rrr => wit("evex") "vrcp28pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 1111 1 01 z 00 1 1 aaa 0xca 11 ggg rrr => wit("evex") "vrcp28pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0xca @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrcp28pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 1 1111 1 01 z ll 1 1 aaa 0xca @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrcp28pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0xca 11 ggg rrr => wit("evex") "vrcp28ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z 00 1 1 aaa 0xca 11 ggg rrr => wit("evex") "vrcp28ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0xca @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrcp28ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 1 1 aaa 0xca @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrcp28ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0xcc 11 ggg rrr => wit("evex") "vrsqrt28pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 1111 1 01 z 00 1 1 aaa 0xcc 11 ggg rrr => wit("evex") "vrsqrt28pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0xcc @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrsqrt28pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 1 1111 1 01 z ll 1 1 aaa 0xcc @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrsqrt28pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0xcc 11 ggg rrr => wit("evex") "vrsqrt28ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z 00 1 1 aaa 0xcc 11 ggg rrr => wit("evex") "vrsqrt28ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0xcc @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrsqrt28ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 1 1 aaa 0xcc @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrsqrt28ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  h k b e 00 11 1 1111 1 01 z ll 0 1 aaa 0x56 11 ggg rrr @imm8 => wit("evex") "vreducepd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z 00 1 1 aaa 0x56 11 ggg rrr @imm8 => wit("evex") "vreducepd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z ll 0 1 aaa 0x56 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vreducepd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 1111 1 01 z ll 1 1 aaa 0x56 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vreducepd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] "," hex($imm8) ;
  h k b e 00 11 0 1111 1 01 z ll 0 1 aaa 0x56 11 ggg rrr @imm8 => wit("evex") "vreduceps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 1111 1 01 z 00 1 1 aaa 0x56 11 ggg rrr @imm8 => wit("evex") "vreduceps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" "," hex($imm8) ;
  h k b e 00 11 0 1111 1 01 z ll 0 1 aaa 0x56 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vreduceps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 1111 1 01 z ll 1 1 aaa 0x56 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vreduceps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] "," hex($imm8) ;
  1 k b 1 00 11 1 1111 1 01 0 ll 0 1 aaa 0x66 11 ggg rrr @imm8 => wit("evex") "vfpclasspd " kreg[$g] kdec[$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  1 k b 1 00 11 1 1111 1 01 0 ll 0 1 aaa 0x66 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vfpclasspd " kreg[$g] kdec[$a] "," $addr "," hex($imm8) ;
  1 k b 1 00 11 1 1111 1 01 0 ll 1 1 aaa 0x66 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vfpclasspd " kreg[$g] kdec[$a] "," $addr bcst64[$l] "," hex($imm8) ;
  1 k b 1 00 11 0 1111 1 01 0 ll 0 1 aaa 0x66 11 ggg rrr @imm8 => wit("evex") "vfpclassps " kreg[$g] kdec[$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  1 k b 1 00 11 0 1111 1 01 0 ll 0 1 aaa 0x66 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vfpclassps " kreg[$g] kdec[$a] "," $addr "," hex($imm8) ;
  1 k b 1 00 11 0 1111 1 01 0 ll 1 1 aaa 0x66 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vfpclassps " kreg[$g] kdec[$a] "," $addr bcst32[$l] "," hex($imm8) ;
  1 k b 1 00 11 1 1111 1 01 0 ll 0 1 aaa 0x67 11 ggg rrr @imm8 => wit("evex") "vfpclasssd " kreg[$g] kdec[$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  1 k b 1 00 11 1 1111 1 01 0 ll 0 1 aaa 0x67 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vfpclasssd " kreg[$g] kdec[$a] "," $addr "," hex($imm8) ;
  1 k b 1 00 11 0 1111 1 01 0 ll 0 1 aaa 0x67 11 ggg rrr @imm8 => wit("evex") "vfpclassss " kreg[$g] kdec[$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  1 k b 1 00 11 0 1111 1 01 0 ll 0 1 aaa 0x67 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vfpclassss " kreg[$g] kdec[$a] "," $addr "," hex($imm8) ;
  # ==== EVEX insert/shuffle imm8: vinsert{f,i}*x*, vshuf{f,i}*x*, vshuf{ps,pd}, vpshuf{d,hw,lw,b} ====
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x18 11 ggg rrr @imm8 => wit("evex") "vinsertf32x4 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," eregx[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x18 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vinsertf32x4 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x18 11 ggg rrr @imm8 => wit("evex") "vinsertf64x2 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," eregx[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x18 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vinsertf64x2 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x1a 11 ggg rrr @imm8 => wit("evex") "vinsertf32x8 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," eregh[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x1a @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vinsertf32x8 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x1a 11 ggg rrr @imm8 => wit("evex") "vinsertf64x4 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," eregh[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x1a @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vinsertf64x4 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x38 11 ggg rrr @imm8 => wit("evex") "vinserti32x4 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," eregx[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x38 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vinserti32x4 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x38 11 ggg rrr @imm8 => wit("evex") "vinserti64x2 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," eregx[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x38 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vinserti64x2 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x3a 11 ggg rrr @imm8 => wit("evex") "vinserti32x8 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," eregh[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x3a @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vinserti32x8 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x3a 11 ggg rrr @imm8 => wit("evex") "vinserti64x4 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," eregh[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x3a @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vinserti64x4 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x23 11 ggg rrr @imm8 => wit("evex") "vshuff32x4 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x23 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vshuff32x4 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 1 u aaa 0x23 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vshuff32x4 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x23 11 ggg rrr @imm8 => wit("evex") "vshuff64x2 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x23 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vshuff64x2 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 1 u aaa 0x23 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vshuff64x2 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x43 11 ggg rrr @imm8 => wit("evex") "vshufi32x4 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x43 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vshufi32x4 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 1 u aaa 0x43 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vshufi32x4 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x43 11 ggg rrr @imm8 => wit("evex") "vshufi64x2 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 0 u aaa 0x43 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vshufi64x2 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 1 vvvv 1 01 z ll 1 u aaa 0x43 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vshufi64x2 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] "," hex($imm8) ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0xc6 11 ggg rrr @imm8 => wit("evex") "vshufps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0xc6 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vshufps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 01 0 vvvv 1 00 z ll 1 u aaa 0xc6 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vshufps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] "," hex($imm8) ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0xc6 11 ggg rrr @imm8 => wit("evex") "vshufpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0xc6 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vshufpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0xc6 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vshufpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] "," hex($imm8) ;
  h k b e 00 01 0 1111 1 01 z ll 0 1 aaa 0x70 11 ggg rrr @imm8 => wit("evex") "vpshufd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 01 0 1111 1 01 z ll 0 1 aaa 0x70 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpshufd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 01 0 1111 1 01 z ll 1 1 aaa 0x70 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpshufd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] "," hex($imm8) ;
  h k b e 00 01 0 1111 1 10 z ll 0 1 aaa 0x70 11 ggg rrr @imm8 => wit("evex") "vpshufhw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 01 0 1111 1 10 z ll 0 1 aaa 0x70 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpshufhw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 01 0 1111 1 11 z ll 0 1 aaa 0x70 11 ggg rrr @imm8 => wit("evex") "vpshuflw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 01 0 1111 1 11 z ll 0 1 aaa 0x70 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpshuflw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x00 11 ggg rrr => wit("evex") "vpshufb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 01 z ll 0 u aaa 0x00 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpshufb " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  # ==== EVEX fp move/scalar/unpack: vcomis/vucomis, vmax/vmin s, vunpck, vmovdup, vmovnt* ====
  h k b e 00 01 0 1111 1 01 z ll 0 1 aaa 0x2f 11 ggg rrr => wit("evex") "vcomisd " ereg[32*$l+16*$e+8*$h+$g] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 01 z ll 0 1 aaa 0x2f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcomisd " ereg[32*$l+16*$e+8*$h+$g] "," $addr ;
  h k b e 00 01 0 1111 1 01 z 00 1 1 aaa 0x2f 11 ggg rrr => wit("evex") "vcomisd " ereg[64+16*$e+8*$h+$g] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x2f 11 ggg rrr => wit("evex") "vcomiss " ereg[32*$l+16*$e+8*$h+$g] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x2f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcomiss " ereg[32*$l+16*$e+8*$h+$g] "," $addr ;
  h k b e 00 01 0 1111 1 00 z 00 1 1 aaa 0x2f 11 ggg rrr => wit("evex") "vcomiss " ereg[64+16*$e+8*$h+$g] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 01 0 1111 1 01 z ll 0 1 aaa 0x2e 11 ggg rrr => wit("evex") "vucomisd " ereg[32*$l+16*$e+8*$h+$g] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 01 z ll 0 1 aaa 0x2e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vucomisd " ereg[32*$l+16*$e+8*$h+$g] "," $addr ;
  h k b e 00 01 0 1111 1 01 z 00 1 1 aaa 0x2e 11 ggg rrr => wit("evex") "vucomisd " ereg[64+16*$e+8*$h+$g] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x2e 11 ggg rrr => wit("evex") "vucomiss " ereg[32*$l+16*$e+8*$h+$g] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x2e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vucomiss " ereg[32*$l+16*$e+8*$h+$g] "," $addr ;
  h k b e 00 01 0 1111 1 00 z 00 1 1 aaa 0x2e 11 ggg rrr => wit("evex") "vucomiss " ereg[64+16*$e+8*$h+$g] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 01 1 vvvv 1 11 z ll 0 u aaa 0x5f 11 ggg rrr => wit("evex") "vmaxsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 11 z ll 0 u aaa 0x5f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmaxsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 11 z 00 1 u aaa 0x5f 11 ggg rrr => wit("evex") "vmaxsd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 01 0 vvvv 1 10 z ll 0 u aaa 0x5f 11 ggg rrr => wit("evex") "vmaxss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 10 z ll 0 u aaa 0x5f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmaxss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 10 z 00 1 u aaa 0x5f 11 ggg rrr => wit("evex") "vmaxss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 01 1 vvvv 1 11 z ll 0 u aaa 0x5d 11 ggg rrr => wit("evex") "vminsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 11 z ll 0 u aaa 0x5d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vminsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 11 z 00 1 u aaa 0x5d 11 ggg rrr => wit("evex") "vminsd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 01 0 vvvv 1 10 z ll 0 u aaa 0x5d 11 ggg rrr => wit("evex") "vminss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 10 z ll 0 u aaa 0x5d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vminss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 10 z 00 1 u aaa 0x5d 11 ggg rrr => wit("evex") "vminss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x15 11 ggg rrr => wit("evex") "vunpckhpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x15 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vunpckhpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0x15 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vunpckhpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x15 11 ggg rrr => wit("evex") "vunpckhps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x15 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vunpckhps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 00 z ll 1 u aaa 0x15 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vunpckhps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x14 11 ggg rrr => wit("evex") "vunpcklpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 vvvv 1 01 z ll 0 u aaa 0x14 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vunpcklpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 1 vvvv 1 01 z ll 1 u aaa 0x14 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vunpcklpd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x14 11 ggg rrr => wit("evex") "vunpcklps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 vvvv 1 00 z ll 0 u aaa 0x14 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vunpcklps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 01 0 vvvv 1 00 z ll 1 u aaa 0x14 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vunpcklps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 01 1 1111 1 11 z ll 0 1 aaa 0x12 11 ggg rrr => wit("evex") "vmovddup " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 1111 1 11 z ll 0 1 aaa 0x12 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovddup " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 10 z ll 0 1 aaa 0x16 11 ggg rrr => wit("evex") "vmovshdup " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 10 z ll 0 1 aaa 0x16 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovshdup " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 10 z ll 0 1 aaa 0x12 11 ggg rrr => wit("evex") "vmovsldup " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 10 z ll 0 1 aaa 0x12 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovsldup " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 01 z ll 0 1 aaa 0xe7 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovntdq " $addr "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 1 1111 1 01 z ll 0 1 aaa 0x2b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovntpd " $addr "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 0 1111 1 00 z ll 0 1 aaa 0x2b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovntps " $addr "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x2a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovntdqa " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 vvvv 1 10 z ll 0 u aaa 0x10 11 ggg rrr => wit("evex") "vmovss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 0 1111 1 10 z ll 0 1 aaa 0x10 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 0 1111 1 10 z ll 0 1 aaa 0x11 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovss " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 01 1 vvvv 1 11 z ll 0 u aaa 0x10 11 ggg rrr => wit("evex") "vmovsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 01 1 1111 1 11 z ll 0 1 aaa 0x10 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovsd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 01 1 1111 1 11 z ll 0 1 aaa 0x11 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovsd " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  # ==== EVEX mask conversions + vdbpsadbw + vexp2 ====
  1 k b 1 00 10 0 1111 1 10 0 ll 0 1 aaa 0x29 11 ggg rrr => wit("evex") "vpmovb2m " kreg[$g] "," ereg[32*$l+16*$k+8*$b+$r] ;
  1 k b 1 00 10 1 1111 1 10 0 ll 0 1 aaa 0x29 11 ggg rrr => wit("evex") "vpmovw2m " kreg[$g] "," ereg[32*$l+16*$k+8*$b+$r] ;
  1 k b 1 00 10 0 1111 1 10 0 ll 0 1 aaa 0x39 11 ggg rrr => wit("evex") "vpmovd2m " kreg[$g] "," ereg[32*$l+16*$k+8*$b+$r] ;
  1 k b 1 00 10 1 1111 1 10 0 ll 0 1 aaa 0x39 11 ggg rrr => wit("evex") "vpmovq2m " kreg[$g] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x28 11 ggg rrr => wit("evex") "vpmovm2b " ereg[32*$l+16*$e+8*$h+$g] "," kreg[$r] ;
  h k b e 00 10 1 1111 1 10 z ll 0 1 aaa 0x28 11 ggg rrr => wit("evex") "vpmovm2w " ereg[32*$l+16*$e+8*$h+$g] "," kreg[$r] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x38 11 ggg rrr => wit("evex") "vpmovm2d " ereg[32*$l+16*$e+8*$h+$g] "," kreg[$r] ;
  h k b e 00 10 1 1111 1 10 z ll 0 1 aaa 0x38 11 ggg rrr => wit("evex") "vpmovm2q " ereg[32*$l+16*$e+8*$h+$g] "," kreg[$r] ;
  1 k b 1 00 10 0 vvvv 1 11 0 ll 0 u aaa 0x68 11 ggg rrr => wit("evex") "vp2intersectd " kreg[$g] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  1 k b 1 00 10 0 vvvv 1 11 0 ll 0 u aaa 0x68 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vp2intersectd " kreg[$g] "," evvv[32*$l+16*$u+$v] "," $addr ;
  1 k b 1 00 10 0 vvvv 1 11 1 u aaa 0x68 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vp2intersectd " kreg[$g] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  1 k b 1 00 10 1 vvvv 1 11 0 ll 0 u aaa 0x68 11 ggg rrr => wit("evex") "vp2intersectq " kreg[$g] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  1 k b 1 00 10 1 vvvv 1 11 0 ll 0 u aaa 0x68 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vp2intersectq " kreg[$g] "," evvv[32*$l+16*$u+$v] "," $addr ;
  1 k b 1 00 10 0 vvvv 1 01 0 ll 0 u aaa 0x8f 11 ggg rrr => wit("evex") "vpshufbitqmb " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  1 k b 1 00 10 0 vvvv 1 01 0 ll 0 u aaa 0x8f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vpshufbitqmb " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x42 11 ggg rrr @imm8 => wit("evex") "vdbpsadbw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 01 z ll 0 u aaa 0x42 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vdbpsadbw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0xc8 11 ggg rrr => wit("evex") "vexp2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 1 1111 1 01 z ll 0 1 aaa 0xc8 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vexp2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 1 1111 1 01 z ll 1 1 aaa 0xc8 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vexp2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0xc8 11 ggg rrr => wit("evex") "vexp2ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0xc8 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vexp2ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 01 z ll 1 1 aaa 0xc8 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vexp2ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  # ==== EVEX vpcmp{b,w,ub,uw,ud,uq} imm8 -> kreg (generic cc; vpcmpd/q already covered) ====
  1 k b 1 00 11 0 vvvv 1 01 0 ll 0 u aaa 0x1e 11 ggg rrr @imm8 => wit("evex") "vpcmpud " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  1 k b 1 00 11 0 vvvv 1 01 0 ll 0 u aaa 0x1e @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpcmpud " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  1 k b 1 00 11 0 vvvv 1 01 0 ll 1 u aaa 0x1e @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpcmpud " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] "," hex($imm8) ;
  1 k b 1 00 11 1 vvvv 1 01 0 ll 0 u aaa 0x1e 11 ggg rrr @imm8 => wit("evex") "vpcmpuq " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  1 k b 1 00 11 1 vvvv 1 01 0 ll 0 u aaa 0x1e @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpcmpuq " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  1 k b 1 00 11 1 vvvv 1 01 0 ll 1 u aaa 0x1e @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpcmpuq " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst64[$l] "," hex($imm8) ;
  1 k b 1 00 11 0 vvvv 1 01 0 ll 0 u aaa 0x3e 11 ggg rrr @imm8 => wit("evex") "vpcmpub " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  1 k b 1 00 11 0 vvvv 1 01 0 ll 0 u aaa 0x3e @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpcmpub " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  1 k b 1 00 11 1 vvvv 1 01 0 ll 0 u aaa 0x3e 11 ggg rrr @imm8 => wit("evex") "vpcmpuw " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  1 k b 1 00 11 1 vvvv 1 01 0 ll 0 u aaa 0x3e @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpcmpuw " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  1 k b 1 00 11 0 vvvv 1 01 0 ll 0 u aaa 0x3f 11 ggg rrr @imm8 => wit("evex") "vpcmpb " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  1 k b 1 00 11 0 vvvv 1 01 0 ll 0 u aaa 0x3f @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpcmpb " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  1 k b 1 00 11 1 vvvv 1 01 0 ll 0 u aaa 0x3f 11 ggg rrr @imm8 => wit("evex") "vpcmpw " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  1 k b 1 00 11 1 vvvv 1 01 0 ll 0 u aaa 0x3f @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vpcmpw " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  # ==== EVEX ph/bf16 converts: vcvtph2ps (widen), vcvtps2ph (MRI store), vcvtne*2bf16 ====
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x13 11 ggg rrr => wit("evex") "vcvtph2ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 01 z 00 1 1 aaa 0x13 11 ggg rrr => wit("evex") "vcvtph2ps " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 10 0 1111 1 01 z ll 0 1 aaa 0x13 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtph2ps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 11 0 1111 1 01 z ll 0 1 aaa 0x1d 11 ggg rrr @imm8 => wit("evex") "vcvtps2ph " eregh[32*$l+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] "," hex($imm8) ;
  h k b e 00 11 0 1111 1 01 z 00 1 1 aaa 0x1d 11 ggg rrr @imm8 => wit("evex") "vcvtps2ph " eregh[64+16*$k+8*$b+$r] kzdec[$z*8+$a] "," ereg[64+16*$e+8*$h+$g] " {sae}" "," hex($imm8) ;
  h k b e 00 11 0 1111 1 01 z ll 0 1 aaa 0x1d @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vcvtps2ph " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] "," hex($imm8) ;
  h k b e 00 10 0 vvvv 1 11 z ll 0 u aaa 0x72 11 ggg rrr => wit("evex") "vcvtne2ps2bf16 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 vvvv 1 11 z ll 0 u aaa 0x72 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtne2ps2bf16 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 11 z ll 1 u aaa 0x72 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtne2ps2bf16 " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x72 11 ggg rrr => wit("evex") "vcvtneps2bf16 " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 10 0 1111 1 10 z ll 0 1 aaa 0x72 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtneps2bf16 " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 10 0 1111 1 10 z ll 1 1 aaa 0x72 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtneps2bf16 " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  # ==== EVEX AVX512-FP16 map3: vfpclass/vgetmant/vreduce/vrndscale ph/sh + vcmpph/sh ====
  1 k b 1 00 11 0 1111 1 00 0 ll 0 1 aaa 0x66 11 ggg rrr @imm8 => wit("evex") "vfpclassph " kreg[$g] kdec[$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  1 k b 1 00 11 0 1111 1 00 0 ll 0 1 aaa 0x66 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vfpclassph " kreg[$g] kdec[$a] "," $addr "," hex($imm8) ;
  1 k b 1 00 11 0 1111 1 00 0 ll 0 1 aaa 0x67 11 ggg rrr @imm8 => wit("evex") "vfpclasssh " kreg[$g] kdec[$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  1 k b 1 00 11 0 1111 1 00 0 ll 0 1 aaa 0x67 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vfpclasssh " kreg[$g] kdec[$a] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 1111 1 00 z ll 0 1 aaa 0x26 11 ggg rrr @imm8 => wit("evex") "vgetmantph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 1111 1 00 z 00 1 1 aaa 0x26 11 ggg rrr @imm8 => wit("evex") "vgetmantph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" "," hex($imm8) ;
  h k b e 00 11 0 1111 1 00 z ll 0 1 aaa 0x26 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vgetmantph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 00 z ll 0 u aaa 0x27 11 ggg rrr @imm8 => wit("evex") "vgetmantsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 00 z 00 1 u aaa 0x27 11 ggg rrr @imm8 => wit("evex") "vgetmantsh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 00 z ll 0 u aaa 0x27 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vgetmantsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 1111 1 00 z ll 0 1 aaa 0x56 11 ggg rrr @imm8 => wit("evex") "vreduceph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 1111 1 00 z 00 1 1 aaa 0x56 11 ggg rrr @imm8 => wit("evex") "vreduceph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" "," hex($imm8) ;
  h k b e 00 11 0 1111 1 00 z ll 0 1 aaa 0x56 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vreduceph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 00 z ll 0 u aaa 0x57 11 ggg rrr @imm8 => wit("evex") "vreducesh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 00 z 00 1 u aaa 0x57 11 ggg rrr @imm8 => wit("evex") "vreducesh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 00 z ll 0 u aaa 0x57 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vreducesh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 1111 1 00 z ll 0 1 aaa 0x08 11 ggg rrr @imm8 => wit("evex") "vrndscaleph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 1111 1 00 z 00 1 1 aaa 0x08 11 ggg rrr @imm8 => wit("evex") "vrndscaleph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" "," hex($imm8) ;
  h k b e 00 11 0 1111 1 00 z ll 0 1 aaa 0x08 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vrndscaleph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 00 z ll 0 u aaa 0x0a 11 ggg rrr @imm8 => wit("evex") "vrndscalesh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 00 z 00 1 u aaa 0x0a 11 ggg rrr @imm8 => wit("evex") "vrndscalesh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" "," hex($imm8) ;
  h k b e 00 11 0 vvvv 1 00 z ll 0 u aaa 0x0a @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vrndscalesh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  1 k b 1 00 11 0 vvvv 1 00 0 ll 0 u aaa 0xc2 11 ggg rrr @imm8 => wit("evex") "vcmpph " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  1 k b 1 00 11 0 vvvv 1 00 0 ll 0 u aaa 0xc2 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vcmpph " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  1 k b 1 00 11 0 vvvv 1 10 0 ll 0 u aaa 0xc2 11 ggg rrr @imm8 => wit("evex") "vcmpsh " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] "," hex($imm8) ;
  1 k b 1 00 11 0 vvvv 1 10 0 ll 0 u aaa 0xc2 @addr {$rexb=1-$b;$rexx=1-$k} @imm8 => wit("evex") "vcmpsh " kreg[$g] kdec[$a] "," evvv[32*$l+16*$u+$v] "," $addr "," hex($imm8) ;
  # ==== EVEX 4-op (AVX512-4FMAPS/4VNNIW): v4fmadd*/v4fnmadd*/vp4dpwssd* (RVM, mem-only) ====
  h k b e 00 10 0 vvvv 1 11 z ll 0 u aaa 0x9a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "v4fmaddps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 11 z ll 0 u aaa 0x9b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "v4fmaddss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 11 z ll 0 u aaa 0xaa @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "v4fnmaddps " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 11 z ll 0 u aaa 0xab @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "v4fnmaddss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 11 z ll 0 u aaa 0x52 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vp4dpwssd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 10 0 vvvv 1 11 z ll 0 u aaa 0x53 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vp4dpwssds " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  # ==== AVX512-FP16 maps 5/6: arithmetic/sqrt/comis/fma/complex/scalef/getexp/rcp/rsqrt/converts ====
  h k b e 00 101 0 vvvv 1 00 z ll 0 u aaa 0x58 11 ggg rrr => wit("evex") "vaddph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 vvvv 1 00 z ll 1 u aaa 0x58 11 ggg rrr => wit("evex") "vaddph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 0 vvvv 1 00 z ll 0 u aaa 0x58 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vaddph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 101 0 vvvv 1 00 z ll 1 u aaa 0x58 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vaddph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 101 0 vvvv 1 00 z ll 0 u aaa 0x5c 11 ggg rrr => wit("evex") "vsubph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 vvvv 1 00 z ll 1 u aaa 0x5c 11 ggg rrr => wit("evex") "vsubph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 0 vvvv 1 00 z ll 0 u aaa 0x5c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vsubph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 101 0 vvvv 1 00 z ll 1 u aaa 0x5c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vsubph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 101 0 vvvv 1 00 z ll 0 u aaa 0x59 11 ggg rrr => wit("evex") "vmulph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 vvvv 1 00 z ll 1 u aaa 0x59 11 ggg rrr => wit("evex") "vmulph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 0 vvvv 1 00 z ll 0 u aaa 0x59 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmulph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 101 0 vvvv 1 00 z ll 1 u aaa 0x59 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmulph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 101 0 vvvv 1 00 z ll 0 u aaa 0x5e 11 ggg rrr => wit("evex") "vdivph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 vvvv 1 00 z ll 1 u aaa 0x5e 11 ggg rrr => wit("evex") "vdivph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 0 vvvv 1 00 z ll 0 u aaa 0x5e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vdivph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 101 0 vvvv 1 00 z ll 1 u aaa 0x5e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vdivph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 101 0 vvvv 1 00 z ll 0 u aaa 0x5f 11 ggg rrr => wit("evex") "vmaxph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 vvvv 1 00 z ll 1 u aaa 0x5f 11 ggg rrr => wit("evex") "vmaxph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 101 0 vvvv 1 00 z ll 0 u aaa 0x5f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmaxph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 101 0 vvvv 1 00 z ll 1 u aaa 0x5f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmaxph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 101 0 vvvv 1 00 z ll 0 u aaa 0x5d 11 ggg rrr => wit("evex") "vminph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 vvvv 1 00 z ll 1 u aaa 0x5d 11 ggg rrr => wit("evex") "vminph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 101 0 vvvv 1 00 z ll 0 u aaa 0x5d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vminph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 101 0 vvvv 1 00 z ll 1 u aaa 0x5d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vminph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 101 0 vvvv 1 10 z ll 0 u aaa 0x58 11 ggg rrr => wit("evex") "vaddsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 vvvv 1 10 z ll 1 u aaa 0x58 11 ggg rrr => wit("evex") "vaddsh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 0 vvvv 1 10 z ll 0 u aaa 0x58 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vaddsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 101 0 vvvv 1 10 z ll 0 u aaa 0x5c 11 ggg rrr => wit("evex") "vsubsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 vvvv 1 10 z ll 1 u aaa 0x5c 11 ggg rrr => wit("evex") "vsubsh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 0 vvvv 1 10 z ll 0 u aaa 0x5c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vsubsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 101 0 vvvv 1 10 z ll 0 u aaa 0x59 11 ggg rrr => wit("evex") "vmulsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 vvvv 1 10 z ll 1 u aaa 0x59 11 ggg rrr => wit("evex") "vmulsh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 0 vvvv 1 10 z ll 0 u aaa 0x59 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmulsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 101 0 vvvv 1 10 z ll 0 u aaa 0x5e 11 ggg rrr => wit("evex") "vdivsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 vvvv 1 10 z ll 1 u aaa 0x5e 11 ggg rrr => wit("evex") "vdivsh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 0 vvvv 1 10 z ll 0 u aaa 0x5e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vdivsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 101 0 vvvv 1 10 z ll 0 u aaa 0x5f 11 ggg rrr => wit("evex") "vmaxsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 vvvv 1 10 z ll 1 u aaa 0x5f 11 ggg rrr => wit("evex") "vmaxsh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 101 0 vvvv 1 10 z ll 0 u aaa 0x5f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmaxsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 101 0 vvvv 1 10 z ll 0 u aaa 0x5d 11 ggg rrr => wit("evex") "vminsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 vvvv 1 10 z ll 1 u aaa 0x5d 11 ggg rrr => wit("evex") "vminsh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 101 0 vvvv 1 10 z ll 0 u aaa 0x5d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vminsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 101 0 1111 1 00 z ll 0 1 aaa 0x51 11 ggg rrr => wit("evex") "vsqrtph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 1111 1 00 z ll 1 1 aaa 0x51 11 ggg rrr => wit("evex") "vsqrtph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 0 1111 1 00 z ll 0 1 aaa 0x51 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vsqrtph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 0 1111 1 00 z ll 1 1 aaa 0x51 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vsqrtph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst16[$l] ;
  h k b e 00 101 0 vvvv 1 10 z ll 0 u aaa 0x51 11 ggg rrr => wit("evex") "vsqrtsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 vvvv 1 10 z ll 1 u aaa 0x51 11 ggg rrr => wit("evex") "vsqrtsh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 0 vvvv 1 10 z ll 0 u aaa 0x51 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vsqrtsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 101 0 1111 1 00 z ll 0 1 aaa 0x2f 11 ggg rrr => wit("evex") "vcomish " ereg[32*$l+16*$e+8*$h+$g] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 1111 1 00 z ll 1 1 aaa 0x2f 11 ggg rrr => wit("evex") "vcomish " ereg[64+16*$e+8*$h+$g] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 101 0 1111 1 00 z ll 0 1 aaa 0x2f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcomish " ereg[32*$l+16*$e+8*$h+$g] "," $addr ;
  h k b e 00 101 0 1111 1 00 z ll 0 1 aaa 0x2e 11 ggg rrr => wit("evex") "vucomish " ereg[32*$l+16*$e+8*$h+$g] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 1111 1 00 z ll 1 1 aaa 0x2e 11 ggg rrr => wit("evex") "vucomish " ereg[64+16*$e+8*$h+$g] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 101 0 1111 1 00 z ll 0 1 aaa 0x2e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vucomish " ereg[32*$l+16*$e+8*$h+$g] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x98 11 ggg rrr => wit("evex") "vfmadd132ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0x98 11 ggg rrr => wit("evex") "vfmadd132ph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x98 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd132ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0x98 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd132ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xa8 11 ggg rrr => wit("evex") "vfmadd213ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xa8 11 ggg rrr => wit("evex") "vfmadd213ph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xa8 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd213ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xa8 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd213ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xb8 11 ggg rrr => wit("evex") "vfmadd231ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xb8 11 ggg rrr => wit("evex") "vfmadd231ph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xb8 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd231ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xb8 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd231ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x9a 11 ggg rrr => wit("evex") "vfmsub132ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0x9a 11 ggg rrr => wit("evex") "vfmsub132ph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x9a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub132ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0x9a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub132ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xaa 11 ggg rrr => wit("evex") "vfmsub213ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xaa 11 ggg rrr => wit("evex") "vfmsub213ph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xaa @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub213ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xaa @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub213ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xba 11 ggg rrr => wit("evex") "vfmsub231ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xba 11 ggg rrr => wit("evex") "vfmsub231ph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xba @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub231ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xba @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub231ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x9c 11 ggg rrr => wit("evex") "vfnmadd132ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0x9c 11 ggg rrr => wit("evex") "vfnmadd132ph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x9c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd132ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0x9c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd132ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xac 11 ggg rrr => wit("evex") "vfnmadd213ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xac 11 ggg rrr => wit("evex") "vfnmadd213ph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xac @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd213ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xac @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd213ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xbc 11 ggg rrr => wit("evex") "vfnmadd231ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xbc 11 ggg rrr => wit("evex") "vfnmadd231ph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xbc @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd231ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xbc @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd231ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x9e 11 ggg rrr => wit("evex") "vfnmsub132ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0x9e 11 ggg rrr => wit("evex") "vfnmsub132ph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x9e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub132ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0x9e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub132ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xae 11 ggg rrr => wit("evex") "vfnmsub213ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xae 11 ggg rrr => wit("evex") "vfnmsub213ph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xae @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub213ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xae @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub213ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xbe 11 ggg rrr => wit("evex") "vfnmsub231ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xbe 11 ggg rrr => wit("evex") "vfnmsub231ph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xbe @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub231ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xbe @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub231ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x96 11 ggg rrr => wit("evex") "vfmaddsub132ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0x96 11 ggg rrr => wit("evex") "vfmaddsub132ph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x96 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmaddsub132ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0x96 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmaddsub132ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xa6 11 ggg rrr => wit("evex") "vfmaddsub213ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xa6 11 ggg rrr => wit("evex") "vfmaddsub213ph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xa6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmaddsub213ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xa6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmaddsub213ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xb6 11 ggg rrr => wit("evex") "vfmaddsub231ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xb6 11 ggg rrr => wit("evex") "vfmaddsub231ph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xb6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmaddsub231ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xb6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmaddsub231ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x97 11 ggg rrr => wit("evex") "vfmsubadd132ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0x97 11 ggg rrr => wit("evex") "vfmsubadd132ph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x97 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsubadd132ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0x97 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsubadd132ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xa7 11 ggg rrr => wit("evex") "vfmsubadd213ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xa7 11 ggg rrr => wit("evex") "vfmsubadd213ph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xa7 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsubadd213ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xa7 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsubadd213ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xb7 11 ggg rrr => wit("evex") "vfmsubadd231ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xb7 11 ggg rrr => wit("evex") "vfmsubadd231ph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xb7 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsubadd231ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xb7 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsubadd231ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x99 11 ggg rrr => wit("evex") "vfmadd132sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0x99 11 ggg rrr => wit("evex") "vfmadd132sh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x99 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd132sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xa9 11 ggg rrr => wit("evex") "vfmadd213sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xa9 11 ggg rrr => wit("evex") "vfmadd213sh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xa9 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd213sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xb9 11 ggg rrr => wit("evex") "vfmadd231sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xb9 11 ggg rrr => wit("evex") "vfmadd231sh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xb9 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmadd231sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x9b 11 ggg rrr => wit("evex") "vfmsub132sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0x9b 11 ggg rrr => wit("evex") "vfmsub132sh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x9b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub132sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xab 11 ggg rrr => wit("evex") "vfmsub213sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xab 11 ggg rrr => wit("evex") "vfmsub213sh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xab @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub213sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xbb 11 ggg rrr => wit("evex") "vfmsub231sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xbb 11 ggg rrr => wit("evex") "vfmsub231sh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xbb @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmsub231sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x9d 11 ggg rrr => wit("evex") "vfnmadd132sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0x9d 11 ggg rrr => wit("evex") "vfnmadd132sh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x9d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd132sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xad 11 ggg rrr => wit("evex") "vfnmadd213sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xad 11 ggg rrr => wit("evex") "vfnmadd213sh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xad @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd213sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xbd 11 ggg rrr => wit("evex") "vfnmadd231sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xbd 11 ggg rrr => wit("evex") "vfnmadd231sh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xbd @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmadd231sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x9f 11 ggg rrr => wit("evex") "vfnmsub132sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0x9f 11 ggg rrr => wit("evex") "vfnmsub132sh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x9f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub132sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xaf 11 ggg rrr => wit("evex") "vfnmsub213sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xaf 11 ggg rrr => wit("evex") "vfnmsub213sh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xaf @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub213sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xbf 11 ggg rrr => wit("evex") "vfnmsub231sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0xbf 11 ggg rrr => wit("evex") "vfnmsub231sh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0xbf @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfnmsub231sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 10 z ll 0 u aaa 0x56 11 ggg rrr => wit("evex") "vfmaddcph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 10 z ll 1 u aaa 0x56 11 ggg rrr => wit("evex") "vfmaddcph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 10 z ll 0 u aaa 0x56 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmaddcph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 10 z ll 1 u aaa 0x56 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmaddcph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 110 0 vvvv 1 11 z ll 0 u aaa 0x56 11 ggg rrr => wit("evex") "vfcmaddcph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 11 z ll 1 u aaa 0x56 11 ggg rrr => wit("evex") "vfcmaddcph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 11 z ll 0 u aaa 0x56 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfcmaddcph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 11 z ll 1 u aaa 0x56 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfcmaddcph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 110 0 vvvv 1 10 z ll 0 u aaa 0xd6 11 ggg rrr => wit("evex") "vfmulcph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 10 z ll 1 u aaa 0xd6 11 ggg rrr => wit("evex") "vfmulcph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 10 z ll 0 u aaa 0xd6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmulcph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 10 z ll 1 u aaa 0xd6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmulcph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 110 0 vvvv 1 11 z ll 0 u aaa 0xd6 11 ggg rrr => wit("evex") "vfcmulcph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 11 z ll 1 u aaa 0xd6 11 ggg rrr => wit("evex") "vfcmulcph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 11 z ll 0 u aaa 0xd6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfcmulcph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 11 z ll 1 u aaa 0xd6 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfcmulcph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst32[$l] ;
  h k b e 00 110 0 vvvv 1 10 z ll 0 u aaa 0x57 11 ggg rrr => wit("evex") "vfmaddcsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 10 z ll 1 u aaa 0x57 11 ggg rrr => wit("evex") "vfmaddcsh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 10 z ll 0 u aaa 0x57 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmaddcsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 11 z ll 0 u aaa 0x57 11 ggg rrr => wit("evex") "vfcmaddcsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 11 z ll 1 u aaa 0x57 11 ggg rrr => wit("evex") "vfcmaddcsh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 11 z ll 0 u aaa 0x57 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfcmaddcsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 10 z ll 0 u aaa 0xd7 11 ggg rrr => wit("evex") "vfmulcsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 10 z ll 1 u aaa 0xd7 11 ggg rrr => wit("evex") "vfmulcsh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 10 z ll 0 u aaa 0xd7 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfmulcsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 11 z ll 0 u aaa 0xd7 11 ggg rrr => wit("evex") "vfcmulcsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 11 z ll 1 u aaa 0xd7 11 ggg rrr => wit("evex") "vfcmulcsh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 11 z ll 0 u aaa 0xd7 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vfcmulcsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x2c 11 ggg rrr => wit("evex") "vscalefph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0x2c 11 ggg rrr => wit("evex") "vscalefph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x2c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vscalefph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0x2c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vscalefph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr bcst16[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x2d 11 ggg rrr => wit("evex") "vscalefsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0x2d 11 ggg rrr => wit("evex") "vscalefsh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x2d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vscalefsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 1111 1 01 z ll 0 1 aaa 0x42 11 ggg rrr => wit("evex") "vgetexpph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 1111 1 01 z ll 1 1 aaa 0x42 11 ggg rrr => wit("evex") "vgetexpph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 110 0 1111 1 01 z ll 0 1 aaa 0x42 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vgetexpph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 110 0 1111 1 01 z ll 1 1 aaa 0x42 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vgetexpph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst16[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x43 11 ggg rrr => wit("evex") "vgetexpsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 1 u aaa 0x43 11 ggg rrr => wit("evex") "vgetexpsh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x43 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vgetexpsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 1111 1 01 z ll 0 1 aaa 0x4c 11 ggg rrr => wit("evex") "vrcpph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 1111 1 01 z ll 0 1 aaa 0x4c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrcpph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 110 0 1111 1 01 z ll 1 1 aaa 0x4c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrcpph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst16[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x4d 11 ggg rrr => wit("evex") "vrcpsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x4d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrcpsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 1111 1 01 z ll 0 1 aaa 0x4e 11 ggg rrr => wit("evex") "vrsqrtph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 1111 1 01 z ll 0 1 aaa 0x4e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrsqrtph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 110 0 1111 1 01 z ll 1 1 aaa 0x4e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrsqrtph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst16[$l] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x4f 11 ggg rrr => wit("evex") "vrsqrtsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 01 z ll 0 u aaa 0x4f @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vrsqrtsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 101 0 1111 1 00 z ll 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtdq2ph " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 1111 1 00 z ll 1 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtdq2ph " eregh[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 0 1111 1 00 z ll 0 1 aaa 0x5b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtdq2ph " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 0 1111 1 00 z ll 1 1 aaa 0x5b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtdq2ph " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  h k b e 00 101 0 1111 1 11 z ll 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtudq2ph " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 1111 1 11 z ll 1 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtudq2ph " eregh[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 0 1111 1 11 z ll 0 1 aaa 0x7a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtudq2ph " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 0 1111 1 11 z ll 1 1 aaa 0x7a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtudq2ph " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  h k b e 00 101 1 1111 1 00 z ll 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtqq2ph " eregx[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 1 1111 1 00 z ll 1 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtqq2ph " eregx[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 1 1111 1 00 z ll 0 1 aaa 0x5b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtqq2ph " eregx[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 1 1111 1 00 z ll 1 1 aaa 0x5b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtqq2ph " eregx[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 101 1 1111 1 11 z ll 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtuqq2ph " eregx[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 1 1111 1 11 z ll 1 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvtuqq2ph " eregx[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 1 1111 1 11 z ll 0 1 aaa 0x7a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtuqq2ph " eregx[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 1 1111 1 11 z ll 1 1 aaa 0x7a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtuqq2ph " eregx[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 101 0 1111 1 10 z ll 0 1 aaa 0x7d 11 ggg rrr => wit("evex") "vcvtw2ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 1111 1 10 z ll 1 1 aaa 0x7d 11 ggg rrr => wit("evex") "vcvtw2ph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 0 1111 1 10 z ll 0 1 aaa 0x7d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtw2ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 0 1111 1 10 z ll 1 1 aaa 0x7d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtw2ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst16[$l] ;
  h k b e 00 101 0 1111 1 11 z ll 0 1 aaa 0x7d 11 ggg rrr => wit("evex") "vcvtuw2ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 1111 1 11 z ll 1 1 aaa 0x7d 11 ggg rrr => wit("evex") "vcvtuw2ph " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 0 1111 1 11 z ll 0 1 aaa 0x7d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtuw2ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 0 1111 1 11 z ll 1 1 aaa 0x7d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtuw2ph " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst16[$l] ;
  h k b e 00 101 0 1111 1 01 z ll 0 1 aaa 0x1d 11 ggg rrr => wit("evex") "vcvtps2phx " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 1111 1 01 z ll 1 1 aaa 0x1d 11 ggg rrr => wit("evex") "vcvtps2phx " eregh[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 0 1111 1 01 z ll 0 1 aaa 0x1d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtps2phx " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 0 1111 1 01 z ll 1 1 aaa 0x1d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtps2phx " eregh[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst32[$l] ;
  h k b e 00 101 1 1111 1 01 z ll 0 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtpd2ph " eregx[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 1 1111 1 01 z ll 1 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtpd2ph " eregx[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 1 1111 1 01 z ll 0 1 aaa 0x5a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtpd2ph " eregx[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 1 1111 1 01 z ll 1 1 aaa 0x5a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtpd2ph " eregx[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst64[$l] ;
  h k b e 00 101 0 1111 1 01 z ll 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtph2dq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 1111 1 01 z ll 1 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvtph2dq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 0 1111 1 01 z ll 0 1 aaa 0x5b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtph2dq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 0 1111 1 01 z ll 1 1 aaa 0x5b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtph2dq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst16[$l] ;
  h k b e 00 101 0 1111 1 00 z ll 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtph2udq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 1111 1 00 z ll 1 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtph2udq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 0 1111 1 00 z ll 0 1 aaa 0x79 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtph2udq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 0 1111 1 00 z ll 1 1 aaa 0x79 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtph2udq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst16[$l] ;
  h k b e 00 101 0 1111 1 01 z ll 0 1 aaa 0x7b 11 ggg rrr => wit("evex") "vcvtph2qq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 1111 1 01 z ll 1 1 aaa 0x7b 11 ggg rrr => wit("evex") "vcvtph2qq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 0 1111 1 01 z ll 0 1 aaa 0x7b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtph2qq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 0 1111 1 01 z ll 1 1 aaa 0x7b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtph2qq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst16[$l] ;
  h k b e 00 101 0 1111 1 01 z ll 0 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtph2uqq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 1111 1 01 z ll 1 1 aaa 0x79 11 ggg rrr => wit("evex") "vcvtph2uqq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 0 1111 1 01 z ll 0 1 aaa 0x79 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtph2uqq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 0 1111 1 01 z ll 1 1 aaa 0x79 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtph2uqq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst16[$l] ;
  h k b e 00 101 0 1111 1 01 z ll 0 1 aaa 0x7d 11 ggg rrr => wit("evex") "vcvtph2w " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 1111 1 01 z ll 1 1 aaa 0x7d 11 ggg rrr => wit("evex") "vcvtph2w " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 0 1111 1 01 z ll 0 1 aaa 0x7d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtph2w " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 0 1111 1 01 z ll 1 1 aaa 0x7d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtph2w " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst16[$l] ;
  h k b e 00 101 0 1111 1 00 z ll 0 1 aaa 0x7d 11 ggg rrr => wit("evex") "vcvtph2uw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 1111 1 00 z ll 1 1 aaa 0x7d 11 ggg rrr => wit("evex") "vcvtph2uw " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 0 1111 1 00 z ll 0 1 aaa 0x7d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtph2uw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 0 1111 1 00 z ll 1 1 aaa 0x7d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtph2uw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst16[$l] ;
  h k b e 00 110 0 1111 1 01 z ll 0 1 aaa 0x13 11 ggg rrr => wit("evex") "vcvtph2psx " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 1111 1 01 z ll 1 1 aaa 0x13 11 ggg rrr => wit("evex") "vcvtph2psx " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 110 0 1111 1 01 z ll 0 1 aaa 0x13 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtph2psx " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 110 0 1111 1 01 z ll 1 1 aaa 0x13 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtph2psx " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst16[$l] ;
  h k b e 00 101 0 1111 1 00 z ll 0 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtph2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 1111 1 00 z ll 1 1 aaa 0x5a 11 ggg rrr => wit("evex") "vcvtph2pd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 101 0 1111 1 00 z ll 0 1 aaa 0x5a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtph2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 0 1111 1 00 z ll 1 1 aaa 0x5a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtph2pd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst16[$l] ;
  h k b e 00 101 0 1111 1 10 z ll 0 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvttph2dq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 1111 1 10 z ll 1 1 aaa 0x5b 11 ggg rrr => wit("evex") "vcvttph2dq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 101 0 1111 1 10 z ll 0 1 aaa 0x5b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttph2dq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 0 1111 1 10 z ll 1 1 aaa 0x5b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttph2dq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst16[$l] ;
  h k b e 00 101 0 1111 1 00 z ll 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttph2udq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 1111 1 00 z ll 1 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttph2udq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregh[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 101 0 1111 1 00 z ll 0 1 aaa 0x78 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttph2udq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 0 1111 1 00 z ll 1 1 aaa 0x78 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttph2udq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst16[$l] ;
  h k b e 00 101 0 1111 1 01 z ll 0 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvttph2qq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 1111 1 01 z ll 1 1 aaa 0x7a 11 ggg rrr => wit("evex") "vcvttph2qq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 101 0 1111 1 01 z ll 0 1 aaa 0x7a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttph2qq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 0 1111 1 01 z ll 1 1 aaa 0x7a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttph2qq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst16[$l] ;
  h k b e 00 101 0 1111 1 01 z ll 0 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttph2uqq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 1111 1 01 z ll 1 1 aaa 0x78 11 ggg rrr => wit("evex") "vcvttph2uqq " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," eregx[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 101 0 1111 1 01 z ll 0 1 aaa 0x78 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttph2uqq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 0 1111 1 01 z ll 1 1 aaa 0x78 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttph2uqq " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst16[$l] ;
  h k b e 00 101 0 1111 1 01 z ll 0 1 aaa 0x7c 11 ggg rrr => wit("evex") "vcvttph2w " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 1111 1 01 z ll 1 1 aaa 0x7c 11 ggg rrr => wit("evex") "vcvttph2w " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 101 0 1111 1 01 z ll 0 1 aaa 0x7c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttph2w " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 0 1111 1 01 z ll 1 1 aaa 0x7c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttph2w " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst16[$l] ;
  h k b e 00 101 0 1111 1 00 z ll 0 1 aaa 0x7c 11 ggg rrr => wit("evex") "vcvttph2uw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 1111 1 00 z ll 1 1 aaa 0x7c 11 ggg rrr => wit("evex") "vcvttph2uw " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 101 0 1111 1 00 z ll 0 1 aaa 0x7c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttph2uw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 0 1111 1 00 z ll 1 1 aaa 0x7c @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttph2uw " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr bcst16[$l] ;
  h k b e 00 101 0 vvvv 1 10 z ll 0 u aaa 0x5a 11 ggg rrr => wit("evex") "vcvtsh2sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 vvvv 1 10 z ll 1 u aaa 0x5a 11 ggg rrr => wit("evex") "vcvtsh2sd " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 101 0 vvvv 1 10 z ll 0 u aaa 0x5a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtsh2sd " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 101 1 vvvv 1 11 z ll 0 u aaa 0x5a 11 ggg rrr => wit("evex") "vcvtsd2sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 1 vvvv 1 11 z ll 1 u aaa 0x5a 11 ggg rrr => wit("evex") "vcvtsd2sh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 1 vvvv 1 11 z ll 0 u aaa 0x5a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtsd2sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 110 0 vvvv 1 00 z ll 0 u aaa 0x13 11 ggg rrr => wit("evex") "vcvtsh2ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 110 0 vvvv 1 00 z ll 1 u aaa 0x13 11 ggg rrr => wit("evex") "vcvtsh2ss " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 110 0 vvvv 1 00 z ll 0 u aaa 0x13 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtsh2ss " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 101 0 vvvv 1 00 z ll 0 u aaa 0x1d 11 ggg rrr => wit("evex") "vcvtss2sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 vvvv 1 00 z ll 1 u aaa 0x1d 11 ggg rrr => wit("evex") "vcvtss2sh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 0 vvvv 1 00 z ll 0 u aaa 0x1d @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtss2sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  # ==== AVX512-FP16 batch F: GPR converts (vcvtsi2sh/vcvtsh2si families), vmovsh, vmovw ====
  h k b e 00 101 w vvvv 1 10 z ll 0 u aaa 0x2a 11 ggg rrr {$rexb=1-$b} => wit("evex") "vcvtsi2sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," greg[32*$w+8*$rexb+$r] ;
  h k b e 00 101 w vvvv 1 10 z ll 1 u aaa 0x2a 11 ggg rrr {$rexb=1-$b} => wit("evex") "vcvtsi2sh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," greg[32*$w+8*$rexb+$r] rcdec[$l] ;
  h k b e 00 101 w vvvv 1 10 z ll 0 u aaa 0x2a @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtsi2sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 101 w vvvv 1 10 z ll 0 u aaa 0x7b 11 ggg rrr {$rexb=1-$b} => wit("evex") "vcvtusi2sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," greg[32*$w+8*$rexb+$r] ;
  h k b e 00 101 w vvvv 1 10 z ll 1 u aaa 0x7b 11 ggg rrr {$rexb=1-$b} => wit("evex") "vcvtusi2sh " ereg[64+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[64+16*$u+$v] "," greg[32*$w+8*$rexb+$r] rcdec[$l] ;
  h k b e 00 101 w vvvv 1 10 z ll 0 u aaa 0x7b @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtusi2sh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," $addr ;
  h k b e 00 101 w 1111 1 10 z ll 0 1 aaa 0x2d 11 ggg rrr {$rexr=1-$h} => wit("evex") "vcvtsh2si " greg[32*$w+8*$rexr+$g] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 w 1111 1 10 z ll 1 1 aaa 0x2d 11 ggg rrr {$rexr=1-$h} => wit("evex") "vcvtsh2si " greg[32*$w+8*$rexr+$g] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 w 1111 1 10 z ll 0 1 aaa 0x2d @addr {$rexr=1-$h;$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtsh2si " greg[32*$w+8*$rexr+$g] "," $addr ;
  h k b e 00 101 w 1111 1 10 z ll 0 1 aaa 0x79 11 ggg rrr {$rexr=1-$h} => wit("evex") "vcvtsh2usi " greg[32*$w+8*$rexr+$g] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 w 1111 1 10 z ll 1 1 aaa 0x79 11 ggg rrr {$rexr=1-$h} => wit("evex") "vcvtsh2usi " greg[32*$w+8*$rexr+$g] "," ereg[64+16*$k+8*$b+$r] rcdec[$l] ;
  h k b e 00 101 w 1111 1 10 z ll 0 1 aaa 0x79 @addr {$rexr=1-$h;$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvtsh2usi " greg[32*$w+8*$rexr+$g] "," $addr ;
  h k b e 00 101 w 1111 1 10 z ll 0 1 aaa 0x2c 11 ggg rrr {$rexr=1-$h} => wit("evex") "vcvttsh2si " greg[32*$w+8*$rexr+$g] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 w 1111 1 10 z 00 1 1 aaa 0x2c 11 ggg rrr {$rexr=1-$h} => wit("evex") "vcvttsh2si " greg[32*$w+8*$rexr+$g] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 101 w 1111 1 10 z ll 0 1 aaa 0x2c @addr {$rexr=1-$h;$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttsh2si " greg[32*$w+8*$rexr+$g] "," $addr ;
  h k b e 00 101 w 1111 1 10 z ll 0 1 aaa 0x78 11 ggg rrr {$rexr=1-$h} => wit("evex") "vcvttsh2usi " greg[32*$w+8*$rexr+$g] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 w 1111 1 10 z 00 1 1 aaa 0x78 11 ggg rrr {$rexr=1-$h} => wit("evex") "vcvttsh2usi " greg[32*$w+8*$rexr+$g] "," ereg[64+16*$k+8*$b+$r] " {sae}" ;
  h k b e 00 101 w 1111 1 10 z ll 0 1 aaa 0x78 @addr {$rexr=1-$h;$rexb=1-$b;$rexx=1-$k} => wit("evex") "vcvttsh2usi " greg[32*$w+8*$rexr+$g] "," $addr ;
  h k b e 00 101 0 vvvv 1 10 z ll 0 u aaa 0x10 11 ggg rrr => wit("evex") "vmovsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," evvv[32*$l+16*$u+$v] "," ereg[32*$l+16*$k+8*$b+$r] ;
  h k b e 00 101 0 1111 1 10 z ll 0 1 aaa 0x10 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovsh " ereg[32*$l+16*$e+8*$h+$g] kzdec[$z*8+$a] "," $addr ;
  h k b e 00 101 0 1111 1 10 z ll 0 1 aaa 0x11 @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovsh " $addr kzdec[$z*8+$a] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 101 w 1111 1 01 z ll 0 1 aaa 0x6e 11 ggg rrr {$rexb=1-$b} => wit("evex") "vmovw " ereg[32*$l+16*$e+8*$h+$g] "," greg[32+8*$rexb+$r] ;
  h k b e 00 101 w 1111 1 01 z ll 0 1 aaa 0x6e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovw " ereg[32*$l+16*$e+8*$h+$g] "," $addr ;
  h k b e 00 101 w 1111 1 01 z ll 0 1 aaa 0x7e 11 ggg rrr {$rexb=1-$b} => wit("evex") "vmovw " greg[32+8*$rexb+$r] "," ereg[32*$l+16*$e+8*$h+$g] ;
  h k b e 00 101 w 1111 1 01 z ll 0 1 aaa 0x7e @addr {$rexb=1-$b;$rexx=1-$k} => wit("evex") "vmovw " $addr "," ereg[32*$l+16*$e+8*$h+$g] ;
}


submatch xop {
  # ====== XOP map 8 (XOP.08), pp=00 ======
  # vprot[bwdq] by imm8 (RMI: dest, r/m, imm8)
  h k b 01000 0 1111 y 00 0xc0 11 ggg rrr @imm8 => "vprotb " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 01000 0 1111 y 00 0xc1 11 ggg rrr @imm8 => "vprotw " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 01000 0 1111 y 00 0xc2 11 ggg rrr @imm8 => "vprotd " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 01000 0 1111 y 00 0xc3 11 ggg rrr @imm8 => "vprotq " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  # vpmacs* / vpmadcs* (RVMR: dest, vvvv, r/m, is4)
  h k b 01000 0 vvvv y 00 0x85 11 ggg rrr qqqq 0000 => "vpmacssww " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 01000 0 vvvv y 00 0x86 11 ggg rrr qqqq 0000 => "vpmacsswd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 01000 0 vvvv y 00 0x87 11 ggg rrr qqqq 0000 => "vpmacssdql " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 01000 0 vvvv y 00 0x8e 11 ggg rrr qqqq 0000 => "vpmacssdd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 01000 0 vvvv y 00 0x8f 11 ggg rrr qqqq 0000 => "vpmacssdqh " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 01000 0 vvvv y 00 0x95 11 ggg rrr qqqq 0000 => "vpmacsww " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 01000 0 vvvv y 00 0x96 11 ggg rrr qqqq 0000 => "vpmacswd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 01000 0 vvvv y 00 0x97 11 ggg rrr qqqq 0000 => "vpmacsdql " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 01000 0 vvvv y 00 0x9e 11 ggg rrr qqqq 0000 => "vpmacsdd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 01000 0 vvvv y 00 0x9f 11 ggg rrr qqqq 0000 => "vpmacsdqh " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 01000 0 vvvv y 00 0xa6 11 ggg rrr qqqq 0000 => "vpmadcsswd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 01000 0 vvvv y 00 0xb6 11 ggg rrr qqqq 0000 => "vpmadcswd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  # vpcmov / vpperm: W0 (RVMR) and W1 (RVRM: is4 & r/m display-swapped)
  h k b 01000 0 vvvv y 00 0xa2 11 ggg rrr qqqq 0000 => "vpcmov " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 01000 1 vvvv y 00 0xa2 11 ggg rrr qqqq 0000 => "vpcmov " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," vreg[16*$y+8*$b+$r] ;
  h k b 01000 0 vvvv y 00 0xa3 11 ggg rrr qqqq 0000 => "vpperm " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," vregd[16*$y+$q] ;
  h k b 01000 1 vvvv y 00 0xa3 11 ggg rrr qqqq 0000 => "vpperm " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vregd[16*$y+$q] "," vreg[16*$y+8*$b+$r] ;
  # vpcom[bwdq] / vpcomu[bwdq] (RVMI: dest, vvvv, r/m, imm8)
  h k b 01000 0 vvvv y 00 0xcc 11 ggg rrr @imm8 => "vpcomb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 01000 0 vvvv y 00 0xcd 11 ggg rrr @imm8 => "vpcomw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 01000 0 vvvv y 00 0xce 11 ggg rrr @imm8 => "vpcomd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 01000 0 vvvv y 00 0xcf 11 ggg rrr @imm8 => "vpcomq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 01000 0 vvvv y 00 0xec 11 ggg rrr @imm8 => "vpcomub " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 01000 0 vvvv y 00 0xed 11 ggg rrr @imm8 => "vpcomuw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 01000 0 vvvv y 00 0xee 11 ggg rrr @imm8 => "vpcomud " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  h k b 01000 0 vvvv y 00 0xef 11 ggg rrr @imm8 => "vpcomuq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] "," hex($imm8) ;
  # ====== XOP map 9 (XOP.09), pp=00 ======
  # vfrcz[ps pd ss sd] (RM: dest, r/m ; vvvv=1111)
  h k b 01001 0 1111 y 00 0x80 11 ggg rrr => "vfrczps " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 1111 y 00 0x81 11 ggg rrr => "vfrczpd " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 1111 y 00 0x82 11 ggg rrr => "vfrczss " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 1111 y 00 0x83 11 ggg rrr => "vfrczsd " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  # vprot / vpshl / vpsha (variable): W0 (RMV: dest,r/m,vvvv), W1 (RVM: dest,vvvv,r/m)
  h k b 01001 0 vvvv y 00 0x90 11 ggg rrr => "vprotb " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," vvv[16*$y+$v] ;
  h k b 01001 1 vvvv y 00 0x90 11 ggg rrr => "vprotb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 vvvv y 00 0x91 11 ggg rrr => "vprotw " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," vvv[16*$y+$v] ;
  h k b 01001 1 vvvv y 00 0x91 11 ggg rrr => "vprotw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 vvvv y 00 0x92 11 ggg rrr => "vprotd " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," vvv[16*$y+$v] ;
  h k b 01001 1 vvvv y 00 0x92 11 ggg rrr => "vprotd " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 vvvv y 00 0x93 11 ggg rrr => "vprotq " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," vvv[16*$y+$v] ;
  h k b 01001 1 vvvv y 00 0x93 11 ggg rrr => "vprotq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 vvvv y 00 0x94 11 ggg rrr => "vpshlb " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," vvv[16*$y+$v] ;
  h k b 01001 1 vvvv y 00 0x94 11 ggg rrr => "vpshlb " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 vvvv y 00 0x95 11 ggg rrr => "vpshlw " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," vvv[16*$y+$v] ;
  h k b 01001 1 vvvv y 00 0x95 11 ggg rrr => "vpshlw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 vvvv y 00 0x96 11 ggg rrr => "vpshld " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," vvv[16*$y+$v] ;
  h k b 01001 1 vvvv y 00 0x96 11 ggg rrr => "vpshld " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 vvvv y 00 0x97 11 ggg rrr => "vpshlq " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," vvv[16*$y+$v] ;
  h k b 01001 1 vvvv y 00 0x97 11 ggg rrr => "vpshlq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 vvvv y 00 0x98 11 ggg rrr => "vpshab " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," vvv[16*$y+$v] ;
  h k b 01001 1 vvvv y 00 0x98 11 ggg rrr => "vpshab " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 vvvv y 00 0x99 11 ggg rrr => "vpshaw " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," vvv[16*$y+$v] ;
  h k b 01001 1 vvvv y 00 0x99 11 ggg rrr => "vpshaw " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 vvvv y 00 0x9a 11 ggg rrr => "vpshad " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," vvv[16*$y+$v] ;
  h k b 01001 1 vvvv y 00 0x9a 11 ggg rrr => "vpshad " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 vvvv y 00 0x9b 11 ggg rrr => "vpshaq " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] "," vvv[16*$y+$v] ;
  h k b 01001 1 vvvv y 00 0x9b 11 ggg rrr => "vpshaq " vreg[16*$y+8*$h+$g] "," vvv[16*$y+$v] "," vreg[16*$y+8*$b+$r] ;
  # vphadd* / vphsub* (RM: dest, r/m ; vvvv=1111)
  h k b 01001 0 1111 y 00 0xc1 11 ggg rrr => "vphaddbw " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 1111 y 00 0xc2 11 ggg rrr => "vphaddbd " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 1111 y 00 0xc3 11 ggg rrr => "vphaddbq " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 1111 y 00 0xc6 11 ggg rrr => "vphaddwd " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 1111 y 00 0xc7 11 ggg rrr => "vphaddwq " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 1111 y 00 0xcb 11 ggg rrr => "vphadddq " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 1111 y 00 0xd1 11 ggg rrr => "vphaddubw " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 1111 y 00 0xd2 11 ggg rrr => "vphaddubd " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 1111 y 00 0xd3 11 ggg rrr => "vphaddubq " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 1111 y 00 0xd6 11 ggg rrr => "vphadduwd " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 1111 y 00 0xd7 11 ggg rrr => "vphadduwq " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 1111 y 00 0xdb 11 ggg rrr => "vphaddudq " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 1111 y 00 0xe1 11 ggg rrr => "vphsubbw " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 1111 y 00 0xe2 11 ggg rrr => "vphsubwd " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  h k b 01001 0 1111 y 00 0xe3 11 ggg rrr => "vphsubdq " vreg[16*$y+8*$h+$g] "," vreg[16*$y+8*$b+$r] ;
  # ==== XOP GPR families: TBM / LWP / XOP-bextr (AMD Bulldozer-family, objdump-decodable) ====
  # These decode on real AMD silicon, so they are lowered semantically (not the structural
  # placeholder). GPR dest/src ride vvvv/reg/r-m; W selects r32 (W0) vs r64 (W1). The mem r/m
  # form is handled by the C++ once the opcode/group cell exists (reg-direct rules suffice).
  # TBM map 9 op 01 (dest=vvvv, src=r/m; mnem by ModR/M /digit): blcfill/blsfill/blcs/tzmsk/blcic/blsic/t1mskc
  h k b 01001 0 vvvv y 00 0x01 11 001 rrr => "blcfill " greg[$v] "," greg[$r] ;
  h k b 01001 0 vvvv y 00 0x01 11 010 rrr => "blsfill " greg[$v] "," greg[$r] ;
  h k b 01001 0 vvvv y 00 0x01 11 011 rrr => "blcs " greg[$v] "," greg[$r] ;
  h k b 01001 0 vvvv y 00 0x01 11 100 rrr => "tzmsk " greg[$v] "," greg[$r] ;
  h k b 01001 0 vvvv y 00 0x01 11 101 rrr => "blcic " greg[$v] "," greg[$r] ;
  h k b 01001 0 vvvv y 00 0x01 11 110 rrr => "blsic " greg[$v] "," greg[$r] ;
  h k b 01001 0 vvvv y 00 0x01 11 111 rrr => "t1mskc " greg[$v] "," greg[$r] ;
  h k b 01001 1 vvvv y 00 0x01 11 001 rrr => "blcfill " greg[32+$v] "," greg[32+$r] ;
  h k b 01001 1 vvvv y 00 0x01 11 010 rrr => "blsfill " greg[32+$v] "," greg[32+$r] ;
  h k b 01001 1 vvvv y 00 0x01 11 011 rrr => "blcs " greg[32+$v] "," greg[32+$r] ;
  h k b 01001 1 vvvv y 00 0x01 11 100 rrr => "tzmsk " greg[32+$v] "," greg[32+$r] ;
  h k b 01001 1 vvvv y 00 0x01 11 101 rrr => "blcic " greg[32+$v] "," greg[32+$r] ;
  h k b 01001 1 vvvv y 00 0x01 11 110 rrr => "blsic " greg[32+$v] "," greg[32+$r] ;
  h k b 01001 1 vvvv y 00 0x01 11 111 rrr => "t1mskc " greg[32+$v] "," greg[32+$r] ;
  # TBM map 9 op 02 (/1 blcmsk, /6 blci)
  h k b 01001 0 vvvv y 00 0x02 11 001 rrr => "blcmsk " greg[$v] "," greg[$r] ;
  h k b 01001 0 vvvv y 00 0x02 11 110 rrr => "blci " greg[$v] "," greg[$r] ;
  h k b 01001 1 vvvv y 00 0x02 11 001 rrr => "blcmsk " greg[32+$v] "," greg[32+$r] ;
  h k b 01001 1 vvvv y 00 0x02 11 110 rrr => "blci " greg[32+$v] "," greg[32+$r] ;
  # LWP map 9 op 12 (single r/m GPR, reg-only; mnem by /digit): /0 llwpcb, /1 slwpcb. vvvv=1111 reserved.
  h k b 01001 0 1111 y 00 0x12 11 000 rrr => "llwpcb " greg[$r] ;
  h k b 01001 0 1111 y 00 0x12 11 001 rrr => "slwpcb " greg[$r] ;
  h k b 01001 1 1111 y 00 0x12 11 000 rrr => "llwpcb " greg[32+$r] ;
  h k b 01001 1 1111 y 00 0x12 11 001 rrr => "slwpcb " greg[32+$r] ;
  # XOP bextr map 10 op 10 (dest=reg, src=r/m, imm32; vvvv=1111 reserved; not a group)
  h k b 01010 0 1111 y 00 0x10 11 ggg rrr @imm32 => "bextr " greg[$g] "," greg[$r] "," hex($imm32) ;
  h k b 01010 0 1111 y 00 0x10 @addr {$rexb=1-$b;$rexx=1-$k} @imm32 => "bextr " greg[$g] "," $addr "," hex($imm32) ;
  h k b 01010 1 1111 y 00 0x10 11 ggg rrr @imm32 => "bextr " greg[32+$g] "," greg[32+$r] "," hex($imm32) ;
  h k b 01010 1 1111 y 00 0x10 @addr {$rexb=1-$b;$rexx=1-$k} @imm32 => "bextr " greg[32+$g] "," $addr "," hex($imm32) ;
  # LWP map 10 op 12 (dest=vvvv, src=r/m, imm32; mnem by /digit): /0 lwpins, /1 lwpval
  h k b 01010 0 vvvv y 00 0x12 11 000 rrr @imm32 => "lwpins " greg[$v] "," greg[$r] "," hex($imm32) ;
  h k b 01010 0 vvvv y 00 0x12 11 001 rrr @imm32 => "lwpval " greg[$v] "," greg[$r] "," hex($imm32) ;
  h k b 01010 1 vvvv y 00 0x12 11 000 rrr @imm32 => "lwpins " greg[32+$v] "," greg[32+$r] "," hex($imm32) ;
  h k b 01010 1 vvvv y 00 0x12 11 001 rrr @imm32 => "lwpval " greg[32+$v] "," greg[32+$r] "," hex($imm32) ;
}

submatch apx {
  # ===== APX EVEX-promoted legacy, map 4 (Stage 3a: ALU/cmov/imul) =====
  # add
  r x b e f 100 w v j 00 c d n a 0x00 11 ggg rrr => "add " rgb[$r] "," rgb[$g] ;
  r x b e f 100 w v j 00 c d n a 0x01 11 ggg rrr => "add " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 00 c d n a 0x02 11 ggg rrr => "add " rgb[$g] "," rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0x03 11 ggg rrr => "add " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x00 11 ggg rrr => "add " rgb[$r] "," rgb[$g] ;
  r x b e f 100 w v j 01 c d n a 0x01 11 ggg rrr => "add " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 01 c d n a 0x02 11 ggg rrr => "add " rgb[$g] "," rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0x03 11 ggg rrr => "add " greg[$g] "," greg[$r] ;
  # or
  r x b e f 100 w v j 00 c d n a 0x08 11 ggg rrr => "or " rgb[$r] "," rgb[$g] ;
  r x b e f 100 w v j 00 c d n a 0x09 11 ggg rrr => "or " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 00 c d n a 0x0a 11 ggg rrr => "or " rgb[$g] "," rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0x0b 11 ggg rrr => "or " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x08 11 ggg rrr => "or " rgb[$r] "," rgb[$g] ;
  r x b e f 100 w v j 01 c d n a 0x09 11 ggg rrr => "or " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 01 c d n a 0x0a 11 ggg rrr => "or " rgb[$g] "," rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0x0b 11 ggg rrr => "or " greg[$g] "," greg[$r] ;
  # adc
  r x b e f 100 w v j 00 c d n a 0x10 11 ggg rrr => "adc " rgb[$r] "," rgb[$g] ;
  r x b e f 100 w v j 00 c d n a 0x11 11 ggg rrr => "adc " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 00 c d n a 0x12 11 ggg rrr => "adc " rgb[$g] "," rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0x13 11 ggg rrr => "adc " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x10 11 ggg rrr => "adc " rgb[$r] "," rgb[$g] ;
  r x b e f 100 w v j 01 c d n a 0x11 11 ggg rrr => "adc " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 01 c d n a 0x12 11 ggg rrr => "adc " rgb[$g] "," rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0x13 11 ggg rrr => "adc " greg[$g] "," greg[$r] ;
  # sbb
  r x b e f 100 w v j 00 c d n a 0x18 11 ggg rrr => "sbb " rgb[$r] "," rgb[$g] ;
  r x b e f 100 w v j 00 c d n a 0x19 11 ggg rrr => "sbb " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 00 c d n a 0x1a 11 ggg rrr => "sbb " rgb[$g] "," rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0x1b 11 ggg rrr => "sbb " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x18 11 ggg rrr => "sbb " rgb[$r] "," rgb[$g] ;
  r x b e f 100 w v j 01 c d n a 0x19 11 ggg rrr => "sbb " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 01 c d n a 0x1a 11 ggg rrr => "sbb " rgb[$g] "," rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0x1b 11 ggg rrr => "sbb " greg[$g] "," greg[$r] ;
  # and
  r x b e f 100 w v j 00 c d n a 0x20 11 ggg rrr => "and " rgb[$r] "," rgb[$g] ;
  r x b e f 100 w v j 00 c d n a 0x21 11 ggg rrr => "and " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 00 c d n a 0x22 11 ggg rrr => "and " rgb[$g] "," rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0x23 11 ggg rrr => "and " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x20 11 ggg rrr => "and " rgb[$r] "," rgb[$g] ;
  r x b e f 100 w v j 01 c d n a 0x21 11 ggg rrr => "and " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 01 c d n a 0x22 11 ggg rrr => "and " rgb[$g] "," rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0x23 11 ggg rrr => "and " greg[$g] "," greg[$r] ;
  # sub
  r x b e f 100 w v j 00 c d n a 0x28 11 ggg rrr => "sub " rgb[$r] "," rgb[$g] ;
  r x b e f 100 w v j 00 c d n a 0x29 11 ggg rrr => "sub " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 00 c d n a 0x2a 11 ggg rrr => "sub " rgb[$g] "," rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0x2b 11 ggg rrr => "sub " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x28 11 ggg rrr => "sub " rgb[$r] "," rgb[$g] ;
  r x b e f 100 w v j 01 c d n a 0x29 11 ggg rrr => "sub " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 01 c d n a 0x2a 11 ggg rrr => "sub " rgb[$g] "," rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0x2b 11 ggg rrr => "sub " greg[$g] "," greg[$r] ;
  # xor
  r x b e f 100 w v j 00 c d n a 0x30 11 ggg rrr => "xor " rgb[$r] "," rgb[$g] ;
  r x b e f 100 w v j 00 c d n a 0x31 11 ggg rrr => "xor " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 00 c d n a 0x32 11 ggg rrr => "xor " rgb[$g] "," rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0x33 11 ggg rrr => "xor " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x30 11 ggg rrr => "xor " rgb[$r] "," rgb[$g] ;
  r x b e f 100 w v j 01 c d n a 0x31 11 ggg rrr => "xor " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 01 c d n a 0x32 11 ggg rrr => "xor " rgb[$g] "," rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0x33 11 ggg rrr => "xor " greg[$g] "," greg[$r] ;
  # cmp
  r x b e f 100 w v j 00 c d n a 0x38 11 ggg rrr => "cmp " rgb[$r] "," rgb[$g] ;
  r x b e f 100 w v j 00 c d n a 0x39 11 ggg rrr => "cmp " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 00 c d n a 0x3a 11 ggg rrr => "cmp " rgb[$g] "," rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0x3b 11 ggg rrr => "cmp " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x38 11 ggg rrr => "cmp " rgb[$r] "," rgb[$g] ;
  r x b e f 100 w v j 01 c d n a 0x39 11 ggg rrr => "cmp " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 01 c d n a 0x3a 11 ggg rrr => "cmp " rgb[$g] "," rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0x3b 11 ggg rrr => "cmp " greg[$g] "," greg[$r] ;
  # cmovcc (map4 40-4f), opsize only
  r x b e f 100 w v j 00 c d n a 0x40 11 ggg rrr => "cmovo " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x40 11 ggg rrr => "cmovo " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0x41 11 ggg rrr => "cmovno " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x41 11 ggg rrr => "cmovno " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0x42 11 ggg rrr => "cmovb " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x42 11 ggg rrr => "cmovb " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0x43 11 ggg rrr => "cmovae " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x43 11 ggg rrr => "cmovae " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0x44 11 ggg rrr => "cmove " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x44 11 ggg rrr => "cmove " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0x45 11 ggg rrr => "cmovne " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x45 11 ggg rrr => "cmovne " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0x46 11 ggg rrr => "cmovbe " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x46 11 ggg rrr => "cmovbe " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0x47 11 ggg rrr => "cmova " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x47 11 ggg rrr => "cmova " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0x48 11 ggg rrr => "cmovs " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x48 11 ggg rrr => "cmovs " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0x49 11 ggg rrr => "cmovns " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x49 11 ggg rrr => "cmovns " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0x4a 11 ggg rrr => "cmovp " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x4a 11 ggg rrr => "cmovp " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0x4b 11 ggg rrr => "cmovnp " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x4b 11 ggg rrr => "cmovnp " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0x4c 11 ggg rrr => "cmovl " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x4c 11 ggg rrr => "cmovl " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0x4d 11 ggg rrr => "cmovge " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x4d 11 ggg rrr => "cmovge " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0x4e 11 ggg rrr => "cmovle " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x4e 11 ggg rrr => "cmovle " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0x4f 11 ggg rrr => "cmovg " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x4f 11 ggg rrr => "cmovg " greg[$g] "," greg[$r] ;
  # imul r,r/m (map4 af), opsize only
  r x b e f 100 w v j 00 c d n a 0xaf 11 ggg rrr => "imul " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xaf 11 ggg rrr => "imul " greg[$g] "," greg[$r] ;
  # ===== Stage 3b: opcode-extension groups (NDD via ND; mnem by /digit) =====
  r x b e f 100 w v j 00 c d n a 0x80 11 000 rrr @imm8 => "add " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0x80 11 001 rrr @imm8 => "or " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0x80 11 010 rrr @imm8 => "adc " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0x80 11 011 rrr @imm8 => "sbb " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0x80 11 100 rrr @imm8 => "and " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0x80 11 101 rrr @imm8 => "sub " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0x80 11 110 rrr @imm8 => "xor " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0x80 11 000 rrr @imm8 => "add " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0x80 11 001 rrr @imm8 => "or " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0x80 11 010 rrr @imm8 => "adc " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0x80 11 011 rrr @imm8 => "sbb " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0x80 11 100 rrr @imm8 => "and " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0x80 11 101 rrr @imm8 => "sub " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0x80 11 110 rrr @imm8 => "xor " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0x81 11 000 rrr @immz => "add " greg[$r] "," hex($immz) ;
  r x b e f 100 w v j 00 c d n a 0x81 11 001 rrr @immz => "or " greg[$r] "," hex($immz) ;
  r x b e f 100 w v j 00 c d n a 0x81 11 010 rrr @immz => "adc " greg[$r] "," hex($immz) ;
  r x b e f 100 w v j 00 c d n a 0x81 11 011 rrr @immz => "sbb " greg[$r] "," hex($immz) ;
  r x b e f 100 w v j 00 c d n a 0x81 11 100 rrr @immz => "and " greg[$r] "," hex($immz) ;
  r x b e f 100 w v j 00 c d n a 0x81 11 101 rrr @immz => "sub " greg[$r] "," hex($immz) ;
  r x b e f 100 w v j 00 c d n a 0x81 11 110 rrr @immz => "xor " greg[$r] "," hex($immz) ;
  r x b e f 100 w v j 01 c d n a 0x81 11 000 rrr @immz => "add " greg[$r] "," hex($immz) ;
  r x b e f 100 w v j 01 c d n a 0x81 11 001 rrr @immz => "or " greg[$r] "," hex($immz) ;
  r x b e f 100 w v j 01 c d n a 0x81 11 010 rrr @immz => "adc " greg[$r] "," hex($immz) ;
  r x b e f 100 w v j 01 c d n a 0x81 11 011 rrr @immz => "sbb " greg[$r] "," hex($immz) ;
  r x b e f 100 w v j 01 c d n a 0x81 11 100 rrr @immz => "and " greg[$r] "," hex($immz) ;
  r x b e f 100 w v j 01 c d n a 0x81 11 101 rrr @immz => "sub " greg[$r] "," hex($immz) ;
  r x b e f 100 w v j 01 c d n a 0x81 11 110 rrr @immz => "xor " greg[$r] "," hex($immz) ;
  r x b e f 100 w v j 00 c d n a 0x83 11 000 rrr @imm8 => "add " greg[$r] "," hex(sx8($imm8)) ;
  r x b e f 100 w v j 00 c d n a 0x83 11 001 rrr @imm8 => "or " greg[$r] "," hex(sx8($imm8)) ;
  r x b e f 100 w v j 00 c d n a 0x83 11 010 rrr @imm8 => "adc " greg[$r] "," hex(sx8($imm8)) ;
  r x b e f 100 w v j 00 c d n a 0x83 11 011 rrr @imm8 => "sbb " greg[$r] "," hex(sx8($imm8)) ;
  r x b e f 100 w v j 00 c d n a 0x83 11 100 rrr @imm8 => "and " greg[$r] "," hex(sx8($imm8)) ;
  r x b e f 100 w v j 00 c d n a 0x83 11 101 rrr @imm8 => "sub " greg[$r] "," hex(sx8($imm8)) ;
  r x b e f 100 w v j 00 c d n a 0x83 11 110 rrr @imm8 => "xor " greg[$r] "," hex(sx8($imm8)) ;
  r x b e f 100 w v j 01 c d n a 0x83 11 000 rrr @imm8 => "add " greg[$r] "," hex(sx8($imm8)) ;
  r x b e f 100 w v j 01 c d n a 0x83 11 001 rrr @imm8 => "or " greg[$r] "," hex(sx8($imm8)) ;
  r x b e f 100 w v j 01 c d n a 0x83 11 010 rrr @imm8 => "adc " greg[$r] "," hex(sx8($imm8)) ;
  r x b e f 100 w v j 01 c d n a 0x83 11 011 rrr @imm8 => "sbb " greg[$r] "," hex(sx8($imm8)) ;
  r x b e f 100 w v j 01 c d n a 0x83 11 100 rrr @imm8 => "and " greg[$r] "," hex(sx8($imm8)) ;
  r x b e f 100 w v j 01 c d n a 0x83 11 101 rrr @imm8 => "sub " greg[$r] "," hex(sx8($imm8)) ;
  r x b e f 100 w v j 01 c d n a 0x83 11 110 rrr @imm8 => "xor " greg[$r] "," hex(sx8($imm8)) ;
  r x b e f 100 w v j 00 c d n a 0xc0 11 000 rrr @imm8 => "rol " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0xc0 11 001 rrr @imm8 => "ror " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0xc0 11 010 rrr @imm8 => "rcl " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0xc0 11 011 rrr @imm8 => "rcr " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0xc0 11 100 rrr @imm8 => "shl " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0xc0 11 101 rrr @imm8 => "shr " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0xc0 11 110 rrr @imm8 => "shl " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0xc0 11 111 rrr @imm8 => "sar " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0xc0 11 000 rrr @imm8 => "rol " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0xc0 11 001 rrr @imm8 => "ror " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0xc0 11 010 rrr @imm8 => "rcl " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0xc0 11 011 rrr @imm8 => "rcr " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0xc0 11 100 rrr @imm8 => "shl " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0xc0 11 101 rrr @imm8 => "shr " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0xc0 11 110 rrr @imm8 => "shl " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0xc0 11 111 rrr @imm8 => "sar " rgb[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0xc1 11 000 rrr @imm8 => "rol " greg[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0xc1 11 001 rrr @imm8 => "ror " greg[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0xc1 11 010 rrr @imm8 => "rcl " greg[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0xc1 11 011 rrr @imm8 => "rcr " greg[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0xc1 11 100 rrr @imm8 => "shl " greg[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0xc1 11 101 rrr @imm8 => "shr " greg[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0xc1 11 110 rrr @imm8 => "shl " greg[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0xc1 11 111 rrr @imm8 => "sar " greg[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0xc1 11 000 rrr @imm8 => "rol " greg[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0xc1 11 001 rrr @imm8 => "ror " greg[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0xc1 11 010 rrr @imm8 => "rcl " greg[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0xc1 11 011 rrr @imm8 => "rcr " greg[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0xc1 11 100 rrr @imm8 => "shl " greg[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0xc1 11 101 rrr @imm8 => "shr " greg[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0xc1 11 110 rrr @imm8 => "shl " greg[$r] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0xc1 11 111 rrr @imm8 => "sar " greg[$r] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0xd0 11 000 rrr => "rol " rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd0 11 001 rrr => "ror " rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd0 11 010 rrr => "rcl " rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd0 11 011 rrr => "rcr " rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd0 11 100 rrr => "shl " rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd0 11 101 rrr => "shr " rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd0 11 110 rrr => "shl " rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd0 11 111 rrr => "sar " rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd0 11 000 rrr => "rol " rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd0 11 001 rrr => "ror " rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd0 11 010 rrr => "rcl " rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd0 11 011 rrr => "rcr " rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd0 11 100 rrr => "shl " rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd0 11 101 rrr => "shr " rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd0 11 110 rrr => "shl " rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd0 11 111 rrr => "sar " rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd1 11 000 rrr => "rol " greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd1 11 001 rrr => "ror " greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd1 11 010 rrr => "rcl " greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd1 11 011 rrr => "rcr " greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd1 11 100 rrr => "shl " greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd1 11 101 rrr => "shr " greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd1 11 110 rrr => "shl " greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd1 11 111 rrr => "sar " greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd1 11 000 rrr => "rol " greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd1 11 001 rrr => "ror " greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd1 11 010 rrr => "rcl " greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd1 11 011 rrr => "rcr " greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd1 11 100 rrr => "shl " greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd1 11 101 rrr => "shr " greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd1 11 110 rrr => "shl " greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd1 11 111 rrr => "sar " greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd2 11 000 rrr => "rol " rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd2 11 001 rrr => "ror " rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd2 11 010 rrr => "rcl " rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd2 11 011 rrr => "rcr " rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd2 11 100 rrr => "shl " rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd2 11 101 rrr => "shr " rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd2 11 110 rrr => "shl " rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd2 11 111 rrr => "sar " rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd2 11 000 rrr => "rol " rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd2 11 001 rrr => "ror " rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd2 11 010 rrr => "rcl " rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd2 11 011 rrr => "rcr " rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd2 11 100 rrr => "shl " rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd2 11 101 rrr => "shr " rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd2 11 110 rrr => "shl " rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd2 11 111 rrr => "sar " rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd3 11 000 rrr => "rol " greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd3 11 001 rrr => "ror " greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd3 11 010 rrr => "rcl " greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd3 11 011 rrr => "rcr " greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd3 11 100 rrr => "shl " greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd3 11 101 rrr => "shr " greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd3 11 110 rrr => "shl " greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd3 11 111 rrr => "sar " greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd3 11 000 rrr => "rol " greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd3 11 001 rrr => "ror " greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd3 11 010 rrr => "rcl " greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd3 11 011 rrr => "rcr " greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd3 11 100 rrr => "shl " greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd3 11 101 rrr => "shr " greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd3 11 110 rrr => "shl " greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xd3 11 111 rrr => "sar " greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xf6 11 010 rrr => "not " rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0xf6 11 011 rrr => "neg " rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0xf6 11 010 rrr => "not " rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0xf6 11 011 rrr => "neg " rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0xf7 11 010 rrr => "not " greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xf7 11 011 rrr => "neg " greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xf7 11 010 rrr => "not " greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xf7 11 011 rrr => "neg " greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xfe 11 000 rrr => "inc " rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0xfe 11 001 rrr => "dec " rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0xfe 11 000 rrr => "inc " rgb[$r] ;
  r x b e f 100 w v j 01 c d n a 0xfe 11 001 rrr => "dec " rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0xff 11 000 rrr => "inc " greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xff 11 001 rrr => "dec " greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xff 11 000 rrr => "inc " greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xff 11 001 rrr => "dec " greg[$r] ;
  # ===== Stage 3c: shld/shrd (NDD; r/m,reg,imm8 or ,cl) =====
  r x b e f 100 w v j 00 c d n a 0x24 11 ggg rrr @imm8 => "shld " greg[$r] "," greg[$g] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0x2c 11 ggg rrr @imm8 => "shrd " greg[$r] "," greg[$g] "," hex($imm8) ;
  r x b e f 100 w v j 00 c d n a 0xa5 11 ggg rrr => "shld " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 00 c d n a 0xad 11 ggg rrr => "shrd " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 01 c d n a 0x24 11 ggg rrr @imm8 => "shld " greg[$r] "," greg[$g] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0x2c 11 ggg rrr @imm8 => "shrd " greg[$r] "," greg[$g] "," hex($imm8) ;
  r x b e f 100 w v j 01 c d n a 0xa5 11 ggg rrr => "shld " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 01 c d n a 0xad 11 ggg rrr => "shrd " greg[$r] "," greg[$g] ;
  # ===== push2/pop2 (ff/6, 8f/0): 64-bit register pair; W picks the {,p} variant =====
  r x b e f 100 0 v j 00 c d n a 0xff 11 110 rrr => "push2 "  greg[$v] "," greg[$r] ;
  r x b e f 100 1 v j 00 c d n a 0xff 11 110 rrr => "push2p " greg[$v] "," greg[$r] ;
  r x b e f 100 0 v j 00 c d n a 0x8f 11 000 rrr => "pop2 "   greg[$v] "," greg[$r] ;
  r x b e f 100 1 v j 00 c d n a 0x8f 11 000 rrr => "pop2p "  greg[$v] "," greg[$r] ;
  # push2/pop2 with the 66 (pp=01) context
  r x b e f 100 0 v j 01 c d n a 0xff 11 110 rrr => "push2 "  greg[$v] "," greg[$r] ;
  r x b e f 100 1 v j 01 c d n a 0xff 11 110 rrr => "push2p " greg[$v] "," greg[$r] ;
  r x b e f 100 0 v j 01 c d n a 0x8f 11 000 rrr => "pop2 "   greg[$v] "," greg[$r] ;
  r x b e f 100 1 v j 01 c d n a 0x8f 11 000 rrr => "pop2p "  greg[$v] "," greg[$r] ;
  # ===== APX map-4 tail: adcx/adox, movbe, crc32, atomics, MSR/CET/VMX, SHA/AES =====
  r x b e f 100 w v j 01 c d n a 0x66 11 ggg rrr => "adcx " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 10 c d n a 0x66 11 ggg rrr => "adox " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0x60 11 ggg rrr => "movbe " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0x60 11 ggg rrr => "movbe " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xf0 11 ggg rrr => "crc32 " greg[$g] "," rgb[$r] ;
  r x b e f 100 w v j 00 c d n a 0xf1 11 ggg rrr => "crc32 " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xf1 11 ggg rrr => "crc32 " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 10 c d n a 0xf8 11 ggg rrr => "uwrmsr " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 10 c d n a 0xf8 @addr => "enqcmds " greg[$g] "," $addr ;
  r x b e f 100 w v j 10 c d n a 0xda 11 ggg rrr => "encodekey128 " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 10 c d n a 0xdb 11 ggg rrr => "encodekey256 " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 10 c d n a 0xf0 11 ggg rrr => "invept " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 10 c d n a 0xf1 11 ggg rrr => "invvpid " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 10 c d n a 0xf2 11 ggg rrr => "invpcid " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 01 c d n a 0xf8 11 ggg rrr => "movdir64b " greg[$g] "," greg[$r] ;
  r x b e f 100 w v j 00 c d n a 0x61 11 ggg rrr => "movbe " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 01 c d n a 0x61 11 ggg rrr => "movbe " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 00 c d n a 0xfc 11 ggg rrr => "aadd " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 01 c d n a 0xfc 11 ggg rrr => "aand " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 10 c d n a 0xfc 11 ggg rrr => "axor " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 11 c d n a 0xfc 11 ggg rrr => "aor " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 00 c d n a 0xf9 11 ggg rrr => "movdiri " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 11 c d n a 0xf8 11 ggg rrr => "urdmsr " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 11 c d n a 0xf8 @addr => "enqcmd " greg[$g] "," $addr ;
  r x b e f 100 0 v j 00 c d n a 0x66 11 ggg rrr => "wrssd " greg[$r] "," greg[$g] ;
  r x b e f 100 1 v j 00 c d n a 0x66 11 ggg rrr => "wrssq " greg[$r] "," greg[$g] ;
  r x b e f 100 0 v j 01 c d n a 0x65 11 ggg rrr => "wrussd " greg[$r] "," greg[$g] ;
  r x b e f 100 1 v j 01 c d n a 0x65 11 ggg rrr => "wrussq " greg[$r] "," greg[$g] ;
  r x b e f 100 w v j 00 c d n a 0xd8 11 ggg rrr => "sha1nexte " ssereg[$g] "," ssereg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd9 11 ggg rrr => "sha1msg1 " ssereg[$g] "," ssereg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xda 11 ggg rrr => "sha1msg2 " ssereg[$g] "," ssereg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xdc 11 ggg rrr => "sha256msg1 " ssereg[$g] "," ssereg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xdd 11 ggg rrr => "sha256msg2 " ssereg[$g] "," ssereg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xdb 11 ggg rrr => "sha256rnds2 " ssereg[$g] "," ssereg[$r] ;
  r x b e f 100 w v j 00 c d n a 0xd4 11 ggg rrr @imm8 => "sha1rnds4 " ssereg[$g] "," ssereg[$r] "," hex($imm8) ;
  r x b e f 100 w v j 10 c d n a 0xdc 11 ggg rrr => "aesenc128kl " ssereg[$g] "," ssereg[$r] ;
  r x b e f 100 w v j 10 c d n a 0xdd 11 ggg rrr => "aesdec128kl " ssereg[$g] "," ssereg[$r] ;
  r x b e f 100 w v j 10 c d n a 0xde 11 ggg rrr => "aesenc256kl " ssereg[$g] "," ssereg[$r] ;
  r x b e f 100 w v j 10 c d n a 0xdf 11 ggg rrr => "aesdec256kl " ssereg[$g] "," ssereg[$r] ;
  r x b e f 100 w v j 10 c d n a 0xd8 @addr(0) => "aesencwide128kl " $addr ;
  r x b e f 100 w v j 10 c d n a 0xd8 @addr(1) => "aesdecwide128kl " $addr ;
  r x b e f 100 w v j 10 c d n a 0xd8 @addr(2) => "aesencwide256kl " $addr ;
  r x b e f 100 w v j 10 c d n a 0xd8 @addr(3) => "aesdecwide256kl " $addr ;
}

submatch main { @pfx(0) => $pfx }























