// sfimg.cpp - dump the Layer II scalefactors of an mp2 as an 8-bit BMP.
//
// Columns are subbands (channel 0's, then channel 1's), rows are the three
// scalefactor parts of each frame, so the image is subband across and time down.
// Values a frame does not transmit are carried forward from the row above, which
// is what the decoder uses anyway and what a 2D predictor wants to see.
//
// This is a diagnostic, not part of the pipeline: the 2D framing was measured
// and loses to the slot-major byte stream the meta already uses.  Against
// 284 452 for that stream under xz -9e:
//
//   expanded image, PNG (Up filter)          486 343
//   expanded image, xz                       412 308
//   expanded image, Up residual + xz         393 676
//   expanded image, MED (2D) residual + xz   414 860
//   dense reshape, PNG (Up filter)           315 541   <- best image result
//   dense reshape, WebP lossless             389 782
//
// Two reasons.  Only one of the two dimensions carries anything: a left
// (cross-subband) residual is the worst of the lot and 2D MED is worse than
// vertical alone, so neighbouring subbands tell you nothing a subband's own
// past does not.  And the grid has to be 3.6x larger than the data, because
// scfsi transmits only 1 to 3 of each subband's 3 scalefactors and most
// subbands are unallocated.  A dense reshape avoids that but is then just the
// meta stream delta-coded, where deflate and WebP both lose to LZMA.
//
// (WebP also caps each dimension at 16 383 pixels; this file is 41 481 rows.)
//
// build: g++ -O2 -std=gnu++17 -I.. -o sfimg sfimg.cpp

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <vector>
#include "mp3_A.h"

typedef uint8_t byte;
typedef uint32_t uint;

static const byte BATAB[] = {
  0,17,3,4,5,6,7,8,9,10,11,12,13,14,15,16, 0,17,18,3,19,4,5,6,7,8,9,10,11,12,13,16,
  0,17,18,3,19,4,5,16, 0,17,18,16, 0,17,18,19,4,5,6,7,8,9,10,11,12,13,14,15,
  0,17,18,3,19,4,5,6,7,8,9,10,11,12,13,14, 0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };

static int scf_count(int s) { int m = 4+((19>>s)&3); return ((m>>2)&1)+((m>>1)&1)+(m&1); }

int main(int argc, char** argv) {
  if( argc!=3 ) { fprintf(stderr, "usage: sfimg in.mp2 out.bmp\n"); return 1; }
  FILE* f = fopen(argv[1], "rb");
  if( !f ) return 1;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  std::vector<byte> b((size_t)n+16, 0);
  if( fread(&b[0], 1, (size_t)n, f)!=(size_t)n ) return 1;
  fclose(f);

  int W = 0, tb0 = 0;
  std::vector<byte> rows;                 // W bytes per row, 3 rows per frame
  std::vector<byte> prev;                 // last known value per column
  long nfr = 0, nval = 0;

  size_t p = 0;
  while( p+4<=(size_t)n ) {
    const byte* h = &b[0]+p;
    if( !(h[0]==0xFF&&_hdr_valid(h)&&_HDR_GET_LAYER(h)==2) ) { p++; continue; }
    int fs = _hdr_frame_bytes(h, 0)+_hdr_padding(h);
    if( fs<8||p+(size_t)fs>(size_t)n ) { p++; continue; }
    int hb = 4+(_HDR_IS_CRC(h) ? 2 : 0);

    _L12_scale_info sci;
    const _L12_subband_alloc* sa = sci._L12_subband_alloc_table(h);
    int tb = sci.total_bands, sbnd = sci.stereo_bands;
    if( !tb0 ) { tb0 = tb; W = 2*tb; prev.assign(W, 0); }
    if( tb!=tb0 ) { fprintf(stderr, "sfimg: subband count changes, unsupported\n"); return 1; }

    _bs bs;
    bs._bs_init(h+hb, fs-hb);
    byte alloc[64], scfsi[64];
    int k = 0, w = 0;
    const byte* t = BATAB;
    memset(alloc, 0, sizeof(alloc));
    for( int i = 0; i<tb; i++ ) {
      if( i==k ) { k += sa->band_count; w = sa->code_tab_width; t = BATAB+sa->tab_offset; sa++; }
      byte v = t[bs._bs_get_bits(w)];
      alloc[2*i] = v;
      if( i<sbnd ) v = t[bs._bs_get_bits(w)];
      alloc[2*i+1] = sbnd ? v : 0;
    }
    for( int j = 0; j<2*tb; j++ ) scfsi[j] = alloc[j] ? (byte)bs._bs_get_bits(2) : 6;

    byte r0[64], r1[64], r2[64];
    for( int c = 0; c<W; c++ ) { r0[c] = r1[c] = r2[c] = prev[c]; }
    for( int j = 0; j<2*tb; j++ ) {
      if( !alloc[j] ) continue;
      int sb = j>>1, ch = j&1, col = ch*tb+sb;
      int s = scfsi[j], cnt = scf_count(s);
      byte v[3];
      for( int i = 0; i<cnt; i++ ) { v[i] = (byte)bs._bs_get_bits(6); nval++; }
      byte a, bb, cc;
      if( s==0 )      { a = v[0]; bb = v[1]; cc = v[2]; }
      else if( s==1 ) { a = bb = v[0]; cc = v[1]; }        // one value covers parts 0 and 1
      else if( s==2 ) { a = bb = cc = v[0]; }
      else            { a = v[0]; bb = cc = v[1]; }        // one value covers parts 1 and 2
      r0[col] = a; r1[col] = bb; r2[col] = cc;
      prev[col] = cc;
    }
    rows.insert(rows.end(), r0, r0+W);
    rows.insert(rows.end(), r1, r1+W);
    rows.insert(rows.end(), r2, r2+W);
    nfr++;
    p += fs;
  }

  int H = (int)(rows.size()/W);
  int stride = (W+3)&~3;
  uint pixoff = 14+40+256*4;
  uint fsz = pixoff+(uint)stride*H;
  std::vector<byte> out(fsz, 0);
  byte* o = &out[0];
  o[0] = 'B'; o[1] = 'M';
  memcpy(o+2, &fsz, 4);
  memcpy(o+10, &pixoff, 4);
  uint hs = 40; memcpy(o+14, &hs, 4);
  int wi = W, he = H; memcpy(o+18, &wi, 4); memcpy(o+22, &he, 4);
  uint16_t planes = 1, bpp = 8; memcpy(o+26, &planes, 2); memcpy(o+28, &bpp, 2);
  uint ncol = 256; memcpy(o+46, &ncol, 4); memcpy(o+50, &ncol, 4);
  for( int i = 0; i<256; i++ ) { o[54+i*4] = o[55+i*4] = o[56+i*4] = (byte)i; }
  for( int y = 0; y<H; y++ ) {                       // BMP is bottom-up
    memcpy(o+pixoff+(size_t)(H-1-y)*stride, &rows[(size_t)y*W], W);
  }
  FILE* fo = fopen(argv[2], "wb");
  fwrite(o, 1, fsz, fo);
  fclose(fo);
  fprintf(stderr, "frames %ld  values %ld  image %dx%d  bmp %u bytes\n", nfr, nval, W, H, fsz);
  return 0;
}
