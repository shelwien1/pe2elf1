#include "x86dec.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
int main(){
  char line[1024]; long lineno=0, ok=0, ud=0, mis=0, lenmis=0;
  while(fgets(line,sizeof line,stdin)){
    lineno++;
    byte b[32]; size_t n=0; char*p=strtok(line," \t\r\n");
    while(p&&n<32){ b[n++]=(byte)strtol(p,0,16); p=strtok(0," \t\r\n"); }
    if(!n) continue;
    x86dec_t d={}; size_t l=decode_insn(b,n,&d);
    if(!l||d.insn.mnem==0xFFFF){ printf("L%ld UD ",lineno); for(size_t i=0;i<n;i++)printf("%02x",b[i]); printf("\n"); ud++; continue; }
    if(l!=n){ printf("L%ld LENMIS in=%zu dec=%zu ",lineno,n,l); for(size_t i=0;i<n;i++)printf("%02x",b[i]); printf("\n"); lenmis++; continue; }
    uint8_t out[32]; size_t m=encode_insn(&d.insn,out);
    if(m!=l||memcmp(b,out,l)){ printf("L%ld MISMATCH in=",lineno); for(size_t i=0;i<n;i++)printf("%02x",b[i]);
      printf(" out="); for(size_t i=0;i<m;i++)printf("%02x",out[i]); printf("\n"); mis++; continue; }
    ok++;
  }
  fprintf(stderr,"total=%ld  OK=%ld  UD=%ld  LENMIS=%ld  MISMATCH=%ld\n",ok+ud+lenmis+mis,ok,ud,lenmis,mis);
  return 0;
}
