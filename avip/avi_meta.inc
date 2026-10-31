// AVI container (incl. OpenDML)
//
// Track streams hold the data of each stream's chunks in file order. Conversions:
// MPEG-4 ASP -> .m4v, H.264 -> .h264, HEVC -> .hevc, MJPEG -> .mjpeg, MPEG-1/2 -> .m2v
// (codec headers from strf prepended when the stream lacks them); MPEG audio, AC-3,
// DTS, ADTS are elementary streams already; raw AAC -> ADTS; PCM/ADPCM/G.711/... ->
// .wav (WAV header from strf prepended). Regenerated on restore:
//   - chunk headers (id, size) and pad bytes of stream chunks; a stream with chunks of
//     two ids ('dc' and 'db': some files have the key frames as '00db'): the id from a
//     map over the frame class + exceptions
//   - chunk sizes            : from self-delimiting data (H.264 access units, MPEG-4
//                              VOPs, JPEG images, MPEG-1/2 pictures, MPEG audio, AC-3,
//                              ADTS, raw AAC syntax) or from the ADTS transform;
//                              otherwise an explicit size column
//   - chunk order            : predicted from the strh timing of each stream
//   - RIFF/LIST sizes        : from their contents
//   - idx1 entries           : from chunk positions/sizes; flags from a per-stream
//                              map over the frame class (key/header/empty) + exceptions
//   - OpenDML ix## entries, indx super index entries: from chunk/index positions
//   - runs of zero bytes, short repeating patterns (JUNK, reserved index space)
// Truncated files: chunks cut by EOF and unterminated lists stay in the struct. The
// encoder runs the decoder in memory and falls back to raw track streams, then to
// plain chunks, then to a plain copy.

// AVI: meta track part, ops, decoder structures and meta parser
namespace avi {

//--- AVI track part and ops (after the common head, see meta.inc) ------------------
//
//   u8   ck: bits 0..2 chunk id type (1 'dc', 2 'db', 3 'wb', 4 'pc', 5 'tx'; 0: 4-byte id
//        follows), bit 3: u8 stream number follows (else = track number), bits 4..7 ckind:
//        frame class parser (0 raw, 1 H.264, 2 MPEG-4 part 2, 3 MJPEG, 4 MPEG-1/2, 5 audio,
//        6 HEVC, 7 MS-MPEG4 v2/v3 / WMV1, 8 WMV2, 9 VC-1: u8 sequence flags follow:
//        1 FINTERPFLAG, 2 RANGERED, 4 MAXBFRAMES>0, 8 advanced profile, 16 INTERLACE)
//   u8   flags: 1 pad bytes: u8 value of odd-sized chunks, index column (see column.inc), n x u8 values (absent: value 0)
//               2 timing (op 0xF5 group): vint scale, vint rate, vint samplesize, vint start,
//               vint zigzag(bias)
//               4 idx1 flags map; 8: 8 x vint flags per frame class (key | header<<1 |
//               empty<<2) follow, else the default (0x10 for key classes, 0 otherwise);
//               exception index column, column of the exception values
//               16 ix## map: u8 non-key bit per frame class, exception index column (bit flipped)
//               32 second chunk id (chunks of either id are the track's frames): u8 its type
//               (as in ck, 1..5; the stream number is the first id's), u8 bit per frame class:
//               the class's chunks have the second id, exception index column (id flipped)
//
// ops: 0x00..0x3F  chunk of track #code: id, size, data, pad byte regenerated
//      0xF0 n      copy n bytes from struct
//      0xF1 t      chunk of track t
//      0xF2 n      n zero bytes
//      0xF9 p n    n bytes repeating the output p bytes back (p <= 64)
//      0xFA        LIST 'rec ' (as 0xF3, id and type implied)
//      0xF3        RIFF/LIST: struct holds id and type; the size is the content length
//      0xF4        end of the innermost open RIFF/LIST
//      0xF5 n      n chunks, each the next chunk of the timing-group track with the
//                  smallest key: floor(units*scale*2^16/rate)+bias, units = chunk index
//                  (samplesize 0) or bytes before the chunk / samplesize, plus start;
//                  ties: lower track
//      0xF6 k [f]  idx1 entries (after the idx1 header in struct; count = size/16) for
//                  the stream chunks of the last movi list: id, flags, offset (from the
//                  'movi' fourcc if k&1=0, absolute if k&1=1), size; k&2: 'rec ' lists of
//                  that movi list are indexed too, with flags vint f
//      0xF7 t k d  ix## entries (struct: 20 header bytes before, then qwBaseOffset as
//                  residual vs the reference and dwReserved): chunks of track t from
//                  (next unindexed + zigzag d); reference: k=0 'movi' fourcc of the first
//                  chunk's movi list, k=1 the first chunk's data
//      0xF8 t      indx entries in use (struct: 32 header bytes before, then a 4-byte
//                  duration residual per entry): offset and size of the track's ix##
//                  chunks in order; duration = entries, or bytes/samplesize
//      0xFB r c..  header record with predicted fields (see avi_hdr.inc)
//      0xFF        copy rest of struct, end
// Restore runs the program twice: sizes and positions first, then output.

enum { OP_RAW=0xF0, OP_CHUNK=0xF1, OP_ZERO=0xF2, OP_LIST=0xF3, OP_LEND=0xF4, OP_PRUN=0xF5,
       OP_IDX1=0xF6, OP_IX=0xF7, OP_INDX=0xF8, OP_REP=0xF9, OP_RLIST=0xFA, OP_FLD=0xFB, OP_END=0xFF };
static const char* const ck_types[] = { "", "dc", "db", "wb", "pc", "tx" };

// type of a chunk id: its last two characters as an index into ck_types (0: none of them)
static uint CkType( uint id ) {
  for( uint q=1; q<=5; q++ ) if( ((id>>16)&255)==byte(ck_types[q][0]) && (id>>24)==byte(ck_types[q][1]) ) return q;
  return 0;
}
// id of type ty (1..5) with the stream number (first two characters) of id
static uint CkWithType( uint id, uint ty ) { return (id&0xFFFF) | uint(byte(ck_types[ty][0]))<<16 | uint(byte(ck_types[ty][1]))<<24; }

//--- decoder -----------------------------------------------------------------

struct IxRec { qword pos, total, n, bytes; };

struct DTrack {
  TrackHead h;
  uint ckid; int ckind; uint ckparam; qword nf;
  int padv; std::vector<qword> pexc, pval;
  bool grp; qword scale, rate, ssize, start; long long bias;
  bool himap; qword imap[8]; std::vector<qword> iexc, ival;
  bool hxmap; int xmap; std::vector<qword> xexc;
  bool hamap; uint ckid2; int amap; std::vector<qword> aexc;
  // derived
  std::vector<uint> sz, xsz; std::vector<qword> fo; std::vector<byte> cls, alt;   // alt: chunk id ckid2
  std::vector<qword> cpos; std::vector<long long> cmovi; std::vector<IxRec> ix;
};

struct DOp { int code; qword t, n, k; };

struct Meta {
  const byte* st; qword stlen;
  std::vector<DOp> ops;
  std::vector<DTrack> trk;
};

static bool ParseMeta( const Buf& m, Meta& M ) {
  if( MetaFormat( m.data(), m.size() )!=FMT_AVI ) return false;
  Reader r( m.data()+1, m.size()-1 );
  qword nt = r.vint(); if( r.err || nt>100 ) return false;
  M.trk.resize( size_t(nt) );
  for( size_t i=0; i<M.trk.size(); i++ ) {
    DTrack& t = M.trk[i];
    if( !GetHead( r, t.h ) ) return false;
    if( t.h.xform!=XF_NONE && t.h.xform!=XF_ADTS && t.h.xform!=XF_HDR ) return false;
    if( t.h.szmode==SZ_TABLES ) return false;
    t.nf = t.h.nf;
    uint ck = r.u8(), ty = ck&7, sn = i;
    t.ckind = int(ck>>4); t.ckparam = 0;
    if( ty>5 || t.ckind>CK_VC1 ) return false;
    if( !ty ) { const byte* p = r.bytes(4); if( !p ) return false; t.ckid = LE32(p); }
    else {
      if( ck&8 ) sn = r.u8();
      if( sn>99 ) return false;
      t.ckid = CkWithType( uint('0'+sn/10) | uint('0'+sn%10)<<8, ty );
    }
    if( t.ckind==CK_VC1 ) t.ckparam = r.u8();
    uint fl = r.u8();
    if( r.err || fl>63 ) return false;
    t.padv = 0; t.pexc.clear(); t.pval.clear();
    if( fl&1 ) {
      t.padv = int(r.u8());
      if( !GetIdxCol( r, t.pexc, t.nf ) ) return false;
      t.pval.resize( t.pexc.size() );
      for( size_t k=0; k<t.pval.size(); k++ ) t.pval[k] = r.u8();
    }
    t.grp = (fl&2)!=0; t.scale = t.rate = t.ssize = t.start = 0; t.bias = 0;
    if( t.grp ) t.scale = r.vint(), t.rate = r.vint(), t.ssize = r.vint(), t.start = r.vint(), t.bias = UnZigZag( r.vint() );
    t.himap = (fl&4)!=0;
    for( int c=0; c<8; c++ ) t.imap[c] = c&FC_KEY ? 0x10 : 0;
    if( (fl&8) && !t.himap ) return false;
    if( t.himap ) {
      if( fl&8 ) for( int c=0; c<8; c++ ) { t.imap[c] = r.vint(); if( t.imap[c]>0xFFFFFFFFu ) return false; }
      if( !GetIdxCol( r, t.iexc, t.nf ) || !GetCol( r, t.ival, t.iexc.size() ) ) return false;
      for( size_t k=0; k<t.ival.size(); k++ ) if( t.ival[k]>0xFFFFFFFFu ) return false;
    }
    t.hxmap = (fl&16)!=0; t.xmap = 0;
    if( t.hxmap ) { t.xmap = r.u8(); if( !GetIdxCol( r, t.xexc, t.nf ) ) return false; }
    t.hamap = (fl&32)!=0; t.ckid2 = t.ckid; t.amap = 0;
    if( t.hamap ) {
      uint ty2 = r.u8(); if( ty2<1 || ty2>5 ) return false;
      t.ckid2 = CkWithType( t.ckid, ty2 ); t.amap = r.u8();
      if( !GetIdxCol( r, t.aexc, t.nf ) ) return false;
    }
    if( r.err ) return false;
  }
  M.ops.clear();
  for(;;) {
    DOp d; d.code = r.u8(); d.t = d.n = d.k = 0;
    if( r.err ) return false;
    if( d.code<0x40 ) d.t = qword(d.code), d.code = OP_CHUNK;
    else if( d.code==OP_CHUNK || d.code==OP_INDX ) d.t = r.vint();
    else if( d.code==OP_RAW || d.code==OP_ZERO || d.code==OP_PRUN ) d.n = r.vint();
    else if( d.code==OP_IDX1 ) { d.k = r.u8(); if( d.k&2 ) d.n = r.vint(); }
    else if( d.code==OP_IX ) d.t = r.vint(), d.k = r.u8(), d.n = r.vint();
    else if( d.code==OP_REP ) { d.k = r.vint(); d.n = r.vint(); if( d.k<1 || d.k>64 ) return false; }
    else if( d.code==OP_FLD ) {                    // record type, 2-bit codes
      d.k = r.u8(); if( d.k<1 || d.k>=HR_N ) return false;
      int nc = HdrNCodes( int(d.k) );
      for( int b=0; b<(2*nc+7)/8; b++ ) d.n |= qword(r.u8())<<(8*b);
      if( !d.n ) return false;
      for( int f=0, ci=0; f<hr_nf[d.k]; f++ ) {
        if( hr_pred[d.k][f][0]==PR_NONE ) continue;
        uint cd = uint(d.n>>(2*ci++))&3;
        if( cd && hr_pred[d.k][f][cd-1]==PR_NONE ) return false;
      }
    }
    else if( d.code!=OP_LIST && d.code!=OP_RLIST && d.code!=OP_LEND && d.code!=OP_END ) return false;
    if( r.err ) return false;
    if( (d.code==OP_CHUNK || d.code==OP_IX || d.code==OP_INDX) && d.t>=M.trk.size() ) return false;
    if( (d.code==OP_IDX1 && d.k>3) || (d.code==OP_IX && d.k>1) || (d.code==OP_IDX1 && d.n>0xFFFFFFFFu) ) return false;
    M.ops.push_back( d );
    if( d.code==OP_END ) break;
  }
  M.st = r.p; M.stlen = qword(r.e-r.p);
  return true;
}

// per-track exception cursor: value for chunk k (lists are sorted; k increases)
struct ExcCur {
  const std::vector<qword>* idx; size_t i;
  void Reset( const std::vector<qword>* v ) { idx = v; i = 0; }
  bool At( qword k ) { while( i<idx->size() && (*idx)[i]<k ) i++; return i<idx->size() && (*idx)[i]==k; }
};

} // namespace avi
