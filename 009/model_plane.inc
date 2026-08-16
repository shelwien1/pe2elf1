// model_plane.inc -- one plane through the main model, encoding.
//
// The body is `ModelBlock::model_plane_slow` and the wrapper is
// `code_plane<0>`; both live beside their decode halves now.  What was here
// was 130 lines of `blk->` where the decoder had a call, which is why
// `pairshare.py` read this pair at 14 shared lines of 170: it was comparing a
// body against a call.

void model_plane(BmfImage* p_i, uint8_t* pixels, uint8_t* raw) {
  code_plane<0>(p_i, pixels, raw);
}
