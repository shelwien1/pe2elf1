


static const byte A9_sid_c_pos[15+1]={ 0,1,2,3,4,5,6,6,7,8,9,10,11,11,12,13 };







static const byte A9_sid_j_sk[4+1]={ 0,1,2,3,4 };

static const byte A9_sid_k_pos[31+1]={ 0,0,1,2,3,4,5,5,6,7,8,9,10,11,12,13,14,15,15,15,16,17,17,17,17,17,17,17,17,17,17,17 };

static const byte A9_sid_l_blk[5+1]={ 0,1,1,2,2,3 };







static const int A9_sid_rA = (253+0) * (1);
static const int A9_sid_mwA = (32+0) * (1);
static const int A9_sid_rB = (255+0) * (1);
static const int A9_sid_mwB = (4+0) * (1);
static const int A9_sid_rC = (177+0) * (1);
static const int A9_sid_mwC = (0+0) * (1);
static const int A9_sid_rD = (255+0) * (1);
static const int A9_sid_mwD = (0+0) * (1);
static const int A9_sid_rE = (255+0) * (1);
static const int A9_sid_mwE = (0+0) * (1);
static const int A9_sid_rH = (254+0) * (1);
static const int A9_sid_mwH = (9+0) * (1);
static const int A9_sid_rI = (252+0) * (1);
static const int A9_sid_mwI = (2+0) * (1);
static const int A9_sid_rJ = (250+0) * (1);
static const int A9_sid_mwJ = (29+0) * (1);
static const int A9_sid_rK = (255+0) * (1);
static const int A9_sid_mwK = (4+0) * (1);
static const int A9_sid_rL = (254+0) * (1);
static const int A9_sid_mwL = (1+0) * (1);
static const int A9_sid_lr = (15+0) * (1);
static const int A9_sid_mb = (96+0) * (1);
static const int A9_sid_rS1 = (0+0) * (1);
static const int A9_sid_rS2 = (8+0) * (1);
static const int A9_sid_qs = (0+3) * (1);
static const int A9_sid_qf = (31+3) * (1);
static const int A9_sid_bw = (12+0) * (1);
static const int A9_sid_rT = (128+0) * (1);
static const int A9_sid_mwT = (0+0) * (1);
static const int A9_sid_mt = (4+0) * (1);
static const int A9_sid_lrm = (31+0) * (1);
static const int A9_sid_mbm = (25+0) * (1);
static const int A9_sid_rG = (255+0) * (1);
static const int A9_sid_mwG = (188+0) * (1);
static const int A9_sid_hA = (22+0) * (1);
static const int A9_sid_hB = (22+0) * (1);
static const int A9_sid_hC = (22+0) * (1);
static const int A9_sid_hD = (22+0) * (1);
static const int A9_sid_hE = (22+0) * (1);
static const int A9_sid_hH = (22+0) * (1);
static const int A9_sid_hI = (22+0) * (1);
static const int A9_sid_hJ = (22+0) * (1);
static const int A9_sid_hK = (22+0) * (1);
static const int A9_sid_hL = (22+0) * (1);
static const int A9_sid_hS = (22+0) * (1);
static const int A9_sid_hF = (22+0) * (1);
static const int A9_sid_hM = (22+0) * (1);
static const int A9_sid_hT = (22+0) * (1);
static const int A9_sid_hN = (22+0) * (1);


static const long long A9_sid_a_Volume = 1; // 1
static const long long A9_sid_b_Volume = tc_vmul(tc_vmul(1, (1<<5)), (1<<6)); // 2048
static const long long A9_sid_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), (1<<5)), 14); // 28672
static const long long A9_sid_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), (1<<6)), 2); // 8192
static const long long A9_sid_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), 3), 3); // 576
static const long long A9_sid_f_Volume = tc_vmul(1, (1<<5)); // 32
static const long long A9_sid_g_Volume = tc_vmul(1, (1<<6)); // 64
static const long long A9_sid_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<5)), (1<<5)), 3), 3); // 9216
static const long long A9_sid_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), 2), (1<<4)); // 2048
static const long long A9_sid_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<5)), 5), (1<<1)); // 320
static const long long A9_sid_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), 18), (1<<5)); // 36864
static const long long A9_sid_l_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), 2), 3), 4); // 1536
static const long long A9_sid_m_Volume = tc_vmul(1, (1<<5)); // 32
static const long long A9_sid_n_Volume = tc_vmul(1, (1<<6)); // 64
static const long long A9_sid_s_Volume = tc_vmul(tc_vmul(1, (1<<6)), 3); // 192
static const long long A9_sid_t_Volume = tc_vmul(1, (1<<5)); // 32
static const long long A9_sid_w_Volume = tc_vmul(tc_vmul(1, 3), 2); // 6

#define A9_sid_USES(X) X(fld) X(lv) X(pos) X(pq) X(lb) X(st) X(nch) X(pr) X(blk) X(sk)

struct A9_sid_T {
  cm_cnt2 A9_sid_A[tc_rows(A9_sid_a_Volume, A9_sid_hA) * M_sid_ND];
  cm_cnt2 A9_sid_B[tc_rows(A9_sid_b_Volume, A9_sid_hB) * M_sid_ND];
  cm_cnt2 A9_sid_C[tc_rows(A9_sid_c_Volume, A9_sid_hC) * M_sid_ND];
  cm_cnt2 A9_sid_D[tc_rows(A9_sid_d_Volume, A9_sid_hD) * M_sid_ND];
  cm_cnt2 A9_sid_E[tc_rows(A9_sid_e_Volume, A9_sid_hE) * M_sid_ND];
  cm_cnt2 A9_sid_H[tc_rows(A9_sid_h_Volume, A9_sid_hH) * M_sid_ND];
  cm_cnt2 A9_sid_I[tc_rows(A9_sid_i_Volume, A9_sid_hI) * M_sid_ND];
  cm_cnt2 A9_sid_J[tc_rows(A9_sid_j_Volume, A9_sid_hJ) * M_sid_ND];
  cm_cnt2 A9_sid_K[tc_rows(A9_sid_k_Volume, A9_sid_hK) * M_sid_ND];
  cm_cnt2 A9_sid_L[tc_rows(A9_sid_l_Volume, A9_sid_hL) * M_sid_ND];
  cm_cnt A9_sid_MT[tc_rows(A9_sid_t_Volume, A9_sid_hT) * M_sid_MND];
  i16 A9_sid_S[tc_rows(A9_sid_s_Volume, A9_sid_hS) * M_sid_ND * cm_apm_q(A9_sid_qs)];
  u8 A9_sid_SC[tc_rows(A9_sid_s_Volume, A9_sid_hS) * M_sid_ND * cm_apm_q(A9_sid_qs)];
  i16 A9_sid_F[tc_rows(A9_sid_f_Volume, A9_sid_hF) * M_sid_ND * cm_apm_q(A9_sid_qf)];
  u8 A9_sid_FC[tc_rows(A9_sid_f_Volume, A9_sid_hF) * M_sid_ND * cm_apm_q(A9_sid_qf)];
  i32 A9_sid_W[tc_rows(A9_sid_m_Volume, A9_sid_hM) * M_sid_ND * tc_nin(10)];
  u8 A9_sid_WC[tc_rows(A9_sid_m_Volume, A9_sid_hM) * M_sid_ND];
  i32 A9_sid_WM[tc_rows(A9_sid_n_Volume, A9_sid_hN) * M_sid_MND * TC_MIN];
  u8 A9_sid_WMC[tc_rows(A9_sid_n_Volume, A9_sid_hN) * M_sid_MND];
  i32 A9_sid_W2[A9_sid_w_Volume * M_sid_ND * tc_nin(10)];
  u8 A9_sid_W2C[A9_sid_w_Volume * M_sid_ND];
  i32 A9_sid_W3[tc_rows(A9_sid_s_Volume, A9_sid_hS) * M_sid_ND * tc_nin(10)];
  u8 A9_sid_W3C[tc_rows(A9_sid_s_Volume, A9_sid_hS) * M_sid_ND];
  i32 A9_sid_WF[M_sid_ND * HS_N * TC_FIN];
  u8 A9_sid_WFC[M_sid_ND * HS_N];
  cm_cnt A9_sid_G[A9_sid_g_Volume * M_sid_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long A9_sid_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A9_sid_a_Volume, A9_sid_hA) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A9_sid_b_Volume, A9_sid_hB) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A9_sid_c_Volume, A9_sid_hC) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A9_sid_d_Volume, A9_sid_hD) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A9_sid_e_Volume, A9_sid_hE) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A9_sid_h_Volume, A9_sid_hH) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A9_sid_i_Volume, A9_sid_hI) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A9_sid_j_Volume, A9_sid_hJ) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A9_sid_k_Volume, A9_sid_hK) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(A9_sid_l_Volume, A9_sid_hL) * M_sid_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(A9_sid_t_Volume, A9_sid_hT) * M_sid_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(A9_sid_s_Volume, A9_sid_hS) * M_sid_ND * cm_apm_q(A9_sid_qs))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(A9_sid_s_Volume, A9_sid_hS) * M_sid_ND * cm_apm_q(A9_sid_qs))
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(A9_sid_f_Volume, A9_sid_hF) * M_sid_ND * cm_apm_q(A9_sid_qf))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(A9_sid_f_Volume, A9_sid_hF) * M_sid_ND * cm_apm_q(A9_sid_qf))
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(A9_sid_m_Volume, A9_sid_hM) * M_sid_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(A9_sid_m_Volume, A9_sid_hM) * M_sid_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(A9_sid_n_Volume, A9_sid_hN) * M_sid_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(A9_sid_n_Volume, A9_sid_hN) * M_sid_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(A9_sid_w_Volume * M_sid_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(A9_sid_w_Volume * M_sid_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(A9_sid_s_Volume, A9_sid_hS) * M_sid_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(A9_sid_s_Volume, A9_sid_hS) * M_sid_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(M_sid_ND * HS_N * TC_FIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(M_sid_ND * HS_N)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(A9_sid_g_Volume * M_sid_NG);
  void A9_sid_Init( void ) {}
  void A9_sid_Quit( void ) {}
};
