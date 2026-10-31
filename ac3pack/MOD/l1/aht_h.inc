
static const byte A1_aht_a_kd[4+1]={ 0,1,2,3,4 };
static const byte A1_aht_a_hb[19+1]={ 0,0,0,1,2,3,3,4,4,4,4,5,6,7,8,9,10,11,12,13 };

static const byte A1_aht_b_kd[4+1]={ 0,1,2,3,4 };
static const byte A1_aht_b_hb[19+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19 };
static const byte A1_aht_b_bk[5+1]={ 0,1,2,3,4,5 };

static const byte A1_aht_c_kd[4+1]={ 0,1,2,3,4 };
static const byte A1_aht_c_hb[19+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19 };
static const byte A1_aht_c_q1[23+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23 };

static const byte A1_aht_d_kd[4+1]={ 0,1,2,3,4 };
static const byte A1_aht_d_bk[5+1]={ 0,1,2,3,4,5 };
static const byte A1_aht_d_qa[23+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23 };

static const byte A1_aht_g_hb[19+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19 };

static const byte A1_aht_m_kd[4+1]={ 0,1,2,3,4 };
static const byte A1_aht_m_hb[19+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19 };

static const byte A1_aht_n_kd[4+1]={ 0,1,2,3,4 };

static const byte A1_aht_t_kd[4+1]={ 0,1,2,3,4 };
static const byte A1_aht_t_hb[19+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19 };


static const int A1_aht_rA = (255+0) * (1);
static const int A1_aht_mwA = (4+0) * (1);
static const int A1_aht_rB = (255+0) * (1);
static const int A1_aht_mwB = (4+0) * (1);
static const int A1_aht_rC = (255+0) * (1);
static const int A1_aht_mwC = (4+0) * (1);
static const int A1_aht_rD = (255+0) * (1);
static const int A1_aht_mwD = (4+0) * (1);
static const int A1_aht_lr = (15+0) * (1);
static const int A1_aht_mb = (51+0) * (1);
static const int A1_aht_rT = (255+0) * (1);
static const int A1_aht_mwT = (2+0) * (1);
static const int A1_aht_mt = (6+0) * (1);
static const int A1_aht_lrm = (31+0) * (1);
static const int A1_aht_mbm = (25+0) * (1);
static const int A1_aht_rG = (255+0) * (1);
static const int A1_aht_mwG = (0+0) * (1);
static const int A1_aht_hA = (22+0) * (1);
static const int A1_aht_hB = (22+0) * (1);
static const int A1_aht_hC = (22+0) * (1);
static const int A1_aht_hD = (22+0) * (1);
static const int A1_aht_hM = (22+0) * (1);
static const int A1_aht_hT = (22+0) * (1);
static const int A1_aht_hN = (22+0) * (1);


static const long long A1_aht_a_Volume = tc_vmul(tc_vmul(1, 5), 14); // 70
static const long long A1_aht_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 5), 20), 3), 6); // 1800
static const long long A1_aht_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 5), 20), 24); // 2400
static const long long A1_aht_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 5), 6), 24), 3); // 2160
static const long long A1_aht_g_Volume = tc_vmul(1, 20); // 20
static const long long A1_aht_m_Volume = tc_vmul(tc_vmul(1, 5), 20); // 100
static const long long A1_aht_n_Volume = tc_vmul(1, 5); // 5
static const long long A1_aht_t_Volume = tc_vmul(tc_vmul(1, 5), 20); // 100

#define A1_aht_USES(X) X(kd) X(hb) X(lg) X(bk) X(q1) X(qa)

struct A1_aht_T {
  cm_cnt2s A1_aht_A[tc_rows(A1_aht_a_Volume, A1_aht_hA) * M_aht_ND];
  cm_cnt2s A1_aht_B[tc_rows(A1_aht_b_Volume, A1_aht_hB) * M_aht_ND];
  cm_cnt2s A1_aht_C[tc_rows(A1_aht_c_Volume, A1_aht_hC) * M_aht_ND];
  cm_cnt2s A1_aht_D[tc_rows(A1_aht_d_Volume, A1_aht_hD) * M_aht_ND];
  cm_cnt A1_aht_MT[tc_rows(A1_aht_t_Volume, A1_aht_hT) * M_aht_MND];
  i32 A1_aht_W[tc_rows(A1_aht_m_Volume, A1_aht_hM) * M_aht_ND * tc_nin(4)];
  u8 A1_aht_WC[tc_rows(A1_aht_m_Volume, A1_aht_hM) * M_aht_ND];
  i32 A1_aht_WM[tc_rows(A1_aht_n_Volume, A1_aht_hN) * M_aht_MND * TC_MIN];
  u8 A1_aht_WMC[tc_rows(A1_aht_n_Volume, A1_aht_hN) * M_aht_MND];
  cm_cnt A1_aht_G[A1_aht_g_Volume * M_aht_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long A1_aht_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_aht_a_Volume, A1_aht_hA) * M_aht_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_aht_b_Volume, A1_aht_hB) * M_aht_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_aht_c_Volume, A1_aht_hC) * M_aht_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_aht_d_Volume, A1_aht_hD) * M_aht_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(A1_aht_t_Volume, A1_aht_hT) * M_aht_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(A1_aht_m_Volume, A1_aht_hM) * M_aht_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(A1_aht_m_Volume, A1_aht_hM) * M_aht_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(A1_aht_n_Volume, A1_aht_hN) * M_aht_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(A1_aht_n_Volume, A1_aht_hN) * M_aht_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(A1_aht_g_Volume * M_aht_NG);
  void A1_aht_Init( void ) {}
  void A1_aht_Quit( void ) {}
};
