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

// -v: report the encoder's coding-method trials and the choices it makes.
// Reporting only prints; it never touches the coded stream.
int32_t bmf_verbose = 0;

// Every cost the encoder measures or estimates is in bits; -v reports bytes.
double bmf_bytes(int64_t bits) {
  return (double)bits/8.0;
}

PRINTFLIKE(1, 2)
void bmf_log(const char* fmt, ...) {
  if( !bmf_verbose )
    return;
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

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

// The cross-plane blend weight scale, in two forms.  The shift is the
// parameter and the scale is derived from it, so a sweep cannot leave the two
// disagreeing -- which would put a factor of two between the weights the
// encoder fits and the divisor it applies them with.
const int32_t kWeightShift = idx_clamp(CD_weight_shift, 1, 16);

// The two decision margins: a challenger has to beat the incumbent by more
// than 1/2^shift of its own cost to displace it.  kKeepShift guards the
// representation trials (transpose, mirror, palette order, colour space),
// kModelShift the per-plane model choice.
// The selector mixers' granularity.  Each mixer's weights are multiples of
// 1<<shift, and the fields packed under the sum in the corresponding index
// fill exactly that many low bits -- so the sum enters the index as its own
// digit and no division is needed to extract it.  The codec checks the two
// agree at startup rather than trusting the declaration.
const int32_t kP1MixShift = idx_clamp(P1_ctxw_shift, 0, 16);
const int32_t kP2MixShift = idx_clamp(P2_ctxw_shift, 0, 16);

const int32_t kKeepShift = idx_clamp(CD_keep_shift, 1, 24);
const int32_t kModelShift = idx_clamp(CD_model_shift, 1, 24);

// The ladder of frequency floors the two-tier renumbering map is cut at: a
// value is dense when the plane's histogram gives it at least npix>>shift of
// the samples.  The right rung is per plane and per image -- PIA13757 wants no
// split at all, PIA13785 wants >>11, PIA13810 wants >>15 -- so all of them get
// probed and the cheapest wins.  The range stops at >>10 because 1/1024 of an
// image is already a visible feature, and at >>16 because below that the floor
// stops excluding anything on the frames that have a strip.
const int32_t kRareShiftLo = 10, kRareShiftHi = 16;
const int32_t kWeightScale = 1<<kWeightShift;

int32_t clamp32(int32_t v, int32_t lo, int32_t hi) {
  return v<lo ? lo : v>hi ? hi : v;
}

// The weight floor is declared as its magnitude: an IDX pattern is unsigned,
// and a sign that can be flipped by the optimizer is not a knob, it is a bug.
const int32_t kWeightMin = -GL_weight_min_neg, kWeightMax = GL_weight_max;
const uint32_t kSlackMax = GL_slack_max;
const int32_t kP2FreqRescaleTotal = GL_p2_freq_rescale_total;
const int32_t kP2RowPad = 234;     // slack columns alt-P2 pads each row with
const int32_t kCtxId3Limit = GL_ctx_id3_limit;
// The frequency arena, sized from the index that addresses it rather than
// carried as its own number: model_strip() cuts it into 254-entry uint16 rows
// and CtxPair is what selects a row, so the arena holds exactly one row per
// context by construction.  As two independent numbers -- it was 520192 in the
// .idx, which is 1024*254*2 -- widening the context silently overflows the
// arena, or leaves it oversized, with nothing to say the two had drifted.
const uint32_t kModelStripSize = 254;
const uint32_t kModelTableBytes =
    (uint32_t)P2_CtxPair_Volume*kModelStripSize*(uint32_t)sizeof(uint16_t);
const uint32_t kBankMirrorMask = 0x7FF0u;  // bits 4..14: the fan the bias banks mirror over
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

const int32_t kMaxWidth = 65535;   // the width field on the wire is 16 bits



template <class T, int32_t N> struct BlockPool {
  alignas(4096) T slot[N];
  uint8_t taken[N];
  // Whether this slot has been seeded since the coding pass began.  Tiled
  // coding carries a model's statistics from tile to tile, but the first tile
  // that reaches a given model still has to seed it, and which tile that is
  // depends on the descriptors -- an image whose first four tiles all choose
  // alt-P2 does not touch the slow model until the fifth.  A global "this is
  // not the first tile" flag gets that wrong, and gets it wrong asymmetrically:
  // the encoder's blocks have been through the descriptor search and the
  // decoder's have not, so the two would carry different state and the stream
  // would not decode.  Seeding is per block, and it is this flag.
  uint8_t seeded[N];
  void unwatch(int32_t k) {}

  void watch(int32_t k) {}

  void Init() {
    for( int32_t k = 0; k<N; ++k ) {
      taken[k] = 0;
      seeded[k] = 0;
    }
  }

  // Called at the start of a coding pass: every block seeds itself again the
  // first time that pass reaches it, and every block is free.  Both halves
  // matter.  The encoder searches before it codes and the decoder does not, so
  // a block still marked taken when the coding pass begins would send the
  // encoder's take_slot to its fallback and the decoder's to the named slot --
  // two different blocks for the same plane, and a stream that cannot decode.
  void unseed() {
    for( int32_t k = 0; k<N; ++k ) {
      taken[k] = 0;
      seeded[k] = 0;
    }
  }

  bool was_seeded(int32_t k) const {
    return k>=0&&k<N&&seeded[k]!=0;
  }

  void mark_seeded(int32_t k) {
    if( k>=0&&k<N )
      seeded[k] = 1;
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

  // A named slot rather than the first free one.  Tiled coding carries a
  // model's statistics from one tile to the next, which only means anything if
  // the same coding slot comes back to the same block; take() would hand every
  // slot the lowest free one and merge them.
  T*take_slot(int32_t k, bool zero) {
    if( k<0||k>=N||taken[k] )
      return take(zero);
    taken[k] = 1;
    unwatch(k);
    if( zero )
      memset(&slot[k], 0, sizeof(T));
    return &slot[k];
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
