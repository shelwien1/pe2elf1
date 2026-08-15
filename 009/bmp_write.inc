// bmp_write.inc -- write one back.

int32_t write_bmp(BmfImage* img, char* path, int32_t want_rle) {
  uint32_t off_bits, stride, done;
  uint8_t *buf_2, *end;
  int32_t rows2, pal_bytes;
  uint8_t *buf;
  int32_t row_i2, run_max, nib;
  FILE* fp, * fp2;
  bool can_rle;
  uint32_t stride1, at, at2;
  uint16_t* pix;
  uint8_t *out_buf, *out_at, *p, *q, *out2;
  uint32_t data_ofs2;
  int32_t rle_on, i, rows, bits, rle_mode, src_bits, rle_kind, rows3, row_i, lit_len, run, coded_bytes, y;
  uint8_t byte;
  uint32_t data_len, stride3, n_bytes, pad;
  rle_on = want_rle;
  fp = fopen(path, "wb");
  if( !fp )
    return 0;
  out_buf = (uint8_t*)bmf_new(img->data_size+8*img->height+(img->data_size>>5)+2048);
  BmpHeader* bmp = (BmpHeader*)out_buf;
  i = img->width;
  bmp->biSize = 40;
  bmp->bfType = 0x4D42;
  rows = img->height;
  rows2 = rows;
  bmp->bfReserved2 = 0;
  bmp->bfReserved1 = 0;
  bmp->biWidth = i;
  bmp->biHeight = rows;
  int8_t depth_flags = (int8_t)img->depth;
  bmp->biPlanes = 1;
  bits = img->depth&depth_bits;
  bmp->biBitCount = bits;
  bmp->biClrImportant = 0;
  bmp->biClrUsed = 0;
  bmp->biYPelsPerMeter = 0;
  bmp->biXPelsPerMeter = 0;
  buf = out_buf+54;
  if( bits<=8 ) {
    int32_t ncolours = 1<<(bits&31);
    int32_t ncol = 1<<(bits&31);
    if( depth_flags&0x40 ) {
      uint32_t levels = 0x100u>>(bits&31);
      if( ncol>0 ) {
        if( ncol/2 ) {
          uint32_t k = 0;
          int32_t grey = 0;
          do {
            *(uint32_t*)&out_buf[8*k+54] = ((uint8_t)grey<<16)|(uint8_t)grey|((uint8_t)grey<<8);
            *(uint32_t*)&out_buf[8*k+58] = ((uint8_t)(grey-levels+2*levels)<<16)|(uint8_t)(grey-levels+2*levels)|((uint8_t)(grey-levels+2*levels)<<8);
            grey += 2*levels;
            ++k;
          } while( k<(uint32_t)(ncol/2) );
          at = 2*k+1;
        } else {
          at = 1;
        }
        if( (at-1)<(uint32_t)ncol )
          *(uint32_t*)&out_buf[4*at+50] = ((uint8_t)(levels*(at-1))<<16)|(uint8_t)(levels*(at-1))|((uint8_t)(levels*(at-1))<<8);
      }
      data_ofs2 = img->data_size;
      pal_bytes = 4*ncol;
    } else if( depth_flags<0 ) {
      data_ofs2 = img->data_size;
      if( ncol<=0 ) {
        pal_bytes = 4*ncolours;
      } else {
        uint32_t pairs = ncolours/2;
        if( ncolours/2 ) {
          uint32_t data_ofs = img->data_size;
          uint32_t j = 0;
          uint8_t *pal = (uint8_t*)img+data_ofs2;
          do {
            uint32_t slot = 2*j;
            int32_t bgr = *(uint16_t*)&pal[6*j+19];
            int32_t r = (uint8_t)pal[6*j+21];
            *(uint32_t*)&out_buf[4*slot+54] = ((uint8_t)pal[6*j+18]<<16)|*(uint16_t*)&pal[6*j+16];
            *(uint32_t*)&out_buf[4*slot+58] = (r<<16)|bgr;
            ++j;
          } while( j<pairs );
          data_ofs2 = data_ofs;
          rle_on = want_rle;
          at2 = 2*j+1;
          done = at2;
        } else {
          at2 = 1;
          done = 1;
        }
        if( (at2-1)<(uint32_t)ncol ) {
          uint8_t *pal2 = (uint8_t*)img+data_ofs2;
          int32_t bgr2 = *(uint16_t*)&pal2[3*done+13];
          *(uint32_t*)&out_buf[4*done+50] = (((uint8_t)pal2[3*done+15])<<16)|bgr2;
        }
        pal_bytes = 4*ncol;
      }
    } else {
      pal_bytes = 4*ncolours;
      memset(buf, 0, 4*ncolours);
      data_ofs2 = img->data_size;
      rows2 = img->height;
    }
    buf = &out_buf[pal_bytes+54];
  } else {
    data_ofs2 = img->data_size;
  }
  stride = img->stride;
  off_bits = (uint32_t)(buf-out_buf);
  pix = (uint16_t*)img;
  rle_mode = rle_on;
  data_len = data_ofs2;
  while( 1 ) {
    out_at = buf;
    p = (uint8_t*)pix+data_len-stride+16;
    bmp->bfOffBits = off_bits;
    if( !rle_mode )
      break;
    src_bits = pix[5]&0x3F;
    can_rle = src_bits==4;
    if( src_bits!=4 ) {
      if( src_bits!=8 )
        break;
      can_rle = 0;
    }
    rle_kind = 2;
    if( !can_rle )
      rle_kind = 1;
    bmp->biCompression = rle_kind;
    nib = rle_kind-1;
    run_max = (0x100u>>((rle_kind-1)&31))-1;
    if( rows2>0 ) {
      rows3 = rows2;
      stride3 = stride;
      row_i = 0;
      while( 1 ) {
        if( p>=&p[stride3] )
          goto write_rows;
        buf_2 = out_at;
        end = &p[stride3];
        row_i2 = row_i;
        lit_len = 0;
        do {
          while( 1 ) {
            while( 1 ) {
              q = p+1;
              if( p+1>=end )
                break;
              byte = (uint8_t)*p;
              run = 1;
              do {
                if( byte!=(uint8_t)p[run] )
                  break;
                ++run;
              } while( end>&p[run] );
              if( run<=2&&(run!=2||lit_len) )
                break;
              if( run_max<run )
                run = run_max;
              if( lit_len ) {
                if( nib ? lit_len!=1 : lit_len>=3 ) {
                  *buf_2 = 0;
                  uint8_t *abs_end2 = buf_2+2;
                  buf_2[1] = lit_len<<(nib&31);
                  memcpy(buf_2+2, &p[-lit_len], lit_len);
                  buf_2 += lit_len+2;
                  if( lit_len&1 ) {
                    abs_end2[lit_len] = 0;
                    ++buf_2;
                  }
                } else if( nib ) {
                  *buf_2 = 2;
                  buf_2[1] = *(p-1);
                  buf_2 += 2;
                } else {
                  if( lit_len==2 ) {
                    *buf_2 = 1;
                    buf_2[1] = *(p-2);
                    buf_2 += 2;
                  }
                  *buf_2 = 1;
                  buf_2[1] = *(p-1);
                  buf_2 += 2;
                }
                lit_len = 0;
                byte = *p;
              }
              buf_2[1] = byte;
              p += run;
              *buf_2 = run<<(nib&31);
              uint8_t *out_end = buf_2+2;
              buf_2 += 2;
              if( p>=end ) {
                out_at = out_end;
                row_i = row_i2;
                rows3 = img->height;
                stride3 = img->stride;
                goto write_rows;
              }
            }
            ++p;
            if( ++lit_len!=run_max )
              break;
            if( nib ? lit_len!=1 : lit_len>=3 ) {
              *buf_2 = 0;
              uint8_t *abs_end = buf_2+2;
              buf_2[1] = lit_len<<(nib&31);
              memcpy(buf_2+2, &q[-lit_len], lit_len);
              buf_2 += lit_len+2;
              if( lit_len&1 ) {
                abs_end[lit_len] = 0;
                ++buf_2;
              }
            } else if( nib ) {
              *buf_2 = 2;
              buf_2[1] = *(q-1);
              buf_2 += 2;
            } else {
              if( lit_len==2 ) {
                *buf_2 = 1;
                buf_2[1] = *(q-2);
                buf_2 += 2;
              }
              *buf_2 = 1;
              buf_2[1] = *(q-1);
              buf_2 += 2;
            }
            if( q>=end ) {
              out_at = buf_2;
              row_i = row_i2;
              rows3 = img->height;
              stride3 = img->stride;
              goto write_rows;
            }
            lit_len = 0;
          }
        } while( q<end );
        out_at = buf_2;
        row_i = row_i2;
        if( lit_len&&(nib ? lit_len!=1 : lit_len>=3) ) {
          *buf_2 = 0;
          buf_2[1] = lit_len<<(nib&31);
          memcpy(buf_2+2, &q[-lit_len], lit_len);
          row_i = row_i2;
          out_at = &buf_2[lit_len+2];
          if( lit_len&1 ) {
            buf_2[lit_len+2] = 0;
            ++out_at;
          }
        } else if( lit_len&&nib ) {
          buf_2[1] = *(q-1);
          *buf_2 = 2;
          out_at = buf_2+2;
        } else if( lit_len ) {
          if( lit_len==2 ) {
            *buf_2 = 1;
            buf_2[1] = *(q-2);
            out_at = buf_2+2;
          }
          out_at[1] = *(q-1);
          *out_at = 1;
          out_at += 2;
        }
        rows3 = img->height;
        stride3 = img->stride;
// The row-writing loop.  Three paths reach it, each setting `row_i`, `rows3`
// and `stride3` first -- which is the loop's three arguments, passed by
// assignment because the decompiler had no function to pass them to.
write_rows:
        *out_at = 0;
        out_at[1] = 0;
        out_at += 2;
        ++row_i;
        p -= 2*stride3;
        if( row_i>=rows3 ) {
          rows2 = rows3;
          stride = stride3;
          pix = (uint16_t*)img;
          data_len = img->data_size;
          break;
        }
      }
    }
    *out_at = 0;
    out_at[1] = 1;
    out_at += 2;
    coded_bytes = out_at-buf;
    if( data_len>(uint32_t)(out_at-buf) ) {
      bmp->biSizeImage = (uint32_t)coded_bytes;
      n_bytes = (uint32_t)(out_at-out_buf);
      bmp->bfSize = n_bytes;
      if( fwrite(out_buf, 1u, n_bytes, fp)!=bmp->bfSize )
        return 0;
      free(out_buf);
      fclose(fp);
      return 1;
    }
    rle_mode = 0;
  }
  fp2 = fp;
  out2 = out_buf;
  bmp->biCompression = 0;
  if( rows2<=0 ) {
    coded_bytes = 0;
  } else {
    pad = ((stride+3)&0xFFFFFFFC)-stride;
    stride1 = stride;
    y = 0;
    do {
      memcpy(out_at, p, stride1);
      stride1 = img->stride;
      out_at += stride1;
      p -= stride1;
      if( pad ) {
        *(uint32_t*)out_at = 0;
        out_at += pad;
      }
      ++y;
    } while( y<img->height );
    out2 = out_buf;
    fp2 = fp;
    coded_bytes = out_at-buf;
  }
  bmp->biSizeImage = (uint32_t)coded_bytes;
  n_bytes = (uint32_t)(out_at-out2);
  bmp->bfSize = n_bytes;
  if( fwrite(out2, 1u, n_bytes, fp2)!=bmp->bfSize )
    return 0;
  free(out2);
  fclose(fp2);
  return 1;
}
