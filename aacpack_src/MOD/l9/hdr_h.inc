
static const byte H9_hdr_a_fld[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

static const byte H9_hdr_b_pb[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H9_hdr_c_p1[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };
static const byte H9_hdr_c_p2[6+1]={ 0,1,2,3,4,5,6 };

static const byte H9_hdr_d_fld[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H9_hdr_d_p1[6+1]={ 0,1,2,3,4,5,6 };

static const byte H9_hdr_e_ws[4+1]={ 0,1,2,3,4 };
static const byte H9_hdr_e_fld[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H9_hdr_f_fld[4+1]={ 0,1,2,3,4 };

static const byte H9_hdr_g_p1[6+1]={ 0,1,2,3,4,5,6 };

static const byte H9_hdr_h_fld[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };


static const byte H9_hdr_j_pos[3+1]={ 0,1,2,3 };
static const byte H9_hdr_j_p1[4+1]={ 0,1,2,3,4 };

static const byte H9_hdr_k_nb[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H9_hdr_l_fld[4+1]={ 0,1,2,3,4 };





static const byte H9_hdr_w_nb[8+1]={ 0,1,2,3,4,5,6,7,8 };


static const int H9_hdr_rA = (130+0) * (1);
static const int H9_hdr_rB = (242+0) * (1);
static const int H9_hdr_rC = (255+0) * (1);
static const int H9_hdr_rD = (255+0) * (1);
static const int H9_hdr_rE = (255+0) * (1);
static const int H9_hdr_rH = (255+0) * (1);
static const int H9_hdr_rI = (137+0) * (1);
static const int H9_hdr_rJ = (254+0) * (1);
static const int H9_hdr_rK = (255+0) * (1);
static const int H9_hdr_rL = (253+0) * (1);
static const int H9_hdr_rT = (128+0) * (1);
static const int H9_hdr_rG = (64+0) * (1);
static const int H9_hdr_rS1 = (8+0) * (1);
static const int H9_hdr_rS2 = (6+0) * (1);
static const int H9_hdr_lr = (15+0) * (1);
static const int H9_hdr_mwA = (128+0) * (1);
static const int H9_hdr_mwB = (4+0) * (1);
static const int H9_hdr_mwC = (4+0) * (1);
static const int H9_hdr_mwD = (27+0) * (1);
static const int H9_hdr_mwE = (16+0) * (1);
static const int H9_hdr_mwH = (8+0) * (1);
static const int H9_hdr_mwI = (1+0) * (1);
static const int H9_hdr_mwJ = (20+0) * (1);
static const int H9_hdr_mwK = (51+0) * (1);
static const int H9_hdr_mwL = (62+0) * (1);
static const int H9_hdr_mwT = (0+0) * (1);
static const int H9_hdr_mwG = (0+0) * (1);
static const int H9_hdr_bw = (12+0) * (1);
static const int H9_hdr_mt = (2+0) * (1);
static const int H9_hdr_qs = (23+3) * (1);
static const int H9_hdr_qf = (17+3) * (1);
static const int H9_hdr_mb = (33+0) * (1);
static const int H9_hdr_lrm = (0+0) * (1);
static const int H9_hdr_mbm = (0+0) * (1);
static const int H9_hdr_hA = (24+0) * (1);
static const int H9_hdr_hB = (24+0) * (1);
static const int H9_hdr_hC = (24+0) * (1);
static const int H9_hdr_hD = (24+0) * (1);
static const int H9_hdr_hE = (24+0) * (1);
static const int H9_hdr_hH = (24+0) * (1);
static const int H9_hdr_hI = (24+0) * (1);
static const int H9_hdr_hJ = (24+0) * (1);
static const int H9_hdr_hK = (24+0) * (1);
static const int H9_hdr_hL = (24+0) * (1);
static const int H9_hdr_hS = (24+0) * (1);
static const int H9_hdr_hF = (24+0) * (1);
static const int H9_hdr_hM = (24+0) * (1);
static const int H9_hdr_hT = (24+0) * (1);
static const int H9_hdr_hN = (24+0) * (1);


static const long long H9_hdr_a_Volume = tc_vmul(tc_vmul(1, (1<<8)), 16); // 4096
static const long long H9_hdr_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), (1<<8)), 9); // 589824
static const long long H9_hdr_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 11), 7); // 19712
static const long long H9_hdr_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 8), 7); // 14336
static const long long H9_hdr_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 5), 8); // 10240
static const long long H9_hdr_f_Volume = tc_vmul(tc_vmul(1, (1<<8)), 5); // 1280
static const long long H9_hdr_g_Volume = tc_vmul(1, 7); // 7
static const long long H9_hdr_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 3), 16); // 12288
static const long long H9_hdr_i_Volume = tc_vmul(1, (1<<8)); // 256
static const long long H9_hdr_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 4), 5); // 5120
static const long long H9_hdr_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 9), (1<<3)); // 18432
static const long long H9_hdr_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), (1<<4)), 5); // 20480
static const long long H9_hdr_m_Volume = tc_vmul(1, (1<<8)); // 256
static const long long H9_hdr_n_Volume = 1; // 1
static const long long H9_hdr_s_Volume = tc_vmul(1, (1<<8)); // 256
static const long long H9_hdr_t_Volume = tc_vmul(1, (1<<8)); // 256
static const long long H9_hdr_w_Volume = tc_vmul(1, 9); // 9

#define H9_hdr_USES(X) X(tag) X(fld) X(p1r) X(pb) X(p1) X(p2) X(ws) X(ch) X(pos) X(nb)

struct H9_hdr_T {
  cm_cnt2 H9_hdr_A[tc_rows(H9_hdr_a_Volume, H9_hdr_hA) * H_hdr_ND];
  cm_cnt2 H9_hdr_B[tc_rows(H9_hdr_b_Volume, H9_hdr_hB) * H_hdr_ND];
  cm_cnt2 H9_hdr_C[tc_rows(H9_hdr_c_Volume, H9_hdr_hC) * H_hdr_ND];
  cm_cnt2 H9_hdr_D[tc_rows(H9_hdr_d_Volume, H9_hdr_hD) * H_hdr_ND];
  cm_cnt2 H9_hdr_E[tc_rows(H9_hdr_e_Volume, H9_hdr_hE) * H_hdr_ND];
  cm_cnt2 H9_hdr_H[tc_rows(H9_hdr_h_Volume, H9_hdr_hH) * H_hdr_ND];
  cm_cnt2 H9_hdr_I[tc_rows(H9_hdr_i_Volume, H9_hdr_hI) * H_hdr_ND];
  cm_cnt2 H9_hdr_J[tc_rows(H9_hdr_j_Volume, H9_hdr_hJ) * H_hdr_ND];
  cm_cnt2 H9_hdr_K[tc_rows(H9_hdr_k_Volume, H9_hdr_hK) * H_hdr_ND];
  cm_cnt2 H9_hdr_L[tc_rows(H9_hdr_l_Volume, H9_hdr_hL) * H_hdr_ND];
  cm_cnt H9_hdr_MT[tc_rows(H9_hdr_t_Volume, H9_hdr_hT) * H_hdr_MND];
  i16 H9_hdr_S[tc_rows(H9_hdr_s_Volume, H9_hdr_hS) * H_hdr_ND * cm_apm_q(H9_hdr_qs)];
  u8 H9_hdr_SC[tc_rows(H9_hdr_s_Volume, H9_hdr_hS) * H_hdr_ND * cm_apm_q(H9_hdr_qs)];
  i16 H9_hdr_F[tc_rows(H9_hdr_f_Volume, H9_hdr_hF) * H_hdr_ND * cm_apm_q(H9_hdr_qf)];
  u8 H9_hdr_FC[tc_rows(H9_hdr_f_Volume, H9_hdr_hF) * H_hdr_ND * cm_apm_q(H9_hdr_qf)];
  i32 H9_hdr_W[tc_rows(H9_hdr_m_Volume, H9_hdr_hM) * H_hdr_ND * TC_MIXN];
  u8 H9_hdr_WC[tc_rows(H9_hdr_m_Volume, H9_hdr_hM) * H_hdr_ND];
  i32 H9_hdr_WM[tc_rows(H9_hdr_n_Volume, H9_hdr_hN) * H_hdr_MND * 3];
  u8 H9_hdr_WMC[tc_rows(H9_hdr_n_Volume, H9_hdr_hN) * H_hdr_MND];
  i32 H9_hdr_W2[H9_hdr_w_Volume * H_hdr_ND * TC_MIXN];
  u8 H9_hdr_W2C[H9_hdr_w_Volume * H_hdr_ND];
  i32 H9_hdr_W3[tc_rows(H9_hdr_s_Volume, H9_hdr_hS) * H_hdr_ND * TC_MIXN];
  u8 H9_hdr_W3C[tc_rows(H9_hdr_s_Volume, H9_hdr_hS) * H_hdr_ND];
  i32 H9_hdr_WF[H_hdr_ND * 5 * 4];
  u8 H9_hdr_WFC[H_hdr_ND * 5];
  cm_cnt H9_hdr_G[H9_hdr_g_Volume * H_hdr_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H9_hdr_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_hdr_a_Volume, H9_hdr_hA) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_hdr_b_Volume, H9_hdr_hB) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_hdr_c_Volume, H9_hdr_hC) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_hdr_d_Volume, H9_hdr_hD) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_hdr_e_Volume, H9_hdr_hE) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_hdr_h_Volume, H9_hdr_hH) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_hdr_i_Volume, H9_hdr_hI) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_hdr_j_Volume, H9_hdr_hJ) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_hdr_k_Volume, H9_hdr_hK) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_hdr_l_Volume, H9_hdr_hL) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H9_hdr_t_Volume, H9_hdr_hT) * H_hdr_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H9_hdr_s_Volume, H9_hdr_hS) * H_hdr_ND * cm_apm_q(H9_hdr_qs))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H9_hdr_s_Volume, H9_hdr_hS) * H_hdr_ND * cm_apm_q(H9_hdr_qs))
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H9_hdr_f_Volume, H9_hdr_hF) * H_hdr_ND * cm_apm_q(H9_hdr_qf))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H9_hdr_f_Volume, H9_hdr_hF) * H_hdr_ND * cm_apm_q(H9_hdr_qf))
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H9_hdr_m_Volume, H9_hdr_hM) * H_hdr_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H9_hdr_m_Volume, H9_hdr_hM) * H_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H9_hdr_n_Volume, H9_hdr_hN) * H_hdr_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H9_hdr_n_Volume, H9_hdr_hN) * H_hdr_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H9_hdr_w_Volume * H_hdr_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H9_hdr_w_Volume * H_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H9_hdr_s_Volume, H9_hdr_hS) * H_hdr_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H9_hdr_s_Volume, H9_hdr_hS) * H_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_hdr_ND * 5 * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_hdr_ND * 5)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H9_hdr_g_Volume * H_hdr_NG);
  void H9_hdr_Init( void ) {}
  void H9_hdr_Quit( void ) {}
};
