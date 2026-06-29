/* prog64d.c -- integer array kernels that gcc -O3 auto-vectorizes to baseline
 * SSE2 packed ops (paddd/psubd/pmullw/pand/pxor/punpck/pcmpeq/movdqa/movdqu and
 * the shift groups). Validates the SSE2 packed-integer set on real vectorized
 * compiler output. Built for the SSE2 baseline (no -march), so no SSE3/SSE4. */
typedef unsigned char  u8;
typedef unsigned int   u32;

void vadd(int *a, const int *b, int n){ for (int i=0;i<n;i++) a[i] += b[i]; }
void vmul(short *a, const short *b, int n){ for (int i=0;i<n;i++) a[i] *= b[i]; }
void vxor(u32 *a, const u32 *b, int n){ for (int i=0;i<n;i++) a[i] ^= b[i]; }
void vand(u32 *a, u32 m, int n){ for (int i=0;i<n;i++) a[i] &= m; }
void vshl(u32 *a, int n){ for (int i=0;i<n;i++) a[i] <<= 3; }
void vshr(u32 *a, int n){ for (int i=0;i<n;i++) a[i] >>= 2; }
int  vsum(const int *a, int n){ int s=0; for (int i=0;i<n;i++) s+=a[i]; return s; }
void vclampb(u8 *a, int n){ for (int i=0;i<n;i++) if (a[i] > 200) a[i] = 200; }
void vcopy(u32 *d, const u32 *s, int n){ for (int i=0;i<n;i++) d[i] = s[i]; }
