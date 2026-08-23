
pmask( CN_mslot, "00000000001111" );



static const int CN_slot_split = (5+0) * (1);
static const int CN_slot_top = (6+0) * (1);
pdesc( CN_seed_0_, 0, "1000" );
static const int CN_seed_0 = CN_seed_0_.value * (1);
pdesc( CN_seed_1_, 0, "10" );
static const int CN_seed_1 = CN_seed_1_.value * (1);
pdesc( CN_seed_2_, 0, "10" );
static const int CN_seed_2 = CN_seed_2_.value * (1);
pdesc( CN_seed_3_, 0, "10" );
static const int CN_seed_3 = CN_seed_3_.value * (1);
pdesc( CN_seed_4_, 0, "10" );
static const int CN_seed_4 = CN_seed_4_.value * (1);
pdesc( CN_seed_5_, 0, "11" );
static const int CN_seed_5 = CN_seed_5_.value * (1);
pdesc( CN_seed_6_, 0, "11" );
static const int CN_seed_6 = CN_seed_6_.value * (1);
pdesc( CN_bump_, 0, "100000" );
static const int CN_bump = CN_bump_.value * (1);
pdesc( CN_rescale_at_, 0, "10000000000000" );
static const int CN_rescale_at = CN_rescale_at_.value * (1);
pdesc( CN_halve_shift_, 0, "1" );
static const int CN_halve_shift = CN_halve_shift_.value * (1);
pdesc( CN_rescale_round_, 0, "10" );
static const int CN_rescale_round = CN_rescale_round_.value * (1);
pdesc( CN_rescale_div_, 0, "11" );
static const int CN_rescale_div = CN_rescale_div_.value * (1);
pdesc( CN_escape_bias_, 0, "1100000" );
static const int CN_escape_bias = CN_escape_bias_.value * (1);
pdesc( CN_nudge_limit_, 0, "110011001100" );
static const int CN_nudge_limit = CN_nudge_limit_.value * (1);
pdesc( CN_step_, 0, "1000" );
static const int CN_step = CN_step_.value * (1);
pdesc( CN_seed_bump_, 0, "100" );
static const int CN_seed_bump = CN_seed_bump_.value * (1);
pdesc( CN_seed_limit_, 0, "1000000000" );
static const int CN_seed_limit = CN_seed_limit_.value * (1);
pdesc( CN_limit_step_, 0, "1000000" );
static const int CN_limit_step = CN_limit_step_.value * (1);
pdesc( CN_max_limit_, 0, "100000000000000" );
static const int CN_max_limit = CN_max_limit_.value * (1);
pdesc( CN_parent_bump_, 0, "10001000" );
static const int CN_parent_bump = CN_parent_bump_.value * (1);
pdesc( CN_parent_limit_, 0, "1001000" );
static const int CN_parent_limit = CN_parent_limit_.value * (1);
pdesc( CN_root_n0_, 0, "101000" );
static const int CN_root_n0 = CN_root_n0_.value * (1);
pdesc( CN_root_n1_, 0, "10000" );
static const int CN_root_n1 = CN_root_n1_.value * (1);
pdesc( CN_seed_scale_, 0, "1000000" );
static const int CN_seed_scale = CN_seed_scale_.value * (1);
pdesc( CN_seed_decay_, 0, "11" );
static const int CN_seed_decay = CN_seed_decay_.value * (1);
pdesc( CN_p16_floor_, 0, "100000" );
static const int CN_p16_floor = CN_p16_floor_.value * (1);
pdesc( CN_p16_ceil_, 0, "1111111111100000" );
static const int CN_p16_ceil = CN_p16_ceil_.value * (1);
pdesc( CN_drift_limit_, 0, "10000" );
static const int CN_drift_limit = CN_drift_limit_.value * (1);


static const int CN_SlotMap_Volume = 1* CN_mslot.Size; // 1
static const int CN_Strip_Volume = 1* 4* 2* 2* 64; // 1024

struct CN_T { 
#undef USE_NEW
#define USE_NEW 1


  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long CN_Size = 0;
  void CN_Init( void ) {
    CN_Size = 0;
  
  
  }
  void CN_Quit( void ) {
  
  }
};
