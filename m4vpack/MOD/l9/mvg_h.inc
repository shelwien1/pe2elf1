
static const byte M9_mvg_a_kind[5+1]={ 0,1,2,3,3,3 };

static const byte M9_mvg_b_kind[5+1]={ 0,1,2,3,4,5 };
static const byte M9_mvg_b_dz[17+1]={ 0,1,2,3,3,4,4,4,4,5,5,5,5,5,5,5,6,6 };


static const byte M9_mvg_d_kind[5+1]={ 0,0,1,2,2,3 };

static const byte M9_mvg_e_kind[5+1]={ 0,0,1,2,2,3 };
static const byte M9_mvg_e_hm[3+1]={ 0,1,2,3 };



static const byte M9_mvg_h_thm[3+1]={ 0,1,2,3 };

static const byte M9_mvg_i_kind[5+1]={ 0,0,1,2,2,3 };

static const byte M9_mvg_j_mag[16+1]={ 0,0,1,2,2,2,2,2,2,3,3,3,3,3,3,3,3 };

static const byte M9_mvg_k_kind[5+1]={ 0,0,1,2,3,3 };


static const byte M9_mvg_m_kind[5+1]={ 0,0,0,1,2,3 };




static const byte M9_mvg_w_kind[5+1]={ 0,1,2,2,3,3 };


static const int M9_mvg_rA = (252+0) * (1);
static const int M9_mvg_rB = (0+0) * (1);
static const int M9_mvg_rC = (0+0) * (1);
static const int M9_mvg_rD = (91+0) * (1);
static const int M9_mvg_rE = (254+0) * (1);
static const int M9_mvg_rH = (255+0) * (1);
static const int M9_mvg_rI = (255+0) * (1);
static const int M9_mvg_rJ = (191+0) * (1);
static const int M9_mvg_rK = (252+0) * (1);
static const int M9_mvg_rL = (0+0) * (1);
static const int M9_mvg_rS1 = (8+0) * (1);
static const int M9_mvg_rS2 = (8+0) * (1);
static const int M9_mvg_lr = (18+0) * (1);
static const int M9_mvg_mwA = (17+0) * (1);
static const int M9_mvg_mwB = (103+0) * (1);
static const int M9_mvg_mwC = (93+0) * (1);
static const int M9_mvg_mwD = (1+0) * (1);
static const int M9_mvg_mwE = (0+0) * (1);
static const int M9_mvg_mwH = (8+0) * (1);
static const int M9_mvg_mwI = (255+0) * (1);
static const int M9_mvg_mwJ = (0+0) * (1);
static const int M9_mvg_mwK = (103+0) * (1);
static const int M9_mvg_mwL = (94+0) * (1);
static const int M9_mvg_bw = (12+0) * (1);
static const int M9_mvg_qs = (0+3) * (1);
static const int M9_mvg_qf = (83+3) * (1);
static const int M9_mvg_mb = (129+0) * (1);
static const int M9_mvg_rT = (0+0) * (1);
static const int M9_mvg_mwT = (0+0) * (1);
static const int M9_mvg_mt = (0+0) * (1);
static const int M9_mvg_lrm = (0+0) * (1);
static const int M9_mvg_mbm = (0+0) * (1);
static const int M9_mvg_rG = (0+0) * (1);
static const int M9_mvg_mwG = (0+0) * (1);
static const int M9_mvg_hA = (22+0) * (1);
static const int M9_mvg_hB = (22+0) * (1);
static const int M9_mvg_hC = (22+0) * (1);
static const int M9_mvg_hD = (22+0) * (1);
static const int M9_mvg_hE = (22+0) * (1);
static const int M9_mvg_hH = (22+0) * (1);
static const int M9_mvg_hI = (22+0) * (1);
static const int M9_mvg_hJ = (22+0) * (1);
static const int M9_mvg_hK = (22+0) * (1);
static const int M9_mvg_hL = (22+0) * (1);
static const int M9_mvg_hS = (22+0) * (1);
static const int M9_mvg_hF = (22+0) * (1);
static const int M9_mvg_hM = (22+0) * (1);
static const int M9_mvg_hT = (22+0) * (1);
static const int M9_mvg_hN = (22+0) * (1);


static const long long M9_mvg_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 2), 3), 3); // 72
static const long long M9_mvg_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 6), 2), 3), 7); // 252
static const long long M9_mvg_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 3); // 18
static const long long M9_mvg_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 1); // 4
static const long long M9_mvg_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 3), 4); // 48
static const long long M9_mvg_f_Volume = tc_vmul(1, 3); // 3
static const long long M9_mvg_g_Volume = 1; // 1
static const long long M9_mvg_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 3), 4); // 24
static const long long M9_mvg_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 2); // 8
static const long long M9_mvg_j_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 3), 4); // 72
static const long long M9_mvg_k_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 3), 3), 2); // 72
static const long long M9_mvg_l_Volume = tc_vmul(tc_vmul(1, 3), 2); // 6
static const long long M9_mvg_m_Volume = tc_vmul(tc_vmul(1, 4), 1); // 4
static const long long M9_mvg_n_Volume = 1; // 1
static const long long M9_mvg_s_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 1), 3); // 3
static const long long M9_mvg_t_Volume = 1; // 1
static const long long M9_mvg_w_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 2), 2); // 16

#define M9_mvg_USES(X) X(kind) X(comp) X(ls) X(as) X(ds) X(dz) X(mag) X(hm) X(ts) X(thm)

struct M9_mvg_T {
  cm_cnt2 M9_mvg_A[tc_rows(M9_mvg_a_Volume, M9_mvg_hA) * M_mvg_ND];
  cm_cnt2 M9_mvg_B[tc_rows(M9_mvg_b_Volume, M9_mvg_hB) * M_mvg_ND];
  cm_cnt2 M9_mvg_C[tc_rows(M9_mvg_c_Volume, M9_mvg_hC) * M_mvg_ND];
  cm_cnt2 M9_mvg_D[tc_rows(M9_mvg_d_Volume, M9_mvg_hD) * M_mvg_ND];
  cm_cnt2 M9_mvg_E[tc_rows(M9_mvg_e_Volume, M9_mvg_hE) * M_mvg_ND];
  cm_cnt2 M9_mvg_H[tc_rows(M9_mvg_h_Volume, M9_mvg_hH) * M_mvg_ND];
  cm_cnt2 M9_mvg_I[tc_rows(M9_mvg_i_Volume, M9_mvg_hI) * M_mvg_ND];
  cm_cnt2 M9_mvg_J[tc_rows(M9_mvg_j_Volume, M9_mvg_hJ) * M_mvg_ND];
  cm_cnt2 M9_mvg_K[tc_rows(M9_mvg_k_Volume, M9_mvg_hK) * M_mvg_ND];
  cm_cnt2 M9_mvg_L[tc_rows(M9_mvg_l_Volume, M9_mvg_hL) * M_mvg_ND];
  cm_cnt M9_mvg_MT[tc_rows(M9_mvg_t_Volume, M9_mvg_hT) * M_mvg_MND];
  i16 M9_mvg_S[tc_rows(M9_mvg_s_Volume, M9_mvg_hS) * M_mvg_ND * cm_apm_q(M9_mvg_qs)];
  u8 M9_mvg_SC[tc_rows(M9_mvg_s_Volume, M9_mvg_hS) * M_mvg_ND * cm_apm_q(M9_mvg_qs)];
  i16 M9_mvg_F[tc_rows(M9_mvg_f_Volume, M9_mvg_hF) * M_mvg_ND * cm_apm_q(M9_mvg_qf)];
  u8 M9_mvg_FC[tc_rows(M9_mvg_f_Volume, M9_mvg_hF) * M_mvg_ND * cm_apm_q(M9_mvg_qf)];
  i32 M9_mvg_W[tc_rows(M9_mvg_m_Volume, M9_mvg_hM) * M_mvg_ND * tc_nin(10)];
  u8 M9_mvg_WC[tc_rows(M9_mvg_m_Volume, M9_mvg_hM) * M_mvg_ND];
  i32 M9_mvg_WM[tc_rows(M9_mvg_n_Volume, M9_mvg_hN) * M_mvg_MND * TC_MIN];
  u8 M9_mvg_WMC[tc_rows(M9_mvg_n_Volume, M9_mvg_hN) * M_mvg_MND];
  i32 M9_mvg_W2[M9_mvg_w_Volume * M_mvg_ND * tc_nin(10)];
  u8 M9_mvg_W2C[M9_mvg_w_Volume * M_mvg_ND];
  i32 M9_mvg_W3[tc_rows(M9_mvg_s_Volume, M9_mvg_hS) * M_mvg_ND * tc_nin(10)];
  u8 M9_mvg_W3C[tc_rows(M9_mvg_s_Volume, M9_mvg_hS) * M_mvg_ND];
  i32 M9_mvg_WF[M_mvg_ND * HS_N * TC_FIN];
  u8 M9_mvg_WFC[M_mvg_ND * HS_N];
  cm_cnt M9_mvg_G[M9_mvg_g_Volume * M_mvg_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M9_mvg_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_mvg_a_Volume, M9_mvg_hA) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_mvg_b_Volume, M9_mvg_hB) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_mvg_c_Volume, M9_mvg_hC) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_mvg_d_Volume, M9_mvg_hD) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_mvg_e_Volume, M9_mvg_hE) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_mvg_h_Volume, M9_mvg_hH) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_mvg_i_Volume, M9_mvg_hI) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_mvg_j_Volume, M9_mvg_hJ) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_mvg_k_Volume, M9_mvg_hK) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_mvg_l_Volume, M9_mvg_hL) * M_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M9_mvg_t_Volume, M9_mvg_hT) * M_mvg_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(M9_mvg_s_Volume, M9_mvg_hS) * M_mvg_ND * cm_apm_q(M9_mvg_qs))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_mvg_s_Volume, M9_mvg_hS) * M_mvg_ND * cm_apm_q(M9_mvg_qs))
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(M9_mvg_f_Volume, M9_mvg_hF) * M_mvg_ND * cm_apm_q(M9_mvg_qf))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_mvg_f_Volume, M9_mvg_hF) * M_mvg_ND * cm_apm_q(M9_mvg_qf))
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M9_mvg_m_Volume, M9_mvg_hM) * M_mvg_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_mvg_m_Volume, M9_mvg_hM) * M_mvg_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M9_mvg_n_Volume, M9_mvg_hN) * M_mvg_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_mvg_n_Volume, M9_mvg_hN) * M_mvg_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M9_mvg_w_Volume * M_mvg_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M9_mvg_w_Volume * M_mvg_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M9_mvg_s_Volume, M9_mvg_hS) * M_mvg_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_mvg_s_Volume, M9_mvg_hS) * M_mvg_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M_mvg_ND * HS_N * TC_FIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M_mvg_ND * HS_N)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M9_mvg_g_Volume * M_mvg_NG);
  void M9_mvg_Init( void ) {}
  void M9_mvg_Quit( void ) {}
};
