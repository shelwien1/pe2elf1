
static const byte H5_hdr_a_fld[3+1]={ 0,1,2,3 };
static const byte H5_hdr_a_p1[7+1]={ 0,1,2,2,2,3,4,4 };


static const byte H5_hdr_c_fld[15+1]={ 0,1,1,2,2,2,3,3,4,4,4,4,4,4,4,5 };

static const byte H5_hdr_d_p1[15+1]={ 0,1,2,3,3,3,3,3,3,4,4,4,5,5,5,5 };
static const byte H5_hdr_d_p2[7+1]={ 0,1,2,3,4,5,5,6 };


static const byte H5_hdr_g_p1[7+1]={ 0,1,2,3,4,5,6,6 };

static const byte H5_hdr_h_sty[9+1]={ 0,0,1,2,2,3,4,4,4,4 };


static const byte H5_hdr_j_p2[7+1]={ 0,1,1,1,1,2,2,3 };

static const byte H5_hdr_k_fld[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };






static const int H5_hdr_rA = (192+0) * (1);
static const int H5_hdr_rB = (71+0) * (1);
static const int H5_hdr_rC = (0+0) * (1);
static const int H5_hdr_rE = (0+0) * (1);
static const int H5_hdr_rH = (115+0) * (1);
static const int H5_hdr_rI = (73+0) * (1);
static const int H5_hdr_rJ = (68+0) * (1);
static const int H5_hdr_rK = (69+0) * (1);
static const int H5_hdr_rL = (252+0) * (1);
static const int H5_hdr_rD = (253+0) * (1);
static const int H5_hdr_rT = (2+0) * (1);
static const int H5_hdr_rG = (64+0) * (1);
static const int H5_hdr_lr = (20+0) * (1);
static const int H5_hdr_lrm = (0+0) * (1);
static const int H5_hdr_mbm = (0+0) * (1);
static const int H5_hdr_mwA = (0+0) * (1);
static const int H5_hdr_mwB = (4+0) * (1);
static const int H5_hdr_mwC = (10+0) * (1);
static const int H5_hdr_mwE = (163+0) * (1);
static const int H5_hdr_mwH = (3+0) * (1);
static const int H5_hdr_mwI = (137+0) * (1);
static const int H5_hdr_mwJ = (32+0) * (1);
static const int H5_hdr_mwK = (0+0) * (1);
static const int H5_hdr_mwL = (0+0) * (1);
static const int H5_hdr_mwD = (5+0) * (1);
static const int H5_hdr_mwT = (7+0) * (1);
static const int H5_hdr_mwG = (0+0) * (1);
static const int H5_hdr_mt = (0+0) * (1);
static const int H5_hdr_mb = (242+0) * (1);
static const int H5_hdr_hA = (24+0) * (1);
static const int H5_hdr_hB = (24+0) * (1);
static const int H5_hdr_hC = (24+0) * (1);
static const int H5_hdr_hE = (24+0) * (1);
static const int H5_hdr_hH = (24+0) * (1);
static const int H5_hdr_hI = (24+0) * (1);
static const int H5_hdr_hJ = (24+0) * (1);
static const int H5_hdr_hK = (24+0) * (1);
static const int H5_hdr_hL = (24+0) * (1);
static const int H5_hdr_hD = (24+0) * (1);
static const int H5_hdr_hM = (24+0) * (1);
static const int H5_hdr_hT = (24+0) * (1);
static const int H5_hdr_hN = (24+0) * (1);


static const long long H5_hdr_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), 4), 5), 3), 2); // 7680
static const long long H5_hdr_b_Volume = tc_vmul(tc_vmul(1, (1<<7)), (1<<8)); // 32768
static const long long H5_hdr_c_Volume = tc_vmul(tc_vmul(1, (1<<7)), 6); // 768
static const long long H5_hdr_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), 6), 7); // 2688
static const long long H5_hdr_e_Volume = tc_vmul(tc_vmul(1, (1<<3)), 2); // 16
static const long long H5_hdr_g_Volume = tc_vmul(tc_vmul(1, 7), 1); // 7
static const long long H5_hdr_h_Volume = tc_vmul(tc_vmul(1, (1<<5)), 5); // 160
static const long long H5_hdr_i_Volume = tc_vmul(1, (1<<6)); // 64
static const long long H5_hdr_j_Volume = tc_vmul(tc_vmul(1, 2), 4); // 8
static const long long H5_hdr_k_Volume = tc_vmul(tc_vmul(1, (1<<7)), 16); // 2048
static const long long H5_hdr_l_Volume = tc_vmul(1, 2); // 2
static const long long H5_hdr_m_Volume = 1; // 1
static const long long H5_hdr_n_Volume = 1; // 1
static const long long H5_hdr_t_Volume = tc_vmul(1, (1<<4)); // 16

#define H5_hdr_USES(X) X(tag) X(fld) X(p1) X(sty) X(pr) X(p1r) X(p2) X(pos)

struct H5_hdr_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H5_hdr_A[tc_rows(H5_hdr_a_Volume, H5_hdr_hA) * H_hdr_ND];
  cm_cnt2 H5_hdr_B[tc_rows(H5_hdr_b_Volume, H5_hdr_hB) * H_hdr_ND];
  cm_cnt2 H5_hdr_C[tc_rows(H5_hdr_c_Volume, H5_hdr_hC) * H_hdr_ND];
  cm_cnt2 H5_hdr_D[tc_rows(H5_hdr_d_Volume, H5_hdr_hD) * H_hdr_ND];
  cm_cnt2 H5_hdr_E[tc_rows(H5_hdr_e_Volume, H5_hdr_hE) * H_hdr_ND];
  cm_cnt2 H5_hdr_H[tc_rows(H5_hdr_h_Volume, H5_hdr_hH) * H_hdr_ND];
  cm_cnt2 H5_hdr_I[tc_rows(H5_hdr_i_Volume, H5_hdr_hI) * H_hdr_ND];
  cm_cnt2 H5_hdr_J[tc_rows(H5_hdr_j_Volume, H5_hdr_hJ) * H_hdr_ND];
  cm_cnt2 H5_hdr_K[tc_rows(H5_hdr_k_Volume, H5_hdr_hK) * H_hdr_ND];
  cm_cnt2 H5_hdr_L[tc_rows(H5_hdr_l_Volume, H5_hdr_hL) * H_hdr_ND];
  cm_cnt H5_hdr_MT[tc_rows(H5_hdr_t_Volume, H5_hdr_hT) * H_hdr_MND];
  i32 H5_hdr_W[tc_rows(H5_hdr_m_Volume, H5_hdr_hM) * H_hdr_ND * 24];
  u8 H5_hdr_WC[tc_rows(H5_hdr_m_Volume, H5_hdr_hM) * H_hdr_ND];
  i32 H5_hdr_WM[tc_rows(H5_hdr_n_Volume, H5_hdr_hN) * H_hdr_MND * 3];
  u8 H5_hdr_WMC[tc_rows(H5_hdr_n_Volume, H5_hdr_hN) * H_hdr_MND];
  cm_cnt H5_hdr_G[H5_hdr_g_Volume * H_hdr_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H5_hdr_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_hdr_a_Volume, H5_hdr_hA) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_hdr_b_Volume, H5_hdr_hB) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_hdr_c_Volume, H5_hdr_hC) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_hdr_d_Volume, H5_hdr_hD) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_hdr_e_Volume, H5_hdr_hE) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_hdr_h_Volume, H5_hdr_hH) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_hdr_i_Volume, H5_hdr_hI) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_hdr_j_Volume, H5_hdr_hJ) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_hdr_k_Volume, H5_hdr_hK) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_hdr_l_Volume, H5_hdr_hL) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H5_hdr_t_Volume, H5_hdr_hT) * H_hdr_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H5_hdr_m_Volume, H5_hdr_hM) * H_hdr_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H5_hdr_m_Volume, H5_hdr_hM) * H_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H5_hdr_n_Volume, H5_hdr_hN) * H_hdr_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H5_hdr_n_Volume, H5_hdr_hN) * H_hdr_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H5_hdr_g_Volume * H_hdr_NG);
  void H5_hdr_Init( void ) {
  
  }
  void H5_hdr_Quit( void ) {}
};
