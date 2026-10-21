
static const byte H1_cbf_a_cat[13+1]={ 0,1,2,3,4,5,5,5,5,5,5,5,5,5 };
static const byte H1_cbf_a_na[9+1]={ 0,1,2,2,3,3,3,4,4,5 };
static const byte H1_cbf_a_nb[9+1]={ 0,1,2,2,3,3,4,4,4,5 };
static const byte H1_cbf_a_ly[10+1]={ 0,0,1,1,2,2,3,3,4,4,4 };

static const byte H1_cbf_b_cat[13+1]={ 0,1,2,3,4,5,5,5,5,5,5,5,5,5 };
static const byte H1_cbf_b_np[3+1]={ 0,1,2,3 };
static const byte H1_cbf_b_dq[8+1]={ 0,0,1,1,2,2,3,3,3 };

static const byte H1_cbf_c_cat[13+1]={ 0,1,2,3,4,4,4,4,4,4,4,4,4,4 };
static const byte H1_cbf_c_q6[8+1]={ 0,0,0,1,1,2,3,3,3 };
static const byte H1_cbf_c_ch[15+1]={ 0,1,2,3,4,4,4,5,6,6,6,7,7,7,7,7 };

static const byte H1_cbf_d_cat[13+1]={ 0,1,1,2,3,4,4,4,4,4,4,4,4,4 };
static const byte H1_cbf_d_md[19+1]={ 0,0,1,2,2,2,2,2,2,3,4,5,6,6,7,7,7,8,8,8 };
static const byte H1_cbf_d_q6[8+1]={ 0,0,1,2,3,3,3,3,3 };


static const byte H1_cbf_m_cat[13+1]={ 0,1,2,3,4,4,4,4,4,4,4,4,4,4 };


static const byte H1_cbf_t_cat[13+1]={ 0,1,2,3,4,5,5,5,5,5,5,5,5,5 };


static const int H1_cbf_rA = (255+0) * (1);
static const int H1_cbf_rB = (254+0) * (1);
static const int H1_cbf_rC = (255+0) * (1);
static const int H1_cbf_rD = (255+0) * (1);
static const int H1_cbf_rT = (48+0) * (1);
static const int H1_cbf_lr = (14+0) * (1);
static const int H1_cbf_lrm = (0+0) * (1);
static const int H1_cbf_mbm = (0+0) * (1);
static const int H1_cbf_mwA = (32+0) * (1);
static const int H1_cbf_mwB = (0+0) * (1);
static const int H1_cbf_mwC = (22+0) * (1);
static const int H1_cbf_mwD = (11+0) * (1);
static const int H1_cbf_mwT = (11+0) * (1);
static const int H1_cbf_mt = (4+0) * (1);
static const int H1_cbf_mb = (130+0) * (1);
static const int H1_cbf_rG = (0+0) * (1);
static const int H1_cbf_mwG = (0+0) * (1);
static const int H1_cbf_hA = (24+0) * (1);
static const int H1_cbf_hB = (17+0) * (1);
static const int H1_cbf_hC = (24+0) * (1);
static const int H1_cbf_hD = (16+0) * (1);
static const int H1_cbf_hM = (24+0) * (1);
static const int H1_cbf_hT = (24+0) * (1);
static const int H1_cbf_hN = (24+0) * (1);


static const long long H1_cbf_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 6), 6), 6), 2), 5); // 2160
static const long long H1_cbf_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 6), 4), 3), 3), 1), 4); // 864
static const long long H1_cbf_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 5), 4), 3), 1), 8); // 480
static const long long H1_cbf_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 5), 9), 4); // 180
static const long long H1_cbf_g_Volume = tc_vmul(1, 1); // 1
static const long long H1_cbf_m_Volume = tc_vmul(tc_vmul(1, 5), 2); // 10
static const long long H1_cbf_n_Volume = tc_vmul(1, 1); // 1
static const long long H1_cbf_t_Volume = tc_vmul(1, 6); // 6

#define H1_cbf_USES(X) X(cat) X(na) X(nb) X(intra) X(ly) X(np) X(pos) X(crcb) X(dq) X(q6) X(col) X(ch) X(md) X(kn)

struct H1_cbf_T {
  cm_cnt2s H1_cbf_A[tc_rows(H1_cbf_a_Volume, H1_cbf_hA) * H_cbf_ND];
  cm_cnt2s H1_cbf_B[tc_rows(H1_cbf_b_Volume, H1_cbf_hB) * H_cbf_ND];
  cm_cnt2s H1_cbf_C[tc_rows(H1_cbf_c_Volume, H1_cbf_hC) * H_cbf_ND];
  cm_cnt2s H1_cbf_D[tc_rows(H1_cbf_d_Volume, H1_cbf_hD) * H_cbf_ND];
  cm_cnt H1_cbf_MT[tc_rows(H1_cbf_t_Volume, H1_cbf_hT) * H_cbf_MND];
  i32 H1_cbf_W[tc_rows(H1_cbf_m_Volume, H1_cbf_hM) * H_cbf_ND * 16];
  u8 H1_cbf_WC[tc_rows(H1_cbf_m_Volume, H1_cbf_hM) * H_cbf_ND];
  i32 H1_cbf_WM[tc_rows(H1_cbf_n_Volume, H1_cbf_hN) * H_cbf_MND * 3];
  u8 H1_cbf_WMC[tc_rows(H1_cbf_n_Volume, H1_cbf_hN) * H_cbf_MND];
  cm_cnt H1_cbf_G[H1_cbf_g_Volume * H_cbf_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H1_cbf_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_cbf_a_Volume, H1_cbf_hA) * H_cbf_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_cbf_b_Volume, H1_cbf_hB) * H_cbf_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_cbf_c_Volume, H1_cbf_hC) * H_cbf_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_cbf_d_Volume, H1_cbf_hD) * H_cbf_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H1_cbf_t_Volume, H1_cbf_hT) * H_cbf_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_cbf_m_Volume, H1_cbf_hM) * H_cbf_ND * 16)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_cbf_m_Volume, H1_cbf_hM) * H_cbf_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_cbf_n_Volume, H1_cbf_hN) * H_cbf_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_cbf_n_Volume, H1_cbf_hN) * H_cbf_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H1_cbf_g_Volume * H_cbf_NG);
  void H1_cbf_Init( void ) {}
  void H1_cbf_Quit( void ) {}
};
