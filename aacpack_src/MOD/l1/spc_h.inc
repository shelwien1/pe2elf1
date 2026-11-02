
static const byte H1_spc_a_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };
static const byte H1_spc_a_a1[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte H1_spc_a_a2[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H1_spc_b_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };
static const byte H1_spc_b_sb[50+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50 };

static const byte H1_spc_c_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };
static const byte H1_spc_c_pn[12+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12 };
static const byte H1_spc_c_pf[5+1]={ 0,1,2,3,4,5 };

static const byte H1_spc_d_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };
static const byte H1_spc_d_pf[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte H1_spc_d_a1[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H1_spc_d_dg[69+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69 };


static const byte H1_spc_m_cb[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };
static const byte H1_spc_m_av[28+1]={ 0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7 };



static const int H1_spc_rA = (254+0) * (1);
static const int H1_spc_rB = (254+0) * (1);
static const int H1_spc_rC = (234+0) * (1);
static const int H1_spc_rD = (255+0) * (1);
static const int H1_spc_lr = (15+0) * (1);
static const int H1_spc_mwA = (8+0) * (1);
static const int H1_spc_mwB = (2+0) * (1);
static const int H1_spc_mwC = (0+0) * (1);
static const int H1_spc_mwD = (131+0) * (1);
static const int H1_spc_mb = (32+0) * (1);
static const int H1_spc_rT = (0+0) * (1);
static const int H1_spc_mwT = (0+0) * (1);
static const int H1_spc_mt = (0+0) * (1);
static const int H1_spc_rG = (0+0) * (1);
static const int H1_spc_mwG = (0+0) * (1);
static const int H1_spc_hA = (24+0) * (1);
static const int H1_spc_hB = (24+0) * (1);
static const int H1_spc_hC = (24+0) * (1);
static const int H1_spc_hD = (24+0) * (1);
static const int H1_spc_hM = (24+0) * (1);
static const int H1_spc_hT = (24+0) * (1);


static const long long H1_spc_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 12), 16), 8); // 1536
static const long long H1_spc_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 12), 51), 2); // 1224
static const long long H1_spc_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, 12), 13), 6); // 936
static const long long H1_spc_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 12), 16), 8), 70); // 107520
static const long long H1_spc_g_Volume = 1; // 1
static const long long H1_spc_m_Volume = tc_vmul(tc_vmul(tc_vmul(1, 12), 2), 8); // 192
static const long long H1_spc_t_Volume = 1; // 1

#define H1_spc_USES(X) X(cb) X(a1) X(a2) X(sb) X(ws) X(pn) X(pf) X(dg) X(av)

struct H1_spc_T {
  cm_cnt2s H1_spc_A[tc_rows(H1_spc_a_Volume, H1_spc_hA) * H_spc_ND];
  cm_cnt2s H1_spc_B[tc_rows(H1_spc_b_Volume, H1_spc_hB) * H_spc_ND];
  cm_cnt2s H1_spc_C[tc_rows(H1_spc_c_Volume, H1_spc_hC) * H_spc_ND];
  cm_cnt2s H1_spc_D[tc_rows(H1_spc_d_Volume, H1_spc_hD) * H_spc_ND];
  cm_cnt H1_spc_MT[tc_rows(H1_spc_t_Volume, H1_spc_hT) * H_spc_MND];
  i32 H1_spc_W[tc_rows(H1_spc_m_Volume, H1_spc_hM) * H_spc_ND * tc_nin(4)];
  u8 H1_spc_WC[tc_rows(H1_spc_m_Volume, H1_spc_hM) * H_spc_ND];
  cm_cnt H1_spc_G[H1_spc_g_Volume * H_spc_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H1_spc_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_spc_a_Volume, H1_spc_hA) * H_spc_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_spc_b_Volume, H1_spc_hB) * H_spc_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_spc_c_Volume, H1_spc_hC) * H_spc_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_spc_d_Volume, H1_spc_hD) * H_spc_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H1_spc_t_Volume, H1_spc_hT) * H_spc_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_spc_m_Volume, H1_spc_hM) * H_spc_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_spc_m_Volume, H1_spc_hM) * H_spc_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H1_spc_g_Volume * H_spc_NG);
  void H1_spc_Init( void ) {}
  void H1_spc_Quit( void ) {}
};
