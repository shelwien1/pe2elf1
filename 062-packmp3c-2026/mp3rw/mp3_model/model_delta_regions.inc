
// things for encoding of region0, region1 & table_select[]
Number1ii<5>    tblsel[32][3];

void Init_Regions() {
  Init_Number1ii( tblsel );
}

void ProcessRegions( int gr, int ch ) {

  int u, i, j, d, bigval;
  int region1Start, region2Start;
  int sfreq = h.frequency + ((h.ID==0)+(h.IDex==0))*3;
  gr_info& gi = si.gi[gr][ch];
  short (&MDCT)[576] = MC[gr][ch];
  RangeCoder& rc = *this;

  if( gi.window_switching_flag ) {   
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
#ifndef DISABLE_rg0cnt
    Process_rgxcnt( gi.region0_count, gi.region1_count, ch );
#endif
  }

  region1Start = bandInfo[sfreq][gi.region0_count+1];
  region2Start = bandInfo[sfreq][__min(22,gi.region0_count+gi.region1_count+2)];

  bigval = gi.big_values*2;
  int lim[] = { 0, __min(region1Start,bigval), __min(region2Start,bigval), bigval };
  for( j=0; j<3-(gi.window_switching_flag); j++ ) {
    d = CalcTableSelect( MDCT, lim[j], lim[j+1] );
#ifndef DISABLE_tblsel
    tblsel[d][j].Process( rc, gi.table_select[j], -1,32, M_tblsel_wr, M_tblsel_mw );
#endif
  }

}
