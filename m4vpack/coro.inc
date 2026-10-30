// coro.inc -- the compressor and the decompressor, as coroutines (Lib3's, as h264p's): a run,
// on the buffers of its input and output, at a level.

// The stream: "m4vpack", the format's version, the model's level; then the arithmetic coded data:
// the chunks (kind, then the records of a VOP or a raw chunk's length and bytes), the end, and
// the CRC-32 of the stream (32 bits at 1/2 each).
enum { M4V_MAGIC = 7, M4V_AT_VERSION = M4V_MAGIC, M4V_AT_LEVEL, M4V_HEAD };  // the header's bytes
static const U8 magic[M4V_MAGIC] = { 'm', '4', 'v', 'p', 'a', 'c', 'k' };
constexpr int M4V_VERSION = 4;  // the format's

// a compression's statistics (its summary line)
typedef struct Stats {
  U32 vops, packed, by_type[PT_N], raw_unsup, raw_long, raw_parse, raw_verify;
  double in_bytes, raw_bytes, vop_bytes;
  U64 parts;    // -t: the parts
  int threads;  // and the threads
} Stats;

// what W_VERIFY's VOP written back is compared with, as its writer flushes it
typedef struct Cmp {
  const U8* p;
  size_t n, at;
  int differs;
} Cmp;
static void cmp_flush(void* ctx, const U8* p, size_t n) {
  Cmp* c = (Cmp*)ctx;
  if (c->differs || n > c->n - c->at || memcmp(c->p + c->at, p, n)) c->differs = 1;
  else c->at += n;
}

#include "m4vcoro.inc"  // M4vCoro: a run of c or d

// A run's coroutine, on pages of its own: zeroed, and taking memory only where the run touches
// them (its tables for the largest pictures). coro_new() makes it (plain data: then Init()),
// coro_free() frees it after its Quit().
template <class T>
T* coro_new(void) {
  return (T*)map_pages(sizeof(T));
}
template <class T>
void coro_free(T* p) {
  unmap_pages(p, sizeof(T));
}

// The output of a call: a file, or (c -v) the input to compare with
typedef struct Sink {
  FILE* f;
  int compare;
  U64 pos, diff_at;
  int differs, err;
} Sink;
static void sink_write(Sink* s, const U8* p, size_t n) {
  static U8 cmp[IO_BUF];  // (the main thread's)
  if (!s->compare) {
    if (n && fwrite(p, 1, n, s->f) != n) s->err = 1;
    s->pos += n;
    return;
  }
  while (n && !s->differs) {
    const size_t k = n < sizeof(cmp) ? n : sizeof(cmp);
    const size_t got = fread(cmp, 1, k, s->f);
    size_t i;
    for (i = 0; i < got && cmp[i] == p[i]; i++) {}
    if (i < k) { s->differs = 1; s->diff_at = s->pos + i; return; }
    s->pos += k;
    p += k;
    n -= k;
  }
}

// Runs the coroutine c in the buffer protocol to its end: in() gives it its next input when it
// asks (Y_IN), out(more) takes what it put in its output buffer when that is full (Y_OUT, more:
// and gives it the next) or as it ends (Y_END, or Y_FAIL after an error: what drive() returns);
// show(code) after each yield (what it tells, Y_VIDEO; the progress line)
template <class C, class In, class Out, class Show>
uint drive(C& c, In in, Out out, Show show) {
  for (;;) {
    const uint r = c.coro_call(&c);
    if (r == Y_IN) in();
    else if (r != Y_VIDEO) out(r == Y_OUT);
    show(r);
    if (r != Y_IN && r != Y_OUT && r != Y_VIDEO) return r;
  }
}

// c on files: its input from f (the n bytes at pre read already, first), its output to s, and
// show(code) after each yield
static U8 io_bufs[2][IO_BUF];  // (the main thread's: input, output)
template <class C, class Show>
uint run_file(C& c, FILE* f, Sink* s, Show show, const U8* pre = 0, size_t n = 0) {
  const uint kIo = sizeof io_bufs[0];
  U8 *ib = io_bufs[0], *ob = io_bufs[1];
  c.addout(ob, kIo);
  if (n) c.addinp((byte*)pre, (uint)n);
  return drive(
      c,
      [&]() {
        const uint l = (uint)fread(ib, 1, kIo, f);
        if (!l) c.f_quit = 1;
        c.addinp(ib, l);
      },
      [&](bool more) {
        sink_write(s, c.outbeg, c.getoutsize());
        if (more) c.addout(ob, kIo);
      },
      show);
}


