
// Generic APPn handler for APP2..APP15: identify the segment by its leading
// signature string and decode the few whose header is worth reporting.  What
// we do not read is skipped in bulk by the outer loop.

byte sig[24];
uint siglen;
uint icc_seq, icc_num;
uint adobe_ver, adobe_f0, adobe_f1;

void pa_APPn( void ) { CORO_START;

  // Adobe's APP14 signature is exactly "Adobe" with NO terminator -- the next
  // byte is already the version field -- so it cannot go through the
  // scan-until-terminator loop below without swallowing part of the header.
  if( tag_id==M_APP14 ) {

    if( l_tag<5 ) { pjpg_warn( "APP14 too short for a signature" ); CORO_EXIT; }
    for( siglen=0; siglen<5; siglen++ ) {
      sig[siglen] = coro2_get();
    }
    for( siglen=0; siglen<5; siglen++ ) sig[siglen] = pr(sig[siglen]);
    sig[5] = 0;
    pf( "APP14: %s\n", (char*)sig );

    if( (sig[0]=='A') && (sig[1]=='d') && (sig[2]=='o') && (sig[3]=='b') && (sig[4]=='e') ) {
      if( l_tag<7 ) { pjpg_warn( "truncated Adobe APP14 header" ); CORO_EXIT; }
      adobe_ver = coro2_get2();
      adobe_f0  = coro2_get2();
      adobe_f1  = coro2_get2();
      Adobe_transform = coro2_get();
      saw_Adobe_marker = 1;
      pf( "  Adobe marker: version %u, flags 0x%04X/0x%04X, transform %u\n",
              adobe_ver, adobe_f0, adobe_f1, Adobe_transform );
      if( Adobe_transform>2 ) pjpg_warn( "unknown Adobe colour transform" );
    }
    CORO_EXIT;
  }

  // Everything else uses a NUL-terminated ASCII signature.
  siglen = 0;
  while( l_tag>0 ) {
    if( siglen+1 >= uint(DIM(sig)) ) break;
    sig[siglen] = coro2_get();
    if( sig[siglen]==0 ) break;
    if( (sig[siglen]<0x20) || (sig[siglen]>0x7E) ) break;
    siglen++;
  }
  sig[siglen] = 0;

  pf( "APP%u: %s\n", tag_id-M_APP0, siglen ? (char*)sig : (char*)"(no signature)" );

  // ICC_PROFILE\0 <seq> <count> <profile chunk>
  if( (tag_id==M_APP2) && (siglen==11) && (sig[0]=='I') && (sig[1]=='C') && (sig[2]=='C') ) {
    if( l_tag<2 ) { pjpg_warn( "truncated ICC_PROFILE header" ); CORO_EXIT; }
    icc_seq = coro2_get();
    icc_num = coro2_get();
    pf( "  ICC profile chunk %u of %u, %u bytes\n", icc_seq, icc_num, l_tag );
    if( (icc_seq<1) || (icc_num<1) || (icc_seq>icc_num) ) pjpg_warn( "ICC chunk numbering out of range" );
  }

  coro_done: CORO_END;
}
