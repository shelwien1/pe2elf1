
// Arithmetic conditioning tables (T.81 B.2.4.3).  Pairs of (Tc/Tb, Cs) until
// the segment runs out.  pjpg records them but does not decode arithmetic data.
uint dac_tc, dac_tb, dac_cs;

void pa_DAC( void ) { CORO_START;

  while( l_tag>=2 ) {

    dac_tc = coro2_getH();
    dac_tb = coro2_getL();
    dac_cs = coro2_get();

    pf( "Define Arithmetic Conditioning: class=%u table=%u Cs=%u\n", dac_tc, dac_tb, dac_cs );

    if( dac_tc>1 )  { pjpg_err( E_BADDAC, "conditioning table class > 1" ); CORO_EXIT; }
    if( dac_tb>=4 ) { pjpg_err( E_BADDAC, "conditioning table index >= 4" ); CORO_EXIT; }

    if( dac_tc==0 ) {
      // DC: Cs is a packed (U<<4)|L pair with L <= U.
      if( (dac_cs&15) > (dac_cs>>4) ) pjpg_warn( "DC conditioning L > U" );
      arith_dc_L[dac_tb] = dac_cs&15;
      arith_dc_U[dac_tb] = dac_cs>>4;
    } else {
      // AC: Cs is Kx, 1..63.
      if( (dac_cs<1) || (dac_cs>63) ) pjpg_warn( "AC conditioning Kx outside 1..63" );
      arith_ac_K[dac_tb] = dac_cs;
    }
  }

  if( l_tag>0 ) pjpg_warn( "odd trailing byte in DAC segment" );

  coro_done: CORO_END;
}
