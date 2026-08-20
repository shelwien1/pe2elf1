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

// Returns nothing, and takes nothing.  It returned `(void**)this` -- after
// `free(this)` on the path that takes it, so the value was a dangling pointer
// -- and neither of the two callers reads it.  The `do_free` flag it tested is
// 1 at every call site of all four functions in this family (`alt_p1_free`,
// `alt_model_p1_d8_decode`, `alt_p2_free` and this), which is the same
// artefact one level up: the decompiler writes the register that happened to
// hold `this` as a return value wherever MSVC left it there, and a flag whose
// false arm no caller takes as a parameter.
void ModelBlock::free_workspace() {
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
  free(this);
}

// A block on the heap, laid out for one image.  Both plane coders wrote the
// allocation out and each guarded it the same way: call `layout_workspace` on
// the raw pointer if `bmf_new` returned one, and hand back null otherwise --
// which the caller then dereferenced unconditionally.  That is the binary's
// behaviour and it stays; the guard is one place now rather than two.
ModelBlock* new_model_block(int32_t img_w, int32_t img_h, int32_t img_depth) {
  void* raw = bmf_new(sizeof(ModelBlock));
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
    PixRec* buf = (PixRec*)bmf_new(sizeof(PixRec)*(w+ModelBlock::kRowMargin
                                                    +ModelBlock::kRowTail));
    row_cur[j] = buf;
    // The second cursor set starts one past the margin, and `start_row` then
    // reseats it *at* the margin -- so the 8 here and the 7 there are the same
    // edge approached from two sides, which is why they differ by one.
    row_cur[j+5] = buf+ModelBlock::kRowMargin+1;
    // `width + 16` is never negative -- `width` is a `uint16_t` field -- so the
    // `> -16` guard the `do` needed to be safe is gone with it, and so is the
    // `w = width` the loop re-did on every record.  The 16 is the two margins:
    // `ModelBlock::kRowMargin` on the left, which is what makes `row[-8]`
    // defined once the cursor has stepped, and `kRowTail` on the right.
    w = width;
    for( uint32_t r = 0; r<width+(uint32_t)(ModelBlock::kRowMargin+ModelBlock::kRowTail); ++r ) {
      row_cur[j][r].sym = 0;
      for( int32_t i = 0; i<6; ++i ) row_cur[j][r].match[5-i] = 1;
    }
  }
  uint8_t* runs = (uint8_t*)bmf_new(w+1);
  run_bucket = runs;
  *runs = 0;
  {
    // The post-increment inside the subscript -- `run_bucket[x+++1]` -- is the
    // one shape Phase 2 asks to be pulled out: it reads as `x + 1` and is
    // `(x++) + 1`.
    uint8_t bucket = 0;
    for( uint32_t x = 0; x<width; ++x ) {
      bucket += x==2u<<(bucket&31);
      run_bucket[x+1] = bucket;
    }
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
