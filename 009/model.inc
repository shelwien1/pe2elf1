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

// Where a symbol sits among the four ranked neighbours the model keeps: 1..4
// for the four, 0 -- the escape -- for anything else.  `nb_sym[8]`, `[10]`,
// `[6]`, `[5]` is the rank order, and it is the same four in the same order
// everywhere that asks.  Three chains in `code_pixel` ran it, each spelling
// the four literals of its own scale into the arms.
static inline int32_t neighbour_rank(const int32_t* nb_sym, int32_t sym) {
  if( sym==nb_sym[8] )
    return 1;
  if( sym==nb_sym[10] )
    return 2;
  if( sym==nb_sym[6] )
    return 3;
  if( sym==nb_sym[5] )
    return 4;
  return 0;
}

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
  void free_workspace(int8_t do_free);

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
      ctr_node = CtxIdx{}.raw(pos<<7).raw(ctx2);
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
      int32_t just = row_cur[5]->sym;
      uint16_t *cache = sym_cache;
      int32_t c0 = cache[0];
      if( just!=c0 ) {
        int32_t c1 = cache[1];
        if( just==c1 ) {
          cache[1] = c0;
        } else {
          int32_t c2 = cache[2];
          if( just==c2 ) {
            cache[2] = c1;
            sym_cache[1] = sym_cache[0];
          } else {
            int32_t c3 = cache[3];
            if( just==c3 ) {
              cache[3] = c2;
              sym_cache[2] = sym_cache[1];
              sym_cache[1] = sym_cache[0];
            } else {
              int32_t c4 = cache[4];
              if( just==c4 ) {
                cache[4] = c3;
                // The direction is load-bearing: the turns touch the same places.
                for( int32_t n = 0; n<3; ++n ) sym_cache[3-n] = sym_cache[2-n];
              } else {
                int32_t c5 = cache[5];
                if( just==c5 ) {
                  cache[5] = c4;
                  // The direction is load-bearing: the turns touch the same places.
                  for( int32_t n = 0; n<4; ++n ) sym_cache[4-n] = sym_cache[3-n];
                } else {
                  int32_t c6 = cache[6];
                  if( just==c6 ) {
                    cache[6] = c5;
                  } else {
                    cache[7] = c6;
                    sym_cache[6] = sym_cache[5];
                  }
                  // The direction is load-bearing: the turns touch the same places.
                  for( int32_t n = 0; n<5; ++n ) sym_cache[5-n] = sym_cache[4-n];
                }
              }
            }
          }
        }
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
    uint32_t* codes_p;
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
      n_syms = alphabet;
      sym_code = (uint32_t*)codes;
      if( n_syms ) {
        for( j = 0; j<n_syms; ++j ) {
          sym_code[j] = j;
          n_syms = alphabet;
        }
      }
      if( (int32_t)depth>8 ) {
        if( 4*nbytes ) {
          uint32_t k = 0;
          do
            lists[k++].init(256, 1);
          while( k<(4*nbytes) );
          n_syms = alphabet;
        }
        if( n_syms ) {
          codes_p = sym_code;
          carry = 0;
          uint32_t s = 0;
          do {
            codes_p[s] = 0;
            if( nbytes ) {
              uint32_t b = 0;
              do {
                uint32_t piece = lists[4*b+carry].decode_symbol_list();
                carry = piece>>6;
                sym_code[s] += piece<<((8*b)&31);
                ++b;
              } while( b<nbytes );
            }
            codes_p = sym_code;
            carry = (uint8_t)codes_p[s++]>>7;
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
  uint8_t* write_row(uint8_t* out_at, uint32_t nbytes, uint32_t row_w) {
    if( row_w<=0 )
      return out_at;
    if( nbytes==4 ) {
      uint32_t *out32 = (uint32_t*)out_at;
      int32_t y2 = 0;
      do
        *out32++ = sym_code[row_cur[0][y2+++8].sym];
      while( (uint32_t)y2<width );
      return (uint8_t*)out32;
    }
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
    if( nbytes==2 ) {
      uint16_t *out16 = (uint16_t*)out_at;
      int32_t x3 = 0;
      do
        *out16++ = sym_code[row_cur[0][x3+++8].sym];
      while( (uint32_t)x3<width );
      return (uint8_t*)out16;
    }
    if( depth==8 ) {
      uint8_t *out8 = out_at;
      int32_t x5 = 0;
      do
        *out8++ = (uint8_t)sym_code[row_cur[0][x5+++8].sym];
      while( (uint32_t)x5<width );
      return out8;
    }
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

  void unmodel_plane_slow(uint8_t* dst) {
    uint8_t* row[19];
    uint32_t row_w;
    uint8_t *out_at, *expand_buf;
    int32_t g0, done, lo1;
    int32_t lo, w2, w4, w2n, lvl_n, bucket, x2, n_pix, q2, n_pix2;
    PixRec* lists, * t;
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
      // The same six folds `model_plane` hoists, under the same names and in
      // the same order.  This half spelled them `y0`, `f_b1`, `f_b4`, `f_b0`,
      // `f_b5`, `f_b3` -- bit numbers, which say where a bit is and not what
      // it does, and left in the decompiler's declaration order, which is not
      // the order the loop below applies them in.  `tables.inc` names the bits.
      int32_t f_w3_double = flags&ctx_w3_double;
      int32_t f_w4_to_w2 = flags&ctx_w4_to_w2;
      int32_t f_w3_to_w2 = flags&ctx_w3_to_w2;
      lo = 0;
      int32_t f_w4_to_w1 = flags&ctx_w4_to_w1;
      int32_t f_w3_to_w1 = flags&ctx_w3_to_w1;
      int32_t f_w2_to_w1 = flags&ctx_w2_to_w1;
      do {
        lo1 = lo;
        int32_t k = 0;
        do {
          ctx_bucket[g+15*lo+75*k] = g1;
          int32_t m5 = alphabet;
          FreqRec *rec = &grid[g1];
          for( int32_t n = 0; n<4; ++n ) rec->w[1+n] = 2;
          if( f_w3_double ) {
            w2 = (uint16_t)(rec->w[4]+rec->w[3]);
            rec->w[3] = w2;
            w4 = 0;
            rec->w[4] = 0;
          } else {
            w2 = rec->w[3];
            w4 = rec->w[4];
          }
          if( f_w4_to_w2 ) {
            w2n = (uint16_t)(w4+rec->w[2]);
            rec->w[2] = w2n;
            w4 = 0;
            rec->w[4] = 0;
          } else {
            w2n = rec->w[2];
          }
          if( f_w3_to_w2 ) {
            w2n = (uint16_t)(w2+w2n);
            rec->w[2] = w2n;
            w2 = 0;
            rec->w[3] = 0;
          }
          if( f_w4_to_w1 ) {
            rec->w[1] += w4;
            w4 = 0;
            rec->w[4] = 0;
          }
          if( f_w3_to_w1 ) {
            rec->w[1] += w2;
            w2 = 0;
            rec->w[3] = 0;
          }
          if( f_w2_to_w1 ) {
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
      bucket = 0;
      while( 1 ) {
        start_row();
        lists = row_cur[6];
        t = row_cur[7];
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
        if( row_w<=0 )
          break;
        x2 = 0;
        do {
          int32_t y = decode_pixel(x2);
          init_tables();
          row_w = width;
          x2 += y;
        } while( (uint32_t)x2<width );
        out_at = write_row(out_at, nbytes, row_w);
  next_list:
        bucket = bucket+1;
        // `height` is `int32_t` and this side is already `uint32_t`, so C
        // converts the right operand too.  Written where it happens.
        if( (uint32_t)bucket>=(uint32_t)height )
          goto list_done;
      }
      out_at = write_row(out_at, nbytes, row_w);
      goto next_list;
    }
  list_done:
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

  int32_t decode_pixel(int32_t x) {
    // The neighbour list `pixel_context` indexes by `sym_pos`.  BMF.exe fills
    // it one slot at a time, so Hex-Rays gave each slot a member, and an
    // earlier round recovered the array beside them.  The frame struct is what
    // was left over: this array, four bytes of gap and thirty-two of trailing
    // pad, no other member, and nothing reading either.  `pixel_context`
    // indexes 0..31 and no further, so the pad held nothing back.
    alignas(16) int32_t nb_sym[32];
    int32_t idx_t;
    FreqRec* freq;
    uint16_t* id3p;
    FreqRec* freq_tbl;
    uint32_t done, lvl_a;
    PixRec *next;
    int32_t nb, key, cap, nb2, all_up, sig1, sig2, s1a, m_up0, run, bucket, bit, hit_a, idx_s;
    PixRec *cur6 = (PixRec*)row_cur[6];
    int32_t up_sym = cur6->sym;
    PixRec *row = row_cur[5];
    int32_t left_sym = row[-1].sym;
    int32_t up_p1_sym = cur6[1].sym;
    int32_t up_m1_sym = cur6[-1].sym;
    nb_sym[0] = up_sym;
    mode_symbol[1] = up_sym;
    nb_sym[1] = left_sym;
    mode_symbol[2] = left_sym;
    nb_sym[3] = up_p1_sym;
    mode_symbol[3] = up_p1_sym;
    int32_t m_lo = cur6->match[1]+4*(up_p1_sym==up_m1_sym);
    int32_t m_up = cur6[1].match[1];
    nb_sym[2] = up_m1_sym;
    mode_symbol[4] = up_m1_sym;
    // Six match flags of the four named neighbours, as six bits.
    nb = CtxIdx{}.bit<5>(row[-1].match[2]).bit<4>(row[-1].match[4])
                 .bit<3>(m_up).bit<1>(row[-1].match[0]).raw(m_lo);
    if( up_sym==left_sym ) {
      if( nb_sym[3]==nb_sym[0] ) {
        int16_t revtab = sym_rev[up_sym];
        if( up_sym==nb_sym[2] )
          key = (uint16_t)(revtab-row[-2].sym);
        else
          key = (uint16_t)(revtab-nb_sym[2]);
      } else {
        key = (uint16_t)(sym_rev[up_sym]-nb_sym[3]);
      }
    } else {
      key = (uint16_t)(sym_rev[up_sym]-nb_sym[1]);
    }
    sym_cache = &sym_ctr[8*key];
    int32_t state = ctx_state[nb];
    ctx_state_seen = state;
    SymPair *pair = &group_ctr[state][key];
    pix_cur = pair;
    int32_t pair_last = pair->last;
    if( pair_last==nb_sym[0] ) {
      cap = 15;
    } else if( pair_last==nb_sym[1] ) {
      cap = 30;
    } else if( pair_last==nb_sym[3] ) {
      cap = 45;
    } else {
      cap = 60;
      if( !(pair_last==nb_sym[2]) )
        cap = 0;
    }
    uint16_t pair_prev = pair->prev;
    if( pair_prev==nb_sym[0] ) {
      cap += 75;
    } else if( pair_prev==nb_sym[1] ) {
      cap += 150;
    } else if( pair_prev==nb_sym[3] ) {
      cap += 225;
    } else if( pair_prev==nb_sym[2] ) {
      cap += 300;
    }
    PixRec *r8 = row_cur[8];
    int32_t bucket_at = ctx_bucket[state+cap];
    PixRec *r7 = row_cur[7];
    bucket_idx = bucket_at;
    int32_t up_m0 = cur6->match[0];
    nb_sym[2] = up_m0;
    int32_t m_w1 = row[-1].match[1];
    PixRec *up9 = row_cur[9];
    nb2 = CtxIdx{}.bit<3>(row[-2].match[2]).bit<2>(row[-2].match[5])
                  .bit<1>(row[-2].match[4]).bit<0>(m_w1);
    // Two run bits at 9 and 8, the bucket from 10 up, and `nb2`'s four
    // bits underneath -- the comment below already said so and the
    // expression did not.  The bucket stays a `raw`: it is a table
    // lookup with no declared width to read one off.
    all_up = CtxIdx{}.raw(bucket_at<<10)
                     .bit<9>(up9->match[0]&r8->match[0]&nb_sym[2]&r7->match[0])
                     .bit<8>(up9->match[1]&r8->match[1]&r7->match[1]&cur6->match[1])
                     .raw(nb2);
    PixRec *cur6b = cur6;
    // The four gradient accumulators as four "is it flat" bits, over the
    // four-bit `nb2` below them; `all_up` carries the two run bits at 8 and 9
    // and the bucket from 10 up.
    sig1 = CtxIdx{}.bit<7>(grad[3]==0).bit<6>(grad[2]==0)
                   .bit<5>(grad[1]==0).bit<4>(grad[0]==0)
                   .raw(all_up);
    int32_t id1 = ctx_id1[sig1];
    if( id1==0xFFFF ) {
      ctx_id1[sig1] = ctx_id1_used;
      cur6b = row_cur[6];
      row = row_cur[5];
      ++ctx_id1_used;
      id1 = ctx_id1[sig1];
      nb_sym[2] = cur6b->match[0];
    }
    sig2 = CtxIdx{}.bit<2>(cur6b[1].match[3]).bit<1>(nb_sym[2])
                   .bit<0>(row[-1].match[5]).raw(8*id1);
    int32_t id2 = ctx_id2[sig2];
    if( id2==0xFFFF ) {
      ctx_id2[sig2] = ctx_id2_used++;
      id2 = ctx_id2[sig2];
    }
    if( (int32_t)alphabet<32 ) {
      int32_t id3_used = ctx_id3_used;
      // The encoder's half of this context is at `sig3` below, written the same
      // way: a four-bit field of the symbol under a lazily allocated id.  It
      // reuses `sym1`, which held the left neighbour's symbol until here.
      nb_sym[1] = CtxIdx{}.bits<0, 4>(nb_sym[1]).raw(16*id2);
      id3p = &ctx_id3[nb_sym[1]];
      id2 = *id3p;
      if( id2==0xFFFF ) {
        s1a = nb_sym[1];
        if( id3_used>53248 )
          s1a = nb_sym[1]|0xF;
        id3p = &ctx_id3[s1a];
        id2 = *id3p;
      }
      if( id2>=id3_used ) {
        *id3p = id3_used;
        ++ctx_id3_used;
        id2 = *id3p;
      }
    }
    if( row_cur[5][-1].match[1]&row_cur[5][-1].match[0] ) {
      PixRec *up1 = row_cur[6];
      PixRec *up2 = row_cur[7];
      if( (uint8_t)(up2[2].match[1]&up2[1].match[1]&up2[0].match[1]&up1[3].match[1]&up1[2].match[1]&up1[1].match[1]&up1[0].match[1]&up1[0].match[0]&(int8_t)up1[-1].match[1])&up2[3].match[1] ) {
        m_up0 = up2[0].match[0];
        int32_t idx1 = 1;
        if( width-x<=1 ) {
          run = 1;
        } else {
          nb_sym[1] = width-x;
          while( 1 ) {
            run = idx1;
            if( !(up1[idx1+2].match[1]&up1[idx1+2].match[0]) )
              break;
            m_up0 = (uint8_t)(up2[run].match[0]&m_up0);
            if( ++idx1>=nb_sym[1] ) {
              run = idx1;
              goto run_copied;
            }
          }
        }
  run_copied:
        bucket = *(run_bucket+idx1);
        // Four fields and a plus-one: the bucket from bit 3 up, then
        // three bits of neighbour agreement.  The `+1` is an offset
        // into `esc_ctr` and not a field, which is why it is a `raw`.
        int32_t esc_at = CtxIdx{}.raw(8*bucket)
                                 .bit<2>(up2[run+3].match[1]&up2[run+2].match[1])
                                 .bit<1>(m_up0)
                                 .bit<0>(alpha_map[nb_sym[0]])
                                 .raw(1);
        bit = esc_ctr[esc_at].decode_context_bit(esc_ctr);
        int32_t msym1c = mode_symbol[1];
        hit = bit;
        *(((uint8_t*)alpha_map)+msym1c) = bit;
        hit_a = hit;
        if( hit_a ) {
          idx_s = idx1;
        } else {
          idx_s = 0;
          if( idx1==1 )
            goto run_done;
          nb_sym[3] = idx1;
          nb_sym[0] = bucket;
          idx_s = 0;
          int32_t mask = 1<<(bucket&31);
          int32_t seen = 0;
          do {
            if( (mask|seen)<nb_sym[3] ) {
              BitCtr *ctr = &run_ctr[16*((seen==0)+(bucket==nb_sym[0]))+bucket];
              int32_t run0 = ctr->n[0];
              int32_t bin_tot = run0+ctr->n[1];
              nb_sym[1] = rc.decode_bit(run0, ctr->n[1]);
              if( ctr->limit<(uint32_t)bin_tot )
                ctr->scale_rare();
              int32_t s1b = nb_sym[1];
              ctr->n[s1b] += 8;
              if( s1b )
                idx_s |= mask;
              seen |= idx_s&mask;
            }
            --bucket;
            mask >>= 1;
          } while( mask );
        }
        if( idx_s )
          row_cur[5][idx_s-1].sym = row_cur[5][-1].sym;
        hit_a = hit;
  run_done:
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
          nb_sym[1] = mode_symbol[1];
          pixp->prev = pixp->last;
          row_cur[5]->sym = (uint16_t)mode_symbol[1];
          pix_cur->last = (uint16_t)mode_symbol[1];
          int32_t flags_word = row_cur[5]->match2345;
          nb_sym[3] = row_cur[5]->head4;
          next = row_cur[5]+1;
          row_cur[5] = next;
          if( idx_s-hit_a!=1 ) {
            nb_sym[2] = (idx_s-hit_a-1)/2;
            if( nb_sym[2] ) {
              int32_t s3c = nb_sym[3];
              nb_sym[0] = hit_a;
              int16_t s1c = nb_sym[1];
              uint32_t si = 0;
              idx_t = idx_s;
              uint32_t s2a = nb_sym[2];
              do {
                pix_cur->prev = s1c;
                row_cur[5]->head4 = s3c;
                row_cur[5]->match2345 = flags_word;
                SymPair *pixq = pix_cur;
                ++row_cur[5];
                pixq->prev = s1c;
                row_cur[5]->head4 = s3c;
                row_cur[5]->match2345 = flags_word;
                next = row_cur[5]+1;
                row_cur[5] = next;
                ++si;
              } while( si<s2a );
              hit_a = nb_sym[0];
              idx_s = idx_t;
              done = 2*si+1;
            } else {
              done = 1;
            }
            if( (uint32_t)(idx_s-hit_a-1)>(done-1) ) {
              int32_t s3d = nb_sym[3];
              pix_cur->prev = nb_sym[1];
              row_cur[5]->head4 = s3d;
              row_cur[5]->match2345 = flags_word;
              next = row_cur[5]+1;
              row_cur[5] = next;
            }
          }
          idx_t = idx_s;
          PixRec *cur6c = (PixRec*)row_cur[6];
          PixRec *up3 = (PixRec*)row_cur[7];
          grad[0] = match_seed_split(cur6c);
          grad[1] = match_seed(up3-3, 8);
          grad[2] = match_seed(next-4, 2, 1);
          int32_t s1d = nb_sym[1];
          grad[3] = match_seed(next-7, 4);
          next[-1].match[4] = s1d==cur6c[1].sym;
          idx_s = idx_t;
          row_cur[5][-1].match[5] = s1d==row_cur[6][2].sym;
          hit_a = hit;
        }
        if( hit_a )
          return idx_s;
        goto pixel_done;
      }
    }
    freq = &grid[bucket_idx];
    freq_tbl = &grid[id2+188];
    // The lambda is in a block of its own because the `goto pixel_done`
    // eighteen lines up jumps over it, and a jump may cross a scope but not an
    // initialisation inside the label's own.
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
    idx_s = 0;
  pixel_done:
    int8_t gen = exclusion_gen;
    int32_t msym3 = mode_symbol[3];
    int32_t msym1 = mode_symbol[1];
    idx_t = idx_s;
    int32_t msym2 = mode_symbol[2];
    exclusion_mask[mode_symbol[4]] = exclusion_gen;
    SymPair *pixr = pix_cur;
    exclusion_mask[msym3] = gen;
    exclusion_mask[msym2] = gen;
    exclusion_mask[msym1] = gen;
    exclusion_mask[no_symbol] = gen;
    sel[0] = nullptr;
    // The 32 candidate symbols `pixel_context` walks, in slot order.  Slots 0
    // and 1 are the two most recent symbols at this exact context, 2..9 the
    // move-to-front cache, and 10..31 the spatial neighbours widening outward.
    //
    nb_sym[0] = pixr->last;
    nb_sym[1] = pixr->prev;
    for( int32_t c = 0; c<8; ++c )
      nb_sym[2+c] = sym_cache[c];
    load_neighbours(nb_sym);
    sym_pos = 0;
    do {
      int32_t psym = pixel_context((uint32_t*)nb_sym);
      if( psym>=0 ) {
        int32_t bit2 = bit_node[ctr_node].decode_context_bit(&bit_root[ctr_fallback]);
        row_cur[5]->sym = psym;
        if( bit2 )
          return idx_t+1;
        exclusion_mask[psym] = exclusion_gen;
      }
      ++sym_pos;
    } while( sym_pos<32 );
    SymList **sel_p = load_selectors();
    while( 1 ) {
      if( sel_p>&escape_list )
        bmf_fatal(bmf_read_error);
      if( (*sel_p)->live ) {
        int32_t lsym = (*sel_p)->decode_symbol_list();
        row_cur[5]->sym = lsym;
        if( lsym>=0 )
          return idx_t+1;
        sel_p = sel_cur;
      }
      sel_cur = ++sel_p;
    }
  }

  int32_t code_pixel(int32_t x) {
    // The neighbour list `pixel_context` indexes by `sym_pos`.  BMF.exe fills
    // it one slot at a time, so Hex-Rays gave each slot a member, and an
    // earlier round recovered the array beside them.  The frame struct is what
    // was left over: this array, four bytes of gap and thirty-two of trailing
    // pad, no other member, and nothing reading either.  `pixel_context`
    // indexes 0..31 and no further, so the pad held nothing back.
    alignas(16) int32_t nb_sym[32];
    int32_t runlen_s;
    uint32_t done;
    uint16_t* id3p;
    int32_t nb, key, cap, nb2, sig1, sig2, m_up0, one, run, amap, runlen, run_left, bit5, lvl_a;
    PixRec *cur5p1;
    PixRec* cur6c, * up2;
    PixRec* cur6 = (PixRec*)row_cur[6];
    int32_t up_sym = cur6->sym;
    PixRec *cur5 = row_cur[5];
    int32_t left_sym = cur5[-1].sym;
    int32_t up_next_sym = cur6[1].sym;
    int32_t upleft_sym = cur6[-1].sym;
    nb_sym[8] = up_sym;
    mode_symbol[1] = up_sym;
    nb_sym[10] = left_sym;
    mode_symbol[2] = left_sym;
    nb_sym[6] = up_next_sym;
    mode_symbol[3] = up_next_sym;
    nb_sym[5] = upleft_sym;
    mode_symbol[4] = upleft_sym;
    // Six match flags of the four named neighbours, as six bits.
    nb = CtxIdx{}.bit<5>(cur5[-1].match[2]).bit<4>(cur5[-1].match[4])
                 .bit<3>(cur6[1].match[1]).bit<2>(up_next_sym==upleft_sym)
                 .bit<1>(cur5[-1].match[0]).bit<0>(cur6->match[1]);
    bool up_eq_west = up_sym==left_sym;
    if( up_eq_west ) {
      if( nb_sym[8]==nb_sym[6] ) {
        uint16_t rev = sym_rev[nb_sym[8]];
        if( nb_sym[8]==nb_sym[5] )
          key = (uint16_t)(rev-cur5[-2].sym);
        else
          key = (uint16_t)(rev-nb_sym[5]);
      } else {
        key = (uint16_t)(sym_rev[nb_sym[8]]-nb_sym[6]);
      }
    } else {
      key = (uint16_t)(sym_rev[nb_sym[8]]-nb_sym[10]);
    }
    sym_cache = &sym_ctr[8*key];
    int32_t state = ctx_state[nb];
    ctx_state_seen = state;
    SymPair* pair = &group_ctr[state][key];
    pix_cur = pair;
    // The pair's two symbols ranked against the same four neighbours, at
    // fifteen and seventy-five to the rank.  The first chain reached rank 4
    // through `up_eq_west` -- a `bool` it had already used and finished with
    // eighteen lines earlier -- and then tested it back.
    cap = 15*neighbour_rank(nb_sym, pair->last)
        +75*neighbour_rank(nb_sym, pair->prev);
    PixRec *r1 = row_cur[8];
    int32_t bucket_at = ctx_bucket[state+cap];
    PixRec *r2 = row_cur[7];
    bucket_idx = bucket_at;
    int32_t up_match0 = cur6->match[0];
    uint8_t match1 = cur6->match[1];
    PixRec *cur2 = cur5;
    nb_sym[0] = up_match0;
    PixRec* up9 = row_cur[9];
    int32_t m_w1 = cur5[-1].match[1];
    nb2 = CtxIdx{}.bit<3>(cur5[-2].match[2]).bit<2>(cur5[-2].match[5])
                  .bit<1>(cur5[-2].match[4]).bit<0>(m_w1);
    // Four "is this gradient flat" bits, two "did four rows above all match"
    // bits, and the bucket, over `nb2`'s four.  Written as one multiplication
    // chain the widths were not countable; here the index is plainly 10 bits
    // plus however many `bucket_at` has.
    sig1 = CtxIdx{}.bit<7>(grad[3]==0).bit<6>(grad[2]==0)
                   .bit<5>(grad[1]==0).bit<4>(grad[0]==0)
                   .bit<9>((uint8_t)(up9->match[0]&r1->match[0]&nb_sym[0]&r2->match[0]))
                   .bit<8>((uint8_t)(up9->match[1]&r1->match[1]&r2->match[1]&match1))
                   .raw(bucket_at<<10).raw(nb2);
    int32_t id1 = ctx_id1[sig1];
    PixRec *cur6b = cur6;
    if( id1==0xFFFF ) {
      ctx_id1[sig1] = ctx_id1_used;
      cur6b = row_cur[6];
      cur5 = row_cur[5];
      ++ctx_id1_used;
      id1 = ctx_id1[sig1];
      nb_sym[0] = cur6b->match[0];
    }
    sig2 = CtxIdx{}.bit<2>(cur6b[1].match[3]).bit<1>(nb_sym[0])
                   .bit<0>(cur5[-1].match[5]).raw(8*id1);
    int32_t id2 = ctx_id2[sig2];
    if( id2==0xFFFF ) {
      ctx_id2[sig2] = (ctx_id2_used)++;
      id2 = ctx_id2[sig2];
    }
    if( (int32_t)alphabet<32 ) {
      int32_t id3_used = ctx_id3_used;
      int32_t sig3 = CtxIdx{}.bits<0, 4>(nb_sym[10]).raw(16*id2);
      id3p = &ctx_id3[sig3];
      id2 = *id3p;
      if( id2==0xFFFF ) {
        if( id3_used>53248 )
          sig3 |= 0xFu;
        id3p = &ctx_id3[sig3];
        id2 = *id3p;
      }
      if( id2>=id3_used ) {
        *id3p = id3_used;
        ++ctx_id3_used;
        id2 = *id3p;
      }
    }
    PixRec* row = row_cur[5];
    int32_t m_w1b = row[-1].match[1];
    int32_t m_w0 = row[-1].match[0];
    PixRec *rowp = row;
    if( (m_w1b&m_w0)!=0&&(cur6c = row_cur[6], up2 = row_cur[7], ((uint8_t)(up2[2].match[1]&up2[1].match[1]&up2[0].match[1]&cur6c[3].match[1]&cur6c[2].match[1]&cur6c[1].match[1]&cur6c[0].match[1]&cur6c[0].match[0]&cur6c[-1].match[1])&up2[3].match[1])!=0) ) {
      m_up0 = up2[0].match[0];
      int32_t to_edge = width-x;
      nb_sym[4] = 1;
      if( to_edge<=1 ) {
        run = 1;
      } else {
        nb_sym[0] = to_edge;
        one = 1;
        while( 1 ) {
          run = one;
          if( !(cur6c[one+2].match[1]&cur6c[one+2].match[0]) )
            break;
          m_up0 = (uint8_t)(up2[run].match[0]&m_up0);
          if( ++one>=nb_sym[0] ) {
            nb_sym[4] = one;
            run = one;
            goto run_copied;
          }
        }
        nb_sym[4] = one;
      }
  run_copied:
      int32_t run_pair = (uint8_t)(up2[run+3].match[1]&up2[run+2].match[1]);
      nb_sym[5] = run_bucket[nb_sym[4]];
      amap = CtxIdx{}.bit<2>(run_pair).bit<1>(m_up0)
                     .raw(8*nb_sym[5]).raw(alpha_map[nb_sym[8]]);
      runlen = 0;
      if( rowp->sym==nb_sym[8] ) {
        do
          ++runlen;
        while( runlen<nb_sym[4]&&rowp[runlen].sym==nb_sym[8] );
      }
      int32_t run_hit = runlen==nb_sym[4];
      nb_sym[6] = run_hit;
      if( runlen>run_hit ) {
        row_cur[6] = &cur6c[runlen-run_hit];
        row_cur[7] = &up2[runlen-run_hit];
        PixRec *row_cur9 = row_cur[9];
        row_cur[8] = row_cur[8]+runlen-run_hit;
        row_cur[9] = row_cur9+runlen-run_hit;
        rowp->match2345 = 0x01010101;
        row_cur[5]->head4 = 0x01010101;
        pix_cur->prev = pix_cur->last;
        uint16_t wp = nb_sym[8];
        row_cur[5]->sym = wp;
        pix_cur->last = wp;
        PixRec *r3 = row_cur[5];
        uint32_t rec_word = *(uint32_t*)r3;
        nb_sym[1] = r3->match2345;
        cur2 = r3+1;
        row_cur[5] = r3+1;
        if( runlen-run_hit!=1 ) {
          run_left = runlen-run_hit-1;
          uint32_t half = run_left/2;
          if( run_left/2 ) {
            nb_sym[0] = run_left;
            int32_t s1 = nb_sym[1];
            runlen_s = runlen;
            uint32_t k = 0;
            nb_sym[3] = amap;
            int16_t s8 = nb_sym[8];
            do {
              pix_cur->prev = s8;
              row_cur[5]->head4 = rec_word;
              row_cur[5]->match2345 = s1;
              SymPair *pixp = pix_cur;
              ++row_cur[5];
              pixp->prev = s8;
              row_cur[5]->head4 = rec_word;
              row_cur[5]->match2345 = s1;
              cur5p1 = row_cur[5]+1;
              row_cur[5] = cur5p1;
              ++k;
            } while( k<half );
            run_left = nb_sym[0];
            amap = nb_sym[3];
            cur2 = cur5p1;
            runlen = runlen_s;
            done = 2*k+1;
          } else {
            done = 1;
          }
          if( (uint32_t)run_left>(done-1) ) {
            pix_cur->prev = nb_sym[8];
            row_cur[5]->head4 = rec_word;
            row_cur[5]->match2345 = nb_sym[1];
            PixRec *cur5p1b = row_cur[5]+1;
            row_cur[5] = cur5p1b;
            cur2 = cur5p1b;
          }
        }
        PixRec *r4 = (PixRec*)row_cur[6];
        nb_sym[3] = amap;
        runlen_s = runlen;
        PixRec *up3 = (PixRec*)row_cur[7];
        grad[0] = match_seed_split(r4);
        grad[1] = match_seed(up3-3, 8);
        grad[2] = match_seed(cur2-4, 2, 1);
        int32_t s8b = nb_sym[8];
        grad[3] = match_seed(cur2-7, 4);
        cur2[-1].match[4] = s8b==r4[1].sym;
        amap = nb_sym[3];
        row_cur[5][-1].match[5] = s8b==row_cur[6][2].sym;
        runlen = runlen_s;
      }
      (&esc_ctr[(amap+1)])->encode_context_bit(esc_ctr, nb_sym[6]);
      int32_t s6 = nb_sym[6];
      int32_t msym1 = mode_symbol[1];
      uint8_t *amap_at = (uint8_t*)alpha_map;
      hit = nb_sym[6];
      *(amap_at+msym1) = s6;
      if( !s6&&nb_sym[4]!=1 ) {
        runlen_s = runlen;
        nb_sym[0] = nb_sym[5];
        int32_t s4 = nb_sym[4];
        bit5 = 1<<(nb_sym[5]&31);
        int32_t first = 0;
        int32_t s5 = nb_sym[5];
        do {
          if( s4>(bit5|first) ) {
            nb_sym[3] = first;
            BitCtr *ctr = &run_ctr[16*((first==0)+(s5==nb_sym[0]))+s5];
            nb_sym[1] = runlen_s&bit5;
            uint32_t bin_tot = ctr->n[0]+ctr->n[1];
            rc.encode_bit(ctr->n[0], ctr->n[1], (runlen_s&bit5)!=0);
            if( ctr->limit<bin_tot )
              ctr->scale_rare();
            int32_t s1b = nb_sym[1];
            nb_sym[4] = bit5;
            ctr->n[nb_sym[1]!=0] += 8;
            first |= s1b;
            bit5 = nb_sym[4];
          }
          --s5;
          bit5 >>= 1;
        } while( bit5 );
        runlen = runlen_s;
      }
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
        const int32_t lvl = neighbour_rank(nb_sym, rowp->sym);
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
    int8_t mode = exclusion_gen;
    int32_t msym3 = mode_symbol[3];
    int32_t excl_sym_a = mode_symbol[2];
    int32_t excl_sym_b = mode_symbol[1];
    exclusion_mask[mode_symbol[4]] = exclusion_gen;
    exclusion_mask[msym3] = mode;
    SymPair *wq = pix_cur;
    exclusion_mask[excl_sym_a] = mode;
    exclusion_mask[excl_sym_b] = mode;
    uint16_t *cache = sym_cache;
    exclusion_mask[no_symbol] = mode;
    runlen_s = runlen;
    sel[0] = nullptr;
    int32_t w6d = wq->last;
    int32_t wq1 = wq->prev;
    // The same 32 candidates as `decode_pixel`'s, in slot order: 0 and 1 the
    // two most recent symbols at this context, 2..9 the move-to-front cache,
    // 10..31 the spatial neighbours widening outward.  The decompilation
    // interleaved the stores with the loads and named every load.
    nb_sym[0] = w6d;
    nb_sym[1] = wq1;
    for( int32_t c = 0; c<8; ++c )
      nb_sym[2+c] = cache[c];
    load_neighbours(nb_sym);
    sym_pos = 0;
    do {
      int32_t esym = pixel_context((uint32_t*)nb_sym);
      if( esym>=0 ) {
        int32_t up_hit = esym==row_cur[5]->sym;
        (&bit_node[(int32_t)ctr_node])->encode_context_bit(&bit_root[(int32_t)ctr_fallback], up_hit);
        if( up_hit )
          return runlen_s+1;
        exclusion_mask[esym] = exclusion_gen;
      }
      ++sym_pos;
    } while( sym_pos<32 );
    SymList **sel_p = load_selectors();
    while( 1 ) {
      if( (*sel_p)->live ) {
        if( (*sel_p)->code_symbol(row_cur[5]->sym) )
          return runlen_s+1;
        sel_p = sel_cur;
      }
      sel_cur = ++sel_p;
    }
  }
};
