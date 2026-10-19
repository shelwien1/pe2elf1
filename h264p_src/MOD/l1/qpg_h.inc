
static const byte H1_qpg_a_as[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H1_qpg_a_mag[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H1_qpg_a_xe[18+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18 };
static const byte H1_qpg_a_lq[9+1]={ 0,1,2,3,4,5,6,7,8,9 };

static const byte H1_qpg_b_as[9+1]={ 0,1,2,3,4,5,6,7,8,9 };

static const byte H1_qpg_c_as[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H1_qpg_c_cs[9+1]={ 0,1,2,3,4,5,6,7,8,9 };

static const byte H1_qpg_d_bs[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H1_qpg_d_mag[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H1_qpg_d_bk[9+1]={ 0,1,2,3,4,5,6,7,8,9 };


static const byte H1_qpg_m_as[9+1]={ 0,1,2,3,4,5,6,7,8,9 };




static const int H1_qpg_rA = (235+0) * (1);
static const int H1_qpg_rB = (205+0) * (1);
static const int H1_qpg_rC = (3+0) * (1);
static const int H1_qpg_rD = (111+0) * (1);
static const int H1_qpg_rT = (55+0) * (1);
static const int H1_qpg_rG = (90+0) * (1);
static const int H1_qpg_lr = (12+0) * (1);
static const int H1_qpg_lrm = (12+0) * (1);
static const int H1_qpg_mbm = (0+0) * (1);
static const int H1_qpg_mwA = (29+0) * (1);
static const int H1_qpg_mwB = (0+0) * (1);
static const int H1_qpg_mwC = (0+0) * (1);
static const int H1_qpg_mwD = (31+0) * (1);
static const int H1_qpg_mwT = (0+0) * (1);
static const int H1_qpg_mwG = (0+0) * (1);
static const int H1_qpg_mt = (6+0) * (1);
static const int H1_qpg_mb = (160+0) * (1);
static const int H1_qpg_hA = (24+0) * (1);
static const int H1_qpg_hB = (24+0) * (1);
static const int H1_qpg_hC = (24+0) * (1);
static const int H1_qpg_hD = (24+0) * (1);
static const int H1_qpg_hM = (24+0) * (1);
static const int H1_qpg_hT = (24+0) * (1);
static const int H1_qpg_hN = (24+0) * (1);


static const long long H1_qpg_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 10), 8), 2), 19), 10); // 30400
static const long long H1_qpg_b_Volume = tc_vmul(tc_vmul(1, 10), 1); // 10
static const long long H1_qpg_c_Volume = tc_vmul(tc_vmul(1, 10), 10); // 100
static const long long H1_qpg_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 10), 8), 10); // 800
static const long long H1_qpg_g_Volume = tc_vmul(1, 1); // 1
static const long long H1_qpg_m_Volume = tc_vmul(1, 10); // 10
static const long long H1_qpg_n_Volume = tc_vmul(1, 1); // 1
static const long long H1_qpg_t_Volume = tc_vmul(1, 1); // 1

#define H1_qpg_USES(X) X(as) X(mag) X(ss) X(xe) X(lq) X(cs) X(bs) X(bk)

struct H1_qpg_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H1_qpg_A[tc_rows(H1_qpg_a_Volume, H1_qpg_hA) * H_qpg_ND];
  cm_cnt2 H1_qpg_B[tc_rows(H1_qpg_b_Volume, H1_qpg_hB) * H_qpg_ND];
  cm_cnt2 H1_qpg_C[tc_rows(H1_qpg_c_Volume, H1_qpg_hC) * H_qpg_ND];
  cm_cnt2 H1_qpg_D[tc_rows(H1_qpg_d_Volume, H1_qpg_hD) * H_qpg_ND];
  cm_cnt H1_qpg_MT[tc_rows(H1_qpg_t_Volume, H1_qpg_hT) * H_qpg_MND];
  i32 H1_qpg_W[tc_rows(H1_qpg_m_Volume, H1_qpg_hM) * H_qpg_ND * 16];
  u8 H1_qpg_WC[tc_rows(H1_qpg_m_Volume, H1_qpg_hM) * H_qpg_ND];
  i32 H1_qpg_WM[tc_rows(H1_qpg_n_Volume, H1_qpg_hN) * H_qpg_MND * 3];
  u8 H1_qpg_WMC[tc_rows(H1_qpg_n_Volume, H1_qpg_hN) * H_qpg_MND];
  cm_cnt H1_qpg_G[H1_qpg_g_Volume * H_qpg_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H1_qpg_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_qpg_a_Volume, H1_qpg_hA) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_qpg_b_Volume, H1_qpg_hB) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_qpg_c_Volume, H1_qpg_hC) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_qpg_d_Volume, H1_qpg_hD) * H_qpg_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H1_qpg_t_Volume, H1_qpg_hT) * H_qpg_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_qpg_m_Volume, H1_qpg_hM) * H_qpg_ND * 16)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_qpg_m_Volume, H1_qpg_hM) * H_qpg_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_qpg_n_Volume, H1_qpg_hN) * H_qpg_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_qpg_n_Volume, H1_qpg_hN) * H_qpg_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H1_qpg_g_Volume * H_qpg_NG);
  void H1_qpg_Init( void ) {
  
  }
};
