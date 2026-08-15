// mp2.cpp - lossless MPEG-1/2/2.5 Layer II <-> Layer III container converter
//
//   mp2 c input.mp2 output.mp3 output.meta    pack mp2 spectral data into mp3
//   mp2 d input.mp3 input.meta output.mp2     restore the original mp2, bit-exact
//
// The mp3 produced by "c" is a syntactically valid Layer III stream (it is not
// supposed to sound like anything) which carries all of the Layer II subband
// samples in its Huffman-coded spectral region.  Everything that has no place
// in the mp3 syntax - frame headers, CRCs, bit allocation, scfsi, scalefactors,
// the trailing/ancillary bits of each Layer II frame and any non-frame bytes of
// the input file - is stored in the .meta side file.
//
// build: g++ -O2 -std=gnu++17 -o mp2 mp2.cpp        (see g2.sh / g2.bat)

#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <vector>

#include "mp3_A.h"    // header/bitrate helpers + Layer II allocation tables
#include "mp3_htabs.h" // Layer III Huffman / sfb / bit-allocation tables

typedef uint8_t  byte;
typedef uint16_t word;
typedef uint32_t uint;
typedef uint64_t qword;

#if defined(_WIN32)
  #define FSEEK64 _fseeki64
  #define FTELL64 _ftelli64
#else
  #define FSEEK64 fseeko
  #define FTELL64 ftello
#endif

static const char MP2_MAGIC[8] = {'M','P','2','>','M','P','3',2};

// number of Layer III granules generated per Layer II frame and per channel.
// 12/G Layer II "parts" (3 samples each) end up in one Layer III granule,
// i.e. 36/G spectral lines per subband.
static const int G_CHOICES[4] = {2, 4, 6, 12};
static const int MAXG = 12;

static int iabs(int x) { return x<0 ? -x : x; }

//---------------------------------------------------------------------------
// bit io
//---------------------------------------------------------------------------

struct BitWriter {
  std::vector<byte> buf;
  qword acc;
  int   accn;
  qword nbits;

  BitWriter() : acc(0), accn(0), nbits(0) {}

  void put(uint v, int n) {          // 0 <= n <= 25
    if( n<=0 ) return;
    acc = (acc<<n)|(qword)(v&(((uint)1<<n)-1));
    accn += n;
    nbits += n;
    while( accn>=8 ) {
      accn -= 8;
      buf.push_back((byte)(acc>>accn));
    }
  }
  void put_long(qword v, int n) {    // n up to 64, msb first
    while( n>24 ) { n -= 24; put((uint)(v>>n), 24); }
    put((uint)v, n);
  }
  void align0() { if( accn ) put(0, 8-accn); }
  qword bitlen() const { return nbits; }
  // byte image; call align0() first
  const byte* data() const { return buf.empty() ? (const byte*)"" : &buf[0]; }
  size_t size() const { return buf.size(); }
};

struct BitReader {
  const byte* buf;
  qword nbits, pos;
  int   over;

  void init(const byte* b, qword bytes) { buf = b; nbits = bytes*8; pos = 0; over = 0; }
  // the caller must guarantee 8 bytes of readable slack past the end
  uint peek32() const {
    const byte* p = buf+(pos>>3);
    qword v = ((qword)p[0]<<56)|((qword)p[1]<<48)|((qword)p[2]<<40)|((qword)p[3]<<32)
             |((qword)p[4]<<24)|((qword)p[5]<<16)|((qword)p[6]<< 8)|((qword)p[7]);
    return (uint)((v<<(pos&7))>>32);
  }
  uint get(int n) {                  // 0 <= n <= 25
    if( n<=0 ) return 0;
    if( pos+n>nbits ) { over = 1; pos += n; return 0; }
    uint v = peek32();
    pos += n;
    return v>>(32-n);
  }
  void skip(qword n) { pos += n; }
};

static void copy_bits(BitReader &r, BitWriter &w, qword n) {
  while( n>=24 ) { w.put(r.get(24), 24); n -= 24; }
  if( n ) w.put(r.get((int)n), (int)n);
}

// Fixed-width packing of a small-alphabet value stream.  Neighbouring values in
// these streams come from the same subband of consecutive frames and are highly
// correlated, so two or four of them share a byte without costing anything -
// it in fact compresses slightly better than one value per byte.
static void pack_stream(const std::vector<byte> &v, int bits, std::vector<byte> &out) {
  BitWriter w;
  for( size_t i = 0; i<v.size(); i++ ) w.put(v[i], bits);
  w.align0();
  out.assign(w.buf.begin(), w.buf.end());
}

static int unpack_stream(const byte* p, qword nbytes, qword count, int bits, std::vector<byte> &out) {
  if( nbytes!=(count*(qword)bits+7)/8 ) return 0;
  BitReader r;
  r.init(p, nbytes);
  out.resize((size_t)count);
  for( qword i = 0; i<count; i++ ) out[(size_t)i] = (byte)r.get(bits);
  return !r.over;
}

//---------------------------------------------------------------------------
// Layer III Huffman tables: the encoder side is derived at start-up straight
// out of the decoder tables of mp3_A.h, so the two can never disagree.
//---------------------------------------------------------------------------

struct HuffTab {
  byte len[16][16];
  uint code[16][16];
  int  maxidx;
};

static HuffTab g_ht[32];
static int     g_hmax[32];           // largest |value| the table can express, -1 = unusable

// exact copy of the pair-decoding step of _L3_huffman()
static int hdec_pair(const _int16* cb, uint bits, int* x, int* y) {
  uint cache = bits;
  int consumed = 0, w = 5;
  int leaf = cb[cache>>(32-w)];
  while( leaf<0 ) {
    cache <<= w;
    consumed += w;
    w = leaf&7;
    leaf = cb[(cache>>(32-w))-(leaf>>3)];
  }
  consumed += leaf>>8;
  *x = leaf&15;
  *y = (leaf>>4)&15;
  return consumed;
}

static void derive_walk(const _int16* cb, HuffTab &h, uint prefix, int len, int &cnt) {
  if( len>19 ) { fprintf(stderr, "mp2: huffman table walk overflow\n"); exit(3); }
  int x, y;
  uint bits = len ? (prefix<<(32-len)) : 0u;
  int consumed = hdec_pair(cb, bits, &x, &y);
  if( consumed<=len ) {
    if( consumed==0 ) return;                    // empty (unused) table
    if( !h.len[x][y] ) { h.len[x][y] = (byte)consumed; h.code[x][y] = prefix>>(len-consumed); cnt++; }
    return;
  }
  derive_walk(cb, h, prefix<<1, len+1, cnt);
  derive_walk(cb, h, (prefix<<1)|1, len+1, cnt);
}

static void init_huff() {
  // ISO/IEC 11172-3 table sizes, used as a sanity check of the derivation
  static const int expect[32] = {
      0,  4,  9,  9,  0, 16, 16, 36, 36, 36, 64, 64, 64,256,  0,256,
    256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256 };
  for( int t = 0; t<32; t++ ) {
    memset(&g_ht[t], 0, sizeof(g_ht[t]));
    g_hmax[t] = -1;
    if( expect[t]==0 ) continue;                 // 0, 4 and 14 are not real tables
    const _int16* cb = g_htabs+g_htabindex[t];
    int cnt = 0;
    derive_walk(cb, g_ht[t], 0, 0, cnt);
    int mx = 0;
    for( int i = 0; i<16; i++ ) for( int j = 0; j<16; j++ ) {
      if( g_ht[t].len[i][j] ) { if( i>mx ) mx = i; if( j>mx ) mx = j; }
    }
    g_ht[t].maxidx = mx;
    if( cnt!=expect[t]||(mx+1)*(mx+1)!=expect[t] ) {
      fprintf(stderr, "mp2: huffman table %d derivation failed (%d/%d entries)\n", t, cnt, expect[t]);
      exit(3);
    }
    int L = g_hlinbits[t];
    g_hmax[t] = L ? 15+(1<<L)-1 : mx;
    // round-trip check: every derived code must decode back to itself
    for( int i = 0; i<=mx; i++ ) for( int j = 0; j<=mx; j++ ) {
      int cl = g_ht[t].len[i][j];
      uint bits = g_ht[t].code[i][j]<<(32-cl);
      int dx, dy, dl = hdec_pair(cb, bits, &dx, &dy);
      if( dl!=cl||dx!=i||dy!=j ) {
        fprintf(stderr, "mp2: huffman table %d round-trip failed at (%d,%d)\n", t, i, j);
        exit(3);
      }
    }
  }
}

// candidate tables, in the order they are tried (table 0 = "all zero", free)
static const byte g_tablist[] = { 1,2,3,5,6,7,8,9,10,11,12,13,15,
                                  16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31 };
static const int  NTAB = (int)sizeof(g_tablist);

static int pair_cost(int t, int ax, int ay) {    // ax,ay = magnitudes
  if( ax>g_hmax[t]||ay>g_hmax[t] ) return -1;
  int L = g_hlinbits[t];
  int cx = ax>15 ? 15 : ax, cy = ay>15 ? 15 : ay;
  int c = g_ht[t].len[cx][cy];
  if( !c ) return -1;
  if( ax ) c++;
  if( ay ) c++;
  if( L ) { if( ax>=15 ) c += L; if( ay>=15 ) c += L; }
  return c;
}

static void write_pair(BitWriter &w, int t, int x, int y) {
  if( !t ) return;                               // table 0 codes (0,0) in zero bits
  int ax = iabs(x), ay = iabs(y);
  int L = g_hlinbits[t];
  int cx = ax>15 ? 15 : ax, cy = ay>15 ? 15 : ay;
  w.put(g_ht[t].code[cx][cy], g_ht[t].len[cx][cy]);
  if( L&&ax>=15 ) w.put((uint)(ax-15), L);
  if( ax ) w.put(x<0, 1);
  if( L&&ay>=15 ) w.put((uint)(ay-15), L);
  if( ay ) w.put(y<0, 1);
}

static void read_pair(BitReader &r, int t, int &x, int &y) {
  if( !t ) { x = y = 0; return; }
  const _int16* cb = g_htabs+g_htabindex[t];
  int ax, ay;
  int consumed = hdec_pair(cb, r.peek32(), &ax, &ay);
  r.pos += consumed;
  int L = g_hlinbits[t];
  if( L&&ax==15 ) ax += (int)r.get(L);
  if( ax ) x = r.get(1) ? -ax : ax; else x = 0;
  if( L&&ay==15 ) ay += (int)r.get(L);
  if( ay ) y = r.get(1) ? -ay : ay; else y = 0;
}

//---------------------------------------------------------------------------
// Layer II frame parsing
//---------------------------------------------------------------------------

// Slot numbering used everywhere below: slot = 2*subband + channel, 0..63.
// The number of slots and their code widths follow from the frame header alone.
static const int NSLOT = 64;

struct L2Layout {
  int total_bands, stereo_bands;
  byte width[32];              // bit_alloc code width per subband
  const byte* tab[32];         // bit_alloc code -> allocation
};

struct L2Info {
  L2Layout lay;
  byte ba[64];                 // allocation, before the ch1 zeroing (scfsi/scf use this)
  byte bas[64];                // allocation, after it (sample reading uses this)
  byte code[64];               // raw bit_alloc codes
  byte scfsi[64];
  uint prefix_bits;            // bit_alloc + scfsi + scalefactors
  uint sample_bits;
};

static int ba_nbits(int ba) { return ba<17 ? ba : (ba==17 ? 5 : (ba==18 ? 7 : 10)); }
static int ba_mod(int ba)   { return ba==17 ? 3 : (ba==18 ? 5 : 9); }

// number of 6-bit scalefactors transmitted for a given scfsi
static int scf_count(int scfsi) {
  int mask = 4+((19>>scfsi)&3);
  return ((mask>>2)&1)+((mask>>1)&1)+(mask&1);
}

// subband grouping / code widths, straight out of _L12_subband_alloc_table()
static void l2_layout(const byte* hdr, L2Layout &L) {
  _L12_scale_info sci;
  const _L12_subband_alloc* sa = sci._L12_subband_alloc_table(hdr);
  L.total_bands = sci.total_bands;
  L.stereo_bands = sci.stereo_bands;
  int k = 0, w = 0;
  const byte* t = g_bitalloc_code_tab_;
  for( int i = 0; i<L.total_bands; i++ ) {
    if( i==k ) { k += sa->band_count; w = sa->code_tab_width; t = g_bitalloc_code_tab_+sa->tab_offset; sa++; }
    L.width[i] = (byte)w;
    L.tab[i] = t;
  }
}

// bit_alloc codes -> allocations, mirroring _L12_read_scale_info()
static void l2_decode_alloc(const L2Layout &L, const byte* code, byte* ba, byte* bas) {
  memset(ba, 0, NSLOT);
  memset(bas, 0, NSLOT);
  for( int sb = 0; sb<L.total_bands; sb++ ) {
    int b0 = L.tab[sb][code[2*sb]], b1;
    if( sb<L.stereo_bands ) b1 = L.tab[sb][code[2*sb+1]];
    else b1 = b0;                       // joint stereo: shared above the bound
    ba[2*sb] = (byte)b0;
    ba[2*sb+1] = (byte)(L.stereo_bands ? b1 : 0);
    bas[2*sb] = (byte)b0;
    bas[2*sb+1] = (byte)(sb<L.stereo_bands ? ba[2*sb+1] : 0);
  }
}

// total number of sample bits of a Layer II frame with this allocation
static uint l2_sample_bits(const L2Info &fi) {
  uint n = 0;
  for( int i = 0; i<2*fi.lay.total_bands; i++ ) {
    int ba = fi.bas[i];
    if( !ba ) continue;
    n += (ba<17 ? 3u*(uint)ba : (uint)ba_nbits(ba))*12u;
  }
  return n;
}

// Parse header + bit_alloc/scfsi/scalefactor region of a Layer II frame.
// When bkA/bkS/bkF are given, the raw field values are appended to their
// per-slot buckets (that is the meta file layout).  0 = not parsable.
static int l2_parse(const byte* fr, int fbytes, L2Info &fi,
                    std::vector<byte>* bkA, std::vector<byte>* bkS, std::vector<byte>* bkF) {
  int hb = 4+(_HDR_IS_CRC(fr) ? 2 : 0);
  if( fbytes<=hb ) return 0;
  BitReader br;
  br.init(fr+hb, (qword)(fbytes-hb));
  l2_layout(fr, fi.lay);
  const L2Layout &L = fi.lay;

  memset(fi.code, 0, sizeof(fi.code));
  for( int sb = 0; sb<L.total_bands; sb++ ) {
    fi.code[2*sb] = (byte)br.get(L.width[sb]);
    if( bkA ) bkA[2*sb].push_back(fi.code[2*sb]);
    if( sb<L.stereo_bands ) {
      fi.code[2*sb+1] = (byte)br.get(L.width[sb]);
      if( bkA ) bkA[2*sb+1].push_back(fi.code[2*sb+1]);
    }
  }
  l2_decode_alloc(L, fi.code, fi.ba, fi.bas);

  memset(fi.scfsi, 6, sizeof(fi.scfsi));
  for( int j = 0; j<2*L.total_bands; j++ ) {
    if( !fi.ba[j] ) continue;
    fi.scfsi[j] = (byte)br.get(2);
    if( bkS ) bkS[j].push_back(fi.scfsi[j]);
  }
  for( int j = 0; j<2*L.total_bands; j++ ) {
    if( !fi.ba[j] ) continue;
    int n = scf_count(fi.scfsi[j]);
    for( int i = 0; i<n; i++ ) {
      byte v = (byte)br.get(6);
      if( bkF ) bkF[j].push_back(v);
    }
  }
  if( br.over ) return 0;
  fi.prefix_bits = (uint)br.pos;
  fi.sample_bits = l2_sample_bits(fi);
  if( (qword)hb*8+fi.prefix_bits+fi.sample_bits>(qword)fbytes*8 ) return 0;
  return 1;
}

// write the bit_alloc/scfsi/scalefactor region back out, bit for bit
static void l2_write_prefix(BitWriter &w, const L2Info &fi, const byte* scf) {
  const L2Layout &L = fi.lay;
  for( int sb = 0; sb<L.total_bands; sb++ ) {
    w.put(fi.code[2*sb], L.width[sb]);
    if( sb<L.stereo_bands ) w.put(fi.code[2*sb+1], L.width[sb]);
  }
  for( int j = 0; j<2*L.total_bands; j++ ) if( fi.ba[j] ) w.put(fi.scfsi[j], 2);
  int k = 0;
  for( int j = 0; j<2*L.total_bands; j++ ) {
    if( !fi.ba[j] ) continue;
    int n = scf_count(fi.scfsi[j]);
    for( int i = 0; i<n; i++ ) w.put(scf[k++], 6);
  }
}

//---------------------------------------------------------------------------
// Layer III granule packing
//---------------------------------------------------------------------------

struct GrParam {
  word part23, big_values;
  byte t[3], r0, r1;
};

// scalefactor band boundaries (cumulative line index) of the long-block table
static void sfb_bounds(const byte* mp3hdr, int* B /*[23]*/) {
  int sr_idx = _HDR_GET_MY_SAMPLE_RATE(mp3hdr);
  sr_idx -= (sr_idx!=0);
  const byte* w = g_scf_long_tab[sr_idx];
  B[0] = 0;
  for( int i = 0; i<22; i++ ) B[i+1] = B[i]+w[i];
}

#define COST_INF (1<<28)

// Pick region split + Huffman tables, return part2_3_length in bits.
//
// With fixtab >= 0 the granule is coded in "uniform" style: one table for all
// three regions, region0_count = region1_count = 0 and big_values = 288, so
// every side info field except part2_3_length is the same in every granule of
// the file.  That is deliberately not the smallest mp3 - it is the one an mp3
// recompressor likes best, since it re-derives the spectral values anyway and
// only has to pay for whatever varies (measured: ~1% smaller after mp3zip,
// even though the mp3 itself grows by a third).
static int choose_granule(const int* v, int nvals, const int* B, GrParam &gp, int fixtab) {
  memset(&gp, 0, sizeof(gp));
  if( fixtab>=0 ) {
    gp.big_values = 288;
    gp.t[0] = gp.t[1] = gp.t[2] = (byte)fixtab;
    int bits = 0;
    for( int i = 0; i<288; i++ ) {
      int c = pair_cost(fixtab, iabs(v[2*i]), iabs(v[2*i+1]));
      if( c<0 ) return COST_INF;
      bits += c;
    }
    gp.part23 = (word)(bits>65535 ? 65535 : bits);
    return bits;
  }
  int P = (nvals+1)>>1;
  if( P<=0 ) return 0;
  if( P>288 ) P = 288;
  gp.big_values = (word)P;

  int bp[23];
  for( int k = 0; k<=22; k++ ) { int q = B[k]>>1; bp[k] = q>P ? P : q; }

  int segmax[22];
  for( int k = 0; k<22; k++ ) {
    int mx = 0;
    for( int i = bp[k]; i<bp[k+1]; i++ ) {
      int a = iabs(v[2*i]), b = iabs(v[2*i+1]);
      if( a>mx ) mx = a;
      if( b>mx ) mx = b;
    }
    segmax[k] = mx;
  }

  static int psb[32][23];
  for( int ti = 0; ti<NTAB; ti++ ) {
    int t = g_tablist[ti];
    psb[t][0] = 0;
    for( int k = 0; k<22; k++ ) {
      int s = 0;
      if( segmax[k]<=g_hmax[t] ) {
        if( segmax[k]==0 ) s = (bp[k+1]-bp[k])*g_ht[t].len[0][0];   // all-zero band
        else for( int i = bp[k]; i<bp[k+1]; i++ ) s += pair_cost(t, iabs(v[2*i]), iabs(v[2*i+1]));
      }
      psb[t][k+1] = psb[t][k]+s;
    }
  }

  // best table (and cost) for every band range [i,j)
  static int bcost[23][23];
  static byte btab[23][23];
  for( int i = 0; i<=22; i++ ) for( int j = i; j<=22; j++ ) { bcost[i][j] = 0; btab[i][j] = 0; }
  for( int i = 0; i<22; i++ ) {
    int rmax = 0;
    for( int j = i+1; j<=22; j++ ) {
      if( segmax[j-1]>rmax ) rmax = segmax[j-1];
      if( bp[i]==bp[j]||rmax==0 ) { bcost[i][j] = 0; btab[i][j] = 0; continue; }
      int best = COST_INF, bt = 0;
      for( int ti = 0; ti<NTAB; ti++ ) {
        int t = g_tablist[ti];
        if( g_hmax[t]<rmax ) continue;
        int c = psb[t][j]-psb[t][i];
        if( c<best ) { best = c; bt = t; }
      }
      bcost[i][j] = best;
      btab[i][j] = (byte)bt;
    }
  }

  int best = COST_INF, br0 = 0, br1 = 0;
  for( int r0 = 0; r0<16; r0++ ) {
    for( int r1 = 0; r1<8; r1++ ) {
      int k1 = r0+1, k2 = r0+r1+2;
      if( k2>22 ) break;
      int c = bcost[0][k1]+bcost[k1][k2]+bcost[k2][22];
      if( c<best ) { best = c; br0 = r0; br1 = r1; }
    }
  }
  if( best>=COST_INF ) {
    fprintf(stderr, "mp2: internal error - granule not representable\n");
    exit(3);
  }
  int k1 = br0+1, k2 = br0+br1+2;
  gp.r0 = (byte)br0;
  gp.r1 = (byte)br1;
  gp.t[0] = btab[0][k1];
  gp.t[1] = btab[k1][k2];
  gp.t[2] = btab[k2][22];
  gp.part23 = (word)best;
  return best;
}

static void granule_pairs(const GrParam &gp, const int* B, int* p1, int* p2) {
  int P = gp.big_values;
  int k1 = gp.r0+1, k2 = gp.r0+gp.r1+2;
  if( k1>22 ) k1 = 22;
  if( k2>22 ) k2 = 22;
  int a = B[k1]>>1, b = B[k2]>>1;
  *p1 = a>P ? P : a;
  *p2 = b>P ? P : b;
}

static void write_granule(BitWriter &w, const int* v, const GrParam &gp, const int* B) {
  int P = gp.big_values, p1, p2;
  granule_pairs(gp, B, &p1, &p2);
  int i = 0;
  for(; i<p1; i++ ) write_pair(w, gp.t[0], v[2*i], v[2*i+1]);
  for(; i<p2; i++ ) write_pair(w, gp.t[1], v[2*i], v[2*i+1]);
  for(; i<P;  i++ ) write_pair(w, gp.t[2], v[2*i], v[2*i+1]);
}

static void read_granule(BitReader &r, const GrParam &gp, const int* B, int* v) {
  memset(v, 0, 576*sizeof(int));
  int P = gp.big_values, p1, p2;
  if( P>288 ) P = 288;
  granule_pairs(gp, B, &p1, &p2);
  int i = 0;
  for(; i<p1; i++ ) read_pair(r, gp.t[0], v[2*i], v[2*i+1]);
  for(; i<p2; i++ ) read_pair(r, gp.t[1], v[2*i], v[2*i+1]);
  for(; i<P;  i++ ) read_pair(r, gp.t[2], v[2*i], v[2*i+1]);
}

//---------------------------------------------------------------------------
// mp3 frame header / side info
//---------------------------------------------------------------------------

static void build_mp3_header(byte* h, const byte* mp2hdr, int bitrate_index) {
  h[0] = 0xFF;
  h[1] = (byte)((mp2hdr[1]&0xF8)|0x02|0x01);   // keep sync+version, layer III, no CRC
  h[2] = (byte)((bitrate_index<<4)|(mp2hdr[2]&0x0C));
  h[3] = (byte)(_HDR_IS_MONO(mp2hdr) ? (3<<6) : 0);
}

static int side_info_bytes(int mpeg1, int nch) {
  return mpeg1 ? (nch==1 ? 17 : 32) : (nch==1 ? 9 : 17);
}

static int max_reservoir(int mpeg1) { return mpeg1 ? 511 : 255; }

static void write_side_info(BitWriter &w, int mpeg1, int nch, int main_data_begin,
                            const GrParam* gp, int nslots) {
  if( mpeg1 ) { w.put((uint)main_data_begin, 9); w.put(0, 7+2*nch); }
  else        { w.put((uint)main_data_begin, 8); w.put(0, nch); }
  for( int s = 0; s<nslots; s++ ) {
    const GrParam &g = gp[s];
    w.put(g.part23, 12);
    w.put(g.big_values, 9);
    w.put(210, 8);                       // global_gain (arbitrary)
    w.put(0, mpeg1 ? 4 : 9);             // scalefac_compress -> zero-length scalefactors
    w.put(0, 1);                         // window_switching_flag
    w.put(g.t[0], 5); w.put(g.t[1], 5); w.put(g.t[2], 5);
    w.put(g.r0, 4); w.put(g.r1, 3);
    if( mpeg1 ) w.put(0, 1);             // preflag
    w.put(0, 1);                         // scalefac_scale
    w.put(0, 1);                         // count1table_select
  }
}

static int read_side_info(BitReader &r, int mpeg1, int nch, GrParam* gp, int nslots) {
  int main_data_begin;
  if( mpeg1 ) { main_data_begin = (int)r.get(9); r.get(7+2*nch); }
  else        { main_data_begin = (int)r.get(8); r.get(nch); }
  for( int s = 0; s<nslots; s++ ) {
    gp[s].part23     = (word)r.get(12);
    gp[s].big_values = (word)r.get(9);
    r.get(8);
    r.get(mpeg1 ? 4 : 9);
    r.get(1);
    gp[s].t[0] = (byte)r.get(5); gp[s].t[1] = (byte)r.get(5); gp[s].t[2] = (byte)r.get(5);
    gp[s].r0 = (byte)r.get(4); gp[s].r1 = (byte)r.get(3);
    if( mpeg1 ) r.get(1);
    r.get(1);
    r.get(1);
  }
  return main_data_begin;
}

//---------------------------------------------------------------------------
// whole-file helpers
//---------------------------------------------------------------------------

struct Buf {
  std::vector<byte> v;
  size_t n;
  Buf() : n(0) {}
  const byte* p() const { return v.empty() ? (const byte*)"" : &v[0]; }
};

static int read_file(const char* path, Buf &b) {
  FILE* f = fopen(path, "rb");
  if( !f ) return 0;
  FSEEK64(f, 0, SEEK_END);
  long long n = FTELL64(f);
  FSEEK64(f, 0, SEEK_SET);
  if( n<0 ) { fclose(f); return 0; }
  b.n = (size_t)n;
  b.v.assign(b.n+16, 0);
  if( b.n&&fread(&b.v[0], 1, b.n, f)!=b.n ) { fclose(f); return 0; }
  fclose(f);
  return 1;
}

static int write_file(const char* path, const void* p, size_t n) {
  FILE* f = fopen(path, "wb");
  if( !f ) return 0;
  if( n&&fwrite(p, 1, n, f)!=n ) { fclose(f); return 0; }
  fclose(f);
  return 1;
}

static void put32at(byte* p, uint x) { p[0] = (byte)x; p[1] = (byte)(x>>8); p[2] = (byte)(x>>16); p[3] = (byte)(x>>24); }
static void put32(std::vector<byte> &v, uint x) {
  v.push_back((byte)x); v.push_back((byte)(x>>8)); v.push_back((byte)(x>>16)); v.push_back((byte)(x>>24));
}
static void put64(std::vector<byte> &v, qword x) { put32(v, (uint)x); put32(v, (uint)(x>>32)); }
static uint get32(const byte* p) { return (uint)p[0]|((uint)p[1]<<8)|((uint)p[2]<<16)|((uint)p[3]<<24); }
static qword get64(const byte* p) { return (qword)get32(p)|((qword)get32(p+4)<<32); }

static void put_section(std::vector<byte> &meta, const byte* p, size_t n) {
  put64(meta, (qword)n);
  meta.insert(meta.end(), p, p+n);
}


//---------------------------------------------------------------------------
// scalefactor image
//
// The scalefactor stream is the biggest thing in the meta and the one with the
// most structure, so it can optionally go out as an 8-bit BMP for an image
// compressor to take instead.  BMF gets it to 246 612 bytes against 284 452 for
// xz on the same bytes - 13 % - because it is a context model and xz is not.
//
// Layout is the meta's own slot-major order folded into columns: value i sits
// at (i/H, i%H), so going down a column steps through one subband in time,
// which is the direction the correlation runs.  The width is arbitrary and read
// back from the header; 2048 measured best and the choice is worth well under
// a percent.
//---------------------------------------------------------------------------

static const int SCF_BMP_WIDTH = 2048;

static int write_scf_bmp(const char* path, const std::vector<byte> &v) {
  int W = SCF_BMP_WIDTH;
  qword N = v.size();
  int H = (int)((N+(qword)W-1)/(qword)W);
  if( H<1 ) H = 1;
  int stride = (W+3)&~3;
  size_t off = 14+40+256*4;
  std::vector<byte> o(off+(size_t)stride*H, 0);
  o[0] = 'B'; o[1] = 'M';
  put32at(&o[0]+2, (uint)o.size());
  put32at(&o[0]+10, (uint)off);
  put32at(&o[0]+14, 40);
  put32at(&o[0]+18, (uint)W);
  put32at(&o[0]+22, (uint)H);
  o[26] = 1; o[28] = 8;                       // 1 plane, 8 bits
  put32at(&o[0]+46, 256);
  put32at(&o[0]+50, 256);
  for( int i = 0; i<256; i++ ) o[54+i*4] = o[55+i*4] = o[56+i*4] = (byte)i;
  for( int c = 0; c<W; c++ ) {
    for( int y = 0; y<H; y++ ) {
      qword idx = (qword)c*H+y;
      if( idx>=N ) break;
      o[off+(size_t)(H-1-y)*stride+c] = v[(size_t)idx];
    }
  }
  return write_file(path, &o[0], o.size());
}

// reads back both plain and RLE8 BMPs - an image compressor may re-emit the
// file run-length coded, and the round trip has to survive that
static int read_scf_bmp(const char* path, qword count, std::vector<byte> &out) {
  Buf b;
  if( !read_file(path, b) ) return 0;
  if( b.n<54||b.p()[0]!='B'||b.p()[1]!='M' ) return 0;
  qword off = get32(b.p()+10);
  int W = (int)get32(b.p()+18);
  int H = (int)get32(b.p()+22);
  int bpp = b.p()[28]|(b.p()[29]<<8);
  uint comp = get32(b.p()+30);
  int topdown = 0;
  if( H<0 ) { H = -H; topdown = 1; }
  if( bpp!=8||W<=0||H<=0||(comp!=0&&comp!=1) ) return 0;
  if( (qword)W*(qword)H<count ) return 0;

  std::vector<byte> px((size_t)W*H, 0);        // stored-row order
  if( comp==0 ) {
    qword stride = ((qword)W+3)&~(qword)3;
    if( off+stride*H>b.n ) return 0;
    for( int y = 0; y<H; y++ ) memcpy(&px[(size_t)y*W], b.p()+off+stride*y, W);
  } else {
    qword i = off;
    int x = 0, y = 0;
    while( i+1<b.n&&y<H ) {
      int a = b.p()[i], v = b.p()[i+1];
      i += 2;
      if( a ) {
        for( int k = 0; k<a; k++, x++ ) if( x<W ) px[(size_t)y*W+x] = (byte)v;
      } else if( v==0 ) { x = 0; y++; }
      else if( v==1 ) break;
      else if( v==2 ) { if( i+1>=b.n ) break; x += b.p()[i]; y += b.p()[i+1]; i += 2; }
      else {
        if( i+(qword)v>b.n ) break;
        for( int k = 0; k<v; k++, x++ ) if( x<W ) px[(size_t)y*W+x] = b.p()[i+k];
        i += (qword)v+(v&1);
      }
    }
  }
  out.resize((size_t)count);
  for( qword idx = 0; idx<count; idx++ ) {
    qword c = idx/(qword)H, y = idx%(qword)H;
    int sy = topdown ? (int)y : H-1-(int)y;    // undo the bottom-up storage
    out[(size_t)idx] = px[(size_t)sy*W+(size_t)c];
  }
  return 1;
}

//---------------------------------------------------------------------------
// frame scan
//---------------------------------------------------------------------------

struct FrameRec { size_t off; int bytes; };
struct GapRec   { uint index; size_t off; size_t len; };

static int mp2_frame_size(const byte* h, int free_format_bytes) {
  int fb = _hdr_frame_bytes(h, free_format_bytes);
  if( !fb ) return 0;
  return fb+_hdr_padding(h);
}

static int is_l2_header(const byte* p) {
  return p[0]==0xFF&&_hdr_valid(p)&&_HDR_GET_LAYER(p)==2;
}

static void scan_mp2(const Buf &in, std::vector<FrameRec> &frames, std::vector<GapRec> &gaps,
                     int &free_format_bytes) {
  size_t p = 0, gap_start = 0;
  int insync = 0;
  free_format_bytes = 0;

  while( p+4<=in.n ) {
    if( !is_l2_header(in.p()+p) ) { p++; insync = 0; continue; }

    if( _HDR_IS_FREE_FORMAT(in.p()+p)&&!free_format_bytes ) {
      // measure the free-format frame length once, from the distance to the
      // next matching header
      size_t q;
      for( q = p+24; q+4<=in.n&&q-p<=(size_t)_MAX_FREE_FORMAT_FRAME_SIZE+4; q++ ) {
        if( _hdr_compare(in.p()+p, in.p()+q) ) break;
      }
      if( q+4<=in.n&&q-p<=(size_t)_MAX_FREE_FORMAT_FRAME_SIZE+4 )
        free_format_bytes = (int)(q-p)-_hdr_padding(in.p()+p);
    }
    int fs = mp2_frame_size(in.p()+p, free_format_bytes);
    if( fs<8||p+(size_t)fs>in.n ) { p++; insync = 0; continue; }

    L2Info fi;
    if( !l2_parse(in.p()+p, fs, fi, 0, 0, 0) ) { p++; insync = 0; continue; }

    if( !insync ) {
      // require the following frame header to match, to avoid false syncs
      size_t q = p+fs;
      if( q+4<=in.n&&!(is_l2_header(in.p()+q)&&_hdr_compare(in.p()+p, in.p()+q)) ) {
        p++;
        continue;
      }
    }
    if( p>gap_start ) {
      GapRec g;
      g.index = (uint)frames.size();
      g.off = gap_start;
      g.len = p-gap_start;
      gaps.push_back(g);
    }
    FrameRec f;
    f.off = p;
    f.bytes = fs;
    frames.push_back(f);
    p += fs;
    gap_start = p;
    insync = 1;
  }
  if( in.n>gap_start ) {
    GapRec g;
    g.index = (uint)frames.size();
    g.off = gap_start;
    g.len = in.n-gap_start;
    gaps.push_back(g);
  }
}

//---------------------------------------------------------------------------
// compression
//---------------------------------------------------------------------------

static int g_vals[2][MAXG][576];

struct EncOut {
  int ok;
  int G;
  int bitrate_index;
  int frames;                 // mp3 frames written
  int maxgran;                // largest part2_3_length used
  int fixtab;                 // uniform Huffman table, -1 = per-granule optimised
  qword src_sample_bits;      // Layer II sample bits packed
  qword huff_bits;            // Layer III huffman bits produced
  std::vector<byte> mp3;
  std::vector<byte> meta;
  std::vector<byte> scf;      // slot-major scalefactors, for the optional bmp
  qword md_bytes;
};

// Parse one Layer II frame and spread its sample codes over Layer III spectral
// lines in g_vals: subband sb of parts [g*PPG, g*PPG+PPG) -> lines
// [sb*VPS, sb*VPS+VPS) of granule g.  bk*/whi/wtail may be null (dry run).
static int frame_to_values(const byte* fr, int fbytes, int PPG, int VPS, L2Info &li,
                           std::vector<byte>* bkA, std::vector<byte>* bkS, std::vector<byte>* bkF,
                           BitWriter* whi, BitWriter* wtail) {
  int hb = 4+(_HDR_IS_CRC(fr) ? 2 : 0);
  if( !l2_parse(fr, fbytes, li, bkA, bkS, bkF) ) return 0;

  BitReader br;
  br.init(fr+hb, (qword)(fbytes-hb));
  br.pos = li.prefix_bits;

  memset(g_vals, 0, sizeof(g_vals));
  int tb = li.lay.total_bands;
  for( int part = 0; part<12; part++ ) {
    int gi = part/PPG, lp = part%PPG;
    for( int i = 0; i<2*tb; i++ ) {
      int ba = li.bas[i];
      if( !ba ) continue;
      int sb = i>>1, ch = i&1;
      int* dst = &g_vals[ch][gi][sb*VPS+lp*3];
      if( ba<17 ) {
        if( ba<15 ) {
          int half = (1<<(ba-1))-1;
          for( int k = 0; k<3; k++ ) dst[k] = (int)br.get(ba)-half;
        } else {
          for( int k = 0; k<3; k++ ) {
            uint code = br.get(ba);
            if( whi ) whi->put(code>>14, ba-14);   // top bits do not fit in an mp3 value
            dst[k] = (int)(code&0x3FFF)-8191;
          }
        }
      } else {
        int mod = ba_mod(ba), h = mod/2;
        uint code = br.get(ba_nbits(ba));
        dst[0] = (int)(code%(uint)mod)-h;
        dst[1] = (int)((code/(uint)mod)%(uint)mod)-h;
        dst[2] = (int)(code/(uint)(mod*mod))-h;
      }
    }
  }
  if( br.over ) return 0;
  if( wtail ) {
    qword used = (qword)hb*8+li.prefix_bits+li.sample_bits;
    copy_bits(br, *wtail, (qword)fbytes*8-used);
  }
  return 1;
}

// Best single Huffman table for the whole file, or -1 if none can express every
// value.  The exact cost of a table follows from a 16x16 histogram of clamped
// magnitude pairs plus the escape and nonzero counts, so all candidates can be
// scored from one pass over the file.
static int pick_uniform_table(const Buf &in, const std::vector<FrameRec> &frames, int G) {
  const int PPG = 12/G, VPS = 3*PPG;
  qword H[16][16], esc = 0, nz = 0;
  int maxabs = 0;
  memset(H, 0, sizeof(H));
  for( size_t f = 0; f<frames.size(); f++ ) {
    L2Info li;
    if( !frame_to_values(in.p()+frames[f].off, frames[f].bytes, PPG, VPS, li, 0, 0, 0, 0, 0) ) return -1;
    int nch = _HDR_IS_MONO(in.p()+frames[f].off) ? 1 : 2;
    for( int ch = 0; ch<nch; ch++ ) for( int gi = 0; gi<G; gi++ ) {
      const int* v = g_vals[ch][gi];
      for( int i = 0; i<288; i++ ) {
        int a = iabs(v[2*i]), b = iabs(v[2*i+1]);
        if( a>maxabs ) maxabs = a;
        if( b>maxabs ) maxabs = b;
        if( a ) nz++;
        if( b ) nz++;
        if( a>=15 ) esc++;
        if( b>=15 ) esc++;
        H[a>15 ? 15 : a][b>15 ? 15 : b]++;
      }
    }
  }
  int best = -1;
  qword bestcost = 0;
  for( int ti = 0; ti<NTAB; ti++ ) {
    int t = g_tablist[ti];
    if( g_hmax[t]<maxabs ) continue;
    qword c = nz+(qword)g_hlinbits[t]*esc;
    for( int i = 0; i<16; i++ ) for( int j = 0; j<16; j++ ) {
      if( H[i][j] ) c += H[i][j]*(qword)g_ht[t].len[i][j];
    }
    if( best<0||c<bestcost ) { best = t; bestcost = c; }
  }
  return best;
}

static int encode_pass(const Buf &in, const std::vector<FrameRec> &frames,
                       const std::vector<GapRec> &gaps, int free_format_bytes,
                       int G, int fixtab, int scf_external, EncOut &out) {
  const int PPG = 12/G;                 // Layer II parts per Layer III granule
  const int VPS = 3*PPG;                // spectral lines per subband per granule

  BitWriter wtail, whi;
  std::vector<byte> bkA[NSLOT], bkS[NSLOT], bkF[NSLOT];
  std::vector<byte> hdrs, crcs;
  std::vector<byte> md;                 // concatenated main data, byte aligned per frame
  std::vector<uint> md_off, md_len;
  std::vector<GrParam> gps;
  std::vector<byte> f_hdr;              // mp3 header of every mp3 frame
  std::vector<byte> f_slots;

  int maxgran = 0;
  qword src_sample_bits = 0, huff_bits = 0;
  size_t nfr = frames.size();

  hdrs.reserve(nfr*4);
  md.reserve(in.n+in.n/2+1024);

  for( size_t fi = 0; fi<nfr; fi++ ) {
    const byte* fr = in.p()+frames[fi].off;
    int fbytes = frames[fi].bytes;
    int hb = 4+(_HDR_IS_CRC(fr) ? 2 : 0);
    int mpeg1 = _HDR_TEST_MPEG1(fr) ? 1 : 0;
    int nch = _HDR_IS_MONO(fr) ? 1 : 2;

    L2Info li;
    if( !frame_to_values(fr, fbytes, PPG, VPS, li, bkA, bkS, bkF, &whi, &wtail) ) {
      fprintf(stderr, "mp2: frame %d unparsable\n", (int)fi);
      return 0;
    }
    int tb = li.lay.total_bands;

    hdrs.push_back(fr[0]); hdrs.push_back(fr[1]); hdrs.push_back(fr[2]); hdrs.push_back(fr[3]);
    if( hb==6 ) { crcs.push_back(fr[4]); crcs.push_back(fr[5]); }

    src_sample_bits += li.sample_bits;

    // ---- how many spectral lines each channel actually uses
    int nvals[2] = {0, 0};
    for( int ch = 0; ch<nch; ch++ ) {
      int last = -1;
      for( int sb = 0; sb<tb; sb++ ) if( li.bas[2*sb+ch] ) last = sb;
      nvals[ch] = last<0 ? 0 : VPS*(last+1);
    }

    byte mh[4];
    build_mp3_header(mh, fr, 1);        // bitrate index is patched in later
    int B[23];
    sfb_bounds(mh, B);

    int gr_per_frame = mpeg1 ? 2 : 1;
    int nslots = gr_per_frame*nch;
    int nmp3 = G/gr_per_frame;

    for( int mf = 0; mf<nmp3; mf++ ) {
      BitWriter w;
      GrParam slot[4];
      for( int s = 0; s<nslots; s++ ) {
        int gr = s/nch, ch = s%nch;
        int gi = mf*gr_per_frame+gr;
        int* v = g_vals[ch][gi];
        int bits = choose_granule(v, nvals[ch], B, slot[s], fixtab);
        if( bits>4095 ) return 0;      // does not fit into part2_3_length
        if( bits>maxgran ) maxgran = bits;
        huff_bits += (qword)bits;
        qword before = w.bitlen();
        write_granule(w, v, slot[s], B);
        if( w.bitlen()-before!=(qword)bits ) {
          fprintf(stderr, "mp2: internal error - granule size mismatch (%d vs %d)\n",
                  (int)(w.bitlen()-before), bits);
          exit(3);
        }
      }
      w.align0();
      md_off.push_back((uint)md.size());
      md_len.push_back((uint)w.size());
      md.insert(md.end(), w.buf.begin(), w.buf.end());
      for( int s = 0; s<nslots; s++ ) gps.push_back(slot[s]);
      f_hdr.push_back(mh[0]); f_hdr.push_back(mh[1]); f_hdr.push_back(mh[2]); f_hdr.push_back(mh[3]);
      f_slots.push_back((byte)nslots);
    }
  }

  // ---- per frame, take the smallest bitrate whose slot, together with what the
  //      bit reservoir already holds, carries that frame's main data (plain VBR)
  int nmp3frames = (int)md_off.size();
  std::vector<int> cap((size_t)nmp3frames), mdb((size_t)nmp3frames), stuff((size_t)nmp3frames);
  std::vector<byte> bri((size_t)nmp3frames);
  int brmin = 15, brmax = 0;
  {
    int R = 0;
    for( int f = 0; f<nmp3frames; f++ ) {
      byte h[4];
      memcpy(h, &f_hdr[f*4], 4);
      int mpeg1 = _HDR_TEST_MPEG1(h) ? 1 : 0;
      int si = side_info_bytes(mpeg1, _HDR_IS_MONO(h) ? 1 : 2);
      int need = (int)md_len[f]-R;
      int sel = 0, C = 0;
      for( int cand = 1; cand<=14; cand++ ) {
        h[2] = (byte)((cand<<4)|(h[2]&0x0F));
        int c = _hdr_frame_bytes(h, 0)-4-si;
        if( c<0 ) continue;
        if( c>=need ) { sel = cand; C = c; break; }
      }
      if( !sel ) return 0;               // even 320 kbps is not enough -> larger G
      bri[f] = (byte)sel;
      cap[f] = C;
      mdb[f] = R;
      if( sel<brmin ) brmin = sel;
      if( sel>brmax ) brmax = sel;
      int nr = R+C-(int)md_len[f];
      int mx = max_reservoir(mpeg1);
      stuff[f] = nr>mx ? nr-mx : 0;
      R = nr-stuff[f];
    }
  }

  // ---- lay the main data out as one continuous byte stream
  std::vector<byte> mdstream;
  qword slots_total = 0;
  mdstream.reserve(md.size()+(size_t)nmp3frames*4+16);
  for( int f = 0; f<nmp3frames; f++ ) {
    mdstream.insert(mdstream.end(), md.begin()+md_off[f], md.begin()+md_off[f]+md_len[f]);
    mdstream.insert(mdstream.end(), (size_t)stuff[f], (byte)0);
    slots_total += (qword)cap[f];
  }
  if( mdstream.size()<slots_total ) mdstream.resize((size_t)slots_total, 0);

  // ---- assemble the mp3
  std::vector<byte> mp3;
  size_t slot_pos = 0;
  qword sp = 0;
  for( int f = 0; f<nmp3frames; f++ ) {
    byte h[4];
    memcpy(h, &f_hdr[f*4], 4);
    h[2] = (byte)((bri[f]<<4)|(h[2]&0x0F));
    int mpeg1 = _HDR_TEST_MPEG1(h) ? 1 : 0;
    int nch = _HDR_IS_MONO(h) ? 1 : 2;
    int nslots = f_slots[f];
    BitWriter w;
    w.put(h[0], 8); w.put(h[1], 8); w.put(h[2], 8); w.put(h[3], 8);
    write_side_info(w, mpeg1, nch, mdb[f], &gps[slot_pos], nslots);
    slot_pos += nslots;
    w.align0();
    mp3.insert(mp3.end(), w.buf.begin(), w.buf.end());
    mp3.insert(mp3.end(), mdstream.begin()+(size_t)sp, mdstream.begin()+(size_t)sp+cap[f]);
    sp += (qword)cap[f];
  }

  // ---- meta file
  wtail.align0();
  whi.align0();

  std::vector<byte> meta;
  meta.insert(meta.end(), MP2_MAGIC, MP2_MAGIC+8);
  put32(meta, (uint)G);
  put32(meta, 0);                        // reserved (mp3 bitrates live in the mp3 headers)
  put32(meta, (uint)nfr);
  put32(meta, (uint)free_format_bytes);
  put32(meta, (uint)(scf_external ? 1 : 0));   // flags: bit 0 = scalefactors are in the bmp

  std::vector<byte> gapblob;
  put32(gapblob, (uint)gaps.size());
  for( size_t i = 0; i<gaps.size(); i++ ) {
    put32(gapblob, gaps[i].index);
    put64(gapblob, (qword)gaps[i].len);
    gapblob.insert(gapblob.end(), in.p()+gaps[i].off, in.p()+gaps[i].off+gaps[i].len);
  }
  // headers and CRCs go out as byte planes (byte 0 of every frame, then byte 1,
  // ...) - all but the bitrate/padding nibble of byte 2 is constant that way
  std::vector<byte> hplanes, cplanes;
  hplanes.resize(hdrs.size());
  for( size_t i = 0; i<nfr; i++ ) for( int b = 0; b<4; b++ ) hplanes[b*nfr+i] = hdrs[i*4+b];
  cplanes.resize(crcs.size());
  for( size_t i = 0; i<crcs.size()/2; i++ ) {
    cplanes[i] = crcs[i*2];
    cplanes[crcs.size()/2+i] = crcs[i*2+1];
  }
  // bit_alloc / scfsi / scalefactors: one byte per field value, grouped by slot
  // so that a slot's values across the whole file are contiguous
  std::vector<byte> sA, sS, sF, pA, pS;
  for( int s = 0; s<NSLOT; s++ ) sA.insert(sA.end(), bkA[s].begin(), bkA[s].end());
  for( int s = 0; s<NSLOT; s++ ) sS.insert(sS.end(), bkS[s].begin(), bkS[s].end());
  for( int s = 0; s<NSLOT; s++ ) sF.insert(sF.end(), bkF[s].begin(), bkF[s].end());
  pack_stream(sA, 4, pA);                // bit_alloc codes are at most 4 bits
  pack_stream(sS, 2, pS);                // scfsi is 2 bits
                                         // scalefactors stay one per byte: 6-bit
                                         // packing measurably hurts compression

  put_section(meta, gapblob.empty() ? (const byte*)"" : &gapblob[0], gapblob.size());
  put_section(meta, hplanes.empty() ? (const byte*)"" : &hplanes[0], hplanes.size());
  put_section(meta, cplanes.empty() ? (const byte*)"" : &cplanes[0], cplanes.size());
  put_section(meta, pA.empty() ? (const byte*)"" : &pA[0], pA.size());
  put_section(meta, pS.empty() ? (const byte*)"" : &pS[0], pS.size());
  if( scf_external ) put_section(meta, (const byte*)"", 0);
  else               put_section(meta, sF.empty() ? (const byte*)"" : &sF[0], sF.size());
  put_section(meta, wtail.data(), wtail.size());
  put_section(meta, whi.data(), whi.size());

  out.ok = 1;
  out.G = G;
  out.bitrate_index = brmin|(brmax<<8);
  out.frames = nmp3frames;
  out.maxgran = maxgran;
  out.fixtab = fixtab;
  out.src_sample_bits = src_sample_bits;
  out.huff_bits = huff_bits;
  out.mp3.swap(mp3);
  out.meta.swap(meta);
  out.scf.swap(sF);
  out.md_bytes = md.size();
  return 1;
}

static int compress(const char* inpath, const char* mp3path, const char* metapath,
                    const char* bmppath) {
  Buf in;
  if( !read_file(inpath, in) ) { fprintf(stderr, "mp2: cannot read '%s'\n", inpath); return 1; }

  std::vector<FrameRec> frames;
  std::vector<GapRec> gaps;
  int free_format_bytes = 0;
  scan_mp2(in, frames, gaps, free_format_bytes);

  if( frames.empty() ) fprintf(stderr, "mp2: warning - no Layer II frames found\n");

  EncOut out;
  out.ok = 0;
  for( int gi = 0; gi<4&&!out.ok; gi++ ) {
    int G = G_CHOICES[gi];
    int ut = frames.empty() ? -1 : pick_uniform_table(in, frames, G);
    if( ut>=0&&encode_pass(in, frames, gaps, free_format_bytes, G, ut, bmppath!=0, out) ) break;
    out.ok = 0;
    if( encode_pass(in, frames, gaps, free_format_bytes, G, -1, bmppath!=0, out) ) break;
    out.ok = 0;
  }
  if( !out.ok ) { fprintf(stderr, "mp2: failed to pack the stream\n"); return 1; }

  if( !write_file(mp3path, out.mp3.empty() ? (const void*)"" : &out.mp3[0], out.mp3.size()) ) {
    fprintf(stderr, "mp2: cannot write '%s'\n", mp3path);
    return 1;
  }
  if( !write_file(metapath, &out.meta[0], out.meta.size()) ) {
    fprintf(stderr, "mp2: cannot write '%s'\n", metapath);
    return 1;
  }
  if( bmppath&&!write_scf_bmp(bmppath, out.scf) ) {
    fprintf(stderr, "mp2: cannot write '%s'\n", bmppath);
    return 1;
  }

  printf("mp2 frames : %u\n", (uint)frames.size());
  printf("mp3 frames : %d\n", out.frames);
  printf("granules   : %d per frame per channel\n", out.G);
  if( out.fixtab>=0 ) printf("side info  : uniform (huffman table %d, fixed regions)\n", out.fixtab);
  else                printf("side info  : per-granule optimised\n");
  if( out.frames ) {
    byte h[4];
    memcpy(h, out.mp3.empty() ? (const byte*)"\0\0\0\0" : &out.mp3[0], 4);
    h[2] = (byte)(((out.bitrate_index&255)<<4)|(h[2]&0x0F));
    uint lo = _hdr_bitrate_kbps(h);
    h[2] = (byte)((((out.bitrate_index>>8)&255)<<4)|(h[2]&0x0F));
    uint hi = _hdr_bitrate_kbps(h);
    printf("mp3 bitrate: %u..%u kbps (vbr)\n", lo, hi);
  }
  printf("max part23 : %d bits\n", out.maxgran);
  printf("samples    : %llu bytes mp2 -> %llu bytes huffman (%.1f%%)\n",
         (unsigned long long)(out.src_sample_bits/8), (unsigned long long)(out.huff_bits/8),
         out.src_sample_bits ? 100.0*(double)out.huff_bits/(double)out.src_sample_bits : 0.0);
  printf("input      : %llu bytes\n", (unsigned long long)in.n);
  printf("output.mp3 : %llu bytes\n", (unsigned long long)out.mp3.size());
  printf("output.meta: %llu bytes\n", (unsigned long long)out.meta.size());
  if( bmppath ) printf("output.bmp : %llu scalefactors\n", (unsigned long long)out.scf.size());
  return 0;
}

//---------------------------------------------------------------------------
// decompression
//---------------------------------------------------------------------------

struct Section { const byte* p; qword n; };

static int get_section(const Buf &meta, size_t &pos, Section &s) {
  if( pos+8>meta.n ) return 0;
  qword n = get64(meta.p()+pos);
  pos += 8;
  if( pos+n>meta.n ) return 0;
  s.p = meta.p()+pos;
  s.n = n;
  pos += (size_t)n;
  return 1;
}

static int decompress(const char* mp3path, const char* metapath, const char* outpath,
                      const char* bmppath) {
  Buf mp3, meta;
  if( !read_file(mp3path, mp3) ) { fprintf(stderr, "mp2: cannot read '%s'\n", mp3path); return 1; }
  if( !read_file(metapath, meta) ) { fprintf(stderr, "mp2: cannot read '%s'\n", metapath); return 1; }

  if( meta.n<8+20||memcmp(meta.p(), MP2_MAGIC, 8)!=0 ) {
    fprintf(stderr, "mp2: '%s' is not a valid meta file\n", metapath);
    return 1;
  }
  size_t pos = 8;
  int G   = (int)get32(meta.p()+pos); pos += 4;
  (void)get32(meta.p()+pos);          pos += 4;   // mp3 bitrate index (informational)
  uint nfr = get32(meta.p()+pos);     pos += 4;
  int free_format_bytes = (int)get32(meta.p()+pos); pos += 4;
  uint flags = get32(meta.p()+pos);   pos += 4;

  Section sgap, shdr, scrc, sA, sS, sF, stail, shi;
  if( !get_section(meta, pos, sgap)||!get_section(meta, pos, shdr)||!get_section(meta, pos, scrc)
    ||!get_section(meta, pos, sA)||!get_section(meta, pos, sS)||!get_section(meta, pos, sF)
    ||!get_section(meta, pos, stail)||!get_section(meta, pos, shi) ) {
    fprintf(stderr, "mp2: truncated meta file\n");
    return 1;
  }
  if( G!=2&&G!=4&&G!=6&&G!=12 ) { fprintf(stderr, "mp2: bad granule count %d\n", G); return 1; }
  if( shdr.n<(qword)nfr*4 ) { fprintf(stderr, "mp2: truncated header section\n"); return 1; }

  const int PPG = 12/G;
  const int VPS = 3*PPG;

  // ---- headers come as byte planes
  std::vector<byte> hdrs((size_t)nfr*4);
  for( uint i = 0; i<nfr; i++ ) for( int b = 0; b<4; b++ ) hdrs[i*4+b] = shdr.p[(qword)b*nfr+i];
  std::vector<byte> crcs;
  {
    size_t nc = (size_t)scrc.n/2;
    crcs.resize(nc*2);
    for( size_t i = 0; i<nc; i++ ) { crcs[i*2] = scrc.p[i]; crcs[i*2+1] = scrc.p[nc+i]; }
  }

  // ---- locate every slot's bucket inside the bit_alloc / scfsi / scalefactor
  //      streams.  The bucket sizes follow from the headers, then from the
  //      allocations, then from the scfsi values, so this takes three counting
  //      walks over the frame list before the real one.
  qword offA[NSLOT+1], offS[NSLOT+1], offF[NSLOT+1];
  qword curA[NSLOT], curS[NSLOT], curF[NSLOT];
  std::vector<byte> vA, vS, vF;
  {
    qword cnt[NSLOT];
    memset(cnt, 0, sizeof(cnt));
    for( uint f = 0; f<nfr; f++ ) {
      L2Layout L;
      l2_layout(&hdrs[f*4], L);
      for( int sb = 0; sb<L.total_bands; sb++ ) {
        cnt[2*sb]++;
        if( sb<L.stereo_bands ) cnt[2*sb+1]++;
      }
    }
    offA[0] = 0;
    for( int s = 0; s<NSLOT; s++ ) offA[s+1] = offA[s]+cnt[s];
    if( !unpack_stream(sA.p, sA.n, offA[NSLOT], 4, vA) ) {
      fprintf(stderr, "mp2: bit_alloc section size mismatch\n");
      return 1;
    }

    memcpy(curA, offA, sizeof(curA));
    memset(cnt, 0, sizeof(cnt));
    for( uint f = 0; f<nfr; f++ ) {
      L2Layout L;
      byte code[64], ba[64], bas[64];
      l2_layout(&hdrs[f*4], L);
      memset(code, 0, sizeof(code));
      for( int sb = 0; sb<L.total_bands; sb++ ) {
        code[2*sb] = vA[(size_t)curA[2*sb]++];
        if( sb<L.stereo_bands ) code[2*sb+1] = vA[(size_t)curA[2*sb+1]++];
      }
      l2_decode_alloc(L, code, ba, bas);
      for( int j = 0; j<2*L.total_bands; j++ ) if( ba[j] ) cnt[j]++;
    }
    offS[0] = 0;
    for( int s = 0; s<NSLOT; s++ ) offS[s+1] = offS[s]+cnt[s];
    if( !unpack_stream(sS.p, sS.n, offS[NSLOT], 2, vS) ) {
      fprintf(stderr, "mp2: scfsi section size mismatch\n");
      return 1;
    }

    memcpy(curA, offA, sizeof(curA));
    memcpy(curS, offS, sizeof(curS));
    memset(cnt, 0, sizeof(cnt));
    for( uint f = 0; f<nfr; f++ ) {
      L2Layout L;
      byte code[64], ba[64], bas[64];
      l2_layout(&hdrs[f*4], L);
      memset(code, 0, sizeof(code));
      for( int sb = 0; sb<L.total_bands; sb++ ) {
        code[2*sb] = vA[(size_t)curA[2*sb]++];
        if( sb<L.stereo_bands ) code[2*sb+1] = vA[(size_t)curA[2*sb+1]++];
      }
      l2_decode_alloc(L, code, ba, bas);
      for( int j = 0; j<2*L.total_bands; j++ ) if( ba[j] ) cnt[j] += (qword)scf_count(vS[(size_t)curS[j]++]);
    }
    offF[0] = 0;
    for( int s = 0; s<NSLOT; s++ ) offF[s+1] = offF[s]+cnt[s];
    if( flags&1 ) {
      if( !bmppath ) { fprintf(stderr, "mp2: this meta needs the scalefactor bmp as a 4th argument\n"); return 1; }
      if( !read_scf_bmp(bmppath, offF[NSLOT], vF) ) {
        fprintf(stderr, "mp2: cannot read scalefactors from '%s'\n", bmppath);
        return 1;
      }
    } else {
      if( offF[NSLOT]!=sF.n ) { fprintf(stderr, "mp2: scalefactor section size mismatch\n"); return 1; }
      vF.assign(sF.p, sF.p+(size_t)sF.n);
    }

    memcpy(curA, offA, sizeof(curA));
    memcpy(curS, offS, sizeof(curS));
    memcpy(curF, offF, sizeof(curF));
  }

  // gaps
  std::vector< std::pair<uint, std::pair<const byte*, qword> > > gaps;
  {
    if( sgap.n<4 ) { fprintf(stderr, "mp2: bad gap section\n"); return 1; }
    uint ng = get32(sgap.p);
    qword q = 4;
    for( uint i = 0; i<ng; i++ ) {
      if( q+12>sgap.n ) { fprintf(stderr, "mp2: bad gap section\n"); return 1; }
      uint idx = get32(sgap.p+q); q += 4;
      qword len = get64(sgap.p+q); q += 8;
      if( q+len>sgap.n ) { fprintf(stderr, "mp2: bad gap section\n"); return 1; }
      gaps.push_back(std::make_pair(idx, std::make_pair(sgap.p+q, len)));
      q += len;
    }
  }

  // ---- walk the mp3 once, rebuilding the continuous main data byte stream
  std::vector<byte> mdstream;
  std::vector<qword> mdstart;          // bit offset of every mp3 frame's granule data
  std::vector<GrParam> gps;
  std::vector<int> f_nslots;
  std::vector<byte> f_hdr;
  {
    size_t p = 0;
    // tolerate an ID3v2 tag some tool may have prepended
    if( mp3.n>10&&memcmp(mp3.p(), "ID3", 3)==0 ) {
      const byte* q = mp3.p()+6;
      size_t sz = ((size_t)(q[0]&0x7F)<<21)|((size_t)(q[1]&0x7F)<<14)|((size_t)(q[2]&0x7F)<<7)|(q[3]&0x7F);
      if( 10+sz<mp3.n ) p = 10+sz;
    }
    while( p+4<=mp3.n ) {
      const byte* mh = mp3.p()+p;
      if( !_hdr_valid(mh)||_HDR_GET_LAYER(mh)!=1 ) break;   // trailing tag / junk
      int mpeg1 = _HDR_TEST_MPEG1(mh) ? 1 : 0;
      int nch = _HDR_IS_MONO(mh) ? 1 : 2;
      int nslots = (mpeg1 ? 2 : 1)*nch;
      int si = side_info_bytes(mpeg1, nch);
      int fs = _hdr_frame_bytes(mh, 0)+_hdr_padding(mh);
      if( fs<4+si||p+(size_t)fs>mp3.n ) break;
      BitReader rs;
      rs.init(mh+4, (qword)si);
      GrParam slot[4];
      int begin = read_side_info(rs, mpeg1, nch, slot, nslots);
      qword s = (qword)mdstream.size();
      if( (qword)begin>s ) { fprintf(stderr, "mp2: bit reservoir underflow\n"); return 1; }
      mdstart.push_back((s-(qword)begin)*8);
      for( int i = 0; i<nslots; i++ ) gps.push_back(slot[i]);
      f_nslots.push_back(nslots);
      f_hdr.push_back(mh[0]); f_hdr.push_back(mh[1]); f_hdr.push_back(mh[2]); f_hdr.push_back(mh[3]);
      mdstream.insert(mdstream.end(), mh+4+si, mh+fs);
      p += (size_t)fs;
    }
    mdstream.resize(mdstream.size()+16, 0);   // slack for peek32()
  }
  BitReader rmd;
  rmd.init(mdstream.empty() ? (const byte*)"" : &mdstream[0],
           mdstream.size() ? mdstream.size()-16 : 0);

  BitReader rtail, rhi;
  rtail.init(stail.p, stail.n);
  rhi.init(shi.p, shi.n);
  size_t crc_pos = 0;

  std::vector<byte> out;
  size_t mp3frame = 0, slot_pos = 0;
  size_t gapi = 0;

  for( uint fi = 0; fi<=nfr; fi++ ) {
    while( gapi<gaps.size()&&gaps[gapi].first==fi ) {
      out.insert(out.end(), gaps[gapi].second.first, gaps[gapi].second.first+gaps[gapi].second.second);
      gapi++;
    }
    if( fi==nfr ) break;

    const byte* hdr = &hdrs[(size_t)fi*4];
    int mpeg1 = _HDR_TEST_MPEG1(hdr) ? 1 : 0;
    int nch = _HDR_IS_MONO(hdr) ? 1 : 2;
    int hb = 4+(_HDR_IS_CRC(hdr) ? 2 : 0);
    int fbytes = mp2_frame_size(hdr, free_format_bytes);
    if( fbytes<=hb ) { fprintf(stderr, "mp2: bad frame size in meta\n"); return 1; }

    // ---- bit_alloc / scfsi / scalefactors, pulled from their slot buckets
    L2Info li;
    byte scf[3*64];
    {
      l2_layout(hdr, li.lay);
      const L2Layout &L = li.lay;
      memset(li.code, 0, sizeof(li.code));
      for( int sb = 0; sb<L.total_bands; sb++ ) {
        if( curA[2*sb]>=offA[2*sb+1] ) { fprintf(stderr, "mp2: bit_alloc stream exhausted\n"); return 1; }
        li.code[2*sb] = vA[(size_t)curA[2*sb]++];
        if( sb<L.stereo_bands ) li.code[2*sb+1] = vA[(size_t)curA[2*sb+1]++];
      }
      l2_decode_alloc(L, li.code, li.ba, li.bas);
      memset(li.scfsi, 6, sizeof(li.scfsi));
      for( int j = 0; j<2*L.total_bands; j++ ) if( li.ba[j] ) li.scfsi[j] = vS[(size_t)curS[j]++];
      int k = 0;
      for( int j = 0; j<2*L.total_bands; j++ ) {
        if( !li.ba[j] ) continue;
        int n = scf_count(li.scfsi[j]);
        for( int i = 0; i<n; i++ ) scf[k++] = vF[(size_t)curF[j]++];
      }
      li.sample_bits = l2_sample_bits(li);
      li.prefix_bits = 0;
      for( int sb = 0; sb<L.total_bands; sb++ ) li.prefix_bits += L.width[sb]*(sb<L.stereo_bands ? 2u : 1u);
      for( int j = 0; j<2*L.total_bands; j++ ) if( li.ba[j] ) li.prefix_bits += 2u+6u*(uint)scf_count(li.scfsi[j]);
    }

    // ---- read the matching mp3 frames
    memset(g_vals, 0, sizeof(g_vals));
    int gr_per_frame = mpeg1 ? 2 : 1;
    int nslots = gr_per_frame*nch;
    int nmp3 = G/gr_per_frame;
    int B[23];
    for( int mf = 0; mf<nmp3; mf++ ) {
      if( mp3frame>=mdstart.size() ) { fprintf(stderr, "mp2: mp3 stream too short\n"); return 1; }
      const byte* mh = &f_hdr[mp3frame*4];
      if( f_nslots[mp3frame]!=nslots ) {
        fprintf(stderr, "mp2: mp3 frame %llu does not match the meta file\n",
                (unsigned long long)mp3frame);
        return 1;
      }
      sfb_bounds(mh, B);
      rmd.pos = mdstart[mp3frame];
      for( int s = 0; s<nslots; s++ ) {
        int gr = s/nch, ch = s%nch;
        int gi = mf*gr_per_frame+gr;
        const GrParam &gp = gps[slot_pos+s];
        qword start = rmd.pos;
        read_granule(rmd, gp, B, g_vals[ch][gi]);
        rmd.pos = start+gp.part23;
      }
      slot_pos += nslots;
      mp3frame++;
    }

    // ---- rebuild the Layer II frame
    BitWriter w;
    w.put(hdr[0], 8); w.put(hdr[1], 8); w.put(hdr[2], 8); w.put(hdr[3], 8);
    if( hb==6 ) {
      if( crc_pos+2>crcs.size() ) { fprintf(stderr, "mp2: truncated crc section\n"); return 1; }
      w.put(crcs[crc_pos], 8);
      w.put(crcs[crc_pos+1], 8);
      crc_pos += 2;
    }
    l2_write_prefix(w, li, scf);

    int tb = li.lay.total_bands;
    for( int part = 0; part<12; part++ ) {
      int gi = part/PPG, lp = part%PPG;
      for( int i = 0; i<2*tb; i++ ) {
        int ba = li.bas[i];
        if( !ba ) continue;
        int sb = i>>1, ch = i&1;
        const int* src = &g_vals[ch][gi][sb*VPS+lp*3];
        if( ba<17 ) {
          if( ba<15 ) {
            int half = (1<<(ba-1))-1;
            for( int k = 0; k<3; k++ ) w.put((uint)(src[k]+half), ba);
          } else {
            for( int k = 0; k<3; k++ ) {
              uint hi = rhi.get(ba-14);
              w.put((hi<<14)|(uint)(src[k]+8191), ba);
            }
          }
        } else {
          int mod = ba_mod(ba), h = mod/2;
          uint code = (uint)(src[0]+h)+(uint)mod*((uint)(src[1]+h)+(uint)mod*(uint)(src[2]+h));
          w.put(code, ba_nbits(ba));
        }
      }
    }
    qword used = (qword)hb*8+li.prefix_bits+li.sample_bits;
    copy_bits(rtail, w, (qword)fbytes*8-used);
    w.align0();
    if( (int)w.size()!=fbytes ) {
      fprintf(stderr, "mp2: frame %u rebuilt to %u bytes, expected %d\n", fi, (uint)w.size(), fbytes);
      return 1;
    }
    out.insert(out.end(), w.buf.begin(), w.buf.end());
  }

  if( rtail.over||rhi.over ) {
    fprintf(stderr, "mp2: meta stream exhausted\n");
    return 1;
  }
  if( !write_file(outpath, out.empty() ? (const void*)"" : &out[0], out.size()) ) {
    fprintf(stderr, "mp2: cannot write '%s'\n", outpath);
    return 1;
  }
  printf("mp2 frames : %u\n", nfr);
  printf("output.mp2 : %llu bytes\n", (unsigned long long)out.size());
  return 0;
}

//---------------------------------------------------------------------------

int main(int argc, char** argv) {
  init_huff();
  if( (argc==5||argc==6)&&(argv[1][0]=='c'||argv[1][0]=='C')&&argv[1][1]==0 )
    return compress(argv[2], argv[3], argv[4], argc==6 ? argv[5] : 0);
  if( (argc==5||argc==6)&&(argv[1][0]=='d'||argv[1][0]=='D')&&argv[1][1]==0 )
    return decompress(argv[2], argv[3], argv[4], argc==6 ? argv[5] : 0);
  fprintf(stderr,
    "mp2 - lossless mp2 <-> mp3 container converter\n"
    "usage: mp2 c input.mp2 output.mp3 output.meta [output.bmp]\n"
    "       mp2 d input.mp3 input.meta output.mp2 [input.bmp]\n"
    "\n"
    "with output.bmp given, the scalefactors go there as an 8-bit image instead\n"
    "of into the meta, for an image compressor such as BMF to take.\n");
  return 1;
}
