
static const byte TC_aux_a_fld[18+1]={ 0,0,0,1,2,3,4,5,6,7,7,7,7,7,8,9,10,11,12 };
static const byte TC_aux_a_p1[15+1]={ 0,1,1,1,1,1,1,1,1,2,2,2,2,3,4,5 };

static const byte TC_aux_b_fld[18+1]={ 0,0,1,1,1,2,3,3,3,4,5,6,7,7,8,8,8,8,8 };

static const byte TC_aux_c_fld[18+1]={ 0,1,1,1,1,2,3,3,3,4,4,5,6,6,7,7,7,7,7 };

static const byte TC_aux_d_fld[18+1]={ 0,0,0,0,1,1,1,1,2,2,2,2,3,3,4,4,4,4,4 };

static const byte TC_aux_f_fld[18+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18 };

static const byte TC_aux_g_p1[15+1]={ 0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,3 };
static const byte TC_aux_g_p2[13+1]={ 0,0,0,0,0,0,0,0,0,1,1,1,2,3 };

static const byte TC_aux_m_fld[18+1]={ 0,0,0,1,1,1,2,3,3,4,5,6,7,7,8,9,10,11,12 };

static const byte TC_aux_n_fld[18+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18 };

static const byte TC_aux_s_fld[18+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18 };

static const byte TC_aux_t_fld[18+1]={ 0,0,0,0,1,1,2,2,2,2,2,2,2,2,3,4,5,6,7 };


static const int TC_aux_rA = (128+0) * (1);
static const int TC_aux_rB = (0+0) * (1);
static const int TC_aux_rC = (0+0) * (1);
static const int TC_aux_rD = (0+0) * (1);
static const int TC_aux_rT = (55+0) * (1);
static const int TC_aux_rG = (90+0) * (1);
static const int TC_aux_rS1 = (16+0) * (1);
static const int TC_aux_rS2 = (6+0) * (1);
static const int TC_aux_lr = (27+0) * (1);
static const int TC_aux_lrm = (31+0) * (1);
static const int TC_aux_mbm = (0+0) * (1);
static const int TC_aux_mwA = (0+0) * (1);
static const int TC_aux_mwB = (16+0) * (1);
static const int TC_aux_mwC = (66+0) * (1);
static const int TC_aux_mwD = (6+0) * (1);
static const int TC_aux_mwT = (2+0) * (1);
static const int TC_aux_mwG = (0+0) * (1);
static const int TC_aux_bw = (6+0) * (1);
static const int TC_aux_qs = (90+3) * (1);
static const int TC_aux_qf = (65+3) * (1);
static const int TC_aux_mb = (64+0) * (1);
static const int TC_aux_hA = (31+0) * (1);
static const int TC_aux_hB = (31+0) * (1);
static const int TC_aux_hD = (31+0) * (1);
static const int TC_aux_hS = (31+0) * (1);
static const int TC_aux_hF = (31+0) * (1);
static const int TC_aux_hM = (31+0) * (1);
static const int TC_aux_hT = (31+0) * (1);
static const int TC_aux_hN = (31+0) * (1);


static const long long TC_aux_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 13), 6), 3); // 234
static const long long TC_aux_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 9), 1), 1); // 9
static const long long TC_aux_c_Volume = tc_vmul(1, 8); // 8
static const long long TC_aux_d_Volume = tc_vmul(1, 5); // 5
static const long long TC_aux_f_Volume = tc_vmul(1, 19); // 19
static const long long TC_aux_g_Volume = tc_vmul(tc_vmul(1, 4), 4); // 16
static const long long TC_aux_m_Volume = tc_vmul(1, 13); // 13
static const long long TC_aux_n_Volume = tc_vmul(1, 19); // 19
static const long long TC_aux_s_Volume = tc_vmul(1, 19); // 19
static const long long TC_aux_t_Volume = tc_vmul(1, 8); // 8

struct TC_aux_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt TC_aux_A[tc_rows(TC_aux_a_Volume, TC_aux_hA) * TC_aux_ND];
  cm_cnt TC_aux_B[tc_rows(TC_aux_b_Volume, TC_aux_hB) * TC_aux_ND];
  cm_cnt TC_aux_C[TC_aux_c_Volume * TC_aux_ND];
  cm_cnt TC_aux_D[tc_rows(TC_aux_d_Volume, TC_aux_hD) * TC_aux_ND];
  cm_cnt TC_aux_MT[tc_rows(TC_aux_t_Volume, TC_aux_hT) * TC_aux_MND];
  i16 TC_aux_S[tc_rows(TC_aux_s_Volume, TC_aux_hS) * TC_aux_ND * TC_aux_qs];
  u8 TC_aux_SC[tc_rows(TC_aux_s_Volume, TC_aux_hS) * TC_aux_ND * TC_aux_qs];
  i16 TC_aux_F[tc_rows(TC_aux_f_Volume, TC_aux_hF) * TC_aux_ND * TC_aux_qf];
  u8 TC_aux_FC[tc_rows(TC_aux_f_Volume, TC_aux_hF) * TC_aux_ND * TC_aux_qf];
  i32 TC_aux_W[tc_rows(TC_aux_m_Volume, TC_aux_hM) * TC_aux_ND * TC_MIXN];
  u8 TC_aux_WC[tc_rows(TC_aux_m_Volume, TC_aux_hM) * TC_aux_ND];
  i32 TC_aux_WM[tc_rows(TC_aux_n_Volume, TC_aux_hN) * TC_aux_MND * 3];
  u8 TC_aux_WMC[tc_rows(TC_aux_n_Volume, TC_aux_hN) * TC_aux_MND];
  cm_cnt TC_aux_G[TC_aux_g_Volume * TC_SGN_AUX];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long TC_aux_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(TC_aux_a_Volume, TC_aux_hA) * TC_aux_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(TC_aux_b_Volume, TC_aux_hB) * TC_aux_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_aux_c_Volume * TC_aux_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(TC_aux_d_Volume, TC_aux_hD) * TC_aux_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(TC_aux_t_Volume, TC_aux_hT) * TC_aux_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(TC_aux_s_Volume, TC_aux_hS) * TC_aux_ND * TC_aux_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(TC_aux_s_Volume, TC_aux_hS) * TC_aux_ND * TC_aux_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(TC_aux_f_Volume, TC_aux_hF) * TC_aux_ND * TC_aux_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(TC_aux_f_Volume, TC_aux_hF) * TC_aux_ND * TC_aux_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(TC_aux_m_Volume, TC_aux_hM) * TC_aux_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(TC_aux_m_Volume, TC_aux_hM) * TC_aux_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(TC_aux_n_Volume, TC_aux_hN) * TC_aux_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(TC_aux_n_Volume, TC_aux_hN) * TC_aux_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_aux_g_Volume * TC_SGN_AUX);
  void TC_aux_Init( void ) {
  
  }
};
