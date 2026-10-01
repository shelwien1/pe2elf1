// weights_write: encoder for the FX2TFWC2 weights format, the mirror of the
// decoder in weights_io_compressed.inc.  It lets coder0 save the model it ends
// a run with, so a subsequent run can start from it instead of from the
// shipped checkpoint.
//
// The two files must stay in exact sync.  Every model here is the same size,
// with the same initial probability and the same shift-5 update as the
// decoder's; every symbol is coded MSB-first through the same bit tree; the
// per-tensor metadata, the name characters and the four payload encodings are
// written in the order load_v2() reads them.  rope.sin / rope.cos carry no
// payload at all - the decoder recomputes them from rope.inv_freq - so
// inv_freq has to be written before them.

#include "weights_io.h"

namespace fx2 {

namespace {

// LZMA-style binary range encoder.  The decoder skips one leading byte (the
// initial zero cache) and then primes on four, which is exactly what the first
// shift_low and the five-byte flush below produce.
struct BinEncoder {
  std::vector<uint8_t> out;
  uint64_t low = 0;
  uint32_t range = 0xFFFFFFFFu;
  uint8_t cache = 0;
  int64_t cache_size = 1;

  void shift_low() {
    if (low < 0xFF000000ull || low > 0xFFFFFFFFull) {
      const uint8_t carry = uint8_t(low >> 32);
      do {
        out.push_back(uint8_t(cache + carry));
        cache = 0xFF;
      } while (--cache_size);
      cache = uint8_t((low >> 24) & 0xFF);
    }
    cache_size++;
    low = (low << 8) & 0xFFFFFFFFull;
  }

  void encode_bit(uint16_t* prob, int bit) {
    const uint32_t bound = (range >> 11) * *prob;
    if (!bit) {
      range = bound;
      *prob = uint16_t(*prob + ((2048 - *prob) >> 5));
    } else {
      low += bound;
      range -= bound;
      *prob = uint16_t(*prob - (*prob >> 5));
    }
    while (range < (1u << 24)) {
      range <<= 8;
      shift_low();
    }
  }

  // probs: (1 << nbits) entries, indices 1.. used; MSB first, as decode_tree
  void encode_tree(uint16_t* probs, int nbits, uint32_t sym) {
    uint32_t node = 1;
    for (int k = nbits - 1; k >= 0; k--) {
      const int bit = int((sym >> k) & 1);
      encode_bit(&probs[node], bit);
      node = (node << 1) | uint32_t(bit);
    }
  }

  void flush() {
    for (int i = 0; i < 5; i++) shift_low();
  }
};

uint16_t f32_to_bf16(float f) {
  uint32_t u;
  memcpy(&u, &f, 4);
  u += 0x7FFFu + ((u >> 16) & 1u);  // round to nearest even
  return uint16_t(u >> 16);
}

// The reference "scaled_max" weight quantizer (pysrc/quantization.py): the
// row's scale is max|w|/7, rounded to bf16 because that is what the file
// stores and what the loader will use, and the row is then rounded to the 15
// int4 levels against that rounded scale.
//
// `prev_bits` is the scale the row was loaded with, or 0.  Keeping it when it
// still covers the row matters: these weights were dequantized from int4, so
// with the original scale every value lands back on its own level and an
// untrained save/load round trip is exact.  Re-fitting to max|w|/7 instead
// would rescale any row that does not reach +-7 - a row whose largest level is
// 5 would be stretched by 7/5 and re-rounded, moving most of its weights.
void quantize_row_int4(const float* w, int n, int8_t* q, uint16_t* scale_bits,
                       uint16_t prev_bits = 0) {
  float amax = 0.0f;
  for (int i = 0; i < n; i++) {
    const float a = fabsf(w[i]);
    if (a > amax) amax = a;
  }
  float sc = amax / 7.0f;
  if (prev_bits) {
    // still covers the row.  Trained scales can be negative (the file stores
    // what training produced, e.g. rows of the block-3/7 q/k projections),
    // and an all-zero row keeps whatever it had: either way the loaded row
    // then re-quantizes to exactly its own values
    const float prev = bf16_to_f32(prev_bits);
    if (prev != 0.0f && fabsf(prev) >= sc) sc = prev;
  }
  if (!(fabsf(sc) > 0.0f)) sc = 1.0f;  // an all-zero row: any scale will do
  *scale_bits = f32_to_bf16(sc);
  sc = bf16_to_f32(*scale_bits);
  for (int i = 0; i < n; i++) {
    float v = nearbyintf(w[i] / sc);
    if (v > 7.0f) v = 7.0f;
    if (v < -7.0f) v = -7.0f;
    q[i] = int8_t(v);
  }
}

// The same for a plain int8 row (cmix-lex-transformer's 8-bit small matrices,
// pysrc/export_weights.py FX2_WBITS_SMALL=8): scale max|w|/127, values
// rounded and clamped to [-127,127].
void quantize_row_int8(const float* w, int n, int8_t* q, uint16_t* scale_bits,
                       uint16_t prev_bits = 0) {
  float amax = 0.0f;
  for (int i = 0; i < n; i++) {
    const float a = fabsf(w[i]);
    if (a > amax) amax = a;
  }
  float sc = amax / 127.0f;
  if (prev_bits) {  // as in quantize_row_int4
    const float prev = bf16_to_f32(prev_bits);
    if (prev != 0.0f && fabsf(prev) >= sc) sc = prev;
  }
  if (!(fabsf(sc) > 0.0f)) sc = 1.0f;
  *scale_bits = f32_to_bf16(sc);
  sc = bf16_to_f32(*scale_bits);
  for (int i = 0; i < n; i++) {
    float v = nearbyintf(w[i] / sc);
    if (v > 127.0f) v = 127.0f;
    if (v < -127.0f) v = -127.0f;
    q[i] = int8_t(v);
  }
}

// ... and for a codebook row (cmix-lex-transformer, FX2_WCODEBOOK=1): weight =
// scale * grid * cb[q + 7], so the scale is max|w| / (grid * max|level|) and
// every weight takes the index of the nearest of the 15 levels.  With the
// loaded scale kept, an untrained weight is exactly level * (scale*grid) and
// comes back to its own index.
void quantize_row_codebook(const float* w, int n, const int8_t* cb, float grid,
                           int8_t* q, uint16_t* scale_bits,
                           uint16_t prev_bits = 0) {
  int lmax = 1;
  for (int k = 0; k < 15; k++) {
    const int a = cb[k] < 0 ? -cb[k] : cb[k];
    if (a > lmax) lmax = a;
  }
  float amax = 0.0f;
  for (int i = 0; i < n; i++) {
    const float a = fabsf(w[i]);
    if (a > amax) amax = a;
  }
  float sc = amax / (grid * float(lmax));
  if (prev_bits) {  // as in quantize_row_int4
    const float prev = bf16_to_f32(prev_bits);
    if (prev != 0.0f && fabsf(prev) >= sc) sc = prev;
  }
  if (!(fabsf(sc) > 0.0f)) sc = 1.0f;
  *scale_bits = f32_to_bf16(sc);
  const float sg = bf16_to_f32(*scale_bits) * grid;
  for (int i = 0; i < n; i++) {
    const float t = w[i] / sg;
    int best = 0;
    float bd = fabsf(t - float(cb[0]));
    for (int k = 1; k < 15; k++) {
      const float d = fabsf(t - float(cb[k]));
      if (d < bd) {
        bd = d;
        best = k;
      }
    }
    q[i] = int8_t(best - 7);
  }
}

// ENC_ADAPT encoder, the mirror of BinDecoder::decode_adapt: symbol s < K-1
// owns [r*cum, r*(cum+cnt)), the last symbol takes the remainder of the range
void encode_adapt(BinEncoder& enc, AdaptModel& m, uint32_t s) {
  const uint32_t r = enc.range / m.tot;
  uint32_t cum = 0;
  for (uint32_t k = 0; k < s; k++) cum += m.cnt[k];
  enc.low += uint64_t(r) * cum;
  enc.range = s == m.k - 1 ? enc.range - r * cum : r * m.cnt[s];
  while (enc.range < (1u << 24)) {
    enc.range <<= 8;
    enc.shift_low();
  }
  m.update(s);
}

}  // namespace

// Writes wf as FX2TFWC2, or with lex3 as cmix-lex-transformer's FX2TFWC3 -
// the same stream with every int8 payload whose values fit [-127,127] coded
// as ENC_ADAPT (qmax = max|v|), which is what that project's "compress3"
// writes.  Tensors are emitted in the order given by `order` (names not
// present in wf are an error), which is how rope.inv_freq is guaranteed to
// precede rope.sin / rope.cos.
bool write_weights_v2(const char* path, const WeightsFile& wf,
                      const std::vector<std::string>& order, bool lex3) {
  BinEncoder enc;
  ModelsV2 m;
  uint8_t meta_prev = 0;

  auto put_meta = [&](uint8_t b) {
    enc.encode_tree(&m.meta[size_t(meta_prev) * 256], 8, b);
    meta_prev = b;
  };

  for (const std::string& name : order) {
    auto it = wf.tensors.find(name);
    if (it == wf.tensors.end()) {
      fprintf(stderr, "weights_write: %s is missing from the model\n",
              name.c_str());
      return false;
    }
    const WTensor& t = it->second;
    if (name.size() > 255) {
      fprintf(stderr, "weights_write: tensor name too long\n");
      return false;
    }

    put_meta(uint8_t(name.size()));
    uint32_t c2 = 0, c1 = 0;
    for (char ch : name) {
      const uint32_t u = uint8_t(ch);
      enc.encode_tree(&m.name[size_t((c2 << 8) | c1) * 256], 8, u);
      c2 = c1;
      c1 = u;
    }

    // the encoding a tensor gets is a function of what it is
    int qmax = -1;  // max |value| of an int8 tensor, -1 if it holds -128
    if (t.dtype == DT_I8) {
      qmax = 0;
      const int8_t* q = reinterpret_cast<const int8_t*>(t.data.data());
      for (size_t k = 0; k < t.numel && qmax >= 0; k++) {
        const int a = q[k] < 0 ? -int(q[k]) : int(q[k]);
        qmax = a > 127 ? -1 : (a > qmax ? a : qmax);
      }
    }
    uint8_t encoding;
    if (name == "rope.sin")
      encoding = ENC_ROPE_SIN;
    else if (name == "rope.cos")
      encoding = ENC_ROPE_COS;
    else if (t.dtype == DT_I8)
      encoding = lex3 && qmax >= 0 ? ENC_ADAPT
                 : qmax >= 0 && qmax <= 7 ? ENC_INT4 : ENC_RAW;
    else if (t.dtype == DT_BF16)
      encoding = ENC_BF16;
    else
      encoding = ENC_PLANE4;

    put_meta(t.dtype);
    put_meta(uint8_t(t.shape.size()));
    for (uint32_t d : t.shape)
      for (int k = 0; k < 4; k++) put_meta(uint8_t((d >> (8 * k)) & 0xFF));
    put_meta(encoding);

    const size_t bytes = t.data.size();
    switch (encoding) {
      case ENC_ADAPT: {
        put_meta(uint8_t(qmax));
        m.adapt.init(2u * uint32_t(qmax) + 1,
                     &m.adapt_glob[size_t(qmax) * kAdaptMaxK]);
        const int8_t* q = reinterpret_cast<const int8_t*>(t.data.data());
        for (size_t k = 0; k < t.numel; k++)
          encode_adapt(enc, m.adapt, uint32_t(int(q[k]) + qmax));
        break;
      }
      case ENC_INT4: {
        const int8_t* q = reinterpret_cast<const int8_t*>(t.data.data());
        for (size_t k = 0; k < t.numel; k++) {
          const int v = int(q[k]) + 7;
          if (v < 0 || v > 14) {
            fprintf(stderr, "weights_write: %s: int4 value out of range\n",
                    name.c_str());
            return false;
          }
          enc.encode_tree(m.int4.data(), 4, uint32_t(v));
        }
        break;
      }
      case ENC_BF16: {
        const uint16_t* p = reinterpret_cast<const uint16_t*>(t.data.data());
        for (size_t k = 0; k < t.numel; k++) {
          const uint32_t hi = (p[k] >> 8) & 0xFF, lo = p[k] & 0xFF;
          enc.encode_tree(m.bf16_hi.data(), 8, hi);
          enc.encode_tree(&m.bf16_lo[hi * 256], 8, lo);
        }
        break;
      }
      case ENC_PLANE4:
        for (size_t k = 0; k < bytes; k++)
          enc.encode_tree(&m.plane[(k & 3) * 256], 8, t.data[k]);
        break;
      case ENC_ROPE_SIN:
      case ENC_ROPE_COS:
        break;  // regenerated from rope.inv_freq on load
      default:
        for (size_t k = 0; k < bytes; k++)
          enc.encode_tree(m.raw.data(), 8, t.data[k]);
        break;
    }
  }
  enc.flush();

  FILE* f = fopen(path, "wb");
  if (!f) {
    fprintf(stderr, "weights_write: cannot open %s for writing\n", path);
    return false;
  }
  const uint32_t n = uint32_t(order.size());
  bool ok = fwrite(lex3 ? "FX2TFWC3" : "FX2TFWC2", 1, 8, f) == 8 &&
            fwrite(&n, 1, 4, f) == 4 &&
            fwrite(enc.out.data(), 1, enc.out.size(), f) == enc.out.size();
  fclose(f);
  if (!ok) fprintf(stderr, "weights_write: failed writing %s\n", path);
  return ok;
}

}  // namespace fx2
