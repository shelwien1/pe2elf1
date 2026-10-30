
static const byte M1_sig_a_rnz[17+1]={ 0,1,2,3,3,3,4,4,4,4,4,5,5,5,5,5,6,6 };
static const byte M1_sig_a_xm[9+1]={ 0,1,2,2,2,2,2,2,3,3 };









static const int M1_sig_rA = (253+0) * (1);
static const int M1_sig_mwA = (65+0) * (1);
static const int M1_sig_rB = (248+0) * (1);
static const int M1_sig_mwB = (63+0) * (1);
static const int M1_sig_rC = (243+0) * (1);
static const int M1_sig_mwC = (32+0) * (1);
static const int M1_sig_rD = (196+0) * (1);
static const int M1_sig_mwD = (7+0) * (1);
static const int M1_sig_lr = (12+0) * (1);
static const int M1_sig_mb = (88+0) * (1);
static const int M1_sig_rT = (0+0) * (1);
static const int M1_sig_mwT = (0+0) * (1);
static const int M1_sig_mt = (0+0) * (1);
static const int M1_sig_lrm = (0+0) * (1);
static const int M1_sig_mbm = (0+0) * (1);
static const int M1_sig_rG = (0+0) * (1);
static const int M1_sig_mwG = (0+0) * (1);
static const int M1_sig_hA = (22+0) * (1);
static const int M1_sig_hB = (22+0) * (1);
static const int M1_sig_hC = (20+0) * (1);
static const int M1_sig_hD = (22+0) * (1);
static const int M1_sig_hM = (22+0) * (1);
static const int M1_sig_hT = (22+0) * (1);
static const int M1_sig_hN = (22+0) * (1);


static const long long M1_sig_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 7), 4), (1<<6)); // 14336
static const long long M1_sig_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), (1<<5)), (1<<6)); // 16384
static const long long M1_sig_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), (1<<6)), (1<<16)); // 33554432
static const long long M1_sig_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), (1<<8)), (1<<6)); // 131072
static const long long M1_sig_g_Volume = 1; // 1
static const long long M1_sig_m_Volume = tc_vmul(tc_vmul(1, (1<<3)), (1<<6)); // 512
static const long long M1_sig_n_Volume = 1; // 1
static const long long M1_sig_t_Volume = 1; // 1

#define M1_sig_USES(X) X(cls) X(rnz) X(xm) X(pos) X(fp) X(h16) X(h8)

struct M1_sig_T {
  cm_cnt2s M1_sig_A[tc_rows(M1_sig_a_Volume, M1_sig_hA) * M_sig_ND];
  cm_cnt2s M1_sig_B[tc_rows(M1_sig_b_Volume, M1_sig_hB) * M_sig_ND];
  cm_cnt2s M1_sig_C[tc_rows(M1_sig_c_Volume, M1_sig_hC) * M_sig_ND];
  cm_cnt2s M1_sig_D[tc_rows(M1_sig_d_Volume, M1_sig_hD) * M_sig_ND];
  cm_cnt M1_sig_MT[tc_rows(M1_sig_t_Volume, M1_sig_hT) * M_sig_MND];
  i32 M1_sig_W[tc_rows(M1_sig_m_Volume, M1_sig_hM) * M_sig_ND * tc_nin(4)];
  u8 M1_sig_WC[tc_rows(M1_sig_m_Volume, M1_sig_hM) * M_sig_ND];
  i32 M1_sig_WM[tc_rows(M1_sig_n_Volume, M1_sig_hN) * M_sig_MND * TC_MIN];
  u8 M1_sig_WMC[tc_rows(M1_sig_n_Volume, M1_sig_hN) * M_sig_MND];
  cm_cnt M1_sig_G[M1_sig_g_Volume * M_sig_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M1_sig_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_sig_a_Volume, M1_sig_hA) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_sig_b_Volume, M1_sig_hB) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_sig_c_Volume, M1_sig_hC) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_sig_d_Volume, M1_sig_hD) * M_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M1_sig_t_Volume, M1_sig_hT) * M_sig_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M1_sig_m_Volume, M1_sig_hM) * M_sig_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M1_sig_m_Volume, M1_sig_hM) * M_sig_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M1_sig_n_Volume, M1_sig_hN) * M_sig_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M1_sig_n_Volume, M1_sig_hN) * M_sig_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M1_sig_g_Volume * M_sig_NG);
  void M1_sig_Init( void ) {}
  void M1_sig_Quit( void ) {}
};
