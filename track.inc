
enum{ N_Cells=1<<20, N_Nest=0x200, N_Margin=1<<16 };

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
extern "C" Cell trk[N_Cells + N_Margin];

uint trkptr = 0;
uint nesting = 0;

uint nest_trkptr[N_Nest];

// N_Margin cells of slack past N_Cells: the stubs reserve their cells and write
// them with no bound test (that is the hot path), so the only guard is the
// capacity check in NEST() below, which runs once per step. If a step ever
// writes more cells than the margin, the slack is what keeps the overrun inside
// this array instead of on top of nest_trkptr, which immediately follows it in
// bss and which the check itself depends on.
alignas(64) Cell trk[N_Cells + N_Margin];

// Journal capacity is not checked per store - that is the hot path - so it is
// checked once per speculative step instead. A model step writing more than
// N_Margin cells would run off the end of trk[] and corrupt memory instead of
// restoring it, which is worth a loud death rather than a silent wrong answer.
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
    // Restore at the cell's own width, which is what its mask records. Doing it
    // with a 32-bit read-modify-write instead - the same for every cell - would
    // read and write up to 3 bytes past a 1- or 2-byte cell: harmless in the
    // middle of a global, but a fault at the end of a mapping, and slower.
    if(      c.msk==0xFFFFFFFFu ) *(uint*)c.ptr = c.val;
    else if( c.msk==0x0000FFFFu ) *(word*)c.ptr = word(c.val);
    else if( c.msk==0x000000FFu ) *(byte*)c.ptr = byte(c.val);
    else {
      // A wide record. c is its footer, naming the address and the width, and
      // the bytes it saved are in the cells just below, packed 16 to a cell.
      // One record for a 16- or 32-byte store rather than the four or eight
      // single-word cells it used to take - wide stores are nearly half of all
      // journal traffic, see SPEED.md. The footer is written last so that this
      // backward walk meets it before the payload it describes.
      uint nc = (c.msk+15)/16;
      const qword* src = (const qword*)&trk[i-nc];
      qword* dst = (qword*)c.ptr;
      for( uint w=0; w<c.msk/8; w++ ) dst[w] = src[w];
      i -= nc;
    }
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

// An 8-byte store is two single-word cells: a record for it would be one
// payload cell plus a footer, which is the same two cells and more work.
#define TRK_CELL(k) \
  "  lea  rdx, [" TRK_ARG "+" #k "*4]\n" \
  "  mov  qword ptr [rax+" #k "*16], rdx\n" \
  "  mov  dword ptr [rax+" #k "*16+8], -1\n" \
  "  mov  edx, dword ptr [" TRK_ARG "+" #k "*4]\n" \
  "  mov  dword ptr [rax+" #k "*16+12], edx\n"

#define TRK_STUB8(name) \
  ".globl " name "\n" \
  TRK_TYPE(name) \
  name ":\n" \
  "  push rax\n" \
  "  push rdx\n" \
  "  mov  eax, dword ptr [rip+trkptr]\n" \
  "  lea  edx, [rax+2]\n" \
  "  mov  dword ptr [rip+trkptr], edx\n" \
  "  lea  rdx, [rip+trk]\n" \
  "  lea  rax, [rax*8]\n" \
  "  lea  rax, [rdx+rax*2]\n" \
  TRK_CELL(0) TRK_CELL(1) \
  "  pop  rdx\n" \
  "  pop  rax\n" \
  "  ret\n" \
  TRK_SIZE(name)

// Anything wider - the 16-byte stores of the SSE mixer in
// tangelo/mixer_train.inc, and whatever the auto-vectorizer widens to 32 or 64
// - is journaled as ONE record: ncells cells of overwritten memory, copied
// verbatim, then a footer naming the address and the width. Writing it the
// obvious way instead, one 16-byte cell per 4 bytes overwritten, cost a 32-byte
// store eight cells and 128 bytes of journal to save 32 bytes of data.
//
// The footer's mask is the width itself, which is what tells UNDO() a record
// from a single cell: 16, 32 and 64 are none of the narrow tags 0xFF, 0xFFFF,
// 0xFFFFFFFF. The payload goes through rdx rather than a vector register, since
// the stub has to leave every register it does not save exactly as it was.
//
// (A store under an AVX-512 write mask cannot use any of these: reading the
// masked-off lanes to save them can fault exactly where the store would not, so
// track.pl refuses it instead.)
#define TRK_PAY(k) \
  "  mov  rdx, qword ptr [" TRK_ARG "+" #k "*16]\n" \
  "  mov  qword ptr [rax+" #k "*16], rdx\n" \
  "  mov  rdx, qword ptr [" TRK_ARG "+" #k "*16+8]\n" \
  "  mov  qword ptr [rax+" #k "*16+8], rdx\n"

#define TRK_P1  TRK_PAY(0)
#define TRK_P2  TRK_P1 TRK_PAY(1)
#define TRK_P4  TRK_P2 TRK_PAY(2) TRK_PAY(3)

#define TRK_STUBW(name,nbytes,ncells,payload) \
  ".globl " name "\n" \
  TRK_TYPE(name) \
  name ":\n" \
  "  push rax\n" \
  "  push rdx\n" \
  "  mov  eax, dword ptr [rip+trkptr]\n" \
  "  lea  edx, [rax+" #ncells "+1]\n" \
  "  mov  dword ptr [rip+trkptr], edx\n" \
  "  lea  rdx, [rip+trk]\n" \
  "  lea  rax, [rax*8]\n" \
  "  lea  rax, [rdx+rax*2]\n" \
  payload \
  "  mov  qword ptr [rax+" #ncells "*16], " TRK_ARG "\n" \
  "  mov  dword ptr [rax+" #ncells "*16+8], " #nbytes "\n" \
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
  TRK_STUBW( "track16", 16, 1, TRK_P1 )
  TRK_STUBW( "track32", 32, 2, TRK_P2 )
  TRK_STUBW( "track64", 64, 4, TRK_P4 )
  ".att_syntax prefix\n"
);

#undef TRK_STUBW
#undef TRK_P4
#undef TRK_P2
#undef TRK_P1
#undef TRK_PAY
#undef TRK_STUB8
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




// The walk that uses all of this lives in bytemodel.inc, which turns the model
// step into a byte distribution. What stays here is the journal itself, and the
// table the walk turns code lengths back into frequencies with.

int unlog[0x100000];

void Track_Init( void ) {
  uint i;
  LOG2.Init();
  for( i=0; i<0x100000; i++ ) { unlog[i]=double(PSCALE)*pow(2.0,-double(i)/0x10000); if( unlog[i]<1 ) unlog[i]=1; }
}
