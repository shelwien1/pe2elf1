// image_compress.inc -- one image into one.

// The member's sixteen-byte header and, when the file carries one, the coded
// block that comes before the payload.  Both exits write these four lines --
// the coded arm with the header it has just filled in and the raw arm with the
// image's own, which are the same sixteen bytes laid out the same way, and
// each spelled the block's length into the same function-scope `coded_len`.
static uint8_t write_member_head(const void* head, const CodedTail* extra, BmfStream* io) {
  uint8_t ok = io->write_exact(head, 0x10u);
  if( extra ) {
    const uint32_t coded_len = extra->len;
    ok &= io->write_exact(extra, coded_len+sizeof(CodedTail));
  }
  return ok;
}

// The image into `stream`, and the flags its coding needs into `hdr`.  Two
// shapes: four bits or fewer is one plane straight through, and anything wider
// gets the filter search, the descriptors, and the planes.  The narrow arm was
// a `goto` past all of that to the flush below, which is this function's own
// early `return`.
void BMFCodec::code_image_body(BmfImage* p_i, BmfImage& hdr) {
  if( (p_i->depth&depth_bits)<=4 ) {
    stream_open(p_i->data_size);
    plane_predictor = 0;
    plane_alt_model = 0;
    alphabet_reduced = 0;
    model_plane(p_i, p_i->pixels, p_i->pixels);
    return;
  }
  const uint32_t filtered = search_filter(p_i);
  hdr.flags |= flags_descriptors;
  // `search_filter` may have transposed the image under the header written at
  // the top, so the header is taken again -- but not the byte count, which the
  // transpose does not move and which both arms of the `if` read the same way.
  const uint32_t data_bytes = p_i->data_size;
  if( p_i->flags&flags_transposed ) {
    hdr.header_from(p_i);
    // The three bits set above, put back over the source's own -- the same
    // value the `|=`s left, re-derived from a header that `search_filter` may
    // have transposed under it.  This was `hdr_pad8b >> 24`, the top byte of
    // the word at offset 8, and `p_i->flags` is not written between the two.
    hdr.flags = flags_coded|flags_descriptors|flags_slow|p_i->flags;
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
  write_plane_descs();
  if( filtered ) {
    hdr.flags |= flags_planar;
    uint8_t *plane_buf = (uint8_t*)bmf_new(p_i->width*p_i->height);
    for( int32_t k = 0; k<plane_count; ++k )
      model_planes(p_i, plane_buf, plane_desc[k].src_plane);
    free(plane_buf);
  } else {
    transform_planes(p_i);
  }
}

// **The in-memory entry point**, and half of what a caller outside this file
// actually wants: an image in memory becomes a coded stream in memory, with no
// path and no file.
//
// It is `compress_image` over a buffer rather than a second copy of it.
// `BmfStream::in_memory` is a stream that grows to hold what is written to it,
// which is the shape `compress_image` already takes -- so the file layer is
// unchanged and the codec gains an entry point rather than a fork.  Nothing
// here is platform-dependent and nothing touches a disk.
//
// The caller frees what comes back.  `nullptr` and a zero length mean the image
// was refused, which is the same answer `compress_image` gives.
uint8_t* BMFCodec::compress_to_memory(BmfImage* p_i, size_t* out_len,
                                      const CodedTail* tail) {
  // A codec is reusable because this line is here.  It is also why the tail is
  // a parameter: it zeroes `coded_block`, so a caller who had put one there
  // would have had it dropped between the two lines.
  reset();
  *out_len = 0;
  BmfFile arc;
  arc.io = BmfStream::in_memory();
  const int32_t coded = compress_image(&arc, p_i, tail);
  if( !coded ) {
    arc.close();
    return nullptr;
  }
  // `take` hands the buffer over and leaves the stream holding nothing, so
  // the `close` below has nothing left to free.
  uint8_t* out = arc.io.take(out_len);
  arc.close();
  return out;
}

int32_t BMFCodec::compress_image(BmfFile* arc_in, BmfImage* p_i, const CodedTail* extra_blk) {
  BmfImage hdr;
  if( !arc_in->io.fp&&!arc_in->io.buf )
    return 0;
  // The walk that was here read every member already in the output and left
  // the file positioned after the last, which is where an appended member had
  // to go.  `bmf c` writes the file, so there is nothing in front of this one.
  // The tail bit says a coded block precedes the payload.  This was a `!= 0`
  // into a byte and then an `if` that stored 1 over it when the pointer was
  // non-null -- which is the value the comparison had already put there.
  if( extra_blk )
    p_i->flags |= flags_tail;
  near_lossless_q = 0;
  hdr.header_from(p_i);
  hdr.flags = p_i->flags;
  plane_count = ((p_i->depth&depth_bits)+7)>>3;
  // An image member, version 2.0.  This was an `fwrite` of the first four
  // bytes of `"\x81\x8A" "20\x81\x90" "20a+b"` -- both signatures and three
  // more bytes in one literal, of which only the first four were ever written,
  // so the tag it writes could not be read off the line that writes it.
  const uint32_t tag = bmf_tag(bmf_sig_image, '2', '0');
  if( !arc_in->io.write_exact(&tag, 4u) )
    return 0;
  uint8_t bpp = p_i->depth;
  uint32_t pal_bytes = 0;
  if( bpp&depth_palette )
    pal_bytes = 3<<(bpp&31);
  // Anything shorter than the header describing it is not worth coding, and the
  // `goto write_raw` that said so jumped the whole of the block below.
  if( p_i->data_size>=0x10u ) {
    desc_slow_mode = 1;
    hdr.flags |= flags_coded|flags_slow;
    code_image_body(p_i, hdr);
    // Everything coded; flush the packer's last word and decide whether the
    // coded stream is smaller than the pixels it came from.  The byte count was
    // spelled out three times over -- once into `fits`, once into
    // `coded_bytes`, once into the header -- and nothing moves the cursor
    // between them.
    packer_flush();
    const int32_t coded_bytes = stream.cur-stream.buf;
    hdr.data_size = (uint32_t)coded_bytes;
    if( (uint32_t)coded_bytes<p_i->data_size ) {
      const uint8_t ok = write_member_head(&hdr, extra_blk, &arc_in->io);
      int32_t ok_all = arc_in->io.write_exact(stream.buf, (size_t)coded_bytes)&ok;
      free(stream.buf);
      if( ok_all&&(p_i->depth&depth_palette)!=0 )
        arc_in->io.write_all(&p_i->pixels[p_i->data_size], pal_bytes);
      arc_in->io.flush();
      if( ok_all )
        return (int32_t)hdr.data_size;
      return ok_all;
    }
    free(stream.buf);
    if( p_i->flags&flags_transposed )
      transpose_image(p_i, plane_count);
  }
  // The raw member: the image's own sixteen-byte header, which is the same
  // record laid out the same way, and then the pixels.  The transpose above
  // swaps the header's width, height and stride and leaves the pixel buffer
  // alone, so both ways in write the same bytes -- which is what the `pixels`
  // local said by being assigned the value it already held.
  const uint8_t ok_raw = write_member_head(p_i, extra_blk, &arc_in->io);
  uint32_t written = (uint32_t)arc_in->io.write(p_i->pixels, pal_bytes+p_i->data_size);
  int32_t data_size = p_i->data_size;
  if( !(ok_raw&(written==data_size+pal_bytes)) )
    return 0;
  return data_size;
}
