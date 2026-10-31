// MP4: meta track part, ops, decoder structures and meta parser
namespace mp4 {

//--- MP4 track part and ops (after the common head, see meta.inc) ------------------
//
// Common head: szmode 1 = sizes from the stsz/stz2/trun tables in struct, 2 = derived
// from sample data, 3 = from the transform. Blob records (cover art) end after the head.
//   u8   has_stbl
//     vint nchunks, stsc_ptr, stsc_len                  (ptr/len into struct)
//     szmode 1: u8 stz2, vint sz_ptr, sz_len
//     nsamples < stbl+trun samples (v4+): truncated file, only the stbl chunks holding
//     the first nsamples-trun samples are present; stco/stsz are regenerated for those
//   vint ntrun, per trun: vint ptr delta (trun payload in struct)
//     szmode 1: vint default_size (used if the trun has no size field)
//   u8 tflags: 1 = timing model (all samples: durations, composition offsets, sync)
//                   2 = trun sample_duration fields derived (with 1, szmode>=2)
//                   4 = trun sample_composition_time_offset fields derived (with 1, szmode>=2)
//                   8 = tfdt base: vint zigzag(base)
//                  16 = trun sample_flags fields derived (with 1, szmode>=2): from the sample
//                       class (key | intra<<1 | non-reference<<2, as sdtp; ckind 4 without stss
//                       too); after the tfdt base: 8 x vint flags per class, exception index
//                       column, column of the exception values
//     timing model (if 1):
//       u8 ckind: 0 none, 1 H.264, 2 HEVC (+u8 NAL length size), 3 Vorbis sample positions,
//         4 key frames by frame class (+u8 class kind, vint class parameter; see FrameClass)
//       vint q, u8 k, vint m, vint c: model_dts(n) = q*round((x(n)*m+c)/2^k),
//         x(n) = sample index, or Vorbis position (sum of packet durations) for ckind 3
//       column: zigzag(duration(n) - (model_dts(n+1)-model_dts(n)))
//       u8 hascto; if: u8 ks, vint zigzag(D), column of zigzag(cto(n) - pred(n)),
//         pred(n) = dts(min(disp(n)+ks, N)) - dts(n) + D, dts = sums of the durations,
//         disp = display index from the picture order counts (ckind 1/2; else n)
//     u8 cflags: 1 = stsc entries regenerated: vint sdi, u8 mode:
//                         0: column of samples-per-chunk (nchunks values)
//                         1: aligned (needs timing models): vint master track, vint ts, vint ts_m
//                            (timescales), vint zigzag(a), u8 rule, vint zigzag(O), runs of
//                            zigzag(R) (nchunks values):
//                            cum(c) = samples of this track with t*ts_m (<|<=) B*ts + O, B =
//                            start time of the master's chunk c-a+1 (0 if c-a+1<1, all if
//                            past the master's chunks or c is the last chunk); +R(c)
//                     2 = chunk order group member: vint zigzag(b) (see op 0xF3)
//                     4 = (with 1) the last chunk has its own stsc entry
//                     (cflags is written after the timing fields)
//       u8 syncmask (0xFF: no sync column), exception index column:
//         sync(n) = IDR/IRAP picture (ckind 4: key frame class), mask 2: or recovery point
//         SEI, 4: or intra first slice, 8: or both; flipped for the listed samples
//       u8 hassdtp; if: 8 bytes value per class (key | intra<<1 | non-reference<<2),
//         exception index column, one byte value per exception
//
// ops: 0x00..0xEF  next chunk of track #code (stbl chunks first, then one chunk per trun)
//      0xF0 n      copy n bytes from struct
//      0xF3 n      n chunks (v7+), each the next chunk of the group track with the smallest
//                  (next chunk index - b, track index)
//      0xF1 t c    chunk c of track t
//      0xF2 k t    regenerate table k of track t:
//                  1=stco, 2=co64, 3=stsz entries; 4=next trun's entries
//                  (struct holds them without the sample_size field)
//                  6=trun data_offset: struct holds pos-chunkpos+data_offset (u32)
//                  7=esds DecoderSpecificInfo of an Ogg track (v6+), rebuilt from its
//                    header packets (Xiph lacing); descriptor tag/size stay in struct;
//                    header transform tracks: the codec config (esds DSI / glbl payload)
//                    = the stream header
//                  8=stts, 9=ctts entries, 10=stss entries (v7+): from the timing model;
//                    the entry count stays in struct, entries are maximal runs
//                  12=stsc entries (v7+): maximal runs of the regenerated samples-per-chunk
//                  13=parameter set NAL in avcC/hvcC (v7+): the next NAL of the Annex B
//                    stream header (its 16-bit length stays in struct)
//                  14=sdtp sample bytes (v7+): from the timing model's class map
//                  11=tfdt baseMediaDecodeTime (v7+): base + dts of the first sample of
//                    the track's next trun (32/64 bit by the tfdt version)
//                  4 (v7+): trun records without the derived fields (size if szmode>=2,
//                    duration / composition offset / sample_flags by tflags) of the trun
//                    whose payload is the last before this point of the struct (a trun
//                    with no derived field has no op)
//      0xF2 5      tfhd base_data_offset: struct holds pos-value (u64)
//                  (pos = output position of the field; residuals are ~constant)
//      0xF4        mark: the current output position (top-level boxes except mdat, from the
//                  first moof on; references for 0xF2 16..18)
//      0xF2 16     sidx entries; struct: vint count, vint j0 (mark of the first subsegment),
//                  vint track+1 (0: none), columns of zigzag residuals: referenced word vs
//                  mark distance, duration vs the track's sample durations in the range
//                  (see SidxPredict), SAP word xor the previous entry's
//      0xF2 17     tfra entries; struct: u8 version, u8 length codes, vint count, vint
//                  track+1, columns: zigzag residuals of time and moof_offset vs TfraPredict,
//                  traf, trun and sample numbers
//      0xF2 18     mfro size: output position after it - the last mark
//      0xFF        copy rest of struct, end

enum { OP_RAW=0xF0, OP_CHUNK=0xF1, OP_TABLE=0xF2, OP_CRUN=0xF3, OP_MARK=0xF4, OP_END=0xFF };
enum { TB_STCO=1, TB_CO64=2, TB_STSZ=3, TB_TRUN=4, TB_BDO=5, TB_DOFF=6, TB_DSI=7, TB_STTS=8, TB_CTTS=9, TB_STSS=10, TB_TFDT=11, TB_STSC=12, TB_PSET=13, TB_SDTP=14,
       TB_MARK=15, TB_SIDX=16, TB_TFRA=17, TB_MFRO=18 };
static bool TbNoTrack( qword k ) { return k==TB_BDO || k==TB_MARK || k==TB_SIDX || k==TB_TFRA || k==TB_MFRO; }

//--- decoder -----------------------------------------------------------------

struct DTrack {
  TrackHead h;
  qword nsamp; int szmode, ptype, pparam;
  bool has_stbl; qword nch_stbl, npc, stsc_ptr, stsc_len, sz_ptr, sz_len; int stz2;   // npc: present stbl chunks
  std::vector<qword> trun, tdef;
  std::vector<uint> ucnt; std::vector<qword> esc;
  int xform, lensize, nhdr; qword hdrlen; IdxLen xa, xb;
  std::vector<uint> xsz;                              // sizes from the transform (szmode 3)
  Buf priv;                                           // Ogg: esds DSI rebuilt from the header packets
  Buf xhdr;                                           // Annex B: parameter set header
  qword pcur;                                         // TB_PSET cursor into xhdr
  int tflags; TimeModel tm; long long tbase;          // timing model (v7)
  FlagMap fm;                                         // trun sample_flags per sample class (tflags 16)
  int cflags, cmode, crule; qword csdi, cmaster, csj, csm; long long ca, cO, cb;   // chunk model (v7)
  std::vector<qword> ccol;                            // spc runs / aligned residuals
  // derived
  std::vector<qword> dur, dts; std::vector<long long> cto; std::vector<byte> sync; Buf sdtp; std::vector<uint> sfl;
  uint csize; qword nconst; std::vector<uint> sz;     // sizes: [0,nconst) = csize, rest in sz
  std::vector<qword> spc, cfirst, csz, cso, cpos;
  uint Size( qword i ) const { return i<nconst ? csize : sz[size_t(i-nconst)]; }
};

struct DOp { int code; int t; qword c; qword n; };

struct Meta {
  int ver;
  const byte* st; qword stlen;
  std::vector<DOp> ops;
  std::vector<DTrack> trk;
};

static bool ParseMeta( const Buf& m, Meta& M ) {
  if( MetaFormat( m.data(), m.size() )!=FMT_MP4 ) return false;
  Reader r( m.data()+1, m.size()-1 );
  M.ver = 7;
  qword nt = r.vint(); if( r.err || nt>0xFFFF ) return false;
  M.trk.resize( size_t(nt) );
  for( size_t i=0; i<M.trk.size(); i++ ) {
    DTrack& t = M.trk[i];
    if( !GetHead( r, t.h ) || t.h.szmode==SZ_EXPLICIT ) return false;
    t.xform = t.h.xform; t.lensize = t.h.lensize; t.nhdr = t.h.nhdr; t.hdrlen = t.h.hdrlen; t.xa = t.h.xa; t.xb = t.h.xb;
    t.tflags = 0; t.tbase = 0; t.cflags = t.cmode = t.crule = 0; t.csdi = t.cmaster = t.csj = t.csm = 0; t.ca = t.cO = t.cb = 0;
    if( t.xform==XF_BLOB ) {
      t.nsamp=0; t.szmode=0; t.has_stbl=false; t.nch_stbl=t.npc=t.stsc_ptr=t.stsc_len=t.sz_ptr=t.sz_len=0;
      t.stz2=0; t.ptype=t.pparam=0; t.trun.clear(); t.tdef.clear(); t.ucnt.clear(); t.esc.clear();
      continue;
    }
    t.nsamp = t.h.nf; t.szmode = t.h.szmode==SZ_TABLES ? 0 : t.h.szmode;
    t.ptype = t.h.ptype; t.pparam = t.h.pparam;
    t.ucnt.assign( t.h.col.begin(), t.h.col.end() ); t.esc = t.h.esc;
    t.has_stbl = r.u8()!=0;
    t.nch_stbl=t.stsc_ptr=t.stsc_len=t.sz_ptr=t.sz_len=0; t.stz2=0;
    if( t.has_stbl ) {
      t.nch_stbl = r.vint(); t.stsc_ptr = r.vint(); t.stsc_len = r.vint();
      if( t.szmode==0 ) t.stz2 = r.u8(), t.sz_ptr = r.vint(), t.sz_len = r.vint();
    }
    qword ntr = r.vint(); if( r.err || ntr>MAXCOUNT ) return false;
    t.trun.resize( size_t(ntr) ); t.tdef.assign( size_t(ntr), 0 );
    qword p=0;
    for( size_t k=0; k<t.trun.size() && !r.err; k++ ) {
      p += r.vint(); t.trun[k] = p;
      if( t.szmode==0 ) t.tdef[k] = r.vint();
    }
    t.tflags = r.u8();
    if( t.tflags>31 || ((t.tflags&30) && !(t.tflags&1)) || ((t.tflags&22) && t.szmode<2) ) return false;
    if( (t.tflags&1) && !GetTimeModel( r, t.tm, t.nsamp ) ) return false;
    if( (t.tflags&4) && !t.tm.hascto ) return false;
    if( t.tflags&8 ) t.tbase = UnZigZag( r.vint() );
    if( (t.tflags&16) && !GetFlagMap( r, t.fm, t.nsamp ) ) return false;
    t.cflags = r.u8();
    if( t.cflags>7 || ((t.cflags&1) && !t.has_stbl) || ((t.cflags&4) && !(t.cflags&1)) ) return false;
    if( t.cflags&1 ) {
      t.csdi = r.vint(); t.cmode = r.u8();
      if( t.csdi>0xFFFFFFFFu || t.cmode>1 ) return false;
      if( t.cmode==1 ) {
        t.cmaster = r.vint(); t.csj = r.vint(); t.csm = r.vint();
        t.ca = UnZigZag( r.vint() ); t.crule = r.u8(); t.cO = UnZigZag( r.vint() );
        if( t.cmaster>=nt || t.cmaster==i || t.crule>1 || !(t.tflags&1) || !t.csj || !t.csm || t.csj>0xFFFFFFFFu || t.csm>0xFFFFFFFFu ) return false;
      }
      if( !GetCol( r, t.ccol, t.nch_stbl ) ) return false;
    }
    if( t.cflags&2 ) t.cb = UnZigZag( r.vint() );
    if( r.err ) return false;
  }
  // ops
  M.ops.clear();
  for(;;) {
    DOp d; d.code = r.u8(); d.t=0; d.c=0; d.n=0;
    if( r.err ) return false;
    if( d.code<OP_RAW ) d.t = d.code;
    else if( d.code==OP_RAW ) d.n = r.vint();
    else if( d.code==OP_CHUNK ) d.t = int(r.vint()), d.c = r.vint();
    else if( d.code==OP_CRUN ) { d.n = r.vint(); if( d.n==0 || d.n>MAXCOUNT ) return false; }
    else if( d.code==OP_MARK ) { d.code = OP_TABLE; d.n = TB_MARK; }
    else if( d.code==OP_TABLE ) { d.n = r.u8(); if( d.n<TB_STCO || d.n>TB_MFRO ) return false; if( !TbNoTrack( d.n ) ) d.t = int(r.vint()); }
    else if( d.code!=OP_END ) return false;
    if( r.err ) return false;
    if( d.code!=OP_RAW && d.code!=OP_END && d.code!=OP_CRUN && !(d.code==OP_TABLE && TbNoTrack( d.n )) && size_t(d.t)>=M.trk.size() ) return false;
    M.ops.push_back( d );
    if( d.code==OP_END ) break;
  }
  M.st = r.p; M.stlen = qword(r.e-r.p);
  for( size_t i=0; i<M.trk.size(); i++ )                 // trun headers live in the struct
    for( size_t k=0; k<M.trk[i].trun.size(); k++ ) {
      qword p = M.trk[i].trun[k]; TrunHdr h;
      if( p>M.stlen || !ParseTrunHdr(M.st+p,M.stlen-p,h) ) return false;
    }
  return true;
}

// inverse stream transforms: converted stream c -> raw sample data r
static bool UnAnnexB( const DTrack& t, const Buf& c, Buf& r ) { return FromAnnexB( c, t.hdrlen, t.lensize, t.xa, t.xb, r ); }

static bool UnADTS( DTrack& t, const Buf& c, Buf& r ) { return FromADTS( c, t.nsamp, t.xa, r, t.xsz ); }

// Ogg pages -> packets; the first nhdr are headers (-> t.priv, Xiph-laced), the rest are the frames
static bool UnOgg( DTrack& t, const Buf& c, Buf& r ) { return FromOgg( c, t.nsamp, t.nhdr, r, t.xsz, t.priv ); }

// decode times before the sample sizes are known (chunk model inputs): sample index
// abscissa, or Vorbis positions from the Ogg packet sizes
static bool EarlyDts( const DTrack& t, const Buf& s, std::vector<qword>& dts ) {
  if( !(t.tflags&1) ) return false;
  TimeIn ti; Buf none;
  if( t.tm.ckind==3 ) {
    if( t.xsz.size()!=t.nsamp ) return false;
    std::vector<qword> fo( size_t(t.nsamp+1), 0 );
    for( qword k=0; k<t.nsamp; k++ ) fo[size_t(k+1)] = fo[size_t(k)]+t.xsz[size_t(k)];
    if( fo.back()>s.size() || !TimeInputs( 3, 0, none, t.priv, s.data(), fo, t.nsamp, ti ) ) return false;
  } else {
    ti.x.resize( size_t(t.nsamp+1) );
    for( qword n=0; n<=t.nsamp; n++ ) ti.x[size_t(n)] = n;
  }
  std::vector<qword> dur; TimeDur( t.tm, ti, t.nsamp, dur, dts );
  return true;
}

} // namespace mp4
