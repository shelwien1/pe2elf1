//  oc_api.h, implemented over oc_coro: the instance a thread holds -- one
//  per thread, because the model's state is the thread's (OC_TLS,
//  oc_platform.inc) and a thread has one of it -- with its encoder and
//  decoder coroutines at each level of the build and what FATAL becomes
//  under the API.  Included by the programs, which run it through
//  oc_load.inc, and by oggcomp_dll.cpp, where it is the library.
#include "oc_api.h"
struct oc_api_unwind {};
//  A level's two coroutines, the encoder and the decoder: each a model of
//  that level, and its walk.
template <class Lv> struct oc_lvco {
  oc_coro<0, Lv> enc;
  oc_coro<1, Lv> dec;
};
//  `used` is per direction and level: what a solid Init keeps is that
//  model as its last stream left it, and one that has not been Init'd in
//  this instance has nothing to keep -- its families are not even wired --
//  whatever the others have done.
struct oc_inst {
  int busy, dir, level, flags, err, in_coro, used[2][10];
  int need_out;  // Loop said OC_NEED_OUTPUT, and no addout has answered it
  char msg[1024];
  char name[512];
  Coroutine *co;            // the coroutine Init chose: its direction's, at its level
  uint (*step)(oc_inst *);  // co's next step, called as its type
#define OC_INST_LV(n) oc_lvco<Lv##n> l##n;
  OC_LEVELS(OC_INST_LV)
#undef OC_INST_LV
};
static OC_TLS oc_inst *oc_the;  // the thread's instance, mapped by its first Alloc
static OC_TLS oc_inst *oc_cur;
static OC_TLS u64 oc_tables_bytes;  // the tables of the level the thread last Init'd
//  The thread's largest state, mapped at its first Init -- in the try of
//  Init, so a mapping refused is an error the API returns -- and given
//  back, with the tables and the instance, when it frees the instance.
template <class T> static void oc_block(T *&p) {
  if(!p)
    p = new(oc_map_block(sizeof(T))) T;
}
template <class T> static void oc_block(T *&p, sz n) {
  if(!p)
    p = (T *)oc_map_block(n * sizeof(T));
}
static void oc_blocks_map(void) {
  oc_block(hist);
  oc_block(tcp);
  oc_block(vb_ar);
  oc_block(oc_hc);
  oc_block(vb_cs, VB_CSMAX);
  oc_block(in_win, IN_WIN);
  oc_block(jb, JB_MAX);
}
template <class T> static void oc_unblock(T *&p, sz n) {
  oc_unmap_block(p, n);
  p = nullptr;
}
static void oc_blocks_unmap(void) {
  //  The history's spill -- tables past its pool, calloc'd for a setup that
  //  asks for more -- is freed by reset(), not by unmapping the pool: a Free
  //  without it leaked them, once per instance, and a run on threads frees
  //  an instance per worker.
  if(hist)
    hist->reset();
  oc_unblock(hist, sizeof *hist);
  oc_unblock(tcp, sizeof *tcp);
  oc_unblock(vb_ar, sizeof *vb_ar);
  oc_unblock(oc_hc, sizeof *oc_hc);
  oc_unblock(vb_cs, VB_CSMAX * sizeof *vb_cs);
  oc_unblock(in_win, IN_WIN);
  oc_unblock(jb, JB_MAX);
#define OC_UNMAP_LV(n) oc_tabs<Lv##n>().unmap();
  OC_LEVELS(OC_UNMAP_LV)
#undef OC_UNMAP_LV
  oc_tables_bytes = 0;
}
//  FATAL under the API: no exit and no file to drop.  Inside the coroutine
//  the error is yielded and never returned from -- a call after it finds
//  the same yield -- and outside it, in Init, it unwinds to the API
//  function.  The message was captured by message() on the way here.
static void oc_api_die(int code) {
  oc_inst *p = oc_cur;
  p->err = 4 + (code > 0 && code <= 4 ? code : 4);
  if(p->in_coro)
    for(;;)
      yield(p->co, 3);
  throw oc_api_unwind();
}
static void oc_api_enter(oc_inst *p) {
  oc_cur = p;
  ogc_die_hook = oc_api_die;
  ogc_msg_dst = p->msg;
  ogc_msg_cap = sizeof p->msg;
}
static void oc_api_leave(void) {
  oc_cur = nullptr;
  ogc_die_hook = nullptr;
  ogc_msg_dst = nullptr;
}
//  One step of the coroutine Init chose, as its own type: coro_call takes
//  the type it is to run.
template <class C> static uint oc_step(oc_inst *p) {
  C *c = (C *)p->co;
  return c->coro_call(c);
}
//  Init at level Lv: the level's tables mapped and the model wired at the
//  thread's first Init of it, then the direction's coroutine of that level,
//  fresh or (solid) as its last stream left it.
template <class Lv> static void oc_init_lv(oc_inst *p, oc_lvco<Lv> &l, int keep) {
  oc_blocks_map();
  if(!oc_tabs<Lv>().mapped)
    l.enc.t.init();
  oc_tables_bytes = oc_tabs<Lv>().bytes();
  if(!p->dir) {
    if(keep)
      oc_next(l.enc, p->name);
    else
      oc_fresh(l.enc, p->name);
    p->co = &l.enc;
    p->step = oc_step<oc_coro<0, Lv>>;
  } else {
    if(keep)
      oc_next(l.dec, p->name);
    else
      oc_fresh(l.dec, p->name);
    p->co = &l.dec;
    p->step = oc_step<oc_coro<1, Lv>>;
  }
}
extern "C" {
OC_API void *OC_CALL oggcomp_Alloc(void) {
  if(oc_the && oc_the->busy)
    return nullptr;
  if(!oc_the) {
    void *m;
#ifdef _WIN32
    m = VirtualAlloc(nullptr, sizeof(oc_inst), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
    m = mmap(nullptr, sizeof(oc_inst), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if(m == MAP_FAILED)
      m = nullptr;
#endif
    if(!m)
      return nullptr;
    oc_the = new(m) oc_inst;
  }
  oc_the->busy = 1;
  oc_the->err = 0;
  memset(oc_the->used, 0, sizeof oc_the->used);
  oc_the->in_coro = 0;
  oc_the->need_out = 0;
  oc_the->co = nullptr;
  oc_the->msg[0] = 0;
  return oc_the;
}
//  The instance back, and with it everything the thread mapped for the
//  model: its next Alloc and Init start from nothing, as a new process.
OC_API void OC_CALL oggcomp_Free(void *pp) {
  if(!pp || pp != oc_the)
    return;
  oc_blocks_unmap();
  oc_unmap_block(oc_the, sizeof(oc_inst));
  oc_the = nullptr;
}
OC_API int OC_CALL oggcomp_Init(void *pp, int dir, int flags, const char *name) {
  oc_inst *p = (oc_inst *)pp;
  int keep, level = OC_LEVEL_OF(flags);
  if(!p || p != oc_the || !p->busy)
    return 8;
  if(!level)
    level = OC_LEVEL_DEFAULT;
  p->err = 0;
  p->msg[0] = 0;
  p->dir = dir ? 1 : 0;
  p->level = level;
  p->flags = flags;
  p->in_coro = 0;
  p->need_out = 0;
  p->co = nullptr;
  snprintf(p->name, sizeof p->name, "%s", name && *name ? name : "the stream");
  tc_verbose = (flags & OC_F_VERBOSE) != 0;
  tc_huge = (flags & OC_F_HUGE) != 0;
  oc_api_enter(p);
  try {
    FATAL_UNLESS(level <= 9 && oc_has_level(level), "%s: this build has no level %d", p->name, level);
    keep = (flags & OC_F_SOLID) != 0 && p->used[p->dir][level];
    switch(level) {
#define OC_INIT_LV(n)              \
  case n:                          \
    oc_init_lv(p, p->l##n, keep); \
    break;
      OC_LEVELS(OC_INIT_LV)
#undef OC_INIT_LV
    }
    p->used[p->dir][level] = 1;
  } catch(oc_api_unwind &) {
    oc_api_leave();
    p->co = nullptr;
    return p->err;
  }
  oc_api_leave();
  return 0;
}
OC_API int OC_CALL oggcomp_Loop(void *pp) {
  oc_inst *p = (oc_inst *)pp;
  uint r;
  if(!p || p != oc_the || !p->co)
    return 8;
  //  Nowhere for the output to go.  The coroutine's pin writes a byte and
  //  then asks whether it has run out, which is right for a buffer and a
  //  null dereference for no buffer, so the check has to be here.
  if(!p->co->pin[1].beg)
    return 8;
  //  Nor after OC_NEED_OUTPUT until a buffer has been given: the coroutine
  //  resumes into the write it stopped before, without asking again, so a
  //  Loop with the full buffer still attached -- addout skipped, or given
  //  a size of 0, which it ignores -- wrote past the end of the caller's.
  if(p->need_out)
    return 8;
  if(p->err)
    return p->err;
  oc_api_enter(p);
  p->in_coro = 1;
  r = p->step(p);
  p->in_coro = 0;
  oc_api_leave();
  if(r == 3)
    return p->err ? p->err : 8;
  if(r == OC_NEED_OUTPUT)
    p->need_out = 1;
  return (int)r;
}
OC_API int OC_CALL oggcomp_getoutlen(void *pp) {
  oc_inst *p = (oc_inst *)pp;
  return p && p == oc_the && p->co ? (int)p->co->getoutsize() : 0;
}
OC_API void OC_CALL oggcomp_addinp(void *pp, void *buf, int size) {
  oc_inst *p = (oc_inst *)pp;
  if(!p || p != oc_the || !p->co)
    return;
  if(size <= 0) {
    p->co->f_quit = 1;
    size = 0;
  }
  p->co->addinp((byte *)buf, (uint)size);
}
OC_API void OC_CALL oggcomp_addout(void *pp, void *buf, int size) {
  oc_inst *p = (oc_inst *)pp;
  if(p && p == oc_the && p->co && size > 0) {
    p->co->addout((byte *)buf, (uint)size);
    p->need_out = 0;
  }
}
OC_API const char *OC_CALL oggcomp_Error(void *pp) {
  oc_inst *p = (oc_inst *)pp;
  return p && p == oc_the ? p->msg : "";
}
OC_API int OC_CALL oggcomp_StreamVersion(void) { return OC_VER; }
OC_API unsigned long long OC_CALL oggcomp_TableBytes(void) { return (unsigned long long)oc_tables_bytes; }
OC_API const char *OC_CALL oggcomp_Describe(void) {
  static char d[160];
  int k = snprintf(d, sizeof d, "oggcomp, stream version %d, levels", OC_VER);
  for(int l : oc_levels)
    k += snprintf(d + k, sizeof d - (sz)k, " -%d", l);
  return d;
}
OC_API unsigned long long OC_CALL oggcomp_Done(void *pp) {
  oc_inst *p = (oc_inst *)pp;
  return p && p == oc_the ? (unsigned long long)oc_done : 0;
}
OC_API const char *OC_CALL oggcomp_Stage(void *pp, int i, double *bits, unsigned long long *values) {
  oc_inst *p = (oc_inst *)pp;
  if(!p || p != oc_the || i < 0 || i >= STG_N)
    return nullptr;
  if(bits)
    *bits = tc_bits[i];
  if(values)
    *values = tc_syms[i];
  return TC_STAGE[i];
}
OC_API void OC_CALL oggcomp_Report(void *pp) {

  oc_inst *p = (oc_inst *)pp;
  int i;
  double tot = 0;
  if(!p || p != oc_the)
    return;
  for(i = 0; i < STG_N; i++)
    tot += tc_bits[i];
  fprintf(stderr, "%s: %.0f bytes of model at -%d, %" PRIu64 " MB of tables\n", ogc_prog, tot / 8, p->level, (u64)(oc_tables_bytes >> 20));
  if(!p->dir)
    for(i = 0; i < STG_N; i++) {
      fprintf(stderr, "  %-8s %12.0f bytes  %5.2f%%", TC_STAGE[i], tc_bits[i] / 8, tot > 0 ? 100.0 * tc_bits[i] / tot : 0.0);
      if(tc_syms[i])
        fprintf(stderr, "  %" PRIu64 " values, %.3f bits each", tc_syms[i], tc_bits[i] / (double)tc_syms[i]);
      fputc('\n', stderr);
    }
}
}
