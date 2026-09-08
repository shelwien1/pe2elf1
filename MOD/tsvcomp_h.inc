

















static const byte TC_dig_b_pq[32+1]={ 0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8 };


static const byte TC_dig_d_pq[32+1]={ 0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8 };


static const byte TC_dig_m_pq[32+1]={ 0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8 };



static const byte TC_flr_a_col[48+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,28,28,28,28,28,29,29,29,30,31,32,32,32,32,32,32,32,32,32,33 };

static const byte TC_flr_b_p1[14+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,12,12 };






static const byte TC_flr_t_col[48+1]={ 0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,19,20,21,22,23,24,25,25,25,25,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26 };









static const byte TC_sgn_a_sq[32+1]={ 0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8 };




static const byte TC_sgn_f_sq[32+1]={ 0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8 };

static const byte TC_sgn_m_sq[32+1]={ 0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8 };




static const int TC_dig_avD = (239+0) * (1);
static const int TC_dig_avA = (7+0) * (1);
static const int TC_dig_avT1 = (0+0) * (1);
static const int TC_dig_avT2 = (0+0) * (1);
static const int TC_dig_avN1 = (5+0) * (1);
static const int TC_dig_avW1 = (4+0) * (1);
static const int TC_dig_avQ1 = (7+0) * (1);
static const int TC_dig_avQ2 = (5+0) * (1);
static const int TC_dig_avC = (430+0) * (1);
static const int TC_dig_rA = (255+0) * (1);
static const int TC_dig_rB = (254+0) * (1);
static const int TC_dig_rC = (15+0) * (1);
static const int TC_dig_rD = (229+0) * (1);
static const int TC_dig_rS = (7+0) * (1);
static const int TC_dig_lr = (11+0) * (1);
static const int TC_sgn_rA = (254+0) * (1);
static const int TC_sgn_rB = (255+0) * (1);
static const int TC_sgn_rC = (10+0) * (1);
static const int TC_sgn_rD = (11+0) * (1);
static const int TC_sgn_rS = (7+0) * (1);
static const int TC_sgn_lr = (18+0) * (1);
static const int TC_flr_rA = (15+0) * (1);
static const int TC_flr_rB = (75+0) * (1);
static const int TC_flr_rC = (63+0) * (1);
static const int TC_flr_rD = (63+0) * (1);
static const int TC_flr_rS = (6+0) * (1);
static const int TC_flr_lr = (22+0) * (1);
static const int TC_cls_rA = (80+0) * (1);
static const int TC_cls_rB = (43+0) * (1);
static const int TC_cls_rC = (63+0) * (1);
static const int TC_cls_rD = (63+0) * (1);
static const int TC_cls_rS = (5+0) * (1);
static const int TC_cls_lr = (24+0) * (1);
static const int TC_aux_rA = (22+0) * (1);
static const int TC_aux_rB = (0+0) * (1);
static const int TC_aux_rC = (63+0) * (1);
static const int TC_aux_rD = (63+0) * (1);
static const int TC_aux_rS = (6+0) * (1);
static const int TC_aux_lr = (31+0) * (1);
static const int TC_hdr_rA = (16+0) * (1);
static const int TC_hdr_rB = (32+0) * (1);
static const int TC_hdr_rC = (63+0) * (1);
static const int TC_hdr_rD = (63+0) * (1);
static const int TC_hdr_rS = (1+0) * (1);
static const int TC_hdr_lr = (31+0) * (1);


static const int TC_aux_a_Volume = 1* 32* 6* 3; // 576
static const int TC_aux_b_Volume = 1* 32* 1* 1; // 32
static const int TC_aux_c_Volume = 1* 32; // 32
static const int TC_aux_d_Volume = 1* 32; // 32
static const int TC_aux_f_Volume = 1* 32; // 32
static const int TC_aux_m_Volume = 1* 32; // 32
static const int TC_aux_s_Volume = 1* 32; // 32
static const int TC_aux_t_Volume = 1* 32; // 32
static const int TC_cls_a_Volume = 1* 2* 8* 5* 5* 2* 1* 1* 1; // 800
static const int TC_cls_b_Volume = 1* 1* 8* 6* 4* 2* 1* 1* 1; // 384
static const int TC_cls_c_Volume = 1* 1* 4* 4* 4* 1* 1; // 64
static const int TC_cls_d_Volume = 1* 4* 4* 1* 1* 4* 1; // 64
static const int TC_cls_f_Volume = 1* 1* 1* 1* 1* 1* 1; // 1
static const int TC_cls_m_Volume = 1* 2* 3* 2* 1* 1* 1; // 12
static const int TC_cls_s_Volume = 1* 1* 1* 1* 1* 1* 1; // 1
static const int TC_cls_t_Volume = 1* 3* 2* 3* 1* 1* 1; // 18
static const int TC_dig_a_Volume = 1* 1* 8* 5* 3* 1* 1* 1* 3* 6* 1* 2* 1* 1* 1* 1* 1* 1* 1* 1* 1* 1* 2* 1; // 8640
static const int TC_dig_b_Volume = 1* 1* 3* 4* 4* 1* 2* 1* 3* 1* 2* 2* 1* 1* 1* 1* 1* 1* 1* 2* 1* 2* 1* 9; // 41472
static const int TC_dig_c_Volume = 1* 3* 1* 3* 1* 1* 1* 1* 3* 1* 2* 1* 1* 1; // 54
static const int TC_dig_d_Volume = 1* 1* 1* 1* 1* 1* 1* 1* 4* 6* 4* 3* 6* 9; // 15552
static const int TC_dig_f_Volume = 1* 2* 1* 6* 2* 4* 1* 4* 2* 1; // 768
static const int TC_dig_m_Volume = 1* 4* 1* 1* 1* 2* 1* 1* 1* 1* 1* 1* 1* 1* 1* 1* 1* 1* 9; // 72
static const int TC_dig_s_Volume = 1* 1* 4* 2* 2* 3* 1* 1* 2* 3* 2* 1* 1* 1* 4* 3* 1* 8* 1; // 55296
static const int TC_dig_t_Volume = 1* 4* 1* 1* 1* 1* 6* 1* 1* 1* 1* 1* 1* 1* 2* 1* 1* 1* 1; // 48
static const int TC_flr_a_Volume = 1* 2* 34* 1* 2* 1* 1* 1* 1* 1* 1; // 136
static const int TC_flr_b_Volume = 1* 1* 2* 13* 2* 1* 2* 1* 1* 1* 1; // 104
static const int TC_flr_c_Volume = 1* 8* 1* 1* 8* 1* 1* 1; // 64
static const int TC_flr_d_Volume = 1* 1* 1* 1* 1* 1* 1* 1; // 1
static const int TC_flr_f_Volume = 1* 1* 1* 1* 1* 1* 1* 1; // 1
static const int TC_flr_m_Volume = 1* 4* 2* 1* 1* 1* 1* 1; // 8
static const int TC_flr_s_Volume = 1* 6* 1* 3* 1* 1* 1* 1; // 18
static const int TC_flr_t_Volume = 1* 27* 1* 1* 1* 1* 1* 1; // 27
static const int TC_hdr_a_Volume = 1* 64* 5* 8* 3; // 7680
static const int TC_hdr_b_Volume = 1* 3* 1* (1<<5); // 96
static const int TC_hdr_c_Volume = 1* 64; // 64
static const int TC_hdr_d_Volume = 1* 64; // 64
static const int TC_hdr_f_Volume = 1* 64; // 64
static const int TC_hdr_m_Volume = 1* 64; // 64
static const int TC_hdr_s_Volume = 1* 64; // 64
static const int TC_hdr_t_Volume = 1* 64* (1<<6); // 4096
static const int TC_sgn_a_Volume = 1* 1* 1* 5* 6* 1* 2* 1* 1* 3* 1* 9; // 1620
static const int TC_sgn_b_Volume = 1* 3* 2* 1* 6* 3* 1* 3* 3* 3* 1* 1; // 2916
static const int TC_sgn_c_Volume = 1* 2* 1* 3* 2* 1* 2* 1* 1* 1; // 24
static const int TC_sgn_d_Volume = 1* 1* 1* 4* 2* 1* 2* 1* 1* 1; // 16
static const int TC_sgn_f_Volume = 1* 3* 2* 6* 3* 1* 9; // 972
static const int TC_sgn_m_Volume = 1* 3* 1* 1* 2* 1* 9; // 54
static const int TC_sgn_s_Volume = 1* 1* 1* 3* 1* 1* 1; // 3
static const int TC_sgn_t_Volume = 1* 1* 1* 1; // 1

struct TC_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt TC_dig_A[TC_dig_a_Volume * TC_NODE];
  cm_cnt TC_dig_B[TC_dig_b_Volume * TC_NODE];
  cm_cnt TC_dig_C[TC_dig_c_Volume * TC_NODE];
  cm_cnt TC_dig_D[TC_dig_d_Volume * TC_NODE];
  cm_cnt TC_dig_T[TC_dig_t_Volume * TC_MNODE];
  u16 TC_dig_S[TC_dig_s_Volume * TC_NODE * 33];
  u16 TC_dig_F[TC_dig_f_Volume * TC_NODE * 33];
  i32 TC_dig_W[TC_dig_m_Volume * TC_NODE * 7];
  i32 TC_dig_WM[TC_dig_t_Volume * TC_MNODE * 3];
  cm_cnt TC_dig_G[TC_SGN_DIG];
  cm_cnt TC_sgn_A[TC_sgn_a_Volume * TC_NODE];
  cm_cnt TC_sgn_B[TC_sgn_b_Volume * TC_NODE];
  cm_cnt TC_sgn_C[TC_sgn_c_Volume * TC_NODE];
  cm_cnt TC_sgn_D[TC_sgn_d_Volume * TC_NODE];
  cm_cnt TC_sgn_T[TC_sgn_t_Volume * TC_MNODE];
  u16 TC_sgn_S[TC_sgn_s_Volume * TC_NODE * 33];
  u16 TC_sgn_F[TC_sgn_f_Volume * TC_NODE * 33];
  i32 TC_sgn_W[TC_sgn_m_Volume * TC_NODE * 7];
  i32 TC_sgn_WM[TC_sgn_t_Volume * TC_MNODE * 3];
  cm_cnt TC_flr_A[TC_flr_a_Volume * TC_NODE];
  cm_cnt TC_flr_B[TC_flr_b_Volume * TC_NODE];
  cm_cnt TC_flr_C[TC_flr_c_Volume * TC_NODE];
  cm_cnt TC_flr_D[TC_flr_d_Volume * TC_NODE];
  cm_cnt TC_flr_T[TC_flr_t_Volume * TC_MNODE];
  u16 TC_flr_S[TC_flr_s_Volume * TC_NODE * 33];
  u16 TC_flr_F[TC_flr_f_Volume * TC_NODE * 33];
  i32 TC_flr_W[TC_flr_m_Volume * TC_NODE * 7];
  i32 TC_flr_WM[TC_flr_t_Volume * TC_MNODE * 3];
  cm_cnt TC_flr_G[TC_SGN_FLR];
  cm_cnt TC_cls_A[TC_cls_a_Volume * TC_NODE];
  cm_cnt TC_cls_B[TC_cls_b_Volume * TC_NODE];
  cm_cnt TC_cls_C[TC_cls_c_Volume * TC_NODE];
  cm_cnt TC_cls_D[TC_cls_d_Volume * TC_NODE];
  cm_cnt TC_cls_T[TC_cls_t_Volume * TC_MNODE];
  u16 TC_cls_S[TC_cls_s_Volume * TC_NODE * 33];
  u16 TC_cls_F[TC_cls_f_Volume * TC_NODE * 33];
  i32 TC_cls_W[TC_cls_m_Volume * TC_NODE * 7];
  i32 TC_cls_WM[TC_cls_t_Volume * TC_MNODE * 3];
  cm_cnt TC_cls_G[TC_SGN_CLS];
  cm_cnt TC_aux_A[TC_aux_a_Volume * TC_NODE];
  cm_cnt TC_aux_B[TC_aux_b_Volume * TC_NODE];
  cm_cnt TC_aux_C[TC_aux_c_Volume * TC_NODE];
  cm_cnt TC_aux_D[TC_aux_d_Volume * TC_NODE];
  cm_cnt TC_aux_T[TC_aux_t_Volume * TC_MNODE];
  u16 TC_aux_S[TC_aux_s_Volume * TC_NODE * 33];
  u16 TC_aux_F[TC_aux_f_Volume * TC_NODE * 33];
  i32 TC_aux_W[TC_aux_m_Volume * TC_NODE * 7];
  i32 TC_aux_WM[TC_aux_t_Volume * TC_MNODE * 3];
  cm_cnt TC_aux_G[TC_SGN_AUX];
  cm_cnt TC_hdr_A[TC_hdr_a_Volume * TC_NODE];
  cm_cnt TC_hdr_B[TC_hdr_b_Volume * TC_NODE];
  cm_cnt TC_hdr_C[TC_hdr_c_Volume * TC_NODE];
  cm_cnt TC_hdr_D[TC_hdr_d_Volume * TC_NODE];
  cm_cnt TC_hdr_T[TC_hdr_t_Volume * TC_MNODE];
  u16 TC_hdr_S[TC_hdr_s_Volume * TC_NODE * 33];
  u16 TC_hdr_F[TC_hdr_f_Volume * TC_NODE * 33];
  i32 TC_hdr_W[TC_hdr_m_Volume * TC_NODE * 7];
  i32 TC_hdr_WM[TC_hdr_t_Volume * TC_MNODE * 3];
  cm_cnt TC_hdr_G[TC_SGN_HDR];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long TC_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_a_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_b_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_c_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_d_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(u16) * (unsigned long long)(TC_dig_s_Volume * TC_NODE * 33)
                              + (unsigned long long)sizeof(u16) * (unsigned long long)(TC_dig_f_Volume * TC_NODE * 33)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_dig_m_Volume * TC_NODE * 7)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_dig_t_Volume * TC_MNODE * 3)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_SGN_DIG)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_sgn_a_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_sgn_b_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_sgn_c_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_sgn_d_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_sgn_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(u16) * (unsigned long long)(TC_sgn_s_Volume * TC_NODE * 33)
                              + (unsigned long long)sizeof(u16) * (unsigned long long)(TC_sgn_f_Volume * TC_NODE * 33)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_sgn_m_Volume * TC_NODE * 7)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_sgn_t_Volume * TC_MNODE * 3)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_flr_a_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_flr_b_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_flr_c_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_flr_d_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_flr_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(u16) * (unsigned long long)(TC_flr_s_Volume * TC_NODE * 33)
                              + (unsigned long long)sizeof(u16) * (unsigned long long)(TC_flr_f_Volume * TC_NODE * 33)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_flr_m_Volume * TC_NODE * 7)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_flr_t_Volume * TC_MNODE * 3)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_SGN_FLR)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_cls_a_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_cls_b_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_cls_c_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_cls_d_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_cls_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(u16) * (unsigned long long)(TC_cls_s_Volume * TC_NODE * 33)
                              + (unsigned long long)sizeof(u16) * (unsigned long long)(TC_cls_f_Volume * TC_NODE * 33)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_cls_m_Volume * TC_NODE * 7)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_cls_t_Volume * TC_MNODE * 3)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_SGN_CLS)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_aux_a_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_aux_b_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_aux_c_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_aux_d_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_aux_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(u16) * (unsigned long long)(TC_aux_s_Volume * TC_NODE * 33)
                              + (unsigned long long)sizeof(u16) * (unsigned long long)(TC_aux_f_Volume * TC_NODE * 33)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_aux_m_Volume * TC_NODE * 7)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_aux_t_Volume * TC_MNODE * 3)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_SGN_AUX)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_hdr_a_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_hdr_b_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_hdr_c_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_hdr_d_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_hdr_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(u16) * (unsigned long long)(TC_hdr_s_Volume * TC_NODE * 33)
                              + (unsigned long long)sizeof(u16) * (unsigned long long)(TC_hdr_f_Volume * TC_NODE * 33)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_hdr_m_Volume * TC_NODE * 7)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_hdr_t_Volume * TC_MNODE * 3)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_SGN_HDR);
  void TC_Init( void ) {
  
  }
  void TC_Quit( void ) {
  }
};
