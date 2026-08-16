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
        } while( x<(uint32_t)(*(int32_t*)&blk1->width) );
        n_distinct = blk1->alphabet;
        ++y;
      } while( (uint32_t)y<(uint32_t)blk1->height );
    }
  } else if( height**(int32_t*)&blk1->width ) {
    uint8_t *q = src;
    uint32_t idx = 0;
    do {
      blk1->alphabet += sym_flag[*q]==0;
      int32_t sym2 = *q;
      sym_flag[sym2] = 1;
      ++q;
      blk1->sym_word[idx++] = sym2;
    } while( idx<(uint32_t)(blk1->height**(int32_t*)&blk1->width) );
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
    if( blk1->height**(int32_t*)&blk1->width ) {
      uint32_t m = 0;
      do {
        blk1->sym_word[m] = sym_flag[blk1->sym_word[m]];
        ++m;
      } while( m<(uint32_t)(blk1->height**(int32_t*)&blk1->width) );
    }
    free_sym_entries(lists, 16);
  } else {
    free_sym_entries(lists, 16);
  }
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
  uint32_t alpha_n, done, done2;
  int32_t node, side, alpha_m, carry;
  uint32_t si_end, word, k, j;
  uint16_t* kidp;
  uint8_t *rp;
  uint8_t *srcp = src;
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
    if( (uint32_t)(blk->height**(int32_t*)&blk->width)>1 ) {
      uint8_t *p = src;
      node = 0;
      uint32_t written = 1;
      while( 1 ) {
        p += n_kids;
        uint32_t val = mask&*(uint32_t*)p;
        if( val!=tree[node].val ) {
          node = 0;
          if( val!=tree[0].val ) {
            while( 1 ) {
              side = tree[node].val<val;
              kidp = tree[node].kid;
              node = kidp[side];
              if( !node )
                break;
              if( val==tree[node].val ) {
                mode_symbol[1] = side;
                goto code_dense;
              }
            }
            int32_t alphabet = blk->alphabet;
            mode_symbol[1] = side;
            node = (uint16_t)alphabet;
            int32_t alpha = alphabet+1;
            kidp[side] = node;
            blk->alphabet = alpha;
            if( alpha>0x2000 ) {
              alpha_n = alpha;
              goto alphabet_done;
            }
            tree[node].val = val;
          }
        }
// The alphabet is dense enough to code as a run of flags.
code_dense:
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
    alpha_m = blk->alphabet;
    if( alpha_m>0x2000 ) {
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
        rp = src;
        if( n_kids ) {
          int32_t n_moved = 0;
          uint32_t n_pairs = n_kids>>1;
          while( 1 ) {
            while( 1 ) {
              if( n_pairs ) {
                for( j = 0; j<n_pairs; ++j ) {
                  uint8_t *dst_a = out[2*j+2];
                  *dst_a = rp[2*j];
                  uint8_t *dst_b = out[2*j+3];
                  out[2*j+2] = dst_a+1;
                  *dst_b = rp[2*j+1];
                  out[2*j+3] = dst_b+1;
                }
                done2 = 2*j+1;
                srcp = &rp[2*j];
              } else {
                done2 = 1;
              }
              if( (done2-1)>=n_kids )
                break;
              uint8_t *dst_c = out[done2+1];
              srcp = &rp[done2];
              *dst_c = rp[done2-1];
              bool more = (uint32_t)(++n_moved)<plane_size;
              out[done2+1] = dst_c+1;
              if( !more )
                goto lists_ready;
              rp += done2;
            }
            if( (uint32_t)(++n_moved)>=plane_size )
              break;
            rp = srcp;
          }
// Two loops out at once, which is all the `goto` is for: the row ran out
// mid-component.  The `break` above lands here too.
lists_ready:
          ;
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
      if( 4*n_kids ) {
        uint32_t li = 0;
        do {
          lists[li].init(256, 1);
          ++li;
        } while( li<4*n_kids );
        alpha_m = blk->alphabet;
      }
      if( alpha_m ) {
        carry = 0;
        uint32_t si = 0;
        si_end = alpha_m;
        do {
          word = tree[si].val;
          if( n_kids ) {
            for( k = 0; k<n_kids; ++k ) {
              lists[4*k+carry].code_symbol((uint8_t)word);
              carry = (uint8_t)word>>6;
              word >>= 8;
            }
            word = tree[si].val;
            si_end = blk->alphabet;
          }
          carry = (uint8_t)word>>7;
          ++si;
        } while( si<si_end );
      }
    }
    free_sym_entries(lists, 16);
  }
}
