
// C0: color texel model contexts


static inline uint MakeCI1( uint pl, uint pu, uint Wci, uint agq ) { int CI1 = 0;

  CI1 = 0;
  C0_c1l.inc( CI1, pl );
  C0_c1u.inc( CI1, pu );
  C0_c1w.inc( CI1, Wci );
  C0_c1g.inc( CI1, agq );
  return CI1; }
static inline uint MakeCI2( uint prev, uint t ) { int CI2 = 0;

  CI2 = 0;
  C0_c2p.inc( CI2, prev );
  C0_c2t.inc( CI2, t );
  return CI2; }
static inline uint MakeCI3( uint Wci, uint pl ) { int CI3 = 0;

  CI3 = 0;
  C0_c3w.inc( CI3, Wci );
  C0_c3l.inc( CI3, pl );
  return CI3; }
static inline uint MakeCI4( uint Nci, uint pu ) { int CI4 = 0;

  CI4 = 0;
  C0_c4n.inc( CI4, Nci );
  C0_c4u.inc( CI4, pu );
  return CI4; }
static inline uint MakeCI0( uint m3 ) { int CI0 = 0;

  CI0 = 0;
  C0_c0m.inc( CI0, m3 );
  return CI0; }
static inline uint MakeCI5( uint pul, uint pur ) { int CI5 = 0;

  CI5 = 0;
  C0_c5a.inc( CI5, pul );
  C0_c5b.inc( CI5, pur );
  return CI5; }
static inline uint MakeCI6( uint h1c, uint pl, uint h1v ) { int CI6 = 0;

  CI6 = 0;
  C0_c6c.inc( CI6, h1c );
  C0_c6l.inc( CI6, pl );
  C0_c6v.inc( CI6, h1v );
  return CI6; }
static inline uint MakeCI7( uint h2c, uint pl, uint h2v ) { int CI7 = 0;

  CI7 = 0;
  C0_c7c.inc( CI7, h2c );
  C0_c7l.inc( CI7, pl );
  C0_c7v.inc( CI7, h2v );
  return CI7; }
static inline uint MakeCI9( uint rawL, uint rawU ) { int CI9 = 0;

  CI9 = 0;
  C0_c9l.inc( CI9, rawL );
  C0_c9u.inc( CI9, rawU );
  return CI9; }
static inline uint MakeCI8( uint pl, uint pu, uint pul ) { int CI8 = 0;

  CI8 = 0;
  C0_c8l.inc( CI8, pl );
  C0_c8u.inc( CI8, pu );
  C0_c8a.inc( CI8, pul );
  return CI8; }
static inline uint MakeCIb( uint pl, uint pu, uint pul, uint pur ) { int CIb = 0;

  CIb = 0;
  C0_cbl.inc( CIb, pl );
  C0_cbu.inc( CIb, pu );
  C0_cba.inc( CIb, pul );
  C0_cbb.inc( CIb, pur );
  return CIb; }
static inline uint MakeCIc( uint pl, uint pu, uint pul, uint pur, uint Wci ) { int CIc = 0;

  CIc = 0;
  C0_ccl.inc( CIc, pl );
  C0_ccu.inc( CIc, pu );
  C0_cca.inc( CIc, pul );
  C0_ccb.inc( CIc, pur );
  C0_ccw.inc( CIc, Wci );
  return CIc; }
static inline uint MakeCId( uint rawL, uint rawU, uint rawUL ) { int CId = 0;

  CId = 0;
  C0_cdl.inc( CId, rawL );
  C0_cdu.inc( CId, rawU );
  C0_cda.inc( CId, rawUL );
  return CId; }
static inline uint MakeCIe( uint pl, uint pu, uint cfl, uint cfu ) { int CIe = 0;

  CIe = 0;
  C0_cel.inc( CIe, pl );
  C0_ceu.inc( CIe, pu );
  C0_cec.inc( CIe, cfl );
  C0_ced.inc( CIe, cfu );
  return CIe; }
static inline uint MakeCIf( uint gl, uint sp, uint prev ) { int CIf = 0;

  CIf = 0;
  C0_cfg.inc( CIf, gl );
  C0_cfs.inc( CIf, sp );
  C0_cfp.inc( CIf, prev );
  return CIf; }
static inline uint MakeCIp( uint ppc, uint pl, uint pu ) { int CIp = 0;

  CIp = 0;
  C0_cpp.inc( CIp, ppc );
  C0_cpl.inc( CIp, pl );
  C0_cpu.inc( CIp, pu );
  return CIp; }
static inline uint MakeCIq( uint ppc, uint pcf, uint prev ) { int CIq = 0;

  CIq = 0;
  C0_cqp.inc( CIq, ppc );
  C0_cqc.inc( CIq, pcf );
  C0_cqv.inc( CIq, prev );
  return CIq; }
static inline uint MakeCIr( uint pci, uint pcs, uint ppc ) { int CIr = 0;

  CIr = 0;
  C0_crp.inc( CIr, pci );
  C0_crs.inc( CIr, pcs );
  C0_crc.inc( CIr, ppc );
  return CIr; }
static inline uint MakeCIg( uint pl, uint pu, uint avq ) { int CIg = 0;

  CIg = 0;
  C0_cgl.inc( CIg, pl );
  C0_cgu.inc( CIg, pu );
  C0_cga.inc( CIg, avq );
  return CIg; }
static inline uint MakeCIM( uint plu, uint d13, uint nd4m, uint spq, uint t, uint stq, uint mdq, uint avq ) { int CIM = 0;

  CIM = 0;
  C0_cmp.inc( CIM, plu );
  C0_cmd.inc( CIM, d13 );
  C0_cmn.inc( CIM, nd4m );
  C0_cms.inc( CIM, spq );
  C0_cmt.inc( CIM, t );
  C0_cmw.inc( CIM, stq );
  C0_cmq.inc( CIM, mdq );
  C0_cma.inc( CIM, avq );
  return CIM; }
static inline uint MakeAPC( uint pl, uint pu, uint pul, uint stq, uint avq ) { int APC = 0;

  APC = 0;
  C0_apl.inc( APC, pl );
  C0_apu.inc( APC, pu );
  C0_apa.inc( APC, pul );
  C0_aps.inc( APC, stq );
  C0_apq.inc( APC, avq );
  return APC; }
static inline uint MakeAPC2( uint t, uint pl, uint mdq, uint agq ) { int APC2 = 0;

  APC2 = 0;
  C0_aqt.inc( APC2, t );
  C0_aql.inc( APC2, pl );
  C0_aqm.inc( APC2, mdq );
  C0_aqg.inc( APC2, agq );
  return APC2; }
static inline uint MakeSLC( uint pl, uint pu, uint pul, uint pur ) { int SLC = 0;

  SLC = 0;
  C0_scl.inc( SLC, pl );
  C0_scu.inc( SLC, pu );
  C0_sca.inc( SLC, pul );
  C0_scb.inc( SLC, pur );
  return SLC; }


// ---- storage ---------------------------------------------------------
// Declared with the IDX Table() directive, so the same source produces
// pointers allocated in C0_Init() (Debug build, sizes are runtime values)
// or fixed-size arrays (Const build).  Codec inherits C0_T and binds its
// CtrTab / Mixer / APM views to these.
