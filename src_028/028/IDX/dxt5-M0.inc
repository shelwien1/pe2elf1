
// M0: match history contexts


static inline uint MakeM1( uint hist, uint eq, uint mt ) {
  int M1 = 0;
  MakeIndex M1
  return M1;
}

static inline uint MakeM2( uint hist ) {
  int M2 = 0;
  MakeIndex M2
  return M2;
}

static inline uint MakeMM( uint eq, uint hist, uint mt ) {
  int MM = 0;
  MakeIndex MM
  return MM;
}


// ---- storage ---------------------------------------------------------
// Declared with the IDX Table() directive, so the same source produces
// pointers allocated in %M%Init() (Debug build, sizes are runtime values)
// or fixed-size arrays (Const build).  Codec inherits %M%T and binds its
// CtrTab / Mixer / APM views to these.
// 15 tables on the M1 context, 8 on M2 (see cap_vol)
Table( Ctr, %M%tAM1, cap_vol(%M%M1_Volume, PCAP(%M%M1_HB)) );
Table( Ctr, %M%tAMH, cap_vol(%M%M1_Volume, PCAP(%M%M1_HB)) );
Table( Ctr, %M%tAN1, cap_vol(%M%M1_Volume, PCAP(%M%M1_HB)) );
Table( Ctr, %M%tCM1, cap_vol(%M%M1_Volume, PCAP(%M%M1_HB)) );
Table( Ctr, %M%tCMH, cap_vol(%M%M1_Volume, PCAP(%M%M1_HB)) );
Table( Ctr, %M%tCMP, cap_vol(%M%M1_Volume, PCAP(%M%M1_HB)) );
Table( Ctr, %M%tCN1, cap_vol(%M%M1_Volume, PCAP(%M%M1_HB)) );
Table( Ctr, %M%tAW1, cap_vol(%M%M1_Volume, PCAP(%M%M1_HB)) );
Table( Ctr, %M%tAWH, cap_vol(%M%M1_Volume, PCAP(%M%M1_HB)) );
Table( Ctr, %M%tAWH2, cap_vol(%M%M1_Volume, PCAP(%M%M1_HB)) );
Table( Ctr, %M%tAN1w, cap_vol(%M%M1_Volume, PCAP(%M%M1_HB)) );
Table( Ctr, %M%tCW1, cap_vol(%M%M1_Volume, PCAP(%M%M1_HB)) );
Table( Ctr, %M%tCWH, cap_vol(%M%M1_Volume, PCAP(%M%M1_HB)) );
Table( Ctr, %M%tCWH2, cap_vol(%M%M1_Volume, PCAP(%M%M1_HB)) );
Table( Ctr, %M%tCN1w, cap_vol(%M%M1_Volume, PCAP(%M%M1_HB)) );
Table( Ctr, %M%tAM2, cap_vol(%M%M2_Volume, PCAP(%M%M2_HB)) );
Table( Ctr, %M%tAN2, cap_vol(%M%M2_Volume, PCAP(%M%M2_HB)) );
Table( Ctr, %M%tCM2, cap_vol(%M%M2_Volume, PCAP(%M%M2_HB)) );
Table( Ctr, %M%tCN2, cap_vol(%M%M2_Volume, PCAP(%M%M2_HB)) );
Table( Ctr, %M%tAW2, cap_vol(%M%M2_Volume, PCAP(%M%M2_HB)) );
Table( Ctr, %M%tAN2w, cap_vol(%M%M2_Volume, PCAP(%M%M2_HB)) );
Table( Ctr, %M%tCW2, cap_vol(%M%M2_Volume, PCAP(%M%M2_HB)) );
Table( Ctr, %M%tCN2w, cap_vol(%M%M2_Volume, PCAP(%M%M2_HB)) );

// every mixer keyed on the match-model context
Table( int, %M%mxAMWw, 4 * %M%MM_Volume );
Table( int, %M%mxAMNw, 4 * %M%MM_Volume );
Table( int, %M%mxAMHw, 4 * %M%MM_Volume );
Table( int, %M%mxCMWw, 4 * %M%MM_Volume );
Table( int, %M%mxCMNw, 4 * %M%MM_Volume );
Table( int, %M%mxCMHw, 4 * %M%MM_Volume );
Table( int, %M%mxCMPw, 4 * %M%MM_Volume );
Table( int, %M%mxAWWw, 4 * %M%MM_Volume );
Table( int, %M%mxAWNw, 4 * %M%MM_Volume );
Table( int, %M%mxAWHw, 4 * %M%MM_Volume );
Table( int, %M%mxAWH2w, 4 * %M%MM_Volume );
Table( int, %M%mxCWWw, 4 * %M%MM_Volume );
Table( int, %M%mxCWNw, 4 * %M%MM_Volume );
Table( int, %M%mxCWHw, 4 * %M%MM_Volume );
Table( int, %M%mxCWH2w, 4 * %M%MM_Volume );

// pair / word match hashes; the *v arrays hold the cached value byte
Table( u32, %M%HA, size_t(1) << PHBITS(%M%HB_A) );
Table( u64, %M%HC, size_t(1) << PHBITS(%M%HB_C) );
Table( u64, %M%HAW, size_t(1) << PHBITS(%M%HB_AW) );
Table( u8, %M%HAWv, size_t(1) << PHBITS(%M%HB_AW) );
Table( u64, %M%HAW2, size_t(1) << PHBITS(%M%HB_AW2) );
Table( u8, %M%HAW2v, size_t(1) << PHBITS(%M%HB_AW2) );
Table( u64, %M%HCW, size_t(1) << PHBITS(%M%HB_CW) );
Table( u8, %M%HCWv, size_t(1) << PHBITS(%M%HB_CW) );
Table( u64, %M%HCW2, size_t(1) << PHBITS(%M%HB_CW2) );
Table( u8, %M%HCW2v, size_t(1) << PHBITS(%M%HB_CW2) );
