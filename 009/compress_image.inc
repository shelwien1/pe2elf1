// compress_image.inc -- one image into one.

int32_t __compress_image(BmfArc* arc_in, BmfImage* p_i, void* extra_blk) {
  BmfImage hdr;
  int32_t plane_n;
  int32_t row_step;
  uint32_t filtered;
  void* pixels;
  int32_t coded_bytes;
  uint8_t* pix_copy;
  uint32_t coded_len;
  BmfArc* arc;
  FILE* i;
  bool fits;
  uint8_t bpp;
  uint32_t acc, data_bytes, img_stride, row_bytes, shifted;
  uint8_t has_coded;
  uint8_t* plane_buf, * row_at;
  uint32_t hdr_pad8, hdr_pad8b;
  int32_t bits_left, pl, free_bits, pl2, ok_all, y, pl_i, countdown, data_size, row_y;
  uint16_t img_h;
  BmfImage* img;
  uint16_t w16;
  uint32_t pal_bytes, word_flags, word_dc, word_w4, word_w8, word_w12, n_pix, written;
  uint8_t ok, * dst, ok_raw;
  arc = arc_in;
  if( !arc_in->fp )
    return 0;
  if( !feof(arc_in->fp) ) {
    __expand_image(arc, 1, (void**)nullptr);
    for( i = arc->fp; i; i = arc->fp ) {
      if( feof(arc->fp) )
        break;
      if( feof(i) )
        break;
      __expand_image(arc, 1, (void**)nullptr);
    }
    if( !arc->fp )
      return 0;
  }
  has_coded = extra_blk!=nullptr;
  img = (BmfImage*)(p_i);
  row_bytes = p_i->stride;
  if( extra_blk )
    has_coded = 1;
  hdr.width = p_i->width;
  hdr.height = p_i->height;
  p_i->flags |= has_coded<<7;
  hdr_pad8 = *(uint32_t*)&p_i->_pad8;
  hdr.stride = (uint32_t)row_bytes;
  plane_desc[0].weight0 = 512;
  near_lossless_q = 0;
  *(uint32_t*)&hdr._pad8 = hdr_pad8;
  hdr.data_size = (uint32_t)p_i->data_size;
  plane_count = ((p_i->depth&0x3Fu)+7)>>3;
  if( fwrite("\x81\x8A"
             "20\x81\x90"
             "20a+b",
             4u, 1u, arc->fp)!=1 )
    return 0;
  bpp = p_i->depth;
  ++*(uint32_t*)arc;
  pal_bytes = bpp&0x80;
  if( bpp&0x80 )
    pal_bytes = 3<<(bpp&31);
  if( p_i->data_size<0x10u )
    goto LABEL_76;
  desc_slow_mode = 1;
  hdr.flags |= 0x24;
  if( (p_i->depth&0x3Fu)<=4 ) {
    coded_size = p_i->data_size+0x20000;
    coded_buf = (uint8_t*)bmf_new(coded_size);
    packer_free_bits = 0;
    packer_acc = 0;
    out_cursor = coded_buf;
    packer_word = (uint32_t*)coded_buf;
    hist_scratch = coded_buf+coded_size-4096;
    plane_predictor = 0;
    plane_alt_model = 0;
    alphabet_reduced = 0;
    __model_plane(p_i, p_i->pixels, p_i->pixels);
    goto LABEL_57;
  }
  filtered = __search_filter(p_i, 0);
  hdr.flags |= 0x10u;
  if( p_i->flags&2 ) {
    img_stride = p_i->stride;
    hdr_pad8b = *(uint32_t*)&p_i->_pad8;
    hdr.width = p_i->width;
    hdr.height = p_i->height;
    data_bytes = p_i->data_size;
    hdr.stride = (uint32_t)img_stride;
    *(uint32_t*)&hdr._pad8 = hdr_pad8b;
    hdr.data_size = (uint32_t)data_bytes;
    hdr.flags = 0x34|(uint8_t)(hdr_pad8b>>24);
  } else {
    data_bytes = p_i->data_size;
  }
  coded_size = data_bytes+0x20000;
  coded_buf = (uint8_t*)bmf_new(data_bytes+0x20000);
  out_cursor = coded_buf;
  packer_free_bits = 0;
  packer_acc = 0;
  packer_word = (uint32_t*)coded_buf;
  hist_scratch = coded_buf+coded_size-4096;
  if( plane_count==1 ) {
    if( p_i->depth&0x40 ) {
LABEL_22:
      near_lossless_q = 0;
      if( packer_free_bits<4 ) {
        *(uint32_t*)packer_word = packer_acc;
        packer_word = (uint32_t*)out_cursor;
        packer_acc = 0;
        packer_free_bits += 28;
        out_cursor += 4;
      } else {
        packer_free_bits -= 4;
      }
    }
  } else if( plane_count>2 ) {
    goto LABEL_22;
  }
  alphabet_reduced = 0;
  if( plane_count>0 ) {
    bits_left = packer_free_bits;
    pl = 0;
    do {
      word_flags = (4*plane_desc[pl+1].flags)|plane_desc[pl+1].nrefs;
      if( bits_left<6 ) {
        *(uint32_t*)packer_word = packer_acc|(2*(word_flags<<((31-bits_left)&31)));
        packer_word = (uint32_t*)out_cursor;
        out_cursor += 4;
        bits_left = packer_free_bits+26;
        packer_acc = word_flags>>(packer_free_bits&31);
      } else {
        packer_acc |= word_flags<<(-bits_left&31);
        bits_left = packer_free_bits-6;
      }
      packer_free_bits = bits_left;
      if( plane_desc[pl+1].flags&8 ) {
        word_dc = plane_desc[pl+1].dc;
        if( bits_left<8 ) {
          *(uint32_t*)packer_word = packer_acc|(2*(word_dc<<((31-bits_left)&31)));
          packer_word = (uint32_t*)out_cursor;
          out_cursor += 4;
          bits_left = packer_free_bits+24;
          packer_acc = word_dc>>(packer_free_bits&31);
        } else {
          shifted = word_dc<<(-bits_left&31);
          bits_left -= 8;
          packer_acc |= shifted;
        }
        packer_free_bits = bits_left;
        if( plane_desc[pl+1].nrefs>1u ) {
          word_w4 = plane_desc[pl+1].weight0+64;
          if( bits_left<8 ) {
            *(uint32_t*)packer_word = packer_acc|(2*(word_w4<<((31-bits_left)&31)));
            packer_word = (uint32_t*)out_cursor;
            out_cursor += 4;
            free_bits = packer_free_bits+24;
            packer_acc = word_w4>>(packer_free_bits&31);
          } else {
            packer_acc |= word_w4<<(-bits_left&31);
            free_bits = packer_free_bits-8;
          }
          packer_free_bits = free_bits;
          word_w8 = plane_desc[pl+1].weight1+64;
          if( free_bits<8 ) {
            *(uint32_t*)packer_word = packer_acc|(2*(word_w8<<((31-free_bits)&31)));
            packer_word = (uint32_t*)out_cursor;
            out_cursor += 4;
            bits_left = packer_free_bits+24;
            packer_acc = word_w8>>(packer_free_bits&31);
          } else {
            bits_left = free_bits-8;
            packer_acc |= word_w8<<(-free_bits&31);
          }
          packer_free_bits = bits_left;
          if( plane_desc[pl+1].nrefs>2u ) {
            word_w12 = plane_desc[pl+1].weight2+64;
            if( bits_left<8 ) {
              *(uint32_t*)packer_word = packer_acc|(2*(word_w12<<((31-bits_left)&31)));
              packer_word = (uint32_t*)out_cursor;
              out_cursor += 4;
              acc = word_w12>>(packer_free_bits&31);
              bits_left = packer_free_bits+24;
              packer_free_bits += 24;
              packer_acc = acc;
            } else {
              packer_acc |= word_w12<<(-bits_left&31);
              bits_left = packer_free_bits-8;
              packer_free_bits -= 8;
            }
          }
        }
      }
      ++pl;
    } while( pl<plane_count );
  }
  if( filtered ) {
    n_pix = p_i->width*p_i->height;
    hdr.flags |= 8u;
    plane_buf = (uint8_t*)bmf_new(n_pix);
    if( plane_count>0 ) {
      pl2 = 0;
      do
        __model_planes((uint8_t*)p_i, plane_buf, plane_desc[pl2+++1].src_plane, 0);
      while( pl2<plane_count );
    }
    free(plane_buf);
  } else {
    __transform_planes(p_i, 0);
  }
LABEL_57:
  *(uint32_t*)packer_word = packer_acc;
  fits = (uint32_t)(out_cursor- coded_buf)<p_i->data_size;
  coded_bytes = out_cursor- coded_buf;
  hdr.data_size = (uint32_t)(out_cursor- coded_buf);
  if( fits ) {
    ok = fwrite(&hdr, 1u, 0x10u, arc->fp)==16;
    if( extra_blk ) {
      coded_len = ((const uint32_t*)extra_blk)[1];
      ok &= fwrite(extra_blk, 1u, coded_len+8, arc->fp)==coded_len+8;
    }
    ok_all = (fwrite(coded_buf, 1u, coded_bytes, arc->fp)==(uint32_t)coded_bytes)&ok;
    free(coded_buf);
    if( ok_all&&(p_i->depth&0x80)!=0 )
      fwrite(&p_i->pixels[p_i->data_size], 1u, pal_bytes, arc->fp);
    fflush(arc->fp);
    if( ok_all )
      return (int32_t)hdr.data_size;
    return ok_all;
  }
  free(coded_buf);
  if( p_i->flags&2 ) {
    pix_copy = (uint8_t*)bmf_new(p_i->data_size);
    plane_n = plane_count;
    row_step = plane_count*(p_i->height-1);
    pixels = (uint16_t*)p_i->pixels;
    memcpy(pix_copy, p_i->pixels, p_i->data_size);
    img_h = p_i->height;
    if( img_h ) {
      row_at = pix_copy;
      row_y = 0;
      y = 0;
      do {
        pl_i = img->width;
        dst = img->pixels+y;
        do {
          countdown = plane_n;
          do {
            *dst++ = *row_at++;
            --countdown;
          } while( countdown );
          dst += row_step;
          --pl_i;
        } while( pl_i );
        img = (BmfImage*)(p_i);
        img_h = p_i->height;
        y += plane_n;
        ++row_y;
      } while( row_y<img_h );
    }
    w16 = img->width;
    img->width = img_h;
    img->height = w16;
    img->flags ^= 2u;
    img->stride = img_h*plane_n;
    free(pix_copy);
    goto LABEL_77;
  }
LABEL_76:
  pixels = (uint16_t*)p_i->pixels;
LABEL_77:
  ok_raw = fwrite(img, 1u, 0x10u, arc->fp)==16;
  if( extra_blk ) {
    coded_len = ((const uint32_t*)extra_blk)[1];
    ok_raw &= fwrite(extra_blk, 1u, coded_len+8, arc->fp)==coded_len+8;
  }
  written = fwrite(pixels, 1u, pal_bytes+img->data_size, arc->fp);
  data_size = img->data_size;
  if( !(ok_raw&(written==data_size+pal_bytes)) )
    return 0;
  return data_size;
}
