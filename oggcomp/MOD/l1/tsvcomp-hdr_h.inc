
static const byte L1_hdr_a_tag[64+1]={ 0,0,0,1,1,2,2,2,2,2,2,2,3,4,4,4,4,5,5,5,5,5,5,5,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8,8,8,9,9,9,9,9,9,10,10,10,10,10,10,10,10,10,10,10,10,10 };
static const byte L1_hdr_a_p1[15+1]={ 0,1,1,1,1,1,1,1,1,1,1,2,2,3,4,4 };
static const byte L1_hdr_a_p2[7+1]={ 0,1,2,3,4,5,6,7 };


static const byte L1_hdr_c_tag[64+1]={ 0,0,0,0,0,1,1,2,2,3,3,4,5,5,5,5,6,7,7,8,8,8,8,8,8,9,9,9,9,9,9,9,9,10,11,11,11,11,11,11,12,12,12,13,13,13,13,13,13,13,13,13,13,14,14,15,15,15,15,15,16,17,17,17,17 };

static const byte L1_hdr_d_tag[64+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64 };


static const byte L1_hdr_g_p2[7+1]={ 0,0,0,0,0,1,2,3 };

static const byte L1_hdr_m_tag[64+1]={ 0,0,0,0,0,0,0,0,0,0,0,0,1,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4 };

static const byte L1_hdr_n_tag[64+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64 };

static const byte L1_hdr_s_tag[64+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64 };

static const byte L1_hdr_t_tag[64+1]={ 0,0,0,1,1,1,1,1,1,1,1,1,2,3,3,3,3,3,3,4,4,4,4,4,5,5,6,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9 };


static const int L1_hdr_rA = (5+0) * (1);
static const int L1_hdr_rB = (0+0) * (1);
static const int L1_hdr_rC = (0+0) * (1);
static const int L1_hdr_rD = (211+0) * (1);
static const int L1_hdr_rT = (16+0) * (1);
static const int L1_hdr_rG = (2+0) * (1);
static const int L1_hdr_rS1 = (5+0) * (1);
static const int L1_hdr_rS2 = (2+0) * (1);
static const int L1_hdr_lr = (31+0) * (1);
static const int L1_hdr_lrm = (31+0) * (1);
static const int L1_hdr_mbm = (0+0) * (1);
static const int L1_hdr_mwA = (8+0) * (1);
static const int L1_hdr_mwB = (64+0) * (1);
static const int L1_hdr_mwC = (33+0) * (1);
static const int L1_hdr_mwD = (0+0) * (1);
static const int L1_hdr_mwT = (42+0) * (1);
static const int L1_hdr_mwG = (32+0) * (1);
static const int L1_hdr_bw = (14+0) * (1);
static const int L1_hdr_qs = (35+3) * (1);
static const int L1_hdr_qf = (2+3) * (1);
static const int L1_hdr_mb = (36+0) * (1);
static const int L1_hdr_hA = (31+0) * (1);
static const int L1_hdr_hB = (31+0) * (1);
static const int L1_hdr_hD = (31+0) * (1);
static const int L1_hdr_hS = (31+0) * (1);
static const int L1_hdr_hF = (31+0) * (1);
static const int L1_hdr_hM = (31+0) * (1);
static const int L1_hdr_hT = (31+0) * (1);
static const int L1_hdr_hN = (31+0) * (1);


static const long long L1_hdr_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 11), 5), 8), 3), (1<<5)); // 42240
static const long long L1_hdr_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 3), 1), (1<<5)); // 96
static const long long L1_hdr_c_Volume = tc_vmul(1, 18); // 18
static const long long L1_hdr_d_Volume = tc_vmul(1, 65); // 65
static const long long L1_hdr_f_Volume = tc_vmul(1, 3); // 3
static const long long L1_hdr_g_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 4), 3), (1<<6)); // 768
static const long long L1_hdr_m_Volume = tc_vmul(1, 5); // 5
static const long long L1_hdr_n_Volume = tc_vmul(tc_vmul(1, 65), (1<<6)); // 4160
static const long long L1_hdr_s_Volume = tc_vmul(1, 65); // 65
static const long long L1_hdr_t_Volume = tc_vmul(tc_vmul(1, 10), (1<<6)); // 640

struct L1_hdr_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt L1_hdr_A[tc_rows(L1_hdr_a_Volume, L1_hdr_hA) * L1_hdr_ND];
  cm_cnt L1_hdr_B[tc_rows(L1_hdr_b_Volume, L1_hdr_hB) * L1_hdr_ND];
  cm_cnt L1_hdr_C[L1_hdr_c_Volume * L1_hdr_ND];
  cm_cnt L1_hdr_D[tc_rows(L1_hdr_d_Volume, L1_hdr_hD) * L1_hdr_ND];
  cm_cnt L1_hdr_MT[tc_rows(L1_hdr_t_Volume, L1_hdr_hT) * L1_hdr_MND];
  i16 L1_hdr_S[tc_rows(L1_hdr_s_Volume, L1_hdr_hS) * L1_hdr_ND * L1_hdr_qs];
  u8 L1_hdr_SC[tc_rows(L1_hdr_s_Volume, L1_hdr_hS) * L1_hdr_ND * L1_hdr_qs];
  i16 L1_hdr_F[tc_rows(L1_hdr_f_Volume, L1_hdr_hF) * L1_hdr_ND * L1_hdr_qf];
  u8 L1_hdr_FC[tc_rows(L1_hdr_f_Volume, L1_hdr_hF) * L1_hdr_ND * L1_hdr_qf];
  i32 L1_hdr_W[tc_rows(L1_hdr_m_Volume, L1_hdr_hM) * L1_hdr_ND * TC_MIXN];
  u8 L1_hdr_WC[tc_rows(L1_hdr_m_Volume, L1_hdr_hM) * L1_hdr_ND];
  i32 L1_hdr_WM[tc_rows(L1_hdr_n_Volume, L1_hdr_hN) * L1_hdr_MND * 3];
  u8 L1_hdr_WMC[tc_rows(L1_hdr_n_Volume, L1_hdr_hN) * L1_hdr_MND];
  cm_cnt L1_hdr_G[L1_hdr_g_Volume * TC_SGN_HDR];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long L1_hdr_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L1_hdr_a_Volume, L1_hdr_hA) * L1_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L1_hdr_b_Volume, L1_hdr_hB) * L1_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(L1_hdr_c_Volume * L1_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L1_hdr_d_Volume, L1_hdr_hD) * L1_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(L1_hdr_t_Volume, L1_hdr_hT) * L1_hdr_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(L1_hdr_s_Volume, L1_hdr_hS) * L1_hdr_ND * L1_hdr_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L1_hdr_s_Volume, L1_hdr_hS) * L1_hdr_ND * L1_hdr_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(L1_hdr_f_Volume, L1_hdr_hF) * L1_hdr_ND * L1_hdr_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L1_hdr_f_Volume, L1_hdr_hF) * L1_hdr_ND * L1_hdr_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(L1_hdr_m_Volume, L1_hdr_hM) * L1_hdr_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L1_hdr_m_Volume, L1_hdr_hM) * L1_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(L1_hdr_n_Volume, L1_hdr_hN) * L1_hdr_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(L1_hdr_n_Volume, L1_hdr_hN) * L1_hdr_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(L1_hdr_g_Volume * TC_SGN_HDR);
  void L1_hdr_Init( void ) {
  
  }
};
