
static const byte H1_sgn_a_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H1_sgn_a_pos[3+1]={ 0,1,2,3 };
static const byte H1_sgn_a_sa[3+1]={ 0,1,2,3 };
static const byte H1_sgn_a_sb[3+1]={ 0,1,2,3 };
static const byte H1_sgn_a_gh[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H1_sgn_b_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H1_sgn_b_scb[3+1]={ 0,1,2,3 };
static const byte H1_sgn_b_pos[3+1]={ 0,1,2,3 };

static const byte H1_sgn_c_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H1_sgn_c_md[19+1]={ 0,1,2,2,2,3,4,4,4,4,4,4,5,5,6,6,7,8,8,8 };

static const byte H1_sgn_d_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };
static const byte H1_sgn_d_mg[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };


static const byte H1_sgn_m_cg[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };




static const int H1_sgn_rA = (233+0) * (1);
static const int H1_sgn_rB = (205+0) * (1);
static const int H1_sgn_rC = (3+0) * (1);
static const int H1_sgn_rD = (246+0) * (1);
static const int H1_sgn_rT = (55+0) * (1);
static const int H1_sgn_rG = (90+0) * (1);
static const int H1_sgn_lr = (12+0) * (1);
static const int H1_sgn_lrm = (12+0) * (1);
static const int H1_sgn_mbm = (0+0) * (1);
static const int H1_sgn_mwA = (29+0) * (1);
static const int H1_sgn_mwB = (5+0) * (1);
static const int H1_sgn_mwC = (0+0) * (1);
static const int H1_sgn_mwD = (31+0) * (1);
static const int H1_sgn_mwT = (0+0) * (1);
static const int H1_sgn_mwG = (0+0) * (1);
static const int H1_sgn_mt = (6+0) * (1);
static const int H1_sgn_mb = (160+0) * (1);
static const int H1_sgn_hA = (24+0) * (1);
static const int H1_sgn_hB = (24+0) * (1);
static const int H1_sgn_hC = (24+0) * (1);
static const int H1_sgn_hD = (24+0) * (1);
static const int H1_sgn_hM = (24+0) * (1);
static const int H1_sgn_hT = (24+0) * (1);
static const int H1_sgn_hN = (24+0) * (1);


static const long long H1_sgn_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 14), 4), 4), 4), 9); // 8064
static const long long H1_sgn_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 14), 4), 4); // 224
static const long long H1_sgn_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 14), 2), 9); // 252
static const long long H1_sgn_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 14), 3), 16); // 672
static const long long H1_sgn_g_Volume = tc_vmul(1, 1); // 1
static const long long H1_sgn_m_Volume = tc_vmul(tc_vmul(1, 14), 1); // 14
static const long long H1_sgn_n_Volume = tc_vmul(1, 1); // 1
static const long long H1_sgn_t_Volume = tc_vmul(1, 1); // 1

#define H1_sgn_USES(X) X(cg) X(pos) X(sa) X(sb) X(gh) X(scb) X(mag) X(md) X(pp) X(mg)

struct H1_sgn_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H1_sgn_A[tc_rows(H1_sgn_a_Volume, H1_sgn_hA) * H_sgn_ND];
  cm_cnt2 H1_sgn_B[tc_rows(H1_sgn_b_Volume, H1_sgn_hB) * H_sgn_ND];
  cm_cnt2 H1_sgn_C[tc_rows(H1_sgn_c_Volume, H1_sgn_hC) * H_sgn_ND];
  cm_cnt2 H1_sgn_D[tc_rows(H1_sgn_d_Volume, H1_sgn_hD) * H_sgn_ND];
  cm_cnt H1_sgn_MT[tc_rows(H1_sgn_t_Volume, H1_sgn_hT) * H_sgn_MND];
  i32 H1_sgn_W[tc_rows(H1_sgn_m_Volume, H1_sgn_hM) * H_sgn_ND * 16];
  u8 H1_sgn_WC[tc_rows(H1_sgn_m_Volume, H1_sgn_hM) * H_sgn_ND];
  i32 H1_sgn_WM[tc_rows(H1_sgn_n_Volume, H1_sgn_hN) * H_sgn_MND * 3];
  u8 H1_sgn_WMC[tc_rows(H1_sgn_n_Volume, H1_sgn_hN) * H_sgn_MND];
  cm_cnt H1_sgn_G[H1_sgn_g_Volume * H_sgn_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H1_sgn_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_sgn_a_Volume, H1_sgn_hA) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_sgn_b_Volume, H1_sgn_hB) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_sgn_c_Volume, H1_sgn_hC) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_sgn_d_Volume, H1_sgn_hD) * H_sgn_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H1_sgn_t_Volume, H1_sgn_hT) * H_sgn_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_sgn_m_Volume, H1_sgn_hM) * H_sgn_ND * 16)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_sgn_m_Volume, H1_sgn_hM) * H_sgn_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_sgn_n_Volume, H1_sgn_hN) * H_sgn_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_sgn_n_Volume, H1_sgn_hN) * H_sgn_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H1_sgn_g_Volume * H_sgn_NG);
  void H1_sgn_Init( void ) {
  
  }
};
