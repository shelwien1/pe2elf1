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
# shifts, imul, jmp/call/jcc, ret, movzx/movsx/movsxd, setcc/cmovcc, groups).
# SSE/AVX legacy-encoded ops are deferred; VEX/EVEX round-trip C++-side already.

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
  # split out because it overrides the 66 prefix for operand size -- and hence for
  # immz/relz immediate width: under REX.W the immediate is always 32-bit (sign-
  # extended), never 16-bit, so reset $opsiz=0 even if a 66 set it. (Operand size
  # 64 itself is applied in C++ from the REX byte; this only fixes FSM imm sizing.)
  0100 0 r x b @pfx($d+1)            => $pfx ;
  0100 1 r x b @pfx($d+1) {$opsiz=0} => $pfx ;
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
}

submatch main { @pfx(0) => $pfx }
