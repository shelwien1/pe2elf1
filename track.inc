
enum{ N_Cells=1<<20, N_Nest=0x200 };

// A prefix is explored while its accumulated code length stays below
// PRUNE_LOG per bit, in the 16.16 log2 scale of log2lut.inc; everything below
// that keeps the minimum frequency of 1. 0x90000 = 9.0, i.e. P(prefix) is still
// above 8^-depth, which is a cost of under 3 bits per bit. Lower = exact and
// slow, higher = fast and lossy; see the tail of log.txt for measurements.
#ifndef PRUNE_LOG
 #define PRUNE_LOG 0x90000
#endif

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

// Journal capacity is not checked per store - that is the hot path - so it is
// checked once per speculative step instead. A model step writing more than
// N_Margin cells would run off the end of trk[] and corrupt memory instead of
// restoring it, which is worth a loud death rather than a silent wrong answer.
enum{ N_Margin = 1<<16 };

void NEST( void ) {
  if( (trkptr>N_Cells-N_Margin) || (nesting>=N_Nest) ) {
    fprintf( stderr, "track: journal overflow (trkptr=%u nesting=%u)\n", trkptr, nesting );
    exit(5);
  }
  nest_trkptr[nesting++] = trkptr;
}

qword trk_undone = 0;   // cells rolled back so far; checked once, in main.inc

void UNDO( void ) {
  uint i;
  uint k = nest_trkptr[--nesting];
  trk_undone += trkptr - k;
  for( i=trkptr-1; int(i)>=int(k); i-- ) {
    Cell& c = trk[i];
    uint* p = (uint*)c.ptr;
    uint d = p[0] & (~c.msk);
    d |= c.val;
    p[0] = d;
  }
  trkptr = k;
}

// track1 ... track64 are the journaling stubs that track.pl inserts in front
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

// Wider stores journal ceil(N/4) consecutive 4-byte cells, since Cell::val is
// 32 bits. This covers the qword stores the compiler makes of two adjacent
// model words, the 16-byte stores of the SSE mixer in tangelo/mixer_train.inc,
// and anything the auto-vectorizer widens to 32 or 64 bytes. A masked vector
// store writes only some of its lanes; journaling the whole width is still
// correct, because saving and restoring bytes the store leaves alone is a no-op.
#define TRK_CELL(k) \
  "  lea  rdx, [" TRK_ARG "+" #k "*4]\n" \
  "  mov  qword ptr [rax+" #k "*16], rdx\n" \
  "  mov  dword ptr [rax+" #k "*16+8], -1\n" \
  "  mov  edx, dword ptr [" TRK_ARG "+" #k "*4]\n" \
  "  mov  dword ptr [rax+" #k "*16+12], edx\n"

#define TRK_C2  TRK_CELL(0)  TRK_CELL(1)
#define TRK_C4  TRK_C2       TRK_CELL(2)  TRK_CELL(3)
#define TRK_C8  TRK_C4       TRK_CELL(4)  TRK_CELL(5)  TRK_CELL(6)  TRK_CELL(7)
#define TRK_C16 TRK_C8       TRK_CELL(8)  TRK_CELL(9)  TRK_CELL(10) TRK_CELL(11) \
                             TRK_CELL(12) TRK_CELL(13) TRK_CELL(14) TRK_CELL(15)

// name: symbol; ncells: how many Cells it takes; cells: that many TRK_CELLs.
// Unrolled rather than looped because a loop counter would need a flag-setting
// instruction, and these stubs must leave EFLAGS alone.
#define TRK_STUBW(name,ncells,cells) \
  ".globl " name "\n" \
  TRK_TYPE(name) \
  name ":\n" \
  "  push rax\n" \
  "  push rdx\n" \
  "  mov  eax, dword ptr [rip+trkptr]\n" \
  "  lea  edx, [rax+" #ncells "]\n" \
  "  mov  dword ptr [rip+trkptr], edx\n" \
  "  lea  rdx, [rip+trk]\n" \
  "  lea  rax, [rax*8]\n" \
  "  lea  rax, [rdx+rax*2]\n"              /* rax = &trk[idx]         */ \
  cells \
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
  TRK_STUBW( "track8",   2, TRK_C2  )
  TRK_STUBW( "track16",  4, TRK_C4  )
  TRK_STUBW( "track32",  8, TRK_C8  )
  TRK_STUBW( "track64", 16, TRK_C16 )
  ".att_syntax prefix\n"
);

#undef TRK_STUBW
#undef TRK_C16
#undef TRK_C8
#undef TRK_C4
#undef TRK_C2
#undef TRK_CELL
#undef TRK_STUB
#undef TRK_SIZE
#undef TRK_TYPE
#undef TRK_ARG

extern "C" {

void track64( void* _p ); // journal a 64-byte store at _p (as 16 cells)
void track32( void* _p ); // journal a 32-byte store at _p (as  8 cells)
void track16( void* _p ); // journal a 16-byte store at _p (as  4 cells)
void track8(  void* _p ); // journal an 8-byte store at _p (as  2 cells)
void track4(  void* _p ); // journal a 4-byte store at _p
void track2(  void* _p ); // journal a 2-byte store at _p
void track1(  void* _p ); // journal a 1-byte store at _p

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

    if( clen[cty]>PRUNE_LOG*(8-K) ) {
      TEST_ENCODE<Model,0,cty,K-1>::test_encode( E, clen ); 
      TEST_ENCODE<Model,1,cty,K-1>::test_encode( E, clen ); 
    }

    UNDO();
  }
};

// Last bit: nothing is predicted after it inside this byte, so the model is not
// advanced at all - the pending prediction is read straight off the model. That
// removes half of the simulated steps, and all of their journaling.
template< class Model, uint bit, uint ctx >
struct TEST_ENCODE<Model,bit,ctx,0> {
  INLINE
  inline static void test_encode( Model& E, uint (&clen)[0x200] ) { if( bit==0 ) {
    uint p = E.P();
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
