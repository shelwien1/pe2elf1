

struct P0_T { 
#undef USE_NEW
#define USE_NEW 1


  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long P0_Size = 0;
  void P0_Init( void ) {
    P0_Size = 0;
  
  
  }
  void P0_Quit( void ) {
  
  
  }
};
