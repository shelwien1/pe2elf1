// alt_p2_decode.inc -- the decode half of alt_p2_encode.inc's template.
//
// The body is `alt_model_p2<1>`; what is left here is the name the caller in
// `plane.inc` reaches for.  See that file for what the two halves differed in,
// which of those differences survive as `f_DEC`, and what the merge is gated
// on.
int32_t alt_model_p2_decode(BmfImage* p_i, uint8_t* out) {
  return alt_model_p2<1>(p_i, out);
}
