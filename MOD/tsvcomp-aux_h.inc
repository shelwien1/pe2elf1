
static const byte TC_aux_a_fld[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };

static const byte TC_aux_b_fld[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };

static const byte TC_aux_c_fld[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };

static const byte TC_aux_d_fld[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };

static const byte TC_aux_f_fld[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };

static const byte TC_aux_g_p1[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

static const byte TC_aux_m_fld[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };

static const byte TC_aux_n_fld[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };

static const byte TC_aux_s_fld[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };

static const byte TC_aux_t_fld[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13 };


static const int TC_aux_rA = (22+0) * (1);
static const int TC_aux_rB = (0+0) * (1);
static const int TC_aux_rC = (63+0) * (1);
static const int TC_aux_rD = (63+0) * (1);
static const int TC_aux_rT = (22+0) * (1);
static const int TC_aux_rG = (22+0) * (1);
static const int TC_aux_rS1 = (6+0) * (1);
static const int TC_aux_rS2 = (6+0) * (1);
static const int TC_aux_lr = (31+0) * (1);
static const int TC_aux_lrm = (31+0) * (1);
static const int TC_aux_mbm = (0+0) * (1);
static const int TC_aux_mwA = (0+0) * (1);
static const int TC_aux_mwB = (0+0) * (1);
static const int TC_aux_mwC = (0+0) * (1);
static const int TC_aux_mwD = (0+0) * (1);
static const int TC_aux_mwT = (0+0) * (1);
static const int TC_aux_mwG = (0+0) * (1);
static const int TC_aux_bw = (12+0) * (1);
static const int TC_aux_qs = (30+3) * (1);
static const int TC_aux_qf = (30+3) * (1);
static const int TC_aux_mb = (0+0) * (1);


static const long long TC_aux_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 14), 6), 3); // 252
static const long long TC_aux_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 14), 1), 1); // 14
static const long long TC_aux_c_Volume = tc_vmul(1, 14); // 14
static const long long TC_aux_d_Volume = tc_vmul(1, 14); // 14
static const long long TC_aux_f_Volume = tc_vmul(1, 14); // 14
static const long long TC_aux_g_Volume = tc_vmul(tc_vmul(1, 16), 1); // 16
static const long long TC_aux_m_Volume = tc_vmul(1, 14); // 14
static const long long TC_aux_n_Volume = tc_vmul(1, 14); // 14
static const long long TC_aux_s_Volume = tc_vmul(1, 14); // 14
static const long long TC_aux_t_Volume = tc_vmul(1, 14); // 14

struct TC_aux_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt TC_aux_A[TC_aux_a_Volume * TC_NODE];
  cm_cnt TC_aux_B[TC_aux_b_Volume * TC_NODE];
  cm_cnt TC_aux_C[TC_aux_c_Volume * TC_NODE];
  cm_cnt TC_aux_D[TC_aux_d_Volume * TC_NODE];
  cm_cnt TC_aux_MT[TC_aux_t_Volume * TC_MNODE];
  i16 TC_aux_S[TC_aux_s_Volume * TC_NODE * TC_aux_qs];
  u8 TC_aux_SC[TC_aux_s_Volume * TC_NODE * TC_aux_qs];
  i16 TC_aux_F[TC_aux_f_Volume * TC_NODE * TC_aux_qf];
  u8 TC_aux_FC[TC_aux_f_Volume * TC_NODE * TC_aux_qf];
  i32 TC_aux_W[TC_aux_m_Volume * TC_NODE * 7];
  u8 TC_aux_WC[TC_aux_m_Volume * TC_NODE];
  i32 TC_aux_WM[TC_aux_n_Volume * TC_MNODE * 3];
  u8 TC_aux_WMC[TC_aux_n_Volume * TC_MNODE];
  cm_cnt TC_aux_G[TC_aux_g_Volume * TC_SGN_AUX];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long TC_aux_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_aux_a_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_aux_b_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_aux_c_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_aux_d_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_aux_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_aux_s_Volume * TC_NODE * TC_aux_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_aux_s_Volume * TC_NODE * TC_aux_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_aux_f_Volume * TC_NODE * TC_aux_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_aux_f_Volume * TC_NODE * TC_aux_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_aux_m_Volume * TC_NODE * 7)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_aux_m_Volume * TC_NODE)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_aux_n_Volume * TC_MNODE * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_aux_n_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_aux_g_Volume * TC_SGN_AUX);
  void TC_aux_Init( void ) {
  
  }
  void TC_aux_Quit( void ) {
  }
};
