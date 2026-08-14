// plane.inc -- one plane in and out of the model, whichever model the header names.

void __unmodel_plane(uint16_t* p_i, uint8_t* out) {
  ModelBlock* blk;
  if( plane_alt_model ) {
    if( plane_predictor==1 ) {
      if( (p_i[5]&0x3F)==8 )
        __alt_model_p1_d8_decode(out, *p_i, p_i[1]);
      else
        __alt_model_p1_decode(p_i, out);
    } else if( plane_predictor==2 ) {
      if( (p_i[5]&0x3F)==8 )
        __alt_model_p2_d8_decode(out, *p_i, p_i[1]);
      else
        __alt_model_p2_decode(p_i, out);
    }
  } else {
    void* raw = bmf_new(sizeof(ModelBlock));
    if( raw )
      blk = ((ModelBlock*)raw)->layout_workspace(*p_i, p_i[1], p_i[5]&0x3F);
    else
      blk = (ModelBlock*)(nullptr);
    (blk)->unmodel_plane_slow(out);
    if( blk )
      blk->free_workspace(1);
  }
}

void __model_planes(uint8_t* img, uint8_t* pixels, int32_t plane) {
  plane_predictor = plane_desc[plane].flags&3;
  plane_alt_model = (uint8_t)(plane_desc[plane].flags&4)>>2;
  __colour_transform(img, pixels, plane);
  // 1,024 bytes.  The original zeroed them as a `memset` of the unaligned head,
  // one aligned 16-byte store at the tail, and a loop of seven more stepping
  // back by 112 -- SSE, unrolled sevenfold, over a buffer whose start it had to
  // round up to 16 first.  The three ranges cover [0, 1023] whatever the
  // rounding takes: the head covers everything below the aligned start (at most
  // 15 bytes), the loop covers 1,008 from there, and the tail store covers what
  // the loop's own offset pushed past the end.  This runs once per plane.
  memset(hist_scratch, 0, 1024);
  {
    BmfImage hdr = *(BmfImage*)img;
    hdr.depth = 72;
    if( plane_predictor==1&&!plane_alt_model )
      __predict_med(pixels, ((const BmfImage*)img)->width, ((const BmfImage*)img)->height);
    __model_plane(&hdr, pixels, pixels);
  }
}
