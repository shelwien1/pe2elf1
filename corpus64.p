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
table movs   { movsd,movsw }
table stos   { stosd,stosw }
table lods   { lodsd,lodsw }
table cmps   { cmpsd,cmpsw }
table scas   { scasd,scasw }
table insx   { insd,insw }
table outsx  { outsd,outsw }
table cbw_t  { cwde,cbw,cdqe }         # 0x98 by ($rexw?2:$opsiz): cwde/cbw/cdqe
table cwd_t  { cdq,cwd,cqo }           # 0x99 by ($rexw?2:$opsiz): cdq/cwd/cqo
table pcnt   { "",popcnt,"" }          # F3 0F B8
table tzt    { "",tzcnt,"" }           # F3 0F BC
table lzt    { "",lzcnt,"" }           # F3 0F BD
table grpba  { "","","","",bt,bts,btr,btc }   # 0F BA /op
table c7r7   { rdseed,rdpid,rdseed,rdseed }   # 0F C7 /7 by reptype
table c7r6   { rdrand,rdrand,vmxon,rdrand }   # 0F C7 /6 reg by reptype
table cx16   { cmpxchg8b, cmpxchg16b }        # 0F C7 /1 by REX.W (m64 / m128)

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
  0x90 [$reptype==1] => "pause" ;
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
  0xe9 @relz => "jmp " hex($relz) ;
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
  0xa5 => movs[$opsiz] ;
  0xaa => "stosb" ;
  0xab => stos[$opsiz] ;
  0xac => "lodsb" ;
  0xad => lods[$opsiz] ;
  0xa6 => "cmpsb" ;
  0xae => "scasb" ;
  0xa7 => cmps[$opsiz] ;
  0xaf => scas[$opsiz] ;

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
}

submatch main { @pfx(0) => $pfx }
