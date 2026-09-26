
static const int M1_Cx_Volume = 1* M1_k.Size* M1_s.Size* M1_e.Size; // 24

struct M1_T { 
#undef USE_NEW
#define USE_NEW 1


  Mix2b_Cell* M1_tbl;
  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long M1_Size = 0;
  void M1_Init( void ) {
    M1_Size = 0;
    M1_tbl = new Mix2b_Cell[ tbl_n(M1_Cx_Volume) ];
    M1_Size += (unsigned long long)sizeof(Mix2b_Cell) * (unsigned long long)tbl_n(M1_Cx_Volume);
  
  }
  void M1_Quit( void ) {
    delete[] M1_tbl;
  
  }
};
