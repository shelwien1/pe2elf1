
static const byte M1_raw_a_at[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };



static const byte M1_raw_d_at[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };


static const byte M1_raw_m_at[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };




static const int M1_raw_rA = (0+0) * (1);
static const int M1_raw_mwA = (0+0) * (1);
static const int M1_raw_rB = (108+0) * (1);
static const int M1_raw_mwB = (1+0) * (1);
static const int M1_raw_rC = (224+0) * (1);
static const int M1_raw_mwC = (0+0) * (1);
static const int M1_raw_rD = (0+0) * (1);
static const int M1_raw_mwD = (0+0) * (1);
static const int M1_raw_lr = (31+0) * (1);
static const int M1_raw_mb = (30+0) * (1);
static const int M1_raw_rT = (0+0) * (1);
static const int M1_raw_mwT = (0+0) * (1);
static const int M1_raw_mt = (0+0) * (1);
static const int M1_raw_lrm = (0+0) * (1);
static const int M1_raw_mbm = (0+0) * (1);
static const int M1_raw_rG = (0+0) * (1);
static const int M1_raw_mwG = (0+0) * (1);
static const int M1_raw_hA = (22+0) * (1);
static const int M1_raw_hB = (20+0) * (1);
static const int M1_raw_hC = (22+0) * (1);
static const int M1_raw_hD = (22+0) * (1);
static const int M1_raw_hM = (22+0) * (1);
static const int M1_raw_hT = (22+0) * (1);
static const int M1_raw_hN = (22+0) * (1);


static const long long M1_raw_a_Volume = tc_vmul(tc_vmul(1, (1<<8)), 12); // 3072
static const long long M1_raw_b_Volume = tc_vmul(tc_vmul(1, (1<<8)), (1<<8)); // 65536
static const long long M1_raw_c_Volume = tc_vmul(1, (1<<8)); // 256
static const long long M1_raw_d_Volume = tc_vmul(1, 12); // 12
static const long long M1_raw_g_Volume = 1; // 1
static const long long M1_raw_m_Volume = tc_vmul(1, 12); // 12
static const long long M1_raw_n_Volume = 1; // 1
static const long long M1_raw_t_Volume = 1; // 1

#define M1_raw_USES(X) X(c1) X(at) X(c2)

struct M1_raw_T {
  cm_cnt2s M1_raw_A[tc_rows(M1_raw_a_Volume, M1_raw_hA) * M_raw_ND];
  cm_cnt2s M1_raw_B[tc_rows(M1_raw_b_Volume, M1_raw_hB) * M_raw_ND];
  cm_cnt2s M1_raw_C[tc_rows(M1_raw_c_Volume, M1_raw_hC) * M_raw_ND];
  cm_cnt2s M1_raw_D[tc_rows(M1_raw_d_Volume, M1_raw_hD) * M_raw_ND];
  cm_cnt M1_raw_MT[tc_rows(M1_raw_t_Volume, M1_raw_hT) * M_raw_MND];
  i32 M1_raw_W[tc_rows(M1_raw_m_Volume, M1_raw_hM) * M_raw_ND * tc_nin(4)];
  u8 M1_raw_WC[tc_rows(M1_raw_m_Volume, M1_raw_hM) * M_raw_ND];
  i32 M1_raw_WM[tc_rows(M1_raw_n_Volume, M1_raw_hN) * M_raw_MND * TC_MIN];
  u8 M1_raw_WMC[tc_rows(M1_raw_n_Volume, M1_raw_hN) * M_raw_MND];
  cm_cnt M1_raw_G[M1_raw_g_Volume * M_raw_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M1_raw_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_raw_a_Volume, M1_raw_hA) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_raw_b_Volume, M1_raw_hB) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_raw_c_Volume, M1_raw_hC) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_raw_d_Volume, M1_raw_hD) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M1_raw_t_Volume, M1_raw_hT) * M_raw_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M1_raw_m_Volume, M1_raw_hM) * M_raw_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M1_raw_m_Volume, M1_raw_hM) * M_raw_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M1_raw_n_Volume, M1_raw_hN) * M_raw_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M1_raw_n_Volume, M1_raw_hN) * M_raw_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M1_raw_g_Volume * M_raw_NG);
  void M1_raw_Init( void ) {}
  void M1_raw_Quit( void ) {}
};
