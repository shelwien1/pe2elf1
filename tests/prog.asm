endbr64
lea rax,[rdi-0x1]
test rdi,rdi
jle 0x30
mov edx,0x1
xor esi,esi
disp8 nop.d [rax+0x0]
mov rcx,rdx
sub rax,0x1
add rdx,rsi
mov rsi,rcx
cmp rax,-0x1
jne 0x18
mov rax,rcx
ret
nop
xor ecx,ecx
mov rax,rcx
ret
opsiz segcs disp32 nop.w cs:[rax+rax*1+0x0]
endbr64
test esi,esi
jle 0x90
movsxd rdx,esi
pxor xmm0,xmm0
lea rax,[rdi+rdx*8]
and edx,0x1
je 0x68
addsd xmm0,[rdi]
add rdi,0x8
cmp rdi,rax
je 0x7a
nop.d [rax]
addsd xmm0,[rdi]
add rdi,0x10
addsd xmm0,[rdi-0x8]
cmp rdi,rax
jne 0x68
pxor xmm1,xmm1
cvtsi2sd xmm1,esi
divsd xmm0,xmm1
ret
disp32 nop.w [rax+rax*1+0x0]
pxor xmm1,xmm1
pxor xmm0,xmm0
cvtsi2sd xmm1,esi
divsd xmm0,xmm1
ret
opsiz opsiz disp32 nop.w cs:[rax+rax*1+0x0]
disp8 nop.d [rax+0x0]
endbr64
test esi,esi
jle 0xd8
movsxd rsi,esi
xor eax,eax
lea rdx,[rdi+rsi*4]
disp32 nop.d [rax+0x0]
add eax,[rdi]
add rdi,0x4
cmp rdi,rdx
jne 0xc8
ret
disp8 nop.d [rax+0x0]
xor eax,eax
ret
disp8 nop.d [rax+rax*1+0x0]
endbr64
long jmp 0xe9
disp32 nop.d [rax+0x0]
endbr64
movabs rax,0xff51afd7ed558ccd
mov rdx,rdi
shr rdx,0x21
xor rdx,rdi
imul rdx,rax
mov rax,rdx
shr rax,0x21
xor rax,rdx
ret
