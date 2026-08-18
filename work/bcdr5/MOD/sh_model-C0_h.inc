

static const int C0_SINGLE_wr = (eSCALE/(386+16));
static const int C0_SINGLE_mw = (904+1) * (1);
pdesc( C0_BITS_wr_, 16, "00000101110000" );
static const int C0_BITS_wr = eSCALE/C0_BITS_wr_.value;
pdesc( C0_BITS_mw_, 1, "00001010000100" );
static const int C0_BITS_mw = C0_BITS_mw_.value * (1);
pdesc( C0_BITS7_wr_, 16, "00100000100100" );
static const int C0_BITS7_wr = eSCALE/C0_BITS7_wr_.value;
pdesc( C0_BITS7_mw_, 1, "00000111000100" );
static const int C0_BITS7_mw = C0_BITS7_mw_.value * (1);
static const int C0_EMPTY_wr = (eSCALE/(516+16));
static const int C0_EMPTY_mw = (553+1) * (1);
static const int C0_FULL_wr = (eSCALE/(219+16));
static const int C0_FULL_mw = (548+1) * (1);



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
