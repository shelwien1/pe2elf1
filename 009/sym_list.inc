// sym_list.inc -- the symbol list: the alphabet, its weights, and the search that
// codes one symbol of it.

struct FreqRec {
  union {
    uint16_t w[8];
    struct {
      uint16_t _w0to6[7];
      uint8_t b14;
      uint8_t b15;
    };
  };

  // Seed this record for one live selector pair: an escape weight that halves
  // with the level, a floor at sixty-four times it, and the weight added to
  // both of the pair's slots.  Answers whether the pair is live, which the
  // caller counts.
  //
  // Written out in both `unmodel_plane_slow` and `model_plane`, ten lines
  // apiece.  One read the level from the local it had just stored into `b14`
  // and cast it to `uint8_t`; the other read it back out of `b14`, which is a
  // `uint8_t` member, so the cast and the read-back are the same value.
  bool seed_pair(int32_t lo, int32_t hi, int32_t alphabet_lvl) {
    if( !w[lo]||!w[hi]||b14>alphabet_lvl )
      return false;
    const int16_t weight = (uint8_t)(1<<((5-b14)&31));
    b15 = weight;
    w[6] = weight<<6;
    w[lo] += weight;
    w[hi] += b15;
    resum();
    return true;
  }

  // Which of the five levels `target` falls in, and the cumulative weight to
  // the end of it.  Five slots walked in order.
  //
  // Both decoders wrote this as a five-deep nest of `if`/`else`, twenty-three
  // lines, with the level a literal in each of the six arms -- a linear search
  // with its loop unrolled and its counter written out as constants.
  int32_t find_level(int32_t target, int32_t& cum) const {
    int32_t lvl = 0;
    cum = w[0];
    while( lvl<4&&cum<=target )
      cum += w[++lvl];
    return lvl;
  }

  // Add `b15` to one level and to the total, rescaling the record first if it
  // has outgrown the escape floor in `w[6]` or passed 0x4000.
  //
  // Three sites wrote this out.  Two of the three called `rescale`; the third
  // had it inlined as well, thirty-eight lines, holding the floor, the level
  // and the increment in `nb_sym[0]`, `nb_sym[2]` and `nb_sym[3]` -- three
  // slots of the pixel's *neighbour table* used as scratch.
  //
  // That third copy also wrote `b15` unconditionally where `rescale` writes it
  // only when it is raising it.  The value is the same either way: the branch
  // it sits in has already taken the maximum of the old `b15` and 15.
  void bump(int32_t lvl) {
    const uint16_t floor_ = w[6];
    uint16_t total = w[5];
    uint8_t kick = b15;
    if( total>floor_&&(w[lvl]+kick+8<total||w[5]>0x4000u) )
      total = rescale(floor_, kick);
    w[5] = kick+total;
    w[lvl] += kick;
  }

  // Blend `src` into this record.  The five selector weights this record has
  // are kept, scaled by `src`'s `b15`; `src`'s weights take their place; each
  // then gets 21 parts in `src`'s total added back, rounded up; and the total
  // is re-summed.  `b14` is multiplied by eight on the way through.
  //
  // `code_pixel` and `decode_pixel` each wrote this out, twenty-three lines
  // apiece, holding four of the five scaled weights in `nb_sym[0..3]` -- four
  // slots of the thirty-two-entry *neighbour table*, borrowed as scratch
  // because that is where MSVC had spare stack.  Reading either body, those
  // four lines look like the neighbourhood being written and they are not.
  //
  // The fifth scaled weight was an `int16_t` in both copies where the other
  // four were `int32_t`, and `b15 * w[1]` reaches 16.7 million, so that
  // truncation is real.  It is also invisible: the value is only ever added to
  // and stored into a `uint16_t`, and the two agree modulo 65536.
  void blend_from(const FreqRec& src) {
    const int32_t scale = src.b15;
    int32_t kept[5];
    for( int32_t i = 0; i<5; ++i )
      kept[i] = scale*w[i];
    *this = src;
    b14 *= 8;
    const int32_t total = w[5];
    for( int32_t i = 0; i<5; ++i )
      w[i] = (uint16_t)(kept[i]+(21*w[i]+total-1)/total);
    resum();
  }

  // `w[5]` is the total of the five selector weights.  Three places compute it:
  // this, and the two `init_tables` bodies that seed a record -- and those two
  // spelled the five terms in the order 0, 4, 3, 2, 1.
  uint16_t resum() {
    uint16_t sum = (uint16_t)(w[0]+w[1]+w[2]+w[3]+w[4]);
    w[5] = sum;
    return sum;
  }

  // Halve the five selector weights and re-sum them into `w[5]`, raising the
  // escape floor and the increment if the halving left them too small.
  //
  // The two pixel bodies wrote this out, thirty lines each, with the five
  // halvings interleaved against each other and three of the locals spilled
  // through the frame -- `frame.sym[0..2]` held `w[6]`, the level and `b15`
  // across the middle of it.  Both copies also re-read `w[5]` afterwards to add
  // `b15`, which is why this returns the new sum instead of only storing it.
  uint16_t rescale(uint16_t escape_floor, uint8_t& kick) {
    halve_counts(w, 5);
    const uint16_t sum = resum();
    if( escape_floor<256&&!b14 ) {
      escape_floor = 256;
      w[6] = 256;
    }
    if( sum>escape_floor&&kick<15 ) {
      kick = 15;
      b15 = kick;
    }
    return sum;
  }
};

#pragma pack(push, 1)
struct SymEntry {
  uint16_t sym;
  uint8_t cnt;
  void set(uint32_t s, uint32_t c) {
    sym = (uint16_t)s;
    cnt = (uint8_t)c;
  }
};
#pragma pack(pop)

struct SymList {
  uint32_t n;
  uint32_t live;
  uint32_t tot;
  uint32_t since_rescale;
  uint32_t rescale_at;
  SymEntry* ent;
  bool rescale(int32_t count) {
    int32_t last_cnt;
    uint32_t n_left, running, since, due = rescale_at;
    SymEntry *back;
    if( count<=251&&due>=since_rescale )
      return false;
    n_left = live;
    int32_t bias = due<20*n;
    SymEntry *cur = ent-1;
    SymEntry *prev = cur;
    do {
      prev = cur;
      ++cur;
      int32_t half = (bias+(uint32_t)cur->cnt)>>1;
      cur->cnt = half;
      if( cur!=ent ) {
        SymEntry *up = cur-1;
        int32_t up_cnt = up->cnt;
        if( half>up_cnt ) {
          uint16_t keep = cur->sym;
          cur->set(up->sym, up_cnt);
          if( up!=ent ) {
            do {
              back = up-1;
              int32_t back_cnt = back->cnt;
              if( half<=back_cnt )
                break;
              up->set(back->sym, back_cnt);
              --up;
            } while( back!=ent );
          }
          up->set(keep, half);
        }
      }
      --n_left;
    } while( n_left );
    running = tot;
    if( !cur->cnt ) {
      do {
        ++n_left;
        tot = ++running;
        last_cnt = prev->cnt;
        --prev;
      } while( !last_cnt );
      live -= n_left;
    }
    since = since_rescale;
    tot = halve_up(running);
    since_rescale = halve_up(since);
    return true;
  }

  // Move `p` up past every entry it now outranks, and return the count it
  // settles at -- which is what decides whether the list rescales.  A
  // move-to-front over a list kept in count order.
  //
  // An earlier round found this walk written out twice, here and in
  // `decode_symbol_list`, identical down to the `goto` back into the
  // `p == ent` arm that both used to leave the inner loop; four returns say the
  // same four exits and the label went with them.
  //
  // It was written out a *third* time, in the live branch of `add_weight`, and
  // that spelling hid it well enough to survive that round: it assigns a
  // `count` at four points where this returns, it reuses `p` for the trailing
  // cursor this calls `r`, and its terminal case reads `q->cnt` where this
  // reads `r->cnt` -- the same entry, because the `--q` between them made it
  // so.  Both early exits and both terminal cases were diffed before the two
  // were made one.
  //
  // What genuinely differs between the callers is the increment, 4 against
  // `add_weight`'s argument, and whether the answer is returned or handed to
  // `rescale`.  Both stay with the callers.
  int32_t move_up(SymEntry* p) {
    if( p==ent )
      return p->cnt;
    uint16_t s_a = p->sym;
    uint8_t c_a = p->cnt;
    SymEntry* q = p-1;
    *p = *q;
    q->set(s_a, c_a);
    if( q==ent )
      return q->cnt;
    while( 1 ) {
      int32_t top = q->cnt;
      SymEntry* r = q-1;
      if( top<=r->cnt )
        return top;
      uint16_t s_b = q->sym;
      uint8_t c_b = q->cnt;
      *q = *r;
      r->set(s_b, c_b);
      --q;
      if( r==ent )
        return r->cnt;
    }
  }

  int32_t promote(SymEntry* p) {
    p->cnt += 4;
    since_rescale += 4;
    return move_up(p);
  }

  int32_t code_symbol(int32_t want) {
    int32_t enc_cum, enc_high, enc_tot;
    int32_t c, c2;
    uint32_t i;
    int8_t gen = bmf_cx.exclusion_gen;
    uint32_t left = live;
    SymEntry *p = ent-1;
    int32_t cum = 0;
    while( 1 ) {
      ++p;
      int32_t s = p->sym;
      if( bmf_cx.exclusion_mask[s]!=bmf_cx.exclusion_gen ) {
        c = p->cnt;
        cum += c;
        if( s==want )
          break;
      }
      // The last live symbol, which needs no code of its own: every other one
      // is excluded and the decoder can name it.  A guard with nothing after it
      // in the loop, so it says `continue` and the fourteen lines it held come
      // out a level.
      if( --left )
        continue;
      if( !cum )
        return 0;
      enc_cum = cum;
      enc_tot = tot+cum;
      enc_high = enc_tot;
      do {
        bmf_cx.exclusion_mask[p->sym] = gen;
        --p;
      } while( p>=ent );
      // `result = 0;` and a braced block around the two lines below it, the
      // block being where the decompiler put a `goto`'s landing.  There is one
      // value and it is a literal.
      bmf_cx.rc.encode(enc_cum, enc_high, enc_tot);
      return 0;
    }
    enc_high = cum;
    uint32_t rest = left-1;
    enc_cum = cum-c;
    if( rest ) {
      for( i = 0; i<rest; ++i ) {
        if( bmf_cx.exclusion_mask[p[i+1].sym]==bmf_cx.exclusion_gen )
          c2 = 0;
        else
          c2 = p[i+1].cnt;
        cum += c2;
      }
    }
    enc_tot = tot+cum;
    rescale(promote(p));
    bmf_cx.rc.encode(enc_cum, enc_high, enc_tot);
    return 1;
  }

  void add_weight(int32_t want, uint32_t add) {
    int32_t recycled;
    SymEntry* list = ent;
    uint32_t n_live = live;
    SymEntry *p = list;
    // Find `want` among the live entries.  Running out of them is what "not in
    // the list" means, and an empty list is that same answer reached without a
    // step -- which is why the recovered walk jumped out of itself into the
    // empty-list arm's block rather than falling anywhere.
    uint32_t left = n_live;
    while( left&&p->sym!=want ) {
      ++p;
      --left;
    }
    if( left ) {
      p->cnt += add;
      since_rescale += add;
      // `head` was `ent` read back at three points across a walk that does not
      // move the list.  The walk is `move_up`.
      rescale(move_up(p));
      return;
    }
    // Not in the list, so it joins it -- over the least-counted entry when the
    // list is already `n` long, which is what the entry the walk left behind
    // is.  `full` was assigned the same comparison twice, once before the
    // length test and once inside it, with nothing between that moves either
    // side.
    const bool full = n_live==n;
    if( n_live>=n&&add<=1 )
      return;
    if( full ) {
      live = --n_live;
      recycled = list[n_live].cnt;
    } else {
      recycled = 1;
    }
    list += n_live;
    live = n_live+1;
    tot = recycled+tot+1;
    list->cnt = 2;
    list->sym = want;
    since_rescale += 4;
    if( list!=ent ) {
      uint16_t s3 = list->sym;
      uint8_t c3 = list->cnt;
      *list = list[-1];
      list[-1].set(s3, c3);
    }
  }
  // `storage` is `n_syms` entries the caller owns.  It was a `bmf_new(3*n_syms)`
  // here, and this is the one allocation in the program that ran *per symbol*:
  // `init_symbol_lists` calls this twice for every symbol of the alphabet, so an
  // 8192-symbol image took sixteen thousand of them and `free_sym_lists` gave
  // them back one at a time.
  //
  // The sizes are fixed per caller -- 99 for a selector-1 list, 33 for a
  // selector-0 list -- so the storage is a sub-array at a computed offset rather
  // than an arena with a cursor: `&sel1_ent[s * 99]` needs no bookkeeping and
  // cannot get out of step with what it handed out.
  void init(int32_t n_syms, int32_t dense, SymEntry* storage) {
    uint32_t result;
    const uint32_t n_u = (uint32_t)n_syms;
    n = n_syms;
    ent = storage;
    if( dense ) {
      tot = 0;
      live = n_u;
      since_rescale = 12*n_u;
      rescale_at = 8*n_u;
      for( result = 0; result<live; ++result ) {
        ent[result].sym = result;
        ent[result].cnt = 1;
      }
    } else {
      tot = 2;
      rescale_at = 20*n_u;
      live = 0;
      since_rescale = 18*n_u;
      memset(storage, 0, 3*n_u);
    }
    return;
  }
  // The decode half of `code_symbol`, in `sym_list_decode.inc`.  It was a free
  // function taking the list as its first parameter until `uncopy.py` folded
  // an `owner = syms` alias away and left the parameter reaching nothing but
  // members, which is what `methodise.py` looks for.
  int32_t decode_symbol_list();
};

// Sixteen lists with nothing in them.  The two alphabet walks each open with
// this, and each wrote the loop over eight with the two halves of every pair
// spelled out -- one unrolling, not two different things.  Only `ent` is
// cleared: the rest of each list is written by `init` before anything reads
// it, and these sixteen live on the stack.
static inline void clear_sym_lists(SymList* lists) {
  for( int32_t k = 0; k<16; ++k )
    lists[k].ent = nullptr;
}

struct SymListBlock {
  uint32_t n;
  SymList list[0];
  static constexpr uint32_t bytes(uint32_t n) {
    return __builtin_offsetof(SymListBlock, list)+sizeof(SymList)*n;
  }
};

// One block of `n` empty symbol lists, or nothing if the allocation failed.
//
// Four copies -- two in `expand_alphabet`, two in `model_plane`'s allocation --
// eleven lines apiece, and the two pairs did not look alike: one pair walked
// the lists with a cursor and a countdown, `for( i = list_a; wt; --wt )
// (list_a++)->ent = nullptr;`, leaving the *cursor* in the variable the caller
// then reads, and the other indexed.  Both mean "clear them all", and the
// countdown pair also consumed the count it was given, which is why each of
// those two had to re-read `blk->alphabet` for the next one.
static SymList* new_sym_lists(uint32_t n, void* storage) {
  SymListBlock* block = (SymListBlock*)storage;
  block->n = n;
  for( uint32_t i = 0; i<n; ++i )
    block->list[i].ent = nullptr;
  return block->list;
}

// Nothing to give back, for the reason `free_sym_lists` has none: the entries
// of a `SymList[16]` come from `byte_list_ent` or `gap_list_ent`, both static.
// Kept as a named no-op so that the point at which a set of lists is finished
// with is still written down.
static inline void free_sym_entries(SymList*, uint32_t) {
}

// One 256-symbol list per byte position and carry, four to a byte.  The wide
// alphabet is coded a byte at a time with the carry out of the byte below
// choosing which of that position's four lists codes it, which is what the
// `4*k + carry` index says; both halves of the coder open with this loop.
// `nbytes` is `(depth + 7) >> 3` and the caller's array is `SymList[16]`, so
// `4 * nbytes` is at most 16 and the entries are `16 * 256` of them: 12 KB, a
// static rather than sixteen allocations.  Neither caller is reentrant -- one
// walks an alphabet to decode it and the other to encode it, one plane at a
// time.
static SymEntry byte_list_ent[16*256];

static inline void init_byte_lists(SymList* list, uint32_t nbytes) {
  for( uint32_t k = 0; k<4*nbytes; ++k )
    list[k].init(256, 1, &byte_list_ent[k*256]);
}

// The gap list both alphabet coders build when the alphabet is sparser than the
// depth allows.  Reached only where `depth <= 8`, so `mask` is at most 255 and
// `mask - n_syms + 2` at most 257 -- which is the size below, and the
// `BMF_ASSUME` beside each use is where that reasoning is checked.
static SymEntry gap_list_ent[257];

static inline SymListBlock*sym_list_block(SymList* list) {
  return (SymListBlock*)((uint8_t*)list-__builtin_offsetof(SymListBlock, list));
}

static inline uint32_t sym_list_count(const SymList* list) {
  return sym_list_block((SymList*)list)->n;
}

static inline bool sym_in_top(const SymEntry* ent, int32_t n, int32_t sym) {
  while( n-- )
    if( ent[n].sym==sym )
      return true;
  return false;
}

// The decode half of `SymList::code_symbol`.  It was a file of its own,
// `sym_list_decode.inc`, included nine files further down -- and nothing made it
// one: it reaches `rc.decode` and `rc.get_freq` and otherwise only this class's
// own members, and `rc.inc` comes two includes *before* this file.  The split
// was the size and the frame, and the note below is what happened to both.
//
// It was carrying its own copies
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
  uint32_t gen = (uint8_t)bmf_cx.exclusion_gen;
  int32_t cum = 0;
  // The candidates: every live symbol the encoder has not already offered and
  // had rejected, with their counts summed.
  for( int32_t i = 0; i<(int32_t)live; ++i ) {
    if( (uint8_t)bmf_cx.exclusion_mask[ent[i].sym]==gen ) {
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
  int32_t target = bmf_cx.rc.get_freq(tot_all);
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
        bmf_cx.exclusion_mask[q->sym] = gen_b;
        q = *rd2++;
      } while( q );
      bmf_cx.rc.decode(cum_lo, tot_all, tot_all);
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
  bmf_cx.rc.decode(sym_cum, cum_lo, tot_all);
  return result;
}
