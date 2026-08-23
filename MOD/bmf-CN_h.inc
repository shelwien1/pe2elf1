



static const int CN_slot_split = (5+0) * (1);
static const int CN_slot_top = (6+0) * (1);
static const int CN_seed_0 = (8+0) * (1);
static const int CN_seed_1 = (2+0) * (1);
static const int CN_seed_2 = (2+0) * (1);
static const int CN_seed_3 = (2+0) * (1);
static const int CN_seed_4 = (2+0) * (1);
static const int CN_seed_5 = (3+0) * (1);
static const int CN_seed_6 = (3+0) * (1);
static const int CN_bump = (32+0) * (1);
static const int CN_rescale_at = (8192+0) * (1);
static const int CN_halve_shift = (1+0) * (1);
static const int CN_rescale_round = (2+0) * (1);
static const int CN_rescale_div = (3+0) * (1);
static const int CN_escape_bias = (96+0) * (1);
static const int CN_nudge_limit = (3276+0) * (1);
static const int CN_step = (8+0) * (1);
static const int CN_seed_bump = (4+0) * (1);
static const int CN_seed_limit = (512+0) * (1);
static const int CN_limit_step = (64+0) * (1);
static const int CN_max_limit = (16384+0) * (1);
static const int CN_parent_bump = (136+0) * (1);
static const int CN_parent_limit = (72+0) * (1);
static const int CN_root_n0 = (40+0) * (1);
static const int CN_root_n1 = (16+0) * (1);
static const int CN_seed_scale = (64+0) * (1);
static const int CN_seed_decay = (3+0) * (1);
static const int CN_p16_floor = (32+0) * (1);
static const int CN_p16_ceil = (65504+0) * (1);
static const int CN_drift_limit = (16+0) * (1);


static const int CN_SlotMap_Volume = 1* (1<<4); // 16
static const int CN_Strip_Volume = 1* 4* 2* 2* 64; // 1024

struct CN_T { 
#undef USE_NEW
#define USE_NEW 0


  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long CN_Size = 0;
  void CN_Init( void ) {
  
  }
  void CN_Quit( void ) {
  }
};
