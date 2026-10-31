
static const byte A1_exp_a_pd[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte A1_exp_a_d1[5+1]={ 0,1,2,3,4,5 };
static const byte A1_exp_a_es[3+1]={ 0,1,2,3 };

static const byte A1_exp_b_pd[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte A1_exp_b_po[9+1]={ 0,0,1,1,2,3,4,5,5,6 };
static const byte A1_exp_b_es[3+1]={ 0,1,2,3 };

static const byte A1_exp_c_pd[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte A1_exp_c_pl[5+1]={ 0,1,2,3,4,5 };
static const byte A1_exp_c_d1[5+1]={ 0,1,2,3,4,5 };

static const byte A1_exp_d_ht[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte A1_exp_d_pd[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte A1_exp_d_po[9+1]={ 0,0,1,2,3,4,4,4,5,5 };


static const byte A1_exp_m_cc[3+1]={ 0,1,2,3 };




static const int A1_exp_rA = (74+0) * (1);
static const int A1_exp_mwA = (4+0) * (1);
static const int A1_exp_rB = (254+0) * (1);
static const int A1_exp_mwB = (37+0) * (1);
static const int A1_exp_rC = (255+0) * (1);
static const int A1_exp_mwC = (4+0) * (1);
static const int A1_exp_rD = (255+0) * (1);
static const int A1_exp_mwD = (4+0) * (1);
static const int A1_exp_lr = (15+0) * (1);
static const int A1_exp_mb = (51+0) * (1);
static const int A1_exp_rT = (0+0) * (1);
static const int A1_exp_mwT = (0+0) * (1);
static const int A1_exp_mt = (0+0) * (1);
static const int A1_exp_lrm = (0+0) * (1);
static const int A1_exp_mbm = (0+0) * (1);
static const int A1_exp_rG = (0+0) * (1);
static const int A1_exp_mwG = (0+0) * (1);
static const int A1_exp_hA = (22+0) * (1);
static const int A1_exp_hB = (22+0) * (1);
static const int A1_exp_hC = (22+0) * (1);
static const int A1_exp_hD = (22+0) * (1);
static const int A1_exp_hM = (22+0) * (1);
static const int A1_exp_hT = (22+0) * (1);
static const int A1_exp_hN = (22+0) * (1);


static const long long A1_exp_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 10), 6), 4); // 240
static const long long A1_exp_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 10), 7), 4); // 280
static const long long A1_exp_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 10), 6), 6); // 360
static const long long A1_exp_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 8), 10), 6); // 480
static const long long A1_exp_g_Volume = 1; // 1
static const long long A1_exp_m_Volume = tc_vmul(tc_vmul(1, 1), 4); // 4
static const long long A1_exp_n_Volume = 1; // 1
static const long long A1_exp_t_Volume = 1; // 1

#define A1_exp_USES(X) X(pd) X(d1) X(es) X(po) X(pl) X(ht) X(cc)

struct A1_exp_T {
  cm_cnt2s A1_exp_A[tc_rows(A1_exp_a_Volume, A1_exp_hA) * M_exp_ND];
  cm_cnt2s A1_exp_B[tc_rows(A1_exp_b_Volume, A1_exp_hB) * M_exp_ND];
  cm_cnt2s A1_exp_C[tc_rows(A1_exp_c_Volume, A1_exp_hC) * M_exp_ND];
  cm_cnt2s A1_exp_D[tc_rows(A1_exp_d_Volume, A1_exp_hD) * M_exp_ND];
  cm_cnt A1_exp_MT[tc_rows(A1_exp_t_Volume, A1_exp_hT) * M_exp_MND];
  i32 A1_exp_W[tc_rows(A1_exp_m_Volume, A1_exp_hM) * M_exp_ND * tc_nin(4)];
  u8 A1_exp_WC[tc_rows(A1_exp_m_Volume, A1_exp_hM) * M_exp_ND];
  i32 A1_exp_WM[tc_rows(A1_exp_n_Volume, A1_exp_hN) * M_exp_MND * TC_MIN];
  u8 A1_exp_WMC[tc_rows(A1_exp_n_Volume, A1_exp_hN) * M_exp_MND];
  cm_cnt A1_exp_G[A1_exp_g_Volume * M_exp_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long A1_exp_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_exp_a_Volume, A1_exp_hA) * M_exp_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_exp_b_Volume, A1_exp_hB) * M_exp_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_exp_c_Volume, A1_exp_hC) * M_exp_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_exp_d_Volume, A1_exp_hD) * M_exp_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(A1_exp_t_Volume, A1_exp_hT) * M_exp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(A1_exp_m_Volume, A1_exp_hM) * M_exp_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(A1_exp_m_Volume, A1_exp_hM) * M_exp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(A1_exp_n_Volume, A1_exp_hN) * M_exp_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(A1_exp_n_Volume, A1_exp_hN) * M_exp_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(A1_exp_g_Volume * M_exp_NG);
  void A1_exp_Init( void ) {}
  void A1_exp_Quit( void ) {}
};
