
static const byte H1_mvg_a_ox[3+1]={ 0,1,2,3 };
static const byte H1_mvg_a_ns[6+1]={ 0,1,2,3,4,5,6 };

static const byte H1_mvg_b_zs[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H1_mvg_b_mag[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H1_mvg_c_zs[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H1_mvg_c_ds[9+1]={ 0,1,2,3,4,5,6,7,8,9 };

static const byte H1_mvg_d_ds[9+1]={ 0,1,2,3,4,5,6,7,8,9 };
static const byte H1_mvg_d_mag[7+1]={ 0,1,2,3,4,5,6,7 };






static const int H1_mvg_rA = (224+0) * (1);
static const int H1_mvg_rB = (205+0) * (1);
static const int H1_mvg_rC = (64+0) * (1);
static const int H1_mvg_rD = (111+0) * (1);
static const int H1_mvg_rT = (55+0) * (1);
static const int H1_mvg_rG = (90+0) * (1);
static const int H1_mvg_lr = (12+0) * (1);
static const int H1_mvg_lrm = (12+0) * (1);
static const int H1_mvg_mbm = (0+0) * (1);
static const int H1_mvg_mwA = (0+0) * (1);
static const int H1_mvg_mwB = (16+0) * (1);
static const int H1_mvg_mwC = (0+0) * (1);
static const int H1_mvg_mwD = (31+0) * (1);
static const int H1_mvg_mwT = (0+0) * (1);
static const int H1_mvg_mwG = (0+0) * (1);
static const int H1_mvg_mt = (6+0) * (1);
static const int H1_mvg_mb = (160+0) * (1);
static const int H1_mvg_hA = (24+0) * (1);
static const int H1_mvg_hB = (24+0) * (1);
static const int H1_mvg_hC = (24+0) * (1);
static const int H1_mvg_hD = (24+0) * (1);
static const int H1_mvg_hM = (24+0) * (1);
static const int H1_mvg_hT = (24+0) * (1);
static const int H1_mvg_hN = (24+0) * (1);


static const long long H1_mvg_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 2), 4), 7); // 112
static const long long H1_mvg_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 9), 8), 2); // 144
static const long long H1_mvg_c_Volume = tc_vmul(tc_vmul(1, 9), 10); // 90
static const long long H1_mvg_d_Volume = tc_vmul(tc_vmul(1, 10), 8); // 80
static const long long H1_mvg_g_Volume = tc_vmul(1, 1); // 1
static const long long H1_mvg_m_Volume = 1; // 1
static const long long H1_mvg_n_Volume = tc_vmul(1, 1); // 1
static const long long H1_mvg_t_Volume = tc_vmul(1, 1); // 1

#define H1_mvg_USES(X) X(comp) X(lst) X(ox) X(ns) X(zs) X(mag) X(ds)

struct H1_mvg_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H1_mvg_A[tc_rows(H1_mvg_a_Volume, H1_mvg_hA) * H_mvg_ND];
  cm_cnt2 H1_mvg_B[tc_rows(H1_mvg_b_Volume, H1_mvg_hB) * H_mvg_ND];
  cm_cnt2 H1_mvg_C[tc_rows(H1_mvg_c_Volume, H1_mvg_hC) * H_mvg_ND];
  cm_cnt2 H1_mvg_D[tc_rows(H1_mvg_d_Volume, H1_mvg_hD) * H_mvg_ND];
  cm_cnt H1_mvg_MT[tc_rows(H1_mvg_t_Volume, H1_mvg_hT) * H_mvg_MND];
  i32 H1_mvg_W[tc_rows(H1_mvg_m_Volume, H1_mvg_hM) * H_mvg_ND * 16];
  u8 H1_mvg_WC[tc_rows(H1_mvg_m_Volume, H1_mvg_hM) * H_mvg_ND];
  i32 H1_mvg_WM[tc_rows(H1_mvg_n_Volume, H1_mvg_hN) * H_mvg_MND * 3];
  u8 H1_mvg_WMC[tc_rows(H1_mvg_n_Volume, H1_mvg_hN) * H_mvg_MND];
  cm_cnt H1_mvg_G[H1_mvg_g_Volume * H_mvg_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H1_mvg_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_mvg_a_Volume, H1_mvg_hA) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_mvg_b_Volume, H1_mvg_hB) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_mvg_c_Volume, H1_mvg_hC) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_mvg_d_Volume, H1_mvg_hD) * H_mvg_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H1_mvg_t_Volume, H1_mvg_hT) * H_mvg_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_mvg_m_Volume, H1_mvg_hM) * H_mvg_ND * 16)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_mvg_m_Volume, H1_mvg_hM) * H_mvg_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_mvg_n_Volume, H1_mvg_hN) * H_mvg_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_mvg_n_Volume, H1_mvg_hN) * H_mvg_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H1_mvg_g_Volume * H_mvg_NG);
  void H1_mvg_Init( void ) {
  
  }
};
