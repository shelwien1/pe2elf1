// bmf_stream.inc -- coded-stream framing, plane descriptors and small shared records
// Included from bmf.cpp; not a standalone translation unit.

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
