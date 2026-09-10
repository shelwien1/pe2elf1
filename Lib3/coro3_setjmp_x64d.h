
struct my_jmpbuf {
  qword rip,rsp;
};

#undef ASM
#define ASM __asm__ volatile

__attribute__((returns_twice, used))
INLINE static int my_setjmp( my_jmpbuf* regs ) {
  int r;
  ASM ("\
   leaq 0f(%%rip), %%rax; \
   movq %%rax, 0(%1);     \
   movq %%rsp, 8(%1);     \
   xorl %%eax, %%eax;     \
0:;                        \
  " : "=a"(r) : "b"(regs) : "%rcx","%rdx","%rsi","%rdi","%rbp","%r8","%r9","%r10","%r11","%r12","%r13","%r14","%r15",
#ifdef __AVX2__
"ymm0","ymm1","ymm2","ymm3","ymm4","ymm5","ymm6","ymm7","ymm8","ymm9","ymm10","ymm11","ymm12","ymm13","ymm14","ymm15","memory"
#else
"xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7","xmm8","xmm9","xmm10","xmm11","xmm12","xmm13","xmm14","xmm15","memory"
#endif
  );
  return r;
}

__attribute__((noreturn, used))
INLINE static void my_jmp( my_jmpbuf* regs, int ) {
  ASM ("\
  movq %0, %%rsp; \
  jmp *%1;        \
  " :  : "r"(regs->rsp),"r"(regs->rip),"a"(1) : "memory" );
  __assume(0);
}


#undef jmp_buf
#undef longjmp
#undef setjmp 

typedef my_jmpbuf m_jmp_buf[1];
#define jmp_buf m_jmp_buf
#define longjmp my_jmp
#define setjmp  my_setjmp

