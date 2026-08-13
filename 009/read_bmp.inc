// read_bmp.inc -- read a BMP, in every depth and both run-length forms.

int32_t*__read_bmp(char* path) {
  BmfImage* img_f;
  uint8_t* row_ofs;
  void* pal_buf;
  uint8_t* row, bmp_bgra[4], pix;
  int32_t y4, y8;
  int32_t run4;
  uint8_t* row_at;
  uint8_t* pal, * pal2, * pal3;
  FILE* fp;
  uint8_t lo;
  BmfImage* img;
  uint32_t row_pad, byte;
  uint8_t cur, lo16;
  uint8_t* row4, * row3, * row6, * pal_at, * row5;
  int32_t pal_n, i, run, run_val, hi_nibble, left4, left4b, y, dx, dy, dxy, dy4, byte_in, step;
  uint32_t stride_pad, pair, hi, stride, got, left;
  BmpHeader hdr;
  fp = fopen(path, "rb");
  if( !fp||fread(&hdr.bfType, 0xEu, 1u, fp)!=1||hdr.bfType!=0x4D42||fread(&hdr.biSize, 0x28u, 1u, fp)!=1||hdr.biSize!=40||hdr.biPlanes!=1 ) {
    return nullptr;
  }
  {
    const int32_t bmp_w = hdr.biWidth;
    const int32_t bmp_h = hdr.biHeight;
    const int32_t bmp_bpp = hdr.biBitCount;
    if( bmp_w<=0||bmp_w>0xFFFF||bmp_h<=0||bmp_h>0xFFFF||(bmp_bpp>=8&&(uint32_t)bmp_w*(uint32_t)((bmp_bpp+7)>>3)>0xFFFFu)||(bmp_bpp!=1&&bmp_bpp!=4&&bmp_bpp!=8&&bmp_bpp!=24&&bmp_bpp!=32)||(int32_t)hdr.biClrUsed<0||(bmp_bpp<=8&&(int32_t)hdr.biClrUsed>(1<<bmp_bpp))||hdr.biCompression>2||(hdr.biCompression==1&&bmp_bpp!=8)||(hdr.biCompression==2&&bmp_bpp!=4) ) {
      fclose(fp);
      return nullptr;
    }
  }
  img = (BmfImage*)(__alloc_image(hdr.biWidth, hdr.biHeight, hdr.biBitCount, hdr.biBitCount<=8u, 1));
  if( !img ) {
    fclose(fp);
    return nullptr;
  }
  stride_pad = (img->stride+3)&0xFFFFFFFC;
  if( hdr.biBitCount<=8u ) {
    pal_n = 1<<(hdr.biBitCount&31);
    if( (int32_t)hdr.biClrUsed )
      pal_n = (int32_t)hdr.biClrUsed;
    if( pal_n>0 ) {
      for( i = 0; i<pal_n; ++i ) {
        fread(bmp_bgra, 4u, 1u, fp);
        if( img->depth&0x80 )
          pal = img->pixels+img->data_size;
        else
          pal = 0;
        *(pal+3*i+2) = bmp_bgra[2];
        if( img->depth&0x80 )
          pal2 = img->pixels+img->data_size;
        else
          pal2 = 0;
        *(pal2+3*i+1) = bmp_bgra[1];
        if( img->depth&0x80 )
          pal3 = img->pixels+img->data_size;
        else
          pal3 = 0;
        *(pal3+3*i) = bmp_bgra[0];
      }
    }
  }
  pal_buf = bmf_new(stride_pad<256 ? 256 : stride_pad);
  row = img->pixels+img->data_size-img->stride;
  fseek(fp, (int32_t)hdr.bfOffBits, 0);
  if( hdr.biCompression ) {
    uint8_t*const pix_lo = img->pixels;
    uint8_t*const pix_hi = img->pixels+img->data_size;
    if( hdr.biCompression==1 ) {
      memset(img->pixels, 0, img->data_size);
      row_at = row;
      img_f = img;
      y4 = img->height-1;
      while( 1 ) {
        row_ofs = row_at;
        if( ferror(fp) )
          return nullptr;
        run = fgetc(fp);
        run_val = fgetc(fp);
        if( run<0||run_val<0 )
          return nullptr;
        if( run ) {
          if( row_ofs<pix_lo||row_ofs+run>pix_hi )
            return nullptr;
          __builtin_memset(row_ofs, run_val, run);
          row_at = row_ofs+run;
        } else if( run_val ) {
          if( run_val==1 )
            goto LABEL_61;
          if( run_val==2 ) {
            dx = fgetc(fp);
            dy = fgetc(fp);
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
            goto LABEL_61;
          row_at = img_f->pixels+y4*(uint16_t)img_f->stride;
        }
      }
    }
    if( hdr.biCompression!=2 )
      return nullptr;
    memset(img->pixels, 0, img->data_size);
    img_f = img;
    hi_nibble = 1;
    y8 = img->height-1;
    while( 1 ) {
      while( 1 ) {
        while( 1 ) {
LABEL_44:
          if( ferror(fp) )
            return nullptr;
          run4 = fgetc(fp);
          byte_in = fgetc(fp);
          if( run4<0||byte_in<0 )
            return nullptr;
          byte = (uint32_t)byte_in;
          pair = byte;
          if( !run4 )
            break;
          if( row<pix_lo||row+(run4/2+1)>pix_hi )
            return nullptr;
          lo = byte&0xF;
          if( hi_nibble ) {
            left4b = run4;
            row3 = row;
            while( left4b!=1 ) {
              *row3++ = pair;
              left4b -= 2;
              if( !left4b ) {
                row = row3;
                hi_nibble = 1;
                goto LABEL_44;
              }
            }
            row = row3;
            *row3 = pair&0xF0;
            hi_nibble = 0;
          } else {
            left4 = run4;
            row4 = row;
            cur = *row;
            hi = pair>>4;
            lo16 = 16*lo;
            while( 1 ) {
              *row4++ = hi|cur;
              if( left4==1 )
                break;
              cur = lo16;
              left4 -= 2;
              if( !left4 ) {
                row = row4;
                *row4 = lo16;
                hi_nibble = 0;
                goto LABEL_44;
              }
            }
            row = row4;
            hi_nibble = 1;
          }
        }
        if( byte )
          break;
        if( --y8<0 )
          goto LABEL_61;
        row = img_f->pixels+y8*(uint16_t)img_f->stride;
      }
      if( byte==1 )
        goto LABEL_61;
      if( byte!=2 )
        break;
      dxy = fgetc(fp);
      dy4 = fgetc(fp);
      if( dxy<0||dy4<0 )
        return nullptr;
      step = (dxy>>1)-dy4*(uint16_t)img_f->stride;
      if( (dxy&1)==1 ) {
        if( !hi_nibble )
          ++step;
        hi_nibble = !hi_nibble;
      }
      row += step;
    }
    if( row<pix_lo||row+((int32_t)byte/2+1)>pix_hi )
      return nullptr;
    fread(pal_buf, (((byte+1)>>1)+1)&0xFFFFFFFE, 1u, fp);
    pal_at = (uint8_t*)pal_buf;
    row5 = row;
    while( 1 ) {
      pix = *pal_at;
      if( hi_nibble ) {
        left = pair-1;
        if( !left ) {
          row = row5;
          *row5 = *pal_at&0xF0;
          hi_nibble = 0;
          goto LABEL_44;
        }
        *row5++ = pix;
        hi_nibble = 1;
      } else {
        *row5++ |= (uint8_t)*pal_at>>4;
        left = pair-1;
        if( !left ) {
          row = row5;
          hi_nibble = 1;
          goto LABEL_44;
        }
        *row5 = 16*(pix&0xF);
        hi_nibble = 0;
      }
      ++pal_at;
      pair = left-1;
      if( !pair ) {
        row = row5;
        goto LABEL_44;
      }
    }
  }
  stride = img->stride;
  row_pad = stride_pad-stride;
  if( hdr.biHeight-1>=0 ) {
    y = hdr.biHeight-1;
    img_f = img;
    row6 = row;
    while( 1 ) {
      got = fread(row6, 1u, stride, fp);
      stride = (uint16_t)img_f->stride;
      if( got!=stride )
        return nullptr;
      if( row_pad ) {
        fseek(fp, row_pad, 1);
        stride = (uint16_t)img_f->stride;
      }
      row6 -= stride;
      if( --y<0 )
        goto LABEL_61;
    }
  }
LABEL_61:
  fclose(fp);
  free(pal_buf);
  return (int32_t*)img;
}
