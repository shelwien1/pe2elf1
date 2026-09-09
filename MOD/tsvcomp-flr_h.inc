
static const byte TC_flr_a_col[48+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,28,28,28,28,28,29,29,29,30,31,32,32,32,32,32,32,32,32,32,33 };

static const byte TC_flr_b_p1[14+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,12,12 };
static const byte TC_flr_b_lov[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte TC_flr_b_ep[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };

static const byte TC_flr_c_ep[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };

static const byte TC_flr_d_ep[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };

static const byte TC_flr_f_ep[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };


static const byte TC_flr_m_col[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte TC_flr_m_pri[32+1]={ 0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8 };

static const byte TC_flr_n_col[48+1]={ 0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,19,20,21,22,23,24,25,25,25,25,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26 };


static const byte TC_flr_t_col[48+1]={ 0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,19,20,21,22,23,24,25,25,25,25,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26 };


static const int TC_flr_rA = (15+0) * (1);
static const int TC_flr_rB = (75+0) * (1);
static const int TC_flr_rC = (63+0) * (1);
static const int TC_flr_rD = (63+0) * (1);
static const int TC_flr_rT = (15+0) * (1);
static const int TC_flr_rG = (15+0) * (1);
static const int TC_flr_rS1 = (6+0) * (1);
static const int TC_flr_rS2 = (6+0) * (1);
static const int TC_flr_lr = (22+0) * (1);
static const int TC_flr_lrm = (22+0) * (1);
static const int TC_flr_mbm = (0+0) * (1);
static const int TC_flr_mwA = (0+0) * (1);
static const int TC_flr_mwB = (0+0) * (1);
static const int TC_flr_mwC = (0+0) * (1);
static const int TC_flr_mwD = (0+0) * (1);
static const int TC_flr_mwT = (0+0) * (1);
static const int TC_flr_mwG = (0+0) * (1);
static const int TC_flr_bw = (12+0) * (1);
static const int TC_flr_qs = (30+3) * (1);
static const int TC_flr_qf = (62+3) * (1);
static const int TC_flr_mb = (0+0) * (1);


static const long long TC_flr_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 34), 1), 2), 1), 1), 1), 1), 1), 1), 1), 3), 1), 1), 1), 1); // 408
static const long long TC_flr_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 13), 2), 1), 2), 1), 1), 1), 1), 1), 1), 9), 1), 1), 12); // 11232
static const long long TC_flr_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 8), 1), 1), 8), 1), 1), 1), 1), 1), 1), 1), 1), 12); // 768
static const long long TC_flr_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 1), 1), 1), 8), 1), 1), 1), 1), 1), 1), 1), 12); // 96
static const long long TC_flr_f_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 1), 1), 1), 1), 1), 1), 1), 3), 1), 1), 1), 12); // 36
static const long long TC_flr_g_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 1), 1), 1), 1), 1), 1), 1), 1), 1), 1), 1), 1); // 1
static const long long TC_flr_m_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 16), 2), 1), 1), 1), 1), 1), 1), 1), 1), 1), 9), 1); // 288
static const long long TC_flr_n_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 27), 1), 1), 1), 1), 1), 1), 1), 1), 1), 1), 1), 1); // 27
static const long long TC_flr_s_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 6), 1), 3), 1), 1), 1), 1), 1), 1), 1), 1), 1), 1); // 18
static const long long TC_flr_t_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 27), 1), 1), 1), 1), 1), 1), 1), 1), 1), 1), 1), 1); // 27

struct TC_flr_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt TC_flr_A[TC_flr_a_Volume * TC_flr_ND];
  cm_cnt TC_flr_B[TC_flr_b_Volume * TC_flr_ND];
  cm_cnt TC_flr_C[TC_flr_c_Volume * TC_flr_ND];
  cm_cnt TC_flr_D[TC_flr_d_Volume * TC_flr_ND];
  cm_cnt TC_flr_MT[TC_flr_t_Volume * TC_flr_MND];
  i16 TC_flr_S[TC_flr_s_Volume * TC_flr_ND * TC_flr_qs];
  u8 TC_flr_SC[TC_flr_s_Volume * TC_flr_ND * TC_flr_qs];
  i16 TC_flr_F[TC_flr_f_Volume * TC_flr_ND * TC_flr_qf];
  u8 TC_flr_FC[TC_flr_f_Volume * TC_flr_ND * TC_flr_qf];
  i32 TC_flr_W[TC_flr_m_Volume * TC_flr_ND * TC_MIXN];
  u8 TC_flr_WC[TC_flr_m_Volume * TC_flr_ND];
  i32 TC_flr_WM[TC_flr_n_Volume * TC_flr_MND * 3];
  u8 TC_flr_WMC[TC_flr_n_Volume * TC_flr_MND];
  cm_cnt TC_flr_G[TC_flr_g_Volume * TC_SGN_FLR];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long TC_flr_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_flr_a_Volume * TC_flr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_flr_b_Volume * TC_flr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_flr_c_Volume * TC_flr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_flr_d_Volume * TC_flr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_flr_t_Volume * TC_flr_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_flr_s_Volume * TC_flr_ND * TC_flr_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_flr_s_Volume * TC_flr_ND * TC_flr_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_flr_f_Volume * TC_flr_ND * TC_flr_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_flr_f_Volume * TC_flr_ND * TC_flr_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_flr_m_Volume * TC_flr_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_flr_m_Volume * TC_flr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_flr_n_Volume * TC_flr_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_flr_n_Volume * TC_flr_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_flr_g_Volume * TC_SGN_FLR);
  void TC_flr_Init( void ) {
  
  }
  void TC_flr_Quit( void ) {
  }
};
