
static const byte TC_hdr_a_tag[64+1]={ 0,1,2,3,4,4,5,6,7,8,9,10,11,12,13,14,14,15,16,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61 };


static const byte TC_hdr_c_tag[64+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64 };

static const byte TC_hdr_d_tag[64+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64 };

static const byte TC_hdr_f_tag[64+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64 };


static const byte TC_hdr_m_tag[64+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64 };

static const byte TC_hdr_n_tag[64+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64 };

static const byte TC_hdr_s_tag[64+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64 };

static const byte TC_hdr_t_tag[64+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64 };


static const int TC_hdr_rA = (16+0) * (1);
static const int TC_hdr_rB = (0+0) * (1);
static const int TC_hdr_rC = (63+0) * (1);
static const int TC_hdr_rD = (63+0) * (1);
static const int TC_hdr_rT = (16+0) * (1);
static const int TC_hdr_rG = (16+0) * (1);
static const int TC_hdr_rS1 = (1+0) * (1);
static const int TC_hdr_rS2 = (1+0) * (1);
static const int TC_hdr_lr = (31+0) * (1);
static const int TC_hdr_lrm = (31+0) * (1);
static const int TC_hdr_mbm = (0+0) * (1);
static const int TC_hdr_mwA = (0+0) * (1);
static const int TC_hdr_mwB = (0+0) * (1);
static const int TC_hdr_mwC = (0+0) * (1);
static const int TC_hdr_mwD = (0+0) * (1);
static const int TC_hdr_mwT = (0+0) * (1);
static const int TC_hdr_mwG = (0+0) * (1);
static const int TC_hdr_bw = (12+0) * (1);
static const int TC_hdr_qs = (2+3) * (1);
static const int TC_hdr_qf = (2+3) * (1);
static const int TC_hdr_mb = (0+0) * (1);


static const long long TC_hdr_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 62), 5), 8), 3); // 7440
static const long long TC_hdr_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 1), (1<<5)); // 96
static const long long TC_hdr_c_Volume = tc_vmul(1, 65); // 65
static const long long TC_hdr_d_Volume = tc_vmul(1, 65); // 65
static const long long TC_hdr_f_Volume = tc_vmul(1, 65); // 65
static const long long TC_hdr_g_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 1), 1); // 1
static const long long TC_hdr_m_Volume = tc_vmul(1, 65); // 65
static const long long TC_hdr_n_Volume = tc_vmul(tc_vmul(1, 65), (1<<6)); // 4160
static const long long TC_hdr_s_Volume = tc_vmul(1, 65); // 65
static const long long TC_hdr_t_Volume = tc_vmul(tc_vmul(1, 65), (1<<6)); // 4160

struct TC_hdr_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt TC_hdr_A[TC_hdr_a_Volume * TC_NODE];
  cm_cnt TC_hdr_B[TC_hdr_b_Volume * TC_NODE];
  cm_cnt TC_hdr_C[TC_hdr_c_Volume * TC_NODE];
  cm_cnt TC_hdr_D[TC_hdr_d_Volume * TC_NODE];
  cm_cnt TC_hdr_MT[TC_hdr_t_Volume * TC_MNODE];
  i16 TC_hdr_S[TC_hdr_s_Volume * TC_NODE * TC_hdr_qs];
  u8 TC_hdr_SC[TC_hdr_s_Volume * TC_NODE * TC_hdr_qs];
  i16 TC_hdr_F[TC_hdr_f_Volume * TC_NODE * TC_hdr_qf];
  u8 TC_hdr_FC[TC_hdr_f_Volume * TC_NODE * TC_hdr_qf];
  i32 TC_hdr_W[TC_hdr_m_Volume * TC_NODE * 7];
  u8 TC_hdr_WC[TC_hdr_m_Volume * TC_NODE];
  i32 TC_hdr_WM[TC_hdr_n_Volume * TC_MNODE * 3];
  u8 TC_hdr_WMC[TC_hdr_n_Volume * TC_MNODE];
  cm_cnt TC_hdr_G[TC_hdr_g_Volume * TC_SGN_HDR];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long TC_hdr_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_hdr_a_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_hdr_b_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_hdr_c_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_hdr_d_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_hdr_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_hdr_s_Volume * TC_NODE * TC_hdr_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_hdr_s_Volume * TC_NODE * TC_hdr_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_hdr_f_Volume * TC_NODE * TC_hdr_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_hdr_f_Volume * TC_NODE * TC_hdr_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_hdr_m_Volume * TC_NODE * 7)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_hdr_m_Volume * TC_NODE)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_hdr_n_Volume * TC_MNODE * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_hdr_n_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_hdr_g_Volume * TC_SGN_HDR);
  void TC_hdr_Init( void ) {
  
  }
  void TC_hdr_Quit( void ) {
  }
};
