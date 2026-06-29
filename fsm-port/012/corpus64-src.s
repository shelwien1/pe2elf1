.intel_syntax noprefix
.text
mov rax,r15
mov r8d,eax
mov rcx,[rbp-0x8]
mov [rsp+0x10],rdx
mov rax,[rdi+rsi*2]
mov r10,[r11+r12*8+0x100]
mov eax,[rip+0x1234]
movzx eax,word ptr [rax]
movsx rax,byte ptr [rbx]
movsxd rax,dword ptr [rcx]
lea rdi,[rip+0x50]
lea rax,[rbx+rcx*4-0x4]
add qword ptr [rax],0x10
sub r8,r9
and rax,0xff
or byte ptr [rdi],0x1
xor r13d,r13d
cmp qword ptr [rbp-0x10],0x0
test al,0x1
test r14,r14
shl rax,cl
sar r9,0x3
shr dword ptr [rax],1
not rbx
neg r10
imul rax,rcx,0x10
imul rbx,qword ptr [rax]
push qword ptr [rax]
push 0x12345678
pop r15
inc r8
dec rdi
call qword ptr [rax+0x8]
jmp r11
ja 0x0
jne 0x0
sete al
cmovl eax,ecx
nop
leave
ret
