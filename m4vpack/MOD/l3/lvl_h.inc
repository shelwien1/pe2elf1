
static const byte M3_lvl_a_nmb[8+1]={ 0,1,2,3,3,4,4,5,6 };
static const byte M3_lvl_a_ngt[8+1]={ 0,1,2,3,3,3,3,4,4 };
static const byte M3_lvl_a_pb[64+1]={ 0,0,0,1,2,3,3,4,5,5,6,7,7,7,8,8,9,9,9,9,10,10,10,11,11,11,11,12,12,12,13,13,13,13,14,15,15,16,17,17,17,18,19,20,20,21,21,21,22,22,23,23,24,24,24,24,24,25,26,26,27,27,27,27,27 };

static const byte M3_lvl_b_nb2[8+1]={ 0,1,2,3,4,5,5,6,6 };

static const byte M3_lvl_c_pm[8+1]={ 0,1,1,2,3,4,4,5,5 };
static const byte M3_lvl_c_pb[64+1]={ 0,1,2,3,4,5,6,7,8,9,10,10,11,12,13,14,15,16,17,18,19,19,19,20,20,20,21,21,22,23,24,25,26,27,27,28,29,29,29,30,30,30,31,31,32,32,32,32,32,32,32,32,33,34,34,35,35,35,35,36,37,37,37,37,37 };

static const byte M3_lvl_d_mvc[30+1]={ 0,0,1,2,2,3,4,5,6,7,8,9,10,10,11,12,13,13,13,14,15,16,17,18,18,18,18,18,18,18,18 };
static const byte M3_lvl_d_pb[64+1]={ 0,1,2,3,4,5,6,7,8,9,9,9,9,10,10,10,10,11,12,12,13,13,13,14,15,16,17,17,17,17,17,18,18,19,19,19,20,20,21,21,22,22,22,22,22,23,24,25,25,26,27,27,28,28,29,29,30,30,30,30,30,30,30,30,30 };

static const byte M3_lvl_e_den[4+1]={ 0,1,2,3,4 };
static const byte M3_lvl_e_pb[64+1]={ 0,1,2,3,4,5,6,7,7,7,8,9,10,10,11,12,13,14,15,16,17,17,18,19,20,21,22,23,24,25,25,26,26,27,27,28,29,29,29,30,31,31,31,31,31,31,31,31,31,31,31,31,31,32,32,33,34,35,36,36,36,36,36,36,36 };



static const byte M3_lvl_m_pb[64+1]={ 0,1,1,2,3,3,3,3,3,4,4,5,5,5,5,5,5,5,5,5,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8,8,9,9,9,9,9,9,9,9,9,10,10,10,10,10,10 };




static const int M3_lvl_rA = (255+0) * (1);
static const int M3_lvl_mwA = (49+0) * (1);
static const int M3_lvl_rB = (255+0) * (1);
static const int M3_lvl_mwB = (22+0) * (1);
static const int M3_lvl_rC = (253+0) * (1);
static const int M3_lvl_mwC = (4+0) * (1);
static const int M3_lvl_rD = (255+0) * (1);
static const int M3_lvl_mwD = (51+0) * (1);
static const int M3_lvl_rE = (253+0) * (1);
static const int M3_lvl_mwE = (32+0) * (1);
static const int M3_lvl_rH = (252+0) * (1);
static const int M3_lvl_mwH = (63+0) * (1);
static const int M3_lvl_lr = (13+0) * (1);
static const int M3_lvl_mb = (48+0) * (1);
static const int M3_lvl_rT = (255+0) * (1);
static const int M3_lvl_mwT = (16+0) * (1);
static const int M3_lvl_mt = (0+0) * (1);
static const int M3_lvl_lrm = (27+0) * (1);
static const int M3_lvl_mbm = (25+0) * (1);
static const int M3_lvl_rG = (0+0) * (1);
static const int M3_lvl_mwG = (0+0) * (1);
static const int M3_lvl_hA = (22+0) * (1);
static const int M3_lvl_hB = (22+0) * (1);
static const int M3_lvl_hC = (22+0) * (1);
static const int M3_lvl_hD = (22+0) * (1);
static const int M3_lvl_hE = (22+0) * (1);
static const int M3_lvl_hH = (22+0) * (1);
static const int M3_lvl_hM = (22+0) * (1);
static const int M3_lvl_hT = (22+0) * (1);
static const int M3_lvl_hN = (22+0) * (1);


static const long long M3_lvl_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 7), 5), 28); // 980
static const long long M3_lvl_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 7), 1); // 56
static const long long M3_lvl_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<2)), 6), 38), 2); // 1824
static const long long M3_lvl_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<2)), 19), 31); // 2356
static const long long M3_lvl_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<2)), 5), 37); // 740
static const long long M3_lvl_g_Volume = 1; // 1
static const long long M3_lvl_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<2)), (1<<4)), (1<<6)); // 4096
static const long long M3_lvl_m_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<2)), 11), 1); // 44
static const long long M3_lvl_n_Volume = 1; // 1
static const long long M3_lvl_t_Volume = tc_vmul(tc_vmul(1, (1<<1)), 1); // 2

#define M3_lvl_USES(X) X(nmb) X(ngt) X(pos) X(cls) X(nb2) X(pm) X(qs) X(mvc) X(den) X(rb)

struct M3_lvl_T {
  cm_cnt2 M3_lvl_A[tc_rows(M3_lvl_a_Volume, M3_lvl_hA) * M_lvl_ND];
  cm_cnt2 M3_lvl_B[tc_rows(M3_lvl_b_Volume, M3_lvl_hB) * M_lvl_ND];
  cm_cnt2 M3_lvl_C[tc_rows(M3_lvl_c_Volume, M3_lvl_hC) * M_lvl_ND];
  cm_cnt2 M3_lvl_D[tc_rows(M3_lvl_d_Volume, M3_lvl_hD) * M_lvl_ND];
  cm_cnt2 M3_lvl_E[tc_rows(M3_lvl_e_Volume, M3_lvl_hE) * M_lvl_ND];
  cm_cnt2 M3_lvl_H[tc_rows(M3_lvl_h_Volume, M3_lvl_hH) * M_lvl_ND];
  cm_cnt M3_lvl_MT[tc_rows(M3_lvl_t_Volume, M3_lvl_hT) * M_lvl_MND];
  i32 M3_lvl_W[tc_rows(M3_lvl_m_Volume, M3_lvl_hM) * M_lvl_ND * tc_nin(6)];
  u8 M3_lvl_WC[tc_rows(M3_lvl_m_Volume, M3_lvl_hM) * M_lvl_ND];
  i32 M3_lvl_WM[tc_rows(M3_lvl_n_Volume, M3_lvl_hN) * M_lvl_MND * TC_MIN];
  u8 M3_lvl_WMC[tc_rows(M3_lvl_n_Volume, M3_lvl_hN) * M_lvl_MND];
  cm_cnt M3_lvl_G[M3_lvl_g_Volume * M_lvl_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M3_lvl_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_lvl_a_Volume, M3_lvl_hA) * M_lvl_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_lvl_b_Volume, M3_lvl_hB) * M_lvl_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_lvl_c_Volume, M3_lvl_hC) * M_lvl_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_lvl_d_Volume, M3_lvl_hD) * M_lvl_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_lvl_e_Volume, M3_lvl_hE) * M_lvl_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(M3_lvl_h_Volume, M3_lvl_hH) * M_lvl_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(M3_lvl_t_Volume, M3_lvl_hT) * M_lvl_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_lvl_m_Volume, M3_lvl_hM) * M_lvl_ND * tc_nin(6))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_lvl_m_Volume, M3_lvl_hM) * M_lvl_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(M3_lvl_n_Volume, M3_lvl_hN) * M_lvl_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(M3_lvl_n_Volume, M3_lvl_hN) * M_lvl_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(M3_lvl_g_Volume * M_lvl_NG);
  void M3_lvl_Init( void ) {}
  void M3_lvl_Quit( void ) {}
};
