// mkmp2.cpp - synthetic MPEG audio Layer II stream generator (test tool)
//
// Produces structurally valid Layer II frames with random bit allocations,
// scalefactors and subband samples, for round-trip testing of mp2.cpp over the
// whole header/allocation configuration space.  The audio is noise; only the
// bitstream syntax matters.
//
//   mkmp2 out.mp2 <b1hex> <b2hex> <b3hex> <nframes> <seed> [junk]
//     b1..b3   header bytes 1..3 (byte 0 is always 0xFF); padding bit is set
//              per frame by the generator
//     junk     0 none, 1 leading+trailing junk, 2 also junk between frames
//
// build: g++ -O2 -std=gnu++17 -I.. -o mkmp2 mkmp2.cpp

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <vector>

#include "../mp3_A.h"
#include "../mp3_htabs.h"

typedef uint8_t byte;
typedef uint32_t uint;
typedef uint64_t qword;

static qword g_seed = 12345;
static uint rnd() {
  g_seed ^= g_seed<<13;
  g_seed ^= g_seed>>7;
  g_seed ^= g_seed<<17;
  return (uint)(g_seed>>16);
}
static uint rnd_n(uint n) { return n ? rnd()%n : 0; }

struct BW {
  std::vector<byte> buf;
  qword acc; int accn; qword nbits;
  BW() : acc(0), accn(0), nbits(0) {}
  void put(uint v, int n) {
    if( n<=0 ) return;
    acc = (acc<<n)|(qword)(v&(((uint)1<<n)-1));
    accn += n; nbits += n;
    while( accn>=8 ) { accn -= 8; buf.push_back((byte)(acc>>accn)); }
  }
  void align0() { if( accn ) put(0, 8-accn); }
};

static int ba_nbits(int ba) { return ba<17 ? ba : (ba==17 ? 5 : (ba==18 ? 7 : 10)); }

struct Layout {
  int total_bands, stereo_bands;
  int nbal[32];
  const byte* tab[32];
  int codes[64];       // raw allocation codes, [2*sb+ch]
  int ba[64];          // decoded, before the ch1 zeroing
  int bas[64];         // decoded, after the ch1 zeroing (used for samples)
  int scfcod[64];
};

static void layout_init(const byte* hdr, Layout &L) {
  _L12_scale_info sci;
  const _L12_subband_alloc* sa = sci._L12_subband_alloc_table(hdr);
  L.total_bands = sci.total_bands;
  L.stereo_bands = sci.stereo_bands;
  int k = 0, w = 0;
  const byte* t = g_bitalloc_code_tab_;
  for( int i = 0; i<L.total_bands; i++ ) {
    if( i==k ) { k += sa->band_count; w = sa->code_tab_width; t = g_bitalloc_code_tab_+sa->tab_offset; sa++; }
    L.nbal[i] = w;
    L.tab[i] = t;
  }
  memset(L.codes, 0, sizeof(L.codes));
}

static void layout_decode(Layout &L) {
  for( int sb = 0; sb<L.total_bands; sb++ ) {
    int b0 = L.tab[sb][L.codes[2*sb]];
    L.ba[2*sb] = b0;
    int b1;
    if( sb<L.stereo_bands ) b1 = L.tab[sb][L.codes[2*sb+1]];
    else b1 = b0;
    L.ba[2*sb+1] = L.stereo_bands ? b1 : 0;
    L.bas[2*sb] = b0;
    L.bas[2*sb+1] = (sb<L.stereo_bands&&L.stereo_bands) ? b1 : 0;
  }
}

static int layout_bits(Layout &L) {
  layout_decode(L);
  int bits = 0;
  for( int sb = 0; sb<L.total_bands; sb++ ) bits += L.nbal[sb]*(sb<L.stereo_bands ? 2 : 1);
  for( int i = 0; i<2*L.total_bands; i++ ) if( L.ba[i] ) bits += 2;
  for( int i = 0; i<2*L.total_bands; i++ ) {
    if( !L.ba[i] ) continue;
    int mask = 4+((19>>L.scfcod[i])&3);
    int n = ((mask>>2)&1)+((mask>>1)&1)+(mask&1);
    bits += 6*n;
  }
  for( int i = 0; i<2*L.total_bands; i++ ) {
    int ba = L.bas[i];
    if( !ba ) continue;
    bits += 12*(ba<17 ? 3*ba : ba_nbits(ba));
  }
  return bits;
}

int main(int argc, char** argv) {
  if( argc<7 ) {
    fprintf(stderr, "usage: mkmp2 out.mp2 b1hex b2hex b3hex nframes seed [junk]\n");
    return 1;
  }
  byte hdr[4];
  hdr[0] = 0xFF;
  hdr[1] = (byte)strtoul(argv[2], 0, 16);
  hdr[2] = (byte)strtoul(argv[3], 0, 16);
  hdr[3] = (byte)strtoul(argv[4], 0, 16);
  int nframes = atoi(argv[5]);
  g_seed = (qword)strtoull(argv[6], 0, 10)*2654435761u+88172645463325252ull;
  int junk = argc>7 ? atoi(argv[7]) : 0;

  if( !_hdr_valid(hdr)||_HDR_GET_LAYER(hdr)!=2 ) {
    fprintf(stderr, "mkmp2: %02x%02x%02x%02x is not a Layer II header\n", hdr[0], hdr[1], hdr[2], hdr[3]);
    return 1;
  }
  int free_format = _HDR_IS_FREE_FORMAT(hdr);

  std::vector<byte> out;
  if( junk>=1 ) {
    int n = 3+(int)rnd_n(40);
    for( int i = 0; i<n; i++ ) out.push_back((byte)rnd_n(256));
  }

  for( int f = 0; f<nframes; f++ ) {
    byte h[4];
    memcpy(h, hdr, 4);
    if( !free_format ) h[2] = (byte)((h[2]&~2)|(rnd_n(2)<<1));
    int fbytes = free_format ? 700 : _hdr_frame_bytes(h, 0)+_hdr_padding(h);
    int crc = _HDR_IS_CRC(h) ? 2 : 0;
    int avail = (fbytes-4-crc)*8;

    Layout L;
    layout_init(h, L);
    for( int i = 0; i<64; i++ ) L.scfcod[i] = (int)rnd_n(4);
    if( layout_bits(L)>avail ) { fprintf(stderr, "mkmp2: frame too small for this layout\n"); return 2; }

    // fill the frame with as much allocation as fits, in random subband order
    int nslots = 2*L.total_bands;
    std::vector<int> ord;
    for( int sb = 0; sb<L.total_bands; sb++ ) {
      ord.push_back(2*sb);
      if( sb<L.stereo_bands ) ord.push_back(2*sb+1);
    }
    for( size_t i = ord.size(); i>1; i-- ) { size_t j = rnd_n((uint)i); int t = ord[i-1]; ord[i-1] = ord[j]; ord[j] = t; }
    for( size_t i = 0; i<ord.size(); i++ ) {
      int slot = ord[i], sb = slot>>1;
      int ncodes = 1<<L.nbal[sb];
      int want = (int)rnd_n((uint)ncodes);
      for( int c = want; c>=0; c-- ) {
        L.codes[slot] = c;
        if( layout_bits(L)<=avail ) break;
        L.codes[slot] = 0;
      }
    }
    layout_decode(L);
    int used = layout_bits(L);
    (void)nslots;

    BW w;
    w.put(h[0], 8); w.put(h[1], 8); w.put(h[2], 8); w.put(h[3], 8);
    if( crc ) { w.put(rnd_n(256), 8); w.put(rnd_n(256), 8); }
    for( int sb = 0; sb<L.total_bands; sb++ ) {
      w.put((uint)L.codes[2*sb], L.nbal[sb]);
      if( sb<L.stereo_bands ) w.put((uint)L.codes[2*sb+1], L.nbal[sb]);
    }
    for( int i = 0; i<2*L.total_bands; i++ ) if( L.ba[i] ) w.put((uint)L.scfcod[i], 2);
    for( int i = 0; i<2*L.total_bands; i++ ) {
      if( !L.ba[i] ) continue;
      int mask = 4+((19>>L.scfcod[i])&3);
      for( int m = 4; m; m >>= 1 ) if( mask&m ) w.put(rnd_n(64), 6);
    }
    for( int part = 0; part<12; part++ ) {
      for( int i = 0; i<2*L.total_bands; i++ ) {
        int ba = L.bas[i];
        if( !ba ) continue;
        if( ba<17 ) { for( int k = 0; k<3; k++ ) w.put(rnd(), ba); }
        else w.put(rnd(), ba_nbits(ba));
      }
    }
    int tail = (fbytes-4-crc)*8-used;
    while( tail>=24 ) { w.put(rnd(), 24); tail -= 24; }
    if( tail>0 ) w.put(rnd(), tail);
    w.align0();
    if( (int)w.buf.size()!=fbytes ) { fprintf(stderr, "mkmp2: internal size error\n"); return 1; }
    out.insert(out.end(), w.buf.begin(), w.buf.end());

    if( junk>=2&&f+1<nframes&&rnd_n(4)==0 ) {
      int n = 1+(int)rnd_n(20);
      for( int i = 0; i<n; i++ ) {
        byte b = (byte)rnd_n(256);
        if( b==0xFF ) b = 0xFE;                 // do not fabricate extra syncs
        out.push_back(b);
      }
    }
  }
  if( junk>=1 ) {
    int n = 3+(int)rnd_n(40);
    for( int i = 0; i<n; i++ ) {
      byte b = (byte)rnd_n(256);
      if( b==0xFF ) b = 0xFE;
      out.push_back(b);
    }
  }

  FILE* fo = fopen(argv[1], "wb");
  if( !fo ) { fprintf(stderr, "mkmp2: cannot create '%s'\n", argv[1]); return 1; }
  if( !out.empty() ) fwrite(&out[0], 1, out.size(), fo);
  fclose(fo);
  return 0;
}
