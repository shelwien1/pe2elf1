
static const byte C0_l0[24+1]={ 0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,16 };




static const int C0_Ml_Volume = 1* 17; // 17

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
