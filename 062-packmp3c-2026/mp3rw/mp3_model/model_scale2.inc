
Number1ii<7> ylong[39];

void Init_Scale2() {
  Init_Number1ii(ylong);
}

// Channel ch's scalefactors, MPEG-2 (one granule), as mode says
// (model_scale1.inc: SCF_CODE, SCF_COPY, SCF_CHECK)
int Scale2Ch( int ch, int mode ) {
  RangeCoder& rc = *this;

  byte const *pnt;
  int i,j,n;
  uint slen;
  const int set = (mode!=SCF_CHECK);

  gr_info&  gi = si.gi[0][ch];
  ScaleFac& sf = SF[0][ch];
  const ScaleFac& s0 = SF[0][0];
  word*    scf = &sf.l[0];
  word*    sf0 = &sf.l[0];

  if( set ) gi.p23len = 0;

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
        int p = int(scf-sf0);
        SCF_SLOT( *scf, s0.l[p], Process_sf2( *scf, ch, p, num, n, ylong[p].P, M_ylong_wr, M_ylong_mw ) )
        scf++;
#endif
        if( set ) gi.p23len += num;
      }
    } else {
      for( j=0; j<int(pnt[i]); j++ ) { if( set ) *scf = 0; scf++; }
    }
  }

  if( set ) {
    // A place no group codes is 0, as the decoder has it.  The encoder's
    // came from the dump, which in a frame mp3rw could not parse holds
    // whatever was there: the stages and CalcSteps read these.
    while( scf < sf0+39 ) *scf++ = 0;

    // (the stage's last frame, of this channel and kind of block)
    for( i=0; i<39; i++ ) zsf[ch][n][i] = byte( sf.l[i] );
    zgg[ch] = gi.global_gain; zscs[ch] = gi.scalefac_scale;
  }

  return 1;
}
#undef SCF_SLOT

// whether channel ch's scalefactors have bits (MPEG-2: a group's of 0
// bits, or none, code nothing)
int Scale2Bits( int ch ) {
  const gr_info& gi = si.gi[0][ch];
  int i_stereo = ( ((h.mode_ext==1) || (h.mode_ext==3)) && (ch==1) );
  uint slen = i_stereo ? i_slen2[gi.scalefac_compress>>1] : n_slen2[gi.scalefac_compress];
  return (slen & 0xFFF)!=0;
}

void ProcessScale2( void ) {
  RangeCoder& rc = *this;

  int nch = (h.mode!=3);

  for( int ch=0; ch<=nch; ch++ ) {
    int dup = 0;
    if( ch && dup_last && Scale2Bits( ch ) ) {
      if( rc.rc_ProcMode==0 ) dup = Scale2Ch( ch, SCF_CHECK ); // (decoding: the flag decoded)
      int same = si.gi[0][1].scalefac_compress==si.gi[0][0].scalefac_compress;
      sfdup[0][same][sfdup_last].Process( rc, dup, TTable[24], 16 );
      sfdup_last = dup;
    }
    Scale2Ch( ch, dup ? SCF_COPY : SCF_CODE );
  } // for ch
 
}
