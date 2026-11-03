
static const byte L7_cls_a_band[15+1]={ 0,0,1,1,1,1,2,3,4,4,5,6,7,7,7,7 };
static const byte L7_cls_a_prev[7+1]={ 0,1,1,2,3,3,3,4 };
static const byte L7_cls_a_t1[7+1]={ 0,1,1,1,2,3,4,5 };

static const byte L7_cls_b_band[15+1]={ 0,1,1,1,1,2,3,4,5,6,7,8,8,8,8,8 };
static const byte L7_cls_b_prev[15+1]={ 0,0,1,1,1,2,3,3,3,3,3,3,3,3,3,3 };
static const byte L7_cls_b_pq[32+1]={ 0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,2,2,3,3,3,4,5,5,5,5,5 };

static const byte L7_cls_c_band[3+1]={ 0,1,2,3 };
static const byte L7_cls_c_t1[3+1]={ 0,1,2,3 };
static const byte L7_cls_c_pq[32+1]={ 0,0,0,0,0,0,0,0,0,1,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4 };

static const byte L7_cls_d_band[3+1]={ 0,1,2,3 };
static const byte L7_cls_d_pq[32+1]={ 0,0,0,0,0,0,0,0,0,0,1,1,1,2,3,3,3,3,4,5,6,6,6,6,6,6,6,6,6,6,6,6,6 };



static const byte L7_cls_m_pq[32+1]={ 0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3 };

static const byte L7_cls_n_band[3+1]={ 0,1,2,3 };
static const byte L7_cls_n_pq[32+1]={ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,2,3,4,4,4,5,6,7,8,9,9 };

static const byte L7_cls_s_t1[3+1]={ 0,1,2,3 };
static const byte L7_cls_s_pq[32+1]={ 0,0,0,0,0,0,0,0,1,1,1,1,1,2,2,2,2,2,2,2,3,4,4,4,4,5,5,5,5,6,6,6,6 };



static const int L7_cls_rA = (106+0) * (1);
static const int L7_cls_rB = (20+0) * (1);
static const int L7_cls_rC = (17+0) * (1);
static const int L7_cls_rD = (0+0) * (1);
static const int L7_cls_rT = (0+0) * (1);
static const int L7_cls_rS1 = (20+0) * (1);
static const int L7_cls_rS2 = (5+0) * (1);
static const int L7_cls_lr = (23+0) * (1);
static const int L7_cls_lrm = (31+0) * (1);
static const int L7_cls_mbm = (199+0) * (1);
static const int L7_cls_mwA = (0+0) * (1);
static const int L7_cls_mwB = (0+0) * (1);
static const int L7_cls_mwC = (1+0) * (1);
static const int L7_cls_mwD = (39+0) * (1);
static const int L7_cls_mwT = (130+0) * (1);
static const int L7_cls_bw = (16+0) * (1);
static const int L7_cls_qs = (5+3) * (1);
static const int L7_cls_qf = (62+3) * (1);
static const int L7_cls_mb = (36+0) * (1);
static const int L7_cls_hA = (31+0) * (1);
static const int L7_cls_hB = (31+0) * (1);
static const int L7_cls_hD = (31+0) * (1);
static const int L7_cls_hS = (31+0) * (1);
static const int L7_cls_hF = (31+0) * (1);
static const int L7_cls_hM = (31+0) * (1);
static const int L7_cls_hT = (31+0) * (1);
static const int L7_cls_hN = (31+0) * (1);


static const long long L7_cls_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 8), 5), 6), 2), 1), 3), 2); // 5760
static const long long L7_cls_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 9), 4), 1), 2), 1), 1), 6); // 432
static const long long L7_cls_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 4), 3), 1), 5); // 240
static const long long L7_cls_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 3), 3), 2), 3), 7); // 1512
static const long long L7_cls_f_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 1), 1), 1), 1), 1); // 1
static const long long L7_cls_g_Volume = 1; // 1
static const long long L7_cls_m_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 1), 3), 1), 4); // 24
static const long long L7_cls_n_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 3), 2), 3), 1), 10); // 720
static const long long L7_cls_s_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 1), 4), 2), 3), 7); // 336
static const long long L7_cls_t_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 2), 3), 1), 2); // 48

struct L7_cls_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt L7_cls_A[tc_rows(L7_cls_a_Volume, L7_cls_hA) * L7_cls_ND];
  cm_cnt L7_cls_B[tc_rows(L7_cls_b_Volume, L7_cls_hB) * L7_cls_ND];
  cm_cnt L7_cls_C[L7_cls_c_Volume * L7_cls_ND];
  cm_cnt L7_cls_D[tc_rows(L7_cls_d_Volume, L7_cls_hD) * L7_cls_ND];
  cm_cnt L7_cls_MT[tc_rows(L7_cls_t_Volume, L7_cls_hT) * L7_cls_MND];
  i16 L7_cls_S[tc_rows(L7_cls_s_Volume, L7_cls_hS) * L7_cls_ND * L7_cls_qs];
  u8 L7_cls_SC[tc_rows(L7_cls_s_Volume, L7_cls_hS) * L7_cls_ND * L7_cls_qs];
  i16 L7_cls_F[tc_rows(L7_cls_f_Volume, L7_cls_hF) * L7_cls_ND * L7_cls_qf];
  u8 L7_cls_FC[tc_rows(L7_cls_f_Volume, L7_cls_hF) * L7_cls_ND * L7_cls_qf];
  i32 L7_cls_W[tc_rows(L7_cls_m_Volume, L7_cls_hM) * L7_cls_ND * TC_MIXN];
  u8 L7_cls_WC[tc_rows(L7_cls_m_Volume, L7_cls_hM) * L7_cls_ND];
  i32 L7_cls_WM[tc_rows(L7_cls_n_Volume, L7_cls_hN) * L7_cls_MND * 3];
  u8 L7_cls_WMC[tc_rows(L7_cls_n_Volume, L7_cls_hN) * L7_cls_MND];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long L7_cls_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L7_cls_a_Volume, L7_cls_hA) * L7_cls_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L7_cls_b_Volume, L7_cls_hB) * L7_cls_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(L7_cls_c_Volume * L7_cls_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L7_cls_d_Volume, L7_cls_hD) * L7_cls_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L7_cls_t_Volume, L7_cls_hT) * L7_cls_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(L7_cls_s_Volume, L7_cls_hS) * L7_cls_ND * L7_cls_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L7_cls_s_Volume, L7_cls_hS) * L7_cls_ND * L7_cls_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(L7_cls_f_Volume, L7_cls_hF) * L7_cls_ND * L7_cls_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L7_cls_f_Volume, L7_cls_hF) * L7_cls_ND * L7_cls_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(L7_cls_m_Volume, L7_cls_hM) * L7_cls_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L7_cls_m_Volume, L7_cls_hM) * L7_cls_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(L7_cls_n_Volume, L7_cls_hN) * L7_cls_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L7_cls_n_Volume, L7_cls_hN) * L7_cls_MND);
  void L7_cls_Init( void ) {
  
  }
};
