
pmask( X0_tp, "11111001" );

pmask( X0_xa, "1111" );

pmask( X0_xb, "1111" );

pmask( X0_xc, "1111" );

pmask( X0_xp, "1111" );
pdesc( X0_xr, 1, "1111" );
pmask( X0_xv, "1" );

pmask( X0_x5p, "1110" );
pmask( X0_x5l, "0000" );
pmask( X0_x5u, "0000" );




static const int X0_TL_Volume = 1* X0_tp.Size; // 1
static const int X0_X1_Volume = 1* X0_xa.Size; // 1
static const int X0_X2_Volume = 1* X0_xb.Size; // 1
static const int X0_X3_Volume = 1* X0_xc.Size; // 1
static const int X0_X4_Volume = 1* X0_xp.Size* X0_xr.Size* X0_xv.Size; // 5
static const int X0_X5_Volume = 1* X0_x5p.Size* X0_x5l.Size* X0_x5u.Size; // 1

struct X0_T { 
#undef USE_NEW
#define USE_NEW 1


  Ctr* X0_tX0;
  Ctr* X0_tX1;
  Ctr* X0_tX2;
  Ctr* X0_tX3;
  Ctr* X0_tX4;
  Ctr* X0_tX5;
  int* X0_mxXw;
  Ctr* X0_tTail;
  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long X0_Size = 0;
  void X0_Init( void ) {
    X0_Size = 0;
    X0_tX0 = new Ctr[ tbl_n(X0_X1_Volume * 16) ];
    X0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(X0_X1_Volume * 16);
    X0_tX1 = new Ctr[ tbl_n(X0_X2_Volume * 16) ];
    X0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(X0_X2_Volume * 16);
    X0_tX2 = new Ctr[ tbl_n(X0_X3_Volume * 16) ];
    X0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(X0_X3_Volume * 16);
    X0_tX3 = new Ctr[ tbl_n(16) ];
    X0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(16);
    X0_tX4 = new Ctr[ tbl_n(X0_X4_Volume * 16) ];
    X0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(X0_X4_Volume * 16);
    X0_tX5 = new Ctr[ tbl_n(X0_X5_Volume * 16) ];
    X0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(X0_X5_Volume * 16);
    X0_mxXw = new int[ tbl_n(7 * 16) ];
    X0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(7 * 16);
    X0_tTail = new Ctr[ tbl_n(X0_TL_Volume * 256) ];
    X0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(X0_TL_Volume * 256);
  
  }
  void X0_Quit( void ) {
    delete[] X0_tX0;
    delete[] X0_tX1;
    delete[] X0_tX2;
    delete[] X0_tX3;
    delete[] X0_tX4;
    delete[] X0_tX5;
    delete[] X0_mxXw;
    delete[] X0_tTail;
  }
};
