// sym_reduce.inc -- drop the symbols an image does not use, and renumber what is left.

// One node of the wide arm's search tree: a symbol value and its two children,
// in the eight bytes MSVC gave each of them.
//
// The recovery had this as two arrays.  The value was `buf[8*node]` read
// through a `uint32_t *` and the children were `kids[node]`, a `uint16_t[4]`,
// and the two *interleave*: `kids` begins four bytes after `buf` and both
// stride by eight, so `kids[n]`'s first two entries are the children of the
// value at `buf[8*n]` and its last two are the next value.  That is one array
// of one record written as two, and the only reason for `[127]` is that 127
// was as far as the recovery could see it indexed.
//
// How many there are is not a guess either: `memset(buf, 0, 0x10000)` clears
// 65536 bytes, which is 8192 of these, and the tree gives up at 0x2000 nodes.
// `_pad0[64504]` was the difference between the 127 and the 8192.
struct ReduceNode {
  uint32_t val;
  uint16_t kid[2];
};

static_assert(sizeof(ReduceNode) == 8,
              "the tree strides by eight bytes per node");

// Eight bits or fewer, where a symbol is a byte (or a packed field of one) and
// the whole alphabet fits in a 256-entry flag table.  Count the distinct
// symbols, code the count, then code the gaps between the ones that occur and
// renumber the image through the same table.
//
// The wide arm below does none of this -- it builds a binary search tree over
// multi-byte symbols -- and the two shared nothing but a frame and 350 lines.
static void reduce_narrow_alphabet(SymList* lists, ModelBlock* blk1, uint8_t* src,
                                   uint32_t mask) {
  int32_t n_distinct, row_w, y, bits, shift, prev, s, s_next;
  uint32_t n_syms3, sym_flag[256];
  // The flag table, one entry per symbol, and then the renumbering: the same
  // 1024 bytes hold a 0/1 flag while the alphabet is being counted and the new
  // symbol number afterwards.  It was `(uint32_t *)&kids_i` -- the frame's
  // node array read from four bytes before it -- because the two arms of
  // `reduce_alphabet` share one stack area and MSVC gave them one slot.  Only
  // one of the two ever runs, so this arm can own its table.
  memset(sym_flag, 0, sizeof sym_flag);
  bool packed = blk1->depth<8;
  int32_t height = blk1->height;
  blk1->alphabet = 0;
  if( packed ) {
    n_distinct = 0;
    if( height ) {
      row_w = blk1->width;
      // `at` and `y` were `at_spill` and `y_spill`, two more slots in that
      // shared area, written at the bottom of the row loop and read back at
      // the top of the next -- which is what a local does.  `bits_spill` was
      // narrower still: assigned `shift` and read back four lines later.
      const uint8_t* p = src-1;
      int32_t at = 0;
      y = 0;
      do {
        if( !row_w )
          break;
        uint32_t x = 0;
        bits = 0;
        do {
          int32_t bpp = blk1->depth;
          shift = bits-bpp;
          if( shift<0 ) {
            ++p;
            shift = 8-bpp;
          }
          uint32_t sym = mask&(*p>>(shift&31));
          bool first = sym_flag[sym]==0;
          ++x;
          sym_flag[sym] = 1;
          bits = shift;
          blk1->alphabet += first;
          blk1->sym_word[at] = sym;
          row_w = blk1->width;
          ++at;
        } while( x<blk1->width );
        n_distinct = blk1->alphabet;
        ++y;
      } while( (uint32_t)y<(uint32_t)blk1->height );
    }
  } else if( height*blk1->width ) {
    uint8_t *q = src;
    uint32_t idx = 0;
    do {
      blk1->alphabet += sym_flag[*q]==0;
      int32_t sym2 = *q;
      sym_flag[sym2] = 1;
      ++q;
      blk1->sym_word[idx++] = sym2;
    } while( idx<(uint32_t)(blk1->height*blk1->width) );
    n_distinct = blk1->alphabet;
  } else {
    n_distinct = 0;
  }
  rc.encode(n_distinct-1, n_distinct, mask+1);
  uint32_t n_syms = blk1->alphabet;
  if( n_syms<=mask ) {
    lists[0].init(mask-n_syms+2, 1);
    lists[0].rescale_at = 19*lists[0].n;
    n_syms3 = blk1->alphabet;
    if( n_syms3 ) {
      prev = 0;
      s = 0;
      uint32_t next_id = 0;
      do {
        if( sym_flag[s] ) {
          lists[0].code_symbol(s-prev);
          n_syms3 = blk1->alphabet;
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
    if( blk1->height*blk1->width ) {
      uint32_t m = 0;
      do {
        blk1->sym_word[m] = sym_flag[blk1->sym_word[m]];
        ++m;
      } while( m<(uint32_t)(blk1->height*blk1->width) );
    }
    free_sym_entries(lists, 16);
  } else {
    free_sym_entries(lists, 16);
  }
}

// Find `val` in the tree, hanging a new leaf off the side the walk fell out
// of when it is not there.  Returns the node holding `val` afterwards, or -1
// when that insert took the alphabet past the 0x2000 the tree gives up at.
//
// This was the deepest thing in the file: a `while( 1 )` inside an `if` inside
// an `if` inside a `while( 1 )`, with a `goto` out of the middle of it to a
// label six lines below the outer loop's bottom.  The jump and the label are
// the same statement said twice -- "the node for `val` is settled, go record
// it" -- which is what a return is.
//
// `mode_symbol[1]` is the side the leaf was hung on, and it is read by the
// caller's caller rather than here, so it stays a global write.
static int32_t tree_place(ReduceNode* tree, ModelBlock* blk, uint32_t val) {
  int32_t node = 0, side;
  uint16_t* kidp;
  while( 1 ) {
    side = tree[node].val<val;
    kidp = tree[node].kid;
    node = kidp[side];
    if( !node )
      break;
    if( val==tree[node].val ) {
      mode_symbol[1] = side;
      return node;
    }
  }
  int32_t alphabet = blk->alphabet;
  mode_symbol[1] = side;
  node = (uint16_t)alphabet;
  int32_t alpha = alphabet+1;
  kidp[side] = node;
  blk->alphabet = alpha;
  if( alpha>0x2000 )
    return -1;
  tree[node].val = val;
  return node;
}

// The wide arm: symbols too big for a flag table, so the distinct values go
// into a binary search tree and the image is renumbered through it.
//
// This was the last reconstructed stack frame in the tree, and every one of its
// members turned out to be one of three things.  `zero_base[15]` was 60 bytes
// nothing read.  `slot0`..`slot11` were spills: MSVC keeps `blk1`, `n_kids`,
// `srcp`, `p` and `written` in registers, drops them to the stack across a call
// or an inner loop, and reads them back, so `frame.slot9 = n_kids;` twelve
// lines above `n_kids = frame.slot9;` says nothing that `n_kids` does not.
// What is left is `out`, and that one is real: the same eight bytes hold the
// source cursor's spill early on and an output cursor later, which is what the
// union recorded and why the slots could not simply be renamed apart.
void reduce_alphabet(ModelBlock* blk, uint8_t* src) {
  ReduceNode tree[8192];
  SymList lists[16];
  // `out[1]` is the buffer the plane is de-interleaved into and `out[2 + k]`
  // walks component `k` of it.  Nineteen entries because that is how wide the
  // slot was; a depth the header admits is at most 32 bits, so at most four
  // components, so nothing past `out[5]` is ever touched.
  uint8_t* out[19];
  uint32_t alpha_n, done;
  int32_t node, carry;
  uint32_t word, k;
  uint8_t *rp;
  uint32_t sym_bits = blk->depth;
  uint32_t mask = 0xFFFFFFFF>>(-sym_bits&31);
  uint32_t n_kids = (sym_bits+7)>>3;
  clear_sym_lists(lists);
  if( sym_bits<=8 ) {
    reduce_narrow_alphabet(lists, blk, src, mask);
  } else {
    memset(tree, 0, sizeof tree);
    blk->alphabet = 1;
    tree[0].val = mask&*(uint32_t*)src;
    blk->sym_word[0] = 0;
    if( (uint32_t)(blk->height*blk->width)>1 ) {
      uint8_t *p = src;
      node = 0;
      uint32_t written = 1;
      while( 1 ) {
        p += n_kids;
        uint32_t val = mask&*(uint32_t*)p;
        if( val!=tree[node].val ) {
          node = 0;
          if( val!=tree[0].val ) {
            node = tree_place(tree, blk, val);
            if( node<0 ) {
              alpha_n = blk->alphabet;
              goto alphabet_done;
            }
          }
        }
        blk->sym_word[written++] = node;
        if( written>=(uint32_t)(blk->height*blk->width) ) {
          alpha_n = blk->alphabet;
          goto alphabet_done;
        }
      }
    }
    alpha_n = blk->alphabet;
// Both codings converge here with `blk->alphabet` set.
alphabet_done:
    rc.encode(alpha_n-1, alpha_n, 0x2001u);
    if( blk->alphabet>0x2000 ) {
      out[1] = (uint8_t*)bmf_new(blk->height*n_kids*blk->width);
      int32_t img_w = blk->width;
      int32_t img_h = blk->height;
      uint32_t plane_size;
      if( n_kids ) {
        plane_size = img_h*img_w;
        if( n_kids>>1 ) {
          uint8_t *half = out[1]+img_h*img_w;
          uint32_t pairs = 0;
          do {
            uint32_t slot_a = 2*pairs;
            uint32_t off = 2*pairs++*plane_size;
            out[slot_a+2] = out[1]+off;
            out[slot_a+3] = &half[off];
          } while( pairs<n_kids>>1 );
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
        // Deinterleave: each turn scatters `n_kids` consecutive source bytes
        // to the `n_kids` component cursors, one byte each, `plane_size`
        // turns.
        //
        // This was two `while( 1 )`s, a `goto` out of both and a `break` that
        // lands in the same place, and the shape of it was an unrolling by
        // *two*: an inner pair loop over `n_kids >> 1` and then a single extra
        // component for an odd count.  Which loop carried the turn counter
        // depended on the parity -- for an even `n_kids` the inner one broke
        // at once and the outer counted, for an odd one the inner never broke
        // and counted itself -- so the two increments are one increment and
        // the two loops are one loop.
        //
        // The pair loop's `j` reached `out[2*j+2]` and `out[2*j+3]` from
        // `rp[2*j]` and `rp[2*j+1]`, and the odd tail reached `out[2*n+2]`
        // from `rp[2*n]`: `out[c + 2]` takes `rp[c]` in both, which is the
        // line below.
        rp = src;
        if( n_kids ) {
          for( uint32_t moved = 0; moved<plane_size; ++moved ) {
            for( uint32_t c = 0; c<n_kids; ++c )
              *out[c+2]++ = rp[c];
            rp += n_kids;
          }
        }
      }
      uint16_t* old_words = blk->sym_word;
      blk->height = n_kids*img_h;
      blk->depth = 8;
      free(old_words);
      void *newbuf = bmf_new(2*blk->height*blk->width);
      blk->sym_word = (uint16_t*)newbuf;
      reduce_alphabet(blk, out[1]);
      free(out[1]);
    } else {
      // `alpha_m` was re-read from `blk->alphabet` after the list setup and
      // again inside the loop below as `si_end`, across walks that touch
      // neither -- the same spilling `expand_alphabet` does on its side.  One
      // read.  `word` likewise: the byte loop shifts it down and the line after
      // reloads it, and the only thing the reload feeds is the carry, which
      // wants the *low* byte.
      init_byte_lists(lists, n_kids);
      const int32_t alpha_m = blk->alphabet;
      if( alpha_m ) {
        carry = 0;
        uint32_t si = 0;
        do {
          word = tree[si].val;
          for( k = 0; k<n_kids; ++k ) {
            lists[4*k+carry].code_symbol((uint8_t)word);
            carry = (uint8_t)word>>6;
            word >>= 8;
          }
          carry = (uint8_t)tree[si].val>>7;
          ++si;
        } while( si<(uint32_t)alpha_m );
      }
    }
    free_sym_entries(lists, 16);
  }
}
