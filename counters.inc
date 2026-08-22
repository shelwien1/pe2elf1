// counters.inc -- the adaptive counters: CounterNode, BitCtr, and the context mixers
// Included from bmf.cpp; not a standalone translation unit.

int32_t sym_slot(int32_t code) {
  return code<5 ? code : 6-(code&1);
}

struct CounterNode {
  uint16_t total;
  uint16_t c[7];
  template <int32_t f_DEC> int32_t code_symbol(BMFState* cx, int32_t ctx, int32_t sym) {
    uint32_t cum;
    int32_t slot;
    uint16_t* cur = c;
    const uint32_t tot = total&0x7FFF;
    if constexpr( f_DEC ) {
      sym = cx[0].rc.get_freq(tot);
      cum = c[0];
      while( cum<=(uint32_t)sym )
        cum += *++cur;
      cx[0].rc.decode(cum-cur[0], cum, tot);
    } else {
      slot = sym_slot(sym);
      cum = 0;
      for( int32_t k = 0; k<slot; k++ )
        cum += c[k];
      cur = &c[slot];
      cx[0].rc.encode(cum, cum+cur[0], tot);
    }
    if( tot>0x2000 ) {
      total = 0x8000;
      int32_t lo = 256;
      for( int32_t k = 6; k>=0; --k )
        if( c[k]<lo )
          lo = c[k];
      for( int32_t k = 6; k>=0; --k ) {
        const uint16_t nf = lo<=1 ? c[k]-(c[k]>>1) : (c[k]+2)/3;
        c[k] = nf;
        total += nf;
      }
    }
    cur[0] += 32;
    const int32_t result = total+32;
    total = result;
    slot = cur-c;
    if( slot>=5 ) {
      uint16_t*const strip = cx[0].model_strip(CtxIdx{}.bit<7>(slot&1).bit<6>(2u*c[slot]>c[0]+(uint32_t)(result&0x7FFF)+96).raw((uint32_t)ctx));
      if constexpr( f_DEC )
        slot += 2*cx[0].decode_symbol_tree(strip);
      else
        cx[0].encode_symbol_tree(strip, (sym-5)>>1);
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



struct BitCtr {
  uint16_t n[2];
  uint16_t limit;
  static const uint32_t kStep = 8;
  static const uint32_t kSeedBump = 4;
  static const uint32_t kSeedLimit = 512;
  static const uint32_t kLimitStep = 64;
  static const uint32_t kMaxLimit = 0x4000;
  static const uint32_t kParentBump = 0x88;
  uint32_t summ() const {
    return n[0]+n[1];
  }

  void init_parent() {
    n[0] = n[1] = kStep/2;
    limit = 72;
  }

  void init_root() {
    n[0] = 40;
    n[1] = 16;
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
    n[0] = (par_tot+(par0<<6)-64)/par_tot;
    n[1] = ((parent[0].n[1]<<6)+par_tot-64)/par_tot;
    n[first] += kSeedBump;
    limit = kSeedLimit;
    const int32_t par_n = parent[0].n[first];
    parent[0].n[first] = par_n-3*(par_n>3);
  }

  template <int32_t f_DEC> int32_t code_context_bit(BMFState* cx, BitCtr* parent, int32_t bit) {
    int32_t result;
    if( n[0] ) {
      if( !n[1] )
        seed_from(parent);
      const uint32_t tot = summ();
      if constexpr( f_DEC )
        bit = cx[0].rc.decode_bit(n[0], n[1]);
      else
        cx[0].rc.encode_bit(n[0], n[1], bit);
      if( tot>limit )
        scale_rare();
      result = n[bit]+kStep;
      n[bit] = result;
      parent[0].n[bit] += tot<kParentBump;
    } else {
      const uint32_t p_tot = parent[0].summ();
      if constexpr( f_DEC )
        bit = cx[0].rc.decode_bit(parent[0].n[0], parent[0].n[1]);
      else
        cx[0].rc.encode_bit(parent[0].n[0], parent[0].n[1], bit);
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
  node[0].c[0] = 8;
  node[0].c[1] = 2;
  node[0].c[2] = 2;
  node[0].c[3] = 2;
  node[0].c[4] = 2;
  node[0].c[5] = 3;
  node[0].c[6] = 3;
  node[0].total = 22;
  return node;
}

template <class Unfold> uint8_t fold_or_refuse(uint8_t* dst, int32_t src, int32_t pred, const uint8_t* fold, const uint8_t* fold_hi, const Unfold* unfold) {
  const int32_t resid = (uint8_t)(src-pred);
  const uint8_t recon = unfold[fold[resid]]+pred;
  const int32_t drift = dst[0]-(uint8_t)(recon+dst[0]-src);
  if( drift<-16||drift>16 ) {
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


void alt_p1_encode_symbol(BMFState* cx, CounterNode* node, int32_t ctx, int32_t sym);

int32_t alt_p1_decode_symbol(BMFState* cx, CounterNode* node, int32_t ctx);
