
static const int M3_Cx_Volume = 1* M3_k.Size* M3_s.Size* M3_p.Size; // 4

struct M3_T { 
#undef USE_NEW
#define USE_NEW 1


  Mix2d_Cell* M3_tbl;
  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long M3_Size = 0;
  void M3_Init( void ) {
    M3_Size = 0;
    M3_tbl = new Mix2d_Cell[ tbl_n(M3_Cx_Volume) ];
    M3_Size += (unsigned long long)sizeof(Mix2d_Cell) * (unsigned long long)tbl_n(M3_Cx_Volume);
  
  }
  void M3_Quit( void ) {
    delete[] M3_tbl;
  
  }
};
