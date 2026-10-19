
static const byte H1_cbp_a_pc[4+1]={ 0,1,2,2,3 };
static const byte H1_cbp_a_cls[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H1_cbp_a_st[9+1]={ 0,1,2,3,4,5,6,7,8,9 };

static const byte H1_cbp_b_col[12+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12 };
static const byte H1_cbp_b_q6[8+1]={ 0,1,1,2,2,2,2,3,3 };

static const byte H1_cbp_c_cls[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte H1_cbp_c_col[12+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12 };

static const byte H1_cbp_d_inc[3+1]={ 0,1,2,3 };
static const byte H1_cbp_d_q6[8+1]={ 0,1,2,3,4,5,6,7,8 };






static const int H1_cbp_rA = (235+0) * (1);
static const int H1_cbp_rB = (245+0) * (1);
static const int H1_cbp_rC = (64+0) * (1);
static const int H1_cbp_rD = (64+0) * (1);
static const int H1_cbp_rT = (55+0) * (1);
static const int H1_cbp_rG = (90+0) * (1);
static const int H1_cbp_lr = (15+0) * (1);
static const int H1_cbp_lrm = (12+0) * (1);
static const int H1_cbp_mbm = (0+0) * (1);
static const int H1_cbp_mwA = (0+0) * (1);
static const int H1_cbp_mwB = (0+0) * (1);
static const int H1_cbp_mwC = (0+0) * (1);
static const int H1_cbp_mwD = (44+0) * (1);
static const int H1_cbp_mwT = (0+0) * (1);
static const int H1_cbp_mwG = (0+0) * (1);
static const int H1_cbp_mt = (6+0) * (1);
static const int H1_cbp_mb = (5+0) * (1);
static const int H1_cbp_hA = (24+0) * (1);
static const int H1_cbp_hB = (24+0) * (1);
static const int H1_cbp_hC = (24+0) * (1);
static const int H1_cbp_hD = (24+0) * (1);
static const int H1_cbp_hM = (24+0) * (1);
static const int H1_cbp_hT = (24+0) * (1);
static const int H1_cbp_hN = (24+0) * (1);


static const long long H1_cbp_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 4), 9), 10); // 1080
static const long long H1_cbp_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 13), 4); // 52
static const long long H1_cbp_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 9), 13), 3); // 351
static const long long H1_cbp_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 3), 9); // 108
static const long long H1_cbp_g_Volume = tc_vmul(1, 1); // 1
static const long long H1_cbp_m_Volume = tc_vmul(1, 1); // 1
static const long long H1_cbp_n_Volume = tc_vmul(1, 1); // 1
static const long long H1_cbp_t_Volume = tc_vmul(1, 1); // 1

#define H1_cbp_USES(X) X(inc) X(pc) X(cls) X(st) X(col) X(q6)

struct H1_cbp_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H1_cbp_A[tc_rows(H1_cbp_a_Volume, H1_cbp_hA) * H_cbp_ND];
  cm_cnt2 H1_cbp_B[tc_rows(H1_cbp_b_Volume, H1_cbp_hB) * H_cbp_ND];
  cm_cnt2 H1_cbp_C[tc_rows(H1_cbp_c_Volume, H1_cbp_hC) * H_cbp_ND];
  cm_cnt2 H1_cbp_D[tc_rows(H1_cbp_d_Volume, H1_cbp_hD) * H_cbp_ND];
  cm_cnt H1_cbp_MT[tc_rows(H1_cbp_t_Volume, H1_cbp_hT) * H_cbp_MND];
  i32 H1_cbp_W[tc_rows(H1_cbp_m_Volume, H1_cbp_hM) * H_cbp_ND * 16];
  u8 H1_cbp_WC[tc_rows(H1_cbp_m_Volume, H1_cbp_hM) * H_cbp_ND];
  i32 H1_cbp_WM[tc_rows(H1_cbp_n_Volume, H1_cbp_hN) * H_cbp_MND * 3];
  u8 H1_cbp_WMC[tc_rows(H1_cbp_n_Volume, H1_cbp_hN) * H_cbp_MND];
  cm_cnt H1_cbp_G[H1_cbp_g_Volume * H_cbp_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H1_cbp_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_cbp_a_Volume, H1_cbp_hA) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_cbp_b_Volume, H1_cbp_hB) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_cbp_c_Volume, H1_cbp_hC) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_cbp_d_Volume, H1_cbp_hD) * H_cbp_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H1_cbp_t_Volume, H1_cbp_hT) * H_cbp_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_cbp_m_Volume, H1_cbp_hM) * H_cbp_ND * 16)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_cbp_m_Volume, H1_cbp_hM) * H_cbp_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_cbp_n_Volume, H1_cbp_hN) * H_cbp_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_cbp_n_Volume, H1_cbp_hN) * H_cbp_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H1_cbp_g_Volume * H_cbp_NG);
  void H1_cbp_Init( void ) {
  
  }
};
