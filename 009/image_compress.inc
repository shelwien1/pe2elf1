// image_compress.inc -- one image into one.

int32_t __compress_image(BmfFile* arc_in, BmfImage* p_i, void* extra_blk) {
  BmfImage hdr;
  int32_t plane_n;
  int32_t row_step;
  uint32_t filtered;
  void* pixels;
  int32_t coded_bytes;
  uint8_t* pix_copy;
  uint32_t coded_len;
  bool fits;
  uint32_t data_bytes, row_bytes;
  uint8_t has_coded;
  uint8_t* plane_buf, * row_at;
  uint32_t hdr_pad8;
  int32_t pl, pl2, ok_all, y, pl_i, countdown, data_size, row_y;
  uint16_t img_h;
  BmfImage* img;
  uint32_t pal_bytes, word_flags, word_dc, word_w4, word_w8, word_w12, n_pix, written;
  uint8_t ok, * dst, ok_raw;
  BmfFile* arc = arc_in;
  if( !arc_in->fp )
    return 0;
  // The walk that was here read every member already in the output and left
  // the file positioned after the last, which is where an appended member had
  // to go.  `bmf c` writes the file, so there is nothing in front of this one.
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
  near_lossless_q = 0;
  *(uint32_t*)&hdr._pad8 = hdr_pad8;
  hdr.data_size = (uint32_t)p_i->data_size;
  plane_count = ((p_i->depth&depth_bits)+7)>>3;
  if( fwrite("\x81\x8A"
             "20\x81\x90"
             "20a+b",
             4u, 1u, arc->fp)!=1 )
    return 0;
  uint8_t bpp = p_i->depth;
  pal_bytes = bpp&0x80;
  if( bpp&0x80 )
    pal_bytes = 3<<(bpp&31);
  if( p_i->data_size<0x10u )
    goto write_raw;
  desc_slow_mode = 1;
  hdr.flags |= 0x24;
  if( (p_i->depth&depth_bits)<=4 ) {
    coded_size = p_i->data_size+0x20000;
    coded_buf = (uint8_t*)bmf_new(coded_size);
    packer_reset();
    plane_predictor = 0;
    plane_alt_model = 0;
    alphabet_reduced = 0;
    __model_plane(p_i, p_i->pixels, p_i->pixels);
    goto write_coded;
  }
  filtered = __search_filter(p_i, 0);
  hdr.flags |= 0x10u;
  if( p_i->flags&flags_transposed ) {
    uint32_t img_stride = p_i->stride;
    uint32_t hdr_pad8b = *(uint32_t*)&p_i->_pad8;
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
  packer_reset();
  // The near-lossless nibble, four zero bits, is written for a single
  // greyscale plane and for three or more planes -- and not for two, nor for a
  // single palette plane.  The decompiler wrote that as a `goto` into the
  // middle of the `if` from its own `else`, which is one condition with the
  // `||` taken out.
  if( plane_count==1 ? (p_i->depth&depth_grey)!=0 : plane_count>2 ) {
    near_lossless_q = 0;
    pack_bits(0, 4);
  }
  alphabet_reduced = 0;
  if( plane_count>0 ) {
    pl = 0;
    do {
      word_flags = (4*plane_desc[pl].flags)|plane_desc[pl].nrefs;
      pack_bits(word_flags, 6);
      if( plane_desc[pl].flags&8 ) {
        word_dc = plane_desc[pl].dc;
        pack_bits(word_dc, 8);
        if( plane_desc[pl].nrefs>1u ) {
          word_w4 = plane_desc[pl].weight0+64;
          pack_bits(word_w4, 8);
          word_w8 = plane_desc[pl].weight1+64;
          pack_bits(word_w8, 8);
          if( plane_desc[pl].nrefs>2u ) {
            word_w12 = plane_desc[pl].weight2+64;
            pack_bits(word_w12, 8);
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
        __model_planes((uint8_t*)p_i, plane_buf, plane_desc[pl2++].src_plane);
      while( pl2<plane_count );
    }
    free(plane_buf);
  } else {
    __transform_planes(p_i);
  }
// Everything coded; flush the packer's last word and decide whether the coded
// stream is smaller than the pixels it came from.
write_coded:
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
    if( ok_all&&(p_i->depth&depth_palette)!=0 )
      fwrite(&p_i->pixels[p_i->data_size], 1u, pal_bytes, arc->fp);
    fflush(arc->fp);
    if( ok_all )
      return (int32_t)hdr.data_size;
    return ok_all;
  }
  free(coded_buf);
  if( p_i->flags&flags_transposed ) {
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
    transpose_image_in_place(img, plane_n);
    free(pix_copy);
  }
  // Both arms end here with `pixels` pointing at the same bytes: the transpose
  // above swaps the header's width, height and stride and leaves the pixel
  // buffer alone.  The `goto` that skipped this assignment was skipping a
  // second write of the value it already held.
write_raw:
  pixels = (uint16_t*)p_i->pixels;
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
