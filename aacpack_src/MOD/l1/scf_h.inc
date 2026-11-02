
static const byte H1_scf_a_oc[13+1]={ 0,1,2,2,3,3,4,4,5,5,6,6,6,6 };

static const byte H1_scf_b_sb[50+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,18,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,33,34,35,36,37,38,39,40,41,42,43,43,44,44,44 };

static const byte H1_scf_c_pa[14+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14 };
static const byte H1_scf_c_d1[5+1]={ 0,0,1,2,3,4 };

static const byte H1_scf_d_pv[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };
static const byte H1_scf_d_sk[15+1]={ 0,1,1,1,2,2,2,2,3,3,3,3,3,3,3,3 };

static const byte H1_scf_g_d1[5+1]={ 0,1,2,3,4,5 };




static const int H1_scf_rA = (255+0) * (1);
static const int H1_scf_rB = (254+0) * (1);
static const int H1_scf_rC = (255+0) * (1);
static const int H1_scf_rD = (255+0) * (1);
static const int H1_scf_rT = (143+0) * (1);
static const int H1_scf_rG = (130+0) * (1);
static const int H1_scf_lr = (15+0) * (1);
static const int H1_scf_mwA = (16+0) * (1);
static const int H1_scf_mwB = (0+0) * (1);
static const int H1_scf_mwC = (1+0) * (1);
static const int H1_scf_mwD = (0+0) * (1);
static const int H1_scf_mwT = (32+0) * (1);
static const int H1_scf_mwG = (2+0) * (1);
static const int H1_scf_mt = (0+0) * (1);
static const int H1_scf_mb = (88+0) * (1);
static const int H1_scf_hA = (24+0) * (1);
static const int H1_scf_hB = (24+0) * (1);
static const int H1_scf_hC = (24+0) * (1);
static const int H1_scf_hD = (24+0) * (1);
static const int H1_scf_hM = (24+0) * (1);
static const int H1_scf_hT = (24+0) * (1);


static const long long H1_scf_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 7), 2), 3); // 42
static const long long H1_scf_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 45), 2); // 270
static const long long H1_scf_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 15), 5); // 225
static const long long H1_scf_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 11), 3), 4); // 396
static const long long H1_scf_g_Volume = tc_vmul(1, 6); // 6
static const long long H1_scf_m_Volume = tc_vmul(tc_vmul(1, 2), 3); // 6
static const long long H1_scf_t_Volume = tc_vmul(1, 2); // 2

#define H1_scf_USES(X) X(oc) X(pa) X(mu) X(kd) X(sb) X(ws) X(d1) X(pv) X(sk)

struct H1_scf_T {
  cm_cnt2s H1_scf_A[tc_rows(H1_scf_a_Volume, H1_scf_hA) * H_scf_ND];
  cm_cnt2s H1_scf_B[tc_rows(H1_scf_b_Volume, H1_scf_hB) * H_scf_ND];
  cm_cnt2s H1_scf_C[tc_rows(H1_scf_c_Volume, H1_scf_hC) * H_scf_ND];
  cm_cnt2s H1_scf_D[tc_rows(H1_scf_d_Volume, H1_scf_hD) * H_scf_ND];
  cm_cnt H1_scf_MT[tc_rows(H1_scf_t_Volume, H1_scf_hT) * H_scf_MND];
  i32 H1_scf_W[tc_rows(H1_scf_m_Volume, H1_scf_hM) * H_scf_ND * tc_nin(4)];
  u8 H1_scf_WC[tc_rows(H1_scf_m_Volume, H1_scf_hM) * H_scf_ND];
  cm_cnt H1_scf_G[H1_scf_g_Volume * H_scf_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H1_scf_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_scf_a_Volume, H1_scf_hA) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_scf_b_Volume, H1_scf_hB) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_scf_c_Volume, H1_scf_hC) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_scf_d_Volume, H1_scf_hD) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H1_scf_t_Volume, H1_scf_hT) * H_scf_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_scf_m_Volume, H1_scf_hM) * H_scf_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_scf_m_Volume, H1_scf_hM) * H_scf_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H1_scf_g_Volume * H_scf_NG);
  void H1_scf_Init( void ) {}
  void H1_scf_Quit( void ) {}
};
