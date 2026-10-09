// lzma.hpp - single-header raw LZMA/LZMA2 encoder, replaces <lzma.h> for x64pp.
//
// A port of the liblzma encoder (xz 5.8.1: lz_encoder.c, lz_encoder_mf.c,
// lzma_encoder.c, lzma_encoder_optimum_normal.c, lzma_encoder_optimum_fast.c,
// lzma2_encoder.c, range_encoder.h; by Igor Pavlov and Lasse Collin, 0BSD),
// cut down to the part of the liblzma API that x64pp uses:
//
//   lzma_lzma_preset()           presets 0-9, LZMA_PRESET_EXTREME
//   lzma_raw_buffer_encode()     a chain of one LZMA1 or LZMA2 filter
//   lzma_raw_encoder_memusage()  same chains
//
// The output is byte-identical to liblzma's for every option combination
// (both modes, all match finders, lc/lp/pb, preset dictionaries).  Each call
// owns all its state, so concurrent calls from several threads are fine.
#ifndef LZMA_HPP_INCLUDED
#define LZMA_HPP_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#endif

//----------------------------------------------------------------------------
// API: names, values and semantics as in liblzma

typedef unsigned char lzma_bool;
typedef uint64_t lzma_vli;
#define LZMA_VLI_C(n) UINT64_C(n)
#define LZMA_VLI_UNKNOWN UINT64_MAX

typedef enum {
  LZMA_OK = 0,
  LZMA_STREAM_END = 1,
  LZMA_NO_CHECK = 2,
  LZMA_UNSUPPORTED_CHECK = 3,
  LZMA_GET_CHECK = 4,
  LZMA_MEM_ERROR = 5,
  LZMA_MEMLIMIT_ERROR = 6,
  LZMA_FORMAT_ERROR = 7,
  LZMA_OPTIONS_ERROR = 8,
  LZMA_DATA_ERROR = 9,
  LZMA_BUF_ERROR = 10,
  LZMA_PROG_ERROR = 11
} lzma_ret;

typedef struct {
  void* (*alloc)(void* opaque, size_t nmemb, size_t size);
  void (*free)(void* opaque, void* ptr);
  void* opaque;
} lzma_allocator;

#define LZMA_FILTER_LZMA1 LZMA_VLI_C(0x4000000000000001)
#define LZMA_FILTER_LZMA2 LZMA_VLI_C(0x21)
#define LZMA_FILTERS_MAX 4

typedef struct {
  lzma_vli id;
  void* options;
} lzma_filter;

typedef enum {
  LZMA_MF_HC3 = 0x03,
  LZMA_MF_HC4 = 0x04,
  LZMA_MF_BT2 = 0x12,
  LZMA_MF_BT3 = 0x13,
  LZMA_MF_BT4 = 0x14
} lzma_match_finder;

typedef enum {
  LZMA_MODE_FAST = 1,
  LZMA_MODE_NORMAL = 2
} lzma_mode;

#define LZMA_DICT_SIZE_MIN UINT32_C(4096)
#define LZMA_DICT_SIZE_DEFAULT (UINT32_C(1) << 23)
#define LZMA_LCLP_MIN 0
#define LZMA_LCLP_MAX 4
#define LZMA_LC_DEFAULT 3
#define LZMA_LP_DEFAULT 0
#define LZMA_PB_MIN 0
#define LZMA_PB_MAX 4
#define LZMA_PB_DEFAULT 2
#define LZMA_PRESET_DEFAULT UINT32_C(6)
#define LZMA_PRESET_LEVEL_MASK UINT32_C(0x1F)
#define LZMA_PRESET_EXTREME (UINT32_C(1) << 31)

typedef struct {
  uint32_t dict_size;
  const uint8_t* preset_dict;
  uint32_t preset_dict_size;
  uint32_t lc;
  uint32_t lp;
  uint32_t pb;
  lzma_mode mode;
  uint32_t nice_len;
  lzma_match_finder mf;
  uint32_t depth;
} lzma_options_lzma;

//----------------------------------------------------------------------------
// Implementation

namespace lzma_hpp {

typedef uint16_t probability;

enum { ACT_RUN = 0, ACT_SYNC_FLUSH = 1, ACT_FINISH = 3 };  // lzma_action

// range coder
static const uint32_t RC_SHIFT_BITS = 8;
static const uint32_t RC_TOP_VALUE = UINT32_C(1) << 24;
static const uint32_t RC_BIT_MODEL_TOTAL_BITS = 11;
static const uint32_t RC_BIT_MODEL_TOTAL = UINT32_C(1) << RC_BIT_MODEL_TOTAL_BITS;
static const uint32_t RC_MOVE_BITS = 5;
static const uint32_t RC_MOVE_REDUCING_BITS = 4;
static const uint32_t RC_BIT_PRICE_SHIFT_BITS = 4;
static const uint32_t RC_PRICE_TABLE_SIZE = RC_BIT_MODEL_TOTAL >> RC_MOVE_REDUCING_BITS;
static const uint32_t RC_INFINITY_PRICE = UINT32_C(1) << 30;
static const uint32_t RC_SYMBOLS_MAX = 53;

// LZMA
static const uint32_t POS_STATES_MAX = 1 << LZMA_PB_MAX;
static const uint32_t STATES = 12;
static const uint32_t LIT_STATES = 7;
static const uint32_t LITERAL_CODER_SIZE = 0x300;
static const uint32_t LITERAL_CODERS_MAX = 1 << LZMA_LCLP_MAX;
static const uint32_t MATCH_LEN_MIN = 2;
static const uint32_t LEN_LOW_BITS = 3;
static const uint32_t LEN_LOW_SYMBOLS = 1 << LEN_LOW_BITS;
static const uint32_t LEN_MID_BITS = 3;
static const uint32_t LEN_MID_SYMBOLS = 1 << LEN_MID_BITS;
static const uint32_t LEN_HIGH_BITS = 8;
static const uint32_t LEN_HIGH_SYMBOLS = 1 << LEN_HIGH_BITS;
static const uint32_t LEN_SYMBOLS = LEN_LOW_SYMBOLS + LEN_MID_SYMBOLS + LEN_HIGH_SYMBOLS;
static const uint32_t MATCH_LEN_MAX = MATCH_LEN_MIN + LEN_SYMBOLS - 1;
static const uint32_t DIST_STATES = 4;
static const uint32_t DIST_SLOT_BITS = 6;
static const uint32_t DIST_SLOTS = 1 << DIST_SLOT_BITS;
static const uint32_t DIST_MODEL_START = 4;
static const uint32_t DIST_MODEL_END = 14;
static const uint32_t FULL_DISTANCES = 1 << (DIST_MODEL_END / 2);
static const uint32_t ALIGN_BITS = 4;
static const uint32_t ALIGN_SIZE = 1 << ALIGN_BITS;
static const uint32_t ALIGN_MASK = ALIGN_SIZE - 1;
static const uint32_t REPS = 4;
static const uint32_t OPTS = 1 << 12;
static const uint32_t LOOP_INPUT_MAX = OPTS + 1;

// LZMA2
static const uint32_t LZMA2_CHUNK_MAX = UINT32_C(1) << 16;
static const uint32_t LZMA2_UNCOMPRESSED_MAX = UINT32_C(1) << 21;
static const uint32_t LZMA2_HEADER_MAX = 6;
static const uint32_t LZMA2_HEADER_UNCOMPRESSED = 3;

// match finder
static const uint32_t HASH_2_SIZE = UINT32_C(1) << 10;
static const uint32_t HASH_3_SIZE = UINT32_C(1) << 16;
static const uint32_t HASH_2_MASK = HASH_2_SIZE - 1;
static const uint32_t HASH_3_MASK = HASH_3_SIZE - 1;
static const uint32_t FIX_3_HASH_SIZE = HASH_2_SIZE;
static const uint32_t FIX_4_HASH_SIZE = HASH_2_SIZE + HASH_3_SIZE;
static const uint32_t EMPTY_HASH_VALUE = 0;
static const uint32_t MUST_NORMALIZE_POS = UINT32_MAX;
static const uint32_t MEMCMPLEN_EXTRA = 16;  // memcmplen() may read this far past write_pos

// lzma_rc_prices[] and the CRC32 table used by the match finder hashes
struct Tables {
  uint8_t prices[RC_PRICE_TABLE_SIZE];
  uint32_t crc[256];
  Tables() {
    for (uint32_t i = (UINT32_C(1) << RC_MOVE_REDUCING_BITS) / 2; i < RC_BIT_MODEL_TOTAL;
         i += UINT32_C(1) << RC_MOVE_REDUCING_BITS) {
      uint32_t w = i, bit_count = 0;
      for (uint32_t j = 0; j < RC_BIT_PRICE_SHIFT_BITS; ++j) {
        w *= w;
        bit_count <<= 1;
        while (w >= (UINT32_C(1) << 16)) { w >>= 1; ++bit_count; }
      }
      prices[i >> RC_MOVE_REDUCING_BITS] = (uint8_t)((RC_BIT_MODEL_TOTAL_BITS << RC_BIT_PRICE_SHIFT_BITS) - 15 - bit_count);
    }
    for (uint32_t i = 0; i < 256; ++i) {
      uint32_t r = i;
      for (int j = 0; j < 8; ++j) r = (r >> 1) ^ (UINT32_C(0xEDB88320) & (0u - (r & 1)));
      crc[i] = r;
    }
  }
};
static const Tables tables;

static inline uint32_t my_min(uint32_t a, uint32_t b) { return a < b ? a : b; }
static inline uint32_t my_max(uint32_t a, uint32_t b) { return a > b ? a : b; }

static inline bool not_equal_16(const uint8_t* a, const uint8_t* b) {
  uint16_t x, y;
  memcpy(&x, a, 2);
  memcpy(&y, b, 2);
  return x != y;
}

// Length of the common prefix of buf1 and buf2, starting from len, at most limit.
static inline uint32_t memcmplen(const uint8_t* buf1, const uint8_t* buf2, uint32_t len, uint32_t limit) {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  while (len < limit) {
    uint64_t a, b;
    memcpy(&a, buf1 + len, 8);
    memcpy(&b, buf2 + len, 8);
    const uint64_t x = a ^ b;
    if (x != 0) {
      len += (uint32_t)__builtin_ctzll(x) >> 3;
      return my_min(len, limit);
    }
    len += 8;
  }
  return limit;
#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_ARM64))
  while (len < limit) {
    uint64_t a, b;
    memcpy(&a, buf1 + len, 8);
    memcpy(&b, buf2 + len, 8);
    const uint64_t x = a ^ b;
    if (x != 0) {
      unsigned long i;
      _BitScanForward64(&i, x);
      len += (uint32_t)i >> 3;
      return my_min(len, limit);
    }
    len += 8;
  }
  return limit;
#else
  while (len < limit && buf1[len] == buf2[len]) ++len;
  return len;
#endif
}

static inline uint32_t bsr32(uint32_t n) {  // n != 0
#if defined(__GNUC__) || defined(__clang__)
  return 31 - (uint32_t)__builtin_clz(n);
#elif defined(_MSC_VER)
  unsigned long i;
  _BitScanReverse(&i, n);
  return (uint32_t)i;
#else
  uint32_t i = 31;
  while ((n & (UINT32_C(1) << i)) == 0) --i;
  return i;
#endif
}

// distance slot: the highest two bits of the distance and their position
static inline uint32_t get_dist_slot_2(uint32_t dist) {
  const uint32_t i = bsr32(dist);
  return i + i + ((dist >> (i - 1)) & 1);
}
static inline uint32_t get_dist_slot(uint32_t dist) { return dist <= 4 ? dist : get_dist_slot_2(dist); }

//----------------------------------------------------------------------------
// memory

static inline void* mem_alloc(size_t size, const lzma_allocator* a) {
  if (size == 0) size = 1;
  if (a != NULL && a->alloc != NULL) return a->alloc(a->opaque, 1, size);
  return malloc(size);
}

static inline void* mem_alloc_zero(size_t size, const lzma_allocator* a) {
  if (size == 0) size = 1;
  if (a != NULL && a->alloc != NULL) {
    void* p = a->alloc(a->opaque, 1, size);
    if (p != NULL) memset(p, 0, size);
    return p;
  }
  return calloc(1, size);
}

static inline void mem_free(void* p, const lzma_allocator* a) {
  if (p == NULL) return;
  if (a != NULL && a->free != NULL) a->free(a->opaque, p);
  else free(p);
}

//----------------------------------------------------------------------------
// LZ window and match finders (lz_encoder.h, lz_encoder_mf.c)

struct Match {
  uint32_t len;
  uint32_t dist;
};

struct MF;
typedef uint32_t (*mf_find_func)(MF* mf, Match* matches);
typedef void (*mf_skip_func)(MF* mf, uint32_t num);

struct MF {
  uint8_t* buffer;            // window
  uint32_t size;              // allocated size of buffer (without MEMCMPLEN_EXTRA)
  uint32_t keep_size_before;  // history kept before read_pos
  uint32_t keep_size_after;   // lookahead kept after read_pos while action == ACT_RUN
  uint32_t offset;            // buffer[hash value - offset] is the byte at that position
  uint32_t read_pos;          // next byte to run through the match finder
  uint32_t read_ahead;        // bytes run through the match finder but not encoded yet
  uint32_t read_limit;        // read_pos < read_limit: enough input for one encoder loop
  uint32_t write_pos;         // end of valid data in buffer
  uint32_t pending;           // bytes not hashed before read_pos (after sync flush)
  mf_find_func find;
  mf_skip_func skip;
  uint32_t* hash;
  uint32_t* son;
  uint32_t cyclic_pos;
  uint32_t cyclic_size;  // dict_size + 1
  uint32_t hash_mask;
  uint32_t depth;
  uint32_t nice_len;
  uint32_t match_len_max;
  int action;
  uint32_t hash_count;
  uint32_t sons_count;
};

struct LZOptions {
  size_t before_size;
  size_t dict_size;
  size_t after_size;
  size_t match_len_max;
  size_t nice_len;
  lzma_match_finder match_finder;
  uint32_t depth;
  const uint8_t* preset_dict;
  uint32_t preset_dict_size;
};

static inline uint32_t mf_get_hash_bytes(lzma_match_finder match_finder) { return (uint32_t)match_finder & 0x0F; }
static inline const uint8_t* mf_ptr(const MF* mf) { return mf->buffer + mf->read_pos; }
static inline uint32_t mf_avail(const MF* mf) { return mf->write_pos - mf->read_pos; }
static inline uint32_t mf_unencoded(const MF* mf) { return mf->write_pos - mf->read_pos + mf->read_ahead; }

static inline void mf_skip(MF* mf, uint32_t amount) {
  if (amount != 0) {
    mf->skip(mf, amount);
    mf->read_ahead += amount;
  }
}

// lzma_mf_find(): returns the length of the longest match
static inline uint32_t mf_find(MF* mf, uint32_t* count_ptr, Match* matches) {
  const uint32_t count = mf->find(mf, matches);
  uint32_t len_best = 0;
  if (count > 0) {
    len_best = matches[count - 1].len;
    // a match of nice_len: extend it as far as possible
    if (len_best == mf->nice_len) {
      uint32_t limit = mf_avail(mf) + 1;
      if (limit > mf->match_len_max) limit = mf->match_len_max;
      const uint8_t* p1 = mf_ptr(mf) - 1;
      const uint8_t* p2 = p1 - matches[count - 1].dist - 1;
      len_best = memcmplen(p1, p2, len_best, limit);
    }
  }
  *count_ptr = count;
  ++mf->read_ahead;
  return len_best;
}

static void mf_normalize(MF* mf) {
  const uint32_t subvalue = MUST_NORMALIZE_POS - mf->cyclic_size;
  for (uint32_t i = 0; i < mf->hash_count; ++i)
    mf->hash[i] = mf->hash[i] <= subvalue ? EMPTY_HASH_VALUE : mf->hash[i] - subvalue;
  for (uint32_t i = 0; i < mf->sons_count; ++i)
    mf->son[i] = mf->son[i] <= subvalue ? EMPTY_HASH_VALUE : mf->son[i] - subvalue;
  mf->offset -= subvalue;
}

static inline void move_pos(MF* mf) {
  if (++mf->cyclic_pos == mf->cyclic_size) mf->cyclic_pos = 0;
  ++mf->read_pos;
  if (mf->read_pos + mf->offset == UINT32_MAX) mf_normalize(mf);
}

static inline void move_pending(MF* mf) {
  ++mf->read_pos;
  ++mf->pending;
}

// header() of lz_encoder_mf.c: false if too little input to run the match finder
static inline bool mf_header(MF* mf, bool is_bt, uint32_t len_min, uint32_t& len_limit) {
  len_limit = mf_avail(mf);
  if (mf->nice_len <= len_limit) {
    len_limit = mf->nice_len;
  } else if (len_limit < len_min || (is_bt && mf->action == ACT_SYNC_FLUSH)) {
    move_pending(mf);
    return false;
  }
  return true;
}

static inline Match* hc_find_func(const uint32_t len_limit, const uint32_t pos, const uint8_t* const cur,
                                  uint32_t cur_match, uint32_t depth, uint32_t* const son, const uint32_t cyclic_pos,
                                  const uint32_t cyclic_size, Match* matches, uint32_t len_best) {
  son[cyclic_pos] = cur_match;
  for (;;) {
    const uint32_t delta = pos - cur_match;
    if (depth-- == 0 || delta >= cyclic_size) return matches;
    const uint8_t* const pb = cur - delta;
    cur_match = son[cyclic_pos - delta + (delta > cyclic_pos ? cyclic_size : 0)];
    if (pb[len_best] == cur[len_best] && pb[0] == cur[0]) {
      uint32_t len = memcmplen(pb, cur, 1, len_limit);
      if (len_best < len) {
        len_best = len;
        matches->len = len;
        matches->dist = delta - 1;
        ++matches;
        if (len == len_limit) return matches;
      }
    }
  }
}

static inline Match* bt_find_func(const uint32_t len_limit, const uint32_t pos, const uint8_t* const cur,
                                  uint32_t cur_match, uint32_t depth, uint32_t* const son, const uint32_t cyclic_pos,
                                  const uint32_t cyclic_size, Match* matches, uint32_t len_best) {
  uint32_t* ptr0 = son + (cyclic_pos << 1) + 1;
  uint32_t* ptr1 = son + (cyclic_pos << 1);
  uint32_t len0 = 0, len1 = 0;
  for (;;) {
    const uint32_t delta = pos - cur_match;
    if (depth-- == 0 || delta >= cyclic_size) {
      *ptr0 = EMPTY_HASH_VALUE;
      *ptr1 = EMPTY_HASH_VALUE;
      return matches;
    }
    uint32_t* const pair = son + ((cyclic_pos - delta + (delta > cyclic_pos ? cyclic_size : 0)) << 1);
    const uint8_t* const pb = cur - delta;
    uint32_t len = my_min(len0, len1);
    if (pb[len] == cur[len]) {
      len = memcmplen(pb, cur, len + 1, len_limit);
      if (len_best < len) {
        len_best = len;
        matches->len = len;
        matches->dist = delta - 1;
        ++matches;
        if (len == len_limit) {
          *ptr1 = pair[0];
          *ptr0 = pair[1];
          return matches;
        }
      }
    }
    if (pb[len] < cur[len]) {
      *ptr1 = cur_match;
      ptr1 = pair + 1;
      cur_match = *ptr1;
      len1 = len;
    } else {
      *ptr0 = cur_match;
      ptr0 = pair;
      cur_match = *ptr0;
      len0 = len;
    }
  }
}

static inline void bt_skip_func(const uint32_t len_limit, const uint32_t pos, const uint8_t* const cur,
                                uint32_t cur_match, uint32_t depth, uint32_t* const son, const uint32_t cyclic_pos,
                                const uint32_t cyclic_size) {
  uint32_t* ptr0 = son + (cyclic_pos << 1) + 1;
  uint32_t* ptr1 = son + (cyclic_pos << 1);
  uint32_t len0 = 0, len1 = 0;
  for (;;) {
    const uint32_t delta = pos - cur_match;
    if (depth-- == 0 || delta >= cyclic_size) {
      *ptr0 = EMPTY_HASH_VALUE;
      *ptr1 = EMPTY_HASH_VALUE;
      return;
    }
    uint32_t* pair = son + ((cyclic_pos - delta + (delta > cyclic_pos ? cyclic_size : 0)) << 1);
    const uint8_t* pb = cur - delta;
    uint32_t len = my_min(len0, len1);
    if (pb[len] == cur[len]) {
      len = memcmplen(pb, cur, len + 1, len_limit);
      if (len == len_limit) {
        *ptr1 = pair[0];
        *ptr0 = pair[1];
        return;
      }
    }
    if (pb[len] < cur[len]) {
      *ptr1 = cur_match;
      ptr1 = pair + 1;
      cur_match = *ptr1;
      len1 = len;
    } else {
      *ptr0 = cur_match;
      ptr0 = pair;
      cur_match = *ptr0;
      len0 = len;
    }
  }
}

static inline void hc_skip(MF* mf, uint32_t cur_match) {
  mf->son[mf->cyclic_pos] = cur_match;
  move_pos(mf);
}

static inline void bt_skip(MF* mf, uint32_t len_limit, uint32_t pos, const uint8_t* cur, uint32_t cur_match) {
  bt_skip_func(len_limit, pos, cur, cur_match, mf->depth, mf->son, mf->cyclic_pos, mf->cyclic_size);
  move_pos(mf);
}

static uint32_t mf_hc3_find(MF* mf, Match* matches) {
  uint32_t len_limit;
  if (!mf_header(mf, false, 3, len_limit)) return 0;
  const uint8_t* cur = mf_ptr(mf);
  const uint32_t pos = mf->read_pos + mf->offset;
  uint32_t matches_count = 0;
  const uint32_t temp = tables.crc[cur[0]] ^ cur[1];
  const uint32_t hash_2_value = temp & HASH_2_MASK;
  const uint32_t hash_value = (temp ^ ((uint32_t)cur[2] << 8)) & mf->hash_mask;
  const uint32_t delta2 = pos - mf->hash[hash_2_value];
  const uint32_t cur_match = mf->hash[FIX_3_HASH_SIZE + hash_value];
  mf->hash[hash_2_value] = pos;
  mf->hash[FIX_3_HASH_SIZE + hash_value] = pos;
  uint32_t len_best = 2;
  if (delta2 < mf->cyclic_size && *(cur - delta2) == *cur) {
    len_best = memcmplen(cur - delta2, cur, len_best, len_limit);
    matches[0].len = len_best;
    matches[0].dist = delta2 - 1;
    matches_count = 1;
    if (len_best == len_limit) {
      hc_skip(mf, cur_match);
      return 1;
    }
  }
  matches_count = (uint32_t)(hc_find_func(len_limit, pos, cur, cur_match, mf->depth, mf->son, mf->cyclic_pos,
                                          mf->cyclic_size, matches + matches_count, len_best) - matches);
  move_pos(mf);
  return matches_count;
}

static void mf_hc3_skip(MF* mf, uint32_t amount) {
  do {
    if (mf_avail(mf) < 3) {
      move_pending(mf);
      continue;
    }
    const uint8_t* cur = mf_ptr(mf);
    const uint32_t pos = mf->read_pos + mf->offset;
    const uint32_t temp = tables.crc[cur[0]] ^ cur[1];
    const uint32_t hash_2_value = temp & HASH_2_MASK;
    const uint32_t hash_value = (temp ^ ((uint32_t)cur[2] << 8)) & mf->hash_mask;
    const uint32_t cur_match = mf->hash[FIX_3_HASH_SIZE + hash_value];
    mf->hash[hash_2_value] = pos;
    mf->hash[FIX_3_HASH_SIZE + hash_value] = pos;
    hc_skip(mf, cur_match);
  } while (--amount != 0);
}

static uint32_t mf_hc4_find(MF* mf, Match* matches) {
  uint32_t len_limit;
  if (!mf_header(mf, false, 4, len_limit)) return 0;
  const uint8_t* cur = mf_ptr(mf);
  const uint32_t pos = mf->read_pos + mf->offset;
  uint32_t matches_count = 0;
  const uint32_t temp = tables.crc[cur[0]] ^ cur[1];
  const uint32_t hash_2_value = temp & HASH_2_MASK;
  const uint32_t hash_3_value = (temp ^ ((uint32_t)cur[2] << 8)) & HASH_3_MASK;
  const uint32_t hash_value = (temp ^ ((uint32_t)cur[2] << 8) ^ (tables.crc[cur[3]] << 5)) & mf->hash_mask;
  uint32_t delta2 = pos - mf->hash[hash_2_value];
  const uint32_t delta3 = pos - mf->hash[FIX_3_HASH_SIZE + hash_3_value];
  const uint32_t cur_match = mf->hash[FIX_4_HASH_SIZE + hash_value];
  mf->hash[hash_2_value] = pos;
  mf->hash[FIX_3_HASH_SIZE + hash_3_value] = pos;
  mf->hash[FIX_4_HASH_SIZE + hash_value] = pos;
  uint32_t len_best = 1;
  if (delta2 < mf->cyclic_size && *(cur - delta2) == *cur) {
    len_best = 2;
    matches[0].len = 2;
    matches[0].dist = delta2 - 1;
    matches_count = 1;
  }
  if (delta2 != delta3 && delta3 < mf->cyclic_size && *(cur - delta3) == *cur) {
    len_best = 3;
    matches[matches_count++].dist = delta3 - 1;
    delta2 = delta3;
  }
  if (matches_count != 0) {
    len_best = memcmplen(cur - delta2, cur, len_best, len_limit);
    matches[matches_count - 1].len = len_best;
    if (len_best == len_limit) {
      hc_skip(mf, cur_match);
      return matches_count;
    }
  }
  if (len_best < 3) len_best = 3;
  matches_count = (uint32_t)(hc_find_func(len_limit, pos, cur, cur_match, mf->depth, mf->son, mf->cyclic_pos,
                                          mf->cyclic_size, matches + matches_count, len_best) - matches);
  move_pos(mf);
  return matches_count;
}

static void mf_hc4_skip(MF* mf, uint32_t amount) {
  do {
    if (mf_avail(mf) < 4) {
      move_pending(mf);
      continue;
    }
    const uint8_t* cur = mf_ptr(mf);
    const uint32_t pos = mf->read_pos + mf->offset;
    const uint32_t temp = tables.crc[cur[0]] ^ cur[1];
    const uint32_t hash_2_value = temp & HASH_2_MASK;
    const uint32_t hash_3_value = (temp ^ ((uint32_t)cur[2] << 8)) & HASH_3_MASK;
    const uint32_t hash_value = (temp ^ ((uint32_t)cur[2] << 8) ^ (tables.crc[cur[3]] << 5)) & mf->hash_mask;
    const uint32_t cur_match = mf->hash[FIX_4_HASH_SIZE + hash_value];
    mf->hash[hash_2_value] = pos;
    mf->hash[FIX_3_HASH_SIZE + hash_3_value] = pos;
    mf->hash[FIX_4_HASH_SIZE + hash_value] = pos;
    hc_skip(mf, cur_match);
  } while (--amount != 0);
}

static uint32_t mf_bt2_find(MF* mf, Match* matches) {
  uint32_t len_limit;
  if (!mf_header(mf, true, 2, len_limit)) return 0;
  const uint8_t* cur = mf_ptr(mf);
  const uint32_t pos = mf->read_pos + mf->offset;
  const uint32_t hash_value = (uint32_t)cur[0] | ((uint32_t)cur[1] << 8);
  const uint32_t cur_match = mf->hash[hash_value];
  mf->hash[hash_value] = pos;
  const uint32_t matches_count = (uint32_t)(bt_find_func(len_limit, pos, cur, cur_match, mf->depth, mf->son,
                                                         mf->cyclic_pos, mf->cyclic_size, matches, 1) - matches);
  move_pos(mf);
  return matches_count;
}

static void mf_bt2_skip(MF* mf, uint32_t amount) {
  do {
    uint32_t len_limit;
    if (!mf_header(mf, true, 2, len_limit)) continue;
    const uint8_t* cur = mf_ptr(mf);
    const uint32_t pos = mf->read_pos + mf->offset;
    const uint32_t hash_value = (uint32_t)cur[0] | ((uint32_t)cur[1] << 8);
    const uint32_t cur_match = mf->hash[hash_value];
    mf->hash[hash_value] = pos;
    bt_skip(mf, len_limit, pos, cur, cur_match);
  } while (--amount != 0);
}

static uint32_t mf_bt3_find(MF* mf, Match* matches) {
  uint32_t len_limit;
  if (!mf_header(mf, true, 3, len_limit)) return 0;
  const uint8_t* cur = mf_ptr(mf);
  const uint32_t pos = mf->read_pos + mf->offset;
  uint32_t matches_count = 0;
  const uint32_t temp = tables.crc[cur[0]] ^ cur[1];
  const uint32_t hash_2_value = temp & HASH_2_MASK;
  const uint32_t hash_value = (temp ^ ((uint32_t)cur[2] << 8)) & mf->hash_mask;
  const uint32_t delta2 = pos - mf->hash[hash_2_value];
  const uint32_t cur_match = mf->hash[FIX_3_HASH_SIZE + hash_value];
  mf->hash[hash_2_value] = pos;
  mf->hash[FIX_3_HASH_SIZE + hash_value] = pos;
  uint32_t len_best = 2;
  if (delta2 < mf->cyclic_size && *(cur - delta2) == *cur) {
    len_best = memcmplen(cur, cur - delta2, len_best, len_limit);
    matches[0].len = len_best;
    matches[0].dist = delta2 - 1;
    matches_count = 1;
    if (len_best == len_limit) {
      bt_skip(mf, len_limit, pos, cur, cur_match);
      return 1;
    }
  }
  matches_count = (uint32_t)(bt_find_func(len_limit, pos, cur, cur_match, mf->depth, mf->son, mf->cyclic_pos,
                                          mf->cyclic_size, matches + matches_count, len_best) - matches);
  move_pos(mf);
  return matches_count;
}

static void mf_bt3_skip(MF* mf, uint32_t amount) {
  do {
    uint32_t len_limit;
    if (!mf_header(mf, true, 3, len_limit)) continue;
    const uint8_t* cur = mf_ptr(mf);
    const uint32_t pos = mf->read_pos + mf->offset;
    const uint32_t temp = tables.crc[cur[0]] ^ cur[1];
    const uint32_t hash_2_value = temp & HASH_2_MASK;
    const uint32_t hash_value = (temp ^ ((uint32_t)cur[2] << 8)) & mf->hash_mask;
    const uint32_t cur_match = mf->hash[FIX_3_HASH_SIZE + hash_value];
    mf->hash[hash_2_value] = pos;
    mf->hash[FIX_3_HASH_SIZE + hash_value] = pos;
    bt_skip(mf, len_limit, pos, cur, cur_match);
  } while (--amount != 0);
}

static uint32_t mf_bt4_find(MF* mf, Match* matches) {
  uint32_t len_limit;
  if (!mf_header(mf, true, 4, len_limit)) return 0;
  const uint8_t* cur = mf_ptr(mf);
  const uint32_t pos = mf->read_pos + mf->offset;
  uint32_t matches_count = 0;
  const uint32_t temp = tables.crc[cur[0]] ^ cur[1];
  const uint32_t hash_2_value = temp & HASH_2_MASK;
  const uint32_t hash_3_value = (temp ^ ((uint32_t)cur[2] << 8)) & HASH_3_MASK;
  const uint32_t hash_value = (temp ^ ((uint32_t)cur[2] << 8) ^ (tables.crc[cur[3]] << 5)) & mf->hash_mask;
  uint32_t delta2 = pos - mf->hash[hash_2_value];
  const uint32_t delta3 = pos - mf->hash[FIX_3_HASH_SIZE + hash_3_value];
  const uint32_t cur_match = mf->hash[FIX_4_HASH_SIZE + hash_value];
  mf->hash[hash_2_value] = pos;
  mf->hash[FIX_3_HASH_SIZE + hash_3_value] = pos;
  mf->hash[FIX_4_HASH_SIZE + hash_value] = pos;
  uint32_t len_best = 1;
  if (delta2 < mf->cyclic_size && *(cur - delta2) == *cur) {
    len_best = 2;
    matches[0].len = 2;
    matches[0].dist = delta2 - 1;
    matches_count = 1;
  }
  if (delta2 != delta3 && delta3 < mf->cyclic_size && *(cur - delta3) == *cur) {
    len_best = 3;
    matches[matches_count++].dist = delta3 - 1;
    delta2 = delta3;
  }
  if (matches_count != 0) {
    len_best = memcmplen(cur, cur - delta2, len_best, len_limit);
    matches[matches_count - 1].len = len_best;
    if (len_best == len_limit) {
      bt_skip(mf, len_limit, pos, cur, cur_match);
      return matches_count;
    }
  }
  if (len_best < 3) len_best = 3;
  matches_count = (uint32_t)(bt_find_func(len_limit, pos, cur, cur_match, mf->depth, mf->son, mf->cyclic_pos,
                                          mf->cyclic_size, matches + matches_count, len_best) - matches);
  move_pos(mf);
  return matches_count;
}

static void mf_bt4_skip(MF* mf, uint32_t amount) {
  do {
    uint32_t len_limit;
    if (!mf_header(mf, true, 4, len_limit)) continue;
    const uint8_t* cur = mf_ptr(mf);
    const uint32_t pos = mf->read_pos + mf->offset;
    const uint32_t temp = tables.crc[cur[0]] ^ cur[1];
    const uint32_t hash_2_value = temp & HASH_2_MASK;
    const uint32_t hash_3_value = (temp ^ ((uint32_t)cur[2] << 8)) & HASH_3_MASK;
    const uint32_t hash_value = (temp ^ ((uint32_t)cur[2] << 8) ^ (tables.crc[cur[3]] << 5)) & mf->hash_mask;
    const uint32_t cur_match = mf->hash[FIX_4_HASH_SIZE + hash_value];
    mf->hash[hash_2_value] = pos;
    mf->hash[FIX_3_HASH_SIZE + hash_3_value] = pos;
    mf->hash[FIX_4_HASH_SIZE + hash_value] = pos;
    bt_skip(mf, len_limit, pos, cur, cur_match);
  } while (--amount != 0);
}

static inline bool is_enc_dict_size_valid(size_t size) {
  return size >= LZMA_DICT_SIZE_MIN && size <= (UINT32_C(1) << 30) + (UINT32_C(1) << 29);
}

// lz_encoder_prepare(): sizes of the buffers and the match finder setup; true on error
static bool lz_encoder_prepare(MF* mf, const LZOptions* lz) {
  if (!is_enc_dict_size_valid(lz->dict_size) || lz->nice_len > lz->match_len_max) return true;
  mf->keep_size_before = (uint32_t)(lz->before_size + lz->dict_size);
  mf->keep_size_after = (uint32_t)(lz->after_size + lz->match_len_max);
  // extra space to make the memmove()s in move_window() rarer
  uint32_t reserve = (uint32_t)(lz->dict_size / 2);
  if (reserve > (UINT32_C(1) << 30)) reserve /= 2;
  reserve += (uint32_t)((lz->before_size + lz->match_len_max + lz->after_size) / 2 + (UINT32_C(1) << 19));
  mf->size = mf->keep_size_before + reserve + mf->keep_size_after;
  mf->match_len_max = (uint32_t)lz->match_len_max;
  mf->nice_len = (uint32_t)lz->nice_len;
  mf->cyclic_size = (uint32_t)lz->dict_size + 1;
  switch (lz->match_finder) {
    case LZMA_MF_HC3: mf->find = &mf_hc3_find; mf->skip = &mf_hc3_skip; break;
    case LZMA_MF_HC4: mf->find = &mf_hc4_find; mf->skip = &mf_hc4_skip; break;
    case LZMA_MF_BT2: mf->find = &mf_bt2_find; mf->skip = &mf_bt2_skip; break;
    case LZMA_MF_BT3: mf->find = &mf_bt3_find; mf->skip = &mf_bt3_skip; break;
    case LZMA_MF_BT4: mf->find = &mf_bt4_find; mf->skip = &mf_bt4_skip; break;
    default: return true;
  }
  const uint32_t hash_bytes = mf_get_hash_bytes(lz->match_finder);
  const bool is_bt = (lz->match_finder & 0x10) != 0;
  uint32_t hs;
  if (hash_bytes == 2) {
    hs = 0xFFFF;
  } else {
    // dictionary size rounded up to 2^n - 1, for use as a hash mask
    hs = (uint32_t)lz->dict_size - 1;
    hs |= hs >> 1;
    hs |= hs >> 2;
    hs |= hs >> 4;
    hs |= hs >> 8;
    hs >>= 1;
    hs |= 0xFFFF;
    if (hs > (UINT32_C(1) << 24)) {
      if (hash_bytes == 3) hs = (UINT32_C(1) << 24) - 1;
      else hs >>= 1;
    }
  }
  mf->hash_mask = hs;
  ++hs;
  if (hash_bytes > 2) hs += HASH_2_SIZE;
  if (hash_bytes > 3) hs += HASH_3_SIZE;
  mf->hash_count = hs;
  mf->sons_count = mf->cyclic_size;
  if (is_bt) mf->sons_count *= 2;
  mf->depth = lz->depth;
  if (mf->depth == 0) mf->depth = is_bt ? 16 + mf->nice_len / 2 : 4 + mf->nice_len / 4;
  return false;
}

// lz_encoder_init(): allocation and the initial state; true on error
static bool lz_encoder_init(MF* mf, const LZOptions* lz, const lzma_allocator* allocator) {
  mf->buffer = (uint8_t*)mem_alloc((size_t)mf->size + MEMCMPLEN_EXTRA, allocator);
  if (mf->buffer == NULL) return true;
  memset(mf->buffer + mf->size, 0, MEMCMPLEN_EXTRA);
  // positions start from cyclic_size, so that 0 (empty) is always too far away
  mf->offset = mf->cyclic_size;
  mf->read_pos = 0;
  mf->read_ahead = 0;
  mf->read_limit = 0;
  mf->write_pos = 0;
  mf->pending = 0;
  if ((uint64_t)mf->hash_count * sizeof(uint32_t) > SIZE_MAX || (uint64_t)mf->sons_count * sizeof(uint32_t) > SIZE_MAX)
    return true;
  // son[] needs no initialization: unused parts are never read
  mf->hash = (uint32_t*)mem_alloc_zero((size_t)mf->hash_count * sizeof(uint32_t), allocator);
  mf->son = (uint32_t*)mem_alloc((size_t)mf->sons_count * sizeof(uint32_t), allocator);
  if (mf->hash == NULL || mf->son == NULL) return true;
  mf->cyclic_pos = 0;
  if (lz->preset_dict != NULL && lz->preset_dict_size > 0) {
    // only the tail of a preset dictionary bigger than the window is used
    mf->write_pos = my_min(lz->preset_dict_size, mf->size);
    memcpy(mf->buffer, lz->preset_dict + lz->preset_dict_size - mf->write_pos, mf->write_pos);
    mf->action = ACT_SYNC_FLUSH;
    mf->skip(mf, mf->write_pos);
  }
  mf->action = ACT_RUN;
  return false;
}

// copies at most *left bytes of history before read_pos to out (LZMA2 uncompressed chunks)
static inline void mf_read(MF* mf, uint8_t* out, size_t* out_pos, size_t out_size, size_t* left) {
  const size_t out_avail = out_size - *out_pos;
  const size_t copy_size = out_avail < *left ? out_avail : *left;
  memcpy(out + *out_pos, mf->buffer + mf->read_pos - *left, copy_size);
  *out_pos += copy_size;
  *left -= copy_size;
}

//----------------------------------------------------------------------------
// Range encoder (range_encoder.h, price.h)
//
// Symbols are queued and coded after the whole LZMA symbol has been chosen;
// the probabilities are updated then, which the price updates depend on.

enum { RC_BIT_0, RC_BIT_1, RC_DIRECT_0, RC_DIRECT_1, RC_FLUSH };

struct RangeEncoder {
  uint64_t low;
  uint64_t cache_size;
  uint32_t range;
  uint8_t cache;
  size_t count;  // symbols queued
  size_t pos;    // rc_encode() position in the queue
  uint32_t symbols[RC_SYMBOLS_MAX];
  probability* probs[RC_SYMBOLS_MAX];
};

static inline void bit_reset(probability& p) { p = (probability)(RC_BIT_MODEL_TOTAL >> 1); }
static inline void bittree_reset(probability* probs, uint32_t bit_levels) {
  for (uint32_t i = 0; i < (UINT32_C(1) << bit_levels); ++i) bit_reset(probs[i]);
}

static inline void rc_reset(RangeEncoder* rc) {
  rc->low = 0;
  rc->cache_size = 1;
  rc->range = UINT32_MAX;
  rc->cache = 0;
  rc->count = 0;
  rc->pos = 0;
}

static inline void rc_bit(RangeEncoder* rc, probability* prob, uint32_t bit) {
  rc->symbols[rc->count] = bit;
  rc->probs[rc->count] = prob;
  ++rc->count;
}

static inline void rc_bittree(RangeEncoder* rc, probability* probs, uint32_t bit_count, uint32_t symbol) {
  uint32_t model_index = 1;
  do {
    const uint32_t bit = (symbol >> --bit_count) & 1;
    rc_bit(rc, &probs[model_index], bit);
    model_index = (model_index << 1) + bit;
  } while (bit_count != 0);
}

static inline void rc_bittree_reverse(RangeEncoder* rc, probability* probs, uint32_t bit_count, uint32_t symbol) {
  uint32_t model_index = 1;
  do {
    const uint32_t bit = symbol & 1;
    symbol >>= 1;
    rc_bit(rc, &probs[model_index], bit);
    model_index = (model_index << 1) + bit;
  } while (--bit_count != 0);
}

static inline void rc_direct(RangeEncoder* rc, uint32_t value, uint32_t bit_count) {
  do {
    rc->symbols[rc->count++] = RC_DIRECT_0 + ((value >> --bit_count) & 1);
  } while (bit_count != 0);
}

static inline void rc_flush(RangeEncoder* rc) {
  for (size_t i = 0; i < 5; ++i) rc->symbols[rc->count++] = RC_FLUSH;
}

// true if out is full
static inline bool rc_shift_low(RangeEncoder* rc, uint8_t* out, size_t* out_pos, size_t out_size) {
  if ((uint32_t)rc->low < UINT32_C(0xFF000000) || (uint32_t)(rc->low >> 32) != 0) {
    do {
      if (*out_pos == out_size) return true;
      out[*out_pos] = (uint8_t)(rc->cache + (uint8_t)(rc->low >> 32));
      ++*out_pos;
      rc->cache = 0xFF;
    } while (--rc->cache_size != 0);
    rc->cache = (uint8_t)((rc->low >> 24) & 0xFF);
  }
  ++rc->cache_size;
  rc->low = (rc->low & 0x00FFFFFF) << RC_SHIFT_BITS;
  return false;
}

// codes the queued symbols; true if out is full
static inline bool rc_encode(RangeEncoder* rc, uint8_t* out, size_t* out_pos, size_t out_size) {
  while (rc->pos < rc->count) {
    if (rc->range < RC_TOP_VALUE) {
      if (rc_shift_low(rc, out, out_pos, out_size)) return true;
      rc->range <<= RC_SHIFT_BITS;
    }
    switch (rc->symbols[rc->pos]) {
      case RC_BIT_0: {
        probability prob = *rc->probs[rc->pos];
        rc->range = (rc->range >> RC_BIT_MODEL_TOTAL_BITS) * prob;
        prob = (probability)(prob + ((RC_BIT_MODEL_TOTAL - prob) >> RC_MOVE_BITS));
        *rc->probs[rc->pos] = prob;
        break;
      }
      case RC_BIT_1: {
        probability prob = *rc->probs[rc->pos];
        const uint32_t bound = prob * (rc->range >> RC_BIT_MODEL_TOTAL_BITS);
        rc->low += bound;
        rc->range -= bound;
        prob = (probability)(prob - (prob >> RC_MOVE_BITS));
        *rc->probs[rc->pos] = prob;
        break;
      }
      case RC_DIRECT_0:
        rc->range >>= 1;
        break;
      case RC_DIRECT_1:
        rc->range >>= 1;
        rc->low += rc->range;
        break;
      case RC_FLUSH:
        rc->range = UINT32_MAX;
        do {
          if (rc_shift_low(rc, out, out_pos, out_size)) return true;
        } while (++rc->pos < rc->count);
        rc_reset(rc);
        return false;
    }
    ++rc->pos;
  }
  rc->count = 0;
  rc->pos = 0;
  return false;
}

static inline uint64_t rc_pending(const RangeEncoder* rc) { return rc->cache_size + 5 - 1; }

static inline uint32_t rc_bit_price(const probability prob, const uint32_t bit) {
  return tables.prices[(prob ^ ((UINT32_C(0) - bit) & (RC_BIT_MODEL_TOTAL - 1))) >> RC_MOVE_REDUCING_BITS];
}
static inline uint32_t rc_bit_0_price(const probability prob) { return tables.prices[prob >> RC_MOVE_REDUCING_BITS]; }
static inline uint32_t rc_bit_1_price(const probability prob) {
  return tables.prices[(prob ^ (RC_BIT_MODEL_TOTAL - 1)) >> RC_MOVE_REDUCING_BITS];
}

static inline uint32_t rc_bittree_price(const probability* const probs, const uint32_t bit_levels, uint32_t symbol) {
  uint32_t price = 0;
  symbol += UINT32_C(1) << bit_levels;
  do {
    const uint32_t bit = symbol & 1;
    symbol >>= 1;
    price += rc_bit_price(probs[symbol], bit);
  } while (symbol != 1);
  return price;
}

static inline uint32_t rc_bittree_reverse_price(const probability* const probs, uint32_t bit_levels, uint32_t symbol) {
  uint32_t price = 0, model_index = 1;
  do {
    const uint32_t bit = symbol & 1;
    symbol >>= 1;
    price += rc_bit_price(probs[model_index], bit);
    model_index = (model_index << 1) + bit;
  } while (--bit_levels != 0);
  return price;
}

static inline uint32_t rc_direct_price(const uint32_t bits) { return bits << RC_BIT_PRICE_SHIFT_BITS; }

//----------------------------------------------------------------------------
// LZMA encoder (lzma_common.h, lzma_encoder_private.h, lzma_encoder.c)

// states, named oldest_older_previous event
static const uint32_t STATE_LIT_LIT = 0;
static const uint32_t STATE_SHORTREP_LIT_LIT = 3;
static const uint32_t STATE_LIT_MATCH = 7;
static const uint32_t STATE_LIT_LONGREP = 8;
static const uint32_t STATE_LIT_SHORTREP = 9;
static const uint32_t STATE_NONLIT_MATCH = 10;
static const uint32_t STATE_NONLIT_REP = 11;

static inline void update_literal(uint32_t& s) {
  s = s <= STATE_SHORTREP_LIT_LIT ? STATE_LIT_LIT : (s <= STATE_LIT_SHORTREP ? s - 3 : s - 6);
}
static inline void update_literal_normal(uint32_t& s) { s = s <= STATE_SHORTREP_LIT_LIT ? STATE_LIT_LIT : s - 3; }
static inline void update_literal_matched(uint32_t& s) { s = s <= STATE_LIT_SHORTREP ? s - 3 : s - 6; }
static inline void update_match(uint32_t& s) { s = s < LIT_STATES ? STATE_LIT_MATCH : STATE_NONLIT_MATCH; }
static inline void update_long_rep(uint32_t& s) { s = s < LIT_STATES ? STATE_LIT_LONGREP : STATE_NONLIT_REP; }
static inline void update_short_rep(uint32_t& s) { s = s < LIT_STATES ? STATE_LIT_SHORTREP : STATE_NONLIT_REP; }
static inline bool is_literal_state(uint32_t s) { return s < LIT_STATES; }

static inline uint32_t literal_mask_calc(uint32_t lc, uint32_t lp) {
  return (UINT32_C(0x100) << lp) - (UINT32_C(0x100) >> lc);
}

static inline uint32_t get_dist_state(uint32_t len) {
  return len < DIST_STATES + MATCH_LEN_MIN ? len - MATCH_LEN_MIN : DIST_STATES - 1;
}

static inline bool is_lclppb_valid(const lzma_options_lzma* o) {
  return o->lc <= LZMA_LCLP_MAX && o->lp <= LZMA_LCLP_MAX && o->lc + o->lp <= LZMA_LCLP_MAX && o->pb <= LZMA_PB_MAX;
}

struct LengthEncoder {
  probability choice;
  probability choice2;
  probability low[POS_STATES_MAX][LEN_LOW_SYMBOLS];
  probability mid[POS_STATES_MAX][LEN_MID_SYMBOLS];
  probability high[LEN_HIGH_SYMBOLS];
  uint32_t prices[POS_STATES_MAX][LEN_SYMBOLS];
  uint32_t table_size;
  uint32_t counters[POS_STATES_MAX];
};

struct Optimal {
  uint32_t state;
  bool prev_1_is_literal;
  bool prev_2;
  uint32_t pos_prev_2;
  uint32_t back_prev_2;
  uint32_t price;
  uint32_t pos_prev;
  uint32_t back_prev;
  uint32_t backs[REPS];
};

struct LZMA1 {
  RangeEncoder rc;
  uint64_t uncomp_size;
  uint32_t state;
  uint32_t reps[REPS];
  Match matches[MATCH_LEN_MAX + 1];
  uint32_t matches_count;
  uint32_t longest_match_length;  // kept between the lzma_lzma_optimum_*() calls
  bool fast_mode;
  bool is_initialized;  // first byte has been coded
  bool use_eopm;
  uint32_t pos_mask;
  uint32_t literal_context_bits;
  uint32_t literal_mask;
  probability literal[LITERAL_CODERS_MAX * LITERAL_CODER_SIZE];
  probability is_match[STATES][POS_STATES_MAX];
  probability is_rep[STATES];
  probability is_rep0[STATES];
  probability is_rep1[STATES];
  probability is_rep2[STATES];
  probability is_rep0_long[STATES][POS_STATES_MAX];
  probability dist_slot[DIST_STATES][DIST_SLOTS];
  probability dist_special[FULL_DISTANCES - DIST_MODEL_END];
  probability dist_align[ALIGN_SIZE];
  LengthEncoder match_len_encoder;
  LengthEncoder rep_len_encoder;
  uint32_t dist_slot_prices[DIST_STATES][DIST_SLOTS];
  uint32_t dist_prices[DIST_STATES][FULL_DISTANCES];
  uint32_t dist_table_size;
  uint32_t match_price_count;
  uint32_t align_prices[ALIGN_SIZE];
  uint32_t align_price_count;
  uint32_t opts_end_index;
  uint32_t opts_current_index;
  Optimal opts[OPTS];
};

static inline probability* literal_subcoder(probability* probs, uint32_t lc, uint32_t literal_mask, uint32_t pos,
                                            uint32_t prev_byte) {
  return probs + UINT32_C(3) * ((((pos << 8) + prev_byte) & literal_mask) << lc);
}
static inline const probability* literal_subcoder(const probability* probs, uint32_t lc, uint32_t literal_mask,
                                                  uint32_t pos, uint32_t prev_byte) {
  return probs + UINT32_C(3) * ((((pos << 8) + prev_byte) & literal_mask) << lc);
}

static inline void literal_matched(RangeEncoder* rc, probability* subcoder, uint32_t match_byte, uint32_t symbol) {
  uint32_t offset = 0x100;
  symbol += UINT32_C(1) << 8;
  do {
    match_byte <<= 1;
    const uint32_t match_bit = match_byte & offset;
    const uint32_t subcoder_index = offset + match_bit + (symbol >> 8);
    const uint32_t bit = (symbol >> 7) & 1;
    rc_bit(rc, &subcoder[subcoder_index], bit);
    symbol <<= 1;
    offset &= ~(match_byte ^ symbol);
  } while (symbol < (UINT32_C(1) << 16));
}

static inline void literal(LZMA1* coder, MF* mf, uint32_t position) {
  const uint8_t cur_byte = mf->buffer[mf->read_pos - mf->read_ahead];
  probability* subcoder = literal_subcoder(coder->literal, coder->literal_context_bits, coder->literal_mask, position,
                                           mf->buffer[mf->read_pos - mf->read_ahead - 1]);
  if (is_literal_state(coder->state)) {
    update_literal_normal(coder->state);
    rc_bittree(&coder->rc, subcoder, 8, cur_byte);
  } else {
    // after a match: code the literal against the byte at rep0
    update_literal_matched(coder->state);
    const uint8_t match_byte = mf->buffer[mf->read_pos - coder->reps[0] - 1 - mf->read_ahead];
    literal_matched(&coder->rc, subcoder, match_byte, cur_byte);
  }
}

static void length_update_prices(LengthEncoder* lc, const uint32_t pos_state) {
  const uint32_t table_size = lc->table_size;
  lc->counters[pos_state] = table_size;
  const uint32_t a0 = rc_bit_0_price(lc->choice);
  const uint32_t a1 = rc_bit_1_price(lc->choice);
  const uint32_t b0 = a1 + rc_bit_0_price(lc->choice2);
  const uint32_t b1 = a1 + rc_bit_1_price(lc->choice2);
  uint32_t* const prices = lc->prices[pos_state];
  uint32_t i;
  for (i = 0; i < table_size && i < LEN_LOW_SYMBOLS; ++i)
    prices[i] = a0 + rc_bittree_price(lc->low[pos_state], LEN_LOW_BITS, i);
  for (; i < table_size && i < LEN_LOW_SYMBOLS + LEN_MID_SYMBOLS; ++i)
    prices[i] = b0 + rc_bittree_price(lc->mid[pos_state], LEN_MID_BITS, i - LEN_LOW_SYMBOLS);
  for (; i < table_size; ++i)
    prices[i] = b1 + rc_bittree_price(lc->high, LEN_HIGH_BITS, i - LEN_LOW_SYMBOLS - LEN_MID_SYMBOLS);
}

static inline void length(RangeEncoder* rc, LengthEncoder* lc, const uint32_t pos_state, uint32_t len,
                          const bool fast_mode) {
  len -= MATCH_LEN_MIN;
  if (len < LEN_LOW_SYMBOLS) {
    rc_bit(rc, &lc->choice, 0);
    rc_bittree(rc, lc->low[pos_state], LEN_LOW_BITS, len);
  } else {
    rc_bit(rc, &lc->choice, 1);
    len -= LEN_LOW_SYMBOLS;
    if (len < LEN_MID_SYMBOLS) {
      rc_bit(rc, &lc->choice2, 0);
      rc_bittree(rc, lc->mid[pos_state], LEN_MID_BITS, len);
    } else {
      rc_bit(rc, &lc->choice2, 1);
      len -= LEN_MID_SYMBOLS;
      rc_bittree(rc, lc->high, LEN_HIGH_BITS, len);
    }
  }
  // only the normal mode uses the prices; note that the queued bits are not coded yet
  if (!fast_mode)
    if (--lc->counters[pos_state] == 0) length_update_prices(lc, pos_state);
}

static inline void match(LZMA1* coder, const uint32_t pos_state, const uint32_t distance, const uint32_t len) {
  update_match(coder->state);
  length(&coder->rc, &coder->match_len_encoder, pos_state, len, coder->fast_mode);
  const uint32_t dist_slot = get_dist_slot(distance);
  const uint32_t dist_state = get_dist_state(len);
  rc_bittree(&coder->rc, coder->dist_slot[dist_state], DIST_SLOT_BITS, dist_slot);
  if (dist_slot >= DIST_MODEL_START) {
    const uint32_t footer_bits = (dist_slot >> 1) - 1;
    const uint32_t base = (2 | (dist_slot & 1)) << footer_bits;
    const uint32_t dist_reduced = distance - base;
    if (dist_slot < DIST_MODEL_END) {
      // base - dist_slot - 1 can be -1, but the tree starts at probs[1]
      rc_bittree_reverse(&coder->rc, coder->dist_special + base - dist_slot - 1, footer_bits, dist_reduced);
    } else {
      rc_direct(&coder->rc, dist_reduced >> ALIGN_BITS, footer_bits - ALIGN_BITS);
      rc_bittree_reverse(&coder->rc, coder->dist_align, ALIGN_BITS, dist_reduced & ALIGN_MASK);
      ++coder->align_price_count;
    }
  }
  coder->reps[3] = coder->reps[2];
  coder->reps[2] = coder->reps[1];
  coder->reps[1] = coder->reps[0];
  coder->reps[0] = distance;
  ++coder->match_price_count;
}

static inline void rep_match(LZMA1* coder, const uint32_t pos_state, const uint32_t rep, const uint32_t len) {
  if (rep == 0) {
    rc_bit(&coder->rc, &coder->is_rep0[coder->state], 0);
    rc_bit(&coder->rc, &coder->is_rep0_long[coder->state][pos_state], len != 1);
  } else {
    const uint32_t distance = coder->reps[rep];
    rc_bit(&coder->rc, &coder->is_rep0[coder->state], 1);
    if (rep == 1) {
      rc_bit(&coder->rc, &coder->is_rep1[coder->state], 0);
    } else {
      rc_bit(&coder->rc, &coder->is_rep1[coder->state], 1);
      rc_bit(&coder->rc, &coder->is_rep2[coder->state], rep - 2);
      if (rep == 3) coder->reps[3] = coder->reps[2];
      coder->reps[2] = coder->reps[1];
    }
    coder->reps[1] = coder->reps[0];
    coder->reps[0] = distance;
  }
  if (len == 1) {
    update_short_rep(coder->state);
  } else {
    length(&coder->rc, &coder->rep_len_encoder, pos_state, len, coder->fast_mode);
    update_long_rep(coder->state);
  }
}

// back: UINT32_MAX = literal, < REPS = repeated match, else match at distance back - REPS
static void encode_symbol(LZMA1* coder, MF* mf, uint32_t back, uint32_t len, uint32_t position) {
  const uint32_t pos_state = position & coder->pos_mask;
  if (back == UINT32_MAX) {
    rc_bit(&coder->rc, &coder->is_match[coder->state][pos_state], 0);
    literal(coder, mf, position);
  } else {
    rc_bit(&coder->rc, &coder->is_match[coder->state][pos_state], 1);
    if (back < REPS) {
      rc_bit(&coder->rc, &coder->is_rep[coder->state], 1);
      rep_match(coder, pos_state, back, len);
    } else {
      rc_bit(&coder->rc, &coder->is_rep[coder->state], 0);
      match(coder, pos_state, back - REPS, len);
    }
  }
  mf->read_ahead -= len;
}

// the first symbol is always a literal
static bool encode_init(LZMA1* coder, MF* mf) {
  if (mf->read_pos == mf->read_limit) {
    if (mf->action == ACT_RUN) return false;
  } else {
    mf_skip(mf, 1);
    mf->read_ahead = 0;
    rc_bit(&coder->rc, &coder->is_match[0][0], 0);
    rc_bittree(&coder->rc, coder->literal + 0, 8, mf->buffer[0]);
    ++coder->uncomp_size;
  }
  coder->is_initialized = true;
  return true;
}

static void encode_eopm(LZMA1* coder, uint32_t position) {
  const uint32_t pos_state = position & coder->pos_mask;
  rc_bit(&coder->rc, &coder->is_match[coder->state][pos_state], 1);
  rc_bit(&coder->rc, &coder->is_rep[coder->state], 0);
  match(coder, pos_state, UINT32_MAX, MATCH_LEN_MIN);
}

//----------------------------------------------------------------------------
// Optimal parsing, normal mode (lzma_encoder_optimum_normal.c)

static uint32_t get_literal_price(const LZMA1* const coder, const uint32_t pos, const uint32_t prev_byte,
                                  const bool match_mode, uint32_t match_byte, uint32_t symbol) {
  const probability* const subcoder =
      literal_subcoder((const probability*)coder->literal, coder->literal_context_bits, coder->literal_mask, pos,
                       prev_byte);
  uint32_t price = 0;
  if (!match_mode) {
    price = rc_bittree_price(subcoder, 8, symbol);
  } else {
    uint32_t offset = 0x100;
    symbol += UINT32_C(1) << 8;
    do {
      match_byte <<= 1;
      const uint32_t match_bit = match_byte & offset;
      const uint32_t subcoder_index = offset + match_bit + (symbol >> 8);
      const uint32_t bit = (symbol >> 7) & 1;
      price += rc_bit_price(subcoder[subcoder_index], bit);
      symbol <<= 1;
      offset &= ~(match_byte ^ symbol);
    } while (symbol < (UINT32_C(1) << 16));
  }
  return price;
}

static inline uint32_t get_len_price(const LengthEncoder* const lencoder, const uint32_t len, const uint32_t pos_state) {
  return lencoder->prices[pos_state][len - MATCH_LEN_MIN];
}

static inline uint32_t get_short_rep_price(const LZMA1* const coder, const uint32_t state, const uint32_t pos_state) {
  return rc_bit_0_price(coder->is_rep0[state]) + rc_bit_0_price(coder->is_rep0_long[state][pos_state]);
}

static inline uint32_t get_pure_rep_price(const LZMA1* const coder, const uint32_t rep_index, const uint32_t state,
                                          uint32_t pos_state) {
  uint32_t price;
  if (rep_index == 0) {
    price = rc_bit_0_price(coder->is_rep0[state]);
    price += rc_bit_1_price(coder->is_rep0_long[state][pos_state]);
  } else {
    price = rc_bit_1_price(coder->is_rep0[state]);
    if (rep_index == 1) {
      price += rc_bit_0_price(coder->is_rep1[state]);
    } else {
      price += rc_bit_1_price(coder->is_rep1[state]);
      price += rc_bit_price(coder->is_rep2[state], rep_index - 2);
    }
  }
  return price;
}

static inline uint32_t get_rep_price(const LZMA1* const coder, const uint32_t rep_index, const uint32_t len,
                                     const uint32_t state, const uint32_t pos_state) {
  return get_len_price(&coder->rep_len_encoder, len, pos_state) + get_pure_rep_price(coder, rep_index, state, pos_state);
}

static inline uint32_t get_dist_len_price(const LZMA1* const coder, const uint32_t dist, const uint32_t len,
                                          const uint32_t pos_state) {
  const uint32_t dist_state = get_dist_state(len);
  uint32_t price;
  if (dist < FULL_DISTANCES) {
    price = coder->dist_prices[dist_state][dist];
  } else {
    const uint32_t dist_slot = get_dist_slot_2(dist);
    price = coder->dist_slot_prices[dist_state][dist_slot] + coder->align_prices[dist & ALIGN_MASK];
  }
  price += get_len_price(&coder->match_len_encoder, len, pos_state);
  return price;
}

static void fill_dist_prices(LZMA1* coder) {
  for (uint32_t dist_state = 0; dist_state < DIST_STATES; ++dist_state) {
    uint32_t* const dist_slot_prices = coder->dist_slot_prices[dist_state];
    for (uint32_t dist_slot = 0; dist_slot < coder->dist_table_size; ++dist_slot)
      dist_slot_prices[dist_slot] = rc_bittree_price(coder->dist_slot[dist_state], DIST_SLOT_BITS, dist_slot);
    // direct bits of distances >= FULL_DISTANCES (align bits: fill_align_prices())
    for (uint32_t dist_slot = DIST_MODEL_END; dist_slot < coder->dist_table_size; ++dist_slot)
      dist_slot_prices[dist_slot] += rc_direct_price(((dist_slot >> 1) - 1) - ALIGN_BITS);
    for (uint32_t i = 0; i < DIST_MODEL_START; ++i) coder->dist_prices[dist_state][i] = dist_slot_prices[i];
  }
  for (uint32_t i = DIST_MODEL_START; i < FULL_DISTANCES; ++i) {
    const uint32_t dist_slot = get_dist_slot(i);
    const uint32_t footer_bits = ((dist_slot >> 1) - 1);
    const uint32_t base = (2 | (dist_slot & 1)) << footer_bits;
    const uint32_t price =
        rc_bittree_reverse_price(coder->dist_special + base - dist_slot - 1, footer_bits, i - base);
    for (uint32_t dist_state = 0; dist_state < DIST_STATES; ++dist_state)
      coder->dist_prices[dist_state][i] = price + coder->dist_slot_prices[dist_state][dist_slot];
  }
  coder->match_price_count = 0;
}

static void fill_align_prices(LZMA1* coder) {
  for (uint32_t i = 0; i < ALIGN_SIZE; ++i)
    coder->align_prices[i] = rc_bittree_reverse_price(coder->dist_align, ALIGN_BITS, i);
  coder->align_price_count = 0;
}

static inline void make_literal(Optimal* optimal) {
  optimal->back_prev = UINT32_MAX;
  optimal->prev_1_is_literal = false;
}

static inline void make_short_rep(Optimal* optimal) {
  optimal->back_prev = 0;
  optimal->prev_1_is_literal = false;
}

static inline bool is_short_rep(const Optimal& optimal) { return optimal.back_prev == 0; }

// reverses the chosen path from opts[cur] back to opts[0]
static void backward(LZMA1* coder, uint32_t* len_res, uint32_t* back_res, uint32_t cur) {
  coder->opts_end_index = cur;
  uint32_t pos_mem = coder->opts[cur].pos_prev;
  uint32_t back_mem = coder->opts[cur].back_prev;
  do {
    if (coder->opts[cur].prev_1_is_literal) {
      make_literal(&coder->opts[pos_mem]);
      coder->opts[pos_mem].pos_prev = pos_mem - 1;
      if (coder->opts[cur].prev_2) {
        coder->opts[pos_mem - 1].prev_1_is_literal = false;
        coder->opts[pos_mem - 1].pos_prev = coder->opts[cur].pos_prev_2;
        coder->opts[pos_mem - 1].back_prev = coder->opts[cur].back_prev_2;
      }
    }
    const uint32_t pos_prev = pos_mem;
    const uint32_t back_cur = back_mem;
    back_mem = coder->opts[pos_prev].back_prev;
    pos_mem = coder->opts[pos_prev].pos_prev;
    coder->opts[pos_prev].back_prev = back_cur;
    coder->opts[pos_prev].pos_prev = cur;
    cur = pos_prev;
  } while (cur != 0);
  coder->opts_current_index = coder->opts[0].pos_prev;
  *len_res = coder->opts[0].pos_prev;
  *back_res = coder->opts[0].back_prev;
}

static inline uint32_t helper1(LZMA1* coder, MF* mf, uint32_t* back_res, uint32_t* len_res, uint32_t position) {
  const uint32_t nice_len = mf->nice_len;
  uint32_t len_main, matches_count;
  if (mf->read_ahead == 0) {
    len_main = mf_find(mf, &matches_count, coder->matches);
  } else {
    len_main = coder->longest_match_length;
    matches_count = coder->matches_count;
  }
  const uint32_t buf_avail = my_min(mf_avail(mf) + 1, MATCH_LEN_MAX);
  if (buf_avail < 2) {
    *back_res = UINT32_MAX;
    *len_res = 1;
    return UINT32_MAX;
  }
  const uint8_t* const buf = mf_ptr(mf) - 1;
  uint32_t rep_lens[REPS];
  uint32_t rep_max_index = 0;
  for (uint32_t i = 0; i < REPS; ++i) {
    const uint8_t* const buf_back = buf - coder->reps[i] - 1;
    if (not_equal_16(buf, buf_back)) {
      rep_lens[i] = 0;
      continue;
    }
    rep_lens[i] = memcmplen(buf, buf_back, 2, buf_avail);
    if (rep_lens[i] > rep_lens[rep_max_index]) rep_max_index = i;
  }
  if (rep_lens[rep_max_index] >= nice_len) {
    *back_res = rep_max_index;
    *len_res = rep_lens[rep_max_index];
    mf_skip(mf, *len_res - 1);
    return UINT32_MAX;
  }
  if (len_main >= nice_len) {
    *back_res = coder->matches[matches_count - 1].dist + REPS;
    *len_res = len_main;
    mf_skip(mf, len_main - 1);
    return UINT32_MAX;
  }
  const uint8_t current_byte = *buf;
  const uint8_t match_byte = *(buf - coder->reps[0] - 1);
  if (len_main < 2 && current_byte != match_byte && rep_lens[rep_max_index] < 2) {
    *back_res = UINT32_MAX;
    *len_res = 1;
    return UINT32_MAX;
  }
  coder->opts[0].state = coder->state;
  const uint32_t pos_state = position & coder->pos_mask;
  coder->opts[1].price = rc_bit_0_price(coder->is_match[coder->state][pos_state]) +
                         get_literal_price(coder, position, buf[-1], !is_literal_state(coder->state), match_byte,
                                           current_byte);
  make_literal(&coder->opts[1]);
  const uint32_t match_price = rc_bit_1_price(coder->is_match[coder->state][pos_state]);
  const uint32_t rep_match_price = match_price + rc_bit_1_price(coder->is_rep[coder->state]);
  if (match_byte == current_byte) {
    const uint32_t short_rep_price = rep_match_price + get_short_rep_price(coder, coder->state, pos_state);
    if (short_rep_price < coder->opts[1].price) {
      coder->opts[1].price = short_rep_price;
      make_short_rep(&coder->opts[1]);
    }
  }
  const uint32_t len_end = my_max(len_main, rep_lens[rep_max_index]);
  if (len_end < 2) {
    *back_res = coder->opts[1].back_prev;
    *len_res = 1;
    return UINT32_MAX;
  }
  coder->opts[1].pos_prev = 0;
  for (uint32_t i = 0; i < REPS; ++i) coder->opts[0].backs[i] = coder->reps[i];
  uint32_t len = len_end;
  do {
    coder->opts[len].price = RC_INFINITY_PRICE;
  } while (--len >= 2);
  for (uint32_t i = 0; i < REPS; ++i) {
    uint32_t rep_len = rep_lens[i];
    if (rep_len < 2) continue;
    const uint32_t price = rep_match_price + get_pure_rep_price(coder, i, coder->state, pos_state);
    do {
      const uint32_t cur_and_len_price = price + get_len_price(&coder->rep_len_encoder, rep_len, pos_state);
      if (cur_and_len_price < coder->opts[rep_len].price) {
        coder->opts[rep_len].price = cur_and_len_price;
        coder->opts[rep_len].pos_prev = 0;
        coder->opts[rep_len].back_prev = i;
        coder->opts[rep_len].prev_1_is_literal = false;
      }
    } while (--rep_len >= 2);
  }
  const uint32_t normal_match_price = match_price + rc_bit_0_price(coder->is_rep[coder->state]);
  len = rep_lens[0] >= 2 ? rep_lens[0] + 1 : 2;
  if (len <= len_main) {
    uint32_t i = 0;
    while (len > coder->matches[i].len) ++i;
    for (;; ++len) {
      const uint32_t dist = coder->matches[i].dist;
      const uint32_t cur_and_len_price = normal_match_price + get_dist_len_price(coder, dist, len, pos_state);
      if (cur_and_len_price < coder->opts[len].price) {
        coder->opts[len].price = cur_and_len_price;
        coder->opts[len].pos_prev = 0;
        coder->opts[len].back_prev = dist + REPS;
        coder->opts[len].prev_1_is_literal = false;
      }
      if (len == coder->matches[i].len)
        if (++i == matches_count) break;
    }
  }
  return len_end;
}

static inline uint32_t helper2(LZMA1* coder, uint32_t* reps, const uint8_t* buf, uint32_t len_end, uint32_t position,
                               const uint32_t cur, const uint32_t nice_len, const uint32_t buf_avail_full) {
  uint32_t matches_count = coder->matches_count;
  uint32_t new_len = coder->longest_match_length;
  uint32_t pos_prev = coder->opts[cur].pos_prev;
  uint32_t state;
  if (coder->opts[cur].prev_1_is_literal) {
    --pos_prev;
    if (coder->opts[cur].prev_2) {
      state = coder->opts[coder->opts[cur].pos_prev_2].state;
      if (coder->opts[cur].back_prev_2 < REPS) update_long_rep(state);
      else update_match(state);
    } else {
      state = coder->opts[pos_prev].state;
    }
    update_literal(state);
  } else {
    state = coder->opts[pos_prev].state;
  }
  if (pos_prev == cur - 1) {
    if (is_short_rep(coder->opts[cur])) update_short_rep(state);
    else update_literal(state);
  } else {
    uint32_t pos;
    if (coder->opts[cur].prev_1_is_literal && coder->opts[cur].prev_2) {
      pos_prev = coder->opts[cur].pos_prev_2;
      pos = coder->opts[cur].back_prev_2;
      update_long_rep(state);
    } else {
      pos = coder->opts[cur].back_prev;
      if (pos < REPS) update_long_rep(state);
      else update_match(state);
    }
    if (pos < REPS) {
      reps[0] = coder->opts[pos_prev].backs[pos];
      uint32_t i;
      for (i = 1; i <= pos; ++i) reps[i] = coder->opts[pos_prev].backs[i - 1];
      for (; i < REPS; ++i) reps[i] = coder->opts[pos_prev].backs[i];
    } else {
      reps[0] = pos - REPS;
      for (uint32_t i = 1; i < REPS; ++i) reps[i] = coder->opts[pos_prev].backs[i - 1];
    }
  }
  coder->opts[cur].state = state;
  for (uint32_t i = 0; i < REPS; ++i) coder->opts[cur].backs[i] = reps[i];
  const uint32_t cur_price = coder->opts[cur].price;
  const uint8_t current_byte = *buf;
  const uint8_t match_byte = *(buf - reps[0] - 1);
  const uint32_t pos_state = position & coder->pos_mask;
  const uint32_t cur_and_1_price = cur_price + rc_bit_0_price(coder->is_match[state][pos_state]) +
                                   get_literal_price(coder, position, buf[-1], !is_literal_state(state), match_byte,
                                                     current_byte);
  bool next_is_literal = false;
  if (cur_and_1_price < coder->opts[cur + 1].price) {
    coder->opts[cur + 1].price = cur_and_1_price;
    coder->opts[cur + 1].pos_prev = cur;
    make_literal(&coder->opts[cur + 1]);
    next_is_literal = true;
  }
  const uint32_t match_price = cur_price + rc_bit_1_price(coder->is_match[state][pos_state]);
  const uint32_t rep_match_price = match_price + rc_bit_1_price(coder->is_rep[state]);
  if (match_byte == current_byte && !(coder->opts[cur + 1].pos_prev < cur && coder->opts[cur + 1].back_prev == 0)) {
    const uint32_t short_rep_price = rep_match_price + get_short_rep_price(coder, state, pos_state);
    if (short_rep_price <= coder->opts[cur + 1].price) {
      coder->opts[cur + 1].price = short_rep_price;
      coder->opts[cur + 1].pos_prev = cur;
      make_short_rep(&coder->opts[cur + 1]);
      next_is_literal = true;
    }
  }
  if (buf_avail_full < 2) return len_end;
  const uint32_t buf_avail = my_min(buf_avail_full, nice_len);
  if (!next_is_literal && match_byte != current_byte) {
    // literal + rep0
    const uint8_t* const buf_back = buf - reps[0] - 1;
    const uint32_t limit = my_min(buf_avail_full, nice_len + 1);
    const uint32_t len_test = memcmplen(buf, buf_back, 1, limit) - 1;
    if (len_test >= 2) {
      uint32_t state_2 = state;
      update_literal(state_2);
      const uint32_t pos_state_next = (position + 1) & coder->pos_mask;
      const uint32_t next_rep_match_price = cur_and_1_price +
                                            rc_bit_1_price(coder->is_match[state_2][pos_state_next]) +
                                            rc_bit_1_price(coder->is_rep[state_2]);
      const uint32_t offset = cur + 1 + len_test;
      while (len_end < offset) coder->opts[++len_end].price = RC_INFINITY_PRICE;
      const uint32_t cur_and_len_price =
          next_rep_match_price + get_rep_price(coder, 0, len_test, state_2, pos_state_next);
      if (cur_and_len_price < coder->opts[offset].price) {
        coder->opts[offset].price = cur_and_len_price;
        coder->opts[offset].pos_prev = cur + 1;
        coder->opts[offset].back_prev = 0;
        coder->opts[offset].prev_1_is_literal = true;
        coder->opts[offset].prev_2 = false;
      }
    }
  }
  uint32_t start_len = 2;
  for (uint32_t rep_index = 0; rep_index < REPS; ++rep_index) {
    const uint8_t* const buf_back = buf - reps[rep_index] - 1;
    if (not_equal_16(buf, buf_back)) continue;
    uint32_t len_test = memcmplen(buf, buf_back, 2, buf_avail);
    while (len_end < cur + len_test) coder->opts[++len_end].price = RC_INFINITY_PRICE;
    const uint32_t len_test_temp = len_test;
    const uint32_t price = rep_match_price + get_pure_rep_price(coder, rep_index, state, pos_state);
    do {
      const uint32_t cur_and_len_price = price + get_len_price(&coder->rep_len_encoder, len_test, pos_state);
      if (cur_and_len_price < coder->opts[cur + len_test].price) {
        coder->opts[cur + len_test].price = cur_and_len_price;
        coder->opts[cur + len_test].pos_prev = cur;
        coder->opts[cur + len_test].back_prev = rep_index;
        coder->opts[cur + len_test].prev_1_is_literal = false;
      }
    } while (--len_test >= 2);
    len_test = len_test_temp;
    if (rep_index == 0) start_len = len_test + 1;
    // rep + literal + rep0
    uint32_t len_test_2 = len_test + 1;
    const uint32_t limit = my_min(buf_avail_full, len_test_2 + nice_len);
    if (len_test_2 < limit) len_test_2 = memcmplen(buf, buf_back, len_test_2, limit);
    len_test_2 -= len_test + 1;
    if (len_test_2 >= 2) {
      uint32_t state_2 = state;
      update_long_rep(state_2);
      uint32_t pos_state_next = (position + len_test) & coder->pos_mask;
      const uint32_t cur_and_len_literal_price =
          price + get_len_price(&coder->rep_len_encoder, len_test, pos_state) +
          rc_bit_0_price(coder->is_match[state_2][pos_state_next]) +
          get_literal_price(coder, position + len_test, buf[len_test - 1], true, buf_back[len_test], buf[len_test]);
      update_literal(state_2);
      pos_state_next = (position + len_test + 1) & coder->pos_mask;
      const uint32_t next_rep_match_price = cur_and_len_literal_price +
                                            rc_bit_1_price(coder->is_match[state_2][pos_state_next]) +
                                            rc_bit_1_price(coder->is_rep[state_2]);
      const uint32_t offset = cur + len_test + 1 + len_test_2;
      while (len_end < offset) coder->opts[++len_end].price = RC_INFINITY_PRICE;
      const uint32_t cur_and_len_price =
          next_rep_match_price + get_rep_price(coder, 0, len_test_2, state_2, pos_state_next);
      if (cur_and_len_price < coder->opts[offset].price) {
        coder->opts[offset].price = cur_and_len_price;
        coder->opts[offset].pos_prev = cur + len_test + 1;
        coder->opts[offset].back_prev = 0;
        coder->opts[offset].prev_1_is_literal = true;
        coder->opts[offset].prev_2 = true;
        coder->opts[offset].pos_prev_2 = cur;
        coder->opts[offset].back_prev_2 = rep_index;
      }
    }
  }
  if (new_len > buf_avail) {
    new_len = buf_avail;
    matches_count = 0;
    while (new_len > coder->matches[matches_count].len) ++matches_count;
    coder->matches[matches_count++].len = new_len;
  }
  if (new_len >= start_len) {
    const uint32_t normal_match_price = match_price + rc_bit_0_price(coder->is_rep[state]);
    while (len_end < cur + new_len) coder->opts[++len_end].price = RC_INFINITY_PRICE;
    uint32_t i = 0;
    while (start_len > coder->matches[i].len) ++i;
    for (uint32_t len_test = start_len;; ++len_test) {
      const uint32_t cur_back = coder->matches[i].dist;
      uint32_t cur_and_len_price = normal_match_price + get_dist_len_price(coder, cur_back, len_test, pos_state);
      if (cur_and_len_price < coder->opts[cur + len_test].price) {
        coder->opts[cur + len_test].price = cur_and_len_price;
        coder->opts[cur + len_test].pos_prev = cur;
        coder->opts[cur + len_test].back_prev = cur_back + REPS;
        coder->opts[cur + len_test].prev_1_is_literal = false;
      }
      if (len_test == coder->matches[i].len) {
        // match + literal + rep0
        const uint8_t* const buf_back = buf - cur_back - 1;
        uint32_t len_test_2 = len_test + 1;
        const uint32_t limit = my_min(buf_avail_full, len_test_2 + nice_len);
        if (len_test_2 < limit) len_test_2 = memcmplen(buf, buf_back, len_test_2, limit);
        len_test_2 -= len_test + 1;
        if (len_test_2 >= 2) {
          uint32_t state_2 = state;
          update_match(state_2);
          uint32_t pos_state_next = (position + len_test) & coder->pos_mask;
          const uint32_t cur_and_len_literal_price =
              cur_and_len_price + rc_bit_0_price(coder->is_match[state_2][pos_state_next]) +
              get_literal_price(coder, position + len_test, buf[len_test - 1], true, buf_back[len_test],
                                buf[len_test]);
          update_literal(state_2);
          pos_state_next = (pos_state_next + 1) & coder->pos_mask;
          const uint32_t next_rep_match_price = cur_and_len_literal_price +
                                                rc_bit_1_price(coder->is_match[state_2][pos_state_next]) +
                                                rc_bit_1_price(coder->is_rep[state_2]);
          const uint32_t offset = cur + len_test + 1 + len_test_2;
          while (len_end < offset) coder->opts[++len_end].price = RC_INFINITY_PRICE;
          cur_and_len_price = next_rep_match_price + get_rep_price(coder, 0, len_test_2, state_2, pos_state_next);
          if (cur_and_len_price < coder->opts[offset].price) {
            coder->opts[offset].price = cur_and_len_price;
            coder->opts[offset].pos_prev = cur + len_test + 1;
            coder->opts[offset].back_prev = 0;
            coder->opts[offset].prev_1_is_literal = true;
            coder->opts[offset].prev_2 = true;
            coder->opts[offset].pos_prev_2 = cur;
            coder->opts[offset].back_prev_2 = cur_back + REPS;
          }
        }
        if (++i == matches_count) break;
      }
    }
  }
  return len_end;
}

static void lzma_optimum_normal(LZMA1* coder, MF* mf, uint32_t* back_res, uint32_t* len_res, uint32_t position) {
  // symbols of an earlier parse still pending
  if (coder->opts_end_index != coder->opts_current_index) {
    *len_res = coder->opts[coder->opts_current_index].pos_prev - coder->opts_current_index;
    *back_res = coder->opts[coder->opts_current_index].back_prev;
    coder->opts_current_index = coder->opts[coder->opts_current_index].pos_prev;
    return;
  }
  if (mf->read_ahead == 0) {
    if (coder->match_price_count >= (1 << 7)) fill_dist_prices(coder);
    if (coder->align_price_count >= ALIGN_SIZE) fill_align_prices(coder);
  }
  uint32_t len_end = helper1(coder, mf, back_res, len_res, position);
  if (len_end == UINT32_MAX) return;
  uint32_t reps[REPS];
  memcpy(reps, coder->reps, sizeof(reps));
  uint32_t cur;
  for (cur = 1; cur < len_end; ++cur) {
    coder->longest_match_length = mf_find(mf, &coder->matches_count, coder->matches);
    if (coder->longest_match_length >= mf->nice_len) break;
    len_end = helper2(coder, reps, mf_ptr(mf) - 1, len_end, position + cur, cur, mf->nice_len,
                      my_min(mf_avail(mf) + 1, OPTS - 1 - cur));
  }
  backward(coder, len_res, back_res, cur);
}

//----------------------------------------------------------------------------
// Greedy parsing, fast mode (lzma_encoder_optimum_fast.c)

static inline bool change_pair(uint32_t small_dist, uint32_t big_dist) { return (big_dist >> 7) > small_dist; }

static void lzma_optimum_fast(LZMA1* coder, MF* mf, uint32_t* back_res, uint32_t* len_res) {
  const uint32_t nice_len = mf->nice_len;
  uint32_t len_main, matches_count;
  if (mf->read_ahead == 0) {
    len_main = mf_find(mf, &matches_count, coder->matches);
  } else {
    len_main = coder->longest_match_length;
    matches_count = coder->matches_count;
  }
  const uint8_t* buf = mf_ptr(mf) - 1;
  const uint32_t buf_avail = my_min(mf_avail(mf) + 1, MATCH_LEN_MAX);
  if (buf_avail < 2) {
    *back_res = UINT32_MAX;
    *len_res = 1;
    return;
  }
  uint32_t rep_len = 0, rep_index = 0;
  for (uint32_t i = 0; i < REPS; ++i) {
    const uint8_t* const buf_back = buf - coder->reps[i] - 1;
    if (not_equal_16(buf, buf_back)) continue;
    const uint32_t len = memcmplen(buf, buf_back, 2, buf_avail);
    if (len >= nice_len) {
      *back_res = i;
      *len_res = len;
      mf_skip(mf, len - 1);
      return;
    }
    if (len > rep_len) {
      rep_index = i;
      rep_len = len;
    }
  }
  if (len_main >= nice_len) {
    *back_res = coder->matches[matches_count - 1].dist + REPS;
    *len_res = len_main;
    mf_skip(mf, len_main - 1);
    return;
  }
  uint32_t back_main = 0;
  if (len_main >= 2) {
    back_main = coder->matches[matches_count - 1].dist;
    while (matches_count > 1 && len_main == coder->matches[matches_count - 2].len + 1) {
      if (!change_pair(coder->matches[matches_count - 2].dist, back_main)) break;
      --matches_count;
      len_main = coder->matches[matches_count - 1].len;
      back_main = coder->matches[matches_count - 1].dist;
    }
    if (len_main == 2 && back_main >= 0x80) len_main = 1;
  }
  if (rep_len >= 2) {
    if (rep_len + 1 >= len_main || (rep_len + 2 >= len_main && back_main > (UINT32_C(1) << 9)) ||
        (rep_len + 3 >= len_main && back_main > (UINT32_C(1) << 15))) {
      *back_res = rep_index;
      *len_res = rep_len;
      mf_skip(mf, rep_len - 1);
      return;
    }
  }
  if (len_main < 2 || buf_avail <= 2) {
    *back_res = UINT32_MAX;
    *len_res = 1;
    return;
  }
  // a better match at the next byte makes this one a literal
  coder->longest_match_length = mf_find(mf, &coder->matches_count, coder->matches);
  if (coder->longest_match_length >= 2) {
    const uint32_t new_dist = coder->matches[coder->matches_count - 1].dist;
    if ((coder->longest_match_length >= len_main && new_dist < back_main) ||
        (coder->longest_match_length == len_main + 1 && !change_pair(back_main, new_dist)) ||
        (coder->longest_match_length > len_main + 1) ||
        (coder->longest_match_length + 1 >= len_main && len_main >= 3 && change_pair(new_dist, back_main))) {
      *back_res = UINT32_MAX;
      *len_res = 1;
      return;
    }
  }
  ++buf;
  const uint32_t limit = my_max(2, len_main - 1);
  for (uint32_t i = 0; i < REPS; ++i) {
    if (memcmp(buf, buf - coder->reps[i] - 1, limit) == 0) {
      *back_res = UINT32_MAX;
      *len_res = 1;
      return;
    }
  }
  *back_res = back_main + REPS;
  *len_res = len_main;
  mf_skip(mf, len_main - 2);
}

//----------------------------------------------------------------------------
// LZMA encoder main loop and initialization (lzma_encoder.c)

// lzma_lzma_encode(): limit != UINT32_MAX for LZMA2 chunks.  LZMA_OK: needs
// more input; LZMA_STREAM_END: done (chunk or stream); LZMA_BUF_ERROR: out
// is full (only LZMA1, LZMA2 always has the space)
static lzma_ret lzma_encode(LZMA1* coder, MF* mf, uint8_t* out, size_t* out_pos, size_t out_size, uint32_t limit) {
  if (!coder->is_initialized && !encode_init(coder, mf)) return LZMA_OK;
  if (rc_encode(&coder->rc, out, out_pos, out_size)) return LZMA_BUF_ERROR;
  for (;;) {
    // LZMA2: the chunk must not get too big
    if (limit != UINT32_MAX && (mf->read_pos - mf->read_ahead >= limit ||
                                *out_pos + rc_pending(&coder->rc) >= LZMA2_CHUNK_MAX - LOOP_INPUT_MAX))
      break;
    if (mf->read_pos >= mf->read_limit) {
      if (mf->action == ACT_RUN) return LZMA_OK;
      if (mf->read_ahead == 0) break;
    }
    uint32_t len, back;
    if (coder->fast_mode) lzma_optimum_fast(coder, mf, &back, &len);
    else lzma_optimum_normal(coder, mf, &back, &len, (uint32_t)coder->uncomp_size);
    encode_symbol(coder, mf, back, len, (uint32_t)coder->uncomp_size);
    coder->uncomp_size += len;
    if (rc_encode(&coder->rc, out, out_pos, out_size)) return LZMA_BUF_ERROR;
  }
  if (coder->use_eopm) encode_eopm(coder, (uint32_t)coder->uncomp_size);
  rc_flush(&coder->rc);
  if (rc_encode(&coder->rc, out, out_pos, out_size)) return LZMA_BUF_ERROR;
  return LZMA_STREAM_END;
}

static inline bool is_options_valid(const lzma_options_lzma* options) {
  return is_lclppb_valid(options) && options->nice_len >= MATCH_LEN_MIN && options->nice_len <= MATCH_LEN_MAX &&
         (options->mode == LZMA_MODE_FAST || options->mode == LZMA_MODE_NORMAL);
}

static void set_lz_options(LZOptions* lz, const lzma_options_lzma* options) {
  lz->before_size = OPTS;
  lz->dict_size = options->dict_size;
  lz->after_size = LOOP_INPUT_MAX;
  lz->match_len_max = MATCH_LEN_MAX;
  lz->nice_len = my_max(mf_get_hash_bytes(options->mf), options->nice_len);
  lz->match_finder = options->mf;
  lz->depth = options->depth;
  lz->preset_dict = options->preset_dict;
  lz->preset_dict_size = options->preset_dict_size;
}

static void length_encoder_reset(LengthEncoder* lencoder, const uint32_t num_pos_states, const bool fast_mode) {
  bit_reset(lencoder->choice);
  bit_reset(lencoder->choice2);
  for (uint32_t pos_state = 0; pos_state < num_pos_states; ++pos_state) {
    bittree_reset(lencoder->low[pos_state], LEN_LOW_BITS);
    bittree_reset(lencoder->mid[pos_state], LEN_MID_BITS);
  }
  bittree_reset(lencoder->high, LEN_HIGH_BITS);
  if (!fast_mode)
    for (uint32_t pos_state = 0; pos_state < num_pos_states; ++pos_state) length_update_prices(lencoder, pos_state);
}

static lzma_ret lzma_encoder_reset(LZMA1* coder, const lzma_options_lzma* options) {
  if (!is_options_valid(options)) return LZMA_OPTIONS_ERROR;
  coder->pos_mask = (1U << options->pb) - 1;
  coder->literal_context_bits = options->lc;
  coder->literal_mask = literal_mask_calc(options->lc, options->lp);
  rc_reset(&coder->rc);
  coder->state = STATE_LIT_LIT;
  for (uint32_t i = 0; i < REPS; ++i) coder->reps[i] = 0;
  const size_t coders = (size_t)LITERAL_CODER_SIZE << (options->lc + options->lp);
  for (size_t i = 0; i < coders; ++i) bit_reset(coder->literal[i]);
  for (uint32_t i = 0; i < STATES; ++i) {
    for (uint32_t j = 0; j <= coder->pos_mask; ++j) {
      bit_reset(coder->is_match[i][j]);
      bit_reset(coder->is_rep0_long[i][j]);
    }
    bit_reset(coder->is_rep[i]);
    bit_reset(coder->is_rep0[i]);
    bit_reset(coder->is_rep1[i]);
    bit_reset(coder->is_rep2[i]);
  }
  for (uint32_t i = 0; i < FULL_DISTANCES - DIST_MODEL_END; ++i) bit_reset(coder->dist_special[i]);
  for (uint32_t i = 0; i < DIST_STATES; ++i) bittree_reset(coder->dist_slot[i], DIST_SLOT_BITS);
  bittree_reset(coder->dist_align, ALIGN_BITS);
  length_encoder_reset(&coder->match_len_encoder, 1U << options->pb, coder->fast_mode);
  length_encoder_reset(&coder->rep_len_encoder, 1U << options->pb, coder->fast_mode);
  // big enough to have the price tables filled before their first use
  coder->match_price_count = UINT32_MAX / 2;
  coder->align_price_count = UINT32_MAX / 2;
  coder->opts_end_index = 0;
  coder->opts_current_index = 0;
  return LZMA_OK;
}

static lzma_ret lzma_encoder_create(LZMA1** coder_ptr, const lzma_allocator* allocator, lzma_vli id,
                                    const lzma_options_lzma* options, LZOptions* lz) {
  if (*coder_ptr == NULL) {
    *coder_ptr = (LZMA1*)mem_alloc_zero(sizeof(LZMA1), allocator);
    if (*coder_ptr == NULL) return LZMA_MEM_ERROR;
  }
  LZMA1* coder = *coder_ptr;
  switch (options->mode) {
    case LZMA_MODE_FAST:
      coder->fast_mode = true;
      break;
    case LZMA_MODE_NORMAL: {
      coder->fast_mode = false;
      if (options->dict_size > (UINT32_C(1) << 30) + (UINT32_C(1) << 29)) return LZMA_OPTIONS_ERROR;
      uint32_t log_size = 0;
      while ((UINT32_C(1) << log_size) < options->dict_size) ++log_size;
      coder->dist_table_size = log_size * 2;
      const uint32_t nice_len = my_max(mf_get_hash_bytes(options->mf), options->nice_len);
      coder->match_len_encoder.table_size = nice_len + 1 - MATCH_LEN_MIN;
      coder->rep_len_encoder.table_size = nice_len + 1 - MATCH_LEN_MIN;
      break;
    }
    default:
      return LZMA_OPTIONS_ERROR;
  }
  // with a preset dictionary the first byte needn't be a literal
  coder->is_initialized = options->preset_dict != NULL && options->preset_dict_size > 0;
  coder->uncomp_size = 0;
  coder->use_eopm = id == LZMA_FILTER_LZMA1;  // never with LZMA2
  set_lz_options(lz, options);
  return lzma_encoder_reset(coder, options);
}

static uint8_t lzma_lclppb_encode(const lzma_options_lzma* options) {
  return (uint8_t)((options->pb * 5 + options->lp) * 9 + options->lc);
}

//----------------------------------------------------------------------------
// LZMA2 (lzma2_encoder.c), with the output written at once instead of the
// resumable copy states, since the output buffer is all there is

struct LZMA2 {
  enum { SEQ_INIT, SEQ_LZMA_ENCODE } sequence;
  lzma_options_lzma opt_cur;
  bool need_properties;
  bool need_state_reset;
  bool need_dictionary_reset;
  size_t uncompressed_size;  // of the current chunk
  size_t compressed_size;    // of the current chunk, without the header
  uint8_t buf[LZMA2_HEADER_MAX + LZMA2_CHUNK_MAX];
};

// LZMA chunk header into buf; returns the start of the header in buf
static size_t lzma2_header_lzma(LZMA2* coder) {
  size_t pos;
  if (coder->need_properties) {
    pos = 0;
    coder->buf[pos] = (uint8_t)(coder->need_dictionary_reset ? 0x80 + (3 << 5) : 0x80 + (2 << 5));
  } else {
    pos = 1;
    coder->buf[pos] = (uint8_t)(coder->need_state_reset ? 0x80 + (1 << 5) : 0x80);
  }
  const size_t start = pos;
  size_t size = coder->uncompressed_size - 1;
  coder->buf[pos] = (uint8_t)(coder->buf[pos] + (size >> 16));
  ++pos;
  coder->buf[pos++] = (uint8_t)((size >> 8) & 0xFF);
  coder->buf[pos++] = (uint8_t)(size & 0xFF);
  size = coder->compressed_size - 1;
  coder->buf[pos++] = (uint8_t)(size >> 8);
  coder->buf[pos++] = (uint8_t)(size & 0xFF);
  if (coder->need_properties) coder->buf[pos] = lzma_lclppb_encode(&coder->opt_cur);
  coder->need_properties = false;
  coder->need_state_reset = false;
  coder->need_dictionary_reset = false;
  return start;
}

static inline bool put_bytes(const uint8_t* src, size_t n, uint8_t* out, size_t* out_pos, size_t out_size) {
  if (out_size - *out_pos < n) return false;
  memcpy(out + *out_pos, src, n);
  *out_pos += n;
  return true;
}

static lzma_ret lzma2_encode(LZMA2* coder, LZMA1* lzma, MF* mf, uint8_t* out, size_t* out_pos, size_t out_size) {
  for (;;) {
    if (coder->sequence == LZMA2::SEQ_INIT) {
      // no input left: don't start a new chunk
      if (mf_unencoded(mf) == 0) {
        if (mf->action == ACT_FINISH) {
          if (*out_pos == out_size) return LZMA_BUF_ERROR;
          out[(*out_pos)++] = 0;  // end of payload
        }
        return mf->action == ACT_RUN ? LZMA_OK : LZMA_STREAM_END;
      }
      if (coder->need_state_reset) {
        const lzma_ret r = lzma_encoder_reset(lzma, &coder->opt_cur);
        if (r != LZMA_OK) return r;
      }
      coder->uncompressed_size = 0;
      coder->compressed_size = 0;
      coder->sequence = LZMA2::SEQ_LZMA_ENCODE;
    }
    // how much more uncompressed data this chunk can take
    const uint32_t left = LZMA2_UNCOMPRESSED_MAX - (uint32_t)coder->uncompressed_size;
    uint32_t limit;
    if (left < mf->match_len_max) limit = 0;  // the next symbol could make the chunk too big
    else limit = mf->read_pos - mf->read_ahead + left - mf->match_len_max;
    const uint32_t read_start = mf->read_pos - mf->read_ahead;
    const lzma_ret ret =
        lzma_encode(lzma, mf, coder->buf + LZMA2_HEADER_MAX, &coder->compressed_size, LZMA2_CHUNK_MAX, limit);
    coder->uncompressed_size += mf->read_pos - mf->read_ahead - read_start;
    if (ret != LZMA_STREAM_END) return ret;
    coder->sequence = LZMA2::SEQ_INIT;
    if (coder->compressed_size >= coder->uncompressed_size) {
      // didn't compress: store the chunk, including the read-ahead bytes
      coder->uncompressed_size += mf->read_ahead;
      mf->read_ahead = 0;
      uint8_t hdr[LZMA2_HEADER_UNCOMPRESSED];
      hdr[0] = (uint8_t)(coder->need_dictionary_reset ? 1 : 2);
      coder->need_dictionary_reset = false;
      hdr[1] = (uint8_t)((coder->uncompressed_size - 1) >> 8);
      hdr[2] = (uint8_t)((coder->uncompressed_size - 1) & 0xFF);
      coder->need_state_reset = true;
      if (!put_bytes(hdr, LZMA2_HEADER_UNCOMPRESSED, out, out_pos, out_size)) return LZMA_BUF_ERROR;
      if (out_size - *out_pos < coder->uncompressed_size) return LZMA_BUF_ERROR;
      mf_read(mf, out, out_pos, out_size, &coder->uncompressed_size);
      continue;
    }
    const size_t start = lzma2_header_lzma(coder);
    if (!put_bytes(coder->buf + start, LZMA2_HEADER_MAX + coder->compressed_size - start, out, out_pos, out_size))
      return LZMA_BUF_ERROR;
  }
}

//----------------------------------------------------------------------------
// LZ encoder driver (lz_encoder.c) for a single LZMA1 or LZMA2 filter

struct Encoder {
  const lzma_allocator* allocator;
  MF mf;
  LZMA1* lzma;
  LZMA2* lzma2;  // NULL for LZMA1

  explicit Encoder(const lzma_allocator* a) : allocator(a), lzma(NULL), lzma2(NULL) { memset(&mf, 0, sizeof(mf)); }
  ~Encoder() {
    mem_free(mf.son, allocator);
    mem_free(mf.hash, allocator);
    mem_free(mf.buffer, allocator);
    mem_free(lzma, allocator);
    mem_free(lzma2, allocator);
  }

  lzma_ret init(const lzma_filter& f) {
    if (f.options == NULL) return LZMA_PROG_ERROR;
    const lzma_options_lzma* opt = (const lzma_options_lzma*)f.options;
    LZOptions lz;
    lzma_ret ret;
    if (f.id == LZMA_FILTER_LZMA2) {
      lzma2 = (LZMA2*)mem_alloc(sizeof(LZMA2), allocator);
      if (lzma2 == NULL) return LZMA_MEM_ERROR;
      lzma2->opt_cur = *opt;
      lzma2->sequence = LZMA2::SEQ_INIT;
      lzma2->need_properties = true;
      lzma2->need_state_reset = false;
      lzma2->need_dictionary_reset = opt->preset_dict == NULL || opt->preset_dict_size == 0;
      ret = lzma_encoder_create(&lzma, allocator, f.id, &lzma2->opt_cur, &lz);
      if (ret != LZMA_OK) return ret;
      // keep enough history for uncompressed chunks
      if (lz.before_size + lz.dict_size < LZMA2_CHUNK_MAX) lz.before_size = LZMA2_CHUNK_MAX - lz.dict_size;
    } else if (f.id == LZMA_FILTER_LZMA1) {
      ret = lzma_encoder_create(&lzma, allocator, f.id, opt, &lz);
      if (ret != LZMA_OK) return ret;
    } else {
      return LZMA_OPTIONS_ERROR;
    }
    if (lz_encoder_prepare(&mf, &lz)) return LZMA_OPTIONS_ERROR;
    if (lz_encoder_init(&mf, &lz, allocator)) return LZMA_MEM_ERROR;
    return LZMA_OK;
  }

  void move_window() {
    // multiple of 16, to keep the low bits of read_pos
    const uint32_t move_offset = (mf.read_pos - mf.keep_size_before) & ~UINT32_C(15);
    const size_t move_size = mf.write_pos - move_offset;
    memmove(mf.buffer, mf.buffer + move_offset, move_size);
    mf.offset += move_offset;
    mf.read_pos -= move_offset;
    mf.read_limit -= move_offset;
    mf.write_pos -= move_offset;
  }

  void fill_window(const uint8_t* in, size_t* in_pos, size_t in_size, int action) {
    if (mf.read_pos >= mf.size - mf.keep_size_after) move_window();
    size_t n = in_size - *in_pos;
    if (n > mf.size - mf.write_pos) n = mf.size - mf.write_pos;
    if (n != 0) memcpy(mf.buffer + mf.write_pos, in + *in_pos, n);
    *in_pos += n;
    mf.write_pos += (uint32_t)n;
    memset(mf.buffer + mf.write_pos, 0, MEMCMPLEN_EXTRA);
    if (action != ACT_RUN && *in_pos == in_size) {
      // end of input: read_pos may reach write_pos
      mf.action = action;
      mf.read_limit = mf.write_pos;
    } else if (mf.write_pos > mf.keep_size_after) {
      mf.read_limit = mf.write_pos - mf.keep_size_after;
    }
    // restart the match finder after a sync flush (preset dictionary)
    if (mf.pending > 0 && mf.read_pos < mf.read_limit) {
      const uint32_t pending = mf.pending;
      mf.pending = 0;
      mf.read_pos -= pending;
      mf.skip(&mf, pending);
    }
  }

  // all of in, finished; LZMA_STREAM_END on success
  lzma_ret run(const uint8_t* in, size_t in_size, uint8_t* out, size_t* out_pos, size_t out_size) {
    size_t in_pos = 0;
    for (;;) {
      if (mf.action == ACT_RUN && mf.read_pos >= mf.read_limit) fill_window(in, &in_pos, in_size, ACT_FINISH);
      const lzma_ret ret = lzma2 != NULL ? lzma2_encode(lzma2, lzma, &mf, out, out_pos, out_size)
                                         : lzma_encode(lzma, &mf, out, out_pos, out_size, UINT32_MAX);
      if (ret != LZMA_OK) return ret;
    }
  }
};

static uint64_t lz_encoder_memusage(const LZOptions* lz) {
  MF mf;
  memset(&mf, 0, sizeof(mf));
  if (lz_encoder_prepare(&mf, lz)) return UINT64_MAX;
  return ((uint64_t)mf.hash_count + mf.sons_count) * sizeof(uint32_t) + mf.size + sizeof(Encoder);
}

static uint64_t lzma_encoder_memusage(const lzma_options_lzma* options) {
  if (options == NULL || !is_options_valid(options)) return UINT64_MAX;
  LZOptions lz;
  set_lz_options(&lz, options);
  const uint64_t lz_memusage = lz_encoder_memusage(&lz);
  if (lz_memusage == UINT64_MAX) return UINT64_MAX;
  return (uint64_t)sizeof(LZMA1) + lz_memusage;
}

// one LZMA1 or LZMA2 filter, then LZMA_VLI_UNKNOWN
static lzma_ret check_chain(const lzma_filter* filters) {
  if (filters == NULL || filters[0].id == LZMA_VLI_UNKNOWN) return LZMA_PROG_ERROR;
  if (filters[0].id != LZMA_FILTER_LZMA1 && filters[0].id != LZMA_FILTER_LZMA2) return LZMA_OPTIONS_ERROR;
  if (filters[1].id != LZMA_VLI_UNKNOWN) return LZMA_OPTIONS_ERROR;
  return LZMA_OK;
}

}  // namespace lzma_hpp

//----------------------------------------------------------------------------
// API functions

static inline lzma_bool lzma_lzma_preset(lzma_options_lzma* options, uint32_t preset) {
  const uint32_t level = preset & LZMA_PRESET_LEVEL_MASK;
  const uint32_t flags = preset & ~LZMA_PRESET_LEVEL_MASK;
  if (level > 9 || (flags & ~LZMA_PRESET_EXTREME)) return 1;
  options->preset_dict = NULL;
  options->preset_dict_size = 0;
  options->lc = LZMA_LC_DEFAULT;
  options->lp = LZMA_LP_DEFAULT;
  options->pb = LZMA_PB_DEFAULT;
  static const uint8_t dict_pow2[] = {18, 20, 21, 22, 22, 23, 23, 24, 25, 26};
  options->dict_size = UINT32_C(1) << dict_pow2[level];
  if (level <= 3) {
    options->mode = LZMA_MODE_FAST;
    options->mf = level == 0 ? LZMA_MF_HC3 : LZMA_MF_HC4;
    options->nice_len = level <= 1 ? 128 : 273;
    static const uint8_t depths[] = {4, 8, 24, 48};
    options->depth = depths[level];
  } else {
    options->mode = LZMA_MODE_NORMAL;
    options->mf = LZMA_MF_BT4;
    options->nice_len = level == 4 ? 16 : level == 5 ? 32 : 64;
    options->depth = 0;
  }
  if (flags & LZMA_PRESET_EXTREME) {
    options->mode = LZMA_MODE_NORMAL;
    options->mf = LZMA_MF_BT4;
    if (level == 3 || level == 5) {
      options->nice_len = 192;
      options->depth = 0;
    } else {
      options->nice_len = 273;
      options->depth = 512;
    }
  }
  return 0;
}

static inline lzma_bool lzma_mf_is_supported(lzma_match_finder mf) {
  return mf == LZMA_MF_HC3 || mf == LZMA_MF_HC4 || mf == LZMA_MF_BT2 || mf == LZMA_MF_BT3 || mf == LZMA_MF_BT4;
}

static inline lzma_bool lzma_mode_is_supported(lzma_mode mode) {
  return mode == LZMA_MODE_FAST || mode == LZMA_MODE_NORMAL;
}

// Memory needed by lzma_raw_buffer_encode() with these filters (estimate, as
// in liblzma), UINT64_MAX if the chain or the options are invalid.
static inline uint64_t lzma_raw_encoder_memusage(const lzma_filter* filters) {
  using namespace lzma_hpp;
  if (check_chain(filters) != LZMA_OK) return UINT64_MAX;
  uint64_t m = lzma_encoder_memusage((const lzma_options_lzma*)filters[0].options);
  if (m == UINT64_MAX) return UINT64_MAX;
  if (filters[0].id == LZMA_FILTER_LZMA2) m += sizeof(LZMA2);
  return m + (UINT64_C(1) << 15);
}

// Raw (headerless) encoding of in[0..in_size) to out[*out_pos..out_size).
// *out_pos is advanced on success and left unchanged on error;
// LZMA_BUF_ERROR: out is too small.
static inline lzma_ret lzma_raw_buffer_encode(const lzma_filter* filters, const lzma_allocator* allocator,
                                              const uint8_t* in, size_t in_size, uint8_t* out, size_t* out_pos,
                                              size_t out_size) {
  using namespace lzma_hpp;
  if ((in == NULL && in_size != 0) || out == NULL || out_pos == NULL || *out_pos > out_size) return LZMA_PROG_ERROR;
  lzma_ret ret = check_chain(filters);
  if (ret != LZMA_OK) return ret;
  Encoder enc(allocator);
  ret = enc.init(filters[0]);
  if (ret != LZMA_OK) return ret;
  const size_t out_start = *out_pos;
  ret = enc.run(in, in_size, out, out_pos, out_size);
  if (ret == LZMA_STREAM_END) return LZMA_OK;
  *out_pos = out_start;
  return ret;
}

#endif  // LZMA_HPP_INCLUDED
