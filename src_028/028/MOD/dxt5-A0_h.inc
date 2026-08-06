
pmask( A0_e0, "11111111" );

pmask( A0_h0, "11111111" );
pmask( A0_hv0, "1" );

pmask( A0_l0, "11111111" );

pmask( A0_m0, "00000011" );

pmask( A0_n0, "11111111" );

pmask( A0_p0, "11111100" );

pmask( A0_q0x, "11111111" );
pmask( A0_q0n, "11000000" );

pmask( A0_w0, "11111111" );

pmask( A0_a1, "11111111" );

pmask( A0_e1, "11111111" );

pmask( A0_h1, "11111111" );
pmask( A0_hv1, "1" );

pmask( A0_l1, "11111110" );

pmask( A0_m1, "00000011" );

pmask( A0_p1, "11111111" );

pmask( A0_q1x, "11100000" );
pmask( A0_q1n, "11111111" );

pmask( A0_w1, "01111111" );


pdesc( A0_L0K_, -256, "0001011100" );
static const int A0_L0K = A0_L0K_.value * (1);
pdesc( A0_L0Wa0_, -256, "0100000000" );
static const int A0_L0Wa0 = A0_L0Wa0_.value * (1);
pdesc( A0_L0Na0_, -256, "0100000011" );
static const int A0_L0Na0 = A0_L0Na0_.value * (1);
pdesc( A0_L0Xa0_, -256, "0010101011" );
static const int A0_L0Xa0 = A0_L0Xa0_.value * (1);
pdesc( A0_L0Wa1_, -256, "0011101110" );
static const int A0_L0Wa1 = A0_L0Wa1_.value * (1);
pdesc( A0_L0Na1_, -256, "0100000100" );
static const int A0_L0Na1 = A0_L0Na1_.value * (1);
pdesc( A0_L0Xa1_, -256, "0100000110" );
static const int A0_L0Xa1 = A0_L0Xa1_.value * (1);
pdesc( A0_L0Emn_, -256, "1000000010" );
static const int A0_L0Emn = A0_L0Emn_.value * (1);
pdesc( A0_L0Emx_, -256, "0111001000" );
static const int A0_L0Emx = A0_L0Emx_.value * (1);
pdesc( A0_L1K_, -256, "0010001111" );
static const int A0_L1K = A0_L1K_.value * (1);
pdesc( A0_L1A0_, -256, "0110010111" );
static const int A0_L1A0 = A0_L1A0_.value * (1);
pdesc( A0_L1Wa1_, -256, "0100001011" );
static const int A0_L1Wa1 = A0_L1Wa1_.value * (1);
pdesc( A0_L1Na1_, -256, "0100010111" );
static const int A0_L1Na1 = A0_L1Na1_.value * (1);
pdesc( A0_L1Xa1_, -256, "0011101110" );
static const int A0_L1Xa1 = A0_L1Xa1_.value * (1);
pdesc( A0_L1Wa0_, -256, "0011101111" );
static const int A0_L1Wa0 = A0_L1Wa0_.value * (1);
pdesc( A0_L1Na0_, -256, "0100000000" );
static const int A0_L1Na0 = A0_L1Na0_.value * (1);
pdesc( A0_L1Emn_, -256, "0111010101" );
static const int A0_L1Emn = A0_L1Emn_.value * (1);
pdesc( A0_L1Emx_, -256, "0100011000" );
static const int A0_L1Emx = A0_L1Emx_.value * (1);


static const int A0_AE0E_Volume = 1* A0_e0.Size; // 1
static const int A0_AE0H_Volume = 1* A0_h0.Size* A0_hv0.Size; // 1
static const int A0_AE0L_Volume = 1* A0_l0.Size; // 1
static const int A0_AE0M_Volume = 1* A0_m0.Size; // 1
static const int A0_AE0N_Volume = 1* A0_n0.Size; // 1
static const int A0_AE0P_Volume = 1* A0_p0.Size; // 1
static const int A0_AE0P2_Volume = 1* A0_q0x.Size* A0_q0n.Size; // 1
static const int A0_AE0W_Volume = 1* A0_w0.Size; // 1
static const int A0_AE1A_Volume = 1* A0_a1.Size; // 1
static const int A0_AE1E_Volume = 1* A0_e1.Size; // 1
static const int A0_AE1H_Volume = 1* A0_h1.Size* A0_hv1.Size; // 1
static const int A0_AE1L_Volume = 1* A0_l1.Size; // 1
static const int A0_AE1M_Volume = 1* A0_m1.Size; // 1
static const int A0_AE1P_Volume = 1* A0_p1.Size; // 1
static const int A0_AE1P2_Volume = 1* A0_q1x.Size* A0_q1n.Size; // 1
static const int A0_AE1W_Volume = 1* A0_w1.Size; // 1

struct A0_T { 
#undef USE_NEW
#define USE_NEW 1


  Ctr* A0_tA0W;
  Ctr* A0_tA0N;
  Ctr* A0_tA0P;
  Ctr* A0_tA0o;
  Ctr* A0_tA0M;
  Ctr* A0_tA0E;
  Ctr* A0_tA0L;
  Ctr* A0_tA1L;
  Ctr* A0_tA1E;
  Ctr* A0_tA0H;
  Ctr* A0_tA0P2;
  Ctr* A0_tA1W;
  Ctr* A0_tA1A;
  Ctr* A0_tA1P;
  Ctr* A0_tA1o;
  Ctr* A0_tA1M;
  Ctr* A0_tA1H;
  Ctr* A0_tA1P2;
  int* A0_mxA0w;
  int* A0_mxA1w;
  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long A0_Size = 0;
  void A0_Init( void ) {
    A0_Size = 0;
    A0_tA0W = new Ctr[ tbl_n(A0_AE0W_Volume * 256) ];
    A0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(A0_AE0W_Volume * 256);
    A0_tA0N = new Ctr[ tbl_n(A0_AE0N_Volume * 256) ];
    A0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(A0_AE0N_Volume * 256);
    A0_tA0P = new Ctr[ tbl_n(A0_AE0P_Volume * 256) ];
    A0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(A0_AE0P_Volume * 256);
    A0_tA0o = new Ctr[ tbl_n(256) ];
    A0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(256);
    A0_tA0M = new Ctr[ tbl_n(A0_AE0M_Volume * 256) ];
    A0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(A0_AE0M_Volume * 256);
    A0_tA0E = new Ctr[ tbl_n(A0_AE0E_Volume * 256) ];
    A0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(A0_AE0E_Volume * 256);
    A0_tA0L = new Ctr[ tbl_n(A0_AE0L_Volume * 256) ];
    A0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(A0_AE0L_Volume * 256);
    A0_tA1L = new Ctr[ tbl_n(A0_AE1L_Volume * 256) ];
    A0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(A0_AE1L_Volume * 256);
    A0_tA1E = new Ctr[ tbl_n(A0_AE1E_Volume * 256) ];
    A0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(A0_AE1E_Volume * 256);
    A0_tA0H = new Ctr[ tbl_n(A0_AE0H_Volume * 256) ];
    A0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(A0_AE0H_Volume * 256);
    A0_tA0P2 = new Ctr[ tbl_n(A0_AE0P2_Volume * 256) ];
    A0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(A0_AE0P2_Volume * 256);
    A0_tA1W = new Ctr[ tbl_n(A0_AE1W_Volume * 256) ];
    A0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(A0_AE1W_Volume * 256);
    A0_tA1A = new Ctr[ tbl_n(A0_AE1A_Volume * 256) ];
    A0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(A0_AE1A_Volume * 256);
    A0_tA1P = new Ctr[ tbl_n(A0_AE1P_Volume * 256) ];
    A0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(A0_AE1P_Volume * 256);
    A0_tA1o = new Ctr[ tbl_n(256) ];
    A0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(256);
    A0_tA1M = new Ctr[ tbl_n(A0_AE1M_Volume * 256) ];
    A0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(A0_AE1M_Volume * 256);
    A0_tA1H = new Ctr[ tbl_n(A0_AE1H_Volume * 256) ];
    A0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(A0_AE1H_Volume * 256);
    A0_tA1P2 = new Ctr[ tbl_n(A0_AE1P2_Volume * 256) ];
    A0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(A0_AE1P2_Volume * 256);
    A0_mxA0w = new int[ tbl_n(16 * 256) ];
    A0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(16 * 256);
    A0_mxA1w = new int[ tbl_n(10 * 256) ];
    A0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(10 * 256);
  
  }
  void A0_Quit( void ) {
    delete[] A0_tA0W;
    delete[] A0_tA0N;
    delete[] A0_tA0P;
    delete[] A0_tA0o;
    delete[] A0_tA0M;
    delete[] A0_tA0E;
    delete[] A0_tA0L;
    delete[] A0_tA1L;
    delete[] A0_tA1E;
    delete[] A0_tA0H;
    delete[] A0_tA0P2;
    delete[] A0_tA1W;
    delete[] A0_tA1A;
    delete[] A0_tA1P;
    delete[] A0_tA1o;
    delete[] A0_tA1M;
    delete[] A0_tA1H;
    delete[] A0_tA1P2;
    delete[] A0_mxA0w;
    delete[] A0_mxA1w;
  }
};
