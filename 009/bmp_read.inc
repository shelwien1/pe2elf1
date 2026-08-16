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

BmfImage* read_bmp(char* path) {
  BmfImage* img_f;
  uint8_t* row, bmp_bgra[4], pix;
  uint8_t* row_at;
  uint32_t row_pad;
  uint8_t cur;
  int32_t pal_n, i, hi_nibble;
  uint32_t pair, stride, left;
  BmpHeader hdr;
  FILE* fp = fopen(path, "rb");
  if( !fp||fread(&hdr.bfType, 0xEu, 1u, fp)!=1||hdr.bfType!=bmp_magic||fread(&hdr.biSize, 0x28u, 1u, fp)!=1||hdr.biSize!=bmp_hdr_v3||hdr.biPlanes!=1 ) {
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
        fread(bmp_bgra, 4u, 1u, fp);
        pal[3*i+2] = bmp_bgra[2];
        pal[3*i+1] = bmp_bgra[1];
        pal[3*i] = bmp_bgra[0];
      }
    }
  }
  void* pal_buf = bmf_new(stride_pad<256 ? 256 : stride_pad);
  row = img->pixels+img->data_size-img->stride;
  fseek(fp, (int32_t)hdr.bfOffBits, 0);
  if( hdr.biCompression!=bmp_rgb ) {
    uint8_t*const pix_lo = img->pixels;
    uint8_t*const pix_hi = img->pixels+img->data_size;
    if( hdr.biCompression==bmp_rle8 ) {
      memset(img->pixels, 0, img->data_size);
      row_at = row;
      img_f = img;
      int32_t y4 = img->height-1;
      while( 1 ) {
        uint8_t *row_ofs = row_at;
        int32_t run, run_val;
        if( !read_rle_op(fp, run, run_val) )
          return nullptr;
        if( run ) {
          if( row_ofs<pix_lo||row_ofs+run>pix_hi )
            return nullptr;
          __builtin_memset(row_ofs, run_val, run);
          row_at = row_ofs+run;
        } else if( run_val ) {
          if( run_val==1 )
            goto done;
          if( run_val==2 ) {
            int32_t dx = fgetc(fp);
            int32_t dy = fgetc(fp);
            if( dx<0||dy<0 )
              return nullptr;
            row_at += dx-dy*(uint16_t)img_f->stride;
          } else {
            if( row_at<pix_lo||row_at+run_val>pix_hi )
              return nullptr;
            fread(pal_buf, (run_val+1)&0xFFFFFFFE, 1u, fp);
            memcpy(row_at, (uint8_t*)pal_buf, run_val);
            row_at += run_val;
          }
        } else {
          if( --y4<0 )
            goto done;
          row_at = img_f->pixels+y4*(uint16_t)img_f->stride;
        }
      }
    }
    if( hdr.biCompression!=bmp_rle4 )
      return nullptr;
    memset(img->pixels, 0, img->data_size);
    img_f = img;
    hi_nibble = 1;
    int32_t y8 = img->height-1;
    // One opcode per turn, which is what RLE8 above already looked like.  This
    // was three `while( 1 )`s with a label at the top of the innermost and five
    // jumps back to it, and the nesting *was* the dispatch: fall out of the
    // innermost loop for an escape, out of the middle one for anything but an
    // end of line, out of the outer one for anything but a delta.  Reading it
    // meant holding three loops in mind to see a five-way branch on two bytes.
    while( 1 ) {
      int32_t run4, byte_in;
      if( !read_rle_op(fp, run4, byte_in) )
        return nullptr;
      const uint32_t byte = (uint32_t)byte_in;
      pair = byte;
      if( run4 ) {
        if( row<pix_lo||row+(run4/2+1)>pix_hi )
          return nullptr;
        uint8_t lo = byte&0xF;
        // `run4` nibbles of the two-nibble pattern in `pair`, from the half of
        // `*row` that `hi_nibble` names.  Both arms ended twice -- once by
        // falling out of their loop and once by a jump back to the top of the
        // one above -- and those are the same exit.  What told them apart is
        // the parity of `run4`, which is what each arm's counter is left
        // holding.
        if( hi_nibble ) {
          // Byte-aligned, so every full byte is `pair` itself.  The counter
          // steps by two and stops at 0 or 1.
          int32_t left4b = run4;
          uint8_t *row3 = row;
          while( left4b>1 ) {
            *row3++ = pair;
            left4b -= 2;
          }
          row = row3;
          hi_nibble = left4b==0;
          if( left4b )
            *row3 = pair&0xF0;
        } else {
          // Half a byte out of step: each byte written is the low nibble of
          // the previous pattern beside the high nibble of the next, so the
          // first byte carries whatever `*row` already held and the last may
          // be half a byte on its own.  Here the counter ends at 0 or -1.
          int32_t left4 = run4;
          uint8_t *row4 = row;
          cur = *row;
          uint32_t hi = pair>>4;
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
        if( --y8<0 )
          goto done;
        row = img_f->pixels+y8*(uint16_t)img_f->stride;
      } else if( byte==1 ) {
        goto done;
      } else if( byte==2 ) {
        int32_t dxy = fgetc(fp);
        int32_t dy4 = fgetc(fp);
        if( dxy<0||dy4<0 )
          return nullptr;
        int32_t step = (dxy>>1)-dy4*(uint16_t)img_f->stride;
        if( (dxy&1)==1 ) {
          if( !hi_nibble )
            ++step;
          hi_nibble = !hi_nibble;
        }
        row += step;
      } else {
        // An absolute run of `byte` nibbles, read a byte at a time and split
        // across whichever half of the output `hi_nibble` names.
        if( row<pix_lo||row+((int32_t)byte/2+1)>pix_hi )
          return nullptr;
        fread(pal_buf, (((byte+1)>>1)+1)&0xFFFFFFFE, 1u, fp);
        uint8_t *pal_at = (uint8_t*)pal_buf;
        uint8_t *row5 = row;
        while( 1 ) {
          pix = *pal_at;
          if( hi_nibble ) {
            left = pair-1;
            if( !left ) {
              row = row5;
              *row5 = *pal_at&0xF0;
              hi_nibble = 0;
              break;
            }
            *row5++ = pix;
            hi_nibble = 1;
          } else {
            *row5++ |= (uint8_t)*pal_at>>4;
            left = pair-1;
            if( !left ) {
              row = row5;
              hi_nibble = 1;
              break;
            }
            *row5 = 16*(pix&0xF);
            hi_nibble = 0;
          }
          ++pal_at;
          pair = left-1;
          if( !pair ) {
            row = row5;
            break;
          }
        }
      }
    }
  }
  stride = img->stride;
  row_pad = stride_pad-stride;
  if( hdr.biHeight-1>=0 ) {
    int32_t y = hdr.biHeight-1;
    img_f = img;
    uint8_t *row6 = row;
    while( 1 ) {
      uint32_t got = fread(row6, 1u, stride, fp);
      stride = (uint16_t)img_f->stride;
      if( got!=stride )
        return nullptr;
      if( row_pad ) {
        fseek(fp, row_pad, 1);
        stride = (uint16_t)img_f->stride;
      }
      row6 -= stride;
      if( --y<0 )
        goto done;
    }
  }
// One exit: close the file, give the palette scratch back, return the image.
done:
  fclose(fp);
  free(pal_buf);
  return img;
}
