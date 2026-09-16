// weights_init: build a WeightsFile in memory instead of reading one, so the
// model can run with freshly initialized weights (coder0's TF_LOAD_WEIGHTS 0).
//
// It produces exactly the tensor set arena_build.cpp asks for, with the same
// names, dtypes and shapes, so the arena builder and the whole inference path
// are used unchanged.  Everything comes from a fixed-seed deterministic PRNG:
// the encoder and the decoder must generate bit-identical weights, so nothing
// here may depend on rand(), the locale, the clock or the allocator.
//
// The scheme is the reference one, transcribed from the fx2-cmix-transformer
// training code (pysrc/model.py, pysrc/quantization.py) and the recipe that
// produced 6m-q4-fp32 (training_recipes/train_6m_v2.py, quantize.py):
//
//   * weights           kaiming_uniform_ with torch's defaults, i.e. uniform
//                       over +-sqrt(6/fan_in)   (model.py:178-182)
//   * embedding and     normal_, i.e. N(0,1) - both are rms-normed downstream,
//     prior_embedding   so only their direction matters (model.py:183-184,
//                       train_6m_v2.py:69-70)
//   * weight quantizer  buckets=15, scale_init="scaled_max": the per-row scale
//                       is max|w| / qmax with qmax = 7 (quantize.py:31-37,
//                       quantization.py:92-95).  Scales are bf16 in the file,
//                       and SPEC.md section 2 rounds them to bf16 before use,
//                       so the rounded scale is what quantizes the row.
//   * activation        buckets=256, scale_init="scaled_mean": the scale is
//     quantizers        (2/sqrt(127)) * mean|x| over the block, averaged over
//                       calibration batches (quantize.py:39-45,
//                       quantization.py:86-89).  There is no calibration data
//                       here, so mean|x| is taken analytically per site type -
//                       see kMeanAbs* below.
//   * residual/token    1.0 and 0.0 (model.py:954-956)
//     coefficients
//   * skip weights      1.0 (model.py:1069-1071)
//   * log decay rate    log(uniform(1,16)) per head (model.py:782-784)
//   * dt_bias           inverse-softplus of exp(uniform(log 1e-3, log 0.1)),
//                       clamped at 1e-4 (model.py:786-795)
//   * gated norm gain   1.0 (fla's FusedRMSNormGated, model.py:805)

#include "weights_io.h"

namespace fx2 {

namespace {

constexpr int V = 205, D = 192, NL = 12, DH = 64, NH = 3, DMLP = 768;
constexpr int ROPE_LEN = 131072;
constexpr bool KIMI_L[NL] = {true, true, true,  false, true, true,
                             true, false, true, true,  true, false};

constexpr float kWQmax = 7.0f;    // weight quantizer, buckets = 15
constexpr float kAQmax = 127.0f;  // activation quantizer, buckets = 256

// mean|x| of an activation quantizer's block, for the "scaled_mean" rule.
// Every quantized activation site in this model is fed either an rms_norm
// output or an RMS-preserving matmul of one, so x ~ N(0,1) and
// mean|x| = sqrt(2/pi) -- except:
//   * mlp.down, whose input is relu(h)^2 with h ~ N(0,1): mean = 1/2
//   * prior_embedding, whose input is a probability vector summing to 1 over
//     the 205-element block: mean = 1/205
const float kMeanAbsNormal = 0.79788456f;  // sqrt(2/pi)
const float kMeanAbsRelu2 = 0.5f;
const float kMeanAbsPrior = 1.0f / float(V);

float act_scale(float mean_abs) {
  return (2.0f / std::sqrt(kAQmax)) * mean_abs;
}

struct Rng {
  uint64_t s;
  explicit Rng(uint64_t seed) : s(seed) {}
  uint64_t next() {  // splitmix64
    uint64_t z = (s += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
  }
  float uniform01() { return float(next() >> 40) * (1.0f / 16777216.0f); }
  float uniform(float lo, float hi) { return lo + (hi - lo) * uniform01(); }
  float normal() {  // Box-Muller; load-time only, so the cost is irrelevant
    float u = uniform01();
    if (u < 1e-7f) u = 1e-7f;
    return std::sqrt(-2.0f * std::log(u)) *
           std::cos(6.2831853071795864f * uniform01());
  }
};

uint16_t f32_to_bf16_bits(float f) {
  uint32_t u;
  std::memcpy(&u, &f, 4);
  u += 0x7FFFu + ((u >> 16) & 1u);  // round to nearest even
  return uint16_t(u >> 16);
}

WTensor make(uint8_t dtype, std::vector<uint32_t> shape) {
  WTensor t;
  t.dtype = dtype;
  t.shape = std::move(shape);
  t.numel = 1;
  for (uint32_t d : t.shape) t.numel *= d;
  const size_t esz = dtype == DT_I8 ? 1 : (dtype == DT_BF16 ? 2 : 4);
  t.data.assign(t.numel * esz, 0);
  return t;
}

struct Builder {
  WeightsFile wf;
  Rng rng;
  explicit Builder(uint64_t seed) : rng(seed) {}

  void put(const std::string& name, WTensor&& t) {
    wf.tensors.emplace(name, std::move(t));
  }

  float* f32(const std::string& name, std::vector<uint32_t> shape) {
    WTensor t = make(DT_F32, std::move(shape));
    float* p = reinterpret_cast<float*>(t.data.data());
    // the tensor is moved into the map, but its heap buffer is not
    float* keep = p;
    put(name, std::move(t));
    return keep;
  }

  void f32_const(const std::string& name, std::vector<uint32_t> shape,
                 float c) {
    WTensor t = make(DT_F32, std::move(shape));
    float* p = reinterpret_cast<float*>(t.data.data());
    for (size_t i = 0; i < t.numel; i++) p[i] = c;
    put(name, std::move(t));
  }

  void bf16_const(const std::string& name, std::vector<uint32_t> shape,
                  float c) {
    WTensor t = make(DT_BF16, std::move(shape));
    uint16_t* p = reinterpret_cast<uint16_t*>(t.data.data());
    const uint16_t b = f32_to_bf16_bits(c);
    for (size_t i = 0; i < t.numel; i++) p[i] = b;
    put(name, std::move(t));
  }

  // kaiming_uniform_ with torch's defaults on a (d_out, d_in) matrix
  void fill_kaiming(float* w, int d_out, int d_in) {
    const float bound = std::sqrt(6.0f / float(d_in));
    for (size_t i = 0; i < size_t(d_out) * d_in; i++)
      w[i] = rng.uniform(-bound, bound);
  }
  void fill_normal(float* w, size_t n) {
    for (size_t i = 0; i < n; i++) w[i] = rng.normal();
  }

  // int4 weight matrix + per-row "scaled_max" scale, exactly as the training
  // code quantizes it: bf16-round the scale first, then divide by it.
  void quantize_rows(const std::string& prefix, const float* w, int d_out,
                     int d_in) {
    WTensor q = make(DT_I8, {uint32_t(d_out), uint32_t(d_in)});
    WTensor s = make(DT_BF16, {uint32_t(d_out)});
    int8_t* qp = reinterpret_cast<int8_t*>(q.data.data());
    uint16_t* sp = reinterpret_cast<uint16_t*>(s.data.data());
    for (int o = 0; o < d_out; o++) {
      const float* row = w + size_t(o) * d_in;
      float amax = 0.0f;
      for (int i = 0; i < d_in; i++) {
        const float a = std::fabs(row[i]);
        if (a > amax) amax = a;
      }
      float sc = amax / kWQmax;
      if (!(sc > 0.0f)) sc = 1.0f;  // an all-zero row cannot happen here
      sp[o] = f32_to_bf16_bits(sc);
      sc = bf16_to_f32(sp[o]);
      for (int i = 0; i < d_in; i++) {
        float v = std::nearbyint(row[i] / sc);  // round half to even
        if (v > kWQmax) v = kWQmax;
        if (v < -kWQmax) v = -kWQmax;
        qp[size_t(o) * d_in + i] = int8_t(v);
      }
    }
    put(prefix + ".weight.q", std::move(q));
    put(prefix + ".weight.scale", std::move(s));
  }

  void qweight_kaiming(const std::string& prefix, int d_out, int d_in) {
    std::vector<float> w(size_t(d_out) * d_in);
    fill_kaiming(w.data(), d_out, d_in);
    quantize_rows(prefix, w.data(), d_out, d_in);
  }
  void qweight_normal(const std::string& prefix, int d_out, int d_in) {
    std::vector<float> w(size_t(d_out) * d_in);
    fill_normal(w.data(), w.size());
    quantize_rows(prefix, w.data(), d_out, d_in);
  }

  // a full quantized matmul site
  void qsite(const std::string& prefix, int d_out, int d_in, float mean_abs) {
    qweight_kaiming(prefix, d_out, d_in);
    bf16_const(prefix + ".quantize_activation.scale", {1},
               act_scale(mean_abs));
  }
};

}  // namespace

WeightsFile WeightsFile::random(uint64_t seed) {
  Builder b(seed);

  {  // config: arena_build asserts on these, so they must be exact
    WTensor ci = make(DT_I32, {11});
    const int32_t vals[11] = {V, D, NL, DH, NH, DMLP, 1024, DH, NH, 4, 10000};
    std::memcpy(ci.data.data(), vals, sizeof(vals));
    b.put("config.ints", std::move(ci));
    WTensor ck = make(DT_I32, {NL});
    int32_t* kp = reinterpret_cast<int32_t*>(ck.data.data());
    for (int l = 0; l < NL; l++) kp[l] = KIMI_L[l] ? 1 : 0;
    b.put("config.kimi", std::move(ck));
  }

  b.qweight_normal("embedding", V, D);      // rms-normed at load
  b.qweight_normal("prior_embedding", D, V);
  b.bf16_const("prior_embedding.quantize_activation.scale", {1},
               act_scale(kMeanAbsPrior));
  b.qsite("unembedding", V, D, kMeanAbsNormal);

  b.f32_const("skip_connection_weights.value", {6}, 1.0f);

  for (int l = 0; l < NL; l++) {
    const std::string bl = "blocks." + std::to_string(l) + ".";
    const std::string a = bl + "attention.";
    b.f32_const(bl + "residual_stream_coefficient.value", {1}, 1.0f);
    b.f32_const(bl + "token_embedding_coefficient.value", {1}, 0.0f);

    b.qsite(a + "query_projection", D, D, kMeanAbsNormal);
    b.qsite(a + "key_projection", D, D, kMeanAbsNormal);
    b.qsite(a + "value_projection", D, D, kMeanAbsNormal);
    b.qsite(a + "output_projection", D, D, kMeanAbsNormal);

    if (KIMI_L[l]) {
      b.qsite(a + "forget_gate_projection.up", DH, D, kMeanAbsNormal);
      b.qsite(a + "forget_gate_projection.down", D, DH, kMeanAbsNormal);
      b.qsite(a + "output_gate_projection.up", DH, D, kMeanAbsNormal);
      b.qsite(a + "output_gate_projection.down", D, DH, kMeanAbsNormal);

      {  // beta_projection: unquantized fp32, kaiming over fan_in = d_model
        float* w = b.f32(a + "beta_projection.weight", {NH, D});
        b.fill_kaiming(w, NH, D);
      }
      for (const char* c : {"query_convolution.weight",
                            "key_convolution.weight",
                            "value_convolution.weight"}) {
        // kaiming over the (d, kernel_size) matrix, so fan_in = 4
        float* w = b.f32(a + c, {D, 4});
        b.fill_kaiming(w, D, 4);
      }
      {  // dt_bias = inverse softplus of a log-uniform dt in [1e-3, 0.1]
        float* p = b.f32(a + "dt_bias", {D});
        for (int i = 0; i < D; i++) {
          float dt = std::exp(b.rng.uniform(std::log(0.001f), std::log(0.1f)));
          if (dt < 1e-4f) dt = 1e-4f;
          p[i] = dt + std::log(-std::expm1(-dt));
        }
      }
      b.f32_const(a + "output_fused_norm_gate.weight", {DH}, 1.0f);
      {  // consumed as -exp(x), giving a decay rate in [-16, -1]
        float* p = b.f32(a + "log_baseline_decay_rate", {NH});
        for (int h = 0; h < NH; h++) p[h] = std::log(b.rng.uniform(1.0f, 16.0f));
      }
    } else {
      // per-head post-rope q/k/v quantizers; q and k are rms-normed per head
      // and v is an RMS-preserving projection, so all three see N(0,1)
      const float s = act_scale(kMeanAbsNormal);
      b.bf16_const(a + "quantize_queries.scale", {NH}, s);
      b.bf16_const(a + "quantize_keys.scale", {NH}, s);
      b.bf16_const(a + "quantize_values.scale", {NH}, s);
    }

    b.qsite(bl + "mlp.up", DMLP, D, kMeanAbsNormal);
    b.qsite(bl + "mlp.down", D, DMLP, kMeanAbsRelu2);
  }

  {  // rope: inv_freq[i] = 10000^(-i/31) (SPEC 3.6), tables from libm
    float* f = b.f32("rope.inv_freq", {32});
    for (int i = 0; i < 32; i++)
      f[i] = 1.0f / std::pow(10000.0f, float(i) / 31.0f);
    const uint32_t rows =
        (g_rope_rows_limit != 0 && g_rope_rows_limit < size_t(ROPE_LEN))
            ? uint32_t(g_rope_rows_limit)
            : uint32_t(ROPE_LEN);
    float* sp = b.f32("rope.sin", {rows, 32});
    float* cp = b.f32("rope.cos", {rows, 32});
    for (uint32_t pos = 0; pos < rows; pos++)
      for (int i = 0; i < 32; i++) {
        const float ang = float(pos) * f[i];
        sp[pos * 32 + i] = std::sin(ang);
        cp[pos * 32 + i] = std::cos(ang);
      }
  }

  return std::move(b.wf);
}

}  // namespace fx2
