

static const int AP_buckets = (33+0) * (1);
static const int AP_map_ctx = (256+0) * (1);
static const int AP_mix_sets = (256+0) * (1);
static const int AP_adapt_cap_max = (255+0) * (1);
pdesc( AP_span_, 0, "100000000000" );
static const int AP_span = AP_span_.value * (1);
static const int AP_rate = (5+0) * (1);
pdesc( AP_map_weight_, 0, "11" );
static const int AP_map_weight = AP_map_weight_.value * (1);
pdesc( AP_p_floor_, 0, "100000" );
static const int AP_p_floor = AP_p_floor_.value * (1);
pdesc( AP_p_ceil_, 0, "1111111111100000" );
static const int AP_p_ceil = AP_p_ceil_.value * (1);
pdesc( AP_stretch_scale_, 0, "100000000" );
static const int AP_stretch_scale = AP_stretch_scale_.value * (1);
pdesc( AP_mix_shift_, 0, "10010" );
static const int AP_mix_shift = AP_mix_shift_.value * (1);
pdesc( AP_adapt_cap_, 0, "111100" );
static const int AP_adapt_cap = AP_adapt_cap_.value * (1);
pdesc( AP_adapt_offset_, 0, "10" );
static const int AP_adapt_offset = AP_adapt_offset_.value * (1);
pdesc( AP_cell_seed_, 0, "1000000000000000" );
static const int AP_cell_seed = AP_cell_seed_.value * (1);



struct AP_T { 
#undef USE_NEW
#define USE_NEW 1


  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long AP_Size = 0;
  void AP_Init( void ) {
    AP_Size = 0;
  
  
  }
  void AP_Quit( void ) {
  
  }
};
