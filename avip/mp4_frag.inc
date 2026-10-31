// MP4: fragments, size modes, struct cutting
namespace mp4 {

static void ParseFragments( const byte* d, qword fs, const std::vector<Box>& top, const Box& moov, std::vector<Track>& T, std::vector<qword>& bdo_pos ) {
  std::vector<Trex> trex; Box mvex;
  if( FindChild(d,moov,FCC_BE('m','v','e','x'),mvex) ) {
    std::vector<Box> c = Children( d, mvex.pay(), mvex.end() );
    for( size_t i=0; i<c.size(); i++ )
      if( c[i].type==FCC_BE('t','r','e','x') && c[i].plen()>=24 ) {
        Trex x = { BE32(d+c[i].pay()+4), BE32(d+c[i].pay()+16), BE32(d+c[i].pay()+12) }; trex.push_back(x);
      }
  }
  for( size_t m=0; m<top.size(); m++ ) {
    if( top[m].type!=FCC_BE('m','o','o','f') ) continue;
    qword mpos = top[m].pos, prevend = mpos; bool firsttraf = true;
    std::vector<Box> tf = Children( d, top[m].pay(), top[m].end() );
    for( size_t f=0; f<tf.size(); f++ ) {
      if( tf[f].type!=FCC_BE('t','r','a','f') ) continue;
      Box tfhd;
      if( !FindChild(d,tf[f],FCC_BE('t','f','h','d'),tfhd) || tfhd.plen()<8 ) continue;
      const byte* p = d+tfhd.pay(); qword pl = tfhd.plen();
      uint fl = BE32(p)&0xFFFFFF, tid = BE32(p+4); qword o=8, bdo=0; uint defsize=0, defdur=0; bool hasdef=false, hasdur=false;
      if( fl&1 ) { if( pl<o+8 ) continue; bdo = BE64(p+o); bdo_pos.push_back( tfhd.pay()+o ); o+=8; }
      if( fl&2 ) o+=4;
      if( fl&8 ) { if( pl<o+4 ) continue; defdur = BE32(p+o); hasdur=true; o+=4; }
      if( fl&0x10 ) { if( pl<o+4 ) continue; defsize = BE32(p+o); hasdef=true; o+=4; }
      Track* t = 0;
      for( size_t i=0; i<T.size(); i++ ) if( T[i].tid==tid ) { t=&T[i]; break; }
      for( size_t i=0; i<trex.size(); i++ ) if( trex[i].tid==tid ) { if( !hasdef ) defsize=trex[i].size; if( !hasdur ) defdur=trex[i].dur; break; }
      Box tfdt; bool htf = FindChild(d,tf[f],FCC_BE('t','f','d','t'),tfdt) && tfdt.plen()>=8 && (d[tfdt.pay()]==0 || (d[tfdt.pay()]==1 && tfdt.plen()>=12));
      if( t && htf ) {
        Track::Tfdt x; x.pos = tfdt.pay()+4; x.ver = d[tfdt.pay()]; x.val = x.ver ? BE64(d+x.pos) : BE32(d+x.pos); x.trun = t->trun.size();
        t->tfdt.push_back( x );
      }
      qword base = (fl&1) ? bdo : ((fl&0x20000) || firsttraf) ? mpos : prevend;
      qword pos = base;
      std::vector<Box> tr = Children( d, tf[f].pay(), tf[f].end() );
      for( size_t r=0; r<tr.size(); r++ ) {
        if( tr[r].type!=FCC_BE('t','r','u','n') ) continue;
        TrunInfo ti; ti.b = tr[r];
        const byte* q = d+tr[r].pay();
        bool good = ParseTrunHdr( q, tr[r].plen(), ti.h ) && qword(ti.h.recsize)*ti.h.count<=tr[r].plen()-ti.h.hlen;
        if( good && (ti.h.flags&1) ) {
          int doff = int(BE32(q+8));
          if( doff<0 && qword(-qword(doff))>base ) good=false; else pos = base+qword(int64_t(doff));
        }
        qword sum=0; std::vector<uint> sz;
        if( good ) {
          sz.resize( ti.h.count );
          for( uint j=0; j<ti.h.count; j++ )
            sum += sz[j] = (ti.h.flags&0x200) ? BE32(q+ti.h.hlen+qword(j)*ti.h.recsize+ti.h.szoff) : defsize;
        }
        if( t && t->ok && !t->trunc ) {               // after truncation: absent, left in struct
          if( !good ) t->ok = false;
          else if( t->AddChunk( pos, ti.h.count, sum, fs ) ) {
            t->fsz.insert( t->fsz.end(), sz.begin(), sz.end() ); t->trun.push_back( ti );
            for( uint j=0; j<ti.h.count; j++ ) {        // per-sample timing
              const byte* rr = q+ti.h.hlen+qword(j)*ti.h.recsize;
              t->sdur.push_back( (ti.h.flags&0x100) ? BE32(rr) : defdur );
              int co = 4*( ((ti.h.flags>>8)&1)+((ti.h.flags>>9)&1)+((ti.h.flags>>10)&1) );
              t->scto.push_back( (ti.h.flags&0x800) ? (long long)int32_t(BE32(rr+co)) : 0 );
            }
            if( ti.h.flags&0x800 ) t->fcto = true;
          }
          else {                                       // data runs past EOF: keep the whole samples that fit
            qword m=0, b=0;
            if( pos<fs ) while( m<ti.h.count && sz[size_t(m)]<=fs-pos-b ) b += sz[size_t(m)], m++;
            if( m ) {
              t->AddChunk( pos, m, b, fs ); t->fsz.insert( t->fsz.end(), sz.begin(), sz.begin()+size_t(m) ); t->trun.push_back( ti );
              for( uint j=0; j<m; j++ ) {                   // timing of the present samples (the trun itself stays in struct)
                const byte* rr = q+ti.h.hlen+qword(j)*ti.h.recsize;
                t->sdur.push_back( (ti.h.flags&0x100) ? BE32(rr) : defdur );
                int co = 4*( ((ti.h.flags>>8)&1)+((ti.h.flags>>9)&1)+((ti.h.flags>>10)&1) );
                t->scto.push_back( (ti.h.flags&0x800) ? (long long)int32_t(BE32(rr+co)) : 0 );
              }
            }
            t->trunc = true;
          }
        }
        pos += sum;
      }
      prevend = pos; firsttraf = false;
    }
  }
}

// choose sample-size representation: derive from sample data if it's self-delimiting
static void ChooseSizeMode( const byte* d, Track& t ) {
  t.szmode = 0; t.ptype = PT_NONE; t.pparam = 0;
  qword n = t.nsamp;
  if( n==0 || (t.has_stbl && t.ss.count && (t.ss.csize || t.stz2)) ) return;
  bool cut = t.has_stbl && t.nstp;                    // something to deduplicate?
  for( size_t k=0; k<t.trun.size(); k++ ) if( (t.trun[k].h.flags&0x200) && t.trun[k].h.count ) cut = true;
  if( !cut ) return;
  if( t.xform==XF_ADTS || t.xform==XF_OGG ) { t.szmode = 3; return; }   // ADTS frame_length / Ogg lacing carry the sizes
  if( t.cpt==PT_VOP || t.cpt==PT_M2V || t.cpt==PT_JPEG || t.cpt==PT_AU264 ) {   // start code units: sizes checked in stream context
    std::vector<qword> fo( (size_t)n ); std::vector<uint> fz( (size_t)n ); Buf raw; raw.reserve( size_t(t.bytes) );
    for( size_t c=0; c<t.coff.size(); c++ ) {
      raw.insert( raw.end(), d+t.coff[c], d+t.coff[c]+t.csz[c] );
      qword o = t.coff[c];
      for( qword j=t.cfirst[c]; j<t.cfirst[c]+t.spc[c]; o+=t.Size(j), j++ ) fo[size_t(j)] = o, fz[size_t(j)] = t.Size(j);
    }
    TrackHead h;
    if( ChooseUnits( t.cpt, t.cpp, d, fo, fz, raw, h ) ) {
      t.szmode = 2; t.ptype = h.ptype; t.pparam = h.pparam; t.esc.swap( h.esc );
      t.ucnt.resize( h.col.size() ); for( size_t k=0; k<h.col.size(); k++ ) t.ucnt[k] = uint(h.col[k]);
    }
    return;
  }
  int cand[8][2]; int nc=0;
  if( t.aacp>=0 ) cand[nc][0]=PT_AAC, cand[nc++][1]=t.aacp;
  if( t.lensize ) cand[nc][0]=PT_NAL, cand[nc++][1]=t.lensize;
  cand[nc][0]=PT_OBU, cand[nc++][1]=0;
  cand[nc][0]=PT_MPA, cand[nc++][1]=0;
  cand[nc][0]=PT_AC3, cand[nc++][1]=0;
  qword best = n/8+1; int bi=-1;
  std::vector<uint> cnt( (size_t)n ), bcnt;
  for( int k=0; k<nc; k++ ) {
    qword e=0;
    for( size_t c=0; c<t.coff.size() && e<best; c++ ) {
      qword o = t.coff[c];
      for( qword j=t.cfirst[c]; j<t.cfirst[c]+t.spc[c]; j++ ) {
        uint z = t.Size(j);
        long long u = CountUnits( cand[k][0], cand[k][1], d+o, z );
        if( u<0 || u>=0xFFFFFFFFll ) cnt[size_t(j)]=0, e++; else cnt[size_t(j)]=uint(u+1);
        o += z;
      }
    }
    if( e<best ) { best=e; bi=k; bcnt.swap(cnt); cnt.resize(size_t(n)); if( e==0 ) break; }
  }
  if( bi<0 ) return;
  t.szmode = 2; t.ptype = cand[bi][0]; t.pparam = cand[bi][1];
  t.ucnt.swap( bcnt ); t.esc.clear();
  for( size_t j=0; j<n; j++ ) if( t.ucnt[j]==0 ) t.esc.push_back( t.Size(j) );
}

static bool Intersects( qword a, qword b, const std::vector<Item>& it ) {
  // it: sorted, non-overlapping, so ends are monotonic; check the last cutting item starting before b
  size_t lo=0, hi=it.size();
  while( lo<hi ) { size_t m=(lo+hi)/2; if( it[m].off<b ) lo=m+1; else hi=m; }
  for( size_t i=lo; i-- >0; ) if( it[i].rem ) return it[i].off+it[i].size>a;   // in-place transforms don't count
  return false;
}

static qword StructOff( qword p, const std::vector<Item>& it, const std::vector<qword>& cut ) {
  // cut[i] = bytes removed by items[0..i)
  size_t lo=0, hi=it.size();
  while( lo<hi ) { size_t m=(lo+hi)/2; if( it[m].off<p ) lo=m+1; else hi=m; }
  return p-cut[lo];
}

// forward stream transforms: raw sample data s (sample order) -> converted stream c
static void ToAnnexB( Track& t, const Buf& s, Buf& c ) {
  ::ToAnnexB( s, [&]( qword j ) { return t.Size(j); }, t.nsamp, t.lensize, t.xhdr, c, t.xa, t.xb );
}

static void ToADTS( Track& t, const Buf& s, Buf& c ) {
  t.xb.clear(); ::ToADTS( s, [&]( qword j ) { return t.Size(j); }, t.nsamp, t.adts, c, t.xa );
}

// Vorbis packets -> Ogg Vorbis file (header packets from the codec config).
// trim: samples to cut from the end (lowers the final granulepos)
static void ToOgg( Track& t, const Buf& s, Buf& c, uint serial, qword trim ) {
  ::ToOgg( s, [&]( qword j ) { return t.Size(j); }, t.nsamp, t.vh, t.vl, t.vi, serial, trim, c );
}

// aligned chunk model fit for a dependent track (see AlignedCum): best a, rule, O by
// interval stabbing, residuals R. Returns the coded cost estimate.
static size_t FitAligned( const std::vector<qword>& dj, qword Nj, qword sj, const std::vector<qword>& B, qword sm,
                          const std::vector<qword>& act, long long& ra, int& rrule, long long& rO, std::vector<qword>& R ) {
  const long long INF = 1LL<<62;
  size_t nch = act.size(), best = ~size_t(0);
  for( long long a=-1; a<=2; a++ ) {
    std::vector<long long> lo, hi;
    for( size_t c=0; c+1<nch; c++ ) {
      long long kk = (long long)c-a+1;
      if( kk<1 || kk>=(long long)B.size() ) continue;
      U128 E = U128::Mul( B[size_t(kk)], sj ); qword C = act[c];
      lo.push_back( C>0 ? U128::Mul( dj[size_t(C-1)], sm ).Diff( E ) : -INF );
      hi.push_back( C<Nj ? U128::Mul( dj[size_t(C)], sm ).Diff( E ) : INF );
    }
    std::vector<long long> sl = lo, sh = hi;
    std::sort( sl.begin(), sl.end() ); std::sort( sh.begin(), sh.end() );
    for( int rule=0; rule<2; rule++ ) {
      std::vector<long long> cand = rule ? lo : hi; cand.push_back( 0 );
      std::sort( cand.begin(), cand.end() ); cand.erase( std::unique( cand.begin(), cand.end() ), cand.end() );
      long long bo = 0; size_t bh = 0; bool any = false;
      for( size_t i=0; i<cand.size(); i++ ) {
        long long O = cand[i]; size_t h;
        if( O<=-INF || O>=INF ) continue;
        if( !rule ) h = size_t( std::lower_bound( sl.begin(), sl.end(), O )-sl.begin() ) - size_t( std::lower_bound( sh.begin(), sh.end(), O )-sh.begin() );
        else h = size_t( std::upper_bound( sl.begin(), sl.end(), O )-sl.begin() ) - size_t( std::upper_bound( sh.begin(), sh.end(), O )-sh.begin() );
        if( !any || h>bh || (h==bh && llabs(O)<llabs(bo)) ) bh = h, bo = O, any = true;
      }
      std::vector<qword> cum, r( nch );
      AlignedCum( dj, Nj, sj, B, sm, a, rule, bo, nch, cum );
      for( size_t c=0; c<nch; c++ ) r[c] = ZigZag( (long long)(act[c]-cum[c]) );
      size_t cost = RunsCost( r )+24;
      if( cost<best ) best = cost, ra = a, rrule = rule, rO = bo, R.swap( r );
    }
  }
  return best;
}

} // namespace mp4
