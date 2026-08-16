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
    for( int32_t i = 0; i<5; ++i )
      w[i] -= w[i]>>1;
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
    tot = running-(running>>1);
    since_rescale = since-(since>>1);
    return true;
  }

  // Move `p` up past every entry it now outranks, and return the count it
  // settles at -- which is what decides whether the list rescales.
  //
  // This was written out twice, here and in `decode_symbol_list`, identical
  // down to the `goto` back into the `p == ent` arm that both used to leave
  // the inner loop.  Four returns say the same four exits and the label goes
  // with them.
  int32_t promote(SymEntry* p) {
    p->cnt += 4;
    since_rescale += 4;
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

  int32_t code_symbol(int32_t want) {
    int32_t enc_cum, enc_high, enc_tot;
    int32_t c, result, c2;
    uint32_t i;
    int8_t gen = exclusion_gen;
    uint32_t left = live;
    SymEntry *p = ent-1;
    int32_t cum = 0;
    while( 1 ) {
      ++p;
      int32_t s = p->sym;
      if( exclusion_mask[s]!=exclusion_gen ) {
        c = p->cnt;
        cum += c;
        if( s==want )
          break;
      }
      if( !--left ) {
        if( !cum )
          return 0;
        enc_cum = cum;
        enc_tot = tot+cum;
        enc_high = enc_tot;
        do {
          exclusion_mask[p->sym] = gen;
          --p;
        } while( p>=ent );
        result = 0;
        {
          rc.encode(enc_cum, enc_high, enc_tot);
          return result;
        }
      }
    }
    enc_high = cum;
    uint32_t rest = left-1;
    enc_cum = cum-c;
    if( rest ) {
      for( i = 0; i<rest; ++i ) {
        if( exclusion_mask[p[i+1].sym]==exclusion_gen )
          c2 = 0;
        else
          c2 = p[i+1].cnt;
        cum += c2;
      }
    }
    enc_tot = tot+cum;
    rescale(promote(p));
    result = 1;
    rc.encode(enc_cum, enc_high, enc_tot);
    return result;
  }

  void add_weight(int32_t want, uint32_t add) {
    uint32_t count;
    bool full;
    SymEntry *head;
    int32_t recycled;
    SymEntry* list = ent;
    uint32_t n_live = live;
    SymEntry *p = list;
    uint32_t left = n_live;
    if( n_live ) {
      while( p->sym!=want ) {
        ++p;
        if( !--left )
          goto rescaled;
      }
      p->cnt += add;
      since_rescale += add;
      head = ent;
      if( p==head ) {
  placed:
        count = p->cnt;
      } else {
        uint16_t s = p->sym;
        uint8_t c = p->cnt;
        SymEntry *q = p-1;
        *p = *q;
        q->set(s, c);
        head = ent;
        if( q==head ) {
          count = q->cnt;
        } else {
          while( 1 ) {
            count = q->cnt;
            p = q-1;
            if( count<=p->cnt )
              break;
            uint16_t s2 = q->sym;
            uint8_t c2 = q->cnt;
            *q = *p;
            p->set(s2, c2);
            head = ent;
            --q;
            if( p==head )
              goto placed;
          }
        }
      }
      rescale(count);
      return;
    } else {
  rescaled:
      full = n_live==n;
      if( n_live>=n ) {
        if( add<=1 )
          return;
        full = n_live==n;
      }
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
  }
  void init(int32_t n_syms, int32_t dense) {
    uint32_t result;
    const uint32_t n_u = (uint32_t)n_syms;
    n = n_syms;
    SymEntry* buf = (SymEntry*)bmf_new(3*n_syms);
    ent = buf;
    if( dense ) {
      tot = 0;
      live = n_u;
      result = 12*n_u;
      since_rescale = result;
      rescale_at = 8*n_u;
      if( n_u ) {
        result = 0;
        do {
          ent[result].sym = result;
          ent[result].cnt = 1;
          ++result;
        } while( result<live );
      }
    } else {
      tot = 2;
      rescale_at = 20*n_u;
      live = 0;
      since_rescale = 18*n_u;
      memset(buf, 0, 3*n_u);
    }
    return;
  }
};

struct SymListBlock {
  uint32_t n;
  SymList list[0];
  static uint32_t bytes(uint32_t n) {
    return __builtin_offsetof(SymListBlock, list)+sizeof(SymList)*n;
  }
};

static inline void free_sym_entries(SymList* list, uint32_t n) {
  while( n-- )
    free(list[n].ent);
}

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
