
static const byte H3_sec_a_kd[3+1]={ 0,1,2,3 };
static const byte H3_sec_a_oc[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
static const byte H3_sec_a_pc[3+1]={ 0,1,2,3 };
static const byte H3_sec_a_pr[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };

static const byte H3_sec_b_kd[3+1]={ 0,1,2,3 };
static const byte H3_sec_b_gc[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
static const byte H3_sec_b_ws[3+1]={ 0,1,2,3 };
static const byte H3_sec_b_ch[12+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12 };

static const byte H3_sec_c_kd[3+1]={ 0,1,2,3 };
static const byte H3_sec_c_run[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
static const byte H3_sec_c_cb[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
static const byte H3_sec_c_ch[12+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12 };

static const byte H3_sec_d_kd[3+1]={ 0,1,2,3 };
static const byte H3_sec_d_pc[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
static const byte H3_sec_d_fc[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
static const byte H3_sec_d_pr[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };

static const byte H3_sec_e_kd[3+1]={ 0,1,2,3 };
static const byte H3_sec_e_sb[50+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50 };
static const byte H3_sec_e_ws[3+1]={ 0,1,2,3 };
static const byte H3_sec_e_cb[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
static const byte H3_sec_e_bq[4+1]={ 0,1,2,3,4 };


static const byte H3_sec_h_kd[3+1]={ 0,1,2,3 };
static const byte H3_sec_h_cb[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
static const byte H3_sec_h_fc[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
static const byte H3_sec_h_run[3+1]={ 0,1,2,3 };
static const byte H3_sec_h_pr[16+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };

static const byte H3_sec_m_kd[3+1]={ 0,1,2,3 };
static const byte H3_sec_m_bq[4+1]={ 0,1,2,3,4 };
static const byte H3_sec_m_pr[6+1]={ 0,1,2,3,4,5,6 };



static const int H3_sec_rA = (255+0) * (1);
static const int H3_sec_rB = (255+0) * (1);
static const int H3_sec_rC = (234+0) * (1);
static const int H3_sec_rD = (254+0) * (1);
static const int H3_sec_rE = (189+0) * (1);
static const int H3_sec_rH = (254+0) * (1);
static const int H3_sec_lr = (15+0) * (1);
static const int H3_sec_mwA = (20+0) * (1);
static const int H3_sec_mwB = (31+0) * (1);
static const int H3_sec_mwC = (0+0) * (1);
static const int H3_sec_mwD = (8+0) * (1);
static const int H3_sec_mwE = (32+0) * (1);
static const int H3_sec_mwH = (2+0) * (1);
static const int H3_sec_mb = (32+0) * (1);
static const int H3_sec_rT = (0+0) * (1);
static const int H3_sec_mwT = (0+0) * (1);
static const int H3_sec_mt = (0+0) * (1);
static const int H3_sec_rG = (0+0) * (1);
static const int H3_sec_mwG = (0+0) * (1);
static const int H3_sec_hA = (24+0) * (1);
static const int H3_sec_hB = (24+0) * (1);
static const int H3_sec_hC = (24+0) * (1);
static const int H3_sec_hD = (24+0) * (1);
static const int H3_sec_hE = (24+0) * (1);
static const int H3_sec_hH = (24+0) * (1);
static const int H3_sec_hM = (24+0) * (1);
static const int H3_sec_hT = (24+0) * (1);


static const long long H3_sec_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 17), 4), 3), 17); // 13872
static const long long H3_sec_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 17), 4), 13); // 3536
static const long long H3_sec_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 17), 17), 13); // 15028
static const long long H3_sec_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 17), 17), 17); // 19652
static const long long H3_sec_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 51), 4), 17), 5); // 69360
static const long long H3_sec_g_Volume = 1; // 1
static const long long H3_sec_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 4), 17), 17), 4), 17); // 78608
static const long long H3_sec_m_Volume = tc_vmul(tc_vmul(tc_vmul(1, 4), 5), 7); // 140
static const long long H3_sec_t_Volume = 1; // 1

#define H3_sec_USES(X) X(kd) X(oc) X(pc) X(mu) X(pr) X(gc) X(ws) X(ch) X(run) X(cb) X(fc) X(sb) X(bq)

struct H3_sec_T {
  cm_cnt2 H3_sec_A[tc_rows(H3_sec_a_Volume, H3_sec_hA) * H_sec_ND];
  cm_cnt2 H3_sec_B[tc_rows(H3_sec_b_Volume, H3_sec_hB) * H_sec_ND];
  cm_cnt2 H3_sec_C[tc_rows(H3_sec_c_Volume, H3_sec_hC) * H_sec_ND];
  cm_cnt2 H3_sec_D[tc_rows(H3_sec_d_Volume, H3_sec_hD) * H_sec_ND];
  cm_cnt2 H3_sec_E[tc_rows(H3_sec_e_Volume, H3_sec_hE) * H_sec_ND];
  cm_cnt2 H3_sec_H[tc_rows(H3_sec_h_Volume, H3_sec_hH) * H_sec_ND];
  cm_cnt H3_sec_MT[tc_rows(H3_sec_t_Volume, H3_sec_hT) * H_sec_MND];
  i32 H3_sec_W[tc_rows(H3_sec_m_Volume, H3_sec_hM) * H_sec_ND * tc_nin(6)];
  u8 H3_sec_WC[tc_rows(H3_sec_m_Volume, H3_sec_hM) * H_sec_ND];
  cm_cnt H3_sec_G[H3_sec_g_Volume * H_sec_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H3_sec_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sec_a_Volume, H3_sec_hA) * H_sec_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sec_b_Volume, H3_sec_hB) * H_sec_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sec_c_Volume, H3_sec_hC) * H_sec_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sec_d_Volume, H3_sec_hD) * H_sec_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sec_e_Volume, H3_sec_hE) * H_sec_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sec_h_Volume, H3_sec_hH) * H_sec_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H3_sec_t_Volume, H3_sec_hT) * H_sec_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_sec_m_Volume, H3_sec_hM) * H_sec_ND * tc_nin(6))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_sec_m_Volume, H3_sec_hM) * H_sec_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H3_sec_g_Volume * H_sec_NG);
  void H3_sec_Init( void ) {}
  void H3_sec_Quit( void ) {}
};
