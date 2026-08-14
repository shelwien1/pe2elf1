// alt_p1_block.inc -- the alternate model for predictor 1: its block, its context
// and its counters.  "p1" is `plane_predictor == 1` and says nothing about depth --
// measured, this codes t24's and t32's *eight-bit* planes.  MODELS.md section 6.

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
  // The two activity sums that follow it stay at the call sites, because they
  // are *not* the same at all three: `alt_p1_code`'s reads the two mirrored
  // records' magnitudes through `(int8_t)` and the other two read them as the
  // `uint8_t` they are declared.  Whether that difference can ever show is a
  // question about how large a residual magnitude gets, and it is not one this
  // extraction needed to answer.
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
      here[-3] = up[2];
      here[-2] = up[1];
      here[-1] = up[0];
    }
  }

  int32_t ctx_of(AltP1Block* nb0, AltP1Block* nb1) {
    bool pick, is_zero, is_neg;
    int32_t act_all;
    uint32_t quiet;
    int32_t guess;
    P1Ctx *cursor1 = cursor[1];
    guess = cursor1->sym;
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
    is_zero = ctx_w[3].sel==0;
    is_neg = ctx_w[3].sel<0;
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
    int32_t result = ctx_w[0].w[s0]+ctx_w[1].w[s1]+ctx_w[2].w[s2]+ctx_w[3].w[s3]+ctx_w[4].w[s4]+ctx_w[5].w[s5]+ctx_w[6].w[s6]+ctx_w[7].w[s7]+ctx_w[8].w[s8]+16*quiet+8*(ctx[3+ctx[2]]==0)+act_lvl;
    ctx[0] = result;
    return result;
  }

  void d8_encode_body(uint8_t* src, uint8_t* out) {
    int32_t code;
    __rc_begin_encode();
    if( height>0 ) {
      int32_t y = 0;
      do {
        ++y;
        advance_row();
        {
          P1Ctx *row0 = cursor[0], *row2 = cursor[2], *row4 = cursor[4];
          ctx[2] = 0;
          ctx[3] = row2[-2].mag+row4[-2].mag+row2[0].mag+row4[0].mag+row2[2].mag+row4[2].mag+row2[4].mag+row4[4].mag+row0[-4].mag+row0[-2].mag;
          ctx[4] = row2[-1].mag+row4[-1].mag+row2[1].mag+row4[1].mag+row2[3].mag+row4[3].mag+row2[5].mag+row4[5].mag+row0[-3].mag+row0[-1].mag;
        }
        if( !(width<=0) ) {
          uint8_t* q = out;
          int32_t x = 0;
          do {
            ++x;
            ctx_of((AltP1Block*)nullptr, (AltP1Block*)0);
            uint8_t guess = (uint8_t)pred;
            int32_t resid = (uint8_t)(*src-guess);
            uint8_t recon = *(fold[resid]+unfold)+guess;
            code = fold[resid];
            int32_t drift = (uint8_t)*q-(uint8_t)(recon+*q-*src);
            if( drift<-16||drift>16 ) {
              *q = *src;
              code = fold_hi[resid];
            } else {
              *q = recon;
            }
            __alt_p1_encode_symbol(&counters[ctx[0]].total, ctx[1], code);
            int32_t val = (uint8_t)*q;
            int32_t err = val-pred;
            cursor[0]->sym = val;
            cursor[0]->mag = abs32(err);
            ctx[3+ctx[2]] = ctx[3+ctx[2]]+cursor[0]->mag-cursor[0][-4].mag-(cursor[4][-2].mag-cursor[4][6].mag+cursor[2][-2].mag-cursor[2][6].mag);
            ctx[2] = ctx[2]==0;
            if( counters[ctx[0]].total<0x4000u )
              update_model();
            ++cursor[0];
            ++q;
            ++cursor[1];
            ++cursor[2];
            ++cursor[3];
            ++cursor[4];
            ++src;
          } while( x<width );
          out = q;
        }
      } while( (uint32_t)y<*(uint32_t*)&height );
    }
    __rc_end_encode();
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
    int32_t slot_f, slot_r;
    uint32_t code_f = *((uint8_t)(cursor[0]->sym-(uint8_t)pred)+fold);
    int32_t code_r = *((uint8_t)((uint8_t)pred-cursor[0]->sym)+fold);
    int32_t tree_sym = (int32_t)(code_f-5)>>1;
    slot_f = 6-(code_f&1);
    if( code_f<5 )
      slot_f = *((uint8_t)(cursor[0]->sym-(uint8_t)pred)+fold);
    slot_r = 6-(code_r&1);
    if( code_r<5 )
      slot_r = code_r;
    // `ctx_w[1].sel` and `ctx_w[2].sel` were read into `sel1_top`/`sel2_top`
    // eight lines above this sum, which made two of its nine terms look unlike
    // the other seven.  Nothing between the read and the use touches `ctx_w`.
    uint32_t ctx_alt = ctx[0]&0x1F;
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
        __update_binary_pair(model_strip(128*(slot_f&1)+ctx_up+node_up[1].escape_bias(slot_f)), (int32_t)(code_f-5)>>1);
    }
    if( result&7 ) {
      CounterNode *node_dn = &counters[result];
      int32_t ctx_dn = ctx[1]-((ctx[1]&7)!=0);
      node_dn[-1].bump(slot_f, 13);
      if( slot_f>=5 )
        __update_binary_pair(model_strip(128*(uint32_t)(slot_f&1)+(uint32_t)ctx_dn+node_dn[-1].escape_bias(slot_f)), tree_sym);
    }
    if( counters[result].total<0xCCCu ) {
      if( (result&7u)<7 ) {
        node_alt[1].bump(slot_r, 7);
      }
      if( result&7 ) {
        node_alt[-1].bump(slot_r, 5);
      }
      if( slot_f>=5 ) {
        uint32_t bias = counters[result].escape_bias(slot_f);
        uint32_t key = ctx[1]+bias+((slot_f&1)<<7);
        if( (key&0x38)>=0x38||(__update_binary_pair(model_strip(128*(slot_f&1)+ctx[1]+bias+8), tree_sym), (key&0x38)!=0) ) {
          __update_binary_pair(model_strip(key-8), tree_sym);
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
    int32_t lvl, grp, slot, wid;
    uint32_t ctr = 0;
    width = img_w;
    height = img_h;
    do
      __init_counter_node(&counters[ctr++]);
    while( ctr<0x99C60 );
    pred = 0;
    lvl = 0;
    grp = 0;
    int32_t k = 0;
    int32_t plane_hi = plane<<8;
    do {
      level_of[2*k] = lvl;
      int32_t lvl1 = (2*k==p1_level_edges[lvl])+lvl;
      group_of[2*k] = plane_hi|grp;
      level_of[2*k+1] = lvl1;
      int32_t odd = 2*k+1;
      int32_t grp1 = (2*k==p1_group_edges[grp])+grp;
      lvl = (odd==p1_level_edges[lvl1])+lvl1;
      group_of[2*k+1] = grp1|plane_hi;
      grp = (odd==p1_group_edges[grp1])+grp1;
      ++k;
    } while( (uint32_t)k<0x100 );
    uint32_t pair = 0;
    slot = 0;
    do {
      slot_of[2*pair] = 8*slot;
      int32_t slot1 = (2*pair==p1_slot_edges[slot])+slot;
      slot_of[2*pair+1] = 8*slot1;
      int32_t bump = 2*pair+++1==p1_slot_edges[slot1];
      slot = bump+slot1;
    } while( pair<0x80 );
    ctx_w[0].w[1] = 32;
    ctx_w[8].w[0] = 0;
    ctx_w[7].w[0] = 0;
    ctx_w[0].w[2] = 64;
    ctx_w[6].w[0] = 0;
    ctx_w[5].w[0] = 0;
    ctx_w[4].w[0] = 0;
    ctx_w[1].w[1] = 96;
    ctx_w[3].w[0] = 0;
    ctx_w[2].w[0] = 0;
    ctx_w[1].w[0] = 0;
    ctx_w[1].w[2] = 192;
    ctx_w[0].w[0] = 0;
    uint32_t row = 0;
    ctx_w[2].w[1] = 288;
    ctx_w[2].w[2] = 576;
    ctx_w[3].w[1] = 864;
    ctx_w[3].w[2] = 1728;
    ctx_w[4].w[1] = 2592;
    ctx_w[4].w[2] = 5184;
    ctx_w[5].w[1] = 7776;
    ctx_w[5].w[2] = 15552;
    ctx_w[6].w[1] = 23328;
    ctx_w[6].w[2] = 46656;
    ctx_w[7].w[1] = 69984;
    ctx_w[7].w[2] = 139968;
    ctx_w[8].w[1] = 209952;
    ctx_w[8].w[2] = 419904;
    do
      buf[row++] = (P1Ctx*)bmf_new(sizeof(P1Ctx)*(width+10));
    while( row<5 );
    __alt_init_tables(fold, (int8_t*)unfold);
    wid = width;
    if( width>-10 ) {
      int32_t r = 0;
      do {
        buf[4][r].sym = 72;
        buf[3][r].sym = 72;
        buf[2][r].sym = 72;
        buf[1][r].sym = 72;
        buf[0][r].sym = 72;
        buf[4][r].mag = 0;
        buf[3][r].mag = 0;
        buf[2][r].mag = 0;
        buf[1][r].mag = 0;
        buf[0][r].mag = 0;
        wid = width;
        ++r;
      } while( r<width+10 );
    }
    cursor[0] = buf[0]+wid+4;
    cursor[1] = buf[1]+wid+4;
    cursor[2] = buf[2]+wid+4;
    cursor[3] = buf[3]+wid+4;
    cursor[4] = buf[4]+wid+4;
    return this;
  }

  // Returns nothing.  The original returned `this` -- a compiler's habit of
  // leaving the object pointer in the return register, not a value anyone
  // wanted -- and after the `free(this)` above it is a dangling pointer that
  // g++ reports under `-Wuse-after-free`.  All three call sites discarded it;
  // the one that did not, `__alt_model_p1_d8_decode`, only passed it on to
  // another caller that did.
  void alt_p1_free(int8_t do_free) {
    for( int32_t k = 0; k<5; ++k )
      free(buf[k]);
    if( do_free&1 )
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
