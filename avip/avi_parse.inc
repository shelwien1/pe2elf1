// AVI: RIFF parser, encoder track setup
namespace avi {

//--- encoder -----------------------------------------------------------------

struct Track {
  int idx; bool ok; uint ckid, ckid2;                  // ckid2: chunks of this id are frames too (0: none)
  uint fcc, handler; qword scale, rate, start, ssize;   // strh
  const byte* strf; qword strfl;
  std::string ext, xext; int ckind; uint ckparam; int cptype, cpparam, xform; Buf xhdr; byte adts[7];
  std::vector<qword> foff; std::vector<uint> fsz; std::vector<int> fpad; std::vector<byte> cls, alt;   // alt: id ckid2
  std::vector<qword> cidx;                              // movi chunk index per frame
  TrackHead h;                                          // head written to the meta
  int padv; std::vector<qword> pexc, pval;
  bool grp; long long bias;
  bool himap; qword imap[8]; std::vector<qword> iexc, ival;
  bool hxmap; int xmap; std::vector<qword> xexc;
  int amap; std::vector<qword> aexc;
  qword bytes;
  bool Has( uint id ) const { return id==ckid || (ckid2 && id==ckid2); }   // chunks of id are frames
};







// stream type -> codec kind, output extension, unit parser, transform
static void SetupTrack( Track& t, const byte* d ) {
  CodecSetup c;
  size_t k = 0; while( k<t.fsz.size() && !t.fsz[k] ) k++;
  const byte* first = k<t.fsz.size() ? d+t.foff[k] : 0; qword firstn = k<t.fsz.size() ? t.fsz[k] : 0;
  if( t.fcc==FCC_LE('v','i','d','s') && t.strfl>=40 ) VfwSetup( t.strf, t.strfl, first, firstn, c );
  else if( t.fcc==FCC_LE('a','u','d','s') && t.strfl>=14 ) AcmSetup( t.strf, t.strfl, t.bytes, c );
  t.ckind = c.ck; t.ckparam = c.ckp; t.cptype = c.pt; t.cpparam = c.pp; t.xform = c.xform; t.xhdr.swap( c.xhdr );
  memcpy( t.adts, c.adts, 7 ); t.ext = c.ext; t.xext = c.xext;
}

// chunk sizes: explicit column, or derived from the data (checked in stream context)
static void ChooseSizeMode( const byte* d, Track& t, const Buf& raw ) {
  t.h.szmode = SZ_EXPLICIT; t.h.ptype = t.h.pparam = 0; t.h.esc.clear();
  t.h.col.assign( t.fsz.begin(), t.fsz.end() );
  if( t.xform==XF_ADTS ) { t.h.szmode = SZ_XFORM; t.h.col.clear(); return; }
  ChooseUnits( t.cptype, t.cpparam, d, t.foff, t.fsz, raw, t.h );
}

static void ToADTS( Track& t, const Buf& s, Buf& c ) {
  ::ToADTS( s, [&]( qword j ) { return t.fsz[size_t(j)]; }, t.fsz.size(), t.adts, c, t.h.xa );
}

enum { K_LIST, K_LEND, K_CHUNK, K_IDX1, K_IX, K_INDX, K_FLD };

struct Item { qword pos, end; int kind, trk, depth; qword a, b; long long c; };   // a/b/c: per kind

} // namespace avi
