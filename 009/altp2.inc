// altp2.inc -- the alternate model for deep planes: its counters, its frequency
// records and the block they live in.

struct P2Count {
  int8_t rate;
  uint8_t b1;
  int16_t weighted;
};

struct P2Freq {
  uint16_t step;
  uint16_t f[3];
  uint32_t rescale_three_way() {
    uint32_t cut, budget;
    this->f[0] -= this->f[0]>>1;
    this->f[1] -= this->f[1]>>1;
    cut = this->f[2]>>1;
    this->f[2] -= cut;
    budget = this->step;
    if( budget<=0x100 ) {
      if( this->step<=0x20u )
        cut = ((16-budget)>>30)&0xFFFFFFFE;
      else
        cut = 32;
      this->step = budget-cut;
    } else {
      this->step >>= 1;
    }
    return cut;
  }

  void encode_symbol(const uint32_t* ctx_pair, int32_t sym) {
    uint16_t f_before;
    uint32_t cum;
    uint16_t st, * slot;
    uint32_t tot, tot_1, f1_old, f2_old;
    uint16_t down;
    cum = this->f[1]+this->f[0];
    tot = cum+this->f[2];
    if( sym ) {
      if( (sym&1)!=0 ) {
        cum = this->f[0];
        slot = &this->f[1];
      } else {
        slot = &this->f[2];
      }
    } else {
      cum = 0;
      slot = &this->f[0];
    }
    tot_1 = cum+*slot;
    rc.encode(cum, tot_1, tot);
    f_before = *slot;
    if( *slot>0x4000u ) {
      f1_old = this->f[1];
      f2_old = this->f[2];
      this->f[0] -= this->f[0]>>1;
      st = this->step;
      this->f[1] = f1_old-(f1_old>>1);
      this->f[2] = f2_old-(f2_old>>1);
      if( st<=256 ) {
        if( st<=32 )
          down = ((uint32_t)(16-st)>>30)&0xFFFFFFFE;
        else
          down = 32;
        st = st-down;
        this->step = st;
        f_before = *slot;
      } else {
        st = (uint32_t)st>>1;
        this->step = st;
        f_before = *slot;
      }
    } else {
      st = this->step;
    }
    *slot = st+f_before;
    if( sym>0 )
      return __encode_symbol_tree(model_strip(ctx_pair[sym&1]), (sym-1)>>1);
  }

  int32_t decode_symbol(const uint32_t* ctx_pair) {
    uint32_t cum;
    int32_t target, idx, c01;
    uint16_t fq, st;
    uint16_t* slot, * base;
    uint32_t f1_old, f2_old, tot;
    c01 = this->f[1]+this->f[0];
    tot = this->f[2]+c01;
    target = rc.get_freq(tot);
    cum = this->f[0];
    if( (uint32_t)target>=cum ) {
      if( target>=c01 ) {
        cum = c01;
        slot = &this->f[2];
      } else {
        slot = &this->f[1];
      }
      base = &this->f[0];
    } else {
      slot = &this->f[0];
      cum = 0;
      base = &this->f[0];
    }
    rc.decode(cum, (cum+((uint16_t)*slot)), tot);
    fq = (uint16_t)*slot;
    if( fq>0x4000 ) {
      f1_old = this->f[1];
      f2_old = this->f[2];
      this->f[0] -= this->f[0]>>1;
      st = this->step;
      this->f[1] = f1_old-(f1_old>>1);
      this->f[2] = f2_old-(f2_old>>1);
      if( st<=256 ) {
        if( st<=32 )
          fq = ((uint32_t)(16-st)>>30)&0xFFFFFFFE;
        else
          fq = 32;
        st = st-fq;
        this->step = st;
        fq = *slot;
      } else {
        st = (uint32_t)st>>1;
        this->step = st;
        fq = *slot;
      }
    } else {
      st = this->step;
    }
    *slot = st+fq;
    idx = slot-base;
    if( idx )
      return idx+2*__decode_symbol_tree(model_strip(ctx_pair[idx&1]));
    else
      return 0;
  }
};

static inline int32_t p2_pred(int32_t weighted, int32_t rate) {
  return (weighted+(1<<((rate+31)&31)))>>(rate&31);
}

static inline int16_t p2_bump(int32_t w2, int32_t err, int32_t shift) {
  const uint32_t kick = 32u*(uint32_t)((err>deadzone_hi)-(err<deadzone_lo));
  return (int16_t)((uint32_t)w2+((kick+(uint32_t)err+(1u<<(shift-1)))>>shift));
}

struct P2Ctx {
  int16_t val;
  int16_t dval;
  int16_t err;
  int16_t aerr;
  int16_t dleft;
  int16_t dup;
  int16_t dupleft;
  int16_t dupright;
  uint8_t sign;
  uint8_t mag;
};

struct AltP2Block {
  float nb_weights[1088][16][4];
  union {
    float p2_row[7][4];
    struct {
      uint8_t p2_row0_head[15];
      uint8_t _u0_0_4[97];
      float bias[4];
      float (*nb_cur)[4];
      float (**row0)[4];
      float (**row1)[4];
      float (**cur)[4];
      float (**above)[4];
      int32_t bank_ctx[5];
      int32_t _unused278696;
      uint16_t pred_prev;
      uint8_t _u0_0_12[2];
      uint32_t ctx;
      uint32_t ctx_pair[2];
      int32_t nb_id_used;
      int32_t band_lo;
      int32_t band_hi;
      uint32_t plane_idx;
      int32_t has_ref;
      P2Ctx* cursor[5];
      P2Ctx* buf[5];
      CtxWeight ctx_w[5];
      uint8_t _u0_0_tail[8];
    };
  };
  uint8_t _pad1[40];
  int32_t nb_sum[10];
  int32_t ctx_delta[260];
  uint8_t fold[256];
  uint8_t fold_hi[256];
  int8_t unfold[256];
  uint8_t nb_ctx[120];
  int16_t nb_id[1916];
  uint8_t _pad2[8];
  P2Count p2_ctr[163840];
  P2Freq freq[15552];
  uint8_t _pad3[8];
  AltP2Block*alt_p2_alloc(int32_t img_w, int32_t plane) {
    uint32_t done, e;
    void* buf1;
    int32_t dz, band, row_bytes, lvl, lvl1, bump, len, len1;
    uint32_t j, k, ctr, pairs, p, row, pair, n;
    this->plane_idx = plane;
    for( j = 0; j<0x14000; ++j ) {
      this->p2_ctr[2*j].weighted = 0;
      this->p2_ctr[2*j+1].weighted = 0;
    }
    for( k = 0; k<0x14000; ++k ) {
      this->p2_ctr[2*k].rate = 5;
      this->p2_ctr[2*k].b1 = 2;
      this->p2_ctr[2*k+1].rate = 5;
      this->p2_ctr[2*k+1].b1 = 2;
    }
    ctr = 0;
    do {
      e = 2*ctr;
      this->freq[e].f[0] = 2048;
      ++ctr;
      this->freq[e].f[1] = 2816;
      this->freq[e].f[2] = 2816;
      this->freq[e].step = 4096;
      this->freq[e+1].f[0] = 2048;
      this->freq[e+1].f[1] = 2816;
      this->freq[e+1].f[2] = 2816;
      this->freq[e+1].step = 4096;
    } while( ctr<0x1E60 );
    dz = 4*near_lossless_q+1;
    band = 16*near_lossless_q;
    this->has_ref = (uint8_t)(plane_desc[plane_desc[this->plane_idx+1].src_plane+1].flags&8)>>3;
    deadzone_hi = dz;
    deadzone_lo = -dz;
    this->band_lo = -band-7;
    this->band_hi = band+8;
    this->row0 = (float (**)[4])bmf_new(sizeof(float (*)[4])*(img_w+4));
    buf1 = bmf_new(sizeof(float (*)[4])*(img_w+4));
    this->nb_weights[0][14][2] = 1.0f;
    this->row1 = (float (**)[4]) buf1;
    this->cur = this->row0+img_w+2;
    if( img_w>-4 ) {
      pairs = (img_w+4)/2;
      if( pairs ) {
        for( p = 0; p<pairs; ++p ) {
          this->row1[2*p] = this->nb_weights[0];
          this->row0[2*p] = this->nb_weights[0];
          this->row1[2*p+1] = this->nb_weights[0];
          this->row0[2*p+1] = this->nb_weights[0];
        }
        done = 2*p+1;
      } else {
        done = 1;
      }
      if( (uint32_t)(img_w+4)>(done-1) ) {
        this->row1[done-1] = this->nb_weights[0];
        this->row0[done-1] = this->nb_weights[0];
      }
    }
    row = 0;
    row_bytes = 18*(img_w<3 ? 3 : img_w)+234;
    do
      this->buf[row++] = (P2Ctx*)bmf_new(row_bytes);
    while( row<5 );
    memset(this->buf[0], 0, row_bytes);
    ctx_bias[3] = 0;
    lvl = 0;
    ctx_bias[2] = 0;
    ctx_bias[1] = 0;
    pair = 0;
    ctx_bias[0] = 0;
    this->cursor[0] = this->buf[0]+8;
    do {
      this->ctx_delta[2*pair] = (this->plane_idx<<8)|(16*lvl);
      lvl1 = (2*pair==p2_ctx_edges[lvl])+lvl;
      this->ctx_delta[2*pair+1] = (this->plane_idx<<8)|(16*lvl1);
      bump = 2*pair+++1==p2_ctx_edges[lvl1];
      lvl = bump+lvl1;
    } while( pair<0x82 );
    len = 0;
    for( n = 0; n<0x3C; ++n ) {
      this->nb_ctx[2*n] = len;
      len1 = (2*n==p2_len_edges[len])+len;
      this->nb_ctx[2*n+1] = len1;
      len = (2*n+1==p2_len_edges[len1])+len1;
    }
    this->ctx = 15;
    __alt_init_tables(this->fold, this->unfold);
    this->ctx_w[4].w[0] = 0;
    this->ctx_w[0].w[1] = 64;
    this->ctx_w[3].w[0] = 0;
    this->ctx_w[0].w[2] = 128;
    this->ctx_w[2].w[0] = 0;
    this->ctx_w[1].w[1] = 192;
    this->ctx_w[1].w[0] = 0;
    this->ctx_w[1].w[2] = 384;
    this->ctx_w[0].w[0] = 0;
    this->ctx_w[2].w[1] = 576;
    this->ctx_w[2].w[2] = 1152;
    this->ctx_w[3].w[1] = 1728;
    this->ctx_w[3].w[2] = 3456;
    this->ctx_w[4].w[1] = 5184;
    this->ctx_w[4].w[2] = 10368;
    return this;
  }

  AltP2Block*alt_p2_free(int8_t do_free) {
    for( int32_t k = 0; k<5; ++k )
      free(this->buf[k]);
    free(this->row0);
    free(this->row1);
    if( (do_free&1)!=0 )
      bmf_page_free(this);
    return this;
  }
};

struct CtxWeights {
  float (*f0[6])[4];
};
