// image_compress.inc -- one image into one.

int32_t compress_image(BmfFile* arc_in, BmfImage* p_i, const CodedTail* extra_blk) {
  BmfImage hdr;
  uint32_t filtered;
  uint8_t* pixels;
  int32_t coded_bytes;
  uint32_t coded_len;
  bool fits;
  uint32_t data_bytes, row_bytes;
  uint8_t has_coded;
  uint32_t hdr_pad8;
  int32_t ok_all, data_size;
  BmfImage* img;
  uint32_t pal_bytes, written;
  uint8_t ok, ok_raw;
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
  p_i->flags |= has_coded ? flags_tail : 0;
  hdr_pad8 = *(uint32_t*)&p_i->_pad8;
  hdr.stride = row_bytes;
  near_lossless_q = 0;
  *(uint32_t*)&hdr._pad8 = hdr_pad8;
  hdr.data_size = (uint32_t)p_i->data_size;
  plane_count = ((p_i->depth&depth_bits)+7)>>3;
  // An image member, version 2.0.  This was an `fwrite` of the first four
  // bytes of `"\x81\x8A" "20\x81\x90" "20a+b"` -- both signatures and three
  // more bytes in one literal, of which only the first four were ever written,
  // so the tag it writes could not be read off the line that writes it.
  const uint32_t tag = bmf_tag(bmf_sig_image, '2', '0');
  if( fwrite(&tag, 4u, 1u, arc->fp)!=1 )
    return 0;
  uint8_t bpp = p_i->depth;
  pal_bytes = bpp&depth_palette;
  if( bpp&depth_palette )
    pal_bytes = 3<<(bpp&31);
  if( p_i->data_size<0x10u )
    goto write_raw;
  desc_slow_mode = 1;
  hdr.flags |= flags_coded|flags_slow;
  if( (p_i->depth&depth_bits)<=4 ) {
    stream.size = p_i->data_size+0x20000;
    stream.buf = (uint8_t*)bmf_new(stream.size);
    packer_reset();
    plane_predictor = 0;
    plane_alt_model = 0;
    alphabet_reduced = 0;
    model_plane(p_i, p_i->pixels, p_i->pixels);
    goto write_coded;
  }
  filtered = search_filter(p_i, 0);
  hdr.flags |= flags_descriptors;
  if( p_i->flags&flags_transposed ) {
    uint32_t img_stride = p_i->stride;
    uint32_t hdr_pad8b = *(uint32_t*)&p_i->_pad8;
    hdr.width = p_i->width;
    hdr.height = p_i->height;
    data_bytes = p_i->data_size;
    hdr.stride = img_stride;
    *(uint32_t*)&hdr._pad8 = hdr_pad8b;
    hdr.data_size = data_bytes;
    // The top byte of the word at offset 8 is `p_i->flags`, so this is the
    // three bits set above put back over the source's own -- the same value
    // the `|=`s left, re-derived from a header that `search_filter` may have
    // transposed under it.
    hdr.flags = flags_coded|flags_descriptors|flags_slow|(uint8_t)(hdr_pad8b>>24);
  } else {
    data_bytes = p_i->data_size;
  }
  stream.size = data_bytes+0x20000;
  stream.buf = (uint8_t*)bmf_new(data_bytes+0x20000);
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
    int32_t pl = 0;
    do {
      uint32_t word_flags = (plane_desc[pl].flags<<2)|plane_desc[pl].nrefs;
      pack_bits(word_flags, 6);
      if( plane_desc[pl].flags&desc_has_refs ) {
        uint32_t word_dc = plane_desc[pl].dc;
        pack_bits(word_dc, 8);
        if( plane_desc[pl].nrefs>1u ) {
          uint32_t word_w4 = plane_desc[pl].weight0+64;
          pack_bits(word_w4, 8);
          uint32_t word_w8 = plane_desc[pl].weight1+64;
          pack_bits(word_w8, 8);
          if( plane_desc[pl].nrefs>2u ) {
            uint32_t word_w12 = plane_desc[pl].weight2+64;
            pack_bits(word_w12, 8);
          }
        }
      }
      ++pl;
    } while( pl<plane_count );
  }
  if( filtered ) {
    uint32_t n_pix = p_i->width*p_i->height;
    hdr.flags |= flags_planar;
    uint8_t *plane_buf = (uint8_t*)bmf_new(n_pix);
    if( plane_count>0 ) {
      int32_t pl2 = 0;
      do
        model_planes(p_i, plane_buf, plane_desc[pl2++].src_plane);
      while( pl2<plane_count );
    }
    free(plane_buf);
  } else {
    transform_planes(p_i);
  }
// Everything coded; flush the packer's last word and decide whether the coded
// stream is smaller than the pixels it came from.
write_coded:
  packer_flush();
  fits = (uint32_t)(stream.cur- stream.buf)<p_i->data_size;
  coded_bytes = stream.cur- stream.buf;
  hdr.data_size = (uint32_t)(stream.cur- stream.buf);
  if( fits ) {
    ok = fwrite(&hdr, 1u, 0x10u, arc->fp)==16;
    if( extra_blk ) {
      coded_len = extra_blk->len;
      ok &= fwrite(extra_blk, 1u, coded_len+sizeof(CodedTail), arc->fp)==coded_len+sizeof(CodedTail);
    }
    ok_all = (fwrite(stream.buf, 1u, coded_bytes, arc->fp)==(uint32_t)coded_bytes)&ok;
    free(stream.buf);
    if( ok_all&&(p_i->depth&depth_palette)!=0 )
      fwrite(&p_i->pixels[p_i->data_size], 1u, pal_bytes, arc->fp);
    fflush(arc->fp);
    if( ok_all )
      return (int32_t)hdr.data_size;
    return ok_all;
  }
  free(stream.buf);
  if( p_i->flags&flags_transposed )
    transpose_image(img, plane_count);
  // Both arms end here with `pixels` pointing at the same bytes: the transpose
  // above swaps the header's width, height and stride and leaves the pixel
  // buffer alone.  The `goto` that skipped this assignment was skipping a
  // second write of the value it already held.
write_raw:
  pixels = p_i->pixels;
  ok_raw = fwrite(img, 1u, 0x10u, arc->fp)==16;
  if( extra_blk ) {
    coded_len = extra_blk->len;
    ok_raw &= fwrite(extra_blk, 1u, coded_len+sizeof(CodedTail), arc->fp)==coded_len+sizeof(CodedTail);
  }
  written = fwrite(pixels, 1u, pal_bytes+img->data_size, arc->fp);
  data_size = img->data_size;
  if( !(ok_raw&(written==data_size+pal_bytes)) )
    return 0;
  return data_size;
}
