#include "arena_build.h"

#include "weights_io.h"
#include "glue.h"

namespace fx2 {
namespace opt {

namespace {

[[noreturn]] void die_arena(const char* msg) {
  fprintf(stderr, "arena_build: %s\n", msg);
  std::exit(1);
}

constexpr size_t round64(size_t x) { return (x + 63) & ~size_t(63); }

}  // namespace

void HugeBuf::alloc(size_t n) {
  const size_t kAlign = size_t(2) << 20;  // hugepage granularity
  bytes = (n + kAlign - 1) & ~(kAlign - 1);
#if defined(__linux__)
  void* m = ::mmap(nullptr, bytes, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (m == MAP_FAILED) die_arena("mmap failed");
  ::madvise(m, bytes, MADV_HUGEPAGE);
  raw = nullptr;
  p = static_cast<uint8_t*>(m);
#else
  raw = static_cast<uint8_t*>(std::malloc(bytes + kAlign));
  if (!raw) die_arena("out of memory");
  p = reinterpret_cast<uint8_t*>(
      (reinterpret_cast<uintptr_t>(raw) + (kAlign - 1)) & ~(uintptr_t(kAlign - 1)));
#endif
  std::memset(p, 0, bytes);  // touch every page
}

HugeBuf::~HugeBuf() {
#if defined(__linux__)
  if (p && !raw) ::munmap(p, bytes);
#else
  std::free(raw);
#endif
  p = nullptr;
  raw = nullptr;
}

void OptModel::load(const char* weights_path, size_t rope_rows,
                    uint64_t init_seed) {
  g_rope_rows_limit = rope_rows;
  // no path: initialize the weights in memory (weights_init.cpp).  Otherwise
  // accept both the raw FX2TFW01 file and the losslessly compressed
  // FX2TFWC1/FX2TFWC2/FX2TFWC3 files (bit-identical tensors either way), and
  // TFWZ (weights_io_tfwz.inc).
  char magic[8] = {0};
  if (weights_path) {
    if (FILE* f = fopen(weights_path, "rb")) {
      if (fread(magic, 1, 8, f) != 8) magic[0] = 0;
      fclose(f);
    }
  }
  WeightsFile wf = !weights_path ? WeightsFile::random(init_seed)
                   : (std::memcmp(magic, "FX2TFWC", 7) == 0 ||
                      std::memcmp(magic, "TFWZ", 4) == 0)
                       ? WeightsFile::load_compressed(weights_path)
                       : WeightsFile::load(weights_path);

  {  // config sanity (same checks as the naive loader)
    const WTensor& ci = wf.get("config.ints", DT_I32, {11});
    const int32_t want[11] = {V, D, NL, DH, NH, DMLP, 1024, DH, NH, 4, 10000};
    for (int i = 0; i < 11; i++)
      if (ci.i32()[i] != want[i]) die_arena("config.ints mismatch");
    const WTensor& ck = wf.get("config.kimi", DT_I32, {NL});
    for (int i = 0; i < NL; i++)
      if ((ck.i32()[i] != 0) != KIMI_L[i]) die_arena("config.kimi mismatch");
    // cmix-lex-transformer's per-layer attention windows are not implemented
    // here: accept the tensor only when it asks for the uniform window
    if (wf.has("config.window_mults")) {
      const WTensor& wm = wf.get("config.window_mults");
      if (wm.dtype != DT_I32) die_arena("config.window_mults: bad dtype");
      for (size_t k = 0; k < wm.numel; k++)
        if (wm.i32()[k] != 1)
          die_arena("config.window_mults: per-layer attention windows are not supported");
    }
    // activation quantization mode: config.actq_dynamic_mm = 1 marks a
    // checkpoint with dynamic per-token activation scales
    dyn = wf.has("config.actq_dynamic_mm") &&
          wf.get("config.actq_dynamic_mm", DT_I32, {1}).i32()[0] != 0;
  }

  // codebook weights (qmat.h): <prefix>.weight.q holds level indices in
  // [-7,7]; <prefix>.weight.codebook the 15 signed level values in units of
  // config.codebook_grid row scales.  A matrix without a codebook tensor is
  // plain (int4, or int8 for the small 8-bit matrices).
  const bool have_grid = wf.has("config.codebook_grid");
  const float cb_grid =
      have_grid ? wf.get("config.codebook_grid", DT_F32, {1}).f32()[0] : 1.0f;
  if (have_grid && !(cb_grid > 0.0f)) die_arena("codebook grid not positive");
  if (wf.has("config.codebook_max_int") &&
      wf.get("config.codebook_max_int", DT_I32, {1}).i32()[0] > QMAT_WMAX_CB)
    die_arena("config.codebook_max_int exceeds the supported codebook range (21)");
  // cb[16] = the 15 levels + 0 (the pad entry); returns max |level|, or 0 if
  // the matrix has no codebook
  auto load_codebook = [&](const std::string& prefix, int8_t cb[16]) -> int {
    if (!wf.has(prefix + ".weight.codebook")) return 0;
    if (!have_grid) die_arena("codebook weights without config.codebook_grid");
    const WTensor& t = wf.get(prefix + ".weight.codebook", DT_I8, {15});
    int wmax = QMAT_WMAX_INT4;
    for (int k = 0; k < 15; k++) {
      cb[k] = t.i8()[k];
      const int a = cb[k] < 0 ? -cb[k] : cb[k];
      if (a > QMAT_WMAX_CB) die_arena("codebook value out of range");
      if (a > wmax) wmax = a;
    }
    cb[15] = 0;
    return wmax;
  };
  // index -> level for a whole matrix (row-major int8 of the same shape)
  std::vector<int8_t> mapped;
  auto map_indices = [&](const WTensor& wq, const int8_t cb[16]) -> const int8_t* {
    mapped.resize(wq.numel);
    for (size_t i = 0; i < wq.numel; i++) {
      const int q = wq.i8()[i];
      if (q < -7 || q > 7) die_arena("level index out of [-7,7]");
      mapped[i] = cb[q + 7];
    }
    return mapped.data();
  };
  auto absmax_i8 = [](const WTensor& t) -> int {
    int m = 0;
    for (size_t i = 0; i < t.numel; i++) {
      const int a = t.i8()[i] < 0 ? -t.i8()[i] : t.i8()[i];
      if (a > m) m = a;
    }
    return m;
  };

  pool.alloc(size_t(16) << 20);
  size_t off = 0;
  auto take = [&](size_t n) -> uint8_t* {
    off = round64(off);
    uint8_t* q = pool.p + off;
    off += n;
    if (off + QMAT_TAIL_SLACK > pool.bytes) die_arena("weight pool overflow");
    return q;
  };

  // the static activation scale of a matmul site (fp32 of the bf16 bits, > 0).
  // A dynamic checkpoint carries none, and its arenas fold only the weight
  // row scales: 1.0.
  auto act_scale = [&](const std::string& prefix) -> float {
    if (dyn) {
      if (wf.has(prefix + ".quantize_activation.scale"))
        die_arena("dynamic activation scales, but the weights file has a static one");
      return 1.0f;
    }
    const WTensor& sa =
        wf.get(prefix + ".quantize_activation.scale", DT_BF16, {1});
    const float s_act = bf16_to_f32(sa.bf16_bits()[0]);
    if (!(s_act > 0.0f)) die_arena("activation scale not positive");
    return s_act;
  };

  // dense arena site in stream order
  auto qsite = [&](const std::string& prefix, int d_out, int d_in,
                   bool biased) -> QSite {
    const WTensor& wq =
        wf.get(prefix + ".weight.q", DT_I8,
               {static_cast<uint32_t>(d_out), static_cast<uint32_t>(d_in)});
    const WTensor& ws = wf.get(prefix + ".weight.scale", DT_BF16,
                               {static_cast<uint32_t>(d_out)});
    QSite s;
    s.s_act = act_scale(prefix);
    std::vector<float> fold(d_out);
    for (int o = 0; o < d_out; o++)
      fold[o] = s.s_act * bf16_to_f32(ws.bf16_bits()[o]);
    uint8_t* dst = take(qdense_bytes(d_out, d_in));
    int8_t cb[16];
    if (const int wmax = load_codebook(prefix, cb)) {
      for (int o = 0; o < d_out; o++) fold[o] *= cb_grid;
      s.m = qdense_build(dst, map_indices(wq, cb), fold.data(), d_out, d_in,
                         biased, wmax);
    } else {
      s.m = qdense_build(dst, wq.i8(), fold.data(), d_out, d_in, biased);
    }
    return s;
  };

  // int4 sparse-column arena site (d_out must be 192): [fold | cols]
  auto qsp4 = [&](const std::string& prefix, int d_in,
                  float* s_act_out) -> QSparse4 {
    const WTensor& wq =
        wf.get(prefix + ".weight.q", DT_I8,
               {static_cast<uint32_t>(D), static_cast<uint32_t>(d_in)});
    const WTensor& ws = wf.get(prefix + ".weight.scale", DT_BF16,
                               {static_cast<uint32_t>(D)});
    const float s_act = act_scale(prefix);
    *s_act_out = s_act;
    std::vector<float> fold(D);
    for (int o = 0; o < D; o++)
      fold[o] = s_act * bf16_to_f32(ws.bf16_bits()[o]);
    float* foldp = reinterpret_cast<float*>(take(sizeof(float) * D));
    uint8_t* cols = take(qsparse4_bytes(d_in));
    int8_t cb[16];
    const bool is_cb = load_codebook(prefix, cb) != 0;
    // nibbles stay level indices (q + 7); a codebook kernel maps them
    // through m.lut
    if (is_cb)
      for (int o = 0; o < D; o++) fold[o] *= cb_grid;
    QSparse4 m4 = qsparse4_build(cols, foldp, wq.i8(), fold.data(), D, d_in);
    if (is_cb) {
      m4.cb = true;
      for (int k = 0; k < 16; k++) m4.lut[k] = m4.lut[16 + k] = cb[k];
    }
    return m4;
  };

  // 8-bit small matmul (qmat.h QW8): plain int8 values in [-127,127]
  auto qw8site = [&](const std::string& prefix, int d_out, int d_in,
                     bool biased, float* s_act_out) -> QW8 {
    const WTensor& wq =
        wf.get(prefix + ".weight.q", DT_I8,
               {static_cast<uint32_t>(d_out), static_cast<uint32_t>(d_in)});
    const WTensor& ws = wf.get(prefix + ".weight.scale", DT_BF16,
                               {static_cast<uint32_t>(d_out)});
    for (size_t i = 0; i < wq.numel; i++)
      if (wq.i8()[i] == -128) die_arena("8-bit weight out of [-127,127]");
    const float s_act = act_scale(prefix);
    if (s_act_out) *s_act_out = s_act;
    std::vector<float> fold(d_out);
    for (int o = 0; o < d_out; o++)
      fold[o] = s_act * bf16_to_f32(ws.bf16_bits()[o]);
    const int rows_padded = qmat_round_up(d_out, 8);
    int8_t* w = reinterpret_cast<int8_t*>(take(qw8_bytes(d_out, d_in)));
    int32_t* corr =
        reinterpret_cast<int32_t*>(take(sizeof(int32_t) * rows_padded));
    float* fld = reinterpret_cast<float*>(take(sizeof(float) * rows_padded));
    return qw8_build(w, corr, fld, wq.i8(), fold.data(), d_out, d_in, biased);
  };
  // a small matrix goes through the 8-bit kernel when it has no codebook and
  // its values do not fit int4
  auto is_w8 = [&](const std::string& prefix, int d_out, int d_in) -> bool {
    if (wf.has(prefix + ".weight.codebook")) return false;
    return absmax_i8(wf.get(prefix + ".weight.q", DT_I8,
                            {static_cast<uint32_t>(d_out),
                             static_cast<uint32_t>(d_in)})) > QMAT_WMAX_INT4;
  };

  auto f32block = [&](const float* src, size_t n) -> float* {
    float* p = reinterpret_cast<float*>(take(sizeof(float) * n));
    std::memcpy(p, src, sizeof(float) * n);
    return p;
  };

  // ---- stream, in per-token consumption order ----
  w8_prior = is_w8("prior_embedding", D, V);
  if (w8_prior)
    prior8 = qw8site("prior_embedding", D, V, false, &prior_s_act);
  else
    prior = qsp4("prior_embedding", V, &prior_s_act);

  {
    const WTensor& sw = wf.get("skip_connection_weights.value", DT_F32, {6});
    for (int i = 0; i < 6; i++) skip_w[i] = sw.f32()[i];
  }

  int ki = 0, vi = 0;
  for (int l = 0; l < NL; l++) {
    std::string b = "blocks." + std::to_string(l) + ".";
    rsc[l] =
        wf.get(b + "residual_stream_coefficient.value", DT_F32, {1}).f32()[0];
    tec[l] =
        wf.get(b + "token_embedding_coefficient.value", DT_F32, {1}).f32()[0];
    std::string a = b + "attention.";

    if (KIMI_L[l]) {
      KimiArenas& L = kimi[ki];
      layer2kimi[l] = ki;
      L.qp = qsite(a + "query_projection", D, D, true);
      L.kp = qsite(a + "key_projection", D, D, true);
      L.vp = qsite(a + "value_projection", D, D, true);
      L.fg_up = qsite(a + "forget_gate_projection.up", DH, D, true);
      L.fg_down = qsite(a + "forget_gate_projection.down", D, DH, true);
      L.og_up = qsite(a + "output_gate_projection.up", DH, D, true);
      L.og_down = qsite(a + "output_gate_projection.down", D, DH, true);

      {
        const WTensor& bw =
            wf.get(a + "beta_projection.weight", DT_F32, {NH, D});
        L.beta_w = f32block(bw.f32(), size_t(NH) * D);
      }
      const char* convs[3] = {"query_convolution.weight",
                              "key_convolution.weight",
                              "value_convolution.weight"};
      for (int c = 0; c < 3; c++) {
        const WTensor& cw = wf.get(a + convs[c], DT_F32, {D, 4});
        float* wt = reinterpret_cast<float*>(take(sizeof(float) * 4 * D));
        for (int j = 0; j < 4; j++)  // tap-major repack, same as naive
          for (int ch = 0; ch < D; ch++) wt[j * D + ch] = cw.f32()[ch * 4 + j];
        L.kw.conv_w[c] = wt;
      }
      {
        const WTensor& dt = wf.get(a + "dt_bias", DT_F32, {D});
        L.kw.dt_bias = f32block(dt.f32(), D);
      }
      {
        const WTensor& gw =
            wf.get(a + "output_fused_norm_gate.weight", DT_F32, {DH});
        L.kw.gn_w = f32block(gw.f32(), DH);
      }
      {
        const WTensor& al = wf.get(a + "log_baseline_decay_rate", DT_F32, {NH});
        for (int h = 0; h < NH; h++) L.kw.a_neg[h] = -std::exp(al.f32()[h]);
      }
      L.op = qsite(a + "output_projection", D, D, true);
      ki++;
    } else {
      VanArenas& L = van[vi];
      layer2van[l] = vi;
      L.qp = qsite(a + "query_projection", D, D, true);
      L.kp = qsite(a + "key_projection", D, D, true);
      L.vp = qsite(a + "value_projection", D, D, true);
      L.op = qsite(a + "output_projection", D, D, true);
      // the post-rope q/k/v quantizers keep learned static scales in both
      // kinds of checkpoint (int8 KV ring)
      const WTensor& qs = wf.get(a + "quantize_queries.scale", DT_BF16, {NH});
      const WTensor& ks = wf.get(a + "quantize_keys.scale", DT_BF16, {NH});
      const WTensor& vs = wf.get(a + "quantize_values.scale", DT_BF16, {NH});
      for (int h = 0; h < NH; h++) {
        L.sq[h] = bf16_to_f32(qs.bf16_bits()[h]);
        L.sk[h] = bf16_to_f32(ks.bf16_bits()[h]);
        L.sv[h] = bf16_to_f32(vs.bf16_bits()[h]);
        L.coef[h] = 0.125f * L.sq[h] * L.sk[h];
      }
      vi++;
    }

    mlp[l].up = qsite(b + "mlp.up", DMLP, D, true);
    mlp[l].down = qsp4(b + "mlp.down", DMLP, &mlp[l].down_s_act);
  }
  if (ki != 9 || vi != 3) die_arena("layer pattern mismatch");

  w8_unembed = is_w8("unembedding", V, D);
  if (w8_unembed) {
    unembed.s_act = act_scale("unembedding");  // rms_norm_quant192 reads it
    unembed8 = qw8site("unembedding", V, D, true, nullptr);
  } else {
    unembed = qsite("unembedding", V, D, true);
  }
  {  // fp32 copy of the same rows, for a caller running its own output layer
    const WTensor& uq = wf.get("unembedding.weight.q", DT_I8, {V, D});
    const WTensor& us = wf.get("unembedding.weight.scale", DT_BF16, {V});
    int8_t cb[16];
    const bool is_cb = load_codebook("unembedding", cb) != 0;
    const int8_t* vals = is_cb ? map_indices(uq, cb) : uq.i8();
    unembed_f32.resize(size_t(V) * D);
    for (int o = 0; o < V; o++) {
      float sc = bf16_to_f32(us.bf16_bits()[o]);
      if (is_cb) sc *= cb_grid;
      for (int i = 0; i < D; i++)
        unembed_f32[size_t(o) * D + i] = sc * float(vals[size_t(o) * D + i]);
    }
  }
  stream_bytes = round64(off);

  // ---- non-stream data: tail slack gap, then the normed embedding table ----
  off += QMAT_TAIL_SLACK;
  {
    const WTensor& eq = wf.get("embedding.weight.q", DT_I8, {V, D});
    const WTensor& es = wf.get("embedding.weight.scale", DT_BF16, {V});
    std::vector<float> rs(V);
    for (int c = 0; c < V; c++) rs[c] = bf16_to_f32(es.bf16_bits()[c]);
    float* table = reinterpret_cast<float*>(take(sizeof(float) * V * D));
    int8_t cb[16];
    if (load_codebook("embedding", cb)) {
      for (int c = 0; c < V; c++) rs[c] *= cb_grid;
      build_normed_embedding_table(map_indices(eq, cb), rs.data(), table);
    } else {
      // plain int values: int4, or int8 for an 8-bit embedding
      build_normed_embedding_table(eq.i8(), rs.data(), table);
    }
    tok_table = table;
  }

  {
    const WTensor& fi = wf.get("rope.inv_freq", DT_F32, {32});
    std::memcpy(inv_freq, fi.f32(), sizeof(inv_freq));
    const WTensor& si = wf.get("rope.sin");
    const WTensor& co = wf.get("rope.cos");
    if (si.dtype != DT_F32 || si.shape.size() != 2 || si.shape[1] != 32 ||
        co.dtype != DT_F32 || co.shape.size() != 2 || co.shape[1] != 32 ||
        si.shape[0] != co.shape[0] || si.shape[0] > uint32_t(ROPE_LEN))
      die_arena("bad rope.sin/rope.cos");
    rope_len = si.shape[0];
    rope_sin.assign(si.f32(), si.f32() + rope_len * 32);
    rope_cos.assign(co.f32(), co.f32() + rope_len * 32);
  }
}

}  // namespace opt
}  // namespace fx2
