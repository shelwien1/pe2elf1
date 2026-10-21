
static const byte H1_pcm_a_dp[6+1]={ 0,1,2,3,4,5,6 };
static const byte H1_pcm_a_pd[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

static const byte H1_pcm_b_pd[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H1_pcm_b_pd2[7+1]={ 0,1,2,3,4,5,6,7 };


static const byte H1_pcm_d_dp[6+1]={ 0,1,2,3,4,5,6 };

static const byte H1_pcm_g_pd[3+1]={ 0,1,2,3 };



static const byte H1_pcm_t_dp[6+1]={ 0,1,2,3,4,5,6 };


static const int H1_pcm_rA = (64+0) * (1);
static const int H1_pcm_rB = (16+0) * (1);
static const int H1_pcm_rC = (64+0) * (1);
static const int H1_pcm_rD = (255+0) * (1);
static const int H1_pcm_rT = (55+0) * (1);
static const int H1_pcm_rG = (90+0) * (1);
static const int H1_pcm_lr = (12+0) * (1);
static const int H1_pcm_mwA = (0+0) * (1);
static const int H1_pcm_mwB = (0+0) * (1);
static const int H1_pcm_mwC = (0+0) * (1);
static const int H1_pcm_mwD = (0+0) * (1);
static const int H1_pcm_mwT = (0+0) * (1);
static const int H1_pcm_mwG = (0+0) * (1);
static const int H1_pcm_mt = (0+0) * (1);
static const int H1_pcm_mb = (5+0) * (1);
static const int H1_pcm_lrm = (12+0) * (1);
static const int H1_pcm_mbm = (0+0) * (1);
static const int H1_pcm_hA = (24+0) * (1);
static const int H1_pcm_hB = (24+0) * (1);
static const int H1_pcm_hC = (24+0) * (1);
static const int H1_pcm_hD = (24+0) * (1);
static const int H1_pcm_hM = (24+0) * (1);
static const int H1_pcm_hT = (24+0) * (1);
static const int H1_pcm_hN = (24+0) * (1);


static const long long H1_pcm_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 7), 16); // 336
static const long long H1_pcm_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 8), 8); // 192
static const long long H1_pcm_c_Volume = tc_vmul(1, 3); // 3
static const long long H1_pcm_d_Volume = tc_vmul(1, 7); // 7
static const long long H1_pcm_g_Volume = tc_vmul(tc_vmul(1, 3), 4); // 12
static const long long H1_pcm_m_Volume = tc_vmul(1, 3); // 3
static const long long H1_pcm_n_Volume = tc_vmul(1, 3); // 3
static const long long H1_pcm_t_Volume = tc_vmul(tc_vmul(1, 3), 7); // 21

#define H1_pcm_USES(X) X(pl) X(dp) X(pd) X(pd2)

struct H1_pcm_T {
  cm_cnt2s H1_pcm_A[tc_rows(H1_pcm_a_Volume, H1_pcm_hA) * H_pcm_ND];
  cm_cnt2s H1_pcm_B[tc_rows(H1_pcm_b_Volume, H1_pcm_hB) * H_pcm_ND];
  cm_cnt2s H1_pcm_C[tc_rows(H1_pcm_c_Volume, H1_pcm_hC) * H_pcm_ND];
  cm_cnt2s H1_pcm_D[tc_rows(H1_pcm_d_Volume, H1_pcm_hD) * H_pcm_ND];
  cm_cnt H1_pcm_MT[tc_rows(H1_pcm_t_Volume, H1_pcm_hT) * H_pcm_MND];
  i32 H1_pcm_W[tc_rows(H1_pcm_m_Volume, H1_pcm_hM) * H_pcm_ND * 16];
  u8 H1_pcm_WC[tc_rows(H1_pcm_m_Volume, H1_pcm_hM) * H_pcm_ND];
  i32 H1_pcm_WM[tc_rows(H1_pcm_n_Volume, H1_pcm_hN) * H_pcm_MND * 3];
  u8 H1_pcm_WMC[tc_rows(H1_pcm_n_Volume, H1_pcm_hN) * H_pcm_MND];
  cm_cnt H1_pcm_G[H1_pcm_g_Volume * H_pcm_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H1_pcm_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_pcm_a_Volume, H1_pcm_hA) * H_pcm_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_pcm_b_Volume, H1_pcm_hB) * H_pcm_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_pcm_c_Volume, H1_pcm_hC) * H_pcm_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_pcm_d_Volume, H1_pcm_hD) * H_pcm_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H1_pcm_t_Volume, H1_pcm_hT) * H_pcm_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_pcm_m_Volume, H1_pcm_hM) * H_pcm_ND * 16)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_pcm_m_Volume, H1_pcm_hM) * H_pcm_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_pcm_n_Volume, H1_pcm_hN) * H_pcm_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_pcm_n_Volume, H1_pcm_hN) * H_pcm_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H1_pcm_g_Volume * H_pcm_NG);
  void H1_pcm_Init( void ) {}
  void H1_pcm_Quit( void ) {}
};
