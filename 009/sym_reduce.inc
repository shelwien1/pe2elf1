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
static void reduce_narrow_alphabet(BMFState* cx, SymList* lists, ModelBlock* blk1, uint8_t* src,
                                   uint32_t mask) {
  int32_t n_distinct, y, bits, shift, prev, s, s_next;
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
      // `at` and `y` were `at_spill` and `y_spill`, two more slots in that
      // shared area, written at the bottom of the row loop and read back at
      // the top of the next -- which is what a local does.  `bits_spill` was
      // narrower still: assigned `shift` and read back four lines later.
      // `row_w = blk1->width` was restated inside the pixel loop, and the
      // `break` above tested it -- a member nothing in either body writes, so
      // the test is the same answer on every row and the `for` below carries
      // it.  `shift & 31` is a dead mask: this path is sub-byte only, so `bpp`
      // is 1, 2 or 4 and the shift is under 8.
      const uint8_t* p = src-1;
      int32_t at = 0;
      const int32_t bpp = blk1->depth;
      for( y = 0; y<blk1->height; ++y ) {
        bits = 0;
        for( uint32_t x = 0; x<blk1->width; ++x, ++at ) {
          shift = bits-bpp;
          if( shift<0 ) {
            ++p;
            shift = 8-bpp;
          }
          const uint32_t sym = mask&(*p>>shift);
          blk1->alphabet += sym_flag[sym]==0;
          sym_flag[sym] = 1;
          bits = shift;
          blk1->sym_word[at] = sym;
        }
      }
      n_distinct = blk1->alphabet;
    }
  } else if( height&&blk1->width ) {
    const uint8_t *q = src;
    const uint32_t n_pix = blk1->height*blk1->width;
    for( uint32_t idx = 0; idx<n_pix; ++idx ) {
      const int32_t sym2 = *q++;
      blk1->alphabet += sym_flag[sym2]==0;
      sym_flag[sym2] = 1;
      blk1->sym_word[idx] = sym2;
    }
    n_distinct = blk1->alphabet;
  } else {
    n_distinct = 0;
  }
  cx->code_alphabet_size<0>(n_distinct, mask+1);
  uint32_t n_syms = blk1->alphabet;
  if( n_syms<=mask ) {
    init_gap_list(cx, lists, mask, n_syms);
    n_syms3 = blk1->alphabet;
    if( n_syms3 ) {
      prev = 0;
      s = 0;
      uint32_t next_id = 0;
      do {
        if( sym_flag[s] ) {
          lists[0].code_symbol(cx, s-prev);
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
    {
      // Every pixel through the reduced alphabet.  The `if` was the `do`'s
      // guaranteed first turn being made safe for an empty image.
      const uint32_t n_pix = blk1->height*blk1->width;
      for( uint32_t m = 0; m<n_pix; ++m )
        blk1->sym_word[m] = sym_flag[blk1->sym_word[m]];
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
int32_t BMFState::tree_place(ReduceNode* tree, ModelBlock* blk, uint32_t val) {
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

// **The alphabet's size, as a flat symbol in `[0, cap)`.**  Both arms of the
// encoder write it and `expand_alphabet` reads it, and `rc_decode_flat` is
// already `rc.encode(s, s+1, tot)` read backwards -- so the two are the same
// field and were spelled as two.  The encoder codes `n - 1`, which is why the
// decoder's `+1` is here rather than at its one call site.
template<int32_t f_DEC>
uint32_t BMFState::code_alphabet_size(uint32_t n, uint32_t cap) {
  if constexpr( f_DEC )
    return rc_decode_flat(cap)+1;
  rc.encode(n-1, n, cap);
  return n;
}

// **One symbol of a wide alphabet, a byte at a time, either way.**  The byte
// being coded picks the list for the byte above it -- `carry` is the top two
// bits of the byte just coded, and the caller's `4*b + carry` index is what
// makes this a context.  After the last byte the carry becomes the *bottom*
// byte's top bit, which carries into the next symbol.
//
// The two bodies this came out of are the same loop over the same index with
// the same two carry updates; what made them look different is that the
// encoder walks a value it already has and shifts it down, while the decoder
// builds one up.  Written with the coded byte named in both, they are one
// body: `piece` is what goes on the stream, and `carry` is a function of it
// either way.
//
// The encoder's `val` comes out of the search tree and the decoder's is
// assembled here, so the return is the symbol in both directions -- which is
// also what the trailing carry has to be taken from.
template<int32_t f_DEC>
uint32_t BMFState::code_symbol_bytes(SymList* lists, uint32_t nbytes,
                                     uint32_t val, int32_t& carry) {
  uint32_t word = val, out = 0;
  for( uint32_t b = 0; b<nbytes; ++b ) {
    uint32_t piece;
    if constexpr( f_DEC ) {
      piece = lists[4*b+carry].decode_symbol_list(this);
      out += piece<<((8*b)&31);
    } else {
      piece = (uint8_t)word;
      lists[4*b+carry].code_symbol(this, (uint8_t)piece);
      word >>= 8;
    }
    carry = piece>>6;
  }
  if constexpr( !f_DEC )
    out = val;
  carry = (uint8_t)out>>7;
  return out;
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
void BMFState::reduce_alphabet(ModelBlock* blk, uint8_t* src) {
  ReduceNode tree[8192];
  SymList lists[16];
  // `out[1]` is the buffer the plane is de-interleaved into and `out[2 + k]`
  // walks component `k` of it.  Nineteen entries because that is how wide the
  // slot was; a depth the header admits is at most 32 bits, so at most four
  // components, so nothing past `out[5]` is ever touched.
  uint8_t* out[19];
  uint32_t alpha_n, done;
  int32_t node, carry;
  uint8_t *rp;
  uint32_t sym_bits = blk->depth;
  uint32_t mask = 0xFFFFFFFF>>(-sym_bits&31);
  uint32_t n_kids = (sym_bits+7)>>3;
  // One symbol's bytes as a word.  `p` walks by `n_kids`, which is 2, 3 or 4
  // here, so two of the three strides leave these loads *unaligned* -- and
  // `*(uint32_t*)p` on an unaligned address is the aliasing violation the
  // `-fno-strict-aliasing` build has been carrying rather than the alignment
  // fault it would be elsewhere.
  //
  // `memcpy` is the standard-sanctioned spelling and compiles to the same
  // unaligned load at -O2.  Not `std::bit_cast` over a `std::array` (kimi's
  // suggestion): that works but goes the long way round, and `hist_bump` next
  // door already uses `memcpy` for exactly this.
  auto load32 = [](const uint8_t* at) {
    uint32_t v;
    memcpy(&v, at, sizeof v);
    return v;
  };
  clear_sym_lists(lists);
  if( sym_bits<=8 ) {
    reduce_narrow_alphabet(this, lists, blk, src, mask);
  } else {
    memset(tree, 0, sizeof tree);
    blk->alphabet = 1;
    tree[0].val = mask&load32(src);
    blk->sym_word[0] = 0;
    if( (uint32_t)(blk->height*blk->width)>1 ) {
      uint8_t *p = src;
      node = 0;
      uint32_t written = 1;
      while( 1 ) {
        p += n_kids;
        uint32_t val = mask&load32(p);
        // `node = 0` makes `tree[node]` the root, so the second test is the
        // first one again with the walk restarted -- and the overflow check
        // can sit beside it rather than one level in, because a value that is
        // the root's is node 0 and never negative.
        if( val!=tree[node].val ) {
          node = 0;
          if( val!=tree[0].val )
            node = tree_place(tree, blk, val);
          if( node<0 )
            break;
        }
        blk->sym_word[written++] = node;
        if( written>=(uint32_t)(blk->height*blk->width) )
          break;
      }
    }
    // Both exits of the loop above jumped here past this line, which is the
    // line they set `alpha_n` from.  Leaving the loop runs it.
    alpha_n = blk->alphabet;
    code_alphabet_size<0>(alpha_n, no_symbol+1);
    if( blk->alphabet>0x2000 ) {
      out[1] = (uint8_t*)bmf_new(blk->height*n_kids*blk->width);
      int32_t img_w = blk->width;
      int32_t img_h = blk->height;
      uint32_t plane_size;
      if( n_kids ) {
        plane_size = img_h*img_w;
        if( n_kids>>1 ) {
          // `2*pairs++*plane_size` is the post-increment-in-an-expression
          // Phase 2 asks for by name: it reads as `2 * pairs * plane_size`.
          uint8_t *half = out[1]+img_h*img_w;
          uint32_t pairs = 0;
          for( ; pairs<n_kids>>1; ++pairs ) {
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
      // read.  The byte walk itself is `code_symbol_bytes`, which is where the
      // reload that fed the trailing carry went: it wants the symbol, and the
      // symbol is what that returns.
      init_byte_lists(this, lists, n_kids);
      const int32_t alpha_m = blk->alphabet;
      if( alpha_m ) {
        carry = 0;
        for( uint32_t si = 0; si<(uint32_t)alpha_m; ++si )
          code_symbol_bytes<0>(lists, n_kids, tree[si].val, carry);
      }
    }
    free_sym_entries(lists, 16);
  }
}
