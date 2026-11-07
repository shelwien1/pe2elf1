
// max number of bits per scalefactor = 4
Number1ii<4> small[32][12];  // for short block
Number1ii<4> mixed[2][8];    // for mixed block


// Channel 1's scalefactors the same as channel 0's, where it codes them
// (a mono recording in two channels, L/R): a flag before them, then they
// are copied, not coded. The flag comes only after a granule whose values
// were channel 0's (model_delta.inc's flag: dup_last), so that a stereo
// stream, where channels have the same scalefactors by chance, codes as
// before, and where they have bits (else they cost nothing). By MPEG
// version, whether the channels' scalefac_compress are the same, and the
// last granule's flag.
Node2ii sfdup[2][2][2];
int sfdup_last;

void Init_Scale1() {
  Init_Number1ii(small);
  Init_Number1ii(mixed);
  Init_Node2ii(sfdup);
  sfdup_last = 0;
}

// What a channel's scalefactors go through (Scale1Ch(), Scale2Ch()):
// coded; copied from channel 0's at their places (the flag above); or,
// the encoder deciding that flag, compared with them, nothing changed
enum { SCF_CODE, SCF_COPY, SCF_CHECK };
// a scalefactor v (channel 0's at its place: w), as the mode says
#define SCF_SLOT( v, w, code ) \
  if( mode==SCF_CODE ) { code; } else if( mode==SCF_COPY ) v = w; else if( v!=w ) return 0;

// Channel ch's scalefactors of granule gr, MPEG-1, as mode says (above).
// SCF_CHECK: whether each it codes is channel 0's.
int Scale1Ch( int gr, int ch, int mode ) {
  RangeCoder& rc = *this;

  int i,j,k,b;
  const int set = (mode!=SCF_CHECK); // (the scalefactors and the state of their stage changed)
  gr_info& gi = si.gi[gr][ch];
  ScaleFac& sf= SF[gr][ch];
  const ScaleFac& s0 = SF[gr][0];
  uint     l1 = slen1_tab[ gi.scalefac_compress ];
  uint     l2 = slen2_tab[ gi.scalefac_compress ];

  if( set ) gi.p23len = 0;

  if( gi.block_type==2 ) {

    if( j = (gi.mixed_block_flag ? 3 : 0) ) {
      // (the short bands a mixed block does not code are 0, as the
      // decoder has them -- the encoder's come from the dump, see
      // model_scale2.inc -- since the short blocks' stage reads them)
      if( set ) for( i=0; i<3; i++ ) for( b=0; b<3; b++ ) sf.s[i][b]=0;
#ifndef DISABLE_s1mixed
      for( i=0; i<8; i++ ) { SCF_SLOT( sf.l[i], s0.l[i], mixed[ch][i].Process( rc, sf.l[i], -1, (1<<l1), M_mixed_wr, M_mixed_mw ) ) }
#endif
      if( set ) {
        for( i=8; i<21; i++ ) sf.l[i]=0;
        gi.p23len += 8*l1;
      }
    } else {
      if( set ) for( i=0; i<21; i++ ) sf.l[i]=0;
    }

    for( i=j; i<12; i++ ) for( b=0; b<3; b++ ) {
#ifndef DISABLE_s1small
      SCF_SLOT( sf.s[i][b], s0.s[i][b], Process_sf1short( sf.s[i][b], ch, gr, i, b, (i<6)?l1:l2, small[ch+2*gi.scalefac_compress][i].P, M_small_wr, M_small_mw ) )
#endif
      if( set ) gi.p23len += (i<6)?l1:l2;
    }
    // (the stage's last short granule, of this channel)
    if( set ) for( i=0; i<12; i++ ) for( b=0; b<3; b++ ) ssf[ch][i][b] = byte( sf.s[i][b] );

  } else {

    // (a long block codes no short-block scalefactors: 0, as above)
    if( set ) for( i=0; i<13; i++ ) for( b=0; b<3; b++ ) sf.s[i][b]=0;

    for( k=0, b=0,i=0,j=6; b<4; b++,i=j,j+=5 ) {

      int flag=0;
      if( gr==1 ) flag = si.scfsi[ch][b];
      if( flag==0 ) {
        for(; i<j; i++ ) {
#ifndef DISABLE_s1xlong
          SCF_SLOT( sf.l[i], s0.l[i], Process_sf1xlong( sf.l[i], ch, k, ((i<11)?l1:l2), i, gr ) )
#endif
          k = sf.l[i];
          if( set ) gi.p23len += (i<11)?l1:l2;
        }
      } else if( set ) {
        // (granule 0's of this channel, scfsi: not channel 0's, under the flag too)
        for(; i<j; i++ ) SF[1][ch].l[i] = SF[0][ch].l[i];
      }

    } // for

    // (the stage's last granule, of this channel)
    if( set ) {
      for( i=0; i<21; i++ ) ysf[ch][i] = byte( sf.l[i] );
      ygg[ch] = gi.global_gain; yscs[ch] = gi.scalefac_scale; ypre[ch] = gi.preflag;
    }
  }

  return 1;
}

// whether channel ch's scalefactors of granule gr have bits (MPEG-1)
int Scale1Bits( int gr, int ch ) {
  const gr_info& gi = si.gi[gr][ch];
  int l1 = slen1_tab[ gi.scalefac_compress ], l2 = slen2_tab[ gi.scalefac_compress ], b, n = 0;
  if( gi.block_type==2 ) return l1+l2; // (short and mixed blocks: bands of both)
  for( b=0; b<4; b++ ) if( !(gr==1 && si.scfsi[ch][b]) ) n += b<2 ? l1 : l2;
  return n;
}

void ProcessScale1( void ) {
  RangeCoder& rc = *this;

  int gr,ch;
  int ngr = h.ID;
  int stereo = (h.mode!=3)+1; //  int stereo = (h.mode==3) ? 1 : 2;

  for( gr=0; gr<=ngr; gr++) {
    for( ch=0; ch<stereo; ch++ ) {
      int dup = 0;
      if( ch && dup_last && Scale1Bits( gr, ch ) ) {
        if( rc.rc_ProcMode==0 ) dup = Scale1Ch( gr, ch, SCF_CHECK ); // (decoding: the flag decoded)
        int same = si.gi[gr][1].scalefac_compress==si.gi[gr][0].scalefac_compress;
        sfdup[1][same][sfdup_last].Process( rc, dup, TTable[24], 16 );
        sfdup_last = dup;
      }
      Scale1Ch( gr, ch, dup ? SCF_COPY : SCF_CODE );
    } // for ch
  } // for gr

}
