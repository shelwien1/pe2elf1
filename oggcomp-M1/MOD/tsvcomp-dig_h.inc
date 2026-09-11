
static const byte TC_dig_a_pass[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte TC_dig_a_band[15+1]={ 0,1,1,1,2,2,3,3,4,4,5,6,6,6,6,6 };
static const byte TC_dig_a_t1[3+1]={ 0,1,2,3 };
static const byte TC_dig_a_cls[15+1]={ 0,0,1,1,1,1,1,2,3,4,4,4,4,4,4,4 };

static const byte TC_dig_b_band[15+1]={ 0,1,1,1,2,2,3,4,4,5,6,7,7,7,7,7 };
static const byte TC_dig_b_q1[5+1]={ 0,1,2,3,4,5 };
static const byte TC_dig_b_p0[3+1]={ 0,1,2,3 };
static const byte TC_dig_b_ax[12+1]={ 0,1,1,1,1,1,2,2,3,4,4,5,5 };
static const byte TC_dig_b_pq[32+1]={ 0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,2,3,3,4,5,5,5,5,5,5,5,5,5,5,5 };

static const byte TC_dig_c_ax[12+1]={ 0,1,2,3,3,3,3,3,3,3,4,4,4 };

static const byte TC_dig_d_ax[12+1]={ 0,0,1,1,2,2,3,3,4,4,5,5,6 };
static const byte TC_dig_d_q1s[13+1]={ 0,1,1,2,2,3,4,4,5,6,6,7,7,8 };
static const byte TC_dig_d_q2s[13+1]={ 0,0,0,0,1,1,2,2,2,3,3,3,3,3 };
static const byte TC_dig_d_p0s[13+1]={ 0,0,0,1,1,2,3,4,5,6,6,6,6,6 };
static const byte TC_dig_d_pq[32+1]={ 0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,3,3,4,4,4,4,4,4,4 };
static const byte TC_dig_d_ps[13+1]={ 0,0,0,1,1,1,1,2,2,2,2,3,3,3 };
static const byte TC_dig_d_x1s[13+1]={ 0,0,0,0,1,2,3,4,5,6,7,7,7,7 };

static const byte TC_dig_f_cls[9+1]={ 0,1,1,2,3,4,5,6,7,8 };
static const byte TC_dig_f_q1s[13+1]={ 0,1,1,1,2,2,2,2,2,2,2,3,3,3 };
static const byte TC_dig_f_p0s[13+1]={ 0,0,0,0,0,0,1,2,3,4,4,4,4,4 };
static const byte TC_dig_f_ax[12+1]={ 0,0,0,0,1,1,2,2,3,3,4,4,5 };
static const byte TC_dig_f_band[5+1]={ 0,1,1,2,2,3 };
static const byte TC_dig_f_pq[32+1]={ 0,0,0,0,0,0,0,0,0,0,0,1,1,2,2,2,2,2,2,3,4,5,5,5,5,5,5,5,5,5,5,5,5 };


static const byte TC_dig_m_cls[15+1]={ 0,1,1,1,1,1,1,2,3,4,4,5,5,6,6,6 };
static const byte TC_dig_m_pq[32+1]={ 0,0,0,0,0,0,0,0,0,0,0,1,2,3,4,5,5,6,7,8,8,9,9,9,9,10,10,10,10,10,10,10,10 };
static const byte TC_dig_m_chn[7+1]={ 0,1,1,2,2,3,3,4 };

static const byte TC_dig_n_pass[3+1]={ 0,1,2,3 };
static const byte TC_dig_n_q1[5+1]={ 0,0,1,2,3,3 };
static const byte TC_dig_n_cls[15+1]={ 0,0,1,1,2,2,2,3,3,4,4,4,5,5,6,6 };

static const byte TC_dig_s_t1s[13+1]={ 0,1,1,2,3,3,3,3,3,3,3,4,4,5 };
static const byte TC_dig_s_ax[12+1]={ 0,0,0,0,0,0,0,1,1,2,2,3,3 };

static const byte TC_dig_t_cls[15+1]={ 0,0,1,1,2,3,3,4,5,6,7,7,8,9,10,10 };
static const byte TC_dig_t_q1s[13+1]={ 0,1,2,3,4,4,5,5,5,6,6,7,8,9 };
static const byte TC_dig_t_ax[12+1]={ 0,0,0,0,1,1,2,2,2,3,3,3,3 };
static const byte TC_dig_t_chn[7+1]={ 0,1,1,2,2,3,3,4 };


static const int TC_dig_avD = (239+0) * (1);
static const int TC_dig_avA = (7+0) * (1);
static const int TC_dig_avT1 = (7+0) * (1);
static const int TC_dig_avT2 = (7+0) * (1);
static const int TC_dig_avN1 = (9+0) * (1);
static const int TC_dig_avW1 = (3+0) * (1);
static const int TC_dig_avQ1 = (0+0) * (1);
static const int TC_dig_avQ2 = (2+0) * (1);
static const int TC_dig_avX1 = (1+0) * (1);
static const int TC_dig_avC = (435+0) * (1);
static const int TC_dig_rA = (255+0) * (1);
static const int TC_dig_rB = (169+0) * (1);
static const int TC_dig_rC = (16+0) * (1);
static const int TC_dig_rD = (249+0) * (1);
static const int TC_dig_rT = (255+0) * (1);
static const int TC_dig_rS1 = (6+0) * (1);
static const int TC_dig_rS2 = (7+0) * (1);
static const int TC_dig_lr = (10+0) * (1);
static const int TC_dig_lrm = (32+0) * (1);
static const int TC_dig_mbm = (64+0) * (1);
static const int TC_dig_mwA = (64+0) * (1);
static const int TC_dig_mwB = (2+0) * (1);
static const int TC_dig_mwC = (64+0) * (1);
static const int TC_dig_mwD = (65+0) * (1);
static const int TC_dig_mwT = (0+0) * (1);
static const int TC_dig_bw = (11+0) * (1);
static const int TC_dig_qs = (16+3) * (1);
static const int TC_dig_qf = (50+3) * (1);
static const int TC_dig_mb = (70+0) * (1);


static const long long TC_dig_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 8), 7), 2), 1), 3), 4), 2), 5), 1), 2), 1), 1), 1), 1), 1), 1), 1), 3), 2), 1), 3), 3), 1), 3), 1), 2), 2), 1); // 17418240
static const long long TC_dig_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 8), 1), 6), 1), 1), 2), 1), 3), 2), 1), 1), 1), 1), 1), 1), 2), 4), 2), 2), 6), 6), 1), 2), 1), 1), 1), 1); // 3981312
static const long long TC_dig_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 3), 1), 1), 1), 1), 5), 2), 1), 1), 2), 3), 3), 1), 1), 1), 1), 1); // 1080
static const long long TC_dig_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 1), 1), 1), 1), 1), 7), 9), 4), 1), 7), 5), 4), 2), 1), 2), 1), 8); // 2257920
static const long long TC_dig_f_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 1), 9), 4), 5), 1), 6), 4), 6), 1), 2), 1), 1), 2), 1); // 311040
static const long long TC_dig_g_Volume = 1; // 1
static const long long TC_dig_m_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 1), 1), 2), 7), 2), 1), 1), 1), 1), 1), 1), 3), 1), 3), 1), 11), 1), 5), 1), 1), 1), 2); // 55440
static const long long TC_dig_n_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 4), 1), 1), 7), 1), 1), 1), 1), 1), 1), 1), 3), 3), 1), 3), 1), 1), 2), 1), 1), 1), 1); // 6048
static const long long TC_dig_s_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 1), 3), 2), 1), 1), 1), 1), 6), 2), 3), 1), 2), 2), 2), 4), 1), 3), 2), 1), 2), 1), 1); // 331776
static const long long TC_dig_t_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 3), 1), 1), 11), 1), 10), 1), 1), 1), 1), 1), 2), 1), 1), 4), 3), 1), 5), 1), 1), 1), 1); // 356400

struct TC_dig_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt TC_dig_A[TC_dig_a_Volume * TC_dig_ND];
  cm_cnt TC_dig_B[TC_dig_b_Volume * TC_dig_ND];
  cm_cnt TC_dig_C[TC_dig_c_Volume * TC_dig_ND];
  cm_cnt TC_dig_D[TC_dig_d_Volume * TC_dig_ND];
  cm_cnt TC_dig_MT[TC_dig_t_Volume * TC_dig_MND];
  i16 TC_dig_S[TC_dig_s_Volume * TC_dig_ND * TC_dig_qs];
  u8 TC_dig_SC[TC_dig_s_Volume * TC_dig_ND * TC_dig_qs];
  i16 TC_dig_F[TC_dig_f_Volume * TC_dig_ND * TC_dig_qf];
  u8 TC_dig_FC[TC_dig_f_Volume * TC_dig_ND * TC_dig_qf];
  i32 TC_dig_W[TC_dig_m_Volume * TC_dig_ND * TC_MIXN];
  u8 TC_dig_WC[TC_dig_m_Volume * TC_dig_ND];
  i32 TC_dig_WM[TC_dig_n_Volume * TC_dig_MND * 3];
  u8 TC_dig_WMC[TC_dig_n_Volume * TC_dig_MND];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long TC_dig_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_a_Volume * TC_dig_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_b_Volume * TC_dig_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_c_Volume * TC_dig_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_d_Volume * TC_dig_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_t_Volume * TC_dig_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_dig_s_Volume * TC_dig_ND * TC_dig_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_dig_s_Volume * TC_dig_ND * TC_dig_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_dig_f_Volume * TC_dig_ND * TC_dig_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_dig_f_Volume * TC_dig_ND * TC_dig_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_dig_m_Volume * TC_dig_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_dig_m_Volume * TC_dig_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_dig_n_Volume * TC_dig_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_dig_n_Volume * TC_dig_MND);
  void TC_dig_Init( void ) {
  
  }
};
