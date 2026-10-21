
static const byte H9_mvg_a_ts[9+1]={ 0,0,0,1,1,2,3,3,4,5 };
static const byte H9_mvg_a_mag[7+1]={ 0,0,1,2,2,2,2,3 };

static const byte H9_mvg_b_zs[8+1]={ 0,0,0,1,2,3,4,5,6 };
static const byte H9_mvg_b_mag[7+1]={ 0,0,0,0,1,2,2,3 };

static const byte H9_mvg_c_ts[9+1]={ 0,0,0,1,1,2,3,4,4,5 };
static const byte H9_mvg_c_zs[8+1]={ 0,0,0,1,2,3,4,5,5 };
static const byte H9_mvg_c_as[9+1]={ 0,0,0,1,2,3,4,5,6,7 };

static const byte H9_mvg_d_ds[9+1]={ 0,0,0,1,2,3,4,5,6,7 };

static const byte H9_mvg_e_ns[6+1]={ 0,0,1,2,3,4,4 };

static const byte H9_mvg_f_ts[9+1]={ 0,0,0,1,2,3,4,5,6,7 };


static const byte H9_mvg_h_ts[9+1]={ 0,0,0,1,2,3,4,5,6,6 };
static const byte H9_mvg_h_tq[12+1]={ 0,0,1,2,3,4,5,5,5,6,6,7,8 };
static const byte H9_mvg_h_es[3+1]={ 0,1,2,3 };

static const byte H9_mvg_i_ts[9+1]={ 0,0,0,1,1,1,2,2,2,3 };
static const byte H9_mvg_i_ds[9+1]={ 0,0,0,1,1,1,2,2,3,3 };

static const byte H9_mvg_j_ox[3+1]={ 0,1,2,3 };
static const byte H9_mvg_j_ns[6+1]={ 0,0,1,2,3,4,4 };

static const byte H9_mvg_k_ts[9+1]={ 0,0,0,1,2,3,4,4,5,6 };

static const byte H9_mvg_l_zs[8+1]={ 0,0,0,1,2,3,4,5,6 };
static const byte H9_mvg_l_ds[9+1]={ 0,0,0,1,1,2,3,3,4,5 };



static const byte H9_mvg_s_ts[9+1]={ 0,0,0,0,0,0,1,2,3,3 };
static const byte H9_mvg_s_mag[7+1]={ 0,0,1,1,2,2,3,3 };


static const byte H9_mvg_w_zs[8+1]={ 0,0,0,1,2,3,4,5,6 };


static const int H9_mvg_rA = (237+0) * (1);
static const int H9_mvg_rB = (0+0) * (1);
static const int H9_mvg_rC = (254+0) * (1);
static const int H9_mvg_rE = (255+0) * (1);
static const int H9_mvg_rH = (255+0) * (1);
static const int H9_mvg_rI = (88+0) * (1);
static const int H9_mvg_rJ = (129+0) * (1);
static const int H9_mvg_rK = (251+0) * (1);
static const int H9_mvg_rL = (254+0) * (1);
static const int H9_mvg_rD = (253+0) * (1);
static const int H9_mvg_rT = (0+0) * (1);
static const int H9_mvg_rG = (0+0) * (1);
static const int H9_mvg_rS1 = (5+0) * (1);
static const int H9_mvg_rS2 = (7+0) * (1);
static const int H9_mvg_lr = (12+0) * (1);
static const int H9_mvg_lrm = (0+0) * (1);
static const int H9_mvg_mbm = (0+0) * (1);
static const int H9_mvg_mwA = (14+0) * (1);
static const int H9_mvg_mwB = (69+0) * (1);
static const int H9_mvg_mwC = (77+0) * (1);
static const int H9_mvg_mwE = (70+0) * (1);
static const int H9_mvg_mwH = (1+0) * (1);
static const int H9_mvg_mwI = (59+0) * (1);
static const int H9_mvg_mwJ = (11+0) * (1);
static const int H9_mvg_mwK = (7+0) * (1);
static const int H9_mvg_mwL = (26+0) * (1);
static const int H9_mvg_mwD = (10+0) * (1);
static const int H9_mvg_mwT = (0+0) * (1);
static const int H9_mvg_mwG = (0+0) * (1);
static const int H9_mvg_bw = (12+0) * (1);
static const int H9_mvg_mt = (0+0) * (1);
static const int H9_mvg_qs = (0+3) * (1);
static const int H9_mvg_qf = (30+3) * (1);
static const int H9_mvg_mb = (129+0) * (1);
static const int H9_mvg_hA = (24+0) * (1);
static const int H9_mvg_hB = (24+0) * (1);
static const int H9_mvg_hC = (24+0) * (1);
static const int H9_mvg_hE = (24+0) * (1);
static const int H9_mvg_hH = (24+0) * (1);
static const int H9_mvg_hI = (24+0) * (1);
static const int H9_mvg_hJ = (24+0) * (1);
static const int H9_mvg_hK = (24+0) * (1);
static const int H9_mvg_hL = (24+0) * (1);
static const int H9_mvg_hD = (24+0) * (1);
static const int H9_mvg_hS = (24+0) * (1);
static const int H9_mvg_hF = (24+0) * (1);
static const int H9_mvg_hM = (24+0) * (1);
static const int H9_mvg_hT = (24+0) * (1);
static const int H9_mvg_hN = (24+0) * (1);


static const long long H9_mvg_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 6), 4); // 48
static const long long H9_mvg_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 7), 4), 2); // 56
static const long long H9_mvg_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 6), 6), 8); // 288
static const long long H9_mvg_d_Volume = tc_vmul(tc_vmul(1, 8), 3); // 24
static const long long H9_mvg_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 2), 5); // 90
static const long long H9_mvg_f_Volume = tc_vmul(tc_vmul(1, 2), 8); // 16
static const long long H9_mvg_g_Volume = tc_vmul(1, 1); // 1
static const long long H9_mvg_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 7), 9), 4); // 252
static const long long H9_mvg_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 4); // 16
static const long long H9_mvg_j_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 4), 5); // 40
static const long long H9_mvg_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 7), 3); // 63
static const long long H9_mvg_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 7), 6), 1); // 42
static const long long H9_mvg_m_Volume = tc_vmul(1, 1); // 1
static const long long H9_mvg_n_Volume = tc_vmul(1, 1); // 1
static const long long H9_mvg_s_Volume = tc_vmul(tc_vmul(1, 4), 4); // 16
static const long long H9_mvg_t_Volume = tc_vmul(1, 1); // 1
static const long long H9_mvg_w_Volume = tc_vmul(tc_vmul(1, 7), 1); // 7

#define H9_mvg_USES(X) X(comp) X(ts) X(mag) X(zs) X(as) X(ds) X(sa) X(sb) X(ns) X(tq) X(es) X(lst) X(ox)

struct H9_mvg_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H9_mvg_A[tc_rows(H9_mvg_a_Volume, H9_mvg_hA) * H_mvg_ND];
  cm_cnt2 H9_mvg_B[tc_rows(H9_mvg_b_Volume, H9_mvg_hB) * H_mvg_ND];
  cm_cnt2 H9_mvg_C[tc_rows(H9_mvg_c_Volume, H9_mvg_hC) * H_mvg_ND];
  cm_cnt2 H9_mvg_D[tc_rows(H9_mvg_d_Volume, H9_mvg_hD) * H_mvg_ND];
  cm_cnt2 H9_mvg_E[tc_rows(H9_mvg_e_Volume, H9_mvg_hE) * H_mvg_ND];
  cm_cnt2 H9_mvg_H[tc_rows(H9_mvg_h_Volume, H9_mvg_hH) * H_mvg_ND];
  cm_cnt2 H9_mvg_I[tc_rows(H9_mvg_i_Volume, H9_mvg_hI) * H_mvg_ND];
  cm_cnt2 H9_mvg_J[tc_rows(H9_mvg_j_Volume, H9_mvg_hJ) * H_mvg_ND];
  cm_cnt2 H9_mvg_K[tc_rows(H9_mvg_k_Volume, H9_mvg_hK) * H_mvg_ND];
  cm_cnt2 H9_mvg_L[tc_rows(H9_mvg_l_Volume, H9_mvg_hL) * H_mvg_ND];
  cm_cnt H9_mvg_MT[tc_rows(H9_mvg_t_Volume, H9_mvg_hT) * H_mvg_MND];
  i16 H9_mvg_S[tc_rows(H9_mvg_s_Volume, H9_mvg_hS) * H_mvg_ND * cm_apm_q(H9_mvg_qs)];
  u8 H9_mvg_SC[tc_rows(H9_mvg_s_Volume, H9_mvg_hS) * H_mvg_ND * cm_apm_q(H9_mvg_qs)];
  i16 H9_mvg_F[tc_rows(H9_mvg_f_Volume, H9_mvg_hF) * H_mvg_ND * cm_apm_q(H9_mvg_qf)];
  u8 H9_mvg_FC[tc_rows(H9_mvg_f_Volume, H9_mvg_hF) * H_mvg_ND * cm_apm_q(H9_mvg_qf)];
  i32 H9_mvg_W[tc_rows(H9_mvg_m_Volume, H9_mvg_hM) * H_mvg_ND * TC_MIXN];
  u8 H9_mvg_WC[tc_rows(H9_mvg_m_Volume, H9_mvg_hM) * H_mvg_ND];
  i32 H9_mvg_WM[tc_rows(H9_mvg_n_Volume, H9_mvg_hN) * H_mvg_MND * 3];
  u8 H9_mvg_WMC[tc_rows(H9_mvg_n_Volume, H9_mvg_hN) * H_mvg_MND];
  i32 H9_mvg_W2[H9_mvg_w_Volume * H_mvg_ND * TC_MIXN];
  u8 H9_mvg_W2C[H9_mvg_w_Volume * H_mvg_ND];
  i32 H9_mvg_W3[tc_rows(H9_mvg_s_Volume, H9_mvg_hS) * H_mvg_ND * TC_MIXN];
  u8 H9_mvg_W3C[tc_rows(H9_mvg_s_Volume, H9_mvg_hS) * H_mvg_ND];
  i32 H9_mvg_WF[H_mvg_ND * 5 * 4];
  u8 H9_mvg_WFC[H_mvg_ND * 5];
  cm_cnt H9_mvg_G[H9_mvg_g_Volume * H_mvg_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H9_mvg_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_mvg_a_Volume, H9_mvg_hA) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_mvg_b_Volume, H9_mvg_hB) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_mvg_c_Volume, H9_mvg_hC) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_mvg_d_Volume, H9_mvg_hD) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_mvg_e_Volume, H9_mvg_hE) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_mvg_h_Volume, H9_mvg_hH) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_mvg_i_Volume, H9_mvg_hI) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_mvg_j_Volume, H9_mvg_hJ) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_mvg_k_Volume, H9_mvg_hK) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_mvg_l_Volume, H9_mvg_hL) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H9_mvg_t_Volume, H9_mvg_hT) * H_mvg_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H9_mvg_s_Volume, H9_mvg_hS) * H_mvg_ND * cm_apm_q(H9_mvg_qs))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H9_mvg_s_Volume, H9_mvg_hS) * H_mvg_ND * cm_apm_q(H9_mvg_qs))
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H9_mvg_f_Volume, H9_mvg_hF) * H_mvg_ND * cm_apm_q(H9_mvg_qf))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H9_mvg_f_Volume, H9_mvg_hF) * H_mvg_ND * cm_apm_q(H9_mvg_qf))
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H9_mvg_m_Volume, H9_mvg_hM) * H_mvg_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H9_mvg_m_Volume, H9_mvg_hM) * H_mvg_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H9_mvg_n_Volume, H9_mvg_hN) * H_mvg_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H9_mvg_n_Volume, H9_mvg_hN) * H_mvg_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H9_mvg_w_Volume * H_mvg_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H9_mvg_w_Volume * H_mvg_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H9_mvg_s_Volume, H9_mvg_hS) * H_mvg_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H9_mvg_s_Volume, H9_mvg_hS) * H_mvg_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_mvg_ND * 5 * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_mvg_ND * 5)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H9_mvg_g_Volume * H_mvg_NG);
  void H9_mvg_Init( void ) {
  
  }
  void H9_mvg_Quit( void ) {}
};
