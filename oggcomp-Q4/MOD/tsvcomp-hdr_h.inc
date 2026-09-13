
static const byte TC_hdr_a_tag[64+1]={ 0,0,0,0,0,0,0,0,0,0,0,1,2,2,2,2,2,3,3,3,4,5,5,5,5,5,5,6,6,7,7,7,8,9,9,9,9,9,9,9,10,11,11,11,11,11,12,13,13,14,15,15,15,15,15,15,16,16,16,16,16,16,16,16,16 };
static const byte TC_hdr_a_p1[15+1]={ 0,1,1,1,1,1,1,1,1,1,1,2,2,3,4,4 };
static const byte TC_hdr_a_p2[7+1]={ 0,1,2,3,4,5,6,7 };


static const byte TC_hdr_c_tag[64+1]={ 0,0,0,0,0,1,1,2,3,4,5,6,6,7,7,7,8,9,9,10,10,10,11,11,12,13,13,14,14,15,15,15,16,17,18,18,18,18,18,18,18,18,18,19,20,21,21,22,22,23,24,24,25,26,27,28,28,28,28,28,28,28,28,28,28 };

static const byte TC_hdr_d_tag[64+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64 };

static const byte TC_hdr_f_tag[64+1]={ 0,0,1,1,1,1,1,1,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,7 };

static const byte TC_hdr_g_p2[7+1]={ 0,1,1,1,2,3,4,4 };

static const byte TC_hdr_m_tag[64+1]={ 0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 };

static const byte TC_hdr_n_tag[64+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64 };

static const byte TC_hdr_s_tag[64+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64 };

static const byte TC_hdr_t_tag[64+1]={ 0,0,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 };


static const int TC_hdr_rA = (16+0) * (1);
static const int TC_hdr_rB = (0+0) * (1);
static const int TC_hdr_rC = (63+0) * (1);
static const int TC_hdr_rD = (211+0) * (1);
static const int TC_hdr_rT = (16+0) * (1);
static const int TC_hdr_rG = (16+0) * (1);
static const int TC_hdr_rS1 = (1+0) * (1);
static const int TC_hdr_rS2 = (1+0) * (1);
static const int TC_hdr_lr = (31+0) * (1);
static const int TC_hdr_lrm = (31+0) * (1);
static const int TC_hdr_mbm = (0+0) * (1);
static const int TC_hdr_mwA = (4+0) * (1);
static const int TC_hdr_mwB = (64+0) * (1);
static const int TC_hdr_mwC = (0+0) * (1);
static const int TC_hdr_mwD = (0+0) * (1);
static const int TC_hdr_mwT = (42+0) * (1);
static const int TC_hdr_mwG = (0+0) * (1);
static const int TC_hdr_bw = (15+0) * (1);
static const int TC_hdr_qs = (2+3) * (1);
static const int TC_hdr_qf = (2+3) * (1);
static const int TC_hdr_mb = (5+0) * (1);
static const int TC_hdr_hA = (31+0) * (1);
static const int TC_hdr_hB = (31+0) * (1);
static const int TC_hdr_hD = (31+0) * (1);
static const int TC_hdr_hS = (31+0) * (1);
static const int TC_hdr_hF = (31+0) * (1);
static const int TC_hdr_hM = (31+0) * (1);
static const int TC_hdr_hT = (31+0) * (1);
static const int TC_hdr_hN = (31+0) * (1);


static const long long TC_hdr_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 17), 5), 8), 3), (1<<4)); // 32640
static const long long TC_hdr_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 1), (1<<5)); // 96
static const long long TC_hdr_c_Volume = tc_vmul(1, 29); // 29
static const long long TC_hdr_d_Volume = tc_vmul(1, 65); // 65
static const long long TC_hdr_f_Volume = tc_vmul(1, 8); // 8
static const long long TC_hdr_g_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 5), 3), (1<<7)); // 1920
static const long long TC_hdr_m_Volume = tc_vmul(1, 6); // 6
static const long long TC_hdr_n_Volume = tc_vmul(tc_vmul(1, 65), (1<<6)); // 4160
static const long long TC_hdr_s_Volume = tc_vmul(1, 65); // 65
static const long long TC_hdr_t_Volume = tc_vmul(tc_vmul(1, 6), (1<<6)); // 384

struct TC_hdr_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt TC_hdr_A[tc_rows(TC_hdr_a_Volume, TC_hdr_hA) * TC_hdr_ND];
  cm_cnt TC_hdr_B[tc_rows(TC_hdr_b_Volume, TC_hdr_hB) * TC_hdr_ND];
  cm_cnt TC_hdr_C[TC_hdr_c_Volume * TC_hdr_ND];
  cm_cnt TC_hdr_D[tc_rows(TC_hdr_d_Volume, TC_hdr_hD) * TC_hdr_ND];
  cm_cnt TC_hdr_MT[tc_rows(TC_hdr_t_Volume, TC_hdr_hT) * TC_hdr_MND];
  i16 TC_hdr_S[tc_rows(TC_hdr_s_Volume, TC_hdr_hS) * TC_hdr_ND * TC_hdr_qs];
  u8 TC_hdr_SC[tc_rows(TC_hdr_s_Volume, TC_hdr_hS) * TC_hdr_ND * TC_hdr_qs];
  i16 TC_hdr_F[tc_rows(TC_hdr_f_Volume, TC_hdr_hF) * TC_hdr_ND * TC_hdr_qf];
  u8 TC_hdr_FC[tc_rows(TC_hdr_f_Volume, TC_hdr_hF) * TC_hdr_ND * TC_hdr_qf];
  i32 TC_hdr_W[tc_rows(TC_hdr_m_Volume, TC_hdr_hM) * TC_hdr_ND * TC_MIXN];
  u8 TC_hdr_WC[tc_rows(TC_hdr_m_Volume, TC_hdr_hM) * TC_hdr_ND];
  i32 TC_hdr_WM[tc_rows(TC_hdr_n_Volume, TC_hdr_hN) * TC_hdr_MND * 3];
  u8 TC_hdr_WMC[tc_rows(TC_hdr_n_Volume, TC_hdr_hN) * TC_hdr_MND];
  cm_cnt TC_hdr_G[TC_hdr_g_Volume * TC_SGN_HDR];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long TC_hdr_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(TC_hdr_a_Volume, TC_hdr_hA) * TC_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(TC_hdr_b_Volume, TC_hdr_hB) * TC_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_hdr_c_Volume * TC_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(TC_hdr_d_Volume, TC_hdr_hD) * TC_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(TC_hdr_t_Volume, TC_hdr_hT) * TC_hdr_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(TC_hdr_s_Volume, TC_hdr_hS) * TC_hdr_ND * TC_hdr_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(TC_hdr_s_Volume, TC_hdr_hS) * TC_hdr_ND * TC_hdr_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(TC_hdr_f_Volume, TC_hdr_hF) * TC_hdr_ND * TC_hdr_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(TC_hdr_f_Volume, TC_hdr_hF) * TC_hdr_ND * TC_hdr_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(TC_hdr_m_Volume, TC_hdr_hM) * TC_hdr_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(TC_hdr_m_Volume, TC_hdr_hM) * TC_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(TC_hdr_n_Volume, TC_hdr_hN) * TC_hdr_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(TC_hdr_n_Volume, TC_hdr_hN) * TC_hdr_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_hdr_g_Volume * TC_SGN_HDR);
  void TC_hdr_Init( void ) {
  
  }
};
