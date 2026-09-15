
enum{ N_Cells=1<<20, N_Nest=0x200 };

struct Cell {
  void* ptr;
  uint msk;
  uint val;
};

extern "C" uint trkptr;
extern "C" Cell trk[N_Cells];

uint trkptr = 0;
uint nesting = 0;

uint nest_trkptr[N_Nest];

alignas(64) Cell trk[N_Cells];

void NEST( void ) {
  nest_trkptr[nesting++] = trkptr;
}

void UNDO( void ) {
  uint i;
  uint k = nest_trkptr[--nesting];
  for( i=trkptr-1; int(i)>=int(k); i-- ) {
    Cell& c = trk[i];
    uint* p = (uint*)c.ptr;
    uint d = p[0] & (~c.msk);
    d |= c.val;
    p[0] = d;
  }
  trkptr = k;
}

// track1/track2/track4/track8 are the journaling stubs that track.pl inserts in front
// of every byte/word/dword/qword store in the compiled encode_sim():
//
//   push ARG            ; ARG = rcx (Win64) or rdi (SysV x86-64)
//   lea  ARG, <store address>
//   call trackN
//   pop  ARG
//
// Each stub is the equivalent of
//
//   void trackN( void* p ) {           // N = 1,2,4 (track8: two 4-byte cells)
//     Cell& c = trk[trkptr++];
//     c.ptr = p;
//     c.msk = (1ULL<<(8*N))-1;
//     c.val = *(uint*)p & c.msk;
//   }
//
// but written in assembly so that it provably preserves *everything*, EFLAGS
// included: the compiler is free to keep condition flags live across a store
// (e.g. cmp / mov [mem],reg / cmovcc), and a flag-preserving stub lets the
// instrumentation drop the pushf/popf pair, which was measured to cost more
// than the journaling itself. Only mov/movzx/lea/push/pop/ret are used;
// none of them modify EFLAGS.
//
// Cell layout used below: ptr @0 (8 bytes), msk @8 (4 bytes), val @12 (4 bytes).

#if defined(_WIN64) || defined(__CYGWIN__)
 #define TRK_ARG "rcx"
#else
 #define TRK_ARG "rdi"
#endif

#ifdef __ELF__
 #define TRK_TYPE(name) ".type " name ", @function\n"
 #define TRK_SIZE(name) ".size " name ", .-" name "\n"
#else
 #define TRK_TYPE(name) ".def " name "; .scl 2; .type 32; .endef\n"
 #define TRK_SIZE(name)
#endif

// name: symbol; mask: immediate for c.msk; load: instruction+operand reading *p into edx
#define TRK_STUB(name,mask,load) \
  ".globl " name "\n" \
  TRK_TYPE(name) \
  name ":\n" \
  "  push rax\n" \
  "  push rdx\n" \
  "  mov  eax, dword ptr [rip+trkptr]\n"   /* idx = trkptr            */ \
  "  lea  edx, [rax+1]\n" \
  "  mov  dword ptr [rip+trkptr], edx\n"   /* trkptr = idx+1          */ \
  "  lea  rdx, [rip+trk]\n" \
  "  lea  rax, [rax*8]\n" \
  "  lea  rax, [rdx+rax*2]\n"              /* rax = &trk[idx]         */ \
  "  mov  qword ptr [rax], " TRK_ARG "\n"  /* c.ptr = p               */ \
  "  mov  dword ptr [rax+8], " mask "\n"   /* c.msk = mask            */ \
  "  " load " [" TRK_ARG "]\n"             /* edx = *p (zero-extended)*/ \
  "  mov  dword ptr [rax+12], edx\n"       /* c.val = edx             */ \
  "  pop  rdx\n" \
  "  pop  rax\n" \
  "  ret\n" \
  TRK_SIZE(name)

// track8 journals a qword store as two 4-byte cells (Cell::val is 32-bit)
#define TRK_STUB8(name) \
  ".globl " name "\n" \
  TRK_TYPE(name) \
  name ":\n" \
  "  push rax\n" \
  "  push rdx\n" \
  "  mov  eax, dword ptr [rip+trkptr]\n" \
  "  lea  edx, [rax+2]\n" \
  "  mov  dword ptr [rip+trkptr], edx\n"   /* trkptr = idx+2          */ \
  "  lea  rdx, [rip+trk]\n" \
  "  lea  rax, [rax*8]\n" \
  "  lea  rax, [rdx+rax*2]\n"              /* rax = &trk[idx]         */ \
  "  mov  qword ptr [rax], " TRK_ARG "\n" \
  "  mov  dword ptr [rax+8], -1\n" \
  "  mov  edx, dword ptr [" TRK_ARG "]\n" \
  "  mov  dword ptr [rax+12], edx\n"       /* trk[idx]   = {p,  -1,*p}   */ \
  "  lea  rdx, [" TRK_ARG "+4]\n" \
  "  mov  qword ptr [rax+16], rdx\n" \
  "  mov  dword ptr [rax+24], -1\n" \
  "  mov  edx, dword ptr [" TRK_ARG "+4]\n" \
  "  mov  dword ptr [rax+28], edx\n"       /* trk[idx+1] = {p+4,-1,p[1]} */ \
  "  pop  rdx\n" \
  "  pop  rax\n" \
  "  ret\n" \
  TRK_SIZE(name)

asm(
  ".intel_syntax noprefix\n"
  ".text\n"
  TRK_STUB( "track4", "-1",     "mov   edx, dword ptr" )
  TRK_STUB( "track2", "0xFFFF", "movzx edx, word ptr"  )
  TRK_STUB( "track1", "0xFF",   "movzx edx, byte ptr"  )
  TRK_STUB8( "track8" )
  ".att_syntax prefix\n"
);

#undef TRK_STUB8
#undef TRK_STUB
#undef TRK_SIZE
#undef TRK_TYPE
#undef TRK_ARG

extern "C" {

void track8( void* _p ); // journal an 8-byte store at _p (as two cells)
void track4( void* _p ); // journal a 4-byte store at _p
void track2( void* _p ); // journal a 2-byte store at _p
void track1( void* _p ); // journal a 1-byte store at _p

uint encode_sim( void* _p, int y ); // instrumented model step, see coder.inc

}




template< class Model, uint bit, uint ctx, int K >
struct TEST_ENCODE {
  INLINE
  inline static void test_encode( Model& E, uint (&clen)[0x200] ) {
    constexpr uint cty = ctx*2 + bit;
    NEST();

    uint p = encode_sim( &E, bit );

    if( bit==0 ) {
      clen[ctx*2+0] = clen[ctx] + LOG2(SCALE-p);
      clen[ctx*2+1] = clen[ctx] + LOG2(p);
    }

    if( clen[cty]>0x90000*(8-K) ) {
      TEST_ENCODE<Model,0,cty,K-1>::test_encode( E, clen ); 
      TEST_ENCODE<Model,1,cty,K-1>::test_encode( E, clen ); 
    }

    UNDO();
  }
};

template< class Model, uint bit, uint ctx >
struct TEST_ENCODE<Model,bit,ctx,0> {
  INLINE
  inline static void test_encode( Model& E, uint (&clen)[0x200] ) { if( bit==0 ) {
    uint p = E.predictor.P(); //E.predictor.p; 
    clen[ctx*2+0] = clen[ctx] + LOG2(SCALE-p);
    clen[ctx*2+1] = clen[ctx] + LOG2(p);
  }}
};

template< class Model >
void test_encode( Model& E, uint (&clen)[0x200] ) {
  for( uint j=0; j<0x200; j++ ) clen[j]=0xFFFFF; clen[1]=0;

  TEST_ENCODE<Model,0,1,7>::test_encode( E, clen );
  TEST_ENCODE<Model,1,1,7>::test_encode( E, clen );
}

int unlog[0x100000];

void Track_Init( void ) {
  uint i;
  LOG2.Init();
  for( i=0; i<0x100000; i++ ) { unlog[i]=double(PSCALE)*pow(2.0,-double(i)/0x10000); if( unlog[i]<1 ) unlog[i]=1; }
}
