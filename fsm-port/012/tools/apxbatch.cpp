#include "x86dec.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
static void pr(const x86op_t*o,const x86insn_t*in){
  int os=in->opsize;
  switch(o->type){
    case T_XMM:printf("xmm%d",o->index);break; case T_YMM:printf("ymm%d",o->index);break;
    case T_ZMM:printf("zmm%d",o->index);break; case T_KREG:printf("k%d",o->index);break;
    case T_MASK:printf("{k%d}%s",o->index,in->zero?"{z}":"");break;
    case T_GPR:printf("%s",greg[(os==2?64:os==1?32:0)+o->index]);break;
    case T_GPR8:printf("%s",rgb[o->index]);break;
    case T_MEM:{int b=in->mem_base,x=in->mem_index;printf("[");
      if(in->rip)printf("rip");else if(b<32)printf("%s",greg[64+b]);
      if(x<32)printf("+%s*%d",greg[64+x],1<<in->mem_scale);
      if(in->disp)printf("+0x%x",(unsigned)in->disp);
      printf("]");}break;
    case T_IMM:printf("0x%llx",(unsigned long long)in->imm);break;
    default:printf("?%d",o->type);
  }
}
int main(){
  char line[512];
  while(fgets(line,sizeof line,stdin)){
    byte b[32]; size_t n=0;
    char*p=strtok(line," \t\r\n");
    while(p&&n<32){ b[n++]=(byte)strtol(p,0,16); p=strtok(0," \t\r\n"); }
    x86dec_t d={}; size_t l=decode_insn(b,n,&d);
    if(!l||d.insn.mnem==0xFFFF){ printf("0|(bad)\n"); continue; }
    printf("%zu|%s",l,mnem_tab[d.insn.mnem]);
    if(d.insn.dfv!=0xFF){ // APX CCMP/CTEST default-flags value {cf,zf,sf,of} (bit order 0..3)
      const char*fn[4]={"cf","zf","sf","of"}; int first=1; printf(" {dfv=");
      for(int b=0;b<4;b++) if(d.insn.dfv&(1<<b)){ printf(first?"%s":", %s",fn[b]); first=0; }
      printf("}");
    }
    for(int i=0;i<d.insn.n_ops;i++){printf(i?",":" ");pr(&d.insn.op[i],&d.insn);}
    printf("\n");
  }
  return 0;
}
