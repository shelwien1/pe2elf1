// Matroska/WebM container
//
// Track streams hold the frame data of each track in block order (laced blocks split
// into their frames). Conversions: V_MPEG4/ISO/AVC, V_MPEGH/ISO/HEVC -> Annex B,
// A_AAC -> ADTS, A_FLAC -> .flac (CodecPrivate prepended), A_VORBIS -> Ogg, A_PCM/* and
// PCM-like A_MS/ACM -> WAV; V_MS/VFW/FOURCC and A_MS/ACM map to their codecs (MPEG-4
// part 2, MPEG-1/2, MJPEG, H.264, AAC, MP3, AC-3 ..); MPEG-4 part 2 / MPEG-1/2 video get
// the CodecPrivate headers prepended when the first frame lacks them; A_MPEG/L1..3,
// A_AC3, A_EAC3, A_DTS are elementary streams already. Header stripping
// (ContentCompAlgo 3) is undone in the streams. Attachments are cut out as blob
// streams. Regenerated on restore:
//   - frame sizes            : from self-delimiting frame data when it parses, from
//                              ADTS frame_length or Ogg packet boundaries; otherwise an
//                              explicit size column per track
//   - SimpleBlock, BlockGroup, Block and Cluster IDs and size fields (when the sizes
//                              have minimal length; else residuals vs the frame data)
//   - block timestamps       : model x*scale+offset, x = H.264 display index, Vorbis
//                              sample position or frame index; only residuals are stored
//   - ReferenceBlock         : predicted as ts(previous block) - ts(this)
//   - SimpleBlock or BlockGroup : the track's most common kind, or one kind for predicted
//                              key frames and one for the others (muxers that write key
//                              frames as BlockGroups among SimpleBlocks or the other way
//                              round); BlockGroup children likewise relative to the key
//                              prediction (none on key frames, a ReferenceBlock on others)
//   - block order of tracks  : predicted by smallest next timestamp (or decode-order
//                              time, with a per-track bias)
//   - cluster Timestamp      : predicted as the time of the cluster's first block
//   - lacing type, lace headers : libmatroska's choice / regenerated from frame sizes
//   - SeekHead entries, CueClusterPosition, CuePoints : from element positions and
//                              the blocks they point at; all-zero Void elements
//   - Vorbis CodecPrivate    : rebuilt from the .ogg header packets
//   - CRC-32 elements        : recomputed (first child of level-1 elements)
// Truncated files: blocks cut by EOF stay in the struct. The encoder runs the decoder in
// memory and falls back to raw track streams, then to the basic layout, then to a plain
// copy.

// MKV: EBML elements, laces, block headers
namespace mkv {

// CRC-32 element payload: IEEE CRC-32 (reflected 0xEDB88320, init and final xor ~0), little endian
static uint mkv_crc[256];
static uint Crc32( const byte* p, qword n ) {
  if( !mkv_crc[1] )
    for( uint i=0; i<256; i++ ) { uint r = i; for( int k=0; k<8; k++ ) r = r&1 ? (r>>1)^0xEDB88320u : r>>1; mkv_crc[i] = r; }
  uint c = 0xFFFFFFFFu;
  for( qword i=0; i<n; i++ ) c = (c>>8) ^ mkv_crc[(c^p[i])&0xFF];
  return ~c;
}


//--- EBML (shared) -----------------------------------------------------------

enum : uint {
  ID_EBML=0x1A45DFA3, ID_SEGMENT=0x18538067,
  ID_SEEKHEAD=0x114D9B74, ID_INFO=0x1549A966, ID_TRACKS=0x1654AE6B, ID_CLUSTER=0x1F43B675,
  ID_CUES=0x1C53BB6B, ID_ATTACHMENTS=0x1941A469, ID_CHAPTERS=0x1043A770, ID_TAGS=0x1254C367,
  ID_TRACKENTRY=0xAE, ID_TRACKNUMBER=0xD7, ID_CODECID=0x86, ID_CODECPRIVATE=0x63A2,
  ID_AUDIO=0xE1, ID_SAMPLINGFREQ=0xB5, ID_CHANNELS=0x9F, ID_CONTENTENCODINGS=0x6D80,
  ID_SIMPLEBLOCK=0xA3, ID_BLOCKGROUP=0xA0, ID_BLOCK=0xA1,
  ID_DISCARDPADDING=0x75A2, ID_TIMESTAMP=0xE7,
  ID_SEEK=0x4DBB, ID_SEEKPOSITION=0x53AC, ID_CUEPOINT=0xBB, ID_CUETRACKPOSITIONS=0xB7, ID_CUECLUSTERPOS=0xF1,
  ID_ATTACHEDFILE=0x61A7, ID_FILENAME=0x466E, ID_FILEMIMETYPE=0x4660, ID_FILEDATA=0x465C
};

static bool IsLevel1( uint id ) {
  return id==ID_SEEKHEAD || id==ID_INFO || id==ID_TRACKS || id==ID_CLUSTER || id==ID_CUES ||
         id==ID_ATTACHMENTS || id==ID_CHAPTERS || id==ID_TAGS || id==ID_EBML || id==ID_SEGMENT;
}

// element ID: 1..4 bytes, marker bit kept. returns length, 0 on error
static int ReadID( const byte* p, qword avail, uint& id ) {
  if( avail<1 || p[0]<0x10 ) return 0;
  int l = p[0]>=0x80 ? 1 : p[0]>=0x40 ? 2 : p[0]>=0x20 ? 3 : 4;
  if( qword(l)>avail ) return 0;
  id = 0; for( int i=0; i<l; i++ ) id = (id<<8)|p[i];
  return l;
}

// vint: 1..8 bytes. returns length (0 on error), value bits; unk = all value bits set
static int ReadVint( const byte* p, qword avail, qword& v, bool* unk=0 ) {
  if( avail<1 || p[0]==0 ) return 0;
  int l = 1; while( !(p[0]&(0x80>>(l-1))) ) l++;
  if( qword(l)>avail ) return 0;
  v = p[0] & (0xFF>>l);
  for( int i=1; i<l; i++ ) v = (v<<8)|p[i];
  if( unk ) *unk = v==(qword(1)<<(7*l))-1;
  return l;
}

static qword VintMask( int l ) { return (qword(1)<<(7*l))-1; }

// write a vint of length l with value bits v (v < 2^(7l))
static void PutVint( byte* p, int l, qword v ) {
  for( int i=l-1; i>=0; i-- ) p[i] = byte(v), v>>=8;
  p[0] |= byte(0x80>>(l-1));
}

// lace header after the block flags byte: count byte, then (Xiph/EBML) the sizes of
// frames 0..nf-2. o advances past it; with sz, the sizes are appended.
static bool ParseLace( const byte* p, qword avail, int lacing, qword& o, int& nf, std::vector<qword>* sz=0 ) {
  nf = 1;
  if( !lacing ) return true;
  if( o>=avail ) return false;
  nf = p[o++]+1;
  if( lacing==1 ) {                                 // Xiph: 255-runs
    for( int i=0; i<nf-1; i++ ) {
      qword s=0; uint c;
      do { if( o>=avail ) return false; c = p[o++]; s += c; } while( c==255 );
      if( sz ) sz->push_back( s );
    }
  } else if( lacing==3 && nf>1 ) {                  // EBML: vint, then signed vint deltas
    qword v; int k = ReadVint( p+o, avail-o, v );
    if( !k ) return false;
    o += k; long long s = (long long)v;
    if( sz ) sz->push_back( v );
    for( int i=1; i<nf-1; i++ ) {
      k = ReadVint( p+o, avail-o, v );
      if( !k ) return false;
      o += k; s += (long long)v - ((1LL<<(7*k-1))-1);
      if( s<0 ) return false;
      if( sz ) sz->push_back( qword(s) );
    }
  }
  return true;
}

// canonical lace header for frame sizes sz[0..nf-1] (minimal vints)
static void MakeLace( int lacing, const uint* sz, int nf, Buf& o ) {
  o.push_back( byte(nf-1) );
  if( lacing==1 ) {
    for( int i=0; i<nf-1; i++ ) { qword s = sz[i]; while( s>=255 ) o.push_back(255), s-=255; o.push_back( byte(s) ); }
  } else if( lacing==3 && nf>1 ) {
    byte b[8]; int l = 1;
    while( l<8 && qword(sz[0])>=VintMask(l) ) l++;
    PutVint( b, l, sz[0] ); o.insert( o.end(), b, b+l );
    for( int i=1; i<nf-1; i++ ) {
      long long dv = (long long)sz[i]-(long long)sz[i-1];
      for( l=1; l<8; l++ ) { long long bias = (1LL<<(7*l-1))-1; if( dv>=-bias && dv<=bias ) break; }
      PutVint( b, l, qword( dv + ((1LL<<(7*l-1))-1) ) ); o.insert( o.end(), b, b+l );
    }
  }
}

// (Simple)Block payload header: track vint, int16 timestamp, flags, lace header.
// hlen = header length, nf = frames. With sz: lace sizes of frames 0..nf-2 (Xiph/EBML).
struct BlkHdr { qword track; qword hlen; int nf, lacing, tvl; };

static bool ParseBlkHdr( const byte* p, qword avail, BlkHdr& h, std::vector<qword>* sz=0 ) {
  qword tn; int l = ReadVint( p, avail, tn );
  if( !l || avail<qword(l)+3 ) return false;
  uint fl = p[l+2]; qword o = l+3;
  h.track = tn; h.tvl = l; h.lacing = (fl>>1)&3;
  if( !ParseLace( p, avail, h.lacing, o, h.nf, sz ) ) return false;
  h.hlen = o;
  return true;
}

static qword ReadUintBE( const byte* p, qword n ) { qword v = 0; for( qword i=0; i<n && i<8; i++ ) v = (v<<8)|p[i]; return v; }

// shortest vint length that holds v (all-ones values are reserved for "unknown")
static int VintLen( qword v ) { int l = 1; while( l<8 && v>=VintMask(l) ) l++; return l; }

// shortest unsigned big-endian integer length for v (at least 1)
static int UMinLen( qword v ) { int n = 1; while( n<8 && (v>>(8*n)) ) n++; return n; }

// EBML unsigned integer element with minimal length
static void PutUintElem( Buf& o, uint id, qword v ) {
  if( id>0xFFFFFF ) o.push_back( byte(id>>24) );
  if( id>0xFFFF ) o.push_back( byte(id>>16) );
  if( id>0xFF ) o.push_back( byte(id>>8) );
  o.push_back( byte(id) );
  int n = UMinLen( v ); o.push_back( byte(0x80|n) );
  for( int j=n-1; j>=0; j-- ) o.push_back( byte(v>>(8*j)) );
}

// shortest signed big-endian integer length for v
static int MinSLen( long long v ) {
  int n = 1;
  while( n<8 && !( v>=-(1LL<<(8*n-1)) && v<(1LL<<(8*n-1)) ) ) n++;
  return n;
}

// lacing type libmatroska picks for these frame sizes: fixed if all equal, else the
// shorter of Xiph (1) and EBML (3)
static int LacePred( const uint* sz, int nf ) {
  bool same = true;
  for( int i=1; i<nf; i++ ) same &= sz[i]==sz[0];
  if( same ) return 2;
  qword xl = 0, el = VintLen( sz[0] );
  for( int i=0; i<nf-1; i++ ) xl += sz[i]/255+1;
  for( int i=1; i<nf-1; i++ ) {
    long long dv = (long long)sz[i]-(long long)sz[i-1]; int l = 1;
    while( l<8 && !( dv>=-((1LL<<(7*l-1))-1) && dv<=(1LL<<(7*l-1))-1 ) ) l++;
    el += l;
  }
  return xl<el ? 1 : 3;
}

static void H264Display( const byte* hdr, qword hl, const byte* s, const std::vector<qword>& fo, qword nf, int ls, std::vector<qword>& disp ) {
  std::vector<byte> fl;
  VideoFeat<H264State>( hdr, hl, s, fo, nf, ls, disp, fl );
}

// codec positions for the timestamp model: 1 = H.264 display index, 2 = Vorbis sample position,
// 3 = HEVC display index
static bool CodecPositions( int ckind, int cparam, const Buf& hdr, const Buf& priv,
                            const byte* s, const std::vector<qword>& fo, qword nf, std::vector<qword>& cp ) {
  if( ckind==1 ) {
    if( cparam<1 || cparam>4 ) return false;
    H264Display( hdr.data(), hdr.size(), s, fo, nf, cparam, cp );
    return true;
  }
  if( ckind==3 ) {
    if( cparam<1 || cparam>4 ) return false;
    std::vector<byte> fl;
    VideoFeat<HevcState>( hdr.data(), hdr.size(), s, fo, nf, cparam, cp, fl );
    return true;
  }
  if( ckind==2 ) {
    const byte* hp[3]; qword hl[3]; VorbisInfo vi;
    if( !SplitXiphHeaders( priv.data(), priv.size(), hp, hl ) || !VorbisParse( hp, hl, vi ) ) return false;
    cp.resize( size_t(nf) ); uint prev = 0; qword g = 0;
    for( qword f=0; f<nf; f++ ) cp[size_t(f)] = g, g += VorbisDur( vi, s+fo[size_t(f)], fo[size_t(f+1)]-fo[size_t(f)], prev );
    return true;
  }
  return false;
}

} // namespace mkv
