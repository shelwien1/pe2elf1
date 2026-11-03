//  The model as a table of pointers: the functions of oc_api.h, which run()
//  and the workers of a run on threads (oc_mt.inc) call through it, and the
//  level they code at, which Init is told in its flags (OC_F_LEVEL).
//
//  Until the levels, a model other than the program's own was a library,
//  oggcompN.dll or oggcompN.so, built from an IDX directory of its own and
//  loaded here by the number in a stream's header.  The four models there
//  were are the program's levels now (levels_h.inc), so there is nothing to
//  load: a level is in the program or it is not, and the number in the
//  header names it (oggcomp.cpp, oc_level_of).
#include "oc_api.h"
struct oc_api {
  int level;  // the level, which goes in the header's level byte
  void *(OC_CALL *Alloc)(void);
  void(OC_CALL *Free)(void *);
  int(OC_CALL *Init)(void *, int, int, const char *);
  int(OC_CALL *Loop)(void *);
  int(OC_CALL *getoutlen)(void *);
  void(OC_CALL *addinp)(void *, void *, int);
  void(OC_CALL *addout)(void *, void *, int);
  const char *(OC_CALL *Error)(void *);
  int(OC_CALL *StreamVersion)(void);
  unsigned long long(OC_CALL *TableBytes)(void);
  const char *(OC_CALL *Describe)(void);
  void(OC_CALL *Report)(void *);
  unsigned long long(OC_CALL *Done)(void *);
  const char *(OC_CALL *Stage)(void *, int, double *, unsigned long long *);
};
//  The program's own functions, for a run at `level`.
static void oc_api_load(oc_api *a, int level) {
  a->level = level;
  a->Alloc = oggcomp_Alloc;
  a->Free = oggcomp_Free;
  a->Init = oggcomp_Init;
  a->Loop = oggcomp_Loop;
  a->getoutlen = oggcomp_getoutlen;
  a->addinp = oggcomp_addinp;
  a->addout = oggcomp_addout;
  a->Error = oggcomp_Error;
  a->StreamVersion = oggcomp_StreamVersion;
  a->TableBytes = oggcomp_TableBytes;
  a->Describe = oggcomp_Describe;
  a->Report = oggcomp_Report;
  a->Done = oggcomp_Done;
  a->Stage = oggcomp_Stage;
}
