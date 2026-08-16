// image_compress.inc -- one image into one.

// The member's sixteen-byte header and, when the file carries one, the coded
// block that comes before the payload.  Both exits write these four lines --
// the coded arm with the header it has just filled in and the raw arm with the
// image's own, which are the same sixteen bytes laid out the same way, and
// each spelled the block's length into the same function-scope `coded_len`.
static uint8_t write_member_head(const void* head, const CodedTail* extra, FILE* fp) {
  uint8_t ok = fwrite(head, 1u, 0x10u, fp)==16;
  if( extra ) {
    const uint32_t coded_len = extra->len;
    ok &= fwrite(extra, 1u, coded_len+sizeof(CodedTail), fp)==coded_len+sizeof(CodedTail);
  }
  return ok;
}

// The member header, taken from the image.  Every field but `flags`, which the
// two writers set differently: the first takes the image's own after adding the
// tail bit to it, and the second puts the three bits it has already `|=`d back
// over a header `search_filter` may have transposed under it.
//
// `_pad8[2]`, `depth` and `flags` are four consecutive bytes at offset 8, and
// both copies of this moved them as one `uint32_t` -- then the second recovered
// `flags` from the top of the saved word twenty lines later.  Member copies say
// it without the layout.  Each copy also spelled the stride into a local of its
// own, `row_bytes` and `img_stride`, read once on the next line but one.
static void header_from(BmfImage* hdr, const BmfImage* p_i) {
  hdr->width = p_i->width;
  hdr->height = p_i->height;
  hdr->stride = p_i->stride;
  hdr->_pad8[0] = p_i->_pad8[0];
  hdr->_pad8[1] = p_i->_pad8[1];
  hdr->depth = p_i->depth;
  hdr->data_size = (uint32_t)p_i->data_size;
}

int32_t compress_image(BmfFile* arc_in, BmfImage* p_i, const CodedTail* extra_blk) {
  BmfImage hdr;
  uint32_t filtered;
  uint8_t* pixels;
  int32_t coded_bytes;
  bool fits;
  uint32_t data_bytes, written;
  if( !arc_in->fp )
    return 0;
  // The walk that was here read every member already in the output and left
  // the file positioned after the last, which is where an appended member had
  // to go.  `bmf c` writes the file, so there is nothing in front of this one.
  uint8_t has_coded = extra_blk!=nullptr;
  if( extra_blk )
    has_coded = 1;
  p_i->flags |= has_coded ? flags_tail : 0;
  near_lossless_q = 0;
  header_from(&hdr, p_i);
  hdr.flags = p_i->flags;
  plane_count = ((p_i->depth&depth_bits)+7)>>3;
  // An image member, version 2.0.  This was an `fwrite` of the first four
  // bytes of `"\x81\x8A" "20\x81\x90" "20a+b"` -- both signatures and three
  // more bytes in one literal, of which only the first four were ever written,
  // so the tag it writes could not be read off the line that writes it.
  const uint32_t tag = bmf_tag(bmf_sig_image, '2', '0');
  if( fwrite(&tag, 4u, 1u, arc_in->fp)!=1 )
    return 0;
  uint8_t bpp = p_i->depth;
  uint32_t pal_bytes = bpp&depth_palette;
  if( bpp&depth_palette )
    pal_bytes = 3<<(bpp&31);
  if( p_i->data_size<0x10u )
    goto write_raw;
  desc_slow_mode = 1;
  hdr.flags |= flags_coded|flags_slow;
  if( (p_i->depth&depth_bits)<=4 ) {
    stream_open(p_i->data_size);
    plane_predictor = 0;
    plane_alt_model = 0;
    alphabet_reduced = 0;
    model_plane(p_i, p_i->pixels, p_i->pixels);
    goto write_coded;
  }
  filtered = search_filter(p_i, 0);
  hdr.flags |= flags_descriptors;
  if( p_i->flags&flags_transposed ) {
    header_from(&hdr, p_i);
    data_bytes = p_i->data_size;
    // The three bits set above, put back over the source's own -- the same
    // value the `|=`s left, re-derived from a header that `search_filter` may
    // have transposed under it.  This was `hdr_pad8b >> 24`, the top byte of
    // the word at offset 8, and `p_i->flags` is not written between the two.
    hdr.flags = flags_coded|flags_descriptors|flags_slow|p_i->flags;
  } else {
    data_bytes = p_i->data_size;
  }
  stream_open(data_bytes);
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
    const uint8_t ok = write_member_head(&hdr, extra_blk, arc_in->fp);
    int32_t ok_all = (fwrite(stream.buf, 1u, coded_bytes, arc_in->fp)==(uint32_t)coded_bytes)&ok;
    free(stream.buf);
    if( ok_all&&(p_i->depth&depth_palette)!=0 )
      fwrite(&p_i->pixels[p_i->data_size], 1u, pal_bytes, arc_in->fp);
    fflush(arc_in->fp);
    if( ok_all )
      return (int32_t)hdr.data_size;
    return ok_all;
  }
  free(stream.buf);
  if( p_i->flags&flags_transposed )
    transpose_image(p_i, plane_count);
  // Both arms end here with `pixels` pointing at the same bytes: the transpose
  // above swaps the header's width, height and stride and leaves the pixel
  // buffer alone.  The `goto` that skipped this assignment was skipping a
  // second write of the value it already held.
write_raw:
  pixels = p_i->pixels;
  const uint8_t ok_raw = write_member_head(p_i, extra_blk, arc_in->fp);
  written = fwrite(pixels, 1u, pal_bytes+p_i->data_size, arc_in->fp);
  int32_t data_size = p_i->data_size;
  if( !(ok_raw&(written==data_size+pal_bytes)) )
    return 0;
  return data_size;
}
