// workspace.inc -- the main model's workspace: how it is laid out and how it is given
// back.

void**__free_workspace(ModelBlock* blk, int8_t do_free) {
  uint32_t n, left;
  int32_t i;
  SymList* lists, * p;
  free(blk->sym_code);
  free(blk->sym_word);
  free(blk->run_bucket);
  free(*(void**)&blk->alpha_map);
  lists = blk->sel1_list;
  if( lists ) {
    n = sym_list_count(lists);
    if( n ) {
      left = n;
      p = &lists[n];
      do {
        --p;
        free(p->ent);
        --left;
      } while( left );
    }
    free(sym_list_block(lists));
  }
  lists = blk->sel0_list;
  if( lists ) {
    n = sym_list_count(lists);
    if( n ) {
      left = n;
      p = &lists[n];
      do {
        --p;
        free(p->ent);
        --left;
      } while( left );
    }
    free(sym_list_block(lists));
  }
  for( i = 0; i<5; ++i )
    free(blk->row_cur[i]);
  free(blk->escape.ent);
  if( (do_free&1)!=0 )
    free(blk);
  return (void**)blk;
}

ModelBlock*__layout_workspace(ModelBlock* blk, int32_t unread_flag, int32_t img_w, int32_t img_h, int32_t img_depth) {
  PixRec* buf;
  uint8_t bucket;
  uint16_t rev;
  uint32_t e0, e1, w;
  uint8_t* runs;
  int32_t j, r, x, bits;
  uint32_t k, m, s, n, i8, i24;
  w = img_w;
  exclusion_gen = 1;
  blk->width = img_w;
  blk->height = img_h;
  blk->depth = img_depth;
  blk->depth_raw = img_depth;
  blk->escape.ent = nullptr;
  blk->sym_code = nullptr;
  for( j = 0; j<5; ++j ) {
    buf = (PixRec*)bmf_new(sizeof(PixRec)*(w+16));
    blk->row_cur[j] = buf;
    blk->row_cur[j+5] = buf+8;
    w = blk->width;
    if( (int32_t)blk->width>-16 ) {
      r = 0;
      do {
        blk->row_cur[j][r].sym = 0;
        blk->row_cur[j][r].match[5] = 1;
        blk->row_cur[j][r].match[4] = 1;
        blk->row_cur[j][r].match[3] = 1;
        blk->row_cur[j][r].match[2] = 1;
        blk->row_cur[j][r].match[1] = 1;
        blk->row_cur[j][r].match[0] = 1;
        w = blk->width;
        ++r;
      } while( (uint32_t)r<blk->width+16 );
    }
  }
  runs = (uint8_t*)bmf_new(w+1);
  blk->run_bucket = runs;
  *runs = 0;
  if( (int32_t)blk->width>0 ) {
    bucket = 0;
    x = 0;
    do {
      bucket += x==2<<(bucket&31);
      blk->run_bucket[x+++1] = bucket;
    } while( (uint32_t)x<blk->width );
  }
  __builtin_memset(blk->sym_rev, 0, sizeof blk->sym_rev);
  for( k = 0; k<0x2000; ++k ) {
    rev = blk->sym_rev[k];
    bits = k;
    for( m = 0; m<0xD; ++m ) {
      rev += rev+(bits&1);
      bits >>= 1;
    }
    blk->sym_rev[k] = rev;
  }
  for( s = 0; s<0x2000; ++s )
    blk->sym_rev[s] *= 8;
  memset(&blk->grid[188], 0, 0x100000);
  blk->ctx_id3_used = 0;
  blk->ctx_id2_used = 0;
  blk->ctx_id1_used = 0;
  memset(blk->ctx_id1, 255, sizeof blk->ctx_id1);
  memset(blk->ctx_id2, 255, sizeof blk->ctx_id2);
  memset(blk->ctx_id3, 255, sizeof blk->ctx_id3);
  memset(exclusion_mask, 0, 8193);
  blk->sel[0] = nullptr;
  blk->sel[1] = nullptr;
  blk->escape_list = nullptr;
  for( n = 0; n<0x40000; ++n ) {
    blk->sym_ctr[2*n] = 0x2000;
    blk->sym_ctr[2*n+1] = 0x2000;
  }
  i8 = 0;
  do {
    e0 = 2*i8;
    ++i8;
    blk->bit_root[e0].init_root();
    blk->bit_root[e0+1].init_root();
  } while( i8<8 );
  i24 = 0;
  memset(blk->bit_node, 0, sizeof blk->bit_node);
  blk->sym_word = (uint16_t*)bmf_new(2*blk->height*blk->width);
  blk->esc_ctr[0].init_parent();
  memset(&blk->esc_ctr[1], 0, 1536);
  do {
    e1 = 2*i24;
    ++i24;
    blk->run_ctr[e1].init_parent();
    blk->run_ctr[e1+1].init_parent();
  } while( i24<0x18 );
  return blk;
}
