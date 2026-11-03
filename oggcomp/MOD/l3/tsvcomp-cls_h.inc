
static const byte L3_cls_a_band[15+1]={ 0,1,2,3,4,5,5,6,7,8,9,9,9,9,9,10 };
static const byte L3_cls_a_prev[7+1]={ 0,0,1,2,3,4,5,6 };

static const byte L3_cls_b_band[15+1]={ 0,1,2,2,3,3,4,5,6,7,8,8,8,8,9,9 };
static const byte L3_cls_b_prev[15+1]={ 0,1,1,1,2,3,4,4,5,5,6,7,7,7,7,8 };
static const byte L3_cls_b_t1[3+1]={ 0,1,2,3 };

static const byte L3_cls_c_t1[3+1]={ 0,1,2,3 };

static const byte L3_cls_d_prev[3+1]={ 0,1,2,3 };
static const byte L3_cls_d_pq[32+1]={ 0,0,0,1,1,1,1,2,2,2,3,3,3,3,3,3,3,3,4,5,6,7,7,8,9,10,10,10,10,10,10,10,10 };

static const byte L3_cls_f_pq[32+1]={ 0,0,0,0,0,0,0,1,1,1,1,2,3,3,4,5,5,6,6,6,7,8,8,8,9,10,10,10,11,12,13,14,14 };


static const byte L3_cls_m_pq[32+1]={ 0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };

static const byte L3_cls_n_band[3+1]={ 0,1,2,3 };
static const byte L3_cls_n_pq[32+1]={ 0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,2,2,2,3,3,3,3,4,5,6,7,8,8 };

static const byte L3_cls_s_pq[32+1]={ 0,0,0,0,0,1,2,3,4,5,5,5,5,5,6,6,6,6,7,8,8,8,8,8,8,8,8,9,9,9,9,9,10 };



static const int L3_cls_rA = (139+0) * (1);
static const int L3_cls_rB = (94+0) * (1);
static const int L3_cls_rC = (34+0) * (1);
static const int L3_cls_rD = (0+0) * (1);
static const int L3_cls_rT = (0+0) * (1);
static const int L3_cls_rS1 = (21+0) * (1);
static const int L3_cls_rS2 = (5+0) * (1);
static const int L3_cls_lr = (31+0) * (1);
static const int L3_cls_lrm = (31+0) * (1);
static const int L3_cls_mbm = (253+0) * (1);
static const int L3_cls_mwA = (0+0) * (1);
static const int L3_cls_mwB = (0+0) * (1);
static const int L3_cls_mwC = (32+0) * (1);
static const int L3_cls_mwD = (144+0) * (1);
static const int L3_cls_mwT = (123+0) * (1);
static const int L3_cls_bw = (11+0) * (1);
static const int L3_cls_qs = (75+3) * (1);
static const int L3_cls_qf = (62+3) * (1);
static const int L3_cls_mb = (18+0) * (1);
static const int L3_cls_hA = (31+0) * (1);
static const int L3_cls_hB = (31+0) * (1);
static const int L3_cls_hD = (31+0) * (1);
static const int L3_cls_hS = (31+0) * (1);
static const int L3_cls_hF = (31+0) * (1);
static const int L3_cls_hM = (31+0) * (1);
static const int L3_cls_hT = (31+0) * (1);
static const int L3_cls_hN = (31+0) * (1);


static const long long L3_cls_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 11), 7), 2), 2), 1), 3), 2); // 3696
static const long long L3_cls_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 10), 9), 4), 2), 2), 2), 3); // 8640
static const long long L3_cls_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 4), 2), 1), 3); // 216
static const long long L3_cls_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 4), 3), 2), 1), 11); // 264
static const long long L3_cls_f_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 3), 1), 1), 15); // 135
static const long long L3_cls_g_Volume = 1; // 1
static const long long L3_cls_m_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 1), 3), 1), 4); // 24
static const long long L3_cls_n_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 1), 2), 1), 9); // 72
static const long long L3_cls_s_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 1), 3), 3), 11); // 198
static const long long L3_cls_t_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 3), 3), 3), 1), 3); // 162

struct L3_cls_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt L3_cls_A[tc_rows(L3_cls_a_Volume, L3_cls_hA) * L3_cls_ND];
  cm_cnt L3_cls_B[tc_rows(L3_cls_b_Volume, L3_cls_hB) * L3_cls_ND];
  cm_cnt L3_cls_C[L3_cls_c_Volume * L3_cls_ND];
  cm_cnt L3_cls_D[tc_rows(L3_cls_d_Volume, L3_cls_hD) * L3_cls_ND];
  cm_cnt L3_cls_MT[tc_rows(L3_cls_t_Volume, L3_cls_hT) * L3_cls_MND];
  i16 L3_cls_S[tc_rows(L3_cls_s_Volume, L3_cls_hS) * L3_cls_ND * L3_cls_qs];
  u8 L3_cls_SC[tc_rows(L3_cls_s_Volume, L3_cls_hS) * L3_cls_ND * L3_cls_qs];
  i16 L3_cls_F[tc_rows(L3_cls_f_Volume, L3_cls_hF) * L3_cls_ND * L3_cls_qf];
  u8 L3_cls_FC[tc_rows(L3_cls_f_Volume, L3_cls_hF) * L3_cls_ND * L3_cls_qf];
  i32 L3_cls_W[tc_rows(L3_cls_m_Volume, L3_cls_hM) * L3_cls_ND * TC_MIXN];
  u8 L3_cls_WC[tc_rows(L3_cls_m_Volume, L3_cls_hM) * L3_cls_ND];
  i32 L3_cls_WM[tc_rows(L3_cls_n_Volume, L3_cls_hN) * L3_cls_MND * 3];
  u8 L3_cls_WMC[tc_rows(L3_cls_n_Volume, L3_cls_hN) * L3_cls_MND];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long L3_cls_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L3_cls_a_Volume, L3_cls_hA) * L3_cls_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L3_cls_b_Volume, L3_cls_hB) * L3_cls_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(L3_cls_c_Volume * L3_cls_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L3_cls_d_Volume, L3_cls_hD) * L3_cls_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L3_cls_t_Volume, L3_cls_hT) * L3_cls_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(L3_cls_s_Volume, L3_cls_hS) * L3_cls_ND * L3_cls_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L3_cls_s_Volume, L3_cls_hS) * L3_cls_ND * L3_cls_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(L3_cls_f_Volume, L3_cls_hF) * L3_cls_ND * L3_cls_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L3_cls_f_Volume, L3_cls_hF) * L3_cls_ND * L3_cls_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(L3_cls_m_Volume, L3_cls_hM) * L3_cls_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L3_cls_m_Volume, L3_cls_hM) * L3_cls_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(L3_cls_n_Volume, L3_cls_hN) * L3_cls_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L3_cls_n_Volume, L3_cls_hN) * L3_cls_MND);
  void L3_cls_Init( void ) {
  
  }
};
