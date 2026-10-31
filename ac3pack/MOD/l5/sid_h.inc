


static const byte A5_sid_c_pos[15+1]={ 0,1,2,3,4,5,6,6,7,8,9,10,11,11,12,13 };






static const byte A5_sid_j_sk[4+1]={ 0,1,2,3,4 };

static const byte A5_sid_k_pos[31+1]={ 0,0,1,2,3,4,5,5,6,7,8,9,10,11,12,13,14,15,15,15,16,17,17,17,17,17,17,17,17,17,17,17 };

static const byte A5_sid_l_blk[5+1]={ 0,1,1,2,2,3 };





static const int A5_sid_rA = (253+0) * (1);
static const int A5_sid_mwA = (32+0) * (1);
static const int A5_sid_rB = (255+0) * (1);
static const int A5_sid_mwB = (4+0) * (1);
static const int A5_sid_rC = (177+0) * (1);
static const int A5_sid_mwC = (0+0) * (1);
static const int A5_sid_rD = (255+0) * (1);
static const int A5_sid_mwD = (0+0) * (1);
static const int A5_sid_rE = (255+0) * (1);
static const int A5_sid_mwE = (0+0) * (1);
static const int A5_sid_rH = (254+0) * (1);
static const int A5_sid_mwH = (9+0) * (1);
static const int A5_sid_rI = (252+0) * (1);
static const int A5_sid_mwI = (2+0) * (1);
static const int A5_sid_rJ = (250+0) * (1);
static const int A5_sid_mwJ = (29+0) * (1);
static const int A5_sid_rK = (255+0) * (1);
static const int A5_sid_mwK = (4+0) * (1);
static const int A5_sid_rL = (254+0) * (1);
static const int A5_sid_mwL = (1+0) * (1);
static const int A5_sid_lr = (15+0) * (1);
static const int A5_sid_mb = (96+0) * (1);
static const int A5_sid_rT = (128+0) * (1);
static const int A5_sid_mwT = (0+0) * (1);
static const int A5_sid_mt = (4+0) * (1);
static const int A5_sid_lrm = (31+0) * (1);
static const int A5_sid_mbm = (25+0) * (1);
static const int A5_sid_rG = (255+0) * (1);
static const int A5_sid_mwG = (188+0) * (1);
static const int A5_sid_hA = (22+0) * (1);
static const int A5_sid_hB = (22+0) * (1);
static const int A5_sid_hC = (22+0) * (1);
static const int A5_sid_hD = (22+0) * (1);
static const int A5_sid_hE = (22+0) * (1);
static const int A5_sid_hH = (22+0) * (1);
static const int A5_sid_hI = (22+0) * (1);
static const int A5_sid_hJ = (22+0) * (1);
static const int A5_sid_hK = (22+0) * (1);
static const int A5_sid_hL = (22+0) * (1);
static const int A5_sid_hM = (22+0) * (1);
static const int A5_sid_hT = (22+0) * (1);
static const int A5_sid_hN = (22+0) * (1);


static const long long A5_sid_a_Volume = 1; // 1
static const long long A5_sid_b_Volume = tc_vmul(tc_vmul(1, (1<<5)), (1<<6)); // 2048
static const long long A5_sid_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), (1<<5)), 14); // 28672
static const long long A5_sid_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), (1<<6)), 2); // 8192
static const long long A5_sid_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), 3), 3); // 576
static const long long A5_sid_g_Volume = tc_vmul(1, (1<<6)); // 64
static const long long A5_sid_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<5)), (1<<5)), 3), 3); // 9216
static const long long A5_sid_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), 2), (1<<4)); // 2048
static const long long A5_sid_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<5)), 5), (1<<1)); // 320
static const long long A5_sid_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), 18), (1<<5)); // 36864
static const long long A5_sid_l_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), 2), 3), 4); // 1536
static const long long A5_sid_m_Volume = tc_vmul(1, (1<<5)); // 32
static const long long A5_sid_n_Volume = tc_vmul(1, (1<<6)); // 64
static const long long A5_sid_t_Volume = tc_vmul(1, (1<<5)); // 32

#define A5_sid_USES(X) X(fld) X(lv) X(pos) X(pq) X(lb) X(st) X(nch) X(pr) X(blk) X(sk)

struct A5_sid_T {
  cm_cnt2 A5_sid_A[tc_rows(A5_sid_a_Volume, A5_sid_hA) * M_sid_ND];
  cm_cnt2 A5_sid_B[tc_rows(A5_sid_b_Volume, A5_sid_hB) * M_sid_ND];
  cm_cnt2 A5_sid_C[tc_rows(A5_sid_c_Volume, A5_sid_hC) * M_sid_ND];
  cm_cnt2 A5_sid_D[tc_rows(A5_sid_d_Volume, A5_sid_hD) * M_sid_ND];
  cm_cnt2 A5_sid_E[tc_rows(A5_sid_e_Volume, A5_sid_hE) * M_sid_ND];
  cm_cnt2 A5_sid_H[tc_rows(A5_sid_h_Volume, A5_sid_hH) * M_sid_ND];
  cm_cnt2 A5_sid_I[tc_rows(A5_sid_i_Volume, A5_sid_hI) * M_sid_ND];
  cm_cnt2 A5_sid_J[tc_rows(A5_sid_j_Volume, A5_sid_hJ) * M_sid_ND];
  cm_cnt2 A5_sid_K[tc_rows(A5_sid_k_Volume, A5_sid_hK) * M_sid_ND];
  cm_cnt2 A5_sid_L[tc_rows(A5_sid_l_Volume, A5_sid_hL) * M_sid_ND];
  cm_cnt A5_sid_MT[tc_rows(A5_sid_t_Volume, A5_sid_hT) * M_sid_MND];
  i32 A5_sid_W[tc_rows(A5_sid_m_Volume, A5_sid_hM) * M_sid_ND * tc_nin(10)];
  u8 A5_sid_WC[tc_rows(A5_sid_m_Volume, A5_sid_hM) * M_sid_ND];
  i32 A5_sid_WM[tc_rows(A5_sid_n_Volume, A5_sid_hN) * M_sid_MND * TC_MIN];
  u8 A5_sid_WMC[tc_rows(A5_sid_n_Volume, A5_sid_hN) * M_sid_MND];
  cm_cnt A5_sid_G[A5_sid_g_Volume * M_sid_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long A5_sid_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A5_sid_a_Volume, A5_sid_hA) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A5_sid_b_Volume, A5_sid_hB) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A5_sid_c_Volume, A5_sid_hC) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A5_sid_d_Volume, A5_sid_hD) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A5_sid_e_Volume, A5_sid_hE) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A5_sid_h_Volume, A5_sid_hH) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A5_sid_i_Volume, A5_sid_hI) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A5_sid_j_Volume, A5_sid_hJ) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A5_sid_k_Volume, A5_sid_hK) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A5_sid_l_Volume, A5_sid_hL) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(A5_sid_t_Volume, A5_sid_hT) * M_sid_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(A5_sid_m_Volume, A5_sid_hM) * M_sid_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(A5_sid_m_Volume, A5_sid_hM) * M_sid_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(A5_sid_n_Volume, A5_sid_hN) * M_sid_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(A5_sid_n_Volume, A5_sid_hN) * M_sid_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(A5_sid_g_Volume * M_sid_NG);
  void A5_sid_Init( void ) {}
  void A5_sid_Quit( void ) {}
};
