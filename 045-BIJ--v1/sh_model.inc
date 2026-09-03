
// Model-side helper lifted out of sh_v1m.inc.  The rangecoder that used to
// live in that file is replaced by sh_bitrc.inc (sh_bit, bijective); this
// probability update is CDM model code and is unchanged.
uint LinearUpdate( uint P, const int c, const int wr, const int Mw ) {
//  const int mw = c ? Mw : SCALE-Mw;
//  p = p*(SCALE-wr) + mw*wr;
  int dp = P + Mw-SCALE + ((SCALE-2*Mw)&(-c));
  dp = ((dp*wr)>>SCALElog);
  int q = P - dp + c;
  P = (q<mSCALE ) ? q : mSCALE;
  return P;
}

