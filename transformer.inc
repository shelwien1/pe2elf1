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

#include "tf/model_opt.h"

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
    TF_MAX_PIECE = 131072 // rope table length = longest independent context
  };

  fx2::opt::TransformerOpt* net_;
  int   byte2tok_[256];   // -1 for bytes outside the file's alphabet
  uint  piece_len_;       // tokens per independent transformer context
  uint  piece_pos_;       // tokens already fed into the current context
  uint  n_tokens_;        // alphabet size (== n_chars)
  ALIGN(64) float prior_[TF_V];
  ALIGN(64) float out_[TF_V];

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
    return true;
  }

  // Loads the weights and prepares the mapping.  Returns false (with the model
  // left disabled) if the file cannot be used, so the caller can fall back to
  // PPMD alone rather than fail.
  bool Init(const char* weights_path, const char* cmap, uint f_len) {
    if( !SelfCheck() )
      return false;
    if( !BuildMap(cmap) )
      return false;
    piece_len_ = f_len<TF_MAX_PIECE ? (f_len ? f_len : 1) : TF_MAX_PIECE;
    piece_pos_ = 0;
    // Only piece_len_ rope positions are ever addressed, so only that many
    // rows of the sin/cos tables are recomputed at load time (the full tables
    // are 32 MB and take ~0.2s to rebuild).
    net_ = new fx2::opt::TransformerOpt(weights_path, fx2::opt::AttnKind::KVI8,
                                        piece_len_);
    for( uint i = 0; i<TF_V; i++ )
      prior_[i] = out_[i] = 0.0f;
    return true;
  }

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
