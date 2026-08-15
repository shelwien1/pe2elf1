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
  void**free_workspace(int8_t do_free);

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
    int32_t near_hit = 32*near+((result==nb[10])<<6);
    int32_t far_hit = 16*far;
    int32_t ctx0 = far_hit+near_hit;
    ctr_node = ctx0;
    if( (far_hit+near_hit==0)&&pos>6 )
      return -1;
    SymList* sel0 = sel0_list;
    int32_t ctx1 = ctx0+8*sym_in_top((sel0[mode_symbol[1]].ent), 10, result);
    ctr_node = ctx1;
    SymEntry *list_prev = sel0[mode_symbol[2]].ent;
    SymEntry *list_sym = sel0[result].ent;
    int32_t fallback = sym_in_top((sel0[mode_symbol[3]].ent), 6, result)+2*sym_in_top(list_sym, 10, mode_symbol[2])+4*sym_in_top(list_prev, 4, result);
    int32_t ctx2 = fallback+ctx1;
    if( pos<=14||(ctx2&0xB)!=0 ) {
      ctr_node = (pos<<7)+ctx2;
      ctr_fallback = fallback+8*(pos>9);
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
                sym_cache[3] = sym_cache[2];
                sym_cache[2] = sym_cache[1];
                sym_cache[1] = sym_cache[0];
              } else {
                int32_t c5 = cache[5];
                if( just==c5 ) {
                  cache[5] = c4;
                  sym_cache[4] = sym_cache[3];
                  sym_cache[3] = sym_cache[2];
                  sym_cache[2] = sym_cache[1];
                  sym_cache[1] = sym_cache[0];
                } else {
                  int32_t c6 = cache[6];
                  if( just==c6 ) {
                    cache[6] = c5;
                  } else {
                    cache[7] = c6;
                    sym_cache[6] = sym_cache[5];
                  }
                  sym_cache[5] = sym_cache[4];
                  sym_cache[4] = sym_cache[3];
                  sym_cache[3] = sym_cache[2];
                  sym_cache[2] = sym_cache[1];
                  sym_cache[1] = sym_cache[0];
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

  void expand_alphabet() {
    SymList lists[16];
    uint32_t* codes_p;
    int32_t left2, left;
    uint32_t i, cap, n_syms, j, carry;
    uint32_t bits = depth;
    uint32_t mask = 0xFFFFFFFF>>(-(uint8_t)depth&31);
    uint32_t nbytes = (bits+7)>>3;
    for( i = 0; i<8; ++i ) {
      lists[2*i].ent = nullptr;
      lists[2*i+1].ent = nullptr;
    }
    cap = mask+1;
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
                uint32_t piece = decode_symbol_list(&lists[4*b+carry]);
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
            int32_t gap = decode_symbol_list(&lists[0]);
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

  void unmodel_plane_slow(uint8_t* dst) {
    uint8_t* row[19];
    ModelBlock* this_1, * blk;
    uint32_t has4, jj, wt, nbytes, row_w;
    uint16_t* out16;
    SymEntry* out_ent;
    uint8_t* out_at, * dst_keep;
    int32_t g0;
    int32_t done;
    int32_t lo1, g1;
    uint32_t* x6;
    uint8_t* dst_base, * dst_buf, * buf, * expand_buf, * out_bits, * row_at, * interleave_at, * p, * out8;
    PixRec* kk, * row_cur3, * row_cur2, * row_cur1, * n_syms;
    int32_t g, lo, w2, w4, w2n, lvl_n, live, bucket, x, x2, step, x7, x3, x4, bits, bpp, bpp_raw, nchunk, n_pix, chunk, q5, q1, at, q2, written, n_pix2, x5;
    SymListBlock* has3, * alpha;
    SymList *i, *j;
    PixRec* lists, * t;
    dst_keep = dst;
    dst_base = &dst[-(depth<8)];
    rc_begin_decode();
    expand_alphabet();
    this_1 = this;
    g1 = 0;
    g = 0;
    do {
      int32_t flags = ctx_group_flags[g];
      this_1->ctx_state[flags] = g;
      lo1 = 0;
      g0 = g;
      int32_t y0 = flags&4;
      int32_t f_b1 = flags&2;
      int32_t f_b4 = flags&0x10;
      lo = 0;
      int32_t f_b0 = flags&1;
      int32_t f_b5 = flags&0x20;
      int32_t f_b3 = flags&8;
      do {
        lo1 = lo;
        int32_t k = 0;
        do {
          this_1->ctx_bucket[g+15*lo+75*k] = g1;
          int32_t m5 = this_1->alphabet;
          FreqRec *rec = &this_1->grid[g1];
          rec->w[1] = 2;
          rec->w[2] = 2;
          rec->w[3] = 2;
          rec->w[4] = 2;
          if( y0 ) {
            w2 = (uint16_t)(rec->w[4]+rec->w[3]);
            rec->w[3] = w2;
            w4 = 0;
            rec->w[4] = 0;
          } else {
            w2 = rec->w[3];
            w4 = rec->w[4];
          }
          if( f_b1 ) {
            w2n = (uint16_t)(w4+rec->w[2]);
            rec->w[2] = w2n;
            w4 = 0;
            rec->w[4] = 0;
          } else {
            w2n = rec->w[2];
          }
          if( f_b4 ) {
            w2n = (uint16_t)(w2+w2n);
            rec->w[2] = w2n;
            w2 = 0;
            rec->w[3] = 0;
          }
          if( f_b0 ) {
            rec->w[1] += w4;
            w4 = 0;
            rec->w[4] = 0;
          }
          if( f_b3 ) {
            rec->w[1] += w2;
            w2 = 0;
            rec->w[3] = 0;
          }
          if( f_b5 ) {
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
          if( rec->w[lo1]&&rec->w[k]&&(uint8_t)lvl_n<=m5 ) {
            live = 1;
            int16_t lvl = (uint8_t)(1<<((5-lvl_n)&31));
            rec->b15 = lvl;
            rec->w[6] = lvl<<6;
            rec->w[lo1] += lvl;
            rec->w[k] += rec->b15;
            rec->w[5] = rec->w[0]+rec->w[4]+rec->w[3]+rec->w[2]+rec->w[1];
          } else {
            live = 0;
          }
          g1 += live;
          ++k;
        } while( k<5 );
        lo = lo1+1;
      } while( lo1+1<5 );
      int32_t gi = 0;
      SymPair* gctr = this_1->group_ctr[g0];
      do {
        gctr[gi].last = no_symbol;
        gctr[gi++].prev = no_symbol;
      } while( gi<0x10000 );
      g = g0+1;
    } while( g0+1<15 );
    dst_buf = dst_base;
    blk = (ModelBlock*)(this_1);
    buf = (uint8_t*)bmf_new(this_1->alphabet);
    uint32_t alpha_n = this_1->alphabet;
    this_1->alpha_map = (uint8_t*)buf;
    memset(buf, 1, alpha_n);
    blk->escape_list = &blk->escape;
    blk->escape.init(blk->alphabet, 1);
    blk->sel_cur = blk->sel;
    wt = blk->alphabet;
    has3 = (SymListBlock*)bmf_new(SymListBlock::bytes(wt));
    if( has3 ) {
      has3->n = wt;
      SymList *list_a = has3->list;
      for( i = list_a; wt; --wt )
        (list_a++)->ent = nullptr;
    } else {
      i = nullptr;
    }
    has4 = blk->alphabet;
    blk->sel1_list = i;
    alpha = (SymListBlock*)bmf_new(SymListBlock::bytes(has4));
    if( alpha ) {
      alpha->n = has4;
      SymList *list_b = alpha->list;
      for( j = list_b; has4; --has4 )
        (list_b++)->ent = nullptr;
    } else {
      j = nullptr;
    }
    blk->sel0_list = j;
    if( !(blk->alphabet<=0) ) {
      int32_t s = 0;
      do {
        blk->sel1_list[s].init(99, 0);
        blk->sel0_list[s++].init(33, 0);
      } while( (uint32_t)s<blk->alphabet );
    }
    jj = blk->depth;
    if( (uint32_t)jj==blk->depth_raw ) {
      expand_buf = nullptr;
    } else {
      dst_buf = (uint8_t*)bmf_new(blk->height*blk->width+3);
      jj = blk->depth;
      expand_buf = dst_buf;
    }
    nbytes = (jj+7)>>3;
    if( blk->height>0 ) {
      out_at = dst_buf;
      bucket = 0;
      while( 1 ) {
        blk->row_cur[5]->match[1] = blk->row_cur[5][-1].sym==0;
        blk->row_cur[5]->match[3] = blk->row_cur[6][-1].sym==0;
        kk = blk->row_cur[4];
        row_cur3 = blk->row_cur[3];
        row_cur2 = blk->row_cur[2];
        row_cur1 = blk->row_cur[1];
        n_syms = blk->row_cur[0];
        blk->row_cur[4] = row_cur3;
        blk->row_cur[3] = row_cur2;
        blk->row_cur[2] = row_cur1;
        blk->row_cur[1] = n_syms;
        blk->row_cur[0] = kk;
        kk += 7;
        blk->row_cur[5] = kk;
        n_syms += 7;
        blk->row_cur[6] = n_syms;
        blk->row_cur[7] = row_cur1+7;
        blk->row_cur[8] = row_cur2+7;
        blk->row_cur[9] = row_cur3+7;
        {
          uint8_t zero = n_syms[1].sym==0;
          kk->match[2] = zero;
          blk->row_cur[5][-1].match[4] = zero;
          blk->row_cur[5][-2].match[5] = zero;
          zero = blk->row_cur[6][2].sym==0;
          blk->row_cur[5]->match[4] = zero;
          blk->row_cur[5][-1].match[5] = zero;
        }
        blk->row_cur[5]->match[5] = blk->row_cur[6][3].sym==0;
        lists = blk->row_cur[6];
        t = blk->row_cur[7];
        ++blk->row_cur[5];
        ++lists;
        ++blk->row_cur[8];
        blk->row_cur[6] = lists;
        ++t;
        blk->row_cur[7] = t;
        ++blk->row_cur[9];
        blk->grad[0] = lists[3].match[0]+lists[2].match[0]+lists[1].match[0]+lists[0].match[0]+lists[4].match[0]-5;
        blk->grad[3] = 0;
        blk->grad[2] = 0;
        row_w = blk->width;
        blk->grad[1] = t[3].match[0]+t[2].match[0]+t[1].match[0]+t[0].match[0]+t[4].match[0]-5;
        x = bucket;
        if( row_w<=0 )
          break;
        x2 = 0;
        do {
          int32_t y = ((ModelBlock*)blk)->decode_pixel(x2);
          blk->init_tables();
          row_w = blk->width;
          x2 += y;
        } while( (uint32_t)x2<blk->width );
        x = bucket;
        if( nbytes!=4 )
          goto list_ready;
        if( row_w>0 ) {
          uint32_t *out32 = (uint32_t*)out_at;
          int32_t y2 = 0;
          do
            *out32++ = blk->sym_code[blk->row_cur[0][y2+++8].sym];
          while( (uint32_t)y2<blk->width );
          out_at = (uint8_t*)out32;
          goto next_list;
        }
  next_list:
        bucket = x+1;
        // `height` is `int32_t` and this side is already `uint32_t`, so C
        // converts the right operand too.  Written where it happens.
        if( (uint32_t)bucket>=(uint32_t)blk->height )
          goto list_done;
      }
      if( nbytes==4 )
        goto next_list;
  list_ready:
      if( nbytes==3 ) {
        if( row_w>0 ) {
          out_ent = (SymEntry*)out_at;
          step = 0;
          do {
            x6 = blk->sym_code;
            x7 = blk->row_cur[0][step+8].sym;
            out_ent->set((uint16_t)x6[x7], (uint8_t)(x6[x7]>>16));
            ++step;
            ++out_ent;
          } while( (uint32_t)step<blk->width );
          out_at = (uint8_t*)out_ent;
        }
        goto next_list;
      }
      if( nbytes!=2 ) {
        if( blk->depth==8 ) {
          if( row_w>0 ) {
            out8 = out_at;
            x5 = 0;
            do
              *out8++ = (uint8_t)blk->sym_code[blk->row_cur[0][x5+++8].sym];
            while( (uint32_t)x5<blk->width );
            out_at = out8;
          }
        } else if( row_w>0 ) {
          x4 = 0;
          bits = 0;
          out_bits = out_at;
          do {
            bpp = blk->depth;
            bits -= bpp;
            if( bits<0 ) {
              bits = 8-bpp;
              *++out_bits = blk->sym_code[blk->row_cur[0][x4+8].sym]<<((8-bpp)&31);
            } else {
              *out_bits |= blk->sym_code[blk->row_cur[0][x4+8].sym]<<(bits&31);
            }
            ++x4;
          } while( (uint32_t)x4<blk->width );
          out_at = out_bits;
        }
        goto next_list;
      }
      if( row_w>0 ) {
        out16 = (uint16_t*)out_at;
        x3 = 0;
        do
          *out16++ = blk->sym_code[blk->row_cur[0][x3+++8].sym];
        while( (uint32_t)x3<blk->width );
        out_at = (uint8_t*)out16;
      }
      goto next_list;
    }
  list_done:
    rc_end_decode();
    bpp_raw = blk->depth_raw;
    if( (uint32_t)bpp_raw!=blk->depth ) {
      nchunk = (bpp_raw+7)>>3;
      if( nchunk<=0 ) {
        n_pix = blk->height*blk->width;
      } else {
        done = 0;
        n_pix = blk->height*blk->width;
        chunk = n_pix/nchunk;
        if( nchunk>=6 ) {
          q5 = 0;
          row_at = expand_buf;
          do {
            row[q5] = row_at;
            row_at += 5*chunk;
            row[q5+1] = &expand_buf[chunk*(q5+1)];
            row[q5+2] = &expand_buf[chunk*(q5+2)];
            row[q5+3] = &expand_buf[chunk*(q5+3)];
            row[q5+4] = &expand_buf[chunk*(q5+4)];
            q5 += 5;
          } while( q5<=nchunk-6 );
          done = q5;
        }
        q1 = done;
        at = chunk*done;
        do {
          row[q1] = &expand_buf[at];
          at += chunk;
          ++q1;
        } while( q1<nchunk );
      }
      if( n_pix>0 ) {
        interleave_at = dst_keep;
        q2 = 0;
        written = 0;
        do {
          p = row[q2];
          *interleave_at = *p;
          n_pix2 = blk->height*blk->width;
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
    uint8_t g_ab;
    int8_t gen;
    FreqRec* freq2;
    PixRec* up4;
    FreqRec* freq_tbl;
    SymList* sel1, ** sel_p;
    int32_t bin_tot;
    int32_t arg_cum, arg_high, arg_tot;
    uint32_t done, lvl_a, pos1;
    PixRec* r8, * r7, * rec;
    SymPair* pixq;
    PixRec* r8b, * r7b, * next, * r5, * cur6b;
    uint8_t g_a, g_b, g_c, g_d, g_e;
    int32_t n2r;
    int16_t s1c, w1s, s3b;
    int32_t up_sym, left_sym, up_p1_sym, up_m1_sym, m_lo, m_up, nb, key, state, pair_last, cap, bucket_at, up_m0, m_w1, nb2, all_up, sig1, id1, sig2, id2, s1a, m_up0, idx1, run, bucket, bit, msym1c, hit_a, idx_s, mask, seen, run0, s1b, flags_word, s3c, s3d, s1d, tot, target, cum, w6a, b15a, w5a, msym3, msym1, msym2, psym, bit2, msym1b, lsym, b15, w2t, w3t, w4t, w5, w0r, w1r, q1, w2r, w3, w3r, s0a, span, w4r, n4r, target2, cum2, lvl_b, b15b, w5b, w6b, s0c, s3a;
    PixRec* row;
    SymPair* pair;
    PixRec* cur6;
    uint16_t pair_prev;
    PixRec* up5, * r8c;
    SymPair *pixp, *pixr;
    uint16_t q1w, tot2, k0;
    uint32_t si, s2a, k2, k2h;
    uint16_t k1;
    uint16_t k4, k3;
    PixRec* up2;
    PixRec* up1;
    PixRec* up3;
    PixRec* up9;
    PixRec* cur6c;
    BitCtr* ctr;
    cur6 = (PixRec*)row_cur[6];
    up_sym = cur6->sym;
    row = row_cur[5];
    left_sym = row[-1].sym;
    up_p1_sym = cur6[1].sym;
    up_m1_sym = cur6[-1].sym;
    nb_sym[0] = up_sym;
    mode_symbol[1] = up_sym;
    nb_sym[1] = left_sym;
    mode_symbol[2] = left_sym;
    nb_sym[3] = up_p1_sym;
    mode_symbol[3] = up_p1_sym;
    m_lo = cur6->match[1]+4*(up_p1_sym==up_m1_sym);
    m_up = cur6[1].match[1];
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
    state = ctx_state[nb];
    ctx_state_seen = state;
    pair = &group_ctr[state][key];
    pix_cur = pair;
    pair_last = pair->last;
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
    pair_prev = pair->prev;
    if( pair_prev==nb_sym[0] ) {
      cap += 75;
    } else if( pair_prev==nb_sym[1] ) {
      cap += 150;
    } else if( pair_prev==nb_sym[3] ) {
      cap += 225;
    } else if( pair_prev==nb_sym[2] ) {
      cap += 300;
    }
    r8 = row_cur[8];
    bucket_at = ctx_bucket[state+cap];
    r7 = row_cur[7];
    bucket_idx = bucket_at;
    up_m0 = cur6->match[0];
    nb_sym[2] = up_m0;
    m_w1 = row[-1].match[1];
    up9 = row_cur[9];
    nb2 = CtxIdx{}.bit<3>(row[-2].match[2]).bit<2>(row[-2].match[5])
                  .bit<1>(row[-2].match[4]).bit<0>(m_w1);
    all_up = ((uint8_t)(up9->match[0]&r8->match[0]&nb_sym[2]&r7->match[0])<<9)+((uint8_t)(up9->match[1]&r8->match[1]&r7->match[1]&cur6->match[1])<<8)+(bucket_at<<10)+nb2;
    cur6b = cur6;
    // The four gradient accumulators as four "is it flat" bits, over the
    // four-bit `nb2` below them; `all_up` carries the two run bits at 8 and 9
    // and the bucket from 10 up.
    sig1 = CtxIdx{}.bit<7>(grad[3]==0).bit<6>(grad[2]==0)
                   .bit<5>(grad[1]==0).bit<4>(grad[0]==0)
                   .raw(all_up);
    id1 = ctx_id1[sig1];
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
    id2 = ctx_id2[sig2];
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
      up1 = row_cur[6];
      up2 = row_cur[7];
      if( (uint8_t)(up2[2].match[1]&up2[1].match[1]&up2[0].match[1]&up1[3].match[1]&up1[2].match[1]&up1[1].match[1]&up1[0].match[1]&up1[0].match[0]&(int8_t)up1[-1].match[1])&up2[3].match[1] ) {
        m_up0 = up2[0].match[0];
        idx1 = 1;
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
        bit = esc_ctr[(8*bucket+4*(uint8_t)(up2[run+3].match[1]&up2[run+2].match[1])+2*m_up0+*(alpha_map+nb_sym[0])+1)].decode_context_bit(esc_ctr);
        msym1c = mode_symbol[1];
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
          mask = 1<<(bucket&31);
          seen = 0;
          do {
            if( (mask|seen)<nb_sym[3] ) {
              ctr = &run_ctr[16*((seen==0)+(bucket==nb_sym[0]))+bucket];
              run0 = ctr->n[0];
              bin_tot = run0+ctr->n[1];
              nb_sym[1] = rc.decode_bit(run0, ctr->n[1]);
              if( ctr->limit<(uint32_t)bin_tot )
                ctr->scale_rare();
              s1b = nb_sym[1];
              ctr->n[nb_sym[1]] += 8;
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
          r8b = row_cur[8];
          r7b = row_cur[7]+idx_s;
          row_cur[7] = r7b-hit_a;
          row_cur[8] = r8b+idx_s-hit_a;
          rec = row_cur[5];
          row_cur[9] = row_cur[9]+idx_s-hit_a;
          rec->match2345 = 0x01010101;
          row_cur[5]->head4 = 0x01010101;
          pixp = pix_cur;
          nb_sym[1] = mode_symbol[1];
          pixp->prev = pixp->last;
          row_cur[5]->sym = (uint16_t)mode_symbol[1];
          pix_cur->last = (uint16_t)mode_symbol[1];
          flags_word = row_cur[5]->match2345;
          nb_sym[3] = row_cur[5]->head4;
          next = row_cur[5]+1;
          row_cur[5] = next;
          if( idx_s-hit_a!=1 ) {
            nb_sym[2] = (idx_s-hit_a-1)/2;
            if( nb_sym[2] ) {
              s3c = nb_sym[3];
              nb_sym[0] = hit_a;
              s1c = nb_sym[1];
              si = 0;
              idx_t = idx_s;
              s2a = nb_sym[2];
              do {
                pix_cur->prev = s1c;
                row_cur[5]->head4 = s3c;
                row_cur[5]->match2345 = flags_word;
                pixq = pix_cur;
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
              s3d = nb_sym[3];
              pix_cur->prev = nb_sym[1];
              row_cur[5]->head4 = s3d;
              row_cur[5]->match2345 = flags_word;
              next = row_cur[5]+1;
              row_cur[5] = next;
            }
          }
          idx_t = idx_s;
          cur6c = (PixRec*)row_cur[6];
          g_a = cur6c[-3].match[0];
          g_b = cur6c[-2].match[0];
          g_c = cur6c[2].match[0];
          g_d = cur6c[3].match[0];
          g_ab = g_b+g_a;
          g_e = cur6c[4].match[0];
          up3 = (PixRec*)row_cur[7];
          grad[0] = g_d+g_c+g_ab+g_e-5;
          grad[1] = up3[3].match[0]+up3[2].match[0]+up3[1].match[0]+up3[0].match[0]+(int8_t)up3[-1].match[0]+(int8_t)up3[-2].match[0]+(int8_t)up3[-3].match[0]+up3[4].match[0]-8;
          grad[2] = next[-4].match[1]+next[-3].match[1]-2;
          s1d = nb_sym[1];
          grad[3] = next[-5].match[0]+next[-6].match[0]+next[-7].match[0]+next[-4].match[0]-4;
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
    tot = freq_tbl->w[5];
    if( freq_tbl->w[5] ) {
      if( tot==1 ) {
        b15 = freq->b15;
        w2t = b15*freq_tbl->w[2];
        w3t = b15*freq_tbl->w[3];
        nb_sym[1] = b15*freq_tbl->w[0];
        w1s = b15*freq_tbl->w[1];
        nb_sym[2] = w2t;
        w4t = b15*freq_tbl->w[4];
        nb_sym[0] = w3t;
        nb_sym[3] = w4t;
        *freq_tbl = *freq;
        w5 = freq_tbl->w[5];
        w0r = freq_tbl->w[0];
        freq_tbl->b14 *= 8;
        w1r = 21*freq_tbl->w[1];
        nb_sym[1] += (21*w0r+w5-1)/w5;
        freq_tbl->w[0] = nb_sym[1];
        q1 = (w1r+w5-1)/w5;
        w2r = 21*freq_tbl->w[2];
        w3 = freq_tbl->w[3];
        q1w = q1+w1s;
        freq_tbl->w[1] = q1w;
        w3r = 21*w3;
        s0a = nb_sym[0];
        n2r = (w2r+w5-1)/w5+nb_sym[2];
        freq_tbl->w[2] = (uint16_t)n2r;
        span = (w3r+w5-1)/w5+s0a;
        w4r = 21*freq_tbl->w[4];
        freq_tbl->w[3] = span;
        n4r = (w4r+w5-1)/w5+nb_sym[3];
        freq_tbl->w[4] = n4r;
        tot2 = nb_sym[1]+n4r+(span+(uint16_t)n2r+q1w);
        tot = tot2;
        freq_tbl->w[5] = tot2;
      }
      arg_tot = tot;
      target = rc.get_freq(arg_tot);
      cum = freq_tbl->w[0];
      if( cum<=target ) {
        cum += freq_tbl->w[1];
        if( cum<=target ) {
          cum += freq_tbl->w[2];
          if( cum<=target ) {
            cum += freq_tbl->w[3];
            if( cum<=target ) {
              cum += freq_tbl->w[4];
              lvl_a = 4;
            } else {
              lvl_a = 3;
            }
          } else {
            lvl_a = 2;
          }
        } else {
          lvl_a = 1;
        }
      } else {
        lvl_a = 0;
      }
      w6a = freq_tbl->w[6];
      arg_high = cum;
      b15a = freq_tbl->b15;
      arg_cum = cum-freq_tbl->w[lvl_a];
      w5a = freq_tbl->w[5];
      if( w5a>w6a&&(freq_tbl->w[lvl_a]+b15a+8<w5a||freq_tbl->w[5]>0x4000u) ) {
        uint8_t kick = (uint8_t)b15a;
        w5a = freq_tbl->rescale((uint16_t)w6a, kick);
        b15a = kick;
      }
      freq_tbl->w[5] = b15a+w5a;
      freq_tbl->w[lvl_a] += b15a;
      rc.decode(arg_cum, arg_high, arg_tot);
      hit = lvl_a;
      if( freq_tbl->b14 ) {
        --freq_tbl->b14;
        freq2 = freq;
        ++freq->w[5];
        ++freq2->w[lvl_a];
        lvl_a = hit;
      }
    } else {
      arg_tot = freq->w[5];
      target2 = rc.get_freq(arg_tot);
      cum2 = freq->w[0];
      if( cum2<=target2 ) {
        cum2 += freq->w[1];
        if( cum2<=target2 ) {
          cum2 += freq->w[2];
          if( cum2<=target2 ) {
            cum2 += freq->w[3];
            if( cum2<=target2 ) {
              cum2 += freq->w[4];
              lvl_b = 4;
            } else {
              lvl_b = 3;
            }
          } else {
            lvl_b = 2;
          }
        } else {
          lvl_b = 1;
        }
      } else {
        lvl_b = 0;
      }
      b15b = freq->b15;
      arg_high = cum2;
      arg_cum = cum2-freq->w[lvl_b];
      w5b = freq->w[5];
      w6b = freq->w[6];
      nb_sym[3] = b15b;
      if( w5b>w6b&&(freq->w[lvl_b]+nb_sym[3]+8<w5b||w5b>0x4000) ) {
        nb_sym[0] = w6b;
        nb_sym[2] = lvl_b;
        k1 = freq->w[1];
        k2 = freq->w[2];
        k0 = freq->w[0]-(freq->w[0]>>1);
        freq->w[0] = k0;
        k1 = k1-(k1>>1);
        freq->w[1] = k1;
        k2h = k2-(k2>>1);
        k3 = (uint16_t)freq->w[3];
        freq->w[2] = k2h;
        k3 = k3-(k3>>1);
        k4 = (uint16_t)freq->w[4];
        freq->w[3] = k3;
        k4 = k4-(k4>>1);
        freq->w[4] = k4;
        k4 = k0+k4;
        w5b = (uint16_t)(k4+k3+k2h+k1);
        s0c = nb_sym[0];
        freq->w[5] = w5b;
        lvl_b = nb_sym[2];
        if( s0c<256&&!freq->b14 ) {
          s0c = 256;
          freq->w[6] = 256;
        }
        if( w5b>s0c ) {
          s3a = nb_sym[3];
          if( nb_sym[3]<15 )
            s3a = 15;
          nb_sym[3] = s3a;
          freq->b15 = s3a;
        }
      }
      s3b = nb_sym[3];
      freq->w[5] = nb_sym[3]+w5b;
      freq->w[lvl_b] += s3b;
      rc.decode(arg_cum, arg_high, arg_tot);
      hit = lvl_b;
      freq_tbl->w[5] = freq_tbl->w[lvl_b]++!=0;
      lvl_a = hit;
    }
    if( lvl_a ) {
      row_cur[5]->sym = mode_symbol[lvl_a];
      return 1;
    }
    idx_s = 0;
  pixel_done:
    gen = exclusion_gen;
    msym3 = mode_symbol[3];
    msym1 = mode_symbol[1];
    idx_t = idx_s;
    msym2 = mode_symbol[2];
    exclusion_mask[mode_symbol[4]] = exclusion_gen;
    pixr = pix_cur;
    exclusion_mask[msym3] = gen;
    exclusion_mask[msym2] = gen;
    exclusion_mask[msym1] = gen;
    exclusion_mask[no_symbol] = gen;
    sel[0] = nullptr;
    // The 32 candidate symbols `pixel_context` walks, in slot order.  Slots 0
    // and 1 are the two most recent symbols at this exact context, 2..9 the
    // move-to-front cache, and 10..31 the spatial neighbours widening outward.
    //
    // The decompilation interleaved the 32 stores with the 24 loads that feed
    // them and gave every load a name -- `h10` through `h31` -- because MSVC
    // scheduled them that way.  Every one was used exactly once.  Nothing here
    // writes `row_cur` or `sym_cache`, so the order is free.
    up4 = row_cur[6];
    r5 = row_cur[5];
    up5 = row_cur[7];
    r8c = row_cur[8];
    nb_sym[0] = pixr->last;
    nb_sym[1] = pixr->prev;
    for( int32_t c = 0; c<8; ++c )
      nb_sym[2+c] = sym_cache[c];
    nb_sym[10] = r5[-2].sym;
    nb_sym[11] = up4[2].sym;
    nb_sym[12] = up5[1].sym;
    nb_sym[13] = up5->sym;
    nb_sym[14] = up4[-2].sym;
    nb_sym[15] = up5[-1].sym;
    nb_sym[16] = r5[-3].sym;
    nb_sym[17] = up4[3].sym;
    nb_sym[18] = up4[4].sym;
    nb_sym[19] = r5[-4].sym;
    nb_sym[20] = up4[-3].sym;
    nb_sym[21] = up5[2].sym;
    nb_sym[22] = r8c->sym;
    nb_sym[23] = up5[-2].sym;
    nb_sym[24] = r5[-5].sym;
    nb_sym[25] = r8c[1].sym;
    nb_sym[26] = up4[5].sym;
    nb_sym[27] = row_cur[9]->sym;
    nb_sym[28] = r5[-7].sym;
    nb_sym[29] = r8c[-1].sym;
    nb_sym[30] = up4[7].sym;
    nb_sym[31] = up5[3].sym;
    sym_pos = 0;
    do {
      psym = pixel_context((uint32_t*)nb_sym);
      if( psym>=0 ) {
        bit2 = bit_node[ctr_node].decode_context_bit(&bit_root[ctr_fallback]);
        row_cur[5]->sym = psym;
        if( bit2 )
          return idx_t+1;
        exclusion_mask[psym] = exclusion_gen;
      }
      pos1 = sym_pos+1;
      sym_pos = pos1;
    } while( pos1<32 );
    msym1b = mode_symbol[1];
    sel1 = sel1_list;
    sel[0] = &sel0_list[mode_symbol[2]];
    sel_p = sel_cur;
    sel[1] = &sel1[msym1b];
    while( 1 ) {
      if( sel_p>&escape_list )
        bmf_fatal(bmf_read_error);
      if( (*sel_p)->live ) {
        lsym = decode_symbol_list(*sel_p);
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
    PixRec* r4;
    uint8_t* amap_at;
    PixRec* rowp;
    PixRec* row_cur9;
    bool up_eq_west;
    int8_t mode;
    int32_t arg_cum;
    FreqRec* frec;
    uint16_t* wr;
    PixRec* up4;
    uint16_t wp, g4, g3;
    SymList* sel1, ** sel_p;
    uint32_t arg_tot, arg_high, done, rec_word, grid_kind;
    PixRec* r1, * r2, * cur6b, * r3, * r5;
    FreqRec* binp, * binp_s;
    SymPair* pixp;
    uint8_t match1, m0, m1, m2, m3;
    int32_t bp;
    uint16_t* id3p;
    int16_t s8, w1s, acc2, s3, g0;
    int32_t up_sym, left_sym, up_next_sym, upleft_sym, nb, key, state, pair_last, cap, pair_prev, bucket_at, up_match0, m_w1, nb2, sig1, id1, sig2, id2, m_w1b, m_w0, m_up0, to_edge, one, run, run_pair, amap, runlen, run_hit, run_left, s1, s8b, s6, msym1, s4, bit5, first, s5, s1b, s9, cum1, lvl_a, w6a, w5a, b15a, msym3, excl_sym_a, excl_sym_b, w6d, wq1, esym, up_hit, pos1, msym1b, b15, w2t, w3t, w4t, acc, q1, w2s, q2, w3s, w4s, s9b, cum2, lvl_b, w5c, w6c, w5b, s3b;
    uint16_t w5, w1;
    PixRec* cur5, * cur5p1, * cur5p1b, * r6, * r7;
    SymPair* wq;
    uint16_t* cache;
    uint16_t w6;
    PixRec* cur2;
    BitCtr* ctr;
    uint32_t bin_tot, half, k, g2, g2h;
    PixRec* up3;
    PixRec* cur6c, * up2;
    PixRec* cur6 = (PixRec*)row_cur[6];
    up_sym = cur6->sym;
    cur5 = row_cur[5];
    left_sym = cur5[-1].sym;
    up_next_sym = cur6[1].sym;
    upleft_sym = cur6[-1].sym;
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
    up_eq_west = up_sym==left_sym;
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
    state = ctx_state[nb];
    ctx_state_seen = state;
    SymPair* pair = &group_ctr[state][key];
    pix_cur = pair;
    pair_last = pair->last;
    if( pair_last==nb_sym[8] ) {
      cap = 15;
    } else if( pair_last==nb_sym[10] ) {
      cap = 30;
    } else if( pair_last==nb_sym[6] ) {
      cap = 45;
    } else {
      up_eq_west = pair_last==nb_sym[5];
      cap = 60;
      if( !up_eq_west )
        cap = 0;
    }
    pair_prev = pair->prev;
    if( pair_prev==nb_sym[8] ) {
      cap += 75;
    } else if( pair_prev==nb_sym[10] ) {
      cap += 150;
    } else if( pair_prev==nb_sym[6] ) {
      cap += 225;
    } else if( pair_prev==nb_sym[5] ) {
      cap += 300;
    }
    r1 = row_cur[8];
    bucket_at = ctx_bucket[state+cap];
    r2 = row_cur[7];
    bucket_idx = bucket_at;
    up_match0 = cur6->match[0];
    match1 = cur6->match[1];
    cur2 = cur5;
    nb_sym[0] = up_match0;
    PixRec* up9 = row_cur[9];
    m_w1 = cur5[-1].match[1];
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
    id1 = ctx_id1[sig1];
    cur6b = cur6;
    if( id1==0xFFFF ) {
      ctx_id1[sig1] = ctx_id1_used;
      cur6b = row_cur[6];
      cur5 = row_cur[5];
      ++*(int32_t*)&ctx_id1_used;
      id1 = ctx_id1[sig1];
      nb_sym[0] = cur6b->match[0];
    }
    sig2 = CtxIdx{}.bit<2>(cur6b[1].match[3]).bit<1>(nb_sym[0])
                   .bit<0>(cur5[-1].match[5]).raw(8*id1);
    id2 = ctx_id2[sig2];
    if( id2==0xFFFF ) {
      ctx_id2[sig2] = (ctx_id2_used)++;
      id2 = ctx_id2[sig2];
    }
    if( *(int32_t*)&alphabet<32 ) {
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
        ++*(int32_t*)&ctx_id3_used;
        id2 = *id3p;
      }
    }
    PixRec* row = row_cur[5];
    m_w1b = row[-1].match[1];
    m_w0 = row[-1].match[0];
    rowp = row;
    if( (m_w1b&m_w0)!=0&&(cur6c = row_cur[6], up2 = row_cur[7], ((uint8_t)(up2[2].match[1]&up2[1].match[1]&up2[0].match[1]&cur6c[3].match[1]&cur6c[2].match[1]&cur6c[1].match[1]&cur6c[0].match[1]&cur6c[0].match[0]&cur6c[-1].match[1])&up2[3].match[1])!=0) ) {
      m_up0 = up2[0].match[0];
      to_edge = width-x;
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
      run_pair = (uint8_t)(up2[run+3].match[1]&up2[run+2].match[1]);
      nb_sym[5] = run_bucket[nb_sym[4]];
      amap = CtxIdx{}.bit<2>(run_pair).bit<1>(m_up0)
                     .raw(8*nb_sym[5]).raw(alpha_map[nb_sym[8]]);
      runlen = 0;
      if( rowp->sym==nb_sym[8] ) {
        do
          ++runlen;
        while( runlen<nb_sym[4]&&rowp[runlen].sym==nb_sym[8] );
      }
      run_hit = runlen==nb_sym[4];
      nb_sym[6] = run_hit;
      if( runlen>run_hit ) {
        row_cur[6] = &cur6c[runlen-run_hit];
        row_cur[7] = &up2[runlen-run_hit];
        row_cur9 = row_cur[9];
        row_cur[8] = row_cur[8]+runlen-run_hit;
        row_cur[9] = row_cur9+runlen-run_hit;
        rowp->match2345 = 0x01010101;
        row_cur[5]->head4 = 0x01010101;
        pix_cur->prev = pix_cur->last;
        wp = nb_sym[8];
        row_cur[5]->sym = nb_sym[8];
        pix_cur->last = wp;
        r3 = row_cur[5];
        rec_word = *(uint32_t*)r3;
        nb_sym[1] = r3->match2345;
        cur2 = r3+1;
        row_cur[5] = r3+1;
        if( runlen-run_hit!=1 ) {
          run_left = runlen-run_hit-1;
          half = run_left/2;
          if( run_left/2 ) {
            nb_sym[0] = run_left;
            s1 = nb_sym[1];
            runlen_s = runlen;
            k = 0;
            nb_sym[3] = amap;
            s8 = nb_sym[8];
            do {
              pix_cur->prev = s8;
              row_cur[5]->head4 = rec_word;
              row_cur[5]->match2345 = s1;
              pixp = pix_cur;
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
            cur5p1b = row_cur[5]+1;
            row_cur[5] = cur5p1b;
            cur2 = cur5p1b;
          }
        }
        r4 = (PixRec*)row_cur[6];
        m0 = r4[-2].match[0];
        m1 = r4[2].match[0];
        m2 = r4[4].match[0];
        nb_sym[3] = amap;
        m3 = r4[-3].match[0];
        runlen_s = runlen;
        up3 = (PixRec*)row_cur[7];
        grad[0] = r4[3].match[0]+m1+m0+m3+m2-5;
        grad[1] = up3[3].match[0]+up3[2].match[0]+up3[1].match[0]+up3[0].match[0]+(int8_t)up3[-1].match[0]+(int8_t)up3[-2].match[0]+(int8_t)up3[-3].match[0]+up3[4].match[0]-8;
        grad[2] = cur2[-4].match[1]+cur2[-3].match[1]-2;
        s8b = nb_sym[8];
        grad[3] = cur2[-5].match[0]+cur2[-6].match[0]+cur2[-7].match[0]+cur2[-4].match[0]-4;
        cur2[-1].match[4] = s8b==r4[1].sym;
        amap = nb_sym[3];
        row_cur[5][-1].match[5] = s8b==row_cur[6][2].sym;
        runlen = runlen_s;
      }
      (&esc_ctr[(amap+1)])->encode_context_bit(esc_ctr, nb_sym[6]);
      s6 = nb_sym[6];
      msym1 = mode_symbol[1];
      amap_at = (uint8_t*)alpha_map;
      hit = nb_sym[6];
      *(amap_at+msym1) = s6;
      if( !s6&&nb_sym[4]!=1 ) {
        runlen_s = runlen;
        nb_sym[0] = nb_sym[5];
        s4 = nb_sym[4];
        bit5 = 1<<(nb_sym[5]&31);
        first = 0;
        s5 = nb_sym[5];
        do {
          if( s4>(bit5|first) ) {
            nb_sym[3] = first;
            ctr = &run_ctr[16*((first==0)+(s5==nb_sym[0]))+s5];
            nb_sym[1] = runlen_s&bit5;
            bin_tot = ctr->n[0]+ctr->n[1];
            rc.encode_bit(ctr->n[0], ctr->n[1], (runlen_s&bit5)!=0);
            if( ctr->limit<(uint32_t)bin_tot )
              ctr->scale_rare();
            s1b = nb_sym[1];
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
      if( *(int32_t*)&hit )
        return runlen;
    } else {
      binp = &grid[bucket_idx];
      frec = &grid[id2+188];
      grid_kind = frec->w[5];
      if( grid_kind ) {
        if( grid_kind==1 ) {
          b15 = binp->b15;
          w2t = b15*frec->w[2];
          w3t = b15*frec->w[3];
          nb_sym[0] = b15*frec->w[0];
          w1s = b15*frec->w[1];
          nb_sym[1] = w2t;
          w4t = b15*frec->w[4];
          nb_sym[2] = w3t;
          nb_sym[3] = w4t;
          // `ip = (int32_t*)&grid[bucket_idx]` ten lines up, cast straight back
          // here and used as nothing else in between.
          *frec = grid[bucket_idx];
          w5 = frec->w[5];
          frec->b14 *= 8;
          acc = 21*frec->w[1];
          nb_sym[0] += (21*frec->w[0]+w5-1)/w5;
          frec->w[0] = nb_sym[0];
          q1 = (acc+w5-1)/w5;
          // `acc` is `21*frec->w[1]` until here, and `q1` above is the last
          // thing to read it whole.  From here down it is sixteen bits: MSVC
          // kept the register and wrote only its low half, and both of the
          // reads that follow are sixteen bits wide -- `frec->w[2]`, and
          // `acc2`, which is an `int16_t`.  So the half the partial store
          // preserved is dead, and this is the assignment it amounts to.
          // (Written the other way first, mask and all: 72 images produce the
          // same streams either way, which is a weaker statement than the
          // types are.)
          acc = (uint16_t)nb_sym[1];
          w2s = 21*frec->w[2];
          w6 = q1+w1s;
          frec->w[1] = w6;
          q2 = (w2s+w5-1)/w5;
          w3s = 21*frec->w[3];
          acc = (uint16_t)(q2+acc);
          frec->w[2] = acc;
          bp = nb_sym[2]+(w3s+w5-1)/w5;
          w4s = 21*frec->w[4];
          frec->w[3] = (uint16_t)bp;
          acc2 = (uint16_t)bp+acc+w6;
          w5 = (w4s+w5-1)/w5+nb_sym[3];
          frec->w[4] = w5;
          grid_kind = (uint16_t)(nb_sym[0]+w5+acc2);
          frec->w[5] = grid_kind;
          rowp = row_cur[5];
        }
        s9 = rowp->sym;
        arg_tot = grid_kind;
        if( s9==nb_sym[8] ) {
          cum1 = frec->w[0];
          lvl_a = 1;
        } else if( s9==nb_sym[10] ) {
          cum1 = frec->w[0]+frec->w[1];
          lvl_a = 2;
        } else if( s9==nb_sym[6] ) {
          cum1 = frec->w[0]+frec->w[2]+frec->w[1];
          lvl_a = 3;
        } else if( s9==nb_sym[5] ) {
          cum1 = frec->w[5]-frec->w[4];
          lvl_a = 4;
        } else {
          cum1 = 0;
          lvl_a = 0;
        }
        w6a = frec->w[6];
        arg_cum = cum1;
        arg_high = frec->w[lvl_a]+cum1;
        w5a = frec->w[5];
        b15a = frec->b15;
        if( w5a>w6a&&(frec->w[lvl_a]+b15a+8<w5a||frec->w[5]>0x4000u) ) {
          uint8_t kick = (uint8_t)b15a;
          w5a = frec->rescale((uint16_t)w6a, kick);
          b15a = kick;
        }
        frec->w[5] = b15a+w5a;
        frec->w[lvl_a] += b15a;
        rc.encode(arg_cum, arg_high, arg_tot);
        hit = lvl_a;
        if( frec->b14 ) {
          --frec->b14;
          binp_s = binp;
          ++binp->w[5];
          ++binp_s->w[lvl_a];
          lvl_a = hit;
        }
      } else {
        // `w` is the union member this was casting to reach.
        wr = grid[bucket_idx].w;
        s9b = rowp->sym;
        arg_tot = binp->w[5];
        if( s9b==nb_sym[8] ) {
          cum2 = *wr;
          lvl_b = 1;
        } else if( s9b==nb_sym[10] ) {
          cum2 = *wr+wr[1];
          lvl_b = 2;
        } else if( s9b==nb_sym[6] ) {
          cum2 = *wr+wr[2]+wr[1];
          lvl_b = 3;
        } else if( s9b==nb_sym[5] ) {
          cum2 = wr[5]-wr[4];
          lvl_b = 4;
        } else {
          cum2 = 0;
          lvl_b = 0;
        }
        arg_cum = cum2;
        arg_high = binp->w[lvl_b]+cum2;
        w5c = binp->w[5];
        w6c = binp->w[6];
        nb_sym[3] = binp->b15;
        if( w5c>w6c&&(binp->w[lvl_b]+nb_sym[3]+8<w5c||w5c>0x4000) ) {
          nb_sym[0] = w6c;
          w1 = binp->w[1];
          g2 = binp->w[2];
          nb_sym[2] = lvl_b;
          g0 = binp->w[0]-(binp->w[0]>>1);
          binp->w[0] = g0;
          w1 = w1-(w1>>1);
          binp->w[1] = w1;
          g2h = g2-(g2>>1);
          g3 = (uint16_t)binp->w[3];
          binp->w[2] = g2h;
          g3 = g3-(g3>>1);
          g4 = (uint16_t)binp->w[4];
          binp->w[3] = g3;
          g4 = g4-(g4>>1);
          binp->w[4] = g4;
          g4 = g0+g4;
          lvl_b = nb_sym[2];
          w5c = (uint16_t)(g4+g3+g2h+w1);
          w5b = nb_sym[0];
          binp->w[5] = w5c;
          if( w5b<256&&!binp->b14 ) {
            w5b = 256;
            binp->w[6] = 256;
          }
          if( w5c>w5b ) {
            s3b = nb_sym[3];
            if( nb_sym[3]<15 )
              s3b = 15;
            nb_sym[3] = s3b;
            binp->b15 = s3b;
          }
        }
        s3 = nb_sym[3];
        binp->w[5] = nb_sym[3]+w5c;
        binp->w[lvl_b] += s3;
        rc.encode(arg_cum, arg_high, arg_tot);
        hit = lvl_b;
        frec->w[5] = (frec->w[lvl_b])++!=0;
        lvl_a = hit;
      }
      if( lvl_a )
        return 1;
      runlen = 0;
    }
    mode = exclusion_gen;
    msym3 = mode_symbol[3];
    excl_sym_a = mode_symbol[2];
    excl_sym_b = mode_symbol[1];
    exclusion_mask[mode_symbol[4]] = exclusion_gen;
    exclusion_mask[msym3] = mode;
    wq = pix_cur;
    exclusion_mask[excl_sym_a] = mode;
    exclusion_mask[excl_sym_b] = mode;
    cache = sym_cache;
    exclusion_mask[no_symbol] = mode;
    runlen_s = runlen;
    sel[0] = nullptr;
    w6d = wq->last;
    wq1 = wq->prev;
    // The same 32 candidates as `decode_pixel`'s, in slot order: 0 and 1 the
    // two most recent symbols at this context, 2..9 the move-to-front cache,
    // 10..31 the spatial neighbours widening outward.  The decompilation
    // interleaved the stores with the loads and named every load.
    r5 = row_cur[5];
    up4 = row_cur[6];
    r6 = row_cur[7];
    r7 = row_cur[8];
    nb_sym[0] = w6d;
    nb_sym[1] = wq1;
    for( int32_t c = 0; c<8; ++c )
      nb_sym[2+c] = cache[c];
    nb_sym[10] = r5[-2].sym;
    nb_sym[11] = up4[2].sym;
    nb_sym[12] = r6[1].sym;
    nb_sym[13] = r6->sym;
    nb_sym[14] = up4[-2].sym;
    nb_sym[15] = r6[-1].sym;
    nb_sym[16] = r5[-3].sym;
    nb_sym[17] = up4[3].sym;
    nb_sym[18] = up4[4].sym;
    nb_sym[19] = r5[-4].sym;
    nb_sym[20] = up4[-3].sym;
    nb_sym[21] = r6[2].sym;
    nb_sym[22] = r7->sym;
    nb_sym[23] = r6[-2].sym;
    nb_sym[24] = r5[-5].sym;
    nb_sym[25] = r7[1].sym;
    nb_sym[26] = up4[5].sym;
    nb_sym[27] = row_cur[9]->sym;
    nb_sym[28] = r5[-7].sym;
    nb_sym[29] = r7[-1].sym;
    nb_sym[30] = up4[7].sym;
    nb_sym[31] = r6[3].sym;
    sym_pos = 0;
    do {
      esym = pixel_context((uint32_t*)nb_sym);
      if( esym>=0 ) {
        up_hit = esym==row_cur[5]->sym;
        (&bit_node[*(int32_t*)&ctr_node])->encode_context_bit(&bit_root[*(int32_t*)&ctr_fallback], up_hit);
        if( up_hit )
          return runlen_s+1;
        exclusion_mask[esym] = exclusion_gen;
      }
      pos1 = sym_pos+1;
      sym_pos = pos1;
    } while( pos1<32 );
    msym1b = mode_symbol[1];
    sel1 = sel1_list;
    sel[0] = &sel0_list[mode_symbol[2]];
    sel_p = sel_cur;
    sel[1] = &sel1[msym1b];
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
