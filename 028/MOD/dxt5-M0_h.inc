
pmask( M0_h1, "00000100000011001110" );
pmask( M0_h1t, "011" );

pmask( M0_h2, "11111111111110111110" );

pmask( M0_me, "1" );
pmask( M0_mh, "1" );
pmask( M0_mmt, "100" );


pdesc( M0_M1_HB_, 1, "01010" );
static const int M0_M1_HB = M0_M1_HB_.value * (1);
pdesc( M0_M2_HB_, 1, "10001" );
static const int M0_M2_HB = M0_M2_HB_.value * (1);
pdesc( M0_HB_A_, 1, "11111" );
static const int M0_HB_A = M0_HB_A_.value * (1);
pdesc( M0_HB_C_, 1, "11111" );
static const int M0_HB_C = M0_HB_C_.value * (1);
pdesc( M0_HB_AW_, 1, "00000" );
static const int M0_HB_AW = M0_HB_AW_.value * (1);
pdesc( M0_HB_AW2_, 1, "11110" );
static const int M0_HB_AW2 = M0_HB_AW2_.value * (1);
pdesc( M0_HB_CW_, 1, "10110" );
static const int M0_HB_CW = M0_HB_CW_.value * (1);
pdesc( M0_HB_CW2_, 1, "11111" );
static const int M0_HB_CW2 = M0_HB_CW2_.value * (1);


static const int M0_M1_Volume = 1* M0_h1.Size* M0_h1t.Size* 2; // 2
static const int M0_M2_Volume = 1* M0_h2.Size; // 1
static const int M0_MM_Volume = 1* M0_me.Size* M0_mh.Size* M0_mmt.Size; // 1

struct M0_T { 
#undef USE_NEW
#define USE_NEW 1


  Ctr* M0_tAM1;
  Ctr* M0_tAMH;
  Ctr* M0_tAN1;
  Ctr* M0_tCM1;
  Ctr* M0_tCMH;
  Ctr* M0_tCMP;
  Ctr* M0_tCN1;
  Ctr* M0_tAW1;
  Ctr* M0_tAWH;
  Ctr* M0_tAWH2;
  Ctr* M0_tAN1w;
  Ctr* M0_tCW1;
  Ctr* M0_tCWH;
  Ctr* M0_tCWH2;
  Ctr* M0_tCN1w;
  Ctr* M0_tAM2;
  Ctr* M0_tAN2;
  Ctr* M0_tCM2;
  Ctr* M0_tCN2;
  Ctr* M0_tAW2;
  Ctr* M0_tAN2w;
  Ctr* M0_tCW2;
  Ctr* M0_tCN2w;
  int* M0_mxAMWw;
  int* M0_mxAMNw;
  int* M0_mxAMHw;
  int* M0_mxCMWw;
  int* M0_mxCMNw;
  int* M0_mxCMHw;
  int* M0_mxCMPw;
  int* M0_mxAWWw;
  int* M0_mxAWNw;
  int* M0_mxAWHw;
  int* M0_mxAWH2w;
  int* M0_mxCWWw;
  int* M0_mxCWNw;
  int* M0_mxCWHw;
  int* M0_mxCWH2w;
  u32* M0_HA;
  u64* M0_HC;
  u64* M0_HAW;
  u8* M0_HAWv;
  u64* M0_HAW2;
  u8* M0_HAW2v;
  u64* M0_HCW;
  u8* M0_HCWv;
  u64* M0_HCW2;
  u8* M0_HCW2v;
  
  /* Total bytes this module's Table() members occupy.  Accumulated in _Init()
   * because the counts are load-time values here. */
  unsigned long long M0_Size = 0;
  void M0_Init( void ) {
    M0_Size = 0;
    M0_tAM1 = new Ctr[ tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB)));
    M0_tAMH = new Ctr[ tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB)));
    M0_tAN1 = new Ctr[ tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB)));
    M0_tCM1 = new Ctr[ tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB)));
    M0_tCMH = new Ctr[ tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB)));
    M0_tCMP = new Ctr[ tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB)));
    M0_tCN1 = new Ctr[ tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB)));
    M0_tAW1 = new Ctr[ tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB)));
    M0_tAWH = new Ctr[ tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB)));
    M0_tAWH2 = new Ctr[ tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB)));
    M0_tAN1w = new Ctr[ tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB)));
    M0_tCW1 = new Ctr[ tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB)));
    M0_tCWH = new Ctr[ tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB)));
    M0_tCWH2 = new Ctr[ tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB)));
    M0_tCN1w = new Ctr[ tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M1_Volume, PCAP(M0_M1_HB)));
    M0_tAM2 = new Ctr[ tbl_n(cap_vol(M0_M2_Volume, PCAP(M0_M2_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M2_Volume, PCAP(M0_M2_HB)));
    M0_tAN2 = new Ctr[ tbl_n(cap_vol(M0_M2_Volume, PCAP(M0_M2_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M2_Volume, PCAP(M0_M2_HB)));
    M0_tCM2 = new Ctr[ tbl_n(cap_vol(M0_M2_Volume, PCAP(M0_M2_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M2_Volume, PCAP(M0_M2_HB)));
    M0_tCN2 = new Ctr[ tbl_n(cap_vol(M0_M2_Volume, PCAP(M0_M2_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M2_Volume, PCAP(M0_M2_HB)));
    M0_tAW2 = new Ctr[ tbl_n(cap_vol(M0_M2_Volume, PCAP(M0_M2_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M2_Volume, PCAP(M0_M2_HB)));
    M0_tAN2w = new Ctr[ tbl_n(cap_vol(M0_M2_Volume, PCAP(M0_M2_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M2_Volume, PCAP(M0_M2_HB)));
    M0_tCW2 = new Ctr[ tbl_n(cap_vol(M0_M2_Volume, PCAP(M0_M2_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M2_Volume, PCAP(M0_M2_HB)));
    M0_tCN2w = new Ctr[ tbl_n(cap_vol(M0_M2_Volume, PCAP(M0_M2_HB))) ];
    M0_Size += (unsigned long long)sizeof(Ctr) * (unsigned long long)tbl_n(cap_vol(M0_M2_Volume, PCAP(M0_M2_HB)));
    M0_mxAMWw = new int[ tbl_n(4 * M0_MM_Volume) ];
    M0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(4 * M0_MM_Volume);
    M0_mxAMNw = new int[ tbl_n(4 * M0_MM_Volume) ];
    M0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(4 * M0_MM_Volume);
    M0_mxAMHw = new int[ tbl_n(4 * M0_MM_Volume) ];
    M0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(4 * M0_MM_Volume);
    M0_mxCMWw = new int[ tbl_n(4 * M0_MM_Volume) ];
    M0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(4 * M0_MM_Volume);
    M0_mxCMNw = new int[ tbl_n(4 * M0_MM_Volume) ];
    M0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(4 * M0_MM_Volume);
    M0_mxCMHw = new int[ tbl_n(4 * M0_MM_Volume) ];
    M0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(4 * M0_MM_Volume);
    M0_mxCMPw = new int[ tbl_n(4 * M0_MM_Volume) ];
    M0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(4 * M0_MM_Volume);
    M0_mxAWWw = new int[ tbl_n(4 * M0_MM_Volume) ];
    M0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(4 * M0_MM_Volume);
    M0_mxAWNw = new int[ tbl_n(4 * M0_MM_Volume) ];
    M0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(4 * M0_MM_Volume);
    M0_mxAWHw = new int[ tbl_n(4 * M0_MM_Volume) ];
    M0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(4 * M0_MM_Volume);
    M0_mxAWH2w = new int[ tbl_n(4 * M0_MM_Volume) ];
    M0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(4 * M0_MM_Volume);
    M0_mxCWWw = new int[ tbl_n(4 * M0_MM_Volume) ];
    M0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(4 * M0_MM_Volume);
    M0_mxCWNw = new int[ tbl_n(4 * M0_MM_Volume) ];
    M0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(4 * M0_MM_Volume);
    M0_mxCWHw = new int[ tbl_n(4 * M0_MM_Volume) ];
    M0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(4 * M0_MM_Volume);
    M0_mxCWH2w = new int[ tbl_n(4 * M0_MM_Volume) ];
    M0_Size += (unsigned long long)sizeof(int) * (unsigned long long)tbl_n(4 * M0_MM_Volume);
    M0_HA = new u32[ tbl_n(size_t(1) << PHBITS(M0_HB_A)) ];
    M0_Size += (unsigned long long)sizeof(u32) * (unsigned long long)tbl_n(size_t(1) << PHBITS(M0_HB_A));
    M0_HC = new u64[ tbl_n(size_t(1) << PHBITS(M0_HB_C)) ];
    M0_Size += (unsigned long long)sizeof(u64) * (unsigned long long)tbl_n(size_t(1) << PHBITS(M0_HB_C));
    M0_HAW = new u64[ tbl_n(size_t(1) << PHBITS(M0_HB_AW)) ];
    M0_Size += (unsigned long long)sizeof(u64) * (unsigned long long)tbl_n(size_t(1) << PHBITS(M0_HB_AW));
    M0_HAWv = new u8[ tbl_n(size_t(1) << PHBITS(M0_HB_AW)) ];
    M0_Size += (unsigned long long)sizeof(u8) * (unsigned long long)tbl_n(size_t(1) << PHBITS(M0_HB_AW));
    M0_HAW2 = new u64[ tbl_n(size_t(1) << PHBITS(M0_HB_AW2)) ];
    M0_Size += (unsigned long long)sizeof(u64) * (unsigned long long)tbl_n(size_t(1) << PHBITS(M0_HB_AW2));
    M0_HAW2v = new u8[ tbl_n(size_t(1) << PHBITS(M0_HB_AW2)) ];
    M0_Size += (unsigned long long)sizeof(u8) * (unsigned long long)tbl_n(size_t(1) << PHBITS(M0_HB_AW2));
    M0_HCW = new u64[ tbl_n(size_t(1) << PHBITS(M0_HB_CW)) ];
    M0_Size += (unsigned long long)sizeof(u64) * (unsigned long long)tbl_n(size_t(1) << PHBITS(M0_HB_CW));
    M0_HCWv = new u8[ tbl_n(size_t(1) << PHBITS(M0_HB_CW)) ];
    M0_Size += (unsigned long long)sizeof(u8) * (unsigned long long)tbl_n(size_t(1) << PHBITS(M0_HB_CW));
    M0_HCW2 = new u64[ tbl_n(size_t(1) << PHBITS(M0_HB_CW2)) ];
    M0_Size += (unsigned long long)sizeof(u64) * (unsigned long long)tbl_n(size_t(1) << PHBITS(M0_HB_CW2));
    M0_HCW2v = new u8[ tbl_n(size_t(1) << PHBITS(M0_HB_CW2)) ];
    M0_Size += (unsigned long long)sizeof(u8) * (unsigned long long)tbl_n(size_t(1) << PHBITS(M0_HB_CW2));
  
  }
  void M0_Quit( void ) {
    delete[] M0_tAM1;
    delete[] M0_tAMH;
    delete[] M0_tAN1;
    delete[] M0_tCM1;
    delete[] M0_tCMH;
    delete[] M0_tCMP;
    delete[] M0_tCN1;
    delete[] M0_tAW1;
    delete[] M0_tAWH;
    delete[] M0_tAWH2;
    delete[] M0_tAN1w;
    delete[] M0_tCW1;
    delete[] M0_tCWH;
    delete[] M0_tCWH2;
    delete[] M0_tCN1w;
    delete[] M0_tAM2;
    delete[] M0_tAN2;
    delete[] M0_tCM2;
    delete[] M0_tCN2;
    delete[] M0_tAW2;
    delete[] M0_tAN2w;
    delete[] M0_tCW2;
    delete[] M0_tCN2w;
    delete[] M0_mxAMWw;
    delete[] M0_mxAMNw;
    delete[] M0_mxAMHw;
    delete[] M0_mxCMWw;
    delete[] M0_mxCMNw;
    delete[] M0_mxCMHw;
    delete[] M0_mxCMPw;
    delete[] M0_mxAWWw;
    delete[] M0_mxAWNw;
    delete[] M0_mxAWHw;
    delete[] M0_mxAWH2w;
    delete[] M0_mxCWWw;
    delete[] M0_mxCWNw;
    delete[] M0_mxCWHw;
    delete[] M0_mxCWH2w;
    delete[] M0_HA;
    delete[] M0_HC;
    delete[] M0_HAW;
    delete[] M0_HAWv;
    delete[] M0_HAW2;
    delete[] M0_HAW2v;
    delete[] M0_HCW;
    delete[] M0_HCWv;
    delete[] M0_HCW2;
    delete[] M0_HCW2v;
  }
};
