
static const byte H3_sig_a_cg[13+1]={ 0,1,1,2,3,4,4,4,4,4,4,4,4,4 };
static const byte H3_sig_a_pos[63+1]={ 0,0,1,1,1,2,3,4,5,6,7,8,9,10,11,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,26,27,28,29,30,31,31,32,33,34,34,35,35,36,36,37,38,38,38,39,40,41,41,41,42,42,43,44,45,46,47,48,48 };
static const byte H3_sig_a_rem[10+1]={ 0,0,1,1,2,3,3,4,5,6,6 };

static const byte H3_sig_b_cg[13+1]={ 0,1,1,2,3,4,4,4,4,4,4,4,4,4 };
static const byte H3_sig_b_pos[63+1]={ 0,0,0,1,1,2,3,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,23,23,24,25,26,27,28,29,30,30,31,32,33,33,33,33,34,35,36,37,38,39,39,40,41,42,43,44,44,45,45,46,46,47,48,48 };
static const byte H3_sig_b_md[19+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,12,13,14,15,16,17,17 };
static const byte H3_sig_b_hb[510+1]={ 0,1,2,3,4,5,6,6,6,7,8,9,10,11,11,12,13,14,15,16,17,18,18,19,19,20,21,21,22,23,23,23,23,23,23,24,24,25,26,26,26,26,26,27,28,28,28,29,30,31,32,32,32,33,34,35,35,35,35,35,35,35,35,35,35,36,36,37,37,38,38,39,39,39,39,40,40,41,41,42,43,43,43,44,44,45,45,45,45,45,45,46,46,46,46,47,47,48,48,49,50,50,50,51,51,52,53,54,54,54,55,56,56,56,57,58,59,59,60,61,61,62,63,64,65,66,67,67,67,67,68,69,70,71,72,73,73,73,73,73,73,73,73,74,75,75,75,76,76,77,77,78,78,78,78,78,78,78,78,79,79,80,80,81,81,81,81,82,83,84,84,85,85,86,86,87,87,88,88,89,90,90,90,91,91,91,91,92,93,94,95,96,97,97,98,99,100,101,101,102,103,103,103,103,103,103,103,103,103,104,105,106,107,107,108,109,110,111,111,111,111,112,113,114,114,114,114,115,115,115,115,116,117,117,117,118,118,118,118,118,118,118,118,118,118,118,118,119,119,119,119,119,120,121,122,122,122,123,123,124,124,124,124,125,125,126,126,127,127,128,128,129,130,131,132,132,132,133,134,135,135,135,135,135,136,137,137,138,139,139,139,139,139,139,139,140,140,141,141,142,142,143,143,144,144,145,145,145,145,145,146,147,147,147,147,147,147,147,148,149,149,149,149,150,151,152,152,153,154,154,154,155,156,157,157,158,159,160,161,162,163,164,164,165,165,165,165,166,166,166,167,168,168,168,168,168,168,168,169,170,170,170,170,171,171,172,172,173,173,173,173,173,174,175,176,177,177,177,177,177,178,179,180,181,182,182,182,182,183,183,183,184,184,185,185,186,186,187,187,188,188,188,188,188,189,190,190,191,191,191,191,191,192,192,193,194,194,194,194,194,194,194,195,196,196,197,197,198,198,199,199,199,200,200,200,200,201,202,203,204,204,204,205,206,206,206,207,208,208,208,208,208,208,208,208,209,209,209,209,209,209,209,210,211,211,211,212,213,213,213,213,214,214,215,215,216,216,216,217,218,218,218,218,219,219,219,219,220,220,220,220,221,221,222,223,224,224,224,224,225,225,225,226,227,227,228,229,229,230,231,232 };

static const byte H3_sig_c_pos[63+1]={ 0,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,3,4,4,4,4,4,4,5,6,7,7,7,7,8,9,10,11,11,11,11,11,11,12,13,14,14,14,14,15,15,16,16,16,17,17,17,18,18,19,20,21,21,21,22,22,23,23 };
static const byte H3_sig_c_q6[8+1]={ 0,0,1,2,3,4,5,6,6 };
static const byte H3_sig_c_ft[8+1]={ 0,1,2,3,3,4,5,5,6 };

static const byte H3_sig_d_cg[13+1]={ 0,1,2,3,3,4,4,4,4,4,4,4,4,4 };
static const byte H3_sig_d_rem[10+1]={ 0,0,1,2,3,3,4,4,4,5,5 };
static const byte H3_sig_d_zl[10+1]={ 0,0,1,2,3,4,5,6,7,7,7 };
static const word H3_sig_d_hb[510+1]={ 0,1,1,2,2,3,3,4,4,4,4,4,4,4,4,4,5,6,7,8,9,10,11,12,12,12,12,12,12,13,13,14,15,16,17,18,19,19,19,20,21,21,21,22,22,23,24,25,26,27,28,29,30,31,31,32,32,32,33,34,34,34,34,35,36,37,38,39,39,40,40,41,41,42,42,43,43,44,44,45,45,46,47,48,48,48,49,50,50,50,50,51,51,51,51,52,53,54,55,56,56,57,58,59,59,60,61,62,62,63,64,64,64,65,66,67,67,68,69,70,70,70,70,70,70,70,71,72,73,74,75,76,77,78,79,80,80,80,80,80,80,81,82,83,83,83,84,85,85,85,86,87,87,87,88,89,89,89,89,90,90,91,91,92,93,94,95,96,97,97,97,98,98,99,99,100,101,102,102,102,102,102,102,103,103,104,105,106,106,107,108,108,109,110,111,112,112,112,113,113,114,114,114,114,114,115,116,116,117,117,118,118,118,118,119,119,120,121,121,122,122,123,123,124,124,124,125,125,126,127,128,129,129,130,130,131,131,131,131,131,131,131,132,133,134,134,134,135,135,135,136,137,138,139,139,140,141,142,143,144,144,145,146,147,147,148,149,150,150,151,152,153,153,154,155,156,157,158,159,160,160,160,161,162,163,164,165,166,166,166,167,168,168,169,170,171,171,172,172,173,174,174,174,175,175,175,175,176,176,177,177,178,178,178,178,179,180,180,180,181,181,182,182,183,184,185,185,186,187,187,187,188,188,189,190,191,191,192,193,194,195,196,197,198,198,198,199,200,200,201,202,203,203,204,204,204,205,205,205,206,206,207,207,208,209,209,209,210,210,210,211,212,212,212,213,214,214,214,214,215,215,215,215,216,216,217,218,218,218,219,219,220,221,222,223,224,224,224,225,226,226,226,227,228,229,230,231,232,233,234,235,236,236,236,236,237,237,238,239,240,240,241,241,242,242,242,243,244,244,244,245,245,245,246,247,248,248,249,250,251,251,251,252,252,253,254,255,256,256,256,256,257,257,258,258,259,259,259,259,260,260,261,262,263,263,263,263,263,263,264,265,266,266,266,267,267,268,269,270,271,272,272,272,272,272,273,274,274,275,276,277,278,278,279,280,281,281,282,282,283,283,283,283,284,284,285,286,286,287,288,288 };

static const byte H3_sig_e_pos[63+1]={ 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,3,4,5,5,6,6,6,6,6,6,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,10,11,11,11,11,11,11,12,13,13 };


static const byte H3_sig_h_cg[13+1]={ 0,1,2,2,3,4,4,4,4,4,4,4,4,4 };
static const byte H3_sig_h_pos[63+1]={ 0,0,0,0,1,2,2,2,3,4,5,6,7,8,9,9,9,10,10,11,12,12,12,13,13,13,14,15,16,17,18,18,19,20,21,21,21,22,23,23,24,25,26,27,27,28,28,29,29,30,31,32,32,33,34,34,34,35,35,36,37,38,38,38 };
static const byte H3_sig_h_rem[10+1]={ 0,0,0,1,1,1,2,2,2,3,4 };
static const byte H3_sig_h_h3[14+1]={ 0,1,1,2,2,2,2,3,3,3,3,3,3,3,3 };

static const byte H3_sig_i_cg[13+1]={ 0,1,2,3,4,5,5,5,5,5,5,5,5,5 };
static const byte H3_sig_i_pos[63+1]={ 0,0,1,2,2,3,4,5,6,7,8,9,10,11,12,12,13,13,14,14,14,15,16,16,16,16,16,16,17,18,18,19,20,21,21,22,22,22,23,24,25,25,26,26,26,27,27,28,29,29,29,29,29,29,29,29,29,30,31,32,33,34,35,35 };

static const byte H3_sig_j_pos[63+1]={ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,2,3,3,3,3,4,4,4,5,5,6,7,7,7,8,8,9,9,10,10,10,10,11,12,13,13,13,13,14,15,16,16,17,17,18,19,20,21,22,22,22 };
static const byte H3_sig_j_ft[8+1]={ 0,0,0,1,2,3,4,4,5 };

static const byte H3_sig_k_cg[13+1]={ 0,1,2,3,4,5,5,5,5,5,5,5,5,5 };
static const byte H3_sig_k_pos[63+1]={ 0,0,0,1,2,3,4,5,6,7,8,9,10,11,12,12,13,14,14,14,15,15,16,17,17,17,17,18,18,19,20,21,22,23,23,24,25,25,26,27,27,28,29,30,31,32,32,33,34,35,36,36,36,36,36,37,38,38,38,38,38,39,39,39 };
static const byte H3_sig_k_rem[10+1]={ 0,0,1,2,3,4,5,6,6,6,6 };
static const byte H3_sig_k_h3[14+1]={ 0,1,2,3,4,5,6,6,7,8,8,9,10,11,12 };
static const byte H3_sig_k_sn[8+1]={ 0,0,1,1,2,3,4,5,6 };

static const byte H3_sig_l_cg[13+1]={ 0,1,2,3,3,4,4,4,4,4,4,4,4,4 };
static const byte H3_sig_l_pos[63+1]={ 0,0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,19,20,21,22,22,23,24,25,26,27,28,28,29,30,31,32,32,33,34,35,36,36,37,37,38,38,39,39,40,40,41,41,42,43,44,45,46,47,48,48,49,49 };
static const byte H3_sig_l_sn[8+1]={ 0,0,0,0,1,2,3,4,5 };

static const byte H3_sig_m_cg[13+1]={ 0,1,2,3,4,5,5,5,5,5,5,5,5,5 };
static const byte H3_sig_m_pos[9+1]={ 0,1,2,3,4,5,6,7,7,8 };
static const byte H3_sig_m_rem[10+1]={ 0,0,1,2,2,2,3,3,3,4,4 };




static const byte H3_sig_w_rem[10+1]={ 0,0,1,1,1,2,2,2,3,3,4 };
static const byte H3_sig_w_zl[10+1]={ 0,0,1,1,2,2,2,3,4,4,5 };


static const int H3_sig_rA = (224+0) * (1);
static const int H3_sig_rB = (253+0) * (1);
static const int H3_sig_rC = (254+0) * (1);
static const int H3_sig_rE = (157+0) * (1);
static const int H3_sig_rH = (0+0) * (1);
static const int H3_sig_rI = (123+0) * (1);
static const int H3_sig_rJ = (255+0) * (1);
static const int H3_sig_rK = (253+0) * (1);
static const int H3_sig_rL = (253+0) * (1);
static const int H3_sig_rD = (254+0) * (1);
static const int H3_sig_rT = (0+0) * (1);
static const int H3_sig_rG = (0+0) * (1);
static const int H3_sig_lr = (10+0) * (1);
static const int H3_sig_lrm = (0+0) * (1);
static const int H3_sig_mbm = (0+0) * (1);
static const int H3_sig_mwA = (63+0) * (1);
static const int H3_sig_mwB = (9+0) * (1);
static const int H3_sig_mwC = (67+0) * (1);
static const int H3_sig_mwE = (80+0) * (1);
static const int H3_sig_mwH = (127+0) * (1);
static const int H3_sig_mwI = (95+0) * (1);
static const int H3_sig_mwJ = (76+0) * (1);
static const int H3_sig_mwK = (22+0) * (1);
static const int H3_sig_mwL = (96+0) * (1);
static const int H3_sig_mwD = (64+0) * (1);
static const int H3_sig_mwT = (0+0) * (1);
static const int H3_sig_mwG = (0+0) * (1);
static const int H3_sig_mt = (0+0) * (1);
static const int H3_sig_mb = (122+0) * (1);
static const int H3_sig_hA = (24+0) * (1);
static const int H3_sig_hB = (21+0) * (1);
static const int H3_sig_hC = (24+0) * (1);
static const int H3_sig_hE = (21+0) * (1);
static const int H3_sig_hH = (20+0) * (1);
static const int H3_sig_hI = (24+0) * (1);
static const int H3_sig_hJ = (24+0) * (1);
static const int H3_sig_hK = (21+0) * (1);
static const int H3_sig_hL = (21+0) * (1);
static const int H3_sig_hD = (24+0) * (1);
static const int H3_sig_hS = (24+0) * (1);
static const int H3_sig_hM = (24+0) * (1);
static const int H3_sig_hT = (24+0) * (1);
static const int H3_sig_hN = (24+0) * (1);


static const long long H3_sig_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, 5), 49), 7); // 1715
static const long long H3_sig_b_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 5), 49), 18), 233); // 1027530
static const long long H3_sig_c_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 3), 24), 7), 7); // 3528
static const long long H3_sig_d_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 5), 6), 8), 289); // 69360
static const long long H3_sig_e_Volume = tc_vmul(tc_vmul(1, 1), 14); // 14
static const long long H3_sig_g_Volume = tc_vmul(1, 1); // 1
static const long long H3_sig_h_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 5), 39), 5), 4); // 3900
static const long long H3_sig_i_Volume = tc_vmul(tc_vmul(1, 6), 36); // 216
static const long long H3_sig_j_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 1), 23), 6), 2), 1); // 276
static const long long H3_sig_k_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, 6), 40), 7), 13), 7); // 152880
static const long long H3_sig_l_Volume = tc_vmul(tc_vmul(tc_vmul(1, 5), 50), 6); // 1500
static const long long H3_sig_m_Volume = tc_vmul(tc_vmul(tc_vmul(1, 6), 9), 5); // 270
static const long long H3_sig_n_Volume = tc_vmul(1, 1); // 1
static const long long H3_sig_s_Volume = tc_vmul(1, 2); // 2
static const long long H3_sig_t_Volume = tc_vmul(1, 1); // 1
static const long long H3_sig_w_Volume = tc_vmul(tc_vmul(tc_vmul(1, 2), 5), 6); // 60

#define H3_sig_USES(X) X(cg) X(pos) X(rem) X(md) X(hb) X(q6) X(ft) X(zl) X(h3) X(sn)

struct H3_sig_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt2 H3_sig_A[tc_rows(H3_sig_a_Volume, H3_sig_hA) * H_sig_ND];
  cm_cnt2 H3_sig_B[tc_rows(H3_sig_b_Volume, H3_sig_hB) * H_sig_ND];
  cm_cnt2 H3_sig_C[tc_rows(H3_sig_c_Volume, H3_sig_hC) * H_sig_ND];
  cm_cnt2 H3_sig_D[tc_rows(H3_sig_d_Volume, H3_sig_hD) * H_sig_ND];
  cm_cnt2 H3_sig_E[tc_rows(H3_sig_e_Volume, H3_sig_hE) * H_sig_ND];
  cm_cnt2 H3_sig_H[tc_rows(H3_sig_h_Volume, H3_sig_hH) * H_sig_ND];
  cm_cnt2 H3_sig_I[tc_rows(H3_sig_i_Volume, H3_sig_hI) * H_sig_ND];
  cm_cnt2 H3_sig_J[tc_rows(H3_sig_j_Volume, H3_sig_hJ) * H_sig_ND];
  cm_cnt2 H3_sig_K[tc_rows(H3_sig_k_Volume, H3_sig_hK) * H_sig_ND];
  cm_cnt2 H3_sig_L[tc_rows(H3_sig_l_Volume, H3_sig_hL) * H_sig_ND];
  cm_cnt H3_sig_MT[tc_rows(H3_sig_t_Volume, H3_sig_hT) * H_sig_MND];
  i32 H3_sig_W[tc_rows(H3_sig_m_Volume, H3_sig_hM) * H_sig_ND * 24];
  u8 H3_sig_WC[tc_rows(H3_sig_m_Volume, H3_sig_hM) * H_sig_ND];
  i32 H3_sig_WM[tc_rows(H3_sig_n_Volume, H3_sig_hN) * H_sig_MND * 3];
  u8 H3_sig_WMC[tc_rows(H3_sig_n_Volume, H3_sig_hN) * H_sig_MND];
  i32 H3_sig_W2[H3_sig_w_Volume * H_sig_ND * 24];
  u8 H3_sig_W2C[H3_sig_w_Volume * H_sig_ND];
  i32 H3_sig_W3[tc_rows(H3_sig_s_Volume, H3_sig_hS) * H_sig_ND * 24];
  u8 H3_sig_W3C[tc_rows(H3_sig_s_Volume, H3_sig_hS) * H_sig_ND];
  i32 H3_sig_WF[H_sig_ND * 5 * 4];
  u8 H3_sig_WFC[H_sig_ND * 5];
  cm_cnt H3_sig_G[H3_sig_g_Volume * H_sig_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H3_sig_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sig_a_Volume, H3_sig_hA) * H_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sig_b_Volume, H3_sig_hB) * H_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sig_c_Volume, H3_sig_hC) * H_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sig_d_Volume, H3_sig_hD) * H_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sig_e_Volume, H3_sig_hE) * H_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sig_h_Volume, H3_sig_hH) * H_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sig_i_Volume, H3_sig_hI) * H_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sig_j_Volume, H3_sig_hJ) * H_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sig_k_Volume, H3_sig_hK) * H_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt2) * (unsigned long long)(tc_rows(H3_sig_l_Volume, H3_sig_hL) * H_sig_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H3_sig_t_Volume, H3_sig_hT) * H_sig_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_sig_m_Volume, H3_sig_hM) * H_sig_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_sig_m_Volume, H3_sig_hM) * H_sig_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_sig_n_Volume, H3_sig_hN) * H_sig_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_sig_n_Volume, H3_sig_hN) * H_sig_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H3_sig_w_Volume * H_sig_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H3_sig_w_Volume * H_sig_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H3_sig_s_Volume, H3_sig_hS) * H_sig_ND * 24)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H3_sig_s_Volume, H3_sig_hS) * H_sig_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_sig_ND * 5 * 4)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_sig_ND * 5)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H3_sig_g_Volume * H_sig_NG);
  void H3_sig_Init( void ) {
  
  }
  void H3_sig_Quit( void ) {}
};
