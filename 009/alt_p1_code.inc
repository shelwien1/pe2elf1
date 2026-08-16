// alt_p1_code.inc -- the predictor-1 model, both directions.
//
// The two bodies this replaces were 263 and 212 lines and shared 134 of them,
// in three runs that carry most of the function: the header parse and the
// per-plane block allocation, the 57 lines of per-row rotation -- five buffers
// moved round, five cursors reseated, the two context sums rebuilt from the
// rows above -- and the teardown.  None of that has a direction.
//
// `buf` is the encoder's `src` and the decoder's `out`: the same walk over the
// interleaved planes, read in one direction and written in the other.
//
// What is one-sided is the near-lossless drift control and the side the colour
// transform falls on.  The encoder takes a sample, subtracts the transform,
// folds the residual, reconstructs it, and if the reconstruction has drifted
// more than sixteen it re-sends through `fold_hi` and leaves the original byte
// in `buf`.  The decoder is told which code was used, and adds the transform
// back after the fact -- which is why its stores sit one plane behind, between
// a plane's cursor advance and the next plane's context.
//
// Two spellings differed and are the same value, and both are `abs32` now.
// The encoder wrote a plane's magnitude as `(BYTE4(err) ^ err) - BYTE4(err)`
// over an `int64_t` -- the sign of `err` broadcast into byte 4, exclusive-ored
// and subtracted, which is `|err|` in the low eight bits and nothing in the
// rest -- and the decoder as `abs32(val - pred)`.  `P1Ctx::mag` is a
// `uint8_t`, so eight bits is all either of them keeps.  The `int64_t` went
// with the macro: the width existed so that a byte 4 existed to read.  And the
// decoder held `blk->ctx[0]` in a local across the `ctx[2]` update, which
// cannot change it -- they are different elements.

// A plane's cross-plane prediction: the weighted sum, in 128ths, of the planes
// already coded at this pixel.  `weight0` goes with plane 0, `weight1` with
// plane 1, `weight2` with plane 2.
//
// Two of them because the original's two sites are not the same expression,
// and the differences are load-bearing:
//
//   * the rounding bias is 40 for two planes and 64 for three.  64 is half of
//     128 and rounds to nearest; 40 does not, and biases low by 24/128 of a
//     level.  Both are what BMF.exe does.
//   * the two-plane form's second term is cast to `uint32_t`, which makes the
//     whole sum unsigned and the `>>7` a logical shift.  The three-plane form
//     stays signed and shifts arithmetically.  With a negative weight the two
//     give different answers, so they cannot be one function with a bias
//     parameter, which is what the first attempt at this was.
//
// The three-plane form reads its operands as `p[-3]`, `p[-2]`, `p[-1]`: the
// planes are interleaved, so the three bytes before this one are planes 0, 1
// and 2 of the same pixel.
static inline uint32_t plane_mix2(const PlaneDesc& d, const uint8_t* p0, const uint8_t* p1) {
  return (d.weight0**p0+d.weight1*(uint32_t)*p1+40)>>7;
}

static inline int32_t plane_mix3(const PlaneDesc& d, const uint8_t* p) {
  return (d.weight1*p[-2]+d.weight0*p[-3]+d.weight2*p[-1]+64)>>7;
}

template<int32_t f_DEC>
int32_t alt_model_p1(BmfImage* hdr, uint8_t* buf) {
  int32_t cur3;
  // `void* plane[4]` with three `AltP1Block*&` aliases onto slots 1..3 -- the
  // decompiler's way of saying that a `void*` array is read as blocks.  It is
  // an array of blocks.
  AltP1Block* plane[4];
  int32_t cur0, cur1;
  AltP1Block* made;
  int32_t w, n_planes, want2;
  uint8_t want1, want3;
  int32_t width = hdr->width;
  int32_t height = hdr->height;
  if( plane_count>0 ) {
    int32_t k = 0;
    do {
      AltP1Block *raw = (AltP1Block*)bmf_new(sizeof(AltP1Block));
      if( raw )
        made = raw->alt_p1_alloc(width, height, k);
      else
        made = nullptr;
      plane[k++] = made;
    } while( k<plane_count );
  }
  int32_t src1 = plane_desc[1].src_plane;
  int32_t src2 = plane_desc[2].src_plane;
  int32_t src3 = plane_desc[3].src_plane;
  uint8_t fl1 = plane_desc[src1].flags;
  uint8_t fl2 = plane_desc[src2].flags;
  uint8_t fl3 = plane_desc[src3].flags;
  int32_t dc2 = plane_desc[src2].dc;
  int32_t dc1 = plane_desc[src1].dc;
  int32_t xf1 = fl1&8;
  int8_t dc3 = plane_desc[src3].dc;
  int32_t xf2 = fl2&8;
  int32_t xf3 = fl3&8;
  if( f_DEC )
    rc_begin_decode();
  else
    rc_begin_encode();
  if( height>0 ) {
    uint32_t y = 0;
    w = width;
    n_planes = plane_count;
    do {
      if( n_planes>0 ) {
        int32_t p = 0;
        do {
          ++p;
          AltP1Block* blk_k = plane[p-1];
          blk_k->advance_row();
          blk_k->seed_activity(true);
          n_planes = plane_count;
        } while( p<plane_count );
        w = width;
      }
      if( w>0 ) {
        // One plane's sample, coded either way: its context built from the
        // planes beside it, the sample itself, and the record it leaves.  All
        // four planes do exactly this and all four wrote it out, each with a
        // `blkN` of its own that no line outside the four uses.
        auto code_sample = [&](int32_t n, AltP1Block* a, AltP1Block* b,
                               int32_t off, int32_t val) {
          AltP1Block*const blk = plane[n];
          blk->ctx_of(a, b);
          val = f_DEC ? blk->decode_sample()
                      : blk->encode_sample(buf, off, val);
          blk->record_sample(val);
          return val;
        };
        uint32_t x = 0;
        do {
          int32_t off0 = plane_desc[0].src_plane;
          if( !f_DEC ) {
            int32_t want0 = *(buf+off0);
            cur0 = want0;
          }
          // `keep0` was a third name for `cur0`: assigned from it in both arms
          // and again beside it when the drift check keeps the reconstruction.
          cur0 = code_sample(0, nullptr, nullptr, off0, cur0);
          int32_t off1 = plane_desc[1].src_plane;
          if( f_DEC ) {
            *(buf+off0) = cur0;
          } else {
            want1 = *(buf+off1);
            if( xf1 )
              want1 = want1-dc1-*(buf+off0);
            cur1 = want1;
          }
          cur1 = code_sample(1, plane[0], nullptr, off1, cur1);
          int32_t off2 = plane_desc[2].src_plane;
          if( f_DEC ) {
            int32_t val1x = cur1;
            if( xf1 )
              val1x += dc1+*(buf+off0);
            *(buf+off1) = val1x;
          } else {
            want2 = *(buf+off2);
            if( xf2 )
              want2 = (uint8_t)(want2-dc2-(plane_mix2(plane_desc[off2], buf+off0, buf+off1)));
          }
          want2 = code_sample(2, plane[1], plane[0], off2, want2);
          if( f_DEC ) {
            if( xf2 )
              *(buf+off2) = (plane_mix2(plane_desc[off2], buf+off0, buf+off1))+dc2+want2;
            else
              *(buf+off2) = want2;
          }
          n_planes = plane_count;
          if( plane_count>=4 ) {
            int32_t off3 = plane_desc[3].src_plane;
            if( !f_DEC ) {
              if( xf3 )
                want3 = *(buf+off3)-dc3-(plane_mix3(plane_desc[off3], buf+off3));
              else
                want3 = *(buf+off3);
              cur3 = want3;
            }
            cur3 = code_sample(3, plane[2], plane[1], off3, cur3);
            if( f_DEC ) {
              int32_t val3x = cur3;
              if( xf3 )
                val3x += (plane_mix3(plane_desc[off3], buf+off3))+dc3;
              *(buf+off3) = val3x;
            }
            n_planes = plane_count;
          }
          buf += n_planes;
          ++x;
        } while( x<(uint32_t)width );
        w = width;
      }
      ++y;
    } while( y<(uint32_t)height );
  }
  if( f_DEC )
    rc_end_decode();
  else
    rc_end_encode();
  int32_t np = plane_count;
  if( plane_count>0 ) {
    int32_t f = 0;
    do {
      AltP1Block* q = plane[f];
      if( q ) {
        q->alt_p1_free(1);
        np = plane_count;
      }
      ++f;
    } while( f<np );
  }
  return np;
}

int32_t alt_model_p1_encode(BmfImage* hdr, uint8_t* src) {
  return alt_model_p1<0>(hdr, src);
}

int32_t alt_model_p1_decode(BmfImage* hdr, uint8_t* out) {
  return alt_model_p1<1>(hdr, out);
}
