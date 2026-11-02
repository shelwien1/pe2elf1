
static const byte H1_sbr_a_kd[14+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14 };
static const byte H1_sbr_a_bd[30+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30 };

static const byte H1_sbr_b_kd[14+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14 };
static const byte H1_sbr_b_lv[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };

static const byte H1_sbr_c_kd[14+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14 };
static const byte H1_sbr_c_d1[10+1]={ 0,1,2,3,4,5,6,7,8,9,10 };
static const byte H1_sbr_c_d2[6+1]={ 0,1,2,3,4,5,6 };
static const byte H1_sbr_c_rm[14+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14 };

static const byte H1_sbr_d_kd[14+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14 };
static const byte H1_sbr_d_bd[12+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12 };

static const byte H1_sbr_g_d1[5+1]={ 0,1,2,3,4,5 };

static const byte H1_sbr_m_kd[14+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14 };

static const byte H1_sbr_t_kd[14+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14 };


static const int H1_sbr_rA = (255+0) * (1);
static const int H1_sbr_rB = (255+0) * (1);
static const int H1_sbr_rC = (255+0) * (1);
static const int H1_sbr_rD = (253+0) * (1);
static const int H1_sbr_rT = (128+0) * (1);
static const int H1_sbr_rG = (64+0) * (1);
static const int H1_sbr_lr = (15+0) * (1);
static const int H1_sbr_mwA = (4+0) * (1);
static const int H1_sbr_mwB = (16+0) * (1);
static const int H1_sbr_mwC = (27+0) * (1);
static const int H1_sbr_mwD = (62+0) * (1);
static const int H1_sbr_mwT = (0+0) * (1);
static const int H1_sbr_mwG = (0+0) * (1);
static const int H1_sbr_mt = (2+0) * (1);
static const int H1_sbr_mb = (8+0) * (1);
static const int H1_sbr_hA = (24+0) * (1);
static const int H1_sbr_hB = (24+0) * (1);
static const int H1_sbr_hC = (24+0) * (1);
static const int H1_sbr_hD = (24+0) * (1);
static const int H1_sbr_hM = (24+0) * (1);
static const int H1_sbr_hT = (24+0) * (1);


static const long long H1_sbr_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 15), 31), 2); // 930
static const long long H1_sbr_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 15), 12), 2); // 360
static const long long H1_sbr_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 15), 11), 7), 15); // 17325
static const long long H1_sbr_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 15), 2), 2), 13); // 780
static const long long H1_sbr_g_Volume = tc_vmul(1, 6); // 6
static const long long H1_sbr_m_Volume = tc_vmul(1, 15); // 15
static const long long H1_sbr_t_Volume = tc_vmul(1, 15); // 15

#define H1_sbr_USES(X) X(kd) X(bd) X(ar) X(lv) X(ch) X(d1) X(d2) X(rm) X(fr)

struct H1_sbr_T {
  cm_cnt2s H1_sbr_A[tc_rows(H1_sbr_a_Volume, H1_sbr_hA) * H_sbr_ND];
  cm_cnt2s H1_sbr_B[tc_rows(H1_sbr_b_Volume, H1_sbr_hB) * H_sbr_ND];
  cm_cnt2s H1_sbr_C[tc_rows(H1_sbr_c_Volume, H1_sbr_hC) * H_sbr_ND];
  cm_cnt2s H1_sbr_D[tc_rows(H1_sbr_d_Volume, H1_sbr_hD) * H_sbr_ND];
  cm_cnt H1_sbr_MT[tc_rows(H1_sbr_t_Volume, H1_sbr_hT) * H_sbr_MND];
  i32 H1_sbr_W[tc_rows(H1_sbr_m_Volume, H1_sbr_hM) * H_sbr_ND * tc_nin(4)];
  u8 H1_sbr_WC[tc_rows(H1_sbr_m_Volume, H1_sbr_hM) * H_sbr_ND];
  cm_cnt H1_sbr_G[H1_sbr_g_Volume * H_sbr_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H1_sbr_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_sbr_a_Volume, H1_sbr_hA) * H_sbr_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_sbr_b_Volume, H1_sbr_hB) * H_sbr_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_sbr_c_Volume, H1_sbr_hC) * H_sbr_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_sbr_d_Volume, H1_sbr_hD) * H_sbr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H1_sbr_t_Volume, H1_sbr_hT) * H_sbr_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_sbr_m_Volume, H1_sbr_hM) * H_sbr_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_sbr_m_Volume, H1_sbr_hM) * H_sbr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H1_sbr_g_Volume * H_sbr_NG);
  void H1_sbr_Init( void ) {}
  void H1_sbr_Quit( void ) {}
};
