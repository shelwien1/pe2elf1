// Placement file for sh_model-B0.idx, kept as the answer to "why does C0
// have one and B0 not".  B0 declares parameters only -- no Index, no Table --
// so there is nothing to place: with no MakeTables marker in the .idx,
// idx2inc.pl routes every declaration to sh_model-B0_h.inc, which is what
// bmpc.cpp includes, and emits no _p.inc of its own.  Nothing reads this
// file; `GCIDX=1 ./gc.sh` leaves it alone.
