// ppmd2.hpp
// Front-end-compatible adaptation of the PPMd model from ppmd.cpp.
//
// Exposes the same API to coder0.cpp that ppmd.hpp does:
//     ppmd_Model::Init(MaxOrder, MMAX, CutOff, filesize)
//     ppmd_Model::ppmd_PrepareByte()   -> fills sqp[256] (read-only prediction)
//     ppmd_Model::ppmd_UpdateByte(c)   -> real model update for the known byte c
//     ppmd_Model::sqp[256]
//
// The model itself (suballocator, contexts, SEE, update logic) is taken
// verbatim from ppmd.cpp. The standalone Rangecoder, file I/O, timing and
// main() are dropped. As ppmd.cpp already notes, the per-byte model update is
// just ProcessByte(c, FakeRangecoder) on the encode path -- that *is*
// ppmd_UpdateByte. ppmd_PrepareByte is the read-only twin of the escape walk:
// it records every (symbol,freq,total) and escape that ProcessByte would have
// coded and folds them into sqp[] via the PPM escape-chain rule, without ever
// mutating the model (this mirrors ppmd.hpp's processSymbol*_T helpers).

namespace TREE {

typedef unsigned short word;
typedef unsigned int uint;
typedef unsigned char byte;
typedef unsigned long long qword;
template <class T> T Min(T x, T y) {
  return (x<y) ? x : y;
}
template <class T> T CLAMP(const T &X, const T &LoX, const T &HiX) {
  return (X>=LoX) ? ((X<=HiX) ? (X) : (HiX)) : (LoX);
}
struct FakeRangecoder {
  static const int f_DEC = 0;
  void rc_Process(uint, uint, uint) {}
  void rc_Arrange(uint) {}
  void rc_BProcess(uint, int&) {}
  uint rc_GetFreq(uint) { return 0; }
};
enum { BIN_TOT = 1<<15 };   // binary-context total (ppmd.cpp Rangecoder::SCALE)
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
struct Ptr_Manager;
uint Ptr_Manager__is_Text(Ptr_Manager* pmem, void* _p);
enum { BLOCKSIZE = 1u<<16, FREE_NONE = 0xFFFFu };
struct MEM_BLK {
  MEM_BLK* next;
  word first_free;
  word free_count;
  byte NumStats;
  byte _pad;        // explicit pad: makes sizeof(MEM_BLK) even (14) so every
                    // block's first context starts at an even arena offset
};
struct MEM_LIST0 {
  word next;
};
struct PPM_CONTEXT;
struct STATE0 {
  byte Symbol;
  byte Freq;
};
struct SUCC {
protected:
  uint iSuccessor;
public:
  PPM_CONTEXT* getSucc(Ptr_Manager* mem);
  void setSucc(PPM_CONTEXT* p, Ptr_Manager* mem);
  uint is_Text(Ptr_Manager* pmem);
  uint is_Succ_0() const {
    return iSuccessor==0;
  }
  void set_Succ_0() {
    iSuccessor = 0;
  }
};
struct STATE : STATE0, SUCC {
  SUCC getSUCC() const { return SUCC(*this); }
  void setSUCC(const SUCC &s) { this->SUCC::operator=(s); }
  uint is_Succ_Text(Ptr_Manager* mem) { return is_Text(mem); }
  static void swap(STATE &s1, STATE &s2) {
    STATE t;
    memcpy(&t, &s1, sizeof(t));
    memcpy(&s1, &s2, sizeof(s1));
    memcpy(&s2, &t, sizeof(s2));
  }
};

#pragma pack(pop)


inline void SWAP_STATE(STATE &s1, STATE &s2) {
  STATE::swap(s1, s2);
}
enum ContextFlagMasks { F_Rescaled = 0x04, F_HasText = 0x08, F_NextIsText = 0x10 };
struct PPM_CONTEXT {
  word _EscFreq;    // was byte; widened to word so sizeof(PPM_CONTEXT) is even (2),
                    // making CtxBytes(NU>1) even. Only the low byte carries data
                    // (bits 0..6 = escape freq, bit 7 = rescaled); high byte stays 0.
  byte NumStats() const {
    return ((const MEM_BLK*)((uintptr_t)this & ~(uintptr_t)(BLOCKSIZE-1)))->NumStats;
  }
  bool hasHeader() const { return NumStats() != 0; }
  byte EscFreq() const { return hasHeader() ? (_EscFreq & 0x7F) : byte(0); }
  void setEscFreq(uint v) {
    if( !hasHeader() ) return;
    _EscFreq = (_EscFreq & 0x80) | ((v>127) ? byte(127) : byte(v));
  }
  bool f_Rescaled() const { return hasHeader() && (_EscFreq & 0x80) != 0; }
  void setRescaled() { if( hasHeader() ) _EscFreq |= 0x80; }
  void clearRescaled() { if( hasHeader() ) _EscFreq &= 0x7F; }
  // Header is sizeof(PPM_CONTEXT) bytes when present (NumStats!=0), else absent.
  STATE0* state0s() { return (STATE0*)((byte*)this + (hasHeader() ? sizeof(PPM_CONTEXT) : 0u)); }
  SUCC* succs() {
    uint ns = NumStats();
    return (SUCC*)((byte*)this + (ns!=0 ? sizeof(PPM_CONTEXT) : 0u) + (uint(ns)+1)*sizeof(STATE0));
  }
  SUCC& succAt(STATE0* s0) { return succs()[s0 - state0s()]; }
  uint SummFreq() {
    STATE0* s0 = state0s();
    uint sum = EscFreq();
    uint ns = NumStats();
    for( uint i = 0; i<=ns; i++ ) sum += s0[i].Freq;
    return sum;
  }
  void swapStateAt(uint i, uint j) {
    STATE0* s0 = state0s(); SUCC* su = succs();
    STATE0 t0 = s0[i]; s0[i] = s0[j]; s0[j] = t0;
    SUCC ts = su[i]; su[i] = su[j]; su[j] = ts;
  }
  SUCC toSUCC(Ptr_Manager* pmem) {
    SUCC r;
    r.setSucc(this, pmem);
    return r;
  }
  SUCC toSUCC_0(void) {
    SUCC r;
    r.set_Succ_0();
    return r;
  }
  STATE0* FindState(byte sym) {
    STATE0* s0 = state0s();
    uint ns = NumStats();
    for( uint i = 0; i<=ns; i++ )
      if( s0[i].Symbol==sym ) return &s0[i];
    return 0;
  }
  STATE0* UpdateSuffixFreq(uint FSymbol, uint FFreq) {
    STATE0* s0 = state0s();
    if( NumStats()!=0 ) {
      uint i = 0;
      if( s0[0].Symbol!=FSymbol ) {
        for( i = 1; s0[i].Symbol!=FSymbol; i++ );
        if( s0[i].Freq>=s0[i-1].Freq ) { swapStateAt(i, i-1); i--; }
      }
      if( s0[i].Freq<MAX_FREQ-3 ) { uint cf = 2+(FFreq<28); s0[i].Freq += cf; }
      return &s0[i];
    } else {
      s0[0].Freq += (s0[0].Freq<14);
      return &s0[0];
    }
  }
  STATE0* UpdateHigherOrder(uint FSymbol, uint FFreq, uint s0_caller, uint ns1, uint ns, SEE_Manager &see) {
    uint cf, sf;
    STATE0* states = state0s();
    if( ns1!=0 ) {
      setEscFreq(EscFreq() + (see.QTable[ns+4]>>3));
    } else {
      states[0].Freq = (states[0].Freq<=MAX_FREQ/3) ? (2*states[0].Freq-1) : (MAX_FREQ-15);
      setEscFreq((ns>1)+ExpEscape[see.QTable[see.BSumm>>8]]);
    }
    uint sumFreq = SummFreq();   // §3a: one call; no Freq/EscFreq mutation between the two reads
    cf = (FFreq-1)*(5+sumFreq);
    sf = s0_caller+sumFreq;
    if( cf<=3*sf ) {
      cf = 1+(2*cf>sf)+(2*cf>3*sf);
      setEscFreq(EscFreq() + 4 - cf);
    } else {
      cf = 5+(cf>5*sf)+(cf>6*sf)+(cf>8*sf)+(cf>10*sf)+(cf>12*sf);
    }
    STATE0* np = &states[1+ns1];
    np->Symbol = FSymbol;
    np->Freq = cf;
    return np;
  }
  STATE0* UpdateLowerSuffix(byte sym, uint suff_ns) {
    STATE0* s0 = state0s();
    if( NumStats()!=0 ) {
      uint i = 0;
      while( s0[i].Symbol!=sym ) i++;
      byte tmp = 2*(s0[i].Freq<MAX_FREQ-1);
      s0[i].Freq += tmp;
      return &s0[i];
    } else {
      s0[0].Freq += !((suff_ns>0)&(s0[0].Freq<16));
      return &s0[0];
    }
  }
  void BequeathBinaryCtx(STATE &ctstate, byte sym, byte sym1) {
    STATE0* s0 = state0s();
    uint ns = NumStats();
    uint sf = ns ? this->SummFreq() : 0;
    uint cf, sc;
    (void)sym;
    ctstate.Symbol = sym1;
    if( ns!=0 ) {
      uint i = 0;
      while( s0[i].Symbol!=sym1 ) i++;
      cf = s0[i].Freq-1;
      sc = 1+sf-(ns+1)-cf;
      sc <<= 7;
      cf = 1+((314*cf<sc) ? (1536*cf>sc) : 2+(cf*175)/sc);
      ctstate.Freq = cf;
    } else {
      ctstate.Freq = s0[0].Freq;
    }
  }
  uint rescale(int OrderFall, STATE0*&FoundState) {
    STATE0 tmp_s0; SUCC tmp_su;
    int of, i, a, f0, sf_orig, esc_local;
    uint reallocSize;
    reallocSize = 0;
    (void)OrderFall;
    clearRescaled();
    STATE0* s0 = state0s();
    SUCC* su = succs();
    {
      uint fs_idx = FoundState - s0;
      if( fs_idx!=0 ) {
        tmp_s0 = s0[fs_idx];
        tmp_su = su[fs_idx];
        for( uint k = fs_idx; k>0; k-- ) { s0[k] = s0[k-1]; su[k] = su[k-1]; }
        s0[0] = tmp_s0;
        su[0] = tmp_su;
      }
    }
    of = (OrderFall!=0);
    if( NumStats()==255 ) of = 1;
    f0 = s0[0].Freq;
    sf_orig = SummFreq();
    esc_local = sf_orig - f0;
    s0[0].Freq = (f0+of)>>1;
    uint cur_ns = NumStats();
    uint p_idx = 0;
    for( i = 0; i<(int)cur_ns; i++ ) {
      p_idx++;
      a = s0[p_idx].Freq;
      esc_local -= a;
      a = (a+of)>>1;
      s0[p_idx].Freq = a;
      if( a>s0[p_idx-1].Freq ) {
        tmp_s0 = s0[p_idx]; tmp_su = su[p_idx];
        uint q = p_idx;
        while( q>0 && tmp_s0.Freq>s0[q-1].Freq ) { s0[q] = s0[q-1]; su[q] = su[q-1]; q--; }
        s0[q] = tmp_s0; su[q] = tmp_su;
      }
    }
    if( s0[p_idx].Freq==0 ) {
      uint dropped = 0;
      while( s0[p_idx].Freq==0 ) { dropped++; if( p_idx==0 ) break; p_idx--; }
      esc_local += dropped;
      cur_ns -= dropped;
      reallocSize = 1+cur_ns;
      if( cur_ns==0 ) {
        tmp_s0 = s0[0];
        i = (2*tmp_s0.Freq+esc_local-1)/esc_local;
        tmp_s0.Freq = i<MAX_FREQ/3 ? i : MAX_FREQ/3;
        s0[0] = tmp_s0;
        FoundState = &s0[0];
        return reallocSize;
      }
    }
    setEscFreq((esc_local+1)>>1);
    a = sf_orig - esc_local - f0;
    if( a>0 )
      a = CLAMP(uint((f0*SummFreq()-(sf_orig-esc_local)*s0[0].Freq+a-1)/a), 2U, MAX_FREQ/2U-18U);
    else
      a = 2;
    s0[0].Freq += a;
    setRescaled();
    FoundState = &s0[0];
    return reallocSize;
  }
  template<class RC>
  STATE0* processBinSymbol(int symbol, uint SuffNumStats, uint flagsValue, RC &rc, SEE_Manager &see) {
    int i, flag;
    STATE0& rs = state0s()[0];
    i = see.NS2BSIndx[SuffNumStats]+see.PrevSuccess+flagsValue+((see.RunLength>>26)&0x20);
    word &bs = see.BinSumm[see.QTable[(rs.Freq-1)&0xFF]][i];
    see.BSumm = bs;
    bs -= (see.BSumm+64)>>PERIOD_BITS;
    flag = (rc.f_DEC!=0) ? 0 : rs.Symbol!=symbol;
    rc.rc_BProcess(see.BSumm+see.BSumm, flag);
    if( flag!=0 ) {
      see.CharMask[rs.Symbol] = see.EscCount;
      see.NumMasked = 0;
      see.PrevSuccess = 0;
      return NULL;
    } else {
      bs += INTERVAL;
      rs.Freq += (rs.Freq<196);
      see.RunLength++;
      see.PrevSuccess = 1;
      return &rs;
    }
  }
  template<class RC>
  STATE0* processSymbol1(int symbol, RC &rc, SEE_Manager &see, int &OrderFall, uint &reallocSize) {
    int cnum, i, low, freq, total, flag, count;
    STATE0* s0 = state0s();
    cnum = NumStats();
    i = s0[0].Symbol;
    low = 0;
    freq = s0[0].Freq;
    total = SummFreq();
    rc.rc_Arrange(total);
    if( rc.f_DEC!=0 ) {
      count = rc.rc_GetFreq(total);
      flag = count<freq;
    } else {
      flag = i==symbol;
    }
    STATE0* found = 0;
    if( flag!=0 ) {
      see.PrevSuccess = 0;
      s0[0].Freq += 4;
      found = &s0[0];
    } else {
      see.PrevSuccess = 0;
      for( low = freq, i = 1; i<=cnum; i++ ) {
        freq = s0[i].Freq;
        flag = (rc.f_DEC!=0) ? low+freq>count : s0[i].Symbol==symbol;
        if( flag!=0 ) break;
        low += freq;
      }
      if( flag!=0 ) {
        s0[i].Freq += 4;
        if( s0[i].Freq>s0[i-1].Freq ) {
          swapStateAt(i, i-1);
          i--;
        }
        found = &s0[i];
      } else {
        freq = total-low;
        see.NumMasked = cnum;
        for( i = 0; i<=cnum; i++ ) see.CharMask[s0[i].Symbol] = see.EscCount;
        found = 0;
      }
    }
    rc.rc_Process(low, freq, total);
    if( (found!=0)&&(found->Freq>MAX_FREQ) ) reallocSize = rescale(OrderFall, found);
    return found;
  }
  template<class RC>
  STATE0* processSymbol2(int symbol, uint SuffNumStats, uint flagsValue, RC &rc, SEE_Manager &see, int &OrderFall, uint &reallocSize) {
    byte px[256];
    int c, count, low, see_freq, freq, cnum;
    SEE2_CONTEXT* psee2c;
    int flag, pl;
    int i, j, Total;
    STATE0* s0 = state0s();
    cnum = NumStats();
    if( cnum!=0xFF ) {
      psee2c = see.SEE2Cont[see.QTable[cnum+3]-4];
      psee2c += (SummFreq()>10*(cnum+1));
      psee2c += 2*(2*cnum<SuffNumStats+see.NumMasked)+flagsValue;
      see_freq = psee2c->getMean()+1;
    } else {
      psee2c = &see.DummySEE2Cont;
      see_freq = 1;
    }
    flag = 0;
    pl = 0;
    j = 0;
    for( i = 0, low = 0; i<=cnum; i++ ) {
      c = s0[i].Symbol;
      if( see.CharMask[c]!=see.EscCount ) {
        see.CharMask[c] = see.EscCount;
        low += s0[i].Freq;
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
    STATE0* found = 0;
    if( flag!=0 ) {
      if( rc.f_DEC!=0 ) {
        for( low = 0, i = 0; (low += s0[px[i]].Freq)<=count; i++ );
        j = px[i];
      } else {
        low = pl;
      }
      found = &s0[j];
      freq = found->Freq;
      if( see_freq>2 ) psee2c->Summ -= see_freq;
      psee2c->update();
      found->Freq += 4;
      if( found->Freq>MAX_FREQ ) reallocSize = rescale(OrderFall, found);
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
};
struct Ptr_Manager {
  byte* pText;
  byte* WinBeg;
  byte* WinEnd;
  uint WinSize;
  qword alloc_size;
  qword waste_size;
  byte* arena_raw;
  byte* arenaBeg;
  byte* arenaEnd;
  MEM_BLK* freeBlocks;
  MEM_BLK* pool[256];
  uint poolnum[256];
  uint cap_tbl[257];
  uint tail_tbl[257];
  static uint CtxBytes(uint NU) {
    return ((NU==1) ? 0u : uint(sizeof(PPM_CONTEXT))) + NU*sizeof(STATE);
  }
  uint blockWaste(uint NU) const {
    return uint(sizeof(MEM_BLK)) + tail_tbl[NU];
  }
  void Init(void) {
    uint NU;
    alloc_size = 0;
    waste_size = 0;
    arena_raw = 0;
    WinBeg = 0;
    for( NU = 1; NU<=256; NU++ ) {
      cap_tbl[NU] = (BLOCKSIZE-sizeof(MEM_BLK))/CtxBytes(NU);
      tail_tbl[NU] = BLOCKSIZE - sizeof(MEM_BLK) - cap_tbl[NU]*CtxBytes(NU);
    }
  }
  void buildFreeChain(void) {
    MEM_BLK* b = (MEM_BLK*)arenaEnd;
    freeBlocks = 0;
    while( (byte*)b > arenaBeg ) {
      b = (MEM_BLK*)((byte*)b-BLOCKSIZE);
      b->next = freeBlocks;
      freeBlocks = b;
    }
  }
  int StartSubAllocator(uint SASize, uint WSize) {
    WinSize = WSize;
    qword arena_sz = qword(SASize)<<20U;
    // iSuccessor (32-bit) addresses: 0=null, 1..WinSize=byte-addressed window,
    // WinSize+1..=arena with offsets stored HALVED. So the index range needed is
    // 1 (null) + WinSize + arena_sz/2 (one past the largest possible arena index).
    // It must fit in 32 bits, i.e. the max stored index (that value minus 1) must
    // be <= 0xFFFFFFFF. Reject configurations that would overflow.
    qword idx_top = qword(1) + qword(WinSize) + (arena_sz >> 1);
    if( idx_top > (qword(1) << 32) ) {
      fprintf(stderr,
        "error: 1+WinSize+arena/2 = %llu does not fit in 32-bit iSuccessor "
        "(WinSize=%u B, arena=%llu MB). Reduce MMAX and/or window size.\n",
        (unsigned long long)idx_top, WinSize, (unsigned long long)(arena_sz>>20));
      return 0;
    }
    WinBeg = new byte[WinSize];
    if( WinBeg==0 ) return 0;
    WinEnd = WinBeg+WinSize;
    qword raw_sz = arena_sz+(BLOCKSIZE-1);
    arena_raw = new byte[raw_sz];
    if( arena_raw==0 ) return 0;
    uintptr_t a = ((uintptr_t)arena_raw+(BLOCKSIZE-1)) & ~(uintptr_t)(BLOCKSIZE-1);
    arenaBeg = (byte*)a;
    arenaEnd = arenaBeg+arena_sz;
    buildFreeChain();
    memset(pool, 0, sizeof(pool));
    memset(poolnum, 0, sizeof(poolnum));
    return 1;
  }
  void StopSubAllocator(void) {
    if( WinBeg!=0 ) { delete[] WinBeg; WinBeg = 0; }
    if( arena_raw!=0 ) { delete[] arena_raw; arena_raw = 0; }
    alloc_size = 0;
    waste_size = 0;
  }
  void InitSubAllocator(void) {
    pText = WinBeg;
    buildFreeChain();
    memset(pool, 0, sizeof(pool));
    memset(poolnum, 0, sizeof(poolnum));
    alloc_size = 0;
    waste_size = 0;
  }
  void* addrOf(MEM_BLK* b, word i, uint NU) {
    return (byte*)b+sizeof(MEM_BLK)+uint(i)*CtxBytes(NU);
  }
  word indexOf(MEM_BLK* b, void* p, uint NU) {
    return word(((byte*)p-(byte*)b-sizeof(MEM_BLK))/CtxBytes(NU));
  }
  MEM_BLK* blockOf(void* p) {
    return (MEM_BLK*)((uintptr_t)p & ~(uintptr_t)(BLOCKSIZE-1));
  }
  void FormatBlock(MEM_BLK* b, uint NU) {
    uint i, c = cap_tbl[NU];
    b->first_free = 0;
    b->free_count = c;
    b->NumStats = byte(NU-1);
    b->_pad = 0;
    for( i = 0; i<c; i++ ) {
      MEM_LIST0* e = (MEM_LIST0*)addrOf(b, word(i), NU);
      e->next = (i==c-1) ? word(FREE_NONE) : word(i+1);
    }
    b->next = pool[NU-1];
    pool[NU-1] = b;
    poolnum[NU-1] += c;
    waste_size += blockWaste(NU);
  }
  PPM_CONTEXT* AllocContextNU(uint NU) {
    MEM_BLK* b;
    if( pool[NU-1]==0 ) {
      b = freeBlocks;
      if( b==0 ) return 0;
      freeBlocks = b->next;
      FormatBlock(b, NU);
    }
    b = pool[NU-1];
    word i = b->first_free;
    MEM_LIST0* e = (MEM_LIST0*)addrOf(b, i, NU);
    b->first_free = e->next;
    b->free_count -= 1;
    poolnum[NU-1] -= 1;
    if( b->free_count==0 ) pool[NU-1] = b->next;
    alloc_size += CtxBytes(NU);
    return (PPM_CONTEXT*)e;
  }
  void FreeContextNU(PPM_CONTEXT* p, uint NU) {
    if( p==0 ) return;
    MEM_BLK* b = blockOf(p);
    word i = indexOf(b, p, NU);
    int was_full = (b->free_count==0);
    MEM_LIST0* e = (MEM_LIST0*)p;
    e->next = b->first_free;
    b->first_free = i;
    b->free_count += 1;
    poolnum[NU-1] += 1;
    alloc_size -= CtxBytes(NU);
    if( was_full ) {
      b->next = pool[NU-1];
      pool[NU-1] = b;
    } else if( b->free_count==cap_tbl[NU] ) {
      if( pool[NU-1]==b ) {
        pool[NU-1] = b->next;
      } else {
        MEM_BLK* prev = pool[NU-1];
        while( prev->next!=b ) prev = prev->next;
        prev->next = b->next;
      }
      b->next = freeBlocks;
      freeBlocks = b;
      poolnum[NU-1] -= cap_tbl[NU];
      waste_size -= blockWaste(NU);
    }
  }
  PPM_CONTEXT* ExpandContext(PPM_CONTEXT* old, uint OldNU) {
    PPM_CONTEXT* p = AllocContextNU(OldNU+1);
    if( p==0 ) return 0;
    if( OldNU == 1 ) {
      p->_EscFreq = 0;
    } else {
      memcpy(p, old, sizeof(PPM_CONTEXT));
    }
    memcpy(p->state0s(), old->state0s(), OldNU*sizeof(STATE0));
    memcpy(p->succs(), old->succs(), OldNU*sizeof(SUCC));
    p->state0s()[OldNU].Symbol = 0;
    p->state0s()[OldNU].Freq = 0;
    p->succs()[OldNU].set_Succ_0();
    FreeContextNU(old, OldNU);
    return p;
  }
  PPM_CONTEXT* ShrinkContext(PPM_CONTEXT* old, uint OldNU, uint NewNU) {
    PPM_CONTEXT* p = AllocContextNU(NewNU);
    if( p==0 ) return 0;
    if( NewNU != 1 ) {
      memcpy(p, old, sizeof(PPM_CONTEXT));
    }
    memcpy(p->state0s(), old->state0s(), NewNU*sizeof(STATE0));
    memcpy(p->succs(), old->succs(), NewNU*sizeof(SUCC));
    FreeContextNU(old, OldNU);
    return p;
  }
  uint is_Text(void* _p) {
    byte* p = (byte*)_p;
    return (p<WinEnd) && (p>=WinBeg);
  }
};
inline uint Ptr_Manager__is_Text(Ptr_Manager* pmem, void* _p) {
  return pmem->is_Text(_p);
}
inline PPM_CONTEXT* SUCC::getSucc(Ptr_Manager* mem) {
  if( iSuccessor==0 ) return 0;
  if( iSuccessor<=mem->WinSize ) return (PPM_CONTEXT*)(mem->WinBeg+(iSuccessor-1));
  // Arena offsets are stored halved (all arena allocations are at even offsets).
  // Widen to 64-bit before doubling so the reconstructed offset can exceed 4GB.
  return (PPM_CONTEXT*)(mem->arenaBeg + (qword(iSuccessor-1-mem->WinSize) << 1));
}
inline void SUCC::setSucc(PPM_CONTEXT* p, Ptr_Manager* mem) {
  byte* bp = (byte*)p;
  if( bp==0 ) { iSuccessor = 0; return; }
  if( bp>=mem->WinBeg && bp<mem->WinEnd ) {
    iSuccessor = uint(bp-mem->WinBeg)+1;
  } else {
    // Halve the (always-even) arena byte offset; the 64-bit ptrdiff can exceed
    // 4GB, but offset/2 fits in 32 bits by the StartSubAllocator capacity check.
    iSuccessor = uint(qword(bp-mem->arenaBeg) >> 1)+1+mem->WinSize;
  }
}
inline uint SUCC::is_Text(Ptr_Manager* mem) {
  return (iSuccessor!=0) & (iSuccessor<=mem->WinSize);
}
struct Model {
  Ptr_Manager mem;
  SEE_Manager see;
  enum { UP_FREQ = 5 };
  int _MaxOrder;
  int _ResetPerc;
  int _MMAX;
  uint _WinSize;
  uint _filesize;
  int OrderFall;
  int order, maxorder;
  bool m_replay = false;
  bool m_replay_aborted = false;
  STATE0* FoundState;
  PPM_CONTEXT* FoundCtx;
  PPM_CONTEXT* MaxContext;
  PPM_CONTEXT* Order0;
  PPM_CONTEXT* SuffCache[MAX_O+1];
  uint NumStats_Cache[MAX_O+1];
  uint Flags_Cache[MAX_O+1];
  STATE0* StateCache[MAX_O+1];
  PPM_CONTEXT* StateCacheCtx[MAX_O+1];
  SUCC* parent_iSuc_cache[MAX_O+1];
  // §8: fused single walk over SuffCache[0..order] — halves the pointer-chase
  // and reuses each context's cache lines (NumStats header + state0s body) for
  // both the numstats cache and the flag scan. Semantically identical to the
  // former CacheNumstats() followed by CacheFlags().
  void CacheNumstatsAndFlags(int order) {
    uint nextBit = 0;
    if( mem.pText > mem.WinBeg && mem.pText[-1] >= 0x40 ) nextBit = F_NextIsText;
    for( int i = 0; i<=order; i++ ) {
      PPM_CONTEXT* pc = SuffCache[i];
      uint ns = pc->NumStats();
      NumStats_Cache[i] = ns;
      STATE0* s0 = pc->state0s();
      uint hasText = 0;
      for( uint j = 0; j<=ns; j++ ) {
        if( s0[j].Symbol >= 0x40 ) { hasText = F_HasText; break; }
      }
      uint rescaledBit = pc->f_Rescaled() ? F_Rescaled : 0;
      Flags_Cache[i] = rescaledBit + hasText + (i>0 ? nextBit : 0);
    }
  }
  void refresh_parent_cache(PPM_CONTEXT* ctx) {
    SUCC* su = ctx->succs();
    uint ns = ctx->NumStats();
    for( uint k = 0; k<=ns; k++ ) {
      byte* succ = (byte*)su[k].getSucc(&mem);
      if( succ!=0&&!mem.is_Text(succ) ) {
        PPM_CONTEXT* child = (PPM_CONTEXT*)succ;
        SUCC* sucp = &su[k];
        for( int i = 1; i<=MAX_O; i++ ) {
          if( SuffCache[i]==child )
            parent_iSuc_cache[i] = sucp;
        }
      }
    }
  }
  void CacheSuccessors(uint sym) {
    int i;
    SuffCache[0] = Order0;
    parent_iSuc_cache[0] = 0;
    for( i = (int)order-1; i>=0; i-- ) {
      if( StateCache[i]==0 ) {
        StateCache[i] = SuffCache[i]->FindState(sym);
        StateCacheCtx[i] = SuffCache[i];
      }
      if( StateCache[i]==0 ) { order = (uint)i; return; }
      SUCC& sc = StateCacheCtx[i]->succAt(StateCache[i]);
      if( sc.is_Succ_0() || sc.is_Text(&mem) ) { order = (uint)i; return; }
      PPM_CONTEXT* sc_pc = sc.getSucc(&mem);
      SuffCache[i+1] = sc_pc;
      parent_iSuc_cache[i+1] = &sc;
    }
  }
  uint Init(uint MaxOrder, uint MMAX, uint CutOff, uint filesize) {
    _MaxOrder = MaxOrder;
    _ResetPerc = CutOff ? 75 : 0;   // CutOff!=0 -> RestoreModelRare on overflow, else full reset
    _MMAX = MMAX;
    _filesize = filesize;
    // coder0 passes filesize==0 (the true length isn't visible through this API),
    // so size the text window generously rather than from filesize. The window
    // only needs to hold the bytes seen since the last reset; 128 MB covers book1
    // (and much larger inputs) with no reset. Pages are mapped lazily by the OS.
    _WinSize = (filesize != 0) ? (filesize + 1U) : (128u << 20);
    if( _WinSize < 256U ) _WinSize = 256U;
    mem.Init();
    if( !mem.StartSubAllocator(_MMAX, _WinSize) ) return 1;
    StartModelRare();
    return 0;
  }
  void Quit(void) {
    mem.StopSubAllocator();
  }
  void StartModelRare(void) {
    int i;
    OrderFall = _MaxOrder;
    mem.InitSubAllocator();
    see.initialize(_MaxOrder);
    Order0 = mem.AllocContextNU(256);
    if( Order0==0 ) exit(1);
    {
      auto &q = *Order0;
      q.clearRescaled();
      q.setEscFreq(1);
    }
    {
      STATE0* qs = Order0->state0s();
      SUCC* qu = Order0->succs();
      for( i = 0; i<256; i++ ) {
        qs[i].Symbol = i;
        qs[i].Freq = 1;
        qu[i].set_Succ_0();
      }
    }
    MaxContext = Order0;
    maxorder = order = 0;
    SuffCache[0] = Order0;
  }
  enum {
    RESTORE_ALLOC_NUM = 3,
    RESTORE_ALLOC_DEN = 4,
    RESTORE_MAX_ATTEMPTS = 16
  };
  void RestoreModelRare(void) {
    uint used = uint(mem.pText - mem.WinBeg);
    uint keep = uint(qword(used) * qword(_ResetPerc) / 100);
    qword arena_size = qword(mem.arenaEnd - mem.arenaBeg);
    qword max_alloc = arena_size * RESTORE_ALLOC_NUM / RESTORE_ALLOC_DEN;
    FakeRangecoder frc;
    int attempt = 0;
    while( keep > 0 && attempt < RESTORE_MAX_ATTEMPTS ) {
      uint shift = used - keep;
      if( shift > 0 ) memmove(mem.WinBeg, mem.WinBeg + shift, keep);
      fprintf(stderr, "\nrestore attempt %d: keep=%u (used=%u, shift=%u)\n",
              attempt, keep, used, shift);
      fflush(stderr);
      StartModelRare();
      m_replay = true;
      m_replay_aborted = false;
      uint i = 0;
      for( ; i < keep; i++ ) {
        ProcessByte(uint(mem.WinBeg[i]), frc);
        if( m_replay_aborted || mem.alloc_size > max_alloc ) break;
      }
      m_replay = false;
      if( i == keep ) {
        fprintf(stderr, "restore done: alloc_size=%llu pText=+%u/%u\n",
                (unsigned long long)mem.alloc_size,
                uint(mem.pText - mem.WinBeg), keep);
        fflush(stderr);
        return;
      }
      fprintf(stderr, "restore retry: alloc_size=%llu hit %llu cap at byte %u/%u\n",
              (unsigned long long)mem.alloc_size,
              (unsigned long long)max_alloc, i, keep);
      fflush(stderr);
      used = keep;
      keep = uint(qword(keep) * qword(_ResetPerc) / 100);
      attempt++;
    }
    fprintf(stderr, "restore gave up: dropping window entirely\n");
    fflush(stderr);
    StartModelRare();
  }
  void FinishRescale(PPM_CONTEXT*&ctx, STATE0*&FoundState, uint newNU) {
    uint oldNU = ctx->NumStats()+1;
    if( newNU<oldNU ) {
      PPM_CONTEXT* new_ctx = mem.ShrinkContext(ctx, oldNU, newNU);
      if( new_ctx!=0 ) {
        if( FoundState!=0 ) {
          uint idx = FoundState - ctx->state0s();
          FoundState = new_ctx->state0s() + idx;
        }
        SuffCache[order] = new_ctx;
        if( order>0 && parent_iSuc_cache[order]!=0 )
          parent_iSuc_cache[order]->setSucc(new_ctx, &mem);
        ctx = new_ctx;
      }
    }
    refresh_parent_cache(ctx);
  }
  template<class RC>
  uint ProcessByte(uint c, RC& rc) {
    PPM_CONTEXT* MinContext;
    PPM_CONTEXT* p;
    uint reallocSize;
    CacheNumstatsAndFlags(order);
    maxorder = order;
    MinContext = SuffCache[order];
    reallocSize = 0;
    if( NumStats_Cache[order]!=0 ) {
      FoundState = MinContext->processSymbol1(c, rc, see, OrderFall, reallocSize);
      refresh_parent_cache(MinContext);
    } else {
      FoundState = MinContext->processBinSymbol(c, NumStats_Cache[order-1], Flags_Cache[order], rc, see);
    }
    if( reallocSize!=0 ) {
      FinishRescale(MinContext, FoundState, reallocSize);
      uint hasText = 0;
      STATE0* s0 = MinContext->state0s();
      uint ns = MinContext->NumStats();
      for( uint j = 0; j<=ns; j++ ) if( s0[j].Symbol >= 0x40 ) { hasText = F_HasText; break; }
      uint rescaledBit = MinContext->f_Rescaled() ? F_Rescaled : 0;
      Flags_Cache[order] = rescaledBit + hasText + (Flags_Cache[order] & F_NextIsText);
    }
    while( FoundState==0 ) {
      do {
        OrderFall++;
        order--;
        MinContext = SuffCache[order];
      } while( order>0 && NumStats_Cache[order]==see.NumMasked );
      reallocSize = 0;
      FoundState = MinContext->processSymbol2(c, (order>0 ? NumStats_Cache[order-1] : 0), Flags_Cache[order], rc, see, OrderFall, reallocSize);
      refresh_parent_cache(MinContext);
      if( reallocSize!=0 ) {
        FinishRescale(MinContext, FoundState, reallocSize);
        uint hasText = 0;
        STATE0* s0 = MinContext->state0s();
        uint ns = MinContext->NumStats();
        for( uint j = 0; j<=ns; j++ ) if( s0[j].Symbol >= 0x40 ) { hasText = F_HasText; break; }
        Flags_Cache[order] = hasText + (Flags_Cache[order] & F_NextIsText);
      }
    }
    FoundCtx = MinContext;
    if( rc.f_DEC!=0 ) c = FoundState->Symbol;
    *mem.pText++ = c;
    memset(StateCache, 0, sizeof(StateCache));
    memset(StateCacheCtx, 0, sizeof(StateCacheCtx));
    StateCache[order] = FoundState;
    StateCacheCtx[order] = MinContext;
    SUCC& fSucc = MinContext->succAt(FoundState);
    if( (order<_MaxOrder)||fSucc.is_Text(&mem) ) {
      p = UpdateModel(MinContext);
    } else {
      p = fSucc.getSucc(&mem);
    }
    MaxContext = p;
    if( p==0 ) {
      if( m_replay ) { m_replay_aborted = true; return c; }
      if( _ResetPerc>=1 && _ResetPerc<=99 ) {
        fprintf(stderr," [reset]\n"),fflush(stderr);
        RestoreModelRare();
      } else {
        fprintf(stderr," [reset]\n"),fflush(stderr);
        StartModelRare();
      }
    }
    CacheSuccessors(c);
    if( mem.pText>=mem.WinEnd ) {
      if( m_replay ) { m_replay_aborted = true; return c; }
      if( _ResetPerc>=1 && _ResetPerc<=99 ) {
        fprintf(stderr," [reset (text)]\n"),fflush(stderr);
        RestoreModelRare();
      } else {
        fprintf(stderr," [reset (text)]\n"),fflush(stderr);
        StartModelRare();
      }
    }
    return c;
  }
  PPM_CONTEXT* UpdateModel(PPM_CONTEXT* MinContext) {
    byte FSymbol;
    uint ns1, ns, s0_caller, FFreq, f_order0;
    SUCC iSuccessor, iFSuccessor;
    PPM_CONTEXT* pc;
    STATE0* p;
    int i;
    p = NULL;
    FSymbol = FoundState->Symbol;
    FFreq = FoundState->Freq;
    iFSuccessor = MinContext->succAt(FoundState);
    if( order>0 ) {
      pc = SuffCache[order-1];
      p = pc->UpdateSuffixFreq(FSymbol, FFreq);
      refresh_parent_cache(pc);
      StateCache[order-1] = p;
      StateCacheCtx[order-1] = pc;
    }
    if( (OrderFall==0)&&(!iFSuccessor.is_Succ_0()) ) {
      SUCC s = CreateSuccessors(1, p, (order>0 ? SuffCache[order-1] : 0), MinContext);
      if( s.is_Succ_0() ) return 0;
      MaxContext = s.getSucc(&mem);
      return MaxContext;
    }
    iSuccessor.setSucc((PPM_CONTEXT*)mem.pText, &mem);
    f_order0 = 0;
    if( !iFSuccessor.is_Succ_0() ) {
      if( iFSuccessor.is_Text(&mem) )
        iFSuccessor = CreateSuccessors(0, p, (order>0 ? SuffCache[order-1] : 0), MinContext);
    } else {
      iFSuccessor = Order0->toSUCC(&mem);
      MinContext->succAt(FoundState).setSucc((PPM_CONTEXT*)mem.pText, &mem);
      OrderFall++;
      f_order0 = 1;
    }
    s0_caller = MinContext->SummFreq()-FFreq;
    ns = NumStats_Cache[order];
    for( i = maxorder; i>order; i-- ) {
      pc = SuffCache[i];
      ns1 = NumStats_Cache[i];
      PPM_CONTEXT* pc_new = mem.ExpandContext(pc, 1+ns1);
      if( pc_new==0 ) return 0;
      if( parent_iSuc_cache[i] ) parent_iSuc_cache[i]->setSucc(pc_new, &mem);
      SuffCache[i] = pc_new;
      pc = pc_new;
      STATE0* np = pc->UpdateHigherOrder(FSymbol, FFreq, s0_caller, ns1, ns, see);
      pc->succAt(np) = iSuccessor;
      StateCache[i] = np;
      StateCacheCtx[i] = pc;
      refresh_parent_cache(pc);
    }
    --OrderFall;
    order = f_order0 ? 0 : ++order;
    MaxContext = iFSuccessor.getSucc(&mem);
    return MaxContext;
  }
  SUCC CreateSuccessors(uint Skip, STATE0* p, PPM_CONTEXT* p_ctx, PPM_CONTEXT* pc) {
    byte sym, sym1;
    uint i;
    STATE0* ps[MAX_O];
    PPM_CONTEXT* ps_ctx[MAX_O];
    uint pps_n = 0;
    STATE ctstate;
    SUCC iUpBranch;
    PPM_CONTEXT* pc1;
    byte* upPtr;
    sym = FoundState->Symbol;
    iUpBranch = pc->succAt(FoundState);
    i = order;
    if( Skip==0 ) {
      ps[pps_n] = FoundState;
      ps_ctx[pps_n] = pc;
      pps_n++;
      if( i==0 ) goto NO_LOOP;
    }
    if( p!=0 ) { pc = SuffCache[--i]; goto LOOP_ENTRY; }
    if( i==0 ) goto NO_LOOP;
    do {
      pc = SuffCache[--i];
      p = pc->UpdateLowerSuffix(sym, (i>0 ? NumStats_Cache[i-1] : 0));
      p_ctx = pc;
      StateCache[i] = p;
      StateCacheCtx[i] = pc;
LOOP_ENTRY:
      if( p_ctx->succAt(p).getSucc(&mem)!=iUpBranch.getSucc(&mem) ) {
        pc = p_ctx->succAt(p).getSucc(&mem);
        break;
      }
      ps[pps_n] = p;
      ps_ctx[pps_n] = p_ctx;
      pps_n++;
    } while( i>0 );
NO_LOOP:
    if( pc==0 ) pc = Order0;
    if( pps_n==0 ) return pc->toSUCC(&mem);
    upPtr = (byte*)iUpBranch.getSucc(&mem);
    sym1 = *upPtr;
    pc->BequeathBinaryCtx(ctstate, sym, sym1);
    ctstate.setSucc((PPM_CONTEXT*)(upPtr+1), &mem);
    do {
      pc1 = mem.AllocContextNU(1);
      if( pc1==0 ) return pc1->toSUCC_0();
      pc1->state0s()[0].Symbol = ctstate.Symbol;
      pc1->state0s()[0].Freq = ctstate.Freq;
      pc1->succs()[0] = (SUCC&)ctstate;
      pc = pc1;
      pps_n--;
      ps_ctx[pps_n]->succAt(ps[pps_n]).setSucc(pc, &mem);
    } while( pps_n!=0 );
    return pc->toSUCC(&mem);
  }

  // ===========================================================================
  // ppmd.hpp-compatible front-end API
  // ===========================================================================

  struct qsym {
    uint sym, freq, total;
    void store( uint s, uint f, uint t ) { sym = s; freq = f; total = t; }
  };
  qsym SQ[2048];   // a symbol is recorded at most once (then masked), so this
  uint SQ_ptr;     // bounds at 256 syms + (order+1) escapes
  uint sqp[256];

  // Read-only twin of processSymbol1: record the whole top context + its escape.
  void processSymbol1_T( PPM_CONTEXT* q ) {
    STATE0* s0 = q->state0s();
    int cnum  = q->NumStats();
    int total = q->SummFreq();
    int low   = 0;
    for( int i = 0; i <= cnum; i++ ) {
      SQ[SQ_ptr++].store( s0[i].Symbol, s0[i].Freq, total );
      low += s0[i].Freq;
    }
    see.NumMasked = cnum;
    for( int i = 0; i <= cnum; i++ ) see.CharMask[s0[i].Symbol] = see.EscCount;
    SQ[SQ_ptr++].store( 256, total - low, total );
  }

  // Read-only twin of processBinSymbol: BinSumm is read but NOT decremented.
  void processBinSymbol_T( PPM_CONTEXT* q, uint SuffNumStats, uint flagsValue ) {
    STATE0& rs = q->state0s()[0];
    int i = see.NS2BSIndx[SuffNumStats] + see.PrevSuccess + flagsValue + ((see.RunLength >> 26) & 0x20);
    uint BSumm = see.BinSumm[ see.QTable[(rs.Freq - 1) & 0xFF] ][i];
    SQ[SQ_ptr++].store( rs.Symbol, BSumm + BSumm, BIN_TOT );
    SQ[SQ_ptr++].store( 256, BIN_TOT - BSumm - BSumm, BIN_TOT );
    see.CharMask[rs.Symbol] = see.EscCount;
    see.NumMasked = 0;
  }

  // Read-only twin of processSymbol2: SEE mean is read but NOT updated.
  void processSymbol2_T( PPM_CONTEXT* q, uint SuffNumStats, uint flagsValue ) {
    STATE0* s0 = q->state0s();
    int cnum = q->NumStats();
    SEE2_CONTEXT* psee2c;
    int see_freq;
    if( cnum != 0xFF ) {
      psee2c  = see.SEE2Cont[ see.QTable[cnum + 3] - 4 ];
      psee2c += (q->SummFreq() > 10 * (cnum + 1));
      psee2c += 2 * (2 * cnum < SuffNumStats + see.NumMasked) + flagsValue;
      see_freq = psee2c->getMean() + 1;
    } else {
      psee2c   = &see.DummySEE2Cont;
      see_freq = 1;
    }
    int low = 0;
    for( int i = 0; i <= cnum; i++ ) {
      int c = s0[i].Symbol;
      if( see.CharMask[c] != see.EscCount ) low += s0[i].Freq;
    }
    int Total = see_freq + low;
    for( int i = 0; i <= cnum; i++ ) {
      int c = s0[i].Symbol;
      if( see.CharMask[c] != see.EscCount ) {
        SQ[SQ_ptr++].store( c, s0[i].Freq, Total );
        see.CharMask[c] = see.EscCount;
      }
    }
    SQ[SQ_ptr++].store( 256, see_freq, Total );
    see.NumMasked = cnum;
  }

  // Fold the recorded (sym,freq,total)/escape chain into per-symbol masses via
  // the PPM escape-chain rule (identical to ppmd.hpp's ConvertSQ).
  void ConvertSQ( void ) {
    uint i, c, freq, total, prob;
    uint cum = 0xFFFFFF00;
    memset( sqp, 0, sizeof(sqp) );
    for( i = 0; i < SQ_ptr; i++ ) {
      c = SQ[i].sym; freq = SQ[i].freq; total = SQ[i].total;
      prob = uint( qword(cum) * qword(freq) / total );
      if( c < 256 ) sqp[c] = prob + 1;
      else          cum    = prob;
    }
  }

  // Read-only prediction pass: same suffix walk as ProcessByte's escape loop,
  // but with the _T helpers. The model is left untouched (only EscCount is
  // advanced, which simply unmasks this pass's CharMask writes).
  void ppmd_PrepareByte( void ) {
    SQ_ptr = 0;
    int saved_NumMasked = see.NumMasked;
    see.NumMasked = 0;

    CacheNumstatsAndFlags( order );

    int o = order;
    PPM_CONTEXT* MinContext = SuffCache[o];
    if( NumStats_Cache[o] != 0 ) {
      processSymbol1_T( MinContext );
    } else {
      processBinSymbol_T( MinContext, (o > 0 ? NumStats_Cache[o - 1] : 0), Flags_Cache[o] );
    }
    while( 1 ) {
      do {
        if( o == 0 ) goto Break;
        o--;
        MinContext = SuffCache[o];
      } while( NumStats_Cache[o] == see.NumMasked );
      processSymbol2_T( MinContext, (o > 0 ? NumStats_Cache[o - 1] : 0), Flags_Cache[o] );
    }
    Break:
    see.EscCount++;
    see.NumMasked = saved_NumMasked;

    ConvertSQ();
  }

  // Real model update for the known byte c (encode path, no entropy coding):
  // ppmd.cpp's FakeRangecoder + ProcessByte already implement exactly this.
  void ppmd_UpdateByte( uint c ) {
    FakeRangecoder frc;
    ProcessByte( c, frc );
  }
};

typedef Model ppmd_Model;

}

using TREE::ppmd_Model;
