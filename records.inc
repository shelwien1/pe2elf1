// records.inc -- the small shared records: coded-stream framing, the bit
// packer, plane descriptors, level geometry and symbol entries.
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
  // The bit packer and the range coder share this one buffer: the packer fills
  // words forward from `cur`, then a range-coded segment starts where it left
  // off.  These two calls are the hand-off in both directions.
  void packer_reset() {
    pk.word = (uint32_t*)cur;
    pk.acc = 0;
    pk.free_bits = 0;
  }

  void packer_rewind() {
    if( cur==(uint8_t*)pk.word ) return;
    int32_t bits = pk.free_bits-8;
    if( bits<0 ) {
      pk.free_bits = bits;
      return;
    }
    uint8_t* at = cur;
    while( bits>=0 ) { --at; bits -= 8; }
    cur = at;
    pk.free_bits = bits;
  }
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
