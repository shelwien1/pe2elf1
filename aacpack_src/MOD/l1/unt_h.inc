
static const byte H1_unt_a_fld[15+1]={ 0,1,2,2,2,3,3,3,3,3,3,3,3,3,3,3 };


static const byte H1_unt_c_fld[15+1]={ 0,1,2,2,3,3,3,3,3,3,3,3,3,3,3,3 };

static const byte H1_unt_d_ctx[7+1]={ 0,1,2,3,4,5,6,7 };


static const byte H1_unt_m_fld[15+1]={ 0,1,2,2,3,3,3,3,3,3,3,3,3,3,3,3 };

static const byte H1_unt_t_fld[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };


static const int H1_unt_rA = (0+0) * (1);
static const int H1_unt_rB = (192+0) * (1);
static const int H1_unt_rC = (0+0) * (1);
static const int H1_unt_rD = (0+0) * (1);
static const int H1_unt_rT = (128+0) * (1);
static const int H1_unt_rG = (64+0) * (1);
static const int H1_unt_lr = (31+0) * (1);
static const int H1_unt_mwA = (0+0) * (1);
static const int H1_unt_mwB = (0+0) * (1);
static const int H1_unt_mwC = (0+0) * (1);
static const int H1_unt_mwD = (0+0) * (1);
static const int H1_unt_mwT = (255+0) * (1);
static const int H1_unt_mwG = (0+0) * (1);
static const int H1_unt_mt = (2+0) * (1);
static const int H1_unt_mb = (255+0) * (1);
static const int H1_unt_hA = (24+0) * (1);
static const int H1_unt_hB = (24+0) * (1);
static const int H1_unt_hC = (24+0) * (1);
static const int H1_unt_hD = (24+0) * (1);
static const int H1_unt_hM = (24+0) * (1);
static const int H1_unt_hT = (24+0) * (1);


static const long long H1_unt_a_Volume = tc_vmul(1, 4); // 4
static const long long H1_unt_b_Volume = tc_vmul(tc_vmul(1, 3), 1); // 3
static const long long H1_unt_c_Volume = tc_vmul(tc_vmul(1, 4), 1); // 4
static const long long H1_unt_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 8), 3); // 72
static const long long H1_unt_g_Volume = 1; // 1
static const long long H1_unt_m_Volume = tc_vmul(1, 4); // 4
static const long long H1_unt_t_Volume = tc_vmul(1, 16); // 16

#define H1_unt_USES(X) X(fld) X(ctx) X(p1)

struct H1_unt_T {
  cm_cnt2s H1_unt_A[tc_rows(H1_unt_a_Volume, H1_unt_hA) * H_unt_ND];
  cm_cnt2s H1_unt_B[tc_rows(H1_unt_b_Volume, H1_unt_hB) * H_unt_ND];
  cm_cnt2s H1_unt_C[tc_rows(H1_unt_c_Volume, H1_unt_hC) * H_unt_ND];
  cm_cnt2s H1_unt_D[tc_rows(H1_unt_d_Volume, H1_unt_hD) * H_unt_ND];
  cm_cnt H1_unt_MT[tc_rows(H1_unt_t_Volume, H1_unt_hT) * H_unt_MND];
  i32 H1_unt_W[tc_rows(H1_unt_m_Volume, H1_unt_hM) * H_unt_ND * tc_nin(4)];
  u8 H1_unt_WC[tc_rows(H1_unt_m_Volume, H1_unt_hM) * H_unt_ND];
  cm_cnt H1_unt_G[H1_unt_g_Volume * H_unt_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H1_unt_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_unt_a_Volume, H1_unt_hA) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_unt_b_Volume, H1_unt_hB) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_unt_c_Volume, H1_unt_hC) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_unt_d_Volume, H1_unt_hD) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H1_unt_t_Volume, H1_unt_hT) * H_unt_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_unt_m_Volume, H1_unt_hM) * H_unt_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_unt_m_Volume, H1_unt_hM) * H_unt_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H1_unt_g_Volume * H_unt_NG);
  void H1_unt_Init( void ) {}
  void H1_unt_Quit( void ) {}
};
