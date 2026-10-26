
int ProcSideInfo( void ) {

  int ch,gr,nch,ngr,i, error=0;
  ngr = h.ID;
  nch = (h.mode!=3);

  procbits( si.backup_mdb, (ngr?9:8) );

  if( fstframe() ) 
    si.main_data_begin=0;
  else
    si.main_data_begin=si.backup_mdb;

  procbits( si.private_bits,    (ngr? (nch?3:5) : (nch?2:1)) );

  if( ngr ) 
    for( ch=0; ch<=nch; ch++ ) for( i=0; i<4; i++ ) 
      procbits( si.scfsi[ch][i], 1 );

  si.p23sum = 0;
  if( ProcMode==1 ) si.p23cleared = 0; // data is still ok

  for( gr=0; gr<=ngr; gr++ ) {
    for( ch=0; ch<=nch; ch++ ) {

      gr_info& gi = si.gi[gr][ch];

      procbits( si.p23cleared?gi.backup_p23:gi.part2_3_length, 12 );
      si.p23sum += gi.part2_3_length;
      procbits( gi.big_values,      9 );
if( gi.big_values>576/2 ) error=1;
      procbits( gi.global_gain,     8 );
      procbits( gi.scalefac_compress, (ngr?4:9) );
      procbits( gi.window_switching_flag, 1 );

      if( gi.window_switching_flag ) {

        procbits( gi.block_type,       2 );
        procbits( gi.mixed_block_flag, 1 );
        procbits( gi.table_select[0],  5 );
        procbits( gi.table_select[1],  5 );
                  gi.table_select[2]=0;  

        for( i=0; i<3; i++ )
         procbits( gi.subblock_gain[i],3 );

        // by mpglib
        if( h.ID==1 ) {
          gi.region0_count = 7;
        } else {     
          if( gi.block_type==2 ) {
            gi.region0_count = 5;
          } else {
            gi.region0_count = 7;
          }
        }
        gi.region1_count = 22;

      } else {

        gi.block_type=0;
        gi.mixed_block_flag=0;

        procbits( gi.table_select[0],  5 );
        procbits( gi.table_select[1],  5 );
        procbits( gi.table_select[2],  5 );
        procbits( gi.region0_count,    4 );
        procbits( gi.region1_count,    3 );
if( gi.region0_count+gi.region1_count+2>22 ) error=1;

      }

      for( i=0; i<3; i++ ) if( gi.table_select[i]==4 || gi.table_select[i]==14 ) {
        error=1;
      }

      if( ngr )
       procbits( gi.preflag, 1 );

      procbits( gi.scalefac_scale, 1 );
      procbits( gi.count1table_select, 1 );

    }
  }

  return error;
}
