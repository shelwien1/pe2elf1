// model_workspace.inc -- the main model's workspace: how it is laid out and how it is given
// back.

// One bank of symbol lists, and the block behind it.  Written out twice, once
// for each of the two banks, ten lines apart.
static void free_sym_lists(SymList* lists) {
  if( !lists )
    return;
  uint32_t n = sym_list_count(lists);
  for( SymList* p = &lists[n]; p>lists; )
    free((--p)->ent);
  free(sym_list_block(lists));
}

// Returns nothing.  It returned `(void**)this` -- after `free(this)` on the
// path that takes it, so the value was a dangling pointer -- and neither of the
// two callers reads it.  Third of its kind in this tree: `alt_p1_free` and
// `alt_model_p1_d8_decode` were the first two, and the decompiler writes the
// register that happened to hold `this` as a return value wherever MSVC left
// it there.
void ModelBlock::free_workspace(int8_t do_free) {
  int32_t i;
  free(sym_code);
  free(sym_word);
  free(run_bucket);
  free(alpha_map);
  free_sym_lists(sel1_list);
  free_sym_lists(sel0_list);
  for( i = 0; i<5; ++i )
    free(row_cur[i]);
  free(escape.ent);
  if( do_free&1 )
    free(this);
}

// A block on the heap, laid out for one image.  Both plane coders wrote the
// allocation out and each guarded it the same way: call `layout_workspace` on
// the raw pointer if `bmf_new` returned one, and hand back null otherwise --
// which the caller then dereferenced unconditionally.  That is the binary's
// behaviour and it stays; the guard is one place now rather than two.
ModelBlock* new_model_block(int32_t img_w, int32_t img_h, int32_t img_depth) {
  void* raw = bmf_new(sizeof(ModelBlock));
  if( !raw )
    return nullptr;
  return ((ModelBlock*)raw)->layout_workspace(img_w, img_h, img_depth);
}

ModelBlock*ModelBlock::layout_workspace(int32_t img_w, int32_t img_h, int32_t img_depth) {
  int32_t j;
  uint32_t k, m, s, n;
  uint32_t w = img_w;
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
        for( int32_t i = 0; i<6; ++i ) row_cur[j][r].match[5-i] = 1;
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
  memset(exclusion_mask, 0, sizeof exclusion_mask);
  sel[0] = nullptr;
  sel[1] = nullptr;
  escape_list = nullptr;
  for( n = 0; n<0x40000; ++n ) {
    sym_ctr[2*n] = no_symbol;
    sym_ctr[2*n+1] = no_symbol;
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
