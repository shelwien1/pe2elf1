#include "fp32_model.h"

#if defined(__AVX2__)
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
  fprintf(stderr, "fp32_model: %s\n", msg);
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

float rms_norm_vec(const float* x, float* y, int n) {
  const float d = std::sqrt(sum_squares(x, n) / float(n) + RMS_EPS);
  const float inv = 1.0f / d;
  for (int i = 0; i < n; i++) y[i] = x[i] * inv;
  return d;
}

// dL/dx of y = x/denom with denom = sqrt(mean(x^2)+eps):
//   dx = (dy - y * dot(dy,y)/n) / denom
void rms_norm_back(const float* dy, const float* y, float denom, int n,
                   float* dx) {
  float d = 0.0f;
  for (int i = 0; i < n; i++) d += dy[i] * y[i];
  d /= float(n);
  const float inv = 1.0f / denom;
  for (int i = 0; i < n; i++) dx[i] = (dy[i] - y[i] * d) * inv;
}

// dL/dx of y = x/denom with denom = sqrt(sum(x^2)+eps)  (the KDA l2 norm):
//   dx = (dy - y * dot(dy,y)) / denom
void l2_norm_back(const float* dy, const float* y, float denom, int n,
                  float* dx) {
  float d = 0.0f;
  for (int i = 0; i < n; i++) d += dy[i] * y[i];
  const float inv = 1.0f / denom;
  for (int i = 0; i < n; i++) dx[i] = (dy[i] - y[i] * d) * inv;
}

// y = W x : dW[o][i] += dy[o]*x[i] and dx[i] += sum_o W[o][i]*dy[o].
// Both are accumulated; the caller zeroes dx.
void lin_back(const float* W, const float* x, const float* dy, int d_out,
              int d_in, float* dW, float* dx) {
  for (int o = 0; o < d_out; o++) {
    const float g = dy[o];
    if (g != 0.0f) {
      float* dwr = dW + size_t(o) * d_in;
      for (int i = 0; i < d_in; i++) dwr[i] += g * x[i];
    }
    if (dx) {
      const float* wr = W + size_t(o) * d_in;
      for (int i = 0; i < d_in; i++) dx[i] += wr[i] * g;
    }
  }
}

float sigmoid_d(float s) { return s * (1.0f - s); }        // from sigmoid(z)
float silu_d(float z) {                                     // d/dz z*sigmoid(z)
  const float s = 1.0f / (1.0f + std::exp(-z));
  return s + z * s * (1.0f - s);
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
struct KdaRec {  // where kda_head_step records what the backward pass needs
  float *Sprev, *qn, *kn, *decay, *u, *r, *dqn, *dkn;
};

void kda_head_step(float* S, const float* q, const float* k, const float* v,
                   const float* g_raw, const float* dt_bias, float a_neg,
                   float beta, float* o, const KdaRec* rec = nullptr) {
  float qn[DH], kn[DH], decay[DH], r[DH], u[DH];
  const float dq = std::sqrt(sum_squares(q, DH) + 1e-6f);
  const float dk = std::sqrt(sum_squares(k, DH) + 1e-6f);
  for (int i = 0; i < DH; i++) qn[i] = q[i] / dq;
  for (int i = 0; i < DH; i++) kn[i] = k[i] / dk;
  for (int i = 0; i < DH; i++)
    decay[i] = std::exp(a_neg * softplus20f(g_raw[i] + dt_bias[i]));
  if (rec) {
    std::memcpy(rec->Sprev, S, sizeof(float) * DH * DH);  // before the decay
    std::memcpy(rec->qn, qn, sizeof(qn));
    std::memcpy(rec->kn, kn, sizeof(kn));
    std::memcpy(rec->decay, decay, sizeof(decay));
    *rec->dqn = dq;
    *rec->dkn = dk;
  }

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
  if (rec) {
    std::memcpy(rec->u, u, sizeof(u));
    std::memcpy(rec->r, r, sizeof(r));
  }
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

// transpose of rope_apply: y0 = x0*c + x1*s, y1 = x1*c - x0*s, so
// dx0 = dy0*c - dy1*s and dx1 = dy0*s + dy1*c
void rope_back(float* d192, const float* sin32, const float* cos32) {
  for (int h = 0; h < NH; h++) {
    float* p = d192 + h * DH;
    for (int i = 0; i < 32; i++) {
      const float d0 = p[2 * i], d1 = p[2 * i + 1];
      const float c = cos32[i], sn = sin32[i];
      p[2 * i] = d0 * c - d1 * sn;
      p[2 * i + 1] = d0 * sn + d1 * c;
    }
  }
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

// ---------------------------------------------------------------------------
// Forward tape: everything backward() needs from the step it is undoing.
// Recorded only while training.
// ---------------------------------------------------------------------------
struct KimiTape {
  float bq[D], bk[D], bv[D];      // projections = newest conv tap
  float h0[3][D], h1[3][D], h2[3][D];  // conv history that was used
  float cz[3][D];                 // conv output before SiLU
  float cq[D], ck[D], cv[D];      // after SiLU
  float fu_f[DH], graw[D], fu_o[DH], og[D];
  float braw[NH], beta[NH];
  float Sprev[NH][DH * DH];       // state before the decay
  float decay[NH][DH], qn[NH][DH], kn[NH][DH];
  float u[NH][DH], r[NH][DH];
  float dqn[NH], dkn[NH];         // the l2 denominators
  float o192[D], gn[D];
};

struct VanTape {
  float q_raw[D], k_raw[D], v_raw[D];
  float qn_pre[D], kn_pre[D];     // per-head rms_norm output, before RoPE
  float qd[NH], kd[NH];           // its denominators
  float q_rope[D];
  float sin32[32], cos32[32];
  float probs[NH][WIN];
  int n = 0, slot = 0;
  float pre[D];
};

struct LayerTape {
  float xpre[D];   // after the skip add, before the residual/token coefficients
  float xn1[D], xn2[D];
  float d1 = 0, d2 = 0;
  float x_mid[D];  // after the attention add
  float h[DMLP];   // mlp up output, before relu^2
  float r[DMLP];   // after relu^2
};

struct Tape {
  int token = 0;
  float prior[V];
  float pri_pre[D], pri_post[D];
  float pri_d = 0;
  float x_out[NL][D];   // block outputs (layers 0..5 are the skip sources)
  LayerTape lay[NL];
  KimiTape kimi[9];
  VanTape van[3];
  float xn_fin[D];
  float d_fin = 0;
  float cap[V];         // post-softcap logits
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

  // training state
  bool training_ = false;
  std::vector<float> grad, adam_m, adam_v;
  std::unique_ptr<Tape> tape;
  // gradient slots mirroring the weight layout, so a parameter pointer p maps
  // to its gradient as grad.data() + (p - arena.data())
  float* g(const float* p) const {
    return const_cast<float*>(grad.data()) + (p - arena.data());
  }

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
  void backward(const float* dcap);
  void kimi_backward(int ki, const float* dy, float* dxn);
  void van_backward(int vi, const float* dy, float* dxn);
  void adam_step(float lr, float wd, long tstep);
  size_t state_size() const;
  void state_save(void* dst) const;
  void state_load(const void* src);
  const float* attn_xn_ = nullptr;  // rms_norm(x) of the layer currently being
                                    // backpropagated; set by backward()
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
    if (FILE* f = fopen(path, "rb")) {
      if (fread(magic, 1, 8, f) != 8) magic[0] = 0;
      fclose(f);
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
  KimiTape* T = training_ ? &tape->kimi[ki] : nullptr;
  float bq[D], bk[D], bv[D], cq[D], ck[D], cv[D];
  float fu_f[DH], fu_o[DH], graw[D], og192[D], braw[NH], o192[D], gn[D];

  L.qp.apply(xin, bq);
  L.kp.apply(xin, bk);
  L.vp.apply(xin, bv);

  if (T) {
    for (int c = 0; c < 3; c++) {
      std::memcpy(T->h0[c], st.hist[c][0], sizeof(float) * D);
      std::memcpy(T->h1[c], st.hist[c][1], sizeof(float) * D);
      std::memcpy(T->h2[c], st.hist[c][2], sizeof(float) * D);
    }
    std::memcpy(T->bq, bq, sizeof(bq));
    std::memcpy(T->bk, bk, sizeof(bk));
    std::memcpy(T->bv, bv, sizeof(bv));
    const float* wts[3] = {L.wt_q, L.wt_k, L.wt_v};
    const float* xs[3] = {bq, bk, bv};
    for (int c = 0; c < 3; c++)
      for (int ch = 0; ch < D; ch++)
        T->cz[c][ch] = wts[c][ch] * st.hist[c][0][ch] +
                       wts[c][D + ch] * st.hist[c][1][ch] +
                       wts[c][2 * D + ch] * st.hist[c][2][ch] +
                       wts[c][3 * D + ch] * xs[c][ch];
  }

  conv4_silu(L.wt_q, st.hist[0][0], st.hist[0][1], st.hist[0][2], bq, cq, D);
  conv4_silu(L.wt_k, st.hist[1][0], st.hist[1][1], st.hist[1][2], bk, ck, D);
  conv4_silu(L.wt_v, st.hist[2][0], st.hist[2][1], st.hist[2][2], bv, cv, D);
  const float* newest[3] = {bq, bk, bv};
  for (int c = 0; c < 3; c++) {
    std::memmove(st.hist[c][0], st.hist[c][1], sizeof(float) * 2 * D);
    std::memcpy(st.hist[c][2], newest[c], sizeof(float) * D);
  }

  L.fg_up.apply(xin, fu_f);
  L.fg_down.apply(fu_f, graw);
  for (int h = 0; h < NH; h++)
    braw[h] = dot_f32(xin, L.beta_w + size_t(h) * D, D);
  L.og_up.apply(xin, fu_o);
  L.og_down.apply(fu_o, og192);

  for (int h = 0; h < NH; h++) {
    const float beta = sigmoid1f(braw[h]);
    if (T) T->beta[h] = beta;
    KdaRec rec;
    if (T) {
      rec.Sprev = T->Sprev[h];
      rec.qn = T->qn[h];
      rec.kn = T->kn[h];
      rec.decay = T->decay[h];
      rec.u = T->u[h];
      rec.r = T->r[h];
      rec.dqn = &T->dqn[h];
      rec.dkn = &T->dkn[h];
    }
    kda_head_step(st.S[h], cq + h * DH, ck + h * DH, cv + h * DH,
                  graw + h * DH, L.dt_bias + h * DH,
                  -std::exp(L.log_decay[h]), beta, o192 + h * DH,
                  T ? &rec : nullptr);
  }
  for (int h = 0; h < NH; h++)
    gated_rms_norm64(o192 + h * DH, og192 + h * DH, L.gn_w, gn + h * DH);
  L.op.apply(gn, y);

  if (T) {
    std::memcpy(T->cq, cq, sizeof(cq));
    std::memcpy(T->ck, ck, sizeof(ck));
    std::memcpy(T->cv, cv, sizeof(cv));
    std::memcpy(T->fu_f, fu_f, sizeof(fu_f));
    std::memcpy(T->fu_o, fu_o, sizeof(fu_o));
    std::memcpy(T->graw, graw, sizeof(graw));
    std::memcpy(T->og, og192, sizeof(og192));
    std::memcpy(T->braw, braw, sizeof(braw));
    std::memcpy(T->o192, o192, sizeof(o192));
    std::memcpy(T->gn, gn, sizeof(gn));
  }
}

void ModelImpl::van_attention(int vi, const float* xin, float* y) {
  VanLayer& L = van[vi];
  VanState& st = vst[vi];
  float q[D], k[D], v[D], pre[D];

  VanTape* T = training_ ? &tape->van[vi] : nullptr;
  L.qp.apply(xin, q);
  L.kp.apply(xin, k);
  L.vp.apply(xin, v);
  if (T) {
    std::memcpy(T->q_raw, q, sizeof(float) * D);
    std::memcpy(T->k_raw, k, sizeof(float) * D);
    std::memcpy(T->v_raw, v, sizeof(float) * D);
  }
  for (int h = 0; h < NH; h++) {
    const float d = rms_norm_vec(q + h * DH, q + h * DH, DH);
    if (T) T->qd[h] = d;
  }
  for (int h = 0; h < NH; h++) {
    const float d = rms_norm_vec(k + h * DH, k + h * DH, DH);
    if (T) T->kd[h] = d;
  }
  if (T) {
    std::memcpy(T->qn_pre, q, sizeof(float) * D);
    std::memcpy(T->kn_pre, k, sizeof(float) * D);
  }

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
  if (T) {
    std::memcpy(T->sin32, sp, sizeof(float) * 32);
    std::memcpy(T->cos32, cp, sizeof(float) * 32);
    std::memcpy(T->q_rope, q, sizeof(float) * D);
  }

  const int slot = int(t % WIN);
  std::memcpy(st.kring.data() + size_t(slot) * D, k, sizeof(float) * D);
  std::memcpy(st.vring.data() + size_t(slot) * D, v, sizeof(float) * D);

  const int n = int(t + 1 < WIN ? t + 1 : WIN);
  if (T) {
    T->n = n;
    T->slot = slot;
  }
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
    if (T)
      for (int j = 0; j < n; j++) T->probs[h][j] = std::exp(scores[j] - m) * inv;
  }
  L.op.apply(pre, y);
  if (T) std::memcpy(T->pre, pre, sizeof(float) * D);
}

void ModelImpl::step(uint8_t token, const float* prior, float* probs_out) {
  if (token >= V) die("token out of range");
  const float* tok = tok_table + size_t(token) * D;

  Tape* T = training_ ? tape.get() : nullptr;
  if (T) {
    T->token = token;
    std::memcpy(T->prior, prior, sizeof(float) * V);
  }
  prior_lin.apply(prior, yb);
  if (T) std::memcpy(T->pri_pre, yb, sizeof(float) * D);
  const float pd = rms_norm_vec(yb, yb, D);
  if (T) {
    T->pri_d = pd;
    std::memcpy(T->pri_post, yb, sizeof(float) * D);
  }
  for (int i = 0; i < D; i++) x[i] = tok[i] + yb[i];

  for (int l = 0; l < NL; l++) {
    if (l >= 6) {
      const float* s = skip_store[11 - l];
      const float w = skip_w[l - 6];
      for (int i = 0; i < D; i++) x[i] += w * s[i];
    }
    if (T) std::memcpy(T->lay[l].xpre, x, sizeof(float) * D);
    {
      const float a = rsc[l], bc = tec[l];
      for (int i = 0; i < D; i++) x[i] = a * x[i] + bc * tok[i];
    }

    {
      const float d = rms_norm_vec(x, xn, D);
      if (T) {
        T->lay[l].d1 = d;
        std::memcpy(T->lay[l].xn1, xn, sizeof(float) * D);
      }
    }
    if (KIMI[l])
      kimi_attention(layer2kimi[l], xn, yb);
    else
      van_attention(layer2van[l], xn, yb);
    for (int i = 0; i < D; i++) x[i] += yb[i];
    if (T) std::memcpy(T->lay[l].x_mid, x, sizeof(float) * D);

    {
      const float d = rms_norm_vec(x, xn, D);
      if (T) {
        T->lay[l].d2 = d;
        std::memcpy(T->lay[l].xn2, xn, sizeof(float) * D);
      }
    }
    mlp_up[l].apply(xn, h768);
    if (T) std::memcpy(T->lay[l].h, h768, sizeof(float) * DMLP);
    for (int j = 0; j < DMLP; j++) {
      const float h = h768[j];
      h768[j] = h > 0.0f ? h * h : 0.0f;  // relu squared
    }
    if (T) std::memcpy(T->lay[l].r, h768, sizeof(float) * DMLP);
    mlp_down[l].apply(h768, yb);
    for (int i = 0; i < D; i++) x[i] += yb[i];

    if (T) std::memcpy(T->x_out[l], x, sizeof(float) * D);
    if (l < 6) std::memcpy(skip_store[l], x, sizeof(float) * D);
  }

  {
    const float d = rms_norm_vec(x, xn, D);
    if (T) {
      T->d_fin = d;
      std::memcpy(T->xn_fin, xn, sizeof(float) * D);
    }
  }
  unembed.apply(xn, logits);
  for (int i = 0; i < V; i++) logits[i] = 15.0f * std::tanh(logits[i] / 15.0f);
  if (T) std::memcpy(T->cap, logits, sizeof(float) * V);

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

// ---------------------------------------------------------------------------
// backward
// ---------------------------------------------------------------------------

void ModelImpl::van_backward(int vi, const float* dy, float* dxn) {
  VanLayer& L = van[vi];
  VanTape& T = tape->van[vi];
  VanState& st = vst[vi];
  float dpre[D] = {};
  lin_back(L.op.w, T.pre, dy, D, D, g(L.op.w), dpre);

  float dq_rope[D] = {}, dk_cur[D] = {}, dv_cur[D] = {};
  std::vector<float> dp(T.n), ds(T.n);
  for (int h = 0; h < NH; h++) {
    const float* dph = dpre + h * DH;
    const float* pr = T.probs[h];
    float sdp = 0.0f;
    for (int j = 0; j < T.n; j++) {
      const float* vh = st.vring.data() + size_t(j) * D + h * DH;
      dp[j] = dot_f32(dph, vh, DH);
      sdp += pr[j] * dp[j];
    }
    for (int j = 0; j < T.n; j++) ds[j] = pr[j] * (dp[j] - sdp);
    // only the current position's k and v are this token's own; earlier ring
    // entries belong to earlier tokens and are truncated away
    for (int i = 0; i < DH; i++) dv_cur[h * DH + i] += pr[T.slot] * dph[i];
    for (int j = 0; j < T.n; j++) {
      const float c = 0.125f * ds[j];
      if (c == 0.0f) continue;
      const float* kh = st.kring.data() + size_t(j) * D + h * DH;
      for (int i = 0; i < DH; i++) dq_rope[h * DH + i] += c * kh[i];
    }
    const float cs = 0.125f * ds[T.slot];
    for (int i = 0; i < DH; i++)
      dk_cur[h * DH + i] += cs * T.q_rope[h * DH + i];
  }

  rope_back(dq_rope, T.sin32, T.cos32);
  rope_back(dk_cur, T.sin32, T.cos32);

  float dq_raw[D], dk_raw[D];
  for (int h = 0; h < NH; h++) {
    rms_norm_back(dq_rope + h * DH, T.qn_pre + h * DH, T.qd[h], DH,
                  dq_raw + h * DH);
    rms_norm_back(dk_cur + h * DH, T.kn_pre + h * DH, T.kd[h], DH,
                  dk_raw + h * DH);
  }

  const float* xn1 = attn_xn_;
  lin_back(L.qp.w, xn1, dq_raw, D, D, g(L.qp.w), dxn);
  lin_back(L.kp.w, xn1, dk_raw, D, D, g(L.kp.w), dxn);
  lin_back(L.vp.w, xn1, dv_cur, D, D, g(L.vp.w), dxn);
}

void ModelImpl::kimi_backward(int ki, const float* dy, float* dxn) {
  KimiLayer& L = kimi[ki];
  KimiTape& T = tape->kimi[ki];
  const float* xn1 = attn_xn_;

  float dgn[D] = {};
  lin_back(L.op.w, T.gn, dy, D, D, g(L.op.w), dgn);

  // gated rms norm: y[j] = o[j]*rstd*w[j]*sigmoid(og[j])
  float do192[D] = {}, dog[D] = {};
  float* g_gnw = g(L.gn_w);
  for (int h = 0; h < NH; h++) {
    const float* o = T.o192 + h * DH;
    const float* og = T.og + h * DH;
    const float rstd = 1.0f / std::sqrt(sum_squares(o, DH) / float(DH) + 1e-5f);
    float a[DH], acc = 0.0f;
    for (int j = 0; j < DH; j++) {
      const float sg = sigmoid1f(og[j]);
      a[j] = L.gn_w[j] * sg;
      const float d = dgn[h * DH + j];
      g_gnw[j] += d * o[j] * rstd * sg;
      dog[h * DH + j] = d * o[j] * rstd * L.gn_w[j] * sigmoid_d(sg);
      acc += d * o[j] * a[j];
    }
    const float c = rstd * rstd * rstd / float(DH) * acc;
    for (int j = 0; j < DH; j++)
      do192[h * DH + j] = rstd * a[j] * dgn[h * DH + j] - c * o[j];
  }

  float dcq[D] = {}, dck[D] = {}, dcv[D] = {}, dgraw[D] = {};
  float* g_dt = g(L.dt_bias);
  float* g_ld = g(L.log_decay);
  float dbraw[NH];

  for (int h = 0; h < NH; h++) {
    const float* Sprev = T.Sprev[h];
    const float* qn = T.qn[h];
    const float* kn = T.kn[h];
    const float* decay = T.decay[h];
    const float* u = T.u[h];
    const float* r = T.r[h];
    const float* dop = do192 + h * DH;
    const float beta = T.beta[h];
    const float a_neg = -std::exp(L.log_decay[h]);
    const float* v = T.cv + h * DH;

    // S = diag(decay) Sprev + kn u^T ;  o = 0.125 * S^T qn
    // dS is rank one (0.125 * qn (x) do), which collapses the whole head
    // backward onto P[i] = <do, Sprev_i>.
    const float qk = dot_f32(qn, kn, DH);
    const float Du = dot_f32(dop, u, DH);
    float dvr = 0.0f;
    for (int j = 0; j < DH; j++) dvr += dop[j] * (v[j] - r[j]);

    float dqn[DH], dkn[DH];
    for (int i = 0; i < DH; i++) {
      const float P = dot_f32(dop, Sprev + size_t(i) * DH, DH);
      dqn[i] = 0.125f * (decay[i] * P + kn[i] * Du);
      dkn[i] = 0.125f * qn[i] * Du - 0.125f * qk * beta * decay[i] * P;
      const float ddecay = 0.125f * P * (qn[i] - qk * beta * kn[i]);

      // decay = exp(a_neg * softplus(graw + dt_bias))
      const float z = T.graw[h * DH + i] + L.dt_bias[h * DH + i];
      const float sp = softplus20f(z);
      const float dz_decay = ddecay * decay[i];
      const float dsp = dz_decay * a_neg;
      const float dzin = dsp * (z > 20.0f ? 1.0f : sigmoid1f(z));
      dgraw[h * DH + i] += dzin;
      g_dt[h * DH + i] += dzin;
      // a_neg = -exp(log_decay[h]) so d a_neg / d log_decay = a_neg
      g_ld[h] += dz_decay * sp * a_neg;
    }

    // u = beta*(v - r);  dv and dbeta
    const float cdu = 0.125f * qk;
    for (int j = 0; j < DH; j++) dcv[h * DH + j] += cdu * beta * dop[j];
    const float dbeta = cdu * dvr;
    dbraw[h] = dbeta * sigmoid_d(beta);

    l2_norm_back(dqn, qn, T.dqn[h], DH, dcq + h * DH);
    l2_norm_back(dkn, kn, T.dkn[h], DH, dck + h * DH);
  }

  // beta projection (unquantized fp32 matvec on xn)
  for (int h = 0; h < NH; h++) {
    float* gw = g(L.beta_w) + size_t(h) * D;
    const float* w = L.beta_w + size_t(h) * D;
    const float gb = dbraw[h];
    for (int i = 0; i < D; i++) {
      gw[i] += gb * xn1[i];
      dxn[i] += w[i] * gb;
    }
  }

  // causal conv + SiLU, per channel; only the newest tap belongs to this token
  float dbq[D] = {}, dbk[D] = {}, dbv[D] = {};
  const float* dcs[3] = {dcq, dck, dcv};
  float* dbs[3] = {dbq, dbk, dbv};
  float* wts[3] = {L.wt_q, L.wt_k, L.wt_v};
  for (int c = 0; c < 3; c++) {
    float* gw = g(wts[c]);
    for (int ch = 0; ch < D; ch++) {
      const float dz = dcs[c][ch] * silu_d(T.cz[c][ch]);
      gw[ch] += dz * T.h0[c][ch];
      gw[D + ch] += dz * T.h1[c][ch];
      gw[2 * D + ch] += dz * T.h2[c][ch];
      const float* newest = c == 0 ? T.bq : (c == 1 ? T.bk : T.bv);
      gw[3 * D + ch] += dz * newest[ch];
      dbs[c][ch] = dz * wts[c][3 * D + ch];
    }
  }
  lin_back(L.qp.w, xn1, dbq, D, D, g(L.qp.w), dxn);
  lin_back(L.kp.w, xn1, dbk, D, D, g(L.kp.w), dxn);
  lin_back(L.vp.w, xn1, dbv, D, D, g(L.vp.w), dxn);

  // the two low-rank gates
  float dfu[DH];
  std::memset(dfu, 0, sizeof(dfu));
  lin_back(L.og_down.w, T.fu_o, dog, D, DH, g(L.og_down.w), dfu);
  lin_back(L.og_up.w, xn1, dfu, DH, D, g(L.og_up.w), dxn);
  std::memset(dfu, 0, sizeof(dfu));
  lin_back(L.fg_down.w, T.fu_f, dgraw, D, DH, g(L.fg_down.w), dfu);
  lin_back(L.fg_up.w, xn1, dfu, DH, D, g(L.fg_up.w), dxn);
}

void ModelImpl::backward(const float* dcap) {
  Tape& T = *tape;

  // logit softcap: cap = 15*tanh(z/15), d cap/dz = 1 - (cap/15)^2
  float dz[V];
  for (int i = 0; i < V; i++) {
    const float u = T.cap[i] * (1.0f / 15.0f);
    dz[i] = dcap[i] * (1.0f - u * u);
  }
  float dxn_fin[D] = {};
  lin_back(unembed.w, T.xn_fin, dz, V, D, g(unembed.w), dxn_fin);

  float dx[D];
  rms_norm_back(dxn_fin, T.xn_fin, T.d_fin, D, dx);

  float dtok[D] = {};
  float dskip_vec[6][D] = {};  // gradient waiting for the block that produced
                               // the skip source (layers 0..5)

  for (int l = NL - 1; l >= 0; l--) {
    LayerTape& LT = T.lay[l];
    if (l < 6)  // this block's output was stored as a skip source
      for (int i = 0; i < D; i++) dx[i] += dskip_vec[l][i];

    // mlp
    float dmlp[D];
    std::memcpy(dmlp, dx, sizeof(dx));
    float dr[DMLP] = {};
    lin_back(mlp_down[l].w, LT.r, dmlp, D, DMLP, g(mlp_down[l].w), dr);
    float dh[DMLP];
    for (int j = 0; j < DMLP; j++)
      dh[j] = dr[j] * 2.0f * (LT.h[j] > 0.0f ? LT.h[j] : 0.0f);
    float dxn2[D] = {};
    lin_back(mlp_up[l].w, LT.xn2, dh, DMLP, D, g(mlp_up[l].w), dxn2);
    float dmid[D];
    rms_norm_back(dxn2, LT.xn2, LT.d2, D, dmid);
    for (int i = 0; i < D; i++) dmid[i] += dx[i];  // residual around the mlp

    // attention
    float dxn1[D] = {};
    attn_xn_ = LT.xn1;
    if (KIMI[l])
      kimi_backward(layer2kimi[l], dmid, dxn1);
    else
      van_backward(layer2van[l], dmid, dxn1);
    float dxin[D];
    rms_norm_back(dxn1, LT.xn1, LT.d1, D, dxin);
    for (int i = 0; i < D; i++) dxin[i] += dmid[i];  // residual around attn

    // x_in = rsc*xpre + tec*tok
    float grsc = 0.0f, gtec = 0.0f;
    for (int i = 0; i < D; i++) {
      grsc += dxin[i] * LT.xpre[i];
      gtec += dxin[i] * tok_table[size_t(T.token) * D + i];
      dtok[i] += tec[l] * dxin[i];
    }
    g(&rsc[l])[0] += grsc;
    g(&tec[l])[0] += gtec;
    for (int i = 0; i < D; i++) dx[i] = rsc[l] * dxin[i];

    if (l >= 6) {  // xpre = stream + skip_w[l-6]*x_out[11-l]
      const int src = 11 - l, wi = l - 6;
      float gs = 0.0f;
      for (int i = 0; i < D; i++) {
        gs += dx[i] * T.x_out[src][i];
        dskip_vec[src][i] += skip_w[wi] * dx[i];
      }
      g(&skip_w[wi])[0] += gs;
    }
  }

  // x0 = tok + rms_norm(prior_lin(prior))
  for (int i = 0; i < D; i++) dtok[i] += dx[i];
  float* g_tok = g(tok_table) + size_t(T.token) * D;
  for (int i = 0; i < D; i++) g_tok[i] += dtok[i];

  float dpri_pre[D];
  rms_norm_back(dx, T.pri_post, T.pri_d, D, dpri_pre);
  lin_back(prior_lin.w, T.prior, dpri_pre, D, V, g(prior_lin.w), nullptr);
}

// state serialization: KDA states + conv histories, the two KV rings per
// vanilla layer, the skip store, and the position/rope counters
size_t ModelImpl::state_size() const {
  return sizeof(kst) + 3 * 2 * size_t(WIN) * D * sizeof(float) +
         sizeof(skip_store) + 2 * sizeof(int64_t);
}

void ModelImpl::state_save(void* dst) const {
  uint8_t* p = static_cast<uint8_t*>(dst);
  std::memcpy(p, kst, sizeof(kst));
  p += sizeof(kst);
  for (int i = 0; i < 3; i++) {
    const size_t n = size_t(WIN) * D * sizeof(float);
    std::memcpy(p, vst[i].kring.data(), n);
    p += n;
    std::memcpy(p, vst[i].vring.data(), n);
    p += n;
  }
  std::memcpy(p, skip_store, sizeof(skip_store));
  p += sizeof(skip_store);
  std::memcpy(p, &t, sizeof(t));
  p += sizeof(t);
  std::memcpy(p, &rope_off, sizeof(rope_off));
}

void ModelImpl::state_load(const void* src) {
  const uint8_t* p = static_cast<const uint8_t*>(src);
  std::memcpy(kst, p, sizeof(kst));
  p += sizeof(kst);
  for (int i = 0; i < 3; i++) {
    const size_t n = size_t(WIN) * D * sizeof(float);
    std::memcpy(vst[i].kring.data(), p, n);
    p += n;
    std::memcpy(vst[i].vring.data(), p, n);
    p += n;
  }
  std::memcpy(skip_store, p, sizeof(skip_store));
  p += sizeof(skip_store);
  std::memcpy(&t, p, sizeof(t));
  p += sizeof(t);
  std::memcpy(&rope_off, p, sizeof(rope_off));
}

void ModelImpl::adam_step(float lr, float wd, long tstep) {
  const float b1 = 0.9f, b2 = 0.999f, eps = 1e-8f;
  const float bc1 = 1.0f - std::pow(b1, float(tstep));
  const float bc2 = 1.0f - std::pow(b2, float(tstep));
  const float a = lr * std::sqrt(bc2) / bc1;
  float* w = arena.data();
  float* gr = grad.data();
  float* m = adam_m.data();
  float* v = adam_v.data();
  for (size_t i = 0; i < used; i++) {
    const float gi = gr[i];
    m[i] = b1 * m[i] + (1.0f - b1) * gi;
    v[i] = b2 * v[i] + (1.0f - b2) * gi * gi;
    w[i] -= a * m[i] / (std::sqrt(v[i]) + eps) + lr * wd * w[i];
    gr[i] = 0.0f;
  }
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
void Transformer32::enable_training() {
  if (impl->training_) return;
  impl->grad.assign(impl->arena.size(), 0.0f);
  impl->adam_m.assign(impl->arena.size(), 0.0f);
  impl->adam_v.assign(impl->arena.size(), 0.0f);
  impl->tape.reset(new Tape());
  impl->training_ = true;
}
bool Transformer32::training() const { return impl->training_; }
void Transformer32::backward(const float* dcap205) { impl->backward(dcap205); }
void Transformer32::adam_step(float lr, float wd, long t) {
  impl->adam_step(lr, wd, t);
}
void Transformer32::zero_grads() {
  std::fill(impl->grad.begin(), impl->grad.end(), 0.0f);
}
const float* Transformer32::grads() const { return impl->grad.data(); }
float* Transformer32::grads() { return impl->grad.data(); }

size_t Transformer32::state_bytes() const { return impl->state_size(); }
void Transformer32::save_state(void* dst) const { impl->state_save(dst); }
void Transformer32::load_state(const void* src) { impl->state_load(src); }

float* Transformer32::weights() { return impl->arena.data(); }
const float* Transformer32::weights() const { return impl->arena.data(); }
size_t Transformer32::weight_count() const { return impl->used; }

}  // namespace f32
}  // namespace fx2
