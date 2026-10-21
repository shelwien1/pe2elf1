
static const byte H9_cbp_a_pc[4+1]={ 0,1,2,2,3 };
static const byte H9_cbp_a_cls[8+1]={ 0,0,0,1,2,3,4,4,4 };
static const byte H9_cbp_a_st[9+1]={ 0,0,1,2,3,3,3,3,3,3 };

static const byte H9_cbp_b_col[12+1]={ 0,1,2,2,3,3,3,3,4,5,5,6,6 };
static const byte H9_cbp_b_q6[8+1]={ 0,1,1,2,2,3,4,5,5 };

static const byte H9_cbp_c_cls[8+1]={ 0,0,0,1,2,2,3,3,3 };
static const byte H9_cbp_c_col[12+1]={ 0,0,1,2,3,3,3,4,4,4,4,4,4 };
static const byte H9_cbp_c_pg[16+1]={ 0,0,0,0,1,1,2,2,2,3,3,3,4,5,5,6,7 };

static const byte H9_cbp_d_em[16+1]={ 0,0,1,1,1,2,2,3,3,4,5,6,7,7,7,8,9 };

static const byte H9_cbp_e_pg[16+1]={ 0,1,1,1,1,2,2,2,2,2,3,3,3,4,4,4,5 };
static const byte H9_cbp_e_tp[16+1]={ 0,0,1,2,2,2,2,2,2,3,3,3,3,4,4,4,4 };



static const byte H9_cbp_h_td[16+1]={ 0,1,1,1,2,2,2,2,2,2,3,3,3,3,3,3,4 };
static const byte H9_cbp_h_en[14+1]={ 0,0,1,1,2,2,2,3,3,3,4,4,5,6,6 };
static const byte H9_cbp_h_q6[8+1]={ 0,1,2,3,4,4,4,5,5 };

static const byte H9_cbp_i_pq[6+1]={ 0,1,2,3,3,3,4 };

static const byte H9_cbp_j_tp[16+1]={ 0,1,2,2,2,2,3,3,4,4,4,5,6,6,6,6,6 };
static const byte H9_cbp_j_em[16+1]={ 0,1,2,3,3,3,4,4,5,5,6,6,6,7,7,7,7 };
static const byte H9_cbp_j_st[9+1]={ 0,0,1,2,2,3,3,3,3,3 };

static const byte H9_cbp_k_pg[16+1]={ 0,0,0,0,0,0,0,1,1,1,2,3,3,3,4,4,4 };
static const byte H9_cbp_k_col[12+1]={ 0,0,1,2,2,2,2,2,3,4,5,5,5 };

static const byte H9_cbp_l_inc[3+1]={ 0,1,2,3 };
static const byte H9_cbp_l_pc[4+1]={ 0,1,2,2,3 };
static const byte H9_cbp_l_en[14+1]={ 0,0,0,0,1,2,2,2,2,2,2,3,3,3,4 };
static const byte H9_cbp_l_q6[8+1]={ 0,0,0,0,1,2,3,3,3 };



static const byte H9_cbp_s_tp[16+1]={ 0,1,2,2,3,3,3,4,4,4,5,5,5,5,6,6,6 };
static const byte H9_cbp_s_pq[6+1]={ 0,0,1,2,2,3,3 };


static const byte H9_cbp_w_inc[3+1]={ 0,1,2,3 };
static const byte H9_cbp_w_em[16+1]={ 0,1,2,2,3,3,4,4,4,5,5,6,6,6,6,6,6 };


static const int H9_cbp_rA = (254+0) * (1);
static const int H9_cbp_rB = (253+0) * (1);
static const int H9_cbp_rC = (253+0) * (1);
static const int H9_cbp_rE = (255+0) * (1);
static const int H9_cbp_rH = (253+0) * (1);
static const int H9_cbp_rI = (241+0) * (1);
static const int H9_cbp_rJ = (254+0) * (1);
static const int H9_cbp_rK = (248+0) * (1);
static const int H9_cbp_rL = (222+0) * (1);
static const int H9_cbp_rD = (254+0) * (1);
static const int H9_cbp_rT = (0+0) * (1);
static const int H9_cbp_rG = (0+0) * (1);
static const int H9_cbp_rS1 = (8+0) * (1);
static const int H9_cbp_rS2 = (0+0) * (1);
static const int H9_cbp_lr = (15+0) * (1);
static const int H9_cbp_lrm = (0+0) * (1);
static const int H9_cbp_mbm = (0+0) * (1);
static const int H9_cbp_mwA = (2+0) * (1);
static const int H9_cbp_mwB = (8+0) * (1);
static const int H9_cbp_mwC = (2+0) * (1);
static const int H9_cbp_mwE = (19+0) * (1);
static const int H9_cbp_mwH = (16+0) * (1);
static const int H9_cbp_mwI = (64+0) * (1);
static const int H9_cbp_mwJ = (4+0) * (1);
static const int H9_cbp_mwK = (11+0) * (1);
static const int H9_cbp_mwL = (32+0) * (1);
static const int H9_cbp_mwD = (57+0) * (1);
static const int H9_cbp_mwT = (0+0) * (1);
static const int H9_cbp_mwG = (0+0) * (1);
static const int H9_cbp_bw = (15+0) * (1);
static const int H9_cbp_mt = (0+0) * (1);
static const int H9_cbp_qs = (52+3) * (1);
static const int H9_cbp_qf = (10+3) * (1);
static const int H9_cbp_mb = (17+0) * (1);
static const int H9_cbp_hA = (24+0) * (1);
static const int H9_cbp_hB = (24+0) * (1);
static const int H9_cbp_hC = (24+0) * (1);
static const int H9_cbp_hE = (24+0) * (1);
static const int H9_cbp_hH = (24+0) * (1);
static const int H9_cbp_hI = (24+0) * (1);
static const int H9_cbp_hJ = (24+0) * (1);
static const int H9_cbp_hK = (24+0) * (1);
static const int H9_cbp_hL = (24+0) * (1);
static const int H9_cbp_hD = (24+0) * (1);
static const int H9_cbp_hS = (24+0) * (1);
static const int H9_cbp_hF = (24+0) * (1);
static const int H9_cbp_hM = (24+0) * (1);
static const int H9_cbp_hT = (24+0) * (1);
static const int H9_cbp_hN = (24+0) * (1);


static const long long H9_cbp_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 4), 5), 4); // 240
static const long long H9_cbp_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 7), 6), 3); // 126
static const long long H9_cbp_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 5), 3), 8); // 480
static const long long H9_cbp_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 10), 2); // 20
static const long long H9_cbp_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 6), 1), 5); // 60
static const long long H9_cbp_f_Volume = tc_vmul(1, 3); // 3
static const long long H9_cbp_g_Volume = tc_vmul(1, 1); // 1
static const long long H9_cbp_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 5), 7), 6); // 210
static const long long H9_cbp_i_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 5), 2), 3); // 30
static const long long H9_cbp_j_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 7), 8), 4); // 224
static const long long H9_cbp_k_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 5), 6), 3); // 90
static const long long H9_cbp_l_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 4), 5), 4); // 320
static const long long H9_cbp_m_Volume = tc_vmul(1, 1); // 1
static const long long H9_cbp_n_Volume = tc_vmul(1, 1); // 1
static const long long H9_cbp_s_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 7), 4); // 56
static const long long H9_cbp_t_Volume = tc_vmul(1, 1); // 1
static const long long H9_cbp_w_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 7), 3); // 84

#define H9_cbp_USES(X) X(inc) X(pc) X(cls) X(st) X(col) X(q6) X(pq) X(pg) X(em) X(tp) X(td) X(en)

struct H9_cbp_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H9_cbp_A[tc_rows(H9_cbp_a_Volume, H9_cbp_hA) * H_cbp_ND];
  cm_cnt2 H9_cbp_B[tc_rows(H9_cbp_b_Volume, H9_cbp_hB) * H_cbp_ND];
  cm_cnt2 H9_cbp_C[tc_rows(H9_cbp_c_Volume, H9_cbp_hC) * H_cbp_ND];
  cm_cnt2 H9_cbp_D[tc_rows(H9_cbp_d_Volume, H9_cbp_hD) * H_cbp_ND];
  cm_cnt2 H9_cbp_E[tc_rows(H9_cbp_e_Volume, H9_cbp_hE) * H_cbp_ND];
  cm_cnt2 H9_cbp_H[tc_rows(H9_cbp_h_Volume, H9_cbp_hH) * H_cbp_ND];
  cm_cnt2 H9_cbp_I[tc_rows(H9_cbp_i_Volume, H9_cbp_hI) * H_cbp_ND];
  cm_cnt2 H9_cbp_J[tc_rows(H9_cbp_j_Volume, H9_cbp_hJ) * H_cbp_ND];
  cm_cnt2 H9_cbp_K[tc_rows(H9_cbp_k_Volume, H9_cbp_hK) * H_cbp_ND];
  cm_cnt2 H9_cbp_L[tc_rows(H9_cbp_l_Volume, H9_cbp_hL) * H_cbp_ND];
  cm_cnt H9_cbp_MT[tc_rows(H9_cbp_t_Volume, H9_cbp_hT) * H_cbp_MND];
  i16 H9_cbp_S[tc_rows(H9_cbp_s_Volume, H9_cbp_hS) * H_cbp_ND * cm_apm_q(H9_cbp_qs)];
  u8 H9_cbp_SC[tc_rows(H9_cbp_s_Volume, H9_cbp_hS) * H_cbp_ND * cm_apm_q(H9_cbp_qs)];
  i16 H9_cbp_F[tc_rows(H9_cbp_f_Volume, H9_cbp_hF) * H_cbp_ND * cm_apm_q(H9_cbp_qf)];
  u8 H9_cbp_FC[tc_rows(H9_cbp_f_Volume, H9_cbp_hF) * H_cbp_ND * cm_apm_q(H9_cbp_qf)];
  i32 H9_cbp_W[tc_rows(H9_cbp_m_Volume, H9_cbp_hM) * H_cbp_ND * TC_MIXN];
  u8 H9_cbp_WC[tc_rows(H9_cbp_m_Volume, H9_cbp_hM) * H_cbp_ND];
  i32 H9_cbp_WM[tc_rows(H9_cbp_n_Volume, H9_cbp_hN) * H_cbp_MND * 3];
  u8 H9_cbp_WMC[tc_rows(H9_cbp_n_Volume, H9_cbp_hN) * H_cbp_MND];
  i32 H9_cbp_W2[H9_cbp_w_Volume * H_cbp_ND * TC_MIXN];
  u8 H9_cbp_W2C[H9_cbp_w_Volume * H_cbp_ND];
  i32 H9_cbp_W3[tc_rows(H9_cbp_s_Volume, H9_cbp_hS) * H_cbp_ND * TC_MIXN];
  u8 H9_cbp_W3C[tc_rows(H9_cbp_s_Volume, H9_cbp_hS) * H_cbp_ND];
  i32 H9_cbp_WF[H_cbp_ND * 5 * 4];
  u8 H9_cbp_WFC[H_cbp_ND * 5];
  cm_cnt H9_cbp_G[H9_cbp_g_Volume * H_cbp_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H9_cbp_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_cbp_a_Volume, H9_cbp_hA) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_cbp_b_Volume, H9_cbp_hB) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_cbp_c_Volume, H9_cbp_hC) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_cbp_d_Volume, H9_cbp_hD) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_cbp_e_Volume, H9_cbp_hE) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_cbp_h_Volume, H9_cbp_hH) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_cbp_i_Volume, H9_cbp_hI) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_cbp_j_Volume, H9_cbp_hJ) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_cbp_k_Volume, H9_cbp_hK) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H9_cbp_l_Volume, H9_cbp_hL) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H9_cbp_t_Volume, H9_cbp_hT) * H_cbp_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H9_cbp_s_Volume, H9_cbp_hS) * H_cbp_ND * cm_apm_q(H9_cbp_qs))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H9_cbp_s_Volume, H9_cbp_hS) * H_cbp_ND * cm_apm_q(H9_cbp_qs))
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H9_cbp_f_Volume, H9_cbp_hF) * H_cbp_ND * cm_apm_q(H9_cbp_qf))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H9_cbp_f_Volume, H9_cbp_hF) * H_cbp_ND * cm_apm_q(H9_cbp_qf))
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H9_cbp_m_Volume, H9_cbp_hM) * H_cbp_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H9_cbp_m_Volume, H9_cbp_hM) * H_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H9_cbp_n_Volume, H9_cbp_hN) * H_cbp_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H9_cbp_n_Volume, H9_cbp_hN) * H_cbp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H9_cbp_w_Volume * H_cbp_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H9_cbp_w_Volume * H_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H9_cbp_s_Volume, H9_cbp_hS) * H_cbp_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H9_cbp_s_Volume, H9_cbp_hS) * H_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_cbp_ND * 5 * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_cbp_ND * 5)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H9_cbp_g_Volume * H_cbp_NG);
  void H9_cbp_Init( void ) {
  
  }
  void H9_cbp_Quit( void ) {}
};
