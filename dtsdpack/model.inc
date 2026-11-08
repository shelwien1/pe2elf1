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
// the WAV's audio (model_smp.inc); and the model itself, the walk's IO.

#include "model_core.inc"
#include "model_hdr.inc"
#include "model_side.inc"
#include "model_smp.inc"

// The model of a run, at the level LV: compressing (f_DEC 0) or decompressing (1). Plain data, on
// the zeroed pages of its run's coroutine (coro.inc): Init() as the run is made, start() as it
// starts (its families' tables mapped), Quit() as it ends.
template <int f_DEC, class LV>
struct Model : ModelSmp<f_DEC, LV> {
  typedef ModelSmp<f_DEC, LV> B;
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
    switch (field_kind[fid]) {
      case FK_H:
        account(CAT_HDR);
        v = checked(hdr_field(fid, nbits, v), lo, hi);
        return good();
      case FK_Q:
        account(CAT_SMP);
        v = code_q(r, s, ch, b, cur_j, v, lo, hi);
        return good();
      default:
        break;
    }
    switch (fid) {
      case F_PMODE:  // (ec: the subband's LPC's gain)
        account(CAT_SIDE);
        v = side_code(SF_PMODE, ch, b, v, 0, 1, 1, hist.pmode[ch][b], b ? s.pmode[ch][b - 1] + 1 : 0,
                      bound[ch] ? ac[ch][b] : 8, 0, bound[ch] ? imin(B::lpg[ch][b], 8) : 9);
        break;
      case F_PVQ:
        account(CAT_SIDE);
        if (bound[ch]) v = B::code_pvq(ch, b, v);
        else v = side_index(SF_PVQ, ch, b, v, 12, hist.pvq[ch][b], 8, 0);
        break;
      case F_ABITS: {  // (against the audio's energy, as the scale factor its RMS makes, and how it changed)
        account(CAT_ABITS);
        int e = bound[ch] ? es[ch][b] : 0;
        if (e && s.pmode[ch][b]) {  // (an ADPCM subband's: its prediction's residual's)
          S64 x[NSMP];
          int near;
          const int n = B::scale_values(s, ch, b, 0, s.nssc, x);
          e = 1 + scale_pred(x, n, r.shuff[ch] == 6 ? SF7 : SF6, r.shuff[ch] == 6 ? 128 : 64, &near);
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
        v = abits_code(ch, b, v, lo, hi, hist.abits[ch][b], b ? s.abits[ch][b - 1] + 1 : 0, e, de, ls,
                       s.pmode[ch][b], lbd, nb, mx, sn ? *sn : 0);
        if (sn) *sn = (U8)(checked(v, lo, hi) + 1);
        break;
      }
      case F_TMODE:
        account(CAT_SIDE);
        v = side_code(SF_TMODE, ch, b, v, lo, hi, 0, hist.tmode[ch][b], 0, bound[ch] ? ac[ch][b] : 8,
                      1 + imin(s.abits[ch][b], 26), bound[ch] ? ec[ch][b] : 9);
        break;
      case F_SCALES:
        account(CAT_SCALE);
        v = code_scale(r, s, ch, b, cur_j, v);
        break;
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
  bool raw(int fid, int n, U8* p) {
    if (fid != F_tail) fatal("internal error: raw bits of %s", field_name[fid]);
#ifdef DTSD_FSTAT
    c->fid = fid;
#endif
    account(CAT_TAIL);
    B::code_tail(p, n);
    pos += n;
    return good();
  }
  bool fixed(U32, int n, const char*) {
    pos += n;
    return true;
  }
  // where the walk is
  void frame_start() { B::unit_reset(); }
  void subframe(const CoreRec& r, int sf) {
    cur_sf = sf;
    B::sst = 0;
    B::smp_subframe(r, sf);
  }
  void at(int ch, int b, int j) {
    cur_ch = ch;
    cur_b = b;
    cur_j = j;
  }
  // the subframe done: its decoder's samples where no index made them, its values as the last ones
  void subframe_end(const CoreRec& r, int sf) {
    const CoreSub& s = r.sub[sf];
    B::smp_fill(r, s);
    B::smp_subframe_end(r);
    for (int ch = 0; ch < r.npchs; ch++)
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
