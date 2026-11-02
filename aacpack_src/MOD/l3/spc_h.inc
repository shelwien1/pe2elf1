
static const byte H3_spc_a_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };
static const byte H3_spc_a_a2[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H3_spc_b_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };
static const byte H3_spc_b_sb[50+1]={ 0,1,2,3,4,5,6,7,8,9,10,10,11,12,13,14,15,15,16,16,17,18,18,19,20,21,21,22,23,24,25,26,27,28,28,29,29,29,29,30,30,31,32,32,33,34,34,34,35,35,35 };

static const byte H3_spc_c_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };
static const byte H3_spc_c_pn[12+1]={ 0,0,1,2,3,4,5,6,7,8,9,9,9 };
static const byte H3_spc_c_pf[5+1]={ 0,1,2,3,4,5 };

static const byte H3_spc_d_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };
static const byte H3_spc_d_pf[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte H3_spc_d_a1[7+1]={ 0,0,0,0,1,2,3,4 };
static const byte H3_spc_d_dg[69+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69 };

static const byte H3_spc_e_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };
static const byte H3_spc_e_av[31+1]={ 0,1,1,1,2,3,4,5,5,6,7,8,8,9,10,10,10,10,10,10,11,12,13,14,15,16,17,18,18,19,20,21 };


static const byte H3_spc_h_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };
static const byte H3_spc_h_a3[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };
static const byte H3_spc_h_nz[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H3_spc_h_bp[3+1]={ 0,1,2,3 };
static const byte H3_spc_h_pl[4+1]={ 0,1,2,3,4 };
static const byte H3_spc_h_zr[13+1]={ 0,1,1,2,3,4,4,5,5,6,7,8,9,9 };

static const byte H3_spc_m_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };
static const byte H3_spc_m_av[28+1]={ 0,0,1,1,1,1,1,1,2,2,2,2,3,3,3,4,5,5,5,5,6,6,6,6,6,6,6,6,6 };



static const int H3_spc_rA = (255+0) * (1);
static const int H3_spc_rB = (254+0) * (1);
static const int H3_spc_rC = (255+0) * (1);
static const int H3_spc_rD = (255+0) * (1);
static const int H3_spc_rE = (255+0) * (1);
static const int H3_spc_rH = (255+0) * (1);
static const int H3_spc_lr = (13+0) * (1);
static const int H3_spc_mwA = (31+0) * (1);
static const int H3_spc_mwB = (3+0) * (1);
static const int H3_spc_mwC = (128+0) * (1);
static const int H3_spc_mwD = (16+0) * (1);
static const int H3_spc_mwE = (44+0) * (1);
static const int H3_spc_mwH = (97+0) * (1);
static const int H3_spc_mb = (32+0) * (1);
static const int H3_spc_rT = (0+0) * (1);
static const int H3_spc_mwT = (0+0) * (1);
static const int H3_spc_mt = (0+0) * (1);
static const int H3_spc_rG = (0+0) * (1);
static const int H3_spc_mwG = (0+0) * (1);
static const int H3_spc_hA = (24+0) * (1);
static const int H3_spc_hB = (24+0) * (1);
static const int H3_spc_hC = (24+0) * (1);
static const int H3_spc_hD = (24+0) * (1);
static const int H3_spc_hE = (24+0) * (1);
static const int H3_spc_hH = (24+0) * (1);
static const int H3_spc_hM = (24+0) * (1);
static const int H3_spc_hT = (24+0) * (1);


static const long long H3_spc_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 12), 2), 8); // 192
static const long long H3_spc_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 12), 36), 2); // 864
static const long long H3_spc_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 12), 10), 6); // 720
static const long long H3_spc_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 12), 16), 5), 70); // 67200
static const long long H3_spc_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 12), 22), 3); // 792
static const long long H3_spc_g_Volume = 1; // 1
static const long long H3_spc_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 12), 11), 8), 4), 5), 10); // 211200
static const long long H3_spc_m_Volume = tc_vmul(tc_vmul(tc_vmul(1, 12), 2), 7); // 168
static const long long H3_spc_t_Volume = 1; // 1

#define H3_spc_USES(X) X(cb) X(a1) X(a2) X(sb) X(ws) X(pn) X(pf) X(dg) X(av) X(a3) X(nz) X(bp) X(pl) X(zr)

struct H3_spc_T {
  cm_cnt2 H3_spc_A[tc_rows(H3_spc_a_Volume, H3_spc_hA) * H_spc_ND];
  cm_cnt2 H3_spc_B[tc_rows(H3_spc_b_Volume, H3_spc_hB) * H_spc_ND];
  cm_cnt2 H3_spc_C[tc_rows(H3_spc_c_Volume, H3_spc_hC) * H_spc_ND];
  cm_cnt2 H3_spc_D[tc_rows(H3_spc_d_Volume, H3_spc_hD) * H_spc_ND];
  cm_cnt2 H3_spc_E[tc_rows(H3_spc_e_Volume, H3_spc_hE) * H_spc_ND];
  cm_cnt2 H3_spc_H[tc_rows(H3_spc_h_Volume, H3_spc_hH) * H_spc_ND];
  cm_cnt H3_spc_MT[tc_rows(H3_spc_t_Volume, H3_spc_hT) * H_spc_MND];
  i32 H3_spc_W[tc_rows(H3_spc_m_Volume, H3_spc_hM) * H_spc_ND * tc_nin(6)];
  u8 H3_spc_WC[tc_rows(H3_spc_m_Volume, H3_spc_hM) * H_spc_ND];
  cm_cnt H3_spc_G[H3_spc_g_Volume * H_spc_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H3_spc_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_spc_a_Volume, H3_spc_hA) * H_spc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_spc_b_Volume, H3_spc_hB) * H_spc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_spc_c_Volume, H3_spc_hC) * H_spc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_spc_d_Volume, H3_spc_hD) * H_spc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_spc_e_Volume, H3_spc_hE) * H_spc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_spc_h_Volume, H3_spc_hH) * H_spc_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H3_spc_t_Volume, H3_spc_hT) * H_spc_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_spc_m_Volume, H3_spc_hM) * H_spc_ND * tc_nin(6))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_spc_m_Volume, H3_spc_hM) * H_spc_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H3_spc_g_Volume * H_spc_NG);
  void H3_spc_Init( void ) {}
  void H3_spc_Quit( void ) {}
};
