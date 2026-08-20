// alt_p2_model.inc -- the predictor-2 model itself, and the largest body in the program.
// "p2" is `plane_predictor == 2`; it is not about depth.  t8g is a single eight-bit
// plane and codes here.  MODELS.md section 7.

// The per-bank counter walk: five banks of context counters, each stepped
// toward this sample's residual, and the mirror and rotate neighbours of each
// stepped with it at lighter weights.
//
// 321 lines inside `alt_p2_model`, and the reason first recorded for leaving
// them there -- "threading about thirty locals through a signature" -- was a
// guess.  What the loop actually needs from the enclosing scope is small and
// nothing it touches is read after it; the compiler was asked rather than a
// regex, twice, because the regex undercounted both times.
void AltP2Block::code_banks(int32_t sample16) {
  // This function opened with a declaration of thirteen `int32_t`, a `bool` and
  // a `uint8_t` -- MSVC's frame, in the order it allocated the slots.  Five of
  // them existed only to carry a value across a comma in the middle of an `if`
  // (`go`, `w1`, `e1`, `w_top3`, `b_top3`) and three more were one residual
  // under four names; with those gone the rest declare where they are first
  // written, which is inside the bank loop, one block in.
  for( uint32_t bank = 0; bank<5; ++bank ) {
    const int32_t bctx = bank_ctx[bank];
    const int32_t res = sample16-nb_sum[2*bank];
    P2Count* node0 = &p2_ctr[slot(bank, bctx)];
    const int32_t w0 = res+(uint16_t)node0->weighted;
    node0->weighted = w0;
    int32_t countdown = node0->b1;
    // Two guards, both of which have nothing after them: the counter is out of
    // its count, or the alphabet was reduced.  They were nested `if`s holding
    // the rest of the function five levels in.
    if( !countdown )
      continue;
    const int32_t w0b = w0+4*((res>deadzone_hi)-(res<deadzone_lo));
    node0->weighted = w0b;
    uint32_t ctxw = bctx;
    // The rate ladder: a counter that keeps seeing a small residual counts
    // down, and at zero its rate steps up and its accumulator doubles --
    // unless it is already at the top of the ladder, where it stays at zero
    // and re-arms nothing.
    //
    // Two of the three arms stored the same thing.  `b1` is `uint8_t` and
    // `countdown` is a copy of it guarded non-zero above, so after the
    // decrement it is 0..254 and `(uint8_t)--countdown` tests the value the
    // decrement produced; the cast was the decompiler recording the byte
    // compare.  Written as a decrement and a test, the "count is not out"
    // arm and the "count is out but the rate is capped" arm are one line.
    if( (int32_t)abs32(res)<38 ) {
      --countdown;
      if( !countdown&&(uint8_t)node0->rate<8u ) {
        // `b0n - 1` is the rate this counter is stepping *from*, and the
        // table starts at rate 5, which is the lowest any counter holds.
        const int32_t from = node0->rate;
        node0->rate = (uint8_t)(from+1);
        node0->b1 = p2_b1_reload[from-5];
        node0->weighted = 2*w0b;
      } else {
        node0->b1 = countdown;
      }
    }
    if( alphabet_reduced )
      continue;
    // The mirrored context: the same counter with the top bits of its
    // index flipped, which is the model's guess at what a symmetric
    // neighbourhood would have used.
    //
    // This was `mir_top`, a `uint16_t*` walking `P2Count` two words at a
    // time -- `[0]` the rate byte, `[1]` the weight, `[2]`/`[3]` the next
    // record's, `[-2]`/`[-1]` the previous one's.  It is a `P2Count*`, and
    // the name is reused a hundred lines below for a `uint16_t*` that
    // genuinely does walk a `P2Freq` as words, which is why one name did
    // two jobs.
    //
    // The decompilation also had `prefetch(&mir_top)` *before* the
    // assignment, which asks for the cache line holding a local the
    // function is about to write anyway.  Every other prefetch below is of
    // the counter the next few lines will read; this one is now too.
    P2Count *mir = &p2_ctr[slot(bank, ctxw^kBankMirrorMask)];
    __builtin_prefetch(mir, 0, 1);
    // The bank's residual with its mirror correction added, and the
    // only value the rest of this block works from.  It reached the three
    // counter walks below through three more names -- `res_c` assigned
    // from it here, `res_s` from `res_c` at the top of the branch,
    // `res_c` from `res_s` again inside it, and `res_t` in the third walk
    // -- with no write to any of them in between.  One name, and `const`
    // says there is no second value to look for.
    const int32_t res2 = nb_sum[2*bank+1]+res;
    // The record after the bank's counter is bumped on three of the four
    // low-bit values and the record before it on three others, and the
    // two sets are not the same three.  That is all this is.
    //
    // It arrived as one `if` with a five-term comma expression in the
    // right half of an `||`: the bump itself was written inside the
    // condition, with a `go` set mid-chain and read as the chain's value
    // two terms later so that the short circuit did double duty as the
    // second guard.  `lowbits >= 3` skips the chain, which is why the
    // first bump does not happen there; `!go` is `lowbits > 0`, which is
    // why the second one does not happen at zero.
    //
    // Written out it is `p2_update` twice, at shifts 1 and 2 -- the first
    // of this file's counter updates the helper could not reach, because
    // a call has no value to hand back to an `||`.
    int32_t lowbits = ctxw&3;
    if( lowbits<3 )
      p2_update(&node0[1], res2, 1);
    if( lowbits )
      p2_update(&node0[-1], res2, 2);
    // Three update blocks follow, one per branch, and each walks these
    // eleven bits in order doing four or five things to each: bump the
    // direct counter at shift 2, the rotated one at 3, nudge the record
    // after the direct one, bump the record before it, and bump the
    // mirrored one with the residual negated.  Which of the five, and at
    // which bit, is the whole of what separates the three.
    //
    // For several rounds this said they were **not** rolled into loops,
    // "a decision rather than an omission", and listed four exceptions at
    // k = 0, 4, 9 and 10.  Every one of the four has since dissolved, and
    // none of them dissolved by being argued with:
    //
    //   * "k = 0's nudge and k = 4's and k = 9's mirror bumps are written
    //     longhand" -- they were, and they are `p2_nudge` and `p2_update`
    //     now.  Two of the three had already been converted; the last two
    //     went when `p2_nudge` learned its shift instead of fixing it
    //     at 2.
    //   * "k = 4 subtracts into the residual variable itself" -- it does,
    //     through `p2_update_into`, and *nothing reads the residual
    //     afterwards*.  A write-back nobody reads is `p2_update`.
    //   * "k = 9 in the third block uses `res_t`, a different residual" --
    //     it was assigned from the block's residual two lines above and
    //     back into it two lines below.  It is the same residual under a
    //     spill's name, which is what the `const` below now says of all
    //     four.
    //   * "k = 10's mirror bump straddles the join and happens once" --
    //     true, and both arms reach it with the negated residual, so
    //     it is one line
    //     after the join rather than a name each arm has to set.
    //
    // What settled it was said to be that "hoisting the longhand cases
    // out of their place reorders counter updates against each other" --
    // `direct`, `mirror` and `rot` index one bank and can name the same
    // record for different k, so the order is not free.  That is true and
    // it is why nothing is hoisted: each loop below does the same five
    // things in the same order the unrolled text did, and the only lines
    // that moved are the four negated residuals under four names.
    P2Count* bankp = &p2_ctr[slot(bank, 0)];
    // The eleven context bits this counter bank tracks, and the three
    // counters each one names: `direct` at the context with that bit
    // flipped, `mirror` at the same one with the top bits flipped as
    // well, and `rot` at the context with its low nibble rotated by
    // `p2_ctx_rotate`.
    //
    // Sixty-three lines, thirty-three names -- `direct[0]`, `mirror[0]`,
    // `rot[0]`, `direct[1]` and so on down -- and eleven of the thirty-three
    // indices were written as a hex constant apiece: `ctxw^0x3FF0`,
    // `ctxw^0x5FF0`, `ctxw^0x77F0`.  Every one of those eleven is
    // `ctxw ^ bit ^ kBankMirrorMask`, which is what "the top bits
    // flipped" means
    // and what none of the constants said.  The last two were spelled a
    // third way again, through `x0020` and `x0010`.
    //
    // The prefetches were interleaved through the thirty-three
    // declarations in the order MSVC scheduled the loads; they are hints
    // and their order is not observable.
    static const uint32_t p2_bank_bits[11] = {
      0x4000, 0x2000, 0x1000, 0x800, 0x400, 0x200, 0x100, 0x80, 0x40,
      0x20, 0x10,
    };
    P2Count *direct[11], *mirror[11], *rot[11];
    for( int32_t k = 0; k<11; ++k ) {
      const uint32_t x = ctxw^p2_bank_bits[k];
      direct[k] = &bankp[x];
      mirror[k] = &bankp[x^kBankMirrorMask];
      rot[k] = &bankp[p2_ctx_rotate[(x>>2)&3]+(x&0xFFFFFFF3)];
      __builtin_prefetch(direct[k], 0, 1);
      __builtin_prefetch(mirror[k], 0, 1);
      __builtin_prefetch(rot[k], 0, 1);
    }
    // The nine prefetches that stood between these lines -- indices 8, 9
    // and 10 of each of the three arrays, interleaved one per statement --
    // asked for cache lines the loop above has already asked for.  They
    // were the tail of MSVC's schedule after the thirty-three loads were
    // hoisted, and re-rolling those loads into the loop left the second
    // set behind.  A hint issued twice is a hint; nine of them are noise
    // between the lines that do the work.
    const int32_t neg = -res2;
    const int32_t w_top = mir[0].weighted;
    const int32_t e_top = neg-p2_pred(w_top, (uint8_t)mir[0].rate);
    // `e_top > deadzone_hi`, which is what the five siblings of this line
    // in this file already say.  It arrived as the x86 flags the compare
    // sets -- `!(lt ^ of | zf)` over `__OFSUB__` -- because signed
    // less-than *is* `SF != OF`, so `(e_top-deadzone_hi<0) ^ overflow` is
    // the less-than, and not-less-and-not-equal is greater.
    const int32_t bump = 32*((e_top>deadzone_hi)-(e_top<deadzone_lo));
    uint32_t w_new = bump+e_top+2;
    // The partial store this was written as -- `LOWORD(w_new) = w_top +
    // (w_new>>2)` -- kept a high half that the next line throws away.
    // Sixteen bits in and sixteen bits out.
    mir[0].weighted = (int16_t)(w_top+(w_new>>2));
    // The same shape as the `||` above and the same unfolding: the
    // record after the mirrored counter is nudged whenever `lowbits < 3`,
    // and *then* the low bits choose between the two walks.  The chain
    // set `go` from `lowbits <= 0` in its middle and returned it at its
    // end, so the branch this reads was decided four terms before the
    // `if` could see it.
    //
    // `res_c = res_s` was the third term; both arms opened with it as
    // well, and with one name for the residual it says nothing at all.
    if( lowbits<3 )
      p2_nudge(&mir[1], -res2);
    if( lowbits<=0 ) {
      // The first arm owns both ends and never touches the record
      // before the direct counter, so it is its own loop.
      for( int32_t k = 0; k<11; ++k ) {
        p2_update(direct[k], res2, 2);
        p2_update(rot[k], res2, 3);
        p2_nudge(&direct[k][1], res2);
        p2_update(mirror[k], -res2, 3);
      }
    } else {
      p2_nudge(&mir[-1], -res2, 3);
      p2_update(direct[0], res2, 2);
      p2_update(rot[0], res2, 3);
      // `lowbits < 3` was assigned into `go` twice on the way here and
      // read once.  This arm runs at `lowbits` of 1, 2 or 3, so the flag
      // is "not the last value", and it is the one line of five that
      // separates the two passes `walk_bank_bits` can make.
      walk_bank_bits(direct, mirror, rot, res2, lowbits<3);
      // The one counter whose update straddles the join.  Both arms
      // reached it with the negated residual under a name of their own.
      p2_update(mirror[10], -res2, 3);
    }
  }
}

uint32_t AltP2Block::alt_p2_model(int32_t sample_in, uint8_t code, int32_t resid_in) {
  int32_t ctx_lo = ctx&0xF;
  int32_t sample16 = 16*sample_in;
  // This row's record and the one above it, read once.  Nothing below
  // advances a cursor until the five `++cursor[k]` a dozen lines down, and
  // the decompilation had a `cursor0` alias for two of the sixteen reads
  // and the member expression for the other fourteen.
  P2Ctx *const c0 = cursor[0];
  P2Ctx *const c1 = cursor[1];
  c0->val = 16*sample_in;
  c0->dval = c0->val-c0->dval;
  c0[1].dval = 0;
  // The deadzone half-width for this context, and where the residual falls
  // against it.  `dead` climbs 0, 1, 2, 4 with the context's low nibble;
  // `sign` is 2 below the band, 1 inside or below the top, 0 above -- a
  // three-way split written as two comparisons summed.
  //
  // The decompilation had `dead` computed twice, both times as three sign
  // shifts of `(K-ctx_lo)`, and negated the unsigned sum before casting it to
  // `int32_t` -- which is `-dead` only because the conversion is
  // implementation-defined in the way gcc happens to define it.
  const int32_t dead = (ctx_lo>6)+(ctx_lo>4)+2*(ctx_lo>9);
  c0->sign = (resid_in<=dead)+(resid_in< -dead);
  c0->mag = abs32(resid_in);
  float sample = (float)sample16;
  int32_t dl = sample16-c0[-1].val;
  c0->dleft = abs32(dl);
  int32_t du = sample16-c1->val;
  c0->dup = abs32(du);
  int32_t dul = sample16-c1[-1].val;
  c0->dupleft = abs32(dul);
  int32_t dur = sample16-c1[1].val;
  c0->dupright = abs32(dur);
  int32_t resid = (int16_t)(sample16-pred_prev);
  c0->err = resid;
  c0->aerr = abs32(resid);
  NbRow *wrow_cur = nb_cur;
  NbRow *wrow_b = cur[-1];
  // `NbRow::predict` answers with three floats through `bias[0..2]`: the centre
  // the filter's features are measured against, the *mixed* prediction of the
  // six weight sets, and this set's *own*.  They arrived as `n2_bias`, `bias1`
  // and `bias2`, which say where they were fetched from and not what they are.
  const float centre = bias[0];
  const float mixed = bias[1];
  const float own = bias[2];
  const float err_own = sample-own;
  const float d_pred = mixed-own;
  // The confidence that reconciles the two predictions, and the reason row 14
  // exists.  `cov` follows the covariance between the sample's distance from
  // this set's own prediction and the two predictions' disagreement; `var`
  // follows the variance of that disagreement.  Both are running averages at a
  // rate of 0.001, and `var` carries a floor of 1e-4 -- `ms1`, computed nine
  // lines above its use -- so the ratio below cannot divide by a variance that
  // has decayed away.
  //
  // `cov` is then clamped into `[0.1 * var, var]` before it is stored, which is
  // what stops the ratio running away in either direction, and `conf` lands in
  // [0, 2].  It scales both the correction fed to `nlms_track_two_rows` and the
  // step that call takes, so a set whose disagreement with the mix carries no
  // information about the error is corrected less rather than more.
  //
  // The three were `ms_a`, `ms_b` and `ms_b10` -- the last of which is not
  // `0.1 * ms_b` by the time it is stored, only where it starts.  The
  // parenthesisation is preserved exactly: float addition is not associative
  // and the gate is the only thing that would have said so.
  const float cov = (((err_own*d_pred)-wrow_cur->w[14][0])*0.001f)+wrow_cur->w[14][0];
  const float var = (wrow_cur->w[14][1]+0.000099999997f)+(((d_pred*d_pred)-wrow_cur->w[14][1])*0.001f);
  wrow_cur->w[14][1] = var;
  // **Not a min, and the one `fminf` in the program sits inside it.**  Stage 2
  // Section 1.6 asks for this said out loud, because the shape invites the
  // rewrite: the false branch keeps `0.1f * var` rather than either operand, so
  // `clamp(cov, 0.1f*var, var)` is what it *does* and `min(var, cov)` is not.
  // Written as a clamp it would read better and mean the same thing; written as
  // a min it would be wrong.  It is left in the source's own shape.
  float cov_kept = 0.1f*var;
  if( cov_kept<=cov )
    cov_kept = fminf(var, cov);
  wrow_cur->w[14][0] = cov_kept;
  const float conf = (1.0f-(cov_kept/(var+576.0f)))*2.0f;
  nlms_track_two_rows(wrow_cur, wrow_b, (sample-mixed)*2.5999999f, err_own*conf, conf);
  nlms_predict_and_correct(wrow_cur, sample, centre);
  *cur = nb_cur;
  ++cur;
  ++above;
  code_banks(sample16);
  int32_t ctx_l = ctx;
  ++cursor[0];
  P2Freq *frec = &freq[ctx_l];
  ++cursor[1];
  ++cursor[2];
  ++cursor[3];
  ++cursor[4];
  // The step of the record this sample was coded against.  Below 17 the context
  // has settled far enough that none of the updates below run, and the whole
  // body was that test's `then` arm -- one `if` around ninety lines, three deep
  // before anything happened.  As an early return the rest is one level out,
  // and the three values that one local carried are three names.
  const uint32_t step0 = frec[0].step;
  if( step0<=0x10 )
    return step0;
  int32_t is_dec = code&1;
  int32_t ctx15 = ctx_l&0xF;
  uint32_t pair_ctx = ctx_pair[code&1];
  // One neighbouring record nudged: rescale it, then add a fraction of its
  // own step to the side `is_dec` names -- or to side 0 when this symbol
  // carried no residual.  `num/16` is that fraction.
  //
  // This was a label and a backward `goto`, and the comment above it said a
  // structured form needed a loop and a flag to say which way the block had
  // been entered.  It needs neither.  The residual coder below runs on every
  // path that gets this far, and the only thing the two entries disagreed
  // about is whether the record *below* the context had been nudged first --
  // which is `ctx15 > 0`, the condition already sitting under the jump.  The
  // two nudges are the same six lines twice over, differing in which
  // neighbour and in the fraction.
  auto nudge = [&](int32_t at, uint32_t num, uint32_t pair_at) {
    // `bin` is the record this nudge moves.  It was `mir_top` -- a
    // `uint16_t*` over a `P2Freq`, where index 1 is `f[0]` and `3 - is_dec`
    // is `f[2]` or `f[1]` -- declared at the top of the function and assigned
    // thirteen times, each with a `P2Freq*` cast written on it.  Twelve of
    // those thirteen are now `p2_freq_add` and `p2_bump_sides`; this is the
    // survivor, and it is a local.
    P2Freq*const bin = &frec[at];
    p2_rescale(bin);
    const uint32_t step10 = (num*(uint32_t)frec[at].step)>>4;
    if( code ) {
      bin->f[2-is_dec] += step10;
      update_binary_pair(model_strip(pair_at), (code-1)>>1);
    } else {
      bin->f[0] += step10;
    }
  };
  if( ctx15<15 )
    nudge(1, 10, pair_ctx+1);
  if( ctx15>0 )
    nudge(-1, 13, pair_ctx-1);
  // The reload of `frec` that stood between the two nudges was `&freq[ctx]`,
  // which is what it already held: `ctx` is written in `code_banks` and in
  // the table set-up, both above this, and nothing between the two nudges
  // touches it.
  if( freq[ctx].step<=0x1Au )
    return ctx;
  int32_t fold_sel = 2-(fold[(uint8_t)-resid_in]&1);
  if( !fold[(uint8_t)-resid_in] )
    fold_sel = fold[(uint8_t)-resid_in];
  // The alternate context: the low six bits of the live one, with all five
  // features read the other way round above it.  `AltP1Block::update_model`
  // builds its own the same way over nine features and five bits.
  //
  // The weights stay a sum for the reason they do there -- each is a table
  // entry already scaled by its feature's stride -- and the parentheses round
  // the last two terms were the register allocator's, not a grouping.
  const uint32_t step_ctx = CtxIdx{}.bits<0, 6>(ctx).raw(mixer_rev(ctx_w, 5));
  P2Freq *frec_step = &freq[step_ctx];
  // The record after the step's and the record before it, each bumped when
  // the level leaves room for it.  This was one `if` with an empty `else`, a
  // `goto` out of its middle to a label below the second bump, and the second
  // bump reached by falling past that label -- three ways of saying two
  // independent conditions, which is what `p2_bump_sides` is.
  p2_bump_sides(frec_step, ctx15, 0, fold_sel, 4, 4);
  if( code ) {
    int32_t n2_half = (code-1)>>1;
    int32_t hi_nibble = (uint8_t)pair_ctx&0xF0;
    if( hi_nibble>=0xF0||(update_binary_pair(model_strip(pair_ctx+16), n2_half), hi_nibble>0) ) {
      update_binary_pair(model_strip(pair_ctx-16), n2_half);
    }
  }
  P2Freq *prec0 = &frec_step[0];
  p2_freq_add(prec0, fold_sel, 6);
  if( !plane_idx||freq[ctx].step>0x100u ) {
    // The mirrored side of the three-way counter -- 0 and 2 swap, 1 stays --
    // or side 0 when the symbol carried no residual.  `sel_alt` was a temporary
    // that held the answer for one line before `is_dec` took it.
    is_dec = code ? 2-is_dec : 0;
    // The five context banks, bumped.  `bump_bank` is one bank's worth; the
    // five were written out in full, thirty-six lines apiece, and its comment
    // says what they had in common.
    for( int32_t k = 0; k<5; ++k )
      bump_bank(k, step_ctx, ctx15, fold_sel, is_dec);
  }
  return step_ctx;
}
