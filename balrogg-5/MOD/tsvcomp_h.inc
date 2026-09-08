
static const byte TC_aux_a_p1[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };





static const byte TC_cls_a_band[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

static const byte TC_cls_b_prev[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };




static const byte TC_dig_a_cls[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };




static const byte TC_dig_t_cls[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

static const byte TC_flr_a_col[48+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48 };

static const byte TC_flr_b_p1[14+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14 };



static const byte TC_flr_t_col[48+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48 };

static const byte TC_hdr_a_p1[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };






static const int TC_dig_rA = (255+0) * (1);
static const int TC_dig_rB = (255+0) * (1);
static const int TC_dig_rS = (7+0) * (1);
static const int TC_dig_lr = (8+0) * (1);
static const int TC_flr_rA = (255+0) * (1);
static const int TC_flr_rB = (255+0) * (1);
static const int TC_flr_rS = (6+0) * (1);
static const int TC_flr_lr = (6+0) * (1);
static const int TC_cls_rA = (255+0) * (1);
static const int TC_cls_rB = (255+0) * (1);
static const int TC_cls_rS = (6+0) * (1);
static const int TC_cls_lr = (6+0) * (1);
static const int TC_aux_rA = (255+0) * (1);
static const int TC_aux_rB = (255+0) * (1);
static const int TC_aux_rS = (6+0) * (1);
static const int TC_aux_lr = (6+0) * (1);
static const int TC_hdr_rA = (255+0) * (1);
static const int TC_hdr_rB = (255+0) * (1);
static const int TC_hdr_rS = (6+0) * (1);
static const int TC_hdr_lr = (6+0) * (1);


static const int TC_aux_a_Volume = 1* 32* 16* 8; // 4096
static const int TC_aux_b_Volume = 1* 32* 4* 1; // 128
static const int TC_aux_m_Volume = 1* 32; // 32
static const int TC_aux_s_Volume = 1* 32; // 32
static const int TC_aux_t_Volume = 1* 32; // 32
static const int TC_cls_a_Volume = 1* 2* 16* 8* 8* 2; // 4096
static const int TC_cls_b_Volume = 1* 1* 1* 16* 4* 1; // 64
static const int TC_cls_m_Volume = 1* 1* 4* 2; // 8
static const int TC_cls_s_Volume = 1* 4* 4* 1; // 16
static const int TC_cls_t_Volume = 1* 4* 4* 1; // 16
static const int TC_dig_a_Volume = 1* 1* 8* 3* 1* 6* 5* 4* 1* 16* 5* 1; // 230400
static const int TC_dig_b_Volume = 1* 1* 8* 1* 1* 6* 1* 4* 1* 1* 4* 2; // 1536
static const int TC_dig_m_Volume = 1* 4* 4* 1* 2* 1* 1; // 32
static const int TC_dig_s_Volume = 1* 3* 4* 4* 1* 1* 4; // 192
static const int TC_dig_t_Volume = 1* 4* 4* 6* 1* 1* 16; // 1536
static const int TC_flr_a_Volume = 1* 2* 49* 8* 4* 1* 1; // 3136
static const int TC_flr_b_Volume = 1* 1* 1* 15* 8* 2* 2; // 480
static const int TC_flr_m_Volume = 1* 4* 4* 1; // 16
static const int TC_flr_s_Volume = 1* 8* 4* 1; // 32
static const int TC_flr_t_Volume = 1* 49* 4* 1; // 196
static const int TC_hdr_a_Volume = 1* 64* 16* 1* 4; // 4096
static const int TC_hdr_b_Volume = 1* 1* 2* (1<<8); // 512
static const int TC_hdr_m_Volume = 1* 64; // 64
static const int TC_hdr_s_Volume = 1* 64; // 64
static const int TC_hdr_t_Volume = 1* 64* (1<<8); // 16384

struct TC_T { 
#undef USE_NEW
#define USE_NEW 0


  cm_cnt TC_dig_A[TC_dig_a_Volume * TC_NODE];
  cm_cnt TC_dig_B[TC_dig_b_Volume * TC_NODE];
  cm_cnt TC_dig_T[TC_dig_t_Volume * TC_MNODE];
  u16 TC_dig_S[TC_dig_s_Volume * TC_NODE * 33];
  i32 TC_dig_W[TC_dig_m_Volume * TC_NODE * 2];
  cm_cnt TC_dig_G[TC_SGN_DIG];
  cm_cnt TC_flr_A[TC_flr_a_Volume * TC_NODE];
  cm_cnt TC_flr_B[TC_flr_b_Volume * TC_NODE];
  cm_cnt TC_flr_T[TC_flr_t_Volume * TC_MNODE];
  u16 TC_flr_S[TC_flr_s_Volume * TC_NODE * 33];
  i32 TC_flr_W[TC_flr_m_Volume * TC_NODE * 2];
  cm_cnt TC_flr_G[TC_SGN_FLR];
  cm_cnt TC_cls_A[TC_cls_a_Volume * TC_NODE];
  cm_cnt TC_cls_B[TC_cls_b_Volume * TC_NODE];
  cm_cnt TC_cls_T[TC_cls_t_Volume * TC_MNODE];
  u16 TC_cls_S[TC_cls_s_Volume * TC_NODE * 33];
  i32 TC_cls_W[TC_cls_m_Volume * TC_NODE * 2];
  cm_cnt TC_cls_G[TC_SGN_CLS];
  cm_cnt TC_aux_A[TC_aux_a_Volume * TC_NODE];
  cm_cnt TC_aux_B[TC_aux_b_Volume * TC_NODE];
  cm_cnt TC_aux_T[TC_aux_t_Volume * TC_MNODE];
  u16 TC_aux_S[TC_aux_s_Volume * TC_NODE * 33];
  i32 TC_aux_W[TC_aux_m_Volume * TC_NODE * 2];
  cm_cnt TC_aux_G[TC_SGN_AUX];
  cm_cnt TC_hdr_A[TC_hdr_a_Volume * TC_NODE];
  cm_cnt TC_hdr_B[TC_hdr_b_Volume * TC_NODE];
  cm_cnt TC_hdr_T[TC_hdr_t_Volume * TC_MNODE];
  u16 TC_hdr_S[TC_hdr_s_Volume * TC_NODE * 33];
  i32 TC_hdr_W[TC_hdr_m_Volume * TC_NODE * 2];
  cm_cnt TC_hdr_G[TC_SGN_HDR];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long TC_Size = 0
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_a_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_b_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_dig_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(u16) * (unsigned long long)(TC_dig_s_Volume * TC_NODE * 33)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_dig_m_Volume * TC_NODE * 2)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_SGN_DIG)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_flr_a_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_flr_b_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_flr_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(u16) * (unsigned long long)(TC_flr_s_Volume * TC_NODE * 33)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_flr_m_Volume * TC_NODE * 2)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_SGN_FLR)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_cls_a_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_cls_b_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_cls_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(u16) * (unsigned long long)(TC_cls_s_Volume * TC_NODE * 33)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_cls_m_Volume * TC_NODE * 2)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_SGN_CLS)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_aux_a_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_aux_b_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_aux_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(u16) * (unsigned long long)(TC_aux_s_Volume * TC_NODE * 33)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_aux_m_Volume * TC_NODE * 2)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_SGN_AUX)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_hdr_a_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_hdr_b_Volume * TC_NODE)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_hdr_t_Volume * TC_MNODE)
                              + (unsigned long long)sizeof(u16) * (unsigned long long)(TC_hdr_s_Volume * TC_NODE * 33)
                              + (unsigned long long)sizeof(i32) * (unsigned long long)(TC_hdr_m_Volume * TC_NODE * 2)
                              + (unsigned long long)sizeof(cm_cnt) * (unsigned long long)(TC_SGN_HDR);
  void TC_Init( void ) {
  
  }
  void TC_Quit( void ) {
  }
};
