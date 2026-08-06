

pdesc( G0_CW0_, 0, "0011111111110100" );
static const int G0_CW0 = G0_CW0_.value * (1);
pdesc( G0_CW1_, 0, "0011111111111101" );
static const int G0_CW1 = G0_CW1_.value * (1);
pdesc( G0_CW2_, 0, "0011100010001110" );
static const int G0_CW2 = G0_CW2_.value * (1);
pdesc( G0_CW3_, 0, "0010111111010000" );
static const int G0_CW3 = G0_CW3_.value * (1);
pdesc( G0_CW4_, 0, "0011000001011001" );
static const int G0_CW4 = G0_CW4_.value * (1);
pdesc( G0_CW5_, 0, "0010011111101000" );
static const int G0_CW5 = G0_CW5_.value * (1);
pdesc( G0_CW6_, 0, "0011000000100000" );
static const int G0_CW6 = G0_CW6_.value * (1);
pdesc( G0_CW7_, 0, "0010011000110101" );
static const int G0_CW7 = G0_CW7_.value * (1);
pdesc( G0_CW8_, 0, "0010001111010011" );
static const int G0_CW8 = G0_CW8_.value * (1);
pdesc( G0_CW9_, 0, "0010001001001000" );
static const int G0_CW9 = G0_CW9_.value * (1);
pdesc( G0_CW10_, 0, "0010000000000100" );
static const int G0_CW10 = G0_CW10_.value * (1);
pdesc( G0_CW11_, 0, "0001111110111000" );
static const int G0_CW11 = G0_CW11_.value * (1);
pdesc( G0_CW12_, 0, "0001110111110111" );
static const int G0_CW12 = G0_CW12_.value * (1);
pdesc( G0_CW13_, 0, "0001111010010011" );
static const int G0_CW13 = G0_CW13_.value * (1);
pdesc( G0_CW14_, 0, "0001101111110000" );
static const int G0_CW14 = G0_CW14_.value * (1);
pdesc( G0_CW15_, 0, "0000100011111011" );
static const int G0_CW15 = G0_CW15_.value * (1);
pdesc( G0_CW_NMAX_, 0, "01111" );
static const int G0_CW_NMAX = G0_CW_NMAX_.value * (1);
pdesc( G0_SLOW_MUL_, 0, "0011111111100100" );
static const int G0_SLOW_MUL = G0_SLOW_MUL_.value * (1);
pdesc( G0_MW_AEP_, 0, "0000001001111111" );
static const int G0_MW_AEP = G0_MW_AEP_.value * (1);
pdesc( G0_MW_E_, 0, "0000000111100010" );
static const int G0_MW_E = G0_MW_E_.value * (1);
pdesc( G0_MW_PAIR_, 0, "0000001000000000" );
static const int G0_MW_PAIR = G0_MW_PAIR_.value * (1);
pdesc( G0_MW_WORD_, 0, "0000001000010100" );
static const int G0_MW_WORD = G0_MW_WORD_.value * (1);
pdesc( G0_MW_AI_, 0, "0000000101111011" );
static const int G0_MW_AI = G0_MW_AI_.value * (1);
pdesc( G0_MW_CI_, 0, "0000000010110011" );
static const int G0_MW_CI = G0_MW_CI_.value * (1);
pdesc( G0_MW_X_, 0, "0000000111101001" );
static const int G0_MW_X = G0_MW_X_.value * (1);
pdesc( G0_MW_P_, 0, "0000000101010010" );
static const int G0_MW_P = G0_MW_P_.value * (1);
pdesc( G0_W_INIT_, 0, "0010101111100010" );
static const int G0_W_INIT = G0_W_INIT_.value * (1);
pdesc( G0_W_CLAMPL_, 1, "11100" );
static const int G0_W_CLAMPL = G0_W_CLAMPL_.value * (1);
pdesc( G0_BIAS_, 0, "0100010100" );
static const int G0_BIAS = G0_BIAS_.value * (1);
pdesc( G0_AB_AI_, 0, "01" );
static const int G0_AB_AI = G0_AB_AI_.value * (1);
pdesc( G0_AB_AI2_, 0, "10" );
static const int G0_AB_AI2 = G0_AB_AI2_.value * (1);
pdesc( G0_AB_CI_, 0, "10" );
static const int G0_AB_CI = G0_AB_CI_.value * (1);
pdesc( G0_AB_CI2_, 0, "10" );
static const int G0_AB_CI2 = G0_AB_CI2_.value * (1);
pdesc( G0_AP_N_AI_, 0, "00000001111" );
static const int G0_AP_N_AI = G0_AP_N_AI_.value * (1);
pdesc( G0_AP_R_AI_, 0, "0000011000011110" );
static const int G0_AP_R_AI = G0_AP_R_AI_.value * (1);
pdesc( G0_AP_N_AI2_, 0, "00001000001" );
static const int G0_AP_N_AI2 = G0_AP_N_AI2_.value * (1);
pdesc( G0_AP_R_AI2_, 0, "0000001100000010" );
static const int G0_AP_R_AI2 = G0_AP_R_AI2_.value * (1);
pdesc( G0_AP_N_CI_, 0, "00000010001" );
static const int G0_AP_N_CI = G0_AP_N_CI_.value * (1);
pdesc( G0_AP_R_CI_, 0, "0000001110010000" );
static const int G0_AP_R_CI = G0_AP_R_CI_.value * (1);
pdesc( G0_AP_N_CI2_, 0, "00000100000" );
static const int G0_AP_N_CI2 = G0_AP_N_CI2_.value * (1);
pdesc( G0_AP_R_CI2_, 0, "0000010000000000" );
static const int G0_AP_R_CI2 = G0_AP_R_CI2_.value * (1);



struct G0_T { 
#undef USE_NEW
#define USE_NEW 1


  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long G0_Size = 0;
  void G0_Init( void ) {
    G0_Size = 0;
  
  
  }
  void G0_Quit( void ) {
  
  }
};
