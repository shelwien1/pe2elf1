

static const byte H1_mvg_b_mag[7+1]={ 0,0,1,1,2,2,3,4 };








static const int H1_mvg_rA = (192+0) * (1);
static const int H1_mvg_rB = (254+0) * (1);
static const int H1_mvg_rC = (64+0) * (1);
static const int H1_mvg_rD = (0+0) * (1);
static const int H1_mvg_lr = (15+0) * (1);
static const int H1_mvg_mwA = (31+0) * (1);
static const int H1_mvg_mwB = (23+0) * (1);
static const int H1_mvg_mwC = (10+0) * (1);
static const int H1_mvg_mwD = (126+0) * (1);
static const int H1_mvg_mb = (18+0) * (1);
static const int H1_mvg_rT = (0+0) * (1);
static const int H1_mvg_mwT = (0+0) * (1);
static const int H1_mvg_mt = (0+0) * (1);
static const int H1_mvg_lrm = (0+0) * (1);
static const int H1_mvg_mbm = (0+0) * (1);
static const int H1_mvg_rG = (0+0) * (1);
static const int H1_mvg_mwG = (0+0) * (1);
static const int H1_mvg_hA = (24+0) * (1);
static const int H1_mvg_hB = (24+0) * (1);
static const int H1_mvg_hC = (24+0) * (1);
static const int H1_mvg_hD = (24+0) * (1);
static const int H1_mvg_hM = (24+0) * (1);
static const int H1_mvg_hT = (24+0) * (1);
static const int H1_mvg_hN = (24+0) * (1);


static const long long H1_mvg_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 2); // 8
static const long long H1_mvg_b_Volume = tc_vmul(tc_vmul(1, 5), 2); // 10
static const long long H1_mvg_c_Volume = 1; // 1
static const long long H1_mvg_d_Volume = tc_vmul(1, 3); // 3
static const long long H1_mvg_g_Volume = tc_vmul(1, 1); // 1
static const long long H1_mvg_m_Volume = 1; // 1
static const long long H1_mvg_n_Volume = tc_vmul(1, 1); // 1
static const long long H1_mvg_t_Volume = tc_vmul(1, 1); // 1

#define H1_mvg_USES(X) X(comp) X(lst) X(ox) X(mag)

struct H1_mvg_T {
  cm_cnt2s H1_mvg_A[tc_rows(H1_mvg_a_Volume, H1_mvg_hA) * H_mvg_ND];
  cm_cnt2s H1_mvg_B[tc_rows(H1_mvg_b_Volume, H1_mvg_hB) * H_mvg_ND];
  cm_cnt2s H1_mvg_C[tc_rows(H1_mvg_c_Volume, H1_mvg_hC) * H_mvg_ND];
  cm_cnt2s H1_mvg_D[tc_rows(H1_mvg_d_Volume, H1_mvg_hD) * H_mvg_ND];
  cm_cnt H1_mvg_MT[tc_rows(H1_mvg_t_Volume, H1_mvg_hT) * H_mvg_MND];
  i32 H1_mvg_W[tc_rows(H1_mvg_m_Volume, H1_mvg_hM) * H_mvg_ND * 16];
  u8 H1_mvg_WC[tc_rows(H1_mvg_m_Volume, H1_mvg_hM) * H_mvg_ND];
  i32 H1_mvg_WM[tc_rows(H1_mvg_n_Volume, H1_mvg_hN) * H_mvg_MND * 3];
  u8 H1_mvg_WMC[tc_rows(H1_mvg_n_Volume, H1_mvg_hN) * H_mvg_MND];
  cm_cnt H1_mvg_G[H1_mvg_g_Volume * H_mvg_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H1_mvg_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_mvg_a_Volume, H1_mvg_hA) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_mvg_b_Volume, H1_mvg_hB) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_mvg_c_Volume, H1_mvg_hC) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_mvg_d_Volume, H1_mvg_hD) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H1_mvg_t_Volume, H1_mvg_hT) * H_mvg_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_mvg_m_Volume, H1_mvg_hM) * H_mvg_ND * 16)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_mvg_m_Volume, H1_mvg_hM) * H_mvg_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_mvg_n_Volume, H1_mvg_hN) * H_mvg_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_mvg_n_Volume, H1_mvg_hN) * H_mvg_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H1_mvg_g_Volume * H_mvg_NG);
  void H1_mvg_Init( void ) {}
  void H1_mvg_Quit( void ) {}
};
