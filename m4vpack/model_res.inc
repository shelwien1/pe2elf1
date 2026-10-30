// model_res.inc -- the model's residual (model.inc): the intra DC differences and the blocks'
// coefficients (code_block.inc); the dcs, sig, lvl, sgn and esc families.

template <int f_DEC, class LV>
struct ModelRes : ModelHdr<f_DEC, LV> {
  typedef ModelHdr<f_DEC, LV> B;  // (what this element takes of those below)
  using B::ENC;
  using B::c;
  using B::rec;
  using B::est;
  using B::est_level;
  using B::est_trust;
  using B::est_account;
  using B::checked;
  using B::sel;
  M_FAM(dcs)
  M_FAM(sig)
  M_FAM(lvl)
  M_FAM(sgn)
  M_FAM(esc)

  // intra DC difference (the dcs family): its magnitude, then its sign. Contexts: the previous
  // block's size, the left and above blocks' (and their coefficient counts), whether the block has
  // AC coefficients, the quantiser; for the sign, the left and above blocks' and the previous one's
  // (e4: the intra estimate's difference, quarter levels, if eok; tq how far to trust it)
  int code_dc(VState* s, int lc, int v, const BlkCtx* Lb, const BlkCtx* Ab, int coded, int qs, int eok, int e4,
              int tq) {
    M_dcs_v D;
    tcx X;
    i64 a;
    int neg;
    const int ea = e4 < 0 ? -e4 : e4;
    D.lc = lc;
    D.prev = s->dc_prev[lc];
    D.ls = Lb && Lb->dcs >= 0 ? Lb->dcs : kDcSizeNone;
    D.as = Ab && Ab->dcs >= 0 ? Ab->dcs : kDcSizeNone;
    D.coded = coded;
    D.qs = qs;
    D.lbn = Lb ? cap(Lb->n, kCap) : kNone;
    D.abn = Ab ? cap(Ab->n, kCap) : kNone;
    D.ls2 = Lb && Lb->dcs > 0 ? 1 + Lb->dcsg : 0;
    D.as2 = Ab && Ab->dcs > 0 ? 1 + Ab->dcsg : 0;
    D.psg = s->dc_sgn[lc];
    D.sz = 0;
    D.ie = eok ? 1 + cap(ea, kDcEstCap) : 0;
    D.es = eok && ea >= kQ / 2 ? 1 + (e4 < 0) : 0;
    D.tq = tq;
    f_dcs.make(D, X);
    f_dcs.hint = eok ? cap(q4_round(ea), kDcMax) : -1;
    a = checked(f_dcs.code(X, ENC ? (v < 0 ? -v : v) : 0), 0, kDcMax);
    f_dcs.hint = -1;
    s->dc_prev[lc] = bitlen((int)a);
    if (!a) return 0;
    D.sz = bitlen((int)a);
    sel(f_dcs, D);
    neg = f_dcs.bit(TC_NODE, ENC && v < 0);
    s->dc_sgn[lc] = 1 + neg;
    return neg ? -(int)a : (int)a;
  }

  // a block's coefficients: significance, levels, signs, escape modes (code_block.inc)
  void code_block(VState* s, Blk* b, int n, int intra, int dcvlc, int coded, int cls, int nctx, int qs,
                  int mvc, int rnz, const BlkCtx* Lb, const BlkCtx* Ab, BlkCtx* out, const BlkCtx* Xb);
};

#include "code_block.inc"
