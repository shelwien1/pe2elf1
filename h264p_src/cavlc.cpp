#include "cavlc.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

#include "tables.h"

namespace h264p {
namespace {

// A prefix code decoded with one table lookup of maxlen bits.
struct Vlc {
  int maxlen = 0;
  std::vector<uint16_t> tab;  // value << 5 | length; 0 = invalid code

  void add(int value, int code, int len) { codes.push_back({value, code, len}); }
  void build() {
    for (auto& c : codes)
      if (c.len > maxlen) maxlen = c.len;
    tab.assign((size_t)1 << maxlen, 0);
    for (auto& c : codes) {
      size_t base = (size_t)c.code << (maxlen - c.len);
      for (size_t i = 0; i < ((size_t)1 << (maxlen - c.len)); i++) {
        if (tab[base + i]) {
          fprintf(stderr, "h264p: internal error: VLC table is not a prefix code\n");
          abort();
        }
        tab[base + i] = (uint16_t)(c.value << 5 | c.len);
      }
    }
    codes.clear();
  }
  int read(BitReader& br) const {
    uint16_t e = tab[br.peek(maxlen)];
    if (!e) throw ParseError("invalid CAVLC code");
    br.skip(e & 31);
    return e >> 5;
  }

  struct Code {
    int value, code, len;
  };
  std::vector<Code> codes;
};

struct Tables {
  Vlc coeff_token[6];  // nC 0-1, 2-3, 4-7, >= 8, -1, -2; value TotalCoeff * 4 + TrailingOnes
  Vlc total_zeros[15], tz_dc420[3], tz_dc422[7];
  Vlc run_before[7];

  Tables() {
    for (int t = 0; t < 5; t++)
      for (int k = 0; k < 17 * 4; k++)
        if (kCoeffToken[t][k][1]) coeff_token[t].add(k, kCoeffToken[t][k][0], kCoeffToken[t][k][1]);
    for (int k = 0; k < 9 * 4; k++)
      if (kCoeffTokenChromaDc422[k][1])
        coeff_token[5].add(k, kCoeffTokenChromaDc422[k][0], kCoeffTokenChromaDc422[k][1]);
    for (int t = 0; t < 15; t++)
      for (int k = 0; k < 16; k++)
        if (kTotalZeros[t][k][1]) total_zeros[t].add(k, kTotalZeros[t][k][0], kTotalZeros[t][k][1]);
    for (int t = 0; t < 3; t++)
      for (int k = 0; k < 4; k++)
        if (kTotalZerosChromaDc420[t][k][1])
          tz_dc420[t].add(k, kTotalZerosChromaDc420[t][k][0], kTotalZerosChromaDc420[t][k][1]);
    for (int t = 0; t < 7; t++)
      for (int k = 0; k < 8; k++)
        if (kTotalZerosChromaDc422[t][k][1])
          tz_dc422[t].add(k, kTotalZerosChromaDc422[t][k][0], kTotalZerosChromaDc422[t][k][1]);
    for (int t = 0; t < 7; t++)
      for (int k = 0; k < 15; k++)
        if (kRunBefore[t][k][1]) run_before[t].add(k, kRunBefore[t][k][0], kRunBefore[t][k][1]);
    for (auto& v : coeff_token) v.build();
    for (auto& v : total_zeros) v.build();
    for (auto& v : tz_dc420) v.build();
    for (auto& v : tz_dc422) v.build();
    for (auto& v : run_before) v.build();
  }
};

const Tables& tables() {
  static const Tables t;
  return t;
}

int token_table(int nC) { return nC == -1 ? 4 : nC == -2 ? 5 : nC < 2 ? 0 : nC < 4 ? 1 : nC < 8 ? 2 : 3; }

}  // namespace

int cavlc_read_block(BitReader& br, int nC, int max_coeff, int* c) {
  const Tables& T = tables();
  for (int i = 0; i < max_coeff; i++) c[i] = 0;
  int tok = T.coeff_token[token_table(nC)].read(br);
  int tc = tok >> 2, t1 = tok & 3;
  if (tc == 0) return 0;
  if (tc > max_coeff) throw ParseError("invalid coeff_token");
  int level[16], run[16];
  for (int i = 0; i < t1; i++) level[i] = br.bit() ? -1 : 1;
  int suffix_length = tc > 10 && t1 < 3;
  for (int i = t1; i < tc; i++) {
    int prefix = 0;
    while (!br.bit())
      if (++prefix > 31) throw ParseError("invalid level_prefix");
    int64_t code = (int64_t)(prefix < 15 ? prefix : 15) << suffix_length;
    int suffix_size = prefix == 14 && suffix_length == 0 ? 4 : prefix >= 15 ? prefix - 3 : suffix_length;
    if (suffix_size > 0) code += br.u(suffix_size);
    if (prefix >= 15 && suffix_length == 0) code += 15;
    if (prefix >= 16) code += ((int64_t)1 << (prefix - 3)) - 4096;
    if (i == t1 && t1 < 3) code += 2;
    level[i] = (int)(code % 2 == 0 ? (code + 2) >> 1 : (-code - 1) >> 1);
    if (suffix_length == 0) suffix_length = 1;
    if (abs(level[i]) > (3 << (suffix_length - 1)) && suffix_length < 6) suffix_length++;
  }
  int zeros_left = 0;
  if (tc < max_coeff) {
    const Vlc& v = nC == -1 ? T.tz_dc420[tc - 1] : nC == -2 ? T.tz_dc422[tc - 1] : T.total_zeros[tc - 1];
    zeros_left = v.read(br);
    if (zeros_left + tc > max_coeff) throw ParseError("invalid total_zeros");
  }
  for (int i = 0; i < tc - 1; i++) {
    run[i] = 0;
    if (zeros_left > 0) {
      run[i] = T.run_before[(zeros_left < 7 ? zeros_left : 7) - 1].read(br);
      if (run[i] > zeros_left) throw ParseError("invalid run_before");
      zeros_left -= run[i];
    }
  }
  run[tc - 1] = zeros_left;
  int pos = -1;
  for (int i = tc - 1; i >= 0; i--) {
    pos += run[i] + 1;
    c[pos] = level[i];
  }
  return tc;
}

int cavlc_write_block(BitWriter& bw, int nC, int max_coeff, const int* c) {
  int level[16], pos[16], tc = 0;
  for (int i = max_coeff - 1; i >= 0; i--)
    if (c[i]) {
      level[tc] = c[i];
      pos[tc++] = i;
    }
  int t1 = 0;
  while (t1 < tc && t1 < 3 && abs(level[t1]) == 1) t1++;
  const uint16_t* tok = nC == -2 ? kCoeffTokenChromaDc422[tc * 4 + t1] : kCoeffToken[token_table(nC)][tc * 4 + t1];
  bw.put(tok[0], tok[1]);
  if (tc == 0) return 0;
  for (int i = 0; i < t1; i++) bw.bit(level[i] < 0);
  int suffix_length = tc > 10 && t1 < 3;
  for (int i = t1; i < tc; i++) {
    if (abs(level[i]) > (1 << 26)) throw TsvError("coefficient too large");
    int64_t code = level[i] > 0 ? 2 * (int64_t)level[i] - 2 : -2 * (int64_t)level[i] - 1;
    if (i == t1 && t1 < 3) code -= 2;
    int prefix, suffix_size = 0;
    int64_t suffix = 0;
    if (suffix_length == 0 && code < 14) {
      prefix = (int)code;
    } else if (suffix_length == 0 && code < 30) {
      prefix = 14;
      suffix = code - 14;
      suffix_size = 4;
    } else if (suffix_length > 0 && code < (15 << suffix_length)) {
      prefix = (int)(code >> suffix_length);
      suffix = code & ((1 << suffix_length) - 1);
      suffix_size = suffix_length;
    } else {
      int64_t esc = code - (suffix_length == 0 ? 30 : 15 << suffix_length);
      prefix = 15;
      while (esc >= ((int64_t)1 << (prefix - 2)) - 4096) prefix++;
      suffix = prefix == 15 ? esc : esc - ((int64_t)1 << (prefix - 3)) + 4096;
      suffix_size = prefix - 3;
    }
    bw.put(0, prefix);
    bw.bit(1);
    bw.put((uint64_t)suffix, suffix_size);
    if (suffix_length == 0) suffix_length = 1;
    if (abs(level[i]) > (3 << (suffix_length - 1)) && suffix_length < 6) suffix_length++;
  }
  int zeros_left = pos[0] + 1 - tc;
  if (tc < max_coeff) {
    const uint8_t* tz = nC == -1 ? kTotalZerosChromaDc420[tc - 1][zeros_left]
                        : nC == -2 ? kTotalZerosChromaDc422[tc - 1][zeros_left]
                                   : kTotalZeros[tc - 1][zeros_left];
    bw.put(tz[0], tz[1]);
  }
  for (int i = 0; i < tc - 1 && zeros_left > 0; i++) {
    int run = pos[i] - pos[i + 1] - 1;
    const uint8_t* rb = kRunBefore[(zeros_left < 7 ? zeros_left : 7) - 1][run];
    bw.put(rb[0], rb[1]);
    zeros_left -= run;
  }
  return tc;
}

}  // namespace h264p
