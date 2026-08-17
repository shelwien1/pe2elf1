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
// The plane descriptors, six bits each: two of reference count and four of
// flags, then the dc byte and the reference weights those flags call for.  The
// weights are written biased by 64 and read back the same way.
static void read_plane_descs() {
  for( int32_t pl = 0; pl<plane_count; ++pl ) {
    uint32_t desc = unpack_bits(6);
    int32_t nrefs = desc&3;
    plane_desc[pl].flags = desc>>2;
    plane_desc[pl].nrefs = nrefs;
    plane_desc[nrefs].src_plane = pl;
    if( !(plane_desc[pl].flags&desc_has_refs) )
      continue;
    plane_desc[pl].dc = unpack_bits(8);
    if( nrefs<=1 )
      continue;
    plane_desc[pl].weight0 = unpack_bits(8)-64;
    plane_desc[pl].weight1 = unpack_bits(8)-64;
    if( nrefs>2 )
      plane_desc[pl].weight2 = unpack_bits(8)-64;
  }
}

// Each plane coded on its own, so each is decoded through a header that says
// one eight-bit greyscale plane and interleaved back afterwards.
static void unmodel_planes_apart(BmfImage* img_at, uint8_t* plane_buf) {
  BmfImage plane_hdr = *img_at;
  plane_hdr.depth = depth_one_plane;
  for( int32_t k = 0; k<plane_count; ++k ) {
    const int32_t plane = plane_desc[k].src_plane;
    plane_predictor = plane_desc[plane].flags&desc_predictor;
    plane_alt_model = (plane_desc[plane].flags&desc_alt_model)!=0;
    unmodel_plane(&plane_hdr, plane_buf);
    // `pred && pred == pred_p1 && !alt` was three nested `if`s with the mode-0
    // arm hanging off the outermost `else`, which is one condition either way.
    if( !plane_predictor )
      expand_predictor_mode0(img_at->width);
    else if( plane_predictor==pred_p1&&!plane_alt_model )
      unpredict_med(plane_buf, img_at->width, img_at->height);
    interleave_plane(img_at, plane_buf, plane);
  }
}

// One coded image, then each plane's predictor undone in place: take the plane
// out, unpredict it, put it back.
static void unmodel_planes_together(BmfImage* img_at, uint8_t* plane_buf) {
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
    if( !pred )
      expand_predictor_mode0(img_at->width);
    else if( pred==pred_p1 )
      unpredict_med(plane_buf, img_at->width, img_at->height);
    interleave_plane(img_at, plane_buf, plane);
  }
}

// A stream that carries descriptors: the near-lossless nibble, the descriptors
// themselves, and then the planes, however they were coded.
static void unmodel_described(BmfImage* img_at, uint8_t flags) {
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
static bool expand_coded(BmfFile* arc_in, BmfImage* img_at, const BmfImage& hdr) {
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
  if( fread(stream.buf, 1u, hdr.data_size, arc_in->fp)!=want )
    return false;
  if( (img_at->depth&depth_bits)<=4||(hdr.flags&flags_descriptors)==0 ) {
    plane_predictor = 0;
    plane_alt_model = 0;
    unmodel_plane(img_at, img_at->pixels);
  } else {
    unmodel_described(img_at, hdr.flags);
  }
  if( stream.buf+hdr.data_size!=stream.cur )
    return false;
  free(stream.buf);
  return true;
}

BmfImage* expand_image(BmfFile* arc_in, CodedTail** p_coded_buf) {
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
  FILE *fp1 = (arc_in)->fp;
  if( !fp1 )
    return nullptr;
  // Skip any member that is not an image, by the length in its eight-byte
  // header, until one is.
  while( 1 ) {
    if( fread(&magic, 4u, 1u, fp1)!=1 ) {
      FILE *fp = arc_in->fp;
      if( feof(fp) )
        return nullptr;
      {
        fclose(fp);
        arc_in->fp = 0;
        return nullptr;
      }
    }
    if( (uint16_t)magic!=bmf_sig_other )
      break;
    version = bmf_tag_version(magic);
    if( version!=bmf_version_20||fread(&head, 8u, 1u, arc_in->fp)!=1 )
      break;
    fseek(arc_in->fp, head.len, 1);
    fp1 = arc_in->fp;
  }
  version = bmf_tag_version(magic);
  if( (uint16_t)magic!=bmf_sig_image||version!=bmf_version_20
      ||fread(&hdr, 0x10u, 1u, arc_in->fp)!=1 ) {
    return arc_in->fail();
  }
  if( hdr.flags&flags_tail ) {
    fread(&head, 8u, 1u, arc_in->fp);
    if( p_coded_buf ) {
      if( head.len>bytes_left(arc_in->fp) ) {
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
      fread(blk->data, head.len, 1u, arc_in->fp);
    } else {
      fseek(arc_in->fp, head.len, 1);
    }
  }
  if( hdr.data_size>bytes_left(arc_in->fp) ) {
    return arc_in->fail();
  }
  uint32_t pal_bytes = 3<<(hdr.depth&31);
  if( !(hdr.depth&depth_palette) )
    pal_bytes = 0;
  if( !hdr.width||!hdr.height ) {
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
    if( !expand_coded(arc_in, img_at, hdr) ) {
      return arc_in->fail();
    }
  } else {
    // Raw pixels.  `hdr.data_size` is what the file says follows and
    // `img_at->data_size` is what was allocated for it.
    if( hdr.data_size>img_at->data_size ) {
      return arc_in->fail();
    }
    uint32_t want = hdr.data_size;
    if( fread(img_at->pixels, 1u, hdr.data_size, arc_in->fp)!=want ) {
      return arc_in->fail();
    }
  }
  if( hdr.depth&depth_palette ) {
    uint8_t *pal_at = (img_at->depth&depth_palette) ? img_at->palette() : nullptr;
    uint32_t got = fread(pal_at, 1u, pal_bytes, arc_in->fp);
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
