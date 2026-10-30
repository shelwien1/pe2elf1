
static const byte M1_cbp_a_cnt[5+1]={ 0,1,2,3,4,5 };

static const byte M1_cbp_b_lu[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte M1_cbp_c_frac[4+1]={ 0,1,2,3,4 };

static const byte M1_cbp_d_lu[8+1]={ 0,1,2,3,4,5,6,7,7 };






static const int M1_cbp_rA = (254+0) * (1);
static const int M1_cbp_mwA = (7+0) * (1);
static const int M1_cbp_rB = (255+0) * (1);
static const int M1_cbp_mwB = (32+0) * (1);
static const int M1_cbp_rC = (255+0) * (1);
static const int M1_cbp_mwC = (16+0) * (1);
static const int M1_cbp_rD = (255+0) * (1);
static const int M1_cbp_mwD = (64+0) * (1);
static const int M1_cbp_lr = (12+0) * (1);
static const int M1_cbp_mb = (31+0) * (1);
static const int M1_cbp_rT = (0+0) * (1);
static const int M1_cbp_mwT = (0+0) * (1);
static const int M1_cbp_mt = (0+0) * (1);
static const int M1_cbp_lrm = (0+0) * (1);
static const int M1_cbp_mbm = (0+0) * (1);
static const int M1_cbp_rG = (0+0) * (1);
static const int M1_cbp_mwG = (0+0) * (1);
static const int M1_cbp_hA = (22+0) * (1);
static const int M1_cbp_hB = (22+0) * (1);
static const int M1_cbp_hC = (22+0) * (1);
static const int M1_cbp_hD = (22+0) * (1);
static const int M1_cbp_hM = (22+0) * (1);
static const int M1_cbp_hT = (22+0) * (1);
static const int M1_cbp_hN = (22+0) * (1);


static const long long M1_cbp_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 6); // 54
static const long long M1_cbp_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 9); // 18
static const long long M1_cbp_c_Volume = tc_vmul(tc_vmul(1, 2), 5); // 10
static const long long M1_cbp_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 8), 3); // 72
static const long long M1_cbp_g_Volume = 1; // 1
static const long long M1_cbp_m_Volume = tc_vmul(1, 1); // 1
static const long long M1_cbp_n_Volume = 1; // 1
static const long long M1_cbp_t_Volume = 1; // 1

#define M1_cbp_USES(X) X(c3) X(rbit) X(cnt) X(lu) X(frac) X(lq)

struct M1_cbp_T {
  cm_cnt2s M1_cbp_A[tc_rows(M1_cbp_a_Volume, M1_cbp_hA) * M_cbp_ND];
  cm_cnt2s M1_cbp_B[tc_rows(M1_cbp_b_Volume, M1_cbp_hB) * M_cbp_ND];
  cm_cnt2s M1_cbp_C[tc_rows(M1_cbp_c_Volume, M1_cbp_hC) * M_cbp_ND];
  cm_cnt2s M1_cbp_D[tc_rows(M1_cbp_d_Volume, M1_cbp_hD) * M_cbp_ND];
  cm_cnt M1_cbp_MT[tc_rows(M1_cbp_t_Volume, M1_cbp_hT) * M_cbp_MND];
  i32 M1_cbp_W[tc_rows(M1_cbp_m_Volume, M1_cbp_hM) * M_cbp_ND * tc_nin(4)];
  u8 M1_cbp_WC[tc_rows(M1_cbp_m_Volume, M1_cbp_hM) * M_cbp_ND];
  i32 M1_cbp_WM[tc_rows(M1_cbp_n_Volume, M1_cbp_hN) * M_cbp_MND * TC_MIN];
  u8 M1_cbp_WMC[tc_rows(M1_cbp_n_Volume, M1_cbp_hN) * M_cbp_MND];
  cm_cnt M1_cbp_G[M1_cbp_g_Volume * M_cbp_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M1_cbp_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_cbp_a_Volume, M1_cbp_hA) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_cbp_b_Volume, M1_cbp_hB) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_cbp_c_Volume, M1_cbp_hC) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_cbp_d_Volume, M1_cbp_hD) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M1_cbp_t_Volume, M1_cbp_hT) * M_cbp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M1_cbp_m_Volume, M1_cbp_hM) * M_cbp_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M1_cbp_m_Volume, M1_cbp_hM) * M_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M1_cbp_n_Volume, M1_cbp_hN) * M_cbp_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M1_cbp_n_Volume, M1_cbp_hN) * M_cbp_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M1_cbp_g_Volume * M_cbp_NG);
  void M1_cbp_Init( void ) {}
  void M1_cbp_Quit( void ) {}
};
