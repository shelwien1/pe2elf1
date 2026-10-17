
static const byte H_hdr_a_fld[3+1]={ 0,1,2,3 };
static const byte H_hdr_a_p1[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H_hdr_a_sty[9+1]={ 0,1,2,3,4,5,6,7,8,9 };


static const byte H_hdr_c_fld[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

static const byte H_hdr_d_p1[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte H_hdr_d_p2[7+1]={ 0,1,2,3,4,5,6,7 };


static const byte H_hdr_g_p1[7+1]={ 0,1,2,3,4,5,6,7 };
static const byte H_hdr_g_pos[3+1]={ 0,1,2,3 };






static const int H_hdr_rA = (128+0) * (1);
static const int H_hdr_rB = (0+0) * (1);
static const int H_hdr_rC = (0+0) * (1);
static const int H_hdr_rD = (255+0) * (1);
static const int H_hdr_rT = (55+0) * (1);
static const int H_hdr_rG = (90+0) * (1);
static const int H_hdr_rS1 = (6+0) * (1);
static const int H_hdr_rS2 = (6+0) * (1);
static const int H_hdr_lr = (12+0) * (1);
static const int H_hdr_lrm = (12+0) * (1);
static const int H_hdr_mbm = (0+0) * (1);
static const int H_hdr_mwA = (0+0) * (1);
static const int H_hdr_mwB = (0+0) * (1);
static const int H_hdr_mwC = (0+0) * (1);
static const int H_hdr_mwD = (0+0) * (1);
static const int H_hdr_mwT = (0+0) * (1);
static const int H_hdr_mwG = (0+0) * (1);
static const int H_hdr_bw = (8+0) * (1);
static const int H_hdr_qs = (16+3) * (1);
static const int H_hdr_qf = (16+3) * (1);
static const int H_hdr_mb = (226+0) * (1);
static const int H_hdr_hA = (24+0) * (1);
static const int H_hdr_hB = (24+0) * (1);
static const int H_hdr_hD = (24+0) * (1);
static const int H_hdr_hS = (24+0) * (1);
static const int H_hdr_hF = (24+0) * (1);
static const int H_hdr_hM = (24+0) * (1);
static const int H_hdr_hT = (24+0) * (1);
static const int H_hdr_hN = (24+0) * (1);


static const long long H_hdr_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, (1<<6)), 4), 8), 10); // 20480
static const long long H_hdr_b_Volume = tc_vmul(tc_vmul(1, (1<<8)), (1<<4)); // 4096
static const long long H_hdr_c_Volume = tc_vmul(tc_vmul(1, (1<<8)), 16); // 4096
static const long long H_hdr_d_Volume = tc_vmul(tc_vmul(tc_vmul(1, (1<<8)), 16), 8); // 32768
static const long long H_hdr_f_Volume = tc_vmul(1, (1<<8)); // 256
static const long long H_hdr_g_Volume = tc_vmul(tc_vmul(1, 8), 4); // 32
static const long long H_hdr_m_Volume = 1; // 1
static const long long H_hdr_n_Volume = tc_vmul(1, (1<<8)); // 256
static const long long H_hdr_s_Volume = tc_vmul(1, (1<<8)); // 256
static const long long H_hdr_t_Volume = tc_vmul(1, (1<<8)); // 256

struct H_hdr_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt H_hdr_A[tc_rows(H_hdr_a_Volume, H_hdr_hA) * H_hdr_ND];
  cm_cnt H_hdr_B[tc_rows(H_hdr_b_Volume, H_hdr_hB) * H_hdr_ND];
  cm_cnt H_hdr_C[H_hdr_c_Volume * H_hdr_ND];
  cm_cnt H_hdr_D[tc_rows(H_hdr_d_Volume, H_hdr_hD) * H_hdr_ND];
  cm_cnt H_hdr_MT[tc_rows(H_hdr_t_Volume, H_hdr_hT) * H_hdr_MND];
  i16 H_hdr_S[tc_rows(H_hdr_s_Volume, H_hdr_hS) * H_hdr_ND * H_hdr_qs];
  u8 H_hdr_SC[tc_rows(H_hdr_s_Volume, H_hdr_hS) * H_hdr_ND * H_hdr_qs];
  i16 H_hdr_F[tc_rows(H_hdr_f_Volume, H_hdr_hF) * H_hdr_ND * H_hdr_qf];
  u8 H_hdr_FC[tc_rows(H_hdr_f_Volume, H_hdr_hF) * H_hdr_ND * H_hdr_qf];
  i32 H_hdr_W[tc_rows(H_hdr_m_Volume, H_hdr_hM) * H_hdr_ND * TC_MIXN];
  u8 H_hdr_WC[tc_rows(H_hdr_m_Volume, H_hdr_hM) * H_hdr_ND];
  i32 H_hdr_WM[tc_rows(H_hdr_n_Volume, H_hdr_hN) * H_hdr_MND * 3];
  u8 H_hdr_WMC[tc_rows(H_hdr_n_Volume, H_hdr_hN) * H_hdr_MND];
  i32 H_hdr_W2[tc_rows(H_hdr_d_Volume, H_hdr_hD) * H_hdr_ND * TC_MIXN];
  u8 H_hdr_W2C[tc_rows(H_hdr_d_Volume, H_hdr_hD) * H_hdr_ND];
  i32 H_hdr_WF[H_hdr_ND * 3];
  u8 H_hdr_WFC[H_hdr_ND];
  cm_cnt H_hdr_G[H_hdr_g_Volume * H_hdr_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long H_hdr_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_hdr_a_Volume, H_hdr_hA) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_hdr_b_Volume, H_hdr_hB) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H_hdr_c_Volume * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_hdr_d_Volume, H_hdr_hD) * H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(H_hdr_t_Volume, H_hdr_hT) * H_hdr_MND)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H_hdr_s_Volume, H_hdr_hS) * H_hdr_ND * H_hdr_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_hdr_s_Volume, H_hdr_hS) * H_hdr_ND * H_hdr_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(tc_rows(H_hdr_f_Volume, H_hdr_hF) * H_hdr_ND * H_hdr_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_hdr_f_Volume, H_hdr_hF) * H_hdr_ND * H_hdr_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H_hdr_m_Volume, H_hdr_hM) * H_hdr_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_hdr_m_Volume, H_hdr_hM) * H_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H_hdr_n_Volume, H_hdr_hN) * H_hdr_MND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_hdr_n_Volume, H_hdr_hN) * H_hdr_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(H_hdr_d_Volume, H_hdr_hD) * H_hdr_ND * TC_MIXN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(H_hdr_d_Volume, H_hdr_hD) * H_hdr_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(H_hdr_ND * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(H_hdr_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(H_hdr_g_Volume * H_hdr_NG);
  void H_hdr_Init( void ) {
  
  }
};
