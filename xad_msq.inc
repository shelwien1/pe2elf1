/* xad_msq.inc -- part of xadpcm.cpp; also compiled standalone by ktest.cpp.
 *
 * The MS quantizer, in its reference and its division-free forms.  Kept apart
 * from xad_ms.inc so the differential test can include it without dragging in
 * the IDX-generated headers: everything here needs is i32/i64/u32/u64.
 */

/* THE REFERENCE.  Not called by the codec any more -- ms_quantize_r below
   replaces it -- but kept, because "bit-exact" has to mean bit-exact against
   something, and ktest.cpp brute-forces the replacement against this over the
   whole legal divisor domain. */
static INLINE int ms_quantize_ref(i64 diff, i32 delta, i64 &err) {
  i32 d = delta<1 ? 1 : delta;
  i64 q = (diff+(diff>=0 ? d/2 : -(d/2)))/d;
  q = q<-8 ? -8 : q>7 ? 7 : q;
  err = diff-q*i64(d);
  return int(q&15);
}

/* Reciprocal for the quantizer.  ((1<<32)-1)/d rather than (1<<32)/d: the
   domain starts at 1, not 16 -- ms_delta_update floors at 16 but the per-block
   header path clamps only at 1 -- and the plain form truncates to 0 at d = 1,
   which destroys the estimate.  The -1 form gives 0xFFFFFFFF there and stays
   inside the off-by-one bound over the whole domain. */
static INLINE u32 ms_rcp(i32 d) {
  return u32((u64(0xFFFFFFFFu))/u32(d<1 ? 1 : d));
}

/* Four of these run per sample-channel and they all share one divisor, which is
   a runtime value the compiler cannot strength-reduce.  With the reciprocal
   carried alongside iDelta (Ch::dl / Ch::rcp, refreshed only where delta
   changes) the inner loop has no division at all: six 64-bit idivs per
   sample-channel become one per nibble.

   The estimate is low by at most 1, so a single correction is exact -- proved
   by brute force over the whole domain in ktest.cpp, not by the bound argument
   alone. */
static INLINE int ms_quantize_r(i64 diff, i32 d, u32 rcp, i64 &err) {
  i64 t = diff+(diff>=0 ? d/2 : -(d/2));
  i64 a = t<0 ? -t : t, q;
  if( a>=8*i64(d) )
    q = t<0 ? -8 : 7;                       // the clamp region: no quotient needed
  else {
    q = i64((u64(a)*rcp)>>32);
    if( (q+1)*i64(d)<=a )
      q++;
    if( t<0 )
      q = -q;                               // C truncates toward zero
  }
  err = diff-q*i64(d);
  return int(q&15);
}

/* q2 and q3 only ever want the code: their `err` outputs were computed and
   never read.  Skipping the final multiply and subtract saves two of each per
   sample-channel. */
static INLINE int ms_quantize_nq(i64 diff, i32 d, u32 rcp) {
  i64 t = diff+(diff>=0 ? d/2 : -(d/2));
  i64 a = t<0 ? -t : t, q;
  if( a>=8*i64(d) )
    return int((t<0 ? -8 : 7)&15);
  q = i64((u64(a)*rcp)>>32);
  if( (q+1)*i64(d)<=a )
    q++;
  return int((t<0 ? -q : q)&15);
}
