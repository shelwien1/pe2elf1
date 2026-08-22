// bmf_util.inc -- exit codes, small integer helpers, the context-index builder and the block pool
// Included from bmf.cpp; not a standalone translation unit.

enum BmfExit {
  bmf_no_file = 1,
  bmf_no_option = 2,
  bmf_bad_file = 3,
  bmf_read_error = 4,
  bmf_write_error = 5,
  bmf_no_open = 6,
  bmf_no_memory = 7,
  bmf_user_break = 8,
};

NOINLINE
void bmf_fatal(int32_t Code, ...) {
  va_list ap;
  static const char*const message[] = {
    "", "File not found: %s", "Unknown option: %s", "%s: bad file!", "Read error!", "Write error for file %s", "Can't open file: %s", "Out of memory!", "User break!",
  };
  va_start(ap, Code);
  vprintf(message[(uint32_t)Code<9 ? Code : bmf_read_error], ap);
  va_end(ap);
  printf("\n");
  exit(Code);
}

// Cache prefetch.  A hint and nothing more: it never faults (an address the
// process does not own is simply dropped), never changes a computed value, and
// can always be deleted without altering the coded stream.  The hint chooses
// which cache levels the line is pulled into:
//
//   pf_all   PREFETCHT0   -- L1+L2+L3; the line is about to be read
//   pf_l2    PREFETCHT1   -- L2+L3 only, leaving L1 for hotter data
//   pf_l3    PREFETCHT2   -- L3 only (Intel treats T2 much like T1)
//   pf_once  PREFETCHNTA  -- streamed data; minimise cache pollution
//
// There is no write prefetch here: PREFETCHW needs the PRFCHW feature, which
// this build's -march=haswell target does not include, so _m_prefetchw would
// silently compile down to PREFETCHT0 anyway.
// (`auto`, because the hint is a plain int in clang's <xmmintrin.h> and an
// `enum _mm_hint` in gcc's.)
constexpr auto pf_all = _MM_HINT_T0;
constexpr auto pf_l2 = _MM_HINT_T1;
constexpr auto pf_l3 = _MM_HINT_T2;
constexpr auto pf_once = _MM_HINT_NTA;

template <decltype(pf_all) hint = pf_all> INLINE void bmf_prefetch(const void* at) {
  _mm_prefetch((const char*)at, hint);
}

inline uint32_t abs32(int32_t x) {
  return x<0 ? 0u-(uint32_t)x : (uint32_t)x;
}

int32_t min32(int32_t a, int32_t b) {
  return a<b ? a : b;
}

int32_t max32(int32_t a, int32_t b) {
  return a>b ? a : b;
}

uint32_t umin32(uint32_t a, uint32_t b) {
  return a<b ? a : b;
}

int32_t clamp32(int32_t v, int32_t lo, int32_t hi) {
  return v<lo ? lo : v>hi ? hi : v;
}

constexpr int32_t kWeightMin = -64, kWeightMax = 191;
constexpr uint32_t kSlackMax = 0x4000;
constexpr int32_t kP2FreqRescaleTotal = 29696;
constexpr uint32_t kCounterNudgeLimit = 0xCCC;
constexpr int32_t kP2RowPad = 234;
constexpr int32_t kCtxId3Limit = 53248;
constexpr uint32_t kModelTableBytes = 0x7F000u;
constexpr uint32_t kBankMirrorMask = 0x7FF0;
template <class T> void swap32(T &a, T &b) {
  const T t = a;
  a = b;
  b = t;
}

int32_t clamp_weight(int32_t w) {
  return clamp32(w, kWeightMin, kWeightMax);
}

uint32_t halve_up(uint32_t x) {
  return x-(x>>1);
}

void halve_counts(uint16_t* c, int32_t n) {
  for( int32_t k = 0; k<n; ++k )
    c[k] = (uint16_t)halve_up(c[k]);
}

constexpr int32_t kMaxWidth = 0xFFFF;


struct CtxIdx {
  uint32_t val;
  template <int32_t Pos> constexpr CtxIdx &bit(bool b) {
    val += (uint32_t)b<<Pos;
    return this[0];
  }

  template <int32_t Pos, int32_t W> constexpr CtxIdx &bits(uint32_t v) {
    val += (v&((1u<<W)-1))<<Pos;
    return this[0];
  }

  template <uint32_t Stride, uint32_t Radix> constexpr CtxIdx &digit(uint32_t v) {

    val += Stride*v;
    return this[0];
  }

  template <int32_t Pos> constexpr CtxIdx &bit_of(uint32_t v) {
    val |= v&(1u<<Pos);
    return this[0];
  }

  template <int32_t Pos> constexpr CtxIdx &above(uint32_t v) {
    val += v<<Pos;
    return this[0];
  }

  constexpr CtxIdx &raw(uint32_t v) {
    val += v;
    return this[0];
  }

  constexpr operator uint32_t() const {
    return val;
  }
};

template <class T, int32_t N> struct BlockPool {
  alignas(4096) T slot[N];
  uint8_t taken[N];
  void unwatch(int32_t k) {}

  void watch(int32_t k) {}

  void Init() {
    for( int32_t k = 0; k<N; ++k )
      taken[k] = 0;
  }

  T*take(bool zero) {
    for( int32_t k = 0; k<N; ++k ) {
      if( taken[k] )
        continue;
      taken[k] = 1;
      unwatch(k);
      if( zero )
        memset(&slot[k], 0, sizeof(T));
      return &slot[k];
    }
    bmf_fatal(bmf_no_memory);
    return 0;
  }

  void give(T* p) {
    const int32_t k = (int32_t)(p-slot);

    taken[k] = 0;
    watch(k);
  }
};

void*bmf_new(size_t n) {
  void* p = malloc(n ? n : 1);
  if( !p )
    bmf_fatal(bmf_no_memory);
  return p;
}

enum : uint8_t {
  ctx_w4_to_w1 = 0x01,
  ctx_w4_to_w2 = 0x02,
  ctx_w3_double = 0x04,
  ctx_w3_to_w1 = 0x08,
  ctx_w3_to_w2 = 0x10,
  ctx_w2_to_w1 = 0x20,
};
