// ---------------------------------------------------------------------------
// 3to2 - convert transformer weight containers between FX2TFWC3 and FX2TFWC2.
//
//     3to2 c <in.tfwc3> <out.tfwc2>     v3 -> v2  (what the name says)
//     3to2 d <in.tfwc2> <out.tfwc3>     v2 -> v3  (and back)
//
// Why: coder0 (../coder0.cpp) reads FX2TFWC2. zmix ships its retrained weights
// as FX2TFWC3 only, and the v3 encoder (experiments/gen7-weights-v3/wcode3.py)
// is not in the zmix source package - only the decoder is. So this is the
// encoder for both containers, written from that decoder, which documents the
// two formats exactly.
//
// Derived from GPL-3 sources and therefore GPL-3:
//   fx2-cmix-transformer (Vladimir Ivanov) - cpp_infer/src/weights_io.h,
//     weights_io_compressed.cpp: the FX2TFWC1/2 container, the LZMA-style
//     binary range coder and its model set, and the CUDA-bit-exact
//     sinf/cosf used to recompute the rope tables.
//   zmix (zmix-src.tar.gz v1.0), third_party/fx2_transformer/
//     weights_io_compressed.cpp: the FX2TFWC3 reader, whose header comment
//     specifies the three v3 changes this file implements the writer for.
// The decoding half below is a transcription of those readers that additionally
// records the things a re-encoder needs and the WeightsFile API throws away:
// the order the tensors appear in, and each tensor's payload encoding.
//
// What it does NOT do: change a single tensor value. The v3 container can hold
// a DT_F32 tensor as bfloat16 (ENC_BF16_F32), which is how zmix's blobs carry
// their 88 raw trained f32 tensors, and that rounding is the encoder's to do -
// not a converter's. Going v2 -> v3, a DT_F32 tensor is written as bfloat16
// only when every value in it already IS a bfloat16, which is exactly true of a
// file this tool produced from a v3 one and (almost) never true otherwise. So
// c then d gets back what it started from, and neither direction loses data.
// ---------------------------------------------------------------------------

#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

[[noreturn]] void die(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  std::fprintf(stderr, "3to2: ");
  std::vfprintf(stderr, fmt, ap);
  std::fprintf(stderr, "\n");
  va_end(ap);
  std::exit(1);
}

// --- the container's own vocabulary (weights_io.h) -------------------------

enum WDtype : uint8_t { DT_I8 = 0, DT_BF16 = 1, DT_F32 = 2, DT_I32 = 3 };

enum : uint8_t {
  ENC_RAW = 0,
  ENC_INT4 = 1,
  ENC_BF16 = 2,
  ENC_PLANE4 = 3,
  ENC_ROPE_SIN = 4,
  ENC_ROPE_COS = 5,
  ENC_BF16_F32 = 6,   // v3 only
};

// frozen v3 format constants; the reader holds the same three numbers
enum { V3_HALVE_LIMIT = 8192, V3_NSYM = 15, V3_NCLASS = 16 };

size_t dtype_size(uint8_t dt) {
  switch (dt) {
    case DT_I8: return 1;
    case DT_BF16: return 2;
    case DT_F32: case DT_I32: return 4;
    default: die("unknown dtype code %u", unsigned(dt));
  }
}

// One tensor, plus the two things the reader's WeightsFile does not keep.
struct Tensor {
  std::string name;
  uint8_t dtype = 0;
  std::vector<uint32_t> shape;
  size_t numel = 0;
  std::vector<uint8_t> data;
  uint8_t enc = ENC_RAW;   // how it was stored
  uint8_t cls = 0;         // v3 ENC_INT4 only: which adaptive count model
};

struct Weights {
  std::vector<Tensor> t;   // in file order, which v3's name coding depends on
  int find(const std::string& n) const {
    for (size_t i = 0; i < t.size(); i++) if (t[i].name == n) return int(i);
    return -1;
  }
};

// --- the LZMA-style binary range coder ------------------------------------
//
// The decoder is weights_io_compressed.cpp's BinDecoder verbatim; the encoder
// below is its mirror. 11-bit probabilities, shift 5, renormalization below
// 2^24, and the first byte of the stream is the encoder's initial zero cache -
// which is why every decoder starts with p++.

struct BinDecoder {
  const uint8_t* p;
  const uint8_t* end;
  uint32_t range = 0xFFFFFFFFu;
  uint32_t code = 0;

  BinDecoder(const uint8_t* data, size_t len) : p(data), end(data + len) {
    p++;
    for (int i = 0; i < 4; i++) code = (code << 8) | byte();
  }
  uint8_t byte() { return p < end ? *p++ : 0; }
  int decode_bit(uint16_t* prob) {
    uint32_t bound = (range >> 11) * *prob;
    int bit;
    if (code < bound) {
      range = bound;
      *prob = uint16_t(*prob + ((2048 - *prob) >> 5));
      bit = 0;
    } else {
      code -= bound;
      range -= bound;
      *prob = uint16_t(*prob - (*prob >> 5));
      bit = 1;
    }
    while (range < (1u << 24)) { code = (code << 8) | byte(); range <<= 8; }
    return bit;
  }
  uint32_t decode_tree(uint16_t* probs, int nbits) {
    uint32_t node = 1;
    for (int k = 0; k < nbits; k++) node = (node << 1) | decode_bit(&probs[node]);
    return node - (1u << nbits);
  }
  uint32_t decode_freq(const uint32_t* cnt, int nsym, uint32_t tot) {
    uint32_t r = range / tot;
    uint32_t v = code / r;
    if (v > tot - 1) v = tot - 1;
    uint32_t cum = 0;
    int s = 0;
    while (s < nsym - 1 && cum + cnt[s] <= v) { cum += cnt[s]; s++; }
    code -= r * cum;
    range = r * cnt[s];
    while (range < (1u << 24)) { code = (code << 8) | byte(); range <<= 8; }
    return uint32_t(s);
  }
};

struct BinEncoder {
  std::vector<uint8_t>& out;
  uint64_t low = 0;
  uint32_t range = 0xFFFFFFFFu;
  uint8_t  cache = 0;
  uint64_t cache_size = 1;   // the leading zero byte the decoder skips

  explicit BinEncoder(std::vector<uint8_t>& o) : out(o) {}

  void shift_low() {
    if (uint32_t(low) < 0xFF000000u || uint32_t(low >> 32) != 0) {
      do {
        out.push_back(uint8_t(cache + uint8_t(low >> 32)));
        cache = 0xFF;
      } while (--cache_size);
      cache = uint8_t(low >> 24);
    }
    cache_size++;
    low = (low << 8) & 0xFFFFFFFFull;
  }
  void encode_bit(uint16_t* prob, int bit) {
    uint32_t bound = (range >> 11) * *prob;
    if (!bit) {
      range = bound;
      *prob = uint16_t(*prob + ((2048 - *prob) >> 5));
    } else {
      low += bound;
      range -= bound;
      *prob = uint16_t(*prob - (*prob >> 5));
    }
    while (range < (1u << 24)) { range <<= 8; shift_low(); }
  }
  void encode_tree(uint16_t* probs, int nbits, uint32_t sym) {
    uint32_t node = 1;
    for (int k = nbits - 1; k >= 0; k--) {
      int bit = int((sym >> k) & 1);
      encode_bit(&probs[node], bit);
      node = (node << 1) | uint32_t(bit);
    }
  }
  void encode_freq(const uint32_t* cnt, int nsym, uint32_t tot, uint32_t s) {
    (void)nsym;
    uint32_t r = range / tot;
    uint32_t cum = 0;
    for (uint32_t k = 0; k < s; k++) cum += cnt[k];
    low += uint64_t(r) * cum;
    range = r * cnt[s];
    while (range < (1u << 24)) { range <<= 8; shift_low(); }
  }
  void flush() { for (int i = 0; i < 5; i++) shift_low(); }
};

// --- the two model sets, shared by the reader and the writer --------------

struct ModelsV2 {
  std::vector<uint16_t> meta, name, raw, int4, bf16_hi, bf16_lo, plane;
  uint8_t meta_prev = 0;
  ModelsV2()
      : meta(256 * 256, 1024), name(size_t(65536) * 256, 1024), raw(256, 1024),
        int4(16, 1024), bf16_hi(256, 1024), bf16_lo(256 * 256, 1024),
        plane(4 * 256, 1024) {}
};

struct ModelsV3 {
  std::vector<uint16_t> meta, name, raw, bf16_hi, bf16_lo, rf32_hi, rf32_lo, plane;
  uint8_t meta_prev = 0;
  uint32_t cnt[V3_NCLASS][V3_NSYM];
  uint32_t tot[V3_NCLASS];
  ModelsV3()
      : meta(256 * 256, 1024), name(256 * 256, 1024), raw(256, 1024),
        bf16_hi(256, 1024), bf16_lo(256 * 256, 1024), rf32_hi(256, 1024),
        rf32_lo(256 * 256, 1024), plane(4 * 256, 1024) {
    for (int c = 0; c < V3_NCLASS; c++) {
      for (int k = 0; k < V3_NSYM; k++) cnt[c][k] = 1;
      tot[c] = V3_NSYM;
    }
  }
  void int4_update(int cls, uint32_t s) {
    uint32_t* c = cnt[cls];
    c[s]++;
    if (++tot[cls] > uint32_t(V3_HALVE_LIMIT)) {
      uint32_t t = 0;
      for (int k = 0; k < V3_NSYM; k++) { c[k] = (c[k] + 1) >> 1; t += c[k]; }
      tot[cls] = t;
    }
  }
};

// --- file helpers ----------------------------------------------------------

std::vector<uint8_t> read_file(const char* path) {
  FILE* f = std::fopen(path, "rb");
  if (!f) die("cannot open %s", path);
  std::fseek(f, 0, SEEK_END);
  long n = std::ftell(f);
  std::fseek(f, 0, SEEK_SET);
  if (n < 12) die("%s: too small to be a weights container", path);
  std::vector<uint8_t> b((size_t(n)));
  if (std::fread(b.data(), 1, b.size(), f) != b.size()) die("%s: short read", path);
  std::fclose(f);
  return b;
}

void write_file(const char* path, const std::vector<uint8_t>& b) {
  FILE* f = std::fopen(path, "wb");
  if (!f) die("cannot create %s", path);
  if (std::fwrite(b.data(), 1, b.size(), f) != b.size()) die("%s: short write", path);
  std::fclose(f);
}


// ---------------------------------------------------------------------------
// FX2TFWC2
// ---------------------------------------------------------------------------

Weights read_v2(const std::vector<uint8_t>& buf, const char* path) {
  uint32_t n_tensors;
  std::memcpy(&n_tensors, buf.data() + 8, 4);
  BinDecoder dec(buf.data() + 12, buf.size() - 12);
  ModelsV2 m;
  auto get_meta = [&]() -> uint8_t {
    uint8_t b = uint8_t(dec.decode_tree(&m.meta[size_t(m.meta_prev) * 256], 8));
    m.meta_prev = b;
    return b;
  };

  Weights w;
  w.t.resize(n_tensors);
  for (uint32_t i = 0; i < n_tensors; i++) {
    Tensor& t = w.t[i];
    uint32_t name_len = get_meta();
    t.name.assign(name_len, '\0');
    uint32_t c2 = 0, c1 = 0;
    for (uint32_t k = 0; k < name_len; k++) {
      uint32_t ch = dec.decode_tree(&m.name[size_t((c2 << 8) | c1) * 256], 8);
      t.name[k] = char(ch);
      c2 = c1; c1 = ch;
    }
    t.dtype = get_meta();
    size_t dsz = dtype_size(t.dtype);
    uint32_t ndim = get_meta();
    if (ndim > 8) die("%s: %s: absurd ndim %u", path, t.name.c_str(), ndim);
    t.shape.resize(ndim);
    t.numel = 1;
    for (uint32_t d = 0; d < ndim; d++) {
      uint32_t v = 0;
      for (int k = 0; k < 4; k++) v |= uint32_t(get_meta()) << (8 * k);
      t.shape[d] = v;
      t.numel *= v;
    }
    size_t bytes = t.numel * dsz;
    t.enc = get_meta();

    switch (t.enc) {
      case ENC_INT4: {
        if (t.dtype != DT_I8) die("%s: %s: ENC_INT4 on dtype %u", path, t.name.c_str(), t.dtype);
        t.data.resize(bytes);
        int8_t* out = reinterpret_cast<int8_t*>(t.data.data());
        for (size_t k = 0; k < t.numel; k++)
          out[k] = int8_t(int(dec.decode_tree(m.int4.data(), 4)) - 7);
        break;
      }
      case ENC_BF16: {
        if (t.dtype != DT_BF16) die("%s: %s: ENC_BF16 on dtype %u", path, t.name.c_str(), t.dtype);
        t.data.resize(bytes);
        uint16_t* out = reinterpret_cast<uint16_t*>(t.data.data());
        for (size_t k = 0; k < t.numel; k++) {
          uint32_t hi = dec.decode_tree(m.bf16_hi.data(), 8);
          uint32_t lo = dec.decode_tree(&m.bf16_lo[hi * 256], 8);
          out[k] = uint16_t((hi << 8) | lo);
        }
        break;
      }
      case ENC_PLANE4: {
        if (dsz != 4) die("%s: %s: ENC_PLANE4 on dtype %u", path, t.name.c_str(), t.dtype);
        t.data.resize(bytes);
        for (size_t k = 0; k < bytes; k++)
          t.data[k] = uint8_t(dec.decode_tree(&m.plane[(k & 3) * 256], 8));
        break;
      }
      case ENC_ROPE_SIN: case ENC_ROPE_COS:
        // Nothing is stored: the loader recomputes the table from
        // rope.inv_freq with a bit-exact port of CUDA's sinf/cosf. So this
        // tool never has to touch the 32 MB of sin/cos, or carry that port -
        // it passes the encoding byte across and lets the loader do what it
        // always does. rope.inv_freq comes earlier in the file, and keeping
        // the tensors in their original order is what keeps that true.
        if (t.dtype != DT_F32 || ndim != 2) die("%s: %s: bad rope tensor", path, t.name.c_str());
        break;
      case ENC_RAW: {
        t.data.resize(bytes);
        for (size_t k = 0; k < bytes; k++)
          t.data[k] = uint8_t(dec.decode_tree(m.raw.data(), 8));
        break;
      }
      default:
        die("%s: %s: unknown encoding %u", path, t.name.c_str(), t.enc);
    }
  }
  return w;
}

void write_v2(const Weights& w, std::vector<uint8_t>& out) {
  out.insert(out.end(), {'F','X','2','T','F','W','C','2'});
  uint32_t n = uint32_t(w.t.size());
  const uint8_t* np = reinterpret_cast<const uint8_t*>(&n);
  out.insert(out.end(), np, np + 4);

  BinEncoder enc(out);
  ModelsV2 m;
  auto put_meta = [&](uint32_t b) {
    enc.encode_tree(&m.meta[size_t(m.meta_prev) * 256], 8, b & 0xFF);
    m.meta_prev = uint8_t(b);
  };

  for (const Tensor& t : w.t) {
    if (t.name.size() > 255) die("%s: name too long for the v2 container", t.name.c_str());
    put_meta(uint32_t(t.name.size()));
    uint32_t c2 = 0, c1 = 0;
    for (unsigned char ch : t.name) {
      enc.encode_tree(&m.name[size_t((c2 << 8) | c1) * 256], 8, ch);
      c2 = c1; c1 = ch;
    }
    put_meta(t.dtype);
    put_meta(uint32_t(t.shape.size()));
    for (uint32_t d : t.shape)
      for (int k = 0; k < 4; k++) put_meta((d >> (8 * k)) & 0xFF);
    put_meta(t.enc);

    size_t bytes = t.numel * dtype_size(t.dtype);
    switch (t.enc) {
      case ENC_INT4: {
        const int8_t* in = reinterpret_cast<const int8_t*>(t.data.data());
        for (size_t k = 0; k < t.numel; k++)
          enc.encode_tree(m.int4.data(), 4, uint32_t(int(in[k]) + 7));
        break;
      }
      case ENC_BF16: {
        const uint16_t* in = reinterpret_cast<const uint16_t*>(t.data.data());
        for (size_t k = 0; k < t.numel; k++) {
          uint32_t hi = in[k] >> 8, lo = in[k] & 0xFF;
          enc.encode_tree(m.bf16_hi.data(), 8, hi);
          enc.encode_tree(&m.bf16_lo[hi * 256], 8, lo);
        }
        break;
      }
      case ENC_PLANE4:
        for (size_t k = 0; k < bytes; k++)
          enc.encode_tree(&m.plane[(k & 3) * 256], 8, t.data[k]);
        break;
      case ENC_ROPE_SIN: case ENC_ROPE_COS:
        break;
      case ENC_RAW:
        for (size_t k = 0; k < bytes; k++) enc.encode_tree(m.raw.data(), 8, t.data[k]);
        break;
      default:
        die("%s: encoding %u has no place in a v2 container", t.name.c_str(), t.enc);
    }
  }
  enc.flush();
}

// ---------------------------------------------------------------------------
// FX2TFWC3
// ---------------------------------------------------------------------------
//
// v3 is v2's stream with three changes (the reader's header comment is the
// specification):
//   (a) int4 weights use an adaptive 15-symbol frequency model per weight
//       CLASS, the class being a byte the encoder puts in the metadata;
//   (b) names are schema-coded - a SKELETON with each run of decimal digits
//       replaced by one 0x00, front-coded against one earlier skeleton named
//       by its distance back, plus those integers as LEB128 varints - and
//       shape dims are varints too;
//   (c) ENC_BF16_F32 carries a DT_F32 tensor as bfloat16.

// "blocks.12.mlp.up.weight.q" -> skeleton "blocks.\0.mlp.up.weight.q" + {12}
void split_name(const std::string& name, std::string& sk, std::vector<uint32_t>& nums) {
  sk.clear();
  nums.clear();
  for (size_t i = 0; i < name.size();) {
    if (name[i] >= '0' && name[i] <= '9') {
      size_t j = i;
      uint64_t v = 0;
      while (j < name.size() && name[j] >= '0' && name[j] <= '9') {
        v = v * 10 + uint64_t(name[j] - '0');
        if (v > 0xFFFFFFFFull) die("%s: number too large for the v3 name schema", name.c_str());
        j++;
      }
      // the reader renders the varint with %u, so a run with a leading zero
      // (or one that is just "0" inside a longer run) would not come back
      char buf[16];
      std::snprintf(buf, sizeof buf, "%u", unsigned(v));
      if (name.compare(i, j - i, buf) != 0)
        die("%s: digit run \"%.*s\" is not representable in the v3 name schema",
            name.c_str(), int(j - i), name.c_str() + i);
      sk.push_back('\0');
      nums.push_back(uint32_t(v));
      i = j;
    } else {
      sk.push_back(name[i]);
      i++;
    }
  }
}

std::string join_name(const std::string& sk, const std::vector<uint32_t>& nums) {
  std::string name;
  size_t n = 0;
  for (char c : sk) {
    if (c == '\0') {
      char buf[16];
      std::snprintf(buf, sizeof buf, "%u", unsigned(nums[n++]));
      name += buf;
    } else {
      name.push_back(c);
    }
  }
  return name;
}

Weights read_v3(const std::vector<uint8_t>& buf, const char* path) {
  uint32_t n_tensors;
  std::memcpy(&n_tensors, buf.data() + 8, 4);
  BinDecoder dec(buf.data() + 12, buf.size() - 12);
  ModelsV3 m;
  auto get_meta = [&]() -> uint8_t {
    uint8_t b = uint8_t(dec.decode_tree(&m.meta[size_t(m.meta_prev) * 256], 8));
    m.meta_prev = b;
    return b;
  };
  auto get_varint = [&]() -> uint32_t {
    uint32_t v = 0;
    for (int k = 0; k < 5; k++) {
      uint8_t b = get_meta();
      v |= uint32_t(b & 0x7F) << (7 * k);
      if (!(b & 0x80)) return v;
    }
    die("%s: varint too long", path);
  };

  Weights w;
  w.t.resize(n_tensors);
  std::vector<std::string> hist;
  hist.reserve(n_tensors);

  for (uint32_t i = 0; i < n_tensors; i++) {
    Tensor& t = w.t[i];
    uint32_t d = get_meta(), lcp = get_meta(), suf = get_meta();
    if (d > hist.size()) die("%s: name reference %u out of range", path, d);
    std::string sk;
    if (d) {
      const std::string& ref = hist[hist.size() - d];
      if (lcp > ref.size()) die("%s: name prefix %u too long", path, lcp);
      sk.assign(ref, 0, lcp);
    } else if (lcp) {
      die("%s: name prefix without a reference", path);
    }
    uint32_t ctx = lcp ? uint8_t(sk[lcp - 1]) : 0;
    for (uint32_t k = 0; k < suf; k++) {
      uint32_t ch = dec.decode_tree(&m.name[size_t(ctx) * 256], 8);
      sk.push_back(char(ch));
      ctx = ch;
    }
    hist.push_back(sk);
    std::vector<uint32_t> nums;
    for (char c : sk) if (c == '\0') nums.push_back(get_varint());
    t.name = join_name(sk, nums);

    t.dtype = get_meta();
    size_t dsz = dtype_size(t.dtype);
    uint32_t ndim = get_meta();
    if (ndim > 8) die("%s: %s: absurd ndim %u", path, t.name.c_str(), ndim);
    t.shape.resize(ndim);
    t.numel = 1;
    for (uint32_t k = 0; k < ndim; k++) { t.shape[k] = get_varint(); t.numel *= t.shape[k]; }
    size_t bytes = t.numel * dsz;
    t.enc = get_meta();

    switch (t.enc) {
      case ENC_INT4: {
        if (t.dtype != DT_I8) die("%s: %s: ENC_INT4 on dtype %u", path, t.name.c_str(), t.dtype);
        t.cls = get_meta();
        if (t.cls >= V3_NCLASS) die("%s: %s: int4 class %u out of range", path, t.name.c_str(), t.cls);
        t.data.resize(bytes);
        int8_t* out = reinterpret_cast<int8_t*>(t.data.data());
        const uint32_t* c = m.cnt[t.cls];
        for (size_t k = 0; k < t.numel; k++) {
          uint32_t s = dec.decode_freq(c, V3_NSYM, m.tot[t.cls]);
          out[k] = int8_t(int(s) - 7);
          m.int4_update(t.cls, s);
        }
        break;
      }
      case ENC_BF16: {
        if (t.dtype != DT_BF16) die("%s: %s: ENC_BF16 on dtype %u", path, t.name.c_str(), t.dtype);
        t.data.resize(bytes);
        uint16_t* out = reinterpret_cast<uint16_t*>(t.data.data());
        for (size_t k = 0; k < t.numel; k++) {
          uint32_t hi = dec.decode_tree(m.bf16_hi.data(), 8);
          uint32_t lo = dec.decode_tree(&m.bf16_lo[hi * 256], 8);
          out[k] = uint16_t((hi << 8) | lo);
        }
        break;
      }
      case ENC_BF16_F32: {
        if (t.dtype != DT_F32) die("%s: %s: ENC_BF16_F32 on dtype %u", path, t.name.c_str(), t.dtype);
        t.data.resize(bytes);
        uint32_t* out = reinterpret_cast<uint32_t*>(t.data.data());
        for (size_t k = 0; k < t.numel; k++) {
          uint32_t hi = dec.decode_tree(m.rf32_hi.data(), 8);
          uint32_t lo = dec.decode_tree(&m.rf32_lo[hi * 256], 8);
          out[k] = ((hi << 8) | lo) << 16;    // bf16_to_f32
        }
        break;
      }
      case ENC_PLANE4: {
        if (dsz != 4) die("%s: %s: ENC_PLANE4 on dtype %u", path, t.name.c_str(), t.dtype);
        t.data.resize(bytes);
        for (size_t k = 0; k < bytes; k++)
          t.data[k] = uint8_t(dec.decode_tree(&m.plane[(k & 3) * 256], 8));
        break;
      }
      case ENC_ROPE_SIN: case ENC_ROPE_COS:
        if (t.dtype != DT_F32 || ndim != 2) die("%s: %s: bad rope tensor", path, t.name.c_str());
        break;
      case ENC_RAW: {
        t.data.resize(bytes);
        for (size_t k = 0; k < bytes; k++)
          t.data[k] = uint8_t(dec.decode_tree(m.raw.data(), 8));
        break;
      }
      default:
        die("%s: %s: unknown encoding %u", path, t.name.c_str(), t.enc);
    }
  }
  return w;
}

void write_v3(const Weights& w, std::vector<uint8_t>& out) {
  out.insert(out.end(), {'F','X','2','T','F','W','C','3'});
  uint32_t n = uint32_t(w.t.size());
  const uint8_t* np = reinterpret_cast<const uint8_t*>(&n);
  out.insert(out.end(), np, np + 4);

  BinEncoder enc(out);
  ModelsV3 m;
  auto put_meta = [&](uint32_t b) {
    enc.encode_tree(&m.meta[size_t(m.meta_prev) * 256], 8, b & 0xFF);
    m.meta_prev = uint8_t(b);
  };
  auto put_varint = [&](uint32_t v) {
    for (int k = 0; k < 5; k++) {
      uint8_t b = uint8_t(v & 0x7F);
      v >>= 7;
      if (v) b |= 0x80;
      put_meta(b);
      if (!v) return;
    }
  };

  std::vector<std::string> hist;
  hist.reserve(w.t.size());

  for (const Tensor& t : w.t) {
    std::string sk;
    std::vector<uint32_t> nums;
    split_name(t.name, sk, nums);
    if (sk.size() > 255) die("%s: skeleton too long for the v3 container", t.name.c_str());

    // Front-code against whichever of the last 255 skeletons shares the longest
    // prefix; d, lcp and suf are each one metadata byte, so 255 is the reach.
    uint32_t best_d = 0, best_lcp = 0;
    size_t reach = hist.size() < 255 ? hist.size() : 255;
    for (size_t j = 1; j <= reach; j++) {
      const std::string& ref = hist[hist.size() - j];
      size_t l = 0, lim = ref.size() < sk.size() ? ref.size() : sk.size();
      if (lim > 255) lim = 255;
      while (l < lim && ref[l] == sk[l]) l++;
      if (l > best_lcp) { best_lcp = uint32_t(l); best_d = uint32_t(j); }
      if (best_lcp == sk.size()) break;   // cannot do better
    }
    if (best_lcp == 0) best_d = 0;        // d>0 with lcp==0 buys nothing
    uint32_t suf = uint32_t(sk.size()) - best_lcp;
    put_meta(best_d);
    put_meta(best_lcp);
    put_meta(suf);
    uint32_t ctx = best_lcp ? uint8_t(sk[best_lcp - 1]) : 0;
    for (uint32_t k = 0; k < suf; k++) {
      uint8_t ch = uint8_t(sk[best_lcp + k]);
      enc.encode_tree(&m.name[size_t(ctx) * 256], 8, ch);
      ctx = ch;
    }
    hist.push_back(sk);
    for (uint32_t v : nums) put_varint(v);

    put_meta(t.dtype);
    put_meta(uint32_t(t.shape.size()));
    for (uint32_t d : t.shape) put_varint(d);
    put_meta(t.enc);

    size_t bytes = t.numel * dtype_size(t.dtype);
    switch (t.enc) {
      case ENC_INT4: {
        put_meta(t.cls);
        const int8_t* in = reinterpret_cast<const int8_t*>(t.data.data());
        const uint32_t* c = m.cnt[t.cls];
        for (size_t k = 0; k < t.numel; k++) {
          uint32_t s = uint32_t(int(in[k]) + 7);
          if (s >= V3_NSYM) die("%s: int4 value %d out of range", t.name.c_str(), int(in[k]));
          enc.encode_freq(c, V3_NSYM, m.tot[t.cls], s);
          m.int4_update(t.cls, s);
        }
        break;
      }
      case ENC_BF16: {
        const uint16_t* in = reinterpret_cast<const uint16_t*>(t.data.data());
        for (size_t k = 0; k < t.numel; k++) {
          uint32_t hi = in[k] >> 8, lo = in[k] & 0xFF;
          enc.encode_tree(m.bf16_hi.data(), 8, hi);
          enc.encode_tree(&m.bf16_lo[hi * 256], 8, lo);
        }
        break;
      }
      case ENC_BF16_F32: {
        const uint32_t* in = reinterpret_cast<const uint32_t*>(t.data.data());
        for (size_t k = 0; k < t.numel; k++) {
          if (in[k] & 0xFFFFu)
            die("%s: value %zu is not a bfloat16 - ENC_BF16_F32 would lose it", t.name.c_str(), k);
          uint32_t b = in[k] >> 16, hi = b >> 8, lo = b & 0xFF;
          enc.encode_tree(m.rf32_hi.data(), 8, hi);
          enc.encode_tree(&m.rf32_lo[hi * 256], 8, lo);
        }
        break;
      }
      case ENC_PLANE4:
        for (size_t k = 0; k < bytes; k++)
          enc.encode_tree(&m.plane[(k & 3) * 256], 8, t.data[k]);
        break;
      case ENC_ROPE_SIN: case ENC_ROPE_COS:
        break;
      case ENC_RAW:
        for (size_t k = 0; k < bytes; k++) enc.encode_tree(m.raw.data(), 8, t.data[k]);
        break;
      default:
        die("%s: unknown encoding %u", t.name.c_str(), t.enc);
    }
  }
  enc.flush();
}

// ---------------------------------------------------------------------------
// conversion
// ---------------------------------------------------------------------------

// v3 -> v2: v2 has no ENC_BF16_F32, so a tensor carried as bfloat16 becomes a
// plain f32 payload holding the very same (already rounded) values.
void to_v2(Weights& w) {
  for (Tensor& t : w.t)
    if (t.enc == ENC_BF16_F32) t.enc = ENC_PLANE4;
}

// v2 -> v3: the reverse, but only where it costs nothing - a DT_F32 tensor
// every value of which is already a bfloat16. That is exactly the set this
// tool's own v3 -> v2 output has, and (almost) never anything else, so "back"
// really is back and an upstream v2 blob keeps every bit it had.
void to_v3(Weights& w) {
  int int4_cls = 0;
  std::vector<std::string> cls_of;
  for (Tensor& t : w.t) {
    if (t.enc == ENC_PLANE4 && t.dtype == DT_F32) {
      const uint32_t* v = reinterpret_cast<const uint32_t*>(t.data.data());
      bool all_bf16 = true;
      for (size_t k = 0; k < t.numel && all_bf16; k++) all_bf16 = (v[k] & 0xFFFFu) == 0;
      if (all_bf16 && t.numel) t.enc = ENC_BF16_F32;
    }
    if (t.enc == ENC_INT4) {
      // One adaptive count model per distinct int4 name skeleton - 13 of them
      // in this model, against the 16 the container allows.
      std::string sk;
      std::vector<uint32_t> nums;
      split_name(t.name, sk, nums);
      size_t j = 0;
      while (j < cls_of.size() && cls_of[j] != sk) j++;
      if (j == cls_of.size()) {
        if (int4_cls >= V3_NCLASS)
          die("more than %d distinct int4 tensor shapes of name - the v3 container has no class left", V3_NCLASS);
        cls_of.push_back(sk);
        int4_cls++;
      }
      t.cls = uint8_t(j);
    }
  }
}

const char* enc_name(uint8_t e) {
  static const char* n[] = {"raw","int4","bf16","plane4","rope.sin","rope.cos","bf16_f32"};
  return e < 7 ? n[e] : "?";
}

void report(const Weights& w, const char* what) {
  size_t counts[8] = {0}, payload = 0;
  for (const Tensor& t : w.t) {
    counts[t.enc < 7 ? t.enc : 7]++;
    payload += t.numel * dtype_size(t.dtype);
  }
  std::fprintf(stderr, "3to2: %s: %zu tensors, %zu bytes of payload\n", what, w.t.size(), payload);
  std::fprintf(stderr, "3to2:   ");
  for (int e = 0; e < 7; e++) if (counts[e]) std::fprintf(stderr, "%s=%zu ", enc_name(uint8_t(e)), counts[e]);
  std::fprintf(stderr, "\n");
}

// Every conversion checks itself: the file just written is decoded again and
// compared, tensor by tensor, with what came out of the input. A container
// this tool cannot read back is a container it has no business writing.
void verify(const Weights& a, const std::vector<uint8_t>& out, const char* path, int v3) {
  Weights b = v3 ? read_v3(out, "<output>") : read_v2(out, "<output>");
  if (a.t.size() != b.t.size()) die("%s: re-read gives %zu tensors, not %zu", path, b.t.size(), a.t.size());
  for (size_t i = 0; i < a.t.size(); i++) {
    const Tensor& x = a.t[i];
    const Tensor& y = b.t[i];
    if (x.name != y.name) die("%s: tensor %zu is \"%s\", not \"%s\"", path, i, y.name.c_str(), x.name.c_str());
    if (x.dtype != y.dtype || x.shape != y.shape || x.numel != y.numel)
      die("%s: %s: dtype or shape changed", path, x.name.c_str());
    if (x.data.size() != y.data.size() || (x.data.size() && std::memcmp(x.data.data(), y.data.data(), x.data.size())))
      die("%s: %s: payload changed", path, x.name.c_str());
  }
  std::fprintf(stderr, "3to2: verified - %zu tensors decode back identically\n", a.t.size());
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 4 || (argv[1][0] != 'c' && argv[1][0] != 'd') || argv[1][1]) {
    std::fprintf(stderr,
      "3to2 - convert transformer weight containers (FX2TFWC2 <-> FX2TFWC3)\n"
      "\n"
      "  3to2 c <in.tfwc3> <out.tfwc2>   v3 -> v2, which is what coder0 reads\n"
      "  3to2 d <in.tfwc2> <out.tfwc3>   v2 -> v3, the way back\n"
      "\n"
      "No tensor value changes in either direction. Both are checked by\n"
      "decoding the output again and comparing it with the input.\n");
    return 1;
  }
  int to2 = argv[1][0] == 'c';
  const char* in_path = argv[2];
  const char* out_path = argv[3];

  std::vector<uint8_t> buf = read_file(in_path);
  int src_v3;
  if (std::memcmp(buf.data(), "FX2TFWC3", 8) == 0) src_v3 = 1;
  else if (std::memcmp(buf.data(), "FX2TFWC2", 8) == 0) src_v3 = 0;
  else die("%s: not an FX2TFWC2 or FX2TFWC3 container (magic %.8s)", in_path, (const char*)buf.data());

  if (src_v3 != to2)
    die("%s is FX2TFWC%d; \"%c\" converts FX2TFWC%d to FX2TFWC%d. Use \"%c\".",
        in_path, src_v3 ? 3 : 2, argv[1][0], to2 ? 3 : 2, to2 ? 2 : 3, to2 ? 'd' : 'c');

  Weights w = src_v3 ? read_v3(buf, in_path) : read_v2(buf, in_path);
  report(w, in_path);

  if (to2) to_v2(w); else to_v3(w);

  std::vector<uint8_t> out;
  if (to2) write_v2(w, out); else write_v3(w, out);
  report(w, out_path);
  verify(w, out, out_path, !to2);
  write_file(out_path, out);

  std::fprintf(stderr, "3to2: %s (%zu B) -> %s (%zu B), %+lld\n",
               in_path, buf.size(), out_path, out.size(),
               (long long)out.size() - (long long)buf.size());
  return 0;
}
