
int ProcScaleFactors2( void ) {

  int error=0;
  byte const *pnt;
  int i,j;
  uint slen;
  int n;

  int ch,gr=0;
  int nch = (h.mode!=3);

  for( ch=0; ch<=nch; ch++ ) {

    gr_info&  gi = si.gi[0][ch];
    ScaleFac& sf = SF[0][ch];
    if( ProcMode==1 ) memset( &sf,0,sizeof(ScaleFac) );
    int BufBorder= (ProcMode?BufPtr:OutPtr) - gi.part2_3_length;
    word*    scf = &sf.l[0];
//    if( gi.part2_3_length==0 ) continue;

    int i_stereo = ( ((h.mode_ext==1) || (h.mode_ext==3)) && (ch==1) );
    /* i_stereo AND second channel -> do_layer3() checks this */
    if( i_stereo ) 
      slen = i_slen2[gi.scalefac_compress>>1];
    else
      slen = n_slen2[gi.scalefac_compress];

    gi.preflag = (slen>>15) & 0x1;

    n = 0;  
    if( gi.block_type==2 ) {
      n++;
      if( gi.mixed_block_flag ) n++;
    }

    pnt = stab[n][(slen>>12)&0x7];

    for( i=0; i<4; i++ ) {
      int num = slen & 0x7;
      slen >>= 3;
      if( num ) {
        for( j=0; j<(int)(pnt[i]); j++ ) procbits( *scf++, num );
      } else {
        for( j=0; j<(int)(pnt[i]); j++ ) *scf++ = 0;
      }
    }
    
    n = (n << 1) + 1;
    for( i=0; i<n; i++ ) *scf++ = 0;

//Mask=0;
    error |= ProcMDCT( MC[gr][ch], gi, BufBorder );
//Mask=-1;

  }
 
  return error;
}


