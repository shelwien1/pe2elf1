// model.inc -- what codes and decodes the records of a stream, at a level (levels.inc): each
// decision goes to one of the level's families (fam.inc), whose contexts its IDX/l<L>/*.idx
// builds from the variables the model gives it (famvars.inc). The model is an IO of the syntax's
// walk (syntax.inc's Walk) at record level: the walk hands it each element of a frame as the
// record has it, which it codes (compressing: the walk reads the record) or decodes into the
// record (decompressing: the walk fills it), so the two directions can't drift apart.
//
// The model is its elements, as the families go, stacked in one line each on those below (what C++
// lets a class see): what they share (model_core.inc); the header, the side information, the raw
// bytes and the container (model_hdr.inc); the exponents (model_exp.inc); the mantissas and AHT's
// data (model_mant.inc); and the model itself, the walk's IO and its callback after each block.

#include "model_core.inc"
#include "model_hdr.inc"
#include "model_exp.inc"
#include "model_mant.inc"

// The model of a run, at the level LV: compressing (f_DEC 0) or decompressing (1). Plain data, on
// the zeroed pages of its run's coroutine (coro.inc): Init() as the run is made, start() as it
// starts (its families' tables mapped), Quit() as it ends.
template <int f_DEC, class LV>
struct Model : ModelMant<f_DEC, LV> {
  typedef ModelMant<f_DEC, LV> B;
  using B::ENC;
  using B::c;
  using B::cat;
  using B::hist;
  using B::h;
  using B::sk;
  using B::strmtyp;
  using B::blk;
  using B::sstate;
  using B::hdr_field;
  using B::sid_code;
  using B::code_dexp;
  using B::code_q;
  using B::code_gaq;
  // the walk's IO: at record level, reading the record where decompressing
  enum { BITS = 0, READ = f_DEC };
  int pos, end;  // (an IO's: the bit level ones' place in the bits; a record level one has none)
  Walk<Model, Model>* w;  // the walk of the frame being coded
  Frame* fr;
  int last_pos, last_cat;  // the frame's bits counted so far by category (-s: the coder's orig)

  void Init(void) {
#define M_INIT(F) B::f_##F.Init(), B::t_##F = nullptr;
    AC3P_FAMS(M_INIT)
#undef M_INIT
    w = nullptr;
    fr = nullptr;
  }
  void Quit(void) {
#define M_QUIT(F)                                                           \
  if (B::t_##F) {                                                           \
    LV::F::quit(*B::t_##F);                                                 \
    tc_unmap(B::t_##F, sizeof(typename LV::F::T));                          \
    B::t_##F = nullptr;                                                     \
  }                                                                         \
  B::f_##F.Quit();
    AC3P_FAMS(M_QUIT)
#undef M_QUIT
  }
  // the run starts: the families' tables mapped and wired to the coder co
  void start(Coder<f_DEC>* co) {
    c = co;
#define M_MAP(F)                            \
  B::t_##F = tc_table<typename LV::F::T>(); \
  LV::F::init(*B::t_##F);                   \
  B::f_##F.wire(*B::t_##F, c);
    AC3P_FAMS(M_MAP)
#undef M_MAP
    B::hdr_start();
    B::exp_start();
  }

  // A frame's record f: coded (compressing: f as parsed), or decoded into f (zeroed); false where
  // it can't be (damaged data)
  bool frame(Frame& f) {
    Walk<Model, Model> wk(*this, f, *this);
    bool ok;
    w = &wk;
    fr = &f;
    last_pos = 0;
    last_cat = CAT_HDR;  // (the sync word)
    ok = wk.frame() && good();
    if (ok) {
      B::hdr_commit();
      if (ENC && c->stats) c->orig[last_cat] += f.fbytes * BYTE_BITS - last_pos;
    }
    w = nullptr;
    return ok;
  }

  // whether the walk goes on: compressing, always (its input's end is no matter of the model's);
  // decompressing, while the data isn't damaged (a value no encoder writes, the stream cut short)
  INLINE bool good() const { return ENC || !c->damaged(); }
  // the bits of the walk so far to the category of the value before, and the value's category
  INLINE void account(int k) {
    cat(k);
    if (ENC && c->stats) {
      c->orig[last_cat] += w->pos - last_pos;
      last_pos = w->pos;
      last_cat = k;
    }
  }

  bool u(int fid, int n, int& v) {
    const Frame& f = *fr;
    switch (fid) {
      case F_gaqgain:
        account(CAT_AHT);
        v = B::code_gaqgain(f, w->r, w->cur_ch, w->cur_bin, w->cur_bap, v);
        return good();
      case F_vq:
        account(CAT_AHT);
        v = B::code_vq(f, w->r, w->cur_ch, w->cur_bin, w->cur_bap, n, v);
        return good();
      default:
        break;
    }
    if (fnums.hdr[fid] >= 0) {
      account(CAT_HDR);
      v = hdr_field(fid, n, v);
      if (fid == F_bsid && v <= 10) h = H_AC3, sk = 0;  // (the frame's substream: known now)
      else if (fid == F_strmtyp) strmtyp = v;
      else if (fid == F_substreamid) h = H_EAC3 + 8 * cap(strmtyp, 2) + v, sk = 1 + strmtyp;
      return good();
    }
    if (fnums.sid[fid] >= 0) {
      account(fid == F_skiple || fid == F_skipl ? CAT_SKIP : CAT_SIDE);
      v = sid_code(fid, n, v, 0, (1 << n) - 1);
      return good();
    }
    fatal("internal error: a field %s at record level", field_name[fid]);
  }
  bool s(int fid, int n, int& v) {
    const Frame& f = *fr;
    if (fid != F_gaqmant && fid != F_gaqbig) fatal("internal error: a signed field %s", field_name[fid]);
    account(CAT_AHT);
    const int hb = w->cur_bap, lg = w->cur_es;
    v = code_gaq(f, w->r, w->cur_ch, w->cur_bin, hb, lg, w->cur_exp, n, fid == F_gaqbig, v);
    return good();
  }
  bool r(int fid, int lo, int hi, int& v) {
    const Frame& f = *fr;
    switch (fid) {
      case F_crc1ok:
      case F_crc2ok:
        account(CAT_HDR);
        v = hdr_field(fid, 1, v);
        break;
      case F_dexp:
        account(CAT_EXP);
        v = code_dexp(f, w->r, w->cur_blk, w->cur_ch, w->cur_es, w->cur_bin, w->cur_exp, v);
        break;
      case F_q:
        account(CAT_MANT);
        v = code_q(f, w->r, w->cur_blk, w->cur_ch, w->cur_bin, w->cur_bap, lo, hi, v);
        break;
      case F_qfill: {
        int n = 1;
        while ((1 << n) <= hi) n++;
        account(CAT_SIDE);
        v = sid_code(fid, n, v, lo, hi);
        break;
      }
      case F_gain:
      case F_gainfill:
        account(CAT_AHT);
        v = B::code_gain3(f, w->r, w->cur_ch, w->cur_bin, w->cur_bap, v);
        break;
      default:
        fatal("internal error: a record level value %s", field_name[fid]);
    }
    return good();
  }
  bool raw(int fid, int n, U8* p) {
    Hist& H = hist[h];
    switch (fid) {
      case F_tail:
        account(CAT_TAIL);
        B::code_tail(p, n);
        break;
      case F_skipfld:
        account(CAT_SKIP);
        B::raw_field(p, n, RC_SKIP, H.skip[blk], &H.skipn[blk], SKIP_MAX);
        break;
      case F_addbsi:
        account(CAT_HDR);
        B::raw_field(p, n, RC_ADDBSI, H.addbsi, &H.addbsin, ADDBSI_MAX);
        break;
      case F_mixdata:
        account(CAT_HDR);
        B::raw_field(p, n, RC_MIXDATA, H.mixdata, &H.mixdatan, MIXDATA_MAX);
        break;
      case F_blkstrtinfo:
        account(CAT_HDR);
        B::raw_field(p, n, RC_BLKSTRT, H.blkstrt, &H.blkstrtn, BLKSTRT_MAX);
        break;
      default:
        fatal("internal error: raw bits of %s", field_name[fid]);
    }
    return good();
  }
  int peek_bsid() const { return 0; }
  void put_at(int, int, int) {}
  void baps(const U8*, int, int) {}
  // where the walk is: the frame's header, its audfrm, a block, a channel's exponents, its
  // mantissas or its AHT data, the trailer
  void open(int kind, int arg) {
    switch (kind) {
      case K_H:
        B::unit_reset();
        h = H_GLOBAL;
        sk = 4;
        break;
      case K_B:
        B::unit_reset();
        blk = arg;
        memset(sstate, 0, sizeof sstate);
        break;
      case K_E:
        B::exp_open(arg, fr->expstr[w->cur_blk][arg]);
        break;
      case K_M:
        B::mant_open(arg);
        break;
      case K_Q:
        B::aht_open(arg);
        break;
      case K_T:
        B::unit_reset();
        break;
      default:
        break;
    }
  }

  // the walk's callback, a block done: its side information, and its channels' exponents, bit
  // allocation and mantissas into the history (its last block)
  void block(const Frame& f, const Run& r, int b) {
    Hist& H = hist[h];
    B::sid_commit();
    for (int ch = 0; ch <= f.nch; ch++) {
      if (ch == CPL && !r.cplinu) {
        H.has[ch] = 0;
        continue;
      }
      H.strt[ch] = (S16)r.strtmant[ch];
      H.end[ch] = (S16)r.endmant[ch];
      memcpy(H.exps[ch], r.exps[ch], sizeof H.exps[ch]);
      memcpy(H.bap[ch], r.bap[ch], sizeof H.bap[ch]);
      if (r.chahtinu[ch]) {
        H.has[ch] = 1;
      } else {
        memcpy(H.q[ch], f.k[b].q[ch], sizeof H.q[ch]);
        H.has[ch] = 2;
      }
    }
  }
};
