
static const byte H1_qpg_a_mag[7+1]={ 0,0,1,2,3,4,5,6 };
static const byte H1_qpg_a_ss[9+1]={ 0,0,0,1,2,3,4,5,6,6 };

static const byte H1_qpg_b_cls[8+1]={ 0,0,0,1,2,3,4,5,5 };

static const byte H1_qpg_c_cs[9+1]={ 0,0,0,1,2,3,4,5,5,6 };

static const byte H1_qpg_d_bs[9+1]={ 0,0,0,1,1,2,3,4,5,6 };
static const byte H1_qpg_d_mag[7+1]={ 0,0,1,1,2,3,3,3 };






static const int H1_qpg_rA = (253+0) * (1);
static const int H1_qpg_rB = (201+0) * (1);
static const int H1_qpg_rC = (254+0) * (1);
static const int H1_qpg_rD = (254+0) * (1);
static const int H1_qpg_rT = (0+0) * (1);
static const int H1_qpg_rG = (0+0) * (1);
static const int H1_qpg_lr = (15+0) * (1);
static const int H1_qpg_lrm = (0+0) * (1);
static const int H1_qpg_mbm = (0+0) * (1);
static const int H1_qpg_mwA = (1+0) * (1);
static const int H1_qpg_mwB = (21+0) * (1);
static const int H1_qpg_mwC = (8+0) * (1);
static const int H1_qpg_mwD = (0+0) * (1);
static const int H1_qpg_mwT = (0+0) * (1);
static const int H1_qpg_mwG = (0+0) * (1);
static const int H1_qpg_mt = (0+0) * (1);
static const int H1_qpg_mb = (192+0) * (1);
static const int H1_qpg_hA = (24+0) * (1);
static const int H1_qpg_hB = (24+0) * (1);
static const int H1_qpg_hC = (24+0) * (1);
static const int H1_qpg_hD = (24+0) * (1);
static const int H1_qpg_hM = (24+0) * (1);
static const int H1_qpg_hT = (24+0) * (1);
static const int H1_qpg_hN = (24+0) * (1);


static const long long H1_qpg_a_Volume = tc_vmul(tc_vmul(1, 7), 7); // 49
static const long long H1_qpg_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 6), 3), 3); // 54
static const long long H1_qpg_c_Volume = tc_vmul(tc_vmul(1, 7), 1); // 7
static const long long H1_qpg_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 7), 4), 2); // 56
static const long long H1_qpg_g_Volume = tc_vmul(1, 1); // 1
static const long long H1_qpg_m_Volume = 1; // 1
static const long long H1_qpg_n_Volume = tc_vmul(1, 1); // 1
static const long long H1_qpg_t_Volume = tc_vmul(1, 1); // 1

#define H1_qpg_USES(X) X(mag) X(ss) X(cls) X(st) X(ls) X(cs) X(bs) X(bk)

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
