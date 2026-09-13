//  oc_api.h, implemented over oc_coro: the one instance a library holds --
//  one, because the model is static and there is one of it -- with its
//  encoder and decoder coroutines and what FATAL becomes under the API.
//  Included by the programs, whose model 0 this is, and by oggcomp_dll.cpp,
//  where it is the library.
#include "oc_api.h"
struct oc_api_unwind {};
struct oc_inst {
  int busy, dir, flags, err, in_coro, used;
  char msg[1024];
  char name[512];
  Coroutine *co;
  oc_coro<0> enc;
  oc_coro<1> dec;
};
static oc_inst oc_the;
static oc_inst *oc_cur;
static int oc_api_ready;
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
extern "C" {
OC_API void *OC_CALL oggcomp_Alloc(void) {
  if(oc_the.busy)
    return nullptr;
  oc_the.busy = 1;
  oc_the.err = 0;
  oc_the.used = 0;
  oc_the.in_coro = 0;
  oc_the.co = nullptr;
  oc_the.msg[0] = 0;
  return &oc_the;
}
OC_API void OC_CALL oggcomp_Free(void *pp) {
  if(pp == &oc_the)
    oc_the.busy = 0;
}
OC_API int OC_CALL oggcomp_Init(void *pp, int dir, int flags, const char *name) {
  oc_inst *p = (oc_inst *)pp;
  int keep;
  if(p != &oc_the || !p->busy)
    return 8;
  p->err = 0;
  p->msg[0] = 0;
  p->dir = dir ? 1 : 0;
  p->flags = flags;
  p->in_coro = 0;
  snprintf(p->name, sizeof p->name, "%s", name && *name ? name : "the stream");
  tc_verbose = (flags & OC_F_VERBOSE) != 0;
  tc_huge = (flags & OC_F_HUGE) != 0;
  keep = (flags & OC_F_SOLID) != 0 && p->used;
  oc_api_enter(p);
  try {
    if(!oc_api_ready) {
      p->enc.t.init();
      oc_api_ready = 1;
    }
    if(!p->dir) {
      if(keep)
        oc_next(p->enc, p->name);
      else
        oc_fresh(p->enc, p->name);
      p->co = &p->enc;
    } else {
      if(keep)
        oc_next(p->dec, p->name);
      else
        oc_fresh(p->dec, p->name);
      p->co = &p->dec;
    }
    p->used = 1;
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
  if(p != &oc_the || !p->co)
    return 8;
  if(p->err)
    return p->err;
  oc_api_enter(p);
  p->in_coro = 1;
  r = p->dir ? p->dec.coro_call(&p->dec) : p->enc.coro_call(&p->enc);
  p->in_coro = 0;
  oc_api_leave();
  if(r == 3)
    return p->err ? p->err : 8;
  return (int)r;
}
OC_API int OC_CALL oggcomp_getoutlen(void *pp) {
  oc_inst *p = (oc_inst *)pp;
  return p == &oc_the && p->co ? (int)p->co->getoutsize() : 0;
}
OC_API void OC_CALL oggcomp_addinp(void *pp, void *buf, int size) {
  oc_inst *p = (oc_inst *)pp;
  if(p != &oc_the || !p->co)
    return;
  if(size <= 0) {
    p->co->f_quit = 1;
    size = 0;
  }
  p->co->addinp((byte *)buf, (uint)size);
}
OC_API void OC_CALL oggcomp_addout(void *pp, void *buf, int size) {
  oc_inst *p = (oc_inst *)pp;
  if(p == &oc_the && p->co && size > 0)
    p->co->addout((byte *)buf, (uint)size);
}
OC_API const char *OC_CALL oggcomp_Error(void *pp) {
  oc_inst *p = (oc_inst *)pp;
  return p == &oc_the ? p->msg : "";
}
OC_API int OC_CALL oggcomp_StreamVersion(void) { return OC_VER; }
OC_API unsigned long long OC_CALL oggcomp_TableBytes(void) { return oc_api_ready ? (unsigned long long)tabs.bytes() : 0; }
OC_API const char *OC_CALL oggcomp_Describe(void) {
  static char d[160];
  snprintf(d, sizeof d, "oggcomp model, stream version %d, %llu MB of tables%s", OC_VER, (unsigned long long)(oc_api_ready ? tabs.bytes() >> 20 : 0), oc_api_ready ? "" : " (not mapped yet)");
  return d;
}
OC_API void OC_CALL oggcomp_Report(void *pp) {
  oc_inst *p = (oc_inst *)pp;
  int i;
  double tot = 0;
  if(p != &oc_the)
    return;
  for(i = 0; i < STG_N; i++)
    tot += tc_bits[i];
  fprintf(stderr, "%s: %.0f bytes of model, %" PRIu64 " MB of tables\n", ogc_prog, tot / 8, (u64)(oc_api_ready ? tabs.bytes() >> 20 : 0));
  if(!p->dir)
    for(i = 0; i < STG_N; i++) {
      fprintf(stderr, "  %-8s %12.0f bytes  %5.2f%%", TC_STAGE[i], tc_bits[i] / 8, tot > 0 ? 100.0 * tc_bits[i] / tot : 0.0);
      if(tc_syms[i])
        fprintf(stderr, "  %" PRIu64 " values, %.3f bits each", tc_syms[i], tc_bits[i] / (double)tc_syms[i]);
      fputc('\n', stderr);
    }
}
}
