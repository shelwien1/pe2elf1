/* System-call shim only. VBN's compressor, archive logic and CRT remain x86 bytes. */
/* gcc -O2 -shared -fPIC -o libvbnshim.so shim.c */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <signal.h>
#include <ucontext.h>
#include <dirent.h>
#include <utime.h>
#include <strings.h>
#endif

#define MEMSIZE 0x4000000u
#define LIMIT 0x3800000u
typedef struct {uint32_t a,b,c,d,si,di,flags;} Registers;
static uint32_t base,heap,dta;
static char root[4096],cwd[512];
static int tracing;
#ifdef _WIN32
static HANDLE handles[256],finder=INVALID_HANDLE_VALUE;
static WIN32_FIND_DATAA found;
#else
static int handles[256];
static DIR *finder;
static struct dirent **findlist;
static int findcount,findindex;
static char finddir[4096],findmask[256];
static unsigned findattr;
#endif
static void die(const char *msg){fprintf(stderr,"VBN shim: %s\n",msg);fflush(stderr);exit(127);}
static void *ptr(uint32_t a,size_t n){if(a<base || a-base>MEMSIZE || n>MEMSIZE-(a-base))die("guest address outside image");return (void *)(uintptr_t)a;}
static void put32(uint32_t a,uint32_t n){memcpy(ptr(a,4),&n,4);}
static void put16(unsigned char *p,uint16_t n){p[0]=(unsigned char)n;p[1]=(unsigned char)(n>>8);}
static const char *string(uint32_t a){const char *p=ptr(a,1);size_t n=MEMSIZE-(a-base);if(!memchr(p,0,n))die("unterminated guest string");return p;}
static int dospath(const char *input,char *output,size_t cap){
 char temp[1024],rel[1024];const char *s=input;size_t n;
 if(s[0] && s[1]==':')s+=2;
 if(strlen(s)>=sizeof(temp))return 0;
 strcpy(temp,s);for(char *p=temp;*p;p++)if(*p=='\\')*p='/';
 if(temp[0]=='/')snprintf(rel,sizeof(rel),"%s",temp+1);
 else if(!cwd[0])snprintf(rel,sizeof(rel),"%s",temp);
 else if(strlen(cwd)+strlen(temp)+2<sizeof(rel)){strcpy(rel,cwd);strcat(rel,"/");strcat(rel,temp);}
 else return 0;
 char norm[1024]="",*save=NULL,*part=strtok_r(rel,"/",&save);
 while(part){
  if(strcmp(part,"..")==0){char *p=strrchr(norm,'/');if(p)*p=0;else if(norm[0])norm[0]=0;else return 0;}
  else if(strcmp(part,".") && part[0]){n=strlen(norm);if(n+strlen(part)+2>=sizeof(norm))return 0;if(n)strcat(norm,"/");strcat(norm,part);}
  part=strtok_r(NULL,"/",&save);
 }
 if(strlen(root)+strlen(norm)+2>=cap)return 0;
 strcpy(output,root);strcat(output,"/");strcat(output,norm);
#ifndef _WIN32
 /* DOS lookup ignores case, while Linux filenames do not. Resolve each existing component. */
 char resolved[4096];strcpy(resolved,root);char *state=NULL,*part2=strtok_r(norm,"/",&state);
 while(part2){DIR *dir=opendir(resolved);char actual[256];snprintf(actual,sizeof(actual),"%s",part2);
  if(dir){struct dirent *entry;while((entry=readdir(dir)))if(!strcasecmp(entry->d_name,part2)){snprintf(actual,sizeof(actual),"%s",entry->d_name);break;}closedir(dir);}
  if(strlen(resolved)+strlen(actual)+2>=sizeof(resolved))return 0;
  strcat(resolved,"/");strcat(resolved,actual);part2=strtok_r(NULL,"/",&state);
 }if(strlen(resolved)>=cap)return 0;strcpy(output,resolved);
#endif
 return 1;
}
static int guestpath(uint32_t p,char *out){
 if(!dospath(string(p),out,4096))return 0;
 size_t vb=strlen(out);if(vb>4&&(!strcmp(out+vb-4,".vbb")||!strcmp(out+vb-4,".VBB")))strcpy(out+vb-4,".vbn");
 size_t n=strlen(out);if(n>4 && (!strcmp(out+n-4,".acp")||!strcmp(out+n-4,".ACP"))){
#ifdef _WIN32
  if(GetFileAttributesA(out)==INVALID_FILE_ATTRIBUTES){char alt[4096];strcpy(alt,out);strcpy(alt+n-4,".vbp");if(GetFileAttributesA(alt)!=INVALID_FILE_ATTRIBUTES)strcpy(out,alt);}
#else
  if(access(out,F_OK)!=0){char alt[4096];strcpy(alt,out);strcpy(alt+n-4,".vbp");if(access(alt,F_OK)!=0)strcpy(alt+n-4,".VBP");if(access(alt,F_OK)==0)strcpy(out,alt);}
#endif
 }return 1;
}
static void failed(Registers *r,unsigned err){r->a=err;r->flags|=1;}
static unsigned errorcode(void){
#ifdef _WIN32
 DWORD e=GetLastError();return e==ERROR_FILE_NOT_FOUND?2:e==ERROR_PATH_NOT_FOUND?3:e==ERROR_INVALID_HANDLE?6:5;
#else
 return errno==ENOENT?2:errno==ENOTDIR?3:errno==EBADF?6:5;
#endif
}
#ifndef _WIN32
static uint16_t packdate(struct tm *t){int y=t->tm_year+1900;if(y<1980)y=1980;if(y>2107)y=2107;return (uint16_t)(((y-1980)<<9)|((t->tm_mon+1)<<5)|t->tm_mday);}
static uint16_t packtime(struct tm *t){return (uint16_t)((t->tm_hour<<11)|(t->tm_min<<5)|(t->tm_sec/2));}
#endif
static void close_find(void){
#ifdef _WIN32
 if(finder!=INVALID_HANDLE_VALUE){FindClose(finder);finder=INVALID_HANDLE_VALUE;}
#else
 if(finder){closedir(finder);finder=NULL;}
 if(findlist){for(int i=0;i<findcount;i++)free(findlist[i]);free(findlist);findlist=NULL;findcount=findindex=0;}
#endif
}
#ifndef _WIN32
static int wildmatch(const char *p,const char *s){
 if(strcmp(p,"*.*")==0)p="*";
 for(;*p;p++,s++){
  if(*p=='*'){p++;if(!*p)return 1;for(;*s;s++)if(wildmatch(p,s))return 1;return wildmatch(p,s);}
  if(*p=='?'){if(!*s)return 0;continue;}
  unsigned a=(unsigned char)*p,b=(unsigned char)*s;if(a>='a'&&a<='z')a-=32;if(b>='a'&&b<='z')b-=32;if(a!=b)return 0;
 }return !*s;
}
#endif
static void find_next(Registers *r,int first,const char *pattern){
 unsigned char buf[43]={0};uint64_t size;uint16_t date,tim;const char *name;unsigned attr;
 if(first){close_find();
#ifdef _WIN32
  finder=FindFirstFileA(pattern,&found);if(finder==INVALID_HANDLE_VALUE){failed(r,18);return;}
#else
  snprintf(finddir,sizeof(finddir),"%s",pattern);char *p=strrchr(finddir,'/');if(!p){failed(r,3);return;}snprintf(findmask,sizeof(findmask),"%s",p+1);*p=0;
  findcount=scandir(finddir,&findlist,NULL,alphasort);findindex=0;findattr=r->c;if(findcount<0){findlist=NULL;findcount=0;failed(r,18);return;}
#endif
 }
 for(;;){
#ifdef _WIN32
  if(!first && !FindNextFileA(finder,&found)){failed(r,18);return;}first=0;
  name=found.cFileName;attr=(found.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)?16:32;
  size=((uint64_t)found.nFileSizeHigh<<32)|found.nFileSizeLow;
  FILETIME local;if(!FileTimeToLocalFileTime(&found.ftLastWriteTime,&local)||!FileTimeToDosDateTime(&local,&date,&tim)){date=33;tim=0;}
#else
  if(findindex>=findcount){failed(r,18);return;}struct dirent *e=findlist[findindex++];name=e->d_name;
  if(!wildmatch(findmask,name))continue;
  char path[4352];snprintf(path,sizeof(path),"%s/%s",finddir,name);struct stat st;if(stat(path,&st)<0)continue;
  attr=S_ISDIR(st.st_mode)?16:32;size=S_ISDIR(st.st_mode)?0:(uint64_t)st.st_size;
  struct tm t;localtime_r(&st.st_mtime,&t);date=packdate(&t);tim=packtime(&t);
  if((attr&16) && !(findattr&16))continue;
#endif
  if(!strcmp(name,".") || !strcmp(name,".."))continue;
  /* Original VBN uses 13-byte DOS names. Fail visibly instead of truncating names. */
  if(strlen(name)>12){fprintf(stderr,"VBN shim: DOS name exceeds 12 bytes: %s\n",name);failed(r,3);return;}
  break;
 }
 buf[21]=(unsigned char)attr;put16(buf+22,tim);put16(buf+24,date);uint32_t sz=(uint32_t)size;memcpy(buf+26,&sz,4);strcpy((char *)buf+30,name);
 memcpy(ptr(dta,43),buf,43);r->a=0;
}
static void dispatch(Registers *r){
 unsigned ax=r->a&65535,ah=(ax>>8),al=ax&255,h=r->b&65535;char path[4096],path2[4096];
 r->flags&=~1u;if(tracing)fprintf(stderr,"DOS ax=%04x bx=%08x cx=%08x dx=%08x\n",ax,r->b,r->c,r->d);
 if(ax==0x350c){uint32_t n=r->c;if(n>base+LIMIT-heap){failed(r,8);return;}r->a=heap;heap+=n;r->d=base+0xec96f0;return;}
 switch(ah){
 case 0x4c:exit(al);
 case 0x19:r->a=2;return;
 case 0x0e:r->a=3;return;
 case 0x30:r->a=5;return;
 case 0x1a:dta=r->d;return;
 case 0x0b:r->a=0;return;
 case 0x01:case 0x07:case 0x08:{int ch=getchar();r->a=ch==EOF?13:(unsigned char)ch;return;}
 case 0x09:{const char *p=ptr(r->d,1);size_t max=MEMSIZE-(r->d-base),n=0;while(n<max&&p[n]!='$')n++;fwrite(p,1,n,stdout);fflush(stdout);return;}
 case 0x2c:case 0x2a:{
#ifdef _WIN32
  SYSTEMTIME t;GetLocalTime(&t);if(ah==0x2c){r->c=(t.wHour<<8)|t.wMinute;r->d=(t.wSecond<<8)|(t.wMilliseconds/10);}else{r->c=t.wYear;r->d=(t.wMonth<<8)|t.wDay;r->a=t.wDayOfWeek;}
#else
  time_t now=time(NULL);struct tm *t=localtime(&now);if(ah==0x2c){r->c=(t->tm_hour<<8)|t->tm_min;r->d=t->tm_sec<<8;}else{r->c=t->tm_year+1900;r->d=((t->tm_mon+1)<<8)|t->tm_mday;r->a=t->tm_wday;}
#endif
  return;}
 case 0x36:r->a=8;r->b=65535;r->c=512;r->d=65535;return;
 case 0x47:{size_t n=strlen(cwd);char *p=ptr(r->si,n+1);memcpy(p,cwd,n+1);for(size_t i=0;i<n;i++)if(p[i]=='/')p[i]='\\';r->a=0;return;}
 case 0x3b:{if(!guestpath(r->d,path)){failed(r,3);return;}
#ifdef _WIN32
  DWORD at=GetFileAttributesA(path);if(at==INVALID_FILE_ATTRIBUTES || !(at&FILE_ATTRIBUTE_DIRECTORY)){failed(r,3);return;}
#else
  struct stat st;if(stat(path,&st)<0 || !S_ISDIR(st.st_mode)){failed(r,3);return;}
#endif
  const char *rel=path+strlen(root)+1;if(strlen(rel)>=sizeof(cwd)){failed(r,3);return;}strcpy(cwd,rel);r->a=0;return;}
 case 0x4e:if(!guestpath(r->d,path)){failed(r,3);return;}find_next(r,1,path);return;
 case 0x4f:find_next(r,0,NULL);return;
 case 0x3c:case 0x3d:{if(!guestpath(r->d,path)){failed(r,3);return;}if(tracing)fprintf(stderr,"file: %s\n",path);unsigned slot;for(slot=5;slot<256;slot++){
#ifdef _WIN32
   if(!handles[slot])break;
#else
   if(handles[slot]<0)break;
#endif
  }if(slot==256){failed(r,4);return;}
#ifdef _WIN32
  DWORD access=ah==0x3c?GENERIC_READ|GENERIC_WRITE:((al&3)==0?GENERIC_READ:(al&3)==1?GENERIC_WRITE:GENERIC_READ|GENERIC_WRITE);
  HANDLE fd=CreateFileA(path,access,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,ah==0x3c?CREATE_ALWAYS:OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
  if(fd==INVALID_HANDLE_VALUE){failed(r,errorcode());return;}handles[slot]=fd;
#else
  int flags=ah==0x3c?O_CREAT|O_TRUNC|O_RDWR:(al&3)==0?O_RDONLY:(al&3)==1?O_WRONLY:O_RDWR;
  int fd=open(path,flags,0666);if(fd<0){failed(r,errorcode());return;}handles[slot]=fd;
#endif
  r->a=slot;return;}
 case 0x3e:if(h>4 && h<256){
#ifdef _WIN32
  if(handles[h]){CloseHandle(handles[h]);handles[h]=NULL;}
#else
  if(handles[h]>=0){close(handles[h]);handles[h]=-1;}
#endif
 }r->a=0;return;
 case 0x3f:case 0x40:{if(h>=256){failed(r,6);return;}void *p=ptr(r->d,r->c);
#ifdef _WIN32
  DWORD n=0;BOOL ok;
  if(ah==0x3f)ok=ReadFile(handles[h],p,r->c,&n,NULL);
  else if(!r->c)ok=SetEndOfFile(handles[h]);else ok=WriteFile(handles[h],p,r->c,&n,NULL);
  if(!ok){failed(r,errorcode());return;}r->a=n;
#else
  ssize_t n;if(ah==0x3f)n=read(handles[h],p,r->c);else if(!r->c)n=ftruncate(handles[h],lseek(handles[h],0,SEEK_CUR));else n=write(handles[h],p,r->c);
  if(n<0){failed(r,errorcode());return;}r->a=(uint32_t)n;
#endif
  return;}
 case 0x42:{if(h>=256 || al>2){failed(r,6);return;}int32_t off=(int32_t)(((r->c&65535)<<16)|(r->d&65535));uint64_t n;
#ifdef _WIN32
  LARGE_INTEGER pos,ret;pos.QuadPart=off;if(!SetFilePointerEx(handles[h],pos,&ret,al)){failed(r,errorcode());return;}n=ret.QuadPart;
#else
  off_t pos=lseek(handles[h],off,al);if(pos<0){failed(r,errorcode());return;}n=(uint64_t)pos;
#endif
  r->a=(uint32_t)n&65535;r->d=(uint32_t)(n>>16)&65535;return;}
 case 0x44:r->d=h<3?0x80:0;r->a=0;return;
 case 0x43:{if(!guestpath(r->d,path)){failed(r,3);return;}
#ifdef _WIN32
  DWORD attr=GetFileAttributesA(path);if(attr==INVALID_FILE_ATTRIBUTES){failed(r,errorcode());return;}
  if(al==0)r->c=(attr&FILE_ATTRIBUTE_DIRECTORY)?16:32;
  else {DWORD keep=attr&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT);if(!SetFileAttributesA(path,keep|(r->c&39))){failed(r,errorcode());return;}}
#else
  struct stat st;if(stat(path,&st)<0){failed(r,errorcode());return;}if(al==0)r->c=S_ISDIR(st.st_mode)?16:32;
  else if(chmod(path,(st.st_mode&0777)|S_IWUSR)<0){failed(r,errorcode());return;}
#endif
  r->a=0;return;}
 case 0x39:case 0x3a:case 0x41:case 0x56:{if(!guestpath(r->d,path)){failed(r,3);return;}if(ah==0x56&&!guestpath(r->di,path2)){failed(r,3);return;}
  if(ah==0x56){size_t a=strlen(path),b=strlen(path2);if(a>4&&b>4&&(!strcmp(path+a-4,".vbp")||!strcmp(path+a-4,".VBP"))&&(!strcmp(path2+b-4,".vbb")||!strcmp(path2+b-4,".VBB")))strcpy(path2+b-4,".vbn");}
#ifdef _WIN32
  BOOL ok=ah==0x39?CreateDirectoryA(path,NULL):ah==0x3a?RemoveDirectoryA(path):ah==0x41?DeleteFileA(path):MoveFileA(path,path2);
  if(!ok){failed(r,errorcode());return;}
#else
  int result=ah==0x39?mkdir(path,0777):ah==0x3a?rmdir(path):ah==0x41?unlink(path):rename(path,path2);
  if(result<0){failed(r,errorcode());return;}
#endif
  r->a=0;return;}
 case 0x57:{if(h>=256){failed(r,6);return;}
#ifdef _WIN32
  FILETIME ft,local;
  if(al==0){WORD date,tim;if(!GetFileTime(handles[h],NULL,NULL,&ft)||!FileTimeToLocalFileTime(&ft,&local)||!FileTimeToDosDateTime(&local,&date,&tim)){failed(r,errorcode());return;}r->c=tim;r->d=date;}
  else {if(!DosDateTimeToFileTime((WORD)r->d,(WORD)r->c,&local)||!LocalFileTimeToFileTime(&local,&ft)||!SetFileTime(handles[h],NULL,NULL,&ft)){failed(r,errorcode());return;}}
#else
  if(al==0){struct stat st;if(fstat(handles[h],&st)<0){failed(r,errorcode());return;}struct tm t;localtime_r(&st.st_mtime,&t);r->c=packtime(&t);r->d=packdate(&t);}
  else {struct tm t={0};t.tm_year=((r->d>>9)&127)+80;t.tm_mon=((r->d>>5)&15)-1;t.tm_mday=r->d&31;t.tm_hour=(r->c>>11)&31;t.tm_min=(r->c>>5)&63;t.tm_sec=(r->c&31)*2;t.tm_isdst=-1;time_t tm=mktime(&t);struct timespec ts[2]={{.tv_nsec=UTIME_OMIT},{.tv_sec=tm,.tv_nsec=0}};if(futimens(handles[h],ts)<0){failed(r,errorcode());return;}}
#endif
  r->a=0;return;}
 default:fprintf(stderr,"VBN shim: unsupported DOS/extender service AX=%04x\n",ax);die("unimplemented system service");
 }
}
#ifdef _WIN32
__declspec(dllexport) __attribute__((force_align_arg_pointer)) void __cdecl vbn_interrupt(uint32_t *frame){
 Registers r={frame[7],frame[4],frame[6],frame[5],frame[1],frame[0],frame[8]};dispatch(&r);
 frame[7]=r.a;frame[4]=r.b;frame[6]=r.c;frame[5]=r.d;frame[1]=r.si;frame[0]=r.di;frame[8]=r.flags;
}
#else
static void handler(int sig,siginfo_t *info,void *context){
 (void)info;ucontext_t *c=context;greg_t *g=c->uc_mcontext.gregs;uint32_t pc=(uint32_t)g[REG_RIP];
 if(sig!=SIGILL || pc<base || pc-base>=MEMSIZE || *(uint16_t *)(uintptr_t)pc!=0x0b0f){fprintf(stderr,"VBN shim: signal %d at %08x (offset %x), SP=%llx, SI=%llx DI=%llx CX=%llx\n",sig,pc,pc-base,(unsigned long long)g[REG_RSP],(unsigned long long)g[REG_RSI],(unsigned long long)g[REG_RDI],(unsigned long long)g[REG_RCX]);_exit(126);}
 Registers r={(uint32_t)g[REG_RAX],(uint32_t)g[REG_RBX],(uint32_t)g[REG_RCX],(uint32_t)g[REG_RDX],(uint32_t)g[REG_RSI],(uint32_t)g[REG_RDI],(uint32_t)g[REG_EFL]};dispatch(&r);
 g[REG_RAX]=r.a;g[REG_RBX]=r.b;g[REG_RCX]=r.c;g[REG_RDX]=r.d;g[REG_RSI]=r.si;g[REG_RDI]=r.di;g[REG_EFL]=r.flags;g[REG_RIP]=pc+2;
}
#endif
static void initialize(uint32_t image,int argc,const char **argv){
 base=image;heap=base+0x1000000;dta=base+0xcd58;tracing=getenv("VBN_SHIM_TRACE")!=NULL;cwd[0]=0;
#ifdef _WIN32
 if(!GetCurrentDirectoryA(sizeof(root),root))die("GetCurrentDirectory failed");
 handles[0]=GetStdHandle(STD_INPUT_HANDLE);handles[1]=GetStdHandle(STD_OUTPUT_HANDLE);handles[2]=GetStdHandle(STD_ERROR_HANDLE);
#else
 if(!getcwd(root,sizeof(root)))die("getcwd failed");
 for(int i=0;i<256;i++)handles[i]=-1;
 handles[0]=0;handles[1]=1;handles[2]=2;
 stack_t ss={.ss_sp=malloc(1024*1024),.ss_size=1024*1024};if(!ss.ss_sp||sigaltstack(&ss,NULL))die("signal stack initialization failed");
 struct sigaction sa={0};sa.sa_sigaction=handler;sa.sa_flags=SA_SIGINFO|SA_ONSTACK;sigemptyset(&sa.sa_mask);
 if(sigaction(SIGILL,&sa,NULL)||sigaction(SIGSEGV,&sa,NULL)||sigaction(SIGBUS,&sa,NULL))die("cannot install signal handler");
#endif
 uint32_t av=base+0x3e00000,cur=av+4*((uint32_t)argc+1);
 for(int i=0;i<argc;i++){size_t n=strlen(argv[i])+1;if(cur-base+n>=0x3ff0000)die("arguments too large");put32(av+4*(uint32_t)i,cur);char *text=ptr(cur,n);memcpy(text,argv[i],n);if(i>=2)for(size_t j=0;j<n-1;j++)if(text[j]=='/')text[j]='\\';cur+=(uint32_t)n;}
 put32(av+4*(uint32_t)argc,0);put32(cur,0);put32(base+0xca88,cur);put32(base+0xca8c,(uint32_t)argc);put32(base+0xca90,av);put32(base+0xa090,base+0xec96f0);
 uint32_t pc=base+0x3ff0000;unsigned char *p=ptr(pc,128);unsigned n=0;
 uint32_t calls[]={0x70e2,0x704c};for(unsigned i=0;i<2;i++){p[n++]=0xe8;put32(pc+n,base+calls[i]-(pc+n+4));n+=4;}
 p[n++]=0x68;put32(pc+n,av);n+=4;p[n++]=0x68;put32(pc+n,(uint32_t)argc);n+=4;
 p[n++]=0xe8;put32(pc+n,base+0x2160-(pc+n+4));n+=4;p[n++]=0x50;
 p[n++]=0xe8;put32(pc+n,base+0x7172-(pc+n+4));n+=4;p[n++]=0x0f;p[n++]=0x0b;
}
#ifdef _WIN32
__declspec(dllexport) void __stdcall vbn_start(uint32_t image){
 char *line=_strdup(GetCommandLineA());const char *argv[256];int argc=0;char *src=line,*dst=line;
 while(*src && argc<255){
  while(*src==' '||*src=='\t')src++;
  if(!*src)break;
  argv[argc++]=dst;int quote=0;
  while(*src){
   if(*src=='\\'){unsigned n=0;while(*src=='\\'){n++;src++;}if(*src=='"'){for(unsigned j=0;j<n/2;j++)*dst++='\\';if(n&1){*dst++='"';src++;continue;}quote=!quote;src++;continue;}while(n--)*dst++='\\';continue;}
   if(*src=='"'){if(quote && src[1]=='"'){*dst++='"';src+=2;}else{quote=!quote;src++;}continue;}
   if(!quote&&(*src==' '||*src=='\t'))break;
   *dst++=*src++;
  }if(*src)src++;*dst++=0;
 }
 if(!argc){argv[0]="VBN.EXE";argc=1;}initialize(image,argc,argv);
 uint32_t pc=base+0x3ff0000,sp=base+0x3d00000;
 NT_TIB *tib;__asm__ volatile("movl %%fs:0x18,%0":"=r"(tib));tib->StackBase=(void *)(uintptr_t)sp;tib->StackLimit=(void *)(uintptr_t)(sp-0x100000);tib->ExceptionList=(struct _EXCEPTION_REGISTRATION_RECORD *)(uintptr_t)0xffffffffu;
 __asm__ volatile("movl %0,%%eax;movl %1,%%esp;jmp *%%eax"::"r"(pc),"r"(sp):"eax","memory");__builtin_unreachable();
}
#else
__attribute__((visibility("default"),noreturn)) void vbn_start(uint64_t *initial_stack,uint32_t image){
 int argc=(int)*initial_stack;const char **argv=(const char **)(initial_stack+1);initialize(image,argc,argv);
 uint32_t pc=base+0x3ff0000,sp=base+0x3d00000;
 __asm__ volatile("movl %0,%%eax;movl %1,%%edx;movw $0x2b,%%cx;movw %%cx,%%ds;movw %%cx,%%es;movl %%edx,%%esp;pushq $0x23;pushq %%rax;lretq"::"r"(pc),"r"(sp):"rax","rdx","rcx","memory");__builtin_unreachable();
}
#endif
