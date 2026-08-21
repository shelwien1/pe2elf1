
pdesc( C0_l0, 1, "000000011111111111111111" );




static const int C0_Ml_Volume = 1* C0_l0.Size; // 18

static const int C0_USE_NEW = 1;

struct C0_T { 
#undef USE_NEW
#define USE_NEW 1


  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long C0_Size = 0;
  void C0_Init( void ) {
    C0_Size = 0;
  
  
  }
  void C0_Quit( void ) {
  
  }
};
