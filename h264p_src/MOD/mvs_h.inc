
static const byte H_mvs_a_kind[3+1]={ 0,1,2,3 };
static const byte H_mvs_a_ma[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H_mvs_a_mb[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H_mvs_a_oth[5+1]={ 0,1,2,3,4,5 };
static const byte H_mvs_a_sp[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H_mvs_b_kind[3+1]={ 0,1,2,3 };
static const byte H_mvs_b_ps[3+1]={ 0,1,2,3 };
static const byte H_mvs_b_rf[3+1]={ 0,1,2,3 };
static const byte H_mvs_b_pi[3+1]={ 0,1,2,3 };

static const byte H_mvs_c_kind[3+1]={ 0,1,2,3 };
static const byte H_mvs_c_pd[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H_mvs_c_sp[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H_mvs_d_kind[3+1]={ 0,1,2,3 };
static const byte H_mvs_d_pd[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H_mvs_d_ma[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H_mvs_d_nr[3+1]={ 0,1,2,3 };

static const byte H_mvs_f_kind[3+1]={ 0,1,2,3 };
static const byte H_mvs_f_sp[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H_mvs_g_kind[3+1]={ 0,1,2,3 };

static const byte H_mvs_m_kind[3+1]={ 0,1,2,3 };
static const byte H_mvs_m_pd[5+1]={ 0,1,2,3,4,5 };

static const byte H_mvs_n_kind[3+1]={ 0,1,2,3 };

static const byte H_mvs_s_kind[3+1]={ 0,1,2,3 };
static const byte H_mvs_s_pd[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H_mvs_t_kind[3+1]={ 0,1,2,3 };


static const int H_mvs_rA = (64+0) * (1);
static const int H_mvs_rB = (16+0) * (1);
static const int H_mvs_rC = (64+0) * (1);
static const int H_mvs_rD = (255+0) * (1);
static const int H_mvs_rT = (55+0) * (1);
static const int H_mvs_rG = (90+0) * (1);
static const int H_mvs_rS1 = (6+0) * (1);
static const int H_mvs_rS2 = (6+0) * (1);
static const int H_mvs_lr = (12+0) * (1);
static const int H_mvs_lrm = (12+0) * (1);
static const int H_mvs_mbm = (0+0) * (1);
static const int H_mvs_mwA = (0+0) * (1);
static const int H_mvs_mwB = (0+0) * (1);
static const int H_mvs_mwC = (0+0) * (1);
static const int H_mvs_mwD = (0+0) * (1);
static const int H_mvs_mwT = (0+0) * (1);
static const int H_mvs_mwG = (0+0) * (1);
static const int H_mvs_bw = (8+0) * (1);
static const int H_mvs_qs = (16+3) * (1);
static const int H_mvs_qf = (16+3) * (1);
static const int H_mvs_mb = (5+0) * (1);
static const int H_mvs_hA = (24+0) * (1);
static const int H_mvs_hB = (24+0) * (1);
static const int H_mvs_hD = (24+0) * (1);
static const int H_mvs_hS = (24+0) * (1);
static const int H_mvs_hF = (24+0) * (1);
static const int H_mvs_hM = (24+0) * (1);
static const int H_mvs_hT = (24+0) * (1);
static const int H_mvs_hN = (24+0) * (1);


static const long long H_mvs_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 2), 8), 8), 6), 9); // 27648
static const long long H_mvs_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 2), 2), 4), 4), 4); // 1024
static const long long H_mvs_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 2), 9), 9); // 648
static const long long H_mvs_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 2), 9), 8), 4); // 2304
static const long long H_mvs_f_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 2), 9); // 72
static const long long H_mvs_g_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 1); // 4
static const long long H_mvs_m_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 2), 6); // 48
static const long long H_mvs_n_Volume = tc_vmul(1, 4); // 4
static const long long H_mvs_s_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 2), 9); // 72
static const long long H_mvs_t_Volume = tc_vmul(tc_vmul(1, 4), 2); // 8

struct H_mvs_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt H_mvs_A[tc_rows(H_mvs_a_Volume, H_mvs_hA) * H_mvs_ND];
  cm_cnt H_mvs_B[tc_rows(H_mvs_b_Volume, H_mvs_hB) * H_mvs_ND];
  cm_cnt H_mvs_C[H_mvs_c_Volume * H_mvs_ND];
  cm_cnt H_mvs_D[tc_rows(H_mvs_d_Volume, H_mvs_hD) * H_mvs_ND];
  cm_cnt H_mvs_MT[tc_rows(H_mvs_t_Volume, H_mvs_hT) * H_mvs_MND];
  i16 H_mvs_S[tc_rows(H_mvs_s_Volume, H_mvs_hS) * H_mvs_ND * H_mvs_qs];
  u8 H_mvs_SC[tc_rows(H_mvs_s_Volume, H_mvs_hS) * H_mvs_ND * H_mvs_qs];
  i16 H_mvs_F[tc_rows(H_mvs_f_Volume, H_mvs_hF) * H_mvs_ND * H_mvs_qf];
  u8 H_mvs_FC[tc_rows(H_mvs_f_Volume, H_mvs_hF) * H_mvs_ND * H_mvs_qf];
  i32 H_mvs_W[tc_rows(H_mvs_m_Volume, H_mvs_hM) * H_mvs_ND * TC_MIXN];
  u8 H_mvs_WC[tc_rows(H_mvs_m_Volume, H_mvs_hM) * H_mvs_ND];
  i32 H_mvs_WM[tc_rows(H_mvs_n_Volume, H_mvs_hN) * H_mvs_MND * 3];
  u8 H_mvs_WMC[tc_rows(H_mvs_n_Volume, H_mvs_hN) * H_mvs_MND];
  i32 H_mvs_W2[tc_rows(H_mvs_d_Volume, H_mvs_hD) * H_mvs_ND * TC_MIXN];
  u8 H_mvs_W2C[tc_rows(H_mvs_d_Volume, H_mvs_hD) * H_mvs_ND];
  i32 H_mvs_WF[H_mvs_ND * 3];
  u8 H_mvs_WFC[H_mvs_ND];
  cm_cnt H_mvs_G[H_mvs_g_Volume * H_mvs_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H_mvs_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_mvs_a_Volume, H_mvs_hA) * H_mvs_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_mvs_b_Volume, H_mvs_hB) * H_mvs_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H_mvs_c_Volume * H_mvs_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_mvs_d_Volume, H_mvs_hD) * H_mvs_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_mvs_t_Volume, H_mvs_hT) * H_mvs_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H_mvs_s_Volume, H_mvs_hS) * H_mvs_ND * H_mvs_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_mvs_s_Volume, H_mvs_hS) * H_mvs_ND * H_mvs_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H_mvs_f_Volume, H_mvs_hF) * H_mvs_ND * H_mvs_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_mvs_f_Volume, H_mvs_hF) * H_mvs_ND * H_mvs_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H_mvs_m_Volume, H_mvs_hM) * H_mvs_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_mvs_m_Volume, H_mvs_hM) * H_mvs_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H_mvs_n_Volume, H_mvs_hN) * H_mvs_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_mvs_n_Volume, H_mvs_hN) * H_mvs_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H_mvs_d_Volume, H_mvs_hD) * H_mvs_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_mvs_d_Volume, H_mvs_hD) * H_mvs_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_mvs_ND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_mvs_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H_mvs_g_Volume * H_mvs_NG);
  void H_mvs_Init( void ) {
  
  }
};
