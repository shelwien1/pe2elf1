// bmp_write.inc -- write one back.

int32_t write_bmp(BmfImage* img, char* path, int32_t want_rle) {
  uint8_t* end;
  int32_t pal_bytes;
  bool can_rle;
  uint8_t *out_at, *p, *q;
  int32_t bits, rle_kind, lit_len, run, coded_bytes;
  uint8_t byte;
  uint32_t pad;
  int32_t rle_on = want_rle;
  FILE *fp = fopen(path, "wb");
  if( !fp )
    return 0;
  uint8_t *out_buf = (uint8_t*)bmf_new(img->data_size+8*img->height+(img->data_size>>5)+2048);
  BmpHeader* bmp = (BmpHeader*)out_buf;
  int32_t i = img->width;
  bmp->biSize = bmp_hdr_v3;
  bmp->bfType = bmp_magic;
  int32_t rows = img->height;
  int32_t rows2 = rows;
  bmp->bfReserved2 = 0;
  bmp->bfReserved1 = 0;
  bmp->biWidth = i;
  bmp->biHeight = rows;
  // The depth byte, whose top two bits pick which of the three palettes below
  // gets written.  It was `int8_t depth_flags`, which named the wrong byte --
  // `flags` is the other one -- and tested `depth_palette` as `depth_flags<0`.
  uint8_t depth = img->depth;
  bmp->biPlanes = 1;
  bits = depth&depth_bits;
  bmp->biBitCount = bits;
  bmp->biClrImportant = 0;
  bmp->biClrUsed = 0;
  bmp->biYPelsPerMeter = 0;
  bmp->biXPelsPerMeter = 0;
  uint8_t *buf = out_buf+54;
  if( bits<=8 ) {
    // The palette's entry count, `1 << bits`.  A depth reaches `write_bmp`
    // only through `expand_image`, which refuses anything whose plane count
    // `((depth & 0x3F) + 7) >> 3` is outside 1..4, so `bits` is 1..32 and here
    // 1..8 -- and `ncol` is 2, 16 or 256.  The two `> 0` tests and the two
    // `/ 2` ones below were tests on that, and the four arms they guarded
    // (`at = 1`, `at2 = 1`/`done = 1`, and a `pal_bytes` with no palette
    // written) cannot
    // be reached.  `hdrscan.sh` puts every one of the 256 depth bytes through
    // this, which is the check the reasoning rests on.
    //
    // The count had two names, `ncolours` and `ncol`, assigned the same
    // expression on consecutive lines and then used interchangeably -- down to
    // `pal_bytes = 4*ncol` in one arm and `4*ncolours` in the next.
    const int32_t ncol = 1<<(bits&31);
    if( depth&depth_grey ) {
      uint32_t levels = 0x100u>>(bits&31);
      uint32_t k = 0;
      int32_t grey = 0;
      do {
        put_u32(&out_buf[8*k+54], bmp_grey_entry((uint8_t)grey));
        put_u32(&out_buf[8*k+58], bmp_grey_entry((uint8_t)(grey+levels)));
        grey += 2*levels;
        ++k;
      } while( k<(uint32_t)(ncol/2) );
      const uint32_t at = 2*k+1;
      if( (at-1)<(uint32_t)ncol )
        put_u32(&out_buf[4*at+50], bmp_grey_entry((uint8_t)(levels*(at-1))));
      pal_bytes = 4*ncol;
    } else if( depth&depth_palette ) {
      uint32_t pairs = ncol/2;
      uint32_t j = 0;
      // `data_ofs2` is `img->data_size` on every path that reaches here, and
      // the palette sits immediately after the pixels -- so this was
      // `img->palette()` with `sizeof(BmfImage)` spelled as a 16 inside each
      // subscript.  Two BGR triples per turn, six bytes.
      const uint8_t *pal = img->palette();
      do {
        uint32_t slot = 2*j;
        const uint8_t *e0 = &pal[6*j], *e1 = &pal[6*j+3];
        put_u32(&out_buf[4*slot+54], bmp_palette_entry(e0[0], e0[1], e0[2]));
        put_u32(&out_buf[4*slot+58], bmp_palette_entry(e1[0], e1[1], e1[2]));
        ++j;
      } while( j<pairs );
      rle_on = want_rle;
      // `done` was `at2` copied on the next line and then read twice below
      // it; the copy existed because the arm this replaced set the two to
      // different things.
      const uint32_t at2 = 2*j+1;
      if( (at2-1)<(uint32_t)ncol ) {
        // The odd colour the loop above did not reach.  `at2` is one past the
        // last pair it wrote, so `3*at2-3` is that entry's first byte.
        const uint8_t *e = img->palette()+3*at2-3;
        put_u32(&out_buf[4*at2+50], bmp_palette_entry(e[0], e[1], e[2]));
      }
      pal_bytes = 4*ncol;
    } else {
      pal_bytes = 4*ncol;
      memset(buf, 0, pal_bytes);
      rows2 = img->height;
    }
    buf = &out_buf[pal_bytes+54];
  }
  uint32_t stride = img->stride;
  // Write the two size fields into the header, put the buffer out, and close.
  // Both exits do this -- the run-length one only when the encoding came out
  // smaller than the raw pixels -- and the second copy reached the file and
  // the buffer through `fp2` and `out2`, which are assigned `fp` and `out_buf`
  // on every path that gets there.
  auto finish = [&](int32_t coded) {
    bmp->biSizeImage = (uint32_t)coded;
    uint32_t n_bytes = (uint32_t)(out_at-out_buf);
    bmp->bfSize = n_bytes;
    if( fwrite(out_buf, 1u, n_bytes, fp)!=bmp->bfSize )
      return 0;
    free(out_buf);
    fclose(fp);
    return 1;
  };
  uint32_t off_bits = (uint32_t)(buf-out_buf);
  int32_t rle_mode = rle_on;
  uint32_t data_len = img->data_size;
  while( 1 ) {
    out_at = buf;
    p = &img->pixels[data_len-stride];
    bmp->bfOffBits = off_bits;
    if( !rle_mode )
      break;
    int32_t src_bits = depth&depth_bits;
    can_rle = src_bits==4;
    if( src_bits!=4 ) {
      if( src_bits!=8 )
        break;
      can_rle = 0;
    }
    rle_kind = bmp_rle4;
    if( !can_rle )
      rle_kind = bmp_rle8;
    bmp->biCompression = rle_kind;
    // Pixels are nibbles rather than bytes, which is the whole difference
    // between the two runs: half the shift, and a run counts to 15 rather
    // than 255.  Both were `rle_kind-1`, arithmetic on a format constant.
    int32_t nib = rle_kind==bmp_rle4;
    int32_t run_max = (0x100u>>(nib&31))-1;
    // Flush the literal run that ends at `run_end`: an escape byte, the count,
    // the bytes themselves, and a pad byte when the count is odd -- or, for a
    // run too short to be worth the two-byte header, one or two encoded pairs
    // instead.
    //
    // Written out three times, and the three differ only in whether the run
    // ends at `p` or at `q` (which are the same pointer at each of the two
    // sites that use `q`) and in which of the output cursor's two names they
    // advanced.  The pair was `buf_2` and `out_at`, the register and the stack
    // slot MSVC spilled it to at every exit of this five-deep nest; there is
    // one cursor now.
    auto flush_literal = [&](const uint8_t* run_end) {
      if( nib ? lit_len!=1 : lit_len>=3 ) {
        *out_at = 0;
        out_at[1] = lit_len<<(nib&31);
        memcpy(out_at+2, run_end-lit_len, lit_len);
        if( lit_len&1 )
          out_at[lit_len+2] = 0;
        out_at += lit_len+2+(lit_len&1);
      } else if( nib ) {
        *out_at = 2;
        out_at[1] = run_end[-1];
        out_at += 2;
      } else {
        if( lit_len==2 ) {
          *out_at = 1;
          out_at[1] = run_end[-2];
          out_at += 2;
        }
        *out_at = 1;
        out_at[1] = run_end[-1];
        out_at += 2;
      }
    };
    // How many bytes from `from` repeat `*from`, counting `from` itself and
    // stopping at `stop`.  The caller has already established `from + 1 < stop`,
    // which is why the recovered loop tested the byte before the bound and this
    // tests the bound before the byte -- at the first turn they are one test.
    //
    // Not `at`/`end`: this body already has locals by both names, and a shadow
    // is what `BMF_WARN=1 ./build.sh` counts.
    auto run_length = [](const uint8_t* from, const uint8_t* stop) {
      int32_t n = 1;
      while( stop>from+n&&from[n]==*from )
        ++n;
      return n;
    };
    // Emit repeat pairs from `p` for as long as the next run is worth more
    // than the literal it would otherwise join, flushing that literal first
    // when there is one.  Returns true when the row ended on a pair boundary,
    // which is the one exit that has to leave the loop *outside* this one --
    // said by a `goto` past both in the recovery, and the reason this was two
    // `while( 1 )`s deep before anything in it happened.
    auto emit_runs = [&]() {
      while( 1 ) {
        q = p+1;
        if( p+1>=end )
          return false;
        byte = (uint8_t)*p;
        run = run_length(p, end);
        if( run<=2&&(run!=2||lit_len) )
          return false;
        if( run_max<run )
          run = run_max;
        if( lit_len ) {
          flush_literal(p);
          lit_len = 0;
          byte = *p;
        }
        out_at[1] = byte;
        p += run;
        *out_at = run<<(nib&31);
        out_at += 2;
        if( p>=end )
          return true;
      }
    };
    // One row of opcodes: runs and literals until the row runs out.  Three
    // paths end a row -- the row is empty, `emit_runs` reached the end on a
    // pair boundary, or a maximum-length literal ended there -- and each used
    // to be a `goto` two loops out to the end-of-line pair below, setting
    // `row_i`, `rows3` and `stride3` on the way as the row loop's three
    // arguments, passed by assignment because the decompiler had no function to
    // pass them to.  Two of the three were `img->height` and `img->stride`
    // re-read under another name, and nothing in this body writes either field,
    // so eight assignments across four exits said nothing.  `row_i` is the one
    // that carries, and it is the row loop's own counter.
    auto emit_row = [&]() {
      end = &p[stride];
      if( p>=end )
        return;
      lit_len = 0;
      do {
        while( 1 ) {
          if( emit_runs() )
            return;
          ++p;
          if( ++lit_len!=run_max )
            break;
          flush_literal(q);
          if( q>=end )
            return;
          lit_len = 0;
        }
      } while( q<end );
      if( lit_len )
        flush_literal(q);
    };
    if( rows2>0 ) {
      for( int32_t row_i = 0; row_i<rows2; ++row_i ) {
        emit_row();
        // End of line, then back down two rows: one for the row just written
        // and one because a BMP's rows run bottom-up while the cursor walks
        // forward through them.
        *out_at = 0;
        out_at[1] = 0;
        out_at += 2;
        p -= 2*stride;
      }
      data_len = img->data_size;
    }
    *out_at = 0;
    out_at[1] = 1;
    out_at += 2;
    coded_bytes = out_at-buf;
    if( data_len>(uint32_t)(coded_bytes) )
      return finish(coded_bytes);
    rle_mode = 0;
  }
  bmp->biCompression = bmp_rgb;
  if( rows2<=0 ) {
    coded_bytes = 0;
  } else {
    pad = ((stride+3)&0xFFFFFFFC)-stride;
    int32_t y = 0;
    do {
      memcpy(out_at, p, stride);
      out_at += stride;
      p -= stride;
      if( pad ) {
        *(uint32_t*)out_at = 0;
        out_at += pad;
      }
      ++y;
    } while( y<img->height );
    coded_bytes = out_at-buf;
  }
  return finish(coded_bytes);
}
