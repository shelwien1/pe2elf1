


static const byte H5_hdr_c_p1[10+1]={ 0,1,2,3,4,5,6,7,7,8,8 };
static const byte H5_hdr_c_p2[6+1]={ 0,1,2,3,4,5,6 };

static const byte H5_hdr_d_fld[7+1]={ 0,1,1,1,2,2,2,3 };
static const byte H5_hdr_d_dm[60+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60 };

static const byte H5_hdr_e_ws[4+1]={ 0,1,1,2,3 };
static const byte H5_hdr_e_ml[7+1]={ 0,0,1,2,3,4,4,4 };

static const byte H5_hdr_g_p1[6+1]={ 0,1,2,3,4,5,5 };

static const byte H5_hdr_h_fld[15+1]={ 0,1,2,3,4,5,6,7,7,8,9,10,11,11,12,13 };
static const byte H5_hdr_h_hx[20+1]={ 0,1,1,2,2,3,4,5,5,5,5,5,5,5,6,7,8,9,9,9,9 };

static const byte H5_hdr_i_pb[8+1]={ 0,1,2,3,3,4,5,5,5 };



static const byte H5_hdr_l_fld[4+1]={ 0,1,2,3,3 };




static const int H5_hdr_rA = (130+0) * (1);
static const int H5_hdr_rB = (87+0) * (1);
static const int H5_hdr_rC = (255+0) * (1);
static const int H5_hdr_rD = (254+0) * (1);
static const int H5_hdr_rE = (255+0) * (1);
static const int H5_hdr_rH = (254+0) * (1);
static const int H5_hdr_rI = (244+0) * (1);
static const int H5_hdr_rJ = (0+0) * (1);
static const int H5_hdr_rK = (223+0) * (1);
static const int H5_hdr_rL = (224+0) * (1);
static const int H5_hdr_rT = (140+0) * (1);
static const int H5_hdr_rG = (64+0) * (1);
static const int H5_hdr_lr = (15+0) * (1);
static const int H5_hdr_mwA = (0+0) * (1);
static const int H5_hdr_mwB = (0+0) * (1);
static const int H5_hdr_mwC = (0+0) * (1);
static const int H5_hdr_mwD = (0+0) * (1);
static const int H5_hdr_mwE = (0+0) * (1);
static const int H5_hdr_mwH = (0+0) * (1);
static const int H5_hdr_mwI = (0+0) * (1);
static const int H5_hdr_mwJ = (0+0) * (1);
static const int H5_hdr_mwK = (16+0) * (1);
static const int H5_hdr_mwL = (2+0) * (1);
static const int H5_hdr_mwT = (0+0) * (1);
static const int H5_hdr_mwG = (0+0) * (1);
static const int H5_hdr_mt = (0+0) * (1);
static const int H5_hdr_mb = (32+0) * (1);
static const int H5_hdr_hA = (24+0) * (1);
static const int H5_hdr_hB = (24+0) * (1);
static const int H5_hdr_hC = (24+0) * (1);
static const int H5_hdr_hD = (24+0) * (1);
static const int H5_hdr_hE = (24+0) * (1);
static const int H5_hdr_hH = (24+0) * (1);
static const int H5_hdr_hI = (24+0) * (1);
static const int H5_hdr_hJ = (24+0) * (1);
static const int H5_hdr_hK = (24+0) * (1);
static const int H5_hdr_hL = (24+0) * (1);
static const int H5_hdr_hM = (24+0) * (1);
static const int H5_hdr_hT = (24+0) * (1);


static const long long H5_hdr_a_Volume = tc_vmul(tc_vmul(1, (1<<7)), 1); // 128
static const long long H5_hdr_b_Volume = tc_vmul(tc_vmul(1, (1<<7)), (1<<5)); // 4096
static const long long H5_hdr_c_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 9), 7); // 16128
static const long long H5_hdr_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<2)), 4), 1), 61); // 976
static const long long H5_hdr_e_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), 4), 3), 5); // 3840
static const long long H5_hdr_g_Volume = tc_vmul(1, 6); // 6
static const long long H5_hdr_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 3), 14), 10); // 107520
static const long long H5_hdr_i_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), 6), (1<<4)); // 6144
static const long long H5_hdr_j_Volume = tc_vmul(tc_vmul(1, 2), 2); // 4
static const long long H5_hdr_k_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<3)), 3), (1<<4)); // 384
static const long long H5_hdr_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<7)), (1<<6)), 4); // 32768
static const long long H5_hdr_m_Volume = tc_vmul(1, (1<<8)); // 256
static const long long H5_hdr_t_Volume = tc_vmul(1, (1<<4)); // 16

#define H5_hdr_USES(X) X(tag) X(p1r) X(p1) X(p2) X(fld) X(dm) X(ws) X(ml) X(ch) X(hx) X(pb) X(pos) X(nb)

struct H5_hdr_T {
  cm_cnt2 H5_hdr_A[tc_rows(H5_hdr_a_Volume, H5_hdr_hA) * H_hdr_ND];
  cm_cnt2 H5_hdr_B[tc_rows(H5_hdr_b_Volume, H5_hdr_hB) * H_hdr_ND];
  cm_cnt2 H5_hdr_C[tc_rows(H5_hdr_c_Volume, H5_hdr_hC) * H_hdr_ND];
  cm_cnt2 H5_hdr_D[tc_rows(H5_hdr_d_Volume, H5_hdr_hD) * H_hdr_ND];
  cm_cnt2 H5_hdr_E[tc_rows(H5_hdr_e_Volume, H5_hdr_hE) * H_hdr_ND];
  cm_cnt2 H5_hdr_H[tc_rows(H5_hdr_h_Volume, H5_hdr_hH) * H_hdr_ND];
  cm_cnt2 H5_hdr_I[tc_rows(H5_hdr_i_Volume, H5_hdr_hI) * H_hdr_ND];
  cm_cnt2 H5_hdr_J[tc_rows(H5_hdr_j_Volume, H5_hdr_hJ) * H_hdr_ND];
  cm_cnt2 H5_hdr_K[tc_rows(H5_hdr_k_Volume, H5_hdr_hK) * H_hdr_ND];
  cm_cnt2 H5_hdr_L[tc_rows(H5_hdr_l_Volume, H5_hdr_hL) * H_hdr_ND];
  cm_cnt H5_hdr_MT[tc_rows(H5_hdr_t_Volume, H5_hdr_hT) * H_hdr_MND];
  i32 H5_hdr_W[tc_rows(H5_hdr_m_Volume, H5_hdr_hM) * H_hdr_ND * tc_nin(10)];
  u8 H5_hdr_WC[tc_rows(H5_hdr_m_Volume, H5_hdr_hM) * H_hdr_ND];
  cm_cnt H5_hdr_G[H5_hdr_g_Volume * H_hdr_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H5_hdr_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_hdr_a_Volume, H5_hdr_hA) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_hdr_b_Volume, H5_hdr_hB) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_hdr_c_Volume, H5_hdr_hC) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_hdr_d_Volume, H5_hdr_hD) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_hdr_e_Volume, H5_hdr_hE) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_hdr_h_Volume, H5_hdr_hH) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_hdr_i_Volume, H5_hdr_hI) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_hdr_j_Volume, H5_hdr_hJ) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_hdr_k_Volume, H5_hdr_hK) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H5_hdr_l_Volume, H5_hdr_hL) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H5_hdr_t_Volume, H5_hdr_hT) * H_hdr_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H5_hdr_m_Volume, H5_hdr_hM) * H_hdr_ND * tc_nin(10))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H5_hdr_m_Volume, H5_hdr_hM) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H5_hdr_g_Volume * H_hdr_NG);
  void H5_hdr_Init( void ) {}
  void H5_hdr_Quit( void ) {}
};
