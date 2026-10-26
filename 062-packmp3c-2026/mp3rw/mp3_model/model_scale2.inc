
Number1ii<7> ylong[39];

void Init_Scale2() {
  Init_Number1ii(ylong);
}

void ProcessScale2( void ) {
  RangeCoder& rc = *this;

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
    word*    scf = &sf.l[0];
    word*    sf0 = &sf.l[0];

    gi.p23len = 0;

    int i_stereo = ( ((h.mode_ext==1) || (h.mode_ext==3)) && (ch==1) );

    // i_stereo AND second channel -> do_layer3() checks this
    if( i_stereo ) 
      slen = i_slen2[gi.scalefac_compress>>1];
    else
      slen = n_slen2[gi.scalefac_compress];

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
        for( j=0; j<int(pnt[i]); j++ ) {
#ifndef DISABLE_s2ylong
          // index before the increment, as C++17 and the original mp3c.exe order it
          Process_sf2( *scf, ch, int(scf-sf0), num, n, ylong[scf-sf0].P, M_ylong_wr, M_ylong_mw ); scf++;
#endif
          gi.p23len += num;
        }
      } else {
        for( j=0; j<int(pnt[i]); j++ ) *scf++ = 0;
      }
    }
    // A place no group codes is 0, as the decoder has it.  The encoder's
    // came from the dump, which in a frame mp3rw could not parse holds
    // whatever was there: the stages and CalcSteps read these.
    while( scf < sf0+39 ) *scf++ = 0;

    // (the stage's last frame, of this channel and kind of block)
    for( i=0; i<39; i++ ) zsf[ch][n][i] = byte( sf.l[i] );
    zgg[ch] = gi.global_gain; zscs[ch] = gi.scalefac_scale;

  } // for ch
 
}

