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
  // opcode: 1-byte, 0F, 0F38/3A, VEX (C5/C4), EVEX (62), XOP (8F), or APX REX2 (D5)
  uint32_t kind = rn(18);
  if (kind < 5) {                       // one-byte opcode
    buf[p++] = (uint8_t)rn(256);
  } else if (kind < 8) {                // 0F two-byte
    buf[p++] = 0x0F; buf[p++] = (uint8_t)rn(256);
  } else if (kind < 10) {               // 0F 38 / 0F 3A three-byte
    buf[p++] = 0x0F; buf[p++] = (rn(2) ? 0x38 : 0x3A); buf[p++] = (uint8_t)rn(256);
  } else if (kind == 10) {              // 2-byte VEX (C5): byte1 = R.vvvv.L.pp (map 0F)
    buf[p++] = 0xC5; buf[p++] = (uint8_t)rn(256); buf[p++] = (uint8_t)rn(256);
  } else if (kind < 13) {               // 3-byte VEX (C4): byte1 = R.X.B.mmmmm, map 1-3
    buf[p++] = 0xC4; buf[p++] = (uint8_t)((rn(8) << 5) | (1 + rn(3)));
    buf[p++] = (uint8_t)rn(256); buf[p++] = (uint8_t)rn(256);
  } else if (kind < 15) {               // EVEX (62): P0 = R.X.B.R'.0.mmm; maps 0-7
    buf[p++] = 0x62; buf[p++] = (uint8_t)((rn(16) << 4) | rn(8));  // incl. APX map 4
    buf[p++] = (uint8_t)rn(256); buf[p++] = (uint8_t)rn(256); buf[p++] = (uint8_t)rn(256);
  } else if (kind < 16) {               // XOP (8F): byte1 = R.X.B.mmmmm, map 8-10
    buf[p++] = 0x8F; buf[p++] = (uint8_t)((rn(8) << 5) | (8 + rn(3)));
    buf[p++] = (uint8_t)rn(256); buf[p++] = (uint8_t)rn(256);
  } else {                              // APX REX2 (D5): payload (M0.R4.X4.B4.W.R.X.B)
    buf[p++] = 0xD5; buf[p++] = (uint8_t)rn(256);    // + opcode (M0 picks 1-byte/0F map)
    buf[p++] = (uint8_t)rn(256);
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

  // Every accepted decode must re-encode to the same bytes -- legacy AND
  // VEX/EVEX/XOP. The counts are split only for diagnosis (a "vex" failure is on
  // the C++ VEX/EVEX/XOP path, a "legacy" failure on the corpus-driven core); both
  // are real bijection bugs and both fail the run.
  uint64_t accepted = 0, legacy_fails = 0, vex_fails = 0;
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
    if (ok) continue;
    bool is_vex = (d.insn.mnem == MNEM_VEX);
    if (is_vex) vex_fails++; else legacy_fails++;
    if (legacy_fails + vex_fails <= 20) {
      printf("ROUND-TRIP FAIL%s in:", is_vex ? " (vex)" : "");
      for (size_t i = 0; i < len; ++i) printf(" %02x", buf[i]);
      printf("   out(%zu):", el);
      for (size_t i = 0; i < el; ++i) printf(" %02x", out[i]);
      if (is_vex) printf("   vex=%d op=%02x\n", d.insn.vex, d.insn.vex_op);
      else        printf("   mnem=%u\n", d.insn.mnem);
    }
  }
  printf("fuzz: %llu iters, %llu accepted (%.1f%%), %llu legacy failures, "
         "%llu vex/evex/xop failures\n",
         (unsigned long long)iters, (unsigned long long)accepted,
         100.0 * (double)accepted / (double)iters,
         (unsigned long long)legacy_fails, (unsigned long long)vex_fails);
  return (legacy_fails || vex_fails) ? 1 : 0;
}
