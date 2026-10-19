
static const byte H1_nal_a_fld[31+1]={ 0,1,2,3,4,4,4,4,4,4,4,4,4,4,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6 };
static const byte H1_nal_a_ctx[31+1]={ 0,1,2,2,2,2,3,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 };

static const byte H1_nal_b_fld[31+1]={ 0,0,0,0,1,1,2,2,2,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };
static const byte H1_nal_b_p2[3+1]={ 0,1,2,3 };

static const byte H1_nal_c_fld[31+1]={ 0,1,1,2,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };
static const byte H1_nal_c_ctx[31+1]={ 0,1,1,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };

static const byte H1_nal_d_fld[31+1]={ 0,0,1,2,3,4,4,5,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7 };


static const byte H1_nal_m_fld[31+1]={ 0,0,1,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };




static const int H1_nal_rA = (252+0) * (1);
static const int H1_nal_rB = (0+0) * (1);
static const int H1_nal_rC = (2+0) * (1);
static const int H1_nal_rD = (16+0) * (1);
static const int H1_nal_rT = (252+0) * (1);
static const int H1_nal_rG = (0+0) * (1);
static const int H1_nal_lr = (31+0) * (1);
static const int H1_nal_lrm = (0+0) * (1);
static const int H1_nal_mbm = (0+0) * (1);
static const int H1_nal_mwA = (0+0) * (1);
static const int H1_nal_mwB = (88+0) * (1);
static const int H1_nal_mwC = (112+0) * (1);
static const int H1_nal_mwD = (0+0) * (1);
static const int H1_nal_mwT = (0+0) * (1);
static const int H1_nal_mwG = (0+0) * (1);
static const int H1_nal_mt = (4+0) * (1);
static const int H1_nal_mb = (255+0) * (1);
static const int H1_nal_hA = (24+0) * (1);
static const int H1_nal_hB = (24+0) * (1);
static const int H1_nal_hC = (24+0) * (1);
static const int H1_nal_hD = (24+0) * (1);
static const int H1_nal_hM = (24+0) * (1);
static const int H1_nal_hT = (24+0) * (1);
static const int H1_nal_hN = (24+0) * (1);


static const long long H1_nal_a_Volume = tc_vmul(tc_vmul(1, 7), 6); // 42
static const long long H1_nal_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 5), 3), 4); // 60
static const long long H1_nal_c_Volume = tc_vmul(tc_vmul(1, 5), 4); // 20
static const long long H1_nal_d_Volume = tc_vmul(1, 8); // 8
static const long long H1_nal_g_Volume = tc_vmul(tc_vmul(1, 1), 1); // 1
static const long long H1_nal_m_Volume = tc_vmul(1, 4); // 4
static const long long H1_nal_n_Volume = tc_vmul(1, 1); // 1
static const long long H1_nal_t_Volume = tc_vmul(1, 1); // 1

#define H1_nal_USES(X) X(fld) X(ctx) X(p1) X(p2)

struct H1_nal_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H1_nal_A[tc_rows(H1_nal_a_Volume, H1_nal_hA) * H_nal_ND];
  cm_cnt2 H1_nal_B[tc_rows(H1_nal_b_Volume, H1_nal_hB) * H_nal_ND];
  cm_cnt2 H1_nal_C[tc_rows(H1_nal_c_Volume, H1_nal_hC) * H_nal_ND];
  cm_cnt2 H1_nal_D[tc_rows(H1_nal_d_Volume, H1_nal_hD) * H_nal_ND];
  cm_cnt H1_nal_MT[tc_rows(H1_nal_t_Volume, H1_nal_hT) * H_nal_MND];
  i32 H1_nal_W[tc_rows(H1_nal_m_Volume, H1_nal_hM) * H_nal_ND * 16];
  u8 H1_nal_WC[tc_rows(H1_nal_m_Volume, H1_nal_hM) * H_nal_ND];
  i32 H1_nal_WM[tc_rows(H1_nal_n_Volume, H1_nal_hN) * H_nal_MND * 3];
  u8 H1_nal_WMC[tc_rows(H1_nal_n_Volume, H1_nal_hN) * H_nal_MND];
  cm_cnt H1_nal_G[H1_nal_g_Volume * H_nal_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H1_nal_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_nal_a_Volume, H1_nal_hA) * H_nal_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_nal_b_Volume, H1_nal_hB) * H_nal_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_nal_c_Volume, H1_nal_hC) * H_nal_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H1_nal_d_Volume, H1_nal_hD) * H_nal_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H1_nal_t_Volume, H1_nal_hT) * H_nal_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_nal_m_Volume, H1_nal_hM) * H_nal_ND * 16)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_nal_m_Volume, H1_nal_hM) * H_nal_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_nal_n_Volume, H1_nal_hN) * H_nal_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_nal_n_Volume, H1_nal_hN) * H_nal_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H1_nal_g_Volume * H_nal_NG);
  void H1_nal_Init( void ) {
  
  }
};
