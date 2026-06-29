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
# palignr, pmovsx/zx, roundsd, blends, pcmpistri, pclmulqdq, AES). The mandatory
# 66/F3/F2 SSE selector and rep/lock ride the prefix run (replayed verbatim), so
# prefix variants round-trip byte-exact. x87 (D8-DF) and byte-exact VEX/EVEX
# tails remain future work; VEX/EVEX round-trip today via the C++ structural path.

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
  eax,ecx,edx,ebx,esp,ebp,esi,edi, r8d,r9d,r10d,r11d,r12d,r13d,r14d,r15d,
  ax,cx,dx,bx,sp,bp,si,di, r8w,r9w,r10w,r11w,r12w,r13w,r14w,r15w,
  rax,rcx,rdx,rbx,rsp,rbp,rsi,rdi, r8,r9,r10,r11,r12,r13,r14,r15
}
table rgb { al,cl,dl,bl,spl,bpl,sil,dil, r8b,r9b,r10b,r11b,r12b,r13b,r14b,r15b }
table ssereg { mm0,mm1,mm2,mm3,mm4,mm5,mm6,mm7,
  xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7,xmm8,xmm9,xmm10,xmm11,xmm12,xmm13,xmm14,xmm15 }
table sreg { es,cs,ss,ds,fs,gs }
table cond { o,no,b,ae,e,ne,be,a,s,ns,p,np,l,ge,le,g }

# size suffix : the dot is part of the entry; index = operand size in bytes
table sfx { "",".b",".w","",".d","","","",".q" }

# segment display : index = sbo(base)+segidx. Row sbo=0 is a DS-default base, row
# sbo=7 an SS-default base (rsp/rbp). 64-bit mostly ignores segments but fs/gs.
table seg { "","es:","cs:","ss:","","fs:","gs:",  "ss:","es:","cs:","ss:","ds:","fs:","gs:" }
table sbo { 0,0,0,0,7,7,0,0 }      # base reg number (low 3) -> seg row

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
  0xc0 11 100 rrr @imm8 => "shl " rgb[$r] "," hex($imm8) ;
  0xc0 @addr(4)   @imm8 => "shl" sfx[1] " " $addr "," hex($imm8) ;
  0xc0 11 101 rrr @imm8 => "shr " rgb[$r] "," hex($imm8) ;
  0xc0 @addr(5)   @imm8 => "shr" sfx[1] " " $addr "," hex($imm8) ;
  0xc0 11 111 rrr @imm8 => "sar " rgb[$r] "," hex($imm8) ;
  0xc0 @addr(7)   @imm8 => "sar" sfx[1] " " $addr "," hex($imm8) ;
  0xc1 11 100 rrr @imm8 => "shl " greg[$r] "," hex($imm8) ;
  0xc1 @addr(4)   @imm8 => "shl" sfx[4] " " $addr "," hex($imm8) ;
  0xc1 11 101 rrr @imm8 => "shr " greg[$r] "," hex($imm8) ;
  0xc1 @addr(5)   @imm8 => "shr" sfx[4] " " $addr "," hex($imm8) ;
  0xc1 11 111 rrr @imm8 => "sar " greg[$r] "," hex($imm8) ;
  0xc1 @addr(7)   @imm8 => "sar" sfx[4] " " $addr "," hex($imm8) ;
  0xd1 11 100 rrr => "shl " greg[$r] ",1" ;
  0xd1 @addr(4)   => "shl" sfx[4] " " $addr ",1" ;
  0xd1 11 101 rrr => "shr " greg[$r] ",1" ;
  0xd1 @addr(5)   => "shr" sfx[4] " " $addr ",1" ;
  0xd1 11 111 rrr => "sar " greg[$r] ",1" ;
  0xd1 @addr(7)   => "sar" sfx[4] " " $addr ",1" ;
  0xd3 11 100 rrr => "shl " greg[$r] ",cl" ;
  0xd3 @addr(4)   => "shl" sfx[4] " " $addr ",cl" ;
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
  0xc9 => "leave" ;
  0xcc => "int3" ;
  0xcd @imm8 => "int " hex($imm8) ;

  # ===== call/jmp rel ; group 5 indirect ; group 3 F6/F7 =======================
  0xe8 @relz => "call " hex($relz) ;
  0xe9 @relz => "jmp " hex($relz) ;
  0xeb @rel8 => "jmp " hex($rel8) ;
  0xf6 11 000 rrr @imm8 => "test " rgb[$r] "," hex($imm8) ;
  0xf6 @addr(0)   @imm8 => "test" sfx[1] " " $addr "," hex($imm8) ;
  0xf6 11 010 rrr => "not " rgb[$r] ;
  0xf6 @addr(2)   => "not" sfx[1] " " $addr ;
  0xf6 11 011 rrr => "neg " rgb[$r] ;
  0xf6 @addr(3)   => "neg" sfx[1] " " $addr ;
  0xf7 11 000 rrr @immz => "test " greg[$r] "," hex($immz) ;
  0xf7 @addr(0)   @immz => "test" sfx[4] " " $addr "," hex($immz) ;
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
  0xff 11 100 rrr => "jmp " greg[$r] ;
  0xff @addr(4)   => "jmp " $addr ;
  0xff 11 110 rrr => "push " greg[$r] ;
  0xff @addr(6)   => "push " $addr ;

  # ===== push/pop r64 (default-64; embedded register 50+r / 58+r) ==============
  01010 bbb => "push " greg[$b] ;
  01011 bbb => "pop " greg[$b] ;

  # ===== 0F two-byte map ========================================================
  0x0f 0x05 => "syscall" ;
  0x0f 0x0b => "ud2" ;
  0x0f 0x1f 11 ggg rrr => "nop " greg[$r] ;
  0x0f 0x1f @addr(0)   => "nop" sfx[4] " " $addr ;
  0x0f 1000 cccc @relz => "j" cond[$c] " " hex($relz) ;
  0x0f 1001 cccc 11 ggg rrr => "set" cond[$c] " " rgb[$r] ;
  0x0f 1001 cccc @addr      => "set" cond[$c] sfx[1] " " $addr ;
  0x0f 0100 cccc 11 ggg rrr => "cmov" cond[$c] " " greg[$g] "," greg[$r] ;
  0x0f 0100 cccc @addr      => "cmov" cond[$c] " " greg[$g] "," $addr ;
  0x0f 0xaf 11 ggg rrr => "imul " greg[$g] "," greg[$r] ;
  0x0f 0xaf @addr      => "imul " greg[$g] "," $addr ;
  0x0f 0xb6 11 ggg rrr => "movzx " greg[$g] "," rgb[$r] ;
  0x0f 0xb6 @addr      => "movzx " greg[$g] "," $addr ;
  0x0f 0xb7 11 ggg rrr => "movzx " greg[$g] "," greg[$r] ;
  0x0f 0xb7 @addr      => "movzx " greg[$g] "," $addr ;
  0x0f 0xbe 11 ggg rrr => "movsx " greg[$g] "," rgb[$r] ;
  0x0f 0xbe @addr      => "movsx " greg[$g] "," $addr ;
  0x0f 0xbf 11 ggg rrr => "movsx " greg[$g] "," greg[$r] ;
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
  0x0f 0xbc 11 ggg rrr => "bsf " greg[$g] "," greg[$r] ;
  0x0f 0xbc @addr      => "bsf " greg[$g] "," $addr ;
  0x0f 0xbd 11 ggg rrr => "bsr " greg[$g] "," greg[$r] ;
  0x0f 0xbd @addr      => "bsr " greg[$g] "," $addr ;
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
  0x0f 11001 bbb       => "bswap " greg[$b] ;

  # ===== 0F AE fences / clflush / fxsave-xsave ; 0F 18 prefetch/nop ; 0F 1E endbr
  # (the CET endbr64/endbr32 = F3 0F 1E FA/FB; the F3 rides in the prefix run, so
  #  the base 0F 1E modrm rule round-trips them byte-exact). ====================
  0x0f 0xae 11 101 rrr => "lfence" ;
  0x0f 0xae 11 110 rrr => "mfence" ;
  0x0f 0xae 11 111 rrr => "sfence" ;
  0x0f 0xae @addr(0)   => "fxsave " $addr ;
  0x0f 0xae @addr(1)   => "fxrstor " $addr ;
  0x0f 0xae @addr(2)   => "ldmxcsr " $addr ;
  0x0f 0xae @addr(3)   => "stmxcsr " $addr ;
  0x0f 0xae @addr(4)   => "xsave " $addr ;
  0x0f 0xae @addr(5)   => "xrstor " $addr ;
  0x0f 0xae @addr(6)   => "xsaveopt " $addr ;
  0x0f 0xae @addr(7)   => "clflush" sfx[1] " " $addr ;
  0x0f 0x18 @addr(0)   => "prefetchnta" sfx[1] " " $addr ;
  0x0f 0x18 @addr(1)   => "prefetcht0" sfx[1] " " $addr ;
  0x0f 0x18 @addr(2)   => "prefetcht1" sfx[1] " " $addr ;
  0x0f 0x18 @addr(3)   => "prefetcht2" sfx[1] " " $addr ;
  0x0f 0x18 @addr(4)   => "nop18 " $addr ;
  0x0f 0x18 @addr(5)   => "nop18 " $addr ;
  0x0f 0x18 @addr(6)   => "nop18 " $addr ;
  0x0f 0x18 @addr(7)   => "nop18 " $addr ;
  # 0F 1E reg form: covers endbr64/endbr32 (F3 0F 1E FA/FB) and rdssp; the dead
  # reg field rides the reg_w witness so the modrm round-trips exactly.
  0x0f 0x1e 11 ggg rrr => "endbr " greg[$r] ;

  # ===== SSE/SSE2 (legacy-encoded, xmm-only). The mandatory 66/F3/F2 prefix
  # selects the pd/ss/sd variant and rides in the prefix run (shown as a prefix
  # token), so one base rule per opcode round-trips all four variants byte-exact.
  # Operands draw from the fixed XMM bank (ssereg[8+..]); the reg<->mem direction
  # rides the enc/dir witness, the mod field rides the modrm capture. ===========
  0x0f 0x10 11 ggg rrr => "movups " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x10 @addr      => "movups " ssereg[8+$g] "," $addr ;
  0x0f 0x11 11 ggg rrr => "movups " ssereg[8+$r] "," ssereg[8+$g] ;
  0x0f 0x11 @addr      => "movups " $addr "," ssereg[8+$g] ;
  0x0f 0x12 11 ggg rrr => "movlps " ssereg[8+$g] "," ssereg[8+$r] ;   # movhlps (reg form)
  0x0f 0x12 @addr      => "movlps " ssereg[8+$g] "," $addr ;
  0x0f 0x13 @addr      => "movlps " $addr "," ssereg[8+$g] ;
  0x0f 0x14 11 ggg rrr => "unpcklps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x14 @addr      => "unpcklps " ssereg[8+$g] "," $addr ;
  0x0f 0x15 11 ggg rrr => "unpckhps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x15 @addr      => "unpckhps " ssereg[8+$g] "," $addr ;
  0x0f 0x16 11 ggg rrr => "movhps " ssereg[8+$g] "," ssereg[8+$r] ;   # movlhps (reg form)
  0x0f 0x16 @addr      => "movhps " ssereg[8+$g] "," $addr ;
  0x0f 0x17 @addr      => "movhps " $addr "," ssereg[8+$g] ;
  0x0f 0x28 11 ggg rrr => "movaps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x28 @addr      => "movaps " ssereg[8+$g] "," $addr ;
  0x0f 0x29 11 ggg rrr => "movaps " ssereg[8+$r] "," ssereg[8+$g] ;
  0x0f 0x29 @addr      => "movaps " $addr "," ssereg[8+$g] ;
  0x0f 0x2e 11 ggg rrr => "ucomiss " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x2e @addr      => "ucomiss " ssereg[8+$g] "," $addr ;
  0x0f 0x2f 11 ggg rrr => "comiss " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x2f @addr      => "comiss " ssereg[8+$g] "," $addr ;
  0x0f 0x51 11 ggg rrr => "sqrtps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x51 @addr      => "sqrtps " ssereg[8+$g] "," $addr ;
  0x0f 0x52 11 ggg rrr => "rsqrtps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x52 @addr      => "rsqrtps " ssereg[8+$g] "," $addr ;
  0x0f 0x53 11 ggg rrr => "rcpps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x53 @addr      => "rcpps " ssereg[8+$g] "," $addr ;
  0x0f 0x54 11 ggg rrr => "andps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x54 @addr      => "andps " ssereg[8+$g] "," $addr ;
  0x0f 0x55 11 ggg rrr => "andnps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x55 @addr      => "andnps " ssereg[8+$g] "," $addr ;
  0x0f 0x56 11 ggg rrr => "orps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x56 @addr      => "orps " ssereg[8+$g] "," $addr ;
  0x0f 0x57 11 ggg rrr => "xorps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x57 @addr      => "xorps " ssereg[8+$g] "," $addr ;
  0x0f 0x58 11 ggg rrr => "addps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x58 @addr      => "addps " ssereg[8+$g] "," $addr ;
  0x0f 0x59 11 ggg rrr => "mulps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x59 @addr      => "mulps " ssereg[8+$g] "," $addr ;
  0x0f 0x5a 11 ggg rrr => "cvtps2pd " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5a @addr      => "cvtps2pd " ssereg[8+$g] "," $addr ;
  0x0f 0x5b 11 ggg rrr => "cvtdq2ps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5b @addr      => "cvtdq2ps " ssereg[8+$g] "," $addr ;
  0x0f 0x5c 11 ggg rrr => "subps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5c @addr      => "subps " ssereg[8+$g] "," $addr ;
  0x0f 0x5d 11 ggg rrr => "minps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5d @addr      => "minps " ssereg[8+$g] "," $addr ;
  0x0f 0x5e 11 ggg rrr => "divps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5e @addr      => "divps " ssereg[8+$g] "," $addr ;
  0x0f 0x5f 11 ggg rrr => "maxps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5f @addr      => "maxps " ssereg[8+$g] "," $addr ;
  0x0f 0xc2 11 ggg rrr @imm8 => "cmpps " ssereg[8+$g] "," ssereg[8+$r] "," hex($imm8) ;
  0x0f 0xc2 @addr      @imm8 => "cmpps " ssereg[8+$g] "," $addr "," hex($imm8) ;
  0x0f 0xc6 11 ggg rrr @imm8 => "shufps " ssereg[8+$g] "," ssereg[8+$r] "," hex($imm8) ;
  0x0f 0xc6 @addr      @imm8 => "shufps " ssereg[8+$g] "," $addr "," hex($imm8) ;

  # ===== SSE2 GPR<->xmm moves, movdq, packed integer. ssereg[$opsiz*8+..] =
  # OPF_SSE_OS (mm at opsiz 0, xmm at 1/2); greg[$r] takes REX.W as a 64-bit GPR
  # (movq / cvtsi2sd r64). The 66/F3/F2 selector rides the prefix run, so one base
  # rule per opcode covers movd/movq, movdqa/movdqu, cvtsi2ss/sd etc. byte-exact. =
  0x0f 0x6e 11 ggg rrr => "movd " ssereg[$opsiz*8+$g] "," greg[$r] ;
  0x0f 0x6e @addr      => "movd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x7e 11 ggg rrr => "movd " greg[$r] "," ssereg[$opsiz*8+$g] ;
  0x0f 0x7e @addr      => "movd " $addr "," ssereg[$opsiz*8+$g] ;
  0x0f 0x6f 11 ggg rrr => "movdqa " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x6f @addr      => "movdqa " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x7f 11 ggg rrr => "movdqa " ssereg[$opsiz*8+$r] "," ssereg[$opsiz*8+$g] ;
  0x0f 0x7f @addr      => "movdqa " $addr "," ssereg[$opsiz*8+$g] ;
  0x0f 0xd6 11 ggg rrr => "movq " ssereg[8+$r] "," ssereg[8+$g] ;
  0x0f 0xd6 @addr      => "movq " $addr "," ssereg[8+$g] ;
  0x0f 0x2a 11 ggg rrr => "cvtsi2ss " ssereg[8+$g] "," greg[$r] ;
  0x0f 0x2a @addr      => "cvtsi2ss " ssereg[8+$g] "," $addr ;
  0x0f 0x2c 11 ggg rrr => "cvttss2si " greg[$g] "," ssereg[8+$r] ;
  0x0f 0x2c @addr      => "cvttss2si " greg[$g] "," $addr ;
  0x0f 0x2d 11 ggg rrr => "cvtss2si " greg[$g] "," ssereg[8+$r] ;
  0x0f 0x2d @addr      => "cvtss2si " greg[$g] "," $addr ;
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
  0x0f 0x70 11 ggg rrr @imm8 => "pshufd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x70 @addr      @imm8 => "pshufd " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x50 11 ggg rrr => "movmskps " greg[$g] "," ssereg[8+$r] ;
  0x0f 0xc4 11 ggg rrr @imm8 => "pinsrw " ssereg[$opsiz*8+$g] "," greg[$r] "," hex($imm8) ;
  0x0f 0xc4 @addr      @imm8 => "pinsrw " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0xc5 11 ggg rrr @imm8 => "pextrw " greg[$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0xd1 11 ggg rrr => "psrlw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd1 @addr      => "psrlw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd2 11 ggg rrr => "psrld " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd2 @addr      => "psrld " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd3 11 ggg rrr => "psrlq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd3 @addr      => "psrlq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd5 11 ggg rrr => "pmullw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd5 @addr      => "pmullw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd7 11 ggg rrr => "pmovmskb " greg[$g] "," ssereg[$opsiz*8+$r] ;
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
  0x0f 0xe7 @addr      => "movntdq " $addr "," ssereg[$opsiz*8+$g] ;
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
  0x0f 0xf7 11 ggg rrr => "maskmovdqu " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x2b @addr      => "movntps " $addr "," ssereg[8+$g] ;

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
  0x0f 0x38 0x04 11 ggg rrr => "pmaddubsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x04 @addr      => "pmaddubsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x05 11 ggg rrr => "phsubw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x05 @addr      => "phsubw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x06 11 ggg rrr => "phsubd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x06 @addr      => "phsubd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x08 11 ggg rrr => "psignb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x08 @addr      => "psignb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x09 11 ggg rrr => "psignw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x09 @addr      => "psignw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x0a 11 ggg rrr => "psignd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x0a @addr      => "psignd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x0b 11 ggg rrr => "pmulhrsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x0b @addr      => "pmulhrsw " ssereg[$opsiz*8+$g] "," $addr ;
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
  0x0f 0x38 0xdc 11 ggg rrr => "aesenc " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0xdc @addr      => "aesenc " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0xdd 11 ggg rrr => "aesenclast " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0xdd @addr      => "aesenclast " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0xde 11 ggg rrr => "aesdec " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0xde @addr      => "aesdec " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0xdf 11 ggg rrr => "aesdeclast " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0xdf @addr      => "aesdeclast " ssereg[$opsiz*8+$g] "," $addr ;
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
  0x0f 0x3a 0x14 11 ggg rrr @imm8 => "pextrb " greg[$r] "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x14 @addr      @imm8 => "pextrb " $addr "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x16 11 ggg rrr @imm8 => "pextrd " greg[$r] "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x16 @addr      @imm8 => "pextrd " $addr "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x17 11 ggg rrr @imm8 => "extractps " greg[$r] "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x17 @addr      @imm8 => "extractps " $addr "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x20 11 ggg rrr @imm8 => "pinsrb " ssereg[$opsiz*8+$g] "," greg[$r] "," hex($imm8) ;
  0x0f 0x3a 0x20 @addr      @imm8 => "pinsrb " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x21 11 ggg rrr @imm8 => "insertps " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x21 @addr      @imm8 => "insertps " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x22 11 ggg rrr @imm8 => "pinsrd " ssereg[$opsiz*8+$g] "," greg[$r] "," hex($imm8) ;
  0x0f 0x3a 0x22 @addr      @imm8 => "pinsrd " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x40 11 ggg rrr @imm8 => "dpps " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x40 @addr      @imm8 => "dpps " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x41 11 ggg rrr @imm8 => "dppd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x41 @addr      @imm8 => "dppd " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x42 11 ggg rrr @imm8 => "mpsadbw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x42 @addr      @imm8 => "mpsadbw " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x44 11 ggg rrr @imm8 => "pclmulqdq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x44 @addr      @imm8 => "pclmulqdq " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
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
  0x0f 0x7c 11 ggg rrr => "haddps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x7c @addr      => "haddps " ssereg[8+$g] "," $addr ;
  0x0f 0x7d 11 ggg rrr => "hsubps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x7d @addr      => "hsubps " ssereg[8+$g] "," $addr ;
  0x0f 0xd0 11 ggg rrr => "addsubps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0xd0 @addr      => "addsubps " ssereg[8+$g] "," $addr ;
  0x0f 0xf0 @addr      => "lddqu " ssereg[8+$g] "," $addr ;
}

submatch main { @pfx(0) => $pfx }
