



static const byte A5_hdr_d_pos[31+1]={ 0,1,2,3,3,4,4,5,5,5,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7 };

static const byte A5_hdr_e_nch[3+1]={ 0,1,2,3 };


static const byte A5_hdr_h_pos[7+1]={ 0,1,2,3,3,4,4,4 };

static const byte A5_hdr_i_sk[4+1]={ 0,1,2,3,4 };


static const byte A5_hdr_k_pos[31+1]={ 0,1,2,2,2,2,3,4,5,6,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8 };

static const byte A5_hdr_l_nch[3+1]={ 0,1,2,3 };





static const int A5_hdr_rA = (0+0) * (1);
static const int A5_hdr_mwA = (12+0) * (1);
static const int A5_hdr_rB = (255+0) * (1);
static const int A5_hdr_mwB = (20+0) * (1);
static const int A5_hdr_rC = (254+0) * (1);
static const int A5_hdr_mwC = (8+0) * (1);
static const int A5_hdr_rD = (255+0) * (1);
static const int A5_hdr_mwD = (5+0) * (1);
static const int A5_hdr_rE = (164+0) * (1);
static const int A5_hdr_mwE = (0+0) * (1);
static const int A5_hdr_rH = (191+0) * (1);
static const int A5_hdr_mwH = (4+0) * (1);
static const int A5_hdr_rI = (223+0) * (1);
static const int A5_hdr_mwI = (4+0) * (1);
static const int A5_hdr_rJ = (252+0) * (1);
static const int A5_hdr_mwJ = (1+0) * (1);
static const int A5_hdr_rK = (255+0) * (1);
static const int A5_hdr_mwK = (32+0) * (1);
static const int A5_hdr_rL = (251+0) * (1);
static const int A5_hdr_mwL = (4+0) * (1);
static const int A5_hdr_lr = (14+0) * (1);
static const int A5_hdr_mb = (129+0) * (1);
static const int A5_hdr_rT = (128+0) * (1);
static const int A5_hdr_mwT = (31+0) * (1);
static const int A5_hdr_mt = (6+0) * (1);
static const int A5_hdr_lrm = (31+0) * (1);
static const int A5_hdr_mbm = (25+0) * (1);
static const int A5_hdr_rG = (255+0) * (1);
static const int A5_hdr_mwG = (96+0) * (1);
static const int A5_hdr_hA = (22+0) * (1);
static const int A5_hdr_hB = (22+0) * (1);
static const int A5_hdr_hC = (22+0) * (1);
static const int A5_hdr_hD = (22+0) * (1);
static const int A5_hdr_hE = (22+0) * (1);
static const int A5_hdr_hH = (22+0) * (1);
static const int A5_hdr_hI = (22+0) * (1);
static const int A5_hdr_hJ = (22+0) * (1);
static const int A5_hdr_hK = (22+0) * (1);
static const int A5_hdr_hL = (22+0) * (1);
static const int A5_hdr_hM = (22+0) * (1);
static const int A5_hdr_hT = (22+0) * (1);
static const int A5_hdr_hN = (22+0) * (1);


static const long long A5_hdr_a_Volume = tc_vmul(1, (1<<1)); // 2
static const long long A5_hdr_b_Volume = tc_vmul(tc_vmul(1, (1<<7)), (1<<4)); // 2048
static const long long A5_hdr_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<2)), (1<<4)), 1); // 64
static const long long A5_hdr_d_Volume = tc_vmul(tc_vmul(1, (1<<7)), 8); // 1024
static const long long A5_hdr_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<7)), 3), 4); // 1536
static const long long A5_hdr_g_Volume = tc_vmul(1, (1<<7)); // 128
static const long long A5_hdr_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<7)), (1<<5)), 5); // 20480
static const long long A5_hdr_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<4)), 5), (1<<5)); // 2560
static const long long A5_hdr_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), 2), 2); // 256
static const long long A5_hdr_k_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<7)), (1<<5)), 9), 1); // 36864
static const long long A5_hdr_l_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<4)), 4), 1), 2); // 128
static const long long A5_hdr_m_Volume = tc_vmul(1, (1<<5)); // 32
static const long long A5_hdr_n_Volume = tc_vmul(1, (1<<7)); // 128
static const long long A5_hdr_t_Volume = tc_vmul(1, (1<<4)); // 16

#define A5_hdr_USES(X) X(fld) X(lv) X(pos) X(st) X(nch) X(sk) X(l2)

struct A5_hdr_T {
  cm_cnt2 A5_hdr_A[tc_rows(A5_hdr_a_Volume, A5_hdr_hA) * M_hdr_ND];
  cm_cnt2 A5_hdr_B[tc_rows(A5_hdr_b_Volume, A5_hdr_hB) * M_hdr_ND];
  cm_cnt2 A5_hdr_C[tc_rows(A5_hdr_c_Volume, A5_hdr_hC) * M_hdr_ND];
  cm_cnt2 A5_hdr_D[tc_rows(A5_hdr_d_Volume, A5_hdr_hD) * M_hdr_ND];
  cm_cnt2 A5_hdr_E[tc_rows(A5_hdr_e_Volume, A5_hdr_hE) * M_hdr_ND];
  cm_cnt2 A5_hdr_H[tc_rows(A5_hdr_h_Volume, A5_hdr_hH) * M_hdr_ND];
  cm_cnt2 A5_hdr_I[tc_rows(A5_hdr_i_Volume, A5_hdr_hI) * M_hdr_ND];
  cm_cnt2 A5_hdr_J[tc_rows(A5_hdr_j_Volume, A5_hdr_hJ) * M_hdr_ND];
  cm_cnt2 A5_hdr_K[tc_rows(A5_hdr_k_Volume, A5_hdr_hK) * M_hdr_ND];
  cm_cnt2 A5_hdr_L[tc_rows(A5_hdr_l_Volume, A5_hdr_hL) * M_hdr_ND];
  cm_cnt A5_hdr_MT[tc_rows(A5_hdr_t_Volume, A5_hdr_hT) * M_hdr_MND];
  i32 A5_hdr_W[tc_rows(A5_hdr_m_Volume, A5_hdr_hM) * M_hdr_ND * tc_nin(10)];
  u8 A5_hdr_WC[tc_rows(A5_hdr_m_Volume, A5_hdr_hM) * M_hdr_ND];
  i32 A5_hdr_WM[tc_rows(A5_hdr_n_Volume, A5_hdr_hN) * M_hdr_MND * TC_MIN];
  u8 A5_hdr_WMC[tc_rows(A5_hdr_n_Volume, A5_hdr_hN) * M_hdr_MND];
  cm_cnt A5_hdr_G[A5_hdr_g_Volume * M_hdr_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long A5_hdr_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A5_hdr_a_Volume, A5_hdr_hA) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A5_hdr_b_Volume, A5_hdr_hB) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A5_hdr_c_Volume, A5_hdr_hC) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A5_hdr_d_Volume, A5_hdr_hD) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A5_hdr_e_Volume, A5_hdr_hE) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A5_hdr_h_Volume, A5_hdr_hH) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A5_hdr_i_Volume, A5_hdr_hI) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A5_hdr_j_Volume, A5_hdr_hJ) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A5_hdr_k_Volume, A5_hdr_hK) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A5_hdr_l_Volume, A5_hdr_hL) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(A5_hdr_t_Volume, A5_hdr_hT) * M_hdr_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(A5_hdr_m_Volume, A5_hdr_hM) * M_hdr_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(A5_hdr_m_Volume, A5_hdr_hM) * M_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(A5_hdr_n_Volume, A5_hdr_hN) * M_hdr_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(A5_hdr_n_Volume, A5_hdr_hN) * M_hdr_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(A5_hdr_g_Volume * M_hdr_NG);
  void A5_hdr_Init( void ) {}
  void A5_hdr_Quit( void ) {}
};
