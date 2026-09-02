// lstm_io.inc - the LSTM's parameters in the transformer version's weights
// file format (FX2TFWC2), so that a run can start from the model an earlier
// run ended with.
//
// The container, the range coder and the per-tensor encodings are the
// transformer's, included from ../tf; this file only decides which tensors an
// LSTM has.
//
// STORAGE.  Two forms, chosen by the `q4` argument to Save():
//
//   f32   every tensor DT_F32 or DT_I32, which the writer stores with its
//         lossless per-byte-plane model.  A save/load round trip is bit-exact.
//
//   q4    the weight matrices instead go out the way the transformer's do -
//         15 int4 levels against a per-row bf16 scale, DT_I8 + DT_BF16, which
//         the container range-codes with its own int4 and bf16 models - as
//         "<name>.q" and "<name>.scale" in place of "<name>".  Lossy, and
//         about a quarter of the size.  Everything else stays f32: A/B/C are
//         90 floats apiece, and an AdamW variance is non-negative with a wide
//         dynamic range, so levels laid out symmetrically about zero would be
//         a poor fit for it.
//
// The loader takes either, tensor by tensor, whatever the build writes, so
// checkpoints in the two forms are interchangeable.
//
// Every name is prefixed "lstm.", so an LSTM checkpoint and a transformer
// checkpoint can never be mistaken for one another even though they share a
// container.  A saved file can be unpacked with the repository's tfwc tool
// like any other.
//
// THE ALPHABET.  The LSTM indexes symbols by their PACKED index - the rank of
// a byte among the distinct byte values that occur in the input - so the same
// weight column means different things for two files with different
// alphabets.  The checkpoint therefore stores those dimensions byte-indexed,
// 256 wide, next to the set of bytes it was actually trained on.  Two rules
// follow, and both matter:
//
//   loading  fills in only the columns the checkpoint was trained on.  Every
//            other column keeps the value Init() gave it, which is the only
//            correct answer available: a column's initial value
//            val*cos(M_PI/(i+1)*j) is a function of the PACKED index j, and so
//            of the whole alphabet, and cannot be recovered from a file.
//
//   saving   carries the checkpoint it started from through.  A run only has
//            columns for the bytes in ITS file; writing just those would drop
//            everything the checkpoint knew about the rest of the alphabet,
//            and one pass over a narrow file would permanently shrink a model
//            trained on a wide one.  So the columns this run cannot speak for
//            are copied over verbatim from the file it loaded - both are
//            byte-indexed, so no remapping is involved - and the two alphabets
//            are unioned.
//
// Which is also why a column counts as trained only if the checkpoint had it
// or this build actually trains (LSTM_TRAIN): an untrained column holds an
// Init() value belonging to one particular alphabet, and passing that off as a
// weight would corrupt the next run that has a different one.

namespace lstm_io {

using fx2::DT_F32;
using fx2::DT_I32;
using fx2::WeightsFile;
using fx2::WTensor;

static const uint SYMS = 256;  // the file's symbol axis is byte-indexed
static const int32_t FORMAT_VERSION = 1;

enum {
  CFG_VERSION = 0,
  CFG_NUM_CELLS,
  CFG_HIDDEN_SIZE,
  CFG_HORIZON,
  CFG_UPDATE_LIMIT,
  CFG_N_SYM,          // how many byte values the model was trained on
  CFG_HAS_OPTIMIZER,  // 1 if the AdamW moments are in the file
  CFG_RESERVED,  // a feature word, when something needs one - see the note
                 // about lstm.inc's "#if 0" at the bottom of this file
  CFG_COUNT
};

// byte -> packed symbol index, or -1 for a byte the alphabet does not contain
struct Alphabet {
  int pack[SYMS];
  uint n_sym;

  void Init(const char* cmap) {
    n_sym = 0;
    for( uint b = 0; b<SYMS; b++ )
      pack[b] = cmap[b] ? int(n_sym++) : -1;
  }
};

static void put_u64(int32_t* p, qword v) {
  p[0] = int32_t(uint32_t(v));
  p[1] = int32_t(uint32_t(v>>32));
}

static qword get_u64(const int32_t* p) {
  return qword(uint32_t(p[0]))|(qword(uint32_t(p[1]))<<32);
}

// ---------------------------------------------------------------- writing --

static size_t dtype_bytes(uint8_t dtype) {
  return dtype==fx2::DT_I8 ? 1 : dtype==fx2::DT_BF16 ? 2 : 4;
}

struct Builder {
  WeightsFile wf;

  void add(const std::string& name, uint8_t dtype,
           std::initializer_list<uint32_t> shape, const void* src) {
    WTensor t;
    t.dtype = dtype;
    t.shape.assign(shape.begin(), shape.end());
    size_t n = 1;
    for( uint32_t d : t.shape )
      n *= d;
    t.numel = n;
    t.data.resize(n*dtype_bytes(dtype));
    memcpy(t.data.data(), src, t.data.size());
    wf.order.push_back(name);
    wf.tensors.emplace(name, std::move(t));
  }
};

// A [rows][cols] matrix, in whichever of the two forms the caller asks for.
// The int4 quantizer is the transformer's own (../tf/weights_write.inc), so
// the two coders round exactly alike.
static void add_matrix(Builder& b, const std::string& name, const float* w,
                       uint rows, uint cols, bool q4, const WeightsFile* base) {
  if( !q4 ) {
    b.add(name, fx2::DT_F32, {rows, cols}, w);
    return;
  }

  // The scale each row was loaded with, which the quantizer keeps when it
  // still covers the row.  That is what makes a frozen save reproduce the file
  // it read: without it a row whose largest level is 5 would be re-fitted to
  // 7 and every weight in it re-rounded.
  //
  // An all-zero row - lstm.output_layer has one per byte outside the alphabet
  // - needs no special case even though its scale is arbitrary: the quantizer
  // gives every such row 1.0, on the first save and on each one after, so the
  // two agree without being made to.
  const std::string sname = name+".scale";
  const uint16_t* prev = 0;
  if( base&&base->has(sname) ) {
    const WTensor& t = base->tensors.find(sname)->second;
    if( t.dtype==fx2::DT_BF16&&t.numel==rows )
      prev = t.bf16_bits();
  }

  std::vector<int8_t> q(size_t(rows)*cols);
  std::vector<uint16_t> sc(rows);
  for( uint i = 0; i<rows; i++ ) {
    const float* row = w+size_t(i)*cols;
    fx2::quantize_row_int4(row, int(cols), &q[size_t(i)*cols], &sc[i],
                           prev ? prev[i] : uint16_t(0));
  }
  b.add(name+".q", fx2::DT_I8, {rows, cols}, q.data());
  b.add(sname, fx2::DT_BF16, {rows}, sc.data());
}

// The reverse, and the one place that knows a matrix can arrive either way.
// False if the file holds neither form, which the callers turn into a
// missing-tensor error.
static bool read_matrix(const WeightsFile& wf, const std::string& name,
                        uint rows, uint cols, std::vector<float>& out) {
  out.assign(size_t(rows)*cols, 0.0f);
  if( wf.has(name) ) {
    const WTensor& t = wf.get(name, DT_F32, {rows, cols});
    memcpy(out.data(), t.f32(), out.size()*sizeof(float));
    return true;
  }
  if( wf.has(name+".q")&&wf.has(name+".scale") ) {
    const WTensor& q = wf.get(name+".q", fx2::DT_I8, {rows, cols});
    const WTensor& s = wf.get(name+".scale", fx2::DT_BF16, {rows});
    const int8_t* qp = q.i8();
    const uint16_t* sb = s.bf16_bits();
    for( uint i = 0; i<rows; i++ ) {
      const float f = fx2::bf16_to_f32(sb[i]);
      for( uint c = 0; c<cols; c++ )
        out[size_t(i)*cols+c] = float(qp[size_t(i)*cols+c])*f;
    }
    return true;
  }
  return false;
}

// What a run may write for each byte: its own model where the byte is both in
// its alphabet and counts as trained, the checkpoint it loaded everywhere that
// one had something, and zero otherwise.
struct SaveMap {
  const int32_t* base_vocab;  // the loaded checkpoint's alphabet, or 0
  int32_t out_vocab[SYMS];    // the union that gets written
  const Alphabet* a;

  void Init(const Alphabet& alpha, const char* cmap, const int32_t* bvocab,
            bool trained) {
    a = &alpha;
    base_vocab = bvocab;
    for( uint b = 0; b<SYMS; b++ )
      out_vocab[b] = (bvocab&&bvocab[b]) || (cmap[b] && trained);
  }
  bool from_model(uint b) const { return a->pack[b]>=0 && out_vocab[b]; }
  bool from_base(uint b) const { return base_vocab && base_vocab[b]; }
};

// [rows][n_sym], packed columns -> [rows][256], byte columns
template <class Arr>
static void save_sym_cols(Builder& b, const std::string& name,
                          const std::vector<Arr>& src, uint rows,
                          const SaveMap& map, const WeightsFile* base,
                          bool q4) {
  std::vector<float> buf(size_t(rows)*SYMS, 0.0f), bp;
  const bool has_base = base&&read_matrix(*base, name, rows, SYMS, bp);
  for( uint c = 0; c<SYMS; c++ ) {
    if( map.from_model(c) ) {
      const uint j = uint(map.a->pack[c]);
      for( uint i = 0; i<rows; i++ )
        buf[size_t(i)*SYMS+c] = src[i].d[j];
    } else if( has_base&&map.from_base(c) ) {
      for( uint i = 0; i<rows; i++ )
        buf[size_t(i)*SYMS+c] = bp[size_t(i)*SYMS+c];
    }
  }
  add_matrix(b, name, buf.data(), rows, SYMS, q4, base);
}

// [rows][cols], no symbol axis: this run's model is the whole story
template <class Arr>
static void save_rows(Builder& b, const std::string& name,
                      const std::vector<Arr>& src, uint rows, uint cols,
                      const WeightsFile* base, bool q4) {
  std::vector<float> buf(size_t(rows)*cols);
  for( uint i = 0; i<rows; i++ )
    memcpy(&buf[size_t(i)*cols], src[i].d, cols*sizeof(float));
  add_matrix(b, name, buf.data(), rows, cols, q4, base);
}

// [n_sym][cols], packed rows -> [256][cols], byte rows
static void save_sym_rows(Builder& b, const std::string& name,
                          const std::vector<ArrHidden>& src, uint cols,
                          const SaveMap& map, const WeightsFile* base,
                          bool q4) {
  std::vector<float> buf(size_t(SYMS)*cols, 0.0f), bp;
  const bool has_base = base&&read_matrix(*base, name, SYMS, cols, bp);
  for( uint s = 0; s<SYMS; s++ ) {
    if( map.from_model(s) )
      memcpy(&buf[size_t(s)*cols], src[uint(map.a->pack[s])].d,
             cols*sizeof(float));
    else if( has_base&&map.from_base(s) )
      memcpy(&buf[size_t(s)*cols], &bp[size_t(s)*cols], cols*sizeof(float));
  }
  add_matrix(b, name, buf.data(), SYMS, cols, q4, base);
}

static void save_gate(Builder& b, const char* prefix, const Mixer& g, uint cells,
                      uint hidden, const SaveMap& map, const WeightsFile* base,
                      bool with_optimizer, bool q4) {
  const std::string p(prefix);
  b.add(p+".A", DT_F32, {cells}, g.A_);
  b.add(p+".B", DT_F32, {cells}, g.B_);
  b.add(p+".C", DT_F32, {cells}, g.C_);
  save_sym_cols(b, p+".sym_w", g.sym_w_, cells, map, base, q4);
  save_sym_cols(b, p+".ppmd_w", g.ppmd_w_, cells, map, base, q4);
  save_rows(b, p+".rec_w", g.rec_w_, cells, hidden, base, q4);
  if( !with_optimizer )
    return;
  // the variances stay f32 whatever the weights do - see the note at the top
  b.add(p+".A.v", DT_F32, {cells}, g.A_v_);
  b.add(p+".B.v", DT_F32, {cells}, g.B_v_);
  b.add(p+".C.v", DT_F32, {cells}, g.C_v_);
  save_sym_cols(b, p+".sym_w.v", g.sym_v_, cells, map, base, false);
  save_sym_cols(b, p+".ppmd_w.v", g.ppmd_v_, cells, map, base, false);
  save_rows(b, p+".rec_w.v", g.rec_v_, cells, hidden, base, false);
}

// Writes everything a later run needs to continue from this model.  Not
// written, because it is state of one pass over one file rather than of the
// model: the cell state and the hidden vector, the per-epoch histories the
// truncated backpropagation walks, the gradient accumulators, and PPMD and the
// mixer, which both adapt from scratch anyway.
//
// Nor AdamW's FIRST moments.  beta1 is 0.024 (lstm_layer.inc), so a restored
// m is 2.4% of itself after one step and gone after two; measured, storing
// them costs a third of the file and changes the result by one byte in
// 18000.  Only the second moments and the step counters are worth carrying.
//
// `base` is the checkpoint this run started from, or 0; see the alphabet note
// at the top.  `trained` says whether this build ran any gradient step, and so
// whether the columns for this file's own bytes are worth anything.
static bool Save(const char* path, const Lstm& lstm, const char* cmap,
                 bool with_optimizer, bool trained, bool q4,
                 const WeightsFile* base) {
  Alphabet a;
  a.Init(cmap);

  const int32_t* bvocab = 0;
  if( base&&base->has("lstm.vocab") ) {
    const WTensor& t = base->tensors.find("lstm.vocab")->second;
    if( t.dtype==DT_I32&&t.numel==SYMS )
      bvocab = t.i32();
  }
  SaveMap map;
  map.Init(a, cmap, bvocab, trained);

  Builder b;
  const uint cells = lstm.num_cells_, hidden = lstm.hidden_size_;

  uint n_trained = 0;
  for( uint i = 0; i<SYMS; i++ )
    n_trained += map.out_vocab[i]!=0;

  int32_t cfg[CFG_COUNT] = {0};
  cfg[CFG_VERSION] = FORMAT_VERSION;
  cfg[CFG_NUM_CELLS] = int32_t(cells);
  cfg[CFG_HIDDEN_SIZE] = int32_t(hidden);
  cfg[CFG_HORIZON] = int32_t(lstm.horizon_);
  cfg[CFG_UPDATE_LIMIT] = int32_t(lstm.UPDATE_LIMIT);
  cfg[CFG_N_SYM] = int32_t(n_trained);
  cfg[CFG_HAS_OPTIMIZER] = with_optimizer;
  b.add("lstm.config", DT_I32, {uint32_t(CFG_COUNT)}, cfg);

  // what the run that wrote this was configured with.  Informational: a load
  // does not override the build it is loaded into, it only says so.
  const float hyper[2] = { lstm.learning_rate_, lstm.gradient_clip_ };
  b.add("lstm.hyper", DT_F32, {2}, hyper);

  // The position on AdamW's learning-rate schedule.  It travels with the
  // second moments and only with them: restoring a saturated step count next
  // to a variance that has not been accumulated makes the bias correction
  // divide by almost nothing, and the opening steps come out some 40x too
  // large - which undoes the very weights the file was loaded for.
  if( with_optimizer ) {
    int32_t steps[4];
    put_u64(steps+0, lstm.update_steps_);
    put_u64(steps+2, lstm.o_update_steps_);
    b.add("lstm.steps", DT_I32, {4}, steps);
  }

  b.add("lstm.vocab", DT_I32, {SYMS}, map.out_vocab);

  save_gate(b, "lstm.forget_gate", lstm.forget_gate_, cells, hidden, map, base,
            with_optimizer, q4);
  save_gate(b, "lstm.input_node", lstm.input_node_, cells, hidden, map, base,
            with_optimizer, q4);
  save_gate(b, "lstm.output_gate", lstm.output_gate_, cells, hidden, map, base,
            with_optimizer, q4);

  // output_layer_ is a rolling per-epoch history: Update() writes
  // output_layer_[epoch_] from output_layer_[last_ep] and Mix() reads it back
  // before advancing epoch_, so the newest copy is the one behind epoch_.
  const uint ep = lstm.epoch_==0 ? lstm.horizon_-1 : lstm.epoch_-1;
  save_sym_rows(b, "lstm.output_layer", lstm.output_layer_[ep], hidden, map,
                base, q4);
  if( with_optimizer )
    save_sym_rows(b, "lstm.output_layer.v", lstm.output_v_, hidden, map, base,
                  false);

  return fx2::write_weights_v2(path, b.wf, b.wf.order);
}

// ---------------------------------------------------------------- reading --

// Every loader reports a tensor it could not find, so that a checkpoint
// written by a different version degrades loudly instead of quietly leaving
// part of the model at its initial values.
static bool load_sym_cols_(const WeightsFile& wf, const std::string& name,
                           float** rowptr, uint rows, const Alphabet& a,
                           const int32_t* fvocab, bool required,
                           std::string* missing) {
  std::vector<float> src;
  if( !read_matrix(wf, name, rows, SYMS, src) ) {
    if( required )
      *missing = name;
    return !required;
  }
  for( uint i = 0; i<rows; i++ )
    for( uint c = 0; c<SYMS; c++ )
      if( a.pack[c]>=0&&fvocab[c] )
        rowptr[i][a.pack[c]] = src[size_t(i)*SYMS+c];
  return true;
}

template <class Arr>
static bool load_sym_cols(const WeightsFile& wf, const std::string& name,
                          std::vector<Arr>& dst, uint rows, const Alphabet& a,
                          const int32_t* fvocab, bool required,
                          std::string* missing) {
  std::vector<float*> rp(rows);
  for( uint i = 0; i<rows; i++ )
    rp[i] = dst[i].d;
  return load_sym_cols_(wf, name, rp.data(), rows, a, fvocab, required,
                        missing);
}

template <class Arr>
static bool load_rows(const WeightsFile& wf, const std::string& name,
                      std::vector<Arr>& dst, uint rows, uint cols,
                      bool required, std::string* missing) {
  std::vector<float> src;
  if( !read_matrix(wf, name, rows, cols, src) ) {
    if( required )
      *missing = name;
    return !required;
  }
  for( uint i = 0; i<rows; i++ )
    memcpy(dst[i].d, &src[size_t(i)*cols], cols*sizeof(float));
  return true;
}

static bool load_vec(const WeightsFile& wf, const std::string& name, float* dst,
                     uint n, bool required, std::string* missing) {
  if( !wf.has(name) ) {
    if( required )
      *missing = name;
    return !required;
  }
  const WTensor& t = wf.get(name, DT_F32, {n});
  memcpy(dst, t.f32(), n*sizeof(float));
  return true;
}

static bool load_sym_rows(const WeightsFile& wf, const std::string& name,
                          std::vector<ArrHidden>& dst, uint cols,
                          const Alphabet& a, const int32_t* fvocab,
                          bool required, std::string* missing) {
  std::vector<float> src;
  if( !read_matrix(wf, name, SYMS, cols, src) ) {
    if( required )
      *missing = name;
    return !required;
  }
  for( uint s = 0; s<SYMS; s++ )
    if( a.pack[s]>=0&&fvocab[s] )
      memcpy(dst[a.pack[s]].d, &src[size_t(s)*cols], cols*sizeof(float));
  return true;
}

static bool load_gate(const WeightsFile& wf, const char* prefix, Mixer& g,
                      uint cells, uint hidden, const Alphabet& a,
                      const int32_t* fvocab, bool want_opt,
                      std::string* missing) {
  const std::string p(prefix);
  return load_vec(wf, p+".A", g.A_, cells, true, missing)&&
         load_vec(wf, p+".B", g.B_, cells, true, missing)&&
         load_vec(wf, p+".C", g.C_, cells, true, missing)&&
         load_sym_cols(wf, p+".sym_w", g.sym_w_, cells, a, fvocab, true,
                       missing)&&
         load_sym_cols(wf, p+".ppmd_w", g.ppmd_w_, cells, a, fvocab, true,
                       missing)&&
         load_rows(wf, p+".rec_w", g.rec_w_, cells, hidden, true, missing)&&
         load_vec(wf, p+".A.v", g.A_v_, cells, want_opt, missing)&&
         load_vec(wf, p+".B.v", g.B_v_, cells, want_opt, missing)&&
         load_vec(wf, p+".C.v", g.C_v_, cells, want_opt, missing)&&
         load_sym_cols(wf, p+".sym_w.v", g.sym_v_, cells, a, fvocab, want_opt,
                       missing)&&
         load_sym_cols(wf, p+".ppmd_w.v", g.ppmd_v_, cells, a, fvocab, want_opt,
                       missing)&&
         load_rows(wf, p+".rec_w.v", g.rec_v_, cells, hidden, want_opt,
                   missing);
}

// True if the file starts with a magic the fx2 decoder accepts (FX2TFWC1 or
// FX2TFWC2).  Worth checking first: the decoder exits the process on a file it
// cannot parse, and a caller that named the wrong file deserves a message.
static bool LooksLikeWeights(const char* path) {
  FILE* f = fopen(path, "rb");
  if( !f )
    return false;
  char magic[8];
  const bool ok = fread(magic, 1, 8, f)==8&&memcmp(magic, "FX2TFWC", 7)==0;
  fclose(f);
  return ok;
}

// Reads and validates a checkpoint, without touching the model: everything
// here is knowable before the input file's alphabet is, so a bad checkpoint
// can be rejected before the coder has created any output.
static bool Open(WeightsFile& wf, const char* path, uint cells, uint hidden) {
  if( !LooksLikeWeights(path) ) {
    fprintf(stderr, "lstm_io: %s is not an fx2 weights file\n", path);
    return false;
  }

  // the tables the transformer recomputes from rope.inv_freq are of no use
  // here, and generating all 131072 rows of them costs 32 MB and a moment
  fx2::g_rope_rows_limit = 1;
  wf = WeightsFile::load_compressed(path);

  if( !wf.has("lstm.config") ) {
    fprintf(stderr, "lstm_io: %s holds no LSTM (a transformer checkpoint?)\n",
            path);
    return false;
  }
  const WTensor& c = wf.get("lstm.config");
  if( c.dtype!=DT_I32||c.numel<CFG_COUNT ) {
    fprintf(stderr, "lstm_io: %s: malformed lstm.config\n", path);
    return false;
  }
  const int32_t* cfg = c.i32();
  if( cfg[CFG_VERSION]!=FORMAT_VERSION ) {
    fprintf(stderr, "lstm_io: %s: format version %d, this build writes %d\n",
            path, cfg[CFG_VERSION], FORMAT_VERSION);
    return false;
  }
  if( uint(cfg[CFG_NUM_CELLS])!=cells||uint(cfg[CFG_HIDDEN_SIZE])!=hidden ) {
    fprintf(stderr, "lstm_io: %s: %d cells, this build has %u\n", path,
            cfg[CFG_NUM_CELLS], cells);
    return false;
  }
  if( !wf.has("lstm.vocab") ) {
    fprintf(stderr, "lstm_io: %s: no lstm.vocab\n", path);
    return false;
  }
  const WTensor& v = wf.get("lstm.vocab");
  if( v.dtype!=DT_I32||v.numel!=SYMS ) {
    fprintf(stderr, "lstm_io: %s: malformed lstm.vocab\n", path);
    return false;
  }
  return true;
}

// Overwrites the parameters of an already-Init()ed LSTM with the checkpoint
// Open() read.  Init() must have run for the CURRENT alphabet first: every
// column this checkpoint has nothing to say about keeps the value Init() gave
// it.
static bool Apply(Lstm& lstm, const WeightsFile& wf, const char* cmap,
                  const char* path) {
  const int32_t* cfg = wf.get("lstm.config").i32();
  const int32_t* fvocab = wf.get("lstm.vocab").i32();
  const uint cells = lstm.num_cells_, hidden = lstm.hidden_size_;
  const bool want_opt = cfg[CFG_HAS_OPTIMIZER]!=0;

  Alphabet a;
  a.Init(cmap);

  std::string missing;
  const bool ok =
      load_gate(wf, "lstm.forget_gate", lstm.forget_gate_, cells, hidden, a,
                fvocab, want_opt, &missing)&&
      load_gate(wf, "lstm.input_node", lstm.input_node_, cells, hidden, a,
                fvocab, want_opt, &missing)&&
      load_gate(wf, "lstm.output_gate", lstm.output_gate_, cells, hidden, a,
                fvocab, want_opt, &missing)&&
      load_sym_rows(wf, "lstm.output_layer", lstm.output_layer_[0], hidden, a,
                    fvocab, true, &missing)&&
      load_sym_rows(wf, "lstm.output_layer.v", lstm.output_v_, hidden, a, fvocab,
                    want_opt, &missing);
  if( !ok ) {
    fprintf(stderr, "lstm_io: %s: no %s\n", path, missing.c_str());
    return false;
  }

  // one saved output layer, copied into every epoch slot: that is the state
  // Update()'s rolling copy would have left behind anyway
  for( uint ep = 1; ep<lstm.horizon_; ep++ )
    for( uint s = 0; s<a.n_sym; s++ )
      memcpy(lstm.output_layer_[ep][s].d, lstm.output_layer_[0][s].d,
             hidden*sizeof(float));

  if( want_opt&&wf.has("lstm.steps") ) {
    const WTensor& t = wf.get("lstm.steps", DT_I32, {4});
    lstm.update_steps_ = get_u64(t.i32()+0);
    lstm.o_update_steps_ = get_u64(t.i32()+2);
  }

  // settings the checkpoint was made with.  They do not override this build -
  // the command line and the constants in coder0.cpp win - but a silent
  // mismatch would re-time AdamW's schedule against a step count accumulated
  // under a different one, so say so.
  if( wf.has("lstm.hyper") ) {
    const WTensor& t = wf.get("lstm.hyper", DT_F32, {2});
    if( t.f32()[0]!=lstm.learning_rate_||t.f32()[1]!=lstm.gradient_clip_ )
      fprintf(stderr,
              "lstm_io: %s was trained with lr %g clip %g, this build uses "
              "%g / %g\n",
              path, t.f32()[0], t.f32()[1], lstm.learning_rate_,
              lstm.gradient_clip_);
  }
  if( uint(cfg[CFG_HORIZON])!=lstm.horizon_||
      uint(cfg[CFG_UPDATE_LIMIT])!=lstm.UPDATE_LIMIT )
    fprintf(stderr,
            "lstm_io: %s was trained with horizon %d limit %d, this build uses "
            "%u / %u\n",
            path, cfg[CFG_HORIZON], cfg[CFG_UPDATE_LIMIT], lstm.horizon_,
            lstm.UPDATE_LIMIT);

  uint shared = 0;
  for( uint i = 0; i<SYMS; i++ )
    shared += (a.pack[i]>=0&&fvocab[i]);
  fprintf(stderr,
          "coder0: loaded %s (%s, %d cells, trained on %d byte values, %u of "
          "this file's %u%s)\n",
          path, wf.has("lstm.forget_gate.sym_w.q") ? "q4" : "f32",
          cfg[CFG_NUM_CELLS], cfg[CFG_N_SYM], shared, a.n_sym,
          want_opt ? ", with the optimizer state" : "");
  return true;
}

}  // namespace lstm_io

// A hazard worth writing down.  UnifiedModel::UpdateLSTM in lstm.inc has an
// "#if 0" that feeds Mix() the raw 256-wide PPMD distribution instead of the
// packed one.  Flipping it re-indexes every ppmd_w_ column from packed to raw
// byte, which is a different meaning for the same tensor, and nothing in a
// checkpoint records which side it was written under.  Anyone flipping it
// should bump FORMAT_VERSION, or spend CFG_RESERVED on a feature bit.
