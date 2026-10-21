
static const byte H1_mbt_a_ca[8+1]={ 0,1,2,3,4,4,5,6,6 };
static const byte H1_mbt_a_cb[8+1]={ 0,1,2,3,4,4,5,6,6 };

static const byte H1_mbt_b_st[9+1]={ 0,0,1,2,3,3,3,3,3,3 };
static const byte H1_mbt_b_cc[8+1]={ 0,0,1,2,3,3,4,5,5 };








static const int H1_mbt_rA = (254+0) * (1);
static const int H1_mbt_rB = (246+0) * (1);
static const int H1_mbt_rC = (0+0) * (1);
static const int H1_mbt_rD = (251+0) * (1);
static const int H1_mbt_rT = (0+0) * (1);
static const int H1_mbt_rG = (0+0) * (1);
static const int H1_mbt_lr = (12+0) * (1);
static const int H1_mbt_lrm = (0+0) * (1);
static const int H1_mbt_mbm = (0+0) * (1);
static const int H1_mbt_mwA = (5+0) * (1);
static const int H1_mbt_mwB = (15+0) * (1);
static const int H1_mbt_mwC = (51+0) * (1);
static const int H1_mbt_mwD = (0+0) * (1);
static const int H1_mbt_mwT = (0+0) * (1);
static const int H1_mbt_mwG = (0+0) * (1);
static const int H1_mbt_mt = (0+0) * (1);
static const int H1_mbt_mb = (39+0) * (1);
static const int H1_mbt_hA = (24+0) * (1);
static const int H1_mbt_hB = (24+0) * (1);
static const int H1_mbt_hC = (24+0) * (1);
static const int H1_mbt_hD = (24+0) * (1);
static const int H1_mbt_hM = (24+0) * (1);
static const int H1_mbt_hT = (24+0) * (1);
static const int H1_mbt_hN = (24+0) * (1);


static const long long H1_mbt_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 7), 7); // 147
static const long long H1_mbt_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 6), 2); // 48
static const long long H1_mbt_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 3); // 6
static const long long H1_mbt_d_Volume = tc_vmul(1, 3); // 3
static const long long H1_mbt_g_Volume = tc_vmul(1, 1); // 1
static const long long H1_mbt_m_Volume = tc_vmul(1, 1); // 1
static const long long H1_mbt_n_Volume = tc_vmul(1, 1); // 1
static const long long H1_mbt_t_Volume = tc_vmul(1, 1); // 1

#define H1_mbt_USES(X) X(st) X(ca) X(cb) X(cc) X(nsk)

struct H1_mbt_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2s H1_mbt_A[tc_rows(H1_mbt_a_Volume, H1_mbt_hA) * H_mbt_ND];
  cm_cnt2s H1_mbt_B[tc_rows(H1_mbt_b_Volume, H1_mbt_hB) * H_mbt_ND];
  cm_cnt2s H1_mbt_C[tc_rows(H1_mbt_c_Volume, H1_mbt_hC) * H_mbt_ND];
  cm_cnt2s H1_mbt_D[tc_rows(H1_mbt_d_Volume, H1_mbt_hD) * H_mbt_ND];
  cm_cnt H1_mbt_MT[tc_rows(H1_mbt_t_Volume, H1_mbt_hT) * H_mbt_MND];
  i32 H1_mbt_W[tc_rows(H1_mbt_m_Volume, H1_mbt_hM) * H_mbt_ND * 16];
  u8 H1_mbt_WC[tc_rows(H1_mbt_m_Volume, H1_mbt_hM) * H_mbt_ND];
  i32 H1_mbt_WM[tc_rows(H1_mbt_n_Volume, H1_mbt_hN) * H_mbt_MND * 3];
  u8 H1_mbt_WMC[tc_rows(H1_mbt_n_Volume, H1_mbt_hN) * H_mbt_MND];
  cm_cnt H1_mbt_G[H1_mbt_g_Volume * H_mbt_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H1_mbt_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_mbt_a_Volume, H1_mbt_hA) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_mbt_b_Volume, H1_mbt_hB) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_mbt_c_Volume, H1_mbt_hC) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_mbt_d_Volume, H1_mbt_hD) * H_mbt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H1_mbt_t_Volume, H1_mbt_hT) * H_mbt_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_mbt_m_Volume, H1_mbt_hM) * H_mbt_ND * 16)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_mbt_m_Volume, H1_mbt_hM) * H_mbt_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_mbt_n_Volume, H1_mbt_hN) * H_mbt_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_mbt_n_Volume, H1_mbt_hN) * H_mbt_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H1_mbt_g_Volume * H_mbt_NG);
  void H1_mbt_Init( void ) {
  
  }
  void H1_mbt_Quit( void ) {}
};
