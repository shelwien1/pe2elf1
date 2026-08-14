// globals.inc -- the program's state, and the exit codes it answers with.  The small
// structs are here rather than with the classes because globals are declared in
// terms of them.
//
// `alignas(16)` on a scalar is not a request: these sat in BMF.exe's data
// segment at sixteen-byte boundaries because the code around them loaded them
// with SSE, and the recovery keeps the alignment so that a build which still
// does can.

// The exit codes, which are also the indices into the message table below --
// that table is what defines them, so there is nothing here to invent.  They
// are the program's exit status as well, and `test.sh` reads them, so the
// numbers are the interface and not just an ordering.
//
// `bmf_no_option` and `bmf_user_break` are in the table and never raised: the
// original had an option parser and a keyboard-interrupt check, and this build
// has neither.  They stay because removing them would renumber the five that
// are raised.
enum BmfExit {
  bmf_no_file    = 1,
  bmf_no_option  = 2,
  bmf_bad_file   = 3,
  bmf_read_error = 4,
  bmf_write_error= 5,
  bmf_no_open    = 6,
  bmf_no_memory  = 7,
  bmf_user_break = 8,
};

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

// Five descriptors for four planes: `plane_desc[0]` is not a plane, and the
// original parked two unrelated globals in the weight fields it left free.  The
// aliases are kept because the *layout* is the program's -- these two names have
// to occupy those two words, and `weight0` of the same record carries the
// version check in `image_expand` for the same reason.
static PlaneDesc plane_desc[5];

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

static inline uint8_t*bmf_plane_desc(int32_t off) {
  return (uint8_t*)&plane_desc[1]+off;
}

__attribute__((noreturn)) void bmf_fatal(int32_t Code, ...);

t_new_handler __set_new_handler(t_new_handler __out_of_memory_handler) {
  t_new_handler __set_new_handler_pout_of_memory_handler = __pout_of_memory_handler;
  __pout_of_memory_handler = __out_of_memory_handler;
  return __set_new_handler_pout_of_memory_handler;
}

static void bmf_set_denormal_mode() {
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
}

static inline float bmf_hsum4(const float x[4]) {
  return (x[0]+x[2])+(x[1]+x[3]);
}

void __out_of_memory_handler() {
  bmf_fatal(bmf_no_memory);
}
