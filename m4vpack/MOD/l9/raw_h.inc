



static const byte M9_raw_d_code[11+1]={ 0,1,1,2,3,4,5,6,6,6,6,6 };

static const byte M9_raw_e_code[11+1]={ 0,1,2,2,3,3,4,5,5,5,5,5 };
static const byte M9_raw_e_at[11+1]={ 0,0,1,2,2,2,3,3,4,4,5,6 };



static const byte M9_raw_h_at[11+1]={ 0,0,0,0,0,1,2,3,4,4,5,5 };

static const byte M9_raw_i_code[11+1]={ 0,1,2,2,2,2,3,3,4,4,4,4 };

static const byte M9_raw_j_at[11+1]={ 0,0,1,2,3,3,4,5,6,6,7,8 };


static const byte M9_raw_l_code[11+1]={ 0,1,1,2,2,3,4,4,5,5,5,5 };

static const byte M9_raw_m_at[11+1]={ 0,0,1,2,3,3,3,4,5,5,5,6 };


static const byte M9_raw_s_code[11+1]={ 0,1,2,2,3,3,4,5,5,5,5,5 };


static const byte M9_raw_w_code[11+1]={ 0,1,2,2,2,3,4,5,5,5,5,5 };


static const int M9_raw_rA = (234+0) * (1);
static const int M9_raw_rB = (41+0) * (1);
static const int M9_raw_rC = (0+0) * (1);
static const int M9_raw_rD = (247+0) * (1);
static const int M9_raw_rE = (246+0) * (1);
static const int M9_raw_rH = (226+0) * (1);
static const int M9_raw_rI = (238+0) * (1);
static const int M9_raw_rJ = (236+0) * (1);
static const int M9_raw_rK = (142+0) * (1);
static const int M9_raw_rL = (51+0) * (1);
static const int M9_raw_rS1 = (8+0) * (1);
static const int M9_raw_rS2 = (16+0) * (1);
static const int M9_raw_lr = (31+0) * (1);
static const int M9_raw_mwA = (255+0) * (1);
static const int M9_raw_mwB = (0+0) * (1);
static const int M9_raw_mwC = (0+0) * (1);
static const int M9_raw_mwD = (255+0) * (1);
static const int M9_raw_mwE = (0+0) * (1);
static const int M9_raw_mwH = (2+0) * (1);
static const int M9_raw_mwI = (0+0) * (1);
static const int M9_raw_mwJ = (241+0) * (1);
static const int M9_raw_mwK = (206+0) * (1);
static const int M9_raw_mwL = (127+0) * (1);
static const int M9_raw_bw = (16+0) * (1);
static const int M9_raw_qs = (11+3) * (1);
static const int M9_raw_qf = (16+3) * (1);
static const int M9_raw_mb = (22+0) * (1);
static const int M9_raw_rT = (0+0) * (1);
static const int M9_raw_mwT = (0+0) * (1);
static const int M9_raw_mt = (0+0) * (1);
static const int M9_raw_lrm = (0+0) * (1);
static const int M9_raw_mbm = (0+0) * (1);
static const int M9_raw_rG = (0+0) * (1);
static const int M9_raw_mwG = (0+0) * (1);
static const int M9_raw_hA = (22+0) * (1);
static const int M9_raw_hB = (22+0) * (1);
static const int M9_raw_hC = (20+0) * (1);
static const int M9_raw_hD = (22+0) * (1);
static const int M9_raw_hE = (22+0) * (1);
static const int M9_raw_hH = (22+0) * (1);
static const int M9_raw_hI = (22+0) * (1);
static const int M9_raw_hJ = (22+0) * (1);
static const int M9_raw_hK = (22+0) * (1);
static const int M9_raw_hL = (22+0) * (1);
static const int M9_raw_hS = (22+0) * (1);
static const int M9_raw_hF = (22+0) * (1);
static const int M9_raw_hM = (22+0) * (1);
static const int M9_raw_hT = (22+0) * (1);
static const int M9_raw_hN = (22+0) * (1);


static const long long M9_raw_a_Volume = 1; // 1
static const long long M9_raw_b_Volume = tc_vmul(1, (1<<8)); // 256
static const long long M9_raw_c_Volume = tc_vmul(tc_vmul(1, (1<<8)), (1<<4)); // 4096
static const long long M9_raw_d_Volume = tc_vmul(1, 7); // 7
static const long long M9_raw_e_Volume = tc_vmul(tc_vmul(1, 6), 7); // 42
static const long long M9_raw_f_Volume = 1; // 1
static const long long M9_raw_g_Volume = 1; // 1
static const long long M9_raw_h_Volume = tc_vmul(tc_vmul(1, (1<<5)), 6); // 192
static const long long M9_raw_i_Volume = tc_vmul(tc_vmul(1, (1<<4)), 5); // 80
static const long long M9_raw_j_Volume = tc_vmul(1, 9); // 9
static const long long M9_raw_k_Volume = tc_vmul(1, (1<<7)); // 128
static const long long M9_raw_l_Volume = tc_vmul(tc_vmul(1, (1<<7)), 6); // 768
static const long long M9_raw_m_Volume = tc_vmul(1, 7); // 7
static const long long M9_raw_n_Volume = 1; // 1
static const long long M9_raw_s_Volume = tc_vmul(1, 6); // 6
static const long long M9_raw_t_Volume = 1; // 1
static const long long M9_raw_w_Volume = tc_vmul(1, 6); // 6

#define M9_raw_USES(X) X(c1) X(c2) X(code) X(at)

struct M9_raw_T {
  cm_cnt2 M9_raw_A[tc_rows(M9_raw_a_Volume, M9_raw_hA) * M_raw_ND];
  cm_cnt2 M9_raw_B[tc_rows(M9_raw_b_Volume, M9_raw_hB) * M_raw_ND];
  cm_cnt2 M9_raw_C[tc_rows(M9_raw_c_Volume, M9_raw_hC) * M_raw_ND];
  cm_cnt2 M9_raw_D[tc_rows(M9_raw_d_Volume, M9_raw_hD) * M_raw_ND];
  cm_cnt2 M9_raw_E[tc_rows(M9_raw_e_Volume, M9_raw_hE) * M_raw_ND];
  cm_cnt2 M9_raw_H[tc_rows(M9_raw_h_Volume, M9_raw_hH) * M_raw_ND];
  cm_cnt2 M9_raw_I[tc_rows(M9_raw_i_Volume, M9_raw_hI) * M_raw_ND];
  cm_cnt2 M9_raw_J[tc_rows(M9_raw_j_Volume, M9_raw_hJ) * M_raw_ND];
  cm_cnt2 M9_raw_K[tc_rows(M9_raw_k_Volume, M9_raw_hK) * M_raw_ND];
  cm_cnt2 M9_raw_L[tc_rows(M9_raw_l_Volume, M9_raw_hL) * M_raw_ND];
  cm_cnt M9_raw_MT[tc_rows(M9_raw_t_Volume, M9_raw_hT) * M_raw_MND];
  i16 M9_raw_S[tc_rows(M9_raw_s_Volume, M9_raw_hS) * M_raw_ND * cm_apm_q(M9_raw_qs)];
  u8 M9_raw_SC[tc_rows(M9_raw_s_Volume, M9_raw_hS) * M_raw_ND * cm_apm_q(M9_raw_qs)];
  i16 M9_raw_F[tc_rows(M9_raw_f_Volume, M9_raw_hF) * M_raw_ND * cm_apm_q(M9_raw_qf)];
  u8 M9_raw_FC[tc_rows(M9_raw_f_Volume, M9_raw_hF) * M_raw_ND * cm_apm_q(M9_raw_qf)];
  i32 M9_raw_W[tc_rows(M9_raw_m_Volume, M9_raw_hM) * M_raw_ND * tc_nin(10)];
  u8 M9_raw_WC[tc_rows(M9_raw_m_Volume, M9_raw_hM) * M_raw_ND];
  i32 M9_raw_WM[tc_rows(M9_raw_n_Volume, M9_raw_hN) * M_raw_MND * TC_MIN];
  u8 M9_raw_WMC[tc_rows(M9_raw_n_Volume, M9_raw_hN) * M_raw_MND];
  i32 M9_raw_W2[M9_raw_w_Volume * M_raw_ND * tc_nin(10)];
  u8 M9_raw_W2C[M9_raw_w_Volume * M_raw_ND];
  i32 M9_raw_W3[tc_rows(M9_raw_s_Volume, M9_raw_hS) * M_raw_ND * tc_nin(10)];
  u8 M9_raw_W3C[tc_rows(M9_raw_s_Volume, M9_raw_hS) * M_raw_ND];
  i32 M9_raw_WF[M_raw_ND * HS_N * TC_FIN];
  u8 M9_raw_WFC[M_raw_ND * HS_N];
  cm_cnt M9_raw_G[M9_raw_g_Volume * M_raw_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M9_raw_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_raw_a_Volume, M9_raw_hA) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_raw_b_Volume, M9_raw_hB) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_raw_c_Volume, M9_raw_hC) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_raw_d_Volume, M9_raw_hD) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_raw_e_Volume, M9_raw_hE) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_raw_h_Volume, M9_raw_hH) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_raw_i_Volume, M9_raw_hI) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_raw_j_Volume, M9_raw_hJ) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_raw_k_Volume, M9_raw_hK) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M9_raw_l_Volume, M9_raw_hL) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M9_raw_t_Volume, M9_raw_hT) * M_raw_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(M9_raw_s_Volume, M9_raw_hS) * M_raw_ND * cm_apm_q(M9_raw_qs))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_raw_s_Volume, M9_raw_hS) * M_raw_ND * cm_apm_q(M9_raw_qs))
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(M9_raw_f_Volume, M9_raw_hF) * M_raw_ND * cm_apm_q(M9_raw_qf))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_raw_f_Volume, M9_raw_hF) * M_raw_ND * cm_apm_q(M9_raw_qf))
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M9_raw_m_Volume, M9_raw_hM) * M_raw_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_raw_m_Volume, M9_raw_hM) * M_raw_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M9_raw_n_Volume, M9_raw_hN) * M_raw_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_raw_n_Volume, M9_raw_hN) * M_raw_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M9_raw_w_Volume * M_raw_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M9_raw_w_Volume * M_raw_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M9_raw_s_Volume, M9_raw_hS) * M_raw_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M9_raw_s_Volume, M9_raw_hS) * M_raw_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M_raw_ND * HS_N * TC_FIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M_raw_ND * HS_N)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M9_raw_g_Volume * M_raw_NG);
  void M9_raw_Init( void ) {}
  void M9_raw_Quit( void ) {}
};
