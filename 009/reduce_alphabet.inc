// reduce_alphabet.inc -- drop the symbols an image does not use, and renumber what is left.

void __reduce_alphabet(ModelBlock* blk, int8_t unread_flag, uint8_t* src) {
  struct alignas(16) ReduceAlphabetFrame {
    uint32_t zero_base[15];
    void* tmp;
    int32_t kids_i;
    uint8_t buf[4];
    uint32_t*sym_flag() {
      return (uint32_t*)&kids_i;
    }

    uint64_t kids[127];
    int32_t y_spill;
    int32_t at_spill;
    int32_t bits_spill;
    uint8_t _pad0[64504];
    SymList lists[16];
    union {
      void* slot[19];
      struct {
        int32_t slot0;
        BMF_SPILL_PAD(0);
        void* slot1;
        uint8_t* slot2;
        uint32_t slot3;
        BMF_SPILL_PAD(3);
        uint32_t slot4;
        BMF_SPILL_PAD(4);
        void* slot5;
        uint32_t slot6;
        BMF_SPILL_PAD(6);
        ModelBlock* slot7;
        uint8_t* slot8;
        uint32_t slot9;
        BMF_SPILL_PAD(9);
        void* slot10;
        ModelBlock* slot11;
        void* slot_tail[7];
      };
    };
  } __frame;
  int8_t mode;
  ModelBlock* blk1, * blk4;
  uint32_t alpha_n, done, done2, off, slot_a, depth_bits;
  bool more, packed;
  uint8_t* half;
  int32_t node, side, alpha_m, carry, img_w, img_h, n_moved, height, n_distinct, row_w, y, bits, shift, prev, s, s_next;
  ModelBlock* blk3;
  uint32_t n_kids, i, written, li, si, si_end, word, k, pairs, n_pairs, j, n_syms, n_syms3;
  uint16_t* kidp;
  uint8_t *srcp, *p, *rp, *dst_a, *dst_b, *dst_c;
  uint32_t val;
  void* newbuf;
  __frame.slot11 = (ModelBlock*)(blk);
  srcp = src;
  depth_bits = blk->depth;
  __frame.slot2 = src;
  __frame.slot7 = (ModelBlock*)(blk);
  __frame.slot4 = 0xFFFFFFFF>>(-depth_bits&31);
  n_kids = (depth_bits+7)>>3;
  for( i = 0; i<8; ++i ) {
    __frame.lists[2*i].ent = nullptr;
    __frame.lists[2*i+1].ent = nullptr;
  }
  blk1 = __frame.slot7;
  if( depth_bits<=8 ) {
    memset(__frame.sym_flag(), 0, 256*sizeof(uint32_t));
    packed = blk1->depth<8;
    height = blk1->height;
    blk1->alphabet = 0;
    if( packed ) {
      n_distinct = 0;
      if( height ) {
        row_w = blk1->width;
        __frame.slot8 = src-1;
        __frame.at_spill = 0;
        y = 0;
        do {
          if( !row_w )
            break;
          __frame.y_spill = y;
          uint32_t x = 0;
          int32_t at = __frame.at_spill;
          bits = 0;
          do {
            int32_t bpp = blk1->depth;
            shift = bits-bpp;
            if( shift<0 ) {
              ++__frame.slot8;
              shift = 8-bpp;
            }
            uint32_t sym = __frame.slot4&(*__frame.slot8>>(shift&31));
            bool first = __frame.sym_flag()[sym]==0;
            __frame.bits_spill = shift;
            ++x;
            __frame.sym_flag()[sym] = 1;
            bits = __frame.bits_spill;
            blk1->alphabet += first;
            blk1->sym_word[at] = sym;
            row_w = blk1->width;
            ++at;
          } while( x<(uint32_t)(*(int32_t*)&blk1->width) );
          n_distinct = blk1->alphabet;
          __frame.at_spill = at;
          y = __frame.y_spill+1;
        } while( (uint32_t)(__frame.y_spill+1)<(uint32_t)blk1->height );
      }
    } else if( height**(int32_t*)&blk1->width ) {
      uint8_t *q = __frame.slot2;
      uint32_t idx = 0;
      do {
        blk1->alphabet += __frame.sym_flag()[*q]==0;
        int32_t sym2 = *q;
        __frame.sym_flag()[sym2] = 1;
        ++q;
        blk1->sym_word[idx++] = sym2;
      } while( idx<(uint32_t)(blk1->height**(int32_t*)&blk1->width) );
      n_distinct = blk1->alphabet;
    } else {
      n_distinct = 0;
    }
    rc.encode(n_distinct-1, n_distinct, __frame.slot4+1);
    n_syms = blk1->alphabet;
    if( n_syms<=__frame.slot4 ) {
      __frame.lists[0].init(__frame.slot4-n_syms+2, 1);
      __frame.lists[0].rescale_at = 19*__frame.lists[0].n;
      n_syms3 = blk1->alphabet;
      if( n_syms3 ) {
        prev = 0;
        s = 0;
        uint32_t next_id = 0;
        do {
          if( __frame.sym_flag()[s] ) {
            __frame.lists[0].code_symbol(s-prev);
            n_syms3 = blk1->alphabet;
            __frame.sym_flag()[s] = next_id;
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
          blk1->sym_word[m] = __frame.sym_flag()[blk1->sym_word[m]];
          ++m;
        } while( m<(uint32_t)(blk1->height**(int32_t*)&blk1->width) );
      }
      free_sym_entries(__frame.lists, 16);
    } else {
      free_sym_entries(__frame.lists, 16);
    }
  } else {
    memset(__frame.buf, 0, 0x10000);
    blk1->alphabet = 1;
    *(uint32_t*)__frame.buf = __frame.slot4&*(uint32_t*)src;
    blk1->sym_word[0] = 0;
    if( (uint32_t)(blk1->height**(int32_t*)&blk1->width)>1 ) {
      __frame.slot8 = src;
      __frame.slot9 = n_kids;
      __frame.slot7 = (ModelBlock*)(blk1);
      p = __frame.slot2;
      node = 0;
      written = 1;
      while( 1 ) {
        p += __frame.slot9;
        val = __frame.slot4&*(uint32_t*)p;
        if( val!=*(uint32_t*)&__frame.buf[8*node] ) {
          node = 0;
          if( val!=*(uint32_t*)__frame.buf ) {
            __frame.slot3 = written;
            __frame.slot2 = p;
            while( 1 ) {
              side = *(uint32_t*)&__frame.buf[8*node]<val;
              kidp = (uint16_t*)&__frame.kids[node];
              node = kidp[side];
              if( !kidp[side] )
                break;
              if( val==*(uint32_t*)&__frame.buf[8*node] ) {
                written = __frame.slot3;
                p = __frame.slot2;
                mode_symbol[1] = side;
                goto LABEL_12;
              }
            }
            p = __frame.slot2;
            __frame.slot0 = side;
            ModelBlock *blk2 = (ModelBlock*)(__frame.slot7);
            int32_t alphabet = __frame.slot7->alphabet;
            mode_symbol[1] = side;
            node = (uint16_t)alphabet;
            int32_t alpha = alphabet+1;
            kidp[side] = node;
            written = __frame.slot3;
            blk2->alphabet = alpha;
            if( alpha>0x2000 ) {
              srcp = __frame.slot8;
              alpha_n = alpha;
              n_kids = __frame.slot9;
              blk1 = __frame.slot7;
              goto LABEL_14;
            }
            *(uint32_t*)&__frame.buf[8*node] = val;
          }
        }
LABEL_12:
        blk3 = __frame.slot7;
        __frame.slot7->sym_word[written++] = node;
        if( written>=blk3->height*blk3->width ) {
          srcp = __frame.slot8;
          n_kids = __frame.slot9;
          blk1 = __frame.slot7;
          alpha_n = __frame.slot7->alphabet;
          goto LABEL_14;
        }
      }
    }
    alpha_n = blk1->alphabet;
LABEL_14:
    rc.encode(alpha_n-1, alpha_n, 0x2001u);
    alpha_m = blk1->alphabet;
    if( alpha_m>0x2000 ) {
      (__frame.slot[1]) = bmf_new(blk1->height*n_kids*blk1->width);
      img_w = blk1->width;
      img_h = blk1->height;
      __frame.kids_i = blk1->width;
      __frame.slot0 = img_h;
      if( n_kids ) {
        __frame.slot6 = __frame.slot0*img_w;
        if( n_kids>>1 ) {
          half = (uint8_t*)(__frame.slot[1])+__frame.slot0*__frame.kids_i;
          __frame.slot8 = srcp;
          __frame.slot9 = n_kids;
          __frame.slot7 = (ModelBlock*)(blk1);
          pairs = 0;
          do {
            slot_a = 2*pairs;
            off = 2*pairs++*__frame.slot6;
            __frame.slot[slot_a+2] = (uint8_t*)(__frame.slot[1])+off;
            __frame.slot[slot_a+3] = (void*)&half[off];
          } while( pairs<n_kids>>1 );
          srcp = __frame.slot8;
          n_kids = __frame.slot9;
          blk1 = __frame.slot7;
          done = 2*pairs+1;
        } else {
          done = 1;
        }
        if( n_kids>(done-1) )
          __frame.slot[done+1] = (uint8_t*)(__frame.slot[1])+__frame.slot0* -__frame.kids_i+__frame.slot6*done;
      } else {
        __frame.slot6 = __frame.slot0*img_w;
      }
      if( __frame.slot6 ) {
        rp = src;
        if( n_kids ) {
          __frame.slot9 = n_kids;
          __frame.slot7 = (ModelBlock*)(blk1);
          __frame.kids_i = 0;
          n_moved = 0;
          n_pairs = n_kids>>1;
          while( 1 ) {
            while( 1 ) {
              if( n_pairs ) {
                for( j = 0; j<n_pairs; ++j ) {
                  dst_a = (uint8_t*)(__frame.slot[2*j+2]);
                  *dst_a = rp[2*j];
                  dst_b = (uint8_t*)__frame.slot[2*j+3];
                  __frame.slot[2*j+2] = dst_a+1;
                  *dst_b = rp[2*j+1];
                  __frame.slot[2*j+3] = dst_b+1;
                }
                done2 = 2*j+1;
                srcp = &rp[2*j];
              } else {
                done2 = 1;
              }
              if( (done2-1)>=__frame.slot9 )
                break;
              dst_c = (uint8_t*)__frame.slot[done2+1];
              srcp = &rp[done2];
              *dst_c = rp[done2-1];
              more = (uint32_t)(++n_moved)<__frame.slot6;
              __frame.slot[done2+1] = dst_c+1;
              if( !more )
                goto LABEL_71;
              rp += done2;
            }
            if( (uint32_t)(++n_moved)>=__frame.slot6 )
              break;
            rp = srcp;
          }
LABEL_71:
          n_kids = __frame.slot9;
          blk1 = __frame.slot7;
        }
      }
      __frame.tmp = blk1->sym_word;
      blk1->height = n_kids*__frame.slot0;
      blk1->depth = 8;
      free(__frame.tmp);
      newbuf = bmf_new(2*blk1->height*blk1->width);
      __frame.tmp = __frame.slot[1];
      blk1->sym_word = (uint16_t*)newbuf;
      __reduce_alphabet(blk1, mode, (uint8_t*)__frame.tmp);
      free((__frame.slot[1]));
    } else {
      if( 4*n_kids ) {
        __frame.slot7 = (ModelBlock*)(blk1);
        li = 0;
        do {
          __frame.lists[li].init(256, 1);
          ++li;
        } while( li<4*n_kids );
        blk1 = __frame.slot7;
        alpha_m = __frame.slot7->alphabet;
      }
      if( alpha_m ) {
        carry = 0;
        si = 0;
        si_end = alpha_m;
        blk4 = (ModelBlock*)(blk1);
        do {
          word = *(uint32_t*)&__frame.buf[8*si];
          if( n_kids ) {
            __frame.kids_i = si;
            __frame.slot9 = n_kids;
            __frame.slot7 = (ModelBlock*)(blk4);
            for( k = 0; k<__frame.slot9; ++k ) {
              __frame.lists[4*k+carry].code_symbol((uint8_t)word);
              carry = (uint8_t)word>>6;
              word >>= 8;
            }
            si = __frame.kids_i;
            n_kids = __frame.slot9;
            blk4 = (ModelBlock*)(__frame.slot7);
            word = *(uint32_t*)&__frame.buf[8*__frame.kids_i];
            si_end = __frame.slot7->alphabet;
          }
          carry = (uint8_t)word>>7;
          ++si;
        } while( si<si_end );
      }
    }
    free_sym_entries(__frame.lists, 16);
  }
}
