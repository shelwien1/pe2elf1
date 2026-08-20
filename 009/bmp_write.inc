// bmp_write.inc -- write one back.

// The palette, and how many bytes of it there are.  Three arms: a greyscale
// ramp of `1 << bits` levels, the image's own palette copied out as BGR quads,
// or zeros -- which of the three is the top two bits of the depth byte.  Zero
// for a depth above eight bits, which has no palette.
//
// Sixty lines in the middle of `write_bmp` between the header fields and the
// pixels, reading only the depth and writing only the bytes at offset 54.  A
// stage of that function and not a step in it.
//
// The middle arm carried `rle_on = want_rle;` between two palette stores, a
// reload of a parameter into the local that already held it, with nothing
// between the two touching either.  It went with the move: there is no
// `want_rle` here to reload from.
static int32_t write_bmp_palette(const BmfImage* img, uint8_t* out_buf,
                                 uint8_t depth, int32_t bits) {
  if( bits>8 )
    return 0;

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
    // `ncol` is 2, 16 or 256, so the half is at least one and the `do`'s
    // guaranteed first turn was never load-bearing.  `k` outlives the loop:
    // the odd entry below reads where it stopped.
    uint32_t k = 0;
    int32_t grey = 0;
    for( ; k<(uint32_t)(ncol/2); ++k, grey += 2*levels ) {
      put_u32(&out_buf[8*k+54], bmp_grey_entry((uint8_t)grey));
      put_u32(&out_buf[8*k+58], bmp_grey_entry((uint8_t)(grey+levels)));
    }
    const uint32_t at = 2*k+1;
    if( (at-1)<(uint32_t)ncol )
      put_u32(&out_buf[4*at+50], bmp_grey_entry((uint8_t)(levels*(at-1))));
    return 4*ncol;
  } else if( depth&depth_palette ) {
    uint32_t pairs = ncol/2;
    uint32_t j = 0;
    // `data_ofs2` is `img->data_size` on every path that reaches here, and
    // the palette sits immediately after the pixels -- so this was
    // `img->palette()` with `sizeof(BmfImage)` spelled as a 16 inside each
    // subscript.  Two BGR triples per turn, six bytes.
    const uint8_t *pal = img->palette();
    for( ; j<pairs; ++j ) {
      const uint32_t slot = 2*j;
      const uint8_t *e0 = &pal[6*j], *e1 = &pal[6*j+3];
      put_u32(&out_buf[4*slot+54], bmp_palette_entry(e0[0], e0[1], e0[2]));
      put_u32(&out_buf[4*slot+58], bmp_palette_entry(e1[0], e1[1], e1[2]));
    }
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
    return 4*ncol;
  } else {
    return 4*ncol;
    memset(out_buf+54, 0, 4*ncol);
    }
}

// The pixels, run-length encoded, written at `buf`.  Returns the cursor past the
// last opcode and sets `*kind` to `bmp_rle4` or `bmp_rle8`, or returns null when
// this depth has no run-length form.
//
// Whether the result is *kept* is the caller's question and not this one's: an
// encoding can come out no smaller than the pixels it replaces, and then the raw
// form wins.  That retry was a `while( 1 )` wrapped around this whole body with
// a `rle_mode = 0` at the bottom of it -- a loop with at most two turns, written
// as an unbounded one, with the raw path reached by a `break` from three places.
// With the encoder a function the caller asks the question once and the loop is
// gone.
//
// `data_len = img->data_size;` sat after the row loop, assigning the value the
// local had held since before the loop with nothing between touching either.  It
// was the second turn's reload and there is no second turn.
static uint8_t* bmp_rle_encode(BmfImage* img, int32_t bits, uint8_t* buf,
                               int32_t rows, uint32_t stride, int32_t* kind) {
  // BMP defines a run-length form for four- and eight-bit pixels and for
  // nothing else, which the original said through a `can_rle` flag set, tested,
  // cleared and tested again, with `rle_kind` derived from the flag and `nib`
  // derived back from `rle_kind` -- four names for one bit.
  //
  // Pixels are nibbles rather than bytes is the whole difference between the two
  // forms: half the shift, and a run counts to 15 rather than 255.
  if( bits!=4&&bits!=8 )
    return nullptr;
  const int32_t nib = bits==4;
  const int32_t run_max = (0x100u>>(nib&31))-1;
  *kind = nib ? bmp_rle4 : bmp_rle8;
  uint8_t *out_at = buf, *q = nullptr, *end = nullptr;
  uint8_t* p = &img->pixels[img->data_size-stride];
  uint8_t byte = 0;
  int32_t lit_len = 0, run = 0;
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
      run = min32(run, run_max);
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
  if( rows>0 ) {
    for( int32_t row_i = 0; row_i<rows; ++row_i ) {
      emit_row();
      // End of line, then back down two rows: one for the row just written
      // and one because a BMP's rows run bottom-up while the cursor walks
      // forward through them.
      *out_at = 0;
      out_at[1] = 0;
      out_at += 2;
      p -= 2*stride;
    }
  }
  *out_at = 0;
  out_at[1] = 1;
  return out_at+2;
}

int32_t write_bmp(BmfImage* img, char* path, int32_t want_rle) {
  uint8_t* out_at;
  int32_t bits, coded_bytes;
  FILE *fp = fopen(path, "wb");
  if( !fp )
    return 0;
  uint8_t *out_buf = (uint8_t*)bmf_new(img->data_size+8*img->height+(img->data_size>>5)+2048);
  BmpHeader* bmp = (BmpHeader*)out_buf;
  bmp->biSize = bmp_hdr_v3;
  bmp->bfType = bmp_magic;
  int32_t rows = img->height;
  bmp->bfReserved2 = 0;
  bmp->bfReserved1 = 0;
  bmp->biWidth = img->width;
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
  // The pixels start after the header and whatever palette there was.
  const int32_t pal_bytes = write_bmp_palette(img, out_buf, depth, bits);
  uint8_t*const buf = &out_buf[pal_bytes+54];
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
  const uint32_t data_len = img->data_size;
  bmp->bfOffBits = off_bits;
  out_at = buf;
  uint8_t* p = &img->pixels[data_len-stride];
  // The run-length form when there is one and it is smaller; the raw pixels
  // otherwise.  Two questions, and the original asked them as one unbounded
  // loop -- see the note at `bmp_rle_encode`.
  if( want_rle ) {
    int32_t rle_kind = 0;
    uint8_t*const rle_end = bmp_rle_encode(img, bits, buf, rows, stride, &rle_kind);
    if( rle_end&&data_len>(uint32_t)(rle_end-buf) ) {
      bmp->biCompression = rle_kind;
      out_at = rle_end;
      return finish(out_at-buf);
    }
  }
  bmp->biCompression = bmp_rgb;
  if( rows<=0 ) {
    coded_bytes = 0;
  } else {
    const uint32_t pad = ((stride+3)&0xFFFFFFFC)-stride;
    // Guarded by `rows > 0` above, and `rows` is `img->height`.
    for( int32_t y = 0; y<img->height; ++y ) {
      memcpy(out_at, p, stride);
      out_at += stride;
      p -= stride;
      if( pad ) {
        *(uint32_t*)out_at = 0;
        out_at += pad;
      }
    }
    coded_bytes = out_at-buf;
  }
  return finish(coded_bytes);
}
