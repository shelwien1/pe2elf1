
static const byte H1_nal_a_fld[31+1]={ 0,0,1,2,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };
static const byte H1_nal_a_ctx[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

static const byte H1_nal_b_fld[31+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31 };
static const byte H1_nal_b_p1[6+1]={ 0,1,2,3,4,5,6 };
static const byte H1_nal_b_p2[3+1]={ 0,1,2,3 };

static const byte H1_nal_c_fld[31+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31 };

static const byte H1_nal_d_fld[31+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31 };

static const byte H1_nal_g_fld[31+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31 };
static const byte H1_nal_g_ctx[3+1]={ 0,1,2,3 };

static const byte H1_nal_m_fld[31+1]={ 0,0,0,0,1,1,1,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };

static const byte H1_nal_n_fld[31+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31 };

static const byte H1_nal_t_fld[31+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31 };


static const int H1_nal_rA = (192+0) * (1);
static const int H1_nal_rB = (0+0) * (1);
static const int H1_nal_rC = (32+0) * (1);
static const int H1_nal_rD = (96+0) * (1);
static const int H1_nal_rT = (55+0) * (1);
static const int H1_nal_rG = (90+0) * (1);
static const int H1_nal_lr = (30+0) * (1);
static const int H1_nal_lrm = (12+0) * (1);
static const int H1_nal_mbm = (0+0) * (1);
static const int H1_nal_mwA = (128+0) * (1);
static const int H1_nal_mwB = (0+0) * (1);
static const int H1_nal_mwC = (0+0) * (1);
static const int H1_nal_mwD = (4+0) * (1);
static const int H1_nal_mwT = (0+0) * (1);
static const int H1_nal_mwG = (0+0) * (1);
static const int H1_nal_mt = (6+0) * (1);
static const int H1_nal_mb = (5+0) * (1);
static const int H1_nal_hA = (24+0) * (1);
static const int H1_nal_hB = (24+0) * (1);
static const int H1_nal_hC = (24+0) * (1);
static const int H1_nal_hD = (24+0) * (1);
static const int H1_nal_hM = (24+0) * (1);
static const int H1_nal_hT = (24+0) * (1);
static const int H1_nal_hN = (24+0) * (1);


static const long long H1_nal_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 5), 16), 1); // 80
static const long long H1_nal_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 32), 7), 4); // 896
static const long long H1_nal_c_Volume = tc_vmul(tc_vmul(1, 32), 3); // 96
static const long long H1_nal_d_Volume = tc_vmul(1, 32); // 32
static const long long H1_nal_g_Volume = tc_vmul(tc_vmul(1, 32), 4); // 128
static const long long H1_nal_m_Volume = tc_vmul(1, 4); // 4
static const long long H1_nal_n_Volume = tc_vmul(1, 32); // 32
static const long long H1_nal_t_Volume = tc_vmul(1, 32); // 32

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
