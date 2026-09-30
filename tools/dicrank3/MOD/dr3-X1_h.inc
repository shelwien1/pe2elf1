
static const int X1_Cx_Volume = 1* (1<<2)* (1<<2)* 16; // 256

struct X1_T { 
#undef USE_NEW
#define USE_NEW 0


  MixX1_Cell X1_tbl[X1_Cx_Volume];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long X1_Size = 0
                              + (unsigned long long)sizeof(MixX1_Cell) * (unsigned long long)(X1_Cx_Volume);
  void X1_Init( void ) {
  
  }
  void X1_Quit( void ) {
  
  }
};
