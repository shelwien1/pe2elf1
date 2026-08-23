

static const int PA_tree = (256+0) * (1);
static const int PA_res_ctx = (6+0) * (1);
pdesc( PA_seed_, 0, "1" );
static const int PA_seed = PA_seed_.value * (1);
pdesc( PA_step_, 0, "11000" );
static const int PA_step = PA_step_.value * (1);
pdesc( PA_limit_, 0, "1000000000000" );
static const int PA_limit = PA_limit_.value * (1);



struct PA_T { 
#undef USE_NEW
#define USE_NEW 1


  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long PA_Size = 0;
  void PA_Init( void ) {
    PA_Size = 0;
  
  
  }
  void PA_Quit( void ) {
  
  }
};
