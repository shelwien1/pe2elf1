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
struct SymList;
struct ModelBlock;
struct ReduceNode;
struct AltP2Block;

struct BMFState {
  alignas(16) CodedStream stream;

  // The range coder, wired to `stream` by the constructor below.
  RangeCoder rc = {};

  alignas(16) CodedTail* coded_block = nullptr;

  alignas(16) int32_t plane_predictor = 0;

  alignas(16) int32_t plane_alt_model = 0;

  int32_t desc_slow_mode = {};

  int32_t alphabet_reduced = {};

  PlaneDesc plane_desc[4] = {};

  int32_t plane_count = {};

  int32_t near_lossless_q = {};

  uint8_t model_geometry[128] = {};

  int8_t exclusion_gen = {};

  alignas(16) int8_t exclusion_mask[(no_symbol+1+15)&~15];

  int32_t mode_symbol[5] = {};

  int32_t alt_freq_limit = {};

  int32_t alt_freq_init = {};

  LevelGeom level_geom[8] = {};

  int32_t ctx_bias[4] = {};

  int32_t deadzone_hi = {};

  int32_t deadzone_lo = {};

  uint8_t* hist_scratch = {};

  alignas(16) uint16_t model_table_store[kModelTableBytes/sizeof(uint16_t)];

  uint16_t* model_tables = {};
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

  // **The six free functions a block class calls.**  Everything else that
  // touches this state is a `BMFCodec` method, because `BMFCodec` is complete
  // before it is parsed.  These six are not: they are reached from inside
  // `P2Freq`, `SymList`, `BitCtr` and the two alphabet coders, all of which are
  // defined before the pools exist.  Declared here and defined in their own
  // files, which is what lets those callers stay where they are.
  //
  // The set was computed rather than guessed -- ten functions are called from a
  // block class, four of those are already methods of one, and the transitive
  // closure of the rest is these six.
  template<int32_t f_DEC>
  int32_t code_symbol_tree(uint16_t* freq, int32_t sym);
  void encode_symbol_tree(uint16_t* freq, int32_t sym);
  int32_t decode_symbol_tree(uint16_t* freq);
  int32_t update_binary_pair(uint16_t* _this, int32_t symbol);
  uint32_t rc_decode_flat(uint32_t tot);
  uint32_t alt_init_tables(uint8_t* fold, int8_t* unfold);
  int32_t tree_place(ReduceNode* tree, ModelBlock* blk, uint32_t val);
  void reduce_alphabet(ModelBlock* blk, uint8_t* src);

  // **The eight the block classes reach through `cx`.**  A block is parsed
  // before `BMFCodec` exists, so anything it calls has to be state-level; these
  // are the range coder's two ends and what they pull in.  Computed the same way
  // the six above were.
  uint16_t* begin_plane_stream();
  void packer_rewind();
  template<int32_t f_DEC>
  uint16_t*rc_begin();
  void rc_begin_decode();
  uint16_t*rc_begin_encode();
  template<int32_t f_DEC>
  void rc_end();
  void rc_end_decode();
  void rc_end_encode();

  // **The two that only `BMFCodec` calls and that still cannot be its methods.**
  // Both are defined in alt_p2_block.inc, which is parsed before codec.inc, and
  // `alt_p2_start_row` takes an `AltP2Block**` -- so moving them later to make
  // them `BMFCodec` methods would mean moving them away from the block they are
  // about.  They read `plane_desc` and `plane_count` and nothing else, so this
  // is the class they belong to anyway.
  //
  // They were free functions taking `BMFState*` as a first parameter, which is
  // what `tools/methodise.py` reports: a first parameter that is only ever a
  // receiver.  It found them the round the class arrived, because that round is
  // what turned them into that shape.
  bool ref_transformed(int32_t k);
  void alt_p2_start_row(AltP2Block** plane, int32_t row, int32_t width);

  // **The p2 model's two borrowed tables, one copy per codec.**  `P2Coef` folds
  // rows 4..6 of the coefficients into 0..2 for the length of one image and puts
  // them back at the end -- a session, as the note there says.  While they were
  // file-scope arrays that session was shared: `tools/parallel.cpp` under
  // ThreadSanitizer named `P2Coef::fold` writing what `nb_dot` was reading in
  // another thread, and `t8g` came back refused.
  //
  // Seeded from `bmf_p2_coef_init` and `bmf_p2_rate_init` in `reset`, which is
  // what makes those genuinely constant.
  // `alignas(16)` and not by habit: the tables they copy carry it because the
  // code around them loads them with SSE, and dropping it here crashed the
  // optimised build inside `nb_dot` while the `-O0` one was fine.  The note at
  // the top of globals.inc says this about the originals; it is true of the
  // copies for the same reason.
  alignas(16) float p2_coef[7][4] = {};
  alignas(16) float p2_rate[7][4] = {};

  // The two alphabet coders' working storage.  `byte_list_ent` is the entries of
  // a `SymList[16]` coded 256 symbols at a time; `gap_list_ent` is the sparse
  // alphabet's gap list, 257 because those sites are only reached where
  // `depth <= 8` and so `mask` is at most 255.
  //
  // **Both were file-scope arrays and the parallel test found them.**  Two
  // codecs running at once wrote into the same 12 KB, and `t8g` compressed to
  // 43688 bytes in a thread against 43664 alone.  Nothing else in the tree had
  // survived as shared state; these two had, because they were introduced as
  // storage rather than as state and never went through the census.
  SymEntry byte_list_ent[16*256] = {};
  SymEntry gap_list_ent[257] = {};

  // **Back to the state a fresh codec has**, and the reason there is a function
  // for it rather than only a constructor: a codec that has coded once is not
  // in that state any more -- the tables, the geometry and the descriptors all
  // carry the last image's setup -- so a *reused* codec answered differently
  // from a new one.  `tools/parallel.cpp` found it: three rounds on one codec
  // disagreed with one round, and the sizes moved between runs.  Both entry
  // points call this, which is what makes reuse mean anything.
  //
  // A `memset` and not twenty-five assignments: this is a standard-layout
  // aggregate and the constructor is the only thing that makes it non-trivial,
  // so one zeroing covers every member, said once rather than once per member
  // and impossible to leave a member out of.
  //
  // **This and not the `= {}` above is what guarantees it**, which is worth
  // being exact about because most members carry one and three do not --
  // `stream`, `exclusion_mask` and `model_table_store`, the three big ones.
  // Those initialisers are belt to this braces: they run first and this
  // overwrites them, so a member added below without one is still zero.
  //
  // `coded_block` is zeroed and not freed: it is a buffer the caller owns and
  // hands back to `compress_image`, and this object never owned it.
  void reset() {
    memset(static_cast<void*>(this), 0, sizeof(BMFState));
    // The coder reads and writes `stream` and needs nothing else of this
    // object, so it takes the stream rather than the state.
    rc.st = &stream;
    memcpy(p2_coef, bmf_p2_coef_init, sizeof p2_coef);
    memcpy(p2_rate, bmf_p2_rate_init, sizeof p2_rate);
  }

  BMFState() {
    reset();
  }
};

