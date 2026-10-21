
static const byte H1_qpd_a_sq[13+1]={ 0,0,1,1,2,2,3,3,4,4,5,5,6,7 };

static const byte H1_qpd_b_sq[13+1]={ 0,0,1,2,2,3,3,4,4,5,5,6,6,7 };

static const byte H1_qpd_c_lz[6+1]={ 0,0,0,1,2,3,3 };
static const byte H1_qpd_c_db[8+1]={ 0,1,2,3,3,3,4,5,5 };
static const byte H1_qpd_c_bc[13+1]={ 0,1,2,2,3,3,3,4,4,4,4,5,5,6 };

static const byte H1_qpd_d_st[9+1]={ 0,0,1,2,2,3,3,3,3,3 };
static const byte H1_qpd_d_cl[13+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,11,12 };






static const int H1_qpd_rA = (254+0) * (1);
static const int H1_qpd_rB = (255+0) * (1);
static const int H1_qpd_rC = (255+0) * (1);
static const int H1_qpd_rD = (254+0) * (1);
static const int H1_qpd_rT = (32+0) * (1);
static const int H1_qpd_lr = (15+0) * (1);
static const int H1_qpd_lrm = (0+0) * (1);
static const int H1_qpd_mbm = (0+0) * (1);
static const int H1_qpd_mwA = (31+0) * (1);
static const int H1_qpd_mwB = (15+0) * (1);
static const int H1_qpd_mwC = (7+0) * (1);
static const int H1_qpd_mwD = (8+0) * (1);
static const int H1_qpd_mwT = (34+0) * (1);
static const int H1_qpd_mt = (2+0) * (1);
static const int H1_qpd_mb = (69+0) * (1);
static const int H1_qpd_rG = (0+0) * (1);
static const int H1_qpd_mwG = (0+0) * (1);
static const int H1_qpd_hA = (24+0) * (1);
static const int H1_qpd_hB = (24+0) * (1);
static const int H1_qpd_hC = (24+0) * (1);
static const int H1_qpd_hD = (24+0) * (1);
static const int H1_qpd_hM = (24+0) * (1);
static const int H1_qpd_hT = (24+0) * (1);
static const int H1_qpd_hN = (24+0) * (1);


static const long long H1_qpd_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 3), 3), 8); // 216
static const long long H1_qpd_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 3), 8); // 24
static const long long H1_qpd_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 2), 6), 7); // 336
static const long long H1_qpd_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, 1), 4), 13); // 52
static const long long H1_qpd_g_Volume = tc_vmul(1, 1); // 1
static const long long H1_qpd_m_Volume = tc_vmul(1, 2); // 2
static const long long H1_qpd_n_Volume = tc_vmul(1, 1); // 1
static const long long H1_qpd_t_Volume = tc_vmul(1, 1); // 1

#define H1_qpd_USES(X) X(pc) X(cc) X(cls) X(sq) X(lz) X(st) X(db) X(bc) X(cl) X(lnz)

struct H1_qpd_T {
  cm_cnt2s H1_qpd_A[tc_rows(H1_qpd_a_Volume, H1_qpd_hA) * H_qpd_ND];
  cm_cnt2s H1_qpd_B[tc_rows(H1_qpd_b_Volume, H1_qpd_hB) * H_qpd_ND];
  cm_cnt2s H1_qpd_C[tc_rows(H1_qpd_c_Volume, H1_qpd_hC) * H_qpd_ND];
  cm_cnt2s H1_qpd_D[tc_rows(H1_qpd_d_Volume, H1_qpd_hD) * H_qpd_ND];
  cm_cnt H1_qpd_MT[tc_rows(H1_qpd_t_Volume, H1_qpd_hT) * H_qpd_MND];
  i32 H1_qpd_W[tc_rows(H1_qpd_m_Volume, H1_qpd_hM) * H_qpd_ND * 16];
  u8 H1_qpd_WC[tc_rows(H1_qpd_m_Volume, H1_qpd_hM) * H_qpd_ND];
  i32 H1_qpd_WM[tc_rows(H1_qpd_n_Volume, H1_qpd_hN) * H_qpd_MND * 3];
  u8 H1_qpd_WMC[tc_rows(H1_qpd_n_Volume, H1_qpd_hN) * H_qpd_MND];
  cm_cnt H1_qpd_G[H1_qpd_g_Volume * H_qpd_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H1_qpd_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_qpd_a_Volume, H1_qpd_hA) * H_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_qpd_b_Volume, H1_qpd_hB) * H_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_qpd_c_Volume, H1_qpd_hC) * H_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_qpd_d_Volume, H1_qpd_hD) * H_qpd_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H1_qpd_t_Volume, H1_qpd_hT) * H_qpd_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_qpd_m_Volume, H1_qpd_hM) * H_qpd_ND * 16)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_qpd_m_Volume, H1_qpd_hM) * H_qpd_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_qpd_n_Volume, H1_qpd_hN) * H_qpd_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_qpd_n_Volume, H1_qpd_hN) * H_qpd_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H1_qpd_g_Volume * H_qpd_NG);
  void H1_qpd_Init( void ) {}
  void H1_qpd_Quit( void ) {}
};
