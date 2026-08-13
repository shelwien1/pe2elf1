// model_plane.inc -- one plane through the main model.

void __model_plane(BmfImage* p_i, uint8_t* pixels, uint8_t* raw) {
  uint32_t alpha_n, hi;
  int32_t y1, bucket, f_b3, f_b0, f_b4, f_b1, f_b2, f_b5, alpha;
  ModelBlock* blk;
  PixRec* r4, * r0;
  int16_t w2n, wt;
  uint32_t row_w;
  int32_t g, flags, lo, w3, w4, has3, has4, lvl, lvl2, live, s, y, x, x2, step;
  uint16_t w2;
  PixRec* row_cur3, * row_cur2, * row_cur1;
  uint32_t gi, n_syms, j, n_syms2, k;
  SymListBlock* blk1, * blk0;
  SymList* lists1, * lists0;
  PixRec* up1, * up2;
  if( plane_alt_model ) {
    if( plane_predictor==1 ) {
      if( (p_i->depth&0x3F)==8 )
        __alt_model_p1_d8_encode(pixels, p_i->width, p_i->height, raw);
      else
        __alt_model_p1_encode((uint16_t*)p_i, pixels);
    } else if( plane_predictor==2 ) {
      if( (p_i->depth&0x3F)==8 )
        __alt_model_p2_d8_encode(pixels, p_i->width, p_i->height, raw);
      else
        __alt_model_p2_encode(p_i, pixels);
    }
  } else {
    void* ws = bmf_new(sizeof(ModelBlock));
    if( ws )
      blk = __layout_workspace((ModelBlock*)ws, p_i->height, p_i->width, p_i->height, p_i->depth&0x3F);
    else
      blk = (ModelBlock*)(0);
    __rc_begin_encode();
    __reduce_alphabet(blk, 0, pixels);
    bucket = 0;
    g = 0;
    do {
      flags = ctx_group_flags[g];
      blk->ctx_state[flags] = g;
      lo = 0;
      f_b2 = flags&4;
      f_b1 = flags&2;
      f_b4 = flags&0x10;
      f_b0 = flags&1;
      f_b3 = flags&8;
      f_b5 = flags&0x20;
      do {
        hi = 0;
        do {
          blk->ctx_bucket[g+15*lo+75*hi] = bucket;
          alpha = blk->alphabet;
          FreqRec* rec = &blk->grid[bucket];
          w2n = 2;
          rec->w[0] = 2;
          w2 = 2;
          w3 = 2;
          w4 = 2;
          if( f_b2 ) {
            w3 = 4;
            w4 = 0;
          }
          if( f_b1 ) {
            w2 = w4+2;
            w4 = 0;
          }
          if( f_b4 ) {
            w2 = w3+w2;
            w3 = 0;
          }
          if( f_b0 ) {
            w2n = w4+2;
            w4 = 0;
            rec->w[4] = 0;
          } else {
            rec->w[4] = w4;
          }
          if( f_b3 ) {
            w2n += w3;
            w3 = 0;
            rec->w[3] = 0;
          } else {
            rec->w[3] = w3;
          }
          if( f_b5 ) {
            rec->w[1] = w2+w2n;
            w2 = 0;
            rec->w[2] = 0;
          } else {
            rec->w[2] = w2;
            rec->w[1] = w2n;
          }
          has3 = w3!=0;
          has4 = w4!=0;
          w2 = w2;
          if( w2 )
            w2 = 1;
          lvl = has4+has3+w2+2;
          if( lvl<=alpha ) {
            rec->b14 = lvl;
          } else {
            rec->b14 = has4+has3+w2+1;
            rec->w[0] = 0;
          }
          if( rec->w[lo]&&rec->w[hi]&&(lvl2 = rec->b14, lvl2<=alpha) ) {
            live = 1;
            wt = (uint8_t)(1<<((5-lvl2)&31));
            rec->b15 = wt;
            rec->w[6] = wt<<6;
            rec->w[lo] += wt;
            rec->w[hi] += rec->b15;
            rec->w[5] = rec->w[0]+rec->w[4]+rec->w[3]+rec->w[2]+rec->w[1];
          } else {
            live = 0;
          }
          bucket += live;
          ++hi;
        } while( hi<5 );
        ++lo;
      } while( (uint32_t)lo<5 );
      gi = 0;
      SymPair* group_ctr = blk->group_ctr[g];
      do {
        group_ctr[gi].last = 0x2000;
        group_ctr[gi++].prev = 0x2000;
      } while( gi<0x10000 );
      ++g;
    } while( (uint32_t)g<0xF );
    uint8_t* buf = (uint8_t*)bmf_new(blk->alphabet);
    alpha_n = blk->alphabet;
    blk->alpha_map = buf;
    memset(buf, 1, alpha_n);
    blk->escape_list = &blk->escape;
    __init_symbol_list(&blk->escape, blk->alphabet, 1);
    blk->sel_cur = blk->sel;
    n_syms = blk->alphabet;
    blk1 = (SymListBlock*)bmf_new(SymListBlock::bytes(n_syms));
    if( blk1 ) {
      blk1->n = n_syms;
      lists1 = blk1->list;
      if( n_syms ) {
        for( j = 0; j<n_syms; ++j )
          lists1[j].ent = nullptr;
      }
    } else {
      lists1 = nullptr;
    }
    n_syms2 = blk->alphabet;
    blk->sel1_list = lists1;
    blk0 = (SymListBlock*)bmf_new(SymListBlock::bytes(n_syms2));
    if( blk0 ) {
      blk0->n = n_syms2;
      lists0 = blk0->list;
      if( n_syms2 ) {
        for( k = 0; k<n_syms2; ++k )
          lists0[k].ent = nullptr;
      }
    } else {
      lists0 = nullptr;
    }
    blk->sel0_list = lists0;
    if( !(blk->alphabet<=0) ) {
      s = 0;
      do {
        __init_symbol_list(&blk->sel1_list[s], 99, 0);
        __init_symbol_list(&blk->sel0_list[s++], 33, 0);
      } while( (uint32_t)s<blk->alphabet );
    }
    if( blk->height>0 ) {
      uint16_t* word = blk->sym_word;
      y = 0;
      do {
        y1 = y+1;
        blk->row_cur[5]->match[1] = blk->row_cur[5][-1].sym==0;
        blk->row_cur[5]->match[3] = blk->row_cur[6][-1].sym==0;
        r4 = blk->row_cur[4];
        row_cur3 = blk->row_cur[3];
        row_cur2 = blk->row_cur[2];
        row_cur1 = blk->row_cur[1];
        r0 = blk->row_cur[0];
        blk->row_cur[4] = row_cur3;
        blk->row_cur[3] = row_cur2;
        blk->row_cur[2] = row_cur1;
        blk->row_cur[1] = r0;
        blk->row_cur[0] = r4;
        r4 += 7;
        blk->row_cur[5] = r4;
        r0 += 7;
        blk->row_cur[6] = r0;
        blk->row_cur[7] = row_cur1+7;
        blk->row_cur[8] = row_cur2+7;
        blk->row_cur[9] = row_cur3+7;
        {
          uint8_t zero = r0[1].sym==0;
          r4->match[2] = zero;
          blk->row_cur[5][-1].match[4] = zero;
          blk->row_cur[5][-2].match[5] = zero;
          zero = blk->row_cur[6][2].sym==0;
          blk->row_cur[5]->match[4] = zero;
          blk->row_cur[5][-1].match[5] = zero;
        }
        blk->row_cur[5]->match[5] = blk->row_cur[6][3].sym==0;
        up1 = blk->row_cur[6];
        up2 = blk->row_cur[7];
        ++blk->row_cur[5];
        ++up1;
        blk->row_cur[6] = up1;
        ++blk->row_cur[8];
        ++up2;
        blk->row_cur[7] = up2;
        ++blk->row_cur[9];
        blk->grad[0] = up1[3].match[0]+up1[2].match[0]+up1[1].match[0]+up1[0].match[0]+up1[4].match[0]-5;
        blk->grad[3] = 0;
        blk->grad[2] = 0;
        y = y1;
        row_w = blk->width;
        blk->grad[1] = up2[3].match[0]+up2[2].match[0]+up2[1].match[0]+up2[0].match[0]+up2[4].match[0]-5;
        if( row_w>0 ) {
          x = 0;
          do {
            ++x;
            blk->row_cur[5][x-1].sym = word[x-1];
            row_w = blk->width;
          } while( (uint32_t)x<blk->width );
          word += x;
        }
        if( row_w>0 ) {
          x2 = 0;
          do {
            step = (blk)->code_pixel(x2);
            blk->init_tables();
            x2 += step;
          } while( (uint32_t)x2<blk->width );
          y = y1;
        }
      } while( (uint32_t)y<*(uint32_t*)&blk->height );
    }
    __rc_end_encode();
    __free_workspace(blk, 1);
  }
}
