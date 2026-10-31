// display index and feature flags per sample. codec 1 = H.264, 2 = HEVC. hdr: Annex B
// parameter sets (may be empty); samples are length-prefixed NALs (ls bytes) at s+fo[f]
template<class ST>
static void VideoFeat( const byte* hdr, qword hl, const byte* s, const std::vector<qword>& fo, qword nf, int ls,
                       std::vector<qword>& disp, std::vector<byte>& fl ) {
  ST h; bool vcl, reset; int f0; long long poc = 0;
  for( qword i=NextSC(hdr,0,hl); i<hl; ) {
    qword a = i+4, e = NextSC( hdr, a, hl );
    vcl = true; reset = false; f0 = 0;      // parameter sets only
    if( e>a ) h.Nal( hdr+a, size_t(e-a), vcl, f0, poc, reset );
    i = e;
  }
  std::vector<std::pair<long long,qword> > per;
  disp.assign( size_t(nf), 0 ); fl.assign( size_t(nf), 0 );
  qword base = 0; long long last = 0;
  for( qword f=0; f<=nf; f++ ) {
    vcl = reset = false; f0 = 0; poc = last+2;
    if( f<nf )
      for( qword o=fo[size_t(f)], e=fo[size_t(f+1)]; e-o>=qword(ls); ) {
        qword L = 0; for( int i=0; i<ls; i++ ) L = (L<<8)|s[o+i];
        o += ls;
        if( L>e-o ) break;
        h.Nal( s+o, size_t(L), vcl, f0, poc, reset ); o += L;
      }
    if( f==nf || (reset && per.size()) ) {   // a POC period ends: rank its pictures
      std::stable_sort( per.begin(), per.end(), []( const std::pair<long long,qword>& a, const std::pair<long long,qword>& b ) { return a.first<b.first; } );
      for( size_t i=0; i<per.size(); i++ ) disp[size_t(per[i].second)] = base+i;
      base += per.size(); per.clear();
    }
    if( f<nf ) per.push_back( std::make_pair( poc, f ) ), last = poc, fl[size_t(f)] = byte(f0);
  }
}

// sync prediction from feature flags: mask bit 1 adds recovery-point frames, bit 2 intra
// frames, bit 3 frames that are both
static bool SyncPred( byte f, int mask ) {
  return (f&VF_KEY) || ((mask&2) && (f&VF_RP)) || ((mask&4) && (f&VF_I)) || ((mask&8) && (f&VF_RP) && (f&VF_I));
}

// feature flags of one sample of length-prefixed NALs (ls bytes) that need no parameter sets:
// VF_KEY H.264 IDR / HEVC IRAP NAL, VF_RP recovery point SEI, VF_I H.264 I or SI first slice
static byte NalFeat( bool hevc, int ls, const byte* s, qword n ) {
  byte f = 0; bool first = true;
  if( ls<1 ) return 0;
  for( qword o=0; n-o>=qword(ls); ) {
    qword L = 0; for( int i=0; i<ls; i++ ) L = (L<<8)|s[o+i];
    o += ls;
    if( L>n-o ) break;
    const byte* p = s+o; o += L;
    if( L<2 ) continue;
    if( hevc ) {
      int ty = (p[0]>>1)&63;
      if( ty>=16 && ty<=23 ) f |= VF_KEY;
      else if( ty==39 && SeiRecovery( p+2, size_t(L-2) ) ) f |= VF_RP;
      continue;
    }
    int ty = p[0]&31;
    if( ty==5 ) f |= VF_KEY;
    else if( ty==6 && SeiRecovery( p+1, size_t(L-1) ) ) f |= VF_RP;
    if( (ty==1 || ty==5) && first ) {            // first_mb_in_slice, slice_type
      BitsMSB r; r.Load( p+1, size_t(L-1), 16 ); r.ue(); uint st = r.ue();
      if( !r.err && (st%5==2 || st%5==4) ) f |= VF_I;
      first = false;
    }
  }
  return f;
}
