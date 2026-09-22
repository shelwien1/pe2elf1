
static const int S0_Cx_Volume = 1* (1<<6)* (1<<5)* 255; // 522240
static const int S0_Cx3_Volume = 1* (1<<8); // 256

struct S0_T { 
#undef USE_NEW
#define USE_NEW 0

#if USE_NEW
  SSE_Tbl<CP_S0> S0_tbl;
#else
  SSE_Row<sse_nb_clamp(CP_S0::NB)> S0_tbl[ sse_row_count( qword(S0_Cx_Volume)*S0_Cx3_Volume ) ];
#endif

  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long S0_Size = 0;
  void S0_Init( void ) {
  #if USE_NEW
    S0_tbl.Init( tbl_n(sse_row_count( qword(S0_Cx_Volume)*S0_Cx3_Volume )) );
    S0_Size += S0_tbl.Bytes();
  #endif
  }
  void S0_Quit( void ) {
  #if USE_NEW
    S0_tbl.Quit();
  #endif
  }
};
