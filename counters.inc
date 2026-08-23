// counters.inc -- the adaptive counters: CounterNode, BitCtr, and the context mixers
// Included from bmf.cpp; not a standalone translation unit.

int32_t sym_slot(int32_t code) {
  return code<CN_slot_split ? code : CN_slot_top-(code&1);
}

struct CounterNode {
  uint16_t total;
  uint16_t c[7];
  // Slot 0 -- the residual the predictor got exactly right -- carries most of
  // this cell's bits, so it is split off the seven-way code as a binary
  // decision and refined through a map before the flat code over the rest.  The
  // split is exactly the same code by the chain rule.
  template <int32_t f_DEC> int32_t code_symbol(BMFState* cx, int32_t ctx, int32_t sym, Apm* apm) {
    uint32_t cum;
    int32_t slot;
    uint16_t* cur = c;
    const uint32_t tot = total&0x7FFF;
    const uint32_t rest = tot-c[0];
    if( !rest ) {
      // Nothing outside slot 0 has mass; the split would need a zero-width
      // interval, so code it flat as it was.
      if constexpr( f_DEC ) {
        sym = cx[0].get_freq(tot);
        cum = c[0];
        while( cum<=(uint32_t)sym )
          cum += *++cur;
        cx[0].decode(cum-cur[0], cum, tot);
      } else {
        slot = sym_slot(sym);
        cum = 0;
        for( int32_t k = 0; k<slot; k++ )
          cum += c[k];
        cur = &c[slot];
        cx[0].encode(cum, cum+cur[0], tot);
      }
    } else {
      int32_t p16 = (int32_t)(((uint32_t)c[0]*65536u+(tot>>1))/tot);
      if( p16<32 )
        p16 = 32;
      if( p16>65504 )
        p16 = 65504;
      const int32_t p_ref = apm[0].refine(p16, ctx&15);
      int32_t is_zero;
      if constexpr( f_DEC ) {
        is_zero = cx[0].decode_bit(65536-p_ref, p_ref);
      } else {
        is_zero = sym_slot(sym)==0;
        cx[0].encode_bit(65536-p_ref, p_ref, is_zero);
      }
      apm[0].update(is_zero);
      if( !is_zero ) {
        cur = &c[1];
        if constexpr( f_DEC ) {
          sym = cx[0].get_freq(rest);
          cum = c[1];
          while( cum<=(uint32_t)sym )
            cum += *++cur;
          cx[0].decode(cum-cur[0], cum, rest);
        } else {
          slot = sym_slot(sym);
          cum = 0;
          for( int32_t k = 1; k<slot; k++ )
            cum += c[k];
          cur = &c[slot];
          cx[0].encode(cum, cum+cur[0], rest);
        }
      }
    }
    if( tot>CN_rescale_at ) {
      total = 0x8000;
      int32_t lo = 256;
      for( int32_t k = 6; k>=0; --k )
        if( c[k]<lo )
          lo = c[k];
      for( int32_t k = 6; k>=0; --k ) {
        const uint16_t nf = lo<=1 ? c[k]-(c[k]>>idx_clamp(CN_halve_shift, 1, 15))
                                 : (c[k]+CN_rescale_round)/idx_clamp(CN_rescale_div, 2, 64);
        c[k] = nf;
        total += nf;
      }
    }
    cur[0] += CN_bump;
    const int32_t result = total+CN_bump;
    total = result;
    slot = cur-c;
    if( slot>=CN_slot_split ) {
      uint16_t*const strip = cx[0].model_strip(CtxIdx{}.bit<IX_counters_strip_p0>(slot&1).bit<IX_counters_strip_p1>(2u*c[slot]>c[0]+(uint32_t)(result&0x7FFF)+CN_escape_bias).raw((uint32_t)ctx));
      if constexpr( f_DEC )
        slot += 2*cx[0].decode_symbol_tree(strip);
      else
        cx[0].encode_symbol_tree(strip, (sym-CN_slot_split)>>1);
    }
    return slot;
  }
  void bump(int32_t slot, uint32_t n) {
    c[slot] += n;
    total += n;
  }

  bool escape_high(int32_t slot) const {
    return 2u*c[slot]>(total&0x7FFFu)+c[0];
  }
};


struct SymPair {
  uint16_t last;
  uint16_t prev;
};



// These were in-class static members.  In the tuning build an IDX value is a
// read from a patchable object rather than a constant expression, and an
// in-class initializer has to be one -- so they sit at namespace scope instead,
// where a dynamic initializer is fine.  The names are unqualified at every use
// site, so nothing else changes.
// A step of zero leaves both counts at zero for ever, and the pair is
// divided by its own total.
static const uint32_t kStep = (uint32_t)idx_clamp(CN_step, 1, 1<<14);
static const uint32_t kSeedBump = CN_seed_bump;
static const uint32_t kSeedLimit = CN_seed_limit;
static const uint32_t kLimitStep = CN_limit_step;
static const uint32_t kMaxLimit = CN_max_limit;
static const uint32_t kParentBump = CN_parent_bump;

struct BitCtr {
  uint16_t n[2];
  uint16_t limit;
  uint32_t summ() const {
    return n[0]+n[1];
  }

  void init_parent() {
    n[0] = n[1] = kStep/2;
    limit = CN_parent_limit;
  }

  void init_root() {
    n[0] = CN_root_n0;
    n[1] = CN_root_n1;
    limit = kSeedLimit;
  }

  int32_t scale_rare() {
    halve_counts(n, 2);
    if( limit<kMaxLimit )
      limit += kLimitStep;
    return limit;
  }

  void seed_from(BitCtr* parent) {
    const int32_t first = n[0]-1;
    const int32_t par0 = parent[0].n[0];
    const int32_t par_tot = par0+parent[0].n[1];
    n[0] = (par_tot+(par0-1)*CN_seed_scale)/par_tot;
    n[1] = (par_tot+((int32_t)parent[0].n[1]-1)*CN_seed_scale)/par_tot;
    n[first] += kSeedBump;
    limit = kSeedLimit;
    const int32_t par_n = parent[0].n[first];
    parent[0].n[first] = par_n-CN_seed_decay*(par_n>CN_seed_decay);
  }

  template <int32_t f_DEC> int32_t code_context_bit(BMFState* cx, BitCtr* parent, int32_t bit) {
    int32_t result;
    if( n[0] ) {
      if( !n[1] )
        seed_from(parent);
      const uint32_t tot = summ();
      if constexpr( f_DEC )
        bit = cx[0].decode_bit(n[0], n[1]);
      else
        cx[0].encode_bit(n[0], n[1], bit);
      if( tot>limit )
        scale_rare();
      result = n[bit]+kStep;
      n[bit] = result;
      parent[0].n[bit] += tot<kParentBump;
    } else {
      const uint32_t p_tot = parent[0].summ();
      if constexpr( f_DEC )
        bit = cx[0].decode_bit(parent[0].n[0], parent[0].n[1]);
      else
        cx[0].encode_bit(parent[0].n[0], parent[0].n[1], bit);
      if( p_tot>parent[0].limit )
        parent[0].scale_rare();
      result = parent[0].n[bit]+kStep;
      parent[0].n[bit] = result;
      n[0] = bit+1;
    }
    return f_DEC ? bit : result;
  }

  // The same code, with the counter's probability refined through a map first.
  // The caller owns the low bits of apm_ctx; the top bit says whether the
  // counter had any statistics of its own or fell back to its parent, which is
  // a different enough distribution to be worth separating.
  // The counter pair still learns exactly as it did; only what reaches the
  // arithmetic coder changes.
  template <int32_t f_DEC> int32_t code_context_bit_apm(BMFState* cx, BitCtr* parent, int32_t bit, Apm* apm, int32_t apm_ctx) {
    BitCtr*const src = n[0] ? this : parent;
    if( n[0]&&!n[1] )
      seed_from(parent);
    const uint32_t f0 = src[0].n[0], f1 = src[0].n[1];
    int32_t p16 = (int32_t)((f1*65536u+((f0+f1)>>1))/(f0+f1));
    if( p16<CN_p16_floor )
      p16 = CN_p16_floor;
    if( p16>CN_p16_ceil )
      p16 = CN_p16_ceil;
    const int32_t p_ref = apm[0].refine(p16, apm_ctx|((src==parent) ? Apm::kCtx>>1 : 0));
    if constexpr( f_DEC )
      bit = cx[0].decode_bit(65536-p_ref, p_ref);
    else
      cx[0].encode_bit(65536-p_ref, p_ref, bit);
    apm[0].update(bit);
    int32_t result;
    if( src==this ) {
      const uint32_t tot = summ();
      if( tot>limit )
        scale_rare();
      result = n[bit]+kStep;
      n[bit] = result;
      parent[0].n[bit] += tot<kParentBump;
    } else {
      const uint32_t p_tot = parent[0].summ();
      if( p_tot>parent[0].limit )
        parent[0].scale_rare();
      result = parent[0].n[bit]+kStep;
      parent[0].n[bit] = result;
      n[0] = bit+1;
    }
    return f_DEC ? bit : result;
  }

  int32_t encode_context_bit(BMFState* cx, BitCtr* parent, int32_t bit) {
    return code_context_bit<0>(cx, parent, bit);
  }

  int32_t decode_context_bit(BMFState* cx, BitCtr* parent) {
    return code_context_bit<1>(cx, parent, 0);
  }
};

CounterNode*init_counter_node(CounterNode* node) {
  node[0].c[0] = CN_seed_0;
  node[0].c[1] = CN_seed_1;
  node[0].c[2] = CN_seed_2;
  node[0].c[3] = CN_seed_3;
  node[0].c[4] = CN_seed_4;
  node[0].c[5] = CN_seed_5;
  node[0].c[6] = CN_seed_6;
  node[0].total = CN_seed_0+CN_seed_1+CN_seed_2+CN_seed_3+CN_seed_4+CN_seed_5+CN_seed_6;
  return node;
}

template <class Unfold> uint8_t fold_or_refuse(uint8_t* dst, int32_t src, int32_t pred, const uint8_t* fold, const uint8_t* fold_hi, const Unfold* unfold) {
  const int32_t resid = (uint8_t)(src-pred);
  const uint8_t recon = unfold[fold[resid]]+pred;
  const int32_t drift = dst[0]-(uint8_t)(recon+dst[0]-src);
  if( drift<-CN_drift_limit||drift>CN_drift_limit ) {
    dst[0] = (uint8_t)src;
    return fold_hi[resid];
  }
  dst[0] = recon;
  return fold[resid];
}

struct CtxWeight {
  int32_t sel;
  uint32_t w[3];
};
uint32_t mixer_fwd(const CtxWeight* cw, int32_t n) {
  uint32_t sum = 0;
  for( int32_t k = 0; k<n; ++k )
    sum += cw[k].w[cw[k].sel];
  return sum;
}

uint32_t mixer_rev(const CtxWeight* cw, int32_t n) {
  uint32_t sum = cw[0].w[1];
  for( int32_t k = 1; k<n; ++k )
    sum += cw[k].w[2-cw[k].sel];
  return sum;
}

int32_t tri_sign(int32_t v, int32_t dead = 0) {
  return (v>=-dead)+(v>dead);
}

struct P1Ctx {
  uint8_t sym;
  uint8_t mag;
};



void alt_p1_encode_symbol(BMFState* cx, CounterNode* node, int32_t ctx, int32_t sym, Apm* apm) {
  node[0].code_symbol<0>(cx, ctx, sym, apm);
}

int32_t alt_p1_decode_symbol(BMFState* cx, CounterNode* node, int32_t ctx, Apm* apm) {
  return node[0].code_symbol<1>(cx, ctx, 0, apm);
}
