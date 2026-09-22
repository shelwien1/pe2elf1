
static const int C0_Cx_Volume = 1* (1<<8)* 255; // 65280

struct C0_T { 
#undef USE_NEW
#define USE_NEW 0


  C0_Cell C0_tbl[C0_Cx_Volume];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long C0_Size = 0
                              + (unsigned long long)sizeof(C0_Cell) * (unsigned long long)(C0_Cx_Volume);
  void C0_Init( void ) {
  
  }
  void C0_Quit( void ) {
  
  }
};
