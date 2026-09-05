
//uint i;
//uint n;
uint ci;
uint cc;

void pa_SOS( void ) { CORO_START;

  n = coro2_get(); if( n==-1 ) return;

  printf( "Start Of Scan: %d components; l_tag=%i\n", n, l_tag );

  comps_in_scan = n;

  for( i=0; i<n; i++ ) {
    cc = coro2_get();

    for( ci=0; ci<num_components; ci++ ) {
      if( comp_info[ci].component_id==cc ) { cur_comp_info[i]=&comp_info[ci]; break; }
    }

    comp_info[ci].dc_tbl_no = coro2_getH();//(c >> 4) & 15;
    comp_info[ci].ac_tbl_no = coro2_getL();//(c     ) & 15;

    printf( "  Component %d: dc=%02X ac=%02X\n", cc, comp_info[ci].dc_tbl_no, comp_info[ci].ac_tbl_no );
  }

  Ss = coro2_get();
  Se = coro2_get();
  Ah = coro2_getH();//(c >> 4) & 15;
  Al = coro2_getL();//(c     ) & 15;

  printf( "  Ss=%d, Se=%d, Ah=%d, Al=%d\n", Ss, Se, Ah, Al );

  next_restart_num = 0;
  input_scan_number++;

  f_SOS=1; // correctly parsed

  CORO_END;
}


