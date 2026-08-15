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
//     fread(frame.hdr_words, 0x10u, 1u, arc->fp)
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
BmfImage* expand_image(BmfFile* arc_in, CodedTail** p_coded_buf) {
  // The member header, as it sits in the file.  `data_size` is the *coded*
  // byte count here, not the pixel count: the two are compared below, and the
  // one that came off the wire is this one.
  BmfImage hdr;
  CodedTail head;
  BmfFile* arc;
  FILE *fp1;
  BmfImage* img_at;
  uint8_t bpp, has_coded;
  uint8_t* pal_at;
  // The archive version, as a plain local.  The decompilation parked it in
  // `plane_desc[0].weight0` because the original did: `plane_desc[0]` is a
  // header slot, not a plane, and its three weight words were free.
  int32_t version;
  uint32_t magic, pal_bytes, want2, got;
  uint8_t* plane_buf;
  arc = arc_in;
  if( p_coded_buf )
    *p_coded_buf = nullptr;
  fp1 = (arc_in)->fp;
  if( !fp1 )
    return nullptr;
  // Skip any member that is not an image, by the length in its eight-byte
  // header, until one is.
  while( 1 ) {
    if( fread(&magic, 4u, 1u, fp1)!=1 ) {
      FILE *fp = arc->fp;
      if( feof(fp) )
        return nullptr;
      {
        fclose(fp);
        arc->fp = 0;
        return nullptr;
      }
    }
    if( (uint16_t)magic!=bmf_sig_other )
      break;
    version = bmf_tag_version(magic);
    if( version!=bmf_version_20||fread(&head, 8u, 1u, arc->fp)!=1 )
      break;
    fseek(arc->fp, head.len, 1);
    fp1 = arc->fp;
  }
  version = bmf_tag_version(magic);
  if( (uint16_t)magic!=bmf_sig_image||version!=bmf_version_20
      ||fread(&hdr, 0x10u, 1u, arc->fp)!=1 ) {
    return arc->fail();
  }
  if( hdr.flags&flags_tail ) {
    fread(&head, 8u, 1u, arc->fp);
    if( p_coded_buf ) {
      if( head.len>bytes_left(arc->fp) ) {
        return arc->fail();
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
      fread(blk->data, head.len, 1u, arc->fp);
    } else {
      fseek(arc->fp, head.len, 1);
    }
  }
  if( hdr.data_size>bytes_left(arc->fp) ) {
    return arc->fail();
  }
  pal_bytes = 3<<(hdr.depth&31);
  if( !(hdr.depth&depth_palette) )
    pal_bytes = 0;
  if( !hdr.width||!hdr.height ) {
    return arc->fail();
  }
  img_at = alloc_image(hdr.width, hdr.height, hdr.depth&depth_bits, (hdr.depth&depth_palette)!=0, 1);
  img_at->depth = hdr.depth;
  has_coded = p_coded_buf&&*p_coded_buf;
  img_at->flags |= (hdr.flags&flags_transposed)|(has_coded ? flags_tail : 0);
  plane_count = ((hdr.depth&depth_bits)+7)>>3;
  if( plane_count<1||plane_count>4 ) {
    return arc->fail();
  }
  if( !(hdr.flags&flags_coded) ) {
    // Raw pixels.  `hdr.data_size` is what the file says follows and
    // `img_at->data_size` is what was allocated for it.
    if( hdr.data_size>img_at->data_size ) {
      return arc->fail();
    }
    uint32_t want = hdr.data_size;
    if( fread(img_at->pixels, 1u, hdr.data_size, arc->fp)!=want ) {
      return arc->fail();
    }
    goto read_palette;
  }
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
  want2 = hdr.data_size;
  if( fread(stream.buf, 1u, hdr.data_size, arc->fp)!=want2 ) {
    return arc->fail();
  }
  bpp = img_at->depth;
  if( (bpp&depth_bits)<=4||(hdr.flags&flags_descriptors)==0 ) {
    plane_predictor = 0;
    plane_alt_model = 0;
    unmodel_plane(img_at, img_at->pixels);
    goto check_length;
  }
  // The near-lossless nibble, mirroring `image_compress`: present for a single
  // greyscale plane and for three or more, absent otherwise.  Two `goto`s
  // jumped over it; the condition is the encoder's, negated.
  if( plane_count==1 ? (bpp&depth_grey)!=0 : plane_count>2 ) {
    uint32_t near_lossless = unpack_bits(4);
    if( near_lossless ) {
      printf("\nnear-lossless stream (E=%d); this build only decodes E=0\n", near_lossless);
      exit(3);
    }
    near_lossless_q = 0;
  }
  if( plane_count>0 ) {
    int32_t pl = 0;
    do {
      uint32_t desc = unpack_bits(6);
      uint32_t desc_flags = desc>>2;
      int32_t nrefs = desc&3;
      plane_desc[pl].flags = desc_flags;
      plane_desc[pl].nrefs = nrefs;
      plane_desc[nrefs].src_plane = pl;
      if( plane_desc[pl].flags&desc_has_refs ) {
        uint8_t dc_v = unpack_bits(8);
        plane_desc[pl].dc = dc_v;
        if( nrefs>1 ) {
          uint32_t w4_v = unpack_bits(8);
          plane_desc[pl].weight0 = w4_v-64;
          uint32_t w8_v = unpack_bits(8);
          plane_desc[pl].weight1 = w8_v-64;
          if( nrefs>2 ) {
            uint32_t w12_v = unpack_bits(8);
            plane_desc[pl].weight2 = w12_v-64;
          }
        }
      }
      ++pl;
    } while( pl<plane_count );
  }
  plane_buf = (uint8_t*)bmf_new(img_at->width*img_at->height);
  if( hdr.flags&flags_planar ) {
    // Each plane coded on its own, so each is decoded through a header that
    // says one eight-bit greyscale plane and interleaved back afterwards.
    BmfImage plane_hdr = *img_at;
    plane_hdr.depth = depth_one_plane;
    if( plane_count>0 ) {
      int32_t plane_i = 0;
      do {
        int32_t plane = plane_desc[plane_i].src_plane;
        plane_predictor = plane_desc[plane].flags&desc_predictor;
        plane_alt_model = (plane_desc[plane].flags&desc_alt_model)!=0;
        unmodel_plane(&plane_hdr, plane_buf);
        if( plane_predictor ) {
          if( plane_predictor==pred_p1 ) {
            if( !plane_alt_model )
              unpredict_med(plane_buf, img_at->width, img_at->height);
          }
        } else {
          expand_predictor_mode0(img_at->width);
        }
        interleave_plane(img_at, plane_buf, plane);
        ++plane_i;
      } while( plane_i<plane_count );
    }
  } else {
    // One coded image, then each plane's predictor undone in place: take the
    // plane out, unpredict it, put it back.
    plane_predictor = plane_desc[0].flags&desc_predictor;
    plane_alt_model = (plane_desc[0].flags&desc_alt_model)!=0;
    unmodel_plane(img_at, img_at->pixels);
    if( plane_alt_model )
      goto free_plane_buf;
    if( plane_count>0 ) {
      int32_t pl2 = 0;
      do {
        ++pl2;
        int32_t plane2 = plane_desc[pl2-1].src_plane;
        int32_t pred = plane_desc[plane2].flags&desc_predictor;
        plane_predictor = pred;
        if( (plane_desc[plane2].flags&desc_has_refs)!=0||pred ) {
          deinterleave_plane(img_at, plane_buf, plane2);
          if( pred ) {
            if( pred==pred_p1 )
              unpredict_med(plane_buf, img_at->width, img_at->height);
          } else {
            expand_predictor_mode0(img_at->width);
          }
          interleave_plane(img_at, plane_buf, plane2);
        }
      } while( pl2<plane_count );
    }
  }
free_plane_buf:
  free(plane_buf);
check_length:
  if( stream.buf+hdr.data_size!=stream.cur ) {
    return arc->fail();
  }
  free(stream.buf);
read_palette:
  if( hdr.depth&depth_palette ) {
    pal_at = (img_at->depth&depth_palette) ? img_at->palette() : nullptr;
    got = fread(pal_at, 1u, pal_bytes, arc->fp);
    if( got!=pal_bytes ) {
      return arc->fail();
    }
  }
  if( img_at->flags&flags_transposed ) {
    if( (uint64_t)img_at->width*img_at->height*(uint32_t)plane_count>(uint64_t)img_at->data_size ) {
      return arc->fail();
    }
    transpose_image(img_at, plane_count);
  }
  return img_at;
}
