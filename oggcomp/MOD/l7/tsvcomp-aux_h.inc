
static const byte L7_aux_a_fld[18+1]={ 0,1,2,3,4,5,6,7,8,9,9,9,10,10,11,12,13,13,13 };
static const byte L7_aux_a_p1[15+1]={ 0,1,1,1,1,1,1,1,1,2,2,2,2,3,4,5 };

static const byte L7_aux_b_fld[18+1]={ 0,1,1,1,2,2,3,4,5,6,6,6,6,7,8,8,8,8,8 };

static const byte L7_aux_c_fld[18+1]={ 0,0,0,0,0,1,2,2,2,2,3,4,5,5,5,5,5,6,6 };

static const byte L7_aux_d_fld[18+1]={ 0,0,1,1,1,2,3,4,4,4,4,4,5,5,5,5,5,5,5 };

static const byte L7_aux_f_fld[18+1]={ 0,1,2,3,3,3,4,5,5,6,6,7,8,8,8,8,9,9,9 };


static const byte L7_aux_m_fld[18+1]={ 0,1,1,1,1,2,2,3,4,4,5,6,6,6,6,7,7,7,7 };

static const byte L7_aux_n_fld[18+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18 };

static const byte L7_aux_s_fld[18+1]={ 0,0,1,1,1,1,1,1,1,1,1,2,3,3,3,3,3,4,4 };



static const int L7_aux_rA = (128+0) * (1);
static const int L7_aux_rB = (0+0) * (1);
static const int L7_aux_rC = (0+0) * (1);
static const int L7_aux_rD = (1+0) * (1);
static const int L7_aux_rT = (16+0) * (1);
static const int L7_aux_rG = (240+0) * (1);
static const int L7_aux_rS1 = (16+0) * (1);
static const int L7_aux_rS2 = (5+0) * (1);
static const int L7_aux_lr = (22+0) * (1);
static const int L7_aux_lrm = (31+0) * (1);
static const int L7_aux_mbm = (0+0) * (1);
static const int L7_aux_mwA = (0+0) * (1);
static const int L7_aux_mwB = (16+0) * (1);
static const int L7_aux_mwC = (172+0) * (1);
static const int L7_aux_mwD = (114+0) * (1);
static const int L7_aux_mwT = (1+0) * (1);
static const int L7_aux_mwG = (0+0) * (1);
static const int L7_aux_bw = (8+0) * (1);
static const int L7_aux_qs = (90+3) * (1);
static const int L7_aux_qf = (15+3) * (1);
static const int L7_aux_mb = (128+0) * (1);
static const int L7_aux_hA = (31+0) * (1);
static const int L7_aux_hB = (31+0) * (1);
static const int L7_aux_hD = (31+0) * (1);
static const int L7_aux_hS = (31+0) * (1);
static const int L7_aux_hF = (31+0) * (1);
static const int L7_aux_hM = (31+0) * (1);
static const int L7_aux_hT = (31+0) * (1);
static const int L7_aux_hN = (31+0) * (1);


static const long long L7_aux_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 14), 6), 3); // 252
static const long long L7_aux_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 9), 1), 1); // 9
static const long long L7_aux_c_Volume = tc_vmul(1, 7); // 7
static const long long L7_aux_d_Volume = tc_vmul(1, 6); // 6
static const long long L7_aux_f_Volume = tc_vmul(1, 10); // 10
static const long long L7_aux_g_Volume = tc_vmul(tc_vmul(1, 3), 2); // 6
static const long long L7_aux_m_Volume = tc_vmul(1, 8); // 8
static const long long L7_aux_n_Volume = tc_vmul(1, 19); // 19
static const long long L7_aux_s_Volume = tc_vmul(1, 5); // 5
static const long long L7_aux_t_Volume = tc_vmul(1, 3); // 3

struct L7_aux_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt L7_aux_A[tc_rows(L7_aux_a_Volume, L7_aux_hA) * L7_aux_ND];
  cm_cnt L7_aux_B[tc_rows(L7_aux_b_Volume, L7_aux_hB) * L7_aux_ND];
  cm_cnt L7_aux_C[L7_aux_c_Volume * L7_aux_ND];
  cm_cnt L7_aux_D[tc_rows(L7_aux_d_Volume, L7_aux_hD) * L7_aux_ND];
  cm_cnt L7_aux_MT[tc_rows(L7_aux_t_Volume, L7_aux_hT) * L7_aux_MND];
  i16 L7_aux_S[tc_rows(L7_aux_s_Volume, L7_aux_hS) * L7_aux_ND * L7_aux_qs];
  u8 L7_aux_SC[tc_rows(L7_aux_s_Volume, L7_aux_hS) * L7_aux_ND * L7_aux_qs];
  i16 L7_aux_F[tc_rows(L7_aux_f_Volume, L7_aux_hF) * L7_aux_ND * L7_aux_qf];
  u8 L7_aux_FC[tc_rows(L7_aux_f_Volume, L7_aux_hF) * L7_aux_ND * L7_aux_qf];
  i32 L7_aux_W[tc_rows(L7_aux_m_Volume, L7_aux_hM) * L7_aux_ND * TC_MIXN];
  u8 L7_aux_WC[tc_rows(L7_aux_m_Volume, L7_aux_hM) * L7_aux_ND];
  i32 L7_aux_WM[tc_rows(L7_aux_n_Volume, L7_aux_hN) * L7_aux_MND * 3];
  u8 L7_aux_WMC[tc_rows(L7_aux_n_Volume, L7_aux_hN) * L7_aux_MND];
  cm_cnt L7_aux_G[L7_aux_g_Volume * TC_SGN_AUX];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long L7_aux_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L7_aux_a_Volume, L7_aux_hA) * L7_aux_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L7_aux_b_Volume, L7_aux_hB) * L7_aux_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(L7_aux_c_Volume * L7_aux_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L7_aux_d_Volume, L7_aux_hD) * L7_aux_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L7_aux_t_Volume, L7_aux_hT) * L7_aux_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(L7_aux_s_Volume, L7_aux_hS) * L7_aux_ND * L7_aux_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L7_aux_s_Volume, L7_aux_hS) * L7_aux_ND * L7_aux_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(L7_aux_f_Volume, L7_aux_hF) * L7_aux_ND * L7_aux_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L7_aux_f_Volume, L7_aux_hF) * L7_aux_ND * L7_aux_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(L7_aux_m_Volume, L7_aux_hM) * L7_aux_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L7_aux_m_Volume, L7_aux_hM) * L7_aux_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(L7_aux_n_Volume, L7_aux_hN) * L7_aux_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L7_aux_n_Volume, L7_aux_hN) * L7_aux_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(L7_aux_g_Volume * TC_SGN_AUX);
  void L7_aux_Init( void ) {
  
  }
};
