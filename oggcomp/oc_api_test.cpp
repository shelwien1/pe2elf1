/*  The library's C interface driven from a program, the way a caller that is
    not oggcomp or oggdet drives it: one instance, both directions, solid
    and not, and the calls a caller can get wrong.  Built by `./mk.sh
    apitest` over the same translation unit as the library, so it tests
    model 0 at static addresses; ./t.sh runs it when it is there.

        ./oc_api_test testfiles/tiny-8k-q0.ogg

    exits 0 when every check passes, and prints one line per check.  */
#include "oggcomp_dll.cpp"
#include <vector>
static int checks, failed;
static void check(int ok, const char *what, const char *how) {
  checks++;
  if(!ok)
    failed++;
  printf("%s  %-40s %s\n", ok ? "ok    " : "FAILED", what, how);
}
static std::vector<u8> slurp(const char *path) {
  std::vector<u8> v;
  FILE *f = fopen(path, "rb");
  int c;
  if(!f)
    return v;
  while((c = fgetc(f)) != EOF)
    v.push_back((u8)c);
  fclose(f);
  return v;
}
//  One stream through the instance: Init, then Loop fed 4 kB of input at a
//  time and drained 4 kB of output at a time, as oc_api.h describes.
//  Returns Loop's last answer: 0 for done, 4 and up for an error.
static int drive(void *p, int dir, int flags, const std::vector<u8> &in, std::vector<u8> &out) {
  static u8 ibuf[4096], obuf[4096];
  sz at = 0;
  int r, fed_end = 0;
  out.clear();
  r = oggcomp_Init(p, dir, flags, "test");
  if(r)
    return r;
  oggcomp_addout(p, obuf, sizeof obuf);
  for(;;) {
    r = oggcomp_Loop(p);
    if(r == OC_NEED_INPUT) {
      sz n = in.size() - at < sizeof ibuf ? in.size() - at : sizeof ibuf;
      if(n == 0 && fed_end)
        return 8;
      if(n)
        memcpy(ibuf, &in[at], n);
      at += n;
      fed_end = n == 0;
      oggcomp_addinp(p, ibuf, (int)n);
    } else if(r == OC_NEED_OUTPUT || r == OC_DONE) {
      int n = oggcomp_getoutlen(p);
      out.insert(out.end(), obuf, obuf + n);
      if(r == OC_DONE)
        return 0;
      oggcomp_addout(p, obuf, sizeof obuf);
    } else
      return r;
  }
}
int main(int argc, char **argv) {
  std::vector<u8> in, a, b, c, back;
  void *p;
  int r;
  char how[200];
  if(argc != 2) {
    fprintf(stderr, "usage: oc_api_test file.ogg\n");
    return 2;
  }
  in = slurp(argv[1]);
  if(in.empty()) {
    fprintf(stderr, "oc_api_test: cannot read %s\n", argv[1]);
    return 2;
  }
  //  Loop with no output buffer: an error return, not a wild store.
  p = oggcomp_Alloc();
  r = oggcomp_Init(p, OC_ENCODE, 0, "test");
  if(!r) {
    oggcomp_addinp(p, &in[0], (int)in.size());
    for(r = 0; r == 0;) {
      r = oggcomp_Loop(p);
      if(r == OC_NEED_INPUT) {
        oggcomp_addinp(p, nullptr, 0);
        r = 0;
      }
    }
  }
  snprintf(how, sizeof how, "Loop returned %d", r);
  check(r == 8, "Loop with no output buffer", how);
  oggcomp_Free(p);
  //  Encode, then decode, on one instance, both solid: the first Init of
  //  each direction is fresh, whatever the other direction has done.
  p = oggcomp_Alloc();
  r = drive(p, OC_ENCODE, OC_F_SOLID, in, a);
  snprintf(how, sizeof how, "%d -> %d bytes", (int)in.size(), (int)a.size());
  check(r == 0 && !a.empty(), "encode, solid", how);
  r = drive(p, OC_DECODE, OC_F_SOLID, a, back);
  snprintf(how, sizeof how, "Loop %d, %d bytes back", r, (int)back.size());
  check(r == 0 && back == in, "then decode, solid, same instance", how);
  //  A solid pair in one direction codes the second stream with what the
  //  first taught it, and a fresh encode is the first solid one.
  r = drive(p, OC_ENCODE, OC_F_SOLID, in, b);
  snprintf(how, sizeof how, "%d then %d bytes", (int)a.size(), (int)b.size());
  check(r == 0 && b.size() < a.size(), "a second solid encode costs less", how);
  r = drive(p, OC_ENCODE, 0, in, c);
  snprintf(how, sizeof how, "%d bytes, %s", (int)c.size(), c == a ? "identical" : "different");
  check(r == 0 && c == a, "a fresh encode is the first solid one", how);
  r = drive(p, OC_DECODE, 0, c, back);
  check(r == 0 && back == in, "and decodes fresh", back == in ? "byte for byte" : "differs");
  oggcomp_Free(p);
  //  Loop again after OC_NEED_OUTPUT with no new buffer -- addout skipped,
  //  or given a size of 0, which it ignores: an error return and the
  //  caller's buffer untouched past its end, where the coroutine used to
  //  resume into the write it had stopped before; and the run goes on as
  //  if nothing had happened once a buffer is given.
  {
    u8 small[64];
    std::vector<u8> out;
    int refused = 0, guard = 1, i;
    p = oggcomp_Alloc();
    r = oggcomp_Init(p, OC_ENCODE, 0, "test");
    memset(small, 0xAA, sizeof small);
    oggcomp_addout(p, small, 16);
    oggcomp_addinp(p, &in[0], (int)in.size());
    while(!r) {
      r = oggcomp_Loop(p);
      if(r == OC_NEED_INPUT) {
        oggcomp_addinp(p, nullptr, 0);
        r = 0;
      } else if(r == OC_NEED_OUTPUT || r == OC_DONE) {
        out.insert(out.end(), small, small + oggcomp_getoutlen(p));
        if(r == OC_DONE)
          break;
        if(!refused) {
          oggcomp_addout(p, small, 0);
          refused = oggcomp_Loop(p) == 8;
          for(i = 16; i < (int)sizeof small; i++)
            if(small[i] != 0xAA)
              guard = 0;
        }
        memset(small, 0xAA, sizeof small);
        oggcomp_addout(p, small, 16);
        r = 0;
      }
    }
    snprintf(how, sizeof how, "%s, %s", refused ? "8" : "not refused", guard ? "nothing past the buffer" : "WROTE PAST THE BUFFER");
    check(refused && guard, "Loop after NEED_OUTPUT with no buffer", how);
    r = r == OC_DONE ? drive(p, OC_DECODE, 0, out, back) : r;
    check(r == 0 && back == in, "and the run goes on once one is given", back == in ? "byte for byte" : "differs");
    oggcomp_Free(p);
  }
  printf("oc_api_test: %d/%d checks passed\n", checks - failed, checks);
  return failed ? 1 : 0;
}
