
static const byte H3_qpd_a_sq[13+1]={ 0,0,1,1,1,1,2,2,3,3,4,4,4,5 };

static const byte H3_qpd_b_dc[8+1]={ 0,0,1,1,2,3,3,4,4 };

static const byte H3_qpd_c_db[8+1]={ 0,1,2,3,4,4,5,6,6 };
static const byte H3_qpd_c_bc[13+1]={ 0,1,2,3,4,4,4,4,4,4,4,5,6,7 };


static const byte H3_qpd_e_cls[8+1]={ 0,0,0,1,2,2,2,3,3 };
static const byte H3_qpd_e_sq[13+1]={ 0,0,1,2,3,4,4,5,5,6,6,7,8,9 };


static const byte H3_qpd_h_dc[8+1]={ 0,1,2,2,3,3,4,5,5 };


static const byte H3_qpd_j_st[9+1]={ 0,0,1,2,2,3,3,3,3,3 };
static const byte H3_qpd_j_dc[8+1]={ 0,1,2,3,4,5,5,6,6 };
static const byte H3_qpd_j_db[8+1]={ 0,1,2,3,3,4,4,5,5 };

static const byte H3_qpd_k_kc[13+1]={ 0,1,2,2,3,4,5,6,7,8,9,10,10,11 };

static const byte H3_qpd_l_st[9+1]={ 0,0,1,2,3,3,3,3,3,3 };
static const byte H3_qpd_l_cl[13+1]={ 0,1,2,3,4,4,5,5,6,7,7,8,9,10 };







static const int H3_qpd_rA = (254+0) * (1);
static const int H3_qpd_rB = (226+0) * (1);
static const int H3_qpd_rC = (254+0) * (1);
static const int H3_qpd_rE = (255+0) * (1);
static const int H3_qpd_rH = (252+0) * (1);
static const int H3_qpd_rI = (68+0) * (1);
static const int H3_qpd_rJ = (255+0) * (1);
static const int H3_qpd_rK = (252+0) * (1);
static const int H3_qpd_rL = (255+0) * (1);
static const int H3_qpd_rD = (158+0) * (1);
static const int H3_qpd_rT = (29+0) * (1);
static const int H3_qpd_lr = (13+0) * (1);
static const int H3_qpd_lrm = (0+0) * (1);
static const int H3_qpd_mbm = (0+0) * (1);
static const int H3_qpd_mwA = (64+0) * (1);
static const int H3_qpd_mwB = (133+0) * (1);
static const int H3_qpd_mwC = (5+0) * (1);
static const int H3_qpd_mwE = (21+0) * (1);
static const int H3_qpd_mwH = (96+0) * (1);
static const int H3_qpd_mwI = (11+0) * (1);
static const int H3_qpd_mwJ = (55+0) * (1);
static const int H3_qpd_mwK = (23+0) * (1);
static const int H3_qpd_mwL = (32+0) * (1);
static const int H3_qpd_mwD = (16+0) * (1);
static const int H3_qpd_mwT = (31+0) * (1);
static const int H3_qpd_mt = (4+0) * (1);
static const int H3_qpd_mb = (64+0) * (1);
static const int H3_qpd_rG = (0+0) * (1);
static const int H3_qpd_mwG = (0+0) * (1);
static const int H3_qpd_hA = (24+0) * (1);
static const int H3_qpd_hB = (24+0) * (1);
static const int H3_qpd_hC = (24+0) * (1);
static const int H3_qpd_hE = (24+0) * (1);
static const int H3_qpd_hH = (24+0) * (1);
static const int H3_qpd_hI = (24+0) * (1);
static const int H3_qpd_hJ = (16+0) * (1);
static const int H3_qpd_hK = (24+0) * (1);
static const int H3_qpd_hL = (24+0) * (1);
static const int H3_qpd_hD = (24+0) * (1);
static const int H3_qpd_hS = (24+0) * (1);
static const int H3_qpd_hM = (24+0) * (1);
static const int H3_qpd_hT = (24+0) * (1);
static const int H3_qpd_hN = (24+0) * (1);


static const long long H3_qpd_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 3), 2), 6); // 108
static const long long H3_qpd_b_Volume = tc_vmul(1, 5); // 5
static const long long H3_qpd_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 1), 7), 8); // 168
static const long long H3_qpd_d_Volume = tc_vmul(1, 1); // 1
static const long long H3_qpd_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 4), 10); // 40
static const long long H3_qpd_g_Volume = tc_vmul(1, 1); // 1
static const long long H3_qpd_h_Volume = tc_vmul(tc_vmul(1, 6), 3); // 18
static const long long H3_qpd_i_Volume = tc_vmul(1, 2); // 2
static const long long H3_qpd_j_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 7), 6), 2); // 336
static const long long H3_qpd_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 12); // 24
static const long long H3_qpd_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 4), 11); // 88
static const long long H3_qpd_m_Volume = tc_vmul(1, 2); // 2
static const long long H3_qpd_n_Volume = tc_vmul(1, 1); // 1
static const long long H3_qpd_s_Volume = tc_vmul(1, 1); // 1
static const long long H3_qpd_t_Volume = tc_vmul(1, 1); // 1
static const long long H3_qpd_w_Volume = tc_vmul(1, 3); // 3

#define H3_qpd_USES(X) X(pc) X(cc) X(cls) X(sq) X(dc) X(lz) X(db) X(bc) X(st) X(lnz) X(kc) X(cl)

struct H3_qpd_T {
  cm_cnt2 H3_qpd_A[tc_rows(H3_qpd_a_Volume, H3_qpd_hA) * H_qpd_ND];
  cm_cnt2 H3_qpd_B[tc_rows(H3_qpd_b_Volume, H3_qpd_hB) * H_qpd_ND];
  cm_cnt2 H3_qpd_C[tc_rows(H3_qpd_c_Volume, H3_qpd_hC) * H_qpd_ND];
  cm_cnt2 H3_qpd_D[tc_rows(H3_qpd_d_Volume, H3_qpd_hD) * H_qpd_ND];
  cm_cnt2 H3_qpd_E[tc_rows(H3_qpd_e_Volume, H3_qpd_hE) * H_qpd_ND];
  cm_cnt2 H3_qpd_H[tc_rows(H3_qpd_h_Volume, H3_qpd_hH) * H_qpd_ND];
  cm_cnt2 H3_qpd_I[tc_rows(H3_qpd_i_Volume, H3_qpd_hI) * H_qpd_ND];
  cm_cnt2 H3_qpd_J[tc_rows(H3_qpd_j_Volume, H3_qpd_hJ) * H_qpd_ND];
  cm_cnt2 H3_qpd_K[tc_rows(H3_qpd_k_Volume, H3_qpd_hK) * H_qpd_ND];
  cm_cnt2 H3_qpd_L[tc_rows(H3_qpd_l_Volume, H3_qpd_hL) * H_qpd_ND];
  cm_cnt H3_qpd_MT[tc_rows(H3_qpd_t_Volume, H3_qpd_hT) * H_qpd_MND];
  i32 H3_qpd_W[tc_rows(H3_qpd_m_Volume, H3_qpd_hM) * H_qpd_ND * 24];
  u8 H3_qpd_WC[tc_rows(H3_qpd_m_Volume, H3_qpd_hM) * H_qpd_ND];
  i32 H3_qpd_WM[tc_rows(H3_qpd_n_Volume, H3_qpd_hN) * H_qpd_MND * 3];
  u8 H3_qpd_WMC[tc_rows(H3_qpd_n_Volume, H3_qpd_hN) * H_qpd_MND];
  i32 H3_qpd_W2[H3_qpd_w_Volume * H_qpd_ND * 24];
  u8 H3_qpd_W2C[H3_qpd_w_Volume * H_qpd_ND];
  i32 H3_qpd_W3[tc_rows(H3_qpd_s_Volume, H3_qpd_hS) * H_qpd_ND * 24];
  u8 H3_qpd_W3C[tc_rows(H3_qpd_s_Volume, H3_qpd_hS) * H_qpd_ND];
  i32 H3_qpd_WF[H_qpd_ND * 5 * 4];
  u8 H3_qpd_WFC[H_qpd_ND * 5];
  cm_cnt H3_qpd_G[H3_qpd_g_Volume * H_qpd_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H3_qpd_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_qpd_a_Volume, H3_qpd_hA) * H_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_qpd_b_Volume, H3_qpd_hB) * H_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_qpd_c_Volume, H3_qpd_hC) * H_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_qpd_d_Volume, H3_qpd_hD) * H_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_qpd_e_Volume, H3_qpd_hE) * H_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_qpd_h_Volume, H3_qpd_hH) * H_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_qpd_i_Volume, H3_qpd_hI) * H_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_qpd_j_Volume, H3_qpd_hJ) * H_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_qpd_k_Volume, H3_qpd_hK) * H_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_qpd_l_Volume, H3_qpd_hL) * H_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H3_qpd_t_Volume, H3_qpd_hT) * H_qpd_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_qpd_m_Volume, H3_qpd_hM) * H_qpd_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_qpd_m_Volume, H3_qpd_hM) * H_qpd_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_qpd_n_Volume, H3_qpd_hN) * H_qpd_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_qpd_n_Volume, H3_qpd_hN) * H_qpd_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H3_qpd_w_Volume * H_qpd_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H3_qpd_w_Volume * H_qpd_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_qpd_s_Volume, H3_qpd_hS) * H_qpd_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_qpd_s_Volume, H3_qpd_hS) * H_qpd_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_qpd_ND * 5 * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_qpd_ND * 5)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H3_qpd_g_Volume * H_qpd_NG);
  void H3_qpd_Init( void ) {}
  void H3_qpd_Quit( void ) {}
};
