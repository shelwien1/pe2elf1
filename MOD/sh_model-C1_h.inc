
static const int C1_Cx_Volume = 1* (1<<8)* 255; // 65280

struct C1_T { 
#undef USE_NEW
#define USE_NEW 0


  C1_Cell C1_tbl[C1_Cx_Volume];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long C1_Size = 0
                              + (unsigned long long)sizeof(C1_Cell) * (unsigned long long)(C1_Cx_Volume);
  void C1_Init( void ) {
  
  }
  void C1_Quit( void ) {
  
  }
};
