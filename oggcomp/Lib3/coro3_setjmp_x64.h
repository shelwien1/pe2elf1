
// __builtin_setjmp/__builtin_longjmp do not validate the stack frame the way
// glibc's setjmp/longjmp do (the latter aborts with "longjmp causes
// uninitialized stack frame" when the coroutine restores a stale stack).
// They also avoid a gcc -O2/-Ofast miscompile of the previous inline-asm
// implementation where the optimizer would short-circuit the saved/restored
// state and let do_process complete after a single coro_call iteration.

typedef void* m_jmp_buf[5];

#undef jmp_buf
#undef longjmp
#undef setjmp
#define jmp_buf m_jmp_buf
#define longjmp __builtin_longjmp
#define setjmp  __builtin_setjmp
