
static const byte H3_qpg_a_mag[7+1]={ 0,0,1,2,3,4,5,6 };
static const byte H3_qpg_a_ss[9+1]={ 0,0,0,1,2,3,4,5,6,6 };


static const byte H3_qpg_c_cs[9+1]={ 0,0,0,1,1,2,3,4,5,5 };

static const byte H3_qpg_d_bs[9+1]={ 0,0,0,1,1,2,3,4,5,6 };
static const byte H3_qpg_d_mag[7+1]={ 0,0,1,1,2,3,3,3 };

static const byte H3_qpg_e_ss[9+1]={ 0,0,0,0,1,2,3,4,5,5 };



static const byte H3_qpg_i_cs[9+1]={ 0,0,0,1,2,2,3,3,4,5 };
static const byte H3_qpg_i_bs[9+1]={ 0,0,0,1,2,3,4,5,6,7 };

static const byte H3_qpg_j_cls[8+1]={ 0,0,0,1,2,2,3,4,4 };

static const byte H3_qpg_k_cls[8+1]={ 0,0,0,0,1,2,3,4,4 };

static const byte H3_qpg_l_cs[9+1]={ 0,0,0,1,1,2,2,2,2,3 };
static const byte H3_qpg_l_bs[9+1]={ 0,0,0,1,2,3,4,5,5,5 };



static const byte H3_qpg_s_mag[7+1]={ 0,0,1,2,2,3,3,3 };


static const byte H3_qpg_w_cs[9+1]={ 0,0,0,1,2,2,3,4,5,6 };


static const int H3_qpg_rA = (255+0) * (1);
static const int H3_qpg_rB = (0+0) * (1);
static const int H3_qpg_rC = (252+0) * (1);
static const int H3_qpg_rE = (255+0) * (1);
static const int H3_qpg_rH = (175+0) * (1);
static const int H3_qpg_rI = (251+0) * (1);
static const int H3_qpg_rJ = (238+0) * (1);
static const int H3_qpg_rK = (250+0) * (1);
static const int H3_qpg_rL = (249+0) * (1);
static const int H3_qpg_rD = (252+0) * (1);
static const int H3_qpg_rT = (0+0) * (1);
static const int H3_qpg_rG = (0+0) * (1);
static const int H3_qpg_lr = (13+0) * (1);
static const int H3_qpg_lrm = (0+0) * (1);
static const int H3_qpg_mbm = (0+0) * (1);
static const int H3_qpg_mwA = (16+0) * (1);
static const int H3_qpg_mwB = (25+0) * (1);
static const int H3_qpg_mwC = (20+0) * (1);
static const int H3_qpg_mwE = (64+0) * (1);
static const int H3_qpg_mwH = (37+0) * (1);
static const int H3_qpg_mwI = (124+0) * (1);
static const int H3_qpg_mwJ = (64+0) * (1);
static const int H3_qpg_mwK = (4+0) * (1);
static const int H3_qpg_mwL = (87+0) * (1);
static const int H3_qpg_mwD = (0+0) * (1);
static const int H3_qpg_mwT = (0+0) * (1);
static const int H3_qpg_mwG = (0+0) * (1);
static const int H3_qpg_mt = (0+0) * (1);
static const int H3_qpg_mb = (224+0) * (1);
static const int H3_qpg_hA = (24+0) * (1);
static const int H3_qpg_hB = (24+0) * (1);
static const int H3_qpg_hC = (24+0) * (1);
static const int H3_qpg_hE = (24+0) * (1);
static const int H3_qpg_hH = (24+0) * (1);
static const int H3_qpg_hI = (24+0) * (1);
static const int H3_qpg_hJ = (24+0) * (1);
static const int H3_qpg_hK = (24+0) * (1);
static const int H3_qpg_hL = (24+0) * (1);
static const int H3_qpg_hD = (24+0) * (1);
static const int H3_qpg_hS = (24+0) * (1);
static const int H3_qpg_hM = (24+0) * (1);
static const int H3_qpg_hT = (24+0) * (1);
static const int H3_qpg_hN = (24+0) * (1);


static const long long H3_qpg_a_Volume = tc_vmul(tc_vmul(1, 7), 7); // 49
static const long long H3_qpg_b_Volume = tc_vmul(1, 2); // 2
static const long long H3_qpg_c_Volume = tc_vmul(1, 6); // 6
static const long long H3_qpg_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 7), 4), 2); // 56
static const long long H3_qpg_e_Volume = tc_vmul(tc_vmul(1, 3), 6); // 18
static const long long H3_qpg_g_Volume = tc_vmul(1, 1); // 1
static const long long H3_qpg_h_Volume = tc_vmul(1, 3); // 3
static const long long H3_qpg_i_Volume = tc_vmul(tc_vmul(1, 6), 8); // 48
static const long long H3_qpg_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, 5), 3), 3); // 45
static const long long H3_qpg_k_Volume = tc_vmul(tc_vmul(1, 2), 5); // 10
static const long long H3_qpg_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 6), 2); // 48
static const long long H3_qpg_m_Volume = 1; // 1
static const long long H3_qpg_n_Volume = tc_vmul(1, 1); // 1
static const long long H3_qpg_s_Volume = tc_vmul(1, 4); // 4
static const long long H3_qpg_t_Volume = tc_vmul(1, 1); // 1
static const long long H3_qpg_w_Volume = tc_vmul(1, 7); // 7

#define H3_qpg_USES(X) X(mag) X(ss) X(ls) X(cs) X(bs) X(bk) X(cls) X(st)

struct H3_qpg_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H3_qpg_A[tc_rows(H3_qpg_a_Volume, H3_qpg_hA) * H_qpg_ND];
  cm_cnt2 H3_qpg_B[tc_rows(H3_qpg_b_Volume, H3_qpg_hB) * H_qpg_ND];
  cm_cnt2 H3_qpg_C[tc_rows(H3_qpg_c_Volume, H3_qpg_hC) * H_qpg_ND];
  cm_cnt2 H3_qpg_D[tc_rows(H3_qpg_d_Volume, H3_qpg_hD) * H_qpg_ND];
  cm_cnt2 H3_qpg_E[tc_rows(H3_qpg_e_Volume, H3_qpg_hE) * H_qpg_ND];
  cm_cnt2 H3_qpg_H[tc_rows(H3_qpg_h_Volume, H3_qpg_hH) * H_qpg_ND];
  cm_cnt2 H3_qpg_I[tc_rows(H3_qpg_i_Volume, H3_qpg_hI) * H_qpg_ND];
  cm_cnt2 H3_qpg_J[tc_rows(H3_qpg_j_Volume, H3_qpg_hJ) * H_qpg_ND];
  cm_cnt2 H3_qpg_K[tc_rows(H3_qpg_k_Volume, H3_qpg_hK) * H_qpg_ND];
  cm_cnt2 H3_qpg_L[tc_rows(H3_qpg_l_Volume, H3_qpg_hL) * H_qpg_ND];
  cm_cnt H3_qpg_MT[tc_rows(H3_qpg_t_Volume, H3_qpg_hT) * H_qpg_MND];
  i32 H3_qpg_W[tc_rows(H3_qpg_m_Volume, H3_qpg_hM) * H_qpg_ND * 24];
  u8 H3_qpg_WC[tc_rows(H3_qpg_m_Volume, H3_qpg_hM) * H_qpg_ND];
  i32 H3_qpg_WM[tc_rows(H3_qpg_n_Volume, H3_qpg_hN) * H_qpg_MND * 3];
  u8 H3_qpg_WMC[tc_rows(H3_qpg_n_Volume, H3_qpg_hN) * H_qpg_MND];
  i32 H3_qpg_W2[H3_qpg_w_Volume * H_qpg_ND * 24];
  u8 H3_qpg_W2C[H3_qpg_w_Volume * H_qpg_ND];
  i32 H3_qpg_W3[tc_rows(H3_qpg_s_Volume, H3_qpg_hS) * H_qpg_ND * 24];
  u8 H3_qpg_W3C[tc_rows(H3_qpg_s_Volume, H3_qpg_hS) * H_qpg_ND];
  i32 H3_qpg_WF[H_qpg_ND * 5 * 4];
  u8 H3_qpg_WFC[H_qpg_ND * 5];
  cm_cnt H3_qpg_G[H3_qpg_g_Volume * H_qpg_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H3_qpg_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_qpg_a_Volume, H3_qpg_hA) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_qpg_b_Volume, H3_qpg_hB) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_qpg_c_Volume, H3_qpg_hC) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_qpg_d_Volume, H3_qpg_hD) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_qpg_e_Volume, H3_qpg_hE) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_qpg_h_Volume, H3_qpg_hH) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_qpg_i_Volume, H3_qpg_hI) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_qpg_j_Volume, H3_qpg_hJ) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_qpg_k_Volume, H3_qpg_hK) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_qpg_l_Volume, H3_qpg_hL) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H3_qpg_t_Volume, H3_qpg_hT) * H_qpg_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_qpg_m_Volume, H3_qpg_hM) * H_qpg_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_qpg_m_Volume, H3_qpg_hM) * H_qpg_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_qpg_n_Volume, H3_qpg_hN) * H_qpg_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_qpg_n_Volume, H3_qpg_hN) * H_qpg_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H3_qpg_w_Volume * H_qpg_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H3_qpg_w_Volume * H_qpg_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_qpg_s_Volume, H3_qpg_hS) * H_qpg_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_qpg_s_Volume, H3_qpg_hS) * H_qpg_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_qpg_ND * 5 * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_qpg_ND * 5)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H3_qpg_g_Volume * H_qpg_NG);
  void H3_qpg_Init( void ) {
  
  }
  void H3_qpg_Quit( void ) {}
};
