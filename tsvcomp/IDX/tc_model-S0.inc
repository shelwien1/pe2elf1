// Generated into MOD/tc_model-S0_p.inc: the S0 knobs and context masks
// (MakeTables), the SSE row-context builder (MakeIndex).  One row of NB
// cells per context, starting at cell S0_MakeCx(..) * CP_S0::NB.
MakeTables

static uint S0_MakeCx( uint k, uint st, uint zrun ) {
  int Cx;
  MakeIndex Cx
  return uint(Cx);
}

Table( SSE_Cell, %M%tbl, %M%Cx_Volume * CP_S0::NB );
