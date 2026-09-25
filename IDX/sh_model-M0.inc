
// Generated into MOD/sh_model-M0_p.inc: the M0 knobs and context masks
// (MakeTables), the mixer-context builder (MakeIndex).  The mixer cells are
// a Table(), one per context: a fixed array member of M0_T in the shipping
// build, a pointer allocated in M0_Init() in the tuning build.
MakeTables

static uint M0_MakeCx( uint c2, uint c1, uint cxt ) {
  int Cx;
  MakeIndex Cx
  return uint(Cx);
}

Table( Mix2_Cell, %M%tbl, %M%Cx_Volume );
