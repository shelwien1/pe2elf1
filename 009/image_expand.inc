// image_expand.inc -- the image out of a `.bmf`.

BmfImage* expand_image(BmfFile* arc_in, CodedTail** p_coded_buf) {
  struct alignas(16) ExpandImageFrame {
    uint8_t* Block;
    union {
      BmfImage img;
      uint32_t mask;
      struct {
        uint32_t s0;
        int32_t s4;
        int32_t s8;
        uint32_t s12;
      };
    };
    int32_t nplanes_s;
    void* row_step;
    BmfFile* arc_f;
    BmfImage* p_i_2;
    int32_t depth_f;
    uint32_t pal_len;
    uint16_t hdr_words[5];
    uint8_t depth_b;
    int8_t flags_b;
    uint32_t data_len;
    CodedTail head;
    uint32_t magic_word;
    uint8_t _pad0[32];
  } frame;
  BmfFile* arc;
  FILE *fp1;
  BmfImage* img_at;
  uint8_t bpp, has_coded;
  uint32_t pred_s;
  uint8_t* pal_at, * copy, * srcp, * dst;
  int32_t plane_i, plane, pl2, plane2, pred, i, n_pix2, pix_at, nplanes_c, i2, n_planes, last_row, at, y, pl_i, left;
  uint16_t img_h;
  uint8_t major_v, minor_v;
  // The archive version, as a plain local.  The decompilation parked it in
  // `plane_desc[0].weight0` because the original did: `plane_desc[0]` is a
  // header slot, not a plane, and its three weight words were free.
  int32_t version, n_pix;
  uint32_t magic, pal_bytes, want2, got;
  uint8_t* plane_buf;
  arc = arc_in;
  if( p_coded_buf )
    *p_coded_buf = nullptr;
  fp1 = (arc_in)->fp;
  if( !fp1 )
    return nullptr;
  while( 1 ) {
    if( fread(&frame.magic_word, 4u, 1u, fp1)!=1 ) {
      FILE *fp = arc->fp;
      if( feof(fp) )
        return nullptr;
      {
        fclose(fp);
        arc->fp = 0;
        return nullptr;
      }
    }
    magic = frame.magic_word;
    if( (uint16_t)frame.magic_word!=0x9081 )
      break;
    major_v = (uint8_t)(frame.magic_word>>16);
    minor_v = (uint8_t)(frame.magic_word>>24);
    version = ((major_v<<8)-12288)|(minor_v-48);
    if( version!=512||fread(&frame.head, 8u, 1u, arc->fp)!=1 )
      break;
    fseek(arc->fp, frame.head.len, 1);
    fp1 = arc->fp;
  }
  if( (uint16_t)magic!=0x8A81||(major_v = (uint8_t)(magic>>16), minor_v = (uint8_t)(magic>>24), version = ((major_v<<8)-12288)|(minor_v-48), version!=512)||fread(frame.hdr_words, 0x10u, 1u, arc->fp)!=1 ) {
    return arc->fail();
  }
  if( frame.flags_b<0 ) {
    fread(&frame.head, 8u, 1u, arc->fp);
    if( p_coded_buf ) {
      if( frame.head.len>bytes_left(arc->fp) ) {
        return arc->fail();
      }
      // Up to a word, and up from zero to four: `(len==0)` is what makes an
      // empty tail one word rather than none, and the last word is cleared
      // because the read below stops at `len` and leaves the padding it wrote
      // nothing into.
      uint32_t pad_len = (frame.head.len+(frame.head.len==0)+3)&0xFFFFFFFC;
      CodedTail *blk = (CodedTail*)bmf_new(pad_len+sizeof(CodedTail));
      blk->tag = frame.head.tag;
      blk->len = pad_len;
      blk->data[pad_len/4-1] = 0;
      *p_coded_buf = blk;
      fread(blk->data, frame.head.len, 1u, arc->fp);
    } else {
      fseek(arc->fp, frame.head.len, 1);
    }
  }
  if( frame.data_len>bytes_left(arc->fp) ) {
    return arc->fail();
  }
  pal_bytes = 3<<(frame.depth_b&31);
  if( !(frame.depth_b&0x80) )
    pal_bytes = frame.depth_b&0x80;
  frame.pal_len = pal_bytes;
  if( !frame.hdr_words[0]||!frame.hdr_words[1] ) {
    return arc->fail();
  }
  img_at = alloc_image(frame.hdr_words[0], frame.hdr_words[1], frame.depth_b&0x3F, (uint8_t)(frame.depth_b&0x80)>>7, 1);
  frame.depth_f = frame.depth_b;
  img_at->depth = frame.depth_b;
  has_coded = p_coded_buf&&*p_coded_buf;
  int8_t hdr_flags = frame.flags_b;
  img_at->flags |= frame.flags_b&2|(has_coded<<7);
  plane_count = ((frame.depth_f&0x3Fu)+7)>>3;
  if( plane_count<1||plane_count>4 ) {
    return arc->fail();
  }
  if( !(hdr_flags&0x20) ) {
    if( frame.data_len>img_at->data_size ) {
      return arc->fail();
    }
    uint32_t want = frame.data_len;
    if( fread(img_at->pixels, 1u, frame.data_len, arc->fp)!=want ) {
      return arc->fail();
    }
    goto read_palette;
  }
  near_lossless_q = 0;
  alphabet_reduced = 0;
  if( !(hdr_flags&4) ) {
    printf("\nwritten in fast mode; this build only decodes -S streams\n");
    exit(3);
  }
  desc_slow_mode = 1;
  stream.size = frame.data_len;
  stream.buf = (uint8_t*)bmf_new(frame.data_len);
  packer_reset();
  want2 = frame.data_len;
  if( fread(stream.buf, 1u, frame.data_len, arc->fp)!=want2 ) {
    return arc->fail();
  }
  bpp = img_at->depth;
  if( (bpp&0x3Fu)<=4||(frame.flags_b&0x10)==0 ) {
    plane_predictor = 0;
    plane_alt_model = 0;
    unmodel_plane(img_at, img_at->pixels);
    goto check_length;
  }
  // The near-lossless nibble, mirroring `image_compress`: present for a single
  // greyscale plane and for three or more, absent otherwise.  Two `goto`s
  // jumped over it; the condition is the encoder's, negated.
  if( plane_count==1 ? (bpp&0x40)!=0 : plane_count>2 ) {
    uint32_t near_lossless = unpack_bits(4);
    if( near_lossless ) {
      printf("\nnear-lossless stream (E=%d); this build only decodes E=0\n", near_lossless);
      exit(3);
    }
    near_lossless_q = 0;
  }
  if( plane_count>0 ) {
    frame.mask = 255;
    frame.p_i_2 = img_at;
    frame.arc_f = arc;
    int32_t pl = 0;
    do {
      uint32_t desc = unpack_bits(6);
      uint32_t desc_flags = desc>>2;
      int32_t nrefs = desc&3;
      plane_desc[pl].flags = desc_flags;
      plane_desc[pl].nrefs = nrefs;
      plane_desc[nrefs].src_plane = pl;
      if( plane_desc[pl].flags&8 ) {
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
    img_at = frame.p_i_2;
    arc = frame.arc_f;
  }
  plane_buf = (uint8_t*)bmf_new(img_at->width*img_at->height);
  if( frame.flags_b&8 ) {
    frame.img = *img_at;
    frame.img.depth = 72;
    if( plane_count>0 ) {
      frame.arc_f = arc;
      plane_i = 0;
      do {
        plane = plane_desc[plane_i].src_plane;
        plane_predictor = plane_desc[plane].flags&3;
        plane_alt_model = (uint8_t)(plane_desc[plane].flags&4)>>2;
        unmodel_plane(&frame.img, plane_buf);
        if( plane_predictor ) {
          if( plane_predictor==1 ) {
            if( !plane_alt_model )
              unpredict_med(plane_buf, img_at->width, img_at->height);
          }
        } else {
          expand_predictor_mode0(img_at->width);
        }
        interleave_plane(img_at, plane_buf, plane);
        ++plane_i;
      } while( plane_i<plane_count );
restore_arc:
      arc = frame.arc_f;
    }
  } else {
    plane_predictor = plane_desc[0].flags&3;
    plane_alt_model = (uint8_t)(plane_desc[0].flags&4)>>2;
    {
      unmodel_plane(img_at, img_at->pixels);
      if( plane_alt_model )
        goto free_plane_buf;
    }
    if( plane_count>0 ) {
      frame.arc_f = arc;
      pl2 = 0;
      do {
        ++pl2;
        plane2 = plane_desc[pl2-1].src_plane;
        pred = plane_desc[plane2].flags&3;
        plane_predictor = pred;
        if( (plane_desc[plane2].flags&8)!=0||pred ) {
          i = img_at->width;
          frame.row_step = &img_at->pixels[plane2];
          n_pix = i*img_at->height;
          frame.nplanes_s = plane_count;
          frame.s12 = n_pix;
          if( plane_count==1 ) {
            memcpy(plane_buf, (uint8_t*)frame.row_step, frame.s12);
            pred_s = plane_predictor;
          } else {
            frame.Block = &img_at->pixels[plane2];
            frame.s0 = pred;
            frame.s4 = plane2;
            frame.s8 = pl2;
            n_pix2 = frame.s12;
            frame.p_i_2 = img_at;
            pix_at = 0;
            nplanes_c = frame.nplanes_s;
            i2 = 0;
            do {
              plane_buf[pix_at] = frame.Block[i2];
              i2 += nplanes_c;
              ++pix_at;
            } while( pix_at<n_pix2 );
            pred_s = frame.s0;
            plane2 = frame.s4;
            pl2 = frame.s8;
            img_at = frame.p_i_2;
          }
          if( pred_s ) {
            if( pred_s==1 )
              unpredict_med(plane_buf, img_at->width, img_at->height);
          } else {
            expand_predictor_mode0(img_at->width);
          }
          interleave_plane(img_at, plane_buf, plane2);
        }
      } while( pl2<plane_count );
      goto restore_arc;
    }
  }
free_plane_buf:
  free(plane_buf);
check_length:
  if( stream.buf+frame.data_len!=stream.cur ) {
    return arc->fail();
  }
  free(stream.buf);
  frame.depth_f = frame.depth_b;
read_palette:
  if( frame.depth_f&0x80 ) {
    pal_at = (img_at->depth&depth_palette) ? img_at->palette() : nullptr;
    got = fread(pal_at, 1u, frame.pal_len, arc->fp);
    if( got!=frame.pal_len ) {
      return arc->fail();
    }
  }
  if( img_at->flags&flags_transposed ) {
    if( (uint64_t)img_at->width*img_at->height*(uint32_t)plane_count>(uint64_t)img_at->data_size ) {
      return arc->fail();
    }
    copy = (uint8_t*)bmf_new(img_at->data_size);
    n_planes = plane_count;
    last_row = plane_count*(img_at->height-1);
    memcpy(copy, img_at->pixels, img_at->data_size);
    img_h = img_at->height;
    if( img_h ) {
      srcp = copy;
      at = 0;
      y = 0;
      do {
        pl_i = img_at->width;
        frame.p_i_2 = img_at;
        dst = &img_at->pixels[at];
        do {
          left = n_planes;
          do {
            *dst++ = *srcp++;
            --left;
          } while( left );
          dst += last_row;
          --pl_i;
        } while( pl_i );
        img_at = frame.p_i_2;
        img_h = frame.p_i_2->height;
        at += n_planes;
        ++y;
      } while( y<img_h );
    }
    transpose_image_in_place(img_at, n_planes);
    free(copy);
  }
  return img_at;
}
