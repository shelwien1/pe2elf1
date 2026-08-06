
pmask( C0_apl, "11" );
pmask( C0_apu, "11" );
pmask( C0_apa, "11" );
pdesc( C0_aps, 1, "1010100000000000" );
pmask( C0_apq, "11" );

pmask( C0_aqt, "1111" );
pmask( C0_aql, "11" );
pdesc( C0_aqm, 1, "1011111000000000000" );
pdesc( C0_aqg, 1, "00000" );

pmask( C0_c0m, "1" );

pmask( C0_c1l, "01" );
pmask( C0_c1u, "01" );
pmask( C0_c1w, "11" );
pdesc( C0_c1g, 1, "00010" );

pmask( C0_c2p, "11" );
pmask( C0_c2t, "0101" );

pmask( C0_c3w, "10" );
pmask( C0_c3l, "11" );

pmask( C0_c4n, "11" );
pmask( C0_c4u, "11" );

pmask( C0_c5a, "00" );
pmask( C0_c5b, "01" );

pmask( C0_c6c, "01" );
pmask( C0_c6l, "10" );
pmask( C0_c6v, "1" );

pmask( C0_c7c, "11" );
pmask( C0_c7l, "10" );
pmask( C0_c7v, "1" );

pmask( C0_c8l, "00" );
pmask( C0_c8u, "00" );
pmask( C0_c8a, "01" );

pmask( C0_c9l, "10" );
pmask( C0_c9u, "11" );

pmask( C0_cmp, "0" );
pmask( C0_cmd, "1" );
pmask( C0_cmn, "10" );
pdesc( C0_cms, 1, "0000101110101000001" );
pmask( C0_cmt, "0000" );
pdesc( C0_cmw, 1, "0" );
pdesc( C0_cmq, 1, "0" );
pmask( C0_cma, "01" );

pmask( C0_cbl, "11" );
pmask( C0_cbu, "00" );
pmask( C0_cba, "00" );
pmask( C0_cbb, "11" );

pmask( C0_ccl, "11" );
pmask( C0_ccu, "11" );
pmask( C0_cca, "11" );
pmask( C0_ccb, "11" );
pmask( C0_ccw, "11" );

pmask( C0_cdl, "11" );
pmask( C0_cdu, "11" );
pmask( C0_cda, "11" );

pmask( C0_cel, "00" );
pmask( C0_ceu, "10" );
pdesc( C0_cec, 1, "0000001110010010000" );
pdesc( C0_ced, 1, "0000001110000000000" );

pmask( C0_cfg, "111" );
pmask( C0_cfs, "11" );
pmask( C0_cfp, "11" );

pmask( C0_cgl, "00" );
pmask( C0_cgu, "00" );
pmask( C0_cga, "11" );

pmask( C0_cpp, "111" );
pmask( C0_cpl, "11" );
pmask( C0_cpu, "11" );

pmask( C0_cqp, "111" );
pdesc( C0_cqc, 1, "1111101111110101000" );
pmask( C0_cqv, "00" );

pmask( C0_crp, "111" );
pmask( C0_crs, "111" );
pmask( C0_crc, "011" );

pmask( C0_scl, "01" );
pmask( C0_scu, "01" );
pmask( C0_sca, "01" );
pmask( C0_scb, "00" );




static const int C0_APC_Volume = 1* C0_apl.Size* C0_apu.Size* C0_apa.Size* C0_aps.Size* C0_apq.Size; // 4
static const int C0_APC2_Volume = 1* C0_aqt.Size* C0_aql.Size* C0_aqm.Size* C0_aqg.Size; // 7
static const int C0_CI0_Volume = 1* C0_c0m.Size; // 1
static const int C0_CI1_Volume = 1* C0_c1l.Size* C0_c1u.Size* C0_c1w.Size* C0_c1g.Size; // 2
static const int C0_CI2_Volume = 1* C0_c2p.Size* C0_c2t.Size; // 1
static const int C0_CI3_Volume = 1* C0_c3w.Size* C0_c3l.Size; // 1
static const int C0_CI4_Volume = 1* C0_c4n.Size* C0_c4u.Size; // 1
static const int C0_CI5_Volume = 1* C0_c5a.Size* C0_c5b.Size; // 1
static const int C0_CI6_Volume = 1* C0_c6c.Size* C0_c6l.Size* C0_c6v.Size; // 1
static const int C0_CI7_Volume = 1* C0_c7c.Size* C0_c7l.Size* C0_c7v.Size; // 1
static const int C0_CI8_Volume = 1* C0_c8l.Size* C0_c8u.Size* C0_c8a.Size; // 1
static const int C0_CI9_Volume = 1* C0_c9l.Size* C0_c9u.Size; // 1
static const int C0_CIM_Volume = 1* C0_cmp.Size* C0_cmd.Size* C0_cmn.Size* C0_cms.Size* C0_cmt.Size* C0_cmw.Size* C0_cmq.Size* C0_cma.Size; // 8
static const int C0_CIb_Volume = 1* C0_cbl.Size* C0_cbu.Size* C0_cba.Size* C0_cbb.Size; // 1
static const int C0_CIc_Volume = 1* C0_ccl.Size* C0_ccu.Size* C0_cca.Size* C0_ccb.Size* C0_ccw.Size; // 1
static const int C0_CId_Volume = 1* C0_cdl.Size* C0_cdu.Size* C0_cda.Size; // 1
static const int C0_CIe_Volume = 1* C0_cel.Size* C0_ceu.Size* C0_cec.Size* C0_ced.Size; // 24
static const int C0_CIf_Volume = 1* C0_cfg.Size* C0_cfs.Size* C0_cfp.Size; // 1
static const int C0_CIg_Volume = 1* C0_cgl.Size* C0_cgu.Size* C0_cga.Size; // 1
static const int C0_CIp_Volume = 1* C0_cpp.Size* C0_cpl.Size* C0_cpu.Size; // 1
static const int C0_CIq_Volume = 1* C0_cqp.Size* C0_cqc.Size* C0_cqv.Size; // 14
static const int C0_CIr_Volume = 1* C0_crp.Size* C0_crs.Size* C0_crc.Size; // 1
static const int C0_SLC_Volume = 1* C0_scl.Size* C0_scu.Size* C0_sca.Size* C0_scb.Size; // 1

struct C0_T { 
#undef USE_NEW
#define USE_NEW 1


  Ctr* C0_tCI1;
  Ctr* C0_tCI2;
  Ctr* C0_tCI5;
  Ctr* C0_tCI6;
  Ctr* C0_tCI7;
  Ctr* C0_tCI9;
  Ctr* C0_tCI8;
  Ctr* C0_tCIb;
  Ctr* C0_tCIc;
  Ctr* C0_tCId;
  Ctr* C0_tCIe;
  Ctr* C0_tCIf;
  Ctr* C0_tCIg;
  Ctr* C0_tCIp;
  Ctr* C0_tCIq;
  Ctr* C0_tCIr;
  Ctr* C0_tCI3;
  Ctr* C0_tCI4;
  Ctr* C0_tCI0;
  CtrS* C0_sCI1;
  int* C0_mxCIw;
  u16* C0_apCIt;
  u16* C0_apCI2t;
  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long C0_Size = 0;
  void C0_Init( void ) {
    C0_Size = 0;
    C0_tCI1 = new Ctr[ tbl_n(C0_CI1_Volume * 4) ];
    C0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(C0_CI1_Volume * 4);
    C0_tCI2 = new Ctr[ tbl_n(C0_CI2_Volume * 4) ];
    C0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(C0_CI2_Volume * 4);
    C0_tCI5 = new Ctr[ tbl_n(C0_CI5_Volume * 4) ];
    C0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(C0_CI5_Volume * 4);
    C0_tCI6 = new Ctr[ tbl_n(C0_CI6_Volume * 4) ];
    C0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(C0_CI6_Volume * 4);
    C0_tCI7 = new Ctr[ tbl_n(C0_CI7_Volume * 4) ];
    C0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(C0_CI7_Volume * 4);
    C0_tCI9 = new Ctr[ tbl_n(C0_CI9_Volume * 4) ];
    C0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(C0_CI9_Volume * 4);
    C0_tCI8 = new Ctr[ tbl_n(C0_CI8_Volume * 4) ];
    C0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(C0_CI8_Volume * 4);
    C0_tCIb = new Ctr[ tbl_n(C0_CIb_Volume * 4) ];
    C0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(C0_CIb_Volume * 4);
    C0_tCIc = new Ctr[ tbl_n(C0_CIc_Volume * 4) ];
    C0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(C0_CIc_Volume * 4);
    C0_tCId = new Ctr[ tbl_n(C0_CId_Volume * 4) ];
    C0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(C0_CId_Volume * 4);
    C0_tCIe = new Ctr[ tbl_n(C0_CIe_Volume * 4) ];
    C0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(C0_CIe_Volume * 4);
    C0_tCIf = new Ctr[ tbl_n(C0_CIf_Volume * 4) ];
    C0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(C0_CIf_Volume * 4);
    C0_tCIg = new Ctr[ tbl_n(C0_CIg_Volume * 4) ];
    C0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(C0_CIg_Volume * 4);
    C0_tCIp = new Ctr[ tbl_n(C0_CIp_Volume * 4) ];
    C0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(C0_CIp_Volume * 4);
    C0_tCIq = new Ctr[ tbl_n(C0_CIq_Volume * 4) ];
    C0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(C0_CIq_Volume * 4);
    C0_tCIr = new Ctr[ tbl_n(C0_CIr_Volume * 4) ];
    C0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(C0_CIr_Volume * 4);
    C0_tCI3 = new Ctr[ tbl_n(C0_CI3_Volume * 4) ];
    C0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(C0_CI3_Volume * 4);
    C0_tCI4 = new Ctr[ tbl_n(C0_CI4_Volume * 4) ];
    C0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(C0_CI4_Volume * 4);
    C0_tCI0 = new Ctr[ tbl_n(C0_CI0_Volume * 4) ];
    C0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(C0_CI0_Volume * 4);
    C0_sCI1 = new CtrS[ tbl_n(C0_SLC_Volume * 4) ];
    C0_Size += (unsigned long long)sizeof(CtrS) * (unsigned long long)tbl_n(C0_SLC_Volume * 4);
    C0_mxCIw = new int[ tbl_n(22 * C0_CIM_Volume * 4) ];
    C0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(22 * C0_CIM_Volume * 4);
    C0_apCIt = new u16[ tbl_n(C0_APC_Volume * 4 * (PAPN(G0_AP_N_CI) + 1)) ];
    C0_Size += (unsigned long long)sizeof(u16) * (unsigned long long)tbl_n(C0_APC_Volume * 4 * (PAPN(G0_AP_N_CI) + 1));
    C0_apCI2t = new u16[ tbl_n(C0_APC2_Volume * 4 * (PAPN(G0_AP_N_CI2) + 1)) ];
    C0_Size += (unsigned long long)sizeof(u16) * (unsigned long long)tbl_n(C0_APC2_Volume * 4 * (PAPN(G0_AP_N_CI2) + 1));
  
  }
  void C0_Quit( void ) {
    delete[] C0_tCI1;
    delete[] C0_tCI2;
    delete[] C0_tCI5;
    delete[] C0_tCI6;
    delete[] C0_tCI7;
    delete[] C0_tCI9;
    delete[] C0_tCI8;
    delete[] C0_tCIb;
    delete[] C0_tCIc;
    delete[] C0_tCId;
    delete[] C0_tCIe;
    delete[] C0_tCIf;
    delete[] C0_tCIg;
    delete[] C0_tCIp;
    delete[] C0_tCIq;
    delete[] C0_tCIr;
    delete[] C0_tCI3;
    delete[] C0_tCI4;
    delete[] C0_tCI0;
    delete[] C0_sCI1;
    delete[] C0_mxCIw;
    delete[] C0_apCIt;
    delete[] C0_apCI2t;
  }
};
