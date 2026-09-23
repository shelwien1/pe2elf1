
// Generated into MOD/sh_model-C1_p.inc: the C1 knobs and context masks
// (MakeTables), the cell-context builder (MakeIndex).  The C1 cells are a
// Table(), one per context: a fixed array member of C1_T in the shipping
// build, a pointer allocated in C1_Init() in the tuning build.  Same cell
// type and bundle layout as C0 (config.hpp), its own knobs: the second
// counter table the mixer blends with C0.
MakeTables

static uint C1_MakeCx( uint c2, uint c1, uint cxt ) {
  int Cx;
  MakeIndex Cx
  return uint(Cx);
}

Table( C1_Cell, %M%tbl, %M%Cx_Volume );
