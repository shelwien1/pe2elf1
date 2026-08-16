// model_plane.inc -- one plane through the main model.

void model_plane(BmfImage* p_i, uint8_t* pixels, uint8_t* raw) {
  int16_t w2n;
  uint32_t row_w;
  int32_t w3, w4, y;
  uint16_t w2;
  if( plane_alt_model ) {
    if( plane_predictor==pred_p1 ) {
      if( (p_i->depth&depth_bits)==8 )
        alt_model_p1_d8_encode(pixels, p_i->width, p_i->height, raw);
      else
        alt_model_p1_encode(p_i, pixels);
    } else if( plane_predictor==pred_p2 ) {
      if( (p_i->depth&depth_bits)==8 )
        alt_model_p2_d8_encode(pixels, p_i->width, p_i->height, raw);
      else
        alt_model_p2_encode(p_i, pixels);
    }
  } else {
    ModelBlock* blk = new_model_block(p_i->width, p_i->height,
                                     p_i->depth&depth_bits);
    rc_begin_encode();
    reduce_alphabet(blk, pixels);
    int32_t bucket = 0;
    int32_t g = 0;
    do {
      int32_t flags = ctx_group_flags[g];
      blk->ctx_state[flags] = g;
      int32_t lo = 0;
      // One of fifteen groups says which of the five selector weights share a
      // weight; `tables.inc` names the bits and gives the order the folds
      // compose in.  Hoisted out of the two loops below, as the original has
      // them.
      int32_t f_w3_double = flags&ctx_w3_double;
      int32_t f_w4_to_w2 = flags&ctx_w4_to_w2;
      int32_t f_w3_to_w2 = flags&ctx_w3_to_w2;
      int32_t f_w4_to_w1 = flags&ctx_w4_to_w1;
      int32_t f_w3_to_w1 = flags&ctx_w3_to_w1;
      int32_t f_w2_to_w1 = flags&ctx_w2_to_w1;
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
      SymPair* group_ctr = blk->group_ctr[g];
      do {
        group_ctr[gi].last = 0x2000;
        group_ctr[gi++].prev = 0x2000;
      } while( gi<0x10000 );
      ++g;
    } while( (uint32_t)g<0xF );
    blk->seed_alphabet();
    if( blk->height>0 ) {
      uint16_t* word = blk->sym_word;
      y = 0;
      do {
        int32_t y1 = y+1;
        blk->start_row();
        PixRec *up1 = blk->row_cur[6];
        PixRec *up2 = blk->row_cur[7];
        ++blk->row_cur[5];
        ++up1;
        blk->row_cur[6] = up1;
        ++blk->row_cur[8];
        ++up2;
        blk->row_cur[7] = up2;
        ++blk->row_cur[9];
        blk->grad[0] = match_seed(up1, 5);
        blk->grad[3] = 0;
        blk->grad[2] = 0;
        y = y1;
        row_w = blk->width;
        blk->grad[1] = match_seed(up2, 5);
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
    rc_end_encode();
    blk->free_workspace(1);
  }
}
