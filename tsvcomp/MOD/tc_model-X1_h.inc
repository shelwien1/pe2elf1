
static const int X1_Cx_Volume = 1* X1_k.Size* X1_s.Size* X1_z.Size* X1_c.Size; // 16

struct X1_T { 
#undef USE_NEW
#define USE_NEW 1


  MixX1_Cell* X1_tbl;
  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long X1_Size = 0;
  void X1_Init( void ) {
    X1_Size = 0;
    X1_tbl = new MixX1_Cell[ tbl_n(X1_Cx_Volume) ];
    X1_Size += (unsigned long long)sizeof(MixX1_Cell) * (unsigned long long)tbl_n(X1_Cx_Volume);
  
  }
  void X1_Quit( void ) {
    delete[] X1_tbl;
  
  }
};
