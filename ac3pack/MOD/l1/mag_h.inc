
static const byte A1_mag_a_bap[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte A1_mag_a_gr[3+1]={ 0,1,2,3 };
static const byte A1_mag_a_tx[4+1]={ 0,1,2,3,4 };
static const byte A1_mag_a_tc[3+1]={ 0,1,2,3 };
static const byte A1_mag_a_tr[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte A1_mag_b_bap[15+1]={ 0,0,0,0,0,0,0,1,2,3,3,3,3,3,3,3 };
static const byte A1_mag_b_e1[23+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23 };
static const byte A1_mag_b_et[23+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23 };
static const byte A1_mag_b_tq[4+1]={ 0,1,2,3,4 };

static const byte A1_mag_c_bap[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte A1_mag_c_ep[23+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23 };

static const byte A1_mag_d_bap[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte A1_mag_d_e1[16+1]={ 0,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9 };
static const byte A1_mag_d_ew[16+1]={ 0,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9 };
static const byte A1_mag_d_ow[16+1]={ 0,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9 };

static const byte A1_mag_g_tq[4+1]={ 0,1,2,3,4 };

static const byte A1_mag_m_bap[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte A1_mag_m_tx[4+1]={ 0,1,2,3,4 };
static const byte A1_mag_m_tr[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte A1_mag_n_bap[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

static const byte A1_mag_t_bap[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };


static const int A1_mag_rA = (255+0) * (1);
static const int A1_mag_mwA = (76+0) * (1);
static const int A1_mag_rB = (252+0) * (1);
static const int A1_mag_mwB = (4+0) * (1);
static const int A1_mag_rC = (255+0) * (1);
static const int A1_mag_mwC = (4+0) * (1);
static const int A1_mag_rD = (255+0) * (1);
static const int A1_mag_mwD = (4+0) * (1);
static const int A1_mag_lr = (13+0) * (1);
static const int A1_mag_mb = (51+0) * (1);
static const int A1_mag_rT = (128+0) * (1);
static const int A1_mag_mwT = (2+0) * (1);
static const int A1_mag_mt = (6+0) * (1);
static const int A1_mag_lrm = (31+0) * (1);
static const int A1_mag_mbm = (25+0) * (1);
static const int A1_mag_rG = (255+0) * (1);
static const int A1_mag_mwG = (0+0) * (1);
static const int A1_mag_hA = (22+0) * (1);
static const int A1_mag_hB = (22+0) * (1);
static const int A1_mag_hC = (22+0) * (1);
static const int A1_mag_hD = (22+0) * (1);
static const int A1_mag_hM = (22+0) * (1);
static const int A1_mag_hT = (22+0) * (1);
static const int A1_mag_hN = (22+0) * (1);


static const long long A1_mag_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 16), 4), 5), 3), 4), 8); // 30720
static const long long A1_mag_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 24), 3), 24), 5); // 34560
static const long long A1_mag_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 16), 24), 3); // 1152
static const long long A1_mag_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 16), 10), 10), 10), 3), 3), 3), 3); // 1296000
static const long long A1_mag_g_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 3), 3), 3), 5); // 405
static const long long A1_mag_m_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 16), 5), 3), 8), 3); // 5760
static const long long A1_mag_n_Volume = tc_vmul(1, 16); // 16
static const long long A1_mag_t_Volume = tc_vmul(1, 16); // 16

#define A1_mag_USES(X) X(bap) X(gr) X(tx) X(tb) X(tc) X(tr) X(e1) X(ts) X(et) X(tq) X(ep) X(sp) X(ew) X(ow) X(s1) X(so) X(rm)

struct A1_mag_T {
  cm_cnt2s A1_mag_A[tc_rows(A1_mag_a_Volume, A1_mag_hA) * M_mag_ND];
  cm_cnt2s A1_mag_B[tc_rows(A1_mag_b_Volume, A1_mag_hB) * M_mag_ND];
  cm_cnt2s A1_mag_C[tc_rows(A1_mag_c_Volume, A1_mag_hC) * M_mag_ND];
  cm_cnt2s A1_mag_D[tc_rows(A1_mag_d_Volume, A1_mag_hD) * M_mag_ND];
  cm_cnt A1_mag_MT[tc_rows(A1_mag_t_Volume, A1_mag_hT) * M_mag_MND];
  i32 A1_mag_W[tc_rows(A1_mag_m_Volume, A1_mag_hM) * M_mag_ND * tc_nin(4)];
  u8 A1_mag_WC[tc_rows(A1_mag_m_Volume, A1_mag_hM) * M_mag_ND];
  i32 A1_mag_WM[tc_rows(A1_mag_n_Volume, A1_mag_hN) * M_mag_MND * TC_MIN];
  u8 A1_mag_WMC[tc_rows(A1_mag_n_Volume, A1_mag_hN) * M_mag_MND];
  cm_cnt A1_mag_G[A1_mag_g_Volume * M_mag_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long A1_mag_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_mag_a_Volume, A1_mag_hA) * M_mag_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_mag_b_Volume, A1_mag_hB) * M_mag_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_mag_c_Volume, A1_mag_hC) * M_mag_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_mag_d_Volume, A1_mag_hD) * M_mag_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(A1_mag_t_Volume, A1_mag_hT) * M_mag_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(A1_mag_m_Volume, A1_mag_hM) * M_mag_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(A1_mag_m_Volume, A1_mag_hM) * M_mag_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(A1_mag_n_Volume, A1_mag_hN) * M_mag_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(A1_mag_n_Volume, A1_mag_hN) * M_mag_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(A1_mag_g_Volume * M_mag_NG);
  void A1_mag_Init( void ) {}
  void A1_mag_Quit( void ) {}
};
