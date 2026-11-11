// model.inc -- what codes and decodes the records of a core stream, at a level (levels.inc): each
// decision goes to one of the level's families (fam.inc), whose contexts its IDX/l<L>/*.idx builds
// from the variables the model gives it (famvars.inc). The model is an IO of the syntax's walk
// (syntax.inc's Walk) at record level: the walk hands it each element of a frame as the record has
// it, which it codes (compressing: the walk reads the record) or decodes into the record
// (decompressing: the walk fills it), so the two directions can't drift apart.
//
// Its elements, as the families go, stacked in one line each on those below: what they share
// (model_core.inc); the headers, the raw bytes and the container (model_hdr.inc); the side
// information and the bit allocations (model_side.inc); the scale factors and the samples, against
// the WAV's audio (model_smp.inc); X96's subbands (model_x96.inc); and the model itself, the walk's
// IO.

#include "model_core.inc"
#include "model_hdr.inc"
#include "model_side.inc"
#include "model_smp.inc"
#include "model_x96.inc"

// The model of a run, at the level LV: compressing (f_DEC 0) or decompressing (1). Plain data, on
// the zeroed pages of its run's coroutine (coro.inc): Init() as the run is made, start() as it
// starts (its families' tables mapped), Quit() as it ends.
template <int f_DEC, class LV>
struct Model : ModelX96<f_DEC, LV> {
  typedef ModelX96<f_DEC, LV> B;
  using B::ENC;
  using B::c;
  using B::cat;
  using B::hist;
  using B::checked;
  using B::hdr_field;
  using B::side_code;
  using B::side_index;
  using B::abits_code;
  using B::code_scale;
  using B::code_q;
  using B::es;
  using B::ac;
  using B::ec;
  using B::pscale;
  using B::bound;
  // the walk's IO: at record level, reading the record where decompressing
  enum { BITS = 0, READ = f_DEC };
  U64 pos;   // the bits the elements take (as the walk counts them, at either level)
  const char* err;
  CoreRec* rec;  // the frame being coded
  int cur_sf, cur_ch, cur_b, cur_j;  // where the walk is
  U64 last_pos;  // the frame's bits counted so far by category (-s: the coder's orig)
  int last_cat;

  void Init(void) {
#define M_INIT(F) B::f_##F.Init(), B::t_##F = nullptr;
    DTSD_FAMS(M_INIT)
#undef M_INIT
    rec = nullptr;
  }
  void Quit(void) {
#define M_QUIT(F)                                  \
  if (B::t_##F) {                                  \
    LV::F::quit(*B::t_##F);                        \
    tc_unmap(B::t_##F, sizeof(typename LV::F::T)); \
    B::t_##F = nullptr;                            \
  }                                                \
  B::f_##F.Quit();
    DTSD_FAMS(M_QUIT)
#undef M_QUIT
  }
  // the run starts: the families' tables mapped and wired to the coder co; the WAV, bound by b
  void start(Coder<f_DEC>* co, WavIn* w, const Bind& b) {
    c = co;
#define M_MAP(F)                            \
  B::t_##F = tc_table<typename LV::F::T>(); \
  LV::F::init(*B::t_##F);                   \
  B::f_##F.wire(*B::t_##F, c);
    DTSD_FAMS(M_MAP)
#undef M_MAP
    B::hdr_start();
    B::smp_start(w, b);
  }

  // A frame's record r: coded (compressing: r as parsed), or decoded into r; false where it can't
  // be (damaged data). nbytes: its size (compressing; -s's counts).
  bool frame(CoreRec& r, int nbytes) {
    Walk<Model> wk(*this);
    rec = &r;
    pos = 0;
    err = nullptr;
    last_pos = 0;
    last_cat = CAT_HDR;  // (the sync word)
    B::tframe = B::tnext;
    const bool ok = wk.core(r) && good();
    if (ok) {
      B::hdr_commit();
      B::tnext += r.nblks + 1;  // (a frame's duration: its header's blocks, as the decoder's)
      if (ENC && c->stats) c->orig[last_cat] += (double)nbytes * BYTE_BITS - (double)last_pos;
    }
    rec = nullptr;
    return ok;
  }

  // whether the walk goes on: compressing, always; decompressing, while the data isn't damaged
  INLINE bool good() const { return ENC || !c->damaged(); }
  // the bits of the walk so far to the category of the value before, and the value's category
  INLINE void account(int k) {
    cat(k);
    if (ENC && c->stats) {
      c->orig[last_cat] += (double)(pos - last_pos);
      last_pos = pos;
      last_cat = k;
    }
  }

  // --- the elements
  // an element of the walk's (v, in lo..hi; nbits its bits where it is a field of them, else 0)
  bool code(int fid, S64& v, S64 lo, S64 hi, int nbits) {
    const CoreRec& r = *rec;
    const CoreSub& s = r.sub[cur_sf];
    const int ch = cur_ch, b = cur_b;
#ifdef DTSD_FSTAT
    c->fid = fid;
#endif
    if (fid == F_LFEscaleIndex && B::lfe_on) {  // (coded with the first LFE sample)
      account(CAT_SCALE);
      v = B::lfe_scl;
      return good();
    }
    if (fid == F_XFSIZE) {  // (less the one its sync word's distance to the frame's end makes, mod 2^nbits)
      account(CAT_HDR);
      const S64 e = (S64)r.fsize - (S64)((pos - 32) >> 3), m = ((S64)1 << nbits) - 1;
      v = (hdr_field(fid, m, (v - e) & m) + e) & m;
      return good();
    }
    if (r.xpass == 2 && field_kind[fid] != FK_H) {  // (X96's subbands: model_x96.inc)
      account(CAT_X96);
      v = checked(B::x96_code(r, cur_sf, fid, ch, b, cur_j, v, lo, hi), lo, hi);
      return good();
    }
    switch (field_kind[fid]) {
      case FK_H:  // (a value of the record's only, in lo..hi: nothing where it can only be lo)
        account(CAT_HDR);
        v = hi > lo ? checked(hdr_field(fid, hi, v), lo, hi) : lo;
        return good();
      case FK_Q:
        account(CAT_SMP);
        v = code_q(r, s, ch, b, cur_j, v, lo, hi);
        return good();
      default:
        break;
    }
    switch (fid) {
      case F_PMODE: {  // (against the gain of the predictor the encoder would choose: ec, ab; where the scale
                       // factor search's rule is taken, against how the values fit the lattice the bit
                       // allocation search finds in them as they are, or a transient's two, the nearer, the
                       // context ac: to a 64th of a step squared a value 0, to 16 64ths 1, worse 2, none
                       // found 3; a transient's values that fit two lattices don't predict, as those that
                       // fit one: `ES 6.1 16bit.dts`'s core 4.5% smaller, `open bitrate.dts`'s 9.3%, FATE's
                       // `dts_es.dts`'s 2.1%)
        account(CAT_SIDE);
        B::abc[ch][b].g = -1;
        int fit = -1;
        if (bound[ch] && B::ss_rule() && es[ch][b]) {
          const int e = es[ch][b], n = s.nssc * 8;
          const S32 og = ab_guess(ch, b, 0, e);
          if (og) {
            auto& k = B::abc[ch][b];
            k.g = og - 1, k.a0 = imax(1, k.g - 4), k.a1 = imin(26, k.g + 4), k.p = e - 1, k.gc = 0, k.gd = ~0ull;
            B::ab_tdist = ~0ull;
            k.G = B::abits_search(r, ch, B::X[ch][b], n, k.g, k.a0, k.a1, k.p, &k.gc, &k.gd);
            const U64 dd = B::ab_tdist < k.gd ? B::ab_tdist : k.gd;
            fit = !k.G ? 3 : dd < ((U64)n << 10) ? 0 : dd < ((U64)n << 14) ? 1 : 2;
          }
          // (where X96 codes the subband too and the core has little of the decode's values: whether
          // the core's own lie on a lattice exactly as they are, jx_core_plain(): `96-24.dts`'s core
          // 0.3% smaller)
          const int f0 = fit >= 0 && B::jx_low(b) ? B::jx_core_plain(r, ch, b) : -1;
          if (f0 >= 0) fit = f0 ? 0 : 2;
        }
        if (fit >= 0)
          v = side_code(SF_PMSR, ch, b, v, 0, 1, 1, hist.pmode[ch][b], b ? s.pmode[ch][b - 1] + 1 : 0, fit,
                        imin(B::lpg4[ch][b], 27), B::pmode_gain(ch, b));
        else  // (a quiet subband, its RMS rule's scale factor under 100, predicts as one that wouldn't gain)
          v = side_code(SF_PMODE, ch, b, v, 0, 1, 1, hist.pmode[ch][b], b ? s.pmode[ch][b - 1] + 1 : 0,
                        bound[ch] ? ac[ch][b] : 8, bound[ch] ? imin(B::lpg4[ch][b], 27) : 0,
                        !bound[ch] ? 9 : (r.shuff[ch] == 6 ? SF7 : SF6)[es[ch][b] - 1] < 100 ? 0 : B::pmode_gain(ch, b));
        break;
      }
      case F_PVQ: {
        account(CAT_SIDE);
        // (where X96 codes the subband too and the core has little of the decode's values, against the
        // core's own where a decoded WAV's say them, the decode less X96's (jx_core_values()): by
        // their LPC, the predictor that leaves them on the core's lattice exactly first, `96-24.dts`'s
        // subband 31's 99.4% the first where X96 has its lattice there, its core 10.2% smaller; else
        // against the last one where it has been right more often than the search's first: 2.0%)
        const bool lw = bound[ch] && B::jx_low(b);
        if (lw && B::ss_rule() && B::code_pvq_core(r, ch, b, v)) {
          if (!B::jc_lat[ch][b]) B::pv_nofit();  // (X96's vectors': as where none fits, as they were)
          break;
        }
        int pa = -1;
        if (lw) {
          S64* const d = B::pvqd;
          pvq_dists(B::lpc[ch][b], d);
          pa = B::pvq_first(B::lpc[ch][b], d);
        }
        if (bound[ch] && !(lw && B::jx_more(B::jx_pr[b], 0, 0))) {
          v = B::code_pvq(r, s, ch, b, v);
        } else {
          if (bound[ch] && B::ss_rule()) B::pv_nofit();
          v = side_index(SF_PVQ, ch, b, v, 12, hist.pvq[ch][b], 8, 0);
        }
        if (lw) B::jx_tally(B::jx_pr[b], v, pa, hist.pvq[ch][b] - 1);
        break;
      }
      case F_ABITS: {  // (against the audio's energy, as the scale factor its RMS makes, and how it changed)
        account(CAT_ABITS);
        int e = bound[ch] ? es[ch][b] : 0;
        const int srch = e && B::ab_search_on();
        S64 x[NSMP];
        int nx = 0;
        if (e && (s.pmode[ch][b] || srch)) nx = B::scale_values(s, ch, b, 0, s.nssc, x);
        // (the staircase's guess at the values' fine energy, less the level: model_smp.inc; the
        // subframe's first ABITS sets the level's centre)
        const int pass = r.xpass ? 1 : 0;
        if (!B::abl[pass].set) B::level_centre(r, s, pass, pos), B::ap_start(pass);
        auto& st = B::ab_st[pass][b];
        const S32 fe = e ? B::fe_s[ch][b] : B::FE_NONE;
        // (the allocation table's, where it has missed less than the staircases: model_smp.inc)
        const int sg0 = fe != B::FE_NONE ? B::level_guess(pass, st, fe) : -1;
        int tm = 0;
        const int tg = fe != B::FE_NONE ? B::ap_guess(pass, b, fe, &tm) : -1, ut = tg >= 0 && B::ap_rule(pass);
        const int sg = ut ? tg : sg0;
        const int sm = ut ? tm : fe != B::FE_NONE ? B::level_margin(pass, st, fe, sg0) : 0;
        // (a transient's: the guesses at its values' energy and at those from it on's)
        int gw = -1, ga = -1;
        if (fe != B::FE_NONE && B::fe_a[ch][b] != B::FE_NONE) {
          int m1;
          gw = ut ? B::ap_guess(pass, b, B::fe_w[ch][b], &m1) : B::level_guess(pass, st, B::fe_w[ch][b]);
          ga = ut ? B::ap_guess(pass, b, B::fe_a[ch][b], &m1) : B::level_guess(pass, st, B::fe_a[ch][b]);
        }
        int eq = e ? B::eq[ch][b] : 4;
        if (e && s.pmode[ch][b]) {  // (an ADPCM subband's: its prediction's residual's)
          int near;
          e = 1 + scale_pred(x, nx, r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]), &near);
          eq = scale_quarter(x, nx, r.shuff[ch] == 6 ? SF7 : SF6, sf_entries(r.shuff[ch]), e - 1);
        }
        B::eab[ch][b] = e;
        const int le = hist.energy[ch][b];
        const int de = e && le ? dvar(e - le) : 9;
        const int ls = hist.scale[ch][b] && hist.pscale[ch][b] ? dvar(hist.scale[ch][b] - hist.pscale[ch][b]) : 9;
        const int lbd = b && hist.abits[ch][b - 1] ? dvar(s.abits[ch][b - 1] - (hist.abits[ch][b - 1] - 1)) : 9;
        int nb = 9, mx = 16;
        if (e) {
          const int n1 = b ? es[ch][b - 1] : 0, n2 = b + 1 < NSUB ? es[ch][b + 1] : 0;
          nb = clip((e - imax(n1, n2)) / 2, -4, 4) + 4;
          mx = imin((B::esmax[ch] - e) / 4, 15);
        }
        U8* const sn = e ? &B::ab_seen[s.pmode[ch][b]][b][imin(e, 128)] : nullptr;
        int nn = 15;  // (the ABITS seen at the energies either side, against sn: below, -1..1 or none, times 4, the same above)
        if (sn && *sn) {
          const U8 *lo_ = sn - 1, *hi_ = e < 128 ? sn + 1 : nullptr;
          const int a = e > 1 && *lo_ ? clip(*lo_ - *sn, -1, 1) + 1 : 3, z = hi_ && *hi_ ? clip(*hi_ - *sn, -1, 1) + 1 : 3;
          nn = a * 4 + z;
        }
        int sv = sn ? *sn : 0, near_e = 0;  // (none at this energy: the nearest energy's, within 2)
        for (int k = 1; sn && !sv && k <= 2; k++) {
          if (e - k >= 1 && sn[-k]) sv = sn[-k], near_e = 1;
          else if (e + k <= 128 && sn[k]) sv = sn[k], near_e = 1;
        }
        // (the search's, where it is right clearly more often than the first guess without it: it
        // the first guess, its sureness and whether it is that one the context sc; PMODE's, where it
        // made the same; else the staircase's, where it has one, else the one seen; the staircase's
        // margin the context sm where the first guess is its)
        const S32 og = sv ? sv : hist.abits[ch][b];
        int G = 0, gc = 0, sc = 0;
        U64 gd = 0;
        if (srch && nx) {
          const int g = og ? og - 1 : 1, a0 = og ? imax(imax((int)lo, 1), g - 4) : imax((int)lo, 1),
                    a1 = og ? imin((int)hi, g + 4) : (int)hi;
          const auto& k = B::abc[ch][b];
          if (!s.pmode[ch][b] && k.g == g && k.a0 == a0 && k.a1 == a1 && k.p == e - 1) G = k.G, gc = k.gc, gd = k.gd;
          else if (a0 <= a1) G = B::abits_search(r, ch, x, nx, g, a0, a1, e - 1, &gc, &gd);
        }
        B::abfit_a[ch][b] = G - 1, B::abfit_d[ch][b] = gd;
        S32 Sn = sv;
        int fs = 0;  // (whether the first guess is the staircase's)
        if (srch && G && B::ab_rule()) {
          Sn = G, sc = 1 + 2 * imin(gc, 6) + (G == og);
          B::n_abs++;
        } else if (sg >= 0)
          Sn = sg + 1, fs = 1;
        // (where the first guess is the staircase's, the second the ABITS across the nearer edge of
        // its energies: a miss's, nearly always)
        const int ms = Sn == sg + 1 ? sm : 0;
        int alt = ms ? (ms <= 8 ? sg - 1 : sg + 1) : -1;
        // (a transient's, the guess at the energy not taken is the second, where it differs)
        const int tro = fs && gw >= 0 && ga >= 0 && gw != ga ? (sg == ga ? gw : ga) : -1;
        if (tro >= 0 && tro != sg) alt = tro;
        // (the budget: what the samples may still take, toward the edge ms says, in steps of this
        // subband's bits across it; a lossless WAV's only: a decoded WAV's streams 0.1-0.9% bigger
        // with it)
        int bg = 0;
        if (!pass && ms && B::abl[0].rn && bound[ch] && !B::ss_rule()) {
          const S64 R = B::ab_budget(r), g0 = B::bud_bits(r, ch, b, sg);
          const S64 st = ms <= 8 ? (sg > 0 ? g0 - B::bud_bits(r, ch, b, sg - 1) : 0) : B::bud_bits(r, ch, b, sg + 1) - g0;
          if (st > 0) {
            const S64 t = (ms <= 8 ? -R : R) * 2;  // (2 R / st: in half steps)
            bg = t >= 4 * st ? 1 : t >= 2 * st ? 2 : t >= st ? 3 : t > -st ? 4 : t > -2 * st ? 5 : t > -4 * st ? 6 : 7;
          }
        }
        // (what the other of the staircase and the table says; where the first guess is the
        // search's, what the one taken of them says: 1 the same, 2 the last ABITS, 3 another, 0
        // none: a decoded WAV's search, right in 99.99% where they agree, misses 20% where not)
        const int og2 = ut ? sg0 : tg;
        const int sa = !fs && G && Sn == G ? (sg < 0 ? 0 : sg == G - 1 ? 1 : hist.abits[ch][b] && sg == hist.abits[ch][b] - 1 ? 2 : 3) : -1;
        const int ag = sa >= 0 ? sa : tro >= 0 && tro != sg ? 4 : !fs || og2 < 0 ? 0 : og2 == sg ? 1 : og2 == alt ? 2 : 3;
        v = abits_code(ch, b, v, lo, hi, hist.abits[ch][b], b ? s.abits[ch][b - 1] + 1 : 0, e, de, ls,
                       s.pmode[ch][b], lbd, nb, mx, Sn, near_e ? 14 : nn, sc, eq, ms,
                       alt >= lo && alt <= hi ? alt + 1 : 0, bg, ag);
        v = checked(v, lo, hi);
        if (!pass && bound[ch]) B::ab_budget_add(r, ch, b, (int)v);
        if (sn) *sn = (U8)(v + 1);
        if (fe != B::FE_NONE) {
          B::level_add(pass, st, fe, (int)v);
          B::ap_score(pass, sg0, tg, (int)v);
          B::ap_add(pass, b, fe, (int)v);
        }
        if (ut) B::n_tb++, B::n_tbhit += v == tg;
        if (gw >= 0 && ga >= 0) B::ab_ev[ch][b] = (U8)(gw == ga ? 3 : v == ga ? 2 : v == gw ? 1 : 0);
        if (fs) B::n_st++, B::n_sthit += v == sg;
        if (sc) B::n_abhit += v == G - 1;
        if (srch) {
          const int ro = og && v == og - 1, rs = G ? v == G - 1 : ro;
          B::abr[0] += ro, B::abr[1] += rs;
          if (B::abr[0] + B::abr[1] >= 1u << 16) B::abr[0] >>= 1, B::abr[1] >>= 1;
          B::ab_n += B::ab_n < 2048;
        }
        B::n_ab++;
        break;
      }
      case F_TMODE: {  // (against the transient search's, where the scale factor search's rule is taken: its
                       // sureness the context ac is)
        account(CAT_SIDE);
        int gt = -1, gc = 0;
        if (bound[ch] && B::ss_rule()) gt = B::tmode_search(r, s, ch, b, &gc);
        if (gt >= 0)
          v = side_code(SF_TMSR, ch, b, v, lo, hi, 0, 1 + gt, hist.tmode[ch][b], gc, 1 + imin(s.abits[ch][b], 26),
                        ec[ch][b]);
        else if (bound[ch]) {  // (a lossless WAV's: against the rule, its class the context ac)
          int tg;
          int tc = B::tmode_rule(s, ch, b, &tg);
          if (tc >= 3) {  // (and what its ABITS said: 3 the values' energy, 6 or 8 that from the transient on)
            const int ev = B::ab_ev[ch][b], r4 = tc >= 5;
            tc = ev == 1 ? 3 : ev == 2 ? (r4 ? 8 : 6) : r4 ? 7 : 4 + (s.abits[ch][b] >= 10);
            if (ev == 1) tg = 0;
          }
          v = side_code(SF_TMODE, ch, b, v, lo, hi, 0, 1 + (tc >= 3 ? tg : 0), hist.tmode[ch][b], tc,
                        1 + imin(s.abits[ch][b], 26), ec[ch][b]);
        } else
          v = side_code(SF_TMODE, ch, b, v, lo, hi, 0, hist.tmode[ch][b], 0, 8, 1 + imin(s.abits[ch][b], 26), 9);
        break;
      }
      case F_SCALES: {  // (where X96 codes the subband too and the core has little of the decode's
                        // values, a decoded WAV's, the scale factor of the core's own where they say it:
                        // `96-24.dts`'s subband 31's 3355 times, all right, its core 0.2% smaller)
        account(CAT_SCALE);
        const int pex = bound[ch] && B::jx_low(b) && B::ss_rule() && s.abits[ch][b] ? B::jx_core_scale(r, s, ch, b) : -1;
        v = code_scale(r, s, ch, b, cur_j, v, pex);
        break;
      }
      case F_JOIN_SHUFF:
        account(CAT_SIDE);
        v = side_code(SF_JSHUFF, ch, 0, v, lo, hi, 0, 0, 0, 8, 0, 9);
        break;
      case F_JOIN_SCALES: {  // (signed: coded 128 up)
        account(CAT_SIDE);
        const S64 u = side_code(SF_JSCALE, ch, b, v + 128, lo + 128, hi + 128, 0, hist.jscale[ch][b], 0, 8, 0, 9);
        v = u - 128;
        break;
      }
      case F_HFVQ:
        account(CAT_SIDE);
        if (bound[ch]) v = B::code_hfvq(r, s, ch, b, v);
        else v = side_index(SF_HFVQ, ch, b, v, 10, hist.hfvq[ch][b], 8, 0);
        break;
      case F_LFE: {  // (against the audio's, or signed: coded 128 up, against the sample before)
        if (B::lfe_on) {
          if (!cur_j) {
            cat(CAT_SCALE);
            B::code_lfe_scale(ENC ? s.lfescale : 0);
          }
          account(CAT_SIDE);
          v = B::code_lfe(cur_j, v);
          hist.lfe = (S32)v + 129;
          break;
        }
        account(CAT_SIDE);
        const S64 u = side_code(SF_LFE, 0, 0, v + 128, lo + 128, hi + 128, 0, hist.lfe,
                                cur_j ? s.lfe[cur_j - 1] + 129 : 0, 8, 0, 9);
        v = u - 128;
        hist.lfe = (S32)u + 1;
        break;
      }
      default:
        fatal("internal error: a field %s at record level", field_name[fid]);
    }
    v = checked(v, lo, hi);
    return good();
  }
  bool u(int fid, int n, S64& v) {
    if (!code(fid, v, 0, ((S64)1 << n) - 1, n)) return false;
    pos += n;
    return true;
  }
  bool s(int fid, int n, S64& v) {
    if (!code(fid, v, -((S64)1 << (n - 1)), ((S64)1 << (n - 1)) - 1, 0)) return false;
    pos += n;
    return true;
  }
  bool huff(int fid, const HuffBook& b, S64& v) {
    if (!code(fid, v, b.minlev, b.minlev + b.nlev - 1, 0)) return false;
    if (!b.has(v)) return c->err = 1, false;  // (a level the book has no code for: damaged data)
    pos += b.bits(v);
    return true;
  }
  bool val(int fid, S64 lo, S64 hi, S64& v) { return code(fid, v, lo, hi, 0); }
  // channel ch's subband b's ABITS first guess (1 + it, 0 none), as F_ABITS's: the ABITS seen at
  // its energy e (pm: whether it predicts), within 2, else its last
  S32 ab_guess(int ch, int b, int pm, int e) const {
    const U8* sn = &B::ab_seen[pm][b][imin(e, 128)];
    int sv = *sn;
    for (int k = 1; !sv && k <= 2; k++) {
      if (e - k >= 1 && sn[-k]) sv = sn[-k];
      else if (e + k <= 128 && sn[k]) sv = sn[k];
    }
    return sv ? sv : hist.abits[ch][b];
  }
  bool raw(int fid, int n, U8* p) {
    const int kind = fid == F_tail ? 0 : fid == F_xgap ? 1 : fid == F_xhrest ? 2 : fid == F_xcrest ? 3 : fid == F_xpad ? 4 : -1;
    if (kind < 0) fatal("internal error: raw bits of %s", field_name[fid]);
#ifdef DTSD_FSTAT
    c->fid = fid;
#endif
    account(CAT_TAIL);
    // (with auxiliary data, the bits after the subframes are its byte count, 0s to a 32-bit
    // boundary, its sync word and the data: their bytes from that boundary, where they end with
    // the frame's byte)
    int a0 = -1;
    if (!kind && rec->auxf && !((pos + n) & 7)) {
      const S64 bnd = (pos + 6 + 31) & ~(S64)31, first = pos + n - 8 * (S64)((n + 7) / 8);
      if (bnd < pos + n) a0 = (int)((bnd - first) / 8);
    }
    B::code_tail(p, n, kind, a0, (S64)pos);
    pos += n;
    return good();
  }
  bool fixed(U32, int n, const char*) {
    pos += n;
    return true;
  }
  // where the walk is
  void frame_start() { B::unit_reset(); }
  // X96's code books after its subframes (every encoder's? `96-24.dts`'s each the one that codes the
  // values in the fewest bits): the subframes coded with the first of each (its values are every
  // book's), then each book against the cheapest for the values (x96_books())
  U8 x96bk[NCH][2 + 10];
  bool x96_defer(CoreRec& r) {
    for (int ch = 0; ch < r.npchs; ch++) {
      if (ENC) {
        x96bk[ch][0] = (U8)r.x96shuff[ch], x96bk[ch][1] = (U8)r.x96bhuff[ch];
        for (int a = 0; a < 10; a++) x96bk[ch][2 + a] = (U8)r.x96sel[ch][a];
      }
      r.x96shuff[ch] = r.x96bhuff[ch] = 0;
      for (int a = 0; a < 10; a++) r.x96sel[ch][a] = 0;
    }
    return true;
  }
  // the same of the joint scale factors' books (each channel's, each subframe: `96-24.dts`'s the
  // cheapest 1875 times of 1876), after the scale factors
  bool x96_jdefer() { return true; }
  // the walk's place set (X96's subframes counted again): the statistics' too, the bits counted
  // with the walk's books taken off the category they went to
  void pos_to(U64 p) {
    if (ENC && c->stats) {
      c->orig[last_cat] += (double)(pos - last_pos) - ((double)pos - (double)p);
      last_pos = p;
    }
    pos = p;
  }
  bool x96_jsel(const CoreRec& r, X96Sub& s) {
    for (int ch = 0; ch < r.npchs; ch++) {
      if (!r.x96joinx[ch]) continue;
      const int src = r.x96joinx[ch] - 1;
      double jc[7] = {0};
      for (int b = r.x96nsubs[ch]; b < r.x96nsubs[src]; b++) {
        const int v = s.jscale[ch][b];
        for (int k = 0; k < 5; k++) jc[k] += SB[k].has(v) ? SB[k].bits(v) : 1e9;
        jc[5] += v >= 0 && v < 64 ? 6 : 1e9;
        jc[6] += v >= 0 && v < 128 ? 7 : 1e9;
      }
      int m = 0;
      for (int k = 1; k < 7; k++) m = jc[k] < jc[m] ? k : m;
      B::cat(CAT_X96);
      S64 v = ENC ? s.jsel[ch] : 0;
      v = B::side_code(SF_JSHUFF, ch, 0, v, 0, 6, 0, m + 1, 0, 8, 0, 9);
      s.jsel[ch] = (int)v;
    }
    return true;
  }
  bool x96_books(CoreRec& r) {
    const int hr = r.x96hr, b0 = r.x96start, na = 6 + 4 * hr;
    B::cat(CAT_HDR);
    for (int ch = 0; ch < r.npchs; ch++) {
      // (each book's bits for the channel's values, over the subframes; 1e9: one a value hasn't)
      double sc[6] = {0}, bc[8] = {0}, qc[10][8];
      memset(qc, 0, sizeof qc);
      for (int sf = 0; sf < r.nsubfs; sf++) {
        const X96Sub& s = r.xs[sf];
        const int n = r.sub[sf].nssc * 8;
        int si = 0, ab = 0;
        for (int b = b0; b < r.x96nsubs[ch]; b++) {
          const int v = s.scale[ch][b], d = v - si, a = s.abits[ch][b], e = a - ab;
          for (int k = 0; k < 5; k++) sc[k] += SB[k].has(d) ? SB[k].bits(d) : 1e9;
          sc[5] += 6;
          for (int k = 0; k < 7; k++) bc[k] += QB[5 + 2 * hr][k].has(e) ? QB[5 + 2 * hr][k].bits(e) : 1e9;
          bc[7] += 3 + hr;
          si = v, ab = a;
          const int q = a - 1;  // (the quantizer's ABITS)
          if (q < 1 || q > na) continue;
          for (int t = 0; t < QB_GROUP[q - 1]; t++)
            for (int j = 0; j < n; j++) qc[q - 1][t] += QB[q - 1][t].has(s.q[ch][b][j]) ? QB[q - 1][t].bits(s.q[ch][b][j]) : 1e9;
          qc[q - 1][QB_GROUP[q - 1]] += q <= 7 ? (double)BLOCK_BITS[q - 1] * n / 4 : (double)(q - 3) * n;
        }
      }
      auto best = [](const double* c, int k) {
        int m = 0;
        for (int i = 1; i < k; i++) m = c[i] < c[m] ? i : m;
        return m;
      };
      S64 v = ENC ? x96bk[ch][0] : 0;
      v = B::hdr_pred(F_SHUFF, 7, v, best(sc, 6));
      if (v >= 6) return false;
      r.x96shuff[ch] = (int)v;
      v = ENC ? x96bk[ch][1] : 0;
      r.x96bhuff[ch] = (int)B::hdr_pred(F_BHUFF, 7, v, best(bc, 8));
      for (int a = 0; a < na; a++) {
        v = ENC ? x96bk[ch][2 + a] : 0;
        r.x96sel[ch][a] = (int)B::hdr_pred(F_SEL, (1 << SEL_BITS[a]) - 1, v, best(qc[a], QB_GROUP[a] + 1));
      }
    }
    return true;
  }
  void subframe(const CoreRec& r, int sf) {
    cur_sf = sf;
    B::jc_key++;
    B::sst = 0;
    if (r.xpass == 2) B::x96_subframe(r, sf);
    else B::smp_subframe(r, sf);
  }
  void at(int ch, int b, int j) {
    cur_ch = ch;
    cur_b = b;
    cur_j = j;
  }
  // the subframe done: its decoder's samples where no index made them, its values as the last ones
  void subframe_end(const CoreRec& r, int sf) {
    if (r.xpass == 2) return B::x96_subframe_end(r, sf);
    const CoreSub& s = r.sub[sf];
    B::level_commit(r, s, r.xpass ? 1 : 0);
    B::smp_fill(r, s);
    B::smp_subframe_end(r);
    for (int ch = r.ch0(); ch < r.ch1(); ch++)
      for (int b = 0; b < NSUB; b++) {
        const int in = b < r.nsubs[ch], q = b < r.nvqsub[ch];
        hist.pmode[ch][b] = in ? s.pmode[ch][b] + 1 : 0;
        if (in && s.pmode[ch][b]) hist.pvq[ch][b] = s.pvq[ch][b] + 1;
        hist.tmode[ch][b] = q && s.abits[ch][b] && s.nssc > 1 ? s.tmode[ch][b] + 1 : hist.tmode[ch][b];
        hist.abits[ch][b] = q ? s.abits[ch][b] + 1 : 0;
        hist.scale[ch][b] = in && (!q || s.abits[ch][b]) ? s.scale[ch][b][0] + 1 : 0;
        hist.pscale[ch][b] = in && (!q || s.abits[ch][b]) ? pscale[ch][b] : 0;
        if (in && !q) hist.hfvq[ch][b] = s.hfvq[ch][b] + 1;
        hist.energy[ch][b] = bound[ch] && q ? B::eab[ch][b] : 0;
        if (r.joinx[ch] && b >= r.nsubs[ch] && b < r.nsubs[r.joinx[ch] - 1]) hist.jscale[ch][b] = s.jscale[ch][b] + 129;
        pscale[ch][b] = 0;
      }
  }
};
