
static const byte H7_cbp_a_pc[4+1]={ 0,1,2,2,3 };
static const byte H7_cbp_a_st[9+1]={ 0,0,1,2,2,3,3,3,3,3 };

static const byte H7_cbp_b_col[12+1]={ 0,0,0,1,2,2,2,2,3,4,4,5,5 };
static const byte H7_cbp_b_pq[6+1]={ 0,0,1,2,3,3,3 };

static const byte H7_cbp_c_cls[8+1]={ 0,0,0,1,2,2,3,3,3 };
static const byte H7_cbp_c_col[12+1]={ 0,0,1,2,3,3,3,4,4,5,6,6,6 };
static const byte H7_cbp_c_pg[16+1]={ 0,0,1,1,1,1,2,3,3,3,4,4,4,5,5,6,7 };

static const byte H7_cbp_d_em[16+1]={ 0,1,2,3,3,4,5,6,6,6,6,7,7,7,7,7,8 };
static const byte H7_cbp_d_cls[8+1]={ 0,0,0,1,2,3,4,4,4 };

static const byte H7_cbp_e_pg[16+1]={ 0,1,1,1,1,1,1,2,2,3,3,3,4,4,4,4,4 };
static const byte H7_cbp_e_tp[16+1]={ 0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,7,7 };


static const byte H7_cbp_h_td[16+1]={ 0,1,1,1,1,2,2,2,2,2,3,3,3,3,3,3,3 };
static const byte H7_cbp_h_en[14+1]={ 0,0,1,1,2,2,2,3,3,3,3,3,4,4,5 };
static const byte H7_cbp_h_q6[8+1]={ 0,0,1,2,2,2,2,3,3 };

static const byte H7_cbp_i_inc[3+1]={ 0,1,2,3 };
static const byte H7_cbp_i_pq[6+1]={ 0,1,2,2,3,3,4 };
static const byte H7_cbp_i_q6[8+1]={ 0,1,1,2,3,4,5,5,5 };

static const byte H7_cbp_j_tp[16+1]={ 0,1,2,2,3,3,3,3,4,4,4,4,5,5,5,5,5 };
static const byte H7_cbp_j_em[16+1]={ 0,1,2,2,3,3,4,4,5,5,6,6,6,7,7,7,7 };
static const byte H7_cbp_j_st[9+1]={ 0,0,1,2,2,3,3,3,3,3 };

static const byte H7_cbp_k_pg[16+1]={ 0,0,0,0,1,1,1,1,2,2,2,3,3,3,4,4,4 };

static const byte H7_cbp_l_pc[4+1]={ 0,1,2,3,4 };
static const byte H7_cbp_l_en[14+1]={ 0,1,1,1,2,2,2,2,2,2,2,3,3,4,4 };
static const byte H7_cbp_l_q6[8+1]={ 0,0,0,0,1,2,3,3,3 };



static const byte H7_cbp_s_tp[16+1]={ 0,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5 };
static const byte H7_cbp_s_pq[6+1]={ 0,0,1,2,2,2,3 };


static const byte H7_cbp_w_em[16+1]={ 0,1,2,2,2,3,3,4,4,5,5,5,6,6,6,6,6 };
static const byte H7_cbp_w_cls[8+1]={ 0,0,0,1,1,2,3,3,3 };


static const int H7_cbp_rA = (254+0) * (1);
static const int H7_cbp_rB = (130+0) * (1);
static const int H7_cbp_rC = (254+0) * (1);
static const int H7_cbp_rE = (255+0) * (1);
static const int H7_cbp_rH = (248+0) * (1);
static const int H7_cbp_rI = (255+0) * (1);
static const int H7_cbp_rJ = (254+0) * (1);
static const int H7_cbp_rK = (255+0) * (1);
static const int H7_cbp_rL = (254+0) * (1);
static const int H7_cbp_rD = (255+0) * (1);
static const int H7_cbp_rT = (0+0) * (1);
static const int H7_cbp_rG = (0+0) * (1);
static const int H7_cbp_lr = (15+0) * (1);
static const int H7_cbp_lrm = (0+0) * (1);
static const int H7_cbp_mbm = (0+0) * (1);
static const int H7_cbp_mwA = (2+0) * (1);
static const int H7_cbp_mwB = (5+0) * (1);
static const int H7_cbp_mwC = (1+0) * (1);
static const int H7_cbp_mwE = (64+0) * (1);
static const int H7_cbp_mwH = (32+0) * (1);
static const int H7_cbp_mwI = (64+0) * (1);
static const int H7_cbp_mwJ = (8+0) * (1);
static const int H7_cbp_mwK = (8+0) * (1);
static const int H7_cbp_mwL = (38+0) * (1);
static const int H7_cbp_mwD = (60+0) * (1);
static const int H7_cbp_mwT = (0+0) * (1);
static const int H7_cbp_mwG = (0+0) * (1);
static const int H7_cbp_mt = (0+0) * (1);
static const int H7_cbp_mb = (17+0) * (1);
static const int H7_cbp_hA = (24+0) * (1);
static const int H7_cbp_hB = (24+0) * (1);
static const int H7_cbp_hC = (24+0) * (1);
static const int H7_cbp_hE = (24+0) * (1);
static const int H7_cbp_hH = (24+0) * (1);
static const int H7_cbp_hI = (24+0) * (1);
static const int H7_cbp_hJ = (24+0) * (1);
static const int H7_cbp_hK = (24+0) * (1);
static const int H7_cbp_hL = (24+0) * (1);
static const int H7_cbp_hD = (24+0) * (1);
static const int H7_cbp_hS = (24+0) * (1);
static const int H7_cbp_hM = (24+0) * (1);
static const int H7_cbp_hT = (24+0) * (1);
static const int H7_cbp_hN = (24+0) * (1);


static const long long H7_cbp_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 4), 3), 4); // 48
static const long long H7_cbp_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 6), 2), 4); // 48
static const long long H7_cbp_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 7), 3), 8); // 672
static const long long H7_cbp_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 9), 5); // 45
static const long long H7_cbp_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 5), 2), 8); // 80
static const long long H7_cbp_g_Volume = tc_vmul(1, 1); // 1
static const long long H7_cbp_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 4), 6), 4); // 96
static const long long H7_cbp_i_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 5), 1), 6); // 120
static const long long H7_cbp_j_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 6), 8), 4); // 576
static const long long H7_cbp_k_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 5), 3), 3); // 45
static const long long H7_cbp_l_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 5), 5), 4); // 300
static const long long H7_cbp_m_Volume = tc_vmul(1, 1); // 1
static const long long H7_cbp_n_Volume = tc_vmul(1, 1); // 1
static const long long H7_cbp_s_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 6), 4); // 48
static const long long H7_cbp_t_Volume = tc_vmul(1, 1); // 1
static const long long H7_cbp_w_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 7), 4); // 56

#define H7_cbp_USES(X) X(pc) X(cls) X(st) X(col) X(q6) X(pq) X(pg) X(em) X(tp) X(td) X(en) X(inc)

struct H7_cbp_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H7_cbp_A[tc_rows(H7_cbp_a_Volume, H7_cbp_hA) * H_cbp_ND];
  cm_cnt2 H7_cbp_B[tc_rows(H7_cbp_b_Volume, H7_cbp_hB) * H_cbp_ND];
  cm_cnt2 H7_cbp_C[tc_rows(H7_cbp_c_Volume, H7_cbp_hC) * H_cbp_ND];
  cm_cnt2 H7_cbp_D[tc_rows(H7_cbp_d_Volume, H7_cbp_hD) * H_cbp_ND];
  cm_cnt2 H7_cbp_E[tc_rows(H7_cbp_e_Volume, H7_cbp_hE) * H_cbp_ND];
  cm_cnt2 H7_cbp_H[tc_rows(H7_cbp_h_Volume, H7_cbp_hH) * H_cbp_ND];
  cm_cnt2 H7_cbp_I[tc_rows(H7_cbp_i_Volume, H7_cbp_hI) * H_cbp_ND];
  cm_cnt2 H7_cbp_J[tc_rows(H7_cbp_j_Volume, H7_cbp_hJ) * H_cbp_ND];
  cm_cnt2 H7_cbp_K[tc_rows(H7_cbp_k_Volume, H7_cbp_hK) * H_cbp_ND];
  cm_cnt2 H7_cbp_L[tc_rows(H7_cbp_l_Volume, H7_cbp_hL) * H_cbp_ND];
  cm_cnt H7_cbp_MT[tc_rows(H7_cbp_t_Volume, H7_cbp_hT) * H_cbp_MND];
  i32 H7_cbp_W[tc_rows(H7_cbp_m_Volume, H7_cbp_hM) * H_cbp_ND * 24];
  u8 H7_cbp_WC[tc_rows(H7_cbp_m_Volume, H7_cbp_hM) * H_cbp_ND];
  i32 H7_cbp_WM[tc_rows(H7_cbp_n_Volume, H7_cbp_hN) * H_cbp_MND * 3];
  u8 H7_cbp_WMC[tc_rows(H7_cbp_n_Volume, H7_cbp_hN) * H_cbp_MND];
  i32 H7_cbp_W2[H7_cbp_w_Volume * H_cbp_ND * 24];
  u8 H7_cbp_W2C[H7_cbp_w_Volume * H_cbp_ND];
  i32 H7_cbp_W3[tc_rows(H7_cbp_s_Volume, H7_cbp_hS) * H_cbp_ND * 24];
  u8 H7_cbp_W3C[tc_rows(H7_cbp_s_Volume, H7_cbp_hS) * H_cbp_ND];
  i32 H7_cbp_WF[H_cbp_ND * 5 * 4];
  u8 H7_cbp_WFC[H_cbp_ND * 5];
  cm_cnt H7_cbp_G[H7_cbp_g_Volume * H_cbp_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H7_cbp_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_cbp_a_Volume, H7_cbp_hA) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_cbp_b_Volume, H7_cbp_hB) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_cbp_c_Volume, H7_cbp_hC) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_cbp_d_Volume, H7_cbp_hD) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_cbp_e_Volume, H7_cbp_hE) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_cbp_h_Volume, H7_cbp_hH) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_cbp_i_Volume, H7_cbp_hI) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_cbp_j_Volume, H7_cbp_hJ) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_cbp_k_Volume, H7_cbp_hK) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H7_cbp_l_Volume, H7_cbp_hL) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H7_cbp_t_Volume, H7_cbp_hT) * H_cbp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H7_cbp_m_Volume, H7_cbp_hM) * H_cbp_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H7_cbp_m_Volume, H7_cbp_hM) * H_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H7_cbp_n_Volume, H7_cbp_hN) * H_cbp_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H7_cbp_n_Volume, H7_cbp_hN) * H_cbp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H7_cbp_w_Volume * H_cbp_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H7_cbp_w_Volume * H_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H7_cbp_s_Volume, H7_cbp_hS) * H_cbp_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H7_cbp_s_Volume, H7_cbp_hS) * H_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_cbp_ND * 5 * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_cbp_ND * 5)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H7_cbp_g_Volume * H_cbp_NG);
  void H7_cbp_Init( void ) {
  
  }
  void H7_cbp_Quit( void ) {}
};
