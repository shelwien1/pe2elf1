
static const byte H1_esc_a_a1[12+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12 };
static const byte H1_esc_a_la[5+1]={ 0,1,2,3,4,5 };
static const byte H1_esc_a_xp[39+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39 };

static const byte H1_esc_b_sb[50+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50 };

static const byte H1_esc_c_la[12+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12 };
static const byte H1_esc_c_kq[5+1]={ 0,1,2,3,4,5 };

static const byte H1_esc_d_pf[12+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12 };
static const byte H1_esc_d_oc[5+1]={ 0,1,2,3,4,5 };



static const byte H1_esc_t_kq[19+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19 };


static const int H1_esc_rA = (255+0) * (1);
static const int H1_esc_rB = (255+0) * (1);
static const int H1_esc_rC = (255+0) * (1);
static const int H1_esc_rD = (137+0) * (1);
static const int H1_esc_rT = (128+0) * (1);
static const int H1_esc_rG = (64+0) * (1);
static const int H1_esc_lr = (15+0) * (1);
static const int H1_esc_mwA = (8+0) * (1);
static const int H1_esc_mwB = (16+0) * (1);
static const int H1_esc_mwC = (27+0) * (1);
static const int H1_esc_mwD = (1+0) * (1);
static const int H1_esc_mwT = (0+0) * (1);
static const int H1_esc_mwG = (0+0) * (1);
static const int H1_esc_mt = (2+0) * (1);
static const int H1_esc_mb = (8+0) * (1);
static const int H1_esc_hA = (24+0) * (1);
static const int H1_esc_hB = (24+0) * (1);
static const int H1_esc_hC = (24+0) * (1);
static const int H1_esc_hD = (24+0) * (1);
static const int H1_esc_hM = (24+0) * (1);
static const int H1_esc_hT = (24+0) * (1);


static const long long H1_esc_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 13), 6), 40); // 3120
static const long long H1_esc_b_Volume = tc_vmul(1, 51); // 51
static const long long H1_esc_c_Volume = tc_vmul(tc_vmul(1, 13), 6); // 78
static const long long H1_esc_d_Volume = tc_vmul(tc_vmul(1, 13), 6); // 78
static const long long H1_esc_g_Volume = 1; // 1
static const long long H1_esc_m_Volume = tc_vmul(1, 2); // 2
static const long long H1_esc_t_Volume = tc_vmul(1, 20); // 20

#define H1_esc_USES(X) X(a1) X(la) X(xp) X(sb) X(kq) X(pf) X(oc) X(ws)

struct H1_esc_T {
  cm_cnt2s H1_esc_A[tc_rows(H1_esc_a_Volume, H1_esc_hA) * H_esc_ND];
  cm_cnt2s H1_esc_B[tc_rows(H1_esc_b_Volume, H1_esc_hB) * H_esc_ND];
  cm_cnt2s H1_esc_C[tc_rows(H1_esc_c_Volume, H1_esc_hC) * H_esc_ND];
  cm_cnt2s H1_esc_D[tc_rows(H1_esc_d_Volume, H1_esc_hD) * H_esc_ND];
  cm_cnt H1_esc_MT[tc_rows(H1_esc_t_Volume, H1_esc_hT) * H_esc_MND];
  i32 H1_esc_W[tc_rows(H1_esc_m_Volume, H1_esc_hM) * H_esc_ND * tc_nin(4)];
  u8 H1_esc_WC[tc_rows(H1_esc_m_Volume, H1_esc_hM) * H_esc_ND];
  cm_cnt H1_esc_G[H1_esc_g_Volume * H_esc_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H1_esc_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_esc_a_Volume, H1_esc_hA) * H_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_esc_b_Volume, H1_esc_hB) * H_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_esc_c_Volume, H1_esc_hC) * H_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(H1_esc_d_Volume, H1_esc_hD) * H_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H1_esc_t_Volume, H1_esc_hT) * H_esc_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H1_esc_m_Volume, H1_esc_hM) * H_esc_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H1_esc_m_Volume, H1_esc_hM) * H_esc_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H1_esc_g_Volume * H_esc_NG);
  void H1_esc_Init( void ) {}
  void H1_esc_Quit( void ) {}
};
