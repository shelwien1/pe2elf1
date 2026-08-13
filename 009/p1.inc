// p1.inc -- the one-bit plane model, around its block.

uint32_t __alt_init_tables(uint8_t* fold, int8_t* unfold) {
  uint8_t even, odd;
  uint32_t done, lo;
  int32_t bucket_size, in_bucket, ofs, bucket, bucket_1;
  uint8_t half;
  uint32_t i, k, b, span, pairs;
  uint8_t* pos, * neg;
  bucket_size = 2*near_lossless_q+1;
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
  lo = 1;
  fold[128] = (uint8_t)-1;
  {
    span = 128-lo;
    pairs = (128-lo)/2;
    half = 0;
    in_bucket = 1;
    if( pairs ) {
      k = 0;
      ofs = 0;
      pos = &(fold)[lo];
      neg = fold-lo;
      do {
        bucket = bucket_size;
        bucket_1 = in_bucket-1;
        if( bucket_1 )
          bucket = bucket_1;
        else
          ++half;
        even = 2*half;
        pos[2*k] = 2*half;
        odd = 2*half-1;
        in_bucket = bucket-1;
        neg[ofs+256] = odd;
        if( bucket==1 ) {
          in_bucket = bucket_size;
          even = 2*++half;
          odd = 2*half-1;
        }
        pos[2*k+++1] = even;
        neg[ofs+255] = odd;
        ofs -= 2;
      } while( k<pairs );
      done = 2*k+1;
    } else {
      done = 1;
    }
    if( (done-1)<span ) {
      if( in_bucket==1 )
        half = half+1;
      (fold)[lo-1+done] = 2*half;
      fold[257-lo-done] = 2*half-1;
    }
  }
  for( b = 0; b<0x80; ++b ) {
    (fold)[(uint8_t)((uint8_t*)unfold)[2*b]+256] = 2*b;
    (fold)[(uint8_t)((uint8_t*)unfold)[2*b+1]+256] = 2*b+1;
  }
  return b;
}

void __alt_model_p1_d8_encode(uint8_t* src, int32_t i, int32_t height, uint8_t* out) {
  AltP1Block* raw, * blk;
  raw = (AltP1Block*)bmf_new(sizeof(AltP1Block));
  if( raw )
    blk = raw->alt_p1_alloc(i, height, 0);
  else
    blk = nullptr;
  blk->d8_encode_body(src, out);
  if( blk )
    blk->alt_p1_free(1);
}

void**__alt_model_p1_d8_decode(int8_t unread_flag, uint8_t* out, int32_t i, int32_t height) {
  P1Ctx* buf1, * b0, * cursor0, * b4, * buf3, * buf2;
  AltP1Block* blk, * raw;
  int64_t err;
  uint8_t* out_at;
  int32_t y, val, x, code;
  P1Ctx* cursor2, * cursor4;
  raw = (AltP1Block*)bmf_new(sizeof(AltP1Block));
  if( raw )
    blk = raw->alt_p1_alloc(i, height, 0);
  else
    blk = nullptr;
  __rc_begin_decode(unread_flag);
  if( blk->height>0 ) {
    y = 0;
    do {
      ++y;
      {
        P1Ctx*const here = (P1Ctx*)blk->cursor[0];
        here[0] = here[-1];
        here[1] = here[-2];
        here[2] = here[-3];
        here[3] = here[-4];
        here[4] = here[-5];
        here[5] = here[-6];
      }
      b4 = blk->buf[4];
      buf3 = blk->buf[3];
      buf2 = blk->buf[2];
      buf1 = blk->buf[1];
      b0 = blk->buf[0];
      blk->buf[4] = buf3;
      blk->buf[3] = buf2;
      blk->buf[2] = buf1;
      blk->buf[1] = b0;
      blk->buf[0] = b4;
      b4 += 4;
      blk->cursor[0] = b4;
      b0 += 4;
      blk->cursor[1] = b0;
      blk->cursor[2] = buf1+4;
      blk->cursor[3] = buf2+4;
      blk->cursor[4] = buf3+4;
      ((P1Ctx*)b4)[-4] = ((P1Ctx*)b0)[3];
      {
        P1Ctx*const here = (P1Ctx*)blk->cursor[0];
        P1Ctx*const up = (P1Ctx*)blk->cursor[1];
        here[-3] = up[2];
        here[-2] = up[1];
        here[-1] = up[0];
      }
      cursor2 = blk->cursor[2];
      cursor4 = blk->cursor[4];
      blk->ctx[2] = 0;
      cursor0 = blk->cursor[0];
      blk->ctx[3] = cursor2[-2].mag+cursor4[-2].mag+cursor2[0].mag+cursor4[0].mag+cursor2[2].mag+cursor4[2].mag+cursor2[4].mag+cursor4[4].mag+cursor0[-4].mag+cursor0[-2].mag;
      blk->ctx[4] = cursor2[-1].mag+cursor4[-1].mag+cursor2[1].mag+cursor4[1].mag+cursor2[3].mag+cursor4[3].mag+cursor2[5].mag+cursor4[5].mag+cursor0[-3].mag+cursor0[-1].mag;
      if( !(blk->width<=0) ) {
        out_at = out;
        x = 0;
        do {
          ++x;
          blk->ctx_of((AltP1Block*)nullptr, (AltP1Block*)0);
          code = __alt_p1_decode_symbol((uint16_t*)&blk->counters[blk->ctx[0]], 0, blk->ctx[1]);
          val = (uint8_t)((uint8_t)blk->pred+blk->unfold[(uint8_t)code]);
          *out_at = val;
          err = val-blk->pred;
          blk->cursor[0]->sym = val;
          blk->cursor[0]->mag = (BYTE4(err)^err)-BYTE4(err);
          blk->ctx[3+blk->ctx[2]] = blk->ctx[3+blk->ctx[2]]+blk->cursor[0]->mag-blk->cursor[0][-4].mag-(blk->cursor[4][-2].mag-blk->cursor[4][6].mag+blk->cursor[2][-2].mag-blk->cursor[2][6].mag);
          blk->ctx[2] = blk->ctx[2]==0;
          if( blk->counters[blk->ctx[0]].total<0x4000u )
            (blk)->update_model();
          ++blk->cursor[0];
          ++out_at;
          ++blk->cursor[1];
          ++blk->cursor[2];
          ++blk->cursor[3];
          ++blk->cursor[4];
        } while( x<blk->width );
        out = out_at;
      }
    } while( y<blk->height );
  }
  __rc_end_decode();
  return (void**)blk->alt_p1_free(1);
}
