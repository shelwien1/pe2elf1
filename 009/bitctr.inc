// bitctr.inc -- the binary counters -- one context bit, coded and decoded, and the
// pair rescale behind them.

struct CounterNode {
  uint16_t total;
  uint16_t c[7];

  // Code one symbol against this node.  Defined out of line in sym_code.inc,
  // beside the tree coder it calls; `alt_p1_encode_symbol` and
  // `alt_p1_decode_symbol` are the two instantiations under their callers'
  // names.
  template<int32_t f_DEC> int32_t code_symbol(int32_t ctx, int32_t sym);

  // One counter bumped, with the node's total kept in step.  119 sites in
  // `alt_p1_block`'s `update_model` wrote both halves out, and the increment --
  // one of nine constants, 2 to 17 -- is what that function is *about*: which
  // counters a coded symbol moves, and by how much.
  //
  // The two halves have to move together, because the coder divides by the
  // total; a site that bumped one and not the other would be a defect no type
  // could catch.  As a method there is one place that can be wrong.
  void bump(int32_t slot, uint32_t n) {
    c[slot] += n;
    total += n;
  }

  // One bit of the escape's strip index: 0 when this slot holds at most half
  // the node's mass, **+64** when it holds more.
  //
  // The comment here read "-64 when it does not", and both halves of that were
  // wrong.  The arithmetic is unsigned -- `(uint32_t)c[slot]` makes it so --
  // so `2*c[slot] > (total&0x7FFF) + c[0]` *wraps* rather than going negative,
  // `>>25` is a logical shift giving 127 rather than a sign test giving -1,
  // and `&0xFFFFFFC0` clears the low six bits of 127 to leave 64.  The
  // condition is inverted as well: the wrap is the case where the slot is
  // *over* half, which the old sentence gave to the 0.
  //
  // The call sites settle it without needing the shift read at all.  Every one
  // of the four adds this to `ctx + 128*(slot&1)` and hands the sum to
  // `model_strip`, which indexes 1024 strips; -64 as a `uint32_t` is
  // 0xFFFFFFC0 and the sum would leave the table on the first escape whose
  // slot was over half.  One of them then tests `key & 0x38`, which reads bits
  // 3..5 of a small non-negative number.  It is a context bit, and it is set
  // high.
  //
  // So it is written as the comparison it is.  Every caller adds the 0-or-64
  // into a strip index where 64 is bit 6, so a `bool` under `bit<6>` is what
  // they want and the two spellings meet there.  `alt_p1_block` spelled the
  // shift out four times, twice reading `total` from a local holding the value
  // just stored into it.
  bool escape_high(int32_t slot) const {
    return 2u*c[slot]>(total&0x7FFFu)+c[0];
  }
};

// The tree slot a folded code takes: itself below 5, then slot 5 for odd codes
// and 6 for even ones.  Two independent spellings of one mapping --
// `CounterNode::code_symbol` writes it this way round and
// `AltP1Block::update_model` computes it twice, once per fold direction.
//
// `update_model` also re-read `fold[(uint8_t)(sym - pred)]` to get the low case
// rather than reusing the `code_f` it had just computed from the same
// expression.  Passing the code in removes that second table read.
static inline int32_t sym_slot(int32_t code) {
  return code<5 ? code : 6-(code&1);
}

struct SymPair {
  uint16_t last;
  uint16_t prev;
};

struct FreqPair {
  uint16_t f[2];
};

static inline FreqPair*bit_tree(uint16_t* freq, int32_t lvl) {
  return (FreqPair*)&freq[2*level_geom[lvl].tbl_base+8];
}

// One binary counter with a parent to fall back on, and a three-state life:
// unseen, seeded, live.  Read against `bcdr.cpp`'s `BIN_CODER`, which is the
// same counter with the same states written by hand -- the two agree line for
// line and only the constants differ, so the names below are that file's.
//
//   BIN_CODER::s[2]     n[2]        the two counts
//   BIN_CODER::MaxFreq  limit       the total at which they are halved
//   STEP = 10           kStep = 8   what a coded bit adds
//   scaleRare()         scale_rare()
//   initChild(bc)       seed_from(parent)
//   process(rc,bc,..)   code_context_bit<f_DEC>(parent, bit)
//
// The one structural difference is where the "seen once" mark lives.
// `BIN_CODER` marks itself unseeded with `MaxFreq == 0` and remembers the
// first bit in `s[1]`; this remembers it in `n[0]` as `bit + 1` and reads
// `n[1] == 0` as "not seeded yet".  Same machine, one slot over -- which is
// worth knowing, because `n[0]` is a *count* in the live state and a
// *remembered bit* before it, and nothing in the decompilation said so.
struct BitCtr {
  uint16_t n[2];
  uint16_t limit;

  // BIN_CODER's STEP, 8 here against its 10, and the same three uses: what a
  // coded bit adds, half of it as the seeding bump, and the ceiling below
  // which the parent is bumped too.
  static const uint32_t kStep = 8;
  static const uint32_t kSeedBump = 4;
  static const uint32_t kSeedLimit = 512;
  static const uint32_t kLimitStep = 64;
  static const uint32_t kMaxLimit = 0x4000;
  static const uint32_t kParentBump = 0x88;

  uint32_t summ() const {
    return n[0]+n[1];
  }

  // `BIN_CODER::initParent`: both sides at half a step, under a low ceiling,
  // so the first few bits move it a long way.  What `esc_ctr[0]` and every
  // `run_ctr[]` start as.
  void init_parent() {
    n[0] = n[1] = kStep/2;
    limit = 72;
  }

  // A root of the escape tree, which does not start even: 40 against 16 is
  // five to two toward "no", at the same ceiling `seed_from` hands a child.
  void init_root() {
    n[0] = 40;
    n[1] = 16;
    limit = kSeedLimit;
  }

  // And the third state has no initialiser at all: a counter is *unseen* when
  // it is all zeroes, which is why `bit_node[]` and `esc_ctr[1..]` are simply
  // memset.  bcdr does the same with `memset(BCdr2, 0, sizeof(BCdr2))`.

  // `BIN_CODER::scaleRare`.  Halve both counts and let the limit climb, so a
  // counter that keeps being right is allowed to get more confident before it
  // is cut back again.  `rescale_counter_pair` is this, called from the run
  // coder, and is now the same code rather than a second copy of it.
  int32_t scale_rare() {
    halve_counts(n, 2);
    if( limit<kMaxLimit )
      limit += kLimitStep;
    return limit;
  }

  // `BIN_CODER::initChild`.  A counter seen once takes its parent's shape
  // scaled to 64 -- `(64 * (parent - 1) + total) / total` on each side, which
  // is bcdr's formula with 32 -- bumps the side the first bit went, and takes
  // the parent's confidence down by 3 for having handed it over.
  void seed_from(BitCtr* parent) {
    const int32_t first = n[0]-1;
    const int32_t par0 = parent->n[0];
    const int32_t par_tot = par0+parent->n[1];
    n[0] = (par_tot+(par0<<6)-64)/par_tot;
    n[1] = ((parent->n[1]<<6)+par_tot-64)/par_tot;
    n[first] += kSeedBump;
    limit = kSeedLimit;
    const int32_t par_n = parent->n[first];
    parent->n[first] = par_n-3*(par_n>3);
  }

  // The two returns are not the same value and that is not a merge artefact:
  // the encoder returned the counter it had just bumped and the decoder
  // returns the bit.  Nothing reads the encoder's, both call sites drop it,
  // and it is kept anyway -- a merge is not the place to decide that a return
  // value is dead.
  template<int32_t f_DEC>
  int32_t code_context_bit(BitCtr* parent, int32_t bit) {
    int32_t result;
    if( n[0] ) {
      if( !n[1] )
        seed_from(parent);
      const uint32_t tot = summ();
      if constexpr( f_DEC )
        bit = rc.decode_bit(n[0], n[1]);
      else
        rc.encode_bit(n[0], n[1], bit);
      if( tot>limit )
        scale_rare();
      result = n[bit]+kStep;
      n[bit] = result;
      // `BIN_CODER`'s "kinda partial update": while this counter is still
      // cheap to be wrong about, the parent hears about the bit as well.
      parent->n[bit] += tot<kParentBump;
    } else {
      // Unseen.  The parent codes the bit, and this keeps it as `bit + 1` so
      // the next visit knows which side to bump when it seeds.
      const uint32_t p_tot = parent->summ();
      if constexpr( f_DEC )
        bit = rc.decode_bit(parent->n[0], parent->n[1]);
      else
        rc.encode_bit(parent->n[0], parent->n[1], bit);
      if( p_tot>parent->limit )
        parent->scale_rare();
      result = parent->n[bit]+kStep;
      parent->n[bit] = result;
      n[0] = bit+1;
    }
    return f_DEC ? bit : result;
  }

  int32_t encode_context_bit(BitCtr* parent, int32_t bit) {
    return code_context_bit<0>(parent, bit);
  }

  int32_t decode_context_bit(BitCtr* parent) {
    return code_context_bit<1>(parent, 0);
  }
};

CounterNode*init_counter_node(CounterNode* node) {
  node->c[0] = 8;
  node->c[1] = 2;
  node->c[2] = 2;
  node->c[3] = 2;
  node->c[4] = 2;
  node->c[5] = 3;
  node->c[6] = 3;
  node->total = 22;
  return node;
}
