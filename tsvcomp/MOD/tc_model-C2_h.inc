
static const int C2_Cx_Volume = 1* (1<<19); // 524288

struct C2_T { 
#undef USE_NEW
#define USE_NEW 0


  C2_Cell C2_tbl[C2_Cx_Volume];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long C2_Size = 0
                              + (unsigned long long)sizeof(C2_Cell) * (unsigned long long)(C2_Cx_Volume);
  void C2_Init( void ) {
  
  }
  void C2_Quit( void ) {
  
  }
};
