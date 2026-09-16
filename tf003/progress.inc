// ===========================================================================
// progress.inc - one-line progress and ETA on stderr.
//
// COST IN THE CODING LOOP.  The loop does one AND and one compare per byte
// against a mask held in this struct; everything else is behind that gate.
//
// The mask is not a constant, because the per-byte cost of this program spans
// four orders of magnitude - about a microsecond with PPMD alone, about ten
// milliseconds with the whole transformer being trained - and a fixed stride
// that is fine at one end is either far too chatty or far too coarse at the
// other.  Each time the clock is read the stride is halved or doubled to keep
// the reads a fixed WALL-CLOCK distance apart (PROGRESS_TARGET seconds), so the
// cost is a bounded number of clock reads per second whatever the
// configuration, and whatever the clock costs on the platform: measured here at
// 27 ns a read, six reads a second is about two parts in a billion.  The line
// itself is only formatted every PROGRESS_INTERVAL seconds.
//
// Set PROGRESS to 0 to compile the whole thing out.
//
// THE CLOCK.  Both branches are the platform's monotonic counter, called
// directly: QueryPerformanceCounter on Windows, clock_gettime(CLOCK_MONOTONIC)
// elsewhere.  <chrono>'s steady_clock is exactly these two and would be one
// line, but coder0 is built with -nostdinc++ against the MSVC headers and this
// keeps a header that large out of a translation unit that already holds the
// transformer.  <windows.h> is not needed either - the two functions are
// declared below, with the signature it gives them (BOOL is int, WINAPI is
// __stdcall and is ignored on x64, and LARGE_INTEGER* is layout-compatible
// with long long*).  Do not include <windows.h> into this translation unit
// without removing these.
//
// If a toolchain ever refuses both, C11's timespec_get(&ts, TIME_UTC) from
// <time.h> is portable and needs no platform branch at all; it is a wall clock
// rather than a monotonic one, which for a progress display only matters if
// the system time is stepped mid-run.
// ===========================================================================

#ifndef PROGRESS
#define PROGRESS 1
#endif
// starting number of bytes between clock reads; adapts from there, and must be
// a power of two.  PROGRESS_TARGET is the wall-clock spacing it aims for.
#ifndef PROGRESS_CHECK
#define PROGRESS_CHECK 64
#endif
#ifndef PROGRESS_TARGET
#define PROGRESS_TARGET (PROGRESS_INTERVAL/6.0)
#endif
// seconds between printed lines, and how long a run must last before the first
#ifndef PROGRESS_INTERVAL
#define PROGRESS_INTERVAL 0.5
#endif
#ifndef PROGRESS_DELAY
#define PROGRESS_DELAY 1.0
#endif

#if PROGRESS

#if defined(_WIN32)
extern "C" __declspec(dllimport) int __stdcall QueryPerformanceCounter(long long*);
extern "C" __declspec(dllimport) int __stdcall QueryPerformanceFrequency(long long*);
#if defined(_MSC_VER) || defined(__clang__)
#pragma comment(lib, "kernel32.lib")
#endif

static double mono_seconds(void) {
  static double inv_freq = 0.0;
  long long c;
  if( inv_freq==0.0 ) {
    long long f = 0;
    QueryPerformanceFrequency(&f);
    inv_freq = f ? 1.0/double(f) : 0.0;
  }
  QueryPerformanceCounter(&c);
  return double(c)*inv_freq;
}
#else
static double mono_seconds(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return double(ts.tv_sec)+1e-9*double(ts.tv_nsec);
}
#endif

// h:mm:ss, or m:ss below an hour
static void fmt_time(double s, char* buf, int n) {
  if( !(s>=0.0) )
    s = 0.0;
  if( s>359999.0 )
    s = 359999.0;   // 99:59:59
  int t = (int)(s+0.5);
  if( t>=3600 )
    snprintf(buf, n, "%d:%02d:%02d", t/3600, (t/60)%60, t%60);
  else
    snprintf(buf, n, "%d:%02d", t/60, t%60);
}

static void fmt_rate(double bps, char* buf, int n) {
  if( bps>=1048576.0 )
    snprintf(buf, n, "%.1f MB/s", bps/1048576.0);
  else if( bps>=1024.0 )
    snprintf(buf, n, "%.1f KB/s", bps/1024.0);
  else
    snprintf(buf, n, "%.0f B/s", bps);
}

struct Progress {
  uint mask;          // the loop reads the clock when (pos & mask) == 0
  double t0, t_last, t_read;
  double rate;        // bytes/s, smoothed over recent intervals
  uint pos_last, total;
  int shown;

  void Init(uint f_len) {
    total = f_len;
    t0 = t_last = t_read = mono_seconds();
    pos_last = 0;
    rate = 0.0;
    shown = 0;
    mask = PROGRESS_CHECK-1;
  }

  // Called from the coding loop when (pos & mask) == 0.
  NOINLINE void Tick(uint pos) {
    const double now = mono_seconds();

    // keep the reads a fixed wall-clock distance apart, whatever this
    // configuration costs per byte
    const double dt = now-t_read;
    t_read = now;
    if( dt<PROGRESS_TARGET*0.5 ) {
      if( mask<0xFFFFu )
        mask = mask*2+1;
    } else if( dt>PROGRESS_TARGET*2.0 ) {
      if( mask>15u )
        mask >>= 1;
    }

    const double since = now-t_last;
    if( since<PROGRESS_INTERVAL )
      return;
    if( now-t0<PROGRESS_DELAY )
      return;                    // short runs stay quiet

    // Smooth the recent rate rather than using the average since the start:
    // with the transformer the first 1024 bytes of a context are cheaper than
    // the rest (the attention window is still filling), so an average would
    // keep predicting too little.
    const double inst = double(pos-pos_last)/since;
    rate = rate>0.0 ? 0.65*rate+0.35*inst : inst;
    t_last = now;
    pos_last = pos;

    char el[16], eta[16], rt[16];
    fmt_time(now-t0, el, sizeof(el));
    fmt_time(rate>0.0 ? double(total-pos)/rate : 0.0, eta, sizeof(eta));
    fmt_rate(rate, rt, sizeof(rt));
    fprintf(stderr, "\r  %5.1f%%  %u/%u  %s  elapsed %s  remaining %s      ",
            100.0*double(pos)/double(total ? total : 1), pos, total, rt, el,
            eta);
    fflush(stderr);
    shown = 1;
  }

  // Replaces the last progress line with a one-line summary.
  void Done(uint pos) {
    if( !shown )
      return;
    const double el = mono_seconds()-t0;
    char t[16], rt[16];
    fmt_time(el, t, sizeof(t));
    fmt_rate(el>0.0 ? double(pos)/el : 0.0, rt, sizeof(rt));
    fprintf(stderr, "\r  %u bytes in %s (%s)%*s\n", pos, t, rt, 24, "");
    fflush(stderr);
  }
};

#else   // PROGRESS 0: compiled out entirely

struct Progress {
  enum { mask = 0xFFFFFFFFu };  // (pos & mask)==0 only at pos 0; Tick is empty
  void Init(uint) {}
  void Tick(uint) {}
  void Done(uint) {}
};

#endif
