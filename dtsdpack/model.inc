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

  int n_frames;  // (the frames so far)
  void Init(void) {
    n_frames = 0;
    B::ffe = nullptr;
    B::ffr = nullptr;
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
    if (B::ffe) free(B::ffe), B::ffe = nullptr;
    if (B::ffr) free(B::ffr), B::ffr = nullptr;
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
    tc_touch = n_frames < 64;  // (the pages the tables take are the most of them first touched by then)
    n_frames++;
    Walk<Model> wk(*this);
    rec = &r;
    pos = 0;
    err = nullptr;
    last_pos = 0;
    last_cat = CAT_HDR;  // (the sync word)
    B::tframe = B::tnext;
    // (the stream's first frame: whether FFmpeg's encoder's rule made its code books, a flag (1/64):
    // where so it goes by FFmpeg's encoder too, since format 138; the frames after by the frames
    // before)
    if (n_frames == 1) {
      B::cat(CAT_HDR);
      B::sel_ff = c->code_bit(CO_ONE - CO_ONE / 64, ENC && ff_first(r));
    }
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
  // a twin's side field as its earlier channel's (B::twin_flag()), where that has one in lo..hi and
  // both are bound in the pass: whether it's that (v then that)
  // (since format 158 too a channel the WAV doesn't bind, against the one before it in the pass
  // where that isn't bound either, where 95% of their side fields have been the same (stw_on):
  // without their WAVs `open bitrate.dts` 2.9% smaller, FATE's 7-frame samples 3.2%, the rest the
  // same; where 75% have, the rest 0.04% bigger)
  bool twin_field(int fid, int ch, int b, S64& v, S64 lo, S64 hi) {
    int tw = B::twin_of(ch);
    const CoreRec& r = *rec;
    if (!bound[ch]) {
      if (!B::stw_on[ch] || ch - 1 < r.ch0() || bound[ch - 1]) return false;
      tw = ch - 1;
    } else if (tw < 0 || !bound[tw])
      return false;
    const CoreSub& s = r.sub[cur_sf];
    S64 tv;
    int k;
    switch (fid) {
      case F_PMODE:
        if (b >= r.nsubs[tw]) return false;
        tv = s.pmode[tw][b], k = 0;
        break;
      case F_PVQ:
        if (b >= r.nsubs[tw] || !s.pmode[tw][b]) return false;
        tv = s.pvq[tw][b], k = 1;
        break;
      case F_ABITS:
        if (b >= r.nvqsub[tw]) return false;
        tv = s.abits[tw][b], k = 2;
        break;
      case F_TMODE:
        if (b >= r.nvqsub[tw] || !s.abits[tw][b]) return false;
        tv = s.tmode[tw][b], k = 3;
        break;
      case F_SCALES: {
        const int nk = b < r.nvqsub[tw] ? (s.abits[tw][b] ? 1 + (s.tmode[tw][b] > 0) : 0) : b < r.nsubs[tw] ? 1 : 0;
        if (cur_j >= nk) return false;
        tv = s.scale[tw][b][cur_j], k = 4;
        break;
      }
      case F_HFVQ:
        if (b < r.nvqsub[tw] || b >= r.nsubs[tw]) return false;
        tv = s.hfvq[tw][b], k = 5;
        break;
      default:
        return false;
    }
    if (tv < lo || tv > hi) return false;
    return B::twin_flag(k, v, tv);
  }
  // (since format 165 an unbound channel's first scale factor, coded with its ABITS, as the channel
  // before's, where that one's was coded so too: twin_field()'s)
  bool twin_s0(int ch, int b, S64& v) {
    const CoreRec& r = *rec;
    if (!B::stw_on[ch] || ch - 1 < r.ch0() || bound[ch - 1]) return false;
    const int tw = ch - 1;
    if (b >= r.nvqsub[tw] || !B::nw_e0[tw][b]) return false;
    return B::twin_flag(4, v, B::nw_e0[tw][b] - 1);
  }
  bool code(int fid, S64& v, S64 lo, S64 hi, int nbits) {
    const CoreRec& r = *rec;
    const CoreSub& s = r.sub[cur_sf];
    const int ch = cur_ch, b = cur_b;
#ifdef DTSD_FSTAT
    c->fid = fid;
#endif
    if (fid == F_LFEscaleIndex) {  // (coded with the first LFE sample: since format 159 everywhere)
      if (B::lfn_scl >= 0) {
        account(CAT_HDR);
        v = B::lfn_scl;
        B::lfn_scl = -1;
      } else if (B::lfe_on || B::ffl_use) {
        account(CAT_SCALE);
        v = B::lfe_scl;
      } else {
        account(CAT_HDR);
        v = hi > lo ? checked(hdr_field(fid, hi, v), lo, hi) : lo;
      }
      B::ffl_tally(s, (int)v);
      return good();
    }
    if (fid == F_XFSIZE) {  // (less the one its sync word's distance to the frame's end makes, mod 2^nbits)
      account(CAT_HDR);
      const S64 e = (S64)r.fsize - (S64)((pos - 32) >> 3), m = ((S64)1 << nbits) - 1;
      v = (hdr_field(fid, m, (v - e) & m) + e) & m;
      return good();
    }
    if (fid == F_XGAP) {  // (where the subframes end at a 4-byte boundary, the last one's there, at
                          // first a whole word: an encoder's fill before the extension may be 1 to 32
                          // bits, `ES 6.1 16bit.dts`'s in 99 frames of 99; else none)
      account(CAT_HDR);
      const int at = !(pos & 31);
      v = hi > lo ? checked(B::hdr_pred(fid, hi, v, at ? (hist.xgap0 ? hist.xgap0 - 1 : 1) : 0), lo, hi) : lo;
      if (at) hist.xgap0 = (S32)imin((int)v, 1 << 20) + 1;
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
            k.G = B::abits_search(r, ch, B::X[ch][b], n, k.g, k.a0, k.a1, k.p, &k.gc, &k.gd, ~0ull, nullptr, 1, 26);
            k.G1 = B::ab_g1, k.Gc = B::ab_gc;
            const U64 dd = B::ab_tdist < k.gd ? B::ab_tdist : k.gd;
            fit = !k.G ? 3 : dd < ((U64)n << 10) ? 0 : dd < ((U64)n << 14) ? 1 : 2;
          }
          // (where X96 codes the subband too and the core has little of the decode's values: whether
          // the core's own lie on a lattice exactly as they are, jx_core_plain(): `96-24.dts`'s core
          // 0.3% smaller)
          const int f0 = fit >= 0 && B::jx_low(b) ? B::jx_core_plain(r, ch, b) : -1;
          if (f0 >= 0) fit = f0 ? 0 : 2;
        }
        // (and where the core has little of the decode's values, the gains of the fit's (je_gains()):
        // `96-24.dts`'s core 0.5% smaller)
        int g4 = B::lpg4[ch][b], pg = -1;
        // (and how the energy rises against where the encoders take a transient, as they make no
        // prediction there, pm_rise(): `s.2.dts`'s PMODEs 3.2 KB to 1.3, its bottom subband's 1173
        // bytes to 81, `ES 6.1 16bit.dts`'s core 3.0% smaller; at the subband's threshold, not one
        // the ABITS move, 1219 bytes to 1042, its TMODEs 931 to 342 and its bit allocations 9867 to
        // 9042 (the transients' mean squares, ab_transient()), its core 1.6% smaller)
        const int tr = bound[ch] ? B::pm_rise(s, ch, b) : 0;
        if (bound[ch] && B::jx_low(b) && B::ss_rule() && B::je_on(ch, b)) B::je_gains(ch, b, &g4, &pg);
        if (twin_field(fid, ch, b, v, lo, hi)) {
        } else if (fit >= 0)
          v = side_code(SF_PMSR, ch, b, v, 0, 1, 1, hist.pmode[ch][b], b ? s.pmode[ch][b - 1] + 1 : 0, fit,
                        imin(g4, 27), pg >= 0 ? pg : B::pmode_gain(ch, b), tr);
        else {  // (a quiet subband, pm_quiet()'s, predicts as one that wouldn't gain; as does an HF VQ
                // one: the encoders seen predict in none, `s.2.dts` none of its 64 that would gain, the
                // other lossless WAVs' none of their 86: its core 0.03% smaller, theirs 0.4-0.6%)
          // (since format 161 a channel the WAV doesn't bind, by its subband's last 64 decoded values
          // (nw_lpc()): their correlation with those one before, their LPC's gain in quarters of
          // halvings and that less 2, 0-8: without their WAVs the 12 streams 4536 bytes smaller, 0.03%)
          int gq = !bound[ch] ? 9 : b >= r.nvqsub[ch] || B::pm_quiet(ch, b) ? 0 : B::pmode_gain(ch, b), ua = 8, ug = 0;
          if (!bound[ch]) {
            S64 al[4];
            int g, g4;
            B::nw_lpc(ch, b, al, &g, &g4, &ua);
            ug = imin(g4, 27), gq = clip(g4 - 2, 0, 8);
          }
          v = side_code(SF_PMODE, ch, b, v, 0, 1, 1, hist.pmode[ch][b], b ? s.pmode[ch][b - 1] + 1 : 0,
                        bound[ch] ? ac[ch][b] : ua, bound[ch] ? imin(B::lpg4[ch][b], 27) : ug, gq, tr, !bound[ch]);
        }
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
        int pa = -1, pe = -1, ge = 0;
        S64 ale[4];
        if (lw) {
          S64* const d = B::pvqd;
          pvq_dists(B::lpc[ch][b], d);
          pa = B::pvq_first(B::lpc[ch][b], d);
          if (B::ss_rule() && B::je_on(ch, b)) {
            S64 y[NSMP];
            int g4;
            B::je_lpc(ch, b, ale, &ge, &g4, y);
            pvq_dists(ale, d);
            pe = B::pvq_first(ale, d);
          }
        }
        // (else by the LPC of the core's values as the decode's fit has them, je_lpc(), where its
        // first has been right more often than the decode's: `96-24.dts`'s subband 30's predictors
        // 6.5 bits each from 11.1, its core 1.4% smaller; as a try none fitted)
        if (twin_field(fid, ch, b, v, lo, hi)) {
        } else if (pe >= 0 && B::jx_more(B::je_pr[b], 0, 0)) {
          v = B::code_pvq2(ale, B::pvqd, -1, (int)v, ch, b, ge, B::lrank[ch][b][VK_PVQ], B::lrank[ch][b][VK_PVE]);
          B::pv_nofit();
        } else if (bound[ch] && !(lw && B::jx_more(B::jx_pr[b], 0, 0))) {
          v = B::code_pvq(r, s, ch, b, v);
        } else if (!bound[ch]) {  // (since format 161 by the LPC of its last 64 decoded values: code_pvq_nw())
          S64 al[4];
          int g, g4;
          B::nw_lpc(ch, b, al, &g, &g4);
          v = B::code_pvq_nw(al, hist.pvq[ch][b] - 1, (int)v, ch, b, g, B::lrank[ch][b][VK_PVQ], B::lrank[ch][b][VK_PVE]);
        } else {
          if (bound[ch] && B::ss_rule()) B::pv_nofit();
          v = side_index(SF_PVQ, ch, b, v, 12, hist.pvq[ch][b], 8, 0);
        }
        if (lw) B::jx_tally(B::jx_pr[b], v, pa, hist.pvq[ch][b] - 1);
        if (pe >= 0) B::jx_tally(B::je_pr[b], v, pa, pe);
        break;
      }
      case F_ABITS: {  // (against the audio's energy, as the scale factor its RMS makes, and how it changed)
        account(CAT_ABITS);
        int e = bound[ch] ? es[ch][b] : 0;
        // (since format 165 a channel the WAV doesn't bind: whether the subband's ABITS is 0, by the
        // last one (nw_zflag()); where not, its first scale factor, coded here (code_scale(), early),
        // the energy its ABITS then go by, as a bound channel's go by its WAV's: the ABITS seen at it,
        // in any channel, guess them 86% of the time in `s.2.dts`'s first 3 MB, the last ABITS 63%)
        bool zk = false;
        if (!bound[ch] && hi >= 1) {
          if (lo <= 0) zk = B::nw_zflag(hist.abits[ch][b] ? (hist.abits[ch][b] == 1 ? 1 : 2) : 0, ENC && v == 0);
          if (!zk) {
            if (lo < 1) lo = 1;
            account(CAT_SCALE);
            S64 s0 = ENC ? s.scale[ch][b][0] : 0;
            if (!twin_s0(ch, b, s0)) s0 = B::code_scale(r, s, ch, b, 0, s0, -1, true);
            s0 = checked(s0, 0, r.shuff[ch] == 6 ? 127 : 63);
            B::nw_e0[ch][b] = (S16)(1 + s0);
            account(CAT_ABITS);
            e = (int)(1 + s0);
          }
        }
        const int srch = bound[ch] && e && B::ab_search_on();
        S64 x[NSMP];
        int nx = 0;
        if (bound[ch] && e && (s.pmode[ch][b] || srch)) nx = B::scale_values(s, ch, b, 0, s.nssc, x);
        // (the staircase's guess at the values' fine energy, less the level: model_smp.inc; the
        // subframe's first ABITS sets the level's centre)
        const int pass = r.xpass ? 1 : 0;
        if (!B::abl[pass].set) B::level_centre(r, s, pass, pos), B::ap_start(pass);
        auto& st = B::ab_st[pass][b];
        const S32 fe = bound[ch] && e ? B::fe_s[ch][b] : B::FE_NONE;
        // (the allocation table's, where it has missed less than the staircases: model_smp.inc)
        const int sg0 = fe != B::FE_NONE ? B::level_guess(pass, st, fe) : -1;
        int tm = 0;
        const int tg = fe != B::FE_NONE ? B::ap_guess(pass, b, fe, &tm) : -1, ut = tg >= 0 && B::ap_rule(pass);
        // (and how its guess stands against the levels the subframe's ABITS so far allow: sx)
        int shr = -1;
        int sx = ut ? B::ap_strad(pass, ch, b, tg, &shr) : 0;
        const int sg = ut ? tg : sg0;
        const int sm = ut ? tm : fe != B::FE_NONE ? B::level_margin(pass, st, fe, sg0) : 0;
        int eq = bound[ch] && e ? B::eq[ch][b] : 4;
        if (bound[ch] && e && s.pmode[ch][b]) {  // (an ADPCM subband's: its prediction's residual's)
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
        if (e && !bound[ch]) {  // (an unbound channel's: the subband below's this subframe, the one above's the last)
          const int n1 = b ? B::nw_e0[ch][b - 1] : 0, n2 = b + 1 < NSUB ? hist.scale[ch][b + 1] : 0;
          if (n1 || n2) nb = clip((e - imax(n1, n2)) / 2, -4, 4) + 4;
        } else if (e) {
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
        // the first guess, its sureness and whether it is that one the context sc; PMODE's, where
        // it made the same; else the staircase's, where it has one, else the one seen; the
        // staircase's margin the context sm where the first guess is its. Since format 150, where
        // the search's lies 2 ABITS or more from the staircase's and the staircase's has been right
        // more often there, at the search's sureness, by 8 and a 16th, the staircase's is the first
        // guess, the search's the second (abq_stair(): at steps under the WAV's LSB finer lattices
        // fit as well, and the search, unsure, takes the coarsest: `padded.dts`'s, quiet at 1509
        // kbps, right 0.3-2% of the time there, the staircase's 80-93%; `dtswavsample14.wav`'s 3%
        // and 94%)
        const S32 og = sv ? sv : hist.abits[ch][b];
        int G = 0, gc = 0, sc = 0;
        U64 gd = 0;
        B::ab_g1 = B::ab_gc = 0;
        if (srch && nx) {
          const int g = og ? og - 1 : 1, a0 = og ? imax(imax((int)lo, 1), g - 4) : imax((int)lo, 1),
                    a1 = og ? imin((int)hi, g + 4) : (int)hi;
          // (PMODE's search where the same but for the ABITS it may go to, all of them: where its is in
          // this one's, the field's)
          const auto& k = B::abc[ch][b];
          const int af0 = imax((int)lo, 1), af1 = (int)hi;
          if (!s.pmode[ch][b] && k.g == g && k.a0 == a0 && k.a1 == a1 && k.p == e - 1 && (!k.G || (k.G - 1 >= af0 && k.G - 1 <= af1)))
            G = k.G, gc = k.gc, gd = k.gd, B::ab_g1 = k.G1, B::ab_gc = k.Gc;
          else if (a0 <= a1) G = B::abits_search(r, ch, x, nx, g, a0, a1, e - 1, &gc, &gd, ~0ull, nullptr, af0, af1);
        }
        B::abfit_a[ch][b] = G - 1, B::abfit_d[ch][b] = gd;
        S32 Sn = bound[ch] ? sv : 0;
        const S32 nsv = !bound[ch] && !zk ? sv : 0;  // (an unbound channel's: the ABITS seen at its energy)
        int fs = 0;  // (whether the first guess is the staircase's)
        const int qk = srch && G && sg >= 0 && (G - 1 - sg >= 2 || sg - (G - 1) >= 2) ? imin(gc, 3) : -1;
        const bool ovr = srch && G && B::ab_rule() && qk >= 0 && B::abq_stair(qk);
        if (srch && G && B::ab_rule() && !ovr) {
          Sn = G, sc = 1 + 2 * imin(gc, 6) + (G == og);
          B::n_abs++;
        } else if (sg >= 0)
          Sn = sg + 1, fs = 1;
        // (where the first guess is the staircase's, the second the ABITS across the nearer edge of
        // its energies: a miss's, nearly always; since format 150 the search's where that is the
        // staircase's in its place; and where the search's is the first guess and the staircase's
        // another, the staircase's, where it has been right more often than the last ABITS where
        // the search's missed, by 8 and an 8th (ab2), else the last ABITS as before
        // (`dtswavsample14.wav`'s staircase's as the second right 3.4 times as often;
        // `dtswavsample16.wav`'s table's, the one it takes, less often, and its core 0.3% bigger:
        // the counts lag where they turn))
        int ms = Sn == sg + 1 ? sm : 0;
        const S32 la = hist.abits[ch][b];
        const bool s2 = sc && sg >= 0 && sg != G - 1;
        int alt = ovr ? G - 1 : ms ? (ms <= 8 ? sg - 1 : sg + 1) : s2 && B::ab2[0] > B::ab2[1] + 8 + B::ab2[1] / 8 ? sg : -1;
        // (the share of the levels the subframe's ABITS allow at which the table says the first guess
        // is the first node's outside prediction (its miss's), and the share at which it says the
        // second candidate, of the rest, the second node's; each within 1/64 of 0 and 1. Since format
        // 97 where the first guess is the table's: the short streams 0.09% smaller, `s.2.dts`'s core
        // 0.01%; the first alone, 0.05% and 0.01%, within 1/32, 1/128 or 1/4096 of them the same or a
        // little bigger. Since format 98 whatever the first guess (a decoded WAV's search's, the
        // table's rule or not) and the second candidate whatever it is (the last ABITS, mostly): the
        // short streams 0.15% smaller again, `96-24.dts`'s core 0.13%, `ES 6.1 16bit.dts`'s 1.3%)
        int pq1 = -1, pq2 = -1;
        const int s1 = fe == B::FE_NONE || Sn < 1 ? -1 : fs && ut ? shr : B::ap_share(pass, ch, b, (int)Sn - 1);
        if (s1 >= 0) {
          pq1 = clip(4096 - s1, 64, 4096 - 64);
          const S32 a2 = alt >= lo && alt <= hi ? alt + 1 : hist.abits[ch][b];  // (the second candidate, 1 + it, as abits_code() takes it)
          const int sa = s1 < 4096 && a2 >= 2 && a2 != Sn ? B::ap_share(pass, ch, b, (int)a2 - 1) : -1;
          if (sa >= 0) pq2 = clip(4096 - (int)((double)sa * 4096.0 / (double)(4096 - s1) + 0.5), 64, 4096 - 64);
        }
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
        const int ag = sa >= 0 ? sa : !fs || og2 < 0 ? 0 : og2 == sg ? 1 : og2 == alt ? 2 : 3;
        // (FFmpeg's encoder's ABITS, where made again: ff_abits())
        // (since format 158 a channel the WAV doesn't bind, without a first guess, where the channel
        // before it in the pass has ABITS in the subband this subframe and the last, and it had: its
        // last moved as that moved, the first guess where it has been the value more often than the
        // last by 4 and an 8th (abx_n): without their WAVs `open bitrate.dts` 1.2% smaller, FATE's
        // 7-frame samples 1.0%; always, the rest 0.26% bigger)
        int abp = -1;
        if (!bound[ch] && !zk && !Sn && ch - 1 >= r.ch0() && b < r.nvqsub[ch - 1] && hist.abits[ch - 1][b] && hist.abits[ch][b]) {
          const int d = s.abits[ch - 1][b] - (hist.abits[ch - 1][b] - 1);
          const U32* const n = B::abx_n[ch];
          abp = (int)clip(hist.abits[ch][b] - 1 + d, lo, hi);
          if (n[1] > n[0] + 4 + n[0] / 8) Sn = abp + 1;
        }
        // (since format 161 where there is still none, and the subband has had more than 6 ABITS,
        // the first guess its ABITS' running mean (nw_am), rounded, where that isn't the last one:
        // without their WAVs the 12 streams 21189 bytes smaller, 0.16%. Against it: half its distance,
        // from the start, 14488 bigger, the 7-frame samples too; 0.75 of it after 4, 4544; all of
        // it after 4, the same but the short streams 50 bigger; 0.9 of it after 6, 1137 bigger)
        if (!bound[ch] && !Sn && hist.abits[ch][b] && B::nw_an[ch][b] > 6) {
          const int sh = (int)llround(B::nw_am[ch][b] - (double)(hist.abits[ch][b] - 1));
          if (sh) Sn = (S32)clip(hist.abits[ch][b] - 1 + sh, lo, hi) + 1;
        }
        // (since format 165 an unbound channel's ABITS seen at its energy is the first guess, where its
        // misses have weighed less than those of the guess as before (nw_gpen(): `dtswavsample16.wav`'s
        // encoder's ABITS go another way, by the counts of hits its core 2.4% bigger); the second guess
        // then the ABITS seen at the nearest energy, within 8, where it differs (89% of the first's
        // misses; the side the subframe's misses have gone, where both lie as near: 72% of a miss's
        // way), sm how near that lies and on which side (1-8 below, 9-16 above, by 1-8 steps: misses
        // 27% at 1, 3% at 2, under 1% from 3), sx which way the subframe's misses have gone (1 neither,
        // 2 up, 3 down))
        int nws = 0, og1 = -1;
        if (nsv) {
          og1 = Sn ? Sn - 1 : hist.abits[ch][b] ? hist.abits[ch][b] - 1 : -1;
          if (B::nwg_p[1] + 32 < B::nwg_p[0]) {
            Sn = nws = nsv;
            int kl = 0, kh = 0, al = -1, ah = -1;
            for (int k = 1; k <= 8 && (!kl || !kh); k++) {
              if (!kl && e - k >= 1 && sn[-k] && sn[-k] != *sn) kl = k, al = sn[-k] - 1;
              if (!kh && e + k <= 128 && sn[k] && sn[k] != *sn) kh = k, ah = sn[k] - 1;
            }
            const int dn = B::nwd_n[0] > B::nwd_n[1] ? 1 : B::nwd_n[1] > B::nwd_n[0] ? -1 : 0;
            const bool up = kh && (!kl || kh < kl || (kh == kl && dn > 0));
            if (kl || kh) {
              alt = up ? ah : al;
              ms = up ? 9 + imin(kh - 1, 7) : 1 + imin(kl - 1, 7);
            }
            sx = dn > 0 ? 2 : dn < 0 ? 3 : 1;
          }
        }
        if (zk) v = 0;
        else if (!twin_field(fid, ch, b, v, lo, hi) && !(B::ffe_ok && !pass && bound[ch] && B::ff_abits(ch, b, v, lo, hi)))
          v = abits_code(ch, b, v, lo, hi, hist.abits[ch][b], b ? s.abits[ch][b - 1] + 1 : 0, e, de, ls,
                         s.pmode[ch][b], lbd, nb, mx, Sn, near_e ? 14 : nn, sc, eq, ms,
                         alt >= lo && alt <= hi ? alt + 1 : 0, bg, ag, (fs && ut) || nws ? sx : 0, pq1, pq2, B::ss_rule() || B::ab_rule(), !bound[ch]);
        v = checked(v, lo, hi);
        if (nws) B::nwd_n[0] += v > nws - 1, B::nwd_n[1] += v < nws - 1;
        if (nsv) {
          int* const p = B::nwg_p;
          p[0] += B::nw_gpen(og1 < 0 ? 0 : og1, (int)v) - (p[0] >> 8), p[1] += B::nw_gpen((int)nsv - 1, (int)v) - (p[1] >> 8);
        }
        if (!bound[ch]) {
          B::nw_am[ch][b] = B::nw_an[ch][b] ? B::nw_am[ch][b] + ((double)v - B::nw_am[ch][b]) / 8 : (double)v;
          B::nw_an[ch][b] += B::nw_an[ch][b] < 255;
        }
        if (abp >= 0) {
          U32* const n = B::abx_n[ch];
          n[0] += v == hist.abits[ch][b] - 1, n[1] += v == abp;
          if (n[0] + n[1] >= 1u << 10) n[0] >>= 1, n[1] >>= 1;
        }
        if (!pass && bound[ch]) B::ab_budget_add(r, ch, b, (int)v);
        if (sn) *sn = (U8)(v + 1);
        if (fe != B::FE_NONE) {
          B::level_add(pass, st, fe, (int)v);
          B::ap_score(pass, sg0, tg, (int)v);
          B::ap_add(pass, b, fe, (int)v);
          B::ap_range(pass, ch, b, (int)v);
        }
        if (ut) B::n_tb++, B::n_tbhit += v == tg;
        if (fs) B::n_st++, B::n_sthit += v == sg;
        if (sc) B::n_abhit += v == G - 1;
        if (qk >= 0) B::abq_add(qk, v == sg, v == G - 1);
        if (s2 && v != G - 1 && la - 1 != sg) {
          B::ab2[0] += v == sg, B::ab2[1] += la && v == la - 1;
          if (B::ab2[0] + B::ab2[1] >= 1u << 12) B::ab2[0] >>= 1, B::ab2[1] >>= 1;
        }
        if (srch) {
          const int ro = og && v == og - 1, rs = G ? v == G - 1 : ro;
          B::abr[0] += ro, B::abr[1] += rs;
          if (B::abr[0] + B::abr[1] >= 1u << 16) B::abr[0] >>= 1, B::abr[1] >>= 1;
          B::ab_n += B::ab_n < 2048;
          if (G && B::ab_g1 && B::ab_gc && B::ab_g1 != B::ab_gc) {  // (abits_search()'s coarse lattices' rule)
            B::abk[0] += v == B::ab_g1 - 1, B::abk[1] += v == B::ab_gc - 1;
            if (B::abk[0] + B::abk[1] >= 1u << 12) B::abk[0] >>= 1, B::abk[1] >>= 1;
          }
        }
        B::n_ab++;
        break;
      }
      case F_TMODE: {  // (against the transient search's, where the scale factor search's rule is taken: its
                       // sureness the context ac is; and since format 147 against the TMODEs of the subband
                       // below in the channel and of the channel before in the subband, tr, each none, 0, the
                       // search's or another, as 6 + 4 the one below's + the other's: a transient is at the
                       // same subsubframe in the subbands and channels it is in, the search finds none where
                       // its two scale factors are the same, and the subband below's and the channel
                       // before's TMODEs are already there: the DTS CD's sample's core 4.6% smaller)
        account(CAT_SIDE);
        int gt = -1, gc = 0;
        if (twin_field(fid, ch, b, v, lo, hi)) break;
        if (bound[ch] && B::ss_rule()) gt = B::tmode_search(r, s, ch, b, &gc);
        if (gt >= 0) {
          auto rel = [&](int c, int k) {  // (0 none coded, 1 none there, 2 the search's, 3 another)
            return c < r.ch0() || k < 0 || k >= r.nvqsub[c] || !s.abits[c][k] ? 0 : !s.tmode[c][k] ? 1 : s.tmode[c][k] == gt ? 2 : 3;
          };
          v = side_code(SF_TMSR, ch, b, v, lo, hi, 0, 1 + gt, hist.tmode[ch][b], gc, 1 + imin(s.abits[ch][b], 26),
                        ec[ch][b], 6 + 4 * rel(ch, b - 1) + rel(ch - 1, b));
        }
        else if (bound[ch]) {  // (a lossless WAV's: against the rule, its class the context ac; and how the
                               // energy rises against the subband's transient threshold, tr: `s.2.dts`'s
                               // TMODEs 1.4 KB to 1.0, to 0.3 at the subband's threshold)
          int tg;
          int tc = B::tmode_rule(s, ch, b, &tg);
          if (tc >= 3) {  // (and whether the encoders take one there, ab_transient()'s: 3 not, 6 or 8 they
                          // do, by the rise; `s.2.dts`'s core 0.07% smaller than by which mean square its
                          // ABITS went by, the other lossless WAVs' 0.2-0.3%)
            const bool tk = B::fe_a[ch][b] != B::FE_NONE;
            tc = !tk ? 3 : tc >= 5 ? 8 : 6;
            if (!tk) tg = 0;
          }
          v = side_code(SF_TMODE, ch, b, v, lo, hi, 0, 1 + (tc >= 3 ? tg : 0), hist.tmode[ch][b], tc,
                        1 + imin(s.abits[ch][b], 26), ec[ch][b], s.pmode[ch][b] ? 0 : B::pm_rise(s, ch, b));
        } else
          v = side_code(SF_TMODE, ch, b, v, lo, hi, 0, hist.tmode[ch][b], 0, 8, 1 + imin(s.abits[ch][b], 26), 9, 0, true);
        break;
      }
      case F_SCALES: {  // (where X96 codes the subband too and the core has little of the decode's
                        // values, a decoded WAV's, the scale factor of the core's own where they say it:
                        // `96-24.dts`'s subband 31's 3355 times, all right, its core 0.2% smaller)
        account(CAT_SCALE);
        if (!bound[ch] && !cur_j && b < r.nvqsub[ch] && B::nw_e0[ch][b]) {  // (coded with the ABITS: since format 165)
          v = B::nw_e0[ch][b] - 1;
          break;
        }
        const int pex = bound[ch] && B::jx_low(b) && B::ss_rule() && s.abits[ch][b] ? B::jx_core_scale(r, s, ch, b) : -1;
        if (!twin_field(fid, ch, b, v, lo, hi)) v = code_scale(r, s, ch, b, cur_j, v, pex);
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
        if (twin_field(fid, ch, b, v, lo, hi)) {
        } else if (bound[ch]) v = B::code_hfvq(r, s, ch, b, v);
        else v = side_index(SF_HFVQ, ch, b, v, 10, hist.hfvq[ch][b], 8, 0, true);
        break;
      case F_LFE: {  // (against the audio's, or signed: coded 128 up, against the sample before)
        if (!cur_j) B::ffl_use = B::ffl_on();
        if (B::ffl_use) {  // (FFmpeg's encoder's made again)
          if (!cur_j) {
            cat(CAT_SCALE);
            B::code_lfe_scale_ff(ENC ? s.lfescale : 0);
          }
          account(CAT_SIDE);
          v = B::code_lfe_ff(cur_j, v);
          hist.lfe = (S32)v + 129;
          break;
        }
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
        // (else since format 159 its scale factor first, the values against the fit of the decoded
        // ones before them (lfn_pred()), the sample before the second guess: without their WAVs
        // the 12 streams 15825 bytes smaller (0.12%; their LFE values took 40115); the decoded
        // value before as the first guess, 205 bytes, twice it less the one before that, 3588; an
        // 8-tap NLMS fit at its best step (0.4), 12263; 16 taps, 385 more; forgetting by 0.98 or
        // 0.995, the same; the fit's errors' spread as ab, 41 bytes)
        if (!cur_j) {
          account(CAT_HDR);
          B::lfn_scl = (int)checked(hdr_field(F_LFEscaleIndex, 255, ENC ? s.lfescale : 0), 0, 127);
        }
        account(CAT_SIDE);
        const double st = (double)SF7[B::lfn_scl & 127];
        const S64 pv = clip((S64)llround(B::lfn_pred() / st), lo, hi);
        const S64 u = side_code(SF_LFE, 0, 0, v + 128, lo + 128, hi + 128, 0, (S32)(pv + 129),
                                cur_j ? s.lfe[cur_j - 1] + 129 : hist.lfe, 8, 0, 9, 0, true);
        v = u - 128;
        hist.lfe = (S32)u + 1;
        B::lfn_learn((double)v * st);
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
  // The core's code books after its subframes, where FFmpeg's encoder's rule has made the stream's
  // (set_best_code() of each frame's values: sel_ff, from the frames before), each against the
  // rule's (ff_books()): the subframes walked with each channel's last books (and ADJs) meanwhile,
  // whose bits the bit allocations' budget counts. FFmpeg's encoder's books are each frame's: `s.2`'s
  // first 3 MB as decoded, encoded at 1509 kbps, 15365 of its 31700 Huffman; the DTS encoders'
  // are the same for the whole stream, the codes of fixed length (the rule would have the others
  // more often than not)
  U8 selbk[NCH][21];
  bool cdfr;
  bool core_defer(CoreRec& r) {
    cdfr = B::sel_ff != 0;
    if (!cdfr) return false;
    for (int ch = 0; ch < r.npchs; ch++) {
      for (int a = 0; a < 10; a++) {
        if (ENC) selbk[ch][a] = (U8)r.sel[ch][a], selbk[ch][10 + a] = (U8)r.adj[ch][a];
        r.sel[ch][a] = B::sel_last[ch][a];
        r.adj[ch][a] = r.sel[ch][a] < QB_GROUP[a] ? B::adj_last[ch][a] : 0;
      }
      if (ENC) selbk[ch][20] = (U8)r.bhuff[ch];
      r.bhuff[ch] = B::bh_last[ch];
    }
    return true;
  }
  // the bit allocations' book FFmpeg's encoder's set_best_abits_code() takes for channel ch's
  // ABITS: of the Huffman books the first whose codes take the fewest bits, where fewer than 5 bits
  // each, else 5 bits (6); 6 where one is 0 or over 12
  static int ff_bhuff(const CoreRec& r, int ch) {
    int t[5] = {0, 0, 0, 0, 0}, n = 0;
    for (int sf = 0; sf < r.nsubfs; sf++)
      for (int b = 0; b < r.nvqsub[ch]; b++, n++) {
        const int A = r.sub[sf].abits[ch][b];
        if (A < 1 || A > 12) return 6;
        for (int k = 0; k < 5; k++) t[k] += BA[k].bits(A);
      }
    int m = 6, mb = 5 * n;
    for (int k = 0; k < 5; k++)
      if (t[k] < mb) mb = t[k], m = k;
    return m;
  }
  // the books FFmpeg's encoder's set_best_code() takes for the frame's values (rule[ch][a]): of an
  // ABITS's Huffman books the first whose codes take the fewest bits, where those and the 2 bits of
  // its ADJ take fewer than the codes of fixed length, else those (none of the ABITS: those)
  static void ff_books(const CoreRec& r, int (*rule)[10]) {
    for (int ch = 0; ch < r.npchs; ch++) {
      U32 vc[10][7], cc[10];
      memset(vc, 0, sizeof vc), memset(cc, 0, sizeof cc);
      for (int sf = 0; sf < r.nsubfs; sf++) {
        const CoreSub& s = r.sub[sf];
        const int n = s.nssc * 8;
        for (int b = 0; b < r.nvqsub[ch]; b++) {
          const int A = s.abits[ch][b];
          if (A < 1 || A > 10) continue;
          const S32* q = s.q[ch][b];
          for (int t = 0; t < QB_GROUP[A - 1]; t++) {
            const HuffBook& hb = QB[A - 1][t];
            U32 k = 0;
            for (int j = 0; j < n; j++) k += (U32)hb.bits(q[j]);
            vc[A - 1][t] += k;
          }
          cc[A - 1] += A <= 7 ? (U32)(BLOCK_BITS[A - 1] * n / 4) : (U32)((A - 3) * n);
        }
      }
      for (int a = 0; a < 10; a++) {
        int m = 0;
        for (int t = 1; t < QB_GROUP[a]; t++) m = vc[a][t] < vc[a][m] ? t : m;
        rule[ch][a] = cc[a] && vc[a][m] + 2 < cc[a] ? m : QB_GROUP[a];
      }
    }
  }
  // (whether frame r's books are the rule's, some Huffman: where the first frame's, from it on)
  static bool ff_first(const CoreRec& r) {
    int rule[NCH][10];
    ff_books(r, rule);
    bool ok = true, vlc = false;
    for (int ch = 0; ch < r.npchs; ch++) {
      for (int a = 0; a < 10; a++) ok &= rule[ch][a] == r.sel[ch][a], vlc |= r.sel[ch][a] < QB_GROUP[a];
      ok &= ff_bhuff(r, ch) == r.bhuff[ch];
    }
    return ok && vlc;
  }
  bool core_books(CoreRec& r) {
    int rule[NCH][10];
    bool vlc = false;
    if (!cdfr)
      for (int ch = 0; ch < r.npchs; ch++)
        for (int a = 0; a < 10; a++) vlc |= r.sel[ch][a] < QB_GROUP[a];
    // (where no frame has had the rule's and this one has no Huffman books, the rule no matter)
    const bool rl = cdfr || B::sel_ff || vlc;
    if (rl) ff_books(r, rule);
    if (cdfr) {
      cdfr = false;
      B::cat(CAT_HDR);
#ifdef DTSD_FSTAT
      c->fid = F_SEL;
#endif
      for (int a = 0; a < 10; a++)
        for (int ch = 0; ch < r.npchs; ch++) {
          S64 v = ENC ? selbk[ch][a] : 0;
          v = B::hdr_pred(F_SEL, (1 << SEL_BITS[a]) - 1, v, rule[ch][a]);
          r.sel[ch][a] = (int)v;
        }
#ifdef DTSD_FSTAT
      c->fid = F_ADJ;
#endif
      for (int a = 0; a < 10; a++)
        for (int ch = 0; ch < r.npchs; ch++) {
          S64 v = 0;
          if (r.sel[ch][a] < QB_GROUP[a]) v = B::hdr_field(F_ADJ, 3, ENC ? selbk[ch][10 + a] : 0);
          r.adj[ch][a] = (int)v;
        }
#ifdef DTSD_FSTAT
      c->fid = F_BHUFF;
#endif
      for (int ch = 0; ch < r.npchs; ch++) {
        const S64 v = B::hdr_pred(F_BHUFF, 7, ENC ? selbk[ch][20] : 0, ff_bhuff(r, ch));
        if (v == 7) return c->err = 1, false;
        r.bhuff[ch] = (int)v;
      }
      vlc = false;
      for (int ch = 0; ch < r.npchs; ch++)
        for (int a = 0; a < 10; a++) vlc |= r.sel[ch][a] < QB_GROUP[a];
    }
    // (the rule's where the frame's books are all its, and some are Huffman; not where one isn't)
    if (rl) {
      bool ok = true;
      for (int ch = 0; ch < r.npchs; ch++) {
        for (int a = 0; a < 10; a++) ok &= rule[ch][a] == r.sel[ch][a];
        ok &= ff_bhuff(r, ch) == r.bhuff[ch];
      }
      B::sel_ff = !ok ? 0 : vlc ? 1 : B::sel_ff;
    }
    for (int ch = 0; ch < r.npchs; ch++) {
      for (int a = 0; a < 10; a++) B::sel_last[ch][a] = (U8)r.sel[ch][a], B::adj_last[ch][a] = (U8)r.adj[ch][a];
      B::bh_last[ch] = (U8)r.bhuff[ch];
    }
    return good();
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
    B::je_learn(r);
    B::smp_subframe_end(r);
    for (int ch = r.ch0() + 1; ch < r.ch1(); ch++) {  // (two unbound channels' side fields' agreement)
      if (bound[ch] || bound[ch - 1]) continue;
      const int e = ch - 1;
      U32 eq = 0, n = 0;
      for (int b = 0; b < NSUB; b++) {
        if (b >= r.nsubs[ch] || b >= r.nsubs[e]) continue;
        n++, eq += s.pmode[ch][b] == s.pmode[e][b];
        if (s.pmode[ch][b] && s.pmode[e][b]) n++, eq += s.pvq[ch][b] == s.pvq[e][b];
        if (b < r.nvqsub[ch] && b < r.nvqsub[e]) {
          n++, eq += s.abits[ch][b] == s.abits[e][b];
          if (s.abits[ch][b] && s.abits[e][b])
            n += 2, eq += (s.tmode[ch][b] == s.tmode[e][b]) + (s.scale[ch][b][0] == s.scale[e][b][0]);
        } else if (b >= r.nvqsub[ch] && b >= r.nvqsub[e])
          n += 2, eq += (s.scale[ch][b][0] == s.scale[e][b][0]) + (s.hfvq[ch][b] == s.hfvq[e][b]);
      }
      U32* const c = B::stw_n[ch];
      c[0] += eq, c[1] += n;
      if (c[1] >= 1u << 12) c[0] >>= 1, c[1] >>= 1;
      B::stw_on[ch] = c[1] >= 32 && 100 * (U64)c[0] >= 95 * (U64)c[1];
    }
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
        hist.energy[ch][b] = q ? B::eab[ch][b] : 0;  // (since format 165 an unbound channel's too: its first scale factor)
        if (r.joinx[ch] && b >= r.nsubs[ch] && b < r.nsubs[r.joinx[ch] - 1]) hist.jscale[ch][b] = s.jscale[ch][b] + 129;
        pscale[ch][b] = 0;
      }
  }
};
