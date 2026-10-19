
static const byte H1_skp_a_c9[9+1]={ 0,1,2,2,3,4,5,6,7,8 };

static const byte H1_skp_b_cnt[15+1]={ 0,0,0,0,0,0,0,0,1,2,2,3,4,5,6,7 };

static const byte H1_skp_c_c9[9+1]={ 0,0,1,1,1,1,1,2,2,3 };
static const byte H1_skp_c_run[9+1]={ 0,0,1,2,2,3,4,4,4,5 };







static const int H1_skp_rA = (255+0) * (1);
static const int H1_skp_rB = (255+0) * (1);
static const int H1_skp_rC = (254+0) * (1);
static const int H1_skp_rD = (254+0) * (1);
static const int H1_skp_rT = (0+0) * (1);
static const int H1_skp_rG = (0+0) * (1);
static const int H1_skp_lr = (13+0) * (1);
static const int H1_skp_lrm = (0+0) * (1);
static const int H1_skp_mbm = (0+0) * (1);
static const int H1_skp_mwA = (6+0) * (1);
static const int H1_skp_mwB = (32+0) * (1);
static const int H1_skp_mwC = (6+0) * (1);
static const int H1_skp_mwD = (31+0) * (1);
static const int H1_skp_mwT = (0+0) * (1);
static const int H1_skp_mwG = (0+0) * (1);
static const int H1_skp_mt = (0+0) * (1);
static const int H1_skp_mb = (64+0) * (1);
static const int H1_skp_hA = (24+0) * (1);
static const int H1_skp_hB = (24+0) * (1);
static const int H1_skp_hC = (24+0) * (1);
static const int H1_skp_hD = (24+0) * (1);
static const int H1_skp_hM = (24+0) * (1);
static const int H1_skp_hT = (24+0) * (1);
static const int H1_skp_hN = (24+0) * (1);


static const long long H1_skp_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 3), 2), 9), 2), 1), 1); // 648
static const long long H1_skp_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 3), 2), 3), 8); // 288
static const long long H1_skp_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 4), 6); // 48
static const long long H1_skp_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 1), 3); // 3
static const long long H1_skp_g_Volume = tc_vmul(1, 1); // 1
static const long long H1_skp_m_Volume = tc_vmul(tc_vmul(1, 3), 2); // 6
static const long long H1_skp_n_Volume = tc_vmul(1, 1); // 1
static const long long H1_skp_t_Volume = tc_vmul(1, 1); // 1

#define H1_skp_USES(X) X(st) X(a3) X(b3) X(col) X(c9) X(eq) X(cd) X(cnt) X(run)

struct H1_skp_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H1_skp_A[tc_rows(H1_skp_a_Volume, H1_skp_hA) * H_skp_ND];
  cm_cnt2 H1_skp_B[tc_rows(H1_skp_b_Volume, H1_skp_hB) * H_skp_ND];
  cm_cnt2 H1_skp_C[tc_rows(H1_skp_c_Volume, H1_skp_hC) * H_skp_ND];
  cm_cnt2 H1_skp_D[tc_rows(H1_skp_d_Volume, H1_skp_hD) * H_skp_ND];
  cm_cnt H1_skp_MT[tc_rows(H1_skp_t_Volume, H1_skp_hT) * H_skp_MND];
  i32 H1_skp_W[tc_rows(H1_skp_m_Volume, H1_skp_hM) * H_skp_ND * 16];
  u8 H1_skp_WC[tc_rows(H1_skp_m_Volume, H1_skp_hM) * H_skp_ND];
  i32 H1_skp_WM[tc_rows(H1_skp_n_Volume, H1_skp_hN) * H_skp_MND * 3];
  u8 H1_skp_WMC[tc_rows(H1_skp_n_Volume, H1_skp_hN) * H_skp_MND];
  cm_cnt H1_skp_G[H1_skp_g_Volume * H_skp_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H1_skp_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_skp_a_Volume, H1_skp_hA) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_skp_b_Volume, H1_skp_hB) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_skp_c_Volume, H1_skp_hC) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_skp_d_Volume, H1_skp_hD) * H_skp_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H1_skp_t_Volume, H1_skp_hT) * H_skp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_skp_m_Volume, H1_skp_hM) * H_skp_ND * 16)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_skp_m_Volume, H1_skp_hM) * H_skp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_skp_n_Volume, H1_skp_hN) * H_skp_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_skp_n_Volume, H1_skp_hN) * H_skp_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H1_skp_g_Volume * H_skp_NG);
  void H1_skp_Init( void ) {
  
  }
};
