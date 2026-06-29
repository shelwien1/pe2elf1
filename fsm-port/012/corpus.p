# corpus.p (v3a, disassembly-only) -- bit-pattern -> string rewriter.  [rev7]
#
# TWO constructs: table and submatch. Interpolated templates glue them. Engine
# knows no x86. Disassembly only.
#
#   <...>  matches the bits of the prefix-dispatch value PASSED to a submatch,
#          MSB-first. No global state is read here -- each `...1` submatch is a
#          pure function of its arguments, and the thin wrappers addr/immz/relz
#          inject the current $adrsiz/$opsiz so call sites stay clean. For our
#          submatches the passed value is a single bit, so it is just <0>/<1>.
#
# A submatch parameter that unifies with a stream field is BIDIRECTIONAL: bound
# at the call -> the field is constrained; free -> the field is captured. addr's
# reg field $g works this way: @addr leaves it free (the reg operand), @addr(N)
# binds it (opcode-extension groups, /N). The prefix value param ($adrsiz/$opsiz)
# has no stream field, so it is the one `<...>` matches.
#
# PREFIX-TOKEN RECOVERY is automatic: a prefix's token is suppressed when the
# render emits a token whose *identity* that prefix determined --
#     opsiz   : a greg upper-half (16-bit) register, or a 16-bit rm16 form
#     adrsiz  : a 16-bit rm16 form
#     segidx  : a non-empty seg[] entry (a segment override `xx:`)
#     reptype : a mnemonic from a prefix-indexed table
#   A plain number is identity-stable, so `66 68`->`push 0x1234` and `67`+
#   absolute keep their token. On multiple overrides the LAST emit wins:
#   `36 2e 2e ...` -> `segss segcs mov eax,cs:[eax]` (corpus 305).
#
# Stream bytes are bare bits. ModR/M ops are TWO rules: mod=11 (register, inline
# table, no suffix) and @addr (memory-only, suffix always). $S/$E = insn start/end.

arch  $mode=32 $endian=le $bitorder=msb $maxlen=15
vars  $opsiz=0 $adrsiz=0 $reptype=0 $segidx=0 $lock=0

# ---- register / addressing tables : size-pairs merged, 16-bit in upper half --
table greg   { eax,ecx,edx,ebx,esp,ebp,esi,edi, ax,cx,dx,bx,sp,bp,si,di }
table rgb    { al,cl,dl,bl,ah,ch,dh,bh }
table ssereg { mm0,mm1,mm2,mm3,mm4,mm5,mm6,mm7, xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7 }
table sreg   { es,cs,ss,ds,fs,gs }
table rm16   { bx+si,bx+di,bp+si,bp+di,si,di,bp,bx }
table cond   { o,no,b,ae,e,ne,be,a,s,ns,p,np,l,ge,le,g }

# ---- size suffix : the dot is part of the entry; index = operand size in bytes
table sfx    { "",".b",".w","",".d","","","",".q","",".t" }

# ---- segment display : index = sbo(base)+segidx. Row sbo=0 is a DS-default base
# (override shown unless effective==ds); row sbo=7 is an SS-default base (esp/ebp,
# or 16-bit bp), where the implicit ss is shown and a ds override prints `ds:`.
table seg    { "","es:","cs:","ss:","","fs:","gs:",  "ss:","es:","cs:","ss:","ds:","fs:","gs:" }
table sbo    { 0,0,0,0,7,7,0,0 }      # 32-bit base reg -> seg row; no-base(none)=0
table sbo16  { 0,0,7,7,0,0,7,0 }      # 16-bit rm16 form -> seg row (bp-forms = ss)

# ---- prefix-indexed mnemonic tables (mnemonic identity recovers the prefix) ---
table m10    { movups,movss,movsd }     # 0F 10 by reptype
table m10sz  { 16,4,8 }
table m54    { andps,andpd }            # 0F 54 by opsiz
table uc     { ucomiss,ucomisd }        # 0F 2E by opsiz
table movs   { movsd,movsw }            # 0xa5 by opsiz
table cdqw   { cdq,cwd }                # 0x99 by opsiz
table elt    { ps,pd,ss,ss,sd,sd }      # ssef element by $reptype*2+$opsiz
table cbwe   { cwde,cbw }              # 0x98 by opsiz
table cmps   { cmpsd,cmpsw }            # 0xa7 by opsiz
table stos   { stosd,stosw }            # 0xab by opsiz
table lods   { lodsd,lodsw }            # 0xad by opsiz
table scas   { scasd,scasw }            # 0xaf by opsiz

# ---- immediates / displacements : the width-dispatch <...> matches the arg ----
submatch imm8  { iiiiiiii                              => $i }
submatch imm16 { iiiiiiii iiiiiiii                     => $i }
submatch imm32 { iiiiiiii iiiiiiii iiiiiiii iiiiiiii   => $i }
submatch disp8 { dddddddd                              => sx8($d) }
submatch disp16{ dddddddd dddddddd                     => sx16($d) }
submatch disp32{ dddddddd dddddddd dddddddd dddddddd   => $d }
submatch rel8  { dddddddd                              => $E + sx8($d) }
submatch immz1($opsiz) { <0> @imm32 => $imm32 ;  <1> @imm16 => $imm16 }
submatch relz1($opsiz) { <0> @imm32 => $E+sx32($imm32) ;  <1> @imm16 => $E+sx16($imm16) }
submatch immz { @immz1($opsiz) => $immz1 }                # inject current opsiz
submatch relz { @relz1($opsiz) => $relz1 }

# ---- prefix run : depth-bounded; each frame just prepends its token ----------
submatch pfx($d) {
  [$d >= $maxlen] => "" ;                      # 15 prefix bytes: standalone insn
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
       @insn => $insn ;
}

# ---- addressing : MEMORY ONLY (mod != 11). addr1 takes the adrsiz bit (matched
# by <...>) and the reg field $g (bidirectional). addr() injects current adrsiz.
# Segment is inline: seg[row+segidx], row = sbo(base), or seg[$segidx] if none.
submatch addr($g) { @addr1($adrsiz, $g) => $addr1 }
submatch addr1($adrsiz, $g) {
  <0> 00 ggg 100 @sib0         => seg[sbo[$sbase]+$segidx] $sib0 ;
  <0> 00 ggg 101 @disp32       => seg[$segidx] "[" hex($disp32) "]" ;
  <0> 00 ggg rrr               => seg[sbo[$r]+$segidx] "[" greg[$r] "]" ;
  <0> 01 ggg 100 @sib1 @disp8  => seg[sbo[$sbase]+$segidx] "[" $sib1 sgn($disp8) "]" ;
  <0> 01 ggg rrr @disp8        => seg[sbo[$r]+$segidx] "[" greg[$r] sgn($disp8) "]" ;
  <0> 10 ggg 100 @sib1 @disp32 => seg[sbo[$sbase]+$segidx] "[" $sib1 sgn($disp32) "]" ;
  <0> 10 ggg rrr @disp32       => seg[sbo[$r]+$segidx] "[" greg[$r] sgn($disp32) "]" ;
  <1> 00 ggg 110 @disp16       => seg[$segidx] "[" hex($disp16) "]" ;
  <1> 00 ggg rrr               => seg[sbo16[$r]+$segidx] "[" rm16[$r] "]" ;
  <1> 01 ggg rrr @disp8        => seg[sbo16[$r]+$segidx] "[" rm16[$r] sgn($disp8) "]" ;
  <1> 10 ggg rrr @disp16       => seg[sbo16[$r]+$segidx] "[" rm16[$r] sgn($disp16) "]" ;
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
# Every ModR/M op = one mod=11 rule (inline table, no suffix) + one @addr rule
# (memory, suffix from sfx[]). Shown for a representative set; all others alike.
submatch insn {
  # ===== ALU: 8 ops x {r/m8,r8 | r/m,r | r8,r/m8 | r,r/m | al,imm8 | eax,immz} =====
  0x00 11 ggg rrr => "add " rgb[$r] "," rgb[$g] ;
  0x00 @addr      => "add" sfx[1] " " $addr "," rgb[$g] ;
  0x01 11 ggg rrr => "add " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x01 @addr      => "add " $addr "," greg[$opsiz*8+$g] ;
  0x02 11 ggg rrr => "add " rgb[$g] "," rgb[$r] ;
  0x02 @addr      => "add " rgb[$g] "," $addr ;
  0x03 11 ggg rrr => "add " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x03 @addr      => "add " greg[$opsiz*8+$g] "," $addr ;
  0x04 @imm8 => "add al," hex($imm8) ;
  0x05 @immz => "add " greg[$opsiz*8+0] "," hex($immz) ;
  0x08 11 ggg rrr => "or " rgb[$r] "," rgb[$g] ;
  0x08 @addr      => "or" sfx[1] " " $addr "," rgb[$g] ;
  0x09 11 ggg rrr => "or " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x09 @addr      => "or " $addr "," greg[$opsiz*8+$g] ;
  0x0a 11 ggg rrr => "or " rgb[$g] "," rgb[$r] ;
  0x0a @addr      => "or " rgb[$g] "," $addr ;
  0x0b 11 ggg rrr => "or " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x0b @addr      => "or " greg[$opsiz*8+$g] "," $addr ;
  0x0c @imm8 => "or al," hex($imm8) ;
  0x0d @immz => "or " greg[$opsiz*8+0] "," hex($immz) ;
  0x10 11 ggg rrr => "adc " rgb[$r] "," rgb[$g] ;
  0x10 @addr      => "adc" sfx[1] " " $addr "," rgb[$g] ;
  0x11 11 ggg rrr => "adc " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x11 @addr      => "adc " $addr "," greg[$opsiz*8+$g] ;
  0x12 11 ggg rrr => "adc " rgb[$g] "," rgb[$r] ;
  0x12 @addr      => "adc " rgb[$g] "," $addr ;
  0x13 11 ggg rrr => "adc " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x13 @addr      => "adc " greg[$opsiz*8+$g] "," $addr ;
  0x14 @imm8 => "adc al," hex($imm8) ;
  0x15 @immz => "adc " greg[$opsiz*8+0] "," hex($immz) ;
  0x18 11 ggg rrr => "sbb " rgb[$r] "," rgb[$g] ;
  0x18 @addr      => "sbb" sfx[1] " " $addr "," rgb[$g] ;
  0x19 11 ggg rrr => "sbb " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x19 @addr      => "sbb " $addr "," greg[$opsiz*8+$g] ;
  0x1a 11 ggg rrr => "sbb " rgb[$g] "," rgb[$r] ;
  0x1a @addr      => "sbb " rgb[$g] "," $addr ;
  0x1b 11 ggg rrr => "sbb " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x1b @addr      => "sbb " greg[$opsiz*8+$g] "," $addr ;
  0x1c @imm8 => "sbb al," hex($imm8) ;
  0x1d @immz => "sbb " greg[$opsiz*8+0] "," hex($immz) ;
  0x20 11 ggg rrr => "and " rgb[$r] "," rgb[$g] ;
  0x20 @addr      => "and" sfx[1] " " $addr "," rgb[$g] ;
  0x21 11 ggg rrr => "and " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x21 @addr      => "and " $addr "," greg[$opsiz*8+$g] ;
  0x22 11 ggg rrr => "and " rgb[$g] "," rgb[$r] ;
  0x22 @addr      => "and " rgb[$g] "," $addr ;
  0x23 11 ggg rrr => "and " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x23 @addr      => "and " greg[$opsiz*8+$g] "," $addr ;
  0x24 @imm8 => "and al," hex($imm8) ;
  0x25 @immz => "and " greg[$opsiz*8+0] "," hex($immz) ;
  0x28 11 ggg rrr => "sub " rgb[$r] "," rgb[$g] ;
  0x28 @addr      => "sub" sfx[1] " " $addr "," rgb[$g] ;
  0x29 11 ggg rrr => "sub " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x29 @addr      => "sub " $addr "," greg[$opsiz*8+$g] ;
  0x2a 11 ggg rrr => "sub " rgb[$g] "," rgb[$r] ;
  0x2a @addr      => "sub " rgb[$g] "," $addr ;
  0x2b 11 ggg rrr => "sub " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x2b @addr      => "sub " greg[$opsiz*8+$g] "," $addr ;
  0x2c @imm8 => "sub al," hex($imm8) ;
  0x2d @immz => "sub " greg[$opsiz*8+0] "," hex($immz) ;
  0x30 11 ggg rrr => "xor " rgb[$r] "," rgb[$g] ;
  0x30 @addr      => "xor" sfx[1] " " $addr "," rgb[$g] ;
  0x31 11 ggg rrr => "xor " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x31 @addr      => "xor " $addr "," greg[$opsiz*8+$g] ;
  0x32 11 ggg rrr => "xor " rgb[$g] "," rgb[$r] ;
  0x32 @addr      => "xor " rgb[$g] "," $addr ;
  0x33 11 ggg rrr => "xor " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x33 @addr      => "xor " greg[$opsiz*8+$g] "," $addr ;
  0x34 @imm8 => "xor al," hex($imm8) ;
  0x35 @immz => "xor " greg[$opsiz*8+0] "," hex($immz) ;
  0x38 11 ggg rrr => "cmp " rgb[$r] "," rgb[$g] ;
  0x38 @addr      => "cmp" sfx[1] " " $addr "," rgb[$g] ;
  0x39 11 ggg rrr => "cmp " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x39 @addr      => "cmp " $addr "," greg[$opsiz*8+$g] ;
  0x3a 11 ggg rrr => "cmp " rgb[$g] "," rgb[$r] ;
  0x3a @addr      => "cmp " rgb[$g] "," $addr ;
  0x3b 11 ggg rrr => "cmp " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x3b @addr      => "cmp " greg[$opsiz*8+$g] "," $addr ;
  0x3c @imm8 => "cmp al," hex($imm8) ;
  0x3d @immz => "cmp " greg[$opsiz*8+0] "," hex($immz) ;
  # ALU imm groups 80(r/m8,imm8) 81(r/m,immz) 82(r/m8,imm8 alias) 83(r/m,imm8sx)
  0x80 11 000 rrr @imm8 => "add " rgb[$r] "," hex($imm8) ;
  0x80 @addr(0)   @imm8 => "add" sfx[1] " " $addr "," hex($imm8) ;
  0x82 11 000 rrr @imm8 => "add " rgb[$r] "," hex($imm8) ;
  0x82 @addr(0)   @imm8 => "add" sfx[1] " " $addr "," hex($imm8) ;
  0x81 11 000 rrr @immz => "add " greg[$opsiz*8+$r] "," hex($immz) ;
  0x81 @addr(0)   @immz => "add" sfx[4] " " $addr "," hex($immz) ;
  0x83 11 000 rrr @imm8 => "add " greg[$opsiz*8+$r] "," hex(sx8($imm8)) ;
  0x83 @addr(0)   @imm8 => "add" sfx[4] " " $addr "," hex(sx8($imm8)) ;
  0x80 11 001 rrr @imm8 => "or " rgb[$r] "," hex($imm8) ;
  0x80 @addr(1)   @imm8 => "or" sfx[1] " " $addr "," hex($imm8) ;
  0x82 11 001 rrr @imm8 => "or " rgb[$r] "," hex($imm8) ;
  0x82 @addr(1)   @imm8 => "or" sfx[1] " " $addr "," hex($imm8) ;
  0x81 11 001 rrr @immz => "or " greg[$opsiz*8+$r] "," hex($immz) ;
  0x81 @addr(1)   @immz => "or" sfx[4] " " $addr "," hex($immz) ;
  0x83 11 001 rrr @imm8 => "or " greg[$opsiz*8+$r] "," hex(sx8($imm8)) ;
  0x83 @addr(1)   @imm8 => "or" sfx[4] " " $addr "," hex(sx8($imm8)) ;
  0x80 11 010 rrr @imm8 => "adc " rgb[$r] "," hex($imm8) ;
  0x80 @addr(2)   @imm8 => "adc" sfx[1] " " $addr "," hex($imm8) ;
  0x82 11 010 rrr @imm8 => "adc " rgb[$r] "," hex($imm8) ;
  0x82 @addr(2)   @imm8 => "adc" sfx[1] " " $addr "," hex($imm8) ;
  0x81 11 010 rrr @immz => "adc " greg[$opsiz*8+$r] "," hex($immz) ;
  0x81 @addr(2)   @immz => "adc" sfx[4] " " $addr "," hex($immz) ;
  0x83 11 010 rrr @imm8 => "adc " greg[$opsiz*8+$r] "," hex(sx8($imm8)) ;
  0x83 @addr(2)   @imm8 => "adc" sfx[4] " " $addr "," hex(sx8($imm8)) ;
  0x80 11 011 rrr @imm8 => "sbb " rgb[$r] "," hex($imm8) ;
  0x80 @addr(3)   @imm8 => "sbb" sfx[1] " " $addr "," hex($imm8) ;
  0x82 11 011 rrr @imm8 => "sbb " rgb[$r] "," hex($imm8) ;
  0x82 @addr(3)   @imm8 => "sbb" sfx[1] " " $addr "," hex($imm8) ;
  0x81 11 011 rrr @immz => "sbb " greg[$opsiz*8+$r] "," hex($immz) ;
  0x81 @addr(3)   @immz => "sbb" sfx[4] " " $addr "," hex($immz) ;
  0x83 11 011 rrr @imm8 => "sbb " greg[$opsiz*8+$r] "," hex(sx8($imm8)) ;
  0x83 @addr(3)   @imm8 => "sbb" sfx[4] " " $addr "," hex(sx8($imm8)) ;
  0x80 11 100 rrr @imm8 => "and " rgb[$r] "," hex($imm8) ;
  0x80 @addr(4)   @imm8 => "and" sfx[1] " " $addr "," hex($imm8) ;
  0x82 11 100 rrr @imm8 => "and " rgb[$r] "," hex($imm8) ;
  0x82 @addr(4)   @imm8 => "and" sfx[1] " " $addr "," hex($imm8) ;
  0x81 11 100 rrr @immz => "and " greg[$opsiz*8+$r] "," hex($immz) ;
  0x81 @addr(4)   @immz => "and" sfx[4] " " $addr "," hex($immz) ;
  0x83 11 100 rrr @imm8 => "and " greg[$opsiz*8+$r] "," hex(sx8($imm8)) ;
  0x83 @addr(4)   @imm8 => "and" sfx[4] " " $addr "," hex(sx8($imm8)) ;
  0x80 11 101 rrr @imm8 => "sub " rgb[$r] "," hex($imm8) ;
  0x80 @addr(5)   @imm8 => "sub" sfx[1] " " $addr "," hex($imm8) ;
  0x82 11 101 rrr @imm8 => "sub " rgb[$r] "," hex($imm8) ;
  0x82 @addr(5)   @imm8 => "sub" sfx[1] " " $addr "," hex($imm8) ;
  0x81 11 101 rrr @immz => "sub " greg[$opsiz*8+$r] "," hex($immz) ;
  0x81 @addr(5)   @immz => "sub" sfx[4] " " $addr "," hex($immz) ;
  0x83 11 101 rrr @imm8 => "sub " greg[$opsiz*8+$r] "," hex(sx8($imm8)) ;
  0x83 @addr(5)   @imm8 => "sub" sfx[4] " " $addr "," hex(sx8($imm8)) ;
  0x80 11 110 rrr @imm8 => "xor " rgb[$r] "," hex($imm8) ;
  0x80 @addr(6)   @imm8 => "xor" sfx[1] " " $addr "," hex($imm8) ;
  0x82 11 110 rrr @imm8 => "xor " rgb[$r] "," hex($imm8) ;
  0x82 @addr(6)   @imm8 => "xor" sfx[1] " " $addr "," hex($imm8) ;
  0x81 11 110 rrr @immz => "xor " greg[$opsiz*8+$r] "," hex($immz) ;
  0x81 @addr(6)   @immz => "xor" sfx[4] " " $addr "," hex($immz) ;
  0x83 11 110 rrr @imm8 => "xor " greg[$opsiz*8+$r] "," hex(sx8($imm8)) ;
  0x83 @addr(6)   @imm8 => "xor" sfx[4] " " $addr "," hex(sx8($imm8)) ;
  0x80 11 111 rrr @imm8 => "cmp " rgb[$r] "," hex($imm8) ;
  0x80 @addr(7)   @imm8 => "cmp" sfx[1] " " $addr "," hex($imm8) ;
  0x82 11 111 rrr @imm8 => "cmp " rgb[$r] "," hex($imm8) ;
  0x82 @addr(7)   @imm8 => "cmp" sfx[1] " " $addr "," hex($imm8) ;
  0x81 11 111 rrr @immz => "cmp " greg[$opsiz*8+$r] "," hex($immz) ;
  0x81 @addr(7)   @immz => "cmp" sfx[4] " " $addr "," hex($immz) ;
  0x83 11 111 rrr @imm8 => "cmp " greg[$opsiz*8+$r] "," hex(sx8($imm8)) ;
  0x83 @addr(7)   @imm8 => "cmp" sfx[4] " " $addr "," hex(sx8($imm8)) ;
  # test
  0x84 11 ggg rrr => "test " rgb[$r] "," rgb[$g] ;
  0x84 @addr      => "test" sfx[1] " " $addr "," rgb[$g] ;
  0x85 11 ggg rrr => "test " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x85 @addr      => "test " $addr "," greg[$opsiz*8+$g] ;
  0xa8 @imm8 => "test al," hex($imm8) ;
  0xa9 @immz => "test " greg[$opsiz*8+0] "," hex($immz) ;
  # shift/rotate groups: c0/c1=imm8, d0/d1=,1, d2/d3=,cl  (reg digit picks op)
  0xc0 11 000 rrr @imm8 => "rol " rgb[$r] "," hex($imm8) ;
  0xc0 @addr(0)   @imm8 => "rol" sfx[1] " " $addr "," hex($imm8) ;
  0xc1 11 000 rrr @imm8 => "rol " greg[$opsiz*8+$r] "," hex($imm8) ;
  0xc1 @addr(0)   @imm8 => "rol" sfx[4] " " $addr "," hex($imm8) ;
  0xd0 11 000 rrr => "rol " rgb[$r] ",1" ;
  0xd0 @addr(0)   => "rol" sfx[1] " " $addr ",1" ;
  0xd1 11 000 rrr => "rol " greg[$opsiz*8+$r] ",1" ;
  0xd1 @addr(0)   => "rol" sfx[4] " " $addr ",1" ;
  0xd2 11 000 rrr => "rol " rgb[$r] ",cl" ;
  0xd2 @addr(0)   => "rol" sfx[1] " " $addr ",cl" ;
  0xd3 11 000 rrr => "rol " greg[$opsiz*8+$r] ",cl" ;
  0xd3 @addr(0)   => "rol" sfx[4] " " $addr ",cl" ;
  0xc0 11 001 rrr @imm8 => "ror " rgb[$r] "," hex($imm8) ;
  0xc0 @addr(1)   @imm8 => "ror" sfx[1] " " $addr "," hex($imm8) ;
  0xc1 11 001 rrr @imm8 => "ror " greg[$opsiz*8+$r] "," hex($imm8) ;
  0xc1 @addr(1)   @imm8 => "ror" sfx[4] " " $addr "," hex($imm8) ;
  0xd0 11 001 rrr => "ror " rgb[$r] ",1" ;
  0xd0 @addr(1)   => "ror" sfx[1] " " $addr ",1" ;
  0xd1 11 001 rrr => "ror " greg[$opsiz*8+$r] ",1" ;
  0xd1 @addr(1)   => "ror" sfx[4] " " $addr ",1" ;
  0xd2 11 001 rrr => "ror " rgb[$r] ",cl" ;
  0xd2 @addr(1)   => "ror" sfx[1] " " $addr ",cl" ;
  0xd3 11 001 rrr => "ror " greg[$opsiz*8+$r] ",cl" ;
  0xd3 @addr(1)   => "ror" sfx[4] " " $addr ",cl" ;
  0xc0 11 010 rrr @imm8 => "rcl " rgb[$r] "," hex($imm8) ;
  0xc0 @addr(2)   @imm8 => "rcl" sfx[1] " " $addr "," hex($imm8) ;
  0xc1 11 010 rrr @imm8 => "rcl " greg[$opsiz*8+$r] "," hex($imm8) ;
  0xc1 @addr(2)   @imm8 => "rcl" sfx[4] " " $addr "," hex($imm8) ;
  0xd0 11 010 rrr => "rcl " rgb[$r] ",1" ;
  0xd0 @addr(2)   => "rcl" sfx[1] " " $addr ",1" ;
  0xd1 11 010 rrr => "rcl " greg[$opsiz*8+$r] ",1" ;
  0xd1 @addr(2)   => "rcl" sfx[4] " " $addr ",1" ;
  0xd2 11 010 rrr => "rcl " rgb[$r] ",cl" ;
  0xd2 @addr(2)   => "rcl" sfx[1] " " $addr ",cl" ;
  0xd3 11 010 rrr => "rcl " greg[$opsiz*8+$r] ",cl" ;
  0xd3 @addr(2)   => "rcl" sfx[4] " " $addr ",cl" ;
  0xc0 11 011 rrr @imm8 => "rcr " rgb[$r] "," hex($imm8) ;
  0xc0 @addr(3)   @imm8 => "rcr" sfx[1] " " $addr "," hex($imm8) ;
  0xc1 11 011 rrr @imm8 => "rcr " greg[$opsiz*8+$r] "," hex($imm8) ;
  0xc1 @addr(3)   @imm8 => "rcr" sfx[4] " " $addr "," hex($imm8) ;
  0xd0 11 011 rrr => "rcr " rgb[$r] ",1" ;
  0xd0 @addr(3)   => "rcr" sfx[1] " " $addr ",1" ;
  0xd1 11 011 rrr => "rcr " greg[$opsiz*8+$r] ",1" ;
  0xd1 @addr(3)   => "rcr" sfx[4] " " $addr ",1" ;
  0xd2 11 011 rrr => "rcr " rgb[$r] ",cl" ;
  0xd2 @addr(3)   => "rcr" sfx[1] " " $addr ",cl" ;
  0xd3 11 011 rrr => "rcr " greg[$opsiz*8+$r] ",cl" ;
  0xd3 @addr(3)   => "rcr" sfx[4] " " $addr ",cl" ;
  0xc0 11 100 rrr @imm8 => "shl " rgb[$r] "," hex($imm8) ;
  0xc0 @addr(4)   @imm8 => "shl" sfx[1] " " $addr "," hex($imm8) ;
  0xc1 11 100 rrr @imm8 => "shl " greg[$opsiz*8+$r] "," hex($imm8) ;
  0xc1 @addr(4)   @imm8 => "shl" sfx[4] " " $addr "," hex($imm8) ;
  0xd0 11 100 rrr => "shl " rgb[$r] ",1" ;
  0xd0 @addr(4)   => "shl" sfx[1] " " $addr ",1" ;
  0xd1 11 100 rrr => "shl " greg[$opsiz*8+$r] ",1" ;
  0xd1 @addr(4)   => "shl" sfx[4] " " $addr ",1" ;
  0xd2 11 100 rrr => "shl " rgb[$r] ",cl" ;
  0xd2 @addr(4)   => "shl" sfx[1] " " $addr ",cl" ;
  0xd3 11 100 rrr => "shl " greg[$opsiz*8+$r] ",cl" ;
  0xd3 @addr(4)   => "shl" sfx[4] " " $addr ",cl" ;
  0xc0 11 101 rrr @imm8 => "shr " rgb[$r] "," hex($imm8) ;
  0xc0 @addr(5)   @imm8 => "shr" sfx[1] " " $addr "," hex($imm8) ;
  0xc1 11 101 rrr @imm8 => "shr " greg[$opsiz*8+$r] "," hex($imm8) ;
  0xc1 @addr(5)   @imm8 => "shr" sfx[4] " " $addr "," hex($imm8) ;
  0xd0 11 101 rrr => "shr " rgb[$r] ",1" ;
  0xd0 @addr(5)   => "shr" sfx[1] " " $addr ",1" ;
  0xd1 11 101 rrr => "shr " greg[$opsiz*8+$r] ",1" ;
  0xd1 @addr(5)   => "shr" sfx[4] " " $addr ",1" ;
  0xd2 11 101 rrr => "shr " rgb[$r] ",cl" ;
  0xd2 @addr(5)   => "shr" sfx[1] " " $addr ",cl" ;
  0xd3 11 101 rrr => "shr " greg[$opsiz*8+$r] ",cl" ;
  0xd3 @addr(5)   => "shr" sfx[4] " " $addr ",cl" ;
  0xc0 11 110 rrr @imm8 => "sal " rgb[$r] "," hex($imm8) ;
  0xc0 @addr(6)   @imm8 => "sal" sfx[1] " " $addr "," hex($imm8) ;
  0xc1 11 110 rrr @imm8 => "sal " greg[$opsiz*8+$r] "," hex($imm8) ;
  0xc1 @addr(6)   @imm8 => "sal" sfx[4] " " $addr "," hex($imm8) ;
  0xd0 11 110 rrr => "sal " rgb[$r] ",1" ;
  0xd0 @addr(6)   => "sal" sfx[1] " " $addr ",1" ;
  0xd1 11 110 rrr => "sal " greg[$opsiz*8+$r] ",1" ;
  0xd1 @addr(6)   => "sal" sfx[4] " " $addr ",1" ;
  0xd2 11 110 rrr => "sal " rgb[$r] ",cl" ;
  0xd2 @addr(6)   => "sal" sfx[1] " " $addr ",cl" ;
  0xd3 11 110 rrr => "sal " greg[$opsiz*8+$r] ",cl" ;
  0xd3 @addr(6)   => "sal" sfx[4] " " $addr ",cl" ;
  0xc0 11 111 rrr @imm8 => "sar " rgb[$r] "," hex($imm8) ;
  0xc0 @addr(7)   @imm8 => "sar" sfx[1] " " $addr "," hex($imm8) ;
  0xc1 11 111 rrr @imm8 => "sar " greg[$opsiz*8+$r] "," hex($imm8) ;
  0xc1 @addr(7)   @imm8 => "sar" sfx[4] " " $addr "," hex($imm8) ;
  0xd0 11 111 rrr => "sar " rgb[$r] ",1" ;
  0xd0 @addr(7)   => "sar" sfx[1] " " $addr ",1" ;
  0xd1 11 111 rrr => "sar " greg[$opsiz*8+$r] ",1" ;
  0xd1 @addr(7)   => "sar" sfx[4] " " $addr ",1" ;
  0xd2 11 111 rrr => "sar " rgb[$r] ",cl" ;
  0xd2 @addr(7)   => "sar" sfx[1] " " $addr ",cl" ;
  0xd3 11 111 rrr => "sar " greg[$opsiz*8+$r] ",cl" ;
  0xd3 @addr(7)   => "sar" sfx[4] " " $addr ",cl" ;
  # f6/f7 group: /0,/1 test imm ; /2 not /3 neg /4 mul /5 imul /6 div /7 idiv
  0xf6 11 000 rrr @imm8 => "test " rgb[$r] "," hex($imm8) ;
  0xf7 11 000 rrr @immz => "test " greg[$opsiz*8+$r] "," hex($immz) ;
  0xf6 @addr(0)   @imm8 => "test" sfx[1] " " $addr "," hex($imm8) ;
  0xf7 @addr(0)   @immz => "test" sfx[4] " " $addr "," hex($immz) ;
  0xf6 11 001 rrr @imm8 => "test " rgb[$r] "," hex($imm8) ;
  0xf7 11 001 rrr @immz => "test " greg[$opsiz*8+$r] "," hex($immz) ;
  0xf6 @addr(1)   @imm8 => "test" sfx[1] " " $addr "," hex($imm8) ;
  0xf7 @addr(1)   @immz => "test" sfx[4] " " $addr "," hex($immz) ;
  0xf6 11 010 rrr => "not " rgb[$r] ;
  0xf6 @addr(2)   => "not" sfx[1] " " $addr ;
  0xf7 11 010 rrr => "not " greg[$opsiz*8+$r] ;
  0xf7 @addr(2)   => "not" sfx[4] " " $addr ;
  0xf6 11 011 rrr => "neg " rgb[$r] ;
  0xf6 @addr(3)   => "neg" sfx[1] " " $addr ;
  0xf7 11 011 rrr => "neg " greg[$opsiz*8+$r] ;
  0xf7 @addr(3)   => "neg" sfx[4] " " $addr ;
  0xf6 11 100 rrr => "mul " rgb[$r] ;
  0xf6 @addr(4)   => "mul" sfx[1] " " $addr ;
  0xf7 11 100 rrr => "mul " greg[$opsiz*8+$r] ;
  0xf7 @addr(4)   => "mul" sfx[4] " " $addr ;
  0xf6 11 101 rrr => "imul " rgb[$r] ;
  0xf6 @addr(5)   => "imul" sfx[1] " " $addr ;
  0xf7 11 101 rrr => "imul " greg[$opsiz*8+$r] ;
  0xf7 @addr(5)   => "imul" sfx[4] " " $addr ;
  0xf6 11 110 rrr => "div " rgb[$r] ;
  0xf6 @addr(6)   => "div" sfx[1] " " $addr ;
  0xf7 11 110 rrr => "div " greg[$opsiz*8+$r] ;
  0xf7 @addr(6)   => "div" sfx[4] " " $addr ;
  0xf6 11 111 rrr => "idiv " rgb[$r] ;
  0xf6 @addr(7)   => "idiv" sfx[1] " " $addr ;
  0xf7 11 111 rrr => "idiv " greg[$opsiz*8+$r] ;
  0xf7 @addr(7)   => "idiv" sfx[4] " " $addr ;
  # fe (inc/dec r/m8), ff (/0 inc /1 dec /2 call /3 callf /4 jmp /5 jmpf /6 push)
  01000 bbb => "inc " greg[$opsiz*8+$b] ;
  01001 bbb => "dec " greg[$opsiz*8+$b] ;
  0xfe 11 000 rrr => "inc " rgb[$r] ;
  0xfe @addr(0)   => "inc" sfx[1] " " $addr ;
  0xfe 11 001 rrr => "dec " rgb[$r] ;
  0xfe @addr(1)   => "dec" sfx[1] " " $addr ;
  0xff 11 000 rrr => "inc " greg[$opsiz*8+$r] ;
  0xff @addr(0)   => "inc" sfx[4] " " $addr ;
  0xff 11 001 rrr => "dec " greg[$opsiz*8+$r] ;
  0xff @addr(1)   => "dec" sfx[4] " " $addr ;
  0xff 11 010 rrr => "call " greg[$opsiz*8+$r] ;
  0xff @addr(2)   => "call" sfx[4] " " $addr ;
  0xff 11 011 rrr => "callf " greg[$opsiz*8+$r] ;
  0xff @addr(3)   => "callf" sfx[4] " " $addr ;
  0xff 11 100 rrr => "jmp " greg[$opsiz*8+$r] ;
  0xff @addr(4)   => "jmp" sfx[4] " " $addr ;
  0xff 11 101 rrr => "jmpf " greg[$opsiz*8+$r] ;
  0xff @addr(5)   => "jmpf" sfx[4] " " $addr ;
  0xff 11 110 rrr => "push " greg[$opsiz*8+$r] ;
  0xff @addr(6)   => "push" sfx[4] " " $addr ;
  # imul: 0F AF (r,r/m), 69 (r,r/m,immz), 6b (r,r/m,imm8)
  0x0f 0xaf 11 ggg rrr => "imul " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x0f 0xaf @addr      => "imul " greg[$opsiz*8+$g] "," $addr ;
  0x69 11 ggg rrr @immz => "imul " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] "," hex($immz) ;
  0x69 @addr      @immz => "imul " greg[$opsiz*8+$g] "," $addr "," hex($immz) ;
  0x6b 11 ggg rrr @imm8 => "imul " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] "," hex(sx8($imm8)) ;
  0x6b @addr      @imm8 => "imul " greg[$opsiz*8+$g] "," $addr "," hex(sx8($imm8)) ;
  # mov family
  0x8d @addr => "lea " greg[$opsiz*8+$g] "," $addr ;
  0x88 11 ggg rrr => "mov " rgb[$r] "," rgb[$g] ;
  0x88 @addr      => "mov" sfx[1] " " $addr "," rgb[$g] ;
  0x89 11 ggg rrr => "mov " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x89 @addr      => "mov " $addr "," greg[$opsiz*8+$g] ;
  0x8a 11 ggg rrr => "mov " rgb[$g] "," rgb[$r] ;
  0x8a @addr      => "mov " rgb[$g] "," $addr ;
  0x8b 11 ggg rrr => "mov " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x8b @addr      => "mov " greg[$opsiz*8+$g] "," $addr ;
  0x8c 11 ggg rrr => "mov " greg[$opsiz*8+$r] "," sreg[$g] ;
  0x8c @addr      => "mov " $addr "," sreg[$g] ;
  0x8e 11 ggg rrr => "mov " sreg[$g] "," greg[$opsiz*8+$r] ;
  0x8e @addr      => "mov " sreg[$g] "," $addr ;
  0xa0 @imm32 => "mov al," seg[$segidx] "[@" hex($imm32) "]" ;
  0xa1 @imm32 => "mov " greg[$opsiz*8+0] "," seg[$segidx] "[@" hex($imm32) "]" ;
  0xa2 @imm32 => "mov " seg[$segidx] "[@" hex($imm32) "],al" ;
  0xa3 @imm32 => "mov " seg[$segidx] "[@" hex($imm32) "]," greg[$opsiz*8+0] ;
  10110 bbb @imm8 => "mov " rgb[$b] "," hex($imm8) ;
  10111 bbb @immz => "mov " greg[$opsiz*8+$b] "," hex($immz) ;
  0xc6 11 000 rrr @imm8 => "mov " rgb[$r] "," hex($imm8) ;
  0xc6 11 111 rrr @imm8 => "xabort " hex($imm8) ;
  0xc6 @addr(0)   @imm8 => "mov" sfx[1] " " $addr "," hex($imm8) ;
  0xc7 11 000 rrr @immz => "mov " greg[$opsiz*8+$r] "," hex($immz) ;
  0xc7 11 111 rrr @immz => "xbegin " hex($immz) ;
  0xc7 @addr(0)   @immz => "mov" sfx[4] " " $addr "," hex($immz) ;
  0x0f 0xb6 11 ggg rrr => "movzx " greg[$opsiz*8+$g] "," rgb[$r] ;
  0x0f 0xb6 @addr      => "movzx" sfx[1] " " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xb7 11 ggg rrr => "movzx " greg[$opsiz*8+$g] "," greg[8+$r] ;
  0x0f 0xb7 @addr      => "movzx" sfx[2] " " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xbe 11 ggg rrr => "movsx " greg[$opsiz*8+$g] "," rgb[$r] ;
  0x0f 0xbe @addr      => "movsx" sfx[1] " " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xbf 11 ggg rrr => "movsx " greg[$opsiz*8+$g] "," greg[8+$r] ;
  0x0f 0xbf @addr      => "movsx" sfx[2] " " greg[$opsiz*8+$g] "," $addr ;
  # xchg (90+r, with 90=nop overriding), 86/87 r/m,r ; xlat
  10010 bbb => "xchg " greg[$opsiz*8+$b] ",eax" ;
  0x86 11 ggg rrr => "xchg " rgb[$r] "," rgb[$g] ;
  0x86 @addr      => "xchg" sfx[1] " " $addr "," rgb[$g] ;
  0x87 11 ggg rrr => "xchg " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x87 @addr      => "xchg " $addr "," greg[$opsiz*8+$g] ;
  0xd7 => "xlat" ;
  0x0f 11001 bbb => "bswap " greg[$opsiz*8+$b] ;
  # push/pop: short reg, imm, r/m, segment registers, pusha/popa, pushf/popf
  01010 bbb => "push " greg[$opsiz*8+$b] ;
  01011 bbb => "pop "  greg[$opsiz*8+$b] ;
  0x68 @immz => "push " hex($immz) ;
  0x6a @imm8 => "push " hex(sx8($imm8)) ;
  0x8f 11 000 rrr => "pop " greg[$opsiz*8+$r] ;
  0x8f @addr(0)   => "pop" sfx[4] " " $addr ;
  0x06 => "push es" ;
  0x0e => "push cs" ;
  0x16 => "push ss" ;
  0x1e => "push ds" ;
  0x07 => "pop es" ;
  0x17 => "pop ss" ;
  0x1f => "pop ds" ;
  0x0f 0xa0 => "push fs" ;
  0x0f 0xa8 => "push gs" ;
  0x0f 0xa1 => "pop fs" ;
  0x0f 0xa9 => "pop gs" ;
  0x60 => "pusha" ;
  0x61 => "popa" ;
  0x9c => "pushf" ;
  0x9d => "popf" ;
  # jcc rel8/rel32, jmp/call, loop/jecxz, ret/retf/int/iret, leave/enter
  0111 cccc @rel8 => "j" cond[$c] " " hex($rel8) ;
  0x0f 1000 cccc @relz => "j" cond[$c] " " hex($relz) ;
  0xeb @rel8 => "jmp " hex($rel8) ;
  0xe9 @relz => "jmp " hex($relz) ;
  0xe8 @relz => "call " hex($relz) ;
  0xea @imm32 @imm16 => "jmpf " hex($imm16) ":" hex($imm32) ;
  0x9a @imm32 @imm16 => "callf " hex($imm16) ":" hex($imm32) ;
  0xe0 @rel8 => "loopne " hex($rel8) ;
  0xe1 @rel8 => "loope " hex($rel8) ;
  0xe2 @rel8 => "loop " hex($rel8) ;
  0xe3 @rel8 => "jecxz " hex($rel8) ;
  0xc3 => "ret" ;
  0xc2 @imm16 => "ret " hex($imm16) ;
  0xcb => "retf" ;
  0xca @imm16 => "retf " hex($imm16) ;
  0xcd @imm8 => "int " hex($imm8) ;
  0xcc => "int3" ;
  0xce => "into" ;
  0xcf => "iret" ;
  0xc9 => "leave" ;
  0xc8 @imm16 @imm8 => "enter " hex($imm16) "," hex($imm8) ;
  # setcc, cmovcc, bt/bts/btr/btc/bsf/bsr, shld/shrd
  0x0f 1001 cccc 11 ggg rrr => "set" cond[$c] " " rgb[$r] ;
  0x0f 1001 cccc @addr      => "set" cond[$c] sfx[1] " " $addr ;
  0x0f 0100 cccc 11 ggg rrr => "cmov" cond[$c] " " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x0f 0100 cccc @addr      => "cmov" cond[$c] " " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xa3 11 ggg rrr => "bt " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x0f 0xa3 @addr      => "bt " $addr "," greg[$opsiz*8+$g] ;
  0x0f 0xab 11 ggg rrr => "bts " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x0f 0xab @addr      => "bts " $addr "," greg[$opsiz*8+$g] ;
  0x0f 0xb3 11 ggg rrr => "btr " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x0f 0xb3 @addr      => "btr " $addr "," greg[$opsiz*8+$g] ;
  0x0f 0xbb 11 ggg rrr => "btc " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x0f 0xbb @addr      => "btc " $addr "," greg[$opsiz*8+$g] ;
  0x0f 0xbc 11 ggg rrr => "bsf " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x0f 0xbc @addr      => "bsf " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xbd 11 ggg rrr => "bsr " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x0f 0xbd @addr      => "bsr " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xba 11 100 rrr @imm8 => "bt " greg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0xba @addr(4)   @imm8 => "bt" sfx[4] " " $addr "," hex($imm8) ;
  0x0f 0xba 11 101 rrr @imm8 => "bts " greg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0xba @addr(5)   @imm8 => "bts" sfx[4] " " $addr "," hex($imm8) ;
  0x0f 0xba 11 110 rrr @imm8 => "btr " greg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0xba @addr(6)   @imm8 => "btr" sfx[4] " " $addr "," hex($imm8) ;
  0x0f 0xba 11 111 rrr @imm8 => "btc " greg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0xba @addr(7)   @imm8 => "btc" sfx[4] " " $addr "," hex($imm8) ;
  0x0f 0xa4 11 ggg rrr @imm8 => "shld " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0xa4 @addr      @imm8 => "shld " $addr "," greg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0xa5 11 ggg rrr => "shld " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ",cl" ;
  0x0f 0xa5 @addr      => "shld " $addr "," greg[$opsiz*8+$g] ",cl" ;
  0x0f 0xac 11 ggg rrr @imm8 => "shrd " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0xac @addr      @imm8 => "shrd " $addr "," greg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0xad 11 ggg rrr => "shrd " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ",cl" ;
  0x0f 0xad @addr      => "shrd " $addr "," greg[$opsiz*8+$g] ",cl" ;
  # les/lds (c4/c5), lss/lfs/lgs (0F b2/b4/b5), bound, arpl
  0xc4 @addr => "les " greg[$opsiz*8+$g] "," $addr ;
  0xc5 @addr => "lds " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xb2 @addr => "lss " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xb4 @addr => "lfs " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0xb5 @addr => "lgs " greg[$opsiz*8+$g] "," $addr ;
  0x62 @addr => "bound " greg[$opsiz*8+$g] "," $addr ;
  0x63 11 ggg rrr => "arpl " greg[8+$r] "," greg[8+$g] ;
  0x63 @addr      => "arpl " $addr "," greg[8+$g] ;
  # flag ops, daa/das/aaa/aas/aam/aad, nop/hlt/fwait, cpuid/rdtsc/rd-wrmsr/ud2
  0xf5 => "cmc" ;
  0xf8 => "clc" ;
  0xf9 => "stc" ;
  0xfa => "cli" ;
  0xfb => "sti" ;
  0xfc => "cld" ;
  0xfd => "std" ;
  0x9e => "sahf" ;
  0x9f => "lahf" ;
  0x27 => "daa" ;
  0x2f => "das" ;
  0x37 => "aaa" ;
  0x3f => "aas" ;
  0x90 => "nop" ;
  0xf4 => "hlt" ;
  0x9b => "fwait" ;
  0xd4 @imm8 => "aam " hex($imm8) ;
  0xd5 @imm8 => "aad " hex($imm8) ;
  0x0f 0xa2 => "cpuid" ;
  0x0f 0x31 => "rdtsc" ;
  0x0f 0x32 => "rdmsr" ;
  0x0f 0x30 => "wrmsr" ;
  0x0f 0x0b => "ud2" ;
  0x99 => cdqw[$opsiz] ;
  0x98 => cbwe[$opsiz] ;
  # string ops (implicit operands; rep/repnz ride in the prefix run)
  0xa4 => "movsb" ;
  0xa6 => "cmpsb" ;
  0xaa => "stosb" ;
  0xac => "lodsb" ;
  0xae => "scasb" ;
  0xa5 => movs[$opsiz] ;
  0xa7 => cmps[$opsiz] ;
  0xab => stos[$opsiz] ;
  0xad => lods[$opsiz] ;
  0xaf => scas[$opsiz] ;
  # in/out (imm8 and dx forms), ins/outs
  0xe4 @imm8 => "in al," hex($imm8) ;
  0xe5 @imm8 => "in " greg[$opsiz*8+0] "," hex($imm8) ;
  0xe6 @imm8 => "out " hex($imm8) ",al" ;
  0xe7 @imm8 => "out " hex($imm8) "," greg[$opsiz*8+0] ;
  0xec => "in al,dx" ;
  0xed => "in " greg[$opsiz*8+0] ",dx" ;
  0xee => "out dx,al" ;
  0xef => "out dx," greg[$opsiz*8+0] ;
  0x6c => "insb" ;
  0x6d => "ins" ;
  0x6e => "outsb" ;
  0x6f => "outs" ;
  # x87 FPU D8-DF: /digit groups; mem forms carry size suffix, reg forms take st(i)
  0xd8 11 000 rrr => "fadd st(" dec($r) ")" ;
  0xd8 @addr(0)   => "fadd.d " $addr ;
  0xd8 11 001 rrr => "fmul st(" dec($r) ")" ;
  0xd8 @addr(1)   => "fmul.d " $addr ;
  0xd8 11 010 rrr => "fcom st(" dec($r) ")" ;
  0xd8 @addr(2)   => "fcom.d " $addr ;
  0xd8 11 011 rrr => "fcomp st(" dec($r) ")" ;
  0xd8 @addr(3)   => "fcomp.d " $addr ;
  0xd8 11 100 rrr => "fsub st(" dec($r) ")" ;
  0xd8 @addr(4)   => "fsub.d " $addr ;
  0xd8 11 101 rrr => "fsubr st(" dec($r) ")" ;
  0xd8 @addr(5)   => "fsubr.d " $addr ;
  0xd8 11 110 rrr => "fdiv st(" dec($r) ")" ;
  0xd8 @addr(6)   => "fdiv.d " $addr ;
  0xd8 11 111 rrr => "fdivr st(" dec($r) ")" ;
  0xd8 @addr(7)   => "fdivr.d " $addr ;
  0xd9 11 000 rrr => "fld st(" dec($r) ")" ;
  0xd9 @addr(0)   => "fld.d " $addr ;
  0xd9 11 001 rrr => "fxch st(" dec($r) ")" ;
  0xd9 @addr(1)   => "fxch.d " $addr ;
  0xd9 11 010 rrr => "fst st(" dec($r) ")" ;
  0xd9 @addr(2)   => "fst.d " $addr ;
  0xd9 11 011 rrr => "fstp st(" dec($r) ")" ;
  0xd9 @addr(3)   => "fstp.d " $addr ;
  0xd9 11 100 rrr => "fldenv st(" dec($r) ")" ;
  0xd9 @addr(4)   => "fldenv.d " $addr ;
  0xd9 11 101 rrr => "fldcw st(" dec($r) ")" ;
  0xd9 @addr(5)   => "fldcw.d " $addr ;
  0xd9 11 110 rrr => "fnstenv st(" dec($r) ")" ;
  0xd9 @addr(6)   => "fnstenv.d " $addr ;
  0xd9 11 111 rrr => "fnstcw st(" dec($r) ")" ;
  0xd9 @addr(7)   => "fnstcw.d " $addr ;
  0xda 11 000 rrr => "fcmovb st(" dec($r) ")" ;
  0xda @addr(0)   => "fcmovb.d " $addr ;
  0xda 11 001 rrr => "fcmove st(" dec($r) ")" ;
  0xda @addr(1)   => "fcmove.d " $addr ;
  0xda 11 010 rrr => "fcmovbe st(" dec($r) ")" ;
  0xda @addr(2)   => "fcmovbe.d " $addr ;
  0xda 11 011 rrr => "fcmovu st(" dec($r) ")" ;
  0xda @addr(3)   => "fcmovu.d " $addr ;
  0xda 11 100 rrr => "fx_a st(" dec($r) ")" ;
  0xda @addr(4)   => "fx_a.d " $addr ;
  0xda 11 101 rrr => "fucompp st(" dec($r) ")" ;
  0xda @addr(5)   => "fucompp.d " $addr ;
  0xda 11 110 rrr => "fx_a st(" dec($r) ")" ;
  0xda @addr(6)   => "fx_a.d " $addr ;
  0xda 11 111 rrr => "fx_a st(" dec($r) ")" ;
  0xda @addr(7)   => "fx_a.d " $addr ;
  0xdb 11 000 rrr => "fild st(" dec($r) ")" ;
  0xdb @addr(0)   => "fild.d " $addr ;
  0xdb 11 001 rrr => "fisttp st(" dec($r) ")" ;
  0xdb @addr(1)   => "fisttp.d " $addr ;
  0xdb 11 010 rrr => "fist st(" dec($r) ")" ;
  0xdb @addr(2)   => "fist.d " $addr ;
  0xdb 11 011 rrr => "fistp st(" dec($r) ")" ;
  0xdb @addr(3)   => "fistp.d " $addr ;
  0xdb 11 100 rrr => "fx_b st(" dec($r) ")" ;
  0xdb @addr(4)   => "fx_b.d " $addr ;
  0xdb 11 101 rrr => "fld st(" dec($r) ")" ;
  0xdb @addr(5)   => "fld.d " $addr ;
  0xdb 11 110 rrr => "fx_b st(" dec($r) ")" ;
  0xdb @addr(6)   => "fx_b.d " $addr ;
  0xdb 11 111 rrr => "fstp st(" dec($r) ")" ;
  0xdb @addr(7)   => "fstp.d " $addr ;
  0xdc 11 000 rrr => "fadd st(" dec($r) ")" ;
  0xdc @addr(0)   => "fadd.q " $addr ;
  0xdc 11 001 rrr => "fmul st(" dec($r) ")" ;
  0xdc @addr(1)   => "fmul.q " $addr ;
  0xdc 11 010 rrr => "fcom st(" dec($r) ")" ;
  0xdc @addr(2)   => "fcom.q " $addr ;
  0xdc 11 011 rrr => "fcomp st(" dec($r) ")" ;
  0xdc @addr(3)   => "fcomp.q " $addr ;
  0xdc 11 100 rrr => "fsub st(" dec($r) ")" ;
  0xdc @addr(4)   => "fsub.q " $addr ;
  0xdc 11 101 rrr => "fsubr st(" dec($r) ")" ;
  0xdc @addr(5)   => "fsubr.q " $addr ;
  0xdc 11 110 rrr => "fdiv st(" dec($r) ")" ;
  0xdc @addr(6)   => "fdiv.q " $addr ;
  0xdc 11 111 rrr => "fdivr st(" dec($r) ")" ;
  0xdc @addr(7)   => "fdivr.q " $addr ;
  0xdd 11 000 rrr => "fld st(" dec($r) ")" ;
  0xdd @addr(0)   => "fld.q " $addr ;
  0xdd 11 001 rrr => "fisttp st(" dec($r) ")" ;
  0xdd @addr(1)   => "fisttp.q " $addr ;
  0xdd 11 010 rrr => "fst st(" dec($r) ")" ;
  0xdd @addr(2)   => "fst.q " $addr ;
  0xdd 11 011 rrr => "fstp st(" dec($r) ")" ;
  0xdd @addr(3)   => "fstp.q " $addr ;
  0xdd 11 100 rrr => "frstor st(" dec($r) ")" ;
  0xdd @addr(4)   => "frstor.q " $addr ;
  0xdd 11 101 rrr => "fx_d st(" dec($r) ")" ;
  0xdd @addr(5)   => "fx_d.q " $addr ;
  0xdd 11 110 rrr => "fnsave st(" dec($r) ")" ;
  0xdd @addr(6)   => "fnsave.q " $addr ;
  0xdd 11 111 rrr => "fnstsw st(" dec($r) ")" ;
  0xdd @addr(7)   => "fnstsw.q " $addr ;
  0xde 11 000 rrr => "faddp st(" dec($r) ")" ;
  0xde @addr(0)   => "faddp.w " $addr ;
  0xde 11 001 rrr => "fmulp st(" dec($r) ")" ;
  0xde @addr(1)   => "fmulp.w " $addr ;
  0xde 11 010 rrr => "fcomp st(" dec($r) ")" ;
  0xde @addr(2)   => "fcomp.w " $addr ;
  0xde 11 011 rrr => "fcompp st(" dec($r) ")" ;
  0xde @addr(3)   => "fcompp.w " $addr ;
  0xde 11 100 rrr => "fsubrp st(" dec($r) ")" ;
  0xde @addr(4)   => "fsubrp.w " $addr ;
  0xde 11 101 rrr => "fsubp st(" dec($r) ")" ;
  0xde @addr(5)   => "fsubp.w " $addr ;
  0xde 11 110 rrr => "fdivrp st(" dec($r) ")" ;
  0xde @addr(6)   => "fdivrp.w " $addr ;
  0xde 11 111 rrr => "fdivp st(" dec($r) ")" ;
  0xde @addr(7)   => "fdivp.w " $addr ;
  0xdf 11 000 rrr => "fild st(" dec($r) ")" ;
  0xdf @addr(0)   => "fild.w " $addr ;
  0xdf 11 001 rrr => "fisttp st(" dec($r) ")" ;
  0xdf @addr(1)   => "fisttp.w " $addr ;
  0xdf 11 010 rrr => "fist st(" dec($r) ")" ;
  0xdf @addr(2)   => "fist.w " $addr ;
  0xdf 11 011 rrr => "fistp st(" dec($r) ")" ;
  0xdf @addr(3)   => "fistp.w " $addr ;
  0xdf 11 100 rrr => "fbld st(" dec($r) ")" ;
  0xdf @addr(4)   => "fbld.w " $addr ;
  0xdf 11 101 rrr => "fild st(" dec($r) ")" ;
  0xdf @addr(5)   => "fild.w " $addr ;
  0xdf 11 110 rrr => "fbstp st(" dec($r) ")" ;
  0xdf @addr(6)   => "fbstp.w " $addr ;
  0xdf 11 111 rrr => "fistp st(" dec($r) ")" ;
  0xdf @addr(7)   => "fistp.w " $addr ;
  # MMX/SSE: opsiz selects mm/xmm where paired; mandatory-prefix variants replay
  0x0f 0x6f 11 ggg rrr => "movq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x6f @addr      => "movq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xfc 11 ggg rrr => "paddb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xfc @addr      => "paddb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xfd 11 ggg rrr => "paddw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xfd @addr      => "paddw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xfe 11 ggg rrr => "paddd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xfe @addr      => "paddd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf8 11 ggg rrr => "psubb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf8 @addr      => "psubb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xdb 11 ggg rrr => "pand " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xdb @addr      => "pand " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xeb 11 ggg rrr => "por " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xeb @addr      => "por " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xef 11 ggg rrr => "pxor " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xef @addr      => "pxor " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x74 11 ggg rrr => "pcmpeqb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x74 @addr      => "pcmpeqb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x63 11 ggg rrr => "packsswb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x63 @addr      => "packsswb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x60 11 ggg rrr => "punpcklbw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x60 @addr      => "punpcklbw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd1 11 ggg rrr => "psrlw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd1 @addr      => "psrlw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd2 11 ggg rrr => "psrld " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd2 @addr      => "psrld " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd3 11 ggg rrr => "psrlq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd3 @addr      => "psrlq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe1 11 ggg rrr => "psraw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe1 @addr      => "psraw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe2 11 ggg rrr => "psrad " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe2 @addr      => "psrad " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf1 11 ggg rrr => "psllw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf1 @addr      => "psllw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf2 11 ggg rrr => "pslld " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf2 @addr      => "pslld " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf3 11 ggg rrr => "psllq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf3 @addr      => "psllq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x28 11 ggg rrr => "movaps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x28 @addr      => "movaps " ssereg[8+$g] "," $addr ;
  0x0f 0x10 11 ggg rrr => "movups " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x10 @addr      => "movups " ssereg[8+$g] "," $addr ;
  0x0f 0x58 11 ggg rrr => "addps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x58 @addr      => "addps " ssereg[8+$g] "," $addr ;
  0x0f 0x59 11 ggg rrr => "mulps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x59 @addr      => "mulps " ssereg[8+$g] "," $addr ;
  0x0f 0x5c 11 ggg rrr => "subps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5c @addr      => "subps " ssereg[8+$g] "," $addr ;
  0x0f 0x2f 11 ggg rrr => "comiss " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x2f @addr      => "comiss " ssereg[8+$g] "," $addr ;
  0x0f 0x57 11 ggg rrr => "xorps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x57 @addr      => "xorps " ssereg[8+$g] "," $addr ;
  0x0f 0x54 11 ggg rrr => "andps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x54 @addr      => "andps " ssereg[8+$g] "," $addr ;
  0x0f 0x2e 11 ggg rrr => uc[$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x2e @addr      => uc[$opsiz] " " ssereg[8+$g] "," $addr ;
  0x0f 0x6e 11 ggg rrr => "movd " ssereg[$opsiz*8+$g] "," greg[$r] ;
  0x0f 0x6e @addr      => "movd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x7e 11 ggg rrr => "movd " greg[$r] "," ssereg[$opsiz*8+$g] ;
  0x0f 0x7e @addr      => "movd " $addr "," ssereg[$opsiz*8+$g] ;
  0x0f 0x2a 11 ggg rrr => "cvtpi2ps " ssereg[8+$g] "," greg[$r] ;
  0x0f 0x2a @addr      => "cvtpi2ps " ssereg[8+$g] "," $addr ;
  0x0f 0x2c 11 ggg rrr => "cvttps2pi " greg[$g] "," ssereg[8+$r] ;
  0x0f 0x2c @addr      => "cvttps2pi " greg[$g] "," $addr ;
  0x0f 0x2d 11 ggg rrr => "cvtps2pi " greg[$g] "," ssereg[8+$r] ;
  0x0f 0x2d @addr      => "cvtps2pi " greg[$g] "," $addr ;
  0x0f 0x70 11 ggg rrr @imm8 => "pshufw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x70 @addr      @imm8 => "pshufw " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x77 => "emms" ;
  0x0f 0xae 11 100 rrr => "ptwrite " greg[$opsiz*8+$r] ;
  0x0f 0xae 11 101 rrr => "lfence" ;
  0x0f 0xae 11 110 rrr => "mfence" ;
  0x0f 0xae 11 111 rrr => "sfence" ;
  0x0f 0xae @addr(7)   => "clflush" sfx[1] " " $addr ;
  0x0f 0x18 @addr(0) => "prefetchnta" sfx[1] " " $addr ;
  0x0f 0x18 @addr(1) => "prefetcht0" sfx[1] " " $addr ;
  0x0f 0x18 @addr(2) => "prefetcht1" sfx[1] " " $addr ;
  0x0f 0x18 @addr(3) => "prefetcht2" sfx[1] " " $addr ;
  0x0f 0x18 @addr(4) => "nop " $addr ;
  0x0f 0x18 @addr(5) => "nop18 " $addr ;
  0x0f 0x18 @addr(6) => "prefetchit1" sfx[1] " " $addr ;
  0x0f 0x18 @addr(7) => "prefetchit0" sfx[1] " " $addr ;
  0x0f 0x18 11 000 rrr => "nop18 " greg[$r] ;
  0x0f 0x18 11 001 rrr => "nop18 " greg[$r] ;
  0x0f 0x18 11 010 rrr => "nop18 " greg[$r] ;
  0x0f 0x18 11 011 rrr => "nop18 " greg[$r] ;
  0x0f 0x18 11 100 rrr => "nop18 " greg[$r] ;
  0x0f 0x18 11 101 rrr => "nop18 " greg[$r] ;
  0x0f 0x18 11 110 rrr => "nop18 " greg[$r] ;
  0x0f 0x18 11 111 rrr => "nop18 " greg[$r] ;
  # xadd/cmpxchg/cmpxchg8b, lar/lsl, clts/invd/wbinvd, descriptor-table groups
  0x0f 0xc0 11 ggg rrr => "xadd " rgb[$r] "," rgb[$g] ;
  0x0f 0xc0 @addr      => "xadd" sfx[1] " " $addr "," rgb[$g] ;
  0x0f 0xc1 11 ggg rrr => "xadd " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x0f 0xc1 @addr      => "xadd " $addr "," greg[$opsiz*8+$g] ;
  0x0f 0xb0 11 ggg rrr => "cmpxchg " rgb[$r] "," rgb[$g] ;
  0x0f 0xb0 @addr      => "cmpxchg" sfx[1] " " $addr "," rgb[$g] ;
  0x0f 0xb1 11 ggg rrr => "cmpxchg " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;
  0x0f 0xb1 @addr      => "cmpxchg " $addr "," greg[$opsiz*8+$g] ;
  0x0f 0xc7 @addr(1)   => "cmpxchg8b " $addr ;
  0x0f 0xc7 @addr(3)   => "xrstors " $addr ;
  0x0f 0xc7 @addr(4)   => "xsavec " $addr ;
  0x0f 0xc7 @addr(5)   => "xsaves " $addr ;
  0x0f 0xc7 @addr(6)   => "vmptrld " $addr ;
  0x0f 0xc7 @addr(7)   => "vmptrst " $addr ;
  0x0f 0xc7 11 110 rrr => "rdrand " greg[$opsiz*8+$r] ;
  0x0f 0xc7 11 111 rrr => "rdseed " greg[$opsiz*8+$r] ;
  0x0f 0x37 => "getsec" ;
  0x0f 0x02 11 ggg rrr => "lar " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x0f 0x02 @addr      => "lar " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0x03 11 ggg rrr => "lsl " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x0f 0x03 @addr      => "lsl " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0x06 => "clts" ;
  0x0f 0x08 => "invd" ;
  0x0f 0x09 => "wbinvd" ;
  0x0f 0x00 11 000 rrr => "sldt " greg[$opsiz*8+$r] ;
  0x0f 0x00 @addr(0)   => "sldt" sfx[2] " " $addr ;
  0x0f 0x00 11 001 rrr => "str " greg[$opsiz*8+$r] ;
  0x0f 0x00 @addr(1)   => "str" sfx[2] " " $addr ;
  0x0f 0x00 11 010 rrr => "lldt " greg[$opsiz*8+$r] ;
  0x0f 0x00 @addr(2)   => "lldt" sfx[2] " " $addr ;
  0x0f 0x00 11 011 rrr => "ltr " greg[$opsiz*8+$r] ;
  0x0f 0x00 @addr(3)   => "ltr" sfx[2] " " $addr ;
  0x0f 0x00 11 100 rrr => "verr " greg[$opsiz*8+$r] ;
  0x0f 0x00 @addr(4)   => "verr" sfx[2] " " $addr ;
  0x0f 0x00 11 101 rrr => "verw " greg[$opsiz*8+$r] ;
  0x0f 0x00 @addr(5)   => "verw" sfx[2] " " $addr ;
  0x0f 0x01 @addr(0)   => "sgdt " $addr ;
  0x0f 0x01 @addr(1)   => "sidt " $addr ;
  0x0f 0x01 @addr(2)   => "lgdt " $addr ;
  0x0f 0x01 @addr(3)   => "lidt " $addr ;
  0x0f 0x01 @addr(4)   => "smsw " $addr ;
  0x0f 0x01 @addr(6)   => "lmsw " $addr ;
  0x0f 0x01 @addr(7)   => "invlpg " $addr ;
  0x0f 0x01 @addr(5)   => "rstorssp " $addr ;
  0x0f 0x01 11 100 rrr => "smsw " greg[$opsiz*8+$r] ;
  0x0f 0x01 11 110 rrr => "lmsw " greg[8+$r] ;
  # more MMX/SSE packed integer ops (opsiz selects mm/xmm)
  0x0f 0xec 11 ggg rrr => "paddsb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xec @addr      => "paddsb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xed 11 ggg rrr => "paddsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xed @addr      => "paddsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xdc 11 ggg rrr => "paddusb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xdc @addr      => "paddusb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xdd 11 ggg rrr => "paddusw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xdd @addr      => "paddusw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf9 11 ggg rrr => "psubw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf9 @addr      => "psubw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xfa 11 ggg rrr => "psubd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xfa @addr      => "psubd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe8 11 ggg rrr => "psubsb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe8 @addr      => "psubsb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe9 11 ggg rrr => "psubsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe9 @addr      => "psubsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd8 11 ggg rrr => "psubusb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd8 @addr      => "psubusb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd9 11 ggg rrr => "psubusw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd9 @addr      => "psubusw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd5 11 ggg rrr => "pmullw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd5 @addr      => "pmullw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe5 11 ggg rrr => "pmulhw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe5 @addr      => "pmulhw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf5 11 ggg rrr => "pmaddwd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf5 @addr      => "pmaddwd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xdf 11 ggg rrr => "pandn " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xdf @addr      => "pandn " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x64 11 ggg rrr => "pcmpgtb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x64 @addr      => "pcmpgtb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x65 11 ggg rrr => "pcmpgtw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x65 @addr      => "pcmpgtw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x66 11 ggg rrr => "pcmpgtd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x66 @addr      => "pcmpgtd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x67 11 ggg rrr => "packuswb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x67 @addr      => "packuswb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x6b 11 ggg rrr => "packssdw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x6b @addr      => "packssdw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x68 11 ggg rrr => "punpckhbw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x68 @addr      => "punpckhbw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x69 11 ggg rrr => "punpckhwd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x69 @addr      => "punpckhwd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x6a 11 ggg rrr => "punpckhdq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x6a @addr      => "punpckhdq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x61 11 ggg rrr => "punpcklwd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x61 @addr      => "punpcklwd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x62 11 ggg rrr => "punpckldq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x62 @addr      => "punpckldq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf4 11 ggg rrr => "pmuludq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf4 @addr      => "pmuludq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x75 11 ggg rrr => "pcmpeqw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x75 @addr      => "pcmpeqw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x76 11 ggg rrr => "pcmpeqd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x76 @addr      => "pcmpeqd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd4 11 ggg rrr => "paddq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xd4 @addr      => "paddq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xfb 11 ggg rrr => "psubq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xfb @addr      => "psubq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe0 11 ggg rrr => "pavgb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe0 @addr      => "pavgb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe3 11 ggg rrr => "pavgw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe3 @addr      => "pavgw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xda 11 ggg rrr => "pminub " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xda @addr      => "pminub " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xde 11 ggg rrr => "pmaxub " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xde @addr      => "pmaxub " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xea 11 ggg rrr => "pminsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xea @addr      => "pminsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xee 11 ggg rrr => "pmaxsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xee @addr      => "pmaxsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xe4 11 ggg rrr => "pmulhuw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xe4 @addr      => "pmulhuw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xf6 11 ggg rrr => "psadbw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xf6 @addr      => "psadbw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0xd7 11 ggg rrr => "pmovmskb " greg[$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0xc4 11 ggg rrr @imm8 => "pinsrw " ssereg[$opsiz*8+$g] "," greg[$r] "," hex($imm8) ;
  0x0f 0xc4 @addr      @imm8 => "pinsrw " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0xc5 11 ggg rrr @imm8 => "pextrw " greg[$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  # packed shift-by-imm groups 0F 71/72/73
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
  # more SSE float ops (xmm; mandatory-prefix variants replay)
  0x0f 0x51 11 ggg rrr => "sqrtps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x51 @addr      => "sqrtps " ssereg[8+$g] "," $addr ;
  0x0f 0x52 11 ggg rrr => "rsqrtps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x52 @addr      => "rsqrtps " ssereg[8+$g] "," $addr ;
  0x0f 0x53 11 ggg rrr => "rcpps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x53 @addr      => "rcpps " ssereg[8+$g] "," $addr ;
  0x0f 0x55 11 ggg rrr => "andnps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x55 @addr      => "andnps " ssereg[8+$g] "," $addr ;
  0x0f 0x56 11 ggg rrr => "orps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x56 @addr      => "orps " ssereg[8+$g] "," $addr ;
  0x0f 0x5d 11 ggg rrr => "minps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5d @addr      => "minps " ssereg[8+$g] "," $addr ;
  0x0f 0x5e 11 ggg rrr => "divps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5e @addr      => "divps " ssereg[8+$g] "," $addr ;
  0x0f 0x5f 11 ggg rrr => "maxps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5f @addr      => "maxps " ssereg[8+$g] "," $addr ;
  0x0f 0x14 11 ggg rrr => "unpcklps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x14 @addr      => "unpcklps " ssereg[8+$g] "," $addr ;
  0x0f 0x15 11 ggg rrr => "unpckhps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x15 @addr      => "unpckhps " ssereg[8+$g] "," $addr ;
  0x0f 0x12 11 ggg rrr => "movlps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x12 @addr      => "movlps " ssereg[8+$g] "," $addr ;
  0x0f 0x16 11 ggg rrr => "movhps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x16 @addr      => "movhps " ssereg[8+$g] "," $addr ;
  0x0f 0x50 11 ggg rrr => "movmskps " greg[$g] "," ssereg[8+$r] ;
  0x0f 0xc2 11 ggg rrr @imm8 => "cmpps " ssereg[8+$g] "," ssereg[8+$r] "," hex($imm8) ;
  0x0f 0xc2 @addr      @imm8 => "cmpps " ssereg[8+$g] "," $addr "," hex($imm8) ;
  0x0f 0xc6 11 ggg rrr @imm8 => "shufps " ssereg[8+$g] "," ssereg[8+$r] "," hex($imm8) ;
  0x0f 0xc6 @addr      @imm8 => "shufps " ssereg[8+$g] "," $addr "," hex($imm8) ;
  # SSE3 + packed cvt + punpckqdq (66/F2/F3 prefix variants replay)
  0x0f 0xd0 11 ggg rrr => "addsubps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0xd0 @addr      => "addsubps " ssereg[8+$g] "," $addr ;
  0x0f 0x7c 11 ggg rrr => "haddps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x7c @addr      => "haddps " ssereg[8+$g] "," $addr ;
  0x0f 0x7d 11 ggg rrr => "hsubps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x7d @addr      => "hsubps " ssereg[8+$g] "," $addr ;
  0x0f 0x5a 11 ggg rrr => "cvtps2pd " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5a @addr      => "cvtps2pd " ssereg[8+$g] "," $addr ;
  0x0f 0x5b 11 ggg rrr => "cvtdq2ps " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x5b @addr      => "cvtdq2ps " ssereg[8+$g] "," $addr ;
  0x0f 0xe6 11 ggg rrr => "cvtpd2dq " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0xe6 @addr      => "cvtpd2dq " ssereg[8+$g] "," $addr ;
  0x0f 0x6c 11 ggg rrr => "punpcklqdq " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x6c @addr      => "punpcklqdq " ssereg[8+$g] "," $addr ;
  0x0f 0x6d 11 ggg rrr => "punpckhqdq " ssereg[8+$g] "," ssereg[8+$r] ;
  0x0f 0x6d @addr      => "punpckhqdq " ssereg[8+$g] "," $addr ;
  0x0f 0xf0 @addr => "lddqu " ssereg[8+$g] "," $addr ;
  # non-temporal stores + masked move
  0x0f 0x2b @addr => "movntps " $addr "," ssereg[8+$g] ;
  0x0f 0xe7 @addr => "movntq " $addr "," ssereg[$opsiz*8+$g] ;
  0x0f 0xc3 @addr => "movnti " $addr "," greg[$g] ;
  0x0f 0xf7 11 ggg rrr => "maskmovq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  # 0F AE memory digits: fxsave/fxrstor, ldmxcsr/stmxcsr, xsave family
  0x0f 0xae @addr(0) => "fxsave " $addr ;
  0x0f 0xae @addr(1) => "fxrstor " $addr ;
  0x0f 0xae @addr(2) => "ldmxcsr " $addr ;
  0x0f 0xae @addr(3) => "stmxcsr " $addr ;
  0x0f 0xae @addr(4) => "xsave " $addr ;
  0x0f 0xae @addr(5) => "xrstor " $addr ;
  0x0f 0xae @addr(6) => "xsaveopt " $addr ;
  # 0F 38: SSSE3 packed (pshufb/phadd/phsub/psign/pabs/pmaddubsw/pmulhrsw)
  0x0f 0x38 0x00 11 ggg rrr => "pshufb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x00 @addr      => "pshufb " ssereg[$opsiz*8+$g] "," $addr ;
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
  0x0f 0x38 0x1c 11 ggg rrr => "pabsb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x1c @addr      => "pabsb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x1d 11 ggg rrr => "pabsw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x1d @addr      => "pabsw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x1e 11 ggg rrr => "pabsd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x1e @addr      => "pabsd " ssereg[$opsiz*8+$g] "," $addr ;
  # 0F 38: SSE4.1/4.2 packed (blend/ptest/pmovsx/pmovzx/pmul/pcmp/pmin/pmax)
  0x0f 0x38 0x10 11 ggg rrr => "pblendvb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x10 @addr      => "pblendvb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x14 11 ggg rrr => "blendvps " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x14 @addr      => "blendvps " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x15 11 ggg rrr => "blendvpd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x15 @addr      => "blendvpd " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x17 11 ggg rrr => "ptest " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x17 @addr      => "ptest " ssereg[$opsiz*8+$g] "," $addr ;
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
  0x0f 0x38 0x29 11 ggg rrr => "pcmpeqq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x29 @addr      => "pcmpeqq " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x2b 11 ggg rrr => "packusdw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x2b @addr      => "packusdw " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0x30 11 ggg rrr => "pmovzxbw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0x30 @addr      => "pmovzxbw " ssereg[$opsiz*8+$g] "," $addr ;
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
  0x0f 0x38 0x2a @addr => "movntdqa " ssereg[8+$g] "," $addr ;
  # 0F 38: AES + SHA
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
  0x0f 0x38 0xc8 11 ggg rrr => "sha1nexte " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0xc8 @addr      => "sha1nexte " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0xc9 11 ggg rrr => "sha1msg1 " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0xc9 @addr      => "sha1msg1 " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0xca 11 ggg rrr => "sha1msg2 " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0xca @addr      => "sha1msg2 " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0xcb 11 ggg rrr => "sha256rnds2 " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0xcb @addr      => "sha256rnds2 " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0xcc 11 ggg rrr => "sha256msg1 " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0xcc @addr      => "sha256msg1 " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0xcd 11 ggg rrr => "sha256msg2 " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0xcd @addr      => "sha256msg2 " ssereg[$opsiz*8+$g] "," $addr ;
  # 0F 3A: SSE4.1/4.2 imm8 (round/blend/dp/mpsadbw/pclmul/pcmpstr/palignr)
  0x0f 0x3a 0x08 11 ggg rrr @imm8 => "roundps " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x08 @addr      @imm8 => "roundps " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x09 11 ggg rrr @imm8 => "roundpd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x09 @addr      @imm8 => "roundpd " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0a 11 ggg rrr @imm8 => "roundss " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0a @addr      @imm8 => "roundss " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0b 11 ggg rrr @imm8 => "roundsd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0b @addr      @imm8 => "roundsd " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0c 11 ggg rrr @imm8 => "blendps " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0c @addr      @imm8 => "blendps " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0d 11 ggg rrr @imm8 => "blendpd " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0d @addr      @imm8 => "blendpd " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0e 11 ggg rrr @imm8 => "pblendw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0e @addr      @imm8 => "pblendw " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x0f 11 ggg rrr @imm8 => "palignr " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x0f @addr      @imm8 => "palignr " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x21 11 ggg rrr @imm8 => "insertps " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0x21 @addr      @imm8 => "insertps " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
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
  0x0f 0x3a 0xcc 11 ggg rrr @imm8 => "sha1rnds4 " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0xcc @addr      @imm8 => "sha1rnds4 " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0xdf 11 ggg rrr @imm8 => "aeskeygenassist " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0xdf @addr      @imm8 => "aeskeygenassist " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  # 0F 3A: pextr/pinsr/extractps (mixed greg<->xmm, imm8)
  0x0f 0x3a 0x14 11 ggg rrr @imm8 => "pextrb " greg[$r] "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x14 @addr      @imm8 => "pextrb " $addr "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x15 11 ggg rrr @imm8 => "pextrw " greg[$r] "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x15 @addr      @imm8 => "pextrw " $addr "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x16 11 ggg rrr @imm8 => "pextrd " greg[$r] "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x16 @addr      @imm8 => "pextrd " $addr "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x17 11 ggg rrr @imm8 => "extractps " greg[$r] "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x17 @addr      @imm8 => "extractps " $addr "," ssereg[$opsiz*8+$g] "," hex($imm8) ;
  0x0f 0x3a 0x20 11 ggg rrr @imm8 => "pinsrb " ssereg[$opsiz*8+$g] "," greg[$r] "," hex($imm8) ;
  0x0f 0x3a 0x20 @addr      @imm8 => "pinsrb " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0x22 11 ggg rrr @imm8 => "pinsrd " ssereg[$opsiz*8+$g] "," greg[$r] "," hex($imm8) ;
  0x0f 0x3a 0x22 @addr      @imm8 => "pinsrd " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  # system: sysenter/sysexit/rsm/rdpmc + 0F 01 reg-form specials (round-trip by rm)
  0x0f 0x34 => "sysenter" ;
  0x0f 0x35 => "sysexit" ;
  0x0f 0xaa => "rsm" ;
  0x0f 0x33 => "rdpmc" ;
  0x0f 0x01 11 001 rrr => "monitor" ;
  0x0f 0x01 11 111 rrr => "rdtscp" ;
  # popcnt (F3 0F B8); lzcnt/tzcnt round-trip via bsr/bsf + F3 replay
  0x0f 0xb8 11 ggg rrr => "popcnt " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x0f 0xb8 @addr      => "popcnt " greg[$opsiz*8+$g] "," $addr ;
  # movbe + crc32 (0F 38 F0/F1). One mnemonic per opcode so reg+mem forms merge;
  # the F2 prefix (crc32 vs movbe) and operand order are replayed, so bytes are exact.
  0x0f 0x38 0xf0 11 ggg rrr => "crc32 " greg[$opsiz*8+$g] "," rgb[$r] ;
  0x0f 0x38 0xf0 @addr      => "crc32 " greg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0xf1 11 ggg rrr => "crc32 " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x0f 0x38 0xf1 @addr      => "crc32 " greg[$opsiz*8+$g] "," $addr ;
  # multi-byte nop (0F 1F, reg+mem) + prefetch group (0F 0D) + nop0d reg form
  0x0f 0x1f 11 ggg rrr => "nop " greg[$r] "," greg[$g] ;
  0x0f 0x1f @addr      => "nop " $addr ;
  0x0f 0x0d @addr(0) => "prefetch " $addr ;
  0x0f 0x0d @addr(1) => "prefetchw " $addr ;
  0x0f 0x0d @addr(2) => "prefetchwt1 " $addr ;
  0x0f 0x0d @addr(3) => "prefetch3 " $addr ;
  0x0f 0x0d @addr(4) => "prefetch4 " $addr ;
  0x0f 0x0d @addr(5) => "prefetch5 " $addr ;
  0x0f 0x0d @addr(6) => "prefetch6 " $addr ;
  0x0f 0x0d @addr(7) => "prefetch7 " $addr ;
  0x0f 0x0d 11 000 rrr => "nop0d " greg[$r] ;
  0x0f 0x0d 11 001 rrr => "nop0d " greg[$r] ;
  0x0f 0x0d 11 010 rrr => "nop0d " greg[$r] ;
  0x0f 0x0d 11 011 rrr => "nop0d " greg[$r] ;
  0x0f 0x0d 11 100 rrr => "nop0d " greg[$r] ;
  0x0f 0x0d 11 101 rrr => "nop0d " greg[$r] ;
  0x0f 0x0d 11 110 rrr => "nop0d " greg[$r] ;
  0x0f 0x0d 11 111 rrr => "nop0d " greg[$r] ;
  # 0F 01 reg-form specials (vmcall/xgetbv/vmrun/...; round-trip by rm)
  0x0f 0x01 11 000 rrr => "vmcall" ;
  0x0f 0x01 11 010 rrr => "xgetbv" ;
  0x0f 0x01 11 011 rrr => "vmrun" ;
  0x0f 0x01 11 101 rrr => "rstorssp" ;
  # ud1 (0F B9), 0F D6 movq/movq2dq/movdq2q, int1 (F1)
  0x0f 0xb9 11 ggg rrr => "ud1 " greg[$g] "," greg[$r] ;
  0x0f 0xb9 @addr      => "ud1 " greg[$g] "," $addr ;
  0x0f 0xd6 11 ggg rrr => "movq " ssereg[8+$r] "," ssereg[8+$g] ;
  0x0f 0xd6 @addr      => "movq " $addr "," ssereg[8+$g] ;
  0xf1 => "int1" ;
  0x0f 0x0f 11 ggg rrr @imm8 => "_3dnow " ssereg[$g] "," ssereg[$r] "," hex($imm8) ;
  0x0f 0x0f @addr      @imm8 => "_3dnow " ssereg[$g] "," $addr "," hex($imm8) ;
  # SSE move STORES (reverse direction of 10/28/12/16/6F) + syscall/sysret/femms/salc
  0xd6 => "salc" ;
  0x0f 0x05 => "syscall" ;
  0x0f 0x07 => "sysret" ;
  0x0f 0x0e => "femms" ;
  0x0f 0x11 11 ggg rrr => "movups " ssereg[8+$r] "," ssereg[8+$g] ;
  0x0f 0x11 @addr      => "movups " $addr "," ssereg[8+$g] ;
  0x0f 0x13 @addr      => "movlps " $addr "," ssereg[8+$g] ;
  0x0f 0x17 @addr      => "movhps " $addr "," ssereg[8+$g] ;
  0x0f 0x29 11 ggg rrr => "movaps " ssereg[8+$r] "," ssereg[8+$g] ;
  0x0f 0x29 @addr      => "movaps " $addr "," ssereg[8+$g] ;
  0x0f 0x7f 11 ggg rrr => "movdqa " ssereg[$opsiz*8+$r] "," ssereg[$opsiz*8+$g] ;
  0x0f 0x7f @addr      => "movdqa " $addr "," ssereg[$opsiz*8+$g] ;
  # vmread/vmwrite (0F 78/79), ud0 (0F FF)
  0x0f 0x78 11 ggg rrr => "vmread " greg[$r] "," greg[$g] ;
  0x0f 0x78 @addr      => "vmread " $addr "," greg[$g] ;
  0x0f 0x79 11 ggg rrr => "vmwrite " greg[$g] "," greg[$r] ;
  0x0f 0x79 @addr      => "vmwrite " greg[$g] "," $addr ;
  0x0f 0xff 11 ggg rrr => "ud0 " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;
  0x0f 0xff @addr      => "ud0 " greg[$opsiz*8+$g] "," $addr ;
  # reserved-nop / MPX bnd / CET space (0F 19-1E): corpus1.p mnemonics
  0x0f 0x19 11 ggg rrr => "nop19 " greg[$r] "," greg[$g] ;
  0x0f 0x19 @addr      => "nop19 " $addr ;
  0x0f 0x1a 11 ggg rrr => "nop1a " greg[$r] "," greg[$g] ;
  0x0f 0x1a @addr      => "nop1a " $addr ;
  0x0f 0x1b 11 ggg rrr => "nop1b " greg[$r] "," greg[$g] ;
  0x0f 0x1b @addr      => "nop1b " $addr ;
  0x0f 0x1d 11 ggg rrr => "nop1d " greg[$r] "," greg[$g] ;
  0x0f 0x1d @addr      => "nop1d " $addr ;
  0x0f 0x1c @addr(0) => "cldemote " $addr ;
  0x0f 0x1c @addr(1) => "nop1c " $addr ;
  0x0f 0x1c @addr(2) => "nop1c " $addr ;
  0x0f 0x1c @addr(3) => "nop1c " $addr ;
  0x0f 0x1c @addr(4) => "nop1c " $addr ;
  0x0f 0x1c @addr(5) => "nop1c " $addr ;
  0x0f 0x1c @addr(6) => "nop1c " $addr ;
  0x0f 0x1c @addr(7) => "nop1c " $addr ;
  0x0f 0x1c 11 000 rrr => "nop1c " greg[$r] ;
  0x0f 0x1c 11 001 rrr => "nop1c " greg[$r] ;
  0x0f 0x1c 11 010 rrr => "nop1c " greg[$r] ;
  0x0f 0x1c 11 011 rrr => "nop1c " greg[$r] ;
  0x0f 0x1c 11 100 rrr => "nop1c " greg[$r] ;
  0x0f 0x1c 11 101 rrr => "nop1c " greg[$r] ;
  0x0f 0x1c 11 110 rrr => "nop1c " greg[$r] ;
  0x0f 0x1c 11 111 rrr => "nop1c " greg[$r] ;
  0x0f 0x1e 11 ggg rrr => "nop " greg[$r] "," greg[$g] ;
  0x0f 0x1e @addr      => "nop " $addr ;
  # VIA PadLock crypto (0F A6/A7 reg-form groups; round-trip by rm)
  0x0f 0xa6 11 000 rrr => "montmul" ;
  0x0f 0xa6 11 001 rrr => "xsha1" ;
  0x0f 0xa6 11 010 rrr => "xsha256" ;
  0x0f 0xa7 11 000 rrr => "xstore" ;
  0x0f 0xa7 11 001 rrr => "xcryptecb" ;
  0x0f 0xa7 11 010 rrr => "xcryptcbc" ;
  0x0f 0xa7 11 011 rrr => "xcryptctr" ;
  0x0f 0xa7 11 100 rrr => "xcryptcfb" ;
  0x0f 0xa7 11 101 rrr => "xcryptofb" ;
  # more 0F 38: invept/invvpid/invpcid, gf2p8mulb, adcx/adox, wrss, movdir, aadd, encodekey
  0x0f 0x38 0x80 @addr => "invept " greg[$g] "," $addr ;
  0x0f 0x38 0x81 @addr => "invvpid " greg[$g] "," $addr ;
  0x0f 0x38 0x82 @addr => "invpcid " greg[$g] "," $addr ;
  0x0f 0x38 0xcf 11 ggg rrr => "gf2p8mulb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;
  0x0f 0x38 0xcf @addr      => "gf2p8mulb " ssereg[$opsiz*8+$g] "," $addr ;
  0x0f 0x38 0xf6 11 ggg rrr => "adcx " greg[$g] "," greg[$r] ;
  0x0f 0x38 0xf6 @addr      => "adcx " greg[$g] "," $addr ;
  0x0f 0x38 0xf5 @addr      => "wrssd " $addr "," greg[$g] ;
  0x0f 0x38 0xf8 @addr      => "movdir64b " greg[$g] "," $addr ;
  0x0f 0x38 0xf9 @addr      => "movdiri " $addr "," greg[$g] ;
  0x0f 0x38 0xfc @addr      => "aadd " $addr "," greg[$g] ;
  0x0f 0x38 0xfa 11 ggg rrr => "encodekey128 " greg[$g] "," greg[$r] ;
  0x0f 0x38 0xfb 11 ggg rrr => "encodekey256 " greg[$g] "," greg[$r] ;
  0x0f 0x38 0xd8 @addr(0) => "aesencwide128kl " $addr ;
  0x0f 0x38 0xd8 @addr(1) => "aesdecwide128kl " $addr ;
  0x0f 0x38 0xd8 @addr(2) => "aesencwide256kl " $addr ;
  0x0f 0x38 0xd8 @addr(3) => "aesdecwide256kl " $addr ;
  # more 0F 3A: gf2p8affine(inv)qb, hreset
  0x0f 0x3a 0xce 11 ggg rrr @imm8 => "gf2p8affineqb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0xce @addr      @imm8 => "gf2p8affineqb " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0xcf 11 ggg rrr @imm8 => "gf2p8affineinvqb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;
  0x0f 0x3a 0xcf @addr      @imm8 => "gf2p8affineinvqb " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;
  0x0f 0x3a 0xf0 11 ggg rrr @imm8 => "hreset " greg[$r] "," hex($imm8) ;

}

submatch main { @pfx(0) => $pfx }
