

static const int CM_max_neighbours = (24+0) * (1);
static const int CM_orders = (4+0) * (1);
static const int CM_max_alpha = (16+0) * (1);
static const int CM_match_lens = (32+0) * (1);
static const int CM_count_cap_max = (255+0) * (1);
pdesc( CM_short_nb_, 0, "100" );
static const int CM_short_nb = CM_short_nb_.value * (1);
pdesc( CM_table_bits_, 0, "10100" );
static const int CM_table_bits = CM_table_bits_.value * (1);
pdesc( CM_mid_bits_, 0, "10010" );
static const int CM_mid_bits = CM_mid_bits_.value * (1);
pdesc( CM_match_bits_, 0, "10100" );
static const int CM_match_bits = CM_match_bits_.value * (1);
pdesc( CM_nb_for_bits_1_, 0, "11000" );
static const int CM_nb_for_bits_1 = CM_nb_for_bits_1_.value * (1);
pdesc( CM_nb_for_bits_2_, 0, "1010" );
static const int CM_nb_for_bits_2 = CM_nb_for_bits_2_.value * (1);
pdesc( CM_nb_for_bits_3_, 0, "111" );
static const int CM_nb_for_bits_3 = CM_nb_for_bits_3_.value * (1);
pdesc( CM_nb_for_bits_4_, 0, "111" );
static const int CM_nb_for_bits_4 = CM_nb_for_bits_4_.value * (1);
pdesc( CM_count_cap_, 0, "111100" );
static const int CM_count_cap = CM_count_cap_.value * (1);
pdesc( CM_adapt_offset_, 0, "10" );
static const int CM_adapt_offset = CM_adapt_offset_.value * (1);
pdesc( CM_blend_, 0, "11000" );
static const int CM_blend = CM_blend_.value * (1);
pdesc( CM_nbucket_shift_, 0, "11" );
static const int CM_nbucket_shift = CM_nbucket_shift_.value * (1);
pdesc( CM_match_span_, 0, "1100" );
static const int CM_match_span = CM_match_span_.value * (1);
pdesc( CM_match_cap_, 0, "101000" );
static const int CM_match_cap = CM_match_cap_.value * (1);
pdesc( CM_match_seed_, 0, "1001110001000000" );
static const int CM_match_seed = CM_match_seed_.value * (1);
pdesc( CM_probe_shift_, 0, "10001" );
static const int CM_probe_shift = CM_probe_shift_.value * (1);
pdesc( CM_probe_bands_, 0, "1000" );
static const int CM_probe_bands = CM_probe_bands_.value * (1);
pdesc( CM_probe_num_1_, 0, "101" );
static const int CM_probe_num_1 = CM_probe_num_1_.value * (1);
pdesc( CM_probe_den_1_, 0, "110" );
static const int CM_probe_den_1 = CM_probe_den_1_.value * (1);
pdesc( CM_probe_round_1_, 0, "11" );
static const int CM_probe_round_1 = CM_probe_round_1_.value * (1);
pdesc( CM_probe_num_2_, 0, "10" );
static const int CM_probe_num_2 = CM_probe_num_2_.value * (1);
pdesc( CM_probe_den_2_, 0, "11" );
static const int CM_probe_den_2 = CM_probe_den_2_.value * (1);
pdesc( CM_probe_round_2_, 0, "1" );
static const int CM_probe_round_2 = CM_probe_round_2_.value * (1);
pdesc( CM_probe_num_3_, 0, "1" );
static const int CM_probe_num_3 = CM_probe_num_3_.value * (1);
pdesc( CM_probe_den_3_, 0, "10" );
static const int CM_probe_den_3 = CM_probe_den_3_.value * (1);
pdesc( CM_probe_round_3_, 0, "1" );
static const int CM_probe_round_3 = CM_probe_round_3_.value * (1);
pdesc( CM_probe_bias_, 0, "110" );
static const int CM_probe_bias = CM_probe_bias_.value * (1);
pdesc( CM_key_hash_, 0, "10011110001101110111100110110001" );
static const int CM_key_hash = CM_key_hash_.value * (1);
pdesc( CM_cell_seed_, 0, "1000000000000000" );
static const int CM_cell_seed = CM_cell_seed_.value * (1);



struct CM_T { 
#undef USE_NEW
#define USE_NEW 1


  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long CM_Size = 0;
  void CM_Init( void ) {
    CM_Size = 0;
  
  
  }
  void CM_Quit( void ) {
  
  }
};
