// MKV: meta track part, ops, decoder structures and meta parser
namespace mkv {

//--- MKV track part and ops (after the common head, see meta.inc) ------------------
//
// Common head: szmode 0 = explicit frame sizes, 2 = derived from frame data, 3 = from the
// transform. Blob records (attachments) end after the head. Columns: see meta.inc.
//   u8 tvlen | 0x80*strip, tvlen bytes   track number vint as written in this track's blocks
//   [u8 n, n bytes]           strip: header stripping prefix (ContentCompAlgo 3); the stream
//                             (after the inverse transform) holds the frames with the prefix,
//                             the size model applies to them, blocks store them without it
//   vint nblocks              block header columns follow
//   u8   dkind | kmode<<2 | dkey<<3 | grel<<5
//                             dkind: block op of this track used by 0xFD: 0 = SimpleBlock,
//                             1 = BlockGroup, 2 = non-canonical block; dkey (needs km): 0 =
//                             dkind for all blocks, else blocks whose first frame is a predicted
//                             key frame use op dkey-1 (key frames as other blocks than the rest);
//                             grel (needs km): group column values 0 and 1 swapped for blocks
//                             whose first frame is not a predicted key frame (no children on key
//                             frames, the predicted ReferenceBlock on the others: all 0)
//   vint zigzag kbias         interleave key of the track's next block (0xFD, 0xF9):
//                             no model: cluster timestamp + int16(timestamp); model, kmode 0:
//                             predicted time + int16(residual); kmode 1: round((f*m+c)/2^k),
//                             f = frame index (decode order); plus kbias
//   vint seg                  segment of the track (count of 0xFB ops before its blocks - 1);
//                             only tracks of the current segment are predicted
//   u8 km, [u8 kp]            key prediction of SimpleBlock flags (see KeyPred): bit 0x80 of the
//                             flags column is relative to it
//   u8   tsmode               block timestamps (int16, relative to the cluster):
//                             0 = column of zigzag(delta vs previous block of the track)
//                             1 = model: u8 ckind (0 = frame index, 1 = H.264 display index
//                                 [u8 NAL length size], 2 = Vorbis sample position,
//                                 3 = HEVC display index [u8 NAL length size]),
//                                 u8 k, vint m, vint c: predicted absolute time =
//                                 round((x*m+c)/2^k), k<64,
//                                 x = ckind position of the block's first frame;
//                                 column of zigzag(delta of residual), residual =
//                                 timestamp - (predicted - cluster timestamp), mod 2^16
//                             2 = piecewise model: u8 ckind [u8 NAL length size], u8 ns,
//                                 ns x (u8 k, vint m), column of the state per block
//                                 (scale #state>>5, c = ((state&31)<<k)>>5), then the
//                                 residual column as in 1
//   column                    flags byte per block; when laced, the lacing bits hold
//                             1+(lacing-predicted)%3 (predicted: fixed if all frame sizes are
//                             equal, else the shorter of Xiph and EBML)
//   column                    per laced block: 1 = lace header regenerated from the frame
//                             sizes, 0 = kept in struct
//   vint n, column            per regenerated lace header: frame count-1
//   vint n, column            per canonical BlockGroup op of the track: 0 = no other children,
//                             1 = one ReferenceBlock equal to the prediction (not stored),
//                             n+2 = n children in struct
//
// ops:      0x00..0x3F  SimpleBlock of track #code; ID and size are regenerated (size
//                       vint of minimal length), struct holds the lace header unless
//                       regenerated; track/timestamp/flags come from the columns
//           0x40..0x7F  BlockGroup of track #code-0x40, Block first, both sizes
//                       regenerated; struct holds the lace header, then the children
//                       given by the track's group column. ReferenceBlock values are
//                       stored minus the prediction ts(previous block of the track)-ts(this)
//           0x80..0xDF  Block of track #code-0x80, sizes not canonical: struct holds its
//                       ID, the size vint with value bits = (size - header - frame data)
//                       mod 2^(7*len) and the lace header unless regenerated
//      0xF0 n      copy n bytes from struct
//      0xE0 d f    CuePoint rebuilt from block #(previous+1+zigzag d) (an earlier block in
//                  op order): CueTime = its timestamp, one CueTrackPositions with
//                  CueTrack, CueClusterPosition (its cluster), and if f&1
//                  CueRelativePosition (its BlockGroup/SimpleBlock in the cluster data),
//                  if f&2 CueDuration (its BlockDuration); minimal lengths throughout
//      0xE1 n l    Void element with n zero bytes, size vint of length l
//      0xE3 n      CRC-32 element (BF 84 + 4 bytes), the CRC of the next n-1 output bytes
//                  (n = 0: the rest of the current cluster)
//      0xE2 d f    Seek entry for mark #(previous+zigzag d) (shared with 0xFC): SeekID =
//                  the ID of the element at the mark ((f&3)+1 bytes), SeekPosition = mark -
//                  segment data start ((f>>2)+1 bytes); Seek size vint minimal
//      0xF1 t      as 0x80+t, for any t
//      0xF2 t k    size vint in struct, value bits relative to the next k frames of
//                  track t (non-canonical BlockGroup size; frames are not consumed)
//      0xF3 t      whole stream of blob track t
//      0xF4 t      CodecPrivate payload of Ogg track t, rebuilt from its header
//                  packets (Xiph lacing); the element ID and size stay in struct
//      0xF5 t      as 0x00+t, 0xF6 t: as 0x40+t, for any t
//      0xF7        Cluster: ID and minimal-length size of everything up to the matching
//                  0xF8 are regenerated; the cluster start is a mark
//      0xF8        end of cluster
//      0xF9        cluster Timestamp element in struct, its value stored minus the prediction
//                  (predicted time + residual of the next block among the tracks with a
//                  timestamp model, chosen by smallest interleave key; else 0);
//                  sets the timestamp base
//      0xFE        cluster Timestamp equal to the prediction, minimal length: not stored
//      0xFA        mark: position of a level-1 element (target of 0xFC)
//      0xFB        segment data start (base of 0xFC positions)
//      0xFC d      position element (SeekPosition, CueClusterPosition): struct holds ID
//                  and size, the value is mark #(previous+zigzag d) - segment data start
//      0xFD n      n blocks; each belongs to the track whose next block has the smallest
//                  interleave key (first track on ties) and uses that track's dkind op
//      0xFF        copy rest of struct, end
// Restore runs the program twice: sizes and mark positions first, then output.

enum { OP_CUE=0xE0, OP_VOID=0xE1, OP_SEEK=0xE2, OP_CRC=0xE3, OP_RAW=0xF0, OP_BLOCK=0xF1, OP_GSIZE=0xF2, OP_BLOB=0xF3, OP_PRIV=0xF4, OP_SB=0xF5, OP_GB=0xF6,
       OP_CLUSTER=0xF7, OP_CEND=0xF8, OP_CTS=0xF9, OP_MARK=0xFA, OP_SEG=0xFB, OP_POS=0xFC, OP_PRUN=0xFD, OP_CTSP=0xFE, OP_END=0xFF };

//--- decoder -----------------------------------------------------------------

struct DTrack {
  TrackHead h;
  int xform, lensize, nhdr; qword hdrlen; IdxLen xa, xb;
  qword nsamp; int szmode, ptype, pparam;
  std::vector<uint> ucnt; std::vector<qword> esc;
  std::vector<uint> sz, xsz;                        // frame sizes; sizes from the transform
  Buf priv;                                         // Ogg: CodecPrivate rebuilt from the header packets
  Buf xhdr;                                         // Annex B: the parameter set header
  std::vector<qword> fo;                            // frame offsets in the raw stream (nsamp+1)
  Buf tvb; Buf bfl, breg;                           // block header columns
  Buf strip, sbuf;                                  // header stripping prefix; frames without it
  int tsmode, ckind, cparam, sk; qword sm, sc;      // timestamp model
  std::vector<qword> tsk, tsmv, tst;                // piecewise model: scales (k, m), state per block
  int dkind, kmode; long long kbias; qword seg;     // default block op, interleave key, segment (OP_PRUN)
  int dkey, grel;                                   // default op of predicted key frames + 1; group column relative to kf
  qword tnum;                                       // track number
  std::vector<uint> te;                             // per block: int16 delta (timestamp or residual)
  std::vector<qword> gmode, cpos, lcnt;             // group children column; codec positions; lace counts
  int km, kp; std::vector<byte> kf;                 // key prediction model, per frame
  uint r16; bool haslast; long long lastabs;        // restore state
};

struct DOp { int code; qword t, n; };

struct Meta {
  int ver;
  const byte* st; qword stlen;
  std::vector<DOp> ops;
  std::vector<DTrack> trk;
};

// key frame prediction for SimpleBlock flags (bit 0x80 is coded relative to it):
// km 0 none, 1 every frame, 2 H.264 / 3 HEVC: IDR/IRAP from the length-prefixed NALs as
// H264State / HevcState see them (they need parameter sets in the frames; kp = NAL length
// size), 4 H.264 / 5 HEVC: by NAL type, no parameter sets needed (kp = NAL length size |
// sync mask<<3, see NalFeat and SyncPred), 16+ck: frame class ck (kp = class parameter)
static void KeyPred( int km, int kp, const byte* const* fp, const qword* fz, qword nf, std::vector<byte>& kf ) {
  kf.assign( size_t(nf), byte(km==1) );
  if( km==2 || km==3 ) {
    H264State h4; HevcState h5;
    for( qword k=0; k<nf; k++ ) {
      bool vcl = false, reset = false; int f0 = 0; long long poc = 0;
      const byte* s = fp[k]; qword e = fz[k];
      for( qword o=0; e-o>=qword(kp); ) {
        qword L = 0; for( int i=0; i<kp; i++ ) L = (L<<8)|s[o+i];
        o += kp;
        if( L>e-o ) break;
        if( km==2 ) h4.Nal( s+o, size_t(L), vcl, f0, poc, reset ); else h5.Nal( s+o, size_t(L), vcl, f0, poc, reset );
        o += L;
      }
      kf[size_t(k)] = (f0&VF_KEY)!=0;
    }
  } else if( km==4 || km==5 )
    for( qword k=0; k<nf; k++ ) kf[size_t(k)] = SyncPred( NalFeat( km==5, kp&7, fp[k], fz[k] ), kp>>3 );
  else if( km>=16 )
    for( qword k=0; k<nf; k++ ) kf[size_t(k)] = (FrameClass( km-16, fp[k], fz[k], uint(kp) )&FC_KEY)!=0;
}

static bool GetCountCol( Reader& r, std::vector<qword>& v ) {
  qword n = r.vint(); return !r.err && n<=MAXCOUNT && GetCol( r, v, n );
}

static bool ParseMeta( const Buf& m, Meta& M ) {
  if( MetaFormat( m.data(), m.size() )!=FMT_MKV ) return false;
  Reader r( m.data()+1, m.size()-1 );
  M.ver = 4;
  qword nt = r.vint(); if( r.err || nt>0xFFFF ) return false;
  M.trk.resize( size_t(nt) );
  for( size_t i=0; i<M.trk.size(); i++ ) {
    DTrack& t = M.trk[i];
    if( !GetHead( r, t.h ) || t.h.szmode==SZ_TABLES ) return false;
    t.xform = t.h.xform; t.lensize = t.h.lensize; t.nhdr = t.h.nhdr; t.hdrlen = t.h.hdrlen; t.xa = t.h.xa; t.xb = t.h.xb;
    t.nsamp = 0; t.szmode = 0; t.ptype = t.pparam = 0;
    t.tsmode = 0; t.ckind = t.cparam = t.sk = 0; t.sm = t.sc = 0; t.dkind = t.kmode = t.dkey = t.grel = 0; t.kbias = 0; t.seg = 0;
    if( t.xform==XF_BLOB ) continue;
    t.nsamp = t.h.nf; t.szmode = t.h.szmode;
    if( t.szmode==SZ_EXPLICIT ) t.sz.assign( t.h.col.begin(), t.h.col.end() );
    else if( t.szmode==SZ_UNITS ) t.ptype = t.h.ptype, t.pparam = t.h.pparam, t.ucnt.assign( t.h.col.begin(), t.h.col.end() ), t.esc = t.h.esc;
    qword tl = r.u8(); const byte* tv = r.bytes(tl&0x7F);
    if( r.err || (tl&0x7F)<1 || (tl&0x7F)>8 ) return false;
    t.tvb.assign( tv, tv+(tl&0x7F) ); t.strip.clear();
    if( tl&0x80 ) { qword sl = r.u8(); const byte* sp = r.bytes(sl); if( r.err || !sl ) return false; t.strip.assign( sp, sp+sl ); }
    tl &= 0x7F;
    if( !ReadVint( tv, tl, t.tnum ) ) return false;
    qword nb = r.vint(); if( r.err || nb>MAXCOUNT ) return false;
    t.te.resize( size_t(nb) ); t.bfl.resize( size_t(nb) );
    qword nl = 0;
    std::vector<qword> v;
    uint dk = r.u8(); t.dkind = int(dk&3); t.kmode = int((dk>>2)&1); t.dkey = int((dk>>3)&3); t.grel = int((dk>>5)&1);
    t.kbias = UnZigZag( r.vint() ); t.seg = r.vint();
    if( t.dkind>2 || dk>63 ) return false;
    t.km = r.u8(); t.kp = t.km>=2 ? int(r.u8()) : 0;
    if( (t.km>5 && t.km<16) || t.km>16+CK_VC1 || ((t.km==2 || t.km==3) && (t.kp<1 || t.kp>4)) ) return false;
    if( (t.km==4 || t.km==5) && ((t.kp&7)<1 || (t.kp&7)>4 || ((t.kp>>3)&~14)) ) return false;
    if( (t.dkey || t.grel) && !t.km ) return false;
    t.tsmode = r.u8();
    if( t.tsmode==1 || t.tsmode==2 ) {
      t.ckind = r.u8();
      if( t.ckind==1 || t.ckind==3 ) t.cparam = r.u8();
      if( t.ckind>3 || (t.ckind==2 && t.xform!=XF_OGG) ) return false;
      if( t.tsmode==1 ) {
        t.sk = r.u8(); t.sm = r.vint(); t.sc = r.vint();
        if( t.sk>63 ) return false;
      } else {
        int ns = r.u8();
        if( ns<1 || ns>8 ) return false;
        t.tsk.resize( ns ); t.tsmv.resize( ns );
        for( int i=0; i<ns; i++ ) { t.tsk[i] = r.u8(); t.tsmv[i] = r.vint(); if( t.tsk[i]>63 ) return false; }
        if( !GetCol( r, t.tst, nb ) ) return false;
        for( size_t k=0; k<t.tst.size(); k++ ) if( t.tst[k]>=qword(32*ns) ) return false;
      }
    } else if( t.tsmode!=0 ) return false;
    if( !GetCol( r, v, nb ) ) return false;
    for( size_t k=0; k<v.size(); k++ ) t.te[k] = uint( UnZigZag( v[k] ) ) & 0xFFFF;
    if( !GetCol( r, v, nb ) ) return false;
    for( size_t k=0; k<v.size(); k++ ) { if( v[k]>255 ) return false; t.bfl[k] = byte(v[k]); nl += (v[k]&6)!=0; }
    if( !GetCol( r, v, nl ) ) return false;
    t.breg.resize( size_t(nl) );
    for( size_t k=0; k<v.size(); k++ ) t.breg[k] = byte( v[k]!=0 );
    if( !GetCountCol( r, t.lcnt ) ) return false;
    for( size_t k=0; k<t.lcnt.size(); k++ ) if( t.lcnt[k]>255 ) return false;
    if( !GetCountCol( r, t.gmode ) ) return false;
    if( r.err ) return false;
  }
  M.ops.clear();
  for(;;) {
    DOp d; d.code = r.u8(); d.t = 0; d.n = 0;
    if( r.err ) return false;
    if( d.code<OP_RAW ) {
      if( d.code<0x40 ) d.t = qword(d.code), d.code = OP_SB;
      else if( d.code<0x80 ) d.t = qword(d.code-0x40), d.code = OP_GB;
      else if( d.code<0xE0 ) d.t = qword(d.code-0x80), d.code = OP_BLOCK;
      else if( d.code==OP_CUE ) { d.n = r.vint(); d.t = r.u8(); if( d.t>3 ) return false; }
      else if( d.code==OP_VOID ) { d.n = r.vint(); d.t = r.u8(); if( d.t<1 || d.t>8 || d.n>=VintMask(int(d.t)) ) return false; }
      else if( d.code==OP_SEEK ) { d.n = r.vint(); d.t = r.u8(); if( d.t>31 ) return false; }
      else if( d.code==OP_CRC ) d.n = r.vint();
      else return false;
    }
    else if( d.code==OP_RAW ) d.n = r.vint();
    else if( d.code==OP_BLOCK || d.code==OP_BLOB || d.code==OP_PRIV ) d.t = r.vint();
    else if( d.code==OP_GSIZE ) d.t = r.vint(), d.n = r.vint();
    else if( d.code==OP_SB || d.code==OP_GB ) d.t = r.vint();
    else if( d.code==OP_POS ) d.n = r.vint();
    else if( d.code==OP_PRUN ) { d.n = r.vint(); if( d.n==0 || d.n>MAXCOUNT ) return false; }
    else if( !((d.code>=OP_CLUSTER && d.code<=OP_SEG) || d.code==OP_CTSP) && d.code!=OP_END ) return false;
    if( r.err ) return false;
    if( d.code==OP_BLOCK || d.code==OP_GSIZE || d.code==OP_BLOB || d.code==OP_PRIV || d.code==OP_SB || d.code==OP_GB ) {
      if( d.t>=M.trk.size() ) return false;
      if( (M.trk[size_t(d.t)].xform==XF_BLOB) != (d.code==OP_BLOB) ) return false;
      if( d.code==OP_PRIV && M.trk[size_t(d.t)].xform!=XF_OGG ) return false;
    }
    M.ops.push_back( d );
    if( d.code==OP_END ) break;
  }
  M.st = r.p; M.stlen = qword(r.e-r.p);
  return true;
}

// inverse stream transforms: converted stream c -> raw sample data r
static bool UnAnnexB( const DTrack& t, const Buf& c, Buf& r ) { return FromAnnexB( c, t.hdrlen, t.lensize, t.xa, t.xb, r ); }

static bool UnADTS( DTrack& t, const Buf& c, Buf& r ) { return FromADTS( c, t.nsamp, t.xa, r, t.xsz ); }

static bool UnHdr( const DTrack& t, const Buf& c, Buf& r ) {
  if( t.hdrlen>c.size() ) return false;
  r.assign( c.begin()+size_t(t.hdrlen), c.end() );
  return true;
}

// Ogg pages -> packets; the first nhdr are headers (-> t.priv, Xiph-laced), the rest are the frames
static bool UnOgg( DTrack& t, const Buf& c, Buf& r ) { return FromOgg( c, t.nsamp, t.nhdr, r, t.xsz, t.priv ); }

static const byte ID_CLUSTER_B[4] = { 0x1F, 0x43, 0xB6, 0x75 };

// timestamp model of block k at position x
static qword ModelAt( const DTrack& t, size_t k, qword x ) {
  if( t.tsmode==1 ) return MulShift( x, t.sm, t.sk, t.sc );
  qword s = k<t.tst.size() ? t.tst[k] : 0; size_t si = size_t(s>>5); int kk = int(t.tsk[si]);
  return MulShift( x, t.tsmv[si], kk, (qword(s&31)<<kk)>>5 );
}

} // namespace mkv
