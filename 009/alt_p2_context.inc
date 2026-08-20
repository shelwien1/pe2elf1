// alt_p2_context.inc -- the predictor-2 model's context: which of its counters the next
// sample is coded against.

// The two low fields of every bank context: the running prediction quantised
// against three thresholds into bits 13-14, and the activity sum against three
// more into bits 11-12.  Six of these, and the thresholds are the model --
// what is not the model is the shape around them, written out six times.
//
// Two of the six spelled the sum's three tests `(uint32_t)(K-sum)>>31`, which
// is the sign bit of `K-sum`, which is `sum > K`.  The same predicate as the
// other four, in the form MSVC left when it compiled a comparison one way here
// and another way there.  The identity is exact for the values these take --
// `sum_all` is a sum of magnitudes and `K` is at most 58, so `K-sum` cannot
// overflow -- and it was checked against the other spelling before the two
// were written as one.
static inline int32_t ctx_quant(int32_t run, int32_t r0, int32_t r1, int32_t r2,
                                int32_t sum, int32_t s0, int32_t s1, int32_t s2) {
  // Two two-bit counts, at 13 and 11: how many of the three run
  // thresholds the run passes, and how many of the three sum thresholds
  // the sum does.  Each is a sum of three comparisons, so 0..3 exactly.
  return CtxIdx{}.bits<13, 2>((run>r0)+(run>r1)+(run>r2))
                 .bits<11, 2>((sum>s0)+(sum>s1)+(sum>s2));
}

// How many of a run of thresholds a value clears: `n > scale * t[k]`, for `k`
// from `hi` down to `lo`.  Four counts at the head of `alt_p2_context` --
// thirteen comparisons between them, every one written out -- and the two
// shapes are the same one: `gA` and `gB` compare against the threshold and
// `gC` and `gD` against a multiple of it, so the scale is a parameter and is
// 1 for the first two.
static inline int32_t over_thresholds(int32_t n, const int32_t* t,
                                      int32_t lo, int32_t hi, int32_t scale) {
  int32_t over = 0;
  for( int32_t k = hi; k>=lo; --k )
    over += n>scale*t[k];
  return over;
}

// The five ratios that put a sample's activity in one of six bands.  These
// were five literals inside five comparisons, which is where a table goes when
// nobody writes it down.
static const int32_t p2_band_edges[5] = { 43, 17, 9, 5, 2 };

// The gradient predictor: this sample's neighbour plus that one's, less the
// sample between them.  Forty-four of the sixty filter inputs below are this
// shape over three of eleven cursors, and spelled out they are indexed
// subscripts three to a line -- which is exactly the form in which two arms of
// the block can be textually different and numerically the same.  Two of them
// were: the fourth input of the no-reference arm and the third of the
// single-plane one differ only in a pair of parentheses the register allocator
// left behind.
static inline int32_t grad(int32_t a, int32_t b, int32_t c) {
  return a+b-c;
}

// A row cursor bound to the field this block reads it through, so that a filter
// input is `grad(v0(-2), a1(2), a1(0))` and not three subscripts with a field
// name repeated on each.  Sixty inputs times three of those is what the block
// was.
//
// Fifteen of these are needed -- five rows by `dval` and by `val` and the two
// reference planes' three each -- and fifteen lambdas is a run `dupblock.py`
// reports as copy, correctly: the bodies differ only in a cursor.  Two functors
// would not help either; written out, a `dval` one and a `val` one differ in a
// field name and nothing else.  A field is what a member-pointer parameter is
// for.
template<int16_t P2Ctx::*Field>
struct RowOf {
  const P2Ctx* row;
  int32_t operator()(int32_t i) const { return row[i].*Field; }
};

using DvalRow = RowOf<&P2Ctx::dval>;
using ValRow = RowOf<&P2Ctx::val>;

// The filter's 28 inputs, and the reference cursors the rest of
// `alt_p2_context` reads.  One stage of that function rather than a step in it:
// a hundred and twenty lines that put `p2_row` together and touch nothing else
// of the frame except the six cursors, which is what it hands back.
//
// The four variants of rows 4..6 stay side by side deliberately.  They are one
// block per reference count -- plane 1, plane 2 and up, plane 0 with
// references, and no references at all -- and being able to diff them against
// each other by eye is the whole reason the sixty subscripts were named `d0`,
// `v1` and so on in the first place.  Four small functions would each read
// better alone and the set of them would read worse, so the seam is drawn
// around all four and not between them.
AltP2Block::P2Refs AltP2Block::fill_row_inputs(AltP2Block* refa, AltP2Block* refb) {
  P2Ctx *const c0 = cursor[0], *const c1 = cursor[1], *const c2 = cursor[2],
        *const c3 = cursor[3], *const c4 = cursor[4];
  const DvalRow d0{c0}, d1{c1}, d2{c2}, d3{c3};
  const ValRow v0{c0}, v1{c1}, v2{c2}, v3{c3}, v4{c4};
  // The filter's 28 inputs, four to a row.  Rows 0..3 are differences of `dval`
  // -- the second difference of the plane -- at widening offsets from the
  // current pixel: row 0 the four nearest, row 1 the next ring, rows 2 and 3
  // the ones two and three records out.  Rows 4..6 are the cross-plane terms
  // and are only filled when this plane has references; without them the
  // filter sees this plane alone.
  //
  // Written out one subscript at a time because that is what the original did,
  // and left that way: the offsets are not a pattern a loop could generate --
  // `c0[-1] + c1->dval - c1[-1]` and `c1[-2] + c1->dval - c2[-2]` are
  // different shapes -- so a table of offsets would be 28 rows of data plus a
  // loop, which is the same information one indirection further away.
  p2_row[0][0] = (float)d1(0);
  p2_row[0][1] = (float)d1(1);
  p2_row[0][2] = (float)grad(d0(-1), d1(0), d1(-1));
  p2_row[0][3] = (float)grad(d0(-2), d1(0), d1(-2));
  p2_row[1][0] = (float)grad(d1(-1), d1(0), d2(-1));
  p2_row[1][1] = (float)(-3*(d2(0)-d1(0))+d3(0));
  p2_row[1][2] = (float)grad(d0(-1), d1(2), d1(1));
  p2_row[1][3] = (float)grad(d0(-2), d1(1), d1(-1));
  p2_row[2][0] = (float)(2*d0(-1)-d0(-2));
  p2_row[2][1] = (float)grad(d0(-3), d1(0), d1(-3));
  p2_row[2][2] = (float)grad(d2(0), d1(1), d3(1));
  p2_row[2][3] = (float)d0(-3);
  p2_row[3][0] = (float)grad(d1(-2), d1(0), d2(-2));
  p2_row[3][1] = (float)grad(d0(-2), d1(-1), d1(-3));
  p2_row[3][2] = (float)grad(d1(1), (d1(2)+d1(0))>>1, d2(2));
  p2_row[3][3] = (float)d3(0);
  if( refa ) {
    const P2Refs refs{ref_rows(refa->cursor), ref_rows(refb->cursor)};
    P2Ctx *const ra0 = refs.a.r0, *const ra1 = refs.a.r1, *const ra2 = refs.a.r2;
    P2Ctx *const rb0 = refs.b.r0, *const rb1 = refs.b.r1, *const rb2 = refs.b.r2;
    const ValRow a0{ra0}, a1{ra1}, a2{ra2}, b0{rb0}, b1{rb1}, b2{rb2};
    // `no_ref = has_ref == 0;` was written twice, thirty lines apart, with
    // nothing between the two touching `has_ref`.  `const` is the proof rather
    // than the claim: the second write does not compile now.  It was a reload
    // of a spilled flag, and with the block a function of its own the two
    // assignments finally sat close enough together to be seen as one.
    const bool no_ref = has_ref==0;
    if( !no_ref ) {
      const float bias_l = (float)c0->dval;
      for( int32_t j = 0; j<4; ++j )
        for( int32_t k = 0; k<4; ++k )
          p2_row[j][k] += bias_l;
    }
    const int32_t plane = (int32_t)plane_idx;
    if( plane ) {
      if( plane==1 ) {
        p2_row[4][0] = (float)(d0(0)+d1(3));
        p2_row[4][1] = (float)grad(v0(-2), a1(2), a1(0));
        p2_row[4][2] = (float)grad(v1(1), a0(-1), a1(0));
        p2_row[4][3] = (float)grad(v0(-1), a1(0), a1(-1));
        p2_row[5][0] = (float)grad(v1(1), b1(2), b2(3));
        p2_row[5][1] = (float)grad(v0(-2), b0(0), b0(-2));
        p2_row[5][2] = (float)grad(v1(0), b0(0), b1(0));
        p2_row[5][3] = (float)(v0(-2)+ra0->err);
        if( no_ref ) {
          p2_row[6][0] = (float)grad(v0(-2), a0(0), a0(-2));
          p2_row[6][1] = (float)grad(v2(0), a0(0), a2(0));
          p2_row[6][2] = (float)(grad(a2(0), a0(0), v2(0))+2*(v1(0)-a1(0)));
          p2_row[6][3] = (float)(v0(-1)+v1(-1)+a1(-2)+a0(0)-a0(-1)-a1(-1)-v1(-2));
        } else {
          p2_row[6][0] = (float)v1(0);
          p2_row[6][1] = (float)v0(-3);
          p2_row[6][2] = (float)grad(v0(-1), a0(-1), a0(-2));
          p2_row[6][3] = (float)grad(v0(-3), v0(-1), v0(-4));
        }
      } else {
        p2_row[4][0] = (float)grad(v0(-3), v0(-1), v0(-4));
        p2_row[4][1] = (float)grad(v0(-1), a0(-1), a0(-2));
        p2_row[4][2] = (float)grad(v1(1), a1(0), a2(1));
        p2_row[4][3] = (float)grad(v0(-2), a1(2), a1(0));
        p2_row[5][0] = (float)grad(v1(0), a0(-2), a1(-2));
        p2_row[5][1] = (float)grad(v1(0), a0(0), a1(0));
        p2_row[5][2] = (float)grad(v0(-2), a0(0), a0(-2));
        p2_row[5][3] = (float)grad(v0(-2), b0(0), b0(-2));
        p2_row[6][0] = (float)grad(v2(0), b0(0), b2(0));
        p2_row[6][1] = (float)(grad(b0(-2), b0(0), v0(-2))+2*(v0(-1)-b0(-1)));
        p2_row[6][2] = (float)(v0(-2)+rb0->err);
        p2_row[6][3] = (float)(v2(0)+ra0->err);
      }
    } else {
      p2_row[4][0] = (float)grad(v0(-3), v0(-1), v0(-4));
      p2_row[4][1] = (float)grad(v0(-5), v1(0), v1(-5));
      p2_row[4][2] = (float)grad(v0(-4), v1(0), v1(-4));
      p2_row[4][3] = (float)((v2(0)+3*v1(1)-4*v2(1))-(((v1(2)-v1(0)-(v3(2)-v3(0)))>>1)-v3(1)));
      p2_row[5][0] = (float)grad(v0(-2), a0(0), a0(-2));
      p2_row[5][1] = (float)grad(v1(1), a1(1), a2(2));
      p2_row[5][2] = (float)grad(v1(-2), a1(2), a2(0));
      p2_row[5][3] = (float)(grad(a0(-2), a0(0), v0(-2))+2*(v0(-1)-a0(-1)));
      p2_row[6][0] = (float)grad(v2(1), b0(0), b2(1));
      p2_row[6][1] = (float)grad(v0(-2), b0(0), b0(-2));
      p2_row[6][2] = (float)grad(v0(-1), b1(1), b1(0));
      p2_row[6][3] = (float)grad(v1(1), b1(2), b2(3));
    }
    return refs;
  } else {
    p2_row[4][0] = (float)grad(v1(3), v1(0), v2(3));
    p2_row[4][1] = (float)grad(v0(-4), v1(0), v1(-4));
    // The same expression as `p2_row[4][3]` of the no-plane arm above, which
    // spelled its inner difference `a - b - (c - d)` where this one has
    // `(a - b) - (c - d)`.  One pair of parentheses, no value: they are now
    // written the same and can be seen to be.
    p2_row[4][2] = (float)((v2(0)+3*v1(1)-4*v2(1))-(((v1(2)-v1(0)-(v3(2)-v3(0)))>>1)-v3(1)));
    p2_row[4][3] = (float)grad(v2(-1), v1(1), v3(0));
    p2_row[5][0] = (float)grad(v3(1), v1(0), v4(1));
    p2_row[5][1] = (float)v1(3);
    p2_row[5][2] = (float)grad(v0(-3), v0(-1), v0(-4));
    p2_row[5][3] = (float)grad(v0(-1), v3(0), v3(-1));
    p2_row[6][0] = (float)grad(v0(-5), v1(0), v1(-5));
    p2_row[6][1] = (float)v4(0);
    p2_row[6][2] = (float)grad(v0(-5), v0(-1), v0(-6));
    p2_row[6][3] = (float)v1(-6);
    return {};
  }
}

// The symbol context this sample is coded in, and the level index that goes
// with it.  The last stage of `alt_p2_context`: everything above it produces
// `run4`, the cascade's running prediction, and this turns that plus the
// neighbourhood's magnitudes into the five ternary selectors, the two flatness
// bits and the sixteen-way magnitude class.
//
// Ninety lines, and what it needs from the caller is `run4` and the reference
// rows -- `ctx_w`, `ctx`, `ctx_pair`, `band_lo`, `band_hi`, `has_ref`,
// `plane_idx` and the five cursors are all members.  It writes `ctx_w[].sel`,
// `ctx` and `ctx_pair[]` and answers with the level index.
int32_t AltP2Block::seat_symbol_context(int32_t run4, const P2Refs& refs) {
  P2Ctx *const c0 = cursor[0], *const c1 = cursor[1], *const c2 = cursor[2],
        *const c3 = cursor[3], *const c4 = cursor[4];
  int32_t flat_a, flat_b;
  int32_t magsum_own = c1[5].mag+c1[4].mag+c1[-3].mag+c1[-4].mag+3*(c1[2].mag+c2[1].mag)+7*c1->mag+6*c1[1].mag+c0[-6].mag+c0[-7].mag+c0[-8].mag+8*c0[-1].mag+c4[2].mag+c4[1].mag+c4->mag+c4[-1].mag+c0[-4].mag+c0[-5].mag+(c3[2].mag+c3[1].mag+c3[-1].mag+c3[-2].mag+c4[-2].mag+c2[5].mag+c2[4].mag+c2[3].mag+c2[-2].mag+c2[-3].mag)+4*(c1[-1].mag+c0[-2].mag+c2[0].mag)+2*(c1[-2].mag+c1[3].mag+c0[-3].mag+c3->mag+c2[-1].mag+c2[2].mag);
  // The two band edges, read once.  They were read one at a time between the
  // selector stores, which is where the scheduler wanted the loads and not an
  // order anything depends on: nothing here writes either member.
  const int32_t lo_band = band_lo, hi_band = band_hi;
  ctx_w[0].sel = (run4<1840)+(run4<272);
  ctx_w[1].sel = (run4-c1->val<=hi_band)+(run4-c1->val<lo_band);
  int32_t d_run4 = run4-c0[-1].val;
  bool in_band = d_run4<=hi_band;
  ctx_w[2].sel = in_band+(d_run4<lo_band);
  ctx_w[3].sel = c1->sign;
  ctx_w[4].sel = c0[-1].sign;
  int32_t magsum = magsum_own;
  int32_t magu = c2[0].mag+c1->mag;
  int32_t magl = c0[-1].mag+c0[-2].mag;
  if( refs.a.r0 ) {
    int32_t mag = refs.b.r0->mag;
    int32_t mag_ref1 = refs.b.r1->mag+refs.a.r1->mag;
    int32_t mag_ref0 = mag+refs.a.r0->mag;
    magsum = mag_ref1+magsum_own+4*mag_ref0+2*(refs.a.r0[-1].mag+refs.b.r0[-1].mag);
    flat_a = mag_ref0+magl+refs.b.r0[-2].mag+refs.b.r0[-1].mag+refs.a.r0[-2].mag+refs.a.r0[-1].mag;
    if( has_ref ) {
      // The second difference was written back over `run4`, which is a
      // register reused and not a value updated: nothing downstream wants the
      // overwritten one, and the level index below recomputed `run3 + pred4`
      // rather than reading it -- the original saying so in its own way.  With
      // its own name `run4` is written once and the index is just `run4`.
      const int32_t d_run4b = run4-c1->dval-c0[0].dval;
      if( d_run4b<lo_band||d_run4b>hi_band ) {
        const int32_t d_run4c = run4-c0[-1].dval-c0[0].dval;
        flat_b = d_run4c>=lo_band&&hi_band>=d_run4c;
      } else {
        flat_b = 1;
      }
    } else {
      flat_b = mag_ref1+mag_ref0+magu+refs.b.r2->mag+refs.a.r2->mag;
    }
    // `magsum_own = magsum;` above each of these and `magsum = magsum_own;` three
    // lines down was MSVC spilling the register `magsum` was in across the
    // reads below it and loading it straight back.  Nothing between the two
    // touches it, in either arm.  What is left of that name is `magsum_own` --
    // this plane's own sum, which is the base both arms add to and not a spill
    // of anything.
    if( plane_idx==1 ) {
      int32_t d_ra_ra1 = refs.a.r0->val-refs.a.r1->val;
      ctx_w[3].sel = (d_ra_ra1<=hi_band)+(d_ra_ra1<lo_band);
      int32_t d_ra_left = refs.a.r0->val-refs.a.r0[-1].val;
      ctx_w[4].sel = (d_ra_left<=hi_band)+(d_ra_left<lo_band);
    } else if( (int32_t)plane_idx>1 ) {
      int32_t d_rb_rb1 = refs.b.r0->val-refs.b.r1->val;
      ctx_w[3].sel = (d_rb_rb1<=hi_band)+(d_rb_rb1<lo_band);
      int32_t d_rb_left = refs.b.r0->val-refs.b.r0[-1].val;
      in_band = d_rb_left<=hi_band;
      ctx_w[4].sel = in_band+(d_rb_left<lo_band);
      int32_t ra0_val = refs.a.r0->val;
      int32_t d_ra_ra1b = ra0_val-(refs.a.r1)->val;
      in_band = lo_band<=d_ra_ra1b;
      if( in_band&&d_ra_ra1b<=hi_band ) {
        flat_a = 1;
      } else {
        int32_t d_ra_leftb = ra0_val-refs.a.r0[-1].val;
        flat_a = d_ra_leftb>=lo_band&&d_ra_leftb<=hi_band;
      }
    }
  } else {
    flat_a = magl+c0[-3].mag+c0[-4].mag+c0[-5].mag;
    flat_b = c4->mag+c3->mag+magu+c0[0].mag;
  }
  // The magnitude class, and the two arms that set it wrote `ctx15` and `ctx`
  // to the same value apiece -- four stores for one number.
  //
  // The `ctx` half of them was dead: nothing between here and the `ctx = ...`
  // at the end of this function reads the member, and that assignment is
  // unconditional.  Proved rather than argued, because a member is not
  // something `deadstore.py` follows: 12345 stored here leaves all 111 checks
  // passing, and added to the assignment at the end it fails twenty.
  const int32_t ctx15 = magsum>=960 ? 15 : nb_ctx[magsum>>3];
  const int32_t ctx_idx = clamp32((run4+7)>>4, 0, 255);
  // The magnitude context in the low nibble, under a `ctx_delta` entry that
  // occupies bits 4 and up -- the run level as `bits<4, 4>` and the plane
  // `above<8>`, built once in `alt_p2_alloc`.  `ctx15` is `nb_ctx[]`, whose
  // values run 0..15 because `p2_len_edges` has fifteen live entries, so the
  // nibble is exactly the room the entry leaves and the two cannot overlap.
  //
  // The pair is the same index four `ctx_idx` apart, which is one step of the
  // level field.
  ctx_pair[0] = CtxIdx{}.bits<0, 4>(ctx15).raw(ctx_delta[ctx_idx+4]);
  ctx_pair[1] = CtxIdx{}.bits<0, 4>(ctx15).raw(ctx_delta[ctx_idx]);
  // The magnitude context in the low nibble, two flatness bits at 4 and 5 over
  // it, and the five weight-table contributions added on top.  Those five are
  // values rather than fields -- each is a table entry pre-scaled the way
  // `AltP1Block`'s nine are -- so they stay a sum and only the rest becomes
  // fields.
  // It was written out with the terms running down from index 4, which is the
  // order the registers came free in.
  ctx = CtxIdx{}.bits<0, 4>(ctx15).bit<4>(flat_a==0).bit<5>(flat_b==0)
                .raw(mixer_fwd(ctx_w, 5));
  return ctx_idx;
}

int32_t AltP2Block::alt_p2_context(AltP2Block* refa, AltP2Block* refb) {
  CtxWeights weights;
  int32_t nb_slot;
  uint32_t ctx0, ctx1, ctx2;
  uint32_t ctx0_lo;
  // The five row cursors, read once.
  //
  // This function reached them through sixteen names bound in five places:
  // `row0p`, `cursor1`, `sub0_row` at the top; `nb0`, `nb1`, `row2c`, `nb3`
  // and `nb2` (a second name for `row2c`) interleaved with the filter inputs;
  // `nb2x`, `nb3x`, `nb4` in the no-reference arm; `cx0`..`cx4` before the
  // bank contexts.  `nb0`/`nb1` were reassigned after the bias loop and `cx2`
  // was assigned identically in both arms of the reference test, so the
  // sixteen names sat over five rows: cursor 2 alone answered to `sub0_row`,
  // `row2c`, `nb2`, `nb2x` and `cx2`.  Reading the body as if `nb2` and
  // `nb2x` were different rows is the mistake that spelling invites.
  //
  // They are safe to hoist because nothing here advances a cursor:
  // `alt_p2_model` does that, after this returns.  Checked by grep before the
  // rewrite, and by the gate after it.
  //
  // The negative subscripts stay.  `c0[-3].val` is a sliding window over a row
  // with margins, which is what the model reads; what goes is the aliasing.
  P2Ctx *const c0 = cursor[0], *const c1 = cursor[1], *const c2 = cursor[2],
        *const c3 = cursor[3], *const c4 = cursor[4];
  //
  // `DvalRow` and `ValRow` used to be built here too, for the sixty filter
  // inputs; those went to `fill_row_inputs` and the accessors went with them.
  // What is left reads `.dupleft`, `.dupright`, `.dleft`, `.dup`, `.val`,
  // `.err` and `.mag`, which is seven fields and not the two an accessor pays
  // for.
  int32_t sum_ul = 21*c2[-1].dupleft+12*c1[3].dupleft+16*c1[2].dupleft+22*c1[1].dupleft+(23*c0[-1].dupleft)+20*c1->dupleft+ctx_bias[0]+14*c0[-2].dupleft;
  int32_t sum_ur = 17*c2[-2].dupright+21*c1[2].dupright+15*c1[1].dupright+25*c1->dupright+9*c1[-1].dupright+22*c0[-1].dupright+ctx_bias[1]+19*c0[-2].dupright;
  int32_t sum4 = 17*c1[3].dleft+15*c1[2].dleft+21*c1[1].dleft+18*c1->dleft+16*c1[-1].dleft+22*c0[-1].dleft+ctx_bias[2]+19*c0[-2].dleft;
  int32_t sum_u = 14*c1[3].dup+23*c1[1].dup+19*c1->dup+25*c0[-1].dup+ctx_bias[3]+17*c0[-2].dup+15*(c3->dup+c2->dup);
  int32_t sum_all = sum4+sum_u+sum_ul+sum_ur;
  int32_t band = over_thresholds(8*sum4, p2_band_edges, 0, 4, sum_u);
  int32_t den_d = 2*c0[-2].val+2*c0[-1].val;
  int32_t num_b = c1[1].val+(c1->val+2*c0[-1].val);
  int32_t num_d = 16*(c2[2].val+c2->val+c1[1].val+c2[-1].val);
  int32_t gA = over_thresholds(sum_all, bmf_p2_thresholds[band], 9, 12, 1);
  int32_t gB = over_thresholds(num_b, bmf_p2_thresholds[band], 6, 8, 1);
  int32_t gC = over_thresholds(16*sum_ur, bmf_p2_thresholds[band], 3, 5, sum_ul);
  int32_t gD = over_thresholds(num_d, bmf_p2_thresholds[band], 0, 2, den_d);
  // Mixed radix, and only three of the five terms are bit fields.  `gB`, `gC`
  // and `gD` each sum three comparisons, so each is 0..3 and each is two bits.
  // `gA` sums *four*, so it is 0..4 -- five values with a stride of 64 -- and
  // `band` sums five, so it is 0..5 with a stride of 320, which is 5 * 64.
  // Neither is a field however it is written: `bits<6, 2>(gA)` was tried, and
  // the one neighbourhood in the corpus that reaches `gA == 4` landed in slot
  // 0 instead of 256.  Three streams moved and the gate said so.
  //
  // So they are digits, with their radices written down, and `320 * 6 = 1920`
  // is what `nb_id[]` is sized for.
  nb_slot = CtxIdx{}.bits<4, 2>(gB).bits<2, 2>(gC).bits<0, 2>(gD)
                    .digit<64, 5>(gA).digit<320, 6>(band);
  seat_nb_row(nb_slot, c0);
  // The six cursors come back rather than being read again here: they are one
  // record back from the ring, which is a thing to get right once.
  const P2Refs refs = fill_row_inputs(refa, refb);
  P2Ctx *const ra0 = refs.a.r0, *const ra1 = refs.a.r1, *const ra2 = refs.a.r2;
  P2Ctx *const rb0 = refs.b.r0, *const rb1 = refs.b.r1, *const rb2 = refs.b.r2;
  // The six weight sets `NbRow::predict` mixes: three from this row's cursor
  // and three from the row above.  `cur_l` and `up_row` were `cur` and `above`
  // read into locals so the six loads could be scheduled against each other,
  // and neither cursor moves until `alt_p2_model` advances it.
  weights.row[0] = cur[-1];
  weights.row[1] = above[1];
  weights.row[2] = above[2];
  weights.row[3] = cur[-2];
  weights.row[4] = above[0];
  weights.row[5] = cur[0];
  // Which of the four directional activity sums is smallest, once each is
  // scaled by its own weight.  The weights are model parameters and stay named
  // candidates rather than going inline.
  //
  // The original walked it with two running minima and three `if`s, and the tie
  // rule is the part that has to survive: `16*sum4 <= 14*sum_u` sends a tie to
  // mode 0, which `best01 == a` reproduces because `min32` returns `a` on
  // equality.  Checked over all 43,046,721 quadruples in [-40,40]^4 -- 0
  // disagreements -- rather than at the three boundaries by hand.
  const int32_t a = 16*sum4, b = 14*sum_u, c = 13*sum_ur, d = 11*sum_ul;
  const int32_t best01 = min32(a, b);
  int32_t mode = (best01==a) ? 0 : 1;
  if( best01>c )
    mode = 2;
  if( min32(best01, c)>d )
    mode = 3;
  const int32_t filt = nb_cur->predict((float (*)[4]) p2_row, &weights, mode);
  // A 32-bit store at `&pred_prev`, which is a `uint16_t` followed by two
  // bytes of padding: the original wrote the whole word and the top half
  // landed in the pad, which nothing reads.  So it is the low half.
  pred_prev = (uint16_t)filt;
  int32_t g3pair = c0[-1].aerr+c1->aerr;
  if( refa )
    g3pair += (rb0->aerr+ra0->aerr)>>1;
  int32_t g3sum = c1[3].aerr+c1[-1].aerr+c0[-1].aerr+c0[-3].aerr+c0[-4].aerr+c0[-2].aerr;
  // Four of the five masks are a sign test the types prove: a sum of `w` of
  // these `int16_t` members is under `w * 2^15`, so every bit from
  // `15 + ceil(log2 w)` up is a copy of the sign.  Those four are `bit<>`.
  //
  // The fifth is a sign too, and for a reason from outside that rule -- the
  // cast makes it bit 15 of a *sixteen-bit* value -- so it says `bit_of<15>`
  // instead: the bit where it is, with no claim about the rest.  Same code as
  // the mask it replaces, and the position is now the thing that generates the
  // mask rather than a hex literal beside it.  tools/ctxidx.py.
  ctx0_lo = CtxIdx{}.bit<19>(c1[1].err+c1->err+c1[-1].err+c0[-1].err < 0)
            .bit<18>(c3->err+c0[-2].err+2*c0[-4].err < 0)
            .bit<17>(c1[-3].err+c0[-3].err+c0[-5].err+c0[-7].err < 0)
            .bit<16>(c0[-1].err < 0)
            .bit_of<15>((uint16_t)c0[-3].err)
            .bits<13, 2>((filt>3536)+(filt>720)+(filt>288))
            .bits<11, 2>(((g3pair+g3sum)>752)+((g3pair+g3sum)>400)+((g3pair+g3sum)>240));
  // `bank_ctx[0] = ctx0_lo;` was here, overwritten nine lines down by the bank
  // the finished word names and read by nothing in between.  Proved rather than
  // argued: storing 12345 here leaves all 111 checks passing, and storing it at
  // the surviving assignment fails 12 of them, so the slot is live and this
  // write to it is not.
  if( refa ) {
    ctx0 = CtxIdx{}.bit<25>(rb0[-1].err < 0)
           .bit<24>(rb0->err < 0)
           .bit<23>(ra0[-1].err < 0)
           .bit<22>(rb0->err+rb0[-2].err < 0)
           .bit<21>(ra0[-2].err+ra0->err < 0)
           .bit<20>(c0[-2].err < 0)
           .raw(ctx0_lo);
  } else {
    ctx0 = CtxIdx{}.bit<25>(c1[3].err+c0[-3].err+c0[-7].err+c0[-5].err < 0)
           .bit<24>(c2[1].err+c2->err+c0[-4].err+c0[-8].err < 0)
           .bit<23>(c0[-4].err+c0[-6].err < 0)
           .bit<22>(c0[-4].err < 0)
           .bit<21>(c0[-5].err < 0)
           .bit<20>(c0[-7].err < 0)
           .raw(ctx0_lo);
  }
  const int32_t run0 = step_bank(0, ctx0, filt);
  int32_t nb4_4 = c4[4].val;
  sum_all = ((g3pair<<9)+sum_all)>>13;
  int32_t d_run0 = c0[-5].val-run0;
  int32_t d_up = nb4_4-run0;
  int32_t up5 = c1[5].val;
  int32_t d_up5 = up5-run0;
  // The six words below -- two spellings of `ctx1`, two of `ctx2`, and `ctx3`
  // and `ctx4` after them -- are `bit_of<>` almost throughout, and this is the
  // measurement that says they have to be.
  //
  // Every `expr & <one bit>` is a sign test *if* `|expr|` stays under that bit,
  // and `bit<k>(expr < 0)` is how a word says so.  These do not qualify.  Every
  // term carries a `run` -- each is the stage before it plus that stage's own
  // prediction, which `step_bank` accumulates -- and that is a prediction with
  // no type bound, while the masks reach as low as bit 15.
  //
  // Instrumented over the corpus, four of them are *not* sign tests at all:
  //
  //   ctx1  bit 15  `d_run0`                           reaches 36157
  //   ctx3  bit 15  `run2-c0[-3].val`                  reaches 35550
  //   ctx3  bit 24  `... - ((uint32_t)(...)>>1) ...`   is unsigned throughout
  //   ctx4  bit 15  `(uint16_t)run3 - ...`             reaches 69572 as an int
  //
  // and twelve more come within a factor of four of their bit, which makes them
  // sign tests by how the corpus happened to run and not by anything the code
  // says.  Rewriting any of these as `< 0` would change what the program
  // computes.  Exactly one term across the six is provable, and it is the
  // `bit<24>` in the first arm.
  //
  // So `bit_of<Pos>` is the spelling: it keeps the bit where it is and claims
  // nothing further, which is the same code the mask was and a strictly better
  // record of it.  These were `(expr) & 0x2000000` with a `// bit 25` beside
  // them, sixty-six times -- a hex literal to count out by hand and a label
  // nothing checked.  The position now *generates* the mask, so the two cannot
  // disagree, and the eleven fields of a word can be read off the chain.
  //
  // What has not changed is which terms are believed and which are proven:
  // `bit<>` and `bit_of<>` sit side by side and say which is which.
  // `tools/ctxidx.py --list` reports the split; it is 1 of 66.
  if( refa ) {
    int32_t dv_now = c0[0].dval;
    ctx1 = CtxIdx{}.bit_of<25>(c0[-1].val+ra0->val-ra0[-1].val-run0)
           .bit<24>(ra2->dval+ra1->dval-2*ra0->dval < 0)
           .bit_of<23>(dv_now+c0[-3].dval-run0-(c1[2].dval-c1[5].dval))
           .bit_of<22>(dv_now+c0[-5].dval-run0)
           .bit_of<21>(dv_now+c1->dval+rb1[2].dval-rb2[2].dval-run0)
           .bit_of<20>(dv_now+c0[-1].dval+rb0->dval-rb0[-1].dval-run0)
           .bit_of<19>(c2[-1].dval+dv_now+ra0->dval-ra2[-1].dval-run0)
           .bit_of<18>(dv_now+c1[2].dval+ra0->dval-ra1[2].dval-run0)
           .bit_of<17>(d_up5)
           .bit_of<16>(d_up)
           .bit_of<15>(d_run0)
           .raw(ctx_quant(run0, 2256, 1056, 144, sum_all, 55, 10, 24));
  } else {
    int32_t d_run0b = run0-c0[-3].val;
    ctx1 = CtxIdx{}.bit_of<25>(nb4_4-c3->val+run0-c1[4].val)
           .bit_of<24>(c1[-5].val-c1[-2].val+d_run0b)
           .bit_of<23>(d_run0b+c1[2].val-up5)
           .bit_of<22>(c4[3].val-c2[2].val+run0-c2[1].val)
           .bit_of<21>(c4[-1].val-c3->val+run0-c1[-1].val)
           .bit_of<20>(c1->val-c1[2].val+run0-c0[-2].val)
           .bit_of<19>(-d_up5)
           .bit_of<18>(-d_up)
           .bit_of<17>(run0-c4[2].val)
           .bit_of<16>(run0-c4[-3].val)
           .bit_of<15>(-d_run0)
           .raw(ctx_quant(run0, 2400, 1024, 240, sum_all, 39, 24, 11));
  }
  const int32_t run1 = step_bank(1, ctx1, run0);
  int32_t cx2_val0 = c2->val;
  int32_t cx1_val = c1->val;
  int32_t dvsum2 = c0[-2].val-run1;
  int32_t lap = c0[-2].val+run1-2*c0[-1].val;
  int32_t dtop2 = cx2_val0-run1;
  if( refa ) {
    int32_t dv1 = c1[1].dval;
    int32_t d_ra = ra0->val-run1;
    ctx2 = CtxIdx{}.bit_of<25>(c0[0].dval+c1[-1].dval-run1-(c2->dval-dv1))
           .bit_of<24>(c0[0].dval+c0[-1].dval-run1-(c1->dval-dv1))
           .bit_of<23>(rb0->val-run1+cx2_val0-rb2->val)
           .bit_of<22>(d_ra+cx2_val0-ra2->val)
           .bit_of<21>(d_ra+c1[-1].val-ra1[-1].val)
           .bit_of<20>(c0[0].dval-c0[-2].dval+2*c0[-1].dval-run1)
           .bit_of<19>(2*cx1_val-run1-cx2_val0)
           .bit_of<18>(-lap)
           .bit_of<17>(dtop2)
           .bit_of<16>(dvsum2)
           .bit_of<15>(208-rb0->val)
           .raw(ctx_quant(run1, 2576, 1280, 640, sum_all, 33, 12, 4));
  } else {
    ctx2 = CtxIdx{}.bit_of<25>(run1+3*(cx2_val0-cx1_val)-c3->val)
           .bit_of<24>(run1+c4->val-(c2[2].val+c2[-2].val))
           .bit_of<23>(c3[2].val-c2[-1].val+run1-c1[3].val)
           .bit_of<22>(c3[1].val-c1[1].val-dtop2)
           .bit_of<21>(c3->val-cx1_val-dtop2)
           .bit_of<20>(lap)
           .bit_of<19>(run1-c4[3].val)
           .bit_of<18>(c3[1].val-run1)
           .bit_of<17>(run1-c4->val)
           .bit_of<16>(run1-c3[-2].val)
           .bit_of<15>(-dvsum2)
           .raw(ctx_quant(run1, 2464, 1216, 688, sum_all, 58, 25, 13));
  }
  const int32_t run2 = step_bank(2, ctx2, run1);
  int32_t run_dv3 = run2-c0[0].dval;
  // `sum_all = -sum_all` was here, and the six thresholds below read `< -K`
  // because of it.  Nothing else used the negated value: it existed so that
  // `q9`, `q39` and `q10` -- computed from the positive one, before this line
  // -- could be added to comparisons made after it.  Those are written out
  // now, so the negation and the six sign flips cancel.
  const uint32_t ctx3 = CtxIdx{}.bit_of<25>(3*(c0[-2].val-c0[-1].val)+run2-c0[-3].val)
                    .bit_of<24>(c2[1].dval-((uint32_t)(c1[1].dval+c1[2].dval+c1[-1].dval+c1->dval)>>1)+run_dv3)
                    .bit_of<23>(c2[-3].val-c1[-2].val+run2-c1[-1].val)
                    .bit_of<22>(-(c2[2].val+run2-2*c1[1].val))
                    .bit_of<21>(c1[-2].dval-c0[-2].val+run2-c1->dval)
                    .bit_of<20>(run2-c1[3].val)
                    .bit_of<19>(run_dv3-c2[1].dval)
                    .bit_of<18>(run2-c3->val)
                    .bit_of<17>(2*run2-c1->dval-(c1->val+c0[0].dval))
                    .bit_of<16>(run2-c0[-1].dval-c0[0].dval)
                    .bit_of<15>(run2-c0[-3].val)
                    .raw(ctx_quant(run2, 2896, 1568, 592, sum_all, 37, 19, 9));
  const int32_t run3 = step_bank(3, ctx3, run2);
  int32_t dv_now4 = c0[0].dval;
  int32_t run_dv4 = run3-dv_now4;
  int32_t run_up4 = run3+c3->val;
  const uint32_t ctx4 = CtxIdx{}.bit_of<25>(run3-((uint32_t)(c1[1].val+c1[-1].val+2*c1->val)>>2))
                    .bit_of<24>(run_up4-c0[-2].val-(c3[2].dval+dv_now4))
                    .bit_of<23>(run3-2*c0[-2].dval-(dv_now4-c0[-4].dval))
                    .bit_of<22>(run3-c0[-3].dval-dv_now4-(c1->val-c1[-3].val))
                    .bit_of<21>(run_up4-dv_now4-(c1[1].val+c2[-1].dval))
                    .bit_of<20>(c2[-2].val+run3-2*c1[-1].val)
                    .bit_of<19>(c2->dval-2*c1->dval+run_dv4)
                    .bit_of<18>((run3-c0[-1].val)-(c1->val-c1[-1].val))
                    .bit_of<17>(run_dv4-c1[4].dval)
                    .bit_of<16>(run_dv4-c1[-2].dval)
                    .bit_of<15>((uint16_t)run3-(uint16_t)c0[-4].dval-(uint16_t)dv_now4)
                    .raw(ctx_quant(run3, 3056, 1952, 368, sum_all, 39, 21, 10));
  const int32_t run4 = step_bank(4, ctx4, run3);
  return seat_symbol_context(run4, refs);
}
