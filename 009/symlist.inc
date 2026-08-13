// symlist.inc -- the symbol list: the alphabet, its weights, and the search that
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
    int32_t half, bias, up_cnt, back_cnt, last_cnt;
    uint32_t n_left, running, since, due = rescale_at;
    uint16_t keep;
    SymEntry* cur, * prev, * up, * back;
    if( count<=251&&due>=since_rescale )
      return false;
    n_left = live;
    bias = due<20*n;
    cur = ent-1;
    prev = cur;
    do {
      prev = cur;
      ++cur;
      half = (bias+(uint32_t)cur->cnt)>>1;
      cur->cnt = half;
      if( cur!=ent ) {
        up = cur-1;
        up_cnt = up->cnt;
        if( half>up_cnt ) {
          keep = cur->sym;
          cur->set(up->sym, up_cnt);
          if( up!=ent ) {
            do {
              back = up-1;
              back_cnt = back->cnt;
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
    int8_t gen;
    int32_t enc_cum, enc_high, enc_tot;
    uint32_t left;
    uint8_t c_a, c_b;
    SymEntry* p, * head, * q;
    int32_t cum, s, c, result, c2, top;
    uint16_t s_a, s_b;
    uint32_t rest, i;
    gen = exclusion_gen;
    left = this->live;
    p = this->ent-1;
    cum = 0;
    while( 1 ) {
      ++p;
      s = p->sym;
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
        enc_tot = this->tot+cum;
        enc_high = enc_tot;
        do {
          exclusion_mask[p->sym] = gen;
          --p;
        } while( p>=this->ent );
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
    enc_tot = this->tot+cum;
    p->cnt += 4;
    head = this->ent;
    this->since_rescale += 4;
    if( p==head ) {
  LABEL_37:
      top = p->cnt;
    } else {
      s_a = p->sym;
      c_a = p->cnt;
      q = p-1;
      *p = *q;
      q->set(s_a, c_a);
      head = this->ent;
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
          head = this->ent;
          --q;
          if( p==head )
            goto LABEL_37;
        }
      }
    }
    this->rescale(top);
    result = 1;
    rc.encode(enc_cum, enc_high, enc_tot);
    return result;
  }

  void add_weight(int32_t want, uint32_t add) {
    SymEntry* list;
    uint32_t count;
    bool full;
    uint8_t c3, c, c2;
    SymEntry* q, * p, * head;
    uint16_t s3, s, s2;
    int32_t recycled;
    uint32_t n_live, left;
    list = this->ent;
    n_live = this->live;
    p = list;
    left = n_live;
    if( n_live ) {
      while( p->sym!=want ) {
        ++p;
        if( !--left )
          goto LABEL_4;
      }
      p->cnt += add;
      this->since_rescale += add;
      head = this->ent;
      if( p==head ) {
  LABEL_16:
        count = p->cnt;
      } else {
        s = p->sym;
        c = p->cnt;
        q = p-1;
        *p = *q;
        q->set(s, c);
        head = this->ent;
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
            head = this->ent;
            --q;
            if( p==head )
              goto LABEL_16;
          }
        }
      }
      this->rescale(count);
      return;
    } else {
  LABEL_4:
      full = n_live==this->n;
      if( n_live>=this->n ) {
        if( add<=1 )
          return;
        full = n_live==this->n;
      }
      if( full ) {
        this->live = --n_live;
        recycled = list[n_live].cnt;
      } else {
        recycled = 1;
      }
      list += n_live;
      this->live = n_live+1;
      this->tot = recycled+this->tot+1;
      list->cnt = 2;
      list->sym = want;
      this->since_rescale += 4;
      if( list!=this->ent ) {
        s3 = list->sym;
        c3 = list->cnt;
        *list = list[-1];
        list[-1].set(s3, c3);
      }
    }
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

void __init_symbol_list(SymList* list, int32_t n_syms, int32_t dense) {
  SymEntry* buf;
  uint32_t result;
  const uint32_t n = (uint32_t)n_syms;
  list->n = n_syms;
  buf = (SymEntry*)bmf_new(3*n_syms);
  list->ent = buf;
  if( dense ) {
    list->tot = 0;
    list->live = n;
    result = 12*n;
    list->since_rescale = 12*n;
    list->rescale_at = 8*n;
    if( n ) {
      result = 0;
      do {
        list->ent[result].sym = result;
        list->ent[result].cnt = 1;
        ++result;
      } while( result<list->live );
    }
  } else {
    list->tot = 2;
    list->rescale_at = 20*n;
    list->live = 0;
    list->since_rescale = 18*n;
    memset(buf, 0, 3*n);
  }
  return;
}
