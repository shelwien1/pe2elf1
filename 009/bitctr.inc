// bitctr.inc -- the binary counters -- one context bit, coded and decoded, and the
// pair rescale behind them.

struct CounterNode {
  uint16_t total;
  uint16_t c[7];
};

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

struct BitCtr {
  uint16_t n[2];
  uint16_t limit;
  int32_t encode_context_bit(BitCtr* a2, int32_t bit) {
    uint32_t c1, p0, p1, c0;
    int32_t par_tot;
    int32_t cap, result, p_cap, par0, par_n;
    uint32_t tot, p_tot, n1_old, p1_old;
    c0 = this->n[0];
    if( this->n[0] ) {
      c1 = this->n[1];
      if( !this->n[1] ) {
        par0 = a2->n[0];
        par_tot = par0+a2->n[1];
        this->n[0] = (par_tot+(par0<<6)-64)/par_tot;
        this->n[1] = ((a2->n[1]<<6)+par_tot-64)/par_tot;
        this->n[c0-1] += 4;
        this->limit = 512;
        par_n = a2->n[c0-1];
        a2->n[c0-1] = -3*((uint32_t)(3-par_n)>>31)+par_n;
        c0 = this->n[0];
        c1 = this->n[1];
      }
      tot = c0+c1;
      rc.encode_bit(c0, c1, bit);
      cap = this->limit;
      if( tot>(uint32_t)cap ) {
        n1_old = this->n[1];
        this->n[0] -= this->n[0]>>1;
        this->n[1] = n1_old-(n1_old>>1);
        if( cap<0x4000 )
          this->limit = cap+64;
      }
      result = this->n[bit]+8;
      this->n[bit] = result;
      a2->n[bit] += (uint32_t)tot<0x88;
      return result;
    }
    p0 = a2->n[0];
    p1 = a2->n[1];
    p_tot = p0+p1;
    rc.encode_bit(p0, p1, bit);
    p_cap = a2->limit;
    if( p_tot>(uint32_t)p_cap ) {
      p1_old = a2->n[1];
      a2->n[0] -= a2->n[0]>>1;
      a2->n[1] = p1_old-(p1_old>>1);
      if( p_cap<0x4000 )
        a2->limit = p_cap+64;
    }
    result = a2->n[bit]+8;
    a2->n[bit] = result;
    this->n[0] = bit+1;
    return result;
  }

  int32_t decode_context_bit(BitCtr* a2) {
    uint32_t c0, c1, p0, p1;
    int32_t par_tot;
    int32_t result, cap, p_cap, par0, par_n;
    uint32_t tot, p_tot, n1_old, p1_old;
    c0 = this->n[0];
    if( this->n[0] ) {
      c1 = this->n[1];
      if( !this->n[1] ) {
        par0 = a2->n[0];
        par_tot = par0+a2->n[1];
        this->n[0] = (par_tot+(par0<<6)-64)/par_tot;
        this->n[1] = ((a2->n[1]<<6)+par_tot-64)/par_tot;
        this->n[c0-1] += 4;
        this->limit = 512;
        par_n = a2->n[c0-1];
        a2->n[c0-1] = -3*((uint32_t)(3-par_n)>>31)+par_n;
        c0 = this->n[0];
        c1 = this->n[1];
      }
      tot = c0+c1;
      result = rc.decode_bit(c0, c1);
      cap = this->limit;
      if( tot>(uint32_t)cap ) {
        n1_old = this->n[1];
        this->n[0] -= this->n[0]>>1;
        this->n[1] = n1_old-(n1_old>>1);
        if( cap<0x4000 )
          this->limit = cap+64;
      }
      this->n[result] += 8;
      a2->n[result] += (uint32_t)tot<0x88;
    } else {
      p0 = a2->n[0];
      p1 = a2->n[1];
      p_tot = p0+p1;
      result = rc.decode_bit(p0, p1);
      p_cap = a2->limit;
      if( p_tot>(uint32_t)p_cap ) {
        p1_old = a2->n[1];
        a2->n[0] -= a2->n[0]>>1;
        a2->n[1] = p1_old-(p1_old>>1);
        if( p_cap<0x4000 )
          a2->limit = p_cap+64;
      }
      a2->n[result] += 8;
      this->n[0] = result+1;
    }
    return result;
  }
};

CounterNode*__init_counter_node(CounterNode* node) {
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

int32_t __rescale_counter_pair(BitCtr* ctr) {
  int32_t tot;
  ctr->n[0] -= ctr->n[0]>>1;
  tot = ctr->limit;
  ctr->n[1] -= ctr->n[1]>>1;
  if( tot<0x4000 ) {
    tot += 64;
    ctr->limit = tot;
  }
  return tot;
}
