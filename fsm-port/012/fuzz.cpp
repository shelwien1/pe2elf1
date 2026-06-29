// fuzz.cpp -- bijection stress test for the corpus-driven (dis)assembler.
//
// The core invariant of this tool is a BIJECTION between the byte stream and the
// decoded x86insn_t: whenever decode_insn accepts a prefix+opcode+operand
// sequence, encode_insn must reproduce those exact bytes (canonical or not).
// This fuzzer hammers that invariant across the whole opcode space: it builds
// random-but-plausible instructions (legal prefixes, 1/2/3-byte opcodes, random
// ModR/M-SIB-disp-imm tail), decodes each, and for every accepted decode
// re-encodes and compares the consumed bytes. Any divergence is a round-trip
// bug. Pure-random bytes would mostly hit undefined opcodes and stop early, so
// the generator is biased toward decodable shapes for real coverage.
//
//   g++ -std=c++17 -O2 -Wall -Wextra -I. -o fuzz fuzz.cpp
//   ./fuzz [iterations] [seed]

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define main parser_main
#include "parser.cpp"
#undef main

// deterministic, fast PRNG (xorshift64*) so any failure is reproducible by seed
static uint64_t RNG;
static inline uint64_t rnd() {
  RNG ^= RNG >> 12; RNG ^= RNG << 25; RNG ^= RNG >> 27;
  return RNG * 0x2545F4914F6CDD1DULL;
}
static inline uint32_t rn(uint32_t n) { return (uint32_t)(rnd() % n); }

// legal stand-alone prefix bytes (the decoder's prefix run consumes these)
static const uint8_t PFX[] = {0x66, 0x67, 0xF0, 0xF2, 0xF3,
                              0x2E, 0x36, 0x3E, 0x26, 0x64, 0x65};

// build one plausible instruction into buf; returns length written
static size_t gen(uint8_t* buf) {
  size_t p = 0;
  // 0..3 prefix bytes (biased low)
  uint32_t npfx = rn(8);
  if (npfx > 3) npfx = 0;
  for (uint32_t i = 0; i < npfx; ++i) buf[p++] = PFX[rn(sizeof PFX)];
  // opcode: 1-byte, 0F, 0F38/3A, VEX (C5/C4), EVEX (62), or XOP (8F)
  uint32_t kind = rn(16);
  if (kind < 5) {                       // one-byte opcode
    buf[p++] = (uint8_t)rn(256);
  } else if (kind < 8) {                // 0F two-byte
    buf[p++] = 0x0F; buf[p++] = (uint8_t)rn(256);
  } else if (kind < 10) {               // 0F 38 / 0F 3A three-byte
    buf[p++] = 0x0F; buf[p++] = (rn(2) ? 0x38 : 0x3A); buf[p++] = (uint8_t)rn(256);
  } else if (kind == 10) {              // 2-byte VEX (C5)
    buf[p++] = 0xC5; buf[p++] = (uint8_t)(0xC0 | rn(64)); buf[p++] = (uint8_t)rn(256);
  } else if (kind < 13) {               // 3-byte VEX (C4)
    buf[p++] = 0xC4; buf[p++] = (uint8_t)(0xC0 | (1 + rn(3)) | (rn(8) << 3));
    buf[p++] = (uint8_t)rn(256); buf[p++] = (uint8_t)rn(256);
  } else if (kind < 15) {               // EVEX (62)
    buf[p++] = 0x62; buf[p++] = (uint8_t)(0xC0 | (1 + rn(3)) | (rn(8) << 3));
    buf[p++] = (uint8_t)rn(256); buf[p++] = (uint8_t)rn(256); buf[p++] = (uint8_t)rn(256);
  } else {                              // XOP (8F): map field 8..10
    buf[p++] = 0x8F; buf[p++] = (uint8_t)(0xC0 | (8 + rn(3)) | (rn(4) << 5));
    buf[p++] = (uint8_t)rn(256); buf[p++] = (uint8_t)rn(256);
  }
  // random tail: ModR/M + SIB + disp32 + imm32 worst case is 10 bytes; give extra
  // so no instruction is truncated (a short buffer makes the immediate read as 0,
  // which is a generator artifact, not a round-trip bug).
  for (int i = 0; i < 22; ++i) buf[p++] = (uint8_t)rn(256);
  return p;
}

int main(int argc, char** argv) {
  uint64_t iters = (argc > 1) ? strtoull(argv[1], 0, 0) : 5000000ULL;
  RNG = (argc > 2) ? strtoull(argv[2], 0, 0) : 0x123456789ABCDEFULL;
  if (!RNG) RNG = 1;

  uint64_t accepted = 0, fails = 0;
  uint8_t buf[48], out[48];
  for (uint64_t it = 0; it < iters; ++it) {
    memset(buf, 0, sizeof buf);
    size_t n = gen(buf);

    x86dec_t d = {};
    size_t len = decode_insn(buf, n, &d);
    if (d.insn.mnem == 0xFFFF || len == 0) continue;     // undefined: not our bytes
    accepted++;

    size_t el = encode_insn(&d.insn, out);
    bool ok = (el == len) && (memcmp(buf, out, len) == 0);
    if (!ok && ++fails <= 20) {
      printf("ROUND-TRIP FAIL  in:");
      for (size_t i = 0; i < len; ++i) printf(" %02x", buf[i]);
      printf("   out(%zu):", el);
      for (size_t i = 0; i < el; ++i) printf(" %02x", out[i]);
      printf("   mnem=%u\n", d.insn.mnem);
    }
  }
  printf("fuzz: %llu iters, %llu accepted (%.1f%%), %llu round-trip failures\n",
         (unsigned long long)iters, (unsigned long long)accepted,
         100.0 * (double)accepted / (double)iters, (unsigned long long)fails);
  return fails ? 1 : 0;
}
