#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pmmintrin.h>
#include <xmmintrin.h>

#ifdef __GNUC__
 #define INLINE   __attribute__((always_inline)) inline
 #define NOINLINE __attribute__((noinline))
 #define ALIGN(n) __attribute__((aligned(n)))
 #define restrict __restrict
#else
 #define INLINE   __forceinline
 #define NOINLINE __declspec(noinline)
 #define ALIGN(n) __declspec(align(n))
#endif

#define if_e0(x) if(__builtin_expect((x),0))
#define if_e1(x) if(__builtin_expect((x),1))

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
  uint32_t val = 0;
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
  uint8_t taken[N] = {};
  void unwatch(int32_t k) {}

  void watch(int32_t k) {}

  BlockPool() {}

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
alignas(16) static constexpr uint8_t ctx_group_flags[32] = {
  0, 1, 2, 4, 8, 10, 13, 16, 17, 22, 32, 35, 36, 56, 63, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
alignas(16) static constexpr int32_t p2_ctx_rotate[4] = {
  4, 8, 12, 8,
};
alignas(16) static constexpr uint8_t p2_ctx_edges[16] = {
  17, 20, 27, 37, 49, 70, 93, 124, 157, 191, 205, 228, 235, 236, 237, 0,
};
alignas(16) static constexpr uint8_t p2_len_edges[32] = {
  4, 6, 8, 11, 14, 17, 21, 25, 30, 37, 45, 55, 67, 87, 120, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
alignas(16) static constexpr int32_t bmf_p2_thresholds[6][13] = {
  {6, 14, 28, 4, 8, 26, 4096, 6144, 13056, 10240, 14336, 26624, 36864}, {4, 15, 42, 1, 8, 19, 2560, 6912, 11264, 10240, 28672, 71680, 139264}, {5, 14, 21, 4, 9, 25, 3840, 8448, 12032, 16384, 26624, 43008, 83968}, {6, 11, 20, 5, 11, 23, 2816, 5888, 10496, 22528, 32768, 51200, 131072}, {0, 11, 30, 7, 13, 22, 4352, 8448, 14848, 16384, 38912, 57344, 94208}, {7, 10, 31, -1, 11, 21, -256, 1024, 5888, -2048, -2048, -2048, 145408},
};
alignas(16) static const uint8_t p2_b1_reload[3] = {7, 46, 197};
alignas(16) static constexpr uint8_t p1_level_edges[8] = {
  1, 2, 4, 8, 14, 35, 103, 0,
};
alignas(16) static constexpr uint8_t p1_group_edges[8] = {
  1, 3, 6, 10, 16, 27, 52, 0,
};
alignas(16) static constexpr uint8_t p1_slot_edges[8] = {
  5, 10, 36, 98, 154, 236, 248, 0,
};
alignas(16) static constexpr int32_t p1_level_step[8] = {
  1, 1, 2, 2, 2, 4, 4, 4,
};
alignas(16) static const float bmf_p2_coef_init[7][4] = {
  {-0.05f, -0.07f, 0.3f, 0.18f}, {-0.03f, 0.04f, 0.06f, -0.01f}, {0.06f, 0.16f, 0.0f, -0.01f}, {-0.1f, -0.02f, 0.05f, 0.05f}, {0.06f, 0.0f, 0.04f, 0.02f}, {0.04f, 0.02f, 0.04f, 0.05f}, {0.07f, 0.0f, 0.03f, 0.02f},
};
alignas(16) static const float bmf_p2_rate_init[7][4] = {
  {0.0108f, 0.0069f, 0.0054f, 0.0052f}, {0.0042f, 0.0042f, 0.0042f, 0.0039f}, {0.0035f, 0.0035f, 0.0028f, 0.0028f}, {0.0027f, 0.0026f, 0.0026f, 0.0024f}, {0.0023f, 0.0023f, 0.0022f, 0.0021f}, {0.0019f, 0.0019f, 0.0017f, 0.0017f}, {0.0015f, 0.0015f, 0.0011f, 0.0009f},
};
const float bmf_p2_mix[4][6] = {
  {0.4f, 0.1f, 0.15f, 0.23f, 0.04f, 0.03f},
  {0.26f, 0.23f, 0.13f, 0.12f, 0.15f, 0.12f},
  {0.27f, 0.22f, 0.26f, 0.08f, 0.12f, 0.05f},
  {0.3f, 0.14f, 0.08f, 0.22f, 0.18f, 0.09f},
};

const float bmf_p2_rate_reset = 0.0024f;
const float bmf_p2_ms_rate = 0.023f;
const float bmf_p2_decay = 0.78f;
const float bmf_p2_seed = 0.19f;

struct CodedTail {
  int32_t tag;
  uint32_t len;
  uint32_t data[0];
};

struct Packer {
  uint32_t* word;
  uint32_t acc;
  int32_t free_bits;
};

struct CodedStream {
  uint8_t* buf;
  uint8_t* cur;
  int32_t size;
  Packer pk;
};

enum : uint8_t {
  desc_predictor = 0x03,
  desc_alt_model = 0x04,
  desc_has_refs = 0x08,
};

enum : int32_t {
  pred_mode0 = 0,
  pred_p1 = 1,
  pred_p2 = 2,
};

struct PlaneDesc {
  union {
    int32_t desc_word;
    struct {
      uint8_t nrefs;
      uint8_t src_plane;
      uint8_t flags;
      uint8_t dc;
    };
  };
  int32_t weight0;
  int32_t weight1;
  int32_t weight2;
};

struct LevelGeom {
  uint8_t first;
  uint8_t half;
  uint8_t tbl_base;
  uint8_t _pad;
};

constexpr int32_t opt_use_filters = 1;
constexpr int32_t opt_slow = 1;
constexpr int32_t opt_filter_template = 0;
constexpr int32_t opt_pack_output = 1;
constexpr int32_t opt_search_quality = 9;
constexpr int32_t opt_max_error = 0;
constexpr int32_t no_symbol = 8192;

void bmf_zero16(void* p) {
  memset(p, 0, 16);
}

void bmf_set_denormal_mode() {
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
}

#pragma pack(push, 1)
struct SymEntry {
  uint16_t sym;
  uint8_t cnt;
  void set(uint32_t s, uint32_t c) {
    sym = (uint16_t)s;
    cnt = (uint8_t)c;
  }
};
#pragma pack(pop)

float bmf_hsum4(const float x[4]) {
  return (x[0]+x[2])+(x[1]+x[3]);
}

float nb_dot(const float (*wt)[4], const float (*nb)[4]) {
  float acc[4];
  for( int32_t k = 0; k<4; ++k ) {
    acc[k] = wt[0][k]*nb[0][k];
    for( int32_t j = 1; j<7; ++j )
      acc[k] += wt[j][k]*nb[j][k];
  }
  return bmf_hsum4(acc);
}

template <typename T> void ring_advance(T** buf, T** cursor, int32_t margin) {
  T*const oldest = buf[4];
  for( int32_t i = 0; i<4; ++i )
    buf[4-i] = buf[3-i];
  buf[0] = oldest;
  for( int32_t k = 0; k<5; ++k )
    cursor[k] = buf[k]+margin;
}

struct RangeCoder {
  CodedStream* st;
  static const uint32_t kTop = 0x00800000;
  static const uint32_t kPend = 0x7F800000;
  static const uint32_t kCarry = 0x80000000;
  static const uint32_t kMask = 0x7FFFFFFF;
  static const uint8_t kMarker = 0x97;

private:
  uint32_t range;
  uint32_t low;
  uint8_t cache;
  uint32_t pending;
  uint32_t rdiv;
  uint32_t bytes;
  uint8_t*p() {
    return st[0].cur;
  }

  void set_p(uint8_t* q) {
    st[0].cur = q;
  }

  uint8_t dec_get(uint8_t*&q) {
    if( q>=st[0].buf+st[0].size )
      bmf_fatal(bmf_read_error);
    return *q++;
  }

public:
  void enc_init() {
    range = 0x80000000;
    low = 0;
    pending = 0;
    bytes = 0;
    cache = kMarker;
  }

  uint8_t*emit_carry(uint32_t carry) {
    uint8_t* q = p();
    *q++ = (uint8_t)(cache+carry);
    for(; pending; --pending )
      *q++ = (uint8_t)(carry-1);
    return q;
  }

  void emit(uint32_t carry) {
    set_p(emit_carry(carry));
    cache = (uint8_t)(low>>23);
  }

  void enc_normalise() {
    while( range<=kTop ) {
      ++bytes;
      if( low<kPend )
        emit(0);
      else if( low&kCarry )
        emit(1);
      else
        ++pending;
      range <<= 8;
      low = (low<<8)&kMask;
    }
  }

  uint32_t encode(uint32_t cum, uint32_t high, uint32_t tot) {
    enc_normalise();
    uint32_t r = range/tot;
    uint32_t below = r*cum;
    uint32_t width = r*(high-cum);
    range = (high<tot) ? width : range-below;
    low += below;
    return width;
  }

  uint32_t encode_bit(uint32_t f0, uint32_t f1, int32_t bit) {
    enc_normalise();
    uint32_t rt = f0*(range/(f0+f1));
    if( bit ) {
      low += rt;
      range -= rt;
    } else {
      range = rt;
    }
    return range;
  }

  void flush() {
    enc_normalise();
    bytes += 5;
    uint32_t len = bytes;
    uint32_t rounded = ((low&0x7FFFFF)>=((len&0xFFFFFF)>>1))+(low>>23);
    uint32_t carry = rounded>0xFF;
    uint8_t* q = emit_carry(carry);
    *q++ = (uint8_t)rounded;
    *q++ = (uint8_t)(len>>16);
    *q++ = (uint8_t)(len>>8);
    *q++ = (uint8_t)len;
    while( (uint32_t)(q-st[0].buf)%4!=3 )
      *q++ = 0;
    *q++ = kMarker;
    set_p(q);
    st[0].pk.free_bits = 0;
    st[0].pk.acc = 0;
    st[0].pk.word = (uint32_t*)st[0].cur;
  }

  void dec_init() {
    uint8_t* q = p();
    if( dec_get(q)!=kMarker )
      bmf_fatal(bmf_read_error);
    cache = dec_get(q);
    set_p(q);
    range = 128;
    low = cache>>1;
  }

  void dec_normalise() {
    uint8_t* q = p();
    while( range<=kTop ) {
      uint32_t head = (uint32_t)(uint8_t)(cache<<7)|(low<<8);
      cache = dec_get(q);
      low = (cache>>1)|head;
      range <<= 8;
    }
    set_p(q);
  }

  uint32_t get_freq(uint32_t tot) {
    dec_normalise();
    rdiv = range/tot;
    uint32_t count = low/rdiv;
    return umin32(count, tot-1);
  }

  void decode(uint32_t cum, uint32_t high, uint32_t tot) {
    uint32_t below = cum*rdiv;
    low -= below;
    range = (high<tot) ? (high-cum)*rdiv : range-below;
  }

  int32_t decode_bit(uint32_t f0, uint32_t f1) {
    dec_normalise();
    uint32_t rt = f0*(range/(f0+f1));
    if( low>=rt ) {
      range -= rt;
      low -= rt;
      return 1;
    }
    range = rt;
    return 0;
  }

  void finish() {
    dec_normalise();
    uint8_t* q = p();
    while( dec_get(q)!=kMarker ) {
    }
    set_p(q);
    st[0].pk.word = (uint32_t*)st[0].cur;
    st[0].pk.free_bits = 0;
    st[0].pk.acc = 0;
  }
};

struct FreqPair {
  uint16_t f[2];
};

uint32_t halve_pair(FreqPair* pair, int32_t go) {
  halve_counts(pair[0].f, 2);
  return pair[0].f[go];
}

void end_plane_stream() {
}

struct BMFState {
  alignas(16) CodedStream stream;
  RangeCoder rc = {};
  alignas(16) CodedTail* coded_block = nullptr;
  alignas(16) int32_t plane_predictor = 0;
  alignas(16) int32_t plane_alt_model = 0;
  int32_t desc_slow_mode = {};
  int32_t alphabet_reduced = {};
  PlaneDesc plane_desc[4] = {};
  int32_t plane_count = {};
  int32_t near_lossless_q = {};
  uint8_t model_geometry[128] = {};
  int8_t exclusion_gen = {};
  alignas(16) int8_t exclusion_mask[(no_symbol+1+15)&~15];
  int32_t mode_symbol[5] = {};
  int32_t alt_freq_limit = {};
  int32_t alt_freq_init = {};
  LevelGeom level_geom[8] = {};
  int32_t ctx_bias[4] = {};
  int32_t deadzone_hi = {};
  int32_t deadzone_lo = {};
  uint8_t* hist_scratch = {};
  alignas(16) uint16_t model_table_store[kModelTableBytes/sizeof(uint16_t)];
  uint16_t* model_tables = {};
  void hist_bump(int32_t k) {
    uint32_t n;
    memcpy(&n, &hist_scratch[4*k], sizeof n);
    ++n;
    memcpy(&hist_scratch[4*k], &n, sizeof n);
  }

  uint16_t*model_strip(uint32_t k) {
    return model_tables+254*k;
  }

  template <int32_t f_DEC> int32_t code_symbol_tree(uint16_t* freq, int32_t sym) {
    uint16_t add;
    uint32_t cum, j;
    int32_t lvl, result, go, span;
    uint16_t fq, * slot;
    uint16_t* f0 = freq+2;
    uint32_t tot = freq[0];
    if constexpr( f_DEC ) {
      uint32_t target = rc.get_freq(tot);
      slot = f0;
      lvl = 0;
      cum = f0[0];
      if( cum<=target ) {
        do {
          ++slot;
          ++lvl;
          cum += slot[0];
        } while( cum<=target );
      }
      rc.decode(cum-slot[0], cum, tot);
    } else {
      lvl = model_geometry[sym];
      cum = 0;
      for( j = 0; j<(uint32_t)lvl; j++ )
        cum += f0[j];
      slot = &f0[lvl];
      uint32_t cum_hi = cum+slot[0];
      result = rc.encode(cum, cum_hi, tot);
    }
    if( freq[0]>0x4000u ) {
      halve_counts(freq+2, 8);
      uint16_t total = 0;
      for( int32_t k = 2; k<10; ++k )
        total += freq[k];

      int32_t esc = freq[1];
      freq[0] = total;
      if( esc<=4*alt_freq_limit ) {
        result = 4*(esc>alt_freq_limit);
        add = esc-result;
        freq[1] = add;
      } else {
        add = esc-16;
        freq[1] = add;
      }
    } else {
      add = freq[1];
    }
    slot[0] += add;
    freq[0] += freq[1];
    if( lvl<2 )
      return lvl;
    int32_t mask = level_geom[lvl].half;
    int32_t path = 0;
    if constexpr( !f_DEC )
      path = sym-level_geom[lvl].first;
    int32_t node = 0;
    for( span = 1;; span *= 2 ) {
      FreqPair* pair = bit_tree(freq, lvl)+span+node;
      int32_t f1 = pair[0].f[1];
      int32_t fa = pair[0].f[0];
      if constexpr( f_DEC ) {
        go = rc.decode_bit(fa, f1);
      } else {
        go = (mask&path)!=0;
        result = rc.encode_bit(fa, f1, go);
      }
      fq = pair[0].f[go];
      if( fq>0x4000u ) fq = halve_pair(pair, go);
      pair[0].f[go] = alt_freq_init+fq;
      mask >>= 1;
      node = go+2*node;
      if( !mask ) return node+level_geom[lvl].first;
    }
  }
  void encode_symbol_tree(uint16_t* freq, int32_t sym) {
    code_symbol_tree<0>(freq, sym);
  }
  int32_t decode_symbol_tree(uint16_t* freq) {
    return code_symbol_tree<1>(freq, 0);
  }
  int32_t update_binary_pair(uint16_t* _this, int32_t symbol) {
    int32_t node;
    uint32_t step, f;
    int32_t tot = _this[0];
    if( (uint32_t)tot<=0x8000 ) {
      int32_t lvl = model_geometry[symbol];
      step = (_this[1]>>2)&0xFFFFFFE0;
      if( plane_predictor==pred_p2 )
        step = 15*(_this[1]>>5);
      _this[lvl+2] += step+4;
      tot = _this[0]+step+4;
      _this[0] = tot;
      if( symbol>=2 ) {
        int32_t mask = level_geom[lvl].half;
        int32_t path = symbol-level_geom[lvl].first;
        node = 0;
        FreqPair* tbl = bit_tree(_this, lvl);
        int32_t span = 1;
        do {
          FreqPair* pair = tbl+node+span;
          int32_t go = (mask&path)!=0;
          f = pair[0].f[go];
          if( f>0x2000 )
            f = halve_pair(pair, go);
          span *= 2;
          mask >>= 1;
          node = go+2*node;
          pair[0].f[go] = f+((alt_freq_init*((uint32_t)(plane_predictor==pred_p2)+5))>>3);
        } while( mask );
        return 0;
      }
    }
    return tot;
  }
  uint32_t rc_decode_flat(uint32_t tot) {
    uint32_t sym = rc.get_freq(tot);
    rc.decode(sym, sym+1, tot);
    return sym;
  }
  uint32_t alt_init_tables(uint8_t* fold, int8_t* unfold) {
    uint8_t even, odd;
    uint32_t done;
    int32_t in_bucket, bucket;
    uint8_t half;
    uint32_t i, b;

    int32_t bucket_size = 2*near_lossless_q+1;
    unfold[0] = 0;
    unfold[255] = 0x80;
    for( i = 0; i<0x3F; ++i ) {
      unfold[4*i+2] = (int8_t)(2*i+1);
      unfold[4*i+1] = (int8_t)(-2*i-1);
      unfold[4*i+4] = (int8_t)(2*i+2);
      unfold[4*i+3] = (int8_t)(-2*i-2);
    }
    unfold[254] = 127;
    unfold[253] = -127;
    fold[0] = 0;
    uint32_t lo = 1;
    fold[128] = (uint8_t)-1;
    {
      uint32_t span = 128-lo;
      uint32_t pairs = (128-lo)/2;
      half = 0;
      in_bucket = 1;
      {
        uint32_t k = 0;
        int32_t ofs = 0;
        uint8_t* pos = &fold[lo];
        uint8_t* neg = fold-lo;
        for(; k<pairs; ++k, ofs -= 2 ) {
          bucket = bucket_size;
          int32_t bucket_1 = in_bucket-1;
          if( bucket_1 )
            bucket = bucket_1;
          else
            ++half;
          even = 2*half;
          pos[2*k] = even;
          odd = 2*half-1;
          in_bucket = bucket-1;
          neg[ofs+256] = odd;
          if( bucket==1 ) {
            in_bucket = bucket_size;
            even = 2*++half;
            odd = 2*half-1;
          }
          pos[2*k+1] = even;
          neg[ofs+255] = odd;
        }
        done = 2*k+1;
      }
      if( (done-1)<span ) {
        if( in_bucket==1 ) half = half+1;
        fold[lo-1+done] = 2*half;
        fold[257-lo-done] = 2*half-1;
      }
    }
    for( b = 0; b<0x80; ++b ) {
      fold[(uint8_t)((uint8_t*)unfold)[2*b]+256] = 2*b;
      fold[(uint8_t)((uint8_t*)unfold)[2*b+1]+256] = 2*b+1;
    }
    return b;
  }
  template <int32_t f_DEC> uint32_t code_alphabet_size(uint32_t n, uint32_t cap) {
    if constexpr( f_DEC ) return rc_decode_flat(cap)+1;
    rc.encode(n-1, n, cap);
    return n;
  }
  FreqPair*bit_tree(uint16_t* freq, int32_t lvl) {
  return (FreqPair*)&freq[2*level_geom[lvl].tbl_base+8];
  }

  uint16_t*begin_plane_stream() {
    if( plane_predictor==pred_p2 ) {
      alt_freq_init = 8;
      alt_freq_limit = 8;
    } else {
      alt_freq_limit = 16;
      alt_freq_init = 64;
    }
    model_geometry[0] = 0;
    model_geometry[1] = 1;
    level_geom[2].half = 1;
    level_geom[2].first = 2;
    level_geom[2].tbl_base = 0;
    memset(&model_geometry[2], 2, 2);
    int32_t slot = 4;
    for( int32_t lvl = 3; lvl<8; ++lvl ) {
      level_geom[lvl].half = 1<<(lvl-2);
      level_geom[lvl].first = (uint8_t)slot;
      level_geom[lvl].tbl_base = (uint8_t)slot-lvl;
      memset(&model_geometry[slot], lvl, 2*level_geom[lvl].half);
      slot += 2*level_geom[lvl].half;
    }
    uint16_t* tbl = model_table_store;
    if( tbl ) {
      int32_t k = 0;
      uint16_t* row = tbl;
      const uint16_t seed_row[10] = {
        635, (uint16_t)(24*alt_freq_limit), 205, 124, 147, 83, 48, 16, 8, 4,
      };
      for( k = 0; k<0x400; ++k, row += 254 ) {
        memcpy(row, seed_row, sizeof seed_row);
        FreqPair* seed = (FreqPair*)&row[10];
        for( uint32_t i = 0; i<0x7A; ++i ) {
          seed[i].f[0] = 60;
          seed[i].f[1] = 36;
        }
      }
    }
    model_tables = tbl;
    return tbl;
  }
  void packer_rewind() {
    if( stream.cur==(uint8_t*)stream.pk.word ) return;
    int32_t bits = stream.pk.free_bits-8;
    if( bits<0 ) {
      stream.pk.free_bits = bits;
      return;
    }
    uint8_t* at = stream.cur;
    while( bits>=0 ) { --at; bits -= 8; }
    stream.cur = at;
    stream.pk.free_bits = bits;
  }
  template <int32_t f_DEC> uint16_t*rc_begin() {
    if constexpr( !f_DEC ) *stream.pk.word = stream.pk.acc;
    packer_rewind();
    uint16_t* tbl = plane_alt_model ? begin_plane_stream() : nullptr;
    if constexpr( f_DEC )
      rc.dec_init();
    else
      rc.enc_init();
    return tbl;
  }
  void rc_begin_decode() {
    rc_begin<1>();
  }
  uint16_t*rc_begin_encode() {
    return rc_begin<0>();
  }
  template <int32_t f_DEC> void rc_end() {
    if constexpr( f_DEC )
      rc.finish();
    else
      rc.flush();
    end_plane_stream();
  }
  void rc_end_decode() {
    rc_end<1>();
  }
  void rc_end_encode() {
    rc_end<0>();
  }
  bool ref_transformed(int32_t k) {
    return (plane_desc[plane_desc[k].src_plane].flags&desc_has_refs)!=0;
  }
  alignas(16) float p2_coef[7][4] = {};
  alignas(16) float p2_rate[7][4] = {};
  SymEntry byte_list_ent[16*256] = {};
  SymEntry gap_list_ent[257] = {};
  void reset() {
    memset(static_cast<void*>(this), 0, sizeof(BMFState));
    bmf_set_denormal_mode();
    rc.st = &stream;
    memcpy(p2_coef, bmf_p2_coef_init, sizeof p2_coef);
    memcpy(p2_rate, bmf_p2_rate_init, sizeof p2_rate);
  }

  BMFState() {
    reset();
  }

  BMFState(const BMFState &) = delete;
  BMFState &operator=(const BMFState &) = delete;
};




struct FreqRec {
  union {
    uint16_t w[8];
    struct {
      uint16_t _w0to6[7];
      uint8_t b14;
      uint8_t b15;
    };
  };
  bool seed_pair(int32_t lo, int32_t hi, int32_t alphabet_lvl) {
    if( !w[lo]||!w[hi]||b14>alphabet_lvl ) return false;
    const int16_t weight = (uint8_t)(1<<((5-b14)&31));
    b15 = weight;
    w[6] = weight<<6;
    w[lo] += weight;
    w[hi] += b15;
    resum();
    return true;
  }

  int32_t find_level(int32_t target, int32_t &cum) const {
    int32_t lvl = 0;
    cum = w[0];
    while( lvl<4&&cum<=target ) cum += w[++lvl];
    return lvl;
  }

  void bump(int32_t lvl) {
    const uint16_t floor_ = w[6];
    uint16_t total = w[5];
    uint8_t kick = b15;
    if( total>floor_&&(w[lvl]+kick+8<total||w[5]>0x4000u) ) total = rescale(floor_, kick);
    w[5] = kick+total;
    w[lvl] += kick;
  }

  void blend_from(const FreqRec &src) {
    const int32_t scale = src.b15;
    int32_t kept[5];
    for( int32_t i = 0; i<5; ++i ) kept[i] = scale*w[i];
    this[0] = src;
    b14 *= 8;
    const int32_t total = w[5];
    for( int32_t i = 0; i<5; ++i ) w[i] = (uint16_t)(kept[i]+(21*w[i]+total-1)/total);
    resum();
  }

  uint16_t resum() {
    uint16_t sum = (uint16_t)(w[0]+w[1]+w[2]+w[3]+w[4]);
    w[5] = sum;
    return sum;
  }

  uint16_t rescale(uint16_t escape_floor, uint8_t &kick) {
    halve_counts(w, 5);
    const uint16_t sum = resum();
    if( escape_floor<256&&!b14 ) {
      escape_floor = 256;
      w[6] = 256;
    }
    if( sum>escape_floor&&kick<15 ) {
      kick = 15;
      b15 = kick;
    }
    return sum;
  }
};

struct SymList {
  uint32_t n;
  uint32_t live;
  uint32_t tot;
  uint32_t since_rescale;
  uint32_t rescale_at;
  SymEntry* ent;
  bool rescale(int32_t count) {
    int32_t last_cnt;
    uint32_t n_left, running, since, due = rescale_at;
    SymEntry* back;
    if( count<=251&&due>=since_rescale )
      return false;
    n_left = live;
    int32_t bias = due<20*n;
    SymEntry* cur = ent-1;
    SymEntry* prev = cur;
    do {
      prev = cur;
      ++cur;
      int32_t half = (bias+(uint32_t)cur[0].cnt)>>1;
      cur[0].cnt = half;
      if( cur!=ent ) {
        SymEntry* up = cur-1;
        int32_t up_cnt = up[0].cnt;
        if( half>up_cnt ) {
          uint16_t keep = cur[0].sym;
          cur[0].set(up[0].sym, up_cnt);
          if( up!=ent ) {
            do {
              back = up-1;
              int32_t back_cnt = back[0].cnt;
              if( half<=back_cnt )
                break;
              up[0].set(back[0].sym, back_cnt);
              --up;
            } while( back!=ent );
          }
          up[0].set(keep, half);
        }
      }
      --n_left;
    } while( n_left );
    running = tot;
    if( !cur[0].cnt ) {
      do {
        ++n_left;
        tot = ++running;
        last_cnt = prev[0].cnt;
        --prev;
      } while( !last_cnt );
      live -= n_left;
    }
    since = since_rescale;
    tot = halve_up(running);
    since_rescale = halve_up(since);
    return true;
  }

  int32_t move_up(SymEntry* p) {
    if( p==ent )
      return p[0].cnt;
    uint16_t s_a = p[0].sym;
    uint8_t c_a = p[0].cnt;
    SymEntry* q = p-1;
    p[0] = q[0];
    q[0].set(s_a, c_a);
    if( q==ent )
      return q[0].cnt;
    while( 1 ) {
      int32_t top = q[0].cnt;
      SymEntry* r = q-1;
      if( top<=r[0].cnt )
        return top;
      uint16_t s_b = q[0].sym;
      uint8_t c_b = q[0].cnt;
      q[0] = r[0];
      r[0].set(s_b, c_b);
      --q;
      if( r==ent )
        return r[0].cnt;
    }
  }

  int32_t promote(SymEntry* p) {
    p[0].cnt += 4;
    since_rescale += 4;
    return move_up(p);
  }

  template <int32_t f_DEC> uint32_t code_symbol_bytes(BMFState* cx, uint32_t nbytes, uint32_t val, int32_t &carry) {
    SymList*const lists = this;
    uint32_t word = val, out = 0;
    for( uint32_t b = 0; b<nbytes; ++b ) {
      uint32_t piece;
      if constexpr( f_DEC ) {
        piece = lists[4*b+carry].decode_symbol_list(cx);
        out += piece<<((8*b)&31);
      } else {
        piece = (uint8_t)word;
        lists[4*b+carry].code_symbol(cx, (uint8_t)piece);
        word >>= 8;
      }
      carry = piece>>6;
    }
    if constexpr( !f_DEC )
      out = val;
    carry = (uint8_t)out>>7;
    return out;
  }

  int32_t code_symbol(BMFState* cx, int32_t want) {
    int32_t enc_cum, enc_high, enc_tot;
    int32_t c, c2;
    uint32_t i;
    int8_t gen = cx[0].exclusion_gen;
    uint32_t left = live;
    SymEntry* p = ent-1;
    int32_t cum = 0;
    while( 1 ) {
      ++p;
      int32_t s = p[0].sym;
      if( cx[0].exclusion_mask[s]!=cx[0].exclusion_gen ) {
        c = p[0].cnt;
        cum += c;
        if( s==want )
          break;
      }
      if( --left )
        continue;
      if( !cum )
        return 0;
      enc_cum = cum;
      enc_tot = tot+cum;
      enc_high = enc_tot;
      do {
        cx[0].exclusion_mask[p[0].sym] = gen;
        --p;
      } while( p>=ent );
      cx[0].rc.encode(enc_cum, enc_high, enc_tot);
      return 0;
    }
    enc_high = cum;
    uint32_t rest = left-1;
    enc_cum = cum-c;
    if( rest ) {
      for( i = 0; i<rest; ++i ) {
        if( cx[0].exclusion_mask[p[i+1].sym]==cx[0].exclusion_gen )
          c2 = 0;
        else
          c2 = p[i+1].cnt;
        cum += c2;
      }
    }
    enc_tot = tot+cum;
    rescale(promote(p));
    cx[0].rc.encode(enc_cum, enc_high, enc_tot);
    return 1;
  }

  void add_weight(int32_t want, uint32_t add) {
    int32_t recycled;
    SymEntry* list = ent;
    uint32_t n_live = live;
    SymEntry* p = list;
    uint32_t left = n_live;
    while( left&&p[0].sym!=want ) {
      ++p;
      --left;
    }
    if( left ) {
      p[0].cnt += add;
      since_rescale += add;
      rescale(move_up(p));
      return;
    }
    const bool full = n_live==n;
    if( n_live>=n&&add<=1 )
      return;
    if( full ) {
      live = --n_live;
      recycled = list[n_live].cnt;
    } else {
      recycled = 1;
    }
    list += n_live;
    live = n_live+1;
    tot = recycled+tot+1;
    list[0].cnt = 2;
    list[0].sym = want;
    since_rescale += 4;
    if( list!=ent ) {
      uint16_t s3 = list[0].sym;
      uint8_t c3 = list[0].cnt;
      list[0] = list[-1];
      list[-1].set(s3, c3);
    }
  }

  void init(int32_t n_syms, int32_t dense, SymEntry* storage) {
    uint32_t result;
    const uint32_t n_u = (uint32_t)n_syms;
    n = n_syms;
    ent = storage;
    if( dense ) {
      tot = 0;
      live = n_u;
      since_rescale = 12*n_u;
      rescale_at = 8*n_u;
      for( result = 0; result<live; ++result ) {
        ent[result].sym = result;
        ent[result].cnt = 1;
      }
    } else {
      tot = 2;
      rescale_at = 20*n_u;
      live = 0;
      since_rescale = 18*n_u;
      memset(storage, 0, 3*n_u);
    }
    return;
  }

  inline int32_t decode_symbol_list(BMFState* cx) {
    SymEntry* list[no_symbol+1];
    SymEntry* q;
    SymEntry** w, ** rd, ** rd2;
    int32_t c;
    w = list;
    uint32_t gen = (uint8_t)cx[0].exclusion_gen;
    int32_t cum = 0;
    for( int32_t i = 0; i<(int32_t)live; ++i ) {
      if( (uint8_t)cx[0].exclusion_mask[ent[i].sym]==gen ) {
        c = 0;
      } else {
        SymEntry* e = &ent[i];
        c = e[0].cnt;
        *w++ = e;
      }
      cum += c;
    }
    if( !cum )
      return -1;
    w[0] = nullptr;
    int32_t tot_all = cum+(int32_t)tot;
    int32_t target = cx[0].rc.get_freq(tot_all);
    SymEntry* p = list[0];
    rd = &list[1];
    SymEntry* first = list[0];
    uint32_t cum_lo = 0;
    while( 1 ) {
      cum_lo += p[0].cnt;
      if( cum_lo>(uint32_t)target )
        break;
      p = *rd++;
      if( !p ) {
        int8_t gen_b = (int8_t)gen;
        q = first;
        rd2 = &list[1];
        do {
          cx[0].exclusion_mask[q[0].sym] = gen_b;
          q = *rd2++;
        } while( q );
        cx[0].rc.decode(cum_lo, tot_all, tot_all);
        return -1;
      }
    }
    int32_t result = p[0].sym;
    uint32_t sym_cum = cum_lo-p[0].cnt;
    this[0].rescale(this[0].promote(p));
    cx[0].rc.decode(sym_cum, cum_lo, tot_all);
    return result;
  }
};

void clear_sym_lists(SymList* lists) {
  for( int32_t k = 0; k<16; ++k )
    lists[k].ent = nullptr;
}

struct SymListBlock {
  uint32_t n;
  SymList list[0];
  static constexpr uint32_t bytes(uint32_t n) {
    return offsetof(SymListBlock, list)+sizeof(SymList)*n;
  }
};

SymList*new_sym_lists(uint32_t n, void* storage) {
  SymListBlock* block = (SymListBlock*)storage;
  block[0].n = n;
  for( uint32_t i = 0; i<n; ++i )
    block[0].list[i].ent = nullptr;
  return block[0].list;
}

void free_sym_entries(SymList*, uint32_t) {
}

void init_byte_lists(BMFState* cx, SymList* list, uint32_t nbytes) {
  for( uint32_t k = 0; k<4*nbytes; ++k )
    list[k].init(256, 1, &cx[0].byte_list_ent[k*256]);
}

void init_gap_list(BMFState* cx, SymList* list, uint32_t mask, uint32_t n_syms) {
  list[0].init(mask-n_syms+2, 1, cx[0].gap_list_ent);
  list[0].rescale_at = 19*list[0].n;
}

SymListBlock*sym_list_block(SymList* list) {
  return (SymListBlock*)((uint8_t*)list-offsetof(SymListBlock, list));
}

uint32_t sym_list_count(const SymList* list) {
  return sym_list_block((SymList*)list)[0].n;
}

bool sym_in_top(const SymEntry* ent, int32_t n, int32_t sym) {
  while( n-- ) if( ent[n].sym==sym ) return true;
  return false;
}
int32_t sym_slot(int32_t code) {
  return code<5 ? code : 6-(code&1);
}

struct CounterNode {
  uint16_t total;
  uint16_t c[7];
  template <int32_t f_DEC> int32_t code_symbol(BMFState* cx, int32_t ctx, int32_t sym) {
    uint32_t cum;
    int32_t slot;
    uint16_t* cur = c;
    const uint32_t tot = total&0x7FFF;
    if constexpr( f_DEC ) {
      sym = cx[0].rc.get_freq(tot);
      cum = c[0];
      while( cum<=(uint32_t)sym )
        cum += *++cur;
      cx[0].rc.decode(cum-cur[0], cum, tot);
    } else {
      slot = sym_slot(sym);
      cum = 0;
      for( int32_t k = 0; k<slot; k++ )
        cum += c[k];
      cur = &c[slot];
      cx[0].rc.encode(cum, cum+cur[0], tot);
    }
    if( tot>0x2000 ) {
      total = 0x8000;
      int32_t lo = 256;
      for( int32_t k = 6; k>=0; --k )
        if( c[k]<lo )
          lo = c[k];
      for( int32_t k = 6; k>=0; --k ) {
        const uint16_t nf = lo<=1 ? c[k]-(c[k]>>1) : (c[k]+2)/3;
        c[k] = nf;
        total += nf;
      }
    }
    cur[0] += 32;
    const int32_t result = total+32;
    total = result;
    slot = cur-c;
    if( slot>=5 ) {
      uint16_t*const strip = cx[0].model_strip(CtxIdx{}.bit<7>(slot&1).bit<6>(2u*c[slot]>c[0]+(uint32_t)(result&0x7FFF)+96).raw((uint32_t)ctx));
      if constexpr( f_DEC )
        slot += 2*cx[0].decode_symbol_tree(strip);
      else
        cx[0].encode_symbol_tree(strip, (sym-5)>>1);
    }
    return slot;
  }
  void bump(int32_t slot, uint32_t n) {
    c[slot] += n;
    total += n;
  }

  bool escape_high(int32_t slot) const {
    return 2u*c[slot]>(total&0x7FFFu)+c[0];
  }
};


struct SymPair {
  uint16_t last;
  uint16_t prev;
};



struct BitCtr {
  uint16_t n[2];
  uint16_t limit;
  static const uint32_t kStep = 8;
  static const uint32_t kSeedBump = 4;
  static const uint32_t kSeedLimit = 512;
  static const uint32_t kLimitStep = 64;
  static const uint32_t kMaxLimit = 0x4000;
  static const uint32_t kParentBump = 0x88;
  uint32_t summ() const {
    return n[0]+n[1];
  }

  void init_parent() {
    n[0] = n[1] = kStep/2;
    limit = 72;
  }

  void init_root() {
    n[0] = 40;
    n[1] = 16;
    limit = kSeedLimit;
  }

  int32_t scale_rare() {
    halve_counts(n, 2);
    if( limit<kMaxLimit )
      limit += kLimitStep;
    return limit;
  }

  void seed_from(BitCtr* parent) {
    const int32_t first = n[0]-1;
    const int32_t par0 = parent[0].n[0];
    const int32_t par_tot = par0+parent[0].n[1];
    n[0] = (par_tot+(par0<<6)-64)/par_tot;
    n[1] = ((parent[0].n[1]<<6)+par_tot-64)/par_tot;
    n[first] += kSeedBump;
    limit = kSeedLimit;
    const int32_t par_n = parent[0].n[first];
    parent[0].n[first] = par_n-3*(par_n>3);
  }

  template <int32_t f_DEC> int32_t code_context_bit(BMFState* cx, BitCtr* parent, int32_t bit) {
    int32_t result;
    if( n[0] ) {
      if( !n[1] )
        seed_from(parent);
      const uint32_t tot = summ();
      if constexpr( f_DEC )
        bit = cx[0].rc.decode_bit(n[0], n[1]);
      else
        cx[0].rc.encode_bit(n[0], n[1], bit);
      if( tot>limit )
        scale_rare();
      result = n[bit]+kStep;
      n[bit] = result;
      parent[0].n[bit] += tot<kParentBump;
    } else {
      const uint32_t p_tot = parent[0].summ();
      if constexpr( f_DEC )
        bit = cx[0].rc.decode_bit(parent[0].n[0], parent[0].n[1]);
      else
        cx[0].rc.encode_bit(parent[0].n[0], parent[0].n[1], bit);
      if( p_tot>parent[0].limit )
        parent[0].scale_rare();
      result = parent[0].n[bit]+kStep;
      parent[0].n[bit] = result;
      n[0] = bit+1;
    }
    return f_DEC ? bit : result;
  }

  int32_t encode_context_bit(BMFState* cx, BitCtr* parent, int32_t bit) {
    return code_context_bit<0>(cx, parent, bit);
  }

  int32_t decode_context_bit(BMFState* cx, BitCtr* parent) {
    return code_context_bit<1>(cx, parent, 0);
  }
};

CounterNode*init_counter_node(CounterNode* node) {
  node[0].c[0] = 8;
  node[0].c[1] = 2;
  node[0].c[2] = 2;
  node[0].c[3] = 2;
  node[0].c[4] = 2;
  node[0].c[5] = 3;
  node[0].c[6] = 3;
  node[0].total = 22;
  return node;
}

template <class Unfold> uint8_t fold_or_refuse(uint8_t* dst, int32_t src, int32_t pred, const uint8_t* fold, const uint8_t* fold_hi, const Unfold* unfold) {
  const int32_t resid = (uint8_t)(src-pred);
  const uint8_t recon = unfold[fold[resid]]+pred;
  const int32_t drift = dst[0]-(uint8_t)(recon+dst[0]-src);
  if( drift<-16||drift>16 ) {
    dst[0] = (uint8_t)src;
    return fold_hi[resid];
  }
  dst[0] = recon;
  return fold[resid];
}

struct CtxWeight {
  int32_t sel;
  uint32_t w[3];
};
uint32_t mixer_fwd(const CtxWeight* cw, int32_t n) {
  uint32_t sum = 0;
  for( int32_t k = 0; k<n; ++k )
    sum += cw[k].w[cw[k].sel];
  return sum;
}

uint32_t mixer_rev(const CtxWeight* cw, int32_t n) {
  uint32_t sum = cw[0].w[1];
  for( int32_t k = 1; k<n; ++k )
    sum += cw[k].w[2-cw[k].sel];
  return sum;
}

int32_t tri_sign(int32_t v, int32_t dead = 0) {
  return (v>=-dead)+(v>dead);
}

struct P1Ctx {
  uint8_t sym;
  uint8_t mag;
};


void alt_p1_encode_symbol(BMFState* cx, CounterNode* node, int32_t ctx, int32_t sym);

int32_t alt_p1_decode_symbol(BMFState* cx, CounterNode* node, int32_t ctx);

struct AltP1Block {
  static const int32_t kRowMargin = 4;
  static const int32_t kRowTail = 6;
  int32_t width;
  int32_t height;
  int32_t pred;
  int32_t ctx[5];
  CtxWeight ctx_w[9];
  P1Ctx* buf[5];
  P1Ctx* cursor[5];
  uint8_t level_of[512];
  uint8_t slot_of[256];
  uint8_t fold[256];
  uint8_t fold_hi[256];
  uint8_t unfold[256];
  int32_t group_of[512];
  static const int32_t kP1CounterCount = 629856;
  CounterNode counters[kP1CounterCount];
  void advance_row() {
    {
      P1Ctx*const here = cursor[0];
      for( int32_t k = 0; k<6; ++k )
        here[k] = here[-1-k];
    }
    ring_advance(buf, cursor, kRowMargin);
    cursor[0][-4] = cursor[1][3];
    {
      P1Ctx*const here = cursor[0];
      P1Ctx*const up = cursor[1];
      for( int32_t i = 0; i<3; ++i )
        here[-3+i] = up[2-i];
    }
  }

  void seed_activity(bool signed_mirror) {
    const P1Ctx*const row0 = cursor[0], *const row2 = cursor[2], *const row4 = cursor[4];
    ctx[2] = 0;
    for( int32_t lane = 0; lane<2; ++lane ) {
      const int32_t mirror = lane-2;
      int32_t sum = signed_mirror ? (int8_t)row2[mirror].mag+(int8_t)row4[mirror].mag : row2[mirror].mag+row4[mirror].mag;
      for( int32_t k = 0; k<3; ++k )
        sum += row2[lane+2*k].mag+row4[lane+2*k].mag;
      ctx[3+lane] = sum+row0[lane-4].mag+row0[lane-2].mag;
    }
  }

  int32_t encode_sample(uint8_t* dst, int32_t at, int32_t cur) {
    const int32_t p = (uint8_t)pred;
    const int32_t resid = (uint8_t)(cur-p);
    int32_t code = fold[resid];
    const int32_t recon = (uint8_t)(unfold[code]+p);
    const int32_t out = (uint8_t)(recon+dst[at]-cur);
    if( dst[at]-out<-16||dst[at]-out>16 ) {
      code = fold_hi[resid];
    } else {
      cur = recon;
      dst[at] = out;
    }
    alt_p1_encode_symbol(cx, &counters[ctx[0]], ctx[1], code);
    return cur;
  }

  int32_t decode_sample() {
    const int32_t code = alt_p1_decode_symbol(cx, &counters[ctx[0]], ctx[1]);
    return (uint8_t)((uint8_t)pred+unfold[code]);
  }

  void record_sample(int32_t val) {
    cursor[0][0].sym = val;
    cursor[0][0].mag = abs32(val-pred);
    ctx[3+ctx[2]] += cursor[0][0].mag-cursor[0][-4].mag-(cursor[4][-2].mag-cursor[4][6].mag)-(cursor[2][-2].mag-cursor[2][6].mag);
    ctx[2] = ctx[2]==0;
    if( counters[ctx[0]].total<0x4000u )
      update_model();
    for( int32_t k = 0; k<5; ++k )
      ++cursor[k];
  }

  int32_t cross_grad(int32_t here, int32_t guess, const AltP1Block* nb) {
    const P1Ctx* r = nb[0].cursor[0];
    return here-(uint32_t)guess+r[-1].sym-r[-2].sym;
  }

  int32_t nb_resid(const AltP1Block* nb) {
    return nb[0].cursor[0][-1].sym-nb[0].pred;
  }

  int32_t ctx_of(AltP1Block* nb0, AltP1Block* nb1) {
    bool pick;
    int32_t act_all;
    uint32_t quiet;
    P1Ctx* cursor1 = cursor[1];
    int32_t guess = cursor1[0].sym;
    P1Ctx* cur = cursor[0];
    int32_t west = cur[-1].sym;
    int32_t northwest = cursor1[-1].sym;
    if( west<guess ) {
      if( northwest>=west ) {
        int32_t plane_b = guess+west-northwest;
        pick = northwest<guess;
        guess = cur[-1].sym;
        if( pick )
          guess = plane_b;
      }
    } else if( northwest<=west ) {
      int32_t plane_a = guess+west-northwest;
      pick = northwest<=guess;
      guess = cur[-1].sym;
      if( !pick )
        guess = plane_a;
    }
    pred = guess;
    P1Ctx* cursor2 = cursor[2];
    int32_t act = cursor1[-1].mag+cur[-3].mag+3*(cursor1[1].mag+cursor2[0].mag)+6*cur[-1].mag+4*(cursor1[0].mag+cur[-2].mag)+2*(cursor2[2].mag+cursor1[2].mag+cur[-4].mag);
    const int32_t west_grad = 2*cur[-1].sym-cur[-2].sym-(uint32_t)guess;
    const int32_t north_grad = -guess-cursor1[0].sym+cursor1[1].sym+cur[-1].sym;
    if( nb0 ) {
      const P1Ctx* r0 = nb0[0].cursor[0];
      if( nb1 ) {
        const P1Ctx* r1 = nb1[0].cursor[0];
        act_all = act+2*(r1[-1].mag+r0[-1].mag);
        ctx_w[5].sel = cursor1[0].sym-(uint32_t)guess+r0[-1].sym-nb0[0].cursor[1][-1].sym;
        ctx_w[6].sel = cross_grad(cur[-1].sym, guess, nb0);
        ctx_w[7].sel = cross_grad(cur[-1].sym, guess, nb1);
        ctx_w[8].sel = nb_resid(nb1);
        ctx_w[3].sel = nb_resid(nb0);
        quiet = (r1[-1].mag+r0[-1].mag<16);
      } else {
        act_all = cursor1[3].mag+act+3*r0[-1].mag;
        ctx_w[5].sel = west_grad;
        ctx_w[6].sel = west_grad;
        ctx_w[7].sel = north_grad;
        ctx_w[8].sel = cross_grad(cur[-1].sym, guess, nb0);
        ctx_w[3].sel = nb_resid(nb0);
        quiet = (r0[-1].mag<8);
      }
    } else {
      P1Ctx* cursor4 = cursor[4];
      act_all = cursor4[0].mag+cursor2[-2].mag+cursor1[3].mag+act+cursor4[2].mag;
      ctx_w[5].sel = west_grad;
      ctx_w[6].sel = 2*cursor1[0].sym-cursor2[0].sym-(uint32_t)guess;
      ctx_w[7].sel = north_grad;
      ctx_w[8].sel = -3*(cur[-2].sym-cur[-1].sym)+cur[-3].sym-(uint32_t)guess;
      ctx_w[3].sel = cursor1[2].sym-(uint32_t)guess;
      quiet = cur[0].mag+cursor4[0].mag+cursor[3][0].mag+cursor2[0].mag+cursor1[0].mag==0;
    }
    int32_t act_q = (act_all+7)>>4;
    int32_t act_lvl = level_of[act_q];
    int32_t step = p1_level_step[(uint32_t)act_lvl];
    ctx[0] = act_lvl;
    ctx[1] = group_of[act_q]+slot_of[(uint32_t)guess];
    ctx_w[0].sel = (guess>216)+(guess>22);
    ctx_w[1].sel = tri_sign(cursor1[-1].sym-cursor1[0].sym);
    ctx_w[2].sel = tri_sign(cursor1[-1].sym-cur[-1].sym);
    ctx_w[3].sel = tri_sign(ctx_w[3].sel);
    ctx_w[4].sel = tri_sign(cursor1[1].sym-guess, step);
    ctx_w[5].sel = tri_sign(ctx_w[5].sel, step);
    for( int32_t k = 6; k<=8; ++k )
      ctx_w[k].sel = tri_sign(ctx_w[k].sel);
    int32_t result = CtxIdx{}.bits<0, 3>(act_lvl).bit<3>(ctx[3+ctx[2]]==0).bit<4>(quiet).raw(mixer_fwd(ctx_w, 9));
    ctx[0] = result;
    return result;
  }

  template <int32_t f_DEC> void d8_body(uint8_t* src, uint8_t* out) {
    if constexpr( f_DEC )
      cx[0].rc_begin_decode();
    else
      cx[0].rc_begin_encode();
    for( int32_t y = 0; y<height; ++y ) {
      advance_row();
      seed_activity(false);
      {
        uint8_t* q = out;
        for( int32_t x = 0; x<width; ++x ) {
          ctx_of(nullptr, nullptr);
          if constexpr( f_DEC ) {
            const int32_t code = alt_p1_decode_symbol(cx, &counters[ctx[0]], ctx[1]);
            q[0] = (uint8_t)((uint8_t)pred+unfold[(uint8_t)code]);
          } else {
            const int32_t code = fold_or_refuse(q, *src, (uint8_t)pred, fold, fold_hi, unfold);
            alt_p1_encode_symbol(cx, &counters[ctx[0]], ctx[1], code);
            ++src;
          }
          record_sample((uint8_t)*q);
          ++q;
        }
        out = q;
      }
    }
    if constexpr( f_DEC )
      cx[0].rc_end_decode();
    else
      cx[0].rc_end_encode();
  }

  void update_selector(int32_t k, int32_t slot_f, int32_t slot_r, uint32_t ctx_alt, int32_t alt_lo, int32_t alt_hi) {
    const CtxWeight &cw = ctx_w[k];
    const int32_t sel = cw.sel;
    const int32_t here = ctx[0];
    if( sel==1 ) {
      int32_t mid = here-cw.w[1];
      CounterNode* lo = &counters[mid+cw.w[0]];
      CounterNode* hi = &counters[cw.w[2]+mid];
      lo[0].bump(slot_f, 6);
      hi[0].bump(slot_f, 6);
      if( (here&7)!=7 ) {
        lo[1].bump(slot_f, 4);
        hi[1].bump(slot_f, 4);
      }
      if( here&7 ) {
        lo[-1].bump(slot_f, 3);
        hi[-1].bump(slot_f, 3);
      }
    } else {
      counters[cw.w[2-sel]+here-cw.w[sel]].bump(slot_f, 7);
      CounterNode* mid = &counters[cw.w[1]+here-cw.w[sel]];
      CounterNode* alt = &counters[ctx_alt+cw.w[alt_lo]-cw.w[alt_hi]];
      mid[0].bump(slot_f, 6);
      alt[0].bump(slot_r, 4);
      if( (here&7)!=7 ) {
        mid[1].bump(slot_f, 4);
        alt[1].bump(slot_r, 2);
      }
      if( here&7 ) {
        mid[-1].bump(slot_f, 3);
        alt[-1].bump(slot_r, 2);
      }
    }
  }

  void update_model() {
    const uint32_t code_f = fold[(uint8_t)(cursor[0][0].sym-(uint8_t)pred)];
    const int32_t code_r = fold[(uint8_t)((uint8_t)pred-cursor[0][0].sym)];
    const int32_t tree_sym = (int32_t)(code_f-5)>>1;
    const int32_t slot_f = sym_slot(code_f);
    const int32_t slot_r = sym_slot(code_r);
    uint32_t ctx_alt = CtxIdx{}.bits<0, 5>(ctx[0]).raw(mixer_rev(ctx_w, 9));
    CounterNode* node_alt = &counters[ctx_alt];
    node_alt[0].bump(slot_r, 17);
    const int32_t result = ctx[0];
    if( (result&7)!=7 ) {
      CounterNode* node_up = &counters[result];
      uint32_t ctx_up = ((ctx[1]&7)<7)+ctx[1];
      node_up[1].bump(slot_f, 11);
      if( slot_f>=5 )
        cx[0].update_binary_pair(cx[0].model_strip(CtxIdx{}.bit<7>(slot_f&1).bit<6>(node_up[1].escape_high(slot_f)).raw(ctx_up)), (int32_t)(code_f-5)>>1);
    }
    if( result&7 ) {
      CounterNode* node_dn = &counters[result];
      int32_t ctx_dn = ctx[1]-((ctx[1]&7)!=0);
      node_dn[-1].bump(slot_f, 13);
      if( slot_f>=5 )
        cx[0].update_binary_pair(cx[0].model_strip(CtxIdx{}.bit<7>(slot_f&1).bit<6>(node_dn[-1].escape_high(slot_f)).raw(ctx_dn)), tree_sym);
    }
    if( counters[result].total>=kCounterNudgeLimit )
      return;
    if( (result&7u)<7 ) {
      node_alt[1].bump(slot_r, 7);
    }
    if( result&7 ) {
      node_alt[-1].bump(slot_r, 5);
    }
    if( slot_f>=5 ) {
      const uint32_t key = CtxIdx{}.bit<7>(slot_f&1).bit<6>(counters[result].escape_high(slot_f)).raw(ctx[1]);
      if( (key&0x38)>=0x38||(cx[0].update_binary_pair(cx[0].model_strip(key+8), tree_sym), (key&0x38)!=0) ) {
        cx[0].update_binary_pair(cx[0].model_strip(key-8), tree_sym);
      }
    }
    update_selector(0, slot_f, slot_r, ctx_alt, 0, 1);
    for( int32_t k = 1; k<9; ++k )
      update_selector(k, slot_f, slot_r, ctx_alt, 1, 2-ctx_w[k].sel);
  }

  AltP1Block*alt_p1_alloc(BMFState* state, int32_t img_w, int32_t img_h, int32_t plane) {
    cx = state;
    uint32_t ctr = 0;
    width = img_w;
    height = img_h;
    do
      init_counter_node(&counters[ctr++]);
    while( ctr<kP1CounterCount );
    pred = 0;
    int32_t lvl = 0;
    int32_t grp = 0;
    int32_t k = 0;
    for( k = 0; k<0x100; ++k ) {
      level_of[2*k] = lvl;
      const int32_t lvl1 = (2*k==p1_level_edges[lvl])+lvl;
      group_of[2*k] = CtxIdx{}.above<8>(plane).bits<0, 3>(grp);
      level_of[2*k+1] = lvl1;
      const int32_t odd = 2*k+1;
      const int32_t grp1 = (2*k==p1_group_edges[grp])+grp;
      lvl = (odd==p1_level_edges[lvl1])+lvl1;
      group_of[2*k+1] = CtxIdx{}.above<8>(plane).bits<0, 3>(grp1);
      grp = (odd==p1_group_edges[grp1])+grp1;
    }
    int32_t slot = 0;
    for( uint32_t pair = 0; pair<0x80; ++pair ) {
      slot_of[2*pair] = CtxIdx{}.above<3>(slot);
      const int32_t slot1 = (2*pair==p1_slot_edges[slot])+slot;
      slot_of[2*pair+1] = CtxIdx{}.above<3>(slot1);
      slot = (2*pair+1==p1_slot_edges[slot1])+slot1;
    }
    for( int32_t c = 0, w1 = 32; c<9; ++c, w1 *= 3 ) {
      ctx_w[c].w[0] = 0;
      ctx_w[c].w[1] = w1;
      ctx_w[c].w[2] = 2*w1;
    }
    for( int32_t row = 0; row<5; ++row )
      buf[row] = row_store[row];
    cx[0].alt_init_tables(fold, (int8_t*)unfold);
    for( int32_t r = 0; r<width+10; ++r ) {
      for( int32_t i = 0; i<5; ++i )
        buf[4-i][r].sym = 72;
      for( int32_t i = 0; i<5; ++i )
        buf[4-i][r].mag = 0;
    }
    for( int32_t i = 0; i<5; ++i )
      cursor[i] = buf[i]+width+4;
    return this;
  }

  P1Ctx row_store[5][kMaxWidth+kRowMargin+kRowTail];
  BMFState* cx;
};





struct P2Count {
  int8_t rate;
  uint8_t b1;
  int16_t weighted;
};

struct P2Freq {
  uint16_t step;
  uint16_t f[3];
  uint32_t rescale_three_way() {
    uint32_t cut = f[2]>>1;
    halve_counts(f, 3);
    uint32_t budget = step;
    if( budget<=0x100 ) {
      if( step<=0x20u )
        cut = budget>16 ? 2 : 0;
      else
        cut = 32;
      step = budget-cut;
    } else {
      step >>= 1;
    }
    return cut;
  }

  template <int32_t f_DEC> int32_t code_three_way(BMFState* cx, const uint32_t* ctx_pair, int32_t sym) {
    uint32_t cum;
    uint16_t st, * slot;
    uint16_t down;
    uint16_t* base = &f[0];
    int32_t c01 = f[1]+f[0];
    uint32_t tot = c01+f[2];
    if constexpr( f_DEC ) {
      int32_t target = cx[0].rc.get_freq(tot);
      cum = f[0];
      if( (uint32_t)target>=cum ) {
        if( target>=c01 ) {
          cum = c01;
          slot = &f[2];
        } else {
          slot = &f[1];
        }
      } else {
        cum = 0;
        slot = &f[0];
      }
    } else {
      cum = c01;
      if( sym ) {
        if( sym&1 ) {
          cum = f[0];
          slot = &f[1];
        } else {
          slot = &f[2];
        }
      } else {
        cum = 0;
        slot = &f[0];
      }
    }
    uint32_t tot_1 = cum+slot[0];
    if constexpr( f_DEC )
      cx[0].rc.decode(cum, tot_1, tot);
    else
      cx[0].rc.encode(cum, tot_1, tot);
    uint16_t f_before = slot[0];
    if( *slot>0x4000u ) {
      halve_counts(f, 3);
      st = step;
      if( st<=256 ) {
        if( st<=32 )
          down = st>16 ? 2 : 0;
        else
          down = 32;
        st = st-down;
        step = st;
        f_before = slot[0];
      } else {
        st = (uint32_t)st>>1;
        step = st;
        f_before = slot[0];
      }
    } else {
      st = step;
    }
    slot[0] = st+f_before;
    int32_t idx = slot-base;
    if( idx )
      return idx+2*cx[0].template code_symbol_tree<f_DEC>(cx[0].model_strip(ctx_pair[idx&1]), (sym-1)>>1);
    return 0;
  }

  void encode_symbol(BMFState* cx, const uint32_t* ctx_pair, int32_t sym) {
    code_three_way<0>(cx, ctx_pair, sym);
  }

  int32_t decode_symbol(BMFState* cx, const uint32_t* ctx_pair) {
    return code_three_way<1>(cx, ctx_pair, 0);
  }
};

int32_t p2_pred(int32_t weighted, int32_t rate) {
  return (weighted+(1<<(rate-1)))>>rate;
}

int16_t p2_bump(BMFState* cx, int32_t w2, int32_t err, int32_t shift) {
  const uint32_t kick = 32u*(uint32_t)((err>cx[0].deadzone_hi)-(err<cx[0].deadzone_lo));
  return (int16_t)((uint32_t)w2+((kick+(uint32_t)err+(1u<<(shift-1)))>>shift));
}

void p2_update(BMFState* cx, P2Count* p, int32_t res, int32_t shift) {
  const int32_t w = p[0].weighted;
  p[0].weighted = p2_bump(cx, w, res-p2_pred(w, p[0].rate), shift);
}

void p2_update_into(BMFState* cx, P2Count* p, int32_t &res, int32_t shift) {
  const int32_t w = p[0].weighted;
  res -= p2_pred(w, p[0].rate);
  p[0].weighted = p2_bump(cx, w, res, shift);
}

void p2_rescale(P2Freq* r) {
  if( r[0].f[0]+r[0].f[1]+r[0].f[2]>kP2FreqRescaleTotal ) r[0].rescale_three_way();
}

void p2_freq_add(P2Freq* r, int32_t slot, uint32_t num) {
  p2_rescale(r);
  r[0].f[slot] += (num*(uint32_t)r[0].step)>>4;
}

void p2_bump_sides(P2Freq* group, int32_t ctx15, int32_t lo, int32_t slot, uint32_t up, uint32_t dn) {
  if( ctx15<15 ) p2_freq_add(&group[1], slot, up);
  if( ctx15>lo ) p2_freq_add(&group[-1], slot, dn);
}

void p2_nudge(P2Count* p, int32_t res, int32_t shift = 2) {
  p[0].weighted = (int16_t)((uint16_t)p[0].weighted+((uint32_t)(res-p2_pred(p[0].weighted, p[0].rate)+(1<<(shift-1)))>>shift));
}

struct P2Ctx {
  int16_t val;
  int16_t dval;
  int16_t err;
  int16_t aerr;
  int16_t dleft;
  int16_t dup;
  int16_t dupleft;
  int16_t dupright;
  uint8_t sign;
  uint8_t mag;
};

// NbRow and CtxWeights are mutually recursive: CtxWeights holds NbRow pointers,
// and NbRow::predict blends through a CtxWeights.  One of the two must be
// introduced ahead of the other.
struct NbRow;

struct CtxWeights {
  NbRow* row[6];
};

struct NbRow {
  float w[15][4];
  uint32_t uses;
  uint32_t _pad[3];
  inline int32_t predict(BMFState* cx, float (*nb)[4], CtxWeights* sets, int32_t mode) {
    const float* mix = bmf_p2_mix[mode];
    float mixed[7][4], centre, prediction, own;
    int32_t i, j, k;
    centre = nb_dot(cx[0].p2_coef, nb);
    nb[7][0] = centre;
    for( j = 0; j<7; j++ )
      for( k = 0; k<4; k++ )
        nb[j][k] -= centre;
    for( j = 0; j<7; j++ )
      for( k = 0; k<4; k++ ) {
        mixed[j][k] = mix[0]*sets[0].row[0][0].w[j][k];
        for( i = 1; i<6; i++ )
          mixed[j][k] += mix[i]*sets[0].row[i][0].w[j][k];
      }
    prediction = nb_dot(mixed, nb)+centre;
    nb[7][2] = prediction;
    if( uses ) {
      own = centre+nb_dot(w, nb);
      nb[7][1] = own;
      return (int32_t)(prediction+((own-prediction)*w[14][0])/w[14][1]);
    }
    for( j = 0; j<7; j++ )
      for( k = 0; k<4; k++ ) {
        w[j][k] = mixed[j][k]*bmf_p2_decay;
        w[j+7][k] = sets[0].row[0][0].w[j+7][k]*bmf_p2_seed;
      }
    w[14][0] = 47.0f;
    w[14][1] = 169.2f;
    w[14][2] = 1.0f;
    nb[7][1] = prediction;
    return (int32_t)prediction;
  }
};

struct P2Coef {
  float saved_coef[7][4];
  float saved_rate[3][4];
  void fold(BMFState* cx) {
    memcpy(saved_coef, cx[0].p2_coef, sizeof saved_coef);
    memcpy(saved_rate, cx[0].p2_rate[4], sizeof saved_rate);
    for( int32_t k = 0; k<4; ++k ) {
      for( int32_t i = 0; i<3; ++i )
        cx[0].p2_coef[i][k] += cx[0].p2_coef[4+i][k];
      for( int32_t i = 0; i<3; ++i )
        cx[0].p2_coef[4+i][k] = 0;
      for( int32_t i = 0; i<3; ++i )
        cx[0].p2_rate[4+i][k] = bmf_p2_rate_reset;
    }
  }

  void restore(BMFState* cx) {
    memcpy(cx[0].p2_coef, saved_coef, sizeof saved_coef);
    memcpy(cx[0].p2_rate[4], saved_rate, sizeof saved_rate);
  }
};

int32_t ctx_quant(int32_t run, int32_t r0, int32_t r1, int32_t r2, int32_t sum, int32_t s0, int32_t s1, int32_t s2) {
  return CtxIdx{}.bits<13, 2>((run>r0)+(run>r1)+(run>r2)).bits<11, 2>((sum>s0)+(sum>s1)+(sum>s2));
}

int32_t over_thresholds(int32_t n, const int32_t* t, int32_t lo, int32_t hi, int32_t scale) {
  int32_t over = 0;
  for( int32_t k = hi; k>=lo; --k ) over += n>scale*t[k];
  return over;
}

const int32_t p2_band_edges[5] = {43, 17, 9, 5, 2};

int32_t grad(int32_t a, int32_t b, int32_t c) {
  return a+b-c;
}

template <int16_t P2Ctx::* Field> struct RowOf {
  const P2Ctx* row;
  int32_t operator()(int32_t i) const {
    return row[i].*Field;
  }
};

using DvalRow = RowOf<&P2Ctx::dval>;

using ValRow = RowOf<&P2Ctx::val>;

struct AltP2Block {
  static const int32_t kRowMargin = 8;
  static const int32_t kRowTail = 5;
  NbRow nb_weights[1088];
  float p2_row[7][4];
  float bias[4];
  NbRow* nb_cur;
  NbRow** row0;
  NbRow** row1;
  NbRow** cur;
  NbRow** above;
  int32_t bank_ctx[5];
  int32_t _pad0;
  uint16_t pred_prev;
  uint8_t _pad2[2];
  uint32_t ctx;
  uint32_t ctx_pair[2];
  int32_t nb_id_used;
  int32_t band_lo;
  int32_t band_hi;
  uint32_t plane_idx;
  int32_t has_ref;
  P2Ctx* cursor[5];
  P2Ctx* buf[5];
  CtxWeight ctx_w[5];
  uint8_t _pad1[48];
  int32_t nb_sum[10];
  int32_t ctx_delta[260];
  uint8_t fold[256];
  uint8_t fold_hi[256];
  int8_t unfold[256];
  uint8_t nb_ctx[120];
  int16_t nb_id[1920];
  static const int32_t kP2CounterCount = 5*32768;
  P2Count p2_ctr[kP2CounterCount];

  static uint32_t slot(int32_t bank, int32_t ctx) {
    return CtxIdx{}.bits<0, 15>(ctx).digit<32768, 5>(bank);
  }

  void seat_nb_row(int32_t nb_slot, const P2Ctx* c0) {
    const int32_t at = nb_id[nb_slot];
    if( !at ) {
      nb_id[nb_slot] = ++nb_id_used;

      nb_cur = &nb_weights[nb_id_used];
      return;
    }
    NbRow*const wrow = &nb_weights[at];
    nb_cur = wrow;
    const float err = ((float)c0[-1].val-(nb_dot(wrow[0].w, p2_row)+bias[0]))*2.0999999f;
    const float floor_ = 7744.0f*wrow[0].w[14][2];
    for( int32_t j = 0; j<7; ++j )
      for( int32_t k = 0; k<4; ++k )
        nlms_step(wrow, j, k, p2_row[j][k], err, bmf_p2_ms_rate, floor_);
  }

  int32_t p2_pred_at(int32_t bank, int32_t bctx) {
    const P2Count &c = p2_ctr[slot(bank, bctx)];
    return p2_pred(c.weighted, c.rate);
  }

  int32_t step_bank(int32_t bank_no, uint32_t word, int32_t run_in) {
    const int32_t bank = (int32_t)(word>>11);
    bank_ctx[bank_no] = bank;
    const int32_t pred = p2_pred_at(bank_no, bank);
    nb_sum[2*bank_no+1] = pred;
    const int32_t run = pred+run_in;
    nb_sum[2*bank_no] = run;
    return run;
  }

  static const int32_t kP2FreqCount = 15552;
  P2Freq freq[kP2FreqCount];
  uint8_t _pad3[8];

  void nlms_step(NbRow* row, int32_t j, int32_t k, float x, float err, float ms_rate, float floor_) {
    const float ms = row[0].w[7+j][k]+(x*x-row[0].w[7+j][k])*ms_rate;
    row[0].w[7+j][k] = ms;
    row[0].w[j][k] += cx[0].p2_rate[j][k]*err*x/(ms+floor_);
  }

  void nlms_track_two_rows(NbRow* fast, NbRow* slow, float err_fast, float err_slow, float conf) {
    const float floor_fast = 26896.0f*fast[0].w[14][2];
    const float floor_slow = 5041.0f*slow[0].w[14][2];
    const float rate_slow = 0.013f*conf;
    for( int32_t j = 0; j<7; ++j )
      for( int32_t k = 0; k<4; ++k ) {
        const float x = p2_row[j][k];
        nlms_step(fast, j, k, x, err_fast, 0.05f, floor_fast);
        nlms_step(slow, j, k, x, err_slow, rate_slow, floor_slow);
      }
  }

  void nlms_predict_and_correct(NbRow* row, float sample, float bias0) {
    const float err = sample-(bias0+nb_dot(row[0].w, p2_row));
    const float ms_scale = row[0].w[14][2];
    for( int32_t j = 0; j<7; ++j )
      for( int32_t k = 0; k<4; ++k )
        row[0].w[j][k] += cx[0].p2_rate[j][k]*err*p2_row[j][k]/(row[0].w[7+j][k]+ms_scale*529.0f);
    ++row[0].uses;
    row[0].w[14][2] = ms_scale+((10.0f-ms_scale)*0.00019999999f);
  }

  struct P2RefRows {
    P2Ctx* r0, * r1, * r2;
  };

  struct P2Refs {
    P2RefRows a, b;
  };

  static P2RefRows ref_rows(P2Ctx*const* ring) {
    return {ring[0]-1, ring[1]-1, ring[2]-1};
  }

  P2Refs fill_row_inputs(AltP2Block* refa, AltP2Block* refb) {
    P2Ctx*const c0 = cursor[0], *const c1 = cursor[1], *const c2 = cursor[2], *const c3 = cursor[3], *const c4 = cursor[4];
    const DvalRow d0{c0}, d1{c1}, d2{c2}, d3{c3};
    const ValRow v0{c0}, v1{c1}, v2{c2}, v3{c3}, v4{c4};
    p2_row[0][0] = (float)d1(0);
    p2_row[0][1] = (float)d1(1);
    p2_row[0][2] = (float)grad(d0(-1), d1(0), d1(-1));
    p2_row[0][3] = (float)grad(d0(-2), d1(0), d1(-2));
    p2_row[1][0] = (float)grad(d1(-1), d1(0), d2(-1));
    p2_row[1][1] = (float)(-3*(d2(0)-d1(0))+d3(0));
    p2_row[1][2] = (float)grad(d0(-1), d1(2), d1(1));
    p2_row[1][3] = (float)grad(d0(-2), d1(1), d1(-1));
    p2_row[2][0] = (float)(2*d0(-1)-d0(-2));
    p2_row[2][1] = (float)grad(d0(-3), d1(0), d1(-3));
    p2_row[2][2] = (float)grad(d2(0), d1(1), d3(1));
    p2_row[2][3] = (float)d0(-3);
    p2_row[3][0] = (float)grad(d1(-2), d1(0), d2(-2));
    p2_row[3][1] = (float)grad(d0(-2), d1(-1), d1(-3));
    p2_row[3][2] = (float)grad(d1(1), (d1(2)+d1(0))>>1, d2(2));
    p2_row[3][3] = (float)d3(0);
    if( refa ) {
      const P2Refs refs{ref_rows(refa[0].cursor), ref_rows(refb[0].cursor)};
      P2Ctx*const ra0 = refs.a.r0, *const ra1 = refs.a.r1, *const ra2 = refs.a.r2;
      P2Ctx*const rb0 = refs.b.r0, *const rb1 = refs.b.r1, *const rb2 = refs.b.r2;
      const ValRow a0{ra0}, a1{ra1}, a2{ra2}, b0{rb0}, b1{rb1}, b2{rb2};
      const bool no_ref = has_ref==0;
      if( !no_ref ) {
        const float bias_l = (float)c0[0].dval;
        for( int32_t j = 0; j<4; ++j )
          for( int32_t k = 0; k<4; ++k )
            p2_row[j][k] += bias_l;
      }
      const int32_t plane = (int32_t)plane_idx;
      if( plane ) {
        if( plane==1 ) {
          p2_row[4][0] = (float)(d0(0)+d1(3));
          p2_row[4][1] = (float)grad(v0(-2), a1(2), a1(0));
          p2_row[4][2] = (float)grad(v1(1), a0(-1), a1(0));
          p2_row[4][3] = (float)grad(v0(-1), a1(0), a1(-1));
          p2_row[5][0] = (float)grad(v1(1), b1(2), b2(3));
          p2_row[5][1] = (float)grad(v0(-2), b0(0), b0(-2));
          p2_row[5][2] = (float)grad(v1(0), b0(0), b1(0));
          p2_row[5][3] = (float)(v0(-2)+ra0[0].err);
          if( no_ref ) {
            p2_row[6][0] = (float)grad(v0(-2), a0(0), a0(-2));
            p2_row[6][1] = (float)grad(v2(0), a0(0), a2(0));
            p2_row[6][2] = (float)(grad(a2(0), a0(0), v2(0))+2*(v1(0)-a1(0)));
            p2_row[6][3] = (float)(v0(-1)+v1(-1)+a1(-2)+a0(0)-a0(-1)-a1(-1)-v1(-2));
          } else {
            p2_row[6][0] = (float)v1(0);
            p2_row[6][1] = (float)v0(-3);
            p2_row[6][2] = (float)grad(v0(-1), a0(-1), a0(-2));
            p2_row[6][3] = (float)grad(v0(-3), v0(-1), v0(-4));
          }
        } else {
          p2_row[4][0] = (float)grad(v0(-3), v0(-1), v0(-4));
          p2_row[4][1] = (float)grad(v0(-1), a0(-1), a0(-2));
          p2_row[4][2] = (float)grad(v1(1), a1(0), a2(1));
          p2_row[4][3] = (float)grad(v0(-2), a1(2), a1(0));
          p2_row[5][0] = (float)grad(v1(0), a0(-2), a1(-2));
          p2_row[5][1] = (float)grad(v1(0), a0(0), a1(0));
          p2_row[5][2] = (float)grad(v0(-2), a0(0), a0(-2));
          p2_row[5][3] = (float)grad(v0(-2), b0(0), b0(-2));
          p2_row[6][0] = (float)grad(v2(0), b0(0), b2(0));
          p2_row[6][1] = (float)(grad(b0(-2), b0(0), v0(-2))+2*(v0(-1)-b0(-1)));
          p2_row[6][2] = (float)(v0(-2)+rb0[0].err);
          p2_row[6][3] = (float)(v2(0)+ra0[0].err);
        }
      } else {
        p2_row[4][0] = (float)grad(v0(-3), v0(-1), v0(-4));
        p2_row[4][1] = (float)grad(v0(-5), v1(0), v1(-5));
        p2_row[4][2] = (float)grad(v0(-4), v1(0), v1(-4));
        p2_row[4][3] = (float)((v2(0)+3*v1(1)-4*v2(1))-(((v1(2)-v1(0)-(v3(2)-v3(0)))>>1)-v3(1)));
        p2_row[5][0] = (float)grad(v0(-2), a0(0), a0(-2));
        p2_row[5][1] = (float)grad(v1(1), a1(1), a2(2));
        p2_row[5][2] = (float)grad(v1(-2), a1(2), a2(0));
        p2_row[5][3] = (float)(grad(a0(-2), a0(0), v0(-2))+2*(v0(-1)-a0(-1)));
        p2_row[6][0] = (float)grad(v2(1), b0(0), b2(1));
        p2_row[6][1] = (float)grad(v0(-2), b0(0), b0(-2));
        p2_row[6][2] = (float)grad(v0(-1), b1(1), b1(0));
        p2_row[6][3] = (float)grad(v1(1), b1(2), b2(3));
      }
      return refs;
    } else {
      p2_row[4][0] = (float)grad(v1(3), v1(0), v2(3));
      p2_row[4][1] = (float)grad(v0(-4), v1(0), v1(-4));
      p2_row[4][2] = (float)((v2(0)+3*v1(1)-4*v2(1))-(((v1(2)-v1(0)-(v3(2)-v3(0)))>>1)-v3(1)));
      p2_row[4][3] = (float)grad(v2(-1), v1(1), v3(0));
      p2_row[5][0] = (float)grad(v3(1), v1(0), v4(1));
      p2_row[5][1] = (float)v1(3);
      p2_row[5][2] = (float)grad(v0(-3), v0(-1), v0(-4));
      p2_row[5][3] = (float)grad(v0(-1), v3(0), v3(-1));
      p2_row[6][0] = (float)grad(v0(-5), v1(0), v1(-5));
      p2_row[6][1] = (float)v4(0);
      p2_row[6][2] = (float)grad(v0(-5), v0(-1), v0(-6));
      p2_row[6][3] = (float)v1(-6);
      return {};
    }
  }
  int32_t seat_symbol_context(int32_t run4, const P2Refs &refs) {
    P2Ctx*const c0 = cursor[0], *const c1 = cursor[1], *const c2 = cursor[2], *const c3 = cursor[3], *const c4 = cursor[4];
    int32_t flat_a, flat_b;
    int32_t magsum_own = c1[5].mag+c1[4].mag+c1[-3].mag+c1[-4].mag+3*(c1[2].mag+c2[1].mag)+7*c1[0].mag+6*c1[1].mag+c0[-6].mag+c0[-7].mag+c0[-8].mag+8*c0[-1].mag+c4[2].mag+c4[1].mag+c4[0].mag+c4[-1].mag+c0[-4].mag+c0[-5].mag+(c3[2].mag+c3[1].mag+c3[-1].mag+c3[-2].mag+c4[-2].mag+c2[5].mag+c2[4].mag+c2[3].mag+c2[-2].mag+c2[-3].mag)+4*(c1[-1].mag+c0[-2].mag+c2[0].mag)+2*(c1[-2].mag+c1[3].mag+c0[-3].mag+c3[0].mag+c2[-1].mag+c2[2].mag);
    const int32_t lo_band = band_lo, hi_band = band_hi;
    ctx_w[0].sel = (run4<1840)+(run4<272);
    ctx_w[1].sel = (run4-c1[0].val<=hi_band)+(run4-c1[0].val<lo_band);
    int32_t d_run4 = run4-c0[-1].val;
    bool in_band = d_run4<=hi_band;
    ctx_w[2].sel = in_band+(d_run4<lo_band);
    ctx_w[3].sel = c1[0].sign;
    ctx_w[4].sel = c0[-1].sign;
    int32_t magsum = magsum_own;
    int32_t magu = c2[0].mag+c1[0].mag;
    int32_t magl = c0[-1].mag+c0[-2].mag;
    if( refs.a.r0 ) {
      int32_t mag = refs.b.r0[0].mag;
      int32_t mag_ref1 = refs.b.r1[0].mag+refs.a.r1[0].mag;
      int32_t mag_ref0 = mag+refs.a.r0[0].mag;
      magsum = mag_ref1+magsum_own+4*mag_ref0+2*(refs.a.r0[-1].mag+refs.b.r0[-1].mag);
      flat_a = mag_ref0+magl+refs.b.r0[-2].mag+refs.b.r0[-1].mag+refs.a.r0[-2].mag+refs.a.r0[-1].mag;
      if( has_ref ) {
        const int32_t d_run4b = run4-c1[0].dval-c0[0].dval;
        if( d_run4b<lo_band||d_run4b>hi_band ) {
          const int32_t d_run4c = run4-c0[-1].dval-c0[0].dval;
          flat_b = d_run4c>=lo_band&&hi_band>=d_run4c;
        } else {
          flat_b = 1;
        }
      } else {
        flat_b = mag_ref1+mag_ref0+magu+refs.b.r2[0].mag+refs.a.r2[0].mag;
      }
      if( plane_idx==1 ) {
        int32_t d_ra_ra1 = refs.a.r0[0].val-refs.a.r1[0].val;
        ctx_w[3].sel = (d_ra_ra1<=hi_band)+(d_ra_ra1<lo_band);
        int32_t d_ra_left = refs.a.r0[0].val-refs.a.r0[-1].val;
        ctx_w[4].sel = (d_ra_left<=hi_band)+(d_ra_left<lo_band);
      } else if( (int32_t)plane_idx>1 ) {
        int32_t d_rb_rb1 = refs.b.r0[0].val-refs.b.r1[0].val;
        ctx_w[3].sel = (d_rb_rb1<=hi_band)+(d_rb_rb1<lo_band);
        int32_t d_rb_left = refs.b.r0[0].val-refs.b.r0[-1].val;
        in_band = d_rb_left<=hi_band;
        ctx_w[4].sel = in_band+(d_rb_left<lo_band);
        int32_t ra0_val = refs.a.r0[0].val;
        int32_t d_ra_ra1b = ra0_val-(refs.a.r1)[0].val;
        in_band = lo_band<=d_ra_ra1b;
        if( in_band&&d_ra_ra1b<=hi_band ) {
          flat_a = 1;
        } else {
          int32_t d_ra_leftb = ra0_val-refs.a.r0[-1].val;
          flat_a = d_ra_leftb>=lo_band&&d_ra_leftb<=hi_band;
        }
      }
    } else {
      flat_a = magl+c0[-3].mag+c0[-4].mag+c0[-5].mag;
      flat_b = c4[0].mag+c3[0].mag+magu+c0[0].mag;
    }
    const int32_t ctx15 = magsum>=960 ? 15 : nb_ctx[magsum>>3];
    const int32_t ctx_idx = clamp32((run4+7)>>4, 0, 255);
    ctx_pair[0] = CtxIdx{}.bits<0, 4>(ctx15).raw(ctx_delta[ctx_idx+4]);
    ctx_pair[1] = CtxIdx{}.bits<0, 4>(ctx15).raw(ctx_delta[ctx_idx]);
    ctx = CtxIdx{}.bits<0, 4>(ctx15).bit<4>(flat_a==0).bit<5>(flat_b==0).raw(mixer_fwd(ctx_w, 5));
    return ctx_idx;
  }
  void code_banks(int32_t sample16) {
    for( uint32_t bank = 0; bank<5; ++bank ) {
      const int32_t bctx = bank_ctx[bank];
      const int32_t res = sample16-nb_sum[2*bank];
      P2Count* node0 = &p2_ctr[slot(bank, bctx)];
      const int32_t w0 = res+(uint16_t)node0[0].weighted;
      node0[0].weighted = w0;
      int32_t countdown = node0[0].b1;
      if( !countdown )
        continue;
      const int32_t w0b = w0+4*((res>cx[0].deadzone_hi)-(res<cx[0].deadzone_lo));
      node0[0].weighted = w0b;
      uint32_t ctxw = bctx;
      if( (int32_t)abs32(res)<38 ) {
        --countdown;
        if( !countdown&&(uint8_t)node0[0].rate<8u ) {
          const int32_t from = node0[0].rate;
          node0[0].rate = (uint8_t)(from+1);
          node0[0].b1 = p2_b1_reload[from-5];
          node0[0].weighted = 2*w0b;
        } else {
          node0[0].b1 = countdown;
        }
      }
      if( cx[0].alphabet_reduced )
        continue;
      P2Count* mir = &p2_ctr[slot(bank, ctxw^kBankMirrorMask)];
      __builtin_prefetch(mir, 0, 1);
      const int32_t res2 = nb_sum[2*bank+1]+res;
      int32_t lowbits = ctxw&3;
      if( lowbits<3 )
        p2_update(cx, &node0[1], res2, 1);
      if( lowbits )
        p2_update(cx, &node0[-1], res2, 2);
      P2Count* bankp = &p2_ctr[slot(bank, 0)];
      static const uint32_t p2_bank_bits[11] = {
        0x4000, 0x2000, 0x1000, 0x800, 0x400, 0x200, 0x100, 0x80, 0x40, 0x20, 0x10,
      };
      P2Count* direct[11], * mirror[11], * rot[11];
      for( int32_t k = 0; k<11; ++k ) {
        const uint32_t x = ctxw^p2_bank_bits[k];
        direct[k] = &bankp[x];
        mirror[k] = &bankp[x^kBankMirrorMask];
        rot[k] = &bankp[p2_ctx_rotate[(x>>2)&3]+(x&0xFFFFFFF3)];
        __builtin_prefetch(direct[k], 0, 1);
        __builtin_prefetch(mirror[k], 0, 1);
        __builtin_prefetch(rot[k], 0, 1);
      }
      const int32_t neg = -res2;
      const int32_t w_top = mir[0].weighted;
      const int32_t e_top = neg-p2_pred(w_top, (uint8_t)mir[0].rate);
      const int32_t bump = 32*((e_top>cx[0].deadzone_hi)-(e_top<cx[0].deadzone_lo));
      uint32_t w_new = bump+e_top+2;
      mir[0].weighted = (int16_t)(w_top+(w_new>>2));
      if( lowbits<3 )
        p2_nudge(&mir[1], -res2);
      if( lowbits<=0 ) {
        for( int32_t k = 0; k<11; ++k ) {
          p2_update(cx, direct[k], res2, 2);
          p2_update(cx, rot[k], res2, 3);
          p2_nudge(&direct[k][1], res2);
          p2_update(cx, mirror[k], -res2, 3);
        }
      } else {
        p2_nudge(&mir[-1], -res2, 3);
        p2_update(cx, direct[0], res2, 2);
        p2_update(cx, rot[0], res2, 3);
        walk_bank_bits(direct, mirror, rot, res2, lowbits<3);
        p2_update(cx, mirror[10], -res2, 3);
      }
    }
  }
  void walk_bank_bits(P2Count*const* direct, P2Count*const* mirror, P2Count*const* rot, int32_t res, bool nudge) {
    for( int32_t k = 0; k<11; ++k ) {
      if( k ) {
        p2_update(cx, direct[k], res, 2);
        p2_update(cx, rot[k], res, 3);
      }
      if( nudge )
        p2_nudge(&direct[k][1], res);
      p2_update(cx, &direct[k][-1], res, 3);
      if( k<10 )
        p2_update(cx, mirror[k], -res, 3);
    }
  }

  void bump_bank(int32_t k, uint32_t step_v, int32_t ctx15, int32_t fold_sel, int32_t is_dec) {
    const int32_t sel = ctx_w[k].sel;
    if( sel==1 ) {
      p2_freq_add(&freq[ctx_w[k].w[0]+step_v-ctx_w[k].w[1]], fold_sel, 4);
      const uint32_t off = ctx-ctx_w[k].w[1];
      P2Freq*const group = &freq[off+ctx_w[k].w[0]];
      const uint32_t next_group = ctx_w[k].w[2]+off;
      p2_freq_add(&group[0], is_dec, 3);
      if( ctx15<15 ) {
        p2_freq_add(&group[1], is_dec, 4);
        p2_freq_add(&freq[next_group+1], is_dec, 2);
      }
      if( ctx15>2 )
        p2_freq_add(&group[-1], is_dec, 6);
    } else {
      p2_freq_add(&freq[step_v+ctx_w[k].w[1]-ctx_w[k].w[2-sel]], fold_sel, 7);
      P2Freq*const group = &freq[ctx_w[k].w[1]+(ctx-ctx_w[k].w[sel])];
      p2_freq_add(&group[0], is_dec, 7);
      p2_bump_sides(group, ctx15, 0, is_dec, 5, 6);
    }
  }

  template <int32_t f_DEC> void alt_p2_d8_body(uint8_t* src, uint8_t* out, int32_t width, int32_t height) {
    int32_t j, code0, code;
    uint8_t* outp, code2;
    uint32_t k;
    uint8_t* src_end = src;
    if constexpr( f_DEC )
      cx[0].rc_begin_decode();
    else
      cx[0].rc_begin_encode();
    P2Ctx* pix = cursor[0];
    if( width>0 ) {
      uint8_t* p = src;
      for( j = 0; j<width; ++j ) {
        P2Ctx* rec = cursor[0];
        int32_t q2 = rec[-1].val>>4;
        ctx_pair[0] = ctx+ctx_delta[q2+4];
        ctx_pair[1] = ctx+ctx_delta[q2];
        P2Freq &fr = freq[ctx+ctx_w[3].w[(rec[-1].val<=rec[-2].val)+(rec[-1].val<rec[-2].val)]+ctx_w[2].w[rec[-2].sign]+ctx_w[1].w[rec[-1].sign]+ctx_w[0].w[(q2<115)+(q2<17)]+ctx_w[4].w[1]];
        if constexpr( f_DEC ) {
          out[0] = (uint8_t)(((uint16_t)rec[-1].val>>4)+(uint8_t)unfold[fr.decode_symbol(cx, ctx_pair)]);
        } else {
          code0 = fold_or_refuse(out, *p, (uint16_t)rec[-1].val>>4, fold, fold_hi, unfold);
          fr.encode_symbol(cx, ctx_pair, code0);
          ++p;
        }
        int32_t val = 16*(uint8_t)*out;
        cursor[0][0].val = val;
        cursor[0][0].dval = val;
        ++out;
        int32_t err = (int16_t)(rec[0].val-rec[-1].val);
        rec[0].err = err;
        int32_t adiff = abs32(err);
        cursor[0][0].aerr = adiff;
        cursor[0][0].dupright = adiff;
        cursor[0][0].dupleft = adiff;
        cursor[0][0].dup = adiff;
        cursor[0][0].dleft = (uint32_t)cursor[0][0].dup>>1;
        cursor[0][0].mag = 2;
        cursor[0][0].sign = (cursor[0][0].err<=0)+(cursor[0][0].err<0);
        pix = cursor[0]+1;
        cursor[0] = pix;
      }
      if constexpr( !f_DEC )
        src_end = p;
    }
    seed_history(width);
    if( height>1 ) {
      uint32_t* pair = ctx_pair;
      outp = out;
      for( uint32_t y = 0; y<(uint32_t)(height-1); ++y ) {
        uint8_t* srcp = src_end;
        start_row();
        if( width<=0 )
          continue;
        for( k = 0; k<(uint32_t)width; ++k ) {
          int32_t pred = alt_p2_context(nullptr, nullptr);
          if constexpr( f_DEC ) {
            code2 = code = freq[ctx].decode_symbol(cx, pair);
            outp[k] = (uint8_t)(pred+(unfold[code]));
          } else {
            code = fold_or_refuse(&outp[k], srcp[k], pred, fold, fold_hi, unfold);
            code2 = code;
            freq[ctx].encode_symbol(cx, pair, code);
          }
          alt_p2_model(outp[k], code2, outp[k]-pred);
        }
        if constexpr( !f_DEC )
          src_end = &srcp[width];
        outp = &outp[width];
      }
    }
    if constexpr( f_DEC )
      cx[0].rc_end_decode();
    else
      cx[0].rc_end_encode();
  }
  int32_t alt_p2_context(AltP2Block* refa, AltP2Block* refb) {
    CtxWeights weights;
    int32_t nb_slot;
    uint32_t ctx0, ctx1, ctx2;
    uint32_t ctx0_lo;
    P2Ctx*const c0 = cursor[0], *const c1 = cursor[1], *const c2 = cursor[2], *const c3 = cursor[3], *const c4 = cursor[4];
    int32_t sum_ul = 21*c2[-1].dupleft+12*c1[3].dupleft+16*c1[2].dupleft+22*c1[1].dupleft+(23*c0[-1].dupleft)+20*c1[0].dupleft+cx[0].ctx_bias[0]+14*c0[-2].dupleft;
    int32_t sum_ur = 17*c2[-2].dupright+21*c1[2].dupright+15*c1[1].dupright+25*c1[0].dupright+9*c1[-1].dupright+22*c0[-1].dupright+cx[0].ctx_bias[1]+19*c0[-2].dupright;
    int32_t sum4 = 17*c1[3].dleft+15*c1[2].dleft+21*c1[1].dleft+18*c1[0].dleft+16*c1[-1].dleft+22*c0[-1].dleft+cx[0].ctx_bias[2]+19*c0[-2].dleft;
    int32_t sum_u = 14*c1[3].dup+23*c1[1].dup+19*c1[0].dup+25*c0[-1].dup+cx[0].ctx_bias[3]+17*c0[-2].dup+15*(c3[0].dup+c2[0].dup);
    int32_t sum_all = sum4+sum_u+sum_ul+sum_ur;
    int32_t band = over_thresholds(8*sum4, p2_band_edges, 0, 4, sum_u);
    int32_t den_d = 2*c0[-2].val+2*c0[-1].val;
    int32_t num_b = c1[1].val+(c1[0].val+2*c0[-1].val);
    int32_t num_d = 16*(c2[2].val+c2[0].val+c1[1].val+c2[-1].val);
    int32_t gA = over_thresholds(sum_all, bmf_p2_thresholds[band], 9, 12, 1);
    int32_t gB = over_thresholds(num_b, bmf_p2_thresholds[band], 6, 8, 1);
    int32_t gC = over_thresholds(16*sum_ur, bmf_p2_thresholds[band], 3, 5, sum_ul);
    int32_t gD = over_thresholds(num_d, bmf_p2_thresholds[band], 0, 2, den_d);
    nb_slot = CtxIdx{}.bits<4, 2>(gB).bits<2, 2>(gC).bits<0, 2>(gD).digit<64, 5>(gA).digit<320, 6>(band);
    seat_nb_row(nb_slot, c0);
    const P2Refs refs = fill_row_inputs(refa, refb);
    P2Ctx*const ra0 = refs.a.r0, *const ra1 = refs.a.r1, *const ra2 = refs.a.r2;
    P2Ctx*const rb0 = refs.b.r0, *const rb1 = refs.b.r1, *const rb2 = refs.b.r2;
    weights.row[0] = cur[-1];
    weights.row[1] = above[1];
    weights.row[2] = above[2];
    weights.row[3] = cur[-2];
    weights.row[4] = above[0];
    weights.row[5] = cur[0];
    const int32_t a = 16*sum4, b = 14*sum_u, c = 13*sum_ur, d = 11*sum_ul;
    const int32_t best01 = min32(a, b);
    int32_t mode = (best01==a) ? 0 : 1;
    if( best01>c )
      mode = 2;
    if( min32(best01, c)>d )
      mode = 3;
    const int32_t filt = nb_cur[0].predict(cx, (float (*)[4]) p2_row, &weights, mode);
    pred_prev = (uint16_t)filt;
    int32_t g3pair = c0[-1].aerr+c1[0].aerr;
    if( refa )
      g3pair += (rb0[0].aerr+ra0[0].aerr)>>1;
    int32_t g3sum = c1[3].aerr+c1[-1].aerr+c0[-1].aerr+c0[-3].aerr+c0[-4].aerr+c0[-2].aerr;
    ctx0_lo = CtxIdx{}.bit<19>(c1[1].err+c1[0].err+c1[-1].err+c0[-1].err<0).bit<18>(c3[0].err+c0[-2].err+2*c0[-4].err<0).bit<17>(c1[-3].err+c0[-3].err+c0[-5].err+c0[-7].err<0).bit<16>(c0[-1].err<0).bit_of<15>((uint16_t)c0[-3].err).bits<13, 2>((filt>3536)+(filt>720)+(filt>288)).bits<11, 2>(((g3pair+g3sum)>752)+((g3pair+g3sum)>400)+((g3pair+g3sum)>240));
    if( refa ) {
      ctx0 = CtxIdx{}.bit<25>(rb0[-1].err<0).bit<24>(rb0[0].err<0).bit<23>(ra0[-1].err<0).bit<22>(rb0[0].err+rb0[-2].err<0).bit<21>(ra0[-2].err+ra0[0].err<0).bit<20>(c0[-2].err<0).raw(ctx0_lo);
    } else {
      ctx0 = CtxIdx{}.bit<25>(c1[3].err+c0[-3].err+c0[-7].err+c0[-5].err<0).bit<24>(c2[1].err+c2[0].err+c0[-4].err+c0[-8].err<0).bit<23>(c0[-4].err+c0[-6].err<0).bit<22>(c0[-4].err<0).bit<21>(c0[-5].err<0).bit<20>(c0[-7].err<0).raw(ctx0_lo);
    }
    const int32_t run0 = step_bank(0, ctx0, filt);
    int32_t nb4_4 = c4[4].val;
    sum_all = ((g3pair<<9)+sum_all)>>13;
    int32_t d_run0 = c0[-5].val-run0;
    int32_t d_up = nb4_4-run0;
    int32_t up5 = c1[5].val;
    int32_t d_up5 = up5-run0;
    if( refa ) {
      int32_t dv_now = c0[0].dval;
      ctx1 = CtxIdx{}.bit_of<25>(c0[-1].val+ra0[0].val-ra0[-1].val-run0).bit<24>(ra2[0].dval+ra1[0].dval-2*ra0[0].dval<0).bit_of<23>(dv_now+c0[-3].dval-run0-(c1[2].dval-c1[5].dval)).bit_of<22>(dv_now+c0[-5].dval-run0).bit_of<21>(dv_now+c1[0].dval+rb1[2].dval-rb2[2].dval-run0).bit_of<20>(dv_now+c0[-1].dval+rb0[0].dval-rb0[-1].dval-run0).bit_of<19>(c2[-1].dval+dv_now+ra0[0].dval-ra2[-1].dval-run0).bit_of<18>(dv_now+c1[2].dval+ra0[0].dval-ra1[2].dval-run0).bit_of<17>(d_up5).bit_of<16>(d_up).bit_of<15>(d_run0).raw(ctx_quant(run0, 2256, 1056, 144, sum_all, 55, 10, 24));
    } else {
      int32_t d_run0b = run0-c0[-3].val;
      ctx1 = CtxIdx{}.bit_of<25>(nb4_4-c3[0].val+run0-c1[4].val).bit_of<24>(c1[-5].val-c1[-2].val+d_run0b).bit_of<23>(d_run0b+c1[2].val-up5).bit_of<22>(c4[3].val-c2[2].val+run0-c2[1].val).bit_of<21>(c4[-1].val-c3[0].val+run0-c1[-1].val).bit_of<20>(c1[0].val-c1[2].val+run0-c0[-2].val).bit_of<19>(-d_up5).bit_of<18>(-d_up).bit_of<17>(run0-c4[2].val).bit_of<16>(run0-c4[-3].val).bit_of<15>(-d_run0).raw(ctx_quant(run0, 2400, 1024, 240, sum_all, 39, 24, 11));
    }
    const int32_t run1 = step_bank(1, ctx1, run0);
    int32_t cx2_val0 = c2[0].val;
    int32_t cx1_val = c1[0].val;
    int32_t dvsum2 = c0[-2].val-run1;
    int32_t lap = c0[-2].val+run1-2*c0[-1].val;
    int32_t dtop2 = cx2_val0-run1;
    if( refa ) {
      int32_t dv1 = c1[1].dval;
      int32_t d_ra = ra0[0].val-run1;
      ctx2 = CtxIdx{}.bit_of<25>(c0[0].dval+c1[-1].dval-run1-(c2[0].dval-dv1)).bit_of<24>(c0[0].dval+c0[-1].dval-run1-(c1[0].dval-dv1)).bit_of<23>(rb0[0].val-run1+cx2_val0-rb2[0].val).bit_of<22>(d_ra+cx2_val0-ra2[0].val).bit_of<21>(d_ra+c1[-1].val-ra1[-1].val).bit_of<20>(c0[0].dval-c0[-2].dval+2*c0[-1].dval-run1).bit_of<19>(2*cx1_val-run1-cx2_val0).bit_of<18>(-lap).bit_of<17>(dtop2).bit_of<16>(dvsum2).bit_of<15>(208-rb0[0].val).raw(ctx_quant(run1, 2576, 1280, 640, sum_all, 33, 12, 4));
    } else {
      ctx2 = CtxIdx{}.bit_of<25>(run1+3*(cx2_val0-cx1_val)-c3[0].val).bit_of<24>(run1+c4[0].val-(c2[2].val+c2[-2].val)).bit_of<23>(c3[2].val-c2[-1].val+run1-c1[3].val).bit_of<22>(c3[1].val-c1[1].val-dtop2).bit_of<21>(c3[0].val-cx1_val-dtop2).bit_of<20>(lap).bit_of<19>(run1-c4[3].val).bit_of<18>(c3[1].val-run1).bit_of<17>(run1-c4[0].val).bit_of<16>(run1-c3[-2].val).bit_of<15>(-dvsum2).raw(ctx_quant(run1, 2464, 1216, 688, sum_all, 58, 25, 13));
    }
    const int32_t run2 = step_bank(2, ctx2, run1);
    int32_t run_dv3 = run2-c0[0].dval;
    const uint32_t ctx3 = CtxIdx{}.bit_of<25>(3*(c0[-2].val-c0[-1].val)+run2-c0[-3].val).bit_of<24>(c2[1].dval-((uint32_t)(c1[1].dval+c1[2].dval+c1[-1].dval+c1[0].dval)>>1)+run_dv3).bit_of<23>(c2[-3].val-c1[-2].val+run2-c1[-1].val).bit_of<22>(-(c2[2].val+run2-2*c1[1].val)).bit_of<21>(c1[-2].dval-c0[-2].val+run2-c1[0].dval).bit_of<20>(run2-c1[3].val).bit_of<19>(run_dv3-c2[1].dval).bit_of<18>(run2-c3[0].val).bit_of<17>(2*run2-c1[0].dval-(c1[0].val+c0[0].dval)).bit_of<16>(run2-c0[-1].dval-c0[0].dval).bit_of<15>(run2-c0[-3].val).raw(ctx_quant(run2, 2896, 1568, 592, sum_all, 37, 19, 9));
    const int32_t run3 = step_bank(3, ctx3, run2);
    int32_t dv_now4 = c0[0].dval;
    int32_t run_dv4 = run3-dv_now4;
    int32_t run_up4 = run3+c3[0].val;
    const uint32_t ctx4 = CtxIdx{}.bit_of<25>(run3-((uint32_t)(c1[1].val+c1[-1].val+2*c1[0].val)>>2)).bit_of<24>(run_up4-c0[-2].val-(c3[2].dval+dv_now4)).bit_of<23>(run3-2*c0[-2].dval-(dv_now4-c0[-4].dval)).bit_of<22>(run3-c0[-3].dval-dv_now4-(c1[0].val-c1[-3].val)).bit_of<21>(run_up4-dv_now4-(c1[1].val+c2[-1].dval)).bit_of<20>(c2[-2].val+run3-2*c1[-1].val).bit_of<19>(c2[0].dval-2*c1[0].dval+run_dv4).bit_of<18>((run3-c0[-1].val)-(c1[0].val-c1[-1].val)).bit_of<17>(run_dv4-c1[4].dval).bit_of<16>(run_dv4-c1[-2].dval).bit_of<15>((uint16_t)run3-(uint16_t)c0[-4].dval-(uint16_t)dv_now4).raw(ctx_quant(run3, 3056, 1952, 368, sum_all, 39, 21, 10));
    const int32_t run4 = step_bank(4, ctx4, run3);
    return seat_symbol_context(run4, refs);
  }
  uint32_t alt_p2_model(int32_t sample_in, uint8_t code, int32_t resid_in) {
    int32_t ctx_lo = ctx&0xF;
    int32_t sample16 = 16*sample_in;
    P2Ctx*const c0 = cursor[0];
    P2Ctx*const c1 = cursor[1];
    c0[0].val = 16*sample_in;
    c0[0].dval = c0[0].val-c0[0].dval;
    c0[1].dval = 0;
    const int32_t dead = (ctx_lo>6)+(ctx_lo>4)+2*(ctx_lo>9);
    c0[0].sign = (resid_in<=dead)+(resid_in<-dead);
    c0[0].mag = abs32(resid_in);
    float sample = (float)sample16;
    int32_t dl = sample16-c0[-1].val;
    c0[0].dleft = abs32(dl);
    int32_t du = sample16-c1[0].val;
    c0[0].dup = abs32(du);
    int32_t dul = sample16-c1[-1].val;
    c0[0].dupleft = abs32(dul);
    int32_t dur = sample16-c1[1].val;
    c0[0].dupright = abs32(dur);
    int32_t resid = (int16_t)(sample16-pred_prev);
    c0[0].err = resid;
    c0[0].aerr = abs32(resid);
    NbRow* wrow_cur = nb_cur;
    NbRow* wrow_b = cur[-1];
    const float centre = bias[0];
    const float mixed = bias[1];
    const float own = bias[2];
    const float err_own = sample-own;
    const float d_pred = mixed-own;
    const float cov = (((err_own*d_pred)-wrow_cur[0].w[14][0])*0.001f)+wrow_cur[0].w[14][0];
    const float var = (wrow_cur[0].w[14][1]+0.000099999997f)+(((d_pred*d_pred)-wrow_cur[0].w[14][1])*0.001f);
    wrow_cur[0].w[14][1] = var;
    float cov_kept = 0.1f*var;
    if( cov_kept<=cov )
      cov_kept = fminf(var, cov);
    wrow_cur[0].w[14][0] = cov_kept;
    const float conf = (1.0f-(cov_kept/(var+576.0f)))*2.0f;
    nlms_track_two_rows(wrow_cur, wrow_b, (sample-mixed)*2.5999999f, err_own*conf, conf);
    nlms_predict_and_correct(wrow_cur, sample, centre);
    cur[0] = nb_cur;
    ++cur;
    ++above;
    code_banks(sample16);
    int32_t ctx_l = ctx;
    ++cursor[0];
    P2Freq* frec = &freq[ctx_l];
    ++cursor[1];
    ++cursor[2];
    ++cursor[3];
    ++cursor[4];
    const uint32_t step0 = frec[0].step;
    if( step0<=0x10 )
      return step0;
    int32_t is_dec = code&1;
    int32_t ctx15 = ctx_l&0xF;
    uint32_t pair_ctx = ctx_pair[code&1];
    auto nudge = [&](int32_t at, uint32_t num, uint32_t pair_at) {
                   P2Freq*const bin = &frec[at];
                   p2_rescale(bin);
                   const uint32_t step10 = (num*(uint32_t)frec[at].step)>>4;
                   if( code ) {
                     bin[0].f[2-is_dec] += step10;
                     cx[0].update_binary_pair(cx[0].model_strip(pair_at), (code-1)>>1);
                   } else {
                     bin[0].f[0] += step10;
                   }
                 };
    if( ctx15<15 )
      nudge(1, 10, pair_ctx+1);
    if( ctx15>0 )
      nudge(-1, 13, pair_ctx-1);
    if( freq[ctx].step<=0x1Au )
      return ctx;
    int32_t fold_sel = 2-(fold[(uint8_t)-resid_in]&1);
    if( !fold[(uint8_t)-resid_in] )
      fold_sel = fold[(uint8_t)-resid_in];
    const uint32_t step_ctx = CtxIdx{}.bits<0, 6>(ctx).raw(mixer_rev(ctx_w, 5));
    P2Freq* frec_step = &freq[step_ctx];
    p2_bump_sides(frec_step, ctx15, 0, fold_sel, 4, 4);
    if( code ) {
      int32_t n2_half = (code-1)>>1;
      int32_t hi_nibble = (uint8_t)pair_ctx&0xF0;
      if( hi_nibble>=0xF0||(cx[0].update_binary_pair(cx[0].model_strip(pair_ctx+16), n2_half), hi_nibble>0) ) {
        cx[0].update_binary_pair(cx[0].model_strip(pair_ctx-16), n2_half);
      }
    }
    P2Freq* prec0 = &frec_step[0];
    p2_freq_add(prec0, fold_sel, 6);
    if( !plane_idx||freq[ctx].step>0x100u ) {
      is_dec = code ? 2-is_dec : 0;
      for( int32_t k = 0; k<5; ++k )
        bump_bank(k, step_ctx, ctx15, fold_sel, is_dec);
    }
    return step_ctx;
  }
  void encode_sample(uint8_t* out, int32_t at, AltP2Block* refa, AltP2Block* refb) {
    int32_t sample = out[at];
    const int32_t pred = alt_p2_context(refa, refb);
    const int32_t resid = (uint8_t)(sample-pred);
    int32_t code = fold[resid];
    const int32_t was = out[at];
    const int32_t clean = (uint8_t)(pred+unfold[code]);
    const int32_t recon = (uint8_t)(clean+was-sample);
    if( was-recon<-16||was-recon>16 ) {
      code = fold_hi[resid];
    } else {
      sample = clean;
      out[at] = recon;
    }
    freq[ctx].encode_symbol(cx, ctx_pair, code);
    alt_p2_model(sample, code, sample-pred);
  }

  void decode_sample(uint8_t* out, int32_t at, AltP2Block* refa, AltP2Block* refb) {
    const int32_t pred = alt_p2_context(refa, refb);
    const int32_t code = freq[ctx].decode_symbol(cx, ctx_pair);
    const int32_t sample = (uint8_t)(pred+unfold[code]);
    alt_p2_model(sample, code, sample-pred);
    out[at] = sample;
  }

  template <int32_t f_DEC> void code_sample(uint8_t* out, int32_t at, AltP2Block* refa, AltP2Block* refb) {
    if constexpr( f_DEC )
      decode_sample(out, at, refa, refb);
    else
      encode_sample(out, at, refa, refb);
  }

  void copy_row0_down(int32_t width) {
    const int32_t row_bytes = sizeof(P2Ctx)*(width+kRowMargin+kRowTail);
    for( int32_t i = 0; i<3; ++i ) memcpy(buf[1+i], buf[0], row_bytes);
  }

  void seed_history(int32_t width) {
    P2Ctx*const here = cursor[0];
    for( int32_t i = 0; i<5; ++i ) here[i] = here[-1];
    P2Ctx*const up = cursor[0]-width;
    for( int32_t i = 0; i<8; ++i ) up[-1-i] = up[i];
    copy_row0_down(width);
  }

  void seed_row0(int32_t width) {
    for( int32_t x = 0; x<width+13; ++x ) {
      P2Ctx &rec = buf[0][x];
      rec.val = 256;
      rec.dval = 256;
      rec.err = -16;
      rec.sign = 1;
      rec.mag = 3;
      rec.aerr = 512;
      rec.dupright = 512;
      rec.dupleft = 512;
      rec.dup = 1024;
      rec.dleft = 256;
    }
    copy_row0_down(width);
    for( int32_t i = 0; i<5; ++i ) cursor[i] = buf[i]+width+8;
  }

  void add_bias(int32_t* acc) const {
    const P2Ctx &rec = cursor[0][-1];
    acc[0] += 32*rec.dupleft;
    acc[1] += 32*rec.dupright;
    acc[2] += 32*rec.dleft;
    acc[3] += 32*rec.dup;
  }

  void start_row() {
    NbRow*const last = cur[-1];
    cur[1] = last;
    cur[0] = last;
    NbRow**const was_row0 = row0;
    row0 = row1;
    row1 = was_row0;
    cur = row0+2;
    above = row1+2;
    cur[-1] = above[0];
    cur[-2] = above[0];
    memset(p2_row, 0, 15);
    for( int32_t i = 0; i<4; ++i ) bias[i] = 0.0f;
    memset(p2_row, 0, sizeof p2_row);
    {
      P2Ctx*const here = cursor[0];
      for( int32_t i = 0; i<5; ++i ) here[i] = here[-1-i];
    }
    ring_advance(buf, cursor, kRowMargin);
    cursor[0][-1] = cursor[1][0];
    {
      P2Ctx*const here = cursor[0];
      P2Ctx*const up = cursor[1];
      for( int32_t i = 0; i<7; ++i ) here[-2-i] = up[1+i];
    }
    cursor[0][0].dval = 0;
  }

  AltP2Block*alt_p2_alloc(BMFState* state, int32_t img_w, int32_t plane) {
    cx = state;
    uint32_t done;
    uint32_t j, p, n;
    plane_idx = plane;
    for( j = 0; j<(uint32_t)kP2CounterCount; ++j ) {
      p2_ctr[j].weighted = 0;
      p2_ctr[j].rate = 5;
      p2_ctr[j].b1 = 2;
    }
    for( uint32_t e = 0; e<(uint32_t)kP2FreqCount; ++e ) {
      freq[e].f[0] = 2048;
      freq[e].f[1] = 2816;
      freq[e].f[2] = 2816;
      freq[e].step = 4096;
    }

    int32_t dz = 4*cx[0].near_lossless_q+1;
    int32_t band = 16*cx[0].near_lossless_q;
    has_ref = (cx[0].plane_desc[cx[0].plane_desc[plane_idx].src_plane].flags&desc_has_refs)!=0;
    cx[0].deadzone_hi = dz;
    cx[0].deadzone_lo = -dz;
    band_lo = -band-7;
    band_hi = band+8;
    row0 = row0_store;
    nb_weights[0].w[14][2] = 1.0f;
    row1 = row1_store;
    cur = row0+img_w+2;
    if( img_w>-4 ) {
      uint32_t pairs = (img_w+4)/2;
      if( pairs ) {
        for( p = 0; p<pairs; ++p ) {
          row1[2*p] = &nb_weights[0];
          row0[2*p] = &nb_weights[0];
          row1[2*p+1] = &nb_weights[0];
          row0[2*p+1] = &nb_weights[0];
        }
        done = 2*p+1;
      } else {
        done = 1;
      }
      if( (uint32_t)(img_w+4)>(done-1) ) {
        row1[done-1] = &nb_weights[0];
        row0[done-1] = &nb_weights[0];
      }
    }
    uint32_t row = 0;

    int32_t row_bytes = 18*max32(img_w, 3)+kP2RowPad;
    for( row = 0; row<5; ++row )
      buf[row] = row_store[row];
    memset(buf[0], 0, row_bytes);
    for( int32_t i = 0; i<4; ++i )
      cx[0].ctx_bias[i] = 0;
    int32_t lvl = 0;
    cursor[0] = buf[0]+8;
    for( uint32_t pair = 0; pair<0x82; ++pair ) {
      ctx_delta[2*pair] = CtxIdx{}.above<8>(plane_idx).bits<4, 4>(lvl);
      const int32_t lvl1 = (2*pair==p2_ctx_edges[lvl])+lvl;
      ctx_delta[2*pair+1] = CtxIdx{}.above<8>(plane_idx).bits<4, 4>(lvl1);
      lvl = (2*pair+1==p2_ctx_edges[lvl1])+lvl1;
    }
    int32_t len = 0;
    for( n = 0; n<0x3C; ++n ) {
      nb_ctx[2*n] = len;
      int32_t len1 = (2*n==p2_len_edges[len])+len;
      nb_ctx[2*n+1] = len1;
      len = (2*n+1==p2_len_edges[len1])+len1;
    }
    ctx = 15;
    cx[0].alt_init_tables(fold, unfold);
    for( int32_t c = 0, w1 = 64; c<5; ++c, w1 *= 3 ) {
      ctx_w[c].w[0] = 0;
      ctx_w[c].w[1] = w1;
      ctx_w[c].w[2] = 2*w1;
    }
    return this;
  }

  P2Ctx row_store[5][kMaxWidth+kRowMargin+kRowTail];
  NbRow* row0_store[kMaxWidth+4];
  NbRow* row1_store[kMaxWidth+4];
  BMFState* cx;
};
struct PixRec {
  union {
    struct {
      uint16_t sym;
      uint8_t match[6];
    };
    struct {
      uint32_t head4;
      uint32_t match2345;
    };
  };
};

int32_t match_seed(const PixRec* first, int32_t n, int32_t lane = 0) {
  int32_t hits = 0;
  for( int32_t i = 0; i<n; ++i )
    hits += first[i].match[lane];
  return hits-n;
}

int32_t match_seed_split(const PixRec* at) {
  return at[-3].match[0]+at[-2].match[0]+at[2].match[0]+at[3].match[0]+at[4].match[0]-5;
}

struct Neighbours {
  int32_t up, left, up_next, upleft;
  int32_t rank(int32_t sym) const {
    if( sym==up )
      return 1;
    if( sym==left )
      return 2;
    if( sym==up_next )
      return 3;
    if( sym==upleft )
      return 4;
    return 0;
  }
};

int32_t cum_below(const uint16_t* w, int32_t lvl) {
  switch( lvl ) {
  case 1:
    return w[0];
  case 2:
    return w[0]+w[1];
  case 3:
    return w[0]+w[2]+w[1];
  case 4:
    return w[5]-w[4];
  }
  return 0;
}

struct GroupFolds {
  int32_t w3_double, w4_to_w2, w3_to_w2, w4_to_w1, w3_to_w1, w2_to_w1;
  explicit GroupFolds(int32_t flags) : w3_double(flags&ctx_w3_double), w4_to_w2(flags&ctx_w4_to_w2), w3_to_w2(flags&ctx_w3_to_w2), w4_to_w1(flags&ctx_w4_to_w1), w3_to_w1(flags&ctx_w3_to_w1), w2_to_w1(flags&ctx_w2_to_w1) {
  }
};

void free_sym_lists(SymList*) {
}

struct ReduceNode {
  uint32_t val;
  uint16_t kid[2];
};

struct ModelBlock {
  uint32_t width;
  int32_t height;
  uint32_t depth;
  uint32_t depth_raw;
  uint32_t alphabet;
  uint32_t ctx_id1_used;
  uint32_t ctx_id2_used;
  uint32_t ctx_id3_used;
  uint32_t hit;
  uint32_t ctx_state_seen;
  static const int32_t kRowMargin = 7;
  static const int32_t kRowTail = 9;
  uint32_t bucket_idx;
  uint32_t sym_pos;
  uint32_t ctr_fallback;
  uint32_t ctr_node;
  PixRec* row_cur[10];
  static const int32_t kFreqTableOffset = 188;
  static const int32_t kFreqGridCount = 65723;
  FreqRec grid[kFreqGridCount];
  uint8_t _pad1051664[16];
  BitCtr bit_root[16];
  BitCtr bit_node[4096];
  BitCtr esc_ctr[257];
  BitCtr run_ctr[48];
  uint8_t _pad16[2];
  SymList escape;
  SymList* sel1_list;
  SymList* sel0_list;
  SymList* sel[2];
  SymList* escape_list;
  uint8_t _pad22[4];
  SymList** sel_cur;
  uint16_t* sym_word;
  uint32_t* sym_code;
  uint8_t ctx_state[64];
  uint8_t ctx_bucket[375];
  uint8_t _pad25[1];
  static uint32_t bucket_slot(int32_t group, int32_t last, int32_t prev) {
    return CtxIdx{}.digit<1, 15>(group).digit<15, 5>(last).digit<75, 5>(prev);
  }

  FreqRec*seat_bucket(int32_t group, int32_t last, int32_t prev, int32_t bucket) {
    ctx_bucket[bucket_slot(group, last, prev)] = (uint8_t)bucket;
    return &grid[bucket];
  }

  uint8_t* run_bucket;
  uint8_t* alpha_map;
  uint8_t grad[4];
  uint16_t sym_ctr[524288];
  SymPair group_ctr[15][65536];
  uint16_t* sym_cache;
  SymPair* pix_cur;
  uint16_t sym_rev[8192];
  uint16_t ctx_id1[192512];
  uint16_t ctx_id2[108800];
  uint16_t ctx_id3[712000];
  ModelBlock*layout_workspace(BMFState* state, int32_t img_w, int32_t img_h, int32_t img_depth) {
    cx = state;
    int32_t j;
    uint32_t k, m, s, n;
    cx[0].exclusion_gen = 1;
    width = img_w;
    height = img_h;
    depth = img_depth;
    depth_raw = img_depth;
    escape.ent = nullptr;
    sym_code = nullptr;
    for( j = 0; j<5; ++j ) {
      PixRec* buf = row_store[j];
      row_cur[j] = buf;
      row_cur[j+5] = buf+ModelBlock::kRowMargin+1;
      for( uint32_t r = 0; r<width+(uint32_t)(ModelBlock::kRowMargin+ModelBlock::kRowTail); ++r ) {
        row_cur[j][r].sym = 0;
        for( int32_t i = 0; i<6; ++i )
          row_cur[j][r].match[5-i] = 1;
      }
    }

    uint8_t* runs = run_bucket_store;
    run_bucket = runs;
    runs[0] = 0;
    {
      uint8_t bucket = 0;
      for( uint32_t x = 0; x<width; ++x ) {
        bucket += x==2u<<(bucket&31);
        run_bucket[x+1] = bucket;
      }
    }
    memset(sym_rev, 0, sizeof sym_rev);
    for( k = 0; k<0x2000; ++k ) {
      uint16_t rev = sym_rev[k];
      int32_t bits = k;
      for( m = 0; m<0xD; ++m ) {
        rev += rev+(bits&1);
        bits >>= 1;
      }
      sym_rev[k] = rev;
    }
    for( s = 0; s<0x2000; ++s )
      sym_rev[s] *= 8;
    memset(&grid[188], 0, 0x100000);
    ctx_id3_used = 0;
    ctx_id2_used = 0;
    ctx_id1_used = 0;
    memset(ctx_id1, 255, sizeof ctx_id1);
    memset(ctx_id2, 255, sizeof ctx_id2);
    memset(ctx_id3, 255, sizeof ctx_id3);
    memset(cx[0].exclusion_mask, 0, sizeof cx[0].exclusion_mask);
    sel[0] = nullptr;
    sel[1] = nullptr;
    escape_list = nullptr;
    for( n = 0; n<0x40000; ++n ) {
      sym_ctr[2*n] = no_symbol;
      sym_ctr[2*n+1] = no_symbol;
    }
    for( uint32_t k8 = 0; k8<8; ++k8 ) {
      bit_root[2*k8].init_root();
      bit_root[2*k8+1].init_root();
    }
    memset(bit_node, 0, sizeof bit_node);
    sym_word = (uint16_t*)bmf_new(2*height*width);
    esc_ctr[0].init_parent();
    memset(&esc_ctr[1], 0, 1536);
    for( uint32_t k24 = 0; k24<0x18; ++k24 ) {
      run_ctr[2*k24].init_parent();
      run_ctr[2*k24+1].init_parent();
    }
    return this;
  }
  void free_workspace() {
    free(sym_word);
    free_sym_lists(sel1_list);
    free_sym_lists(sel0_list);
  }
  void reduce_narrow_alphabet(SymList* lists, uint8_t* src, uint32_t mask) {
    int32_t n_distinct, y, bits, shift, prev, s, s_next;
    uint32_t n_syms3, sym_flag[256];
    memset(sym_flag, 0, sizeof sym_flag);
    bool packed = this[0].depth<8;
    int32_t height = this[0].height;
    this[0].alphabet = 0;
    if( packed ) {
      n_distinct = 0;
      if( height ) {
        const uint8_t* p = src-1;
        int32_t at = 0;
        const int32_t bpp = this[0].depth;
        for( y = 0; y<this[0].height; ++y ) {
          bits = 0;
          for( uint32_t x = 0; x<this[0].width; ++x, ++at ) {
            shift = bits-bpp;
            if( shift<0 ) {
              ++p;
              shift = 8-bpp;
            }
            const uint32_t sym = mask&(*p>>shift);
            this[0].alphabet += sym_flag[sym]==0;
            sym_flag[sym] = 1;
            bits = shift;
            this[0].sym_word[at] = sym;
          }
        }
        n_distinct = this[0].alphabet;
      }
    } else if( height&&this[0].width ) {
      const uint8_t* q = src;
      const uint32_t n_pix = this[0].height*this[0].width;
      for( uint32_t idx = 0; idx<n_pix; ++idx ) {
        const int32_t sym2 = *q++;
        this[0].alphabet += sym_flag[sym2]==0;
        sym_flag[sym2] = 1;
        this[0].sym_word[idx] = sym2;
      }
      n_distinct = this[0].alphabet;
    } else {
      n_distinct = 0;
    }
    cx[0].code_alphabet_size<0>(n_distinct, mask+1);
    uint32_t n_syms = this[0].alphabet;
    if( n_syms<=mask ) {
      init_gap_list(cx, lists, mask, n_syms);
      n_syms3 = this[0].alphabet;
      if( n_syms3 ) {
        prev = 0;
        s = 0;
        uint32_t next_id = 0;
        do {
          if( sym_flag[s] ) {
            lists[0].code_symbol(cx, s-prev);
            n_syms3 = this[0].alphabet;
            sym_flag[s] = next_id;
            s_next = s+1;
            prev = s+1;
            ++next_id;
          } else {
            s_next = s+1;
          }
          s = s_next;
        } while( next_id<n_syms3 );
      }
      {
        const uint32_t n_pix = this[0].height*this[0].width;
        for( uint32_t m = 0; m<n_pix; ++m )
          this[0].sym_word[m] = sym_flag[this[0].sym_word[m]];
      }
      free_sym_entries(lists, 16);
    } else {
      free_sym_entries(lists, 16);
    }
  }

  int32_t tree_place(ReduceNode* tree, uint32_t val) {
    int32_t node = 0, side;
    uint16_t* kidp;
    while( 1 ) {
      side = tree[node].val<val;
      kidp = tree[node].kid;
      node = kidp[side];
      if( !node )
        break;
      if( val==tree[node].val ) {
        cx[0].mode_symbol[1] = side;
        return node;
      }
    }
    cx[0].mode_symbol[1] = side;
    node = (uint16_t)alphabet;
    int32_t alpha = alphabet+1;
    kidp[side] = node;
    alphabet = alpha;
    if( alpha>0x2000 )
      return -1;
    tree[node].val = val;
    return node;
  }
  void reduce_alphabet(uint8_t* src) {
    ReduceNode tree[8192];
    SymList lists[16];
    uint8_t* out[19];
    uint32_t alpha_n, done;
    int32_t node, carry;
    uint8_t* rp;
    uint32_t sym_bits = depth;
    uint32_t mask = 0xFFFFFFFF>>(-sym_bits&31);
    uint32_t n_kids = (sym_bits+7)>>3;
    auto load32 = [](const uint8_t* at) {
                    uint32_t v;
                    memcpy(&v, at, sizeof v);
                    return v;
                  };
    clear_sym_lists(lists);
    if( sym_bits<=8 ) {
      reduce_narrow_alphabet(lists, src, mask);
    } else {
      memset(tree, 0, sizeof tree);
      alphabet = 1;
      tree[0].val = mask&load32(src);
      sym_word[0] = 0;
      if( (uint32_t)(height*width)>1 ) {
        uint8_t* p = src;
        node = 0;
        uint32_t written = 1;
        while( 1 ) {
          p += n_kids;
          uint32_t val = mask&load32(p);
          if( val!=tree[node].val ) {
            node = 0;
            if( val!=tree[0].val ) node = tree_place(tree, val);
            if( node<0 ) break;
          }
          sym_word[written++] = node;
          if( written>=(uint32_t)(height*width) ) break;
        }
      }
      alpha_n = alphabet;
      cx[0].code_alphabet_size<0>(alpha_n, no_symbol+1);
      if( alphabet>0x2000 ) {
        out[1] = (uint8_t*)bmf_new(height*n_kids*width);
        int32_t img_w = width;
        int32_t img_h = height;
        uint32_t plane_size;
        if( n_kids ) {
          plane_size = img_h*img_w;
          if( n_kids>>1 ) {
            uint8_t* half = out[1]+img_h*img_w;
            uint32_t pairs = 0;
            for(; pairs<n_kids>>1; ++pairs ) {
              const uint32_t slot_a = 2*pairs;
              const uint32_t off = 2*pairs*plane_size;
              out[slot_a+2] = out[1]+off;
              out[slot_a+3] = &half[off];
            }
            done = 2*pairs+1;
          } else {
            done = 1;
          }
          if( n_kids>(done-1) )
            out[done+1] = out[1]+img_h* -img_w+plane_size*done;
        } else {
          plane_size = img_h*img_w;
        }
        if( plane_size ) {
          rp = src;
          if( n_kids ) {
            for( uint32_t moved = 0; moved<plane_size; ++moved ) {
              for( uint32_t c = 0; c<n_kids; ++c )
                *out[c+2]++ = rp[c];
              rp += n_kids;
            }
          }
        }
        uint16_t* old_words = sym_word;
        height = n_kids*img_h;
        depth = 8;
        free(old_words);
        void* newbuf = bmf_new(2*height*width);
        sym_word = (uint16_t*)newbuf;
        reduce_alphabet(out[1]);
        free(out[1]);
      } else {
        init_byte_lists(cx, lists, n_kids);
        const int32_t alpha_m = alphabet;
        if( alpha_m ) {
          carry = 0;
          for( uint32_t si = 0; si<(uint32_t)alpha_m; ++si )
            lists[0].code_symbol_bytes<0>(cx, n_kids, tree[si].val, carry);
        }
      }
      free_sym_entries(lists, 16);
    }
  }
  int32_t pixel_context(uint32_t* nb) {
    int32_t band;
    int32_t pos = sym_pos;
    uint32_t result = (nb)[pos];
    if( cx[0].exclusion_mask[result]==cx[0].exclusion_gen )
      return -1;
    bool near = false;
    for( band = 11; band<16; ++band )
      near |= result==nb[band];
    bool far = false;
    for( band = 16; band<32; ++band )
      far |= result==nb[band];
    int32_t near_hit = CtxIdx{}.bit<6>(result==nb[10]).bit<5>(near);
    int32_t far_hit = CtxIdx{}.bit<4>(far);
    int32_t ctx0 = far_hit+near_hit;
    if( (far_hit+near_hit==0)&&pos>6 )
      return -1;
    SymList* sel0 = sel0_list;
    int32_t ctx1 = CtxIdx{}.bit<3>(sym_in_top((sel0[cx[0].mode_symbol[1]].ent), 10, result)).raw(ctx0);
    SymEntry* list_prev = sel0[cx[0].mode_symbol[2]].ent;
    SymEntry* list_sym = sel0[result].ent;
    int32_t fallback = CtxIdx{}.bit<2>(sym_in_top(list_prev, 4, result)).bit<1>(sym_in_top(list_sym, 10, cx[0].mode_symbol[2])).bit<0>(sym_in_top((sel0[cx[0].mode_symbol[3]].ent), 6, result));
    int32_t ctx2 = fallback+ctx1;
    if( pos<=14||(ctx2&0xB)!=0 ) {
      ctr_node = CtxIdx{}.above<7>(pos).raw(ctx2);
      ctr_fallback = CtxIdx{}.bit<3>(pos>9).raw(fallback);
    } else {
      return -1;
    }
    return result;
  }

  int32_t init_tables() {
    SymList** cur;
    bool promoted;
    SymList* list, ** prev;
    int32_t recycled, hit1;
    uint32_t hit0 = hit;
    if( !hit0 ) {
      if( sel==sel_cur ) {
        if( sel[0] ) {
          sel0_list[cx[0].mode_symbol[1]].add_weight(row_cur[5][0].sym, 3u);
          sel0_list[row_cur[5][0].sym].add_weight(cx[0].mode_symbol[2], 2u);
          sel1_list[cx[0].mode_symbol[1]].add_weight(row_cur[5][0].sym, 4u);
          sel1_list[row_cur[5][0].sym].add_weight(cx[0].mode_symbol[1], 2u);
        } else {
          sel0_list[cx[0].mode_symbol[2]].add_weight(row_cur[5][0].sym, (sym_pos>3)+2);
        }
      } else {
        sel0_list[cx[0].mode_symbol[1]].add_weight(row_cur[5][0].sym, 3u);
        sel0_list[row_cur[5][0].sym].add_weight(cx[0].mode_symbol[2], 2u);
        sel0_list[row_cur[5][0].sym].add_weight(cx[0].mode_symbol[1], 1u);
        sel1_list[row_cur[5][0].sym].add_weight(cx[0].mode_symbol[1], 2u);
        cur = sel_cur;
        do {
          prev = cur-1;
          sel_cur = prev;
          list = prev[0];
          uint16_t want = row_cur[5][0].sym;
          uint32_t n_live = list[0].live;
          SymEntry* ent = list[0].ent;
          if( n_live==list[0].n ) {
            list[0].live = --n_live;
            recycled = list[0].ent[n_live].cnt;
          } else {
            recycled = 1;
          }
          SymEntry* slot = &ent[n_live];
          list[0].live = n_live+1;
          list[0].tot = recycled+list[0].tot+1;
          slot[0].set(want, 2);
          list[0].since_rescale += 4;
          if( slot!=list[0].ent ) {
            uint16_t sym = slot[0].sym;
            uint8_t cnt = slot[0].cnt;
            slot[0] = slot[-1];
            slot[-1].set(sym, cnt);
          }
          cur = sel_cur;
        } while( cur!=sel );
      }
      if( cx[0].exclusion_gen==-1 ) {
        cx[0].exclusion_gen = 1;
        uint8_t* buf = (uint8_t*)cx[0].exclusion_mask;
        uint32_t blocks = (alphabet+15)>>4;
        do {
          bmf_zero16(buf);
          buf += 16;
          --blocks;
        } while( blocks );
        hit1 = hit;
      } else {
        ++cx[0].exclusion_gen;
        hit1 = hit;
      }
      promoted = hit1&&hit1<=2;
    } else {
      promoted = hit0<=2;
      if( !promoted&&cx[0].mode_symbol[3]!=cx[0].mode_symbol[4] ) {
        sel0_list[cx[0].mode_symbol[2]].add_weight(row_cur[5][0].sym, 1u);
        hit1 = hit;
        promoted = hit1&&hit1<=2;
      }
    }
    if( !promoted ) {
      int32_t just = row_cur[5][0].sym;
      if( just!=sym_cache[0] ) {
        int32_t k = 7;
        for( int32_t i = 1; i<7; ++i )
          if( just==sym_cache[i] ) {
            k = i;
            break;
          }
        for( int32_t i = k; i>0; --i )
          sym_cache[i] = sym_cache[i-1];
        sym_cache[0] = just;
      }
    }
    pix_cur[0].prev = pix_cur[0].last;
    pix_cur[0].last = row_cur[5][0].sym;
    {
      PixRec*const here = row_cur[5];
      PixRec*const up = row_cur[6];
      here[0].match[0] = here[0].sym==up[0].sym;
      here[0].match[1] = here[0].sym==here[-1].sym;
      here[0].match[2] = here[0].sym==up[1].sym;
      here[0].match[3] = here[0].sym==up[-1].sym;
      here[0].match[4] = here[0].sym==up[2].sym;
      here[0].match[5] = here[0].sym==up[3].sym;
    }
    ++row_cur[5];
    ++row_cur[6];
    ++row_cur[7];
    ++row_cur[8];
    ++row_cur[9];
    PixRec*const row = row_cur[5];
    PixRec*const up1 = row_cur[6];
    PixRec*const up2 = row_cur[7];
    grad[0] += up1[4].match[0]-up1[-4].match[0];
    grad[1] += up2[4].match[0]-up2[-4].match[0];
    grad[2] += row[-1].match[1]-row[-5].match[1];
    int32_t result = row[-1].match[0]-row[-8].match[0];
    grad[3] += result;
    return result;
  }

  SymList**load_selectors() {
    sel[0] = &sel0_list[cx[0].mode_symbol[2]];
    sel[1] = &sel1_list[cx[0].mode_symbol[1]];
    return sel_cur;
  }

  void start_row() {
    row_cur[5][0].match[1] = row_cur[5][-1].sym==0;
    row_cur[5][0].match[3] = row_cur[6][-1].sym==0;
    ring_advance(row_cur, row_cur+5, kRowMargin);
    uint8_t zero = row_cur[6][1].sym==0;
    row_cur[5][0].match[2] = zero;
    row_cur[5][-1].match[4] = zero;
    row_cur[5][-2].match[5] = zero;
    zero = row_cur[6][2].sym==0;
    row_cur[5][0].match[4] = zero;
    row_cur[5][-1].match[5] = zero;
    row_cur[5][0].match[5] = row_cur[6][3].sym==0;
  }

  template <int32_t f_DEC> int32_t offer_candidates(int32_t* nb_sym) {
    for( sym_pos = 0; sym_pos<32; ++sym_pos ) {
      const int32_t sym = pixel_context((uint32_t*)nb_sym);
      if( sym<0 )
        continue;
      int32_t is_sym;
      if constexpr( f_DEC ) {
        is_sym = bit_node[ctr_node].decode_context_bit(cx, &bit_root[ctr_fallback]);
        row_cur[5][0].sym = sym;
      } else {
        is_sym = sym==row_cur[5][0].sym;
        bit_node[ctr_node].encode_context_bit(cx, &bit_root[ctr_fallback], is_sym);
      }
      if( is_sym )
        return sym;
      cx[0].exclusion_mask[sym] = cx[0].exclusion_gen;
    }
    return -1;
  }

  int32_t context_ids(int32_t state, int32_t cap) {
    PixRec*const cur6 = row_cur[6];
    PixRec*const r8 = row_cur[8];
    PixRec*const r7 = row_cur[7];
    PixRec*const up9 = row_cur[9];
    PixRec*const row = row_cur[5];
    const int32_t bucket_at = ctx_bucket[state+cap];
    bucket_idx = bucket_at;
    const int32_t up_m0 = cur6[0].match[0];
    const uint32_t nb2 = CtxIdx{}.bit<3>(row[-2].match[2]).bit<2>(row[-2].match[5]).bit<1>(row[-2].match[4]).bit<0>(row[-1].match[1]);
    const uint32_t sig1 = CtxIdx{}.bit<7>(grad[3]==0).bit<6>(grad[2]==0).bit<5>(grad[1]==0).bit<4>(grad[0]==0).bit<9>(up9[0].match[0]&r8[0].match[0]&up_m0&r7[0].match[0]).bit<8>(up9[0].match[1]&r8[0].match[1]&r7[0].match[1]&cur6[0].match[1]).above<10>(bucket_at).raw(nb2);
    int32_t id1;
    intern_ctx(ctx_id1, sig1, ctx_id1_used, id1);
    const uint32_t sig2 = CtxIdx{}.bit<2>(cur6[1].match[3]).bit<1>(up_m0).bit<0>(row[-1].match[5]).above<3>(id1);
    int32_t id2;
    intern_ctx(ctx_id2, sig2, ctx_id2_used, id2);
    return id2;
  }

  static bool intern_ctx(uint16_t* tbl, int32_t sig, uint32_t &used, int32_t &id) {
    const bool minted = tbl[sig]==0xFFFF;
    if( minted )
      tbl[sig] = used++;
    id = tbl[sig];
    return minted;
  }

  int32_t run_scan(const PixRec* up1, const PixRec* up2, int32_t limit, int32_t &m_up0, int32_t &run) {
    m_up0 = up2[0].match[0];
    if( limit<=1 ) {
      run = 1;
      return 1;
    }
    int32_t n = 1;
    while( 1 ) {
      run = n;
      if( !(up1[n+2].match[1]&up1[n+2].match[0]) )
        return n;
      m_up0 = (uint8_t)(up2[run].match[0]&m_up0);
      if( ++n>=limit ) {
        run = n;
        return n;
      }
    }
  }

  void exclude_stage_one() {
    const int8_t gen = cx[0].exclusion_gen;
    for( int32_t k = 1; k<5; ++k )
      cx[0].exclusion_mask[cx[0].mode_symbol[k]] = gen;
    cx[0].exclusion_mask[no_symbol] = gen;
  }

  Neighbours rank_neighbours() {
    const PixRec* up = row_cur[6];
    const PixRec* west = row_cur[5];
    const Neighbours n = {up[0].sym, west[-1].sym, up[1].sym, up[-1].sym};
    cx[0].mode_symbol[1] = n.up;
    cx[0].mode_symbol[2] = n.left;
    cx[0].mode_symbol[3] = n.up_next;
    cx[0].mode_symbol[4] = n.upleft;
    return n;
  }

  uint32_t match_context(const Neighbours &n, const PixRec* up, const PixRec* west) const {
    return CtxIdx{}.bit<5>(west[-1].match[2]).bit<4>(west[-1].match[4]).bit<3>(up[1].match[1]).bit<2>(n.up_next==n.upleft).bit<1>(west[-1].match[0]).bit<0>(up[0].match[1]);
  }

  uint16_t pair_key(const Neighbours &n, const PixRec* west) const {
    if( n.up!=n.left )
      return (uint16_t)(sym_rev[n.up]-n.left);
    if( n.up!=n.up_next )
      return (uint16_t)(sym_rev[n.up]-n.up_next);
    if( n.up!=n.upleft )
      return (uint16_t)(sym_rev[n.up]-n.upleft);
    return (uint16_t)(sym_rev[n.up]-west[-2].sym);
  }

  int32_t open_pixel(Neighbours &nb4, const PixRec* up, const PixRec* west) {
    nb4 = rank_neighbours();
    const int32_t nb = match_context(nb4, up, west);
    const int32_t key = pair_key(nb4, west);
    sym_cache = &sym_ctr[CtxIdx{}.above<3>(key)];
    const int32_t state = ctx_state[nb];
    ctx_state_seen = state;
    SymPair*const pair = &group_ctr[state][key];
    pix_cur = pair;
    const int32_t cap = 15*nb4.rank(pair[0].last)+75*nb4.rank(pair[0].prev);
    int32_t id2 = context_ids(state, cap);
    if( (int32_t)alphabet<32 ) {
      int32_t id3_used = ctx_id3_used;
      int32_t sig3 = CtxIdx{}.bits<0, 4>(nb4.left).above<4>(id2);
      uint16_t* id3p = &ctx_id3[sig3];
      id2 = id3p[0];
      if( id2==0xFFFF ) {
        if( id3_used>kCtxId3Limit )
          sig3 |= 0xF;
        id3p = &ctx_id3[sig3];
        id2 = id3p[0];
      }
      if( id2>=id3_used ) {
        id3p[0] = id3_used;
        ++ctx_id3_used;
        id2 = id3p[0];
      }
    }
    return id2;
  }

  void fill_run(int32_t sym, uint32_t head_word, uint32_t flags_word, uint32_t n) {
    for( uint32_t k = 0; k<n; ++k ) {
      pix_cur[0].prev = sym;
      row_cur[5][0].head4 = head_word;
      row_cur[5][0].match2345 = flags_word;
      ++row_cur[5];
    }
  }

  void seed_after_run(PixRec* cur, int32_t sym) {
    PixRec*const r4 = row_cur[6];
    PixRec*const up3 = row_cur[7];
    grad[0] = match_seed_split(r4);
    grad[1] = match_seed(up3-3, 8);
    grad[2] = match_seed(cur-4, 2, 1);
    grad[3] = match_seed(cur-7, 4);
    cur[-1].match[4] = sym==r4[1].sym;
    row_cur[5][-1].match[5] = sym==row_cur[6][2].sym;
  }

  void seed_candidates(int32_t* nb_sym) {
    nb_sym[0] = pix_cur[0].last;
    nb_sym[1] = pix_cur[0].prev;
    for( int32_t c = 0; c<8; ++c )
      nb_sym[2+c] = sym_cache[c];
    load_neighbours(nb_sym);
  }

  void load_neighbours(int32_t* nb_sym) {
    const PixRec*const r5 = row_cur[5], *const r6 = row_cur[6], *const r7 = row_cur[7], *const r8 = row_cur[8];
    nb_sym[10] = r5[-2].sym;
    nb_sym[11] = r6[2].sym;
    nb_sym[12] = r7[1].sym;
    nb_sym[13] = r7[0].sym;
    nb_sym[14] = r6[-2].sym;
    nb_sym[15] = r7[-1].sym;
    nb_sym[16] = r5[-3].sym;
    nb_sym[17] = r6[3].sym;
    nb_sym[18] = r6[4].sym;
    nb_sym[19] = r5[-4].sym;
    nb_sym[20] = r6[-3].sym;
    nb_sym[21] = r7[2].sym;
    nb_sym[22] = r8[0].sym;
    nb_sym[23] = r7[-2].sym;
    nb_sym[24] = r5[-5].sym;
    nb_sym[25] = r8[1].sym;
    nb_sym[26] = r6[5].sym;
    nb_sym[27] = row_cur[9][0].sym;
    nb_sym[28] = r5[-7].sym;
    nb_sym[29] = r8[-1].sym;
    nb_sym[30] = r6[7].sym;
    nb_sym[31] = r7[3].sym;
  }

  void init_symbol_lists() {
    for( uint32_t s = 0; s<alphabet; ++s ) {
      sel1_list[s].init(kSel1Syms, 0, &sel1_ent[s*kSel1Syms]);
      sel0_list[s].init(kSel0Syms, 0, &sel0_ent[s*kSel0Syms]);
    }
  }

  void seed_alphabet() {
    uint8_t* map = alpha_map_store;
    alpha_map = map;
    memset(map, 1, alphabet);
    escape_list = &escape;
    escape.init(alphabet, 1, escape_ent);
    sel_cur = sel;
    sel1_list = new_sym_lists(alphabet, sel1_lists_store);
    sel0_list = new_sym_lists(alphabet, sel0_lists_store);
    init_symbol_lists();
  }

  void expand_alphabet() {
    SymList lists[16];
    uint32_t n_syms, j;
    uint32_t bits = depth;
    uint32_t mask = 0xFFFFFFFF>>(-(uint8_t)depth&31);
    uint32_t nbytes = (bits+7)>>3;
    clear_sym_lists(lists);
    uint32_t cap = mask+1;
    if( bits>8 )
      cap = no_symbol+1;
    alphabet = cx[0].code_alphabet_size<1>(0, cap);
    if( (int32_t)alphabet<=0x2000 ) {
      void* codes = sym_code_store;
      n_syms = alphabet;
      sym_code = (uint32_t*)codes;
      for( j = 0; j<n_syms; ++j )
        sym_code[j] = j;
      if( (int32_t)depth>8 ) {
        init_byte_lists(cx, lists, nbytes);
        if( n_syms ) {
          int32_t carry = 0;
          for( uint32_t s = 0; s<alphabet; ++s )
            sym_code[s] = lists[0].code_symbol_bytes<1>(cx, nbytes, 0, carry);
        }
      } else if( n_syms<=mask ) {
        init_gap_list(cx, lists, mask, n_syms);
        if( alphabet ) {
          int32_t run = 0;
          for( uint32_t s2 = 0; s2<alphabet; ++s2 ) {
            int32_t gap = lists[0].decode_symbol_list(cx);
            sym_code[s2] = gap+run;
            run += gap+1;
          }
        }
      }
      free_sym_entries(lists, 16);
    } else {
      depth = 8;
      height = height*nbytes;
      expand_alphabet();
      free_sym_entries(lists, 16);
    }
  }

  template <class T> uint8_t*write_row_at(uint8_t* out_at) {
    T* out = (T*)out_at;
    uint32_t i = 0;
    do {
      *out++ = (T)sym_code[row_cur[0][i+8].sym];
      ++i;
    } while( i<width );
    return (uint8_t*)out;
  }

  uint8_t*write_row(uint8_t* out_at, uint32_t nbytes, uint32_t row_w) {
    if( row_w<=0 )
      return out_at;
    if( nbytes==4 )
      return write_row_at<uint32_t>(out_at);
    if( nbytes==3 ) {
      SymEntry* out_ent = (SymEntry*)out_at;
      for( uint32_t step = 0; step<width; ++step, ++out_ent ) {
        const uint32_t code = sym_code[row_cur[0][step+8].sym];
        out_ent[0].set((uint16_t)code, (uint8_t)(code>>16));
      }
      return (uint8_t*)out_ent;
    }
    if( nbytes==2 )
      return write_row_at<uint16_t>(out_at);
    if( depth==8 )
      return write_row_at<uint8_t>(out_at);
    int32_t bits = 0;
    uint8_t* out_bits = out_at;
    const int32_t bpp = depth;
    for( uint32_t x = 0; x<width; ++x ) {
      const uint32_t code = sym_code[row_cur[0][x+8].sym];
      bits -= bpp;
      if( bits<0 ) {
        bits = 8-bpp;
        *++out_bits = code<<(8-bpp);
      } else {
        out_bits[0] |= code<<bits;
      }
    }
    return out_bits;
  }

  template <int32_t f_DEC> void code_row_pixels() {
    for( uint32_t x2 = 0; x2<width; ) {
      x2 += f_DEC ? decode_pixel(x2) : code_pixel(x2);
      init_tables();
    }
  }

  void begin_row() {
    start_row();
    ++row_cur[5];
    ++row_cur[6];
    ++row_cur[7];
    ++row_cur[8];
    ++row_cur[9];
    grad[0] = match_seed(row_cur[6], 5);
    grad[1] = match_seed(row_cur[7], 5);
    grad[2] = 0;
    grad[3] = 0;
  }

  void seed_context_groups() {
    int16_t w1;
    uint16_t w2;
    int32_t w3, w4;
    int32_t bucket = 0;
    for( int32_t g = 0; g<15; ++g ) {
      int32_t flags = ctx_group_flags[g];
      ctx_state[flags] = g;
      const GroupFolds fold(flags);
      for( int32_t lo = 0; lo<5; ++lo ) {
        for( int32_t hi = 0; hi<5; ++hi ) {
          FreqRec* rec = seat_bucket(g, lo, hi, bucket);
          int32_t alpha = alphabet;
          w1 = 2;
          rec[0].w[0] = 2;
          w2 = 2;
          w3 = 2;
          w4 = 2;
          if( fold.w3_double ) {
            w3 = 4;
            w4 = 0;
          }
          if( fold.w4_to_w2 ) {
            w2 = w4+2;
            w4 = 0;
          }
          if( fold.w3_to_w2 ) {
            w2 = w3+w2;
            w3 = 0;
          }
          if( fold.w4_to_w1 ) {
            w1 = w4+2;
            w4 = 0;
            rec[0].w[4] = 0;
          } else {
            rec[0].w[4] = w4;
          }
          if( fold.w3_to_w1 ) {
            w1 += w3;
            w3 = 0;
            rec[0].w[3] = 0;
          } else {
            rec[0].w[3] = w3;
          }
          if( fold.w2_to_w1 ) {
            rec[0].w[1] = w2+w1;
            w2 = 0;
            rec[0].w[2] = 0;
          } else {
            rec[0].w[2] = w2;
            rec[0].w[1] = w1;
          }
          int32_t has3 = w3!=0;
          int32_t has4 = w4!=0;
          if( w2 ) w2 = 1;
          int32_t lvl = has4+has3+w2+2;
          if( lvl<=alpha ) {
            rec[0].b14 = lvl;
          } else {
            rec[0].b14 = has4+has3+w2+1;
            rec[0].w[0] = 0;
          }
          bucket += rec[0].seed_pair(lo, hi, alpha);
        }
      }
      SymPair* gctr = group_ctr[g];
      for( uint32_t gi = 0; gi<0x10000; ++gi ) {
        gctr[gi].last = no_symbol;
        gctr[gi].prev = no_symbol;
      }
    }
  }

  void interleave_depth_bytes(uint8_t* dst, uint8_t* expand_buf) {
    uint8_t* row[19];
    const int32_t n_pix = height*width;
    const int32_t nchunk = ((int32_t)depth_raw+7)>>3;
    if( nchunk>0 ) {
      const int32_t chunk = n_pix/nchunk;
      int32_t q5 = 0;
      uint8_t* row_at = expand_buf;
      for(; q5<=nchunk-6; q5 += 5, row_at += 5*chunk ) {
        row[q5] = row_at;
        for( int32_t n = 0; n<4; ++n )
          row[q5+1+n] = &expand_buf[chunk*(q5+1+n)];
      }
      for( int32_t q1 = nchunk>=6 ? q5 : 0; q1<nchunk; ++q1 )
        row[q1] = &expand_buf[chunk*q1];
    }
    uint8_t* interleave_at = dst;
    int32_t q2 = 0;
    for( int32_t written = 0; written<n_pix; ++written ) {
      uint8_t* p = row[q2];
      *interleave_at++ = p[0];
      row[q2] = p+1;
      if( ++q2==nchunk )
        q2 = 0;
    }
  }

  template <int32_t f_DEC> void code_plane_slow(uint8_t* buf) {
    cx[0].rc_begin<f_DEC>();
    if constexpr( f_DEC )
      expand_alphabet();
    else
      reduce_alphabet(buf);
    seed_context_groups();
    seed_alphabet();
    uint8_t* expand_buf = nullptr;
    uint8_t* out_at = nullptr;
    uint16_t* word = nullptr;
    uint32_t nbytes = 0;
    if constexpr( f_DEC ) {
      uint8_t* dst_buf = &buf[-(depth<8)];
      if( depth!=depth_raw ) {
        dst_buf = (uint8_t*)bmf_new(height*width+3);
        expand_buf = dst_buf;
      }
      nbytes = (depth+7)>>3;
      out_at = dst_buf;
    } else {
      word = sym_word;
    }
    for( int32_t y = 0; y<height; ++y ) {
      begin_row();
      if constexpr( f_DEC ) {
        code_row_pixels<1>();
        out_at = write_row(out_at, nbytes, width);
      } else {
        for( uint32_t x = 0; x<width; ++x )
          row_cur[5][x].sym = word[x];
        word += width;
        code_row_pixels<0>();
      }
    }
    cx[0].rc_end<f_DEC>();
    if constexpr( f_DEC ) {
      if( depth_raw!=depth )
        interleave_depth_bytes(buf, expand_buf);
      free(expand_buf);
    }
  }

  void model_plane_slow(uint8_t* pixels) {
    code_plane_slow<0>(pixels);
  }

  void unmodel_plane_slow(uint8_t* dst) {
    code_plane_slow<1>(dst);
  }

  template <int32_t f_DEC> int32_t code_run_length(int32_t bucket, int32_t cap, int32_t len) {
    const int32_t top = bucket;
    int32_t idx = 0;
    int32_t mask = 1<<(bucket&31);
    int32_t seen = 0;
    do {
      if( (mask|seen)<cap ) {
        BitCtr* ctr = &run_ctr[CtxIdx{}.bits<0, 4>(bucket).digit<16, 3>((seen==0)+(bucket==top))];
        const uint32_t bin_tot = ctr[0].n[0]+ctr[0].n[1];
        int32_t run_bit;
        if constexpr( f_DEC ) {
          run_bit = cx[0].rc.decode_bit(ctr[0].n[0], ctr[0].n[1]);
        } else {
          run_bit = (len&mask)!=0;
          cx[0].rc.encode_bit(ctr[0].n[0], ctr[0].n[1], run_bit);
        }
        if( ctr[0].limit<bin_tot )
          ctr[0].scale_rare();
        ctr[0].n[run_bit] += 8;
        if( run_bit )
          idx |= mask;
        seen |= idx&mask;
      }
      --bucket;
      mask >>= 1;
    } while( mask );
    return idx;
  }

  int32_t decode_stage_two(int32_t* nb_sym, int32_t idx_s) {
    exclude_stage_one();
    sel[0] = nullptr;
    seed_candidates(nb_sym);
    if( offer_candidates<1>(nb_sym)>=0 ) return idx_s+1;
    SymList** sel_p = load_selectors();
    while( 1 ) {
      if( sel_p>&escape_list ) bmf_fatal(bmf_read_error);
      if( (sel_p[0])[0].live ) {
        int32_t lsym = (sel_p[0])[0].decode_symbol_list(cx);
        row_cur[5][0].sym = lsym;
        if( lsym>=0 ) return idx_s+1;
        sel_p = sel_cur;
      }
      sel_cur = ++sel_p;
    }
  }

  int32_t decode_pixel(int32_t x) {
    alignas(16) int32_t nb_sym[32];
    uint32_t lvl_a;
    int32_t m_up0, run, bit, idx_s;
    PixRec* cur6 = row_cur[6];
    PixRec* row = row_cur[5];
    Neighbours nb4;
    int32_t id2 = open_pixel(nb4, cur6, row);
    if( row_cur[5][-1].match[1]&row_cur[5][-1].match[0] ) {
      PixRec* up1 = row_cur[6];
      PixRec* up2 = row_cur[7];
      if( (uint8_t)(up2[2].match[1]&up2[1].match[1]&up2[0].match[1]&up1[3].match[1]&up1[2].match[1]&up1[1].match[1]&up1[0].match[1]&up1[0].match[0]&(int8_t)up1[-1].match[1])&up2[3].match[1] ) {
        const int32_t idx1 = run_scan(up1, up2, width-x, m_up0, run);
        int32_t bucket = run_bucket[idx1];
        int32_t esc_at = CtxIdx{}.above<3>(bucket).bit<2>(up2[run+3].match[1]&up2[run+2].match[1]).bit<1>(m_up0).bit<0>(alpha_map[nb4.up]).raw(1);
        bit = esc_ctr[esc_at].decode_context_bit(cx, esc_ctr);
        int32_t msym1c = cx[0].mode_symbol[1];
        hit = bit;
        *(((uint8_t*)alpha_map)+msym1c) = bit;
        const int32_t hit_a = hit;
        idx_s = hit_a ? idx1 : idx1>1 ? code_run_length<1>(bucket, idx1, 0) : 0;
        if( idx_s )
          row_cur[5][idx_s-1].sym = row_cur[5][-1].sym;
        if( idx_s>hit_a ) {
          row_cur[6] = row_cur[6]+idx_s-hit_a;
          PixRec* r8b = row_cur[8];
          PixRec* r7b = row_cur[7]+idx_s;
          row_cur[7] = r7b-hit_a;
          row_cur[8] = r8b+idx_s-hit_a;
          PixRec* rec = row_cur[5];
          row_cur[9] = row_cur[9]+idx_s-hit_a;
          rec[0].match2345 = 0x01010101;
          row_cur[5][0].head4 = 0x01010101;
          SymPair* pixp = pix_cur;
          const int32_t run_sym = cx[0].mode_symbol[1];
          pixp[0].prev = pixp[0].last;
          row_cur[5][0].sym = (uint16_t)run_sym;
          pix_cur[0].last = (uint16_t)run_sym;
          const int32_t flags_word = row_cur[5][0].match2345;
          const int32_t head_word = row_cur[5][0].head4;
          ++row_cur[5];
          fill_run(run_sym, head_word, flags_word, idx_s-hit_a-1);
          seed_after_run(row_cur[5], run_sym);
        }
        if( hit_a )
          return idx_s;
        return decode_stage_two(nb_sym, idx_s);
      }
    }
    FreqRec* freq = &grid[bucket_idx];
    FreqRec* freq_tbl = &grid[id2+kFreqTableOffset];
    {
      auto decode_against = [&](FreqRec* rec) {
                              const int32_t total = rec[0].w[5];
                              int32_t cum;
                              const int32_t lvl = rec[0].find_level(cx[0].rc.get_freq(total), cum);
                              const int32_t low = cum-rec[0].w[lvl];
                              rec[0].bump(lvl);
                              cx[0].rc.decode(low, cum, total);
                              hit = lvl;
                              return lvl;
                            };
      const int32_t tot = freq_tbl[0].w[5];
      if( tot ) {
        if( tot==1 )
          freq_tbl[0].blend_from(freq[0]);
        lvl_a = decode_against(freq_tbl);
        if( freq_tbl[0].b14 ) {
          --freq_tbl[0].b14;
          ++freq[0].w[5];
          ++freq[0].w[lvl_a];
        }
      } else {
        lvl_a = decode_against(freq);
        freq_tbl[0].w[5] = freq_tbl[0].w[lvl_a]++!=0;
      }
    }
    if( lvl_a ) {
      row_cur[5][0].sym = cx[0].mode_symbol[lvl_a];
      return 1;
    }
    return decode_stage_two(nb_sym, 0);
  }

  int32_t code_pixel(int32_t x) {
    alignas(16) int32_t nb_sym[32];
    uint32_t done;
    int32_t m_up0, run, amap, runlen, lvl_a;
    PixRec* cur6c, * up2;
    PixRec* cur6 = row_cur[6];
    PixRec* cur5 = row_cur[5];
    Neighbours nb4;
    int32_t id2 = open_pixel(nb4, cur6, cur5);
    PixRec* cur2 = cur5;
    PixRec* row = row_cur[5];
    int32_t m_w1b = row[-1].match[1];
    int32_t m_w0 = row[-1].match[0];
    PixRec* rowp = row;
    if( (m_w1b&m_w0)!=0&&(cur6c = row_cur[6], up2 = row_cur[7], ((uint8_t)(up2[2].match[1]&up2[1].match[1]&up2[0].match[1]&cur6c[3].match[1]&cur6c[2].match[1]&cur6c[1].match[1]&cur6c[0].match[1]&cur6c[0].match[0]&cur6c[-1].match[1])&up2[3].match[1])!=0) ) {
      const int32_t run_len = run_scan(cur6c, up2, width-x, m_up0, run);
      int32_t run_pair = (uint8_t)(up2[run+3].match[1]&up2[run+2].match[1]);
      const int32_t bucket = run_bucket[run_len];
      amap = CtxIdx{}.bit<2>(run_pair).bit<1>(m_up0).above<3>(bucket).bit<0>(alpha_map[nb4.up]);
      runlen = 0;
      if( rowp[0].sym==nb4.up ) {
        do ++runlen; while( runlen<run_len&&rowp[runlen].sym==nb4.up );
      }
      const int32_t run_hit = runlen==run_len;
      if( runlen>run_hit ) {
        row_cur[6] = &cur6c[runlen-run_hit];
        row_cur[7] = &up2[runlen-run_hit];
        PixRec* row_cur9 = row_cur[9];
        row_cur[8] = row_cur[8]+runlen-run_hit;
        row_cur[9] = row_cur9+runlen-run_hit;
        rowp[0].match2345 = 0x01010101;
        row_cur[5][0].head4 = 0x01010101;
        pix_cur[0].prev = pix_cur[0].last;
        uint16_t wp = nb4.up;
        row_cur[5][0].sym = wp;
        pix_cur[0].last = wp;
        PixRec* r3 = row_cur[5];
        uint32_t rec_word = *(uint32_t*)r3;
        const int32_t flag_word = r3[0].match2345;
        cur2 = r3+1;
        row_cur[5] = r3+1;
        if( runlen-run_hit!=1 ) {
          int32_t run_left = runlen-run_hit-1;
          uint32_t half = run_left/2;
          if( run_left/2 ) {
            fill_run(nb4.up, rec_word, flag_word, 2*half);
            cur2 = row_cur[5];
            done = 2*half+1;
          } else {
            done = 1;
          }
          if( (uint32_t)run_left>(done-1) ) {
            pix_cur[0].prev = nb4.up;
            row_cur[5][0].head4 = rec_word;
            row_cur[5][0].match2345 = flag_word;
            PixRec* cur5p1b = row_cur[5]+1;
            row_cur[5] = cur5p1b;
            cur2 = cur5p1b;
          }
        }
        seed_after_run(cur2, nb4.up);
      }
      (&esc_ctr[(amap+1)])[0].encode_context_bit(cx, esc_ctr, run_hit);
      int32_t msym1 = cx[0].mode_symbol[1];
      uint8_t* amap_at = (uint8_t*)alpha_map;
      hit = run_hit;
      amap_at[msym1] = run_hit;
      if( !run_hit&&run_len!=1 )
        code_run_length<0>(bucket, run_len, runlen);
      if( hit )
        return runlen;
    } else {
      FreqRec* binp = &grid[bucket_idx];
      FreqRec* frec = &grid[id2+kFreqTableOffset];
      auto code_against = [&](FreqRec* rec) {
                            const int32_t lvl = nb4.rank(rowp[0].sym);
                            const int32_t cum = cum_below(rec[0].w, lvl);
                            const uint32_t total = rec[0].w[5];
                            const uint32_t high = rec[0].w[lvl]+cum;
                            rec[0].bump(lvl);
                            cx[0].rc.encode(cum, high, total);
                            hit = lvl;
                            return lvl;
                          };
      const uint32_t grid_kind = frec[0].w[5];
      if( grid_kind ) {
        if( grid_kind==1 ) {
          frec[0].blend_from(binp[0]);
          rowp = row_cur[5];
        }
        lvl_a = code_against(frec);
        if( frec[0].b14 ) {
          --frec[0].b14;
          ++binp[0].w[5];
          ++binp[0].w[lvl_a];
        }
      } else {
        lvl_a = code_against(binp);
        frec[0].w[5] = frec[0].w[lvl_a]++!=0;
      }
      if( lvl_a ) return 1;
      runlen = 0;
    }
    exclude_stage_one();
    sel[0] = nullptr;
    seed_candidates(nb_sym);
    if( offer_candidates<0>(nb_sym)>=0 ) return runlen+1;
    SymList** sel_p = load_selectors();
    while( 1 ) {
      if( (sel_p[0])[0].live ) {
        if( (sel_p[0])[0].code_symbol(cx, row_cur[5][0].sym) ) return runlen+1;
        sel_p = sel_cur;
      }
      sel_cur = ++sel_p;
    }
  }

  static const int32_t kSel1Syms = 99;
  static const int32_t kSel0Syms = 33;
  SymEntry sel1_ent[(no_symbol+1)*kSel1Syms];
  SymEntry sel0_ent[(no_symbol+1)*kSel0Syms];
  SymEntry escape_ent[no_symbol+1];
  alignas(8) uint8_t sel1_lists_store[SymListBlock::bytes(no_symbol+1)];
  alignas(8) uint8_t sel0_lists_store[SymListBlock::bytes(no_symbol+1)];
  uint8_t alpha_map_store[no_symbol+1];
  uint32_t sym_code_store[no_symbol+1];
  uint8_t run_bucket_store[kMaxWidth+1];
  PixRec row_store[5][kMaxWidth+kRowMargin+kRowTail];
  BMFState* cx;
};





enum : uint8_t {
  depth_bits = 0x3F,
  depth_grey = 0x40,
  depth_palette = 0x80,
  depth_one_plane = depth_grey|8,
};

enum : uint8_t {
  flags_transposed = 0x02,
  flags_slow = 0x04,
  flags_planar = 0x08,
  flags_descriptors = 0x10,
  flags_coded = 0x20,
  flags_packed = 0x40,
  flags_tail = 0x80,
};

uint32_t bmp_palette_entry(uint8_t b, uint8_t g, uint8_t r) {
  return (uint32_t)r<<16|(uint32_t)g<<8|b;
}

void put_u32(uint8_t* at, uint32_t v) {
  memcpy(at, &v, sizeof v);
}

uint32_t bmp_grey_entry(uint8_t level) {
  return bmp_palette_entry(level, level, level);
}

struct BmfImage {
  uint16_t width;
  uint16_t height;
  uint16_t stride;
  uint8_t _pad8[4];
  uint8_t depth;
  uint8_t flags;
  uint32_t data_size;
  uint8_t pixels[0];
  uint8_t*palette() {
    return pixels+data_size;
  }

  const uint8_t*palette() const {
    return pixels+data_size;
  }

  bool holds(const uint8_t* at, int32_t n) const {
    return at>=pixels&&at+n<=pixels+data_size;
  }

  uint8_t*row(int32_t y) {
    return pixels+y*stride;
  }

  void header_from(const BmfImage* src) {
    width = src[0].width;
    height = src[0].height;
    stride = src[0].stride;
    for( int32_t k = 0; k<4; ++k )
      _pad8[k] = src[0]._pad8[k];
    depth = src[0].depth;
    data_size = src[0].data_size;
  }
};

void transpose_image_in_place(BmfImage* img, int32_t planes) {
  uint16_t w = img[0].width;
  img[0].width = img[0].height;
  img[0].height = w;
  img[0].flags ^= flags_transposed;
  img[0].stride = img[0].width*planes;
}

void transpose_image(BmfImage* img, int32_t planes) {
  uint8_t* copy = (uint8_t*)bmf_new(img[0].data_size);
  memcpy(copy, img[0].pixels, img[0].data_size);
  const int32_t step = planes*(img[0].height-1);
  const uint8_t* src = copy;
  for( int32_t y = 0; y<img[0].height; ++y ) {
    uint8_t* dst = img[0].pixels+planes*y;
    for( int32_t x = 0; x<img[0].width; ++x ) {
      for( int32_t k = 0; k<planes; ++k )
        *dst++ = *src++;
      dst += step;
    }
  }
  transpose_image_in_place(img, planes);
  free(copy);
}

BmfImage*alloc_image(int32_t img_w, int32_t img_h, int32_t bpp, int32_t palette, int32_t packed) {
  int32_t row_pack, row_bytes, pal_bytes;
  bool sub_byte_rows = false;
  uint16_t row16 = img_w;
  int32_t bits = bpp;
  if( bpp>=5&&bpp<=7 ) {
    row_bytes = row16;
  } else {
    if( bpp==3 )
      bits = 4;
    else if( !packed )
      bits = max32(bpp, 4);
    bool byte_rows = bpp!=3&&bits>=8;
    if( !packed ) {
      if( !byte_rows&&bits==4 )
        row16 = ((img_w+7)>>1)&0xFFFC;
      else
        row16 = (uint16_t)(((bits+7)>>3)*img_w);
      row_bytes = row16;
    } else if( byte_rows ) {
      row16 = (uint16_t)(((bits+7)>>3)*img_w);
      row_bytes = row16;
    } else {
      if( bits==1 ) {
        row_pack = (int32_t)(((uint32_t)((img_w+7)>>2)>>29)+img_w+7)>>3;
      } else if( bits==2 ) {
        row_pack = (int32_t)(((uint32_t)((img_w+3)>>1)>>30)+img_w+3)>>2;
      } else {
        row_pack = (int32_t)(img_w+((uint32_t)(img_w+1)>>31)+1)>>1;
      }
      sub_byte_rows = true;
      row_bytes = (uint16_t)row_pack;
    }
  }
  int32_t data_bytes = row_bytes*img_h;
  bool has_palette = false;
  if( palette&&bpp<=8 ) {
    pal_bytes = 3<<(bpp&31);
    has_palette = true;
  } else {
    pal_bytes = 0;
  }
  BmfImage* img = (BmfImage*)bmf_new(data_bytes+pal_bytes+19);
  if( !img )
    return nullptr;
  img[0].width = img_w;
  img[0].height = img_h;
  img[0].stride = row_bytes;
  for( int32_t k = 0; k<4; ++k )
    img[0]._pad8[k] = 0;
  img[0].depth = (uint8_t)(bpp|(has_palette ? depth_palette : 0));
  img[0].flags = sub_byte_rows ? flags_packed : 0;
  img[0].data_size = data_bytes;
  if( pal_bytes )
    memset(img[0].palette(), 0, pal_bytes);
  return img;
}

enum : uint32_t {
  bmp_magic = 'B'|'M'<<8,
  bmp_hdr_v3 = 40,
  bmp_rgb = 0,
  bmp_rle8 = 1,
  bmp_rle4 = 2,
};

#pragma pack(push, 1)
struct BmpHeader {
  uint16_t bfType;
  uint32_t bfSize;
  uint16_t bfReserved1;
  uint16_t bfReserved2;
  uint32_t bfOffBits;
  uint32_t biSize;
  int32_t biWidth;
  int32_t biHeight;
  uint16_t biPlanes;
  uint16_t biBitCount;
  uint32_t biCompression;
  uint32_t biSizeImage;
  int32_t biXPelsPerMeter;
  int32_t biYPelsPerMeter;
  uint32_t biClrUsed;
  uint32_t biClrImportant;
};
#pragma pack(pop)

int32_t bmf_pixels(const BmfImage* img) {
  return img[0].width*img[0].height;
}

enum : uint16_t {
  bmf_sig_image = 0x8A81,
  bmf_sig_other = 0x9081,
};
constexpr int32_t bmf_version_20 = 2<<8;
int32_t bmf_tag_version(uint32_t tag) {
  return (((uint8_t)(tag>>16)-'0')<<8)|((uint8_t)(tag>>24)-'0');
}

uint32_t bmf_tag(uint16_t sig, char major, char minor) {
  return sig|((uint32_t)(uint8_t)major<<16)|((uint32_t)(uint8_t)minor<<24);
}

struct BmfStream {
  FILE* fp = nullptr;
  uint8_t* buf = nullptr;
  size_t len = 0;
  size_t cap = 0;
  size_t pos = 0;
  bool ran_off = false;
  static BmfStream over_file(FILE* f) {
    BmfStream s;
    s.fp = f;
    return s;
  }

  static BmfStream over_memory(const void* data, size_t n) {
    BmfStream s;
    s.buf = (uint8_t*)data;
    s.len = n;
    return s;
  }

  static BmfStream in_memory() {
    BmfStream s;
    s.cap = 1;
    s.buf = (uint8_t*)bmf_new(1);
    return s;
  }

  size_t read(void* dst, size_t n) {
    if( fp ) return fread(dst, 1u, n, fp);
    const size_t got = pos>=len ? 0 : (n<len-pos ? n : len-pos);
    memcpy(dst, buf+pos, got);
    pos += got;
    if( got<n ) ran_off = true;
    return got;
  }

  size_t write(const void* src, size_t n) {
    if( fp ) return fwrite(src, 1u, n, fp);
    if( !cap ) return 0;
    if( pos+n>cap ) {
      size_t want = cap;
      while( want<pos+n ) want *= 2;
      uint8_t* wider = (uint8_t*)bmf_new(want);
      memcpy(wider, buf, len);
      free(buf);
      buf = wider;
      cap = want;
    }
    memcpy(buf+pos, src, n);
    pos += n;
    if( pos>len ) len = pos;
    return n;
  }

  bool read_exact(void* dst, size_t n) {
    return n==0||read(dst, n)==n;
  }

  bool write_exact(const void* src, size_t n) {
    return n==0||write(src, n)==n;
  }

  void write_all(const void* src, size_t n) {
    if( !write_exact(src, n) ) bmf_fatal(bmf_write_error);
  }

  bool seek(long off, int32_t whence) {
    if( fp ) return fseek(fp, off, whence)==0;
    long at = whence==SEEK_SET ? 0 : whence==SEEK_END ? (long)len : (long)pos;
    if( at+off<0||(size_t)(at+off)>len ) return false;
    pos = (size_t)(at+off);
    return true;
  }

  bool eof() const {
    return fp ? feof(fp)!=0 : ran_off;
  }

  void flush() {
    if( fp ) fflush(fp);
  }

  uint32_t bytes_left() {
    if( !fp ) return (uint32_t)(len-pos);
    const long here = ftell(fp);
    if( here<0||fseek(fp, 0, SEEK_END)!=0 ) return 0;
    const long end = ftell(fp);
    if( end<0||fseek(fp, here, SEEK_SET)!=0 ) return 0;
    return (uint32_t)(end-here);
  }

  uint8_t*take(size_t* out_len) {
    uint8_t* out = buf;
    out_len[0] = len;
    buf = nullptr;
    cap = len = pos = 0;
    return out;
  }

  void close() {
    if( fp ) {
      fclose(fp);
      fp = nullptr;
    } else if( cap ) {
      free(buf);
      buf = nullptr;
      cap = len = pos = 0;
    }
  }
};

struct BmfFile {
  BmfStream io;
  BmfImage*fail() {
    close();
    return nullptr;
  }

  void close() {
    io.close();
  }
};

void bmf_close_file(BmfFile* arc) {
  arc[0].close();
  free(arc);
}

bool read_exact(void* dst, size_t n, FILE* fp) {
  return n==0||fread(dst, n, 1u, fp)==1;
}

BmfFile*bmf_open_file(BmfFile* out, char* path, int32_t read_only) {
  FILE* fp = fopen(path, read_only ? "rb" : "wb");
  if( !fp ) bmf_fatal(bmf_no_open, path);
  out[0].io = BmfStream::over_file(fp);
  return out;
}
void alt_p1_encode_symbol(BMFState* cx, CounterNode* node, int32_t ctx, int32_t sym) {
  node[0].code_symbol<0>(cx, ctx, sym);
}

int32_t alt_p1_decode_symbol(BMFState* cx, CounterNode* node, int32_t ctx) {
  return node[0].code_symbol<1>(cx, ctx, 0);
}
int32_t estimate_cost(const int32_t* bin, int32_t n) {
  double sum_even = 0.0, sum_odd = 0.0, ent_even = 0.0, ent_odd = 0.0;
  int32_t i;
  for( i = 0; i+1<n; i += 2 ) {
    if( bin[i] ) {
      sum_even += (double)bin[i];
      ent_even += (double)bin[i]*log((double)bin[i]);
    }
    if( bin[i+1] ) {
      sum_odd += (double)bin[i+1];
      ent_odd += (double)bin[i+1]*log((double)bin[i+1]);
    }
  }
  double entropy = ent_even+ent_odd;
  double total = sum_even+sum_odd;
  for(; i<n; i++ )
    if( bin[i] ) {
      total += (double)bin[i];
      entropy += (double)bin[i]*log((double)bin[i]);
    }
  if( total!=0.0 )
    total = total*log(total);
  return (int32_t)((total-entropy)*1.442695040888963);
}
uint32_t plane_mix2(const PlaneDesc &d, const uint8_t* p0, const uint8_t* p1) {
  return (d.weight0**p0+d.weight1*(uint32_t)*p1+40)>>7;
}

int32_t plane_mix3(const PlaneDesc &d, const uint8_t* p) {
  return (d.weight1*p[-2]+d.weight0*p[-3]+d.weight2*p[-1]+64)>>7;
}
uint8_t med_predict(int32_t west, int32_t north, int32_t northwest) {
  const int32_t lo = min32(west, north), hi = max32(west, north);
  if( northwest>=hi )
    return (uint8_t)lo;
  if( northwest<=lo )
    return (uint8_t)hi;
  return (uint8_t)(west+north-northwest);
}

void med_fold_table(uint8_t* fold) {
  for( int32_t j = 0; j<128; ++j ) {
    fold[j] = (uint8_t)(2*j);
    fold[128+j] = (uint8_t)(-1-2*j);
  }
}

void med_unfold_table(uint8_t* unfold) {
  unfold[0] = 0;
  for( int32_t j = 0; j<127; ++j ) {
    unfold[2*j+1] = (uint8_t)(-1-j);
    unfold[2*j+2] = (uint8_t)(1+j);
  }
  unfold[255] = (uint8_t)(-1-127);
}
void strided_copy(uint8_t* dst, int32_t dstep, const uint8_t* src, int32_t sstep, int32_t n) {
  if( dstep==1&&sstep==1 ) {
    memcpy(dst, src, n);
    return;
  }
  for( int32_t i = 0; i<n; ++i, dst += dstep, src += sstep )
    *dst = src[0];
}
struct PlaneTransform {
  int32_t to_ref0, to_ref1, mode, dc, wgt0, wgt1, wgt2;
  bool by_weights;
  uint32_t blend(const uint8_t* p) const {
    if( mode==2 )
      return (wgt0*(uint32_t)p[to_ref0]+wgt1*(uint32_t)p[to_ref1]+40)>>7;
    return (wgt1*(uint32_t)p[-2]+wgt0*(uint32_t)p[-3]+wgt2*(uint32_t)p[-1]+63)>>7;
  }
};
uint8_t*unpredict_med(uint8_t* pixels, int32_t width, int32_t height) {
  uint32_t done, k, x_left;
  alignas(16) uint8_t unfold[256];
  uint8_t* p = pixels+1;
  med_unfold_table(unfold);
  int32_t rows_left = height-1;
  if( width!=1 ) {
    uint32_t row_rest = width-1;
    uint32_t pairs = row_rest/2;
    if( pairs ) {
      for( k = 0; k<pairs; ++k ) {
        uint8_t cur = pixels[2*k]+unfold[pixels[2*k+1]];
        pixels[2*k+1] = cur;
        pixels[2*k+2] = cur+(unfold[pixels[2*k+2]]);
        p = &pixels[2*k+3];
      }
      done = 2*k+1;
    } else {
      done = 1;
    }
    if( row_rest>done-1 ) {
      pixels[done] = pixels[done-1]+unfold[pixels[done]];
      p = &pixels[done+1];
    }
  }
  if( height==1 )
    return p;
  if( width==1 ) {
    for(; rows_left; --rows_left ) {
      p[0] = unfold[(uint8_t)*p]+p[-width];
      ++p;
    }
    return p;
  }
  uint8_t* up = &p[-width];
  for(; rows_left; --rows_left ) {
    ++up;
    p[0] = unfold[(uint8_t)*p]+p[-width];
    ++p;
    for( x_left = width-1; x_left; --x_left ) {
      const uint8_t pred = med_predict(p[-1], (uint8_t)*up, p[-width-1]);
      p[0] = pred+unfold[(uint8_t)*p];
      ++up;
      ++p;
    }
  }
  return p;
}
struct HistRows {
  int32_t* r0, * r1, * r2, * r3, * r4, * r5;
};

HistRows hist_rows(int32_t* hists) {
  return {hists, hists+1*1024, hists+2*1024, hists+3*1024, hists+4*1024, hists+5*1024};
}

int32_t residual_bin(int32_t own, int32_t w0, int32_t a, int32_t w1, uint32_t b) {
  return ((uint16_t)own-(uint16_t)((w0*a+w1*b+40)>>7)-256)&0x1FF;
}
int32_t widest_window(const int32_t* hist, int32_t first, int32_t end, int32_t miss) {
  int32_t sum = 0;
  for( int32_t i = 0; i<256; ++i ) sum += hist[first+i];
  int32_t best = sum, at = miss;
  for( int32_t w = first+256; w<end; ++w ) {
    sum += hist[w]-hist[w-256];
    if( sum>=best ) {
      at = w;
      best = sum;
    }
  }
  return at;
}



struct AlphaWeights {
  int32_t a, b, c;
};

AlphaWeights fit_alpha_weights(const uint8_t* px, const uint8_t* pp, uint32_t uu) {
  double Sxx = 0.0, Sxy = 0.0, Sxz = 0.0;
  double Syy = 0.0, Syz = 0.0, Szz = 0.0;
  double Sxw = 0.0, Syw = 0.0, Szw = 0.0;
  for( uint32_t i4 = 0; i4<uu; ++i4 ) {
    const double dv0 = (double)(px[4*i4]+pp[4*i4+4]-(px[4*i4+4]+pp[4*i4]));
    const double dv1 = (double)(px[4*i4+1]+pp[4*i4+5]-(px[4*i4+5]+pp[4*i4+1]));
    const double dv2 = (double)(px[4*i4+2]+pp[4*i4+6]-(px[4*i4+6]+pp[4*i4+2]));
    const int32_t dv3 = px[4*i4+3]+pp[4*i4+7]-(px[4*i4+7]+pp[4*i4+3]);
    Sxx += dv0*dv0;
    Sxy += dv0*dv1;
    Sxz += dv0*dv2;
    Syy += dv1*dv1;
    Syz += dv1*dv2;
    Szz += dv2*dv2;
    Sxw += dv0*(double)dv3;
    Syw += dv1*(double)dv3;
    Szw += dv2*(double)dv3;
  }
  double d1 = Sxx*Szz-Sxz*Sxz;
  double d2 = 0.0-Sxx*Syz+Sxz*Sxy;
  double d3 = Sxz*Syz-Sxy*Szz;
  double inv = 128.0/(Syy*d1+Syz*d2+Sxy*d3+0.1);
  const int32_t wa = clamp_weight((int32_t)(((Sxz*Syz-Sxy*Szz)*Syw+(0.0-Sxz*Syy+Sxy*Syz)*Szw+(Szz*Syy-Syz*Syz)*Sxw)*inv));
  const int32_t wb = clamp_weight((int32_t)((d1*Syw+d2*Szw+d3*Sxw)*inv));
  const int32_t wc = clamp_weight((int32_t)(inv*((Sxx*Szw-Sxz*Sxw)*Syy+(0.0-Sxx*Syw+Sxw*Sxy)*Syz+(Sxz*Syw-Sxy*Szw)*Sxy)));
  return {wa, wb, wc};
}
enum : int32_t {
  try_mode0 = pred_mode0,
  try_p1 = pred_p1|desc_alt_model,
  try_p2 = pred_p2|desc_alt_model,
  try_refs = desc_has_refs,
  try_refs_p1 = desc_has_refs|pred_p1|desc_alt_model,
  try_refs_p2 = desc_has_refs|pred_p2|desc_alt_model,
};
struct PlaneSearch {
  int32_t bits;
  int32_t n_p1;
  int32_t n_p2;
  int32_t n_refs;
};
bool read_rle_op(FILE* fp, int32_t &n, int32_t &v) {
  if( ferror(fp) ) return false;
  n = fgetc(fp);
  v = fgetc(fp);
  return n>=0&&v>=0;
}

uint8_t*write_nibbles(uint8_t* at, int32_t &hi, const uint8_t* src, uint32_t n) {
  while( 1 ) {
    const uint8_t pix = src[0];
    const uint32_t left = n-1;
    if( hi ) {
      if( !left ) {
        at[0] = src[0]&0xF0;
        hi = 0;
        return at;
      }
      *at++ = pix;
    } else {
      *at++ |= (uint8_t)*src>>4;
      if( !left ) {
        hi = 1;
        return at;
      }
      at[0] = 16*(pix&0xF);
    }
    ++src;
    n = left-1;
    if( !n )
      return at;
  }
}

bool read_rle8(FILE* fp, BmfImage* img, uint8_t* at, void* pal_buf) {
  int32_t y = img[0].height-1;
  int32_t n, v;
  while( read_rle_op(fp, n, v) ) {
    if( n ) {
      if( !img[0].holds(at, n) )
        return false;
      memset(at, v, n);
      at += n;
    } else if( !v ) {
      if( --y<0 )
        return true;
      at = img[0].row(y);
    } else if( v==1 ) {
      return true;
    } else if( v==2 ) {
      int32_t dx, dy;
      if( !read_rle_op(fp, dx, dy) )
        return false;
      at += dx-dy*img[0].stride;
    } else {
      if( !img[0].holds(at, v) )
        return false;
      if( !read_exact(pal_buf, (v+1)&0xFFFFFFFE, fp) )
        return false;
      memcpy(at, (uint8_t*)pal_buf, v);
      at += v;
    }
  }
  return false;
}

bool read_rle4(FILE* fp, BmfImage* img, uint8_t* row, void* pal_buf) {
  int32_t hi_nibble = 1;
  int32_t y = img[0].height-1;
  int32_t run4, byte_in;
  while( read_rle_op(fp, run4, byte_in) ) {
    const uint32_t byte = (uint32_t)byte_in;
    if( run4 ) {
      if( !img[0].holds(row, run4/2+1) )
        return false;
      uint8_t lo = byte&0xF;
      if( hi_nibble ) {
        int32_t left4b = run4;
        uint8_t* row3 = row;
        while( left4b>1 ) {
          *row3++ = byte;
          left4b -= 2;
        }
        row = row3;
        hi_nibble = left4b==0;
        if( left4b )
          *row3 = byte&0xF0;
      } else {
        int32_t left4 = run4;
        uint8_t* row4 = row;
        uint8_t cur = row[0];
        uint32_t hi = byte>>4;
        uint8_t lo16 = 16*lo;
        do {
          *row4++ = hi|cur;
          cur = lo16;
          left4 -= 2;
        } while( left4>0 );
        row = row4;
        hi_nibble = left4<0;
        if( !hi_nibble )
          *row4 = lo16;
      }
    } else if( !byte ) {
      if( --y<0 )
        return true;
      row = img[0].row(y);
    } else if( byte==1 ) {
      return true;
    } else if( byte==2 ) {
      int32_t dxy, dy4;
      if( !read_rle_op(fp, dxy, dy4) )
        return false;
      int32_t step = (dxy>>1)-dy4*img[0].stride;
      if( (dxy&1)==1 ) {
        if( !hi_nibble )
          ++step;
        hi_nibble = !hi_nibble;
      }
      row += step;
    } else {
      if( !img[0].holds(row, (int32_t)byte/2+1) )
        return false;
      if( !read_exact(pal_buf, (((byte+1)>>1)+1)&0xFFFFFFFE, fp) )
        return false;
      row = write_nibbles(row, hi_nibble, (const uint8_t*)pal_buf, byte);
    }
  }
  return false;
}

bool read_rows(FILE* fp, BmfImage* img, uint8_t* row, uint32_t stride_pad) {
  const uint32_t stride = img[0].stride;
  const uint32_t row_pad = stride_pad-stride;
  for( int32_t y = img[0].height-1; y>=0; --y ) {
    if( fread(row, 1u, stride, fp)!=stride )
      return false;
    if( row_pad )
      fseek(fp, row_pad, 1);
    row -= stride;
  }
  return true;
}

BmfImage*read_bmp(char* path) {
  uint8_t bmp_bgra[4];
  int32_t pal_n, i;
  BmpHeader hdr;
  FILE* fp = fopen(path, "rb");
  if( !fp )
    bmf_fatal(bmf_no_open, path);
  if( fread(&hdr.bfType, 0xEu, 1u, fp)!=1||hdr.bfType!=bmp_magic||fread(&hdr.biSize, 0x28u, 1u, fp)!=1||hdr.biSize!=bmp_hdr_v3||hdr.biPlanes!=1 ) {
    fclose(fp);
    return nullptr;
  }
  {
    const int32_t bmp_w = hdr.biWidth;
    const int32_t bmp_h = hdr.biHeight;
    const int32_t bmp_bpp = hdr.biBitCount;
    if( bmp_w<=0||bmp_w>0xFFFF||bmp_h<=0||bmp_h>0xFFFF||(bmp_bpp>=8&&(uint32_t)bmp_w*(uint32_t)((bmp_bpp+7)>>3)>0xFFFFu)||(bmp_bpp!=1&&bmp_bpp!=4&&bmp_bpp!=8&&bmp_bpp!=24&&bmp_bpp!=32)||(int32_t)hdr.biClrUsed<0||(bmp_bpp<=8&&(int32_t)hdr.biClrUsed>(1<<bmp_bpp))||hdr.biCompression>bmp_rle4||(hdr.biCompression==bmp_rle8&&bmp_bpp!=8)||(hdr.biCompression==bmp_rle4&&bmp_bpp!=4) ) {
      fclose(fp);
      return nullptr;
    }
  }
  BmfImage* img = alloc_image(hdr.biWidth, hdr.biHeight, hdr.biBitCount, hdr.biBitCount<=8u, 1);
  if( !img ) {
    fclose(fp);
    return nullptr;
  }
  uint32_t stride_pad = (img[0].stride+3)&0xFFFFFFFC;
  if( hdr.biBitCount<=8u ) {
    pal_n = 1<<(hdr.biBitCount&31);
    if( (int32_t)hdr.biClrUsed )
      pal_n = (int32_t)hdr.biClrUsed;
    if( pal_n>0 ) {
      uint8_t* pal = img[0].depth&depth_palette ? img[0].pixels+img[0].data_size : nullptr;
      for( i = 0; i<pal_n; ++i ) {
        if( !read_exact(bmp_bgra, 4u, fp) ) {
          fclose(fp);
          return nullptr;
        }
        pal[3*i+2] = bmp_bgra[2];
        pal[3*i+1] = bmp_bgra[1];
        pal[3*i] = bmp_bgra[0];
      }
    }
  }
  static uint8_t pal_store[65536];
  void* pal_buf = pal_store;

  uint8_t* row = img[0].pixels+img[0].data_size-img[0].stride;
  fseek(fp, (int32_t)hdr.bfOffBits, 0);
  bool ok;
  if( hdr.biCompression==bmp_rgb ) {
    ok = read_rows(fp, img, row, stride_pad);
  } else {
    memset(img[0].pixels, 0, img[0].data_size);
    ok = hdr.biCompression==bmp_rle8 ? read_rle8(fp, img, row, pal_buf) : hdr.biCompression==bmp_rle4 ? read_rle4(fp, img, row, pal_buf) : false;
  }
  if( !ok )
    return nullptr;
  fclose(fp);
  return img;
}

int32_t write_bmp_palette(const BmfImage* img, uint8_t* out_buf, uint8_t depth, int32_t bits) {
  if( bits>8 )
    return 0;
  const int32_t ncol = 1<<(bits&31);
  if( depth&depth_grey ) {
    uint32_t levels = 0x100u>>(bits&31);
    uint32_t k = 0;
    int32_t grey = 0;
    for(; k<(uint32_t)(ncol/2); ++k, grey += 2*levels ) {
      put_u32(&out_buf[8*k+54], bmp_grey_entry((uint8_t)grey));
      put_u32(&out_buf[8*k+58], bmp_grey_entry((uint8_t)(grey+levels)));
    }
    const uint32_t at = 2*k+1;
    if( (at-1)<(uint32_t)ncol )
      put_u32(&out_buf[4*at+50], bmp_grey_entry((uint8_t)(levels*(at-1))));
    return 4*ncol;
  } else if( depth&depth_palette ) {
    uint32_t pairs = ncol/2;
    uint32_t j = 0;
    const uint8_t* pal = img[0].palette();
    for(; j<pairs; ++j ) {
      const uint32_t slot = 2*j;
      const uint8_t* e0 = &pal[6*j], * e1 = &pal[6*j+3];
      put_u32(&out_buf[4*slot+54], bmp_palette_entry(e0[0], e0[1], e0[2]));
      put_u32(&out_buf[4*slot+58], bmp_palette_entry(e1[0], e1[1], e1[2]));
    }
    const uint32_t at2 = 2*j+1;
    if( (at2-1)<(uint32_t)ncol ) {
      const uint8_t* e = img[0].palette()+3*at2-3;
      put_u32(&out_buf[4*at2+50], bmp_palette_entry(e[0], e[1], e[2]));
    }
    return 4*ncol;
  } else {
    return 4*ncol;
    memset(out_buf+54, 0, 4*ncol);
  }
}

uint8_t*bmp_rle_encode(BmfImage* img, int32_t bits, uint8_t* buf, int32_t rows, uint32_t stride, int32_t* kind) {
  if( bits!=4&&bits!=8 )
    return nullptr;
  const int32_t nib = bits==4;
  const int32_t run_max = (0x100u>>(nib&31))-1;
  kind[0] = nib ? bmp_rle4 : bmp_rle8;
  uint8_t* out_at = buf, * q = nullptr, * end = nullptr;
  uint8_t* p = &img[0].pixels[img[0].data_size-stride];
  uint8_t byte = 0;
  int32_t lit_len = 0, run = 0;
  auto flush_literal = [&](const uint8_t* run_end) {
                         if( nib ? lit_len!=1 : lit_len>=3 ) {
                           out_at[0] = 0;
                           out_at[1] = lit_len<<(nib&31);
                           memcpy(out_at+2, run_end-lit_len, lit_len);
                           if( lit_len&1 )
                             out_at[lit_len+2] = 0;
                           out_at += lit_len+2+(lit_len&1);
                         } else if( nib ) {
                           out_at[0] = 2;
                           out_at[1] = run_end[-1];
                           out_at += 2;
                         } else {
                           if( lit_len==2 ) {
                             out_at[0] = 1;
                             out_at[1] = run_end[-2];
                             out_at += 2;
                           }
                           out_at[0] = 1;
                           out_at[1] = run_end[-1];
                           out_at += 2;
                         }
                       };
  auto run_length = [](const uint8_t* from, const uint8_t* stop) {
                      int32_t n = 1;
                      while( stop>from+n&&from[n]==from[0] )
                        ++n;
                      return n;
                    };
  auto emit_runs = [&]() {
                     while( 1 ) {
                       q = p+1;
                       if( p+1>=end )
                         return false;
                       byte = (uint8_t)*p;
                       run = run_length(p, end);
                       if( run<=2&&(run!=2||lit_len) )
                         return false;
                       run = min32(run, run_max);
                       if( lit_len ) {
                         flush_literal(p);
                         lit_len = 0;
                         byte = p[0];
                       }
                       out_at[1] = byte;
                       p += run;
                       out_at[0] = run<<(nib&31);
                       out_at += 2;
                       if( p>=end )
                         return true;
                     }
                   };
  auto emit_row = [&]() {
                    end = &p[stride];
                    if( p>=end )
                      return;
                    lit_len = 0;
                    do {
                      while( 1 ) {
                        if( emit_runs() )
                          return;
                        ++p;
                        if( ++lit_len!=run_max )
                          break;
                        flush_literal(q);
                        if( q>=end )
                          return;
                        lit_len = 0;
                      }
                    } while( q<end );
                    if( lit_len )
                      flush_literal(q);
                  };
  if( rows>0 ) {
    for( int32_t row_i = 0; row_i<rows; ++row_i ) {
      emit_row();
      out_at[0] = 0;
      out_at[1] = 0;
      out_at += 2;
      p -= 2*stride;
    }
  }
  out_at[0] = 0;
  out_at[1] = 1;
  return out_at+2;
}

int32_t write_bmp(BmfImage* img, char* path, int32_t want_rle) {
  uint8_t* out_at;
  int32_t bits, coded_bytes;
  FILE* fp = fopen(path, "wb");
  if( !fp )
    return 0;
  uint8_t* out_buf = (uint8_t*)bmf_new(img[0].data_size+8*img[0].height+(img[0].data_size>>5)+2048);
  BmpHeader* bmp = (BmpHeader*)out_buf;
  bmp[0].biSize = bmp_hdr_v3;
  bmp[0].bfType = bmp_magic;
  int32_t rows = img[0].height;
  bmp[0].bfReserved2 = 0;
  bmp[0].bfReserved1 = 0;
  bmp[0].biWidth = img[0].width;
  bmp[0].biHeight = rows;
  uint8_t depth = img[0].depth;
  bmp[0].biPlanes = 1;
  bits = depth&depth_bits;
  bmp[0].biBitCount = bits;
  bmp[0].biClrImportant = 0;
  bmp[0].biClrUsed = 0;
  bmp[0].biYPelsPerMeter = 0;
  bmp[0].biXPelsPerMeter = 0;
  const int32_t pal_bytes = write_bmp_palette(img, out_buf, depth, bits);
  uint8_t*const buf = &out_buf[pal_bytes+54];
  uint32_t stride = img[0].stride;
  auto finish = [&](int32_t coded) {
                  bmp[0].biSizeImage = (uint32_t)coded;
                  uint32_t n_bytes = (uint32_t)(out_at-out_buf);
                  bmp[0].bfSize = n_bytes;
                  if( fwrite(out_buf, 1u, n_bytes, fp)!=bmp[0].bfSize )
                    return 0;
                  free(out_buf);
                  fclose(fp);
                  return 1;
                };
  uint32_t off_bits = (uint32_t)(buf-out_buf);
  const uint32_t data_len = img[0].data_size;
  bmp[0].bfOffBits = off_bits;
  out_at = buf;
  uint8_t* p = &img[0].pixels[data_len-stride];
  if( want_rle ) {
    int32_t rle_kind = 0;
    uint8_t*const rle_end = bmp_rle_encode(img, bits, buf, rows, stride, &rle_kind);
    if( rle_end&&data_len>(uint32_t)(rle_end-buf) ) {
      bmp[0].biCompression = rle_kind;
      out_at = rle_end;
      return finish(out_at-buf);
    }
  }
  bmp[0].biCompression = bmp_rgb;
  if( rows<=0 ) {
    coded_bytes = 0;
  } else {
    const uint32_t pad = ((stride+3)&0xFFFFFFFC)-stride;
    for( int32_t y = 0; y<img[0].height; ++y ) {
      memcpy(out_at, p, stride);
      out_at += stride;
      p -= stride;
      if( pad ) {
        *(uint32_t*)out_at = 0;
        out_at += pad;
      }
    }
    coded_bytes = out_at-buf;
  }
  return finish(coded_bytes);
}
uint8_t write_member_head(const void* head, const CodedTail* extra, BmfStream* io) {
  uint8_t ok = io[0].write_exact(head, 0x10u);
  if( extra ) {
    const uint32_t coded_len = extra[0].len;
    ok &= io[0].write_exact(extra, coded_len+sizeof(CodedTail));
  }
  return ok;
}
// ---------------------------------------------------------------------------
struct BMFCodec : BMFState {
  struct WeightSearch {
    BMFCodec* cx;
    const BmfImage* img;
    const uint8_t* plane0;
    int32_t plane_a, plane_b;
    int32_t w[2];
    uint32_t cost;
    int32_t trial(int32_t axis, int32_t cand, int32_t step) {
      int32_t pair[2] = {w[0], w[1]};
      pair[axis] = cand;
      const uint32_t c = cx[0].weight_pair_cost(img, plane0, plane_a, plane_b, pair[0], pair[1]);
      if( c<cost ) {
        cost = c;
        w[axis] = cand;
      }
      return w[axis]+step;
    }

    void descend(int32_t step, int32_t dir) {
      int32_t at[2] = {w[0]+dir, w[1]+dir};
      int32_t end[2] = {w[0]+dir*step, w[1]+dir*step};
      while( dir*at[0]<dir*end[0]||dir*at[1]<dir*end[1] ) {
        for( int32_t k = 0; k<2; ++k )
          if( dir*at[k]<dir*end[k]&&(dir<0 ? at[k]>=kWeightMin : at[k]<kWeightMax+1) )
            end[k] = trial(k, at[k], dir*step);
        at[0] += dir;
        at[1] += dir;
      }
    }
  };

  BlockPool<AltP1Block, 4> p1_blocks;
  BlockPool<AltP2Block, 4> p2_blocks;
  BlockPool<ModelBlock, 1> model_blocks;

  template <class T, int32_t N> int32_t planes_free(BlockPool<T, N> &pool, T** plane) {
    for( int32_t k = 0; k<plane_count; ++k ) pool.give(plane[k]);
    return plane_count;
  }

  void allow_refs_where_present() {
    for( int32_t k = 0; k<plane_count; ++k )
      plane_desc[k].flags |= plane_desc[k].nrefs!=0 ? desc_has_refs : 0;
  }
  template <int32_t f_DEC> int32_t alt_model_p1(BmfImage* hdr, uint8_t* buf) {
    AltP1Block* plane[4];
    int32_t cur0, cur1, cur3;
    int32_t want2;
    int32_t width = hdr[0].width;
    int32_t height = hdr[0].height;
    for( int32_t k = 0; k<plane_count; ++k ) {
      AltP1Block* raw = p1_blocks.take(false);
      plane[k] = raw[0].alt_p1_alloc(this, width, height, k);
    }
    int32_t src1 = plane_desc[1].src_plane;
    int32_t src2 = plane_desc[2].src_plane;
    int32_t src3 = plane_desc[3].src_plane;
    uint8_t fl1 = plane_desc[src1].flags;
    uint8_t fl2 = plane_desc[src2].flags;
    uint8_t fl3 = plane_desc[src3].flags;
    int32_t dc2 = plane_desc[src2].dc;
    int32_t dc1 = plane_desc[src1].dc;
    int32_t xf1 = fl1&8;
    int8_t dc3 = plane_desc[src3].dc;
    int32_t xf2 = fl2&8;
    int32_t xf3 = fl3&8;
    if constexpr( f_DEC )
      rc_begin_decode();
    else
      rc_begin_encode();
    for( uint32_t y = 0; y<(uint32_t)height; ++y ) {
      for( int32_t p = 0; p<plane_count; ++p ) {
        plane[p][0].advance_row();
        plane[p][0].seed_activity(true);
      }
      {
        auto code_sample = [&](int32_t n, AltP1Block* a, AltP1Block* b, int32_t off, int32_t val) {
                             AltP1Block*const blk = plane[n];
                             blk[0].ctx_of(a, b);
                             val = f_DEC ? blk[0].decode_sample() : blk[0].encode_sample(buf, off, val);
                             blk[0].record_sample(val);
                             return val;
                           };
        auto fold_from = [&](int32_t off, int32_t xf, int32_t dc, int32_t mix) {
                           const int32_t v = buf[off];
                           return xf ? v-dc-mix : v;
                         };
        auto unfold_to = [&](int32_t off, int32_t xf, int32_t dc, int32_t mix, int32_t val) {
                           if( xf )
                             val += dc+mix;
                           buf[off] = val;
                         };
        for( uint32_t x = 0; x<(uint32_t)width; ++x ) {
          int32_t off0 = plane_desc[0].src_plane;
          if constexpr( !f_DEC )
            cur0 = fold_from(off0, 0, 0, 0);
          cur0 = code_sample(0, nullptr, nullptr, off0, cur0);
          int32_t off1 = plane_desc[1].src_plane;
          if constexpr( f_DEC )
            unfold_to(off0, 0, 0, 0, cur0);
          else
            cur1 = (uint8_t)fold_from(off1, xf1, dc1, buf[off0]);
          cur1 = code_sample(1, plane[0], nullptr, off1, cur1);
          int32_t off2 = plane_desc[2].src_plane;
          if constexpr( f_DEC )
            unfold_to(off1, xf1, dc1, buf[off0], cur1);
          else
            want2 = (uint8_t)fold_from(off2, xf2, dc2, plane_mix2(plane_desc[off2], buf+off0, buf+off1));
          want2 = code_sample(2, plane[1], plane[0], off2, want2);
          if constexpr( f_DEC )
            unfold_to(off2, xf2, dc2, plane_mix2(plane_desc[off2], buf+off0, buf+off1), want2);
          if( plane_count>=4 ) {
            int32_t off3 = plane_desc[3].src_plane;
            if constexpr( !f_DEC )
              cur3 = (uint8_t)fold_from(off3, xf3, dc3, plane_mix3(plane_desc[off3], buf+off3));
            cur3 = code_sample(3, plane[2], plane[1], off3, cur3);
            if constexpr( f_DEC )
              unfold_to(off3, xf3, dc3, plane_mix3(plane_desc[off3], buf+off3), cur3);
          }
          buf += plane_count;
        }
      }
    }
    if constexpr( f_DEC )
      rc_end_decode();
    else
      rc_end_encode();
    return planes_free(p1_blocks, plane);
  }
  int32_t alt_model_p1_decode(BmfImage* hdr, uint8_t* out) {
    return alt_model_p1<1>(hdr, out);
  }
  int32_t alt_model_p1_encode(BmfImage* hdr, uint8_t* src) {
    return alt_model_p1<0>(hdr, src);
  }
  template <int32_t f_DEC> int32_t alt_model_p2(BmfImage* p_i, uint8_t* out) {
    AltP2Block* plane[4];
    P2Coef coef_session;
    coef_session.fold(this);
    const int32_t width = p_i[0].width;
    const int32_t height = p_i[0].height;
    alt_p2_planes_alloc(plane, width);
    const bool ref1_mixed = ref_transformed(1);
    const bool ref2_mixed = ref_transformed(2);
    const bool ref3_mixed = ref_transformed(3);
    if constexpr( f_DEC )
      rc_begin_decode();
    else
      rc_begin_encode();
    int32_t nplanes = plane_count;
    for( int32_t y = 0; y<height; ++y ) {
      if( nplanes>0 ) {
        alt_p2_start_row(plane, y, width);
        nplanes = plane_count;
      }
      for( int32_t j = 0; j<4; ++j )
        ctx_bias[j] = 0;
      for( int32_t x = 0; x<width; ++x ) {
        for( int32_t j = 0; j<4; ++j )
          ctx_bias[j] >>= 3;
        plane[0][0].code_sample<f_DEC>(out, plane_desc[0].src_plane, plane[2], plane[1]);
        plane[0][0].add_bias(ctx_bias);
        int16_t seed1 = 0;
        if( ref1_mixed )
          seed1 = 16*out[plane_desc[0].src_plane];
        plane[1][0].cursor[0][0].dval = seed1;
        plane[1][0].code_sample<f_DEC>(out, plane_desc[1].src_plane, plane[0], plane[2]);
        plane[1][0].add_bias(ctx_bias);
        int16_t seed2 = 0;
        if( ref2_mixed ) {
          const PlaneDesc &mix = plane_desc[plane_desc[2].src_plane];
          seed2 = (mix.weight1*out[plane_desc[1].src_plane]+mix.weight0*out[plane_desc[0].src_plane])>>3;
        }
        plane[2][0].cursor[0][0].dval = seed2;
        plane[2][0].code_sample<f_DEC>(out, plane_desc[2].src_plane, plane[0], plane[1]);
        plane[2][0].add_bias(ctx_bias);
        nplanes = plane_count;
        if( plane_count>=4 ) {
          int16_t seed3 = 0;
          if( ref3_mixed ) {
            const PlaneDesc &mix = plane_desc[plane_desc[3].src_plane];
            seed3 = (mix.weight2*out[2]+mix.weight1*out[1]+mix.weight0*out[0])>>3;
          }
          plane[3][0].cursor[0][0].dval = seed3;
          plane[3][0].code_sample<f_DEC>(out, f_DEC ? 3 : plane_desc[3].src_plane, plane[2], plane[0]);
          plane[3][0].add_bias(ctx_bias);
          nplanes = plane_count;
        }
        out += nplanes;
      }
    }
    if constexpr( f_DEC )
      rc_end_decode();
    else
      rc_end_encode();
    coef_session.restore(this);
    return planes_free(p2_blocks, plane);
  }
  template <int32_t f_DEC> void alt_model_p2_d8(uint8_t* src, int32_t i, int32_t height, uint8_t* out) {
    AltP2Block* blk;
    AltP2Block* raw = p2_blocks.take(true);
    if( raw )
      blk = raw[0].alt_p2_alloc(this, i, 0);
    else
      blk = nullptr;
    if constexpr( f_DEC )
      blk[0].alt_p2_d8_body<1>(nullptr, out, i, height);
    else
      blk[0].alt_p2_d8_body<0>(src, out, i, height);
    if( blk )
      p2_blocks.give(blk);
  }
  void alt_model_p2_d8_decode(uint8_t* out, int32_t i, int32_t height) {
    alt_model_p2_d8<1>(nullptr, i, height, out);
  }
  void alt_model_p2_d8_encode(uint8_t* src, int32_t i, int32_t height, uint8_t* out) {
    alt_model_p2_d8<0>(src, i, height, out);
  }
  int32_t alt_model_p2_decode(BmfImage* p_i, uint8_t* out) {
    return alt_model_p2<1>(p_i, out);
  }
  int32_t alt_model_p2_encode(BmfImage* p_i, uint8_t* a2) {
    return alt_model_p2<0>(p_i, a2);
  }
  template <int32_t f_DEC> bool alt_model_plane(BmfImage* p_i, uint8_t* pixels, uint8_t* raw) {
    if( !plane_alt_model )
      return false;
    const bool p1 = plane_predictor==pred_p1;
    if( !p1&&plane_predictor!=pred_p2 )
      return true;
    const bool d8 = (p_i[0].depth&depth_bits)==8;
    if( d8 ) {
      if constexpr( f_DEC ) {
        if( p1 )
          alt_model_p1_d8_decode(pixels, p_i[0].width, p_i[0].height);
        else
          alt_model_p2_d8_decode(pixels, p_i[0].width, p_i[0].height);
      } else {
        if( p1 )
          alt_model_p1_d8_encode(pixels, p_i[0].width, p_i[0].height, raw);
        else
          alt_model_p2_d8_encode(pixels, p_i[0].width, p_i[0].height, raw);
      }
    } else if constexpr( f_DEC ) {
      if( p1 )
        alt_model_p1_decode(p_i, pixels);
      else
        alt_model_p2_decode(p_i, pixels);
    } else {
      if( p1 )
        alt_model_p1_encode(p_i, pixels);
      else
        alt_model_p2_encode(p_i, pixels);
    }
    return true;
  }
  AltP1Block*alt_p1_block_alloc(int32_t width, int32_t height) {
    AltP1Block* raw = p1_blocks.take(false);
    return raw ? raw[0].alt_p1_alloc(this, width, height, 0) : nullptr;
  }
  int32_t choose_plane_coding(BmfImage* img) {
    alignas(16) int32_t hists[8*1024];
    const HistRows hr = hist_rows(hists);
    int32_t*const hist_flat = hr.r0, *const hist_a = hr.r1;
    int32_t*const hist_b = hr.r2, *const hist_c = hr.r3;
    uint32_t cand_cost[3][4];
    PlaneDesc cand_desc[3][4];
    uint32_t next_plane, row;
    int32_t result, xform, wt8, wt4, pred;
    uint32_t best_cost, best;
    int32_t n_planes = plane_count;
    int32_t data_size = img[0].data_size;
    alphabet_reduced = 1;
    int32_t row_stride = img[0].stride;
    const uint8_t* data_end = &img[0].pixels[data_size];
    memset(hists, 0, sizeof hists);
    memset(cand_desc, 0, sizeof cand_desc);
    if( n_planes>0 ) {
      if( n_planes/2 ) {
        uint32_t pair = 0;
        for(; pair<(uint32_t)(n_planes/2); ++pair ) {
          const uint8_t pl_even = 2*pair, pl_odd = 2*pair+1;
          row = 2*pair;
          plane_desc[row].src_plane = pl_even;
          plane_desc[row].nrefs = pl_even;
          plane_desc[row+1].src_plane = pl_odd;
          plane_desc[row+1].nrefs = pl_odd;
        }
        next_plane = 2*pair+1;
      } else {
        next_plane = 1;
      }
      result = next_plane-1;
      if( (uint32_t)n_planes>(uint32_t)result ) {
        plane_desc[next_plane-1].src_plane = result;
        plane_desc[next_plane-1].nrefs = result;
      }
      if( n_planes>=3 ) {
        xform = 0;
        uint32_t cheapest = cost_candidate(img, 0, cand_desc[0], cand_cost[0]);
        for( int32_t cand = 1; cand<3; ++cand ) {
          const uint32_t c = cost_candidate(img, cand, cand_desc[cand], cand_cost[0]);
          if( c<cheapest ) {
            cheapest = c;
            xform = cand;
          }
        }
        memcpy(plane_desc, cand_desc[xform], sizeof cand_desc[xform]);
        best_cost = cand_cost[xform][0];
        int32_t plane_a = plane_desc[0].src_plane-xform;
        wt8 = plane_desc[xform].weight1;
        int32_t plane_b = plane_desc[1].src_plane-xform;
        wt4 = plane_desc[xform].weight0;
        {
          const uint8_t* plane0 = &img[0].pixels[xform];
          WeightSearch search = {this, img, plane0, plane_a, plane_b, {wt4, wt8}, best_cost};
          int32_t wt_step = 4;
          search.descend(wt_step, -1);
          search.descend(wt_step, +1);
          wt8 = search.w[1];
          wt4 = search.w[0];
          best_cost = search.cost;
        }
        uint8_t* px = &img[0].pixels[row_stride+n_planes+xform];
        if( px<data_end ) {
          do {
            int32_t c2 = px[plane_b];
            int32_t c2w = c2*wt8;
            int32_t c1 = px[plane_a];
            int32_t c1w = c1*wt4;
            ++hist_c[c2-c1+1280];
            int32_t c0 = px[0]+512;
            px += n_planes;
            int32_t bin0 = ((uint16_t)c0-(uint16_t)((uint32_t)(c2w+c1w+40)>>7))&0x3FF;
            ++hist_flat[bin0];
            ++hist_a[c0-c1];
            ++hist_b[c0-c2];
            ++hist_c[(c0-((uint32_t)(((c1+c2)<<6)+40)>>7))];
          } while( px<data_end );
        }
        const uint32_t slack = umin32(best_cost>>7, kSlackMax);
        best = slack+best_cost;
        uint32_t cost_flat = cand_cost[xform][1];
        bool cheaper = cost_flat<best;
        if( cost_flat<best ) {
          best = cost_flat;
          wt4 = 128;
          wt8 = 0;
        }
        pred = cheaper;
        if( cand_cost[xform][2]<best ) {
          best = cand_cost[xform][2];
          pred = 2;
          wt4 = 0;
          wt8 = 128;
        }
        bool keep3 = best<=cand_cost[xform][3];
        if( best>cand_cost[xform][3] )
          pred = 3;
        if( !keep3 ) {
          wt4 = 64;
          wt8 = 64;
        }
        plane_desc[xform].weight0 = wt4;
        plane_desc[xform].weight1 = wt8;
        plane_desc[xform].dc = widest_window(hists+1024*pred, 4, 1024, -1)+1;
        result = widest_window(hist_c+1024, 0, 512, 255)+1;
        plane_desc[plane_b+xform].dc = result;
        if( n_planes>=4 ) {
          choose_alpha_plane(img, hists, data_end, row_stride);
        }
      }
    }
    return result;
  }
  void clear_flags_on_all() {
    for( int32_t k = 0; k<plane_count; ++k )
      plane_desc[k].flags = 0;
  }
  template <int32_t f_DEC> void code_colour_plane(BmfImage* img, uint8_t* side, int32_t plane) {
    uint8_t* base = &img[0].pixels[plane];
    if( !(plane_desc[plane].flags&desc_has_refs) ) {
      if constexpr( f_DEC )
        interleave_flat(img, side, plane);
      else
        deinterleave_plane(img, side, plane);
      return;
    }
    int32_t stride = plane_count;
    int32_t n = bmf_pixels(img);
    const PlaneTransform t = plane_transform(plane);
    if( t.by_weights||plane_desc[plane].nrefs==1 ) {
      const uint8_t* ref = base+t.to_ref0;
      for( int32_t i = 0, ofs = 0; i<n; ++i, ofs += stride ) {
        if constexpr( f_DEC )
          base[ofs] = ref[ofs]+t.dc+side[i];
        else
          side[i] = base[ofs]-t.dc-ref[ofs];
      }
      return;
    }
    if( t.mode==2||t.mode==3 ) {
      uint8_t* p = base;
      for( int32_t left = n; left; --left, p += stride ) {
        const uint32_t blend = t.blend(p);
        if constexpr( f_DEC )
          *p = (uint8_t)(blend+t.dc+*side++);
        else
          *side++ = (uint8_t)(*p-t.dc-blend);
      }
    }
  }
  template <int32_t f_DEC, typename T> void code_field(T &v, int32_t bits, int32_t bias) {
    if constexpr( f_DEC )
      v = (T)((int32_t)unpack_bits(bits)-bias);
    else
      pack_bits((uint32_t)((int32_t)v+bias), bits);
  }
  void code_image_body(BmfImage* p_i, BmfImage &hdr) {
    if( (p_i[0].depth&depth_bits)<=4 ) {
      stream_open(p_i[0].data_size);
      plane_predictor = 0;
      plane_alt_model = 0;
      alphabet_reduced = 0;
      model_plane(p_i, p_i[0].pixels, p_i[0].pixels);
      return;
    }
    const uint32_t filtered = search_filter(p_i);
    hdr.flags |= flags_descriptors;
    const uint32_t data_bytes = p_i[0].data_size;
    if( p_i[0].flags&flags_transposed ) {
      hdr.header_from(p_i);
      hdr.flags = flags_coded|flags_descriptors|flags_slow|p_i[0].flags;
    }
    stream_open(data_bytes);
    if( plane_count==1 ? (p_i[0].depth&depth_grey)!=0 : plane_count>2 ) {
      near_lossless_q = 0;
      pack_bits(0, 4);
    }
    alphabet_reduced = 0;
    write_plane_descs();
    if( filtered ) {
      hdr.flags |= flags_planar;
      uint8_t* plane_buf = (uint8_t*)bmf_new(p_i[0].width*p_i[0].height);
      for( int32_t k = 0; k<plane_count; ++k )
        model_planes(p_i, plane_buf, plane_desc[k].src_plane);
      free(plane_buf);
    } else {
      transform_planes(p_i);
    }
  }
  template <int32_t f_DEC> void code_plane(BmfImage* p_i, uint8_t* pixels, uint8_t* raw) {
    if( alt_model_plane<f_DEC>(p_i, pixels, raw) )
      return;
    ModelBlock* blk = new_model_block(p_i[0].width, p_i[0].height, p_i[0].depth&depth_bits);
    if constexpr( f_DEC )
      blk[0].unmodel_plane_slow(pixels);
    else
      blk[0].model_plane_slow(pixels);
    blk[0].free_workspace();
    model_blocks.give(blk);
  }
  template <int32_t f_DEC> void code_plane_descs() {
    for( int32_t pl = 0; pl<plane_count; ++pl ) {
      PlaneDesc &d = plane_desc[pl];
      uint32_t head = (uint32_t)((d.flags<<2)|d.nrefs);
      code_field<f_DEC>(head, 6, 0);
      if constexpr( f_DEC ) {
        d.flags = (uint8_t)(head>>2);
        d.nrefs = (uint8_t)(head&3);
        plane_desc[head&3].src_plane = (uint8_t)pl;
      }
      if( !(d.flags&desc_has_refs) )
        continue;
      code_field<f_DEC>(d.dc, 8, 0);
      if( d.nrefs<=1u )
        continue;
      code_field<f_DEC>(d.weight0, 8, 64);
      code_field<f_DEC>(d.weight1, 8, 64);
      if( d.nrefs>2u )
        code_field<f_DEC>(d.weight2, 8, 64);
    }
  }
  void colour_transform(BmfImage* img, uint8_t* dst, int32_t plane) {
    code_colour_plane<0>(img, dst, plane);
  }
  int32_t compress_image(BmfFile* arc_in, BmfImage* p_i, const CodedTail* extra_blk) {
    BmfImage hdr;
    if( !arc_in[0].io.fp&&!arc_in[0].io.buf )
      return 0;
    if( extra_blk )
      p_i[0].flags |= flags_tail;
    near_lossless_q = 0;
    hdr.header_from(p_i);
    hdr.flags = p_i[0].flags;
    plane_count = ((p_i[0].depth&depth_bits)+7)>>3;
    const uint32_t tag = bmf_tag(bmf_sig_image, '2', '0');
    if( !arc_in[0].io.write_exact(&tag, 4u) )
      return 0;
    uint8_t bpp = p_i[0].depth;
    uint32_t pal_bytes = 0;
    if( bpp&depth_palette )
      pal_bytes = 3<<(bpp&31);
    if( p_i[0].data_size>=0x10u ) {
      desc_slow_mode = 1;
      hdr.flags |= flags_coded|flags_slow;
      code_image_body(p_i, hdr);
      packer_flush();
      const int32_t coded_bytes = stream.cur-stream.buf;
      hdr.data_size = (uint32_t)coded_bytes;
      if( (uint32_t)coded_bytes<p_i[0].data_size ) {
        const uint8_t ok = write_member_head(&hdr, extra_blk, &arc_in[0].io);
        int32_t ok_all = arc_in[0].io.write_exact(stream.buf, (size_t)coded_bytes)&ok;
        free(stream.buf);
        if( ok_all&&(p_i[0].depth&depth_palette)!=0 )
          arc_in[0].io.write_all(&p_i[0].pixels[p_i[0].data_size], pal_bytes);
        arc_in[0].io.flush();
        if( ok_all )
          return (int32_t)hdr.data_size;
        return ok_all;
      }
      free(stream.buf);
      if( p_i[0].flags&flags_transposed )
        transpose_image(p_i, plane_count);
    }
    const uint8_t ok_raw = write_member_head(p_i, extra_blk, &arc_in[0].io);
    uint32_t written = (uint32_t)arc_in[0].io.write(p_i[0].pixels, pal_bytes+p_i[0].data_size);
    int32_t data_size = p_i[0].data_size;
    if( !(ok_raw&(written==data_size+pal_bytes)) )
      return 0;
    return data_size;
  }
  int32_t cost_candidate(BmfImage* img, int32_t cand, PlaneDesc* pd, uint32_t* costs) {
    alignas(16) int32_t hists[6*1024];
    const HistRows hr = hist_rows(hists);
    int32_t*const hist_x = hr.r0;
    int32_t*const hist_y = hr.r1;
    int32_t*const hist_yx = hr.r2;
    int32_t*const hist_zx = hr.r3;
    int32_t*const hist_zy = hr.r4;
    int32_t*const hist_zp = hr.r5;
    memset(hists, 0, sizeof hists);
    const int32_t planes = plane_count;
    const uint32_t row_b = img[0].stride;
    int32_t d1 = (cand+1)%3-cand;
    int32_t d2 = (cand+2)%3-cand;
    uint8_t* img_end = &img[0].pixels[img[0].data_size];
    uint8_t* base = img[0].pixels;
    double syz = 0.0, syy = 0.0, sxz = 0.0, sxy = 0.0, sxx = 0.0;
    pd[cand].nrefs = 2;
    pd[2].src_plane = (uint8_t)cand;
    uint8_t* p = &base[cand+row_b+planes];
    if( p<img_end ) {
      const int32_t a_n = d1-row_b, a_w = d1-planes, a_nw = d1-row_b-planes;
      const int32_t b_n = d2-row_b, b_w = d2-planes, b_nw = d2-row_b-planes;
      const int32_t step = row_b+planes;
      do {
        int32_t dx = p[a_nw]+p[d1]-(p[a_n]+p[a_w]);
        ++hist_x[dx+512];
        int32_t dy = p[b_nw]+p[d2]-(p[b_n]+p[b_w]);
        sxx = sxx+(double)dx*(double)dx;
        ++hist_y[dy+512];
        syy = syy+(double)dy*(double)dy;
        sxy = sxy+(double)dx*(double)dy;
        ++hist_yx[((uint16_t)dy-(uint16_t)dx-512)&0x3FF];
        int32_t diag = p[-step]+p[0];
        int32_t west = p[-planes];
        uint8_t* q = p-row_b;
        p += planes;
        int32_t dz = diag-(*q+west);
        sxz = sxz+(double)dx*(double)dz;
        ++hist_zx[((uint16_t)dz-(uint16_t)dx-512)&0x3FF];
        syz = syz+(double)dy*(double)dz;
        ++hist_zy[((uint16_t)dz-(uint16_t)dy-512)&0x3FF];
        int32_t bin = ((uint16_t)dz-(uint16_t)((uint32_t)(((dx+dy)<<6)+40)>>7)-512)&0x3FF;
        ++hist_zp[bin];
      } while( p<img_end );
    }
    double inv = 128.0/(0.1-sxy*sxy+sxx*syy);
    double w1f = (syy*sxz-sxy*syz)*inv;
    double w2f = inv*(sxx*syz-sxy*sxz);
    int32_t wa = clamp_weight((int32_t)w1f);
    int32_t wb = clamp_weight((int32_t)w2f);
    int32_t rec = 4*cand;
    costs[rec] = weight_pair_cost(img, &base[cand], d1, d2, wa, wb);
    costs[rec+1] = estimate_cost(hist_zx, 1024);
    costs[rec+2] = estimate_cost(hist_zy, 1024);
    costs[rec+3] = estimate_cost(hist_zp, 1024);
    int32_t cost = estimate_cost(hist_x, 1024);
    int32_t cost_y = estimate_cost(hist_y, 1024);
    int32_t cost_yx = estimate_cost(hist_yx, 1024);
    int32_t c2 = cost_yx;
    int32_t lo1 = c2;
    if( cost<c2 )
      lo1 = cost;
    if( cost_y<c2 )
      c2 = cost_y;
    int32_t s1 = cost_y+lo1;
    int32_t s2 = cost+c2;
    int32_t best = s2;
    if( s1<s2 ) {
      best = s1;
      cost = cost_y;
      swap32(d1, d2);
      swap32(costs[rec+1], costs[rec+2]);
      swap32(wa, wb);
    }
    pd[cand].weight0 = wa;
    pd[cand].weight1 = wb;
    int32_t idx1 = cand+d1;
    int32_t idx2 = cand+d2;
    pd[idx1].nrefs = 0;
    pd[0].src_plane = (uint8_t)idx1;
    pd[idx2].nrefs = 1;
    pd[1].src_plane = (uint8_t)idx2;
    if( img[0].data_size>0x1000000u )
      return cost+cost_yx+costs[rec];
    uint32_t lo2 = costs[rec];
    uint32_t lo3 = costs[rec+2];
    lo2 = umin32(lo2, costs[rec+1]);
    lo3 = umin32(lo3, costs[rec+3]);
    lo3 = umin32(lo3, lo2);
    return best+lo3;
  }
  void deinterleave_plane(const BmfImage* img, uint8_t* dst, int32_t plane) {
    strided_copy(dst, 1, &img[0].pixels[plane], plane_count, bmf_pixels(img));
  }
  void drop_alt_model_from_all() {
    for( int32_t k = 0; k<plane_count; ++k )
      plane_desc[k].flags &= ~desc_alt_model;
  }
  bool expand_coded(BmfStream* io, BmfImage* img_at, const BmfImage &hdr) {
    near_lossless_q = 0;
    alphabet_reduced = 0;
    if( !(hdr.flags&flags_slow) ) {
      printf("\nwritten in fast mode; this build only decodes -S streams\n");
      exit(3);
    }
    desc_slow_mode = 1;
    stream.size = hdr.data_size;
    stream.buf = (uint8_t*)bmf_new(hdr.data_size);
    packer_reset();
    uint32_t want = hdr.data_size;
    bool ok = io[0].read(stream.buf, hdr.data_size)==want;
    if( ok ) {
      if( (img_at[0].depth&depth_bits)<=4||(hdr.flags&flags_descriptors)==0 ) {
        plane_predictor = 0;
        plane_alt_model = 0;
        unmodel_plane(img_at, img_at[0].pixels);
      } else {
        unmodel_described(img_at, hdr.flags);
      }
      ok = stream.buf+hdr.data_size==stream.cur;
    }
    free(stream.buf);
    stream.buf = nullptr;
    return ok;
  }
  BmfImage*expand_image(BmfFile* arc_in, CodedTail** p_coded_buf) {
    BmfImage hdr;
    CodedTail head;
    int32_t version;
    uint32_t magic;
    if( p_coded_buf )
      *p_coded_buf = nullptr;
    BmfStream* io = &arc_in[0].io;
    if( !io[0].fp&&!io[0].buf )
      return nullptr;
    while( 1 ) {
      if( !io[0].read_exact(&magic, 4u) ) {
        return io[0].eof() ? nullptr : arc_in[0].fail();
      }
      if( (uint16_t)magic!=bmf_sig_other )
        break;
      version = bmf_tag_version(magic);
      if( version!=bmf_version_20||!io[0].read_exact(&head, 8u) )
        break;
      io[0].seek(head.len, SEEK_CUR);
    }
    version = bmf_tag_version(magic);
    if( (uint16_t)magic!=bmf_sig_image||version!=bmf_version_20||!io[0].read_exact(&hdr, 0x10u) ) {
      return arc_in[0].fail();
    }
    if( hdr.flags&flags_tail ) {
      if( !io[0].read_exact(&head, 8u) )
        return arc_in[0].fail();
      if( p_coded_buf ) {
        if( head.len>io[0].bytes_left() ) {
          return arc_in[0].fail();
        }
        uint32_t pad_len = (head.len+(head.len==0)+3)&0xFFFFFFFC;
        CodedTail* blk = (CodedTail*)bmf_new(pad_len+sizeof(CodedTail));
        blk[0].tag = head.tag;
        blk[0].len = pad_len;
        blk[0].data[pad_len/4-1] = 0;
        p_coded_buf[0] = blk;
        if( !io[0].read_exact(blk[0].data, head.len) )
          return arc_in[0].fail();
      } else {
        io[0].seek(head.len, SEEK_CUR);
      }
    }
    if( hdr.data_size>io[0].bytes_left() ) {
      return arc_in[0].fail();
    }
    uint32_t pal_bytes = 3<<(hdr.depth&31);
    if( !(hdr.depth&depth_palette) )
      pal_bytes = 0;
    if( !hdr.width||!hdr.height||(uint32_t)hdr.width*(uint32_t)((((hdr.depth&depth_bits)+7)>>3))>0xFFFFu ) {
      return arc_in[0].fail();
    }
    BmfImage* img_at = alloc_image(hdr.width, hdr.height, hdr.depth&depth_bits, (hdr.depth&depth_palette)!=0, 1);
    img_at[0].depth = hdr.depth;
    uint8_t has_coded = p_coded_buf&&p_coded_buf[0];
    img_at[0].flags |= (hdr.flags&flags_transposed)|(has_coded ? flags_tail : 0);
    plane_count = ((hdr.depth&depth_bits)+7)>>3;
    if( plane_count<1||plane_count>4 ) {
      return arc_in[0].fail();
    }
    if( hdr.flags&flags_coded ) {
      if( !expand_coded(io, img_at, hdr) ) {
        return arc_in[0].fail();
      }
    } else {
      if( hdr.data_size>img_at[0].data_size ) {
        return arc_in[0].fail();
      }
      uint32_t want = hdr.data_size;
      if( io[0].read(img_at[0].pixels, hdr.data_size)!=want ) {
        return arc_in[0].fail();
      }
    }
    if( hdr.depth&depth_palette ) {
      uint8_t* pal_at = (img_at[0].depth&depth_palette) ? img_at[0].palette() : nullptr;
      uint32_t got = (uint32_t)io[0].read(pal_at, pal_bytes);
      if( got!=pal_bytes ) {
        return arc_in[0].fail();
      }
    }
    if( img_at[0].flags&flags_transposed ) {
      if( (uint64_t)img_at[0].width*img_at[0].height*(uint32_t)plane_count>(uint64_t)img_at[0].data_size ) {
        return arc_in[0].fail();
      }
      transpose_image(img_at, plane_count);
    }
    return img_at;
  }
  void interleave_flat(BmfImage* img, const uint8_t* src, int32_t plane) {
    strided_copy(&img[0].pixels[plane], plane_count, src, 1, bmf_pixels(img));
  }
  void interleave_plane(BmfImage* img, uint8_t* src, int32_t plane) {
    code_colour_plane<1>(img, src, plane);
  }
  void model_plane(BmfImage* p_i, uint8_t* pixels, uint8_t* raw) {
    code_plane<0>(p_i, pixels, raw);
  }
  void model_planes(BmfImage* img, uint8_t* pixels, int32_t plane) {
    plane_predictor = plane_desc[plane].flags&desc_predictor;
    plane_alt_model = (plane_desc[plane].flags&desc_alt_model)!=0;
    colour_transform(img, pixels, plane);
    memset(hist_scratch, 0, 1024);
    {
      BmfImage hdr = img[0];
      hdr.depth = depth_one_plane;
      if( plane_predictor==pred_p1&&!plane_alt_model )
        predict_med(pixels, img[0].width, img[0].height);
      model_plane(&hdr, pixels, pixels);
    }
  }
  ModelBlock*new_model_block(int32_t img_w, int32_t img_h, int32_t img_depth) {
    void* raw = model_blocks.take(false);
    return ((ModelBlock*)raw)[0].layout_workspace(this, img_w, img_h, img_depth);
  }
  void offer_to_all(int32_t want) {
    for( int32_t k = 0; k<plane_count; ++k )
      plane_desc[k].flags = (plane_desc[k].flags&desc_has_refs)|want;
  }
  void pack_bits(uint32_t word, int32_t n) {
    if( stream.pk.free_bits<n ) {
      *stream.pk.word = stream.pk.acc|(2*(word<<((31-stream.pk.free_bits)&31)));
      stream.pk.word = (uint32_t*)stream.cur;
      stream.cur += 4;
      stream.pk.acc = word>>(stream.pk.free_bits&31);
      stream.pk.free_bits += 32-n;
    } else {
      stream.pk.acc |= word<<(-stream.pk.free_bits&31);
      stream.pk.free_bits -= n;
    }
  }
  void packer_flush() {
    *stream.pk.word = stream.pk.acc;
  }
  void packer_reset() {
    stream.pk.free_bits = 0;
    stream.pk.acc = 0;
    stream.cur = stream.buf;
    stream.pk.word = (uint32_t*)stream.buf;
    hist_scratch = stream.buf+stream.size-4096;
  }
  uint32_t packer_word() {
    if( stream.cur+4>stream.buf+stream.size ) bmf_fatal(bmf_read_error);
    uint32_t w = *(uint32_t*)stream.cur;
    stream.cur += 4;
    return w;
  }
  int32_t plane_in_order(int32_t k) {
    const int32_t plane = plane_desc[k].src_plane;
    plane_predictor = plane_desc[plane].flags&desc_predictor;
    return plane;
  }
  PlaneTransform plane_transform(int32_t plane) {
    PlaneTransform t;
    t.to_ref0 = plane_desc[0].src_plane-plane;
    t.to_ref1 = plane_desc[1].src_plane-plane;
    t.mode = plane_desc[plane].nrefs;
    t.dc = plane_desc[plane].dc;
    t.wgt1 = plane_desc[plane].weight1;
    t.wgt0 = plane_desc[plane].weight0;
    t.wgt2 = plane_desc[plane].weight2;
    t.by_weights = false;
    if( t.mode==2&&t.wgt0+t.wgt1==128 ) {
      if( !t.wgt1 ) {
        t.by_weights = true;
      } else if( !t.wgt0 ) {
        t.to_ref0 = plane_desc[1].src_plane-plane;
        t.by_weights = true;
      }
    }
    return t;
  }
  uint32_t predict_med(uint8_t* pixels, int32_t width, int32_t height) {
    uint32_t done, last;
    alignas(16) uint8_t fold[272];
    uint8_t* p = pixels+height*width;
    uint8_t* up = &p[-width];
    med_fold_table(fold);
    for( int32_t rows_left = height-1; rows_left; --rows_left ) {
      for( int32_t x_left = width-1; x_left; --x_left ) {
        const int32_t north = (uint8_t)*--up;
        --p;
        const uint8_t pred = med_predict(p[-1], north, p[-width-1]);
        const int32_t code = (uint8_t)fold[(uint8_t)(*p-pred)];
        p[0] = code;
        hist_bump(code);
      }
      --up;
      --p;
      last = (uint8_t)fold[(uint8_t)(*p-p[-width])];
      p[0] = last;
      hist_bump(last);
    }
    if( width!=1 ) {
      last = width-1;
      const int32_t pairs = last/2;
      {
        uint32_t k = 0;
        for( int32_t ofs = 0; k<(uint32_t)pairs; ++k, ofs -= 2 ) {
          const uint8_t left = p[ofs-2];
          p[ofs-1] = fold[(uint8_t)(p[ofs-1]-left)];
          p[ofs-2] = fold[(uint8_t)(left-p[ofs-3])];
        }
        done = 2*k+1;
      }
      if( last>(done-1) ) {
        uint8_t* q = p-done;
        last = (uint8_t)(*q-*((int8_t*)q-1));
        q[0] = fold[(uint8_t)last];
      }
    }
    return last;
  }
  void read_plane_descs() {
    code_plane_descs<1>();
  }
  void reset_descriptors() {
    for( int32_t k = 0; k<plane_count; ++k ) {
      plane_desc[k].flags = 0;
      plane_desc[k].src_plane = k;
      plane_desc[k].nrefs = k;
    }
  }
  void restore_descriptors(const PlaneDesc* saved) {
    for( int32_t k = 0; k<4; ++k )
      plane_desc[k] = saved[k];
  }
  void save_descriptors(PlaneDesc* saved) {
    for( int32_t k = 0; k<4; ++k )
      saved[k] = plane_desc[k];
  }
  uint32_t search_filter(BmfImage* img) {
    PlaneDesc saved[4];
    uint32_t plane_cost[4];
    uint8_t* tile_buf;
    int32_t tile_h, y0, cand, pl_k;
    int32_t tile_w = img[0].width;
    tile_h = img[0].height;
    if( tile_w<4||tile_h<3 ) {
      reset_descriptors();
      return 0;
    }
    choose_plane_coding(img);
    BmfImage* tile_img = alloc_image(tile_w, tile_h, img[0].depth&depth_bits, 0, 0);
    stream_open(tile_img[0].data_size);
    tile_buf = (uint8_t*)bmf_new(tile_h*tile_w);
    y0 = (img[0].height-tile_h)>>1;
    int32_t dx = img[0].width-tile_w;
    int32_t off_y = y0*img[0].stride;
    uint8_t* srcp = img[0].pixels+plane_count*(dx>>1)+off_y;
    uint8_t* tile_src = tile_img[0].pixels;
    if( tile_h>0 ) {
      int32_t row_bytes = tile_img[0].stride;
      uint8_t* dstp = tile_img[0].pixels;
      for( int32_t y = 0; y<tile_h; ++y ) {
        memcpy(dstp, srcp, row_bytes);
        dstp += row_bytes;
        srcp += img[0].stride;
      }
    }
    PlaneSearch ps = search_planes(tile_img, tile_buf, plane_cost);
    int32_t best_bits = ps.bits;
    {
      transpose_image(tile_img, plane_count);
      int32_t bits_d = 0;
      int32_t pk = 0;
      while( pk<plane_count ) {
        pl_k = plane_desc[pk].src_plane;
        model_planes(tile_img, tile_buf, pl_k);
        const int32_t bits = 8*(stream.cur-stream.buf);
        packer_flush();
        bits_d += bits;
        packer_reset();
        if( (uint32_t)(bits-(bits>>8))>plane_cost[pl_k] )
          break;
        ++pk;
      }
      if( pk<plane_count )
        bits_d += best_bits+1;
      if( bits_d+(bits_d>>12)>=best_bits ) {
        transpose_image(tile_img, plane_count);
      } else {
        best_bits = bits_d;
        transpose_image(img, plane_count);
      }
    }
    free(tile_buf);
    if( plane_count>2 ) {
      if( ps.n_p1 ) {
        save_descriptors(saved);
        offer_to_all(try_p1);
        int32_t bits_e = transform_cost(tile_img);
        if( bits_e<=best_bits ) {
          best_bits = bits_e;
          cand = 0;
        } else {
          restore_descriptors(saved);
          cand = 1;
        }
      } else {
        cand = 1;
      }
      if( ps.n_p2+(cand==0) ) {
        save_descriptors(saved);
        offer_to_all(try_p2);
        if( plane_count==ps.n_p2&&plane_count-1==ps.n_refs ) {
          cand = 0;
        } else {
          const int32_t bits_a = transform_cost(tile_img);
          if( bits_a<=best_bits ) {
            best_bits = bits_a;
            ps.n_p2 = bits_a;
            if( plane_count-1==ps.n_refs ) {
              cand = 0;
            } else {
              save_descriptors(saved);
              allow_refs_where_present();
              int32_t bits2 = transform_cost(tile_img);
              if( bits2>bits_a )
                restore_descriptors(saved);
              cand = 0;
            }
          } else {
            restore_descriptors(saved);
          }
        }
      }
    } else {
      cand = 1;
    }
    if( !ps.n_p2&&plane_count>1 ) {
      uint8_t* tile_copy = (uint8_t*)bmf_new(tile_img[0].data_size);
      memcpy(tile_copy, tile_src, tile_img[0].data_size);
      save_descriptors(saved);
      drop_alt_model_from_all();
      int32_t bits_b = transform_cost(tile_img);
      if( bits_b>best_bits ) {
        restore_descriptors(saved);
      } else {
        best_bits = bits_b;
        cand = 0;
      }
      if( ps.n_refs+ps.n_p1 ) {
        save_descriptors(saved);
        clear_flags_on_all();
        memcpy(tile_src, tile_copy, tile_img[0].data_size);
        int32_t bits_c = transform_cost(tile_img);
        if( bits_c<=best_bits ) {
          cand = 0;
        } else {
          restore_descriptors(saved);
        }
      }
      free(tile_copy);
    }
    free(stream.buf);
    free((uint8_t*)tile_img);
    return cand;
  }
  void stream_open(uint32_t data_size) {
    stream.size = data_size+0x20000;
    stream.buf = (uint8_t*)bmf_new(stream.size);
    packer_reset();
  }
  int32_t transform_cost(BmfImage* tile_img) {
    transform_planes(tile_img);
    const int32_t bits = 8*(stream.cur-stream.buf);
    packer_flush();
    packer_reset();
    return bits;
  }
  void transform_planes(BmfImage* p_i) {
    int32_t i;
    memset(hist_scratch, 0, 4096);

    BmfImage* hdr = (BmfImage*)(stream.buf+sizeof(BmfImage));
    hdr[0] = p_i[0];
    uint8_t* src_pixels = p_i[0].pixels;
    memcpy(hdr[0].pixels, p_i[0].pixels, p_i[0].data_size);
    uint8_t* tmp = (uint8_t*)bmf_new(p_i[0].width*p_i[0].height);
    for( int32_t k = 0; k<plane_count; ++k ) {
      const int32_t plane = plane_in_order(k);
      const int32_t predictor = plane_predictor;
      const int32_t alt = (plane_desc[plane].flags&desc_alt_model)!=0;
      plane_alt_model = alt;
      const bool transform_it = ((plane_desc[plane].flags&desc_has_refs)!=0||predictor)&&!alt;
      if( !transform_it )
        continue;
      colour_transform(hdr, tmp, plane);
      if( plane_predictor==pred_p1 )
        predict_med(tmp, p_i[0].width, p_i[0].height);
      int32_t n = p_i[0].width*p_i[0].height;
      int32_t stride = plane_count;
      if( plane_count==1 ) {
        memcpy(&p_i[0].pixels[plane], tmp, n);
      } else {
        uint8_t* dst = &p_i[0].pixels[plane];
        int32_t ofs = 0;
        for( i = 0; i<n; ++i ) {
          dst[ofs] = tmp[i];
          ofs += stride;
        }
      }
    }
    free(tmp);
    model_plane(p_i, src_pixels, src_pixels);
  }
  void unmodel_described(BmfImage* img_at, uint8_t flags) {
    if( plane_count==1 ? (img_at[0].depth&depth_grey)!=0 : plane_count>2 ) {
      uint32_t near_lossless = unpack_bits(4);
      if( near_lossless ) {
        printf("\nnear-lossless stream (E=%d); this build only decodes E=0\n", near_lossless);
        exit(3);
      }
      near_lossless_q = 0;
    }
    read_plane_descs();
    uint8_t* plane_buf = (uint8_t*)bmf_new(img_at[0].width*img_at[0].height);
    if( flags&flags_planar )
      unmodel_planes_apart(img_at, plane_buf);
    else
      unmodel_planes_together(img_at, plane_buf);
    free(plane_buf);
  }
  void unmodel_plane(BmfImage* p_i, uint8_t* out) {
    code_plane<1>(p_i, out, nullptr);
  }
  void unmodel_planes_apart(BmfImage* img_at, uint8_t* plane_buf) {
    BmfImage plane_hdr = img_at[0];
    plane_hdr.depth = depth_one_plane;
    for( int32_t k = 0; k<plane_count; ++k ) {
      const int32_t plane = plane_desc[k].src_plane;
      plane_predictor = plane_desc[plane].flags&desc_predictor;
      plane_alt_model = (plane_desc[plane].flags&desc_alt_model)!=0;
      unmodel_plane(&plane_hdr, plane_buf);
      if( plane_predictor==pred_p1&&!plane_alt_model )
        unpredict_med(plane_buf, img_at[0].width, img_at[0].height);
      interleave_plane(img_at, plane_buf, plane);
    }
  }
  void unmodel_planes_together(BmfImage* img_at, uint8_t* plane_buf) {
    plane_predictor = plane_desc[0].flags&desc_predictor;
    plane_alt_model = (plane_desc[0].flags&desc_alt_model)!=0;
    unmodel_plane(img_at, img_at[0].pixels);
    if( plane_alt_model )
      return;
    for( int32_t k = 0; k<plane_count; ++k ) {
      const int32_t plane = plane_in_order(k);
      const int32_t pred = plane_predictor;
      if( (plane_desc[plane].flags&desc_has_refs)==0&&!pred )
        continue;
      deinterleave_plane(img_at, plane_buf, plane);
      if( pred==pred_p1 )
        unpredict_med(plane_buf, img_at[0].width, img_at[0].height);
      interleave_plane(img_at, plane_buf, plane);
    }
  }
  uint32_t unpack_bits(int32_t n) {
    const uint32_t mask = (1u<<n)-1;
    uint32_t v;
    stream.pk.free_bits -= n;
    if( stream.pk.free_bits<0 ) {
      const uint32_t w = packer_word();
      v = stream.pk.acc|((w<<((stream.pk.free_bits+n)&31))&mask);
      stream.pk.acc = w>>(-stream.pk.free_bits&31);
      stream.pk.free_bits += 32;
    } else {
      v = stream.pk.acc&mask;
      stream.pk.acc = stream.pk.acc>>n;
    }
    return v;
  }
  void write_plane_descs() {
    code_plane_descs<0>();
  }
  int32_t weight_pair_cost(const BmfImage* img, const uint8_t* plane0, int32_t plane_a, int32_t plane_b, int32_t w4, int32_t w8) {
    int32_t hist[512];
    memset(hist, 0, sizeof hist);
    const int32_t stride = img[0].stride;
    const int32_t left = -plane_count;
    const int32_t ofs_up = -stride;
    const int32_t ofs_ul = -stride-plane_count;
    const uint8_t* p = plane0-ofs_ul;
    const uint8_t* q = plane0+plane_a-ofs_ul;
    const uint8_t* r = plane0+plane_b-ofs_ul;
    for( int32_t n = img[0].width*(img[0].height-1)-1; n; --n ) {
      const int32_t cur = q[0];
      ++hist[residual_bin(p[ofs_ul]+p[0]-p[ofs_up]-p[left], w4, q[ofs_ul]+cur-q[ofs_up]-q[left], w8, r[ofs_ul]+r[0]-r[ofs_up]-(uint32_t)r[left])];
      q -= left;
      p -= left;
      r -= left;
    }
    return estimate_cost(hist, 512);
  }
  PlaneSearch search_planes(BmfImage* tile_img, uint8_t* tile_buf, uint32_t* plane_cost) {
    int32_t best_cost = 0, best_flags = 0;
    auto probe_plane = [&](int32_t pl, int32_t flags) {
                         plane_desc[pl].flags = flags;
                         model_planes(tile_img, tile_buf, pl);
                         int32_t cost = 8*(stream.cur-stream.buf);
                         packer_flush();
                         return cost;
                       };
    bool won = false;
    auto probe_keep = [&](int32_t pl, int32_t flags) {
                        const int32_t cost = probe_plane(pl, flags);
                        packer_reset();
                        won = cost<best_cost;
                        if( won ) {
                          best_cost = cost;
                          best_flags = flags;
                        }
                        return cost;
                      };
    PlaneSearch out = {0, 0, 0, 0};
    for( int32_t pi = 0; pi<plane_count; ++pi ) {
      const int32_t plane_i = plane_desc[pi].src_plane;
      if( out.n_p2 ) {
        best_cost = 0x7FFFFFFF;
      } else {
        best_cost = probe_plane(plane_i, try_mode0);
        packer_reset();
        if( best_cost!=0x7FFFFFFF )
          best_flags = 0;
      }
      const int32_t bits_f5 = probe_keep(plane_i, try_p1);
      if( bits_f5<best_cost+(best_cost>>5)||out.n_p2 )
        probe_keep(plane_i, try_p2);
      int32_t pred;
      if( pi ) {
        probe_keep(plane_i, try_refs);
        const int32_t bits_f13 = probe_keep(plane_i, try_refs_p1);
        pred = best_flags&desc_predictor;
        if( pred==pred_p2||best_cost+(best_cost>>5)>bits_f13 ) {
          probe_keep(plane_i, try_refs_p2);
          if( won )
            pred = pred_p2;
        }
      } else {
        pred = best_flags&desc_predictor;
      }
      const int32_t flag8 = (uint8_t)best_flags&desc_has_refs;
      out.bits += best_cost;
      out.n_p2 += pred==pred_p2;
      if( pred!=pred_p1 )
        pred = pred_mode0;
      out.n_p1 += pred;
      plane_cost[plane_i] = best_cost;
      out.n_refs += flag8!=0;
      plane_desc[plane_i].flags = (uint8_t)best_flags;
    }
    return out;
  }
  void choose_alpha_plane(BmfImage* img, int32_t (&hists)[8*1024], const uint8_t* data_end, int32_t row_stride) {
    const HistRows hr = hist_rows(hists);
    memset(hists, 0, sizeof hists);
    const uint8_t*const px = img[0].pixels;
    const uint8_t*const pp = &px[row_stride];
    const uint32_t uu = &px[row_stride+4]<data_end ? (uint32_t)((data_end-1-pp)/4) : 0;
    const AlphaWeights w = fit_alpha_weights(px, pp, uu);
    int32_t wa_slot = w.a, wb = w.b, wc = w.c;
    for( uint32_t quad = 0; quad<uu; ++quad ) {
      int32_t nx0 = px[4*quad+4];
      int32_t dg0 = px[4*quad]+pp[4*quad+4];
      int32_t dn0 = pp[4*quad];
      int32_t ad0 = nx0+dn0;
      int32_t dnx1 = pp[4*quad+5];
      const int32_t g0 = dg0-ad0;
      int32_t dn1 = pp[4*quad+1];
      int32_t nx2 = px[4*quad+6];
      const int32_t g1 = px[4*quad+1]+dnx1-(px[4*quad+5]+dn1);
      int32_t dnx2 = pp[4*quad+6];
      int32_t g2 = px[4*quad+2]+dnx2-(nx2+pp[4*quad+2]);
      uint16_t g1w = (uint16_t)(px[4*quad+3]+pp[4*quad+7]-pp[4*quad+3]-px[4*quad+7]);
      int16_t g1_lo = (int16_t)g1;
      g1w = g1w-512;
      int32_t pa = g1*wb+g0*wa_slot;
      int32_t pb = g2*wc;
      ++hr.r1[(g1w-(int16_t)g0)&0x3FF];
      int32_t bin_lin = (g1w-(uint16_t)((uint32_t)(pa+pb+63)>>7))&0x3FF;
      ++hr.r0[bin_lin];
      ++hr.r2[(g1w-g1_lo)&0x3FF];
      g1w = g1w-g2;
      ++hr.r3[g1w&0x3FF];
      const int32_t alpha = pp[4*quad+7]+256;
      int32_t bin_lin2 = ((uint16_t)alpha-(uint16_t)((wb*pp[4*quad+5]+wa_slot*pp[4*quad+4]+wc*(uint32_t)pp[4*quad+6]+63)>>7)+256)&0x3FF;
      ++hr.r3[bin_lin2+1024];
      int32_t r0 = alpha-pp[4*quad+4];
      ++hr.r3[r0+2048];
      int32_t r1 = alpha-pp[4*quad+5];
      ++hr.r3[r1+3072];
      int32_t r2 = alpha-pp[4*quad+6];
      ++hr.r3[r2+4096];
    }
    uint32_t cost_lin = estimate_cost(hr.r0, 1024);
    uint32_t best4 = (cost_lin>>7)+cost_lin;
    const uint32_t cost_c0 = estimate_cost(hr.r1, 1024);
    const bool pick0 = cost_c0<best4;
    if( pick0 ) {
      best4 = cost_c0;
      wa_slot = 128;
      wc = 0;
      wb = 0;
    }
    const uint32_t cost_c1 = estimate_cost(hr.r2, 1024);
    int32_t pred4 = pick0;
    if( cost_c1<best4 ) {
      best4 = cost_c1;
      pred4 = 2;
      wb = 128;
      wc = 0;
      wa_slot = 0;
    }
    const uint32_t cost_c2 = estimate_cost(hr.r3, 1024);
    if( cost_c2<best4 ) {
      pred4 = 3;
      wc = 128;
      wb = 0;
      wa_slot = 0;
    }
    plane_desc[3].weight0 = wa_slot;
    plane_desc[3].weight1 = wb;
    plane_desc[3].weight2 = wc;
    plane_desc[3].src_plane = 3;
    plane_desc[3].nrefs = 3;
    plane_desc[3].dc = widest_window(hists+1024*(4+pred4), 4, 1024, -1)+1;
  }
  template <int32_t f_DEC> void alt_model_p1_d8(uint8_t* src, int32_t width, int32_t height, uint8_t* out) {
    AltP1Block* blk = alt_p1_block_alloc(width, height);
    blk[0].d8_body<f_DEC>(src, out);
    if( blk ) p1_blocks.give(blk);
  }
  void alt_model_p1_d8_encode(uint8_t* src, int32_t i, int32_t height, uint8_t* out) {
    alt_model_p1_d8<0>(src, i, height, out);
  }
  void alt_model_p1_d8_decode(uint8_t* out, int32_t i, int32_t height) {
    alt_model_p1_d8<1>(nullptr, i, height, out);
  }
  void alt_p2_planes_alloc(AltP2Block** plane, int32_t width) {
    for( int32_t pl = 0; pl<plane_count; ++pl ) {
      AltP2Block* page = p2_blocks.take(true);
      plane[pl] = page[0].alt_p2_alloc(this, width, pl);
    }
  }
  void alt_p2_start_row(AltP2Block** plane, int32_t row, int32_t width) {
    for( int32_t k = 0; k<plane_count; ++k ) {
      if( row==0 )
        plane[k][0].seed_row0(width);
      else if( row==1 )
        plane[k][0].seed_history(width);
      plane[k][0].start_row();
    }
  }
  uint8_t*compress_to_memory(BmfImage* p_i, size_t* out_len, const CodedTail* tail) {
    reset();
    out_len[0] = 0;
    BmfFile arc;
    arc.io = BmfStream::in_memory();
    const int32_t coded = compress_image(&arc, p_i, tail);
    if( !coded ) {
      arc.close();
      return nullptr;
    }
    uint8_t* out = arc.io.take(out_len);
    arc.close();
    return out;
  }
  BmfImage*expand_from_memory(const uint8_t* data, size_t n, CodedTail** tail) {
    reset();
    BmfFile arc;
    arc.io = BmfStream::over_memory(data, n);
    BmfImage* img = expand_image(&arc, &coded_block);
    arc.close();
    if( tail )
      *tail = coded_block;
    else
      free(coded_block);
    coded_block = nullptr;
    return img;
  }
};

// Layout guards.  Several routines deliberately write past the end of one
// member into the next (the fold/fold_hi pair filled by alt_init_tables, the
// bias[] slots NbRow::predict reaches through p2_row, the ModelBlock grid
// memset that runs one record into its padding), and reset() clears exactly
// the BMFState subobject.  These assertions make any reordering that would
// silently break those a build failure instead.
static_assert(offsetof(AltP1Block, fold_hi)==offsetof(AltP1Block, fold)+256);
static_assert(offsetof(AltP1Block, unfold)==offsetof(AltP1Block, fold_hi)+256);
static_assert(offsetof(AltP2Block, fold_hi)==offsetof(AltP2Block, fold)+256);
static_assert(offsetof(AltP2Block, unfold)==offsetof(AltP2Block, fold_hi)+256);
static_assert(offsetof(AltP2Block, bias)==offsetof(AltP2Block, p2_row)+sizeof(float)*7*4);
static_assert(offsetof(ModelBlock, _pad1051664)==offsetof(ModelBlock, grid)+sizeof(FreqRec)*ModelBlock::kFreqGridCount);
static_assert(offsetof(ModelBlock, grid)+188*sizeof(FreqRec)+0x100000<=offsetof(ModelBlock, bit_root));
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
static_assert(offsetof(BMFCodec, p1_blocks)>=sizeof(BMFState));
#pragma clang diagnostic pop
static_assert(sizeof(BmfImage)==16&&offsetof(BmfImage, pixels)==16);
static_assert(offsetof(BmfImage, depth)==10&&offsetof(BmfImage, flags)==11&&offsetof(BmfImage, data_size)==12);
static_assert(sizeof(CodedTail)==8);
static_assert(sizeof(BmpHeader)==54);
static_assert(sizeof(SymEntry)==3);

BMFCodec bmf_codec;

void bmf_compress(const char* InName, const char* OutName) {
  int32_t i;
  BmfImage* p_i = read_bmp((char*)InName);
  if( !p_i )
    bmf_fatal(bmf_read_error);
  printf("File %16s, image %dx%dx%d, size - %d:", InName, p_i[0].width, p_i[0].height, p_i[0].depth&depth_bits, p_i[0].data_size);
  static BmfFile arc_store;
  BmfFile* Arc = bmf_open_file(&arc_store, (char*)OutName, 0);
  int32_t Depth = p_i[0].depth;
  if( Depth&depth_palette ) {
    if( Depth&depth_grey ) {
      p_i[0].depth = Depth^depth_palette;
    } else {
      int32_t Colours = 1<<(Depth&depth_bits);
      int32_t Step = 0x100u>>(Depth&depth_bits);
      const uint8_t* Palette = p_i[0].palette();
      int32_t Grey = 0;
      for( i = 0; i<Colours; ++i ) {
        if( Palette[3*i]!=Grey||Palette[3*i+1]!=Grey||Palette[3*i+2]!=Grey )
          break;
        Grey += Step;
      }
      if( i>=Colours )
        p_i[0].depth = (Depth|depth_grey)^depth_palette;
    }
  }
  int32_t coded_len = bmf_codec.compress_image(Arc, p_i, bmf_codec.coded_block);
  if( !coded_len )
    bmf_fatal(bmf_write_error, OutName);
  printf("%6.3f bpp\n", (double)coded_len*8.0/(double)(p_i[0].height*p_i[0].width));
  free(p_i);
}

void bmf_decompress(const char* InName, const char* OutName) {
  BmfFile* arc;
  if( void* nb = bmf_new(sizeof(BmfFile)) )
    arc = bmf_open_file((BmfFile*)nb, (char*)InName, 1);
  else
    arc = nullptr;
  BmfImage* p_i = bmf_codec.expand_image(arc, &bmf_codec.coded_block);
  if( !p_i )
    bmf_fatal(bmf_bad_file, InName);
  printf("File %16s, image %dx%dx%d, size - %d\n", InName, p_i[0].width, p_i[0].height, p_i[0].depth&depth_bits, p_i[0].data_size);
  int32_t Depth = p_i[0].depth&depth_bits;
  if( Depth==2||Depth==15||Depth==16 ) {
    printf("%s: %d bits per pixel is not a BMP depth\n", OutName, Depth);
    exit(5);
  }
  if( !write_bmp(p_i, (char*)OutName, 1) )
    bmf_fatal(bmf_write_error, OutName);
  free(bmf_codec.coded_block);
  bmf_codec.coded_block = nullptr;
  free(p_i);
  bmf_close_file(arc);
}

int32_t main(int32_t argc, char** argv) {
  const char*const* args = (const char*const*)argv;
  bmf_set_denormal_mode();
  printf("BMF lossless image compressor, v.2.01 (C) 1998-1999, 2009 by Dmitry Shkarin\n");
  int32_t mode = argc==4&&!args[1][1] ? toupper(args[1][0]) : 0;
  if( mode!='C'&&mode!='D' ) {
    printf("e-mail: <dmitry.shkarin@mtu-net.ru>;  web: http://compression.graphicon.ru/ds/\n"
           "Usage: bmf c input.bmp output     compress, always with -S -Q9\n"
           "       bmf d input output.bmp     expand\n");
    return 1;
  }
  if( mode=='C' )
    bmf_compress(args[2], args[3]);
  else
    bmf_decompress(args[2], args[3]);
  return 0;
}
