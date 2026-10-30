
static const byte M3_hdr_a_fld[39+1]={ 0,1,1,2,3,3,3,3,4,5,5,5,5,6,6,6,7,7,7,7,7,8,8,8,8,8,8,8,9,9,9,9,9,9,10,10,10,10,10,10 };

static const byte M3_hdr_b_fld[39+1]={ 0,0,1,1,2,3,4,4,4,5,5,6,6,6,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9 };

static const byte M3_hdr_c_fld[39+1]={ 0,1,1,2,3,3,3,4,4,4,4,4,4,4,4,5,5,6,6,6,6,7,8,8,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9 };

static const byte M3_hdr_d_fld[39+1]={ 0,1,2,3,4,5,6,7,7,7,7,8,8,8,8,9,10,10,10,11,11,12,12,12,12,12,12,12,12,12,12,12,12,12,13,13,13,13,13,13 };

static const byte M3_hdr_e_fld[39+1]={ 0,0,1,2,2,2,2,2,3,3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7 };
static const byte M3_hdr_e_p1[64+1]={ 0,1,2,2,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };

static const byte M3_hdr_g_fld[39+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39 };

static const byte M3_hdr_h_fld[39+1]={ 0,1,1,2,3,3,3,4,5,5,5,6,6,6,6,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,8,9,9,10,10,10,10,10,10 };
static const byte M3_hdr_h_ty[4+1]={ 0,1,2,2,3 };

static const byte M3_hdr_m_fld[39+1]={ 0,1,2,3,4,5,6,7,7,8,8,8,8,8,8,9,9,10,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,12,12,12,12,12,12 };

static const byte M3_hdr_n_fld[39+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39 };

static const byte M3_hdr_t_fld[39+1]={ 0,0,0,0,1,1,2,2,2,2,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };


static const int M3_hdr_rA = (0+0) * (1);
static const int M3_hdr_mwA = (63+0) * (1);
static const int M3_hdr_rB = (255+0) * (1);
static const int M3_hdr_mwB = (10+0) * (1);
static const int M3_hdr_rC = (0+0) * (1);
static const int M3_hdr_mwC = (7+0) * (1);
static const int M3_hdr_rD = (252+0) * (1);
static const int M3_hdr_mwD = (34+0) * (1);
static const int M3_hdr_rE = (255+0) * (1);
static const int M3_hdr_mwE = (16+0) * (1);
static const int M3_hdr_rH = (35+0) * (1);
static const int M3_hdr_mwH = (0+0) * (1);
static const int M3_hdr_lr = (31+0) * (1);
static const int M3_hdr_mb = (123+0) * (1);
static const int M3_hdr_rT = (254+0) * (1);
static const int M3_hdr_mwT = (0+0) * (1);
static const int M3_hdr_mt = (6+0) * (1);
static const int M3_hdr_rG = (64+0) * (1);
static const int M3_hdr_mwG = (255+0) * (1);
static const int M3_hdr_lrm = (0+0) * (1);
static const int M3_hdr_mbm = (0+0) * (1);
static const int M3_hdr_hA = (22+0) * (1);
static const int M3_hdr_hB = (22+0) * (1);
static const int M3_hdr_hC = (22+0) * (1);
static const int M3_hdr_hD = (22+0) * (1);
static const int M3_hdr_hE = (22+0) * (1);
static const int M3_hdr_hH = (22+0) * (1);
static const int M3_hdr_hM = (22+0) * (1);
static const int M3_hdr_hT = (22+0) * (1);
static const int M3_hdr_hN = (22+0) * (1);


static const long long M3_hdr_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 11), 3), 3); // 99
static const long long M3_hdr_b_Volume = tc_vmul(tc_vmul(1, 10), 1); // 10
static const long long M3_hdr_c_Volume = tc_vmul(1, 10); // 10
static const long long M3_hdr_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 14), 3), 2); // 84
static const long long M3_hdr_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 8), 5), 1); // 40
static const long long M3_hdr_g_Volume = tc_vmul(1, 40); // 40
static const long long M3_hdr_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 11), 4), 2), 2); // 176
static const long long M3_hdr_m_Volume = tc_vmul(1, 13); // 13
static const long long M3_hdr_n_Volume = tc_vmul(1, 40); // 40
static const long long M3_hdr_t_Volume = tc_vmul(1, 5); // 5

#define M3_hdr_USES(X) X(fld) X(ty) X(p1) X(pos)

struct M3_hdr_T {
  cm_cnt2 M3_hdr_A[tc_rows(M3_hdr_a_Volume, M3_hdr_hA) * M_hdr_ND];
  cm_cnt2 M3_hdr_B[tc_rows(M3_hdr_b_Volume, M3_hdr_hB) * M_hdr_ND];
  cm_cnt2 M3_hdr_C[tc_rows(M3_hdr_c_Volume, M3_hdr_hC) * M_hdr_ND];
  cm_cnt2 M3_hdr_D[tc_rows(M3_hdr_d_Volume, M3_hdr_hD) * M_hdr_ND];
  cm_cnt2 M3_hdr_E[tc_rows(M3_hdr_e_Volume, M3_hdr_hE) * M_hdr_ND];
  cm_cnt2 M3_hdr_H[tc_rows(M3_hdr_h_Volume, M3_hdr_hH) * M_hdr_ND];
  cm_cnt M3_hdr_MT[tc_rows(M3_hdr_t_Volume, M3_hdr_hT) * M_hdr_MND];
  i32 M3_hdr_W[tc_rows(M3_hdr_m_Volume, M3_hdr_hM) * M_hdr_ND * tc_nin(6)];
  u8 M3_hdr_WC[tc_rows(M3_hdr_m_Volume, M3_hdr_hM) * M_hdr_ND];
  i32 M3_hdr_WM[tc_rows(M3_hdr_n_Volume, M3_hdr_hN) * M_hdr_MND * TC_MIN];
  u8 M3_hdr_WMC[tc_rows(M3_hdr_n_Volume, M3_hdr_hN) * M_hdr_MND];
  cm_cnt M3_hdr_G[M3_hdr_g_Volume * M_hdr_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M3_hdr_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_hdr_a_Volume, M3_hdr_hA) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_hdr_b_Volume, M3_hdr_hB) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_hdr_c_Volume, M3_hdr_hC) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_hdr_d_Volume, M3_hdr_hD) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_hdr_e_Volume, M3_hdr_hE) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_hdr_h_Volume, M3_hdr_hH) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M3_hdr_t_Volume, M3_hdr_hT) * M_hdr_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_hdr_m_Volume, M3_hdr_hM) * M_hdr_ND * tc_nin(6))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_hdr_m_Volume, M3_hdr_hM) * M_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_hdr_n_Volume, M3_hdr_hN) * M_hdr_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_hdr_n_Volume, M3_hdr_hN) * M_hdr_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M3_hdr_g_Volume * M_hdr_NG);
  void M3_hdr_Init( void ) {}
  void M3_hdr_Quit( void ) {}
};
