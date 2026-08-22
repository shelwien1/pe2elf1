// image_expand.inc -- the image out of a `.bmf`.

// This frame was `liftframe.py`'s one standing failure.  Lifting it made
// seventeen of nineteen images fail to expand at all -- `expand exits 3`, the
// bad-file code -- which is not a stream that moved but the header parse
// itself ceasing to work, and the entry recorded that as the frame's layout
// being what the parse reads through.  It was, and the whole of it is one
// `fread`:
//
//     uint16_t hdr_words[5];
//     uint8_t depth_b;
//     uint8_t flags_b;
//     uint32_t data_len;
//     ...
//     fread(frame.hdr_words, 0x10u, 1u, arc_in->fp)
//
// Sixteen bytes across four members, of which the array is the first ten.  So
// the four are one object and the array's size was never five of anything:
// they are `BmfImage`'s `width`, `height`, `stride` and `_pad8`, then its
// `depth`, `flags` and `data_size`, at exactly those offsets.
// `compress_image` writes that same record with `fwrite(&hdr, 1u, 0x10u, …)`
// from a `BmfImage`, so the writer knew what it was and only the reader did
// not.
//
// That is the `read_bmp` finding one file over -- two `fread`s written across
// slots Hex-Rays had split a record into -- and it is the frame's only layout
// dependency.  With the header a record the rest lifts: a `CodedTail`, a
// `BmfImage` scratch for the one-plane decode, and fourteen spill slots each
// holding a copy of a local that is still live at the read.

// One packed field of the descriptor block, either way.  `bias` goes on on the
// way out and comes off on the way back, which is the whole of the difference
// between the two loops below.  A reference and not a value in and out, so
// that a `uint8_t` field and an `int32_t` one are the same call.
template<int32_t f_DEC, typename T>
void BMFCodec::code_field(T& v, int32_t bits, int32_t bias) {
  if constexpr( f_DEC )
    v = (T)((int32_t)unpack_bits(bits)-bias);
  else
    pack_bits((uint32_t)((int32_t)v+bias), bits);
}

// **The plane descriptors, and the one format both directions spell.**  Six
// bits of reference count and flags, then the dc byte and the reference
// weights those flags call for, each biased by 64.
//
// This was two bodies with a comment apiece describing the same six fields --
// which is the duplication saying so out loud, a paragraph written twice
// because the code was.  Field for field the two agree: the same loop, the
// same two `continue` guards on the same conditions, the same widths in the
// same order, the same bias.
//
// **Their measured share was 2 lines of 24, and that number meant nothing.**
// `pack_bits(w+64, 8)` and `unpack_bits(8)-64` have no text in common, so a
// longest-common-subsequence over lines scores a pair like this at the floor
// however identical it is -- the same way it scored `code_colour_plane`, whose
// two halves were also one computation under four spellings.  Neither of these
// was ever on the table above `bmf.cpp`: nothing enumerated the pairs, so a
// pair nobody had named was a pair nobody had measured.  `tools/pairnames.py`
// is what closes that, and it is what found this one.
//
// One line is genuinely the decoder's alone.  `src_plane` is not in the stream
// -- the encoder gets it from the filter search, in `plane_choose.inc` and
// `filter_cost.inc` -- so the decoder rebuilds it here from the count it has
// just read.
template<int32_t f_DEC>
void BMFCodec::code_plane_descs() {
  for( int32_t pl = 0; pl<plane_count; ++pl ) {
    PlaneDesc& d = plane_desc[pl];
    uint32_t head = (uint32_t)((d.flags<<2)|d.nrefs);
    code_field<f_DEC>(head, 6, 0);
    if constexpr( f_DEC ) {
      d.flags = (uint8_t)(head>>2);
      d.nrefs = (uint8_t)(head&3);
      plane_desc[head&3].src_plane = (uint8_t)pl;
    }
    if( !(d.flags&desc_has_refs) )
      continue;
    code_field<f_DEC>(d.dc, 8, 0);
    if( d.nrefs<=1u )
      continue;
    code_field<f_DEC>(d.weight0, 8, 64);
    code_field<f_DEC>(d.weight1, 8, 64);
    if( d.nrefs>2u )
      code_field<f_DEC>(d.weight2, 8, 64);
  }
}

void BMFCodec::read_plane_descs() {
  code_plane_descs<1>();
}

void BMFCodec::write_plane_descs() {
  code_plane_descs<0>();
}

// Each plane coded on its own, so each is decoded through a header that says
// one eight-bit greyscale plane and interleaved back afterwards.
void BMFCodec::unmodel_planes_apart(BmfImage* img_at, uint8_t* plane_buf) {
  BmfImage plane_hdr = *img_at;
  plane_hdr.depth = depth_one_plane;
  for( int32_t k = 0; k<plane_count; ++k ) {
    const int32_t plane = plane_desc[k].src_plane;
    plane_predictor = plane_desc[plane].flags&desc_predictor;
    plane_alt_model = (plane_desc[plane].flags&desc_alt_model)!=0;
    unmodel_plane(&plane_hdr, plane_buf);
    // `pred && pred == pred_p1 && !alt` was three nested `if`s with the mode-0
    // arm hanging off the outermost `else`, which is one condition either way.
    //
    // That arm called `expand_predictor_mode0(width)`, whose body is empty.
    // It is not a format hook and not a table entry -- both call sites are the
    // `else` of an `if`/`else if` chain, and `pred_p1` is 1, so testing for it
    // already excludes mode 0.  The function, its declaration and both calls
    // are gone; the enumerator stays, because it is a value in the stream.
    if( plane_predictor==pred_p1&&!plane_alt_model )
      unpredict_med(plane_buf, img_at->width, img_at->height);
    interleave_plane(img_at, plane_buf, plane);
  }
}

// One coded image, then each plane's predictor undone in place: take the plane
// out, unpredict it, put it back.
void BMFCodec::unmodel_planes_together(BmfImage* img_at, uint8_t* plane_buf) {
  plane_predictor = plane_desc[0].flags&desc_predictor;
  plane_alt_model = (plane_desc[0].flags&desc_alt_model)!=0;
  unmodel_plane(img_at, img_at->pixels);
  // The alternate model leaves this loop nothing to undo; the recovery said so
  // by jumping past it to the `free` that follows, which is the caller's now.
  if( plane_alt_model )
    return;
  for( int32_t k = 0; k<plane_count; ++k ) {
    const int32_t plane = plane_in_order(k);
    const int32_t pred = plane_predictor;
    if( (plane_desc[plane].flags&desc_has_refs)==0&&!pred )
      continue;
    deinterleave_plane(img_at, plane_buf, plane);
    if( pred==pred_p1 )
      unpredict_med(plane_buf, img_at->width, img_at->height);
    interleave_plane(img_at, plane_buf, plane);
  }
}

// A stream that carries descriptors: the near-lossless nibble, the descriptors
// themselves, and then the planes, however they were coded.
void BMFCodec::unmodel_described(BmfImage* img_at, uint8_t flags) {
  // The near-lossless nibble, mirroring `image_compress`: present for a single
  // greyscale plane and for three or more, absent otherwise.  Two `goto`s
  // jumped over it; the condition is the encoder's, negated.
  if( plane_count==1 ? (img_at->depth&depth_grey)!=0 : plane_count>2 ) {
    uint32_t near_lossless = unpack_bits(4);
    if( near_lossless ) {
      printf("\nnear-lossless stream (E=%d); this build only decodes E=0\n", near_lossless);
      exit(3);
    }
    near_lossless_q = 0;
  }
  read_plane_descs();
  uint8_t *plane_buf = (uint8_t*)bmf_new(img_at->width*img_at->height);
  if( flags&flags_planar )
    unmodel_planes_apart(img_at, plane_buf);
  else
    unmodel_planes_together(img_at, plane_buf);
  free(plane_buf);
}

// The coded body of a member: the packed stream read whole, then decoded into
// `img_at`.  False when the file is short of what the header promised, or when
// the stream did not end where its own length said it would -- the caller turns
// either into `fail()`, which is what each was before.
//
// `FILE*` and not the `BmfFile*` the caller holds: the archive is what
// `expand_image` reads members out of, and this reads bytes out of a stream.
// The lift took `arc_in` at first, which left a first parameter every use of
// which was `arc_in->fp` -- `methodise.py`'s rule for a receiver, and the
// wrong reading, since `arc_in->expand_coded(...)` would be the archive
// expanding an image.  Its opposite number is declined in that tool for the
// same sentence; this one did not have to be.
bool BMFCodec::expand_coded(BmfStream* io, BmfImage* img_at, const BmfImage& hdr) {
  near_lossless_q = 0;
  alphabet_reduced = 0;
  if( !(hdr.flags&flags_slow) ) {
    printf("\nwritten in fast mode; this build only decodes -S streams\n");
    exit(3);
  }
  desc_slow_mode = 1;
  stream.size = hdr.data_size;
  stream.buf = (uint8_t*)bmf_new(hdr.data_size);
  packer_reset();
  uint32_t want = hdr.data_size;
  // **One exit and one `free`, which is a change of shape and not of
  // behaviour.**  Both refusals below used to `return false` with the coded
  // buffer still allocated, and for as long as refusing a stream was the last
  // thing a process ever did that cost nothing.  `expand_from_memory` is what
  // makes it cost something: the same codec is handed the next stream, so a
  // caller expanding a hundred and refusing some of them leaked `data_size`
  // bytes -- an attacker's number -- for each one.
  //
  // Justified by reading and not by a probe that went red, and that is worth
  // being plain about.  The first refusal cannot happen at all: `expand_image`
  // has already compared `data_size` against what is left in the stream.  The
  // second needs a stream of the right length whose contents decode to a
  // different number of bytes, and the decoder calls `bmf_fatal` on most of
  // those before it gets here.  Nothing in this tree provokes it; what stands
  // is that a `bmf_new` on one path and a `free` on one of three is wrong on
  // its face.
  bool ok = io->read(stream.buf, hdr.data_size)==want;
  if( ok ) {
    if( (img_at->depth&depth_bits)<=4||(hdr.flags&flags_descriptors)==0 ) {
      plane_predictor = 0;
      plane_alt_model = 0;
      unmodel_plane(img_at, img_at->pixels);
    } else {
      unmodel_described(img_at, hdr.flags);
    }
    ok = stream.buf+hdr.data_size==stream.cur;
  }
  free(stream.buf);
  stream.buf = nullptr;
  return ok;
}

// **The in-memory entry point**, and the other half of what a caller wants.
// `BmfStream::over_memory` is `in_memory`'s mirror: a stream reading a buffer
// the caller owns, which is the shape `expand_image` already takes.  The file
// layer is untouched and the codec gains an entry point rather than a fork.
// The buffer is the caller's and stays the caller's -- `close` on a stream made
// this way frees nothing, because `cap` is zero and only a grown buffer is ours.
//
// The caller frees the image.  `nullptr` means the stream was refused, which is
// what `expand_image` answers and what the malformed-stream leg of `test.sh`
// depends on.
//
// **`nullptr` is not the only way this fails, and a caller has to know it.**  A
// stream that can be rejected by inspection -- a bad signature, a length past
// the end of the buffer, a depth outside 1..4 -- comes back as `nullptr` and
// leaves the codec usable.  A stream that only falls apart once the arithmetic
// decoder is inside it calls `bmf_fatal`, which is `noreturn`, and takes the
// process with it.  That is the original program's design, not this round's,
// and it is the one thing an in-memory entry point cannot paper over: the
// decoder has no way to unwind.  `tools/parallel.cpp` checks the recoverable
// half sixteen times over on one codec and says the same thing at more length.
BmfImage* BMFCodec::expand_from_memory(const uint8_t* data, size_t n,
                                       CodedTail** tail) {
  // As `compress_to_memory`: each call starts from a fresh codec.
  reset();
  BmfFile arc;
  arc.io = BmfStream::over_memory(data, n);
  // `&coded_block` and not `tail` directly, because a null one means something
  // different down there: `expand_image` reading no tail also clears
  // `flags_tail` off the image it answers, so a caller that did not ask for the
  // block would get a *different image* rather than the same one without it.
  BmfImage* img = expand_image(&arc, &coded_block);
  arc.close();
  // The block is the caller's if it asked and nobody's otherwise -- and nobody's
  // has to mean freed here, because the next call's `reset()` zeroes the member
  // without freeing it and every stream carrying a tail would leak one.
  if( tail )
    *tail = coded_block;
  else
    free(coded_block);
  coded_block = nullptr;
  return img;
}

BmfImage* BMFCodec::expand_image(BmfFile* arc_in, CodedTail** p_coded_buf) {
  // The member header, as it sits in the file.  `data_size` is the *coded*
  // byte count here, not the pixel count: the two are compared below, and the
  // one that came off the wire is this one.
  BmfImage hdr;
  CodedTail head;
  // The archive version, as a plain local.  The decompilation parked it in
  // `plane_desc[0].weight0` because the original did: `plane_desc[0]` is a
  // header slot, not a plane, and its three weight words were free.
  int32_t version;
  uint32_t magic;
  if( p_coded_buf )
    *p_coded_buf = nullptr;
  BmfStream* io = &arc_in->io;
  if( !io->fp&&!io->buf )
    return nullptr;
  // Skip any member that is not an image, by the length in its eight-byte
  // header, until one is.
  while( 1 ) {
    if( !io->read_exact(&magic, 4u) ) {
      // Out of input either way; the only difference was whether the stream
      // got closed on the way out, and `fail()` closes it.
      return io->eof() ? nullptr : arc_in->fail();
    }
    if( (uint16_t)magic!=bmf_sig_other )
      break;
    version = bmf_tag_version(magic);
    if( version!=bmf_version_20||!io->read_exact(&head, 8u) )
      break;
    io->seek(head.len, SEEK_CUR);
  }
  version = bmf_tag_version(magic);
  if( (uint16_t)magic!=bmf_sig_image||version!=bmf_version_20
      ||!io->read_exact(&hdr, 0x10u) ) {
    return arc_in->fail();
  }
  if( hdr.flags&flags_tail ) {
    if( !io->read_exact(&head, 8u) )
      return arc_in->fail();
    if( p_coded_buf ) {
      if( head.len>io->bytes_left() ) {
        return arc_in->fail();
      }
      // Up to a word, and up from zero to four: `(len==0)` is what makes an
      // empty tail one word rather than none, and the last word is cleared
      // because the read below stops at `len` and leaves the padding it wrote
      // nothing into.
      uint32_t pad_len = (head.len+(head.len==0)+3)&0xFFFFFFFC;
      CodedTail *blk = (CodedTail*)bmf_new(pad_len+sizeof(CodedTail));
      blk->tag = head.tag;
      blk->len = pad_len;
      blk->data[pad_len/4-1] = 0;
      *p_coded_buf = blk;
      if( !io->read_exact(blk->data, head.len) )
        return arc_in->fail();
    } else {
      io->seek(head.len, SEEK_CUR);
    }
  }
  if( hdr.data_size>io->bytes_left() ) {
    return arc_in->fail();
  }
  uint32_t pal_bytes = 3<<(hdr.depth&31);
  if( !(hdr.depth&depth_palette) )
    pal_bytes = 0;
  // Stage-2 11.3.  `read_bmp` refuses a source whose row will not fit in
  // sixteen bits; this did not, and `width` is `uint16_t`, so a header claiming
  // 16384 pixels at 32 bits asked for a 262140-byte row and `alloc_image`
  // truncated it to 0.  A zero stride is not caught anywhere downstream -- every
  // row of the image is then row 0.
  //
  // The same predicate as the reader's, written the same way round, and it can
  // only fire on a crafted stream: nothing this program writes gets here,
  // because the encoder refused the source that would have produced it.
  if( !hdr.width||!hdr.height
   || (uint32_t)hdr.width*(uint32_t)((((hdr.depth&depth_bits)+7)>>3))>0xFFFFu ) {
    return arc_in->fail();
  }
  BmfImage *img_at = alloc_image(hdr.width, hdr.height, hdr.depth&depth_bits, (hdr.depth&depth_palette)!=0, 1);
  img_at->depth = hdr.depth;
  uint8_t has_coded = p_coded_buf&&*p_coded_buf;
  img_at->flags |= (hdr.flags&flags_transposed)|(has_coded ? flags_tail : 0);
  plane_count = ((hdr.depth&depth_bits)+7)>>3;
  if( plane_count<1||plane_count>4 ) {
    return arc_in->fail();
  }
  if( hdr.flags&flags_coded ) {
    if( !expand_coded(io, img_at, hdr) ) {
      return arc_in->fail();
    }
  } else {
    // Raw pixels.  `hdr.data_size` is what the file says follows and
    // `img_at->data_size` is what was allocated for it.
    if( hdr.data_size>img_at->data_size ) {
      return arc_in->fail();
    }
    uint32_t want = hdr.data_size;
    if( io->read(img_at->pixels, hdr.data_size)!=want ) {
      return arc_in->fail();
    }
  }
  if( hdr.depth&depth_palette ) {
    uint8_t *pal_at = (img_at->depth&depth_palette) ? img_at->palette() : nullptr;
    uint32_t got = (uint32_t)io->read(pal_at, pal_bytes);
    if( got!=pal_bytes ) {
      return arc_in->fail();
    }
  }
  if( img_at->flags&flags_transposed ) {
    if( (uint64_t)img_at->width*img_at->height*(uint32_t)plane_count>(uint64_t)img_at->data_size ) {
      return arc_in->fail();
    }
    transpose_image(img_at, plane_count);
  }
  return img_at;
}
