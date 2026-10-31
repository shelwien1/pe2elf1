//--- AAC raw_data_block parser: frame length from the bitstream ------------------
// ISO/IEC 14496-3 GA syntax (AAC Main/LC/SSR/LTP core; HE-AAC SBR/PS live in FIL
// elements and are skipped by count). Huffman tables are complete prefix codes, so
// zero-padded lookahead past the buffer end can't change a decoded codeword: the
// parse depends only on the frame's own bits.


static const uint16_t* const aac_swb1024[13] = { swb_1024_96,swb_1024_96,swb_1024_64,swb_1024_48,swb_1024_48,swb_1024_32,swb_1024_24,swb_1024_24,swb_1024_16,swb_1024_16,swb_1024_16,swb_1024_8,swb_1024_8 };
static const uint16_t* const aac_swb960[13]  = { swb_960_96,swb_960_96,swb_960_64,swb_960_48,swb_960_48,swb_960_48,swb_960_24,swb_960_24,swb_960_16,swb_960_16,swb_960_16,swb_960_8,swb_960_8 };
static const uint16_t* const aac_swb128[13]  = { swb_128_96,swb_128_96,swb_128_96,swb_128_48,swb_128_48,swb_128_48,swb_128_24,swb_128_24,swb_128_16,swb_128_16,swb_128_16,swb_128_8,swb_128_8 };
static const uint16_t* const aac_swb120[13]  = { swb_120_96,swb_120_96,swb_120_96,swb_120_48,swb_120_48,swb_120_48,swb_120_24,swb_120_24,swb_120_16,swb_120_16,swb_120_16,swb_120_8,swb_120_8 };

struct AacBits {
  const byte* p; qword nb, pos; bool err;
  uint peek( int n ) const {                 // n<=25, zero-padded past the end
    qword b = pos>>3, ne = nb>>3;
    if( b+8<=ne ) {
      const byte* q = p+b;
      qword w = (qword(q[0])<<56)|(qword(q[1])<<48)|(qword(q[2])<<40)|(qword(q[3])<<32)|
                (qword(q[4])<<24)|(qword(q[5])<<16)|(qword(q[6])<<8)|qword(q[7]);
      return uint( (w<<(pos&7))>>(64-n) );
    }
    uint w=0;
    for( int i=0; i<4; i++ ) w = (w<<8) | (b+i<ne ? uint(p[b+i]) : 0u);
    return (w<<(pos&7))>>(32-n);
  }
  uint get( int n ) {
    if( n==0 ) return 0;
    if( qword(n)>nb-pos ) { err=true; pos=nb; return 0; }
    uint v = peek(n); pos+=n; return v;
  }
  void skip( qword n ) { if( n>nb-pos ) err=true, pos=nb; else pos+=n; }
  void align() { skip( (8-(pos&7))&7 ); }
};

struct AacHuff {
  int maxlen, l2; std::vector<uint32_t> t;
  void Build( const uint32_t* code, const byte* bits, int n ) {
    maxlen=0; for( int i=0; i<n; i++ ) maxlen = std::max( maxlen, int(bits[i]) );
    int l1 = std::min( maxlen, 10 ); l2 = maxlen-l1;
    t.assign( size_t(1)<<l1, 0 );
    for( int i=0; i<n; i++ ) {
      uint c = code[i], l = bits[i], e = (l<<16)|uint(i);
      if( int(l)<=l1 ) {
        for( uint j=0; j<(1u<<(l1-l)); j++ ) t[(c<<(l1-l))+j] = e;
      } else {
        uint pre = c>>(l-l1);
        if( !(t[pre]&0x80000000u) ) { t[pre] = 0x80000000u|uint(t.size()); t.resize( t.size()+(size_t(1)<<l2), 0 ); }
        uint base = (t[pre]&0x7FFFFFFF) + ((c&((1u<<(l-l1))-1))<<(maxlen-l));
        for( uint j=0; j<(1u<<(maxlen-l)); j++ ) t[base+j] = e;
      }
    }
  }
  int Dec( AacBits& b ) const {
    uint v = b.peek(maxlen), e = t[v>>l2];
    if( e&0x80000000u ) e = t[(e&0x7FFFFFFF)+(v&((1u<<l2)-1))];
    uint l = e>>16;
    if( l==0 ) { b.err=true; return 0; }
    b.skip(l);
    return int(e&0xFFFF);
  }
};

static AacHuff aac_hsf, aac_hspec[11];
static byte aac_extra[11][289];             // per codeword: sign bit count | escape count<<4

static void AacInit() {
  static bool done=false;
  if( done ) return;
  done=true;
  static const int size[11] = { 81,81,81,81,81,81,64,64,169,169,289 };
  std::vector<uint32_t> c;
  c.assign( aac_sf_code, aac_sf_code+121 ); aac_hsf.Build( c.data(), aac_sf_bits, 121 );
  int o=0;
  for( int k=0; k<11; k++ ) {
    c.assign( aac_spec_code+o, aac_spec_code+o+size[k] );
    aac_hspec[k].Build( c.data(), aac_spec_bits+o, size[k] );
    o += size[k];
    int cb=k+1, mod = cb<=4 ? 3 : cb<=6 ? 9 : cb<=8 ? 8 : cb<=10 ? 13 : 17;
    bool uns = cb==3||cb==4||cb>=7;
    for( int s=0; s<size[k]; s++ ) {
      int nnz=0, nesc=0;
      if( uns ) {
        int v[4], nv = cb<=4 ? 4 : 2, x=s;
        for( int i=nv; i-- >0; ) v[i] = x%mod, x/=mod;
        for( int i=0; i<nv; i++ ) nnz += v[i]!=0, nesc += cb==11 && v[i]==16;
      }
      aac_extra[k][s] = byte( nnz | nesc<<4 );
    }
  }
}

struct AacIcs { int ws, max_sfb, ng, nwin, num_swb, glen[8]; bool pred; const uint16_t* swb; };

struct AacCtx {
  AacBits b; int aot, sfi, fl;

  void Ltp( int max_sfb ) { b.get(11); b.get(3); for( int i=0; i<std::min(max_sfb,40); i++ ) b.get(1); }

  bool IcsInfo( AacIcs& s ) {
    b.get(1); s.ws = b.get(2); b.get(1);
    s.ng = 1; s.glen[0] = 1; s.pred = false;
    if( s.ws==2 ) {
      s.max_sfb = b.get(4);
      for( int i=0; i<7; i++ ) if( b.get(1) ) s.glen[s.ng-1]++; else s.glen[s.ng++] = 1;
      s.nwin = 8; s.swb = fl ? aac_swb120[sfi] : aac_swb128[sfi]; s.num_swb = aac_num_swb_128[sfi];
    } else {
      s.max_sfb = b.get(6);
      s.nwin = 1; s.swb = fl ? aac_swb960[sfi] : aac_swb1024[sfi]; s.num_swb = fl ? aac_num_swb_960[sfi] : aac_num_swb_1024[sfi];
      if( (s.pred = b.get(1)!=0) ) {
        if( aot==1 ) {
          if( b.get(1) ) b.get(5);
          for( int i=0; i<std::min(s.max_sfb,int(aac_pred_sfb_max[sfi])); i++ ) b.get(1);
        } else if( aot==4 ) {
          if( b.get(1) ) Ltp( s.max_sfb );
        } else return false;                // no prediction in LC/SSR
      }
    }
    return !b.err && s.max_sfb<=s.num_swb;
  }

  // individual_channel_stream; cb receives the section codebooks
  bool Ics( AacIcs& s, byte cb[8][64], bool common ) {
    b.get(8);                               // global_gain
    if( !common && !IcsInfo(s) ) return false;
    // section_data
    int sb = s.ws==2 ? 3 : 5, esc = (1<<sb)-1;
    for( int g=0; g<s.ng; g++ ) {
      int k=0;
      while( k<s.max_sfb ) {
        int c = b.get(4), len=0, inc;
        if( c==12 ) return false;
        do { inc = b.get(sb); len += inc; if( b.err ) return false; } while( inc==esc );
        if( k+len>s.max_sfb ) return false;
        for( int i=0; i<len; i++ ) cb[g][k++] = byte(c);
      }
    }
    // scale_factor_data
    bool noise1 = true;
    for( int g=0; g<s.ng; g++ )
      for( int i=0; i<s.max_sfb; i++ ) {
        int c = cb[g][i];
        if( c==0 ) continue;
        if( c==13 && noise1 ) { b.get(9); noise1=false; }
        else aac_hsf.Dec(b);
      }
    if( b.err ) return false;
    // pulse_data
    if( b.get(1) ) {
      if( s.ws==2 ) return false;
      int np = b.get(2)+1; b.get(6);
      for( int i=0; i<np; i++ ) b.get(5), b.get(4);
    }
    // tns_data
    if( b.get(1) ) {
      bool sh = s.ws==2;
      for( int w=0; w<s.nwin; w++ ) {
        int nf = b.get(sh?1:2);
        if( !nf ) continue;
        int res = b.get(1);
        for( int f=0; f<nf; f++ ) {
          b.get(sh?4:6);
          int ord = b.get(sh?3:5);
          if( ord ) {
            b.get(1); int cl = res+3-int(b.get(1));
            for( int i=0; i<ord; i++ ) b.get(cl);
          }
        }
      }
    }
    // gain_control_data (SSR)
    if( b.get(1) ) {
      static const byte gm[4][3] = { {1,0,5}, {2,1,2}, {8,0,2}, {2,1,5} };
      int mb = b.get(2);
      for( int bd=0; bd<mb; bd++ )
        for( int wd=0; wd<gm[s.ws][0]; wd++ ) {
          int an = b.get(3);
          for( int ad=0; ad<an; ad++ ) b.get( 4 + ((wd==0 && gm[s.ws][1]) ? 4 : gm[s.ws][2]) );
        }
    }
    if( b.err ) return false;
    // spectral_data
    for( int g=0; g<s.ng; g++ )
      for( int i=0; i<s.max_sfb; i++ ) {
        int c = cb[g][i];
        if( c==0 || c>11 ) continue;
        const AacHuff& h = aac_hspec[c-1]; const byte* ex = aac_extra[c-1];
        int n = s.glen[g]*(s.swb[i+1]-s.swb[i]) >> (c<5 ? 2 : 1);
        for( int j=0; j<n; j++ ) {
          int x = ex[h.Dec(b)];
          b.skip( x&15 );
          for( int e=x>>4; e>0; e-- ) {     // escape: N ones, a zero, N+4 bits
            int N=0;
            while( b.get(1) ) if( ++N>8 || b.err ) return false;
            b.get(N+4);
          }
        }
        if( b.err ) return false;
      }
    return !b.err;
  }

  bool Cpe() {
    AacIcs s0, s1; byte cb0[8][64], cb1[8][64];
    bool common = b.get(1)!=0;
    if( common ) {
      if( !IcsInfo(s0) ) return false;
      if( s0.pred && aot==4 && b.get(1) ) Ltp( s0.max_sfb );
      int ms = b.get(2);
      if( ms==3 ) return false;
      if( ms==1 ) b.skip( qword(s0.ng)*s0.max_sfb );
      s1 = s0;
    }
    return Ics( s0, cb0, common ) && Ics( s1, cb1, common );
  }

  bool Cce() {
    int ind = b.get(1), nc = b.get(3), ng = 0;
    for( int c=0; c<=nc; c++ ) {
      ng++;
      if( b.get(1) ) { b.get(4); if( b.get(2)==3 ) ng++; } else b.get(4);
    }
    b.get(1); b.get(1); b.get(2);
    AacIcs s; byte cb[8][64];
    if( !Ics( s, cb, false ) ) return false;
    for( int c=1; c<ng; c++ ) {
      int cge = ind ? 1 : b.get(1);
      if( cge ) aac_hsf.Dec(b);
      else for( int g=0; g<s.ng; g++ ) for( int i=0; i<s.max_sfb; i++ ) if( cb[g][i] ) aac_hsf.Dec(b);
    }
    return !b.err;
  }

  void Pce() {
    b.get(2); b.get(4);
    int nf=b.get(4), ns=b.get(4), nb=b.get(4), nl=b.get(2), na=b.get(3), nc=b.get(4);
    if( b.get(1) ) b.get(4);
    if( b.get(1) ) b.get(4);
    if( b.get(1) ) b.get(3);
    b.skip( 5*(nf+ns+nb) + 4*(nl+na) + 5*nc );
    b.align();
    b.skip( 8*qword(b.get(8)) );
  }
};

// byte length of one raw_data_block, 0 on failure.
// param: bits 0-1 = AOT-1 (Main/LC/SSR/LTP), 2-5 = sampling index, 6 = 960-sample frames
static qword AacFrameLen( int param, const byte* p, qword avail ) {
  AacInit();
  AacCtx x; x.b.p=p; x.b.nb=avail*8; x.b.pos=0; x.b.err=false;
  x.aot = (param&3)+1; x.sfi = (param>>2)&15; x.fl = (param>>6)&1;
  if( x.sfi>12 || avail>(qword(1)<<40) ) return 0;
  for( int ne=0; ne<4096; ne++ ) {
    int id = x.b.get(3);
    if( x.b.err ) return 0;
    if( id==7 ) return (x.b.pos+7)>>3;      // ID_END, byte aligned
    int tag = x.b.get(4);
    bool ok = true;
    switch( id ) {
    case 0: case 3: { AacIcs s; byte cb[8][64]; ok = x.Ics( s, cb, false ); } break;   // SCE, LFE
    case 1: ok = x.Cpe(); break;
    case 2: ok = x.Cce(); break;
    case 4: {                                                                       // DSE
      int al = x.b.get(1), cnt = x.b.get(8);
      if( cnt==255 ) cnt += x.b.get(8);
      if( al ) x.b.align();
      x.b.skip( 8*qword(cnt) );
    } break;
    case 5: x.Pce(); break;
    case 6: {                                                                       // FIL
      int cnt = tag;
      if( cnt==15 ) cnt += int(x.b.get(8))-1;
      x.b.skip( 8*qword(cnt) );
    } break;
    }
    if( !ok || x.b.err ) return 0;
  }
  return 0;
}

// AudioSpecificConfig -> AacFrameLen param, or -1 if not a supported GA AAC config
static int AacParamFromAsc( const byte* a, qword n ) {
  AacBits b; b.p=a; b.nb=n*8; b.pos=0; b.err=false;
  int aot = b.get(5); if( aot==31 ) aot = 32+b.get(6);
  int sfi = b.get(4);
  if( sfi==15 ) {
    static const uint thr[12] = { 92017,75132,55426,46009,37566,27713,23004,18783,13856,11502,9391,0 };
    uint f = b.get(24); sfi=0; while( f<thr[sfi] ) sfi++;
  }
  b.get(4);                                 // channel configuration
  if( aot==5 || aot==29 ) {                 // explicit SBR/PS: core follows
    if( b.get(4)==15 ) b.get(24);
    aot = b.get(5); if( aot==31 ) aot = 32+b.get(6);
    if( aot==22 ) b.get(4);
  }
  int fl = b.get(1);
  if( b.err || aot<1 || aot>4 || sfi>12 ) return -1;
  return (aot-1) | sfi<<2 | fl<<6;
}
