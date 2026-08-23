

static const int GL_max_width = (65535+0) * (1);
static const int GL_no_symbol = (8192+0) * (1);
static const int GL_model_table_bytes = (520192+0) * (1);
static const int GL_bank_mirror_mask = (32752+0) * (1);
static const int GL_p2_row_pad = (234+0) * (1);
pdesc( GL_weight_min_neg_, 0, "1000000" );
static const int GL_weight_min_neg = GL_weight_min_neg_.value * (1);
pdesc( GL_weight_max_, 0, "10111111" );
static const int GL_weight_max = GL_weight_max_.value * (1);
pdesc( GL_p2_freq_rescale_total_, 0, "111010000000000" );
static const int GL_p2_freq_rescale_total = GL_p2_freq_rescale_total_.value * (1);
pdesc( GL_ctx_id3_limit_, 0, "1101000000000000" );
static const int GL_ctx_id3_limit = GL_ctx_id3_limit_.value * (1);
pdesc( GL_slack_shift_, 0, "111" );
static const int GL_slack_shift = GL_slack_shift_.value * (1);
pdesc( GL_slack_max_, 0, "100000000000000" );
static const int GL_slack_max = GL_slack_max_.value * (1);
pdesc( GL_seed_shift_, 0, "101" );
static const int GL_seed_shift = GL_seed_shift_.value * (1);
pdesc( GL_rescale_force_, 0, "11111011" );
static const int GL_rescale_force = GL_rescale_force_.value * (1);
pdesc( GL_rescale_bias_ratio_, 0, "10100" );
static const int GL_rescale_bias_ratio = GL_rescale_bias_ratio_.value * (1);
pdesc( GL_escape_floor_, 0, "100000000" );
static const int GL_escape_floor = GL_escape_floor_.value * (1);
pdesc( GL_escape_kick_, 0, "1111" );
static const int GL_escape_kick = GL_escape_kick_.value * (1);
pdesc( GL_blend_share_, 0, "10101" );
static const int GL_blend_share = GL_blend_share_.value * (1);
pdesc( GL_blend_lvl_scale_, 0, "1000" );
static const int GL_blend_lvl_scale = GL_blend_lvl_scale_.value * (1);



struct GL_T { 
#undef USE_NEW
#define USE_NEW 1


  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long GL_Size = 0;
  void GL_Init( void ) {
    GL_Size = 0;
  
  
  }
  void GL_Quit( void ) {
  
  }
};
