#!/usr/bin/env python3
# gen_rules.py -- emit the insn{} body of corpus.p covering everything corpus.bin
# exercises. Repetitive families (ALU, shift, x87, MMX/SSE) are generated so the
# 8-way symmetry stays correct. All rules use only constructs gen.py supports:
# literal mnemonics, /digit groups (one-byte AND -- after the gen.py patch --
# two-byte), embedded reg (10111 bbb), cond[$c], and table[$opsiz] selection.
# Mandatory-prefix variants (66/F3/F2 SSE) need no rule of their own: the prefix
# is replayed from pfx[], so one literal rule per opcode round-trips them all.

out = []
def R(s): out.append("  " + s)
def C(s): out.append("  # " + s)

# ----- ALU: add/or/adc/sbb/and/sub/xor/cmp, bases 00/08/10/18/20/28/30/38 -----
ALU = [("add",0x00),("or",0x08),("adc",0x10),("sbb",0x18),
       ("and",0x20),("sub",0x28),("xor",0x30),("cmp",0x38)]
C("===== ALU: 8 ops x {r/m8,r8 | r/m,r | r8,r/m8 | r,r/m | al,imm8 | eax,immz} =====")
for m,b in ALU:
  R('0x%02x 11 ggg rrr => "%s " rgb[$r] "," rgb[$g] ;'                      % (b+0,m))
  R('0x%02x @addr      => "%s" sfx[1] " " $addr "," rgb[$g] ;'              % (b+0,m))
  R('0x%02x 11 ggg rrr => "%s " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;'  % (b+1,m))
  R('0x%02x @addr      => "%s " $addr "," greg[$opsiz*8+$g] ;'              % (b+1,m))
  R('0x%02x 11 ggg rrr => "%s " rgb[$g] "," rgb[$r] ;'                      % (b+2,m))
  R('0x%02x @addr      => "%s " rgb[$g] "," $addr ;'                        % (b+2,m))
  R('0x%02x 11 ggg rrr => "%s " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;'  % (b+3,m))
  R('0x%02x @addr      => "%s " greg[$opsiz*8+$g] "," $addr ;'              % (b+3,m))
  R('0x%02x @imm8 => "%s al," hex($imm8) ;'                                 % (b+4,m))
  R('0x%02x @immz => "%s " greg[$opsiz*8+0] "," hex($immz) ;'               % (b+5,m))

# ----- ALU immediate groups 80/81/82/83 (reg digit picks the op) ------------
C("ALU imm groups 80(r/m8,imm8) 81(r/m,immz) 82(r/m8,imm8 alias) 83(r/m,imm8sx)")
for i,(m,_) in enumerate(ALU):
  R('0x80 11 %03s rrr @imm8 => "%s " rgb[$r] "," hex($imm8) ;'        % (format(i,"03b"),m))
  R('0x80 @addr(%d)   @imm8 => "%s" sfx[1] " " $addr "," hex($imm8) ;'% (i,m))
  R('0x82 11 %03s rrr @imm8 => "%s " rgb[$r] "," hex($imm8) ;'        % (format(i,"03b"),m))
  R('0x82 @addr(%d)   @imm8 => "%s" sfx[1] " " $addr "," hex($imm8) ;'% (i,m))
  R('0x81 11 %03s rrr @immz => "%s " greg[$opsiz*8+$r] "," hex($immz) ;'     % (format(i,"03b"),m))
  R('0x81 @addr(%d)   @immz => "%s" sfx[4] " " $addr "," hex($immz) ;'       % (i,m))
  R('0x83 11 %03s rrr @imm8 => "%s " greg[$opsiz*8+$r] "," hex(sx8($imm8)) ;'% (format(i,"03b"),m))
  R('0x83 @addr(%d)   @imm8 => "%s" sfx[4] " " $addr "," hex(sx8($imm8)) ;'  % (i,m))

# ----- test ----------------------------------------------------------------
C("test")
R('0x84 11 ggg rrr => "test " rgb[$r] "," rgb[$g] ;')
R('0x84 @addr      => "test" sfx[1] " " $addr "," rgb[$g] ;')
R('0x85 11 ggg rrr => "test " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;')
R('0x85 @addr      => "test " $addr "," greg[$opsiz*8+$g] ;')
R('0xa8 @imm8 => "test al," hex($imm8) ;')
R('0xa9 @immz => "test " greg[$opsiz*8+0] "," hex($immz) ;')

# ----- shift / rotate groups c0 c1 d0 d1 d2 d3 -----------------------------
SHIFT = ["rol","ror","rcl","rcr","shl","shr","sal","sar"]
C("shift/rotate groups: c0/c1=imm8, d0/d1=,1, d2/d3=,cl  (reg digit picks op)")
for i,m in enumerate(SHIFT):
  d=format(i,"03b")
  R('0xc0 11 %s rrr @imm8 => "%s " rgb[$r] "," hex($imm8) ;'                % (d,m))
  R('0xc0 @addr(%d)   @imm8 => "%s" sfx[1] " " $addr "," hex($imm8) ;'      % (i,m))
  R('0xc1 11 %s rrr @imm8 => "%s " greg[$opsiz*8+$r] "," hex($imm8) ;'      % (d,m))
  R('0xc1 @addr(%d)   @imm8 => "%s" sfx[4] " " $addr "," hex($imm8) ;'      % (i,m))
  R('0xd0 11 %s rrr => "%s " rgb[$r] ",1" ;'                               % (d,m))
  R('0xd0 @addr(%d)   => "%s" sfx[1] " " $addr ",1" ;'                     % (i,m))
  R('0xd1 11 %s rrr => "%s " greg[$opsiz*8+$r] ",1" ;'                     % (d,m))
  R('0xd1 @addr(%d)   => "%s" sfx[4] " " $addr ",1" ;'                     % (i,m))
  R('0xd2 11 %s rrr => "%s " rgb[$r] ",cl" ;'                             % (d,m))
  R('0xd2 @addr(%d)   => "%s" sfx[1] " " $addr ",cl" ;'                   % (i,m))
  R('0xd3 11 %s rrr => "%s " greg[$opsiz*8+$r] ",cl" ;'                   % (d,m))
  R('0xd3 @addr(%d)   => "%s" sfx[4] " " $addr ",cl" ;'                   % (i,m))

# ----- unary group f6/f7 (test/not/neg/mul/imul/div/idiv) ------------------
F67 = ["test","test","not","neg","mul","imul","div","idiv"]
C("f6/f7 group: /0,/1 test imm ; /2 not /3 neg /4 mul /5 imul /6 div /7 idiv")
for i,m in enumerate(F67):
  d=format(i,"03b")
  if i<2:   # test takes an immediate
    R('0xf6 11 %s rrr @imm8 => "test " rgb[$r] "," hex($imm8) ;' % d)
    R('0xf7 11 %s rrr @immz => "test " greg[$opsiz*8+$r] "," hex($immz) ;' % d)
    R('0xf6 @addr(%d)   @imm8 => "test" sfx[1] " " $addr "," hex($imm8) ;' % i)
    R('0xf7 @addr(%d)   @immz => "test" sfx[4] " " $addr "," hex($immz) ;' % i)
  else:
    R('0xf6 11 %s rrr => "%s " rgb[$r] ;'                  % (d,m))
    R('0xf6 @addr(%d)   => "%s" sfx[1] " " $addr ;'        % (i,m))
    R('0xf7 11 %s rrr => "%s " greg[$opsiz*8+$r] ;'        % (d,m))
    R('0xf7 @addr(%d)   => "%s" sfx[4] " " $addr ;'        % (i,m))

# ----- inc/dec group fe/ff, call/jmp/push via ff --------------------------
C("fe (inc/dec r/m8), ff (/0 inc /1 dec /2 call /3 callf /4 jmp /5 jmpf /6 push)")
R('01000 bbb => "inc " greg[$opsiz*8+$b] ;')
R('01001 bbb => "dec " greg[$opsiz*8+$b] ;')
R('0xfe 11 000 rrr => "inc " rgb[$r] ;')
R('0xfe @addr(0)   => "inc" sfx[1] " " $addr ;')
R('0xfe 11 001 rrr => "dec " rgb[$r] ;')
R('0xfe @addr(1)   => "dec" sfx[1] " " $addr ;')
FF = ["inc","dec","call","callf","jmp","jmpf","push","push"]
for i,m in enumerate(FF[:7]):
  d=format(i,"03b")
  R('0xff 11 %s rrr => "%s " greg[$opsiz*8+$r] ;' % (d,m))
  R('0xff @addr(%d)   => "%s" sfx[4] " " $addr ;' % (i,m))

# ----- imul 3-operand, 0F AF -----------------------------------------------
C("imul: 0F AF (r,r/m), 69 (r,r/m,immz), 6b (r,r/m,imm8)")
R('0x0f 0xaf 11 ggg rrr => "imul " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;')
R('0x0f 0xaf @addr      => "imul " greg[$opsiz*8+$g] "," $addr ;')
R('0x69 11 ggg rrr @immz => "imul " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] "," hex($immz) ;')
R('0x69 @addr      @immz => "imul " greg[$opsiz*8+$g] "," $addr "," hex($immz) ;')
R('0x6b 11 ggg rrr @imm8 => "imul " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] "," hex(sx8($imm8)) ;')
R('0x6b @addr      @imm8 => "imul " greg[$opsiz*8+$g] "," $addr "," hex(sx8($imm8)) ;')

# ----- mov family ----------------------------------------------------------
C("mov family")
R('0x8d @addr => "lea " greg[$opsiz*8+$g] "," $addr ;')
R('0x88 11 ggg rrr => "mov " rgb[$r] "," rgb[$g] ;')
R('0x88 @addr      => "mov" sfx[1] " " $addr "," rgb[$g] ;')
R('0x89 11 ggg rrr => "mov " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;')
R('0x89 @addr      => "mov " $addr "," greg[$opsiz*8+$g] ;')
R('0x8a 11 ggg rrr => "mov " rgb[$g] "," rgb[$r] ;')
R('0x8a @addr      => "mov " rgb[$g] "," $addr ;')
R('0x8b 11 ggg rrr => "mov " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;')
R('0x8b @addr      => "mov " greg[$opsiz*8+$g] "," $addr ;')
R('0x8c 11 ggg rrr => "mov " greg[$opsiz*8+$r] "," sreg[$g] ;')
R('0x8c @addr      => "mov " $addr "," sreg[$g] ;')
R('0x8e 11 ggg rrr => "mov " sreg[$g] "," greg[$opsiz*8+$r] ;')
R('0x8e @addr      => "mov " sreg[$g] "," $addr ;')
R('0xa0 @imm32 => "mov al," seg[$segidx] "[@" hex($imm32) "]" ;')
R('0xa1 @imm32 => "mov " greg[$opsiz*8+0] "," seg[$segidx] "[@" hex($imm32) "]" ;')
R('0xa2 @imm32 => "mov " seg[$segidx] "[@" hex($imm32) "],al" ;')
R('0xa3 @imm32 => "mov " seg[$segidx] "[@" hex($imm32) "]," greg[$opsiz*8+0] ;')
R('10110 bbb @imm8 => "mov " rgb[$b] "," hex($imm8) ;')
R('10111 bbb @immz => "mov " greg[$opsiz*8+$b] "," hex($immz) ;')
R('0xc6 11 000 rrr @imm8 => "mov " rgb[$r] "," hex($imm8) ;')
R('0xc6 11 111 rrr @imm8 => "xabort " hex($imm8) ;')             # C6 /7 (F8) = xabort
R('0xc6 @addr(0)   @imm8 => "mov" sfx[1] " " $addr "," hex($imm8) ;')
R('0xc7 11 000 rrr @immz => "mov " greg[$opsiz*8+$r] "," hex($immz) ;')
R('0xc7 11 111 rrr @immz => "xbegin " hex($immz) ;')             # C7 /7 (F8) = xbegin
R('0xc7 @addr(0)   @immz => "mov" sfx[4] " " $addr "," hex($immz) ;')
# movzx/movsx (byte and word source)
R('0x0f 0xb6 11 ggg rrr => "movzx " greg[$opsiz*8+$g] "," rgb[$r] ;')
R('0x0f 0xb6 @addr      => "movzx" sfx[1] " " greg[$opsiz*8+$g] "," $addr ;')
R('0x0f 0xb7 11 ggg rrr => "movzx " greg[$opsiz*8+$g] "," greg[8+$r] ;')
R('0x0f 0xb7 @addr      => "movzx" sfx[2] " " greg[$opsiz*8+$g] "," $addr ;')
R('0x0f 0xbe 11 ggg rrr => "movsx " greg[$opsiz*8+$g] "," rgb[$r] ;')
R('0x0f 0xbe @addr      => "movsx" sfx[1] " " greg[$opsiz*8+$g] "," $addr ;')
R('0x0f 0xbf 11 ggg rrr => "movsx " greg[$opsiz*8+$g] "," greg[8+$r] ;')
R('0x0f 0xbf @addr      => "movsx" sfx[2] " " greg[$opsiz*8+$g] "," $addr ;')

# ----- xchg / xlat ---------------------------------------------------------
C("xchg (90+r, with 90=nop overriding), 86/87 r/m,r ; xlat")
R('10010 bbb => "xchg " greg[$opsiz*8+$b] ",eax" ;')
R('0x86 11 ggg rrr => "xchg " rgb[$r] "," rgb[$g] ;')
R('0x86 @addr      => "xchg" sfx[1] " " $addr "," rgb[$g] ;')
R('0x87 11 ggg rrr => "xchg " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;')
R('0x87 @addr      => "xchg " $addr "," greg[$opsiz*8+$g] ;')
R('0xd7 => "xlat" ;')
R('0x0f 11001 bbb => "bswap " greg[$opsiz*8+$b] ;')

# ----- push / pop ----------------------------------------------------------
C("push/pop: short reg, imm, r/m, segment registers, pusha/popa, pushf/popf")
R('01010 bbb => "push " greg[$opsiz*8+$b] ;')
R('01011 bbb => "pop "  greg[$opsiz*8+$b] ;')
R('0x68 @immz => "push " hex($immz) ;')
R('0x6a @imm8 => "push " hex(sx8($imm8)) ;')
R('0x8f 11 000 rrr => "pop " greg[$opsiz*8+$r] ;')
R('0x8f @addr(0)   => "pop" sfx[4] " " $addr ;')
for (op,sg) in [(0x06,"es"),(0x0e,"cs"),(0x16,"ss"),(0x1e,"ds")]:
  R('0x%02x => "push %s" ;' % (op,sg))
for (op,sg) in [(0x07,"es"),(0x17,"ss"),(0x1f,"ds")]:
  R('0x%02x => "pop %s" ;' % (op,sg))
R('0x0f 0xa0 => "push fs" ;')
R('0x0f 0xa8 => "push gs" ;')
R('0x0f 0xa1 => "pop fs" ;')
R('0x0f 0xa9 => "pop gs" ;')
R('0x60 => "pusha" ;')
R('0x61 => "popa" ;')
R('0x9c => "pushf" ;')
R('0x9d => "popf" ;')

# ----- control flow --------------------------------------------------------
C("jcc rel8/rel32, jmp/call, loop/jecxz, ret/retf/int/iret, leave/enter")
R('0111 cccc @rel8 => "j" cond[$c] " " hex($rel8) ;')
R('0x0f 1000 cccc @relz => "j" cond[$c] " " hex($relz) ;')
R('0xeb @rel8 => "jmp " hex($rel8) ;')
R('0xe9 @relz => "jmp " hex($relz) ;')
R('0xe8 @relz => "call " hex($relz) ;')
R('0xea @imm32 @imm16 => "jmpf " hex($imm16) ":" hex($imm32) ;')
R('0x9a @imm32 @imm16 => "callf " hex($imm16) ":" hex($imm32) ;')
R('0xe0 @rel8 => "loopne " hex($rel8) ;')
R('0xe1 @rel8 => "loope " hex($rel8) ;')
R('0xe2 @rel8 => "loop " hex($rel8) ;')
R('0xe3 @rel8 => "jecxz " hex($rel8) ;')
R('0xc3 => "ret" ;')
R('0xc2 @imm16 => "ret " hex($imm16) ;')
R('0xcb => "retf" ;')
R('0xca @imm16 => "retf " hex($imm16) ;')
R('0xcd @imm8 => "int " hex($imm8) ;')
R('0xcc => "int3" ;')
R('0xce => "into" ;')
R('0xcf => "iret" ;')
R('0xc9 => "leave" ;')
R('0xc8 @imm16 @imm8 => "enter " hex($imm16) "," hex($imm8) ;')

# ----- setcc / cmovcc / bit ops / shld-shrd --------------------------------
C("setcc, cmovcc, bt/bts/btr/btc/bsf/bsr, shld/shrd")
R('0x0f 1001 cccc 11 ggg rrr => "set" cond[$c] " " rgb[$r] ;')
R('0x0f 1001 cccc @addr      => "set" cond[$c] sfx[1] " " $addr ;')
R('0x0f 0100 cccc 11 ggg rrr => "cmov" cond[$c] " " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;')
R('0x0f 0100 cccc @addr      => "cmov" cond[$c] " " greg[$opsiz*8+$g] "," $addr ;')
for (op,m) in [(0xa3,"bt"),(0xab,"bts"),(0xb3,"btr"),(0xbb,"btc")]:
  R('0x0f 0x%02x 11 ggg rrr => "%s " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;' % (op,m))
  R('0x0f 0x%02x @addr      => "%s " $addr "," greg[$opsiz*8+$g] ;' % (op,m))
for (op,m) in [(0xbc,"bsf"),(0xbd,"bsr")]:
  R('0x0f 0x%02x 11 ggg rrr => "%s " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;' % (op,m))
  R('0x0f 0x%02x @addr      => "%s " greg[$opsiz*8+$g] "," $addr ;' % (op,m))
# 0F BA group /4 bt /5 bts /6 btr /7 btc, imm8  (two-byte group)
for i,m in [(4,"bt"),(5,"bts"),(6,"btr"),(7,"btc")]:
  R('0x0f 0xba 11 %s rrr @imm8 => "%s " greg[$opsiz*8+$r] "," hex($imm8) ;' % (format(i,"03b"),m))
  R('0x0f 0xba @addr(%d)   @imm8 => "%s" sfx[4] " " $addr "," hex($imm8) ;' % (i,m))
R('0x0f 0xa4 11 ggg rrr @imm8 => "shld " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] "," hex($imm8) ;')
R('0x0f 0xa4 @addr      @imm8 => "shld " $addr "," greg[$opsiz*8+$g] "," hex($imm8) ;')
R('0x0f 0xa5 11 ggg rrr => "shld " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ",cl" ;')
R('0x0f 0xa5 @addr      => "shld " $addr "," greg[$opsiz*8+$g] ",cl" ;')
R('0x0f 0xac 11 ggg rrr @imm8 => "shrd " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] "," hex($imm8) ;')
R('0x0f 0xac @addr      @imm8 => "shrd " $addr "," greg[$opsiz*8+$g] "," hex($imm8) ;')
R('0x0f 0xad 11 ggg rrr => "shrd " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ",cl" ;')
R('0x0f 0xad @addr      => "shrd " $addr "," greg[$opsiz*8+$g] ",cl" ;')

# ----- far-pointer loads, bound, arpl --------------------------------------
C("les/lds (c4/c5), lss/lfs/lgs (0F b2/b4/b5), bound, arpl")
for (op,m) in [(0xc4,"les"),(0xc5,"lds")]:
  R('0x%02x @addr => "%s " greg[$opsiz*8+$g] "," $addr ;' % (op,m))
for (op,m) in [(0xb2,"lss"),(0xb4,"lfs"),(0xb5,"lgs")]:
  R('0x0f 0x%02x @addr => "%s " greg[$opsiz*8+$g] "," $addr ;' % (op,m))
R('0x62 @addr => "bound " greg[$opsiz*8+$g] "," $addr ;')
R('0x63 11 ggg rrr => "arpl " greg[8+$r] "," greg[8+$g] ;')
R('0x63 @addr      => "arpl " $addr "," greg[8+$g] ;')

# ----- flags / misc one-byte ops -------------------------------------------
C("flag ops, daa/das/aaa/aas/aam/aad, nop/hlt/fwait, cpuid/rdtsc/rd-wrmsr/ud2")
for (op,m) in [(0xf5,"cmc"),(0xf8,"clc"),(0xf9,"stc"),(0xfa,"cli"),(0xfb,"sti"),
               (0xfc,"cld"),(0xfd,"std"),(0x9e,"sahf"),(0x9f,"lahf"),
               (0x27,"daa"),(0x2f,"das"),(0x37,"aaa"),(0x3f,"aas"),
               (0x90,"nop"),(0xf4,"hlt"),(0x9b,"fwait")]:
  R('0x%02x => "%s" ;' % (op,m))
R('0xd4 @imm8 => "aam " hex($imm8) ;')
R('0xd5 @imm8 => "aad " hex($imm8) ;')
R('0x0f 0xa2 => "cpuid" ;')
R('0x0f 0x31 => "rdtsc" ;')
R('0x0f 0x32 => "rdmsr" ;')
R('0x0f 0x30 => "wrmsr" ;')
R('0x0f 0x0b => "ud2" ;')
R('0x99 => cdqw[$opsiz] ;')
R('0x98 => cbwe[$opsiz] ;')

# ----- string ops ----------------------------------------------------------
C("string ops (implicit operands; rep/repnz ride in the prefix run)")
for (op,m) in [(0xa4,"movsb"),(0xa6,"cmpsb"),(0xaa,"stosb"),(0xac,"lodsb"),(0xae,"scasb")]:
  R('0x%02x => "%s" ;' % (op,m))
for (op,tab) in [(0xa5,"movs"),(0xa7,"cmps"),(0xab,"stos"),(0xad,"lods"),(0xaf,"scas")]:
  R('0x%02x => %s[$opsiz] ;' % (op,tab))

# ----- I/O -----------------------------------------------------------------
C("in/out (imm8 and dx forms), ins/outs")
R('0xe4 @imm8 => "in al," hex($imm8) ;')
R('0xe5 @imm8 => "in " greg[$opsiz*8+0] "," hex($imm8) ;')
R('0xe6 @imm8 => "out " hex($imm8) ",al" ;')
R('0xe7 @imm8 => "out " hex($imm8) "," greg[$opsiz*8+0] ;')
R('0xec => "in al,dx" ;')
R('0xed => "in " greg[$opsiz*8+0] ",dx" ;')
R('0xee => "out dx,al" ;')
R('0xef => "out dx," greg[$opsiz*8+0] ;')
R('0x6c => "insb" ;')
R('0x6d => "ins" ;')
R('0x6e => "outsb" ;')
R('0x6f => "outs" ;')

# ----- x87 FPU (D8-DF): each is a group (digit picks op; mem + st(i) forms) --
C("x87 FPU D8-DF: /digit groups; mem forms carry size suffix, reg forms take st(i)")
X87 = {
 0xd8:(["fadd","fmul","fcom","fcomp","fsub","fsubr","fdiv","fdivr"],"d"),
 0xd9:(["fld","fxch","fst","fstp","fldenv","fldcw","fnstenv","fnstcw"],"d"),
 0xda:(["fcmovb","fcmove","fcmovbe","fcmovu","fx_a","fucompp","fx_a","fx_a"],"d"),
 0xdb:(["fild","fisttp","fist","fistp","fx_b","fld","fx_b","fstp"],"d"),
 0xdc:(["fadd","fmul","fcom","fcomp","fsub","fsubr","fdiv","fdivr"],"q"),
 0xdd:(["fld","fisttp","fst","fstp","frstor","fx_d","fnsave","fnstsw"],"q"),
 0xde:(["faddp","fmulp","fcomp","fcompp","fsubrp","fsubp","fdivrp","fdivp"],"w"),
 0xdf:(["fild","fisttp","fist","fistp","fbld","fild","fbstp","fistp"],"w"),
}
sfxmap={"b":1,"w":2,"d":4,"q":8,"t":10}
for op,(mn,sz) in X87.items():
  for i,m in enumerate(mn):
    d=format(i,"03b")
    R('0x%02x 11 %s rrr => "%s st(" dec($r) ")" ;' % (op,d,m))
    R('0x%02x @addr(%d)   => "%s.%s " $addr ;' % (op,i,m,sz))

# ----- MMX / SSE (one literal rule per opcode; 66/F3/F2 ride the prefix run) -
C("MMX/SSE: opsiz selects mm/xmm where paired; mandatory-prefix variants replay")
def sse2(op,m,xmm_only=False):
  reg = "ssereg[8+$%s]" if xmm_only else "ssereg[$opsiz*8+$%s]"
  R('0x0f 0x%02x 11 ggg rrr => "%s " %s "," %s ;' % (op,m,reg%"g",reg%"r"))
  R('0x0f 0x%02x @addr      => "%s " %s "," $addr ;' % (op,m,reg%"g"))
# MMX-paired (mm without 66, xmm with 66)
for (op,m) in [(0x6f,"movq"),(0xfc,"paddb"),(0xfd,"paddw"),(0xfe,"paddd"),(0xf8,"psubb"),
               (0xdb,"pand"),(0xeb,"por"),(0xef,"pxor"),(0x74,"pcmpeqb"),
               (0x63,"packsswb"),(0x60,"punpcklbw"),
               # shift-by-mm/xmm family (the by-imm8 forms live under 0F 71/72/73)
               (0xd1,"psrlw"),(0xd2,"psrld"),(0xd3,"psrlq"),
               (0xe1,"psraw"),(0xe2,"psrad"),
               (0xf1,"psllw"),(0xf2,"pslld"),(0xf3,"psllq")]:
  sse2(op,m)
# SSE xmm-only
for (op,m) in [(0x28,"movaps"),(0x10,"movups"),(0x58,"addps"),(0x59,"mulps"),(0x5c,"subps"),
               (0x2f,"comiss"),(0x57,"xorps"),(0x54,"andps")]:
  sse2(op,m,xmm_only=True)
# 0F 2E ucomiss/ucomisd by opsiz (kept as a sel pair)
R('0x0f 0x2e 11 ggg rrr => uc[$opsiz] " " ssereg[8+$g] "," ssereg[8+$r] ;')
R('0x0f 0x2e @addr      => uc[$opsiz] " " ssereg[8+$g] "," $addr ;')
# movd: reg is mm/xmm, r/m is greg
R('0x0f 0x6e 11 ggg rrr => "movd " ssereg[$opsiz*8+$g] "," greg[$r] ;')
R('0x0f 0x6e @addr      => "movd " ssereg[$opsiz*8+$g] "," $addr ;')
R('0x0f 0x7e 11 ggg rrr => "movd " greg[$r] "," ssereg[$opsiz*8+$g] ;')
R('0x0f 0x7e @addr      => "movd " $addr "," ssereg[$opsiz*8+$g] ;')
# cvt scalar/packed conversions (variant via prefix replay)
R('0x0f 0x2a 11 ggg rrr => "cvtpi2ps " ssereg[8+$g] "," greg[$r] ;')
R('0x0f 0x2a @addr      => "cvtpi2ps " ssereg[8+$g] "," $addr ;')
R('0x0f 0x2c 11 ggg rrr => "cvttps2pi " greg[$g] "," ssereg[8+$r] ;')
R('0x0f 0x2c @addr      => "cvttps2pi " greg[$g] "," $addr ;')
R('0x0f 0x2d 11 ggg rrr => "cvtps2pi " greg[$g] "," ssereg[8+$r] ;')
R('0x0f 0x2d @addr      => "cvtps2pi " greg[$g] "," $addr ;')
# pshufw/pshufd: 0F 70 with imm8
R('0x0f 0x70 11 ggg rrr @imm8 => "pshufw " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;')
R('0x0f 0x70 @addr      @imm8 => "pshufw " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;')
# psllw group 0F 71 /6 imm8
# psllw/psraw/etc. (0F 71/72/73 shift-by-imm groups) are added in the extras below
R('0x0f 0x77 => "emms" ;')
# 0F AE fences/clflush (two-byte group): /5 lfence /6 mfence /7 sfence(reg) clflush(mem)
R('0x0f 0xae 11 100 rrr => "ptwrite " greg[$opsiz*8+$r] ;')   # 0F AE /4 reg = ptwrite
R('0x0f 0xae 11 101 rrr => "lfence" ;')
R('0x0f 0xae 11 110 rrr => "mfence" ;')
R('0x0f 0xae 11 111 rrr => "sfence" ;')
R('0x0f 0xae @addr(7)   => "clflush" sfx[1] " " $addr ;')
# 0F 18 prefetch group (two-byte)
R('0x0f 0x18 @addr(0) => "prefetchnta" sfx[1] " " $addr ;')
R('0x0f 0x18 @addr(1) => "prefetcht0" sfx[1] " " $addr ;')
R('0x0f 0x18 @addr(2) => "prefetcht1" sfx[1] " " $addr ;')
R('0x0f 0x18 @addr(3) => "prefetcht2" sfx[1] " " $addr ;')
R('0x0f 0x18 @addr(4) => "nop " $addr ;')                 # 0F 18 /4 = nop, /5 = nop18
R('0x0f 0x18 @addr(5) => "nop18 " $addr ;')
R('0x0f 0x18 @addr(6) => "prefetchit1" sfx[1] " " $addr ;')
R('0x0f 0x18 @addr(7) => "prefetchit0" sfx[1] " " $addr ;')
for d in range(8):                                          # 0F 18 reg form = nop18
  R('0x0f 0x18 11 %s rrr => "nop18 " greg[$r] ;' % format(d, "03b"))

# ----- atomics + system (ported from corpus1.p) ----------------------------
C("xadd/cmpxchg/cmpxchg8b, lar/lsl, clts/invd/wbinvd, descriptor-table groups")
R('0x0f 0xc0 11 ggg rrr => "xadd " rgb[$r] "," rgb[$g] ;')
R('0x0f 0xc0 @addr      => "xadd" sfx[1] " " $addr "," rgb[$g] ;')
R('0x0f 0xc1 11 ggg rrr => "xadd " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;')
R('0x0f 0xc1 @addr      => "xadd " $addr "," greg[$opsiz*8+$g] ;')
R('0x0f 0xb0 11 ggg rrr => "cmpxchg " rgb[$r] "," rgb[$g] ;')
R('0x0f 0xb0 @addr      => "cmpxchg" sfx[1] " " $addr "," rgb[$g] ;')
R('0x0f 0xb1 11 ggg rrr => "cmpxchg " greg[$opsiz*8+$r] "," greg[$opsiz*8+$g] ;')
R('0x0f 0xb1 @addr      => "cmpxchg " $addr "," greg[$opsiz*8+$g] ;')
R('0x0f 0xc7 @addr(1)   => "cmpxchg8b " $addr ;')        # 0F C7 /1 (two-byte group)
R('0x0f 0xc7 @addr(3)   => "xrstors " $addr ;')          # 0F C7 /3-/7 mem
R('0x0f 0xc7 @addr(4)   => "xsavec " $addr ;')
R('0x0f 0xc7 @addr(5)   => "xsaves " $addr ;')
R('0x0f 0xc7 @addr(6)   => "vmptrld " $addr ;')
R('0x0f 0xc7 @addr(7)   => "vmptrst " $addr ;')
R('0x0f 0xc7 11 110 rrr => "rdrand " greg[$opsiz*8+$r] ;')   # 0F C7 /6 reg
R('0x0f 0xc7 11 111 rrr => "rdseed " greg[$opsiz*8+$r] ;')   # 0F C7 /7 reg
R('0x0f 0x37 => "getsec" ;')
R('0x0f 0x02 11 ggg rrr => "lar " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;')
R('0x0f 0x02 @addr      => "lar " greg[$opsiz*8+$g] "," $addr ;')
R('0x0f 0x03 11 ggg rrr => "lsl " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;')
R('0x0f 0x03 @addr      => "lsl " greg[$opsiz*8+$g] "," $addr ;')
R('0x0f 0x06 => "clts" ;')
R('0x0f 0x08 => "invd" ;')
R('0x0f 0x09 => "wbinvd" ;')
for i,m in enumerate(["sldt","str","lldt","ltr","verr","verw"]):   # 0F 00 group
  R('0x0f 0x00 11 %s rrr => "%s " greg[$opsiz*8+$r] ;' % (format(i,"03b"),m))
  R('0x0f 0x00 @addr(%d)   => "%s" sfx[2] " " $addr ;' % (i,m))
for i,m in [(0,"sgdt"),(1,"sidt"),(2,"lgdt"),(3,"lidt"),(4,"smsw"),(6,"lmsw"),(7,"invlpg")]:
  R('0x0f 0x01 @addr(%d)   => "%s " $addr ;' % (i,m))      # 0F 01 group (mem forms)
R('0x0f 0x01 @addr(5)   => "rstorssp " $addr ;')         # 0F 01 /5 mem
R('0x0f 0x01 11 100 rrr => "smsw " greg[$opsiz*8+$r] ;')
R('0x0f 0x01 11 110 rrr => "lmsw " greg[8+$r] ;')

# ----- more MMX / SSE packed ops (ported from corpus1.p) --------------------
C("more MMX/SSE packed integer ops (opsiz selects mm/xmm)")
for (op,m) in [(0xec,"paddsb"),(0xed,"paddsw"),(0xdc,"paddusb"),(0xdd,"paddusw"),
               (0xf9,"psubw"),(0xfa,"psubd"),(0xe8,"psubsb"),(0xe9,"psubsw"),
               (0xd8,"psubusb"),(0xd9,"psubusw"),(0xd5,"pmullw"),(0xe5,"pmulhw"),
               (0xf5,"pmaddwd"),(0xdf,"pandn"),(0x64,"pcmpgtb"),(0x65,"pcmpgtw"),
               (0x66,"pcmpgtd"),(0x67,"packuswb"),(0x6b,"packssdw"),(0x68,"punpckhbw"),
               (0x69,"punpckhwd"),(0x6a,"punpckhdq"),(0x61,"punpcklwd"),(0x62,"punpckldq"),
               (0xf4,"pmuludq"),(0x75,"pcmpeqw"),(0x76,"pcmpeqd"),(0xd4,"paddq"),(0xfb,"psubq"),
               (0xe0,"pavgb"),(0xe3,"pavgw"),(0xda,"pminub"),(0xde,"pmaxub"),
               (0xea,"pminsw"),(0xee,"pmaxsw"),(0xe4,"pmulhuw"),(0xf6,"psadbw")]:
  sse2(op,m)
R('0x0f 0xd7 11 ggg rrr => "pmovmskb " greg[$g] "," ssereg[$opsiz*8+$r] ;')
# pinsrw/pextrw two-byte forms (0F C4 / 0F C5); the 0F 3A 14-16 forms are added later
R('0x0f 0xc4 11 ggg rrr @imm8 => "pinsrw " ssereg[$opsiz*8+$g] "," greg[$r] "," hex($imm8) ;')
R('0x0f 0xc4 @addr      @imm8 => "pinsrw " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;')
R('0x0f 0xc5 11 ggg rrr @imm8 => "pextrw " greg[$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;')
C("packed shift-by-imm groups 0F 71/72/73")
for i,m in [(2,"psrlw"),(4,"psraw"),(6,"psllw")]:
  R('0x0f 0x71 11 %s rrr @imm8 => "%s " ssereg[$opsiz*8+$r] "," hex($imm8) ;' % (format(i,"03b"),m))
for i,m in [(2,"psrld"),(4,"psrad"),(6,"pslld")]:
  R('0x0f 0x72 11 %s rrr @imm8 => "%s " ssereg[$opsiz*8+$r] "," hex($imm8) ;' % (format(i,"03b"),m))
for i,m in [(2,"psrlq"),(3,"psrldq"),(6,"psllq"),(7,"pslldq")]:
  R('0x0f 0x73 11 %s rrr @imm8 => "%s " ssereg[$opsiz*8+$r] "," hex($imm8) ;' % (format(i,"03b"),m))
C("more SSE float ops (xmm; mandatory-prefix variants replay)")
for (op,m) in [(0x51,"sqrtps"),(0x52,"rsqrtps"),(0x53,"rcpps"),(0x55,"andnps"),
               (0x56,"orps"),(0x5d,"minps"),(0x5e,"divps"),(0x5f,"maxps"),
               (0x14,"unpcklps"),(0x15,"unpckhps"),(0x12,"movlps"),(0x16,"movhps")]:
  sse2(op,m,xmm_only=True)
R('0x0f 0x50 11 ggg rrr => "movmskps " greg[$g] "," ssereg[8+$r] ;')
R('0x0f 0xc2 11 ggg rrr @imm8 => "cmpps " ssereg[8+$g] "," ssereg[8+$r] "," hex($imm8) ;')
R('0x0f 0xc2 @addr      @imm8 => "cmpps " ssereg[8+$g] "," $addr "," hex($imm8) ;')
R('0x0f 0xc6 11 ggg rrr @imm8 => "shufps " ssereg[8+$g] "," ssereg[8+$r] "," hex($imm8) ;')
R('0x0f 0xc6 @addr      @imm8 => "shufps " ssereg[8+$g] "," $addr "," hex($imm8) ;')

# ----- SSE3 + more SSE2 (packed cvt, hadd/addsub, non-temporal, punpckqdq) ---
C("SSE3 + packed cvt + punpckqdq (66/F2/F3 prefix variants replay)")
for op,m in [(0xd0,"addsubps"),(0x7c,"haddps"),(0x7d,"hsubps"),
             (0x5a,"cvtps2pd"),(0x5b,"cvtdq2ps"),(0xe6,"cvtpd2dq"),
             (0x6c,"punpcklqdq"),(0x6d,"punpckhqdq")]:
  sse2(op,m,xmm_only=True)
R('0x0f 0xf0 @addr => "lddqu " ssereg[8+$g] "," $addr ;')           # lddqu (F2), mem-only
C("non-temporal stores + masked move")
R('0x0f 0x2b @addr => "movntps " $addr "," ssereg[8+$g] ;')         # movntps/movntpd
R('0x0f 0xe7 @addr => "movntq " $addr "," ssereg[$opsiz*8+$g] ;')   # movntq / movntdq
R('0x0f 0xc3 @addr => "movnti " $addr "," greg[$g] ;')
R('0x0f 0xf7 11 ggg rrr => "maskmovq " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;')
C("0F AE memory digits: fxsave/fxrstor, ldmxcsr/stmxcsr, xsave family")
for i,m in [(0,"fxsave"),(1,"fxrstor"),(2,"ldmxcsr"),(3,"stmxcsr"),
            (4,"xsave"),(5,"xrstor"),(6,"xsaveopt")]:
  R('0x0f 0xae @addr(%d) => "%s " $addr ;' % (i,m))

# ----- three-byte maps: 0F 38 (SSSE3/SSE4/AES/SHA) and 0F 3A (imm8 SSE4/AES) ---
# uniform packed forms: reg field and r/m are both mm/xmm (bank chosen by opsiz,
# i.e. the 66 prefix, which is replayed). pextr/pinsr (mixed greg<->xmm) follow.
def t38(op,m):                                           # 0F 38 NN  (no immediate)
  R('0x0f 0x38 0x%02x 11 ggg rrr => "%s " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;' % (op,m))
  R('0x0f 0x38 0x%02x @addr      => "%s " ssereg[$opsiz*8+$g] "," $addr ;' % (op,m))
def t3a(op,m):                                           # 0F 3A NN  (imm8)
  R('0x0f 0x3a 0x%02x 11 ggg rrr @imm8 => "%s " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;' % (op,m))
  R('0x0f 0x3a 0x%02x @addr      @imm8 => "%s " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;' % (op,m))

C("0F 38: SSSE3 packed (pshufb/phadd/phsub/psign/pabs/pmaddubsw/pmulhrsw)")
for op,m in [(0x00,"pshufb"),(0x01,"phaddw"),(0x02,"phaddd"),(0x03,"phaddsw"),
             (0x04,"pmaddubsw"),(0x05,"phsubw"),(0x06,"phsubd"),(0x07,"phsubsw"),
             (0x08,"psignb"),(0x09,"psignw"),(0x0a,"psignd"),(0x0b,"pmulhrsw"),
             (0x1c,"pabsb"),(0x1d,"pabsw"),(0x1e,"pabsd")]:
  t38(op,m)
C("0F 38: SSE4.1/4.2 packed (blend/ptest/pmovsx/pmovzx/pmul/pcmp/pmin/pmax)")
for op,m in [(0x10,"pblendvb"),(0x14,"blendvps"),(0x15,"blendvpd"),(0x17,"ptest"),
             (0x20,"pmovsxbw"),(0x21,"pmovsxbd"),(0x22,"pmovsxbq"),(0x23,"pmovsxwd"),
             (0x24,"pmovsxwq"),(0x25,"pmovsxdq"),(0x28,"pmuldq"),(0x29,"pcmpeqq"),
             (0x2b,"packusdw"),(0x30,"pmovzxbw"),(0x31,"pmovzxbd"),(0x32,"pmovzxbq"),
             (0x33,"pmovzxwd"),(0x34,"pmovzxwq"),(0x35,"pmovzxdq"),(0x37,"pcmpgtq"),
             (0x38,"pminsb"),(0x39,"pminsd"),(0x3a,"pminuw"),(0x3b,"pminud"),
             (0x3c,"pmaxsb"),(0x3d,"pmaxsd"),(0x3e,"pmaxuw"),(0x3f,"pmaxud"),
             (0x40,"pmulld"),(0x41,"phminposuw")]:
  t38(op,m)
R('0x0f 0x38 0x2a @addr => "movntdqa " ssereg[8+$g] "," $addr ;')   # load-only
C("0F 38: AES + SHA")
for op,m in [(0xdb,"aesimc"),(0xdc,"aesenc"),(0xdd,"aesenclast"),
             (0xde,"aesdec"),(0xdf,"aesdeclast"),(0xc8,"sha1nexte"),
             (0xc9,"sha1msg1"),(0xca,"sha1msg2"),(0xcb,"sha256rnds2"),
             (0xcc,"sha256msg1"),(0xcd,"sha256msg2")]:
  t38(op,m)

C("0F 3A: SSE4.1/4.2 imm8 (round/blend/dp/mpsadbw/pclmul/pcmpstr/palignr)")
for op,m in [(0x08,"roundps"),(0x09,"roundpd"),(0x0a,"roundss"),(0x0b,"roundsd"),
             (0x0c,"blendps"),(0x0d,"blendpd"),(0x0e,"pblendw"),(0x0f,"palignr"),
             (0x21,"insertps"),(0x40,"dpps"),(0x41,"dppd"),(0x42,"mpsadbw"),
             (0x44,"pclmulqdq"),(0x60,"pcmpestrm"),(0x61,"pcmpestri"),
             (0x62,"pcmpistrm"),(0x63,"pcmpistri"),(0xcc,"sha1rnds4"),
             (0xdf,"aeskeygenassist")]:
  t3a(op,m)
C("0F 3A: pextr/pinsr/extractps (mixed greg<->xmm, imm8)")
for op,m in [(0x14,"pextrb"),(0x15,"pextrw"),(0x16,"pextrd"),(0x17,"extractps")]:
  R('0x0f 0x3a 0x%02x 11 ggg rrr @imm8 => "%s " greg[$r] "," ssereg[$opsiz*8+$g] "," hex($imm8) ;' % (op,m))
  R('0x0f 0x3a 0x%02x @addr      @imm8 => "%s " $addr "," ssereg[$opsiz*8+$g] "," hex($imm8) ;' % (op,m))
for op,m in [(0x20,"pinsrb"),(0x22,"pinsrd")]:
  R('0x0f 0x3a 0x%02x 11 ggg rrr @imm8 => "%s " ssereg[$opsiz*8+$g] "," greg[$r] "," hex($imm8) ;' % (op,m))
  R('0x0f 0x3a 0x%02x @addr      @imm8 => "%s " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;' % (op,m))

# ----- system specials + popcnt + movbe/crc32 (ported from corpus1.p) --------
C("system: sysenter/sysexit/rsm/rdpmc + 0F 01 reg-form specials (round-trip by rm)")
R('0x0f 0x34 => "sysenter" ;')
R('0x0f 0x35 => "sysexit" ;')
R('0x0f 0xaa => "rsm" ;')
R('0x0f 0x33 => "rdpmc" ;')
R('0x0f 0x01 11 001 rrr => "monitor" ;')     # 0F 01 C8-CF: monitor/mwait/clac/stac (by rm)
R('0x0f 0x01 11 111 rrr => "rdtscp" ;')       # 0F 01 F8-FF: rdtscp/monitorx/mwaitx  (by rm)
C("popcnt (F3 0F B8); lzcnt/tzcnt round-trip via bsr/bsf + F3 replay")
R('0x0f 0xb8 11 ggg rrr => "popcnt " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;')
R('0x0f 0xb8 @addr      => "popcnt " greg[$opsiz*8+$g] "," $addr ;')
C("movbe + crc32 (0F 38 F0/F1). One mnemonic per opcode so reg+mem forms merge;")
C("the F2 prefix (crc32 vs movbe) and operand order are replayed, so bytes are exact.")
R('0x0f 0x38 0xf0 11 ggg rrr => "crc32 " greg[$opsiz*8+$g] "," rgb[$r] ;')        # crc32 r,r/m8
R('0x0f 0x38 0xf0 @addr      => "crc32 " greg[$opsiz*8+$g] "," $addr ;')          # movbe load / crc32 m8
R('0x0f 0x38 0xf1 11 ggg rrr => "crc32 " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;')
R('0x0f 0x38 0xf1 @addr      => "crc32 " greg[$opsiz*8+$g] "," $addr ;')          # movbe store / crc32 m

# ----- control/debug regs, multibyte nop, prefetch, 0F 01 specials, ud1, D6 ---
C("multi-byte nop (0F 1F, reg+mem) + prefetch group (0F 0D) + nop0d reg form")
R('0x0f 0x1f 11 ggg rrr => "nop " greg[$r] "," greg[$g] ;')
R('0x0f 0x1f @addr      => "nop " $addr ;')
for i,m in [(0,"prefetch"),(1,"prefetchw"),(2,"prefetchwt1"),(3,"prefetch3"),
            (4,"prefetch4"),(5,"prefetch5"),(6,"prefetch6"),(7,"prefetch7")]:
  R('0x0f 0x0d @addr(%d) => "%s " $addr ;' % (i,m))
for d in range(8):                                          # 0F 0D reg form = nop0d
  R('0x0f 0x0d 11 %s rrr => "nop0d " greg[$r] ;' % format(d, "03b"))
C("0F 01 reg-form specials (vmcall/xgetbv/vmrun/...; round-trip by rm)")
R('0x0f 0x01 11 000 rrr => "vmcall" ;')       # C0-C7
R('0x0f 0x01 11 010 rrr => "xgetbv" ;')       # D0-D7
R('0x0f 0x01 11 011 rrr => "vmrun" ;')        # D8-DF
R('0x0f 0x01 11 101 rrr => "rstorssp" ;')     # E8-EF
C("ud1 (0F B9), 0F D6 movq/movq2dq/movdq2q, int1 (F1)")
R('0x0f 0xb9 11 ggg rrr => "ud1 " greg[$g] "," greg[$r] ;')
R('0x0f 0xb9 @addr      => "ud1 " greg[$g] "," $addr ;')
R('0x0f 0xd6 11 ggg rrr => "movq " ssereg[8+$r] "," ssereg[8+$g] ;')
R('0x0f 0xd6 @addr      => "movq " $addr "," ssereg[8+$g] ;')
R('0xf1 => "int1" ;')

# ----- 3DNow! (0F 0F): suffix-opcode encoding ------------------------------
# 0F 0F modrm [addr] SUFFIX -- the trailing byte selects the op (pfadd/pavgusb
# /...). Like a mandatory prefix but positioned last: one rule replays every
# suffix byte-exactly, carrying it as a trailing imm8 (mm operands; no 66 form).
R('0x0f 0x0f 11 ggg rrr @imm8 => "_3dnow " ssereg[$g] "," ssereg[$r] "," hex($imm8) ;')
R('0x0f 0x0f @addr      @imm8 => "_3dnow " ssereg[$g] "," $addr "," hex($imm8) ;')

# ----- final coverage sweep from corpus1.p (legal non-VEX opcodes) -----------
C("SSE move STORES (reverse direction of 10/28/12/16/6F) + syscall/sysret/femms/salc")
R('0xd6 => "salc" ;')                                  # one-byte SALC (undocumented)
R('0x0f 0x05 => "syscall" ;')
R('0x0f 0x07 => "sysret" ;')
R('0x0f 0x0e => "femms" ;')
R('0x0f 0x11 11 ggg rrr => "movups " ssereg[8+$r] "," ssereg[8+$g] ;')   # store form
R('0x0f 0x11 @addr      => "movups " $addr "," ssereg[8+$g] ;')
R('0x0f 0x13 @addr      => "movlps " $addr "," ssereg[8+$g] ;')          # mem-only store
R('0x0f 0x17 @addr      => "movhps " $addr "," ssereg[8+$g] ;')          # mem-only store
R('0x0f 0x29 11 ggg rrr => "movaps " ssereg[8+$r] "," ssereg[8+$g] ;')
R('0x0f 0x29 @addr      => "movaps " $addr "," ssereg[8+$g] ;')
R('0x0f 0x7f 11 ggg rrr => "movdqa " ssereg[$opsiz*8+$r] "," ssereg[$opsiz*8+$g] ;')
R('0x0f 0x7f @addr      => "movdqa " $addr "," ssereg[$opsiz*8+$g] ;')
C("vmread/vmwrite (0F 78/79), ud0 (0F FF)")
R('0x0f 0x78 11 ggg rrr => "vmread " greg[$r] "," greg[$g] ;')
R('0x0f 0x78 @addr      => "vmread " $addr "," greg[$g] ;')
R('0x0f 0x79 11 ggg rrr => "vmwrite " greg[$g] "," greg[$r] ;')
R('0x0f 0x79 @addr      => "vmwrite " greg[$g] "," $addr ;')
R('0x0f 0xff 11 ggg rrr => "ud0 " greg[$opsiz*8+$g] "," greg[$opsiz*8+$r] ;')
R('0x0f 0xff @addr      => "ud0 " greg[$opsiz*8+$g] "," $addr ;')
C("reserved-nop / MPX bnd / CET space (0F 19-1E): corpus1.p mnemonics")
for op in (0x19, 0x1a, 0x1b, 0x1d):                          # distinct per-opcode placeholders
  R('0x0f 0x%02x 11 ggg rrr => "nop%02x " greg[$r] "," greg[$g] ;' % (op, op))
  R('0x0f 0x%02x @addr      => "nop%02x " $addr ;' % (op, op))
R('0x0f 0x1c @addr(0) => "cldemote " $addr ;')              # 0F 1C /0 = cldemote
for d in range(1, 8):
  R('0x0f 0x1c @addr(%d) => "nop1c " $addr ;' % d)
for d in range(8):
  R('0x0f 0x1c 11 %s rrr => "nop1c " greg[$r] ;' % format(d, "03b"))
R('0x0f 0x1e 11 ggg rrr => "nop " greg[$r] "," greg[$g] ;') # 0F 1E generic = nop
R('0x0f 0x1e @addr      => "nop " $addr ;')
C("VIA PadLock crypto (0F A6/A7 reg-form groups; round-trip by rm)")
for i, m in [(0, "montmul"), (1, "xsha1"), (2, "xsha256")]:
  R('0x0f 0xa6 11 %s rrr => "%s" ;' % (format(i, "03b"), m))
for i, m in [(0, "xstore"), (1, "xcryptecb"), (2, "xcryptcbc"),
             (3, "xcryptctr"), (4, "xcryptcfb"), (5, "xcryptofb")]:
  R('0x0f 0xa7 11 %s rrr => "%s" ;' % (format(i, "03b"), m))
C("more 0F 38: invept/invvpid/invpcid, gf2p8mulb, adcx/adox, wrss, movdir, aadd, encodekey")
for op, m in [(0x80, "invept"), (0x81, "invvpid"), (0x82, "invpcid")]:
  R('0x0f 0x38 0x%02x @addr => "%s " greg[$g] "," $addr ;' % (op, m))
R('0x0f 0x38 0xcf 11 ggg rrr => "gf2p8mulb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] ;')
R('0x0f 0x38 0xcf @addr      => "gf2p8mulb " ssereg[$opsiz*8+$g] "," $addr ;')
R('0x0f 0x38 0xf6 11 ggg rrr => "adcx " greg[$g] "," greg[$r] ;')        # adcx/adox/wrss
R('0x0f 0x38 0xf6 @addr      => "adcx " greg[$g] "," $addr ;')
R('0x0f 0x38 0xf5 @addr      => "wrssd " $addr "," greg[$g] ;')
R('0x0f 0x38 0xf8 @addr      => "movdir64b " greg[$g] "," $addr ;')      # movdir64b/enqcmd(s)
R('0x0f 0x38 0xf9 @addr      => "movdiri " $addr "," greg[$g] ;')
R('0x0f 0x38 0xfc @addr      => "aadd " $addr "," greg[$g] ;')
R('0x0f 0x38 0xfa 11 ggg rrr => "encodekey128 " greg[$g] "," greg[$r] ;')
R('0x0f 0x38 0xfb 11 ggg rrr => "encodekey256 " greg[$g] "," greg[$r] ;')
for i, m in [(0, "aesencwide128kl"), (1, "aesdecwide128kl"),
             (2, "aesencwide256kl"), (3, "aesdecwide256kl")]:
  R('0x0f 0x38 0xd8 @addr(%d) => "%s " $addr ;' % (i, m))   # 0F 38 D8 group (three-byte)
C("more 0F 3A: gf2p8affine(inv)qb, hreset")
R('0x0f 0x3a 0xce 11 ggg rrr @imm8 => "gf2p8affineqb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;')
R('0x0f 0x3a 0xce @addr      @imm8 => "gf2p8affineqb " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;')
R('0x0f 0x3a 0xcf 11 ggg rrr @imm8 => "gf2p8affineinvqb " ssereg[$opsiz*8+$g] "," ssereg[$opsiz*8+$r] "," hex($imm8) ;')
R('0x0f 0x3a 0xcf @addr      @imm8 => "gf2p8affineinvqb " ssereg[$opsiz*8+$g] "," $addr "," hex($imm8) ;')
R('0x0f 0x3a 0xf0 11 ggg rrr @imm8 => "hreset " greg[$r] "," hex($imm8) ;')

print("\n".join(out))
