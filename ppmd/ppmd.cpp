#include <vector>
#if defined(__linux__) || defined(__APPLE__)
#include <sys/mman.h>
#define HAVE_MMAP 1
#endif
#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif
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

#ifndef CLOCK_MONOTONIC
// Windows fallback: <time.h> defines `struct timespec` (since C11) but
// not CLOCK_MONOTONIC / clock_gettime. Provide both via the WinAPI
// performance counter; forward-declare to avoid pulling in <windows.h>.
#define CLOCK_MONOTONIC 1
extern "C" __declspec(dllimport) int __stdcall QueryPerformanceCounter(long long* count);
extern "C" __declspec(dllimport) int __stdcall QueryPerformanceFrequency(long long* freq);
static int clock_gettime(int /*clk_id*/, struct timespec* ts) {
  long long counter, freq;
  QueryPerformanceCounter(&counter);
  QueryPerformanceFrequency(&freq);
  ts->tv_sec  = (time_t)(counter / freq);
  ts->tv_nsec = (long)(((counter % freq) * 1000000000LL) / freq);
  return 0;
}
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
  void yield(void* p, int value) {}
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
  void resetMask() { EscCount++; }
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
// * The tree lives in 64 KB pages (virtual memory, committed on first touch).
//   A context reference is 16 bits:
//     >= FAR_LIM : near ref = page offset | M   (M=1 -> multi, 0 -> binary)
//     <  FAR_LIM : index into the page's far table -> global root id -> GRoot
//                  entry {page, near ref}, which is the parent slot of the root.
//   Children are allocated in their parent's page. Pages are compacted / split
//   (a set of sibling subtrees moves to a fresh page) only at the start of a
//   step, so only SuffCache[] and parentSlot[] need fixing up.
// * Text successors are coarse: P>>CZK in 16 bits. The exact P is recovered at
//   materialisation by scanning that block for the first end of the context
//   string + symbol (the last order+1 bytes of history).
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
  NFREE = 48,                  // free-list classes: 0/1 binary at ≡0/≡2 mod 4, n = multi with n states
  NO_ROOT = 0xFFFFFFFFu,
  DEAD_PAGE = 0xFFFFFFFFu
};

struct GRootEnt { uint page; word ref; word _pad; };

struct PageMeta {
  uint bump;                   // first free offset (>= RESV)
  uint live;                   // bytes in allocated records (upper bound: unreachable records count until compaction)
  uint hwm;                    // highest bump since last trim (touched memory)
  word freeHead[NFREE+1];      // [NFREE]: large records (>= NFREE states), each holding [next][NU]
  std::vector<uint> roots;     // global root ids whose record lives in this page
  std::vector<uint> fars;      // far table: global root id, or NO_ROOT when free
  std::vector<word> farFree;
  void reset() {
    bump = RESV; live = 0; hwm = RESV;
    memset(freeHead, 0, sizeof(freeHead));
    roots.clear(); fars.clear(); farFree.clear();
  }
};

// ---------------------------------------------------------------------------
// Coarse text pointer block scan
// ---------------------------------------------------------------------------
static uint scan_ref(const byte* win, uint lo, uint hi, const byte* pat, uint L) {
  for( uint pos = lo; pos<hi; pos++ )
    if( win[pos-1]==pat[L-1] && memcmp(win+pos-L, pat, L)==0 ) return pos;
  return 0xFFFFFFFFu;
}
#if defined(__x86_64__) || defined(_M_X64)
__attribute__((target("avx2")))
static uint scan_avx2(const byte* win, uint lo, uint hi, const byte* pat, uint L) {
  const __m256i vl = _mm256_set1_epi8((char)pat[L-1]);
  const __m256i vf = _mm256_set1_epi8((char)pat[0]);
  for( uint pos = lo; pos<hi; pos += 32 ) {
    if( !(pos & 63) ) _mm_prefetch((const char*)(win+pos+1024), _MM_HINT_T0);
    __m256i a = _mm256_loadu_si256((const __m256i*)(win+pos-1));
    __m256i b = _mm256_loadu_si256((const __m256i*)(win+pos-L));
    uint m = (uint)_mm256_movemask_epi8(_mm256_and_si256(_mm256_cmpeq_epi8(a, vl), _mm256_cmpeq_epi8(b, vf)));
    while( m ) {
      uint p = pos+__builtin_ctz(m);
      if( p>=hi ) return 0xFFFFFFFFu;
      if( L<=2 || memcmp(win+p-L+1, pat+1, L-2)==0 ) return p;
      m &= m-1;
    }
  }
  return 0xFFFFFFFFu;
}
__attribute__((target("avx512bw")))
static uint scan_avx512(const byte* win, uint lo, uint hi, const byte* pat, uint L) {
  const __m512i vl = _mm512_set1_epi8((char)pat[L-1]);
  const __m512i vf = _mm512_set1_epi8((char)pat[0]);
  for( uint pos = lo; pos<hi; pos += 64 ) {
    _mm_prefetch((const char*)(win+pos+1024), _MM_HINT_T0);
    __m512i a = _mm512_loadu_si512((const void*)(win+pos-1));
    __m512i b = _mm512_loadu_si512((const void*)(win+pos-L));
    qword m = _mm512_mask_cmpeq_epi8_mask(_mm512_cmpeq_epi8_mask(a, vl), b, vf);
    while( m ) {
      uint p = pos+uint(__builtin_ctzll(m));
      if( p>=hi ) return 0xFFFFFFFFu;
      if( L<=2 || memcmp(win+p-L+1, pat+1, L-2)==0 ) return p;
      m &= m-1;
    }
  }
  return 0xFFFFFFFFu;
}
static bool cpu_has_avx2() { __builtin_cpu_init(); return __builtin_cpu_supports("avx2"); }
static bool cpu_has_avx512bw() { __builtin_cpu_init(); return __builtin_cpu_supports("avx512bw"); }
#else
static uint scan_avx2(const byte* win, uint lo, uint hi, const byte* pat, uint L) { return scan_ref(win, lo, hi, pat, L); }
static uint scan_avx512(const byte* win, uint lo, uint hi, const byte* pat, uint L) { return scan_ref(win, lo, hi, pat, L); }
static bool cpu_has_avx2() { return false; }
static bool cpu_has_avx512bw() { return false; }
#endif

static void* map_mem(qword size) {
#ifdef HAVE_MMAP
  void* p = mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE, -1, 0);
  return (p==MAP_FAILED) ? 0 : p;
#else
  return calloc(1, size);
#endif
}
static void unmap_mem(void* p, qword size) {
#ifdef HAVE_MMAP
  if( p ) munmap(p, size);
#else
  free(p);
#endif
}
static void release_mem(void* p, qword size) {
#ifdef HAVE_MMAP
  if( size ) madvise(p, size, MADV_DONTNEED);
#endif
}

enum ContextFlagMasks { F_Rescaled = 0x04, F_HasText = 0x08, F_NextIsText = 0x10 };

struct Model {
  Rangecoder rc;
  SEE_Manager see;
  int _MaxOrder;
  int _ResetPerc;
  int _MMAX;
  uint _WinSize;
  uint _filesize;
  // text window
  byte* WinMap; qword WinMapSize;
  byte* WinBeg; byte* WinEnd; byte* pText;
  uint CZK;
  int scan_impl;   // 0 = scalar, 1 = AVX2, 2 = AVX-512BW
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
  qword st_compact, st_split, st_scan, st_scanbytes, st_moves, st_pagefail;
  qword tsc_compact, tsc_split, n_recs;
  qword touched_total;          // sum over pages of committed (touched) OS pages
  qword st_alloc[NFREE+1], st_free[NFREE+1], st_freehit[NFREE+1];
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
  uint pol_deadsh, pol_splitnum, pol_recvgap, pol_margin;
  uint recvPage;
  // maintenance scratch
  word fwd[PG_SIZE/2];
  byte* scratch;
  byte farUsed[FAR_LIM];

  // ---------------- record / state accessors ----------------
  static bool isM(Ctx c) { return (c & 1)!=0; }
  static byte* rec(Ctx c) { return (byte*)(c & ~(Ctx)1); }
  static Ctx mkCtx(byte* r, uint m) { return (Ctx)r | (Ctx)m; }
  static uint NS(Ctx c) { return isM(c) ? rec(c)[1] : 0u; }
  static State* S0(Ctx c) { return (State*)(rec(c) + (isM(c) ? 2 : 0)); }
  static uint EscF(Ctx c) { return isM(c) ? (rec(c)[0] & 0x7Fu) : 0u; }
  static void setEscF(Ctx c, uint v) { if( isM(c) ) rec(c)[0] = byte((rec(c)[0] & 0x80) | ((v>127) ? 127u : v)); }
  static bool Resc(Ctx c) { return isM(c) && (rec(c)[0] & 0x80)!=0; }
  static void setResc(Ctx c) { if( isM(c) ) rec(c)[0] |= 0x80; }
  static void clrResc(Ctx c) { if( isM(c) ) rec(c)[0] &= 0x7F; }
  static uint MF(const State& s) { return (s.tf & 0x7Fu)+1; }              // multi freq
  static void MaddF(State& s, uint d) { s.tf = byte(s.tf+d); }             // freq+d must stay <= 128
  static uint F(Ctx c, const State* s) { return isM(c) ? MF(*s) : s->tf; }
  static bool succIsCtx(Ctx c, const State* s) { return isM(c) ? (s->tf & 0x80)!=0 : (c & 2)==0; }
  static bool succIsNull(Ctx c, const State* s) { return !succIsCtx(c, s) && s->succ==NULL_SUCC; }
  static bool succIsText(Ctx c, const State* s) { return !succIsCtx(c, s) && s->succ!=NULL_SUCC; }
  static uint recSizeNU(uint NU) { return NU==1 ? 4u : 2u+4u*NU; }
  static uint recSize(Ctx c) { return isM(c) ? 2u+4u*(rec(c)[1]+1u) : 4u; }
  static word refOf(Ctx c) { return word(((uintptr_t)rec(c) & (PG_SIZE-1)) | (c & 1)); }
  static uint offOf(const void* a) { return uint((uintptr_t)a & (PG_SIZE-1)); }
  byte* pageBase(uint p) const { return arena + (qword(p)<<PG_BITS); }
  uint pageOf(const void* a) const { return uint(qword((const byte*)a - arena)>>PG_BITS); }
  static void swapS(State& a, State& b) { State t = a; a = b; b = t; }

  Ctx child(Ctx c, State* s, word** slot) {
    word r = s->succ;
    if( r>=FAR_LIM ) {
      if( slot ) *slot = &s->succ;
      byte* base = (byte*)((uintptr_t)rec(c) & ~(uintptr_t)(PG_SIZE-1));
      return mkCtx(base+(r & ~1u), r & 1u);
    }
    uint gid = pm[pageOf(rec(c))].fars[r];
    GRootEnt& g = groot[gid];
    if( slot ) *slot = &g.ref;
    return mkCtx(pageBase(g.page)+(g.ref & ~1u), g.ref & 1u);
  }
  uint SummFreq(Ctx c) {
    State* s0 = S0(c);
    if( !isM(c) ) return s0[0].tf;
    uint sum = EscF(c), ns = NS(c);
    for( uint i = 0; i<=ns; i++ ) sum += MF(s0[i]);
    return sum;
  }
  State* FindState(Ctx c, byte sym) {
    State* s0 = S0(c);
    uint ns = NS(c);
    for( uint i = 0; i<=ns; i++ ) if( s0[i].sym==sym ) return &s0[i];
    return 0;
  }
  word textRef(const byte* ptr) const { return word(uint(ptr-WinBeg)>>CZK); }

  // ---------------- per-page allocator ----------------
  static uint freeClass(uint NU, uint addrBits) { return NU==1 ? ((addrBits & 2) ? 1u : 0u) : (NU<NFREE ? NU : NFREE); }
  // Free-list classes: 0/1 = binary slot at ≡0/≡2 mod 4, n (2..NFREE-1) = multi with n states,
  // NFREE = large multi (size kept in the record). A miss falls back to the smallest larger free
  // record, which is carved; the remainder is kept as a smaller multi-sized free record (losing
  // at most 2 bytes) so that it can serve the next expansion.
  void pushFree(PageMeta& m, byte* base, uint cls, uint off) {
    *(word*)(base+off) = m.freeHead[cls];
    m.freeHead[cls] = word(off);
  }
  void pushBinSlots(PageMeta& m, byte* base, uint off, uint n) {   // n slots of 4 bytes starting at off
    uint cls = (off & 2) ? 1u : 0u;
    for( uint k = 0; k<n; k++ ) pushFree(m, base, cls, off+4*k);
  }
  void pushFreeMulti(PageMeta& m, byte* base, uint off, uint NU) {
    if( NU<NFREE ) pushFree(m, base, NU, off);
    else { *(word*)(base+off+2) = word(NU); pushFree(m, base, NFREE, off); }
  }
  // a free region of `bytes` (a multiple of 2) at off: as one multi record (+2 lost bytes) if it is
  // large enough, otherwise as binary slots
  void pushFreeTail(PageMeta& m, byte* base, uint off, uint bytes) {
    if( bytes>=10 ) {
      if( (bytes & 3)==2 ) pushFreeMulti(m, base, off, (bytes-2)/4);
      else pushFreeMulti(m, base, off+2, (bytes-4)/4);   // 4k bytes: lose 2 at the front... and 0 at the end
      return;
    }
    if( (bytes & 3)==0 ) pushBinSlots(m, base, off, bytes/4);
    else if( bytes>=6 ) pushBinSlots(m, base, off+2, (bytes-2)/4);
  }
  // pops a free multi record with >= NU states (smallest class first); returns offset, sets *haveNU
  uint popMultiAtLeast(PageMeta& m, byte* base, uint NU, uint* haveNU) {
    for( uint c = (NU<2 ? 2u : NU); c<NFREE; c++ ) {
      if( m.freeHead[c] ) {
        uint off = m.freeHead[c];
        m.freeHead[c] = *(word*)(base+off);
        *haveNU = c;
        return off;
      }
    }
    // large list: first fit
    word* link = &m.freeHead[NFREE];
    while( *link ) {
      uint off = *link;
      uint n = *(word*)(base+off+2);
      if( n>=NU ) {
        *link = *(word*)(base+off);
        *haveNU = n;
        return off;
      }
      link = (word*)(base+off);
    }
    return 0;
  }
  byte* allocRec(uint p, uint NU, bool textSucc) {
    PageMeta& m = pm[p];
    uint sz = recSizeNU(NU);
    uint cls = (NU==1) ? (textSucc ? 1u : 0u) : (NU<NFREE ? NU : (uint)NFREE);
    byte* base = pageBase(p);
    m.live += sz; live_total += sz;
    st_alloc[cls]++;
    if( cls<NFREE && m.freeHead[cls] ) {
      st_freehit[cls]++;
      uint off = m.freeHead[cls];
      m.freeHead[cls] = *(word*)(base+off);
      return base+off;
    }
    uint have, off;
    if( NU==1 ) {
      uint want = textSucc ? 2u : 0u;
      off = popMultiAtLeast(m, base, 2, &have);
      if( off ) {
        st_freehit[cls]++;
        // take one slot from a free multi record; the rest stays a (smaller) multi-sized free record
        if( (off & 3)==want ) {
          // [slot][rest: 2+4*(have-1) bytes]
          if( have-1>=2 ) pushFreeMulti(m, base, off+4, have-1); else pushFreeTail(m, base, off+4, 2+4*(have-1));
          return base+off;
        }
        // [2 lost][slot][rest: 4*(have-1) bytes]
        pushFreeTail(m, base, off+6, 4*(have-1));
        return base+off+2;
      }
    } else {
      off = popMultiAtLeast(m, base, NU+1, &have);
      if( off ) {
        st_freehit[cls]++;
        pushFreeTail(m, base, off+2+4*NU, 4*(have-NU));   // remainder: 4*(have-NU) bytes
        return base+off;
      }
    }
    off = m.bump;
    if( NU==1 ) {
      uint want = textSucc ? 2u : 0u;
      if( (off & 3)!=want ) off += 2;
    }
    if( off+sz>PG_SIZE ) {
      fprintf(stderr, "\nfatal: page %u overflow (bump=%u sz=%u live=%u)\n", p, m.bump, sz, m.live);
      exit(7);
    }
    m.bump = off+sz;
    if( m.bump>m.hwm ) setHwm(p, m.bump);
    updSlack(p);
    return base+off;
  }
  void freeRec(byte* r, uint NU) {
    uint p = pageOf(r);
    PageMeta& m = pm[p];
    uint sz = recSizeNU(NU);
    m.live -= sz; live_total -= sz;
    uint cls = freeClass(NU, uint((uintptr_t)r));
    st_free[cls]++;
    if( cls>=NFREE ) { cls = NFREE; *(word*)(r+2) = word(NU); }
    *(word*)r = m.freeHead[cls];
    m.freeHead[cls] = word(offOf(r));
  }
  static uint osPages(uint hwm) { return ((hwm+4095)>>12)-1; }   // touched OS pages above RESV
  void setHwm(uint p, uint hwm) {
    PageMeta& m = pm[p];
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
  uint newGRoot(uint page, word ref) {
    if( nGroot>=maxGroot ) { fprintf(stderr, "\nfatal: root table full\n"); exit(7); }
    uint g = nGroot++;
    groot[g].page = page; groot[g].ref = ref; groot[g]._pad = 0;
    return g;
  }
  Ctx rootCtx(uint gid) const { const GRootEnt& g = groot[gid]; return mkCtx(pageBase(g.page)+(g.ref & ~1u), g.ref & 1u); }

  // ---------------- maintenance (step boundary only) ----------------
  enum { K_MULTI = 0, K_BIN0 = 1, K_BIN2 = 2 };
  struct LNode { word off; word sz; int parent; word st; byte kind; byte moved; uint sub; };
  std::vector<LNode> Ls;
  std::vector<int> selBuf, kidBuf;
  std::vector<uint> selGid;
  static byte kindOfRef(word r) { return (r & 1) ? byte(K_MULTI) : ((r & 2) ? byte(K_BIN2) : byte(K_BIN0)); }
  void removeDeadRoots(uint p) {
    std::vector<uint>& R = pm[p].roots;
    for( size_t k = 0; k<R.size(); ) {
      if( R[k]==NO_ROOT || groot[R[k]].page!=p ) { R[k] = R.back(); R.pop_back(); } else k++;
    }
  }
  // Breadth-first list of the records of page p reachable from its roots through near refs
  // (parents precede their children).
  void enumPage(uint p) {
    PageMeta& m = pm[p];
    byte* base = pageBase(p);
    Ls.clear();
    for( size_t k = 0; k<m.roots.size(); k++ ) {
      word r = groot[m.roots[k]].ref;
      Ls.push_back(LNode{word(r & ~1u), 0, -1, word(k), kindOfRef(r), 0, 0});
    }
    for( size_t k = 0; k<Ls.size(); k++ ) {
      byte* r = base+Ls[k].off;
      if( Ls[k].kind==K_MULTI ) {
        uint ns = r[1];
        Ls[k].sz = word(2+4*(ns+1));
        State* s0 = (State*)(r+2);
        for( uint j = 0; j<=ns; j++ ) {
          if( !(s0[j].tf & 0x80) ) continue;
          word c = s0[j].succ;
          if( c>=FAR_LIM ) Ls.push_back(LNode{word(c & ~1u), 0, int(k), word(j), kindOfRef(c), 0, 0});
        }
      } else {
        Ls[k].sz = 4;
        if( Ls[k].kind==K_BIN0 ) {
          word c = ((State*)r)->succ;
          if( c>=FAR_LIM ) Ls.push_back(LNode{word(c & ~1u), 0, int(k), 0, kindOfRef(c), 0, 0});
        }
      }
    }
  }
  void trimPage(uint p) {
    PageMeta& m = pm[p];
    uint a = (m.bump+4095) & ~4095u, b = (m.hwm+4095) & ~4095u;
    if( b>=a+8192 ) { release_mem(pageBase(p)+a, b-a); setHwm(p, m.bump); }
  }
  // Picks a set of sibling subtrees holding about pol_splitnum/16 of the page.
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
    uint target = total*pol_splitnum/16, cap = target+target/4;
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
  // Compacts page p in place. With doSplit, a set of sibling subtrees first moves to a receiver
  // page. Only SuffCache[0..order] and parentSlot[1..order] are live transient references.
  bool Repack(uint p, bool doSplit) {
    qword t0 = __rdtsc();
    removeDeadRoots(p);
    enumPage(p);
    size_t n = Ls.size();
    n_recs += n;
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
      // far refs that leave with the moved records, plus the moved roots' own entries
      uint movedFar = uint(sel.size());
      byte* bp0 = pageBase(p);
      for( size_t k = 0; k<n; k++ ) {
        if( !Ls[k].moved ) continue;
        byte* r = bp0+Ls[k].off;
        if( Ls[k].kind==K_MULTI ) {
          State* s0 = (State*)(r+2);
          for( uint j = 0; j<=r[1]; j++ ) if( (s0[j].tf & 0x80) && s0[j].succ<FAR_LIM ) movedFar++;
        } else if( Ls[k].kind==K_BIN0 && ((State*)r)->succ<FAR_LIM ) movedFar++;
      }
      if( recvPage!=DEAD_PAGE && recvPage!=p && recvPage<nPages && pm[recvPage].bump+selBytes+pol_recvgap<=PG_SIZE
          && farUsedCount(recvPage)+movedFar+16<=FAR_LIM ) q = recvPage;
      else {
        q = newPage();
        if( q==DEAD_PAGE ) { st_pagefail++; return false; }
        recvPage = q;
      }
      st_split++;
    } else st_compact++;
    PageMeta& P = pm[p];
    // layout: [multi][binary ≡0][binary ≡2] for the staying part (page p, rebuilt from RESV)
    // and for the moved part (appended to page q)
    uint sb[3] = {0, 0, 0}, mb[3] = {0, 0, 0};
    for( size_t k = 0; k<n; k++ ) {
      uint* b = Ls[k].moved ? mb : sb;
      if( Ls[k].kind==K_MULTI ) b[0] += Ls[k].sz; else b[Ls[k].kind]++;
    }
    uint so[3], mo[3], send, mend = 0, qstart = 0;
    so[0] = RESV; so[1] = (RESV+sb[0]+3) & ~3u; so[2] = so[1]+4*sb[1]+2;
    send = sb[2] ? so[2]+4*sb[2] : so[1]+4*sb[1];
    if( doSplit ) {
      qstart = (pm[q].bump+3) & ~3u;
      mo[0] = qstart; mo[1] = (qstart+mb[0]+3) & ~3u; mo[2] = mo[1]+4*mb[1]+2;
      mend = mb[2] ? mo[2]+4*mb[2] : mo[1]+4*mb[1];
      if( mend>PG_SIZE ) { fprintf(stderr, "\nfatal: receiver overflow\n"); exit(7); }
    }
    if( send>PG_SIZE ) { fprintf(stderr, "\nfatal: repack overflow\n"); exit(7); }
    for( size_t k = 0; k<n; k++ ) {
      uint* o = Ls[k].moved ? mo : so;
      uint kd = Ls[k].kind;
      uint no = o[kd];
      o[kd] += (kd==K_MULTI) ? Ls[k].sz : 4u;
      fwd[Ls[k].off>>1] = word(no | Ls[k].moved);
    }
    byte* bp = pageBase(p);
    byte* bq = doSplit ? pageBase(q) : 0;
    // global ids of the moved subtree roots
    selGid.resize(sel.size());
    for( size_t t = 0; t<sel.size(); t++ ) {
      const LNode& s = Ls[sel[t]];
      word nref = word((fwd[s.off>>1] & ~1u) | (s.kind==K_MULTI ? 1u : 0u));
      uint gid;
      if( s.parent<0 ) {
        gid = P.roots[s.st];
        groot[gid].page = q; groot[gid].ref = nref;
        P.roots[s.st] = NO_ROOT;
      } else gid = newGRoot(q, nref);
      pm[q].roots.push_back(gid);
      selGid[t] = gid;
    }
    memset(farUsed, 0, P.fars.size());
    uint stayBytes = 0, movedBytes = 0;
    for( size_t k = 0; k<n; k++ ) {
      const LNode& L = Ls[k];
      uint f = fwd[L.off>>1];
      bool mv = (f & 1)!=0;
      byte* dst = mv ? bq+(f & ~1u) : scratch+f;
      byte* src = bp+L.off;
      State* ds;
      uint cnt;
      if( L.kind==K_MULTI ) {
        memcpy(dst, src, L.sz);
        ds = (State*)(dst+2); cnt = dst[1]+1u;
      } else {
        *(uint32_t*)dst = *(const uint32_t*)src;
        ds = (State*)dst; cnt = (L.kind==K_BIN0) ? 1u : 0u;
      }
      if( mv ) movedBytes += L.sz; else stayBytes += L.sz;
      for( uint j = 0; j<cnt; j++ ) {
        if( L.kind==K_MULTI && !(ds[j].tf & 0x80) ) continue;
        word c = ds[j].succ;
        if( c>=FAR_LIM ) {
          uint cf = fwd[(c & ~1u)>>1];
          if( mv || !(cf & 1) ) ds[j].succ = word((cf & ~1u) | (c & 1u));
          else {
            // child is a moved subtree root: becomes a far ref
            uint t = 0;
            while( Ls[sel[t]].off!=(c & ~1u) ) t++;
            word jf = allocFar(p, selGid[t]);
            farUsed[jf] = 1;
            ds[j].succ = jf;
          }
        } else if( mv ) {
          uint gid = P.fars[c];
          freeFar(p, c);
          ds[j].succ = allocFar(q, gid);
        } else farUsed[c] = 1;
      }
    }
    // remap the transient references into page p
    for( int i = 0; i<=order; i++ ) {
      byte* r = rec(SuffCache[i]);
      if( pageOf(r)!=p ) continue;
      uint f = fwd[offOf(r)>>1];
      SuffCache[i] = mkCtx(((f & 1) ? bq : bp)+(f & ~1u), uint(SuffCache[i] & 1));
    }
    for( int i = 1; i<=order; i++ ) {
      byte* a = (byte*)parentSlot[i];
      if( a<bp || a>=bp+PG_SIZE ) continue;
      uint so_ = offOf(a);
      size_t k = 0;
      while( k<n && !(so_>=Ls[k].off && so_<uint(Ls[k].off)+Ls[k].sz) ) k++;
      if( k==n ) { fprintf(stderr, "\nfatal: parent slot not in a live record\n"); exit(7); }
      uint f = fwd[Ls[k].off>>1];
      uint nofs = (f & ~1u)+(so_-Ls[k].off);
      if( f & 1 ) parentSlot[i] = (word*)(bq+nofs);
      else {
        word v = *(word*)(scratch+nofs);
        parentSlot[i] = (v<FAR_LIM) ? &groot[P.fars[v]].ref : (word*)(bp+nofs);
      }
    }
    memcpy(bp+RESV, scratch+RESV, send-RESV);
    for( size_t k = 0; k<n && Ls[k].parent<0; k++ ) {
      if( Ls[k].moved ) continue;
      groot[P.roots[Ls[k].st]].ref = word((fwd[Ls[k].off>>1] & ~1u) | (Ls[k].kind==K_MULTI ? 1u : 0u));
    }
    removeDeadRoots(p);
    live_total += qword(stayBytes)+movedBytes-P.live;
    P.live = stayBytes;
    P.bump = send;
    memset(P.freeHead, 0, sizeof(P.freeHead));
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
      tsc_split += __rdtsc()-t0;
    } else tsc_compact += __rdtsc()-t0;
    return true;
  }
  uint needFor(Ctx c) {
    uint NU = NS(c)+1;
    return (NU<256 ? recSizeNU(NU+1) : 0u) + 12u;
  }
  // Makes sure every page holding a current context can absorb this step's allocations.
  bool EnsureHeadroom() {
    {
      // fast path: every page can absorb the step's worst case for all current contexts together
      uint sum = 0;
      for( int i = 0; i<=order; i++ ) sum += needFor(SuffCache[i]);
      int i = 0;
      while( i<=order && pgSlack[pageOf(rec(SuffCache[i]))]>=sum+64 ) i++;
      if( i>order ) return true;
    }
    for( int iter = 0; iter<64; iter++ ) {
      uint pg[MAX_O+2], nd[MAX_O+2]; int np = 0;
      for( int i = 0; i<=order; i++ ) {
        uint p = pageOf(rec(SuffCache[i])), n = needFor(SuffCache[i]);
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
        bool tight = freeb<nd[k]+64;
        bool wasteful = (m.bump+nd[k]>((m.hwm+4095) & ~4095u)) && dead>(m.live>>pol_deadsh)+256;
        if( tight || wasteful ) {
          bad = k;
          doSplit = (m.live+nd[k]+pol_margin>USABLE);
          break;
        }
      }
      if( bad<0 ) { Order0 = rootCtx(0); return true; }
      if( !Repack(pg[bad], doSplit) ) return false;
    }
    fprintf(stderr, "\nfatal: EnsureHeadroom did not converge\n");
    exit(7);
  }

  // ---------------- debug: full tree consistency check (PPMD_CHECK=n: every n steps) ----------------
  qword check_every = 0, check_step = 0;
  void CheckFail(const char* msg, Ctx c) {
    fprintf(stderr, "\nCHECK FAILED at step %llu: %s (page %u off %u M=%d)\n", (unsigned long long)check_step, msg,
            pageOf(rec(c)), offOf(rec(c)), int(c & 1));
    exit(11);
  }
  void CheckTree() {
    std::vector<byte> seen(size_t(nPages)<<(PG_BITS-1), 0);
    std::vector<Ctx> st;
    st.push_back(rootCtx(0));
    while( !st.empty() ) {
      Ctx c = st.back(); st.pop_back();
      uint p = pageOf(rec(c)), off = offOf(rec(c));
      if( p>=nPages ) CheckFail("page out of range", c);
      if( off<RESV || off+recSize(c)>pm[p].bump ) CheckFail("record outside the allocated part of its page", c);
      if( !isM(c) && (off & 1) ) CheckFail("misaligned binary", c);
      size_t vi = (size_t(p)<<(PG_BITS-1))+(off>>1);
      if( seen[vi] ) CheckFail("record reachable twice", c);
      seen[vi] = 1;
      if( isM(c) && NS(c)==0 ) CheckFail("multi record with NumStats 0", c);
      State* s0 = S0(c);
      uint ns = NS(c);
      for( uint j = 0; j<=ns; j++ ) {
        if( isM(c) && MF(s0[j])>124 ) CheckFail("multi freq above MAX_FREQ", c);
        if( !succIsCtx(c, &s0[j]) ) continue;
        word r = s0[j].succ;
        if( r<FAR_LIM ) {
          if( r>=pm[p].fars.size() || pm[p].fars[r]==NO_ROOT ) CheckFail("dangling far ref", c);
          uint gid = pm[p].fars[r];
          if( gid>=nGroot || groot[gid].page>=nPages ) CheckFail("bad root id", c);
          bool listed = false;
          for( size_t k = 0; k<pm[groot[gid].page].roots.size(); k++ ) if( pm[groot[gid].page].roots[k]==gid ) listed = true;
          if( !listed ) CheckFail("root missing from its page's root list", c);
        }
        st.push_back(child(c, &s0[j], 0));
      }
    }
    for( int i = 0; i<=order; i++ ) {
      uint p = pageOf(rec(SuffCache[i])), off = offOf(rec(SuffCache[i]));
      if( !seen[(size_t(p)<<(PG_BITS-1))+(off>>1)] ) CheckFail("SuffCache entry unreachable", SuffCache[i]);
      if( i>0 && *parentSlot[i]!=refOf(SuffCache[i]) ) CheckFail("parent slot does not reference SuffCache entry", SuffCache[i]);
    }
  }
  // ---------------- model ----------------
  void CacheNumstatsAndFlags(int order) {
    uint nextBit = 0;
    if( pText>WinBeg && pText[-1]>=0x40 ) nextBit = F_NextIsText;
    for( int i = 0; i<=order; i++ ) {
      Ctx pc = SuffCache[i];
      uint ns = NS(pc);
      NumStats_Cache[i] = ns;
      State* s0 = S0(pc);
      uint hasText = 0;
      for( uint j = 0; j<=ns; j++ ) if( s0[j].sym>=0x40 ) { hasText = F_HasText; break; }
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
      SuffCache[i+1] = child(sctx, st, &slot);
      parentSlot[i+1] = slot;
    }
  }
  uint Init(uint MaxOrder, uint MMAX, uint ResetPerc, uint WinPerc, uint WinSize, uint filesize) {
    _MaxOrder = MaxOrder;
    _ResetPerc = (int)ResetPerc;
    _MMAX = MMAX;
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
    if( getenv("PPMD_CZK") ) { uint k = atoi(getenv("PPMD_CZK")); if( k>CZK ) CZK = k; }
    scan_impl = cpu_has_avx512bw() ? 2 : cpu_has_avx2() ? 1 : 0;
    if( getenv("PPMD_SCAN") ) scan_impl = atoi(getenv("PPMD_SCAN"));
    pol_deadsh = getenv("PPMD_DEADSH") ? atoi(getenv("PPMD_DEADSH")) : 6;
    pol_splitnum = getenv("PPMD_SPLIT") ? atoi(getenv("PPMD_SPLIT")) : 3;   // moved fraction = pol_splitnum/16
    pol_recvgap = getenv("PPMD_RECVGAP") ? atoi(getenv("PPMD_RECVGAP")) : 8192;
    pol_margin = getenv("PPMD_MARGIN") ? atoi(getenv("PPMD_MARGIN")) : 2048;
    check_every = getenv("PPMD_CHECK") ? strtoull(getenv("PPMD_CHECK"), 0, 10) : 0;
    // sane ranges: a split moves at most half a page; the split threshold leaves at least half a page
    pol_splitnum = CLAMP(pol_splitnum, 1u, 8u);
    pol_margin = CLAMP(pol_margin, 256u, 16384u);
    pol_deadsh = CLAMP(pol_deadsh, 0u, 12u);
    pol_recvgap = CLAMP(pol_recvgap, 1024u, 32768u);
    // window (+ padding for the vector scan), 2 MB aligned for transparent huge pages
    WinMapSize = qword(_WinSize)+(4u<<20);
    WinMap = (byte*)map_mem(WinMapSize);
    if( WinMap==0 ) return 1;
    WinBeg = (byte*)(((uintptr_t)WinMap+(2u<<20)-1) & ~(uintptr_t)((2u<<20)-1));
#if defined(HAVE_MMAP) && defined(MADV_HUGEPAGE)
    madvise(WinBeg, qword(_WinSize), MADV_HUGEPAGE);
#endif
    WinEnd = WinBeg+_WinSize;
    // arena of 64 KB pages
    maxPages = uint((qword(MMAX)<<20)>>PG_BITS);
    if( maxPages<4 ) maxPages = 4;
    arenaMapSize = (qword(maxPages)+1)<<PG_BITS;
    arenaMap = (byte*)map_mem(arenaMapSize);
    if( arenaMap==0 ) return 1;
    arena = (byte*)(((uintptr_t)arenaMap+PG_SIZE-1) & ~(uintptr_t)(PG_SIZE-1));
    pm.reserve(maxPages);
    pgSlack.assign(maxPages, 0);
    maxGroot = 1u<<25;
    groot = (GRootEnt*)map_mem(qword(maxGroot)*sizeof(GRootEnt));
    if( groot==0 ) return 1;
    scratch = (byte*)malloc(PG_SIZE);
    nPages = 0;
    st_compact = st_split = st_scan = st_scanbytes = st_moves = st_pagefail = 0;
    tsc_compact = tsc_split = n_recs = 0;
    memset(st_alloc, 0, sizeof(st_alloc)); memset(st_free, 0, sizeof(st_free)); memset(st_freehit, 0, sizeof(st_freehit));
    StartModelRare();
    return 0;
  }
  void Quit(void) {
    unmap_mem(WinMap, WinMapSize);
    unmap_mem(arenaMap, arenaMapSize);
    unmap_mem(groot, qword(maxGroot)*sizeof(GRootEnt));
    free(scratch);
  }
  qword touchedBytes() const {
    qword t = 0;
    for( uint p = 0; p<nPages; p++ ) t += ((pm[p].hwm+4095) & ~4095u)-4096u;
    return t;
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
    recvPage = DEAD_PAGE;
    pText = WinBeg;
    see.initialize(_MaxOrder);
    uint p = newPage();
    byte* r = allocRec(p, 256, false);
    Order0 = mkCtx(r, 1);
    r[0] = 1;       // EscFreq 1, not rescaled
    r[1] = 255;     // NumStats
    State* s0 = S0(Order0);
    for( i = 0; i<256; i++ ) { s0[i].sym = byte(i); s0[i].tf = 0; s0[i].succ = NULL_SUCC; }
    uint g = newGRoot(p, refOf(Order0));
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
      uint i = 0;
      while( s0[i].sym!=sym1 ) i++;
      uint cf = MF(s0[i])-1;
      uint sc = 1+sf-(ns+1)-cf;
      sc <<= 7;
      cf = 1+((314*cf<sc) ? (1536*cf>sc) : 2+(cf*175)/sc);
      return byte(cf);
    }
    return s0[0].tf;
  }
  // rescale of a multi context; mirrors the reference exactly (operates on an unpacked copy)
  uint rescale(Ctx ctx, int OrderFall, State*& FoundState) {
    struct U { byte sym; byte T; word succ; int f; byte orig; };
    U u[256], tmp;
    int of, i, a, f0, sf_orig, esc_local;
    uint reallocSize = 0;
    clrResc(ctx);
    State* s0 = S0(ctx);
    uint ns = NS(ctx);
    for( uint k = 0; k<=ns; k++ ) { u[k].sym = s0[k].sym; u[k].T = s0[k].tf & 0x80; u[k].succ = s0[k].succ; u[k].f = MF(s0[k]); u[k].orig = byte(k); }
    {
      uint fs_idx = uint(FoundState-s0);
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
        for( uint k = 0; k<=ns; k++ ) { s0[k].sym = u[k].sym; s0[k].tf = byte(u[k].T | ((u[k].f>0 ? u[k].f : 1)-1)); s0[k].succ = u[k].succ; }
        { byte ni[256]; for( uint k = 0; k<=ns; k++ ) ni[u[k].orig] = byte(k); slotPermute(s0, ns+1, ni); }
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
    for( uint k = 0; k<=ns; k++ ) { s0[k].sym = u[k].sym; s0[k].tf = byte(u[k].T | ((u[k].f>0 ? u[k].f : 1)-1)); s0[k].succ = u[k].succ; }
    { byte ni[256]; for( uint k = 0; k<=ns; k++ ) ni[u[k].orig] = byte(k); slotPermute(s0, ns+1, ni); }
    setResc(ctx);
    FoundState = &s0[0];
    return reallocSize;
  }
  Ctx ShrinkContext(Ctx old, uint OldNU, uint NewNU) {
    uint p = pageOf(rec(old));
    State* os = S0(old);
    Ctx nc;
    if( NewNU==1 ) {
      bool ctxSucc = (os[0].tf & 0x80)!=0;
      byte* r = allocRec(p, 1, !ctxSucc);
      nc = mkCtx(r, 0);
      State* ns0 = S0(nc);
      ns0[0].sym = os[0].sym;
      ns0[0].tf = byte(MF(os[0]));
      ns0[0].succ = os[0].succ;
    } else {
      byte* r = allocRec(p, NewNU, false);
      nc = mkCtx(r, 1);
      r[0] = rec(old)[0];
      r[1] = byte(NewNU-1);
      memcpy(r+2, os, 4*NewNU);
    }
    freeRec(rec(old), OldNU);
    return nc;
  }
  void FinishRescale(Ctx& ctx, State*& FoundState, uint newNU) {
    uint oldNU = NS(ctx)+1;
    if( newNU<oldNU ) {
      uint idx = uint(FoundState-S0(ctx));
      State* oldS = S0(ctx);
      Ctx nc = ShrinkContext(ctx, oldNU, newNU);
      slotMove(oldS, newNU, S0(nc));
      FoundState = S0(nc)+idx;
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
    byte* nr = allocRec(p, OldNU+1, false);
    Ctx nc = mkCtx(nr, 1);
    State* n0 = (State*)(nr+2);
    if( ns1==0 ) {
      State b = S0(pc)[0];
      uint T = succIsCtx(pc, &b) ? 0x80u : 0u;
      uint f = b.tf;
      f = (f<=MAX_FREQ/3) ? (2*f-1) : (MAX_FREQ-15);
      nr[0] = 0; nr[1] = 1;
      n0[0].sym = b.sym; n0[0].tf = byte(T | (f-1)); n0[0].succ = b.succ;
      setEscF(nc, (ns>1)+ExpEscape[see.QTable[see.BSumm>>8]]);
    } else {
      memcpy(nr, rec(pc), 2+4*OldNU);
      nr[1] = byte(OldNU);
      setEscF(nc, EscF(nc)+(see.QTable[ns+4]>>3));
    }
    slotMove(S0(pc), OldNU, n0);
    freeRec(rec(pc), OldNU);
    if( parentSlot[i] ) *parentSlot[i] = refOf(nc);
    SuffCache[i] = nc;
    uint sumFreq = EscF(nc);
    for( uint k = 0; k<OldNU; k++ ) sumFreq += MF(n0[k]);
    uint cf = (FFreq-1)*(5+sumFreq);
    uint sf = s0_caller+sumFreq;
    if( cf<=3*sf ) {
      cf = 1+(2*cf>sf)+(2*cf>3*sf);
      setEscF(nc, EscF(nc)+4-cf);
    } else {
      cf = 5+(cf>5*sf)+(cf>6*sf)+(cf>8*sf)+(cf>10*sf)+(cf>12*sf);
    }
    State& np = n0[OldNU];
    np.sym = FSymbol;
    np.tf = byte(cf-1);
    np.succ = tsucc;
    StateCache[i] = &np;
    StateCacheCtx[i] = nc;
  }
  byte* textRecover(word cz, uint L) {
    uint cur = uint(pText-WinBeg);
    uint lo = uint(cz)<<CZK, hi = lo+(1u<<CZK);
    if( hi>cur ) hi = cur;
    if( lo<L ) lo = L;
    const byte* pat = pText-L;
    uint pos = scan_impl==2 ? scan_avx512(WinBeg, lo, hi, pat, L) : scan_impl==1 ? scan_avx2(WinBeg, lo, hi, pat, L) : scan_ref(WinBeg, lo, hi, pat, L);
    if( pos==0xFFFFFFFFu ) {
      fprintf(stderr, "\nfatal: text pointer recovery failed (block %u, L=%u, cur=%u)\n", uint(cz), L, cur);
      exit(8);
    }
    st_scan++; st_scanbytes += pos-lo+1;
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
        freq = MF(s0[i]);
        flag = (rc.f_DEC!=0) ? low+freq>count : s0[i].sym==symbol;
        if( flag!=0 ) break;
        low += freq;
      }
      if( flag!=0 ) {
        MaddF(s0[i], 4);
        if( MF(s0[i])>MF(s0[i-1]) ) {
          slotSwap(&s0[i], &s0[i-1]);
          swapS(s0[i], s0[i-1]);
          i--;
        }
        found = &s0[i];
      } else {
        freq = total-low;
        see.NumMasked = cnum;
        for( i = 0; i<=cnum; i++ ) see.CharMask[s0[i].sym] = see.EscCount;
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
      c = s0[i].sym;
      if( see.CharMask[c]!=see.EscCount ) {
        see.CharMask[c] = see.EscCount;
        low += MF(s0[i]);
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
        for( low = 0, i = 0; (low += MF(s0[px[i]]))<=count; i++ );
        j = px[i];
      } else {
        low = pl;
      }
      found = &s0[j];
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
  }
  template<class RC>
  uint ProcessByte(uint c, RC& rc) {
    Ctx MinContext;
    Ctx p;
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
      uint hasText = 0;
      State* s0 = S0(MinContext);
      uint ns = NS(MinContext);
      for( uint j = 0; j<=ns; j++ ) if( s0[j].sym>=0x40 ) { hasText = F_HasText; break; }
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
        uint hasText = 0;
        State* s0 = S0(MinContext);
        uint ns = NS(MinContext);
        for( uint j = 0; j<=ns; j++ ) if( s0[j].sym>=0x40 ) { hasText = F_HasText; break; }
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
    if( fText ) {
      const byte* blk = WinBeg+(uint(FoundState->succ)<<CZK);
      _mm_prefetch((const char*)blk, _MM_HINT_T0);
      _mm_prefetch((const char*)blk+64, _MM_HINT_T0);
      _mm_prefetch((const char*)blk+128, _MM_HINT_T0);
      _mm_prefetch((const char*)blk+192, _MM_HINT_T0);
    }
    if( (order<_MaxOrder)||fText ) {
      p = UpdateModel(MinContext);
    } else {
      p = succIsCtx(MinContext, FoundState) ? child(MinContext, FoundState, 0) : 0;
    }
    if( p==0 ) {
      if( m_replay ) { m_replay_aborted = true; return c; }
      Reset("");
    }
    if( order+1>hwm ) hwm = order+1;
    CacheSuccessors(c);
    if( order+1>hwm ) hwm = order+1;
    if( check_every && ++check_step%check_every==0 ) CheckTree();
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
    word iSuccessor = textRef(pText);
    Ctx iF;
    f_order0 = 0;
    if( !fNull ) {
      if( fText ) iF = CreateSuccessors(0, p, pc, MinContext);
      else iF = child(MinContext, FoundState, 0);
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
    word upVal = FoundState->succ;
    Ctx upCtx = (!upText && !upNull) ? child(pc, FoundState, 0) : 0;
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
        else same = succIsCtx(p_ctx, p) && child(p_ctx, p, 0)==upCtx;
        if( !same ) {
          pc = succIsCtx(p_ctx, p) ? child(p_ctx, p, 0) : 0;
          break;
        }
      }
      ps[pps_n] = p; ps_ctx[pps_n] = p_ctx; ps_ord[pps_n] = i; pps_n++;
    } while( i>0 );
NO_LOOP:
    if( pc==0 ) pc = Order0;
    if( pps_n==0 ) return pc;
    {
      byte* upPtr = textRecover(upVal, order+1);
      byte sym1 = *upPtr;
      uint cf = BequeathFreq(pc, sym1);
      word nsucc = textRef(upPtr+1);
      Ctx nc = 0;
      do {
        pps_n--;
        Ctx pctx = ps_ctx[pps_n];
        State* pst = ps[pps_n];
        int pord = ps_ord[pps_n];
        uint pg = pageOf(rec(pctx));
        byte* b = allocRec(pg, 1, true);
        State* bs = (State*)b;
        bs->sym = sym1; bs->tf = byte(cf); bs->succ = nsucc;
        nc = mkCtx(b, 0);
        if( isM(pctx) ) {
          pst->tf |= 0x80;
          pst->succ = refOf(nc);
        } else {
          // a binary whose successor becomes a context moves to a ≡0 mod 4 slot
          byte* nb = allocRec(pg, 1, false);
          State* nbs = (State*)nb;
          *nbs = *pst;
          nbs->succ = refOf(nc);
          freeRec(rec(pctx), 1);
          Ctx moved = mkCtx(nb, 0);
          st_moves++;
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
static double now_s(void) {
  struct timespec ts;
  clock_gettime(1, &ts);
  return double(ts.tv_sec)+double(ts.tv_nsec)*1e-9;
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
    rc.yield(this, 0);
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
  {
    double MB = 1.0/(1<<20);
    qword touched = C.touchedBytes();
    if( touched!=C.touched_total ) fprintf(stderr, "warning: touched counter %llu != %llu\n", (unsigned long long)C.touched_total, (unsigned long long)touched);
    qword meta = qword(C.nPages)*sizeof(PageMeta)+qword(C.nGroot)*sizeof(GRootEnt);
    for( uint p = 0; p<C.nPages; p++ ) meta += C.pm[p].roots.capacity()*4+C.pm[p].fars.capacity()*4+C.pm[p].farFree.capacity()*2;
    printf("WinSize=%.2lf MB; tree_live=%.2lf MB; tree_touched=%.2lf MB; meta=%.2lf MB; pages=%u roots=%u; total=%.2lf MB; MemSize=%u MB\n",
           double(C._WinSize)*MB, double(C.live_total)*MB, double(touched)*MB, double(meta)*MB, C.nPages, C.nGroot,
           double(C._WinSize+touched+meta)*MB, pmd_args1[1]);
    {
      qword dead = 0, slack = 0, fl = 0, lv = 0;
      for( uint p = 0; p<C.nPages; p++ ) {
        const PageMeta& m = C.pm[p];
        lv += m.live; dead += m.bump-RESV-m.live; slack += ((m.hwm+4095) & ~4095u)-m.bump;
        for( int k = 0; k<=NFREE; k++ ) { uint o = m.freeHead[k]; while( o ) { fl += (k<2 ? 4 : 2+4*(k<NFREE ? k : *(word*)(C.pageBase(p)+o+2))); o = *(word*)(C.pageBase(p)+o); } }
      }
      if( getenv("PPMD_ALLOCSTATS") ) {
        qword clsb[NFREE+1]; memset(clsb, 0, sizeof(clsb));
        for( uint p = 0; p<C.nPages; p++ ) {
          const PageMeta& m = C.pm[p];
          for( int k = 0; k<=NFREE; k++ ) { uint o = m.freeHead[k]; while( o ) { clsb[k] += (k<2 ? 4 : 2+4*(k<NFREE ? k : *(word*)(C.pageBase(p)+o+2))); o = *(word*)(C.pageBase(p)+o); } }
        }
        for( int k = 0; k<=NFREE; k++ ) if( clsb[k] ) fprintf(stderr, "free cls %2d: %.3f MB\n", k, clsb[k]/1048576.0);
      }
      fprintf(stderr, "pages: live=%.2fMB dead=%.2fMB (freelists %.2fMB) above-bump=%.2fMB avg_live/page=%.1fKB\n",
              lv/1048576.0, dead/1048576.0, fl/1048576.0, slack/1048576.0, C.nPages ? lv/1024.0/C.nPages : 0.0);
    }
    if( getenv("PPMD_STATS") ) {
      fprintf(stderr, "maintenance: compact=%.0f Mcyc split=%.0f Mcyc, %.1fM records copied\n", C.tsc_compact/1e6, C.tsc_split/1e6, C.n_recs/1e6);
      size_t mx = 0, mr = 0;
      for( uint p = 0; p<C.nPages; p++ ) { if( C.pm[p].fars.size()>mx ) mx = C.pm[p].fars.size(); if( C.pm[p].roots.size()>mr ) mr = C.pm[p].roots.size(); }
      fprintf(stderr, "max far table=%zu max roots/page=%zu\n", mx, mr);
    }
    fprintf(stderr, "stats: CZK=%u scans=%llu avgscan=%.1f compact=%llu split=%llu binmoves=%llu pagefail=%llu\n",
            C.CZK, (unsigned long long)C.st_scan, C.st_scan ? double(C.st_scanbytes)/C.st_scan : 0.0,
            (unsigned long long)C.st_compact, (unsigned long long)C.st_split, (unsigned long long)C.st_moves,
            (unsigned long long)C.st_pagefail);
  }
  if( getenv("PPMD_CENSUS") ) {
    // walk all live records
    qword nm = 0, nms = 0, nb0 = 0, nb2 = 0, ntext = 0, nctx = 0, nfar = 0, nnull = 0;
    qword nu2tt = 0, nbb = 0;
    for( uint p = 0; p<C.nPages; p++ ) {
      C.removeDeadRoots(p);
      C.enumPage(p);
      for( size_t k = 0; k<C.Ls.size(); k++ ) {
        const Model::LNode& L = C.Ls[k];
        byte* r = C.pageBase(p)+L.off;
        if( L.kind==Model::K_MULTI ) {
          uint ns = r[1]; nm++; nms += ns+1;
          State* s0 = (State*)(r+2);
          uint tt = 0;
          for( uint j = 0; j<=ns; j++ ) {
            if( s0[j].tf & 0x80 ) { nctx++; if( s0[j].succ<FAR_LIM ) nfar++; }
            else if( s0[j].succ==NULL_SUCC ) nnull++;
            else { ntext++; tt++; }
          }
          if( ns==1 && tt==2 ) nu2tt++;
        } else if( L.kind==Model::K_BIN0 ) {
          nb0++; nctx++;
          word c = ((State*)r)->succ;
          if( c<FAR_LIM ) nfar++;
          else if( !(c & 1) ) nbb++;    // binary whose child is a binary in the same page
        }
        else { nb2++; ntext++; }
      }
    }
    fprintf(stderr, "census: multi=%llu (states %llu, avgNU %.2f) bin_ctx=%llu bin_text=%llu | succ: text=%llu ctx=%llu (far %llu) null=%llu | NU2 both-text=%llu\n",
      (unsigned long long)nm, (unsigned long long)nms, nm ? double(nms)/nm : 0.0, (unsigned long long)nb0, (unsigned long long)nb2,
      (unsigned long long)ntext, (unsigned long long)nctx, (unsigned long long)nfar, (unsigned long long)nnull, (unsigned long long)nu2tt);
    fprintf(stderr, "census: binary->binary near links=%llu\n", (unsigned long long)nbb);
    fprintf(stderr, "census bytes: multi hdr=%.2fMB multi states=%.2fMB binaries=%.2fMB\n", nm*2/1048576.0, nms*4/1048576.0, (nb0+nb2)*4/1048576.0);
  }
  if( getenv("PPMD_ALLOCSTATS") ) {
    for( int k = 0; k<=NFREE; k++ ) if( C.st_alloc[k] ) fprintf(stderr, "cls %2d: alloc %10llu  freehit %10llu  free %10llu  sz=%d\n", k,
      (unsigned long long)C.st_alloc[k], (unsigned long long)C.st_freehit[k], (unsigned long long)C.st_free[k], k<2 ? 4 : 2+4*k);
  }
  fclose(f);
  fclose(g);
  C.Quit();
  return 0;
}
