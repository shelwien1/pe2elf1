int fib(int n){ if(n<2) return n; return fib(n-1)+fib(n-2); }
long sum(long *a, int n){ long s=0; for(int i=0;i<n;i++) s+=a[i]*3+i; return s; }
int cmp(const void*a,const void*b){ return *(const int*)a-*(const int*)b; }
unsigned gcd(unsigned a, unsigned b){ while(b){ unsigned t=b; b=a%b; a=t;} return a; }
