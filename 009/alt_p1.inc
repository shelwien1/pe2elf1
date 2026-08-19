// alt_p1.inc -- the predictor-1 model, around its block.

// The residual codes: `unfold[]` runs 0, -1, +1, -2, +2, ... so a small residual
// of either sign gets a small code, and `fold[]` is its inverse.
//
// The buckets are `2*near_lossless_q + 1` wide, which is the near-lossless
// quantiser: at `E = 0` that is one and the mapping is the plain zigzag above.
// Nothing else in this build sets `near_lossless_q`, so the bucket arithmetic
// is present, correct and never exercised with a width above one.
uint32_t alt_init_tables(uint8_t* fold, int8_t* unfold) {
  uint8_t even, odd;
  uint32_t done;
  int32_t in_bucket, bucket;
  uint8_t half;
  uint32_t i, b;
  int32_t bucket_size = 2*near_lossless_q+1;
  *unfold = 0;
  unfold[255] = 0x80;
  for( i = 0; i<0x3F; ++i ) {
    ((uint8_t*)unfold)[4*i+2] = 2*i+1;
    ((uint8_t*)unfold)[4*i+1] = (uint8_t)(-2*i-1);
    ((uint8_t*)unfold)[4*i+4] = 2*i+2;
    ((uint8_t*)unfold)[4*i+3] = (uint8_t)(-2*i-2);
  }
  unfold[254] = 127;
  unfold[253] = -127;
  *fold = 0;
  uint32_t lo = 1;
  fold[128] = (uint8_t)-1;
  {
    uint32_t span = 128-lo;
    uint32_t pairs = (128-lo)/2;
    half = 0;
    in_bucket = 1;
    {
      // `pos[2*k+++1]` reads as `pos[2*k + 1]` and is `pos[2*(k++) + 1]`; the
      // increment is the loop's own.  `(fold)[lo]` had parentheses round a
      // plain array name.
      uint32_t k = 0;
      int32_t ofs = 0;
      uint8_t *pos = &fold[lo];
      uint8_t *neg = fold-lo;
      for( ; k<pairs; ++k, ofs -= 2 ) {
        bucket = bucket_size;
        int32_t bucket_1 = in_bucket-1;
        if( bucket_1 )
          bucket = bucket_1;
        else
          ++half;
        even = 2*half;
        pos[2*k] = even;
        odd = 2*half-1;
        in_bucket = bucket-1;
        neg[ofs+256] = odd;
        if( bucket==1 ) {
          in_bucket = bucket_size;
          even = 2*++half;
          odd = 2*half-1;
        }
        pos[2*k+1] = even;
        neg[ofs+255] = odd;
      }
      // `done = 1` when `pairs` is zero, which is what the `else` arm this
      // replaces assigned: `k` never moves, so `2*k + 1` is already 1.
      done = 2*k+1;
    }
    if( (done-1)<span ) {
      if( in_bucket==1 )
        half = half+1;
      fold[lo-1+done] = 2*half;
      fold[257-lo-done] = 2*half-1;
    }
  }
  for( b = 0; b<0x80; ++b ) {
    (fold)[(uint8_t)((uint8_t*)unfold)[2*b]+256] = 2*b;
    (fold)[(uint8_t)((uint8_t*)unfold)[2*b+1]+256] = 2*b+1;
  }
  return b;
}

// One p1 block, allocated for one plane, and the whole of what the two
// eight-bit entry points do besides naming a direction.  The body they share
// is `AltP1Block::d8_body<f_DEC>`; the decode half used to be written out
// here, twenty-six lines against the encoder's four.
static AltP1Block* alt_p1_block_alloc(int32_t width, int32_t height) {
  AltP1Block* raw = (AltP1Block*)bmf_new(sizeof(AltP1Block));
  return raw ? raw->alt_p1_alloc(width, height, 0) : nullptr;
}

template<int32_t f_DEC>
static void alt_model_p1_d8(uint8_t* src, int32_t width, int32_t height,
                            uint8_t* out) {
  AltP1Block *blk = alt_p1_block_alloc(width, height);
  blk->d8_body<f_DEC>(src, out);
  if( blk )
    blk->alt_p1_free();
}

void alt_model_p1_d8_encode(uint8_t* src, int32_t i, int32_t height, uint8_t* out) {
  alt_model_p1_d8<0>(src, i, height, out);
}

void alt_model_p1_d8_decode(uint8_t* out, int32_t i, int32_t height) {
  alt_model_p1_d8<1>(nullptr, i, height, out);
}
