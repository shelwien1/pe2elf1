// image_expand.inc -- the image out of a `.bmf`.

uint8_t*__expand_image(BmfFile* arc_in, void** p_coded_buf) {
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
    uint8_t hdr[8];
    uint32_t magic_word;
    uint8_t _pad0[32];
  } __frame;
  BmfFile* arc;
  FILE* fp1, * fp;
  BmfImage* img_at;
  uint8_t bpp, has_coded, dc_v;
  uint32_t near_lossless, pred_s, w12_v, w4_v, w8_v;
  uint8_t* pal_at, * copy, * srcp, * dst;
  int32_t pl, nrefs, plane_i, plane, pl2, plane2, pred, i, n_pix2, pix_at, nplanes_c, i2, n_planes, last_row, at, y, pl_i, left;
  uint16_t img_h, w16;
  uint8_t major_v, minor_v;
  // The archive version, as a plain local.  The decompilation parked it in
  // `plane_desc[0].weight0` because the original did: `plane_desc[0]` is a
  // header slot, not a plane, and its three weight words were free.
  int32_t version;
  int32_t n_pix;
  uint32_t magic, pal_bytes, want2, desc, desc_flags, got;
  uint8_t* plane_buf;
  arc = arc_in;
  if( p_coded_buf )
    *p_coded_buf = nullptr;
  fp1 = (arc_in)->fp;
  if( !fp1 )
    return nullptr;
  while( 1 ) {
    if( fread(&__frame.magic_word, 4u, 1u, fp1)!=1 ) {
      fp = arc->fp;
      if( feof(fp) )
        return nullptr;
      {
        fclose(fp);
        arc->fp = 0;
        return nullptr;
      }
    }
    magic = __frame.magic_word;
    if( (uint16_t)__frame.magic_word!=0x9081 )
      break;
    major_v = (uint8_t)(__frame.magic_word>>16);
    minor_v = (uint8_t)(__frame.magic_word>>24);
    version = ((major_v<<8)-12288)|(minor_v-48);
    if( version!=512||fread(__frame.hdr, 8u, 1u, arc->fp)!=1 )
      break;
    fseek(arc->fp, (*(uint32_t*)&__frame.hdr[4]), 1);
    fp1 = arc->fp;
  }
  if( (uint16_t)magic!=0x8A81||(major_v = (uint8_t)(magic>>16), minor_v = (uint8_t)(magic>>24), version = ((major_v<<8)-12288)|(minor_v-48), version!=512)||fread(__frame.hdr_words, 0x10u, 1u, arc->fp)!=1 ) {
    return arc->fail();
  }
  if( __frame.flags_b<0 ) {
    fread(__frame.hdr, 8u, 1u, arc->fp);
    if( p_coded_buf ) {
      int32_t hdr_word = *(int32_t*)&__frame.hdr[0];
      if( (*(uint32_t*)&__frame.hdr[4])>__bytes_left(arc->fp) ) {
        return arc->fail();
      }
      uint32_t pad_len = ((*(uint32_t*)&__frame.hdr[4])+((*(uint32_t*)&__frame.hdr[4])==0)+3)&0xFFFFFFFC;
      uint32_t *blk = (uint32_t*)bmf_new(pad_len+8);
      *blk = hdr_word;
      blk[1] = pad_len;
      blk[pad_len/4+1] = 0;
      *p_coded_buf = blk;
      fread(blk+2, (*(uint32_t*)&__frame.hdr[4]), 1u, arc->fp);
    } else {
      fseek(arc->fp, (*(uint32_t*)&__frame.hdr[4]), 1);
    }
  }
  if( __frame.data_len>__bytes_left(arc->fp) ) {
    return arc->fail();
  }
  pal_bytes = 3<<(__frame.depth_b&31);
  if( !(__frame.depth_b&0x80) )
    pal_bytes = __frame.depth_b&0x80;
  __frame.pal_len = pal_bytes;
  if( !__frame.hdr_words[0]||!__frame.hdr_words[1] ) {
    return arc->fail();
  }
  img_at = (BmfImage*)((uint8_t*)__alloc_image(__frame.hdr_words[0], __frame.hdr_words[1], __frame.depth_b&0x3F, (uint8_t)(__frame.depth_b&0x80)>>7, 1));
  __frame.depth_f = __frame.depth_b;
  img_at->depth = __frame.depth_b;
  has_coded = p_coded_buf&&*p_coded_buf;
  int8_t hdr_flags = __frame.flags_b;
  img_at->flags |= __frame.flags_b&2|(has_coded<<7);
  plane_count = ((__frame.depth_f&0x3Fu)+7)>>3;
  if( plane_count<1||plane_count>4 ) {
    return arc->fail();
  }
  if( !(hdr_flags&0x20) ) {
    if( __frame.data_len>img_at->data_size ) {
      return arc->fail();
    }
    uint32_t want = __frame.data_len;
    if( fread(img_at->pixels, 1u, __frame.data_len, arc->fp)!=want ) {
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
  coded_size = __frame.data_len;
  coded_buf = (uint8_t*)bmf_new(__frame.data_len);
  packer_reset();
  want2 = __frame.data_len;
  if( fread(coded_buf, 1u, __frame.data_len, arc->fp)!=want2 ) {
    return arc->fail();
  }
  bpp = img_at->depth;
  if( (bpp&0x3Fu)<=4||(__frame.flags_b&0x10)==0 ) {
    plane_predictor = 0;
    plane_alt_model = 0;
    __unmodel_plane((uint16_t*)img_at, img_at->pixels);
    goto check_length;
  }
  // The near-lossless nibble, mirroring `image_compress`: present for a single
  // greyscale plane and for three or more, absent otherwise.  Two `goto`s
  // jumped over it; the condition is the encoder's, negated.
  if( plane_count==1 ? (bpp&0x40)!=0 : plane_count>2 ) {
    near_lossless = unpack_bits(4);
    if( near_lossless ) {
      printf("\nnear-lossless stream (E=%d); this build only decodes E=0\n", near_lossless);
      exit(3);
    }
    near_lossless_q = 0;
  }
  if( plane_count>0 ) {
    __frame.mask = 255;
    __frame.p_i_2 = img_at;
    __frame.arc_f = arc;
    pl = 0;
    do {
      desc = unpack_bits(6);
      desc_flags = desc>>2;
      nrefs = desc&3;
      plane_desc[pl].flags = desc_flags;
      plane_desc[pl].nrefs = nrefs;
      plane_desc[nrefs].src_plane = pl;
      if( plane_desc[pl].flags&8 ) {
        dc_v = unpack_bits(8);
        plane_desc[pl].dc = dc_v;
        if( nrefs>1 ) {
          w4_v = unpack_bits(8);
          plane_desc[pl].weight0 = w4_v-64;
          w8_v = unpack_bits(8);
          plane_desc[pl].weight1 = w8_v-64;
          if( nrefs>2 ) {
            w12_v = unpack_bits(8);
            plane_desc[pl].weight2 = w12_v-64;
          }
        }
      }
      ++pl;
    } while( pl<plane_count );
    img_at = __frame.p_i_2;
    arc = __frame.arc_f;
  }
  plane_buf = (uint8_t*)bmf_new(img_at->width*img_at->height);
  if( __frame.flags_b&8 ) {
    __frame.img = *img_at;
    __frame.img.depth = 72;
    if( plane_count>0 ) {
      __frame.arc_f = arc;
      plane_i = 0;
      do {
        plane = plane_desc[plane_i].src_plane;
        plane_predictor = plane_desc[plane].flags&3;
        plane_alt_model = (uint8_t)(plane_desc[plane].flags&4)>>2;
        __unmodel_plane((uint16_t*)&__frame.img, plane_buf);
        if( plane_predictor ) {
          if( plane_predictor==1 ) {
            if( !plane_alt_model )
              __unpredict_med(plane_buf, img_at->width, img_at->height);
          }
        } else {
          __expand_predictor_mode0(img_at->width);
        }
        __interleave_plane((uint8_t*)img_at, plane_buf, plane);
        ++plane_i;
      } while( plane_i<plane_count );
restore_arc:
      arc = __frame.arc_f;
    }
  } else {
    plane_predictor = plane_desc[0].flags&3;
    plane_alt_model = (uint8_t)(plane_desc[0].flags&4)>>2;
    {
      __unmodel_plane((uint16_t*)img_at, img_at->pixels);
      if( plane_alt_model )
        goto free_plane_buf;
    }
    if( plane_count>0 ) {
      __frame.arc_f = arc;
      pl2 = 0;
      do {
        ++pl2;
        plane2 = plane_desc[pl2-1].src_plane;
        pred = plane_desc[plane2].flags&3;
        plane_predictor = pred;
        if( (plane_desc[plane2].flags&8)!=0||pred ) {
          i = img_at->width;
          __frame.row_step = &((uint8_t*)img_at)[plane2+16];
          n_pix = i*img_at->height;
          __frame.nplanes_s = plane_count;
          __frame.s12 = n_pix;
          if( plane_count==1 ) {
            memcpy(plane_buf, (uint8_t*)__frame.row_step, __frame.s12);
            pred_s = plane_predictor;
          } else {
            __frame.Block = &((uint8_t*)img_at)[plane2];
            __frame.s0 = pred;
            __frame.s4 = plane2;
            __frame.s8 = pl2;
            n_pix2 = __frame.s12;
            __frame.p_i_2 = img_at;
            pix_at = 0;
            nplanes_c = __frame.nplanes_s;
            i2 = 0;
            do {
              plane_buf[pix_at] = __frame.Block[i2+16];
              i2 += nplanes_c;
              ++pix_at;
            } while( pix_at<n_pix2 );
            pred_s = __frame.s0;
            plane2 = __frame.s4;
            pl2 = __frame.s8;
            img_at = __frame.p_i_2;
          }
          if( pred_s ) {
            if( pred_s==1 )
              __unpredict_med(plane_buf, img_at->width, img_at->height);
          } else {
            __expand_predictor_mode0(img_at->width);
          }
          __interleave_plane((uint8_t*)img_at, plane_buf, plane2);
        }
      } while( pl2<plane_count );
      goto restore_arc;
    }
  }
free_plane_buf:
  free(plane_buf);
check_length:
  if( coded_buf+__frame.data_len!=out_cursor ) {
    return arc->fail();
  }
  free(coded_buf);
  __frame.depth_f = __frame.depth_b;
read_palette:
  if( __frame.depth_f&0x80 ) {
    pal_at = (img_at->depth&0x80) ? &((uint8_t*)img_at)[img_at->data_size+16] : nullptr;
    got = fread(pal_at, 1u, __frame.pal_len, arc->fp);
    if( got!=__frame.pal_len ) {
      return arc->fail();
    }
  }
  if( img_at->flags&2 ) {
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
        __frame.p_i_2 = img_at;
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
        img_at = __frame.p_i_2;
        img_h = __frame.p_i_2->height;
        at += n_planes;
        ++y;
      } while( y<img_h );
    }
    w16 = img_at->width;
    img_at->width = img_h;
    img_at->flags ^= 2u;
    img_at->height = w16;
    *(uint16_t*)&img_at->stride = img_h*n_planes;
    free(copy);
  }
  return (uint8_t*)img_at;
}
