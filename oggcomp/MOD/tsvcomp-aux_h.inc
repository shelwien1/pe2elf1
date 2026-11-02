
pdesc( TC_aux_a_fld, 1, "001111111000011111" );
mdesc( TC_aux_a_p1, 1, "100000001000111" );
mdesc( TC_aux_a_p2, 1, "1000001" );

pdesc( TC_aux_b_fld, 1, "010011001111010000" );
mdesc( TC_aux_b_p1, 1, "000" );
mdesc( TC_aux_b_p2, 1, "000" );

pdesc( TC_aux_c_fld, 1, "100011001011010000" );

pdesc( TC_aux_d_fld, 1, "000100010001010000" );

pdesc( TC_aux_f_fld, 1, "111111111111111111" );

pdesc( TC_aux_g_p1, 1, "000000000000111" );
pdesc( TC_aux_g_p2, 1, "0000000010011" );

pdesc( TC_aux_m_fld, 1, "001001101111011111" );

pdesc( TC_aux_n_fld, 1, "111111111111111111" );

pdesc( TC_aux_s_fld, 1, "111111111111111111" );

pdesc( TC_aux_t_fld, 1, "000101000000011111" );


pdesc( TC_aux_rA_, 0, "10000000" );
static const int TC_aux_rA = TC_aux_rA_.value * (1);
pdesc( TC_aux_rB_, 0, "00000000" );
static const int TC_aux_rB = TC_aux_rB_.value * (1);
pdesc( TC_aux_rC_, 0, "00000000" );
static const int TC_aux_rC = TC_aux_rC_.value * (1);
pdesc( TC_aux_rD_, 0, "00000000" );
static const int TC_aux_rD = TC_aux_rD_.value * (1);
pdesc( TC_aux_rT_, 0, "00110111" );
static const int TC_aux_rT = TC_aux_rT_.value * (1);
pdesc( TC_aux_rG_, 0, "01011010" );
static const int TC_aux_rG = TC_aux_rG_.value * (1);
pdesc( TC_aux_rS1_, 0, "10000" );
static const int TC_aux_rS1 = TC_aux_rS1_.value * (1);
pdesc( TC_aux_rS2_, 0, "00110" );
static const int TC_aux_rS2 = TC_aux_rS2_.value * (1);
pdesc( TC_aux_lr_, 0, "11011" );
static const int TC_aux_lr = TC_aux_lr_.value * (1);
pdesc( TC_aux_lrm_, 0, "11111" );
static const int TC_aux_lrm = TC_aux_lrm_.value * (1);
pdesc( TC_aux_mbm_, 0, "00000000" );
static const int TC_aux_mbm = TC_aux_mbm_.value * (1);
pdesc( TC_aux_mwA_, 0, "00000000" );
static const int TC_aux_mwA = TC_aux_mwA_.value * (1);
pdesc( TC_aux_mwB_, 0, "00010000" );
static const int TC_aux_mwB = TC_aux_mwB_.value * (1);
pdesc( TC_aux_mwC_, 0, "01000010" );
static const int TC_aux_mwC = TC_aux_mwC_.value * (1);
pdesc( TC_aux_mwD_, 0, "00000110" );
static const int TC_aux_mwD = TC_aux_mwD_.value * (1);
pdesc( TC_aux_mwT_, 0, "00000010" );
static const int TC_aux_mwT = TC_aux_mwT_.value * (1);
pdesc( TC_aux_mwG_, 0, "00000000" );
static const int TC_aux_mwG = TC_aux_mwG_.value * (1);
pdesc( TC_aux_bw_, 0, "00110" );
static const int TC_aux_bw = TC_aux_bw_.value * (1);
pdesc( TC_aux_qs_, 3, "1011010" );
static const int TC_aux_qs = TC_aux_qs_.value * (1);
pdesc( TC_aux_qf_, 3, "1000001" );
static const int TC_aux_qf = TC_aux_qf_.value * (1);
pdesc( TC_aux_mb_, 0, "01000000" );
static const int TC_aux_mb = TC_aux_mb_.value * (1);
mdesc( TC_aux_hA_, 0, "11111" );
static const int TC_aux_hA = TC_aux_hA_.value * (1);
mdesc( TC_aux_hB_, 0, "11111" );
static const int TC_aux_hB = TC_aux_hB_.value * (1);
mdesc( TC_aux_hD_, 0, "11111" );
static const int TC_aux_hD = TC_aux_hD_.value * (1);
mdesc( TC_aux_hS_, 0, "11111" );
static const int TC_aux_hS = TC_aux_hS_.value * (1);
mdesc( TC_aux_hF_, 0, "11111" );
static const int TC_aux_hF = TC_aux_hF_.value * (1);
mdesc( TC_aux_hM_, 0, "11111" );
static const int TC_aux_hM = TC_aux_hM_.value * (1);
mdesc( TC_aux_hT_, 0, "11111" );
static const int TC_aux_hT = TC_aux_hT_.value * (1);
mdesc( TC_aux_hN_, 0, "11111" );
static const int TC_aux_hN = TC_aux_hN_.value * (1);


static const long long TC_aux_a_Volume = tc_vmul(tc_vmul(tc_vmul(1, TC_aux_a_fld.Size), TC_aux_a_p1.Size), TC_aux_a_p2.Size); // 234
static const long long TC_aux_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, TC_aux_b_fld.Size), TC_aux_b_p1.Size), TC_aux_b_p2.Size); // 9
static const long long TC_aux_c_Volume = tc_vmul(1, TC_aux_c_fld.Size); // 8
static const long long TC_aux_d_Volume = tc_vmul(1, TC_aux_d_fld.Size); // 5
static const long long TC_aux_f_Volume = tc_vmul(1, TC_aux_f_fld.Size); // 19
static const long long TC_aux_g_Volume = tc_vmul(tc_vmul(1, TC_aux_g_p1.Size), TC_aux_g_p2.Size); // 16
static const long long TC_aux_m_Volume = tc_vmul(1, TC_aux_m_fld.Size); // 13
static const long long TC_aux_n_Volume = tc_vmul(1, TC_aux_n_fld.Size); // 19
static const long long TC_aux_s_Volume = tc_vmul(1, TC_aux_s_fld.Size); // 19
static const long long TC_aux_t_Volume = tc_vmul(1, TC_aux_t_fld.Size); // 8

struct TC_aux_T { 
#undef USE_NEW
#define USE_NEW 1


  cm_cnt* TC_aux_A;
  cm_cnt* TC_aux_B;
  cm_cnt* TC_aux_C;
  cm_cnt* TC_aux_D;
  cm_cnt* TC_aux_MT;
  i16* TC_aux_S;
  u8* TC_aux_SC;
  i16* TC_aux_F;
  u8* TC_aux_FC;
  i32* TC_aux_W;
  u8* TC_aux_WC;
  i32* TC_aux_WM;
  u8* TC_aux_WMC;
  cm_cnt* TC_aux_G;
  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long TC_aux_Size = 0;
  void TC_aux_Init( void ) {
    TC_aux_Size = 0;
    TC_aux_A = (cm_cnt *) tc_map( (unsigned long long)sizeof(cm_cnt) * tbl_n(tc_rows(TC_aux_a_Volume, TC_aux_hA) * TC_aux_ND) );
    TC_aux_Size += (unsigned long long)sizeof(cm_cnt) * (unsigned long long)tbl_n(tc_rows(TC_aux_a_Volume, TC_aux_hA) * TC_aux_ND);
    TC_aux_B = (cm_cnt *) tc_map( (unsigned long long)sizeof(cm_cnt) * tbl_n(tc_rows(TC_aux_b_Volume, TC_aux_hB) * TC_aux_ND) );
    TC_aux_Size += (unsigned long long)sizeof(cm_cnt) * (unsigned long long)tbl_n(tc_rows(TC_aux_b_Volume, TC_aux_hB) * TC_aux_ND);
    TC_aux_C = (cm_cnt *) tc_map( (unsigned long long)sizeof(cm_cnt) * tbl_n(TC_aux_c_Volume * TC_aux_ND) );
    TC_aux_Size += (unsigned long long)sizeof(cm_cnt) * (unsigned long long)tbl_n(TC_aux_c_Volume * TC_aux_ND);
    TC_aux_D = (cm_cnt *) tc_map( (unsigned long long)sizeof(cm_cnt) * tbl_n(tc_rows(TC_aux_d_Volume, TC_aux_hD) * TC_aux_ND) );
    TC_aux_Size += (unsigned long long)sizeof(cm_cnt) * (unsigned long long)tbl_n(tc_rows(TC_aux_d_Volume, TC_aux_hD) * TC_aux_ND);
    TC_aux_MT = (cm_cnt *) tc_map( (unsigned long long)sizeof(cm_cnt) * tbl_n(tc_rows(TC_aux_t_Volume, TC_aux_hT) * TC_aux_MND) );
    TC_aux_Size += (unsigned long long)sizeof(cm_cnt) * (unsigned long long)tbl_n(tc_rows(TC_aux_t_Volume, TC_aux_hT) * TC_aux_MND);
    TC_aux_S = (i16 *) tc_map( (unsigned long long)sizeof(i16) * tbl_n(tc_rows(TC_aux_s_Volume, TC_aux_hS) * TC_aux_ND * TC_aux_qs) );
    TC_aux_Size += (unsigned long long)sizeof(i16) * (unsigned long long)tbl_n(tc_rows(TC_aux_s_Volume, TC_aux_hS) * TC_aux_ND * TC_aux_qs);
    TC_aux_SC = (u8 *) tc_map( (unsigned long long)sizeof(u8) * tbl_n(tc_rows(TC_aux_s_Volume, TC_aux_hS) * TC_aux_ND * TC_aux_qs) );
    TC_aux_Size += (unsigned long long)sizeof(u8) * (unsigned long long)tbl_n(tc_rows(TC_aux_s_Volume, TC_aux_hS) * TC_aux_ND * TC_aux_qs);
    TC_aux_F = (i16 *) tc_map( (unsigned long long)sizeof(i16) * tbl_n(tc_rows(TC_aux_f_Volume, TC_aux_hF) * TC_aux_ND * TC_aux_qf) );
    TC_aux_Size += (unsigned long long)sizeof(i16) * (unsigned long long)tbl_n(tc_rows(TC_aux_f_Volume, TC_aux_hF) * TC_aux_ND * TC_aux_qf);
    TC_aux_FC = (u8 *) tc_map( (unsigned long long)sizeof(u8) * tbl_n(tc_rows(TC_aux_f_Volume, TC_aux_hF) * TC_aux_ND * TC_aux_qf) );
    TC_aux_Size += (unsigned long long)sizeof(u8) * (unsigned long long)tbl_n(tc_rows(TC_aux_f_Volume, TC_aux_hF) * TC_aux_ND * TC_aux_qf);
    TC_aux_W = (i32 *) tc_map( (unsigned long long)sizeof(i32) * tbl_n(tc_rows(TC_aux_m_Volume, TC_aux_hM) * TC_aux_ND * TC_MIXN) );
    TC_aux_Size += (unsigned long long)sizeof(i32) * (unsigned long long)tbl_n(tc_rows(TC_aux_m_Volume, TC_aux_hM) * TC_aux_ND * TC_MIXN);
    TC_aux_WC = (u8 *) tc_map( (unsigned long long)sizeof(u8) * tbl_n(tc_rows(TC_aux_m_Volume, TC_aux_hM) * TC_aux_ND) );
    TC_aux_Size += (unsigned long long)sizeof(u8) * (unsigned long long)tbl_n(tc_rows(TC_aux_m_Volume, TC_aux_hM) * TC_aux_ND);
    TC_aux_WM = (i32 *) tc_map( (unsigned long long)sizeof(i32) * tbl_n(tc_rows(TC_aux_n_Volume, TC_aux_hN) * TC_aux_MND * 3) );
    TC_aux_Size += (unsigned long long)sizeof(i32) * (unsigned long long)tbl_n(tc_rows(TC_aux_n_Volume, TC_aux_hN) * TC_aux_MND * 3);
    TC_aux_WMC = (u8 *) tc_map( (unsigned long long)sizeof(u8) * tbl_n(tc_rows(TC_aux_n_Volume, TC_aux_hN) * TC_aux_MND) );
    TC_aux_Size += (unsigned long long)sizeof(u8) * (unsigned long long)tbl_n(tc_rows(TC_aux_n_Volume, TC_aux_hN) * TC_aux_MND);
    TC_aux_G = (cm_cnt *) tc_map( (unsigned long long)sizeof(cm_cnt) * tbl_n(TC_aux_g_Volume * TC_SGN_AUX) );
    TC_aux_Size += (unsigned long long)sizeof(cm_cnt) * (unsigned long long)tbl_n(TC_aux_g_Volume * TC_SGN_AUX);
  
  }
};
