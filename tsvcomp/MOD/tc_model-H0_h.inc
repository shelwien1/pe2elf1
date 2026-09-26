

struct H0_T { 
#undef USE_NEW
#define USE_NEW 1


  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long H0_Size = 0;
  void H0_Init( void ) {
    H0_Size = 0;
  
  
  }
  void H0_Quit( void ) {
  
  
  }
};
