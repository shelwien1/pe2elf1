



static const byte M5_raw_d_code[11+1]={ 0,1,1,2,3,4,5,6,6,6,6,6 };

static const byte M5_raw_e_code[11+1]={ 0,0,1,1,2,2,3,4,4,4,4,4 };
static const byte M5_raw_e_at[11+1]={ 0,0,1,2,2,2,3,3,3,3,4,5 };


static const byte M5_raw_h_at[11+1]={ 0,0,0,0,0,1,2,3,4,4,5,5 };

static const byte M5_raw_i_code[11+1]={ 0,1,2,2,2,2,3,3,4,4,4,4 };

static const byte M5_raw_j_at[11+1]={ 0,0,1,2,3,3,4,5,6,7,8,9 };


static const byte M5_raw_l_code[11+1]={ 0,1,1,2,3,4,5,5,6,6,6,6 };

static const byte M5_raw_m_at[11+1]={ 0,0,0,1,2,2,2,2,3,3,3,4 };




static const int M5_raw_rA = (248+0) * (1);
static const int M5_raw_mwA = (254+0) * (1);
static const int M5_raw_rB = (41+0) * (1);
static const int M5_raw_mwB = (2+0) * (1);
static const int M5_raw_rC = (0+0) * (1);
static const int M5_raw_mwC = (0+0) * (1);
static const int M5_raw_rD = (247+0) * (1);
static const int M5_raw_mwD = (254+0) * (1);
static const int M5_raw_rE = (253+0) * (1);
static const int M5_raw_mwE = (0+0) * (1);
static const int M5_raw_rH = (255+0) * (1);
static const int M5_raw_mwH = (1+0) * (1);
static const int M5_raw_rI = (230+0) * (1);
static const int M5_raw_mwI = (32+0) * (1);
static const int M5_raw_rJ = (239+0) * (1);
static const int M5_raw_mwJ = (254+0) * (1);
static const int M5_raw_rK = (140+0) * (1);
static const int M5_raw_mwK = (248+0) * (1);
static const int M5_raw_rL = (57+0) * (1);
static const int M5_raw_mwL = (124+0) * (1);
static const int M5_raw_lr = (26+0) * (1);
static const int M5_raw_mb = (22+0) * (1);
static const int M5_raw_rT = (0+0) * (1);
static const int M5_raw_mwT = (0+0) * (1);
static const int M5_raw_mt = (0+0) * (1);
static const int M5_raw_lrm = (0+0) * (1);
static const int M5_raw_mbm = (0+0) * (1);
static const int M5_raw_rG = (0+0) * (1);
static const int M5_raw_mwG = (0+0) * (1);
static const int M5_raw_hA = (22+0) * (1);
static const int M5_raw_hB = (22+0) * (1);
static const int M5_raw_hC = (20+0) * (1);
static const int M5_raw_hD = (22+0) * (1);
static const int M5_raw_hE = (22+0) * (1);
static const int M5_raw_hH = (22+0) * (1);
static const int M5_raw_hI = (22+0) * (1);
static const int M5_raw_hJ = (22+0) * (1);
static const int M5_raw_hK = (22+0) * (1);
static const int M5_raw_hL = (22+0) * (1);
static const int M5_raw_hM = (22+0) * (1);
static const int M5_raw_hT = (22+0) * (1);
static const int M5_raw_hN = (22+0) * (1);


static const long long M5_raw_a_Volume = 1; // 1
static const long long M5_raw_b_Volume = tc_vmul(1, (1<<8)); // 256
static const long long M5_raw_c_Volume = tc_vmul(tc_vmul(1, (1<<8)), (1<<6)); // 16384
static const long long M5_raw_d_Volume = tc_vmul(1, 7); // 7
static const long long M5_raw_e_Volume = tc_vmul(tc_vmul(1, 5), 6); // 30
static const long long M5_raw_g_Volume = 1; // 1
static const long long M5_raw_h_Volume = tc_vmul(tc_vmul(1, (1<<5)), 6); // 192
static const long long M5_raw_i_Volume = tc_vmul(tc_vmul(1, (1<<4)), 5); // 80
static const long long M5_raw_j_Volume = tc_vmul(1, 10); // 10
static const long long M5_raw_k_Volume = tc_vmul(1, (1<<7)); // 128
static const long long M5_raw_l_Volume = tc_vmul(tc_vmul(1, (1<<7)), 7); // 896
static const long long M5_raw_m_Volume = tc_vmul(1, 5); // 5
static const long long M5_raw_n_Volume = 1; // 1
static const long long M5_raw_t_Volume = 1; // 1

#define M5_raw_USES(X) X(c1) X(c2) X(code) X(at)

struct M5_raw_T {
  cm_cnt2 M5_raw_A[tc_rows(M5_raw_a_Volume, M5_raw_hA) * M_raw_ND];
  cm_cnt2 M5_raw_B[tc_rows(M5_raw_b_Volume, M5_raw_hB) * M_raw_ND];
  cm_cnt2 M5_raw_C[tc_rows(M5_raw_c_Volume, M5_raw_hC) * M_raw_ND];
  cm_cnt2 M5_raw_D[tc_rows(M5_raw_d_Volume, M5_raw_hD) * M_raw_ND];
  cm_cnt2 M5_raw_E[tc_rows(M5_raw_e_Volume, M5_raw_hE) * M_raw_ND];
  cm_cnt2 M5_raw_H[tc_rows(M5_raw_h_Volume, M5_raw_hH) * M_raw_ND];
  cm_cnt2 M5_raw_I[tc_rows(M5_raw_i_Volume, M5_raw_hI) * M_raw_ND];
  cm_cnt2 M5_raw_J[tc_rows(M5_raw_j_Volume, M5_raw_hJ) * M_raw_ND];
  cm_cnt2 M5_raw_K[tc_rows(M5_raw_k_Volume, M5_raw_hK) * M_raw_ND];
  cm_cnt2 M5_raw_L[tc_rows(M5_raw_l_Volume, M5_raw_hL) * M_raw_ND];
  cm_cnt M5_raw_MT[tc_rows(M5_raw_t_Volume, M5_raw_hT) * M_raw_MND];
  i32 M5_raw_W[tc_rows(M5_raw_m_Volume, M5_raw_hM) * M_raw_ND * tc_nin(10)];
  u8 M5_raw_WC[tc_rows(M5_raw_m_Volume, M5_raw_hM) * M_raw_ND];
  i32 M5_raw_WM[tc_rows(M5_raw_n_Volume, M5_raw_hN) * M_raw_MND * TC_MIN];
  u8 M5_raw_WMC[tc_rows(M5_raw_n_Volume, M5_raw_hN) * M_raw_MND];
  cm_cnt M5_raw_G[M5_raw_g_Volume * M_raw_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M5_raw_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_raw_a_Volume, M5_raw_hA) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_raw_b_Volume, M5_raw_hB) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_raw_c_Volume, M5_raw_hC) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_raw_d_Volume, M5_raw_hD) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_raw_e_Volume, M5_raw_hE) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_raw_h_Volume, M5_raw_hH) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_raw_i_Volume, M5_raw_hI) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_raw_j_Volume, M5_raw_hJ) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_raw_k_Volume, M5_raw_hK) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M5_raw_l_Volume, M5_raw_hL) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M5_raw_t_Volume, M5_raw_hT) * M_raw_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_raw_m_Volume, M5_raw_hM) * M_raw_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_raw_m_Volume, M5_raw_hM) * M_raw_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M5_raw_n_Volume, M5_raw_hN) * M_raw_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M5_raw_n_Volume, M5_raw_hN) * M_raw_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M5_raw_g_Volume * M_raw_NG);
  void M5_raw_Init( void ) {}
  void M5_raw_Quit( void ) {}
};
