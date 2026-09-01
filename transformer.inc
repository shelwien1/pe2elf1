// ===========================================================================
// transformer.inc - pretrained-transformer byte model for coder0.
//
// Drop-in replacement for lstm.inc / lstm_layer.inc: the same per-byte
// contract (feed the byte that was just coded plus PPMD's distribution over
// the next byte, get back a distribution over the next byte), but instead of
// an LSTM trained on the fly it runs the frozen 6M-parameter transformer that
// cmix's fx2-cmix-transformer submission uses, loaded from the quantized
// weights file 6m-q4-fp32.tfwc2.
//
// The engine itself lives in tf/ (ported unchanged from that submission apart
// from the portability fixes noted in tf/README); this file is the glue:
// weights loading, the byte <-> token mapping, context (article) splitting and
// the two probability vectors that cross the boundary.
// ===========================================================================

// ---------------------------------------------------------------------------
// Configuration switches (override with -D on the command line).
//
//   TF_FP32          1  dequantize the weights to fp32 once at load and run
//                       the model in plain floating point, with no activation
//                       quantization anywhere (default).  Every parameter is
//                       then an ordinary float in one contiguous block that
//                       can be copied out and back with a memcpy.
//                    0  run the quantized AVX2 engine instead (int4 weights,
//                       int8 activations): several times faster, but the
//                       weights only exist pre-packed into its arenas.
//
//   TF_LOAD_WEIGHTS  1  load the pretrained weights file (default)
//                    0  initialize the weights in memory instead, from
//                       TF_INIT_SEED - no weights file needed.  The
//                       initialization is described in tf/weights_init.cpp;
//                       it is deterministic, so the encoder and the decoder
//                       build the same model.
//
//   TF_TRAIN         0  frozen model: the engine's own output layer (default)
//                    1  train the output layer online, once per byte, the way
//                       the LSTM this replaced trained its output layer.
//                       coder0 then runs its own fp32 copy of the unembedding
//                       (started from the model's) on the transformer's final
//                       activation, and updates it with AdamW from the byte
//                       that was just coded.
//                    2  additionally run and train one more transformer block
//                       on top of the frozen ones - the model's own MLP shape,
//                       rms_norm -> up -> relu^2 -> down -> residual - with
//                       backprop through it in fp32.  TF_ADAPTER_D sets its
//                       hidden width.
//
// The four combinations are all valid and all round-trip: the pretrained model
// frozen (1/0), fine-tuned online (1/1), a fresh model trained from scratch
// (0/1) and a fresh frozen model as a control (0/0).
//
// SCOPE: what is trained sits ON TOP of the 12 frozen blocks, not inside them.
// The engine cannot be trained through as it stands - it consumes weights
// pre-packed as int4 nibbles with folded per-row scales, behind fused
// quantization epilogues and hand-written kernels, so a backward pass through
// the body would need fp32 master weights plus a full re-pack of the 2.9 MB
// weight stream after every byte.  The submission has no C++ backward pass to
// reuse either: cpp_infer/src is inference only and its training is PyTorch
// (Muon for the matrices, AdamW for the rest).  So the backward pass here is
// written from scratch, in fp32, over the model's own operations - rms_norm,
// relu-squared, the logit softcap, the residual add - and everything it runs
// on is position-local, which is what makes a per-byte online update exact:
// there is no recurrence to backpropagate through, unlike the LSTM, which
// could only afford full BPTT every `horizon_` bytes.
// ---------------------------------------------------------------------------
#ifndef TF_FP32
#define TF_FP32 1
#endif
#ifndef TF_LOAD_WEIGHTS
#define TF_LOAD_WEIGHTS 1
#endif
#ifndef TF_TRAIN
#define TF_TRAIN 0
#endif
#ifndef TF_INIT_SEED
#define TF_INIT_SEED 0xC0DE0ULL
#endif
#ifndef TF_ADAPTER_D
#define TF_ADAPTER_D 192
#endif

#if TF_FP32
#include "tf/fp32_model.h"
typedef fx2::f32::Transformer32 TFNet;
#else
#include "tf/model_opt.h"
typedef fx2::opt::TransformerOpt TFNet;
#endif

// The two engines expose the same step/begin_article/last_final_norm/
// unembed_rows surface; only construction differs.
static TFNet* TF_MakeNet(const char* path, uint rope_rows) {
#if TF_FP32
  return new fx2::f32::Transformer32(path, rope_rows, TF_INIT_SEED);
#else
  return new fx2::opt::TransformerOpt(path, fx2::opt::AttnKind::KVI8,
                                      rope_rows, TF_INIT_SEED);
#endif
}

// Learning rate (x100000) and the step count the schedule decays over.  The
// schedule is the one the LSTM used for its own output layer (lstm_layer.inc);
// its learning rate (7200) is tuned for a model that starts from noise and is
// too aggressive here, so the default is lower.  Measured on book1[:65536]
// with the pretrained weights: 21085 bytes frozen, 20956 at 7200, 20796 at
// 600, 20772 at 1000, 20770 at 2000.  2000 is also the best of the values
// tried with TF_LOAD_WEIGHTS 0 (book1000 1880 at 2000, 1914 at 7200).
#ifndef TF_LR_X100000
#define TF_LR_X100000 2000
#endif
#ifndef TF_UPDATE_LIMIT
#define TF_UPDATE_LIMIT 3000
#endif

#if TF_TRAIN

#ifdef TF_GRADCHECK
// Set by the gradient check: while non-null, TF_AdamW::Row records the
// gradients it is given instead of applying them, so the weights stay put and
// finite differences are taken around the same point.
static float* tf_gc_capture = 0;
static uint   tf_gc_n = 0;
#endif

// rms_norm, the model's own (SPEC 3.1): y = x / sqrt(mean(x^2) + FLT_EPSILON).
// Returns the denominator, which the backward pass needs.
static float TF_RmsNorm(cprfloat x, prfloat y, uint n) {
  const float eps = 1.1920928955078125e-07f;   // FLT_EPSILON, as in the model
  float ss = 0.0f;
  for( uint i = 0; i<n; i++ )
    ss += x[i]*x[i];
  const float s = sqrtf(ss/n+eps);
  const float inv = 1.0f/s;
  for( uint i = 0; i<n; i++ )
    y[i] = x[i]*inv;
  return s;
}

// dL/dx of the above, from dy, the forward output y and its denominator s:
//   dx = (dy - y * dot(dy,y)/n) / s
static void TF_RmsNormBack(cprfloat dy, cprfloat y, float s, uint n, prfloat dx) {
  float d = 0.0f;
  for( uint i = 0; i<n; i++ )
    d += dy[i]*y[i];
  d /= (float)n;
  const float inv = 1.0f/s;
  for( uint i = 0; i<n; i++ )
    dx[i] = (dy[i]-y[i]*d)*inv;
}

// AdamW over one weight row, with the step-dependent scalars precomputed by
// the caller so they are not recomputed 205 times per byte.
struct TF_AdamW {
  float alpha, b1_t, b2_t;
  static const float beta1() { return 0.024f; }
  static const float beta2() { return 0.9999f; }

  void Prepare(float lr, qword t, uint lim) {
    qword t_eff = t<lim ? t : lim;
    alpha = lr*0.1f/sqrtf(5e-5f*t_eff+1.0f)*(1.0f-(float)t_eff/(lim*1.33f));
    float t_pow = t>0 ? (float)t : 1.0f;
    b1_t = 1.0f-powf(beta1(), t_pow);
    b2_t = 1.0f-powf(beta2(), t_pow);
  }

  void Row(cprfloat g, prfloat m, prfloat v, prfloat w, uint n) const {
    const float eps = 1e-6f, decay = 1e-5f;
#ifdef TF_GRADCHECK
    // Every gradient the backward pass produces passes through here, so this
    // is the one place a test has to tap to compare against finite
    // differences.  See tools/gradcheck.cpp.
    if( tf_gc_capture ) {
      for( uint i = 0; i<n; ++i )
        tf_gc_capture[tf_gc_n++] = g[i];
      return;
    }
#endif
    for( uint i = 0; i<n; ++i ) {
      m[i] = beta1()*m[i]+(1.0f-beta1())*g[i];
      v[i] = beta2()*v[i]+(1.0f-beta2())*g[i]*g[i];
      w[i] -= alpha*((m[i]/b1_t)/(sqrtf(v[i]/b2_t)+eps)+decay*w[i]);
    }
  }
};
#endif

// ---------------------------------------------------------------------------
// The model's vocabulary.
//
// The transformer was trained on exactly the byte stream cmix feeds to its
// PPMD when compressing enwik9: enwik9 after the WRT dictionary transform
// (preprocess/dictionary.cpp) followed by the byte permutation at the end of
// preprocessor::encode_text.  Its 205 tokens are the byte values occurring in
// that stream, in ascending order - cmix's runner.cpp:ExtractVocab makes the
// token id of a byte the number of smaller byte values that occur.
//
// Which byte values occur is almost fully determined by the encoder:
//   * 'A'..'Z' never occur: Dictionary::EncodeWord lower-cases every word and
//     emits a case marker (0x40 / 0x07 / 0x06) instead, so no upper-case
//     letter is ever written literally;
//   * encode_text then permutes the bytes it writes:
//       0x7B..0x7E <-> 0x50..0x53,  0x3A..0x3F <-> 0x4A..0x4F,  0x58 <-> 0x60
//     (TF_Permute below), which moves six absent upper-case values into
//     0x3A..0x3F and 0x7B..0x7E and brings ':'..'?', '{'..'~' and '`' into
//     ranges that would otherwise be upper case;
//   * 0x20..0x39, 0x40, 0x5B..0x5F and 0x61..0x7A all occur in 10^9 bytes of
//     text, as do all 128 values >= 0x80 (dictionary codes plus escaped
//     UTF-8 bytes);
//   * eight values below 0x20 occur (block-header bytes, '\n' and the WRT
//     markers).  Only their count matters here, and only five of them are
//     below '\n'.
// That adds up to 8 + 26 + 1 + 6 + 4 + 1 + 5 + 26 + 128 = 205 exactly.
//
// The reconstruction is pinned by the four anchors cmix asserts in
// runner.cpp:SaveArticleBoundaries, where the encoding of the article
// separator "  <page>\n    <title>" must map to the token ids
//     08 08 25 ac 65 27 05 08 08 08 08 25 ac 68 27
// giving token(' ') = 0x08, token('<'->0x4C) = 0x25, token('>'->0x4E) = 0x27
// and token('\n') = 0x05.  TF_Vocab::SelfCheck() re-derives all four from the
// table below.
//
// Measured on the first 20000 bytes of book1 with a flat prior, this mapping
// costs 3.73 bits/byte against 7.19 for the LSTM-style "packed alphabet index"
// mapping and 8.33 for a deliberately scrambled one - i.e. the pretrained
// weights only mean anything under it.
// ---------------------------------------------------------------------------

// The byte permutation applied by preprocessor::encode_text, verbatim.
static int TF_Permute(int c) {
  if( c>='{' && c<127 ) c += 'P'-'{';
  else if( c>='P' && c<'T' ) c -= 'P'-'{';
  else if( (c>=':' && c<='?') || (c>='J' && c<='O') ) c ^= 0x70;
  if( c=='X' || c=='`' ) c ^= 'X'^'`';
  return c;
}

// Token id of a byte value of the preprocessed stream, or -1 if that value
// does not occur in it.  The eight values below 0x20 are deliberately left
// unmapped except '\n': their identities are not recoverable, only the counts
// (five below '\n', eight below ' '), which is all the ids of everything else
// depends on.  Tokens 0..4, 6 and 7 are therefore never produced here; bytes
// that need them get a spare token instead.
static int TF_TokenOfStreamByte(int b) {
  if( b==0x0A ) return 5;                       // '\n'
  if( b>=0x20 && b<=0x39 ) return  8 + (b-0x20);  // ' ' .. '9'
  if( b==0x40 )            return 34;             // WRT "capitalized" marker
  if( b>=0x4A && b<=0x4F ) return 35 + (b-0x4A);  // permuted ':' .. '?'
  if( b>=0x50 && b<=0x53 ) return 41 + (b-0x50);  // permuted '{' .. '~'
  if( b==0x58 )            return 45;             // permuted '`'
  if( b>=0x5B && b<=0x5F ) return 46 + (b-0x5B);  // '[' .. '_'
  if( b>=0x61 && b<=0x7A ) return 51 + (b-0x61);  // 'a' .. 'z'
  if( b>=0x80 )            return 77 + (b-0x80);  // dictionary codes
  return -1;
}

// ---------------------------------------------------------------------------

struct Transformer {
  enum {
    TF_V = 205,           // the model's vocabulary size
    TF_D = 192,           // the model's residual width
    TF_MAX_PIECE = 131072 // rope table length = longest independent context
  };

  TFNet* net_;
  int   byte2tok_[256];   // -1 for bytes outside the file's alphabet
  int   tok_list_[TF_V];  // the n_tokens_ token ids actually in use
  uint  piece_len_;       // tokens per independent transformer context
  uint  piece_pos_;       // tokens already fed into the current context
  uint  n_tokens_;        // alphabet size (== n_chars)
  ALIGN(64) float prior_[TF_V];
  ALIGN(64) float out_[TF_V];
#if TF_TRAIN
  // coder0's own output layer: an fp32 copy of the model's unembedding rows,
  // trained online.  Only the rows of tokens the file uses are ever touched.
  ALIGN(64) float head_w_[TF_V*TF_D];
  ALIGN(64) float head_m_[TF_V*TF_D];
  ALIGN(64) float head_v_[TF_V*TF_D];
  ALIGN(64) float head_l_[TF_V];    // post-softcap logits of the last forward
  ALIGN(64) float prev_xn_[TF_D];   // the block's input: the model's final norm
  uint  pending_;                   // a prediction is waiting for its target
  qword head_steps_;
#endif
#if TF_TRAIN>=2
  // one more transformer block, in the model's own MLP shape and trained in
  // fp32:  h2 = h + down(relu(up(rms_norm(h)))^2),  head input = rms_norm(h2).
  // down starts at zero, so the block is the identity at step 0 and level 2
  // begins exactly where level 1 does.
  ALIGN(64) float up_w_[TF_ADAPTER_D*TF_D], up_m_[TF_ADAPTER_D*TF_D],
                  up_v_[TF_ADAPTER_D*TF_D];
  ALIGN(64) float dn_w_[TF_D*TF_ADAPTER_D], dn_m_[TF_D*TF_ADAPTER_D],
                  dn_v_[TF_D*TF_ADAPTER_D];
  // forward values the backward pass needs, from the last step
  ALIGN(64) float a_n1_[TF_D], a_u_[TF_ADAPTER_D], a_r_[TF_ADAPTER_D];
  ALIGN(64) float a_h2_[TF_D], a_n2_[TF_D];
  float a_s2_;
#endif

  Transformer() : net_(0), piece_len_(0), piece_pos_(0), n_tokens_(0) {}

  ~Transformer() { delete net_; net_ = 0; }

  // Re-derives the four separator anchors of runner.cpp from the table above.
  static bool SelfCheck() {
    return TF_TokenOfStreamByte(0x0A)==0x05 && TF_TokenOfStreamByte(0x20)==0x08
        && TF_TokenOfStreamByte(0x4C)==0x25 && TF_TokenOfStreamByte(0x4E)==0x27;
  }

  // Builds the byte -> token map for the file's alphabet.  Bytes whose
  // preprocessed-stream value does not occur in the model's vocabulary (the
  // upper-case letters above all, but also DEL and most control codes) get a
  // spare token from the still-unclaimed ones, in ascending byte order, so the
  // map stays injective.  Both directions of the coder derive it from cmap,
  // which is coded before any byte, so encoder and decoder always agree.
  bool BuildMap(const char* cmap) {
    bool taken[TF_V];
    uint i;
    for( i = 0; i<TF_V; i++ )
      taken[i] = false;
    n_tokens_ = 0;
    for( i = 0; i<TF_V; i++ )
      tok_list_[i] = 0;
    for( i = 0; i<256; i++ ) {
      byte2tok_[i] = -1;
      if( !cmap[i] )
        continue;
      ++n_tokens_;
      int t = TF_TokenOfStreamByte(TF_Permute(i));
      if( t>=0 && !taken[t] ) {
        byte2tok_[i] = t;
        taken[t] = true;
      }
    }
    if( n_tokens_>TF_V )
      return false;               // alphabet too large for this model
    // Spare tokens are taken from the dictionary-code half of the vocabulary
    // (77..204 are the stream's byte values >= 0x80, i.e. the WRT word codes),
    // which measures ~0.2% better than handing out the unclaimed low tokens -
    // those are the model's rare control and marker symbols.  The scan wraps,
    // so a map always exists as long as the alphabet fits in 205 tokens.
    const uint kSpareBase = 128;
    uint spare = kSpareBase, wrapped = 0;
    for( i = 0; i<256; i++ ) {
      if( !cmap[i] || byte2tok_[i]>=0 )
        continue;
      for(;;) {
        if( spare>=TF_V ) {
          if( wrapped )
            return false;         // unreachable while n_tokens_ <= TF_V
          wrapped = 1;
          spare = 0;
        }
        if( !taken[spare] )
          break;
        ++spare;
      }
      byte2tok_[i] = spare;
      taken[spare] = true;
    }
    for( i = 0, n_tokens_ = 0; i<256; i++ )
      if( byte2tok_[i]>=0 )
        tok_list_[n_tokens_++] = byte2tok_[i];
    return true;
  }

  // Prepares the mapping and brings up the model: weights_path names the
  // weights file, or is NULL to initialize them in memory (TF_LOAD_WEIGHTS 0).
  // Returns false (with the model left disabled) if the alphabet does not fit,
  // so the caller can fall back to PPMD alone rather than fail.
  bool Init(const char* weights_path, const char* cmap, uint f_len) {
    if( !SelfCheck() )
      return false;
    if( !BuildMap(cmap) )
      return false;
    piece_len_ = f_len<TF_MAX_PIECE ? (f_len ? f_len : 1) : TF_MAX_PIECE;
    piece_pos_ = 0;
    // Only piece_len_ rope positions are ever addressed, so only that many
    // rows of the sin/cos tables are built at load time (the full tables are
    // 32 MB and take ~0.2s).
    net_ = TF_MakeNet(weights_path, piece_len_);
    for( uint i = 0; i<TF_V; i++ )
      prior_[i] = out_[i] = 0.0f;
#if TF_TRAIN
    // Start the trainable head from the model's own unembedding, so training
    // begins where the frozen model would have been rather than from noise.
    memcpy(head_w_, net_->unembed_rows(), sizeof(head_w_));
    memset(head_m_, 0, sizeof(head_m_));
    memset(head_v_, 0, sizeof(head_v_));
    memset(head_l_, 0, sizeof(head_l_));
    memset(prev_xn_, 0, sizeof(prev_xn_));
    pending_ = 0;
    head_steps_ = 0;
#endif
#if TF_TRAIN>=2
    // up: kaiming_uniform over fan_in = TF_D, the reference's init for an mlp
    // up projection.  down: zero, so the block starts as the identity.
    {
      uint64_t st = TF_INIT_SEED ^ 0x5DEECE66Dull;
      const float bound = sqrtf(6.0f/(float)TF_D);
      for( uint i = 0; i<TF_ADAPTER_D*TF_D; i++ ) {
        uint64_t z = (st += 0x9E3779B97F4A7C15ull);
        z = (z^(z>>30))*0xBF58476D1CE4E5B9ull;
        z = (z^(z>>27))*0x94D049BB133111EBull;
        z ^= z>>31;
        up_w_[i] = bound*(2.0f*(float(z>>40)*(1.0f/16777216.0f))-1.0f);
      }
    }
    memset(up_m_, 0, sizeof(up_m_));
    memset(up_v_, 0, sizeof(up_v_));
    memset(dn_w_, 0, sizeof(dn_w_));
    memset(dn_m_, 0, sizeof(dn_m_));
    memset(dn_v_, 0, sizeof(dn_v_));
    memset(a_n1_, 0, sizeof(a_n1_));
    memset(a_u_, 0, sizeof(a_u_));
    memset(a_r_, 0, sizeof(a_r_));
    memset(a_h2_, 0, sizeof(a_h2_));
    memset(a_n2_, 0, sizeof(a_n2_));
    a_s2_ = 1.0f;
#endif
    return true;
  }

#if TF_TRAIN
  // What the output layer reads: the model's final activation at level 1, the
  // extra block's output at level 2.
  cprfloat HeadInput() const {
#if TF_TRAIN>=2
    return a_n2_;
#else
    return prev_xn_;
#endif
  }

  // Forward pass of everything coder0 owns, on the transformer's final
  // activation: at level 2 the extra block, then the output layer - logits,
  // the model's logit softcap, and a softmax over the tokens the file uses.
  // Leaves the post-softcap logits in head_l_ and the distribution in out_,
  // plus every intermediate the backward pass needs.
  void StackForward() {
    uint j, k;
#if TF_TRAIN>=2
    TF_RmsNorm(prev_xn_, a_n1_, TF_D);
    for( k = 0; k<TF_ADAPTER_D; k++ ) {
      const float u = DotProduct(up_w_+k*TF_D, a_n1_, TF_D);
      a_u_[k] = u;
      const float t = u>0.0f ? u : 0.0f;   // relu squared, the model's mlp
      a_r_[k] = t*t;
    }
    for( j = 0; j<TF_D; j++ )
      a_h2_[j] = prev_xn_[j] +
                 DotProduct(dn_w_+j*TF_ADAPTER_D, a_r_, TF_ADAPTER_D);
    a_s2_ = TF_RmsNorm(a_h2_, a_n2_, TF_D);
#endif
    cprfloat hin = HeadInput();
    float m = -1e30f;
    for( k = 0; k<n_tokens_; k++ ) {
      const int o = tok_list_[k];
      const float z = DotProduct(head_w_+(uint)o*TF_D, hin, TF_D);
      const float l = 15.0f*tanhf(z*(1.0f/15.0f));
      head_l_[o] = l;
      if( l>m )
        m = l;
    }
    float sum = 0.0f;
    for( k = 0; k<n_tokens_; k++ ) {
      const int o = tok_list_[k];
      sum += (out_[o] = expf(head_l_[o]-m));
    }
    const float inv = 1.0f/(sum+1e-30f);
    for( k = 0; k<n_tokens_; k++ )
      out_[tok_list_[k]] *= inv;
  }

  // One online step, from the target that has just arrived.  The loss is
  // -log p[target] over the used tokens, so
  //   dL/dz = (p - onehot) * dsoftcap/dz,
  //   dsoftcap/dz = 1 - tanh(z/15)^2 = 1 - (l/15)^2   with l the capped logit.
  // Every gradient below is read off the pre-update weights before that weight
  // is touched, and every operation is position-local, so the update is the
  // exact gradient of this byte's loss - no truncation, unlike BPTT.
  void StackUpdate(int target) {
    uint j, k;
    TF_AdamW opt;
    opt.Prepare(TF_LR_X100000/100000.0f, head_steps_/10+1, TF_UPDATE_LIMIT*2);
    cprfloat hin = HeadInput();
    ALIGN(64) float dz[TF_V];
    for( k = 0; k<n_tokens_; k++ ) {
      const int o = tok_list_[k];
      const float t = head_l_[o]*(1.0f/15.0f);
      dz[o] = (out_[o]-(o==target ? 1.0f : 0.0f))*(1.0f-t*t);
    }
#if TF_TRAIN>=2
    // dL/d(head input), with the head weights as they were in the forward pass
    ALIGN(64) float dn2[TF_D];
    for( j = 0; j<TF_D; j++ )
      dn2[j] = 0.0f;
    for( k = 0; k<n_tokens_; k++ ) {
      const int o = tok_list_[k];
      cprfloat w = head_w_+(uint)o*TF_D;
      const float g = dz[o];
      for( j = 0; j<TF_D; j++ )
        dn2[j] += w[j]*g;
    }
#endif
    ALIGN(64) float grad[TF_D>TF_ADAPTER_D ? TF_D : TF_ADAPTER_D];
    for( k = 0; k<n_tokens_; k++ ) {
      const int o = tok_list_[k];
      const float g = dz[o];
      for( j = 0; j<TF_D; j++ )
        grad[j] = g*hin[j];
      opt.Row(grad, head_m_+(uint)o*TF_D, head_v_+(uint)o*TF_D,
              head_w_+(uint)o*TF_D, TF_D);
    }
#if TF_TRAIN>=2
    // back through the final rms_norm, then the residual: dh2 reaches both the
    // down projection and (unused) the block input, since the body is frozen
    ALIGN(64) float dh2[TF_D];
    TF_RmsNormBack(dn2, a_n2_, a_s2_, TF_D, dh2);
    // dr needs the down weights as they were, so read it before updating them
    ALIGN(64) float dr[TF_ADAPTER_D];
    for( k = 0; k<TF_ADAPTER_D; k++ )
      dr[k] = 0.0f;
    for( j = 0; j<TF_D; j++ ) {
      cprfloat w = dn_w_+j*TF_ADAPTER_D;
      const float g = dh2[j];
      for( k = 0; k<TF_ADAPTER_D; k++ )
        dr[k] += w[k]*g;
    }
    for( j = 0; j<TF_D; j++ ) {
      const float g = dh2[j];
      for( k = 0; k<TF_ADAPTER_D; k++ )
        grad[k] = g*a_r_[k];
      opt.Row(grad, dn_m_+j*TF_ADAPTER_D, dn_v_+j*TF_ADAPTER_D,
              dn_w_+j*TF_ADAPTER_D, TF_ADAPTER_D);
    }
    // d(relu(u)^2)/du = 2*relu(u)
    for( k = 0; k<TF_ADAPTER_D; k++ ) {
      const float du = dr[k]*2.0f*(a_u_[k]>0.0f ? a_u_[k] : 0.0f);
      for( j = 0; j<TF_D; j++ )
        grad[j] = du*a_n1_[j];
      opt.Row(grad, up_m_+k*TF_D, up_v_+k*TF_D, up_w_+k*TF_D, TF_D);
    }
#endif
    ++head_steps_;
  }
#endif

  bool Ready() const { return net_!=0; }

  // Feeds the byte that was just coded together with PPMD's distribution over
  // the next byte, and writes the transformer's distribution over the next
  // byte into probs[256] (zero outside the alphabet).  Returns false at the
  // last token of a context piece: there is no prediction then, and the caller
  // keeps PPMD's distribution for that one byte (this mirrors what cmix does
  // at article boundaries).
  bool Predict(uint sym, cprfloat ppmd_probs, prfloat probs) {
    uint i;
    if( sym>255 || byte2tok_[sym]<0 )
      return false;               // not an alphabet byte (corrupt archive)
#if TF_TRAIN
    // The byte just coded is the target of the prediction made last call.
    if( pending_ ) {
      StackUpdate(byte2tok_[sym]);
      pending_ = 0;
    }
#endif
    if( piece_pos_+1>=piece_len_ ) {
      // Last token of this context: the next byte starts a fresh one, so
      // this token's prediction would be thrown away.  Skip the step, as
      // cmix does at its article boundaries.
      piece_pos_ = 0;
      return false;
    }
    // The prior is PPMD's distribution over the byte that comes NEXT, paired
    // with the byte just coded - the same pairing the model was trained on
    // (cmix writes it with --save-ppmd-probs right after its byte update).
    // cmix additionally rounds it to float16 there, to reproduce the training
    // file pipeline bit-for-bit; that was measured here and is not worth it
    // (book1000 1735 vs 1734, book1[:16384] 6004 vs 6003) - the model's own
    // int8 prior quantizer is far coarser than float16 anyway.
    for( i = 0; i<TF_V; i++ )
      prior_[i] = 0.0f;
    for( i = 0; i<256; i++ )
      if( byte2tok_[i]>=0 )
        prior_[byte2tok_[i]] = ppmd_probs[i];

    if( piece_pos_==0 )
      net_->begin_article(0);
    net_->step((uint8_t)byte2tok_[sym], prior_, out_);
    ++piece_pos_;
#if TF_TRAIN
    // Replace the engine's frozen output layer with the trained one; out_ and
    // head_l_ are what HeadUpdate will need once the target arrives.
    memcpy(prev_xn_, net_->last_final_norm(), sizeof(prev_xn_));
    StackForward();
    pending_ = 1;
#endif

    float sum = 0.0f;
    for( i = 0; i<256; i++ ) {
      float p = 0.0f;
      if( byte2tok_[i]>=0 ) {
        p = out_[byte2tok_[i]];
        // guard against zero/NaN, which would break the log-domain mixer
        if( !(p>1e-6f) )
          p = 1e-6f;
      }
      probs[i] = p;
      sum += p;
    }
    float inv = 1.0f/(sum+1e-12f);
    for( i = 0; i<256; i++ )
      probs[i] *= inv;
    return true;
  }
};

// ---------------------------------------------------------------------------
// PPMD + transformer, with the same interface coder0.cpp used for the LSTM.
// ---------------------------------------------------------------------------

template <typename TfType> struct UnifiedModel {
  ppmd_Model ppmd_model_;
  TfType* tf_;
  char* vocab_;
  float ppmd_probs_[256], tf_probs_[256];

  NOINLINE void Init(int order, int memory, char* vocab, TfType* tf, uint f_len) {
    vocab_ = vocab;
    tf_ = tf;
    ppmd_model_.Init(order, memory, 50, f_len);

    for( uint i = 0; i<256; i++ )
      tf_probs_[i] = ppmd_probs_[i] = vocab_[i] ? 1.0f/256.0f : 0.0f;
    Renorm_probs(ppmd_probs_);
    Renorm_probs(tf_probs_);
  }

  void Renorm_probs(prfloat probs) {
    float sum = SumOfAbs(probs, 256);
    float inv_sum = 1.0f/(sum+1e-12f);
    for( uint i = 0; i<256; ++i )
      probs[i] *= inv_sum;
  }

  NOINLINE void UpdatePPMD(uint byte_val) {
    ppmd_model_.ppmd_UpdateByte(byte_val&0xFF);
    ppmd_model_.ppmd_PrepareByte();

    for( uint i = 0; i<256; ++i )
      ppmd_probs_[i] = vocab_[i] ? (ppmd_model_.sqp[i]<1.0f ? 1.0f : ppmd_model_.sqp[i]) : 0.0f;
    Renorm_probs(ppmd_probs_);
  }

  // Must run after UpdatePPMD(sym): the transformer is fed the byte that was
  // just coded together with PPMD's distribution over the *next* byte, exactly
  // the pairing the model was trained on.
  NOINLINE void UpdateTransformer(uint sym) {
    if( !tf_ || !tf_->Ready() || !tf_->Predict(sym, ppmd_probs_, tf_probs_) )
      memcpy(tf_probs_, ppmd_probs_, sizeof(tf_probs_));
  }
};
