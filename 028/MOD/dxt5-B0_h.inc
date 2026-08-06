
pmask( B0_i0z, "1" );
pmask( B0_i0m, "0" );
pdesc( B0_i0d, 1, "0111110100110011111111011100001101110100100010000001001110011100" );

pmask( B0_i1l, "111" );
pmask( B0_i1u, "111" );
pmask( B0_i1m, "1" );
pdesc( B0_i1d, 1, "0111111111111111111111111100100110100011000000001000000001100001" );

pmask( B0_i2p, "101" );
pmask( B0_i2t, "1111" );

pmask( B0_i3w, "111" );
pmask( B0_i3l, "111" );

pmask( B0_i4n, "000" );
pmask( B0_i4u, "111" );

pmask( B0_i5c, "000" );
pmask( B0_i5l, "110" );
pmask( B0_i5v, "0" );

pmask( B0_i6c, "000" );
pmask( B0_i6l, "010" );
pmask( B0_i6v, "0" );

pmask( B0_i8l, "010" );
pmask( B0_i8u, "001" );
pmask( B0_i8q, "1" );

pmask( B0_i9l, "111" );
pmask( B0_i9u, "111" );

pmask( B0_imp, "0" );
pmask( B0_imm, "1" );
pdesc( B0_imd, 1, "0111111111101100001000000000000000000000000000000000000000000000" );
pdesc( B0_ims, 1, "0000000000" );
pmask( B0_imt, "0011" );
pdesc( B0_imc, 1, "000" );

pmask( B0_ibl, "111" );
pmask( B0_ibu, "111" );
pmask( B0_ibq, "111" );
pmask( B0_ibr, "111" );

pmask( B0_iel, "000" );
pmask( B0_ieu, "100" );
pdesc( B0_iec, 1, "00111111" );
pdesc( B0_ied, 1, "11110000" );

pmask( B0_ifg, "1111" );
pmask( B0_ifs, "111" );
pmask( B0_ifp, "111" );

pmask( B0_ipp, "0111" );
pmask( B0_ipl, "111" );
pmask( B0_ipu, "111" );

pmask( B0_iqp, "1111" );
pdesc( B0_iqc, 1, "10101000" );
pmask( B0_iqi, "0111" );

pmask( B0_aap, "0" );
pmask( B0_aal, "111" );
pmask( B0_aam, "1" );
pdesc( B0_aas, 1, "1110101010" );

pmask( B0_a2l, "111" );
pmask( B0_a2u, "111" );
pmask( B0_a2w, "000" );

pmask( B0_sll, "000" );
pmask( B0_slu, "000" );
pmask( B0_slm, "1" );
pdesc( B0_sld, 1, "0111111000000000000000000000000000000000000100011001001111001001" );




static const int B0_AI0_Volume = 1* B0_i0z.Size* B0_i0m.Size* B0_i0d.Size; // 35
static const int B0_AI1_Volume = 1* B0_i1l.Size* B0_i1u.Size* B0_i1m.Size* B0_i1d.Size; // 36
static const int B0_AI2_Volume = 1* B0_i2p.Size* B0_i2t.Size; // 1
static const int B0_AI3_Volume = 1* B0_i3w.Size* B0_i3l.Size; // 1
static const int B0_AI4_Volume = 1* B0_i4n.Size* B0_i4u.Size; // 1
static const int B0_AI5_Volume = 1* B0_i5c.Size* B0_i5l.Size* B0_i5v.Size; // 1
static const int B0_AI6_Volume = 1* B0_i6c.Size* B0_i6l.Size* B0_i6v.Size; // 1
static const int B0_AI8_Volume = 1* B0_i8l.Size* B0_i8u.Size* B0_i8q.Size; // 1
static const int B0_AI9_Volume = 1* B0_i9l.Size* B0_i9u.Size; // 1
static const int B0_AIM_Volume = 1* B0_imp.Size* B0_imm.Size* B0_imd.Size* B0_ims.Size* B0_imt.Size* B0_imc.Size; // 14
static const int B0_AIb_Volume = 1* B0_ibl.Size* B0_ibu.Size* B0_ibq.Size* B0_ibr.Size; // 1
static const int B0_AIe_Volume = 1* B0_iel.Size* B0_ieu.Size* B0_iec.Size* B0_ied.Size; // 35
static const int B0_AIf_Volume = 1* B0_ifg.Size* B0_ifs.Size* B0_ifp.Size; // 1
static const int B0_AIp_Volume = 1* B0_ipp.Size* B0_ipl.Size* B0_ipu.Size; // 1
static const int B0_AIq_Volume = 1* B0_iqp.Size* B0_iqc.Size* B0_iqi.Size; // 4
static const int B0_APA_Volume = 1* B0_aap.Size* B0_aal.Size* B0_aam.Size* B0_aas.Size; // 7
static const int B0_APA2_Volume = 1* B0_a2l.Size* B0_a2u.Size* B0_a2w.Size; // 1
static const int B0_SLA_Volume = 1* B0_sll.Size* B0_slu.Size* B0_slm.Size* B0_sld.Size; // 17

struct B0_T { 
#undef USE_NEW
#define USE_NEW 1


  Ctr* B0_tAI1;
  Ctr* B0_tAI2;
  Ctr* B0_tAI3;
  Ctr* B0_tAI4;
  Ctr* B0_tAI0;
  Ctr* B0_tAI5;
  Ctr* B0_tAI6;
  Ctr* B0_tAI9;
  Ctr* B0_tAI8;
  Ctr* B0_tAIb;
  Ctr* B0_tAIe;
  Ctr* B0_tAIf;
  Ctr* B0_tAIp;
  Ctr* B0_tAIq;
  CtrS* B0_sAI1;
  int* B0_mxAIw;
  u16* B0_apAIt;
  u16* B0_apAI2t;
  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long B0_Size = 0;
  void B0_Init( void ) {
    B0_Size = 0;
    B0_tAI1 = new Ctr[ tbl_n(B0_AI1_Volume * 8) ];
    B0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(B0_AI1_Volume * 8);
    B0_tAI2 = new Ctr[ tbl_n(B0_AI2_Volume * 8) ];
    B0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(B0_AI2_Volume * 8);
    B0_tAI3 = new Ctr[ tbl_n(B0_AI3_Volume * 8) ];
    B0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(B0_AI3_Volume * 8);
    B0_tAI4 = new Ctr[ tbl_n(B0_AI4_Volume * 8) ];
    B0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(B0_AI4_Volume * 8);
    B0_tAI0 = new Ctr[ tbl_n(B0_AI0_Volume * 8) ];
    B0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(B0_AI0_Volume * 8);
    B0_tAI5 = new Ctr[ tbl_n(B0_AI5_Volume * 8) ];
    B0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(B0_AI5_Volume * 8);
    B0_tAI6 = new Ctr[ tbl_n(B0_AI6_Volume * 8) ];
    B0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(B0_AI6_Volume * 8);
    B0_tAI9 = new Ctr[ tbl_n(B0_AI9_Volume * 8) ];
    B0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(B0_AI9_Volume * 8);
    B0_tAI8 = new Ctr[ tbl_n(B0_AI8_Volume * 8) ];
    B0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(B0_AI8_Volume * 8);
    B0_tAIb = new Ctr[ tbl_n(B0_AIb_Volume * 8) ];
    B0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(B0_AIb_Volume * 8);
    B0_tAIe = new Ctr[ tbl_n(B0_AIe_Volume * 8) ];
    B0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(B0_AIe_Volume * 8);
    B0_tAIf = new Ctr[ tbl_n(B0_AIf_Volume * 8) ];
    B0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(B0_AIf_Volume * 8);
    B0_tAIp = new Ctr[ tbl_n(B0_AIp_Volume * 8) ];
    B0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(B0_AIp_Volume * 8);
    B0_tAIq = new Ctr[ tbl_n(B0_AIq_Volume * 8) ];
    B0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(B0_AIq_Volume * 8);
    B0_sAI1 = new CtrS[ tbl_n(B0_SLA_Volume * 8) ];
    B0_Size += (unsigned long long)sizeof(CtrS) * (unsigned long long)tbl_n(B0_SLA_Volume * 8);
    B0_mxAIw = new int[ tbl_n(17 * B0_AIM_Volume * 8) ];
    B0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(17 * B0_AIM_Volume * 8);
    B0_apAIt = new u16[ tbl_n(B0_APA_Volume * 8 * (PAPN(G0_AP_N_AI) + 1)) ];
    B0_Size += (unsigned long long)sizeof(u16) * (unsigned long long)tbl_n(B0_APA_Volume * 8 * (PAPN(G0_AP_N_AI) + 1));
    B0_apAI2t = new u16[ tbl_n(B0_APA2_Volume * 8 * (PAPN(G0_AP_N_AI2) + 1)) ];
    B0_Size += (unsigned long long)sizeof(u16) * (unsigned long long)tbl_n(B0_APA2_Volume * 8 * (PAPN(G0_AP_N_AI2) + 1));
  
  }
  void B0_Quit( void ) {
    delete[] B0_tAI1;
    delete[] B0_tAI2;
    delete[] B0_tAI3;
    delete[] B0_tAI4;
    delete[] B0_tAI0;
    delete[] B0_tAI5;
    delete[] B0_tAI6;
    delete[] B0_tAI9;
    delete[] B0_tAI8;
    delete[] B0_tAIb;
    delete[] B0_tAIe;
    delete[] B0_tAIf;
    delete[] B0_tAIp;
    delete[] B0_tAIq;
    delete[] B0_sAI1;
    delete[] B0_mxAIw;
    delete[] B0_apAIt;
    delete[] B0_apAI2t;
  }
};
