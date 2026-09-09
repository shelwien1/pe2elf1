
















static const byte TC_dig_a_band[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

static const byte TC_dig_b_band[15+1]={ 0,1,2,2,3,4,5,6,6,7,8,9,10,10,11,12 };
static const byte TC_dig_b_pq[32+1]={ 0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8 };


static const byte TC_dig_d_q1s[13+1]={ 0,1,1,2,2,3,4,4,5,6,6,7,7,8 };

static const byte TC_dig_f_cls[9+1]={ 0,1,1,2,3,4,5,6,7,8 };

static const byte TC_dig_m_cls[15+1]={ 0,1,1,2,2,3,4,5,5,6,6,7,7,8,8,9 };



static const byte TC_flr_a_col[48+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,28,28,28,28,28,29,29,29,30,31,32,32,32,32,32,32,32,32,32,33 };

static const byte TC_flr_b_p1[14+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,12,12 };
static const byte TC_flr_b_lov[8+1]={ 0,1,2,3,4,5,6,7,8 };
static const byte TC_flr_b_ep[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };

static const byte TC_flr_c_ep[11+1]={ 0,1,2,3,4,5,6,7,8,9,10,11 };



static const byte TC_flr_m_col[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
static const byte TC_flr_m_pri[32+1]={ 0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8 };


static const byte TC_flr_t_col[48+1]={ 0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,19,20,21,22,23,24,25,25,25,25,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26,26 };









static const byte TC_sgn_a_sq[32+1]={ 0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8 };
static const byte TC_sgn_a_ps[13+1]={ 0,1,1,2,2,3,4,4,5,6,6,7,7,8 };

static const byte TC_sgn_b_sq[32+1]={ 0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8,8 };



static const byte TC_sgn_f_sq[32+1]={ 0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8,8 };





static const int TC_dig_avD = (239+0) * (1);
static const int TC_dig_avA = (7+0) * (1);
static const int TC_dig_avT1 = (2+0) * (1);
static const int TC_dig_avT2 = (1+0) * (1);
static const int TC_dig_avN1 = (5+0) * (1);
static const int TC_dig_avW1 = (3+0) * (1);
static const int TC_dig_avQ1 = (5+0) * (1);
static const int TC_dig_avQ2 = (3+0) * (1);
static const int TC_dig_avC = (431+0) * (1);
static const int TC_dig_rA = (255+0) * (1);
static const int TC_dig_rB = (254+0) * (1);
static const int TC_dig_rC = (16+0) * (1);
static const int TC_dig_rD = (227+0) * (1);
static const int TC_dig_rS = (7+0) * (1);
static const int TC_dig_lr = (11+0) * (1);
static const int TC_dig_mw = (2+0) * (1);
static const int TC_dig_bw = (11+0) * (1);
static const int TC_dig_qs = (30+3) * (1);
static const int TC_dig_qf = (62+3) * (1);
static const int TC_dig_mb = (68+0) * (1);
static const int TC_sgn_rA = (254+0) * (1);
static const int TC_sgn_rB = (255+0) * (1);
static const int TC_sgn_rC = (10+0) * (1);
static const int TC_sgn_rD = (11+0) * (1);
static const int TC_sgn_rS = (7+0) * (1);
static const int TC_sgn_lr = (18+0) * (1);
static const int TC_sgn_mw = (0+0) * (1);
static const int TC_sgn_bw = (12+0) * (1);
static const int TC_sgn_qs = (46+3) * (1);
static const int TC_sgn_qf = (62+3) * (1);
static const int TC_sgn_mb = (16+0) * (1);
static const int TC_flr_rA = (15+0) * (1);
static const int TC_flr_rB = (75+0) * (1);
static const int TC_flr_rC = (63+0) * (1);
static const int TC_flr_rD = (63+0) * (1);
static const int TC_flr_rS = (6+0) * (1);
static const int TC_flr_lr = (22+0) * (1);
static const int TC_flr_mw = (0+0) * (1);
static const int TC_flr_bw = (12+0) * (1);
static const int TC_flr_qs = (30+3) * (1);
static const int TC_flr_qf = (62+3) * (1);
static const int TC_flr_mb = (0+0) * (1);
static const int TC_cls_rA = (80+0) * (1);
static const int TC_cls_rB = (43+0) * (1);
static const int TC_cls_rC = (63+0) * (1);
static const int TC_cls_rD = (63+0) * (1);
static const int TC_cls_rS = (5+0) * (1);
static const int TC_cls_lr = (24+0) * (1);
static const int TC_cls_mw = (0+0) * (1);
static const int TC_cls_bw = (12+0) * (1);
static const int TC_cls_qs = (30+3) * (1);
static const int TC_cls_qf = (62+3) * (1);
static const int TC_cls_mb = (0+0) * (1);
static const int TC_aux_rA = (22+0) * (1);
static const int TC_aux_rB = (0+0) * (1);
static const int TC_aux_rC = (63+0) * (1);
static const int TC_aux_rD = (63+0) * (1);
static const int TC_aux_rS = (6+0) * (1);
static const int TC_aux_lr = (31+0) * (1);
static const int TC_aux_mw = (0+0) * (1);
static const int TC_aux_bw = (12+0) * (1);
static const int TC_aux_qs = (30+3) * (1);
static const int TC_aux_qf = (30+3) * (1);
static const int TC_aux_mb = (0+0) * (1);
static const int TC_hdr_rA = (16+0) * (1);
static const int TC_hdr_rB = (32+0) * (1);
static const int TC_hdr_rC = (63+0) * (1);
static const int TC_hdr_rD = (63+0) * (1);
static const int TC_hdr_rS = (1+0) * (1);
static const int TC_hdr_lr = (31+0) * (1);
static const int TC_hdr_mw = (0+0) * (1);
static const int TC_hdr_bw = (12+0) * (1);
static const int TC_hdr_qs = (30+3) * (1);
static const int TC_hdr_qf = (30+3) * (1);
static const int TC_hdr_mb = (0+0) * (1);


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
static const int TC_dig_a_Volume = 1* 1* 8* 16* 5* 1* 1* 1* 2* 6* 1* 2* 1* 1* 1* 1* 1* 1* 1* 1* 2* 1* 2* 3* 1* 5* 1* 1; // 921600
static const int TC_dig_b_Volume = 1* 1* 3* 13* 3* 4* 1* 1* 3* 2* 3* 2* 1* 1* 1* 1* 1* 1* 1* 4* 1* 2* 1* 9* 1* 2* 1* 1; // 2426112
static const int TC_dig_c_Volume = 1* 2* 1* 3* 1* 1* 1* 1* 5* 2* 1* 1* 2* 3* 3* 5* 1* 1; // 5400
static const int TC_dig_d_Volume = 1* 1* 2* 1* 1* 1* 1* 1* 7* 9* 1* 1* 7* 3* 4* 5* 1* 1; // 52920
static const int TC_dig_f_Volume = 1* 3* 1* 9* 4* 1* 1* 6* 4* 1* 1* 5* 1* 1; // 12960
static const int TC_dig_m_Volume = 1* 2* 1* 1* 1* 2* 10* 8* 1* 1* 1* 1* 1* 1* 2* 1* 3* 1* 7* 1* 5* 1* 1; // 67200
static const int TC_dig_s_Volume = 1* 2* 2* 1* 3* 3* 1* 1* 1* 1* 2* 1* 1* 1* 3* 2* 1* 8* 1* 2* 5* 1* 2; // 69120
static const int TC_dig_t_Volume = 1* 4* 1* 1* 1* 1* 7* 1* 1* 1* 1* 1* 1* 1* 2* 1* 1* 1* 1* 1* 5* 1* 1; // 280
static const int TC_flr_a_Volume = 1* 2* 34* 1* 2* 1* 1* 1* 1* 1* 1* 1* 3* 1* 1* 1* 1; // 408
static const int TC_flr_b_Volume = 1* 1* 2* 13* 2* 1* 2* 1* 1* 1* 1* 1* 1* 9* 1* 1* 12; // 11232
static const int TC_flr_c_Volume = 1* 8* 1* 1* 8* 1* 1* 1* 1* 1* 1* 1* 1* 12; // 768
static const int TC_flr_d_Volume = 1* 1* 1* 1* 1* 1* 1* 1* 1* 1* 1* 1* 1* 1; // 1
static const int TC_flr_f_Volume = 1* 1* 1* 1* 1* 1* 1* 1* 1* 1* 1* 1* 1* 1; // 1
static const int TC_flr_m_Volume = 1* 16* 2* 1* 1* 1* 1* 1* 1* 1* 1* 1* 9* 1; // 288
static const int TC_flr_s_Volume = 1* 6* 1* 3* 1* 1* 1* 1* 1* 1* 1* 1* 1* 1; // 18
static const int TC_flr_t_Volume = 1* 27* 1* 1* 1* 1* 1* 1* 1* 1* 1* 1* 1* 1; // 27
static const int TC_hdr_a_Volume = 1* 64* 5* 8* 3; // 7680
static const int TC_hdr_b_Volume = 1* 3* 1* (1<<5); // 96
static const int TC_hdr_c_Volume = 1* 64; // 64
static const int TC_hdr_d_Volume = 1* 64; // 64
static const int TC_hdr_f_Volume = 1* 64; // 64
static const int TC_hdr_m_Volume = 1* 64; // 64
static const int TC_hdr_s_Volume = 1* 64; // 64
static const int TC_hdr_t_Volume = 1* 64* (1<<6); // 4096
static const int TC_sgn_a_Volume = 1* 3* 1* 6* 7* 1* 5* 1* 1* 2* 5* 9* 9* 1* 1* 1; // 510300
static const int TC_sgn_b_Volume = 1* 3* 2* 1* 8* 5* 1* 1* 4* 4* 5* 9* 1* 1* 1* 1; // 172800
static const int TC_sgn_c_Volume = 1* 2* 1* 3* 2* 1* 2* 1* 1* 1* 1* 1* 1* 1; // 24
static const int TC_sgn_d_Volume = 1* 1* 1* 3* 2* 1* 2* 1* 1* 3* 4* 5* 1* 1; // 720
static const int TC_sgn_f_Volume = 1* 3* 2* 7* 3* 2* 9* 2* 1* 2* 2; // 18144
static const int TC_sgn_m_Volume = 1* 3* 1* 1* 2* 3* 3* 1* 5* 1* 1; // 270
static const int TC_sgn_s_Volume = 1* 2* 1* 4* 1* 5* 3* 4* 5* 2* 1; // 4800
static const int TC_sgn_t_Volume = 1* 1* 1* 1* 1* 1* 1* 1; // 1

struct TC_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt TC_dig_A[TC_dig_a_Volume * TC_NODE];
  cm_cnt TC_dig_B[TC_dig_b_Volume * TC_NODE];
  cm_cnt TC_dig_C[TC_dig_c_Volume * TC_NODE];
  cm_cnt TC_dig_D[TC_dig_d_Volume * TC_NODE];
  cm_cnt TC_dig_T[TC_dig_t_Volume * TC_MNODE];
  i16 TC_dig_S[TC_dig_s_Volume * TC_NODE * TC_dig_qs];
  u8 TC_dig_SC[TC_dig_s_Volume * TC_NODE * TC_dig_qs];
  i16 TC_dig_F[TC_dig_f_Volume * TC_NODE * TC_dig_qf];
  u8 TC_dig_FC[TC_dig_f_Volume * TC_NODE * TC_dig_qf];
  i32 TC_dig_W[TC_dig_m_Volume * TC_NODE * 7];
  u8 TC_dig_WC[TC_dig_m_Volume * TC_NODE];
  i32 TC_dig_WM[TC_dig_t_Volume * TC_MNODE * 3];
  u8 TC_dig_WMC[TC_dig_t_Volume * TC_MNODE];
  cm_cnt TC_sgn_A[TC_sgn_a_Volume * TC_NODE];
  cm_cnt TC_sgn_B[TC_sgn_b_Volume * TC_NODE];
  cm_cnt TC_sgn_C[TC_sgn_c_Volume * TC_NODE];
  cm_cnt TC_sgn_D[TC_sgn_d_Volume * TC_NODE];
  cm_cnt TC_sgn_T[TC_sgn_t_Volume * TC_MNODE];
  i16 TC_sgn_S[TC_sgn_s_Volume * TC_NODE * TC_sgn_qs];
  u8 TC_sgn_SC[TC_sgn_s_Volume * TC_NODE * TC_sgn_qs];
  i16 TC_sgn_F[TC_sgn_f_Volume * TC_NODE * TC_sgn_qf];
  u8 TC_sgn_FC[TC_sgn_f_Volume * TC_NODE * TC_sgn_qf];
  i32 TC_sgn_W[TC_sgn_m_Volume * TC_NODE * 7];
  u8 TC_sgn_WC[TC_sgn_m_Volume * TC_NODE];
  i32 TC_sgn_WM[TC_sgn_t_Volume * TC_MNODE * 3];
  u8 TC_sgn_WMC[TC_sgn_t_Volume * TC_MNODE];
  cm_cnt TC_flr_A[TC_flr_a_Volume * TC_NODE];
  cm_cnt TC_flr_B[TC_flr_b_Volume * TC_NODE];
  cm_cnt TC_flr_C[TC_flr_c_Volume * TC_NODE];
  cm_cnt TC_flr_D[TC_flr_d_Volume * TC_NODE];
  cm_cnt TC_flr_T[TC_flr_t_Volume * TC_MNODE];
  i16 TC_flr_S[TC_flr_s_Volume * TC_NODE * TC_flr_qs];
  u8 TC_flr_SC[TC_flr_s_Volume * TC_NODE * TC_flr_qs];
  i16 TC_flr_F[TC_flr_f_Volume * TC_NODE * TC_flr_qf];
  u8 TC_flr_FC[TC_flr_f_Volume * TC_NODE * TC_flr_qf];
  i32 TC_flr_W[TC_flr_m_Volume * TC_NODE * 7];
  u8 TC_flr_WC[TC_flr_m_Volume * TC_NODE];
  i32 TC_flr_WM[TC_flr_t_Volume * TC_MNODE * 3];
  u8 TC_flr_WMC[TC_flr_t_Volume * TC_MNODE];
  cm_cnt TC_flr_G[TC_SGN_FLR];
  cm_cnt TC_cls_A[TC_cls_a_Volume * TC_NODE];
  cm_cnt TC_cls_B[TC_cls_b_Volume * TC_NODE];
  cm_cnt TC_cls_C[TC_cls_c_Volume * TC_NODE];
  cm_cnt TC_cls_D[TC_cls_d_Volume * TC_NODE];
  cm_cnt TC_cls_T[TC_cls_t_Volume * TC_MNODE];
  i16 TC_cls_S[TC_cls_s_Volume * TC_NODE * TC_cls_qs];
  u8 TC_cls_SC[TC_cls_s_Volume * TC_NODE * TC_cls_qs];
  i16 TC_cls_F[TC_cls_f_Volume * TC_NODE * TC_cls_qf];
  u8 TC_cls_FC[TC_cls_f_Volume * TC_NODE * TC_cls_qf];
  i32 TC_cls_W[TC_cls_m_Volume * TC_NODE * 7];
  u8 TC_cls_WC[TC_cls_m_Volume * TC_NODE];
  i32 TC_cls_WM[TC_cls_t_Volume * TC_MNODE * 3];
  u8 TC_cls_WMC[TC_cls_t_Volume * TC_MNODE];
  cm_cnt TC_cls_G[TC_SGN_CLS];
  cm_cnt TC_aux_A[TC_aux_a_Volume * TC_NODE];
  cm_cnt TC_aux_B[TC_aux_b_Volume * TC_NODE];
  cm_cnt TC_aux_C[TC_aux_c_Volume * TC_NODE];
  cm_cnt TC_aux_D[TC_aux_d_Volume * TC_NODE];
  cm_cnt TC_aux_T[TC_aux_t_Volume * TC_MNODE];
  i16 TC_aux_S[TC_aux_s_Volume * TC_NODE * TC_aux_qs];
  u8 TC_aux_SC[TC_aux_s_Volume * TC_NODE * TC_aux_qs];
  i16 TC_aux_F[TC_aux_f_Volume * TC_NODE * TC_aux_qf];
  u8 TC_aux_FC[TC_aux_f_Volume * TC_NODE * TC_aux_qf];
  i32 TC_aux_W[TC_aux_m_Volume * TC_NODE * 7];
  u8 TC_aux_WC[TC_aux_m_Volume * TC_NODE];
  i32 TC_aux_WM[TC_aux_t_Volume * TC_MNODE * 3];
  u8 TC_aux_WMC[TC_aux_t_Volume * TC_MNODE];
  cm_cnt TC_aux_G[TC_SGN_AUX];
  cm_cnt TC_hdr_A[TC_hdr_a_Volume * TC_NODE];
  cm_cnt TC_hdr_B[TC_hdr_b_Volume * TC_NODE];
  cm_cnt TC_hdr_C[TC_hdr_c_Volume * TC_NODE];
  cm_cnt TC_hdr_D[TC_hdr_d_Volume * TC_NODE];
  cm_cnt TC_hdr_T[TC_hdr_t_Volume * TC_MNODE];
  i16 TC_hdr_S[TC_hdr_s_Volume * TC_NODE * TC_hdr_qs];
  u8 TC_hdr_SC[TC_hdr_s_Volume * TC_NODE * TC_hdr_qs];
  i16 TC_hdr_F[TC_hdr_f_Volume * TC_NODE * TC_hdr_qf];
  u8 TC_hdr_FC[TC_hdr_f_Volume * TC_NODE * TC_hdr_qf];
  i32 TC_hdr_W[TC_hdr_m_Volume * TC_NODE * 7];
  u8 TC_hdr_WC[TC_hdr_m_Volume * TC_NODE];
  i32 TC_hdr_WM[TC_hdr_t_Volume * TC_MNODE * 3];
  u8 TC_hdr_WMC[TC_hdr_t_Volume * TC_MNODE];
  cm_cnt TC_hdr_G[TC_SGN_HDR];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long TC_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_a_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_b_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_c_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_d_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_dig_s_Volume * TC_NODE * TC_dig_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_dig_s_Volume * TC_NODE * TC_dig_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_dig_f_Volume * TC_NODE * TC_dig_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_dig_f_Volume * TC_NODE * TC_dig_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_dig_m_Volume * TC_NODE * 7)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_dig_m_Volume * TC_NODE)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_dig_t_Volume * TC_MNODE * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_dig_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_sgn_a_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_sgn_b_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_sgn_c_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_sgn_d_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_sgn_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_sgn_s_Volume * TC_NODE * TC_sgn_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_sgn_s_Volume * TC_NODE * TC_sgn_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_sgn_f_Volume * TC_NODE * TC_sgn_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_sgn_f_Volume * TC_NODE * TC_sgn_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_sgn_m_Volume * TC_NODE * 7)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_sgn_m_Volume * TC_NODE)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_sgn_t_Volume * TC_MNODE * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_sgn_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_flr_a_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_flr_b_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_flr_c_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_flr_d_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_flr_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_flr_s_Volume * TC_NODE * TC_flr_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_flr_s_Volume * TC_NODE * TC_flr_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_flr_f_Volume * TC_NODE * TC_flr_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_flr_f_Volume * TC_NODE * TC_flr_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_flr_m_Volume * TC_NODE * 7)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_flr_m_Volume * TC_NODE)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_flr_t_Volume * TC_MNODE * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_flr_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_SGN_FLR)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_cls_a_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_cls_b_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_cls_c_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_cls_d_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_cls_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_cls_s_Volume * TC_NODE * TC_cls_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_cls_s_Volume * TC_NODE * TC_cls_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_cls_f_Volume * TC_NODE * TC_cls_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_cls_f_Volume * TC_NODE * TC_cls_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_cls_m_Volume * TC_NODE * 7)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_cls_m_Volume * TC_NODE)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_cls_t_Volume * TC_MNODE * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_cls_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_SGN_CLS)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_aux_a_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_aux_b_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_aux_c_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_aux_d_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_aux_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_aux_s_Volume * TC_NODE * TC_aux_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_aux_s_Volume * TC_NODE * TC_aux_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_aux_f_Volume * TC_NODE * TC_aux_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_aux_f_Volume * TC_NODE * TC_aux_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_aux_m_Volume * TC_NODE * 7)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_aux_m_Volume * TC_NODE)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_aux_t_Volume * TC_MNODE * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_aux_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_SGN_AUX)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_hdr_a_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_hdr_b_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_hdr_c_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_hdr_d_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_hdr_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_hdr_s_Volume * TC_NODE * TC_hdr_qs)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_hdr_s_Volume * TC_NODE * TC_hdr_qs)
                              + (unsigned long long)sizeof(i16) * (unsigned long long)(TC_hdr_f_Volume * TC_NODE * TC_hdr_qf)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_hdr_f_Volume * TC_NODE * TC_hdr_qf)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_hdr_m_Volume * TC_NODE * 7)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_hdr_m_Volume * TC_NODE)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_hdr_t_Volume * TC_MNODE * 3)
                              + (unsigned long long)sizeof(u8) * (unsigned long long)(TC_hdr_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_SGN_HDR);
  void TC_Init( void ) {
  
  }
  void TC_Quit( void ) {
  }
};
