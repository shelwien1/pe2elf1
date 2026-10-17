
static const byte H_cbp_a_pc[4+1]={ 0,1,2,2,3 };
static const byte H_cbp_a_cls[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H_cbp_a_st[9+1]={ 0,1,2,3,4,5,6,7,8,9 };

static const byte H_cbp_b_col[12+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12 };
static const byte H_cbp_b_q6[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H_cbp_b_pq[6+1]={ 0,1,2,2,2,2,3 };

static const byte H_cbp_c_cls[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H_cbp_c_col[12+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12 };
static const byte H_cbp_c_pg[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };

static const byte H_cbp_d_inc[3+1]={ 0,1,2,3 };
static const byte H_cbp_d_em[16+1]={ 0,1,2,2,2,3,3,4,5,5,6,7,8,8,9,9,9 };
static const byte H_cbp_d_cls[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H_cbp_e_pg[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
static const byte H_cbp_e_cls[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H_cbp_e_tp[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };



static const byte H_cbp_h_td[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
static const byte H_cbp_h_en[14+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14 };
static const byte H_cbp_h_q6[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H_cbp_i_inc[3+1]={ 0,1,2,3 };
static const byte H_cbp_i_pq[6+1]={ 0,1,2,3,4,5,6 };
static const byte H_cbp_i_cls[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H_cbp_i_q6[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H_cbp_j_inc[3+1]={ 0,1,2,3 };
static const byte H_cbp_j_tp[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
static const byte H_cbp_j_em[16+1]={ 0,0,1,1,1,1,2,2,3,3,4,4,4,5,5,5,5 };
static const byte H_cbp_j_st[9+1]={ 0,1,2,3,4,5,6,7,8,9 };

static const byte H_cbp_k_inc[3+1]={ 0,1,2,3 };
static const byte H_cbp_k_col[12+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12 };
static const byte H_cbp_k_cls[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H_cbp_l_inc[3+1]={ 0,1,2,3 };
static const byte H_cbp_l_pc[4+1]={ 0,1,2,3,4 };
static const byte H_cbp_l_en[14+1]={ 0,1,1,1,2,2,2,2,2,2,2,3,3,3,4 };
static const byte H_cbp_l_q6[8+1]={ 0,1,2,3,4,5,6,7,8 };



static const byte H_cbp_s_inc[3+1]={ 0,1,2,3 };
static const byte H_cbp_s_tp[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
static const byte H_cbp_s_pq[6+1]={ 0,1,2,3,4,5,6 };


static const byte H_cbp_w_inc[3+1]={ 0,1,2,3 };
static const byte H_cbp_w_em[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
static const byte H_cbp_w_cls[8+1]={ 0,1,2,3,4,5,6,7,8 };


static const int H_cbp_rA = (64+0) * (1);
static const int H_cbp_rB = (245+0) * (1);
static const int H_cbp_rC = (64+0) * (1);
static const int H_cbp_rE = (64+0) * (1);
static const int H_cbp_rH = (64+0) * (1);
static const int H_cbp_rI = (100+0) * (1);
static const int H_cbp_rJ = (64+0) * (1);
static const int H_cbp_rK = (224+0) * (1);
static const int H_cbp_rL = (64+0) * (1);
static const int H_cbp_rD = (255+0) * (1);
static const int H_cbp_rT = (55+0) * (1);
static const int H_cbp_rG = (90+0) * (1);
static const int H_cbp_rS1 = (8+0) * (1);
static const int H_cbp_rS2 = (0+0) * (1);
static const int H_cbp_lr = (15+0) * (1);
static const int H_cbp_lrm = (12+0) * (1);
static const int H_cbp_mbm = (0+0) * (1);
static const int H_cbp_mwA = (0+0) * (1);
static const int H_cbp_mwB = (0+0) * (1);
static const int H_cbp_mwC = (0+0) * (1);
static const int H_cbp_mwE = (0+0) * (1);
static const int H_cbp_mwH = (0+0) * (1);
static const int H_cbp_mwI = (0+0) * (1);
static const int H_cbp_mwJ = (0+0) * (1);
static const int H_cbp_mwK = (0+0) * (1);
static const int H_cbp_mwL = (0+0) * (1);
static const int H_cbp_mwD = (18+0) * (1);
static const int H_cbp_mwT = (0+0) * (1);
static const int H_cbp_mwG = (0+0) * (1);
static const int H_cbp_bw = (15+0) * (1);
static const int H_cbp_qs = (22+3) * (1);
static const int H_cbp_qf = (4+3) * (1);
static const int H_cbp_mb = (5+0) * (1);
static const int H_cbp_hA = (24+0) * (1);
static const int H_cbp_hB = (24+0) * (1);
static const int H_cbp_hE = (24+0) * (1);
static const int H_cbp_hH = (24+0) * (1);
static const int H_cbp_hI = (24+0) * (1);
static const int H_cbp_hJ = (24+0) * (1);
static const int H_cbp_hK = (24+0) * (1);
static const int H_cbp_hL = (24+0) * (1);
static const int H_cbp_hD = (24+0) * (1);
static const int H_cbp_hS = (24+0) * (1);
static const int H_cbp_hF = (24+0) * (1);
static const int H_cbp_hM = (24+0) * (1);
static const int H_cbp_hT = (24+0) * (1);
static const int H_cbp_hN = (24+0) * (1);


static const long long H_cbp_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 4), 9), 10); // 1080
static const long long H_cbp_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 13), 9), 4); // 468
static const long long H_cbp_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 9), 13), 3), 17); // 5967
static const long long H_cbp_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 10), 9); // 360
static const long long H_cbp_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 17), 9), 17); // 7803
static const long long H_cbp_f_Volume = tc_vmul(1, 3); // 3
static const long long H_cbp_g_Volume = tc_vmul(1, 1); // 1
static const long long H_cbp_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 17), 15), 9); // 6885
static const long long H_cbp_i_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 7), 9), 9); // 2268
static const long long H_cbp_j_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 17), 6), 10); // 4080
static const long long H_cbp_k_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 3), 13), 9); // 1404
static const long long H_cbp_l_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 5), 5), 9); // 900
static const long long H_cbp_m_Volume = tc_vmul(1, 1); // 1
static const long long H_cbp_n_Volume = tc_vmul(1, 1); // 1
static const long long H_cbp_s_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 17), 7); // 476
static const long long H_cbp_t_Volume = tc_vmul(1, 1); // 1
static const long long H_cbp_w_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 17), 9); // 612

struct H_cbp_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H_cbp_A[tc_rows(H_cbp_a_Volume, H_cbp_hA) * H_cbp_ND];
  cm_cnt2 H_cbp_B[tc_rows(H_cbp_b_Volume, H_cbp_hB) * H_cbp_ND];
  cm_cnt2 H_cbp_C[H_cbp_c_Volume * H_cbp_ND];
  cm_cnt2 H_cbp_D[tc_rows(H_cbp_d_Volume, H_cbp_hD) * H_cbp_ND];
  cm_cnt2 H_cbp_E[tc_rows(H_cbp_e_Volume, H_cbp_hE) * H_cbp_ND];
  cm_cnt2 H_cbp_H[tc_rows(H_cbp_h_Volume, H_cbp_hH) * H_cbp_ND];
  cm_cnt2 H_cbp_I[tc_rows(H_cbp_i_Volume, H_cbp_hI) * H_cbp_ND];
  cm_cnt2 H_cbp_J[tc_rows(H_cbp_j_Volume, H_cbp_hJ) * H_cbp_ND];
  cm_cnt2 H_cbp_K[tc_rows(H_cbp_k_Volume, H_cbp_hK) * H_cbp_ND];
  cm_cnt2 H_cbp_L[tc_rows(H_cbp_l_Volume, H_cbp_hL) * H_cbp_ND];
  cm_cnt H_cbp_MT[tc_rows(H_cbp_t_Volume, H_cbp_hT) * H_cbp_MND];
  i16 H_cbp_S[tc_rows(H_cbp_s_Volume, H_cbp_hS) * H_cbp_ND * H_cbp_qs];
  u8 H_cbp_SC[tc_rows(H_cbp_s_Volume, H_cbp_hS) * H_cbp_ND * H_cbp_qs];
  i16 H_cbp_F[tc_rows(H_cbp_f_Volume, H_cbp_hF) * H_cbp_ND * H_cbp_qf];
  u8 H_cbp_FC[tc_rows(H_cbp_f_Volume, H_cbp_hF) * H_cbp_ND * H_cbp_qf];
  i32 H_cbp_W[tc_rows(H_cbp_m_Volume, H_cbp_hM) * H_cbp_ND * TC_MIXN];
  u8 H_cbp_WC[tc_rows(H_cbp_m_Volume, H_cbp_hM) * H_cbp_ND];
  i32 H_cbp_WM[tc_rows(H_cbp_n_Volume, H_cbp_hN) * H_cbp_MND * 3];
  u8 H_cbp_WMC[tc_rows(H_cbp_n_Volume, H_cbp_hN) * H_cbp_MND];
  i32 H_cbp_W2[H_cbp_w_Volume * H_cbp_ND * TC_MIXN];
  u8 H_cbp_W2C[H_cbp_w_Volume * H_cbp_ND];
  i32 H_cbp_W3[tc_rows(H_cbp_s_Volume, H_cbp_hS) * H_cbp_ND * TC_MIXN];
  u8 H_cbp_W3C[tc_rows(H_cbp_s_Volume, H_cbp_hS) * H_cbp_ND];
  i32 H_cbp_WF[H_cbp_ND * 5 * 4];
  u8 H_cbp_WFC[H_cbp_ND * 5];
  cm_cnt H_cbp_G[H_cbp_g_Volume * H_cbp_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H_cbp_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_cbp_a_Volume, H_cbp_hA) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_cbp_b_Volume, H_cbp_hB) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(H_cbp_c_Volume * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_cbp_d_Volume, H_cbp_hD) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_cbp_e_Volume, H_cbp_hE) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_cbp_h_Volume, H_cbp_hH) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_cbp_i_Volume, H_cbp_hI) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_cbp_j_Volume, H_cbp_hJ) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_cbp_k_Volume, H_cbp_hK) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H_cbp_l_Volume, H_cbp_hL) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_cbp_t_Volume, H_cbp_hT) * H_cbp_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H_cbp_s_Volume, H_cbp_hS) * H_cbp_ND * H_cbp_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_cbp_s_Volume, H_cbp_hS) * H_cbp_ND * H_cbp_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H_cbp_f_Volume, H_cbp_hF) * H_cbp_ND * H_cbp_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_cbp_f_Volume, H_cbp_hF) * H_cbp_ND * H_cbp_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H_cbp_m_Volume, H_cbp_hM) * H_cbp_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_cbp_m_Volume, H_cbp_hM) * H_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H_cbp_n_Volume, H_cbp_hN) * H_cbp_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_cbp_n_Volume, H_cbp_hN) * H_cbp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_cbp_w_Volume * H_cbp_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_cbp_w_Volume * H_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H_cbp_s_Volume, H_cbp_hS) * H_cbp_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_cbp_s_Volume, H_cbp_hS) * H_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_cbp_ND * 5 * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_cbp_ND * 5)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H_cbp_g_Volume * H_cbp_NG);
  void H_cbp_Init( void ) {
  
  }
};
