// model.inc -- ModelBlock: the general match/symbol model used for palette and low-depth planes
// Included from bmf.cpp; not a standalone translation unit.

struct PixRec {
  union {
    struct {
      uint16_t sym;
      uint8_t match[6];
    };
    struct {
      uint32_t head4;
      uint32_t match2345;
    };
  };
};

int32_t match_seed(const PixRec* first, int32_t n, int32_t lane = 0) {
  int32_t hits = 0;
  for( int32_t i = 0; i<n; ++i )
    hits += first[i].match[lane];
  return hits-n;
}

int32_t match_seed_split(const PixRec* at) {
  return at[-3].match[0]+at[-2].match[0]+at[2].match[0]+at[3].match[0]+at[4].match[0]-5;
}

struct Neighbours {
  int32_t up, left, up_next, upleft;
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


struct GroupFolds {
  int32_t w3_double, w4_to_w2, w3_to_w2, w4_to_w1, w3_to_w1, w2_to_w1;
  void Init(int32_t flags) {
    w3_double = flags&ctx_w3_double;
    w4_to_w2 = flags&ctx_w4_to_w2;
    w3_to_w2 = flags&ctx_w3_to_w2;
    w4_to_w1 = flags&ctx_w4_to_w1;
    w3_to_w1 = flags&ctx_w3_to_w1;
    w2_to_w1 = flags&ctx_w2_to_w1;
  }
};

void free_sym_lists(SymList*) {
}

struct ReduceNode {
  uint32_t val;
  uint16_t kid[2];
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
  static const int32_t kRowMargin = MB_row_margin;
  static const int32_t kRowTail = MB_row_tail;
  uint32_t bucket_idx;
  uint32_t sym_pos;
  uint32_t ctr_fallback;
  uint32_t ctr_node;
  PixRec* row_cur[10];
  static const int32_t kFreqTableOffset = MB_freq_table_offset;
  static const int32_t kFreqGridCount = MB_freq_grid_count;
  FreqRec grid[kFreqGridCount];
  uint8_t _pad1051664[16];
  BitCtr bit_root[MB_CandFallback_Volume];
  BitCtr bit_node[MB_CandNode_Volume];
  // One map per cell kind: the context cell and the bucket cell it falls back
  // to have different distributions and different amounts of evidence.
  Apm level_apm[2][4];
  // The candidate scan's map, indexed by how deep into the scan we are.
  Apm cand_apm;
  BitCtr esc_ctr[257];
  BitCtr run_ctr[MB_RunCtr_Volume];
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
  uint8_t ctx_bucket[MB_Bucket_Volume];
  uint8_t _pad25[1];
  // One declaration, in IDX/bmf-MB.idx, decides both the radices and the
  // strides -- the generator derives the strides from the radices, so a
  // mixed-radix index whose strides do not match its radices is no longer
  // something the sources can express.  ctx_bucket is sized by the same
  // declaration's Volume, so moving a radix moves the table with it.
  static uint32_t bucket_slot(int32_t group, int32_t last, int32_t prev) {
    return idx_bound(MakeBucket((uint32_t)prev, (uint32_t)last, (uint32_t)group),
                     MB_Bucket_Volume);
  }

  FreqRec*seat_bucket(int32_t group, int32_t last, int32_t prev, int32_t bucket) {
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
  ModelBlock*layout_workspace(BMFState* state, int32_t img_w, int32_t img_h, int32_t img_depth) {
    cx = state;
    int32_t j;
    uint32_t k, m, s, n;
    cx[0].exclusion_gen = 1;
    width = img_w;
    height = img_h;
    depth = img_depth;
    depth_raw = img_depth;
    escape.ent = nullptr;
    sym_code = nullptr;
    for( j = 0; j<5; ++j ) {
      PixRec* buf = row_store[j];
      row_cur[j] = buf;
      row_cur[j+5] = buf+ModelBlock::kRowMargin+1;
      for( uint32_t r = 0; r<width+(uint32_t)(ModelBlock::kRowMargin+ModelBlock::kRowTail); ++r ) {
        row_cur[j][r].sym = 0;
        for( int32_t i = 0; i<6; ++i )
          row_cur[j][r].match[5-i] = 1;
      }
    }

    uint8_t* runs = run_bucket_store;
    run_bucket = runs;
    runs[0] = 0;
    {
      uint8_t bucket = 0;
      for( uint32_t x = 0; x<width; ++x ) {
        bucket += x==2u<<(bucket&31);
        run_bucket[x+1] = bucket;
      }
    }
    memset(sym_rev, 0, sizeof sym_rev);
    for( k = 0; k<0x2000; ++k ) {
      uint16_t rev = sym_rev[k];
      int32_t bits = k;
      for( m = 0; m<0xD; ++m ) {
        rev += rev+(bits&1);
        bits >>= 1;
      }
      sym_rev[k] = rev;
    }
    for( s = 0; s<0x2000; ++s )
      sym_rev[s] *= 8;
    memset(&grid[188], 0, 0x100000);
    ctx_id3_used = 0;
    ctx_id2_used = 0;
    ctx_id1_used = 0;
    for( int32_t k = 0; k<2; ++k )
      for( int32_t j = 0; j<4; ++j )
        level_apm[k][j].Init();
    cand_apm.Init();
    memset(ctx_id1, 255, sizeof ctx_id1);
    memset(ctx_id2, 255, sizeof ctx_id2);
    memset(ctx_id3, 255, sizeof ctx_id3);
    memset(cx[0].exclusion_mask, 0, sizeof cx[0].exclusion_mask);
    sel[0] = nullptr;
    sel[1] = nullptr;
    escape_list = nullptr;
    for( n = 0; n<0x40000; ++n ) {
      sym_ctr[2*n] = no_symbol;
      sym_ctr[2*n+1] = no_symbol;
    }
    for( uint32_t k8 = 0; k8<8; ++k8 ) {
      bit_root[2*k8].init_root();
      bit_root[2*k8+1].init_root();
    }
    memset(bit_node, 0, sizeof bit_node);
    sym_word = (uint16_t*)bmf_new(2*height*width);
    esc_ctr[0].init_parent();
    memset(&esc_ctr[1], 0, 1536);
    for( uint32_t k24 = 0; k24<0x18; ++k24 ) {
      run_ctr[2*k24].init_parent();
      run_ctr[2*k24+1].init_parent();
    }
    return this;
  }
  void free_workspace() {
    free(sym_word);
    free_sym_lists(sel1_list);
    free_sym_lists(sel0_list);
  }
  void reduce_narrow_alphabet(SymList* lists, uint8_t* src, uint32_t mask) {
    int32_t n_distinct, y, bits, shift, prev, s, s_next;
    uint32_t n_syms3, sym_flag[256];
    memset(sym_flag, 0, sizeof sym_flag);
    bool packed = this[0].depth<8;
    int32_t rows = this[0].height;
    this[0].alphabet = 0;
    if( packed ) {
      n_distinct = 0;
      if( rows ) {
        const uint8_t* p = src-1;
        int32_t at = 0;
        const int32_t bpp = this[0].depth;
        for( y = 0; y<this[0].height; ++y ) {
          bits = 0;
          for( uint32_t x = 0; x<this[0].width; ++x, ++at ) {
            shift = bits-bpp;
            if( shift<0 ) {
              ++p;
              shift = 8-bpp;
            }
            const uint32_t sym = mask&(p[0]>>shift);
            this[0].alphabet += sym_flag[sym]==0;
            sym_flag[sym] = 1;
            bits = shift;
            this[0].sym_word[at] = sym;
          }
        }
        n_distinct = this[0].alphabet;
      }
    } else if( rows&&this[0].width ) {
      const uint8_t* q = src;
      const uint32_t n_pix = this[0].height*this[0].width;
      for( uint32_t idx = 0; idx<n_pix; ++idx ) {
        const int32_t sym2 = *q++;
        this[0].alphabet += sym_flag[sym2]==0;
        sym_flag[sym2] = 1;
        this[0].sym_word[idx] = sym2;
      }
      n_distinct = this[0].alphabet;
    } else {
      n_distinct = 0;
    }
    cx[0].code_alphabet_size<0>(n_distinct, mask+1);
    uint32_t n_syms = this[0].alphabet;
    if( n_syms<=mask ) {
      init_gap_list(cx, lists, mask, n_syms);
      n_syms3 = this[0].alphabet;
      if( n_syms3 ) {
        prev = 0;
        s = 0;
        uint32_t next_id = 0;
        do {
          if( sym_flag[s] ) {
            lists[0].code_symbol(cx, s-prev);
            n_syms3 = this[0].alphabet;
            sym_flag[s] = next_id;
            s_next = s+1;
            prev = s+1;
            ++next_id;
          } else {
            s_next = s+1;
          }
          s = s_next;
        } while( next_id<n_syms3 );
      }
      {
        const uint32_t n_pix = this[0].height*this[0].width;
        for( uint32_t m = 0; m<n_pix; ++m )
          this[0].sym_word[m] = sym_flag[this[0].sym_word[m]];
      }
      free_sym_entries(lists, 16);
    } else {
      free_sym_entries(lists, 16);
    }
  }

  int32_t tree_place(ReduceNode* tree, uint32_t val) {
    int32_t node = 0, side;
    uint16_t* kidp;
    while( 1 ) {
      side = tree[node].val<val;
      kidp = tree[node].kid;
      node = kidp[side];
      if( !node )
        break;
      if( val==tree[node].val ) {
        cx[0].mode_symbol[1] = side;
        return node;
      }
    }
    cx[0].mode_symbol[1] = side;
    node = (uint16_t)alphabet;
    int32_t alpha = alphabet+1;
    kidp[side] = node;
    alphabet = alpha;
    if( alpha>0x2000 )
      return -1;
    tree[node].val = val;
    return node;
  }
  void reduce_alphabet(uint8_t* src) {
    ReduceNode tree[8192];
    SymList lists[16];
    uint8_t* out[19];
    uint32_t alpha_n, done;
    int32_t node, carry;
    uint8_t* rp;
    uint32_t sym_bits = depth;
    uint32_t mask = 0xFFFFFFFF>>(-sym_bits&31);
    uint32_t n_kids = (sym_bits+7)>>3;
    auto load32 = [](const uint8_t* at) {
                    uint32_t v;
                    memcpy(&v, at, sizeof v);
                    return v;
                  };
    clear_sym_lists(lists);
    if( sym_bits<=8 ) {
      reduce_narrow_alphabet(lists, src, mask);
    } else {
      memset(tree, 0, sizeof tree);
      alphabet = 1;
      tree[0].val = mask&load32(src);
      sym_word[0] = 0;
      if( (uint32_t)(height*width)>1 ) {
        uint8_t* p = src;
        node = 0;
        uint32_t written = 1;
        while( 1 ) {
          p += n_kids;
          uint32_t val = mask&load32(p);
          if( val!=tree[node].val ) {
            node = 0;
            if( val!=tree[0].val ) node = tree_place(tree, val);
            if( node<0 ) break;
          }
          sym_word[written++] = node;
          if( written>=(uint32_t)(height*width) ) break;
        }
      }
      alpha_n = alphabet;
      cx[0].code_alphabet_size<0>(alpha_n, no_symbol+1);
      if( alphabet>0x2000 ) {
        out[1] = (uint8_t*)bmf_new(height*n_kids*width);
        int32_t img_w = width;
        int32_t img_h = height;
        uint32_t plane_size;
        if( n_kids ) {
          plane_size = img_h*img_w;
          if( n_kids>>1 ) {
            uint8_t* half = out[1]+img_h*img_w;
            uint32_t pairs = 0;
            for(; pairs<n_kids>>1; ++pairs ) {
              const uint32_t slot_a = 2*pairs;
              const uint32_t off = 2*pairs*plane_size;
              out[slot_a+2] = out[1]+off;
              out[slot_a+3] = &half[off];
            }
            done = 2*pairs+1;
          } else {
            done = 1;
          }
          if( n_kids>(done-1) )
            out[done+1] = out[1]+img_h* -img_w+plane_size*done;
        } else {
          plane_size = img_h*img_w;
        }
        if( plane_size ) {
          rp = src;
          if( n_kids ) {
            for( uint32_t moved = 0; moved<plane_size; ++moved ) {
              for( uint32_t c = 0; c<n_kids; ++c )
                *out[c+2]++ = rp[c];
              rp += n_kids;
            }
          }
        }
        uint16_t* old_words = sym_word;
        height = n_kids*img_h;
        depth = 8;
        free(old_words);
        void* newbuf = bmf_new(2*height*width);
        sym_word = (uint16_t*)newbuf;
        reduce_alphabet(out[1]);
        free(out[1]);
      } else {
        init_byte_lists(cx, lists, n_kids);
        const int32_t alpha_m = alphabet;
        if( alpha_m ) {
          carry = 0;
          for( uint32_t si = 0; si<(uint32_t)alpha_m; ++si )
            lists[0].code_symbol_bytes<0>(cx, n_kids, tree[si].val, carry);
        }
      }
      free_sym_entries(lists, 16);
    }
  }
  int32_t pixel_context(uint32_t* nb) {
    int32_t band;
    int32_t pos = sym_pos;
    uint32_t result = (nb)[pos];
    if( cx[0].exclusion_mask[result]==cx[0].exclusion_gen )
      return -1;
    bool near = false;
    for( band = MB_near_lo; band<MB_near_hi; ++band )
      near |= result==nb[band];
    bool far = false;
    for( band = MB_near_hi; band<MB_far_hi; ++band )
      far |= result==nb[band];
    const bool exact = result==nb[MB_exact_rank];
    // The bare test was on the packed value being zero, which is the same as
    // none of the three hits firing.
    if( !exact&&!near&&!far&&pos>MB_pos_bare )
      return -1;
    SymList* sel0 = sel0_list;
    SymEntry* list_prev = sel0[cx[0].mode_symbol[2]].ent;
    SymEntry* list_sym = sel0[result].ent;
    const int32_t ctx2 = (int32_t)MakeCandCtx(
        exact, near, far,
        sym_in_top((sel0[cx[0].mode_symbol[1]].ent), MB_top_depth_0, result),
        sym_in_top(list_prev, MB_top_depth_1, result),
        sym_in_top(list_sym, MB_top_depth_2, cx[0].mode_symbol[2]),
        sym_in_top((sel0[cx[0].mode_symbol[3]].ent), MB_top_depth_3, result));
    if( pos<=MB_pos_split||(ctx2&0xB)!=0 ) {
      ctr_node = (int32_t)idx_bound(MakeCandNode((uint32_t)pos, (uint32_t)ctx2),
                                   MB_CandNode_Volume);
      // The fallback is the low three factors of the same index.
      ctr_fallback = (int32_t)idx_bound(MakeCandFallback((uint32_t)(pos>MB_pos_deep),
                                                        (uint32_t)ctx2&7u),
                                       MB_CandFallback_Volume);
    } else {
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
          sel0_list[cx[0].mode_symbol[1]].add_weight(row_cur[5][0].sym, 3u);
          sel0_list[row_cur[5][0].sym].add_weight(cx[0].mode_symbol[2], 2u);
          sel1_list[cx[0].mode_symbol[1]].add_weight(row_cur[5][0].sym, 4u);
          sel1_list[row_cur[5][0].sym].add_weight(cx[0].mode_symbol[1], 2u);
        } else {
          sel0_list[cx[0].mode_symbol[2]].add_weight(row_cur[5][0].sym, (sym_pos>3)+2);
        }
      } else {
        sel0_list[cx[0].mode_symbol[1]].add_weight(row_cur[5][0].sym, 3u);
        sel0_list[row_cur[5][0].sym].add_weight(cx[0].mode_symbol[2], 2u);
        sel0_list[row_cur[5][0].sym].add_weight(cx[0].mode_symbol[1], 1u);
        sel1_list[row_cur[5][0].sym].add_weight(cx[0].mode_symbol[1], 2u);
        cur = sel_cur;
        do {
          prev = cur-1;
          sel_cur = prev;
          list = prev[0];
          uint16_t want = row_cur[5][0].sym;
          uint32_t n_live = list[0].live;
          SymEntry* ent = list[0].ent;
          if( n_live==list[0].n ) {
            list[0].live = --n_live;
            recycled = list[0].ent[n_live].cnt;
          } else {
            recycled = 1;
          }
          SymEntry* slot = &ent[n_live];
          list[0].live = n_live+1;
          list[0].tot = recycled+list[0].tot+1;
          slot[0].set(want, 2);
          list[0].since_rescale += 4;
          if( slot!=list[0].ent ) {
            uint16_t sym = slot[0].sym;
            uint8_t cnt = slot[0].cnt;
            slot[0] = slot[-1];
            slot[-1].set(sym, cnt);
          }
          cur = sel_cur;
        } while( cur!=sel );
      }
      if( cx[0].exclusion_gen==-1 ) {
        cx[0].exclusion_gen = 1;
        uint8_t* buf = (uint8_t*)cx[0].exclusion_mask;
        uint32_t blocks = (alphabet+15)>>4;
        do {
          bmf_zero16(buf);
          buf += 16;
          --blocks;
        } while( blocks );
        hit1 = hit;
      } else {
        ++cx[0].exclusion_gen;
        hit1 = hit;
      }
      promoted = hit1&&hit1<=2;
    } else {
      promoted = hit0<=2;
      if( !promoted&&cx[0].mode_symbol[3]!=cx[0].mode_symbol[4] ) {
        sel0_list[cx[0].mode_symbol[2]].add_weight(row_cur[5][0].sym, 1u);
        hit1 = hit;
        promoted = hit1&&hit1<=2;
      }
    }
    if( !promoted ) {
      int32_t just = row_cur[5][0].sym;
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
    pix_cur[0].prev = pix_cur[0].last;
    pix_cur[0].last = row_cur[5][0].sym;
    {
      PixRec*const here = row_cur[5];
      PixRec*const up = row_cur[6];
      here[0].match[0] = here[0].sym==up[0].sym;
      here[0].match[1] = here[0].sym==here[-1].sym;
      here[0].match[2] = here[0].sym==up[1].sym;
      here[0].match[3] = here[0].sym==up[-1].sym;
      here[0].match[4] = here[0].sym==up[2].sym;
      here[0].match[5] = here[0].sym==up[3].sym;
    }
    ++row_cur[5];
    ++row_cur[6];
    ++row_cur[7];
    ++row_cur[8];
    ++row_cur[9];
    PixRec*const row = row_cur[5];
    PixRec*const up1 = row_cur[6];
    PixRec*const up2 = row_cur[7];
    grad[0] += up1[4].match[0]-up1[-4].match[0];
    grad[1] += up2[4].match[0]-up2[-4].match[0];
    grad[2] += row[-1].match[1]-row[-5].match[1];
    int32_t result = row[-1].match[0]-row[-8].match[0];
    grad[3] += result;
    return result;
  }

  SymList**load_selectors() {
    sel[0] = &sel0_list[cx[0].mode_symbol[2]];
    sel[1] = &sel1_list[cx[0].mode_symbol[1]];
    return sel_cur;
  }

  void start_row() {
    row_cur[5][0].match[1] = row_cur[5][-1].sym==0;
    row_cur[5][0].match[3] = row_cur[6][-1].sym==0;
    ring_advance(row_cur, row_cur+5, kRowMargin);
    uint8_t zero = row_cur[6][1].sym==0;
    row_cur[5][0].match[2] = zero;
    row_cur[5][-1].match[4] = zero;
    row_cur[5][-2].match[5] = zero;
    zero = row_cur[6][2].sym==0;
    row_cur[5][0].match[4] = zero;
    row_cur[5][-1].match[5] = zero;
    row_cur[5][0].match[5] = row_cur[6][3].sym==0;
  }

  template <int32_t f_DEC> int32_t offer_candidates(int32_t* nb_sym) {
    for( sym_pos = 0; sym_pos<32; ++sym_pos ) {
      const int32_t sym = pixel_context((uint32_t*)nb_sym);
      if( sym<0 )
        continue;
      int32_t is_sym;
      const int32_t apm_ctx = (int32_t)(sym_pos<MB_apm_pos_cap ? sym_pos : MB_apm_pos_cap)|((ctx_state_seen&15)<<3);
      if constexpr( f_DEC ) {
        is_sym = bit_node[ctr_node].template code_context_bit_apm<1>(cx, &bit_root[ctr_fallback], 0, &cand_apm, apm_ctx);
        row_cur[5][0].sym = sym;
      } else {
        is_sym = sym==row_cur[5][0].sym;
        bit_node[ctr_node].template code_context_bit_apm<0>(cx, &bit_root[ctr_fallback], is_sym, &cand_apm, apm_ctx);
      }
      if( is_sym )
        return sym;
      cx[0].exclusion_mask[sym] = cx[0].exclusion_gen;
    }
    return -1;
  }

  int32_t context_ids(uint32_t bucket_slot_at) {
    PixRec*const cur6 = row_cur[6];
    PixRec*const r8 = row_cur[8];
    PixRec*const r7 = row_cur[7];
    PixRec*const up9 = row_cur[9];
    PixRec*const row = row_cur[5];
    const int32_t bucket_at = ctx_bucket[idx_bound(bucket_slot_at, MB_Bucket_Volume)];
    bucket_idx = bucket_at;
    const int32_t up_m0 = cur6[0].match[0];
    const uint32_t nb2 = CtxIdx{}.bit<IX_model_nb2_p0>(row[-2].match[2]).bit<IX_model_nb2_p1>(row[-2].match[5]).bit<IX_model_nb2_p2>(row[-2].match[4]).bit<IX_model_nb2_p3>(row[-1].match[1]);
    const uint32_t sig1 = CtxIdx{}.bit<IX_model_sig1_p0>(grad[3]==0).bit<IX_model_sig1_p1>(grad[2]==0).bit<IX_model_sig1_p2>(grad[1]==0).bit<IX_model_sig1_p3>(grad[0]==0).bit<IX_model_sig1_p4>(up9[0].match[0]&r8[0].match[0]&up_m0&r7[0].match[0]).bit<IX_model_sig1_p5>(up9[0].match[1]&r8[0].match[1]&r7[0].match[1]&cur6[0].match[1]).above<IX_model_sig1_p6>(bucket_at).raw(nb2);
    int32_t id1;
    intern_ctx(ctx_id1, sizeof ctx_id1/sizeof ctx_id1[0], sig1, ctx_id1_used, id1);
    const uint32_t sig2 = CtxIdx{}.bit<IX_model_sig2_p0>(cur6[1].match[3]).bit<IX_model_sig2_p1>(up_m0).bit<IX_model_sig2_p2>(row[-1].match[5]).above<IX_model_sig2_p3>(id1);
    int32_t id2;
    intern_ctx(ctx_id2, sizeof ctx_id2/sizeof ctx_id2[0], sig2, ctx_id2_used, id2);
    return id2;
  }

  // The map's context: the neighbourhood match state the model already groups
  // its counters by, which is its coarsest activity measure.
  int32_t level_apm_context() const {
    return (int32_t)(ctx_state_seen&15);
  }

  // The signature is a context built from IDX layout parameters, so it is
  // bounded here rather than at each of the three call sites -- the table's own
  // length, passed in, since the three differ.
  static bool intern_ctx(uint16_t* tbl, size_t n, int32_t sig, uint32_t &used, int32_t &id) {
    const uint32_t at = idx_bound((uint32_t)sig, (uint32_t)n);
    const bool minted = tbl[at]==0xFFFF;
    if( minted )
      tbl[at] = used++;
    id = tbl[at];
    return minted;
  }

  int32_t run_scan(const PixRec* up1, const PixRec* up2, int32_t limit, int32_t &m_up0, int32_t &run) {
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
    const int8_t gen = cx[0].exclusion_gen;
    for( int32_t k = 1; k<5; ++k )
      cx[0].exclusion_mask[cx[0].mode_symbol[k]] = gen;
    cx[0].exclusion_mask[no_symbol] = gen;
  }

  Neighbours rank_neighbours() {
    const PixRec* up = row_cur[6];
    const PixRec* west = row_cur[5];
    const Neighbours n = {up[0].sym, west[-1].sym, up[1].sym, up[-1].sym};
    cx[0].mode_symbol[1] = n.up;
    cx[0].mode_symbol[2] = n.left;
    cx[0].mode_symbol[3] = n.up_next;
    cx[0].mode_symbol[4] = n.upleft;
    return n;
  }

  uint32_t match_context(const Neighbours &n, const PixRec* up, const PixRec* west) const {
    return CtxIdx{}.bit<IX_model_ix1_p0>(west[-1].match[2]).bit<IX_model_ix1_p1>(west[-1].match[4]).bit<IX_model_ix1_p2>(up[1].match[1]).bit<IX_model_ix1_p3>(n.up_next==n.upleft).bit<IX_model_ix1_p4>(west[-1].match[0]).bit<IX_model_ix1_p5>(up[0].match[1]);
  }

  uint16_t pair_key(const Neighbours &n, const PixRec* west) const {
    if( n.up!=n.left )
      return (uint16_t)(sym_rev[n.up]-n.left);
    if( n.up!=n.up_next )
      return (uint16_t)(sym_rev[n.up]-n.up_next);
    if( n.up!=n.upleft )
      return (uint16_t)(sym_rev[n.up]-n.upleft);
    return (uint16_t)(sym_rev[n.up]-west[-2].sym);
  }

  int32_t open_pixel(Neighbours &nb4, const PixRec* up, const PixRec* west) {
    nb4 = rank_neighbours();
    const int32_t nb = match_context(nb4, up, west);
    const int32_t key = pair_key(nb4, west);
    sym_cache = &sym_ctr[idx_bound(CtxIdx{}.above<IX_model_sym_cache_p0>(key),
                                  sizeof sym_ctr/sizeof sym_ctr[0])];
    const int32_t state = ctx_state[idx_bound((uint32_t)nb, sizeof ctx_state)];
    ctx_state_seen = state;
    SymPair*const pair = &group_ctr[idx_bound((uint32_t)state, sizeof group_ctr/sizeof group_ctr[0])][key];
    pix_cur = pair;
    // The same index seat_bucket writes through, built by the same
    // declaration -- state is the group factor, the two ranks the others.
    int32_t id2 = context_ids(MakeBucket((uint32_t)nb4.rank(pair[0].prev),
                                         (uint32_t)nb4.rank(pair[0].last),
                                         (uint32_t)state));
    if( (int32_t)alphabet<MB_small_alphabet ) {
      int32_t id3_used = ctx_id3_used;
      // Bounded like the other two interned tables; this one does not go
      // through intern_ctx because it retries at a folded signature.
      int32_t sig3 = (int32_t)idx_bound(CtxIdx{}.bits<IX_model_sig3_p0, IX_model_sig3_w0>(nb4.left).above<IX_model_sig3_p1>(id2),
                                       sizeof ctx_id3/sizeof ctx_id3[0]);
      uint16_t* id3p = &ctx_id3[sig3];
      id2 = id3p[0];
      if( id2==0xFFFF ) {
        if( id3_used>kCtxId3Limit )
          sig3 |= 0xF;
        id3p = &ctx_id3[sig3];
        id2 = id3p[0];
      }
      if( id2>=id3_used ) {
        id3p[0] = id3_used;
        ++ctx_id3_used;
        id2 = id3p[0];
      }
    }
    return id2;
  }

  void fill_run(int32_t sym, uint32_t head_word, uint32_t flags_word, uint32_t n) {
    for( uint32_t k = 0; k<n; ++k ) {
      pix_cur[0].prev = sym;
      row_cur[5][0].head4 = head_word;
      row_cur[5][0].match2345 = flags_word;
      ++row_cur[5];
    }
  }

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
    nb_sym[0] = pix_cur[0].last;
    nb_sym[1] = pix_cur[0].prev;
    for( int32_t c = 0; c<8; ++c )
      nb_sym[2+c] = sym_cache[c];
    load_neighbours(nb_sym);
  }

  void load_neighbours(int32_t* nb_sym) {
    const PixRec*const r5 = row_cur[5], *const r6 = row_cur[6], *const r7 = row_cur[7], *const r8 = row_cur[8];
    nb_sym[10] = r5[-2].sym;
    nb_sym[11] = r6[2].sym;
    nb_sym[12] = r7[1].sym;
    nb_sym[13] = r7[0].sym;
    nb_sym[14] = r6[-2].sym;
    nb_sym[15] = r7[-1].sym;
    nb_sym[16] = r5[-3].sym;
    nb_sym[17] = r6[3].sym;
    nb_sym[18] = r6[4].sym;
    nb_sym[19] = r5[-4].sym;
    nb_sym[20] = r6[-3].sym;
    nb_sym[21] = r7[2].sym;
    nb_sym[22] = r8[0].sym;
    nb_sym[23] = r7[-2].sym;
    nb_sym[24] = r5[-5].sym;
    nb_sym[25] = r8[1].sym;
    nb_sym[26] = r6[5].sym;
    nb_sym[27] = row_cur[9][0].sym;
    nb_sym[28] = r5[-7].sym;
    nb_sym[29] = r8[-1].sym;
    nb_sym[30] = r6[7].sym;
    nb_sym[31] = r7[3].sym;
  }

  void init_symbol_lists() {
    for( uint32_t s = 0; s<alphabet; ++s ) {
      sel1_list[s].init(kSel1Syms, 0, &sel1_ent[s*kSel1Syms]);
      sel0_list[s].init(kSel0Syms, 0, &sel0_ent[s*kSel0Syms]);
    }
  }

  void seed_alphabet() {
    uint8_t* map = alpha_map_store;
    alpha_map = map;
    memset(map, 1, alphabet);
    escape_list = &escape;
    escape.init(alphabet, 1, escape_ent);
    sel_cur = sel;
    sel1_list = new_sym_lists(alphabet, sel1_lists_store);
    sel0_list = new_sym_lists(alphabet, sel0_lists_store);
    init_symbol_lists();
  }

  void expand_alphabet() {
    SymList lists[16];
    uint32_t n_syms, j;
    uint32_t bits = depth;
    uint32_t mask = 0xFFFFFFFF>>(-(uint8_t)depth&31);
    uint32_t nbytes = (bits+7)>>3;
    clear_sym_lists(lists);
    uint32_t cap = mask+1;
    if( bits>8 )
      cap = no_symbol+1;
    alphabet = cx[0].code_alphabet_size<1>(0, cap);
    if( (int32_t)alphabet<=0x2000 ) {
      void* codes = sym_code_store;
      n_syms = alphabet;
      sym_code = (uint32_t*)codes;
      for( j = 0; j<n_syms; ++j )
        sym_code[j] = j;
      if( (int32_t)depth>8 ) {
        init_byte_lists(cx, lists, nbytes);
        if( n_syms ) {
          int32_t carry = 0;
          for( uint32_t s = 0; s<alphabet; ++s )
            sym_code[s] = lists[0].code_symbol_bytes<1>(cx, nbytes, 0, carry);
        }
      } else if( n_syms<=mask ) {
        init_gap_list(cx, lists, mask, n_syms);
        if( alphabet ) {
          int32_t run = 0;
          for( uint32_t s2 = 0; s2<alphabet; ++s2 ) {
            int32_t gap = lists[0].decode_symbol_list(cx);
            sym_code[s2] = gap+run;
            run += gap+1;
          }
        }
      }
      free_sym_entries(lists, 16);
    } else {
      depth = 8;
      height = height*nbytes;
      expand_alphabet();
      free_sym_entries(lists, 16);
    }
  }

  template <class T> uint8_t*write_row_at(uint8_t* out_at) {
    T* out = (T*)out_at;
    uint32_t i = 0;
    do {
      *out++ = (T)sym_code[row_cur[0][i+8].sym];
      ++i;
    } while( i<width );
    return (uint8_t*)out;
  }

  uint8_t*write_row(uint8_t* out_at, uint32_t nbytes, uint32_t row_w) {
    if( row_w<=0 )
      return out_at;
    if( nbytes==4 )
      return write_row_at<uint32_t>(out_at);
    if( nbytes==3 ) {
      SymEntry* out_ent = (SymEntry*)out_at;
      for( uint32_t step = 0; step<width; ++step, ++out_ent ) {
        const uint32_t code = sym_code[row_cur[0][step+8].sym];
        out_ent[0].set((uint16_t)code, (uint8_t)(code>>16));
      }
      return (uint8_t*)out_ent;
    }
    if( nbytes==2 )
      return write_row_at<uint16_t>(out_at);
    if( depth==8 )
      return write_row_at<uint8_t>(out_at);
    int32_t bits = 0;
    uint8_t* out_bits = out_at;
    const int32_t bpp = depth;
    for( uint32_t x = 0; x<width; ++x ) {
      const uint32_t code = sym_code[row_cur[0][x+8].sym];
      bits -= bpp;
      if( bits<0 ) {
        bits = 8-bpp;
        *++out_bits = code<<(8-bpp);
      } else {
        out_bits[0] |= code<<bits;
      }
    }
    return out_bits;
  }

  template <int32_t f_DEC> void code_row_pixels() {
    for( uint32_t x2 = 0; x2<width; ) {
      x2 += f_DEC ? decode_pixel(x2) : code_pixel(x2);
      init_tables();
    }
  }

  void begin_row() {
    start_row();
    ++row_cur[5];
    ++row_cur[6];
    ++row_cur[7];
    ++row_cur[8];
    ++row_cur[9];
    grad[0] = match_seed(row_cur[6], 5);
    grad[1] = match_seed(row_cur[7], 5);
    grad[2] = 0;
    grad[3] = 0;
  }

  void seed_context_groups() {
    int16_t w1;
    uint16_t w2;
    int32_t w3, w4;
    int32_t bucket = 0;
    for( int32_t g = 0; g<15; ++g ) {
      int32_t flags = ctx_group_flags[g];
      ctx_state[flags] = g;
      GroupFolds fold;
      fold.Init(flags);
      for( int32_t lo = 0; lo<5; ++lo ) {
        for( int32_t hi = 0; hi<5; ++hi ) {
          FreqRec* rec = seat_bucket(g, lo, hi, bucket);
          int32_t alpha = alphabet;
          w1 = 2;
          rec[0].w[0] = 2;
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
            w1 = w4+2;
            w4 = 0;
            rec[0].w[4] = 0;
          } else {
            rec[0].w[4] = w4;
          }
          if( fold.w3_to_w1 ) {
            w1 += w3;
            w3 = 0;
            rec[0].w[3] = 0;
          } else {
            rec[0].w[3] = w3;
          }
          if( fold.w2_to_w1 ) {
            rec[0].w[1] = w2+w1;
            w2 = 0;
            rec[0].w[2] = 0;
          } else {
            rec[0].w[2] = w2;
            rec[0].w[1] = w1;
          }
          int32_t has3 = w3!=0;
          int32_t has4 = w4!=0;
          if( w2 ) w2 = 1;
          int32_t lvl = has4+has3+w2+2;
          if( lvl<=alpha ) {
            rec[0].b14 = lvl;
          } else {
            rec[0].b14 = has4+has3+w2+1;
            rec[0].w[0] = 0;
          }
          bucket += rec[0].seed_pair(lo, hi, alpha);
        }
      }
      SymPair* gctr = group_ctr[g];
      for( uint32_t gi = 0; gi<0x10000; ++gi ) {
        gctr[gi].last = no_symbol;
        gctr[gi].prev = no_symbol;
      }
    }
  }

  void interleave_depth_bytes(uint8_t* dst, uint8_t* expand_buf) {
    uint8_t* row[19];
    const int32_t n_pix = height*width;
    const int32_t nchunk = ((int32_t)depth_raw+7)>>3;
    if( nchunk>0 ) {
      const int32_t chunk = n_pix/nchunk;
      int32_t q5 = 0;
      uint8_t* row_at = expand_buf;
      for(; q5<=nchunk-6; q5 += 5, row_at += 5*chunk ) {
        row[q5] = row_at;
        for( int32_t n = 0; n<4; ++n )
          row[q5+1+n] = &expand_buf[chunk*(q5+1+n)];
      }
      for( int32_t q1 = nchunk>=6 ? q5 : 0; q1<nchunk; ++q1 )
        row[q1] = &expand_buf[chunk*q1];
    }
    uint8_t* interleave_at = dst;
    int32_t q2 = 0;
    for( int32_t written = 0; written<n_pix; ++written ) {
      uint8_t* p = row[q2];
      *interleave_at++ = p[0];
      row[q2] = p+1;
      if( ++q2==nchunk )
        q2 = 0;
    }
  }

  template <int32_t f_DEC> void code_plane_slow(uint8_t* buf) {
    cx[0].rc_begin<f_DEC>();
    if constexpr( f_DEC )
      expand_alphabet();
    else
      reduce_alphabet(buf);
    seed_context_groups();
    seed_alphabet();
    uint8_t* expand_buf = nullptr;
    uint8_t* out_at = nullptr;
    uint16_t* word = nullptr;
    uint32_t nbytes = 0;
    if constexpr( f_DEC ) {
      uint8_t* dst_buf = &buf[-(depth<8)];
      if( depth!=depth_raw ) {
        dst_buf = (uint8_t*)bmf_new(height*width+3);
        expand_buf = dst_buf;
      }
      nbytes = (depth+7)>>3;
      out_at = dst_buf;
    } else {
      word = sym_word;
    }
    for( int32_t y = 0; y<height; ++y ) {
      begin_row();
      if constexpr( f_DEC ) {
        code_row_pixels<1>();
        out_at = write_row(out_at, nbytes, width);
      } else {
        for( uint32_t x = 0; x<width; ++x )
          row_cur[5][x].sym = word[x];
        word += width;
        code_row_pixels<0>();
      }
    }
    cx[0].rc_end<f_DEC>();
    if constexpr( f_DEC ) {
      if( depth_raw!=depth )
        interleave_depth_bytes(buf, expand_buf);
      free(expand_buf);
    }
  }

  void model_plane_slow(uint8_t* pixels) {
    code_plane_slow<0>(pixels);
  }

  void unmodel_plane_slow(uint8_t* dst) {
    code_plane_slow<1>(dst);
  }

  template <int32_t f_DEC> int32_t code_run_length(int32_t bucket, int32_t cap, int32_t len) {
    const int32_t top = bucket;
    int32_t idx = 0;
    int32_t mask = 1<<(bucket&31);
    int32_t seen = 0;
    do {
      if( (mask|seen)<cap ) {
        BitCtr* ctr = &run_ctr[idx_bound(MakeRunCtr((uint32_t)((seen==0)+(bucket==top)), (uint32_t)bucket),
                                         MB_RunCtr_Volume)];
        const uint32_t bin_tot = ctr[0].n[0]+ctr[0].n[1];
        int32_t run_bit;
        if constexpr( f_DEC ) {
          run_bit = cx[0].decode_bit(ctr[0].n[0], ctr[0].n[1]);
        } else {
          run_bit = (len&mask)!=0;
          cx[0].encode_bit(ctr[0].n[0], ctr[0].n[1], run_bit);
        }
        if( ctr[0].limit<bin_tot )
          ctr[0].scale_rare();
        ctr[0].n[run_bit] += 8;
        if( run_bit )
          idx |= mask;
        seen |= idx&mask;
      }
      --bucket;
      mask >>= 1;
    } while( mask );
    return idx;
  }

  int32_t decode_stage_two(int32_t* nb_sym, int32_t idx_s) {
    exclude_stage_one();
    sel[0] = nullptr;
    seed_candidates(nb_sym);
    if( offer_candidates<1>(nb_sym)>=0 ) return idx_s+1;
    SymList** sel_p = load_selectors();
    while( 1 ) {
      if( sel_p>&escape_list ) bmf_fatal(bmf_read_error);
      if( (sel_p[0])[0].live ) {
        int32_t lsym = (sel_p[0])[0].decode_symbol_list(cx);
        row_cur[5][0].sym = lsym;
        if( lsym>=0 ) return idx_s+1;
        sel_p = sel_cur;
      }
      sel_cur = ++sel_p;
    }
  }

  int32_t decode_pixel(int32_t x) {
    alignas(16) int32_t nb_sym[32];
    uint32_t lvl_a;
    int32_t m_up0, run, bit, idx_s;
    PixRec* cur6 = row_cur[6];
    PixRec* row = row_cur[5];
    Neighbours nb4;
    int32_t id2 = open_pixel(nb4, cur6, row);
    if( row_cur[5][-1].match[1]&row_cur[5][-1].match[0] ) {
      PixRec* up1 = row_cur[6];
      PixRec* up2 = row_cur[7];
      if( (uint8_t)(up2[2].match[1]&up2[1].match[1]&up2[0].match[1]&up1[3].match[1]&up1[2].match[1]&up1[1].match[1]&up1[0].match[1]&up1[0].match[0]&(int8_t)up1[-1].match[1])&up2[3].match[1] ) {
        const int32_t idx1 = run_scan(up1, up2, width-x, m_up0, run);
        int32_t bucket = run_bucket[idx1];
        int32_t esc_at = CtxIdx{}.above<IX_model_esc_at_p0>(bucket).bit<IX_model_esc_at_p1>(up2[run+3].match[1]&up2[run+2].match[1]).bit<IX_model_esc_at_p2>(m_up0).bit<IX_model_esc_at_p3>(alpha_map[nb4.up]).raw(1);
        bit = esc_ctr[esc_at].decode_context_bit(cx, esc_ctr);
        int32_t msym1c = cx[0].mode_symbol[1];
        hit = bit;
        *(((uint8_t*)alpha_map)+msym1c) = bit;
        const int32_t hit_a = hit;
        idx_s = hit_a ? idx1 : idx1>1 ? code_run_length<1>(bucket, idx1, 0) : 0;
        if( idx_s )
          row_cur[5][idx_s-1].sym = row_cur[5][-1].sym;
        if( idx_s>hit_a ) {
          row_cur[6] = row_cur[6]+idx_s-hit_a;
          PixRec* r8b = row_cur[8];
          PixRec* r7b = row_cur[7]+idx_s;
          row_cur[7] = r7b-hit_a;
          row_cur[8] = r8b+idx_s-hit_a;
          PixRec* rec = row_cur[5];
          row_cur[9] = row_cur[9]+idx_s-hit_a;
          rec[0].match2345 = 0x01010101;
          row_cur[5][0].head4 = 0x01010101;
          SymPair* pixp = pix_cur;
          const int32_t run_sym = cx[0].mode_symbol[1];
          pixp[0].prev = pixp[0].last;
          row_cur[5][0].sym = (uint16_t)run_sym;
          pix_cur[0].last = (uint16_t)run_sym;
          const int32_t flags_word = row_cur[5][0].match2345;
          const int32_t head_word = row_cur[5][0].head4;
          ++row_cur[5];
          fill_run(run_sym, head_word, flags_word, idx_s-hit_a-1);
          seed_after_run(row_cur[5], run_sym);
        }
        if( hit_a )
          return idx_s;
        return decode_stage_two(nb_sym, idx_s);
      }
    }
    FreqRec* freq = &grid[bucket_idx];
    FreqRec* freq_tbl = &grid[id2+kFreqTableOffset];
    {
      auto decode_against = [&](FreqRec* rec, int32_t kind) {
                              const int32_t lvl = rec[0].template code_level<1>(cx, 0, level_apm[kind], level_apm_context());
                              hit = lvl;
                              return lvl;
                            };
      const int32_t tot = freq_tbl[0].w[5];
      if( tot ) {
        if( tot==1 )
          freq_tbl[0].blend_from(freq[0]);
        lvl_a = decode_against(freq_tbl, 0);
        if( freq_tbl[0].b14 ) {
          --freq_tbl[0].b14;
          ++freq[0].w[5];
          ++freq[0].w[lvl_a];
        }
      } else {
        lvl_a = decode_against(freq, 1);
        freq_tbl[0].w[5] = freq_tbl[0].w[lvl_a]++!=0;
      }
    }
    if( lvl_a ) {
      row_cur[5][0].sym = cx[0].mode_symbol[lvl_a];
      return 1;
    }
    return decode_stage_two(nb_sym, 0);
  }

  int32_t code_pixel(int32_t x) {
    alignas(16) int32_t nb_sym[32];
    uint32_t done;
    int32_t m_up0, run, amap, runlen, lvl_a;
    PixRec* cur6c, * up2;
    PixRec* cur6 = row_cur[6];
    PixRec* cur5 = row_cur[5];
    Neighbours nb4;
    int32_t id2 = open_pixel(nb4, cur6, cur5);
    PixRec* cur2 = cur5;
    PixRec* row = row_cur[5];
    int32_t m_w1b = row[-1].match[1];
    int32_t m_w0 = row[-1].match[0];
    PixRec* rowp = row;
    if( (m_w1b&m_w0)!=0&&(cur6c = row_cur[6], up2 = row_cur[7], ((uint8_t)(up2[2].match[1]&up2[1].match[1]&up2[0].match[1]&cur6c[3].match[1]&cur6c[2].match[1]&cur6c[1].match[1]&cur6c[0].match[1]&cur6c[0].match[0]&cur6c[-1].match[1])&up2[3].match[1])!=0) ) {
      const int32_t run_len = run_scan(cur6c, up2, width-x, m_up0, run);
      int32_t run_pair = (uint8_t)(up2[run+3].match[1]&up2[run+2].match[1]);
      const int32_t bucket = run_bucket[run_len];
      amap = CtxIdx{}.bit<IX_model_amap_p0>(run_pair).bit<IX_model_amap_p1>(m_up0).above<IX_model_amap_p2>(bucket).bit<IX_model_amap_p3>(alpha_map[nb4.up]);
      runlen = 0;
      if( rowp[0].sym==nb4.up ) {
        do ++runlen; while( runlen<run_len&&rowp[runlen].sym==nb4.up );
      }
      const int32_t run_hit = runlen==run_len;
      if( runlen>run_hit ) {
        row_cur[6] = &cur6c[runlen-run_hit];
        row_cur[7] = &up2[runlen-run_hit];
        PixRec* row_cur9 = row_cur[9];
        row_cur[8] = row_cur[8]+runlen-run_hit;
        row_cur[9] = row_cur9+runlen-run_hit;
        rowp[0].match2345 = 0x01010101;
        row_cur[5][0].head4 = 0x01010101;
        pix_cur[0].prev = pix_cur[0].last;
        uint16_t wp = nb4.up;
        row_cur[5][0].sym = wp;
        pix_cur[0].last = wp;
        PixRec* r3 = row_cur[5];
        uint32_t rec_word = ((uint32_t*)r3)[0];
        const int32_t flag_word = r3[0].match2345;
        cur2 = r3+1;
        row_cur[5] = r3+1;
        if( runlen-run_hit!=1 ) {
          int32_t run_left = runlen-run_hit-1;
          uint32_t half = run_left/2;
          if( run_left/2 ) {
            fill_run(nb4.up, rec_word, flag_word, 2*half);
            cur2 = row_cur[5];
            done = 2*half+1;
          } else {
            done = 1;
          }
          if( (uint32_t)run_left>(done-1) ) {
            pix_cur[0].prev = nb4.up;
            row_cur[5][0].head4 = rec_word;
            row_cur[5][0].match2345 = flag_word;
            PixRec* cur5p1b = row_cur[5]+1;
            row_cur[5] = cur5p1b;
            cur2 = cur5p1b;
          }
        }
        seed_after_run(cur2, nb4.up);
      }
      (&esc_ctr[(amap+1)])[0].encode_context_bit(cx, esc_ctr, run_hit);
      int32_t msym1 = cx[0].mode_symbol[1];
      uint8_t* amap_at = (uint8_t*)alpha_map;
      hit = run_hit;
      amap_at[msym1] = run_hit;
      if( !run_hit&&run_len!=1 )
        code_run_length<0>(bucket, run_len, runlen);
      if( hit )
        return runlen;
    } else {
      FreqRec* binp = &grid[bucket_idx];
      FreqRec* frec = &grid[id2+kFreqTableOffset];
      auto code_against = [&](FreqRec* rec, int32_t kind) {
                            const int32_t lvl = rec[0].template code_level<0>(cx, nb4.rank(rowp[0].sym), level_apm[kind], level_apm_context());
                            hit = lvl;
                            return lvl;
                          };
      const uint32_t grid_kind = frec[0].w[5];
      if( grid_kind ) {
        if( grid_kind==1 ) {
          frec[0].blend_from(binp[0]);
          rowp = row_cur[5];
        }
        lvl_a = code_against(frec, 0);
        if( frec[0].b14 ) {
          --frec[0].b14;
          ++binp[0].w[5];
          ++binp[0].w[lvl_a];
        }
      } else {
        lvl_a = code_against(binp, 1);
        frec[0].w[5] = frec[0].w[lvl_a]++!=0;
      }
      if( lvl_a ) return 1;
      runlen = 0;
    }
    exclude_stage_one();
    sel[0] = nullptr;
    seed_candidates(nb_sym);
    if( offer_candidates<0>(nb_sym)>=0 ) return runlen+1;
    SymList** sel_p = load_selectors();
    while( 1 ) {
      // The same guard the decoder's copy of this walk has always had.  The
      // escape list is the last one and always codes, so reaching past it means
      // every list went empty -- which a rescale schedule swept to its extreme
      // can do, and which walked off the end of the array here while the
      // decoder refused cleanly.
      if( sel_p>&escape_list ) bmf_fatal(bmf_read_error);
      if( (sel_p[0])[0].live ) {
        if( (sel_p[0])[0].code_symbol(cx, row_cur[5][0].sym) ) return runlen+1;
        sel_p = sel_cur;
      }
      sel_cur = ++sel_p;
    }
  }

  static const int32_t kSel1Syms = MB_sel1_syms;
  static const int32_t kSel0Syms = MB_sel0_syms;
  SymEntry sel1_ent[(no_symbol+1)*kSel1Syms];
  SymEntry sel0_ent[(no_symbol+1)*kSel0Syms];
  SymEntry escape_ent[no_symbol+1];
  alignas(8) uint8_t sel1_lists_store[SymListBlock::bytes(no_symbol+1)];
  alignas(8) uint8_t sel0_lists_store[SymListBlock::bytes(no_symbol+1)];
  uint8_t alpha_map_store[no_symbol+1];
  uint32_t sym_code_store[no_symbol+1];
  uint8_t run_bucket_store[kMaxWidth+1];
  PixRec row_store[5][kMaxWidth+kRowMargin+kRowTail];
  BMFState* cx;
};
