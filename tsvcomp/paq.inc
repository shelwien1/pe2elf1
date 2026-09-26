// -------------------------------------------------------------
// paq.inc -- paq-style block of tsvcomp: hashed context models and a
// 2-layer mixer (knobs: IDX/tc_model-P0.idx).
//
//   HashCM   one context model: a hash table of slots {check, p, n}; the
//            slot of (context hash, bit node) predicts p = P(bit==1) and
//            adapts at rate 1/(n+1.5), n <= LIM (paq's StateMap without
//            the bit-history states).  A slot whose check does not match
//            is "new": it predicts nothing (inputs 0) and is taken over
//            on update.
//   PaqMixer inputs: per context model st(p) and st(p)*n/(n+2) (a
//            confidence-weighted copy), plus extra logits (the C0/C1/C2
//            counters) and a bias; NSEL weight sets, each selected by
//            its own small context, give NSEL logits; layer 2 mixes those
//            with one weight set per layer-2 context.  Both layers learn
//            online from their own error (y - p) * x, as in paq's Mixer.
// Everything is float; the output is the logit of P(bit==1).
// -------------------------------------------------------------

#include <vector>

namespace paq {

static inline float stf( float p ) { return logf( p / (1.0f - p) ); }
static inline float sqf( float x ) { return 1.0f / (1.0f + expf( -x )); }

struct Slot { uint32_t chk; float p; uint32_t n; };

struct HashCM {
  std::vector<Slot> t;
  qword mask = 0;
  void init( int bits ) { t.assign( size_t(1) << bits, Slot{ 0, 0.5f, 0 } ); mask = (qword(1) << bits) - 1; }
};

template<int NCX, int NEXT, int NSEL> struct Block {
  static const int NIN = 2*NCX + NEXT + 1;
  HashCM cm[NCX];
  std::vector<float> w1;   // [NSEL][sel size][NIN]
  std::vector<float> w2;   // [sel2 size][NSEL+1]
  int selsz[NSEL], seloff[NSEL], sel2sz;
  float lr1, lr2, w0; uint lim;

  // per-bit state between predict() and update()
  Slot* s[NCX]; uint32_t chk[NCX]; bool hit[NCX];
  float x[NIN], z1[NSEL], p1[NSEL], x2[NSEL+1];
  int sel[NSEL], sel2; float pz;

  void init( const int* sizes, int s2size ) {
    int tb = P0_TB < 16 ? 16 : P0_TB > 24 ? 24 : P0_TB;
    for( int i = 0; i < NCX; i++ ) cm[i].init( tb );
    int off = 0;
    for( int j = 0; j < NSEL; j++ ) { selsz[j] = sizes[j]; seloff[j] = off; off += sizes[j]; }
    lr1 = float(P0_LR1) / 65536; lr2 = float(P0_LR2) / 65536;
    w0 = float(P0_W0) / 65536;
    lim = P0_LIM < 1 ? 1 : P0_LIM > 1023 ? 1023 : P0_LIM;
    w1.assign( size_t(off) * NIN, w0 );
    sel2sz = s2size;
    w2.assign( size_t(s2size) * (NSEL+1), 1.0f / NSEL );
    for( int i = 0; i < s2size; i++ ) w2[ size_t(i)*(NSEL+1) + NSEL ] = 0;   // bias weight
  }

  // h[i] = context hash of model i for this bit; ext = extra input logits
  // (of P(bit==1)); sl = layer-1 selector values; s2 = layer-2 selector.
  // Returns the logit of P(bit==1).
  float predict( const qword* h, const float* ext, const int* sl, int s2 ) {
    for( int i = 0; i < NCX; i++ ) {
      Slot& e = cm[i].t[ h[i] & cm[i].mask ];
      s[i] = &e; chk[i] = uint32_t( h[i] >> 32 ) | 1;
      hit[i] = e.chk == chk[i];
      if( hit[i] ) {
        float st = stf( clamp( e.p, 1.0f/4096, 1.0f-1.0f/4096 ) );
        x[2*i] = st; x[2*i+1] = st * float(e.n) / float(e.n + 2);
      } else x[2*i] = x[2*i+1] = 0;
    }
    for( int i = 0; i < NEXT; i++ ) x[2*NCX + i] = clip( ext[i], 8.0f );
    x[NIN-1] = 0.25f;
    for( int j = 0; j < NSEL; j++ ) {
      int sv = sl[j] < 0 ? 0 : sl[j] >= selsz[j] ? selsz[j]-1 : sl[j];
      sel[j] = seloff[j] + sv;
      const float* w = &w1[ size_t(sel[j]) * NIN ];
      float d = 0; for( int i = 0; i < NIN; i++ ) d += w[i] * x[i];
      z1[j] = clip( d, 16.0f ); p1[j] = sqf( z1[j] ); x2[j] = z1[j];
    }
    x2[NSEL] = 0.25f;
    sel2 = s2 < 0 ? 0 : s2 >= sel2sz ? sel2sz-1 : s2;
    const float* w = &w2[ size_t(sel2) * (NSEL+1) ];
    float d = 0; for( int j = 0; j <= NSEL; j++ ) d += w[j] * x2[j];
    d = clip( d, 16.0f ); pz = sqf( d );
    return d;
  }

  void update( int y ) {
    float fy = float(y);
    float e2 = (fy - pz) * lr2;
    float* w = &w2[ size_t(sel2) * (NSEL+1) ];
    for( int j = 0; j <= NSEL; j++ ) w[j] += e2 * x2[j];
    for( int j = 0; j < NSEL; j++ ) {
      float e1 = (fy - p1[j]) * lr1;
      float* v = &w1[ size_t(sel[j]) * NIN ];
      for( int i = 0; i < NIN; i++ ) v[i] += e1 * x[i];
    }
    for( int i = 0; i < NCX; i++ ) {
      Slot& e = *s[i];
      if( !hit[i] ) { e.chk = chk[i]; e.p = 0.5f; e.n = 0; }
      e.p += (fy - e.p) / (float(e.n) + 1.5f);
      if( e.n < lim ) e.n++;
    }
  }
};

} // namespace paq
