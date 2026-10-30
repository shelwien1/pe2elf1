

static const byte M3_lvl_b_nmb[8+1]={ 0,1,2,3,3,4,4,4,4 };
static const byte M3_lvl_b_ngt[8+1]={ 0,1,2,3,4,4,4,4,4 };
static const byte M3_lvl_b_pb[64+1]={ 0,1,1,2,2,2,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 };


static const byte M3_lvl_d_den[4+1]={ 0,1,2,3,4 };
static const byte M3_lvl_d_pb[64+1]={ 0,1,1,2,2,2,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 };

static const byte M3_lvl_e_nb2[8+1]={ 0,1,2,3,3,4,4,4,4 };
static const byte M3_lvl_e_pb[64+1]={ 0,1,1,2,2,2,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 };


static const byte M3_lvl_h_gs[9+1]={ 0,1,2,3,4,4,4,4,4,4 };
static const byte M3_lvl_h_kc[32+1]={ 0,0,1,2,3,3,4,4,4,4,5,5,5,5,5,5,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7 };

static const byte M3_lvl_m_pb[64+1]={ 0,1,1,2,2,2,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 };
static const byte M3_lvl_m_gs[9+1]={ 0,1,2,3,4,4,4,4,4,4 };




static const int M3_lvl_rA = (253+0) * (1);
static const int M3_lvl_mwA = (62+0) * (1);
static const int M3_lvl_rB = (242+0) * (1);
static const int M3_lvl_mwB = (4+0) * (1);
static const int M3_lvl_rC = (255+0) * (1);
static const int M3_lvl_mwC = (51+0) * (1);
static const int M3_lvl_rD = (255+0) * (1);
static const int M3_lvl_mwD = (8+0) * (1);
static const int M3_lvl_rE = (255+0) * (1);
static const int M3_lvl_mwE = (16+0) * (1);
static const int M3_lvl_rH = (255+0) * (1);
static const int M3_lvl_mwH = (4+0) * (1);
static const int M3_lvl_lr = (15+0) * (1);
static const int M3_lvl_mb = (33+0) * (1);
static const int M3_lvl_rT = (128+0) * (1);
static const int M3_lvl_mwT = (0+0) * (1);
static const int M3_lvl_mt = (0+0) * (1);
static const int M3_lvl_lrm = (25+0) * (1);
static const int M3_lvl_mbm = (128+0) * (1);
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


static const long long M3_lvl_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), (1<<4)), (1<<6)); // 8192
static const long long M3_lvl_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 5), 5), 6); // 1200
static const long long M3_lvl_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), (1<<6)), (1<<6)); // 32768
static const long long M3_lvl_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 5), 6); // 240
static const long long M3_lvl_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 5), 6); // 240
static const long long M3_lvl_g_Volume = 1; // 1
static const long long M3_lvl_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 5), 8); // 320
static const long long M3_lvl_m_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 6), 5); // 240
static const long long M3_lvl_n_Volume = tc_vmul(1, (1<<3)); // 8
static const long long M3_lvl_t_Volume = tc_vmul(tc_vmul(1, (1<<3)), 3); // 24

#define M3_lvl_USES(X) X(cls) X(rb) X(pos) X(nmb) X(ngt) X(lh) X(den) X(nb2) X(gs) X(kc) X(qs)

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
