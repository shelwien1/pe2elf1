// alt_p2_block.inc -- the alternate model for predictor 2: its counters, its frequency
// records and the block they live in.

struct P2Count {
  int8_t rate;
  uint8_t b1;
  int16_t weighted;
};

struct P2Freq {
  uint16_t step;
  uint16_t f[3];
  uint32_t rescale_three_way() {
    // `cut` is read before the halving because the halving consumes it:
    // `f[2] -= f[2] >> 1` is `f[2] -= cut`.  It is also the return value on the
    // branch below that does not overwrite it.
    uint32_t cut = f[2]>>1;
    halve_counts(f, 3);
    uint32_t budget = step;
    if( budget<=0x100 ) {
      // `((16 - budget) >> 30) & 0xFFFFFFFE` on the guarded range, which is a
      // decompiled branchless select and not an idiom: the unsigned wrap makes
      // `16 - budget` reach 2^30 exactly when `budget > 16`, the shift turns
      // that into 3, and the mask into 2.  Below 17 it is 0.
      if( step<=0x20u )
        cut = budget>16 ? 2 : 0;
      else
        cut = 32;
      step = budget-cut;
    } else {
      step >>= 1;
    }
    return cut;
  }

  // One symbol of the deep model's three-way counter, and then the tree.  The
  // encoder is handed a symbol and picks the slot it names; the decoder is
  // handed a point in the range and finds the slot containing it.  They agree
  // on `idx = slot - base`, which is what makes the tail one piece of code:
  // slot 0 is the symbol, and slots 1 and 2 escape into the level tree with
  // `idx & 1` choosing the context and `idx` the parity of what comes back.
  //
  // The encoder said `ctx_pair[sym & 1]` and `(sym - 1) >> 1` where this says
  // `ctx_pair[idx & 1]`; they are the same, because `idx` is 1 for odd symbols
  // and 2 for even ones above zero.
  //
  // Named apart from `SymList::code_symbol`, which is a different coder in a
  // different model.  Two methods with one name is not an error the compiler
  // can see, and it made `unstale.py` measure the wrong one.
  template<int32_t f_DEC>
  int32_t code_three_way(BMFState* cx, const uint32_t* ctx_pair, int32_t sym) {
    uint32_t cum;
    uint16_t st, *slot;
    uint16_t down;
    uint16_t *base = &f[0];
    int32_t c01 = f[1]+f[0];
    uint32_t tot = c01+f[2];
    if constexpr( f_DEC ) {
      int32_t target = cx->rc.get_freq(tot);
      cum = f[0];
      if( (uint32_t)target>=cum ) {
        if( target>=c01 ) {
          cum = c01;
          slot = &f[2];
        } else {
          slot = &f[1];
        }
      } else {
        cum = 0;
        slot = &f[0];
      }
    } else {
      cum = c01;
      if( sym ) {
        if( sym&1 ) {
          cum = f[0];
          slot = &f[1];
        } else {
          slot = &f[2];
        }
      } else {
        cum = 0;
        slot = &f[0];
      }
    }
    uint32_t tot_1 = cum+*slot;
    if constexpr( f_DEC )
      cx->rc.decode(cum, tot_1, tot);
    else
      cx->rc.encode(cum, tot_1, tot);
    uint16_t f_before = *slot;
    if( *slot>0x4000u ) {
      halve_counts(f, 3);
      st = step;
      if( st<=256 ) {
        // The same branchless select as `rescale_three_way`'s.
        if( st<=32 )
          down = st>16 ? 2 : 0;
        else
          down = 32;
        st = st-down;
        step = st;
        f_before = *slot;
      } else {
        st = (uint32_t)st>>1;
        step = st;
        f_before = *slot;
      }
    } else {
      st = step;
    }
    *slot = st+f_before;
    int32_t idx = slot-base;
    if( idx )
      return idx+2*cx->template code_symbol_tree<f_DEC>(cx->model_strip(ctx_pair[idx&1]), (sym-1)>>1);
    return 0;
  }

  void encode_symbol(BMFState* cx, const uint32_t* ctx_pair, int32_t sym) {
    code_three_way<0>(cx, ctx_pair, sym);
  }

  int32_t decode_symbol(BMFState* cx, const uint32_t* ctx_pair) {
    return code_three_way<1>(cx, ctx_pair, 0);
  }
};

// A rounding arithmetic shift right, and the two `&31` masks it was written
// with are dead.
//
// `rate` is `P2Count::rate`.  `alt_p2_alloc` seeds every one of the 163,840
// counters to 5, and exactly one other line writes it -- `code_banks`'s
// `node0->rate = b0n` -- which only fires while `(uint8_t)rate < 8`.  So
// `rate` is in [5, 8] always, and on that range `(rate+31)&31` is `rate-1` and
// `rate&31` is `rate`.
//
// Not mmax's `uint32_t{1u} << ...`: that makes the whole expression unsigned and
// turns the arithmetic shift into a logical one.  `weighted` is `int16_t`-stored
// and routinely negative, so it would change the prediction.  The shift count is
// 4..7 and never 31, so the UB that fix is aimed at is unreachable.
static inline int32_t p2_pred(int32_t weighted, int32_t rate) {
  BMF_ASSUME(rate>=5&&rate<=8);
  return (weighted+(1<<(rate-1)))>>rate;
}

static inline int16_t p2_bump(BMFState* cx, int32_t w2, int32_t err, int32_t shift) {
  // `1u << (shift - 1)` is the rounding term, and the shift is 1, 2 or 3:
  // every caller is `p2_update`, `p2_update_into` or `p2_nudge`, and between
  // them they pass 1 once, 2 four times and 3 eight, plus three `p2_nudge`
  // calls taking the default 2.  Nothing here bounds it, which is what the
  // assert is for -- a comment saying "and this cannot happen because" that
  // can be run.
  //
  // It was written `>= 2` first, on the plan's word that the shift is 2 or 3,
  // and the `BMF_ASSERTS` leg failed 15 checks on the spot.  The 1 is real and
  // it is *new*: unfolding `code_banks`'s comma chain into statements turned an
  // expression that spelled its own `+1` and `>>1` into `p2_update(cx, p, res, 1)`.
  // An invariant taken from a document rather than from the callers, checked by
  // a leg that runs.
  BMF_ASSUME(shift>=1&&shift<=3);
  const uint32_t kick = 32u*(uint32_t)((err>cx->deadzone_hi)-(err<cx->deadzone_lo));
  return (int16_t)((uint32_t)w2+((kick+(uint32_t)err+(1u<<(shift-1)))>>shift));
}

// One counter update: predict from the weight, bump it by how wrong that was.
// All but two of `alt_p2_model`'s 118 `p2_bump` sites are this; the two that
// are not sit inside the comma expression of an `if`, where the update's own
// result decides the branch and a call cannot say it.
//
// Eight of them read the rate as `*(uint8_t*)&p->rate`, and `rate` is
// `int8_t` -- so they took the byte unsigned where this takes it signed.  That
// is the same value here and only here: `p2_pred` uses the rate as two shift
// counts and masks both with `&31`, and the two readings differ by 256, which
// is a multiple of 32.
static inline void p2_update(BMFState* cx, P2Count* p, int32_t res, int32_t shift) {
  const int32_t w = p->weighted;
  p->weighted = p2_bump(cx, w, res-p2_pred(w, p->rate), shift);
}

// The same step with the residual kept.  `p2_update` cannot be used at the two
// sites this replaces: they subtract the bank's prediction *into* the caller's
// residual, which the caller reads again below, so a helper taking it by value
// would drop that write.  Both spelled it out with a temporary of their own,
// `wm0400z` and `wm0400x`.
static inline void p2_update_into(BMFState* cx, P2Count* p, int32_t& res, int32_t shift) {
  const int32_t w = p->weighted;
  res -= p2_pred(w, p->rate);
  p->weighted = p2_bump(cx, w, res, shift);
}

// The rescale on its own.  Two of `alt_p2_model`'s fifty sites compute the
// increment and then add it *conditionally*, so the add cannot move into the
// helper; the guard still can.
static inline void p2_rescale(P2Freq* r) {
  if( r->f[0]+r->f[1]+r->f[2]>kP2FreqRescaleTotal )
    r->rescale_three_way();
}

// One count of a three-way record, bumped by `num` sixteenths of the record's
// step, with the rescale that keeps the three counts from overflowing.  35 of
// the 41 sites in `alt_p2_model` wrote both steps out.
//
// It takes the numerator rather than the increment because `rescale_three_way`
// steps `step` down as well as halving the counts, and the bump reads `step`
// *after* it.  Passing a computed increment would read it before.
//
// The increments in the file are 2, 3, 4, 5, 6, 7, 10 and 13 sixteenths and
// nothing else, which is only visible once they are written as sixteenths:
// `(step&0xFFFC)>>2` and `(step&0xFFF8)>>3` are `(4*step)>>4` and `(2*step)>>4`,
// the masks clearing bits the shift discards anyway.
static inline void p2_freq_add(P2Freq* r, int32_t slot, uint32_t num) {
  p2_rescale(r);
  r->f[slot] += (num*(uint32_t)r->step)>>4;
}

// The record either side of a group's, each touched only where the magnitude
// context leaves room: level 15 has no record above it, and the level at the
// bottom has none below.  Where that bottom is differs -- one of the two
// selector shapes in `bump_selector` stops at 2 rather than 0 -- so the low
// bound is a parameter and not a normalization.
//
// The `sel == 1` arm of `bump_selector` is this shape with a *second* bump on
// the next group's record wedged between the two, inside the upper branch.  It
// stays written out: hoisting that bump out would run it unconditionally, and
// moving it after the lower one would reorder two `p2_freq_add`s whose records
// are only distinct because two weight-table entries happen to differ by more
// than two.  That is a fact about the tables, not about the code, and it is not
// worth resting a byte-identical stream on.
static inline void p2_bump_sides(P2Freq* group, int32_t ctx15, int32_t lo,
                                 int32_t slot, uint32_t up, uint32_t dn) {
  if( ctx15<15 )
    p2_freq_add(&group[1], slot, up);
  if( ctx15>lo )
    p2_freq_add(&group[-1], slot, dn);
}

// The same counter, updated in place and *without* `p2_bump`'s dead-zone kick.
// 22 sites in the decompilation folded it into one `+=` on the next counter in
// the bank; the rounding term is the only thing they share with `p2_update`,
// and folding the two together would put the kick where the original has none.
//
// Four call sites now, three of them inside loops, plus one that used to be a
// comma term in the middle of an `if`.
static inline void p2_nudge(P2Count* p, int32_t res, int32_t shift) {
  // Sixteen bits in and sixteen bits out, through `uint16_t` so that the
  // wrap is defined rather than signed overflow.
  //
  // The shift defaults to 2 because 21 of the original 22 sites wanted 2, and
  // three of the four that are left still do; the odd one is a mirror counter
  // at 3.  The default is declared in `decls.inc` and cannot be repeated here,
  // which is worth knowing before looking for it.
  p->weighted = (int16_t)((uint16_t)p->weighted+((uint32_t)(res-p2_pred(p->weighted, p->rate)+(1<<(shift-1)))>>shift));
}

struct P2Ctx {
  int16_t val;
  int16_t dval;
  int16_t err;
  int16_t aerr;
  int16_t dleft;
  int16_t dup;
  int16_t dupleft;
  int16_t dupright;
  uint8_t sign;
  uint8_t mag;
};

// `alt_p2_context` reached ten of these fields by byte offset off a `uint8_t*`
// -- `cx2p[107]`, `cx2p[89]`, `*(cx2p-19)` and so on, for `c2[5].mag` down to
// `c2[-3].mag` -- plus a `*(int16_t*)((uint8_t*)nb0-18)` for `c0[-1].val` and
// two first-member punnings of a `P2Ctx*` to `int16_t*` for `c2->val`.  Every
// one of them is arithmetic on this layout, so the layout is asserted rather
// than left as a fact about whichever offsets happened to be typed in.
//
// These are not restating a struct definition for its own sake: 107 is
// `5*18 + 17`, and both numbers have to be right for the old spelling and the
// new one to name the same byte.
static_assert(sizeof(P2Ctx) == 18, "the byte offsets in alt_p2_context assume 18");
static_assert(offsetof(P2Ctx, val) == 0 && offsetof(P2Ctx, mag) == 17,
              "alt_p2_context reached `val` at +0 and `mag` at +17");

// One row of the neighbour-weight bank.
//
// Lines 0..6 are the filter taps, 7..13 the running mean square of each tap's
// input -- which is what makes it *normalised* least mean squares -- and 14
// three scalars the two halves share.  Line 15 is a count of the samples this
// row has seen, and the original kept it in the first of that line's four
// `float` slots.
//
// It was a `float[16][4]` and the count was reached by casting: through a
// `uint32_t*` at one site and an `int32_t*` at the other, which is two answers
// to what the type is.  A `memcpy` accessor made that defined; a member makes
// it unnecessary.  Same 256 bytes, same offsets, and `-Wstrict-aliasing` has
// nothing left to say about it.
struct NbRow {
  float w[15][4];
  uint32_t uses;
  uint32_t _pad[3];
  int32_t predict(float (*nb)[4], CtxWeights* sets, int32_t mode);
};

static_assert(sizeof(NbRow) == 16*4*sizeof(float),
              "NbRow replaces a float[16][4] and has to be the same size");
static_assert(offsetof(NbRow, uses) == 15*4*sizeof(float),
              "the use count is the first slot of line 15");

// `bmf_p2_coef` and `bmf_p2_rate` read like tuning constants and are not: for
// the length of one image the model borrows them.  Rows 4..6 of the
// coefficients -- the cross-plane terms -- fold into rows 0..2 and are zeroed,
// three rate rows go to the reset value, and all of it goes back at the end.
//
// Both directions do it, in the same six lines each, 300 lines apart from the
// six that undo them.  Filing the two tables with the genuine constants would
// make the non-d8 path's behaviour inexplicable, so they are named as what
// they are: a session, folded at the top of an image and restored at the
// bottom.  Explicitly, not by a destructor -- the restore sits before the
// plane-freeing loop in both callers and stays exactly there.
struct P2Coef {
  float saved_coef[7][4];
  float saved_rate[3][4];

  void fold() {
    __builtin_memcpy(saved_coef, bmf_p2_coef, sizeof saved_coef);
    __builtin_memcpy(saved_rate, bmf_p2_rate[4], sizeof saved_rate);
    for( int32_t k = 0; k<4; ++k ) {
      for( int32_t i = 0; i<3; ++i ) bmf_p2_coef[i][k] += bmf_p2_coef[4+i][k];
      for( int32_t i = 0; i<3; ++i ) bmf_p2_coef[4+i][k] = 0;
      for( int32_t i = 0; i<3; ++i ) bmf_p2_rate[4+i][k] = bmf_p2_rate_reset;
    }
  }

  void restore() {
    __builtin_memcpy(bmf_p2_coef, saved_coef, sizeof saved_coef);
    __builtin_memcpy(bmf_p2_rate[4], saved_rate, sizeof saved_rate);
  }
};

// The pool below the class hands these out, and `alt_p2_free` gives one back
// from inside it -- so the name has to exist before the class and the storage
// cannot, since the pool holds `AltP2Block` by value.
struct AltP2Block;
static void p2_block_give(AltP2Block* p);

struct AltP2Block {
  // The same pair as `AltP1Block::kRowMargin` and for the same reason: the 8
  // was a `ring_advance` argument and the 13 an allocation, with nothing
  // saying they are two ends of one row.  The deepest subscript the p2 sources
  // reach is exactly -8, measured, which is the margin; the tail is what the
  // right-edge mirror and the `c1[5]` lookahead need.
  static const int32_t kRowMargin = 8;
  static const int32_t kRowTail = 5;

  NbRow nb_weights[1088];
  // This was a union of `p2_row` with a struct whose first 112 bytes were
  // `p2_row0_head[15]` and `_u0_0_4[97]` -- 112 being `sizeof p2_row`, so the
  // union existed only to pin everything after it at the original's offsets.
  // The one overlap ever exercised is a partial clear of the first fifteen
  // bytes of `p2_row` at three sites, which is what `p2_row0_head` was for and
  // is written as that now.
  //
  // Fifteen, not sixteen: three and three-quarter floats.  The original very
  // likely cleared a differently-shaped structure that started here, and
  // rounding it up to a whole element would change what the model sees on the
  // first sample of a plane, so it is preserved rather than tidied.
  float p2_row[7][4];
  // `bias` has to follow `p2_row`, and that is not decoration: `NbRow::predict`
  // is handed `p2_row` as its `nb` and writes `nb[7][0]`, `nb[7][1]` and
  // `nb[7][2]` -- one row past the end of a `[7][4]`, which lands here.  Those
  // three floats are how it returns the centre, the mixed prediction and its
  // own; `alt_p2_model` reads `bias[0..2]` and no other slot.  So the filter
  // returns an `int32_t` and three floats, and the second half of that is
  // invisible at both the call site and the signature.
  //
  // The union this pair came out of guaranteed the adjacency by construction.
  // Sequential members do not, so it is asserted below -- the one place in this
  // tree where an `offsetof` check is not restating the layout, because here
  // the code depends on it.
  float bias[4];
  NbRow* nb_cur;
  NbRow** row0;
  NbRow** row1;
  NbRow** cur;
  NbRow** above;
  int32_t bank_ctx[5];
  int32_t _pad0;
  uint16_t pred_prev;
  uint8_t _pad2[2];
  uint32_t ctx;
  uint32_t ctx_pair[2];
  int32_t nb_id_used;
  int32_t band_lo;
  int32_t band_hi;
  uint32_t plane_idx;
  int32_t has_ref;
  P2Ctx* cursor[5];
  P2Ctx* buf[5];
  CtxWeight ctx_w[5];
  uint8_t _pad1[48];
  int32_t nb_sum[10];
  int32_t ctx_delta[260];
  uint8_t fold[256];
  uint8_t fold_hi[256];
  int8_t unfold[256];
  uint8_t nb_ctx[120];
  // 6 bands * 320.  Declared `[1916]` with the four entries after it split off
  // as `uint8_t _pad2[8]` -- the recovery could not see them indexed, and the
  // index reaches them: `63 + 64*gA + 320*band` with `gA` 0..4 and `band` 0..5
  // is 1919 at its largest, which eight of the seventeen corpus images hit on
  // every run.  The overrun landed in the padding and stopped there, exactly,
  // because four `int16_t` is eight bytes.  Same size, same offsets, and the
  // accesses are in bounds now.  CONTEXT-INDEX.md section 1.
  int16_t nb_id[1920];
  // Five banks of 32768, which `slot()` is the index arithmetic for.  The
  // seeding loop counted to `0x28000` and this said 163840; the same number.
  static const int32_t kP2CounterCount = 5*32768;
  P2Count p2_ctr[kP2CounterCount];

  // The four `_padN` arrays above are the decompilation's record of gaps in
  // MSVC's frame, and a filler array is a layout contract written in a form
  // nothing checks.  Stage 2 Section 10 asks for the contract to travel with
  // the declaration instead, which is what these are: the offset of the member
  // *after* each pad, which is the only thing the pad exists to produce.
  //
  // They matter more than the usual `offsetof` restatement because this struct
  // is 1,064,560 bytes of model state, one slot of `p2_blocks`, and
  // because two of its members are already load-bearing on adjacency -- `bias`
  // one row past `p2_row`, asserted below, and `nb_id` whose last four entries
  // were a separate `_pad2[8]` until an index reached them.

  // Counter `ctx` of bank `k`.  Five banks of 32768, so the bank is a digit at
  // that stride and the context is the fifteen bits under it -- which is a
  // width the construction proves rather than one asserted here: a bank context
  // sets no bit above 25 and is then shifted down by 11.
  //
  // Nine sites indexed this array and they did it two ways.  `alt_p2_model`
  // wrote `32768*bank + bctx` in a loop over the five; `alt_p2_context` walks
  // them straight through and wrote the stride out as five literals -- `bank1 +
  // 32768`, `bank2 + 65536`, `bank3 + 98304`, `bank4 + 131072` -- so which bank
  // a line touches had to be recovered by dividing.  They are the same index
  // and now say so.
  static uint32_t slot(int32_t bank, int32_t ctx) {
    return CtxIdx{}.bits<0, 15>(ctx).digit<32768, 5>(bank);
  }

  // The prediction bank `bank` makes in context `ctx`.  `alt_p2_context` builds
  // five of these, one per stage of the cascade, and each of the five evaluated
  // `slot()` twice -- once for the weight and once for the rate -- because the
  // decompilation had the two loads as separate expressions.  `slot` is a pure
  // static, so folding them is exact.
  //
  // Two of the five did not call `p2_pred` at all: they spelled its shift out,
  // with the `&31` masks that are dead for the reason the helper's comment
  // gives.  Those two are the same call as the other three.

  // The weight row this neighbourhood slot uses, found or minted.
  //
  // A slot seen before names a row, and that row is stepped toward the sample
  // first: predict with it, take the error against `c0[-1].val`, and walk all
  // twenty-eight taps.  A slot seen for the first time takes the next row and
  // is not stepped -- it has nothing to step from, and `NbRow::predict` seeds
  // it from the mixture on its first call instead.
  //
  // Lifted out of `alt_p2_context` because it is a stage of that function
  // rather than a step in it: fifty-five lines between the slot number and the
  // filter inputs, doing one thing that has a name.  What it needs from the
  // caller is the slot and the row cursor whose sample the error is measured
  // against; `p2_row`, `bias`, `nb_id` and `nb_cur` are all members.
  void seat_nb_row(int32_t nb_slot, const P2Ctx* c0) {
    const int32_t at = nb_id[nb_slot];
    if( !at ) {
      // **The counter is not bounded and the array it indexes is.**  `nb_slot`
      // names 1920 slots -- `320*band + 64*gA + 16*gB + 4*gC + gD`, and every
      // one of the five is a nested count of threshold crossings, so every one
      // of its values is reachable -- while `nb_weights` holds 1088 rows, of
      // which row 0 is the no-history row `alt_p2_alloc` seeds and
      // `row0`/`row1` start pointing at.  So 1087 distinct slots in one plane
      // is the most this can allocate, and the 1088th writes past the array
      // into `p2_row`, `bias` and `nb_cur` -- the last of which is a pointer
      // the next sample follows.
      //
      // Nothing here or anywhere else checks it, and the sizing is not
      // obviously wrong: instrumented, the nineteen corpus images reach a
      // high-water mark of 593 (`x_ep`; the next are `t8g` and `t8p` at 473),
      // and a montage built to spread activity, gradient direction and noise
      // across 2500 independently-parameterised tiles reaches 980.  The *union*
      // over all twenty is 1034 distinct slots -- 53 short of the bound.  That
      // margin is small enough that 1088 looks measured rather than guessed,
      // and small enough that no input found so far crosses it.
      //
      // It is left as it is: a clamp would change the model on exactly the
      // inputs in question and no corpus stream would move to say so.  The
      // `BMF_ASSUME` is the honest instrument -- a comment under the default
      // build and a check under `BMF_ASSERTS=1`, which is a build leg.
      //
      // The three statements this replaced carried a `next_id` that was
      // `nb_id_used` before, during and after the increment, and an `(int16_t)`
      // on the last read that matched the width the store narrows to and cannot
      // bite below 32768.
      nb_id[nb_slot] = ++nb_id_used;
      BMF_ASSUME(nb_id_used<1088);
      nb_cur = &nb_weights[nb_id_used];
      return;
    }
    NbRow*const wrow = &nb_weights[at];
    nb_cur = wrow;
    const float err = ((float)c0[-1].val-(nb_dot(wrow->w, p2_row)+bias[0]))*2.0999999f;
    const float floor_ = 7744.0f*wrow->w[14][2];
    for( int32_t j = 0; j<7; ++j )
      for( int32_t k = 0; k<4; ++k )
        nlms_step(wrow, j, k, p2_row[j][k], err, bmf_p2_ms_rate, floor_);
  }

  int32_t p2_pred_at(int32_t bank, int32_t bctx) {
    const P2Count& c = p2_ctr[slot(bank, bctx)];
    return p2_pred(c.weighted, c.rate);
  }

  // One stage of the five-bank cascade.  The context word's top bits name the
  // bank slot, the counter there predicts, and that prediction accumulates onto
  // the running one, which is what the next stage's word is built against.
  //
  // The two `nb_sum` slots per bank are the pair `alt_p2_model` reads back: the
  // running total at `2*bank` -- the residual is taken against it -- and the
  // stage's own prediction at `2*bank + 1`, which the second arm folds into the
  // residual again.  That indexing is why this is one function and not five: the
  // even/odd convention was spelled out ten times, with the two stores in one
  // order for banks 0, 2, 3 and 4 and the other order for bank 1, so which slot
  // held which could only be read off by matching each literal to its neighbour.
  //
  // `word>>11` drops the eleven low bits the `bit_of<>` chains fill and is the
  // same shift at all five sites.  The parameter is `word` rather than `ctx`
  // because `AltP2Block::ctx` is a member and a parameter of that name shadows
  // it -- which the `BMF_WARN=1` leg says and the default build does not.  It takes the word unsigned because that is
  // what `CtxIdx` converts to and what banks 3 and 4 shifted -- banks 0, 1 and 2
  // went through an `int32_t` local first and shifted arithmetically, which is
  // the same value for a word whose highest field is bit 25 but not the same
  // spelling.  One spelling now, and the three locals are `uint32_t`.
  //
  // Bank 0's `run_in` is the filter output rather than a previous stage; the
  // other four chain.
  int32_t step_bank(int32_t bank_no, uint32_t word, int32_t run_in) {
    const int32_t bank = (int32_t)(word>>11);
    bank_ctx[bank_no] = bank;
    const int32_t pred = p2_pred_at(bank_no, bank);
    nb_sum[2*bank_no+1] = pred;
    const int32_t run = pred+run_in;
    nb_sum[2*bank_no] = run;
    return run;
  }
  // 15552 = 16 magnitude levels * 972, the mixed-radix span the five ternary
  // selectors reach through `mixer_fwd`.  Seeded by a loop that counted to
  // `0x3CC0`.
  static const int32_t kP2FreqCount = 15552;
  P2Freq freq[kP2FreqCount];
  uint8_t _pad3[8];
  // Defined out of line, in alt_p2_context.inc and alt_p2_model.inc.  They are
  // 400 and 1100 lines and belong in their own files, which is what the split
  // is for; every other method in this tree is defined in its class body
  // because every other method fits there.  They were free functions taking
  // the block as a first argument, which is what a method is.

  // The NLMS filter, in two halves, and each was a bare braced block inside
  // `alt_p2_model`'s 674 lines.
  //
  // The row's shape is `NbRow` above: lines 0..6 the taps, 7..13 the running
  // mean square of each tap's input -- which is what makes this *normalised*
  // least mean squares, each tap's step divided by the energy it has been
  // seeing -- and line 14 three scalars the two halves share.
  //
  // Both halves are written as `[7][4]` nests rather than 28 scalars because
  // that is the shape the original's SSE was in; keeping it is what lets a
  // compiler put it back.

  // One tap of a normalised least-mean-squares step: move the tap's running
  // mean square toward `x*x` at `ms_rate`, then move the weight by the error
  // scaled by `x` and divided by that mean square.  Dividing by the running
  // power is the whole of what "normalised" means here.
  //
  // Written out three times -- twice below for the fast and slow rows and once
  // in `alt_p2_context` -- differing only in the rate and the floor, so what
  // the three had in common was invisible.  It takes `j` and `k` rather than a
  // `float&` because the mean square it updates is the same row seven lines
  // down: `w[j]` and `w[7 + j]` are one tap.
  void nlms_step(NbRow* row, int32_t j, int32_t k, float x, float err,
                 float ms_rate, float floor_) {
    const float ms = row->w[7+j][k]+(x*x-row->w[7+j][k])*ms_rate;
    row->w[7+j][k] = ms;
    row->w[j][k] += bmf_p2_rate[j][k]*err*x/(ms+floor_);
  }

  // Track the input energy and step both rows' weights toward their own error.
  // Two rows, not one: `fast` adapts against this sample and `slow` against the
  // confidence-scaled one, and the pair is the mixer.
  void nlms_track_two_rows(NbRow* fast, NbRow* slow,
                           float err_fast, float err_slow, float conf) {
    const float floor_fast = 26896.0f*fast->w[14][2];
    const float floor_slow = 5041.0f*slow->w[14][2];
    const float rate_slow = 0.013f*conf;
    for( int32_t j = 0; j<7; ++j )
      for( int32_t k = 0; k<4; ++k ) {
        const float x = p2_row[j][k];
        nlms_step(fast, j, k, x, err_fast, 0.05f, floor_fast);
        nlms_step(slow, j, k, x, err_slow, rate_slow, floor_slow);
      }
  }

  // Predict this sample from the fast row, then step that row toward the error
  // it just made.  `w[14][2]` decays toward 10 and scales the step floor, so
  // the filter starts cautious and loosens; `uses` counts the samples this
  // row has seen.
  void nlms_predict_and_correct(NbRow* row, float sample, float bias0) {
    const float err = sample-(bias0+nb_dot(row->w, p2_row));
    const float ms_scale = row->w[14][2];
    for( int32_t j = 0; j<7; ++j )
      for( int32_t k = 0; k<4; ++k )
        row->w[j][k] += bmf_p2_rate[j][k]*err*p2_row[j][k]/(row->w[7+j][k]+ms_scale*529.0f);
    ++row->uses;
    row->w[14][2] = ms_scale+((10.0f-ms_scale)*0.00019999999f);
  }

  // The two reference planes' rows as `alt_p2_context` reads them: three
  // cursors each, one record back from where the ring points.  All six are null
  // when this plane has no references, which is what `refa` being null means and
  // what the arms downstream test for.
  //
  // A struct rather than six out-parameters because they are one thing -- the
  // reference neighbourhood -- and because six `P2Ctx**` in a signature is six
  // chances to pass them in the wrong order.
  //
  struct P2RefRows {
    P2Ctx *r0, *r1, *r2;
  };

  struct P2Refs {
    P2RefRows a, b;
  };

  // Rows 0, 1 and 2 of a reference plane's ring, each one record back from where
  // it points.  Both reference planes are read identically and that `-1` had
  // been spelled out six times, which is the kind of offset that wants one home.
  //
  // A named factory over an aggregate rather than a constructor.  The
  // constructor was written first and `covered.py` reported the type as an
  // undescribed function while `deadcheck.py` read `r2(ring[2]-1)` in its
  // initialiser list as a call to something nothing defines -- a member
  // initialiser list being a construct nothing else in this tree uses.  Two
  // tools could have been taught it, but a function with a name that says what
  // it returns is the better of the two spellings on its own merits, so the
  // construct went instead of the parsers.
  //
  // It takes the ring rather than the block so that it does not depend on a
  // type it is declared inside.
  static P2RefRows ref_rows(P2Ctx* const* ring) {
    return {ring[0]-1, ring[1]-1, ring[2]-1};
  }

  P2Refs fill_row_inputs(AltP2Block* refa, AltP2Block* refb);

  int32_t seat_symbol_context(int32_t run4, const P2Refs& refs);

  void code_banks(int32_t sample16);

  // One pass over a bank's eleven context bits, doing to each what the arm it
  // was reached by says: bump the direct counter at shift 2, the rotated one
  // at 3, optionally nudge the record after the direct one, bump the record
  // before it, and bump the mirrored one with the residual negated.
  //
  // The second and third arms of `code_banks` differ in one line of the five:
  // whether that nudge happens.  `direct[0]` and `rot[0]` are bumped before
  // the branch that chooses between them and `mirror[10]` after the two join,
  // which is why this skips both ends.
  void walk_bank_bits(P2Count*const* direct, P2Count*const* mirror,
                      P2Count*const* rot, int32_t res, bool nudge) {
    for( int32_t k = 0; k<11; ++k ) {
      if( k ) {
        p2_update(cx, direct[k], res, 2);
        p2_update(cx, rot[k], res, 3);
      }
      if( nudge )
        p2_nudge(&direct[k][1], res);
      p2_update(cx, &direct[k][-1], res, 3);
      if( k<10 )
        p2_update(cx, mirror[k], -res, 3);
    }
  }

  // Bank `k`, bumped.  Its selector picks one of two shapes: a selector of 1
  // bumps the group's record, the one after it, the first record of the next
  // group and the one before -- four counts at 3, 4, 2 and 6 sixteenths -- and
  // any other selector bumps three, at 7, 5 and 6.  The bank the step came
  // from is bumped first either way, at 4 or 7.
  //
  // Written out five times in `alt_p2_model`, thirty-six lines apiece, over
  // `ctx_w[0]` through `ctx_w[4]` with a fresh set of temporaries in each --
  // `frec3`/`frec4b`/`frec5`/`frec4c` for the first record, `idx0`/`rec_idx`
  // for the next group, `grp`/`frec_step`/`gtop` for the group.  Nothing else
  // differed, and not one of those names outlived the copy it was written in.
  // Round nine made the five a loop over `k`; this makes the loop's body a
  // name, which is what took the model's deepest nesting off eight.
  void bump_bank(int32_t k, uint32_t step_v, int32_t ctx15, int32_t fold_sel,
                 int32_t is_dec) {
    const int32_t sel = ctx_w[k].sel;
    if( sel==1 ) {
      p2_freq_add(&freq[ctx_w[k].w[0]+step_v-ctx_w[k].w[1]], fold_sel, 4);
      const uint32_t off = ctx-ctx_w[k].w[1];
      P2Freq*const group = &freq[off+ctx_w[k].w[0]];
      const uint32_t next_group = ctx_w[k].w[2]+off;
      p2_freq_add(&group[0], is_dec, 3);
      if( ctx15<15 ) {
        p2_freq_add(&group[1], is_dec, 4);
        p2_freq_add(&freq[next_group+1], is_dec, 2);
      }
      if( ctx15>2 )
        p2_freq_add(&group[-1], is_dec, 6);
    } else {
      p2_freq_add(&freq[step_v+ctx_w[k].w[1]-ctx_w[k].w[2-sel]], fold_sel, 7);
      P2Freq*const group = &freq[ctx_w[k].w[1]+(ctx-ctx_w[k].w[sel])];
      p2_freq_add(&group[0], is_dec, 7);
      p2_bump_sides(group, ctx15, 0, is_dec, 5, 6);
    }
  }

  template<int32_t f_DEC>
  void alt_p2_d8_body(uint8_t* src, uint8_t* out,
                      int32_t width, int32_t height);
  int32_t alt_p2_context(AltP2Block* refa, AltP2Block* refb);
  uint32_t alt_p2_model(int32_t sample_in, uint8_t a4, int32_t resid_in);

  // Code one sample of this plane, with the near-lossless drift control: fold
  // the residual against the prediction, reconstruct it, and if the
  // reconstruction has drifted more than sixteen from the byte already in the
  // buffer, send the sample through `fold_hi` instead and leave that byte
  // alone.
  //
  // Four copies in `alt_model_p2_encode`, one per plane, twelve lines apiece,
  // differing in the block, the two reference planes and the output offset.
  // The fourth spelled the reconstruction twice instead of naming it.
  //
  // `out[at]` is read twice on purpose, as the copies did: once before
  // `alt_p2_context` and once after.
  void encode_sample(uint8_t* out, int32_t at, AltP2Block* refa, AltP2Block* refb) {
    int32_t sample = out[at];
    const int32_t pred = alt_p2_context(refa, refb);
    const int32_t resid = (uint8_t)(sample-pred);
    int32_t code = fold[resid];
    const int32_t was = out[at];
    const int32_t clean = (uint8_t)(pred+unfold[code]);
    const int32_t recon = (uint8_t)(clean+was-sample);
    if( was-recon<-16||was-recon>16 ) {
      code = fold_hi[resid];
    } else {
      sample = clean;
      out[at] = recon;
    }
    freq[ctx].encode_symbol(cx, ctx_pair, code);
    alt_p2_model(sample, code, sample-pred);
  }

  // The decode half of `encode_sample`: predict, take the symbol, unfold it,
  // write the byte, and step the model.  Four copies in `alt_model_p2_decode`,
  // one per plane.
  //
  // Shorter than the encoder by exactly the near-lossless drift control, which
  // is the asymmetry the header of alt_p2.inc records: the decoder is told
  // which fold was used and has nothing to decide.
  void decode_sample(uint8_t* out, int32_t at, AltP2Block* refa, AltP2Block* refb) {
    const int32_t pred = alt_p2_context(refa, refb);
    const int32_t code = freq[ctx].decode_symbol(cx, ctx_pair);
    const int32_t sample = (uint8_t)(pred+unfold[code]);
    alt_p2_model(sample, code, sample-pred);
    out[at] = sample;
  }

  // One sample, in whichever direction the caller is going.  `alt_model_p2`
  // is one template over both and this is the only line of its inner loop that
  // differs between them; four planes make four such lines, which is what kept
  // the two 90-line bodies apart.
  template<int32_t f_DEC>
  void code_sample(uint8_t* out, int32_t at, AltP2Block* refa, AltP2Block* refb) {
    if constexpr( f_DEC )
      decode_sample(out, at, refa, refb);
    else
      encode_sample(out, at, refa, refb);
  }

  // Seed the three spare history rows from the row just coded: mirror its last
  // record five forward past the right edge, mirror eight back from the start
  // of the row above it, and copy the whole row into rows 1, 2 and 3.
  //
  // Three copies -- `alt_p2_d8_body` after its first row, and both of
  // `alt_model_p2_encode` and `alt_model_p2_decode` -- with the width spelled
  // as `width`, `first` and `back`, one of them negated at the call site.
  // The three history rows seeded from row 0, which both seeds end with.
  // `18*width + 234` is `sizeof(P2Ctx) * (width + 13)` -- a whole row, margins
  // included -- and written that way it is a layout declared rather than
  // multiplied out.
  void copy_row0_down(int32_t width) {
    static_assert(sizeof(P2Ctx) == 18, "a row is eighteen bytes to the record");
    const int32_t row_bytes = sizeof(P2Ctx)*(width+kRowMargin+kRowTail);
    for( int32_t i = 0; i<3; ++i )
      memcpy(buf[1+i], buf[0], row_bytes);
  }

  void seed_history(int32_t width) {
    P2Ctx*const here = cursor[0];
    for( int32_t i = 0; i<5; ++i )
      here[i] = here[-1];
    P2Ctx*const up = cursor[0]-width;
    for( int32_t i = 0; i<8; ++i )
      up[-1-i] = up[i];
    copy_row0_down(width);
  }

  // The neutral record every row 0 starts from, and the five cursors seated
  // past it.  Written out in both coders, twelve lines apiece for the record
  // and one re-rolled loop each for the copies and the cursors.
  void seed_row0(int32_t width) {
    for( int32_t x = 0; x<width+13; ++x ) {
      P2Ctx&rec = buf[0][x];
      rec.val = 256;
      rec.dval = 256;
      rec.err = -16;
      rec.sign = 1;
      rec.mag = 3;
      rec.aerr = 512;
      rec.dupright = 512;
      rec.dupleft = 512;
      rec.dup = 1024;
      rec.dleft = 256;
    }
    copy_row0_down(width);
    for( int32_t i = 0; i<5; ++i )
      cursor[i] = buf[i]+width+8;
  }

  // Roll the four bias accumulators forward by the record just coded: thirty-two
  // times each of its four difference fields.
  //
  // Eight copies, four in each coder, and the two coders do not agree on where
  // the total lives.  The decoder keeps it in `ctx_bias` and adds to it after
  // every plane, which is the form that reads.  The encoder carries it in four
  // locals -- `at0`, `seed0`, `off0`, `want0` -- and writes `ctx_bias` back from
  // them one plane late, so plane 2's contribution reaches `ctx_bias` only if
  // there is a plane 3 to need it.  That delay is MSVC's scheduling and not the
  // model: `alt_p2_context` is the only reader, and by the time it could see the
  // difference the next sample has already reloaded `ctx_bias` from the total.
  void add_bias(int32_t* acc) const {
    const P2Ctx&rec = cursor[0][-1];
    acc[0] += 32*rec.dupleft;
    acc[1] += 32*rec.dupright;
    acc[2] += 32*rec.dleft;
    acc[3] += 32*rec.dup;
  }

  // Roll this plane forward by one row: carry the last neighbour of the row
  // just finished into both seams of the next, swap the two neighbour rows and
  // step them past their margins, clear the filter's row and its four biases,
  // and rotate the five context rings so the oldest becomes the new row.
  //
  // Three copies of this, forty-seven lines apiece.  `alt_p2_model` had it on
  // its own members; `alt_model_p2_encode` and `alt_model_p2_decode` each had
  // it written out on `blk_r->`, with the ring rotation spelled through five
  // named temporaries rather than the `ring_advance` next door -- which is why
  // the three did not look alike at a glance even though they are one thing.
  void start_row() {
    NbRow*const last = cur[-1];
    cur[1] = last;
    cur[0] = last;
    NbRow**const was_row0 = row0;
    row0 = row1;
    row1 = was_row0;
    cur = row0+2;
    above = row1+2;
    cur[-1] = *above;
    cur[-2] = *above;
    memset(p2_row, 0, 15);
    for( int32_t i = 0; i<4; ++i ) bias[i] = 0.0f;
    __builtin_memset(p2_row, 0, sizeof p2_row);
    {
      P2Ctx*const here = cursor[0];
      for( int32_t i = 0; i<5; ++i ) here[i] = here[-1-i];
    }
    ring_advance(buf, cursor, kRowMargin);
    cursor[0][-1] = cursor[1][0];
    {
      P2Ctx*const here = cursor[0];
      P2Ctx*const up = cursor[1];
      for( int32_t i = 0; i<7; ++i ) here[-2-i] = up[1+i];
    }
    cursor[0]->dval = 0;
  }

  AltP2Block*alt_p2_alloc(BMFState* state, int32_t img_w, int32_t plane) {
    cx = state;
    uint32_t done;
    uint32_t j, p, n;
    plane_idx = plane;
    // One pass, not two 2x-unrolled ones.  The original walked the 163,840
    // counters twice -- once zeroing `weighted`, once seeding `rate` and `b1`
    // -- with each loop doing two records an iteration.  This runs once per
    // plane, and the difference is not measurable: 9.7s either way on `x_ep`.
    for( j = 0; j<(uint32_t)kP2CounterCount; ++j ) {
      p2_ctr[j].weighted = 0;
      p2_ctr[j].rate = 5;
      p2_ctr[j].b1 = 2;
    }
    for( uint32_t e = 0; e<(uint32_t)kP2FreqCount; ++e ) {
      freq[e].f[0] = 2048;
      freq[e].f[1] = 2816;
      freq[e].f[2] = 2816;
      freq[e].step = 4096;
    }
    // See `alt_init_tables`: this is the second of the three reads of a
    // quantiser nothing in this build sets above zero.
    BMF_ASSUME(cx->near_lossless_q==0);
    int32_t dz = 4*cx->near_lossless_q+1;
    int32_t band = 16*cx->near_lossless_q;
    has_ref = (cx->plane_desc[cx->plane_desc[plane_idx].src_plane].flags&desc_has_refs)!=0;
    cx->deadzone_hi = dz;
    cx->deadzone_lo = -dz;
    band_lo = -band-7;
    band_hi = band+8;
    // Two rows of weight-set pointers, one per column plus the four the
    // margins need.  `img_w + 4` is at most `kMaxWidth + 4`, so they are
    // members like the record rows below.
    row0 = row0_store;
    nb_weights[0].w[14][2] = 1.0f;
    row1 = row1_store;
    cur = row0+img_w+2;
    if( img_w>-4 ) {
      uint32_t pairs = (img_w+4)/2;
      if( pairs ) {
        for( p = 0; p<pairs; ++p ) {
          row1[2*p] = &nb_weights[0];
          row0[2*p] = &nb_weights[0];
          row1[2*p+1] = &nb_weights[0];
          row0[2*p+1] = &nb_weights[0];
        }
        done = 2*p+1;
      } else {
        done = 1;
      }
      if( (uint32_t)(img_w+4)>(done-1) ) {
        row1[done-1] = &nb_weights[0];
        row0[done-1] = &nb_weights[0];
      }
    }
    uint32_t row = 0;
    // `18*width + 234` is `sizeof(P2Ctx) * (width + 13)`, which is the same
    // margin-plus-samples-plus-tail as the line above spells directly.  It is
    // kept in the decompilation's form because `kP2RowPad` is what the original
    // added and the two readings are worth having side by side.
    static_assert(kP2RowPad == sizeof(P2Ctx)*(kRowMargin+kRowTail),
                  "the row pad is the two margins");
    int32_t row_bytes = 18*max32(img_w, 3)+kP2RowPad;
    // The ring points into the block, as `AltP1Block`'s does.  `row_bytes` is
    // still what the memsets and the row copies use -- only the storage moved.
    //
    // A `for` and not the `do` that was here: writing it `buf[row++] =
    // row_store[row]` reads `row` and increments it in one expression, which
    // g++ reports as `-Wsequence-point` and C++17 only makes well-defined by a
    // rule about which side of an assignment is evaluated first.  The
    // `BMF_CONV=1` leg counted it the moment it was written.
    for( row = 0; row<5; ++row )
      buf[row] = row_store[row];
    memset(buf[0], 0, row_bytes);
    // Four independent stores the decompiler left in the order MSVC scheduled
    // them, counting down with an unrelated initialisation dropped in the
    // middle.
    for( int32_t i = 0; i<4; ++i )
      cx->ctx_bias[i] = 0;
    int32_t lvl = 0;
    cursor[0] = buf[0]+8;
    // `2*pair+++1` -- a post-increment inside the comparison that decides the
    // next level, with the answer parked in a `bump` the next line adds.  The
    // `nb_ctx` walk three lines below does the same thing in a `for` with
    // neither, and this is that shape.
    for( uint32_t pair = 0; pair<0x82; ++pair ) {
      // The plane from bit 8 up and the run level as a four-bit field at 4.
      // The level is bounded by its own table -- `p2_ctx_edges` has sixteen
      // entries and `lvl` indexes it -- so the width is a fact rather than a
      // guess, and `bits<4, 4>` states it.
      //
      // The plane is `above<8>` and not `bits<8, 2>`, which is the distinction
      // those two methods exist to draw.  `plane_count` is
      // `((depth & 0x3F) + 7) >> 3`, which is two bits only because the header
      // check rejects depths above 32 -- a width that rests on a test in
      // another file is not a width to assert here, and a mask that turns out
      // too narrow drops the top of the value silently.  That is what moved
      // three streams when `gA` was written as `bits<6, 2>`.  `above<8>` says
      // where the fields below it end and claims nothing else.
      ctx_delta[2*pair] = CtxIdx{}.above<8>(plane_idx).bits<4, 4>(lvl);
      const int32_t lvl1 = (2*pair==p2_ctx_edges[lvl])+lvl;
      ctx_delta[2*pair+1] = CtxIdx{}.above<8>(plane_idx).bits<4, 4>(lvl1);
      lvl = (2*pair+1==p2_ctx_edges[lvl1])+lvl1;
    }
    int32_t len = 0;
    for( n = 0; n<0x3C; ++n ) {
      nb_ctx[2*n] = len;
      int32_t len1 = (2*n==p2_len_edges[len])+len;
      nb_ctx[2*n+1] = len1;
      len = (2*n+1==p2_len_edges[len1])+len1;
    }
    ctx = 15;
    cx->alt_init_tables(fold, unfold);
    // The same three sequences as `AltP1Block`'s, five contexts instead of
    // nine and starting at 64 instead of 32: the middle weight triples, the
    // top is twice the middle, the bottom is zero.  Fifteen assignments in
    // scheduler order did not say that.
    for( int32_t c = 0, w1 = 64; c<5; ++c, w1 *= 3 ) {
      ctx_w[c].w[0] = 0;
      ctx_w[c].w[1] = w1;
      ctx_w[c].w[2] = 2*w1;
    }
    return this;
  }

  // Returns nothing, and takes nothing.  It returned `this` *after*
  // `bmf_page_free(this)` -- a dangling pointer, which g++ reports under
  // `-Wuse-after-free` -- and the `do_free` flag it tested is 1 at every call
  // site in the tree.  Fourth of its kind here, after `alt_p1_free`,
  // `alt_model_p1_d8_decode` and `ModelBlock::free_workspace`: the decompiler
  // writes the register that happened to hold `this` as a return value wherever
  // MSVC left it there, and a `do_free` nobody clears is the same artefact one
  // level up.
  //
  // The plan asks for "release workspace" and "release object" to be split.
  // There is nothing to split: no caller wants the workspace freed and the
  // object kept, which is what the always-1 flag says.
  void alt_p2_free() {
    p2_block_give(this);
  }

  // The rows the three cursors above point into, last in the struct so that no
  // member's offset moves.  18 bytes to the record and 65548 records to the row
  // -- 5.9 MB for the ring -- plus two pointer rows of 65539 each.
  //
  // That is 7.9 MB a block against 1.02, and four blocks live, so this is where
  // most of the program's `.bss` comes from.  What it buys is seven allocations
  // and seven frees per plane gone, and a model whose working set is a
  // compile-time constant.
  P2Ctx row_store[5][kMaxWidth+kRowMargin+kRowTail];
  NbRow* row0_store[kMaxWidth+4];
  NbRow* row1_store[kMaxWidth+4];

  // The codec state this block belongs to.  It was reached as a file-scope
  // `bmf_cx` from every method here; a pointer means two blocks in one process
  // can belong to two different codecs, which is the whole point of the
  // exercise.  Set once, by this block's allocator.
  //
  // Last in the struct, with the other appended members: putting it first
  // shifted every offset the decompiled record has and the four `_padN`
  // assertions said so at once.
  BMFState* cx;
};

// The four blocks this model can have live at once, in the data segment.  One
// per plane and `plane_count` is at most 4, which `expand_image` enforces and
// the corpus reaches: instrumented both directions, `x_ep`, `xform2` and
// `med32` all hit four.
//
// Zeroed on take, and that `true` is load-bearing rather than tidy: the page
// allocator this replaces memset what it returned, and filling these slots with
// 0xA5 instead fails 15 of 111 checks.  Which is also what makes the *other* two
// pools' `false` an answer and not an omission -- the same poison leaves them
// green.  1.02 MB apiece, 4.1 MB of `.bss`.
static BlockPool<AltP2Block, 4> p2_blocks;

static AltP2Block* p2_block_take() {
  return p2_blocks.take(true);
}

static void p2_block_give(AltP2Block* p) {
  p2_blocks.give(p);
}

// Whether the plane that plane `k` references carries a colour transform, and
// so whether the sample it is seeded from has to be mixed from its references
// or is zero.
//
// Three of these open each coder, under six names between them -- `xf0`, `np`
// and `y` in the encoder, `xf0`, `dc_flag` and `xf4` in the decoder -- none of
// which says which plane it is about.
static bool ref_transformed(BMFState* cx, int32_t k) {
  return (cx->plane_desc[cx->plane_desc[k].src_plane].flags&desc_has_refs)!=0;
}

// Every plane made ready for the next row.  Row 0 seeds the neutral record and
// seats the cursors; row 1 seeds the three history rows from what row 0 left
// behind; every row after that only rolls forward.
//
// Both coders wrote this out, fifteen lines each, and both spelled the row
// index twice -- `src1`/`xf4` in the encoder, `src2`/`first` in the decoder --
// because MSVC saved it around the loop and read it back after.  The decoder
// spelled the width twice as well, `i` and `i_1`.
//
// The width goes to `seed_history` unnegated.  Both call sites read
// `seed_history(-first)` where `first` was itself `-width`, and writing the
// obvious `seed_history(-width)` here cost eight of the 110 checks -- which is
// the one thing a double negative spelled through a named local is good at.
static void alt_p2_start_row(BMFState* cx, AltP2Block** plane, int32_t row, int32_t width) {
  for( int32_t k = 0; k<cx->plane_count; ++k ) {
    if( row==0 )
      plane[k]->seed_row0(width);
    else if( row==1 )
      plane[k]->seed_history(width);
    plane[k]->start_row();
  }
}

// The planes both directions of the p2 model work through.  Each of
// `alt_model_p2_encode` and `alt_model_p2_decode` opened with the first of
// these and closed with the second, written out, and the closing one is the
// longer of the two duplicated blocks left in this pair after `start_row` came
// out -- thirteen lines that re-read `plane_count` from the global on every
// turn of a loop it is also the bound of.  Nothing in `alt_p2_alloc` or
// `alt_p2_free` writes `plane_count`, which is what makes the re-read say
// nothing and the `for` say the same thing as the `do`.
static void alt_p2_planes_alloc(BMFState* cx, AltP2Block** plane, int32_t width) {
  for( int32_t pl = 0; pl<cx->plane_count; ++pl ) {
    AltP2Block* page = p2_block_take();
    plane[pl] = page->alt_p2_alloc(cx, width, pl);
  }
}

static int32_t alt_p2_planes_free(BMFState* cx, AltP2Block** plane) {
  for( int32_t pl = 0; pl<cx->plane_count; ++pl )
    plane[pl]->alt_p2_free();
  return cx->plane_count;
}

// `NbRow::predict` writes `nb[7][0..2]` where `nb` is `p2_row`, a `[7][4]`.
// That is `bias`, and this says so in a form the compiler checks.
// Both widths, for the reason the `ModelBlock` set gives: the block holds
// pointers -- `cursor[5]`, `cur`, `above`, `nb_cur` and the row buffers -- so
// every offset past them moves, by 20 here and by 64 once the five cursors and
// the three weight-row pointers have gone by.
#if UINTPTR_MAX > 0xFFFFFFFFu
static_assert(offsetof(AltP2Block, pred_prev) == 278720, "_pad0 sizes the gap");
static_assert(offsetof(AltP2Block, ctx) == 278724, "_pad2 sizes the gap");
static_assert(offsetof(AltP2Block, nb_sum) == 278968, "_pad1 sizes the gap");
// `sizeof` no longer answers this: the row storage is appended to the record,
// so what pins `_pad3` is where that storage starts.  The number is the same
// one -- the decompiled record still ends at 1064560 -- and saying it this way
// also says that everything past it is this tree's and not the binary's.
static_assert(offsetof(AltP2Block, row_store) == 1064560, "_pad3 closes the record");
#else
static_assert(offsetof(AltP2Block, pred_prev) == 278700, "_pad0 sizes the gap");
static_assert(offsetof(AltP2Block, ctx) == 278704, "_pad2 sizes the gap");
static_assert(offsetof(AltP2Block, nb_sum) == 278904, "_pad1 sizes the gap");
static_assert(offsetof(AltP2Block, row_store) == 1064496, "_pad3 closes the record");
#endif

static_assert(offsetof(AltP2Block, bias)
              == offsetof(AltP2Block, p2_row)+sizeof(AltP2Block::p2_row),
              "NbRow::predict returns three floats by writing one row past "
              "p2_row, which has to be bias");

// The six weight sets `NbRow::predict` mixes, in the order `bmf_p2_mix[mode]`
// weights them -- so the order is the model's and not a choice.  In cursor
// terms, with `cur` and `above` both pointing at this pixel's column and both
// advanced one slot per pixel:
//
//     row[0] = cur[-1]     one to the left, this row
//     row[1] = above[1]    one to the right, the row above
//     row[2] = above[2]    two to the right, the row above
//     row[3] = cur[-2]     two to the left, this row
//     row[4] = above[0]    directly above
//     row[5] = cur[0]      this column's slot, which this row has not written
//                          yet -- the two row buffers alternate, so it still
//                          holds what was put there when this buffer was last
//                          the current row
//
// `f0` was the name, which is an offset and not a role: the member sits at
// zero and the decompiler said so.
struct CtxWeights {
  NbRow* row[6];
};
