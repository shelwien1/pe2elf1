// AVI: RIFF structure parser (encoder, and the decoder's header field model)
namespace avi {

struct MChunk { qword pos, size; uint id; long long movi; bool padok; int trk; qword idx; };   // chunk in a movi list
struct ListRec { qword pos, end; uint ty; int depth; };                                        // complete RIFF/LIST
struct StrlRec { qword strh, strhl, strf, strfl, indx, vprp, vprpl; };                         // payload positions
struct LeafRec { qword pos, size; };


struct Parser {
  const byte* d; qword fs;
  std::vector<ListRec> lists; std::vector<MChunk> mch; std::vector<qword> movis;   // movis: 'movi' fourcc positions (complete lists)
  std::vector<long long> movord;                    // per movi seen: ordinal among complete movi lists, or -1
  std::vector<StrlRec> strl; std::vector<LeafRec> idx1, ix;
  LeafRec avih, dmlh;                               // payload positions (0 = absent)
  Parser( const byte* _d, qword _fs ) : d(_d), fs(_fs) { avih.pos = avih.size = dmlh.pos = dmlh.size = 0; }

  void Children( qword p, qword e, int depth, long long movi, long long sl ) {
    while( p+8<=e ) {
      uint id = LE32(d+p); qword size = LE32(d+p+4);
      if( id==FCC_LE('R','I','F','F') || id==FCC_LE('L','I','S','T') ) {
        if( p+12>e ) break;
        uint ty = LE32(d+p+8); qword end = p+8+size;
        bool complete = size>=4 && end<=e;
        if( complete ) { ListRec l = { p, end, ty, depth }; lists.push_back( l ); }
        long long mv = movi, s2 = sl;
        if( ty==FCC_LE('m','o','v','i') ) {
          mv = complete ? (long long)movis.size() : -1;
          if( complete ) movis.push_back( p+8 );
        }
        if( ty==FCC_LE('s','t','r','l') ) { StrlRec r = { 0, 0, 0, 0, 0, 0, 0 }; s2 = (long long)strl.size(); strl.push_back( r ); }
        Children( p+12, complete ? end : e, depth+1, ty==FCC_LE('m','o','v','i') || ty==FCC_LE('r','e','c',' ') ? mv : -2, s2 );
        if( !complete ) return;
        p = end + (size&1);
        continue;
      }
      qword end = p+8+size;
      if( end>e ) break;
      bool padok = !(size&1) || end<e;
      if( movi!=-2 ) { MChunk c = { p, size, id, movi, padok, -1, 0 }; mch.push_back( c ); if( !padok ) break; }
      if( sl>=0 ) {
        StrlRec& r = strl[size_t(sl)];
        if( id==FCC_LE('s','t','r','h') && !r.strh ) r.strh = p+8, r.strhl = size;
        else if( id==FCC_LE('s','t','r','f') && !r.strf ) r.strf = p+8, r.strfl = size;
        else if( id==FCC_LE('i','n','d','x') && !r.indx ) r.indx = p;
        else if( id==FCC_LE('v','p','r','p') && !r.vprp ) r.vprp = p+8, r.vprpl = size;
      }
      if( id==FCC_LE('a','v','i','h') && !avih.pos ) avih.pos = p+8, avih.size = size;
      if( id==FCC_LE('d','m','l','h') && !dmlh.pos ) dmlh.pos = p+8, dmlh.size = size;
      if( id==FCC_LE('i','d','x','1') ) { LeafRec x = { p, size }; idx1.push_back( x ); }
      if( (id&0xFFFF)==('i'|('x'<<8)) ) { LeafRec x = { p, size }; ix.push_back( x ); }
      if( !padok ) break;
      p = end + (size&1);
    }
  }
  void File() { Children( 0, fs, 0, -2, -1 ); }
};

} // namespace avi
