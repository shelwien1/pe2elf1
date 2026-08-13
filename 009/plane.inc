// plane.inc -- one plane in and out of the model, whichever model the header names.

void __unmodel_plane(int8_t unread_flag, uint16_t* p_i, uint8_t* out) {
  ModelBlock* blk;
  void* raw;
  if( plane_alt_model ) {
    if( plane_predictor==1 ) {
      if( (p_i[5]&0x3F)==8 )
        __alt_model_p1_d8_decode(unread_flag, out, *p_i, p_i[1]);
      else
        __alt_model_p1_decode(p_i, out);
    } else if( plane_predictor==2 ) {
      if( (p_i[5]&0x3F)==8 )
        __alt_model_p2_d8_decode(out, *p_i, p_i[1]);
      else
        __alt_model_p2_decode(p_i, out);
    }
  } else {
    raw = bmf_new(sizeof(ModelBlock));
    if( raw )
      blk = __layout_workspace((ModelBlock*)raw, p_i[1], *p_i, p_i[1], p_i[5]&0x3F);
    else
      blk = (ModelBlock*)(nullptr);
    ((ModelBlock*)blk)->unmodel_plane_slow(out);
    if( blk )
      __free_workspace((ModelBlock*)blk, 1);
  }
}

void __model_planes(uint8_t* img, uint8_t* pixels, int32_t plane, int8_t unread_flag) {
  BmfImage hdr;
  uint8_t* aligned;
  uint8_t* scratch;
  int32_t ofs;
  plane_predictor = plane_desc[plane+1].flags&3;
  plane_alt_model = (uint8_t)(plane_desc[plane+1].flags&4)>>2;
  __colour_transform(img, pixels, plane, unread_flag);
  scratch = ::hist_scratch;
  aligned = (uint8_t*)((uintptr_t)(::hist_scratch+15)&~(uintptr_t)15);
  memset(scratch, 0, 15);
  bmf_zero16(&scratch[1008]);
  ofs = 1008;
  do {
    bmf_zero16((aligned+ofs-16));
    bmf_zero16((aligned+ofs-32));
    bmf_zero16((aligned+ofs-48));
    bmf_zero16((aligned+ofs-64));
    bmf_zero16((aligned+ofs-80));
    bmf_zero16((aligned+ofs-96));
    bmf_zero16((aligned+ofs-112));
    ofs -= 112;
  } while( ofs );
  {
    hdr = *(BmfImage*)img;
    hdr.depth = 72;
    if( plane_predictor==1&&!plane_alt_model )
      __predict_med(pixels, ((const BmfImage*)img)->width, ((const BmfImage*)img)->height);
    __model_plane(&hdr, pixels, pixels);
  }
}
