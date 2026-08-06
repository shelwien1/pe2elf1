
// B0: alpha texel model contexts


static inline uint MakeAI1( uint pl, uint pu, uint mode, uint d ) { int AI1 = 0;

  AI1 = 0;
  B0_i1l.inc( AI1, pl );
  B0_i1u.inc( AI1, pu );
  B0_i1m.inc( AI1, mode );
  B0_i1d.inc( AI1, d );
  return AI1; }
static inline uint MakeAI2( uint prev, uint t ) { int AI2 = 0;

  AI2 = 0;
  B0_i2p.inc( AI2, prev );
  B0_i2t.inc( AI2, t );
  return AI2; }
static inline uint MakeAI3( uint Wai, uint pl ) { int AI3 = 0;

  AI3 = 0;
  B0_i3w.inc( AI3, Wai );
  B0_i3l.inc( AI3, pl );
  return AI3; }
static inline uint MakeAI4( uint Nai, uint pu ) { int AI4 = 0;

  AI4 = 0;
  B0_i4n.inc( AI4, Nai );
  B0_i4u.inc( AI4, pu );
  return AI4; }
static inline uint MakeAI0( uint dz, uint mode, uint d ) { int AI0 = 0;

  AI0 = 0;
  B0_i0z.inc( AI0, dz );
  B0_i0m.inc( AI0, mode );
  B0_i0d.inc( AI0, d );
  return AI0; }
static inline uint MakeAI5( uint h1c, uint pl, uint h1v ) { int AI5 = 0;

  AI5 = 0;
  B0_i5c.inc( AI5, h1c );
  B0_i5l.inc( AI5, pl );
  B0_i5v.inc( AI5, h1v );
  return AI5; }
static inline uint MakeAI6( uint h2c, uint pl, uint h2v ) { int AI6 = 0;

  AI6 = 0;
  B0_i6c.inc( AI6, h2c );
  B0_i6l.inc( AI6, pl );
  B0_i6v.inc( AI6, h2v );
  return AI6; }
static inline uint MakeAI9( uint rawL, uint rawU ) { int AI9 = 0;

  AI9 = 0;
  B0_i9l.inc( AI9, rawL );
  B0_i9u.inc( AI9, rawU );
  return AI9; }
static inline uint MakeAI8( uint pl, uint pu, uint qeq ) { int AI8 = 0;

  AI8 = 0;
  B0_i8l.inc( AI8, pl );
  B0_i8u.inc( AI8, pu );
  B0_i8q.inc( AI8, qeq );
  return AI8; }
static inline uint MakeAIb( uint pl, uint pu, uint pql, uint pqr ) { int AIb = 0;

  AIb = 0;
  B0_ibl.inc( AIb, pl );
  B0_ibu.inc( AIb, pu );
  B0_ibq.inc( AIb, pql );
  B0_ibr.inc( AIb, pqr );
  return AIb; }
static inline uint MakeAIe( uint pl, uint pu, uint cfl, uint cfu ) { int AIe = 0;

  AIe = 0;
  B0_iel.inc( AIe, pl );
  B0_ieu.inc( AIe, pu );
  B0_iec.inc( AIe, cfl );
  B0_ied.inc( AIe, cfu );
  return AIe; }
static inline uint MakeAIf( uint gl, uint sp, uint prev ) { int AIf = 0;

  AIf = 0;
  B0_ifg.inc( AIf, gl );
  B0_ifs.inc( AIf, sp );
  B0_ifp.inc( AIf, prev );
  return AIf; }
static inline uint MakeAIp( uint ppa, uint pl, uint pu ) { int AIp = 0;

  AIp = 0;
  B0_ipp.inc( AIp, ppa );
  B0_ipl.inc( AIp, pl );
  B0_ipu.inc( AIp, pu );
  return AIp; }
static inline uint MakeAIq( uint ppa, uint paf, uint pai ) { int AIq = 0;

  AIq = 0;
  B0_iqp.inc( AIq, ppa );
  B0_iqc.inc( AIq, paf );
  B0_iqi.inc( AIq, pai );
  return AIq; }
static inline uint MakeAIM( uint plu, uint mode, uint d, uint sq, uint t, uint cfl ) { int AIM = 0;

  AIM = 0;
  B0_imp.inc( AIM, plu );
  B0_imm.inc( AIM, mode );
  B0_imd.inc( AIM, d );
  B0_ims.inc( AIM, sq );
  B0_imt.inc( AIM, t );
  B0_imc.inc( AIM, cfl );
  return AIM; }
static inline uint MakeAPA( uint plu, uint pl, uint mode, uint sq ) { int APA = 0;

  APA = 0;
  B0_aap.inc( APA, plu );
  B0_aal.inc( APA, pl );
  B0_aam.inc( APA, mode );
  B0_aas.inc( APA, sq );
  return APA; }
static inline uint MakeAPA2( uint pl, uint pu, uint Wai ) { int APA2 = 0;

  APA2 = 0;
  B0_a2l.inc( APA2, pl );
  B0_a2u.inc( APA2, pu );
  B0_a2w.inc( APA2, Wai );
  return APA2; }
static inline uint MakeSLA( uint pl, uint pu, uint mode, uint d ) { int SLA = 0;

  SLA = 0;
  B0_sll.inc( SLA, pl );
  B0_slu.inc( SLA, pu );
  B0_slm.inc( SLA, mode );
  B0_sld.inc( SLA, d );
  return SLA; }


// ---- storage ---------------------------------------------------------
// Declared with the IDX Table() directive, so the same source produces
// pointers allocated in B0_Init() (Debug build, sizes are runtime values)
// or fixed-size arrays (Const build).  Codec inherits B0_T and binds its
// CtrTab / Mixer / APM views to these.
