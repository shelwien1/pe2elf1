
static const int S0_Cx_Volume = 1* (1<<5)* 255; // 8160

struct S0_T { 
#undef USE_NEW
#define USE_NEW 0


  SSE_Cell S0_tbl[S0_Cx_Volume * CP_S0::NB];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long S0_Size = 0
                              + (unsigned long long)sizeof(SSE_Cell) * (unsigned long long)(S0_Cx_Volume * CP_S0::NB);
  void S0_Init( void ) {
  
  }
  void S0_Quit( void ) {
  
  }
};
