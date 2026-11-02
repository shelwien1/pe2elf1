
static const byte H3_hdr_a_dm[60+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60 };

static const byte H3_hdr_b_fld[15+1]={ 0,1,1,2,3,4,5,6,6,7,8,9,10,10,11,12 };
static const byte H3_hdr_b_hx[20+1]={ 0,1,1,1,2,2,3,4,4,4,4,4,4,4,5,6,7,8,8,8,8 };

static const byte H3_hdr_c_pb[8+1]={ 0,1,2,3,3,3,3,3,3 };

static const byte H3_hdr_d_ws[4+1]={ 0,1,2,3,4 };
static const byte H3_hdr_d_fld[7+1]={ 0,1,2,3,4,5,6,6 };
static const byte H3_hdr_d_ml[7+1]={ 0,0,1,2,3,4,4,4 };

static const byte H3_hdr_e_p1[10+1]={ 0,1,2,2,2,2,2,2,2,2,3 };

static const byte H3_hdr_g_p1[6+1]={ 0,1,2,3,4,5,5 };





static const int H3_hdr_rA = (254+0) * (1);
static const int H3_hdr_rB = (243+0) * (1);
static const int H3_hdr_rC = (255+0) * (1);
static const int H3_hdr_rD = (255+0) * (1);
static const int H3_hdr_rE = (255+0) * (1);
static const int H3_hdr_rH = (223+0) * (1);
static const int H3_hdr_rT = (138+0) * (1);
static const int H3_hdr_rG = (64+0) * (1);
static const int H3_hdr_lr = (15+0) * (1);
static const int H3_hdr_mwA = (3+0) * (1);
static const int H3_hdr_mwB = (0+0) * (1);
static const int H3_hdr_mwC = (1+0) * (1);
static const int H3_hdr_mwD = (0+0) * (1);
static const int H3_hdr_mwE = (0+0) * (1);
static const int H3_hdr_mwH = (0+0) * (1);
static const int H3_hdr_mwT = (0+0) * (1);
static const int H3_hdr_mwG = (0+0) * (1);
static const int H3_hdr_mt = (0+0) * (1);
static const int H3_hdr_mb = (63+0) * (1);
static const int H3_hdr_hA = (24+0) * (1);
static const int H3_hdr_hB = (24+0) * (1);
static const int H3_hdr_hC = (24+0) * (1);
static const int H3_hdr_hD = (24+0) * (1);
static const int H3_hdr_hE = (24+0) * (1);
static const int H3_hdr_hH = (24+0) * (1);
static const int H3_hdr_hM = (24+0) * (1);
static const int H3_hdr_hT = (24+0) * (1);


static const long long H3_hdr_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 3), 2), 61); // 2928
static const long long H3_hdr_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 3), 13), 9); // 89856
static const long long H3_hdr_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<7)), 4), (1<<5)); // 16384
static const long long H3_hdr_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), 5), 7), 5); // 11200
static const long long H3_hdr_e_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 4), 3); // 3072
static const long long H3_hdr_g_Volume = tc_vmul(1, 6); // 6
static const long long H3_hdr_h_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<7)), 3), (1<<4)); // 6144
static const long long H3_hdr_m_Volume = tc_vmul(1, (1<<3)); // 8
static const long long H3_hdr_t_Volume = tc_vmul(1, (1<<4)); // 16

#define H3_hdr_USES(X) X(tag) X(fld) X(p1) X(dm) X(ch) X(hx) X(pb) X(p1r) X(ws) X(ml) X(p2) X(nb)

struct H3_hdr_T {
  cm_cnt2 H3_hdr_A[tc_rows(H3_hdr_a_Volume, H3_hdr_hA) * H_hdr_ND];
  cm_cnt2 H3_hdr_B[tc_rows(H3_hdr_b_Volume, H3_hdr_hB) * H_hdr_ND];
  cm_cnt2 H3_hdr_C[tc_rows(H3_hdr_c_Volume, H3_hdr_hC) * H_hdr_ND];
  cm_cnt2 H3_hdr_D[tc_rows(H3_hdr_d_Volume, H3_hdr_hD) * H_hdr_ND];
  cm_cnt2 H3_hdr_E[tc_rows(H3_hdr_e_Volume, H3_hdr_hE) * H_hdr_ND];
  cm_cnt2 H3_hdr_H[tc_rows(H3_hdr_h_Volume, H3_hdr_hH) * H_hdr_ND];
  cm_cnt H3_hdr_MT[tc_rows(H3_hdr_t_Volume, H3_hdr_hT) * H_hdr_MND];
  i32 H3_hdr_W[tc_rows(H3_hdr_m_Volume, H3_hdr_hM) * H_hdr_ND * tc_nin(6)];
  u8 H3_hdr_WC[tc_rows(H3_hdr_m_Volume, H3_hdr_hM) * H_hdr_ND];
  cm_cnt H3_hdr_G[H3_hdr_g_Volume * H_hdr_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H3_hdr_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_hdr_a_Volume, H3_hdr_hA) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_hdr_b_Volume, H3_hdr_hB) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_hdr_c_Volume, H3_hdr_hC) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_hdr_d_Volume, H3_hdr_hD) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_hdr_e_Volume, H3_hdr_hE) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_hdr_h_Volume, H3_hdr_hH) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H3_hdr_t_Volume, H3_hdr_hT) * H_hdr_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_hdr_m_Volume, H3_hdr_hM) * H_hdr_ND * tc_nin(6))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_hdr_m_Volume, H3_hdr_hM) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H3_hdr_g_Volume * H_hdr_NG);
  void H3_hdr_Init( void ) {}
  void H3_hdr_Quit( void ) {}
};
