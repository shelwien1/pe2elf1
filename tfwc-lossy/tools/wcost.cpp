// wcost: ideal code length of each tensor under the FX2TFWC2 adaptive models
#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <initializer_list>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "../tfwz/tf/weights_io.h"
#include "../tfwz/tf/weights_io.inc"
#include "../tfwz/tf/weights_io_compressed.inc"
using namespace fx2;
static double bits;
static void bit(uint16_t* p, int b) {
  bits += -log2((b ? 2048.0 - *p : double(*p)) / 2048.0);
  if (!b) *p += (2048 - *p) >> 5; else *p -= *p >> 5;
}
static void tree(uint16_t* probs, int nb, uint32_t sym) {
  uint32_t node = 1;
  for (int k = nb - 1; k >= 0; k--) { int b = (sym >> k) & 1; bit(&probs[node], b); node = (node << 1) | b; }
}
int main(int argc, char** argv) {
  g_rope_rows_limit = 1;
  WeightsFile wf = WeightsFile::load_compressed(argv[1]);
  ModelsV2 m; uint8_t meta_prev = 0;
  auto put_meta = [&](uint8_t b) { tree(&m.meta[size_t(meta_prev) * 256], 8, b); meta_prev = b; };
  double cat[4] = {0, 0, 0, 0}; // int4, bf16, f32/i32, meta+names
  for (auto& name : wf.order) {
    const WTensor& t = wf.get(name);
    double b0 = bits;
    put_meta(uint8_t(name.size()));
    uint32_t c2 = 0, c1 = 0;
    for (char ch : name) { uint32_t u = uint8_t(ch); tree(&m.name[size_t((c2 << 8) | c1) * 256], 8, u); c2 = c1; c1 = u; }
    put_meta(t.dtype); put_meta(uint8_t(t.shape.size()));
    for (uint32_t d : t.shape) for (int k = 0; k < 4; k++) put_meta(uint8_t((d >> (8 * k)) & 0xFF));
    put_meta(0);
    cat[3] += bits - b0; b0 = bits;
    bool rope = name == "rope.sin" || name == "rope.cos";
    if (rope) continue;
    if (t.dtype == DT_I8) { for (size_t k = 0; k < t.numel; k++) tree(m.int4.data(), 4, uint32_t(t.i8()[k] + 7)); cat[0] += bits - b0; }
    else if (t.dtype == DT_BF16) { auto p = t.bf16_bits(); for (size_t k = 0; k < t.numel; k++) { uint32_t hi = p[k] >> 8, lo = p[k] & 255; tree(m.bf16_hi.data(), 8, hi); tree(&m.bf16_lo[hi * 256], 8, lo); } cat[1] += bits - b0; }
    else { for (size_t k = 0; k < t.data.size(); k++) tree(&m.plane[(k & 3) * 256], 8, t.data[k]); cat[2] += bits - b0; }
    if (argc > 2 && t.dtype != DT_I8) printf("%-60s %9.0f bytes\n", name.c_str(), (bits - b0) / 8);
  }
  printf("int4 %.0f  bf16 %.0f  f32/i32 %.0f  meta %.0f  total %.0f bytes\n", cat[0] / 8, cat[1] / 8, cat[2] / 8, cat[3] / 8, bits / 8);
}
