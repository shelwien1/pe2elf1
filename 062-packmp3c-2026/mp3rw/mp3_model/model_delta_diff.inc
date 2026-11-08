// Channel 1's values coded as their differences from channel 0's, in
// dual-mono mode: within 16 granules of one whose channel 1 was a copy of
// channel 0 or coded so itself (model_delta.inc: dm_mode), where the
// encoder finds the channels differing a little (DiffWorth(): a flag). A
// mono recording in two channels has stretches where the channels differ
// so: there, a fifth of the lines, nearly all by one. Where they differ
// more, the model codes channel 1 better with channel 0's values as its
// contexts, as it does in stereo.
//
// Each line below channel 1's count: whether it differs, mixed (cm_code)
// from two predictions and four counters, by channel 0's magnitude there
// (am), whether the last two lines differed (pp), whether this line did in
// the last granule (pg) and the band; then where it does, its sign (by
// channel 0's sign there and the last difference's) and its magnitude (in
// unary to 8, then 15 bits).

enum { DF_AM = 8, DF_PP = 4, DF_PG = 3, DF_BAND = 9 };

Node2ii dfA[DF_AM*DF_PP];         // by am, pp
Node2ii dfB[DF_PG*DF_BAND];       // by pg, the band
cm_cnt2 dfC0[DF_AM*DF_BAND];      // am, the band
cm_cnt2 dfC1[DF_PP*DF_PG];        // pp, pg
cm_cnt2 dfC2[DF_AM*DF_PG];        // am, pg
cm_cnt2 dfC3[576/8];              // the line, by 8
int     dfW[DF_AM*XNIN];          // the mixer's weight sets, by am
byte    dfWN[DF_AM];
Node2ii dfS[3*3];                 // the sign: channel 0's there, the last difference's
Node2ii dfM[8*4];                 // |difference| > t, t 1..8: by t, min(|channel 0's|, 3)
byte    dfPrev[576];              // a line's last granule: 0 none, 1 the same, 2 different
Node2ii dfFlag[2];                // the granule coded as differences, by the last one's
int     dm_mode;                  // the last granule copied or coded as differences
int     df_last;

void Init_Diff( void ) {
  int i;
  Init_Node2ii( dfA );
  Init_Node2ii( dfB );
  for( i=0; i<DF_AM*DF_BAND; i++ ) dfC0[i].Init();
  for( i=0; i<DF_PP*DF_PG; i++ ) dfC1[i].Init();
  for( i=0; i<DF_AM*DF_PG; i++ ) dfC2[i].Init();
  for( i=0; i<576/8; i++ ) dfC3[i].Init();
  for( i=0; i<DF_AM*XNIN; i++ ) dfW[i] = (i%XNIN==0 || i%XNIN==10) ? 32768 : 0;
  memset( dfWN, 0, sizeof(dfWN) );
  Init_Node2ii( dfS );
  Init_Node2ii( dfM );
  memset( dfPrev, 0, sizeof(dfPrev) );
  Init_Node2ii( dfFlag );
  dm_mode = 0;
  df_last = 0;
}

// the encoder's choice: channel 1 of granule gr as differences, where at
// most half the lines below the counts differ, by 1.1 on the mean at most
// (else the stereo of a stream that had a copy -- a silence, a moment in
// mono -- went through this coder, which codes large differences badly)
int DiffWorth( int gr ) {
  const short *a = MC[gr][0], *b = MC[gr][1];
  int j, k, n = 0, s = 0;
  for( k=576; k>0 && !a[k-1] && !b[k-1]; k-- );
  for( j=0; j<k; j++ ) if( a[j]!=b[j] ) { n++; s += l_abs(b[j]-a[j]); }
  return 2*n <= k && 10*s <= 11*n;
}

// a granule whose channel 1 was copied: its lines the same, for pg
void DiffSame( int gr ) {
  for( int j=0; j<cc0[gr][1]; j++ ) dfPrev[j] = 1;
}

// Channel 1 of granule gr coded as its differences from channel 0's, below
// its count (the counts coded before); the model's histories of each place
// (RangeDelta inst) kept as coding the values would have kept them
void DiffMDCT( int gr, int inst ) {
  RangeCoder& rc = *this;
  const short* a = MC[gr][0];
  short* b = MC[gr][1];
  const int k = cc0[gr][1];
  int pp = 0, ps = 0, j;
  for( j=0; j<576; j++ ) {
    int v = 0;
    if( j<k ) {
      const int m0 = l_abs(a[j]);
      const int am = m0>15 ? 7 : m0>9 ? 6 : m0>5 ? 5 : m0>3 ? 4 : m0;
      const int pg = dfPrev[j], band = j>>6;
      int d = rc.rc_ProcMode ? 0 : b[j]-a[j];
      int nz = (d!=0);
      Node2ii& A = dfA[am*DF_PP+pp];
      Node2ii& B = dfB[pg*DF_BAND+band];
      cm_code( rc, nz, A.P, B.P, dfC0[am*DF_BAND+band], dfC1[pp*DF_PG+pg], dfC2[am*DF_PG+pg], dfC3[j>>3],
               &dfW[am*XNIN], dfWN[am], 1012, 32, 4, 6, 130 );
      A.Update( nz, TTable[24], 16 );
      B.Update( nz, TTable[24], 16 );
      if( nz ) {
        int s = d<0;
        dfS[(a[j]>0 ? 2 : a[j]<0)*3+ps].Process( rc, s, TTable[24], 16 );
        int ad = l_abs(d), t;
        for( t=1; ; t++ ) {
          if( t>8 ) {
            int x = ad-9;
            rc.NProcess( x, 15, 0, 1<<15 ); // (|d| 9 and up)
            ad = x+9;
            break;
          }
          int more = (ad>t);
          dfM[(t-1)*4+(m0>3 ? 3 : m0)].Process( rc, more, TTable[24], 16 );
          if( !more ) { ad = t; break; }
        }
        d = s ? -ad : ad;
        ps = 1+s;
      }
      v = a[j]+d;
      if( l_abs(v)>8206 ) BadData(); // (a broken stream: no MP3 value is larger)
      pp = (pp*2+nz)&3;
      dfPrev[j] = 1+nz;
    }
    b[j] = v;
    m_rDelta[inst].CopyMDCT( j, v, j<k );
  }
}
