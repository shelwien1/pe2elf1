
static const byte M1_hdr_a_fld[39+1]={ 0,0,0,1,2,2,2,3,4,4,4,5,6,6,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9 };
static const byte M1_hdr_a_p1[64+1]={ 0,1,2,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };

static const byte M1_hdr_b_fld[39+1]={ 0,1,2,3,4,5,6,7,7,7,7,7,8,8,8,9,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,11,11,11,11,11,11 };
static const byte M1_hdr_b_ty[4+1]={ 0,1,2,3,3 };

static const byte M1_hdr_c_fld[39+1]={ 0,0,1,1,1,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6 };

static const byte M1_hdr_d_fld[39+1]={ 0,1,1,1,1,1,1,1,1,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5 };
static const byte M1_hdr_d_p1[64+1]={ 0,1,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };

static const byte M1_hdr_g_fld[39+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39 };

static const byte M1_hdr_m_fld[39+1]={ 0,1,2,3,4,5,6,7,8,8,9,10,11,11,12,13,13,13,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,15,15,15,15,15,15 };

static const byte M1_hdr_n_fld[39+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39 };

static const byte M1_hdr_t_fld[39+1]={ 0,0,0,0,1,1,2,2,2,2,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };


static const int M1_hdr_rA = (0+0) * (1);
static const int M1_hdr_mwA = (32+0) * (1);
static const int M1_hdr_rB = (252+0) * (1);
static const int M1_hdr_mwB = (0+0) * (1);
static const int M1_hdr_rC = (0+0) * (1);
static const int M1_hdr_mwC = (32+0) * (1);
static const int M1_hdr_rD = (254+0) * (1);
static const int M1_hdr_mwD = (8+0) * (1);
static const int M1_hdr_lr = (31+0) * (1);
static const int M1_hdr_mb = (182+0) * (1);
static const int M1_hdr_rT = (128+0) * (1);
static const int M1_hdr_mwT = (0+0) * (1);
static const int M1_hdr_mt = (6+0) * (1);
static const int M1_hdr_rG = (64+0) * (1);
static const int M1_hdr_mwG = (255+0) * (1);
static const int M1_hdr_lrm = (0+0) * (1);
static const int M1_hdr_mbm = (0+0) * (1);
static const int M1_hdr_hA = (22+0) * (1);
static const int M1_hdr_hB = (22+0) * (1);
static const int M1_hdr_hC = (22+0) * (1);
static const int M1_hdr_hD = (22+0) * (1);
static const int M1_hdr_hM = (22+0) * (1);
static const int M1_hdr_hT = (22+0) * (1);
static const int M1_hdr_hN = (22+0) * (1);


static const long long M1_hdr_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 10), 3), 5); // 150
static const long long M1_hdr_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 12), 4), 2); // 96
static const long long M1_hdr_c_Volume = tc_vmul(tc_vmul(1, 7), 1); // 7
static const long long M1_hdr_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 6), 3), 4), 2); // 144
static const long long M1_hdr_g_Volume = tc_vmul(1, 40); // 40
static const long long M1_hdr_m_Volume = tc_vmul(1, 16); // 16
static const long long M1_hdr_n_Volume = tc_vmul(1, 40); // 40
static const long long M1_hdr_t_Volume = tc_vmul(1, 5); // 5

#define M1_hdr_USES(X) X(fld) X(ty) X(p1) X(pos)

struct M1_hdr_T {
  cm_cnt2s M1_hdr_A[tc_rows(M1_hdr_a_Volume, M1_hdr_hA) * M_hdr_ND];
  cm_cnt2s M1_hdr_B[tc_rows(M1_hdr_b_Volume, M1_hdr_hB) * M_hdr_ND];
  cm_cnt2s M1_hdr_C[tc_rows(M1_hdr_c_Volume, M1_hdr_hC) * M_hdr_ND];
  cm_cnt2s M1_hdr_D[tc_rows(M1_hdr_d_Volume, M1_hdr_hD) * M_hdr_ND];
  cm_cnt M1_hdr_MT[tc_rows(M1_hdr_t_Volume, M1_hdr_hT) * M_hdr_MND];
  i32 M1_hdr_W[tc_rows(M1_hdr_m_Volume, M1_hdr_hM) * M_hdr_ND * tc_nin(4)];
  u8 M1_hdr_WC[tc_rows(M1_hdr_m_Volume, M1_hdr_hM) * M_hdr_ND];
  i32 M1_hdr_WM[tc_rows(M1_hdr_n_Volume, M1_hdr_hN) * M_hdr_MND * TC_MIN];
  u8 M1_hdr_WMC[tc_rows(M1_hdr_n_Volume, M1_hdr_hN) * M_hdr_MND];
  cm_cnt M1_hdr_G[M1_hdr_g_Volume * M_hdr_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M1_hdr_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_hdr_a_Volume, M1_hdr_hA) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_hdr_b_Volume, M1_hdr_hB) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_hdr_c_Volume, M1_hdr_hC) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(M1_hdr_d_Volume, M1_hdr_hD) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M1_hdr_t_Volume, M1_hdr_hT) * M_hdr_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M1_hdr_m_Volume, M1_hdr_hM) * M_hdr_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M1_hdr_m_Volume, M1_hdr_hM) * M_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M1_hdr_n_Volume, M1_hdr_hN) * M_hdr_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M1_hdr_n_Volume, M1_hdr_hN) * M_hdr_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M1_hdr_g_Volume * M_hdr_NG);
  void M1_hdr_Init( void ) {}
  void M1_hdr_Quit( void ) {}
};
