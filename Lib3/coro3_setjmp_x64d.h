
// Two variants, selected by CORO_FRAME_POINTER.
//
// Default (no define): saves only rip/rsp, as before.  This is correct only
// while the compiler keeps nothing live in %rbp across the setjmp edge, which
// is what -fomit-frame-pointer gives you -- and it is what gc.bat, gc.sh and
// the Makefile all build with.
//
// CORO_FRAME_POINTER: also saves and restores %rbp.  Needed when building with
// -fno-omit-frame-pointer, where %rbp is the frame base and the setjmp landing
// pad reloads its spills through it.  %rbp cannot simply be added to the
// clobber list instead: gcc rejects that outright ("bp cannot be used in 'asm'
// here") and clang accepts it and miscompiles.  The other callee-saved
// registers need no such treatment -- they are already in the clobber list, so
// the compiler keeps nothing live in them across the edge.
//
// Nothing distinguishes the two at compile time: no predefined macro differs
// between -fomit-frame-pointer and -fno-omit-frame-pointer on gcc or clang at
// any -O level, so the define has to be passed by whoever sets the flag.

#ifdef CORO_FRAME_POINTER

struct my_jmpbuf {
  qword rip,rsp,rbp;
};

#undef ASM
#define ASM __asm__ volatile

__attribute__((returns_twice, used))
INLINE static int my_setjmp( my_jmpbuf* regs ) {
  int r;
  ASM ("\
   leaq 0f(%%rip), %%rax; \
   movq %%rax,  0(%1);    \
   movq %%rsp,  8(%1);    \
   movq %%rbp, 16(%1);    \
   xorl %%eax, %%eax;     \
0:;                        \
  " : "=a"(r) : "b"(regs) : "%rcx","%rdx","%rsi","%rdi","%r8","%r9","%r10","%r11","%r12","%r13","%r14","%r15",
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
  // Addressed through %rdi so the base register is not one of the registers
  // being written; %rbp must be restored before the jump, not after.
  ASM ("\
  movq 16(%%rdi), %%rbp; \
  movq  8(%%rdi), %%rsp; \
  jmp  *0(%%rdi);        \
  " :  : "D"(regs),"a"(1) : "memory" );
  __assume(0);
}

#else   // default: -fomit-frame-pointer

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

#endif  // CORO_FRAME_POINTER


#undef jmp_buf
#undef longjmp
#undef setjmp 

typedef my_jmpbuf m_jmp_buf[1];
#define jmp_buf m_jmp_buf
#define longjmp my_jmp
#define setjmp  my_setjmp
