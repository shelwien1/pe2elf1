
// max number of bits per scalefactor = 4
Number1ii<4> small[32][12];  // for short block
Number1ii<4> mixed[2][8];    // for mixed block


void Init_Scale1() {
  Init_Number1ii(small);
  Init_Number1ii(mixed);
}

void ProcessScale1( void ) {
  RangeCoder& rc = *this;

  int gr,ch,i,ii,j,k;
  int ngr = h.ID;
  int stereo = (h.mode!=3)+1; //  int stereo = (h.mode==3) ? 1 : 2;

  for( gr=0; gr<=ngr; gr++) {
    for( ch=0; ch<stereo; ch++ ) {

      int      sfb, b,b0;
      gr_info& gi = si.gi[gr][ch];
      ScaleFac& sf= SF[gr][ch];
      uint     l1 = slen1_tab[ gi.scalefac_compress ];
      uint     l2 = slen2_tab[ gi.scalefac_compress ];

      gi.p23len = 0;

      if( gi.block_type==2 ) {

        if( j = (gi.mixed_block_flag ? 3 : 0) ) {
          // (the short bands a mixed block does not code are 0, as the
          // decoder has them -- the encoder's come from the dump, see
          // model_scale2.inc -- since the short blocks' stage reads them)
          for( i=0; i<3; i++ ) for( b=0; b<3; b++ ) sf.s[i][b]=0;
#ifndef DISABLE_s1mixed
          for( i=0; i<8; i++ ) mixed[ch][i].Process( rc, sf.l[i], -1, (1<<l1), M_mixed_wr, M_mixed_mw );
#endif
          for( i=i; i<21; i++ ) sf.l[i]=0;
          gi.p23len += 8*l1;
        } else {
          for( i=0; i<21; i++ ) sf.l[i]=0;
        }

        for( i=j; i<12; i++ ) for( b=0; b<3; b++ ) {
#ifndef DISABLE_s1small
          Process_sf1short( sf.s[i][b], ch, gr, i, b, (i<6)?l1:l2, small[ch+2*gi.scalefac_compress][i].P, M_small_wr, M_small_mw );
#endif
          gi.p23len += (i<6)?l1:l2;
        }
        // (the stage's last short granule, of this channel)
        for( i=0; i<12; i++ ) for( b=0; b<3; b++ ) ssf[ch][i][b] = byte( sf.s[i][b] );

      } else {

        // (a long block codes no short-block scalefactors: 0, as above)
        for( i=0; i<13; i++ ) for( b=0; b<3; b++ ) sf.s[i][b]=0;

        for( k=0, b=0,b0=0,i=0,j=6; b<4; b++,i=j,j+=5 ) {

          int flag=0;
          if( gr==1 ) flag = si.scfsi[ch][b];
//          if( (gr==0) || (si.scfsi[ch][b]==0) ) {
          if( flag==0 ) {
            for(; i<j; i++ ) {
#ifndef DISABLE_s1xlong
              Process_sf1xlong( sf.l[i], ch, k, ((i<11)?l1:l2), i, gr );
#endif
              k = sf.l[i];
              gi.p23len += (i<11)?l1:l2;
            }
          } else {
            for(; i<j; i++ ) {
              SF[1][ch].l[i] = SF[0][ch].l[i];
            }
          }

        } // for

        // (the stage's last granule, of this channel)
        for( i=0; i<21; i++ ) ysf[ch][i] = byte( sf.l[i] );
        ygg[ch] = gi.global_gain; yscs[ch] = gi.scalefac_scale; ypre[ch] = gi.preflag;
      }

    } // for ch
  } // for gr

}
