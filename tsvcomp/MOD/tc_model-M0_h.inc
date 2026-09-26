
static const int M0_Cx_Volume = 1* M0_k.Size* M0_s.Size* M0_e.Size; // 24

struct M0_T { 
#undef USE_NEW
#define USE_NEW 1


  Mix2_Cell* M0_tbl;
  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long M0_Size = 0;
  void M0_Init( void ) {
    M0_Size = 0;
    M0_tbl = new Mix2_Cell[ tbl_n(M0_Cx_Volume) ];
    M0_Size += (unsigned long long)sizeof(Mix2_Cell) * (unsigned long long)tbl_n(M0_Cx_Volume);
  
  }
  void M0_Quit( void ) {
    delete[] M0_tbl;
  
  }
};
