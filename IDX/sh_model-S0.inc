
// Generated into MOD/sh_model-S0_p.inc: the S0 knobs and context masks
// (MakeTables), the row-context builders (MakeIndex).  The rows are declared
// below through def_Data (SSE-MIX2-REDESIGN.md sec.2.4): a fixed array of
// SSE_Row<NB> in the shipping build, NB folded; in the tuning build NB is a
// load-time value, so the table is an SSE_Tbl<CP_S0> that instantiates the
// row for every NB and dispatches on the one in force -- allocated in
// S0_Init() and freed in S0_Quit() like a Table() member.
MakeTables

// Row context of the SSE stage: c3/c2/c1 = the three last bytes, cxt = the
// bit-tree node (1..255).  The coder combines the two results in 64 bits:
// row = Cx * S0_Cx3_Volume + Cx3.
static uint S0_MakeCx( uint c2, uint c1, uint cxt ) {
  int Cx;
  MakeIndex Cx
  return uint(Cx);
}
static uint S0_MakeCx3( uint c3 ) {
  int Cx3;
  MakeIndex Cx3
  return uint(Cx3);
}

// Phase 1: today's row count, min(volume, 2^HBITS, 2^SSE_MAXCELLS_LOG/NB),
// with the coder hashing the context onto it (sse_row_count, coder0.cpp);
// phase 2 sizes the table by Cx_Volume and indexes it directly.
def_Data
#if USE_NEW
  SSE_Tbl<CP_S0> %M%tbl;
#else
  SSE_Row<sse_nb_clamp(CP_S0::NB)> %M%tbl[ sse_row_count( qword(%M%Cx_Volume)*%M%Cx3_Volume ) ];
#endif
end_Data
def_Init
#if USE_NEW
  %M%tbl.Init( tbl_n(sse_row_count( qword(%M%Cx_Volume)*%M%Cx3_Volume )) );
  %M%Size += %M%tbl.Bytes();
#endif
end_Init
def_Quit
#if USE_NEW
  %M%tbl.Quit();
#endif
end_Quit
