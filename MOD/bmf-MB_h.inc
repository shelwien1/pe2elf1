

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
pdesc( MB_group_flags_00_, 0, "0" );
static const int MB_group_flags_00 = MB_group_flags_00_.value * (1);
pdesc( MB_group_flags_01_, 0, "1" );
static const int MB_group_flags_01 = MB_group_flags_01_.value * (1);
pdesc( MB_group_flags_02_, 0, "10" );
static const int MB_group_flags_02 = MB_group_flags_02_.value * (1);
pdesc( MB_group_flags_03_, 0, "100" );
static const int MB_group_flags_03 = MB_group_flags_03_.value * (1);
pdesc( MB_group_flags_04_, 0, "1000" );
static const int MB_group_flags_04 = MB_group_flags_04_.value * (1);
pdesc( MB_group_flags_05_, 0, "1010" );
static const int MB_group_flags_05 = MB_group_flags_05_.value * (1);
pdesc( MB_group_flags_06_, 0, "1101" );
static const int MB_group_flags_06 = MB_group_flags_06_.value * (1);
pdesc( MB_group_flags_07_, 0, "10000" );
static const int MB_group_flags_07 = MB_group_flags_07_.value * (1);
pdesc( MB_group_flags_08_, 0, "10001" );
static const int MB_group_flags_08 = MB_group_flags_08_.value * (1);
pdesc( MB_group_flags_09_, 0, "10110" );
static const int MB_group_flags_09 = MB_group_flags_09_.value * (1);
pdesc( MB_group_flags_10_, 0, "100000" );
static const int MB_group_flags_10 = MB_group_flags_10_.value * (1);
pdesc( MB_group_flags_11_, 0, "100011" );
static const int MB_group_flags_11 = MB_group_flags_11_.value * (1);
pdesc( MB_group_flags_12_, 0, "100100" );
static const int MB_group_flags_12 = MB_group_flags_12_.value * (1);
pdesc( MB_group_flags_13_, 0, "111000" );
static const int MB_group_flags_13 = MB_group_flags_13_.value * (1);
pdesc( MB_group_flags_14_, 0, "111111" );
static const int MB_group_flags_14 = MB_group_flags_14_.value * (1);



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
