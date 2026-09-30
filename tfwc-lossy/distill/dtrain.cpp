// dtrain: distil a requantized student toward the original model's recorded
// distributions, keeping every int4 matrix on its row grids (straight-through
// estimator on latent fp32 copies).
//
//   dtrain student.tfwc2 out.q data.dump [data.dump ...]
// env: LR (1e-5)  BATCH (32)  MAXTOK (all)  TRAIN (substring filter, "" = all Lin)
//      SLR (0 = scales fixed; else Adam rate on log row scales, kept bf16)
//      SKIP (comma list of substrings to exclude)  EVAL (dump for a held-out KL,
//      forward only, before and after)  EPOCHS (1)
#include "common_head.inc"

struct Rec {
  uint8_t newpiece, tok;
  float prior[205], pt[205];
};

static bool read_rec(FILE* f, Rec& r) {
  uint8_t h[2];
  if (fread(h, 1, 2, f) != 2) return false;
  r.newpiece = h[0];
  r.tok = h[1];
  return fread(r.prior, 4, 205, f) == 205 && fread(r.pt, 4, 205, f) == 205;
}

static double kl(const float* pt, const float* ps) {
  double k = 0;
  for (int i = 0; i < 205; i++)
    if (pt[i] > 0) k += pt[i] * std::log(double(pt[i]) / std::max(double(ps[i]), 1e-30));
  return k;
}

// forward-only KL over a dump (tokens, nats averaged)
static double eval_kl(fx2::f32::Transformer32& net, const char* path, long maxtok) {
  FILE* f = fopen(path, "rb");
  if (!f) return -1;
  Rec r;
  float ps[205];
  double tot = 0;
  long n = 0;
  while (read_rec(f, r) && (maxtok <= 0 || n < maxtok)) {
    if (r.newpiece) net.begin_article(0);
    net.step(r.tok, r.prior, ps);
    tot += kl(r.pt, ps);
    n++;
  }
  fclose(f);
  return n ? tot / n : 0;
}

int main(int argc, char** argv) {
  if (argc < 4) {
    fprintf(stderr, "usage: dtrain student.tfwc2 out.q data.dump...\n");
    return 1;
  }
  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
  const double lr = getenv("LR") ? atof(getenv("LR")) : 1e-5;
  const int batch = getenv("BATCH") ? atoi(getenv("BATCH")) : 32;
  const long maxtok = getenv("MAXTOK") ? atol(getenv("MAXTOK")) : 0;
  const int epochs = getenv("EPOCHS") ? atoi(getenv("EPOCHS")) : 1;
  const std::string filt = getenv("TRAIN") ? getenv("TRAIN") : "";
  std::vector<std::string> skip;
  if (getenv("SKIP")) {
    std::string s = getenv("SKIP");
    size_t p = 0;
    while (p <= s.size()) {
      size_t q = s.find(',', p);
      if (q == std::string::npos) q = s.size();
      if (q > p) skip.push_back(s.substr(p, q - p));
      p = q + 1;
    }
  }
  const char* evalp = getenv("EVAL");
  const double slr = getenv("SLR") ? atof(getenv("SLR")) : 0.0;

  fx2::f32::Transformer32 net(argv[1], 131072 + 8, 0);

  struct Reg {
    LinReg* r;
    std::vector<float> lat, m, v;
    std::vector<double> ls, sm, sv;  // log |scale| and its Adam moments
  };
  std::vector<Reg> regs;
  size_t nparam = 0;
  for (LinReg& r : g_linreg) {
    if (!filt.empty() && r.name.find(filt) == std::string::npos) continue;
    bool sk = false;
    for (auto& s : skip) sk |= r.name.find(s) != std::string::npos;
    if (sk) continue;
    Reg g;
    g.r = &r;
    const size_t n = size_t(r.d_out) * r.d_in;
    g.lat.assign(r.w, r.w + n);
    g.m.assign(n, 0.0f);
    g.v.assign(n, 0.0f);
    for (int o = 0; o < r.d_out; o++) g.ls.push_back(r.scale[o] != 0.0f ? std::log(std::fabs(double(r.scale[o]))) : 0.0);
    g.sm.assign(r.d_out, 0.0);
    g.sv.assign(r.d_out, 0.0);
    nparam += n;
    regs.push_back(std::move(g));
  }
  fprintf(stderr, "dtrain: %zu matrices, %zu weights trainable, lr %g, batch %d\n", regs.size(), nparam, lr, batch);

  if (evalp) fprintf(stderr, "eval KL before: %.6f nats/token\n", eval_kl(net, evalp, 32768));

  net.enable_training();
  float* arena = net.weights();
  float* grad = net.grads();
  (void)arena;
  const double b1 = 0.9, b2 = 0.999, eps = 1e-8;
  long step = 0, tok = 0;
  double klsum = 0;
  long klcnt = 0;
  float ps[205], dcap[205];
  Rec r;
  for (int ep = 0; ep < epochs; ep++) {
    for (int a = 3; a < argc; a++) {
      FILE* f = fopen(argv[a], "rb");
      if (!f) { fprintf(stderr, "cannot open %s\n", argv[a]); return 1; }
      while (read_rec(f, r) && (maxtok <= 0 || tok < maxtok)) {
        if (r.newpiece) net.begin_article(0);
        net.step(r.tok, r.prior, ps);
        for (int i = 0; i < 205; i++) dcap[i] = ps[i] - r.pt[i];
        klsum += kl(r.pt, ps);
        klcnt++;
        net.backward(dcap);
        tok++;
        if (tok % batch == 0) {
          step++;
          const double bc1 = 1 - std::pow(b1, double(step)), bc2 = 1 - std::pow(b2, double(step));
          for (Reg& g : regs) {
            LinReg& R = *g.r;
            const float* gr = grad + (R.w - net.weights());
            for (int o = 0; o < R.d_out; o++) {
              if (slr > 0 && R.scale[o] != 0.0f) {  // scale step (q held fixed)
                double gs = 0;
                for (int i = 0; i < R.d_in; i++) {
                  const size_t k = size_t(o) * R.d_in + i;
                  gs += double(gr[k]) * std::nearbyint(R.w[k] / R.scale[o]);
                }
                gs = gs / batch * R.scale[o];  // d/d log|s| (sign folded in scale)
                g.sm[o] = b1 * g.sm[o] + (1 - b1) * gs;
                g.sv[o] = b2 * g.sv[o] + (1 - b2) * gs * gs;
                g.ls[o] -= slr * (g.sm[o] / bc1) / (std::sqrt(g.sv[o] / bc2) + 1e-12);
                const float sgn = R.scale[o] < 0 ? -1.0f : 1.0f;
                R.scale[o] = fx2::bf16_to_f32(fx2::f32_to_bf16(sgn * float(std::exp(g.ls[o]))));
              }
              const float s = R.scale[o];
              const float inv = s != 0.0f ? 1.0f / s : 0.0f;
              for (int i = 0; i < R.d_in; i++) {
                const size_t k = size_t(o) * R.d_in + i;
                const double gk = gr[k] / batch;
                g.m[k] = float(b1 * g.m[k] + (1 - b1) * gk);
                g.v[k] = float(b2 * g.v[k] + (1 - b2) * gk * gk);
                g.lat[k] -= float(lr * (g.m[k] / bc1) / (std::sqrt(g.v[k] / bc2) + eps));
                float q = s != 0.0f ? std::nearbyint(g.lat[k] * inv) : 0.0f;
                q = q > 7 ? 7 : q < -7 ? -7 : q;
                R.w[k] = q * s;
              }
            }
          }
          net.zero_grads();
          if (step % 500 == 0) {
            fprintf(stderr, "step %ld tok %ld train KL %.6f\n", step, tok, klsum / klcnt);
            klsum = 0;
            klcnt = 0;
          }
        }
      }
      fclose(f);
      if (maxtok > 0 && tok >= maxtok) break;
    }
  }
  if (evalp) fprintf(stderr, "eval KL after: %.6f nats/token\n", eval_kl(net, evalp, 32768));

  // the trained matrices as int4 values
  FILE* o = fopen(argv[2], "wb");
  uint32_t n = regs.size();
  fwrite(&n, 4, 1, o);
  size_t changed = 0;
  for (Reg& g : regs) {
    LinReg& R = *g.r;
    uint32_t nl = R.name.size();
    fwrite(&nl, 4, 1, o);
    fwrite(R.name.data(), 1, nl, o);
    int32_t hd[2] = {R.d_out, R.d_in};
    fwrite(hd, 4, 2, o);
    std::vector<int8_t> q(size_t(R.d_out) * R.d_in);
    for (int oo = 0; oo < R.d_out; oo++)
      for (int i = 0; i < R.d_in; i++) {
        const size_t k = size_t(oo) * R.d_in + i;
        const float s = R.scale[oo];
        q[k] = int8_t(s != 0.0f ? std::max(-7.0f, std::min(7.0f, std::nearbyint(R.w[k] / s))) : 0.0f);
      }
    fwrite(q.data(), 1, q.size(), o);
    std::vector<uint16_t> sb(R.d_out);
    for (int oo = 0; oo < R.d_out; oo++) sb[oo] = fx2::f32_to_bf16(R.scale[oo]);
    fwrite(sb.data(), 2, sb.size(), o);
  }
  fclose(o);
  (void)changed;
  fprintf(stderr, "dtrain: %ld tokens, %ld steps\n", tok, step);
  return 0;
}
