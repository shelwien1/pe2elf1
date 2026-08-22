// sym_list.inc -- FreqRec and SymList: the adaptive frequency records and symbol lists
// Included from bmf.cpp; not a standalone translation unit.

struct FreqRec {
  union {
    uint16_t w[8];
    struct {
      uint16_t _w0to6[7];
      uint8_t b14;
      uint8_t b15;
    };
  };
  bool seed_pair(int32_t lo, int32_t hi, int32_t alphabet_lvl) {
    if( !w[lo]||!w[hi]||b14>alphabet_lvl ) return false;
    const int16_t weight = (uint8_t)(1<<((5-b14)&31));
    b15 = weight;
    w[6] = weight<<6;
    w[lo] += weight;
    w[hi] += b15;
    resum();
    return true;
  }

  int32_t find_level(int32_t target, int32_t &cum) const {
    int32_t lvl = 0;
    cum = w[0];
    while( lvl<4&&cum<=target ) cum += w[++lvl];
    return lvl;
  }

  void bump(int32_t lvl) {
    const uint16_t floor_ = w[6];
    uint16_t total = w[5];
    uint8_t kick = b15;
    if( total>floor_&&(w[lvl]+kick+8<total||w[5]>0x4000u) ) total = rescale(floor_, kick);
    w[5] = kick+total;
    w[lvl] += kick;
  }

  void blend_from(const FreqRec &src) {
    const int32_t scale = src.b15;
    int32_t kept[5];
    for( int32_t i = 0; i<5; ++i ) kept[i] = scale*w[i];
    this[0] = src;
    b14 *= 8;
    const int32_t total = w[5];
    for( int32_t i = 0; i<5; ++i ) w[i] = (uint16_t)(kept[i]+(21*w[i]+total-1)/total);
    resum();
  }

  uint16_t resum() {
    uint16_t sum = (uint16_t)(w[0]+w[1]+w[2]+w[3]+w[4]);
    w[5] = sum;
    return sum;
  }

  uint16_t rescale(uint16_t escape_floor, uint8_t &kick) {
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
    SymEntry* back;
    if( count<=251&&due>=since_rescale )
      return false;
    n_left = live;
    int32_t bias = due<20*n;
    SymEntry* cur = ent-1;
    SymEntry* prev = cur;
    do {
      prev = cur;
      ++cur;
      int32_t half = (bias+(uint32_t)cur[0].cnt)>>1;
      cur[0].cnt = half;
      if( cur!=ent ) {
        SymEntry* up = cur-1;
        int32_t up_cnt = up[0].cnt;
        if( half>up_cnt ) {
          uint16_t keep = cur[0].sym;
          cur[0].set(up[0].sym, up_cnt);
          if( up!=ent ) {
            do {
              back = up-1;
              int32_t back_cnt = back[0].cnt;
              if( half<=back_cnt )
                break;
              up[0].set(back[0].sym, back_cnt);
              --up;
            } while( back!=ent );
          }
          up[0].set(keep, half);
        }
      }
      --n_left;
    } while( n_left );
    running = tot;
    if( !cur[0].cnt ) {
      do {
        ++n_left;
        tot = ++running;
        last_cnt = prev[0].cnt;
        --prev;
      } while( !last_cnt );
      live -= n_left;
    }
    since = since_rescale;
    tot = halve_up(running);
    since_rescale = halve_up(since);
    return true;
  }

  int32_t move_up(SymEntry* p) {
    if( p==ent )
      return p[0].cnt;
    uint16_t s_a = p[0].sym;
    uint8_t c_a = p[0].cnt;
    SymEntry* q = p-1;
    p[0] = q[0];
    q[0].set(s_a, c_a);
    if( q==ent )
      return q[0].cnt;
    while( 1 ) {
      int32_t top = q[0].cnt;
      SymEntry* r = q-1;
      if( top<=r[0].cnt )
        return top;
      uint16_t s_b = q[0].sym;
      uint8_t c_b = q[0].cnt;
      q[0] = r[0];
      r[0].set(s_b, c_b);
      --q;
      if( r==ent )
        return r[0].cnt;
    }
  }

  int32_t promote(SymEntry* p) {
    p[0].cnt += 4;
    since_rescale += 4;
    return move_up(p);
  }

  template <int32_t f_DEC> uint32_t code_symbol_bytes(BMFState* cx, uint32_t nbytes, uint32_t val, int32_t &carry) {
    SymList*const lists = this;
    uint32_t word = val, out = 0;
    for( uint32_t b = 0; b<nbytes; ++b ) {
      uint32_t piece;
      if constexpr( f_DEC ) {
        piece = lists[4*b+carry].decode_symbol_list(cx);
        out += piece<<((8*b)&31);
      } else {
        piece = (uint8_t)word;
        lists[4*b+carry].code_symbol(cx, (uint8_t)piece);
        word >>= 8;
      }
      carry = piece>>6;
    }
    if constexpr( !f_DEC )
      out = val;
    carry = (uint8_t)out>>7;
    return out;
  }

  int32_t code_symbol(BMFState* cx, int32_t want) {
    int32_t enc_cum, enc_high, enc_tot;
    int32_t c, c2;
    uint32_t i;
    int8_t gen = cx[0].exclusion_gen;
    uint32_t left = live;
    SymEntry* p = ent-1;
    int32_t cum = 0;
    while( 1 ) {
      ++p;
      int32_t s = p[0].sym;
      if( cx[0].exclusion_mask[s]!=cx[0].exclusion_gen ) {
        c = p[0].cnt;
        cum += c;
        if( s==want )
          break;
      }
      if( --left )
        continue;
      if( !cum )
        return 0;
      enc_cum = cum;
      enc_tot = tot+cum;
      enc_high = enc_tot;
      do {
        cx[0].exclusion_mask[p[0].sym] = gen;
        --p;
      } while( p>=ent );
      cx[0].rc.encode(enc_cum, enc_high, enc_tot);
      return 0;
    }
    enc_high = cum;
    uint32_t rest = left-1;
    enc_cum = cum-c;
    if( rest ) {
      for( i = 0; i<rest; ++i ) {
        if( cx[0].exclusion_mask[p[i+1].sym]==cx[0].exclusion_gen )
          c2 = 0;
        else
          c2 = p[i+1].cnt;
        cum += c2;
      }
    }
    enc_tot = tot+cum;
    rescale(promote(p));
    cx[0].rc.encode(enc_cum, enc_high, enc_tot);
    return 1;
  }

  void add_weight(int32_t want, uint32_t add) {
    int32_t recycled;
    SymEntry* list = ent;
    uint32_t n_live = live;
    SymEntry* p = list;
    uint32_t left = n_live;
    while( left&&p[0].sym!=want ) {
      ++p;
      --left;
    }
    if( left ) {
      p[0].cnt += add;
      since_rescale += add;
      rescale(move_up(p));
      return;
    }
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
    list[0].cnt = 2;
    list[0].sym = want;
    since_rescale += 4;
    if( list!=ent ) {
      uint16_t s3 = list[0].sym;
      uint8_t c3 = list[0].cnt;
      list[0] = list[-1];
      list[-1].set(s3, c3);
    }
  }

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

  inline int32_t decode_symbol_list(BMFState* cx) {
    SymEntry* list[no_symbol+1];
    SymEntry* q;
    SymEntry** w, ** rd, ** rd2;
    int32_t c;
    w = list;
    uint32_t gen = (uint8_t)cx[0].exclusion_gen;
    int32_t cum = 0;
    for( int32_t i = 0; i<(int32_t)live; ++i ) {
      if( (uint8_t)cx[0].exclusion_mask[ent[i].sym]==gen ) {
        c = 0;
      } else {
        SymEntry* e = &ent[i];
        c = e[0].cnt;
        *w++ = e;
      }
      cum += c;
    }
    if( !cum )
      return -1;
    w[0] = nullptr;
    int32_t tot_all = cum+(int32_t)tot;
    int32_t target = cx[0].rc.get_freq(tot_all);
    SymEntry* p = list[0];
    rd = &list[1];
    SymEntry* first = list[0];
    uint32_t cum_lo = 0;
    while( 1 ) {
      cum_lo += p[0].cnt;
      if( cum_lo>(uint32_t)target )
        break;
      p = *rd++;
      if( !p ) {
        int8_t gen_b = (int8_t)gen;
        q = first;
        rd2 = &list[1];
        do {
          cx[0].exclusion_mask[q[0].sym] = gen_b;
          q = *rd2++;
        } while( q );
        cx[0].rc.decode(cum_lo, tot_all, tot_all);
        return -1;
      }
    }
    int32_t result = p[0].sym;
    uint32_t sym_cum = cum_lo-p[0].cnt;
    this[0].rescale(this[0].promote(p));
    cx[0].rc.decode(sym_cum, cum_lo, tot_all);
    return result;
  }
};

void clear_sym_lists(SymList* lists) {
  for( int32_t k = 0; k<16; ++k )
    lists[k].ent = nullptr;
}

struct SymListBlock {
  uint32_t n;
  SymList list[0];
  static constexpr uint32_t bytes(uint32_t n) {
    return offsetof(SymListBlock, list)+sizeof(SymList)*n;
  }
};

SymList*new_sym_lists(uint32_t n, void* storage) {
  SymListBlock* block = (SymListBlock*)storage;
  block[0].n = n;
  for( uint32_t i = 0; i<n; ++i )
    block[0].list[i].ent = nullptr;
  return block[0].list;
}

void free_sym_entries(SymList*, uint32_t) {
}

void init_byte_lists(BMFState* cx, SymList* list, uint32_t nbytes) {
  for( uint32_t k = 0; k<4*nbytes; ++k )
    list[k].init(256, 1, &cx[0].byte_list_ent[k*256]);
}

void init_gap_list(BMFState* cx, SymList* list, uint32_t mask, uint32_t n_syms) {
  list[0].init(mask-n_syms+2, 1, cx[0].gap_list_ent);
  list[0].rescale_at = 19*list[0].n;
}

SymListBlock*sym_list_block(SymList* list) {
  return (SymListBlock*)((uint8_t*)list-offsetof(SymListBlock, list));
}

uint32_t sym_list_count(const SymList* list) {
  return sym_list_block((SymList*)list)[0].n;
}

bool sym_in_top(const SymEntry* ent, int32_t n, int32_t sym) {
  while( n-- ) if( ent[n].sym==sym ) return true;
  return false;
}
