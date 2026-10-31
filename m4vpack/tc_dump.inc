// tc_dump.inc -- fam.inc's tc_dump: a family's dump (a build with M4VPACK_DUMP).

//  (each family's own, for each model: R as its model starts; none for a
//  decompressor's, fam null)
struct tc_dump {
  FILE *f;
  void Init(const char *fam) {
    f = nullptr;
    const char *dir = getenv("M4VPACK_DUMP");
    if(!fam || !dir || !*dir)
      return;
    size_t n = strlen(dir) + strlen(fam) + sizeof "/.dump";
    char *fn = (char *)xmalloc(n);
    snprintf(fn, n, "%s/%s.dump", dir, fam);
    f = fopen(fn, "ab");
    if(!f)
      fatal("cannot write %s", fn);
    free(fn);
    putc('R', f);
  }
  void Quit() {
    if(f)
      fclose(f);
    f = nullptr;
  }
  template <class... A> void rec(char tag, A... a) {
    if(!f)
      return;
    putc(tag, f);
    int unused[] = {0, (tc_dump_put(f, (i64)a), 0)...};
    (void)unused;
  }
  void vars(pcInt v, int n) {  // M
    if(!f)
      return;
    putc('M', f);
    for(int i = 0; i < n; i++)
      tc_dump_put(f, v[i]);
  }
};
