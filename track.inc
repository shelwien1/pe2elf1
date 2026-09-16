#ifndef TRACK_INC
#define TRACK_INC

enum{ N_Cells=1<<20, N_Nest=0x200, N_Margin=1<<16 };

// A prefix is explored while its accumulated code length stays below
// PRUNE_LOG per bit, in the 16.16 log2 scale of log2lut.inc; everything below
// that keeps the minimum frequency of 1. 0x90000 = 9.0, i.e. P(prefix) is still
// above 8^-depth, which is a cost of under 3 bits per bit. Lower = exact and
// slow, higher = fast and lossy; see the tail of log.txt for measurements.
#ifndef PRUNE_LOG
 #define PRUNE_LOG 0x90000
#endif

// A journal cell is 8 bytes, not the 12 the fields suggest or the 16 an address
// and two words would take.
//
// Everything the model step writes lives inside one contiguous window of memory
// - the model object and the handful of globals beside it, which main.inc
// measures at startup (Track_Base) - so a store's address is recorded as a
// 30-bit offset from the base of that window rather than as a pointer. The two
// bits that frees hold the width, which used to need a whole second word:
//
//   off = (address - trk_base) | (tag<<30)     tag 0,1,2 = 1,2,4 bytes
//                                              tag 3     = wide record footer
//   val = the overwritten bytes                (a footer: the record's width)
//
// Halving the cell nearly halves the journal's memory traffic - 206 KB per input
// byte rather than 376 - and the journal is most of this program's running time
// (SPEED.md 3.5). It costs the stubs nothing either: forming the offset takes
// one extra load (trk_negbase), and it takes back three, because trkptr is a
// pointer rather than an index into 16-byte cells and the cell is two stores
// where it was four.
struct Cell {
  uint off;
  uint val;
};

enum{ N_Span = 1u<<30 };   // how far the window may reach, offsets being 30 bits

// The width tags, in place: a cell's off field is the offset with one of these
// added, so comparing off against them tests the tag without extracting it.
enum{ T1=0u<<30, T2=1u<<30, T4=2u<<30, TW=3u<<30 };

extern "C" Cell* trkptr;
extern "C" qword trk_negbase;
extern "C" Cell trk[N_Cells + N_Margin];

uint  nesting = 0;
byte* trk_base = 0;
uint  trk_span = 0;

// -trk_base, kept ready to add: the stubs turn an address into an offset with
// one lea and no flags, and lea cannot subtract a register.
qword trk_negbase = 0;

Cell* nest_trkptr[N_Nest];

// N_Margin cells of slack past N_Cells: the stubs reserve their cells and write
// them with no bound test (that is the hot path), so the only guard is the
// capacity check in NEST() below, which runs once per step. If a step ever
// writes more cells than the margin, the slack is what keeps the overrun inside
// this array instead of on top of nest_trkptr, which immediately follows it in
// bss and which the check itself depends on.
alignas(64) Cell trk[N_Cells + N_Margin];

Cell* trkptr = trk;

// The window every journaled address is an offset into. main.inc works it out
// from the model's own declaration of its state (TRACK_STATE_RANGES) and calls
// this once, before the first walk. The span is what makes the format safe to
// use: a store outside the window would be recorded as an offset that aliases
// some other address inside it, so the size is checked here, and -DTRACK_VERIFY
// checks every cell against it in UNDO().
void Track_Base( byte* lo, byte* hi ) {
  qword span = qword(hi-lo);
  if( span > N_Span ) {
    fprintf( stderr, "track: the model's state spans %llu bytes, more than the %u a\n"
                     "  journal offset can reach. Either the model is too large for this\n"
                     "  build, or TRACK_STATE_RANGES names two objects the linker put far\n"
                     "  apart - see the arena note in main.inc.\n", span, N_Span );
    exit(9);
  }
  trk_base    = lo;
  trk_span    = uint(span);
  trk_negbase = -qword(lo);
}

// Journal capacity is not checked per store - that is the hot path - so it is
// checked once per speculative step instead. A model step writing more than
// N_Margin cells would run off the end of trk[] and corrupt memory instead of
// restoring it, which is worth a loud death rather than a silent wrong answer.
void NEST( void ) {
  if( (trkptr>trk+(N_Cells-N_Margin)) || (nesting>=N_Nest) ) {
    fprintf( stderr, "track: journal overflow (cells=%u nesting=%u)\n", uint(trkptr-trk), nesting );
    exit(5);
  }
  nest_trkptr[nesting++] = trkptr;
}

qword trk_undone = 0;   // cells rolled back so far; checked once, in main.inc

// Journaling is on only inside the speculative walk: ByteModel::Predict() turns
// it on around the walk and off again, and the model's real update then runs
// with nothing recorded. The track.pl route does not need this - there the walk
// and the real update are two separately compiled copies of the step, and only
// one of them has journaling calls in it - but a model that marks its own writes
// (write.inc) has a single copy of each write, so the flag is what tells that
// copy which of the two it is running as.
bool trk_on = false;

// A wide record: c is its footer, naming the address and the width, and the
// bytes it saved are in the cells just below, packed 8 to a cell. One record for
// a store of 8 bytes or more, rather than the two to sixteen single-word cells
// it used to take - wide stores are nearly half of all journal traffic, see
// SPEED.md. The footer is written last, so the backward walk below meets it
// before the payload it describes.
//
// Only the tail is out of line. A record of one or two payload cells - which is
// every 8- and 16-byte store, and so nearly all of them - is copied straight
// line; leaving the loop in UNDO() instead let the vectorizer have it, and its
// four callee-saved registers and stack frame were then paid for by every cell,
// including by fpaq0mw, whose model has no store wider than 4 bytes at all.
NOINLINE static void undo_wide_tail( qword* dst, const qword* src, uint nc ) {
  for( uint w=2; w<nc; w++ ) dst[w] = src[w];
}

void UNDO( void ) {
  // One base per tag, each pre-biased by that tag's bit pattern, so that a
  // cell's offset can be added to it without first masking the tag off: the
  // restore is then a single indexed store. (A cell records an address as
  // (address-trk_base) | tag<<30; base[tag] is trk_base - tag<<30.)
  byte* b1 = trk_base;                    // tag 0: 1 byte
  byte* b2 = trk_base - (qword(1)<<30);   // tag 1: 2 bytes
  byte* b4 = trk_base - (qword(2)<<30);   // tag 2: 4 bytes
  byte* bw = trk_base - (qword(3)<<30);   // tag 3: wide record footer
  Cell* k = nest_trkptr[--nesting];
  trk_undone += trkptr - k;
  // Backwards, so that several writes to one location unwind in the right order.
  // A wide record's footer is met first and its payload stepped over below.
  for( Cell* c=trkptr-1; c>=k; --c ) {
    uint o = c->off;
    uint v = c->val;
#ifdef TRACK_VERIFY
    // Only a store inside the window Track_Base() measured can be represented,
    // and only the model's declared state is supposed to be stored to at all.
    // A store anywhere else is journaled as an offset that lands somewhere
    // arbitrary in the window, so say so here rather than corrupt whatever it
    // aliases.
    if( (o & (N_Span-1)) >= trk_span ) {
      fprintf( stderr, "track: a journaled store is outside the model state declared in\n"
                       "  TRACK_STATE_RANGES (offset %u, window %u bytes)\n",
               o & (N_Span-1), trk_span );
      exit(9);
    }
#endif
    // Restore at the cell's own width, which is what its tag records. Doing it
    // with a 32-bit read-modify-write instead - the same for every cell - would
    // read and write up to 3 bytes past a 1- or 2-byte cell: harmless in the
    // middle of a global, but a fault at the end of a mapping, and slower
    // (SPEED.md 4). The tag is tested by comparing the whole offset against the
    // tag boundaries rather than by shifting it out, and the order is the order
    // of how often each width occurs: that is two compares for the commonest
    // case and no extraction at all. A switch here is slower still, because the
    // compiler orders the tags as it likes and picks an order that puts the
    // commonest case last.
    if( o>=T4 ) {
      if( o<TW ) *(uint*)(b4+o) = v;
      else {
        uint nc = v>>3;
        const qword* src = (const qword*)(c-nc);
        qword* dst = (qword*)(bw+o);
        c -= nc;
        dst[0] = src[0];
        if( nc>1 ) { dst[1] = src[1]; if( nc>2 ) undo_wide_tail(dst,src,nc); }
      }
    }
    else if( o>=T2 ) *(word*)(b2+o) = word(v);
    else             *(byte*)(b1+o) = byte(v);
  }
  trkptr = k;
}

// The journaling stubs, for the track.pl route only. A program that marks its
// own writes instead (write.inc, -DTRACK_SRC) journals them inline, and needs
// neither the stubs nor a separately compiled encode_sim.
#ifndef TRACK_SRC

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
//   void trackN( void* p ) {                    // N = 1, 2, 4
//     Cell& c = *trkptr++;
//     c.off = uint((byte*)p - trk_base) | (tag<<30);
//     c.val = *(uint*)p & ((1ULL<<(8*N))-1);
//   }
//
// track8 and wider append a record instead: N/8 cells of payload and a footer.
//
// but written in assembly so that it provably preserves *everything*, EFLAGS
// included: the compiler is free to keep condition flags live across a store
// (e.g. cmp / mov [mem],reg / cmovcc), and a flag-preserving stub lets the
// instrumentation drop the pushf/popf pair, which was measured to cost more
// than the journaling itself. Only mov/movzx/lea/push/pop/ret are used;
// none of them modify EFLAGS - which is also why the offset is formed by adding
// trk_negbase with lea rather than by subtracting trk_base with sub.
//
// Cell layout used below: off @0 (4 bytes), val @4 (4 bytes).
// Tag immediates: 1<<30 = 0x40000000, 2<<30 and 3<<30 written as the negative
// signed displacements the assembler wants for the same 32 bits.

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

// Reserve nbytes of journal in rax, leaving trkptr past it. Common to every
// stub. rax and rdx are the only registers any of them touch, and both are
// saved and restored.
#define TRK_HEAD(name,nbytes) \
  ".globl " name "\n" \
  TRK_TYPE(name) \
  name ":\n" \
  "  push rax\n" \
  "  push rdx\n" \
  "  mov  rax, qword ptr [rip+trkptr]\n"   /* rax = &trk[trkptr]     */ \
  "  lea  rdx, [rax+" #nbytes "]\n" \
  "  mov  qword ptr [rip+trkptr], rdx\n"   /* trkptr += nbytes       */

#define TRK_TAIL \
  "  pop  rdx\n" \
  "  pop  rax\n" \
  "  ret\n"

// name: symbol; tag: lea displacement carrying the width tag; load: instruction
// +operand reading *p into edx
#define TRK_STUB(name,tag,load) \
  TRK_HEAD(name,8) \
  "  mov  rdx, qword ptr [rip+trk_negbase]\n" \
  "  lea  edx, [" TRK_ARG "+rdx" tag "]\n"  /* edx = (p-base)|tag<<30  */ \
  "  mov  dword ptr [rax], edx\n"           /* c.off                   */ \
  "  " load " [" TRK_ARG "]\n"              /* edx = *p (zero-extended)*/ \
  "  mov  dword ptr [rax+4], edx\n"         /* c.val                   */ \
  TRK_TAIL \
  TRK_SIZE(name)

// An 8-byte store and anything wider - the 16-byte stores of the SSE mixer in
// tangelo/mixer_train.inc, and whatever the auto-vectorizer widens to 32 or 64
// - is journaled as ONE record (UNDO() above puts it back): ncells cells of
// overwritten memory copied verbatim, then a footer naming the address and the
// width. The footer's tag is 3, which is none of the widths; its val is the
// width itself, which is how UNDO() knows how many payload cells to step over.
// The payload goes through rdx rather than a vector register, since the stub
// has to leave every register it does not save exactly as it was.
//
// (A store under an AVX-512 write mask cannot use any of these: reading the
// masked-off lanes to save them can fault exactly where the store would not, so
// track.pl refuses it instead.)
#define TRK_PAY(k) \
  "  mov  rdx, qword ptr [" TRK_ARG "+" #k "*8]\n" \
  "  mov  qword ptr [rax+" #k "*8], rdx\n"

#define TRK_P2  TRK_PAY(0) TRK_PAY(1)
#define TRK_P4  TRK_P2 TRK_PAY(2) TRK_PAY(3)
#define TRK_P8  TRK_P4 TRK_PAY(4) TRK_PAY(5) TRK_PAY(6) TRK_PAY(7)

#define TRK_STUBW(name,nbytes,ncells,payload) \
  TRK_HEAD(name,(ncells+1)*8) \
  payload \
  "  mov  rdx, qword ptr [rip+trk_negbase]\n" \
  "  lea  edx, [" TRK_ARG "+rdx-0x40000000]\n" \
  "  mov  dword ptr [rax+" #ncells "*8], edx\n" \
  "  mov  dword ptr [rax+" #ncells "*8+4], " #nbytes "\n" \
  TRK_TAIL \
  TRK_SIZE(name)

asm(
  ".intel_syntax noprefix\n"
  ".text\n"
  TRK_STUB( "track4", "-0x80000000", "mov   edx, dword ptr" )
  TRK_STUB( "track2", "+0x40000000", "movzx edx, word ptr"  )
  TRK_STUB( "track1", "",            "movzx edx, byte ptr"  )
  TRK_STUBW( "track8",   8, 1, TRK_PAY(0) )
  TRK_STUBW( "track16", 16, 2, TRK_P2 )
  TRK_STUBW( "track32", 32, 4, TRK_P4 )
  TRK_STUBW( "track64", 64, 8, TRK_P8 )
  ".att_syntax prefix\n"
);

#undef TRK_STUBW
#undef TRK_P8
#undef TRK_P4
#undef TRK_P2
#undef TRK_PAY
#undef TRK_STUB
#undef TRK_TAIL
#undef TRK_HEAD
#undef TRK_SIZE
#undef TRK_TYPE
#undef TRK_ARG

extern "C" {

void track64( void* _p ); // journal a 64-byte store at _p (as 9 cells)
void track32( void* _p ); // journal a 32-byte store at _p (as 5 cells)
void track16( void* _p ); // journal a 16-byte store at _p (as 3 cells)
void track8(  void* _p ); // journal an 8-byte store at _p (as 2 cells)
void track4(  void* _p ); // journal a 4-byte store at _p
void track2(  void* _p ); // journal a 2-byte store at _p
void track1(  void* _p ); // journal a 1-byte store at _p

uint encode_sim( void* _p, int y ); // instrumented model step, see coder.inc

}

#endif // !TRACK_SRC

// The walk that uses all of this lives in bytemodel.inc, which turns the model
// step into a byte distribution. What stays here is the journal itself, and the
// table the walk turns code lengths back into frequencies with.

int unlog[0x100000];

void Track_Init( void ) {
  uint i;
  LOG2.Init();
  for( i=0; i<0x100000; i++ ) { unlog[i]=double(PSCALE)*pow(2.0,-double(i)/0x10000); if( unlog[i]<1 ) unlog[i]=1; }
}

#endif // TRACK_INC
