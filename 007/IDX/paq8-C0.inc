// paq8-C0.inc -- template for the generated MOD/paq8-C0_p.inc.
//
// C0 declares no Index and no storage: the order hashes are built by a loop in
// ContextModel::p over the primes[] table, not by a packing, so there is
// nothing for MakeIndex to emit.  The file exists because idx2inc.pl takes a
// template per module; the constants themselves land in paq8-C0_h.inc.
