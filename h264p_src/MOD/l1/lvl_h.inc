
static const byte H1_lvl_a_lt[7+1]={ 0,0,1,2,3,4,5,6 };
static const byte H1_lvl_a_lh[64+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,35,36,37,38,39,40,41,42,43,44,45,45,46,47,47,48,49,50,51,52,53,54,55,55,56,57,58,59,60 };
static const byte H1_lvl_a_pos[5+1]={ 0,1,2,3,4,5 };

static const byte H1_lvl_b_cg[13+1]={ 0,1,2,3,4,5,5,5,5,5,5,5,5,5 };
static const byte H1_lvl_b_pos[5+1]={ 0,1,2,3,3,4 };
static const byte H1_lvl_b_tot[7+1]={ 0,0,1,2,3,3,4,5 };

static const byte H1_lvl_c_cg[13+1]={ 0,1,1,2,3,4,4,4,4,4,4,4,4,4 };
static const byte H1_lvl_c_sa[3+1]={ 0,1,2,3 };
static const byte H1_lvl_c_sb[3+1]={ 0,1,2,3 };
static const byte H1_lvl_c_q6[8+1]={ 0,1,2,3,4,5,5,5,5 };
static const byte H1_lvl_c_lh[64+1]={ 0,1,2,3,4,5,5,6,7,8,9,10,11,12,13,14,15,16,17,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,52,53,53,54,55,56,57,58,59 };

static const byte H1_lvl_d_cg[13+1]={ 0,1,1,2,3,4,4,4,4,4,4,4,4,4 };
static const byte H1_lvl_d_md[19+1]={ 0,0,1,2,2,2,2,2,2,3,3,3,3,3,4,4,5,6,6,6 };
static const byte H1_lvl_d_lt[7+1]={ 0,1,2,3,3,4,4,5 };






static const int H1_lvl_rA = (253+0) * (1);
static const int H1_lvl_rB = (242+0) * (1);
static const int H1_lvl_rC = (255+0) * (1);
static const int H1_lvl_rD = (254+0) * (1);
static const int H1_lvl_rT = (129+0) * (1);
static const int H1_lvl_rG = (0+0) * (1);
static const int H1_lvl_lr = (15+0) * (1);
static const int H1_lvl_lrm = (0+0) * (1);
static const int H1_lvl_mbm = (0+0) * (1);
static const int H1_lvl_mwA = (60+0) * (1);
static const int H1_lvl_mwB = (8+0) * (1);
static const int H1_lvl_mwC = (8+0) * (1);
static const int H1_lvl_mwD = (59+0) * (1);
static const int H1_lvl_mwT = (49+0) * (1);
static const int H1_lvl_mwG = (0+0) * (1);
static const int H1_lvl_mt = (0+0) * (1);
static const int H1_lvl_mb = (77+0) * (1);
static const int H1_lvl_hA = (24+0) * (1);
static const int H1_lvl_hB = (24+0) * (1);
static const int H1_lvl_hC = (24+0) * (1);
static const int H1_lvl_hD = (24+0) * (1);
static const int H1_lvl_hM = (24+0) * (1);
static const int H1_lvl_hT = (24+0) * (1);
static const int H1_lvl_hN = (24+0) * (1);


static const long long H1_lvl_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 7), 61), 6); // 2562
static const long long H1_lvl_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 6), 5), 6), 3); // 540
static const long long H1_lvl_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 5), 4), 4), 6), 60); // 28800
static const long long H1_lvl_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 5), 2), 7), 6); // 420
static const long long H1_lvl_g_Volume = tc_vmul(1, 1); // 1
static const long long H1_lvl_m_Volume = tc_vmul(tc_vmul(1, 2), 2); // 4
static const long long H1_lvl_n_Volume = tc_vmul(1, 1); // 1
static const long long H1_lvl_t_Volume = tc_vmul(1, 1); // 1

#define H1_lvl_USES(X) X(lt) X(lh) X(pos) X(cg) X(tot) X(fq) X(sa) X(sb) X(q6) X(g1) X(md)

struct H1_lvl_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H1_lvl_A[tc_rows(H1_lvl_a_Volume, H1_lvl_hA) * H_lvl_ND];
  cm_cnt2 H1_lvl_B[tc_rows(H1_lvl_b_Volume, H1_lvl_hB) * H_lvl_ND];
  cm_cnt2 H1_lvl_C[tc_rows(H1_lvl_c_Volume, H1_lvl_hC) * H_lvl_ND];
  cm_cnt2 H1_lvl_D[tc_rows(H1_lvl_d_Volume, H1_lvl_hD) * H_lvl_ND];
  cm_cnt H1_lvl_MT[tc_rows(H1_lvl_t_Volume, H1_lvl_hT) * H_lvl_MND];
  i32 H1_lvl_W[tc_rows(H1_lvl_m_Volume, H1_lvl_hM) * H_lvl_ND * 16];
  u8 H1_lvl_WC[tc_rows(H1_lvl_m_Volume, H1_lvl_hM) * H_lvl_ND];
  i32 H1_lvl_WM[tc_rows(H1_lvl_n_Volume, H1_lvl_hN) * H_lvl_MND * 3];
  u8 H1_lvl_WMC[tc_rows(H1_lvl_n_Volume, H1_lvl_hN) * H_lvl_MND];
  cm_cnt H1_lvl_G[H1_lvl_g_Volume * H_lvl_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H1_lvl_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_lvl_a_Volume, H1_lvl_hA) * H_lvl_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_lvl_b_Volume, H1_lvl_hB) * H_lvl_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_lvl_c_Volume, H1_lvl_hC) * H_lvl_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_lvl_d_Volume, H1_lvl_hD) * H_lvl_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H1_lvl_t_Volume, H1_lvl_hT) * H_lvl_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_lvl_m_Volume, H1_lvl_hM) * H_lvl_ND * 16)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_lvl_m_Volume, H1_lvl_hM) * H_lvl_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_lvl_n_Volume, H1_lvl_hN) * H_lvl_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_lvl_n_Volume, H1_lvl_hN) * H_lvl_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H1_lvl_g_Volume * H_lvl_NG);
  void H1_lvl_Init( void ) {
  
  }
};
