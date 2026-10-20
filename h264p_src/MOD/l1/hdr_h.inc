
static const byte H1_hdr_a_fld[3+1]={ 0,1,2,3 };
static const byte H1_hdr_a_p1[7+1]={ 0,1,2,2,2,3,4,4 };
static const byte H1_hdr_a_sty[9+1]={ 0,0,1,2,3,3,3,3,3,3 };


static const byte H1_hdr_c_fld[15+1]={ 0,1,2,3,4,5,6,7,8,8,8,8,8,8,8,9 };

static const byte H1_hdr_d_p1[15+1]={ 0,1,2,3,4,5,5,6,7,7,7,8,8,8,8,8 };
static const byte H1_hdr_d_p2[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H1_hdr_g_p1[7+1]={ 0,1,2,3,4,5,6,6 };





static const int H1_hdr_rA = (197+0) * (1);
static const int H1_hdr_rB = (0+0) * (1);
static const int H1_hdr_rC = (0+0) * (1);
static const int H1_hdr_rD = (236+0) * (1);
static const int H1_hdr_rT = (4+0) * (1);
static const int H1_hdr_rG = (64+0) * (1);
static const int H1_hdr_lr = (31+0) * (1);
static const int H1_hdr_lrm = (0+0) * (1);
static const int H1_hdr_mbm = (0+0) * (1);
static const int H1_hdr_mwA = (0+0) * (1);
static const int H1_hdr_mwB = (7+0) * (1);
static const int H1_hdr_mwC = (4+0) * (1);
static const int H1_hdr_mwD = (4+0) * (1);
static const int H1_hdr_mwT = (8+0) * (1);
static const int H1_hdr_mwG = (0+0) * (1);
static const int H1_hdr_mt = (0+0) * (1);
static const int H1_hdr_mb = (100+0) * (1);
static const int H1_hdr_hA = (24+0) * (1);
static const int H1_hdr_hB = (24+0) * (1);
static const int H1_hdr_hC = (24+0) * (1);
static const int H1_hdr_hD = (24+0) * (1);
static const int H1_hdr_hM = (24+0) * (1);
static const int H1_hdr_hT = (24+0) * (1);
static const int H1_hdr_hN = (24+0) * (1);


static const long long H1_hdr_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), 4), 5), 4), 3); // 15360
static const long long H1_hdr_b_Volume = tc_vmul(tc_vmul(1, (1<<5)), (1<<7)); // 4096
static const long long H1_hdr_c_Volume = tc_vmul(tc_vmul(1, (1<<5)), 10); // 320
static const long long H1_hdr_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<7)), 9), 8); // 9216
static const long long H1_hdr_g_Volume = tc_vmul(tc_vmul(1, 7), 1); // 7
static const long long H1_hdr_m_Volume = 1; // 1
static const long long H1_hdr_n_Volume = 1; // 1
static const long long H1_hdr_t_Volume = tc_vmul(1, (1<<4)); // 16

#define H1_hdr_USES(X) X(tag) X(fld) X(p1) X(sty) X(pr) X(p1r) X(p2)

struct H1_hdr_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2s H1_hdr_A[tc_rows(H1_hdr_a_Volume, H1_hdr_hA) * H_hdr_ND];
  cm_cnt2s H1_hdr_B[tc_rows(H1_hdr_b_Volume, H1_hdr_hB) * H_hdr_ND];
  cm_cnt2s H1_hdr_C[tc_rows(H1_hdr_c_Volume, H1_hdr_hC) * H_hdr_ND];
  cm_cnt2s H1_hdr_D[tc_rows(H1_hdr_d_Volume, H1_hdr_hD) * H_hdr_ND];
  cm_cnt H1_hdr_MT[tc_rows(H1_hdr_t_Volume, H1_hdr_hT) * H_hdr_MND];
  i32 H1_hdr_W[tc_rows(H1_hdr_m_Volume, H1_hdr_hM) * H_hdr_ND * 16];
  u8 H1_hdr_WC[tc_rows(H1_hdr_m_Volume, H1_hdr_hM) * H_hdr_ND];
  i32 H1_hdr_WM[tc_rows(H1_hdr_n_Volume, H1_hdr_hN) * H_hdr_MND * 3];
  u8 H1_hdr_WMC[tc_rows(H1_hdr_n_Volume, H1_hdr_hN) * H_hdr_MND];
  cm_cnt H1_hdr_G[H1_hdr_g_Volume * H_hdr_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H1_hdr_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_hdr_a_Volume, H1_hdr_hA) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_hdr_b_Volume, H1_hdr_hB) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_hdr_c_Volume, H1_hdr_hC) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_hdr_d_Volume, H1_hdr_hD) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H1_hdr_t_Volume, H1_hdr_hT) * H_hdr_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_hdr_m_Volume, H1_hdr_hM) * H_hdr_ND * 16)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_hdr_m_Volume, H1_hdr_hM) * H_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_hdr_n_Volume, H1_hdr_hN) * H_hdr_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_hdr_n_Volume, H1_hdr_hN) * H_hdr_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H1_hdr_g_Volume * H_hdr_NG);
  void H1_hdr_Init( void ) {
  
  }
};
