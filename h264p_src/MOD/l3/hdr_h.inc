
static const byte H3_hdr_a_fld[3+1]={ 0,1,2,3 };
static const byte H3_hdr_a_p1[7+1]={ 0,1,2,2,2,3,4,5 };
static const byte H3_hdr_a_sty[9+1]={ 0,0,1,2,3,3,3,3,3,3 };


static const byte H3_hdr_c_fld[15+1]={ 0,1,1,2,2,2,3,3,4,4,4,4,4,4,4,5 };

static const byte H3_hdr_d_p1[15+1]={ 0,1,2,3,3,4,4,4,5,5,6,7,8,9,9,10 };
static const byte H3_hdr_d_p2[7+1]={ 0,1,2,3,4,4,4,4 };


static const byte H3_hdr_g_p1[7+1]={ 0,1,2,3,4,5,6,6 };

static const byte H3_hdr_h_sty[9+1]={ 0,0,1,2,3,3,3,3,3,3 };


static const byte H3_hdr_j_p2[7+1]={ 0,1,1,2,3,3,3,4 };

static const byte H3_hdr_k_fld[15+1]={ 0,1,2,3,3,4,5,6,7,8,9,10,11,12,13,14 };








static const int H3_hdr_rA = (131+0) * (1);
static const int H3_hdr_rB = (0+0) * (1);
static const int H3_hdr_rC = (0+0) * (1);
static const int H3_hdr_rE = (0+0) * (1);
static const int H3_hdr_rH = (0+0) * (1);
static const int H3_hdr_rI = (0+0) * (1);
static const int H3_hdr_rJ = (242+0) * (1);
static const int H3_hdr_rK = (198+0) * (1);
static const int H3_hdr_rL = (195+0) * (1);
static const int H3_hdr_rD = (249+0) * (1);
static const int H3_hdr_rT = (2+0) * (1);
static const int H3_hdr_rG = (8+0) * (1);
static const int H3_hdr_lr = (18+0) * (1);
static const int H3_hdr_lrm = (0+0) * (1);
static const int H3_hdr_mbm = (0+0) * (1);
static const int H3_hdr_mwA = (0+0) * (1);
static const int H3_hdr_mwB = (6+0) * (1);
static const int H3_hdr_mwC = (7+0) * (1);
static const int H3_hdr_mwE = (110+0) * (1);
static const int H3_hdr_mwH = (8+0) * (1);
static const int H3_hdr_mwI = (128+0) * (1);
static const int H3_hdr_mwJ = (12+0) * (1);
static const int H3_hdr_mwK = (0+0) * (1);
static const int H3_hdr_mwL = (190+0) * (1);
static const int H3_hdr_mwD = (5+0) * (1);
static const int H3_hdr_mwT = (8+0) * (1);
static const int H3_hdr_mwG = (0+0) * (1);
static const int H3_hdr_mt = (0+0) * (1);
static const int H3_hdr_mb = (130+0) * (1);
static const int H3_hdr_hA = (24+0) * (1);
static const int H3_hdr_hB = (24+0) * (1);
static const int H3_hdr_hC = (24+0) * (1);
static const int H3_hdr_hE = (24+0) * (1);
static const int H3_hdr_hH = (24+0) * (1);
static const int H3_hdr_hI = (24+0) * (1);
static const int H3_hdr_hJ = (24+0) * (1);
static const int H3_hdr_hK = (24+0) * (1);
static const int H3_hdr_hL = (24+0) * (1);
static const int H3_hdr_hD = (24+0) * (1);
static const int H3_hdr_hS = (24+0) * (1);
static const int H3_hdr_hM = (24+0) * (1);
static const int H3_hdr_hT = (24+0) * (1);
static const int H3_hdr_hN = (24+0) * (1);


static const long long H3_hdr_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<5)), 4), 6), 4), 2); // 6144
static const long long H3_hdr_b_Volume = tc_vmul(tc_vmul(1, (1<<5)), (1<<7)); // 4096
static const long long H3_hdr_c_Volume = tc_vmul(tc_vmul(1, (1<<7)), 6); // 768
static const long long H3_hdr_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<7)), 11), 5); // 7040
static const long long H3_hdr_e_Volume = tc_vmul(1, 2); // 2
static const long long H3_hdr_g_Volume = tc_vmul(tc_vmul(1, 7), 1); // 7
static const long long H3_hdr_h_Volume = tc_vmul(tc_vmul(1, (1<<5)), 4); // 128
static const long long H3_hdr_i_Volume = tc_vmul(1, (1<<3)); // 8
static const long long H3_hdr_j_Volume = tc_vmul(tc_vmul(1, 2), 5); // 10
static const long long H3_hdr_k_Volume = tc_vmul(tc_vmul(1, (1<<6)), 15); // 960
static const long long H3_hdr_l_Volume = tc_vmul(tc_vmul(1, (1<<3)), 3); // 24
static const long long H3_hdr_m_Volume = 1; // 1
static const long long H3_hdr_n_Volume = 1; // 1
static const long long H3_hdr_s_Volume = tc_vmul(1, (1<<6)); // 64
static const long long H3_hdr_t_Volume = tc_vmul(1, (1<<4)); // 16
static const long long H3_hdr_w_Volume = tc_vmul(1, (1<<7)); // 128

#define H3_hdr_USES(X) X(tag) X(fld) X(p1) X(sty) X(pr) X(p1r) X(p2) X(pos)

struct H3_hdr_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H3_hdr_A[tc_rows(H3_hdr_a_Volume, H3_hdr_hA) * H_hdr_ND];
  cm_cnt2 H3_hdr_B[tc_rows(H3_hdr_b_Volume, H3_hdr_hB) * H_hdr_ND];
  cm_cnt2 H3_hdr_C[tc_rows(H3_hdr_c_Volume, H3_hdr_hC) * H_hdr_ND];
  cm_cnt2 H3_hdr_D[tc_rows(H3_hdr_d_Volume, H3_hdr_hD) * H_hdr_ND];
  cm_cnt2 H3_hdr_E[tc_rows(H3_hdr_e_Volume, H3_hdr_hE) * H_hdr_ND];
  cm_cnt2 H3_hdr_H[tc_rows(H3_hdr_h_Volume, H3_hdr_hH) * H_hdr_ND];
  cm_cnt2 H3_hdr_I[tc_rows(H3_hdr_i_Volume, H3_hdr_hI) * H_hdr_ND];
  cm_cnt2 H3_hdr_J[tc_rows(H3_hdr_j_Volume, H3_hdr_hJ) * H_hdr_ND];
  cm_cnt2 H3_hdr_K[tc_rows(H3_hdr_k_Volume, H3_hdr_hK) * H_hdr_ND];
  cm_cnt2 H3_hdr_L[tc_rows(H3_hdr_l_Volume, H3_hdr_hL) * H_hdr_ND];
  cm_cnt H3_hdr_MT[tc_rows(H3_hdr_t_Volume, H3_hdr_hT) * H_hdr_MND];
  i32 H3_hdr_W[tc_rows(H3_hdr_m_Volume, H3_hdr_hM) * H_hdr_ND * 24];
  u8 H3_hdr_WC[tc_rows(H3_hdr_m_Volume, H3_hdr_hM) * H_hdr_ND];
  i32 H3_hdr_WM[tc_rows(H3_hdr_n_Volume, H3_hdr_hN) * H_hdr_MND * 3];
  u8 H3_hdr_WMC[tc_rows(H3_hdr_n_Volume, H3_hdr_hN) * H_hdr_MND];
  i32 H3_hdr_W2[H3_hdr_w_Volume * H_hdr_ND * 24];
  u8 H3_hdr_W2C[H3_hdr_w_Volume * H_hdr_ND];
  i32 H3_hdr_W3[tc_rows(H3_hdr_s_Volume, H3_hdr_hS) * H_hdr_ND * 24];
  u8 H3_hdr_W3C[tc_rows(H3_hdr_s_Volume, H3_hdr_hS) * H_hdr_ND];
  i32 H3_hdr_WF[H_hdr_ND * 5 * 4];
  u8 H3_hdr_WFC[H_hdr_ND * 5];
  cm_cnt H3_hdr_G[H3_hdr_g_Volume * H_hdr_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H3_hdr_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_hdr_a_Volume, H3_hdr_hA) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_hdr_b_Volume, H3_hdr_hB) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_hdr_c_Volume, H3_hdr_hC) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_hdr_d_Volume, H3_hdr_hD) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_hdr_e_Volume, H3_hdr_hE) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_hdr_h_Volume, H3_hdr_hH) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_hdr_i_Volume, H3_hdr_hI) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_hdr_j_Volume, H3_hdr_hJ) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_hdr_k_Volume, H3_hdr_hK) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_hdr_l_Volume, H3_hdr_hL) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H3_hdr_t_Volume, H3_hdr_hT) * H_hdr_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_hdr_m_Volume, H3_hdr_hM) * H_hdr_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_hdr_m_Volume, H3_hdr_hM) * H_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_hdr_n_Volume, H3_hdr_hN) * H_hdr_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_hdr_n_Volume, H3_hdr_hN) * H_hdr_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H3_hdr_w_Volume * H_hdr_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H3_hdr_w_Volume * H_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_hdr_s_Volume, H3_hdr_hS) * H_hdr_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_hdr_s_Volume, H3_hdr_hS) * H_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_hdr_ND * 5 * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_hdr_ND * 5)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H3_hdr_g_Volume * H_hdr_NG);
  void H3_hdr_Init( void ) {
  
  }
};
