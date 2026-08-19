// model.inc -- the main model.  One pixel in and one pixel out are the two largest
// bodies in the program, and they are methods of the block they read.

struct PixRec {
  union {
    struct {
      uint16_t sym;
      uint8_t match[6];
    };
    // The same eight bytes as two words.  `match2345` was already named; the
    // low word had only `_head[4]` and eleven sites reached it as
    // `*(uint32_t*)rec`, which is the punning this union exists to make
    // unnecessary.
    struct {
      uint32_t head4;
      uint32_t match2345;
    };
  };
};

// How many of `n` consecutive records matched their neighbour on `lane`, less
// `n` -- so 0 when every one of them did and `-n` when none did.  That is what
// each of the four gradients is seeded with at the start of a row, and `sig1`
// tests each of the four for zero.
//
// Ten copies between `code_pixel`, `decode_pixel`, `model_plane` and
// `unmodel_plane`, every one of them written out term by term with the terms
// in scheduler order.
//
// Three of the eight terms in one of them were cast to `int8_t`, and the casts
// say nothing: every write to a `match` byte in this program is a comparison,
// an `&` of two of those, a copy of another record's byte, or the `0x01010101`
// two sites store through the union -- so the byte is 0 or 1 and the two
// readings agree.
static int32_t match_seed(const PixRec* first, int32_t n, int32_t lane = 0) {
  int32_t hits = 0;
  for( int32_t i = 0; i<n; ++i )
    hits += first[i].match[lane];
  return hits-n;
}

// `grad[0]`'s five records are not consecutive: -3, -2, 2, 3, 4, stepping over
// the three around the one being coded.  Both pixel coders spell them out
// through a handful of `uint8_t` locals -- five in one and four in the other,
// with a `g_ab` that is two of them added early -- and none of those locals is
// read anywhere else.
static int32_t match_seed_split(const PixRec* at) {
  return at[-3].match[0]+at[-2].match[0]
        +at[2].match[0]+at[3].match[0]+at[4].match[0]-5;
}

// The four neighbours both pixel coders rank a symbol against, in rank order:
// north, west, north-east, north-west.
//
// They lived in the candidate array until this round -- `nb_sym[8]`, `[10]`,
// `[6]`, `[5]` in the encoder and `[0]`, `[1]`, `[3]`, `[2]` in the decoder,
// two arbitrary sets of slots for one set of values, which is why the two
// coders' openings did not look alike and why a `rank` written against one set
// could not be called from the other.
//
// The round that wrote `neighbour_rank` left this alone for a reason it
// recorded: `code_pixel` was borrowing eleven of those slots for unrelated
// values, two of them (`nb_sym[5] = run_bucket[nb_sym[4]]`, `nb_sym[6] =
// run_hit`) among the four, so "the four slots still hold the four neighbours"
// was a claim about paths rather than about the text, and it was settled by
// instrumenting -- 9,144,286 calls over three images, all four intact.  The
// round after that gave those eleven borrowed slots their own names, which
// removed the hazard and left the note behind: **between the four stores and
// the last read of them there is now no write to `nb_sym` at all in either
// body.**  So the four are four locals, by construction, and the note was
// describing a tree that no longer existed.
struct Neighbours {
  int32_t up, left, up_next, upleft;

  // Where a symbol sits among them: 1..4 for the four, 0 -- the escape -- for
  // anything else.  Five chains between the two coders ran this, each spelling
  // the four literals of its own scale into the arms.
  int32_t rank(int32_t sym) const {
    if( sym==up )
      return 1;
    if( sym==left )
      return 2;
    if( sym==up_next )
      return 3;
    if( sym==upleft )
      return 4;
    return 0;
  }
};

// The cumulative weight below a level, as the arms of those chains spell it.
// Level 4 takes it as the total less the top weight rather than summing the
// four below, and level 3 sums in the order 0, 2, 1; the weights are
// `uint16_t` widened to `int`, so no ordering of the same terms differs.
static inline int32_t cum_below(const uint16_t* w, int32_t lvl) {
  switch( lvl ) {
  case 1: return w[0];
  case 2: return w[0]+w[1];
  case 3: return w[0]+w[2]+w[1];
  case 4: return w[5]-w[4];
  }
  return 0;
}

// The six folds a context group's flag word carries: which of the five
// selector weights share a weight with which.  `tables.inc` names the bits and
// gives the order the folds compose in.
//
// Both halves of the model hoist all six out of the two loops that apply them,
// as the original does, and both spelled the six out.  The encoder's names were
// these; the decoder's were `y0`, `f_b1`, `f_b4`, `f_b0`, `f_b5`, `f_b3` -- bit
// numbers, which say where a bit is and not what it does, in the decompiler's
// declaration order rather than the order they compose in.  What the two loops
// do with the six is genuinely different, so this is the six and not the loop.
struct GroupFolds {
  int32_t w3_double, w4_to_w2, w3_to_w2, w4_to_w1, w3_to_w1, w2_to_w1;

  explicit GroupFolds(int32_t flags)
      : w3_double(flags&ctx_w3_double),
        w4_to_w2(flags&ctx_w4_to_w2),
        w3_to_w2(flags&ctx_w3_to_w2),
        w4_to_w1(flags&ctx_w4_to_w1),
        w3_to_w1(flags&ctx_w3_to_w1),
        w2_to_w1(flags&ctx_w2_to_w1) {}
};

struct ModelBlock {
  uint32_t width;
  int32_t height;
  uint32_t depth;
  uint32_t depth_raw;
  uint32_t alphabet;
  uint32_t ctx_id1_used;
  uint32_t ctx_id2_used;
  uint32_t ctx_id3_used;
  uint32_t hit;
  uint32_t ctx_state_seen;
  uint32_t bucket_idx;
  uint32_t sym_pos;
  uint32_t ctr_fallback;
  uint32_t ctr_node;
  PixRec* row_cur[10];
  FreqRec grid[65723];
  uint8_t _pad1051664[16];
  BitCtr bit_root[16];
  BitCtr bit_node[4096];
  BitCtr esc_ctr[257];
  BitCtr run_ctr[48];
  uint8_t _pad16[2];
  SymList escape;
  SymList* sel1_list;
  SymList* sel0_list;
  SymList* sel[2];
  SymList* escape_list;
  uint8_t _pad22[4];
  SymList** sel_cur;
  uint16_t* sym_word;
  uint32_t* sym_code;
  uint8_t ctx_state[64];
  uint8_t ctx_bucket[375];
  uint8_t _pad25[1];

  // Which bucket a (group, last rank, previous rank) triple names.  Three
  // digits of a mixed-radix number: fifteen context groups, and five values
  // each for the two ranks, because `Neighbours::rank` answers 1..4 for the
  // four neighbours and 0 for anything else.  15 * 5 * 5 = 375, which is the
  // size of the array above -- so the radices are not a description of this
  // index, they are what makes it fit.
  //
  // One place, because there are three sites: the two seeding walks and the
  // read in `context_ids`.  Written inline the two walks became identical for
  // three more lines than they had been and `dupblock.py` went 39 -> 42; a
  // helper is shorter than the chain it replaces, so the ratchet went back to
  // 39 and the 15/5/5 fact stopped being written twice.
  static uint32_t bucket_slot(int32_t group, int32_t last, int32_t prev) {
    return CtxIdx{}.digit<1, 15>(group).digit<15, 5>(last).digit<75, 5>(prev);
  }

  // Seat `bucket` at that slot and hand back its frequency record.  Both
  // seeding walks open their innermost loop with these two lines and diverge on
  // the next one -- one sets the five weights out, the other loops over four of
  // them -- so this is the whole of what they share and it is worth exactly
  // what it removes.
  FreqRec* seat_bucket(int32_t group, int32_t last, int32_t prev,
                       int32_t bucket) {
    ctx_bucket[bucket_slot(group, last, prev)] = (uint8_t)bucket;
    return &grid[bucket];
  }
  uint8_t* run_bucket;
  uint8_t* alpha_map;
  uint8_t grad[4];
  uint16_t sym_ctr[524288];
  SymPair group_ctr[15][65536];
  uint16_t* sym_cache;
  SymPair* pix_cur;
  uint16_t sym_rev[8192];
  uint16_t ctx_id1[192512];
  uint16_t ctx_id2[108800];
  uint16_t ctx_id3[712000];
  // The pair AltP1Block and AltP2Block already have as methods: one lays the
  // workspace out over a fresh allocation and returns `this`, the other gives
  // it back.  Defined out of line in model_workspace.inc.
  ModelBlock*layout_workspace(int32_t img_w, int32_t img_h, int32_t img_depth);
  void free_workspace();

  int32_t pixel_context(uint32_t* nb) {
    int32_t band;
    int32_t pos = sym_pos;
    uint32_t result = (nb)[pos];
    if( exclusion_mask[result]==exclusion_gen )
      return -1;
    bool near = false;
    for( band = 11; band<16; ++band )
      near |= result==nb[band];
    bool far = false;
    for( band = 16; band<32; ++band )
      far |= result==nb[band];
    // Three bits of one context word, written as a multiply chain by the
    // decompiler: whether the symbol is the immediate neighbour, whether it
    // is in bands 11..15, and whether it is in 16..31.
    int32_t near_hit = CtxIdx{}.bit<6>(result==nb[10]).bit<5>(near);
    int32_t far_hit = CtxIdx{}.bit<4>(far);
    int32_t ctx0 = far_hit+near_hit;
    ctr_node = ctx0;
    if( (far_hit+near_hit==0)&&pos>6 )
      return -1;
    SymList* sel0 = sel0_list;
    int32_t ctx1 = CtxIdx{}.bit<3>(sym_in_top((sel0[mode_symbol[1]].ent), 10, result)).raw(ctx0);
    ctr_node = ctx1;
    SymEntry *list_prev = sel0[mode_symbol[2]].ent;
    SymEntry *list_sym = sel0[result].ent;
    // The low three bits: is the symbol near the top of the previous
    // symbol's list, of its own, and of the one before that.
    int32_t fallback = CtxIdx{}.bit<2>(sym_in_top(list_prev, 4, result))
                               .bit<1>(sym_in_top(list_sym, 10, mode_symbol[2]))
                               .bit<0>(sym_in_top((sel0[mode_symbol[3]].ent), 6, result));
    int32_t ctx2 = fallback+ctx1;
    if( pos<=14||(ctx2&0xB)!=0 ) {
      ctr_node = CtxIdx{}.above<7>(pos).raw(ctx2);
      ctr_fallback = CtxIdx{}.bit<3>(pos>9).raw(fallback);
    } else {
      ctr_fallback = fallback;
      return -1;
    }
    return result;
  }

  int32_t init_tables() {
    SymList** cur;
    bool promoted;
    SymList* list, ** prev;
    int32_t recycled, hit1;
    uint32_t hit0 = hit;
    if( !hit0 ) {
      if( sel==sel_cur ) {
        if( sel[0] ) {
          sel0_list[mode_symbol[1]].add_weight(row_cur[5]->sym, 3u);
          sel0_list[row_cur[5]->sym].add_weight(mode_symbol[2], 2u);
          sel1_list[mode_symbol[1]].add_weight(row_cur[5]->sym, 4u);
          sel1_list[row_cur[5]->sym].add_weight(mode_symbol[1], 2u);
        } else {
          sel0_list[mode_symbol[2]].add_weight(row_cur[5]->sym, (sym_pos>3)+2);
        }
      } else {
        sel0_list[mode_symbol[1]].add_weight(row_cur[5]->sym, 3u);
        sel0_list[row_cur[5]->sym].add_weight(mode_symbol[2], 2u);
        sel0_list[row_cur[5]->sym].add_weight(mode_symbol[1], 1u);
        sel1_list[row_cur[5]->sym].add_weight(mode_symbol[1], 2u);
        cur = sel_cur;
        do {
          prev = cur-1;
          sel_cur = prev;
          list = *prev;
          uint16_t want = row_cur[5]->sym;
          uint32_t n_live = list->live;
          SymEntry *ent = list->ent;
          if( n_live==list->n ) {
            list->live = --n_live;
            recycled = list->ent[n_live].cnt;
          } else {
            recycled = 1;
          }
          SymEntry *slot = &ent[n_live];
          list->live = n_live+1;
          list->tot = recycled+list->tot+1;
          slot->set(want, 2);
          list->since_rescale += 4;
          if( slot!=list->ent ) {
            uint16_t sym = slot->sym;
            uint8_t cnt = slot->cnt;
            *slot = slot[-1];
            slot[-1].set(sym, cnt);
          }
          cur = sel_cur;
        } while( cur!=sel );
      }
      if( exclusion_gen==-1 ) {
        exclusion_gen = 1;
        uint8_t *buf = (uint8_t*)exclusion_mask;
        uint32_t blocks = (alphabet+15)>>4;
        do {
          bmf_zero16(buf);
          buf += 16;
          --blocks;
        } while( blocks );
        hit1 = hit;
      } else {
        ++exclusion_gen;
        hit1 = hit;
      }
      promoted = hit1&&hit1<=2;
    } else {
      promoted = hit0<=2;
      if( !promoted&&mode_symbol[3]!=mode_symbol[4] ) {
        sel0_list[mode_symbol[2]].add_weight(row_cur[5]->sym, 1u);
        hit1 = hit;
        promoted = hit1&&hit1<=2;
      }
    }
    if( !promoted ) {
      // Move-to-front over the eight-entry symbol cache: find the symbol just
      // coded, slide everything above it down one, and put it at the front.
      //
      // This was an eight-deep `if`/`else` nest with the slide unrolled into
      // every arm -- a linear search with its loop written out and its counter
      // spelled as the depth of the nesting, which is what put this body at
      // nesting depth 9.  Seven arms named the entry they had just read (`c0`
      // to `c6`) and `cache` was a second name for `sym_cache`, used
      // interchangeably between the two halves of the same arm.
      //
      // Only slots 1..6 are searched, which is the original's reach and not a
      // simplification: nothing tests slot 7.  A miss therefore lands on 7,
      // and the deepest arm's `cache[7] = c6` is exactly the store the slide
      // makes for `k == 7` -- so not-found is found-at-the-end and needs no
      // arm of its own.
      //
      // The slide runs downward because the turns touch the same places; the
      // three arms that had it as a loop already said so.
      int32_t just = row_cur[5]->sym;
      if( just!=sym_cache[0] ) {
        int32_t k = 7;
        for( int32_t i = 1; i<7; ++i )
          if( just==sym_cache[i] ) {
            k = i;
            break;
          }
        for( int32_t i = k; i>0; --i )
          sym_cache[i] = sym_cache[i-1];
        sym_cache[0] = just;
      }
    }
    pix_cur->prev = pix_cur->last;
    pix_cur->last = row_cur[5]->sym;
    {
      PixRec*const here = row_cur[5];
      PixRec*const up = row_cur[6];
      here->match[0] = here->sym==up->sym;
      here->match[1] = here->sym==here[-1].sym;
      here->match[2] = here->sym==up[1].sym;
      here->match[3] = here->sym==up[-1].sym;
      here->match[4] = here->sym==up[2].sym;
      here->match[5] = here->sym==up[3].sym;
    }
    PixRec *up1 = row_cur[6];
    PixRec *up2 = row_cur[7];
    PixRec *row = row_cur[5]+1;
    row_cur[5] = row;
    ++row_cur[8];
    ++up1;
    row_cur[6] = up1;
    ++up2;
    ++row_cur[9];
    row_cur[7] = up2;
    grad[0] += up1[4].match[0]-up1[-4].match[0];
    grad[1] += up2[4].match[0]-up2[-4].match[0];
    grad[2] += row[-1].match[1]-row[-5].match[1];
    int32_t result = row[-1].match[0]-row[-8].match[0];
    grad[3] += result;
    return result;
  }

  // The two lists a pixel is searched through, loaded, and where that search
  // resumes.  `sel_cur` walks `sel` and is left wherever the last pixel
  // stopped, so both coders reload the pair before every search -- the same
  // three lines in `code_pixel` and `decode_pixel`, and the last thing the two
  // still had in common.
  SymList** load_selectors() {
    sel[0] = &sel0_list[mode_symbol[2]];
    sel[1] = &sel1_list[mode_symbol[1]];
    return sel_cur;
  }

  // Start a row: rotate the five record rows by one, reseat the five cursors
  // seven records into the rows they now point at, and seed the six `match`
  // bytes the first samples of the row would otherwise read from the row
  // before it.
  //
  // Written out in both `model_plane` and `unmodel_plane`, and the eleven
  // lines of the rotation are `ring_advance` -- the same template the p1 and
  // p2 rings use, here over `row_cur[0..4]` with `row_cur[5..9]` as its
  // cursors.  The copies spelled it through five named temporaries, which is
  // what hid the shape: `row_cur[5]` is `row_cur[0] + 7` and so on down.
  void start_row() {
    row_cur[5]->match[1] = row_cur[5][-1].sym==0;
    row_cur[5]->match[3] = row_cur[6][-1].sym==0;
    ring_advance(row_cur, row_cur+5, 7);
    uint8_t zero = row_cur[6][1].sym==0;
    row_cur[5]->match[2] = zero;
    row_cur[5][-1].match[4] = zero;
    row_cur[5][-2].match[5] = zero;
    zero = row_cur[6][2].sym==0;
    row_cur[5]->match[4] = zero;
    row_cur[5][-1].match[5] = zero;
    row_cur[5]->match[5] = row_cur[6][3].sym==0;
  }

  // The twenty-two neighbours the escape context is built from, in the order
  // `pixel_context` reads them: rows 5 to 8 of the workspace at fixed offsets
  // from the sample being coded, and one from row 9.  Slots 0 and 1 are the
  // two most recent symbols seen at this position and 2..9 the symbol cache;
  // those stay at the call sites, which hold them under different names.
  //
  // `code_pixel` and `decode_pixel` each wrote the twenty-two out a line
  // apiece over the same four rows under different names -- `up5` against
  // `r6`, `r8c` against `r7` -- which is why nothing comparing text could see
  // they were the same table.
  //
  // The decompilation had them interleaved with the twenty-four loads that
  // feed them, each load under a name of its own, `h10` through `h31`, because
  // MSVC scheduled them that way.  Every one was used exactly once, and
  // nothing here writes `row_cur`, so the order is free.
  // The 32 candidate symbols this pixel is coded against, in slot order: 0 and
  // 1 the two most recent at this context, 2..9 the move-to-front cache, and
  // 10..31 the spatial neighbours widening outward.
  //
  // Both pixel coders fill it and both reached the same two members through a
  // local of their own -- `pixr` and `wq` for `pix_cur`, `cache` for
  // `sym_cache` -- and the encoder then spelled the first two slots into `w6d`
  // and `wq1` before storing them.  Six names for three members.
  // The 32 candidates offered one at a time.  Each valid one costs a single
  // context bit -- is this the symbol? -- and a hit ends the pixel; a miss
  // stamps the candidate excluded so no later list offers it again.  Answers
  // the symbol, or -1 when all 32 miss.
  //
  // The decoder stores the candidate into the record before it reads the bit
  // and the encoder reads the record to make the bit; that is the whole of the
  // difference, and it is why this answers the symbol rather than storing it.
  // On a full miss both leave the *last* valid candidate sitting in the record,
  // which the escape path below then overwrites.
  //
  // `sym_pos` is a member because `pixel_context` indexes the candidate list by
  // it, and it is still live after this returns -- `update_selector` reads
  // `sym_pos > 3`.  A hit leaves it at the hit's index and a full miss at 32,
  // which is what the two `do`/`while`s left.
  template<int32_t f_DEC>
  int32_t offer_candidates(int32_t* nb_sym) {
    for( sym_pos = 0; sym_pos<32; ++sym_pos ) {
      const int32_t sym = pixel_context((uint32_t*)nb_sym);
      if( sym<0 )
        continue;
      // `is_sym` and not `hit`: `hit` is a member of this block, carrying an
      // answer from the stage-one coder to three other methods.
      int32_t is_sym;
      if( f_DEC ) {
        is_sym = bit_node[ctr_node].decode_context_bit(&bit_root[ctr_fallback]);
        row_cur[5]->sym = sym;
      } else {
        is_sym = sym==row_cur[5]->sym;
        bit_node[ctr_node].encode_context_bit(&bit_root[ctr_fallback], is_sym);
      }
      if( is_sym )
        return sym;
      exclusion_mask[sym] = exclusion_gen;
    }
    return -1;
  }

  // The four symbols the stage-one coder already offered, and the escape,
  // stamped with this generation so the candidate walk below does not offer
  // any of them again.
  //
  // Both coders spelled `exclusion_gen` into a local and three of the four
  // `mode_symbol` slots into three more -- `msym3`, `msym2`/`excl_sym_a`,
  // `msym1`/`excl_sym_b` -- while reading the fourth straight out of the
  // table.  Eight names for five stores of one value, which is also why the
  // order does not matter: every one of them writes `exclusion_gen`.
  // The two context ids this pixel is coded in, and the bucket they hang off.
  // `sig1` is four "is this gradient flat" bits, two "did four rows above all
  // match" bits, the bucket from 10 up and `nb2`'s four underneath; `sig2`
  // narrows the interned `id1` with three more bits taken from records the
  // intern miss may have just reloaded.  Answers `id2`, and leaves the bucket
  // in `bucket_idx` where the frequency table is picked off it.
  //
  // Both pixel coders build this and the decompilation gave every value in it
  // a different name on each side: `r8`/`r1` and `r7`/`r2` for `row_cur[8]`
  // and `row_cur[7]`, `row`/`cur5` for the row cursor, `up_m0`/`up_match0`
  // for the borrowed slot, `match1` for a `cur6->match[1]` the other read
  // directly.  The decoder also named `all_up` -- the bucket and the two run
  // bits -- and folded it in with a `raw`, where the encoder wrote one chain.
  // `CtxIdx` adds rather than ORs and the terms do not overlap, so the two
  // orders are the same sum.
  //
  // The encoder cast the two run bits to `uint8_t` and the decoder did not.
  // Every `match` byte in this program is a comparison, an `&` of two of
  // those, a copy of another record's byte, or the `0x01010101` two sites
  // store through the union -- so the byte is 0 or 1, the `&` chain is 0 or 1,
  // and the two readings agree.
  //
  // The reload of the row cursor inside the miss is dead and kept: `row` and
  // `cur5` are both `row_cur[5]` on entry, nothing between reassigns either,
  // and neither is read after this returns.
  int32_t context_ids(int32_t state, int32_t cap) {
    PixRec *const cur6 = row_cur[6];
    PixRec *const r8 = row_cur[8];
    PixRec *const r7 = row_cur[7];
    PixRec *const up9 = row_cur[9];
    PixRec *const row = row_cur[5];
    const int32_t bucket_at = ctx_bucket[state+cap];
    bucket_idx = bucket_at;
    const int32_t up_m0 = cur6->match[0];
    const uint32_t nb2 = CtxIdx{}.bit<3>(row[-2].match[2])
                                 .bit<2>(row[-2].match[5])
                                 .bit<1>(row[-2].match[4])
                                 .bit<0>(row[-1].match[1]);
    const uint32_t sig1 =
        CtxIdx{}.bit<7>(grad[3]==0).bit<6>(grad[2]==0)
                .bit<5>(grad[1]==0).bit<4>(grad[0]==0)
                .bit<9>(up9->match[0]&r8->match[0]&up_m0&r7->match[0])
                .bit<8>(up9->match[1]&r8->match[1]&r7->match[1]&cur6->match[1])
                .above<10>(bucket_at).raw(nb2);
    // Both coders reloaded three things on the intern miss -- the row-6
    // cursor into a second name, the row-5 cursor, and `up_m0` -- and all
    // three are re-reads of values the intern cannot have moved: `ctx_id1` is
    // a `uint16_t` table and `row_cur` is an array of pointers beside it, so
    // the store to `ctx_id1[sig1]` reaches neither.  What the decompiler is
    // showing is the compiler reloading across a store it could not prove
    // did not alias.  The seventeen streams say the same.
    int32_t id1;
    intern_ctx(ctx_id1, sig1, ctx_id1_used, id1);
    const uint32_t sig2 = CtxIdx{}.bit<2>(cur6[1].match[3]).bit<1>(up_m0)
                                  .bit<0>(row[-1].match[5]).above<3>(id1);
    int32_t id2;
    intern_ctx(ctx_id2, sig2, ctx_id2_used, id2);
    return id2;
  }

  // Intern a context signature: a dense id per distinct signature, minted in
  // first-seen order, with `0xFFFF` the empty slot.  Answers whether the
  // signature had *not* been seen, which the two `sig1` sites need -- they
  // reload two row cursors and a neighbour slot on exactly that path.
  //
  // Four sites, two per pixel coder.  Every one of them read the table back
  // after storing rather than using the value it had just stored; that is what
  // the decompilation makes of a store followed by a reload, and it is why the
  // four looked like six lines instead of three.
  static bool intern_ctx(uint16_t* tbl, int32_t sig, uint32_t& used,
                         int32_t& id) {
    const bool minted = tbl[sig]==0xFFFF;
    if( minted )
      tbl[sig] = used++;
    id = tbl[sig];
    return minted;
  }

  // How far the north row's "matched" flags run on from here, capped by the
  // rest of the row, and the AND of the north-north row's `match[0]` over that
  // run.  Answers the length.
  //
  // Both pixel coders scan it.  `run` trails the counter by a turn -- it is the
  // last index whose flags were tested, which is what the escape context below
  // indexes with -- and the encoder mirrored the counter into a slot of the
  // candidate table at all three exits where the decoder just used its own
  // counter.  Each also reached the tail through a `goto run_copied` out of
  // the loop, which a `return` is.
  int32_t run_scan(const PixRec* up1, const PixRec* up2, int32_t limit,
                   int32_t& m_up0, int32_t& run) {
    m_up0 = up2[0].match[0];
    if( limit<=1 ) {
      run = 1;
      return 1;
    }
    int32_t n = 1;
    while( 1 ) {
      run = n;
      if( !(up1[n+2].match[1]&up1[n+2].match[0]) )
        return n;
      m_up0 = (uint8_t)(up2[run].match[0]&m_up0);
      if( ++n>=limit ) {
        run = n;
        return n;
      }
    }
  }

  void exclude_stage_one() {
    const int8_t gen = exclusion_gen;
    for( int32_t k = 1; k<5; ++k )
      exclusion_mask[mode_symbol[k]] = gen;
    exclusion_mask[no_symbol] = gen;
  }

  // The four ranked neighbours of the pixel at the cursor, and the four
  // `mode_symbol` slots that carry them for the rest of the pixel.  Both pixel
  // coders open with exactly this; they spelled it with two different sets of
  // `nb_sym` slots and so it did not read as one thing.
  Neighbours rank_neighbours() {
    const PixRec* up = row_cur[6];
    const PixRec* west = row_cur[5];
    const Neighbours n = { up->sym, west[-1].sym, up[1].sym, up[-1].sym };
    mode_symbol[1] = n.up;
    mode_symbol[2] = n.left;
    mode_symbol[3] = n.up_next;
    mode_symbol[4] = n.upleft;
    return n;
  }

  // Six match flags of the four named neighbours, as six bits.
  //
  // The decoder folded two of the six into one `raw` term, `N.match[1] +
  // 4*(NE == NW)`, which is the same word only because `match[1]` is 0 or 1 --
  // and it is: the two places that write it write a comparison.
  uint32_t match_context(const Neighbours& n, const PixRec* up,
                         const PixRec* west) const {
    return CtxIdx{}.bit<5>(west[-1].match[2]).bit<4>(west[-1].match[4])
                   .bit<3>(up[1].match[1]).bit<2>(n.up_next==n.upleft)
                   .bit<1>(west[-1].match[0]).bit<0>(up->match[1]);
  }

  // The key into the symbol-pair tables: the north neighbour's reverse-table
  // entry less whichever of the other three first disagrees with it, and less
  // the second neighbour west when all four agree.
  //
  // Both coders wrote this as a nest of three `if`s with the disagreements
  // tested inside out, so the run of "first one that differs" was spelled
  // backwards; one of them widened the table entry through an `int16_t` and
  // the other through a `uint16_t`, which the cast at the end makes the same
  // sixteen bits either way.
  uint16_t pair_key(const Neighbours& n, const PixRec* west) const {
    if( n.up!=n.left )
      return (uint16_t)(sym_rev[n.up]-n.left);
    if( n.up!=n.up_next )
      return (uint16_t)(sym_rev[n.up]-n.up_next);
    if( n.up!=n.upleft )
      return (uint16_t)(sym_rev[n.up]-n.upleft);
    return (uint16_t)(sym_rev[n.up]-west[-2].sym);
  }

  // Everything both pixel coders work out before they part: the four ranked
  // neighbours, the six-bit match context, the pair key, the context state,
  // the symbol pair, and the second-level context id -- with a third level
  // lazily allocated under it when the alphabet is small enough to want one.
  //
  // The encoder then asks where a known symbol ranks and the decoder asks
  // which rank holds a target.  *That* is where the two genuinely diverge, and
  // everything above it was these twenty-eight lines written twice.  They did
  // not read as one thing because the four neighbours sat in two arbitrary
  // sets of scratch slots; once those became `Neighbours`, what was left
  // between the two copies was three spellings -- a `bool` one of them tested
  // twice, an extra name for `sig3`'s masked form in the other, and two of the
  // six match bits folded into one `raw` term.
  //
  // `nb4` comes back by reference because both bodies rank against it again
  // further down; everything else here is a member or is dead at the return.
  int32_t open_pixel(Neighbours& nb4, const PixRec* up, const PixRec* west) {
    nb4 = rank_neighbours();
    const int32_t nb = match_context(nb4, up, west);
    const int32_t key = pair_key(nb4, west);
    // The key's row of the eight-entry cache: the key from bit 3 up, and the
    // three bits under it are the slot within the row that `sym_cache[0..7]`
    // then walks.
    sym_cache = &sym_ctr[CtxIdx{}.above<3>(key)];
    const int32_t state = ctx_state[nb];
    ctx_state_seen = state;
    SymPair*const pair = &group_ctr[state][key];
    pix_cur = pair;
    // The pair's two symbols ranked against the four neighbours, at fifteen
    // and seventy-five to the rank.  Five chains between the two coders
    // spelled the four literals of their own scale into the arms; one of them
    // reached rank 4 through an `else` that sets 60 and then takes it back,
    // and another through a `bool` it had finished with eighteen lines above.
    const int32_t cap = 15*nb4.rank(pair->last)+75*nb4.rank(pair->prev);
    int32_t id2 = context_ids(state, cap);
    if( (int32_t)alphabet<32 ) {
      // A third level under the second: four bits of the west neighbour's
      // symbol beneath the id, with the id for that pair allocated the first
      // time it is seen.  Past 53248 ids the four bits are forced to 15, which
      // merges those sixteen contexts into one rather than allocating past the
      // table.
      int32_t id3_used = ctx_id3_used;
      int32_t sig3 = CtxIdx{}.bits<0, 4>(nb4.left).above<4>(id2);
      uint16_t* id3p = &ctx_id3[sig3];
      id2 = *id3p;
      if( id2==0xFFFF ) {
        if( id3_used>53248 )
          sig3 |= 0xF;
        id3p = &ctx_id3[sig3];
        id2 = *id3p;
      }
      if( id2>=id3_used ) {
        *id3p = id3_used;
        ++ctx_id3_used;
        id2 = *id3p;
      }
    }
    return id2;
  }

  // The records a coded run covers, each taking the symbol, the head word and
  // the flag word of the one before it.
  //
  // The decoder wrote one record a turn.  The encoder's half was unrolled by
  // two, and the second copy of the three stores went through `pixp`, a second
  // name for a `pix_cur` the first copy had not moved -- so the pair loop
  // wrote the same `prev` twice and advanced the row cursor twice, which is
  // two turns of this.
  void fill_run(int32_t sym, uint32_t head_word, uint32_t flags_word,
                uint32_t n) {
    for( uint32_t k = 0; k<n; ++k ) {
      pix_cur->prev = sym;
      row_cur[5]->head4 = head_word;
      row_cur[5]->match2345 = flags_word;
      ++row_cur[5];
    }
  }

  // The four gradient seeds and the two match flags a coded run leaves behind.
  // Both pixel coders write these eight lines, and each wrapped them in a spill
  // of an unrelated local -- `idx_s` in the decoder and `runlen` in the encoder
  // -- saved above and loaded back below with nothing between touching either.
  void seed_after_run(PixRec* cur, int32_t sym) {
    PixRec*const r4 = row_cur[6];
    PixRec*const up3 = row_cur[7];
    grad[0] = match_seed_split(r4);
    grad[1] = match_seed(up3-3, 8);
    grad[2] = match_seed(cur-4, 2, 1);
    grad[3] = match_seed(cur-7, 4);
    cur[-1].match[4] = sym==r4[1].sym;
    row_cur[5][-1].match[5] = sym==row_cur[6][2].sym;
  }

  void seed_candidates(int32_t* nb_sym) {
    nb_sym[0] = pix_cur->last;
    nb_sym[1] = pix_cur->prev;
    for( int32_t c = 0; c<8; ++c )
      nb_sym[2+c] = sym_cache[c];
    load_neighbours(nb_sym);
  }

  void load_neighbours(int32_t* nb_sym) {
    const PixRec *const r5 = row_cur[5], *const r6 = row_cur[6],
                 *const r7 = row_cur[7], *const r8 = row_cur[8];
    nb_sym[10] = r5[-2].sym;
    nb_sym[11] = r6[2].sym;
    nb_sym[12] = r7[1].sym;
    nb_sym[13] = r7->sym;
    nb_sym[14] = r6[-2].sym;
    nb_sym[15] = r7[-1].sym;
    nb_sym[16] = r5[-3].sym;
    nb_sym[17] = r6[3].sym;
    nb_sym[18] = r6[4].sym;
    nb_sym[19] = r5[-4].sym;
    nb_sym[20] = r6[-3].sym;
    nb_sym[21] = r7[2].sym;
    nb_sym[22] = r8->sym;
    nb_sym[23] = r7[-2].sym;
    nb_sym[24] = r5[-5].sym;
    nb_sym[25] = r8[1].sym;
    nb_sym[26] = r6[5].sym;
    nb_sym[27] = row_cur[9]->sym;
    nb_sym[28] = r5[-7].sym;
    nb_sym[29] = r8[-1].sym;
    nb_sym[30] = r6[7].sym;
    nb_sym[31] = r7[3].sym;
  }

  // Both selector lists, one entry per symbol of the alphabet.  Written out at
  // the end of `expand_alphabet` and again in `model_plane`'s allocation, the
  // same seven lines with the same two capacities: 99 for the list that walks
  // by weight and 33 for the one that walks by symbol.
  void init_symbol_lists() {
    for( uint32_t s = 0; s<alphabet; ++s ) {
      sel1_list[s].init(99, 0);
      sel0_list[s].init(33, 0);
    }
  }

  // Everything a block needs before its first pixel that depends on the
  // alphabet: the per-symbol exclusion map seeded to one, the escape list, the
  // selector cursor, and the two selector lists.  Both plane coders wrote this
  // out; the decoder's copy read the same object under two more names.
  void seed_alphabet() {
    uint8_t* map = (uint8_t*)bmf_new(alphabet);
    alpha_map = map;
    memset(map, 1, alphabet);
    escape_list = &escape;
    escape.init(alphabet, 1);
    sel_cur = sel;
    sel1_list = new_sym_lists(alphabet);
    sel0_list = new_sym_lists(alphabet);
    init_symbol_lists();
  }

  void expand_alphabet() {
    SymList lists[16];
    int32_t left2, left;
    uint32_t n_syms, j, carry;
    uint32_t bits = depth;
    uint32_t mask = 0xFFFFFFFF>>(-(uint8_t)depth&31);
    uint32_t nbytes = (bits+7)>>3;
    clear_sym_lists(lists);
    uint32_t cap = mask+1;
    if( bits>8 )
      cap = no_symbol+1;
    uint32_t n_1 = rc_decode_flat(cap);
    alphabet = n_1+1;
    if( (int32_t)(n_1+1)<=0x2000 ) {
      void* codes = bmf_new(sizeof(uint32_t)*(n_1+1));
      // `n_syms` was re-read from `alphabet` at four points of this body, and
      // `codes_p` from `sym_code` at two more, across loops that touch
      // neither.  One read of each.
      n_syms = alphabet;
      sym_code = (uint32_t*)codes;
      for( j = 0; j<n_syms; ++j )
        sym_code[j] = j;
      if( (int32_t)depth>8 ) {
        init_byte_lists(lists, nbytes);
        if( n_syms ) {
          carry = 0;
          uint32_t s = 0;
          do {
            sym_code[s] = 0;
            for( uint32_t b = 0; b<nbytes; ++b ) {
              uint32_t piece = lists[4*b+carry].decode_symbol_list();
              carry = piece>>6;
              sym_code[s] += piece<<((8*b)&31);
            }
            carry = (uint8_t)sym_code[s++]>>7;
          } while( s<alphabet );
        }
      } else if( n_syms<=mask ) {
        lists[0].init(mask-n_syms+2, 1);
        lists[0].rescale_at = 19*lists[0].n;
        if( alphabet ) {
          int32_t run = 0;
          uint32_t s2 = 0;
          do {
            int32_t gap = lists[0].decode_symbol_list();
            sym_code[s2] = gap+run;
            run += gap+1;
            ++s2;
          } while( s2<alphabet );
        }
      }
      for( left = 15; left>=0; --left )
        free(lists[left].ent);
    } else {
      depth = 8;
      height = height*nbytes;
      expand_alphabet();
      for( left2 = 15; left2>=0; --left2 )
        free(lists[left2].ent);
    }
  }

  // One decoded row written out, at whatever width the stream is: four arms
  // chosen by `nbytes` and then, for the narrowest, by `depth`.  Answers where
  // the next row starts.
  //
  // The decompilation reached the arms with `goto list_ready` out of the row
  // loop and four `goto next_list` back into it, and the two places that write
  // a row are the two calls below -- the second reached only by the `break`
  // that a zero-width image takes, where every arm's `row_w > 0` guard is
  // false and the whole of it is a no-op.  That is why both sites can be the
  // same call: the arm `nbytes` selects there writes nothing either way.
  //
  // `row_w` is the guard the four carried and it is kept rather than dropped.
  // It is `width` re-read after the pixel loop, so on every path that decodes
  // anything it is `width` and non-zero; on the one path where it is zero it
  // is what makes this do nothing.
  // One row of symbols out at a fixed store width.  Three of `write_row`'s
  // four arms are this walk with a different cursor type -- `uint32_t`,
  // `uint16_t`, `uint8_t` -- and the narrowest of the three was the only one
  // that spelled the cast the other two make implicitly.
  template<class T>
  uint8_t* write_row_at(uint8_t* out_at) {
    T* out = (T*)out_at;
    int32_t i = 0;
    do
      *out++ = (T)sym_code[row_cur[0][i+++8].sym];
    while( (uint32_t)i<width );
    return (uint8_t*)out;
  }

  uint8_t* write_row(uint8_t* out_at, uint32_t nbytes, uint32_t row_w) {
    if( row_w<=0 )
      return out_at;
    if( nbytes==4 )
      return write_row_at<uint32_t>(out_at);
    if( nbytes==3 ) {
      SymEntry *out_ent = (SymEntry*)out_at;
      int32_t step = 0;
      do {
        uint32_t *x6 = sym_code;
        int32_t x7 = row_cur[0][step+8].sym;
        out_ent->set((uint16_t)x6[x7], (uint8_t)(x6[x7]>>16));
        ++step;
        ++out_ent;
      } while( (uint32_t)step<width );
      return (uint8_t*)out_ent;
    }
    if( nbytes==2 )
      return write_row_at<uint16_t>(out_at);
    if( depth==8 )
      return write_row_at<uint8_t>(out_at);
    // Sub-byte depths, packed high bits first.  `out_bits` starts one byte
    // *before* the row and the first symbol's `bits < 0` moves it on, which is
    // why the pre-increment is on the store and not on the cursor.
    int32_t x4 = 0, bits = 0;
    uint8_t *out_bits = out_at;
    do {
      int32_t bpp = depth;
      bits -= bpp;
      if( bits<0 ) {
        bits = 8-bpp;
        *++out_bits = sym_code[row_cur[0][x4+8].sym]<<((8-bpp)&31);
      } else {
        *out_bits |= sym_code[row_cur[0][x4+8].sym]<<(bits&31);
      }
      ++x4;
    } while( (uint32_t)x4<width );
    return out_bits;
  }

  // One plane through the main model, encoding.  The mirror of
  // `unmodel_plane_slow` above, and it was written out inside `model_plane`
  // where its inverse was a call -- 130 lines of `blk->` against
  // `blk->unmodel_plane_slow(out)`.
  //
  // That asymmetry is the same one `AltP1Block::d8_body` had, and it has the
  // same effect on the measurement: `pairshare.py` was comparing a body
  // against a call and reporting what the two *wrappers* share.  With both
  // sides named, the pair is 4 of 146 -- and those four are the allocation
  // and the free, which is where the two genuinely agree.  The bodies do not:
  // the encoder seeds fifteen context groups and the alphabet before its row
  // walk, and the decoder reads all of that back off the stream.
  void model_plane_slow(uint8_t* pixels) {
    int16_t w2n;
    uint32_t row_w;
    int32_t w3, w4, y;
    uint16_t w2;
    rc_begin_encode();
    reduce_alphabet(this, pixels);
    int32_t bucket = 0;
    int32_t g = 0;
    do {
      int32_t flags = ctx_group_flags[g];
      ctx_state[flags] = g;
      int32_t lo = 0;
      // Hoisted out of the two loops below, as the original has it.
      const GroupFolds fold(flags);
      do {
        uint32_t hi = 0;
        do {
          FreqRec* rec = seat_bucket(g, lo, hi, bucket);
          int32_t alpha = alphabet;
          w2n = 2;
          rec->w[0] = 2;
          w2 = 2;
          w3 = 2;
          w4 = 2;
          if( fold.w3_double ) {
            w3 = 4;
            w4 = 0;
          }
          if( fold.w4_to_w2 ) {
            w2 = w4+2;
            w4 = 0;
          }
          if( fold.w3_to_w2 ) {
            w2 = w3+w2;
            w3 = 0;
          }
          if( fold.w4_to_w1 ) {
            w2n = w4+2;
            w4 = 0;
            rec->w[4] = 0;
          } else {
            rec->w[4] = w4;
          }
          if( fold.w3_to_w1 ) {
            w2n += w3;
            w3 = 0;
            rec->w[3] = 0;
          } else {
            rec->w[3] = w3;
          }
          if( fold.w2_to_w1 ) {
            rec->w[1] = w2+w2n;
            w2 = 0;
            rec->w[2] = 0;
          } else {
            rec->w[2] = w2;
            rec->w[1] = w2n;
          }
          int32_t has3 = w3!=0;
          int32_t has4 = w4!=0;
          if( w2 )
            w2 = 1;
          int32_t lvl = has4+has3+w2+2;
          if( lvl<=alpha ) {
            rec->b14 = lvl;
          } else {
            rec->b14 = has4+has3+w2+1;
            rec->w[0] = 0;
          }
          bucket += rec->seed_pair(lo, hi, alpha);
          ++hi;
        } while( hi<5 );
        ++lo;
      } while( (uint32_t)lo<5 );
      uint32_t gi = 0;
      // `gctr` rather than `group_ctr`: as a free function this read
      // `blk->group_ctr[g]` and the local could take the member's name.
      SymPair* gctr = group_ctr[g];
      do {
        gctr[gi].last = 0x2000;
        gctr[gi++].prev = 0x2000;
      } while( gi<0x10000 );
      ++g;
    } while( (uint32_t)g<0xF );
    seed_alphabet();
    if( height>0 ) {
      uint16_t* word = sym_word;
      y = 0;
      do {
        int32_t y1 = y+1;
        start_row();
        PixRec *up1 = row_cur[6];
        PixRec *up2 = row_cur[7];
        ++row_cur[5];
        ++up1;
        row_cur[6] = up1;
        ++row_cur[8];
        ++up2;
        row_cur[7] = up2;
        ++row_cur[9];
        grad[0] = match_seed(up1, 5);
        grad[3] = 0;
        grad[2] = 0;
        y = y1;
        row_w = width;
        grad[1] = match_seed(up2, 5);
        if( row_w>0 ) {
          int32_t x = 0;
          do {
            ++x;
            row_cur[5][x-1].sym = word[x-1];
            row_w = width;
          } while( (uint32_t)x<width );
          word += x;
        }
        if( row_w>0 ) {
          int32_t x2 = 0;
          do {
            int32_t step = code_pixel(x2);
            init_tables();
            x2 += step;
          } while( (uint32_t)x2<width );
          y = y1;
        }
      } while( (uint32_t)y<(uint32_t)height );
    }
    rc_end_encode();
  }

  void unmodel_plane_slow(uint8_t* dst) {
    uint8_t* row[19];
    uint32_t row_w;
    uint8_t *out_at, *expand_buf;
    int32_t g0, done, lo1;
    int32_t lo, w2, w4, w2n, lvl_n, n_pix, q2, n_pix2;
      uint8_t *dst_base = &dst[-(depth<8)];
    rc_begin_decode();
    expand_alphabet();
    int32_t g1 = 0;
    int32_t g = 0;
    do {
      int32_t flags = ctx_group_flags[g];
      ctx_state[flags] = g;
      lo1 = 0;
      g0 = g;
      // The same six folds `model_plane` hoists.
      const GroupFolds fold(flags);
      lo = 0;
      do {
        lo1 = lo;
        int32_t k = 0;
        do {
          FreqRec *rec = seat_bucket(g, lo, k, g1);
          int32_t m5 = alphabet;
          for( int32_t n = 0; n<4; ++n ) rec->w[1+n] = 2;
          if( fold.w3_double ) {
            w2 = (uint16_t)(rec->w[4]+rec->w[3]);
            rec->w[3] = w2;
            w4 = 0;
            rec->w[4] = 0;
          } else {
            w2 = rec->w[3];
            w4 = rec->w[4];
          }
          if( fold.w4_to_w2 ) {
            w2n = (uint16_t)(w4+rec->w[2]);
            rec->w[2] = w2n;
            w4 = 0;
            rec->w[4] = 0;
          } else {
            w2n = rec->w[2];
          }
          if( fold.w3_to_w2 ) {
            w2n = (uint16_t)(w2+w2n);
            rec->w[2] = w2n;
            w2 = 0;
            rec->w[3] = 0;
          }
          if( fold.w4_to_w1 ) {
            rec->w[1] += w4;
            w4 = 0;
            rec->w[4] = 0;
          }
          if( fold.w3_to_w1 ) {
            rec->w[1] += w2;
            w2 = 0;
            rec->w[3] = 0;
          }
          if( fold.w2_to_w1 ) {
            rec->w[1] += w2n;
            w2n = 0;
            rec->w[2] = 0;
          }
          lvl_n = (w2!=0)+(w2n!=0)+(w4!=0)+2;
          if( lvl_n<=m5 ) {
            rec->b14 = lvl_n;
            rec->w[0] = 2;
          } else {
            lvl_n = lvl_n-1;
            rec->b14 = lvl_n;
            rec->w[0] = 0;
          }
          g1 += rec->seed_pair(lo1, k, m5);
          ++k;
        } while( k<5 );
        lo = lo1+1;
      } while( lo1+1<5 );
      int32_t gi = 0;
      SymPair* gctr = group_ctr[g0];
      do {
        gctr[gi].last = no_symbol;
        gctr[gi++].prev = no_symbol;
      } while( gi<0x10000 );
      g = g0+1;
    } while( g0+1<15 );
    uint8_t *dst_buf = dst_base;
    seed_alphabet();
    uint32_t jj = depth;
    if( jj==depth_raw ) {
      expand_buf = nullptr;
    } else {
      dst_buf = (uint8_t*)bmf_new(height*width+3);
      jj = depth;
      expand_buf = dst_buf;
    }
    uint32_t nbytes = (jj+7)>>3;
    if( height>0 ) {
      out_at = dst_buf;
      int32_t bucket = 0;
      // A `while( 1 )` whose only exit was a `break` on the last line, which is
      // the `do`/`while` it is written as.  `MINIMAL-SYNTAX.md`'s Phase 7 is
      // the second of that document's declines to have expired: it measured 32
      // of these and nought convertible, and this is the shape it named first.
      do {
        start_row();
        PixRec* lists = row_cur[6];
        PixRec* t = row_cur[7];
        ++row_cur[5];
        ++lists;
        ++row_cur[8];
        row_cur[6] = lists;
        ++t;
        row_cur[7] = t;
        ++row_cur[9];
        grad[0] = match_seed(lists, 5);
        grad[3] = 0;
        grad[2] = 0;
        row_w = width;
        grad[1] = match_seed(t, 5);
        // `row_w` is `width`, which nothing in this loop writes, so the test
        // below is the same answer on every row.  The recovery said it with a
        // `break` out of the loop, the row write repeated underneath it, and a
        // `goto` back *into* the loop at the counter -- three statements for
        // "skip the pixels and go round again".
        if( row_w>0 ) {
          int32_t x2 = 0;
          do {
            int32_t y = decode_pixel(x2);
            init_tables();
            row_w = width;
            x2 += y;
          } while( (uint32_t)x2<width );
        }
        out_at = write_row(out_at, nbytes, row_w);
        // `height` is `int32_t` and this side is already `uint32_t`, so C
        // converts the right operand too.  Written where it happens.
      } while( (uint32_t)++bucket<(uint32_t)height );
    }
    rc_end_decode();
    int32_t bpp_raw = depth_raw;
    if( (uint32_t)bpp_raw!=depth ) {
      int32_t nchunk = (bpp_raw+7)>>3;
      if( nchunk<=0 ) {
        n_pix = height*width;
      } else {
        done = 0;
        n_pix = height*width;
        int32_t chunk = n_pix/nchunk;
        if( nchunk>=6 ) {
          int32_t q5 = 0;
          uint8_t *row_at = expand_buf;
          do {
            row[q5] = row_at;
            row_at += 5*chunk;
            for( int32_t n = 0; n<4; ++n ) row[q5+1+n] = &expand_buf[chunk*(q5+1+n)];
            q5 += 5;
          } while( q5<=nchunk-6 );
          done = q5;
        }
        int32_t q1 = done;
        int32_t at = chunk*done;
        do {
          row[q1] = &expand_buf[at];
          at += chunk;
          ++q1;
        } while( q1<nchunk );
      }
      if( n_pix>0 ) {
        uint8_t *interleave_at = dst;
        q2 = 0;
        int32_t written = 0;
        do {
          uint8_t *p = row[q2];
          *interleave_at = *p;
          n_pix2 = height*width;
          ++interleave_at;
          row[q2++] = p+1;
          if( q2==nchunk )
            q2 = 0;
          ++written;
        } while( written<n_pix2 );
      }
      free(expand_buf);
    }
  }

  // The run's length, one bit per bucket level from the top down.  A bit is
  // coded only at a level that could still belong to a run shorter than `cap`,
  // and `seen` says whether this is the first bit at a level: `bucket` counts
  // *down*, so `bucket == top` is "this is the first turn" and not the
  // tautology it reads as.
  //
  // Both coders wrote this out and they differ in where the bit comes from and
  // in nothing else, down to the order of the rescale against the counter
  // bump.  The decoder counted `bucket` itself down and the encoder kept a
  // second name for it -- `bucket_at` beside `bucket`, with a comment in each
  // body explaining that the comparison of the two is not a tautology -- and
  // both spelled every value in here through a slot of the candidate table.
  //
  // The encoder's `first |= run_bit` accumulated the mask rather than the bit
  // and its counter bump then wrote `n[run_bit != 0]`; with the bit normalised
  // the two arms are the same three lines.  The return is the decoder's
  // answer; the encoder's caller already holds the length and drops it.
  template<int32_t f_DEC>
  int32_t code_run_length(int32_t bucket, int32_t cap, int32_t len) {
    const int32_t top = bucket;
    int32_t idx = 0;
    int32_t mask = 1<<(bucket&31);
    int32_t seen = 0;
    do {
      if( (mask|seen)<cap ) {
        BitCtr *ctr = &run_ctr[CtxIdx{}.bits<0, 4>(bucket)
                                .digit<16, 3>((seen==0)+(bucket==top))];
        const uint32_t bin_tot = ctr->n[0]+ctr->n[1];
        int32_t run_bit;
        if( f_DEC ) {
          run_bit = rc.decode_bit(ctr->n[0], ctr->n[1]);
        } else {
          run_bit = (len&mask)!=0;
          rc.encode_bit(ctr->n[0], ctr->n[1], run_bit);
        }
        if( ctr->limit<bin_tot )
          ctr->scale_rare();
        ctr->n[run_bit] += 8;
        if( run_bit )
          idx |= mask;
        seen |= idx&mask;
      }
      --bucket;
      mask >>= 1;
    } while( mask );
    return idx;
  }

  // Stage one missed, so the symbol is looked for among the candidates and then
  // down the selector lists.  `idx_s` is how far the run reached -- zero when
  // there was no run -- and every answer here is one past it.
  //
  // Two paths reach this and both were a `goto pixel_done`: the run coder when
  // it decoded a run but not the pixel that follows it, and stage one when its
  // level came back zero.  The label was the last of the tree's, and it forced
  // the stage-one block that sits between the two into a scope of its own,
  // because a jump may cross a scope but not an initialisation inside the
  // label's.
  int32_t decode_stage_two(int32_t* nb_sym, int32_t idx_s) {
    exclude_stage_one();
    sel[0] = nullptr;
    // The 32 candidate symbols `pixel_context` walks, in slot order.  Slots 0
    // and 1 are the two most recent symbols at this exact context, 2..9 the
    // move-to-front cache, and 10..31 the spatial neighbours widening outward.
    seed_candidates(nb_sym);
    if( offer_candidates<1>(nb_sym)>=0 )
      return idx_s+1;
    SymList **sel_p = load_selectors();
    while( 1 ) {
      if( sel_p>&escape_list )
        bmf_fatal(bmf_read_error);
      if( (*sel_p)->live ) {
        int32_t lsym = (*sel_p)->decode_symbol_list();
        row_cur[5]->sym = lsym;
        if( lsym>=0 )
          return idx_s+1;
        sel_p = sel_cur;
      }
      sel_cur = ++sel_p;
    }
  }

  int32_t decode_pixel(int32_t x) {
    // The neighbour list `pixel_context` indexes by `sym_pos`.  BMF.exe fills
    // it one slot at a time, so Hex-Rays gave each slot a member, and an
    // earlier round recovered the array beside them.  The frame struct is what
    // was left over: this array, four bytes of gap and thirty-two of trailing
    // pad, no other member, and nothing reading either.  `pixel_context`
    // indexes 0..31 and no further, so the pad held nothing back.
    alignas(16) int32_t nb_sym[32];
    uint32_t lvl_a;
    int32_t m_up0, run, bit, idx_s;
    PixRec *cur6 = row_cur[6];
    PixRec *row = row_cur[5];
    Neighbours nb4;
    int32_t id2 = open_pixel(nb4, cur6, row);
    if( row_cur[5][-1].match[1]&row_cur[5][-1].match[0] ) {
      PixRec *up1 = row_cur[6];
      PixRec *up2 = row_cur[7];
      if( (uint8_t)(up2[2].match[1]&up2[1].match[1]&up2[0].match[1]&up1[3].match[1]&up1[2].match[1]&up1[1].match[1]&up1[0].match[1]&up1[0].match[0]&(int8_t)up1[-1].match[1])&up2[3].match[1] ) {
        const int32_t idx1 = run_scan(up1, up2, width-x, m_up0, run);
        int32_t bucket = *(run_bucket+idx1);
        // Four fields and a plus-one: the bucket from bit 3 up, then
        // three bits of neighbour agreement.  The `+1` is an offset
        // into `esc_ctr` and not a field, which is why it is a `raw`.
        int32_t esc_at = CtxIdx{}.above<3>(bucket)
                                 .bit<2>(up2[run+3].match[1]&up2[run+2].match[1])
                                 .bit<1>(m_up0)
                                 .bit<0>(alpha_map[nb4.up])
                                 .raw(1);
        bit = esc_ctr[esc_at].decode_context_bit(esc_ctr);
        int32_t msym1c = mode_symbol[1];
        hit = bit;
        *(((uint8_t*)alpha_map)+msym1c) = bit;
        // `hit_a` is `hit`, and nothing between here and the end of this block
        // writes either: `hit` is a member of this record and the only writes
        // to it in the tree are this line and the two `hit = lvl` in the stage
        // -one coders.  The two `hit_a = hit` further down were the same value
        // fetched again after a spill, and one of them was the whole reason
        // the `goto` below could skip a line.
        const int32_t hit_a = hit;
        // A run of one is already decoded: the escape bit said so and the
        // length needs no bits.  That was a `goto` past the walk *and* past the
        // two lines below it -- both of which are no-ops when `idx_s` is zero,
        // which it is on exactly this path.
        idx_s = hit_a ? idx1
              : idx1>1 ? code_run_length<1>(bucket, idx1, 0)
              : 0;
        if( idx_s )
          row_cur[5][idx_s-1].sym = row_cur[5][-1].sym;
        if( idx_s>hit_a ) {
          row_cur[6] = row_cur[6]+idx_s-hit_a;
          PixRec *r8b = row_cur[8];
          PixRec *r7b = row_cur[7]+idx_s;
          row_cur[7] = r7b-hit_a;
          row_cur[8] = r8b+idx_s-hit_a;
          PixRec *rec = row_cur[5];
          row_cur[9] = row_cur[9]+idx_s-hit_a;
          rec->match2345 = 0x01010101;
          row_cur[5]->head4 = 0x01010101;
          SymPair *pixp = pix_cur;
          // The symbol, the head word and the flag word the run replicates.
          // The first two went into two slots of the candidate table and came
          // back out through `s1c`, `s3c`, `s3d` -- and `s1d` eighteen lines
          // below the block, which is what makes slot 1 more than scratch and
          // is why the run's symbol needs a name rather than a deletion.
          const int32_t run_sym = mode_symbol[1];
          pixp->prev = pixp->last;
          row_cur[5]->sym = (uint16_t)run_sym;
          pix_cur->last = (uint16_t)run_sym;
          const int32_t flags_word = row_cur[5]->match2345;
          const int32_t head_word = row_cur[5]->head4;
          ++row_cur[5];
          // The records the run covers, each taking the symbol, head word and
          // flag word of the one before it.  The original unrolled this by two
          // and carried the odd one in a tail: a pair count in a third slot,
          // `2*pairs+1` in `done`, and a tail that runs exactly when `fill` is
          // odd -- so the two paths together are `fill` turns.  `hit_a` and
          // `idx_s` were saved and restored around the pair loop, which
          // touches neither.
          //
          // Unsigned, as the tail's own comparison was; the guard above is
          // what keeps `fill` off zero.
          fill_run(run_sym, head_word, flags_word, idx_s-hit_a-1);
          seed_after_run(row_cur[5], run_sym);
        }
        if( hit_a )
          return idx_s;
        // The run was coded but the pixel was not, so what follows is the
        // second stage with `idx_s` carrying how far the run reached.  The
        // recovery jumped past the whole of stage one to say that.
        return decode_stage_two(nb_sym, idx_s);
      }
    }
    FreqRec *freq = &grid[bucket_idx];
    FreqRec *freq_tbl = &grid[id2+188];
    {
    // Decode the stage-one symbol against one record: take the target, find
    // which of its five levels holds it, let the record grow, and leave the
    // level in `hit` for the callers that read it.  Both arms below run this
    // and differ only in which record, and in what they then do to the other
    // one -- so the two wrote it out with a private name for every value in
    // it, `cum2`, `target2`, `lvl_b`, and a `freq2` that is `freq`.
    auto decode_against = [&](FreqRec* rec) {
      const int32_t total = rec->w[5];
      int32_t cum;
      const int32_t lvl = rec->find_level(rc.get_freq(total), cum);
      // Before the bump, which is what moves `w[lvl]`.
      const int32_t low = cum-rec->w[lvl];
      rec->bump(lvl);
      rc.decode(low, cum, total);
      hit = lvl;
      return lvl;
    };
    const int32_t tot = freq_tbl->w[5];
    if( tot ) {
      // `tot` was re-read after the blend for the sake of `arg_tot`, which
      // reads `w[5]` itself now.
      if( tot==1 )
        freq_tbl->blend_from(*freq);
      lvl_a = decode_against(freq_tbl);
      if( freq_tbl->b14 ) {
        --freq_tbl->b14;
        ++freq->w[5];
        ++freq->w[lvl_a];
      }
    } else {
      lvl_a = decode_against(freq);
      freq_tbl->w[5] = freq_tbl->w[lvl_a]++!=0;
    }
    }
    if( lvl_a ) {
      row_cur[5]->sym = mode_symbol[lvl_a];
      return 1;
    }
    return decode_stage_two(nb_sym, 0);
  }

  int32_t code_pixel(int32_t x) {
    // The neighbour list `pixel_context` indexes by `sym_pos`.  BMF.exe fills
    // it one slot at a time, so Hex-Rays gave each slot a member, and an
    // earlier round recovered the array beside them.  The frame struct is what
    // was left over: this array, four bytes of gap and thirty-two of trailing
    // pad, no other member, and nothing reading either.  `pixel_context`
    // indexes 0..31 and no further, so the pad held nothing back.
    alignas(16) int32_t nb_sym[32];
    uint32_t done;
    int32_t m_up0, run, amap, runlen, lvl_a;
    PixRec* cur6c, * up2;
    PixRec* cur6 = row_cur[6];
    PixRec *cur5 = row_cur[5];
    Neighbours nb4;
    int32_t id2 = open_pixel(nb4, cur6, cur5);
    PixRec *cur2 = cur5;
    PixRec* row = row_cur[5];
    int32_t m_w1b = row[-1].match[1];
    int32_t m_w0 = row[-1].match[0];
    PixRec *rowp = row;
    if( (m_w1b&m_w0)!=0&&(cur6c = row_cur[6], up2 = row_cur[7], ((uint8_t)(up2[2].match[1]&up2[1].match[1]&up2[0].match[1]&cur6c[3].match[1]&cur6c[2].match[1]&cur6c[1].match[1]&cur6c[0].match[1]&cur6c[0].match[0]&cur6c[-1].match[1])&up2[3].match[1])!=0) ) {
      const int32_t run_len = run_scan(cur6c, up2, width-x, m_up0, run);
      int32_t run_pair = (uint8_t)(up2[run+3].match[1]&up2[run+2].match[1]);
      const int32_t bucket = run_bucket[run_len];
      amap = CtxIdx{}.bit<2>(run_pair).bit<1>(m_up0)
                     .above<3>(bucket).bit<0>(alpha_map[nb4.up]);
      runlen = 0;
      if( rowp->sym==nb4.up ) {
        do
          ++runlen;
        while( runlen<run_len&&rowp[runlen].sym==nb4.up );
      }
      const int32_t run_hit = runlen==run_len;
      if( runlen>run_hit ) {
        row_cur[6] = &cur6c[runlen-run_hit];
        row_cur[7] = &up2[runlen-run_hit];
        PixRec *row_cur9 = row_cur[9];
        row_cur[8] = row_cur[8]+runlen-run_hit;
        row_cur[9] = row_cur9+runlen-run_hit;
        rowp->match2345 = 0x01010101;
        row_cur[5]->head4 = 0x01010101;
        pix_cur->prev = pix_cur->last;
        uint16_t wp = nb4.up;
        row_cur[5]->sym = wp;
        pix_cur->last = wp;
        PixRec *r3 = row_cur[5];
        uint32_t rec_word = *(uint32_t*)r3;
        // The record's flag word, saved into a slot of the candidate table and
        // read back twice below across walks that do not touch it.
        const int32_t flag_word = r3->match2345;
        cur2 = r3+1;
        row_cur[5] = r3+1;
        if( runlen-run_hit!=1 ) {
          int32_t run_left = runlen-run_hit-1;
          uint32_t half = run_left/2;
          if( run_left/2 ) {
            // `run_left` and `amap` were saved into two slots of the
            // candidate table and read straight back below, across a walk that
            // touches neither -- register spills with names.
            fill_run(nb4.up, rec_word, flag_word, 2*half);
            cur2 = row_cur[5];
            done = 2*half+1;
          } else {
            done = 1;
          }
          if( (uint32_t)run_left>(done-1) ) {
            pix_cur->prev = nb4.up;
            row_cur[5]->head4 = rec_word;
            row_cur[5]->match2345 = flag_word;
            PixRec *cur5p1b = row_cur[5]+1;
            row_cur[5] = cur5p1b;
            cur2 = cur5p1b;
          }
        }
        seed_after_run(cur2, nb4.up);
      }
      (&esc_ctr[(amap+1)])->encode_context_bit(esc_ctr, run_hit);
      int32_t msym1 = mode_symbol[1];
      uint8_t *amap_at = (uint8_t*)alpha_map;
      hit = run_hit;
      *(amap_at+msym1) = run_hit;
      if( !run_hit&&run_len!=1 )
        code_run_length<0>(bucket, run_len, runlen);
      if( hit )
        return runlen;
    } else {
      FreqRec *binp = &grid[bucket_idx];
      FreqRec *frec = &grid[id2+188];
      // The mirror of `decode_pixel`'s `decode_against`: rank the symbol among
      // the four neighbours, take the interval below that rank, let the record
      // grow, and leave the rank in `hit`.  The second arm reached the same
      // record through `grid[bucket_idx].w`, cast to the union's other member,
      // so the two chains did not look like one.
      auto code_against = [&](FreqRec* rec) {
        const int32_t lvl = nb4.rank(rowp->sym);
        const int32_t cum = cum_below(rec->w, lvl);
        const uint32_t total = rec->w[5];
        // Before the bump, which is what moves `w[lvl]`.
        const uint32_t high = rec->w[lvl]+cum;
        rec->bump(lvl);
        rc.encode(cum, high, total);
        hit = lvl;
        return lvl;
      };
      const uint32_t grid_kind = frec->w[5];
      if( grid_kind ) {
        if( grid_kind==1 ) {
          // `ip = (int32_t*)&grid[bucket_idx]` ten lines up, cast straight
          // back here and used as nothing else in between.
          frec->blend_from(*binp);
          rowp = row_cur[5];
        }
        lvl_a = code_against(frec);
        if( frec->b14 ) {
          --frec->b14;
          ++binp->w[5];
          ++binp->w[lvl_a];
        }
      } else {
        lvl_a = code_against(binp);
        frec->w[5] = frec->w[lvl_a]++!=0;
      }
      if( lvl_a )
        return 1;
      runlen = 0;
    }
    exclude_stage_one();
    sel[0] = nullptr;
    // The same 32 candidates as `decode_pixel`'s, in slot order: 0 and 1 the
    // two most recent symbols at this context, 2..9 the move-to-front cache,
    // 10..31 the spatial neighbours widening outward.  The decompilation
    // interleaved the stores with the loads and named every load.
    seed_candidates(nb_sym);
    if( offer_candidates<0>(nb_sym)>=0 )
      return runlen+1;
    SymList **sel_p = load_selectors();
    while( 1 ) {
      if( (*sel_p)->live ) {
        if( (*sel_p)->code_symbol(row_cur[5]->sym) )
          return runlen+1;
        sel_p = sel_cur;
      }
      sel_cur = ++sel_p;
    }
  }
};
