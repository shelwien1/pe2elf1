









static const int P0_P1_Volume = 1* (1<<8)* (1<<4); // 4096
static const int P0_P2_Volume = 1* (1<<8)* (1<<4); // 4096
static const int P0_P3_Volume = 1* (1<<8)* (1<<4); // 4096
static const int P0_P4_Volume = 1* (1<<1)* (1<<4); // 32
static const int P0_P5_Volume = 1* (1<<4)* (1<<4); // 256
static const int P0_P6_Volume = 1* (1<<6)* (1<<6)* (1<<3); // 32768

struct P0_T { 
#undef USE_NEW
#define USE_NEW 1


  Ctr* P0_tP0;
  Ctr* P0_tP1;
  Ctr* P0_tP2;
  Ctr* P0_tP3;
  Ctr* P0_tP4;
  Ctr* P0_tP5;
  int* P0_mxPw;
  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long P0_Size = 0;
  void P0_Init( void ) {
    P0_Size = 0;
    P0_tP0 = new Ctr[ tbl_n(P0_P1_Volume * 256) ];
    P0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(P0_P1_Volume * 256);
    P0_tP1 = new Ctr[ tbl_n(P0_P2_Volume * 256) ];
    P0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(P0_P2_Volume * 256);
    P0_tP2 = new Ctr[ tbl_n(P0_P3_Volume * 256) ];
    P0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(P0_P3_Volume * 256);
    P0_tP3 = new Ctr[ tbl_n(P0_P4_Volume * 256) ];
    P0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(P0_P4_Volume * 256);
    P0_tP4 = new Ctr[ tbl_n(P0_P5_Volume * 256) ];
    P0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(P0_P5_Volume * 256);
    P0_tP5 = new Ctr[ tbl_n(P0_P6_Volume * 256) ];
    P0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(P0_P6_Volume * 256);
    P0_mxPw = new int[ tbl_n(7 * 256) ];
    P0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(7 * 256);
  
  }
  void P0_Quit( void ) {
    delete[] P0_tP0;
    delete[] P0_tP1;
    delete[] P0_tP2;
    delete[] P0_tP3;
    delete[] P0_tP4;
    delete[] P0_tP5;
    delete[] P0_mxPw;
  }
};
