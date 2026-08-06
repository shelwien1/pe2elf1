
// C0: color texel model contexts


static inline uint MakeCI1( uint pl, uint pu, uint Wci, uint agq ) { int CI1 = 0;
  MakeIndex CI1
  return CI1; }
static inline uint MakeCI2( uint prev, uint t ) { int CI2 = 0;
  MakeIndex CI2
  return CI2; }
static inline uint MakeCI3( uint Wci, uint pl ) { int CI3 = 0;
  MakeIndex CI3
  return CI3; }
static inline uint MakeCI4( uint Nci, uint pu ) { int CI4 = 0;
  MakeIndex CI4
  return CI4; }
static inline uint MakeCI0( uint m3 ) { int CI0 = 0;
  MakeIndex CI0
  return CI0; }
static inline uint MakeCI5( uint pul, uint pur ) { int CI5 = 0;
  MakeIndex CI5
  return CI5; }
static inline uint MakeCI6( uint h1c, uint pl, uint h1v ) { int CI6 = 0;
  MakeIndex CI6
  return CI6; }
static inline uint MakeCI7( uint h2c, uint pl, uint h2v ) { int CI7 = 0;
  MakeIndex CI7
  return CI7; }
static inline uint MakeCI9( uint rawL, uint rawU ) { int CI9 = 0;
  MakeIndex CI9
  return CI9; }
static inline uint MakeCI8( uint pl, uint pu, uint pul ) { int CI8 = 0;
  MakeIndex CI8
  return CI8; }
static inline uint MakeCIb( uint pl, uint pu, uint pul, uint pur ) { int CIb = 0;
  MakeIndex CIb
  return CIb; }
static inline uint MakeCIc( uint pl, uint pu, uint pul, uint pur, uint Wci ) { int CIc = 0;
  MakeIndex CIc
  return CIc; }
static inline uint MakeCId( uint rawL, uint rawU, uint rawUL ) { int CId = 0;
  MakeIndex CId
  return CId; }
static inline uint MakeCIe( uint pl, uint pu, uint cfl, uint cfu ) { int CIe = 0;
  MakeIndex CIe
  return CIe; }
static inline uint MakeCIf( uint gl, uint sp, uint prev ) { int CIf = 0;
  MakeIndex CIf
  return CIf; }
static inline uint MakeCIp( uint ppc, uint pl, uint pu ) { int CIp = 0;
  MakeIndex CIp
  return CIp; }
static inline uint MakeCIq( uint ppc, uint pcf, uint prev ) { int CIq = 0;
  MakeIndex CIq
  return CIq; }
static inline uint MakeCIr( uint pci, uint pcs, uint ppc ) { int CIr = 0;
  MakeIndex CIr
  return CIr; }
static inline uint MakeCIg( uint pl, uint pu, uint avq ) { int CIg = 0;
  MakeIndex CIg
  return CIg; }
static inline uint MakeCIM( uint plu, uint d13, uint nd4m, uint spq, uint t, uint stq, uint mdq, uint avq ) { int CIM = 0;
  MakeIndex CIM
  return CIM; }
static inline uint MakeAPC( uint pl, uint pu, uint pul, uint stq, uint avq ) { int APC = 0;
  MakeIndex APC
  return APC; }
static inline uint MakeAPC2( uint t, uint pl, uint mdq, uint agq ) { int APC2 = 0;
  MakeIndex APC2
  return APC2; }
static inline uint MakeSLC( uint pl, uint pu, uint pul, uint pur ) { int SLC = 0;
  MakeIndex SLC
  return SLC; }


// ---- storage ---------------------------------------------------------
// Declared with the IDX Table() directive, so the same source produces
// pointers allocated in %M%Init() (Debug build, sizes are runtime values)
// or fixed-size arrays (Const build).  Codec inherits %M%T and binds its
// CtrTab / Mixer / APM views to these.
Table( Ctr, %M%tCI1, %M%CI1_Volume * 4 );
Table( Ctr, %M%tCI2, %M%CI2_Volume * 4 );
Table( Ctr, %M%tCI5, %M%CI5_Volume * 4 );
Table( Ctr, %M%tCI6, %M%CI6_Volume * 4 );
Table( Ctr, %M%tCI7, %M%CI7_Volume * 4 );
Table( Ctr, %M%tCI9, %M%CI9_Volume * 4 );
Table( Ctr, %M%tCI8, %M%CI8_Volume * 4 );
Table( Ctr, %M%tCIb, %M%CIb_Volume * 4 );
Table( Ctr, %M%tCIc, %M%CIc_Volume * 4 );
Table( Ctr, %M%tCId, %M%CId_Volume * 4 );
Table( Ctr, %M%tCIe, %M%CIe_Volume * 4 );
Table( Ctr, %M%tCIf, %M%CIf_Volume * 4 );
Table( Ctr, %M%tCIg, %M%CIg_Volume * 4 );
Table( Ctr, %M%tCIp, %M%CIp_Volume * 4 );
Table( Ctr, %M%tCIq, %M%CIq_Volume * 4 );
Table( Ctr, %M%tCIr, %M%CIr_Volume * 4 );
Table( Ctr, %M%tCI3, %M%CI3_Volume * 4 );
Table( Ctr, %M%tCI4, %M%CI4_Volume * 4 );
Table( Ctr, %M%tCI0, %M%CI0_Volume * 4 );
Table( CtrS, %M%sCI1, %M%SLC_Volume * 4 );
Table( int, %M%mxCIw, 22 * %M%CIM_Volume * 4 );
Table( u16, %M%apCIt, %M%APC_Volume * 4 * (PAPN(G0_AP_N_CI) + 1) );
Table( u16, %M%apCI2t, %M%APC2_Volume * 4 * (PAPN(G0_AP_N_CI2) + 1) );
