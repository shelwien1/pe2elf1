
static const byte H1_sgn_a_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };

static const byte H1_sgn_b_mg[4+1]={ 0,1,2,3,4 };

static const byte H1_sgn_c_mg[7+1]={ 0,1,2,3,4,5,6,7 };



static const byte H1_sgn_m_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };



static const int H1_sgn_rA = (159+0) * (1);
static const int H1_sgn_rB = (255+0) * (1);
static const int H1_sgn_rC = (255+0) * (1);
static const int H1_sgn_rD = (234+0) * (1);
static const int H1_sgn_lr = (15+0) * (1);
static const int H1_sgn_mwA = (4+0) * (1);
static const int H1_sgn_mwB = (31+0) * (1);
static const int H1_sgn_mwC = (29+0) * (1);
static const int H1_sgn_mwD = (0+0) * (1);
static const int H1_sgn_mb = (32+0) * (1);
static const int H1_sgn_rT = (0+0) * (1);
static const int H1_sgn_mwT = (0+0) * (1);
static const int H1_sgn_mt = (0+0) * (1);
static const int H1_sgn_rG = (0+0) * (1);
static const int H1_sgn_mwG = (0+0) * (1);
static const int H1_sgn_hA = (24+0) * (1);
static const int H1_sgn_hB = (24+0) * (1);
static const int H1_sgn_hC = (24+0) * (1);
static const int H1_sgn_hD = (24+0) * (1);
static const int H1_sgn_hM = (24+0) * (1);
static const int H1_sgn_hT = (24+0) * (1);


static const long long H1_sgn_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 12); // 108
static const long long H1_sgn_b_Volume = tc_vmul(tc_vmul(1, 3), 5); // 15
static const long long H1_sgn_c_Volume = tc_vmul(tc_vmul(1, 3), 8); // 24
static const long long H1_sgn_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 3); // 27
static const long long H1_sgn_g_Volume = 1; // 1
static const long long H1_sgn_m_Volume = tc_vmul(1, 12); // 12
static const long long H1_sgn_t_Volume = 1; // 1

#define H1_sgn_USES(X) X(s1) X(s2) X(cb) X(oc) X(mg) X(pf)

struct H1_sgn_T {
  cm_cnt2s H1_sgn_A[tc_rows(H1_sgn_a_Volume, H1_sgn_hA) * H_sgn_ND];
  cm_cnt2s H1_sgn_B[tc_rows(H1_sgn_b_Volume, H1_sgn_hB) * H_sgn_ND];
  cm_cnt2s H1_sgn_C[tc_rows(H1_sgn_c_Volume, H1_sgn_hC) * H_sgn_ND];
  cm_cnt2s H1_sgn_D[tc_rows(H1_sgn_d_Volume, H1_sgn_hD) * H_sgn_ND];
  cm_cnt H1_sgn_MT[tc_rows(H1_sgn_t_Volume, H1_sgn_hT) * H_sgn_MND];
  i32 H1_sgn_W[tc_rows(H1_sgn_m_Volume, H1_sgn_hM) * H_sgn_ND * tc_nin(4)];
  u8 H1_sgn_WC[tc_rows(H1_sgn_m_Volume, H1_sgn_hM) * H_sgn_ND];
  cm_cnt H1_sgn_G[H1_sgn_g_Volume * H_sgn_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H1_sgn_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_sgn_a_Volume, H1_sgn_hA) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_sgn_b_Volume, H1_sgn_hB) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_sgn_c_Volume, H1_sgn_hC) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_sgn_d_Volume, H1_sgn_hD) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H1_sgn_t_Volume, H1_sgn_hT) * H_sgn_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_sgn_m_Volume, H1_sgn_hM) * H_sgn_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_sgn_m_Volume, H1_sgn_hM) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H1_sgn_g_Volume * H_sgn_NG);
  void H1_sgn_Init( void ) {}
  void H1_sgn_Quit( void ) {}
};
