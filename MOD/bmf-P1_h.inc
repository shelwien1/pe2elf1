





static const int P1_row_margin = (4+0) * (1);
static const int P1_row_tail = (6+0) * (1);
static const int P1_counter_count = (629856+0) * (1);
static const int P1_act_round = (7+0) * (1);
static const int P1_act_shift = (4+0) * (1);
static const int P1_quiet_thr = (16+0) * (1);
static const int P1_level_step_0 = (1+0) * (1);
static const int P1_level_step_1 = (1+0) * (1);
static const int P1_level_step_2 = (2+0) * (1);
static const int P1_level_step_3 = (2+0) * (1);
static const int P1_level_step_4 = (2+0) * (1);
static const int P1_level_step_5 = (4+0) * (1);
static const int P1_level_step_6 = (4+0) * (1);
static const int P1_level_step_7 = (4+0) * (1);
static const int P1_ctxw_shift = (5+0) * (1);
static const int P1_ctxw_ratio = (3+0) * (1);
static const int P1_ctxw_range = (19683+0) * (1);
static const int P1_sel1_ctr = (6+0) * (1);
static const int P1_sel1_up = (4+0) * (1);
static const int P1_sel1_dn = (3+0) * (1);
static const int P1_selN_far = (7+0) * (1);
static const int P1_selN_mid = (6+0) * (1);
static const int P1_selN_alt = (4+0) * (1);
static const int P1_selN_mid_up = (4+0) * (1);
static const int P1_selN_alt_up = (2+0) * (1);
static const int P1_selN_mid_dn = (3+0) * (1);
static const int P1_selN_alt_dn = (2+0) * (1);
static const int P1_bump_alt = (17+0) * (1);
static const int P1_bump_up = (11+0) * (1);
static const int P1_bump_dn = (13+0) * (1);
static const int P1_nudge_up = (7+0) * (1);
static const int P1_nudge_dn = (5+0) * (1);


static const int P1_Guess_Volume = 1* 3; // 3
static const int P1_P1Alt_Volume = 1* P1_ctxw_range* 32; // 0
static const int P1_P1Low_Volume = 1* 8* 8; // 64
static const int P1_P1Result_Volume = 1* P1_ctxw_range* 2* 2* 8; // 0

struct P1_T { 
#undef USE_NEW
#define USE_NEW 0


  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long P1_Size = 0;
  void P1_Init( void ) {
  
  }
  void P1_Quit( void ) {
  }
};
