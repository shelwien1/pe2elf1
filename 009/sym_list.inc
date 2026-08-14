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
    SymEntry *prev, *back;
    if( count<=251&&due>=since_rescale )
      return false;
    n_left = live;
    int32_t bias = due<20*n;
    SymEntry *cur = ent-1;
    prev = cur;
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

  int32_t code_symbol(int32_t want) {
    int32_t enc_cum, enc_high, enc_tot;
    uint32_t left;
    uint8_t c_a, c_b;
    SymEntry* p, * head, * q;
    int32_t cum, c, result, c2, top;
    uint16_t s_a, s_b;
    uint32_t rest, i;
    int8_t gen = exclusion_gen;
    left = live;
    p = ent-1;
    cum = 0;
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
    rest = left-1;
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
    p->cnt += 4;
    head = ent;
    since_rescale += 4;
    if( p==head ) {
  sorted:
      top = p->cnt;
    } else {
      s_a = p->sym;
      c_a = p->cnt;
      q = p-1;
      *p = *q;
      q->set(s_a, c_a);
      head = ent;
      if( q==head ) {
        top = q->cnt;
      } else {
        while( 1 ) {
          top = q->cnt;
          p = q-1;
          if( top<=p->cnt )
            break;
          s_b = q->sym;
          c_b = q->cnt;
          *q = *p;
          p->set(s_b, c_b);
          head = ent;
          --q;
          if( p==head )
            goto sorted;
        }
      }
    }
    rescale(top);
    result = 1;
    rc.encode(enc_cum, enc_high, enc_tot);
    return result;
  }

  void add_weight(int32_t want, uint32_t add) {
    uint32_t count;
    bool full;
    uint8_t c3, c, c2;
    SymEntry* q, * p, * head;
    uint16_t s3, s, s2;
    int32_t recycled;
    uint32_t n_live, left;
    SymEntry* list = ent;
    n_live = live;
    p = list;
    left = n_live;
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
        s = p->sym;
        c = p->cnt;
        q = p-1;
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
            s2 = q->sym;
            c2 = q->cnt;
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
        s3 = list->sym;
        c3 = list->cnt;
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
      since_rescale = 12*n_u;
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
