
// Generated into MOD/sh_model-S0_p.inc: the S0 knobs and context masks
// (MakeTables), the row-context builders (MakeIndex).  The cell storage is
// a Table(): a fixed array member of S0_T in the shipping build, a pointer
// allocated in S0_Init() in the tuning build (IDX-FORMAT.md sec.9); the
// size helper is defined by the coder before MOD/sh_model-S0_h.inc.
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

Table( SSE_Cell, %M%tbl, sse_table_cells( qword(%M%Cx_Volume)*%M%Cx3_Volume ) );
