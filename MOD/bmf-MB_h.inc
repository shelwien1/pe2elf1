

static const int MB_row_margin = (7+0) * (1);
static const int MB_row_tail = (9+0) * (1);
static const int MB_freq_table_offset = (188+0) * (1);
static const int MB_freq_grid_count = (65723+0) * (1);
static const int MB_sel1_syms = (99+0) * (1);
static const int MB_sel0_syms = (33+0) * (1);
pdesc( MB_near_lo_, 0, "1011" );
static const int MB_near_lo = MB_near_lo_.value * (1);
pdesc( MB_near_hi_, 0, "10000" );
static const int MB_near_hi = MB_near_hi_.value * (1);
pdesc( MB_far_hi_, 0, "100000" );
static const int MB_far_hi = MB_far_hi_.value * (1);
pdesc( MB_exact_rank_, 0, "1010" );
static const int MB_exact_rank = MB_exact_rank_.value * (1);
pdesc( MB_top_depth_0_, 0, "1010" );
static const int MB_top_depth_0 = MB_top_depth_0_.value * (1);
pdesc( MB_top_depth_1_, 0, "100" );
static const int MB_top_depth_1 = MB_top_depth_1_.value * (1);
pdesc( MB_top_depth_2_, 0, "1010" );
static const int MB_top_depth_2 = MB_top_depth_2_.value * (1);
pdesc( MB_top_depth_3_, 0, "110" );
static const int MB_top_depth_3 = MB_top_depth_3_.value * (1);
pdesc( MB_pos_bare_, 0, "110" );
static const int MB_pos_bare = MB_pos_bare_.value * (1);
pdesc( MB_pos_split_, 0, "1110" );
static const int MB_pos_split = MB_pos_split_.value * (1);
pdesc( MB_pos_deep_, 0, "1001" );
static const int MB_pos_deep = MB_pos_deep_.value * (1);
pdesc( MB_apm_pos_cap_, 0, "111" );
static const int MB_apm_pos_cap = MB_apm_pos_cap_.value * (1);
pdesc( MB_small_alphabet_, 0, "100000" );
static const int MB_small_alphabet = MB_small_alphabet_.value * (1);



struct MB_T { 
#undef USE_NEW
#define USE_NEW 1


  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long MB_Size = 0;
  void MB_Init( void ) {
    MB_Size = 0;
  
  
  }
  void MB_Quit( void ) {
  
  }
};
