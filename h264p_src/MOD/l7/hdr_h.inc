
static const byte H7_hdr_a_fld[3+1]={ 0,1,2,3 };
static const byte H7_hdr_a_p1[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H7_hdr_a_sty[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H7_hdr_a_pr[3+1]={ 0,1,2,3 };


static const byte H7_hdr_c_fld[15+1]={ 0,1,1,1,2,2,3,4,5,5,6,6,6,6,6,6 };

static const byte H7_hdr_d_p1[15+1]={ 0,1,2,3,3,3,4,4,5,5,5,6,6,6,7,8 };
static const byte H7_hdr_d_p2[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H7_hdr_e_pos[3+1]={ 0,1,2,3 };

static const byte H7_hdr_g_p1[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H7_hdr_g_pos[3+1]={ 0,1,2,3 };

static const byte H7_hdr_h_sty[9+1]={ 0,1,2,3,4,5,6,7,8,9 };


static const byte H7_hdr_j_fld[3+1]={ 0,1,2,3 };
static const byte H7_hdr_j_p2[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H7_hdr_k_fld[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

static const byte H7_hdr_l_pos[3+1]={ 0,1,2,3 };







static const int H7_hdr_rA = (128+0) * (1);
static const int H7_hdr_rB = (0+0) * (1);
static const int H7_hdr_rC = (0+0) * (1);
static const int H7_hdr_rE = (0+0) * (1);
static const int H7_hdr_rH = (64+0) * (1);
static const int H7_hdr_rI = (64+0) * (1);
static const int H7_hdr_rJ = (64+0) * (1);
static const int H7_hdr_rK = (64+0) * (1);
static const int H7_hdr_rL = (64+0) * (1);
static const int H7_hdr_rD = (255+0) * (1);
static const int H7_hdr_rT = (36+0) * (1);
static const int H7_hdr_rG = (90+0) * (1);
static const int H7_hdr_lr = (17+0) * (1);
static const int H7_hdr_lrm = (12+0) * (1);
static const int H7_hdr_mbm = (0+0) * (1);
static const int H7_hdr_mwA = (0+0) * (1);
static const int H7_hdr_mwB = (4+0) * (1);
static const int H7_hdr_mwC = (0+0) * (1);
static const int H7_hdr_mwE = (0+0) * (1);
static const int H7_hdr_mwH = (2+0) * (1);
static const int H7_hdr_mwI = (0+0) * (1);
static const int H7_hdr_mwJ = (0+0) * (1);
static const int H7_hdr_mwK = (1+0) * (1);
static const int H7_hdr_mwL = (0+0) * (1);
static const int H7_hdr_mwD = (12+0) * (1);
static const int H7_hdr_mwT = (0+0) * (1);
static const int H7_hdr_mwG = (0+0) * (1);
static const int H7_hdr_mt = (6+0) * (1);
static const int H7_hdr_mb = (226+0) * (1);
static const int H7_hdr_hA = (24+0) * (1);
static const int H7_hdr_hB = (24+0) * (1);
static const int H7_hdr_hC = (24+0) * (1);
static const int H7_hdr_hE = (24+0) * (1);
static const int H7_hdr_hH = (24+0) * (1);
static const int H7_hdr_hI = (24+0) * (1);
static const int H7_hdr_hJ = (24+0) * (1);
static const int H7_hdr_hK = (24+0) * (1);
static const int H7_hdr_hL = (24+0) * (1);
static const int H7_hdr_hD = (24+0) * (1);
static const int H7_hdr_hS = (24+0) * (1);
static const int H7_hdr_hM = (24+0) * (1);
static const int H7_hdr_hT = (24+0) * (1);
static const int H7_hdr_hN = (24+0) * (1);


static const long long H7_hdr_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), 4), 8), 10), 4); // 81920
static const long long H7_hdr_b_Volume = tc_vmul(tc_vmul(1, (1<<5)), (1<<4)); // 512
static const long long H7_hdr_c_Volume = tc_vmul(tc_vmul(1, (1<<8)), 7); // 1792
static const long long H7_hdr_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 9), 8); // 18432
static const long long H7_hdr_e_Volume = tc_vmul(tc_vmul(1, (1<<3)), 4); // 32
static const long long H7_hdr_g_Volume = tc_vmul(tc_vmul(1, 8), 4); // 32
static const long long H7_hdr_h_Volume = tc_vmul(tc_vmul(1, (1<<6)), 10); // 640
static const long long H7_hdr_i_Volume = tc_vmul(1, (1<<8)); // 256
static const long long H7_hdr_j_Volume = tc_vmul(tc_vmul(1, 4), 8); // 32
static const long long H7_hdr_k_Volume = tc_vmul(tc_vmul(1, (1<<8)), 16); // 4096
static const long long H7_hdr_l_Volume = tc_vmul(tc_vmul(1, (1<<4)), 4); // 64
static const long long H7_hdr_m_Volume = 1; // 1
static const long long H7_hdr_n_Volume = tc_vmul(1, (1<<8)); // 256
static const long long H7_hdr_s_Volume = tc_vmul(1, (1<<8)); // 256
static const long long H7_hdr_t_Volume = tc_vmul(1, (1<<8)); // 256
static const long long H7_hdr_w_Volume = tc_vmul(1, (1<<5)); // 32

#define H7_hdr_USES(X) X(tag) X(fld) X(p1) X(sty) X(pr) X(p1r) X(p2) X(pos)

struct H7_hdr_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H7_hdr_A[tc_rows(H7_hdr_a_Volume, H7_hdr_hA) * H_hdr_ND];
  cm_cnt2 H7_hdr_B[tc_rows(H7_hdr_b_Volume, H7_hdr_hB) * H_hdr_ND];
  cm_cnt2 H7_hdr_C[tc_rows(H7_hdr_c_Volume, H7_hdr_hC) * H_hdr_ND];
  cm_cnt2 H7_hdr_D[tc_rows(H7_hdr_d_Volume, H7_hdr_hD) * H_hdr_ND];
  cm_cnt2 H7_hdr_E[tc_rows(H7_hdr_e_Volume, H7_hdr_hE) * H_hdr_ND];
  cm_cnt2 H7_hdr_H[tc_rows(H7_hdr_h_Volume, H7_hdr_hH) * H_hdr_ND];
  cm_cnt2 H7_hdr_I[tc_rows(H7_hdr_i_Volume, H7_hdr_hI) * H_hdr_ND];
  cm_cnt2 H7_hdr_J[tc_rows(H7_hdr_j_Volume, H7_hdr_hJ) * H_hdr_ND];
  cm_cnt2 H7_hdr_K[tc_rows(H7_hdr_k_Volume, H7_hdr_hK) * H_hdr_ND];
  cm_cnt2 H7_hdr_L[tc_rows(H7_hdr_l_Volume, H7_hdr_hL) * H_hdr_ND];
  cm_cnt H7_hdr_MT[tc_rows(H7_hdr_t_Volume, H7_hdr_hT) * H_hdr_MND];
  i32 H7_hdr_W[tc_rows(H7_hdr_m_Volume, H7_hdr_hM) * H_hdr_ND * 24];
  u8 H7_hdr_WC[tc_rows(H7_hdr_m_Volume, H7_hdr_hM) * H_hdr_ND];
  i32 H7_hdr_WM[tc_rows(H7_hdr_n_Volume, H7_hdr_hN) * H_hdr_MND * 3];
  u8 H7_hdr_WMC[tc_rows(H7_hdr_n_Volume, H7_hdr_hN) * H_hdr_MND];
  i32 H7_hdr_W2[H7_hdr_w_Volume * H_hdr_ND * 24];
  u8 H7_hdr_W2C[H7_hdr_w_Volume * H_hdr_ND];
  i32 H7_hdr_W3[tc_rows(H7_hdr_s_Volume, H7_hdr_hS) * H_hdr_ND * 24];
  u8 H7_hdr_W3C[tc_rows(H7_hdr_s_Volume, H7_hdr_hS) * H_hdr_ND];
  i32 H7_hdr_WF[H_hdr_ND * 5 * 4];
  u8 H7_hdr_WFC[H_hdr_ND * 5];
  cm_cnt H7_hdr_G[H7_hdr_g_Volume * H_hdr_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H7_hdr_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_hdr_a_Volume, H7_hdr_hA) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_hdr_b_Volume, H7_hdr_hB) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_hdr_c_Volume, H7_hdr_hC) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_hdr_d_Volume, H7_hdr_hD) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_hdr_e_Volume, H7_hdr_hE) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_hdr_h_Volume, H7_hdr_hH) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_hdr_i_Volume, H7_hdr_hI) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_hdr_j_Volume, H7_hdr_hJ) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_hdr_k_Volume, H7_hdr_hK) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_hdr_l_Volume, H7_hdr_hL) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H7_hdr_t_Volume, H7_hdr_hT) * H_hdr_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H7_hdr_m_Volume, H7_hdr_hM) * H_hdr_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H7_hdr_m_Volume, H7_hdr_hM) * H_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H7_hdr_n_Volume, H7_hdr_hN) * H_hdr_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H7_hdr_n_Volume, H7_hdr_hN) * H_hdr_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H7_hdr_w_Volume * H_hdr_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H7_hdr_w_Volume * H_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H7_hdr_s_Volume, H7_hdr_hS) * H_hdr_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H7_hdr_s_Volume, H7_hdr_hS) * H_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_hdr_ND * 5 * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_hdr_ND * 5)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H7_hdr_g_Volume * H_hdr_NG);
  void H7_hdr_Init( void ) {
  
  }
};
