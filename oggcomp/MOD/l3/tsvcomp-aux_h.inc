
static const byte L3_aux_a_fld[18+1]={ 0,1,1,2,2,3,4,5,5,5,6,7,8,9,10,11,11,11,11 };
static const byte L3_aux_a_p1[15+1]={ 0,1,1,1,1,1,1,1,1,2,2,2,2,3,4,5 };

static const byte L3_aux_b_fld[18+1]={ 0,0,1,2,2,2,3,3,3,4,4,4,4,5,6,6,6,6,6 };


static const byte L3_aux_d_fld[18+1]={ 0,0,0,0,1,1,1,1,2,2,2,2,3,3,4,4,4,4,4 };

static const byte L3_aux_f_fld[18+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18 };

static const byte L3_aux_g_p1[15+1]={ 0,0,0,0,0,0,0,1,1,1,1,2,2,3,4,4 };
static const byte L3_aux_g_p2[13+1]={ 0,0,0,0,0,0,1,2,3,3,3,3,3,4 };

static const byte L3_aux_m_fld[18+1]={ 0,0,1,2,2,2,2,2,2,3,4,5,5,6,6,7,7,7,7 };

static const byte L3_aux_n_fld[18+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18 };

static const byte L3_aux_s_fld[18+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18 };

static const byte L3_aux_t_fld[18+1]={ 0,0,0,0,1,2,3,3,3,3,3,4,5,5,6,6,7,8,9 };


static const int L3_aux_rA = (140+0) * (1);
static const int L3_aux_rB = (0+0) * (1);
static const int L3_aux_rC = (0+0) * (1);
static const int L3_aux_rD = (0+0) * (1);
static const int L3_aux_rT = (13+0) * (1);
static const int L3_aux_rG = (110+0) * (1);
static const int L3_aux_rS1 = (16+0) * (1);
static const int L3_aux_rS2 = (7+0) * (1);
static const int L3_aux_lr = (27+0) * (1);
static const int L3_aux_lrm = (31+0) * (1);
static const int L3_aux_mbm = (0+0) * (1);
static const int L3_aux_mwA = (0+0) * (1);
static const int L3_aux_mwB = (16+0) * (1);
static const int L3_aux_mwC = (66+0) * (1);
static const int L3_aux_mwD = (65+0) * (1);
static const int L3_aux_mwT = (2+0) * (1);
static const int L3_aux_mwG = (0+0) * (1);
static const int L3_aux_bw = (8+0) * (1);
static const int L3_aux_qs = (28+3) * (1);
static const int L3_aux_qf = (65+3) * (1);
static const int L3_aux_mb = (64+0) * (1);
static const int L3_aux_hA = (31+0) * (1);
static const int L3_aux_hB = (31+0) * (1);
static const int L3_aux_hD = (31+0) * (1);
static const int L3_aux_hS = (31+0) * (1);
static const int L3_aux_hF = (31+0) * (1);
static const int L3_aux_hM = (31+0) * (1);
static const int L3_aux_hT = (31+0) * (1);
static const int L3_aux_hN = (31+0) * (1);


static const long long L3_aux_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 12), 6), 3); // 216
static const long long L3_aux_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 7), 1), 1); // 7
static const long long L3_aux_c_Volume = tc_vmul(1, 3); // 3
static const long long L3_aux_d_Volume = tc_vmul(1, 5); // 5
static const long long L3_aux_f_Volume = tc_vmul(1, 19); // 19
static const long long L3_aux_g_Volume = tc_vmul(tc_vmul(1, 5), 5); // 25
static const long long L3_aux_m_Volume = tc_vmul(1, 8); // 8
static const long long L3_aux_n_Volume = tc_vmul(1, 19); // 19
static const long long L3_aux_s_Volume = tc_vmul(1, 19); // 19
static const long long L3_aux_t_Volume = tc_vmul(1, 10); // 10

struct L3_aux_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt L3_aux_A[tc_rows(L3_aux_a_Volume, L3_aux_hA) * L3_aux_ND];
  cm_cnt L3_aux_B[tc_rows(L3_aux_b_Volume, L3_aux_hB) * L3_aux_ND];
  cm_cnt L3_aux_C[L3_aux_c_Volume * L3_aux_ND];
  cm_cnt L3_aux_D[tc_rows(L3_aux_d_Volume, L3_aux_hD) * L3_aux_ND];
  cm_cnt L3_aux_MT[tc_rows(L3_aux_t_Volume, L3_aux_hT) * L3_aux_MND];
  i16 L3_aux_S[tc_rows(L3_aux_s_Volume, L3_aux_hS) * L3_aux_ND * L3_aux_qs];
  u8 L3_aux_SC[tc_rows(L3_aux_s_Volume, L3_aux_hS) * L3_aux_ND * L3_aux_qs];
  i16 L3_aux_F[tc_rows(L3_aux_f_Volume, L3_aux_hF) * L3_aux_ND * L3_aux_qf];
  u8 L3_aux_FC[tc_rows(L3_aux_f_Volume, L3_aux_hF) * L3_aux_ND * L3_aux_qf];
  i32 L3_aux_W[tc_rows(L3_aux_m_Volume, L3_aux_hM) * L3_aux_ND * TC_MIXN];
  u8 L3_aux_WC[tc_rows(L3_aux_m_Volume, L3_aux_hM) * L3_aux_ND];
  i32 L3_aux_WM[tc_rows(L3_aux_n_Volume, L3_aux_hN) * L3_aux_MND * 3];
  u8 L3_aux_WMC[tc_rows(L3_aux_n_Volume, L3_aux_hN) * L3_aux_MND];
  cm_cnt L3_aux_G[L3_aux_g_Volume * TC_SGN_AUX];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long L3_aux_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L3_aux_a_Volume, L3_aux_hA) * L3_aux_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L3_aux_b_Volume, L3_aux_hB) * L3_aux_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(L3_aux_c_Volume * L3_aux_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L3_aux_d_Volume, L3_aux_hD) * L3_aux_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L3_aux_t_Volume, L3_aux_hT) * L3_aux_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(L3_aux_s_Volume, L3_aux_hS) * L3_aux_ND * L3_aux_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L3_aux_s_Volume, L3_aux_hS) * L3_aux_ND * L3_aux_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(L3_aux_f_Volume, L3_aux_hF) * L3_aux_ND * L3_aux_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L3_aux_f_Volume, L3_aux_hF) * L3_aux_ND * L3_aux_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(L3_aux_m_Volume, L3_aux_hM) * L3_aux_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L3_aux_m_Volume, L3_aux_hM) * L3_aux_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(L3_aux_n_Volume, L3_aux_hN) * L3_aux_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L3_aux_n_Volume, L3_aux_hN) * L3_aux_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(L3_aux_g_Volume * TC_SGN_AUX);
  void L3_aux_Init( void ) {
  
  }
};
