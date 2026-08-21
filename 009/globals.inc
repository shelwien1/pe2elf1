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

// One more member after the image, which a BMF file may or may not carry.
// Eight bytes of header and then its bytes; `expand_image` reads it and
// `compress_image` writes it back out unread, so what is in `data` is this
// program's business only insofar as it copies it.  `len` is the byte count
// from the file, rounded up to a word when the block is allocated -- and
// rounded up from *zero* to four, which is why an empty tail still costs a
// word.
//
// `data[0]` rather than `data[]`, for the same reason `BmfImage::pixels` is:
// a flexible array member may not be followed by anything, and this record is
// also a member of `expand_image`'s frame, where the eight bytes are read
// before the block they describe is allocated.
struct CodedTail {
  int32_t tag;
  uint32_t len;
  uint32_t data[0];
};




// The compressed output, and the bit packer riding on the same bytes.
//
// Six globals until now -- `stream.buf`, `stream.cur`, `stream.size`,
// `stream.pk.word`, `stream.pk.acc`, `stream.pk.free_bits` -- and they are two objects,
// not six: the first three are the byte stream, the last three are a bit
// packer over it.  Three of them carried `alignas(16)` from the donor's data
// segment; nothing loads them with SSE, and the grouping is what says which
// belongs to which.
//
// `free_bits` goes transiently negative: `packer_rewind` counts it down past
// zero while it hands whole bytes back to the coder.  That is the one
// non-obvious thing about this struct and it is load-bearing.
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




// A plane descriptor's `flags` nibble, which is the top four bits of the
// six-bit word on the wire -- `flags<<2 | nrefs`, packed by `compress_image`
// and unpacked by `expand_image`.
//
// `desc_predictor` is two bits holding a number rather than a flag, and it is
// the `plane_predictor` every model in the tree branches on: 0 is the mode-0
// predictor, 1 is MED (alt_p1), 2 is alt_p2.
//
// 3 is representable and the encoder never writes it.  `search_filter` probes
// six descriptors and every later edit it makes either leaves these two bits
// alone or assigns 1 or 2 outright, so no path produces the fourth value; on
// the decode side it is therefore a number off a file that matches none of the
// `==pred_p1`/`==pred_p2` tests and falls through to mode 0's shape.
enum : uint8_t {
  desc_predictor = 0x03,  // which predictor, 0..2 -- the three below
  desc_alt_model = 0x04,  // code the residual with the alternate model
  desc_has_refs  = 0x08,  // a DC and up to three weights follow, and the plane
                          // is predicted from `nrefs` others
};

// The three values of that two-bit field, which is what `plane_predictor`
// holds and what every `==1` and `==2` in the models was testing.  `p1` and
// `p2` are the tree's own names for the two alternate models -- alt_p1_*.inc
// and alt_p2_*.inc -- and neither is about pixel depth.
enum : int32_t {
  pred_mode0 = 0,         // no per-plane predictor at all

  pred_p1    = 1,         // MED, and the p1 model
  pred_p2    = 2,         // the p2 model
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

// Four descriptors for four planes.  There were five: slot 0 was a header slot
// rather than a plane, and the original parked three unrelated things in the
// weight words it left free -- `plane_count` and `near_lossless_q` were
// references onto two of them and the archive version word used the third.
// All three are their own storage now, which is what let the array lose the
// slot and every access lose its `+1`.


// The near-lossless quantiser: the residual bucket width is `2*q+1`, so `q=0`
// is lossless.  Three sites read it -- `alt_init_tables`'s bucket size and
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





struct LevelGeom {
  uint8_t first;
  uint8_t half;
  uint8_t tbl_base;
  uint8_t _pad;
};





static constexpr int32_t opt_use_filters = 1;

static constexpr int32_t opt_slow = 1;

static constexpr int32_t opt_filter_template = 0;

static constexpr int32_t opt_pack_output = 1;

static constexpr int32_t opt_search_quality = 9;

static constexpr int32_t opt_max_error = 0;

// The 1024 `uint32_t` counters at the tail of the coded buffer, which
// `packer_reset` re-bases to `stream.buf + stream.size - 4096` between trials.
//
// A byte pointer indexed by four, not a `uint32_t*`, and that is measured
// rather than inherited.  The base is not always four-aligned: an assert in
// `packer_reset` fires on the `len1` malformed-decode case in the corpus.  It
// never fires at a site that actually *indexes* the counters -- the same
// assert moved to the three that do survives 110 checks, 400 fuzz mutants and
// 8,704 header variations -- but a `uint32_t*` would be a misaligned pointer
// the moment `packer_reset` formed it, dereferenced or not.
//
// So the storage stays as the binary has it, and the access is a `memcpy`
// pair rather than a `uint32_t&` -- which is the same read-modify-write with
// the alignment told truthfully, and needs no claim about the base at all.

// `hist_bump` reads `hist_scratch` and is a `BMFState` method now; see
// state.inc.

// The exclusion mask: one byte per symbol, stamped with the current generation
// once the symbol has been offered and rejected, so the next candidate that
// names it can be skipped.  `exclusion_gen` is that generation, and bumping it
// is how the whole mask is cleared without touching it.
//
// This was two references onto one 8736-byte blob -- `exclusion_mask[8192]`
// and a `__byte_445440[544]` for the tail -- and the split was wrong twice
// over.  Three things set the real size and none of them is 8192:
//
//   * the alphabet reaches 8193 symbols.  `rc_decode_flat(8193)` returns
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


// 520,192 bytes, the same every time, and reached through this one pointer --
// which is what bounds it to one: a second table would have to overwrite the
// pointer to the first, so no two can be live.  `begin_plane_stream` filled a
// fresh `bmf_new` and `end_plane_stream` freed it, once per plane stream, for a
// table whose size is a compile-time constant.
//
// `uint16_t` and a byte count: `kModelTableBytes` is a byte count in the
// decompilation and stays one, because nothing divides it by an element size.


// `model_strip` reads `model_tables` and is a `BMFState` method now; see
// state.inc.


// **These three were file-scope, and the stage-2 appendix's reason for moving
// them into `ModelBlock` did not hold.**  They are `BMFState` members now,
// which is a different move for a different reason -- the appendix wanted them
// inside the one class that seemed to use them, and the measurement below is
// why that was wrong.  A state object takes all three without having to claim
// that any one class owns them.  It says `exclusion_gen`, `exclusion_mask` and
// `mode_symbol` "are used only by `ModelBlock`, so they can become members
// without a context object".  Measured, they are not:
//
//   `SymList::code_symbol`         reads `exclusion_gen` and writes
//                                  `exclusion_mask`, four sites
//   `SymList::decode_symbol_list`  the same, three sites
//   `reduce_alphabet`              writes `mode_symbol[1]`, twice, and it is a
//                                  free function
//
// `SymList` is not a member of `ModelBlock` -- the block holds pointers to
// lists whose lifetime it does not own -- so making these members means
// threading a `ModelBlock&` through the two hottest coding loops in the
// program.  That is the whole-program control-flow change the same appendix
// rejects two entries above, bought for no expression-level gain, and it is
// the `BmfContext` consolidation under another name.  Stage 3, with the split.

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

// Seven taps by four lanes against seven inputs by four lanes: one prediction.
// The four lanes accumulate independently and fold at the end, which is not the
// same rounding as one running sum and is why the fold is `bmf_hsum4` and not a
// fifth `+=`.
//
// `[7][4]` rather than twenty-eight scalars is the layout the original's SSE
// was in; keeping it is what lets a compiler put it back, and it is also what
// makes the five call sites one function.  Three are in `NbRow::predict` -- the
// fixed coefficients, the mixture of the six sets, and the row's own weights --
// and two are the forward halves of the NLMS steps in `AltP2Block`, which is
// why this lives here rather than in `alt_p2.inc`: that file is included after
// `alt_p2_block.inc`.
static inline float nb_dot(const float (*wt)[4], const float (*nb)[4]) {
  float acc[4];
  for( int32_t k = 0; k<4; ++k ) {
    acc[k] = wt[0][k]*nb[0][k];
    for( int32_t j = 1; j<7; ++j )
      acc[k] += wt[j][k]*nb[j][k];
  }
  return bmf_hsum4(acc);
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
  // The direction is load-bearing: the turns touch the same places.
  for( int32_t i = 0; i<4; ++i ) buf[4-i] = buf[3-i];
  buf[0] = oldest;
  for( int32_t k = 0; k<5; ++k )
    cursor[k] = buf[k]+margin;
}

// `ring_free` was here, four lines giving five rows back.  Both rings point
// into their block's own storage now, so there is nothing to give.

