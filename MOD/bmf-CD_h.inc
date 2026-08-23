

pdesc( CD_weight_shift_, 0, "111" );
static const int CD_weight_shift = CD_weight_shift_.value * (1);
pdesc( CD_weight_round_, 0, "101000" );
static const int CD_weight_round = CD_weight_round_.value * (1);
pdesc( CD_weight_bits_, 0, "1000" );
static const int CD_weight_bits = CD_weight_bits_.value * (1);
pdesc( CD_weight_bias_, 0, "1000000" );
static const int CD_weight_bias = CD_weight_bias_.value * (1);
pdesc( CD_fit_ridge_den_, 0, "1010" );
static const int CD_fit_ridge_den = CD_fit_ridge_den_.value * (1);
pdesc( CD_dc_window_lo_, 0, "100" );
static const int CD_dc_window_lo = CD_dc_window_lo_.value * (1);
pdesc( CD_dc_window_hi_, 0, "10000000000" );
static const int CD_dc_window_hi = CD_dc_window_hi_.value * (1);
pdesc( CD_dc_diff_lo_, 0, "0" );
static const int CD_dc_diff_lo = CD_dc_diff_lo_.value * (1);
pdesc( CD_dc_diff_hi_, 0, "1000000000" );
static const int CD_dc_diff_hi = CD_dc_diff_hi_.value * (1);



struct CD_T { 
#undef USE_NEW
#define USE_NEW 1


  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long CD_Size = 0;
  void CD_Init( void ) {
    CD_Size = 0;
  
  
  }
  void CD_Quit( void ) {
  
  }
};
