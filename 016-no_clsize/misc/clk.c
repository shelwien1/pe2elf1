/* The core clock, measured rather than believed. /proc/cpuinfo reports the
   base clock; a turboing core runs faster, and every cycle figure derived
   from the wrong one is wrong by that ratio -- which is how the encoder's
   model pass came out at 4.65 instructions per cycle on a 4-wide machine.

   Four dependent adds per iteration, one cycle each; the sub/jg run in
   parallel with them.

     gcc -O2 -o clk clk.c && ./clk                                        */

#include <stdio.h>
#include <time.h>

int main( void ) {
  long long n = 2000000000LL, x = 0;
  struct timespec a, b;
  clock_gettime( CLOCK_MONOTONIC, &a );
  __asm__ volatile(
    "1:\n\t"
    "addq $1, %[x]\n\t"
    "addq $1, %[x]\n\t"
    "addq $1, %[x]\n\t"
    "addq $1, %[x]\n\t"
    "subq $1, %[n]\n\t"
    "jg 1b\n\t"
    : [x]"+r"(x), [n]"+r"(n) :: "cc" );
  clock_gettime( CLOCK_MONOTONIC, &b );
  double s = (b.tv_sec-a.tv_sec) + 1e-9*(b.tv_nsec-a.tv_nsec);
  printf( "%.4f s for %lld dependent adds -> %.3f GHz\n",
          s, 4*2000000000LL, 4*2e9/s/1e9 );
  return 0;
}
