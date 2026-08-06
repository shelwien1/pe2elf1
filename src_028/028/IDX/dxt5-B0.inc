
// B0: alpha texel model contexts


static inline uint MakeAI1( uint pl, uint pu, uint mode, uint d ) { int AI1 = 0;
  MakeIndex AI1
  return AI1; }
static inline uint MakeAI2( uint prev, uint t ) { int AI2 = 0;
  MakeIndex AI2
  return AI2; }
static inline uint MakeAI3( uint Wai, uint pl ) { int AI3 = 0;
  MakeIndex AI3
  return AI3; }
static inline uint MakeAI4( uint Nai, uint pu ) { int AI4 = 0;
  MakeIndex AI4
  return AI4; }
static inline uint MakeAI0( uint dz, uint mode, uint d ) { int AI0 = 0;
  MakeIndex AI0
  return AI0; }
static inline uint MakeAI5( uint h1c, uint pl, uint h1v ) { int AI5 = 0;
  MakeIndex AI5
  return AI5; }
static inline uint MakeAI6( uint h2c, uint pl, uint h2v ) { int AI6 = 0;
  MakeIndex AI6
  return AI6; }
static inline uint MakeAI9( uint rawL, uint rawU ) { int AI9 = 0;
  MakeIndex AI9
  return AI9; }
static inline uint MakeAI8( uint pl, uint pu, uint qeq ) { int AI8 = 0;
  MakeIndex AI8
  return AI8; }
static inline uint MakeAIb( uint pl, uint pu, uint pql, uint pqr ) { int AIb = 0;
  MakeIndex AIb
  return AIb; }
static inline uint MakeAIe( uint pl, uint pu, uint cfl, uint cfu ) { int AIe = 0;
  MakeIndex AIe
  return AIe; }
static inline uint MakeAIf( uint gl, uint sp, uint prev ) { int AIf = 0;
  MakeIndex AIf
  return AIf; }
static inline uint MakeAIp( uint ppa, uint pl, uint pu ) { int AIp = 0;
  MakeIndex AIp
  return AIp; }
static inline uint MakeAIq( uint ppa, uint paf, uint pai ) { int AIq = 0;
  MakeIndex AIq
  return AIq; }
static inline uint MakeAIM( uint plu, uint mode, uint d, uint sq, uint t, uint cfl ) { int AIM = 0;
  MakeIndex AIM
  return AIM; }
static inline uint MakeAPA( uint plu, uint pl, uint mode, uint sq ) { int APA = 0;
  MakeIndex APA
  return APA; }
static inline uint MakeAPA2( uint pl, uint pu, uint Wai ) { int APA2 = 0;
  MakeIndex APA2
  return APA2; }
static inline uint MakeSLA( uint pl, uint pu, uint mode, uint d ) { int SLA = 0;
  MakeIndex SLA
  return SLA; }


// ---- storage ---------------------------------------------------------
// Declared with the IDX Table() directive, so the same source produces
// pointers allocated in %M%Init() (Debug build, sizes are runtime values)
// or fixed-size arrays (Const build).  Codec inherits %M%T and binds its
// CtrTab / Mixer / APM views to these.
Table( Ctr, %M%tAI1, %M%AI1_Volume * 8 );
Table( Ctr, %M%tAI2, %M%AI2_Volume * 8 );
Table( Ctr, %M%tAI3, %M%AI3_Volume * 8 );
Table( Ctr, %M%tAI4, %M%AI4_Volume * 8 );
Table( Ctr, %M%tAI0, %M%AI0_Volume * 8 );
Table( Ctr, %M%tAI5, %M%AI5_Volume * 8 );
Table( Ctr, %M%tAI6, %M%AI6_Volume * 8 );
Table( Ctr, %M%tAI9, %M%AI9_Volume * 8 );
Table( Ctr, %M%tAI8, %M%AI8_Volume * 8 );
Table( Ctr, %M%tAIb, %M%AIb_Volume * 8 );
Table( Ctr, %M%tAIe, %M%AIe_Volume * 8 );
Table( Ctr, %M%tAIf, %M%AIf_Volume * 8 );
Table( Ctr, %M%tAIp, %M%AIp_Volume * 8 );
Table( Ctr, %M%tAIq, %M%AIq_Volume * 8 );
Table( CtrS, %M%sAI1, %M%SLA_Volume * 8 );
Table( int, %M%mxAIw, 17 * %M%AIM_Volume * 8 );
Table( u16, %M%apAIt, %M%APA_Volume * 8 * (PAPN(G0_AP_N_AI) + 1) );
Table( u16, %M%apAI2t, %M%APA2_Volume * 8 * (PAPN(G0_AP_N_AI2) + 1) );
