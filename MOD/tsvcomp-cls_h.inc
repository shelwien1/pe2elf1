











static const int TC_cls_rA = (80+0) * (1);
static const int TC_cls_rB = (43+0) * (1);
static const int TC_cls_rC = (63+0) * (1);
static const int TC_cls_rD = (63+0) * (1);
static const int TC_cls_rT = (80+0) * (1);
static const int TC_cls_rS1 = (5+0) * (1);
static const int TC_cls_rS2 = (5+0) * (1);
static const int TC_cls_lr = (24+0) * (1);
static const int TC_cls_lrm = (24+0) * (1);
static const int TC_cls_mbm = (0+0) * (1);
static const int TC_cls_mwA = (0+0) * (1);
static const int TC_cls_mwB = (0+0) * (1);
static const int TC_cls_mwC = (0+0) * (1);
static const int TC_cls_mwD = (0+0) * (1);
static const int TC_cls_mwT = (0+0) * (1);
static const int TC_cls_bw = (12+0) * (1);
static const int TC_cls_qs = (30+3) * (1);
static const int TC_cls_qf = (62+3) * (1);
static const int TC_cls_mb = (0+0) * (1);


static const long long TC_cls_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 8), 5), 5), 2), 1), 1), 1); // 800
static const long long TC_cls_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 8), 6), 4), 2), 1), 1), 1); // 384
static const long long TC_cls_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 4), 4), 4), 1), 1); // 64
static const long long TC_cls_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 4), 1), 1), 4), 1); // 64
static const long long TC_cls_f_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 1), 1), 1), 1), 1); // 1
static const long long TC_cls_g_Volume = 1; // 1
static const long long TC_cls_m_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 3), 2), 1), 1), 1); // 12
static const long long TC_cls_n_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 3), 1), 1), 1); // 18
static const long long TC_cls_s_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 1), 1), 1), 1), 1); // 1
static const long long TC_cls_t_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 3), 1), 1), 1); // 18

struct TC_cls_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt TC_cls_A[TC_cls_a_Volume * TC_cls_ND];
  cm_cnt TC_cls_B[TC_cls_b_Volume * TC_cls_ND];
  cm_cnt TC_cls_C[TC_cls_c_Volume * TC_cls_ND];
  cm_cnt TC_cls_D[TC_cls_d_Volume * TC_cls_ND];
  cm_cnt TC_cls_MT[TC_cls_t_Volume * TC_cls_MND];
  i16 TC_cls_S[TC_cls_s_Volume * TC_cls_ND * TC_cls_qs];
  u8 TC_cls_SC[TC_cls_s_Volume * TC_cls_ND * TC_cls_qs];
  i16 TC_cls_F[TC_cls_f_Volume * TC_cls_ND * TC_cls_qf];
  u8 TC_cls_FC[TC_cls_f_Volume * TC_cls_ND * TC_cls_qf];
  i32 TC_cls_W[TC_cls_m_Volume * TC_cls_ND * TC_MIXN];
  u8 TC_cls_WC[TC_cls_m_Volume * TC_cls_ND];
  i32 TC_cls_WM[TC_cls_n_Volume * TC_cls_MND * 3];
  u8 TC_cls_WMC[TC_cls_n_Volume * TC_cls_MND];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long TC_cls_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_cls_a_Volume * TC_cls_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_cls_b_Volume * TC_cls_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_cls_c_Volume * TC_cls_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_cls_d_Volume * TC_cls_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_cls_t_Volume * TC_cls_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_cls_s_Volume * TC_cls_ND * TC_cls_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_cls_s_Volume * TC_cls_ND * TC_cls_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_cls_f_Volume * TC_cls_ND * TC_cls_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_cls_f_Volume * TC_cls_ND * TC_cls_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_cls_m_Volume * TC_cls_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_cls_m_Volume * TC_cls_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_cls_n_Volume * TC_cls_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_cls_n_Volume * TC_cls_MND);
  void TC_cls_Init( void ) {
  
  }
  void TC_cls_Quit( void ) {
  }
};
