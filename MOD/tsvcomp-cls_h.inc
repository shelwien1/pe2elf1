
static const byte TC_cls_a_band[15+1]={ 0,0,0,1,2,3,4,5,6,7,8,8,8,8,8,8 };
static const byte TC_cls_a_t1[7+1]={ 0,1,1,1,2,3,4,5 };
static const byte TC_cls_a_pq[32+1]={ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,2,3,4,4,4,4,4 };

static const byte TC_cls_b_band[15+1]={ 0,1,1,1,2,2,3,4,5,6,7,7,7,7,7,7 };
static const byte TC_cls_b_prev[15+1]={ 0,0,0,0,1,2,3,3,4,4,4,5,5,5,6,6 };
static const byte TC_cls_b_t1[3+1]={ 0,1,2,3 };
static const byte TC_cls_b_pq[32+1]={ 0,0,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,4,5,5,5,5,5,5,5,5,5,5,5 };

static const byte TC_cls_c_t1[3+1]={ 0,1,2,3 };

static const byte TC_cls_d_prev[3+1]={ 0,1,2,3 };
static const byte TC_cls_d_prev2[3+1]={ 0,1,2,3 };
static const byte TC_cls_d_pq[32+1]={ 0,0,0,0,0,0,1,1,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,5 };




static const byte TC_cls_n_band[3+1]={ 0,1,2,3 };
static const byte TC_cls_n_pq[32+1]={ 0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,3,3,4,4,4,4,4,4,4,4 };

static const byte TC_cls_s_band[3+1]={ 0,1,2,3 };

static const byte TC_cls_t_pq[32+1]={ 0,0,0,0,0,0,1,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,4,4,4,4,4,4 };


static const int TC_cls_rA = (29+0) * (1);
static const int TC_cls_rB = (43+0) * (1);
static const int TC_cls_rC = (34+0) * (1);
static const int TC_cls_rD = (5+0) * (1);
static const int TC_cls_rT = (32+0) * (1);
static const int TC_cls_rS1 = (5+0) * (1);
static const int TC_cls_rS2 = (5+0) * (1);
static const int TC_cls_lr = (18+0) * (1);
static const int TC_cls_lrm = (31+0) * (1);
static const int TC_cls_mbm = (192+0) * (1);
static const int TC_cls_mwA = (0+0) * (1);
static const int TC_cls_mwB = (0+0) * (1);
static const int TC_cls_mwC = (32+0) * (1);
static const int TC_cls_mwD = (36+0) * (1);
static const int TC_cls_mwT = (3+0) * (1);
static const int TC_cls_bw = (16+0) * (1);
static const int TC_cls_qs = (15+3) * (1);
static const int TC_cls_qf = (62+3) * (1);
static const int TC_cls_mb = (32+0) * (1);


static const long long TC_cls_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 9), 1), 6), 2), 1), 1), 5); // 1080
static const long long TC_cls_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 8), 7), 4), 2), 1), 2), 6); // 5376
static const long long TC_cls_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 2), 4), 2), 1), 3); // 48
static const long long TC_cls_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 4), 1), 1), 4), 6); // 288
static const long long TC_cls_f_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 1), 1), 1), 1), 1); // 1
static const long long TC_cls_g_Volume = 1; // 1
static const long long TC_cls_m_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 2), 3), 2), 1), 1), 1); // 12
static const long long TC_cls_n_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 3), 1), 1), 5); // 60
static const long long TC_cls_s_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 1), 1), 2), 3), 1); // 24
static const long long TC_cls_t_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 2), 3), 3), 3), 5); // 810

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
};
