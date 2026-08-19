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
//
// And that is the same expression `PlaneTransform::blend` computes in
// `plane_predict.inc` -- same weights, same operands, same shift -- with one
// difference nobody would predict: **blend rounds the three-plane form with 63
// and this rounds it with 64.**  The two-plane forms agree at 40.  So the same
// reference prediction is a level apart depending on whether the plane is being
// coded planar (through `code_colour_plane`) or interleaved (through here).
//
// Left alone deliberately, and noted here because the obvious tidy -- one blend
// helper called from both -- silently changes the output of every four-plane
// image.  Both constants are in BMF.exe.  `plane_mix3` is entered by `altp1`,
// `med32` and `t32` in the corpus, so neither side is a path the program never
// takes; counted by instrumenting both and running all nineteen images.
static inline uint32_t plane_mix2(const PlaneDesc& d, const uint8_t* p0, const uint8_t* p1) {
  return (d.weight0**p0+d.weight1*(uint32_t)*p1+40)>>7;
}

static inline int32_t plane_mix3(const PlaneDesc& d, const uint8_t* p) {
  return (d.weight1*p[-2]+d.weight0*p[-3]+d.weight2*p[-1]+64)>>7;
}

template<int32_t f_DEC>
int32_t alt_model_p1(BmfImage* hdr, uint8_t* buf) {
  // `void* plane[4]` with three `AltP1Block*&` aliases onto slots 1..3 -- the
  // decompiler's way of saying that a `void*` array is read as blocks.  It is
  // an array of blocks.
  AltP1Block* plane[4];
  int32_t cur0, cur1, cur3;
  int32_t want2;
  int32_t width = hdr->width;
  int32_t height = hdr->height;
  for( int32_t k = 0; k<plane_count; ++k ) {
    AltP1Block *raw = (AltP1Block*)bmf_new(sizeof(AltP1Block));
    plane[k] = raw->alt_p1_alloc(width, height, k);
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
  {
    // `n_planes = plane_count` and `w = width` were restated six times between
    // them inside bodies that write neither -- MSVC reloading two globals it
    // could not prove unchanged across the calls that sit between the reads.
    for( uint32_t y = 0; y<(uint32_t)height; ++y ) {
      for( int32_t p = 0; p<plane_count; ++p ) {
        plane[p]->advance_row();
        plane[p]->seed_activity(true);
      }
      {
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
        // The residual the encoder hands the coder, and the pixel the decoder
        // writes back -- the pixel less the DC and less what the planes before
        // it predict, and the same sum put back.  Four planes do each, and all
        // eight were written out: three of the four spelled the intermediate
        // into a name of its own (`want1`, `val1x`, `val3x`) and the fourth put
        // the sum in the store.
        //
        // Three of the four residuals were narrowed to a byte on their way to
        // the coder -- two through a `uint8_t` temporary and one through a
        // cast -- and the fourth is a plane that is never transformed, so its
        // value is a byte already.  The casts are at the calls.
        //
        // `mix` is evaluated at the call whether or not the plane is
        // transformed, which the four sites did not do.  It is a pure read of
        // the pixels beside this one either way -- the encoder's from the
        // image and the decoder's from what it has already put back, which are
        // the same bytes at this point, and that is what makes the codec work.
        auto fold_from = [&](int32_t off, int32_t xf, int32_t dc, int32_t mix) {
          const int32_t v = *(buf+off);
          return xf ? v-dc-mix : v;
        };
        auto unfold_to = [&](int32_t off, int32_t xf, int32_t dc, int32_t mix,
                             int32_t val) {
          if( xf )
            val += dc+mix;
          *(buf+off) = val;
        };
        for( uint32_t x = 0; x<(uint32_t)width; ++x ) {
          int32_t off0 = plane_desc[0].src_plane;
          if( !f_DEC )
            cur0 = fold_from(off0, 0, 0, 0);
          // `keep0` was a third name for `cur0`: assigned from it in both arms
          // and again beside it when the drift check keeps the reconstruction.
          cur0 = code_sample(0, nullptr, nullptr, off0, cur0);
          int32_t off1 = plane_desc[1].src_plane;
          if( f_DEC )
            unfold_to(off0, 0, 0, 0, cur0);
          else
            // Narrowed to a byte, as the `uint8_t want1` this replaces was:
            // the residual reaches the coder eight bits wide.
            cur1 = (uint8_t)fold_from(off1, xf1, dc1, *(buf+off0));
          cur1 = code_sample(1, plane[0], nullptr, off1, cur1);
          int32_t off2 = plane_desc[2].src_plane;
          if( f_DEC )
            unfold_to(off1, xf1, dc1, *(buf+off0), cur1);
          else
            want2 = (uint8_t)fold_from(off2, xf2, dc2,
                                       plane_mix2(plane_desc[off2], buf+off0, buf+off1));
          want2 = code_sample(2, plane[1], plane[0], off2, want2);
          if( f_DEC )
            unfold_to(off2, xf2, dc2,
                      plane_mix2(plane_desc[off2], buf+off0, buf+off1), want2);
          if( plane_count>=4 ) {
            int32_t off3 = plane_desc[3].src_plane;
            if( !f_DEC )
              cur3 = (uint8_t)fold_from(off3, xf3, dc3,
                                        plane_mix3(plane_desc[off3], buf+off3));
            cur3 = code_sample(3, plane[2], plane[1], off3, cur3);
            if( f_DEC )
              unfold_to(off3, xf3, dc3,
                        plane_mix3(plane_desc[off3], buf+off3), cur3);
          }
          buf += plane_count;
        }
      }
    }
  }
  if( f_DEC )
    rc_end_decode();
  else
    rc_end_encode();
  // `bmf_new` is fatal, so every slot is populated -- the `if( q )` was the
  // same dead null check 11.1 removed from the p2 side.
  for( int32_t f = 0; f<plane_count; ++f )
    plane[f]->alt_p1_free();
  return plane_count;
}

int32_t alt_model_p1_encode(BmfImage* hdr, uint8_t* src) {
  return alt_model_p1<0>(hdr, src);
}

int32_t alt_model_p1_decode(BmfImage* hdr, uint8_t* out) {
  return alt_model_p1<1>(hdr, out);
}
