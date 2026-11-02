
pdesc( TC_hdr_a_tag, 1, "0000000000110000100110000010100110000001100001101100000100000000" );
mdesc( TC_hdr_a_p1, 1, "100000000010110" );
mdesc( TC_hdr_a_p2, 1, "1111111" );
mdesc( TC_hdr_a_pos, 1, "011" );
pmask( TC_hdr_a_p1r, "00010111" );

mdesc( TC_hdr_b_p1, 1, "101" );
mdesc( TC_hdr_b_pos, 1, "0" );
mmask( TC_hdr_b_p1r, "01011110" );

pdesc( TC_hdr_c_tag, 1, "0000101111101001101001011010100111000000001110101101111000000000" );

pdesc( TC_hdr_d_tag, 1, "1111111111111111111111111111111111111111111111111111111111111111" );

pdesc( TC_hdr_f_tag, 1, "0100000100100000000000000000000000100010000100000000000000000001" );

pdesc( TC_hdr_g_p1, 1, "000000000000000" );
pdesc( TC_hdr_g_p2, 1, "1001110" );
pdesc( TC_hdr_g_pos, 1, "110" );
pmask( TC_hdr_g_p1r, "11111110" );

pdesc( TC_hdr_m_tag, 1, "0000000000001100100000000000000000000000000110000000000000000000" );

pdesc( TC_hdr_n_tag, 1, "1111111111111111111111111111111111111111111111111111111111111111" );
mmask( TC_hdr_n_p1r, "01101111" );

pdesc( TC_hdr_s_tag, 1, "1111111111111111111111111111111111111111111111111111111111111111" );

pdesc( TC_hdr_t_tag, 1, "0100000100000000000000101000000000000000100000000000000000000000" );
mmask( TC_hdr_t_p1r, "01101111" );


pdesc( TC_hdr_rA_, 0, "00010000" );
static const int TC_hdr_rA = TC_hdr_rA_.value * (1);
pdesc( TC_hdr_rB_, 0, "00000000" );
static const int TC_hdr_rB = TC_hdr_rB_.value * (1);
pdesc( TC_hdr_rC_, 0, "00111111" );
static const int TC_hdr_rC = TC_hdr_rC_.value * (1);
pdesc( TC_hdr_rD_, 0, "11010011" );
static const int TC_hdr_rD = TC_hdr_rD_.value * (1);
pdesc( TC_hdr_rT_, 0, "00010000" );
static const int TC_hdr_rT = TC_hdr_rT_.value * (1);
pdesc( TC_hdr_rG_, 0, "00010000" );
static const int TC_hdr_rG = TC_hdr_rG_.value * (1);
pdesc( TC_hdr_rS1_, 0, "00001" );
static const int TC_hdr_rS1 = TC_hdr_rS1_.value * (1);
pdesc( TC_hdr_rS2_, 0, "00001" );
static const int TC_hdr_rS2 = TC_hdr_rS2_.value * (1);
pdesc( TC_hdr_lr_, 0, "11111" );
static const int TC_hdr_lr = TC_hdr_lr_.value * (1);
pdesc( TC_hdr_lrm_, 0, "11111" );
static const int TC_hdr_lrm = TC_hdr_lrm_.value * (1);
pdesc( TC_hdr_mbm_, 0, "00000000" );
static const int TC_hdr_mbm = TC_hdr_mbm_.value * (1);
pdesc( TC_hdr_mwA_, 0, "00000100" );
static const int TC_hdr_mwA = TC_hdr_mwA_.value * (1);
pdesc( TC_hdr_mwB_, 0, "01000000" );
static const int TC_hdr_mwB = TC_hdr_mwB_.value * (1);
pdesc( TC_hdr_mwC_, 0, "00000000" );
static const int TC_hdr_mwC = TC_hdr_mwC_.value * (1);
pdesc( TC_hdr_mwD_, 0, "00000000" );
static const int TC_hdr_mwD = TC_hdr_mwD_.value * (1);
pdesc( TC_hdr_mwT_, 0, "00101010" );
static const int TC_hdr_mwT = TC_hdr_mwT_.value * (1);
pdesc( TC_hdr_mwG_, 0, "00000000" );
static const int TC_hdr_mwG = TC_hdr_mwG_.value * (1);
pdesc( TC_hdr_bw_, 0, "01111" );
static const int TC_hdr_bw = TC_hdr_bw_.value * (1);
pdesc( TC_hdr_qs_, 3, "0000010" );
static const int TC_hdr_qs = TC_hdr_qs_.value * (1);
pdesc( TC_hdr_qf_, 3, "0000010" );
static const int TC_hdr_qf = TC_hdr_qf_.value * (1);
pdesc( TC_hdr_mb_, 0, "00000101" );
static const int TC_hdr_mb = TC_hdr_mb_.value * (1);
mdesc( TC_hdr_hA_, 0, "11111" );
static const int TC_hdr_hA = TC_hdr_hA_.value * (1);
mdesc( TC_hdr_hB_, 0, "11111" );
static const int TC_hdr_hB = TC_hdr_hB_.value * (1);
mdesc( TC_hdr_hD_, 0, "11111" );
static const int TC_hdr_hD = TC_hdr_hD_.value * (1);
mdesc( TC_hdr_hS_, 0, "11111" );
static const int TC_hdr_hS = TC_hdr_hS_.value * (1);
mdesc( TC_hdr_hF_, 0, "11111" );
static const int TC_hdr_hF = TC_hdr_hF_.value * (1);
mdesc( TC_hdr_hM_, 0, "11111" );
static const int TC_hdr_hM = TC_hdr_hM_.value * (1);
mdesc( TC_hdr_hT_, 0, "11111" );
static const int TC_hdr_hT = TC_hdr_hT_.value * (1);
mdesc( TC_hdr_hN_, 0, "11111" );
static const int TC_hdr_hN = TC_hdr_hN_.value * (1);


static const long long TC_hdr_a_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, TC_hdr_a_tag.Size), TC_hdr_a_p1.Size), TC_hdr_a_p2.Size), TC_hdr_a_pos.Size), TC_hdr_a_p1r.Size); // 2040
static const long long TC_hdr_b_Volume = tc_vmul(tc_vmul(tc_vmul(1, TC_hdr_b_p1.Size), TC_hdr_b_pos.Size), TC_hdr_b_p1r.Size); // 3
static const long long TC_hdr_c_Volume = tc_vmul(1, TC_hdr_c_tag.Size); // 29
static const long long TC_hdr_d_Volume = tc_vmul(1, TC_hdr_d_tag.Size); // 65
static const long long TC_hdr_f_Volume = tc_vmul(1, TC_hdr_f_tag.Size); // 8
static const long long TC_hdr_g_Volume = tc_vmul(tc_vmul(tc_vmul(tc_vmul(1, TC_hdr_g_p1.Size), TC_hdr_g_p2.Size), TC_hdr_g_pos.Size), TC_hdr_g_p1r.Size); // 15
static const long long TC_hdr_m_Volume = tc_vmul(1, TC_hdr_m_tag.Size); // 6
static const long long TC_hdr_n_Volume = tc_vmul(tc_vmul(1, TC_hdr_n_tag.Size), TC_hdr_n_p1r.Size); // 65
static const long long TC_hdr_s_Volume = tc_vmul(1, TC_hdr_s_tag.Size); // 65
static const long long TC_hdr_t_Volume = tc_vmul(tc_vmul(1, TC_hdr_t_tag.Size), TC_hdr_t_p1r.Size); // 6

struct TC_hdr_T { 
#undef USE_NEW
#define USE_NEW 1


  cm_cnt* TC_hdr_A;
  cm_cnt* TC_hdr_B;
  cm_cnt* TC_hdr_C;
  cm_cnt* TC_hdr_D;
  cm_cnt* TC_hdr_MT;
  i16* TC_hdr_S;
  u8* TC_hdr_SC;
  i16* TC_hdr_F;
  u8* TC_hdr_FC;
  i32* TC_hdr_W;
  u8* TC_hdr_WC;
  i32* TC_hdr_WM;
  u8* TC_hdr_WMC;
  cm_cnt* TC_hdr_G;
  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long TC_hdr_Size = 0;
  void TC_hdr_Init( void ) {
    TC_hdr_Size = 0;
    TC_hdr_A = (cm_cnt *) tc_map( (unsigned long long)sizeof(cm_cnt) * tbl_n(tc_rows(TC_hdr_a_Volume, TC_hdr_hA) * TC_hdr_ND) );
    TC_hdr_Size += (unsigned long long)sizeof(cm_cnt) * (unsigned long long)tbl_n(tc_rows(TC_hdr_a_Volume, TC_hdr_hA) * TC_hdr_ND);
    TC_hdr_B = (cm_cnt *) tc_map( (unsigned long long)sizeof(cm_cnt) * tbl_n(tc_rows(TC_hdr_b_Volume, TC_hdr_hB) * TC_hdr_ND) );
    TC_hdr_Size += (unsigned long long)sizeof(cm_cnt) * (unsigned long long)tbl_n(tc_rows(TC_hdr_b_Volume, TC_hdr_hB) * TC_hdr_ND);
    TC_hdr_C = (cm_cnt *) tc_map( (unsigned long long)sizeof(cm_cnt) * tbl_n(TC_hdr_c_Volume * TC_hdr_ND) );
    TC_hdr_Size += (unsigned long long)sizeof(cm_cnt) * (unsigned long long)tbl_n(TC_hdr_c_Volume * TC_hdr_ND);
    TC_hdr_D = (cm_cnt *) tc_map( (unsigned long long)sizeof(cm_cnt) * tbl_n(tc_rows(TC_hdr_d_Volume, TC_hdr_hD) * TC_hdr_ND) );
    TC_hdr_Size += (unsigned long long)sizeof(cm_cnt) * (unsigned long long)tbl_n(tc_rows(TC_hdr_d_Volume, TC_hdr_hD) * TC_hdr_ND);
    TC_hdr_MT = (cm_cnt *) tc_map( (unsigned long long)sizeof(cm_cnt) * tbl_n(tc_rows(TC_hdr_t_Volume, TC_hdr_hT) * TC_hdr_MND) );
    TC_hdr_Size += (unsigned long long)sizeof(cm_cnt) * (unsigned long long)tbl_n(tc_rows(TC_hdr_t_Volume, TC_hdr_hT) * TC_hdr_MND);
    TC_hdr_S = (i16 *) tc_map( (unsigned long long)sizeof(i16) * tbl_n(tc_rows(TC_hdr_s_Volume, TC_hdr_hS) * TC_hdr_ND * TC_hdr_qs) );
    TC_hdr_Size += (unsigned long long)sizeof(i16) * (unsigned long long)tbl_n(tc_rows(TC_hdr_s_Volume, TC_hdr_hS) * TC_hdr_ND * TC_hdr_qs);
    TC_hdr_SC = (u8 *) tc_map( (unsigned long long)sizeof(u8) * tbl_n(tc_rows(TC_hdr_s_Volume, TC_hdr_hS) * TC_hdr_ND * TC_hdr_qs) );
    TC_hdr_Size += (unsigned long long)sizeof(u8) * (unsigned long long)tbl_n(tc_rows(TC_hdr_s_Volume, TC_hdr_hS) * TC_hdr_ND * TC_hdr_qs);
    TC_hdr_F = (i16 *) tc_map( (unsigned long long)sizeof(i16) * tbl_n(tc_rows(TC_hdr_f_Volume, TC_hdr_hF) * TC_hdr_ND * TC_hdr_qf) );
    TC_hdr_Size += (unsigned long long)sizeof(i16) * (unsigned long long)tbl_n(tc_rows(TC_hdr_f_Volume, TC_hdr_hF) * TC_hdr_ND * TC_hdr_qf);
    TC_hdr_FC = (u8 *) tc_map( (unsigned long long)sizeof(u8) * tbl_n(tc_rows(TC_hdr_f_Volume, TC_hdr_hF) * TC_hdr_ND * TC_hdr_qf) );
    TC_hdr_Size += (unsigned long long)sizeof(u8) * (unsigned long long)tbl_n(tc_rows(TC_hdr_f_Volume, TC_hdr_hF) * TC_hdr_ND * TC_hdr_qf);
    TC_hdr_W = (i32 *) tc_map( (unsigned long long)sizeof(i32) * tbl_n(tc_rows(TC_hdr_m_Volume, TC_hdr_hM) * TC_hdr_ND * TC_MIXN) );
    TC_hdr_Size += (unsigned long long)sizeof(i32) * (unsigned long long)tbl_n(tc_rows(TC_hdr_m_Volume, TC_hdr_hM) * TC_hdr_ND * TC_MIXN);
    TC_hdr_WC = (u8 *) tc_map( (unsigned long long)sizeof(u8) * tbl_n(tc_rows(TC_hdr_m_Volume, TC_hdr_hM) * TC_hdr_ND) );
    TC_hdr_Size += (unsigned long long)sizeof(u8) * (unsigned long long)tbl_n(tc_rows(TC_hdr_m_Volume, TC_hdr_hM) * TC_hdr_ND);
    TC_hdr_WM = (i32 *) tc_map( (unsigned long long)sizeof(i32) * tbl_n(tc_rows(TC_hdr_n_Volume, TC_hdr_hN) * TC_hdr_MND * 3) );
    TC_hdr_Size += (unsigned long long)sizeof(i32) * (unsigned long long)tbl_n(tc_rows(TC_hdr_n_Volume, TC_hdr_hN) * TC_hdr_MND * 3);
    TC_hdr_WMC = (u8 *) tc_map( (unsigned long long)sizeof(u8) * tbl_n(tc_rows(TC_hdr_n_Volume, TC_hdr_hN) * TC_hdr_MND) );
    TC_hdr_Size += (unsigned long long)sizeof(u8) * (unsigned long long)tbl_n(tc_rows(TC_hdr_n_Volume, TC_hdr_hN) * TC_hdr_MND);
    TC_hdr_G = (cm_cnt *) tc_map( (unsigned long long)sizeof(cm_cnt) * tbl_n(TC_hdr_g_Volume * TC_SGN_HDR) );
    TC_hdr_Size += (unsigned long long)sizeof(cm_cnt) * (unsigned long long)tbl_n(TC_hdr_g_Volume * TC_SGN_HDR);
  
  }
};
