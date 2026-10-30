


static const byte M3_raw_c_at[11+1]={ 0,0,0,0,0,1,2,3,4,4,5,5 };

static const byte M3_raw_d_code[11+1]={ 0,1,2,2,2,2,3,3,4,4,4,4 };

static const byte M3_raw_e_code[11+1]={ 0,0,1,1,2,2,3,4,4,4,4,4 };
static const byte M3_raw_e_at[11+1]={ 0,0,1,2,2,2,3,3,3,3,3,4 };


static const byte M3_raw_h_code[11+1]={ 0,1,1,2,2,3,4,4,5,5,5,5 };

static const byte M3_raw_m_at[11+1]={ 0,0,1,2,3,3,3,3,4,4,4,5 };




static const int M3_raw_rA = (0+0) * (1);
static const int M3_raw_mwA = (0+0) * (1);
static const int M3_raw_rB = (40+0) * (1);
static const int M3_raw_mwB = (8+0) * (1);
static const int M3_raw_rC = (255+0) * (1);
static const int M3_raw_mwC = (2+0) * (1);
static const int M3_raw_rD = (228+0) * (1);
static const int M3_raw_mwD = (72+0) * (1);
static const int M3_raw_rE = (253+0) * (1);
static const int M3_raw_mwE = (0+0) * (1);
static const int M3_raw_rH = (57+0) * (1);
static const int M3_raw_mwH = (126+0) * (1);
static const int M3_raw_lr = (29+0) * (1);
static const int M3_raw_mb = (29+0) * (1);
static const int M3_raw_rT = (0+0) * (1);
static const int M3_raw_mwT = (0+0) * (1);
static const int M3_raw_mt = (0+0) * (1);
static const int M3_raw_lrm = (0+0) * (1);
static const int M3_raw_mbm = (0+0) * (1);
static const int M3_raw_rG = (0+0) * (1);
static const int M3_raw_mwG = (0+0) * (1);
static const int M3_raw_hA = (20+0) * (1);
static const int M3_raw_hB = (22+0) * (1);
static const int M3_raw_hC = (22+0) * (1);
static const int M3_raw_hD = (22+0) * (1);
static const int M3_raw_hE = (22+0) * (1);
static const int M3_raw_hH = (22+0) * (1);
static const int M3_raw_hM = (22+0) * (1);
static const int M3_raw_hT = (22+0) * (1);
static const int M3_raw_hN = (22+0) * (1);


static const long long M3_raw_a_Volume = tc_vmul(tc_vmul(1, (1<<8)), (1<<7)); // 32768
static const long long M3_raw_b_Volume = tc_vmul(1, (1<<8)); // 256
static const long long M3_raw_c_Volume = tc_vmul(tc_vmul(1, (1<<5)), 6); // 192
static const long long M3_raw_d_Volume = tc_vmul(tc_vmul(1, (1<<4)), 5); // 80
static const long long M3_raw_e_Volume = tc_vmul(tc_vmul(1, 5), 5); // 25
static const long long M3_raw_g_Volume = 1; // 1
static const long long M3_raw_h_Volume = tc_vmul(tc_vmul(1, (1<<8)), 6); // 1536
static const long long M3_raw_m_Volume = tc_vmul(1, 6); // 6
static const long long M3_raw_n_Volume = 1; // 1
static const long long M3_raw_t_Volume = 1; // 1

#define M3_raw_USES(X) X(c1) X(c2) X(at) X(code)

struct M3_raw_T {
  cm_cnt2 M3_raw_A[tc_rows(M3_raw_a_Volume, M3_raw_hA) * M_raw_ND];
  cm_cnt2 M3_raw_B[tc_rows(M3_raw_b_Volume, M3_raw_hB) * M_raw_ND];
  cm_cnt2 M3_raw_C[tc_rows(M3_raw_c_Volume, M3_raw_hC) * M_raw_ND];
  cm_cnt2 M3_raw_D[tc_rows(M3_raw_d_Volume, M3_raw_hD) * M_raw_ND];
  cm_cnt2 M3_raw_E[tc_rows(M3_raw_e_Volume, M3_raw_hE) * M_raw_ND];
  cm_cnt2 M3_raw_H[tc_rows(M3_raw_h_Volume, M3_raw_hH) * M_raw_ND];
  cm_cnt M3_raw_MT[tc_rows(M3_raw_t_Volume, M3_raw_hT) * M_raw_MND];
  i32 M3_raw_W[tc_rows(M3_raw_m_Volume, M3_raw_hM) * M_raw_ND * tc_nin(6)];
  u8 M3_raw_WC[tc_rows(M3_raw_m_Volume, M3_raw_hM) * M_raw_ND];
  i32 M3_raw_WM[tc_rows(M3_raw_n_Volume, M3_raw_hN) * M_raw_MND * TC_MIN];
  u8 M3_raw_WMC[tc_rows(M3_raw_n_Volume, M3_raw_hN) * M_raw_MND];
  cm_cnt M3_raw_G[M3_raw_g_Volume * M_raw_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M3_raw_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_raw_a_Volume, M3_raw_hA) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_raw_b_Volume, M3_raw_hB) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_raw_c_Volume, M3_raw_hC) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_raw_d_Volume, M3_raw_hD) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_raw_e_Volume, M3_raw_hE) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_raw_h_Volume, M3_raw_hH) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M3_raw_t_Volume, M3_raw_hT) * M_raw_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_raw_m_Volume, M3_raw_hM) * M_raw_ND * tc_nin(6))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_raw_m_Volume, M3_raw_hM) * M_raw_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_raw_n_Volume, M3_raw_hN) * M_raw_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_raw_n_Volume, M3_raw_hN) * M_raw_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M3_raw_g_Volume * M_raw_NG);
  void M3_raw_Init( void ) {}
  void M3_raw_Quit( void ) {}
};
