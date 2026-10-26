
// __builtin_setjmp/__builtin_longjmp instead of the hand-rolled asm in
// coro3_setjmp_x32.h, for the same reason as coro3_setjmp_x64.h: from gcc 11
// on the optimizer no longer keeps the coroutine's live state where that asm
// block assumes it is, and the coroutine loses it after the first resume.

typedef void* m_jmp_buf[5];

#undef jmp_buf
#undef longjmp
#undef setjmp
#define jmp_buf m_jmp_buf
#define longjmp __builtin_longjmp
#define setjmp  __builtin_setjmp
