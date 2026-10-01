// tfwz - compact container for the 6m transformer weights (FX2TFWC2 <-> TFWZ)
//
//   tfwz c in.tfwc2 out.tfwz [plan.txt]   encode
//   tfwz d in.tfwz out.tfwc2              decode (byte-identical FX2TFWC2)
//   tfwz n in.tfwc2 plan.txt out.tfwc2    apply the plan's noise substitution
//                                         only (what d would reproduce)
//
// The tensor set, order, dtypes and shapes are the model's architecture and
// live in schema.inc, so the stream holds values only.  Everything is coded
// with one binary arithmetic coder and adaptive (KT-estimator) bit models:
//
//   int4 weights  4-bit trees over q+7, one adaptive model per tensor and
//                 row class.  The encoder picks the row classes (none, the
//                 plan's, or 2..8 row-energy quantiles) by simulating the cost;
//                 they pay off where rows were requantized with different steps.
//                 (The weights are i.i.d. within a row class: no column or
//                 neighbour context helps.)
//   row scales    bf16: the tensor's median, then signed deltas in the ordered
//                 (sign, exponent, mantissa) domain, dropping mantissa bits that
//                 are zero in every row; models shared by tensors of one kind.
//   f32 tensors   a flag, then bf16-exact values as sign / exponent / mantissa
//                 trees (context: column for [n][4] conv weights), else raw
//                 32-bit words.
//   noise         plan lines "noise NAME SEED [rows r0 r1 ...]": those rows
//                 are not stored; the decoder regenerates them from SEED and
//                 a 15-bin histogram (the stored rows' own) with splitmix64.
//   classes       plan lines "classes NAME c0 c1 ...": a candidate row
//                 classing for the encoder to consider.

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
#include "tf/weights_io.h"
#include "tf/weights_io.inc"
#include "tf/weights_io_compressed.inc"
#include "tf/weights_write.inc"

using fx2::WTensor;
using fx2::WeightsFile;

struct SchemaEntry {
  const char* name;
  uint8_t dtype;
  int ndim;
  uint32_t dims[2];
};
#include "schema.inc"

[[noreturn]] static void fail(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "tfwz: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

// ---------------------------------------------------------------------------
// binary arithmetic coder (carryless, 32-bit, paq-style), 16-bit probabilities
// ---------------------------------------------------------------------------
struct Encoder {
  std::vector<uint8_t> out;
  uint32_t x1 = 0, x2 = 0xFFFFFFFFu;
  void bit(int y, uint32_t p1) {  // p1 = P(y=1) * 65536, in [1, 65535]
    const uint32_t xm = x1 + uint32_t((uint64_t(x2 - x1) * p1) >> 16);
    if (y) x2 = xm; else x1 = xm + 1;
    while (((x1 ^ x2) & 0xFF000000u) == 0) {
      out.push_back(uint8_t(x2 >> 24));
      x1 <<= 8;
      x2 = (x2 << 8) | 255;
    }
  }
  void flush() {
    for (int i = 0; i < 4; i++) { out.push_back(uint8_t(x1 >> 24)); x1 <<= 8; }
  }
};
struct Decoder {
  const uint8_t *p, *end;
  uint32_t x1 = 0, x2 = 0xFFFFFFFFu, x = 0;
  Decoder(const uint8_t* b, size_t n) : p(b), end(b + n) {
    for (int i = 0; i < 4; i++) x = (x << 8) | byte();
  }
  uint8_t byte() { return p < end ? *p++ : 0; }
  int bit(uint32_t p1) {
    const uint32_t xm = x1 + uint32_t((uint64_t(x2 - x1) * p1) >> 16);
    int y = x <= xm;
    if (y) x2 = xm; else x1 = xm + 1;
    while (((x1 ^ x2) & 0xFF000000u) == 0) {
      x1 <<= 8;
      x2 = (x2 << 8) | 255;
      x = (x << 8) | byte();
    }
    return y;
  }
};

// KT-estimator bit: P(1) = (n1 + 0.4) / (n0 + n1 + 0.8), counts halved at 2^16
struct Bit {
  uint32_t n0 = 0, n1 = 0;
  uint32_t p1() const {
    double p = (n1 + 0.4) / (n0 + n1 + 0.8);
    uint32_t v = uint32_t(p * 65536.0 + 0.5);
    return v < 32 ? 32 : v > 65504 ? 65504 : v;
  }
  void update(int y) {
    if (y) n1++; else n0++;
    if (n0 + n1 >= 65536) { n0 = (n0 + 1) >> 1; n1 = (n1 + 1) >> 1; }
  }
};

struct Coder {
  Encoder* enc = nullptr;
  Decoder* dec = nullptr;
  int code(Bit& b, int y) {
    const uint32_t p = b.p1();
    if (enc) enc->bit(y, p); else y = dec->bit(p);
    b.update(y);
    return y;
  }
  // nbits-level binary tree, MSB first; t has (1 << nbits) entries
  uint32_t tree(Bit* t, int nbits, uint32_t v) {
    uint32_t node = 1;
    for (int k = nbits - 1; k >= 0; k--) {
      const int y = code(t[node], enc ? int((v >> k) & 1) : 0);
      node = (node << 1) | uint32_t(y);
    }
    return node - (1u << nbits);
  }
  uint32_t raw(int nbits, uint32_t v) {  // uniform bits
    uint32_t r = 0;
    for (int k = nbits - 1; k >= 0; k--) {
      int y = enc ? int((v >> k) & 1) : 0;
      if (enc) enc->bit(y, 32768); else y = dec->bit(32768);
      r = (r << 1) | uint32_t(y);
    }
    return r;
  }
};

// ---------------------------------------------------------------------------
// deterministic noise: splitmix64, inverse CDF over a 15-bin histogram
// ---------------------------------------------------------------------------
struct SplitMix {
  uint64_t s;
  uint64_t next() {
    uint64_t z = (s += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
  }
};
// hist: 15 counts summing to a power of two 2^16
static void gen_noise_row(uint64_t seed, int tensor, int row, const uint32_t* hist,
                          int8_t* out, int n) {
  SplitMix r{seed ^ (uint64_t(tensor) << 40) ^ (uint64_t(row) << 16)};
  uint32_t cum[16];
  cum[0] = 0;
  for (int i = 0; i < 15; i++) cum[i + 1] = cum[i] + hist[i];
  for (int j = 0; j < n; j++) {
    const uint32_t u = uint32_t(r.next() >> 48);  // 16 bits
    int s = 0;
    while (s < 14 && cum[s + 1] <= u) s++;
    out[j] = int8_t(s - 7);
  }
}
// normalizes counts to sum exactly 65536, every used bin >= 1
static void norm_hist(const uint64_t* c, uint32_t* h) {
  uint64_t tot = 0;
  for (int i = 0; i < 15; i++) tot += c[i];
  if (!tot) { for (int i = 0; i < 15; i++) h[i] = i == 7 ? 65536 : 0; return; }
  int64_t sum = 0, big = 0;
  for (int i = 0; i < 15; i++) {
    h[i] = c[i] ? std::max<uint32_t>(1, uint32_t((c[i] * 65536 + tot / 2) / tot)) : 0;
    sum += h[i];
    if (h[i] > h[big]) big = i;
  }
  h[big] = uint32_t(int64_t(h[big]) + (65536 - sum));
}

// ---------------------------------------------------------------------------
// plan: noise rows and candidate row classes, per int4 tensor
// ---------------------------------------------------------------------------
struct PlanEntry {
  uint64_t seed = 0;
  bool all = false;
  std::vector<int> rows;
  bool noise = false;        // a "noise" line was given
  std::vector<int> classes;  // "classes" line: per-row coding class (0..7)
};
// whitespace-separated tokens of a line (portable: no strtok_r / strtok_s)
static std::vector<std::string> split_ws(const char* line) {
  std::vector<std::string> v;
  const char* p = line;
  while (*p) {
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    const char* q = p;
    while (*q && *q != ' ' && *q != '\t' && *q != '\r' && *q != '\n') q++;
    if (q > p) v.emplace_back(p, q);
    p = q;
  }
  return v;
}

static std::unordered_map<std::string, PlanEntry> read_plan(const char* path) {
  std::unordered_map<std::string, PlanEntry> plan;
  if (!path) return plan;
  FILE* f = fopen(path, "rb");
  if (!f) fail("cannot open plan %s", path);
  static char line[1 << 20];
  while (fgets(line, sizeof line, f)) {
    const std::vector<std::string> w = split_ws(line);
    if (w.empty() || w[0][0] == '#') continue;
    if (w[0] == "classes") {
      if (w.size() < 2) fail("plan: bad classes line");
      PlanEntry& ns = plan[w[1]];
      for (size_t k = 2; k < w.size(); k++) {
        const int c = atoi(w[k].c_str());
        if (c < 0 || c > 7) fail("plan: class out of range");
        ns.classes.push_back(c);
      }
      continue;
    }
    if (w[0] != "noise") fail("plan: unknown directive %s", w[0].c_str());
    if (w.size() < 3) fail("plan: bad noise line");
    PlanEntry& ns = plan[w[1]];
    ns.noise = true;
    ns.seed = strtoull(w[2].c_str(), nullptr, 0);
    if (w.size() == 3) ns.all = true;
    else {
      if (w[3] != "rows") fail("plan: expected rows");
      for (size_t k = 4; k < w.size(); k++) ns.rows.push_back(atoi(w[k].c_str()));
    }
  }
  fclose(f);
  return plan;
}

// ---------------------------------------------------------------------------
// the models and the (de)coding of one tensor
// ---------------------------------------------------------------------------
struct Bf16Model {
  Bit sign[8][2];
  Bit exp[8][256];
  Bit man[8][128];
};

static size_t esize(uint8_t d) { return d == fx2::DT_I8 ? 1 : d == fx2::DT_BF16 ? 2 : 4; }

// codes a bf16 value (bits) with context ctx (0..7)
static uint16_t code_bf16(Coder& c, Bf16Model& m, int ctx, uint16_t v) {
  const int s = c.code(m.sign[ctx][0], (v >> 15) & 1);
  const uint32_t e = c.tree(m.exp[ctx], 8, (v >> 7) & 0xFF);
  const uint32_t mm = c.tree(m.man[ctx], 7, v & 0x7F);
  return uint16_t((s << 15) | (e << 7) | mm);
}

// integer residual coder: zero flag, sign, Elias-gamma length, mantissa bits
struct IntModel {
  Bit zero[4], sign[4], len[24], man[24][4];
};
static int32_t code_int(Coder& c, IntModel& m, int32_t v) {
  const bool dec = c.dec != nullptr;
  if (c.code(m.zero[0], dec ? 0 : v == 0)) return 0;
  const int neg = c.code(m.sign[0], dec ? 0 : v < 0);
  uint32_t a = dec ? 0 : uint32_t(v < 0 ? -v : v);
  int nb = 0;  // bit length of a, >= 1
  if (!dec) { while ((a >> nb) > 1) nb++; }
  int k = 0;
  for (;; k++) {  // unary length
    const int more = c.code(m.len[std::min(k, 23)], dec ? 0 : k < nb);
    if (!more) break;
  }
  nb = k;
  uint32_t r = 1;
  for (int b = nb - 1; b >= 0; b--) {
    const int pos = nb - 1 - b;  // 0 = the bit right below the leading one
    const int y = c.code(m.man[std::min(nb, 23)][std::min(pos, 3)], dec ? 0 : int((a >> b) & 1));
    r = (r << 1) | uint32_t(y);
  }
  const int32_t mag = int32_t(r);
  return neg ? -mag : mag;
}

struct Stream {
  Coder c;
  std::vector<std::vector<Bit>> int4;                          // per tensor
  std::unordered_map<std::string, std::unique_ptr<Bf16Model>> bf;  // per kind
  std::unordered_map<std::string, std::unique_ptr<IntModel>> im;   // per kind
  Bit is_bf16[4], noise_flag[4], rowflag[4], mode[4];
  std::vector<Bit> clsm = std::vector<Bit>(8);    // row class tree, fresh per tensor
  std::vector<Bit> shiftm = std::vector<Bit>(8);  // scale mantissa shift
  Bf16Model& bfm(const std::string& k) { auto& p = bf[k]; if (!p) p.reset(new Bf16Model()); return *p; }
  IntModel& imm(const std::string& k) { auto& p = im[k]; if (!p) p.reset(new IntModel()); return *p; }
};

// model kind of a tensor: the name without its layer number
static std::string kind_of(const std::string& name) {
  std::string s = name;
  if (s.rfind("blocks.", 0) == 0) s = s.substr(s.find('.', 7) + 1);
  return s;
}

// ideal adaptive code length (bits) of int4 rows under per-class KT bit trees,
// plus the class side information under the shared class tree
static double class_cost(const int8_t* q, int R, int C, const std::vector<int>& cls) {
  std::vector<Bit> m(16 * 8), cm(8);
  double bits = 0.0;
  auto cost = [&](Bit& b, int y) { const double p = b.p1() / 65536.0; bits -= std::log2(y ? p : 1.0 - p); b.update(y); };
  const bool has = !cls.empty();
  for (int r = 0; r < R && has; r++) {
    uint32_t node = 1;
    for (int k = 2; k >= 0; k--) { const int y = (cls[r] >> k) & 1; cost(cm[node], y); node = (node << 1) | uint32_t(y); }
  }
  for (int r = 0; r < R; r++) {
    Bit* mc = m.data() + 16 * (has ? cls[r] : 0);
    for (int j = 0; j < C; j++) {
      const uint32_t v = uint32_t(q[size_t(r) * C + j] + 7);
      uint32_t node = 1;
      for (int k = 3; k >= 0; k--) { const int y = (v >> k) & 1; cost(mc[node], y); node = (node << 1) | uint32_t(y); }
    }
  }
  return bits;
}

// the encoder's choice of row classes: none, the plan's, or row-energy quantiles
static std::vector<int> choose_classes(const int8_t* q, int R, int C, const std::vector<int>& plan_cls) {
  std::vector<int> best;
  double best_bits = class_cost(q, R, C, best);
  auto consider = [&](const std::vector<int>& cls) {
    const double b = class_cost(q, R, C, cls);
    if (b < best_bits) { best_bits = b; best = cls; }
  };
  if (!plan_cls.empty()) consider(plan_cls);
  std::vector<double> en(R);
  for (int r = 0; r < R; r++) {
    double s = 0;
    for (int j = 0; j < C; j++) s += double(q[size_t(r) * C + j]) * q[size_t(r) * C + j];
    en[r] = s / C;
  }
  std::vector<double> sorted = en;
  std::sort(sorted.begin(), sorted.end());
  for (int nc : {2, 3, 4, 6, 8}) {
    if (R < 8 * nc) continue;
    std::vector<int> cls(R);
    for (int r = 0; r < R; r++) {
      int c = 0;
      for (int b = 1; b < nc; b++) if (en[r] > sorted[size_t(R) * b / nc]) c = b;
      cls[r] = c;
    }
    consider(cls);
  }
  return best;
}

// codes tensor i of the schema.  enc: t holds the data; dec: t is filled.
static void code_tensor(Stream& S, int i, WTensor& t, const PlanEntry* ns) {
  const SchemaEntry& e = kSchema[i];
  Coder& c = S.c;
  const bool dec = c.dec != nullptr;
  const std::string name = e.name;
  if (name == "rope.sin" || name == "rope.cos") return;  // recomputed by the loader
  if (dec) {
    t.dtype = e.dtype;
    t.shape.assign(e.dims, e.dims + e.ndim);
    t.numel = 1;
    for (uint32_t d : t.shape) t.numel *= d;
    t.data.assign(t.numel * esize(e.dtype), 0);
  }
  const size_t n = t.numel;
  if (e.dtype == fx2::DT_I8) {
    const int R = int(e.dims[0]), C = int(e.dims[1]);
    int8_t* q = reinterpret_cast<int8_t*>(t.data.data());
    // noise: mode bit, then seed and histogram, then per-row flags
    int mode = c.code(S.noise_flag[0], ns && ns->noise ? 1 : 0);
    std::vector<uint8_t> isnoise(R, 0);
    uint32_t hist[15] = {0};
    uint64_t seed = 0;
    if (mode) {
      int all = c.code(S.noise_flag[1], ns && ns->all ? 1 : 0);
      if (!dec) {
        if (ns->all) std::fill(isnoise.begin(), isnoise.end(), 1);
        else for (int r : ns->rows) { if (r < 0 || r >= R) fail("%s: bad noise row", e.name); isnoise[r] = 1; }
        uint64_t cnt[15] = {0};
        for (int r = 0; r < R; r++)
          if (isnoise[r]) for (int j = 0; j < C; j++) cnt[q[size_t(r) * C + j] + 7]++;
        norm_hist(cnt, hist);
        seed = ns->seed;
      }
      seed = c.raw(32, uint32_t(seed));  // 32-bit seeds
      for (int k = 0; k < 15; k++) hist[k] = c.raw(17, hist[k]);
      if (all) std::fill(isnoise.begin(), isnoise.end(), 1);
      else for (int r = 0; r < R; r++) isnoise[r] = uint8_t(c.code(S.rowflag[0], dec ? 0 : isnoise[r]));
    }
    // per-row coding classes: each class has its own adaptive model
    std::vector<int> chosen;
    if (!dec) {
      std::vector<int> plan_cls;
      if (ns && !ns->classes.empty()) {
        if (int(ns->classes.size()) != R) fail("%s: %zu classes for %d rows", e.name, ns->classes.size(), R);
        plan_cls = ns->classes;
      }
      bool any_noise = false;
      for (int r = 0; r < R; r++) any_noise |= isnoise[r] != 0;
      if (!any_noise) chosen = choose_classes(q, R, C, plan_cls);
    }
    const int has_cls = c.code(S.mode[0], chosen.empty() ? 0 : 1);
    std::vector<int> cls(R, 0);
    if (has_cls) {
      std::vector<Bit>& cm = S.clsm;
      cm.assign(8, Bit());  // fresh per tensor, as class_cost assumes
      for (int r = 0; r < R; r++) cls[r] = int(c.tree(cm.data(), 3, dec ? 0 : uint32_t(chosen[r])));
    }
    std::vector<Bit>& m = S.int4[i];
    m.assign(16 * 8, Bit());
    for (int r = 0; r < R; r++) {
      int8_t* row = q + size_t(r) * C;
      if (isnoise[r]) {
        gen_noise_row(seed, i, r, hist, row, C);
        continue;
      }
      Bit* mc = m.data() + 16 * cls[r];
      for (int j = 0; j < C; j++) {
        const uint32_t v = c.tree(mc, 4, dec ? 0 : uint32_t(row[j] + 7));
        if (v > 14) fail("int4 symbol out of range");
        row[j] = int8_t(int(v) - 7);
      }
    }
    return;
  }
  const std::string kind = kind_of(name);
  if (e.dtype == fx2::DT_BF16) {
    uint16_t* v = reinterpret_cast<uint16_t*>(t.data.data());
    if (n < 8) {  // scalars (activation scales): shared sign/exp/mantissa model
      for (size_t k = 0; k < n; k++) v[k] = code_bf16(c, S.bfm("scalar.bf16"), 0, v[k]);
      return;
    }
    // row scales: the median as a reference, then signed deltas in the
    // ordered domain of the bits (-0.0 sorts just below +0.0)
    uint16_t ref = 0;
    if (!dec) {
      std::vector<uint16_t> vs(v, v + n);
      auto less = [](uint16_t a, uint16_t b) {
        const int32_t ka = (a & 0x8000) ? -int32_t(a & 0x7FFF) - 1 : int32_t(a);
        const int32_t kb = (b & 0x8000) ? -int32_t(b & 0x7FFF) - 1 : int32_t(b);
        return ka < kb;
      };
      std::nth_element(vs.begin(), vs.begin() + n / 2, vs.end(), less);
      ref = vs[n / 2];
    }
    ref = code_bf16(c, S.bfm("scale.ref"), 0, ref);
    // common low zero bits of all mantissas (scales snapped to a coarser grid)
    int shift = 0;
    if (!dec) {
      uint32_t orv = 0;
      for (size_t k = 0; k < n; k++) orv |= v[k] & 0x7F;
      while (shift < 6 && !((orv >> shift) & 1)) shift++;
    }
    shift = int(c.tree(S.shiftm.data(), 3, uint32_t(shift)));
    IntModel& im = S.imm(kind + (shift ? ".s" : ""));
    // the reduced, ordered domain: exponent and the (7 - shift) high mantissa
    // bits as one magnitude u; negative values map to -u - 1
    auto red = [&](uint16_t b) {
      const int32_t u = int32_t((((b >> 7) & 0xFF) << (7 - shift)) | ((b & 0x7F) >> shift));
      return (b & 0x8000) ? -u - 1 : u;
    };
    auto unred = [&](int32_t k) {
      const bool neg = k < 0;
      const int32_t u = neg ? -k - 1 : k;
      const uint16_t b = uint16_t(((u >> (7 - shift)) << 7) | ((u & ((1 << (7 - shift)) - 1)) << shift));
      return uint16_t(neg ? (b | 0x8000) : b);
    };
    const int32_t rr = red(ref);
    for (size_t k = 0; k < n; k++) {
      const int32_t d = code_int(c, im, dec ? 0 : red(v[k]) - rr);
      v[k] = unred(rr + d);
    }
    return;
  }
  // 32-bit: bf16-exact flag
  Bf16Model& m = S.bfm(kind);
  uint32_t* v = reinterpret_cast<uint32_t*>(t.data.data());
  int exact = 1;
  if (!dec) for (size_t k = 0; k < n; k++) if (v[k] & 0xFFFF) exact = 0;
  exact = c.code(S.is_bf16[0], exact);
  const int cols = e.ndim == 2 ? int(e.dims[1]) : 1;
  for (size_t k = 0; k < n; k++) {
    if (exact) {
      const int ctx = cols <= 8 ? int(k % cols) : 0;
      v[k] = uint32_t(code_bf16(c, m, ctx, uint16_t(v[k] >> 16))) << 16;
    } else {
      v[k] = c.raw(32, v[k]);
    }
  }
}

static std::vector<uint8_t> load_file(const char* fn) {
  FILE* f = fopen(fn, "rb");
  if (!f) fail("cannot open %s", fn);
  std::vector<uint8_t> b;
  uint8_t buf[1 << 16];
  size_t k;
  while ((k = fread(buf, 1, sizeof buf, f)) > 0) b.insert(b.end(), buf, buf + k);
  fclose(f);
  return b;
}

static void check_schema(const WeightsFile& wf) {
  if (int(wf.order.size()) != kSchemaN) fail("input has %zu tensors, schema %d", wf.order.size(), kSchemaN);
  for (int i = 0; i < kSchemaN; i++) {
    const SchemaEntry& e = kSchema[i];
    if (wf.order[i] != e.name) fail("tensor %d is %s, schema says %s", i, wf.order[i].c_str(), e.name);
    const WTensor& t = wf.get(e.name);
    if (t.dtype != e.dtype || int(t.shape.size()) != e.ndim) fail("%s: dtype/ndim mismatch", e.name);
    for (int d = 0; d < e.ndim; d++)
      if (t.shape[d] != e.dims[d] && std::string(e.name).rfind("rope.", 0) != 0) fail("%s: shape mismatch", e.name);
  }
}

// encode (and verify by decoding) - returns the stream
static std::vector<uint8_t> encode(WeightsFile& wf, const std::unordered_map<std::string, PlanEntry>& plan,
                                   std::vector<double>* sizes) {
  Encoder enc;
  Stream S;
  S.c.enc = &enc;
  S.int4.resize(kSchemaN);
  for (int i = 0; i < kSchemaN; i++) {
    auto it = plan.find(kSchema[i].name);
    const PlanEntry* ns = it == plan.end() ? nullptr : &it->second;
    if (ns && kSchema[i].dtype != fx2::DT_I8) fail("%s: plan entry for a non-int4 tensor", kSchema[i].name);
    WTensor& t = wf.tensors[kSchema[i].name];
    const size_t before = enc.out.size();
    // code_tensor replaces noise rows in place, so wf ends up holding exactly
    // what the decoder will build
    code_tensor(S, i, t, ns);
    if (sizes) (*sizes)[i] = double(enc.out.size() - before);
  }
  enc.flush();
  std::vector<uint8_t> o = {'T', 'F', 'W', 'Z', 1};
  o.insert(o.end(), enc.out.begin(), enc.out.end());
  return o;
}

static WeightsFile decode(const std::vector<uint8_t>& b) {
  if (b.size() < 5 || memcmp(b.data(), "TFWZ", 4) || b[4] != 1) fail("not a TFWZ v1 stream");
  Decoder dec(b.data() + 5, b.size() - 5);
  Stream S;
  S.c.dec = &dec;
  S.int4.resize(kSchemaN);
  WeightsFile wf;
  for (int i = 0; i < kSchemaN; i++) {
    WTensor t;
    const std::string name = kSchema[i].name;
    if (name == "rope.sin" || name == "rope.cos") {
      t.dtype = kSchema[i].dtype;
      t.shape.assign(kSchema[i].dims, kSchema[i].dims + kSchema[i].ndim);
      t.numel = 0;  // no payload: write_weights_v2 regenerates nothing, the loader recomputes
    } else {
      code_tensor(S, i, t, nullptr);
    }
    wf.order.push_back(name);
    wf.tensors[name] = std::move(t);
  }
  return wf;
}

static bool same(const WeightsFile& a, const WeightsFile& b) {
  for (int i = 0; i < kSchemaN; i++) {
    const std::string name = kSchema[i].name;
    if (name == "rope.sin" || name == "rope.cos") continue;
    if (a.get(name).data != b.get(name).data) {
      fprintf(stderr, "tfwz: mismatch in %s\n", name.c_str());
      return false;
    }
  }
  return true;
}

int main(int argc, char** argv) {
  if (argc < 4) {
    fprintf(stderr, "usage: tfwz c in.tfwc2 out.tfwz [plan] | d in.tfwz out.tfwc2 | n in.tfwc2 plan out.tfwc2\n");
    return 1;
  }
  fx2::g_rope_rows_limit = 1;  // the tables are never needed here
  const char mode = argv[1][0];
  if (mode == 'c' || mode == 'n') {
    WeightsFile wf = WeightsFile::load_compressed(argv[2]);
    check_schema(wf);
    auto plan = read_plan(mode == 'c' ? (argc > 4 ? argv[4] : nullptr) : argv[3]);
    std::vector<double> sizes(kSchemaN, 0.0);
    std::vector<uint8_t> z = encode(wf, plan, &sizes);
    WeightsFile back = decode(z);
    if (!same(wf, back)) fail("round trip failed");
    if (mode == 'n') {
      if (!fx2::write_weights_v2(argv[4], back, back.order)) fail("write failed");
      return 0;
    }
    FILE* f = fopen(argv[3], "wb");
    if (!f || fwrite(z.data(), 1, z.size(), f) != z.size()) fail("cannot write %s", argv[3]);
    fclose(f);
    double cat[4] = {0, 0, 0, 0};
    for (int i = 0; i < kSchemaN; i++) {
      const int k = kSchema[i].dtype == fx2::DT_I8 ? 0 : kSchema[i].dtype == fx2::DT_BF16 ? 1 : 2;
      cat[k] += sizes[i];
    }
    printf("%s: %zu bytes (int4 %.0f, bf16 %.0f, f32/i32 %.0f)\n", argv[3], z.size(), cat[0], cat[1], cat[2]);
    if (getenv("TFWZ_VERBOSE"))
      for (int i = 0; i < kSchemaN; i++) printf("%9.0f %s\n", sizes[i], kSchema[i].name);
    return 0;
  }
  if (mode == 'd') {
    WeightsFile wf = decode(load_file(argv[2]));
    if (!fx2::write_weights_v2(argv[3], wf, wf.order)) fail("write failed");
    return 0;
  }
  fail("unknown mode %s", argv[1]);
}
