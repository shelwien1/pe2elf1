/* prog64e.c -- vectorizable float/int kernels gcc lowers to VEX/EVEX (AVX2 or
 * AVX-512 depending on -march): vmovup[sd]/vaddp[sd]/vmulp[sd]/vfmadd*, vmovdqu,
 * vpmulld, vbroadcast*, the masked AVX-512 forms, vzeroupper. Validates that the
 * VEX/EVEX path round-trips real AVX compiler output byte-exact (not just random
 * VEX bytes). Built with whatever -m flags the Makefile target passes. */
#include <stddef.h>
void vadd8(double *a, const double *b, int n){ for(int i=0;i<n;i++) a[i]+=b[i]; }
void saxpy(float *y, const float *x, float a, int n){ for(int i=0;i<n;i++) y[i]+=a*x[i]; }
int  idot(const int *a, const int *b, int n){ int s=0; for(int i=0;i<n;i++) s+=a[i]*b[i]; return s; }
float fmax_arr(const float *a, int n){ float m=a[0]; for(int i=1;i<n;i++) if(a[i]>m) m=a[i]; return m; }
void  scale2(float *a, float k, int n){ for(int i=0;i<n;i++) a[i]*=k; }
