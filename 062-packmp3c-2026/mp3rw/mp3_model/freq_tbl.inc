
const int   hSCALE = SCALE/2;
const int   mSCALE = SCALE-1;
//#define mMW(Mw) int(SCALE*Mw*0.5)

word Init_TTable( word* Tbl, int N, int z ) {
  int i;
  const uint SL = 16;
  const uint ST = 4;      // T logscale
  const uint SS = 1U<<SL; // wr scale
  uint wr = SS - SS/N;    // 0:31 fixedpoint
  uint T  = 0;            // ?:2  fixedpoint
  Tbl[0] = z-1;
  for( i=1; i<z; i++ ) {
    T = ((T*wr)>>SL)+(1<<ST);
    Tbl[i] = (SCALE<<ST)/T;
  }
  return z-1;
}

/*
word Init_TTable( word* Tbl, int N, int z ) {
  int i;
  const uint SL = 16;
  const uint ST = 4;      // T logscale
  const uint SS = 1U<<SL; // wr scale
  uint wr = SS - SS/N;    // 0:31 fixedpoint
  uint T  = 0;            // ?:2  fixedpoint
  Tbl[0] = z-1;
  Tbl[-1] = N;
  for( i=1; i<z; i++ ) {
    T = ((T*wr)>>SL)+(1<<ST);
    Tbl[i] = ((SCALE<<ST)/T) | (i<z-1?SCALE:0);
  }
  return z-1;
}
*/

/*
word Read_TTable( word* Tbl, int N, int z ) {
  char fname[256];
  sprintf( fname, "ttable_%i.dmp", N );
  FILE* f = fopen( fname, "rb" );
  fread( Tbl, 1,2*z, f );
  fclose(f);
  return 0;
}
*/

//#define DefTbl( NN, ZZ ) word TTable_##NN[ZZ]={ Init_TTable( TTable_##NN, NN, ZZ ) }
//#define RidTbl( NN, ZZ ) word TTable_##NN[ZZ]; int unusedTR_##NN=Read_TTable( TTable_##NN, NN, ZZ );

//#define DefTbl( NN, XX, ZZ ) word TTable_##NN[ZZ]; int unusedTT_##NN=Init_TTable( TTable_##NN, XX, ZZ );

#define DefTbl( NN, XX, ZZ ) \
  word* TTable_##NN = new word[ZZ];\
  int unusedTT_##NN = Init_TTable( TTable_##NN, XX, ZZ );

DefTbl( 999, F_wr_00, F_wq_00 )
DefTbl( 800, F_wr_01, F_wq_01 )
DefTbl( 700, F_wr_02, F_wq_02 )
DefTbl( 600, F_wr_03, F_wq_03 )
DefTbl( 550, F_wr_04, F_wq_04 )
DefTbl( 512, F_wr_05, F_wq_05 )
DefTbl( 450, F_wr_06, F_wq_06 )
DefTbl( 400, F_wr_07, F_wq_07 )
DefTbl( 350, F_wr_08, F_wq_08 )
DefTbl( 300, F_wr_09, F_wq_09 )
DefTbl( 250, F_wr_0A, F_wq_0A )
DefTbl( 200, F_wr_0B, F_wq_0B )
DefTbl( 175, F_wr_0C, F_wq_0C )
DefTbl( 150, F_wr_0D, F_wq_0D )
DefTbl( 120, F_wr_0E, F_wq_0E )
DefTbl( 100, F_wr_0F, F_wq_0F )
DefTbl( 80, F_wr_10, F_wq_10 )
DefTbl( 64, F_wr_11, F_wq_11 )
DefTbl( 45, F_wr_12, F_wq_12 )
DefTbl( 40, F_wr_13, F_wq_13 )
DefTbl( 34, F_wr_14, F_wq_14 )
DefTbl( 28, F_wr_15, F_wq_15 )
DefTbl( 24, F_wr_16, F_wq_16 )
DefTbl( 20, F_wr_17, F_wq_17 )
DefTbl( 15, F_wr_18, F_wq_18 )
DefTbl( 10, F_wr_19, F_wq_19 )
DefTbl( 8, F_wr_1A, F_wq_1A )
DefTbl( 6, F_wr_1B, F_wq_1B )
DefTbl( 5, F_wr_1C, F_wq_1C )
DefTbl( 4, F_wr_1D, F_wq_1D )
DefTbl( 3, F_wr_1E, F_wq_1E )
DefTbl( 2, F_wr_1F, F_wq_1F )

const word* TTable[] = {
TTable_999,TTable_800,TTable_700,TTable_600,
TTable_550,TTable_512,TTable_450,TTable_400,
TTable_350,TTable_300,TTable_250,TTable_200,
TTable_175,TTable_150,TTable_120,TTable_100,
TTable_80 ,TTable_64 ,TTable_45 ,TTable_40 ,
TTable_34 ,TTable_28 ,TTable_24 ,TTable_20 ,
TTable_15 ,TTable_10 ,TTable_8  ,TTable_6  ,
TTable_5  ,TTable_4  ,TTable_3  ,TTable_2  };

