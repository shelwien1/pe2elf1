



static const byte A1_hdr_d_pos[31+1]={ 0,1,2,2,2,2,3,4,5,6,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8 };






static const int A1_hdr_rA = (0+0) * (1);
static const int A1_hdr_mwA = (12+0) * (1);
static const int A1_hdr_rB = (255+0) * (1);
static const int A1_hdr_mwB = (20+0) * (1);
static const int A1_hdr_rC = (254+0) * (1);
static const int A1_hdr_mwC = (8+0) * (1);
static const int A1_hdr_rD = (255+0) * (1);
static const int A1_hdr_mwD = (32+0) * (1);
static const int A1_hdr_lr = (14+0) * (1);
static const int A1_hdr_mb = (129+0) * (1);
static const int A1_hdr_rT = (128+0) * (1);
static const int A1_hdr_mwT = (31+0) * (1);
static const int A1_hdr_mt = (6+0) * (1);
static const int A1_hdr_lrm = (31+0) * (1);
static const int A1_hdr_mbm = (25+0) * (1);
static const int A1_hdr_rG = (255+0) * (1);
static const int A1_hdr_mwG = (96+0) * (1);
static const int A1_hdr_hA = (22+0) * (1);
static const int A1_hdr_hB = (22+0) * (1);
static const int A1_hdr_hC = (22+0) * (1);
static const int A1_hdr_hD = (22+0) * (1);
static const int A1_hdr_hM = (22+0) * (1);
static const int A1_hdr_hT = (22+0) * (1);
static const int A1_hdr_hN = (22+0) * (1);


static const long long A1_hdr_a_Volume = tc_vmul(1, (1<<1)); // 2
static const long long A1_hdr_b_Volume = tc_vmul(tc_vmul(1, (1<<7)), (1<<4)); // 2048
static const long long A1_hdr_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<2)), (1<<4)), 1); // 64
static const long long A1_hdr_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<7)), (1<<5)), 9), 1); // 36864
static const long long A1_hdr_g_Volume = tc_vmul(1, (1<<7)); // 128
static const long long A1_hdr_m_Volume = tc_vmul(1, (1<<5)); // 32
static const long long A1_hdr_n_Volume = tc_vmul(1, (1<<7)); // 128
static const long long A1_hdr_t_Volume = tc_vmul(1, (1<<4)); // 16

#define A1_hdr_USES(X) X(fld) X(lv) X(pos)

struct A1_hdr_T {
  cm_cnt2s A1_hdr_A[tc_rows(A1_hdr_a_Volume, A1_hdr_hA) * M_hdr_ND];
  cm_cnt2s A1_hdr_B[tc_rows(A1_hdr_b_Volume, A1_hdr_hB) * M_hdr_ND];
  cm_cnt2s A1_hdr_C[tc_rows(A1_hdr_c_Volume, A1_hdr_hC) * M_hdr_ND];
  cm_cnt2s A1_hdr_D[tc_rows(A1_hdr_d_Volume, A1_hdr_hD) * M_hdr_ND];
  cm_cnt A1_hdr_MT[tc_rows(A1_hdr_t_Volume, A1_hdr_hT) * M_hdr_MND];
  i32 A1_hdr_W[tc_rows(A1_hdr_m_Volume, A1_hdr_hM) * M_hdr_ND * tc_nin(4)];
  u8 A1_hdr_WC[tc_rows(A1_hdr_m_Volume, A1_hdr_hM) * M_hdr_ND];
  i32 A1_hdr_WM[tc_rows(A1_hdr_n_Volume, A1_hdr_hN) * M_hdr_MND * TC_MIN];
  u8 A1_hdr_WMC[tc_rows(A1_hdr_n_Volume, A1_hdr_hN) * M_hdr_MND];
  cm_cnt A1_hdr_G[A1_hdr_g_Volume * M_hdr_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long A1_hdr_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_hdr_a_Volume, A1_hdr_hA) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_hdr_b_Volume, A1_hdr_hB) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_hdr_c_Volume, A1_hdr_hC) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_hdr_d_Volume, A1_hdr_hD) * M_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(A1_hdr_t_Volume, A1_hdr_hT) * M_hdr_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(A1_hdr_m_Volume, A1_hdr_hM) * M_hdr_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(A1_hdr_m_Volume, A1_hdr_hM) * M_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(A1_hdr_n_Volume, A1_hdr_hN) * M_hdr_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(A1_hdr_n_Volume, A1_hdr_hN) * M_hdr_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(A1_hdr_g_Volume * M_hdr_NG);
  void A1_hdr_Init( void ) {}
  void A1_hdr_Quit( void ) {}
};
