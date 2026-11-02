
static const byte H3_sbr_a_kd[14+1]={ 0,1,1,2,3,3,4,4,4,5,6,7,8,8,8 };
static const byte H3_sbr_a_bd[30+1]={ 0,1,1,1,2,3,3,4,5,5,6,6,6,7,8,9,10,10,11,11,11,11,11,11,11,11,11,11,11,11,11 };

static const byte H3_sbr_b_kd[14+1]={ 0,0,1,2,2,2,2,2,2,2,2,2,2,2,3 };
static const byte H3_sbr_b_lv[11+1]={ 0,0,1,2,2,2,2,3,4,4,5,5 };

static const byte H3_sbr_c_kd[14+1]={ 0,1,2,2,2,2,2,2,3,3,4,5,5,5,6 };
static const byte H3_sbr_c_d1[10+1]={ 0,1,2,3,4,4,4,4,4,4,4 };
static const byte H3_sbr_c_d2[6+1]={ 0,1,2,3,4,5,6 };
static const byte H3_sbr_c_rm[14+1]={ 0,1,2,2,3,3,4,4,4,4,4,4,4,5,5 };

static const byte H3_sbr_d_kd[14+1]={ 0,1,2,3,3,4,5,5,5,5,5,5,5,6,6 };
static const byte H3_sbr_d_bd[12+1]={ 0,0,0,1,1,2,2,3,4,4,5,5,5 };

static const byte H3_sbr_e_kd[14+1]={ 0,1,2,3,3,3,4,4,4,4,5,5,5,5,6 };
static const byte H3_sbr_e_d1[10+1]={ 0,1,2,3,4,4,4,4,4,5,5 };
static const byte H3_sbr_e_pv[5+1]={ 0,1,2,3,4,5 };
static const byte H3_sbr_e_rs[14+1]={ 0,0,1,2,3,4,5,5,5,5,5,6,6,6,6 };

static const byte H3_sbr_g_d1[5+1]={ 0,1,2,3,4,5 };

static const byte H3_sbr_h_kd[14+1]={ 0,1,1,2,2,2,2,2,2,2,3,3,3,3,3 };
static const byte H3_sbr_h_nb[20+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20 };
static const byte H3_sbr_h_bd[3+1]={ 0,1,2,3 };

static const byte H3_sbr_m_kd[14+1]={ 0,0,0,1,2,3,4,4,4,4,4,4,4,4,5 };



static const int H3_sbr_rA = (254+0) * (1);
static const int H3_sbr_rB = (255+0) * (1);
static const int H3_sbr_rC = (255+0) * (1);
static const int H3_sbr_rD = (242+0) * (1);
static const int H3_sbr_rE = (255+0) * (1);
static const int H3_sbr_rH = (207+0) * (1);
static const int H3_sbr_rT = (128+0) * (1);
static const int H3_sbr_rG = (64+0) * (1);
static const int H3_sbr_lr = (14+0) * (1);
static const int H3_sbr_mwA = (4+0) * (1);
static const int H3_sbr_mwB = (22+0) * (1);
static const int H3_sbr_mwC = (0+0) * (1);
static const int H3_sbr_mwD = (62+0) * (1);
static const int H3_sbr_mwE = (0+0) * (1);
static const int H3_sbr_mwH = (40+0) * (1);
static const int H3_sbr_mwT = (0+0) * (1);
static const int H3_sbr_mwG = (17+0) * (1);
static const int H3_sbr_mt = (4+0) * (1);
static const int H3_sbr_mb = (128+0) * (1);
static const int H3_sbr_hA = (24+0) * (1);
static const int H3_sbr_hB = (24+0) * (1);
static const int H3_sbr_hC = (24+0) * (1);
static const int H3_sbr_hD = (24+0) * (1);
static const int H3_sbr_hE = (24+0) * (1);
static const int H3_sbr_hH = (24+0) * (1);
static const int H3_sbr_hM = (24+0) * (1);
static const int H3_sbr_hT = (24+0) * (1);


static const long long H3_sbr_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 9), 12), 2); // 216
static const long long H3_sbr_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 6), 1); // 24
static const long long H3_sbr_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 7), 5), 7), 6); // 1470
static const long long H3_sbr_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 7), 2), 2), 6); // 168
static const long long H3_sbr_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 7), 6), 6), 7); // 1764
static const long long H3_sbr_g_Volume = tc_vmul(1, 6); // 6
static const long long H3_sbr_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 21), 4); // 336
static const long long H3_sbr_m_Volume = tc_vmul(1, 6); // 6
static const long long H3_sbr_t_Volume = tc_vmul(1, 3); // 3

#define H3_sbr_USES(X) X(kd) X(bd) X(ar) X(lv) X(d1) X(d2) X(rm) X(fr) X(pv) X(rs) X(nb)

struct H3_sbr_T {
  cm_cnt2 H3_sbr_A[tc_rows(H3_sbr_a_Volume, H3_sbr_hA) * H_sbr_ND];
  cm_cnt2 H3_sbr_B[tc_rows(H3_sbr_b_Volume, H3_sbr_hB) * H_sbr_ND];
  cm_cnt2 H3_sbr_C[tc_rows(H3_sbr_c_Volume, H3_sbr_hC) * H_sbr_ND];
  cm_cnt2 H3_sbr_D[tc_rows(H3_sbr_d_Volume, H3_sbr_hD) * H_sbr_ND];
  cm_cnt2 H3_sbr_E[tc_rows(H3_sbr_e_Volume, H3_sbr_hE) * H_sbr_ND];
  cm_cnt2 H3_sbr_H[tc_rows(H3_sbr_h_Volume, H3_sbr_hH) * H_sbr_ND];
  cm_cnt H3_sbr_MT[tc_rows(H3_sbr_t_Volume, H3_sbr_hT) * H_sbr_MND];
  i32 H3_sbr_W[tc_rows(H3_sbr_m_Volume, H3_sbr_hM) * H_sbr_ND * tc_nin(6)];
  u8 H3_sbr_WC[tc_rows(H3_sbr_m_Volume, H3_sbr_hM) * H_sbr_ND];
  cm_cnt H3_sbr_G[H3_sbr_g_Volume * H_sbr_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H3_sbr_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sbr_a_Volume, H3_sbr_hA) * H_sbr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sbr_b_Volume, H3_sbr_hB) * H_sbr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sbr_c_Volume, H3_sbr_hC) * H_sbr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sbr_d_Volume, H3_sbr_hD) * H_sbr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sbr_e_Volume, H3_sbr_hE) * H_sbr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sbr_h_Volume, H3_sbr_hH) * H_sbr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H3_sbr_t_Volume, H3_sbr_hT) * H_sbr_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_sbr_m_Volume, H3_sbr_hM) * H_sbr_ND * tc_nin(6))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_sbr_m_Volume, H3_sbr_hM) * H_sbr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H3_sbr_g_Volume * H_sbr_NG);
  void H3_sbr_Init( void ) {}
  void H3_sbr_Quit( void ) {}
};
