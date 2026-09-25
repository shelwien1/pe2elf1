
static const int M1_Cx_Volume = 1* (1<<1)* 4* 17; // 136

struct M1_T { 
#undef USE_NEW
#define USE_NEW 0


  Mix2b_Cell M1_tbl[M1_Cx_Volume];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M1_Size = 0
                              + (unsigned long long)sizeof(Mix2b_Cell) * (unsigned long long)(M1_Cx_Volume);
  void M1_Init( void ) {
  
  }
  void M1_Quit( void ) {
  
  }
};
