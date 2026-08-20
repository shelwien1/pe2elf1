// bmp_read.inc -- read a BMP, in every depth and both run-length forms.

// One run-length opcode: a count and a value.  False on a stream error or an
// end of file, which both arms answer the same way and neither can tell apart
// from a legitimate byte without this check -- `fgetc` returns `int` for that
// reason.  Five lines at the top of each of the two loops.
static bool read_rle_op(FILE* fp, int32_t& n, int32_t& v) {
  if( ferror(fp) )
    return false;
  n = fgetc(fp);
  v = fgetc(fp);
  return n>=0&&v>=0;
}

// An absolute run of `n` nibbles, read a byte at a time from `src` and written
// from the half of `*at` that `hi` names.  Answers where the run ended, leaving
// `hi` saying which half of that byte the next nibble goes in.
//
// The parity does not change inside the run -- a turn writes a whole byte -- so
// the two `hi_nibble = ` stores in the middle of the recovered loop stored what
// was already there.  Lifting the loop out is what takes `read_bmp` off a
// nesting depth of seven; the three exits were three copies of "save the
// cursor" and are the return.
static uint8_t* write_nibbles(uint8_t* at, int32_t& hi, const uint8_t* src,
                              uint32_t n) {
  while( 1 ) {
    const uint8_t pix = *src;
    const uint32_t left = n-1;
    if( hi ) {
      if( !left ) {
        *at = *src&0xF0;
        hi = 0;
        return at;
      }
      *at++ = pix;
    } else {
      *at++ |= (uint8_t)*src>>4;
      if( !left ) {
        hi = 1;
        return at;
      }
      *at = 16*(pix&0xF);
    }
    ++src;
    n = left-1;
    if( !n )
      return at;
  }
}

// The RLE8 opcode stream.  True when it ended -- by the end-of-bitmap escape or
// by running off the top row -- and false on a short read or an opcode that
// would write outside the image.
//
// Both endings were a `goto done` five frames up, to the one place that closes
// the file and gives the scratch back; false is the `return nullptr` that was
// already written five times here and leaves both exactly as it found them.
//
// This and `read_rle4` share a shape and share it as far as it goes: one opcode
// a turn through `read_rle_op`, then a five-way branch, and the two arms that
// are the same in both -- end of line and end of bitmap -- are the same three
// lines.  The other three are not shared and cannot be: a run, a delta and an
// absolute block each carry the nibble/byte difference in their arithmetic, and
// the only way to fold them would be a callback per arm, which is the dispatch
// written twice with an indirection over it.
//
// `write_nibbles` has no counterpart on the writing side either.  A 4-bit image
// is stored *packed*, two pixels to a byte -- which is what `run4/2 + 1` and
// `dxy >> 1` in `read_rle4` are about -- so `write_bmp` copies bytes and never
// has a nibble to lay down.  The helper unpacks an RLE4 absolute run into that
// packed buffer; there is nothing in the other direction for it to do.
static bool read_rle8(FILE* fp, BmfImage* img, uint8_t* at, void* pal_buf) {
  int32_t y = img->height-1;
  int32_t n, v;
  while( read_rle_op(fp, n, v) ) {
    if( n ) {
      if( !img->holds(at, n) )
        return false;
      __builtin_memset(at, v, n);
      at += n;
    } else if( !v ) {
      if( --y<0 )
        return true;
      at = img->row(y);
    } else if( v==1 ) {
      return true;
    } else if( v==2 ) {
      // A delta is two more bytes of the same opcode, so it is the same read.
      // It picks up a `ferror` check the recovery did not have here: after a
      // read error `fgetc` answers EOF, which is the -1 this already refused,
      // so the two agree.
      int32_t dx, dy;
      if( !read_rle_op(fp, dx, dy) )
        return false;
      at += dx-dy*img->stride;
    } else {
      if( !img->holds(at, v) )
        return false;
      if( !read_exact(pal_buf, (v+1)&0xFFFFFFFE, fp) )
        return false;
      memcpy(at, (uint8_t*)pal_buf, v);
      at += v;
    }
  }
  return false;
}

// The RLE4 opcode stream, which is RLE8's with a nibble for a pixel: half the
// horizontal step, a run counted in nibbles, and a cursor that carries which
// half of `*row` the next one goes in.
//
// One opcode per turn, which is what RLE8 above already looked like.  This was
// three `while( 1 )`s with a label at the top of the innermost and five jumps
// back to it, and the nesting *was* the dispatch: fall out of the innermost
// loop for an escape, out of the middle one for anything but an end of line,
// out of the outer one for anything but a delta.  Reading it meant holding
// three loops in mind to see a five-way branch on two bytes.
static bool read_rle4(FILE* fp, BmfImage* img, uint8_t* row, void* pal_buf) {
  int32_t hi_nibble = 1;
  int32_t y = img->height-1;
  int32_t run4, byte_in;
  while( read_rle_op(fp, run4, byte_in) ) {
    const uint32_t byte = (uint32_t)byte_in;
    if( run4 ) {
      if( !img->holds(row, run4/2+1) )
        return false;
      uint8_t lo = byte&0xF;
      // `run4` nibbles of the two-nibble pattern in `byte`, from the half of
      // `*row` that `hi_nibble` names.  Both arms ended twice -- once by
      // falling out of their loop and once by a jump back to the top of the
      // one above -- and those are the same exit.  What told them apart is
      // the parity of `run4`, which is what each arm's counter is left
      // holding.
      if( hi_nibble ) {
        // Byte-aligned, so every full byte is `byte` itself.  The counter
        // steps by two and stops at 0 or 1.
        int32_t left4b = run4;
        uint8_t *row3 = row;
        while( left4b>1 ) {
          *row3++ = byte;
          left4b -= 2;
        }
        row = row3;
        hi_nibble = left4b==0;
        if( left4b )
          *row3 = byte&0xF0;
      } else {
        // Half a byte out of step: each byte written is the low nibble of
        // the previous pattern beside the high nibble of the next, so the
        // first byte carries whatever `*row` already held and the last may
        // be half a byte on its own.  Here the counter ends at 0 or -1.
        int32_t left4 = run4;
        uint8_t *row4 = row;
        uint8_t cur = *row;
        uint32_t hi = byte>>4;
        uint8_t lo16 = 16*lo;
        do {
          *row4++ = hi|cur;
          cur = lo16;
          left4 -= 2;
        } while( left4>0 );
        row = row4;
        hi_nibble = left4<0;
        if( !hi_nibble )
          *row4 = lo16;
      }
    } else if( !byte ) {
      if( --y<0 )
        return true;
      row = img->row(y);
    } else if( byte==1 ) {
      return true;
    } else if( byte==2 ) {
      int32_t dxy, dy4;
      if( !read_rle_op(fp, dxy, dy4) )
        return false;
      // Half the horizontal step, because a column here is a nibble.
      int32_t step = (dxy>>1)-dy4*img->stride;
      if( (dxy&1)==1 ) {
        if( !hi_nibble )
          ++step;
        hi_nibble = !hi_nibble;
      }
      row += step;
    } else {
      // An absolute run of `byte` nibbles, read a byte at a time and split
      // across whichever half of the output `hi_nibble` names.
      if( !img->holds(row, (int32_t)byte/2+1) )
        return false;
      if( !read_exact(pal_buf, (((byte+1)>>1)+1)&0xFFFFFFFE, fp) )
        return false;
      row = write_nibbles(row, hi_nibble, (const uint8_t*)pal_buf, byte);
    }
  }
  return false;
}

// Uncompressed rows, bottom-up, each padded out to a multiple of four bytes.
//
// The stride was re-read from the header after every call in the recovered
// loop -- `stride = (uint16_t)img_f->stride`, twice a turn -- which is one
// value under a spill and a truncation.  Neither says anything: `fread` and
// `fseek` do not write the header, and the accepted-format check one screen up
// bounds a row to 65535 bytes, so the cast cannot narrow it.
static bool read_rows(FILE* fp, BmfImage* img, uint8_t* row, uint32_t stride_pad) {
  const uint32_t stride = img->stride;
  const uint32_t row_pad = stride_pad-stride;
  for( int32_t y = img->height-1; y>=0; --y ) {
    if( fread(row, 1u, stride, fp)!=stride )
      return false;
    if( row_pad )
      fseek(fp, row_pad, 1);
    row -= stride;
  }
  return true;
}

BmfImage* read_bmp(char* path) {
  uint8_t bmp_bgra[4];
  int32_t pal_n, i;
  BmpHeader hdr;
  // Stage-2 11.4.  `bmf_compress` used to open this path, close it again and
  // then call here, purely so that "cannot open" could exit 1 (`bmf_no_open`)
  // while "not a BMP" exits 3 (`bmf_read_error`) -- the two are the same
  // `return nullptr` from here, so the probe was the only thing that told them
  // apart.  Deleting the probe without this line would have changed the exit
  // code for a missing file, which is what makes the caveat worth having.
  FILE* fp = fopen(path, "rb");
  if( !fp )
    bmf_fatal(bmf_no_open, path);
  if( fread(&hdr.bfType, 0xEu, 1u, fp)!=1||hdr.bfType!=bmp_magic||fread(&hdr.biSize, 0x28u, 1u, fp)!=1||hdr.biSize!=bmp_hdr_v3||hdr.biPlanes!=1 ) {
    fclose(fp);
    return nullptr;
  }
  {
    const int32_t bmp_w = hdr.biWidth;
    const int32_t bmp_h = hdr.biHeight;
    const int32_t bmp_bpp = hdr.biBitCount;
    // Everything this reader will not take, one reason per line.  It was one
    // 358-character condition, which is the only statement of the accepted
    // format anywhere -- `MODELS.md` had to reconstruct the list from it by
    // hand.
    if( bmp_w<=0||bmp_w>0xFFFF                                  // width 1..65535
     || bmp_h<=0||bmp_h>0xFFFF                                  // height likewise
     || (bmp_bpp>=8&&(uint32_t)bmp_w*(uint32_t)((bmp_bpp+7)>>3)>0xFFFFu)
                                                                // and a row of it fits in 16 bits
     || (bmp_bpp!=1&&bmp_bpp!=4&&bmp_bpp!=8&&bmp_bpp!=24&&bmp_bpp!=32)
                                                                // 1, 4, 8, 24 or 32 -- no 16
     || (int32_t)hdr.biClrUsed<0                                // a palette count, not a sign
     || (bmp_bpp<=8&&(int32_t)hdr.biClrUsed>(1<<bmp_bpp))       // no more entries than the depth allows
     || hdr.biCompression>bmp_rle4                              // none, RLE8 or RLE4
     || (hdr.biCompression==bmp_rle8&&bmp_bpp!=8)               // RLE8 is 8 bpp only
     || (hdr.biCompression==bmp_rle4&&bmp_bpp!=4) ) {           // RLE4 is 4 bpp only
      fclose(fp);
      return nullptr;
    }
  }
  BmfImage* img = alloc_image(hdr.biWidth, hdr.biHeight, hdr.biBitCount, hdr.biBitCount<=8u, 1);
  if( !img ) {
    fclose(fp);
    return nullptr;
  }
  uint32_t stride_pad = (img->stride+3)&0xFFFFFFFC;
  if( hdr.biBitCount<=8u ) {
    pal_n = 1<<(hdr.biBitCount&31);
    if( (int32_t)hdr.biClrUsed )
      pal_n = (int32_t)hdr.biClrUsed;
    if( pal_n>0 ) {
      // The palette sits past the pixels, and the recovery worked that out
      // again before each of the three stores -- three names, `pal`, `pal2`
      // and `pal3`, for one pointer MSVC rematerialised rather than kept in a
      // register.  It does not depend on `i` either, so it is one line above
      // the loop.
      //
      // The null it takes when the depth carries no palette is the original's:
      // storing through it would fault either way, and nothing that reaches
      // here has `biBitCount <= 8` without `depth_palette`.
      uint8_t* pal = img->depth&depth_palette ? img->pixels+img->data_size : nullptr;
      for( i = 0; i<pal_n; ++i ) {
        if( !read_exact(bmp_bgra, 4u, fp) ) {
          fclose(fp);
          return nullptr;
        }
        pal[3*i+2] = bmp_bgra[2];
        pal[3*i+1] = bmp_bgra[1];
        pal[3*i] = bmp_bgra[0];
      }
    }
  }
  void* pal_buf = bmf_new(max32(stride_pad, 256));
  uint8_t *row = img->pixels+img->data_size-img->stride;
  fseek(fp, (int32_t)hdr.bfOffBits, 0);
  // Three ways to read the pixels, and each ended by jumping five frames out to
  // one tail that closes the file and gives the scratch back.  That tail is
  // here; each walk answers whether it got to the end of the bitmap.
  // Both run-length forms leave the pixels they never reach untouched, so both
  // cleared the buffer first; the uncompressed walk writes every byte.
  bool ok;
  if( hdr.biCompression==bmp_rgb ) {
    ok = read_rows(fp, img, row, stride_pad);
  } else {
    memset(img->pixels, 0, img->data_size);
    ok = hdr.biCompression==bmp_rle8 ? read_rle8(fp, img, row, pal_buf)
       : hdr.biCompression==bmp_rle4 ? read_rle4(fp, img, row, pal_buf)
       : false;
  }
  if( !ok )
    return nullptr;
  fclose(fp);
  free(pal_buf);
  return img;
}
