// sym_list_decode.inc -- read back the alphabet an encoder wrote.

// The decode half of `SymList::code_symbol`, and it was carrying its own copies
// of two things that half already had as methods: the sort-up after a hit
// (`promote`, 25 lines) and the halving pass (`rescale`, 55).  Both are called
// here now, which is also what emptied the frame this used to run on.
//
// That frame was a `union` of the candidate array with a struct of eight spill
// slots, each 4-byte member padded out to a pointer so it sat over one entry
// of the array.  The two arms are a sequence rather than an overlap: the last
// read of the array is the exclusion walk in the escape branch, and the first
// spill write is the line after it.  Of the eight, one was the return value,
// five were the two shared methods' locals, one was a save/restore pair of a
// variable nothing else reads -- a register spill the decompiler kept with
// nothing behind it -- and one was never used at all.
inline int32_t SymList::decode_symbol_list() {
  // One pointer per symbol still in play, and a null terminator.
  //
  // `no_symbol+1` is the alphabet ceiling this program enforces, so this
  // cannot be overrun by any `SymList` it builds -- which the old `[8192]`
  // was one short of, and got away with because the eight bytes after it were
  // the frame's trailing pad.
  //
  // What reaches here is far narrower.  The three call sites pass lists built
  // by `init(256, 1)`, by `init(mask-n_syms+2, 1)` on a path where `mask` is
  // at most 255, and by `init(99, 0)` and `init(33, 0)`; so 257 is the bound,
  // and 257 is what an instrumented build measures as the high-water mark over
  // the corpus -- `live = 256` on a 256-symbol list, plus the terminator.  The
  // array is a plain local now, which is what would let ASan say so if that
  // stopped being true.
  //
  // It is still **64 KiB of stack per call** -- 8193 pointers -- against 257
  // that are ever written.  That is a deliberate keep, not an oversight: the
  // bound the array is declared with is the one the format enforces, while 257
  // is a fact about the three call sites, and sizing a buffer from the narrower
  // of the two is how the `[8192]` this replaced came to be one short.  Worth
  // stating because it is a per-call stack cost nothing else in the program
  // comes close to, and because the recursion depth here is one:
  // `decode_symbol_list` calls nothing that calls it back.
  SymEntry* list[no_symbol+1];
  SymEntry *q;
  SymEntry** w, ** rd, ** rd2;
  int32_t c;
  // `live`, `ent` and `tot` were copied into locals of the same name, which is
  // what the parameter form left behind and what made this a method's three
  // shadowed members.  The two counts are `uint32_t` here and were `int32_t`
  // there; the alphabet ceiling this file argues for above is 257, so the two
  // readings of every comparison below agree, and the casts say which one the
  // binary made.
  w = list;
  uint32_t gen = (uint8_t)exclusion_gen;
  int32_t cum = 0;
  // The candidates: every live symbol the encoder has not already offered and
  // had rejected, with their counts summed.
  for( int32_t i = 0; i<(int32_t)live; ++i ) {
    if( (uint8_t)exclusion_mask[ent[i].sym]==gen ) {
      c = 0;
    } else {
      SymEntry *e = &ent[i];
      c = e->cnt;
      *w++ = e;
    }
    cum += c;
  }
  if( !cum )
    return -1;
  *w = nullptr;
  // One total, which the frame spelled twice: `cum + tot` went into two slots
  // and the escape path read one while the hit path read the other.
  int32_t tot_all = cum+(int32_t)tot;
  int32_t target = rc.get_freq(tot_all);
  SymEntry *p = list[0];
  rd = &list[1];
  SymEntry *first = list[0];
  uint32_t cum_lo = 0;
  while( 1 ) {
    cum_lo += p->cnt;
    if( cum_lo>(uint32_t)target )
      break;
    p = *rd++;
    if( !p ) {
      // Past the last candidate: the escape.  Stamp every one of them with
      // this generation so the next list does not offer them again.
      int8_t gen_b = (int8_t)gen;
      q = first;
      rd2 = &list[1];
      do {
        exclusion_mask[q->sym] = gen_b;
        q = *rd2++;
      } while( q );
      rc.decode(cum_lo, tot_all, tot_all);
      return -1;
    }
  }
  // The interval, taken before `promote` moves the entry and changes its
  // count.  This order is the original's: the list is updated and only then is
  // the decode committed, which is why the two bounds and the total were three
  // of the frame's spill slots.
  int32_t result = p->sym;
  uint32_t sym_cum = cum_lo-p->cnt;
  this->rescale(this->promote(p));
  rc.decode(sym_cum, cum_lo, tot_all);
  return result;
}
