
// Generated into MOD/sh_model-S0_p.inc: the S0 knobs and context masks
// (MakeTables), the row-context builder (MakeIndex).  The SSE cells are a
// Table(), NB per row context: a fixed array member of S0_T in the shipping
// build (NB folded), a pointer allocated in S0_Init() in the tuning build.
MakeTables

// Row context of the SSE stage: c1 = the last byte, cxt = the bit-tree node
// (1..255).  One row of NB cells per context, starting at cell
// S0_MakeCx(c1, cxt) * CP_S0::NB.
static uint S0_MakeCx( uint c1, uint cxt ) {
  int Cx;
  MakeIndex Cx
  return uint(Cx);
}

Table( SSE_Cell, %M%tbl, %M%Cx_Volume * CP_S0::NB );
