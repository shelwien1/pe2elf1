// globals.inc -- the program's state, and the exit codes it answers with.  The small
// structs are here rather than with the classes because globals are declared in
// terms of them.

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
  __exit_402E40(7);
}
