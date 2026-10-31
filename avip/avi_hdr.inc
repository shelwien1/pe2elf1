// AVI: header field model (op 0xFB)
//
// avih, strh, the video strf BITMAPINFOHEADER, vprp and dmlh are records of 32-bit
// fields; many fields repeat other header values or chunk statistics. Op 0xFB r c..:
// record type r at the current position, then 2 bits per predictable field (LSB first,
// fields without predictors are skipped): 0 = field in struct, k = the field's k-th
// predictor. The non-predicted fields follow in struct, in record order.
//
// Predictor inputs ("context"): the header bytes before the first complete movi list
// (as restored; predicted fields don't feed other predictions), parsed like the input,
// and per stream number the chunk count, the count before the second movi list, the
// largest chunk and the total chunk bytes.

namespace avi {

enum { HR_AVIH=1, HR_STRH=2, HR_BIH=3, HR_VPRP=4, HR_DMLH=5, HR_N };
enum { PR_NONE=0, PR_ZERO, PR_ONE, PR_FF, PR_1M, PR_C910, PR_C810, PR_C40, PR_USPF, PR_USPF_FLOOR, PR_TOTFIRST,
       PR_TOTALL, PR_NSTRL, PR_MAXALL, PR_VW, PR_VH, PR_TAG, PR_LEN, PR_MAXS, PR_RECT, PR_SIZEIMG, PR_STRFLEN,
       PR_REFRESH };

static const int hr_nf[HR_N] = { 0, 14, 14, 10, 17, 1 };      // fields covered per record type
// up to 3 predictors per field, by record type
static const byte hr_pred[HR_N][17][3] = {
  {},
  { {PR_USPF,PR_USPF_FLOOR}, {PR_ZERO}, {PR_ZERO}, {PR_C910,PR_C810,PR_ZERO}, {PR_TOTFIRST,PR_TOTALL}, {PR_ZERO},
    {PR_NSTRL}, {PR_1M,PR_MAXALL,PR_ZERO}, {PR_VW}, {PR_VH}, {PR_ZERO}, {PR_ZERO}, {PR_ZERO}, {PR_ZERO} },
  { {}, {PR_TAG,PR_ZERO,PR_ONE}, {PR_ZERO}, {PR_ZERO}, {PR_ZERO}, {}, {}, {PR_ZERO}, {PR_LEN}, {PR_MAXS,PR_ZERO,PR_1M},
    {PR_FF,PR_ZERO}, {}, {PR_ZERO}, {PR_RECT,PR_ZERO} },
  { {PR_C40,PR_STRFLEN}, {}, {}, {}, {}, {PR_SIZEIMG,PR_ZERO}, {PR_ZERO}, {PR_ZERO}, {PR_ZERO}, {PR_ZERO} },
  { {PR_ZERO}, {PR_ZERO}, {PR_REFRESH}, {PR_VW}, {PR_VH}, {}, {PR_VW}, {PR_VH}, {PR_ONE}, {PR_VH}, {PR_VW}, {PR_VH},
    {PR_VW}, {PR_ZERO}, {PR_ZERO}, {PR_ZERO}, {PR_ZERO} },
  { {PR_TOTALL} } };

struct HdrCtx {
  const byte* h; Parser* P;                       // header bytes and their parse
  int vs;                                         // video stream (first 'vids' strl with a BITMAPINFOHEADER), or -1
  std::vector<char> has; std::vector<qword> nall, nfirst, maxsz, bytes;   // per stream number: chunk statistics
};

// record size in bytes for a payload of n bytes, or 0 if too short
static qword HdrRecLen( int ty, qword n ) { qword z = 4*qword(hr_nf[ty]); return n>=z ? z : 0; }

static bool HdrPredict( const HdrCtx& c, int s, int pr, uint& v ) {
  const Parser& P = *c.P;
  auto vid = [&]( int i, uint& w, uint& hgt, uint& bc ) -> bool {
    if( i<0 || size_t(i)>=P.strl.size() || P.strl[size_t(i)].strfl<40 ) return false;
    const byte* f = c.h+P.strl[size_t(i)].strf;
    w = LE32(f+4); int32_t hh = int32_t(LE32(f+8)); hgt = uint( hh<0 ? -int64_t(hh) : hh ); bc = LE16(f+14);
    return true;
  };
  auto strh = [&]( int i ) -> const byte* {
    if( i<0 || size_t(i)>=P.strl.size() || P.strl[size_t(i)].strhl<48 ) return 0;
    return c.h+P.strl[size_t(i)].strh;
  };
  bool hs = s>=0 && size_t(s)<c.has.size() && c.has[size_t(s)];
  uint w, hgt, bc;
  switch( pr ) {
  case PR_ZERO: v = 0; return true;
  case PR_ONE: v = 1; return true;
  case PR_FF: v = 0xFFFFFFFFu; return true;
  case PR_1M: v = 0x100000; return true;
  case PR_C910: v = 0x910; return true;
  case PR_C810: v = 0x810; return true;
  case PR_C40: v = 40; return true;
  case PR_NSTRL: v = uint(P.strl.size()); return true;
  case PR_VW: if( !vid( c.vs, w, hgt, bc ) ) return false; v = w; return true;
  case PR_VH: if( !vid( c.vs, w, hgt, bc ) ) return false; v = hgt; return true;
  case PR_USPF: case PR_USPF_FLOOR: case PR_REFRESH: {
    const byte* h = strh( c.vs ); if( !h ) return false;
    qword sc = LE32(h+20), rt = LE32(h+24);
    if( pr==PR_REFRESH ) { if( !sc ) return false; v = uint( (rt+sc/2)/sc ); return true; }
    if( !rt ) return false;
    qword u = pr==PR_USPF ? (1000000*sc+rt/2)/rt : 1000000*sc/rt;
    if( u>0xFFFFFFFFu ) return false;
    v = uint(u); return true;
  }
  case PR_TOTFIRST: case PR_TOTALL:
    if( c.vs<0 || size_t(c.vs)>=c.has.size() || !c.has[size_t(c.vs)] ) return false;
    v = uint( pr==PR_TOTFIRST ? c.nfirst[size_t(c.vs)] : c.nall[size_t(c.vs)] ); return true;
  case PR_MAXALL: { qword m = 0; bool any = false; for( size_t i=0; i<c.has.size(); i++ ) if( c.has[i] ) m = std::max( m, c.maxsz[i] ), any = true; if( !any ) return false; v = uint(m); return true; }
  case PR_MAXS: if( !hs ) return false; v = uint(c.maxsz[size_t(s)]); return true;
  case PR_TAG: {
    const byte* h = strh( s ); if( !h || P.strl[size_t(s)].strfl<4 ) return false;
    const byte* f = c.h+P.strl[size_t(s)].strf;
    if( LE32(h)==FCC_LE('v','i','d','s') ) { if( P.strl[size_t(s)].strfl<40 ) return false; v = LE32(f+16); return true; }
    if( LE32(h)==FCC_LE('a','u','d','s') ) { v = LE16(f); return true; }
    return false;
  }
  case PR_LEN: {
    const byte* h = strh( s ); if( !h || !hs ) return false;
    qword ss = LE32(h+44);
    v = uint( LE32(h)!=FCC_LE('v','i','d','s') && ss ? c.bytes[size_t(s)]/ss : c.nall[size_t(s)] ); return true;
  }
  case PR_RECT: { const byte* h = strh( s ); if( !h || LE32(h)!=FCC_LE('v','i','d','s') || !vid( s, w, hgt, bc ) || w>0xFFFF || hgt>0xFFFF ) return false; v = w | hgt<<16; return true; }
  case PR_SIZEIMG: { if( !vid( s, w, hgt, bc ) ) return false; qword z = qword(w)*hgt*bc/8; if( z>0xFFFFFFFFu ) return false; v = uint(z); return true; }
  case PR_STRFLEN: if( s<0 || size_t(s)>=P.strl.size() ) return false; v = uint(P.strl[size_t(s)].strfl); return true;
  }
  return false;
}

// stream of a record at payload position pos (strh/strf/vprp), -1 for file-level records
static int HdrStream( const Parser& P, int ty, qword pos ) {
  for( size_t i=0; i<P.strl.size(); i++ ) {
    const StrlRec& r = P.strl[i];
    if( (ty==HR_STRH && r.strh==pos) || (ty==HR_BIH && r.strf==pos) || (ty==HR_VPRP && r.vprp==pos) ) return int(i);
  }
  return -1;
}

static void HdrVideo( HdrCtx& c ) {
  c.vs = -1;
  for( size_t i=0; i<c.P->strl.size() && c.vs<0; i++ ) {
    const StrlRec& r = c.P->strl[i];
    if( r.strhl>=48 && LE32(c.h+r.strh)==FCC_LE('v','i','d','s') && r.strfl>=40 ) c.vs = int(i);
  }
}

// number of 2-bit codes in a record's code bytes
static int HdrNCodes( int ty ) { int n = 0; for( int f=0; f<hr_nf[ty]; f++ ) n += hr_pred[ty][f][0]!=PR_NONE; return n; }

} // namespace avi
