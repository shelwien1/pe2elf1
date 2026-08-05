
static const byte M0_d0[72+1]={ 0,0,1,1,1,1,2,2,3,4,5,6,7,8,9,10,11,12,12,12,13,13,13,13,14,14,14,15,15,15,15,15,16,16,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17,17 };


static const int M0_MSMAP = (1+0) * (1);
static const int M0_MSCONF = (0+0) * (1);
static const int M0_MSDSH = (12+0) * (1);


static const int M0_Md_Volume = 1* 18; // 18

static const int M0_USE_NEW = 1;

struct M0_T { 
#undef USE_NEW
#define USE_NEW 1


  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long M0_Size = 0;
  void M0_Init( void ) {
    M0_Size = 0;
  
  
  }
  void M0_Quit( void ) {
  
  }
};
