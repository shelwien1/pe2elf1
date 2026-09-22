
static const int S0_Cx_Volume = 1* (1<<6)* (1<<5)* 255; // 522240
static const int S0_Cx3_Volume = 1* (1<<8); // 256

struct S0_T { 
#undef USE_NEW
#define USE_NEW 0


  SSE_Cell S0_tbl[sse_table_cells( qword(S0_Cx_Volume)*S0_Cx3_Volume )];
  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long S0_Size = 0
                              + (unsigned long long)sizeof(SSE_Cell) * (unsigned long long)(sse_table_cells( qword(S0_Cx_Volume)*S0_Cx3_Volume ));
  void S0_Init( void ) {
  
  }
  void S0_Quit( void ) {
  
  }
};
