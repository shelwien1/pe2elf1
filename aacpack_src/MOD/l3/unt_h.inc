
static const byte H3_unt_a_fld[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

static const byte H3_unt_b_fld[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte H3_unt_b_ctx[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

static const byte H3_unt_c_fld[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte H3_unt_c_p1[6+1]={ 0,1,2,3,4,5,6 };

static const byte H3_unt_d_fld[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte H3_unt_d_ctx[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H3_unt_e_fld[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte H3_unt_e_p1[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };


static const byte H3_unt_h_fld[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte H3_unt_h_ctx[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H3_unt_m_fld[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

static const byte H3_unt_t_fld[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };


static const int H3_unt_rA = (255+0) * (1);
static const int H3_unt_rB = (255+0) * (1);
static const int H3_unt_rC = (242+0) * (1);
static const int H3_unt_rD = (255+0) * (1);
static const int H3_unt_rE = (254+0) * (1);
static const int H3_unt_rH = (130+0) * (1);
static const int H3_unt_rT = (128+0) * (1);
static const int H3_unt_rG = (64+0) * (1);
static const int H3_unt_lr = (15+0) * (1);
static const int H3_unt_mwA = (27+0) * (1);
static const int H3_unt_mwB = (4+0) * (1);
static const int H3_unt_mwC = (4+0) * (1);
static const int H3_unt_mwD = (51+0) * (1);
static const int H3_unt_mwE = (20+0) * (1);
static const int H3_unt_mwH = (128+0) * (1);
static const int H3_unt_mwT = (0+0) * (1);
static const int H3_unt_mwG = (0+0) * (1);
static const int H3_unt_mt = (2+0) * (1);
static const int H3_unt_mb = (8+0) * (1);
static const int H3_unt_hA = (24+0) * (1);
static const int H3_unt_hB = (24+0) * (1);
static const int H3_unt_hC = (24+0) * (1);
static const int H3_unt_hD = (24+0) * (1);
static const int H3_unt_hE = (24+0) * (1);
static const int H3_unt_hH = (24+0) * (1);
static const int H3_unt_hM = (24+0) * (1);
static const int H3_unt_hT = (24+0) * (1);


static const long long H3_unt_a_Volume = tc_vmul(1, 16); // 16
static const long long H3_unt_b_Volume = tc_vmul(tc_vmul(1, 16), 16); // 256
static const long long H3_unt_c_Volume = tc_vmul(tc_vmul(1, 16), 7); // 112
static const long long H3_unt_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 16), 8), 3); // 384
static const long long H3_unt_e_Volume = tc_vmul(tc_vmul(1, 16), 11); // 176
static const long long H3_unt_g_Volume = 1; // 1
static const long long H3_unt_h_Volume = tc_vmul(tc_vmul(1, 16), 8); // 128
static const long long H3_unt_m_Volume = tc_vmul(1, 16); // 16
static const long long H3_unt_t_Volume = tc_vmul(1, 16); // 16

#define H3_unt_USES(X) X(fld) X(ctx) X(p1)

struct H3_unt_T {
  cm_cnt2 H3_unt_A[tc_rows(H3_unt_a_Volume, H3_unt_hA) * H_unt_ND];
  cm_cnt2 H3_unt_B[tc_rows(H3_unt_b_Volume, H3_unt_hB) * H_unt_ND];
  cm_cnt2 H3_unt_C[tc_rows(H3_unt_c_Volume, H3_unt_hC) * H_unt_ND];
  cm_cnt2 H3_unt_D[tc_rows(H3_unt_d_Volume, H3_unt_hD) * H_unt_ND];
  cm_cnt2 H3_unt_E[tc_rows(H3_unt_e_Volume, H3_unt_hE) * H_unt_ND];
  cm_cnt2 H3_unt_H[tc_rows(H3_unt_h_Volume, H3_unt_hH) * H_unt_ND];
  cm_cnt H3_unt_MT[tc_rows(H3_unt_t_Volume, H3_unt_hT) * H_unt_MND];
  i32 H3_unt_W[tc_rows(H3_unt_m_Volume, H3_unt_hM) * H_unt_ND * tc_nin(6)];
  u8 H3_unt_WC[tc_rows(H3_unt_m_Volume, H3_unt_hM) * H_unt_ND];
  cm_cnt H3_unt_G[H3_unt_g_Volume * H_unt_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H3_unt_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_unt_a_Volume, H3_unt_hA) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_unt_b_Volume, H3_unt_hB) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_unt_c_Volume, H3_unt_hC) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_unt_d_Volume, H3_unt_hD) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_unt_e_Volume, H3_unt_hE) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_unt_h_Volume, H3_unt_hH) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H3_unt_t_Volume, H3_unt_hT) * H_unt_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_unt_m_Volume, H3_unt_hM) * H_unt_ND * tc_nin(6))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_unt_m_Volume, H3_unt_hM) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H3_unt_g_Volume * H_unt_NG);
  void H3_unt_Init( void ) {}
  void H3_unt_Quit( void ) {}
};
