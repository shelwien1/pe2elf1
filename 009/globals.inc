// globals.inc -- the program's state, and the exit codes it answers with.  The small
// structs are here rather than with the classes because globals are declared in
// terms of them.
//
// `alignas(16)` on a scalar is not a request: these sat in BMF.exe's data
// segment at sixteen-byte boundaries because the code around them loaded them
// with SSE, and the recovery keeps the alignment so that a build which still
// does can.

__attribute__((noreturn)) void bmf_fatal(int32_t Code, ...) {
  va_list ap;
  static const char*const message[] = {
    "", "File not found: %s", "Unknown option: %s", "%s: bad file!", "Read error!", "Write error for file %s", "Can't open file: %s", "Out of memory!", "User break!",
  };
  va_start(ap, Code);
  vprintf(message[(uint32_t)Code<9 ? Code : bmf_read_error], ap);
  va_end(ap);
  printf("\n");
  exit(Code);
  __builtin_unreachable();
}

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

// Four descriptors for four planes.  There were five: slot 0 was a header slot
// rather than a plane, and the original parked three unrelated things in the
// weight words it left free -- `plane_count` and `near_lossless_q` were
// references onto two of them and the archive version word used the third.
// All three are their own storage now, which is what let the array lose the
// slot and every access lose its `+1`.
static PlaneDesc plane_desc[4];

static int32_t plane_count;

// The near-lossless quantiser: the residual bucket width is `2*q+1`, so `q=0`
// is lossless.  Three sites read it -- `__alt_init_tables`'s bucket size and
// two deadzone widths in `AltP2Block` -- and four set it, all four to zero,
// two on each side of the round trip.  There is no option that reaches it.
//
// It stays.  The plan this tree is being refactored against asks for the
// half-state to be resolved either by exposing it as a flag or by stripping the
// dead plumbing, and neither is right here: a flag would change what the
// program does, and stripping would delete arithmetic that is in BMF.exe.
// Recovering the binary is what this tree is for, so the resolution is to say
// so, once, here -- the plumbing is correct, complete, and never exercised
// above a bucket width of one.
static int32_t near_lossless_q;

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

// The exclusion mask: one byte per symbol, stamped with the current generation
// once the symbol has been offered and rejected, so the next candidate that
// names it can be skipped.  `exclusion_gen` is that generation, and bumping it
// is how the whole mask is cleared without touching it.
//
// This was two references onto one 8736-byte blob -- `exclusion_mask[8192]`
// and a `__byte_445440[544]` for the tail -- and the split was wrong twice
// over.  Three things set the real size and none of them is 8192:
//
//   * the alphabet reaches 8193 symbols.  `__rc_decode_flat(8193)` returns
//     0..8192 and the count is that plus one (`model.inc`), so a symbol index
//     reaches 8192 -- one past the end of the array that was named for it.
//   * a `SymPair` slot with no symbol yet holds 8192 (`model_workspace.inc`
//     seeds every one of them to it), and `ModelBlock::pixel_context` rejects
//     such a candidate by finding that byte stamped.  So the empty-slot
//     sentinel and the largest possible symbol share slot 8192 on purpose, and
//     `__byte_445440[0]` was that byte: the two writes to it sit in the middle
//     of the four that exclude `mode_symbol[1..4]`.
//   * the generation reset clears in sixteen-byte blocks, `(alphabet+15)>>4`
//     of them (`model.inc`), which for 8193 symbols writes 8208 bytes.
//
// The last is the binding constraint, and it is why `blob-independence.txt`
// records the tail as SHARED: something writes across the boundary, and the
// something is that round-up.  One array, sized by the clear that is widest.
static constexpr int32_t no_symbol = 8192;

alignas(16) static int8_t exclusion_mask[(no_symbol+1+15)&~15];

static uint16_t* model_tables;

static inline uint16_t*model_strip(uint32_t k) {
  return model_tables+254*k;
}

static int32_t mode_symbol[5];

static inline void bmf_zero16(void* p) {
  __builtin_memset(p, 0, 16);
}

static void bmf_set_denormal_mode() {
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
}

static inline float bmf_hsum4(const float x[4]) {
  return (x[0]+x[2])+(x[1]+x[3]);
}

// The five-row ring both alternate models keep.  `buf[0..4]` are the last five
// rows, oldest last, and `cursor[k]` points `margin` records into row `k` so
// that a negative index reaches the left-edge padding.  Advancing rotates the
// rows -- the oldest buffer is reused as the newest -- and repoints all five.
//
// Written out in `AltP1Block::advance_row` and again in `alt_p2_d8_body`, the
// two differing only in `margin` and the record type, which is what a template
// is for.  Both spelled the rotation with five named temporaries because MSVC
// held all five in registers.
//
// The padding-cell copies on either side of the call are deliberately *not*
// here: the two models mirror different numbers of cells from different
// offsets, and folding those in would mean one helper with two shapes behind
// a flag, which is not one helper.
template<typename T>
static inline void ring_advance(T** buf, T** cursor, int32_t margin) {
  T*const oldest = buf[4];
  buf[4] = buf[3];
  buf[3] = buf[2];
  buf[2] = buf[1];
  buf[1] = buf[0];
  buf[0] = oldest;
  for( int32_t k = 0; k<5; ++k )
    cursor[k] = buf[k]+margin;
}

