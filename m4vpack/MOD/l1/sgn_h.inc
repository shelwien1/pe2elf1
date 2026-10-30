

static const byte M1_sgn_b_fdlu[26+1]={ 0,1,2,2,3,4,5,5,5,6,6,6,6,7,8,9,10,11,12,12,12,12,13,14,15,16,17 };

static const byte M1_sgn_c_flu[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte M1_sgn_c_flu2[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte M1_sgn_d_vote[19+1]={ 0,0,0,0,0,0,0,0,1,2,2,3,3,3,3,3,3,3,3,3 };


static const byte M1_sgn_m_pb[64+1]={ 0,1,2,3,4,5,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9 };




static const int M1_sgn_rA = (255+0) * (1);
static const int M1_sgn_mwA = (0+0) * (1);
static const int M1_sgn_rB = (255+0) * (1);
static const int M1_sgn_mwB = (8+0) * (1);
static const int M1_sgn_rC = (255+0) * (1);
static const int M1_sgn_mwC = (2+0) * (1);
static const int M1_sgn_rD = (0+0) * (1);
static const int M1_sgn_mwD = (96+0) * (1);
static const int M1_sgn_lr = (31+0) * (1);
static const int M1_sgn_mb = (36+0) * (1);
static const int M1_sgn_rT = (0+0) * (1);
static const int M1_sgn_mwT = (0+0) * (1);
static const int M1_sgn_mt = (0+0) * (1);
static const int M1_sgn_lrm = (0+0) * (1);
static const int M1_sgn_mbm = (0+0) * (1);
static const int M1_sgn_rG = (0+0) * (1);
static const int M1_sgn_mwG = (0+0) * (1);
static const int M1_sgn_hA = (22+0) * (1);
static const int M1_sgn_hB = (22+0) * (1);
static const int M1_sgn_hC = (22+0) * (1);
static const int M1_sgn_hD = (22+0) * (1);
static const int M1_sgn_hM = (22+0) * (1);
static const int M1_sgn_hT = (22+0) * (1);
static const int M1_sgn_hN = (22+0) * (1);


static const long long M1_sgn_a_Volume = tc_vmul(tc_vmul(1, (1<<1)), 3); // 6
static const long long M1_sgn_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<2)), 18), 2); // 144
static const long long M1_sgn_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 9), 9); // 648
static const long long M1_sgn_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<1)), 4), 2); // 16
static const long long M1_sgn_g_Volume = 1; // 1
static const long long M1_sgn_m_Volume = tc_vmul(tc_vmul(1, (1<<3)), 10); // 80
static const long long M1_sgn_n_Volume = 1; // 1
static const long long M1_sgn_t_Volume = 1; // 1

#define M1_sgn_USES(X) X(cls) X(xs) X(fdlu) X(pos) X(flu) X(flu2) X(vote)

struct M1_sgn_T {
  cm_cnt2s M1_sgn_A[tc_rows(M1_sgn_a_Volume, M1_sgn_hA) * M_sgn_ND];
  cm_cnt2s M1_sgn_B[tc_rows(M1_sgn_b_Volume, M1_sgn_hB) * M_sgn_ND];
  cm_cnt2s M1_sgn_C[tc_rows(M1_sgn_c_Volume, M1_sgn_hC) * M_sgn_ND];
  cm_cnt2s M1_sgn_D[tc_rows(M1_sgn_d_Volume, M1_sgn_hD) * M_sgn_ND];
  cm_cnt M1_sgn_MT[tc_rows(M1_sgn_t_Volume, M1_sgn_hT) * M_sgn_MND];
  i32 M1_sgn_W[tc_rows(M1_sgn_m_Volume, M1_sgn_hM) * M_sgn_ND * tc_nin(4)];
  u8 M1_sgn_WC[tc_rows(M1_sgn_m_Volume, M1_sgn_hM) * M_sgn_ND];
  i32 M1_sgn_WM[tc_rows(M1_sgn_n_Volume, M1_sgn_hN) * M_sgn_MND * TC_MIN];
  u8 M1_sgn_WMC[tc_rows(M1_sgn_n_Volume, M1_sgn_hN) * M_sgn_MND];
  cm_cnt M1_sgn_G[M1_sgn_g_Volume * M_sgn_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M1_sgn_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_sgn_a_Volume, M1_sgn_hA) * M_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_sgn_b_Volume, M1_sgn_hB) * M_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_sgn_c_Volume, M1_sgn_hC) * M_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_sgn_d_Volume, M1_sgn_hD) * M_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M1_sgn_t_Volume, M1_sgn_hT) * M_sgn_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M1_sgn_m_Volume, M1_sgn_hM) * M_sgn_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M1_sgn_m_Volume, M1_sgn_hM) * M_sgn_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M1_sgn_n_Volume, M1_sgn_hN) * M_sgn_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M1_sgn_n_Volume, M1_sgn_hN) * M_sgn_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M1_sgn_g_Volume * M_sgn_NG);
  void M1_sgn_Init( void ) {}
  void M1_sgn_Quit( void ) {}
};
