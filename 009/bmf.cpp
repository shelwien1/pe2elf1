#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pmmintrin.h>
#include <xmmintrin.h>
#define BYTEn(x, n) (*((uint8_t*)&(x)+(n)))
#define WORDn(x, n) (*((uint16_t*)&(x)+(n)))
#define LOBYTE(x) BYTEn(x, 0)
#define LOWORD(x) WORDn(x, 0)
#define HIBYTE(x) BYTEn(x, sizeof(x)-1)
#define HIWORD(x) WORDn(x, sizeof(x)/2-1)
#define BYTE1(x) BYTEn(x, 1)
#define BYTE2(x) BYTEn(x, 2)
#define BYTE4(x) BYTEn(x, 4)
#define WORD2(x) WORDn(x, 2)
#define __PAIR64__(high, low) (((uint64_t)(high)<<32)|(uint32_t)(low))
inline uint32_t abs32(int32_t x) {
  return x>=0 ? (uint32_t)x : (uint32_t)-x;
}

inline int8_t __OFSUB__(int32_t x, int32_t y) {
  int8_t sx = (int8_t)(x<0);
  return (int8_t)((sx^(int8_t)(y<0))&(sx^(int8_t)((x-y)<0)));
}

#include "memory.inc"

__attribute__((noreturn)) void __exit_402E40(int32_t Code, ...) {
  va_list ap;
  static const char*const message[] = {
    "", "File not found: %s", "Unknown option: %s", "%s: bad file!", "Read error!", "Write error for file %s", "Can't open file: %s", "Out of memory!", "User break!",
  };
  va_start(ap, Code);
  vprintf(message[(uint32_t)Code<9 ? Code : 4], ap);
  va_end(ap);
  printf("\n");
  exit(Code);
  __builtin_unreachable();
}

alignas(16) static uint8_t ctx_group_flags[32] = {
  0, 1, 2, 4, 8, 10, 13, 16, 17, 22, 32, 35, 36, 56, 63, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
alignas(16) static int32_t p2_ctx_rotate[4] = {
  4, 8, 12, 8,
};
alignas(16) static uint8_t p2_ctx_edges[16] = {
  17, 20, 27, 37, 49, 70, 93, 124, 157, 191, 205, 228, 235, 236, 237, 0,
};
alignas(16) static uint8_t p2_len_edges[32] = {
  4, 6, 8, 11, 14, 17, 21, 25, 30, 37, 45, 55, 67, 87, 120, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
alignas(16) static int32_t bmf_p2_thresholds[6][13] = {
  {6, 14, 28, 4, 8, 26, 4096, 6144, 13056, 10240, 14336, 26624, 36864}, {4, 15, 42, 1, 8, 19, 2560, 6912, 11264, 10240, 28672, 71680, 139264}, {5, 14, 21, 4, 9, 25, 3840, 8448, 12032, 16384, 26624, 43008, 83968}, {6, 11, 20, 5, 11, 23, 2816, 5888, 10496, 22528, 32768, 51200, 131072}, {0, 11, 30, 7, 13, 22, 4352, 8448, 14848, 16384, 38912, 57344, 94208}, {7, 10, 31, -1, 11, 21, -256, 1024, 5888, -2048, -2048, -2048, 145408},
};
alignas(16) static int32_t p2_float_pool[17] = {
  0x43293333, 0x3F800000, (int32_t)0xC52E0702, 0x3A83126F, 0x38D1B717, 0x3DCCCCCD, 0x40000000, 0x44100000, 0x40266666, 0x46D22000, 0x3C54FDF4, 0x459D8800, 0x44044000, 0x3951B717, 0x41200000, 0x3F800000, 0x3F800000,
};
alignas(16) static uint8_t p1_level_edges[8] = {
  1, 2, 4, 8, 14, 35, 103, 0,
};
alignas(16) static uint8_t p1_group_edges[8] = {
  1, 3, 6, 10, 16, 27, 52, 0,
};
alignas(16) static uint8_t p1_slot_edges[8] = {
  5, 10, 36, 98, 154, 236, 248, 0,
};
alignas(16) static int32_t p1_level_step[8] = {
  1, 1, 2, 2, 2, 4, 4, 4,
};
alignas(16) static float bmf_p2_coef[7][4] = {
  {-0.05f, -0.07f, 0.3f, 0.18f}, {-0.03f, 0.04f, 0.06f, -0.01f}, {0.06f, 0.16f, 0.0f, -0.01f}, {-0.1f, -0.02f, 0.05f, 0.05f}, {0.06f, 0.0f, 0.04f, 0.02f}, {0.04f, 0.02f, 0.04f, 0.05f}, {0.07f, 0.0f, 0.03f, 0.02f},
};
alignas(16) static float bmf_p2_rate[7][4] = {
  {0.0108f, 0.0069f, 0.0054f, 0.0052f}, {0.0042f, 0.0042f, 0.0042f, 0.0039f}, {0.0035f, 0.0035f, 0.0028f, 0.0028f}, {0.0027f, 0.0026f, 0.0026f, 0.0024f}, {0.0023f, 0.0023f, 0.0022f, 0.0021f}, {0.0019f, 0.0019f, 0.0017f, 0.0017f}, {0.0015f, 0.0015f, 0.0011f, 0.0009f},
};
static const float bmf_p2_rate_reset = 0.0024f;
static const float bmf_p2_ms_rate = 0.023f;
alignas(16) static void* coded_block = nullptr;
alignas(16) static int32_t plane_predictor = 0;
alignas(16) static int32_t plane_alt_model = 0;
alignas(16) static int32_t packer_free_bits = 0;
alignas(16) static uint32_t packer_acc = 0;
alignas(16) static int32_t coded_size = 0;
static int32_t desc_slow_mode;
static int32_t alphabet_reduced;
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
static PlaneDesc plane_desc[5];
#define BMF_SPILL_PAD(n) uint8_t _spill_pad ## n[sizeof(void*)-4]
static int32_t &plane_count = *&plane_desc[0].weight1;
static int32_t &near_lossless_q = *&plane_desc[0].weight2;
static uint8_t model_geometry[128];
static int8_t exclusion_gen;
static int32_t alt_freq_limit;
static int32_t alt_freq_init;
struct LevelGeom {
  uint8_t first;
  uint8_t half;
  uint8_t tbl_base;
  uint8_t _pad;
};
static LevelGeom level_geom[8];
static int32_t ctx_bias[4];
static int32_t deadzone_hi;
static int32_t deadzone_lo;
static constexpr int32_t opt_use_filters = 1;
static constexpr int32_t opt_slow = 1;
static constexpr int32_t opt_filter_template = 0;
static constexpr int32_t opt_pack_output = 1;
static constexpr int32_t opt_search_quality = 9;
static constexpr int32_t opt_max_error = 0;
static uint32_t* packer_word;
static uint8_t* out_cursor;
static uint8_t* coded_buf;
static uint8_t* hist_scratch;
alignas(16) static uint8_t bss_exclusion[8192+544];
static int8_t(&exclusion_mask)[8192] = *(int8_t(*)[8192]) bss_exclusion;
static uint8_t(&__byte_445440)[544] = *(uint8_t(*)[544])(bss_exclusion+8192);
static uint16_t* model_tables;
static inline uint16_t*model_strip(uint32_t k) {
  return model_tables+254*k;
}

static int32_t mode_symbol[5];
static inline void bmf_zero16(void* p) {
  __builtin_memset(p, 0, 16);
}

static inline uint8_t*bmf_plane_desc(int32_t off) {
  return (uint8_t*)&plane_desc[1]+off;
}

__attribute__((noreturn)) void __exit_402E40(int32_t Code, ...);
struct CtxWeight {
  int32_t sel;
  uint32_t w[3];
};
struct CounterNode {
  uint16_t total;
  uint16_t c[7];
};
struct P1Ctx {
  uint8_t sym;
  uint8_t mag;
};
struct AltP1Block {
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
  CounterNode counters[629856];
  int32_t ctx_of(AltP1Block* nb0, AltP1Block* nb1);
  void d8_encode_body(uint8_t* src, uint8_t* out);
  int32_t update_model();
  AltP1Block*alt_p1_alloc(int32_t img_w, int32_t img_h, int32_t plane);
  AltP1Block*alt_p1_free(int8_t do_free);
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
};
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
struct SymList {
  uint32_t n;
  uint32_t live;
  uint32_t tot;
  uint32_t since_rescale;
  uint32_t rescale_at;
  SymEntry* ent;
  bool rescale(int32_t count);
  int32_t code_symbol(int32_t want);
  void add_weight(int32_t want, uint32_t add);
};
inline bool SymList::rescale(int32_t count) {
  int32_t half, bias, up_cnt, back_cnt, last_cnt;
  uint32_t n_left, running, since, due = rescale_at;
  uint16_t keep;
  SymEntry* cur, * prev, * up, * back;
  if( count<=251&&due>=since_rescale )
    return false;
  n_left = live;
  bias = due<20*n;
  cur = ent-1;
  prev = cur;
  do {
    prev = cur;
    ++cur;
    half = (bias+(uint32_t)cur->cnt)>>1;
    cur->cnt = half;
    if( cur!=ent ) {
      up = cur-1;
      up_cnt = up->cnt;
      if( half>up_cnt ) {
        keep = cur->sym;
        cur->set(up->sym, up_cnt);
        if( up!=ent ) {
          do {
            back = up-1;
            back_cnt = back->cnt;
            if( half<=back_cnt )
              break;
            up->set(back->sym, back_cnt);
            --up;
          } while( back!=ent );
        }
        up->set(keep, half);
      }
    }
    --n_left;
  } while( n_left );
  running = tot;
  if( !cur->cnt ) {
    do {
      ++n_left;
      tot = ++running;
      last_cnt = prev->cnt;
      --prev;
    } while( !last_cnt );
    live -= n_left;
  }
  since = since_rescale;
  tot = running-(running>>1);
  since_rescale = since-(since>>1);
  return true;
}

struct SymListBlock {
  uint32_t n;
  SymList list[0];
  static uint32_t bytes(uint32_t n) {
    return __builtin_offsetof(SymListBlock, list)+sizeof(SymList)*n;
  }
};
static inline void free_sym_entries(SymList* list, uint32_t n) {
  while( n-- )
    free(list[n].ent);
}

static inline SymListBlock*sym_list_block(SymList* list) {
  return (SymListBlock*)((uint8_t*)list-__builtin_offsetof(SymListBlock, list));
}

static inline uint32_t sym_list_count(const SymList* list) {
  return sym_list_block((SymList*)list)->n;
}

struct SymPair {
  uint16_t last;
  uint16_t prev;
};
struct FreqPair {
  uint16_t f[2];
};
static inline FreqPair*bit_tree(uint16_t* freq, int32_t lvl) {
  return (FreqPair*)&freq[2*level_geom[lvl].tbl_base+8];
}

struct BitCtr {
  uint16_t n[2];
  uint16_t limit;
  int32_t encode_context_bit(BitCtr* a2, int32_t bit);
  int32_t decode_context_bit(BitCtr* a2);
};
struct PixRec {
  union {
    struct {
      uint16_t sym;
      uint8_t match[6];
    };
    struct {
      uint8_t _head[4];
      uint32_t match2345;
    };
  };
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
  uint32_t bucket_idx;
  uint32_t sym_pos;
  uint32_t ctr_fallback;
  uint32_t ctr_node;
  PixRec* row_cur[10];
  FreqRec grid[65723];
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
  uint8_t* run_bucket;
  uint8_t* alpha_map;
  uint8_t grad[4];
  uint16_t sym_ctr[524288];
  SymPair group_ctr[15][65536];
  uint16_t* sym_cache;
  uint16_t* pix_cur;
  uint16_t sym_rev[8192];
  uint16_t ctx_id1[192512];
  uint16_t ctx_id2[108800];
  uint16_t ctx_id3[712000];
  int32_t pixel_context(uint32_t* nb);
  int32_t init_tables();
  void expand_alphabet();
  void unmodel_plane_slow(uint8_t* dst);
  int32_t decode_pixel(int32_t x);
  int32_t code_pixel(int32_t x);
};
struct P2Count {
  int8_t rate;
  uint8_t b1;
  int16_t weighted;
};
struct P2Freq {
  uint16_t step;
  uint16_t f[3];
  uint32_t rescale_three_way();
  void encode_symbol(const uint32_t* ctx_pair, int32_t sym);
  int32_t decode_symbol(const uint32_t* ctx_pair);
};
static inline int32_t p2_pred(int32_t weighted, int32_t rate) {
  return (weighted+(1<<((rate+31)&31)))>>(rate&31);
}

static inline int16_t p2_bump(int32_t w2, int32_t err, int32_t shift) {
  const uint32_t kick = 32u*(uint32_t)((err>deadzone_hi)-(err<deadzone_lo));
  return (int16_t)((uint32_t)w2+((kick+(uint32_t)err+(1u<<(shift-1)))>>shift));
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
struct AltP2Block {
  float nb_weights[1088][16][4];
  union {
    float p2_row[7][4];
    struct {
      uint8_t p2_row0_head[15];
      uint8_t _u0_0_4[97];
      float bias[4];
      float (*nb_cur)[4];
      float (**row0)[4];
      float (**row1)[4];
      float (**cur)[4];
      float (**above)[4];
      int32_t bank_ctx[5];
      int32_t _unused278696;
      uint16_t pred_prev;
      uint8_t _u0_0_12[2];
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
      uint8_t _u0_0_tail[8];
    };
  };
  uint8_t _pad1[40];
  int32_t nb_sum[10];
  int32_t ctx_delta[260];
  uint8_t fold[256];
  uint8_t fold_hi[256];
  int8_t unfold[256];
  uint8_t nb_ctx[120];
  int16_t nb_id[1916];
  uint8_t _pad2[8];
  P2Count p2_ctr[163840];
  P2Freq freq[15552];
  uint8_t _pad3[8];
  AltP2Block*alt_p2_alloc(int32_t img_w, int32_t plane);
  AltP2Block*alt_p2_free(int8_t do_free);
};
struct CtxWeights {
  float (*f0[6])[4];
};

#include "rc.inc"

static RangeCoder rc;

struct BmfArc {
  uint32_t images;
  FILE* fp;
  FILE*bmf_close_archive();
};
inline FILE*BmfArc::bmf_close_archive() {
  FILE* fp;
  fp = this->fp;
  if( fp ) {
    fseek(fp, 0, 2);
    fclose(this->fp);
    return nullptr;
  }
  return fp;
}

BmfArc*__bmf_destroy_archive(BmfArc* arc, int8_t do_free) {
  ((BmfArc*)arc)->bmf_close_archive();
  if( (do_free&1)!=0 )
    free(arc);
  return arc;
}

void __expand_predictor_mode0(uint8_t* unread_src, int32_t i, int32_t unread_h) {
}

uint32_t __predict_med(uint8_t* pixels, int32_t width, int32_t height) {
  uint8_t left;
  uint32_t done;
  int32_t rows_left, x_left, north, northwest, code, pairs, ofs;
  uint8_t pred, * q;
  uint32_t j, last, k;
  uint8_t* p, * up;
  alignas(16) uint8_t fold[272];
  p = (pixels+height*width);
  up = &p[-width];
  for( j = 0; j<128; ++j ) {
    fold[j] = (uint8_t)(2*j);
    fold[128+j] = (uint8_t)(-1-2*(int32_t)j);
  }
  rows_left = height-1;
  if( height!=1 ) {
    do {
      x_left = width-1;
      if( width!=1 ) {
        do {
          north = (uint8_t)*--up;
          pred = (uint8_t)*(--p-1);
          northwest = (uint8_t)p[-width-1];
          if( pred<north ) {
            if( northwest<pred )
              pred = (uint8_t)north;
            else if( northwest<=north )
              pred = (uint8_t)(north+pred-northwest);
          } else {
            if( northwest>pred )
              pred = (uint8_t)north;
            else if( northwest>=north )
              pred = (uint8_t)(north+pred-northwest);
          }
          code = (uint8_t)fold[(uint8_t)(*p-pred)];
          *p = code;
          ++*(uint32_t*)&hist_scratch[4*code];
          --x_left;
        } while( x_left );
      }
      --up;
      --p;
      last = (uint8_t)fold[(uint8_t)(*p-p[-width])];
      *p = last;
      ++*(uint32_t*)&hist_scratch[4*last];
      --rows_left;
    } while( rows_left );
  }
  if( width!=1 ) {
    last = width-1;
    pairs = (width-1)/2;
    if( pairs ) {
      k = 0;
      ofs = 0;
      do {
        left = p[ofs-2];
        ++k;
        p[ofs-1] = fold[(uint8_t)(p[ofs-1]-left)];
        p[ofs-2] = fold[(uint8_t)(left-p[ofs-3])];
        ofs -= 2;
      } while( k<(uint32_t)pairs );
      done = 2*k+1;
    } else {
      done = 1;
    }
    if( last>(done-1) ) {
      q = p-done;
      last = (uint8_t)(*q-*((int8_t*)q-1));
      *q = fold[(uint8_t)last];
    }
  }
  return last;
}

uint32_t __alt_init_tables(uint8_t* fold, int8_t* unfold) {
  uint8_t even, odd;
  uint32_t done, lo;
  int32_t bucket_size, in_bucket, ofs, bucket, bucket_1;
  uint8_t half;
  uint32_t i, k, b, span, pairs;
  uint8_t* pos, * neg;
  bucket_size = 2*near_lossless_q+1;
  *unfold = 0;
  unfold[255] = 0x80;
  for( i = 0; i<0x3F; ++i ) {
    ((uint8_t*)unfold)[4*i+2] = 2*i+1;
    ((uint8_t*)unfold)[4*i+1] = (uint8_t)(-2*i-1);
    ((uint8_t*)unfold)[4*i+4] = 2*i+2;
    ((uint8_t*)unfold)[4*i+3] = (uint8_t)(-2*i-2);
  }
  unfold[254] = 127;
  unfold[253] = -127;
  *fold = 0;
  lo = 1;
  fold[128] = (uint8_t)-1;
  {
    span = 128-lo;
    pairs = (128-lo)/2;
    half = 0;
    in_bucket = 1;
    if( pairs ) {
      k = 0;
      ofs = 0;
      pos = &(fold)[lo];
      neg = fold-lo;
      do {
        bucket = bucket_size;
        bucket_1 = in_bucket-1;
        if( bucket_1 )
          bucket = bucket_1;
        else
          ++half;
        even = 2*half;
        pos[2*k] = 2*half;
        odd = 2*half-1;
        in_bucket = bucket-1;
        neg[ofs+256] = odd;
        if( bucket==1 ) {
          in_bucket = bucket_size;
          even = 2*++half;
          odd = 2*half-1;
        }
        pos[2*k+++1] = even;
        neg[ofs+255] = odd;
        ofs -= 2;
      } while( k<pairs );
      done = 2*k+1;
    } else {
      done = 1;
    }
    if( (done-1)<span ) {
      if( in_bucket==1 )
        half = half+1;
      (fold)[lo-1+done] = 2*half;
      fold[257-lo-done] = 2*half-1;
    }
  }
  for( b = 0; b<0x80; ++b ) {
    (fold)[(uint8_t)((uint8_t*)unfold)[2*b]+256] = 2*b;
    (fold)[(uint8_t)((uint8_t*)unfold)[2*b+1]+256] = 2*b+1;
  }
  return b;
}

uint32_t __rc_decode_flat(uint32_t tot) {
  uint32_t sym = rc.get_freq(tot);
  rc.decode(sym, sym+1, tot);
  return sym;
}

inline int32_t BitCtr::encode_context_bit(BitCtr* a2, int32_t bit) {
  uint32_t c1, p0, p1, c0;
  int32_t par_tot;
  int32_t cap, result, p_cap, par0, par_n;
  uint32_t tot, p_tot, n1_old, p1_old;
  c0 = this->n[0];
  if( this->n[0] ) {
    c1 = this->n[1];
    if( !this->n[1] ) {
      par0 = a2->n[0];
      par_tot = par0+a2->n[1];
      this->n[0] = (par_tot+(par0<<6)-64)/par_tot;
      this->n[1] = ((a2->n[1]<<6)+par_tot-64)/par_tot;
      this->n[c0-1] += 4;
      this->limit = 512;
      par_n = a2->n[c0-1];
      a2->n[c0-1] = -3*((uint32_t)(3-par_n)>>31)+par_n;
      c0 = this->n[0];
      c1 = this->n[1];
    }
    tot = c0+c1;
    rc.encode_bit(c0, c1, bit);
    cap = this->limit;
    if( tot>(uint32_t)cap ) {
      n1_old = this->n[1];
      this->n[0] -= this->n[0]>>1;
      this->n[1] = n1_old-(n1_old>>1);
      if( cap<0x4000 )
        this->limit = cap+64;
    }
    result = this->n[bit]+8;
    this->n[bit] = result;
    a2->n[bit] += (uint32_t)tot<0x88;
    return result;
  }
  p0 = a2->n[0];
  p1 = a2->n[1];
  p_tot = p0+p1;
  rc.encode_bit(p0, p1, bit);
  p_cap = a2->limit;
  if( p_tot>(uint32_t)p_cap ) {
    p1_old = a2->n[1];
    a2->n[0] -= a2->n[0]>>1;
    a2->n[1] = p1_old-(p1_old>>1);
    if( p_cap<0x4000 )
      a2->limit = p_cap+64;
  }
  result = a2->n[bit]+8;
  a2->n[bit] = result;
  this->n[0] = bit+1;
  return result;
}

inline int32_t BitCtr::decode_context_bit(BitCtr* a2) {
  uint32_t c0, c1, p0, p1;
  int32_t par_tot;
  int32_t result, cap, p_cap, par0, par_n;
  uint32_t tot, p_tot, n1_old, p1_old;
  c0 = this->n[0];
  if( this->n[0] ) {
    c1 = this->n[1];
    if( !this->n[1] ) {
      par0 = a2->n[0];
      par_tot = par0+a2->n[1];
      this->n[0] = (par_tot+(par0<<6)-64)/par_tot;
      this->n[1] = ((a2->n[1]<<6)+par_tot-64)/par_tot;
      this->n[c0-1] += 4;
      this->limit = 512;
      par_n = a2->n[c0-1];
      a2->n[c0-1] = -3*((uint32_t)(3-par_n)>>31)+par_n;
      c0 = this->n[0];
      c1 = this->n[1];
    }
    tot = c0+c1;
    result = rc.decode_bit(c0, c1);
    cap = this->limit;
    if( tot>(uint32_t)cap ) {
      n1_old = this->n[1];
      this->n[0] -= this->n[0]>>1;
      this->n[1] = n1_old-(n1_old>>1);
      if( cap<0x4000 )
        this->limit = cap+64;
    }
    this->n[result] += 8;
    a2->n[result] += (uint32_t)tot<0x88;
  } else {
    p0 = a2->n[0];
    p1 = a2->n[1];
    p_tot = p0+p1;
    result = rc.decode_bit(p0, p1);
    p_cap = a2->limit;
    if( p_tot>(uint32_t)p_cap ) {
      p1_old = a2->n[1];
      a2->n[0] -= a2->n[0]>>1;
      a2->n[1] = p1_old-(p1_old>>1);
      if( p_cap<0x4000 )
        a2->limit = p_cap+64;
    }
    a2->n[result] += 8;
    this->n[0] = result+1;
  }
  return result;
}

inline int32_t SymList::code_symbol(int32_t want) {
  int8_t gen;
  int32_t enc_cum, enc_high, enc_tot;
  uint32_t left;
  uint8_t c_a, c_b;
  SymEntry* p, * head, * q;
  int32_t cum, s, c, result, c2, top;
  uint16_t s_a, s_b;
  uint32_t rest, i;
  gen = exclusion_gen;
  left = this->live;
  p = this->ent-1;
  cum = 0;
  while( 1 ) {
    ++p;
    s = p->sym;
    if( exclusion_mask[s]!=exclusion_gen ) {
      c = p->cnt;
      cum += c;
      if( s==want )
        break;
    }
    if( !--left ) {
      if( !cum )
        return 0;
      enc_cum = cum;
      enc_tot = this->tot+cum;
      enc_high = enc_tot;
      do {
        exclusion_mask[p->sym] = gen;
        --p;
      } while( p>=this->ent );
      result = 0;
      {
        rc.encode(enc_cum, enc_high, enc_tot);
        return result;
      }
    }
  }
  enc_high = cum;
  rest = left-1;
  enc_cum = cum-c;
  if( rest ) {
    for( i = 0; i<rest; ++i ) {
      if( exclusion_mask[p[i+1].sym]==exclusion_gen )
        c2 = 0;
      else
        c2 = p[i+1].cnt;
      cum += c2;
    }
  }
  enc_tot = this->tot+cum;
  p->cnt += 4;
  head = this->ent;
  this->since_rescale += 4;
  if( p==head ) {
LABEL_37:
    top = p->cnt;
  } else {
    s_a = p->sym;
    c_a = p->cnt;
    q = p-1;
    *p = *q;
    q->set(s_a, c_a);
    head = this->ent;
    if( q==head ) {
      top = q->cnt;
    } else {
      while( 1 ) {
        top = q->cnt;
        p = q-1;
        if( top<=p->cnt )
          break;
        s_b = q->sym;
        c_b = q->cnt;
        *q = *p;
        p->set(s_b, c_b);
        head = this->ent;
        --q;
        if( p==head )
          goto LABEL_37;
      }
    }
  }
  this->rescale(top);
  result = 1;
  rc.encode(enc_cum, enc_high, enc_tot);
  return result;
}

inline void SymList::add_weight(int32_t want, uint32_t add) {
  SymEntry* list;
  uint32_t count;
  bool full;
  uint8_t c3, c, c2;
  SymEntry* q, * p, * head;
  uint16_t s3, s, s2;
  int32_t recycled;
  uint32_t n_live, left;
  list = this->ent;
  n_live = this->live;
  p = list;
  left = n_live;
  if( n_live ) {
    while( p->sym!=want ) {
      ++p;
      if( !--left )
        goto LABEL_4;
    }
    p->cnt += add;
    this->since_rescale += add;
    head = this->ent;
    if( p==head ) {
LABEL_16:
      count = p->cnt;
    } else {
      s = p->sym;
      c = p->cnt;
      q = p-1;
      *p = *q;
      q->set(s, c);
      head = this->ent;
      if( q==head ) {
        count = q->cnt;
      } else {
        while( 1 ) {
          count = q->cnt;
          p = q-1;
          if( count<=p->cnt )
            break;
          s2 = q->sym;
          c2 = q->cnt;
          *q = *p;
          p->set(s2, c2);
          head = this->ent;
          --q;
          if( p==head )
            goto LABEL_16;
        }
      }
    }
    this->rescale(count);
    return;
  } else {
LABEL_4:
    full = n_live==this->n;
    if( n_live>=this->n ) {
      if( add<=1 )
        return;
      full = n_live==this->n;
    }
    if( full ) {
      this->live = --n_live;
      recycled = list[n_live].cnt;
    } else {
      recycled = 1;
    }
    list += n_live;
    this->live = n_live+1;
    this->tot = recycled+this->tot+1;
    list->cnt = 2;
    list->sym = want;
    this->since_rescale += 4;
    if( list!=this->ent ) {
      s3 = list->sym;
      c3 = list->cnt;
      *list = list[-1];
      list[-1].set(s3, c3);
    }
  }
}

CounterNode*__init_counter_node(CounterNode* node) {
  node->c[0] = 8;
  node->c[1] = 2;
  node->c[2] = 2;
  node->c[3] = 2;
  node->c[4] = 2;
  node->c[5] = 3;
  node->c[6] = 3;
  node->total = 22;
  return node;
}

void __encode_symbol_tree(uint16_t* freq, int32_t sym) {
  uint16_t add;
  int16_t sum4;
  uint32_t cum;
  int32_t lvl, result, go, fa, esc, path, node, span, f1, mask;
  uint16_t* f0, fq, h2, h4, * slot;
  FreqPair* pair;
  uint32_t j, f1_old, tot, cum_hi;
  uint16_t h9, acc8, acc6, acc;
  lvl = model_geometry[sym];
  f0 = freq+2;
  cum = 0;
  for( j = 0; j<(uint32_t)lvl; j++ )
    cum += f0[j];
  slot = &f0[lvl];
  cum_hi = cum+*slot;
  tot = *freq;
  result = rc.encode(cum, cum_hi, tot);
  if( *freq>0x4000u ) {
    h2 = *(freq+2)-(*(freq+2)>>1);
    acc = *(freq+3);
    *(freq+2) = h2;
    acc = acc-(acc>>1);
    *(freq+3) = acc;
    acc = h2+acc;
    h4 = *(freq+4)-(*(freq+4)>>1);
    *(freq+4) = h4;
    sum4 = acc+h4;
    acc = *(freq+5)-(*(freq+5)>>1);
    acc6 = *(freq+6);
    *(freq+5) = acc;
    acc6 = acc6-(acc6>>1);
    *(freq+6) = acc6;
    acc6 = sum4+acc+acc6;
    acc = *(freq+7)-(*(freq+7)>>1);
    acc8 = *(freq+8);
    *(freq+7) = acc;
    acc = acc6+acc;
    acc8 = acc8-(acc8>>1);
    h9 = *(freq+9);
    *(freq+8) = acc8;
    acc8 = acc+acc8;
    h9 = h9-(h9>>1);
    esc = *(freq+1);
    *(freq+9) = h9;
    *freq = acc8+h9;
    if( (esc<=4*alt_freq_limit) ) {
      result = 4*(esc>alt_freq_limit);
      add = esc-result;
      *(freq+1) = esc-result;
    } else {
      add = esc-16;
      *(freq+1) = esc-16;
    }
  } else {
    add = *(freq+1);
  }
  *slot += add;
  *freq += *(freq+1);
  if( sym>=2 ) {
    mask = level_geom[lvl].half;
    path = sym-level_geom[lvl].first;
    node = 0;
    for( span = 1;; span *= 2 ) {
      pair = bit_tree(freq, lvl)+span+node;
      f1 = pair->f[1];
      go = (mask&path)!=0;
      fa = pair->f[0];
      result = rc.encode_bit(fa, f1, go);
      fq = pair->f[go];
      if( fq>0x4000u ) {
        f1_old = pair->f[1];
        pair->f[0] -= pair->f[0]>>1;
        pair->f[1] = f1_old-(f1_old>>1);
        fq = pair->f[go];
      }
      pair->f[go] = alt_freq_init+fq;
      mask >>= 1;
      node = go+2*node;
      if( !mask )
        return;
    }
  }
  return;
}

void __alt_p1_encode_symbol(uint16_t* freq, int32_t unread_ctx, int32_t ctx, int32_t sym) {
  bool done;
  uint16_t tot0;
  uint32_t cum;
  int32_t slot, result, lo;
  uint16_t* p, * q, nf, * cur, * base;
  uint32_t tot, k, cum_hi;
  tot0 = *freq;
  slot = 6-(sym&1);
  if( sym<5 )
    slot = sym;
  base = freq+1;
  cur = freq+1;
  tot = tot0&0x7FFF;
  cum = 0;
  for( k = 0; k<(uint32_t)slot; k++ )
    cum += base[k];
  cur = &base[slot];
  cum_hi = cum+*cur;
  rc.encode(cum, cum_hi, tot);
  if( tot>0x2000 ) {
    *freq = 0x8000;
    p = freq+7;
    lo = 256;
    done = freq+7<base;
    q = freq+7;
    if( freq+7>=base ) {
      do {
        if( *q<lo )
          lo = *q;
        --q;
      } while( q>=base );
      done = p<base;
    }
    if( !done ) {
      do {
        if( (uint16_t)lo<=1u )
          nf = *p-(*p>>1);
        else
          nf = (*p+2)/3;
        *p = nf;
        *freq += nf;
        --p;
      } while( p>=base );
    }
  }
  *cur += 32;
  result = *freq+32;
  *freq = result;
  if( slot>=5 )
    return __encode_symbol_tree(model_strip(128*(uint32_t)(slot&1)+((((freq[1]+(result&0x7FFF)+96-2*(uint32_t)freq[slot+1])>>25)&0xFFFFFFC0)+(uint32_t)ctx)), (sym-5)>>1);
  return;
}

int32_t __decode_symbol_tree(uint16_t* freq) {
  int16_t sum4, sum6;
  int32_t k, fa, go, node1, esc, mask, node, sym, span, f1;
  uint16_t* cur, add, fq, h2, h4, h6, h8, * slot;
  FreqPair* pair;
  uint32_t target, cum, tot;
  uint16_t acc8, acc;
  tot = *freq;
  sym = 0;
  target = rc.get_freq(tot);
  cur = freq+2;
  cum = *(freq+2);
  if( cum<=target ) {
    k = 0;
    do {
      ++cur;
      ++k;
      cum += *cur;
    } while( cum<=target );
    sym = k;
  }
  slot = cur;
  rc.decode((cum-*cur), cum, tot);
  if( *freq>0x4000u ) {
    h2 = *(freq+2)-(*(freq+2)>>1);
    acc = *(freq+3);
    *(freq+2) = h2;
    acc = acc-(acc>>1);
    *(freq+3) = acc;
    acc = h2+acc;
    h4 = *(freq+4)-(*(freq+4)>>1);
    *(freq+4) = h4;
    sum4 = acc+h4;
    acc = *(freq+5)-(*(freq+5)>>1);
    *(freq+5) = acc;
    acc = sum4+acc;
    h6 = *(freq+6)-(*(freq+6)>>1);
    *(freq+6) = h6;
    sum6 = acc+h6;
    acc = *(freq+7)-(*(freq+7)>>1);
    *(freq+7) = acc;
    acc = sum6+acc;
    h8 = *(freq+8)-(*(freq+8)>>1);
    acc8 = *(freq+9);
    *(freq+8) = h8;
    acc8 = acc8-(acc8>>1);
    *(freq+9) = acc8;
    acc8 = acc+h8+acc8;
    esc = *(freq+1);
    *freq = acc8;
    if( esc<=4*alt_freq_limit )
      add = esc-4*(esc>alt_freq_limit);
    else
      add = esc-16;
    *(freq+1) = add;
  } else {
    add = *(freq+1);
  }
  *slot += add;
  *freq += *(freq+1);
  if( sym<2 )
    return sym;
  else {
    mask = level_geom[sym].half;
    node = 0;
    span = 1;
    do {
      pair = bit_tree(freq, sym)+span+node;
      fa = pair->f[0];
      f1 = pair->f[1];
      go = rc.decode_bit(fa, f1);
      fq = pair->f[go];
      if( fq>0x4000u ) {
        pair->f[0] -= pair->f[0]>>1;
        pair->f[1] -= pair->f[1]>>1;
        fq = pair->f[go];
      }
      pair->f[go] = alt_freq_init+fq;
      mask >>= 1;
      span *= 2;
      node1 = go+2*node;
      node = node1;
    } while( mask );
    return node1+level_geom[sym].first;
  }
}

int32_t __alt_p1_decode_symbol(uint16_t* freq, int32_t unread_arg, int32_t ctx) {
  bool done;
  int16_t tot_new, nf;
  int32_t slot, lo;
  uint16_t* cur, * p, * q;
  uint32_t tot, sym, cum;
  uint16_t* base;
  tot = *freq&0x7FFF;
  sym = rc.get_freq(tot);
  cur = freq+1;
  cum = (uint16_t)freq[1];
  base = freq+1;
  while( cum<=sym )
    cum += (uint16_t)*++cur;
  rc.decode(cum-(uint16_t)*cur, cum, tot);
  if( tot>0x2000 ) {
    *freq = 0x8000;
    p = freq+7;
    lo = 256;
    done = freq+7<base;
    q = freq+7;
    if( freq+7>=base ) {
      do {
        if( *q<lo )
          lo = *q;
        --q;
      } while( q>=base );
      done = p<base;
    }
    if( !done ) {
      do {
        if( (uint16_t)lo<=1u )
          nf = *p-(*p>>1);
        else
          nf = ((uint16_t)*p+2)/3;
        *p = nf;
        *freq += nf;
        --p;
      } while( p>=base );
    }
  }
  *cur += 32;
  tot_new = *freq+32;
  *freq = tot_new;
  slot = cur-base;
  if( slot>=5 )
    slot += 2*__decode_symbol_tree(model_strip(128*(uint32_t)(slot&1)+(((((uint16_t)freq[1]+(tot_new&0x7FFF)+96-2*(uint32_t)(uint16_t)freq[slot+1])>>25)&0xFFFFFFC0)+(uint32_t)ctx)));
  return slot;
}

inline void P2Freq::encode_symbol(const uint32_t* ctx_pair, int32_t sym) {
  uint16_t f_before;
  uint32_t cum;
  uint16_t st, * slot;
  uint32_t tot, tot_1, f1_old, f2_old;
  uint16_t down;
  cum = this->f[1]+this->f[0];
  tot = cum+this->f[2];
  if( sym ) {
    if( (sym&1)!=0 ) {
      cum = this->f[0];
      slot = &this->f[1];
    } else {
      slot = &this->f[2];
    }
  } else {
    cum = 0;
    slot = &this->f[0];
  }
  tot_1 = cum+*slot;
  rc.encode(cum, tot_1, tot);
  f_before = *slot;
  if( *slot>0x4000u ) {
    f1_old = this->f[1];
    f2_old = this->f[2];
    this->f[0] -= this->f[0]>>1;
    st = this->step;
    this->f[1] = f1_old-(f1_old>>1);
    this->f[2] = f2_old-(f2_old>>1);
    if( st<=256 ) {
      if( st<=32 )
        down = ((uint32_t)(16-st)>>30)&0xFFFFFFFE;
      else
        down = 32;
      st = st-down;
      this->step = st;
      f_before = *slot;
    } else {
      st = (uint32_t)st>>1;
      this->step = st;
      f_before = *slot;
    }
  } else {
    st = this->step;
  }
  *slot = st+f_before;
  if( sym>0 )
    return __encode_symbol_tree(model_strip(ctx_pair[sym&1]), (sym-1)>>1);
}

inline int32_t P2Freq::decode_symbol(const uint32_t* ctx_pair) {
  uint32_t cum;
  int32_t target, idx, c01;
  uint16_t fq, st;
  uint16_t* slot, * base;
  uint32_t f1_old, f2_old, tot;
  c01 = this->f[1]+this->f[0];
  tot = this->f[2]+c01;
  target = rc.get_freq(tot);
  cum = this->f[0];
  if( (uint32_t)target>=cum ) {
    if( target>=c01 ) {
      cum = c01;
      slot = &this->f[2];
    } else {
      slot = &this->f[1];
    }
    base = &this->f[0];
  } else {
    slot = &this->f[0];
    cum = 0;
    base = &this->f[0];
  }
  rc.decode(cum, (cum+((uint16_t)*slot)), tot);
  fq = (uint16_t)*slot;
  if( fq>0x4000 ) {
    f1_old = this->f[1];
    f2_old = this->f[2];
    this->f[0] -= this->f[0]>>1;
    st = this->step;
    this->f[1] = f1_old-(f1_old>>1);
    this->f[2] = f2_old-(f2_old>>1);
    if( st<=256 ) {
      if( st<=32 )
        fq = ((uint32_t)(16-st)>>30)&0xFFFFFFFE;
      else
        fq = 32;
      st = st-fq;
      this->step = st;
      fq = *slot;
    } else {
      st = (uint32_t)st>>1;
      this->step = st;
      fq = *slot;
    }
  } else {
    st = this->step;
  }
  *slot = st+fq;
  idx = slot-base;
  if( idx )
    return idx+2*__decode_symbol_tree(model_strip(ctx_pair[idx&1]));
  else
    return 0;
}

inline uint32_t P2Freq::rescale_three_way() {
  uint32_t cut, budget;
  this->f[0] -= this->f[0]>>1;
  this->f[1] -= this->f[1]>>1;
  cut = this->f[2]>>1;
  this->f[2] -= cut;
  budget = this->step;
  if( budget<=0x100 ) {
    if( this->step<=0x20u )
      cut = ((16-budget)>>30)&0xFFFFFFFE;
    else
      cut = 32;
    this->step = budget-cut;
  } else {
    this->step >>= 1;
  }
  return cut;
}

int32_t __rescale_counter_pair(BitCtr* ctr) {
  int32_t tot;
  ctr->n[0] -= ctr->n[0]>>1;
  tot = ctr->limit;
  ctr->n[1] -= ctr->n[1]>>1;
  if( tot<0x4000 ) {
    tot += 64;
    ctr->limit = tot;
  }
  return tot;
}

void __rc_end_decode() {
  rc.finish();
  if( plane_alt_model )
    free(model_tables);
}

void __rc_end_encode() {
  rc.flush();
  if( plane_alt_model )
    free(model_tables);
}

void**__free_workspace(ModelBlock* blk, int8_t do_free) {
  uint32_t n, left;
  int32_t i;
  SymList* lists, * p;
  free(blk->sym_code);
  free(blk->sym_word);
  free(blk->run_bucket);
  free(*(void**)&blk->alpha_map);
  lists = blk->sel1_list;
  if( lists ) {
    n = sym_list_count(lists);
    if( n ) {
      left = n;
      p = &lists[n];
      do {
        --p;
        free(p->ent);
        --left;
      } while( left );
    }
    free(sym_list_block(lists));
  }
  lists = blk->sel0_list;
  if( lists ) {
    n = sym_list_count(lists);
    if( n ) {
      left = n;
      p = &lists[n];
      do {
        --p;
        free(p->ent);
        --left;
      } while( left );
    }
    free(sym_list_block(lists));
  }
  for( i = 0; i<5; ++i )
    free(blk->row_cur[i]);
  free(blk->escape.ent);
  if( (do_free&1)!=0 )
    free(blk);
  return (void**)blk;
}

static inline bool sym_in_top(const SymEntry* ent, int32_t n, int32_t sym) {
  while( n-- )
    if( ent[n].sym==sym )
      return true;
  return false;
}

inline int32_t ModelBlock::pixel_context(uint32_t* nb) {
  SymList* sel0_list;
  bool near, far;
  int32_t band;
  uint32_t result;
  int32_t near_hit, far_hit, ctx0, ctx1, fallback, ctx2, pos;
  SymEntry* list_prev, * list_sym;
  pos = this->sym_pos;
  result = (nb)[pos];
  if( exclusion_mask[result]==exclusion_gen )
    return -1;
  near = false;
  for( band = 11; band<16; ++band )
    near |= result==nb[band];
  far = false;
  for( band = 16; band<32; ++band )
    far |= result==nb[band];
  near_hit = 32*near+((result==nb[10])<<6);
  far_hit = 16*far;
  ctx0 = far_hit+near_hit;
  this->ctr_node = ctx0;
  if( (far_hit+near_hit==0)&&pos>6 )
    return -1;
  sel0_list = this->sel0_list;
  ctx1 = ctx0+8*sym_in_top((sel0_list[mode_symbol[1]].ent), 10, result);
  this->ctr_node = ctx1;
  list_prev = sel0_list[mode_symbol[2]].ent;
  list_sym = sel0_list[result].ent;
  fallback = sym_in_top((sel0_list[mode_symbol[3]].ent), 6, result)+2*sym_in_top(list_sym, 10, mode_symbol[2])+4*sym_in_top(list_prev, 4, result);
  ctx2 = fallback+ctx1;
  if( pos<=14||(ctx2&0xB)!=0 ) {
    this->ctr_node = (pos<<7)+ctx2;
    this->ctr_fallback = fallback+8*(pos>9);
  } else {
    this->ctr_fallback = fallback;
    return -1;
  }
  return result;
}

inline int32_t ModelBlock::init_tables() {
  SymEntry* slot, * ent;
  PixRec* up1, * up2;
  uint8_t cnt;
  SymList** cur;
  bool promoted;
  uint16_t* sym_cache;
  SymList* list, ** prev;
  uint32_t blocks, n_live, hit0;
  uint8_t* buf;
  uint16_t want, sym;
  PixRec* row;
  int32_t recycled, hit1, just, c0, c1, c2, c3, c4, c5, c6, result;
  hit0 = this->hit;
  if( !hit0 ) {
    if( this->sel==this->sel_cur ) {
      if( this->sel[0] ) {
        this->sel0_list[mode_symbol[1]].add_weight(this->row_cur[5]->sym, 3u);
        this->sel0_list[this->row_cur[5]->sym].add_weight(mode_symbol[2], 2u);
        this->sel1_list[mode_symbol[1]].add_weight(this->row_cur[5]->sym, 4u);
        this->sel1_list[this->row_cur[5]->sym].add_weight(mode_symbol[1], 2u);
      } else {
        this->sel0_list[mode_symbol[2]].add_weight(this->row_cur[5]->sym, (this->sym_pos>3)+2);
      }
    } else {
      this->sel0_list[mode_symbol[1]].add_weight(this->row_cur[5]->sym, 3u);
      this->sel0_list[this->row_cur[5]->sym].add_weight(mode_symbol[2], 2u);
      this->sel0_list[this->row_cur[5]->sym].add_weight(mode_symbol[1], 1u);
      this->sel1_list[this->row_cur[5]->sym].add_weight(mode_symbol[1], 2u);
      cur = this->sel_cur;
      do {
        prev = cur-1;
        this->sel_cur = prev;
        list = *prev;
        want = this->row_cur[5]->sym;
        n_live = list->live;
        ent = list->ent;
        if( n_live==list->n ) {
          list->live = --n_live;
          recycled = list->ent[n_live].cnt;
        } else {
          recycled = 1;
        }
        slot = &ent[n_live];
        list->live = n_live+1;
        list->tot = recycled+list->tot+1;
        slot->set(want, 2);
        list->since_rescale += 4;
        if( slot!=list->ent ) {
          sym = slot->sym;
          cnt = slot->cnt;
          *slot = slot[-1];
          slot[-1].set(sym, cnt);
        }
        cur = this->sel_cur;
      } while( cur!=this->sel );
    }
    if( exclusion_gen==-1 ) {
      exclusion_gen = 1;
      buf = (uint8_t*)exclusion_mask;
      blocks = (this->alphabet+15)>>4;
      do {
        bmf_zero16(buf);
        buf += 16;
        --blocks;
      } while( blocks );
      hit1 = this->hit;
    } else {
      ++exclusion_gen;
      hit1 = this->hit;
    }
    promoted = hit1&&hit1<=2;
  } else {
    promoted = hit0<=2;
    if( !promoted&&mode_symbol[3]!=mode_symbol[4] ) {
      this->sel0_list[mode_symbol[2]].add_weight(this->row_cur[5]->sym, 1u);
      hit1 = this->hit;
      promoted = hit1&&hit1<=2;
    }
  }
  if( !promoted ) {
    just = this->row_cur[5]->sym;
    sym_cache = this->sym_cache;
    c0 = sym_cache[0];
    if( just!=c0 ) {
      c1 = sym_cache[1];
      if( just==c1 ) {
        sym_cache[1] = c0;
      } else {
        c2 = sym_cache[2];
        if( just==c2 ) {
          sym_cache[2] = c1;
          this->sym_cache[1] = this->sym_cache[0];
        } else {
          c3 = sym_cache[3];
          if( just==c3 ) {
            sym_cache[3] = c2;
            this->sym_cache[2] = this->sym_cache[1];
            this->sym_cache[1] = this->sym_cache[0];
          } else {
            c4 = sym_cache[4];
            if( just==c4 ) {
              sym_cache[4] = c3;
              this->sym_cache[3] = this->sym_cache[2];
              this->sym_cache[2] = this->sym_cache[1];
              this->sym_cache[1] = this->sym_cache[0];
            } else {
              c5 = sym_cache[5];
              if( just==c5 ) {
                sym_cache[5] = c4;
                this->sym_cache[4] = this->sym_cache[3];
                this->sym_cache[3] = this->sym_cache[2];
                this->sym_cache[2] = this->sym_cache[1];
                this->sym_cache[1] = this->sym_cache[0];
              } else {
                c6 = sym_cache[6];
                if( just==c6 ) {
                  sym_cache[6] = c5;
                } else {
                  sym_cache[7] = c6;
                  this->sym_cache[6] = this->sym_cache[5];
                }
                this->sym_cache[5] = this->sym_cache[4];
                this->sym_cache[4] = this->sym_cache[3];
                this->sym_cache[3] = this->sym_cache[2];
                this->sym_cache[2] = this->sym_cache[1];
                this->sym_cache[1] = this->sym_cache[0];
              }
            }
          }
        }
      }
      this->sym_cache[0] = just;
    }
  }
  this->pix_cur[1] = this->pix_cur[0];
  this->pix_cur[0] = this->row_cur[5]->sym;
  {
    PixRec*const here = this->row_cur[5];
    PixRec*const up = this->row_cur[6];
    here->match[0] = here->sym==up->sym;
    here->match[1] = here->sym==here[-1].sym;
    here->match[2] = here->sym==up[1].sym;
    here->match[3] = here->sym==up[-1].sym;
    here->match[4] = here->sym==up[2].sym;
    here->match[5] = here->sym==up[3].sym;
  }
  up1 = this->row_cur[6];
  up2 = this->row_cur[7];
  row = this->row_cur[5]+1;
  this->row_cur[5] = row;
  ++this->row_cur[8];
  ++up1;
  this->row_cur[6] = up1;
  ++up2;
  ++this->row_cur[9];
  this->row_cur[7] = up2;
  this->grad[0] += up1[4].match[0]-up1[-4].match[0];
  this->grad[1] += up2[4].match[0]-up2[-4].match[0];
  this->grad[2] += row[-1].match[1]-row[-5].match[1];
  result = row[-1].match[0]-row[-8].match[0];
  this->grad[3] += result;
  return result;
}

inline AltP2Block*AltP2Block::alt_p2_free(int8_t do_free) {
  for( int32_t k = 0; k<5; ++k )
    free(this->buf[k]);
  free(this->row0);
  free(this->row1);
  if( (do_free&1)!=0 )
    bmf_page_free(this);
  return this;
}

inline AltP1Block*AltP1Block::alt_p1_free(int8_t do_free) {
  for( int32_t k = 0; k<5; ++k )
    free(this->buf[k]);
  if( (do_free&1)!=0 )
    free(this);
  return this;
}

inline int32_t AltP1Block::ctx_of(AltP1Block* nb0, AltP1Block* nb1) {
  P1Ctx* nb0_row, * nb0_row2, * cursor1, * cursor2;
  P1Ctx* cur, * cursor4;
  uint32_t nb0_b;
  bool pick, is_zero, is_neg;
  int32_t west, northwest, act, nb0_a, act_q, s1, hi, lo, s3, s5, g6, h6, g7, h7, s8, result, step, act_all, s2, s4, s6, s7;
  uint32_t quiet, s0;
  int32_t guess, plane_a, plane_b, level_of;
  cursor1 = this->cursor[1];
  guess = cursor1->sym;
  cur = this->cursor[0];
  west = cur[-1].sym;
  northwest = cursor1[-1].sym;
  if( west<guess ) {
    if( northwest>=west ) {
      plane_b = guess+west-northwest;
      pick = northwest<guess;
      guess = cur[-1].sym;
      if( pick )
        guess = plane_b;
    }
  } else if( northwest<=west ) {
    plane_a = guess+west-northwest;
    pick = northwest<=guess;
    guess = cur[-1].sym;
    if( !pick )
      guess = plane_a;
  }
  this->pred = guess;
  cursor2 = this->cursor[2];
  act = cursor1[-1].mag+cur[-3].mag+3*(cursor1[1].mag+cursor2->mag)+6*cur[-1].mag+4*(cursor1->mag+cur[-2].mag)+2*(cursor2[2].mag+cursor1[2].mag+cur[-4].mag);
  if( nb0 ) {
    if( nb1 ) {
      nb0_row = nb0->cursor[0];
      act_all = act+2*(nb1->cursor[0][-1].mag+nb0_row[-1].mag);
      this->ctx_w[5].sel = (cursor1->sym-(uint32_t)guess+nb0_row[-1].sym-nb0->cursor[1][-1].sym);
      nb0_row2 = nb0->cursor[0];
      nb0_a = nb0_row2[-1].sym;
      nb0_b = nb0_row2[-2].sym;
      this->ctx_w[6].sel = (cur[-1].sym-(uint32_t)guess+nb0_a-nb0_b);
      this->ctx_w[7].sel = (cur[-1].sym-(uint32_t)guess+nb1->cursor[0][-1].sym-nb1->cursor[0][-2].sym);
      this->ctx_w[8].sel = (nb1->cursor[0][-1].sym-nb1->pred);
      this->ctx_w[3].sel = (nb0->cursor[0][-1].sym-nb0->pred);
      quiet = (nb1->cursor[0][-1].mag+(uint32_t)nb0->cursor[0][-1].mag-16)>>31;
    } else {
      act_all = cursor1[3].mag+act+3*nb0->cursor[0][-1].mag;
      this->ctx_w[5].sel = (2*cur[-1].sym-cur[-2].sym-(uint32_t)guess);
      this->ctx_w[6].sel = (2*cur[-1].sym-cur[-2].sym-(uint32_t)guess);
      this->ctx_w[7].sel = (-guess-cursor1->sym+cursor1[1].sym+cur[-1].sym);
      this->ctx_w[8].sel = (cur[-1].sym-(uint32_t)guess+nb0->cursor[0][-1].sym-nb0->cursor[0][-2].sym);
      this->ctx_w[3].sel = (nb0->cursor[0][-1].sym-nb0->pred);
      quiet = ((uint32_t)nb0->cursor[0][-1].mag-8)>>31;
    }
  } else {
    cursor4 = this->cursor[4];
    act_all = cursor4->mag+cursor2[-2].mag+cursor1[3].mag+act+cursor4[2].mag;
    this->ctx_w[5].sel = (2*cur[-1].sym-cur[-2].sym-(uint32_t)guess);
    this->ctx_w[6].sel = (2*cursor1->sym-cursor2->sym-(uint32_t)guess);
    this->ctx_w[7].sel = (-guess-cursor1->sym+cursor1[1].sym+cur[-1].sym);
    this->ctx_w[8].sel = (-3*(cur[-2].sym-cur[-1].sym)+cur[-3].sym-(uint32_t)guess);
    this->ctx_w[3].sel = (cursor1[2].sym-(uint32_t)guess);
    quiet = cur->mag+cursor4->mag+this->cursor[3]->mag+cursor2->mag+cursor1->mag==0;
  }
  act_q = (act_all+7)>>4;
  level_of = this->level_of[act_q];
  step = p1_level_step[(uint32_t)level_of];
  this->ctx[0] = level_of;
  this->ctx[1] = this->group_of[act_q]+this->slot_of[(uint32_t)guess];
  this->ctx_w[0].sel = (((216-(uint32_t)guess)>>31)+((22-(uint32_t)guess)>>31));
  s0 = ((216-(uint32_t)guess)>>31)+((22-(uint32_t)guess)>>31);
  s1 = (cursor1[-1].sym-cursor1->sym>=0)+(cursor1[-1].sym>(int32_t)cursor1->sym);
  this->ctx_w[1].sel = s1;
  s2 = (cursor1[-1].sym-cur[-1].sym>=0)+(cursor1[-1].sym>(int32_t)cur[-1].sym);
  this->ctx_w[2].sel = s2;
  is_zero = this->ctx_w[3].sel==0;
  is_neg = this->ctx_w[3].sel<0;
  s4 = (cursor1[1].sym-guess>=-step)+(cursor1[1].sym-guess>step);
  this->ctx_w[4].sel = s4;
  hi = step<this->ctx_w[5].sel;
  lo = -step<=this->ctx_w[5].sel;
  s3 = !is_neg+(!is_neg&&!is_zero);
  this->ctx_w[3].sel = s3;
  s5 = lo+hi;
  is_zero = this->ctx_w[6].sel==0;
  is_neg = this->ctx_w[6].sel<0;
  this->ctx_w[5].sel = s5;
  g6 = !is_neg&&!is_zero;
  h6 = !is_neg;
  is_zero = this->ctx_w[7].sel==0;
  is_neg = this->ctx_w[7].sel<0;
  s6 = h6+g6;
  this->ctx_w[6].sel = (h6+g6);
  g7 = !is_neg&&!is_zero;
  h7 = !is_neg;
  is_zero = this->ctx_w[8].sel==0;
  is_neg = this->ctx_w[8].sel<0;
  s7 = h7+g7;
  this->ctx_w[7].sel = (h7+g7);
  s8 = !is_neg+(!is_neg&&!is_zero);
  this->ctx_w[8].sel = s8;
  result = this->ctx_w[0].w[s0]+this->ctx_w[1].w[s1]+this->ctx_w[2].w[s2]+this->ctx_w[3].w[s3]+this->ctx_w[4].w[s4]+this->ctx_w[5].w[s5]+this->ctx_w[6].w[s6]+this->ctx_w[7].w[s7]+this->ctx_w[8].w[s8]+16*quiet+8*(this->ctx[3+this->ctx[2]]==0)+level_of;
  this->ctx[0] = result;
  return result;
}

int32_t __update_binary_pair(uint16_t* _this, int32_t symbol) {
  FreqPair* tbl, * pair;
  int32_t tot, mask, lvl, node, span, path, go;
  uint32_t step, f, f1_old;
  tot = *_this;
  if( (uint32_t)tot<=0x8000 ) {
    lvl = model_geometry[symbol];
    step = (_this[1]>>2)&0xFFFFFFE0;
    if( ::plane_predictor==2 )
      step = 15*(_this[1]>>5);
    *(_this+lvl+2) += step+4;
    tot = *_this+step+4;
    *_this = tot;
    if( symbol>=2 ) {
      mask = level_geom[lvl].half;
      path = symbol-level_geom[lvl].first;
      node = 0;
      tbl = bit_tree(_this, lvl);
      span = 1;
      do {
        pair = tbl+node+span;
        go = (mask&path)!=0;
        f = pair->f[go];
        if( f>0x2000 ) {
          f1_old = pair->f[1];
          pair->f[0] -= pair->f[0]>>1;
          pair->f[1] = f1_old-(f1_old>>1);
          f = pair->f[go];
        }
        span *= 2;
        mask >>= 1;
        node = go+2*node;
        pair->f[go] = f+((alt_freq_init*((uint32_t)(::plane_predictor==2)+5))>>3);
      } while( mask );
      return 0;
    }
  }
  return tot;
}

inline int32_t AltP1Block::update_model() {
  int32_t result;
  CounterNode* node;
  CounterNode* node_alt, * node_up, * node_dn;
  int32_t alti8;
  uint32_t w11, w12, w13, w14, w15, w16, w17, w18, alti1, ctx_alt, alti0, alti2, alti3, alti4, alti5, alti6, alti7, x2, x3, x4, x5, x6, x7;
  CounterNode* opp0, * opp1, * opp2, * opp3, * opp4, * opp5, * opp6, * opp7, * opp8;
  int16_t tot_up, tot_dn;
  int32_t sel1_top, code_r, sel2_top, slot_f, slot_r, ctx_dn, sel0, ctx0, sel1, ctx1, sel2, ctx2, sel3, ctx3, sel4, ctx4, sel5, ctx5, sel6, ctx6, sel7, ctx7, sel8, midn8, ctx8, mid7, mid6, mid5, mid4, mid3, mid2, mid1, mid0, tree_sym;
  CounterNode* lo0, * hi0, * midn0, * altn0, * lo1, * hi1, * midn1, * altn1, * lo2, * hi2, * midn2, * altn2, * lo3, * hi3, * midn3, * altn3, * lo4, * hi4, * midn4, * altn4, * lo5, * hi5, * midn5, * altn5, * lo6, * hi6, * midn6, * altn6, * lo7, * hi7, * midn7, * altn7, * altn8, * x8;
  uint32_t key;
  uint32_t code_f, ctx_up;
  code_f = *((uint8_t)(this->cursor[0]->sym-(uint8_t)this->pred)+this->fold);
  sel1_top = this->ctx_w[1].sel;
  code_r = *((uint8_t)((uint8_t)this->pred-this->cursor[0]->sym)+this->fold);
  sel2_top = this->ctx_w[2].sel;
  tree_sym = (int32_t)(code_f-5)>>1;
  slot_f = 6-(code_f&1);
  if( code_f<5 )
    slot_f = *((uint8_t)(this->cursor[0]->sym-(uint8_t)this->pred)+this->fold);
  slot_r = 6-(code_r&1);
  if( code_r<5 )
    slot_r = code_r;
  ctx_alt = this->ctx_w[8].w[2-this->ctx_w[8].sel]+this->ctx_w[7].w[2-this->ctx_w[7].sel]+this->ctx_w[6].w[2-this->ctx_w[6].sel]+this->ctx_w[5].w[2-this->ctx_w[5].sel]+this->ctx_w[4].w[2-this->ctx_w[4].sel]+this->ctx_w[3].w[2-this->ctx_w[3].sel]+this->ctx_w[2].w[2-sel2_top]+this->ctx_w[1].w[2-sel1_top]+this->ctx_w[0].w[1]+(this->ctx[0]&0x1F);
  node_alt = &this->counters[ctx_alt];
  node_alt[0].c[slot_r] += 17;
  node_alt[0].total += 17;
  result = this->ctx[0];
  if( (result&7)!=7 ) {
    node_up = &this->counters[result];
    ctx_up = (((this->ctx[1]&7u)-7)>>31)+this->ctx[1];
    node_up[1].c[slot_f] += 11;
    tot_up = node_up[1].total+11;
    node_up[1].total = tot_up;
    if( slot_f>=5 )
      __update_binary_pair(model_strip(128*(slot_f&1)+ctx_up+(((((tot_up&0x7FFF)+node_up[1].c[0]-2*(uint32_t)node_up[1].c[slot_f])>>25)&0xFFFFFFC0))), (int32_t)(code_f-5)>>1);
    result = this->ctx[0];
  }
  if( (result&7)!=0 ) {
    node_dn = &this->counters[result];
    ctx_dn = this->ctx[1]-((this->ctx[1]&7)!=0);
    node_dn[-1].c[slot_f] += 13;
    tot_dn = node_dn[-1].total+13;
    node_dn[-1].total = tot_dn;
    if( slot_f>=5 )
      __update_binary_pair(model_strip(128*(uint32_t)(slot_f&1)+(uint32_t)ctx_dn+(((((tot_dn&0x7FFF)+node_dn[-1].c[0]-2*(uint32_t)node_dn[-1].c[slot_f])>>25)&0xFFFFFFC0))), tree_sym);
    result = this->ctx[0];
  }
  if( this->counters[result].total<0xCCCu ) {
    if( (result&7u)<7 ) {
      node_alt[1].c[slot_r] += 7;
      node_alt[1].total += 7;
      result = this->ctx[0];
    }
    if( (result&7)!=0 ) {
      node_alt[-1].c[slot_r] += 5;
      node_alt[-1].total += 5;
      result = this->ctx[0];
    }
    if( slot_f>=5 ) {
      key = this->ctx[1]+(((this->counters[result].c[0]+(this->counters[result].total&0x7FFF)-2*(uint32_t)this->counters[result].c[slot_f])>>25)&0xFFFFFFC0)+((slot_f&1)<<7);
      if( (key&0x38)>=0x38||(__update_binary_pair(model_strip(128*(slot_f&1)+this->ctx[1]+((((this->counters[result].c[0]+(this->counters[result].total&0x7FFF)-2*(uint32_t)this->counters[result].c[slot_f])>>25)&0xFFFFFFC0))+8), tree_sym), (key&0x38)!=0) ) {
        __update_binary_pair(model_strip(key-8), tree_sym);
      }
      result = this->ctx[0];
    }
    sel0 = this->ctx_w[0].sel;
    if( sel0==1 ) {
      mid0 = result-this->ctx_w[0].w[1];
      lo0 = &this->counters[mid0+this->ctx_w[0].w[0]];
      hi0 = &this->counters[this->ctx_w[0].w[2]+mid0];
      lo0[0].c[slot_f] += 6;
      lo0[0].total += 6;
      hi0[0].c[slot_f] += 6;
      hi0[0].total += 6;
      ctx0 = this->ctx[0];
      if( (ctx0&7)!=7 ) {
        lo0[1].c[slot_f] += 4;
        lo0[1].total += 4;
        hi0[1].c[slot_f] += 4;
        hi0[1].total += 4;
        ctx0 = this->ctx[0];
      }
      if( (ctx0&7)!=0 ) {
        lo0[-1].c[slot_f] += 3;
        lo0[-1].total += 3;
        hi0[-1].c[slot_f] += 3;
        hi0[-1].total += 3;
        ctx0 = this->ctx[0];
      }
    } else {
      opp0 = &this->counters[this->ctx_w[0].w[2-sel0]+result-this->ctx_w[0].w[sel0]];
      opp0[0].c[slot_f] += 7;
      opp0[0].total += 7;
      alti0 = ctx_alt+this->ctx_w[0].w[0]-this->ctx_w[0].w[1];
      midn0 = &this->counters[this->ctx_w[0].w[1]+this->ctx[0]-this->ctx_w[0].w[this->ctx_w[0].sel]];
      midn0[0].c[slot_f] += 6;
      midn0[0].total += 6;
      altn0 = &this->counters[alti0];
      altn0[0].c[slot_r] += 4;
      altn0[0].total += 4;
      ctx0 = this->ctx[0];
      if( (ctx0&7)!=7 ) {
        midn0[1].c[slot_f] += 4;
        midn0[1].total += 4;
        altn0[1].c[slot_r] += 2;
        altn0[1].total += 2;
        ctx0 = this->ctx[0];
      }
      if( (ctx0&7)!=0 ) {
        midn0[-1].c[slot_f] += 3;
        midn0[-1].total += 3;
        altn0[-1].c[slot_r] += 2;
        altn0[-1].total += 2;
        ctx0 = this->ctx[0];
      }
    }
    sel1 = this->ctx_w[1].sel;
    if( sel1==1 ) {
      mid1 = ctx0-this->ctx_w[1].w[1];
      lo1 = &this->counters[mid1+this->ctx_w[1].w[0]];
      hi1 = &this->counters[this->ctx_w[1].w[2]+mid1];
      lo1[0].c[slot_f] += 6;
      lo1[0].total += 6;
      hi1[0].c[slot_f] += 6;
      hi1[0].total += 6;
      ctx1 = this->ctx[0];
      if( (ctx1&7)!=7 ) {
        lo1[1].c[slot_f] += 4;
        lo1[1].total += 4;
        hi1[1].c[slot_f] += 4;
        hi1[1].total += 4;
        ctx1 = this->ctx[0];
      }
      if( (ctx1&7)!=0 ) {
        lo1[-1].c[slot_f] += 3;
        lo1[-1].total += 3;
        hi1[-1].c[slot_f] += 3;
        hi1[-1].total += 3;
        ctx1 = this->ctx[0];
      }
    } else {
      opp1 = &this->counters[this->ctx_w[1].w[2-sel1]+ctx0-this->ctx_w[1].w[sel1]];
      opp1[0].c[slot_f] += 7;
      opp1[0].total += 7;
      w11 = this->ctx_w[1].w[1];
      alti1 = ctx_alt+w11-this->ctx_w[1].w[2-this->ctx_w[1].sel];
      midn1 = &this->counters[(w11+this->ctx[0]-this->ctx_w[1].w[this->ctx_w[1].sel])];
      midn1[0].c[slot_f] += 6;
      midn1[0].total += 6;
      altn1 = &this->counters[alti1];
      altn1[0].c[slot_r] += 4;
      altn1[0].total += 4;
      ctx1 = this->ctx[0];
      if( (ctx1&7)!=7 ) {
        midn1[1].c[slot_f] += 4;
        midn1[1].total += 4;
        altn1[1].c[slot_r] += 2;
        altn1[1].total += 2;
        ctx1 = this->ctx[0];
      }
      if( (ctx1&7)!=0 ) {
        midn1[-1].c[slot_f] += 3;
        midn1[-1].total += 3;
        altn1[-1].c[slot_r] += 2;
        altn1[-1].total += 2;
        ctx1 = this->ctx[0];
      }
    }
    sel2 = this->ctx_w[2].sel;
    if( sel2==1 ) {
      mid2 = ctx1-this->ctx_w[2].w[1];
      lo2 = &this->counters[mid2+this->ctx_w[2].w[0]];
      hi2 = &this->counters[this->ctx_w[2].w[2]+mid2];
      lo2[0].c[slot_f] += 6;
      lo2[0].total += 6;
      hi2[0].c[slot_f] += 6;
      hi2[0].total += 6;
      ctx2 = this->ctx[0];
      if( (ctx2&7)!=7 ) {
        lo2[1].c[slot_f] += 4;
        lo2[1].total += 4;
        hi2[1].c[slot_f] += 4;
        hi2[1].total += 4;
        ctx2 = this->ctx[0];
      }
      if( (ctx2&7)!=0 ) {
        lo2[-1].c[slot_f] += 3;
        lo2[-1].total += 3;
        hi2[-1].c[slot_f] += 3;
        hi2[-1].total += 3;
        ctx2 = this->ctx[0];
      }
    } else {
      opp2 = &this->counters[this->ctx_w[2].w[2-sel2]+ctx1-this->ctx_w[2].w[sel2]];
      opp2[0].c[slot_f] += 7;
      opp2[0].total += 7;
      w12 = this->ctx_w[2].w[1];
      x2 = w12+this->ctx[0]-this->ctx_w[2].w[this->ctx_w[2].sel];
      alti2 = ctx_alt+w12-this->ctx_w[2].w[2-this->ctx_w[2].sel];
      midn2 = &this->counters[x2];
      midn2[0].c[slot_f] += 6;
      midn2[0].total += 6;
      altn2 = &this->counters[alti2];
      altn2[0].c[slot_r] += 4;
      altn2[0].total += 4;
      ctx2 = this->ctx[0];
      if( (ctx2&7)!=7 ) {
        midn2[1].c[slot_f] += 4;
        midn2[1].total += 4;
        altn2[1].c[slot_r] += 2;
        altn2[1].total += 2;
        ctx2 = this->ctx[0];
      }
      if( (ctx2&7)!=0 ) {
        midn2[-1].c[slot_f] += 3;
        midn2[-1].total += 3;
        altn2[-1].c[slot_r] += 2;
        altn2[-1].total += 2;
        ctx2 = this->ctx[0];
      }
    }
    sel3 = this->ctx_w[3].sel;
    if( sel3==1 ) {
      mid3 = ctx2-this->ctx_w[3].w[1];
      lo3 = &this->counters[mid3+this->ctx_w[3].w[0]];
      hi3 = &this->counters[this->ctx_w[3].w[2]+mid3];
      lo3[0].c[slot_f] += 6;
      lo3[0].total += 6;
      hi3[0].c[slot_f] += 6;
      hi3[0].total += 6;
      ctx3 = this->ctx[0];
      if( (ctx3&7)!=7 ) {
        lo3[1].c[slot_f] += 4;
        lo3[1].total += 4;
        hi3[1].c[slot_f] += 4;
        hi3[1].total += 4;
        ctx3 = this->ctx[0];
      }
      if( (ctx3&7)!=0 ) {
        lo3[-1].c[slot_f] += 3;
        lo3[-1].total += 3;
        hi3[-1].c[slot_f] += 3;
        hi3[-1].total += 3;
        ctx3 = this->ctx[0];
      }
    } else {
      opp3 = &this->counters[this->ctx_w[3].w[2-sel3]+ctx2-this->ctx_w[3].w[sel3]];
      opp3[0].c[slot_f] += 7;
      opp3[0].total += 7;
      w13 = this->ctx_w[3].w[1];
      x3 = w13+this->ctx[0]-this->ctx_w[3].w[this->ctx_w[3].sel];
      alti3 = ctx_alt+w13-this->ctx_w[3].w[2-this->ctx_w[3].sel];
      midn3 = &this->counters[x3];
      midn3[0].c[slot_f] += 6;
      midn3[0].total += 6;
      altn3 = &this->counters[alti3];
      altn3[0].c[slot_r] += 4;
      altn3[0].total += 4;
      ctx3 = this->ctx[0];
      if( (ctx3&7)!=7 ) {
        midn3[1].c[slot_f] += 4;
        midn3[1].total += 4;
        altn3[1].c[slot_r] += 2;
        altn3[1].total += 2;
        ctx3 = this->ctx[0];
      }
      if( (ctx3&7)!=0 ) {
        midn3[-1].c[slot_f] += 3;
        midn3[-1].total += 3;
        altn3[-1].c[slot_r] += 2;
        altn3[-1].total += 2;
        ctx3 = this->ctx[0];
      }
    }
    sel4 = this->ctx_w[4].sel;
    if( sel4==1 ) {
      mid4 = ctx3-this->ctx_w[4].w[1];
      lo4 = &this->counters[mid4+this->ctx_w[4].w[0]];
      hi4 = &this->counters[this->ctx_w[4].w[2]+mid4];
      lo4[0].c[slot_f] += 6;
      lo4[0].total += 6;
      hi4[0].c[slot_f] += 6;
      hi4[0].total += 6;
      ctx4 = this->ctx[0];
      if( (ctx4&7)!=7 ) {
        lo4[1].c[slot_f] += 4;
        lo4[1].total += 4;
        hi4[1].c[slot_f] += 4;
        hi4[1].total += 4;
        ctx4 = this->ctx[0];
      }
      if( (ctx4&7)!=0 ) {
        lo4[-1].c[slot_f] += 3;
        lo4[-1].total += 3;
        hi4[-1].c[slot_f] += 3;
        hi4[-1].total += 3;
        ctx4 = this->ctx[0];
      }
    } else {
      opp4 = &this->counters[this->ctx_w[4].w[2-sel4]+ctx3-this->ctx_w[4].w[sel4]];
      opp4[0].c[slot_f] += 7;
      opp4[0].total += 7;
      w14 = this->ctx_w[4].w[1];
      x4 = w14+this->ctx[0]-this->ctx_w[4].w[this->ctx_w[4].sel];
      alti4 = ctx_alt+w14-this->ctx_w[4].w[2-this->ctx_w[4].sel];
      midn4 = &this->counters[x4];
      midn4[0].c[slot_f] += 6;
      midn4[0].total += 6;
      altn4 = &this->counters[alti4];
      altn4[0].c[slot_r] += 4;
      altn4[0].total += 4;
      ctx4 = this->ctx[0];
      if( (ctx4&7)!=7 ) {
        midn4[1].c[slot_f] += 4;
        midn4[1].total += 4;
        altn4[1].c[slot_r] += 2;
        altn4[1].total += 2;
        ctx4 = this->ctx[0];
      }
      if( (ctx4&7)!=0 ) {
        midn4[-1].c[slot_f] += 3;
        midn4[-1].total += 3;
        altn4[-1].c[slot_r] += 2;
        altn4[-1].total += 2;
        ctx4 = this->ctx[0];
      }
    }
    sel5 = this->ctx_w[5].sel;
    if( sel5==1 ) {
      mid5 = ctx4-this->ctx_w[5].w[1];
      lo5 = &this->counters[mid5+this->ctx_w[5].w[0]];
      hi5 = &this->counters[this->ctx_w[5].w[2]+mid5];
      lo5[0].c[slot_f] += 6;
      lo5[0].total += 6;
      hi5[0].c[slot_f] += 6;
      hi5[0].total += 6;
      ctx5 = this->ctx[0];
      if( (ctx5&7)!=7 ) {
        lo5[1].c[slot_f] += 4;
        lo5[1].total += 4;
        hi5[1].c[slot_f] += 4;
        hi5[1].total += 4;
        ctx5 = this->ctx[0];
      }
      if( (ctx5&7)!=0 ) {
        lo5[-1].c[slot_f] += 3;
        lo5[-1].total += 3;
        hi5[-1].c[slot_f] += 3;
        hi5[-1].total += 3;
        ctx5 = this->ctx[0];
      }
    } else {
      opp5 = &this->counters[this->ctx_w[5].w[2-sel5]+ctx4-this->ctx_w[5].w[sel5]];
      opp5[0].c[slot_f] += 7;
      opp5[0].total += 7;
      w15 = this->ctx_w[5].w[1];
      x5 = w15+this->ctx[0]-this->ctx_w[5].w[this->ctx_w[5].sel];
      alti5 = ctx_alt+w15-this->ctx_w[5].w[2-this->ctx_w[5].sel];
      midn5 = &this->counters[x5];
      midn5[0].c[slot_f] += 6;
      midn5[0].total += 6;
      altn5 = &this->counters[alti5];
      altn5[0].c[slot_r] += 4;
      altn5[0].total += 4;
      ctx5 = this->ctx[0];
      if( (ctx5&7)!=7 ) {
        midn5[1].c[slot_f] += 4;
        midn5[1].total += 4;
        altn5[1].c[slot_r] += 2;
        altn5[1].total += 2;
        ctx5 = this->ctx[0];
      }
      if( (ctx5&7)!=0 ) {
        midn5[-1].c[slot_f] += 3;
        midn5[-1].total += 3;
        altn5[-1].c[slot_r] += 2;
        altn5[-1].total += 2;
        ctx5 = this->ctx[0];
      }
    }
    sel6 = this->ctx_w[6].sel;
    if( sel6==1 ) {
      mid6 = ctx5-this->ctx_w[6].w[1];
      lo6 = &this->counters[mid6+this->ctx_w[6].w[0]];
      hi6 = &this->counters[this->ctx_w[6].w[2]+mid6];
      lo6[0].c[slot_f] += 6;
      lo6[0].total += 6;
      hi6[0].c[slot_f] += 6;
      hi6[0].total += 6;
      ctx6 = this->ctx[0];
      if( (ctx6&7)!=7 ) {
        lo6[1].c[slot_f] += 4;
        lo6[1].total += 4;
        hi6[1].c[slot_f] += 4;
        hi6[1].total += 4;
        ctx6 = this->ctx[0];
      }
      if( (ctx6&7)!=0 ) {
        lo6[-1].c[slot_f] += 3;
        lo6[-1].total += 3;
        hi6[-1].c[slot_f] += 3;
        hi6[-1].total += 3;
        ctx6 = this->ctx[0];
      }
    } else {
      opp6 = &this->counters[this->ctx_w[6].w[2-sel6]+ctx5-this->ctx_w[6].w[sel6]];
      opp6[0].c[slot_f] += 7;
      opp6[0].total += 7;
      w16 = this->ctx_w[6].w[1];
      x6 = w16+this->ctx[0]-this->ctx_w[6].w[this->ctx_w[6].sel];
      alti6 = ctx_alt+w16-this->ctx_w[6].w[2-this->ctx_w[6].sel];
      midn6 = &this->counters[x6];
      midn6[0].c[slot_f] += 6;
      midn6[0].total += 6;
      altn6 = &this->counters[alti6];
      altn6[0].c[slot_r] += 4;
      altn6[0].total += 4;
      ctx6 = this->ctx[0];
      if( (ctx6&7)!=7 ) {
        midn6[1].c[slot_f] += 4;
        midn6[1].total += 4;
        altn6[1].c[slot_r] += 2;
        altn6[1].total += 2;
        ctx6 = this->ctx[0];
      }
      if( (ctx6&7)!=0 ) {
        midn6[-1].c[slot_f] += 3;
        midn6[-1].total += 3;
        altn6[-1].c[slot_r] += 2;
        altn6[-1].total += 2;
        ctx6 = this->ctx[0];
      }
    }
    sel7 = this->ctx_w[7].sel;
    if( sel7==1 ) {
      mid7 = ctx6-this->ctx_w[7].w[1];
      lo7 = &this->counters[mid7+this->ctx_w[7].w[0]];
      hi7 = &this->counters[this->ctx_w[7].w[2]+mid7];
      lo7[0].c[slot_f] += 6;
      lo7[0].total += 6;
      hi7[0].c[slot_f] += 6;
      hi7[0].total += 6;
      ctx7 = this->ctx[0];
      if( (ctx7&7)!=7 ) {
        lo7[1].c[slot_f] += 4;
        lo7[1].total += 4;
        hi7[1].c[slot_f] += 4;
        hi7[1].total += 4;
        ctx7 = this->ctx[0];
      }
      if( (ctx7&7)!=0 ) {
        lo7[-1].c[slot_f] += 3;
        lo7[-1].total += 3;
        hi7[-1].c[slot_f] += 3;
        hi7[-1].total += 3;
        ctx7 = this->ctx[0];
      }
    } else {
      opp7 = &this->counters[this->ctx_w[7].w[2-sel7]+ctx6-this->ctx_w[7].w[sel7]];
      opp7[0].c[slot_f] += 7;
      opp7[0].total += 7;
      w17 = this->ctx_w[7].w[1];
      x7 = w17+this->ctx[0]-this->ctx_w[7].w[this->ctx_w[7].sel];
      alti7 = ctx_alt+w17-this->ctx_w[7].w[2-this->ctx_w[7].sel];
      midn7 = &this->counters[x7];
      midn7[0].c[slot_f] += 6;
      midn7[0].total += 6;
      altn7 = &this->counters[alti7];
      altn7[0].c[slot_r] += 4;
      altn7[0].total += 4;
      ctx7 = this->ctx[0];
      if( (ctx7&7)!=7 ) {
        midn7[1].c[slot_f] += 4;
        midn7[1].total += 4;
        altn7[1].c[slot_r] += 2;
        altn7[1].total += 2;
        ctx7 = this->ctx[0];
      }
      if( (ctx7&7)!=0 ) {
        midn7[-1].c[slot_f] += 3;
        midn7[-1].total += 3;
        altn7[-1].c[slot_r] += 2;
        altn7[-1].total += 2;
        ctx7 = this->ctx[0];
      }
    }
    sel8 = this->ctx_w[8].sel;
    if( sel8==1 ) {
      midn8 = ctx7-this->ctx_w[8].w[1];
      altn8 = &this->counters[midn8+this->ctx_w[8].w[0]];
      node = &this->counters[this->ctx_w[8].w[2]+midn8];
      altn8[0].c[slot_f] += 6;
      altn8[0].total += 6;
      node[0].c[slot_f] += 6;
      node[0].total += 6;
      ctx8 = this->ctx[0];
      if( (ctx8&7)!=7 ) {
        altn8[1].c[slot_f] += 4;
        altn8[1].total += 4;
        node[1].c[slot_f] += 4;
        node[1].total += 4;
        ctx8 = this->ctx[0];
      }
      if( (ctx8&7)!=0 ) {
        altn8[-1].c[slot_f] += 3;
        altn8[-1].total += 3;
        node[-1].c[slot_f] += 3;
        node[-1].total += 3;
      }
    } else {
      opp8 = &this->counters[this->ctx_w[8].w[2-sel8]+ctx7-this->ctx_w[8].w[sel8]];
      opp8[0].c[slot_f] += 7;
      opp8[0].total += 7;
      w18 = this->ctx_w[8].w[1];
      node = &this->counters[w18+this->ctx[0]-this->ctx_w[8].w[this->ctx_w[8].sel]];
      x8 = &this->counters[w18-this->ctx_w[8].w[2-this->ctx_w[8].sel]+ctx_alt];
      node[0].c[slot_f] += 6;
      node[0].total += 6;
      x8[0].c[slot_r] += 4;
      x8[0].total += 4;
      alti8 = this->ctx[0];
      if( (alti8&7)!=7 ) {
        node[1].c[slot_f] += 4;
        node[1].total += 4;
        x8[1].c[slot_r] += 2;
        x8[1].total += 2;
        alti8 = this->ctx[0];
      }
      if( (alti8&7)!=0 ) {
        node[-1].c[slot_f] += 3;
        node[-1].total += 3;
        result = (uint16_t)x8[-1].c[slot_r]+2;
        x8[-1].c[slot_r] = result;
        x8[-1].total += 2;
      }
    }
  }
  return result;
}

inline AltP1Block*AltP1Block::alt_p1_alloc(int32_t img_w, int32_t img_h, int32_t plane) {
  int32_t lvl, grp, k, lvl1, odd, grp1, slot, slot1, bump, wid, r, plane_hi;
  uint32_t ctr, pair, row;
  ctr = 0;
  this->width = img_w;
  this->height = img_h;
  do
    __init_counter_node(&this->counters[ctr++]);
  while( ctr<0x99C60 );
  this->pred = 0;
  lvl = 0;
  grp = 0;
  k = 0;
  plane_hi = plane<<8;
  do {
    this->level_of[2*k] = lvl;
    lvl1 = (2*k==p1_level_edges[lvl])+lvl;
    this->group_of[2*k] = plane_hi|grp;
    this->level_of[2*k+1] = lvl1;
    odd = 2*k+1;
    grp1 = (2*k==p1_group_edges[grp])+grp;
    lvl = (odd==p1_level_edges[lvl1])+lvl1;
    this->group_of[2*k+1] = grp1|plane_hi;
    grp = (odd==p1_group_edges[grp1])+grp1;
    ++k;
  } while( (uint32_t)k<0x100 );
  pair = 0;
  slot = 0;
  do {
    this->slot_of[2*pair] = 8*slot;
    slot1 = (2*pair==p1_slot_edges[slot])+slot;
    this->slot_of[2*pair+1] = 8*slot1;
    bump = 2*pair+++1==p1_slot_edges[slot1];
    slot = bump+slot1;
  } while( pair<0x80 );
  this->ctx_w[0].w[1] = 32;
  this->ctx_w[8].w[0] = 0;
  this->ctx_w[7].w[0] = 0;
  this->ctx_w[0].w[2] = 64;
  this->ctx_w[6].w[0] = 0;
  this->ctx_w[5].w[0] = 0;
  this->ctx_w[4].w[0] = 0;
  this->ctx_w[1].w[1] = 96;
  this->ctx_w[3].w[0] = 0;
  this->ctx_w[2].w[0] = 0;
  this->ctx_w[1].w[0] = 0;
  this->ctx_w[1].w[2] = 192;
  this->ctx_w[0].w[0] = 0;
  row = 0;
  this->ctx_w[2].w[1] = 288;
  this->ctx_w[2].w[2] = 576;
  this->ctx_w[3].w[1] = 864;
  this->ctx_w[3].w[2] = 1728;
  this->ctx_w[4].w[1] = 2592;
  this->ctx_w[4].w[2] = 5184;
  this->ctx_w[5].w[1] = 7776;
  this->ctx_w[5].w[2] = 15552;
  this->ctx_w[6].w[1] = 23328;
  this->ctx_w[6].w[2] = 46656;
  this->ctx_w[7].w[1] = 69984;
  this->ctx_w[7].w[2] = 139968;
  this->ctx_w[8].w[1] = 209952;
  this->ctx_w[8].w[2] = 419904;
  do
    this->buf[row++] = (P1Ctx*)bmf_new(sizeof(P1Ctx)*(this->width+10));
  while( row<5 );
  __alt_init_tables(this->fold, (int8_t*)this->unfold);
  wid = this->width;
  if( this->width>-10 ) {
    r = 0;
    do {
      this->buf[4][r].sym = 72;
      this->buf[3][r].sym = 72;
      this->buf[2][r].sym = 72;
      this->buf[1][r].sym = 72;
      this->buf[0][r].sym = 72;
      this->buf[4][r].mag = 0;
      this->buf[3][r].mag = 0;
      this->buf[2][r].mag = 0;
      this->buf[1][r].mag = 0;
      this->buf[0][r].mag = 0;
      wid = this->width;
      ++r;
    } while( r<this->width+10 );
  }
  this->cursor[0] = this->buf[0]+wid+4;
  this->cursor[1] = this->buf[1]+wid+4;
  this->cursor[2] = this->buf[2]+wid+4;
  this->cursor[3] = this->buf[3]+wid+4;
  this->cursor[4] = this->buf[4]+wid+4;
  return this;
}

uint16_t*__rc_begin_encode() {
  uint16_t* tbl, * row;
  FreqPair* seed;
  int32_t at4, at5, k;
  int32_t bits;
  uint8_t* at;
  int32_t at6, at7;
  uint32_t i;
  *packer_word = ::packer_acc;
  at = out_cursor;
  if( out_cursor!=(uint8_t*)packer_word ) {
    bits = packer_free_bits-8;
    if( bits<0 ) {
      packer_free_bits = bits;
    } else {
      do {
        --at;
        bits -= 8;
      } while( bits>=0 );
      out_cursor = at;
      packer_free_bits = bits;
    }
  }
  if( plane_alt_model ) {
    if( plane_predictor==2 ) {
      alt_freq_init = 8;
      alt_freq_limit = 8;
    } else {
      alt_freq_limit = 16;
      alt_freq_init = 64;
    }
    level_geom[2].first = 2;
    model_geometry[0] = 0;
    model_geometry[1] = 1;
    model_geometry[2] = 2;
    model_geometry[3] = 2;
    level_geom[2].tbl_base = 0;
    level_geom[2].half = 1;
    level_geom[3].half = 2;
    level_geom[3].first = 4;
    level_geom[3].tbl_base = 1;
    memset(&model_geometry[4], 3, 4);
    level_geom[4].half = 4;
    at4 = 2*level_geom[3].half+4;
    level_geom[4].first = 2*level_geom[3].half+4;
    level_geom[4].tbl_base = 2*level_geom[3].half;
    memset(&model_geometry[at4], 4, 8);
    level_geom[5].half = 8;
    at5 = at4+2*level_geom[4].half;
    level_geom[5].first = at5;
    level_geom[5].tbl_base = at5-5;
    memset(&model_geometry[at5], 0x05, 16);
    level_geom[6].half = 16;
    at6 = at5+2*level_geom[5].half;
    level_geom[6].first = (uint8_t)at6;
    level_geom[6].tbl_base = (uint8_t)at6-6;
    memset(&model_geometry[at6], 0x06, 32);
    level_geom[7].half = 32;
    at7 = at6+2*level_geom[6].half;
    level_geom[7].first = (uint8_t)at7;
    level_geom[7].tbl_base = (uint8_t)at7-7;
    memset(&model_geometry[at7], 0x07, 64);
    tbl = (uint16_t*)bmf_new(0x7F000u);
    if( tbl ) {
      k = 0;
      row = tbl;
      do {
        row[1] = 24*alt_freq_limit;
        row[2] = 205;
        row[6] = 48;
        row[3] = 124;
        row[7] = 16;
        row[4] = 147;
        row[5] = 83;
        row[8] = 8;
        row[9] = 4;
        *row = 635;
        seed = (FreqPair*)&row[10];
        for( i = 0; i<0x7A; ++i ) {
          seed[i].f[0] = 60;
          seed[i].f[1] = 36;
        }
        row += 254;
        ++k;
      } while( (uint32_t)k<0x400 );
    }
    ::model_tables = tbl;
  }
  rc.enc_init();
  return tbl;
}

inline void AltP1Block::d8_encode_body(uint8_t* src, uint8_t* out) {
  P1Ctx* cursor2, * cursor4, * cursor0, * b4, * buf3, * buf2, * buf1, * b0;
  uint8_t pred, recon;
  int32_t y, resid, code, drift, val, x;
  int64_t err;
  uint8_t* q;
  __rc_begin_encode();
  if( this->height>0 ) {
    y = 0;
    do {
      ++y;
      {
        P1Ctx*const here = (P1Ctx*)this->cursor[0];
        here[0] = here[-1];
        here[1] = here[-2];
        here[2] = here[-3];
        here[3] = here[-4];
        here[4] = here[-5];
        here[5] = here[-6];
      }
      b4 = this->buf[4];
      buf3 = this->buf[3];
      buf2 = this->buf[2];
      buf1 = this->buf[1];
      b0 = this->buf[0];
      this->buf[4] = buf3;
      this->buf[3] = buf2;
      this->buf[2] = buf1;
      this->buf[1] = b0;
      this->buf[0] = b4;
      b4 += 4;
      this->cursor[0] = b4;
      b0 += 4;
      this->cursor[1] = b0;
      this->cursor[2] = buf1+4;
      this->cursor[3] = buf2+4;
      this->cursor[4] = buf3+4;
      ((P1Ctx*)b4)[-4] = ((P1Ctx*)b0)[3];
      {
        P1Ctx*const here = (P1Ctx*)this->cursor[0];
        P1Ctx*const up = (P1Ctx*)this->cursor[1];
        here[-3] = up[2];
        here[-2] = up[1];
        here[-1] = up[0];
      }
      cursor2 = this->cursor[2];
      cursor4 = this->cursor[4];
      this->ctx[2] = 0;
      cursor0 = this->cursor[0];
      this->ctx[3] = cursor2[-2].mag+cursor4[-2].mag+cursor2[0].mag+cursor4[0].mag+cursor2[2].mag+cursor4[2].mag+cursor2[4].mag+cursor4[4].mag+cursor0[-4].mag+cursor0[-2].mag;
      this->ctx[4] = cursor2[-1].mag+cursor4[-1].mag+cursor2[1].mag+cursor4[1].mag+cursor2[3].mag+cursor4[3].mag+cursor2[5].mag+cursor4[5].mag+cursor0[-3].mag+cursor0[-1].mag;
      if( !(this->width<=0) ) {
        q = out;
        x = 0;
        do {
          ++x;
          (this)->ctx_of((AltP1Block*)nullptr, (AltP1Block*)0);
          pred = (uint8_t)this->pred;
          resid = (uint8_t)(*src-pred);
          recon = *(this->fold[resid]+this->unfold)+pred;
          code = this->fold[resid];
          drift = (uint8_t)*q-(uint8_t)(recon+*q-*src);
          if( drift<-16||drift>16 ) {
            *q = *src;
            code = this->fold_hi[resid];
          } else {
            *q = recon;
          }
          __alt_p1_encode_symbol(&this->counters[this->ctx[0]].total, 16*this->ctx[0], this->ctx[1], code);
          val = (uint8_t)*q;
          err = val-this->pred;
          this->cursor[0]->sym = val;
          this->cursor[0]->mag = (BYTE4(err)^err)-BYTE4(err);
          this->ctx[3+this->ctx[2]] = this->ctx[3+this->ctx[2]]+this->cursor[0]->mag-this->cursor[0][-4].mag-(this->cursor[4][-2].mag-this->cursor[4][6].mag+this->cursor[2][-2].mag-this->cursor[2][6].mag);
          this->ctx[2] = this->ctx[2]==0;
          if( this->counters[this->ctx[0]].total<0x4000u )
            this->update_model();
          ++this->cursor[0];
          ++q;
          ++this->cursor[1];
          ++this->cursor[2];
          ++this->cursor[3];
          ++this->cursor[4];
          ++src;
        } while( x<this->width );
        out = q;
      }
    } while( (uint32_t)y<*(uint32_t*)&this->height );
  }
  __rc_end_encode();
}

void __alt_model_p1_d8_encode(uint8_t* src, int32_t i, int32_t height, uint8_t* out) {
  AltP1Block* raw, * blk;
  raw = (AltP1Block*)bmf_new(sizeof(AltP1Block));
  if( raw )
    blk = raw->alt_p1_alloc(i, height, 0);
  else
    blk = nullptr;
  blk->d8_encode_body(src, out);
  if( blk )
    blk->alt_p1_free(1);
}

t_new_handler __set_new_handler(t_new_handler __out_of_memory_handler) {
  t_new_handler __set_new_handler_pout_of_memory_handler;
  __set_new_handler_pout_of_memory_handler = ::__pout_of_memory_handler;
  ::__pout_of_memory_handler = __out_of_memory_handler;
  return __set_new_handler_pout_of_memory_handler;
}

static void bmf_set_denormal_mode() {
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
}

inline AltP2Block*AltP2Block::alt_p2_alloc(int32_t img_w, int32_t plane) {
  uint32_t done, e;
  void* buf1;
  int32_t dz, band, row_bytes, lvl, lvl1, bump, len, len1;
  uint32_t j, k, ctr, pairs, p, row, pair, n;
  this->plane_idx = plane;
  for( j = 0; j<0x14000; ++j ) {
    this->p2_ctr[2*j].weighted = 0;
    this->p2_ctr[2*j+1].weighted = 0;
  }
  for( k = 0; k<0x14000; ++k ) {
    this->p2_ctr[2*k].rate = 5;
    this->p2_ctr[2*k].b1 = 2;
    this->p2_ctr[2*k+1].rate = 5;
    this->p2_ctr[2*k+1].b1 = 2;
  }
  ctr = 0;
  do {
    e = 2*ctr;
    this->freq[e].f[0] = 2048;
    ++ctr;
    this->freq[e].f[1] = 2816;
    this->freq[e].f[2] = 2816;
    this->freq[e].step = 4096;
    this->freq[e+1].f[0] = 2048;
    this->freq[e+1].f[1] = 2816;
    this->freq[e+1].f[2] = 2816;
    this->freq[e+1].step = 4096;
  } while( ctr<0x1E60 );
  dz = 4*near_lossless_q+1;
  band = 16*near_lossless_q;
  this->has_ref = (uint8_t)(plane_desc[plane_desc[this->plane_idx+1].src_plane+1].flags&8)>>3;
  deadzone_hi = dz;
  deadzone_lo = -dz;
  this->band_lo = -band-7;
  this->band_hi = band+8;
  this->row0 = (float (**)[4])bmf_new(sizeof(float (*)[4])*(img_w+4));
  buf1 = bmf_new(sizeof(float (*)[4])*(img_w+4));
  this->nb_weights[0][14][2] = 1.0f;
  this->row1 = (float (**)[4]) buf1;
  this->cur = this->row0+img_w+2;
  if( img_w>-4 ) {
    pairs = (img_w+4)/2;
    if( pairs ) {
      for( p = 0; p<pairs; ++p ) {
        this->row1[2*p] = this->nb_weights[0];
        this->row0[2*p] = this->nb_weights[0];
        this->row1[2*p+1] = this->nb_weights[0];
        this->row0[2*p+1] = this->nb_weights[0];
      }
      done = 2*p+1;
    } else {
      done = 1;
    }
    if( (uint32_t)(img_w+4)>(done-1) ) {
      this->row1[done-1] = this->nb_weights[0];
      this->row0[done-1] = this->nb_weights[0];
    }
  }
  row = 0;
  row_bytes = 18*(img_w<3 ? 3 : img_w)+234;
  do
    this->buf[row++] = (P2Ctx*)bmf_new(row_bytes);
  while( row<5 );
  memset(this->buf[0], 0, row_bytes);
  ctx_bias[3] = 0;
  lvl = 0;
  ctx_bias[2] = 0;
  ctx_bias[1] = 0;
  pair = 0;
  ctx_bias[0] = 0;
  this->cursor[0] = this->buf[0]+8;
  do {
    this->ctx_delta[2*pair] = (this->plane_idx<<8)|(16*lvl);
    lvl1 = (2*pair==p2_ctx_edges[lvl])+lvl;
    this->ctx_delta[2*pair+1] = (this->plane_idx<<8)|(16*lvl1);
    bump = 2*pair+++1==p2_ctx_edges[lvl1];
    lvl = bump+lvl1;
  } while( pair<0x82 );
  len = 0;
  for( n = 0; n<0x3C; ++n ) {
    this->nb_ctx[2*n] = len;
    len1 = (2*n==p2_len_edges[len])+len;
    this->nb_ctx[2*n+1] = len1;
    len = (2*n+1==p2_len_edges[len1])+len1;
  }
  this->ctx = 15;
  __alt_init_tables(this->fold, this->unfold);
  this->ctx_w[4].w[0] = 0;
  this->ctx_w[0].w[1] = 64;
  this->ctx_w[3].w[0] = 0;
  this->ctx_w[0].w[2] = 128;
  this->ctx_w[2].w[0] = 0;
  this->ctx_w[1].w[1] = 192;
  this->ctx_w[1].w[0] = 0;
  this->ctx_w[1].w[2] = 384;
  this->ctx_w[0].w[0] = 0;
  this->ctx_w[2].w[1] = 576;
  this->ctx_w[2].w[2] = 1152;
  this->ctx_w[3].w[1] = 1728;
  this->ctx_w[3].w[2] = 3456;
  this->ctx_w[4].w[1] = 5184;
  this->ctx_w[4].w[2] = 10368;
  return this;
}

struct BmfImage {
  uint16_t width;
  uint16_t height;
  uint32_t stride;
  uint8_t _pad8[2];
  uint8_t depth;
  uint8_t flags;
  uint32_t data_size;
  uint8_t pixels[0];
};
int32_t*__alloc_image(int32_t img_w, int32_t img_h, int32_t bpp, int32_t palette, int32_t packed) {
  uint8_t* buf;
  int32_t* result;
  bool byte_rows;
  int32_t bits, word2, row_pack, row_bytes, data_bytes, pal_bytes;
  uint16_t row16;
  row16 = img_w;
  bits = bpp;
  word2 = (uint8_t)bpp<<16;
  if( bpp>=5&&bpp<=7 ) {
    row_bytes = row16;
  } else {
    if( bpp==3 )
      bits = 4;
    else if( !packed )
      bits = bpp>4 ? bpp : 4;
    byte_rows = bpp!=3&&bits>=8;
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
      word2 = ((uint8_t)bpp<<16)|0x40000000;
      row_bytes = (uint16_t)row_pack;
    }
  }
  data_bytes = row_bytes*img_h;
  if( palette ) {
    pal_bytes = 3<<(bpp&31);
    if( bpp>8 )
      pal_bytes = 0;
    else
      word2 |= 0x00800000;
  } else {
    pal_bytes = 0;
  }
  result = ((int32_t*)bmf_new(data_bytes+pal_bytes+19));
  if( !result )
    return nullptr;
  BmfImage*const img = (BmfImage*)result;
  img->width = img_w;
  img->height = img_h;
  img->stride = row_bytes;
  result[2] = word2;
  img->data_size = data_bytes;
  if( pal_bytes ) {
    if( (((const BmfImage*)result)->depth&0x80)!=0 )
      buf = (uint8_t*)result+result[3]+16;
    else
      buf = nullptr;
    memset(buf, 0, pal_bytes);
    return result;
  }
  return result;
}

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
int32_t __write_bmp(BmfImage* img, char* path, int32_t want_rle) {
  int8_t depth_flags;
  uint32_t levels;
  uint32_t off_bits, data_ofs, stride, pairs, done;
  uint8_t* abs_end, * buf_2, * end, * out;
  int32_t rows2;
  int32_t ncol, pal_bytes;
  uint8_t* buf, * abs_end2;
  int32_t row_i2, run_max, nib;
  FILE* fp, * fp2;
  bool can_rle;
  uint32_t stride1, at, at2, slot;
  uint16_t* pix;
  uint8_t* out_buf, * pal, * pal2, * out_at, * p, * q, * out_end, * out2;
  uint32_t data_ofs2;
  int32_t rle_on, i, rows, bits, ncolours, grey, bgr, r, bgr2, rle_mode, src_bits, rle_kind, rows3, row_i, lit_len, run, coded_bytes, y;
  uint8_t byte;
  uint32_t k, j, data_len, stride3, n_bytes, pad;
  rle_on = want_rle;
  fp = fopen(path, "wb");
  if( !fp )
    return 0;
  out_buf = (uint8_t*)bmf_new(img->data_size+8*img->height+(img->data_size>>5)+2048);
  BmpHeader* bmp = (BmpHeader*)out_buf;
  i = img->width;
  bmp->biSize = 40;
  bmp->bfType = 0x4D42;
  rows = img->height;
  rows2 = rows;
  bmp->bfReserved2 = 0;
  bmp->bfReserved1 = 0;
  bmp->biWidth = i;
  bmp->biHeight = rows;
  depth_flags = (int8_t)img->depth;
  bmp->biPlanes = 1;
  bits = img->depth&0x3F;
  bmp->biBitCount = bits;
  bmp->biClrImportant = 0;
  bmp->biClrUsed = 0;
  bmp->biYPelsPerMeter = 0;
  bmp->biXPelsPerMeter = 0;
  buf = out_buf+54;
  if( bits<=8 ) {
    ncolours = 1<<(bits&31);
    ncol = 1<<(bits&31);
    if( (depth_flags&0x40)!=0 ) {
      levels = 0x100u>>(bits&31);
      if( ncol>0 ) {
        if( ncol/2 ) {
          k = 0;
          grey = 0;
          do {
            *(uint32_t*)&out_buf[8*k+54] = ((uint8_t)grey<<16)|(uint8_t)grey|((uint8_t)grey<<8);
            *(uint32_t*)&out_buf[8*k+58] = ((uint8_t)(grey-levels+2*levels)<<16)|(uint8_t)(grey-levels+2*levels)|((uint8_t)(grey-levels+2*levels)<<8);
            grey += 2*levels;
            ++k;
          } while( k<(uint32_t)(ncol/2) );
          at = 2*k+1;
        } else {
          at = 1;
        }
        if( (at-1)<(uint32_t)ncol )
          *(uint32_t*)&out_buf[4*at+50] = ((uint8_t)(levels*(at-1))<<16)|(uint8_t)(levels*(at-1))|((uint8_t)(levels*(at-1))<<8);
      }
      data_ofs2 = img->data_size;
      pal_bytes = 4*ncol;
    } else if( depth_flags<0 ) {
      data_ofs2 = img->data_size;
      if( ncol<=0 ) {
        pal_bytes = 4*ncolours;
      } else {
        pairs = ncolours/2;
        if( ncolours/2 ) {
          data_ofs = img->data_size;
          j = 0;
          pal = (uint8_t*)img+data_ofs2;
          do {
            slot = 2*j;
            bgr = *(uint16_t*)&pal[6*j+19];
            r = (uint8_t)pal[6*j+21];
            *(uint32_t*)&out_buf[4*slot+54] = ((uint8_t)pal[6*j+18]<<16)|*(uint16_t*)&pal[6*j+16];
            *(uint32_t*)&out_buf[4*slot+58] = (r<<16)|bgr;
            ++j;
          } while( j<pairs );
          data_ofs2 = data_ofs;
          rle_on = want_rle;
          at2 = 2*j+1;
          done = at2;
        } else {
          at2 = 1;
          done = 1;
        }
        if( (at2-1)<(uint32_t)ncol ) {
          pal2 = (uint8_t*)img+data_ofs2;
          bgr2 = *(uint16_t*)&pal2[3*done+13];
          *(uint32_t*)&out_buf[4*done+50] = (((uint8_t)pal2[3*done+15])<<16)|bgr2;
        }
        pal_bytes = 4*ncol;
      }
    } else {
      pal_bytes = 4*ncolours;
      memset(buf, 0, 4*ncolours);
      data_ofs2 = img->data_size;
      rows2 = img->height;
    }
    buf = &out_buf[pal_bytes+54];
  } else {
    data_ofs2 = img->data_size;
  }
  stride = img->stride;
  out = out_buf;
  off_bits = (uint32_t)(buf-out_buf);
  pix = (uint16_t*)img;
  rle_mode = rle_on;
  data_len = data_ofs2;
  while( 1 ) {
    out_at = buf;
    p = (uint8_t*)pix+data_len-stride+16;
    bmp->bfOffBits = off_bits;
    if( !rle_mode )
      break;
    src_bits = pix[5]&0x3F;
    can_rle = src_bits==4;
    if( src_bits!=4 ) {
      if( src_bits!=8 )
        break;
      can_rle = 0;
    }
    rle_kind = 2;
    if( !can_rle )
      rle_kind = 1;
    bmp->biCompression = rle_kind;
    nib = rle_kind-1;
    run_max = (0x100u>>((rle_kind-1)&31))-1;
    if( rows2>0 ) {
      rows3 = rows2;
      stride3 = stride;
      row_i = 0;
      while( 1 ) {
        if( p>=&p[stride3] )
          goto LABEL_72;
        buf_2 = out_at;
        end = &p[stride3];
        row_i2 = row_i;
        lit_len = 0;
        do {
          while( 1 ) {
            while( 1 ) {
              q = p+1;
              if( p+1>=end )
                break;
              byte = (uint8_t)*p;
              run = 1;
              do {
                if( byte!=(uint8_t)p[run] )
                  break;
                ++run;
              } while( end>&p[run] );
              if( run<=2&&(run!=2||lit_len) )
                break;
              if( run_max<run )
                run = run_max;
              if( lit_len ) {
                if( nib ? lit_len!=1 : lit_len>=3 ) {
                  *buf_2 = 0;
                  abs_end2 = buf_2+2;
                  buf_2[1] = lit_len<<(nib&31);
                  memcpy(buf_2+2, &p[-lit_len], lit_len);
                  buf_2 += lit_len+2;
                  if( (lit_len&1)!=0 ) {
                    abs_end2[lit_len] = 0;
                    ++buf_2;
                  }
                } else if( nib ) {
                  *buf_2 = 2;
                  buf_2[1] = *(p-1);
                  buf_2 += 2;
                } else {
                  if( lit_len==2 ) {
                    *buf_2 = 1;
                    buf_2[1] = *(p-2);
                    buf_2 += 2;
                  }
                  *buf_2 = 1;
                  buf_2[1] = *(p-1);
                  buf_2 += 2;
                }
                lit_len = 0;
                byte = *p;
              }
              buf_2[1] = byte;
              p += run;
              *buf_2 = run<<(nib&31);
              out_end = buf_2+2;
              buf_2 += 2;
              if( p>=end ) {
                out_at = out_end;
                row_i = row_i2;
                rows3 = img->height;
                stride3 = img->stride;
                goto LABEL_72;
              }
            }
            ++p;
            if( ++lit_len!=run_max )
              break;
            if( nib ? lit_len!=1 : lit_len>=3 ) {
              *buf_2 = 0;
              abs_end = buf_2+2;
              buf_2[1] = lit_len<<(nib&31);
              memcpy(buf_2+2, &q[-lit_len], lit_len);
              buf_2 += lit_len+2;
              if( (lit_len&1)!=0 ) {
                abs_end[lit_len] = 0;
                ++buf_2;
              }
            } else if( nib ) {
              *buf_2 = 2;
              buf_2[1] = *(q-1);
              buf_2 += 2;
            } else {
              if( lit_len==2 ) {
                *buf_2 = 1;
                buf_2[1] = *(q-2);
                buf_2 += 2;
              }
              *buf_2 = 1;
              buf_2[1] = *(q-1);
              buf_2 += 2;
            }
            if( q>=end ) {
              out_at = buf_2;
              row_i = row_i2;
              rows3 = img->height;
              stride3 = img->stride;
              goto LABEL_72;
            }
            lit_len = 0;
          }
        } while( q<end );
        out_at = buf_2;
        row_i = row_i2;
        if( lit_len&&(nib ? lit_len!=1 : lit_len>=3) ) {
          *buf_2 = 0;
          buf_2[1] = lit_len<<(nib&31);
          memcpy(buf_2+2, &q[-lit_len], lit_len);
          row_i = row_i2;
          out_at = &buf_2[lit_len+2];
          if( (lit_len&1)!=0 ) {
            buf_2[lit_len+2] = 0;
            ++out_at;
          }
        } else if( lit_len&&nib ) {
          buf_2[1] = *(q-1);
          *buf_2 = 2;
          out_at = buf_2+2;
        } else if( lit_len ) {
          if( lit_len==2 ) {
            *buf_2 = 1;
            buf_2[1] = *(q-2);
            out_at = buf_2+2;
          }
          out_at[1] = *(q-1);
          *out_at = 1;
          out_at += 2;
        }
        rows3 = img->height;
        stride3 = img->stride;
LABEL_72:
        *out_at = 0;
        out_at[1] = 0;
        out_at += 2;
        ++row_i;
        p -= 2*stride3;
        if( row_i>=rows3 ) {
          rows2 = rows3;
          stride = stride3;
          pix = (uint16_t*)img;
          data_len = img->data_size;
          break;
        }
      }
    }
    *out_at = 0;
    out_at[1] = 1;
    out_at += 2;
    coded_bytes = out_at-buf;
    if( data_len>(uint32_t)(out_at-buf) ) {
      bmp->biSizeImage = (uint32_t)coded_bytes;
      n_bytes = (uint32_t)(out_at-out);
      bmp->bfSize = n_bytes;
      if( fwrite(out, 1u, n_bytes, fp)!=bmp->bfSize )
        return 0;
      free(out);
      fclose(fp);
      return 1;
    }
    rle_mode = 0;
  }
  fp2 = fp;
  out2 = out;
  bmp->biCompression = 0;
  if( rows2<=0 ) {
    coded_bytes = 0;
  } else {
    pad = ((stride+3)&0xFFFFFFFC)-stride;
    stride1 = stride;
    y = 0;
    do {
      memcpy(out_at, p, stride1);
      stride1 = img->stride;
      out_at += stride1;
      p -= stride1;
      if( pad ) {
        *(uint32_t*)out_at = 0;
        out_at += pad;
      }
      ++y;
    } while( y<img->height );
    out2 = out;
    fp2 = fp;
    coded_bytes = out_at-buf;
  }
  bmp->biSizeImage = (uint32_t)coded_bytes;
  n_bytes = (uint32_t)(out_at-out2);
  bmp->bfSize = n_bytes;
  if( fwrite(out2, 1u, n_bytes, fp2)!=bmp->bfSize )
    return 0;
  free(out2);
  fclose(fp2);
  return 1;
}

void __init_symbol_list(SymList* list, int32_t n_syms, int32_t dense) {
  SymEntry* buf;
  uint32_t result;
  const uint32_t n = (uint32_t)n_syms;
  list->n = n_syms;
  buf = (SymEntry*)bmf_new(3*n_syms);
  list->ent = buf;
  if( dense ) {
    list->tot = 0;
    list->live = n;
    result = 12*n;
    list->since_rescale = 12*n;
    list->rescale_at = 8*n;
    if( n ) {
      result = 0;
      do {
        list->ent[result].sym = result;
        list->ent[result].cnt = 1;
        ++result;
      } while( result<list->live );
    }
  } else {
    list->tot = 2;
    list->rescale_at = 20*n;
    list->live = 0;
    list->since_rescale = 18*n;
    memset(buf, 0, 3*n);
  }
  return;
}

static inline int32_t bmf_pixels(const uint8_t* p) {
  const BmfImage* img = (const BmfImage*)p;
  return img->width*img->height;
}

uint8_t*__interleave_plane(uint8_t* img, uint8_t* src, int32_t plane, int8_t unread_flag) {
  uint8_t* ref;
  int32_t wgt1, i2, ofs2, n_flat, step, i, ofs, left3, x3, left2, x2, mode, to_ref0, wgt2, to_ref1, wgt0, stride, dc, n;
  uint8_t* base, * p3, * p2;
  if( (plane_desc[plane+1].flags&8)==0 ) {
    n_flat = bmf_pixels(img);
    step = plane_count;
    if( plane_count==1 )
      return (uint8_t*)memcpy(&((BmfImage*)img)->pixels[plane], src, n_flat);
    img += plane;
    i = 0;
    ofs = 0;
    do {
      img[ofs+16] = src[i];
      ofs += step;
      ++i;
    } while( i<n_flat );
    return img;
  }
  stride = plane_count;
  n = bmf_pixels(img);
  bool by_weights;
  to_ref0 = plane_desc[1].src_plane-plane;
  to_ref1 = plane_desc[2].src_plane-plane;
  mode = plane_desc[plane+1].nrefs;
  dc = plane_desc[plane+1].dc;
  base = &((BmfImage*)img)->pixels[plane];
  wgt1 = plane_desc[plane+1].weight1;
  wgt0 = plane_desc[plane+1].weight0;
  wgt2 = plane_desc[plane+1].weight2;
  by_weights = false;
  if( mode==2&&wgt0+wgt1==128 ) {
    if( !wgt1 ) {
      by_weights = true;
    } else if( !wgt0 ) {
      to_ref0 = plane_desc[2].src_plane-plane;
      by_weights = true;
    }
  }
  if( !by_weights ) {
    if( plane_desc[plane+1].nrefs!=1 ) {
      if( mode==2 ) {
        left2 = bmf_pixels(img);
        p2 = base;
        do {
          x2 = dc+(uint8_t)*src++;
          *p2 = (uint8_t)(((wgt0*(uint8_t)p2[to_ref0]+wgt1*(uint32_t)(uint8_t)p2[to_ref1]+40)>>7)+x2);
          p2 += stride;
          --left2;
        } while( left2 );
      } else if( mode==3 ) {
        left3 = bmf_pixels(img);
        p3 = base;
        do {
          x3 = dc+(uint8_t)*src++;
          *p3 = (uint8_t)(((wgt1*(uint8_t)*(p3-2)+wgt0*(uint8_t)*(p3-3)+wgt2*(uint32_t)(uint8_t)*(p3-1)+63)>>7)+x3);
          p3 += stride;
          --left3;
        } while( left3 );
      }
      return img;
    }
  }
  img += plane;
  ref = &img[to_ref0];
  i2 = 0;
  ofs2 = 0;
  do {
    img[ofs2+16] = ref[ofs2+16]+dc+src[i2];
    ofs2 += stride;
    ++i2;
  } while( i2<n );
  return img;
}

uint8_t*__colour_transform(uint8_t* img, uint8_t* dst, int32_t plane, int8_t unread_flag) {
  uint8_t d;
  uint8_t* ref;
  int32_t wgt1, i2, ofs2, n_flat, step, i, ofs, left3, x3, wsum, w12sum, left2, x2, mode, to_ref0, wgt2, to_ref1, wgt0, dc, stride, n;
  uint32_t blend;
  uint8_t* src, * p3, * p2;
  if( (plane_desc[plane+1].flags&8)==0 ) {
    n_flat = bmf_pixels(img);
    step = plane_count;
    if( plane_count==1 )
      return (uint8_t*)memcpy(dst, &((const BmfImage*)img)->pixels[plane], n_flat);
    img += plane;
    i = 0;
    ofs = 0;
    do {
      dst[i] = img[ofs+16];
      ofs += step;
      ++i;
    } while( i<n_flat );
    return img;
  }
  stride = plane_count;
  n = bmf_pixels(img);
  bool by_weights;
  to_ref0 = plane_desc[1].src_plane-plane;
  to_ref1 = plane_desc[2].src_plane-plane;
  mode = plane_desc[plane+1].nrefs;
  dc = plane_desc[plane+1].dc;
  src = &((BmfImage*)img)->pixels[plane];
  wgt1 = plane_desc[plane+1].weight1;
  wgt0 = plane_desc[plane+1].weight0;
  wgt2 = plane_desc[plane+1].weight2;
  by_weights = false;
  if( mode==2&&wgt0+wgt1==128 ) {
    if( !wgt1 ) {
      by_weights = true;
    } else if( !wgt0 ) {
      to_ref0 = plane_desc[2].src_plane-plane;
      by_weights = true;
    }
  }
  if( by_weights||plane_desc[plane+1].nrefs==1 ) {
    img += plane;
    ref = &img[to_ref0];
    i2 = 0;
    ofs2 = 0;
    do {
      d = img[ofs2+16]-dc-ref[ofs2+16];
      ofs2 += stride;
      dst[i2++] = d;
    } while( i2<n );
    return img;
  }
  if( mode==2 ) {
    left2 = bmf_pixels(img);
    p2 = src;
    do {
      x2 = *p2-dc;
      blend = wgt0*p2[to_ref0]+wgt1*p2[to_ref1]+40;
      p2 += stride;
      *dst++ = (uint8_t)(x2-(blend>>7));
      --left2;
    } while( left2 );
  } else if( mode==3 ) {
    left3 = bmf_pixels(img);
    p3 = src;
    do {
      x3 = *p3-dc;
      wsum = wgt1**(p3-2)+wgt0**(p3-3);
      w12sum = wgt2**(p3-1);
      p3 += stride;
      *dst++ = (uint8_t)(x3-((uint32_t)(wsum+w12sum+63)>>7));
      --left3;
    } while( left3 );
  }
  return img;
}

void __rc_begin_decode(int8_t unread_flag) {
  int32_t bits_left, at4, at5, k;
  uint16_t* tbl0, * row;
  FreqPair* seed;
  int32_t at6, at7;
  uint8_t* cursor;
  uint32_t i;
  cursor = out_cursor;
  if( (uint32_t*)out_cursor!=packer_word ) {
    bits_left = ::packer_free_bits-8;
    if( ::packer_free_bits-8<0 ) {
      ::packer_free_bits -= 8;
    } else {
      do {
        --cursor;
        bits_left -= 8;
      } while( bits_left>=0 );
      out_cursor = cursor;
      ::packer_free_bits = bits_left;
    }
  }
  if( plane_alt_model ) {
    if( plane_predictor==2 ) {
      alt_freq_init = 8;
      alt_freq_limit = 8;
    } else {
      alt_freq_limit = 16;
      alt_freq_init = 64;
    }
    level_geom[2].first = 2;
    model_geometry[0] = 0;
    model_geometry[1] = 1;
    model_geometry[2] = 2;
    model_geometry[3] = 2;
    level_geom[2].tbl_base = 0;
    level_geom[2].half = 1;
    level_geom[3].half = 2;
    level_geom[3].first = 4;
    level_geom[3].tbl_base = 1;
    memset(&model_geometry[4], 3, 4);
    level_geom[4].half = 4;
    at4 = 2*level_geom[3].half+4;
    level_geom[4].first = 2*level_geom[3].half+4;
    level_geom[4].tbl_base = 2*level_geom[3].half;
    memset(&model_geometry[at4], 4, 8);
    level_geom[5].half = 8;
    at5 = at4+2*level_geom[4].half;
    level_geom[5].first = at5;
    level_geom[5].tbl_base = at5-5;
    memset(&model_geometry[at5], 0x05, 16);
    level_geom[6].half = 16;
    at6 = at5+2*level_geom[5].half;
    level_geom[6].first = (uint8_t)at6;
    level_geom[6].tbl_base = (uint8_t)at6-6;
    memset(&model_geometry[at6], 0x06, 32);
    level_geom[7].half = 32;
    at7 = at6+2*level_geom[6].half;
    level_geom[7].first = (uint8_t)at7;
    level_geom[7].tbl_base = (uint8_t)at7-7;
    memset(&model_geometry[at7], 0x07, 64);
    tbl0 = (uint16_t*)bmf_new(0x7F000u);
    if( tbl0 ) {
      row = tbl0;
      k = 0;
      do {
        row[1] = 24*alt_freq_limit;
        row[2] = 205;
        row[6] = 48;
        row[3] = 124;
        row[7] = 16;
        row[4] = 147;
        row[5] = 83;
        row[8] = 8;
        row[9] = 4;
        *row = 635;
        seed = (FreqPair*)&row[10];
        for( i = 0; i<0x7A; ++i ) {
          seed[i].f[0] = 60;
          seed[i].f[1] = 36;
        }
        row += 254;
        ++k;
      } while( (uint32_t)k<0x400 );
    }
    ::model_tables = tbl0;
    cursor = out_cursor;
  }
  rc.dec_init();
}

uint8_t*__unpredict_med(uint8_t* pixels, int32_t width, int32_t height) {
  uint32_t done;
  uint8_t cur, pred;
  int32_t rows_left, north, northwest;
  uint32_t j, row_rest, pairs, k, x_left;
  uint8_t* p, * up;
  alignas(16) uint8_t unfold[256];
  p = (pixels+1);
  unfold[0] = 0;
  for( j = 0; j<127; ++j ) {
    unfold[2*j+1] = (uint8_t)(-1-(int32_t)j);
    unfold[2*j+2] = (uint8_t)(1+j);
  }
  unfold[255] = (uint8_t)(-1-127);
  if( width==1 ) {
    rows_left = height-1;
    if( height==1 )
      return p;
    row_rest = 0;
  } else {
    row_rest = width-1;
    pairs = (width-1)/2;
    if( pairs ) {
      for( k = 0; k<pairs; ++k ) {
        cur = pixels[2*k]+unfold[pixels[2*k+1]];
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
    rows_left = height-1;
    if( height==1 )
      return p;
  }
  {
    up = &p[-width];
    do {
      while( 1 ) {
        ++up;
        *p = unfold[(uint8_t)*p]+p[-width];
        x_left = row_rest;
        ++p;
        if( width!=1 )
          break;
        if( !--rows_left )
          return p;
      }
      do {
        pred = (uint8_t)*(p-1);
        north = (uint8_t)*up;
        northwest = (uint8_t)p[-width-1];
        if( pred<north ) {
          if( northwest<pred )
            pred = (uint8_t)north;
          else if( northwest<=north )
            pred = (uint8_t)(north+pred-northwest);
        } else {
          if( northwest>pred )
            pred = (uint8_t)north;
          else if( northwest>=north )
            pred = (uint8_t)(north+pred-northwest);
        }
        *p = pred+unfold[(uint8_t)*p];
        ++up;
        ++p;
        --x_left;
      } while( x_left );
      --rows_left;
    } while( rows_left );
  }
  return p;
}

int32_t __estimate_cost(uint8_t* bins, int32_t n) {
  const int32_t* bin = (const int32_t*)bins;
  double sum_even = 0.0, sum_odd = 0.0, ent_even = 0.0, ent_odd = 0.0;
  double total, entropy;
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
  entropy = ent_even+ent_odd;
  total = sum_even+sum_odd;
  for(; i<n; i++ )
    if( bin[i] ) {
      total += (double)bin[i];
      entropy += (double)bin[i]*log((double)bin[i]);
    }
  if( total!=0.0 )
    total = total*log(total);
  return (int32_t)((total-entropy)*1.442695040888963);
}

void**__alt_model_p1_d8_decode(int8_t unread_flag, uint8_t* out, int32_t i, int32_t height) {
  P1Ctx* buf1, * b0, * cursor0, * b4, * buf3, * buf2;
  AltP1Block* blk, * raw;
  int64_t err;
  uint8_t* out_at;
  int32_t y, val, x, code;
  P1Ctx* cursor2, * cursor4;
  raw = (AltP1Block*)bmf_new(sizeof(AltP1Block));
  if( raw )
    blk = raw->alt_p1_alloc(i, height, 0);
  else
    blk = nullptr;
  __rc_begin_decode(unread_flag);
  if( blk->height>0 ) {
    y = 0;
    do {
      ++y;
      {
        P1Ctx*const here = (P1Ctx*)blk->cursor[0];
        here[0] = here[-1];
        here[1] = here[-2];
        here[2] = here[-3];
        here[3] = here[-4];
        here[4] = here[-5];
        here[5] = here[-6];
      }
      b4 = blk->buf[4];
      buf3 = blk->buf[3];
      buf2 = blk->buf[2];
      buf1 = blk->buf[1];
      b0 = blk->buf[0];
      blk->buf[4] = buf3;
      blk->buf[3] = buf2;
      blk->buf[2] = buf1;
      blk->buf[1] = b0;
      blk->buf[0] = b4;
      b4 += 4;
      blk->cursor[0] = b4;
      b0 += 4;
      blk->cursor[1] = b0;
      blk->cursor[2] = buf1+4;
      blk->cursor[3] = buf2+4;
      blk->cursor[4] = buf3+4;
      ((P1Ctx*)b4)[-4] = ((P1Ctx*)b0)[3];
      {
        P1Ctx*const here = (P1Ctx*)blk->cursor[0];
        P1Ctx*const up = (P1Ctx*)blk->cursor[1];
        here[-3] = up[2];
        here[-2] = up[1];
        here[-1] = up[0];
      }
      cursor2 = blk->cursor[2];
      cursor4 = blk->cursor[4];
      blk->ctx[2] = 0;
      cursor0 = blk->cursor[0];
      blk->ctx[3] = cursor2[-2].mag+cursor4[-2].mag+cursor2[0].mag+cursor4[0].mag+cursor2[2].mag+cursor4[2].mag+cursor2[4].mag+cursor4[4].mag+cursor0[-4].mag+cursor0[-2].mag;
      blk->ctx[4] = cursor2[-1].mag+cursor4[-1].mag+cursor2[1].mag+cursor4[1].mag+cursor2[3].mag+cursor4[3].mag+cursor2[5].mag+cursor4[5].mag+cursor0[-3].mag+cursor0[-1].mag;
      if( !(blk->width<=0) ) {
        out_at = out;
        x = 0;
        do {
          ++x;
          blk->ctx_of((AltP1Block*)nullptr, (AltP1Block*)0);
          code = __alt_p1_decode_symbol((uint16_t*)&blk->counters[blk->ctx[0]], 0, blk->ctx[1]);
          val = (uint8_t)((uint8_t)blk->pred+blk->unfold[(uint8_t)code]);
          *out_at = val;
          err = val-blk->pred;
          blk->cursor[0]->sym = val;
          blk->cursor[0]->mag = (BYTE4(err)^err)-BYTE4(err);
          blk->ctx[3+blk->ctx[2]] = blk->ctx[3+blk->ctx[2]]+blk->cursor[0]->mag-blk->cursor[0][-4].mag-(blk->cursor[4][-2].mag-blk->cursor[4][6].mag+blk->cursor[2][-2].mag-blk->cursor[2][6].mag);
          blk->ctx[2] = blk->ctx[2]==0;
          if( blk->counters[blk->ctx[0]].total<0x4000u )
            ((AltP1Block*)blk)->update_model();
          ++blk->cursor[0];
          ++out_at;
          ++blk->cursor[1];
          ++blk->cursor[2];
          ++blk->cursor[3];
          ++blk->cursor[4];
        } while( x<blk->width );
        out = out_at;
      }
    } while( y<blk->height );
  }
  __rc_end_decode();
  return (void**)blk->alt_p1_free(1);
}

int32_t __alt_model_p1_decode(uint16_t* hdr, uint8_t* out) {
  P1Ctx* b4, * buf3, * buf2, * buf1, * b0, * cursor0;
  void* p0v;
  int32_t val3x;
  void* plane[4];
  AltP1Block*&plane1 = (AltP1Block*&)plane[1];
  AltP1Block*&plane2 = (AltP1Block*&)plane[2];
  AltP1Block*&plane3 = (AltP1Block*&)plane[3];
  int32_t dc1;
  uint32_t x;
  int32_t val1x;
  AltP1Block* blk1;
  int32_t code1;
  AltP1Block* blk_k, * raw;
  uint32_t at0;
  int32_t code0;
  uint32_t pred0, pred1;
  AltP1Block* blk2, * blk3;
  P1Ctx* cursor2, * cursor4;
  void** q;
  AltP1Block* made;
  int32_t width, height, k, src1, src2, src3, dc2, dc3, xf1, xf2, xf3, w, n_planes, p, val0, val1, code2, val2, at2, code3, val3, at3, np, f;
  uint32_t y;
  int32_t pred2, pred3;
  width = *hdr;
  height = hdr[1];
  if( plane_count>0 ) {
    k = 0;
    do {
      raw = (AltP1Block*)bmf_new(sizeof(AltP1Block));
      if( raw )
        made = raw->alt_p1_alloc(width, height, k);
      else
        made = nullptr;
      plane[k++] = made;
    } while( k<plane_count );
  }
  src1 = plane_desc[2].src_plane;
  src2 = plane_desc[3].src_plane;
  src3 = plane_desc[4].src_plane;
  dc2 = plane_desc[src2+1].dc;
  dc3 = plane_desc[src3+1].dc;
  dc1 = plane_desc[src1+1].dc;
  xf1 = plane_desc[src1+1].flags&8;
  xf2 = plane_desc[src2+1].flags&8;
  xf3 = plane_desc[src3+1].flags&8;
  __rc_begin_decode(xf2);
  if( height>0 ) {
    y = 0;
    w = width;
    n_planes = plane_count;
    do {
      if( n_planes>0 ) {
        p = 0;
        do {
          ++p;
          blk_k = (AltP1Block*)plane[p-1];
          {
            P1Ctx*const here = (P1Ctx*)blk_k->cursor[0];
            here[0] = here[-1];
            here[1] = here[-2];
            here[2] = here[-3];
            here[3] = here[-4];
            here[4] = here[-5];
            here[5] = here[-6];
          }
          b4 = blk_k->buf[4];
          buf3 = blk_k->buf[3];
          buf2 = blk_k->buf[2];
          buf1 = blk_k->buf[1];
          b0 = blk_k->buf[0];
          blk_k->buf[4] = buf3;
          blk_k->buf[3] = buf2;
          blk_k->buf[2] = buf1;
          blk_k->buf[1] = b0;
          blk_k->buf[0] = b4;
          b4 += 4;
          blk_k->cursor[0] = b4;
          b0 += 4;
          blk_k->cursor[1] = b0;
          blk_k->cursor[2] = buf1+4;
          blk_k->cursor[3] = buf2+4;
          blk_k->cursor[4] = buf3+4;
          ((P1Ctx*)b4)[-4] = ((P1Ctx*)b0)[3];
          {
            P1Ctx*const here = (P1Ctx*)blk_k->cursor[0];
            P1Ctx*const up = (P1Ctx*)blk_k->cursor[1];
            here[-3] = up[2];
            here[-2] = up[1];
            here[-1] = up[0];
          }
          cursor2 = blk_k->cursor[2];
          cursor4 = blk_k->cursor[4];
          cursor0 = blk_k->cursor[0];
          blk_k->ctx[2] = 0;
          blk_k->ctx[3] = (int8_t)cursor2[-2].mag+(int8_t)cursor4[-2].mag+cursor2[0].mag+cursor4[0].mag+cursor2[2].mag+cursor4[2].mag+cursor2[4].mag+cursor4[4].mag+cursor0[-4].mag+cursor0[-2].mag;
          blk_k->ctx[4] = (int8_t)cursor2[-1].mag+(int8_t)cursor4[-1].mag+cursor2[1].mag+cursor4[1].mag+cursor2[3].mag+cursor4[3].mag+cursor2[5].mag+cursor4[5].mag+cursor0[-3].mag+cursor0[-1].mag;
          n_planes = plane_count;
        } while( p<plane_count );
        w = width;
      }
      if( w>0 ) {
        x = 0;
        do {
          AltP1Block*const blk = (AltP1Block*)plane[0];
          blk->ctx_of(nullptr, nullptr);
          code0 = __alt_p1_decode_symbol((uint16_t*)&blk->counters[blk->ctx[0]], 0, blk->ctx[1]);
          pred0 = blk->pred;
          val0 = (uint8_t)(pred0+blk->unfold[code0]);
          blk->cursor[0]->sym = val0;
          blk->cursor[0]->mag = abs32(val0-pred0);
          blk->ctx[blk->ctx[2]+3] = blk->ctx[blk->ctx[2]+3]+blk->cursor[0]->mag-blk->cursor[0][-4].mag-(blk->cursor[4][-2].mag-blk->cursor[4][6].mag+blk->cursor[2][-2].mag-blk->cursor[2][6].mag);
          at0 = blk->ctx[0];
          blk->ctx[2] = blk->ctx[2]==0;
          if( blk->counters[at0].total<0x4000u )
            blk->update_model();
          ++blk->cursor[0];
          ++blk->cursor[1];
          ++blk->cursor[2];
          ++blk->cursor[3];
          ++blk->cursor[4];
          blk1 = (AltP1Block*)plane1;
          *(plane_desc[1].src_plane+out) = val0;
          ((AltP1Block*)blk1)->ctx_of((AltP1Block*)plane[0], (AltP1Block*)0);
          code1 = __alt_p1_decode_symbol(&blk1->counters[blk1->ctx[0]].total, 0, blk1->ctx[1]);
          pred1 = blk1->pred;
          val1 = (uint8_t)(pred1+blk1->unfold[code1]);
          val1x = val1;
          blk1->cursor[0]->sym = val1;
          blk1->cursor[0]->mag = abs32(val1-pred1);
          blk1->ctx[3+blk1->ctx[2]] = blk1->ctx[3+blk1->ctx[2]]+blk1->cursor[0]->mag-blk1->cursor[0][-4].mag-(blk1->cursor[4][-2].mag-blk1->cursor[4][6].mag+blk1->cursor[2][-2].mag-blk1->cursor[2][6].mag);
          blk1->ctx[2] = blk1->ctx[2]==0;
          if( blk1->counters[blk1->ctx[0]].total<0x4000u )
            blk1->update_model();
          ++blk1->cursor[0];
          ++blk1->cursor[1];
          ++blk1->cursor[2];
          ++blk1->cursor[3];
          ++blk1->cursor[4];
          if( xf1 )
            val1x += dc1+*(plane_desc[1].src_plane+out);
          blk2 = (AltP1Block*)(plane2);
          p0v = plane[0];
          *(out+plane_desc[2].src_plane) = val1x;
          ((AltP1Block*)blk2)->ctx_of((AltP1Block*)plane1, (AltP1Block*)p0v);
          code2 = __alt_p1_decode_symbol((uint16_t*)&blk2->counters[blk2->ctx[0]], 0, (int32_t)blk2->ctx[1]);
          pred2 = blk2->pred;
          val2 = (uint8_t)(pred2+blk2->unfold[code2]);
          blk2->cursor[0]->sym = val2;
          blk2->cursor[0]->mag = abs32(val2-pred2);
          blk2->ctx[blk2->ctx[2]+3] += (blk2->cursor[0]->mag-blk2->cursor[0][-4].mag-(blk2->cursor[4][-2].mag-blk2->cursor[4][6].mag)-(blk2->cursor[2][-2].mag-blk2->cursor[2][6].mag));
          at2 = blk2->ctx[0];
          blk2->ctx[2] = blk2->ctx[2]==0;
          if( blk2->counters[at2].total<0x4000u )
            ((AltP1Block*)blk2)->update_model();
          ++blk2->cursor[0];
          ++blk2->cursor[1];
          ++blk2->cursor[2];
          ++blk2->cursor[3];
          ++blk2->cursor[4];
          if( xf2 )
            *(plane_desc[3].src_plane+out) = ((plane_desc[plane_desc[3].src_plane+1].weight0**(plane_desc[1].src_plane+out)+plane_desc[plane_desc[3].src_plane+1].weight1*(uint32_t)*(plane_desc[2].src_plane+out)+40)>>7)+dc2+val2;
          else
            *(plane_desc[3].src_plane+out) = val2;
          n_planes = plane_count;
          if( plane_count>=4 ) {
            blk3 = (AltP1Block*)(plane3);
            ((AltP1Block*)plane3)->ctx_of((AltP1Block*)plane2, (AltP1Block*)plane1);
            code3 = __alt_p1_decode_symbol((uint16_t*)&blk3->counters[blk3->ctx[0]], 0, (int32_t)blk3->ctx[1]);
            pred3 = blk3->pred;
            val3 = (uint8_t)(pred3+blk3->unfold[code3]);
            val3x = val3;
            blk3->cursor[0]->sym = val3;
            blk3->cursor[0]->mag = abs32(val3-pred3);
            blk3->ctx[blk3->ctx[2]+3] += (blk3->cursor[0]->mag-blk3->cursor[0][-4].mag-(blk3->cursor[4][-2].mag-blk3->cursor[4][6].mag)-(blk3->cursor[2][-2].mag-blk3->cursor[2][6].mag));
            at3 = blk3->ctx[0];
            blk3->ctx[2] = blk3->ctx[2]==0;
            if( blk3->counters[at3].total<0x4000u )
              ((AltP1Block*)blk3)->update_model();
            ++blk3->cursor[0];
            ++blk3->cursor[1];
            ++blk3->cursor[2];
            ++blk3->cursor[3];
            ++blk3->cursor[4];
            if( xf3 )
              val3x += ((plane_desc[plane_desc[4].src_plane+1].weight1**(plane_desc[4].src_plane+out-2)+plane_desc[plane_desc[4].src_plane+1].weight0**(plane_desc[4].src_plane+out-3)+plane_desc[plane_desc[4].src_plane+1].weight2**(plane_desc[4].src_plane+out-1)+64)>>7)+dc3;
            *(plane_desc[4].src_plane+out) = val3x;
            n_planes = plane_count;
          }
          out += n_planes;
          ++x;
        } while( x<(uint32_t)width );
        w = width;
      }
      ++y;
    } while( y<(uint32_t)height );
  }
  __rc_end_decode();
  np = plane_count;
  if( plane_count>0 ) {
    f = 0;
    do {
      q = (void**)plane[f];
      if( q ) {
        ((AltP1Block*)q)->alt_p1_free(1);
        np = plane_count;
      }
      ++f;
    } while( f<np );
  }
  return np;
}

static inline float bmf_hsum4(const float x[4]) {
  return (x[0]+x[2])+(x[1]+x[3]);
}

static const float bmf_p2_mix[4][6] = {
  {0.4f, 0.1f, 0.15f, 0.23f, 0.04f, 0.03f},
  {0.26f, 0.23f, 0.13f, 0.12f, 0.15f, 0.12f},
  {0.27f, 0.22f, 0.26f, 0.08f, 0.12f, 0.05f},
  {0.3f, 0.14f, 0.08f, 0.22f, 0.18f, 0.09f},
};
static const float bmf_p2_decay = 0.78f;
static const float bmf_p2_seed = 0.19f;
int32_t __alt_p2_filter(float (*_this)[4], float (*a2)[4], CtxWeights* w, int32_t mode) {
  const float* mix = bmf_p2_mix[mode];
  float acc[4], mixed[7][4], centre, prediction, own;
  int32_t i, j, k;
  for( k = 0; k<4; k++ ) {
    acc[k] = bmf_p2_coef[0][k]*a2[0][k];
    for( j = 1; j<7; j++ )
      acc[k] += bmf_p2_coef[j][k]*a2[j][k];
  }
  centre = bmf_hsum4(acc);
  a2[7][0] = centre;
  for( j = 0; j<7; j++ )
    for( k = 0; k<4; k++ )
      a2[j][k] -= centre;
  for( j = 0; j<7; j++ )
    for( k = 0; k<4; k++ ) {
      mixed[j][k] = mix[0]*w->f0[0][j][k];
      for( i = 1; i<6; i++ )
        mixed[j][k] += mix[i]*w->f0[i][j][k];
    }
  for( k = 0; k<4; k++ ) {
    acc[k] = mixed[0][k]*a2[0][k];
    for( j = 1; j<7; j++ )
      acc[k] += mixed[j][k]*a2[j][k];
  }
  prediction = bmf_hsum4(acc)+centre;
  a2[7][2] = prediction;
  if( *(uint32_t*)&_this[15][0] ) {
    for( k = 0; k<4; k++ ) {
      acc[k] = _this[0][k]*a2[0][k];
      for( j = 1; j<7; j++ )
        acc[k] += _this[j][k]*a2[j][k];
    }
    own = centre+bmf_hsum4(acc);
    a2[7][1] = own;
    return (int32_t)(prediction+((own-prediction)*_this[14][0])/_this[14][1]);
  }
  for( j = 0; j<7; j++ )
    for( k = 0; k<4; k++ ) {
      _this[j][k] = mixed[j][k]*bmf_p2_decay;
      _this[j+7][k] = w->f0[0][j+7][k]*bmf_p2_seed;
    }
  _this[14][0] = 47.0f;
  _this[14][1] = 169.2f;
  _this[14][2] = 1.0f;
  a2[7][1] = prediction;
  return (int32_t)prediction;
}

int32_t __alt_p2_context(AltP2Block* blk, AltP2Block* refa, AltP2Block* refb) {
  P2Ctx* unused_p, * row0;
  CtxWeights weights;
  int32_t dv_now, mag_ref1, ctx15, mag_ref0, d_run0b, g3pair;
  P2Ctx* ra1, * ra0, * rb1, * rb0, * rb2, * ra2, * sub0_row, * row2c;
  int32_t sum_all;
  P2Ctx* cx1, * cx3;
  uint32_t q39, q10;
  float (**up_row)[4];
  int32_t dtop2, run_dv3, dv_now4, run_dv4, run_up4;
  int8_t rate1;
  uint8_t* cx2p;
  P2Ctx* nb4;
  int32_t num_b, band, num_d;
  int32_t gA, gB, gC, gD, den_d, nb_slot;
  P2Ctx* cx0;
  P2Ctx* cx2;
  int32_t up5, lap, magl;
  int32_t dvsum2, magu;
  P2Ctx* nb0, * nb3, * nb2, * cx4, * nb3x;
  float (*wrow)[4];
  bool in_band, no_ref;
  int16_t* nb2w, * nb2w2;
  P2Ctx* cursor1, * nb1, * nb2x;
  float (**cur)[4];
  int32_t sum4, lane5, nb_id, next_id, plane, in63, c_lo, mode, c_mid, filt, lane3, g3sum, run_s, lane2, ctx0, bank0, pred0, sum_c, run0, ctx1, bank1, w1c, one1, pred1, run1, cx2_val0, ctx2, bank2, pred2, run2, bank3, w3c, pred3, run3, bank4, pred4, band_lo, band_hi, d_run4, magsum, run_s2, mag, flat_a, flat_b, d_run4b, d_rb_rb1, d_rb_left, ra0_val, d_ra_ra1b, d_ra_leftb, ctx_idx, d_ra_ra1, d_ra_left;
  uint32_t ctx0_lo, q24, q10a, q10b, q9;
  int32_t sum_u, run, nb4_4, run4, sum_ul, d_up, d_ra;
  int32_t sum_ur, d_up5, dv1, d_run0, cx1_val, magsum_s;
  row0 = blk->cursor[0];
  cursor1 = blk->cursor[1];
  sub0_row = (P2Ctx*)blk->cursor[2];
  sum_ul = 21*sub0_row[-1].dupleft+12*cursor1[3].dupleft+16*cursor1[2].dupleft+22*cursor1[1].dupleft+(23*row0[-1].dupleft)+20*cursor1->dupleft+ctx_bias[0]+14*row0[-2].dupleft;
  sum_ur = 17*sub0_row[-2].dupright+21*cursor1[2].dupright+15*cursor1[1].dupright+25*cursor1->dupright+9*cursor1[-1].dupright+22*row0[-1].dupright+ctx_bias[1]+19*row0[-2].dupright;
  sum4 = 17*cursor1[3].dleft+15*cursor1[2].dleft+21*cursor1[1].dleft+18*cursor1->dleft+16*cursor1[-1].dleft+22*row0[-1].dleft+ctx_bias[2]+19*row0[-2].dleft;
  lane5 = sub0_row->dup;
  sum_u = 14*cursor1[3].dup+23*cursor1[1].dup+19*cursor1->dup+25*row0[-1].dup+ctx_bias[3]+17*row0[-2].dup+15*(blk->cursor[3]->dup+lane5);
  sum_all = sum4+sum_u+sum_ul+sum_ur;
  band = (8*sum4>43*sum_u)+(8*sum4>17*sum_u)+(8*sum4>9*sum_u)+(8*sum4>5*sum_u)+(8*sum4>2*sum_u);
  den_d = 2*row0[-2].val+2*row0[-1].val;
  num_b = cursor1[1].val+(cursor1->val+2*row0[-1].val);
  num_d = 16*(sub0_row[2].val+sub0_row->val+cursor1[1].val+sub0_row[-1].val);
  gA = (sum_all>bmf_p2_thresholds[band][12])+(sum_all>bmf_p2_thresholds[band][11])+(sum_all>bmf_p2_thresholds[band][10])+(sum_all>bmf_p2_thresholds[band][9]);
  gB = (num_b>bmf_p2_thresholds[band][8])+(num_b>bmf_p2_thresholds[band][7])+(num_b>bmf_p2_thresholds[band][6]);
  gC = (16*sum_ur>sum_ul*bmf_p2_thresholds[band][5])+(16*sum_ur>sum_ul*bmf_p2_thresholds[band][4])+(16*sum_ur>sum_ul*bmf_p2_thresholds[band][3]);
  gD = (num_d>bmf_p2_thresholds[band][2]*den_d)+(num_d>den_d*bmf_p2_thresholds[band][1])+(num_d>den_d*bmf_p2_thresholds[band][0]);
  nb_slot = 320*band+64*gA+16*gB+4*gC+gD;
  nb_id = blk->nb_id[(uint32_t)nb_slot];
  if( blk->nb_id[(uint32_t)nb_slot] ) {
    wrow = blk->nb_weights[nb_id];
    blk->nb_cur = wrow;
    {
      float acc[4], err, floor_;
      int32_t j, k;
      for( k = 0; k<4; ++k ) {
        acc[k] = wrow[0][k]*blk->p2_row[0][k];
        for( j = 1; j<7; ++j )
          acc[k] += wrow[j][k]*blk->p2_row[j][k];
      }
      err = ((float)row0[-1].val-(bmf_hsum4(acc)+blk->bias[0]))*2.0999999f;
      floor_ = 7744.0f*wrow[14][2];
      for( j = 0; j<7; ++j )
        for( k = 0; k<4; ++k ) {
          float x = blk->p2_row[j][k];
          float ms = wrow[7+j][k]+(x*x-wrow[7+j][k])*bmf_p2_ms_rate;
          wrow[7+j][k] = ms;
          wrow[j][k] += bmf_p2_rate[j][k]*err*x/(ms+floor_);
        }
    }
  } else {
    next_id = blk->nb_id_used;
    blk->nb_id_used = ++next_id;
    blk->nb_id[(uint32_t)nb_slot] = next_id;
    blk->nb_cur = blk->nb_weights[(int16_t)next_id];
  }
  nb1 = (P2Ctx*)blk->cursor[1];
  blk->p2_row[0][0] = (float)nb1->dval;
  nb0 = (P2Ctx*)blk->cursor[0];
  blk->p2_row[0][1] = (float)nb1[1].dval;
  blk->p2_row[0][2] = (float)(nb0[-1].dval+nb1->dval-nb1[-1].dval);
  row2c = (P2Ctx*)blk->cursor[2];
  blk->p2_row[0][3] = (float)(nb0[-2].dval+(nb1->dval-nb1[-2].dval));
  nb3 = (P2Ctx*)blk->cursor[3];
  blk->p2_row[1][0] = (float)(nb1[-1].dval+nb1->dval-row2c[-1].dval);
  blk->p2_row[1][1] = (float)(-3*(row2c->dval-nb1->dval)+nb3->dval);
  blk->p2_row[1][2] = (float)(nb0[-1].dval+nb1[2].dval-nb1[1].dval);
  blk->p2_row[1][3] = (float)(nb0[-2].dval+nb1[1].dval-nb1[-1].dval);
  blk->p2_row[2][0] = (float)(2*nb0[-1].dval-nb0[-2].dval);
  nb2 = (P2Ctx*)(row2c);
  blk->p2_row[2][1] = (float)(nb0[-3].dval+nb1->dval-nb1[-3].dval);
  blk->p2_row[2][2] = (float)(nb2->dval+nb1[1].dval-nb3[1].dval);
  blk->p2_row[2][3] = (float)nb0[-3].dval;
  blk->p2_row[3][0] = (float)(nb1[-2].dval+nb1->dval-nb2[-2].dval);
  blk->p2_row[3][1] = (float)(nb0[-2].dval+nb1[-1].dval-nb1[-3].dval);
  blk->p2_row[3][2] = (float)(nb1[1].dval+((nb1[2].dval+nb1->dval)>>1)-nb2[2].dval);
  blk->p2_row[3][3] = (float)nb3->dval;
  if( refa ) {
    ra0 = (P2Ctx*)refa->cursor[0]-1;
    ra1 = (P2Ctx*)((int16_t*)(refa->cursor[1]-1));
    ra2 = (P2Ctx*)((int16_t*)(refa->cursor[2]-1));
    rb0 = (P2Ctx*)refb->cursor[0]-1;
    no_ref = blk->has_ref==0;
    rb1 = refb->cursor[1]-1;
    rb2 = refb->cursor[2]-1;
    if( !no_ref ) {
      {
        float bias = (float)nb0->dval;
        int32_t j, k;
        for( j = 0; j<4; ++j )
          for( k = 0; k<4; ++k )
            blk->p2_row[j][k] += bias;
      }
      nb0 = (P2Ctx*)blk->cursor[0];
      nb1 = (P2Ctx*)blk->cursor[1];
    }
    plane = blk->plane_idx;
    if( plane ) {
      if( plane==1 ) {
        blk->p2_row[4][0] = (float)(nb0->dval+nb1[3].dval);
        blk->p2_row[4][1] = (float)(nb0[-2].val+ra1[2].val-ra1->val);
        blk->p2_row[4][2] = (float)(nb1[1].val+ra0[-1].val-ra1->val);
        blk->p2_row[4][3] = (float)(nb0[-1].val+ra1->val-ra1[-1].val);
        blk->p2_row[5][0] = (float)(nb1[1].val+(rb1[2].val-((P2Ctx*)(rb2))[3].val));
        blk->p2_row[5][1] = (float)(nb0[-2].val+rb0->val-rb0[-2].val);
        blk->p2_row[5][2] = (float)(nb1->val+rb0->val-rb1->val);
        no_ref = blk->has_ref==0;
        blk->p2_row[5][3] = (float)(nb0[-2].val+ra0->err);
        if( no_ref ) {
          nb2w = (int16_t*)blk->cursor[2];
          blk->p2_row[6][0] = ((float)(nb0[-2].val+ra0->val-ra0[-2].val));
          blk->p2_row[6][1] = (float)(*nb2w+ra0->val-((P2Ctx*)(ra2))->val);
          blk->p2_row[6][2] = (float)(ra2->val+ra0->val-*nb2w+2*(nb1->val-ra1->val));
          in63 = *(int16_t*)((uint8_t*)nb0-18)+nb1[-1].val+ra1[-2].val+ra0->val-ra0[-1].val-ra1[-1].val-nb1[-2].val;
        } else {
          blk->p2_row[6][0] = (float)nb1->val;
          blk->p2_row[6][1] = (float)nb0[-3].val;
          blk->p2_row[6][2] = (float)(nb0[-1].val+ra0[-1].val-ra0[-2].val);
          in63 = nb0[-3].val+nb0[-1].val-nb0[-4].val;
        }
        blk->p2_row[6][3] = (float)in63;
      } else {
        blk->p2_row[4][0] = (float)(nb0[-3].val+nb0[-1].val-nb0[-4].val);
        blk->p2_row[4][1] = ((float)(nb0[-1].val+ra0[-1].val-ra0[-2].val));
        blk->p2_row[4][2] = (float)(nb1[1].val+ra1->val-ra2[1].val);
        blk->p2_row[4][3] = (float)(nb0[-2].val+ra1[2].val-ra1->val);
        blk->p2_row[5][0] = (float)(nb1->val+ra0[-2].val-ra1[-2].val);
        blk->p2_row[5][1] = (float)(nb1->val+ra0->val-ra1->val);
        blk->p2_row[5][2] = (float)(nb0[-2].val+ra0->val-ra0[-2].val);
        nb2w2 = (int16_t*)blk->cursor[2];
        blk->p2_row[5][3] = (float)(nb0[-2].val+rb0->val-rb0[-2].val);
        blk->p2_row[6][0] = (float)(*nb2w2+rb0->val-((P2Ctx*)(rb2))->val);
        blk->p2_row[6][1] = (float)(rb0[-2].val+rb0->val-nb0[-2].val+2*(nb0[-1].val-rb0[-1].val));
        blk->p2_row[6][2] = ((float)(nb0[-2].val+rb0->err));
        blk->p2_row[6][3] = (float)(*nb2w2+ra0->err);
      }
    } else {
      blk->p2_row[4][0] = (float)(nb0[-3].val+nb0[-1].val-nb0[-4].val);
      nb3x = blk->cursor[3];
      blk->p2_row[4][1] = (float)(nb0[-5].val+nb1->val-nb1[-5].val);
      nb2x = (P2Ctx*)blk->cursor[2];
      blk->p2_row[4][2] = (float)(nb0[-4].val+nb1->val-nb1[-4].val);
      blk->p2_row[4][3] = ((float)((nb2x->val+3*nb1[1].val-4*nb2x[1].val)-(((nb1[2].val-nb1->val-(nb3x[2].val-nb3x->val))>>1)-nb3x[1].val)));
      blk->p2_row[5][0] = (float)(nb0[-2].val+ra0->val-ra0[-2].val);
      blk->p2_row[5][1] = (float)(nb1[1].val+ra1[1].val-ra2[2].val);
      blk->p2_row[5][2] = (float)(nb1[-2].val+ra1[2].val-ra2->val);
      blk->p2_row[5][3] = ((float)((ra0[-2].val+ra0->val-nb0[-2].val)+2*(nb0[-1].val-ra0[-1].val)));
      blk->p2_row[6][0] = (float)(nb2x[1].val+rb0->val-rb2[1].val);
      blk->p2_row[6][1] = ((float)(nb0[-2].val+rb0->val-rb0[-2].val));
      blk->p2_row[6][2] = (float)(nb0[-1].val+rb1[1].val-rb1->val);
      blk->p2_row[6][3] = (float)(nb1[1].val+rb1[2].val-rb2[3].val);
    }
  } else {
    blk->p2_row[4][0] = (float)(nb1[3].val+nb1->val-nb2[3].val);
    row2c = nb2;
    blk->p2_row[4][1] = (float)(nb0[-4].val+nb1->val-nb1[-4].val);
    blk->p2_row[4][2] = ((float)((nb2->val+3*nb1[1].val-4*nb2[1].val)-((((nb1[2].val-nb1->val)-(nb3[2].val-nb3->val))>>1)-nb3[1].val)));
    nb4 = (P2Ctx*)blk->cursor[4];
    blk->p2_row[4][3] = (float)(row2c[-1].val+nb1[1].val-nb3->val);
    blk->p2_row[5][0] = (float)(nb3[1].val+nb1->val-nb4[1].val);
    blk->p2_row[5][1] = (float)nb1[3].val;
    blk->p2_row[5][2] = (float)(nb0[-3].val+nb0[-1].val-nb0[-4].val);
    blk->p2_row[5][3] = (float)(nb0[-1].val+nb3->val-nb3[-1].val);
    blk->p2_row[6][0] = (float)(nb0[-5].val+nb1->val-nb1[-5].val);
    blk->p2_row[6][1] = (float)nb4->val;
    blk->p2_row[6][2] = (float)(nb0[-5].val+nb0[-1].val-nb0[-6].val);
    blk->p2_row[6][3] = (float)nb1[-6].val;
    rb2 = (P2Ctx*)(nullptr);
    rb1 = (P2Ctx*)(nullptr);
    rb0 = nullptr;
    ra2 = (P2Ctx*)(nullptr);
    ra1 = (P2Ctx*)(nullptr);
    ra0 = nullptr;
  }
  cur = blk->cur;
  weights.f0[0] = cur[-1];
  up_row = blk->above;
  weights.f0[1] = up_row[1];
  weights.f0[2] = up_row[2];
  weights.f0[3] = cur[-2];
  weights.f0[4] = up_row[0];
  weights.f0[5] = cur[0];
  c_lo = 14*sum_u;
  mode = 1;
  c_mid = 13*sum_ur;
  if( 16*sum4<=14*sum_u ) {
    c_lo = 16*sum4;
    mode = 0;
  }
  if( c_lo>c_mid )
    mode = 2;
  else
    c_mid = c_lo;
  if( c_mid>11*sum_ul )
    mode = 3;
  filt = __alt_p2_filter(blk->nb_cur, (float (*)[4]) blk->p2_row, &weights, mode);
  cx0 = (P2Ctx*)blk->cursor[0];
  cx1 = (P2Ctx*)blk->cursor[1];
  *(int32_t*)&blk->pred_prev = filt;
  lane3 = cx1->aerr;
  g3pair = cx0[-1].aerr+lane3;
  if( refa )
    g3pair += (rb0->aerr+ra0->aerr)>>1;
  run = filt;
  cx3 = (P2Ctx*)(blk->cursor[3]);
  g3sum = cx1[3].aerr+cx1[-1].aerr+cx0[-1].aerr+cx0[-3].aerr+cx0[-4].aerr+cx0[-2].aerr;
  run_s = run;
  ctx0_lo = (cx1[1].err+cx1->err+cx1[-1].err+cx0[-1].err)&0x80000|(cx3->err+cx0[-2].err+2*cx0[-4].err)&0x40000|(cx1[-3].err+cx0[-3].err+cx0[-5].err+cx0[-7].err)&0x20000|cx0[-1].err&0x10000|(uint16_t)cx0[-3].err&0x8000|(((run>3536)+(run>720)+(run>288))<<13)|((((uint32_t)(752-(g3pair+g3sum))>>31)+((uint32_t)(400-(g3pair+g3sum))>>31)+((uint32_t)(240-(g3pair+g3sum))>>31))<<11);
  blk->bank_ctx[0] = ctx0_lo;
  if( refa ) {
    lane2 = ra0->err;
    run = run_s;
    ctx0 = rb0[-1].err&0x2000000|rb0->err&0x1000000|ra0[-1].err&0x800000|(rb0->err+rb0[-2].err)&0x400000|(ra0[-2].err+lane2)&0x200000|cx0[-2].err&0x100000|ctx0_lo;
    cx2 = (P2Ctx*)blk->cursor[2];
  } else {
    run = run_s;
    cx2 = (P2Ctx*)blk->cursor[2];
    ctx0 = (cx1[3].err+cx0[-3].err+cx0[-7].err+cx0[-5].err)&0x2000000|(cx2[1].err+cx2->err+cx0[-4].err+cx0[-8].err)&0x1000000|(cx0[-4].err+cx0[-6].err)&0x800000|cx0[-4].err&0x400000|cx0[-5].err&0x200000|cx0[-7].err&0x100000|ctx0_lo;
  }
  bank0 = ctx0>>11;
  blk->bank_ctx[0] = bank0;
  pred0 = p2_pred(blk->p2_ctr[bank0].weighted, blk->p2_ctr[bank0].rate);
  sum_c = sum_all;
  cx4 = blk->cursor[4];
  blk->nb_sum[1] = pred0;
  run0 = pred0+run_s;
  blk->nb_sum[0] = run0;
  nb4_4 = cx4[4].val;
  sum_all = ((g3pair<<9)+sum_c)>>13;
  d_run0 = cx0[-5].val-run0;
  q24 = ((uint32_t)(24-sum_all)>>20)&0xFFFFF800;
  d_up = nb4_4-run0;
  up5 = cx1[5].val;
  d_up5 = up5-run0;
  q39 = ((uint32_t)(39-sum_all)>>20)&0xFFFFF800;
  if( refa ) {
    q10 = ((uint32_t)(10-sum_all)>>20)&0xFFFFF800;
    dv_now = cx0[0].dval;
    q10a = q10;
    ctx1 = (cx0[-1].val+ra0->val-ra0[-1].val-run0)&0x2000000|(ra2->dval+ra1->dval-2*ra0->dval)&0x1000000|((dv_now+cx0[-3].dval-run0-(cx1[2].dval-cx1[5].dval))&0x800000|(dv_now+cx0[-5].dval-run0)&0x400000|(dv_now+cx1->dval+rb1[2].dval-rb2[2].dval-run0)&0x200000|(dv_now+cx0[-1].dval+rb0->dval-rb0[-1].dval-run0)&0x100000|(cx2[-1].dval+dv_now+ra0->dval-ra2[-1].dval-run0)&0x80000|(dv_now+cx1[2].dval+ra0->dval-ra1[2].dval-run0)&0x40000|d_up5&0x20000|d_up&0x10000|d_run0&0x8000|(((run0>2256)+(run0>1056)+(run0>144))<<13)|((((uint32_t)(55-sum_all)>>20)&0xFFFFF800)+q10+q24));
  } else {
    q10 = ((uint32_t)(10-sum_all)>>20)&0xFFFFF800;
    d_run0b = run0-cx0[-3].val;
    ctx1 = (cx1[-5].val-cx1[-2].val+d_run0b)&0x1000000|(d_run0b+cx1[2].val-up5)&0x800000|(cx4[3].val-cx2[2].val+run0-cx2[1].val)&0x400000|(cx4[-1].val-cx3->val+run0-cx1[-1].val)&0x200000|(cx1->val-cx1[2].val+run0-cx0[-2].val)&0x100000| -d_up5&0x80000| -d_up&0x40000|(run0-cx4[2].val)&0x20000|(run0-cx4[-3].val)&0x10000| -d_run0&0x8000|(((run0>2400)+(run0>1024)+(run0>240))<<13)|(q39+q24+(((uint32_t)(11-sum_all)>>20)&0xFFFFF800))|(nb4_4-cx3->val+run0-cx1[4].val)&0x2000000;
    q10a = q10;
  }
  q10 = q10a;
  bank1 = ctx1>>11;
  blk->bank_ctx[1] = bank1;
  rate1 = blk->p2_ctr[bank1+32768].rate;
  w1c = blk->p2_ctr[bank1+32768].weighted;
  one1 = 1<<((rate1+31)&31);
  pred1 = (one1+w1c)>>(rate1&31);
  run1 = pred1+run0;
  blk->nb_sum[2] = run1;
  blk->nb_sum[3] = pred1;
  cx2_val0 = cx2->val;
  cx1_val = cx1->val;
  dvsum2 = cx0[-2].val-run1;
  lap = cx0[-2].val+run1-2*cx0[-1].val;
  dtop2 = cx2_val0-run1;
  if( refa ) {
    dv1 = cx1[1].dval;
    d_ra = ra0->val-run1;
    q10b = q10;
    ctx2 = (cx0[0].dval+cx1[-1].dval-run1-(cx2->dval-dv1))&0x2000000|(cx0[0].dval+cx0[-1].dval-run1-(cx1->dval-dv1))&0x1000000|(rb0->val-run1+cx2_val0-rb2->val)&0x800000|(d_ra+cx2_val0-ra2->val)&0x400000|(d_ra+cx1[-1].val-ra1[-1].val)&0x200000|((cx0[0].dval-cx0[-2].dval+2*cx0[-1].dval-run1)&0x100000|(2*cx1_val-run1-cx2_val0)&0x80000| -lap&0x40000|dtop2&0x20000|dvsum2&0x10000|(208-rb0->val)&0x8000|(((run1>2576)+(run1>1280)+(run1>640))<<13)|((((uint32_t)(33-sum_all)>>31)+((uint32_t)(12-sum_all)>>31)+((uint32_t)(4-sum_all)>>31))<<11));
  } else {
    q10b = q10;
    ctx2 = (run1+3*(cx2_val0-cx1_val)-cx3->val)&0x2000000|(run1+cx4->val-(cx2[2].val+cx2[-2].val))&0x1000000|(cx3[2].val-cx2[-1].val+run1-cx1[3].val)&0x800000|(cx3[1].val-cx1[1].val-dtop2)&0x400000|(cx3->val-cx1_val-dtop2)&0x200000|lap&0x100000|(run1-cx4[3].val)&0x80000|(cx3[1].val-run1)&0x40000|(run1-cx4->val)&0x20000|(run1-cx3[-2].val)&0x10000| -dvsum2&0x8000|(((run1>2464)+(run1>1216)+(run1>688))<<13)|((((uint32_t)(58-sum_all)>>31)+((uint32_t)(25-sum_all)>>31)+((uint32_t)(13-sum_all)>>31))<<11);
  }
  q10 = q10b;
  bank2 = ctx2>>11;
  blk->bank_ctx[2] = bank2;
  pred2 = p2_pred(blk->p2_ctr[bank2+65536].weighted, blk->p2_ctr[bank2+65536].rate);
  blk->nb_sum[5] = pred2;
  run2 = pred2+run1;
  blk->nb_sum[4] = run2;
  run_dv3 = run2-cx0[0].dval;
  q9 = 9-sum_all;
  sum_all = -sum_all;
  bank3 = (int32_t)((3*(cx0[-2].val-cx0[-1].val)+run2-cx0[-3].val)&0x2000000|(cx2[1].dval-((uint32_t)(cx1[1].dval+cx1[2].dval+cx1[-1].dval+cx1->dval)>>1)+run_dv3)&0x1000000|(cx2[-3].val-cx1[-2].val+run2-cx1[-1].val)&0x800000| -(cx2[2].val+run2-2*cx1[1].val)&0x400000|(cx1[-2].dval-cx0[-2].val+run2-cx1->dval)&0x200000|(run2-cx1[3].val)&0x100000|(run_dv3-cx2[1].dval)&0x80000|(run2-cx3->val)&0x40000|(2*run2-cx1->dval-(cx1->val+cx0[0].dval))&0x20000|(run2-cx0[-1].dval-cx0[0].dval)&0x10000|(run2-cx0[-3].val)&0x8000|(((run2>2896)+(run2>1568)+(run2>592))<<13)|((((uint32_t)(sum_all+37)>>31)+((uint32_t)(sum_all+19)>>31)+(q9>>31))<<11))>>11;
  blk->bank_ctx[3] = bank3;
  w3c = blk->p2_ctr[bank3+98304].weighted;
  pred3 = p2_pred(w3c, blk->p2_ctr[bank3+98304].rate);
  blk->nb_sum[7] = pred3;
  run3 = pred3+run2;
  run = run3;
  blk->nb_sum[6] = run3;
  dv_now4 = cx0[0].dval;
  run_dv4 = run3-dv_now4;
  run_up4 = run3+cx3->val;
  bank4 = (int32_t)((run-((uint32_t)(cx1[1].val+cx1[-1].val+2*cx1->val)>>2))&0x2000000|(run_up4-cx0[-2].val-(cx3[2].dval+dv_now4))&0x1000000|(run-2*cx0[-2].dval-(dv_now4-cx0[-4].dval))&0x800000|(run-cx0[-3].dval-dv_now4-(cx1->val-cx1[-3].val))&0x400000|(run_up4-dv_now4-(cx1[1].val+cx2[-1].dval))&0x200000|(cx2[-2].val+run-2*cx1[-1].val)&0x100000|(cx2->dval-2*cx1->dval+run_dv4)&0x80000|((run3-cx0[-1].val)-(cx1->val-cx1[-1].val))&0x40000|(run_dv4-cx1[4].dval)&0x20000|(run_dv4-cx1[-2].dval)&0x10000|(((uint16_t)run3-(uint16_t)cx0[-4].dval-(uint16_t)dv_now4)&0x8000)|((((run3>3056)+(run3>1952)+(run3>368))<<13)|(q39+(((uint32_t)(sum_all+21)>>20)&0xFFFFF800)+q10)))>>11;
  blk->bank_ctx[4] = bank4;
  pred4 = p2_pred(blk->p2_ctr[bank4+131072].weighted, blk->p2_ctr[bank4+131072].rate);
  cx2p = (uint8_t*)cx2;
  blk->nb_sum[9] = pred4;
  run4 = run+pred4;
  blk->nb_sum[8] = run+pred4;
  magsum_s = cx1[5].mag+cx1[4].mag+cx1[-3].mag+cx1[-4].mag+3*(cx1[2].mag+cx2[1].mag)+7*cx1->mag+6*cx1[1].mag+cx0[-6].mag+cx0[-7].mag+cx0[-8].mag+8*cx0[-1].mag+cx4[2].mag+cx4[1].mag+cx4->mag+cx4[-1].mag+cx0[-4].mag+cx0[-5].mag+(cx3[2].mag+cx3[1].mag+cx3[-1].mag+cx3[-2].mag+cx4[-2].mag+cx2p[107]+cx2p[89]+cx2p[71]+(*(cx2p-19))+*(cx2p-37))+4*(cx1[-1].mag+cx0[-2].mag+cx2p[17])+2*(cx1[-2].mag+cx1[3].mag+cx0[-3].mag+cx3->mag+*(cx2p-1)+cx2p[53]);
  band_lo = blk->band_lo;
  blk->ctx_w[0].sel = (run4<1840)+(run4<272);
  band_hi = blk->band_hi;
  blk->ctx_w[1].sel = ((run4-cx1->val<=band_hi)+(run4-cx1->val<band_lo));
  d_run4 = run4-cx0[-1].val;
  in_band = d_run4<=band_hi;
  blk->ctx_w[2].sel = in_band+(d_run4<band_lo);
  blk->ctx_w[3].sel = cx1->sign;
  blk->ctx_w[4].sel = cx0[-1].sign;
  magsum = magsum_s;
  magu = (cx2[0].mag+cx1->mag);
  run_s2 = run;
  magl = cx0[-1].mag+cx0[-2].mag;
  if( refa ) {
    mag = rb0->mag;
    mag_ref1 = rb1->mag+ra1->mag;
    mag_ref0 = mag+ra0->mag;
    magsum = mag_ref1+magsum_s+4*mag_ref0+2*(ra0[-1].mag+rb0[-1].mag);
    flat_a = mag_ref0+magl+rb0[-2].mag+rb0[-1].mag+ra0[-2].mag+ra0[-1].mag;
    run_s2 = run;
    if( blk->has_ref ) {
      d_run4b = run4-cx1->dval-cx0[0].dval;
      if( d_run4b<band_lo||d_run4b>band_hi ) {
        run4 = run4-cx0[-1].dval-cx0[0].dval;
        flat_b = run4>=band_lo&&band_hi>=run4;
      } else {
        flat_b = 1;
      }
    } else {
      flat_b = mag_ref1+mag_ref0+magu+rb2->mag+ra2->mag;
    }
    if( blk->plane_idx==1 ) {
      magsum_s = magsum;
      d_ra_ra1 = ra0->val-ra1->val;
      blk->ctx_w[3].sel = (d_ra_ra1<=band_hi)+(d_ra_ra1<band_lo);
      d_ra_left = ra0->val-ra0[-1].val;
      magsum = magsum_s;
      run_s2 = run;
      blk->ctx_w[4].sel = ((d_ra_left<=band_hi)+(d_ra_left<band_lo));
    } else if( *(int32_t*)&blk->plane_idx>1 ) {
      magsum_s = magsum;
      d_rb_rb1 = rb0->val-rb1->val;
      blk->ctx_w[3].sel = (d_rb_rb1<=band_hi)+(d_rb_rb1<band_lo);
      d_rb_left = rb0->val-rb0[-1].val;
      in_band = d_rb_left<=band_hi;
      magsum = magsum_s;
      blk->ctx_w[4].sel = in_band+(d_rb_left<band_lo);
      ra0_val = ra0->val;
      d_ra_ra1b = ra0_val-((P2Ctx*)(ra1))->val;
      in_band = band_lo<=d_ra_ra1b;
      run_s2 = run;
      if( in_band&&d_ra_ra1b<=band_hi ) {
        flat_a = 1;
      } else {
        d_ra_leftb = ra0_val-ra0[-1].val;
        flat_a = d_ra_leftb>=band_lo&&d_ra_leftb<=band_hi;
      }
    }
  } else {
    flat_a = magl+cx0[-3].mag+cx0[-4].mag+cx0[-5].mag;
    flat_b = cx4->mag+cx3->mag+magu+cx0[0].mag;
  }
  if( magsum>=960 ) {
    ctx15 = 15;
    blk->ctx = 15;
  } else {
    ctx15 = blk->nb_ctx[magsum>>3];
    blk->ctx = ctx15;
  }
  ctx_idx = (run_s2+pred4+7)>>4;
  if( ctx_idx>=255 )
    ctx_idx = 255;
  if( ctx_idx<0 )
    ctx_idx = 0;
  blk->ctx_pair[0] = blk->ctx_delta[ctx_idx+4]+ctx15;
  blk->ctx_pair[1] = ctx15+blk->ctx_delta[ctx_idx];
  blk->ctx = ctx15+32*(flat_b==0)+16*(flat_a==0)+blk->ctx_w[4].w[blk->ctx_w[4].sel]+blk->ctx_w[3].w[blk->ctx_w[3].sel]+blk->ctx_w[2].w[blk->ctx_w[2].sel]+blk->ctx_w[1].w[blk->ctx_w[1].sel]+blk->ctx_w[0].w[blk->ctx_w[0].sel];
  return ctx_idx;
}

void __reduce_alphabet(ModelBlock* blk, int8_t unread_flag, uint8_t* src) {
  struct alignas(16) ReduceAlphabetFrame {
    uint32_t zero_base[15];
    void* tmp;
    int32_t kids_i;
    uint8_t buf[4];
    uint32_t*sym_flag() {
      return (uint32_t*)&kids_i;
    }

    uint64_t kids[127];
    int32_t y_spill;
    int32_t at_spill;
    int32_t bits_spill;
    uint8_t _pad0[64504];
    SymList lists[16];
    union {
      void* slot[19];
      struct {
        int32_t slot0;
        BMF_SPILL_PAD(0);
        void* slot1;
        uint8_t* slot2;
        uint32_t slot3;
        BMF_SPILL_PAD(3);
        uint32_t slot4;
        BMF_SPILL_PAD(4);
        void* slot5;
        uint32_t slot6;
        BMF_SPILL_PAD(6);
        ModelBlock* slot7;
        uint8_t* slot8;
        uint32_t slot9;
        BMF_SPILL_PAD(9);
        void* slot10;
        ModelBlock* slot11;
        void* slot_tail[7];
      };
    };
  } __frame;
  ModelBlock* blk2;
  int8_t mode;
  ModelBlock* blk1, * blk4;
  uint32_t alpha_n, done, done2, off, slot_a, sym, depth_bits;
  bool more, packed, first;
  uint8_t* half;
  int32_t node, side, alpha_m, carry, img_w, img_h, n_moved, height, n_distinct, row_w, y, at, bits, bpp, shift, sym2, alphabet, alpha, prev, s, s_next;
  ModelBlock* blk3;
  uint32_t n_kids, i, written, li, si, si_end, word, k, pairs, n_pairs, j, x, idx, n_syms, n_syms3, next_id, m;
  uint16_t* kidp;
  uint8_t* srcp, * p, * rp, * dst_a, * dst_b, * dst_c, * q;
  uint32_t val;
  void* newbuf;
  __frame.slot11 = (ModelBlock*)(blk);
  srcp = src;
  depth_bits = blk->depth;
  __frame.slot2 = src;
  __frame.slot7 = (ModelBlock*)(blk);
  __frame.slot4 = 0xFFFFFFFF>>(-depth_bits&31);
  n_kids = (depth_bits+7)>>3;
  for( i = 0; i<8; ++i ) {
    __frame.lists[2*i].ent = nullptr;
    __frame.lists[2*i+1].ent = nullptr;
  }
  blk1 = __frame.slot7;
  if( depth_bits<=8 ) {
    memset(__frame.sym_flag(), 0, 256*sizeof(uint32_t));
    packed = blk1->depth<8;
    height = blk1->height;
    blk1->alphabet = 0;
    if( packed ) {
      n_distinct = 0;
      if( height ) {
        row_w = blk1->width;
        __frame.slot8 = src-1;
        __frame.at_spill = 0;
        y = 0;
        do {
          if( !row_w )
            break;
          __frame.y_spill = y;
          x = 0;
          at = __frame.at_spill;
          bits = 0;
          do {
            bpp = blk1->depth;
            shift = bits-bpp;
            if( shift<0 ) {
              ++__frame.slot8;
              shift = 8-bpp;
            }
            sym = __frame.slot4&(*__frame.slot8>>(shift&31));
            first = __frame.sym_flag()[sym]==0;
            __frame.bits_spill = shift;
            ++x;
            __frame.sym_flag()[sym] = 1;
            bits = __frame.bits_spill;
            blk1->alphabet += first;
            blk1->sym_word[at] = sym;
            row_w = blk1->width;
            ++at;
          } while( x<(uint32_t)(*(int32_t*)&blk1->width) );
          n_distinct = blk1->alphabet;
          __frame.at_spill = at;
          y = __frame.y_spill+1;
        } while( (uint32_t)(__frame.y_spill+1)<(uint32_t)blk1->height );
      }
    } else if( height**(int32_t*)&blk1->width ) {
      q = __frame.slot2;
      idx = 0;
      do {
        blk1->alphabet += __frame.sym_flag()[*q]==0;
        sym2 = *q;
        __frame.sym_flag()[sym2] = 1;
        ++q;
        blk1->sym_word[idx++] = sym2;
      } while( idx<(uint32_t)(blk1->height**(int32_t*)&blk1->width) );
      n_distinct = blk1->alphabet;
    } else {
      n_distinct = 0;
    }
    rc.encode(n_distinct-1, n_distinct, __frame.slot4+1);
    n_syms = blk1->alphabet;
    if( n_syms<=__frame.slot4 ) {
      __init_symbol_list(&__frame.lists[0], __frame.slot4-n_syms+2, 1);
      __frame.lists[0].rescale_at = 19*__frame.lists[0].n;
      n_syms3 = blk1->alphabet;
      if( n_syms3 ) {
        prev = 0;
        s = 0;
        next_id = 0;
        do {
          if( __frame.sym_flag()[s] ) {
            __frame.lists[0].code_symbol(s-prev);
            n_syms3 = blk1->alphabet;
            __frame.sym_flag()[s] = next_id;
            s_next = s+1;
            prev = s+1;
            ++next_id;
          } else {
            s_next = s+1;
          }
          s = s_next;
        } while( next_id<n_syms3 );
      }
      if( blk1->height**(int32_t*)&blk1->width ) {
        m = 0;
        do {
          blk1->sym_word[m] = __frame.sym_flag()[blk1->sym_word[m]];
          ++m;
        } while( m<(uint32_t)(blk1->height**(int32_t*)&blk1->width) );
      }
      free_sym_entries(__frame.lists, 16);
    } else {
      free_sym_entries(__frame.lists, 16);
    }
  } else {
    memset(__frame.buf, 0, 0x10000);
    blk1->alphabet = 1;
    *(uint32_t*)__frame.buf = __frame.slot4&*(uint32_t*)src;
    blk1->sym_word[0] = 0;
    if( (uint32_t)(blk1->height**(int32_t*)&blk1->width)>1 ) {
      __frame.slot8 = src;
      __frame.slot9 = n_kids;
      __frame.slot7 = (ModelBlock*)(blk1);
      p = __frame.slot2;
      node = 0;
      written = 1;
      while( 1 ) {
        p += __frame.slot9;
        val = __frame.slot4&*(uint32_t*)p;
        if( val!=*(uint32_t*)&__frame.buf[8*node] ) {
          node = 0;
          if( val!=*(uint32_t*)__frame.buf ) {
            __frame.slot3 = written;
            __frame.slot2 = p;
            while( 1 ) {
              side = *(uint32_t*)&__frame.buf[8*node]<val;
              kidp = (uint16_t*)&__frame.kids[node];
              node = kidp[side];
              if( !kidp[side] )
                break;
              if( val==*(uint32_t*)&__frame.buf[8*node] ) {
                written = __frame.slot3;
                p = __frame.slot2;
                mode_symbol[1] = side;
                goto LABEL_12;
              }
            }
            p = __frame.slot2;
            __frame.slot0 = side;
            blk2 = (ModelBlock*)(__frame.slot7);
            alphabet = __frame.slot7->alphabet;
            mode_symbol[1] = side;
            node = (uint16_t)alphabet;
            alpha = alphabet+1;
            kidp[side] = node;
            written = __frame.slot3;
            blk2->alphabet = alpha;
            if( alpha>0x2000 ) {
              srcp = __frame.slot8;
              alpha_n = alpha;
              n_kids = __frame.slot9;
              blk1 = __frame.slot7;
              goto LABEL_14;
            }
            *(uint32_t*)&__frame.buf[8*node] = val;
          }
        }
LABEL_12:
        blk3 = __frame.slot7;
        __frame.slot7->sym_word[written++] = node;
        if( written>=blk3->height*blk3->width ) {
          srcp = __frame.slot8;
          n_kids = __frame.slot9;
          blk1 = __frame.slot7;
          alpha_n = __frame.slot7->alphabet;
          goto LABEL_14;
        }
      }
    }
    alpha_n = blk1->alphabet;
LABEL_14:
    rc.encode(alpha_n-1, alpha_n, 0x2001u);
    alpha_m = blk1->alphabet;
    if( alpha_m>0x2000 ) {
      (__frame.slot[1]) = bmf_new(blk1->height*n_kids*blk1->width);
      img_w = blk1->width;
      img_h = blk1->height;
      __frame.kids_i = blk1->width;
      __frame.slot0 = img_h;
      if( n_kids ) {
        __frame.slot6 = __frame.slot0*img_w;
        if( n_kids>>1 ) {
          half = (uint8_t*)(__frame.slot[1])+__frame.slot0*__frame.kids_i;
          __frame.slot8 = srcp;
          __frame.slot9 = n_kids;
          __frame.slot7 = (ModelBlock*)(blk1);
          pairs = 0;
          do {
            slot_a = 2*pairs;
            off = 2*pairs++*__frame.slot6;
            __frame.slot[slot_a+2] = (uint8_t*)(__frame.slot[1])+off;
            __frame.slot[slot_a+3] = (void*)&half[off];
          } while( pairs<n_kids>>1 );
          srcp = __frame.slot8;
          n_kids = __frame.slot9;
          blk1 = __frame.slot7;
          done = 2*pairs+1;
        } else {
          done = 1;
        }
        if( n_kids>(done-1) )
          __frame.slot[done+1] = (uint8_t*)(__frame.slot[1])+__frame.slot0* -__frame.kids_i+__frame.slot6*done;
      } else {
        __frame.slot6 = __frame.slot0*img_w;
      }
      if( __frame.slot6 ) {
        rp = src;
        if( n_kids ) {
          __frame.slot9 = n_kids;
          __frame.slot7 = (ModelBlock*)(blk1);
          __frame.kids_i = 0;
          n_moved = 0;
          n_pairs = n_kids>>1;
          while( 1 ) {
            while( 1 ) {
              if( n_pairs ) {
                for( j = 0; j<n_pairs; ++j ) {
                  dst_a = (uint8_t*)(__frame.slot[2*j+2]);
                  *dst_a = rp[2*j];
                  dst_b = (uint8_t*)__frame.slot[2*j+3];
                  __frame.slot[2*j+2] = dst_a+1;
                  *dst_b = rp[2*j+1];
                  __frame.slot[2*j+3] = dst_b+1;
                }
                done2 = 2*j+1;
                srcp = &rp[2*j];
              } else {
                done2 = 1;
              }
              if( (done2-1)>=__frame.slot9 )
                break;
              dst_c = (uint8_t*)__frame.slot[done2+1];
              srcp = &rp[done2];
              *dst_c = rp[done2-1];
              more = (uint32_t)(++n_moved)<__frame.slot6;
              __frame.slot[done2+1] = dst_c+1;
              if( !more )
                goto LABEL_71;
              rp += done2;
            }
            if( (uint32_t)(++n_moved)>=__frame.slot6 )
              break;
            rp = srcp;
          }
LABEL_71:
          n_kids = __frame.slot9;
          blk1 = __frame.slot7;
        }
      }
      __frame.tmp = blk1->sym_word;
      blk1->height = n_kids*__frame.slot0;
      blk1->depth = 8;
      free(__frame.tmp);
      newbuf = bmf_new(2*blk1->height*blk1->width);
      __frame.tmp = (__frame.slot[1]);
      blk1->sym_word = (uint16_t*)newbuf;
      __reduce_alphabet((ModelBlock*)blk1, mode, (uint8_t*)__frame.tmp);
      free((__frame.slot[1]));
    } else {
      if( 4*n_kids ) {
        __frame.slot7 = (ModelBlock*)(blk1);
        li = 0;
        do {
          __init_symbol_list(&__frame.lists[li], 256, 1);
          ++li;
        } while( li<4*n_kids );
        blk1 = __frame.slot7;
        alpha_m = __frame.slot7->alphabet;
      }
      if( alpha_m ) {
        carry = 0;
        si = 0;
        si_end = alpha_m;
        blk4 = (ModelBlock*)(blk1);
        do {
          word = *(uint32_t*)&__frame.buf[8*si];
          if( n_kids ) {
            __frame.kids_i = si;
            __frame.slot9 = n_kids;
            __frame.slot7 = (ModelBlock*)(blk4);
            for( k = 0; k<__frame.slot9; ++k ) {
              __frame.lists[4*k+carry].code_symbol((uint8_t)word);
              carry = (uint8_t)word>>6;
              word >>= 8;
            }
            si = __frame.kids_i;
            n_kids = __frame.slot9;
            blk4 = (ModelBlock*)(__frame.slot7);
            word = *(uint32_t*)&__frame.buf[8*__frame.kids_i];
            si_end = __frame.slot7->alphabet;
          }
          carry = (uint8_t)word>>7;
          ++si;
        } while( si<si_end );
      }
    }
    free_sym_entries(__frame.lists, 16);
  }
}

int32_t __cost_candidate(uint8_t* img, int32_t cand, uint8_t* desc, int8_t unread4, int32_t unread5, int32_t unread6, int32_t unread7, uint32_t* costs) {
  struct alignas(16) CostCandidateFrame {
    uint8_t buf[4096];
    int32_t hist_y[1024];
    int32_t hist_yx[1024];
    int32_t hist_zx[1024];
    int32_t hist_zy[1024];
    int32_t hist_zp[1024];
    uint8_t buf_1[4];
    uint8_t* img_end;
    int32_t step_d;
    int32_t off_b0;
    int32_t off_b1;
    int32_t off_a0;
    int32_t off_a1;
    int32_t nplanes;
    int32_t off_a2;
    int32_t off_b2;
    uint8_t _pad0[2008];
    int32_t cand_i;
    uint8_t* desc_f;
    BmfImage* img_f;
    int32_t rec_off;
    int32_t _pad91;
    int32_t d1_f;
    int32_t best;
    int32_t rows;
    int32_t d2_f;
    int32_t wb;
    int32_t wa;
    uint8_t* r1_f;
    uint8_t* r0_f;
    uint8_t* cursor;
    int32_t off_up;
    uint8_t _pad1[28];
  } __frame;
  int32_t cost;
  int32_t off;
  uint8_t* descp;
  bool deep;
  uint32_t row_b2, swap;
  double syz, syy, sxz, sxy, sxx, inv, w1f, w2f;
  int32_t row_b, d1, d2, dx, o1, dy, o2, diag, west, dz, bin, w1, w2, img_w, nstep, e0, e1, e2, left, bin2, pick, c0, rec, c2, lo1, s1, s2, c2b, tmp, w2s, w1s;
  uint32_t lo2, lo3;
  uint8_t* base, * p, * q, * r0, * r2, * r1;
  int32_t cand2, idx1, idx2;
  __frame.desc_f = desc;
  __frame.nplanes = plane_count;
  row_b = *(const uint16_t*)&((const BmfImage*)img)->stride;
  __frame.cand_i = cand;
  __frame.img_f = (BmfImage*)img;
  __frame.d1_f = (cand+1)%3-cand;
  __frame.d2_f = (cand+2)%3-cand;
  __frame.img_end = &__frame.img_f->pixels[__frame.img_f->data_size];
  d1 = __frame.d1_f;
  *(uint32_t*)__frame.buf_1 = row_b;
  memset(__frame.buf, 0, 24576);
  row_b2 = *(uint32_t*)__frame.buf_1;
  cand2 = __frame.cand_i;
  base = __frame.img_f->pixels;
  syz = 0;
  syy = 0.0;
  sxz = 0.0;
  sxy = 0.0;
  sxx = 0.0;
  __frame.rec_off = 16*__frame.cand_i;
  __frame.desc_f[16*__frame.cand_i] = 2;
  __frame.desc_f[33] = (uint8_t)cand2;
  p = &base[cand2+row_b2+__frame.nplanes];
  if( p<__frame.img_end ) {
    __frame.d1_f = d1;
    *(uint32_t*)__frame.buf_1 = row_b2;
    __frame.off_a2 = d1-row_b2;
    __frame.off_a1 = d1-row_b2-__frame.nplanes;
    __frame.off_a0 = d1-__frame.nplanes;
    __frame.off_b2 = __frame.d2_f-row_b2;
    __frame.off_b1 = __frame.d2_f-row_b2-__frame.nplanes;
    __frame.off_b0 = __frame.d2_f-__frame.nplanes;
    __frame.step_d = row_b2+__frame.nplanes;
    do {
      d2 = __frame.d2_f;
      dx = p[__frame.off_a1]+p[__frame.d1_f]-(p[__frame.off_a2]+p[__frame.off_a0]);
      o1 = __frame.off_b1;
      ++*(uint32_t*)&__frame.buf[4*dx+2048];
      dy = p[o1]+p[d2]-(p[__frame.off_b2]+p[__frame.off_b0]);
      o2 = __frame.step_d;
      sxx = sxx+(double)dx*(double)dx;
      ++__frame.hist_y[dy+512];
      syy = syy+(double)dy*(double)dy;
      sxy = sxy+(double)dx*(double)dy;
      ++__frame.hist_yx[((uint16_t)dy-(uint16_t)dx-512)&0x3FF];
      diag = p[-o2]+*p;
      west = p[-__frame.nplanes];
      q = p-*(uint32_t*)__frame.buf_1;
      p += __frame.nplanes;
      dz = diag-(*q+west);
      sxz = sxz+(double)dx*(double)dz;
      ++__frame.hist_zx[((uint16_t)dz-(uint16_t)dx-512)&0x3FF];
      syz = syz+(double)dy*(double)dz;
      ++__frame.hist_zy[((uint16_t)dz-(uint16_t)dy-512)&0x3FF];
      bin = ((uint16_t)dz-(uint16_t)((uint32_t)(((dx+dy)<<6)+40)>>7)-512)&0x3FF;
      ++__frame.hist_zp[bin];
    } while( p<__frame.img_end );
    d1 = __frame.d1_f;
  }
  inv = 128.0/(0.1-sxy*sxy+sxx*syy);
  w1f = (syy*sxz-sxy*syz)*inv;
  w2f = inv*(sxx*syz-sxy*sxz);
  w1 = (int32_t)w1f;
  if( (int32_t)w1f>=191 )
    w1 = 191;
  if( w1<-64 )
    w1 = -64;
  __frame.wa = w1;
  w2 = (int32_t)w2f;
  if( (int32_t)w2f>=191 )
    w2 = 191;
  if( w2<-64 )
    w2 = -64;
  __frame.wb = w2;
  memset(__frame.buf_1, 0, 2048);
  img_w = (uint16_t)__frame.img_f->stride;
  __frame.d1_f = d1;
  __frame.rows = ((__frame.img_f->height-1)*__frame.img_f->width)-1;
  __frame.best = -img_w;
  nstep = -plane_count;
  __frame.off_up = -img_w-plane_count;
  r0 = &base[cand2+img_w+plane_count];
  __frame.r0_f = r0;
  r1 = r0+d1;
  __frame.r1_f = r1;
  r2 = r0+__frame.d2_f;
  __frame.cursor = r2;
  do {
    e0 = r0[__frame.off_up];
    __frame.r1_f = r1;
    __frame.r0_f = r0;
    __frame.cursor = r2;
    e1 = e0+*r0-r0[__frame.best]-r0[nstep];
    e2 = *r1;
    left = __frame.rows;
    r2 = &__frame.cursor[-nstep];
    bin2 = ((uint16_t)e1-(uint16_t)((__frame.wa*(r1[__frame.off_up]+e2-r1[__frame.best]-r1[nstep])+__frame.wb*(__frame.cursor[__frame.off_up]+*__frame.cursor-__frame.cursor[__frame.best]-(uint32_t)__frame.cursor[nstep])+40)>>7)-256)&0x1FF;
    r1 -= nstep;
    ++*(uint32_t*)&__frame.buf_1[4*bin2];
    r0 = &__frame.r0_f[-nstep];
    __frame.rows = left-1;
  } while( left!=1 );
  pick = __frame.d1_f;
  c0 = __estimate_cost((uint8_t*)__frame.buf_1, 512);
  rec = 4*__frame.cand_i;
  costs[rec] = c0;
  costs[rec+1] = __estimate_cost((uint8_t*)__frame.hist_zx, 1024);
  costs[rec+2] = __estimate_cost((uint8_t*)__frame.hist_zy, 1024);
  costs[rec+3] = __estimate_cost((uint8_t*)__frame.hist_zp, 1024);
  cost = __estimate_cost((uint8_t*)__frame.buf, 1024);
  __frame.d1_f = __estimate_cost((uint8_t*)__frame.hist_y, 1024);
  c2 = __estimate_cost((uint8_t*)__frame.hist_yx, 1024);
  __frame.rows = c2;
  lo1 = c2;
  if( cost<c2 )
    lo1 = cost;
  if( __frame.d1_f<c2 )
    c2 = __frame.d1_f;
  s1 = __frame.d1_f+lo1;
  s2 = cost+c2;
  __frame.best = s2;
  c2b = __frame.rows;
  if( (s1<s2) ) {
    __frame.best = s1;
    tmp = pick;
    pick = __frame.d2_f;
    cost = __frame.d1_f;
    __frame.d2_f = tmp;
    swap = costs[rec+1];
    costs[rec+1] = costs[rec+2];
    w2s = __frame.wb;
    costs[rec+2] = swap;
    w1s = __frame.wa;
    __frame.wa = w2s;
    __frame.wb = w1s;
  }
  off = __frame.rec_off;
  descp = __frame.desc_f;
  *(uint32_t*)&__frame.desc_f[__frame.rec_off+4] = __frame.wa;
  *(uint32_t*)&descp[off+8] = __frame.wb;
  idx1 = __frame.cand_i+pick;
  idx2 = __frame.cand_i+__frame.d2_f;
  descp[16*idx1] = 0;
  descp[1] = (uint8_t)idx1;
  deep = 0;
  descp[16*idx2] = 1;
  descp[17] = (uint8_t)idx2;
  if( !deep&&__frame.img_f->data_size>0x1000000u )
    return cost+c2b+costs[rec];
  lo2 = costs[rec];
  lo3 = costs[rec+2];
  if( lo2>=costs[rec+1] )
    lo2 = costs[rec+1];
  if( lo3>=costs[rec+3] )
    lo3 = costs[rec+3];
  if( lo2<lo3 )
    lo3 = lo2;
  return __frame.best+lo3;
}

int32_t __choose_plane_coding(BmfImage* img, int32_t unused_h, int8_t unused_c) {
  struct alignas(16) ChoosePlaneCodingFrame {
    uint8_t _pad0[4];
    union {
      uint8_t buf[32768];
      struct {
        uint8_t _buf_head[4096];
        int32_t hist_a[1024];
        int32_t hist_b[1024];
        int32_t hist_c[5120];
      };
    };
    union {
      uint8_t buf_1[2048];
      struct {
        uint8_t _buf_1_head[4];
        int32_t s0;
        int32_t s1;
        int32_t s2;
        uint8_t* p0;
        uint8_t* p1;
        int32_t s3;
        int32_t s4;
        uint8_t* p2;
        int32_t s5;
        int32_t s6;
        int32_t s7;
        uint8_t* p3;
        uint8_t* p4;
        int32_t s8;
        int32_t s9;
        uint8_t* p5;
        int32_t s10;
        uint8_t _pad3[1976];
      };
    };
    union {
      int64_t q0;
      struct {
        int32_t row_stride;
        int32_t plane_b;
      };
    };
    union {
      int64_t q1;
      struct {
        int32_t plane_a;
        uint32_t nplanes_s;
      };
    };
    union {
      double d0;
      uint32_t wt_slot;
    };
    uint8_t _pad6[4];
    uint8_t _pad5[4];
    uint8_t _pad7[28];
  } __frame;
  int32_t best0, best1, best2;
  uint8_t buf_3[2048], buf_4[2048], buf_2[2048];
  alignas(16) int32_t x0[4], x1[4], x2[4], x3[4], x4[4], x5[4];
  double d1, d2, d3;
  alignas(16) uint8_t scratch[256];
  int32_t(&acc0)[4] = *(int32_t(*)[4])&scratch[0];
  uint64_t(&acc1)[5] = *(uint64_t(*)[5])&scratch[16];
  int32_t(&tbl16)[16] = *(int32_t(*)[16])&scratch[56];
  double d4;
  uint8_t tbl64a[64], tbl64b[64];
  int64_t q2;
  double d5, d6;
  uint8_t* row_c, * row_d;
  uint8_t* plane0;
  int32_t wt_step;
  uint8_t* pp;
  uint32_t uu;
  int32_t wa_slot;
  BmfImage* img_a;
  const uint8_t* data_end;
  bool keep0, cheaper, keep3, pick0;
  int8_t unread0, unread1, unread2, pos, pos3;
  uint8_t pl_even, pl_odd;
  uint8_t* hist;
  double sum22, sum11, dv0, dv1, dv2, sum01, sum02, sum12, sum02_c, inv;
  int16_t g1_lo;
  uint32_t n_quads, next_plane, row, slack;
  uint16_t g1w;
  int32_t n_planes, data_size, result, pick01, xform, dw, * win_row, wt8, wt4, c2, c2w, c1, c1w, c0, bin0, pred, xform_row, win, sum, best_sum, i, pos2, best_sum2, sum2, wt4_dn, wt8_dn, wt4_dn_end, wt8_dn_end, dv3, wa, wb, wc, nx0, dg0, dn0, ad0, dnx1, g0, dn1, nx2, g1, dnx2, g2, pa, pb, bin_lin, quad_r, alpha, bin_lin2, r0, r1, r2, wa_pick, pred4, pred4b, * hist2, sum3, best_sum3, npix_c, stride_c, left_c, ul_c, g_c, cur_c, n_c, bin_c, wt4_best_dn, npix_d, stride_d, left_d, ul_d, g_d, cur_d, n_d, bin_d, wt8_best_dn, wt4_up, wt8_up, wt4_up_end, wt8_up_end, npix_a, stride_a, left_a, ul_a, ofs_ul_a, ofs_up_a, ofs_ul_b, ofs_up_b, g_a, cur_a, n_a, bin_a, wt4_best, npix_b, stride_b, left_b, ul_b, g_b, cur_b, n_b, bin_b, wt8_best;
  uint32_t pair, cost0, cost1, best_cost, best, cost_flat, i4, quad, cost_lin, best4, cost_c0, cost_c1, cost_c2, cost_c, cost_d, cost_a, cost_b;
  uint8_t* px, * end_px, * q_c, * r_c, * p_c, * q_d, * r_d, * p_d, * q_a, * p_a, * q_b, * p_b;
  n_planes = plane_count;
  data_size = img->data_size;
  img_a = (BmfImage*)((uint8_t*)img);
  alphabet_reduced = 1;
  __frame.row_stride = ((uint16_t)img->stride);
  data_end = &img->pixels[data_size];
  memset(__frame.buf, 0, 0x8000);
  result = 192;
  do {
    bmf_zero16(&scratch[result+48]);
    bmf_zero16(&scratch[result+32]);
    bmf_zero16(&scratch[result+16]);
    bmf_zero16(&scratch[result]);
    result -= 64;
  } while( result );
  if( n_planes>0 ) {
    if( n_planes/2 ) {
      pair = 0;
      do {
        pl_even = 2*pair;
        row = 2*pair;
        plane_desc[row+1].src_plane = 2*pair;
        pl_odd = 2*pair+++1;
        plane_desc[row+1].nrefs = pl_even;
        plane_desc[row+2].src_plane = pl_odd;
        plane_desc[row+2].nrefs = pl_odd;
      } while( pair<(uint32_t)(n_planes/2) );
      next_plane = 2*pair+1;
    } else {
      next_plane = 1;
    }
    result = next_plane-1;
    if( (uint32_t)n_planes>(next_plane-1) ) {
      plane_desc[next_plane].src_plane = result;
      plane_desc[next_plane].nrefs = result;
    }
    if( n_planes>=3 ) {
      cost0 = __cost_candidate((uint8_t*)img_a, 0, (uint8_t*)tbl16, unread0, best0, best1, best2, (uint32_t*)&acc0[2]);
      cost1 = __cost_candidate((uint8_t*)img_a, 1, tbl64a, unread1, best0, best1, best2, (uint32_t*)&acc0[2]);
      keep0 = cost1>=cost0;
      if( cost1>=cost0 )
        cost1 = cost0;
      pick01 = !keep0;
      x3[2] = pick01;
      cheaper = (uint32_t)(__cost_candidate((uint8_t*)img_a, 2, tbl64b, unread2, best0, best1, best2, (uint32_t*)&acc0[2]))<cost1;
      xform = pick01;
      if( cheaper )
        xform = 2;
      x3[2] = xform;
      dw = 16;
      win_row = &tbl16[16*xform];
      do {
        *(uint64_t*)(bmf_plane_desc(dw*4-8)) = *(uint64_t*)&win_row[dw-2];
        *(uint64_t*)(bmf_plane_desc(dw*4-16)) = *(uint64_t*)&win_row[dw-4];
        *(uint64_t*)(bmf_plane_desc(dw*4-24)) = *(uint64_t*)&win_row[dw-6];
        *(uint64_t*)(bmf_plane_desc(dw*4-32)) = *(uint64_t*)&win_row[dw-8];
        dw -= 8;
      } while( dw*4 );
      best_cost = acc0[4*x3[2]+2];
      x3[1] = 16*x3[2];
      __frame.plane_a = plane_desc[1].src_plane-x3[2];
      wt8 = plane_desc[x3[2]+1].weight1;
      __frame.plane_b = plane_desc[2].src_plane-x3[2];
      wt4 = plane_desc[x3[2]+1].weight0;
      {
        __frame.wt_slot = wt8;
        x5[3] = wt4;
        __frame.nplanes_s = n_planes;
        wt_step = 4;
        __frame.s1 = wt4-1;
        wt4_dn = wt4-1;
        __frame.s6 = wt8-1;
        wt8_dn = wt8-1;
        __frame.s0 = wt4-wt_step;
        wt4_dn_end = __frame.s0;
        *(uint32_t*)__frame.buf_1 = wt8-wt_step;
        plane0 = &img_a->pixels[x3[2]];
        wt8_dn_end = wt8-wt_step;
        while( 1 ) {
          if( wt4_dn<=wt4_dn_end ) {
            if( wt8_dn<=wt8_dn_end ) {
              x1[0] = x5[3]+1;
              wt4_up = x5[3]+1;
              x3[3] = __frame.wt_slot+1;
              wt8_up = __frame.wt_slot+1;
              x0[3] = x5[3]+wt_step;
              wt4_up_end = x5[3]+wt_step;
              x0[2] = __frame.wt_slot+wt_step;
              wt8_up_end = __frame.wt_slot+wt_step;
              while( 1 ) {
                if( wt4_up>=wt4_up_end&&wt8_up>=wt8_up_end ) {
                  wt8 = __frame.wt_slot;
                  wt4 = x5[3];
                  n_planes = __frame.nplanes_s;
                  goto LABEL_19;
                }
                if( wt4_up<wt4_up_end&&wt4_up<192 ) {
                  x0[2] = wt8_up_end;
                  memset(__frame.buf_1, 0, 2048);
                  npix_a = img_a->width*(img_a->height-1);
                  stride_a = (uint16_t)img_a->stride;
                  x3[3] = wt8_up;
                  x1[0] = wt4_up;
                  n_a = npix_a-1;
                  ofs_up_a = -stride_a;
                  left_a = -plane_count;
                  ofs_ul_a = -stride_a-plane_count;
                  p_a = plane0-ofs_ul_a;
                  q_a = plane0+__frame.plane_a-ofs_ul_a;
                  row_c = plane0+__frame.plane_b-ofs_ul_a;
                  do {
                    ul_a = p_a[ofs_ul_a];
                    g_a = ul_a+*p_a-p_a[ofs_up_a]-p_a[left_a];
                    cur_a = *q_a;
                    bin_a = ((int16_t)g_a-(uint16_t)((x1[0]*(q_a[ofs_ul_a]+cur_a-q_a[ofs_up_a]-q_a[left_a])+__frame.wt_slot*(row_c[ofs_ul_a]+*row_c-row_c[ofs_up_a]-(uint32_t)row_c[left_a])+40)>>7)-256)&0x1FF;
                    ++*(uint32_t*)&__frame.buf_1[4*bin_a];
                    q_a -= left_a;
                    p_a -= left_a;
                    row_c -= left_a;
                    --n_a;
                  } while( n_a );
                  wt8_up = x3[3];
                  wt4_up = x1[0];
                  cost_a = __estimate_cost((uint8_t*)__frame.buf_1, 512);
                  wt8_up_end = x0[2];
                  wt4_best = x5[3];
                  if( cost_a<best_cost ) {
                    best_cost = cost_a;
                    wt4_best = wt4_up;
                  }
                  x5[3] = wt4_best;
                  wt4_up_end = wt_step+wt4_best;
                }
                if( wt8_up<wt8_up_end&&wt8_up<192 ) {
                  {
                    x0[3] = wt4_up_end;
                    memset(buf_2, 0, 2048);
                    npix_b = img_a->width*(img_a->height-1);
                    stride_b = (uint16_t)img_a->stride;
                    x3[3] = wt8_up;
                    x1[0] = wt4_up;
                    n_b = npix_b-1;
                    ofs_up_b = -stride_b;
                    left_b = -plane_count;
                    ofs_ul_b = -stride_b-plane_count;
                    p_b = plane0-ofs_ul_b;
                    q_b = plane0+__frame.plane_a-ofs_ul_b;
                    row_d = plane0+__frame.plane_b-ofs_ul_b;
                    do {
                      ul_b = p_b[ofs_ul_b];
                      g_b = ul_b+*p_b-p_b[ofs_up_b]-p_b[left_b];
                      cur_b = *q_b;
                      bin_b = ((int16_t)g_b-(uint16_t)((x5[3]*(q_b[ofs_ul_b]+cur_b-q_b[ofs_up_b]-q_b[left_b])+x3[3]*(row_d[ofs_ul_b]+*row_d-row_d[ofs_up_b]-(uint32_t)row_d[left_b])+40)>>7)-256)&0x1FF;
                      ++*(uint32_t*)&buf_2[4*bin_b];
                      q_b -= left_b;
                      p_b -= left_b;
                      row_d -= left_b;
                      --n_b;
                    } while( n_b );
                    wt8_up = x3[3];
                    wt4_up = x1[0];
                    cost_b = __estimate_cost((uint8_t*)buf_2, 512);
                    wt4_up_end = x0[3];
                    wt8_best = __frame.wt_slot;
                    if( cost_b<best_cost ) {
                      best_cost = cost_b;
                      wt8_best = wt8_up;
                    }
                    __frame.wt_slot = wt8_best;
                    wt8_up_end = wt_step+wt8_best;
                  }
                }
                ++wt4_up;
                ++wt8_up;
              }
            }
          }
          if( wt4_dn>wt4_dn_end&&wt4_dn>=-64 ) {
            *(uint32_t*)__frame.buf_1 = wt8_dn_end;
            memset(buf_3, 0, 2048);
            npix_c = img_a->width*(img_a->height-1);
            stride_c = (uint16_t)img_a->stride;
            __frame.s6 = wt8_dn;
            __frame.s1 = wt4_dn;
            __frame.s2 = npix_c-1;
            __frame.s4 = -stride_c;
            left_c = -plane_count;
            __frame.s3 = -stride_c-plane_count;
            __frame.p1 = plane0-__frame.s3;
            __frame.p0 = plane0+__frame.plane_a-__frame.s3;
            q_c = __frame.p0;
            r_c = plane0+__frame.plane_b-__frame.s3;
            p_c = __frame.p1;
            do {
              ul_c = p_c[__frame.s3];
              __frame.p0 = q_c;
              __frame.p1 = p_c;
              __frame.p2 = r_c;
              g_c = ul_c+*p_c-p_c[__frame.s4]-p_c[left_c];
              cur_c = *q_c;
              __frame.s5 = g_c;
              r_c = &__frame.p2[-left_c];
              n_c = __frame.s2;
              bin_c = ((uint16_t)__frame.s5-(uint16_t)((__frame.s1*(q_c[__frame.s3]+cur_c-q_c[__frame.s4]-q_c[left_c])+__frame.wt_slot*(__frame.p2[__frame.s3]+*__frame.p2-__frame.p2[__frame.s4]-(uint32_t)__frame.p2[left_c])+40)>>7)-256)&0x1FF;
              ++*(uint32_t*)&buf_3[4*bin_c];
              q_c -= left_c;
              p_c = &__frame.p1[-left_c];
              __frame.s2 = n_c-1;
            } while( n_c!=1 );
            wt8_dn = __frame.s6;
            wt4_dn = __frame.s1;
            cost_c = __estimate_cost((uint8_t*)buf_3, 512);
            wt8_dn_end = *(uint32_t*)__frame.buf_1;
            wt4_best_dn = x5[3];
            if( cost_c<best_cost ) {
              best_cost = cost_c;
              wt4_best_dn = wt4_dn;
            }
            x5[3] = wt4_best_dn;
            wt4_dn_end = wt4_best_dn-wt_step;
          }
          if( wt8_dn>wt8_dn_end&&wt8_dn>=-64 ) {
            {
              __frame.s0 = wt4_dn_end;
              memset(buf_4, 0, 2048);
              npix_d = img_a->width*(img_a->height-1);
              stride_d = (uint16_t)img_a->stride;
              __frame.s6 = wt8_dn;
              __frame.s1 = wt4_dn;
              __frame.s7 = npix_d-1;
              __frame.s9 = -stride_d;
              left_d = -plane_count;
              __frame.s8 = -stride_d-plane_count;
              __frame.p4 = plane0-__frame.s8;
              q_d = plane0+__frame.plane_a-__frame.s8;
              __frame.p3 = q_d;
              r_d = plane0+__frame.plane_b-__frame.s8;
              p_d = __frame.p4;
              do {
                ul_d = p_d[__frame.s8];
                __frame.p3 = q_d;
                __frame.p4 = p_d;
                __frame.p5 = r_d;
                g_d = ul_d+*p_d-p_d[__frame.s9]-p_d[left_d];
                cur_d = *q_d;
                __frame.s10 = g_d;
                r_d = &__frame.p5[-left_d];
                n_d = __frame.s7;
                bin_d = ((uint16_t)__frame.s10-(uint16_t)((x5[3]*(q_d[__frame.s8]+cur_d-q_d[__frame.s9]-q_d[left_d])+__frame.s6*(__frame.p5[__frame.s8]+*__frame.p5-__frame.p5[__frame.s9]-(uint32_t)__frame.p5[left_d])+40)>>7)-256)&0x1FF;
                ++*(uint32_t*)&buf_4[4*bin_d];
                q_d -= left_d;
                p_d = &__frame.p4[-left_d];
                __frame.s7 = n_d-1;
              } while( n_d!=1 );
              wt8_dn = __frame.s6;
              wt4_dn = __frame.s1;
              cost_d = __estimate_cost((uint8_t*)buf_4, 512);
              wt4_dn_end = __frame.s0;
              wt8_best_dn = __frame.wt_slot;
              if( cost_d<best_cost ) {
                best_cost = cost_d;
                wt8_best_dn = wt8_dn;
              }
              __frame.wt_slot = wt8_best_dn;
              wt8_dn_end = wt8_best_dn-wt_step;
            }
          }
          --wt4_dn;
          --wt8_dn;
        }
      }
LABEL_19:
      px = &((uint8_t*)img_a)[__frame.row_stride+16+n_planes+x3[2]];
      if( px<data_end ) {
        __frame.wt_slot = wt8;
        x5[3] = wt4;
        __frame.nplanes_s = n_planes;
        do {
          c2 = px[__frame.plane_b];
          c2w = c2*__frame.wt_slot;
          c1 = px[__frame.plane_a];
          c1w = c1*x5[3];
          ++__frame.hist_c[c2-c1+1280];
          c0 = *px+512;
          px += __frame.nplanes_s;
          bin0 = ((uint16_t)c0-(uint16_t)((uint32_t)(c2w+c1w+40)>>7))&0x3FF;
          ++*(uint32_t*)&__frame.buf[4*bin0];
          ++__frame.hist_a[c0-c1];
          ++__frame.hist_b[c0-c2];
          ++__frame.hist_c[(c0-((uint32_t)(((c1+c2)<<6)+40)>>7))];
        } while( px<data_end );
        wt8 = __frame.wt_slot;
        wt4 = x5[3];
        n_planes = __frame.nplanes_s;
      }
      slack = best_cost>>7;
      if( best_cost>>7>=0x4000 )
        slack = 0x4000;
      best = slack+best_cost;
      cost_flat = *(int32_t*)((uint8_t*)&acc0[3]+x3[1]);
      cheaper = cost_flat<best;
      if( cost_flat<best ) {
        best = *(int32_t*)((uint8_t*)&acc0[3]+x3[1]);
        wt4 = 128;
        wt8 = 0;
      }
      pred = cheaper;
      if( *(uint32_t*)&scratch[x3[1]+16]<best ) {
        best = *(uint32_t*)&scratch[x3[1]+16];
        pred = 2;
        wt4 = 0;
        wt8 = 128;
      }
      keep3 = best<=*(uint32_t*)&scratch[x3[1]+20];
      if( best>*(uint32_t*)&scratch[x3[1]+20] )
        pred = 3;
      xform_row = x3[2];
      if( !keep3 ) {
        wt4 = 64;
        wt8 = 64;
      }
      plane_desc[x3[2]+1].weight0 = wt4;
      plane_desc[xform_row+1].weight1 = wt8;
      hist = &__frame.buf[4096*pred];
      win = (uintptr_t)hist&0xF;
      sum = 0;
      for( i = 0; i<256; ++i )
        sum += *(int32_t*)&hist[4*(win+i)];
      win += 256;
      best_sum = sum;
      pos = -1;
      if( win<1024 ) {
        __frame.nplanes_s = n_planes;
        do {
          sum = *(uint32_t*)&hist[4*win]+sum-*(uint32_t*)&hist[4*win-1024];
          if( sum>=best_sum ) {
            pos = win;
            best_sum = sum;
          }
          ++win;
        } while( win<1024 );
        n_planes = __frame.nplanes_s;
      }
      plane_desc[x3[2]+1].dc = pos+1;
      best_sum2 = 0;
      for( i = 0; i<0x100; ++i )
        best_sum2 += __frame.hist_c[i+1024];
      pos2 = 255;
      for( sum2 = best_sum2; i<512; ++i ) {
        sum2 = __frame.hist_c[i+1024]+sum2-__frame.hist_c[i+768];
        if( sum2>=best_sum2 ) {
          pos2 = i;
          best_sum2 = sum2;
        }
      }
      result = pos2+1;
      plane_desc[__frame.plane_b+x3[2]+1].dc = result;
      if( n_planes>=4 ) {
        __builtin_memset(x0, 0, 16);
        __builtin_memset(x1, 0, 16);
        __builtin_memset(x2, 0, 16);
        __builtin_memset(x3, 0, 16);
        __builtin_memset(x4, 0, 16);
        __builtin_memset(x5, 0, 16);
        memset(__frame.buf, 0, 0x8000);
        pp = &((uint8_t*)img_a)[__frame.row_stride];
        n_quads = (data_end-17-pp)/4;
        end_px = &((uint8_t*)img_a)[__frame.row_stride+20];
        if( data_end<=end_px ) {
          sum01 = *(double*)&x0[2];
          sum02 = *(double*)x1;
          sum12 = *(double*)&x2[2];
          __frame.q1 = *(int64_t*)&x0[2];
          __frame.q0 = *(int64_t*)&x2[2];
          d4 = *(double*)x0;
          q2 = *(int64_t*)&x4[2];
          sum02_c = *(double*)x1;
        } else {
          sum22 = *(double*)x4;
          sum11 = *(double*)x2;
          q2 = *(int64_t*)&x4[2];
          d4 = *(double*)x0;
          d5 = *(double*)&x0[2];
          d2 = *(double*)x1;
          d6 = *(double*)&x2[2];
          d3 = *(double*)x5;
          d1 = *(double*)&x5[2];
          i4 = 0;
          uu = (data_end-17-pp)/4;
          do {
            dv0 = (double)(((uint8_t*)img_a)[4*i4+16]+pp[4*i4+20]-(((uint8_t*)img_a)[4*i4+20]+pp[4*i4+16]));
            dv1 = (double)(((uint8_t*)img_a)[4*i4+17]+pp[4*i4+21]-(((uint8_t*)img_a)[4*i4+21]+pp[4*i4+17]));
            dv2 = (double)(((uint8_t*)img_a)[4*i4+18]+pp[4*i4+22]-(((uint8_t*)img_a)[4*i4+22]+pp[4*i4+18]));
            dv3 = ((uint8_t*)img_a)[4*i4+19]+pp[4*i4+23]-(((uint8_t*)img_a)[4*i4+23]+pp[4*i4+19]);
            d4 = d4+dv0*dv0;
            ++i4;
            d5 = d5+dv0*dv1;
            d2 = d2+dv0*dv2;
            sum11 = sum11+dv1*dv1;
            d6 = d6+dv1*dv2;
            sum22 = sum22+dv2*dv2;
            *(double*)&q2 = *(double*)&q2+dv0*(double)dv3;
            d3 = d3+dv1*(double)dv3;
            d1 = d1+dv2*(double)dv3;
          } while( i4<uu );
          sum01 = d5;
          sum02 = d2;
          sum12 = d6;
          n_quads = uu;
          *(double*)&x5[2] = d1;
          *(double*)x5 = d3;
          *(int64_t*)&x4[2] = q2;
          *(double*)x4 = sum22;
          *(double*)&x2[2] = d6;
          *(double*)x2 = sum11;
          *(double*)x1 = d2;
          *(double*)&x0[2] = d5;
          *(double*)x0 = d4;
          sum02_c = d2;
          *(double*)&__frame.q0 = d6;
          *(double*)&__frame.q1 = d5;
        }
        d5 = sum01;
        d6 = sum12;
        __frame.d0 = sum02_c;
        *(double*)&x1[2] = sum01;
        *(double*)x3 = sum02;
        *(double*)&x3[2] = sum12;
        d1 = d4**(double*)x4-sum02_c*sum02;
        d2 = 0.0-d4*sum12+sum02**(double*)&__frame.q1;
        d3 = sum02_c*sum12-*(double*)&__frame.q1**(double*)x4;
        inv = 128.0/(*(double*)x2*d1+*(double*)&__frame.q0*d2+sum01*d3+0.1);
        *(double*)acc0 = inv;
        wa = (int32_t)(((sum02**(double*)&__frame.q0-sum01**(double*)x4)**(double*)x5+(0.0-sum02**(double*)x2+sum01*d6)**(double*)&x5[2]+(*(double*)x4**(double*)x2-d6**(double*)&__frame.q0)**(double*)&q2)*inv);
        if( wa>=191 )
          wa = 191;
        if( wa<-64 )
          wa = -64;
        wa_slot = wa;
        wb = (int32_t)((d1**(double*)x5+d2**(double*)&x5[2]+d3**(double*)&q2)*inv);
        if( wb>=191 )
          wb = 191;
        if( wb<-64 )
          wb = -64;
        wc = (int32_t)(*(double*)acc0*((d4**(double*)&x5[2]-__frame.d0**(double*)&q2)**(double*)x2+(0.0-d4**(double*)x5+*(double*)&q2**(double*)&__frame.q1)**(double*)&__frame.q0+(__frame.d0**(double*)x5-*(double*)&__frame.q1**(double*)&x5[2])*d5));
        if( wc>=191 )
          wc = 191;
        if( wc<-64 )
          wc = -64;
        if( end_px<data_end ) {
          *(int64_t*)x0 = __PAIR64__(wc, wb);
          uu = n_quads;
          quad = 0;
          do {
            nx0 = ((uint8_t*)img_a)[4*quad+20];
            dg0 = ((uint8_t*)img_a)[4*quad+16]+pp[4*quad+20];
            dn0 = pp[4*quad+16];
            x0[2] = quad;
            ad0 = nx0+dn0;
            dnx1 = pp[4*quad+21];
            g0 = dg0-ad0;
            dn1 = pp[4*quad+17];
            x0[3] = g0;
            nx2 = ((uint8_t*)img_a)[4*quad+22];
            g1 = ((uint8_t*)img_a)[4*quad+17]+dnx1-(((uint8_t*)img_a)[4*quad+21]+dn1);
            dnx2 = pp[4*quad+22];
            x1[0] = g1;
            g2 = ((uint8_t*)img_a)[4*quad+18]+dnx2-(nx2+pp[4*quad+18]);
            g1w = (uint16_t)(((uint8_t*)img_a)[4*quad+19]+pp[4*quad+23]-pp[4*quad+19]-((uint8_t*)img_a)[4*quad+23]);
            g1_lo = ((int16_t*)x1)[0];
            g1w = g1w-512;
            pa = x1[0]*x0[0]+x0[3]*wa_slot;
            pb = g2*x0[1];
            ++__frame.hist_a[(g1w-((int16_t*)x0)[6])&0x3FF];
            bin_lin = (g1w-(uint16_t)((uint32_t)(pa+pb+63)>>7))&0x3FF;
            ++*(uint32_t*)&__frame.buf[4*bin_lin];
            ++__frame.hist_b[(g1w-g1_lo)&0x3FF];
            g1w = g1w-g2;
            quad_r = x0[2];
            ++__frame.hist_c[g1w&0x3FF];
            alpha = pp[4*quad_r+23]+256;
            bin_lin2 = ((uint16_t)alpha-(uint16_t)((x0[0]*pp[4*quad_r+21]+wa_slot*pp[4*quad_r+20]+x0[1]*(uint32_t)pp[4*quad_r+22]+63)>>7)+256)&0x3FF;
            ++__frame.hist_c[bin_lin2+1024];
            r0 = alpha-pp[4*quad_r+20];
            ++__frame.hist_c[r0+2048];
            r1 = alpha-pp[4*quad_r+21];
            ++__frame.hist_c[r1+3072];
            r2 = alpha-pp[4*quad_r+22];
            ++__frame.hist_c[r2+4096];
            quad = quad_r+1;
          } while( quad<uu );
          wc = x0[1];
          wb = x0[0];
        }
        cost_lin = __estimate_cost((uint8_t*)__frame.buf, 1024);
        best4 = (cost_lin>>7)+cost_lin;
        cost_c0 = __estimate_cost((uint8_t*)__frame.hist_a, 1024);
        pick0 = cost_c0<best4;
        if( cost_c0<best4 )
          best4 = cost_c0;
        wa_pick = wa_slot;
        if( pick0 ) {
          wa_pick = 128;
          wc = 0;
          wb = 0;
        }
        wa_slot = wa_pick;
        x0[0] = pick0;
        cost_c1 = __estimate_cost((uint8_t*)__frame.hist_b, 1024);
        pred4 = x0[0];
        if( cost_c1<best4 ) {
          best4 = cost_c1;
          pred4 = 2;
          wb = 128;
          wc = 0;
          wa_slot = 0;
        }
        x0[0] = pred4;
        cost_c2 = __estimate_cost((uint8_t*)__frame.hist_c, 1024);
        pred4b = x0[0];
        if( cost_c2<best4 ) {
          pred4b = 3;
          wc = 128;
          wb = 0;
          wa_slot = 0;
        }
        plane_desc[4].weight0 = wa_slot;
        plane_desc[4].weight1 = wb;
        plane_desc[4].weight2 = wc;
        plane_desc[4].src_plane = 3;
        plane_desc[4].nrefs = 3;
        hist2 = &__frame.hist_c[1024*pred4b+1024];
        result = (uintptr_t)hist2&0xF;
        sum3 = 0;
        for( i = 0; i<256; ++i )
          sum3 += hist2[result+i];
        result += 256;
        best_sum3 = sum3;
        for( pos3 = -1; result<1024; ++result ) {
          sum3 = hist2[result]+sum3-hist2[result-256];
          if( sum3>=best_sum3 ) {
            pos3 = result;
            best_sum3 = sum3;
          }
        }
        plane_desc[4].dc = pos3+1;
      }
    }
  }
  return result;
}

int32_t*__read_bmp(char* path) {
  BmfImage* img_f;
  uint8_t* row_ofs;
  void* pal_buf;
  uint8_t* row, bmp_bgra[4], pix;
  int32_t y4, y8;
  int32_t run4;
  uint8_t* row_at;
  uint8_t* pal, * pal2, * pal3;
  FILE* fp;
  uint8_t lo;
  BmfImage* img;
  uint32_t row_pad, byte;
  uint8_t cur, lo16;
  uint8_t* row4, * row3, * row6, * pal_at, * row5;
  int32_t pal_n, i, run, run_val, hi_nibble, left4, left4b, y, dx, dy, dxy, dy4, byte_in, step;
  uint32_t stride_pad, pair, hi, stride, got, left;
  BmpHeader hdr;
  fp = fopen(path, "rb");
  if( !fp||fread(&hdr.bfType, 0xEu, 1u, fp)!=1||hdr.bfType!=0x4D42||fread(&hdr.biSize, 0x28u, 1u, fp)!=1||hdr.biSize!=40||hdr.biPlanes!=1 ) {
    return nullptr;
  }
  {
    const int32_t bmp_w = hdr.biWidth;
    const int32_t bmp_h = hdr.biHeight;
    const int32_t bmp_bpp = hdr.biBitCount;
    if( bmp_w<=0||bmp_w>0xFFFF||bmp_h<=0||bmp_h>0xFFFF||(bmp_bpp>=8&&(uint32_t)bmp_w*(uint32_t)((bmp_bpp+7)>>3)>0xFFFFu)||(bmp_bpp!=1&&bmp_bpp!=4&&bmp_bpp!=8&&bmp_bpp!=24&&bmp_bpp!=32)||(int32_t)hdr.biClrUsed<0||(bmp_bpp<=8&&(int32_t)hdr.biClrUsed>(1<<bmp_bpp))||hdr.biCompression>2||(hdr.biCompression==1&&bmp_bpp!=8)||(hdr.biCompression==2&&bmp_bpp!=4) ) {
      fclose(fp);
      return nullptr;
    }
  }
  img = (BmfImage*)(__alloc_image(hdr.biWidth, hdr.biHeight, hdr.biBitCount, hdr.biBitCount<=8u, 1));
  if( !img ) {
    fclose(fp);
    return nullptr;
  }
  stride_pad = (img->stride+3)&0xFFFFFFFC;
  if( hdr.biBitCount<=8u ) {
    pal_n = 1<<(hdr.biBitCount&31);
    if( (int32_t)hdr.biClrUsed )
      pal_n = (int32_t)hdr.biClrUsed;
    if( pal_n>0 ) {
      for( i = 0; i<pal_n; ++i ) {
        fread(bmp_bgra, 4u, 1u, fp);
        if( (img->depth&0x80)!=0 )
          pal = img->pixels+img->data_size;
        else
          pal = 0;
        *(pal+3*i+2) = bmp_bgra[2];
        if( (img->depth&0x80)!=0 )
          pal2 = img->pixels+img->data_size;
        else
          pal2 = 0;
        *(pal2+3*i+1) = bmp_bgra[1];
        if( (img->depth&0x80)!=0 )
          pal3 = img->pixels+img->data_size;
        else
          pal3 = 0;
        *(pal3+3*i) = bmp_bgra[0];
      }
    }
  }
  pal_buf = bmf_new(stride_pad<256 ? 256 : stride_pad);
  row = img->pixels+img->data_size-img->stride;
  fseek(fp, (int32_t)hdr.bfOffBits, 0);
  if( hdr.biCompression ) {
    uint8_t*const pix_lo = img->pixels;
    uint8_t*const pix_hi = img->pixels+img->data_size;
    if( hdr.biCompression==1 ) {
      memset(img->pixels, 0, img->data_size);
      row_at = row;
      img_f = (BmfImage*)img;
      y4 = img->height-1;
      while( 1 ) {
        row_ofs = row_at;
        if( ferror(fp) )
          return nullptr;
        run = fgetc(fp);
        run_val = fgetc(fp);
        if( run<0||run_val<0 )
          return nullptr;
        if( run ) {
          if( row_ofs<pix_lo||row_ofs+run>pix_hi )
            return nullptr;
          __builtin_memset(row_ofs, run_val, run);
          row_at = row_ofs+run;
        } else if( run_val ) {
          if( run_val==1 )
            goto LABEL_61;
          if( run_val==2 ) {
            dx = fgetc(fp);
            dy = fgetc(fp);
            if( dx<0||dy<0 )
              return nullptr;
            row_at += dx-dy*(uint16_t)img_f->stride;
          } else {
            if( row_at<pix_lo||row_at+run_val>pix_hi )
              return nullptr;
            fread(pal_buf, (run_val+1)&0xFFFFFFFE, 1u, fp);
            memcpy(row_at, (uint8_t*)pal_buf, run_val);
            row_at += run_val;
          }
        } else {
          if( --y4<0 )
            goto LABEL_61;
          row_at = img_f->pixels+y4*(uint16_t)img_f->stride;
        }
      }
    }
    if( hdr.biCompression!=2 )
      return nullptr;
    memset(img->pixels, 0, img->data_size);
    img_f = (BmfImage*)img;
    hi_nibble = 1;
    y8 = img->height-1;
    while( 1 ) {
      while( 1 ) {
        while( 1 ) {
LABEL_44:
          if( ferror(fp) )
            return nullptr;
          run4 = fgetc(fp);
          byte_in = fgetc(fp);
          if( run4<0||byte_in<0 )
            return nullptr;
          byte = (uint32_t)byte_in;
          pair = byte;
          if( !run4 )
            break;
          if( row<pix_lo||row+(run4/2+1)>pix_hi )
            return nullptr;
          lo = byte&0xF;
          if( hi_nibble ) {
            left4b = run4;
            row3 = row;
            while( left4b!=1 ) {
              *row3++ = pair;
              left4b -= 2;
              if( !left4b ) {
                row = row3;
                hi_nibble = 1;
                goto LABEL_44;
              }
            }
            row = row3;
            *row3 = pair&0xF0;
            hi_nibble = 0;
          } else {
            left4 = run4;
            row4 = row;
            cur = *row;
            hi = pair>>4;
            lo16 = 16*lo;
            while( 1 ) {
              *row4++ = hi|cur;
              if( left4==1 )
                break;
              cur = lo16;
              left4 -= 2;
              if( !left4 ) {
                row = row4;
                *row4 = lo16;
                hi_nibble = 0;
                goto LABEL_44;
              }
            }
            row = row4;
            hi_nibble = 1;
          }
        }
        if( byte )
          break;
        if( --y8<0 )
          goto LABEL_61;
        row = img_f->pixels+y8*(uint16_t)img_f->stride;
      }
      if( byte==1 )
        goto LABEL_61;
      if( byte!=2 )
        break;
      dxy = fgetc(fp);
      dy4 = fgetc(fp);
      if( dxy<0||dy4<0 )
        return nullptr;
      step = (dxy>>1)-dy4*(uint16_t)img_f->stride;
      if( (dxy&1)==1 ) {
        if( !hi_nibble )
          ++step;
        hi_nibble = !hi_nibble;
      }
      row += step;
    }
    if( row<pix_lo||row+((int32_t)byte/2+1)>pix_hi )
      return nullptr;
    fread(pal_buf, (((byte+1)>>1)+1)&0xFFFFFFFE, 1u, fp);
    pal_at = (uint8_t*)pal_buf;
    row5 = row;
    while( 1 ) {
      pix = *pal_at;
      if( hi_nibble ) {
        left = pair-1;
        if( !left ) {
          row = row5;
          *row5 = *pal_at&0xF0;
          hi_nibble = 0;
          goto LABEL_44;
        }
        *row5++ = pix;
        hi_nibble = 1;
      } else {
        *row5++ |= (uint8_t)*pal_at>>4;
        left = pair-1;
        if( !left ) {
          row = row5;
          hi_nibble = 1;
          goto LABEL_44;
        }
        *row5 = 16*(pix&0xF);
        hi_nibble = 0;
      }
      ++pal_at;
      pair = left-1;
      if( !pair ) {
        row = row5;
        goto LABEL_44;
      }
    }
  }
  stride = img->stride;
  row_pad = stride_pad-stride;
  if( hdr.biHeight-1>=0 ) {
    y = hdr.biHeight-1;
    img_f = (BmfImage*)img;
    row6 = row;
    while( 1 ) {
      got = fread(row6, 1u, stride, fp);
      stride = (uint16_t)img_f->stride;
      if( got!=stride )
        return nullptr;
      if( row_pad ) {
        fseek(fp, row_pad, 1);
        stride = (uint16_t)img_f->stride;
      }
      row6 -= stride;
      if( --y<0 )
        goto LABEL_61;
    }
  }
LABEL_61:
  fclose(fp);
  free(pal_buf);
  return (int32_t*)img;
}

int32_t __decode_symbol_list(SymList* syms) {
  struct alignas(16) DecodeSymbolListFrame {
    union {
      SymEntry* list[8192];
      struct {
        uint32_t list0;
        BMF_SPILL_PAD(l0);
        SymEntry* list1;
        uint32_t list2;
        BMF_SPILL_PAD(l2);
        int32_t list3;
        BMF_SPILL_PAD(l3);
        int32_t list4;
        BMF_SPILL_PAD(l4);
        uint32_t list5;
        BMF_SPILL_PAD(l5);
        SymEntry* list6;
        int32_t list7;
        BMF_SPILL_PAD(l7);
        SymEntry* list_tail[8184];
      };
    };
    uint8_t _pad1[32];
  } __frame;
  int32_t tot, tot0;
  SymEntry* first;
  SymList* owner;
  uint32_t gen;
  SymEntry* ent, * e, * p, * q, * head, * q2, * r, * cur, * prev, * up, * back;
  SymEntry** w, ** rd, ** rd2;
  int8_t gen_b;
  uint32_t cum_lo, sym_cum, sym_high, cum_hi;
  int32_t tot_1, tot_all, target;
  uint8_t c_a, c_b;
  uint16_t s_a, s_b;
  int32_t live, cum, i, c, top, half, back_cnt, last_cnt;
  uint32_t n_left, zeros;
  uint32_t list5_s, rescale_at, limit20, running, since_rescale;
  live = syms->live;
  ent = syms->ent;
  w = __frame.list;
  gen = (uint8_t)exclusion_gen;
  owner = syms;
  cum = 0;
  i = 0;
  do {
    if( (uint8_t)exclusion_mask[ent[i].sym]==gen ) {
      c = 0;
    } else {
      e = &ent[i];
      c = e->cnt;
      *w++ = e;
    }
    cum += c;
    ++i;
  } while( i<live );
  if( !cum )
    return -1;
  *w = nullptr;
  tot0 = owner->tot;
  tot_all = cum+tot0;
  target = rc.get_freq(tot_all);
  tot = cum+tot0;
  p = (__frame.list[0]);
  rd = &__frame.list[1];
  first = (__frame.list[0]);
  cum_lo = 0;
  while( 1 ) {
    cum_lo += p->cnt;
    if( cum_lo>(uint32_t)(int32_t)target )
      break;
    p = *rd++;
    if( !p ) {
      gen_b = (int8_t)gen;
      sym_cum = cum_lo;
      cum_hi = tot;
      sym_high = tot;
      q = first;
      rd2 = &__frame.list[1];
      do {
        exclusion_mask[q->sym] = gen_b;
        q = *rd2++;
      } while( q );
      tot_1 = tot;
      __frame.list7 = -1;
      {
        rc.decode(cum_lo, cum_hi, tot_1);
        return __frame.list7;
      }
    }
  }
  sym_high = cum_lo;
  sym_cum = cum_lo-p->cnt;
  __frame.list7 = p->sym;
  p->cnt += 4;
  head = owner->ent;
  owner->since_rescale += 4;
  if( p==head ) {
    top = p->cnt;
  } else {
    c_a = p->cnt;
    s_a = p->sym;
    q2 = p-1;
    *p = *q2;
    q2->set(s_a, c_a);
    head = owner->ent;
    if( q2==head ) {
      top = q2->cnt;
    } else {
      __frame.list5 = list5_s;
      while( 1 ) {
        top = q2->cnt;
        r = q2-1;
        if( top<=r->cnt )
          break;
        s_b = q2->sym;
        c_b = q2->cnt;
        *q2 = *r;
        r->set(s_b, c_b);
        head = owner->ent;
        --q2;
        if( r==head ) {
          list5_s = __frame.list5;
          top = r->cnt;
          goto LABEL_30;
        }
      }
      list5_s = __frame.list5;
    }
  }
LABEL_30:
  rescale_at = owner->rescale_at;
  if( top>251||rescale_at<owner->since_rescale ) {
    __frame.list0 = owner->live;
    limit20 = 20*owner->n;
    n_left = __frame.list0;
    __frame.list5 = list5_s;
    __frame.list4 = rescale_at<limit20;
    cur = head-1;
    do {
      prev = cur;
      ++cur;
      half = (__frame.list4+(uint32_t)cur->cnt)>>1;
      cur->cnt = half;
      if( cur!=owner->ent ) {
        up = cur-1;
        __frame.list3 = up->cnt;
        if( half>__frame.list3 ) {
          __frame.list2 = cur->sym;
          cur->set(up->sym, __frame.list3);
          if( up!=owner->ent ) {
            (__frame.list[1]) = cur;
            __frame.list0 = n_left;
            do {
              back = up-1;
              back_cnt = back->cnt;
              if( half<=back_cnt )
                break;
              up->set(back->sym, back_cnt);
              --up;
            } while( back!=owner->ent );
            cur = (__frame.list[1]);
            n_left = __frame.list0;
          }
          up->set(__frame.list2, half);
        }
      }
      --n_left;
    } while( n_left );
    list5_s = __frame.list5;
    running = owner->tot;
    __frame.list0 = 0;
    if( !cur->cnt ) {
      zeros = __frame.list0;
      do {
        ++zeros;
        owner->tot = ++running;
        last_cnt = prev->cnt;
        --prev;
      } while( !last_cnt );
      __frame.list0 = zeros;
      owner->live -= zeros;
    }
    since_rescale = owner->since_rescale;
    owner->tot = running-(running>>1);
    cum_lo = sym_cum;
    cum_hi = sym_high;
    tot_1 = tot_all;
    owner->since_rescale = since_rescale-(since_rescale>>1);
  } else {
    tot_1 = tot_all;
    cum_lo = sym_cum;
    cum_hi = sym_high;
  }
  rc.decode(cum_lo, cum_hi, tot_1);
  return __frame.list7;
}

inline int32_t ModelBlock::decode_pixel(int32_t x) {
  struct alignas(16) DecodePixelFrame {
    union {
      uint32_t sym[32];
      struct {
        int32_t sym0;
        int32_t sym1;
        int32_t sym2;
        int32_t sym3;
        int32_t sym4;
        int32_t sym5;
        int32_t sym6;
        int32_t sym7;
        int32_t sym8;
        int32_t sym9;
        int32_t sym10;
        int32_t sym11;
        int32_t sym12;
        int32_t sym13;
        int32_t sym14;
        int32_t sym15;
        int32_t sym16;
        int32_t sym17;
        int32_t sym18;
        int32_t sym19;
        int32_t sym20;
        int32_t sym21;
        int32_t sym22;
        int32_t sym23;
        int32_t sym24;
        int32_t sym25;
        int32_t sym26;
        int32_t sym27;
        int32_t sym28;
        int32_t sym29;
        int32_t sym30;
        int32_t sym31;
      };
    };
    uint8_t _gap0[4];
    uint8_t _pad0[32];
  } __frame;
  int32_t idx_t;
  FreqRec* freq;
  uint16_t* id3p;
  uint8_t g_ab;
  int8_t gen;
  FreqRec* freq2;
  uint16_t* sym_cache;
  PixRec* up4;
  FreqRec* freq_tbl;
  SymList* sel1_list, ** sel_p;
  int32_t bin_tot;
  int32_t arg_cum, arg_high, arg_tot;
  uint32_t done, lvl_a, pos1;
  PixRec* r8, * r7, * rec;
  uint16_t* pixq;
  PixRec* r8b, * r7b, * next, * r5, * cur6b;
  uint8_t g_a, g_b, g_c, g_d, g_e;
  int32_t n2r;
  int16_t sym_rev, s1c, w1s, s3b;
  int32_t up_sym, left_sym, up_p1_sym, up_m1_sym, m_lo, m_up, nb, key, ctx_state, pair_last, cap, ctx_bucket, up_m0, m_w1, nb2, all_up, sig1, id1, sig2, id2, id3_used, s1a, m_up0, idx1, run, bucket, bit, msym1c, hit_a, idx_s, mask, seen, run0, s1b, * recw, flags_word, s3c, s3d, s1d, tot, target, cum, w6a, b15a, w5a, msym3, msym1, msym2, pix1, cache0, cache1, c4, c5, c6, c7, h11, h10, h12, h13, h14, h15, h16, h17, h18, h19, h20, h21, h24, h28, h26, h30, h31, psym, bit2, msym1b, lsym, s0b, b15, w2t, w3t, w4t, w5, w0r, w1r, q1, w2r, w3, w3r, s0a, span, w4r, n4r, target2, cum2, lvl_b, b15b, w5b, w6b, s0c, s3a;
  PixRec* row;
  SymPair* pair;
  PixRec* cur6;
  uint16_t pair_prev;
  PixRec* up5, * r8c;
  uint16_t* pixp, * pixr, q1w, tot2, k0;
  uint32_t si, s2a, k2, k2h;
  uint16_t k1, g4, g3, g2, g1, g0;
  uint16_t k4, k3;
  PixRec* up2;
  PixRec* up1;
  PixRec* up3;
  PixRec* up9;
  PixRec* cur6c;
  BitCtr* ctr;
  cur6 = (PixRec*)this->row_cur[6];
  up_sym = cur6->sym;
  row = this->row_cur[5];
  left_sym = row[-1].sym;
  up_p1_sym = cur6[1].sym;
  up_m1_sym = cur6[-1].sym;
  __frame.sym0 = up_sym;
  ::mode_symbol[1] = up_sym;
  __frame.sym1 = left_sym;
  ::mode_symbol[2] = left_sym;
  __frame.sym3 = up_p1_sym;
  ::mode_symbol[3] = up_p1_sym;
  m_lo = cur6->match[1]+4*(up_p1_sym==up_m1_sym);
  m_up = cur6[1].match[1];
  __frame.sym2 = up_m1_sym;
  mode_symbol[4] = up_m1_sym;
  nb = 32*row[-1].match[2]+16*row[-1].match[4]+(2*row[-1].match[0]+8*m_up+m_lo);
  if( up_sym==left_sym ) {
    if( __frame.sym3==__frame.sym0 ) {
      sym_rev = this->sym_rev[up_sym];
      if( up_sym==__frame.sym2 )
        key = (uint16_t)(sym_rev-row[-2].sym);
      else
        key = (uint16_t)(sym_rev-__frame.sym2);
    } else {
      key = (uint16_t)(this->sym_rev[up_sym]-__frame.sym3);
    }
  } else {
    key = (uint16_t)(this->sym_rev[up_sym]-__frame.sym1);
  }
  this->sym_cache = &this->sym_ctr[8*key];
  ctx_state = this->ctx_state[nb];
  this->ctx_state_seen = ctx_state;
  pair = &this->group_ctr[ctx_state][key];
  this->pix_cur = (uint16_t*)pair;
  pair_last = pair->last;
  if( pair_last==__frame.sym0 ) {
    cap = 15;
  } else if( pair_last==__frame.sym1 ) {
    cap = 30;
  } else if( pair_last==__frame.sym3 ) {
    cap = 45;
  } else {
    cap = 60;
    if( !(pair_last==__frame.sym2) )
      cap = 0;
  }
  pair_prev = pair->prev;
  if( pair_prev==__frame.sym0 ) {
    cap += 75;
  } else if( pair_prev==__frame.sym1 ) {
    cap += 150;
  } else if( pair_prev==__frame.sym3 ) {
    cap += 225;
  } else if( pair_prev==__frame.sym2 ) {
    cap += 300;
  }
  r8 = this->row_cur[8];
  ctx_bucket = this->ctx_bucket[ctx_state+cap];
  r7 = this->row_cur[7];
  this->bucket_idx = ctx_bucket;
  up_m0 = cur6->match[0];
  __frame.sym2 = up_m0;
  m_w1 = row[-1].match[1];
  up9 = this->row_cur[9];
  nb2 = 8*row[-2].match[2]+4*row[-2].match[5]+m_w1+2*row[-2].match[4];
  all_up = ((uint8_t)(up9->match[0]&r8->match[0]&__frame.sym2&r7->match[0])<<9)+((uint8_t)(up9->match[1]&r8->match[1]&r7->match[1]&cur6->match[1])<<8)+(ctx_bucket<<10)+nb2;
  cur6b = cur6;
  sig1 = ((this->grad[3]==0)<<7)+((this->grad[2]==0)<<6)+32*(this->grad[1]==0)+16*(this->grad[0]==0)+all_up;
  id1 = this->ctx_id1[sig1];
  if( id1==0xFFFF ) {
    this->ctx_id1[sig1] = this->ctx_id1_used;
    cur6b = this->row_cur[6];
    row = this->row_cur[5];
    ++this->ctx_id1_used;
    id1 = this->ctx_id1[sig1];
    __frame.sym2 = cur6b->match[0];
  }
  sig2 = row[-1].match[5]+4*cur6b[1].match[3]+2*__frame.sym2+8*id1;
  id2 = this->ctx_id2[sig2];
  if( id2==0xFFFF ) {
    this->ctx_id2[sig2] = this->ctx_id2_used++;
    id2 = this->ctx_id2[sig2];
  }
  if( (int32_t)this->alphabet<32 ) {
    id3_used = this->ctx_id3_used;
    __frame.sym1 = 16*id2+(__frame.sym1&0xF);
    id3p = &this->ctx_id3[__frame.sym1];
    id2 = *id3p;
    if( id2==0xFFFF ) {
      s1a = __frame.sym1;
      if( id3_used>53248 )
        s1a = __frame.sym1|0xF;
      id3p = &this->ctx_id3[s1a];
      id2 = *id3p;
    }
    if( id2>=id3_used ) {
      *id3p = id3_used;
      ++this->ctx_id3_used;
      id2 = *id3p;
    }
  }
  if( (this->row_cur[5][-1].match[1]&this->row_cur[5][-1].match[0])!=0 ) {
    up1 = this->row_cur[6];
    up2 = this->row_cur[7];
    if( ((uint8_t)(up2[2].match[1]&up2[1].match[1]&up2[0].match[1]&up1[3].match[1]&up1[2].match[1]&up1[1].match[1]&up1[0].match[1]&up1[0].match[0]&(int8_t)up1[-1].match[1])&up2[3].match[1])!=0 ) {
      m_up0 = up2[0].match[0];
      idx1 = 1;
      if( this->width-x<=1 ) {
        run = 1;
      } else {
        __frame.sym1 = this->width-x;
        while( 1 ) {
          run = idx1;
          if( (up1[idx1+2].match[1]&up1[idx1+2].match[0])==0 )
            break;
          m_up0 = (uint8_t)(up2[run].match[0]&m_up0);
          if( ++idx1>=__frame.sym1 ) {
            run = idx1;
            goto LABEL_42;
          }
        }
      }
LABEL_42:
      bucket = *(this->run_bucket+idx1);
      bit = this->esc_ctr[(8*bucket+4*(uint8_t)(up2[run+3].match[1]&up2[run+2].match[1])+2*m_up0+*(this->alpha_map+__frame.sym0)+1)].decode_context_bit(this->esc_ctr);
      msym1c = ::mode_symbol[1];
      this->hit = bit;
      *(((uint8_t*)this->alpha_map)+msym1c) = bit;
      hit_a = this->hit;
      if( hit_a ) {
        idx_s = idx1;
      } else {
        idx_s = 0;
        if( idx1==1 )
          goto LABEL_57;
        __frame.sym3 = idx1;
        __frame.sym0 = bucket;
        idx_s = 0;
        mask = 1<<(bucket&31);
        seen = 0;
        do {
          if( (mask|seen)<__frame.sym3 ) {
            ctr = &this->run_ctr[16*((seen==0)+(bucket==__frame.sym0))+bucket];
            run0 = ctr->n[0];
            bin_tot = run0+ctr->n[1];
            __frame.sym1 = rc.decode_bit(run0, ctr->n[1]);
            if( ctr->limit<(uint32_t)bin_tot )
              __rescale_counter_pair(ctr);
            s1b = __frame.sym1;
            ctr->n[__frame.sym1] += 8;
            if( s1b )
              idx_s |= mask;
            seen |= idx_s&mask;
          }
          --bucket;
          mask >>= 1;
        } while( mask );
      }
      if( idx_s )
        this->row_cur[5][idx_s-1].sym = this->row_cur[5][-1].sym;
      hit_a = this->hit;
LABEL_57:
      if( idx_s>hit_a ) {
        this->row_cur[6] = this->row_cur[6]+idx_s-hit_a;
        r8b = this->row_cur[8];
        r7b = this->row_cur[7]+idx_s;
        this->row_cur[7] = r7b-hit_a;
        this->row_cur[8] = r8b+idx_s-hit_a;
        rec = this->row_cur[5];
        this->row_cur[9] = this->row_cur[9]+idx_s-hit_a;
        rec->match2345 = 0x01010101;
        *(uint32_t*)this->row_cur[5] = 0x01010101;
        pixp = (uint16_t*)this->pix_cur;
        __frame.sym1 = ::mode_symbol[1];
        pixp[1] = *pixp;
        this->row_cur[5]->sym = (uint16_t)::mode_symbol[1];
        this->pix_cur[0] = (uint16_t)::mode_symbol[1];
        recw = (int32_t*)this->row_cur[5];
        flags_word = recw[1];
        __frame.sym3 = *recw;
        next = this->row_cur[5]+1;
        this->row_cur[5] = next;
        if( idx_s-hit_a!=1 ) {
          __frame.sym2 = (idx_s-hit_a-1)/2;
          if( __frame.sym2 ) {
            s3c = __frame.sym3;
            __frame.sym0 = hit_a;
            s1c = __frame.sym1;
            si = 0;
            idx_t = idx_s;
            s2a = __frame.sym2;
            do {
              this->pix_cur[1] = s1c;
              *(uint32_t*)this->row_cur[5] = s3c;
              this->row_cur[5]->match2345 = flags_word;
              pixq = this->pix_cur;
              ++this->row_cur[5];
              pixq[1] = s1c;
              *(uint32_t*)this->row_cur[5] = s3c;
              this->row_cur[5]->match2345 = flags_word;
              next = this->row_cur[5]+1;
              this->row_cur[5] = next;
              ++si;
            } while( si<s2a );
            hit_a = __frame.sym0;
            idx_s = idx_t;
            done = 2*si+1;
          } else {
            done = 1;
          }
          if( (uint32_t)(idx_s-hit_a-1)>(done-1) ) {
            s3d = __frame.sym3;
            this->pix_cur[1] = __frame.sym1;
            *(uint32_t*)this->row_cur[5] = s3d;
            this->row_cur[5]->match2345 = flags_word;
            next = this->row_cur[5]+1;
            this->row_cur[5] = next;
          }
        }
        idx_t = idx_s;
        cur6c = (PixRec*)this->row_cur[6];
        g_a = cur6c[-3].match[0];
        g_b = cur6c[-2].match[0];
        g_c = cur6c[2].match[0];
        g_d = cur6c[3].match[0];
        g_ab = g_b+g_a;
        g_e = cur6c[4].match[0];
        up3 = (PixRec*)this->row_cur[7];
        this->grad[0] = g_d+g_c+g_ab+g_e-5;
        this->grad[1] = up3[3].match[0]+up3[2].match[0]+up3[1].match[0]+up3[0].match[0]+(int8_t)up3[-1].match[0]+(int8_t)up3[-2].match[0]+(int8_t)up3[-3].match[0]+up3[4].match[0]-8;
        this->grad[2] = next[-4].match[1]+next[-3].match[1]-2;
        s1d = __frame.sym1;
        this->grad[3] = next[-5].match[0]+next[-6].match[0]+next[-7].match[0]+next[-4].match[0]-4;
        next[-1].match[4] = s1d==cur6c[1].sym;
        idx_s = idx_t;
        this->row_cur[5][-1].match[5] = s1d==this->row_cur[6][2].sym;
        hit_a = this->hit;
      }
      if( hit_a )
        return idx_s;
      goto LABEL_86;
    }
  }
  freq = &this->grid[this->bucket_idx];
  freq_tbl = &this->grid[id2+188];
  tot = freq_tbl->w[5];
  if( freq_tbl->w[5] ) {
    if( tot==1 ) {
      b15 = freq->b15;
      w2t = b15*freq_tbl->w[2];
      w3t = b15*freq_tbl->w[3];
      __frame.sym1 = b15*freq_tbl->w[0];
      w1s = b15*freq_tbl->w[1];
      __frame.sym2 = w2t;
      w4t = b15*freq_tbl->w[4];
      __frame.sym0 = w3t;
      __frame.sym3 = w4t;
      *freq_tbl = *freq;
      w5 = freq_tbl->w[5];
      w0r = freq_tbl->w[0];
      freq_tbl->b14 *= 8;
      w1r = 21*freq_tbl->w[1];
      __frame.sym1 += (21*w0r+w5-1)/w5;
      freq_tbl->w[0] = __frame.sym1;
      q1 = (w1r+w5-1)/w5;
      w2r = 21*freq_tbl->w[2];
      w3 = freq_tbl->w[3];
      q1w = q1+w1s;
      freq_tbl->w[1] = q1w;
      w3r = 21*w3;
      s0a = __frame.sym0;
      n2r = (w2r+w5-1)/w5+__frame.sym2;
      freq_tbl->w[2] = (uint16_t)n2r;
      span = (w3r+w5-1)/w5+s0a;
      w4r = 21*freq_tbl->w[4];
      freq_tbl->w[3] = span;
      n4r = (w4r+w5-1)/w5+__frame.sym3;
      freq_tbl->w[4] = n4r;
      tot2 = __frame.sym1+n4r+(span+(uint16_t)n2r+q1w);
      tot = tot2;
      freq_tbl->w[5] = tot2;
    }
    arg_tot = tot;
    target = rc.get_freq(arg_tot);
    cum = freq_tbl->w[0];
    if( cum<=target ) {
      cum += freq_tbl->w[1];
      if( cum<=target ) {
        cum += freq_tbl->w[2];
        if( cum<=target ) {
          cum += freq_tbl->w[3];
          if( cum<=target ) {
            cum += freq_tbl->w[4];
            lvl_a = 4;
          } else {
            lvl_a = 3;
          }
        } else {
          lvl_a = 2;
        }
      } else {
        lvl_a = 1;
      }
    } else {
      lvl_a = 0;
    }
    w6a = freq_tbl->w[6];
    arg_high = cum;
    b15a = freq_tbl->b15;
    arg_cum = cum-freq_tbl->w[lvl_a];
    w5a = freq_tbl->w[5];
    if( w5a>w6a&&(freq_tbl->w[lvl_a]+b15a+8<w5a||freq_tbl->w[5]>0x4000u) ) {
      g2 = freq_tbl->w[2];
      __frame.sym0 = w6a;
      g1 = freq_tbl->w[1];
      g0 = freq_tbl->w[0];
      __frame.sym1 = lvl_a;
      __frame.sym2 = b15a;
      g0 = g0-(g0>>1);
      freq_tbl->w[0] = g0;
      g1 = g1-(g1>>1);
      freq_tbl->w[1] = g1;
      g2 = g2-(g2>>1);
      g3 = freq_tbl->w[3];
      freq_tbl->w[2] = g2;
      g3 = g3-(g3>>1);
      g4 = freq_tbl->w[4];
      freq_tbl->w[3] = g3;
      g4 = g4-(g4>>1);
      freq_tbl->w[4] = g4;
      g1 = g3+g2+g1;
      b15a = __frame.sym2;
      g4 = g0+g4;
      w5a = (uint16_t)(g4+g1);
      s0b = __frame.sym0;
      lvl_a = __frame.sym1;
      freq_tbl->w[5] = w5a;
      if( s0b<256&&!freq_tbl->b14 ) {
        s0b = 256;
        freq_tbl->w[6] = 256;
      }
      if( w5a>s0b ) {
        if( b15a<15 )
          b15a = 15;
        freq_tbl->b15 = b15a;
      }
    }
    freq_tbl->w[5] = b15a+w5a;
    freq_tbl->w[lvl_a] += b15a;
    rc.decode(arg_cum, arg_high, arg_tot);
    this->hit = lvl_a;
    if( freq_tbl->b14 ) {
      --freq_tbl->b14;
      freq2 = freq;
      ++freq->w[5];
      ++freq2->w[lvl_a];
      lvl_a = this->hit;
    }
  } else {
    arg_tot = freq->w[5];
    target2 = rc.get_freq(arg_tot);
    cum2 = freq->w[0];
    if( cum2<=target2 ) {
      cum2 += freq->w[1];
      if( cum2<=target2 ) {
        cum2 += freq->w[2];
        if( cum2<=target2 ) {
          cum2 += freq->w[3];
          if( cum2<=target2 ) {
            cum2 += freq->w[4];
            lvl_b = 4;
          } else {
            lvl_b = 3;
          }
        } else {
          lvl_b = 2;
        }
      } else {
        lvl_b = 1;
      }
    } else {
      lvl_b = 0;
    }
    b15b = freq->b15;
    arg_high = cum2;
    arg_cum = cum2-freq->w[lvl_b];
    w5b = freq->w[5];
    w6b = freq->w[6];
    __frame.sym3 = b15b;
    if( w5b>w6b&&(freq->w[lvl_b]+__frame.sym3+8<w5b||w5b>0x4000) ) {
      __frame.sym0 = w6b;
      __frame.sym2 = lvl_b;
      k1 = freq->w[1];
      k2 = freq->w[2];
      k0 = freq->w[0]-(freq->w[0]>>1);
      freq->w[0] = k0;
      k1 = k1-(k1>>1);
      freq->w[1] = k1;
      k2h = k2-(k2>>1);
      k3 = (uint16_t)freq->w[3];
      freq->w[2] = k2h;
      k3 = k3-(k3>>1);
      k4 = (uint16_t)freq->w[4];
      freq->w[3] = k3;
      k4 = k4-(k4>>1);
      freq->w[4] = k4;
      k4 = k0+k4;
      w5b = (uint16_t)(k4+k3+k2h+k1);
      s0c = __frame.sym0;
      freq->w[5] = w5b;
      lvl_b = __frame.sym2;
      if( s0c<256&&!freq->b14 ) {
        s0c = 256;
        freq->w[6] = 256;
      }
      if( w5b>s0c ) {
        s3a = __frame.sym3;
        if( __frame.sym3<15 )
          s3a = 15;
        __frame.sym3 = s3a;
        freq->b15 = s3a;
      }
    }
    s3b = __frame.sym3;
    freq->w[5] = __frame.sym3+w5b;
    freq->w[lvl_b] += s3b;
    rc.decode(arg_cum, arg_high, arg_tot);
    this->hit = lvl_b;
    freq_tbl->w[5] = freq_tbl->w[lvl_b]++!=0;
    lvl_a = this->hit;
  }
  if( lvl_a ) {
    this->row_cur[5]->sym = mode_symbol[lvl_a];
    return 1;
  }
  idx_s = 0;
LABEL_86:
  gen = exclusion_gen;
  msym3 = ::mode_symbol[3];
  msym1 = ::mode_symbol[1];
  idx_t = idx_s;
  msym2 = ::mode_symbol[2];
  exclusion_mask[mode_symbol[4]] = exclusion_gen;
  pixr = (uint16_t*)this->pix_cur;
  exclusion_mask[msym3] = gen;
  exclusion_mask[msym2] = gen;
  exclusion_mask[msym1] = gen;
  __byte_445440[0] = gen;
  this->sel[0] = nullptr;
  pix1 = pixr[1];
  __frame.sym0 = *pixr;
  sym_cache = this->sym_cache;
  cache0 = sym_cache[0];
  cache1 = sym_cache[1];
  __frame.sym1 = pix1;
  __frame.sym2 = cache0;
  c4 = sym_cache[4];
  __frame.sym3 = cache1;
  c5 = sym_cache[5];
  __frame.sym4 = sym_cache[2];
  c6 = sym_cache[6];
  c7 = sym_cache[7];
  __frame.sym5 = sym_cache[3];
  up4 = (PixRec*)this->row_cur[6];
  __frame.sym6 = c4;
  h11 = up4[2].sym;
  __frame.sym7 = c5;
  r5 = this->row_cur[5];
  __frame.sym8 = c6;
  h10 = r5[-2].sym;
  __frame.sym9 = c7;
  __frame.sym10 = h10;
  up5 = this->row_cur[7];
  h12 = up5[1].sym;
  __frame.sym11 = h11;
  h13 = up5->sym;
  __frame.sym12 = h12;
  h14 = up4[-2].sym;
  __frame.sym13 = h13;
  h15 = up5[-1].sym;
  __frame.sym14 = h14;
  h16 = r5[-3].sym;
  __frame.sym15 = h15;
  h17 = up4[3].sym;
  __frame.sym16 = h16;
  h18 = up4[4].sym;
  __frame.sym17 = h17;
  h19 = r5[-4].sym;
  __frame.sym18 = h18;
  h20 = up4[-3].sym;
  __frame.sym19 = h19;
  h21 = up5[2].sym;
  __frame.sym20 = h20;
  __frame.sym21 = h21;
  r8c = this->row_cur[8];
  __frame.sym22 = r8c->sym;
  __frame.sym23 = up5[-2].sym;
  h24 = r5[-5].sym;
  h28 = r5[-7].sym;
  __frame.sym24 = h24;
  __frame.sym25 = r8c[1].sym;
  h26 = up4[5].sym;
  h30 = up4[7].sym;
  __frame.sym26 = h26;
  __frame.sym27 = this->row_cur[9]->sym;
  __frame.sym28 = h28;
  __frame.sym29 = r8c[-1].sym;
  h31 = up5[3].sym;
  this->sym_pos = 0;
  __frame.sym30 = h30;
  __frame.sym31 = h31;
  do {
    psym = (this)->pixel_context((uint32_t*)__frame.sym);
    if( psym>=0 ) {
      bit2 = this->bit_node[this->ctr_node].decode_context_bit(&this->bit_root[this->ctr_fallback]);
      this->row_cur[5]->sym = psym;
      if( bit2 )
        return idx_t+1;
      exclusion_mask[psym] = exclusion_gen;
    }
    pos1 = this->sym_pos+1;
    this->sym_pos = pos1;
  } while( pos1<32 );
  msym1b = ::mode_symbol[1];
  sel1_list = this->sel1_list;
  this->sel[0] = &this->sel0_list[::mode_symbol[2]];
  sel_p = this->sel_cur;
  this->sel[1] = &sel1_list[msym1b];
  while( 1 ) {
    if( sel_p>&this->escape_list )
      __exit_402E40(4);
    if( (*sel_p)->live ) {
      lsym = __decode_symbol_list(*sel_p);
      this->row_cur[5]->sym = lsym;
      if( lsym>=0 )
        return idx_t+1;
      sel_p = this->sel_cur;
    }
    this->sel_cur = ++sel_p;
  }
}

inline int32_t ModelBlock::code_pixel(int32_t x) {
  struct alignas(16) CodePixelFrame {
    union {
      uint32_t sym[32];
      struct {
        int32_t sym0;
        int32_t sym1;
        int32_t sym2;
        int32_t sym3;
        int32_t sym4;
        int32_t sym5;
        int32_t sym6;
        int32_t sym7;
        int32_t sym8;
        int32_t sym9;
        int32_t sym10;
        int32_t sym11;
        int32_t sym12;
        int32_t sym13;
        int32_t sym14;
        int32_t sym15;
        int32_t sym16;
        int32_t sym17;
        int32_t sym18;
        int32_t sym19;
        int32_t sym20;
        int32_t sym21;
        int32_t sym22;
        int32_t sym23;
        int32_t sym24;
        int32_t sym25;
        int32_t sym26;
        int32_t sym27;
        int32_t sym28;
        int32_t sym29;
        int32_t sym30;
        int32_t sym31;
      };
    };
    uint8_t _gap0[4];
    uint8_t _pad0[32];
  } __frame;
  int32_t runlen_s;
  PixRec* r4;
  uint8_t* alpha_map;
  PixRec* up9;
  PixRec* rowp;
  PixRec* row_cur9;
  bool up_eq_west;
  int8_t mode;
  uint16_t rev;
  int32_t arg_cum;
  FreqRec* frec;
  uint16_t* wr;
  PixRec* up4;
  uint16_t wp, g4, g3;
  SymList* sel1_list, ** sel_p;
  uint32_t arg_tot, arg_high, done, rec_word, grid_kind;
  PixRec* r1, * r2, * cur6b, * r3, * r5;
  FreqRec* binp, * binp_s;
  uint16_t* pixp;
  uint8_t match1, m0, m1, m2, m3;
  int32_t bp;
  uint16_t* id3p;
  int16_t s8, w1s, acc2, s3, g0;
  SymPair* pair;
  int32_t up_sym, left_sym, up_next_sym, upleft_sym, nb, key, ctx_state, pair_last, cap, pair_prev, ctx_bucket, up_match0, m_w1, nb2, sig1, id1, sig2, id2, id3_used, sig3, m_w1b, m_w0, m_up0, to_edge, one, run, run_pair, amap, runlen, run_hit, run_left, s1, s8b, s6, msym1, s4, bit5, first, s5, s1b, s9, cum1, lvl_a, w6a, w5a, b15a, msym3, excl_sym_a, excl_sym_b, w6d, wq1, cache2, cache1, cache3, cache4, cache6, cur5_back2, h11, h12, h13, h14, h15, h16, h17, h18, h19, h20, h21, h24, h28, h26, h30, esym, up_hit, pos1, msym1b, w6b, * ip, b15, w2t, w3t, w4t, acc, q1, w2s, q2, w3s, w4s, s9b, cum2, lvl_b, w5c, w6c, w5b, s3b;
  uint16_t w5, w1, h4, h3, h2, w1a, w0;
  PixRec* row;
  PixRec* cur6;
  PixRec* cur5, * cur5p1, * cur5p1b, * r6, * r7;
  uint16_t* wq, * sym_cache, w6;
  uint16_t cache0p;
  PixRec* cur2;
  BitCtr* ctr;
  uint32_t bin_tot, half, k, g2, g2h;
  PixRec* up3;
  PixRec* cur6c, * up2;
  cur6 = (PixRec*)this->row_cur[6];
  up_sym = cur6->sym;
  cur5 = this->row_cur[5];
  left_sym = cur5[-1].sym;
  up_next_sym = cur6[1].sym;
  upleft_sym = cur6[-1].sym;
  __frame.sym8 = up_sym;
  ::mode_symbol[1] = up_sym;
  __frame.sym10 = left_sym;
  ::mode_symbol[2] = left_sym;
  __frame.sym6 = up_next_sym;
  ::mode_symbol[3] = up_next_sym;
  __frame.sym5 = upleft_sym;
  mode_symbol[4] = upleft_sym;
  nb = 32*cur5[-1].match[2]+16*cur5[-1].match[4]+2*cur5[-1].match[0]+8*cur6[1].match[1]+(cur6->match[1]+4*(up_next_sym==upleft_sym));
  up_eq_west = up_sym==left_sym;
  if( up_eq_west ) {
    if( __frame.sym8==__frame.sym6 ) {
      rev = this->sym_rev[__frame.sym8];
      if( __frame.sym8==__frame.sym5 )
        key = (uint16_t)(rev-cur5[-2].sym);
      else
        key = (uint16_t)(rev-__frame.sym5);
    } else {
      key = (uint16_t)(this->sym_rev[__frame.sym8]-__frame.sym6);
    }
  } else {
    key = (uint16_t)(this->sym_rev[__frame.sym8]-__frame.sym10);
  }
  this->sym_cache = &this->sym_ctr[8*key];
  ctx_state = this->ctx_state[nb];
  this->ctx_state_seen = ctx_state;
  pair = &this->group_ctr[ctx_state][key];
  this->pix_cur = (uint16_t*)pair;
  pair_last = pair->last;
  if( pair_last==__frame.sym8 ) {
    cap = 15;
  } else if( pair_last==__frame.sym10 ) {
    cap = 30;
  } else if( pair_last==__frame.sym6 ) {
    cap = 45;
  } else {
    up_eq_west = pair_last==__frame.sym5;
    cap = 60;
    if( !up_eq_west )
      cap = 0;
  }
  pair_prev = pair->prev;
  if( pair_prev==__frame.sym8 ) {
    cap += 75;
  } else if( pair_prev==__frame.sym10 ) {
    cap += 150;
  } else if( pair_prev==__frame.sym6 ) {
    cap += 225;
  } else if( pair_prev==__frame.sym5 ) {
    cap += 300;
  }
  r1 = this->row_cur[8];
  ctx_bucket = this->ctx_bucket[ctx_state+cap];
  r2 = this->row_cur[7];
  this->bucket_idx = ctx_bucket;
  up_match0 = cur6->match[0];
  match1 = cur6->match[1];
  cur2 = cur5;
  __frame.sym0 = up_match0;
  up9 = this->row_cur[9];
  m_w1 = cur5[-1].match[1];
  nb2 = 8*cur5[-2].match[2]+4*cur5[-2].match[5]+m_w1+2*cur5[-2].match[4];
  sig1 = ((this->grad[3]==0)<<7)+((this->grad[2]==0)<<6)+32*(this->grad[1]==0)+16*(this->grad[0]==0)+((uint8_t)(up9->match[0]&r1->match[0]&__frame.sym0&r2->match[0])<<9)+((uint8_t)(up9->match[1]&r1->match[1]&r2->match[1]&match1)<<8)+(ctx_bucket<<10)+nb2;
  id1 = this->ctx_id1[sig1];
  cur6b = cur6;
  if( id1==0xFFFF ) {
    this->ctx_id1[sig1] = this->ctx_id1_used;
    cur6b = this->row_cur[6];
    cur5 = this->row_cur[5];
    ++*(int32_t*)&this->ctx_id1_used;
    id1 = this->ctx_id1[sig1];
    __frame.sym0 = cur6b->match[0];
  }
  sig2 = cur5[-1].match[5]+4*cur6b[1].match[3]+2*__frame.sym0+8*id1;
  id2 = this->ctx_id2[sig2];
  if( id2==0xFFFF ) {
    this->ctx_id2[sig2] = (this->ctx_id2_used)++;
    id2 = this->ctx_id2[sig2];
  }
  if( *(int32_t*)&this->alphabet<32 ) {
    id3_used = this->ctx_id3_used;
    sig3 = 16*id2+(__frame.sym10&0xF);
    id3p = &this->ctx_id3[sig3];
    id2 = *id3p;
    if( id2==0xFFFF ) {
      if( id3_used>53248 )
        sig3 |= 0xFu;
      id3p = &this->ctx_id3[sig3];
      id2 = *id3p;
    }
    if( id2>=id3_used ) {
      *id3p = id3_used;
      ++*(int32_t*)&this->ctx_id3_used;
      id2 = *id3p;
    }
  }
  row = this->row_cur[5];
  m_w1b = row[-1].match[1];
  m_w0 = row[-1].match[0];
  rowp = row;
  if( (m_w1b&m_w0)!=0&&(cur6c = this->row_cur[6], up2 = this->row_cur[7], ((uint8_t)(up2[2].match[1]&up2[1].match[1]&up2[0].match[1]&cur6c[3].match[1]&cur6c[2].match[1]&cur6c[1].match[1]&cur6c[0].match[1]&cur6c[0].match[0]&cur6c[-1].match[1])&up2[3].match[1])!=0) ) {
    m_up0 = up2[0].match[0];
    to_edge = this->width-x;
    __frame.sym4 = 1;
    if( to_edge<=1 ) {
      run = 1;
    } else {
      __frame.sym0 = to_edge;
      one = 1;
      while( 1 ) {
        run = one;
        if( (cur6c[one+2].match[1]&cur6c[one+2].match[0])==0 )
          break;
        m_up0 = (uint8_t)(up2[run].match[0]&m_up0);
        if( ++one>=__frame.sym0 ) {
          __frame.sym4 = one;
          run = one;
          goto LABEL_42;
        }
      }
      __frame.sym4 = one;
    }
LABEL_42:
    run_pair = (uint8_t)(up2[run+3].match[1]&up2[run+2].match[1]);
    __frame.sym5 = this->run_bucket[__frame.sym4];
    amap = this->alpha_map[__frame.sym8]+8*__frame.sym5+4*run_pair+2*m_up0;
    runlen = 0;
    if( rowp->sym==__frame.sym8 ) {
      do
        ++runlen;
      while( runlen<__frame.sym4&&rowp[runlen].sym==__frame.sym8 );
    }
    run_hit = runlen==__frame.sym4;
    __frame.sym6 = run_hit;
    if( runlen>run_hit ) {
      this->row_cur[6] = &cur6c[runlen-run_hit];
      this->row_cur[7] = &up2[runlen-run_hit];
      row_cur9 = this->row_cur[9];
      this->row_cur[8] = this->row_cur[8]+runlen-run_hit;
      this->row_cur[9] = row_cur9+runlen-run_hit;
      rowp->match2345 = 0x01010101;
      *(uint32_t*)this->row_cur[5] = 0x01010101;
      this->pix_cur[1] = this->pix_cur[0];
      wp = __frame.sym8;
      this->row_cur[5]->sym = __frame.sym8;
      this->pix_cur[0] = wp;
      r3 = this->row_cur[5];
      rec_word = *(uint32_t*)r3;
      __frame.sym1 = r3->match2345;
      cur2 = r3+1;
      this->row_cur[5] = r3+1;
      if( runlen-run_hit!=1 ) {
        run_left = runlen-run_hit-1;
        half = run_left/2;
        if( run_left/2 ) {
          __frame.sym0 = run_left;
          s1 = __frame.sym1;
          runlen_s = runlen;
          k = 0;
          __frame.sym3 = amap;
          s8 = __frame.sym8;
          do {
            this->pix_cur[1] = s8;
            *(uint32_t*)this->row_cur[5] = rec_word;
            this->row_cur[5]->match2345 = s1;
            pixp = this->pix_cur;
            ++this->row_cur[5];
            pixp[1] = s8;
            *(uint32_t*)this->row_cur[5] = rec_word;
            this->row_cur[5]->match2345 = s1;
            cur5p1 = this->row_cur[5]+1;
            this->row_cur[5] = cur5p1;
            ++k;
          } while( k<half );
          run_left = __frame.sym0;
          amap = __frame.sym3;
          cur2 = cur5p1;
          runlen = runlen_s;
          done = 2*k+1;
        } else {
          done = 1;
        }
        if( (uint32_t)run_left>(done-1) ) {
          this->pix_cur[1] = __frame.sym8;
          *(uint32_t*)this->row_cur[5] = rec_word;
          this->row_cur[5]->match2345 = __frame.sym1;
          cur5p1b = this->row_cur[5]+1;
          this->row_cur[5] = cur5p1b;
          cur2 = cur5p1b;
        }
      }
      r4 = (PixRec*)this->row_cur[6];
      m0 = r4[-2].match[0];
      m1 = r4[2].match[0];
      m2 = r4[4].match[0];
      __frame.sym3 = amap;
      m3 = r4[-3].match[0];
      runlen_s = runlen;
      up3 = (PixRec*)this->row_cur[7];
      this->grad[0] = (r4[3].match[0]+m1+m0+m3+m2-5);
      this->grad[1] = up3[3].match[0]+up3[2].match[0]+up3[1].match[0]+up3[0].match[0]+(int8_t)up3[-1].match[0]+(int8_t)up3[-2].match[0]+(int8_t)up3[-3].match[0]+up3[4].match[0]-8;
      this->grad[2] = cur2[-4].match[1]+cur2[-3].match[1]-2;
      s8b = __frame.sym8;
      this->grad[3] = cur2[-5].match[0]+cur2[-6].match[0]+cur2[-7].match[0]+cur2[-4].match[0]-4;
      cur2[-1].match[4] = s8b==r4[1].sym;
      amap = __frame.sym3;
      this->row_cur[5][-1].match[5] = s8b==this->row_cur[6][2].sym;
      runlen = runlen_s;
    }
    (&this->esc_ctr[(amap+1)])->encode_context_bit(this->esc_ctr, __frame.sym6);
    s6 = __frame.sym6;
    msym1 = ::mode_symbol[1];
    alpha_map = (uint8_t*)this->alpha_map;
    this->hit = __frame.sym6;
    *(alpha_map+msym1) = s6;
    if( !s6&&__frame.sym4!=1 ) {
      runlen_s = runlen;
      __frame.sym0 = __frame.sym5;
      s4 = __frame.sym4;
      bit5 = 1<<(__frame.sym5&31);
      first = 0;
      s5 = __frame.sym5;
      do {
        if( s4>(bit5|first) ) {
          __frame.sym3 = first;
          ctr = &this->run_ctr[16*((first==0)+(s5==__frame.sym0))+s5];
          __frame.sym1 = runlen_s&bit5;
          bin_tot = ctr->n[0]+ctr->n[1];
          rc.encode_bit(ctr->n[0], ctr->n[1], (runlen_s&bit5)!=0);
          if( ctr->limit<(uint32_t)bin_tot )
            __rescale_counter_pair(ctr);
          s1b = __frame.sym1;
          __frame.sym4 = bit5;
          ctr->n[__frame.sym1!=0] += 8;
          first |= s1b;
          bit5 = __frame.sym4;
        }
        --s5;
        bit5 >>= 1;
      } while( bit5 );
      runlen = runlen_s;
    }
    if( *(int32_t*)&this->hit )
      return runlen;
  } else {
    binp = &this->grid[this->bucket_idx];
    frec = &this->grid[id2+188];
    grid_kind = frec->w[5];
    if( grid_kind ) {
      if( grid_kind==1 ) {
        ip = (int32_t*)&this->grid[this->bucket_idx];
        b15 = binp->b15;
        w2t = b15*frec->w[2];
        w3t = b15*frec->w[3];
        __frame.sym0 = b15*frec->w[0];
        w1s = b15*frec->w[1];
        __frame.sym1 = w2t;
        w4t = b15*frec->w[4];
        __frame.sym2 = w3t;
        __frame.sym3 = w4t;
        *frec = *(FreqRec*)ip;
        w5 = frec->w[5];
        frec->b14 *= 8;
        acc = 21*frec->w[1];
        __frame.sym0 += (21*frec->w[0]+w5-1)/w5;
        frec->w[0] = __frame.sym0;
        q1 = (acc+w5-1)/w5;
        LOWORD(acc) = __frame.sym1;
        w2s = 21*frec->w[2];
        w6 = q1+w1s;
        frec->w[1] = w6;
        q2 = (w2s+w5-1)/w5;
        w3s = 21*frec->w[3];
        LOWORD(acc) = q2+acc;
        frec->w[2] = acc;
        bp = __frame.sym2+(w3s+w5-1)/w5;
        w4s = 21*frec->w[4];
        frec->w[3] = (uint16_t)bp;
        acc2 = (uint16_t)bp+acc+w6;
        w5 = (w4s+w5-1)/w5+__frame.sym3;
        frec->w[4] = w5;
        grid_kind = (uint16_t)(__frame.sym0+w5+acc2);
        frec->w[5] = grid_kind;
        rowp = this->row_cur[5];
      }
      s9 = rowp->sym;
      arg_tot = grid_kind;
      if( s9==__frame.sym8 ) {
        cum1 = frec->w[0];
        lvl_a = 1;
      } else if( s9==__frame.sym10 ) {
        cum1 = frec->w[0]+frec->w[1];
        lvl_a = 2;
      } else if( s9==__frame.sym6 ) {
        cum1 = frec->w[0]+frec->w[2]+frec->w[1];
        lvl_a = 3;
      } else if( s9==__frame.sym5 ) {
        cum1 = frec->w[5]-frec->w[4];
        lvl_a = 4;
      } else {
        cum1 = 0;
        lvl_a = 0;
      }
      w6a = frec->w[6];
      arg_cum = cum1;
      arg_high = frec->w[lvl_a]+cum1;
      w5a = frec->w[5];
      b15a = frec->b15;
      if( w5a>w6a&&(frec->w[lvl_a]+b15a+8<w5a||frec->w[5]>0x4000u) ) {
        h2 = frec->w[2];
        __frame.sym0 = w6a;
        w1a = frec->w[1];
        __frame.sym1 = lvl_a;
        w0 = frec->w[0];
        __frame.sym2 = b15a;
        w0 = w0-(w0>>1);
        frec->w[0] = w0;
        w1a = w1a-(w1a>>1);
        frec->w[1] = w1a;
        h2 = h2-(h2>>1);
        h3 = frec->w[3];
        frec->w[2] = h2;
        h3 = h3-(h3>>1);
        h4 = frec->w[4];
        frec->w[3] = h3;
        h4 = h4-(h4>>1);
        frec->w[4] = h4;
        w1a = h3+h2+w1a;
        b15a = __frame.sym2;
        w1a = w0+h4+w1a;
        lvl_a = __frame.sym1;
        w5a = w1a;
        w6b = __frame.sym0;
        frec->w[5] = w5a;
        if( w6b<256&&!frec->b14 ) {
          w6b = 256;
          frec->w[6] = 256;
        }
        if( w5a>w6b ) {
          if( b15a<15 )
            b15a = 15;
          frec->b15 = b15a;
        }
      }
      frec->w[5] = b15a+w5a;
      frec->w[lvl_a] += b15a;
      rc.encode(arg_cum, arg_high, arg_tot);
      this->hit = lvl_a;
      if( frec->b14 ) {
        --frec->b14;
        binp_s = binp;
        ++binp->w[5];
        ++binp_s->w[lvl_a];
        lvl_a = this->hit;
      }
    } else {
      wr = (uint16_t*)&this->grid[this->bucket_idx];
      s9b = rowp->sym;
      arg_tot = binp->w[5];
      if( s9b==__frame.sym8 ) {
        cum2 = *wr;
        lvl_b = 1;
      } else if( s9b==__frame.sym10 ) {
        cum2 = *wr+wr[1];
        lvl_b = 2;
      } else if( s9b==__frame.sym6 ) {
        cum2 = *wr+wr[2]+wr[1];
        lvl_b = 3;
      } else if( s9b==__frame.sym5 ) {
        cum2 = wr[5]-wr[4];
        lvl_b = 4;
      } else {
        cum2 = 0;
        lvl_b = 0;
      }
      arg_cum = cum2;
      arg_high = binp->w[lvl_b]+cum2;
      w5c = binp->w[5];
      w6c = binp->w[6];
      __frame.sym3 = binp->b15;
      if( w5c>w6c&&(binp->w[lvl_b]+__frame.sym3+8<w5c||w5c>0x4000) ) {
        __frame.sym0 = w6c;
        w1 = binp->w[1];
        g2 = binp->w[2];
        __frame.sym2 = lvl_b;
        g0 = binp->w[0]-(binp->w[0]>>1);
        binp->w[0] = g0;
        w1 = w1-(w1>>1);
        binp->w[1] = w1;
        g2h = g2-(g2>>1);
        g3 = (uint16_t)binp->w[3];
        binp->w[2] = g2h;
        g3 = g3-(g3>>1);
        g4 = (uint16_t)binp->w[4];
        binp->w[3] = g3;
        g4 = g4-(g4>>1);
        binp->w[4] = g4;
        g4 = g0+g4;
        lvl_b = __frame.sym2;
        w5c = (uint16_t)(g4+g3+g2h+w1);
        w5b = __frame.sym0;
        binp->w[5] = w5c;
        if( w5b<256&&!binp->b14 ) {
          w5b = 256;
          binp->w[6] = 256;
        }
        if( w5c>w5b ) {
          s3b = __frame.sym3;
          if( __frame.sym3<15 )
            s3b = 15;
          __frame.sym3 = s3b;
          binp->b15 = s3b;
        }
      }
      s3 = __frame.sym3;
      binp->w[5] = __frame.sym3+w5c;
      binp->w[lvl_b] += s3;
      rc.encode(arg_cum, arg_high, arg_tot);
      this->hit = lvl_b;
      frec->w[5] = (frec->w[lvl_b])++!=0;
      lvl_a = this->hit;
    }
    if( lvl_a )
      return 1;
    runlen = 0;
  }
  mode = exclusion_gen;
  msym3 = ::mode_symbol[3];
  excl_sym_a = ::mode_symbol[2];
  excl_sym_b = ::mode_symbol[1];
  exclusion_mask[mode_symbol[4]] = exclusion_gen;
  exclusion_mask[msym3] = mode;
  wq = (uint16_t*)this->pix_cur;
  exclusion_mask[excl_sym_a] = mode;
  exclusion_mask[excl_sym_b] = mode;
  sym_cache = this->sym_cache;
  __byte_445440[0] = mode;
  runlen_s = runlen;
  this->sel[0] = nullptr;
  w6d = *wq;
  wq1 = wq[1];
  cache0p = *sym_cache;
  cache2 = sym_cache[2];
  __frame.sym0 = w6d;
  cache1 = sym_cache[1];
  __frame.sym1 = wq1;
  cache3 = sym_cache[3];
  __frame.sym2 = cache0p;
  cache4 = sym_cache[4];
  __frame.sym3 = cache1;
  __frame.sym4 = cache2;
  cache6 = sym_cache[6];
  __frame.sym5 = cache3;
  r5 = this->row_cur[5];
  __frame.sym6 = cache4;
  cur5_back2 = r5[-2].sym;
  __frame.sym7 = sym_cache[5];
  up4 = (PixRec*)this->row_cur[6];
  __frame.sym8 = cache6;
  h11 = up4[2].sym;
  __frame.sym9 = sym_cache[7];
  __frame.sym10 = cur5_back2;
  r6 = this->row_cur[7];
  h12 = r6[1].sym;
  __frame.sym11 = h11;
  h13 = r6->sym;
  __frame.sym12 = h12;
  h14 = up4[-2].sym;
  __frame.sym13 = h13;
  h15 = r6[-1].sym;
  __frame.sym14 = h14;
  h16 = r5[-3].sym;
  __frame.sym15 = h15;
  h17 = up4[3].sym;
  __frame.sym16 = h16;
  h18 = up4[4].sym;
  __frame.sym17 = h17;
  h19 = r5[-4].sym;
  __frame.sym18 = h18;
  h20 = up4[-3].sym;
  __frame.sym19 = h19;
  h21 = r6[2].sym;
  __frame.sym20 = h20;
  __frame.sym21 = h21;
  r7 = this->row_cur[8];
  __frame.sym22 = r7->sym;
  __frame.sym23 = r6[-2].sym;
  h24 = r5[-5].sym;
  h28 = r5[-7].sym;
  __frame.sym24 = h24;
  __frame.sym25 = r7[1].sym;
  h26 = up4[5].sym;
  h30 = up4[7].sym;
  __frame.sym26 = h26;
  __frame.sym27 = this->row_cur[9]->sym;
  __frame.sym28 = h28;
  __frame.sym29 = r7[-1].sym;
  __frame.sym30 = h30;
  __frame.sym31 = r6[3].sym;
  this->sym_pos = 0;
  do {
    esym = (this)->pixel_context((uint32_t*)__frame.sym);
    if( esym>=0 ) {
      up_hit = esym==this->row_cur[5]->sym;
      (&this->bit_node[*(int32_t*)&this->ctr_node])->encode_context_bit(&this->bit_root[*(int32_t*)&this->ctr_fallback], up_hit);
      if( up_hit )
        return runlen_s+1;
      exclusion_mask[esym] = exclusion_gen;
    }
    pos1 = this->sym_pos+1;
    this->sym_pos = pos1;
  } while( pos1<32 );
  msym1b = ::mode_symbol[1];
  sel1_list = this->sel1_list;
  this->sel[0] = &this->sel0_list[::mode_symbol[2]];
  sel_p = this->sel_cur;
  this->sel[1] = &sel1_list[msym1b];
  while( 1 ) {
    if( (*sel_p)->live ) {
      if( (*sel_p)->code_symbol(this->row_cur[5]->sym) )
        return runlen_s+1;
      sel_p = this->sel_cur;
    }
    this->sel_cur = ++sel_p;
  }
}

inline void ModelBlock::expand_alphabet() {
  SymList lists[16];
  uint32_t* codes_p;
  uint32_t nbytes, bits;
  void* codes;
  int32_t left2, left, run, gap;
  uint32_t mask, i, cap, n_1, n_syms, j, k, carry, s, b, piece, s2;
  bits = this->depth;
  mask = 0xFFFFFFFF>>(-(uint8_t)this->depth&31);
  nbytes = (bits+7)>>3;
  for( i = 0; i<8; ++i ) {
    lists[2*i].ent = nullptr;
    lists[2*i+1].ent = nullptr;
  }
  cap = mask+1;
  if( bits>8 )
    cap = 8193;
  n_1 = __rc_decode_flat(cap);
  this->alphabet = n_1+1;
  if( (int32_t)(n_1+1)<=0x2000 ) {
    codes = bmf_new(sizeof(uint32_t)*(n_1+1));
    n_syms = this->alphabet;
    this->sym_code = (uint32_t*)codes;
    if( n_syms ) {
      for( j = 0; j<n_syms; ++j ) {
        this->sym_code[j] = j;
        n_syms = this->alphabet;
      }
    }
    if( (int32_t)this->depth>8 ) {
      if( 4*nbytes ) {
        k = 0;
        do
          __init_symbol_list(&lists[k++], 256, 1);
        while( k<(4*nbytes) );
        n_syms = this->alphabet;
      }
      if( n_syms ) {
        codes_p = this->sym_code;
        carry = 0;
        s = 0;
        do {
          codes_p[s] = 0;
          if( nbytes ) {
            b = 0;
            do {
              piece = __decode_symbol_list(&lists[4*b+carry]);
              carry = piece>>6;
              this->sym_code[s] += (piece<<((8*b)&31));
              ++b;
            } while( b<nbytes );
          }
          codes_p = this->sym_code;
          carry = (uint8_t)codes_p[s++]>>7;
        } while( s<this->alphabet );
      }
    } else if( n_syms<=mask ) {
      __init_symbol_list(&lists[0], mask-n_syms+2, 1);
      lists[0].rescale_at = 19*lists[0].n;
      if( !(this->alphabet==0) ) {
        run = 0;
        s2 = 0;
        do {
          gap = __decode_symbol_list(&lists[0]);
          this->sym_code[s2] = gap+run;
          run += gap+1;
          ++s2;
        } while( s2<this->alphabet );
      }
    }
    for( left = 15; left>=0; --left )
      free(lists[left].ent);
  } else {
    this->depth = 8;
    this->height = (this->height*nbytes);
    this->expand_alphabet();
    for( left2 = 15; left2>=0; --left2 )
      free(lists[left2].ent);
  }
}

ModelBlock*__layout_workspace(ModelBlock* blk, int32_t unread_flag, int32_t img_w, int32_t img_h, int32_t img_depth) {
  PixRec* buf;
  uint8_t bucket;
  uint16_t rev;
  uint32_t e0, e1, w;
  uint8_t* runs;
  int32_t j, r, x, bits;
  uint32_t k, m, s, n, i8, i24;
  w = img_w;
  exclusion_gen = 1;
  blk->width = img_w;
  blk->height = img_h;
  blk->depth = img_depth;
  blk->depth_raw = img_depth;
  blk->escape.ent = nullptr;
  blk->sym_code = nullptr;
  for( j = 0; j<5; ++j ) {
    buf = (PixRec*)bmf_new(sizeof(PixRec)*(w+16));
    blk->row_cur[j] = buf;
    blk->row_cur[j+5] = buf+8;
    w = blk->width;
    if( (int32_t)blk->width>-16 ) {
      r = 0;
      do {
        blk->row_cur[j][r].sym = 0;
        blk->row_cur[j][r].match[5] = 1;
        blk->row_cur[j][r].match[4] = 1;
        blk->row_cur[j][r].match[3] = 1;
        blk->row_cur[j][r].match[2] = 1;
        blk->row_cur[j][r].match[1] = 1;
        blk->row_cur[j][r].match[0] = 1;
        w = blk->width;
        ++r;
      } while( (uint32_t)r<blk->width+16 );
    }
  }
  runs = (uint8_t*)bmf_new(w+1);
  blk->run_bucket = runs;
  *runs = 0;
  if( (int32_t)blk->width>0 ) {
    bucket = 0;
    x = 0;
    do {
      bucket += x==2<<(bucket&31);
      blk->run_bucket[x+++1] = bucket;
    } while( (uint32_t)x<blk->width );
  }
  __builtin_memset(blk->sym_rev, 0, sizeof blk->sym_rev);
  for( k = 0; k<0x2000; ++k ) {
    rev = blk->sym_rev[k];
    bits = k;
    for( m = 0; m<0xD; ++m ) {
      rev += rev+(bits&1);
      bits >>= 1;
    }
    blk->sym_rev[k] = rev;
  }
  for( s = 0; s<0x2000; ++s )
    blk->sym_rev[s] *= 8;
  memset(&blk->grid[188], 0, 0x100000);
  blk->ctx_id3_used = 0;
  blk->ctx_id2_used = 0;
  blk->ctx_id1_used = 0;
  memset(blk->ctx_id1, 255, sizeof blk->ctx_id1);
  memset(blk->ctx_id2, 255, sizeof blk->ctx_id2);
  memset(blk->ctx_id3, 255, sizeof blk->ctx_id3);
  memset(exclusion_mask, 0, 8193);
  blk->sel[0] = nullptr;
  blk->sel[1] = nullptr;
  blk->escape_list = nullptr;
  for( n = 0; n<0x40000; ++n ) {
    blk->sym_ctr[2*n] = 0x2000;
    blk->sym_ctr[2*n+1] = 0x2000;
  }
  i8 = 0;
  do {
    e0 = 2*i8;
    blk->bit_root[e0].n[0] = 40;
    ++i8;
    blk->bit_root[e0].n[1] = 16;
    blk->bit_root[e0].limit = 512;
    blk->bit_root[e0+1].n[0] = 40;
    blk->bit_root[e0+1].n[1] = 16;
    blk->bit_root[e0+1].limit = 512;
  } while( i8<8 );
  i24 = 0;
  memset(blk->bit_node, 0, sizeof blk->bit_node);
  blk->sym_word = (uint16_t*)bmf_new(2*blk->height*blk->width);
  blk->esc_ctr[0].n[0] = 4;
  blk->esc_ctr[0].n[1] = 4;
  blk->esc_ctr[0].limit = 72;
  memset(&blk->esc_ctr[1], 0, 1536);
  do {
    e1 = 2*i24;
    blk->run_ctr[e1].n[0] = 4;
    ++i24;
    blk->run_ctr[e1].n[1] = 4;
    blk->run_ctr[e1].limit = 72;
    blk->run_ctr[e1+1].n[0] = 4;
    blk->run_ctr[e1+1].n[1] = 4;
    blk->run_ctr[e1+1].limit = 72;
  } while( i24<0x18 );
  return blk;
}

inline void ModelBlock::unmodel_plane_slow(uint8_t* dst) {
  uint8_t* row[19];
  uint32_t alpha_n;
  ModelBlock* this_1, * blk;
  int16_t lvl;
  uint32_t has4, jj, wt, nbytes, row_w, * out32;
  uint16_t* out16;
  SymEntry* out_ent;
  uint8_t* out_at, * dst_keep;
  int32_t g0, y0;
  int32_t done, f_b0, f_b3, f_b4, f_b5;
  int32_t f_b1, lo1, m5, g1;
  uint32_t* x6;
  uint8_t* dst_base, * dst_buf, * buf, * expand_buf, * out_bits, * row_at, * interleave_at, * p, * out8;
  PixRec* kk, * row_cur3, * row_cur2, * row_cur1, * n_syms;
  int32_t g, flags, lo, k, w2, w4, w2n, lvl_n, live, gi, s, bucket, x, x2, y, step, x7, x3, x4, bits, depth, y2, depth_raw, nchunk, n_pix, chunk, q5, q1, at, q2, written, n_pix2, x5;
  SymListBlock* has3, * alpha;
  SymPair* group_ctr;
  SymList* list_a, * i, * list_b, * j;
  FreqRec* rec;
  PixRec* lists, * t;
  dst_keep = dst;
  dst_base = &dst[-(this->depth<8)];
  __rc_begin_decode(0);
  (this)->expand_alphabet();
  this_1 = (this);
  g1 = 0;
  g = 0;
  do {
    flags = ctx_group_flags[g];
    this_1->ctx_state[flags] = g;
    lo1 = 0;
    g0 = g;
    y0 = flags&4;
    f_b1 = flags&2;
    f_b4 = flags&0x10;
    lo = 0;
    f_b0 = flags&1;
    f_b5 = flags&0x20;
    f_b3 = flags&8;
    do {
      lo1 = lo;
      k = 0;
      do {
        this_1->ctx_bucket[g+15*lo+75*k] = g1;
        m5 = this_1->alphabet;
        rec = &this_1->grid[g1];
        rec->w[1] = 2;
        rec->w[2] = 2;
        rec->w[3] = 2;
        rec->w[4] = 2;
        if( y0 ) {
          w2 = (uint16_t)(rec->w[4]+rec->w[3]);
          rec->w[3] = w2;
          w4 = 0;
          rec->w[4] = 0;
        } else {
          w2 = rec->w[3];
          w4 = rec->w[4];
        }
        if( f_b1 ) {
          w2n = (uint16_t)(w4+rec->w[2]);
          rec->w[2] = w2n;
          w4 = 0;
          rec->w[4] = 0;
        } else {
          w2n = rec->w[2];
        }
        if( f_b4 ) {
          w2n = (uint16_t)(w2+w2n);
          rec->w[2] = w2n;
          w2 = 0;
          rec->w[3] = 0;
        }
        if( f_b0 ) {
          rec->w[1] += w4;
          w4 = 0;
          rec->w[4] = 0;
        }
        if( f_b3 ) {
          rec->w[1] += w2;
          w2 = 0;
          rec->w[3] = 0;
        }
        if( f_b5 ) {
          rec->w[1] += w2n;
          w2n = 0;
          rec->w[2] = 0;
        }
        lvl_n = (w2!=0)+(w2n!=0)+(w4!=0)+2;
        if( lvl_n<=m5 ) {
          rec->b14 = lvl_n;
          rec->w[0] = 2;
        } else {
          lvl_n = lvl_n-1;
          rec->b14 = lvl_n;
          rec->w[0] = 0;
        }
        if( rec->w[lo1]&&rec->w[k]&&(uint8_t)lvl_n<=m5 ) {
          live = 1;
          lvl = (uint8_t)(1<<((5-lvl_n)&31));
          rec->b15 = lvl;
          rec->w[6] = lvl<<6;
          rec->w[lo1] += lvl;
          rec->w[k] += rec->b15;
          rec->w[5] = rec->w[0]+rec->w[4]+rec->w[3]+rec->w[2]+rec->w[1];
        } else {
          live = 0;
        }
        g1 += live;
        ++k;
      } while( k<5 );
      lo = lo1+1;
    } while( lo1+1<5 );
    gi = 0;
    group_ctr = this_1->group_ctr[g0];
    do {
      group_ctr[gi].last = 0x2000;
      group_ctr[gi++].prev = 0x2000;
    } while( gi<0x10000 );
    g = g0+1;
  } while( g0+1<15 );
  dst_buf = dst_base;
  blk = (ModelBlock*)(this_1);
  buf = (uint8_t*)bmf_new(this_1->alphabet);
  alpha_n = this_1->alphabet;
  this_1->alpha_map = (uint8_t*)buf;
  memset(buf, 1, alpha_n);
  blk->escape_list = &blk->escape;
  __init_symbol_list(&blk->escape, blk->alphabet, 1);
  blk->sel_cur = blk->sel;
  wt = blk->alphabet;
  has3 = (SymListBlock*)bmf_new(SymListBlock::bytes(wt));
  if( has3 ) {
    has3->n = wt;
    list_a = has3->list;
    for( i = list_a; wt; --wt )
      (list_a++)->ent = nullptr;
  } else {
    i = nullptr;
  }
  has4 = blk->alphabet;
  blk->sel1_list = i;
  alpha = (SymListBlock*)bmf_new(SymListBlock::bytes(has4));
  if( alpha ) {
    alpha->n = has4;
    list_b = alpha->list;
    for( j = list_b; has4; --has4 )
      (list_b++)->ent = nullptr;
  } else {
    j = nullptr;
  }
  blk->sel0_list = j;
  if( !(blk->alphabet<=0) ) {
    s = 0;
    do {
      __init_symbol_list(&blk->sel1_list[s], 99, 0);
      __init_symbol_list(&blk->sel0_list[s++], 33, 0);
    } while( (uint32_t)s<blk->alphabet );
  }
  jj = blk->depth;
  if( (uint32_t)jj==blk->depth_raw ) {
    expand_buf = nullptr;
  } else {
    dst_buf = (uint8_t*)bmf_new(blk->height*blk->width+3);
    jj = blk->depth;
    expand_buf = dst_buf;
  }
  nbytes = (jj+7)>>3;
  if( blk->height>0 ) {
    out_at = dst_buf;
    bucket = 0;
    while( 1 ) {
      blk->row_cur[5]->match[1] = blk->row_cur[5][-1].sym==0;
      blk->row_cur[5]->match[3] = blk->row_cur[6][-1].sym==0;
      kk = blk->row_cur[4];
      row_cur3 = blk->row_cur[3];
      row_cur2 = blk->row_cur[2];
      row_cur1 = blk->row_cur[1];
      n_syms = blk->row_cur[0];
      blk->row_cur[4] = row_cur3;
      blk->row_cur[3] = row_cur2;
      blk->row_cur[2] = row_cur1;
      blk->row_cur[1] = n_syms;
      blk->row_cur[0] = kk;
      kk += 7;
      blk->row_cur[5] = kk;
      n_syms += 7;
      blk->row_cur[6] = n_syms;
      blk->row_cur[7] = row_cur1+7;
      blk->row_cur[8] = row_cur2+7;
      blk->row_cur[9] = row_cur3+7;
      {
        uint8_t zero = n_syms[1].sym==0;
        kk->match[2] = zero;
        blk->row_cur[5][-1].match[4] = zero;
        blk->row_cur[5][-2].match[5] = zero;
        zero = blk->row_cur[6][2].sym==0;
        blk->row_cur[5]->match[4] = zero;
        blk->row_cur[5][-1].match[5] = zero;
      }
      blk->row_cur[5]->match[5] = blk->row_cur[6][3].sym==0;
      lists = blk->row_cur[6];
      t = blk->row_cur[7];
      ++blk->row_cur[5];
      ++lists;
      ++blk->row_cur[8];
      blk->row_cur[6] = lists;
      ++t;
      blk->row_cur[7] = t;
      ++blk->row_cur[9];
      blk->grad[0] = lists[3].match[0]+lists[2].match[0]+lists[1].match[0]+lists[0].match[0]+lists[4].match[0]-5;
      blk->grad[3] = 0;
      blk->grad[2] = 0;
      row_w = blk->width;
      blk->grad[1] = t[3].match[0]+t[2].match[0]+t[1].match[0]+t[0].match[0]+t[4].match[0]-5;
      x = bucket;
      if( row_w<=0 )
        break;
      x2 = 0;
      do {
        y = ((ModelBlock*)blk)->decode_pixel(x2);
        blk->init_tables();
        row_w = blk->width;
        x2 += y;
      } while( (uint32_t)x2<blk->width );
      x = bucket;
      if( nbytes!=4 )
        goto LABEL_53;
      if( row_w>0 ) {
        out32 = (uint32_t*)out_at;
        y2 = 0;
        do
          *out32++ = blk->sym_code[blk->row_cur[0][y2+++8].sym];
        while( (uint32_t)y2<blk->width );
        out_at = (uint8_t*)out32;
        goto LABEL_74;
      }
LABEL_74:
      bucket = x+1;
      if( (uint32_t)bucket>=blk->height )
        goto LABEL_76;
    }
    if( nbytes==4 )
      goto LABEL_74;
LABEL_53:
    if( nbytes==3 ) {
      if( row_w>0 ) {
        out_ent = (SymEntry*)out_at;
        step = 0;
        do {
          x6 = blk->sym_code;
          x7 = blk->row_cur[0][step+8].sym;
          out_ent->set((uint16_t)x6[x7], (uint8_t)(x6[x7]>>16));
          ++step;
          ++out_ent;
        } while( (uint32_t)step<blk->width );
        out_at = (uint8_t*)out_ent;
      }
      goto LABEL_74;
    }
    if( nbytes!=2 ) {
      if( blk->depth==8 ) {
        if( row_w>0 ) {
          out8 = out_at;
          x5 = 0;
          do
            *out8++ = (uint8_t)blk->sym_code[blk->row_cur[0][x5+++8].sym];
          while( (uint32_t)x5<blk->width );
          out_at = out8;
        }
      } else if( row_w>0 ) {
        x4 = 0;
        bits = 0;
        out_bits = out_at;
        do {
          depth = blk->depth;
          bits -= depth;
          if( bits<0 ) {
            bits = 8-depth;
            *++out_bits = blk->sym_code[blk->row_cur[0][x4+8].sym]<<((8-depth)&31);
          } else {
            *out_bits |= blk->sym_code[blk->row_cur[0][x4+8].sym]<<(bits&31);
          }
          ++x4;
        } while( (uint32_t)x4<blk->width );
        out_at = out_bits;
      }
      goto LABEL_74;
    }
    if( row_w>0 ) {
      out16 = (uint16_t*)out_at;
      x3 = 0;
      do
        *out16++ = blk->sym_code[blk->row_cur[0][x3+++8].sym];
      while( (uint32_t)x3<blk->width );
      out_at = (uint8_t*)out16;
    }
    goto LABEL_74;
  }
LABEL_76:
  __rc_end_decode();
  depth_raw = blk->depth_raw;
  if( (uint32_t)depth_raw!=blk->depth ) {
    nchunk = (depth_raw+7)>>3;
    if( nchunk<=0 ) {
      n_pix = blk->height*blk->width;
    } else {
      done = 0;
      n_pix = blk->height*blk->width;
      chunk = n_pix/nchunk;
      if( nchunk>=6 ) {
        q5 = 0;
        row_at = expand_buf;
        do {
          row[q5] = row_at;
          row_at += 5*chunk;
          row[q5+1] = &expand_buf[chunk*(q5+1)];
          row[q5+2] = &expand_buf[chunk*(q5+2)];
          row[q5+3] = &expand_buf[chunk*(q5+3)];
          row[q5+4] = &expand_buf[chunk*(q5+4)];
          q5 += 5;
        } while( q5<=nchunk-6 );
        done = q5;
      }
      q1 = done;
      at = chunk*done;
      do {
        row[q1] = &expand_buf[at];
        at += chunk;
        ++q1;
      } while( q1<nchunk );
    }
    if( n_pix>0 ) {
      interleave_at = dst_keep;
      q2 = 0;
      written = 0;
      do {
        p = row[q2];
        *interleave_at = *p;
        n_pix2 = blk->height*blk->width;
        ++interleave_at;
        row[q2++] = p+1;
        if( q2==nchunk )
          q2 = 0;
        ++written;
      } while( written<n_pix2 );
    }
    free(expand_buf);
  }
}

int32_t __alt_model_p1_encode(uint16_t* hdr, uint8_t* src) {
  P1Ctx* b4, * buf3, * buf2, * buf1, * b0, * cursor0;
  int8_t dc3;
  uint8_t out3;
  int32_t resid3, cur3;
  void* plane[4];
  AltP1Block*&plane1 = (AltP1Block*&)plane[1];
  AltP1Block*&plane2 = (AltP1Block*&)plane[2];
  AltP1Block*&plane3 = (AltP1Block*&)plane[3];
  int32_t dc1;
  uint32_t x, y;
  AltP1Block* blk0;
  void** q;
  int32_t out1, resid0, off1, off0, resid2, cur0, off2, cur1, code1, off3;
  AltP1Block* blk_k, * raw;
  uint8_t fl1, fl2, fl3, pred1, pred2, pred3;
  AltP1Block* made;
  int32_t width, height, k, src1, src2, src3, dc2, xf1, xf2, xf3, w, n_planes, p, want0, keep0, code0, recon0, out0, drift0, err0, resid1, at1, recon1, drift1, code1x, want2, code2, recon2, out2, drift2, at2, code3, recon3, drift3, np, f;
  int64_t err1, err2, err3;
  AltP1Block* blk1, * blk2, * blk3;
  P1Ctx* cursor2, * cursor4;
  uint8_t pred0, want1, want3;
  width = *hdr;
  height = hdr[1];
  if( plane_count>0 ) {
    k = 0;
    do {
      raw = (AltP1Block*)bmf_new(sizeof(AltP1Block));
      if( raw )
        made = raw->alt_p1_alloc(width, height, k);
      else
        made = nullptr;
      plane[k++] = made;
    } while( k<plane_count );
  }
  src1 = plane_desc[2].src_plane;
  src2 = plane_desc[3].src_plane;
  src3 = plane_desc[4].src_plane;
  fl1 = plane_desc[src1+1].flags;
  fl2 = plane_desc[src2+1].flags;
  fl3 = plane_desc[src3+1].flags;
  dc2 = plane_desc[src2+1].dc;
  dc1 = plane_desc[src1+1].dc;
  xf1 = fl1&8;
  dc3 = plane_desc[src3+1].dc;
  xf2 = fl2&8;
  xf3 = fl3&8;
  __rc_begin_encode();
  if( height>0 ) {
    y = 0;
    w = width;
    n_planes = plane_count;
    do {
      if( n_planes>0 ) {
        p = 0;
        do {
          ++p;
          blk_k = (AltP1Block*)plane[p-1];
          {
            P1Ctx*const here = (P1Ctx*)blk_k->cursor[0];
            here[0] = here[-1];
            here[1] = here[-2];
            here[2] = here[-3];
            here[3] = here[-4];
            here[4] = here[-5];
            here[5] = here[-6];
          }
          b4 = blk_k->buf[4];
          buf3 = blk_k->buf[3];
          buf2 = blk_k->buf[2];
          buf1 = blk_k->buf[1];
          b0 = blk_k->buf[0];
          blk_k->buf[4] = buf3;
          blk_k->buf[3] = buf2;
          blk_k->buf[2] = buf1;
          blk_k->buf[1] = b0;
          blk_k->buf[0] = b4;
          b4 += 4;
          blk_k->cursor[0] = b4;
          b0 += 4;
          blk_k->cursor[1] = b0;
          blk_k->cursor[2] = buf1+4;
          blk_k->cursor[3] = buf2+4;
          blk_k->cursor[4] = buf3+4;
          ((P1Ctx*)b4)[-4] = ((P1Ctx*)b0)[3];
          {
            P1Ctx*const here = (P1Ctx*)blk_k->cursor[0];
            P1Ctx*const up = (P1Ctx*)blk_k->cursor[1];
            here[-3] = up[2];
            here[-2] = up[1];
            here[-1] = up[0];
          }
          cursor2 = blk_k->cursor[2];
          cursor4 = blk_k->cursor[4];
          cursor0 = blk_k->cursor[0];
          blk_k->ctx[2] = 0;
          blk_k->ctx[3] = (int8_t)cursor2[-2].mag+(int8_t)cursor4[-2].mag+cursor2[0].mag+cursor4[0].mag+cursor2[2].mag+cursor4[2].mag+cursor2[4].mag+cursor4[4].mag+cursor0[-4].mag+cursor0[-2].mag;
          blk_k->ctx[4] = (int8_t)cursor2[-1].mag+(int8_t)cursor4[-1].mag+cursor2[1].mag+cursor4[1].mag+cursor2[3].mag+cursor4[3].mag+cursor2[5].mag+cursor4[5].mag+cursor0[-3].mag+cursor0[-1].mag;
          n_planes = plane_count;
        } while( p<plane_count );
        w = width;
      }
      if( w>0 ) {
        x = 0;
        do {
          blk0 = (AltP1Block*)plane[0];
          want0 = *(src+plane_desc[1].src_plane);
          off0 = plane_desc[1].src_plane;
          cur0 = want0;
          ((AltP1Block*)plane[0])->ctx_of((AltP1Block*)nullptr, (AltP1Block*)0);
          keep0 = cur0;
          pred0 = (uint8_t)blk0->pred;
          resid0 = (uint8_t)(cur0-pred0);
          code0 = blk0->fold[resid0];
          recon0 = (uint8_t)(blk0->unfold[code0]+pred0);
          out0 = (uint8_t)(recon0+*(src+off0)-cur0);
          drift0 = *(src+off0)-out0;
          if( drift0<-16||drift0>16 ) {
            code0 = blk0->fold_hi[resid0];
          } else {
            cur0 = recon0;
            *(src+off0) = out0;
            keep0 = recon0;
          }
          __alt_p1_encode_symbol(&blk0->counters[blk0->ctx[0]].total, recon0, blk0->ctx[1], code0);
          err0 = keep0-blk0->pred;
          blk0->cursor[0]->sym = cur0;
          blk0->cursor[0]->mag = abs32(err0);
          blk0->ctx[blk0->ctx[2]+3] = blk0->ctx[blk0->ctx[2]+3]+blk0->cursor[0]->mag-blk0->cursor[0][-4].mag-(blk0->cursor[4][-2].mag-blk0->cursor[4][6].mag+blk0->cursor[2][-2].mag-blk0->cursor[2][6].mag);
          blk0->ctx[2] = blk0->ctx[2]==0;
          if( blk0->counters[blk0->ctx[0]].total<0x4000u )
            blk0->update_model();
          ++blk0->cursor[0];
          ++blk0->cursor[1];
          ++blk0->cursor[2];
          ++blk0->cursor[3];
          ++blk0->cursor[4];
          want1 = *(src+plane_desc[2].src_plane);
          off1 = plane_desc[2].src_plane;
          if( xf1 )
            want1 = want1-dc1-*(plane_desc[1].src_plane+src);
          blk1 = (AltP1Block*)(plane1);
          cur1 = want1;
          ((AltP1Block*)plane1)->ctx_of((AltP1Block*)plane[0], (AltP1Block*)0);
          pred1 = (uint8_t)blk1->pred;
          resid1 = (uint8_t)(cur1-pred1);
          code1 = blk1->fold[resid1];
          at1 = *(src+off1);
          recon1 = (uint8_t)(blk1->unfold[code1]+pred1);
          out1 = (uint8_t)(recon1+*(src+off1)-cur1);
          drift1 = at1-out1;
          code1x = code1;
          if( drift1<-16||drift1>16 ) {
            code1x = blk1->fold_hi[resid1];
          } else {
            *(src+off1) = out1;
            cur1 = recon1;
          }
          __alt_p1_encode_symbol((uint16_t*)&blk1->counters[blk1->ctx[0]], 16*blk1->ctx[0], (int32_t)blk1->ctx[1], code1x);
          err1 = cur1-blk1->pred;
          blk1->cursor[0]->sym = cur1;
          blk1->cursor[0]->mag = (BYTE4(err1)^err1)-BYTE4(err1);
          blk1->ctx[blk1->ctx[2]+3] += (blk1->cursor[0]->mag-blk1->cursor[0][-4].mag-(blk1->cursor[4][-2].mag-blk1->cursor[4][6].mag)-(blk1->cursor[2][-2].mag-blk1->cursor[2][6].mag));
          blk1->ctx[2] = blk1->ctx[2]==0;
          if( blk1->counters[blk1->ctx[0]].total<0x4000u )
            ((AltP1Block*)blk1)->update_model();
          ++blk1->cursor[0];
          ++blk1->cursor[1];
          ++blk1->cursor[2];
          ++blk1->cursor[3];
          ++blk1->cursor[4];
          want2 = *(plane_desc[3].src_plane+src);
          off2 = plane_desc[3].src_plane;
          if( xf2 )
            want2 = (uint8_t)(want2-dc2-((plane_desc[plane_desc[3].src_plane+1].weight0**(plane_desc[1].src_plane+src)+plane_desc[plane_desc[3].src_plane+1].weight1*(uint32_t)*(plane_desc[2].src_plane+src)+40)>>7));
          blk2 = (AltP1Block*)(plane2);
          ((AltP1Block*)plane2)->ctx_of((AltP1Block*)plane1, (AltP1Block*)plane[0]);
          pred2 = (uint8_t)blk2->pred;
          resid2 = (uint8_t)(want2-pred2);
          code2 = blk2->fold[resid2];
          recon2 = (uint8_t)(blk2->unfold[code2]+pred2);
          out2 = (uint8_t)(recon2+*(off2+src)-want2);
          drift2 = *(off2+src)-out2;
          if( drift2<-16||drift2>16 ) {
            code2 = blk2->fold_hi[resid2];
          } else {
            want2 = recon2;
            *(off2+src) = out2;
          }
          __alt_p1_encode_symbol((uint16_t*)&blk2->counters[blk2->ctx[0]], code2, (int32_t)blk2->ctx[1], code2);
          err2 = want2-blk2->pred;
          blk2->cursor[0]->sym = want2;
          blk2->cursor[0]->mag = (BYTE4(err2)^err2)-BYTE4(err2);
          blk2->ctx[blk2->ctx[2]+3] += (blk2->cursor[0]->mag-blk2->cursor[0][-4].mag-(blk2->cursor[4][-2].mag-blk2->cursor[4][6].mag)-(blk2->cursor[2][-2].mag-blk2->cursor[2][6].mag));
          at2 = blk2->ctx[0];
          blk2->ctx[2] = blk2->ctx[2]==0;
          if( blk2->counters[at2].total<0x4000u )
            ((AltP1Block*)blk2)->update_model();
          ++blk2->cursor[0];
          ++blk2->cursor[1];
          ++blk2->cursor[2];
          ++blk2->cursor[3];
          ++blk2->cursor[4];
          n_planes = plane_count;
          if( plane_count>=4 ) {
            off3 = plane_desc[4].src_plane;
            if( xf3 )
              want3 = *(plane_desc[4].src_plane+src)-dc3-((plane_desc[plane_desc[4].src_plane+1].weight1**(plane_desc[4].src_plane+src-2)+plane_desc[plane_desc[4].src_plane+1].weight0**(plane_desc[4].src_plane+src-3)+plane_desc[plane_desc[4].src_plane+1].weight2**(plane_desc[4].src_plane+src-1)+64)>>7);
            else
              want3 = *(plane_desc[4].src_plane+src);
            blk3 = (AltP1Block*)(plane3);
            cur3 = want3;
            ((AltP1Block*)plane3)->ctx_of((AltP1Block*)plane2, (AltP1Block*)plane1);
            pred3 = (uint8_t)blk3->pred;
            code3 = blk3->fold[(uint8_t)(cur3-pred3)];
            resid3 = (uint8_t)(cur3-pred3);
            recon3 = (uint8_t)(blk3->unfold[code3]+pred3);
            drift3 = *(off3+src)-(uint8_t)(recon3+*(off3+src)-cur3);
            out3 = recon3+*(off3+src)-cur3;
            if( drift3<-16||drift3>16 ) {
              code3 = blk3->fold_hi[resid3];
            } else {
              cur3 = recon3;
              *(off3+src) = out3;
            }
            __alt_p1_encode_symbol((uint16_t*)&blk3->counters[blk3->ctx[0]], code3, (int32_t)blk3->ctx[1], code3);
            err3 = cur3-blk3->pred;
            blk3->cursor[0]->sym = cur3;
            blk3->cursor[0]->mag = (BYTE4(err3)^err3)-BYTE4(err3);
            blk3->ctx[blk3->ctx[2]+3] += (blk3->cursor[0]->mag-blk3->cursor[0][-4].mag-(blk3->cursor[4][-2].mag-blk3->cursor[4][6].mag)-(blk3->cursor[2][-2].mag-blk3->cursor[2][6].mag));
            blk3->ctx[2] = blk3->ctx[2]==0;
            if( blk3->counters[blk3->ctx[0]].total<0x4000u )
              ((AltP1Block*)blk3)->update_model();
            ++blk3->cursor[0];
            ++blk3->cursor[1];
            ++blk3->cursor[2];
            ++blk3->cursor[3];
            ++blk3->cursor[4];
            n_planes = plane_count;
          }
          src += n_planes;
          ++x;
        } while( x<(uint32_t)width );
        w = width;
      }
      ++y;
    } while( y<(uint32_t)height );
  }
  __rc_end_encode();
  np = plane_count;
  if( plane_count>0 ) {
    f = 0;
    do {
      q = (void**)plane[f];
      if( q ) {
        ((AltP1Block*)q)->alt_p1_free(1);
        np = plane_count;
      }
      ++f;
    } while( f<np );
  }
  return np;
}

uint32_t __alt_p2_model(AltP2Block* blk, int32_t sample_in, uint8_t a4, int32_t resid_in) {
  P2Ctx* unused_c, * cursor0;
  float n2_bias;
  int32_t n2_half;
  uint16_t* mir_top;
  P2Freq* grp;
  uint32_t rec_idx;
  int32_t hi_nibble;
  P2Freq* p2_rec;
  uint32_t pair_ctx;
  uint32_t step_s, bank_off2, ctxw_s;
  int32_t sample16;
  uint32_t bank;
  int32_t countdown;
  P2Count* node0;
  int32_t fold_sel2, is_dec;
  P2Count* m0080, * r0400, * r0800, * m0800, * d1000, * r1000, * m1000, * d2000, * r2000, * m2000, * d4000, * r4000, * m4000;
  int32_t lowbits, nres1, nres2, nres3, res_c;
  P2Count* d0800, * d0400, * m0400, * d0200, * m0200, * d0100, * m0100, * r0100, * d0080, * r0080, * d0040, * m0040, * d0020, * m0020, * r0020, * d0010, * m0010;
  int32_t nres4, nres5;
  uint32_t idx0;
  uint16_t* mir_top2;
  float sample, d_bias2;
  uint8_t b_top3, b0n;
  P2Freq* frecg2, * frecg1, * frecg0, * frecg3, * gtop, * frec;
  P2Count* r0200, * r0040, * r0010;
  uint32_t off0, off1, off2, off3, off4, ctxw;
  float (*nb_cur)[4];
  float (*wrow_b)[4];
  bool go, eq_hi, lt_hi, ovf;
  P2Count* bankp;
  float ms1, bias2, bias1, d_bias, ms_a, ms_b, ms_b10, conf;
  int32_t ctx_lo, bank_ctx, res, w0, w0b, res2, w1, e1, wnode0m1a, enode0m1a, x0020, x0010, neg, w_top, e_top, bump, res_s, w_top3, w_topm1, wd4000a, ed4000a, wr4000b, er4000b, wd4000m1b, ed4000m1b, wm4000c, em4000c, wd2000c, ed2000c, wr2000c, er2000c, wd2000m1b, ed2000m1b, wm2000c, em2000c, wd1000c, ed1000c, wr1000c, er1000c, wd1000m1b, ed1000m1b, wm1000c, em1000c, wd0800c, ed0800c, wr0800c, er0800c, wd0800m1b, ed0800m1b, wm0800c, em0800c, wd0400c, ed0400c, wr0400c, er0400c, wd0400m1b, ed0400m1b, wm0400x, wd0200c, ed0200c, wr0200c, er0200c, wd0200m1b, ed0200m1b, neg_b, wm0200c, em0200c, wd0100c, ed0100c, wr0100c, er0100c, wd0100m1b, ed0100m1b, wm0100c, em0100c, wd0080c, ed0080c, wr0080c, er0080c, wd0080m1b, ed0080m1b, wm0080c, em0080c, wd0040c, ed0040c, wr0040c, er0040c, wd0040m1b, ed0040m1b, wm0040c, em0040c, wd0020c, res_t, ed0020c, wr0020c, er0020c, wd0020m1b, ed0020m1b, wm0020c, em0020c, wd0010c, ed0010c, wr0010c, er0010c, wd0010m1b, ed0010m1b, wm0010b, em0010b, wd4000m1a,
          ed4000m1a, neg_c, wm4000b, em4000b, wd2000b, ed2000b, wr2000b, er2000b, wd2000m1a, ed2000m1a, wm2000b, em2000b, wd1000b, ed1000b, wr1000b, er1000b, wd1000m1a, ed1000m1a, wm1000b, em1000b, wd0800b, ed0800b, wr0800b, er0800b, wd0800m1a, ed0800m1a, wm0800b, em0800b, wd0400b, ed0400b, wr0400b, er0400b, wd0400m1a, ed0400m1a, wm0400y, nb_slot, wd0200b, ed0200b, wr0200b, er0200b, wd0200m1a, ed0200m1a, wm0200b, em0200b, wd0100b, ed0100b, wr0100b, er0100b, wd0100m1a, ed0100m1a, wm0100b, em0100b, wd0080b, ed0080b, wr0080b, er0080b, wd0080m1a, ed0080m1a, wm0080b, em0080b, wd0040b, ed0040b, wr0040b, er0040b, wd0040m1a, ed0040m1a, neg_d, wm0040b, em0040b, wd0020b, ed0020b, wr0020b, er0020b, wd0020m1a, ed0020m1a, wm0020b, em0020b, wd0010b, ed0010b, wr0010b, er0010b, wd0010m1a, ed0010m1a, wd4000b, ed4000b, wr4000a, er4000a, wd4000p1a, pd4000p1a, wm4000a, em4000a, wd2000a, ed2000a, wr2000a, er2000a, wm2000a, em2000a, wd1000a, ed1000a, wr1000a, er1000a, wm1000a, em1000a, wd0800a, ed0800a,
          wr0800a, er0800a, wm0800a, em0800a, wd0400a, ed0400a, wr0400a, er0400a, wm0400z, wd0200a, ed0200a, wr0200a, er0200a, wm0200a, em0200a, wd0100a, ed0100a, wr0100a, er0100a, wm0100a, em0100a, wd0080a, ed0080a, wr0080a, er0080a, wm0080a, em0080a, wd0040a, ed0040a, wr0040a, er0040a, wm0040a, em0040a, wd0020a, ed0020a, wr0020a, er0020a, wm0020a, pm0020a, wd0010a, ed0010a, wr0010a, er0010a, wm0010a, em0010a, ctx, ctx15, fold_sel, sel_alt, sel0, sel1, sel2, sel3, sel4;
  int64_t dl, du, dul, dur, resid;
  P2Freq* frec_step, * frec4c;
  P2Freq* frec2, * frec3, * frec4b, * frec5, * frec6, * frec7, * prec0, * g0, * g1, * g2, * g3, * g4, * h4, * nxt4, * prec_m1, * h3, * nxt3, * h2, * nxt2, * h1, * nxt1, * h0, * nxt0;
  uint32_t bank_off, ri0100, ri0010, w_new, step_v, step10, step13;
  ctx_lo = blk->ctx&0xF;
  sample16 = 16*sample_in;
  blk->cursor[0]->val = 16*sample_in;
  blk->cursor[0]->dval = blk->cursor[0]->val-blk->cursor[0]->dval;
  blk->cursor[0][1].dval = 0;
  blk->cursor[0]->sign = (resid_in<=(int32_t)(((uint32_t)(6-ctx_lo)>>31)+((uint32_t)(4-ctx_lo)>>31)+2*((uint32_t)(9-ctx_lo)>>31)))+(resid_in<(int32_t)-(((uint32_t)(6-ctx_lo)>>31)+((uint32_t)(4-ctx_lo)>>31)+2*((uint32_t)(9-ctx_lo)>>31)));
  blk->cursor[0]->mag = abs32(resid_in);
  cursor0 = blk->cursor[0];
  sample = (float)sample16;
  dl = sample16-cursor0[-1].val;
  cursor0->dleft = (WORD2(dl)^dl)-WORD2(dl);
  du = sample16-blk->cursor[1]->val;
  blk->cursor[0]->dup = (WORD2(du)^du)-WORD2(du);
  dul = sample16-blk->cursor[1][-1].val;
  blk->cursor[0]->dupleft = (WORD2(dul)^dul)-WORD2(dul);
  dur = sample16-blk->cursor[1][1].val;
  blk->cursor[0]->dupright = (WORD2(dur)^dur)-WORD2(dur);
  resid = (int16_t)(sample16-blk->pred_prev);
  blk->cursor[0]->err = resid;
  blk->cursor[0]->aerr = (WORD2(resid)^resid)-WORD2(resid);
  nb_cur = blk->nb_cur;
  ms1 = nb_cur[14][1]+0.000099999997f;
  wrow_b = blk->cur[-1];
  bias2 = blk->bias[2];
  n2_bias = blk->bias[0];
  d_bias2 = sample-bias2;
  bias1 = blk->bias[1];
  d_bias = bias1-bias2;
  ms_a = ((((sample-bias2)*(bias1-bias2))-nb_cur[14][0])*0.001f)+nb_cur[14][0];
  ms_b = ms1+(((d_bias*d_bias)-nb_cur[14][1])*0.001f);
  nb_cur[14][1] = ms_b;
  ms_b10 = 0.1f*ms_b;
  if( (0.1f*ms_b)<=ms_a )
    ms_b10 = fminf(ms_b, ms_a);
  nb_cur[14][0] = ms_b10;
  conf = (1.0f-(ms_b10/(ms_b+576.0f)))*2.0f;
  bank = 0;
  {
    const float err_a = (sample-bias1)*2.5999999f;
    const float err_b = d_bias2*conf;
    const float floor_a = 26896.0f*nb_cur[14][2];
    const float floor_b = 5041.0f*wrow_b[14][2];
    const float ms_rate_b = 0.013f*conf;
    int32_t j, k;
    for( j = 0; j<7; ++j )
      for( k = 0; k<4; ++k ) {
        float x = blk->p2_row[j][k];
        float ms;
        ms = nb_cur[7+j][k]+(x*x-nb_cur[7+j][k])*0.05f;
        nb_cur[7+j][k] = ms;
        nb_cur[j][k] += bmf_p2_rate[j][k]*err_a*x/(ms+floor_a);
        ms = wrow_b[7+j][k]+(x*x-wrow_b[7+j][k])*ms_rate_b;
        wrow_b[7+j][k] = ms;
        wrow_b[j][k] += bmf_p2_rate[j][k]*err_b*x/(ms+floor_b);
      }
  }
  {
    float acc[4], pred, err, ms_scale;
    int32_t j, k;
    for( k = 0; k<4; ++k ) {
      acc[k] = nb_cur[0][k]*blk->p2_row[0][k];
      for( j = 1; j<7; ++j )
        acc[k] += nb_cur[j][k]*blk->p2_row[j][k];
    }
    pred = n2_bias+bmf_hsum4(acc);
    err = sample-pred;
    ms_scale = nb_cur[14][2];
    for( j = 0; j<7; ++j )
      for( k = 0; k<4; ++k )
        nb_cur[j][k] += bmf_p2_rate[j][k]*err*blk->p2_row[j][k]/(nb_cur[7+j][k]+ms_scale*529.0f);
    ++*(int32_t*)&nb_cur[15][0];
    nb_cur[14][2] = ms_scale+((10.0f-ms_scale)*0.00019999999f);
  }
  *blk->cur = blk->nb_cur;
  ++blk->cur;
  ++blk->above;
  do {
    bank_ctx = (uint32_t)blk->bank_ctx[bank];
    res = sample16-(blk->nb_sum[2*bank]);
    bank_off = bank<<17;
    node0 = &blk->p2_ctr[32768*bank+bank_ctx];
    w0 = res+(uint16_t)node0->weighted;
    node0->weighted = w0;
    countdown = node0->b1;
    if( countdown ) {
      ctxw_s = bank_ctx;
      w0b = w0+4*((res>deadzone_hi)-(res<deadzone_lo));
      node0->weighted = w0b;
      ctxw = ctxw_s;
      if( (int32_t)abs32(res)<38 ) {
        if( (uint8_t)-- countdown ) {
          node0->b1 = countdown;
        } else {
          if( *(uint8_t*)&node0->rate<8u ) {
            b0n = node0->rate+1;
            node0->rate = b0n;
            node0->b1 = *((uint8_t*)&p2_float_pool+b0n+3);
            node0->weighted = 2*w0b;
          } else {
            node0->b1 = countdown;
          }
        }
      }
      if( !alphabet_reduced ) {
        __builtin_prefetch(&mir_top, 0, 1);
        mir_top = (uint16_t*)((uint8_t*)&blk->p2_ctr[(ctxw^0x7FF0)]+bank_off);
        res2 = (blk->nb_sum[2*bank+1])+res;
        lowbits = ctxw&3;
        if( (uint32_t)lowbits>=3||(bank_off2 = bank_off, ctxw_s = ctxw, w1 = node0[1].weighted, e1 = res2-p2_pred(w1, *(uint8_t*)&node0[1].rate), go = lowbits<=0, *(uint16_t*)&node0[1].weighted = w1+((32*((e1>deadzone_hi)-(uint32_t)(e1<deadzone_lo))+e1+1)>>1), ctxw = ctxw_s, !go) ) {
          bank_off2 = bank_off;
          ctxw_s = ctxw;
          wnode0m1a = node0[-1].weighted;
          enode0m1a = res2-p2_pred(wnode0m1a, *(uint8_t*)&node0[-1].rate);
          *(uint16_t*)&node0[-1].weighted = wnode0m1a+((32*((enode0m1a>deadzone_hi)-(uint32_t)(enode0m1a<deadzone_lo))+enode0m1a+2)>>2);
          ctxw = ctxw_s;
        }
        bankp = &blk->p2_ctr[32768*bank];
        res_c = res2;
        __builtin_prefetch(&bankp[(ctxw^0x4000)], 0, 1);
        d4000 = &bankp[(ctxw^0x4000)];
        m4000 = &bankp[(ctxw^0x3FF0)];
        __builtin_prefetch(m4000, 0, 1);
        r4000 = &bankp[p2_ctx_rotate[((ctxw^0x4000)>>2)&3]+((ctxw^0x4000)&0xFFFFFFF3)];
        __builtin_prefetch(r4000, 0, 1);
        __builtin_prefetch(&bankp[(ctxw^0x2000)], 0, 1);
        d2000 = &bankp[(ctxw^0x2000)];
        __builtin_prefetch(&bankp[(ctxw^0x5FF0)], 0, 1);
        m2000 = &bankp[(ctxw^0x5FF0)];
        r2000 = &bankp[p2_ctx_rotate[((ctxw^0x2000)>>2)&3]+((ctxw^0x2000)&0xFFFFFFF3)];
        __builtin_prefetch(r2000, 0, 1);
        __builtin_prefetch(&bankp[(ctxw^0x1000)], 0, 1);
        d1000 = &bankp[(ctxw^0x1000)];
        __builtin_prefetch(&bankp[(ctxw^0x6FF0)], 0, 1);
        m1000 = &bankp[(ctxw^0x6FF0)];
        r1000 = &bankp[p2_ctx_rotate[((ctxw^0x1000)>>2)&3]+((ctxw^0x1000)&0xFFFFFFF3)];
        __builtin_prefetch(r1000, 0, 1);
        d0800 = &bankp[(ctxw^0x800)];
        m0800 = &bankp[(ctxw^0x77F0)];
        r0800 = &bankp[p2_ctx_rotate[((ctxw^0x800)>>2)&3]+((ctxw^0x800)&0xFFFFFFF3)];
        d0400 = &bankp[(ctxw^0x400)];
        __builtin_prefetch(d0800, 0, 1);
        __builtin_prefetch(m0800, 0, 1);
        __builtin_prefetch(r0800, 0, 1);
        m0400 = &bankp[(ctxw^0x7BF0)];
        r0400 = &bankp[p2_ctx_rotate[((ctxw^0x400)>>2)&3]+((ctxw^0x400)&0xFFFFFFF3)];
        d0200 = &bankp[(ctxw^0x200)];
        __builtin_prefetch(d0400, 0, 1);
        m0200 = &bankp[(ctxw^0x7DF0)];
        r0200 = &bankp[p2_ctx_rotate[((ctxw^0x200)>>2)&3]+((ctxw^0x200)&0xFFFFFFF3)];
        __builtin_prefetch(m0400, 0, 1);
        __builtin_prefetch(r0400, 0, 1);
        d0100 = &bankp[(ctxw^0x100)];
        ri0100 = p2_ctx_rotate[((ctxw^0x100)>>2)&3]+((ctxw^0x100)&0xFFFFFFF3);
        m0100 = &bankp[(ctxw^0x7EF0)];
        __builtin_prefetch(d0200, 0, 1);
        r0100 = &bankp[ri0100];
        d0080 = &bankp[(ctxw^0x80)];
        __builtin_prefetch(m0200, 0, 1);
        __builtin_prefetch(r0200, 0, 1);
        m0080 = &bankp[(ctxw^0x7F70)];
        r0080 = &bankp[p2_ctx_rotate[((ctxw^0x80)>>2)&3]+((ctxw^0x80)&0xFFFFFFF3)];
        d0040 = &bankp[(ctxw^0x40)];
        __builtin_prefetch(d0100, 0, 1);
        m0040 = &bankp[(ctxw^0x7FB0)];
        r0040 = &bankp[p2_ctx_rotate[((ctxw^0x40)>>2)&3]+((ctxw^0x40)&0xFFFFFFF3)];
        __builtin_prefetch(m0100, 0, 1);
        __builtin_prefetch(r0100, 0, 1);
        x0020 = ctxw^0x20;
        x0010 = ctxw^0x10;
        d0020 = &bankp[x0020];
        m0020 = &bankp[(x0020^0x7FF0)];
        __builtin_prefetch(d0080, 0, 1);
        __builtin_prefetch(m0080, 0, 1);
        r0020 = &bankp[p2_ctx_rotate[(x0020>>2)&3]+(x0020&0xFFFFFFF3)];
        d0010 = &bankp[x0010];
        ri0010 = p2_ctx_rotate[(x0010>>2)&3]+(x0010&0xFFFFFFF3);
        m0010 = &bankp[(x0010^0x7FF0)];
        __builtin_prefetch(r0080, 0, 1);
        r0010 = &bankp[ri0010];
        mir_top2 = mir_top;
        neg = -res_c;
        __builtin_prefetch(d0040, 0, 1);
        w_top = (int16_t)mir_top2[1];
        __builtin_prefetch(m0040, 0, 1);
        e_top = neg-p2_pred(w_top, (uint8_t)mir_top[0]);
        ovf = __OFSUB__(e_top, deadzone_hi);
        eq_hi = e_top==deadzone_hi;
        lt_hi = e_top-deadzone_hi<0;
        __builtin_prefetch(r0040, 0, 1);
        bump = 32*(!(lt_hi^ovf|eq_hi)-(e_top<deadzone_lo));
        __builtin_prefetch(d0020, 0, 1);
        __builtin_prefetch(m0020, 0, 1);
        res_s = res_c;
        w_new = bump+e_top+2;
        LOWORD(w_new) = w_top+(w_new>>2);
        go = lowbits<3;
        mir_top[1] = w_new;
        __builtin_prefetch(((P2Count*)(r0020)), 0, 1);
        __builtin_prefetch(d0010, 0, 1);
        __builtin_prefetch(m0010, 0, 1);
        __builtin_prefetch(r0010, 0, 1);
        if( go&&(w_top3 = (int16_t)mir_top[3], b_top3 = (uint8_t)mir_top[2], res_c = res_s, go = lowbits<=0, mir_top[3] = ((uint32_t)(-res_s-p2_pred(w_top3, b_top3)+2)>>2)+w_top3, go) ) {
          wd4000b = d4000->weighted;
          res_c = res_s;
          ed4000b = res_s-p2_pred(wd4000b, (d4000->rate));
          d4000->weighted = p2_bump(wd4000b, ed4000b, 2);
          wr4000a = r4000->weighted;
          er4000a = res_s-p2_pred(wr4000a, r4000->rate);
          r4000->weighted = p2_bump(wr4000a, er4000a, 3);
          wd4000p1a = d4000[1].weighted;
          pd4000p1a = p2_pred(wd4000p1a, *(uint8_t*)&d4000[1].rate);
          nres3 = -res_s;
          *(uint16_t*)&d4000[1].weighted = ((uint32_t)(res_s-pd4000p1a+2)>>2)+wd4000p1a;
          wm4000a = m4000->weighted;
          em4000a = -res_s-p2_pred(wm4000a, m4000->rate);
          m4000->weighted = p2_bump(wm4000a, em4000a, 3);
          wd2000a = d2000->weighted;
          ed2000a = res_c-p2_pred(wd2000a, d2000->rate);
          d2000->weighted = p2_bump(wd2000a, ed2000a, 2);
          wr2000a = r2000->weighted;
          er2000a = res_c-p2_pred(wr2000a, r2000->rate);
          r2000->weighted = p2_bump(wr2000a, er2000a, 3);
          *(uint16_t*)&d2000[1].weighted += (uint32_t)(res_c-p2_pred(*(int16_t*)&*(uint16_t*)&d2000[1].weighted, *(uint8_t*)&d2000[1].rate)+2)>>2;
          wm2000a = m2000->weighted;
          em2000a = nres3-p2_pred(wm2000a, m2000->rate);
          m2000->weighted = p2_bump(wm2000a, em2000a, 3);
          wd1000a = d1000->weighted;
          ed1000a = res_c-p2_pred(wd1000a, d1000->rate);
          d1000->weighted = p2_bump(wd1000a, ed1000a, 2);
          wr1000a = r1000->weighted;
          er1000a = res_c-p2_pred(wr1000a, r1000->rate);
          r1000->weighted = p2_bump(wr1000a, er1000a, 3);
          *(uint16_t*)&d1000[1].weighted += (uint32_t)(res_c-p2_pred(*(int16_t*)&*(uint16_t*)&d1000[1].weighted, *(uint8_t*)&d1000[1].rate)+2)>>2;
          wm1000a = m1000->weighted;
          em1000a = nres3-p2_pred(wm1000a, m1000->rate);
          m1000->weighted = p2_bump(wm1000a, em1000a, 3);
          wd0800a = d0800->weighted;
          ed0800a = res_c-p2_pred(wd0800a, d0800->rate);
          d0800->weighted = p2_bump(wd0800a, ed0800a, 2);
          wr0800a = r0800->weighted;
          er0800a = res_c-p2_pred(wr0800a, r0800->rate);
          r0800->weighted = p2_bump(wr0800a, er0800a, 3);
          *(uint16_t*)&d0800[1].weighted += (uint32_t)(res_c-p2_pred(d0800[1].weighted, d0800[1].rate)+2)>>2;
          wm0800a = m0800->weighted;
          em0800a = nres3-p2_pred(wm0800a, m0800->rate);
          m0800->weighted = p2_bump(wm0800a, em0800a, 3);
          wd0400a = d0400->weighted;
          ed0400a = res_c-p2_pred(wd0400a, d0400->rate);
          d0400->weighted = p2_bump(wd0400a, ed0400a, 2);
          wr0400a = r0400->weighted;
          er0400a = res_c-p2_pred(wr0400a, r0400->rate);
          r0400->weighted = p2_bump(wr0400a, er0400a, 3);
          *(uint16_t*)&d0400[1].weighted += (uint32_t)(res_c-p2_pred(d0400[1].weighted, d0400[1].rate)+2)>>2;
          wm0400z = m0400->weighted;
          nres3 -= p2_pred(wm0400z, m0400->rate);
          m0400->weighted = p2_bump(wm0400z, nres3, 3);
          wd0200a = d0200->weighted;
          ed0200a = res_c-p2_pred(wd0200a, d0200->rate);
          d0200->weighted = p2_bump(wd0200a, ed0200a, 2);
          wr0200a = r0200->weighted;
          er0200a = res_c-p2_pred(wr0200a, r0200->rate);
          r0200->weighted = p2_bump(wr0200a, er0200a, 3);
          *(uint16_t*)&d0200[1].weighted += (uint32_t)(res_c-p2_pred(d0200[1].weighted, d0200[1].rate)+2)>>2;
          wm0200a = m0200->weighted;
          nres2 = -res_c;
          em0200a = -res_c-p2_pred(wm0200a, m0200->rate);
          m0200->weighted = p2_bump(wm0200a, em0200a, 3);
          wd0100a = d0100->weighted;
          ed0100a = res_c-p2_pred(wd0100a, d0100->rate);
          d0100->weighted = p2_bump(wd0100a, ed0100a, 2);
          wr0100a = r0100->weighted;
          er0100a = res_c-p2_pred(wr0100a, r0100->rate);
          r0100->weighted = p2_bump(wr0100a, er0100a, 3);
          *(uint16_t*)&d0100[1].weighted += (uint32_t)(res_c-p2_pred(d0100[1].weighted, d0100[1].rate)+2)>>2;
          wm0100a = m0100->weighted;
          em0100a = nres2-p2_pred(wm0100a, m0100->rate);
          m0100->weighted = p2_bump(wm0100a, em0100a, 3);
          wd0080a = d0080->weighted;
          ed0080a = res_c-p2_pred(wd0080a, d0080->rate);
          d0080->weighted = p2_bump(wd0080a, ed0080a, 2);
          wr0080a = r0080->weighted;
          er0080a = res_c-p2_pred(wr0080a, r0080->rate);
          r0080->weighted = p2_bump(wr0080a, er0080a, 3);
          *(uint16_t*)&d0080[1].weighted += (uint32_t)(res_c-p2_pred(d0080[1].weighted, d0080[1].rate)+2)>>2;
          wm0080a = m0080->weighted;
          em0080a = nres2-p2_pred(wm0080a, m0080->rate);
          m0080->weighted = p2_bump(wm0080a, em0080a, 3);
          wd0040a = d0040->weighted;
          ed0040a = res_c-p2_pred(wd0040a, d0040->rate);
          d0040->weighted = p2_bump(wd0040a, ed0040a, 2);
          wr0040a = r0040->weighted;
          er0040a = res_c-p2_pred(wr0040a, r0040->rate);
          r0040->weighted = p2_bump(wr0040a, er0040a, 3);
          *(uint16_t*)&d0040[1].weighted += (uint32_t)(res_c-p2_pred(d0040[1].weighted, d0040[1].rate)+2)>>2;
          wm0040a = m0040->weighted;
          em0040a = nres2-p2_pred(wm0040a, m0040->rate);
          m0040->weighted = p2_bump(wm0040a, em0040a, 3);
          wd0020a = d0020->weighted;
          ed0020a = res_c-p2_pred(wd0020a, d0020->rate);
          d0020->weighted = p2_bump(wd0020a, ed0020a, 2);
          wr0020a = r0020->weighted;
          er0020a = res_c-p2_pred(wr0020a, r0020->rate);
          r0020->weighted = p2_bump(wr0020a, er0020a, 3);
          *(uint16_t*)&d0020[1].weighted += (uint32_t)(res_c-p2_pred(d0020[1].weighted, d0020[1].rate)+2)>>2;
          wm0020a = m0020->weighted;
          pm0020a = p2_pred(wm0020a, m0020->rate);
          m0020->weighted = wm0020a+((32*((nres2-pm0020a>deadzone_hi)-(uint32_t)(nres2-pm0020a<deadzone_lo))+nres2-pm0020a+4)>>3);
          wd0010a = d0010->weighted;
          ed0010a = res_c-p2_pred(wd0010a, d0010->rate);
          d0010->weighted = p2_bump(wd0010a, ed0010a, 2);
          wr0010a = r0010->weighted;
          er0010a = res_c-p2_pred(wr0010a, r0010->rate);
          r0010->weighted = p2_bump(wr0010a, er0010a, 3);
          *(uint16_t*)&d0010[1].weighted += (uint32_t)(res_c-p2_pred(*(int16_t*)&*(uint16_t*)&d0010[1].weighted, *(uint8_t*)&d0010[1].rate)+2)>>2;
          wm0010a = m0010->weighted;
          em0010a = -res_c-p2_pred(wm0010a, m0010->rate);
          m0010->weighted = p2_bump(wm0010a, em0010a, 3);
        } else {
          w_topm1 = (int16_t)mir_top[-1];
          res_c = res_s;
          mir_top[-1] = ((uint32_t)(-res_s-p2_pred(w_topm1, ((uint8_t)mir_top[-2]))+4)>>3)+w_topm1;
          wd4000a = d4000->weighted;
          ed4000a = res_c-p2_pred(wd4000a, d4000->rate);
          d4000->weighted = p2_bump(wd4000a, ed4000a, 2);
          wr4000b = r4000->weighted;
          go = lowbits<3;
          er4000b = res_c-p2_pred(wr4000b, r4000->rate);
          r4000->weighted = p2_bump(wr4000b, er4000b, 3);
          if( go ) {
            *(uint16_t*)&d4000[1].weighted += (uint32_t)(res_c-p2_pred(d4000[1].weighted, *(uint8_t*)&d4000[1].rate)+2)>>2;
            wd4000m1a = d4000[-1].weighted;
            ed4000m1a = res_c-p2_pred(wd4000m1a, *(uint8_t*)&d4000[-1].rate);
            neg_c = -res_c;
            d4000[-1].weighted = p2_bump(wd4000m1a, ed4000m1a, 3);
            wm4000b = m4000->weighted;
            em4000b = neg_c-p2_pred(wm4000b, m4000->rate);
            m4000->weighted = p2_bump(wm4000b, em4000b, 3);
            wd2000b = d2000->weighted;
            ed2000b = res_c-p2_pred(wd2000b, d2000->rate);
            d2000->weighted = p2_bump(wd2000b, ed2000b, 2);
            wr2000b = r2000->weighted;
            er2000b = res_c-p2_pred(wr2000b, r2000->rate);
            r2000->weighted = p2_bump(wr2000b, er2000b, 3);
            *(uint16_t*)&d2000[1].weighted += (uint32_t)(res_c-p2_pred(*(int16_t*)&*(uint16_t*)&d2000[1].weighted, *(uint8_t*)&d2000[1].rate)+2)>>2;
            wd2000m1a = d2000[-1].weighted;
            ed2000m1a = res_c-p2_pred(wd2000m1a, *(uint8_t*)&d2000[-1].rate);
            d2000[-1].weighted = p2_bump(wd2000m1a, ed2000m1a, 3);
            wm2000b = m2000->weighted;
            em2000b = neg_c-p2_pred(wm2000b, m2000->rate);
            m2000->weighted = p2_bump(wm2000b, em2000b, 3);
            wd1000b = d1000->weighted;
            ed1000b = res_c-p2_pred(wd1000b, d1000->rate);
            d1000->weighted = p2_bump(wd1000b, ed1000b, 2);
            wr1000b = r1000->weighted;
            er1000b = res_c-p2_pred(wr1000b, r1000->rate);
            r1000->weighted = p2_bump(wr1000b, er1000b, 3);
            *(uint16_t*)&d1000[1].weighted += (uint32_t)(res_c-p2_pred(*(int16_t*)&*(uint16_t*)&d1000[1].weighted, *(uint8_t*)&d1000[1].rate)+2)>>2;
            wd1000m1a = d1000[-1].weighted;
            ed1000m1a = res_c-p2_pred(wd1000m1a, *(uint8_t*)&d1000[-1].rate);
            d1000[-1].weighted = p2_bump(wd1000m1a, ed1000m1a, 3);
            wm1000b = m1000->weighted;
            em1000b = neg_c-p2_pred(wm1000b, m1000->rate);
            m1000->weighted = p2_bump(wm1000b, em1000b, 3);
            wd0800b = d0800->weighted;
            ed0800b = res_c-p2_pred(wd0800b, d0800->rate);
            d0800->weighted = p2_bump(wd0800b, ed0800b, 2);
            wr0800b = r0800->weighted;
            er0800b = res_c-p2_pred(wr0800b, r0800->rate);
            r0800->weighted = p2_bump(wr0800b, er0800b, 3);
            *(uint16_t*)&d0800[1].weighted += (uint32_t)(res_c-p2_pred(d0800[1].weighted, d0800[1].rate)+2)>>2;
            wd0800m1a = d0800[-1].weighted;
            ed0800m1a = res_c-p2_pred(wd0800m1a, d0800[-1].rate);
            d0800[-1].weighted = p2_bump(wd0800m1a, ed0800m1a, 3);
            wm0800b = m0800->weighted;
            em0800b = neg_c-p2_pred(wm0800b, m0800->rate);
            m0800->weighted = p2_bump(wm0800b, em0800b, 3);
            wd0400b = d0400->weighted;
            ed0400b = res_c-p2_pred(wd0400b, d0400->rate);
            d0400->weighted = p2_bump(wd0400b, ed0400b, 2);
            wr0400b = r0400->weighted;
            er0400b = res_c-p2_pred(wr0400b, r0400->rate);
            r0400->weighted = p2_bump(wr0400b, er0400b, 3);
            *(uint16_t*)&d0400[1].weighted += (uint32_t)(res_c-p2_pred(d0400[1].weighted, d0400[1].rate)+2)>>2;
            wd0400m1a = d0400[-1].weighted;
            ed0400m1a = res_c-p2_pred(wd0400m1a, d0400[-1].rate);
            d0400[-1].weighted = p2_bump(wd0400m1a, ed0400m1a, 3);
            wm0400y = m0400->weighted;
            nres5 = -res_c;
            nb_slot = -res_c-p2_pred(wm0400y, m0400->rate);
            m0400->weighted = p2_bump(wm0400y, nb_slot, 3);
            wd0200b = d0200->weighted;
            ed0200b = res_c-p2_pred(wd0200b, d0200->rate);
            d0200->weighted = p2_bump(wd0200b, ed0200b, 2);
            wr0200b = r0200->weighted;
            er0200b = res_c-p2_pred(wr0200b, r0200->rate);
            r0200->weighted = p2_bump(wr0200b, er0200b, 3);
            *(uint16_t*)&d0200[1].weighted += (uint32_t)(res_c-p2_pred(d0200[1].weighted, d0200[1].rate)+2)>>2;
            wd0200m1a = d0200[-1].weighted;
            ed0200m1a = res_c-p2_pred(wd0200m1a, d0200[-1].rate);
            d0200[-1].weighted = p2_bump(wd0200m1a, ed0200m1a, 3);
            wm0200b = m0200->weighted;
            em0200b = nres5-p2_pred(wm0200b, m0200->rate);
            m0200->weighted = p2_bump(wm0200b, em0200b, 3);
            wd0100b = d0100->weighted;
            ed0100b = res_c-p2_pred(wd0100b, d0100->rate);
            d0100->weighted = p2_bump(wd0100b, ed0100b, 2);
            wr0100b = r0100->weighted;
            er0100b = res_c-p2_pred(wr0100b, r0100->rate);
            r0100->weighted = p2_bump(wr0100b, er0100b, 3);
            *(uint16_t*)&d0100[1].weighted += (uint32_t)(res_c-p2_pred(d0100[1].weighted, d0100[1].rate)+2)>>2;
            wd0100m1a = d0100[-1].weighted;
            ed0100m1a = res_c-p2_pred(wd0100m1a, d0100[-1].rate);
            d0100[-1].weighted = p2_bump(wd0100m1a, ed0100m1a, 3);
            wm0100b = m0100->weighted;
            em0100b = nres5-p2_pred(wm0100b, m0100->rate);
            m0100->weighted = p2_bump(wm0100b, em0100b, 3);
            wd0080b = d0080->weighted;
            ed0080b = res_c-p2_pred(wd0080b, d0080->rate);
            d0080->weighted = p2_bump(wd0080b, ed0080b, 2);
            wr0080b = r0080->weighted;
            er0080b = res_c-p2_pred(wr0080b, r0080->rate);
            r0080->weighted = p2_bump(wr0080b, er0080b, 3);
            *(uint16_t*)&d0080[1].weighted += (uint32_t)(res_c-p2_pred(d0080[1].weighted, d0080[1].rate)+2)>>2;
            wd0080m1a = d0080[-1].weighted;
            ed0080m1a = res_c-p2_pred(wd0080m1a, d0080[-1].rate);
            d0080[-1].weighted = p2_bump(wd0080m1a, ed0080m1a, 3);
            wm0080b = m0080->weighted;
            em0080b = nres5-p2_pred(wm0080b, m0080->rate);
            m0080->weighted = p2_bump(wm0080b, em0080b, 3);
            wd0040b = d0040->weighted;
            ed0040b = res_c-p2_pred(wd0040b, d0040->rate);
            d0040->weighted = p2_bump(wd0040b, ed0040b, 2);
            wr0040b = r0040->weighted;
            er0040b = res_c-p2_pred(wr0040b, r0040->rate);
            r0040->weighted = p2_bump(wr0040b, er0040b, 3);
            *(uint16_t*)&d0040[1].weighted += (uint32_t)(res_c-p2_pred(d0040[1].weighted, d0040[1].rate)+2)>>2;
            wd0040m1a = d0040[-1].weighted;
            ed0040m1a = res_c-p2_pred(wd0040m1a, d0040[-1].rate);
            neg_d = -res_c;
            d0040[-1].weighted = p2_bump(wd0040m1a, ed0040m1a, 3);
            wm0040b = m0040->weighted;
            em0040b = neg_d-p2_pred(wm0040b, m0040->rate);
            m0040->weighted = p2_bump(wm0040b, em0040b, 3);
            wd0020b = d0020->weighted;
            ed0020b = res_c-p2_pred(wd0020b, d0020->rate);
            d0020->weighted = p2_bump(wd0020b, ed0020b, 2);
            wr0020b = r0020->weighted;
            er0020b = res_c-p2_pred(wr0020b, r0020->rate);
            r0020->weighted = p2_bump(wr0020b, er0020b, 3);
            *(uint16_t*)&d0020[1].weighted += (uint32_t)(res_c-p2_pred(d0020[1].weighted, d0020[1].rate)+2)>>2;
            wd0020m1a = d0020[-1].weighted;
            ed0020m1a = res_c-p2_pred(wd0020m1a, d0020[-1].rate);
            d0020[-1].weighted = p2_bump(wd0020m1a, ed0020m1a, 3);
            wm0020b = m0020->weighted;
            em0020b = neg_d-p2_pred(wm0020b, m0020->rate);
            m0020->weighted = p2_bump(wm0020b, em0020b, 3);
            wd0010b = d0010->weighted;
            ed0010b = res_c-p2_pred(wd0010b, d0010->rate);
            d0010->weighted = p2_bump(wd0010b, ed0010b, 2);
            wr0010b = r0010->weighted;
            er0010b = res_c-p2_pred(wr0010b, r0010->rate);
            r0010->weighted = p2_bump(wr0010b, er0010b, 3);
            *(uint16_t*)&d0010[1].weighted += (uint32_t)(res_c-p2_pred(*(int16_t*)&*(uint16_t*)&d0010[1].weighted, *(uint8_t*)&d0010[1].rate)+2)>>2;
            wd0010m1a = d0010[-1].weighted;
            ed0010m1a = res_c-p2_pred(wd0010m1a, *(uint8_t*)&d0010[-1].rate);
            d0010[-1].weighted = p2_bump(wd0010m1a, ed0010m1a, 3);
            wm0010b = m0010->weighted;
            em0010b = neg_d-p2_pred(wm0010b, m0010->rate);
          } else {
            wd4000m1b = d4000[-1].weighted;
            ed4000m1b = res_c-p2_pred(wd4000m1b, *(uint8_t*)&d4000[-1].rate);
            d4000[-1].weighted = p2_bump(wd4000m1b, ed4000m1b, 3);
            wm4000c = m4000->weighted;
            nres1 = -res_c;
            em4000c = -res_c-p2_pred(wm4000c, m4000->rate);
            m4000->weighted = p2_bump(wm4000c, em4000c, 3);
            wd2000c = d2000->weighted;
            ed2000c = res_c-p2_pred(wd2000c, d2000->rate);
            d2000->weighted = p2_bump(wd2000c, ed2000c, 2);
            wr2000c = r2000->weighted;
            er2000c = res_c-p2_pred(wr2000c, r2000->rate);
            r2000->weighted = p2_bump(wr2000c, er2000c, 3);
            wd2000m1b = d2000[-1].weighted;
            ed2000m1b = res_c-p2_pred(wd2000m1b, *(uint8_t*)&d2000[-1].rate);
            d2000[-1].weighted = p2_bump(wd2000m1b, ed2000m1b, 3);
            wm2000c = m2000->weighted;
            em2000c = nres1-p2_pred(wm2000c, m2000->rate);
            m2000->weighted = p2_bump(wm2000c, em2000c, 3);
            wd1000c = d1000->weighted;
            ed1000c = res_c-p2_pred(wd1000c, d1000->rate);
            d1000->weighted = p2_bump(wd1000c, ed1000c, 2);
            wr1000c = r1000->weighted;
            er1000c = res_c-p2_pred(wr1000c, r1000->rate);
            r1000->weighted = p2_bump(wr1000c, er1000c, 3);
            wd1000m1b = d1000[-1].weighted;
            ed1000m1b = res_c-p2_pred(wd1000m1b, *(uint8_t*)&d1000[-1].rate);
            d1000[-1].weighted = p2_bump(wd1000m1b, ed1000m1b, 3);
            wm1000c = m1000->weighted;
            em1000c = nres1-p2_pred(wm1000c, m1000->rate);
            m1000->weighted = p2_bump(wm1000c, em1000c, 3);
            wd0800c = d0800->weighted;
            ed0800c = res_c-p2_pred(wd0800c, d0800->rate);
            d0800->weighted = p2_bump(wd0800c, ed0800c, 2);
            wr0800c = r0800->weighted;
            er0800c = res_c-p2_pred(wr0800c, r0800->rate);
            r0800->weighted = p2_bump(wr0800c, er0800c, 3);
            wd0800m1b = d0800[-1].weighted;
            ed0800m1b = res_c-p2_pred(wd0800m1b, d0800[-1].rate);
            d0800[-1].weighted = p2_bump(wd0800m1b, ed0800m1b, 3);
            wm0800c = m0800->weighted;
            em0800c = nres1-p2_pred(wm0800c, m0800->rate);
            m0800->weighted = p2_bump(wm0800c, em0800c, 3);
            wd0400c = d0400->weighted;
            ed0400c = res_c-p2_pred(wd0400c, d0400->rate);
            d0400->weighted = p2_bump(wd0400c, ed0400c, 2);
            wr0400c = r0400->weighted;
            er0400c = res_c-p2_pred(wr0400c, r0400->rate);
            r0400->weighted = p2_bump(wr0400c, er0400c, 3);
            wd0400m1b = d0400[-1].weighted;
            ed0400m1b = res_c-p2_pred(wd0400m1b, d0400[-1].rate);
            d0400[-1].weighted = p2_bump(wd0400m1b, ed0400m1b, 3);
            wm0400x = m0400->weighted;
            nres1 -= p2_pred(wm0400x, m0400->rate);
            m0400->weighted = p2_bump(wm0400x, nres1, 3);
            wd0200c = d0200->weighted;
            ed0200c = res_c-p2_pred(wd0200c, d0200->rate);
            d0200->weighted = p2_bump(wd0200c, ed0200c, 2);
            wr0200c = r0200->weighted;
            er0200c = res_c-p2_pred(wr0200c, r0200->rate);
            r0200->weighted = p2_bump(wr0200c, er0200c, 3);
            wd0200m1b = d0200[-1].weighted;
            ed0200m1b = res_c-p2_pred(wd0200m1b, d0200[-1].rate);
            neg_b = -res_c;
            d0200[-1].weighted = p2_bump(wd0200m1b, ed0200m1b, 3);
            wm0200c = m0200->weighted;
            em0200c = neg_b-p2_pred(wm0200c, m0200->rate);
            m0200->weighted = p2_bump(wm0200c, em0200c, 3);
            wd0100c = d0100->weighted;
            ed0100c = res_c-p2_pred(wd0100c, d0100->rate);
            d0100->weighted = p2_bump(wd0100c, ed0100c, 2);
            wr0100c = r0100->weighted;
            er0100c = res_c-p2_pred(wr0100c, r0100->rate);
            r0100->weighted = p2_bump(wr0100c, er0100c, 3);
            wd0100m1b = d0100[-1].weighted;
            ed0100m1b = res_c-p2_pred(wd0100m1b, d0100[-1].rate);
            d0100[-1].weighted = p2_bump(wd0100m1b, ed0100m1b, 3);
            wm0100c = m0100->weighted;
            em0100c = neg_b-p2_pred(wm0100c, m0100->rate);
            m0100->weighted = p2_bump(wm0100c, em0100c, 3);
            wd0080c = d0080->weighted;
            ed0080c = res_c-p2_pred(wd0080c, d0080->rate);
            d0080->weighted = p2_bump(wd0080c, ed0080c, 2);
            wr0080c = r0080->weighted;
            er0080c = res_c-p2_pred(wr0080c, r0080->rate);
            r0080->weighted = p2_bump(wr0080c, er0080c, 3);
            wd0080m1b = d0080[-1].weighted;
            ed0080m1b = res_c-p2_pred(wd0080m1b, d0080[-1].rate);
            d0080[-1].weighted = p2_bump(wd0080m1b, ed0080m1b, 3);
            wm0080c = m0080->weighted;
            em0080c = neg_b-p2_pred(wm0080c, m0080->rate);
            m0080->weighted = p2_bump(wm0080c, em0080c, 3);
            wd0040c = d0040->weighted;
            ed0040c = res_c-p2_pred(wd0040c, d0040->rate);
            d0040->weighted = p2_bump(wd0040c, ed0040c, 2);
            wr0040c = r0040->weighted;
            er0040c = res_c-p2_pred(wr0040c, r0040->rate);
            r0040->weighted = p2_bump(wr0040c, er0040c, 3);
            wd0040m1b = d0040[-1].weighted;
            ed0040m1b = res_c-p2_pred(wd0040m1b, d0040[-1].rate);
            d0040[-1].weighted = p2_bump(wd0040m1b, ed0040m1b, 3);
            wm0040c = m0040->weighted;
            em0040c = neg_b-p2_pred(wm0040c, m0040->rate);
            m0040->weighted = p2_bump(wm0040c, em0040c, 3);
            wd0020c = d0020->weighted;
            res_t = res_c;
            ed0020c = res_c-p2_pred(wd0020c, d0020->rate);
            d0020->weighted = p2_bump(wd0020c, ed0020c, 2);
            wr0020c = r0020->weighted;
            er0020c = res_t-p2_pred(wr0020c, r0020->rate);
            res_c = res_t;
            r0020->weighted = p2_bump(wr0020c, er0020c, 3);
            wd0020m1b = d0020[-1].weighted;
            ed0020m1b = res_t-p2_pred(wd0020m1b, d0020[-1].rate);
            d0020[-1].weighted = p2_bump(wd0020m1b, ed0020m1b, 3);
            wm0020c = m0020->weighted;
            nres4 = -res_t;
            em0020c = -res_t-p2_pred(wm0020c, m0020->rate);
            m0020->weighted = p2_bump(wm0020c, em0020c, 3);
            wd0010c = d0010->weighted;
            ed0010c = res_c-p2_pred(wd0010c, d0010->rate);
            d0010->weighted = p2_bump(wd0010c, ed0010c, 2);
            wr0010c = r0010->weighted;
            er0010c = res_c-p2_pred(wr0010c, r0010->rate);
            r0010->weighted = p2_bump(wr0010c, er0010c, 3);
            wd0010m1b = d0010[-1].weighted;
            ed0010m1b = res_c-p2_pred(wd0010m1b, *(uint8_t*)&d0010[-1].rate);
            d0010[-1].weighted = p2_bump(wd0010m1b, ed0010m1b, 3);
            wm0010b = m0010->weighted;
            em0010b = nres4-p2_pred(wm0010b, m0010->rate);
          }
          m0010->weighted = p2_bump(wm0010b, em0010b, 3);
        }
      }
    }
    ++bank;
  } while( bank<5 );
  ctx = blk->ctx;
  ++blk->cursor[0];
  frec = (&blk->freq[ctx]);
  ++blk->cursor[1];
  ++blk->cursor[2];
  ++blk->cursor[3];
  ++blk->cursor[4];
  step_v = frec[0].step;
  if( step_v>0x10 ) {
    is_dec = a4&1;
    ctx15 = ctx&0xF;
    pair_ctx = blk->ctx_pair[a4&1];
    if( ctx15<15 ) {
      mir_top = (uint16_t*)&frec[1];
      if( frec[1].f[2]+frec[1].f[1]+frec[1].f[0]>29696 )
        frec[1].rescale_three_way();
      step10 = (10*(uint32_t)frec[1].step)>>4;
      if( a4 ) {
        mir_top[3-is_dec] += step10;
        __update_binary_pair(model_strip(pair_ctx+1), (a4-1)>>1);
      } else {
        mir_top[1] += step10;
      }
      if( ctx15<=0 ) {
LABEL_37:
        step_v = blk->ctx;
        if( blk->freq[step_v].step<=0x1Au )
          return step_v;
        fold_sel = 2-(blk->fold[(uint8_t)-resid_in]&1);
        if( !blk->fold[(uint8_t)-resid_in] )
          fold_sel = blk->fold[(uint8_t)-resid_in];
        fold_sel2 = fold_sel;
        step_v = blk->ctx_w[4].w[2-blk->ctx_w[4].sel]+blk->ctx_w[3].w[2-blk->ctx_w[3].sel]+blk->ctx_w[2].w[2-blk->ctx_w[2].sel]+blk->ctx_w[1].w[2-blk->ctx_w[1].sel]+(blk->ctx_w[0].w[1]+(blk->ctx&0x3F));
        frec_step = &blk->freq[step_v];
        p2_rec = frec_step;
        if( ctx15<15 ) {
          frec2 = &frec_step[1];
          mir_top = (uint16_t*)frec2;
          if( frec2->f[2]+frec_step[1].f[1]+frec_step[1].f[0]>29696 ) {
            step_s = step_v;
            frec2->rescale_three_way();
            step_v = step_s;
          }
          mir_top[fold_sel2+1] += (p2_rec[1].step&0xFFFC)>>2;
          if( ctx15<=0 )
            goto LABEL_48;
        } else {
        }
        mir_top = (uint16_t*)&p2_rec[-1];
        if( p2_rec[-1].f[2]+p2_rec[-1].f[1]+p2_rec[-1].f[0]>29696 ) {
          step_s = step_v;
          p2_rec[-1].rescale_three_way();
          step_v = step_s;
        }
        mir_top[fold_sel2+1] += (uint16_t)(p2_rec[-1].step&0xFFFC)>>2;
LABEL_48:
        if( a4 ) {
          n2_half = (a4-1)>>1;
          hi_nibble = (uint8_t)pair_ctx&0xF0;
          if( hi_nibble>=0xF0||(step_s = step_v, __update_binary_pair(model_strip(pair_ctx+16), n2_half), step_v = step_s, hi_nibble>0) ) {
            step_s = step_v;
            __update_binary_pair(model_strip(pair_ctx-16), n2_half);
            step_v = step_s;
          }
        }
        prec0 = &p2_rec[0];
        if( p2_rec[0].f[2]+p2_rec[0].f[1]+p2_rec[0].f[0]>29696 ) {
          step_s = step_v;
          p2_rec[0].rescale_three_way();
          step_v = step_s;
        }
        prec0->f[fold_sel2] += (6*(uint32_t)p2_rec[0].step)>>4;
        if( !blk->plane_idx||blk->freq[blk->ctx].step>0x100u ) {
          sel_alt = 2-is_dec;
          if( !a4 )
            sel_alt = 0;
          sel0 = blk->ctx_w[0].sel;
          is_dec = sel_alt;
          if( sel0==1 ) {
            h0 = &blk->freq[blk->ctx_w[0].w[0]+step_v-blk->ctx_w[0].w[1]];
            if( h0->f[2]+(blk->freq[blk->ctx_w[0].w[0]+step_v-blk->ctx_w[0].w[1]].f[1])+(blk->freq[blk->ctx_w[0].w[0]+step_v-blk->ctx_w[0].w[1]].f[0])>29696 ) {
              step_s = step_v;
              h0->rescale_three_way();
              step_v = step_s;
            }
            h0->f[fold_sel2] += (uint16_t)(h0->step&0xFFFC)>>2;
            off0 = blk->ctx-blk->ctx_w[0].w[1];
            frecg0 = (&blk->freq[off0+blk->ctx_w[0].w[0]]);
            idx0 = blk->ctx_w[0].w[2]+off0;
            grp = frecg0;
            frec3 = &frecg0[0];
            mir_top = (uint16_t*)frec3;
            if( frec3->f[2]+(frec3->f[1]+frec3->f[0])>29696 ) {
              step_s = step_v;
              frec3->rescale_three_way();
              step_v = step_s;
            }
            mir_top[is_dec+1] += (3*grp[0].step)>>4;
            if( ctx15<15 ) {
              mir_top = (uint16_t*)&grp[1];
              if( grp[1].f[2]+grp[1].f[1]+grp[1].f[0]>29696 ) {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (uint16_t)(grp[1].step&0xFFFC)>>2;
              nxt0 = &blk->freq[idx0+1];
              if( blk->freq[idx0+1].f[2]+blk->freq[idx0+1].f[1]+blk->freq[idx0+1].f[0]>29696 ) {
                step_s = step_v;
                (&blk->freq[idx0+1])->rescale_three_way();
                step_v = step_s;
              }
              nxt0->f[is_dec] += (nxt0->step&0xFFF8)>>3;
            }
            if( ctx15>2 ) {
              mir_top = (uint16_t*)&grp[-1];
              if( grp[-1].f[2]+grp[-1].f[1]+grp[-1].f[0]>29696 ) {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (6*(uint32_t)grp[-1].step)>>4;
            }
          } else {
            g0 = &blk->freq[step_v+blk->ctx_w[0].w[1]-blk->ctx_w[0].w[2-sel0]];
            if( blk->freq[step_v+blk->ctx_w[0].w[1]-blk->ctx_w[0].w[2-sel0]].f[2]+blk->freq[step_v+blk->ctx_w[0].w[1]-blk->ctx_w[0].w[2-sel0]].f[1]+blk->freq[step_v+blk->ctx_w[0].w[1]-blk->ctx_w[0].w[2-sel0]].f[0]>29696 ) {
              step_s = step_v;
              g0->rescale_three_way();
              step_v = step_s;
            }
            g0->f[fold_sel2] += (7*(uint32_t)g0->step)>>4;
            grp = ((&blk->freq[blk->ctx_w[0].w[1]+(blk->ctx-blk->ctx_w[0].w[blk->ctx_w[0].sel])]));
            mir_top = (uint16_t*)&grp[0];
            if( grp[0].f[2]+grp[0].f[1]+grp[0].f[0]>29696 ) {
              step_s = step_v;
              grp[0].rescale_three_way();
              step_v = step_s;
            }
            mir_top[is_dec+1] += (7*grp[0].step)>>4;
            if( ctx15<15 ) {
              mir_top = (uint16_t*)&grp[1];
              if( grp[1].f[2]+grp[1].f[1]+grp[1].f[0]>29696 ) {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              step_s = step_v;
              mir_top[is_dec+1] = mir_top[is_dec+1]+((5*(uint32_t)grp[1].step)>>4);
              step_v = step_s;
            }
            if( ctx15>0 ) {
              mir_top = (uint16_t*)&grp[-1];
              if( grp[-1].f[2]+grp[-1].f[1]+grp[-1].f[0]>29696 ) {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (6*(uint32_t)grp[-1].step)>>4;
            }
          }
          sel1 = blk->ctx_w[1].sel;
          if( sel1==1 ) {
            frec4b = &blk->freq[blk->ctx_w[1].w[0]+step_v-blk->ctx_w[1].w[1]];
            mir_top = (uint16_t*)frec4b;
            if( frec4b->f[2]+(blk->freq[blk->ctx_w[1].w[0]+step_v-blk->ctx_w[1].w[1]].f[1])+(blk->freq[blk->ctx_w[1].w[0]+step_v-blk->ctx_w[1].w[1]].f[0])>29696 ) {
              step_s = step_v;
              frec4b->rescale_three_way();
              step_v = step_s;
            }
            mir_top[fold_sel2+1] += (uint16_t)(mir_top[0]&0xFFFC)>>2;
            off1 = blk->ctx-blk->ctx_w[1].w[1];
            frecg1 = (&blk->freq[off1+blk->ctx_w[1].w[0]]);
            idx0 = blk->ctx_w[1].w[2]+off1;
            grp = frecg1;
            h1 = &frecg1[0];
            if( h1->f[2]+h1->f[1]+h1->f[0]>29696 ) {
              step_s = step_v;
              h1->rescale_three_way();
              step_v = step_s;
            }
            h1->f[is_dec] += (3*(uint32_t)grp[0].step)>>4;
            if( ctx15<15 ) {
              mir_top = (uint16_t*)&grp[1];
              if( grp[1].f[2]+grp[1].f[1]+grp[1].f[0]>29696 ) {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (uint16_t)(grp[1].step&0xFFFC)>>2;
              nxt1 = &blk->freq[idx0+1];
              if( blk->freq[idx0+1].f[2]+blk->freq[idx0+1].f[1]+blk->freq[idx0+1].f[0]>29696 ) {
                step_s = step_v;
                (&blk->freq[idx0+1])->rescale_three_way();
                step_v = step_s;
              }
              nxt1->f[is_dec] += (nxt1->step&0xFFF8)>>3;
            }
            if( ctx15>2 ) {
              mir_top = (uint16_t*)&grp[-1];
              if( grp[-1].f[2]+grp[-1].f[1]+grp[-1].f[0]>29696 ) {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (6*(uint32_t)grp[-1].step)>>4;
            }
          } else {
            g1 = &blk->freq[step_v+blk->ctx_w[1].w[1]-blk->ctx_w[1].w[2-sel1]];
            if( blk->freq[step_v+blk->ctx_w[1].w[1]-blk->ctx_w[1].w[2-sel1]].f[2]+blk->freq[step_v+blk->ctx_w[1].w[1]-blk->ctx_w[1].w[2-sel1]].f[1]+blk->freq[step_v+blk->ctx_w[1].w[1]-blk->ctx_w[1].w[2-sel1]].f[0]>29696 ) {
              step_s = step_v;
              g1->rescale_three_way();
              step_v = step_s;
            }
            g1->f[fold_sel2] += (7*(uint32_t)g1->step)>>4;
            grp = ((&blk->freq[blk->ctx_w[1].w[1]+(blk->ctx-blk->ctx_w[1].w[blk->ctx_w[1].sel])]));
            mir_top = (uint16_t*)&grp[0];
            if( grp[0].f[2]+grp[0].f[1]+grp[0].f[0]>29696 ) {
              step_s = step_v;
              grp[0].rescale_three_way();
              step_v = step_s;
            }
            mir_top[is_dec+1] += (7*(uint32_t)grp[0].step)>>4;
            if( ctx15<15 ) {
              mir_top = (uint16_t*)&grp[1];
              if( grp[1].f[2]+grp[1].f[1]+grp[1].f[0]>29696 ) {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (5*(uint32_t)grp[1].step)>>4;
            }
            if( ctx15>0 ) {
              mir_top = (uint16_t*)&grp[-1];
              if( grp[-1].f[2]+grp[-1].f[1]+grp[-1].f[0]>29696 ) {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (6*(uint32_t)grp[-1].step)>>4;
            }
          }
          sel2 = blk->ctx_w[2].sel;
          if( sel2==1 ) {
            frec5 = &blk->freq[blk->ctx_w[2].w[0]+step_v-blk->ctx_w[2].w[1]];
            mir_top = (uint16_t*)frec5;
            if( frec5->f[2]+(blk->freq[blk->ctx_w[2].w[0]+step_v-blk->ctx_w[2].w[1]].f[1])+(blk->freq[blk->ctx_w[2].w[0]+step_v-blk->ctx_w[2].w[1]].f[0])>29696 ) {
              step_s = step_v;
              frec5->rescale_three_way();
              step_v = step_s;
            }
            mir_top[fold_sel2+1] += (uint16_t)(mir_top[0]&0xFFFC)>>2;
            off2 = blk->ctx-blk->ctx_w[2].w[1];
            frecg2 = (&blk->freq[off2+blk->ctx_w[2].w[0]]);
            idx0 = blk->ctx_w[2].w[2]+off2;
            grp = frecg2;
            h2 = &frecg2[0];
            if( h2->f[2]+h2->f[1]+h2->f[0]>29696 ) {
              step_s = step_v;
              h2->rescale_three_way();
              step_v = step_s;
            }
            h2->f[is_dec] += (3*(uint32_t)grp[0].step)>>4;
            if( ctx15<15 ) {
              mir_top = (uint16_t*)&grp[1];
              if( grp[1].f[2]+grp[1].f[1]+grp[1].f[0]>29696 ) {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (uint16_t)(grp[1].step&0xFFFC)>>2;
              nxt2 = &blk->freq[idx0+1];
              if( blk->freq[idx0+1].f[2]+blk->freq[idx0+1].f[1]+blk->freq[idx0+1].f[0]>29696 ) {
                step_s = step_v;
                (&blk->freq[idx0+1])->rescale_three_way();
                step_v = step_s;
              }
              nxt2->f[is_dec] += (nxt2->step&0xFFF8)>>3;
            }
            if( ctx15>2 ) {
              mir_top = (uint16_t*)&grp[-1];
              if( grp[-1].f[2]+grp[-1].f[1]+grp[-1].f[0]>29696 ) {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (6*(uint32_t)grp[-1].step)>>4;
            }
          } else {
            g2 = &blk->freq[step_v+blk->ctx_w[2].w[1]-blk->ctx_w[2].w[2-sel2]];
            if( blk->freq[step_v+blk->ctx_w[2].w[1]-blk->ctx_w[2].w[2-sel2]].f[2]+blk->freq[step_v+blk->ctx_w[2].w[1]-blk->ctx_w[2].w[2-sel2]].f[1]+blk->freq[step_v+blk->ctx_w[2].w[1]-blk->ctx_w[2].w[2-sel2]].f[0]>29696 ) {
              step_s = step_v;
              g2->rescale_three_way();
              step_v = step_s;
            }
            g2->f[fold_sel2] += (7*(uint32_t)g2->step)>>4;
            grp = ((&blk->freq[blk->ctx_w[2].w[1]+(blk->ctx-blk->ctx_w[2].w[blk->ctx_w[2].sel])]));
            mir_top = (uint16_t*)&grp[0];
            if( grp[0].f[2]+grp[0].f[1]+grp[0].f[0]>29696 ) {
              step_s = step_v;
              grp[0].rescale_three_way();
              step_v = step_s;
            }
            mir_top[is_dec+1] += (7*(uint32_t)grp[0].step)>>4;
            if( ctx15<15 ) {
              mir_top = (uint16_t*)&grp[1];
              if( grp[1].f[2]+grp[1].f[1]+grp[1].f[0]>29696 ) {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (5*(uint32_t)grp[1].step)>>4;
            }
            if( ctx15>0 ) {
              mir_top = (uint16_t*)&grp[-1];
              if( grp[-1].f[2]+grp[-1].f[1]+grp[-1].f[0]>29696 ) {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (6*(uint32_t)grp[-1].step)>>4;
            }
          }
          sel3 = blk->ctx_w[3].sel;
          if( sel3==1 ) {
            frec6 = &blk->freq[blk->ctx_w[3].w[0]+step_v-blk->ctx_w[3].w[1]];
            mir_top = (uint16_t*)frec6;
            if( frec6->f[2]+(blk->freq[blk->ctx_w[3].w[0]+step_v-blk->ctx_w[3].w[1]].f[1])+(blk->freq[blk->ctx_w[3].w[0]+step_v-blk->ctx_w[3].w[1]].f[0])>29696 ) {
              step_s = step_v;
              frec6->rescale_three_way();
              step_v = step_s;
            }
            mir_top[fold_sel2+1] += (uint16_t)(mir_top[0]&0xFFFC)>>2;
            off3 = blk->ctx-blk->ctx_w[3].w[1];
            frecg3 = (&blk->freq[off3+blk->ctx_w[3].w[0]]);
            idx0 = blk->ctx_w[3].w[2]+off3;
            grp = frecg3;
            h3 = &frecg3[0];
            if( h3->f[2]+h3->f[1]+h3->f[0]>29696 ) {
              step_s = step_v;
              h3->rescale_three_way();
              step_v = step_s;
            }
            h3->f[is_dec] += (3*(uint32_t)grp[0].step)>>4;
            if( ctx15<15 ) {
              mir_top = (uint16_t*)&grp[1];
              if( grp[1].f[2]+grp[1].f[1]+grp[1].f[0]>29696 ) {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (uint16_t)(grp[1].step&0xFFFC)>>2;
              nxt3 = &blk->freq[idx0+1];
              if( blk->freq[idx0+1].f[2]+blk->freq[idx0+1].f[1]+blk->freq[idx0+1].f[0]>29696 ) {
                step_s = step_v;
                (&blk->freq[idx0+1])->rescale_three_way();
                step_v = step_s;
              }
              nxt3->f[is_dec] += (nxt3->step&0xFFF8)>>3;
            }
            if( ctx15>2 ) {
              mir_top = (uint16_t*)&grp[-1];
              if( grp[-1].f[2]+grp[-1].f[1]+grp[-1].f[0]>29696 ) {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (6*(uint32_t)grp[-1].step)>>4;
            }
          } else {
            g3 = &blk->freq[step_v+blk->ctx_w[3].w[1]-blk->ctx_w[3].w[2-sel3]];
            if( blk->freq[step_v+blk->ctx_w[3].w[1]-blk->ctx_w[3].w[2-sel3]].f[2]+blk->freq[step_v+blk->ctx_w[3].w[1]-blk->ctx_w[3].w[2-sel3]].f[1]+blk->freq[step_v+blk->ctx_w[3].w[1]-blk->ctx_w[3].w[2-sel3]].f[0]>29696 ) {
              step_s = step_v;
              g3->rescale_three_way();
              step_v = step_s;
            }
            g3->f[fold_sel2] += (7*(uint32_t)g3->step)>>4;
            grp = ((&blk->freq[blk->ctx_w[3].w[1]+(blk->ctx-blk->ctx_w[3].w[blk->ctx_w[3].sel])]));
            mir_top = (uint16_t*)&grp[0];
            if( grp[0].f[2]+grp[0].f[1]+grp[0].f[0]>29696 ) {
              step_s = step_v;
              grp[0].rescale_three_way();
              step_v = step_s;
            }
            mir_top[is_dec+1] += (7*(uint32_t)grp[0].step)>>4;
            if( ctx15<15 ) {
              mir_top = (uint16_t*)&grp[1];
              if( grp[1].f[2]+grp[1].f[1]+grp[1].f[0]>29696 ) {
                step_s = step_v;
                grp[1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (5*(uint32_t)grp[1].step)>>4;
            }
            if( ctx15>0 ) {
              mir_top = (uint16_t*)&grp[-1];
              if( grp[-1].f[2]+grp[-1].f[1]+grp[-1].f[0]>29696 ) {
                step_s = step_v;
                grp[-1].rescale_three_way();
                step_v = step_s;
              }
              mir_top[is_dec+1] += (6*(uint32_t)grp[-1].step)>>4;
            }
          }
          sel4 = blk->ctx_w[4].sel;
          if( sel4==1 ) {
            frec7 = &blk->freq[blk->ctx_w[4].w[0]+step_v-blk->ctx_w[4].w[1]];
            mir_top = (uint16_t*)frec7;
            if( frec7->f[2]+(blk->freq[blk->ctx_w[4].w[0]+step_v-blk->ctx_w[4].w[1]].f[1])+(blk->freq[blk->ctx_w[4].w[0]+step_v-blk->ctx_w[4].w[1]].f[0])>29696 )
              frec7->rescale_three_way();
            mir_top[fold_sel2+1] += (uint16_t)(mir_top[0]&0xFFFC)>>2;
            off4 = blk->ctx-blk->ctx_w[4].w[1];
            frec4c = &blk->freq[off4+blk->ctx_w[4].w[0]];
            rec_idx = blk->ctx_w[4].w[2]+off4;
            p2_rec = frec4c;
            h4 = &frec4c[0];
            if( frec4c[0].f[2]+frec4c[0].f[1]+frec4c[0].f[0]>29696 )
              h4->rescale_three_way();
            h4->f[is_dec] += (3*(uint32_t)p2_rec[0].step)>>4;
            if( ctx15<15 ) {
              mir_top = (uint16_t*)&p2_rec[1];
              if( p2_rec[1].f[2]+(p2_rec[1].f[1]+p2_rec[1].f[0])>29696 )
                p2_rec[1].rescale_three_way();
              mir_top[is_dec+1] += (uint16_t)(p2_rec[1].step&0xFFFC)>>2;
              nxt4 = &blk->freq[rec_idx+1];
              if( nxt4->f[2]+nxt4->f[1]+nxt4->f[0]>29696 )
                nxt4->rescale_three_way();
              step_v = (nxt4->step&0xFFF8)>>3;
              nxt4->f[is_dec] += step_v;
            }
            if( ctx15>2 ) {
              prec_m1 = &p2_rec[-1];
              if( p2_rec[-1].f[2]+p2_rec[-1].f[1]+p2_rec[-1].f[0]>29696 )
                p2_rec[-1].rescale_three_way();
              prec_m1->f[is_dec] += (6*(uint32_t)p2_rec[-1].step)>>4;
            }
          } else {
            g4 = &blk->freq[step_v+blk->ctx_w[4].w[1]-blk->ctx_w[4].w[2-sel4]];
            if( blk->freq[step_v+blk->ctx_w[4].w[1]-blk->ctx_w[4].w[2-sel4]].f[2]+blk->freq[step_v+blk->ctx_w[4].w[1]-blk->ctx_w[4].w[2-sel4]].f[1]+blk->freq[step_v+blk->ctx_w[4].w[1]-blk->ctx_w[4].w[2-sel4]].f[0]>29696 )
              g4->rescale_three_way();
            g4->f[fold_sel2] += (7*(uint32_t)g4->step)>>4;
            gtop = ((&blk->freq[blk->ctx_w[4].w[1]+(blk->ctx-blk->ctx_w[4].w[blk->ctx_w[4].sel])]));
            if( gtop[0].f[2]+gtop[0].f[1]+gtop[0].f[0]>29696 )
              gtop[0].rescale_three_way();
            gtop[0].f[is_dec] += (7*(uint32_t)gtop[0].step)>>4;
            if( ctx15<15 ) {
              if( gtop[1].f[2]+gtop[1].f[1]+gtop[1].f[0]>29696 )
                gtop[1].rescale_three_way();
              step_v = gtop[1].step;
              gtop[1].f[is_dec] += (5*step_v)>>4;
            }
            if( ctx15>0 ) {
              if( gtop[-1].f[2]+gtop[-1].f[1]+gtop[-1].f[0]>29696 )
                gtop[-1].rescale_three_way();
              step_v = gtop[-1].step;
              gtop[-1].f[is_dec] += (6*step_v)>>4;
            }
          }
        }
        return step_v;
      }
      frec = (&blk->freq[blk->ctx]);
    }
    mir_top = (uint16_t*)&frec[-1];
    if( frec[-1].f[2]+frec[-1].f[1]+frec[-1].f[0]>29696 )
      frec[-1].rescale_three_way();
    step13 = (13*(uint32_t)frec[-1].step)>>4;
    if( a4 ) {
      mir_top[3-is_dec] += step13;
      __update_binary_pair(model_strip(pair_ctx-1), (a4-1)>>1);
    } else {
      mir_top[1] += step13;
    }
    goto LABEL_37;
  }
  return step_v;
}

void __alt_p2_d8_decode_body(AltP2Block* blk, int8_t unread_flag, uint8_t* out, int32_t width, int32_t height) {
  P2Ctx* pix, * prev, * rec, * buf3, * b4, * buf2, * buf1, * b0;
  int32_t x;
  uint32_t y;
  uint8_t* row;
  int32_t code;
  float (**cur)[4], (**r1)[4];
  bool more;
  int16_t val;
  float (*last)[4], (**r0)[4];
  int32_t wid, q, pred, val2;
  int64_t err;
  uint32_t j;
  __rc_begin_decode(unread_flag);
  wid = width;
  pix = blk->cursor[0];
  prev = pix;
  if( width>0 ) {
    x = 0;
    while( 1 ) {
      q = pix[-1].val>>4;
      blk->ctx_pair[0] = blk->ctx+(blk->ctx_delta[q+4]);
      blk->ctx_pair[1] = blk->ctx+(blk->ctx_delta[q]);
      val = (uint8_t)(((uint16_t)blk->cursor[0][-1].val>>4)+(uint8_t)blk->unfold[blk->freq[blk->ctx+blk->ctx_w[3].w[(prev[-1].val<=prev[-2].val)+(prev[-1].val<prev[-2].val)]+blk->ctx_w[2].w[prev[-2].sign]+blk->ctx_w[1].w[prev[-1].sign]+blk->ctx_w[0].w[(((uint32_t)(q-115)>>31)+((uint32_t)(q-17)>>31))]+blk->ctx_w[4].w[1]].decode_symbol(blk->ctx_pair)]);
      *out = val;
      val *= 16;
      blk->cursor[0]->val = val;
      ++out;
      blk->cursor[0]->dval = val;
      rec = blk->cursor[0];
      err = (int16_t)(rec->val-rec[-1].val);
      rec->err = err;
      LOWORD(err) = (WORD2(err)^err)-WORD2(err);
      blk->cursor[0]->aerr = err;
      blk->cursor[0]->dupright = err;
      blk->cursor[0]->dupleft = err;
      blk->cursor[0]->dup = err;
      blk->cursor[0]->dleft = (uint32_t)blk->cursor[0]->dup>>1;
      blk->cursor[0]->mag = 2;
      blk->cursor[0]->sign = (blk->cursor[0]->err<=0)+(blk->cursor[0]->err<0);
      pix = (blk->cursor[0]+1);
      more = x+1<width;
      blk->cursor[0] = pix;
      ++x;
      if( !more )
        break;
      prev = blk->cursor[0];
    }
    wid = width;
  }
  {
    P2Ctx*const here = blk->cursor[0];
    here[0] = here[-1];
    here[1] = here[-1];
    here[2] = here[-1];
    here[3] = here[-1];
    here[4] = here[-1];
  }
  P2Ctx*const rec1 = blk->cursor[0]+(-wid);
  rec1[-1] = rec1[0];
  rec1[-2] = rec1[1];
  rec1[-3] = rec1[2];
  rec1[-4] = rec1[3];
  rec1[-5] = rec1[4];
  rec1[-6] = rec1[5];
  rec1[-7] = rec1[6];
  rec1[-8] = rec1[7];
  memcpy(blk->buf[1], blk->buf[0], 18*wid+234);
  memcpy(blk->buf[2], blk->buf[0], 18*wid+234);
  memcpy(blk->buf[3], blk->buf[0], 18*wid+234);
  if( height>1 ) {
    y = 0;
    do {
      cur = blk->cur;
      last = cur[-1];
      row = out;
      cur[1] = last;
      *blk->cur = last;
      r0 = blk->row0;
      r1 = blk->row1;
      blk->row0 = r1;
      blk->row1 = r0;
      r1 += 2;
      r0 += 2;
      blk->cur = r1;
      blk->above = r0;
      r1[-1] = *r0;
      blk->cur[-2] = *r0;
      memset(blk->p2_row0_head, 0, sizeof blk->p2_row0_head);
      blk->bias[0] = 0.0f;
      blk->bias[1] = 0.0f;
      blk->bias[2] = 0.0f;
      blk->bias[3] = 0.0f;
      __builtin_memset(blk->p2_row, 0, sizeof blk->p2_row);
      {
        P2Ctx*const here = blk->cursor[0];
        here[0] = here[-1];
        here[1] = here[-2];
        here[2] = here[-3];
        here[3] = here[-4];
        here[4] = here[-5];
      }
      buf3 = blk->buf[3];
      b4 = blk->buf[4];
      buf2 = blk->buf[2];
      buf1 = blk->buf[1];
      b0 = blk->buf[0];
      blk->buf[4] = buf3;
      blk->buf[3] = buf2;
      blk->buf[2] = buf1;
      blk->buf[0] = b4;
      blk->buf[1] = b0;
      b4 += 8;
      blk->cursor[0] = b4;
      b0 += 8;
      blk->cursor[1] = b0;
      blk->cursor[2] = buf1+8;
      blk->cursor[3] = buf2+8;
      blk->cursor[4] = buf3+8;
      ((P2Ctx*)b4)[-1] = ((P2Ctx*)b0)[0];
      {
        P2Ctx*const here = blk->cursor[0];
        P2Ctx*const up = blk->cursor[1];
        here[-2] = up[1];
        here[-3] = up[2];
        here[-4] = up[3];
        here[-5] = up[4];
        here[-6] = up[5];
        here[-7] = up[6];
        here[-8] = up[7];
      }
      blk->cursor[0]->dval = 0;
      if( width>0 ) {
        for( j = 0; j<(uint32_t)width; ++j ) {
          pred = __alt_p2_context((AltP2Block*)blk, (AltP2Block*)nullptr, (AltP2Block*)nullptr);
          code = blk->freq[blk->ctx].decode_symbol(blk->ctx_pair);
          val2 = (uint8_t)(pred+(blk->unfold[code]));
          row[j] = val2;
          __alt_p2_model(blk, val2, code, val2-pred);
          out = &row[j+1];
        }
      }
      ++y;
    } while( y<(uint32_t)(height-1) );
  }
  __rc_end_decode();
}

void __alt_model_p2_d8_decode(uint8_t* out, int32_t i, int32_t height) {
  AltP2Block* raw, * blk;
  raw = (AltP2Block*)bmf_page_alloc(sizeof(AltP2Block));
  if( raw )
    blk = raw->alt_p2_alloc(i, 0);
  else
    blk = nullptr;
  __alt_p2_d8_decode_body(blk, i, out, i, height);
  if( blk )
    blk->alt_p2_free(1);
}

int32_t __alt_model_p2_decode(uint16_t* p_i, uint8_t* out) {
  P2Ctx* cur0, * cur1, * cur2, * cur3, * buf3, * bb4, * buf2, * buf1, * bb0;
  int32_t xf4, val3;
  AltP2Block* plane[4], ** w;
  uint32_t row_bytes, i_1, first, i_2, np;
  int32_t back;
  AltP2Block* blk_r, * src3, * blk_k, * blk2, * blk3, * blk1;
  int16_t seed1;
  uint32_t i;
  int16_t seed3, seed2;
  P2Freq* freq;
  AltP2Block* blk0;
  uint32_t src2;
  AltP2Block** planep;
  float saved_p2_coef[7][4];
  float (**cur)[4], (**r1)[4];
  bool xf3, off;
  float (*b4)[4], (**b0)[4];
  int32_t pl2, dc_flag, xf0, raw, pl, nplanes, xf1, xf2, pred0, code0, val0, pred1, code1, val1, l7a, l4a, l5a, pred2, code2, val2, l7b, l4b, l5b, pred3, code3, l7c, l4c, l5c, nplanes2, pl3;
  void* made, * src1, ** plane_p;
  __builtin_memcpy(saved_p2_coef, bmf_p2_coef, sizeof saved_p2_coef);
  for( int32_t k = 0; k<4; k++ ) {
    bmf_p2_coef[0][k] += bmf_p2_coef[4][k];
    bmf_p2_coef[1][k] += bmf_p2_coef[5][k];
    bmf_p2_coef[2][k] += bmf_p2_coef[6][k];
    bmf_p2_coef[4][k] = 0;
    bmf_p2_coef[5][k] = 0;
    bmf_p2_coef[6][k] = 0;
  }
  float saved_p2_rate[3][4];
  __builtin_memcpy(saved_p2_rate, bmf_p2_rate[4], sizeof saved_p2_rate);
  for( int32_t k = 0; k<4; k++ ) {
    bmf_p2_rate[4][k] = bmf_p2_rate_reset;
    bmf_p2_rate[5][k] = bmf_p2_rate_reset;
    bmf_p2_rate[6][k] = bmf_p2_rate_reset;
  }
  i = *p_i;
  raw = p_i[1];
  if( plane_count>0 ) {
    pl = 0;
    do {
      made = bmf_page_alloc(sizeof(AltP2Block));
      if( made )
        src1 = ((AltP2Block*)made)->alt_p2_alloc(i, pl);
      else
        src1 = nullptr;
      plane[pl++] = (AltP2Block*)src1;
    } while( pl<plane_count );
  }
  xf0 = plane_desc[plane_desc[2].src_plane+1].flags&8;
  dc_flag = plane_desc[plane_desc[3].src_plane+1].flags&8;
  xf4 = (plane_desc[plane_desc[4].src_plane+1].flags)&8;
  __rc_begin_decode(dc_flag);
  if( raw>0 ) {
    src2 = 0;
    back = -i;
    nplanes = plane_count;
    row_bytes = 18*i+234;
    np = i+13;
    do {
      if( nplanes>0 ) {
        first = src2;
        pl2 = 0;
        i_1 = i;
        do {
          ++pl2;
          if( first ) {
            planep = &plane[pl2];
            if( first==1 ) {
              blk_k = (AltP2Block*)(*(planep-1));
              {
                P2Ctx*const here = blk_k->cursor[0];
                here[0] = here[-1];
                here[1] = here[-1];
                here[2] = here[-1];
                here[3] = here[-1];
                here[4] = here[-1];
              }
              P2Ctx*const rec2 = blk_k->cursor[0]+back;
              rec2[-1] = rec2[0];
              rec2[-2] = rec2[1];
              rec2[-3] = rec2[2];
              rec2[-4] = rec2[3];
              rec2[-5] = rec2[4];
              rec2[-6] = rec2[5];
              rec2[-7] = rec2[6];
              rec2[-8] = rec2[7];
              memcpy(blk_k->buf[1], blk_k->buf[0], row_bytes);
              memcpy(blk_k->buf[2], blk_k->buf[0], row_bytes);
              memcpy(blk_k->buf[3], blk_k->buf[0], row_bytes);
            }
          } else {
            src3 = (AltP2Block*)(plane[pl2-1]);
            w = &plane[pl2];
            xf1 = 0;
            do {
              xf2 = xf1;
              src3->buf[0][xf2].val = 256;
              xf3 = (uint32_t)(++xf1)<np;
              src3->buf[0][xf2].dval = 256;
              src3->buf[0][xf2].err = -16;
              src3->buf[0][xf2].sign = 1;
              src3->buf[0][xf2].mag = 3;
              src3->buf[0][xf2].aerr = 512;
              src3->buf[0][xf2].dupright = 512;
              src3->buf[0][xf2].dupleft = 512;
              src3->buf[0][xf2].dup = 1024;
              src3->buf[0][xf2].dleft = 256;
            } while( xf3 );
            planep = w;
            memcpy(src3->buf[1], src3->buf[0], row_bytes);
            memcpy(src3->buf[2], src3->buf[0], row_bytes);
            memcpy(src3->buf[3], src3->buf[0], row_bytes);
            src3->cursor[0] = src3->buf[0]+i+8;
            src3->cursor[1] = (src3->buf[1]+i+8);
            src3->cursor[2] = src3->buf[2]+i+8;
            src3->cursor[3] = src3->buf[3]+i+8;
            src3->cursor[4] = (src3->buf[4]+i+8);
          }
          blk_r = (AltP2Block*)(*(planep-1));
          cur = blk_r->cur;
          b4 = cur[-1];
          cur[1] = b4;
          *blk_r->cur = b4;
          b0 = blk_r->row0;
          r1 = blk_r->row1;
          blk_r->row0 = r1;
          blk_r->row1 = b0;
          r1 += 2;
          blk_r->cur = r1;
          b0 += 2;
          blk_r->above = b0;
          r1[-1] = *b0;
          blk_r->cur[-2] = *b0;
          memset(blk_r->p2_row0_head, 0, sizeof blk_r->p2_row0_head);
          blk_r->bias[0] = 0.0f;
          blk_r->bias[1] = 0.0f;
          blk_r->bias[2] = 0.0f;
          blk_r->bias[3] = 0.0f;
          __builtin_memset(blk_r->p2_row, 0, sizeof blk_r->p2_row);
          {
            P2Ctx*const here = blk_r->cursor[0];
            here[0] = here[-1];
            here[1] = here[-2];
            here[2] = here[-3];
            here[3] = here[-4];
            here[4] = here[-5];
          }
          buf3 = blk_r->buf[3];
          bb4 = blk_r->buf[4];
          buf2 = blk_r->buf[2];
          buf1 = blk_r->buf[1];
          bb0 = blk_r->buf[0];
          blk_r->buf[4] = buf3;
          blk_r->buf[3] = buf2;
          blk_r->buf[2] = buf1;
          blk_r->buf[0] = bb4;
          blk_r->buf[1] = bb0;
          bb4 += 8;
          blk_r->cursor[0] = bb4;
          bb0 += 8;
          blk_r->cursor[1] = bb0;
          blk_r->cursor[2] = buf1+8;
          blk_r->cursor[3] = buf2+8;
          blk_r->cursor[4] = buf3+8;
          ((P2Ctx*)bb4)[-1] = ((P2Ctx*)bb0)[0];
          {
            P2Ctx*const here = blk_r->cursor[0];
            P2Ctx*const up = blk_r->cursor[1];
            here[-2] = up[1];
            here[-3] = up[2];
            here[-4] = up[3];
            here[-5] = up[4];
            here[-6] = up[5];
            here[-7] = up[6];
            here[-8] = up[7];
          }
          blk_r->cursor[0]->dval = 0;
          nplanes = plane_count;
        } while( plane_count>pl2 );
        src2 = first;
        i = i_1;
      }
      ctx_bias[3] = 0;
      ctx_bias[2] = 0;
      ctx_bias[1] = 0;
      ctx_bias[0] = 0;
      if( i>0 ) {
        first = src2;
        i_2 = 0;
        i_1 = i;
        do {
          ctx_bias[0] >>= 3;
          ctx_bias[1] >>= 3;
          ctx_bias[2] >>= 3;
          ctx_bias[3] >>= 3;
          blk0 = (AltP2Block*)(plane[0]);
          pred0 = __alt_p2_context((AltP2Block*)plane[0], (AltP2Block*)plane[2], (AltP2Block*)plane[1]);
          freq = &blk0->freq[blk0->ctx];
          code0 = freq->decode_symbol(blk0->ctx_pair);
          val0 = (uint8_t)(pred0+(blk0->unfold[code0]));
          __alt_p2_model((AltP2Block*)blk0, val0, code0, val0-pred0);
          cur0 = blk0->cursor[0];
          ctx_bias[0] += 32*cur0[-1].dupleft;
          ctx_bias[1] += (32*cur0[-1].dupright);
          ctx_bias[2] += 32*cur0[-1].dleft;
          ctx_bias[3] += 32*cur0[-1].dup;
          off = xf0==0;
          out[plane_desc[1].src_plane] = val0;
          if( off )
            seed1 = 0;
          else
            seed1 = 16*out[plane_desc[1].src_plane];
          blk1 = (AltP2Block*)(plane[1]);
          plane[1]->cursor[0]->dval = seed1;
          pred1 = __alt_p2_context((AltP2Block*)blk1, (AltP2Block*)plane[0], (AltP2Block*)plane[2]);
          code1 = blk1->freq[blk1->ctx].decode_symbol(blk1->ctx_pair);
          val1 = (uint8_t)(pred1+blk1->unfold[code1]);
          __alt_p2_model((AltP2Block*)blk1, val1, code1, val1-pred1);
          cur1 = blk1->cursor[0];
          l7a = cur1[-1].dupright;
          l4a = cur1[-1].dleft;
          ctx_bias[0] += 32*cur1[-1].dupleft;
          l5a = cur1[-1].dup;
          ctx_bias[1] += 32*l7a;
          ctx_bias[2] += 32*l4a;
          ctx_bias[3] += 32*l5a;
          off = dc_flag==0;
          out[plane_desc[2].src_plane] = val1;
          if( off )
            seed2 = 0;
          else
            seed2 = (plane_desc[plane_desc[3].src_plane+1].weight1*out[plane_desc[2].src_plane]+plane_desc[plane_desc[3].src_plane+1].weight0*out[plane_desc[1].src_plane])>>3;
          blk2 = (AltP2Block*)(plane[2]);
          plane[2]->cursor[0]->dval = seed2;
          pred2 = __alt_p2_context((AltP2Block*)blk2, (AltP2Block*)plane[0], (AltP2Block*)plane[1]);
          code2 = blk2->freq[blk2->ctx].decode_symbol(blk2->ctx_pair);
          val2 = (uint8_t)(pred2+blk2->unfold[code2]);
          __alt_p2_model((AltP2Block*)blk2, val2, code2, val2-pred2);
          cur2 = blk2->cursor[0];
          l7b = cur2[-1].dupright;
          l4b = cur2[-1].dleft;
          ctx_bias[0] += 32*cur2[-1].dupleft;
          l5b = cur2[-1].dup;
          ctx_bias[1] += 32*l7b;
          ctx_bias[2] += 32*l4b;
          ctx_bias[3] += 32*l5b;
          out[plane_desc[3].src_plane] = val2;
          nplanes = plane_count;
          if( plane_count>=4 ) {
            if( xf4 )
              seed3 = (plane_desc[plane_desc[4].src_plane+1].weight2*out[2]+plane_desc[plane_desc[4].src_plane+1].weight1*out[1]+plane_desc[plane_desc[4].src_plane+1].weight0**out)>>3;
            else
              seed3 = 0;
            blk3 = (AltP2Block*)(plane[3]);
            plane[3]->cursor[0]->dval = seed3;
            pred3 = __alt_p2_context((AltP2Block*)blk3, (AltP2Block*)plane[2], (AltP2Block*)plane[0]);
            code3 = blk3->freq[blk3->ctx].decode_symbol(blk3->ctx_pair);
            val3 = (uint8_t)(pred3+blk3->unfold[code3]);
            __alt_p2_model((AltP2Block*)blk3, val3, code3, val3-pred3);
            cur3 = blk3->cursor[0];
            l7c = cur3[-1].dupright;
            l4c = cur3[-1].dleft;
            l5c = cur3[-1].dup;
            ctx_bias[0] += 32*cur3[-1].dupleft;
            ctx_bias[1] += 32*l7c;
            ctx_bias[2] += 32*l4c;
            ctx_bias[3] += 32*l5c;
            out[3] = val3;
            nplanes = plane_count;
          }
          out += nplanes;
          ++i_2;
        } while( i_2<i_1 );
        src2 = first;
        i = i_1;
      }
      ++src2;
    } while( src2<(uint32_t)raw );
  }
  __rc_end_decode();
  __builtin_memcpy(bmf_p2_coef, saved_p2_coef, sizeof saved_p2_coef);
  __builtin_memcpy(bmf_p2_rate[4], saved_p2_rate, sizeof saved_p2_rate);
  nplanes2 = plane_count;
  if( plane_count>0 ) {
    pl3 = 0;
    do {
      plane_p = (void**)plane[pl3];
      if( plane_p ) {
        ((AltP2Block*)plane_p)->alt_p2_free(1);
        nplanes2 = plane_count;
      }
      ++pl3;
    } while( pl3<nplanes2 );
  }
  return nplanes2;
}

void __unmodel_plane(int8_t unread_flag, uint16_t* p_i, uint8_t* out) {
  ModelBlock* blk;
  void* raw;
  if( plane_alt_model ) {
    if( plane_predictor==1 ) {
      if( (p_i[5]&0x3F)==8 )
        __alt_model_p1_d8_decode(unread_flag, out, *p_i, p_i[1]);
      else
        __alt_model_p1_decode(p_i, out);
    } else if( plane_predictor==2 ) {
      if( (p_i[5]&0x3F)==8 )
        __alt_model_p2_d8_decode(out, *p_i, p_i[1]);
      else
        __alt_model_p2_decode(p_i, out);
    }
  } else {
    raw = bmf_new(sizeof(ModelBlock));
    if( raw )
      blk = __layout_workspace((ModelBlock*)raw, p_i[1], *p_i, p_i[1], p_i[5]&0x3F);
    else
      blk = (ModelBlock*)(nullptr);
    ((ModelBlock*)blk)->unmodel_plane_slow(out);
    if( blk )
      __free_workspace((ModelBlock*)blk, 1);
  }
}

void __alt_p2_d8_encode_body(AltP2Block* blk, uint8_t* src, int32_t width, int32_t height, uint8_t* out) {
  int32_t val;
  P2Ctx* pix, * rec, * buf3, * b4, * buf2, * buf1, * b0;
  int32_t j;
  uint32_t y;
  uint8_t* p;
  int32_t code0;
  uint8_t* outp, want, * srcp, code2;
  uint32_t* pair;
  float (**cur)[4], (**r1)[4];
  uint8_t recon, recon2;
  float (*last)[4], (**r0)[4];
  int32_t q, resid, drift, q2, row_bytes, pred, resid2, drift2, code;
  int64_t err;
  P2Ctx* rec2;
  uint32_t k;
  uint8_t* src_end;
  src_end = src;
  __rc_begin_encode();
  pix = blk->cursor[0];
  if( width>0 ) {
    p = src;
    for( j = 0; j<width; ++j ) {
      q = (uint16_t)pix[-1].val>>4;
      resid = (uint8_t)(*p-q);
      code0 = blk->fold[resid];
      recon = blk->unfold[code0]+q;
      drift = (uint8_t)*out-(uint8_t)(recon+*out-*p);
      if( drift<-16||drift>16 ) {
        *out = *p;
        code0 = blk->fold_hi[resid];
      } else {
        *out = recon;
      }
      rec = blk->cursor[0];
      q2 = rec[-1].val>>4;
      blk->ctx_pair[0] = blk->ctx+blk->ctx_delta[q2+4];
      blk->ctx_pair[1] = blk->ctx+blk->ctx_delta[q2];
      blk->freq[blk->ctx+blk->ctx_w[3].w[(rec[-1].val<=rec[-2].val)+(rec[-1].val<rec[-2].val)]+blk->ctx_w[2].w[rec[-2].sign]+blk->ctx_w[1].w[rec[-1].sign]+blk->ctx_w[0].w[((uint32_t)(q2-115)>>31)+((uint32_t)(q2-17)>>31)]+blk->ctx_w[4].w[1]].encode_symbol(blk->ctx_pair, code0);
      ++p;
      val = 16*(uint8_t)*out;
      blk->cursor[0]->val = val;
      blk->cursor[0]->dval = val;
      ++out;
      rec2 = blk->cursor[0];
      err = (int16_t)(rec2->val-rec2[-1].val);
      rec2->err = err;
      LOWORD(err) = (WORD2(err)^err)-WORD2(err);
      blk->cursor[0]->aerr = err;
      blk->cursor[0]->dupright = err;
      blk->cursor[0]->dupleft = err;
      blk->cursor[0]->dup = err;
      blk->cursor[0]->dleft = (uint32_t)blk->cursor[0]->dup>>1;
      blk->cursor[0]->mag = 2;
      blk->cursor[0]->sign = (blk->cursor[0]->err<=0)+(blk->cursor[0]->err<0);
      pix = (blk->cursor[0]+1);
      blk->cursor[0] = pix;
    }
    src_end = p;
  }
  {
    P2Ctx*const here = blk->cursor[0];
    here[0] = here[-1];
    here[1] = here[-1];
    here[2] = here[-1];
    here[3] = here[-1];
    here[4] = here[-1];
  }
  P2Ctx*const rec3 = blk->cursor[0]+(-width);
  rec3[-1] = rec3[0];
  rec3[-2] = rec3[1];
  rec3[-3] = rec3[2];
  rec3[-4] = rec3[3];
  rec3[-5] = rec3[4];
  rec3[-6] = rec3[5];
  rec3[-7] = rec3[6];
  rec3[-8] = rec3[7];
  row_bytes = 18*width+234;
  memcpy(blk->buf[1], blk->buf[0], row_bytes);
  memcpy(blk->buf[2], blk->buf[0], row_bytes);
  memcpy(blk->buf[3], blk->buf[0], row_bytes);
  if( height>1 ) {
    pair = blk->ctx_pair;
    outp = out;
    y = 0;
    do {
      cur = blk->cur;
      last = cur[-1];
      srcp = src_end;
      cur[1] = last;
      *blk->cur = last;
      r0 = blk->row0;
      r1 = blk->row1;
      blk->row0 = r1;
      blk->row1 = r0;
      r1 += 2;
      blk->cur = r1;
      r0 += 2;
      blk->above = r0;
      r1[-1] = *r0;
      blk->cur[-2] = *r0;
      memset(blk->p2_row0_head, 0, sizeof blk->p2_row0_head);
      blk->bias[0] = 0.0f;
      blk->bias[1] = 0.0f;
      blk->bias[2] = 0.0f;
      blk->bias[3] = 0.0f;
      __builtin_memset(blk->p2_row, 0, sizeof blk->p2_row);
      {
        P2Ctx*const here = blk->cursor[0];
        here[0] = here[-1];
        here[1] = here[-2];
        here[2] = here[-3];
        here[3] = here[-4];
        here[4] = here[-5];
      }
      buf3 = blk->buf[3];
      b4 = blk->buf[4];
      buf2 = blk->buf[2];
      buf1 = blk->buf[1];
      b0 = blk->buf[0];
      blk->buf[4] = buf3;
      blk->buf[3] = buf2;
      blk->buf[2] = buf1;
      blk->buf[0] = b4;
      blk->buf[1] = b0;
      b4 += 8;
      blk->cursor[0] = b4;
      b0 += 8;
      blk->cursor[1] = b0;
      blk->cursor[2] = buf1+8;
      blk->cursor[3] = buf2+8;
      blk->cursor[4] = buf3+8;
      ((P2Ctx*)b4)[-1] = ((P2Ctx*)b0)[0];
      {
        P2Ctx*const here = blk->cursor[0];
        P2Ctx*const up = blk->cursor[1];
        here[-2] = up[1];
        here[-3] = up[2];
        here[-4] = up[3];
        here[-5] = up[4];
        here[-6] = up[5];
        here[-7] = up[6];
        here[-8] = up[7];
      }
      blk->cursor[0]->dval = 0;
      if( width>0 ) {
        for( k = 0; k<(uint32_t)width; ++k ) {
          pred = __alt_p2_context((AltP2Block*)blk, (AltP2Block*)nullptr, (AltP2Block*)nullptr);
          want = srcp[k];
          resid2 = (uint8_t)(want-pred);
          recon2 = pred+blk->unfold[blk->fold[resid2]];
          drift2 = (uint8_t)outp[k]-(uint8_t)(recon2+outp[k]-want);
          code = blk->fold[resid2];
          if( drift2<-16||drift2>16 ) {
            outp[k] = want;
            code = blk->fold_hi[(uint8_t)(want-pred)];
          } else {
            outp[k] = recon2;
          }
          code2 = code;
          blk->freq[blk->ctx].encode_symbol(pair, code);
          __alt_p2_model((AltP2Block*)blk, (uint8_t)outp[k], code2, (uint8_t)outp[k]-pred);
        }
        src_end = &srcp[width];
        outp = &outp[width];
      }
      ++y;
    } while( y<(uint32_t)(height-1) );
  }
  __rc_end_encode();
}

void __alt_model_p2_d8_encode(uint8_t* src, int32_t i, int32_t height, uint8_t* out) {
  AltP2Block* raw, * blk;
  raw = (AltP2Block*)bmf_page_alloc(sizeof(AltP2Block));
  if( raw )
    blk = raw->alt_p2_alloc(i, 0);
  else
    blk = nullptr;
  __alt_p2_d8_encode_body(blk, src, i, height, out);
  if( blk )
    blk->alt_p2_free(1);
}

int32_t __alt_model_p2_encode(BmfImage* p_i, uint8_t* a2) {
  P2Ctx* rec0, * rec1, * rec2, * rec3, * buf3, * bb4, * buf2, * buf1, * bb0;
  int32_t y, alpha_src, x, w;
  AltP2Block* plane[4];
  uint32_t row_bytes;
  uint8_t* out;
  uint32_t xf4;
  int32_t pl2;
  uint32_t i, code3;
  AltP2Block** back;
  int32_t first;
  int32_t np, xf0;
  int32_t recon0b, code2b, recon1b, off1, off2, pred0, pred1, pred2, resid0, resid2, resid1, off0b;
  AltP2Block* blk_r, * src2, * blk_k, * blk2, * l4c, * blk1;
  bool xf2;
  int16_t seed1, l7c, seed2;
  uint32_t src1;
  AltP2Block* blk0, ** xf3;
  float (**cur)[4], (**r1)[4];
  float (*b4)[4], (**b0)[4];
  int32_t row_i, height, pl, nplanes, src3, xf1, want0, off0, seed0, at0, cur0, code0, out0, recon0, drift0, l7a, l4a, l5a, cur1, code1, out1, recon1, drift1, l7b, l4b, l5b, cur2, code2, out2, recon2, drift2, l5c, seed3, blk3, cur3, drift3, nplanes2, pl3;
  void* raw, * made, ** plane_p;
  out = a2;
  float saved_p2_coef[7][4];
  __builtin_memcpy(saved_p2_coef, bmf_p2_coef, sizeof saved_p2_coef);
  for( int32_t k = 0; k<4; k++ ) {
    bmf_p2_coef[0][k] += bmf_p2_coef[4][k];
    bmf_p2_coef[1][k] += bmf_p2_coef[5][k];
    bmf_p2_coef[2][k] += bmf_p2_coef[6][k];
    bmf_p2_coef[4][k] = 0;
    bmf_p2_coef[5][k] = 0;
    bmf_p2_coef[6][k] = 0;
  }
  float saved_p2_rate[3][4];
  __builtin_memcpy(saved_p2_rate, bmf_p2_rate[4], sizeof saved_p2_rate);
  for( int32_t k = 0; k<4; k++ ) {
    bmf_p2_rate[4][k] = bmf_p2_rate_reset;
    bmf_p2_rate[5][k] = bmf_p2_rate_reset;
    bmf_p2_rate[6][k] = bmf_p2_rate_reset;
  }
  row_i = p_i->width;
  height = p_i->height;
  if( plane_count>0 ) {
    pl = 0;
    do {
      raw = bmf_page_alloc(sizeof(AltP2Block));
      if( raw )
        made = (void*)((AltP2Block*)raw)->alt_p2_alloc(row_i, pl);
      else
        made = nullptr;
      plane[pl++] = (AltP2Block*)made;
    } while( pl<plane_count );
  }
  xf0 = plane_desc[plane_desc[2].src_plane+1].flags&8;
  np = plane_desc[plane_desc[3].src_plane+1].flags&8;
  y = (plane_desc[plane_desc[4].src_plane+1].flags)&8;
  __rc_begin_encode();
  if( height>0 ) {
    src1 = 0;
    first = -row_i;
    row_bytes = 18*row_i+234;
    code3 = row_i+13;
    nplanes = plane_count;
    do {
      if( nplanes>0 ) {
        xf4 = src1;
        pl2 = 0;
        do {
          ++pl2;
          if( xf4 ) {
            xf3 = &plane[pl2];
            if( xf4==1 ) {
              blk_k = (AltP2Block*)(*(xf3-1));
              {
                P2Ctx*const here = blk_k->cursor[0];
                here[0] = here[-1];
                here[1] = here[-1];
                here[2] = here[-1];
                here[3] = here[-1];
                here[4] = here[-1];
              }
              P2Ctx*const rec4 = blk_k->cursor[0]+first;
              rec4[-1] = rec4[0];
              rec4[-2] = rec4[1];
              rec4[-3] = rec4[2];
              rec4[-4] = rec4[3];
              rec4[-5] = rec4[4];
              rec4[-6] = rec4[5];
              rec4[-7] = rec4[6];
              rec4[-8] = rec4[7];
              memcpy(blk_k->buf[1], blk_k->buf[0], row_bytes);
              memcpy(blk_k->buf[2], blk_k->buf[0], row_bytes);
              memcpy(blk_k->buf[3], blk_k->buf[0], row_bytes);
            }
          } else {
            src2 = (AltP2Block*)(plane[pl2-1]);
            back = &plane[pl2];
            src3 = 0;
            do {
              xf1 = src3;
              src2->buf[0][xf1].val = 256;
              xf2 = (uint32_t)(++src3)<code3;
              src2->buf[0][xf1].dval = 256;
              src2->buf[0][xf1].err = -16;
              src2->buf[0][xf1].sign = 1;
              src2->buf[0][xf1].mag = 3;
              src2->buf[0][xf1].aerr = 512;
              src2->buf[0][xf1].dupright = 512;
              src2->buf[0][xf1].dupleft = 512;
              src2->buf[0][xf1].dup = 1024;
              src2->buf[0][xf1].dleft = 256;
            } while( xf2 );
            xf3 = back;
            memcpy(src2->buf[1], src2->buf[0], row_bytes);
            memcpy(src2->buf[2], src2->buf[0], row_bytes);
            memcpy(src2->buf[3], src2->buf[0], row_bytes);
            src2->cursor[0] = src2->buf[0]+row_i+8;
            src2->cursor[1] = (src2->buf[1]+row_i+8);
            src2->cursor[2] = src2->buf[2]+row_i+8;
            src2->cursor[3] = src2->buf[3]+row_i+8;
            src2->cursor[4] = (src2->buf[4]+row_i+8);
          }
          blk_r = (AltP2Block*)(*(xf3-1));
          cur = blk_r->cur;
          b4 = cur[-1];
          cur[1] = b4;
          *blk_r->cur = b4;
          b0 = blk_r->row0;
          r1 = blk_r->row1;
          blk_r->row0 = r1;
          blk_r->row1 = b0;
          r1 += 2;
          blk_r->cur = r1;
          b0 += 2;
          blk_r->above = b0;
          r1[-1] = *b0;
          blk_r->cur[-2] = *b0;
          memset(blk_r->p2_row0_head, 0, sizeof blk_r->p2_row0_head);
          blk_r->bias[0] = 0.0f;
          blk_r->bias[1] = 0.0f;
          blk_r->bias[2] = 0.0f;
          blk_r->bias[3] = 0.0f;
          __builtin_memset(blk_r->p2_row, 0, sizeof blk_r->p2_row);
          {
            P2Ctx*const here = blk_r->cursor[0];
            here[0] = here[-1];
            here[1] = here[-2];
            here[2] = here[-3];
            here[3] = here[-4];
            here[4] = here[-5];
          }
          buf3 = blk_r->buf[3];
          bb4 = blk_r->buf[4];
          buf2 = blk_r->buf[2];
          buf1 = blk_r->buf[1];
          bb0 = blk_r->buf[0];
          blk_r->buf[4] = buf3;
          blk_r->buf[3] = buf2;
          blk_r->buf[2] = buf1;
          blk_r->buf[0] = bb4;
          blk_r->buf[1] = bb0;
          bb4 += 8;
          blk_r->cursor[0] = bb4;
          bb0 += 8;
          blk_r->cursor[1] = bb0;
          blk_r->cursor[2] = buf1+8;
          blk_r->cursor[3] = buf2+8;
          blk_r->cursor[4] = buf3+8;
          ((P2Ctx*)bb4)[-1] = ((P2Ctx*)bb0)[0];
          {
            P2Ctx*const here = blk_r->cursor[0];
            P2Ctx*const up = blk_r->cursor[1];
            here[-2] = up[1];
            here[-3] = up[2];
            here[-4] = up[3];
            here[-5] = up[4];
            here[-6] = up[5];
            here[-7] = up[6];
            here[-8] = up[7];
          }
          blk_r->cursor[0]->dval = 0;
          nplanes = plane_count;
        } while( plane_count>pl2 );
        src1 = xf4;
      }
      if( row_i<=0 ) {
        ctx_bias[0] = 0;
        ctx_bias[1] = 0;
        ctx_bias[2] = 0;
        ctx_bias[3] = 0;
      } else {
        xf4 = src1;
        want0 = 0;
        off0 = 0;
        seed0 = 0;
        at0 = 0;
        for( i = 0; i<(uint32_t)row_i; ++i ) {
          off0b = plane_desc[1].src_plane;
          ctx_bias[0] = at0>>3;
          ctx_bias[1] = seed0>>3;
          ctx_bias[2] = off0>>3;
          ctx_bias[3] = want0>>3;
          blk0 = (AltP2Block*)(plane[0]);
          cur0 = out[plane_desc[1].src_plane];
          pred0 = __alt_p2_context((AltP2Block*)plane[0], (AltP2Block*)plane[2], (AltP2Block*)plane[1]);
          resid0 = (uint8_t)(cur0-pred0);
          code0 = (uint8_t)blk0->fold[resid0];
          out0 = out[off0b];
          recon0b = (uint8_t)(pred0+(blk0->unfold[code0]));
          recon0 = (uint8_t)(recon0b+out0-cur0);
          drift0 = out0-recon0;
          if( drift0<-16||drift0>16 ) {
            code0 = (uint8_t)blk0->fold_hi[resid0];
          } else {
            cur0 = recon0b;
            out[off0b] = recon0;
          }
          blk0->freq[blk0->ctx].encode_symbol(blk0->ctx_pair, code0);
          __alt_p2_model((AltP2Block*)blk0, cur0, code0, cur0-pred0);
          rec0 = blk0->cursor[0];
          l7a = rec0[-1].dupright;
          l4a = rec0[-1].dleft;
          l5a = rec0[-1].dup;
          ctx_bias[0] += 32*rec0[-1].dupleft;
          ctx_bias[1] += 32*l7a;
          ctx_bias[2] += 32*l4a;
          ctx_bias[3] += 32*l5a;
          if( xf0 )
            seed1 = 16*out[plane_desc[1].src_plane];
          else
            seed1 = 0;
          blk1 = (AltP2Block*)(plane[1]);
          plane[1]->cursor[0]->dval = seed1;
          off1 = plane_desc[2].src_plane;
          cur1 = out[plane_desc[2].src_plane];
          pred1 = __alt_p2_context((AltP2Block*)blk1, (AltP2Block*)plane[0], (AltP2Block*)plane[2]);
          resid1 = (uint8_t)(cur1-pred1);
          code1 = blk1->fold[resid1];
          out1 = out[off1];
          recon1b = (uint8_t)(pred1+blk1->unfold[code1]);
          recon1 = (uint8_t)(recon1b+out1-cur1);
          drift1 = out1-recon1;
          if( drift1<-16||drift1>16 ) {
            code1 = blk1->fold_hi[resid1];
          } else {
            cur1 = recon1b;
            out[off1] = recon1;
          }
          blk1->freq[blk1->ctx].encode_symbol(blk1->ctx_pair, code1);
          __alt_p2_model((AltP2Block*)blk1, cur1, code1, cur1-pred1);
          rec1 = blk1->cursor[0];
          l7b = rec1[-1].dupright;
          l4b = rec1[-1].dleft;
          l5b = rec1[-1].dup;
          ctx_bias[0] += 32*rec1[-1].dupleft;
          ctx_bias[1] += 32*l7b;
          ctx_bias[2] += 32*l4b;
          ctx_bias[3] += 32*l5b;
          if( np )
            seed2 = (plane_desc[plane_desc[3].src_plane+1].weight1*out[plane_desc[2].src_plane]+plane_desc[plane_desc[3].src_plane+1].weight0*out[plane_desc[1].src_plane])>>3;
          else
            seed2 = 0;
          blk2 = (AltP2Block*)(plane[2]);
          plane[2]->cursor[0]->dval = seed2;
          off2 = plane_desc[3].src_plane;
          cur2 = out[plane_desc[3].src_plane];
          pred2 = __alt_p2_context((AltP2Block*)blk2, (AltP2Block*)plane[0], (AltP2Block*)plane[1]);
          resid2 = (uint8_t)(cur2-pred2);
          code2 = blk2->fold[resid2];
          out2 = out[off2];
          code2b = (uint8_t)(pred2+blk2->unfold[code2]);
          recon2 = (uint8_t)(code2b+out2-cur2);
          drift2 = out2-recon2;
          if( drift2<-16||drift2>16 ) {
            code2 = blk2->fold_hi[resid2];
          } else {
            cur2 = code2b;
            out[off2] = recon2;
          }
          blk2->freq[blk2->ctx].encode_symbol(blk2->ctx_pair, code2);
          __alt_p2_model((AltP2Block*)blk2, cur2, code2, cur2-pred2);
          rec2 = blk2->cursor[0];
          nplanes = plane_count;
          at0 = ctx_bias[0]+32*rec2[-1].dupleft;
          seed0 = ctx_bias[1]+32*rec2[-1].dupright;
          off0 = ctx_bias[2]+32*rec2[-1].dleft;
          want0 = ctx_bias[3]+32*rec2[-1].dup;
          if( plane_count>=4 ) {
            ctx_bias[0] += 32*rec2[-1].dupleft;
            ctx_bias[1] = seed0;
            ctx_bias[2] = off0;
            ctx_bias[3] = want0;
            if( y )
              l7c = (plane_desc[plane_desc[4].src_plane+1].weight2*out[2]+plane_desc[plane_desc[4].src_plane+1].weight1*out[1]+plane_desc[plane_desc[4].src_plane+1].weight0**out)>>3;
            else
              l7c = 0;
            l4c = (AltP2Block*)(plane[3]);
            plane[3]->cursor[0]->dval = l7c;
            alpha_src = plane_desc[4].src_plane;
            l5c = out[plane_desc[4].src_plane];
            x = __alt_p2_context((AltP2Block*)l4c, (AltP2Block*)plane[2], (AltP2Block*)plane[0]);
            w = (uint8_t)(l5c-x);
            seed3 = l4c->fold[w];
            blk3 = out[alpha_src];
            cur3 = (uint8_t)(x+l4c->unfold[seed3]+blk3-l5c);
            drift3 = blk3-cur3;
            if( drift3<-16||drift3>16 ) {
              seed3 = l4c->fold_hi[w];
            } else {
              l5c = (uint8_t)(x+l4c->unfold[seed3]);
              out[alpha_src] = cur3;
            }
            l4c->freq[l4c->ctx].encode_symbol(l4c->ctx_pair, seed3);
            __alt_p2_model((AltP2Block*)l4c, l5c, seed3, l5c-x);
            rec3 = l4c->cursor[0];
            nplanes = plane_count;
            at0 = ctx_bias[0]+32*rec3[-1].dupleft;
            seed0 = ctx_bias[1]+32*rec3[-1].dupright;
            off0 = ctx_bias[2]+32*rec3[-1].dleft;
            want0 = ctx_bias[3]+32*rec3[-1].dup;
          }
          out += nplanes;
        }
        src1 = xf4;
        ctx_bias[0] = at0;
        ctx_bias[1] = seed0;
        ctx_bias[2] = off0;
        ctx_bias[3] = want0;
      }
      ++src1;
    } while( src1<(uint32_t)height );
  }
  __rc_end_encode();
  __builtin_memcpy(bmf_p2_coef, saved_p2_coef, sizeof saved_p2_coef);
  __builtin_memcpy(bmf_p2_rate[4], saved_p2_rate, sizeof saved_p2_rate);
  nplanes2 = plane_count;
  if( plane_count>0 ) {
    pl3 = 0;
    do {
      plane_p = (void**)plane[pl3];
      if( plane_p ) {
        ((AltP2Block*)plane_p)->alt_p2_free(1);
        nplanes2 = plane_count;
      }
      ++pl3;
    } while( pl3<nplanes2 );
  }
  return nplanes2;
}

void __model_plane(BmfImage* p_i, uint8_t* pixels, uint8_t* raw) {
  uint32_t alpha_n, hi;
  int32_t y1, bucket, f_b3, f_b0, f_b4, f_b1, f_b2, f_b5, alpha;
  uint16_t* word;
  ModelBlock* blk;
  PixRec* r4, * r0;
  uint8_t* buf;
  int16_t w2n, wt;
  uint32_t row_w;
  int32_t g, flags, lo, w3, w4, has3, has4, lvl, lvl2, live, s, y, x, x2, step;
  uint16_t w2;
  PixRec* row_cur3, * row_cur2, * row_cur1;
  uint32_t gi, n_syms, j, n_syms2, k;
  SymListBlock* blk1, * blk0;
  SymPair* group_ctr;
  SymList* lists1, * lists0;
  FreqRec* rec;
  PixRec* up1, * up2;
  void* ws;
  if( plane_alt_model ) {
    if( ::plane_predictor==1 ) {
      if( (p_i->depth&0x3F)==8 )
        __alt_model_p1_d8_encode(pixels, p_i->width, p_i->height, raw);
      else
        __alt_model_p1_encode((uint16_t*)p_i, pixels);
    } else if( ::plane_predictor==2 ) {
      if( (p_i->depth&0x3F)==8 )
        __alt_model_p2_d8_encode(pixels, p_i->width, p_i->height, raw);
      else
        __alt_model_p2_encode((BmfImage*)p_i, pixels);
    }
  } else {
    ws = bmf_new(sizeof(ModelBlock));
    if( ws )
      blk = __layout_workspace((ModelBlock*)ws, p_i->height, p_i->width, p_i->height, p_i->depth&0x3F);
    else
      blk = (ModelBlock*)(0);
    __rc_begin_encode();
    __reduce_alphabet(blk, 0, pixels);
    bucket = 0;
    g = 0;
    do {
      flags = ctx_group_flags[g];
      blk->ctx_state[flags] = g;
      lo = 0;
      f_b2 = flags&4;
      f_b1 = flags&2;
      f_b4 = flags&0x10;
      f_b0 = flags&1;
      f_b3 = flags&8;
      f_b5 = flags&0x20;
      do {
        hi = 0;
        do {
          blk->ctx_bucket[g+15*lo+75*hi] = bucket;
          alpha = blk->alphabet;
          rec = &blk->grid[bucket];
          w2n = 2;
          rec->w[0] = 2;
          w2 = 2;
          w3 = 2;
          w4 = 2;
          if( f_b2 ) {
            w3 = 4;
            w4 = 0;
          }
          if( f_b1 ) {
            w2 = w4+2;
            w4 = 0;
          }
          if( f_b4 ) {
            w2 = w3+w2;
            w3 = 0;
          }
          if( f_b0 ) {
            w2n = w4+2;
            w4 = 0;
            rec->w[4] = 0;
          } else {
            rec->w[4] = w4;
          }
          if( f_b3 ) {
            w2n += w3;
            w3 = 0;
            rec->w[3] = 0;
          } else {
            rec->w[3] = w3;
          }
          if( f_b5 ) {
            rec->w[1] = w2+w2n;
            w2 = 0;
            rec->w[2] = 0;
          } else {
            rec->w[2] = w2;
            rec->w[1] = w2n;
          }
          has3 = w3!=0;
          has4 = w4!=0;
          w2 = w2;
          if( w2 )
            w2 = 1;
          lvl = has4+has3+w2+2;
          if( lvl<=alpha ) {
            rec->b14 = lvl;
          } else {
            rec->b14 = has4+has3+w2+1;
            rec->w[0] = 0;
          }
          if( rec->w[lo]&&rec->w[hi]&&(lvl2 = rec->b14, lvl2<=alpha) ) {
            live = 1;
            wt = (uint8_t)(1<<((5-lvl2)&31));
            rec->b15 = wt;
            rec->w[6] = wt<<6;
            rec->w[lo] += wt;
            rec->w[hi] += rec->b15;
            rec->w[5] = rec->w[0]+rec->w[4]+rec->w[3]+rec->w[2]+rec->w[1];
          } else {
            live = 0;
          }
          bucket += live;
          ++hi;
        } while( hi<5 );
        ++lo;
      } while( (uint32_t)lo<5 );
      gi = 0;
      group_ctr = blk->group_ctr[g];
      do {
        group_ctr[gi].last = 0x2000;
        group_ctr[gi++].prev = 0x2000;
      } while( gi<0x10000 );
      ++g;
    } while( (uint32_t)g<0xF );
    buf = (uint8_t*)bmf_new(blk->alphabet);
    alpha_n = blk->alphabet;
    blk->alpha_map = buf;
    memset(buf, 1, alpha_n);
    blk->escape_list = &blk->escape;
    __init_symbol_list(&blk->escape, blk->alphabet, 1);
    blk->sel_cur = blk->sel;
    n_syms = blk->alphabet;
    blk1 = (SymListBlock*)bmf_new(SymListBlock::bytes(n_syms));
    if( blk1 ) {
      blk1->n = n_syms;
      lists1 = blk1->list;
      if( n_syms ) {
        for( j = 0; j<n_syms; ++j )
          lists1[j].ent = nullptr;
      }
    } else {
      lists1 = nullptr;
    }
    n_syms2 = blk->alphabet;
    blk->sel1_list = lists1;
    blk0 = (SymListBlock*)bmf_new(SymListBlock::bytes(n_syms2));
    if( blk0 ) {
      blk0->n = n_syms2;
      lists0 = blk0->list;
      if( n_syms2 ) {
        for( k = 0; k<n_syms2; ++k )
          lists0[k].ent = nullptr;
      }
    } else {
      lists0 = nullptr;
    }
    blk->sel0_list = lists0;
    if( !(blk->alphabet<=0) ) {
      s = 0;
      do {
        __init_symbol_list(&blk->sel1_list[s], 99, 0);
        __init_symbol_list(&blk->sel0_list[s++], 33, 0);
      } while( (uint32_t)s<blk->alphabet );
    }
    if( blk->height>0 ) {
      word = blk->sym_word;
      y = 0;
      do {
        y1 = y+1;
        blk->row_cur[5]->match[1] = blk->row_cur[5][-1].sym==0;
        blk->row_cur[5]->match[3] = blk->row_cur[6][-1].sym==0;
        r4 = blk->row_cur[4];
        row_cur3 = blk->row_cur[3];
        row_cur2 = blk->row_cur[2];
        row_cur1 = blk->row_cur[1];
        r0 = blk->row_cur[0];
        blk->row_cur[4] = row_cur3;
        blk->row_cur[3] = row_cur2;
        blk->row_cur[2] = row_cur1;
        blk->row_cur[1] = r0;
        blk->row_cur[0] = r4;
        r4 += 7;
        blk->row_cur[5] = r4;
        r0 += 7;
        blk->row_cur[6] = r0;
        blk->row_cur[7] = row_cur1+7;
        blk->row_cur[8] = row_cur2+7;
        blk->row_cur[9] = row_cur3+7;
        {
          uint8_t zero = r0[1].sym==0;
          r4->match[2] = zero;
          blk->row_cur[5][-1].match[4] = zero;
          blk->row_cur[5][-2].match[5] = zero;
          zero = blk->row_cur[6][2].sym==0;
          blk->row_cur[5]->match[4] = zero;
          blk->row_cur[5][-1].match[5] = zero;
        }
        blk->row_cur[5]->match[5] = blk->row_cur[6][3].sym==0;
        up1 = blk->row_cur[6];
        up2 = blk->row_cur[7];
        ++blk->row_cur[5];
        ++up1;
        blk->row_cur[6] = up1;
        ++blk->row_cur[8];
        ++up2;
        blk->row_cur[7] = up2;
        ++blk->row_cur[9];
        blk->grad[0] = up1[3].match[0]+up1[2].match[0]+up1[1].match[0]+up1[0].match[0]+up1[4].match[0]-5;
        blk->grad[3] = 0;
        blk->grad[2] = 0;
        y = y1;
        row_w = blk->width;
        blk->grad[1] = up2[3].match[0]+up2[2].match[0]+up2[1].match[0]+up2[0].match[0]+up2[4].match[0]-5;
        if( row_w>0 ) {
          x = 0;
          do {
            ++x;
            blk->row_cur[5][x-1].sym = word[x-1];
            row_w = blk->width;
          } while( (uint32_t)x<blk->width );
          word += x;
        }
        if( row_w>0 ) {
          x2 = 0;
          do {
            step = ((ModelBlock*)blk)->code_pixel(x2);
            blk->init_tables();
            x2 += step;
          } while( (uint32_t)x2<blk->width );
          y = y1;
        }
      } while( (uint32_t)y<*(uint32_t*)&blk->height );
    }
    __rc_end_encode();
    __free_workspace((ModelBlock*)blk, 1);
  }
}

void __model_planes(uint8_t* img, uint8_t* pixels, int32_t plane, int8_t unread_flag) {
  BmfImage hdr;
  uint8_t* aligned;
  uint8_t* scratch;
  int32_t ofs;
  plane_predictor = plane_desc[plane+1].flags&3;
  plane_alt_model = (uint8_t)(plane_desc[plane+1].flags&4)>>2;
  __colour_transform(img, pixels, plane, unread_flag);
  scratch = ::hist_scratch;
  aligned = (uint8_t*)((uintptr_t)(::hist_scratch+15)&~(uintptr_t)15);
  memset(scratch, 0, 15);
  bmf_zero16(&scratch[1008]);
  ofs = 1008;
  do {
    bmf_zero16((aligned+ofs-16));
    bmf_zero16((aligned+ofs-32));
    bmf_zero16((aligned+ofs-48));
    bmf_zero16((aligned+ofs-64));
    bmf_zero16((aligned+ofs-80));
    bmf_zero16((aligned+ofs-96));
    bmf_zero16((aligned+ofs-112));
    ofs -= 112;
  } while( ofs );
  {
    hdr = *(BmfImage*)img;
    hdr.depth = 72;
    if( plane_predictor==1&&!plane_alt_model )
      __predict_med(pixels, ((const BmfImage*)img)->width, ((const BmfImage*)img)->height);
    __model_plane(&hdr, pixels, pixels);
  }
}

void __transform_planes(BmfImage* p_i, int8_t unread_flag) {
  uint8_t* arc, * hdr, * tmp, * dst;
  int32_t k, plane, predictor, alt, n, stride, ofs, i;
  uint8_t* src_pixels;
  memset(hist_scratch, 0, 4096);
  arc = (uint8_t*)::coded_buf;
  hdr = (uint8_t*)::coded_buf+16;
  *(BmfImage*)hdr = *p_i;
  src_pixels = (uint8_t*)((uint16_t*)p_i->pixels);
  memcpy(arc+32, p_i->pixels, p_i->data_size);
  tmp = (uint8_t*)bmf_new(p_i->width*p_i->height);
  if( plane_count>0 ) {
    k = 0;
    do {
      ++k;
      plane = plane_desc[k].src_plane;
      predictor = plane_desc[plane+1].flags&3;
      ::plane_predictor = predictor;
      alt = (uint8_t)(plane_desc[plane+1].flags&4)>>2;
      plane_alt_model = alt;
      if( ((plane_desc[plane+1].flags&8)!=0||predictor)&&!alt ) {
        __colour_transform(hdr, tmp, plane, 0);
        if( ::plane_predictor!=2 ) {
          if( ::plane_predictor==1 ) {
            __predict_med(tmp, p_i->width, p_i->height);
          }
        }
        n = p_i->width*p_i->height;
        stride = plane_count;
        if( plane_count==1 ) {
          memcpy(&p_i->pixels[plane], tmp, n);
        } else {
          dst = (uint8_t*)p_i+plane;
          ofs = 0;
          for( i = 0; i<n; ++i ) {
            dst[ofs+16] = tmp[i];
            ofs += stride;
          }
        }
      }
    } while( k<plane_count );
  }
  free(tmp);
  {
    __model_plane((BmfImage*)p_i, src_pixels, src_pixels);
  }
}

static uint32_t __bytes_left(FILE* fp) {
  long here = ftell(fp);
  if( here<0||fseek(fp, 0, SEEK_END)!=0 )
    return 0;
  long end = ftell(fp);
  if( end<0||fseek(fp, here, SEEK_SET)!=0 )
    return 0;
  return (uint32_t)(end-here);
}

static uint32_t __packer_word() {
  if( out_cursor+4>coded_buf+coded_size )
    __exit_402E40(4);
  uint32_t w = *(uint32_t*)out_cursor;
  out_cursor += 4;
  return w;
}

uint8_t*__expand_image(BmfArc* arc_in, int32_t want_pal, void** p_coded_buf) {
  struct alignas(16) ExpandImageFrame {
    uint8_t* Block;
    union {
      BmfImage img;
      uint32_t mask;
      struct {
        uint32_t s0;
        int32_t s4;
        int32_t s8;
        uint32_t s12;
      };
    };
    int32_t nplanes_s;
    void* row_step;
    BmfArc* arc_f;
    BmfImage* p_i_2;
    int32_t depth_f;
    uint32_t pal_len;
    uint16_t hdr_words[5];
    uint8_t depth_b;
    int8_t flags_b;
    uint32_t data_len;
    uint8_t hdr[8];
    uint32_t magic_word;
    uint8_t _pad0[32];
  } __frame;
  BmfArc* arc;
  FILE* fp1, * fp;
  BmfImage* img_at;
  int8_t hdr_flags;
  uint8_t bpp, has_coded, dc_v;
  uint32_t near_lossless, pred_s, w12_v, w4_v, w8_v;
  uint8_t* pal_at, * copy, * srcp, * dst;
  int32_t hdr_word, pl, nrefs, plane_i, plane, pl2, plane2, pred, i, n_pix2, pix_at, nplanes_c, i2, n_planes, last_row, at, y, pl_i, left;
  uint16_t img_h, w16;
  uint8_t major_v, minor_v;
  int32_t n_pix;
  uint32_t magic, pad_len, * blk, pal_bytes, want2, desc, desc_flags, want, got, word12, word8, word4, worddc, word6, word4b;
  uint8_t* plane_buf;
  arc = arc_in;
  if( p_coded_buf )
    *p_coded_buf = nullptr;
  fp1 = ((BmfArc*)arc_in)->fp;
  if( !fp1 )
    return nullptr;
  while( 1 ) {
    if( fread(&__frame.magic_word, 4u, 1u, fp1)!=1 ) {
      fp = arc->fp;
      if( feof(fp) )
        return nullptr;
      {
        fclose(fp);
        arc->fp = 0;
        return nullptr;
      }
    }
    magic = __frame.magic_word;
    if( (uint16_t)__frame.magic_word!=0x9081 )
      break;
    major_v = (uint8_t)(__frame.magic_word>>16);
    minor_v = (uint8_t)(__frame.magic_word>>24);
    plane_desc[0].weight0 = ((major_v<<8)-12288)|(minor_v-48);
    if( plane_desc[0].weight0!=512||fread(__frame.hdr, 8u, 1u, arc->fp)!=1 )
      break;
    fseek(arc->fp, (*(uint32_t*)&__frame.hdr[4]), 1);
    fp1 = arc->fp;
  }
  if( (uint16_t)magic!=0x8A81||(major_v = (uint8_t)(magic>>16), minor_v = (uint8_t)(magic>>24), plane_desc[0].weight0 = ((major_v<<8)-12288)|(minor_v-48), plane_desc[0].weight0!=512)||fread(__frame.hdr_words, 0x10u, 1u, arc->fp)!=1 ) {
    fp = arc->fp;
    fclose(fp);
    arc->fp = 0;
    return nullptr;
  }
  ++arc->images;
  if( __frame.flags_b<0 ) {
    fread(__frame.hdr, 8u, 1u, arc->fp);
    if( p_coded_buf ) {
      hdr_word = (*(int32_t*)&__frame.hdr[0]);
      if( (*(uint32_t*)&__frame.hdr[4])>__bytes_left(arc->fp) ) {
        fclose(arc->fp);
        arc->fp = 0;
        return nullptr;
      }
      pad_len = ((*(uint32_t*)&__frame.hdr[4])+((*(uint32_t*)&__frame.hdr[4])==0)+3)&0xFFFFFFFC;
      blk = (uint32_t*)bmf_new(pad_len+8);
      *blk = hdr_word;
      blk[1] = pad_len;
      blk[pad_len/4+1] = 0;
      *p_coded_buf = blk;
      fread(blk+2, (*(uint32_t*)&__frame.hdr[4]), 1u, arc->fp);
    } else {
      fseek(arc->fp, (*(uint32_t*)&__frame.hdr[4]), 1);
    }
  }
  if( __frame.data_len>__bytes_left(arc->fp) ) {
    fclose(arc->fp);
    arc->fp = 0;
    return nullptr;
  }
  pal_bytes = 3<<(__frame.depth_b&31);
  if( (__frame.depth_b&0x80)==0 )
    pal_bytes = __frame.depth_b&0x80;
  __frame.pal_len = pal_bytes;
  if( want_pal ) {
    fseek(arc->fp, __frame.pal_len+__frame.data_len, 1);
    return nullptr;
  }
  if( !__frame.hdr_words[0]||!__frame.hdr_words[1] ) {
    fclose(arc->fp);
    arc->fp = 0;
    return nullptr;
  }
  img_at = (BmfImage*)((uint8_t*)__alloc_image(__frame.hdr_words[0], __frame.hdr_words[1], __frame.depth_b&0x3F, (uint8_t)(__frame.depth_b&0x80)>>7, 1));
  __frame.depth_f = __frame.depth_b;
  img_at->depth = __frame.depth_b;
  has_coded = p_coded_buf&&*p_coded_buf;
  hdr_flags = __frame.flags_b;
  img_at->flags |= __frame.flags_b&2|(has_coded<<7);
  ::plane_count = ((__frame.depth_f&0x3Fu)+7)>>3;
  if( ::plane_count<1||::plane_count>4 ) {
    fclose(arc->fp);
    arc->fp = 0;
    return nullptr;
  }
  if( (hdr_flags&0x20)==0 ) {
    if( __frame.data_len>img_at->data_size ) {
      fclose(arc->fp);
      arc->fp = 0;
      return nullptr;
    }
    want = __frame.data_len;
    if( fread(img_at->pixels, 1u, __frame.data_len, arc->fp)!=want ) {
      fclose(arc->fp);
      arc->fp = 0;
      return nullptr;
    }
    goto LABEL_109;
  }
  near_lossless_q = 0;
  alphabet_reduced = 0;
  if( (hdr_flags&4)==0 ) {
    printf("\nwritten in fast mode; this build only decodes -S streams\n");
    exit(3);
  }
  desc_slow_mode = 1;
  coded_size = __frame.data_len;
  ::coded_buf = (uint8_t*)bmf_new(__frame.data_len);
  out_cursor = ::coded_buf;
  packer_free_bits = 0;
  packer_acc = 0;
  ::packer_word = (uint32_t*)::coded_buf;
  hist_scratch = ::coded_buf+coded_size-4096;
  want2 = __frame.data_len;
  if( fread(::coded_buf, 1u, __frame.data_len, arc->fp)!=want2 ) {
    fclose(arc->fp);
    arc->fp = 0;
    return nullptr;
  }
  bpp = img_at->depth;
  if( (bpp&0x3Fu)<=4||(__frame.flags_b&0x10)==0 ) {
    ::plane_predictor = 0;
    plane_alt_model = 0;
    __unmodel_plane(want2, (uint16_t*)img_at, img_at->pixels);
    goto LABEL_106;
  }
  if( ::plane_count==1 ) {
    if( (bpp&0x40)==0 )
      goto LABEL_42;
  } else if( ::plane_count<=2 ) {
    goto LABEL_42;
  }
  packer_free_bits -= 4;
  if( packer_free_bits<0 ) {
    word4b = __packer_word();
    want2 = word4b>>(-packer_free_bits&31);
    near_lossless = packer_acc|(word4b<<((packer_free_bits+4)&31))&0xF;
    packer_acc = want2;
    packer_free_bits += 32;
  } else {
    near_lossless = packer_acc&0xF;
    packer_acc = packer_acc>>4;
  }
  if( near_lossless ) {
    printf("\nnear-lossless stream (E=%d); this build only decodes E=0\n", near_lossless);
    exit(3);
  }
  near_lossless_q = 0;
LABEL_42:
  if( ::plane_count>0 ) {
    __frame.mask = 255;
    __frame.p_i_2 = img_at;
    __frame.arc_f = arc;
    pl = 0;
    do {
      packer_free_bits -= 6;
      if( packer_free_bits<0 ) {
        word6 = __packer_word();
        desc = packer_acc|(word6<<((packer_free_bits+6)&31))&0x3F;
        packer_acc = word6>>(-packer_free_bits&31);
        packer_free_bits += 32;
      } else {
        desc = packer_acc&0x3F;
        packer_acc = packer_acc>>6;
      }
      desc_flags = desc>>2;
      nrefs = desc&3;
      plane_desc[pl+1].flags = desc_flags;
      plane_desc[pl+1].nrefs = nrefs;
      plane_desc[nrefs+1].src_plane = pl;
      if( (plane_desc[pl+1].flags&8)!=0 ) {
        packer_free_bits -= 8;
        if( packer_free_bits<0 ) {
          worddc = __packer_word();
          dc_v = packer_acc|__frame.mask&(worddc<<((packer_free_bits+8)&31));
          packer_acc = worddc>>(-packer_free_bits&31);
          packer_free_bits += 32;
        } else {
          dc_v = packer_acc&(uint8_t)__frame.mask;
          packer_acc = packer_acc>>8;
        }
        plane_desc[pl+1].dc = dc_v;
        if( nrefs>1 ) {
          packer_free_bits -= 8;
          if( packer_free_bits<0 ) {
            word4 = __packer_word();
            w4_v = packer_acc|__frame.mask&(word4<<((packer_free_bits+8)&31));
            packer_acc = word4>>(-packer_free_bits&31);
            packer_free_bits += 32;
          } else {
            w4_v = packer_acc&__frame.mask;
            packer_acc = packer_acc>>8;
          }
          plane_desc[pl+1].weight0 = w4_v-64;
          packer_free_bits -= 8;
          if( packer_free_bits<0 ) {
            word8 = __packer_word();
            w8_v = packer_acc|__frame.mask&(word8<<((packer_free_bits+8)&31));
            packer_acc = word8>>(-packer_free_bits&31);
            packer_free_bits += 32;
          } else {
            w8_v = packer_acc&__frame.mask;
            packer_acc = packer_acc>>8;
          }
          plane_desc[pl+1].weight1 = w8_v-64;
          if( nrefs>2 ) {
            packer_free_bits -= 8;
            if( packer_free_bits<0 ) {
              word12 = __packer_word();
              w12_v = packer_acc|__frame.mask&(word12<<((packer_free_bits+8)&31));
              packer_acc = word12>>(-packer_free_bits&31);
              packer_free_bits += 32;
            } else {
              w12_v = packer_acc&__frame.mask;
              packer_acc = packer_acc>>8;
            }
            plane_desc[pl+1].weight2 = w12_v-64;
          }
        }
      }
      ++pl;
    } while( pl<::plane_count );
    img_at = __frame.p_i_2;
    arc = __frame.arc_f;
  }
  plane_buf = (uint8_t*)bmf_new(img_at->width*img_at->height);
  if( (__frame.flags_b&8)!=0 ) {
    __frame.img = *img_at;
    __frame.img.depth = 72;
    if( ::plane_count>0 ) {
      __frame.arc_f = arc;
      plane_i = 0;
      do {
        plane = plane_desc[plane_i+1].src_plane;
        ::plane_predictor = plane_desc[plane+1].flags&3;
        plane_alt_model = (uint8_t)(plane_desc[plane+1].flags&4)>>2;
        __unmodel_plane(plane_i, (uint16_t*)&__frame.img, plane_buf);
        if( ::plane_predictor ) {
          if( ::plane_predictor==1 ) {
            if( !plane_alt_model )
              __unpredict_med(plane_buf, img_at->width, img_at->height);
          }
        } else {
          __expand_predictor_mode0(plane_buf, img_at->width, img_at->height);
        }
        __interleave_plane((uint8_t*)img_at, plane_buf, plane, 0);
        ++plane_i;
      } while( plane_i<::plane_count );
LABEL_104:
      arc = __frame.arc_f;
    }
  } else {
    ::plane_predictor = plane_desc[1].flags&3;
    plane_alt_model = (uint8_t)(plane_desc[1].flags&4)>>2;
    {
      __unmodel_plane(63, (uint16_t*)img_at, img_at->pixels);
      if( plane_alt_model )
        goto LABEL_105;
    }
    if( ::plane_count>0 ) {
      __frame.arc_f = arc;
      pl2 = 0;
      do {
        ++pl2;
        plane2 = plane_desc[pl2].src_plane;
        pred = plane_desc[plane2+1].flags&3;
        ::plane_predictor = pred;
        if( (plane_desc[plane2+1].flags&8)!=0||pred ) {
          i = img_at->width;
          __frame.row_step = &((uint8_t*)img_at)[plane2+16];
          n_pix = i*img_at->height;
          __frame.nplanes_s = ::plane_count;
          __frame.s12 = n_pix;
          if( ::plane_count==1 ) {
            memcpy(plane_buf, (uint8_t*)__frame.row_step, __frame.s12);
            pred_s = ::plane_predictor;
          } else {
            __frame.Block = &((uint8_t*)img_at)[plane2];
            __frame.s0 = pred;
            __frame.s4 = plane2;
            __frame.s8 = pl2;
            n_pix2 = __frame.s12;
            __frame.p_i_2 = img_at;
            pix_at = 0;
            nplanes_c = __frame.nplanes_s;
            i2 = 0;
            do {
              plane_buf[pix_at] = __frame.Block[i2+16];
              i2 += nplanes_c;
              ++pix_at;
            } while( pix_at<n_pix2 );
            pred_s = __frame.s0;
            plane2 = __frame.s4;
            pl2 = __frame.s8;
            img_at = __frame.p_i_2;
          }
          if( pred_s ) {
            if( pred_s==1 )
              __unpredict_med(plane_buf, img_at->width, img_at->height);
          } else {
            __expand_predictor_mode0(plane_buf, img_at->width, img_at->height);
          }
          __interleave_plane((uint8_t*)img_at, plane_buf, plane2, 0);
        }
      } while( pl2<::plane_count );
      goto LABEL_104;
    }
  }
LABEL_105:
  free(plane_buf);
LABEL_106:
  if( ::coded_buf+__frame.data_len!=out_cursor ) {
    fclose(arc->fp);
    arc->fp = 0;
    return nullptr;
  }
  free(::coded_buf);
  __frame.depth_f = __frame.depth_b;
LABEL_109:
  if( (__frame.depth_f&0x80)!=0 ) {
    pal_at = (img_at->depth&0x80) ? &((uint8_t*)img_at)[img_at->data_size+16] : nullptr;
    got = fread(pal_at, 1u, __frame.pal_len, arc->fp);
    if( got!=__frame.pal_len ) {
      fclose(arc->fp);
      arc->fp = 0;
      return nullptr;
    }
  }
  if( (img_at->flags&2)!=0 ) {
    if( (uint64_t)img_at->width*img_at->height*(uint32_t)::plane_count>(uint64_t)img_at->data_size ) {
      fclose(arc->fp);
      arc->fp = 0;
      return nullptr;
    }
    copy = (uint8_t*)bmf_new(img_at->data_size);
    n_planes = ::plane_count;
    last_row = ::plane_count*(img_at->height-1);
    memcpy(copy, img_at->pixels, img_at->data_size);
    img_h = img_at->height;
    if( img_h ) {
      srcp = copy;
      at = 0;
      y = 0;
      do {
        pl_i = img_at->width;
        __frame.p_i_2 = img_at;
        dst = &img_at->pixels[at];
        do {
          left = n_planes;
          do {
            *dst++ = *srcp++;
            --left;
          } while( left );
          dst += last_row;
          --pl_i;
        } while( pl_i );
        img_at = __frame.p_i_2;
        img_h = __frame.p_i_2->height;
        at += n_planes;
        ++y;
      } while( y<img_h );
    }
    w16 = img_at->width;
    img_at->width = img_h;
    img_at->flags ^= 2u;
    img_at->height = w16;
    *(uint16_t*)&img_at->stride = img_h*n_planes;
    free(copy);
  }
  return (uint8_t*)img_at;
}

uint32_t __search_filter(BmfImage* img, int8_t mode) {
  struct alignas(16) SearchFilterFrame {
    uint8_t* base;
    uint8_t marks[16];
    int32_t rows[2];
    int32_t dims[2];
    union {
      PlaneDesc saved[4];
      struct {
        int32_t plane_i;
        uint32_t costs[4];
        int32_t pi;
        void* tile_buf;
        int32_t flag8;
        int32_t best_flags;
        uint8_t _pad0[28];
      };
    };
    void* tile_src;
    int32_t n_hard;
    int32_t bits_total;
    int32_t n_flagged;
    int32_t best_bits;
    BmfImage* p_i_2;
    BmfImage* tile_img;
    uint8_t _pad1[36];
  } __frame;
  uint8_t* base2;
  bool deep, deep2;
  int8_t f4;
  uint16_t w_a, w_d;
  BmfImage* img_c;
  uint8_t f0, f1, f2, f3;
  uint8_t* srcp, * dstp, * p2, * tile_copy, * tile_a, * p4, * p5, * tile_b, * tile_c, * p12, * p13, * tile_d;
  int32_t y, hard, bits_a, bits_b, cost_f13;
  int32_t p3, p6, p7, p10, p11, p14;
  int16_t w_b, w_e;
  int32_t flags_s, desc_k;
  int32_t tile_w, tile_h, pl, nplanes, y0, dx, off_y, row_bytes, best_cost, bits_f5, cost_f5, f5, pred, c0, c1, c2, c3, cand, nplanes_c, pl_a, pi0, pl_b, pi1, bits2, plane, pi3, bits_e, nplanes_b, pl_c, pi4, pl_d, bits_c, img_h1, y1, off1, x1, bits_d, pk, pl_k, bits, img_h3, y3, off3, pl_i, rows_x_planes, y2, off2, x2, pl2, bits_f8, cost_f8, f8, pl3, bits_f13, f13, pl4, cost_f14, f14, pl0, bits_f6, cost_f6, f6, pl1, bits_f0;
  uint16_t img_h3b, img_h2, img_h1b;
  uint16_t w16, w_c;
  uint8_t* p8, * p9;
  img_c = (BmfImage*)(img);
  tile_w = img->width;
  tile_h = img->height;
  if( tile_w<4||tile_h<3 ) {
    if( ::plane_count>0 ) {
      pl = 0;
      do {
        plane_desc[pl+1].flags = 0;
        plane_desc[pl+1].src_plane = pl;
        plane_desc[pl+1].nrefs = pl;
        ++pl;
      } while( pl<::plane_count );
    }
    return 0;
  }
  __choose_plane_coding((BmfImage*)img_c, tile_h, mode);
  __frame.tile_img = (BmfImage*)__alloc_image(tile_w, tile_h, img_c->depth&0x3F, 0, 0);
  coded_size = __frame.tile_img->data_size+0x20000;
  coded_buf = (uint8_t*)bmf_new(coded_size);
  out_cursor = coded_buf;
  packer_free_bits = 0;
  packer_acc = 0;
  packer_word = (uint32_t*)coded_buf;
  __frame.dims[0] = tile_h*tile_w;
  hist_scratch = coded_buf+coded_size-4096;
  __frame.tile_buf = bmf_new(tile_h*tile_w);
  nplanes = ::plane_count;
  y0 = (img_c->height-tile_h)>>1;
  dx = img_c->width-tile_w;
  off_y = y0*img_c->stride;
  __frame.rows[1] = y0;
  srcp = (uint8_t*)img_c+ ::plane_count*(dx>>1)+off_y+16;
  __frame.tile_src = __frame.tile_img->pixels;
  if( y0<tile_h+y0 ) {
    row_bytes = (uint16_t)__frame.tile_img->stride;
    __frame.dims[1] = tile_h;
    y = __frame.rows[1];
    __frame.p_i_2 = (BmfImage*)(img_c);
    dstp = __frame.tile_img->pixels;
    do {
      memcpy(dstp, srcp, row_bytes);
      row_bytes = (uint16_t)__frame.tile_img->stride;
      dstp += row_bytes;
      srcp += __frame.p_i_2->stride;
      ++y;
    } while( y<__frame.dims[1]+((__frame.p_i_2->height-__frame.dims[1])>>1) );
    img_c = (BmfImage*)(__frame.p_i_2);
    nplanes = ::plane_count;
  }
  __frame.n_hard = 0;
  __frame.bits_total = 0;
  __frame.n_flagged = 0;
  __frame.best_bits = 0;
  if( nplanes>0 ) {
    __frame.p_i_2 = (BmfImage*)(img_c);
    hard = __frame.n_hard;
    __frame.best_flags = 0;
    __frame.pi = 0;
    __frame.dims[0] *= 8;
    while( 1 ) {
      __frame.dims[1] = plane_desc[__frame.pi+1].src_plane;
      __frame.plane_i = __frame.dims[1];
      if( hard ) {
        best_cost = 0x7FFFFFFF;
      } else {
        pl1 = __frame.dims[1];
        plane_desc[__frame.dims[1]+1].flags = 0;
        __model_planes((uint8_t*)__frame.tile_img, (uint8_t*)__frame.tile_buf, pl1, 0);
        bits_f0 = 8*(out_cursor-coded_buf);
        best_cost = plane_desc[0].desc_word-packer_free_bits+bits_f0+32;
        deep = 0;
        *(uint32_t*)packer_word = packer_acc;
        if( !deep )
          best_cost = bits_f0;
        packer_free_bits = 0;
        packer_acc = 0;
        out_cursor = coded_buf;
        packer_word = (uint32_t*)coded_buf;
        flags_s = __frame.best_flags;
        hist_scratch = coded_buf+coded_size-4096;
        if( best_cost==0x7FFFFFFF )
          best_cost = 0x7FFFFFFF;
        else
          flags_s = 0;
        __frame.best_flags = flags_s;
      }
      plane_desc[__frame.plane_i+1].flags = 5;
      __model_planes((uint8_t*)__frame.tile_img, (uint8_t*)__frame.tile_buf, __frame.dims[1], 0);
      bits_f5 = 8*(out_cursor-coded_buf);
      cost_f5 = plane_desc[0].desc_word-packer_free_bits+bits_f5+32;
      deep = 0;
      *(uint32_t*)packer_word = packer_acc;
      out_cursor = coded_buf;
      packer_word = (uint32_t*)coded_buf;
      if( !deep )
        cost_f5 = bits_f5;
      packer_free_bits = 0;
      packer_acc = 0;
      hist_scratch = coded_buf+coded_size-4096;
      f5 = __frame.best_flags;
      if( cost_f5<best_cost ) {
        best_cost = cost_f5;
        f5 = 5;
      }
      __frame.best_flags = f5;
      if( cost_f5<best_cost+(best_cost>>5)||hard ) {
        pl0 = __frame.dims[1];
        plane_desc[__frame.plane_i+1].flags = 6;
        __model_planes((uint8_t*)__frame.tile_img, (uint8_t*)__frame.tile_buf, pl0, 0);
        bits_f6 = 8*(out_cursor-coded_buf);
        cost_f6 = plane_desc[0].desc_word-packer_free_bits+bits_f6+32;
        deep = 0;
        *(uint32_t*)packer_word = packer_acc;
        out_cursor = coded_buf;
        packer_word = (uint32_t*)coded_buf;
        if( !deep )
          cost_f6 = bits_f6;
        packer_free_bits = 0;
        packer_acc = 0;
        f6 = __frame.best_flags;
        hist_scratch = coded_buf+coded_size-4096;
        if( cost_f6<best_cost ) {
          best_cost = cost_f6;
          f6 = 6;
        }
        __frame.best_flags = f6;
        if( __frame.pi ) {
          {
LABEL_191:
            pl2 = __frame.dims[1];
            plane_desc[__frame.plane_i+1].flags = 8;
            __model_planes((uint8_t*)__frame.tile_img, (uint8_t*)__frame.tile_buf, pl2, 0);
            bits_f8 = 8*(out_cursor-coded_buf);
            cost_f8 = plane_desc[0].desc_word-packer_free_bits+bits_f8+32;
            deep = 0;
            *(uint32_t*)packer_word = packer_acc;
            out_cursor = coded_buf;
            packer_word = (uint32_t*)coded_buf;
            if( !deep )
              cost_f8 = bits_f8;
            packer_free_bits = 0;
            packer_acc = 0;
            f8 = __frame.best_flags;
            hist_scratch = coded_buf+coded_size-4096;
            if( cost_f8<best_cost ) {
              best_cost = cost_f8;
              f8 = 8;
            }
            __frame.best_flags = f8;
          }
          pl3 = __frame.dims[1];
          plane_desc[__frame.plane_i+1].flags = 13;
          __model_planes((uint8_t*)__frame.tile_img, (uint8_t*)__frame.tile_buf, pl3, 0);
          bits_f13 = 8*(out_cursor-coded_buf);
          cost_f13 = plane_desc[0].desc_word-packer_free_bits+bits_f13+32;
          deep = 0;
          *(uint32_t*)packer_word = packer_acc;
          out_cursor = coded_buf;
          packer_word = (uint32_t*)coded_buf;
          if( !deep )
            cost_f13 = bits_f13;
          __frame.rows[0] = cost_f13;
          packer_free_bits = 0;
          packer_acc = 0;
          f13 = __frame.best_flags;
          hist_scratch = coded_buf+coded_size-4096;
          if( cost_f13<best_cost ) {
            best_cost = (int32_t)cost_f13;
            f13 = 13;
          }
          __frame.best_flags = f13;
          pred = f13&3;
          if( pred==2||best_cost+(best_cost>>5)>__frame.rows[0] ) {
            pl4 = __frame.dims[1];
            plane_desc[__frame.plane_i+1].flags = 14;
            __model_planes((uint8_t*)__frame.tile_img, (uint8_t*)__frame.tile_buf, pl4, 0);
            cost_f14 = plane_desc[0].desc_word-packer_free_bits+8*(out_cursor-coded_buf)+32;
            cost_f14 = 8*(out_cursor-coded_buf);
            deep2 = cost_f14<best_cost;
            if( cost_f14<best_cost )
              best_cost = cost_f14;
            *(uint32_t*)packer_word = packer_acc;
            packer_free_bits = 0;
            packer_acc = 0;
            out_cursor = coded_buf;
            packer_word = (uint32_t*)coded_buf;
            hist_scratch = coded_buf+coded_size-4096;
            f14 = __frame.best_flags;
            if( deep2 )
              f14 = 14;
            __frame.best_flags = f14;
            if( deep2 )
              pred = 2;
          }
          goto LABEL_43;
        }
        pred = f6&3;
      } else {
        if( __frame.pi )
          goto LABEL_191;
        pred = (uint8_t)__frame.best_flags&3;
      }
LABEL_43: { __frame.flag8 = (uint8_t)__frame.best_flags&8; }
      __frame.best_bits += best_cost;
      c0 = pred==2;
      if( pred!=1 )
        pred = 0;
      c1 = __frame.plane_i;
      hard += c0;
      __frame.bits_total += pred;
      c2 = __frame.flag8;
      __frame.costs[__frame.dims[1]] = best_cost;
      c3 = __frame.pi;
      __frame.n_flagged += c2!=0;
      plane_desc[c1+1].flags = (uint8_t)__frame.best_flags;
      __frame.pi = c3+1;
      if( c3+1>=::plane_count ) {
        __frame.n_hard = hard;
        img_c = (BmfImage*)(__frame.p_i_2);
        break;
      }
    }
  }
  {
    p2 = (uint8_t*)bmf_new(__frame.tile_img->data_size);
    img_h1 = __frame.tile_img->height;
    __frame.rows[1] = ::plane_count;
    __frame.pi = ::plane_count*(img_h1-1);
    memcpy(p2, (uint8_t*)__frame.tile_src, __frame.tile_img->data_size);
    img_h1b = __frame.tile_img->height;
    if( img_h1b ) {
      p5 = p2;
      p3 = __frame.rows[1];
      __frame.p_i_2 = (BmfImage*)(img_c);
      y1 = 0;
      tile_a = (uint8_t*)__frame.tile_img;
      off1 = 0;
      do {
        x1 = ((const BmfImage*)tile_a)->width;
        p4 = &tile_a[off1+16];
        do {
          p6 = p3;
          do {
            *p4++ = *p5++;
            --p6;
          } while( p6 );
          p4 += __frame.pi;
          --x1;
        } while( x1 );
        tile_a = (uint8_t*)__frame.tile_img;
        img_h1b = __frame.tile_img->height;
        off1 += p3;
        ++y1;
      } while( y1<img_h1b );
      img_c = (BmfImage*)(__frame.p_i_2);
    }
    tile_b = (uint8_t*)__frame.tile_img;
    w_a = (__frame.tile_img)->width;
    w_b = img_h1b*LOWORD(__frame.rows[1]);
    (__frame.tile_img)->width = img_h1b;
    ((BmfImage*)tile_b)->height = w_a;
    ((BmfImage*)tile_b)->flags ^= 2u;
    *(uint16_t*)&((BmfImage*)tile_b)->stride = w_b;
    free(p2);
    bits_d = 0;
    if( ::plane_count>0 ) {
      __frame.p_i_2 = (BmfImage*)(img_c);
      pk = 0;
      while( 1 ) {
        pl_k = plane_desc[pk+1].src_plane;
        __frame.rows[0] = pl_k;
        __model_planes((uint8_t*)__frame.tile_img, (uint8_t*)__frame.tile_buf, pl_k, f4);
        bits = 8*(out_cursor-coded_buf);
        *(uint32_t*)packer_word = packer_acc;
        packer_free_bits = 0;
        bits_d += bits;
        packer_acc = 0;
        out_cursor = coded_buf;
        packer_word = (uint32_t*)coded_buf;
        hist_scratch = coded_buf+coded_size-4096;
        if( (uint32_t)(bits-(bits>>8))>__frame.costs[__frame.rows[0]] )
          break;
        if( ++pk>=::plane_count ) {
          img_c = (BmfImage*)(__frame.p_i_2);
          goto LABEL_172;
        }
      }
      img_c = (BmfImage*)(__frame.p_i_2);
      bits_d += __frame.best_bits+1;
    }
LABEL_172:
    if( bits_d+(bits_d>>12)>=__frame.best_bits ) {
      base2 = (uint8_t*)bmf_new(__frame.tile_img->data_size);
      rows_x_planes = ::plane_count*(__frame.tile_img->height-1);
      __frame.rows[1] = ::plane_count;
      memcpy(base2, (uint8_t*)__frame.tile_src, __frame.tile_img->data_size);
      img_h2 = __frame.tile_img->height;
      if( img_h2 ) {
        __frame.costs[0] = rows_x_planes;
        p11 = __frame.rows[1];
        __frame.p_i_2 = (BmfImage*)(img_c);
        y2 = 0;
        tile_c = (uint8_t*)__frame.tile_img;
        off2 = 0;
        p13 = base2;
        do {
          x2 = ((const BmfImage*)tile_c)->width;
          p12 = &tile_c[off2+16];
          do {
            p14 = p11;
            do {
              *p12++ = *p13++;
              --p14;
            } while( p14 );
            p12 += __frame.costs[0];
            --x2;
          } while( x2 );
          tile_c = (uint8_t*)__frame.tile_img;
          img_h2 = __frame.tile_img->height;
          off2 += p11;
          ++y2;
        } while( y2<img_h2 );
        img_c = (BmfImage*)(__frame.p_i_2);
      }
      tile_d = (uint8_t*)__frame.tile_img;
      w_d = (__frame.tile_img)->width;
      w_e = img_h2*LOWORD(__frame.rows[1]);
      (__frame.tile_img)->width = img_h2;
      ((BmfImage*)tile_d)->height = w_d;
      ((BmfImage*)tile_d)->flags ^= 2u;
      *(uint16_t*)&((BmfImage*)tile_d)->stride = w_e;
      free(base2);
    } else {
      __frame.best_bits = bits_d;
      __frame.base = (uint8_t*)bmf_new(img_c->data_size);
      img_h3 = img_c->height;
      __frame.rows[1] = ::plane_count;
      __frame.costs[0] = ::plane_count*(img_h3-1);
      memcpy(__frame.base, img_c->pixels, img_c->data_size);
      img_h3b = img_c->height;
      if( img_h3b ) {
        p7 = __frame.rows[1];
        y3 = 0;
        off3 = 0;
        p9 = __frame.base;
        do {
          pl_i = img_c->width;
          p8 = img_c->pixels+off3;
          __frame.p_i_2 = (BmfImage*)(img_c);
          do {
            p10 = p7;
            do {
              *p8++ = *p9++;
              --p10;
            } while( p10 );
            p8 += __frame.costs[0];
            --pl_i;
          } while( pl_i );
          img_c = (BmfImage*)(__frame.p_i_2);
          img_h3b = __frame.p_i_2->height;
          off3 += p7;
          ++y3;
        } while( y3<img_h3b );
      }
      w16 = img_c->width;
      w_c = img_h3b*LOWORD(__frame.rows[1]);
      img_c->width = img_h3b;
      img_c->height = w16;
      img_c->flags ^= 2u;
      img_c->stride = w_c;
      free(__frame.base);
    }
  }
  free(__frame.tile_buf);
  if( ::plane_count>2 ) {
    if( __frame.bits_total ) {
      for( desc_k = 0; desc_k<4; ++desc_k )
        __frame.saved[desc_k] = plane_desc[desc_k+1];
      if( ::plane_count>0 ) {
        plane = 0;
        do {
          pi3 = plane;
          f2 = plane_desc[plane+++1].flags&8|5;
          plane_desc[pi3+1].flags = f2;
        } while( plane<::plane_count );
      }
      __transform_planes(__frame.tile_img, 0);
      bits_e = 8*(out_cursor-coded_buf);
      deep2 = bits_e<=__frame.best_bits;
      *(uint32_t*)packer_word = packer_acc;
      packer_free_bits = 0;
      packer_acc = 0;
      out_cursor = coded_buf;
      packer_word = (uint32_t*)coded_buf;
      hist_scratch = coded_buf+coded_size-4096;
      if( deep2 ) {
        __frame.best_bits = bits_e;
        nplanes_b = ::plane_count;
        cand = 0;
      } else {
        for( desc_k = 0; desc_k<4; ++desc_k )
          plane_desc[desc_k+1] = __frame.saved[desc_k];
        nplanes_b = ::plane_count;
        cand = 1;
      }
      if( nplanes_b<=2 )
        goto LABEL_63;
    } else {
      cand = 1;
    }
    if( __frame.n_hard+(cand==0) ) {
      for( desc_k = 0; desc_k<4; ++desc_k )
        __frame.saved[desc_k] = plane_desc[desc_k+1];
      nplanes_c = ::plane_count;
      if( ::plane_count>0 ) {
        pl_a = 0;
        do {
          pi0 = pl_a;
          f0 = plane_desc[pl_a+++1].flags&8|6;
          plane_desc[pi0+1].flags = f0;
          nplanes_c = ::plane_count;
        } while( pl_a<::plane_count );
      }
      if( nplanes_c==__frame.n_hard&&nplanes_c-1==__frame.n_flagged ) {
        cand = 0;
      } else {
        __transform_planes(__frame.tile_img, 0);
        bits_a = 8*(out_cursor-coded_buf);
        deep2 = bits_a<=__frame.best_bits;
        __frame.rows[0] = bits_a;
        *(uint32_t*)packer_word = packer_acc;
        packer_free_bits = 0;
        packer_acc = 0;
        out_cursor = coded_buf;
        packer_word = (uint32_t*)coded_buf;
        hist_scratch = coded_buf+coded_size-4096;
        if( deep2 ) {
          __frame.best_bits = bits_a;
          __frame.n_hard = bits_a;
          if( ::plane_count-1==__frame.n_flagged ) {
            cand = 0;
          } else {
            for( desc_k = 0; desc_k<4; ++desc_k )
              __frame.saved[desc_k] = plane_desc[desc_k+1];
            if( ::plane_count>0 ) {
              pl_b = 0;
              do {
                pi1 = pl_b;
                f1 = plane_desc[pl_b+++1].nrefs;
                plane_desc[pi1+1].flags |= 8*(f1!=0);
              } while( pl_b<::plane_count );
            }
            __transform_planes(__frame.tile_img, 0);
            bits2 = 8*(out_cursor-coded_buf);
            deep2 = bits2<=__frame.rows[0];
            *(uint32_t*)packer_word = packer_acc;
            out_cursor = coded_buf;
            packer_word = (uint32_t*)coded_buf;
            packer_free_bits = 0;
            packer_acc = 0;
            hist_scratch = coded_buf+coded_size-4096;
            if( deep2 ) {
              cand = 0;
            } else {
              for( desc_k = 0; desc_k<4; ++desc_k )
                plane_desc[desc_k+1] = __frame.saved[desc_k];
              cand = 0;
            }
          }
        } else {
          for( desc_k = 0; desc_k<4; ++desc_k )
            plane_desc[desc_k+1] = __frame.saved[desc_k];
        }
      }
    }
  } else {
    cand = 1;
  }
LABEL_63:
  if( !__frame.n_hard&&::plane_count>1 ) {
    tile_copy = (uint8_t*)bmf_new(__frame.tile_img->data_size);
    memcpy(tile_copy, (uint8_t*)__frame.tile_src, __frame.tile_img->data_size);
    for( desc_k = 0; desc_k<4; ++desc_k )
      __frame.saved[desc_k] = plane_desc[desc_k+1];
    if( ::plane_count>0 ) {
      pl_c = 0;
      do {
        pi4 = pl_c;
        f3 = plane_desc[pl_c+++1].flags&0xFB;
        plane_desc[pi4+1].flags = f3;
      } while( pl_c<::plane_count );
    }
    __transform_planes(__frame.tile_img, 0);
    bits_b = 8*(out_cursor-coded_buf);
    __frame.rows[1] = bits_b;
    *(uint32_t*)packer_word = packer_acc;
    packer_free_bits = 0;
    packer_acc = 0;
    out_cursor = coded_buf;
    packer_word = (uint32_t*)coded_buf;
    hist_scratch = coded_buf+coded_size-4096;
    if( bits_b>__frame.best_bits ) {
      for( desc_k = 0; desc_k<4; ++desc_k )
        plane_desc[desc_k+1] = __frame.saved[desc_k];
    } else {
      __frame.best_bits = bits_b;
      cand = 0;
    }
    if( __frame.n_flagged+__frame.bits_total ) {
      for( desc_k = 0; desc_k<4; ++desc_k )
        __frame.saved[desc_k] = plane_desc[desc_k+1];
      if( ::plane_count>0 ) {
        pl_d = 0;
        do
          plane_desc[pl_d+++1].flags = 0;
        while( pl_d<::plane_count );
      }
      memcpy((uint8_t*)__frame.tile_src, tile_copy, __frame.tile_img->data_size);
      __transform_planes(__frame.tile_img, 0);
      bits_c = 8*(out_cursor-coded_buf);
      deep2 = bits_c<=__frame.best_bits;
      *(uint32_t*)packer_word = packer_acc;
      packer_free_bits = 0;
      packer_acc = 0;
      out_cursor = coded_buf;
      packer_word = (uint32_t*)coded_buf;
      hist_scratch = coded_buf+coded_size-4096;
      if( deep2 ) {
        cand = 0;
      } else {
        for( desc_k = 0; desc_k<4; ++desc_k )
          plane_desc[desc_k+1] = __frame.saved[desc_k];
      }
    }
    free(tile_copy);
  }
  free(coded_buf);
  free((uint8_t*)__frame.tile_img);
  return cand;
}

BmfArc*__bmf_open_archive(BmfArc* out, char* path, int32_t read_only) {
  BmfArc* arc;
  FILE* fp, * fp2;
  const char* mode;
  int32_t rc, live;
  arc = out;
  mode = "a+b";
  if( read_only )
    mode = "rb";
  out->images = 0;
  fp = fopen(path, mode);
  arc->fp = fp;
  if( !fp )
    __exit_402E40(6, path);
  arc->images = 0;
  rc = fseek(fp, 0, 0);
  live = arc->fp!=nullptr;
  if( rc ) {
    if( !live )
      __exit_402E40(3, path);
    return arc;
  }
  if( !live ) {
    __exit_402E40(3, path);
    return arc;
  }
  if( !feof(arc->fp) ) {
    __expand_image(arc, 1, (void**)nullptr);
    fp2 = arc->fp;
    if( !fp2 ) {
      __exit_402E40(3, path);
      return arc;
    }
    if( !feof(arc->fp) ) {
      arc->images = 0;
      fseek(fp2, 0, 0);
      live = arc->fp!=nullptr;
      {
        if( !live )
          __exit_402E40(3, path);
        return arc;
      }
    }
  }
  return arc;
}

int32_t __compress_image(BmfArc* arc_in, BmfImage* p_i, void* coded_buf) {
  BmfImage hdr;
  int32_t plane_n;
  int32_t row_step;
  uint32_t filtered;
  void* pixels;
  int32_t coded_bytes;
  uint8_t* pix_copy;
  uint32_t coded_len;
  BmfArc* arc;
  FILE* i;
  bool fits;
  uint8_t bpp;
  uint32_t acc, data_bytes, img_stride, row_bytes, shifted;
  uint8_t has_coded;
  uint8_t* plane_buf, * row_at;
  uint32_t hdr_pad8, hdr_pad8b;
  int32_t bits_left, pl, free_bits, pl2, ok_all, y, pl_i, countdown, data_size, row_y;
  uint16_t img_h;
  BmfImage* img;
  uint16_t w16;
  uint32_t pal_bytes, word_flags, word_dc, word_w4, word_w8, word_w12, n_pix, written;
  uint8_t ok, * dst, ok_raw;
  arc = arc_in;
  if( !arc_in->fp )
    return 0;
  if( !feof(arc_in->fp) ) {
    __expand_image(arc, 1, (void**)nullptr);
    for( i = arc->fp; i; i = arc->fp ) {
      if( feof(arc->fp) )
        break;
      if( feof(i) )
        break;
      __expand_image(arc, 1, (void**)nullptr);
    }
    if( !arc->fp )
      return 0;
  }
  has_coded = coded_buf!=nullptr;
  img = (BmfImage*)(p_i);
  row_bytes = p_i->stride;
  if( coded_buf )
    has_coded = 1;
  hdr.width = p_i->width;
  hdr.height = p_i->height;
  p_i->flags |= has_coded<<7;
  hdr_pad8 = *(uint32_t*)&p_i->_pad8;
  hdr.stride = (uint32_t)row_bytes;
  plane_desc[0].weight0 = 512;
  near_lossless_q = 0;
  *(uint32_t*)&hdr._pad8 = hdr_pad8;
  hdr.data_size = (uint32_t)p_i->data_size;
  ::plane_count = ((p_i->depth&0x3Fu)+7)>>3;
  if( fwrite("\x81\x8A"
             "20\x81\x90"
             "20a+b",
             4u, 1u, arc->fp)!=1 )
    return 0;
  bpp = p_i->depth;
  ++*(uint32_t*)arc;
  pal_bytes = bpp&0x80;
  if( (bpp&0x80)!=0 )
    pal_bytes = 3<<(bpp&31);
  if( p_i->data_size<0x10u )
    goto LABEL_76;
  desc_slow_mode = 1;
  hdr.flags |= 0x24;
  if( (p_i->depth&0x3Fu)<=4 ) {
    coded_size = p_i->data_size+0x20000;
    ::coded_buf = (uint8_t*)bmf_new(coded_size);
    ::packer_free_bits = 0;
    ::packer_acc = 0;
    out_cursor = ::coded_buf;
    ::packer_word = (uint32_t*)::coded_buf;
    hist_scratch = ::coded_buf+coded_size-4096;
    plane_predictor = 0;
    plane_alt_model = 0;
    alphabet_reduced = 0;
    __model_plane((BmfImage*)p_i, p_i->pixels, p_i->pixels);
    goto LABEL_57;
  }
  filtered = __search_filter((BmfImage*)p_i, 0);
  hdr.flags |= 0x10u;
  if( (p_i->flags&2)!=0 ) {
    img_stride = p_i->stride;
    hdr_pad8b = *(uint32_t*)&p_i->_pad8;
    hdr.width = p_i->width;
    hdr.height = p_i->height;
    data_bytes = p_i->data_size;
    hdr.stride = (uint32_t)img_stride;
    *(uint32_t*)&hdr._pad8 = hdr_pad8b;
    hdr.data_size = (uint32_t)data_bytes;
    hdr.flags = 0x34|(uint8_t)(hdr_pad8b>>24);
  } else {
    data_bytes = p_i->data_size;
  }
  coded_size = data_bytes+0x20000;
  ::coded_buf = (uint8_t*)bmf_new(data_bytes+0x20000);
  out_cursor = ::coded_buf;
  ::packer_free_bits = 0;
  ::packer_acc = 0;
  ::packer_word = (uint32_t*)::coded_buf;
  hist_scratch = ::coded_buf+coded_size-4096;
  if( ::plane_count==1 ) {
    if( (p_i->depth&0x40)!=0 ) {
LABEL_22:
      near_lossless_q = 0;
      if( ::packer_free_bits<4 ) {
        *(uint32_t*)::packer_word = ::packer_acc;
        ::packer_word = (uint32_t*)out_cursor;
        ::packer_acc = 0;
        ::packer_free_bits += 28;
        out_cursor += 4;
      } else {
        ::packer_free_bits -= 4;
      }
    }
  } else if( ::plane_count>2 ) {
    goto LABEL_22;
  }
  alphabet_reduced = 0;
  if( ::plane_count>0 ) {
    bits_left = ::packer_free_bits;
    pl = 0;
    do {
      word_flags = (4*plane_desc[pl+1].flags)|plane_desc[pl+1].nrefs;
      if( bits_left<6 ) {
        *(uint32_t*)::packer_word = ::packer_acc|(2*(word_flags<<((31-bits_left)&31)));
        ::packer_word = (uint32_t*)out_cursor;
        out_cursor += 4;
        bits_left = ::packer_free_bits+26;
        ::packer_acc = word_flags>>(::packer_free_bits&31);
      } else {
        ::packer_acc |= word_flags<<(-bits_left&31);
        bits_left = ::packer_free_bits-6;
      }
      ::packer_free_bits = bits_left;
      if( (plane_desc[pl+1].flags&8)!=0 ) {
        word_dc = plane_desc[pl+1].dc;
        if( bits_left<8 ) {
          *(uint32_t*)::packer_word = ::packer_acc|(2*(word_dc<<((31-bits_left)&31)));
          ::packer_word = (uint32_t*)out_cursor;
          out_cursor += 4;
          bits_left = ::packer_free_bits+24;
          ::packer_acc = word_dc>>(::packer_free_bits&31);
        } else {
          shifted = word_dc<<(-bits_left&31);
          bits_left -= 8;
          ::packer_acc |= shifted;
        }
        ::packer_free_bits = bits_left;
        if( plane_desc[pl+1].nrefs>1u ) {
          word_w4 = plane_desc[pl+1].weight0+64;
          if( bits_left<8 ) {
            *(uint32_t*)::packer_word = ::packer_acc|(2*(word_w4<<((31-bits_left)&31)));
            ::packer_word = (uint32_t*)out_cursor;
            out_cursor += 4;
            free_bits = ::packer_free_bits+24;
            ::packer_acc = word_w4>>(::packer_free_bits&31);
          } else {
            ::packer_acc |= word_w4<<(-bits_left&31);
            free_bits = ::packer_free_bits-8;
          }
          ::packer_free_bits = free_bits;
          word_w8 = plane_desc[pl+1].weight1+64;
          if( free_bits<8 ) {
            *(uint32_t*)::packer_word = ::packer_acc|(2*(word_w8<<((31-free_bits)&31)));
            ::packer_word = (uint32_t*)out_cursor;
            out_cursor += 4;
            bits_left = ::packer_free_bits+24;
            ::packer_acc = word_w8>>(::packer_free_bits&31);
          } else {
            bits_left = free_bits-8;
            ::packer_acc |= (word_w8<<(-free_bits&31));
          }
          ::packer_free_bits = bits_left;
          if( plane_desc[pl+1].nrefs>2u ) {
            word_w12 = plane_desc[pl+1].weight2+64;
            if( bits_left<8 ) {
              *(uint32_t*)::packer_word = ::packer_acc|(2*(word_w12<<((31-bits_left)&31)));
              ::packer_word = (uint32_t*)out_cursor;
              out_cursor += 4;
              acc = word_w12>>(::packer_free_bits&31);
              bits_left = ::packer_free_bits+24;
              ::packer_free_bits += 24;
              ::packer_acc = acc;
            } else {
              ::packer_acc |= word_w12<<(-bits_left&31);
              bits_left = ::packer_free_bits-8;
              ::packer_free_bits -= 8;
            }
          }
        }
      }
      ++pl;
    } while( pl<::plane_count );
  }
  if( filtered ) {
    n_pix = p_i->width*p_i->height;
    hdr.flags |= 8u;
    plane_buf = (uint8_t*)bmf_new(n_pix);
    if( ::plane_count>0 ) {
      pl2 = 0;
      do
        __model_planes((uint8_t*)p_i, plane_buf, plane_desc[pl2+++1].src_plane, 0);
      while( pl2<::plane_count );
    }
    free(plane_buf);
  } else {
    __transform_planes((BmfImage*)p_i, 0);
  }
LABEL_57:
  *(uint32_t*)::packer_word = ::packer_acc;
  fits = (uint32_t)(out_cursor- ::coded_buf)<p_i->data_size;
  coded_bytes = out_cursor- ::coded_buf;
  hdr.data_size = (uint32_t)(out_cursor- ::coded_buf);
  if( fits ) {
    ok = fwrite(&hdr, 1u, 0x10u, arc->fp)==16;
    if( coded_buf ) {
      coded_len = ((const uint32_t*)coded_buf)[1];
      ok &= fwrite(coded_buf, 1u, coded_len+8, arc->fp)==coded_len+8;
    }
    ok_all = (fwrite(::coded_buf, 1u, coded_bytes, arc->fp)==(uint32_t)coded_bytes)&ok;
    free(::coded_buf);
    if( ok_all&&(p_i->depth&0x80)!=0 )
      fwrite(&p_i->pixels[p_i->data_size], 1u, pal_bytes, arc->fp);
    fflush(arc->fp);
    if( ok_all )
      return (int32_t)hdr.data_size;
    return ok_all;
  }
  free(::coded_buf);
  if( (p_i->flags&2)!=0 ) {
    pix_copy = (uint8_t*)bmf_new(p_i->data_size);
    plane_n = ::plane_count;
    row_step = ::plane_count*(p_i->height-1);
    pixels = (uint16_t*)p_i->pixels;
    memcpy(pix_copy, p_i->pixels, p_i->data_size);
    img_h = p_i->height;
    if( img_h ) {
      row_at = pix_copy;
      row_y = 0;
      y = 0;
      do {
        pl_i = img->width;
        dst = img->pixels+y;
        do {
          countdown = plane_n;
          do {
            *dst++ = *row_at++;
            --countdown;
          } while( countdown );
          dst += row_step;
          --pl_i;
        } while( pl_i );
        img = (BmfImage*)(p_i);
        img_h = p_i->height;
        y += plane_n;
        ++row_y;
      } while( row_y<img_h );
    }
    w16 = img->width;
    img->width = img_h;
    img->height = w16;
    img->flags ^= 2u;
    img->stride = (img_h*plane_n);
    free(pix_copy);
    goto LABEL_77;
  }
LABEL_76:
  pixels = (uint16_t*)p_i->pixels;
LABEL_77:
  ok_raw = fwrite(img, 1u, 0x10u, arc->fp)==16;
  if( coded_buf ) {
    coded_len = ((const uint32_t*)coded_buf)[1];
    ok_raw &= fwrite(coded_buf, 1u, coded_len+8, arc->fp)==coded_len+8;
  }
  written = fwrite(pixels, 1u, pal_bytes+img->data_size, arc->fp);
  data_size = img->data_size;
  if( (ok_raw&(written==data_size+pal_bytes))==0 )
    return 0;
  return data_size;
}

void __bmf_compress(const char* InName, const char* OutName) {
  FILE* fp;
  const uint8_t* Palette;
  int32_t* p_i, Flags, Colours, Step, Grey, i;
  BmfArc* Arc;
  int32_t coded_len;
  fp = fopen(InName, "rb");
  if( !fp )
    __exit_402E40(6, InName);
  fclose(fp);
  p_i = __read_bmp((char*)InName);
  if( !p_i )
    __exit_402E40(4);
  BmfImage*const p_i_img = (BmfImage*)p_i;
  printf("File %16s, image %dx%dx%d, size - %d:", InName, p_i_img->width, p_i_img->height, p_i_img->depth&0x3F, p_i_img->data_size);
  if( void* __nb = bmf_new(sizeof(BmfArc)) )
    Arc = __bmf_open_archive((BmfArc*)__nb, (char*)OutName, 0);
  else
    Arc = nullptr;
  Flags = p_i_img->depth;
  if( (Flags&0x80)!=0 ) {
    if( (Flags&0x40)!=0 ) {
      p_i_img->depth = Flags^0x80;
    } else {
      Colours = 1<<(Flags&31);
      Step = 0x100u>>(Flags&31);
      Palette = (const uint8_t*)p_i+p_i_img->data_size+16;
      Grey = 0;
      for( i = 0; i<Colours; ++i ) {
        if( Palette[3*i]!=Grey||Palette[3*i+1]!=Grey||Palette[3*i+2]!=Grey )
          break;
        Grey += Step;
      }
      if( i>=Colours )
        p_i_img->depth = (Flags|0x40)^0x80;
    }
  }
  coded_len = __compress_image(Arc, (BmfImage*)p_i, (void*)coded_block);
  if( !coded_len )
    __exit_402E40(5, OutName);
  printf("%6.3f bpp\n", (double)coded_len*8.0/(double)(p_i_img->height*p_i_img->width));
  free(p_i);
}

void __bmf_decompress(const char* InName, const char* OutName) {
  int32_t Number, Depth;
  uint32_t* p_i;
  BmfArc* arc;
  if( void* __nb = bmf_new(sizeof(BmfArc)) )
    arc = __bmf_open_archive((BmfArc*)__nb, (char*)InName, 1);
  else
    arc = nullptr;
  printf("File %16s,\r", InName);
  Number = 0;
  while( 1 ) {
    p_i = (uint32_t*)__expand_image(arc, 0, &coded_block);
    BmfImage*const p_i_img = (BmfImage*)p_i;
    if( !p_i ) {
      printf("\n");
      if( !arc->fp )
        __exit_402E40(3, InName);
      __bmf_destroy_archive(arc, 1);
      return;
    }
    ++Number;
    printf("File %16s, image %dx%dx%d, size - %d, number: %d\r", InName, p_i_img->width, p_i_img->height, p_i_img->depth&0x3F, p_i_img->data_size, Number);
    Depth = p_i_img->depth&0x3F;
    if( Depth==2||Depth==15||Depth==16 ) {
      printf("\n%s: %d bits per pixel is not a BMP depth\n", OutName, Depth);
      exit(5);
    }
    if( !__write_bmp((BmfImage*)p_i, (char*)OutName, 1) )
      __exit_402E40(5, OutName);
    free(coded_block);
    coded_block = nullptr;
    free(p_i);
  }
}

int32_t __main(int32_t argc, const char** argv) {
  int32_t mode;
  bmf_set_denormal_mode();
  __set_new_handler(__out_of_memory_handler);
  printf("BMF lossless image compressor, v.2.01 (C) 1998-1999, 2009 by Dmitry Shkarin\n");
  mode = argc==4&&!argv[1][1] ? toupper(argv[1][0]) : 0;
  if( mode!='C'&&mode!='D' ) {
    printf("e-mail: <dmitry.shkarin@mtu-net.ru>;  web: http://compression.graphicon.ru/ds/\n"
           "Usage: bmf c input.bmp output     compress, always with -S -Q9\n"
           "       bmf d input output.bmp     expand\n");
    return 1;
  }
  if( mode=='C' )
    __bmf_compress(argv[2], argv[3]);
  else
    __bmf_decompress(argv[2], argv[3]);
  return 0;
}

void __out_of_memory_handler() {
  __exit_402E40(7);
}

int32_t main(int32_t argc, char** argv) {
  return __main(argc, (const char**)argv);
}
