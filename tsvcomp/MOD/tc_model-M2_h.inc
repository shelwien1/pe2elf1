
static const int M2_Cx_Volume = 1* (1<<1)* 4* (1<<6); // 512

struct M2_T { 
#undef USE_NEW
#define USE_NEW 0


  Mix2c_Cell M2_tbl[M2_Cx_Volume];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M2_Size = 0
                              + (unsigned long long)sizeof(Mix2c_Cell) * (unsigned long long)(M2_Cx_Volume);
  void M2_Init( void ) {
  
  }
  void M2_Quit( void ) {
  
  }
};
