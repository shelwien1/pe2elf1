
static const byte H7_hdr_a_fld[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };


static const byte H7_hdr_c_p1[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };
static const byte H7_hdr_c_p2[6+1]={ 0,1,2,3,4,5,6 };

static const byte H7_hdr_d_fld[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H7_hdr_d_p1[6+1]={ 0,1,2,3,4,5,6 };
static const byte H7_hdr_d_dm[60+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60 };

static const byte H7_hdr_e_ws[4+1]={ 0,1,2,3,4 };
static const byte H7_hdr_e_fld[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H7_hdr_e_ml[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H7_hdr_g_p1[6+1]={ 0,1,2,3,4,5,6 };

static const byte H7_hdr_h_fld[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte H7_hdr_h_hx[20+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20 };

static const byte H7_hdr_i_pb[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H7_hdr_j_pos[3+1]={ 0,1,2,3 };
static const byte H7_hdr_j_p1[4+1]={ 0,1,2,3,4 };

static const byte H7_hdr_k_nb[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H7_hdr_l_fld[4+1]={ 0,1,2,3,4 };




static const byte H7_hdr_w_nb[8+1]={ 0,1,2,3,4,5,6,7,8 };


static const int H7_hdr_rA = (130+0) * (1);
static const int H7_hdr_rB = (242+0) * (1);
static const int H7_hdr_rC = (255+0) * (1);
static const int H7_hdr_rD = (255+0) * (1);
static const int H7_hdr_rE = (255+0) * (1);
static const int H7_hdr_rH = (255+0) * (1);
static const int H7_hdr_rI = (137+0) * (1);
static const int H7_hdr_rJ = (254+0) * (1);
static const int H7_hdr_rK = (255+0) * (1);
static const int H7_hdr_rL = (253+0) * (1);
static const int H7_hdr_rT = (128+0) * (1);
static const int H7_hdr_rG = (64+0) * (1);
static const int H7_hdr_lr = (15+0) * (1);
static const int H7_hdr_mwA = (128+0) * (1);
static const int H7_hdr_mwB = (4+0) * (1);
static const int H7_hdr_mwC = (4+0) * (1);
static const int H7_hdr_mwD = (27+0) * (1);
static const int H7_hdr_mwE = (16+0) * (1);
static const int H7_hdr_mwH = (8+0) * (1);
static const int H7_hdr_mwI = (1+0) * (1);
static const int H7_hdr_mwJ = (20+0) * (1);
static const int H7_hdr_mwK = (51+0) * (1);
static const int H7_hdr_mwL = (62+0) * (1);
static const int H7_hdr_mwT = (0+0) * (1);
static const int H7_hdr_mwG = (0+0) * (1);
static const int H7_hdr_mt = (2+0) * (1);
static const int H7_hdr_mb = (8+0) * (1);
static const int H7_hdr_hA = (24+0) * (1);
static const int H7_hdr_hB = (24+0) * (1);
static const int H7_hdr_hC = (24+0) * (1);
static const int H7_hdr_hD = (24+0) * (1);
static const int H7_hdr_hE = (24+0) * (1);
static const int H7_hdr_hH = (24+0) * (1);
static const int H7_hdr_hI = (24+0) * (1);
static const int H7_hdr_hJ = (24+0) * (1);
static const int H7_hdr_hK = (24+0) * (1);
static const int H7_hdr_hL = (24+0) * (1);
static const int H7_hdr_hS = (24+0) * (1);
static const int H7_hdr_hM = (24+0) * (1);
static const int H7_hdr_hT = (24+0) * (1);


static const long long H7_hdr_a_Volume = tc_vmul(tc_vmul(1, (1<<8)), 16); // 4096
static const long long H7_hdr_b_Volume = tc_vmul(tc_vmul(1, (1<<8)), (1<<8)); // 65536
static const long long H7_hdr_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 11), 7); // 19712
static const long long H7_hdr_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 8), 7), 61); // 874496
static const long long H7_hdr_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 5), 8), 8); // 81920
static const long long H7_hdr_g_Volume = tc_vmul(1, 7); // 7
static const long long H7_hdr_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 3), 16), 21); // 258048
static const long long H7_hdr_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 9), (1<<3)); // 18432
static const long long H7_hdr_j_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 4), 5); // 5120
static const long long H7_hdr_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 9), (1<<3)); // 18432
static const long long H7_hdr_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), (1<<4)), 5); // 20480
static const long long H7_hdr_m_Volume = tc_vmul(1, (1<<8)); // 256
static const long long H7_hdr_s_Volume = tc_vmul(1, (1<<8)); // 256
static const long long H7_hdr_t_Volume = tc_vmul(1, (1<<8)); // 256
static const long long H7_hdr_w_Volume = tc_vmul(1, 9); // 9

#define H7_hdr_USES(X) X(tag) X(fld) X(p1r) X(p1) X(p2) X(dm) X(ws) X(ml) X(ch) X(hx) X(pb) X(pos) X(nb)

struct H7_hdr_T {
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
  i32 H7_hdr_W[tc_rows(H7_hdr_m_Volume, H7_hdr_hM) * H_hdr_ND * tc_nin(10)];
  u8 H7_hdr_WC[tc_rows(H7_hdr_m_Volume, H7_hdr_hM) * H_hdr_ND];
  i32 H7_hdr_W2[H7_hdr_w_Volume * H_hdr_ND * tc_nin(10)];
  u8 H7_hdr_W2C[H7_hdr_w_Volume * H_hdr_ND];
  i32 H7_hdr_W3[tc_rows(H7_hdr_s_Volume, H7_hdr_hS) * H_hdr_ND * tc_nin(10)];
  u8 H7_hdr_W3C[tc_rows(H7_hdr_s_Volume, H7_hdr_hS) * H_hdr_ND];
  i32 H7_hdr_WF[H_hdr_ND * 4];
  u8 H7_hdr_WFC[H_hdr_ND];
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
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H7_hdr_m_Volume, H7_hdr_hM) * H_hdr_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H7_hdr_m_Volume, H7_hdr_hM) * H_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H7_hdr_w_Volume * H_hdr_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H7_hdr_w_Volume * H_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H7_hdr_s_Volume, H7_hdr_hS) * H_hdr_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H7_hdr_s_Volume, H7_hdr_hS) * H_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_hdr_ND * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H7_hdr_g_Volume * H_hdr_NG);
  void H7_hdr_Init( void ) {}
  void H7_hdr_Quit( void ) {}
};
