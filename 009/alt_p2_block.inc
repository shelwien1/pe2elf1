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
    f[0] -= f[0]>>1;
    f[1] -= f[1]>>1;
    uint32_t cut = f[2]>>1;
    f[2] -= cut;
    uint32_t budget = step;
    if( budget<=0x100 ) {
      if( step<=0x20u )
        cut = ((16-budget)>>30)&0xFFFFFFFE;
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
  // Named apart from `SymList::code_symbol`, which is a different coder in a
  // different model.  Two methods with one name is not an error the compiler
  // can see, and it made `unstale.py` measure the wrong one.
  template<int32_t f_DEC>
  int32_t code_three_way(const uint32_t* ctx_pair, int32_t sym) {
    uint32_t cum;
    uint16_t st, *slot;
    uint16_t down;
    uint16_t *base = &f[0];
    int32_t c01 = f[1]+f[0];
    uint32_t tot = c01+f[2];
    if( f_DEC ) {
      int32_t target = rc.get_freq(tot);
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
    if( f_DEC )
      rc.decode(cum, tot_1, tot);
    else
      rc.encode(cum, tot_1, tot);
    uint16_t f_before = *slot;
    if( *slot>0x4000u ) {
      uint32_t f1_old = f[1];
      uint32_t f2_old = f[2];
      f[0] -= f[0]>>1;
      f[1] = f1_old-(f1_old>>1);
      f[2] = f2_old-(f2_old>>1);
      st = step;
      if( st<=256 ) {
        if( st<=32 )
          down = ((uint32_t)(16-st)>>30)&0xFFFFFFFE;
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
      return idx+2*code_symbol_tree<f_DEC>(model_strip(ctx_pair[idx&1]), (sym-1)>>1);
    return 0;
  }

  void encode_symbol(const uint32_t* ctx_pair, int32_t sym) {
    code_three_way<0>(ctx_pair, sym);
  }

  int32_t decode_symbol(const uint32_t* ctx_pair) {
    return code_three_way<1>(ctx_pair, 0);
  }
};

static inline int32_t p2_pred(int32_t weighted, int32_t rate) {
  return (weighted+(1<<((rate+31)&31)))>>(rate&31);
}

static inline int16_t p2_bump(int32_t w2, int32_t err, int32_t shift) {
  const uint32_t kick = 32u*(uint32_t)((err>deadzone_hi)-(err<deadzone_lo));
  return (int16_t)((uint32_t)w2+((kick+(uint32_t)err+(1u<<(shift-1)))>>shift));
}

// One counter update: predict from the weight, bump it by how wrong that was.
// 115 of `alt_p2_model`'s 117 `p2_bump` sites were these three statements
// written out, with a pair of temporaries each.
//
// Eight of those 115 read the rate as `*(uint8_t*)&p->rate`, and `rate` is
// `int8_t` -- so they took the byte unsigned where this takes it signed.  That
// is the same value here and only here: `p2_pred` uses the rate as two shift
// counts and masks both with `&31`, and the two readings differ by 256, which
// is a multiple of 32.
static inline void p2_update(P2Count* p, int32_t res, int32_t shift) {
  const int32_t w = p->weighted;
  p->weighted = p2_bump(w, res-p2_pred(w, p->rate), shift);
}

// The same counter, updated in place at a fixed shift of 2 and *without*
// `p2_bump`'s dead-zone kick.  21 sites fold it into one `+=` on the next
// counter in the bank; the rounding term is the only thing they share with
// `p2_update`, and folding them together would put the kick where the original
// has none.
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
// The rescale on its own.  Two of `alt_p2_model`'s fifty sites compute the
// increment and then add it *conditionally*, so the add cannot move into the
// helper; the guard still can.
static inline void p2_rescale(P2Freq* r) {
  if( r->f[0]+r->f[1]+r->f[2]>29696 )
    r->rescale_three_way();
}

static inline void p2_freq_add(P2Freq* r, int32_t slot, uint32_t num) {
  p2_rescale(r);
  r->f[slot] += (num*(uint32_t)r->step)>>4;
}

static inline void p2_nudge(P2Count* p, int32_t res) {
  // Sixteen bits in and sixteen bits out, through `uint16_t` so that the
  // wrap is defined rather than signed overflow.
  p->weighted = (int16_t)((uint16_t)p->weighted+((uint32_t)(res-p2_pred(p->weighted, p->rate)+2)>>2));
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

struct AltP2Block {
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
  P2Count p2_ctr[163840];
  P2Freq freq[15552];
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
        float ms = fast->w[7+j][k]+(x*x-fast->w[7+j][k])*0.05f;
        fast->w[7+j][k] = ms;
        fast->w[j][k] += bmf_p2_rate[j][k]*err_fast*x/(ms+floor_fast);
        ms = slow->w[7+j][k]+(x*x-slow->w[7+j][k])*rate_slow;
        slow->w[7+j][k] = ms;
        slow->w[j][k] += bmf_p2_rate[j][k]*err_slow*x/(ms+floor_slow);
      }
  }

  // Predict this sample from the fast row, then step that row toward the error
  // it just made.  `w[14][2]` decays toward 10 and scales the step floor, so
  // the filter starts cautious and loosens; `uses` counts the samples this
  // row has seen.
  void nlms_predict_and_correct(NbRow* row, float sample, float bias0) {
    float acc[4];
    for( int32_t k = 0; k<4; ++k ) {
      acc[k] = row->w[0][k]*p2_row[0][k];
      for( int32_t j = 1; j<7; ++j )
        acc[k] += row->w[j][k]*p2_row[j][k];
    }
    const float err = sample-(bias0+bmf_hsum4(acc));
    const float ms_scale = row->w[14][2];
    for( int32_t j = 0; j<7; ++j )
      for( int32_t k = 0; k<4; ++k )
        row->w[j][k] += bmf_p2_rate[j][k]*err*p2_row[j][k]/(row->w[7+j][k]+ms_scale*529.0f);
    ++row->uses;
    row->w[14][2] = ms_scale+((10.0f-ms_scale)*0.00019999999f);
  }

  void code_banks(int32_t sample16);

  template<int32_t f_DEC>
  void alt_p2_d8_body(uint8_t* src, uint8_t* out,
                      int32_t width, int32_t height);
  int32_t alt_p2_context(AltP2Block* refa, AltP2Block* refb);
  uint32_t alt_p2_model(int32_t sample_in, uint8_t a4, int32_t resid_in);

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
    ring_advance(buf, cursor, 8);
    cursor[0][-1] = cursor[1][0];
    {
      P2Ctx*const here = cursor[0];
      P2Ctx*const up = cursor[1];
      for( int32_t i = 0; i<7; ++i ) here[-2-i] = up[1+i];
    }
    cursor[0]->dval = 0;
  }

  AltP2Block*alt_p2_alloc(int32_t img_w, int32_t plane) {
    uint32_t done;
    uint32_t j, p, n;
    plane_idx = plane;
    // One pass, not two 2x-unrolled ones.  The original walked the 163,840
    // counters twice -- once zeroing `weighted`, once seeding `rate` and `b1`
    // -- with each loop doing two records an iteration.  This runs once per
    // plane, and the difference is not measurable: 9.7s either way on `x_ep`.
    for( j = 0; j<0x28000; ++j ) {
      p2_ctr[j].weighted = 0;
      p2_ctr[j].rate = 5;
      p2_ctr[j].b1 = 2;
    }
    for( uint32_t e = 0; e<0x3CC0; ++e ) {
      freq[e].f[0] = 2048;
      freq[e].f[1] = 2816;
      freq[e].f[2] = 2816;
      freq[e].step = 4096;
    }
    int32_t dz = 4*near_lossless_q+1;
    int32_t band = 16*near_lossless_q;
    has_ref = (plane_desc[plane_desc[plane_idx].src_plane].flags&desc_has_refs)!=0;
    deadzone_hi = dz;
    deadzone_lo = -dz;
    band_lo = -band-7;
    band_hi = band+8;
    row0 = (NbRow**)bmf_new(sizeof(NbRow*)*(img_w+4));
    void* buf1 = bmf_new(sizeof(NbRow*)*(img_w+4));
    nb_weights[0].w[14][2] = 1.0f;
    row1 = (NbRow**) buf1;
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
    int32_t row_bytes = 18*(img_w<3 ? 3 : img_w)+234;
    do
      buf[row++] = (P2Ctx*)bmf_new(row_bytes);
    while( row<5 );
    memset(buf[0], 0, row_bytes);
    ctx_bias[3] = 0;
    int32_t lvl = 0;
    ctx_bias[2] = 0;
    ctx_bias[1] = 0;
    uint32_t pair = 0;
    ctx_bias[0] = 0;
    cursor[0] = buf[0]+8;
    do {
      // The plane at bit 8 and the run level as a four-bit field at 4.  The
      // level is bounded by its own table -- `p2_ctx_edges` has sixteen
      // entries and `lvl` indexes it -- so the width is a fact rather than a
      // guess.  The plane stays a `raw`: `plane_count` is
      // `((depth & 0x3F) + 7) >> 3`, which is two bits only because the header
      // check rejects depths above 32, and a mask that rests on a check
      // somewhere else is the shape that moved three streams when `gA` was
      // written as `bits<6, 2>`.
      ctx_delta[2*pair] = CtxIdx{}.raw(plane_idx<<8).bits<4, 4>(lvl);
      int32_t lvl1 = (2*pair==p2_ctx_edges[lvl])+lvl;
      ctx_delta[2*pair+1] = CtxIdx{}.raw(plane_idx<<8).bits<4, 4>(lvl1);
      int32_t bump = 2*pair+++1==p2_ctx_edges[lvl1];
      lvl = bump+lvl1;
    } while( pair<0x82 );
    int32_t len = 0;
    for( n = 0; n<0x3C; ++n ) {
      nb_ctx[2*n] = len;
      int32_t len1 = (2*n==p2_len_edges[len])+len;
      nb_ctx[2*n+1] = len1;
      len = (2*n+1==p2_len_edges[len1])+len1;
    }
    ctx = 15;
    alt_init_tables(fold, unfold);
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

  AltP2Block*alt_p2_free(int8_t do_free) {
    for( int32_t k = 0; k<5; ++k )
      free(buf[k]);
    free(row0);
    free(row1);
    if( do_free&1 )
      bmf_page_free(this);
    return this;
  }
};

// The planes both directions of the p2 model work through.  Each of
// `alt_model_p2_encode` and `alt_model_p2_decode` opened with the first of
// these and closed with the second, written out, and the closing one is the
// longer of the two duplicated blocks left in this pair after `start_row` came
// out -- thirteen lines that re-read `plane_count` from the global on every
// turn of a loop it is also the bound of.  Nothing in `alt_p2_alloc` or
// `alt_p2_free` writes `plane_count`, which is what makes the re-read say
// nothing and the `for` say the same thing as the `do`.
static void alt_p2_planes_alloc(AltP2Block** plane, int32_t width) {
  for( int32_t pl = 0; pl<plane_count; ++pl ) {
    AltP2Block* page = (AltP2Block*)bmf_page_alloc(sizeof(AltP2Block));
    plane[pl] = page ? page->alt_p2_alloc(width, pl) : nullptr;
  }
}

static int32_t alt_p2_planes_free(AltP2Block** plane) {
  for( int32_t pl = 0; pl<plane_count; ++pl )
    if( plane[pl] )
      plane[pl]->alt_p2_free(1);
  return plane_count;
}

// `NbRow::predict` writes `nb[7][0..2]` where `nb` is `p2_row`, a `[7][4]`.
// That is `bias`, and this says so in a form the compiler checks.
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
