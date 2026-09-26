
static const int M2_Cx_Volume = 1* M2_k.Size* M2_s.Size* M2_p.Size; // 4

struct M2_T { 
#undef USE_NEW
#define USE_NEW 1


  Mix2c_Cell* M2_tbl;
  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long M2_Size = 0;
  void M2_Init( void ) {
    M2_Size = 0;
    M2_tbl = new Mix2c_Cell[ tbl_n(M2_Cx_Volume) ];
    M2_Size += (unsigned long long)sizeof(Mix2c_Cell) * (unsigned long long)tbl_n(M2_Cx_Volume);
  
  }
  void M2_Quit( void ) {
    delete[] M2_tbl;
  
  }
};
