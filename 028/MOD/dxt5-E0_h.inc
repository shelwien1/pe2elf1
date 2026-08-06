
pmask( E0_ee0, "11111111" );

pmask( E0_eh, "111111" );
pmask( E0_ehv, "1" );

pmask( E0_ejw, "111111" );
pmask( E0_ejn, "111111" );

pmask( E0_em, "000000" );

pmask( E0_en, "000001" );

pmask( E0_ep, "111111" );

pmask( E0_ep0, "111111" );
pmask( E0_ep0n, "111110" );
pmask( E0_epv, "1" );

pmask( E0_eq, "111111" );

pmask( E0_ew, "111111" );

pmask( E0_fa, "111111" );
pdesc( E0_fl, 1, "1111111110010100" );

pmask( E0_ee1, "11111111" );

pmask( E0_fh, "000000" );
pmask( E0_fhv, "0" );

pmask( E0_fja, "111111" );
pmask( E0_fjw, "111111" );

pmask( E0_fm, "000001" );

pmask( E0_fp, "111110" );

pmask( E0_ep1, "111110" );
pmask( E0_ep1n, "111111" );
pmask( E0_epw, "1" );

pmask( E0_fq, "111111" );

pmask( E0_fw, "011111" );

pmask( E0_x1r, "11111" );

pmask( E0_x2r, "00111" );
pmask( E0_x2g, "111111" );




static const int E0_E0E_Volume = 1* E0_ee0.Size; // 1
static const int E0_E0H_Volume = 1* E0_eh.Size* E0_ehv.Size; // 1
static const int E0_E0J_Volume = 1* E0_ejw.Size* E0_ejn.Size; // 1
static const int E0_E0M_Volume = 1* E0_em.Size; // 1
static const int E0_E0N_Volume = 1* E0_en.Size; // 1
static const int E0_E0P_Volume = 1* E0_ep.Size; // 1
static const int E0_E0PP_Volume = 1* E0_ep0.Size* E0_ep0n.Size* E0_epv.Size; // 1
static const int E0_E0Q_Volume = 1* E0_eq.Size; // 1
static const int E0_E0W_Volume = 1* E0_ew.Size; // 1
static const int E0_E1A_Volume = 1* E0_fa.Size* E0_fl.Size; // 12
static const int E0_E1E_Volume = 1* E0_ee1.Size; // 1
static const int E0_E1H_Volume = 1* E0_fh.Size* E0_fhv.Size; // 1
static const int E0_E1J_Volume = 1* E0_fja.Size* E0_fjw.Size; // 1
static const int E0_E1M_Volume = 1* E0_fm.Size; // 1
static const int E0_E1P_Volume = 1* E0_fp.Size; // 1
static const int E0_E1PP_Volume = 1* E0_ep1.Size* E0_ep1n.Size* E0_epw.Size; // 1
static const int E0_E1Q_Volume = 1* E0_fq.Size; // 1
static const int E0_E1W_Volume = 1* E0_fw.Size; // 1
static const int E0_EX1_Volume = 1* E0_x1r.Size; // 1
static const int E0_EX2_Volume = 1* E0_x2r.Size* E0_x2g.Size; // 1

struct E0_T { 
#undef USE_NEW
#define USE_NEW 1


  Ctr* E0_tE09;
  Ctr* E0_tE19;
  Ctr* E0_tE29;
  Ctr* E0_tE39;
  Ctr* E0_tE49;
  Ctr* E0_tE59;
  Ctr* E0_tE00;
  Ctr* E0_tE01;
  Ctr* E0_tE02;
  Ctr* E0_tE03;
  Ctr* E0_tE04;
  Ctr* E0_tE05;
  Ctr* E0_tE06;
  Ctr* E0_tE07;
  Ctr* E0_tE08;
  int* E0_mxE0w;
  Ctr* E0_tE10;
  Ctr* E0_tE11;
  Ctr* E0_tE12;
  Ctr* E0_tE13;
  Ctr* E0_tE14;
  Ctr* E0_tE15;
  Ctr* E0_tE16;
  Ctr* E0_tE17;
  Ctr* E0_tE18;
  int* E0_mxE1w;
  Ctr* E0_tE20;
  Ctr* E0_tE21;
  Ctr* E0_tE22;
  Ctr* E0_tE23;
  Ctr* E0_tE24;
  Ctr* E0_tE25;
  Ctr* E0_tE26;
  Ctr* E0_tE27;
  Ctr* E0_tE28;
  int* E0_mxE2w;
  Ctr* E0_tE30;
  Ctr* E0_tE31;
  Ctr* E0_tE32;
  Ctr* E0_tE33;
  Ctr* E0_tE34;
  Ctr* E0_tE35;
  Ctr* E0_tE36;
  Ctr* E0_tE37;
  Ctr* E0_tE38;
  int* E0_mxE3w;
  Ctr* E0_tE40;
  Ctr* E0_tE41;
  Ctr* E0_tE42;
  Ctr* E0_tE43;
  Ctr* E0_tE44;
  Ctr* E0_tE45;
  Ctr* E0_tE46;
  Ctr* E0_tE47;
  Ctr* E0_tE48;
  int* E0_mxE4w;
  Ctr* E0_tE50;
  Ctr* E0_tE51;
  Ctr* E0_tE52;
  Ctr* E0_tE53;
  Ctr* E0_tE54;
  Ctr* E0_tE55;
  Ctr* E0_tE56;
  Ctr* E0_tE57;
  Ctr* E0_tE58;
  int* E0_mxE5w;
  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long E0_Size = 0;
  void E0_Init( void ) {
    E0_Size = 0;
    E0_tE09 = new Ctr[ tbl_n(E0_E0E_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0E_Volume * 32);
    E0_tE19 = new Ctr[ tbl_n(E0_E0E_Volume * 64) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0E_Volume * 64);
    E0_tE29 = new Ctr[ tbl_n(E0_E0E_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0E_Volume * 32);
    E0_tE39 = new Ctr[ tbl_n(E0_E1E_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1E_Volume * 32);
    E0_tE49 = new Ctr[ tbl_n(E0_E1E_Volume * 64) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1E_Volume * 64);
    E0_tE59 = new Ctr[ tbl_n(E0_E1E_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1E_Volume * 32);
    E0_tE00 = new Ctr[ tbl_n(E0_E0W_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0W_Volume * 32);
    E0_tE01 = new Ctr[ tbl_n(E0_E0N_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0N_Volume * 32);
    E0_tE02 = new Ctr[ tbl_n(E0_E0P_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0P_Volume * 32);
    E0_tE03 = new Ctr[ tbl_n(1 * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(1 * 32);
    E0_tE04 = new Ctr[ tbl_n(E0_E0Q_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0Q_Volume * 32);
    E0_tE05 = new Ctr[ tbl_n(E0_E0J_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0J_Volume * 32);
    E0_tE06 = new Ctr[ tbl_n(E0_E0M_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0M_Volume * 32);
    E0_tE07 = new Ctr[ tbl_n(E0_E0H_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0H_Volume * 32);
    E0_tE08 = new Ctr[ tbl_n(E0_E0PP_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0PP_Volume * 32);
    E0_mxE0w = new int[ tbl_n(11 * 32) ];
    E0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(11 * 32);
    E0_tE10 = new Ctr[ tbl_n(E0_E0W_Volume * 64) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0W_Volume * 64);
    E0_tE11 = new Ctr[ tbl_n(E0_E0N_Volume * 64) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0N_Volume * 64);
    E0_tE12 = new Ctr[ tbl_n(E0_E0P_Volume * 64) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0P_Volume * 64);
    E0_tE13 = new Ctr[ tbl_n(E0_EX1_Volume * 64) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_EX1_Volume * 64);
    E0_tE14 = new Ctr[ tbl_n(E0_E0Q_Volume * 64) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0Q_Volume * 64);
    E0_tE15 = new Ctr[ tbl_n(E0_E0J_Volume * 64) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0J_Volume * 64);
    E0_tE16 = new Ctr[ tbl_n(E0_E0M_Volume * 64) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0M_Volume * 64);
    E0_tE17 = new Ctr[ tbl_n(E0_E0H_Volume * 64) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0H_Volume * 64);
    E0_tE18 = new Ctr[ tbl_n(E0_E0PP_Volume * 64) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0PP_Volume * 64);
    E0_mxE1w = new int[ tbl_n(11 * 64) ];
    E0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(11 * 64);
    E0_tE20 = new Ctr[ tbl_n(E0_E0W_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0W_Volume * 32);
    E0_tE21 = new Ctr[ tbl_n(E0_E0N_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0N_Volume * 32);
    E0_tE22 = new Ctr[ tbl_n(E0_E0P_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0P_Volume * 32);
    E0_tE23 = new Ctr[ tbl_n(E0_EX2_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_EX2_Volume * 32);
    E0_tE24 = new Ctr[ tbl_n(E0_E0Q_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0Q_Volume * 32);
    E0_tE25 = new Ctr[ tbl_n(E0_E0J_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0J_Volume * 32);
    E0_tE26 = new Ctr[ tbl_n(E0_E0M_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0M_Volume * 32);
    E0_tE27 = new Ctr[ tbl_n(E0_E0H_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0H_Volume * 32);
    E0_tE28 = new Ctr[ tbl_n(E0_E0PP_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E0PP_Volume * 32);
    E0_mxE2w = new int[ tbl_n(11 * 32) ];
    E0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(11 * 32);
    E0_tE30 = new Ctr[ tbl_n(E0_E1A_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1A_Volume * 32);
    E0_tE31 = new Ctr[ tbl_n(E0_E1W_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1W_Volume * 32);
    E0_tE32 = new Ctr[ tbl_n(E0_E1P_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1P_Volume * 32);
    E0_tE33 = new Ctr[ tbl_n(1 * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(1 * 32);
    E0_tE34 = new Ctr[ tbl_n(E0_E1Q_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1Q_Volume * 32);
    E0_tE35 = new Ctr[ tbl_n(E0_E1J_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1J_Volume * 32);
    E0_tE36 = new Ctr[ tbl_n(E0_E1M_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1M_Volume * 32);
    E0_tE37 = new Ctr[ tbl_n(E0_E1H_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1H_Volume * 32);
    E0_tE38 = new Ctr[ tbl_n(E0_E1PP_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1PP_Volume * 32);
    E0_mxE3w = new int[ tbl_n(11 * 32) ];
    E0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(11 * 32);
    E0_tE40 = new Ctr[ tbl_n(E0_E1A_Volume * 64) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1A_Volume * 64);
    E0_tE41 = new Ctr[ tbl_n(E0_E1W_Volume * 64) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1W_Volume * 64);
    E0_tE42 = new Ctr[ tbl_n(E0_E1P_Volume * 64) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1P_Volume * 64);
    E0_tE43 = new Ctr[ tbl_n(E0_EX1_Volume * 64) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_EX1_Volume * 64);
    E0_tE44 = new Ctr[ tbl_n(E0_E1Q_Volume * 64) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1Q_Volume * 64);
    E0_tE45 = new Ctr[ tbl_n(E0_E1J_Volume * 64) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1J_Volume * 64);
    E0_tE46 = new Ctr[ tbl_n(E0_E1M_Volume * 64) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1M_Volume * 64);
    E0_tE47 = new Ctr[ tbl_n(E0_E1H_Volume * 64) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1H_Volume * 64);
    E0_tE48 = new Ctr[ tbl_n(E0_E1PP_Volume * 64) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1PP_Volume * 64);
    E0_mxE4w = new int[ tbl_n(11 * 64) ];
    E0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(11 * 64);
    E0_tE50 = new Ctr[ tbl_n(E0_E1A_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1A_Volume * 32);
    E0_tE51 = new Ctr[ tbl_n(E0_E1W_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1W_Volume * 32);
    E0_tE52 = new Ctr[ tbl_n(E0_E1P_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1P_Volume * 32);
    E0_tE53 = new Ctr[ tbl_n(E0_EX2_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_EX2_Volume * 32);
    E0_tE54 = new Ctr[ tbl_n(E0_E1Q_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1Q_Volume * 32);
    E0_tE55 = new Ctr[ tbl_n(E0_E1J_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1J_Volume * 32);
    E0_tE56 = new Ctr[ tbl_n(E0_E1M_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1M_Volume * 32);
    E0_tE57 = new Ctr[ tbl_n(E0_E1H_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1H_Volume * 32);
    E0_tE58 = new Ctr[ tbl_n(E0_E1PP_Volume * 32) ];
    E0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(E0_E1PP_Volume * 32);
    E0_mxE5w = new int[ tbl_n(11 * 32) ];
    E0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(11 * 32);
  
  }
  void E0_Quit( void ) {
    delete[] E0_tE09;
    delete[] E0_tE19;
    delete[] E0_tE29;
    delete[] E0_tE39;
    delete[] E0_tE49;
    delete[] E0_tE59;
    delete[] E0_tE00;
    delete[] E0_tE01;
    delete[] E0_tE02;
    delete[] E0_tE03;
    delete[] E0_tE04;
    delete[] E0_tE05;
    delete[] E0_tE06;
    delete[] E0_tE07;
    delete[] E0_tE08;
    delete[] E0_mxE0w;
    delete[] E0_tE10;
    delete[] E0_tE11;
    delete[] E0_tE12;
    delete[] E0_tE13;
    delete[] E0_tE14;
    delete[] E0_tE15;
    delete[] E0_tE16;
    delete[] E0_tE17;
    delete[] E0_tE18;
    delete[] E0_mxE1w;
    delete[] E0_tE20;
    delete[] E0_tE21;
    delete[] E0_tE22;
    delete[] E0_tE23;
    delete[] E0_tE24;
    delete[] E0_tE25;
    delete[] E0_tE26;
    delete[] E0_tE27;
    delete[] E0_tE28;
    delete[] E0_mxE2w;
    delete[] E0_tE30;
    delete[] E0_tE31;
    delete[] E0_tE32;
    delete[] E0_tE33;
    delete[] E0_tE34;
    delete[] E0_tE35;
    delete[] E0_tE36;
    delete[] E0_tE37;
    delete[] E0_tE38;
    delete[] E0_mxE3w;
    delete[] E0_tE40;
    delete[] E0_tE41;
    delete[] E0_tE42;
    delete[] E0_tE43;
    delete[] E0_tE44;
    delete[] E0_tE45;
    delete[] E0_tE46;
    delete[] E0_tE47;
    delete[] E0_tE48;
    delete[] E0_mxE4w;
    delete[] E0_tE50;
    delete[] E0_tE51;
    delete[] E0_tE52;
    delete[] E0_tE53;
    delete[] E0_tE54;
    delete[] E0_tE55;
    delete[] E0_tE56;
    delete[] E0_tE57;
    delete[] E0_tE58;
    delete[] E0_mxE5w;
  }
};
