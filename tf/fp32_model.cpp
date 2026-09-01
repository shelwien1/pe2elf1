#include "fp32_model.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#if defined(__AVX2__)
#include <immintrin.h>
#endif

#include "weights_io.h"

namespace fx2 {
namespace f32 {

namespace {

constexpr int V = 205, D = 192, NL = 12, DH = 64, NH = 3, DMLP = 768;
constexpr int WIN = 1024, ROPE_LEN = 131072;
constexpr bool KIMI[NL] = {true, true, true,  false, true, true,
                           true, false, true, true,  true, false};
constexpr float RMS_EPS = 1.1920928955078125e-07f;  // FLT_EPSILON, as in torch

[[noreturn]] void die(const char* msg) {
  std::fprintf(stderr, "fp32_model: %s\n", msg);
  std::exit(1);
}

// ---------------------------------------------------------------------------
// primitives (all of these are the reference kernels, unchanged apart from
// taking fp32 activations where the reference took quantized ones)
// ---------------------------------------------------------------------------

float sum_squares(const float* x, int n) {
  float s = 0.0f;
  for (int i = 0; i < n; i++) s += x[i] * x[i];
  return s;
}

void rms_norm_vec(const float* x, float* y, int n) {
  const float d = std::sqrt(sum_squares(x, n) / float(n) + RMS_EPS);
  const float inv = 1.0f / d;
  for (int i = 0; i < n; i++) y[i] = x[i] * inv;
}

float dot_f32(const float* a, const float* b, int n) {
#if defined(__AVX2__)
  __m256 s0 = _mm256_setzero_ps(), s1 = _mm256_setzero_ps();
  int i = 0;
  for (; i + 16 <= n; i += 16) {
    s0 = _mm256_fmadd_ps(_mm256_loadu_ps(a + i), _mm256_loadu_ps(b + i), s0);
    s1 = _mm256_fmadd_ps(_mm256_loadu_ps(a + i + 8), _mm256_loadu_ps(b + i + 8),
                         s1);
  }
  for (; i + 8 <= n; i += 8)
    s0 = _mm256_fmadd_ps(_mm256_loadu_ps(a + i), _mm256_loadu_ps(b + i), s0);
  __m256 s = _mm256_add_ps(s0, s1);
  __m128 v = _mm_add_ps(_mm256_castps256_ps128(s), _mm256_extractf128_ps(s, 1));
  v = _mm_add_ps(v, _mm_movehl_ps(v, v));
  v = _mm_add_ss(v, _mm_shuffle_ps(v, v, 1));
  float r = _mm_cvtss_f32(v);
  for (; i < n; i++) r += a[i] * b[i];
  return r;
#else
  float r = 0.0f;
  for (int i = 0; i < n; i++) r += a[i] * b[i];
  return r;
#endif
}

float sigmoid1f(float z) { return 1.0f / (1.0f + std::exp(-z)); }
float siluf(float z) { return z / (1.0f + std::exp(-z)); }
float softplus20f(float x) { return (x > 20.0f) ? x : std::log1p(std::exp(x)); }

// causal conv, kernel 4, per channel, then SiLU (SPEC 3.8)
void conv4_silu(const float* wt, const float* h0, const float* h1,
                const float* h2, const float* xc, float* y, int d) {
  const float* w0 = wt;
  const float* w1 = wt + d;
  const float* w2 = wt + 2 * d;
  const float* w3 = wt + 3 * d;
  for (int c = 0; c < d; c++)
    y[c] = siluf(w0[c] * h0[c] + w1[c] * h1[c] + w2[c] * h2[c] + w3[c] * xc[c]);
}

// one KDA head step (KIMI_SEMANTICS): decay the state, delta rule, read out
void kda_head_step(float* S, const float* q, const float* k, const float* v,
                   const float* g_raw, const float* dt_bias, float a_neg,
                   float beta, float* o) {
  float qn[DH], kn[DH], decay[DH], r[DH], u[DH];
  const float dq = std::sqrt(sum_squares(q, DH) + 1e-6f);
  const float dk = std::sqrt(sum_squares(k, DH) + 1e-6f);
  for (int i = 0; i < DH; i++) qn[i] = q[i] / dq;
  for (int i = 0; i < DH; i++) kn[i] = k[i] / dk;
  for (int i = 0; i < DH; i++)
    decay[i] = std::exp(a_neg * softplus20f(g_raw[i] + dt_bias[i]));

  for (int j = 0; j < DH; j++) r[j] = 0.0f;
  for (int i = 0; i < DH; i++) {
    float* row = S + DH * i;
    const float d = decay[i], kni = kn[i];
    for (int j = 0; j < DH; j++) {
      row[j] *= d;
      r[j] += kni * row[j];
    }
  }
  for (int j = 0; j < DH; j++) u[j] = beta * (v[j] - r[j]);
  for (int j = 0; j < DH; j++) o[j] = 0.0f;
  for (int i = 0; i < DH; i++) {
    float* row = S + DH * i;
    const float kni = kn[i], qi = 0.125f * qn[i];
    for (int j = 0; j < DH; j++) {
      row[j] += kni * u[j];
      o[j] += qi * row[j];
    }
  }
}

void gated_rms_norm64(const float* o, const float* og, const float* w,
                      float* y) {
  const float rstd = 1.0f / std::sqrt(sum_squares(o, DH) / float(DH) + 1e-5f);
  for (int j = 0; j < DH; j++) y[j] = o[j] * rstd * w[j] * sigmoid1f(og[j]);
}

void rope_apply(float* h192, const float* sin32, const float* cos32) {
  for (int h = 0; h < NH; h++) {
    float* p = h192 + h * DH;
    for (int i = 0; i < 32; i++) {
      const float x0 = p[2 * i], x1 = p[2 * i + 1];
      const float c = cos32[i], s = sin32[i];
      p[2 * i] = x0 * c + x1 * s;
      p[2 * i + 1] = x1 * c - x0 * s;
    }
  }
}

// ---------------------------------------------------------------------------
// weights: every parameter is a plain float in one contiguous arena
// ---------------------------------------------------------------------------

struct Lin {
  float* w = nullptr;  // [d_out][d_in], row-major, no padding
  int d_out = 0, d_in = 0;
  void apply(const float* x, float* y) const {
    for (int o = 0; o < d_out; o++)
      y[o] = dot_f32(w + size_t(o) * d_in, x, d_in);
  }
};

struct KimiLayer {
  Lin qp, kp, vp, fg_up, fg_down, og_up, og_down, op;
  float* wt_q = nullptr;    // conv weights, tap-major [4][192]
  float* wt_k = nullptr;
  float* wt_v = nullptr;
  float* beta_w = nullptr;  // [3][192]
  float* dt_bias = nullptr; // [192]
  float* gn_w = nullptr;    // [64]
  float* log_decay = nullptr;  // [3]; the head uses -exp(log_decay[h])
};

struct VanLayer {
  Lin qp, kp, vp, op;
};

struct KimiState {
  float S[NH][DH * DH];
  float hist[3][3][D];  // conv history: [q/k/v][t-3, t-2, t-1][channel]
};

struct VanState {
  std::vector<float> kring, vring;  // WIN x D each, fp32
  VanState() : kring(size_t(WIN) * D, 0.0f), vring(size_t(WIN) * D, 0.0f) {}
};

}  // namespace

struct ModelImpl {
  std::vector<float> arena;  // every parameter, contiguous
  size_t used = 0;

  Lin prior_lin, unembed;
  KimiLayer kimi[9];
  VanLayer van[3];
  Lin mlp_up[NL], mlp_down[NL];
  int layer2kimi[NL] = {}, layer2van[NL] = {};
  float* rsc = nullptr;      // [NL]
  float* tec = nullptr;      // [NL]
  float* skip_w = nullptr;   // [6]
  float* tok_table = nullptr;  // [V][D], rms-normed embedding rows

  std::vector<float> rope_sin, rope_cos;
  size_t rope_len = 0;
  float inv_freq[32] = {};

  KimiState kst[9];
  VanState vst[3];
  float skip_store[6][D] = {};
  int64_t t = 0, rope_off = 0;

  alignas(64) float x[D] = {}, xn[D] = {}, yb[D] = {}, h768[DMLP] = {};
  alignas(64) float logits[V] = {};

  float* take(size_t n) {
    if (used + n > arena.size()) die("weight arena overflow");
    float* p = arena.data() + used;
    used += n;
    return p;
  }

  void load(const char* path, size_t rope_rows, uint64_t seed);
  void dequant(const WeightsFile& wf, const std::string& prefix, int d_out,
               int d_in, Lin* out);
  float* copy_f32(const WeightsFile& wf, const std::string& name,
                  std::initializer_list<uint32_t> shape, size_t n);
  void begin(int64_t off);
  void step(uint8_t token, const float* prior, float* probs_out);
  void kimi_attention(int ki, const float* xin, float* y);
  void van_attention(int vi, const float* xin, float* y);
};

// dequantize one weight matrix: w[o][i] = q[o][i] * row_scale[o].  The
// activation scale that the quantized path folds in here is simply dropped -
// there is no activation quantizer any more.
void ModelImpl::dequant(const WeightsFile& wf, const std::string& prefix,
                        int d_out, int d_in, Lin* out) {
  const WTensor& wq = wf.get(prefix + ".weight.q", DT_I8,
                             {uint32_t(d_out), uint32_t(d_in)});
  const WTensor& ws =
      wf.get(prefix + ".weight.scale", DT_BF16, {uint32_t(d_out)});
  out->d_out = d_out;
  out->d_in = d_in;
  out->w = take(size_t(d_out) * d_in);
  for (int o = 0; o < d_out; o++) {
    const float s = bf16_to_f32(ws.bf16_bits()[o]);
    const int8_t* src = wq.i8() + size_t(o) * d_in;
    float* dst = out->w + size_t(o) * d_in;
    for (int i = 0; i < d_in; i++) dst[i] = float(src[i]) * s;
  }
}

float* ModelImpl::copy_f32(const WeightsFile& wf, const std::string& name,
                           std::initializer_list<uint32_t> shape, size_t n) {
  const WTensor& t = wf.get(name, DT_F32, shape);
  float* p = take(n);
  std::memcpy(p, t.f32(), sizeof(float) * n);
  return p;
}

void ModelImpl::load(const char* path, size_t rope_rows, uint64_t seed) {
  g_rope_rows_limit = rope_rows;
  char magic[8] = {0};
  if (path) {
    if (FILE* f = std::fopen(path, "rb")) {
      if (std::fread(magic, 1, 8, f) != 8) magic[0] = 0;
      std::fclose(f);
    }
  }
  WeightsFile wf = !path ? WeightsFile::random(seed)
                   : std::memcmp(magic, "FX2TFWC", 7) == 0
                       ? WeightsFile::load_compressed(path)
                       : WeightsFile::load(path);

  {
    const WTensor& ci = wf.get("config.ints", DT_I32, {11});
    const int32_t want[11] = {V, D, NL, DH, NH, DMLP, WIN, DH, NH, 4, 10000};
    for (int i = 0; i < 11; i++)
      if (ci.i32()[i] != want[i]) die("config.ints mismatch");
    const WTensor& ck = wf.get("config.kimi", DT_I32, {NL});
    for (int i = 0; i < NL; i++)
      if ((ck.i32()[i] != 0) != KIMI[i]) die("config.kimi mismatch");
  }

  // 5,897,145 floats of parameters; round up
  arena.assign(size_t(6) << 20, 0.0f);

  {  // normed token embedding rows
    const WTensor& eq = wf.get("embedding.weight.q", DT_I8, {V, D});
    const WTensor& es = wf.get("embedding.weight.scale", DT_BF16, {V});
    tok_table = take(size_t(V) * D);
    for (int c = 0; c < V; c++) {
      const float s = bf16_to_f32(es.bf16_bits()[c]);
      float* row = tok_table + size_t(c) * D;
      for (int i = 0; i < D; i++) row[i] = float(eq.i8()[c * D + i]) * s;
      rms_norm_vec(row, row, D);
    }
  }

  dequant(wf, "prior_embedding", D, V, &prior_lin);
  dequant(wf, "unembedding", V, D, &unembed);
  skip_w = copy_f32(wf, "skip_connection_weights.value", {6}, 6);
  rsc = take(NL);
  tec = take(NL);

  int ki = 0, vi = 0;
  for (int l = 0; l < NL; l++) {
    const std::string b = "blocks." + std::to_string(l) + ".";
    const std::string a = b + "attention.";
    rsc[l] =
        wf.get(b + "residual_stream_coefficient.value", DT_F32, {1}).f32()[0];
    tec[l] =
        wf.get(b + "token_embedding_coefficient.value", DT_F32, {1}).f32()[0];

    if (KIMI[l]) {
      KimiLayer& L = kimi[ki];
      layer2kimi[l] = ki;
      dequant(wf, a + "query_projection", D, D, &L.qp);
      dequant(wf, a + "key_projection", D, D, &L.kp);
      dequant(wf, a + "value_projection", D, D, &L.vp);
      dequant(wf, a + "forget_gate_projection.up", DH, D, &L.fg_up);
      dequant(wf, a + "forget_gate_projection.down", D, DH, &L.fg_down);
      dequant(wf, a + "output_gate_projection.up", DH, D, &L.og_up);
      dequant(wf, a + "output_gate_projection.down", D, DH, &L.og_down);
      dequant(wf, a + "output_projection", D, D, &L.op);

      const char* convs[3] = {"query_convolution.weight",
                              "key_convolution.weight",
                              "value_convolution.weight"};
      float** dst[3] = {&L.wt_q, &L.wt_k, &L.wt_v};
      for (int c = 0; c < 3; c++) {
        const WTensor& cw = wf.get(a + convs[c], DT_F32, {D, 4});
        float* wt = take(size_t(4) * D);
        for (int j = 0; j < 4; j++)  // tap-major, as the reference repacks it
          for (int ch = 0; ch < D; ch++) wt[j * D + ch] = cw.f32()[ch * 4 + j];
        *dst[c] = wt;
      }
      L.beta_w = copy_f32(wf, a + "beta_projection.weight", {NH, D},
                          size_t(NH) * D);
      L.dt_bias = copy_f32(wf, a + "dt_bias", {D}, D);
      L.gn_w = copy_f32(wf, a + "output_fused_norm_gate.weight", {DH}, DH);
      // kept as the raw parameter (the head uses -exp of it) so that every
      // trainable number lives in the arena
      L.log_decay = copy_f32(wf, a + "log_baseline_decay_rate", {NH}, NH);
      ki++;
    } else {
      VanLayer& L = van[vi];
      layer2van[l] = vi;
      dequant(wf, a + "query_projection", D, D, &L.qp);
      dequant(wf, a + "key_projection", D, D, &L.kp);
      dequant(wf, a + "value_projection", D, D, &L.vp);
      dequant(wf, a + "output_projection", D, D, &L.op);
      vi++;
    }
    dequant(wf, b + "mlp.up", DMLP, D, &mlp_up[l]);
    dequant(wf, b + "mlp.down", D, DMLP, &mlp_down[l]);
  }
  if (ki != 9 || vi != 3) die("layer pattern mismatch");

  {
    const WTensor& fi = wf.get("rope.inv_freq", DT_F32, {32});
    std::memcpy(inv_freq, fi.f32(), sizeof(inv_freq));
    const WTensor& si = wf.get("rope.sin");
    const WTensor& co = wf.get("rope.cos");
    if (si.dtype != DT_F32 || si.shape.size() != 2 || si.shape[1] != 32 ||
        co.shape[0] != si.shape[0] || si.shape[0] > uint32_t(ROPE_LEN))
      die("bad rope.sin/rope.cos");
    rope_len = si.shape[0];
    rope_sin.assign(si.f32(), si.f32() + rope_len * 32);
    rope_cos.assign(co.f32(), co.f32() + rope_len * 32);
  }

  std::memset(kst, 0, sizeof(kst));
}

void ModelImpl::begin(int64_t off) {
  std::memset(kst, 0, sizeof(kst));
  t = 0;
  rope_off = off;
}

void ModelImpl::kimi_attention(int ki, const float* xin, float* y) {
  KimiLayer& L = kimi[ki];
  KimiState& st = kst[ki];
  float bq[D], bk[D], bv[D], cq[D], ck[D], cv[D];
  float fu[DH], graw[D], og192[D], braw[NH], o192[D], gn[D];

  L.qp.apply(xin, bq);
  L.kp.apply(xin, bk);
  L.vp.apply(xin, bv);

  conv4_silu(L.wt_q, st.hist[0][0], st.hist[0][1], st.hist[0][2], bq, cq, D);
  conv4_silu(L.wt_k, st.hist[1][0], st.hist[1][1], st.hist[1][2], bk, ck, D);
  conv4_silu(L.wt_v, st.hist[2][0], st.hist[2][1], st.hist[2][2], bv, cv, D);
  const float* newest[3] = {bq, bk, bv};
  for (int c = 0; c < 3; c++) {
    std::memmove(st.hist[c][0], st.hist[c][1], sizeof(float) * 2 * D);
    std::memcpy(st.hist[c][2], newest[c], sizeof(float) * D);
  }

  L.fg_up.apply(xin, fu);
  L.fg_down.apply(fu, graw);
  for (int h = 0; h < NH; h++)
    braw[h] = dot_f32(xin, L.beta_w + size_t(h) * D, D);
  L.og_up.apply(xin, fu);
  L.og_down.apply(fu, og192);

  for (int h = 0; h < NH; h++)
    kda_head_step(st.S[h], cq + h * DH, ck + h * DH, cv + h * DH,
                  graw + h * DH, L.dt_bias + h * DH,
                  -std::exp(L.log_decay[h]), sigmoid1f(braw[h]), o192 + h * DH);
  for (int h = 0; h < NH; h++)
    gated_rms_norm64(o192 + h * DH, og192 + h * DH, L.gn_w, gn + h * DH);
  L.op.apply(gn, y);
}

void ModelImpl::van_attention(int vi, const float* xin, float* y) {
  VanLayer& L = van[vi];
  VanState& st = vst[vi];
  float q[D], k[D], v[D], pre[D];

  L.qp.apply(xin, q);
  L.kp.apply(xin, k);
  L.vp.apply(xin, v);
  for (int h = 0; h < NH; h++) rms_norm_vec(q + h * DH, q + h * DH, DH);
  for (int h = 0; h < NH; h++) rms_norm_vec(k + h * DH, k + h * DH, DH);

  const int64_t pos = rope_off + t;
  const float *sp, *cp;
  float sbuf[32], cbuf[32];
  if (size_t(pos) < rope_len) {
    sp = rope_sin.data() + size_t(pos) * 32;
    cp = rope_cos.data() + size_t(pos) * 32;
  } else {
    const float fpos = float(pos);
    for (int i = 0; i < 32; i++) {
      sbuf[i] = std::sin(fpos * inv_freq[i]);
      cbuf[i] = std::cos(fpos * inv_freq[i]);
    }
    sp = sbuf;
    cp = cbuf;
  }
  rope_apply(q, sp, cp);
  rope_apply(k, sp, cp);

  const int slot = int(t % WIN);
  std::memcpy(st.kring.data() + size_t(slot) * D, k, sizeof(float) * D);
  std::memcpy(st.vring.data() + size_t(slot) * D, v, sizeof(float) * D);

  const int n = int(t + 1 < WIN ? t + 1 : WIN);
  float scores[WIN];
  for (int h = 0; h < NH; h++) {
    const float* qh = q + h * DH;
    float m = -1e30f;
    for (int j = 0; j < n; j++) {
      const float s =
          0.125f * dot_f32(qh, st.kring.data() + size_t(j) * D + h * DH, DH);
      scores[j] = s;
      if (s > m) m = s;
    }
    float acc[DH] = {}, den = 0.0f;
    for (int j = 0; j < n; j++) {
      const float e = std::exp(scores[j] - m);
      den += e;
      const float* vh = st.vring.data() + size_t(j) * D + h * DH;
      for (int i = 0; i < DH; i++) acc[i] += e * vh[i];
    }
    const float inv = 1.0f / den;
    for (int i = 0; i < DH; i++) pre[h * DH + i] = acc[i] * inv;
  }
  L.op.apply(pre, y);
}

void ModelImpl::step(uint8_t token, const float* prior, float* probs_out) {
  if (token >= V) die("token out of range");
  const float* tok = tok_table + size_t(token) * D;

  prior_lin.apply(prior, yb);
  rms_norm_vec(yb, yb, D);
  for (int i = 0; i < D; i++) x[i] = tok[i] + yb[i];

  for (int l = 0; l < NL; l++) {
    if (l >= 6) {
      const float* s = skip_store[11 - l];
      const float w = skip_w[l - 6];
      for (int i = 0; i < D; i++) x[i] += w * s[i];
    }
    {
      const float a = rsc[l], bc = tec[l];
      for (int i = 0; i < D; i++) x[i] = a * x[i] + bc * tok[i];
    }

    rms_norm_vec(x, xn, D);
    if (KIMI[l])
      kimi_attention(layer2kimi[l], xn, yb);
    else
      van_attention(layer2van[l], xn, yb);
    for (int i = 0; i < D; i++) x[i] += yb[i];

    rms_norm_vec(x, xn, D);
    mlp_up[l].apply(xn, h768);
    for (int j = 0; j < DMLP; j++) {
      const float h = h768[j];
      h768[j] = h > 0.0f ? h * h : 0.0f;  // relu squared
    }
    mlp_down[l].apply(h768, yb);
    for (int i = 0; i < D; i++) x[i] += yb[i];

    if (l < 6) std::memcpy(skip_store[l], x, sizeof(float) * D);
  }

  rms_norm_vec(x, xn, D);
  unembed.apply(xn, logits);
  for (int i = 0; i < V; i++) logits[i] = 15.0f * std::tanh(logits[i] / 15.0f);

  float m = logits[0];
  for (int i = 1; i < V; i++)
    if (logits[i] > m) m = logits[i];
  float den = 0.0f;
  for (int i = 0; i < V; i++) {
    const float e = std::exp(logits[i] - m);
    probs_out[i] = e;
    den += e;
  }
  for (int i = 0; i < V; i++) probs_out[i] /= den;
  t++;
}

Transformer32::Transformer32(const char* weights_path, size_t rope_rows,
                             uint64_t init_seed)
    : impl(new ModelImpl()) {
  impl->load(weights_path, rope_rows, init_seed);
}
Transformer32::~Transformer32() = default;

void Transformer32::begin_article(int64_t off) { impl->begin(off); }
void Transformer32::step(uint8_t token, const float* prior205,
                         float* probs_out) {
  impl->step(token, prior205, probs_out);
}
const float* Transformer32::last_logits() const { return impl->logits; }
const float* Transformer32::last_final_norm() const { return impl->xn; }
const float* Transformer32::unembed_rows() const { return impl->unembed.w; }
float* Transformer32::weights() { return impl->arena.data(); }
const float* Transformer32::weights() const { return impl->arena.data(); }
size_t Transformer32::weight_count() const { return impl->used; }

}  // namespace f32
}  // namespace fx2
