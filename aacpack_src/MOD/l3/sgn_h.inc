
static const byte H3_sgn_a_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };

static const byte H3_sgn_b_mg[4+1]={ 0,1,2,3,4 };



static const byte H3_sgn_e_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };



static const byte H3_sgn_m_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };



static const int H3_sgn_rA = (128+0) * (1);
static const int H3_sgn_rB = (255+0) * (1);
static const int H3_sgn_rC = (64+0) * (1);
static const int H3_sgn_rD = (128+0) * (1);
static const int H3_sgn_rE = (254+0) * (1);
static const int H3_sgn_rH = (255+0) * (1);
static const int H3_sgn_lr = (25+0) * (1);
static const int H3_sgn_mwA = (4+0) * (1);
static const int H3_sgn_mwB = (14+0) * (1);
static const int H3_sgn_mwC = (0+0) * (1);
static const int H3_sgn_mwD = (80+0) * (1);
static const int H3_sgn_mwE = (24+0) * (1);
static const int H3_sgn_mwH = (114+0) * (1);
static const int H3_sgn_mb = (0+0) * (1);
static const int H3_sgn_rT = (0+0) * (1);
static const int H3_sgn_mwT = (0+0) * (1);
static const int H3_sgn_mt = (0+0) * (1);
static const int H3_sgn_rG = (0+0) * (1);
static const int H3_sgn_mwG = (0+0) * (1);
static const int H3_sgn_hA = (24+0) * (1);
static const int H3_sgn_hB = (24+0) * (1);
static const int H3_sgn_hC = (24+0) * (1);
static const int H3_sgn_hD = (24+0) * (1);
static const int H3_sgn_hE = (24+0) * (1);
static const int H3_sgn_hH = (24+0) * (1);
static const int H3_sgn_hM = (24+0) * (1);
static const int H3_sgn_hT = (24+0) * (1);


static const long long H3_sgn_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 12); // 36
static const long long H3_sgn_b_Volume = tc_vmul(tc_vmul(1, 3), 5); // 15
static const long long H3_sgn_c_Volume = tc_vmul(tc_vmul(1, 3), 2); // 6
static const long long H3_sgn_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 3); // 27
static const long long H3_sgn_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, 12), 3), 3); // 108
static const long long H3_sgn_g_Volume = 1; // 1
static const long long H3_sgn_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 3); // 12
static const long long H3_sgn_m_Volume = tc_vmul(1, 12); // 12
static const long long H3_sgn_t_Volume = 1; // 1

#define H3_sgn_USES(X) X(s2) X(cb) X(oc) X(mg) X(s1) X(pf) X(ms)

struct H3_sgn_T {
  cm_cnt2 H3_sgn_A[tc_rows(H3_sgn_a_Volume, H3_sgn_hA) * H_sgn_ND];
  cm_cnt2 H3_sgn_B[tc_rows(H3_sgn_b_Volume, H3_sgn_hB) * H_sgn_ND];
  cm_cnt2 H3_sgn_C[tc_rows(H3_sgn_c_Volume, H3_sgn_hC) * H_sgn_ND];
  cm_cnt2 H3_sgn_D[tc_rows(H3_sgn_d_Volume, H3_sgn_hD) * H_sgn_ND];
  cm_cnt2 H3_sgn_E[tc_rows(H3_sgn_e_Volume, H3_sgn_hE) * H_sgn_ND];
  cm_cnt2 H3_sgn_H[tc_rows(H3_sgn_h_Volume, H3_sgn_hH) * H_sgn_ND];
  cm_cnt H3_sgn_MT[tc_rows(H3_sgn_t_Volume, H3_sgn_hT) * H_sgn_MND];
  i32 H3_sgn_W[tc_rows(H3_sgn_m_Volume, H3_sgn_hM) * H_sgn_ND * tc_nin(6)];
  u8 H3_sgn_WC[tc_rows(H3_sgn_m_Volume, H3_sgn_hM) * H_sgn_ND];
  cm_cnt H3_sgn_G[H3_sgn_g_Volume * H_sgn_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H3_sgn_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sgn_a_Volume, H3_sgn_hA) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sgn_b_Volume, H3_sgn_hB) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sgn_c_Volume, H3_sgn_hC) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sgn_d_Volume, H3_sgn_hD) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sgn_e_Volume, H3_sgn_hE) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sgn_h_Volume, H3_sgn_hH) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H3_sgn_t_Volume, H3_sgn_hT) * H_sgn_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_sgn_m_Volume, H3_sgn_hM) * H_sgn_ND * tc_nin(6))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_sgn_m_Volume, H3_sgn_hM) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H3_sgn_g_Volume * H_sgn_NG);
  void H3_sgn_Init( void ) {}
  void H3_sgn_Quit( void ) {}
};
