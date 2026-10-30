
static const byte M3_cbp_a_cnt[5+1]={ 0,1,2,3,3,4 };

static const byte M3_cbp_b_lu[8+1]={ 0,1,2,3,4,5,6,7,7 };

static const byte M3_cbp_c_frac[4+1]={ 0,1,2,3,4 };

static const byte M3_cbp_d_lu[8+1]={ 0,1,2,2,3,4,5,6,6 };



static const byte M3_cbp_h_lu[8+1]={ 0,1,2,3,4,4,5,6,7 };
static const byte M3_cbp_h_mvc[17+1]={ 0,1,2,3,4,5,5,5,6,6,6,6,6,6,7,7,8,8 };





static const int M3_cbp_rA = (255+0) * (1);
static const int M3_cbp_mwA = (6+0) * (1);
static const int M3_cbp_rB = (255+0) * (1);
static const int M3_cbp_mwB = (32+0) * (1);
static const int M3_cbp_rC = (254+0) * (1);
static const int M3_cbp_mwC = (16+0) * (1);
static const int M3_cbp_rD = (255+0) * (1);
static const int M3_cbp_mwD = (64+0) * (1);
static const int M3_cbp_rE = (128+0) * (1);
static const int M3_cbp_mwE = (64+0) * (1);
static const int M3_cbp_rH = (255+0) * (1);
static const int M3_cbp_mwH = (64+0) * (1);
static const int M3_cbp_lr = (12+0) * (1);
static const int M3_cbp_mb = (19+0) * (1);
static const int M3_cbp_rT = (0+0) * (1);
static const int M3_cbp_mwT = (0+0) * (1);
static const int M3_cbp_mt = (0+0) * (1);
static const int M3_cbp_lrm = (0+0) * (1);
static const int M3_cbp_mbm = (0+0) * (1);
static const int M3_cbp_rG = (0+0) * (1);
static const int M3_cbp_mwG = (0+0) * (1);
static const int M3_cbp_hA = (22+0) * (1);
static const int M3_cbp_hB = (22+0) * (1);
static const int M3_cbp_hC = (22+0) * (1);
static const int M3_cbp_hD = (22+0) * (1);
static const int M3_cbp_hE = (22+0) * (1);
static const int M3_cbp_hH = (22+0) * (1);
static const int M3_cbp_hM = (22+0) * (1);
static const int M3_cbp_hT = (22+0) * (1);
static const int M3_cbp_hN = (22+0) * (1);


static const long long M3_cbp_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 5); // 45
static const long long M3_cbp_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 8); // 24
static const long long M3_cbp_c_Volume = tc_vmul(tc_vmul(1, 2), 5); // 10
static const long long M3_cbp_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 7), 3); // 42
static const long long M3_cbp_e_Volume = tc_vmul(tc_vmul(1, 2), (1<<5)); // 64
static const long long M3_cbp_g_Volume = 1; // 1
static const long long M3_cbp_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 8), 9); // 216
static const long long M3_cbp_m_Volume = tc_vmul(1, 1); // 1
static const long long M3_cbp_n_Volume = 1; // 1
static const long long M3_cbp_t_Volume = 1; // 1

#define M3_cbp_USES(X) X(c3) X(rbit) X(cnt) X(lu) X(frac) X(lq) X(pat) X(mvc)

struct M3_cbp_T {
  cm_cnt2 M3_cbp_A[tc_rows(M3_cbp_a_Volume, M3_cbp_hA) * M_cbp_ND];
  cm_cnt2 M3_cbp_B[tc_rows(M3_cbp_b_Volume, M3_cbp_hB) * M_cbp_ND];
  cm_cnt2 M3_cbp_C[tc_rows(M3_cbp_c_Volume, M3_cbp_hC) * M_cbp_ND];
  cm_cnt2 M3_cbp_D[tc_rows(M3_cbp_d_Volume, M3_cbp_hD) * M_cbp_ND];
  cm_cnt2 M3_cbp_E[tc_rows(M3_cbp_e_Volume, M3_cbp_hE) * M_cbp_ND];
  cm_cnt2 M3_cbp_H[tc_rows(M3_cbp_h_Volume, M3_cbp_hH) * M_cbp_ND];
  cm_cnt M3_cbp_MT[tc_rows(M3_cbp_t_Volume, M3_cbp_hT) * M_cbp_MND];
  i32 M3_cbp_W[tc_rows(M3_cbp_m_Volume, M3_cbp_hM) * M_cbp_ND * tc_nin(6)];
  u8 M3_cbp_WC[tc_rows(M3_cbp_m_Volume, M3_cbp_hM) * M_cbp_ND];
  i32 M3_cbp_WM[tc_rows(M3_cbp_n_Volume, M3_cbp_hN) * M_cbp_MND * TC_MIN];
  u8 M3_cbp_WMC[tc_rows(M3_cbp_n_Volume, M3_cbp_hN) * M_cbp_MND];
  cm_cnt M3_cbp_G[M3_cbp_g_Volume * M_cbp_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M3_cbp_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_cbp_a_Volume, M3_cbp_hA) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_cbp_b_Volume, M3_cbp_hB) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_cbp_c_Volume, M3_cbp_hC) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_cbp_d_Volume, M3_cbp_hD) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_cbp_e_Volume, M3_cbp_hE) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_cbp_h_Volume, M3_cbp_hH) * M_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M3_cbp_t_Volume, M3_cbp_hT) * M_cbp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_cbp_m_Volume, M3_cbp_hM) * M_cbp_ND * tc_nin(6))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_cbp_m_Volume, M3_cbp_hM) * M_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_cbp_n_Volume, M3_cbp_hN) * M_cbp_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_cbp_n_Volume, M3_cbp_hN) * M_cbp_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M3_cbp_g_Volume * M_cbp_NG);
  void M3_cbp_Init( void ) {}
  void M3_cbp_Quit( void ) {}
};
