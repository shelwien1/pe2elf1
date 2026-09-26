
static const int C2_Cx_Volume = 1* C2_h.Size; // 1

struct C2_T { 
#undef USE_NEW
#define USE_NEW 1


  C2_Cell* C2_tbl;
  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long C2_Size = 0;
  void C2_Init( void ) {
    C2_Size = 0;
    C2_tbl = new C2_Cell[ tbl_n(C2_Cx_Volume) ];
    C2_Size += (unsigned long long)sizeof(C2_Cell) * (unsigned long long)tbl_n(C2_Cx_Volume);
  
  }
  void C2_Quit( void ) {
    delete[] C2_tbl;
  
  }
};
