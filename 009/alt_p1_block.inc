// alt_p1_block.inc -- the alternate model for predictor 1: its block, its context
// and its counters.  "p1" is `plane_predictor == 1` and says nothing about depth --
// measured, this codes t24's and t32's *eight-bit* planes.  MODELS.md section 6.

// The near-lossless drift control, and the whole of what makes these two
// models lossy.  Fold the residual between the source byte and the prediction
// into a symbol, reconstruct from it, and measure how far that pulls the byte
// already standing in the output buffer.  Past sixteen either way the
// approximation is refused: `*dst` takes the source byte unchanged and the
// symbol comes from the high fold instead.  Answers the symbol to code.
//
// Three copies -- the generic-depth arm of the p1 encoder and both of the p2
// encoder's -- and no two spelled it alike.  One wrote `*(fold[resid]+unfold)`
// for the inverse lookup, one indexed `fold_hi` with the residual recomputed
// from scratch rather than with the name three lines above it, and one took
// the drift before naming the symbol while the others did it after.
//
// It lives here, ahead of both blocks, because both call it; `unfold` is
// `uint8_t` in the p1 block and `int8_t` in the p2 one, which is what the
// template parameter is for.  The sum is narrowed to a byte either way, so the
// signedness cannot reach the output.
//
// The two `encode_sample` methods make a *different* decision that looks like
// this one, and the difference is worth stating: there the source byte and the
// buffer byte are the same byte, read twice around the context step, so the
// reconstruction they compare against is not this one.
template<class Unfold>
static inline uint8_t fold_or_refuse(uint8_t* dst, int32_t src, int32_t pred,
                                     const uint8_t* fold,
                                     const uint8_t* fold_hi,
                                     const Unfold* unfold) {
  const int32_t resid = (uint8_t)(src-pred);
  const uint8_t recon = unfold[fold[resid]]+pred;
  const int32_t drift = *dst-(uint8_t)(recon+*dst-src);
  if( drift<-16||drift>16 ) {
    *dst = (uint8_t)src;
    return fold_hi[resid];
  }
  *dst = recon;
  return fold[resid];
}

struct CtxWeight {
  int32_t sel;
  uint32_t w[3];
};

struct P1Ctx {
  uint8_t sym;
  uint8_t mag;
};

// Two shapes `ctx_of` builds from a *neighbouring plane's* row, each of which
// it used in more than one of its three arms.
//
//   `cross_grad`  this plane's west-minus-prediction plus that plane's own
//                 gradient over its last two samples.
//   `nb_resid`    that plane's residual at the sample to the west.
static inline int32_t cross_grad(int32_t here, int32_t guess, const AltP1Block* nb);
static inline int32_t nb_resid(const AltP1Block* nb);

struct AltP1Block {
  int32_t width;
  int32_t height;
  int32_t pred;
  int32_t ctx[5];
  CtxWeight ctx_w[9];
  P1Ctx* buf[5];
  P1Ctx* cursor[5];
  uint8_t level_of[512];
  uint8_t slot_of[256];
  uint8_t fold[256];
  uint8_t fold_hi[256];
  uint8_t unfold[256];
  int32_t group_of[512];
  CounterNode counters[629856];
  // Start a row: mirror the last six records past the right edge, rotate the
  // five row buffers by one, reseat the five cursors, carry the last record of
  // the row above into the left margin, mirror three records back from it, and
  // rebuild the two activity sums the context reads.
  //
  // Twenty-five lines, written out three times -- here, in `alt_p1_code.inc`
  // and in `alt_p1.inc` -- because in the decompilation each of the three
  // callers had its own copy of the loop.
  //
  // The two activity sums that follow it are `seed_activity` below, which took
  // a parameter rather than a fourth copy: the three callers do not agree on
  // how two of the twenty magnitudes are read.  The question this comment used
  // to leave open -- whether that difference can ever show -- is answered
  // there, with the measurement.
  void advance_row() {
    {
      P1Ctx*const here = cursor[0];
      for( int32_t k = 0; k<6; ++k )
        here[k] = here[-1-k];
    }
    ring_advance(buf, cursor, 4);
    cursor[0][-4] = cursor[1][3];
    {
      P1Ctx*const here = cursor[0];
      P1Ctx*const up = cursor[1];
      for( int32_t i = 0; i<3; ++i ) here[-3+i] = up[2-i];
    }
  }

  // The two activity sums the context reads at the start of a row: ten
  // neighbour magnitudes each, taken from rows 2 and 4 -- even offsets for
  // `ctx[3]`, odd for `ctx[4]` -- plus two from the row being written.
  //
  // Three callers wrote this out, and the third is not quite the same as the
  // other two: the generic `alt_model_p1` reads the two *mirrored* magnitudes,
  // the ones `advance_row` copied back past the left edge, through `(int8_t)`
  // where `d8_body` and the decoder it now shares with read the `uint8_t`
  // they are declared.  `advance_row`'s comment left that difference standing
  // as a question about how large a residual magnitude gets.
  //
  // It is answered, and the answer is that the difference is exercised and
  // still does not show.  `mag` is `abs32(val - pred)` of two bytes, so it
  // reaches 255 and the two readings then differ by 256; instrumented over the
  // corpus, the generic coder runs this **18,224** times and **1,212** of those
  // calls read at least one `mag` at or above 128.  Dropping the casts moves no
  // stream even so, because these two sums are read in exactly one place --
  // `ctx[3 + ctx[2]] == 0` -- and are only ever tested against zero.
  //
  // Those two numbers said 3,216 and 70 for as long as the sentence existed,
  // and both were stale by more than 5x.  A measurement written into a comment
  // is a claim with a date on it and nothing here checks this one, which is the
  // same defect `pairshare.py` was built for one directory over; re-measured by
  // counting calls and operands over all nineteen images.  The conclusion is
  // unchanged and was never close -- the sums are compared against zero, so any
  // count above zero settles it identically.
  //
  // So the casts stay as the binary has them and the parameter says which
  // coder is asking, rather than one of the three being quietly changed to
  // agree with the other two.
  void seed_activity(bool signed_mirror) {
    const P1Ctx *const row0 = cursor[0], *const row2 = cursor[2],
                *const row4 = cursor[4];
    ctx[2] = 0;
    for( int32_t lane = 0; lane<2; ++lane ) {
      const int32_t mirror = lane-2;
      int32_t sum = signed_mirror
                  ? (int8_t)row2[mirror].mag+(int8_t)row4[mirror].mag
                  : row2[mirror].mag+row4[mirror].mag;
      for( int32_t k = 0; k<3; ++k )
        sum += row2[lane+2*k].mag+row4[lane+2*k].mag;
      ctx[3+lane] = sum+row0[lane-4].mag+row0[lane-2].mag;
    }
  }

  // Code one sample with the near-lossless drift control, and answer with the
  // value the model should record: fold the residual against the prediction,
  // reconstruct it, and if the reconstruction has drifted more than sixteen
  // from the byte already in the buffer, send the sample through `fold_hi`
  // instead and leave that byte alone.
  //
  // Four copies in `alt_model_p1`, one per plane, and the same shape as
  // `AltP2Block::encode_sample`.  One of the four carried the code under a
  // second name, `code1x`; one computed the drift inline and the byte it
  // compares against afterwards, as a `uint8_t` where the others used an
  // `int32_t` -- the store truncates either way.
  int32_t encode_sample(uint8_t* dst, int32_t at, int32_t cur) {
    const int32_t p = (uint8_t)pred;
    const int32_t resid = (uint8_t)(cur-p);
    int32_t code = fold[resid];
    const int32_t recon = (uint8_t)(unfold[code]+p);
    const int32_t out = (uint8_t)(recon+dst[at]-cur);
    if( dst[at]-out<-16||dst[at]-out>16 ) {
      code = fold_hi[resid];
    } else {
      cur = recon;
      dst[at] = out;
    }
    alt_p1_encode_symbol(&counters[ctx[0]], ctx[1], code);
    return cur;
  }

  // The decode half of `encode_sample`: take the symbol and unfold it against
  // the prediction.  Four copies in `alt_model_p1`, one per plane, and shorter
  // than the encode half by exactly the near-lossless drift control -- the
  // decoder is told which fold was used.
  int32_t decode_sample() {
    const int32_t code = alt_p1_decode_symbol(&counters[ctx[0]], ctx[1]);
    return (uint8_t)((uint8_t)pred+unfold[code]);
  }

  // Record one coded sample: store it and its residual magnitude, roll that
  // magnitude into the activity sum for this parity, flip the parity, let the
  // counter bank rescale if it has room, and step the five cursors.
  //
  // Six copies -- four in `alt_model_p1`, one in `d8_body`, one in
  // `alt_model_p1_d8_decode`.  Four wrote the activity update as
  // `- (a - b) - (c - d)` and two as `- (a - b + c - d)`, which is the same
  // sum; one hoisted `ctx[0]` into a local across the parity flip, which does
  // not touch it.
  void record_sample(int32_t val) {
    cursor[0]->sym = val;
    cursor[0]->mag = abs32(val-pred);
    ctx[3+ctx[2]] += cursor[0]->mag-cursor[0][-4].mag
                    -(cursor[4][-2].mag-cursor[4][6].mag)
                    -(cursor[2][-2].mag-cursor[2][6].mag);
    ctx[2] = ctx[2]==0;
    if( counters[ctx[0]].total<0x4000u )
      update_model();
    for( int32_t k = 0; k<5; ++k )
      ++cursor[k];
  }

  int32_t ctx_of(AltP1Block* nb0, AltP1Block* nb1) {
    bool pick;
    int32_t act_all;
    uint32_t quiet;
    P1Ctx *cursor1 = cursor[1];
    int32_t guess = cursor1->sym;
    P1Ctx *cur = cursor[0];
    int32_t west = cur[-1].sym;
    int32_t northwest = cursor1[-1].sym;
    if( west<guess ) {
      if( northwest>=west ) {
        int32_t plane_b = guess+west-northwest;
        pick = northwest<guess;
        guess = cur[-1].sym;
        if( pick )
          guess = plane_b;
      }
    } else if( northwest<=west ) {
      int32_t plane_a = guess+west-northwest;
      pick = northwest<=guess;
      guess = cur[-1].sym;
      if( !pick )
        guess = plane_a;
    }
    pred = guess;
    P1Ctx *cursor2 = cursor[2];
    int32_t act = cursor1[-1].mag+cur[-3].mag+3*(cursor1[1].mag+cursor2->mag)+6*cur[-1].mag+4*(cursor1->mag+cur[-2].mag)+2*(cursor2[2].mag+cursor1[2].mag+cur[-4].mag);
    // The five selectors below this point are the ones that depend on how many
    // *other planes* this one has to look at -- two, one, or none.  The three
    // arms are genuinely three formulas and not one formula three times, but
    // four shapes recur across them and are named here rather than spelled out
    // seven times between them.
    //
    // `west_grad` and `north_grad` are computed unconditionally: both are pure
    // reads of this plane's own row, and each appeared in two of the three
    // arms.
    const int32_t west_grad = 2*cur[-1].sym-cur[-2].sym-(uint32_t)guess;
    const int32_t north_grad = -guess-cursor1->sym+cursor1[1].sym+cur[-1].sym;
    if( nb0 ) {
      const P1Ctx* r0 = nb0->cursor[0];
      if( nb1 ) {
        const P1Ctx* r1 = nb1->cursor[0];
        act_all = act+2*(r1[-1].mag+r0[-1].mag);
        ctx_w[5].sel = cursor1->sym-(uint32_t)guess+r0[-1].sym-nb0->cursor[1][-1].sym;
        ctx_w[6].sel = cross_grad(cur[-1].sym, guess, nb0);
        ctx_w[7].sel = cross_grad(cur[-1].sym, guess, nb1);
        ctx_w[8].sel = nb_resid(nb1);
        ctx_w[3].sel = nb_resid(nb0);
        quiet = (r1[-1].mag+r0[-1].mag<16);
      } else {
        act_all = cursor1[3].mag+act+3*r0[-1].mag;
        ctx_w[5].sel = west_grad;
        ctx_w[6].sel = west_grad;
        ctx_w[7].sel = north_grad;
        ctx_w[8].sel = cross_grad(cur[-1].sym, guess, nb0);
        ctx_w[3].sel = nb_resid(nb0);
        quiet = (r0[-1].mag<8);
      }
    } else {
      P1Ctx *cursor4 = cursor[4];
      act_all = cursor4->mag+cursor2[-2].mag+cursor1[3].mag+act+cursor4[2].mag;
      ctx_w[5].sel = west_grad;
      ctx_w[6].sel = 2*cursor1->sym-cursor2->sym-(uint32_t)guess;
      ctx_w[7].sel = north_grad;
      ctx_w[8].sel = -3*(cur[-2].sym-cur[-1].sym)+cur[-3].sym-(uint32_t)guess;
      ctx_w[3].sel = cursor1[2].sym-(uint32_t)guess;
      quiet = cur->mag+cursor4->mag+cursor[3]->mag+cursor2->mag+cursor1->mag==0;
    }
    int32_t act_q = (act_all+7)>>4;
    int32_t act_lvl = level_of[act_q];
    int32_t step = p1_level_step[(uint32_t)act_lvl];
    ctx[0] = act_lvl;
    ctx[1] = group_of[act_q]+slot_of[(uint32_t)guess];
    // The same expression twice in the decompilation, once into the slot and
    // once into the index that reads it.  The other eight slots need both --
    // `ctx_w[N].sel` is overwritten further down while `sN` keeps the index --
    // but nothing overwrites slot 0's.
    ctx_w[0].sel = (guess>216)+(guess>22);
    uint32_t s0 = ctx_w[0].sel;
    int32_t s1 = (cursor1[-1].sym-cursor1->sym>=0)+(cursor1[-1].sym>(int32_t)cursor1->sym);
    ctx_w[1].sel = s1;
    int32_t s2 = (cursor1[-1].sym-cur[-1].sym>=0)+(cursor1[-1].sym>(int32_t)cur[-1].sym);
    ctx_w[2].sel = s2;
    bool is_zero = ctx_w[3].sel==0;
    bool is_neg = ctx_w[3].sel<0;
    int32_t s4 = (cursor1[1].sym-guess>=-step)+(cursor1[1].sym-guess>step);
    ctx_w[4].sel = s4;
    int32_t hi = step<ctx_w[5].sel;
    int32_t lo = -step<=ctx_w[5].sel;
    int32_t s3 = !is_neg+(!is_neg&&!is_zero);
    ctx_w[3].sel = s3;
    int32_t s5 = lo+hi;
    is_zero = ctx_w[6].sel==0;
    is_neg = ctx_w[6].sel<0;
    ctx_w[5].sel = s5;
    int32_t g6 = !is_neg&&!is_zero;
    int32_t h6 = !is_neg;
    is_zero = ctx_w[7].sel==0;
    is_neg = ctx_w[7].sel<0;
    int32_t s6 = h6+g6;
    ctx_w[6].sel = h6+g6;
    int32_t g7 = !is_neg&&!is_zero;
    int32_t h7 = !is_neg;
    is_zero = ctx_w[8].sel==0;
    is_neg = ctx_w[8].sel<0;
    int32_t s7 = h7+g7;
    ctx_w[7].sel = h7+g7;
    int32_t s8 = !is_neg+(!is_neg&&!is_zero);
    ctx_w[8].sel = s8;
    // The activity level in the low three bits, two flatness flags over it, and
    // the nine ternary features above bit 5.
    //
    // `act_lvl` is `level_of[]`, which `alt_p1_alloc` fills by walking the
    // seven edges of `p1_level_edges` -- so eight levels, three bits, and the
    // width is a fact about that table rather than a guess.  `quiet` is a
    // comparison at all three of the places that set it, and the other flag is
    // the one bit that survives the two activity sums of `seed_activity`.
    //
    // The nine weights stay a sum and cannot be anything else: `CtxWeight::w`
    // holds each feature's three values *pre-scaled* to `32 * 3^k`, so the
    // stride lives in the table and what is written here is nine lookups added.
    // That is a mixed-radix number whose digits were multiplied out at set-up,
    // which `digit<>` cannot say and `raw` can.
    int32_t result = CtxIdx{}.bits<0, 3>(act_lvl)
                             .bit<3>(ctx[3+ctx[2]]==0)
                             .bit<4>(quiet)
                             .raw(ctx_w[0].w[s0]+ctx_w[1].w[s1]+ctx_w[2].w[s2]
                                 +ctx_w[3].w[s3]+ctx_w[4].w[s4]+ctx_w[5].w[s5]
                                 +ctx_w[6].w[s6]+ctx_w[7].w[s7]+ctx_w[8].w[s8]);
    ctx[0] = result;
    return result;
  }

  // One plane of an eight-bit image, in whichever direction the caller is
  // going.  The mirror of `AltP2Block::alt_p2_d8_body`, and merged for the
  // same reason: the row walk, the row seed and the per-sample context are one
  // skeleton and only the middle three lines differ.
  //
  // The encode half was a method here and the decode half was written out
  // inside `alt_model_p1_d8_decode` in `alt_p1.inc`, which is why
  // `pairshare.py` measured that pair at 3 shared lines of 34: it was
  // comparing a four-line wrapper against a twenty-six-line one, and the
  // twenty-two lines they really share were on the wrong side of a call.
  //
  // The decoder walked its rows with `y < height` signed where the encoder
  // cast both to `uint32_t`.  `height` is `int32_t` and the `if` above has
  // already established it is positive, so the two readings agree; the
  // encoder's is kept.
  template<int32_t f_DEC>
  void d8_body(uint8_t* src, uint8_t* out) {
    if( f_DEC )
      rc_begin_decode();
    else
      rc_begin_encode();
    if( height>0 ) {
      int32_t y = 0;
      do {
        ++y;
        advance_row();
        seed_activity(false);
        if( !(width<=0) ) {
          uint8_t* q = out;
          int32_t x = 0;
          do {
            ++x;
            ctx_of(nullptr, nullptr);
            if( f_DEC ) {
              const int32_t code = alt_p1_decode_symbol(&counters[ctx[0]],
                                                        ctx[1]);
              *q = (uint8_t)((uint8_t)pred+unfold[(uint8_t)code]);
            } else {
              const int32_t code = fold_or_refuse(q, *src, (uint8_t)pred, fold,
                                                  fold_hi, unfold);
              alt_p1_encode_symbol(&counters[ctx[0]], ctx[1], code);
              ++src;
            }
            record_sample((uint8_t)*q);
            ++q;
          } while( x<width );
          out = q;
        }
      } while( (uint32_t)y<(uint32_t)height );
    }
    if( f_DEC )
      rc_end_decode();
    else
      rc_end_encode();
  }

  // One selector's counters, and there are nine of them -- `ctx_w[0]` through
  // `ctx_w[8]`.  The decompilation wrote all nine out, which is why
  // `update_model` was 406 lines: about 360 of them are these forty, nine
  // times over, differing only in which `ctx_w` they read and which locals
  // they spelled the intermediate values into.
  //
  // `sel` names which of the three weights is the live one.  At `sel == 1` the
  // update walks the other two symmetrically.  Otherwise it walks three
  // counters: the opposite weight, the middle one, and an alternate context
  // built from two of the three -- and *which* two is the one thing across the
  // nine that is not uniform.  Slot 0 names `w[0]` and `w[1]` where the other
  // eight name `w[1]` and `w[2-sel]`.
  //
  // That is the same one-weight shift `ctx_alt` shows: its slot-0 term reads
  // `w[1]` where every other term reads `w[2-sel]`.  So the pair is a
  // parameter, not a special case invented to make the loop close.
  void update_selector(int32_t k, int32_t slot_f, int32_t slot_r,
                       uint32_t ctx_alt, int32_t alt_lo, int32_t alt_hi) {
    const CtxWeight& cw = ctx_w[k];
    const int32_t sel = cw.sel;
    // Every block re-read `ctx[0]` after each pair of bumps, into a local of
    // its own -- `ctx0`, `ctx1`, ... `ctx8`, which is where those names came
    // from.  Nothing here writes `ctx`; the re-reads are MSVC spilling a
    // register, and the ninth of them was the value the function returned.
    const int32_t here = ctx[0];
    if( sel==1 ) {
      int32_t mid = here-cw.w[1];
      CounterNode *lo = &counters[mid+cw.w[0]];
      CounterNode *hi = &counters[cw.w[2]+mid];
      lo[0].bump(slot_f, 6);
      hi[0].bump(slot_f, 6);
      if( (here&7)!=7 ) {
        lo[1].bump(slot_f, 4);
        hi[1].bump(slot_f, 4);
      }
      if( here&7 ) {
        lo[-1].bump(slot_f, 3);
        hi[-1].bump(slot_f, 3);
      }
    } else {
      counters[cw.w[2-sel]+here-cw.w[sel]].bump(slot_f, 7);
      CounterNode *mid = &counters[cw.w[1]+here-cw.w[sel]];
      CounterNode *alt = &counters[ctx_alt+cw.w[alt_lo]-cw.w[alt_hi]];
      mid[0].bump(slot_f, 6);
      alt[0].bump(slot_r, 4);
      if( (here&7)!=7 ) {
        mid[1].bump(slot_f, 4);
        alt[1].bump(slot_r, 2);
      }
      if( here&7 ) {
        mid[-1].bump(slot_f, 3);
        alt[-1].bump(slot_r, 2);
      }
    }
  }

  // Returned `ctx[0]` -- or, if the ninth selector's last branch ran, a counter
  // it had just bumped.  All five call sites discard it.
  void update_model() {
    uint32_t code_f = *((uint8_t)(cursor[0]->sym-(uint8_t)pred)+fold);
    int32_t code_r = *((uint8_t)((uint8_t)pred-cursor[0]->sym)+fold);
    int32_t tree_sym = (int32_t)(code_f-5)>>1;
    int32_t slot_f = 6-(code_f&1);
    if( code_f<5 )
      slot_f = *((uint8_t)(cursor[0]->sym-(uint8_t)pred)+fold);
    int32_t slot_r = 6-(code_r&1);
    if( code_r<5 )
      slot_r = code_r;
    // `ctx_w[1].sel` and `ctx_w[2].sel` were read into `sel1_top`/`sel2_top`
    // eight lines above this sum, which made two of its nine terms look unlike
    // the other seven.  Nothing between the read and the use touches `ctx_w`.
    // The alternate context: the low five bits of the live one -- the activity
    // level and the two flatness flags of `ctx_of`, kept -- with all nine
    // features read the other way round above them.  `&0x1F` is that field, and
    // saying so is what makes the boundary between "kept" and "mirrored"
    // visible; `AltP2Block` builds its own the same way over five features and
    // six bits.
    uint32_t ctx_alt = CtxIdx{}.bits<0, 5>(ctx[0]);
    for( int32_t k = 8; k>=1; --k )
      ctx_alt += ctx_w[k].w[2-ctx_w[k].sel];
    ctx_alt += ctx_w[0].w[1];
    CounterNode *node_alt = &counters[ctx_alt];
    node_alt[0].bump(slot_r, 17);
    const int32_t result = ctx[0];
    if( (result&7)!=7 ) {
      CounterNode *node_up = &counters[result];
      uint32_t ctx_up = ((ctx[1]&7)<7)+ctx[1];
      node_up[1].bump(slot_f, 11);
      if( slot_f>=5 )
        update_binary_pair(model_strip(CtxIdx{}.bit<7>(slot_f&1)
                                              .bit<6>(node_up[1].escape_high(slot_f))
                                              .raw(ctx_up)),
                           (int32_t)(code_f-5)>>1);
    }
    if( result&7 ) {
      CounterNode *node_dn = &counters[result];
      int32_t ctx_dn = ctx[1]-((ctx[1]&7)!=0);
      node_dn[-1].bump(slot_f, 13);
      if( slot_f>=5 )
        update_binary_pair(model_strip(CtxIdx{}.bit<7>(slot_f&1)
                                              .bit<6>(node_dn[-1].escape_high(slot_f))
                                              .raw(ctx_dn)),
                           tree_sym);
    }
    if( counters[result].total<0xCCCu ) {
      if( (result&7u)<7 ) {
        node_alt[1].bump(slot_r, 7);
      }
      if( result&7 ) {
        node_alt[-1].bump(slot_r, 5);
      }
      if( slot_f>=5 ) {
        // `key + 8` and `key - 8`: the two strips one context step either side
        // of this one, the same "train the neighbours" the other two arms do.
        // The first was written out longhand as
        // `128*(slot_f&1) + ctx[1] + bias + 8`, which is `key` re-derived three
        // terms at a time beside the `key` on the line above it.
        const uint32_t key = CtxIdx{}.bit<7>(slot_f&1)
                                     .bit<6>(counters[result].escape_high(slot_f))
                                     .raw(ctx[1]);
        if( (key&0x38)>=0x38||(update_binary_pair(model_strip(key+8), tree_sym), (key&0x38)!=0) ) {
          update_binary_pair(model_strip(key-8), tree_sym);
        }
      }
      // The nine selectors.  `update_selector` is what the decompilation
      // wrote out nine times.
      update_selector(0, slot_f, slot_r, ctx_alt, 0, 1);
      for( int32_t k = 1; k<9; ++k )
        update_selector(k, slot_f, slot_r, ctx_alt, 1, 2-ctx_w[k].sel);
    }
  }

  AltP1Block*alt_p1_alloc(int32_t img_w, int32_t img_h, int32_t plane) {
    uint32_t ctr = 0;
    width = img_w;
    height = img_h;
    do
      init_counter_node(&counters[ctr++]);
    while( ctr<0x99C60 );
    pred = 0;
    int32_t lvl = 0;
    int32_t grp = 0;
    int32_t k = 0;
    // `group_of` and `slot_of` are the two halves of the strip index `ctx[1]`,
    // and they are built to sit beside each other without overlapping:
    //
    //   bits 0..2   the activity group, 0..7 -- `p1_group_edges` has seven
    //               live entries, so eight groups
    //   bits 3..5   the predicted-value slot, the same shape over
    //               `p1_slot_edges`, which is why `slot_of` holds `8 * slot`
    //   bit  6      whether the escaping node's slot holds over half its mass
    //   bit  7      the escape's parity
    //   bits 8+     the plane
    //
    // The two coding sites supply bits 6 and 7; these two loops supply the
    // rest.  Bits 3..5 being the slot is what `key & 0x38` reads in
    // `update_model`, and why the `key +- 8` there steps that field by one.
    //
    // The plane was hoisted into `plane_hi = plane << 8` and then written
    // `plane_hi | grp` on one line and `grp1 | plane_hi` on the next -- the
    // same value, twice, in two orders.
    do {
      level_of[2*k] = lvl;
      int32_t lvl1 = (2*k==p1_level_edges[lvl])+lvl;
      group_of[2*k] = CtxIdx{}.above<8>(plane).bits<0, 3>(grp);
      level_of[2*k+1] = lvl1;
      int32_t odd = 2*k+1;
      int32_t grp1 = (2*k==p1_group_edges[grp])+grp;
      lvl = (odd==p1_level_edges[lvl1])+lvl1;
      group_of[2*k+1] = CtxIdx{}.above<8>(plane).bits<0, 3>(grp1);
      grp = (odd==p1_group_edges[grp1])+grp1;
      ++k;
    } while( (uint32_t)k<0x100 );
    // The same walk `alt_p2_block` makes over its own edge table, and it was
    // spelled the same way: `2*pair+++1`, a post-increment inside the
    // comparison that decides the next slot, with the answer parked in a
    // `bump` the next line adds.
    int32_t slot = 0;
    for( uint32_t pair = 0; pair<0x80; ++pair ) {
      slot_of[2*pair] = CtxIdx{}.above<3>(slot);
      const int32_t slot1 = (2*pair==p1_slot_edges[slot])+slot;
      slot_of[2*pair+1] = CtxIdx{}.above<3>(slot1);
      slot = (2*pair+1==p1_slot_edges[slot1])+slot1;
    }
    // The nine weights `alt_p1_context` sums, one per selector.  This was
    // twenty-seven assignments in scheduler order with `uint32_t row = 0;`
    // dropped into the middle of them, and none of the three sequences was
    // readable through the interleaving: the middle weight triples from one
    // context to the next, the top weight is twice the middle, and the bottom
    // weight is zero.  The numbers 32 .. 419904 are that formula run out.
    for( int32_t c = 0, w1 = 32; c<9; ++c, w1 *= 3 ) {
      ctx_w[c].w[0] = 0;
      ctx_w[c].w[1] = w1;
      ctx_w[c].w[2] = 2*w1;
    }
    uint32_t row = 0;
    do
      buf[row++] = (P1Ctx*)bmf_new(sizeof(P1Ctx)*(width+10));
    while( row<5 );
    alt_init_tables(fold, (int8_t*)unfold);
    int32_t wid = width;
    if( width>-10 ) {
      int32_t r = 0;
      do {
        for( int32_t i = 0; i<5; ++i ) buf[4-i][r].sym = 72;
        for( int32_t i = 0; i<5; ++i ) buf[4-i][r].mag = 0;
        wid = width;
        ++r;
      } while( r<width+10 );
    }
    for( int32_t i = 0; i<5; ++i ) cursor[i] = buf[i]+wid+4;
    return this;
  }

  // Returns nothing.  The original returned `this` -- a compiler's habit of
  // leaving the object pointer in the return register, not a value anyone
  // wanted -- and after the `free(this)` above it is a dangling pointer that
  // g++ reports under `-Wuse-after-free`.  All three call sites discarded it;
  // the one that did not, `alt_model_p1_d8_decode`, only passed it on to
  // another caller that did.
  void alt_p1_free() {
    for( int32_t k = 0; k<5; ++k )
      free(buf[k]);
    free(this);
  }
};

static inline int32_t cross_grad(int32_t here, int32_t guess, const AltP1Block* nb) {
  const P1Ctx* r = nb->cursor[0];
  return here-(uint32_t)guess+r[-1].sym-r[-2].sym;
}

static inline int32_t nb_resid(const AltP1Block* nb) {
  return nb->cursor[0][-1].sym-nb->pred;
}
