
static const int C0_Cx_Volume = 1* C0_h.Size* C0_z.Size* C0_s.Size; // 2

struct C0_T { 
#undef USE_NEW
#define USE_NEW 1


  C0_Cell* C0_tbl;
  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long C0_Size = 0;
  void C0_Init( void ) {
    C0_Size = 0;
    C0_tbl = new C0_Cell[ tbl_n(C0_Cx_Volume) ];
    C0_Size += (unsigned long long)sizeof(C0_Cell) * (unsigned long long)tbl_n(C0_Cx_Volume);
  
  }
  void C0_Quit( void ) {
    delete[] C0_tbl;
  
  }
};
