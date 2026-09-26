
static const int X0_Cx_Volume = 1* X0_g.Size* X0_k.Size* X0_s.Size* X0_e.Size; // 28

struct X0_T { 
#undef USE_NEW
#define USE_NEW 1


  MixX0_Cell* X0_tbl;
  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long X0_Size = 0;
  void X0_Init( void ) {
    X0_Size = 0;
    X0_tbl = new MixX0_Cell[ tbl_n(X0_Cx_Volume) ];
    X0_Size += (unsigned long long)sizeof(MixX0_Cell) * (unsigned long long)tbl_n(X0_Cx_Volume);
  
  }
  void X0_Quit( void ) {
    delete[] X0_tbl;
  
  }
};
