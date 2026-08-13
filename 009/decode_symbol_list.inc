// decode_symbol_list.inc -- read back the alphabet an encoder wrote.

int32_t __decode_symbol_list(SymList* syms) {
  struct alignas(16) DecodeSymbolListFrame {
    union {
      SymEntry* list[8192];
      struct {
        uint32_t list0;
        BMF_SPILL_PAD(l0);
        SymEntry* list1;
        uint32_t list2;
        BMF_SPILL_PAD(l2);
        int32_t list3;
        BMF_SPILL_PAD(l3);
        int32_t list4;
        BMF_SPILL_PAD(l4);
        uint32_t list5;
        BMF_SPILL_PAD(l5);
        SymEntry* list6;
        int32_t list7;
        BMF_SPILL_PAD(l7);
        SymEntry* list_tail[8184];
      };
    };
    uint8_t _pad1[32];
  } __frame;
  int32_t tot, tot0;
  SymEntry* first;
  SymList* owner;
  uint32_t gen;
  SymEntry* ent, * e, * p, * q, * head, * q2, * r, * cur, * prev, * up, * back;
  SymEntry** w, ** rd, ** rd2;
  int8_t gen_b;
  uint32_t cum_lo, sym_cum, sym_high, cum_hi;
  int32_t tot_1, tot_all, target;
  uint8_t c_a, c_b;
  uint16_t s_a, s_b;
  int32_t live, cum, i, c, top, half, back_cnt, last_cnt;
  uint32_t n_left, zeros;
  uint32_t list5_s, rescale_at, limit20, running, since_rescale;
  live = syms->live;
  ent = syms->ent;
  w = __frame.list;
  gen = (uint8_t)exclusion_gen;
  owner = syms;
  cum = 0;
  i = 0;
  do {
    if( (uint8_t)exclusion_mask[ent[i].sym]==gen ) {
      c = 0;
    } else {
      e = &ent[i];
      c = e->cnt;
      *w++ = e;
    }
    cum += c;
    ++i;
  } while( i<live );
  if( !cum )
    return -1;
  *w = nullptr;
  tot0 = owner->tot;
  tot_all = cum+tot0;
  target = rc.get_freq(tot_all);
  tot = cum+tot0;
  p = (__frame.list[0]);
  rd = &__frame.list[1];
  first = (__frame.list[0]);
  cum_lo = 0;
  while( 1 ) {
    cum_lo += p->cnt;
    if( cum_lo>(uint32_t)(int32_t)target )
      break;
    p = *rd++;
    if( !p ) {
      gen_b = (int8_t)gen;
      sym_cum = cum_lo;
      cum_hi = tot;
      sym_high = tot;
      q = first;
      rd2 = &__frame.list[1];
      do {
        exclusion_mask[q->sym] = gen_b;
        q = *rd2++;
      } while( q );
      tot_1 = tot;
      __frame.list7 = -1;
      {
        rc.decode(cum_lo, cum_hi, tot_1);
        return __frame.list7;
      }
    }
  }
  sym_high = cum_lo;
  sym_cum = cum_lo-p->cnt;
  __frame.list7 = p->sym;
  p->cnt += 4;
  head = owner->ent;
  owner->since_rescale += 4;
  if( p==head ) {
    top = p->cnt;
  } else {
    c_a = p->cnt;
    s_a = p->sym;
    q2 = p-1;
    *p = *q2;
    q2->set(s_a, c_a);
    head = owner->ent;
    if( q2==head ) {
      top = q2->cnt;
    } else {
      __frame.list5 = list5_s;
      while( 1 ) {
        top = q2->cnt;
        r = q2-1;
        if( top<=r->cnt )
          break;
        s_b = q2->sym;
        c_b = q2->cnt;
        *q2 = *r;
        r->set(s_b, c_b);
        head = owner->ent;
        --q2;
        if( r==head ) {
          list5_s = __frame.list5;
          top = r->cnt;
          goto LABEL_30;
        }
      }
      list5_s = __frame.list5;
    }
  }
LABEL_30:
  rescale_at = owner->rescale_at;
  if( top>251||rescale_at<owner->since_rescale ) {
    __frame.list0 = owner->live;
    limit20 = 20*owner->n;
    n_left = __frame.list0;
    __frame.list5 = list5_s;
    __frame.list4 = rescale_at<limit20;
    cur = head-1;
    do {
      prev = cur;
      ++cur;
      half = (__frame.list4+(uint32_t)cur->cnt)>>1;
      cur->cnt = half;
      if( cur!=owner->ent ) {
        up = cur-1;
        __frame.list3 = up->cnt;
        if( half>__frame.list3 ) {
          __frame.list2 = cur->sym;
          cur->set(up->sym, __frame.list3);
          if( up!=owner->ent ) {
            (__frame.list[1]) = cur;
            __frame.list0 = n_left;
            do {
              back = up-1;
              back_cnt = back->cnt;
              if( half<=back_cnt )
                break;
              up->set(back->sym, back_cnt);
              --up;
            } while( back!=owner->ent );
            cur = (__frame.list[1]);
            n_left = __frame.list0;
          }
          up->set(__frame.list2, half);
        }
      }
      --n_left;
    } while( n_left );
    list5_s = __frame.list5;
    running = owner->tot;
    __frame.list0 = 0;
    if( !cur->cnt ) {
      zeros = __frame.list0;
      do {
        ++zeros;
        owner->tot = ++running;
        last_cnt = prev->cnt;
        --prev;
      } while( !last_cnt );
      __frame.list0 = zeros;
      owner->live -= zeros;
    }
    since_rescale = owner->since_rescale;
    owner->tot = running-(running>>1);
    cum_lo = sym_cum;
    cum_hi = sym_high;
    tot_1 = tot_all;
    owner->since_rescale = since_rescale-(since_rescale>>1);
  } else {
    tot_1 = tot_all;
    cum_lo = sym_cum;
    cum_hi = sym_high;
  }
  rc.decode(cum_lo, cum_hi, tot_1);
  return __frame.list7;
}
