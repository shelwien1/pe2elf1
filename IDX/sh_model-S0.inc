
// Generated into MOD/sh_model-S0_p.inc: the S0 knobs and context masks
// (MakeTables), the row-context builders (MakeIndex).  The rows are declared
// below through def_Data (SSE-MIX2-REDESIGN.md sec.2.4): a fixed array of
// SSE_Row<NB> in the shipping build, NB folded; in the tuning build NB is a
// load-time value, so the table is an SSE_Tbl<CP_S0> that instantiates the
// row for every NB and dispatches on the one in force -- allocated in
// S0_Init() and freed in S0_Quit() like a Table() member.
MakeTables

// Row context of the SSE stage: c3/c2/c1 = the three last bytes, cxt = the
// bit-tree node (1..255); one row per context, S0_tbl[S0_MakeCx(..)].
static uint S0_MakeCx( uint c3, uint c2, uint c1, uint cxt ) {
  int Cx;
  MakeIndex Cx
  return uint(Cx);
}

// The shipping table is allocated too, not a fixed array as a Table()'s
// would be: at 133.7M rows it is 51 GB, and a static object that size
// links on neither side (2 GB: the x86-64 small code model's relocations,
// and the PE image on Windows).  Same place as a Table() member's
// allocation -- S0_Init() / S0_Quit() -- and no () on the new[] for the
// same reason (IDX-FORMAT.md sec.9): Init() fills every row before it is
// read.
def_Data
#if USE_NEW
  SSE_Tbl<CP_S0> %M%tbl;
#else
  SSE_Row<sse_nb_clamp(CP_S0::NB)>* %M%tbl;
#endif
end_Data
def_Init
#if USE_NEW
  %M%tbl.Init( tbl_n(%M%Cx_Volume) );
  %M%Size += %M%tbl.Bytes();
#else
  %M%tbl = new SSE_Row<sse_nb_clamp(CP_S0::NB)>[ tbl_n(%M%Cx_Volume) ];
#endif
end_Init
def_Quit
#if USE_NEW
  %M%tbl.Quit();
#else
  delete[] %M%tbl;
#endif
end_Quit
