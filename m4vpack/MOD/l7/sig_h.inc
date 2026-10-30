


static const byte M7_sig_c_nz[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte M7_sig_d_past[8+1]={ 0,1,2,3,4,5,6,6,7 };

static const byte M7_sig_e_run[16+1]={ 0,1,2,3,3,4,4,4,4,5,5,5,5,5,5,5,5 };




static const byte M7_sig_i_mvc[30+1]={ 0,1,2,3,3,4,4,4,4,5,5,5,5,5,5,5,6,6,7,8,9,10,11,12,13,14,14,14,14,14,14 };

static const byte M7_sig_j_rnz[17+1]={ 0,1,2,3,3,3,4,4,4,4,4,5,5,5,5,5,5,5 };
static const byte M7_sig_j_xm[9+1]={ 0,1,2,2,2,2,2,2,3,3 };





static const byte M7_sig_s_sl[17+1]={ 0,0,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3 };
static const byte M7_sig_s_sa[17+1]={ 0,1,1,2,2,2,3,3,3,3,3,3,3,3,3,3,4,4 };




static const int M7_sig_rA = (3+0) * (1);
static const int M7_sig_mwA = (3+0) * (1);
static const int M7_sig_rB = (255+0) * (1);
static const int M7_sig_mwB = (128+0) * (1);
static const int M7_sig_rC = (255+0) * (1);
static const int M7_sig_mwC = (46+0) * (1);
static const int M7_sig_rD = (252+0) * (1);
static const int M7_sig_mwD = (5+0) * (1);
static const int M7_sig_rE = (254+0) * (1);
static const int M7_sig_mwE = (1+0) * (1);
static const int M7_sig_rH = (254+0) * (1);
static const int M7_sig_mwH = (19+0) * (1);
static const int M7_sig_rI = (255+0) * (1);
static const int M7_sig_mwI = (32+0) * (1);
static const int M7_sig_rJ = (255+0) * (1);
static const int M7_sig_mwJ = (4+0) * (1);
static const int M7_sig_rK = (255+0) * (1);
static const int M7_sig_mwK = (32+0) * (1);
static const int M7_sig_rL = (254+0) * (1);
static const int M7_sig_mwL = (4+0) * (1);
static const int M7_sig_lr = (12+0) * (1);
static const int M7_sig_mb = (66+0) * (1);
static const int M7_sig_rS1 = (16+0) * (1);
static const int M7_sig_rS2 = (0+0) * (1);
static const int M7_sig_qs = (67+3) * (1);
static const int M7_sig_qf = (0+3) * (1);
static const int M7_sig_bw = (16+0) * (1);
static const int M7_sig_rT = (0+0) * (1);
static const int M7_sig_mwT = (0+0) * (1);
static const int M7_sig_mt = (0+0) * (1);
static const int M7_sig_lrm = (0+0) * (1);
static const int M7_sig_mbm = (0+0) * (1);
static const int M7_sig_rG = (0+0) * (1);
static const int M7_sig_mwG = (0+0) * (1);
static const int M7_sig_hA = (22+0) * (1);
static const int M7_sig_hB = (22+0) * (1);
static const int M7_sig_hC = (22+0) * (1);
static const int M7_sig_hD = (22+0) * (1);
static const int M7_sig_hE = (22+0) * (1);
static const int M7_sig_hH = (22+0) * (1);
static const int M7_sig_hI = (22+0) * (1);
static const int M7_sig_hJ = (22+0) * (1);
static const int M7_sig_hK = (22+0) * (1);
static const int M7_sig_hL = (20+0) * (1);
static const int M7_sig_hM = (22+0) * (1);
static const int M7_sig_hT = (22+0) * (1);
static const int M7_sig_hN = (22+0) * (1);
static const int M7_sig_hS = (22+0) * (1);
static const int M7_sig_hF = (22+0) * (1);


static const long long M7_sig_a_Volume = tc_vmul(tc_vmul(1, 1), 3); // 3
static const long long M7_sig_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 1), (1<<2)); // 32
static const long long M7_sig_c_Volume = tc_vmul(tc_vmul(1, (1<<1)), 9); // 18
static const long long M7_sig_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 8), 1), (1<<6)); // 4096
static const long long M7_sig_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<2)), 6), (1<<6)); // 1536
static const long long M7_sig_f_Volume = tc_vmul(tc_vmul(1, (1<<3)), (1<<6)); // 512
static const long long M7_sig_g_Volume = 1; // 1
static const long long M7_sig_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<1)), (1<<5)), (1<<6)); // 4096
static const long long M7_sig_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 15), (1<<6)); // 7680
static const long long M7_sig_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<2)), 6), 4); // 96
static const long long M7_sig_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), (1<<8)), (1<<6)); // 131072
static const long long M7_sig_l_Volume = tc_vmul(tc_vmul(1, (1<<6)), (1<<14)); // 1048576
static const long long M7_sig_m_Volume = tc_vmul(tc_vmul(1, (1<<1)), (1<<5)); // 64
static const long long M7_sig_n_Volume = 1; // 1
static const long long M7_sig_s_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<1)), 4), 5), 3); // 120
static const long long M7_sig_t_Volume = 1; // 1
static const long long M7_sig_w_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<1)), 3), 1); // 6

#define M7_sig_USES(X) X(qs) X(cls) X(pos) X(nz) X(past) X(run) X(fp) X(mvc) X(rnz) X(xm) X(h8) X(h16) X(kb) X(sl) X(sa)

struct M7_sig_T {
  cm_cnt2 M7_sig_A[tc_rows(M7_sig_a_Volume, M7_sig_hA) * M_sig_ND];
  cm_cnt2 M7_sig_B[tc_rows(M7_sig_b_Volume, M7_sig_hB) * M_sig_ND];
  cm_cnt2 M7_sig_C[tc_rows(M7_sig_c_Volume, M7_sig_hC) * M_sig_ND];
  cm_cnt2 M7_sig_D[tc_rows(M7_sig_d_Volume, M7_sig_hD) * M_sig_ND];
  cm_cnt2 M7_sig_E[tc_rows(M7_sig_e_Volume, M7_sig_hE) * M_sig_ND];
  cm_cnt2 M7_sig_H[tc_rows(M7_sig_h_Volume, M7_sig_hH) * M_sig_ND];
  cm_cnt2 M7_sig_I[tc_rows(M7_sig_i_Volume, M7_sig_hI) * M_sig_ND];
  cm_cnt2 M7_sig_J[tc_rows(M7_sig_j_Volume, M7_sig_hJ) * M_sig_ND];
  cm_cnt2 M7_sig_K[tc_rows(M7_sig_k_Volume, M7_sig_hK) * M_sig_ND];
  cm_cnt2 M7_sig_L[tc_rows(M7_sig_l_Volume, M7_sig_hL) * M_sig_ND];
  cm_cnt M7_sig_MT[tc_rows(M7_sig_t_Volume, M7_sig_hT) * M_sig_MND];
  i16 M7_sig_S[tc_rows(M7_sig_s_Volume, M7_sig_hS) * M_sig_ND * cm_apm_q(M7_sig_qs)];
  u8 M7_sig_SC[tc_rows(M7_sig_s_Volume, M7_sig_hS) * M_sig_ND * cm_apm_q(M7_sig_qs)];
  i16 M7_sig_F[tc_rows(M7_sig_f_Volume, M7_sig_hF) * M_sig_ND * cm_apm_q(M7_sig_qf)];
  u8 M7_sig_FC[tc_rows(M7_sig_f_Volume, M7_sig_hF) * M_sig_ND * cm_apm_q(M7_sig_qf)];
  i32 M7_sig_W[tc_rows(M7_sig_m_Volume, M7_sig_hM) * M_sig_ND * tc_nin(10)];
  u8 M7_sig_WC[tc_rows(M7_sig_m_Volume, M7_sig_hM) * M_sig_ND];
  i32 M7_sig_WM[tc_rows(M7_sig_n_Volume, M7_sig_hN) * M_sig_MND * TC_MIN];
  u8 M7_sig_WMC[tc_rows(M7_sig_n_Volume, M7_sig_hN) * M_sig_MND];
  i32 M7_sig_W2[M7_sig_w_Volume * M_sig_ND * tc_nin(10)];
  u8 M7_sig_W2C[M7_sig_w_Volume * M_sig_ND];
  i32 M7_sig_W3[tc_rows(M7_sig_s_Volume, M7_sig_hS) * M_sig_ND * tc_nin(10)];
  u8 M7_sig_W3C[tc_rows(M7_sig_s_Volume, M7_sig_hS) * M_sig_ND];
  i32 M7_sig_WF[M_sig_ND * HS_N * TC_FIN];
  u8 M7_sig_WFC[M_sig_ND * HS_N];
  cm_cnt M7_sig_G[M7_sig_g_Volume * M_sig_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M7_sig_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_sig_a_Volume, M7_sig_hA) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_sig_b_Volume, M7_sig_hB) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_sig_c_Volume, M7_sig_hC) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_sig_d_Volume, M7_sig_hD) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_sig_e_Volume, M7_sig_hE) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_sig_h_Volume, M7_sig_hH) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_sig_i_Volume, M7_sig_hI) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_sig_j_Volume, M7_sig_hJ) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_sig_k_Volume, M7_sig_hK) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M7_sig_l_Volume, M7_sig_hL) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M7_sig_t_Volume, M7_sig_hT) * M_sig_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(M7_sig_s_Volume, M7_sig_hS) * M_sig_ND * cm_apm_q(M7_sig_qs))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M7_sig_s_Volume, M7_sig_hS) * M_sig_ND * cm_apm_q(M7_sig_qs))
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(M7_sig_f_Volume, M7_sig_hF) * M_sig_ND * cm_apm_q(M7_sig_qf))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M7_sig_f_Volume, M7_sig_hF) * M_sig_ND * cm_apm_q(M7_sig_qf))
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M7_sig_m_Volume, M7_sig_hM) * M_sig_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M7_sig_m_Volume, M7_sig_hM) * M_sig_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M7_sig_n_Volume, M7_sig_hN) * M_sig_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M7_sig_n_Volume, M7_sig_hN) * M_sig_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M7_sig_w_Volume * M_sig_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M7_sig_w_Volume * M_sig_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M7_sig_s_Volume, M7_sig_hS) * M_sig_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M7_sig_s_Volume, M7_sig_hS) * M_sig_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M_sig_ND * HS_N * TC_FIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M_sig_ND * HS_N)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M7_sig_g_Volume * M_sig_NG);
  void M7_sig_Init( void ) {}
  void M7_sig_Quit( void ) {}
};
