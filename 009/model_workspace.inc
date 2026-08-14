// model_workspace.inc -- the main model's workspace: how it is laid out and how it is given
// back.

void**ModelBlock::free_workspace(int8_t do_free) {
  uint32_t n, left;
  int32_t i;
  SymList* lists, * p;
  free(sym_code);
  free(sym_word);
  free(run_bucket);
  free(*(void**)&alpha_map);
  lists = sel1_list;
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
  lists = sel0_list;
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
    free(row_cur[i]);
  free(escape.ent);
  if( do_free&1 )
    free(this);
  return (void**)this;
}

ModelBlock*ModelBlock::layout_workspace(int32_t unread_flag, int32_t img_w, int32_t img_h, int32_t img_depth) {
  uint32_t w;
  int32_t j;
  uint32_t k, m, s, n;
  w = img_w;
  exclusion_gen = 1;
  width = img_w;
  height = img_h;
  depth = img_depth;
  depth_raw = img_depth;
  escape.ent = nullptr;
  sym_code = nullptr;
  for( j = 0; j<5; ++j ) {
    PixRec* buf = (PixRec*)bmf_new(sizeof(PixRec)*(w+16));
    row_cur[j] = buf;
    row_cur[j+5] = buf+8;
    w = width;
    if( (int32_t)width>-16 ) {
      int32_t r = 0;
      do {
        row_cur[j][r].sym = 0;
        row_cur[j][r].match[5] = 1;
        row_cur[j][r].match[4] = 1;
        row_cur[j][r].match[3] = 1;
        row_cur[j][r].match[2] = 1;
        row_cur[j][r].match[1] = 1;
        row_cur[j][r].match[0] = 1;
        w = width;
        ++r;
      } while( (uint32_t)r<width+16 );
    }
  }
  uint8_t* runs = (uint8_t*)bmf_new(w+1);
  run_bucket = runs;
  *runs = 0;
  if( (int32_t)width>0 ) {
    uint8_t bucket = 0;
    int32_t x = 0;
    do {
      bucket += x==2<<(bucket&31);
      run_bucket[x+++1] = bucket;
    } while( (uint32_t)x<width );
  }
  __builtin_memset(sym_rev, 0, sizeof sym_rev);
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
  memset(ctx_id1, 255, sizeof ctx_id1);
  memset(ctx_id2, 255, sizeof ctx_id2);
  memset(ctx_id3, 255, sizeof ctx_id3);
  memset(exclusion_mask, 0, 8193);
  sel[0] = nullptr;
  sel[1] = nullptr;
  escape_list = nullptr;
  for( n = 0; n<0x40000; ++n ) {
    sym_ctr[2*n] = 0x2000;
    sym_ctr[2*n+1] = 0x2000;
  }
  uint32_t i8 = 0;
  do {
    uint32_t e0 = 2*i8;
    ++i8;
    bit_root[e0].init_root();
    bit_root[e0+1].init_root();
  } while( i8<8 );
  uint32_t i24 = 0;
  memset(bit_node, 0, sizeof bit_node);
  sym_word = (uint16_t*)bmf_new(2*height*width);
  esc_ctr[0].init_parent();
  memset(&esc_ctr[1], 0, 1536);
  do {
    uint32_t e1 = 2*i24;
    ++i24;
    run_ctr[e1].init_parent();
    run_ctr[e1+1].init_parent();
  } while( i24<0x18 );
  return this;
}
