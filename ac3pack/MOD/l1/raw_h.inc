
static const byte A1_raw_a_code[6+1]={ 0,1,2,3,4,5,6 };

static const byte A1_raw_b_code[6+1]={ 0,1,2,3,4,5,6 };

static const byte A1_raw_c_code[6+1]={ 0,1,2,3,4,5,6 };
static const word A1_raw_c_cl[256+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,256 };



static const byte A1_raw_m_code[6+1]={ 0,1,2,3,4,5,6 };




static const int A1_raw_rA = (255+0) * (1);
static const int A1_raw_mwA = (4+0) * (1);
static const int A1_raw_rB = (255+0) * (1);
static const int A1_raw_mwB = (4+0) * (1);
static const int A1_raw_rC = (255+0) * (1);
static const int A1_raw_mwC = (4+0) * (1);
static const int A1_raw_rD = (255+0) * (1);
static const int A1_raw_mwD = (4+0) * (1);
static const int A1_raw_lr = (15+0) * (1);
static const int A1_raw_mb = (51+0) * (1);
static const int A1_raw_rT = (0+0) * (1);
static const int A1_raw_mwT = (0+0) * (1);
static const int A1_raw_mt = (0+0) * (1);
static const int A1_raw_lrm = (0+0) * (1);
static const int A1_raw_mbm = (0+0) * (1);
static const int A1_raw_rG = (0+0) * (1);
static const int A1_raw_mwG = (0+0) * (1);
static const int A1_raw_hA = (20+0) * (1);
static const int A1_raw_hB = (20+0) * (1);
static const int A1_raw_hC = (20+0) * (1);
static const int A1_raw_hD = (20+0) * (1);
static const int A1_raw_hM = (20+0) * (1);
static const int A1_raw_hT = (20+0) * (1);
static const int A1_raw_hN = (20+0) * (1);


static const long long A1_raw_a_Volume = tc_vmul(tc_vmul(1, 7), (1<<8)); // 1792
static const long long A1_raw_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, 7), (1<<8)), (1<<4)); // 28672
static const long long A1_raw_c_Volume = tc_vmul(tc_vmul(1, 7), 257); // 1799
static const long long A1_raw_d_Volume = tc_vmul(tc_vmul(1, (1<<8)), (1<<8)); // 65536
static const long long A1_raw_g_Volume = 1; // 1
static const long long A1_raw_m_Volume = tc_vmul(1, 7); // 7
static const long long A1_raw_n_Volume = 1; // 1
static const long long A1_raw_t_Volume = 1; // 1

#define A1_raw_USES(X) X(code) X(c1) X(c2) X(cl)

struct A1_raw_T {
  cm_cnt2s A1_raw_A[tc_rows(A1_raw_a_Volume, A1_raw_hA) * M_raw_ND];
  cm_cnt2s A1_raw_B[tc_rows(A1_raw_b_Volume, A1_raw_hB) * M_raw_ND];
  cm_cnt2s A1_raw_C[tc_rows(A1_raw_c_Volume, A1_raw_hC) * M_raw_ND];
  cm_cnt2s A1_raw_D[tc_rows(A1_raw_d_Volume, A1_raw_hD) * M_raw_ND];
  cm_cnt A1_raw_MT[tc_rows(A1_raw_t_Volume, A1_raw_hT) * M_raw_MND];
  i32 A1_raw_W[tc_rows(A1_raw_m_Volume, A1_raw_hM) * M_raw_ND * tc_nin(4)];
  u8 A1_raw_WC[tc_rows(A1_raw_m_Volume, A1_raw_hM) * M_raw_ND];
  i32 A1_raw_WM[tc_rows(A1_raw_n_Volume, A1_raw_hN) * M_raw_MND * TC_MIN];
  u8 A1_raw_WMC[tc_rows(A1_raw_n_Volume, A1_raw_hN) * M_raw_MND];
  cm_cnt A1_raw_G[A1_raw_g_Volume * M_raw_NG];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long A1_raw_Size = 0
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_raw_a_Volume, A1_raw_hA) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_raw_b_Volume, A1_raw_hB) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_raw_c_Volume, A1_raw_hC) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt2s) * (unsigned long long)(tc_rows(A1_raw_d_Volume, A1_raw_hD) * M_raw_ND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(tc_rows(A1_raw_t_Volume, A1_raw_hT) * M_raw_MND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(A1_raw_m_Volume, A1_raw_hM) * M_raw_ND * tc_nin(4))
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(A1_raw_m_Volume, A1_raw_hM) * M_raw_ND)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(tc_rows(A1_raw_n_Volume, A1_raw_hN) * M_raw_MND * TC_MIN)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(tc_rows(A1_raw_n_Volume, A1_raw_hN) * M_raw_MND)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(A1_raw_g_Volume * M_raw_NG);
  void A1_raw_Init( void ) {}
  void A1_raw_Quit( void ) {}
};
