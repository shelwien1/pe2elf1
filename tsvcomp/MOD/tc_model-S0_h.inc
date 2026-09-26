
static const int S0_Cx_Volume = 1* S0_k.Size* S0_s.Size* S0_z.Size; // 16

struct S0_T { 
#undef USE_NEW
#define USE_NEW 1


  SSE_Cell* S0_tbl;
  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long S0_Size = 0;
  void S0_Init( void ) {
    S0_Size = 0;
    S0_tbl = new SSE_Cell[ tbl_n(S0_Cx_Volume * CP_S0::NB) ];
    S0_Size += (unsigned long long)sizeof(SSE_Cell) * (unsigned long long)tbl_n(S0_Cx_Volume * CP_S0::NB);
  
  }
  void S0_Quit( void ) {
    delete[] S0_tbl;
  
  }
};
