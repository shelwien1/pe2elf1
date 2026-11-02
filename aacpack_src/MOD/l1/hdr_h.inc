
static const byte H1_hdr_a_fld[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H1_hdr_a_p1[6+1]={ 0,1,2,3,4,5,6 };
static const byte H1_hdr_a_dm[60+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60 };

static const byte H1_hdr_b_fld[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte H1_hdr_b_hx[20+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20 };

static const byte H1_hdr_c_pb[8+1]={ 0,1,2,3,4,5,6,7,8 };

static const byte H1_hdr_d_ws[4+1]={ 0,1,2,3,4 };
static const byte H1_hdr_d_fld[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H1_hdr_d_ml[7+1]={ 0,1,2,3,4,5,6,7 };

static const byte H1_hdr_g_p1[6+1]={ 0,1,2,3,4,5,6 };




static const int H1_hdr_rA = (255+0) * (1);
static const int H1_hdr_rB = (255+0) * (1);
static const int H1_hdr_rC = (137+0) * (1);
static const int H1_hdr_rD = (255+0) * (1);
static const int H1_hdr_rT = (128+0) * (1);
static const int H1_hdr_rG = (64+0) * (1);
static const int H1_hdr_lr = (15+0) * (1);
static const int H1_hdr_mwA = (27+0) * (1);
static const int H1_hdr_mwB = (8+0) * (1);
static const int H1_hdr_mwC = (1+0) * (1);
static const int H1_hdr_mwD = (16+0) * (1);
static const int H1_hdr_mwT = (0+0) * (1);
static const int H1_hdr_mwG = (0+0) * (1);
static const int H1_hdr_mt = (2+0) * (1);
static const int H1_hdr_mb = (8+0) * (1);
static const int H1_hdr_hA = (24+0) * (1);
static const int H1_hdr_hB = (24+0) * (1);
static const int H1_hdr_hC = (24+0) * (1);
static const int H1_hdr_hD = (24+0) * (1);
static const int H1_hdr_hM = (24+0) * (1);
static const int H1_hdr_hT = (24+0) * (1);


static const long long H1_hdr_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 8), 7), 61); // 874496
static const long long H1_hdr_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 3), 16), 21); // 258048
static const long long H1_hdr_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 9), (1<<3)); // 18432
static const long long H1_hdr_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 5), 8), 8); // 81920
static const long long H1_hdr_g_Volume = tc_vmul(1, 7); // 7
static const long long H1_hdr_m_Volume = tc_vmul(1, (1<<8)); // 256
static const long long H1_hdr_t_Volume = tc_vmul(1, (1<<8)); // 256

#define H1_hdr_USES(X) X(tag) X(fld) X(p1) X(dm) X(ch) X(hx) X(pb) X(p1r) X(ws) X(ml)

struct H1_hdr_T {
  cm_cnt2s H1_hdr_A[tc_rows(H1_hdr_a_Volume, H1_hdr_hA) * H_hdr_ND];
  cm_cnt2s H1_hdr_B[tc_rows(H1_hdr_b_Volume, H1_hdr_hB) * H_hdr_ND];
  cm_cnt2s H1_hdr_C[tc_rows(H1_hdr_c_Volume, H1_hdr_hC) * H_hdr_ND];
  cm_cnt2s H1_hdr_D[tc_rows(H1_hdr_d_Volume, H1_hdr_hD) * H_hdr_ND];
  cm_cnt H1_hdr_MT[tc_rows(H1_hdr_t_Volume, H1_hdr_hT) * H_hdr_MND];
  i32 H1_hdr_W[tc_rows(H1_hdr_m_Volume, H1_hdr_hM) * H_hdr_ND * tc_nin(4)];
  u8 H1_hdr_WC[tc_rows(H1_hdr_m_Volume, H1_hdr_hM) * H_hdr_ND];
  cm_cnt H1_hdr_G[H1_hdr_g_Volume * H_hdr_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H1_hdr_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_hdr_a_Volume, H1_hdr_hA) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_hdr_b_Volume, H1_hdr_hB) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_hdr_c_Volume, H1_hdr_hC) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_hdr_d_Volume, H1_hdr_hD) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H1_hdr_t_Volume, H1_hdr_hT) * H_hdr_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_hdr_m_Volume, H1_hdr_hM) * H_hdr_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_hdr_m_Volume, H1_hdr_hM) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H1_hdr_g_Volume * H_hdr_NG);
  void H1_hdr_Init( void ) {}
  void H1_hdr_Quit( void ) {}
};
