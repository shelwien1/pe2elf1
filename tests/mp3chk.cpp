// mp3chk.cpp - walk an mp3 with the mp3_A.h decoder and report how cleanly it
// parses.  Used to prove that the stream produced by "mp2 c" is a real Layer III
// bitstream: every byte must belong to a frame that decodes without a resync.
//
// build: g++ -O2 -std=gnu++17 -I.. -o mp3chk mp3chk.cpp

#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <vector>

#include "../mp3_A.h"

typedef uint8_t byte;

static mp3dec dec;
static _int16 pcm[_MAX_SAMPLES_PER_FRAME];

int main(int argc, char** argv) {
  if( argc!=2 ) { fprintf(stderr, "usage: mp3chk file.mp3\n"); return 1; }
  FILE* f = fopen(argv[1], "rb");
  if( !f ) { fprintf(stderr, "mp3chk: cannot open '%s'\n", argv[1]); return 1; }
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  std::vector<byte> b((size_t)n+16, 0);
  if( n&&fread(&b[0], 1, (size_t)n, f)!=(size_t)n ) { fclose(f); return 1; }
  fclose(f);

  dec.mp3dec_init();
  size_t pos = 0;
  long frames = 0, skipped = 0, failed = 0;
  int layer = 0, ch = 0, sr = 0;
  while( pos<(size_t)n ) {
    mp3dec_frame_info info;
    memset(&info, 0, sizeof(info));
    int samples = dec.mp3dec_decode_frame(&b[0]+pos, (int)((size_t)n-pos), pcm, &info);
    if( info.frame_bytes<=0 ) break;
    if( samples>0 ) {
      int fb = _hdr_frame_bytes(&b[0]+pos, dec.free_format_bytes)+_hdr_padding(&b[0]+pos);
      if( info.frame_bytes!=fb ) skipped += info.frame_bytes-fb;
      frames++;
      layer = info.layer;
      ch = info.channels;
      sr = info.sample_rate;
    } else {
      failed++;
      skipped += info.frame_bytes;
    }
    pos += (size_t)info.frame_bytes;
  }
  printf("frames %ld layer %d ch %d sr %d consumed %llu/%llu skipped %ld failed %ld\n",
         frames, layer, ch, sr, (unsigned long long)pos, (unsigned long long)n, skipped, failed);
  return (skipped||failed||pos!=(size_t)n) ? 2 : 0;
}
