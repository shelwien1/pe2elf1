mov rax,rcx
mov eax,ebx
mov al,bpl
mov rcx,rdx
mov ecx,esp
mov cl,sil
mov rdx,rbx
mov edx,ebp
mov dl,dil
mov rbx,rsp
mov ebx,esi
mov bl,r8b
mov rsp,rbp
mov esp,edi
mov spl,r9b
mov rbp,rsi
mov ebp,r8d
mov bpl,r10b
mov rsi,rdi
mov esi,r9d
mov sil,r11b
mov rdi,r8
mov edi,r10d
mov dil,r12b
mov r8,r9
mov r8d,r11d
mov r8b,r13b
mov r9,r10
mov r9d,r12d
mov r9b,r14b
mov r10,r11
mov r10d,r13d
mov r10b,r15b
mov r11,r12
mov r11d,r14d
mov r11b,al
mov r12,r13
mov r12d,r15d
mov r12b,cl
mov r13,r14
mov r13d,eax
mov r13b,dl
mov r14,r15
mov r14d,ecx
mov r14b,bl
mov r15,rax
mov r15d,edx
mov r15b,spl
mov rax,[rax]
lea rcx,[rax+0x10]
lea rcx,[rax+0x12345678]
mov rax,[rbx]
lea rcx,[rbx+0x10]
lea rcx,[rbx+0x12345678]
mov rax,ss:[rsp]
lea rcx,ss:[rsp+0x10]
lea rcx,ss:[rsp+0x12345678]
disp8 mov rax,ss:[rbp+0x0]
lea rcx,ss:[rbp+0x10]
lea rcx,ss:[rbp+0x12345678]
mov rax,ss:[r12]
lea rcx,ss:[r12+0x10]
lea rcx,ss:[r12+0x12345678]
disp8 mov rax,ss:[r13+0x0]
lea rcx,ss:[r13+0x10]
lea rcx,ss:[r13+0x12345678]
mov rax,[rdi]
lea rcx,[rdi+0x10]
lea rcx,[rdi+0x12345678]
mov rax,[r9]
lea rcx,[r9+0x10]
lea rcx,[r9+0x12345678]
mov rdx,[rax+rcx*1]
mov rdx,[r8+rcx*1+0x40]
mov rdx,[rax+rcx*2]
mov rdx,[r8+rcx*2+0x40]
mov rdx,[rax+rcx*4]
mov rdx,[r8+rcx*4+0x40]
mov rdx,[rax+rcx*8]
mov rdx,[r8+rcx*8+0x40]
mov rdx,[rax+r10*1]
mov rdx,[r8+r10*1+0x40]
mov rdx,[rax+r10*2]
mov rdx,[r8+r10*2+0x40]
mov rdx,[rax+r10*4]
mov rdx,[r8+r10*4+0x40]
mov rdx,[rax+r10*8]
mov rdx,[r8+r10*8+0x40]
mov rdx,[rax+r15*1]
mov rdx,[r8+r15*1+0x40]
mov rdx,[rax+r15*2]
mov rdx,[r8+r15*2+0x40]
mov rdx,[rax+r15*4]
mov rdx,[r8+r15*4+0x40]
mov rdx,[rax+r15*8]
mov rdx,[r8+r15*8+0x40]
mov rax,[rip+0x1000]
mov rax,[0x12345678]
lea rax,[rcx*4+0x100]
add rax,rbx
add ecx,r8d
add al,r9b
add rax,0x10
add rcx,0x12345678
add.b [rdi],0x5
add [r8+0x8],rax
or rax,rbx
or ecx,r8d
or al,r9b
or rax,0x10
or rcx,0x12345678
or.b [rdi],0x5
or [r8+0x8],rax
adc rax,rbx
adc ecx,r8d
adc al,r9b
adc rax,0x10
adc rcx,0x12345678
adc.b [rdi],0x5
adc [r8+0x8],rax
sbb rax,rbx
sbb ecx,r8d
sbb al,r9b
sbb rax,0x10
sbb rcx,0x12345678
sbb.b [rdi],0x5
sbb [r8+0x8],rax
and rax,rbx
and ecx,r8d
and al,r9b
and rax,0x10
and rcx,0x12345678
and.b [rdi],0x5
and [r8+0x8],rax
sub rax,rbx
sub ecx,r8d
sub al,r9b
sub rax,0x10
sub rcx,0x12345678
sub.b [rdi],0x5
sub [r8+0x8],rax
xor rax,rbx
xor ecx,r8d
xor al,r9b
xor rax,0x10
xor rcx,0x12345678
xor.b [rdi],0x5
xor [r8+0x8],rax
cmp rax,rbx
cmp ecx,r8d
cmp al,r9b
cmp rax,0x10
cmp rcx,0x12345678
cmp.b [rdi],0x5
cmp [r8+0x8],rax
rol rax,1
rol ecx,cl
rol.q [rdi],0x4
ror rax,1
ror ecx,cl
ror.q [rdi],0x4
rcl rax,1
rcl ecx,cl
rcl.q [rdi],0x4
rcr rax,1
rcr ecx,cl
rcr.q [rdi],0x4
shl rax,1
shl ecx,cl
shl.q [rdi],0x4
shr rax,1
shr ecx,cl
shr.q [rdi],0x4
sar rax,1
sar ecx,cl
sar.q [rdi],0x4
not rax
not.d [rdi]
neg rax
neg.d [rdi]
mul rax
mul.d [rdi]
imul rax
imul.d [rdi]
div rax
div.d [rdi]
idiv rax
idiv.d [rdi]
inc rax
inc.d [rdi]
dec rax
dec.d [rdi]
long jo 0x2cb
seto al
seto r9b
cmovo rax,rcx
cmovo ecx,r10d
long jno 0x2e0
setno al
setno r9b
cmovno rax,rcx
cmovno ecx,r10d
long jb 0x2f5
setb al
setb r9b
cmovb rax,rcx
cmovb ecx,r10d
long jae 0x30a
setae al
setae r9b
cmovae rax,rcx
cmovae ecx,r10d
long je 0x31f
sete al
sete r9b
cmove rax,rcx
cmove ecx,r10d
long jne 0x334
setne al
setne r9b
cmovne rax,rcx
cmovne ecx,r10d
long jbe 0x349
setbe al
setbe r9b
cmovbe rax,rcx
cmovbe ecx,r10d
long ja 0x35e
seta al
seta r9b
cmova rax,rcx
cmova ecx,r10d
long js 0x373
sets al
sets r9b
cmovs rax,rcx
cmovs ecx,r10d
long jns 0x388
setns al
setns r9b
cmovns rax,rcx
cmovns ecx,r10d
long jp 0x39d
setp al
setp r9b
cmovp rax,rcx
cmovp ecx,r10d
long jnp 0x3b2
setnp al
setnp r9b
cmovnp rax,rcx
cmovnp ecx,r10d
long jl 0x3c7
setl al
setl r9b
cmovl rax,rcx
cmovl ecx,r10d
long jge 0x3dc
setge al
setge r9b
cmovge rax,rcx
cmovge ecx,r10d
long jle 0x3f1
setle al
setle r9b
cmovle rax,rcx
cmovle ecx,r10d
long jg 0x406
setg al
setg r9b
cmovg rax,rcx
cmovg ecx,r10d
movzx eax,bl
movzx rax,r9b
movzx eax,cx
movzx rcx,dx
movsx eax,bl
movsx.b rax,[rdi]
movsx rcx,ax
movsxd rax,ecx
push rax
push rcx
push rdx
push rbx
push rsp
push rbp
push rsi
push rdi
push r8
push r9
push r10
push r11
push r12
push r13
push r14
push r15
pop rax
pop rcx
pop rdx
pop rbx
pop rsp
pop rbp
pop rsi
pop rdi
pop r8
pop r9
pop r10
pop r11
pop r12
pop r13
pop r14
pop r15
push 0x10
push 0x12345678
call rax
call.q [rbx]
jmp r11
jmp.q [rcx]
ret
leave
int3
syscall
cpuid
rdtsc
imul rax,rbx
imul ecx,edx
imul rax,rbx,0x10
imul rcx,[rdi],0x100
bt rax,rcx
bts [rdi],rax
btr ebx,0x4
btc rdx,0x20
bsf rax,rbx
bsr ecx,edx
rep popcnt rax,rbx
rep tzcnt ecx,edx
rep lzcnt rax,rbx
bswap rax
bswap r12d
xadd rcx,rdx
cmpxchg rbx,rax
cmpxchg16b [rdi]
movabs rax,0x1122334455667788
movabs r15,0xfedcba9876543210
movaps xmm0,xmm1
movups xmm8,[rdi]
movsd xmm0,xmm1
movss.d xmm2,[rax]
opsiz movdqa xmm1,xmm15
rep movdqu xmm8,[rsi]
addps xmm0,xmm1
addss xmm2,xmm8
addsd xmm3,[rdi]
addpd xmm4,xmm5
subps xmm0,xmm1
subss xmm2,xmm8
subsd xmm3,[rdi]
subpd xmm4,xmm5
mulps xmm0,xmm1
mulss xmm2,xmm8
mulsd xmm3,[rdi]
mulpd xmm4,xmm5
divps xmm0,xmm1
divss xmm2,xmm8
divsd xmm3,[rdi]
divpd xmm4,xmm5
minps xmm0,xmm1
minss xmm2,xmm8
minsd xmm3,[rdi]
minpd xmm4,xmm5
maxps xmm0,xmm1
maxss xmm2,xmm8
maxsd xmm3,[rdi]
maxpd xmm4,xmm5
sqrtss xmm0,xmm1
andps xmm0,xmm1
xorpd xmm2,xmm3
ucomiss xmm0,xmm1
comisd xmm2,xmm3
cvtsi2sd xmm0,rax
cvtsi2ss xmm1,ecx
cvttsd2si rax,xmm0
cvtsd2si ecx,xmm1
movd xmm0,eax
movd ecx,xmm1
movq xmm0,rax
movq rdx,xmm2
paddd xmm0,xmm1
pxor xmm8,xmm9
pand xmm0,xmm1
pcmpeqb xmm2,xmm3
pslld xmm0,0x4
psrlq xmm1,0x8
opsiz pshufd xmm0,xmm1,0x1b
pshufb xmm0,xmm1
pmuldq xmm2,xmm3
pcmpgtq xmm8,xmm9
opsiz aesenc xmm0,xmm1
roundsd xmm0,xmm1,0x2
palignr xmm0,xmm1,0x4
opsiz pextrd eax,xmm0,0x1
opsiz pinsrq xmm0,rax,0x0
repnz crc32 rax,rbx
movbe ecx,[rdi]
adcx rax,rbx
fadd st(0),st(1)
fld.q [rax]
fstp.q [rdi]
fmulp st(1),st(0)
fild.d [rcx]
fsqrt
fldz
fucomi st(0),st(2)
movsb
stosq
lodsd
cmpsb
scasq
cdqe
cqo
cwde
rep movsb
repnz scasb
lock add [rdi],rax
vaddps xmm0,xmm1,xmm2
vaddps ymm0,ymm1,ymm2
vaddps xmm8,xmm9,xmm10
vaddpd xmm0,xmm1,xmm2
vaddss xmm0,xmm1,xmm2
vaddsd xmm3,xmm4,xmm5
vaddps xmm0,xmm1,[rax]
vaddps ymm0,ymm1,[r8+rcx*4]
vaddps xmm8,xmm9,[rdi]
vmovaps xmm0,xmm1
vmovaps ymm8,ymm9
vmovaps xmm0,[rax]
vmovaps [rdi],ymm0
vmovaps xmm15,[r8]
vaddps xmm0,xmm1,xmm2
vmulpd ymm0,ymm1,ymm2
vsubss xmm0,xmm1,xmm2
vdivsd xmm3,xmm4,[rax]
vminps ymm8,ymm9,ymm10
vmaxpd xmm0,xmm1,[r8+rdx*8]
vandps xmm0,xmm1,xmm2
vxorps ymm0,ymm0,ymm0
vorpd xmm5,xmm6,xmm7
vmovaps xmm0,xmm1
vmovaps ymm8,[rdi]
vmovapd [rsi],ymm0
vmovups xmm0,[rax]
vmovupd ymm1,ymm2
vmovdqa ymm0,ymm1
vmovdqu xmm8,ss:[rsp]
vmovdqu [rdi],ymm15
vpaddd xmm0,xmm1,xmm2
vpaddq ymm0,ymm1,ymm2
vpsubb xmm0,xmm1,[rax]
vpand ymm8,ymm9,ymm10
vpor xmm0,xmm1,xmm2
vpxor ymm0,ymm0,ymm0
vpcmpeqb xmm0,xmm1,xmm2
vpcmpgtd ymm0,ymm1,ymm2
vpunpcklbw xmm0,xmm1,xmm2
vpmullw ymm0,ymm1,ymm2
vcmpps xmm0,xmm1,xmm2,0x0
vcmpsd xmm0,xmm1,xmm2,0x2
vshufps ymm0,ymm1,ymm2,0x1b
vzeroupper
vzeroall
