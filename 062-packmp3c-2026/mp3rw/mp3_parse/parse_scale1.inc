
int ProcScaleFactors1( void ) {

  int ch,gr,nch,ngr,i,j,b;
  int error=0;
  ngr = h.ID;
  nch = (h.mode!=3);

  for( gr=0; gr<=ngr; gr++ ) {
    for( ch=0; ch<=nch; ch++ ) {

      gr_info& gi = si.gi[gr][ch];
      ScaleFac& sf= SF[gr][ch];
      uint l1 = slen1_tab[ gi.scalefac_compress ];
      uint l2 = slen2_tab[ gi.scalefac_compress ];
      if( gi.part2_3_length==0 ) continue;
      int BufBorder= (ProcMode?BufPtr:OutPtr) - gi.part2_3_length;

      if( ProcMode==1 ) memset( &sf,0,sizeof(ScaleFac) );

      if( gi.block_type==2 ) {

        if( j = (gi.mixed_block_flag ? 3 : 0) )
          for( i=0; i<8; i++ ) procbits( sf.l[i], l1 );

        for( i=j; i<12; i++ ) for( b=0; b<3; b++ )
          procbits( sf.s[i][b], ((i<6)?l1:l2) );

      } else {

        for( b=0,i=0,j=6; b<4; b++,i=j,j+=5 ) {
         if( (gr==0) || (si.scfsi[ch][b]==0) ) {
          for(; i<j; i++ )
           procbits( sf.l[i], ((i<11)?l1:l2) );
         } else {
          for(; i<j; i++ ) {
           SF[1][ch].l[i] = SF[0][ch].l[i];
          }
         }
        }

      }

//if( (ch==0 && gr==1) ) Mask=0;
      error |= ProcMDCT( MC[gr][ch], gi, BufBorder );
//Mask=-1;

    }
  }

  return error;
}


