

static const int AP_span = (2048+0) * (1);
static const int AP_rate = (5+0) * (1);
static const int AP_map_weight = (3+0) * (1);
static const int AP_p_floor = (32+0) * (1);
static const int AP_p_ceil = (65504+0) * (1);
static const int AP_stretch_scale = (256+0) * (1);
static const int AP_mix_shift = (18+0) * (1);
static const int AP_adapt_cap = (60+0) * (1);
static const int AP_adapt_offset = (2+0) * (1);
static const int AP_cell_seed = (32768+0) * (1);



struct AP_T { 
#undef USE_NEW
#define USE_NEW 0


  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long AP_Size = 0;
  void AP_Init( void ) {
  
  }
  void AP_Quit( void ) {
  }
};
