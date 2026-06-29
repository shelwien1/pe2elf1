#include <stdint.h>
#include <string.h>
long fib(long n){ long a=0,b=1; while(n-->0){long t=a+b;a=b;b=t;} return a; }
double favg(double*p,int n){ double s=0; for(int i=0;i<n;i++) s+=p[i]; return s/n; }
int sum(int*a,int n){ int s=0; for(int i=0;i<n;i++) s+=a[i]; return s; }
void cpy(char*d,const char*s,unsigned long n){ memcpy(d,s,n); }
uint64_t mix(uint64_t x){ x^=x>>33; x*=0xff51afd7ed558ccdULL; x^=x>>33; return x; }
