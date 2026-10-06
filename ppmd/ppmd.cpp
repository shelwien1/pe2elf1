// Builds on Linux (g++/clang++) and Windows (MSVC or MinGW), x86-64 with AVX2.
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <intrin.h>
#else
#include <sys/mman.h>
#endif
#include <immintrin.h>
#include <memory.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// §10: per-byte getc/putc carry the implicit stream lock + a call. Use the
// unlocked variants (single-threaded codec). Falls back to locked on platforms
// lacking them. Carry/flush logic in ShiftLow is untouched.
#if defined(_WIN32)
  #define GETC_UNLOCKED(f)    _getc_nolock(f)
  #define PUTC_UNLOCKED(c,f)  _putc_nolock((c),(f))
#elif defined(__GLIBC__) || defined(__APPLE__) || defined(_POSIX_C_SOURCE)
  #define GETC_UNLOCKED(f)    getc_unlocked(f)
  #define PUTC_UNLOCKED(c,f)  putc_unlocked((c),(f))
#else
  #define GETC_UNLOCKED(f)    getc(f)
  #define PUTC_UNLOCKED(c,f)  putc((c),(f))
#endif

#ifdef _WIN32
static double now_s(void) {
  LARGE_INTEGER c, f;
  QueryPerformanceCounter(&c);
  QueryPerformanceFrequency(&f);
  return double(c.QuadPart)/double(f.QuadPart);
}
#else
static double now_s(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return double(ts.tv_sec)+double(ts.tv_nsec)*1e-9;
}
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#define ALWAYS_INLINE __forceinline
#define TARGET_AVX2
static inline unsigned ctz32(unsigned x) { unsigned long i; _BitScanForward(&i, x); return unsigned(i); }
static inline unsigned ctz64(unsigned long long x) { unsigned long i; _BitScanForward64(&i, x); return unsigned(i); }
#else
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#define TARGET_AVX2 __attribute__((target("avx2")))
static inline unsigned ctz32(unsigned x) { return unsigned(__builtin_ctz(x)); }
static inline unsigned ctz64(unsigned long long x) { return unsigned(__builtin_ctzll(x)); }
#endif

typedef unsigned short word;
typedef unsigned int uint;
typedef unsigned char byte;
typedef unsigned long long qword;
template <class T, int N> constexpr int DIM(T (&wr)[N]) {
  return sizeof(wr)/sizeof(wr[0]);
};
template <class T> T Min(T x, T y) {
  return (x<y) ? x : y;
}
template <class T> T CLAMP(const T &X, const T &LoX, const T &HiX) {
  return (X>=LoX) ? ((X<=HiX) ? (X) : (HiX)) : (LoX);
}
struct Rangecoder {
  enum { SCALElog = 15, SCALE = 1<<SCALElog };
  enum { NUM = 4, sTOP = 0x01000000U, gTOP = 0x00010000U, Thres = 0xFF000000U, Threg = 0x00FF0000U };
  int f_DEC;
  int f_quit;
  union { FILE* f; qword f_q; };
  union { FILE* g; qword g_q; };
  union {
    struct {
      uint low;
      uint Carry;
    };
    qword lowc;
    uint code;
  };
  uint FFNum;
  uint Cache;
  uint range;
  void SetIO(uint _f_DEC, FILE* _f, FILE* _g) { f = _f; g = _g; f_DEC = _f_DEC; }
  uint get(void) { return byte(GETC_UNLOCKED(f)); }
  void put(uint c) { PUTC_UNLOCKED(c, g); }
  void rc_Process(uint cumFreq, uint freq, uint totFreq) {
    uint tmp;
    tmp = cumFreq*range;
    if( f_DEC ) {
      code -= tmp;
    } else {
      lowc += tmp;
    }
    range *= freq;
    Renorm();
  }
  void rc_Arrange(uint totFreq) {
    range /= totFreq;
  }
  uint rc_GetFreq(uint totFreq) {
    return code/range;
  }
  void Renorm(void) {
    if( f_DEC ) {
      if( range<gTOP ) {
        range <<= 16;
        code <<= 16;
        code += (get()<<8)+get();
      } else if( range<sTOP ) {
        range <<= 8;
        code <<= 8;
        code += get();
      }
    } else {
      if( range<gTOP ) {
        range <<= 16;
        ShiftLow2();
      } else if( range<sTOP ) {
        range <<= 8;
        ShiftLow();
      }
    }
  }
  void rc_BProcess(uint freq, int &bit) {
    uint rnew;
    rnew = (range>>SCALElog)*freq;
    if( f_DEC ) {
      bit = (code>=rnew);
    }
    range = ((range-rnew-rnew)&(-bit))+rnew;
    rnew &= -bit;
    if( f_DEC ) {
      code -= rnew;
    } else {
      lowc += rnew;
    }
    Renorm();
  }
  void ShiftLow(void) {
    uint i;
    if( low<Thres||Carry ) {
      put(Cache+Carry);
      for( i = 0; i<FFNum; i++ ) put(Carry-1);
      FFNum = 0;
      Cache = low>>24;
      Carry = 0;
    } else {
      FFNum++;
    }
    low <<= 8;
  }
  void ShiftLow2(void) {
    uint i;
    if( low<Thres||Carry ) {
      put(Cache+Carry);
      for( i = 0; i<FFNum; i++ ) put(Carry-1);
      FFNum = 0;
      Cache = low>>24;
      Carry = 0;
    } else {
      FFNum++;
    }
    low &= sTOP-1;
    if( low<Threg ) {
      put(Cache);
      for( i = 0; i<FFNum; i++ ) put(0xFF);
      FFNum = 0;
      Cache = low>>16;
    } else {
      FFNum++;
    }
    low <<= 16;
  }
  void rcInit(void) {
    range = 0xFFFFFFFF;
    low = 0;
    FFNum = 0;
    Carry = 0;
    Cache = 0;
  }
  void rc_Init(void) {
    int _;
    rcInit();
    if( f_DEC==1 ) {
      code = 0;
      for( _ = 0; _<NUM+1; _++ ) { code <<= 8; code += get(); }
    }
  }
  void rc_Quit(void) {
    if( f_DEC==0 ) for( int _ = 0; _<NUM+1; _++ ) ShiftLow();
  }
};
struct FakeRangecoder {
  static const int f_DEC = 0;
  void rc_Process(uint, uint, uint) {}
  void rc_Arrange(uint) {}
  void rc_BProcess(uint, int&) {}
  uint rc_GetFreq(uint) { return 0; }
};
const int ORealMAX = 256;
enum { INT_BITS = 7, PERIOD_BITS = 7, TOT_BITS = INT_BITS+PERIOD_BITS, INTERVAL = 1<<INT_BITS, BIN_SCALE = 1<<TOT_BITS, ROUND = 16 };
enum { MAX_FREQ = 124, MAX_O = ORealMAX };
const signed char EscCoef[12] = {16, -10, 1, 51, 14, 89, 23, 35, 64, 26, -42, 43};
const byte ExpEscape[16] = {51, 43, 18, 12, 11, 9, 8, 7, 6, 5, 4, 3, 3, 2, 2, 2};

#pragma pack(push,1)

struct SEE2_CONTEXT {
  word Summ;
  byte Shift;
  byte Count;
  void init(uint InitVal) {
    Shift = PERIOD_BITS-4;
    Summ = InitVal<<Shift;
    Count = 7;
  }
  uint getMean() {
    return Summ>>Shift;
  }
  void setShift_rare() {
    uint i;
    i = Summ>>Shift;
    i = PERIOD_BITS-(i>40)-(i>280)-(i>1020);
    if( i<Shift ) {
      Summ >>= 1;
      Shift--;
    } else if( i>Shift ) {
      Summ <<= 1;
      Shift++;
    }
    Count = 5<<Shift;
  }
  void update() {
    if( (--Count)==0 )
      setShift_rare();
  }
};
struct SEE_Manager {
  // [27][128]: row = QTable[Freq-1] reaches 26 at the byte-max Freq=255, and
  // the column index NS2BSIndx+PrevSuccess+flags+RLbit reaches 67; the former
  // [25][64] overflowed both, corrupting adjacent state and yielding freq>=SCALE
  // in the binary coder (enc/dec desync). All cells are initialised below.
  word BinSumm[27][128];
  SEE2_CONTEXT SEE2Cont[23][32];
  SEE2_CONTEXT DummySEE2Cont;
  int PrevSuccess;
  int NumMasked;
  int RunLength;
  int InitRL;
  int BSumm;
  uint CharMask[256];
  uint EscCount;
  byte QTable[260];
  byte NS2BSIndx[256];
  void initialize(int MaxOrder) {
    int i, k, m, Step, s;
    byte i2f[27];
    for( i = 0; i<5; i++ ) QTable[i] = i;
    for( m = i = 5, k = Step = 1; i<260; i++ ) { QTable[i] = m; if( (--k)==0 ) k = ++Step, m++; }
    for( i = 0; i<256; i++ ) NS2BSIndx[i] = ((i>0)+(i>2)+(i>28))*2;
    memset(CharMask, 0, sizeof(CharMask));
    EscCount = 1;
    PrevSuccess = 0;
    NumMasked = 0;
    InitRL = -(MaxOrder<13 ? MaxOrder : 13);
    RunLength = InitRL;
    BSumm = 0;
    for( k = i = 0; i<27; i2f[i++] = k+1 ) while( QTable[k]==i ) k++;
    for( k = 0; k<128; k++ ) {
      for( s = i = 0; i<6; i++ ) s += EscCoef[2*i+((k>>i)&1)];
      s = 128*(s<32 ? 32 : (s>224 ? 224 : s));
      for( i = 0; i<27; i++ ) BinSumm[i][k] = BIN_SCALE-s/i2f[i];
    }
    for( i = 0; i<23; i++ ) for( k = 0; k<32; k++ ) SEE2Cont[i][k].init(8*i+5);
  }
};
#pragma pack(pop)

// ===========================================================================
// Compact context tree.
//
// * Every state is one 32-bit word: [sym:8][tf:8][succ:16].
//     multi context : tf = T<<7 | (freq-1)   (T=1: succ is a context ref)
//     binary context: tf = freq (8 bits, exact); the successor type is encoded by
//                     the record address: ≡0 mod 4 -> context, ≡2 mod 4 -> text.
// * Multi record   : [esc:7|resc:1][NumStats] + NU states   = 2+4*NU bytes.
//   Binary record  : one state                               = 4 bytes.
// * Leaf contexts (depth MaxOrder) never get a context successor, and their text
//   successor is never needed (CreateSuccessors takes it from the lazy state one
//   order below). Their states are [sym][tf], 2 bytes: a leaf multi is 2+2*NU
//   bytes, a leaf binary 2. A Ctx handle carries LEAFB for them.
// * Contexts that are not current are stored by Repack in smaller read-only cold
//   forms at the start of their page (path records for binary chains, codebooked
//   NU=2 records; see "cold records") and expanded when they become current.
// * The tree lives in 64 KB pages (virtual memory, committed on first touch).
//   A context reference is 16 bits:
//     >= FAR_LIM : near ref = page offset | M   (M=1 -> multi, 0 -> binary)
//     <  FAR_LIM : index into the page's far table -> global root id -> GRoot
//                  entry {page, near ref}, which is the parent slot of the root.
//   Children are allocated in their parent's page (per-page free lists, larger
//   free records are carved). Pages are compacted or split (a set of sibling
//   subtrees moves to a receiver page) only at the start of a step, so only
//   SuffCache[] and parentSlot[] need fixing up.
// * parentSlot[t] (the field referencing SuffCache[t]) is kept exact through
//   swaps, reallocations and the rescale permutation; the original re-derived it
//   by scanning all SuffCache entries for every state of a modified context.
// * Text successors are coarse: P>>CZK in 16 bits. The exact P is recovered at
//   materialisation by scanning that block for the first end of the context
//   string + symbol (the last order+1 bytes of history). P is the end of the
//   first occurrence of that pattern, so the recovery is exact.
// * The model is untouched: compressed output is byte-identical to the original
//   (except under a memory limit, where the smaller tree resets less often).
// ===========================================================================

typedef uintptr_t Ctx;     // record address | M

#pragma pack(push,1)
struct State {
  byte sym;
  byte tf;
  word succ;
};
#pragma pack(pop)

enum {
  PG_BITS = 16, PG_SIZE = 1u<<PG_BITS,
  RESV = 4096,                 // offsets below RESV are never allocated (first OS page untouched)
  FAR_LIM = RESV,              // ref values below FAR_LIM are far-table indices
  USABLE = PG_SIZE-RESV,
  NULL_SUCC = 0xFFFFu,
  NCLS = 63,                   // free-list classes by size: 0/1 = 4 bytes at ≡0/≡2 mod 4, 2 = 2 bytes,
                               // k>=3 = 2k bytes; NCLS = larger records (size kept in the record)
  ANYPAR = 4,                  // allocRec parity: any even offset
  NO_ROOT = 0xFFFFFFFFu,
  DEAD_PAGE = 0xFFFFFFFFu
};

struct GRootEnt { uint page; word ref; word depth; };

struct PageMeta {
  uint bump;                   // first free offset (>= RESV)
  uint live;                   // bytes in allocated records (upper bound: unreachable records count until compaction)
  uint hwm;                    // highest bump since last trim (touched memory)
  word freeHead[NCLS+1];       // [NCLS]: large records, each holding [next][size]
  qword fmask;                 // bit k: freeHead[k] non-empty
  uint cdead;                  // dead bytes in the cold region (expanded cold records)
  std::vector<uint> roots;     // global root ids whose record lives in this page
  std::vector<uint> fars;      // far table: global root id, or NO_ROOT when free
  std::vector<word> farFree;
  void reset() {
    bump = RESV; live = 0; hwm = RESV;
    memset(freeHead, 0, sizeof(freeHead)); fmask = 0; cdead = 0;
    roots.clear(); fars.clear(); farFree.clear();
  }
};

// ---------------------------------------------------------------------------
// Coarse text pointer block scan
// ---------------------------------------------------------------------------
// first pos in [lo, hi) where the L-byte pattern ends (win[pos-L..pos-1] == pat), or ~0
TARGET_AVX2
static uint scan_avx2(const byte* win, uint lo, uint hi, const byte* pat, uint L) {
  const __m256i vl = _mm256_set1_epi8((char)pat[L-1]);
  const __m256i vf = _mm256_set1_epi8((char)pat[0]);
  for( uint pos = lo; pos<hi; pos += 32 ) {
    if( !(pos & 63) ) _mm_prefetch((const char*)(win+pos+1024), _MM_HINT_T0);
    __m256i a = _mm256_loadu_si256((const __m256i*)(win+pos-1));
    __m256i b = _mm256_loadu_si256((const __m256i*)(win+pos-L));
    uint m = (uint)_mm256_movemask_epi8(_mm256_and_si256(_mm256_cmpeq_epi8(a, vl), _mm256_cmpeq_epi8(b, vf)));
    while( m ) {
      uint p = pos+ctz32(m);
      if( p>=hi ) return 0xFFFFFFFFu;
      if( L<=2 || memcmp(win+p-L+1, pat+1, L-2)==0 ) return p;
      m &= m-1;
    }
  }
  return 0xFFFFFFFFu;
}

// map_mem reserves address space; physical memory is used only where it is touched. The arena
// (commit=false) is committed page by page as it grows (commit_mem), which Windows needs and
// Linux does by itself; release_mem returns memory to the OS.
#ifdef _WIN32
static void* map_mem(qword size, bool commit = true) {
  return VirtualAlloc(0, SIZE_T(size), commit ? MEM_RESERVE|MEM_COMMIT : MEM_RESERVE, PAGE_READWRITE);
}
static void unmap_mem(void* p, qword) { if( p ) VirtualFree(p, 0, MEM_RELEASE); }
static void commit_mem(void* p, qword size) { VirtualAlloc(p, SIZE_T(size), MEM_COMMIT, PAGE_READWRITE); }
static void release_mem(void* p, qword size) { if( size ) VirtualFree(p, SIZE_T(size), MEM_DECOMMIT); }
#else
static void* map_mem(qword size, bool = true) {
  void* p = mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE, -1, 0);
  return (p==MAP_FAILED) ? 0 : p;
}
static void unmap_mem(void* p, qword size) { if( p ) munmap(p, size); }
static void commit_mem(void*, qword) {}
static void release_mem(void* p, qword size) { if( size ) madvise(p, size, MADV_DONTNEED); }
#endif

enum ContextFlagMasks { F_Rescaled = 0x04, F_HasText = 0x08, F_NextIsText = 0x10 };

struct Model {
  Rangecoder rc;
  SEE_Manager see;
  int _MaxOrder;
  int _ResetPerc;
  uint _WinSize;
  uint _filesize;
  // text window
  byte* WinMap; qword WinMapSize;
  byte* WinBeg; byte* WinEnd; byte* pText;
  uint CZK;
  // pages
  byte* arenaMap; qword arenaMapSize;
  byte* arena;
  uint maxPages, nPages;
  std::vector<uint> freePages;
  std::vector<PageMeta> pm;
  std::vector<word> pgSlack;   // bytes a page can bump-allocate without touching a new OS page (dense, hot)
  GRootEnt* groot; uint nGroot, maxGroot;
  qword live_total;
  // statistics
  qword touched_total;          // sum over pages of committed (touched) OS pages
  // model state
  int OrderFall;
  int order, maxorder;
  bool m_replay = false;
  bool m_replay_aborted = false;
  State* FoundState;
  Ctx Order0;
  Ctx SuffCache[MAX_O+2];
  uint NumStats_Cache[MAX_O+2];
  uint Flags_Cache[MAX_O+2];
  State* StateCache[MAX_O+2];
  Ctx StateCacheCtx[MAX_O+2];
  word* parentSlot[MAX_O+2];
  int hwm;
  // maintenance policy
  // maintenance policy
  enum {
    POL_DEADSH = 6,       // compact when dead bytes > live>>6 ...
    POL_CDEADSH = 5,      // ... not counting dead cold bytes up to live>>5
    POL_SPLITNUM = 3,     // a split moves about 3/16 of the page
    POL_RECVGAP = 8192,   // free bytes a receiver page must keep
    POL_MARGIN = 2048     // split when live + need + margin > USABLE
  };
  uint recvPage;
  // maintenance scratch
  byte* scratch;
  byte farUsed[FAR_LIM];

  // ---------------- record / state accessors ----------------
  // A context at depth MaxOrder (a leaf) stores no successors: its states are [sym][tf], 2 bytes.
  // Its Ctx handle carries LEAFB.
  // A cold record's Ctx carries COLDB (§ cold records).
  static const Ctx LEAFB = Ctx(1)<<62, COLDB = Ctx(1)<<61;
  static bool isM(Ctx c) { return (c & 1)!=0; }
  static bool isL(Ctx c) { return (c & LEAFB)!=0; }
  // rec: the record of a hot context (bit 0 is M); crec: also for a cold one, which may start at
  // an odd address. Cold handles only reach the maintenance code.
  static byte* rec(Ctx c) { return (byte*)(c & ~(Ctx)1 & ~LEAFB); }
  static byte* crec(Ctx c) { return (byte*)(c & ~LEAFB & ~COLDB & ~(((c>>61) & 1) ^ 1)); }
  static Ctx mkCtx(byte* r, uint m) { return (Ctx)r | (Ctx)m; }
  static Ctx mkCtxL(byte* r, uint m, bool leaf) { return (Ctx)r | (Ctx)m | (leaf ? LEAFB : 0); }
  static uint SH(Ctx c) { return isL(c) ? 1u : 2u; }     // log2 of the state size
  static uint NS(Ctx c) { return isM(c) ? rec(c)[1] : 0u; }
  static State* S0(Ctx c) { return (State*)(rec(c) + (isM(c) ? 2 : 0)); }
  static State* SI(Ctx c, uint i) { return (State*)((byte*)S0(c) + (i<<SH(c))); }
  static uint SIdx(Ctx c, const State* s) { return uint(((const byte*)s-(const byte*)S0(c))>>SH(c)); }
  static uint EscF(Ctx c) { return isM(c) ? (rec(c)[0] & 0x7Fu) : 0u; }
  static void setEscF(Ctx c, uint v) { if( isM(c) ) rec(c)[0] = byte((rec(c)[0] & 0x80) | ((v>127) ? 127u : v)); }
  static bool Resc(Ctx c) { return isM(c) && (rec(c)[0] & 0x80)!=0; }
  static void setResc(Ctx c) { if( isM(c) ) rec(c)[0] |= 0x80; }
  static void clrResc(Ctx c) { if( isM(c) ) rec(c)[0] &= 0x7F; }
  static uint MF(const State& s) { return (s.tf & 0x7Fu)+1; }              // multi freq
  static void MaddF(State& s, uint d) { s.tf = byte(s.tf+d); }             // freq+d must stay <= 128
  static uint F(Ctx c, const State* s) { return isM(c) ? MF(*s) : s->tf; }
  static bool succIsCtx(Ctx c, const State* s) { return isL(c) ? false : isM(c) ? (s->tf & 0x80)!=0 : (c & 2)==0; }
  static bool succIsNull(Ctx c, const State* s) { return !isL(c) && !succIsCtx(c, s) && s->succ==NULL_SUCC; }
  static bool succIsText(Ctx c, const State* s) { return isL(c) || (!succIsCtx(c, s) && s->succ!=NULL_SUCC); }
  static uint recSizeNU(uint NU, bool leaf) { return leaf ? (NU==1 ? 2u : 2u+2u*NU) : (NU==1 ? 4u : 2u+4u*NU); }
  static word refOf(Ctx c) { return word(((uintptr_t)rec(c) & (PG_SIZE-1)) | (c & 1)); }
  static uint offOf(const void* a) { return uint((uintptr_t)a & (PG_SIZE-1)); }
  byte* pageBase(uint p) const { return arena + (qword(p)<<PG_BITS); }
  uint pageOf(const void* a) const { return uint(qword((const byte*)a - arena)>>PG_BITS); }
  static void swapS(State& a, State& b) { State t = a; a = b; b = t; }
  static void swapSt(Ctx c, State* a, State* b) {
    if( isL(c) ) { word t = *(word*)a; *(word*)a = *(word*)b; *(word*)b = t; } else swapS(*a, *b);
  }

  // cd: depth of the child
  Ctx child(Ctx c, State* s, word** slot, int cd) {
    word r = s->succ;
    bool leaf = cd>=_MaxOrder;
    uint p = pageOf(rec(c));
    if( r>=FAR_LIM ) {
      if( slot ) *slot = &s->succ;
      if( r<pgCold[p] ) return mkCtxL(pageBase(p)+r, 0, leaf) | COLDB;
      return mkCtxL(pageBase(p)+(r & ~1u), r & 1u, leaf);
    }
    uint gid = pm[p].fars[r];
    GRootEnt& g = groot[gid];
    if( slot ) *slot = &g.ref;
    if( g.ref<pgCold[g.page] ) return mkCtxL(pageBase(g.page)+g.ref, 0, leaf) | COLDB;
    return mkCtxL(pageBase(g.page)+(g.ref & ~1u), g.ref & 1u, leaf);
  }
  uint SummFreq(Ctx c) {
    State* s0 = S0(c);
    if( !isM(c) ) return s0[0].tf;
    uint sum = EscF(c), ns = NS(c), sh = SH(c);
    const byte* t = (const byte*)s0+1;
    for( uint i = 0; i<=ns; i++ ) sum += (t[i<<sh] & 0x7Fu)+1;
    return sum;
  }
  State* FindState(Ctx c, byte sym) {
    byte* s0 = (byte*)S0(c);
    uint ns = NS(c), sh = SH(c);
    for( uint i = 0; i<=ns; i++ ) if( s0[i<<sh]==sym ) return (State*)(s0+(i<<sh));
    return 0;
  }
  word textRef(const byte* ptr) const { return word(uint(ptr-WinBeg)>>CZK); }

  // ---------------- per-page allocator ----------------
  // Free records are kept by exact size (2-byte units); 4-byte slots also by parity, because a
  // non-leaf binary's successor type is its address parity. A miss carves the smallest larger
  // free record; every remainder (>= 2 bytes) is usable.
  static uint clsOf(uint sz, uint off) { return sz==4 ? ((off & 2) ? 1u : 0u) : sz==2 ? 2u : (sz/2<NCLS ? sz/2 : (uint)NCLS); }
  void pushFree(PageMeta& m, byte* base, uint off, uint sz) {
    if( sz<2 ) return;
    uint k = clsOf(sz, off);
    if( k==NCLS ) *(word*)(base+off+2) = word(sz);
    *(word*)(base+off) = m.freeHead[k];
    m.freeHead[k] = word(off);
    m.fmask |= qword(1)<<k;
  }
  uint popCls(PageMeta& m, byte* base, uint k) {
    uint off = m.freeHead[k];
    m.freeHead[k] = *(word*)(base+off);
    if( !m.freeHead[k] ) m.fmask &= ~(qword(1)<<k);
    return off;
  }
  // par: 0/2 = required offset mod 4 (4-byte binaries only), ANYPAR = any even offset
  byte* allocRec(uint p, uint sz, uint par) {
    PageMeta& m = pm[p];
    byte* base = pageBase(p);
    m.live += sz; live_total += sz;
    uint k = sz==4 ? (par==2 ? 1u : 0u) : clsOf(sz, 0);
    if( k<NCLS && m.freeHead[k] ) return base+popCls(m, base, k);
    // smallest larger exact class: sizes 2 (k=2) < 4 (k=0,1) < 6 (k=3) < ...
    qword cand = m.fmask & ((qword(1)<<NCLS)-1);
    if( sz==2 ) cand &= ~qword(4);
    else cand &= ~qword(7) & ~((qword(2)<<(sz/2<NCLS ? sz/2 : NCLS-1))-1);
    uint off = 0, have = 0;
    if( cand ) {
      uint c = ctz64(cand);
      off = popCls(m, base, c);
      have = c<2 ? 4u : 2u*c;
    } else if( m.freeHead[NCLS] ) {
      word* link = &m.freeHead[NCLS];
      while( *link ) {
        uint o = *link;
        uint n = *(word*)(base+o+2);
        if( n>=sz+(sz==4 ? 2u : 0u) ) {
          *link = *(word*)(base+o);
          if( !m.freeHead[NCLS] ) m.fmask &= ~(qword(1)<<NCLS);
          off = o; have = n;
          break;
        }
        link = (word*)(base+o);
      }
    }
    if( have ) {
      if( par!=ANYPAR && (off & 3)!=par ) { pushFree(m, base, off, 2); off += 2; have -= 2; }
      pushFree(m, base, off+sz, have-sz);
      return base+off;
    }
    off = m.bump;
    uint pad = (par!=ANYPAR && (off & 3)!=par) ? 2u : 0u;
    if( off+pad+sz>PG_SIZE ) {
      fprintf(stderr, "\nfatal: page %u overflow (bump=%u sz=%u live=%u)\n", p, m.bump, sz, m.live);
      exit(7);
    }
    if( off+pad+sz>m.hwm ) setHwm(p, off+pad+sz);
    if( pad ) { pushFree(m, base, off, 2); off += 2; }
    m.bump = off+sz;
    updSlack(p);
    return base+off;
  }
  void freeRec(byte* r, uint sz) {
    uint p = pageOf(r);
    PageMeta& m = pm[p];
    m.live -= sz; live_total -= sz;
    pushFree(m, pageBase(p), offOf(r), sz);
  }
  static uint osPages(uint hwm) { return ((hwm+4095)>>12)-1; }   // touched OS pages above RESV
  // [RESV, hwm) of a page may be written; everything a page writes is below its hwm first
  void setHwm(uint p, uint hwm) {
    PageMeta& m = pm[p];
    uint a = (m.hwm+4095) & ~4095u, b = (hwm+4095) & ~4095u;
    if( b>a ) commit_mem(pageBase(p)+a, b-a);
    touched_total += (qword(osPages(hwm))-qword(osPages(m.hwm)))<<12;
    m.hwm = hwm;
  }
  void updSlack(uint p) {
    const PageMeta& m = pm[p];
    uint lim = (m.hwm+4095) & ~4095u;
    if( lim>PG_SIZE-64 ) lim = PG_SIZE-64;
    pgSlack[p] = word(lim>m.bump ? lim-m.bump : 0);
  }
  uint newPage() {
    uint p;
    if( !freePages.empty() ) { p = freePages.back(); freePages.pop_back(); }
    else if( nPages<maxPages ) { p = nPages++; if( pm.size()<nPages ) pm.emplace_back(); }
    else return DEAD_PAGE;
    if( p<pm.size() && pm[p].hwm>=RESV ) setHwm(p, RESV);
    pm[p].reset();
    pgCold[p] = RESV;
    updSlack(p);
    return p;
  }
  word allocFar(uint p, uint gid) {
    PageMeta& m = pm[p];
    if( !m.farFree.empty() ) { word j = m.farFree.back(); m.farFree.pop_back(); m.fars[j] = gid; return j; }
    if( m.fars.size()>=FAR_LIM ) { fprintf(stderr, "\nfatal: far table full in page %u\n", p); exit(7); }
    m.fars.push_back(gid);
    return word(m.fars.size()-1);
  }
  void freeFar(uint p, word j) { pm[p].fars[j] = NO_ROOT; pm[p].farFree.push_back(j); }
  uint newGRoot(uint page, word ref, uint depth) {
    if( nGroot>=maxGroot ) { fprintf(stderr, "\nfatal: root table full\n"); exit(7); }
    uint g = nGroot++;
    groot[g].page = page; groot[g].ref = ref; groot[g].depth = word(depth);
    return g;
  }
  Ctx rootCtx(uint gid) const {
    const GRootEnt& g = groot[gid];
    if( g.ref<pgCold[g.page] ) return mkCtxL(pageBase(g.page)+g.ref, 0, g.depth>=_MaxOrder) | COLDB;
    return mkCtxL(pageBase(g.page)+(g.ref & ~1u), g.ref & 1u, g.depth>=_MaxOrder);
  }

  // ---------------- cold records ----------------
  // Records that are not current can be stored in a smaller, read-only form. Repack writes them
  // in [RESV, pgCold[p]) of each page, byte-aligned; a near ref below pgCold[p] is the exact byte
  // offset of a cold record (no M bit), and the record's first byte tells its kind:
  //   path  [A:1 (0x10)|N:1 (0x08)|L-1:3][sym x L][freq x L][end ref:16]
  //         a chain of L binaries, each the only child of the previous one. With N the freqs
  //         are nibbles (freq-1, element e in nibble e, low first, (L+1)/2 bytes). The end ref
  //         (the last element's child) is absent when the last element is a leaf, or when A is
  //         set: then the child is the cold record that follows. L >= 2 unless A is set.
  //   NU=2  [code][y0][y1][succ0:16][succ1:16]          (a leaf has no successors)
  //         code: 2 bytes big-endian 0x2000 + code for the first CB_SHORT codes, else 3 bytes
  //         [0xF0 | c>>16][c>>8][c] with c = code - CB_SHORT.
  //         code -> key (EscFreq|resc, tf0, tf1, adj) through a codebook of the keys in use
  //         (reference counted: a code is recycled when no cold record uses it). adj = j+1:
  //         the context child of state j is the cold record that follows (no succ field).
  // A cold record is expanded into its hot form (in the same page) when it becomes current, at
  // the start of a step; its bytes stay dead until the next Repack of the page.
  enum { CPATH_LIM = 0x20, CPATH_ADJ = 0x10, CPATH_NIB = 0x08, PATH_MAXL = 8,
         CNU2_BASE = 0x2000, CNU2_LONG = 0xF0, CB_SHORT = (CNU2_LONG<<8)-CNU2_BASE, CB_MAX = 1<<17, CBH_BITS = 18 };
  std::vector<word> pgCold;     // per page: end of the cold region
  std::vector<uint> cbKey;      // code -> key (byte0 | tf0<<8 | tf1<<16 | adj<<24)
  std::vector<qword> cbHash;    // open addressing: code<<32 | key+1
  std::vector<uint> cbRef;      // cold records using each code
  std::vector<uint> cbFree, cbFreeS;   // recycled long / short codes
  byte coldTmp[64];
  static bool isC(Ctx c) { return (c & COLDB)!=0; }
  int cbCode(uint key) {
    uint h = cbSlot(key);
    for( ;; ) {
      qword v = cbHash[h];
      if( !v ) break;
      if( uint(v)==key+1 ) return int(v>>32);
      h = (h+1) & ((1u<<CBH_BITS)-1);
    }
    // a short (2-byte) code if one is free, else a long one
    uint code;
    if( !cbFreeS.empty() ) { code = cbFreeS.back(); cbFreeS.pop_back(); cbKey[code] = key; }
    else if( cbKey.size()<CB_SHORT ) { code = uint(cbKey.size()); cbKey.push_back(key); cbRef.push_back(0); }
    else if( !cbFree.empty() ) { code = cbFree.back(); cbFree.pop_back(); cbKey[code] = key; }
    else if( cbKey.size()<CB_MAX ) { code = uint(cbKey.size()); cbKey.push_back(key); cbRef.push_back(0); }
    else return -1;
    cbHash[h] = (qword(code)<<32) | (key+1);
    return int(code);
  }
  static uint cbSlot(uint key) { return (key*2654435761u)>>(32-CBH_BITS); }
  void cbRelease(uint code) {
    if( --cbRef[code] ) return;
    // remove the key (linear probing, backward-shift deletion) and recycle the code
    const uint M = (1u<<CBH_BITS)-1;
    uint key = cbKey[code], i = cbSlot(key);
    while( uint(cbHash[i])!=key+1 ) i = (i+1) & M;
    for( ;; ) {
      cbHash[i] = 0;
      uint j = i;
      for( ;; ) {
        j = (j+1) & M;
        if( !cbHash[j] ) { (code<CB_SHORT ? cbFreeS : cbFree).push_back(code); return; }
        uint k = cbSlot(uint(cbHash[j])-1);
        // move j back to i unless its home slot k lies cyclically in (i, j]
        bool stay = (i<=j) ? (i<k && k<=j) : (i<k || k<=j);
        if( !stay ) { cbHash[i] = cbHash[j]; i = j; break; }
      }
    }
  }
  void cbReset() {
    cbKey.clear(); cbRef.clear(); cbFree.clear(); cbFreeS.clear();
    std::fill(cbHash.begin(), cbHash.end(), 0);
  }
  static uint wget(const byte* p) { return p[0] | (uint(p[1])<<8); }
  static void wput(byte* p, uint v) { p[0] = byte(v); p[1] = byte(v>>8); }
  bool lastIsLeaf(uint depth, uint L) const { return int(depth+L-1)>=_MaxOrder; }
  // NU=2 record: header length and code
  static uint nu2Hdr(const byte* r, uint* code) {
    if( r[0]<CNU2_LONG ) { *code = ((r[0]<<8) | r[1])-CNU2_BASE; return 2; }
    *code = CB_SHORT+(((r[0] & 0x0Fu)<<16) | (uint(r[1])<<8) | r[2]); return 3;
  }
  static uint nu2PutHdr(byte* d, uint code) {
    if( code<CB_SHORT ) { uint v = CNU2_BASE+code; d[0] = byte(v>>8); d[1] = byte(v); return 2; }
    uint c = code-CB_SHORT; d[0] = byte(CNU2_LONG | (c>>16)); d[1] = byte(c>>8); d[2] = byte(c); return 3;
  }
  static uint nu2Size(uint code, uint key, bool leaf) {
    return (code<CB_SHORT ? 2u : 3u)+2+(leaf ? 0u : (key>>24) ? 2u : 4u);
  }
  // the successor of state j of the NU=2 record at offset off (hl: header length)
  static word nu2Succ(const byte* r, uint off, uint hl, uint code, uint key, uint j, bool leaf) {
    uint adj = key>>24;
    if( adj==j+1 ) return word(off+nu2Size(code, key, leaf));
    return word(wget(r+hl+2+((j==1 && adj!=1) ? 2u : 0u)));
  }
  static uint pathL(const byte* r) { return (r[0] & 0x07u)+1; }
  static bool pathNib(const byte* r) { return (r[0] & CPATH_NIB)!=0; }
  static uint pathFB(uint L, bool nib) { return nib ? (L+1)/2 : L; }   // bytes of freqs
  static uint pathFreq(const byte* r, uint L, uint e) {
    return pathNib(r) ? ((r[1+L+(e>>1)]>>(4*(e & 1))) & 15u)+1 : r[1+L+e];
  }
  static bool pathAdj(const byte* r) { return (r[0] & CPATH_ADJ)!=0; }
  uint coldSize(const byte* r, uint depth) const {
    if( r[0]<CPATH_LIM ) { uint L = pathL(r); return 1+L+pathFB(L, pathNib(r))+((pathAdj(r) || lastIsLeaf(depth, L)) ? 0u : 2u); }
    uint code; nu2Hdr(r, &code);
    return nu2Size(code, cbKey[code], int(depth)>=_MaxOrder);
  }
  uint coldHotSize(const byte* r, uint depth) const {
    if( r[0]<CPATH_LIM ) { uint L = pathL(r); return 4*(L-1)+(lastIsLeaf(depth, L) ? 2u : 4u); }
    return int(depth)>=_MaxOrder ? 6u : 10u;
  }
  // ref of the child of a path record's last element (not a leaf); off: the record's offset
  word pathChild(const byte* r, uint off) const {
    uint L = pathL(r);
    uint e = 1+L+pathFB(L, pathNib(r));
    return pathAdj(r) ? word(off+e) : word(wget(r+e));
  }
  // hot-format copy of a cold record's first context, for reading only (BequeathFreq)
  Ctx coldView(Ctx c) {
    const byte* r = crec(c);
    byte* t = coldTmp;
    if( r[0]<CPATH_LIM ) {
      uint L = pathL(r);
      t[0] = r[1]; t[1] = byte(pathFreq(r, L, 0)); t[2] = t[3] = 0;
      return mkCtxL(t, 0, false);
    }
    uint code, hl = nu2Hdr(r, &code), key = cbKey[code];
    bool leaf = isL(c);
    t[0] = byte(key); t[1] = 1;
    State* s = (State*)(t+2);
    if( leaf ) {
      t[2] = r[hl]; t[3] = byte(key>>8); t[4] = r[hl+1]; t[5] = byte(key>>16);
    } else {
      s[0].sym = r[hl]; s[0].tf = byte(key>>8); s[0].succ = 0;
      s[1].sym = r[hl+1]; s[1].tf = byte(key>>16); s[1].succ = 0;
    }
    return mkCtxL(t, 1, leaf);
  }
  // Expands the cold SuffCache[i] in place of its page's free space; the page has the headroom.
  void expandCold(int i) {
    Ctx c = SuffCache[i];
    byte* r = crec(c);
    uint p = pageOf(r);
    uint csz = coldSize(r, uint(i));
    Ctx nc;
    if( r[0]<CPATH_LIM ) {
      uint L = pathL(r);
      bool ll = lastIsLeaf(uint(i), L);
      word nxt = ll ? 0 : pathChild(r, offOf(r));
      byte* e = 0;
      for( int k = int(L)-1; k>=0; k-- ) {
        bool leaf = (k==int(L)-1) && ll;
        e = allocRec(p, leaf ? 2u : 4u, leaf ? uint(ANYPAR) : 0u);
        State* s = (State*)e;
        s->sym = r[1+k]; s->tf = byte(pathFreq(r, L, uint(k)));
        if( !leaf ) s->succ = nxt;
        nxt = word(offOf(e));
      }
      nc = mkCtxL(e, 0, L==1 && ll);
    } else {
      uint code, hl = nu2Hdr(r, &code), key = cbKey[code];
      bool leaf = isL(c);
      byte* nr = allocRec(p, leaf ? 6u : 10u, ANYPAR);
      nr[0] = byte(key); nr[1] = 1;
      if( leaf ) {
        nr[2] = r[hl]; nr[3] = byte(key>>8); nr[4] = r[hl+1]; nr[5] = byte(key>>16);
      } else {
        State* s = (State*)(nr+2);
        s[0].sym = r[hl]; s[0].tf = byte(key>>8); s[0].succ = nu2Succ(r, offOf(r), hl, code, key, 0, false);
        s[1].sym = r[hl+1]; s[1].tf = byte(key>>16); s[1].succ = nu2Succ(r, offOf(r), hl, code, key, 1, false);
      }
      cbRelease(code);
      nc = mkCtxL(nr, 1, leaf);
    }
    PageMeta& m = pm[p];
    m.live -= csz; live_total -= csz; m.cdead += csz;
    if( i>0 ) *parentSlot[i] = refOf(nc);
    SuffCache[i] = nc;
  }

  // ---------------- maintenance (step boundary only) ----------------
  // K_LBIN: leaf binary (2 bytes); K_PATH: a cold path record, as one node until it is exploded
  enum { K_MULTI = 0, K_BIN0 = 1, K_BIN2 = 2, K_LBIN = 3, K_PATH = 4 };
  // F_KEEP: a cold record copied as it is
  enum { F_HOT = 0, F_CPATH = 1, F_CNU2 = 2, F_INNER = 3, F_KEEP = 4 };
  enum { NOOFF = 0xFFFF };
  // One node per context. off: offset of its source record (NOOFF for the inner elements of a
  // cold path record); sz: size of its hot form; sp: its states (in the page for hot non-leaf
  // records, otherwise decoded into DS).
  struct LNode {
    word off; word sz; int parent; word st; byte kind; byte moved; byte depth; byte leaf;
    byte ns; byte hdr; byte cold; byte pin; byte fmt; byte adj; byte placed; byte nib; word nref;
    const State* sp; int next;
    union { uint sub; uint gid; };     // subtree size (split selection), then the moved root's id
    union { int tail; int code; };     // last element of a path / codebook index of an NU=2
  };
  static bool isColdFmt(uint f) { return f==F_CNU2 || f==F_CPATH || f==F_KEEP; }
  uint coldNodeSize(const LNode& L) const {
    if( L.fmt==F_CNU2 ) return nu2Size(uint(L.code), cbKey[L.code], L.leaf);
    uint m = L.ns+1u;
    bool noEnd = L.adj || (L.fmt==F_CPATH ? Ls[L.tail].kind==K_LBIN : L.hdr!=0);
    return 1+m+pathFB(m, L.nib)+(noEnd ? 0u : 2u);
  }
  enum { DS_MAX = 40000 };
  struct NodeBuf {
    LNode* a; uint n;
    LNode& operator[](size_t i) { return a[i]; }
    const LNode& operator[](size_t i) const { return a[i]; }
    size_t size() const { return n; }
  } Ls;
  uint nRootNodes;              // the first nodes are the page roots
  LNode& newNode() {
    if( Ls.n>=DS_MAX ) { fprintf(stderr, "\nfatal: node buffer full\n"); exit(7); }
    return Ls.a[Ls.n++];
  }
  State DS[DS_MAX]; uint nDS;
  State* dsAlloc(uint n) {
    if( nDS+n>DS_MAX ) { fprintf(stderr, "\nfatal: decode buffer full\n"); exit(7); }
    State* r = DS+nDS; nDS += n; return r;
  }
  std::vector<int> selBuf, kidBuf;
  uint fwdN[PG_SIZE/2];         // source offset/2 -> node index, valid where fwdStamp == enumStamp
  uint fwdStamp[PG_SIZE/2];
  uint enumStamp = 0;
  void removeDeadRoots(uint p) {
    std::vector<uint>& R = pm[p].roots;
    for( size_t k = 0; k<R.size(); ) {
      if( R[k]==NO_ROOT || groot[R[k]].page!=p ) { R[k] = R.back(); R.pop_back(); } else k++;
    }
  }
  static void initNode(LNode& L, int parent, uint st, uint depth, bool leaf) {
    L.parent = parent; L.st = word(st); L.depth = byte(depth); L.leaf = leaf;
    L.moved = 0; L.ns = 0; L.hdr = 0; L.pin = 0; L.fmt = F_HOT; L.adj = 0; L.placed = 0; L.nib = 0; L.nref = 0; L.sub = 0;
    L.next = -1; L.tail = -1;
  }
  ALWAYS_INLINE int addNode(const byte* base, uint kc, word r, int parent, uint st, uint depth) {
    int k = int(Ls.n);
    bool leaf = int(depth)>=_MaxOrder;
    if( r<kc ) {
      const byte* c = base+r;
      fwdN[r>>1] = uint(k); fwdStamp[r>>1] = enumStamp;
      if( c[0]<CPATH_LIM ) {
        uint n = pathL(c);
        bool ll = lastIsLeaf(depth, n);
        LNode& L = newNode();
        initNode(L, parent, st, depth, leaf);
        L.off = r; L.cold = 1; L.kind = K_PATH; L.ns = byte(n-1); L.hdr = ll; L.nib = pathNib(c);
        L.sz = word(4*(n-1)+(ll ? 2u : 4u));
        State* s = dsAlloc(1);   // the last element's successor
        s->sym = 0; s->tf = 0; s->succ = ll ? 0 : pathChild(c, r);
        L.sp = s;
        return k;
      }
      LNode& L = newNode();
      initNode(L, parent, st, depth, leaf);
      L.off = r; L.cold = 1;
      uint code, hl = nu2Hdr(c, &code), key = cbKey[code];
      L.code = int(code);
      L.kind = K_MULTI; L.ns = 1; L.hdr = byte(key);
      L.sz = word(recSizeNU(2, leaf));
      State* s = dsAlloc(2);
      L.sp = s;
      s[0].sym = c[hl]; s[0].tf = byte(key>>8); s[1].sym = c[hl+1]; s[1].tf = byte(key>>16);
      s[0].succ = leaf ? 0 : nu2Succ(c, r, hl, code, key, 0, false);
      s[1].succ = leaf ? 0 : nu2Succ(c, r, hl, code, key, 1, false);
      return k;
    }
    LNode& L = newNode();
    initNode(L, parent, st, depth, leaf);
    L.cold = 0;
    L.off = word(r & ~1u);
    fwdN[L.off>>1] = uint(k); fwdStamp[L.off>>1] = enumStamp;
    const byte* h = base+L.off;
    if( r & 1 ) {
      L.kind = K_MULTI; L.ns = h[1]; L.hdr = h[0];
      L.sz = word(recSizeNU(L.ns+1u, leaf));
      if( leaf ) {
        State* s = dsAlloc(L.ns+1u);
        L.sp = s;
        for( uint j = 0; j<=L.ns; j++ ) { s[j].sym = h[2+2*j]; s[j].tf = h[3+2*j]; s[j].succ = 0; }
      } else L.sp = (const State*)(h+2);
    } else if( leaf ) {
      L.kind = K_LBIN; L.sz = 2;
      State* s = dsAlloc(1);
      L.sp = s;
      s->sym = h[0]; s->tf = h[1]; s->succ = 0;
    } else {
      L.kind = (r & 2) ? byte(K_BIN2) : byte(K_BIN0); L.sz = 4;
      L.sp = (const State*)h;
    }
    return k;
  }
  // Breadth-first list of the contexts of page p reachable from its roots through near refs
  // (parents precede their children).
  void enumPage(uint p) {
    PageMeta& m = pm[p];
    const byte* base = pageBase(p);
    uint kc = pgCold[p];
    Ls.n = 0; nDS = 0;
    if( ++enumStamp==0 ) { memset(fwdStamp, 0, sizeof(fwdStamp)); enumStamp = 1; }
    for( size_t k = 0; k<m.roots.size(); k++ ) {
      const GRootEnt& g = groot[m.roots[k]];
      addNode(base, kc, g.ref, -1, uint(k), g.depth);
    }
    nRootNodes = Ls.n;
    for( size_t k = 0; k<Ls.size(); k++ ) {
      uint cd = Ls[k].depth+1u;
      if( Ls[k].kind==K_MULTI ) {
        if( Ls[k].leaf ) continue;
        uint ns = Ls[k].ns;
        const State* sp = Ls[k].sp;
        for( uint j = 0; j<=ns; j++ ) {
          State s = sp[j];
          if( (s.tf & 0x80) && s.succ>=FAR_LIM ) addNode(base, kc, s.succ, int(k), j, cd);
        }
      } else if( (Ls[k].kind==K_BIN0 && Ls[k].next<0) || (Ls[k].kind==K_PATH && !Ls[k].hdr) ) {
        word c = Ls[k].sp[0].succ;
        if( Ls[k].kind==K_PATH ) cd = Ls[k].depth+Ls[k].ns+1u;
        if( c>=FAR_LIM ) { int ck = addNode(base, kc, c, int(k), 0, cd); Ls[k].next = ck; }
      }
    }
  }
  void trimPage(uint p) {
    PageMeta& m = pm[p];
    uint a = (m.bump+4095) & ~4095u, b = (m.hwm+4095) & ~4095u;
    if( b>=a+8192 ) { release_mem(pageBase(p)+a, b-a); setHwm(p, m.bump); }
  }
  // Picks a set of sibling subtrees holding about POL_SPLITNUM/16 of the page.
  uint curPage;
  void selectSplit() {
    std::vector<int>& sel = selBuf;
    std::vector<int>& kids = kidBuf;
    sel.clear();
    size_t n = Ls.size();
    uint total = 0;
    for( size_t k = n; k-->0; ) {
      Ls[k].sub += Ls[k].sz;
      total += Ls[k].sz;
      if( Ls[k].parent>=0 ) Ls[Ls[k].parent].sub += Ls[k].sub;
    }
    uint target = total*POL_SPLITNUM/16, cap = target+target/4;
    int cur = -1;
    for( ;; ) {
      kids.clear();
      for( size_t k = (cur<0 ? 0 : cur+1); k<n; k++ ) if( Ls[k].parent==cur ) kids.push_back(int(k));
      int best = -1;
      for( size_t k = 0; k<kids.size(); k++ ) if( best<0 || Ls[kids[k]].sub>Ls[best].sub ) best = kids[k];
      if( best>=0 && Ls[best].sub>cap && Ls[best].sub>Ls[best].sz ) { cur = best; continue; }
      break;
    }
    // largest first, while they fit under the cap
    for( size_t a = 0; a<kids.size(); a++ )
      for( size_t b = a+1; b<kids.size(); b++ )
        if( Ls[kids[b]].sub>Ls[kids[a]].sub ) { int t = kids[a]; kids[a] = kids[b]; kids[b] = t; }
    uint sum = 0;
    // every moved non-root subtree needs a far-table entry in this page
    int farLeft = int(FAR_LIM)-int(farUsedCount(curPage))-16;
    for( size_t a = 0; a<kids.size(); a++ ) {
      if( sum+Ls[kids[a]].sub>cap ) continue;
      if( Ls[kids[a]].parent>=0 ) { if( farLeft<=0 ) continue; farLeft--; }
      sel.push_back(kids[a]);
      sum += Ls[kids[a]].sub;
    }
  }
  uint farUsedCount(uint p) const { return uint(pm[p].fars.size()-pm[p].farFree.size()); }
  // group start offsets for byte totals b[kind], from start; returns the end
  static uint layout(uint* o, const uint* b, uint start) {
    o[K_MULTI] = start;
    uint e = start+b[K_MULTI];
    o[K_BIN0] = (e+3) & ~3u; e = o[K_BIN0]+b[K_BIN0];
    o[K_LBIN] = e; e += b[K_LBIN];
    if( b[K_BIN2] ) { o[K_BIN2] = (e & 2) ? e : e+2; e = o[K_BIN2]+b[K_BIN2]; } else o[K_BIN2] = e;
    return e;
  }
  // number of far refs held by node k
  uint farRefs(const LNode& L) const {
    if( L.leaf ) return 0;
    if( L.kind==K_MULTI ) {
      uint c = 0;
      for( uint j = 0; j<=L.ns; j++ ) if( (L.sp[j].tf & 0x80) && L.sp[j].succ<FAR_LIM ) c++;
      return c;
    }
    if( L.kind==K_PATH ) return (!L.hdr && L.next<0) ? 1u : 0u;
    return (L.kind==K_BIN0 && L.next<0 && L.sp[0].succ<FAR_LIM) ? 1u : 0u;
  }
  // replaces cold path node k by its elements (the head keeps index k); all inherit moved/pin
  void explodePath(const byte* base, int k) {
    LNode& H = Ls[k];
    const byte* c = base+H.off;
    uint n = H.ns+1u;
    bool ll = H.hdr;
    int child = H.next;
    word endref = H.sp[0].succ;
    State* s = dsAlloc(n);
    int prev = -1;
    for( uint e = 0; e<n; e++ ) {
      int idx = e==0 ? k : int(Ls.n);
      LNode& E = e==0 ? H : newNode();
      if( e>0 ) {
        initNode(E, prev, 0, H.depth+e, int(H.depth+e)>=_MaxOrder);
        E.off = NOOFF; E.cold = 2; E.moved = H.moved; E.pin = H.pin;
        Ls[prev].next = idx;
      }
      bool lb = (e==n-1) && ll;
      E.kind = lb ? byte(K_LBIN) : byte(K_BIN0);
      E.sz = lb ? 2 : 4;
      E.ns = 0; E.hdr = 0;
      E.sp = s+e;
      s[e].sym = c[1+e]; s[e].tf = byte(pathFreq(c, n, e)); s[e].succ = (e+1<n) ? 0 : endref;
      prev = idx;
    }
    Ls[prev].next = child;
    if( child>=0 ) Ls[child].parent = prev;
  }
  // new value of a child pointer v of node par (ck: the child's node, -1 for a far ref)
  uint rp_p, rp_q;
  inline word remapChild(const LNode& par, word v, int ck) {
    if( ck>=0 && !Ls[ck].moved ) return Ls[ck].nref;
    return remapSlow(par, v, ck);
  }
  word remapSlow(const LNode& par, word v, int ck) {
    if( ck<0 ) {
      if( par.moved ) { uint gid = pm[rp_p].fars[v]; freeFar(rp_p, v); return allocFar(rp_q, gid); }
      farUsed[v] = 1;
      return v;
    }
    const LNode& C = Ls[ck];
    if( par.moved || !C.moved ) return C.nref;
    word jf = allocFar(rp_p, C.gid);   // child is a moved subtree root: becomes a far ref
    farUsed[jf] = 1;
    return jf;
  }
  // Rebuilds page p: [cold records][multi][binary ≡0][leaf binary][binary ≡2] from RESV. With
  // doSplit, a set of sibling subtrees first moves (in hot form) to a receiver page. Only
  // SuffCache[0..order] and parentSlot[1..order] are live transient references; the records
  // holding them stay hot.
  bool Repack(uint p, bool doSplit) {
    removeDeadRoots(p);
    enumPage(p);
    size_t n = Ls.size();
    uint q = DEAD_PAGE;
    std::vector<int>& sel = selBuf;
    sel.clear();
    if( doSplit ) {
      curPage = p;
      selectSplit();
      if( sel.empty() ) doSplit = false;
    }
    if( doSplit ) {
      uint selBytes = 0;
      for( size_t t = 0; t<sel.size(); t++ ) selBytes += Ls[sel[t]].sub;
      for( size_t t = 0; t<sel.size(); t++ ) Ls[sel[t]].moved = 1;
      for( size_t k = 0; k<n; k++ ) if( Ls[k].parent>=0 && Ls[Ls[k].parent].moved ) Ls[k].moved = 1;
      uint movedFar = uint(sel.size());
      for( size_t k = 0; k<n; k++ ) if( Ls[k].moved ) movedFar += farRefs(Ls[k]);
      if( recvPage!=DEAD_PAGE && recvPage!=p && recvPage<nPages && pm[recvPage].bump+selBytes+POL_RECVGAP<=PG_SIZE
          && farUsedCount(recvPage)+movedFar+16<=FAR_LIM ) q = recvPage;
      else {
        q = newPage();
        if( q==DEAD_PAGE ) return false;
        recvPage = q;
      }
    }
    rp_p = p; rp_q = q;
    PageMeta& P = pm[p];
    byte* bp = pageBase(p);
    byte* bq = doSplit ? pageBase(q) : 0;
    // transient references: their records stay hot
    int slotNode[MAX_O+2]; uint slotDelta[MAX_O+2];
    for( int i = 0; i<=order; i++ ) {
      if( pageOf(crec(SuffCache[i]))==p ) Ls[fwdN[offOf(crec(SuffCache[i]))>>1]].pin = 1;
    }
    for( int i = 1; i<=order; i++ ) {
      slotNode[i] = -1;
      byte* a = (byte*)parentSlot[i];
      if( a<bp || a>=bp+PG_SIZE ) continue;
      uint so_ = offOf(a);
      // the record holding the slot starts at most 1026 bytes below it
      size_t k = n;
      for( uint o = so_ & ~1u; o>=RESV && o+1100>so_; o -= 2 ) {
        if( fwdStamp[o>>1]!=enumStamp ) continue;
        const LNode& C = Ls[fwdN[o>>1]];
        if( C.cold==0 && C.off==o && so_<uint(C.off)+C.sz ) { k = fwdN[o>>1]; break; }
      }
      if( k==n ) { fprintf(stderr, "\nfatal: parent slot not in a live record\n"); exit(7); }
      slotNode[i] = int(k); slotDelta[i] = so_-Ls[k].off;
      Ls[k].pin = 1;
    }
    // output formats (pass 1, forward). Cold records stay as they are unless they are current or
    // move to the receiver.
    for( size_t k = 0; k<Ls.n; k++ ) {
      LNode& L = Ls[k];
      if( L.cold==1 ) {
        if( L.kind==K_PATH ) {
          if( L.pin || L.moved ) explodePath(bp, int(k));
          else { L.fmt = F_KEEP; continue; }
        } else if( L.pin || L.moved ) { cbRelease(uint(L.code)); L.code = -1; }
        else { L.fmt = F_CNU2; continue; }
      }
      if( L.moved || L.fmt!=F_HOT ) continue;
      if( L.kind==K_MULTI ) {
        if( L.ns!=1 || L.pin ) continue;
        L.fmt = F_CNU2;      // its code is chosen in pass 2, with its adjacent child
        L.code = -1;
        continue;
      }
      if( L.kind!=K_BIN0 ) continue;
      // chain head: not the continuation of a binary parent's chain
      if( L.parent>=0 && Ls[L.parent].kind==K_BIN0 && Ls[L.parent].next==int(k) ) continue;
      // segments of unpinned elements, at most PATH_MAXL long
      int cur = int(k);
      while( cur>=0 ) {
        int h = cur, m = 0, last = -1;
        while( cur>=0 && !Ls[cur].pin && !Ls[cur].moved && (Ls[cur].kind==K_BIN0 || Ls[cur].kind==K_LBIN) && m<PATH_MAXL ) {
          last = cur; m++;
          cur = (Ls[cur].kind==K_BIN0) ? Ls[cur].next : -1;
        }
        if( m>=2 ) {
          Ls[h].fmt = F_CPATH; Ls[h].ns = byte(m-1); Ls[h].tail = last;
          { uint mx = 0; for( int e = h; ; e = Ls[e].next ) { if( Ls[e].sp[0].tf>mx ) mx = Ls[e].sp[0].tf; if( e==last ) break; } Ls[h].nib = mx<=16; }
          for( int e = Ls[h].next; e!=Ls[last].next; e = Ls[e].next ) Ls[e].fmt = F_INNER;
         
        }
        if( m==0 ) {
          // a pinned or non-chain element: skip it
          if( cur<0 ) break;
          const LNode& C = Ls[cur];
          if( C.moved || !(C.kind==K_BIN0 || C.kind==K_LBIN) ) break;
          cur = (C.kind==K_BIN0) ? C.next : -1;
        }
      }
    }
    n = Ls.size();
    // pass 2, children first: a path whose end child is cold stores that child right after itself
    // (no end ref), which also makes a lone binary with a cold child a path of 1. Sizes of the
    // groups.
    uint sb[4] = {0, 0, 0, 0}, mb[4] = {0, 0, 0, 0};
    uint cb = 0;   // cold bytes
    for( size_t k = n; k-->0; ) {
      LNode& L = Ls[k];
      {
        int c = -1;
        if( L.fmt==F_CPATH ) c = Ls[L.tail].kind==K_LBIN ? -1 : Ls[L.tail].next;
        else if( L.fmt==F_KEEP ) c = L.hdr ? -1 : L.next;
        else if( L.fmt==F_HOT && L.kind==K_BIN0 && !L.pin && !L.moved ) c = L.next;
        if( c>=0 && isColdFmt(Ls[c].fmt) && Ls[c].parent==(L.fmt==F_CPATH ? L.tail : int(k)) ) {
          if( L.fmt==F_HOT ) { L.fmt = F_CPATH; L.ns = 0; L.tail = int(k); L.nib = 0; }
          L.adj = 1;
        }
      }
      if( L.fmt==F_CNU2 ) {
        // NU=2: the first context child that is cold follows the record; then its code
        uint adj = 0;
        L.next = -1;
        if( !L.leaf ) {
          for( uint j = 0; j<2 && !adj; j++ ) {
            if( !(L.sp[j].tf & 0x80) || L.sp[j].succ<FAR_LIM ) continue;
            int c = int(fwdN[L.sp[j].succ>>1]);
            if( isColdFmt(Ls[c].fmt) && Ls[c].parent==int(k) && !Ls[c].moved ) { adj = j+1; L.next = c; }
          }
        }
        uint key = L.hdr | (uint(L.sp[0].tf)<<8) | (uint(L.sp[1].tf)<<16) | (adj<<24);
        if( L.code<0 || cbKey[L.code]!=key ) {
          if( L.code>=0 ) cbRelease(uint(L.code));
          L.code = cbCode(key);
          if( L.code>=0 ) cbRef[L.code]++;
        }
        if( L.code<0 ) { L.fmt = F_HOT; L.next = -1; adj = 0; }
        L.adj = byte(adj);
      }
      if( L.fmt==F_HOT ) (L.moved ? mb : sb)[L.kind] += L.sz;
      else if( isColdFmt(L.fmt) ) cb += coldNodeSize(L);
    }
    // pass 3: offsets. Cold records first (an adjacent child right after its parent), then the
    // hot groups.
    uint kc = (RESV+cb+1) & ~1u;
    uint so[4], mo[4], send, mend = 0;
    send = layout(so, sb, kc);
    if( doSplit ) {
      mend = layout(mo, mb, pm[q].bump);
      if( mend>PG_SIZE ) { fprintf(stderr, "\nfatal: receiver overflow\n"); exit(7); }
      if( mend>pm[q].hwm ) setHwm(q, mend);
    }
    if( send>PG_SIZE ) { fprintf(stderr, "\nfatal: repack overflow\n"); exit(7); }
    uint co = RESV;
    for( size_t k = 0; k<n; k++ ) {
      LNode& L = Ls[k];
      if( L.fmt==F_HOT ) {
        uint* o = L.moved ? mo : so;
        L.nref = word(o[L.kind] | (L.kind==K_MULTI ? 1u : 0u));
        o[L.kind] += L.sz;
        continue;
      }
      if( !isColdFmt(L.fmt) || L.placed ) continue;
      int j = int(k);
      for( ;; ) {
        LNode& C = Ls[j];
        if( C.placed ) { fprintf(stderr, "\nfatal: cold record placed twice\n"); exit(7); }
        C.placed = 1;
        C.nref = word(co);
        co += coldNodeSize(C);
        if( !C.adj ) break;
        j = C.fmt==F_CPATH ? Ls[C.tail].next : C.next;   // F_KEEP and F_CNU2 keep it in next
      }
    }
    if( co!=RESV+cb ) { fprintf(stderr, "\nfatal: cold layout mismatch\n"); exit(7); }
    // global ids of the moved subtree roots
    for( size_t t = 0; t<sel.size(); t++ ) {
      LNode& s = Ls[sel[t]];
      uint gid;
      if( s.parent<0 ) {
        gid = P.roots[s.st];
        groot[gid].page = q; groot[gid].ref = s.nref;
        P.roots[s.st] = NO_ROOT;
      } else gid = newGRoot(q, s.nref, s.depth);
      pm[q].roots.push_back(gid);
      s.gid = gid;
    }
    memset(farUsed, 0, P.fars.size());
    uint stayBytes = cb, movedBytes = 0;
    for( size_t k = 0; k<n; k++ ) {
      const LNode& L = Ls[k];
      if( L.fmt==F_INNER ) continue;
      const State* ss = L.sp;
      if( L.fmt==F_CNU2 ) {
        byte* d = scratch+L.nref;
        uint hl = nu2PutHdr(d, uint(L.code));
        d[hl] = ss[0].sym; d[hl+1] = ss[1].sym;
        if( !L.leaf ) {
          byte* o = d+hl+2;
          for( uint j = 0; j<2; j++ ) {
            if( L.adj==j+1 ) continue;
            word v = ss[j].succ;
            if( ss[j].tf & 0x80 ) v = remapChild(L, v, v>=FAR_LIM ? int(fwdN[v>>1]) : -1);
            wput(o, v); o += 2;
          }
        }
        continue;
      }
      if( L.fmt==F_KEEP ) {
        byte* d = scratch+L.nref;
        uint m = L.ns+1u, fb = pathFB(m, L.nib);
        memcpy(d+1, bp+L.off+1, m+fb);
        d[0] = byte((m-1) | (L.adj ? uint(CPATH_ADJ) : 0u) | (L.nib ? uint(CPATH_NIB) : 0u));
        if( !L.hdr && !L.adj ) wput(d+1+m+fb, remapChild(L, ss[0].succ, L.next));
        continue;
      }
      if( L.fmt==F_CPATH ) {
        byte* d = scratch+L.nref;
        uint m = L.ns+1u, fb = pathFB(m, L.nib);
        d[0] = byte((m-1) | (L.adj ? uint(CPATH_ADJ) : 0u) | (L.nib ? uint(CPATH_NIB) : 0u));
        if( L.nib ) memset(d+1+m, 0, fb);
        int e = int(k), last = int(k);
        for( uint t = 0; t<m; t++ ) {
          d[1+t] = Ls[e].sp[0].sym;
          if( L.nib ) d[1+m+(t>>1)] |= byte((Ls[e].sp[0].tf-1u)<<(4*(t & 1)));
          else d[1+m+t] = Ls[e].sp[0].tf;
          last = e; e = Ls[e].next;
        }
        if( Ls[last].kind!=K_LBIN && !L.adj ) wput(d+1+m+fb, remapChild(Ls[last], Ls[last].sp[0].succ, Ls[last].next));
        continue;
      }
      // hot
      byte* d = (L.moved ? bq : scratch)+(L.nref & ~1u);
      if( L.moved ) movedBytes += L.sz; else stayBytes += L.sz;
      if( L.kind==K_MULTI ) {
        d[0] = L.hdr; d[1] = L.ns;
        if( L.leaf ) {
          for( uint j = 0; j<=L.ns; j++ ) { d[2+2*j] = ss[j].sym; d[3+2*j] = ss[j].tf; }
        } else {
          State* ds = (State*)(d+2);
          for( uint j = 0; j<=L.ns; j++ ) {
            ds[j] = ss[j];
            if( ss[j].tf & 0x80 ) ds[j].succ = remapChild(L, ss[j].succ, ss[j].succ>=FAR_LIM ? int(fwdN[ss[j].succ>>1]) : -1);
          }
        }
      } else if( L.kind==K_LBIN ) {
        d[0] = ss[0].sym; d[1] = ss[0].tf;
      } else {
        State* ds = (State*)d;
        *ds = ss[0];
        if( L.kind==K_BIN0 ) ds->succ = remapChild(L, ss[0].succ, L.next);
      }
    }
    // remap the transient references into page p
    for( int i = 0; i<=order; i++ ) {
      byte* r = crec(SuffCache[i]);
      if( pageOf(r)!=p ) continue;
      const LNode& L = Ls[fwdN[offOf(r)>>1]];
      SuffCache[i] = mkCtxL((L.moved ? bq : bp)+(L.nref & ~1u), L.nref & 1u, L.leaf);
    }
    for( int i = 1; i<=order; i++ ) {
      if( slotNode[i]<0 ) continue;
      const LNode& L = Ls[slotNode[i]];
      uint nofs = (L.nref & ~1u)+slotDelta[i];
      if( L.moved ) parentSlot[i] = (word*)(bq+nofs);
      else {
        word v = *(word*)(scratch+nofs);
        parentSlot[i] = (v<FAR_LIM) ? &groot[P.fars[v]].ref : (word*)(bp+nofs);
      }
    }
    if( send>P.hwm ) setHwm(p, send);
    memcpy(bp+RESV, scratch+RESV, send-RESV);
    for( size_t k = 0; k<nRootNodes; k++ ) {   // the roots are the first nodes
      if( Ls[k].moved ) continue;
      groot[P.roots[Ls[k].st]].ref = Ls[k].nref;
    }
    removeDeadRoots(p);
    live_total += qword(stayBytes)+movedBytes-P.live;
    P.live = stayBytes;
    P.bump = send;
    P.cdead = 0;
    pgCold[p] = word(kc);
    memset(P.freeHead, 0, sizeof(P.freeHead)); P.fmask = 0;
    for( size_t j = 0; j<P.fars.size(); j++ ) {
      if( P.fars[j]!=NO_ROOT && !farUsed[j] ) {
        groot[P.fars[j]].page = DEAD_PAGE;   // subtree dropped by rescale: unreachable
        freeFar(p, word(j));
      }
    }
    trimPage(p);
    updSlack(p);
    if( doSplit ) {
      PageMeta& Q = pm[q];
      Q.live += movedBytes;
      if( mend>Q.bump ) Q.bump = mend;
      if( Q.bump>Q.hwm ) setHwm(q, Q.bump);
      updSlack(q);
    }
    return true;
  }
  uint needFor(Ctx c, int i) {
    if( isC(c) ) return coldHotSize(crec(c), uint(i))+recSizeNU(3, false)+12u;
    uint NU = NS(c)+1;
    return (NU<256 ? recSizeNU(NU+1, false) : 0u) + 12u;
  }
  void expandCurrent() {
    for( int i = 1; i<=order; i++ ) if( isC(SuffCache[i]) ) expandCold(i);
  }
  // Makes sure every page holding a current context can absorb this step's allocations, then
  // expands the cold current contexts.
  bool EnsureHeadroom() {
    {
      // fast path: every page can absorb the step's worst case for all current contexts together
      uint sum = 0;
      for( int i = 0; i<=order; i++ ) sum += needFor(SuffCache[i], i);
      int i = 0;
      while( i<=order && pgSlack[pageOf(crec(SuffCache[i]))]>=sum+64 ) i++;
      if( i>order ) { expandCurrent(); return true; }
      // the page tests below with the total need of all pages (an upper bound of each page's)
      for( ; i<=order; i++ ) {
        const PageMeta& m = pm[pageOf(crec(SuffCache[i]))];
        if( PG_SIZE-m.bump<sum+64 ) break;
        if( m.bump+sum>((m.hwm+4095) & ~4095u) ) {
          uint dead = m.bump-RESV-m.live;
          uint cfree = m.cdead<(m.live>>POL_CDEADSH) ? m.cdead : (m.live>>POL_CDEADSH);
          if( dead-cfree>(m.live>>POL_DEADSH)+256 ) break;
        }
      }
      if( i>order ) { expandCurrent(); return true; }
    }
    for( int iter = 0; iter<64; iter++ ) {
      uint pg[MAX_O+2], nd[MAX_O+2]; int np = 0;
      for( int i = 0; i<=order; i++ ) {
        uint p = pageOf(crec(SuffCache[i])), n = needFor(SuffCache[i], i);
        int k = 0;
        while( k<np && pg[k]!=p ) k++;
        if( k==np ) { pg[np] = p; nd[np] = 0; np++; }
        nd[k] += n;
      }
      int bad = -1; bool doSplit = false;
      for( int k = 0; k<np; k++ ) {
        PageMeta& m = pm[pg[k]];
        uint freeb = PG_SIZE-m.bump;
        uint dead = m.bump-RESV-m.live;
        uint cfree = m.cdead<(m.live>>POL_CDEADSH) ? m.cdead : (m.live>>POL_CDEADSH);
        bool tight = freeb<nd[k]+64;
        bool wasteful = (m.bump+nd[k]>((m.hwm+4095) & ~4095u)) && dead-cfree>(m.live>>POL_DEADSH)+256;
        if( tight || wasteful ) {
          bad = k;
          doSplit = (m.live+nd[k]+POL_MARGIN>USABLE);
          break;
        }
      }
      if( bad<0 ) { Order0 = rootCtx(0); expandCurrent(); return true; }
      if( !Repack(pg[bad], doSplit) ) return false;
    }
    fprintf(stderr, "\nfatal: EnsureHeadroom did not converge\n");
    exit(7);
  }

  // ---------------- model ----------------
  static uint hasTextSym(Ctx pc) {
    const byte* s0 = (const byte*)S0(pc);
    uint ns = NS(pc), sh = SH(pc);
    for( uint j = 0; j<=ns; j++ ) if( s0[j<<sh]>=0x40 ) return F_HasText;
    return 0;
  }
  void CacheNumstatsAndFlags(int order) {
    uint nextBit = 0;
    if( pText>WinBeg && pText[-1]>=0x40 ) nextBit = F_NextIsText;
    for( int i = 0; i<=order; i++ ) {
      Ctx pc = SuffCache[i];
      uint ns = NS(pc);
      NumStats_Cache[i] = ns;
      uint hasText = hasTextSym(pc);
      uint rescaledBit = Resc(pc) ? F_Rescaled : 0;
      Flags_Cache[i] = rescaledBit+hasText+(i>0 ? nextBit : 0);
    }
  }
  // parentSlot[t] points at the field that references SuffCache[t] (a succ field in the parent's
  // record, or a root-table entry). These keep it valid when states move inside a record or a
  // record is reallocated (the reference re-derives it with a full scan, refresh_parent_cache).
  void slotSwap(State* x, State* y) {
    word* px = &x->succ;
    word* py = &y->succ;
    for( int t = 1; t<=hwm; t++ ) {
      if( parentSlot[t]==px ) parentSlot[t] = py;
      else if( parentSlot[t]==py ) parentSlot[t] = px;
    }
  }
  void slotMove(State* oldS, uint n, State* newS) {
    byte* lo = (byte*)oldS;
    byte* hi = (byte*)(oldS+n);
    for( int t = 1; t<=hwm; t++ ) {
      byte* s = (byte*)parentSlot[t];
      if( s>=lo && s<hi ) parentSlot[t] = (word*)((byte*)newS+(s-lo));
    }
  }
  void slotPermute(State* s0, uint n, const byte* newIdxOfOld) {
    byte* lo = (byte*)s0;
    byte* hi = (byte*)(s0+n);
    for( int t = 1; t<=hwm; t++ ) {
      byte* s = (byte*)parentSlot[t];
      if( s>=lo && s<hi ) parentSlot[t] = &s0[newIdxOfOld[(s-lo)>>2]].succ;
    }
  }
  void CacheSuccessors(uint sym) {
    SuffCache[0] = Order0;
    parentSlot[0] = 0;
    for( int i = order-1; i>=0; i-- ) {
      if( StateCache[i]==0 ) {
        StateCache[i] = FindState(SuffCache[i], byte(sym));
        StateCacheCtx[i] = SuffCache[i];
      }
      if( StateCache[i]==0 ) { order = i; return; }
      Ctx sctx = StateCacheCtx[i];
      State* st = StateCache[i];
      if( !succIsCtx(sctx, st) ) { order = i; return; }
      word* slot;
      SuffCache[i+1] = child(sctx, st, &slot, i+1);
      parentSlot[i+1] = slot;
    }
  }
  uint Init(uint MaxOrder, uint MMAX, uint ResetPerc, uint WinPerc, uint WinSize, uint filesize) {
    _MaxOrder = MaxOrder;
    _ResetPerc = (int)ResetPerc;
    _filesize = filesize;
    if( WinSize!=0 ) {
      _WinSize = WinSize;
    } else if( WinPerc==0 ) {
      _WinSize = filesize+1U;
    } else {
      _WinSize = uint(qword(filesize)*qword(WinPerc)/100U)+1U;
    }
    if( _WinSize<256U ) _WinSize = 256U;
    CZK = 0;
    while( (qword(_WinSize)>>CZK)>=0xFFFFu ) CZK++;
    // window (+ padding for the vector scan), 2 MB aligned for transparent huge pages
    WinMapSize = qword(_WinSize)+(4u<<20);
    WinMap = (byte*)map_mem(WinMapSize);
    if( WinMap==0 ) return 1;
    WinBeg = (byte*)(((uintptr_t)WinMap+(2u<<20)-1) & ~(uintptr_t)((2u<<20)-1));
#if defined(MADV_HUGEPAGE)
    madvise(WinBeg, qword(_WinSize), MADV_HUGEPAGE);
#endif
    WinEnd = WinBeg+_WinSize;
    // arena of 64 KB pages
    maxPages = uint((qword(MMAX)<<20)>>PG_BITS);
    if( maxPages<4 ) maxPages = 4;
    arenaMapSize = (qword(maxPages)+1)<<PG_BITS;
    arenaMap = (byte*)map_mem(arenaMapSize, false);
    if( arenaMap==0 ) return 1;
    arena = (byte*)(((uintptr_t)arenaMap+PG_SIZE-1) & ~(uintptr_t)(PG_SIZE-1));
    pm.reserve(maxPages);
    pgSlack.assign(maxPages, 0);
    pgCold.assign(maxPages, RESV);
    cbHash.assign(size_t(1)<<CBH_BITS, 0);
    cbKey.reserve(CB_MAX);
    maxGroot = 1u<<25;
    groot = (GRootEnt*)map_mem(qword(maxGroot)*sizeof(GRootEnt));
    if( groot==0 ) return 1;
    scratch = (byte*)malloc(PG_SIZE);
    Ls.a = (LNode*)malloc(sizeof(LNode)*DS_MAX); Ls.n = 0;
    nPages = 0;
    StartModelRare();
    return 0;
  }
  void Quit(void) {
    unmap_mem(WinMap, WinMapSize);
    unmap_mem(arenaMap, arenaMapSize);
    unmap_mem(groot, qword(maxGroot)*sizeof(GRootEnt));
    free(scratch);
    free(Ls.a);
  }
  void StartModelRare(void) {
    int i;
    OrderFall = _MaxOrder;
    // drop the whole tree
    if( nPages ) release_mem(arena, qword(nPages)<<PG_BITS);
    for( size_t p = 0; p<pm.size(); p++ ) pm[p].hwm = RESV;   // memory released above
    nPages = 0;
    touched_total = 0;
    freePages.clear();
    nGroot = 0;
    live_total = 0;
    cbReset();
    recvPage = DEAD_PAGE;
    pText = WinBeg;
    see.initialize(_MaxOrder);
    uint p = newPage();
    byte* r = allocRec(p, recSizeNU(256, false), ANYPAR);
    Order0 = mkCtx(r, 1);
    r[0] = 1;       // EscFreq 1, not rescaled
    r[1] = 255;     // NumStats
    State* s0 = S0(Order0);
    for( i = 0; i<256; i++ ) { s0[i].sym = byte(i); s0[i].tf = 0; s0[i].succ = NULL_SUCC; }
    uint g = newGRoot(p, refOf(Order0), 0);
    pm[p].roots.push_back(g);
    maxorder = order = 0;
    SuffCache[0] = Order0;
    parentSlot[0] = 0;
    hwm = 0;
  }
  enum {
    RESTORE_ALLOC_NUM = 3,
    RESTORE_ALLOC_DEN = 4,
    RESTORE_MAX_ATTEMPTS = 16
  };
  void RestoreModelRare(void) {
    uint used = uint(pText-WinBeg);
    uint keep = uint(qword(used)*qword(_ResetPerc)/100);
    qword max_alloc = qword(maxPages)*USABLE*RESTORE_ALLOC_NUM/RESTORE_ALLOC_DEN;
    FakeRangecoder frc;
    int attempt = 0;
    while( keep>0 && attempt<RESTORE_MAX_ATTEMPTS ) {
      uint shift = used-keep;
      if( shift>0 ) memmove(WinBeg, WinBeg+shift, keep);
      fprintf(stderr, "\nrestore attempt %d: keep=%u (used=%u, shift=%u)\n", attempt, keep, used, shift);
      fflush(stderr);
      StartModelRare();
      m_replay = true;
      m_replay_aborted = false;
      uint i = 0;
      for( ; i<keep; i++ ) {
        ProcessByte(uint(WinBeg[i]), frc);
        if( m_replay_aborted || live_total>max_alloc ) break;
      }
      m_replay = false;
      if( i==keep ) {
        fprintf(stderr, "restore done: alloc_size=%llu pText=+%u/%u\n", (unsigned long long)live_total, uint(pText-WinBeg), keep);
        fflush(stderr);
        return;
      }
      fprintf(stderr, "restore retry: alloc_size=%llu hit %llu cap at byte %u/%u\n", (unsigned long long)live_total, (unsigned long long)max_alloc, i, keep);
      fflush(stderr);
      used = keep;
      keep = uint(qword(keep)*qword(_ResetPerc)/100);
      attempt++;
    }
    fprintf(stderr, "restore gave up: dropping window entirely\n");
    fflush(stderr);
    StartModelRare();
  }
  void Reset(const char* why) {
    fprintf(stderr, " [reset%s]\n", why), fflush(stderr);
    if( _ResetPerc>=1 && _ResetPerc<=99 ) RestoreModelRare();
    else StartModelRare();
  }

  // ---- context updates ----
  State* UpdateSuffixFreq(Ctx pc, uint FSymbol, uint FFreq) {
    State* s0 = S0(pc);
    if( NS(pc)!=0 ) {
      uint i = 0;
      if( s0[0].sym!=FSymbol ) {
        for( i = 1; s0[i].sym!=FSymbol; i++ );
        if( MF(s0[i])>=MF(s0[i-1]) ) { slotSwap(&s0[i], &s0[i-1]); swapS(s0[i], s0[i-1]); i--; }
      }
      if( MF(s0[i])<MAX_FREQ-3 ) { uint cf = 2+(FFreq<28); MaddF(s0[i], cf); }
      return &s0[i];
    } else {
      s0[0].tf += (s0[0].tf<14);
      return &s0[0];
    }
  }
  State* UpdateLowerSuffix(Ctx pc, byte sym, uint suff_ns) {
    State* s0 = S0(pc);
    if( NS(pc)!=0 ) {
      uint i = 0;
      while( s0[i].sym!=sym ) i++;
      byte tmp = 2*(MF(s0[i])<MAX_FREQ-1);
      MaddF(s0[i], tmp);
      return &s0[i];
    } else {
      s0[0].tf += !((suff_ns>0)&(s0[0].tf<16));
      return &s0[0];
    }
  }
  uint BequeathFreq(Ctx pc, byte sym1) {
    State* s0 = S0(pc);
    uint ns = NS(pc);
    if( ns!=0 ) {
      uint sf = SummFreq(pc);
      uint cf = MF(*FindState(pc, sym1))-1;
      uint sc = 1+sf-(ns+1)-cf;
      sc <<= 7;
      cf = 1+((314*cf<sc) ? (1536*cf>sc) : 2+(cf*175)/sc);
      return byte(cf);
    }
    return s0[0].tf;
  }
  struct RU { byte sym; byte T; word succ; int f; byte orig; };
  void rescaleStore(Ctx ctx, const RU* u, uint ns) {
    if( isL(ctx) ) {
      for( uint k = 0; k<=ns; k++ ) { State* sk = SI(ctx, k); sk->sym = u[k].sym; sk->tf = byte((u[k].f>0 ? u[k].f : 1)-1); }
      return;
    }
    State* s0 = S0(ctx);
    for( uint k = 0; k<=ns; k++ ) { s0[k].sym = u[k].sym; s0[k].tf = byte(u[k].T | ((u[k].f>0 ? u[k].f : 1)-1)); s0[k].succ = u[k].succ; }
    byte ni[256]; for( uint k = 0; k<=ns; k++ ) ni[u[k].orig] = byte(k);
    slotPermute(s0, ns+1, ni);
  }
  // rescale of a multi context; mirrors the reference exactly (operates on an unpacked copy)
  uint rescale(Ctx ctx, int OrderFall, State*& FoundState) {
    typedef RU U;
    U u[256], tmp;
    int of, i, a, f0, sf_orig, esc_local;
    uint reallocSize = 0;
    clrResc(ctx);
    State* s0 = S0(ctx);
    uint ns = NS(ctx);
    bool leaf = isL(ctx);
    for( uint k = 0; k<=ns; k++ ) {
      const State* sk = SI(ctx, k);
      u[k].sym = sk->sym; u[k].T = sk->tf & 0x80; u[k].succ = leaf ? 0 : sk->succ; u[k].f = MF(*sk); u[k].orig = byte(k);
    }
    {
      uint fs_idx = SIdx(ctx, FoundState);
      if( fs_idx!=0 ) {
        tmp = u[fs_idx];
        for( uint k = fs_idx; k>0; k-- ) u[k] = u[k-1];
        u[0] = tmp;
      }
    }
    of = (OrderFall!=0);
    if( ns==255 ) of = 1;
    f0 = u[0].f;
    sf_orig = EscF(ctx);
    for( uint k = 0; k<=ns; k++ ) sf_orig += u[k].f;
    esc_local = sf_orig-f0;
    u[0].f = (f0+of)>>1;
    uint cur_ns = ns;
    uint p_idx = 0;
    for( i = 0; i<(int)cur_ns; i++ ) {
      p_idx++;
      a = u[p_idx].f;
      esc_local -= a;
      a = (a+of)>>1;
      u[p_idx].f = a;
      if( a>u[p_idx-1].f ) {
        tmp = u[p_idx];
        uint q = p_idx;
        while( q>0 && tmp.f>u[q-1].f ) { u[q] = u[q-1]; q--; }
        u[q] = tmp;
      }
    }
    if( u[p_idx].f==0 ) {
      uint dropped = 0;
      while( u[p_idx].f==0 ) { dropped++; if( p_idx==0 ) break; p_idx--; }
      esc_local += dropped;
      cur_ns -= dropped;
      reallocSize = 1+cur_ns;
      if( cur_ns==0 ) {
        i = (2*u[0].f+esc_local-1)/esc_local;
        u[0].f = i<MAX_FREQ/3 ? i : MAX_FREQ/3;
        rescaleStore(ctx, u, ns);
        FoundState = &s0[0];
        return reallocSize;
      }
    }
    setEscF(ctx, (esc_local+1)>>1);
    a = sf_orig-esc_local-f0;
    if( a>0 ) {
      uint sum = EscF(ctx);
      for( uint k = 0; k<=ns; k++ ) sum += u[k].f;
      a = CLAMP(uint((f0*sum-(sf_orig-esc_local)*u[0].f+a-1)/a), 2U, MAX_FREQ/2U-18U);
    } else
      a = 2;
    u[0].f += a;
    rescaleStore(ctx, u, ns);
    setResc(ctx);
    FoundState = &s0[0];
    return reallocSize;
  }
  Ctx ShrinkContext(Ctx old, uint OldNU, uint NewNU) {
    uint p = pageOf(rec(old));
    State* os = S0(old);
    bool leaf = isL(old);
    Ctx nc;
    if( NewNU==1 ) {
      bool ctxSucc = (os[0].tf & 0x80)!=0;
      byte* r = allocRec(p, recSizeNU(1, leaf), leaf ? uint(ANYPAR) : ctxSucc ? 0u : 2u);
      nc = mkCtxL(r, 0, leaf);
      State* ns0 = S0(nc);
      ns0[0].sym = os[0].sym;
      ns0[0].tf = byte(MF(os[0]));
      if( !leaf ) ns0[0].succ = os[0].succ;
    } else {
      byte* r = allocRec(p, recSizeNU(NewNU, leaf), ANYPAR);
      nc = mkCtxL(r, 1, leaf);
      r[0] = rec(old)[0];
      r[1] = byte(NewNU-1);
      memcpy(r+2, os, NewNU<<SH(old));
    }
    freeRec(rec(old), recSizeNU(OldNU, leaf));
    return nc;
  }
  void FinishRescale(Ctx& ctx, State*& FoundState, uint newNU) {
    uint oldNU = NS(ctx)+1;
    if( newNU<oldNU ) {
      uint idx = SIdx(ctx, FoundState);
      State* oldS = S0(ctx);
      Ctx nc = ShrinkContext(ctx, oldNU, newNU);
      if( !isL(nc) ) slotMove(oldS, newNU, S0(nc));
      FoundState = SI(nc, idx);
      SuffCache[order] = nc;
      if( order>0 && parentSlot[order]!=0 ) *parentSlot[order] = refOf(nc);
      ctx = nc;
    }
  }
  // ExpandContext + UpdateHigherOrder of the reference, fused (a binary freq may not fit 7 bits
  // until UpdateHigherOrder has transformed it).
  void ExpandAndAdd(int i, byte FSymbol, uint FFreq, uint s0_caller, uint ns, word tsucc) {
    Ctx pc = SuffCache[i];
    uint ns1 = NumStats_Cache[i];
    uint OldNU = ns1+1;
    uint p = pageOf(rec(pc));
    bool leaf = isL(pc);
    byte* nr = allocRec(p, recSizeNU(OldNU+1, leaf), ANYPAR);
    Ctx nc = mkCtxL(nr, 1, leaf);
    State* n0 = (State*)(nr+2);
    if( ns1==0 ) {
      State* b = S0(pc);
      uint T = succIsCtx(pc, b) ? 0x80u : 0u;
      uint f = b->tf;
      f = (f<=MAX_FREQ/3) ? (2*f-1) : (MAX_FREQ-15);
      nr[0] = 0; nr[1] = 1;
      n0[0].sym = b->sym; n0[0].tf = byte(T | (f-1));
      if( !leaf ) n0[0].succ = b->succ;
      setEscF(nc, (ns>1)+ExpEscape[see.QTable[see.BSumm>>8]]);
    } else {
      memcpy(nr, rec(pc), recSizeNU(OldNU, leaf));
      nr[1] = byte(OldNU);
      setEscF(nc, EscF(nc)+(see.QTable[ns+4]>>3));
    }
    if( !leaf ) slotMove(S0(pc), OldNU, n0);
    freeRec(rec(pc), recSizeNU(OldNU, leaf));
    if( parentSlot[i] ) *parentSlot[i] = refOf(nc);
    SuffCache[i] = nc;
    uint sumFreq = EscF(nc);
    for( uint k = 0; k<OldNU; k++ ) sumFreq += MF(*SI(nc, k));
    uint cf = (FFreq-1)*(5+sumFreq);
    uint sf = s0_caller+sumFreq;
    if( cf<=3*sf ) {
      cf = 1+(2*cf>sf)+(2*cf>3*sf);
      setEscF(nc, EscF(nc)+4-cf);
    } else {
      cf = 5+(cf>5*sf)+(cf>6*sf)+(cf>8*sf)+(cf>10*sf)+(cf>12*sf);
    }
    State& np = *SI(nc, OldNU);
    np.sym = FSymbol;
    np.tf = byte(cf-1);
    if( !leaf ) np.succ = tsucc;
    StateCache[i] = &np;
    StateCacheCtx[i] = nc;
  }
  byte* textRecover(word cz, uint L) {
    uint cur = uint(pText-WinBeg);
    uint lo = uint(cz)<<CZK, hi = lo+(1u<<CZK);
    if( hi>cur ) hi = cur;
    if( lo<L ) lo = L;
    const byte* pat = pText-L;
    uint pos = scan_avx2(WinBeg, lo, hi, pat, L);
    if( pos==0xFFFFFFFFu ) {
      fprintf(stderr, "\nfatal: text pointer recovery failed (block %u, L=%u, cur=%u)\n", uint(cz), L, cur);
      exit(8);
    }
    return WinBeg+pos;
  }
  template<class RC>
  State* processBinSymbol(Ctx ctx, int symbol, uint SuffNumStats, uint flagsValue, RC &rc) {
    int i, flag;
    State& rs = S0(ctx)[0];
    i = see.NS2BSIndx[SuffNumStats]+see.PrevSuccess+flagsValue+((see.RunLength>>26)&0x20);
    word &bs = see.BinSumm[see.QTable[(rs.tf-1)&0xFF]][i];
    see.BSumm = bs;
    bs -= (see.BSumm+64)>>PERIOD_BITS;
    flag = (rc.f_DEC!=0) ? 0 : rs.sym!=symbol;
    rc.rc_BProcess(see.BSumm+see.BSumm, flag);
    if( flag!=0 ) {
      see.CharMask[rs.sym] = see.EscCount;
      see.NumMasked = 0;
      see.PrevSuccess = 0;
      return NULL;
    } else {
      bs += INTERVAL;
      rs.tf += (rs.tf<196);
      see.RunLength++;
      see.PrevSuccess = 1;
      return &rs;
    }
  }
  template<class RC>
  State* processSymbol1(Ctx ctx, int symbol, RC &rc, int &OrderFall, uint &reallocSize) {
    int cnum, i, low, freq, total, flag, count = 0;
    State* s0 = S0(ctx);
    uint sh = SH(ctx);
#define SX(k) (*(State*)((byte*)s0+((k)<<sh)))
    cnum = NS(ctx);
    i = s0[0].sym;
    low = 0;
    freq = MF(s0[0]);
    total = SummFreq(ctx);
    rc.rc_Arrange(total);
    if( rc.f_DEC!=0 ) {
      count = rc.rc_GetFreq(total);
      flag = count<freq;
    } else {
      flag = i==symbol;
    }
    State* found = 0;
    if( flag!=0 ) {
      see.PrevSuccess = 0;
      MaddF(s0[0], 4);
      found = &s0[0];
    } else {
      see.PrevSuccess = 0;
      for( low = freq, i = 1; i<=cnum; i++ ) {
        freq = MF(SX(i));
        flag = (rc.f_DEC!=0) ? low+freq>count : SX(i).sym==symbol;
        if( flag!=0 ) break;
        low += freq;
      }
      if( flag!=0 ) {
        MaddF(SX(i), 4);
        if( MF(SX(i))>MF(SX(i-1)) ) {
          if( !isL(ctx) ) slotSwap(&SX(i), &SX(i-1));
          swapSt(ctx, &SX(i), &SX(i-1));
          i--;
        }
        found = &SX(i);
      } else {
        freq = total-low;
        see.NumMasked = cnum;
        for( i = 0; i<=cnum; i++ ) see.CharMask[SX(i).sym] = see.EscCount;
        found = 0;
      }
    }
    rc.rc_Process(low, freq, total);
    if( (found!=0)&&(MF(*found)>MAX_FREQ) ) reallocSize = rescale(ctx, OrderFall, found);
    return found;
  }
  template<class RC>
  State* processSymbol2(Ctx ctx, int symbol, uint SuffNumStats, uint flagsValue, RC &rc, int &OrderFall, uint &reallocSize) {
    byte px[256];
    int c, count = 0, low, see_freq, freq, cnum;
    SEE2_CONTEXT* psee2c;
    int flag, pl;
    int i, j, Total;
    State* s0 = S0(ctx);
    uint sh = SH(ctx);
    cnum = NS(ctx);
    if( cnum!=0xFF ) {
      psee2c = see.SEE2Cont[see.QTable[cnum+3]-4];
      psee2c += (SummFreq(ctx)>uint(10*(cnum+1)));
      psee2c += 2*(2*cnum<int(SuffNumStats+see.NumMasked))+flagsValue;
      see_freq = psee2c->getMean()+1;
    } else {
      psee2c = &see.DummySEE2Cont;
      see_freq = 1;
    }
    flag = 0;
    pl = 0;
    j = 0;
    for( i = 0, low = 0; i<=cnum; i++ ) {
      c = SX(i).sym;
      if( see.CharMask[c]!=see.EscCount ) {
        see.CharMask[c] = see.EscCount;
        low += MF(SX(i));
        if( rc.f_DEC!=0 ) px[j++] = i;
        else if( c==symbol ) flag = 1, j = i, pl = low;
      }
    }
    Total = see_freq+low;
    rc.rc_Arrange(Total);
    if( rc.f_DEC!=0 ) {
      count = rc.rc_GetFreq(Total);
      flag = count<low;
    }
    State* found = 0;
    if( flag!=0 ) {
      if( rc.f_DEC!=0 ) {
        for( low = 0, i = 0; (low += MF(SX(px[i])))<=count; i++ );
        j = px[i];
      } else {
        low = pl;
      }
      found = &SX(j);
      freq = MF(*found);
      if( see_freq>2 ) psee2c->Summ -= see_freq;
      psee2c->update();
      MaddF(*found, 4);
      if( MF(*found)>MAX_FREQ ) reallocSize = rescale(ctx, OrderFall, found);
      see.RunLength = see.InitRL;
      see.EscCount++;
    } else {
      low = Total;
      freq = see_freq;
      see.NumMasked = cnum;
      psee2c->Summ += Total-see_freq;
    }
    rc.rc_Process(low-freq, freq, Total);
    return found;
#undef SX
  }
  template<class RC>
  uint ProcessByte(uint c, RC& rc) {
    Ctx MinContext;
    uint reallocSize;
    if( !EnsureHeadroom() ) {
      if( m_replay ) { m_replay_aborted = true; return c; }
      Reset("");
      if( !EnsureHeadroom() ) { fprintf(stderr, "fatal: no memory after reset\n"); exit(7); }
    }
    CacheNumstatsAndFlags(order);
    maxorder = order;
    MinContext = SuffCache[order];
    reallocSize = 0;
    if( NumStats_Cache[order]!=0 ) {
      FoundState = processSymbol1(MinContext, c, rc, OrderFall, reallocSize);
    } else {
      FoundState = processBinSymbol(MinContext, c, NumStats_Cache[order-1], Flags_Cache[order], rc);
    }
    if( reallocSize!=0 ) {
      FinishRescale(MinContext, FoundState, reallocSize);
      uint hasText = hasTextSym(MinContext);
      uint rescaledBit = Resc(MinContext) ? F_Rescaled : 0;
      Flags_Cache[order] = rescaledBit+hasText+(Flags_Cache[order] & F_NextIsText);
    }
    while( FoundState==0 ) {
      do {
        OrderFall++;
        order--;
        MinContext = SuffCache[order];
      } while( order>0 && NumStats_Cache[order]==(uint)see.NumMasked );
      reallocSize = 0;
      FoundState = processSymbol2(MinContext, c, (order>0 ? NumStats_Cache[order-1] : 0), Flags_Cache[order], rc, OrderFall, reallocSize);
      if( reallocSize!=0 ) {
        FinishRescale(MinContext, FoundState, reallocSize);
        uint hasText = hasTextSym(MinContext);
        Flags_Cache[order] = hasText+(Flags_Cache[order] & F_NextIsText);
      }
    }
    if( maxorder+1>hwm ) hwm = maxorder+1;
    if( rc.f_DEC!=0 ) c = FoundState->sym;
    *pText++ = byte(c);
    memset(StateCache, 0, sizeof(StateCache[0])*(hwm+2));
    memset(StateCacheCtx, 0, sizeof(StateCacheCtx[0])*(hwm+2));
    StateCache[order] = FoundState;
    StateCacheCtx[order] = MinContext;
    bool fText = succIsText(MinContext, FoundState);
    if( fText && !isL(MinContext) ) {
      const byte* blk = WinBeg+(uint(FoundState->succ)<<CZK);
      _mm_prefetch((const char*)blk, _MM_HINT_T0);
      _mm_prefetch((const char*)blk+64, _MM_HINT_T0);
      _mm_prefetch((const char*)blk+128, _MM_HINT_T0);
      _mm_prefetch((const char*)blk+192, _MM_HINT_T0);
    }
    // (at order MaxOrder the found state is a leaf state, whose successor is always text)
    UpdateModel(MinContext);
    if( order+1>hwm ) hwm = order+1;
    CacheSuccessors(c);
    if( order+1>hwm ) hwm = order+1;
    if( pText>=WinEnd ) {
      if( m_replay ) { m_replay_aborted = true; return c; }
      Reset(" (text)");
    }
    return c;
  }
  Ctx UpdateModel(Ctx MinContext) {
    byte FSymbol;
    uint ns, s0_caller, FFreq, f_order0;
    State* p = NULL;
    Ctx pc = 0;
    FSymbol = FoundState->sym;
    FFreq = F(MinContext, FoundState);
    bool fNull = succIsNull(MinContext, FoundState);
    bool fText = succIsText(MinContext, FoundState);
    if( order>0 ) {
      pc = SuffCache[order-1];
      p = UpdateSuffixFreq(pc, FSymbol, FFreq);
      StateCache[order-1] = p;
      StateCacheCtx[order-1] = pc;
    }
    if( (OrderFall==0)&&!fNull ) {
      return CreateSuccessors(1, p, pc, MinContext);
    }
    if( isL(MinContext) ) { fprintf(stderr, "\nfatal: leaf context with OrderFall %d\n", OrderFall); exit(9); }
    word iSuccessor = textRef(pText);
    Ctx iF;
    f_order0 = 0;
    if( !fNull ) {
      if( fText ) iF = CreateSuccessors(0, p, pc, MinContext);
      else iF = child(MinContext, FoundState, 0, order+1);
    } else {
      iF = Order0;
      FoundState->tf &= 0x7F;           // null successors only exist in the (multi) order-0 context
      FoundState->succ = iSuccessor;
      OrderFall++;
      f_order0 = 1;
    }
    MinContext = SuffCache[order];      // CreateSuccessors may have relocated a binary MinContext
    s0_caller = SummFreq(MinContext)-FFreq;
    ns = NumStats_Cache[order];
    for( int i = maxorder; i>order; i-- )
      ExpandAndAdd(i, FSymbol, FFreq, s0_caller, ns, iSuccessor);
    --OrderFall;
    order = f_order0 ? 0 : order+1;
    return iF;
  }
  Ctx CreateSuccessors(uint Skip, State* p, Ctx p_ctx, Ctx pc) {
    byte sym = FoundState->sym;
    bool upText = succIsText(pc, FoundState);
    bool upNull = succIsNull(pc, FoundState);
    // a leaf state has no successor field: its text position is recovered through ps[0], the
    // lazy state one order below, which holds the end of the same occurrence (Skip is 1 here)
    bool upLeaf = isL(pc);
    word upVal = upLeaf ? 0 : FoundState->succ;
    Ctx upCtx = (!upText && !upNull) ? child(pc, FoundState, 0, order+1) : 0;
    int i = order;
    State* ps[MAX_O+2];
    Ctx ps_ctx[MAX_O+2];
    int ps_ord[MAX_O+2];
    uint pps_n = 0;
    if( Skip==0 ) {
      ps[pps_n] = FoundState; ps_ctx[pps_n] = pc; ps_ord[pps_n] = order; pps_n++;
      if( i==0 ) goto NO_LOOP;
    }
    if( p!=0 ) { pc = SuffCache[--i]; goto LOOP_ENTRY; }
    if( i==0 ) goto NO_LOOP;
    do {
      pc = SuffCache[--i];
      p = UpdateLowerSuffix(pc, sym, (i>0 ? NumStats_Cache[i-1] : 0));
      p_ctx = pc;
      StateCache[i] = p;
      StateCacheCtx[i] = pc;
LOOP_ENTRY:
      {
        bool same;
        if( upText ) same = succIsText(p_ctx, p);
        else if( upNull ) same = succIsNull(p_ctx, p);
        else same = succIsCtx(p_ctx, p) && child(p_ctx, p, 0, i+1)==upCtx;
        if( !same ) {
          pc = succIsCtx(p_ctx, p) ? child(p_ctx, p, 0, i+1) : 0;
          break;
        }
      }
      ps[pps_n] = p; ps_ctx[pps_n] = p_ctx; ps_ord[pps_n] = i; pps_n++;
    } while( i>0 );
NO_LOOP:
    if( pc==0 ) pc = Order0;
    if( pps_n==0 ) return pc;
    {
      byte* upPtr = upLeaf ? textRecover(ps[0]->succ, order) : textRecover(upVal, order+1);
      byte sym1 = *upPtr;
      uint cf = BequeathFreq(isC(pc) ? coldView(pc) : pc, sym1);
      word nsucc = textRef(upPtr+1);
      Ctx nc = 0;
      do {
        pps_n--;
        Ctx pctx = ps_ctx[pps_n];
        State* pst = ps[pps_n];
        int pord = ps_ord[pps_n];
        uint pg = pageOf(rec(pctx));
        bool leaf = pord+1>=_MaxOrder;
        byte* b = allocRec(pg, leaf ? 2u : 4u, leaf ? uint(ANYPAR) : 2u);
        State* bs = (State*)b;
        bs->sym = sym1; bs->tf = byte(cf);
        if( !leaf ) bs->succ = nsucc;
        nc = mkCtxL(b, 0, leaf);
        if( isM(pctx) ) {
          pst->tf |= 0x80;
          pst->succ = refOf(nc);
        } else {
          // a binary whose successor becomes a context moves to a ≡0 mod 4 slot
          byte* nb = allocRec(pg, 4, 0);
          State* nbs = (State*)nb;
          *nbs = *pst;
          nbs->succ = refOf(nc);
          freeRec(rec(pctx), 4);
          Ctx moved = mkCtx(nb, 0);
          if( parentSlot[pord] ) *parentSlot[pord] = refOf(moved);
          if( SuffCache[pord]==pctx ) SuffCache[pord] = moved;
          if( StateCacheCtx[pord]==pctx ) { StateCacheCtx[pord] = moved; StateCache[pord] = nbs; }
          if( FoundState==pst ) FoundState = nbs;
        }
      } while( pps_n!=0 );
      return nc;
    }
  }
};
uint flen(FILE* f) {
  fseek(f, 0, SEEK_END);
  uint len = ftell(f);
  fseek(f, 0, SEEK_SET);
  return len;
}
struct Model1 : Model {
  void do_process(void) {
    uint c, i;
    if( rc.f_DEC==0 ) {
      for( c = 24; c!=-8; c -= 8 ) rc.put(byte(_filesize>>c));
    } else {
      for( c = 0, i = 24; i!=-8; i -= 8 ) c |= rc.get()<<i;
      _filesize = c;
    }
    rc.rc_Init();
    const char* tag = (rc.f_DEC==0) ? "enc" : "dec";
    double t_start = now_s();
    double t_last = t_start;
    for( i = 0; i<_filesize; i++ ) {
      c = 0;
      if( rc.f_DEC==0 ) c = rc.get();
      c = ProcessByte(c, rc);
      if( rc.f_DEC==1 ) rc.put(c);
      if( (i & 0xFFFF)==0 ) {
        double t_now = now_s();
        if( t_now-t_last>=0.5 ) {
          double pct = 100.0*double(i+1)/double(_filesize);
          double elap = t_now-t_start;
          double mbps = (elap>0) ? (double(i+1)/elap)/(1024.0*1024.0) : 0.0;
          long csize = ftell((rc.f_DEC==0) ? rc.g : rc.f);
          fprintf(stderr, "\r[%s] %u > %ld  %5.1f%%  %5.2f MB/s  %5.2fM RAM",
                  tag, i+1, csize, pct, mbps, float(touched_total)/(1<<20));
          fflush(stderr);
          t_last = t_now;
        }
      }
    }
    long csize_end = ftell((rc.f_DEC==0) ? rc.g : rc.f);
    fprintf(stderr, "\r[%s] %u > %ld  100.0%%  done in %.2fs  %5.2fM RAM    \n",
            tag, _filesize, csize_end, now_s()-t_start, float(touched_total)/(1<<20));
    fflush(stderr);
    rc.rc_Quit();
  }
};
Model1 C;
uint pmd_args1[] = {12, 256, 0, 0, 0, 0};
//                  order MMAX reset win  Win  filesize
//                              perc perc Size (computed)
int main(int argc, char** argv) {
  uint i, r, f_DEC;

  if( argc<4 ) {
    fprintf(stderr, "ppmd c|d input output [order [MMAX [reset_perc [win_perc [WinSize]]]]]\n");
    return 1;
  }
  FILE* f = fopen(argv[2], "rb"); if( f==0 ) return 2;
  FILE* g = fopen(argv[3], "wb"); if( g==0 ) return 3;
  f_DEC = (argv[1][0]=='d');
  for( i = 0; i<Min<int>(argc-4, DIM(pmd_args1)-1); i++ ) {
    char* p;
    p = argv[4+i];
    p += (p[0]=='-');
    pmd_args1[i] = atoi(p);
  }
  uint filesize;
  if( f_DEC==0 ) {
    filesize = flen(f);
  } else {
    uint c = 0;
    for( int sh = 24; sh>=0; sh -= 8 ) c |= (uint)(byte)getc(f) << sh;
    filesize = c;
    fseek(f, 0, SEEK_SET);
  }
  pmd_args1[5] = filesize;
  C.rc.SetIO(f_DEC, f, g);
  r = C.Init(pmd_args1[0], pmd_args1[1], pmd_args1[2], pmd_args1[3], pmd_args1[4], pmd_args1[5]);
  if( r!=0 ) return r;
  C.do_process();
  qword meta = qword(C.nPages)*sizeof(PageMeta)+qword(C.nGroot)*sizeof(GRootEnt);
  for( uint p = 0; p<C.nPages; p++ ) meta += C.pm[p].roots.capacity()*4+C.pm[p].fars.capacity()*4+C.pm[p].farFree.capacity()*2;
  double MB = 1.0/(1<<20);
  printf("WinSize=%.2lf MB; tree_live=%.2lf MB; tree_touched=%.2lf MB; meta=%.2lf MB; pages=%u roots=%u; total=%.2lf MB; MemSize=%u MB\n",
         double(C._WinSize)*MB, double(C.live_total)*MB, double(C.touched_total)*MB, double(meta)*MB, C.nPages, C.nGroot,
         double(C._WinSize+C.touched_total+meta)*MB, pmd_args1[1]);
  fclose(f);
  fclose(g);
  C.Quit();
  return 0;
}
