
static const int S0_Cx_Volume = 1* (1<<8)* (1<<6)* (1<<5)* 255; // 133693440

struct S0_T { 
#undef USE_NEW
#define USE_NEW 0

#if USE_NEW
  SSE_Tbl<CP_S0> S0_tbl;
#else
  SSE_Row<sse_nb_clamp(CP_S0::NB)>* S0_tbl;
#endif

  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long S0_Size = 0;
  void S0_Init( void ) {
  #if USE_NEW
    S0_tbl.Init( tbl_n(S0_Cx_Volume) );
    S0_Size += S0_tbl.Bytes();
  #else
    S0_tbl = new SSE_Row<sse_nb_clamp(CP_S0::NB)>[ tbl_n(S0_Cx_Volume) ];
  #endif
  }
  void S0_Quit( void ) {
  #if USE_NEW
    S0_tbl.Quit();
  #else
    delete[] S0_tbl;
  #endif
  }
};
