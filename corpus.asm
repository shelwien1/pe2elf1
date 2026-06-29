lea eax,[eax]
lea eax,[ecx]
lea eax,[edx]
lea eax,[ebx]
lea eax,[esi]
lea eax,[edi]
lea eax,[0x12345678]
lea eax,ss:[esp]
lea eax,ss:[ebp+0x0]
lea eax,[eax+0x10]
lea eax,ss:[ebp+0x10]
lea eax,ss:[esp+0x10]
lea eax,[eax+0x12345678]
lea eax,ss:[ebp+0x12345678]
lea eax,ss:[esp+0x12345678]
lea eax,[eax+ecx*1]
lea eax,[eax+ecx*2]
lea eax,[eax+ecx*4]
lea eax,[eax+ecx*8]
lea eax,[ecx*2+0x0]
lea eax,[ecx*4+0x1000]
lea eax,ss:[ebp+ecx*4+0x0]
lea eax,ss:[ebp+ecx*4+0x10]
lea eax,ss:[esp+ecx*4]
lea eax,ss:[esp+ecx*4+0x10]
lea eax,[eax+esi*8+0x12345678]
mov eax,[@0x1000]
mov ebx,[0x1000]
mov eax,es:[ebx]
mov eax,cs:[ebx]
mov eax,ss:[ebx]
mov eax,fs:[@0x30]
mov eax,gs:[@0x30]
mov eax,ss:[ebp+0x0]
mov eax,ds:[ebp+0x0]
mov eax,ss:[esp]
mov.b fs:[ecx+0x10],0x12
add ax,bx
mov ax,0x1234
add ax,0x1234
inc ax
push ax
xchg bx,ax
mov eax,ebx
mov [eax],ebx
mov ebx,[eax]
mov al,bl
mov eax,0x12345678
mov.d [eax],0x12345678
mov.b [eax],0x12
mov.w [eax],0x1234
mov eax,[@0x12345678]
mov [@0x12345678],eax
mov al,[@0x12345678]
mov es,eax
mov ax,es
movzx.b eax,[ebx]
movzx.w eax,[ebx]
movsx.b eax,[ebx]
movsx eax,bl
xchg ecx,eax
xchg [eax],ebx
bswap eax
push eax
push.d [eax]
push 0x12345678
push 0x12
pop eax
pop.d [eax]
push es
pop es
push fs
pop fs
pusha
popa
pushf
popf
lea ecx,[eax+ebx*2+0x40]
xlatb
add eax,ebx
add [eax],ebx
add ebx,[eax]
add al,0x10
add eax,0x10
add eax,0x12345678
add.d [eax],0x10
add.d [eax],0x12345678
adc eax,ebx
sub eax,ebx
sbb eax,ebx
cmp eax,ebx
cmp eax,0x12345678
inc eax
inc.d [eax]
dec ecx
neg eax
mul ebx
imul ebx
imul eax,ebx
imul eax,ebx,0x10
imul eax,[ebx],0x12345678
div ebx
idiv ebx
daa
das
aaa
aas
aam 0xa
aad 0xa
and eax,ebx
or eax,ebx
xor eax,ebx
not eax
test eax,ebx
test eax,0x12345678
test al,0x10
shl eax,1
shl eax,0x4
shl eax,cl
shr eax,cl
sar eax,0x4
rol eax,1
ror eax,cl
rcl eax,0x2
rcr eax,1
shld eax,ebx,0x4
shld eax,ebx,cl
shrd eax,ebx,0x4
bt eax,ebx
bt eax,0x4
bts [eax],ebx
btr eax,ebx
btc eax,0x4
bsf eax,ebx
bsr eax,ebx
sete al
setne bl
setl cl
setg dl
seta.b [eax]
cmove eax,ebx
cmovne eax,[ebx]
cmovl eax,ebx
jmp 0x1b8
jmp.d [eax]
jmp eax
call 0x1c8
call.d [eax]
call eax
je 0x1c8
jne 0x1b8
jl 0x1c8
jg 0x1b8
jecxz 0x1c8
loop 0x1b8
loope 0x1c8
loopne 0x1b8
ret
ret 0x10
retf
retf 0x8
int 0x80
int3
into
iret
leave
enter 0x10,0x0
bound.q eax,[ebx]
arpl ax,bx
les eax,[ebx]
lds eax,[ebx]
lfs eax,[ebx]
lgs eax,[ebx]
lss eax,[ebx]
movsb
movsw
movsd
stosb
stosd
lodsb
lodsd
cmpsb
scasb
rep movsb
rep stosd
rep cmpsb
repnz scasb
movsb
rep movsd
clc
stc
cmc
cld
std
cli
sti
lahf
sahf
nop
hlt
cpuid
rdtsc
cdq
cwde
cbw
cwd
fwait
leave
fld.d [eax]
fld.q [eax]
fld.t [eax]
fld st(1)
fst.d [eax]
fstp.q [eax]
fadd st(0),st(1)
fadd st(2),st(0)
faddp st(1),st(0)
fadd.d [eax]
fsub st(0),st(1)
fmul.q [eax]
fdiv st(0),st(3)
fild.d [eax]
fild.q [eax]
fistp.d [eax]
fchs
fabs
fsqrt
fsin
fcos
fldz
fld1
fldpi
fninit
fnstsw
fnstcw.w [eax]
fldcw.w [eax]
fcom st(1)
fcomp st(2)
fucom st(1)
fxch st(1)
movd mm0,eax
movd.d mm0,[eax]
movd eax,mm0
movq mm0,mm1
movq mm0,[eax]
paddb mm0,mm1
paddw mm0,mm1
paddd mm0,[eax]
psubb mm0,mm1
pand mm0,mm1
por mm0,mm1
pxor mm0,mm1
pcmpeqb mm0,mm1
packsswb mm0,mm1
punpcklbw mm0,mm1
psllw mm0,0x4
psrld mm0,mm1
emms
movaps xmm0,xmm1
movaps xmm0,[eax]
movups xmm0,[eax]
movss.d xmm0,[eax]
movsd.q xmm0,[eax]
movd xmm0,eax
movq xmm0,xmm1
addps xmm0,xmm1
addss xmm0,xmm1
addpd xmm0,xmm1
addsd xmm0,xmm1
mulps xmm0,xmm1
subss xmm0,xmm1
cvtsi2ss xmm0,eax
cvtss2si eax,xmm0
cvttsd2si eax,xmm0
comiss xmm0,xmm1
ucomisd xmm0,xmm1
pshufd xmm0,xmm1,0x4
paddb xmm0,xmm1
pxor xmm0,xmm1
xorps xmm0,xmm1
andpd xmm0,xmm1
in al,0x60
in ax,0x60
in eax,dx
out 0x60,al
out dx,eax
insb
outsb
rdmsr
wrmsr
ud2
clflush.b [eax]
prefetcht0.b [eax]
lfence
mfence
sfence
segcs opsiz opsiz opsiz opsiz opsiz opsiz opsiz opsiz opsiz opsiz opsiz opsiz opsiz movsb
alt add al,0x10
disp32 mov eax,[eax+0x0]
disp8 mov eax,[eax+0x0]
disp32 mov eax,ss:[esp+0x0]
mov eax,[0x12345678]
segcs nop
opsiz add al,0x10
segss segcs mov eax,cs:[eax]
mov eax,[bx]
mov eax,[bx+si]
mov eax,ss:[bp+0x10]
adrsiz mov eax,[0x1234]
jmp 0x8:0x12345678
call 0x8:0x12345678
mov eax,fs:[bx]
lock disp8 add.d [eax+0x0],0x1
