
//uint i;
//uint n;
uint ci;
uint cc;
uint f_found;

void pa_SOS( void ) { CORO_START;

  // Ns(1) + 2 bytes per component + Ss(1) + Se(1) + AhAl(1) = 4 minimum.
  if( l_tag<4 ) { pjpg_err( E_BADLEN, "SOS segment shorter than 4 bytes" ); CORO_EXIT; }

  n = coro2_get();

  pf( "Start Of Scan: %d components; l_tag=%i\n", n, l_tag );

  // THE fix for the cur_comp_info[4] overflow.  Ns is a whole byte from the
  // file but the scan component table holds exactly 4 entries, which is also
  // the limit T.81 imposes.  Without this, Ns=22 walked far enough past the
  // array to overwrite pjpg0::tag_id and the next byte segfaulted.
  if( (n<1) || (n>uint(DIM(cur_comp_info))) ) {
    pjpg_err( E_BADSOS, "scan declares 0 or more than 4 components" ); CORO_EXIT;
  }
  if( l_tag < 2*n+3 ) { pjpg_err( E_BADSOS, "scan header shorter than its component count" ); CORO_EXIT; }
  if( saw_SOF==0 )    { pjpg_err( E_BADSOS, "scan before any frame header" ); CORO_EXIT; }

  comps_in_scan = n;

  for( i=0; i<n; i++ ) {
    cc = coro2_get();

    // Resolve the component id against the frame.  The old code let ci fall out
    // of the loop equal to num_components and then wrote comp_info[ci], a slot
    // outside the declared frame.
    f_found = 0;
    for( ci=0; ci<uint(num_components); ci++ ) {
      if( uint(comp_info[ci].component_id)==cc ) { cur_comp_info[i]=&comp_info[ci]; f_found=1; break; }
    }
    if( f_found==0 ) {
      pjpg_err( E_BADSOS, "scan references a component the frame does not define" ); CORO_EXIT;
    }

    comp_info[ci].dc_tbl_no = coro2_getH();
    comp_info[ci].ac_tbl_no = coro2_getL();

    pf( "  Component %d: dc=%02X ac=%02X\n", cc, comp_info[ci].dc_tbl_no, comp_info[ci].ac_tbl_no );
  }

  Ss = coro2_get();
  Se = coro2_get();
  Ah = coro2_getH();
  Al = coro2_getL();

  pf( "  Ss=%d, Se=%d, Ah=%d, Al=%d\n", Ss, Se, Ah, Al );

  if( progressive_mode ) {
    if( (Ss>Se) || (Se>63) ) pjpg_warn( "progressive scan spectral range outside 0..63" );
  } else if( !sof_lossless ) {
    if( (Ss!=0) || (Se!=63) ) pjpg_warn( "sequential scan is not a full 0..63 spectral selection" );
  }

  next_restart_num = 0;
  input_scan_number++;

  f_SOS=1; // correctly parsed -> entropy-coded data follows

  coro_done: CORO_END;
}
