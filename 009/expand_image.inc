// expand_image.inc -- the image out of a `.bmf`.

uint8_t*__expand_image(BmfArc* arc_in, void** p_coded_buf) {
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
    BmfArc* arc_f;
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
  BmfArc* arc;
  FILE* fp1, * fp;
  BmfImage* img_at;
  uint8_t bpp, has_coded, dc_v;
  uint32_t near_lossless, pred_s, w12_v, w4_v, w8_v;
  uint8_t* pal_at, * copy, * srcp, * dst;
  int32_t hdr_word, pl, nrefs, plane_i, plane, pl2, plane2, pred, i, n_pix2, pix_at, nplanes_c, i2, n_planes, last_row, at, y, pl_i, left;
  uint16_t img_h, w16;
  uint8_t major_v, minor_v;
  int32_t n_pix;
  uint32_t magic, pad_len, * blk, pal_bytes, want2, desc, desc_flags, want, got, word12, word8, word4, worddc, word6, word4b;
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
    plane_desc[0].weight0 = ((major_v<<8)-12288)|(minor_v-48);
    if( plane_desc[0].weight0!=512||fread(__frame.hdr, 8u, 1u, arc->fp)!=1 )
      break;
    fseek(arc->fp, (*(uint32_t*)&__frame.hdr[4]), 1);
    fp1 = arc->fp;
  }
  if( (uint16_t)magic!=0x8A81||(major_v = (uint8_t)(magic>>16), minor_v = (uint8_t)(magic>>24), plane_desc[0].weight0 = ((major_v<<8)-12288)|(minor_v-48), plane_desc[0].weight0!=512)||fread(__frame.hdr_words, 0x10u, 1u, arc->fp)!=1 ) {
    fp = arc->fp;
    fclose(fp);
    arc->fp = 0;
    return nullptr;
  }
  if( __frame.flags_b<0 ) {
    fread(__frame.hdr, 8u, 1u, arc->fp);
    if( p_coded_buf ) {
      hdr_word = *(int32_t*)&__frame.hdr[0];
      if( (*(uint32_t*)&__frame.hdr[4])>__bytes_left(arc->fp) ) {
        fclose(arc->fp);
        arc->fp = 0;
        return nullptr;
      }
      pad_len = ((*(uint32_t*)&__frame.hdr[4])+((*(uint32_t*)&__frame.hdr[4])==0)+3)&0xFFFFFFFC;
      blk = (uint32_t*)bmf_new(pad_len+8);
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
    fclose(arc->fp);
    arc->fp = 0;
    return nullptr;
  }
  pal_bytes = 3<<(__frame.depth_b&31);
  if( !(__frame.depth_b&0x80) )
    pal_bytes = __frame.depth_b&0x80;
  __frame.pal_len = pal_bytes;
  if( !__frame.hdr_words[0]||!__frame.hdr_words[1] ) {
    fclose(arc->fp);
    arc->fp = 0;
    return nullptr;
  }
  img_at = (BmfImage*)((uint8_t*)__alloc_image(__frame.hdr_words[0], __frame.hdr_words[1], __frame.depth_b&0x3F, (uint8_t)(__frame.depth_b&0x80)>>7, 1));
  __frame.depth_f = __frame.depth_b;
  img_at->depth = __frame.depth_b;
  has_coded = p_coded_buf&&*p_coded_buf;
  int8_t hdr_flags = __frame.flags_b;
  img_at->flags |= __frame.flags_b&2|(has_coded<<7);
  plane_count = ((__frame.depth_f&0x3Fu)+7)>>3;
  if( plane_count<1||plane_count>4 ) {
    fclose(arc->fp);
    arc->fp = 0;
    return nullptr;
  }
  if( !(hdr_flags&0x20) ) {
    if( __frame.data_len>img_at->data_size ) {
      fclose(arc->fp);
      arc->fp = 0;
      return nullptr;
    }
    want = __frame.data_len;
    if( fread(img_at->pixels, 1u, __frame.data_len, arc->fp)!=want ) {
      fclose(arc->fp);
      arc->fp = 0;
      return nullptr;
    }
    goto LABEL_109;
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
  out_cursor = coded_buf;
  packer_free_bits = 0;
  packer_acc = 0;
  packer_word = (uint32_t*)coded_buf;
  hist_scratch = coded_buf+coded_size-4096;
  want2 = __frame.data_len;
  if( fread(coded_buf, 1u, __frame.data_len, arc->fp)!=want2 ) {
    fclose(arc->fp);
    arc->fp = 0;
    return nullptr;
  }
  bpp = img_at->depth;
  if( (bpp&0x3Fu)<=4||(__frame.flags_b&0x10)==0 ) {
    plane_predictor = 0;
    plane_alt_model = 0;
    __unmodel_plane(want2, (uint16_t*)img_at, img_at->pixels);
    goto LABEL_106;
  }
  if( plane_count==1 ) {
    if( !(bpp&0x40) )
      goto LABEL_42;
  } else if( plane_count<=2 ) {
    goto LABEL_42;
  }
  packer_free_bits -= 4;
  if( packer_free_bits<0 ) {
    word4b = __packer_word();
    want2 = word4b>>(-packer_free_bits&31);
    near_lossless = packer_acc|(word4b<<((packer_free_bits+4)&31))&0xF;
    packer_acc = want2;
    packer_free_bits += 32;
  } else {
    near_lossless = packer_acc&0xF;
    packer_acc = packer_acc>>4;
  }
  if( near_lossless ) {
    printf("\nnear-lossless stream (E=%d); this build only decodes E=0\n", near_lossless);
    exit(3);
  }
  near_lossless_q = 0;
LABEL_42:
  if( plane_count>0 ) {
    __frame.mask = 255;
    __frame.p_i_2 = img_at;
    __frame.arc_f = arc;
    pl = 0;
    do {
      packer_free_bits -= 6;
      if( packer_free_bits<0 ) {
        word6 = __packer_word();
        desc = packer_acc|(word6<<((packer_free_bits+6)&31))&0x3F;
        packer_acc = word6>>(-packer_free_bits&31);
        packer_free_bits += 32;
      } else {
        desc = packer_acc&0x3F;
        packer_acc = packer_acc>>6;
      }
      desc_flags = desc>>2;
      nrefs = desc&3;
      plane_desc[pl+1].flags = desc_flags;
      plane_desc[pl+1].nrefs = nrefs;
      plane_desc[nrefs+1].src_plane = pl;
      if( plane_desc[pl+1].flags&8 ) {
        packer_free_bits -= 8;
        if( packer_free_bits<0 ) {
          worddc = __packer_word();
          dc_v = packer_acc|__frame.mask&(worddc<<((packer_free_bits+8)&31));
          packer_acc = worddc>>(-packer_free_bits&31);
          packer_free_bits += 32;
        } else {
          dc_v = packer_acc&(uint8_t)__frame.mask;
          packer_acc = packer_acc>>8;
        }
        plane_desc[pl+1].dc = dc_v;
        if( nrefs>1 ) {
          packer_free_bits -= 8;
          if( packer_free_bits<0 ) {
            word4 = __packer_word();
            w4_v = packer_acc|__frame.mask&(word4<<((packer_free_bits+8)&31));
            packer_acc = word4>>(-packer_free_bits&31);
            packer_free_bits += 32;
          } else {
            w4_v = packer_acc&__frame.mask;
            packer_acc = packer_acc>>8;
          }
          plane_desc[pl+1].weight0 = w4_v-64;
          packer_free_bits -= 8;
          if( packer_free_bits<0 ) {
            word8 = __packer_word();
            w8_v = packer_acc|__frame.mask&(word8<<((packer_free_bits+8)&31));
            packer_acc = word8>>(-packer_free_bits&31);
            packer_free_bits += 32;
          } else {
            w8_v = packer_acc&__frame.mask;
            packer_acc = packer_acc>>8;
          }
          plane_desc[pl+1].weight1 = w8_v-64;
          if( nrefs>2 ) {
            packer_free_bits -= 8;
            if( packer_free_bits<0 ) {
              word12 = __packer_word();
              w12_v = packer_acc|__frame.mask&(word12<<((packer_free_bits+8)&31));
              packer_acc = word12>>(-packer_free_bits&31);
              packer_free_bits += 32;
            } else {
              w12_v = packer_acc&__frame.mask;
              packer_acc = packer_acc>>8;
            }
            plane_desc[pl+1].weight2 = w12_v-64;
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
        plane = plane_desc[plane_i+1].src_plane;
        plane_predictor = plane_desc[plane+1].flags&3;
        plane_alt_model = (uint8_t)(plane_desc[plane+1].flags&4)>>2;
        __unmodel_plane(plane_i, (uint16_t*)&__frame.img, plane_buf);
        if( plane_predictor ) {
          if( plane_predictor==1 ) {
            if( !plane_alt_model )
              __unpredict_med(plane_buf, img_at->width, img_at->height);
          }
        } else {
          __expand_predictor_mode0(plane_buf, img_at->width, img_at->height);
        }
        __interleave_plane((uint8_t*)img_at, plane_buf, plane, 0);
        ++plane_i;
      } while( plane_i<plane_count );
LABEL_104:
      arc = __frame.arc_f;
    }
  } else {
    plane_predictor = plane_desc[1].flags&3;
    plane_alt_model = (uint8_t)(plane_desc[1].flags&4)>>2;
    {
      __unmodel_plane(63, (uint16_t*)img_at, img_at->pixels);
      if( plane_alt_model )
        goto LABEL_105;
    }
    if( plane_count>0 ) {
      __frame.arc_f = arc;
      pl2 = 0;
      do {
        ++pl2;
        plane2 = plane_desc[pl2].src_plane;
        pred = plane_desc[plane2+1].flags&3;
        plane_predictor = pred;
        if( (plane_desc[plane2+1].flags&8)!=0||pred ) {
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
            __expand_predictor_mode0(plane_buf, img_at->width, img_at->height);
          }
          __interleave_plane((uint8_t*)img_at, plane_buf, plane2, 0);
        }
      } while( pl2<plane_count );
      goto LABEL_104;
    }
  }
LABEL_105:
  free(plane_buf);
LABEL_106:
  if( coded_buf+__frame.data_len!=out_cursor ) {
    fclose(arc->fp);
    arc->fp = 0;
    return nullptr;
  }
  free(coded_buf);
  __frame.depth_f = __frame.depth_b;
LABEL_109:
  if( __frame.depth_f&0x80 ) {
    pal_at = (img_at->depth&0x80) ? &((uint8_t*)img_at)[img_at->data_size+16] : nullptr;
    got = fread(pal_at, 1u, __frame.pal_len, arc->fp);
    if( got!=__frame.pal_len ) {
      fclose(arc->fp);
      arc->fp = 0;
      return nullptr;
    }
  }
  if( img_at->flags&2 ) {
    if( (uint64_t)img_at->width*img_at->height*(uint32_t)plane_count>(uint64_t)img_at->data_size ) {
      fclose(arc->fp);
      arc->fp = 0;
      return nullptr;
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
