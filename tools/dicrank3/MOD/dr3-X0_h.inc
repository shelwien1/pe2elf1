
static const int X0_Cx_Volume = 1* (1<<3)* (1<<2)* (1<<2)* 16; // 2048

struct X0_T { 
#undef USE_NEW
#define USE_NEW 0


  MixX0_Cell X0_tbl[X0_Cx_Volume];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long X0_Size = 0
                              + (unsigned long long)sizeof(MixX0_Cell) * (unsigned long long)(X0_Cx_Volume);
  void X0_Init( void ) {
  
  }
  void X0_Quit( void ) {
  
  }
};
