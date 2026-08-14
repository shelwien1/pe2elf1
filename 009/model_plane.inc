// model_plane.inc -- one plane through the main model.

void __model_plane(BmfImage* p_i, uint8_t* pixels, uint8_t* raw) {
  ModelBlock* blk;
  int16_t w2n;
  uint32_t row_w;
  int32_t w3, w4, lvl2, live, y;
  uint16_t w2;
  uint32_t j, k;
  SymList* lists1, * lists0;
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
      blk = ((ModelBlock*)ws)->layout_workspace(p_i->height, p_i->width, p_i->height, p_i->depth&0x3F);
    else
      blk = (ModelBlock*)(0);
    __rc_begin_encode();
    __reduce_alphabet(blk, 0, pixels);
    int32_t bucket = 0;
    int32_t g = 0;
    do {
      int32_t flags = ctx_group_flags[g];
      blk->ctx_state[flags] = g;
      int32_t lo = 0;
      // The five selector weights are the five outcomes of the stage-one
      // decision -- north, west, north-east, north-west, escape -- and one of
      // fifteen groups says which of them share a weight.  Each bit of the
      // group's mask folds one weight into another and zeroes the source, so
      // the six below are read in the order the folds compose: 3 doubles first,
      // then 4 into 2, then 3 into 2, then 4 into 1, then 3 into 1, then 2 into
      // 1.  A group with no bits set gives all five their own weight.
      int32_t f_w3_double = flags&4;
      int32_t f_w4_to_w2 = flags&2;
      int32_t f_w3_to_w2 = flags&0x10;
      int32_t f_w4_to_w1 = flags&1;
      int32_t f_w3_to_w1 = flags&8;
      int32_t f_w2_to_w1 = flags&0x20;
      do {
        uint32_t hi = 0;
        do {
          blk->ctx_bucket[g+15*lo+75*hi] = bucket;
          int32_t alpha = blk->alphabet;
          FreqRec* rec = &blk->grid[bucket];
          w2n = 2;
          rec->w[0] = 2;
          w2 = 2;
          w3 = 2;
          w4 = 2;
          if( f_w3_double ) {
            w3 = 4;
            w4 = 0;
          }
          if( f_w4_to_w2 ) {
            w2 = w4+2;
            w4 = 0;
          }
          if( f_w3_to_w2 ) {
            w2 = w3+w2;
            w3 = 0;
          }
          if( f_w4_to_w1 ) {
            w2n = w4+2;
            w4 = 0;
            rec->w[4] = 0;
          } else {
            rec->w[4] = w4;
          }
          if( f_w3_to_w1 ) {
            w2n += w3;
            w3 = 0;
            rec->w[3] = 0;
          } else {
            rec->w[3] = w3;
          }
          if( f_w2_to_w1 ) {
            rec->w[1] = w2+w2n;
            w2 = 0;
            rec->w[2] = 0;
          } else {
            rec->w[2] = w2;
            rec->w[1] = w2n;
          }
          int32_t has3 = w3!=0;
          int32_t has4 = w4!=0;
          w2 = w2;
          if( w2 )
            w2 = 1;
          int32_t lvl = has4+has3+w2+2;
          if( lvl<=alpha ) {
            rec->b14 = lvl;
          } else {
            rec->b14 = has4+has3+w2+1;
            rec->w[0] = 0;
          }
          if( rec->w[lo]&&rec->w[hi]&&(lvl2 = rec->b14, lvl2<=alpha) ) {
            live = 1;
            int16_t wt = (uint8_t)(1<<((5-lvl2)&31));
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
      uint32_t gi = 0;
      SymPair* group_ctr = blk->group_ctr[g];
      do {
        group_ctr[gi].last = 0x2000;
        group_ctr[gi++].prev = 0x2000;
      } while( gi<0x10000 );
      ++g;
    } while( (uint32_t)g<0xF );
    uint8_t* buf = (uint8_t*)bmf_new(blk->alphabet);
    uint32_t alpha_n = blk->alphabet;
    blk->alpha_map = buf;
    memset(buf, 1, alpha_n);
    blk->escape_list = &blk->escape;
    blk->escape.init(blk->alphabet, 1);
    blk->sel_cur = blk->sel;
    uint32_t n_syms = blk->alphabet;
    SymListBlock *blk1 = (SymListBlock*)bmf_new(SymListBlock::bytes(n_syms));
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
    uint32_t n_syms2 = blk->alphabet;
    blk->sel1_list = lists1;
    SymListBlock *blk0 = (SymListBlock*)bmf_new(SymListBlock::bytes(n_syms2));
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
      int32_t s = 0;
      do {
        blk->sel1_list[s].init(99, 0);
        blk->sel0_list[s++].init(33, 0);
      } while( (uint32_t)s<blk->alphabet );
    }
    if( blk->height>0 ) {
      uint16_t* word = blk->sym_word;
      y = 0;
      do {
        int32_t y1 = y+1;
        blk->row_cur[5]->match[1] = blk->row_cur[5][-1].sym==0;
        blk->row_cur[5]->match[3] = blk->row_cur[6][-1].sym==0;
        PixRec *r4 = blk->row_cur[4];
        PixRec *row_cur3 = blk->row_cur[3];
        PixRec *row_cur2 = blk->row_cur[2];
        PixRec *row_cur1 = blk->row_cur[1];
        PixRec *r0 = blk->row_cur[0];
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
        PixRec *up1 = blk->row_cur[6];
        PixRec *up2 = blk->row_cur[7];
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
          int32_t x = 0;
          do {
            ++x;
            blk->row_cur[5][x-1].sym = word[x-1];
            row_w = blk->width;
          } while( (uint32_t)x<blk->width );
          word += x;
        }
        if( row_w>0 ) {
          int32_t x2 = 0;
          do {
            int32_t step = (blk)->code_pixel(x2);
            blk->init_tables();
            x2 += step;
          } while( (uint32_t)x2<blk->width );
          y = y1;
        }
      } while( (uint32_t)y<*(uint32_t*)&blk->height );
    }
    __rc_end_encode();
    blk->free_workspace(1);
  }
}
