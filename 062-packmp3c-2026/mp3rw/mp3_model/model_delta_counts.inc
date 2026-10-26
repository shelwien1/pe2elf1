
int cc0[2][2]; // k  = current count0s
int cc1[2][2]; // c1 = current count1s

Node2ii          bvflag;
Number1ii<9>     bvdump;
Node2ii          xbigval0;
Node2ii          xbigval1;

void Init_Counts() {

  C_Init();
  C_InitData();

  Init_Node2ii( bvflag );
  Init_Number1ii( bvdump );
  Init_Node2ii( xbigval0 );
  Init_Node2ii( xbigval1 );

  memset( &cc0, 0, sizeof(cc0) );
  memset( &cc1, 0, sizeof(cc1) );
}

void ProcessCounts( int i, int gr, int ch ) {

  int k, c1, flag;
  gr_info& gi = si.gi[gr][ch];
  short (&MDCT)[576] = MC[gr][ch];
  RangeCoder& rc = *this;

  for( k=576; k>0 && ( MDCT[k-1]==0 ); k-- );
  for( c1=k; c1>0 && ( l_abs(MDCT[c1-1])<=1 ); c1-- );

  ProcessCounts1( k, c1, i, gr, ch );

  int bigval = gi.big_values;
  int u = (c1+1)/2;

  int v = __min((k+1)/2,u+2);
  flag = ( bigval<u || bigval>v );
#ifndef DISABLE_bvflag
  bvflag.Process( rc, flag, M_bvflag_wr, M_bvflag_mw ); //branch = 0;
#endif
  if( flag ) {
#ifndef DISABLE_bvdump
    bvdump.Process( rc, bigval, -1, 289, M_bvdump_wr, M_bvdump_mw );
#endif
  } else {
    flag = (bigval==u);
    if( v==u ) flag=1; else {
#ifndef DISABLE_xbigval0
      xbigval0.Process( rc, flag, M_xbigval0_wr, M_xbigval0_mw );
#endif
    }
    if( flag ) bigval=u; else {
      flag = (bigval==u+1);
      if( v==u+1) flag=1; else {
#ifndef DISABLE_xbigval1
        xbigval1.Process( rc, flag, M_xbigval1_wr, M_xbigval1_mw );
#endif
      }
      if( flag ) bigval=u+1; else bigval=u+2;
    }
  }

  gi.big_values = bigval;

  cc0[gr][ch] = k;
  cc1[gr][ch] = c1;
}

