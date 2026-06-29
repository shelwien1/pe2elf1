/* prog64b.c -- integer-only x86-64 that exercises the instructions added to
 * corpus64.p beyond the basic ALU set: the cmpxchg/xadd atomics, the bsf/bsr/
 * tzcnt/lzcnt/popcnt bit scans, bswap, and (via -fcf-protection) the endbr64
 * CET prologue. Built -mgeneral-regs-only so no SSE/AVX appears in .text, which
 * keeps every byte inside the integer subset the 64-bit corpus now covers. */
typedef unsigned long u64;
typedef unsigned int  u32;

u32 atomic_cas(u32 *p, u32 want, u32 set){ return __sync_val_compare_and_swap(p, want, set); }
u64 atomic_add(u64 *p, u64 d){ return __sync_fetch_and_add(p, d); }
int atomic_xchg(int *p, int v){ return __sync_lock_test_and_set(p, v); }

int   ctz32(u32 x){ return __builtin_ctz(x); }
int   clz32(u32 x){ return __builtin_clz(x); }
int   ctz64(u64 x){ return __builtin_ctzll(x); }
int   pop64(u64 x){ return __builtin_popcountll(x); }
u64   bswap64(u64 x){ return __builtin_bswap64(x); }
u32   bswap32(u32 x){ return __builtin_bswap32(x); }

/* a plain bit test the compiler lowers to bt/shr, plus shifts and mul */
int   bittest(const u32 *p, int i){ return (p[i >> 5] >> (i & 31)) & 1u; }
u64   mix(u64 a, u64 b){ return (a << 13) ^ (b >> 7) ^ (a * 0x9E3779B97F4A7C15ull); }

/* recursion + loop so the build has calls, jcc, setcc, cmov, movzx/movsx */
long  collatz(u64 n){ long s = 0; while (n > 1){ n = (n & 1) ? 3*n+1 : n>>1; s++; } return s; }
int   imax(int a, int b){ return a > b ? a : b; }
