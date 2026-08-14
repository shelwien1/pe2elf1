// mp3.cpp - streaming mp3 -> raw s16 PCM via dr_mp3 pull API, own file I/O
// build: g++ -O2 -o mp3 mp3.cpp     (clang++ / MinGW g++ also fine)
#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "mp3_A.h"

typedef uint64_t qword;
typedef uint32_t uint;
typedef uint8_t  byte;
typedef uint16_t word;

#if defined(_WIN32)
  #define FSEEK64 _fseeki64
  #define FTELL64 _ftelli64
#else
  #define FSEEK64 fseeko
  #define FTELL64 ftello
#endif

#define CHUNK 8192  // PCM frames per decode call

// --- dr_mp3 pulls bytes from our FILE* through these callbacks ---
static size_t on_read(void* u, void* buf, size_t n) {
  return fread(buf, 1, n, (FILE*)u);
}

MP3 mp3;
static _int16 buf[CHUNK*2];

int main(int argc, char** argv) {
  if (argc != 3) {
    fprintf(stderr, "usage: %s input.mp3 output.pcm\n", argv[0]);
    return 1;
  }

  FILE* in = fopen(argv[1], "rb");
  if (!in) {
    fprintf(stderr, "error: cannot open '%s'\n", argv[1]);
    return 1;
  }

  //if (!mp3._init(on_read, in)) {
  if (!mp3._init(on_read, in)) {
    fprintf(stderr, "error: not a valid mp3 stream\n");
    fclose(in);
    return 1;
  }

  FILE* out = fopen(argv[2], "wb");
  if (!out) {
    fprintf(stderr, "error: cannot create '%s'\n", argv[2]);
    fclose(in);
    return 1;
  }

  uint ch = mp3.channels;
  uint sr = mp3.sampleRate;
  printf("channels   : %u\n", ch);
  printf("sample rate: %u Hz\n", sr);
  printf("format     : s16le, interleaved\n");

  qword total = 0;
  for (;;) {
    qword n = mp3._read_pcm_frames_s16(CHUNK, buf);
    if (n == 0) break;
    fwrite(buf, sizeof(_int16), (size_t)(n * ch), out);
    total += n;
  }

  printf("decoded    : %llu frames (%.2f s, %llu bytes)\n",
         (unsigned long long)total,
         sr ? (double)total / sr : 0.0,
         (unsigned long long)(total * ch * sizeof(_int16)));

  fclose(out);
  fclose(in);
  return 0;
}
