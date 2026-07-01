
// 4gb fix

namespace DUMMY {

typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned short word;
typedef unsigned long long qword;
typedef long long sqword;
static const char EscIdxFreqBoost[16] = {-3,-2,-1,-1,-1,0,0,0,0,0,0,0,2,3,4,0};
static const char FreqRescaleTab[36] = {2,3,3,3,4,4,4,5,5,6,6,6,7,7,7,7,8,8,8,8,8,9,9,9,9,9,9,10,10,10,10,10,10,10,10,10};
static const char SseWeight1PopBits[16] = {10,-19,-24,17,34,27,2,5,2,31,20,-16,17,30,0,0};
static const char SseWeight1BiasPerDim[16] = {45,73,84,90,116,122,121,126,-106,-109,-100,-75,-96,-38,0,0};
static const char SseWeight2PopBits[16] = {5,-4,12,7,47,0,-3,1,16,25,5,11,-2,0,0,0};
static const char SseWeight2BiasPerDim[24] = {1,2,5,1,2,5,0,5,20,18,24,24,39,44,64,65,77,90,99,99,124,-119,-82,-53};
static const char BinSseInitPerBucket[8] = {-128,-64,-56,-32,-40,0,0,0};
static const char PredWeightInitPerBucket[28] = {76,-118,-122,-95,-70,0,0,0,0,0,0,0,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32};
static const byte RLQBounds[16] = {1,2,3,4,5,6,8,11,13,20,50,68,117,0,0,0};
static const byte SSE0QBounds[16] = {0,1,3,10,14,27,44,65,88,125,byte(-87),byte(-42),byte(-15),0,0,0};
static const byte SSE1QBounds[8] = {1,2,7,17,0,0,0,0};
static const byte SEEQBounds[8] = {1,13,43,byte(-74),0,0,0,0};
static const char predBoostTab[32]= {17,63,68,26,110,20,119,24,110,20,-85,10,-84,25,-126,22,105,74,60,84,44,92,23,100,44,121,0,0,0,0,0,0};
struct SubAllocator {
  enum { UNIT_SIZE = 12, N_INDEXES = 38, MAX_O = 256, MEM_DIVISOR = 10 };
#pragma pack(1)
  struct MEM_BLK {
    union {
      struct {
        byte NU, Stamp;
      };
      uint QueueSize;
    };
    uint Next;
    uint Prev;
    MEM_BLK* next(SubAllocator* M) const { return (MEM_BLK*)M->Indx2Ptr(Next); }
    MEM_BLK* prev(SubAllocator* M) const { return (MEM_BLK*)M->Indx2Ptr(Prev); }
    uint avail() const { return (QueueSize!=0); }
    MEM_BLK* unlinkPrev(SubAllocator* M) {
      MEM_BLK* p = prev(M);
      Prev = p->Prev;
      p->prev(M)->Next = M->Ptr2Indx(this);
      QueueSize--;
      return p;
    }
    void linkNext(SubAllocator* M, MEM_BLK* p, uint NU, byte Stamp=0xFF ) {
      p->Stamp = Stamp;
      p->NU = NU;
      p->Prev = M->Ptr2Indx(this);
      p->Next = Next;
      Next = next(M)->Prev = M->Ptr2Indx(p);
      QueueSize++;
    }
    void linkPrev(SubAllocator* M, MEM_BLK* p, uint NU, byte Stamp=0xFF ) {
      p->Stamp = Stamp;
      p->NU = NU;
      p->Next = M->Ptr2Indx(this);
      p->Prev = Prev;
      Prev = prev(M)->Next = M->Ptr2Indx(p);
      QueueSize++;
    }
    MEM_BLK* unlinkNext(SubAllocator* M) {
      MEM_BLK* p = next(M);
      Next = p->Next;
      p->next(M)->Prev = M->Ptr2Indx(this);
      QueueSize--;
      return p;
    }
    void unlink(SubAllocator* M) {
      next(M)->Prev = Prev;
      prev(M)->Next = Next;
    }
    uint canMerge() const {
      return (Stamp==byte(-1));
    }
  };
#pragma pack()
  byte* HeapNull;
  byte* UnitsStart;
  void* HeapStart;
  qword SubAllocatorSize;
  // Fixed packing boundary (in (ptr-HeapNull) addr space), captured once at
  // heap init = initial UnitsStart - HeapNull. Everything below it (header,
  // text, and any units later carved *down* from UnitsStart) is stored raw;
  // everything at/above it (the original Lo/Hi unit region, 12-byte aligned to
  // the initial UnitsStart) is packed at UNIT_SIZE granularity. Using a fixed
  // boundary (recomputed deterministically each restart) rather than the live
  // UnitsStart keeps index<->ptr round-tripping correct even after UnitsStart
  // moves down under memory pressure, extending the addressable heap past 4GB.
  qword UnitsStartLim;
  byte Units2Indx[128];
  byte Indx2Units[48];
  byte *HiUnit, *LoUnit, *pText;
  MEM_BLK* BList;
  int GlueCount, CutOffCount;
  void* Indx2Ptr(uint indx) {
    qword lim = UnitsStartLim;
    qword addr = ((qword)indx >= lim) ? ((qword)indx - lim) * UNIT_SIZE + lim : (qword)indx;
    return (void*)(HeapNull + addr);
  }

  uint Ptr2Indx(const void* p) {
    qword addr = (byte*)p - HeapNull;
    qword lim = UnitsStartLim;
    qword indx = (addr >= lim) ? (addr - lim) / UNIT_SIZE + lim : addr;
    return (uint)indx;
  }

  void UnitsCpy_(void* Dest, const void* Src, uint NU) {
    uint* p1 = (uint*)Dest;
    const uint* p2 = (const uint*)Src;
    if (NU & 1) {
      p1[0] = p2[0]; p1[1] = p2[1]; p1[2] = p2[2];
      p1 += 3; p2 += 3;
    }
    for (NU >>= 1; NU != 0; --NU, p1 += 6, p2 += 6) {
      p1[0] = p2[0]; p1[1] = p2[1]; p1[2] = p2[2];
      p1[3] = p2[3]; p1[4] = p2[4]; p1[5] = p2[5];
    }
  }

  byte* AllocUnits_(uint sizeClass) {
    MEM_BLK* queue = &BList[sizeClass];
    if (queue->avail()) {
      return (byte*)queue->unlinkNext(this);
    }
    uint byteOff = UNIT_SIZE * (uint)Indx2Units[sizeClass];
    if (LoUnit + byteOff > HiUnit) {
      return AllocUnitsRare(sizeClass);
    }
    byte* result = LoUnit;
    bool isExact = (LoUnit + byteOff == HiUnit);
    LoUnit += byteOff;
    if (!isExact) {
      *(uint*)(byteOff + result) = 0;
    }
    return result;
  }

  void FreeUnits_(void* ptr, uint NU) {
    uint indx = Units2Indx[NU - 1];
    NU = Indx2Units[indx];
    MEM_BLK* p = (MEM_BLK*)ptr;
    if (!p[NU].canMerge())
      BList[indx].linkNext(this, p, NU);
    else
      FreeUnitsRare(p, NU);
  }

  void* MoveUnits_(void* OldPtr, uint NU) {
    uint indx = Units2Indx[NU - 1];
    uint NewNU = Indx2Units[indx];
    MEM_BLK* p = (MEM_BLK*)OldPtr;
    if (!p[NewNU].canMerge() || !BList[indx].avail()) return OldPtr;
    void* NewPtr = BList[indx].unlinkNext(this);
    UnitsCpy_(NewPtr, OldPtr, NU);
    FreeUnitsRare(p, NewNU);
    return NewPtr;
  }

  void* ShrinkUnits_(void* OldPtr, uint OldNU, uint NewNU) {
    uint i0 = Units2Indx[OldNU - 1];
    uint i1 = Units2Indx[NewNU - 1];
    if (i0 == i1) return OldPtr;
    if (BList[i1].avail()) {
      void* ptr = BList[i1].unlinkNext(this);
      UnitsCpy_(ptr, OldPtr, NewNU);
      FreeUnits_(OldPtr, Indx2Units[i0]);
      return ptr;
    }
    NewNU = Indx2Units[i1];
    FreeUnitsRare((MEM_BLK*)OldPtr + NewNU, Indx2Units[i0] - NewNU);
    return OldPtr;
  }

  void GlueFreeBlocks_() {
    MEM_BLK* sentinel = (MEM_BLK*)((char*)HeapStart + SubAllocatorSize - UNIT_SIZE);
    int sentinelField2 = sentinel->Prev;
    qword sentinelField1;
    memcpy(&sentinelField1, sentinel, sizeof sentinelField1);
    for (uint queueIdxOuter = 0; queueIdxOuter < N_INDEXES; ++queueIdxOuter) {
      MEM_BLK* queue = &BList[queueIdxOuter];
      queue->linkNext(this, sentinel, 1, (byte)-2);
      MEM_BLK* blockWalker = queue->unlinkPrev(this);
      for (; blockWalker != sentinel; --queue->QueueSize) {
        FreeUnitsRare(blockWalker, Indx2Units[Units2Indx[blockWalker->NU - 1]]);
        MEM_BLK* prevBlk = queue->prev(this);
        queue->Prev = prevBlk->Prev;
        prevBlk->prev(this)->Next = Ptr2Indx(queue);
        blockWalker = prevBlk;
      }
    }
    memcpy(sentinel, &sentinelField1, sizeof sentinelField1);
    sentinel->Prev = sentinelField2;
  }

  byte* AllocUnitsRare(uint unitsIdx) {
    byte* result;
    uint reqSizeIdx = unitsIdx;
    uint reqUnits = Indx2Units[unitsIdx];
    while (++unitsIdx != N_INDEXES) {
      MEM_BLK* freeQueue = &BList[unitsIdx];
      if (freeQueue->avail()) {
        byte* result = (byte*)freeQueue->unlinkNext(this);
        uint biggerUnits = Indx2Units[unitsIdx];
        FreeUnitsRare(result + UNIT_SIZE*reqUnits, biggerUnits - reqUnits);
        return result;
      }
    }
    if( CutOffCount ) {
      if( GlueCount )
        return 0;
      qword textBufBytes = UNIT_SIZE*reqUnits;
      if( UnitsStart - textBufBytes <= pText ) {
        return 0;
      } else {
        result = UnitsStart - textBufBytes;
        UnitsStart -= textBufBytes;
      }
    } else {
      GlueFreeBlocks_();
      CutOffCount = 1;
      result = AllocUnits_(Units2Indx[Indx2Units[reqSizeIdx] - 1]);
    }
    return result;
  }

  uint CoalesceTrailingBlocks_(byte* blockAddr, uint sizeClass) {
    while (true) {
      MEM_BLK* probe = (MEM_BLK*)(blockAddr + UNIT_SIZE*sizeClass);
      if (probe->Stamp != (byte)-1) break;
      probe->unlink(this);
      --BList[Units2Indx[probe->NU - 1]].QueueSize;
      sizeClass += probe->NU;
    }
    return sizeClass;
  }

  void SplitOverlargeChunksOff_(byte*& blockAddr, uint& sizeClass) {
    if (sizeClass <= 0x80) return;
    uint chunksOver128 = (sizeClass - 1) >> 7;
    for (uint k = 0; k < chunksOver128; ++k) {
      BList[37].linkNext(this, (MEM_BLK*)blockAddr, 0x80);
      blockAddr += 1536;
    }
    sizeClass -= 128 * chunksOver128;
  }

  void TailSplitAndInsert_(byte* blockAddr, uint sizeClass) {
    int biggerSizeClass = Units2Indx[sizeClass - 1];
    int biggerUnits = Indx2Units[biggerSizeClass];
    if (sizeClass != (uint)biggerUnits) {
      --biggerSizeClass;
      biggerUnits = Indx2Units[biggerSizeClass];
      int deltaUnits = sizeClass - biggerUnits;
      BList[deltaUnits - 1].linkNext(this, (MEM_BLK*)(blockAddr + UNIT_SIZE*biggerUnits), deltaUnits);
    }
    BList[biggerSizeClass].linkNext(this, (MEM_BLK*)blockAddr, biggerUnits);
  }

  void FreeUnitsRare(void* blockAddr, uint sizeClass) {
    byte* p = (byte*)blockAddr;
    sizeClass = CoalesceTrailingBlocks_(p, sizeClass);
    SplitOverlargeChunksOff_(p, sizeClass);
    TailSplitAndInsert_(p, sizeClass);
  }

  void InitIndx2UnitsTables_() {
    for (uint i = 0; i < 5; ++i) Indx2Units[0 + i] = 1 + i;
    for (uint i = 0; i < 3; ++i) Indx2Units[5 + i] = 7 + 2*i;
    for (uint i = 0; i < 3; ++i) Indx2Units[8 + i] = 14 + 3*i;
    for (uint i = 0; i < 27; ++i) Indx2Units[11 + i] = 24 + 4*i;
    for (uint i = 0, j = 0; i < 0x80; ++i) {
      if (Indx2Units[j] < i + 1) ++j;
      Units2Indx[i] = j;
    }
  }

  qword PPMIIGetCurrentModelSize() {
    if (!SubAllocatorSize) return 0;
    qword result = (qword)pText - (qword)UnitsStart + (qword)LoUnit + SubAllocatorSize - (qword)HiUnit;
    for (uint i = 0; i < N_INDEXES; ++i) {
      qword bytesUsed = (qword)BList[i].QueueSize * UNIT_SIZE * Indx2Units[i];
      result -= bytesUsed;
    }
    return result;
  }
};
// ---------------------------------------------------------------------------
// ppmd3 harvest support: write-journal (in-place undo) + fake rangecoder.
// The strong ppmd.cpp prediction code is run UNCHANGED for ppmd_PrepareByte
// (always-escape via FakeRangecoder), but every persistent model write is
// routed through WJ()/JWP()/jmemcpy()/jmemset() so its prior bytes are logged
// while g_harvest!=0; jundo() rolls the log back, leaving the model bit-exact.
// When g_harvest==0 (ppmd_UpdateByte) the macros are transparent (just write).
// ---------------------------------------------------------------------------
struct PPMD3_JEnt { void* p; uint n; };
static const uint PPMD3_JMAXE = 1u<<19;     // max journal entries / pass
static const uint PPMD3_JMAXB = 1u<<23;     // max journaled bytes  / pass (8MB)
static PPMD3_JEnt ppmd3_jent[PPMD3_JMAXE];
static byte       ppmd3_jval[PPMD3_JMAXB];
static uint       ppmd3_jn  = 0;            // entry count
static uint       ppmd3_jvp = 0;            // byte cursor
static int        ppmd3_harvest = 0;        // 1 while harvesting (PrepareByte)
static uint       ppmd3_jovf = 0;           // overflow flag (sizing guard)

// Compile-time-sized log (Lever 1): N constant => __builtin_memcpy lowers to a
// single mov, no call, no runtime size branch. (A per-pass first-touch dedup
// tag table was tried and measured a net LOSS here: after Lever 1 each log is a
// hot-L1 mov, cheaper than a scattered tag-table probe, so dedup only adds
// cache-miss-prone bookkeeping. Kept simple.)
template<uint N> static inline void ppmd3_jlogN(void* p) {
  if( ppmd3_jn>=PPMD3_JMAXE || ppmd3_jvp+N>PPMD3_JMAXB ) { ppmd3_jovf=1; return; }
  PPMD3_JEnt& e = ppmd3_jent[ppmd3_jn++];
  e.p=p; e.n=N;
  __builtin_memcpy(ppmd3_jval+ppmd3_jvp, p, N);
  ppmd3_jvp += N;
}
static inline void ppmd3_jundo(void) {
  while( ppmd3_jn ) {
    PPMD3_JEnt& e = ppmd3_jent[--ppmd3_jn];
    uint n = e.n; ppmd3_jvp -= n;
    byte* s = ppmd3_jval+ppmd3_jvp;
    if(n==8)      __builtin_memcpy(e.p,s,8);
    else if(n==4) __builtin_memcpy(e.p,s,4);
    else if(n==2) __builtin_memcpy(e.p,s,2);
    else if(n==1) *(byte*)e.p = *s;
    else          memcpy(e.p,s,n);
  }
  ppmd3_jvp = 0;
}
#define JREG(ptr,nbytes) do{ if(ppmd3_harvest) ppmd3_jlogN<(uint)(nbytes)>((void*)(ptr)); }while(0)

struct FakeRangecoder {
  // Update (encode, f_DEC=0): every method a no-op; match decided by sym!=Symbol.
  // Prepare(decode, f_DEC=1): IsDecodeMatched=>false forces escape everywhere.
  uint getSubRange(uint,uint){ return 0; }
  void encodeSymbol(uint){}
  void encodeEscape(uint){}
  bool IsDecodeMatched(uint){ return false; }
  void DecodeNotMatched(uint){}
  void commitRange(){}
  void initEncoder(FILE*){}
  void initDecoder(FILE*){}
  void EncodeNormalize(){}
  void DecodeNormalize(){}
  void Flush(){}
};

struct Model : SubAllocator {
  void* FileName;
  int f_ENC, f_LOG;
  struct SseCounter {
    union {
      uint sum;
      struct { word hits, predHi; };
    };
    word freq0;
    word freq1;
  };
  uint abs32( int x ) { return x >= 0 ? x : -x; }

  uint BinMapTable[16];
  char SSE1QTable[128], SEEQTable[256], NextBinFreq[128];
  byte SSE0[256], SSE1[256], SSE0QTable[256];
  int BinSse[0x280];
  enum { SSE2_BASE = 0x3C000 };
  SseCounter SseWeight1[SSE2_BASE + 0x38020];
  SseCounter EscMirrorSse[0x1020];
  int PredWeight[0xA1C];
  char SymType[256];
  int SymMask[256];
  int SymLastCtx[256];
  int SymLastCtx2[256];
  int MatchPosBySym[256];
  char BijectMap[0x40000];
  int bijectCellSink;
  char foundSymHist;
  byte* bijectCellPtr;
  int SseScale, hintSymRecent, FoundSymbol, HashSeed2, HashSeed1, RSContext;
  struct STATE;
  STATE* CtxChain[0x100];
  int Sse1[0x60040];
  int Sse2[0x30040];
  int Sse3[0x2A03E];
  char SymFreqs[256];
  SseCounter SseWeight2[0x4020];
  int pairHashIdxByEpoch[2];
  int sse2HistRotation, b31Key, Order1Ctx, b31KeyPrev, order1CtxSaved;
  int matchHintByte, bdiffSaved, bdiffStickyCnt, matchHashSy;
  int matchPosAge, matchEpoch2, sseState3Hash;
  int RecentPos[0x1000];
  int LastEpochBySym[256];
  int SseMatch[0x200040];
  int MatchPosPrev[131072];
  int MatchPosTable[0x10000];
  int SparseBitmapA[0x4000];
  int SparseBitmapB[0x10000];
  struct PPM_CONTEXT;
  PPM_CONTEXT* MaxContext;
  int* PredWeightAG;
  int* PredWeightBG;
  PPM_CONTEXT* RootContext;
  int InitsCount, MaxOrder, CutOff, Interrupted, RunLength, SymCount;
  int SseCtx3, EscapeSymbol, SymEpoch, SseCtxExtra, OrderFall, SseCtx, SseCtx2;
  int PrevSymbol, OrderCtxSeed, SseSeed, EscIndexSeed, NMasked;
  int predWeightSink2[2];
  int predDeltaNum, predDeltaDen, runLengthInit, OrderFall0;
  byte freqmap[0x100];
  int sseTot, sseCum, orderBumpVariance, predWeightSink;
  int predBaseDeltaA, predBaseDeltaB, binSseDeltaHi, binSseDeltaLo;
  int predRescaleDiv, cumFreqAcc, cumFreqSseSave, sseIdxStorage;
  STATE* FoundState;
  struct QSlotScratch { SseCounter* q[7]; int wDelta[7]; };
  QSlotScratch qs;
  struct SlotPtrs {
    SseCounter *BinSseCenterG, *BinSseHiG, *BinSseLoG, *PredBaseAG, *PredBaseBG;
    int *Sse1SlotG, *Sse2SlotG, *Sse3SlotG, *SseMatchSlotG, *BinSseCellG;
  };
  SlotPtrs sp;
  STATE** CtxChainEnd;
  struct HintScratch { int hintSymB31, hintSymM2, hintSymB29, hintSymB33, hintSymMatch3, hintSymBmCell; };
  HintScratch hint;
  struct DeltaScratch { int sse2DenDelta, sse2NumDelta, sseMatchDenDelta, sseMatchNumDelta, predSseTotDelta, predWeightDelta; };
  DeltaScratch dlt;
  int MatchCtxHi, recentSym, sseScaleCntr, symHalfHistory;
  int SparseHashA, SparseIdxA, SparseHashB, SparseIdxB, SparseBit;
  byte SseSubBlocks[0x810];
  byte* Sse2BaseG;
  byte MatchPosHash[0x40000];
  byte SseState2[0x80000];
  byte SseState3[0x20000];
  int PopCountWeighted_(uint v, const char* weights) {
    int sum = 0;
    for (int j = 0; v > 0; v >>= 1, ++j) {
      sum += weights[j] * (int)(v & 1);
    }
    return sum;
  }

  void InitSseCell_(SseCounter& cell, int w) {
    cell.freq0 = 18432;
    cell.freq1 = 5120;
    cell.sum = (w << 23) | (72 * w);
  }

  int ClampSseWeight_(int w) {
    if (w >= 241) return 241;
    if (w < 9) return 9;
    return w;
  }

  byte& Order1LastSymCell_(int sym, int prev1Sym) {
    return SseState2[0x70000 + sym + (prev1Sym << 8)];
  }

  byte& Order1HintSymCell_(int sym, int prev1Sym) {
    return SseState2[0x40000 + sym + (prev1Sym << 8)];
  }

#pragma pack(1)
  struct STATE {
    byte Symbol, Freq;
    uint iSuccessor;
    bool hasSuccessor(Model* M) const {
      return (byte*)M->Indx2Ptr(iSuccessor) >= M->UnitsStart;
    }
    PPM_CONTEXT* getSuccessor(Model* M) const {
      return (PPM_CONTEXT*)M->Indx2Ptr(iSuccessor);
    }
  };
  struct PPM_CONTEXT {
    enum { MAX_FREQ = 244, BIN_MAX_FREQ = 127, O_BOUND = 9 };
    byte NStates, Flags;
    word SummFreq;
    uint iStates;
    uint iSuffix;
    STATE &oneState() const {
      return (STATE &)SummFreq;
    }

    STATE*getStates(Model* M) const {
      return (STATE*)M->Indx2Ptr(iStates);
    }

    PPM_CONTEXT*getSuffix(Model* M) const {
      return (PPM_CONTEXT*)M->Indx2Ptr(iSuffix);
    }

    STATE& mruState(Model* M) const {
      return getStates(M)[Flags & 0xF];
    }
  };
#pragma pack()
  PPM_CONTEXT* AllocContext_() {
    if (HiUnit != LoUnit) {
      HiUnit -= UNIT_SIZE;
      return (PPM_CONTEXT*)HiUnit;
    }
    if (BList->avail()) {
      return (PPM_CONTEXT*)BList->unlinkPrev(this);
    }
    return (PPM_CONTEXT*)AllocUnitsRare(0);
  }

  void FreeContext_(PPM_CONTEXT* ptr) {
    MEM_BLK* p = (MEM_BLK*)ptr;
    if (!p[1].canMerge())
      BList[0].linkPrev(this, p, 1);
    else
      FreeUnitsRare(p, 1);
  }

  PPM_CONTEXT* MoveContext_(PPM_CONTEXT* OldPtr) {
    MEM_BLK* p = (MEM_BLK*)OldPtr;
    if (!p[1].canMerge() || !BList[0].avail()) return OldPtr;
    PPM_CONTEXT* NewPtr = (PPM_CONTEXT*)BList[0].unlinkPrev(this);
    UnitsCpy_(NewPtr, OldPtr, 1);
    FreeUnitsRare(p, 1);
    return NewPtr;
  }

  struct SseIdx {
    uint val = 0;
    template <int Pos>
    constexpr SseIdx& bit(bool b) { val += (uint)b << Pos; return *this; }

    template <int Pos, int W>
    constexpr SseIdx& bits(uint v) { val += (v & ((1u<<W)-1)) << Pos; return *this; }

    template <int Pos, int W>
    constexpr SseIdx& field(uint v) {
      constexpr uint M = ((1u<<W) - 1) << Pos;
      val += v & M;
      return *this;
    }

    constexpr SseIdx& raw(uint v) { val += v; return *this; }

    constexpr operator uint() const { return val; }
  };
  static constexpr int PATH_BUF_MAX = 128;

  STATE* FindStateBySymbol_(STATE* states, byte sym) {
    while (states->Symbol != sym) ++states;
    return states;
  }

  int SparseBitFlags_() {
    return (int)SseIdx{}
      .bit<0>(SparseBit & SparseBitmapA[SparseIdxA])
      .bit<1>(SparseBit & SparseBitmapB[SparseIdxB]);
  }

  void SeedWalkSparseHash_(int sym) {
    SparseBit = 1 << sym;
    SparseIdxA = (uint)(sym + SparseHashA) >> 5;
    SparseIdxB = (uint)(sym + SparseHashB) >> 5;
  }

  void SeedCandidateSparseHash_(uint candSymbol, int predShiftFlags) {
    SparseBit = 1 << candSymbol;
    SparseIdxA = ((candSymbol + SparseHashA) >> 5) + 0x2000;
    SparseIdxB = predShiftFlags + ((candSymbol + SparseHashB) >> 5);
  }

  void ClearSparseBitmapHit_() {
    JREG(&SparseBitmapA[SparseIdxA], sizeof(int));
    JREG(&SparseBitmapB[SparseIdxB], sizeof(int));
    SparseBitmapA[SparseIdxA] &= ~SparseBit;
    SparseBitmapB[SparseIdxB] &= ~SparseBit;
  }

  void MarkSparseBitmapHit_() {
    JREG(&SparseBitmapA[SparseIdxA], sizeof(int));
    JREG(&SparseBitmapB[SparseIdxB], sizeof(int));
    SparseBitmapA[SparseIdxA] |= SparseBit;
    SparseBitmapB[SparseIdxB] |= SparseBit;
  }

  void InitFreqMap_() {
    static const byte freqtmp[] = {0,0,0,1,1,2,3,3,4,0,0,0};
    memset(freqmap, 0, sizeof(freqmap));
    memcpy(freqmap, freqtmp, sizeof(freqtmp));
  }

  void InitModelScalars_() {
    runLengthInit = (MaxOrder >= 11) ? -11 : -MaxOrder;
    RunLength = runLengthInit;
    SseScale = 1024;
    FoundSymbol = -1;
    HashSeed1 = -1;
    HashSeed2 = -1;
    SymEpoch = 1;
  }

  void ZeroAllScratchGlobals_() {
    memset(SymFreqs, 0, 256);
    sseTot = sseCum = orderBumpVariance = 0;
    predWeightSink = predBaseDeltaA = predBaseDeltaB = 0;
    binSseDeltaHi = binSseDeltaLo = 0;
    predRescaleDiv = cumFreqAcc = cumFreqSseSave = sseIdxStorage = 0;
    qs = QSlotScratch{};
    FoundState = nullptr;
    CtxChainEnd = nullptr;
    sp = SlotPtrs{};
    hint = HintScratch{};
    dlt = DeltaScratch{};
    MatchCtxHi = recentSym = sseScaleCntr = symHalfHistory = 0;
    SparseHashA = SparseIdxA = SparseHashB = SparseIdxB = SparseBit = 0;
    memset(SseState3, 0, 0x20000);
    memset(&SseState2[0x10000], 0, 0x10000);
  }

  void InitSSE0SSE1Tables_() {
    memset(SSE0, 0, 64);
    memset(SSE0 + 64, 0x80, 192);
    SSE1[0] = 0; SSE1[1] = 2; SSE1[2] = 2;
    memset(SSE1 + 3, 4, 35);
    memset(SSE1 + 38, 6, 218);
  }

  void InitBinMapTable_() {
    memset(BinMapTable, 0, sizeof(BinMapTable));
    BinMapTable[3] = 1u << 9;
    BinMapTable[7] = 2u << 9;
    BinMapTable[11] = 1u << 9;
    BinMapTable[15] = 3u << 9;
  }

  template<typename T, typename B>
  void BuildQuantTable_(T* target, const B* bounds, uint nVals, int baseOffset) {
    uint j = 0;
    for (uint i = 0; i < nVals; ++i) {
      target[i] = (T)(j + baseOffset);
      if (i == (uint)bounds[j]) ++j;
    }
  }

  void BuildSymTypeQuantTable_() {
    int quantStepSize = 1, stepRemaining = 1;
    SymType[0] = 1; SymType[1] = 2;
    for (uint i = 2, j = 2, k = 2; i < 0x100; ++i) {
      SymType[j] = k + 1;
      if (--stepRemaining == 0) { ++k; stepRemaining = ++quantStepSize; }
      j = i + 1;
    }
    SymType[0xFF] = 24;
  }

  void InitBijectMapSelfTrigrams_() {
    for (int i = 0; i < 0x10000; ++i) {
      BijectMap[4*i + 0] = i;
      BijectMap[4*i + 1] = i;
      BijectMap[4*i + 2] = i;
    }
  }

  void InitPrimarySseTable_() {
    memset(SseWeight1, 0, 0x4000 * sizeof(SseCounter));
    for (int outerIdx = 0; outerIdx < 0x4000; ++outerIdx) {
      int bitSum = PopCountWeighted_((uint)outerIdx, SseWeight1PopBits);
      for (int sseDimIdx = 0; sseDimIdx < 14; ++sseDimIdx) {
        int idx = (sseDimIdx + 1) * 0x4000 + outerIdx;
        InitSseCell_(SseWeight1[idx], ClampSseWeight_(bitSum + (byte)SseWeight1BiasPerDim[sseDimIdx]));
      }
      SseWeight1[outerIdx].freq1 = 1024;
    }
  }

  void InitSecondarySseTable_() {
    SseCounter* sse2base = &SseWeight1[SSE2_BASE];
    memset(sse2base, 0, 0x38020 * sizeof(SseCounter));
    for (int outerIdx2 = 0; outerIdx2 < 0x2000; ++outerIdx2) {
      int bitSum = PopCountWeighted_((uint)outerIdx2, SseWeight2PopBits);
      for (int sseDim2Idx = 0; sseDim2Idx < 24; ++sseDim2Idx) {
        int idx = 0x4000 + sseDim2Idx * 0x2000 + outerIdx2;
        InitSseCell_(sse2base[idx], ClampSseWeight_(bitSum + (byte)SseWeight2BiasPerDim[sseDim2Idx]));
      }
      sse2base[0x36000 + outerIdx2].freq1 = 1024;
      sse2base[0x34000 + outerIdx2].freq1 = 1024;
      sse2base[0x02000 + outerIdx2].freq1 = 1024;
      sse2base[0x00000 + outerIdx2].freq1 = 1024;
    }
  }

  void InitFlatSseTables_() {
    auto initFlatSseCell = [](SseCounter& c) {
      c.sum = 20480;
      c.freq0 = 2048;
      c.freq1 = 2048;
    };
    for (int ctxBucket = 0; ctxBucket < 16; ++ctxBucket) {
      for (int sseCellIdx = 0; sseCellIdx < 1024; ++sseCellIdx)
        initFlatSseCell(SseWeight2[ctxBucket * 0x400 + sseCellIdx]);
      for (int sseCellIdx2 = 0; sseCellIdx2 < 256; ++sseCellIdx2)
        initFlatSseCell(EscMirrorSse[ctxBucket * 256 + sseCellIdx2]);
    }
  }

  void InitBinSseFromB22_() {
    for (int contextSize = 0; contextSize < 5; ++contextSize) {
      int initialSseValue = 49 * (byte)BinSseInitPerBucket[contextSize];
      for (int sseInitIdx = 0; sseInitIdx < 128; ++sseInitIdx) {
        BinSse[contextSize * 128 + sseInitIdx] = initialSseValue;
      }
    }
  }

  void InitPredWeightFromB23_() {
    for (int contextSize2 = 0; contextSize2 < 5; ++contextSize2) {
      int baseWeight = 48 * (byte)PredWeightInitPerBucket[contextSize2];
      for (int predIdx = 0; predIdx < 256; ++predIdx) {
        int* cell = &PredWeight[contextSize2 * 512 + predIdx * 2];
        cell[0] = baseWeight;
        cell[1] = 15104;
      }
    }
  }

  void InitHeapAllocator_() {
    byte* heapBlocks = (byte*)HeapStart;
    BList = (MEM_BLK*)HeapStart;
    ++InitsCount;
    pText = (byte*)HeapStart + N_INDEXES * UNIT_SIZE;
    CutOffCount = 0;
    GlueCount = 0;
    // Must be computed in 64-bit: at ~2276 MB the product
    // UNIT_SIZE*(MEM_DIVISOR-1)*(SubAllocatorSize/(UNIT_SIZE*MEM_DIVISOR))
    // exceeds INT_MAX, so an int cast/multiply here wraps negative and sends
    // unitsSegment far past the heap end. The (qword) cast keeps the whole
    // chain 64-bit (matches PPMIIDeleteModel's USSize).
    // 38661 max
    qword winsize = SubAllocatorSize - qword(UNIT_SIZE) * (MEM_DIVISOR-1) * (SubAllocatorSize / (UNIT_SIZE * MEM_DIVISOR));
    qword maxwinsize = ((1000000000ULL+UNIT_SIZE-1)/UNIT_SIZE)*UNIT_SIZE; if( winsize>maxwinsize ) winsize=maxwinsize;
    winsize += (SubAllocatorSize - UNIT_SIZE - winsize) % UNIT_SIZE; // snap HiUnit onto the UnitsStart 12B grid
    byte* unitsSegment = (byte*)HeapStart + winsize;
    UnitsStart = unitsSegment;
    LoUnit = unitsSegment;
    HeapNull = heapBlocks - 1;
    // Capture the fixed packing boundary now, before any Ptr2Indx/Indx2Ptr use.
    // Deterministic from SubAllocatorSize, so it is identical across restarts.
    UnitsStartLim = (qword)(UnitsStart - HeapNull);
    *(uint*)unitsSegment = 0;
    for (uint i = 0; i < N_INDEXES; ++i) {
      BList[i].QueueSize = 0;
      BList[i].Next = BList[i].Prev = UNIT_SIZE * i + 1;
    }
  }

  void ResetSseAndMatchTables_() {
    memset(SseSubBlocks, 0, sizeof(SseSubBlocks));
    Sse2BaseG = SseSubBlocks;
    memset(MatchPosHash, 0, sizeof(MatchPosHash));
    memset(SseState2, 0x55, sizeof(SseState2));
    memset(SseState3, 0x55, sizeof(SseState3));
    memset(LastEpochBySym, 0x55, sizeof(LastEpochBySym));
    memset(MatchPosTable, 0x55, sizeof(MatchPosTable));
  }

  void InitSseCellArrays_() {
    auto initSseCells = [](int* cells, int nCells, int num0, int denInit) {
      for (int i = 0; i < nCells; ++i) {
        cells[2*i + 0] = num0;
        cells[2*i + 1] = denInit;
      }
    };
    OrderCtxSeed = 0;
    initSseCells(Sse1, 196608, 0x2000, 24576);
    initSseCells(SseMatch, 0x100000, 0, 0x40000);
    SseSeed = 0;
    initSseCells(Sse2, 98304, 0, 0x40000);
    SseCtxExtra = 0;
    initSseCells(Sse3, 86016, 0, 0x80000);
  }

  qword InitRootContextAndStates_() {
    PPM_CONTEXT* allocatedContextAddr;
    byte* heapEnd = (byte*)HeapStart + SubAllocatorSize;
    byte* unitsSegment = UnitsStart;
    if (heapEnd == unitsSegment) {
      if (!BList[0].avail())
        allocatedContextAddr = (PPM_CONTEXT*)AllocUnitsRare(0);
      else
        allocatedContextAddr = (PPM_CONTEXT*)BList[0].unlinkPrev(this);
    } else {
      heapEnd = (byte*)HeapStart + SubAllocatorSize - UNIT_SIZE;
      HiUnit = heapEnd;
      allocatedContextAddr = (PPM_CONTEXT*)heapEnd;
    }
    PPM_CONTEXT* rootCtxP = allocatedContextAddr;
    rootCtxP->iSuffix = 0;
    RootContext = allocatedContextAddr;
    rootCtxP->Flags = 0xC7;
    MaxContext = allocatedContextAddr;
    rootCtxP->SummFreq = 256;
    NMasked = 255;
    rootCtxP->NStates = 255;
    byte* stateStorageAddr = AllocUnits_(Units2Indx[127]);
    FoundState = (STATE*)stateStorageAddr;
    rootCtxP->iStates = Ptr2Indx(stateStorageAddr);
    SseCtx = 0;
    SseCtx2 = 0;
    SseCtx3 = 0;
    EscapeSymbol = 0;
    PrevSymbol = 0;
    STATE* states = rootCtxP->getStates(this);
    for (int i = 0; i < 256; ++i) {
      states[i].Symbol = (byte)i;
      states[i].Freq = 1;
      states[i].iSuccessor = 0;
    }
    return 1536;
  }

  qword AppendTailStateToBlock_(PPM_CONTEXT* curCtxP, qword newStateEnd, byte foundSym, uint succIdxSaved, char sse0BitSaved) {
    STATE* tailState = (STATE*)newStateEnd;
    tailState->Symbol = foundSym;
    tailState->Freq = 0;
    tailState->iSuccessor = succIdxSaved;
    char upperFlagBits = curCtxP->Flags & 0xF0;
    qword statesBaseAddr = (qword)Indx2Ptr(curCtxP->iStates);
    ++curCtxP->NStates;
    qword stateByteOff = newStateEnd - statesBaseAddr;
    curCtxP->Flags = sse0BitSaved | (stateByteOff / 6) | upperFlagBits;
    return stateByteOff / 6;
  }

  qword ShiftStatesBackForTail_(PPM_CONTEXT* curCtxP, qword newStateEnd) {
    qword statesBase = (qword)Indx2Ptr(curCtxP->iStates);
    if (newStateEnd <= statesBase + 42) return newStateEnd;
    STATE* dst = (STATE*)newStateEnd;
    do {
      *dst = dst[-1];
      --dst;
    } while ((qword)dst > statesBase + 42);
    return (qword)dst;
  }

  PPM_CONTEXT* FindSeedContextForSuccessors_(STATE**& chainPtr,uint seedSuccIdx, PPM_CONTEXT* seedCtx) {
    bool skipSuffixLoop = false;
    while (1) {
      int curSuccIdx = (*chainPtr)->iSuccessor;
      if (curSuccIdx != seedSuccIdx) {
        seedCtx = (PPM_CONTEXT*)Indx2Ptr(curSuccIdx);
        skipSuffixLoop = true;
        break;
      }
      uint ctxSuffixIdx = seedCtx->iSuffix;
      ++chainPtr;
      if (!ctxSuffixIdx) { skipSuffixLoop = true; break; }
      if (chainPtr >= CtxChainEnd) break;
      seedCtx = (PPM_CONTEXT*)Indx2Ptr(ctxSuffixIdx);
    }
    if (skipSuffixLoop) return seedCtx;
    STATE* localFoundState = FoundState;
    uint suffixIdx0 = seedCtx->iSuffix;
    while (1) {
      PPM_CONTEXT* ctxAddr = (PPM_CONTEXT*)Indx2Ptr(suffixIdx0);
      PPM_CONTEXT* pc = ctxAddr;
      STATE* state = pc->NStates
                       ? FindStateBySymbol_(pc->getStates(this), localFoundState->Symbol)
                       : &pc->oneState();
      int stateSuccIdx = state->iSuccessor;
      if (stateSuccIdx != seedSuccIdx) { return (PPM_CONTEXT*)Indx2Ptr(stateSuccIdx); }
      suffixIdx0 = pc->iSuffix;
      *chainPtr = state;
      ++chainPtr;
      if (!suffixIdx0) return ctxAddr;
    }
  }

  qword AllocateBinarySuccessorChain_(STATE** chainPtrEnd, STATE** chainEnd,PPM_CONTEXT* seedCtx, byte newSym,byte newFlags, int newCtxBytePos) {
    PPM_CONTEXT* prevCtx = seedCtx;
    while (true) {
      PPM_CONTEXT* newCtx = AllocContext_();
      if (!newCtx) return 0;
      newCtx->NStates = 0;
      newCtx->Flags = newFlags;
      STATE& one = newCtx->oneState();
      one.Symbol = newSym;
      one.Freq = 0;
      one.iSuccessor = newCtxBytePos;
      newCtx->iSuffix = Ptr2Indx(prevCtx);
      prevCtx = newCtx;
      qword result = Ptr2Indx(newCtx);
      --chainPtrEnd;
      (*chainPtrEnd)->iSuccessor = result;
      if (chainPtrEnd == chainEnd) return result;
    }
  }

  byte ComputeRescaledBinEscFreq_(uint suffixFreq, uint summFreq, int nStates, int numStates, uint suffixNStates, uint suffixSummFreq) {
    uint totalFreqBound = summFreq - suffixFreq + 2*suffixNStates + 2 + suffixSummFreq;
    uint freqMultiplier = (suffixFreq >= 6) ? suffixFreq - 1
                        : (suffixFreq >= 4) ? 3u : 1u;
    int stateOffsetBase = 4*nStates + 4;
    int clampedDiff = (int)suffixNStates + 1 - 4*nStates;
    if (clampedDiff < 0) clampedDiff = 0;
    else if (clampedDiff > 16*numStates) clampedDiff = 16*numStates;
    uint rescaleIndex = 3 * (clampedDiff + summFreq + stateOffsetBase) * freqMultiplier / totalFreqBound;
    byte newFreq = (rescaleIndex >= 36) ? 11 : FreqRescaleTab[rescaleIndex];
    if (rescaleIndex <= 33 || summFreq >= 32u * numStates) return newFreq;
    int frequencyBonus = (summFreq >= 6u*nStates + 6) ? 7 : 23;
    int scaleDelta = (int)rescaleIndex / 6 - 4;
    if (scaleDelta < frequencyBonus) frequencyBonus = scaleDelta;
    return (byte)(newFreq + frequencyBonus);
  }

  int BubbleStateUpInBlock_(STATE* baseStates, int currIdx) {
    while (currIdx > 0) {
      STATE* curr = &baseStates[currIdx];
      STATE* prev = &baseStates[currIdx - 1];
      if (curr->Freq <= prev->Freq + 1) break;
      STATE temp = *curr;
      *curr = *prev;
      *prev = temp;
      --currIdx;
    }
    return currIdx;
  }

  void RecurseOnUpdateSurvivors_(STATE* p0, int i, uint Order) {
    STATE* sLast = p0 + i;
    if (Order < (uint)MaxOrder) {
      do {
        PPM_CONTEXT* succPtr = sLast->getSuccessor(this);
        sLast->iSuccessor = (uint)UpdateModel(succPtr, Order + 1);
        sLast--;
      } while (--i >= 0);
    } else {
      do {
        sLast->iSuccessor = 0;
        sLast--;
      } while (--i >= 0);
    }
  }

  int PackStatesWithSuccessor_(STATE* states, int NStates0) {
    int i = NStates0, k = NStates0;
    STATE* p = states;
    STATE* p1 = states;
    do {
      if (!p->hasSuccessor(this)) {
        p->iSuccessor = 0;
        --i;
      } else {
        if (p != p1) {
          STATE tmp = *p1;
          *p1 = *p;
          *p = tmp;
        }
        p1++;
      }
      p++;
      --k;
    } while (k >= 0);
    return i;
  }

  STATE* CollapseToOneStateSurvivor_(PPM_CONTEXT* ctx, STATE* states, int NU) {
    ctx->Flags = SSE0[states[0].Symbol];
    int f = states[0].Freq;
    int s = (int)ctx->SummFreq - (f - 1) + 19;
    int newFreq;
    if (s < 2*f - 2)
      newFreq = (byte)((f - 1) / s + 2);
    else
      newFreq = (s < 3*f - 3);
    states[0].Freq = (byte)(newFreq + 1);
    STATE* p0 = &ctx->oneState();
    *p0 = states[0];
    FreeUnits_(states, NU);
    return p0;
  }

  STATE* ShrinkAndRescaleStates_(PPM_CONTEXT* ctx, STATE* states, int oldNU, int i, byte oldFlags) {
    int newNU = (i + 2) >> 1;
    states = (STATE*)ShrinkUnits_(states, oldNU, newNU);
    ctx->iStates = Ptr2Indx(states);
    int Scale = (ctx->SummFreq > 16 * (uint)i);
    ctx->Flags = (byte)((((byte)Scale << 6) & oldFlags) + SSE0[states[0].Symbol]);
    states[0].Freq = (byte)(((uint)states[0].Freq + Scale) >> Scale);
    ctx->SummFreq = (byte)states[0].Freq;
    STATE* p = states;
    for (int j = i; j != 0; --j) {
      p++;
      ctx->Flags |= SSE0[p->Symbol];
      p->Freq = (byte)(((uint)p->Freq + Scale) >> Scale);
      ctx->SummFreq += (byte)p->Freq;
    }
    return states;
  }

  qword GrowStateBlockIfNeeded_(PPM_CONTEXT* curCtxP, uint nStatesP1) {
    if ((nStatesP1 & 1) != 0) {
      return curCtxP->iStates;
    }
    byte* statesPtr = (byte*)Indx2Ptr(curCtxP->iStates);
    uint halfNStatesP1 = nStatesP1 >> 1;
    uint sizeClassP = halfNStatesP1 ? Units2Indx[halfNStatesP1 - 1] : 0;
    if (sizeClassP != Units2Indx[halfNStatesP1]) {
      uint sizeClass4P = Units2Indx[halfNStatesP1];
      byte* newStatesPtr = AllocUnits_((uint)sizeClass4P);
      sizeClassP = halfNStatesP1 ? Units2Indx[halfNStatesP1 - 1] : 0;
      if (newStatesPtr) {
        UnitsCpy_(newStatesPtr, statesPtr, nStatesP1 >> 1);
        FreeUnitsRare(statesPtr, (uint)Indx2Units[sizeClassP]);
      }
      statesPtr = newStatesPtr;
    }
    if (!statesPtr) return 0;
    qword newStatesIdx2 = Ptr2Indx(statesPtr);
    curCtxP->iStates = newStatesIdx2;
    return newStatesIdx2;
  }

  qword PromoteSingleStateContext_(PPM_CONTEXT* curCtxP, uint escIdxClipped) {
    byte* allocedUnit = AllocUnits_(Units2Indx[0]);
    if (!allocedUnit) return 0;
    STATE* newStates = (STATE*)allocedUnit;
    *newStates = curCtxP->oneState();
    int freqBoost = EscIdxFreqBoost[escIdxClipped];
    curCtxP->iStates = Ptr2Indx(allocedUnit);
    int newStateFreq = 4 * (int)newStates->Freq + freqBoost;
    if (newStateFreq >= 238) newStateFreq = 238;
    if (newStateFreq < 2) newStateFreq = 2;
    newStates->Freq = (byte)newStateFreq;
    curCtxP->SummFreq = (byte)newStateFreq;
    return (qword)(allocedUnit + 6);
  }

  void CollapseTwoStateChainToOneState_(PPM_CONTEXT* startCtx, PPM_CONTEXT* endCtx, byte parentSym) {
    PPM_CONTEXT* walker = startCtx;
    do {
      STATE* states = walker->getStates(this);
      STATE* kept = &states[states[0].Symbol != parentSym];
      walker->Flags = SSE0[kept->Symbol];
      kept->Freq = (byte)(((uint)kept->Freq + 3) >> 2);
      walker->oneState() = *kept;
      walker->NStates = 0;
      FreeUnitsRare(states, 1);
      walker = walker->getSuffix(this);
    } while (walker != endCtx);
  }

  uint BuildSseCtxComposite_(int sym, int epoch, uint orderShortfall, byte ctxFoundSym, int sparseFlags) {
    return (uint)(SseIdx{}
      .raw (SseCtx)
      .bit <5> (RunLength > 0)
      .raw (MaxContext->Flags & 0x80)
      .bit <6> (SseCtx2 & 1)
      .bit <8> ((SseCtx2 & 6) == 6)
      .bit <9> (sym == FoundSymbol)
      .bit <10>(epoch == SymLastCtx[sym])
      .bit <11>(orderShortfall > 3)
      .bit <12>(sym == ctxFoundSym)
      .bit <13>(sparseFlags == 3 || epoch == MatchPosBySym[sym]));
  }

  int SelectSseCtxFromContext_(uint sseCtxComposite, PPM_CONTEXT* max_suffix_ctx, uint& suffixNStates, uint path_threshold, uint initial_threshold, uint scale_diff, uint freq_bound, uint orderShortfall, uint verification_nstates, PPM_CONTEXT* ctx, STATE* found_state, int path_depth, int sym) {
    if (suffixNStates) {
      uint sseCtxMasked = sseCtxComposite & 0xFFFFFFDE;
      uint boostBit = (path_threshold != initial_threshold) || (scale_diff <= 10*freq_bound);
      return 32*boostBit + sseCtxMasked + (sym == PrevSymbol);
    }
    PPM_CONTEXT* deep_suffix_ctx = max_suffix_ctx->getSuffix(this);
    suffixNStates = deep_suffix_ctx->NStates;
    if (deep_suffix_ctx->NStates || orderShortfall < 4 || (orderShortfall < 5 && (verification_nstates+1) < 4 && ctx->SummFreq - found_state->Freq < 36)) {
      uint deeperBit = deep_suffix_ctx->iSuffix ? (uint)(2*suffixNStates < deep_suffix_ctx->getSuffix(this)->NStates) : 0;
      return sseCtxComposite + 8*deeperBit + 16;
    }
    int depthBoost = 2*(4 + (path_depth>=6) + (path_depth>=14) + (path_depth>=32) );
    return sseCtxComposite + depthBoost;
  }

  struct VerifyOut { PPM_CONTEXT* ctx; uint total_depth; };
  VerifyOut WalkSuffixVerificationChain_(PPM_CONTEXT* ctx, int sym, int path_depth) {
    STATE* last_state = ctx->getStates(this) + ctx->NStates;
    if (last_state->Freq != 0) return { ctx, (uint)path_depth };
    uint temp_depth = path_depth;
    do {
      if (last_state->Symbol != sym) break;
      ctx = ctx->getSuffix(this);
      ++temp_depth;
      last_state = ctx->getStates(this) + ctx->NStates;
    } while (last_state->Freq == 0);
    return { ctx, temp_depth };
  }

  struct PathThresholdOut { uint threshold, freq_bound; };
  PathThresholdOut ComputePathThreshold_(uint freqStateFreq, uint summ_freq) {
    uint freq_bound = (freqStateFreq <= 9) ? freqmap[freqStateFreq - 1] : freqStateFreq - 6;
    uint scale_diff = summ_freq - freq_bound;
    uint threshold = 1
      + (35*scale_diff < 4*freq_bound)
      + (15*scale_diff < 4*freq_bound)
      + (13*scale_diff < 8*freq_bound)
      + ( 9*scale_diff < 4*freq_bound)
      + ( 6*scale_diff < freq_bound)
      + ( scale_diff < freq_bound)
      + ( scale_diff < 2*freq_bound)
      + ( scale_diff < 6*freq_bound);
    return { threshold, freq_bound };
  }

  PPM_CONTEXT* WalkPathThreshold_(PPM_CONTEXT** path, int path_depth, uint& path_threshold) {
    PPM_CONTEXT* path_ctx = nullptr;
    for (int i = path_depth - 1; i >= 0; --i) {
      path_ctx = path[i];
      byte& high_byte = ((byte*)&path_ctx->SummFreq)[1];
      if (path_threshold > high_byte) high_byte = path_threshold;
      else path_threshold = high_byte;
    }
    return path_ctx;
  }

  void RunInertiaTuning_(PPM_CONTEXT* ctx, int sym, STATE* found_state, STATE* states, uint verification_nstates, word& summ_freq) {
    if (2*found_state->Freq + 23 <= states[0].Freq + states[1].Freq) return;
    if (!ctx->iSuffix) return;
    PPM_CONTEXT* suffix_ctx = ctx->getSuffix(this);
    word suffix_summ_freq = suffix_ctx->SummFreq;
    byte suffix_nstates = suffix_ctx->NStates;
    if (summ_freq + summ_freq*suffix_nstates >= (verification_nstates+1)*(suffix_summ_freq+15))
      return;
    if (suffix_ctx->mruState(this).Freq == 0) {
      BinEscFreq(suffix_ctx);
      suffix_summ_freq = suffix_ctx->SummFreq;
      summ_freq = ctx->SummFreq;
    }
    STATE* suffix_state = FindStateBySymbol_(suffix_ctx->getStates(this), (byte)sym);
    int suffix_found_freq = suffix_state->Freq;
    int base_weight = 5*verification_nstates + 5;
    uint tunedSumm = summ_freq + 2*base_weight;
    int suffixScale = (2*base_weight < summ_freq) ? 7 : 5;
    int tunedSuffix = suffixScale*(suffix_nstates+1) + suffix_summ_freq;
    if( tunedSuffix*found_state->Freq >= (int)(tunedSumm*suffix_found_freq) || found_state->Freq > 228 ) return;
    word delta_freq = summ_freq - found_state->Freq;
    uint deltaSum = tunedSumm - found_state->Freq;
    uint denomAdj = 3*deltaSum + tunedSuffix - suffix_found_freq;
    uint adjusted_freq = (deltaSum*(suffix_found_freq + 3*found_state->Freq) + denomAdj - 4) / denomAdj;
    if (adjusted_freq > found_state->Freq + 11) adjusted_freq = found_state->Freq + 11;
    summ_freq = adjusted_freq + delta_freq;
    ctx->SummFreq = summ_freq;
    found_state->Freq = adjusted_freq;
  }

  struct PPMContextWalkOut {
    uint seeIndex;
    uint suffixNStates;
    int sseCtx;
    uint iStates;
    int sparseFlags;
  };
  PPMContextWalkOut PPMContextWalk(int epoch, int sym) {
    PPM_CONTEXT* path[PATH_BUF_MAX];
    int path_depth = 0;
    PPM_CONTEXT* curr = MaxContext;
    RSContext = sym;
    SymLastCtx[Order1LastSymCell_(sym, Order1Ctx)] = epoch;
    do {
      path[path_depth++] = curr;
      curr = curr->getSuffix(this);
    } while (!curr->NStates);
    auto [ctx, total_depth] = WalkSuffixVerificationChain_(curr, sym, path_depth);
    uint verification_nstates = ctx->NStates;
    if( ctx->mruState(this).Freq==0 ) BinEscFreq(ctx);
    EscapeSymbol = ctx->mruState(this).Symbol;
    STATE* states = ctx->getStates(this);
    STATE* found_state = FindStateBySymbol_(states, (byte)sym);
    word summ_freq = ctx->SummFreq;
    RunInertiaTuning_(ctx, sym, found_state, states, verification_nstates, summ_freq);
    auto [path_threshold, freq_bound] = ComputePathThreshold_(found_state->Freq, summ_freq);
    uint scale_diff = summ_freq - freq_bound;
    uint initial_threshold = path_threshold;
    PPM_CONTEXT* path_ctx = WalkPathThreshold_(path, path_depth, path_threshold);
    orderBumpVariance = path_threshold-initial_threshold;
    SeedWalkSparseHash_(sym);
    int sparseFlags = SparseBitFlags_();
    int matchTableEntry = MatchPosTable[sym+(MatchCtxHi<<8)];
    ComputeMatchHints_(matchTableEntry, sym);
    PPM_CONTEXT* max_suffix_ctx = MaxContext->getSuffix(this);
    uint suffixNStates = max_suffix_ctx->NStates;
    SseCtxExtra += SseIdx{}.bit<7>(suffixNStates == 0);
    uint orderShortfall = OrderFall-total_depth;
    byte ctxFoundSym = ctx->mruState(this).Symbol;
    uint sseCtxComposite = BuildSseCtxComposite_(sym, epoch, orderShortfall,ctxFoundSym, sparseFlags);
    int sseCtx = SelectSseCtxFromContext_(sseCtxComposite, max_suffix_ctx, suffixNStates, path_threshold,initial_threshold, scale_diff,freq_bound, orderShortfall,verification_nstates, ctx, found_state, path_depth, sym);
    return PPMContextWalkOut{ path_threshold, suffixNStates, sseCtx, path_ctx->iStates, sparseFlags };
  }

  void InitTables() {
    ZeroAllScratchGlobals_();
    InitFreqMap_();
    InitIndx2UnitsTables_();
    BuildSymTypeQuantTable_();
    InitSSE0SSE1Tables_();
    InitBinMapTable_();
    BuildQuantTable_(NextBinFreq, RLQBounds, 0x80, 1);
    BuildQuantTable_(SSE0QTable, SSE0QBounds, 0x100, 1);
    BuildQuantTable_(SSE1QTable, SSE1QBounds, 0x80, 0);
    BuildQuantTable_(SEEQTable, SEEQBounds, 0x100, 0);
  }

  void BinEscFreq(PPM_CONTEXT* pc) {
    int nStates = pc->NStates;
    int numStates = nStates + 1;
    STATE* baseStates = pc->getStates(this);
    int currIdx = pc->Flags & 0x0F;
    STATE* currState = &baseStates[currIdx];
    PPM_CONTEXT* suffix = pc->getSuffix(this);
    while (true) {
      STATE* suffixMaxState = suffix->getStates(this) + suffix->NStates;
      if (suffixMaxState->Freq != 0 || suffixMaxState->Symbol != currState->Symbol) break;
      suffix = suffix->getSuffix(this);
    }
    if( suffix->mruState(this).Freq==0 ) BinEscFreq(suffix);
    STATE* matchingSuffixState = FindStateBySymbol_(suffix->getStates(this),currState->Symbol);
    uint summFreq = pc->SummFreq;
    currState->Freq = ComputeRescaledBinEscFreq_(matchingSuffixState->Freq,summFreq, nStates, numStates, suffix->NStates, suffix->SummFreq);
    pc->SummFreq = currState->Freq + summFreq;
    currIdx = BubbleStateUpInBlock_(baseStates, currIdx);
    pc->Flags = currIdx | (pc->Flags & 0xF0);
  }

  void SwapFoundStateToFront_(STATE* states) {
    STATE* fp = FoundState;
    while (fp != states) {
      STATE tmp = *fp;
      *fp = *(fp - 1);
      *(fp - 1) = tmp;
      --fp;
    }
  }

  STATE* RescaleAndCompactStates_(PPM_CONTEXT* ctx, STATE* states, int totalCount, int bias, int mask, int shift) {
    STATE* endState = states + totalCount;
    STATE* lastState = endState - 1;
    ctx->Flags = 0;
    ctx->SummFreq = 0;
    int remaining = totalCount;
    do {
      --endState;
      int newFreq = (bias + mask * endState->Freq) >> shift;
      endState->Freq = (byte)newFreq;
      ctx->SummFreq += (byte)newFreq;
      if (endState->Freq) {
        ctx->Flags |= SSE0[endState->Symbol];
      } else {
        for (STATE* p = endState; p < lastState; ++p) {
          *p = p[1];
        }
        lastState->Freq = 0;
      }
      --remaining;
    } while (remaining);
    return lastState;
  }

  void CollapseRescaledToOneState_(PPM_CONTEXT* ctx, STATE* states, int NStates0) {
    word firstSF = states[0].Symbol | (states[0].Freq << 8);
    uint firstSucc = states[0].iSuccessor;
    uint newFreq2 = (NStates0 + ((firstSF >> 7) & 0xFFFFFFFE)) / (uint)(NStates0 + 1);
    if (newFreq2 >= 0x2Cu) newFreq2 = 44;
    ((byte*)&firstSF)[1] = (byte)newFreq2;
    FreeUnits_(states, (uint)((NStates0 + 2) >> 1));
    STATE& one = ctx->oneState();
    FoundState = &one;
    one.Symbol = (byte)firstSF;
    one.Freq = (byte)(firstSF >> 8);
    one.iSuccessor = firstSucc;
    ctx->Flags = SSE0[(byte)firstSF];
  }

  void RescaleCtx(PPM_CONTEXT* ctx) {
    int NStates0 = ctx->NStates;
    int totalCount = NStates0 + 1;
    STATE* states = ctx->getStates(this);
    int modeBit = (ctx->Flags & 0x40) >> 6;
    int shift = modeBit + 1;
    int multA = 2 * modeBit + 2;
    int mask = (1 << shift) - 1;
    int bias = (OrderFall != MaxOrder) ? multA : -modeBit;
    SwapFoundStateToFront_(states);
    STATE* lastState = RescaleAndCompactStates_(ctx, states, totalCount, bias, mask, shift);
    if (lastState->Freq) {
      ctx->Flags |= 0x40;
      FoundState = ctx->getStates(this);
      return;
    }
    int dropped = 0;
    do {
      ++dropped;
      --lastState;
    } while (!lastState->Freq);
    int newNStates = (byte)(NStates0 - dropped);
    ctx->NStates = (byte)newNStates;
    if (newNStates) {
      uint oldNU = (uint)((NStates0 + 2) >> 1);
      uint newNU = (uint)((newNStates + 2) >> 1);
      STATE* newStates = (STATE*)ShrinkUnits_(states, oldNU, newNU);
      ctx->iStates = Ptr2Indx(newStates);
      ctx->Flags |= 0x40;
      FoundState = newStates;
      return;
    }
    CollapseRescaledToOneState_(ctx, states, NStates0);
  }

  uint halfRoundUp_(uint x) { return x - (x >> 1); }

  void SseScale1(SseCounter* cnt) {
    PPM_CONTEXT* topCtx = MaxContext;
    uint freq0Old = cnt->freq0;
    uint weight = cnt->freq1 * (cnt->sum / (11 * freq0Old) + 1);
    if (weight >= 0x800) weight = (weight & 0xFFFF0000u) | 2048u;
    uint nStatesP1 = topCtx->NStates + 1;
    uint perStateF0 = cnt->freq0 / nStatesP1;
    uint complement = (256 - nStatesP1) * topCtx->SummFreq / nStatesP1;
    uint targetSum = perStateF0 * complement;
    if (cnt->sum < targetSum) targetSum = cnt->sum;
    uint freq0Bound = (freq0Old > 16 * nStatesP1) ? cnt->freq0 : 16 * nStatesP1;
    uint halfSum = halfRoundUp_(targetSum);
    uint halfFreq0 = halfRoundUp_(freq0Bound);
    if ((word)weight > 0x400u) {
      cnt->sum = halfRoundUp_((uint)halfSum);
      cnt->freq0 = (word)halfRoundUp_(halfFreq0);
      cnt->freq1 = (word)weight >> 1;
      return;
    }
    cnt->freq0 = (word)halfFreq0;
    cnt->sum = (uint)halfSum;
    short newFreq1;
    if ((word)weight > 0x200u) {
      newFreq1 = (short)(weight - 128);
    } else if ((word)weight > 0x40u) {
      newFreq1 = (short)(weight - 32);
    } else {
      uint dec = (32 - (uint)(word)weight) >> 31;
      newFreq1 = (short)(weight - dec);
    }
    cnt->freq1 = (word)newFreq1;
  }

  typedef SseCounter SseSlot;
  void SseScale2(SseSlot* s) {
    uint scale = s->freq0;
    uint q = ((scale >> 1) + ((uint)s->hits << 15)) / scale;
    if (q >= 0x7FFF) q = 0x7FFF;
    if (q <= 1) q = 1;
    uint predHi = s->predHi;
    int squareErr = (int)(predHi - q) * (int)(predHi - q);
    uint uncertainty = (0x10000 - (q + predHi)) * (q + predHi);
    uint weight = s->freq1;
    uint gainCap = (weight < 4096) ? weight : 4096;
    uint partial = (weight * (uncertainty >> 11)) >> 4;
    char gainShift = (gainCap <= 24 || gainCap == 4096) ? 14 : 1;
    sqword gain;
    if (2 * squareErr <= 3 * (int)partial) {
      gain = (sqword)gainCap * ((((int)partial >> gainShift) + 5 * squareErr) / (int)partial + 1);
      if ((uint)gain >= 0x2000) gain = 0x2000;
    } else {
      gain = 0x2000;
    }
    short newWeight;
    if ((word)gain <= 0x40u && (word)gain <= (uint)s->freq1) {
      if ((word)gain <= 0x20u) {
        uint gt1 = ((uint)1 - (uint)(word)gain) >> 31;
        uint gt24 = ((uint)24 - (uint)(word)gain) >> 31;
        newWeight = (short)((uint)gain - gt1 - gt24);
      } else {
        newWeight = (short)((uint)gain - 8);
      }
    } else {
      newWeight = (short)((word)gain >> 1);
    }
    s->freq1 = (word)newWeight;
    uint hits = s->hits;
    uint newPredHi;
    if ((word)newWeight <= 2u) {
      int bumpHi = 17 * (8 * hits < scale);
      int bumpLo = (8 * (scale - hits) < scale);
      uint twoScale = 2 * scale;
      gain = (uint)(1431655766u * twoScale);
      newPredHi = (uint)(-6 * bumpLo + bumpHi + 2 * (int)hits) / 3u;
      s->freq0 = (word)(twoScale / 3u);
    } else {
      s->freq0 = (word)(scale >> 1);
      int bumpDown = -12 * (6 * (scale - hits) < scale);
      int bumpUp = 16 * (4 * hits < scale);
      newPredHi = (uint)((int)hits + bumpDown + bumpUp) >> 1;
    }
    s->hits = (word)newPredHi;
    s->predHi = (word)q;
  }

  qword StartModelRare(int mode) {
    OrderFall = 0;
    OrderFall0 = 0;
    memset(SymMask, 0, sizeof(SymMask));
    SymCount = 1;
    qword result = 0;
    if (mode != 1 || RunLength == -100) {
      InitHeapAllocator_();
      result = InitRootContextAndStates_();
      if (mode != 2 || RunLength == -100) {
        ResetSseAndMatchTables_();
        InitModelScalars_();
        InitBijectMapSelfTrigrams_();
        InitPrimarySseTable_();
        InitSecondarySseTable_();
        InitFlatSseTables_();
        InitBinSseFromB22_();
        InitPredWeightFromB23_();
        InitSseCellArrays_();
        result = 86016;
      }
    } else {
      result = (qword)RootContext;
      PPM_CONTEXT* rootP = RootContext;
      int suffixCount = 0;
      if (rootP->iSuffix) {
        result = (qword)HeapNull;
        for (PPM_CONTEXT* w = rootP->getSuffix(this); ; w = w->getSuffix(this)) {
          ++suffixCount;
          if (!w->iSuffix) break;
        }
        OrderFall = suffixCount;
      }
      OrderFall0 = suffixCount;
    }
    return result;
  }

  qword CreateSuccessors(int depth, STATE** chainStart, PPM_CONTEXT* seedCtx) {
    uint seedSuccIdx = (*chainStart)->iSuccessor;
    STATE** chainPtr = chainStart;
    seedCtx = FindSeedContextForSuccessors_(chainPtr, seedSuccIdx, seedCtx);
    STATE** chainEnd = chainStart + depth;
    if (chainPtr == chainEnd) return Ptr2Indx(seedCtx);
    byte* baseCtxAddr = (byte*)Indx2Ptr(seedSuccIdx);
    byte newSym = *baseCtxAddr;
    byte newFlags = SSE0[newSym];
    int newCtxBytePos = Ptr2Indx(baseCtxAddr) + 1;
    return AllocateBinarySuccessorChain_(chainPtr, chainEnd, seedCtx, newSym, newFlags, newCtxBytePos);
  }

  qword UpdateModel(PPM_CONTEXT* ctx, uint Order) {
    if (ctx->NStates == 0) {
      STATE& one = ctx->oneState();
      if (one.hasSuccessor(this)) {
        PPM_CONTEXT* succPtr = one.getSuccessor(this);
        if (Order < (uint)MaxOrder) {
          uint res = (uint)UpdateModel(succPtr, Order + 1);
          one.iSuccessor = res;
          if (res) goto at_return;
        } else {
          one.iSuccessor = 0;
        }
        if (Order < PPM_CONTEXT::O_BOUND) goto at_return;
      }
      FreeContext_(ctx);
      return 0;
    }
    {
      int NStates0 = ctx->NStates;
      byte oldFlags = ctx->Flags;
      STATE* states = ctx->getStates(this);
      int NU = (NStates0 + 2) >> 1;
      STATE& mru = ctx->mruState(this);
      if (mru.Freq == 0) {
        mru.Freq = 8;
        ctx->SummFreq += 8;
      }
      int i = PackStatesWithSuccessor_(states, NStates0);
      STATE* p0;
      if (i == NStates0 || Order == 0) {
        p0 = (STATE*)MoveUnits_(states, NU);
        ctx->iStates = Ptr2Indx(p0);
      } else {
        ctx->NStates = (byte)i;
        if (i < 0) {
          FreeUnits_(states, NU);
          FreeContext_(ctx);
          return 0;
        }
        if (i == 0) {
          p0 = CollapseToOneStateSurvivor_(ctx, states, NU);
        } else {
          p0 = ShrinkAndRescaleStates_(ctx, states, NU, i, oldFlags);
          i = ctx->NStates;
        }
      }
      RecurseOnUpdateSurvivors_(p0, i, Order);
    }
  at_return:
    if (Order == (uint)MaxOrder)
      ctx = MoveContext_(ctx);
    return (qword)(uint)Ptr2Indx(ctx);
  }

  qword RestoreModelFromFallback_(PPM_CONTEXT* rootCtx, PPM_CONTEXT* rootCtxSaved, PPM_CONTEXT* parentCtx) {
    if (!CutOff) return StartModelRare(2);
    PPM_CONTEXT* rootCtxP = rootCtx;
    if (rootCtxP->NStates == 1) {
      if (parentCtx->NStates == 0 && rootCtxP != parentCtx) {
        CollapseTwoStateChainToOneState_(rootCtxSaved, parentCtx, parentCtx->oneState().Symbol);
      }
    }
    if (rootCtxP->iSuffix) {
      while (rootCtxP->iSuffix) rootCtxP = rootCtxP->getSuffix(this);
      rootCtx = rootCtxP;
      RootContext = rootCtx;
    }
    qword result = UpdateModel(rootCtxP, 0);
    ++GlueCount;
    CutOffCount = 0;
    pText = (byte*)BList + N_INDEXES*UNIT_SIZE;
    MaxContext = rootCtx;
    OrderFall = 0;
    return result;
  }

  qword ReduceOrder() {
    qword result;
    PPM_CONTEXT* maxCtxStart;
    byte *pTextEntry, *pTextNext, *pTextNewSlot;
    qword succIdx;
    PPM_CONTEXT *succAddr, *curCtx;
    PPM_CONTEXT *rootCtxAtBuildEntry, *rootCtxAtSearchEntry;
    uint newByteIdx, curCtxSuffix;
    int sym;
    STATE *chainEndState;
    STATE **chainPtr, **chainPtrEnd;
    STATE** ctxChainEndS;
    PPM_CONTEXT* rootCtx = RootContext;
    STATE* localFoundState = FoundState;
    int orderFall = OrderFall;
    int maxOrder = MaxOrder;
    uint succIdxW = localFoundState->iSuccessor;
    byte foundSym = localFoundState->Symbol;
    PPM_CONTEXT* walkCtx = RootContext;
    PPM_CONTEXT* rootCtxSaved = RootContext;
    char sse0Bit = SSE0[localFoundState->Symbol];
    if( OrderFall==MaxOrder&&succIdxW ) {
      uint succCreatedTop = CreateSuccessors(1, CtxChain, MaxContext);
      localFoundState->iSuccessor = succCreatedTop;
      if( succCreatedTop ) {
        result = (qword)Indx2Ptr(succCreatedTop);
        RootContext = (PPM_CONTEXT*)result;
        MaxContext = (PPM_CONTEXT*)result;
        return result;
      }
      goto REDUCE_FALLBACK;
    }
    pTextEntry = pText;
    *pText = localFoundState->Symbol;
    pTextNext = pTextEntry + 1;
    pTextNewSlot = pTextNext;
    pText = pTextNext;
    newByteIdx = Ptr2Indx(pTextNext);
    if (pTextNext >= UnitsStart)
      goto REDUCE_FALLBACK;
    *pTextNext = 0;
    if( !succIdxW ) {
      maxCtxStart = MaxContext;
      ctxChainEndS = CtxChainEnd;
      sym = localFoundState->Symbol;
      rootCtxAtSearchEntry = rootCtx;
      curCtx = MaxContext;
      chainPtr = CtxChain;
      STATE* state;
      while( 1 ) {
        PPM_CONTEXT* pc = curCtx;
        if( chainPtr>=ctxChainEndS ) {
          state = pc->NStates
                    ? FindStateBySymbol_(pc->getStates(this), (byte)sym)
                    : &pc->oneState();
          *chainPtr++ = state;
        } else {
          state = *chainPtr++;
        }
        succIdxW = state->iSuccessor;
        if( succIdxW )
          break;
        state->iSuccessor = newByteIdx;
        curCtxSuffix = pc->iSuffix;
        OrderFall = --orderFall;
        if (!curCtxSuffix) {
          succIdxW = Ptr2Indx(curCtx);
          goto SUCC_FOUND;
        }
        curCtx = (PPM_CONTEXT*)Indx2Ptr(curCtxSuffix);
      }
      chainEndState = state;
      chainPtrEnd = chainPtr;
      if( succIdxW<=newByteIdx ) {
        succIdxW = CreateSuccessors(0, chainPtrEnd-1, curCtx);
        chainEndState->iSuccessor = succIdxW;
      }
      if( orderFall==maxOrder-1&&maxCtxStart==rootCtxAtSearchEntry ) {
        localFoundState->iSuccessor = succIdxW;
        succIdxW = chainEndState->iSuccessor;
        pTextNewSlot = pTextEntry;
        pText = pTextEntry;
      }
  SUCC_FOUND:
      if( succIdxW ) {
        succIdx = succIdxW;
        goto BUILD_SUCCESSORS;
      }
  REDUCE_FALLBACK:
      return RestoreModelFromFallback_(rootCtx, rootCtxSaved, walkCtx);
    }
    maxCtxStart = MaxContext;
    succIdx = succIdxW;
    if( UnitsStart > (byte*)Indx2Ptr(succIdxW) ) {
      succIdxW = CreateSuccessors(0, CtxChain, MaxContext);
      goto SUCC_FOUND;
    }
  BUILD_SUCCESSORS:
    succAddr = (PPM_CONTEXT*)Indx2Ptr((uint)succIdx);
    {
      PPM_CONTEXT* succCtx = succAddr;
      OrderFall = orderFall + 1;
      if (OrderFall == maxOrder) {
        newByteIdx = succIdxW;
        pText = pTextNewSlot - (rootCtx != maxCtxStart);
      }
      result = succCtx->iStates;
    }
    if( rootCtx!=maxCtxStart ) {
      int escIdx = EscIndexSeed+8;
      rootCtxAtBuildEntry = rootCtx;
      if (escIdx >= 14) escIdx = 14;
      if (escIdx < 0) escIdx = 0;
      char sse0BitSaved = sse0Bit;
      uint escIdxClipped = escIdx;
      uint succIdxSaved = newByteIdx;
      while( 1 ) {
        PPM_CONTEXT* curCtxP = walkCtx;
        uint nStatesP1 = curCtxP->NStates + 1;
        qword newStateEnd;
        if (curCtxP->NStates) {
          qword newStatesIdx2 = GrowStateBlockIfNeeded_(curCtxP, nStatesP1);
          if (!newStatesIdx2) { rootCtx = rootCtxAtBuildEntry; goto REDUCE_FALLBACK; }
          newStateEnd = ShiftStatesBackForTail_(curCtxP,
                                                  (qword)Indx2Ptr((uint)newStatesIdx2) + 6LL*nStatesP1);
        } else {
          newStateEnd = PromoteSingleStateContext_(curCtxP, escIdxClipped);
          if (!newStateEnd) { rootCtx = rootCtxAtBuildEntry; goto REDUCE_FALLBACK; }
        }
        result = AppendTailStateToBlock_(curCtxP, newStateEnd, foundSym,
                                           succIdxSaved, sse0BitSaved);
        walkCtx = curCtxP->getSuffix(this);
        if (walkCtx == maxCtxStart) break;
      }
    }
    MaxContext = succAddr;
    RootContext = succAddr;
    return result;
  }

  void UpdateWeightPair_(int* w, int delta, int decay) {
    JREG(w, 2*sizeof(int));
    int p0 = w[0] - delta;
    int p1 = w[1] + delta - decay;
    if (p1 > 0x40000000) { p0 >>= 1; p1 >>= 1; }
    w[0] = p0;
    w[1] = p1;
  }

  int MatchPosHint_(int hist_off, int symEpoch, int symEpochN, int sc) {
    int prev1 = MatchPosPrev[(symEpoch - hist_off) & 0x1FFFF];
    if ((uint)(symEpochN - prev1) >= 0x20000) return -1;
    byte h1 = MatchPosHash[(prev1 + hist_off + 1) & 0x1FFFF];
    SymLastCtx[h1] = sc;
    int prev2 = MatchPosPrev[prev1 & 0x1FFFF];
    if ((uint)(symEpochN - prev2) < 0x20000) {
      byte h2 = MatchPosHash[(prev2 + hist_off + 1) & 0x1FFFF];
      SymLastCtx[h2] = sc;
      if ((uint)h1 == (uint)h2) {
        if (HashSeed2 < 0) HashSeed2 = h1;
        else if (HashSeed2 == (uint)h1) HashSeed1 = h1;
        MatchPosBySym[h1] = sc;
      }
    }
    return h1;
  }

  void MatchPosHint16_(int hist_off, int symEpoch, int symEpochN, int sc) {
    int prev1 = MatchPosPrev[(symEpoch - hist_off) & 0x1FFFF];
    if ((uint)(symEpochN - prev1) >= 0x10000) return;
    byte h1 = MatchPosHash[(prev1 + hist_off + 1) & 0x1FFFF];
    SymLastCtx[h1] = sc;
    int prev2 = MatchPosPrev[prev1 & 0x1FFFF];
    if ((uint)(symEpochN - prev2) < 0x20000) {
      byte h2 = MatchPosHash[(prev2 + hist_off + 1) & 0x1FFFF];
      if (h2 == h1) MatchPosBySym[h2] = sc;
      else SymLastCtx[h2] = sc;
    }
  }

  void PairHashUpdate_(byte* arrA, byte* arrB, uint readIdx, uint writeIdx, byte sym, int sc) {
    byte a = arrA[readIdx];
    byte b = arrB[readIdx];
    SymLastCtx2[a] = sc;
    SymLastCtx2[b] = sc;
    if ((uint)b == (uint)a) {
      if (b == HashSeed2) HashSeed1 = b;
      MatchPosBySym[b] = sc;
    }
    arrA[writeIdx] = arrB[writeIdx];
    arrB[writeIdx] = sym;
  }

  void BijectMapCellInsert_(byte* bm, byte newSym) {
    if (newSym == bm[0]) {
      bm[3] += (bm[3] < 255);
    } else {
      bm[2] = bm[1];
      bm[1] = bm[0];
      bm[0] = newSym;
      bm[3] = 0;
    }
  }

  void HalveSse2Histogram_(byte* sse2Base, uint* counter) {
    *counter = 0;
    for (int j = 0; j < 512; ++j) {
      int halved = sse2Base[j] >>= 1;
      *counter += halved;
    }
  }

  void WalkM2Consensus_(int symEpoch, uint symEpochN, int sc) {
    int m2_prev1 = MatchPosPrev[(symEpoch-2) & 0x1FFFF];
    if ((uint)(symEpochN-m2_prev1) >= 0x20000) return;
    int m2_h1 = (byte)MatchPosHash[(m2_prev1+3) & 0x1FFFF];
    hint.hintSymM2 = m2_h1;
    SymLastCtx2[m2_h1] = sc;
    int m2_prev2 = MatchPosPrev[m2_prev1 & 0x1FFFF];
    if ((uint)(symEpochN-m2_prev2) >= 0x20000) return;
    int m2_h2 = (byte)MatchPosHash[(m2_prev2+3) & 0x1FFFF];
    SymLastCtx2[m2_h2] = sc;
    if (m2_h1 == m2_h2)
      MatchPosBySym[m2_h1] = sc;
    int m2_h3 = -1;
    int m2_prev3 = MatchPosPrev[m2_prev2 & 0x1FFFF];
    if ((uint)(symEpochN-m2_prev3) < 0x20000) {
      int m2_bias = 0;
      do {
        m2_bias += 6144;
        m2_h3 = (byte)MatchPosHash[(m2_prev3+3) & 0x1FFFF];
        SymLastCtx2[m2_h3] = sc;
        m2_h1 &= m2_h3;
        m2_h2 = m2_h3 | m2_h2;
        m2_prev3 = MatchPosPrev[m2_prev3 & 0x1FFFF];
      } while ((uint)(m2_bias+symEpochN-m2_prev3) < 0x20000);
    }
    if (m2_h1 == m2_h2 && m2_h3 >= 0) {
      if (HashSeed2 < 0)
        HashSeed2 = m2_h1;
      else if (HashSeed2 == m2_h1)
        HashSeed1 = m2_h1;
    }
  }

  void WalkRecentPosChain_(int recentEpoch, int symEpoch, int sc) {
    int rp = RecentPos[recentEpoch & 0xFFF];
    if ((uint)(symEpoch - rp) >= 0xC0) return;
    uint cnt = 192;
    do {
      --cnt;
      SymLastCtx2[(byte)MatchPosHash[(rp+1) & 0x1FFFF]] = sc;
      rp = RecentPos[rp & 0xFFF];
    } while (cnt > symEpoch - rp);
  }

  void EmitDeltaPredictionHints_(byte predA, byte predB, int sc) {
    SymLastCtx[(byte)(predB+2)] = sc;
    SymLastCtx[(byte)(predB-2)] = sc;
    SymLastCtx[(byte)(predB+1)] = sc;
    SymLastCtx[predB] = sc;
    SymLastCtx2[(byte)(predA+2)] = sc;
    MatchPosBySym[(byte)(predA-2)] = sc;
    MatchPosBySym[(byte)(predA+1)] = sc;
    MatchPosBySym[(byte)(predA-1)] = sc;
    MatchPosBySym[predA] = sc;
  }

  void EmitFlatPredictionHints_(byte pred, int sc) {
    SymLastCtx[(byte)(pred+1)] = sc;
    SymLastCtx[(byte)(pred-1)] = sc;
    SymLastCtx[(byte)(pred+2)] = sc;
    SymLastCtx[(byte)(pred-2)] = sc;
    MatchPosBySym[(byte)(pred+1)] = sc;
    MatchPosBySym[(byte)(pred-1)] = sc;
    MatchPosBySym[pred] = sc;
  }

  void BijectMapTriPrediction_(uint b1, uint b2, int b3, char* bmPtr, int sseScale, const byte* bmCell, int sc, int predGuessSym) {
    if (b1 == b2 && b2 == b3) {
      PrevSymbol = predGuessSym;
      FoundSymbol = b1;
      MatchPosBySym[(byte)(b1+1)] = sc;
      SymLastCtx2 [(byte)(b1-1)] = sc;
    } else if (FoundSymbol < 0) {
      if (b1 == b2 || b2 == b3) {
        PrevSymbol = predGuessSym;
        FoundSymbol = b1;
        SymLastCtx2 [(byte)(b3+1)] = sc;
        MatchPosBySym[(byte)(b1+1)] = sc;
        SymLastCtx2 [(byte)(b1-1)] = sc;
        MatchPosBySym[b3] = sc;
      } else if (b1 == b3) {
        PrevSymbol = predGuessSym;
        if (b2 == (byte)bmPtr[-4*sseScale] && (byte)bmPtr[-5*sseScale] == b3)
          b1 = b2;
        FoundSymbol = b1;
        MatchPosBySym[b2] = sc;
      } else {
        char predDelta = b1 + b3 - 2*b2;
        if (predDelta) {
          PrevSymbol = predGuessSym;
          if (bmCell[3] <= 0x10u) {
            if ((byte)(predDelta+19) <= 0x26u) {
              byte predA = 2*b1 - b2;
              byte predB = predA - predDelta;
              EmitDeltaPredictionHints_(predA, predB, sc);
            }
          } else {
            FoundSymbol = bmCell[0];
          }
        } else {
          FoundSymbol = (byte)(2*b1 - b2);
          PrevSymbol = FoundSymbol;
          EmitFlatPredictionHints_(FoundSymbol, sc);
        }
      }
    } else {
      PrevSymbol = predGuessSym;
    }
  }

  void ComputeMatchHints_(int matchTblHit, uint sym) {
    if ((uint)(SymEpoch-matchTblHit) >= 0x20000) {
      matchEpoch2 = 0x20000;
      matchPosAge = 0x20000;
      matchHashSy = 0x20000;
    } else {
      matchHashSy = (byte)MatchPosHash[(matchTblHit+2) & 0x1FFFF];
      matchPosAge = SymEpoch - matchTblHit;
      matchEpoch2 = SymEpoch - MatchPosTable[256*sym + matchHashSy];
    }
  }

  byte HashArmUpdate_(byte* arr, uint readKey, uint writeKey, byte sym, int sc) {
    byte h = arr[readKey];
    if (sc == SymLastCtx[h]) SymLastCtx2[h] = sc;
    else SymLastCtx[h] = sc;
    arr[writeKey] = sym;
    return h;
  }

  STATE* FindAndBubble7_(STATE* states, byte sym, byte* flagsByte, int margin) {
    STATE* p = states;
    if (p->Symbol == sym) return p;
    do { ++p; } while (p->Symbol != sym);
    STATE saved = *p;
    STATE* stopAt7 = states + 7;
    while (true) {
      bool stable = (p <= states) || ((int)saved.Freq + margin <= (int)(p - 1)->Freq);
      if (stable && p <= stopAt7) {
        *p = saved;
        *flagsByte |= (byte)(p - states);
        return p;
      }
      *p = *(p - 1);
      --p;
    }
  }

  void RefreshIfRank0Empty_(PPM_CONTEXT* ctx, byte& flags, STATE*& states, STATE** chainEnd) {
    if (states[flags & 0xF].Freq == 0) {
      CtxChainEnd = chainEnd;
      BinEscFreq(ctx);
      flags = ctx->Flags;
      states = ctx->getStates(this);
    }
  }

  void RunFastOneStateWalk_(PPM_CONTEXT*& walkCtx, STATE**& chain, int& depthLeft, int ofall) {
    do {
      if (chain > &CtxChain[3]) {
        STATE* prevSt = *(chain - 2);
        prevSt->Freq += (prevSt->Freq < 2);
      }
      *chain++ = &walkCtx->oneState();
      --depthLeft;
      walkCtx = walkCtx->getSuffix(this);
    } while (walkCtx->NStates == 0);
    if (ofall < MaxOrder) {
      STATE* head0 = CtxChain[0];
      if (head0->Freq < 7u) {
        if (chain <= &CtxChain[3]) {
          ++CtxChain[1]->Freq;
        } else {
          ++CtxChain[2]->Freq;
          if (chain > &CtxChain[4]) {
            head0->Freq += (head0->Freq < 4);
          }
        }
      }
    }
  }

  int RunDeepFindWalk_(PPM_CONTEXT*& walkCtx, STATE**& chain, int& depthLeft, byte searchSym, STATE* foundState, int minNStates, PPM_CONTEXT* minCtx) {
    int sseWeight = 2;
    int depth5 = 5 * OrderFall;
    byte newFoundFreq;
    uint sseFlag;
    if (minNStates)
      sseFlag = ((uint)minCtx->SummFreq < (uint)(45*minNStates));
    else
      sseFlag = orderBumpVariance == 0;
    int ofallP3 = OrderFall + 3;
    int ofallP1 = OrderFall + 1;
    int depth3 = 3 * OrderFall;
    do {
      byte deepFlags = walkCtx->Flags;
      STATE* deepStates = walkCtx->getStates(this);
      RefreshIfRank0Empty_(walkCtx, deepFlags, deepStates, chain);
      walkCtx->Flags = deepFlags & 0xF0;
      STATE* deepFound = FindAndBubble7_(deepStates, searchSym, &walkCtx->Flags, 13);
      int foundFreq = foundState->Freq;
      *chain++ = deepFound;
      depth5 -= 5;
      depth3 -= 3;
      --depthLeft;
      if (foundFreq > 130 || deepFound->Freq >= 0xE4u) {
        walkCtx = walkCtx->getSuffix(this);
        break;
      }
      int deepSumFreq = walkCtx->SummFreq;
      short sseBoostA = sseWeight + (depth5 > ofallP3);
      sseWeight >>= 1;
      short sseBoostB = (7*deepSumFreq < 4u*(uint)deepFound->Freq*(walkCtx->NStates + 1u))
                      + (2*depthLeft > ofallP1)
                      + sseBoostA;
      walkCtx->SummFreq = (word)(sseBoostB + deepSumFreq);
      newFoundFreq = sseBoostB + deepFound->Freq;
      deepFound->Freq = newFoundFreq;
      walkCtx = walkCtx->getSuffix(this);
    } while (newFoundFreq < 0x45u && depth3 > ofallP3 && sseFlag);
    return ofallP1;
  }

  STATE** RunOrderFallWalk_(PPM_CONTEXT* walkCtx, int depthLeft, byte searchSym, STATE* foundState, int ofall, int minNStates, PPM_CONTEXT* minCtx, STATE** chain) {
    STATE* trailStates;
    byte trailFlags;
    int trailBound;
    if (walkCtx->NStates == 0) {
      RunFastOneStateWalk_(walkCtx, chain, depthLeft, ofall);
      trailFlags = walkCtx->Flags;
      trailStates = walkCtx->getStates(this);
      goto TRAIL_WALK_BODY;
    }
    trailBound = RunDeepFindWalk_(walkCtx, chain, depthLeft, searchSym,
                                    foundState, minNStates, minCtx);
    while (trailBound < 4*depthLeft) {
      trailFlags = walkCtx->Flags;
      trailStates = walkCtx->getStates(this);
      if (trailStates[trailFlags & 0xF].Symbol == (byte)searchSym)
        break;
  TRAIL_WALK_BODY:
      RefreshIfRank0Empty_(walkCtx, trailFlags, trailStates, chain);
      walkCtx->Flags = trailFlags & 0xF0;
      STATE* trailFound = FindAndBubble7_(trailStates, searchSym, &walkCtx->Flags, 1);
      *chain++ = trailFound;
      if ((uint)(trailStates[0].Freq + trailStates[1].Freq) > 0x5F)
        break;
      walkCtx = walkCtx->getSuffix(this);
      --depthLeft;
      trailBound = ofall + 1;
    }
    return chain;
  }

  void RunPairHashCascade_(uint sym, int sc, int symEpochN) {
    qword epochBit = (symEpochN&1)==0;
    int savedIdx = pairHashIdxByEpoch[epochBit];
    uint newIdx = (word)(16*(word)pairHashIdxByEpoch[epochBit]+((sym>>2)&0xFFFC))&0xFFFC;
    pairHashIdxByEpoch[epochBit] = newIdx;
    uint otherIdx = (uint)pairHashIdxByEpoch[!epochBit];
    hint.hintSymB33 = SseState2[0x60002 + newIdx];
    PairHashUpdate_(&SseState2[0x60003], &SseState2[0x60002], newIdx, otherIdx, sym, sc);
    {
      uint savedIdx34 = (uint)symHalfHistory;
      uint newIdx34 = (word)(((sym>>4)&0xFFFE)+8*symHalfHistory)&0xFFFE;
      PairHashUpdate_(&SseState2[0x50001], &SseState2[0x50000], newIdx34, savedIdx34, sym, sc);
      symHalfHistory = newIdx34;
    }
    PairHashUpdate_(&SseState2[0x60000], &SseState2[0x60001], otherIdx, savedIdx, sym, sc);
  }

  void UpdateSseScale_(uint sym, int dt, int recentEpoch, int recentForHi, const char* sseSlot, qword matchHi, int prevSymCount) {
    if (sym == FoundSymbol && SseScale <= 256) {
      sseScaleCntr = 4 * SseScale;
    } else if (sseScaleCntr > (uint)(3 * SseScale)
            && (prevSymCount == SymLastCtx[sym]
             || prevSymCount == SymLastCtx2[sym]
             || (uint)sseScaleCntr > 4 * SseScale - 9)) {
      sseScaleCntr -= SseScale > 13;
    } else if (dt > 1) {
      int rp1 = RecentPos[recentEpoch&0xFFF];
      if (dt == recentEpoch - rp1 && dt == rp1 - RecentPos[rp1&0xFFF] && dt <= 256) {
        int rp2 = RecentPos[recentForHi & 0xFFF];
        bool symAt4Dt = (sym == (byte)sseSlot[-4*dt]);
        bool hiTriad = (dt == recentForHi - rp2)
                     && (dt == rp2 - RecentPos[rp2 & 0xFFF])
                     && ((uint)matchHi == (byte)sseSlot[-3*dt - 1]);
        if (symAt4Dt || hiTriad) {
          SseScale = dt;
          sseScaleCntr = dt;
          bijectCellPtr = (byte*)&bijectCellSink;
        }
      }
    }
  }

  uint BuildSparseHashA_(uint sym, qword matchHi) {
    return (uint)(SseIdx{}
      .bits<13, 5>((uint)matchHi >> 3)
      .bits<8, 5>((uint)sym >> 3));
  }

  struct PerSymCommitOut { int sseCtxOld; int recentEpoch; int sseCtx2New; };
  PerSymCommitOut CommitSymToHashTables_(uint sym, qword matchHi, int symEpoch, char* sseSlot) {
    MarkSparseBitmapHit_();
    SseState3[sseState3Hash] = sym;
    MatchPosHash[symEpoch & 0x1FFFF] = sym;
    *sseSlot = sym;
    recentSym = sym;
    MatchCtxHi = sym;
    sseState3Hash = (sym+(sseState3Hash<<6))&0x1FFFF;
    if (FoundSymbol >= 0 && FoundSymbol != SseCtx3)
      foundSymHist = (foundSymHist << 1) | (sym == FoundSymbol);
    b31KeyPrev = b31Key;
    order1CtxSaved = Order1Ctx;
    SparseHashA = BuildSparseHashA_(sym, matchHi);
    int sseCtxOld = SseCtx;
    uint sse0sym = SSE0[sym];
    RunLength += SseCtx;
    SparseHashB = (8*(sym+SparseHashB))&0xFFF00;
    int recentEpoch = LastEpochBySym[sym];
    int sseCtx2New = SseCtx2+SseCtx2+(sse0sym>>7);
    SseCtx2 = sseCtx2New;
    RecentPos[symEpoch & 0xFFF] = recentEpoch;
    LastEpochBySym[sym] = symEpoch;
    return { sseCtxOld, recentEpoch, sseCtx2New };
  }

  void CommitPredictorDeltas_() {
    {
      SseCounter* binSseCenter = sp.BinSseCenterG;
      binSseCenter->sum += binSseCenter->freq1;
    }
    sp.BinSseHiG->sum += binSseDeltaHi;
    sp.BinSseLoG->sum += binSseDeltaLo;
    sp.PredBaseAG->sum += predBaseDeltaA;
    sp.PredBaseBG->sum += predBaseDeltaB;
    sp.Sse1SlotG[0] += dlt.sse2NumDelta; sp.Sse1SlotG[1] -= dlt.sse2DenDelta;
    sp.SseMatchSlotG[0] += dlt.sseMatchNumDelta; sp.SseMatchSlotG[1] -= dlt.sseMatchDenDelta;
    UpdateWeightPair_(sp.Sse2SlotG, dlt.predWeightDelta + dlt.predSseTotDelta, 2 * sseCum);
    UpdateWeightPair_(sp.Sse3SlotG, dlt.predWeightDelta, sseCum);
  }

  void UpdateRecentPosHint_(int dt, int recentEpoch, int symEpoch, int sc) {
    if ((uint)dt >= 0x104) {
      b31Key = 0;
      hintSymRecent = -1;
    } else {
      int hashByte = (byte)MatchPosHash[(recentEpoch+1)&0x1FFFF];
      hintSymRecent = hashByte;
      if (dt >= 50) hashByte = 0;
      b31Key = hashByte;
      WalkRecentPosChain_(recentEpoch, symEpoch, sc);
    }
  }

  struct MatchDeltaOut { int matchScore; int predGuessSym; };
  MatchDeltaOut UpdateMatchDeltaPrediction_(uint sym, qword matchHi, int symEpoch, int sc) {
    qword matchKey = sym + (matchHi << 8);
    int matchPrev = MatchPosTable[matchKey];
    MatchPosPrev[(symEpoch-1)&0x1FFFF] = matchPrev;
    MatchPosTable[matchKey] = symEpoch - 1;
    uint matchDelta = symEpoch - matchPrev;
    int matchScore = SseIdx{}
      .bits<13, 2>((matchDelta < 0xE800) + (matchDelta < 0xF0) + (matchDelta < 7));
    int predGuessSym = 0;
    if (matchDelta >= 0x1000) {
      Order1Ctx = 0;
      matchHintByte = -1;
    } else {
      Order1Ctx = predGuessSym = (byte)MatchPosHash[(matchPrev+2)&0x1FFFF];
      matchHintByte = (byte)MatchPosHash[(MatchPosPrev[matchPrev&0x1FFFF]+2)&0x1FFFF];
      if (predGuessSym == matchHintByte) {
        MatchPosBySym[predGuessSym] = sc;
      }
      if (matchDelta >= 0x240)
        predGuessSym = 0;
    }
    return { matchScore, predGuessSym };
  }

  void UpdateSse2BaseG_(uint sym) {
    sse2HistRotation *= 2;
    char newQ12Sel = sse2HistRotation;
    if (sym != (uint)RSContext) {
      byte* sse2Base = Sse2BaseG;
      uint* counter = (uint*)(sse2Base + 512);
      *counter += 2;
      qword sseHistOff = ((word)sym - (word)RSContext) & 0x1FF;
      byte newHistCnt = sse2Base[sseHistOff] + 2;
      sse2Base[sseHistOff] = newHistCnt;
      if (newHistCnt > 0xA7u)
        HalveSse2Histogram_(sse2Base, counter);
      newQ12Sel = ++sse2HistRotation;
      if (sym != (uint)order1CtxSaved) {
        Order1LastSymCell_(RSContext, order1CtxSaved) = sym;
        newQ12Sel = sse2HistRotation;
      }
    }
    Sse2BaseG = &SseSubBlocks[516*(newQ12Sel & 3)];
  }

  void EmitMatchPosHintSequence_(int symEpoch, int symEpochN, int sc) {
    hint.hintSymMatch3 = MatchPosHint_ (3, symEpoch, symEpochN, sc);
                    MatchPosHint_ (4, symEpoch, symEpochN, sc);
                    MatchPosHint_ (5, symEpoch, symEpochN, sc);
                    MatchPosHint_ (8, symEpoch, symEpochN, sc);
                    MatchPosHint16_(6, symEpoch, symEpochN, sc);
                    MatchPosHint16_(10, symEpoch, symEpochN, sc);
  }

  void EmitFourHashArmUpdates_(const char* sseSlot, uint sym, int sc) {
    byte sm0 = (byte)*sseSlot, sm1 = (byte)*(sseSlot-1);
    byte sm2 = (byte)*(sseSlot-2), sm3 = (byte)*(sseSlot-3);
    byte sm4 = (byte)*(sseSlot-4), sm6 = (byte)*(sseSlot-6);
    byte sm7 = (byte)*(sseSlot-7), sm8 = (byte)*(sseSlot-8);
    HashArmUpdate_(SseState2, 256*sm1 + sm3, 256*sm2 + sm4, sym, sc);
    HashArmUpdate_(&SseState2[0x10000], 256*sm2 + sm6, 256*sm3 + sm7, sym, sc);
    hint.hintSymB29 =
    HashArmUpdate_(&SseState2[0x20000], 256*sm0 + sm7, 256*sm1 + sm8, sym, sc);
    HashArmUpdate_(&SseState2[0x30000], 256*sm1 + sm7, 256*sm2 + sm8, sym, sc);
  }

  void EmitMainHintCascade_(const char* sseSlot, uint sym, qword matchHi, int symEpoch, int symEpochN, int sc) {
    EmitMatchPosHintSequence_(symEpoch, symEpochN, sc);
    EmitFourHashArmUpdates_(sseSlot, sym, sc);
    hint.hintSymB31 = Order1HintSymCell_(Order1Ctx, b31Key);
    SymLastCtx[hint.hintSymB31] = sc;
    if (sym != b31KeyPrev && sym != order1CtxSaved)
      Order1HintSymCell_(order1CtxSaved, b31KeyPrev) = sym;
    byte vh = MatchPosHash[(RecentPos[LastEpochBySym[matchHi] & 0xFFF] + 2) & 0x1FFFF];
    if (sc == SymLastCtx[vh]) SymLastCtx2[vh] = sc;
    else SymLastCtx [vh] = sc;
  }

  void EmitBijectMapCellHints_(const byte* bmCell, int sc) {
    SymLastCtx[(byte)bmCell[2]] = sc;
    SymLastCtx[bmCell[1]] = sc;
    SymLastCtx[bmCell[0]] = sc;
    hint.hintSymBmCell = bmCell[0];
    if (bmCell[3]) {
      if (bmCell[3] > 1u || hintSymRecent <= 0) hintSymRecent = bmCell[0];
      MatchPosBySym[bmCell[0]] = sc;
    }
  }

  void RunBijectMapPrediction_(const char* sseSlot, uint sym, int sc, int predGuessSym) {
    if (sseScaleCntr) {
      BijectMapCellInsert_(bijectCellPtr, sym);
      char* bmPtr = (char*)sseSlot + 1;
      uint b1 = (byte)bmPtr[-SseScale];
      uint b2 = (byte)bmPtr[-2*SseScale];
      int b3 = (byte)bmPtr[-3*SseScale];
      if( --sseScaleCntr>(uint)SseScale )
        recentSym = b1;
      char* b1Ptr = &bmPtr[-SseScale];
      char* b2Ptr = &bmPtr[-2*SseScale];
      int bm1 = (byte)*(b1Ptr-1);
      qword bmComposite = SseIdx{}
        .bits <8, 6>((b2 & 0x2E) + ((byte)b2Ptr[1] < (uint)(byte)b1Ptr[1]))
        .bit <12> (b1Ptr[2] == b2Ptr[2])
        .bits <14, 3>((sym > (uint)(bm1 + 32)) + (sym > (uint)bm1) + ((int)sym >= bm1));
      bijectCellPtr = (byte*)&BijectMap[4*bmComposite + 4*b1];
      byte* bmCell = bijectCellPtr;
      EmitBijectMapCellHints_(bmCell, sc);
      BijectMapTriPrediction_(b1, b2, b3, bmPtr, SseScale, bmCell, sc, predGuessSym);
    } else {
      PrevSymbol = predGuessSym;
      SseScale = 1024;
      if( FoundSymbol<0 )
        FoundSymbol = HashSeed1;
    }
  }

  void ResetHintSentinels_() {
    hint.hintSymB31 = -1;
    hint.hintSymM2 = -1;
    hint.hintSymB29 = -1;
    hint.hintSymMatch3 = -1;
    hint.hintSymBmCell = -1;
    HashSeed1 = -1;
    HashSeed2 = -1;
    FoundSymbol = -1;
  }

  void RunSymPredictionStep_(uint sym, qword matchHi, const char* sseSlot, int symEpoch, int symEpochN, int sc, int& predGuessSym) {
    ResetHintSentinels_();
    int bdiff = (byte)(sym-matchHi);
    SymEpoch = symEpochN;
    bool bdiffShortCut = false;
    if( bdiff==bdiffSaved ) {
      if( (uint)++ bdiffStickyCnt>1 ) {
        FoundSymbol = predGuessSym = (byte)(2*sym-matchHi);
        bdiffShortCut = true;
      }
    } else {
      bdiffStickyCnt = 0;
      bdiffSaved = (bdiff == 0) ? -1 : bdiff;
    }
    if (!bdiffShortCut) {
      int ssem3 = (byte)*(sseSlot-3);
      int ssem7 = (byte)*(sseSlot-7);
      char ssem11 = *(sseSlot-11);
      byte progResid1 = (byte)(ssem11 + ssem3 - 2*ssem7);
      byte progResid2 = (byte)(*(sseSlot-15) + ssem7 - 2*ssem11);
      if (ssem3==ssem7 || progResid1 || progResid2) {
        if( 4*SseScale-1>(uint)sseScaleCntr ) {
          EmitMainHintCascade_(sseSlot, sym, matchHi, symEpoch, symEpochN, sc);
        }
      } else {
        FoundSymbol = predGuessSym = (byte)(2*ssem3-ssem7);
      }
    }
  }

  qword CommitFoundAndResolveResult_(STATE* foundState, int ofall) {
    qword result;
    if (ofall == MaxOrder && foundState->hasSuccessor(this)) {
      result = (qword)foundState->getSuccessor(this);
      RootContext = (PPM_CONTEXT*)result;
      MaxContext = (PPM_CONTEXT*)result;
    } else {
      result = ReduceOrder();
      ofall = OrderFall;
    }
    OrderFall0 = ofall;
    return result;
  }

  void RecomputePostCommitSeeds_(int matchScore, int sseCtxOld, int sseCtx2New) {
    OrderCtxSeed = SseIdx{}
      .bit <9> (recentSym & 0x80)
      .raw ((uint)matchScore)
      .bit <15>(OrderFall > 0);
    SseSeed = SseIdx{}
      .raw (BinMapTable[sseCtx2New & 0xF])
      .raw ((uint)sseCtxOld << 14)
      .bit <15>(OrderFall > 0)
      .bit <15>(OrderFall > 2);
    SseCtxExtra = SseIdx{}
      .bit <6> (true)
      .bits <12, 3>((OrderFall>32) + (OrderFall>8) + (OrderFall>4) + (OrderFall>3) + (OrderFall>2) + (OrderFall>1));
  }

  qword SseUpdate(PPM_CONTEXT* minCtx) {
    int prevSymCount = SymCount;
    int sc = SymCount - 1;
    SymCount = sc;
    CommitPredictorDeltas_();
    int symEpoch = SymEpoch;
    char* sseSlot = (char*)&MatchPosHash[0x20000 + (symEpoch & 0x1FFFF)];
    STATE* foundState = FoundState;
    uint sym = foundState->Symbol;
    qword matchHi = (qword)MatchCtxHi;
    auto [sseCtxOld, recentEpoch, sseCtx2New] = CommitSymToHashTables_(sym, matchHi, symEpoch, sseSlot);
    int dt = symEpoch-recentEpoch;
    UpdateRecentPosHint_(dt, recentEpoch, symEpoch, sc);
    auto [matchScore, predGuessSym] = UpdateMatchDeltaPrediction_(sym, matchHi, symEpoch, sc);
    UpdateSse2BaseG_(sym);
    int recentForHi = LastEpochBySym[matchHi];
    UpdateSseScale_(sym, dt, recentEpoch, recentForHi, sseSlot, matchHi, prevSymCount);
    int symEpochN = symEpoch + 1;
    RunSymPredictionStep_(sym, matchHi, sseSlot, symEpoch, symEpochN, sc, predGuessSym);
    WalkM2Consensus_(symEpoch, symEpochN, sc);
    RunPairHashCascade_(sym, sc, symEpochN);
    RunBijectMapPrediction_(sseSlot, sym, sc, predGuessSym);
    int ofall = OrderFall;
    CtxChain[0] = foundState;
    int minNStates = minCtx->NStates;
    NMasked = minNStates;
    int searchSym = foundState->Symbol;
    STATE** chain = &CtxChain[1];
    RecomputePostCommitSeeds_(matchScore, sseCtxOld, sseCtx2New);
    if (minCtx->iSuffix) {
      chain = RunOrderFallWalk_(minCtx->getSuffix(this), OrderFall, (byte)searchSym, foundState, ofall, minNStates, minCtx, chain);
      CtxChainEnd = chain;
    } else {
      CtxChainEnd = &CtxChain[1];
    }
    return CommitFoundAndResolveResult_(foundState, ofall);
  }

  int StartSubAllocator(uint memsize_mb, int order, int cutOff) {
    if( memsize_mb==0 ) return 0;
    // Cap kept well under the packing ceiling (~22 GB, where the largest packed
    // unit index would reach 2^32). 0x4FFF MB = ~20 GB leaves headroom.
    //if( memsize_mb>0x4FFF ) memsize_mb = 0x4FFF; // clamp (was: return 0) under the packed-index ceiling
    if( order<2 ) return 0;
    if( order>MAX_O ) return 0;
    if( SubAllocatorSize ) return 0;

    size_t memsize_b = (size_t)memsize_mb << 20;
    HeapStart = new char[memsize_b];
    if( !HeapStart ) return 0;

    RunLength = -100;
    SubAllocatorSize = memsize_b;
    InitsCount = 0;
    CutOff = cutOff;
    Interrupted = 0;
    //if (order > 12) order = 16 << ((order + 19) & 31);
    MaxOrder = order;
    return 1;
  }

  qword PPMIIDeleteModel() {
    if( SubAllocatorSize==0) return 0;
    qword finalSize = SubAllocatorSize;
    if( (InitsCount==1) && (CutOffCount+GlueCount==0) ) {
      qword textSize = ((char*)pText) - ((char*)HeapStart);
      qword USSize = UNIT_SIZE * (MEM_DIVISOR-1) * (SubAllocatorSize / (UNIT_SIZE*MEM_DIVISOR));
      qword scaledUnits = (USSize - (HiUnit-LoUnit)) / (MEM_DIVISOR-1);
      qword maxMetric = (scaledUnits > textSize) ? scaledUnits : textSize;
      qword sizeLimit = MEM_DIVISOR * maxMetric + UNIT_SIZE;
      if( SubAllocatorSize > sizeLimit ) finalSize = sizeLimit;
    }
    SubAllocatorSize = 0;
    free( HeapStart );
    return (finalSize + 0xFFFFF) >> 20;
  }

  struct Rangecoder {
    FILE* In;
    FILE* Out;
    uint Range;
    uint Code;
    uint Cache;
    uint ff_count;
    qword Low;
    uint SubRange;
    uint getSubRange(uint freq, uint totFreq) {
      return freq * (Range / totFreq);
    }

    void encodeSymbol(uint subRange) {
      SubRange = subRange;
    }

    void encodeEscape(uint subRange) {
      Low += subRange;
      Range -= subRange;
    }

    bool IsDecodeMatched(uint subRange) {
      if (Code < subRange) {
        SubRange = subRange;
        return true;
      }
      return false;
    }

    void DecodeNotMatched(uint subRange) {
      Code -= subRange;
      Range -= subRange;
    }

    void commitRange() {
      Range = SubRange;
    }

    void initEncoder(FILE* outFile) {
      Out = outFile;
      Low = 0;
      Cache = 0;
      Range = -1;
      ff_count = 0;
    }

    void initDecoder(FILE* inFile) {
      In = inFile;
      Range = -1;
      int b0 = getc(In);
      int b1 = getc(In);
      int b2 = getc(In);
      int b3 = getc(In);
      int b4 = getc(In);
      int pack1 = b1 | (b0 << 8);
      int pack2 = b2 | (pack1 << 8);
      int pack3 = b3 | (pack2 << 8);
      Code = b4 | (pack3 << 8);
    }

    void emitOneByte() {
      if ((Low ^ 0xFF000000LL) <= 0xFFFFFF) {
        ++ff_count;
      } else {
        putc(Cache + (int)(Low >> 32), Out);
        int carry = (int)(Low >> 32) + 255;
        if (ff_count) {
          do {
            putc(carry, Out);
          } while (--ff_count);
          Low = (uint)Low;
        }
        Cache = (Low >> 24) & 0xFF;
      }
      Low = (uint)(Low << 8);
    }

    void EncodeShift() {
      emitOneByte();
      Range <<= 8;
    }

    void EncodeNormalize() {
      while (Range < 0x1000000) {
        EncodeShift();
      }
    }

    void DecodeNormalize() {
      while (Range < 0x1000000) {
        Code = getc(In) | (Code << 8);
        Range <<= 8;
      }
    }

    void Flush() {
      for (int i = 0; i < 5; ++i)
        emitOneByte();
    }
  };

  FakeRangecoder rc;

  template<typename T>
  void MaybeRescale1_(void* slot, T& freq0Local, uint weight) {
    if ((uint)freq0Local > 0x8000u || weight > 0x80000u) {
      SseScale1((SseCounter*)slot);
      freq0Local = (T)((SseCounter*)slot)->freq0;
    }
  }

  template<typename T>
  void MaybeRescale2_(void* slot, T& freq0Local) {
    if ((uint)freq0Local > 0x8000u) {
      SseScale2((SseSlot*)slot);
      freq0Local = (T)((SseCounter*)slot)->freq0;
    }
  }

  int RescaleAccum1_(void* slot, uint weight, int shift) {
    SseCounter* cnt = (SseCounter*)slot;
    JREG(cnt, sizeof(SseCounter));
    word freq0 = cnt->freq0;
    MaybeRescale1_(slot, freq0, weight);
    int delta = (int)((uint)cnt->freq1 >> shift);
    cnt->freq0 = (word)(freq0 + delta);
    return delta;
  }

  int RescaleAccum2_(void* slot, int shift) {
    SseCounter* cnt = (SseCounter*)slot;
    JREG(cnt, sizeof(SseCounter));
    word freq0 = cnt->freq0;
    MaybeRescale2_(slot, freq0);
    int delta = (int)((uint)cnt->freq1 >> shift);
    cnt->freq0 = (word)(freq0 + delta);
    return delta;
  }

  void SseDeltaUpdate_(int* p, int delta, int absThresh, int denThresh, int adder) {
    JREG(p, 2*sizeof(int));
    int num = p[0];
    int den = p[1];
    if ((int)abs32(p[0]) > absThresh) {
      num >>= 1;
      p[0] = num;
      den >>= (den > denThresh);
    }
    int newNum = num - delta;
    p[0] = newNum;
    int newDen = delta + den + adder;
    if (newDen > 0x40000000) {
      p[0] = newNum >> 1;
      newDen >>= 1;
    }
    p[1] = newDen;
  }

  void SseWeightUpdate_(int* p, int delta, int adder) {
    JREG(p, 2*sizeof(int));
    int num = p[0];
    int den = p[1];
    if ((int)abs32(p[0]) > 0x100000) {
      num >>= 1;
      p[0] = num;
      den >>= (den > 0x2000);
    }
    p[0] = num + delta;
    p[1] = den + adder;
  }

  sqword ClampToBand_(sqword val, sqword lo, sqword hi) {
    sqword r = lo;
    if (val >= lo) {
      r = hi;
      if (val < hi) r = val;
    }
    return r;
  }

  sqword SseClampMean_(int* slot, sqword scale, sqword lo, sqword hi) {
    sqword mean = scale * (sqword)slot[0] / slot[1];
    return ClampToBand_(mean, lo, hi);
  }

  void FreqSseStep_(STATE* st, byte sxFreq, uint sseWeight, uint constSse, int& sumFreq, int& sumFreqW, PPM_CONTEXT* ctx) {
    uint curFreq = st->Freq;
    uint comb = constSse + (uint)sumFreqW;
    if (curFreq * sseWeight >= comb * sxFreq || curFreq > 0xE4u) return;
    int freqDelta = sumFreq - (int)curFreq;
    uint cm = comb - curFreq;
    uint denom = 3*cm + sseWeight - sxFreq;
    uint newFreq = (cm*(3*curFreq + sxFreq) + denom - 4) / denom;
    if (newFreq > curFreq + 11) newFreq = curFreq + 11;
    sumFreq = (int)(newFreq + freqDelta);
    JREG(&ctx->SummFreq, sizeof(word));
    JREG(&st->Freq, sizeof(byte));
    ctx->SummFreq = (word)sumFreq;
    st->Freq = (byte)newFreq;
    sumFreqW = (int)(word)sumFreq;
  }

  void BubbleSortChain_(STATE** chainEnd, STATE** sortLimit, int sortPriority) {
    for (STATE** p = chainEnd - 1; p > sortLimit; --p) {
      STATE* tmp = *(p - 1);
      if (sortPriority * (*p)->Freq <= tmp->Freq) break;
      *(p - 1) = *p;
      *p = tmp;
    }
  }

  void FillFreqMap_(PPM_CONTEXT* ctx) {
    int n = ctx->NStates + 1;
    STATE* s = ctx->getStates(this);
    do {
      SymFreqs[s->Symbol] = s->Freq;
      ++s;
    } while (--n);
  }

  struct WalkEscapeOut { PPM_CONTEXT* ctx; int finalSym; };
  WalkEscapeOut WalkEscapeChain_(PPM_CONTEXT* startCtx, PPM_CONTEXT* refCtx, int escSym) {
    PPM_CONTEXT* w = startCtx;
    byte sym = w->mruState(this).Symbol;
    while (sym == (byte)escSym && w->NStates == refCtx->NStates) {
      w = w->getSuffix(this);
      sym = w->mruState(this).Symbol;
    }
    EscapeSymbol = sym;
    return { w, sym };
  }

  struct Sse1Out { int cumWeight, cumFreq; };
  Sse1Out Sse1Step_(int* slot, uint hits) {
    int cumIn = sseCum;
    int clamp = SseClampMean_(slot, hits, 1 - sseCum, 0x40000);
    SseDeltaUpdate_(slot, cumIn, 0x40000, 4096, 2);
    return { clamp + sseTot, clamp + cumIn };
  }

  void SseMatchStep_(int* slot, int boost) {
    sseCum = boost;
    sseTot = 60416;
    dlt.sseMatchDenDelta = boost;
    dlt.sseMatchNumDelta = 60416;
    int clamp = (int)SseClampMean_(slot, boost, 1 - boost, 0x40000);
    SseDeltaUpdate_(slot, boost, 0x80000, 0x2000, 1120);
    sseTot += clamp;
    dlt.predSseTotDelta = sseTot;
    sseCum = clamp + boost;
  }

  void Sse2Step_(int* slot) {
    int cumIn = sseCum;
    int clamp = SseClampMean_(slot, sseTot - cumIn, cumIn - sseTot + 1, 0x40000);
    sseTot += clamp;
    SseWeightUpdate_(slot, 2 * cumIn, 1);
  }

  void Sse3Step_(int* slot, int sse2CumIn, int totFreqPreSse3) {
    int clamp = SseClampMean_(slot, totFreqPreSse3 - sse2CumIn, sseCum - sseTot + 1, 0x80000);
    sseTot += clamp;
    SseWeightUpdate_(slot, sse2CumIn, 2);
  }

  uint SseCumFreq_(uint sseFreq, uint sseWeight, uint nStatesP1) {
    return ((sseFreq >> 1) + nStatesP1 * sseWeight) / sseFreq + 2;
  }

  void RewindPredictor_(SseCounter* slot, int delta, int mult) {
    JREG(slot, sizeof(SseCounter));
    slot->freq0 -= (short)delta;
    slot->sum += delta * mult;
  }

  void RewindAllPredictors_(uint rewindMult) {
    RewindPredictor_(qs.q[5], qs.q[5]->freq1, rewindMult);
    RewindPredictor_(qs.q[4], qs.wDelta[4], rewindMult);
    RewindPredictor_(qs.q[3], qs.wDelta[3], rewindMult);
    RewindPredictor_(qs.q[2], qs.wDelta[2], rewindMult);
    RewindPredictor_(qs.q[1], qs.wDelta[1], rewindMult);
    RewindPredictor_(qs.q[0], qs.wDelta[0], rewindMult);
  }

  void RescaleAllQSlots_(int shift) {
    qs.wDelta[3] = RescaleAccum2_(qs.q[3], shift);
    qs.wDelta[2] = RescaleAccum2_(qs.q[2], shift);
    qs.wDelta[1] = RescaleAccum2_(qs.q[1], shift);
    qs.wDelta[0] = RescaleAccum2_(qs.q[0], shift);
    qs.wDelta[5] = RescaleAccum2_(qs.q[5], shift);
    qs.wDelta[6] = RescaleAccum2_(qs.q[6], shift);
  }

  void CommitAllQSlotDeltas_() {
    qs.q[3]->sum += qs.wDelta[3];
    qs.q[2]->sum += qs.wDelta[2];
    qs.q[1]->sum += qs.wDelta[1];
    qs.q[0]->sum += qs.wDelta[0];
    qs.q[5]->sum += qs.wDelta[5];
    qs.q[6]->sum += qs.wDelta[6];
  }

  int BuildBigSlotIdxA_(qword sseIdxA, uint maskFlagEsc, uint maskFlagPrev) {
    return (int)(SseIdx{}
      .field<0, 2> ((byte)sseIdxA)
      .bit <3> (maskFlagEsc | maskFlagPrev)
      .field<4, 4> ((byte)sseIdxA));
  }

  struct SseAccum { uint sseWeight, sseFreq; };
  SseAccum BlendInnerNeighbourPair_(SseCounter* sseCell, int stride, char blendShift, char rescaleShift, uint sseWeightIn, uint sseFreqIn) {
    SseCounter* up = sseCell + stride;
    SseCounter* dn = sseCell - stride;
    qs.q[4] = up;
    qs.q[3] = dn;
    uint upWeight = (uint)up->sum;
    uint sseWeight = ((upWeight + (uint)dn->sum) >> blendShift) + sseWeightIn;
    uint sseFreq = ((uint)(up->freq0 + dn->freq0) >> blendShift) + sseFreqIn;
    sseCum = sseWeight;
    sseTot = sseFreq;
    qs.wDelta[4] = RescaleAccum1_(up, upWeight, rescaleShift);
    qs.wDelta[3] = RescaleAccum1_(dn, (uint)dn->sum, rescaleShift);
    return { sseWeight, sseFreq };
  }

  void RescaleXorNeighbourPair_(SseCounter* sseStrideBase, qword sseXorIdx, qword xor1, qword xor2, char shift) {
    SseCounter* sse3Nbr = sseStrideBase + (sseXorIdx ^ xor1);
    SseCounter* sse4Nbr = sseStrideBase + (sseXorIdx ^ xor2);
    qs.q[2] = sse3Nbr;
    qs.q[1] = sse4Nbr;
    qs.wDelta[2] = RescaleAccum1_(sse3Nbr, (uint)sse3Nbr->sum, shift);
    qs.wDelta[1] = RescaleAccum1_(sse4Nbr, (uint)sse4Nbr->sum, shift);
  }

  SseAccum BlendSseCascadeA_(SseCounter* sseCellA, uint sseDeltaA, qword sseQTableIdxA, qword sseIdxA, uint maskFlagEsc, uint maskFlagPrev, uint sseWeightAIn, uint sseFreqAIn) {
    SseCounter* bigSlotA = &EscMirrorSse[256*sseQTableIdxA
                              + BuildBigSlotIdxA_(sseIdxA, maskFlagEsc, maskFlagPrev)];
    qs.q[0] = bigSlotA;
    char sseShiftA = sseDeltaA<0x200;
    SseAccum blended = BlendInnerNeighbourPair_(sseCellA, 1024, sseShiftA+1, sseShiftA,
                                                  sseWeightAIn, sseFreqAIn);
    uint centerExpandA = (uint)sseCellA->freq1;
    bool sseShiftLowA = centerExpandA<0x400u;
    char sseShiftBSel = sseShiftA+sseShiftLowA;
    RescaleXorNeighbourPair_(sseCellA - sseIdxA, sseIdxA, 0x100, 0x200, sseShiftBSel);
    qs.wDelta[0] = RescaleAccum1_(bigSlotA, (uint)bigSlotA->sum, sseShiftA+sseShiftLowA+1);
    return blended;
  }

  struct LES1Entry {
    int nStates;
    uint nStatesPlus1;
    int remStates;
    int escSymbol;
    STATE* stateIter;
  };
  LES1Entry EnterMultiStateBranch_(PPM_CONTEXT* minCtx, int epoch) {
    if (!minCtx->mruState(this).Freq) {
      BinEscFreq(minCtx);
    }
    int nStates = minCtx->NStates;
    STATE* escState = &minCtx->mruState(this);
    int escSymbol = escState->Symbol;
    CtxChain[0] = escState;
    SseCtx3 = escSymbol;
    SymMask[escSymbol] = epoch;
    return { nStates, (uint)(nStates + 1), nStates + 1, escSymbol, minCtx->getStates(this) - 1 };
  }

  struct CentralCellOut { uint sseWeight, sseFreq, delta; };
  CentralCellOut LookupCentralCellAndRescale_(SseCounter* row) {
    qs.q[5] = row;
    uint sseWeight = (uint)row->sum;
    sseCum = sseWeight;
    uint sseFreq = row->freq0;
    sseTot = sseFreq;
    uint delta = RescaleAccum1_(row, sseWeight, 0);
    return { sseWeight, sseFreq, delta };
  }

  bool IsSxRank0SymUnseen_(PPM_CONTEXT* ctx, int epoch) {
    return epoch != SymMask[ctx->mruState(this).Symbol];
  }

  struct EscapeMaskOut { bool maskFlagEsc; int escCandidate; };
  EscapeMaskOut MaskAndWalkEscapeChain_(PPM_CONTEXT* minCtx, int epoch, int escSymbol) {
    PPM_CONTEXT* escWalkCtx = minCtx->getSuffix(this);
    bool maskFlagEsc = IsSxRank0SymUnseen_(escWalkCtx, epoch);
    return { maskFlagEsc, WalkEscapeChain_(escWalkCtx, minCtx, escSymbol).finalSym };
  }

  struct BinarySseSlot {
    SseCounter* slot;
    qword sseIdxB;
    int freq0;
  };
  BinarySseSlot SetupBinarySseSlotB_(int sseCtx, uint suffixNStates, uint seeIndex) {
    qword sseIdxB = sseCtx + (uint)SSE1[suffixNStates];
    SseCounter* sseBaseB = &SseWeight1[0x4000*(qword)(byte)NextBinFreq[seeIndex]];
    qs.q[3] = &sseBaseB[sseIdxB^0x1000];
    qs.q[2] = &sseBaseB[sseIdxB^1];
    qs.q[1] = &sseBaseB[sseIdxB^0x2000];
    qs.q[0] = &sseBaseB[sseIdxB^0x20];
    qs.q[5] = &sseBaseB[sseIdxB^0x200];
    qs.q[6] = &sseBaseB[sseIdxB^0x400];
    SseCounter* slot = &sseBaseB[sseIdxB];
    sp.BinSseCenterG = slot;
    sp.PredBaseAG = slot-2;
    sp.PredBaseBG = slot+2;
    return { slot, sseIdxB, slot->freq0 };
  }

  template<int f_DEC>
  STATE* CommitMatchedBinary_(uint subRange, int cumFreq, uint totFreq, PPM_CONTEXT* minCtx) {
    if (!f_DEC) rc.encodeSymbol(subRange);
    STATE* localFoundState = &minCtx->oneState();
    FoundState = localFoundState;
    SseCtx = 1;
    int oneStateFreqF = localFoundState->Freq;
    byte freqIncCap = (oneStateFreqF < PPM_CONTEXT::BIN_MAX_FREQ);
    byte firstHitBump = (oneStateFreqF == 1 && totFreq < 4u*(uint)cumFreq);
    localFoundState->Freq = oneStateFreqF + freqIncCap + firstHitBump;
    sp.BinSseCellG[0] += 1568;
    CommitAllQSlotDeltas_();
    return localFoundState;
  }

  int RunFreqSseCascadeA_(PPM_CONTEXT* minCtx, uint nStatesPlus1, int nStatesP1Save, int sxSumFreqA0, int minSumFreqA, int nStates) {
    FillFreqMap_(minCtx->getSuffix(this));
    int nStatesCnt = nStates+1;
    STATE* stateBackM = &minCtx->getStates(this)[nStatesPlus1];
    int sseConstM = 11*(nStates+1);
    int sumFreqM = minSumFreqA;
    int sumFreqWM = sumFreqM;
    int sseWeightScale = (sumFreqM > sseConstM) ? 15 : 7;
    int sseWeightM = nStatesP1Save*sseWeightScale + sxSumFreqA0;
    do {
      stateBackM -= 1;
      FreqSseStep_(stateBackM, SymFreqs[stateBackM->Symbol], sseWeightM,
                   sseConstM, sumFreqM, sumFreqWM, minCtx);
    } while (--nStatesCnt);
    return sumFreqWM;
  }

  template<int f_DEC>
  void HandleEscapedCandidate_(uint subRangeC, STATE* localFoundState, PPM_CONTEXT* minCtx) {
    STATE* priorFoundStateF = FoundState;
    cumFreqAcc -= localFoundState->Freq;
    if (f_DEC) rc.DecodeNotMatched(subRangeC); else rc.encodeEscape(subRangeC);
    ClearSparseBitmapHit_();
    if (priorFoundStateF) {
      FoundState = nullptr;
      SseCtx = 0;
    } else if (localFoundState > minCtx->getStates(this)) {
      qs.wDelta[5] = 0x8000;
    }
  }

  void CommitPredWeightPairDeltas_() {
    PredWeightAG[0] += predDeltaNum;
    PredWeightAG[1] += predDeltaDen;
    PredWeightBG[0] += predDeltaNum;
    PredWeightBG[1] += predDeltaDen;
  }

  short ComputeMatchFreqBoost_(byte flagsCtxFC) {
    return (matchPosAge > 0x4800) + (matchPosAge > 0x380) + (matchPosAge > 0x80)
         + ((flagsCtxFC & 0x40) == 0 || matchPosAge > 0xE00) + 4;
  }

  void BubbleMatchedStateToFront_(STATE*& localFoundState, STATE* statesBaseFC) {
    STATE saved = *localFoundState;
    do {
      *localFoundState = *(localFoundState - 1);
      --localFoundState;
    } while (localFoundState > statesBaseFC);
    *localFoundState = saved;
  }

  template<int f_DEC>
  bool CommitMatchedCandidate_(uint subRangeC, STATE*& localFoundState, PPM_CONTEXT* preCommitMinCtx, PPM_CONTEXT* minCtx, int cumFreqC, uint totFreqC) {
    if (!f_DEC) rc.encodeSymbol(subRangeC);
    CommitPredWeightPairDeltas_();
    byte flagsCtxFC = minCtx->Flags;
    short freqBoostFC = ComputeMatchFreqBoost_(flagsCtxFC);
    minCtx->Flags = flagsCtxFC & 0xF0;
    minCtx->SummFreq += freqBoostFC;
    localFoundState->Freq += freqBoostFC;
    STATE* statesBaseFC = minCtx->getStates(this);
    if (localFoundState == statesBaseFC) {
      if (MaxContext == RootContext)
        SseCtx = totFreqC < 2*cumFreqC;
    } else {
      BubbleMatchedStateToFront_(localFoundState, statesBaseFC);
    }
    FoundState = localFoundState;
    if (localFoundState->Freq > PPM_CONTEXT::MAX_FREQ) {
      RescaleCtx(preCommitMinCtx);
      localFoundState = FoundState;
    }
    return localFoundState != nullptr;
  }

  struct PredWeightDecayOut {
    uint predWPostF;
    uint predDenIncF;
  };
  PredWeightDecayOut RunPredWeightCellPairDecay_(uint centerExpandF, int sseEntryC2F, qword candProbBF, int sparseHitsF, qword seeIdxF, int sseCumFreqF, int sseCumWeightF) {
    int* predWBase = &PredWeight[512*(qword)(byte)SEEQTable[candProbBF] + 2*(uint)(sparseHitsF<<6)];
    int* predWAF = &predWBase[2*(seeIdxF & 0x1F)];
    int predWAVal = predWAF[1];
    PredWeightAG = predWAF;
    int* predWBF = &predWBase[2*(((uint)seeIdxF >> 4) & 0x1F) + 64];
    PredWeightBG = predWBF;
    char predBoostShiftF = centerExpandF < 0x150;
    int predWBVal = predWBF[1];
    uint predScaleAF = predWBVal + predWAVal;
    uint predScaledAF = (uint)(predWAVal*(sseEntryC2F+2)) >> 8;
    uint predScaledBF = (uint)(predWBVal*(sseEntryC2F+2)) >> 8;
    int predBoostF = (predScaleAF >= 0xC000) ? 24 : ((predScaleAF >> 12) << 1);
    uint predTotEarlyF = (predScaledAF + predScaledBF >= 0x6000u) ? 24576u
                                                                  : (predScaledAF + predScaledBF);
    int predConstAF = (byte)predBoostTab[predBoostF];
    int predConstBF = (byte)predBoostTab[predBoostF + 1];
    uint predWPostF = ((predConstAF*predTotEarlyF + predConstBF*(*predWBF + *predWAF)) >> (predBoostShiftF + 7))
                     + sseCumFreqF;
    char predShiftF = predBoostShiftF + (centerExpandF < 0x48) + 3;
    uint predDenIncF = ((uint)(192*(predConstAF + predConstBF)) >> predBoostShiftF) + sseCumWeightF;
    predDeltaNum = 0x3000u >> predShiftF;
    char predDenShiftF = predBoostShiftF + 4;
    predDeltaDen = 0x3000u >> predDenShiftF;
    JREG(predWAF, 2*sizeof(int));
    JREG(predWBF, 2*sizeof(int));
    *predWAF -= (*predWAF >> predShiftF);
    predWAF[1] -= (predScaledAF + 7) >> predDenShiftF;
    *predWBF -= (*predWBF >> predShiftF);
    predWBF[1] -= (predScaledBF + 7) >> predDenShiftF;
    return { predWPostF, predDenIncF };
  }

  void RunPredWeightOuterExpansion_(uint centerExpandF, SseCounter* binSseCenter) {
    SseCounter* binUp16F = binSseCenter + 0x4000;
    SseCounter* binDn16F = binSseCenter - 0x4000;
    sp.PredBaseAG = binUp16F;
    sp.PredBaseBG = binDn16F;
    char predExpShiftF = (centerExpandF < 0x1E0) + (centerExpandF < 0x3D0) + 1;
    predBaseDeltaA = RescaleAccum2_(binUp16F, predExpShiftF);
    predBaseDeltaB = RescaleAccum2_(binDn16F, predExpShiftF);
  }

  struct PredWeightStageOut { int cumFreq, cumWeight; };
  PredWeightStageOut RunPredWeightStageF_(uint centerExpandF, int sseEntryC2F, qword candProbBF, int sparseHitsF, qword seeIdxF, SseCounter* binSseCenter, int sseCumFreqIn, int sseCumWeightIn) {
    PredWeightDecayOut pw = RunPredWeightCellPairDecay_(centerExpandF, sseEntryC2F,candProbBF, sparseHitsF,seeIdxF, sseCumFreqIn,sseCumWeightIn);
    SseCounter* binUp8F = binSseCenter + 0x2000;
    SseCounter* binDn8F = binSseCenter - 0x2000;
    sp.BinSseHiG = binUp8F;
    sp.BinSseLoG = binDn8F;
    int cumFreq = ((binUp8F->hits + binDn8F->hits) >> 3) + pw.predWPostF;
    int cumWeight = ((uint)(binUp8F->freq0 + binDn8F->freq0) >> 3) + pw.predDenIncF;
    sseCum = cumFreq;
    sseTot = cumWeight;
    char predDoExpandF = (centerExpandF < 0x30) + 1;
    binSseDeltaHi = RescaleAccum2_(binUp8F, predDoExpandF);
    binSseDeltaLo = RescaleAccum2_(binDn8F, predDoExpandF);
    if (centerExpandF > 0x48) RunPredWeightOuterExpansion_(centerExpandF, binSseCenter);
    return { cumFreq, cumWeight };
  }

  void ScanCandidateStateA_(STATE* stateIter, int escSymbol, int epoch, STATE**& chainEnd, STATE**& chainStart) {
    int walkSym = stateIter->Symbol;
    if (escSymbol == walkSym) return;
    SymMask[walkSym] = epoch;
    *chainEnd++ = stateIter;
    STATE** sortLimit;
    int sortPriority;
    if (walkSym == FoundSymbol) {
      sortLimit = &CtxChain[(foundSymHist & 7) == 0];
      sortPriority = 22;
    } else if (epoch == MatchPosBySym[walkSym] || epoch == SymLastCtx2[walkSym]) {
      sortLimit = chainStart;
      sortPriority = (epoch == MatchPosBySym[walkSym]) ? 18 : 7;
    } else {
      return;
    }
    BubbleSortChain_(chainEnd, sortLimit, sortPriority);
    ++chainStart;
  }

  void SeedEscapeCandidateChain_(PPM_CONTEXT* minCtx, int freqDeltaE, char& descendFlags, STATE**& chainEndE, STATE**& chainEndF, int& freqSumE, int& predShiftFlags, int& remStatesE, int& escSymbol) {
    STATE* firstStateE = &minCtx->mruState(this);
    escSymbol = firstStateE->Symbol;
    SseCtx3 = escSymbol;
    if (SymCount == SymMask[escSymbol]) {
      qs.wDelta[5] = 0x8000;
    } else {
      descendFlags = 1;
      chainEndE = &CtxChain[1];
      CtxChain[0] = firstStateE;
      SymMask[escSymbol] = SymCount;
      chainEndF = &CtxChain[1];
      freqSumE = firstStateE->Freq;
      predShiftFlags = 0;
      remStatesE = freqDeltaE - 1;
      qs.wDelta[5] = 0;
    }
  }

  int BuildBigSlotIdxC_(qword sseIdxC, int sxNStatesC, int ofallSavedE, int maskFlagPrevC, int escCandidate, PPM_CONTEXT* minCtx, int epoch) {
    return (int)(SseIdx{}
      .field<0, 2> ((byte)sseIdxC)
      .bit <2> (ofallSavedE < 10)
      .bit <3> (minCtx->NStates + 2*sxNStatesC + 2 > NMasked)
      .field<4, 4> ((byte)sseIdxC)
      .bit <8> (maskFlagPrevC)
      .bit <9> (epoch != SymMask[escCandidate]));
  }

  SseAccum BlendSseCascadeC_(SseCounter* sseCellC, uint sseWeightDeltaC, qword sseQTableIdxC, qword sseIdxC, int sxNStatesC, int ofallSavedE, int maskFlagPrevC, int escCandidate, PPM_CONTEXT* minCtx, int epoch, uint sseWeightCIn, uint sseFreqCacheCIn) {
    SseCounter* bigSlotC = &SseWeight2[1024 * sseQTableIdxC + BuildBigSlotIdxC_(sseIdxC, sxNStatesC,ofallSavedE, maskFlagPrevC,escCandidate, minCtx, epoch)];
    qs.q[0] = bigSlotC;
    bool predLoBeyondC = sseWeightDeltaC<0x100;
    char shiftSelC = predLoBeyondC+1;
    SseAccum blended = BlendInnerNeighbourPair_(sseCellC, 256, shiftSelC, shiftSelC, sseWeightCIn, sseFreqCacheCIn);
    uint centerExpandC = (uint)sseCellC->freq1;
    qs.wDelta[0] = RescaleAccum1_(bigSlotC, (uint)bigSlotC->sum, centerExpandC<0x200u);
    char predShiftIncC = (centerExpandC<0x400u)+predLoBeyondC+1;
    RescaleXorNeighbourPair_(sseCellC - sseIdxC, sseIdxC, 2, 8, predShiftIncC);
    return blended;
  }

  qword BuildCandidateLoopSeeIdxBase_(PPM_CONTEXT* minCtx, bool bit1Pred) {
    return SseIdx{}
      .bit <1> (bit1Pred)
      .raw (1032u * (minCtx->iSuffix == 0))
      .bits <5, 2> (SseCtx2 & 3);
  }

  uint BuildMultiStateSseCtxExtra_(PPM_CONTEXT* minCtx, int nStatesP1Save, uint nStatesPlus1) {
    return SseIdx{}
      .bit <5> (minCtx->NStates > NMasked)
      .bit <7> (nStatesP1Save < (int)nStatesPlus1 + 4)
      .bit <12>(!(nStatesPlus1 > 40))
      .bits<13, 2>(3)
      .bit <15>(nStatesPlus1 > 40);
  }

  void EnterMultiStateCandidateLoop_(PPM_CONTEXT* minCtx, int nStates, uint nStatesPlus1, int nStatesP1Save, uint cumFreqDivA, uint cumFreqSseA, uint& sumFreqLimit, int& seeIdxBase, int& remCandF, STATE**& chainPtr, uint& sumFreqF, short& orderCtxSeedSave, int& predShiftFlags, int& predBinFlags) {
    qs.wDelta[6] = 171;
    sumFreqLimit = cumFreqDivA;
    predShiftFlags = 0;
    qs.wDelta[5] = 0;
    predBinFlags = 171;
    sseIdxStorage = BuildCandidateLoopSeeIdxBase_(minCtx, OrderFall < 3);
    seeIdxBase = sseIdxStorage;
    remCandF = nStates+1;
    chainPtr = CtxChain;
    SseCtxExtra += BuildMultiStateSseCtxExtra_(minCtx, nStatesP1Save, nStatesPlus1);
    sumFreqF = cumFreqSseA;
    orderCtxSeedSave = OrderCtxSeed;
  }

  qword BuildSeeIdxF_(uint candSymbol, bool tagSymLastCtx2F, int escCandidate, int escSymbol, uint sumFreqF, uint sumFreqLimit, const byte* sse2Base, int rsContext, qword seeIdxBase, int epoch) {
    return seeIdxBase | (uint)(SseIdx{}
      .bit <0> (tagSymLastCtx2F)
      .bit <2> (epoch == MatchPosBySym[candSymbol])
      .bit <3> (candSymbol == (uint)escCandidate)
      .bit <4> (candSymbol == (uint)FoundSymbol)
      .bits <0, 8> (SSE0[candSymbol])
      .bit <8> (epoch == SymLastCtx[candSymbol])
      .bit <9> (candSymbol == (uint)hintSymRecent || *(uint*)(sse2Base+512) < (uint)(sse2Base[((word)candSymbol-(word)rsContext)&0x1FF]<<7))
      .bit <10> (candSymbol == (uint)escSymbol)
      .bit <11> (sumFreqF < sumFreqLimit)
      .bit <12> ((uint)matchEpoch2 < 0x220));
  }

  short SeedEscapeMirrorBit15_(int ofallSavedE) {
    uint orderShift15C = (ofallSavedE>1)<<15;
    short orderCtxSeedSave = orderShift15C + (OrderCtxSeed & 0x7FFF);
    OrderCtxSeed = orderShift15C + (OrderCtxSeed & 0xFFFF7FFF);
    SseSeed = ((ofallSavedE>3)<<15) + orderShift15C + (SseSeed & 0x4600);
    return orderCtxSeedSave;
  }

  void RunEscapeMirrorFreqSseWalk_(PPM_CONTEXT* minCtx, PPM_CONTEXT* suffixCtxC, int freqDeltaE, int descendNStates, int& freqSumE, STATE** chainEndE, STATE** chainEndF, uint& sumFreqCacheC) {
    if (16*freqDeltaE <= freqSumE || (minCtx->Flags & 0x40) != 0) {
      CtxChainEnd = chainEndF;
      sumFreqCacheC = minCtx->SummFreq;
      return;
    }
    FillFreqMap_(suffixCtxC);
    int walkFreqSumC = 0;
    int sumFreqC = minCtx->SummFreq;
    int sseFiveC = 5*descendNStates + 5;
    uint sseWeightCfull = (5*(2*sseFiveC < sumFreqC) + 8)*(suffixCtxC->NStates + 1) + suffixCtxC->SummFreq;
    int sumFreqW0C = (word)sumFreqC;
    do {
      STATE* st = *--chainEndE;
      FreqSseStep_(st, SymFreqs[st->Symbol], sseWeightCfull,
                   2*sseFiveC, sumFreqC, sumFreqW0C, minCtx);
      walkFreqSumC += st->Freq;
    } while (chainEndE != CtxChain);
    freqSumE = walkFreqSumC;
    sumFreqCacheC = sumFreqW0C;
    CtxChainEnd = chainEndE;
  }

  qword BuildSseIdxC_(PPM_CONTEXT* minCtx, PPM_CONTEXT* sx_p, int sxNStatesC, int descendNStates, int descendNStatesP1E, int freqDeltaE, int freqSumE, uint sumFreqCacheC, int maskFlagPrevC, byte minCtxFlagsC, int epoch) {
    return SseIdx{}
      .bits <0, 2> (SseCtx2)
      .bit <2> (descendNStatesP1E*freqSumE > sumFreqCacheC*freqDeltaE)
      .bit <3> (maskFlagPrevC | IsSxRank0SymUnseen_(sx_p, epoch))
      .bit <4> (freqDeltaE >= 2*sxNStatesC - 2*descendNStates)
      .bit <5> (sumFreqCacheC > 16*(uint)minCtx->NStates)
      .field<6, 2> (minCtxFlagsC);
  }

  struct EscapeMirrorCumFreqOut {
    int escCandidate;
    uint sumFreqDivC;
    int descendNStatesP1C;
  };
  EscapeMirrorCumFreqOut EnterEscapeMirrorAllMasked_(int descendNStatesP1Default) {
    EscapeSymbol = -1;
    qs.q[5] = &SseWeight1[SSE2_BASE];
    return { -1, 1, descendNStatesP1Default };
  }

  EscapeMirrorCumFreqOut ComputeEscapeMirrorCumFreq_(PPM_CONTEXT* minCtx, int freqDeltaE, int descendNStates, int ofallSavedE, int descendNStatesP1E, int epoch, int escSymbol, STATE** chainEndF, int& freqSumE, STATE** chainEndE) {
    auto [suffixCtxC, escCandidate] = WalkEscapeChain_(minCtx->getSuffix(this), minCtx, escSymbol);
    byte minCtxFlagsC = minCtx->Flags;
    uint sumFreqCacheC = 0;
    RunEscapeMirrorFreqSseWalk_(minCtx, suffixCtxC, freqDeltaE, descendNStates,
                                  freqSumE, chainEndE, chainEndF, sumFreqCacheC);
    PPM_CONTEXT* sx_p = minCtx->getSuffix(this);
    int sxNStatesC = sx_p->NStates;
    int descendNStatesP1C = sxNStatesC + 1;
    int maskFlagPrevC = epoch != SymMask[PrevSymbol];
    qword sseIdxC = BuildSseIdxC_(minCtx, sx_p, sxNStatesC, descendNStates,
                                     descendNStatesP1E, freqDeltaE, freqSumE,
                                     sumFreqCacheC, maskFlagPrevC, minCtxFlagsC,
                                     epoch);
    qword sseQTableIdxC = SSE0QTable[descendNStatesP1E-2];
    SseCounter* sseCellC = &EscMirrorSse[256*sseQTableIdxC + sseIdxC];
    auto [sseWeightC, sseFreqCacheC, sseWeightDeltaC] = LookupCentralCellAndRescale_(sseCellC);
    if (sseWeightDeltaC > 0x78) {
      SseAccum blended = BlendSseCascadeC_(sseCellC, sseWeightDeltaC, sseQTableIdxC, sseIdxC,
                                            sxNStatesC, ofallSavedE, maskFlagPrevC, escCandidate,
                                            minCtx, epoch, sseWeightC, sseFreqCacheC);
      sseWeightC = blended.sseWeight;
      sseFreqCacheC = blended.sseFreq;
    }
    uint sumFreqDivC = SseCumFreq_(sseFreqCacheC, sseWeightC, descendNStatesP1E);
    return { escCandidate, sumFreqDivC, descendNStatesP1C };
  }

  qword BuildSseIdxA_(int nStates, int sxNStates, int sumFreqSaveA, byte flagsSaveA, uint maskFlagPrev, int escCandidate, int epoch) {
    return SseIdx{}
      .bits <0, 2> (SseCtx2)
      .bit <2> (OrderFall < 10)
      .bit <3> (nStates + 2*sxNStates + 2 > NMasked)
      .bit <4> (4*nStates + 4 > 3*sxNStates + 3)
      .bit <5> (sumFreqSaveA > 12*nStates + 12)
      .field<6, 2> (flagsSaveA)
      .bit <8> (maskFlagPrev)
      .bit <9> (epoch != SymMask[escCandidate]);
  }

  struct CascadeCumFreqOut { uint cumFreqSseA; int escCandidate; int nStatesP1Save; };
  CascadeCumFreqOut ComputeMultiStateCumFreq_(PPM_CONTEXT* minCtx, uint nStatesPlus1, int nStates, int epoch, int escSymbol) {
    PPM_CONTEXT* sx_p = minCtx->getSuffix(this);
    int sxNStates = sx_p->NStates;
    int minSumFreqA = minCtx->SummFreq;
    int sxSumFreqA0 = sx_p->SummFreq;
    CtxChainEnd = CtxChain;
    int nStatesP1Save = sxNStates + 1;
    byte flagsSaveA = minCtx->Flags;
    int sumFreqSaveA = minSumFreqA;
    if ((sxNStates+1)*minSumFreqA <= (int)(sxSumFreqA0*nStatesPlus1)
        && minCtx->getStates(this)[0].Freq < 0x6Au
        && (flagsSaveA & 0x40) == 0) {
      sumFreqSaveA = RunFreqSseCascadeA_(minCtx, nStatesPlus1,nStatesP1Save, sxSumFreqA0,minSumFreqA, nStates);
    }
    auto [maskFlagEsc, escCandidate] = MaskAndWalkEscapeChain_(minCtx, epoch, escSymbol);
    uint maskFlagPrev = epoch != SymMask[PrevSymbol];
    qword sseQTableIdxA = SSE0QTable[nStatesPlus1-2];
    qword sseIdxA = BuildSseIdxA_(nStates, sxNStates, sumFreqSaveA, flagsSaveA,
                                     maskFlagPrev, escCandidate, epoch);
    SseCounter* sseCellA = &SseWeight2[1024*sseQTableIdxA + sseIdxA];
    auto [sseWeightA, sseFreqA, sseDeltaA] = LookupCentralCellAndRescale_(sseCellA);
    if (sseDeltaA > 0x80) {
      SseAccum blended = BlendSseCascadeA_(sseCellA, sseDeltaA, sseQTableIdxA, sseIdxA,
                                            maskFlagEsc, maskFlagPrev, sseWeightA, sseFreqA);
      sseWeightA = blended.sseWeight;
      sseFreqA = blended.sseFreq;
    }
    predRescaleDiv = minCtx->SummFreq;
    uint cumFreqSseA = predRescaleDiv + SseCumFreq_(sseFreqA, sseWeightA, nStatesPlus1);
    cumFreqAcc = cumFreqSseA;
    return { cumFreqSseA, escCandidate, nStatesP1Save };
  }

  uint BuildEscapeMirrorSseCtxExtra_(PPM_CONTEXT* minCtx, int descendNStatesP1C, int descendNStatesP1E, int ofallSavedE) {
    return SseIdx{}
      .bit <5> (minCtx->NStates + 28 > 2*NMasked)
      .bit <7> (descendNStatesP1C < descendNStatesP1E + 3)
      .bits <12, 5>((OrderFall0 > 7) + (OrderFall0 > 2) + 2 * (ofallSavedE > 2) + 7 * (descendNStatesP1E > 74) + 7 * (descendNStatesP1E > 2) + (ofallSavedE > 16));
  }

  void EnterEscapeMirrorCandidateLoop_(PPM_CONTEXT* minCtx, int descendNStates, int descendNStatesP1E, int descendNStatesP1C, int freqSumE, uint sumFreqDivC, int ofallSavedE, uint& sumFreqLimit, uint& sumFreqF, int& predBinFlags, STATE**& chainPtr, short& orderCtxSeedSave, int& seeIdxBase) {
    sumFreqLimit = (descendNStatesP1E < 24) ? 0 : (int)(sumFreqDivC + minCtx->SummFreq) >> 1;
    cumFreqSseSave = sumFreqLimit;
    sumFreqF = freqSumE + sumFreqDivC;
    cumFreqAcc = sumFreqF;
    RunLength = runLengthInit;
    predRescaleDiv = freqSumE + descendNStates + 1;
    predBinFlags = 0;
    chainPtr = CtxChain;
    qs.wDelta[6] = 0;
    CtxChainEnd = CtxChain;
    orderCtxSeedSave = SeedEscapeMirrorBit15_(ofallSavedE);
    seeIdxBase = BuildCandidateLoopSeeIdxBase_(minCtx, ofallSavedE < 3 || ofallSavedE + 23 < OrderFall0);
    sseIdxStorage = seeIdxBase;
    SseCtxExtra = BuildEscapeMirrorSseCtxExtra_(minCtx, descendNStatesP1C, descendNStatesP1E, ofallSavedE);
  }

  CascadeCumFreqOut EnterCascadeAllSyms_(PPM_CONTEXT* minCtx, int nStatesP1Default) {
    predRescaleDiv = minCtx->SummFreq;
    qs.q[5] = &SseWeight1[SSE2_BASE];
    EscapeSymbol = -1;
    uint cumFreqSseA = predRescaleDiv + 1;
    cumFreqAcc = cumFreqSseA;
    return { cumFreqSseA, -1, nStatesP1Default };
  }

  void ResetQClusterToSse2Sink_() {
    qs.q[0] = qs.q[1] = qs.q[2] = qs.q[3] = qs.q[4] = &SseWeight1[SSE2_BASE];
  }

  void EnterCascadeRSContext_(int epoch) {
    RSContext = CtxChain[0]->Symbol;
    SymLastCtx[Order1LastSymCell_(RSContext, Order1Ctx)] = epoch;
    ResetQClusterToSse2Sink_();
  }

  void RunBinaryCheapPath_(int cumFreqB, int cumWeightB) {
    sseTot = cumWeightB;
    sseCum = cumFreqB;
    sp.BinSseHiG = sp.BinSseLoG = &SseWeight1[SSE2_BASE];
    sp.PredBaseBG = sp.PredBaseAG = &SseWeight1[SSE2_BASE];
    qs.q[6] = qs.q[5] = qs.q[0] = qs.q[1] = qs.q[2] = qs.q[3] = &SseWeight1[SSE2_BASE];
    sp.BinSseCellG = &predWeightSink;
  }

  int BuildBinSseSlotIdxB_(qword sseIdxB, int sparseFlags, uint seeIndex) {
    return (int)(SseIdx{}
      .bits <0, 2> ((uint)sseIdxB >> 6)
      .bit <2> (SymLastCtx2[SseCtx3] == SymCount)
      .bit <3> ((uint)sseIdxB & 0x200)
      .bits <4, 2> (sparseFlags)
      .bit <6> (RunLength > -9)
      .bits <7, 8> ((byte)SSE1QTable[seeIndex]));
  }

  struct BinSseDeepOut { int cumFreq, cumWeight; };
  BinSseDeepOut RunBinSseDeep_(qword sseIdxB, int sparseFlags, uint seeIndex, uint centerExpandB, int cumFreqIn, int cumWeightIn) {
    SseCounter* binSseCenter = sp.BinSseCenterG;
    int* binSseSlotB = &BinSse[BuildBinSseSlotIdxB_(sseIdxB, sparseFlags, seeIndex)];
    uint binSseVal = *binSseSlotB;
    sp.BinSseCellG = binSseSlotB;
    int sseSseMeanB = (binSseVal>>(centerExpandB<0x230))+cumFreqIn;
    int sseSseFreqB = (0x3100u>>(centerExpandB<0x230))+cumWeightIn;
    sseCum = sseSseMeanB;
    sseTot = sseSseFreqB;
    JREG(binSseSlotB, sizeof(int));
    *binSseSlotB = binSseVal-((binSseVal+2)>>3);
    char sseShiftB = centerExpandB<0x220;
    predBaseDeltaA = RescaleAccum2_(sp.PredBaseAG, sseShiftB);
    predBaseDeltaB = RescaleAccum2_(sp.PredBaseBG, sseShiftB);
    SseCounter* binUpCell = binSseCenter + 0x4000;
    SseCounter* binDnCell = binSseCenter - 0x4000;
    sp.BinSseLoG = binDnCell;
    sp.BinSseHiG = binUpCell;
    int cumFreq = ((binUpCell->hits + binDnCell->hits) >> (sseShiftB+1)) + sseSseMeanB;
    int cumWeight = ((uint)(binUpCell->freq0 + binDnCell->freq0) >> (sseShiftB+1)) + sseSseFreqB;
    sseCum = cumFreq;
    sseTot = cumWeight;
    binSseDeltaHi = RescaleAccum2_(binUpCell, sseShiftB);
    binSseDeltaLo = RescaleAccum2_(binDnCell, sseShiftB);
    char sseShiftC = (binSseCenter->freq1<0x398u) + sseShiftB;
    RescaleAllQSlots_(sseShiftC);
    return { cumFreq, cumWeight };
  }

  void WalkEscapeMirrorStates_(PPM_CONTEXT* minCtx, byte descendFlags, int epoch, STATE**& chainEndE, STATE**& chainEndF, int& remStatesE, int& freqSumE) {
    STATE* walkStateIterE = minCtx->getStates(this) - 1;
    STATE** sortRangeE = chainEndE;
    STATE** sortLimitC;
    int sortPriorityC;
    int walkFreqSumE = freqSumE;
    int walkSymE;
    while (1) {
      do {
        walkStateIterE += 1;
        walkSymE = walkStateIterE->Symbol;
      } while (epoch == SymMask[walkSymE]);
      SymMask[walkSymE] = epoch;
      *chainEndE++ = walkStateIterE;
      chainEndF = chainEndE;
      walkFreqSumE += walkStateIterE->Freq;
      bool sortHit = true;
      if (walkSymE == FoundSymbol) {
        sortLimitC = &CtxChain[(byte)descendFlags & ((foundSymHist & 7) == 0)];
        sortPriorityC = 22;
      } else if (epoch == MatchPosBySym[walkSymE] || epoch == SymLastCtx2[walkSymE]) {
        sortLimitC = sortRangeE;
        sortPriorityC = (epoch == MatchPosBySym[walkSymE]) ? 16 : 12;
      } else {
        sortHit = false;
      }
      if (sortHit) {
        BubbleSortChain_(chainEndE, sortLimitC, sortPriorityC);
        ++sortRangeE;
      }
      if (!--remStatesE) {
        freqSumE = walkFreqSumE;
        break;
      }
    }
  }

  bool DescendEscapeChain_(PPM_CONTEXT*& minCtx, int entryNStates, int& walkNStates, int& walkDelta, sqword& result) {
    do {
      if (!minCtx->iSuffix) return true;
      --OrderFall;
      minCtx = minCtx->getSuffix(this);
      walkNStates = minCtx->NStates;
      MaxContext = minCtx;
      if (walkNStates) {
        result = minCtx->Flags & 0xF;
        if (!minCtx->mruState(this).Freq) {
          BinEscFreq(minCtx);
          result = 2 * (minCtx->Flags & 0xF);
        }
      }
      walkDelta = walkNStates - entryNStates;
    } while (!walkDelta);
    return false;
  }

  template<int f_DEC>
  sqword BeginEscapeFromBinary_(uint subRange, int cumFreq, int totFreq, PPM_CONTEXT* minCtx) {
    int oneStateFreq = minCtx->oneState().Freq;
    if (f_DEC) rc.DecodeNotMatched(subRange); else rc.encodeEscape(subRange);
    SymMask[minCtx->oneState().Symbol] = SymCount;
    FoundState = nullptr;
    ClearSparseBitmapHit_();
    SseCtx = 0;
    int oneStateFreqP1 = oneStateFreq + 1;
    sqword result = (int)(16 * (oneStateFreqP1*cumFreq - oneStateFreq*totFreq))
                  / (int)(totFreq * oneStateFreqP1);
    EscIndexSeed = result;
    return result;
  }

  uint OrderCtxSeedBuild_(int sym, int matchCtxHi, int sparseFlag, int carriedFrom, uint freq, uint hits) {
    return SseIdx{}
      .bit <0> (sym == hint.hintSymB31)
      .bit <1> (sym == hint.hintSymB29)
      .bit <2> (sym == hint.hintSymB33)
      .bit <3> (sym == b31Key)
      .bit <4> (sym == Order1Ctx)
      .field<5, 3> (sym)
      .bit <8> (sym == matchHintByte)
      .field<9, 1> (carriedFrom)
      .field<10,1> ((word)recentSym - (word)sym)
      .field<11,1> ((word)sym - (word)matchCtxHi)
      .bit <12> (sparseFlag)
      .field<13,3> (carriedFrom)
      .bits <16,2> ((freq < (uint)(56*hits)) + (freq < (uint)( 6*hits)))
      .val;
  }

  uint Sse3IdxBuild_(int sym, bool tagSymLast2, uint heapIdx, uint sseIdx, uint sse2Counter, uint histByte) {
    return SseIdx{}
      .bit <0> (tagSymLast2)
      .bit <1> (sym == (byte)SseState3[sseState3Hash])
      .bit <2> ((uint)((byte*)Indx2Ptr(heapIdx) - pText + 736) < 736)
      .bits <3, 2> ((sse2Counter < 384*histByte) + (sse2Counter < 58*histByte) + (sse2Counter < 22*histByte))
      .field<8, 3> (sseIdx)
      .bit <11> ((uint)matchPosAge < 0x2C00)
      .raw (SseCtxExtra)
      .val;
  }

  int SseMatchIdxBuild_(int sym) {
    return (int)SseIdx{}
      .bits <1, 8> (sym)
      .bits <9, 8> (recentSym)
      .bit <17> (SseCtx)
      .bit <18> ((uint)matchPosAge < 0x78)
      .bit <19> (sym == FoundSymbol)
      .bit <20> (SseScale < (uint)sseScaleCntr)
      .val;
  }

  uint Sse2IdxBuild_(int sym, uint prevWeight, uint prevTot) {
    return SseIdx{}
      .bit <0> (sym == hint.hintSymM2)
      .bit <1> (sym == hint.hintSymMatch3)
      .bit <2> (sym == hint.hintSymBmCell)
      .bit <3> (sym == FoundSymbol)
      .bit <4> (sym == PrevSymbol)
      .bits <5, 2> (((uint)matchPosAge < 0xA800) + ((uint)matchPosAge < 0x600) + ((uint)matchPosAge < 0xD))
      .bit <7> ((uint)matchEpoch2 < 0x29)
      .bits <1, 8> (SSE0[sym])
      .field<11,1> (OrderCtxSeed)
      .field<12,1> ((word)matchHashSy - (word)sym)
      .bit <13> (17*prevWeight < prevTot)
      .raw (SseSeed)
      .val;
  }

  Sse1Out RunSse1_(uint hits) {
    int* slot = &Sse1[2*OrderCtxSeed];
    sp.Sse1SlotG = slot;
    return Sse1Step_(slot, hits);
  }

  int BinSlotRescaleCommit_(void* slot, int& freq) {
    JREG(slot, sizeof(SseCounter));
    MaybeRescale2_(slot, freq);
    SseSlot* s = (SseSlot*)slot;
    int hits = s->hits;
    sseCum = hits;
    s->freq0 = freq + s->freq1;
    sseTot = freq;
    dlt.sse2DenDelta = hits;
    dlt.sse2NumDelta = freq;
    return hits;
  }

  struct CandidateContext {
    STATE* localFoundState;
    PPM_CONTEXT* preCommitMinCtx;
    uint candSymbol;
    qword candProbBF;
    int sseEntryC2F;
    qword sseSseSizeF;
    int sparseHitsF;
    bool tagSymLastCtx2F;
    short matchCtxHiSave;
    qword seeIdxF;
    SseCounter* binSseSlotF;
    int freq0F;
    int hitsF;
  };
  CandidateContext SetupPerCandidate_(STATE** chainPtr, PPM_CONTEXT* minCtx, int sumFreqF, int predBinFlags, int predShiftFlags, int escSymbol, int escCandidate, int epoch, const byte* sse2Base, qword seeIdxBase, uint sumFreqLimit) {
    CandidateContext c;
    c.localFoundState = *chainPtr;
    c.preCommitMinCtx = minCtx;
    CtxChainEnd = chainPtr+1;
    sp.BinSseHiG = sp.BinSseLoG = &SseWeight1[SSE2_BASE];
    sp.PredBaseBG = sp.PredBaseAG = &SseWeight1[SSE2_BASE];
    PredWeightBG = predWeightSink2;
    PredWeightAG = predWeightSink2;
    c.candSymbol = c.localFoundState->Symbol;
    c.candProbBF = ((c.localFoundState->Freq<<8) - predBinFlags) / sumFreqF;
    c.sseEntryC2F = c.candProbBF;
    c.sseSseSizeF = (uint)(byte)SymType[c.candProbBF] + 1;
    SeedCandidateSparseHash_(c.candSymbol, predShiftFlags);
    c.tagSymLastCtx2F = epoch == SymLastCtx2[c.candSymbol];
    c.sparseHitsF = SparseBitFlags_();
    c.matchCtxHiSave = MatchCtxHi;
    int matchTblHitF = MatchPosTable[256*MatchCtxHi + c.candSymbol];
    ComputeMatchHints_(matchTblHitF, c.candSymbol);
    c.seeIdxF = BuildSeeIdxF_(c.candSymbol, c.tagSymLastCtx2F, escCandidate, escSymbol, sumFreqF, sumFreqLimit, sse2Base, RSContext, seeIdxBase, epoch);
    qword binSseOffF = 0x8000*c.sseSseSizeF + 4*c.seeIdxF;
    SseCounter* sse2base = &SseWeight1[SSE2_BASE];
    SseCounter* binCell = sse2base + (binSseOffF >> 2);
    c.binSseSlotF = binCell;
    c.freq0F = binCell->freq0;
    sp.BinSseCenterG = binCell;
    if (c.freq0F > 0x8000) c.matchCtxHiSave = MatchCtxHi;
    c.hitsF = BinSlotRescaleCommit_(c.binSseSlotF, c.freq0F);
    return c;
  }

  void RunMatchSse2Sse3_(int sym, int cumFreq, int cumWeight, bool tagSymLastCtx2, uint heapIdx) {
    int* matchSlot = &SseMatch[SseMatchIdxBuild_(sym)];
    sp.SseMatchSlotG = matchSlot;
    SseMatchStep_(matchSlot, (int)(60416LL*cumFreq/cumWeight));
    int sse2In = sseCum;
    qword sse2Idx = Sse2IdxBuild_(sym, sse2In, (uint)sseTot);
    int* slotSse2 = &Sse2[2*sse2Idx];
    sp.Sse2SlotG = slotSse2;
    Sse2Step_(slotSse2);
    int sse2CumTot = sseTot;
    dlt.predWeightDelta = sse2CumTot;
    int sse2HistByte = Sse2BaseG[((word)sym-(word)RSContext)&0x1FF];
    uint sse2Counter = *(uint*)(Sse2BaseG+512);
    int* slotSse3 = &Sse3[2 * (int)Sse3IdxBuild_(sym, tagSymLastCtx2, heapIdx,sse2Idx, sse2Counter,sse2HistByte)];
    sp.Sse3SlotG = slotSse3;
    Sse3Step_(slotSse3, sse2In, sse2CumTot);
  }

  void RunPerCandidateSseCascade_(const CandidateContext& c, SseCounter* binSseCenter, short orderCtxSeedSave) {
    OrderCtxSeed = OrderCtxSeedBuild_(c.candSymbol, c.matchCtxHiSave, c.sparseHitsF, orderCtxSeedSave, c.freq0F, c.hitsF);
    auto [sseCumWeightF, sseCumFreqF] = RunSse1_(c.hitsF);
    uint centerExpandF = binSseCenter->freq1;
    if (centerExpandF <= 8) {
      sseTot = sseCumWeightF;
      sseCum = sseCumFreqF;
    } else {
      auto pw = RunPredWeightStageF_(centerExpandF, c.sseEntryC2F, c.candProbBF,c.sparseHitsF, c.seeIdxF, binSseCenter,sseCumFreqF, sseCumWeightF);
      sseCumFreqF = pw.cumFreq;
      sseCumWeightF = pw.cumWeight;
    }
    RunMatchSse2Sse3_(c.candSymbol, sseCumFreqF, sseCumWeightF,c.tagSymLastCtx2F, c.localFoundState->iSuccessor);
  }

  template<int f_DEC>
  qword PrepareNextStep_(PPM_CONTEXT* MinContext, int& epoch) {
    if( f_DEC ) return 0;              // W2: harvest discards this; skip the work
    rc.commitRange();
    qword result = SseUpdate(MinContext);
    rc.EncodeNormalize();
    epoch = SymCount;
    return result;
  }

  template<int f_DEC>
  bool ProcessOneCandidate_(int inputByte, STATE**& chainPtr, PPM_CONTEXT*& MinContext, STATE*& localFoundState, uint sumFreqF, int predBinFlags, int predShiftFlags, int escSymbol, int escCandidate, int epoch, qword seeIdxBase, uint sumFreqLimit, short orderCtxSeedSave) {
    CandidateContext c = SetupPerCandidate_(chainPtr, MinContext, sumFreqF,
                                              predBinFlags, predShiftFlags,
                                              escSymbol, escCandidate, epoch,
                                              Sse2BaseG, seeIdxBase, sumFreqLimit);
    RunPerCandidateSseCascade_(c, sp.BinSseCenterG, orderCtxSeedSave);
    localFoundState = c.localFoundState;
    int cumFreqC = sseCum;
    uint totFreqC = sseTot;
    if (ppmd3_harvest && SQ_ptr+2<=4096 && totFreqC>0 && cumFreqC>0 && (uint)cumFreqC<totFreqC) {
      SQ[SQ_ptr++].store(c.candSymbol, (uint)cumFreqC, totFreqC);
      SQ[SQ_ptr++].store(256, totFreqC-(uint)cumFreqC, totFreqC);
    }
    uint subRangeC = rc.getSubRange(cumFreqC, totFreqC);
    bool matched;
    if (f_DEC ? !rc.IsDecodeMatched(subRangeC) : (inputByte != (int)c.candSymbol)) {
      HandleEscapedCandidate_<f_DEC>(subRangeC, localFoundState, MinContext);
      matched = false;
    } else {
      matched = CommitMatchedCandidate_<f_DEC>(subRangeC, localFoundState,
                                            c.preCommitMinCtx, MinContext,
                                            cumFreqC, totFreqC);
    }
    MinContext = MaxContext;
    return matched;
  }

  void AdvanceToNextCandidate_(STATE**& chainPtr, uint& sumFreqF, int& predBinFlags, int& predShiftFlags, int& escSymbol, int& escCandidate, int& epoch, int& seeIdxBase, uint& sumFreqLimit, short& orderCtxSeedSave) {
    predShiftFlags = qs.wDelta[5];
    predBinFlags = qs.wDelta[6];
    escSymbol = SseCtx3;
    epoch = SymCount;
    escCandidate = EscapeSymbol;
    chainPtr = CtxChainEnd;
    orderCtxSeedSave = OrderCtxSeed;
    sumFreqF = cumFreqAcc;
    sumFreqLimit = cumFreqSseSave;
    seeIdxBase = sseIdxStorage;
  }

  int RewindAndPrepEscape_(PPM_CONTEXT* MinContext) {
    uint rewindMult = predRescaleDiv / (MinContext->NStates + 1);
    RewindAllPredictors_(rewindMult);
    return MinContext->NStates;
  }

  void ScanAllCandidateStatesA_(STATE* stateIter, int remStates, int escSymbol, int epoch, STATE** chainEnd, STATE** chainStart) {
    do {
      stateIter += 1;
      ScanCandidateStateA_(stateIter, escSymbol, epoch, chainEnd, chainStart);
    } while (--remStates);
  }

  void EnterMultiStateCascade_(PPM_CONTEXT* MinContext, int nStates, uint nStatesPlus1, int epoch, int escSymbol, int& escCandidate, uint& sumFreqLimit, int& seeIdxBase, int& remCandF, STATE**& chainPtr, uint& sumFreqF, short& orderCtxSeedSave, int& predShiftFlags, int& predBinFlags) {
    EnterCascadeRSContext_(epoch);
    CascadeCumFreqOut cf = (nStatesPlus1 == 256)
                             ? EnterCascadeAllSyms_(MinContext, nStates + 1)
                             : ComputeMultiStateCumFreq_(MinContext, nStatesPlus1, nStates,
                                                          epoch, escSymbol);
    escCandidate = cf.escCandidate;
    uint cumFreqDivA = (nStatesPlus1 < 24 && nStatesPlus1 != 256) ? 0 : (cf.cumFreqSseA >> 1);
    cumFreqSseSave = cumFreqDivA;
    EnterMultiStateCandidateLoop_(MinContext, nStates, nStatesPlus1,
                                   cf.nStatesP1Save, cumFreqDivA, cf.cumFreqSseA,
                                   sumFreqLimit, seeIdxBase, remCandF,
                                   chainPtr, sumFreqF, orderCtxSeedSave,
                                   predShiftFlags, predBinFlags);
  }

  void RunMultiStateBranch_(PPM_CONTEXT* MinContext, int epoch, uint& sumFreqF, uint& sumFreqLimit, int& escSymbol, int& escCandidate, int& seeIdxBase, int& remCandF, STATE**& chainPtr, short& orderCtxSeedSave, int& predShiftFlags, int& predBinFlags) {
    LES1Entry les1 = EnterMultiStateBranch_(MinContext, epoch);
    escSymbol = les1.escSymbol;
    ScanAllCandidateStatesA_(les1.stateIter, les1.remStates, escSymbol, epoch,
                              &CtxChain[1], &CtxChain[1]);
    EnterMultiStateCascade_(MinContext, les1.nStates, les1.nStatesPlus1, epoch,
                             escSymbol, escCandidate, sumFreqLimit,
                             seeIdxBase, remCandF, chainPtr, sumFreqF,
                             orderCtxSeedSave, predShiftFlags, predBinFlags);
  }

  void RunBinaryCascade_(PPM_CONTEXT* MinContext, int epoch, int& cumFreq, uint& totFreq) {
    int currentSymbol = MinContext->oneState().Symbol;
    SseCtx3 = currentSymbol;
    PPMContextWalkOut walk = PPMContextWalk(epoch, currentSymbol);
    BinarySseSlot bslot = SetupBinarySseSlotB_(walk.sseCtx, walk.suffixNStates, walk.seeIndex);
    int sseFreqB = bslot.freq0;
    int sseHitsB = BinSlotRescaleCommit_(bslot.slot, sseFreqB);
    OrderCtxSeed = OrderCtxSeedBuild_(currentSymbol, (int)MatchCtxHi,
                                      walk.sparseFlags, OrderCtxSeed,
                                      (uint)sseFreqB, (uint)sseHitsB);
    auto [cumWeightB, cumFreqB] = RunSse1_(sseHitsB);
    uint centerExpandB = sp.BinSseCenterG->freq1;
    int escSymB = SseCtx3;
    if (centerExpandB <= 0x20) {
      RunBinaryCheapPath_(cumFreqB, cumWeightB);
    } else {
      auto deep = RunBinSseDeep_(bslot.sseIdxB, walk.sparseFlags, walk.seeIndex,
                                  centerExpandB, cumFreqB, cumWeightB);
      cumFreqB = deep.cumFreq;
      cumWeightB = deep.cumWeight;
    }
    RunMatchSse2Sse3_(escSymB, cumFreqB, cumWeightB,
                      SymLastCtx2[escSymB] == SymCount,
                      walk.iStates);
    cumFreq = sseCum;
    totFreq = sseTot;
  }

  struct EscapeDescentResult {
    int descendNStates;
    int ofallSavedE;
    int freqDeltaE;
    int remCandF;
    int descendNStatesP1C;
    int descendNStatesP1E;
    int freqSumE;
    STATE** chainEndE;
    STATE** chainEndF;
  };
  bool SetupEscapeDescent_(PPM_CONTEXT*& MinContext, int entryNStates, sqword& result, int& predShiftFlags, int& epoch, int& escSymbol, EscapeDescentResult& d) {
    int walkNStates, walkDelta;
    if (DescendEscapeChain_(MinContext, entryNStates, walkNStates, walkDelta, result))
      return true;
    int dnsp1 = walkNStates + 1;
    d.descendNStates = walkNStates;
    d.ofallSavedE = OrderFall;
    d.freqDeltaE = walkNStates - entryNStates;
    d.remCandF = walkDelta;
    d.descendNStatesP1C = dnsp1;
    d.descendNStatesP1E = dnsp1;
    d.freqSumE = 0;
    d.chainEndE = CtxChain;
    d.chainEndF = CtxChain;
    int remStatesE = d.freqDeltaE;
    char descendFlags = 0;
    predShiftFlags = 0x8000;
    epoch = SymCount;
    SeedEscapeCandidateChain_(MinContext, d.freqDeltaE, descendFlags,
                               d.chainEndE, d.chainEndF, d.freqSumE,
                               predShiftFlags, remStatesE, escSymbol);
    if (remStatesE) {
      WalkEscapeMirrorStates_(MinContext, descendFlags, epoch, d.chainEndE, d.chainEndF, remStatesE, d.freqSumE);
    }
    return false;
  }

  bool RunEscapePath_(PPM_CONTEXT*& MinContext, int entryNStates, int& predShiftFlags, int& epoch, int& escSymbol, int& escCandidate, int& predBinFlags, uint& sumFreqF, uint& sumFreqLimit, int& seeIdxBase, STATE**& chainPtr, short& orderCtxSeedSave, int& remCandF, sqword& result) {
    EscapeDescentResult d;
    if (SetupEscapeDescent_(MinContext, entryNStates, result,predShiftFlags, epoch, escSymbol, d)) return true;
    remCandF = d.remCandF;
    RunEscapeMirrorCascade_(MinContext, d, epoch, escSymbol, escCandidate, sumFreqF, sumFreqLimit, seeIdxBase, chainPtr, orderCtxSeedSave, predBinFlags);
    return false;
  }

  void RunEscapeMirrorCascade_(PPM_CONTEXT* MinContext, EscapeDescentResult d, int epoch, int escSymbol, int& escCandidate, uint& sumFreqF, uint& sumFreqLimit, int& seeIdxBase, STATE**& chainPtr, short& orderCtxSeedSave, int& predBinFlags) {
    ResetQClusterToSse2Sink_();
    EscapeMirrorCumFreqOut mf = (d.descendNStates == 255)
      ? EnterEscapeMirrorAllMasked_(d.descendNStatesP1C)
      : ComputeEscapeMirrorCumFreq_(MinContext, d.freqDeltaE, d.descendNStates,
                                      d.ofallSavedE, d.descendNStatesP1E, epoch,
                                      escSymbol, d.chainEndF,
                                      d.freqSumE, d.chainEndE);
    escCandidate = mf.escCandidate;
    uint sumFreqDivC = mf.sumFreqDivC;
    d.descendNStatesP1C = mf.descendNStatesP1C;
    EnterEscapeMirrorCandidateLoop_(MinContext, d.descendNStates,
                                     d.descendNStatesP1E, d.descendNStatesP1C,
                                     d.freqSumE, sumFreqDivC, d.ofallSavedE,
                                     sumFreqLimit, sumFreqF, predBinFlags,
                                     chainPtr, orderCtxSeedSave, seeIdxBase);
  }

  template<int f_DEC>
  int ProcessByte(int sym) {
    int entryNStates, predShiftFlags, predBinFlags;
    int cumFreq;
    int remCandF, escSymbol, escCandidate, seeIdxBase;
    short orderCtxSeedSave;
    uint sumFreqF, totFreq, subRange, sumFreqLimit;
    sqword result;
    STATE **chainPtr, *localFoundState;
    PPM_CONTEXT* MinContext = MaxContext;
    int epoch = SymCount;
    if( MinContext->NStates ) {
      RunMultiStateBranch_(MinContext, epoch, sumFreqF, sumFreqLimit,
                            escSymbol, escCandidate, seeIdxBase, remCandF,
                            chainPtr, orderCtxSeedSave,
                            predShiftFlags, predBinFlags);
      goto ENCODE_CANDIDATE;
    }
    RunBinaryCascade_(MinContext, epoch, cumFreq, totFreq);
    if( ppmd3_harvest && SQ_ptr+2<=4096 && totFreq>0 && cumFreq>0 && (uint)cumFreq<totFreq ) {
      SQ[SQ_ptr++].store(MinContext->oneState().Symbol, (uint)cumFreq, totFreq);
      SQ[SQ_ptr++].store(256, totFreq-(uint)cumFreq, totFreq);
    }
    subRange = rc.getSubRange(cumFreq, totFreq);
    if( f_DEC ? !rc.IsDecodeMatched(subRange) : (sym != MinContext->oneState().Symbol) ) {
      result = BeginEscapeFromBinary_<f_DEC>(subRange, cumFreq, totFreq, MinContext);
      MinContext = MaxContext;
      entryNStates = MinContext->NStates;
  ESCAPE_DESCEND:
      if( RunEscapePath_(MinContext, entryNStates, predShiftFlags, epoch, escSymbol, escCandidate, predBinFlags, sumFreqF, sumFreqLimit, seeIdxBase, chainPtr, orderCtxSeedSave, remCandF, result) )
        return -1;
      while( 1 ) {
        if( f_DEC ) rc.DecodeNormalize(); else rc.EncodeNormalize();
  ENCODE_CANDIDATE:
        if( ProcessOneCandidate_<f_DEC>( sym, chainPtr, MinContext, localFoundState, sumFreqF, predBinFlags, predShiftFlags, escSymbol, escCandidate, epoch, seeIdxBase, sumFreqLimit, orderCtxSeedSave) )
          goto SYMBOL_FOUND;
        if( !--remCandF ) {
          entryNStates = RewindAndPrepEscape_(MinContext);
          goto ESCAPE_DESCEND;
        }
        AdvanceToNextCandidate_(chainPtr, sumFreqF, predBinFlags, predShiftFlags, escSymbol, escCandidate, epoch, seeIdxBase, sumFreqLimit, orderCtxSeedSave);
      }
    }
    localFoundState = CommitMatchedBinary_<f_DEC>(subRange, cumFreq, totFreq, MinContext);
    MinContext = MaxContext;
  SYMBOL_FOUND:
    if (f_DEC) sym = localFoundState->Symbol;
    PrepareNextStep_<f_DEC>(MinContext, epoch);
    return sym;
  }

  void ResetEpochStateBetweenChunks_() {
    memset(SymMask, 0, sizeof(SymMask));
    memset(SymLastCtx, 0, sizeof(SymLastCtx));
    memset(SymLastCtx2, 0, sizeof(SymLastCtx2));
    memset(MatchPosBySym, 0, sizeof(MatchPosBySym));
  }

  // ===================== ppmd3 harvest API =====================
  struct qsym {
    uint sym, freq, total;
    void store(uint s,uint f,uint t){ sym=s; freq=f; total=t; }
  };
  qsym SQ[4096];
  uint SQ_ptr;
  uint sqp[256];

  // chained-multiplication of unary-bit probabilities -> per-symbol weights.
  // (ppmd.hpp ConvertSQ, trF/trT dropped: coder0 ignores them.)
  void ConvertSQ( void ) {
    uint i,c,freq,total,prob;
    uint cum = 0xFFFFFF00;
    memset(sqp, 0, sizeof(sqp));
    for( i=0; i<SQ_ptr; i++ ) {
      c=SQ[i].sym; freq=SQ[i].freq; total=SQ[i].total;
      if( total==0 ) continue;
      prob = (uint)( qword(qword(cum)*freq) / total );
      if( c<256 ) sqp[c]=prob+1; else cum=prob;
    }
  }

  // idempotent 64K-chunk boundary: refill SymCount + reset epoch arrays.
  void MaybeChunkBoundary_( void ) {
    if( SymCount<=0 ) { SymCount=0x10000; ResetEpochStateBetweenChunks_(); }
  }

  void Init( uint MaxOrder, uint MMAX, uint CutOff, uint /*filesize*/ ) {
    InitTables();
    StartSubAllocator(MMAX, MaxOrder, CutOff);
    StartModelRare(0);                 // sets SymCount=1 (first chunk = 1 symbol)
    SQ_ptr = 0;
  }

  // advance the model by the known byte c, no coding (FakeRangecoder no-ops).
  void ppmd_UpdateByte( uint c ) {
    MaybeChunkBoundary_();
    ProcessByte<0>( (int)(c & 0xFF) );
  }

  // small/scratch members the always-escape pass mutates: blanket save/restore
  // (cheap, fixed footprint). Big sparse arrays + arena are handled by the journal.
  #define PPMD3_BLANKET(F) \
    F(SymMask) F(SymLastCtx) F(SymLastCtx2) F(SymFreqs) F(MatchPosBySym) F(LastEpochBySym) \
    F(CtxChain) F(CtxChainEnd) F(MaxContext) F(RootContext) F(RunLength) F(OrderFall) F(OrderFall0) \
    F(EscapeSymbol) F(OrderCtxSeed) F(sseTot) F(sseCum) F(FoundState) F(qs) F(sp) F(hint) F(dlt) \
    F(RSContext) F(SparseBit) F(SparseHashA) F(SparseIdxA) F(SparseHashB) F(SparseIdxB) \
    F(predDeltaNum) F(predDeltaDen) F(predRescaleDiv) F(cumFreqAcc) F(cumFreqSseSave) F(sseIdxStorage) \
    F(PredWeightAG) F(PredWeightBG) F(NMasked) F(PrevSymbol) F(SymEpoch) F(MatchCtxHi) \
    F(SseScale) F(SseCtx) F(SseCtx2) F(SseCtx3) F(SseCtxExtra) F(EscIndexSeed) F(SseSeed) \
    F(orderBumpVariance) F(predWeightSink) F(predBaseDeltaA) F(predBaseDeltaB) \
    F(binSseDeltaHi) F(binSseDeltaLo) F(recentSym) F(sseScaleCntr) F(symHalfHistory) \
    F(matchHintByte) F(bdiffSaved) F(bdiffStickyCnt) F(matchHashSy) F(matchPosAge) F(matchEpoch2) F(sseState3Hash)

  void ppmd3_blanket_save_( byte* b ){ uint o=0;
  #define F(m) { memcpy(b+o,&m,sizeof(m)); o+=sizeof(m); }
    PPMD3_BLANKET(F)
  #undef F
  }
  void ppmd3_blanket_restore_( const byte* b ){ uint o=0;
  #define F(m) { memcpy(&m,b+o,sizeof(m)); o+=sizeof(m); }
    PPMD3_BLANKET(F)
  #undef F
  }

  // read-only harvest of next-byte distribution into sqp[].
  void ppmd_PrepareByte( void ) {
    MaybeChunkBoundary_();
    SQ_ptr = 0;
    static byte blanket[16384];
    ppmd3_blanket_save_(blanket);      // small scratch via blanket
    ppmd3_harvest = 1;
    ProcessByte<1>( 0 );               // always-escape: walks full chain, records SQ
    ppmd3_harvest = 0;
    ppmd3_jundo();                     // big sparse arrays + arena via journal
    ppmd3_blanket_restore_(blanket);
    ConvertSQ();
  }
};

typedef Model ppmd_Model;

} // namespace

// ppmd.h forward-declares `struct PPMD::ppmd_Model;` and holds a
// std::unique_ptr<PPMD::ppmd_Model>. A `using DUMMY::ppmd_Model;` cannot
// complete that forward declaration -- it names a different entity and the
// two stay distinct types (PPMD::ppmd_Model vs PPMD::DUMMY::ppmd_Model).
// Instead, define PPMD::ppmd_Model as a thin wrapper that publicly inherits
// the model, which both completes the forward-declared class and exposes
// Init / ppmd_UpdateByte / ppmd_PrepareByte / sqp / HeapStart.
struct ppmd_Model : DUMMY::Model {};
using DUMMY::byte;   // for the (byte*) cast in ppmd.cpp's ByteUpdate

