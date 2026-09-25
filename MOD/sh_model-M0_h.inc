
static const int M0_Cx_Volume = 1* (1<<8)* (1<<2)* 255; // 261120

struct M0_T { 
#undef USE_NEW
#define USE_NEW 0


  Mix2_Cell M0_tbl[M0_Cx_Volume];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M0_Size = 0
                              + (unsigned long long)sizeof(Mix2_Cell) * (unsigned long long)(M0_Cx_Volume);
  void M0_Init( void ) {
  
  }
  void M0_Quit( void ) {
  
  }
};
