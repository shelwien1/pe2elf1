
static const byte H1_ipm_a_ma[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };
static const byte H1_ipm_a_mb[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };
static const byte H1_ipm_a_mc[10+1]={ 0,1,2,3,4,4,5,6,7,8,9 };

static const byte H1_ipm_b_mh[16+1]={ 0,1,2,3,4,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte H1_ipm_b_mb[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };

static const byte H1_ipm_c_mp[9+1]={ 0,1,2,3,4,4,5,6,7,7 };
static const byte H1_ipm_c_col[9+1]={ 0,1,2,3,4,5,6,7,8,9 };







static const int H1_ipm_rA = (255+0) * (1);
static const int H1_ipm_rB = (246+0) * (1);
static const int H1_ipm_rC = (255+0) * (1);
static const int H1_ipm_rD = (254+0) * (1);
static const int H1_ipm_lr = (31+0) * (1);
static const int H1_ipm_mwA = (37+0) * (1);
static const int H1_ipm_mwB = (131+0) * (1);
static const int H1_ipm_mwC = (8+0) * (1);
static const int H1_ipm_mwD = (63+0) * (1);
static const int H1_ipm_mb = (18+0) * (1);
static const int H1_ipm_rT = (0+0) * (1);
static const int H1_ipm_mwT = (0+0) * (1);
static const int H1_ipm_mt = (0+0) * (1);
static const int H1_ipm_lrm = (0+0) * (1);
static const int H1_ipm_mbm = (0+0) * (1);
static const int H1_ipm_rG = (0+0) * (1);
static const int H1_ipm_mwG = (0+0) * (1);
static const int H1_ipm_hA = (24+0) * (1);
static const int H1_ipm_hB = (24+0) * (1);
static const int H1_ipm_hC = (24+0) * (1);
static const int H1_ipm_hD = (24+0) * (1);
static const int H1_ipm_hM = (24+0) * (1);
static const int H1_ipm_hT = (24+0) * (1);
static const int H1_ipm_hN = (24+0) * (1);


static const long long H1_ipm_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 11), 11), 10); // 2420
static const long long H1_ipm_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 16), 11); // 352
static const long long H1_ipm_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 8), 10); // 80
static const long long H1_ipm_d_Volume = tc_vmul(1, 1); // 1
static const long long H1_ipm_g_Volume = tc_vmul(1, 1); // 1
static const long long H1_ipm_m_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 2); // 8
static const long long H1_ipm_n_Volume = tc_vmul(1, 1); // 1
static const long long H1_ipm_t_Volume = tc_vmul(1, 1); // 1

#define H1_ipm_USES(X) X(k8) X(ma) X(mb) X(mc) X(mh) X(mp) X(col)

struct H1_ipm_T {
  cm_cnt2s H1_ipm_A[tc_rows(H1_ipm_a_Volume, H1_ipm_hA) * H_ipm_ND];
  cm_cnt2s H1_ipm_B[tc_rows(H1_ipm_b_Volume, H1_ipm_hB) * H_ipm_ND];
  cm_cnt2s H1_ipm_C[tc_rows(H1_ipm_c_Volume, H1_ipm_hC) * H_ipm_ND];
  cm_cnt2s H1_ipm_D[tc_rows(H1_ipm_d_Volume, H1_ipm_hD) * H_ipm_ND];
  cm_cnt H1_ipm_MT[tc_rows(H1_ipm_t_Volume, H1_ipm_hT) * H_ipm_MND];
  i32 H1_ipm_W[tc_rows(H1_ipm_m_Volume, H1_ipm_hM) * H_ipm_ND * 16];
  u8 H1_ipm_WC[tc_rows(H1_ipm_m_Volume, H1_ipm_hM) * H_ipm_ND];
  i32 H1_ipm_WM[tc_rows(H1_ipm_n_Volume, H1_ipm_hN) * H_ipm_MND * 3];
  u8 H1_ipm_WMC[tc_rows(H1_ipm_n_Volume, H1_ipm_hN) * H_ipm_MND];
  cm_cnt H1_ipm_G[H1_ipm_g_Volume * H_ipm_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H1_ipm_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_ipm_a_Volume, H1_ipm_hA) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_ipm_b_Volume, H1_ipm_hB) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_ipm_c_Volume, H1_ipm_hC) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_ipm_d_Volume, H1_ipm_hD) * H_ipm_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H1_ipm_t_Volume, H1_ipm_hT) * H_ipm_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_ipm_m_Volume, H1_ipm_hM) * H_ipm_ND * 16)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_ipm_m_Volume, H1_ipm_hM) * H_ipm_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_ipm_n_Volume, H1_ipm_hN) * H_ipm_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_ipm_n_Volume, H1_ipm_hN) * H_ipm_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H1_ipm_g_Volume * H_ipm_NG);
  void H1_ipm_Init( void ) {}
  void H1_ipm_Quit( void ) {}
};
