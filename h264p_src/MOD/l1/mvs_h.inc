
static const byte H1_mvs_a_ma[7+1]={ 0,1,2,3,4,4,4,4 };
static const byte H1_mvs_a_mb[7+1]={ 0,1,2,3,4,5,5,5 };
static const byte H1_mvs_a_oth[5+1]={ 0,1,2,3,4,5 };
static const byte H1_mvs_a_sp[8+1]={ 0,1,2,3,4,4,4,5,6 };

static const byte H1_mvs_b_kind[3+1]={ 0,1,2,3 };
static const byte H1_mvs_b_ps[3+1]={ 0,1,2,3 };
static const byte H1_mvs_b_rf[3+1]={ 0,1,2,3 };
static const byte H1_mvs_b_zd[12+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12 };


static const byte H1_mvs_d_kind[3+1]={ 0,1,2,3 };
static const byte H1_mvs_d_pd[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H1_mvs_d_nr[3+1]={ 0,1,2,3 };

static const byte H1_mvs_g_kind[3+1]={ 0,1,2,3 };

static const byte H1_mvs_m_kind[3+1]={ 0,1,2,3 };
static const byte H1_mvs_m_pd[5+1]={ 0,1,2,3,4,5 };

static const byte H1_mvs_n_kind[3+1]={ 0,1,2,3 };

static const byte H1_mvs_t_kind[3+1]={ 0,1,2,3 };


static const int H1_mvs_rA = (253+0) * (1);
static const int H1_mvs_rB = (0+0) * (1);
static const int H1_mvs_rC = (6+0) * (1);
static const int H1_mvs_rD = (255+0) * (1);
static const int H1_mvs_rT = (30+0) * (1);
static const int H1_mvs_rG = (72+0) * (1);
static const int H1_mvs_lr = (12+0) * (1);
static const int H1_mvs_lrm = (30+0) * (1);
static const int H1_mvs_mbm = (0+0) * (1);
static const int H1_mvs_mwA = (8+0) * (1);
static const int H1_mvs_mwB = (8+0) * (1);
static const int H1_mvs_mwC = (146+0) * (1);
static const int H1_mvs_mwD = (46+0) * (1);
static const int H1_mvs_mwT = (0+0) * (1);
static const int H1_mvs_mwG = (0+0) * (1);
static const int H1_mvs_mt = (6+0) * (1);
static const int H1_mvs_mb = (108+0) * (1);
static const int H1_mvs_hA = (16+0) * (1);
static const int H1_mvs_hB = (24+0) * (1);
static const int H1_mvs_hC = (24+0) * (1);
static const int H1_mvs_hD = (24+0) * (1);
static const int H1_mvs_hM = (24+0) * (1);
static const int H1_mvs_hT = (24+0) * (1);
static const int H1_mvs_hN = (24+0) * (1);


static const long long H1_mvs_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 5), 6), 6), 7); // 7560
static const long long H1_mvs_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 2), 2), 4), 4), 13); // 3328
static const long long H1_mvs_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 1), 1); // 3
static const long long H1_mvs_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 2), 9), 1), 4); // 288
static const long long H1_mvs_g_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 1); // 4
static const long long H1_mvs_m_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 6); // 24
static const long long H1_mvs_n_Volume = tc_vmul(1, 4); // 4
static const long long H1_mvs_t_Volume = tc_vmul(tc_vmul(1, 4), 2); // 8

#define H1_mvs_USES(X) X(kind) X(comp) X(ma) X(mb) X(oth) X(sp) X(lst) X(ps) X(rf) X(zd) X(pd) X(nr)

struct H1_mvs_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H1_mvs_A[tc_rows(H1_mvs_a_Volume, H1_mvs_hA) * H_mvs_ND];
  cm_cnt2 H1_mvs_B[tc_rows(H1_mvs_b_Volume, H1_mvs_hB) * H_mvs_ND];
  cm_cnt2 H1_mvs_C[tc_rows(H1_mvs_c_Volume, H1_mvs_hC) * H_mvs_ND];
  cm_cnt2 H1_mvs_D[tc_rows(H1_mvs_d_Volume, H1_mvs_hD) * H_mvs_ND];
  cm_cnt H1_mvs_MT[tc_rows(H1_mvs_t_Volume, H1_mvs_hT) * H_mvs_MND];
  i32 H1_mvs_W[tc_rows(H1_mvs_m_Volume, H1_mvs_hM) * H_mvs_ND * 16];
  u8 H1_mvs_WC[tc_rows(H1_mvs_m_Volume, H1_mvs_hM) * H_mvs_ND];
  i32 H1_mvs_WM[tc_rows(H1_mvs_n_Volume, H1_mvs_hN) * H_mvs_MND * 3];
  u8 H1_mvs_WMC[tc_rows(H1_mvs_n_Volume, H1_mvs_hN) * H_mvs_MND];
  cm_cnt H1_mvs_G[H1_mvs_g_Volume * H_mvs_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H1_mvs_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_mvs_a_Volume, H1_mvs_hA) * H_mvs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_mvs_b_Volume, H1_mvs_hB) * H_mvs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_mvs_c_Volume, H1_mvs_hC) * H_mvs_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_mvs_d_Volume, H1_mvs_hD) * H_mvs_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H1_mvs_t_Volume, H1_mvs_hT) * H_mvs_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_mvs_m_Volume, H1_mvs_hM) * H_mvs_ND * 16)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_mvs_m_Volume, H1_mvs_hM) * H_mvs_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_mvs_n_Volume, H1_mvs_hN) * H_mvs_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_mvs_n_Volume, H1_mvs_hN) * H_mvs_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H1_mvs_g_Volume * H_mvs_NG);
  void H1_mvs_Init( void ) {
  
  }
};
