
static const byte M3_esc_a_dm[3+1]={ 0,1,2,3 };

static const byte M3_esc_b_dm[3+1]={ 0,1,2,3 };

static const byte M3_esc_c_dm[3+1]={ 0,1,2,3 };

static const byte M3_esc_d_dm[3+1]={ 0,1,2,3 };




static const byte M3_esc_m_dm[3+1]={ 0,1,2,3 };




static const int M3_esc_rA = (255+0) * (1);
static const int M3_esc_mwA = (23+0) * (1);
static const int M3_esc_rB = (252+0) * (1);
static const int M3_esc_mwB = (15+0) * (1);
static const int M3_esc_rC = (234+0) * (1);
static const int M3_esc_mwC = (8+0) * (1);
static const int M3_esc_rD = (130+0) * (1);
static const int M3_esc_mwD = (32+0) * (1);
static const int M3_esc_rE = (255+0) * (1);
static const int M3_esc_mwE = (12+0) * (1);
static const int M3_esc_rH = (255+0) * (1);
static const int M3_esc_mwH = (224+0) * (1);
static const int M3_esc_lr = (24+0) * (1);
static const int M3_esc_mb = (15+0) * (1);
static const int M3_esc_rT = (0+0) * (1);
static const int M3_esc_mwT = (0+0) * (1);
static const int M3_esc_mt = (0+0) * (1);
static const int M3_esc_lrm = (0+0) * (1);
static const int M3_esc_mbm = (0+0) * (1);
static const int M3_esc_rG = (0+0) * (1);
static const int M3_esc_mwG = (0+0) * (1);
static const int M3_esc_hA = (22+0) * (1);
static const int M3_esc_hB = (22+0) * (1);
static const int M3_esc_hC = (22+0) * (1);
static const int M3_esc_hD = (22+0) * (1);
static const int M3_esc_hE = (22+0) * (1);
static const int M3_esc_hH = (22+0) * (1);
static const int M3_esc_hM = (22+0) * (1);
static const int M3_esc_hT = (22+0) * (1);
static const int M3_esc_hN = (22+0) * (1);


static const long long M3_esc_a_Volume = tc_vmul(1, 4); // 4
static const long long M3_esc_b_Volume = tc_vmul(1, 4); // 4
static const long long M3_esc_c_Volume = tc_vmul(1, 4); // 4
static const long long M3_esc_d_Volume = tc_vmul(1, 4); // 4
static const long long M3_esc_e_Volume = 1; // 1
static const long long M3_esc_g_Volume = 1; // 1
static const long long M3_esc_h_Volume = 1; // 1
static const long long M3_esc_m_Volume = tc_vmul(1, 4); // 4
static const long long M3_esc_n_Volume = 1; // 1
static const long long M3_esc_t_Volume = 1; // 1

#define M3_esc_USES(X) X(dm)

struct M3_esc_T {
  cm_cnt2 M3_esc_A[tc_rows(M3_esc_a_Volume, M3_esc_hA) * M_esc_ND];
  cm_cnt2 M3_esc_B[tc_rows(M3_esc_b_Volume, M3_esc_hB) * M_esc_ND];
  cm_cnt2 M3_esc_C[tc_rows(M3_esc_c_Volume, M3_esc_hC) * M_esc_ND];
  cm_cnt2 M3_esc_D[tc_rows(M3_esc_d_Volume, M3_esc_hD) * M_esc_ND];
  cm_cnt2 M3_esc_E[tc_rows(M3_esc_e_Volume, M3_esc_hE) * M_esc_ND];
  cm_cnt2 M3_esc_H[tc_rows(M3_esc_h_Volume, M3_esc_hH) * M_esc_ND];
  cm_cnt M3_esc_MT[tc_rows(M3_esc_t_Volume, M3_esc_hT) * M_esc_MND];
  i32 M3_esc_W[tc_rows(M3_esc_m_Volume, M3_esc_hM) * M_esc_ND * tc_nin(6)];
  u8 M3_esc_WC[tc_rows(M3_esc_m_Volume, M3_esc_hM) * M_esc_ND];
  i32 M3_esc_WM[tc_rows(M3_esc_n_Volume, M3_esc_hN) * M_esc_MND * TC_MIN];
  u8 M3_esc_WMC[tc_rows(M3_esc_n_Volume, M3_esc_hN) * M_esc_MND];
  cm_cnt M3_esc_G[M3_esc_g_Volume * M_esc_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M3_esc_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_esc_a_Volume, M3_esc_hA) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_esc_b_Volume, M3_esc_hB) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_esc_c_Volume, M3_esc_hC) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_esc_d_Volume, M3_esc_hD) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_esc_e_Volume, M3_esc_hE) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_esc_h_Volume, M3_esc_hH) * M_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M3_esc_t_Volume, M3_esc_hT) * M_esc_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_esc_m_Volume, M3_esc_hM) * M_esc_ND * tc_nin(6))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_esc_m_Volume, M3_esc_hM) * M_esc_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_esc_n_Volume, M3_esc_hN) * M_esc_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_esc_n_Volume, M3_esc_hN) * M_esc_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M3_esc_g_Volume * M_esc_NG);
  void M3_esc_Init( void ) {}
  void M3_esc_Quit( void ) {}
};
