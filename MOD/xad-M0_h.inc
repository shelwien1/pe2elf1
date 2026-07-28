
pdesc( M0_d0, 1, "011011011010101001010001011010000000000000000000000000000000000000000000" );


pdesc( M0_MSMAP_, 0, "00" );
static const int M0_MSMAP = M0_MSMAP_.value * (1);
pdesc( M0_MSCONF_, 0, "0" );
static const int M0_MSCONF = M0_MSCONF_.value * (1);
pdesc( M0_MSDSH_, 0, "00101" );
static const int M0_MSDSH = M0_MSDSH_.value * (1);


static const int M0_Md_Volume = 1* M0_d0.Size; // 16

struct M0_T { 
#undef USE_NEW
#define USE_NEW 0


  
  /* Total bytes this module's Table() members occupy -- a constant here, since
   * Const mode has already folded every count. */
  static constexpr unsigned long long M0_Size = 0;
  void M0_Init( void ) {
  
  }
  void M0_Quit( void ) {
  }
};
