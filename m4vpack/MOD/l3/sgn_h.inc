
static const byte M3_sgn_a_flu[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte M3_sgn_a_flu2[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte M3_sgn_b_fdlu[26+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26 };
static const byte M3_sgn_b_pb[64+1]={ 0,1,1,2,2,2,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 };


static const byte M3_sgn_d_gh[26+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26 };
static const byte M3_sgn_d_pb[64+1]={ 0,1,1,2,2,2,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 };

static const byte M3_sgn_e_lsa[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte M3_sgn_e_pb[64+1]={ 0,1,1,2,2,2,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 };


static const byte M3_sgn_h_lam[24+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24 };
static const byte M3_sgn_h_pb[64+1]={ 0,1,1,2,2,2,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 };

static const byte M3_sgn_m_pb[64+1]={ 0,1,1,2,2,2,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 };




static const int M3_sgn_rA = (189+0) * (1);
static const int M3_sgn_mwA = (32+0) * (1);
static const int M3_sgn_rB = (254+0) * (1);
static const int M3_sgn_mwB = (14+0) * (1);
static const int M3_sgn_rC = (255+0) * (1);
static const int M3_sgn_mwC = (29+0) * (1);
static const int M3_sgn_rD = (0+0) * (1);
static const int M3_sgn_mwD = (131+0) * (1);
static const int M3_sgn_rE = (159+0) * (1);
static const int M3_sgn_mwE = (4+0) * (1);
static const int M3_sgn_rH = (254+0) * (1);
static const int M3_sgn_mwH = (2+0) * (1);
static const int M3_sgn_lr = (15+0) * (1);
static const int M3_sgn_mb = (128+0) * (1);
static const int M3_sgn_rT = (0+0) * (1);
static const int M3_sgn_mwT = (0+0) * (1);
static const int M3_sgn_mt = (0+0) * (1);
static const int M3_sgn_lrm = (0+0) * (1);
static const int M3_sgn_mbm = (0+0) * (1);
static const int M3_sgn_rG = (0+0) * (1);
static const int M3_sgn_mwG = (0+0) * (1);
static const int M3_sgn_hA = (22+0) * (1);
static const int M3_sgn_hB = (22+0) * (1);
static const int M3_sgn_hC = (22+0) * (1);
static const int M3_sgn_hD = (22+0) * (1);
static const int M3_sgn_hE = (22+0) * (1);
static const int M3_sgn_hH = (22+0) * (1);
static const int M3_sgn_hM = (22+0) * (1);
static const int M3_sgn_hT = (22+0) * (1);
static const int M3_sgn_hN = (22+0) * (1);


static const long long M3_sgn_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 9), 9); // 648
static const long long M3_sgn_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 27), 6); // 1296
static const long long M3_sgn_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 3), (1<<6)); // 1536
static const long long M3_sgn_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 27), 6); // 1296
static const long long M3_sgn_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 9), 6); // 432
static const long long M3_sgn_g_Volume = 1; // 1
static const long long M3_sgn_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 25), 6); // 1200
static const long long M3_sgn_m_Volume = tc_vmul(tc_vmul(1, (1<<3)), 6); // 48
static const long long M3_sgn_n_Volume = 1; // 1
static const long long M3_sgn_t_Volume = 1; // 1

#define M3_sgn_USES(X) X(cls) X(flu) X(flu2) X(fdlu) X(pos) X(xs) X(gh) X(lsa) X(lam)

struct M3_sgn_T {
  cm_cnt2 M3_sgn_A[tc_rows(M3_sgn_a_Volume, M3_sgn_hA) * M_sgn_ND];
  cm_cnt2 M3_sgn_B[tc_rows(M3_sgn_b_Volume, M3_sgn_hB) * M_sgn_ND];
  cm_cnt2 M3_sgn_C[tc_rows(M3_sgn_c_Volume, M3_sgn_hC) * M_sgn_ND];
  cm_cnt2 M3_sgn_D[tc_rows(M3_sgn_d_Volume, M3_sgn_hD) * M_sgn_ND];
  cm_cnt2 M3_sgn_E[tc_rows(M3_sgn_e_Volume, M3_sgn_hE) * M_sgn_ND];
  cm_cnt2 M3_sgn_H[tc_rows(M3_sgn_h_Volume, M3_sgn_hH) * M_sgn_ND];
  cm_cnt M3_sgn_MT[tc_rows(M3_sgn_t_Volume, M3_sgn_hT) * M_sgn_MND];
  i32 M3_sgn_W[tc_rows(M3_sgn_m_Volume, M3_sgn_hM) * M_sgn_ND * tc_nin(6)];
  u8 M3_sgn_WC[tc_rows(M3_sgn_m_Volume, M3_sgn_hM) * M_sgn_ND];
  i32 M3_sgn_WM[tc_rows(M3_sgn_n_Volume, M3_sgn_hN) * M_sgn_MND * TC_MIN];
  u8 M3_sgn_WMC[tc_rows(M3_sgn_n_Volume, M3_sgn_hN) * M_sgn_MND];
  cm_cnt M3_sgn_G[M3_sgn_g_Volume * M_sgn_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M3_sgn_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_sgn_a_Volume, M3_sgn_hA) * M_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_sgn_b_Volume, M3_sgn_hB) * M_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_sgn_c_Volume, M3_sgn_hC) * M_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_sgn_d_Volume, M3_sgn_hD) * M_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_sgn_e_Volume, M3_sgn_hE) * M_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_sgn_h_Volume, M3_sgn_hH) * M_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M3_sgn_t_Volume, M3_sgn_hT) * M_sgn_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_sgn_m_Volume, M3_sgn_hM) * M_sgn_ND * tc_nin(6))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_sgn_m_Volume, M3_sgn_hM) * M_sgn_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_sgn_n_Volume, M3_sgn_hN) * M_sgn_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_sgn_n_Volume, M3_sgn_hN) * M_sgn_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M3_sgn_g_Volume * M_sgn_NG);
  void M3_sgn_Init( void ) {}
  void M3_sgn_Quit( void ) {}
};
