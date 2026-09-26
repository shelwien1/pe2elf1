
static const int C1_Cx_Volume = 1* C1_h.Size; // 1

struct C1_T { 
#undef USE_NEW
#define USE_NEW 1


  C1_Cell* C1_tbl;
  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long C1_Size = 0;
  void C1_Init( void ) {
    C1_Size = 0;
    C1_tbl = new C1_Cell[ tbl_n(C1_Cx_Volume) ];
    C1_Size += (unsigned long long)sizeof(C1_Cell) * (unsigned long long)tbl_n(C1_Cx_Volume);
  
  }
  void C1_Quit( void ) {
    delete[] C1_tbl;
  
  }
};
