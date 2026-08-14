// sym_code.inc -- the symbol trees: a symbol through a tree of binary counters, both
// ways, for both models.

// One symbol out of a level's tree.  The two directions meet at `lvl`, the slot
// in the level frequencies: the encoder is handed a symbol and looks its level
// up in `model_geometry`, the decoder walks the same frequencies until it
// passes the point the coder gave it.  From there the rescale, the escape
// counter and the walk down the binary tree are the same code.
//
// The guard on the walk reads `lvl >= 2` for both, where the encoder said
// `sym >= 2`.  They are the same test: `model_geometry` is 0 at 0, 1 at 1 and
// 2 or more everywhere above -- __rc_begin lays it out that way -- so a symbol
// below two has its own index as its level, which is also what the decoder
// returns for those two.
//
// The rescale halves freq[2..9] and leaves the sum in *freq, and the two bodies
// did it with the loads and stores interleaved differently and different
// intermediates -- acc6/acc8/h9 against sum6/h6/h8, some int16_t and some
// uint16_t.  It is the same sum: every intermediate is 16 bits wide and the
// result is stored through a uint16_t, so the orderings agree in the low 16
// bits, which is all of them that survive.  The encoder's spellings are kept.
template<int f_DEC>
int32_t __code_symbol_tree(uint16_t* freq, int32_t sym) {
  uint16_t add;
  uint32_t cum;
  int32_t lvl, result, go, path, node, span;
  uint16_t fq, *slot;
  uint32_t j;
  uint16_t h9, acc8, acc6, acc;
  uint16_t *f0 = freq+2;
  uint32_t tot = *freq;
  if( f_DEC ) {
    uint32_t target = rc.get_freq(tot);
    slot = f0;
    lvl = 0;
    cum = *f0;
    if( cum<=target ) {
      do {
        ++slot;
        ++lvl;
        cum += *slot;
      } while( cum<=target );
    }
    rc.decode(cum-*slot, cum, tot);
  } else {
    lvl = model_geometry[sym];
    cum = 0;
    for( j = 0; j<(uint32_t)lvl; j++ )
      cum += f0[j];
    slot = &f0[lvl];
    uint32_t cum_hi = cum+*slot;
    result = rc.encode(cum, cum_hi, tot);
  }
  if( *freq>0x4000u ) {
    uint16_t h2 = *(freq+2)-(*(freq+2)>>1);
    acc = *(freq+3);
    *(freq+2) = h2;
    acc = acc-(acc>>1);
    *(freq+3) = acc;
    acc = h2+acc;
    uint16_t h4 = *(freq+4)-(*(freq+4)>>1);
    *(freq+4) = h4;
    int16_t sum4 = acc+h4;
    acc = *(freq+5)-(*(freq+5)>>1);
    acc6 = *(freq+6);
    *(freq+5) = acc;
    acc6 = acc6-(acc6>>1);
    *(freq+6) = acc6;
    acc6 = sum4+acc+acc6;
    acc = *(freq+7)-(*(freq+7)>>1);
    acc8 = *(freq+8);
    *(freq+7) = acc;
    acc = acc6+acc;
    acc8 = acc8-(acc8>>1);
    h9 = *(freq+9);
    *(freq+8) = acc8;
    acc8 = acc+acc8;
    h9 = h9-(h9>>1);
    int32_t esc = *(freq+1);
    *(freq+9) = h9;
    *freq = acc8+h9;
    if( esc<=4*alt_freq_limit ) {
      result = 4*(esc>alt_freq_limit);
      add = esc-result;
      *(freq+1) = esc-result;
    } else {
      add = esc-16;
      *(freq+1) = esc-16;
    }
  } else {
    add = *(freq+1);
  }
  *slot += add;
  *freq += *(freq+1);
  if( lvl<2 )
    return lvl;
  int32_t mask = level_geom[lvl].half;
  if( !f_DEC )
    path = sym-level_geom[lvl].first;
  node = 0;
  for( span = 1;; span *= 2 ) {
    FreqPair *pair = bit_tree(freq, lvl)+span+node;
    int32_t f1 = pair->f[1];
    int32_t fa = pair->f[0];
    if( f_DEC ) {
      go = rc.decode_bit(fa, f1);
    } else {
      go = (mask&path)!=0;
      result = rc.encode_bit(fa, f1, go);
    }
    fq = pair->f[go];
    if( fq>0x4000u ) {
      uint32_t f1_old = pair->f[1];
      pair->f[0] -= pair->f[0]>>1;
      pair->f[1] = f1_old-(f1_old>>1);
      fq = pair->f[go];
    }
    pair->f[go] = alt_freq_init+fq;
    mask >>= 1;
    node = go+2*node;
    if( !mask )
      return node+level_geom[lvl].first;
  }
}

void __encode_symbol_tree(uint16_t* freq, int32_t sym) {
  __code_symbol_tree<0>(freq, sym);
}

int32_t __decode_symbol_tree(uint16_t* freq) {
  return __code_symbol_tree<1>(freq, 0);
}

// One symbol of the one-bit model's alphabet.  The two directions meet at the
// slot: the encoder is given a symbol, maps it to a slot and sums the
// frequencies below it; the decoder is given a point in the range and walks the
// same frequencies until it passes it.  Everything after -- the rescale at
// 0x2000, the +32 on the slot and the total, the escape into the binary tree
// for slots 5 and up -- is the same code, and was written twice.
//
// `slot` is `cur - base` in both, which is why the tail can be shared: the
// encoder's `cur = &base[slot]` and the decoder's walk leave it in the same
// place.  The decoder folds the tree's answer back into the slot; the encoder
// sends it and has nothing to fold.
//
// The two bodies differed in a handful of casts as well -- `(uint16_t)freq[1]`
// against `freq[1]`, `int16_t tot_new` against `int32_t result`.  None of them
// is a difference: `freq` is `uint16_t *`, so the casts are the promotion that
// was already happening, and the two totals agree in the low 16 bits that
// `*freq` and `& 0x7FFF` keep.  The encoder's spellings are the ones kept.
template<int f_DEC>
int32_t __alt_p1_code_symbol(uint16_t* freq, int32_t ctx, int32_t sym) {
  bool done;
  uint32_t cum;
  int32_t slot, lo;
  uint16_t nf, *cur;
  uint32_t k;
  uint16_t tot0 = *freq;
  uint16_t *base = freq+1;
  cur = freq+1;
  uint32_t tot = tot0&0x7FFF;
  if( f_DEC ) {
    sym = rc.get_freq(tot);
    cum = (uint16_t)freq[1];
    while( cum<=(uint32_t)sym )
      cum += (uint16_t)*++cur;
    rc.decode(cum-(uint16_t)*cur, cum, tot);
  } else {
    slot = 6-(sym&1);
    if( sym<5 )
      slot = sym;
    cum = 0;
    for( k = 0; k<(uint32_t)slot; k++ )
      cum += base[k];
    cur = &base[slot];
    uint32_t cum_hi = cum+*cur;
    rc.encode(cum, cum_hi, tot);
  }
  if( tot>0x2000 ) {
    *freq = 0x8000;
    uint16_t *p = freq+7;
    lo = 256;
    done = freq+7<base;
    uint16_t *q = freq+7;
    if( freq+7>=base ) {
      do {
        if( *q<lo )
          lo = *q;
        --q;
      } while( q>=base );
      done = p<base;
    }
    if( !done ) {
      do {
        if( (uint16_t)lo<=1u )
          nf = *p-(*p>>1);
        else
          nf = (*p+2)/3;
        *p = nf;
        *freq += nf;
        --p;
      } while( p>=base );
    }
  }
  *cur += 32;
  int32_t result = *freq+32;
  *freq = result;
  slot = cur-base;
  if( slot>=5 ) {
    uint16_t*const strip = model_strip(128*(uint32_t)(slot&1)
        +((((freq[1]+(result&0x7FFF)+96-2*(uint32_t)freq[slot+1])>>25)&0xFFFFFFC0)
          +(uint32_t)ctx));
    if( f_DEC )
      slot += 2*__decode_symbol_tree(strip);
    else
      __encode_symbol_tree(strip, (sym-5)>>1);
  }
  return slot;
}

// `unread_ctx` and `unread_arg` are the decompiler's names for an argument the
// donor pushed and neither body reads; the call sites still pass it.
void __alt_p1_encode_symbol(uint16_t* freq, int32_t unread_ctx, int32_t ctx, int32_t sym) {
  __alt_p1_code_symbol<0>(freq, ctx, sym);
}

int32_t __alt_p1_decode_symbol(uint16_t* freq, int32_t unread_arg, int32_t ctx) {
  return __alt_p1_code_symbol<1>(freq, ctx, 0);
}



int32_t __update_binary_pair(uint16_t* _this, int32_t symbol) {
  int32_t tot, node;
  uint32_t step, f;
  tot = *_this;
  if( (uint32_t)tot<=0x8000 ) {
    int32_t lvl = model_geometry[symbol];
    step = (_this[1]>>2)&0xFFFFFFE0;
    if( plane_predictor==2 )
      step = 15*(_this[1]>>5);
    *(_this+lvl+2) += step+4;
    tot = *_this+step+4;
    *_this = tot;
    if( symbol>=2 ) {
      int32_t mask = level_geom[lvl].half;
      int32_t path = symbol-level_geom[lvl].first;
      node = 0;
      FreqPair *tbl = bit_tree(_this, lvl);
      int32_t span = 1;
      do {
        FreqPair *pair = tbl+node+span;
        int32_t go = (mask&path)!=0;
        f = pair->f[go];
        if( f>0x2000 ) {
          uint32_t f1_old = pair->f[1];
          pair->f[0] -= pair->f[0]>>1;
          pair->f[1] = f1_old-(f1_old>>1);
          f = pair->f[go];
        }
        span *= 2;
        mask >>= 1;
        node = go+2*node;
        pair->f[go] = f+((alt_freq_init*((uint32_t)(plane_predictor==2)+5))>>3);
      } while( mask );
      return 0;
    }
  }
  return tot;
}

// The histogram's entropy, in bits, as one number:
//
//     (N*ln N - sum of n_i*ln n_i) * log2(e)
//
// which is `N * H(p)` -- the total cost of coding N symbols with these
// frequencies, not the per-symbol cost.  Both decision bodies compare
// candidates through this and nothing else.
//
// The two accumulators over even and odd bins are the original's pairing, for
// a two-lane SSE add that is not here any more; they change the rounding of
// the sum and nothing else.
int32_t __estimate_cost(uint8_t* bins, int32_t n) {
  const int32_t* bin = (const int32_t*)bins;
  double sum_even = 0.0, sum_odd = 0.0, ent_even = 0.0, ent_odd = 0.0;
  double total;
  int32_t i;
  for( i = 0; i+1<n; i += 2 ) {
    if( bin[i] ) {
      sum_even += (double)bin[i];
      ent_even += (double)bin[i]*log((double)bin[i]);
    }
    if( bin[i+1] ) {
      sum_odd += (double)bin[i+1];
      ent_odd += (double)bin[i+1]*log((double)bin[i+1]);
    }
  }
  double entropy = ent_even+ent_odd;
  total = sum_even+sum_odd;
  for(; i<n; i++ )
    if( bin[i] ) {
      total += (double)bin[i];
      entropy += (double)bin[i]*log((double)bin[i]);
    }
  if( total!=0.0 )
    total = total*log(total);
  return (int32_t)((total-entropy)*1.442695040888963);
}
