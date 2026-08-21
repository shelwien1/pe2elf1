// state.inc -- every mutable thing this program has, in one object.

// **The whole of the codec's state, and the reason this file exists.**  These
// twenty-three members were file-scope statics, which is what the decompilation
// had and what a 1997 single-shot compressor could afford.  Two instances of
// this program cannot run in one process while they are: they would share the
// stream, the coder, the plane descriptors and the deadzone.
//
// Grouping them changes nothing on its own -- the same storage, in the same
// order, reached through one name instead of twenty-three.  What it buys is the
// next step: a class that owns them can be instantiated more than once.
//
// `BMFCodec` in codec.inc derives from this and adds the three model-block
// pools, which cannot live here because they hold those classes by value and
// those classes hold a `BMFState*`.  Splitting the state from the pools is what
// lets every block method stay inline: `BMFState` is complete before the first
// of them is parsed.
struct BMFState {
  alignas(16) CodedStream stream;

  // The range coder, wired to `stream` by the constructor below.
  RangeCoder rc;

  alignas(16) CodedTail* coded_block = nullptr;

  alignas(16) int32_t plane_predictor = 0;

  alignas(16) int32_t plane_alt_model = 0;

  int32_t desc_slow_mode;

  int32_t alphabet_reduced;

  PlaneDesc plane_desc[4];

  int32_t plane_count;

  int32_t near_lossless_q;

  uint8_t model_geometry[128];

  int8_t exclusion_gen;

  alignas(16) int8_t exclusion_mask[(no_symbol+1+15)&~15];

  int32_t mode_symbol[5];

  int32_t alt_freq_limit;

  int32_t alt_freq_init;

  LevelGeom level_geom[8];

  int32_t ctx_bias[4];

  int32_t deadzone_hi;

  int32_t deadzone_lo;

  uint8_t* hist_scratch;

  alignas(16) uint16_t model_table_store[kModelTableBytes/sizeof(uint16_t)];

  uint16_t* model_tables;
  // The 1024 histogram counters, bumped through a byte pointer indexed by four
  // -- see the note beside `hist_scratch` in globals.inc for why the storage
  // stays a byte pointer and the access a `memcpy` pair.
  void hist_bump(int32_t k) {
    uint32_t n;
    memcpy(&n, &hist_scratch[4*k], sizeof n);
    ++n;
    memcpy(&hist_scratch[4*k], &n, sizeof n);
  }

  // One strip of the model's frequency tables.  254 `uint16_t` to the strip.
  uint16_t* model_strip(uint32_t k) {
    return model_tables+254*k;
  }

  // The coder reads and writes `stream` and needs nothing else of this object,
  // so it takes the stream rather than the state.
  BMFState() {
    rc.st = &stream;
  }
};

// One instance, for now.  Every reference in the tree names it, which is what
// makes the next step -- passing the state rather than reaching for it -- a
// change of spelling at each site rather than a change of shape.
static BMFState bmf_cx;
