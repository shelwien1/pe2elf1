#include <stdio.h>
#include <string.h>
#define main parser_main
#include "parser.cpp"
#undef main

static int bad=0;
static void ck(bool c,const char* m,int a,int b){ if(!c){ if(++bad<=25) printf("FAIL %s got=%d exp=%d\n",m,a,b);} }

int main(){
  static const int sbo32[8]={0,0,0,0,7,7,0,0};
  static const int sbo16[8]={0,0,7,7,0,0,7,0};
  // 16-bit register NUMBERS now: bx=3 bp=5 si=6 di=7 ; none=15
  static const int rb[8]={3,3,5,5,6,7,5,3}, rx[8]={6,7,6,7,15,15,15,15};

  for(int modrm=0;modrm<256;++modrm){
    int mod=modrm>>6,reg=(modrm>>3)&7,rm=modrm&7;
    byte buf[8]={(byte)modrm,0x00,0x11,0x22,0x33,0x44,0,0};
    x86dec_t d={}; d.cap[VAR_ADRSIZ]=0; parse_addr(buf,sizeof buf,0,&d);
    ck(d.insn.op[0].index==reg,"32reg",d.insn.op[0].index,reg);
    if(mod==3) continue;
    if(rm==4){ ck(d.insn.mem_index==0,"sibidx",d.insn.mem_index,0); ck(d.insn.mem_base==0,"sibbase",d.insn.mem_base,0); }
    else if(rm==5&&mod==0){ ck(d.insn.mem_base==GREG_NONE,"d32b",d.insn.mem_base,GREG_NONE); ck(d.insn.disp==0x33221100,"d32v",d.insn.disp,0x33221100); }
    else { ck(d.insn.mem_base==rm,"32base",d.insn.mem_base,rm); ck(d.insn.mem_index==GREG_NONE,"32noidx",d.insn.mem_index,GREG_NONE);
           ck((int)d.insn.mem_seg==sbo32[rm],"32seg",d.insn.mem_seg,sbo32[rm]); }
  }
  for(int modrm=0;modrm<256;++modrm){
    int mod=modrm>>6,rm=modrm&7;
    byte buf[8]={(byte)modrm,0x34,0x12,0,0,0,0,0};
    x86dec_t d={}; d.cap[VAR_ADRSIZ]=1; parse_addr(buf,sizeof buf,0,&d);
    if(mod==3) continue;
    if(mod==0&&rm==6){ ck(d.insn.mem_base==GREG_NONE,"16d16b",d.insn.mem_base,GREG_NONE);
                       ck(d.insn.disp==0x1234,"16d16v",d.insn.disp,0x1234); }
    else { ck(d.insn.mem_base==rb[rm],"16base",d.insn.mem_base,rb[rm]);
           ck(d.insn.mem_index==rx[rm],"16idx",d.insn.mem_index,rx[rm]);
           ck((int)d.insn.mem_seg==sbo16[rm],"16seg",d.insn.mem_seg,sbo16[rm]); }
  }
  // *** the GREG_NONE/di fix: [bx+di] must render with di, distinct from [bx] ***
  { byte buf[2]={0x01,0}; x86dec_t d={}; d.cap[VAR_ADRSIZ]=1; parse_addr(buf,1,0,&d);
    ck(d.insn.mem_base==3,"bxdi_base",d.insn.mem_base,3);
    ck(d.insn.mem_index==7,"bxdi_index(di!=none)",d.insn.mem_index,7);
    char m[64]; fmt_mem(&d.insn,0,m);
    ck(strcmp(m,"[bx+di]")==0,"bxdi_render",strcmp(m,"[bx+di]"),0);
    printf("  [bx+di] renders as: %s\n", m); }
  { byte buf[2]={0x07,0}; x86dec_t d={}; d.cap[VAR_ADRSIZ]=1; parse_addr(buf,1,0,&d);   // [bx]
    char m[64]; fmt_mem(&d.insn,0,m);
    ck(strcmp(m,"[bx]")==0,"bx_render",strcmp(m,"[bx]"),0);
    printf("  [bx]    renders as: %s\n", m); }
  // prefix bookkeeping unchanged
  { byte buf[8]={0x66,0x67,0x66,0x2e,0x90,0,0,0}; x86dec_t d={};
    parse_prefixes(buf,sizeof buf,&d);
    ck(d.insn.pfx_off[VAR_OPSIZ]==2,"opsiz_lastoff",d.insn.pfx_off[VAR_OPSIZ],2);
    ck(d.insn.pfx_off[VAR_LOCK]==0xFF,"lock_none",d.insn.pfx_off[VAR_LOCK],0xFF); }
  printf(bad? "VALIDATION: %d failures\n":"VALIDATION: all OK (512 modrm + di-fix + prefix bookkeeping)\n", bad);
  return bad?1:0;
}
