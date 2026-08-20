
pdesc( M0_d0, 1, "010001011111111110010001001000010100000000000000000000000000000000000000" );


pdesc( M0_MSMAP_, 0, "01" );
static const int M0_MSMAP = M0_MSMAP_.value * (1);
pdesc( M0_MSCONF_, 0, "0" );
static const int M0_MSCONF = M0_MSCONF_.value * (1);
pdesc( M0_MSDSH_, 0, "01100" );
static const int M0_MSDSH = M0_MSDSH_.value * (1);


static const int M0_Md_Volume = 1* M0_d0.Size; // 18

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
