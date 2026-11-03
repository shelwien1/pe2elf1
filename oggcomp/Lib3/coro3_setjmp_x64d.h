
struct my_jmpbuf {
  qword rip,rsp,rbp;
};

#undef ASM
#define ASM __asm__ volatile

// The buffer pointer is taken from memory into rbx inside the asm, so that
// rbx too is a clobber: my_jmp lands on the label below with every register
// as the jumper left it, and the code after the asm must reload everything
// from memory rather than keep anything -- the buffer pointer included -- in
// a register across it.
//
// Every register but one: rbp is saved and put back, not clobbered.  Where
// it is the frame pointer -- -fno-omit-frame-pointer, -O0, many a distro's
// CFLAGS -- clang cannot honour a clobber of it, and the code after the
// label reloads its locals through rbp: the jumper's rbp, as it was, so the
// first resume read another frame's locals and crashed.  Restored, rbp is
// this frame's again; where it is an ordinary register, the clobber keeps
// nothing in it across the asm and putting it back costs a move.
__attribute__((returns_twice, used))
INLINE static int my_setjmp( my_jmpbuf* regs ) {
  int r;
  ASM ("\
   movq %1, %%rbx;        \
   leaq 0f(%%rip), %%rax; \
   movq %%rax, 0(%%rbx);  \
   movq %%rsp, 8(%%rbx);  \
   movq %%rbp, 16(%%rbx); \
   xorl %%eax, %%eax;     \
0:;                        \
  " : "=a"(r) : "m"(regs) : "%rbx","%rcx","%rdx","%rsi","%rdi","%rbp","%r8","%r9","%r10","%r11","%r12","%r13","%r14","%r15",
#ifdef __AVX2__
"ymm0","ymm1","ymm2","ymm3","ymm4","ymm5","ymm6","ymm7","ymm8","ymm9","ymm10","ymm11","ymm12","ymm13","ymm14","ymm15","memory"
#else
"xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7","xmm8","xmm9","xmm10","xmm11","xmm12","xmm13","xmm14","xmm15","memory"
#endif
  );
  return r;
}

// The three values in named registers, none of them rbp: in a build where
// rbp is allocatable an "r" operand could be put there, and the move into
// rbp would overwrite the target before the jump.
__attribute__((noreturn, used))
INLINE static void my_jmp( my_jmpbuf* regs, int ) {
  ASM ("\
  movq %%rsi, %%rsp; \
  movq %%rcx, %%rbp; \
  jmp *%%rdi;        \
  " :  : "S"(regs->rsp),"D"(regs->rip),"c"(regs->rbp),"a"(1) : "memory" );
  __assume(0);
}


#undef jmp_buf
#undef longjmp
#undef setjmp 

typedef my_jmpbuf m_jmp_buf[1];
#define jmp_buf m_jmp_buf
#define longjmp my_jmp
#define setjmp  my_setjmp

