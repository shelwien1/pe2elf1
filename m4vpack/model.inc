// model.inc -- what codes and decodes the records of a stream, at a level (levels.inc): each
// decision goes to one of the level's families (fam.inc), whose contexts its IDX/l<L>/*.idx
// builds from the variables the model gives it (famvars.inc). The same functions code and decode
// every record: compressing (ENC) they take the values, decompressing they return them, so the
// two directions can't drift apart.
//
// The model is its elements, as the families go, stacked in one line each on those below (what C++
// lets a class see): what they share (model_core.inc); the headers (model_hdr.inc); the residual,
// the intra DC differences and the coefficients (model_res.inc); the vectors (model_mv.inc); the
// macroblocks, which code their vectors and blocks with the two before and their stuffing with the
// headers' family (model_mb.inc); and the VOP, the model itself, which walks one (vop_walk.inc).

#include "model_core.inc"
#include "model_hdr.inc"
#include "model_res.inc"
#include "model_mv.inc"
#include "model_mb.inc"

// The model of a run, at the level LV: compressing (f_DEC 0) or decompressing (1). Plain data, on
// the zeroed pages of its run's coroutine (coro.inc): Init() as the run is made, start() as it
// starts (its families' tables mapped), Quit() as it ends.
template <int f_DEC, class LV>
struct Model : ModelMb<f_DEC, LV> {
  typedef ModelMb<f_DEC, LV> B;  // (what the VOP takes of the elements)
  using B::c;
  using B::mbcur;
  using B::mbref;
  using B::ref_mbs;
  using B::mo;
  using B::rec;
  using B::mb_room;
  using B::hdr_flag;
  using B::code_vop_hdr;
  using B::code_vp;
  using B::code_tail;
  using B::code_mb;
  // (a level reads no variable its parts cannot make: famvars.inc)
#define M_PARTS(F)                                                         \
  static_assert(LV::recon || !(LV::F::Uses & M_##F##_needs<NEED_REC>()), \
                "a level without the decoder reads a variable of " #F " that needs it");
  M4V_FAMS(M_PARTS)
#undef M_PARTS
  U8 tailbuf[TAIL_MAX];
  MB mrec;  // the macroblock vop_walk() has

  void Init(void) {
#define M_INIT(F) this->f_##F.Init(), this->t_##F = nullptr;
    M4V_FAMS(M_INIT)
#undef M_INIT
    this->core_init(LV::recon);
  }
  void Quit(void) {
#define M_QUIT(F)                                                           \
  if (this->t_##F) {                                                        \
    LV::F::quit(this->t_##F);                                               \
    tc_unmap(this->t_##F, sizeof(typename LV::F::T));                       \
    this->t_##F = nullptr;                                                  \
  }                                                                         \
  this->f_##F.Quit();
    M4V_FAMS(M_QUIT)
#undef M_QUIT
    this->core_quit(LV::recon);
  }
  // the run starts: the families' tables mapped and wired to the coder co
  void start(Coder* co) {
    c = co;
#define M_MAP(F)                               \
  this->t_##F = tc_table<typename LV::F::T>(); \
  LV::F::init(this->t_##F);                    \
  this->f_##F.wire(this->t_##F, c);
    M4V_FAMS(M_MAP)
#undef M_MAP
    this->hdr_start();
    ref_mbs = 0;
  }

  // One VOP (after its start code). W_VERIFY: parse it and write it again (into pb); W_ENCODE:
  // parse it and code the records; W_DECODE: decode the records and write the VOP (with its start
  // code). W_FAIL: it can't be (parsed, written: unsupported or damaged data), W_NOMEM: no memory.
  // (vop_walk.inc)
  int vop_walk(Ctx* cx, int mode, GB* gb, PB* pb);
};

#include "vop_walk.inc"
