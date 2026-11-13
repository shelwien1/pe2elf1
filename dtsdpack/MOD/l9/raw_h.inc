
static const byte D9_raw_a_code[3+1]={ 0,1,2,3 };

static const byte D9_raw_b_code[3+1]={ 0,1,2,3 };

static const byte D9_raw_c_code[3+1]={ 0,1,2,3 };

static const byte D9_raw_d_code[3+1]={ 0,1,2,3 };
static const byte D9_raw_d_cl[256+1]={ 0,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9 };

static const byte D9_raw_e_code[3+1]={ 0,1,2,3 };
static const byte D9_raw_e_at[39+1]={ 0,0,0,0,1,2,3,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 };

static const byte D9_raw_f_code[3+1]={ 0,1,2,3 };


static const byte D9_raw_h_code[3+1]={ 0,1,2,3 };
static const byte D9_raw_h_cl[256+1]={ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6 };

static const byte D9_raw_i_code[3+1]={ 0,1,2,3 };
static const byte D9_raw_i_at[39+1]={ 0,1,1,1,1,1,1,2,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };

static const byte D9_raw_j_code[3+1]={ 0,1,2,3 };
static const byte D9_raw_j_cl[256+1]={ 0,0,0,0,0,0,1,1,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9 };
static const byte D9_raw_j_at[39+1]={ 0,0,0,0,0,1,2,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };


static const byte D9_raw_l_code[3+1]={ 0,1,2,3 };

static const byte D9_raw_m_code[3+1]={ 0,1,2,3 };


static const byte D9_raw_s_code[3+1]={ 0,1,2,3 };
static const byte D9_raw_s_at[39+1]={ 0,0,1,1,2,3,4,5,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7 };


static const byte D9_raw_w_at[39+1]={ 0,1,2,2,2,3,4,4,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6 };


static const int D9_raw_rA = (10+0) * (1);
static const int D9_raw_mwA = (253+0) * (1);
static const int D9_raw_rB = (0+0) * (1);
static const int D9_raw_mwB = (32+0) * (1);
static const int D9_raw_rC = (0+0) * (1);
static const int D9_raw_mwC = (0+0) * (1);
static const int D9_raw_rD = (16+0) * (1);
static const int D9_raw_mwD = (0+0) * (1);
static const int D9_raw_rE = (22+0) * (1);
static const int D9_raw_mwE = (114+0) * (1);
static const int D9_raw_rH = (0+0) * (1);
static const int D9_raw_mwH = (7+0) * (1);
static const int D9_raw_rI = (0+0) * (1);
static const int D9_raw_mwI = (0+0) * (1);
static const int D9_raw_rJ = (0+0) * (1);
static const int D9_raw_mwJ = (33+0) * (1);
static const int D9_raw_rK = (0+0) * (1);
static const int D9_raw_mwK = (0+0) * (1);
static const int D9_raw_rL = (0+0) * (1);
static const int D9_raw_mwL = (66+0) * (1);
static const int D9_raw_lr = (15+0) * (1);
static const int D9_raw_mb = (191+0) * (1);
static const int D9_raw_rS1 = (8+0) * (1);
static const int D9_raw_rS2 = (2+0) * (1);
static const int D9_raw_qs = (116+3) * (1);
static const int D9_raw_qf = (16+3) * (1);
static const int D9_raw_bw = (0+0) * (1);
static const int D9_raw_rT = (0+0) * (1);
static const int D9_raw_mwT = (0+0) * (1);
static const int D9_raw_mt = (0+0) * (1);
static const int D9_raw_lrm = (0+0) * (1);
static const int D9_raw_mbm = (0+0) * (1);
static const int D9_raw_rG = (0+0) * (1);
static const int D9_raw_mwG = (0+0) * (1);
static const int D9_raw_hA = (20+0) * (1);
static const int D9_raw_hB = (20+0) * (1);
static const int D9_raw_hC = (20+0) * (1);
static const int D9_raw_hD = (20+0) * (1);
static const int D9_raw_hE = (20+0) * (1);
static const int D9_raw_hH = (20+0) * (1);
static const int D9_raw_hI = (20+0) * (1);
static const int D9_raw_hJ = (20+0) * (1);
static const int D9_raw_hK = (20+0) * (1);
static const int D9_raw_hL = (20+0) * (1);
static const int D9_raw_hS = (20+0) * (1);
static const int D9_raw_hF = (20+0) * (1);
static const int D9_raw_hM = (20+0) * (1);
static const int D9_raw_hT = (20+0) * (1);
static const int D9_raw_hN = (20+0) * (1);


static const long long D9_raw_a_Volume = tc_vmul(1, 4); // 4
static const long long D9_raw_b_Volume = tc_vmul(tc_vmul(1, 4), (1<<7)); // 512
static const long long D9_raw_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), (1<<5)), (1<<4)); // 2048
static const long long D9_raw_d_Volume = tc_vmul(tc_vmul(1, 4), 10); // 40
static const long long D9_raw_e_Volume = tc_vmul(tc_vmul(1, 4), 6); // 24
static const long long D9_raw_f_Volume = tc_vmul(tc_vmul(1, 4), (1<<7)); // 512
static const long long D9_raw_g_Volume = 1; // 1
static const long long D9_raw_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 7), (1<<5)); // 896
static const long long D9_raw_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), (1<<5)), 5); // 640
static const long long D9_raw_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 10), 5); // 200
static const long long D9_raw_k_Volume = tc_vmul(tc_vmul(1, (1<<4)), (1<<2)); // 64
static const long long D9_raw_l_Volume = tc_vmul(tc_vmul(1, 4), (1<<5)); // 128
static const long long D9_raw_m_Volume = tc_vmul(1, 4); // 4
static const long long D9_raw_n_Volume = 1; // 1
static const long long D9_raw_s_Volume = tc_vmul(tc_vmul(1, 4), 8); // 32
static const long long D9_raw_t_Volume = 1; // 1
static const long long D9_raw_w_Volume = tc_vmul(1, 7); // 7

#define D9_raw_USES(X) X(code) X(c1) X(c2) X(cl) X(at)

struct D9_raw_T {
  cm_cnt2 D9_raw_A[tc_rows(D9_raw_a_Volume, D9_raw_hA) * M_raw_ND];
  cm_cnt2 D9_raw_B[tc_rows(D9_raw_b_Volume, D9_raw_hB) * M_raw_ND];
  cm_cnt2 D9_raw_C[tc_rows(D9_raw_c_Volume, D9_raw_hC) * M_raw_ND];
  cm_cnt2 D9_raw_D[tc_rows(D9_raw_d_Volume, D9_raw_hD) * M_raw_ND];
  cm_cnt2 D9_raw_E[tc_rows(D9_raw_e_Volume, D9_raw_hE) * M_raw_ND];
  cm_cnt2 D9_raw_H[tc_rows(D9_raw_h_Volume, D9_raw_hH) * M_raw_ND];
  cm_cnt2 D9_raw_I[tc_rows(D9_raw_i_Volume, D9_raw_hI) * M_raw_ND];
  cm_cnt2 D9_raw_J[tc_rows(D9_raw_j_Volume, D9_raw_hJ) * M_raw_ND];
  cm_cnt2 D9_raw_K[tc_rows(D9_raw_k_Volume, D9_raw_hK) * M_raw_ND];
  cm_cnt2 D9_raw_L[tc_rows(D9_raw_l_Volume, D9_raw_hL) * M_raw_ND];
  cm_cnt D9_raw_MT[tc_rows(D9_raw_t_Volume, D9_raw_hT) * M_raw_MND];
  cm_apme D9_raw_S[tc_rows(D9_raw_s_Volume, D9_raw_hS) * M_raw_ND * cm_apm_q(D9_raw_qs)];
  cm_apme D9_raw_F[tc_rows(D9_raw_f_Volume, D9_raw_hF) * M_raw_ND * cm_apm_q(D9_raw_qf)];
  i32 D9_raw_W[tc_rows(D9_raw_m_Volume, D9_raw_hM) * M_raw_ND * tc_nin(10)];
  u8 D9_raw_WC[tc_rows(D9_raw_m_Volume, D9_raw_hM) * M_raw_ND];
  i32 D9_raw_WM[tc_rows(D9_raw_n_Volume, D9_raw_hN) * M_raw_MND * TC_MIN];
  u8 D9_raw_WMC[tc_rows(D9_raw_n_Volume, D9_raw_hN) * M_raw_MND];
  i32 D9_raw_W2[D9_raw_w_Volume * M_raw_ND * tc_nin(10)];
  u8 D9_raw_W2C[D9_raw_w_Volume * M_raw_ND];
  i32 D9_raw_W3[tc_rows(D9_raw_s_Volume, D9_raw_hS) * M_raw_ND * tc_nin(10)];
  u8 D9_raw_W3C[tc_rows(D9_raw_s_Volume, D9_raw_hS) * M_raw_ND];
  i32 D9_raw_WF[M_raw_ND * HS_N * TC_FIN];
  u8 D9_raw_WFC[M_raw_ND * HS_N];
  cm_cnt D9_raw_G[D9_raw_g_Volume * M_raw_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long D9_raw_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_raw_a_Volume, D9_raw_hA) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_raw_b_Volume, D9_raw_hB) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_raw_c_Volume, D9_raw_hC) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_raw_d_Volume, D9_raw_hD) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_raw_e_Volume, D9_raw_hE) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_raw_h_Volume, D9_raw_hH) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_raw_i_Volume, D9_raw_hI) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_raw_j_Volume, D9_raw_hJ) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_raw_k_Volume, D9_raw_hK) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(D9_raw_l_Volume, D9_raw_hL) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(D9_raw_t_Volume, D9_raw_hT) * M_raw_MND)
                              + (unsigned long long)sizeof(cm_apme) * (unsigned long long)(tc_rows(D9_raw_s_Volume, D9_raw_hS) * M_raw_ND * cm_apm_q(D9_raw_qs))
                              + (unsigned long long)sizeof(cm_apme) * (unsigned long long)(tc_rows(D9_raw_f_Volume, D9_raw_hF) * M_raw_ND * cm_apm_q(D9_raw_qf))
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(D9_raw_m_Volume, D9_raw_hM) * M_raw_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(D9_raw_m_Volume, D9_raw_hM) * M_raw_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(D9_raw_n_Volume, D9_raw_hN) * M_raw_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(D9_raw_n_Volume, D9_raw_hN) * M_raw_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(D9_raw_w_Volume * M_raw_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(D9_raw_w_Volume * M_raw_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(D9_raw_s_Volume, D9_raw_hS) * M_raw_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(D9_raw_s_Volume, D9_raw_hS) * M_raw_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M_raw_ND * HS_N * TC_FIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M_raw_ND * HS_N)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(D9_raw_g_Volume * M_raw_NG);
  void D9_raw_Init( void ) {}
  void D9_raw_Quit( void ) {}
};
