// mtdbg.inc -- MTwrap_v3_dbg's statistics: what each thread of a run did and when, written to
// dbg.txt (TSV) as the run quits. mtdbg.py reads it.
//
// Each thread logs its own events, in time order and without locks: the main loop's log is
// MTwrap's, each worker's its thread_pipe's. MTwrap::Quit() (the threads joined) merges them into
// dbg.txt, a row per event:
//
//   run     the MTwrap run in the process (0, 1, ...: c -t -v runs c, then d); the first truncates
//           dbg.txt, the others append
//   mode    c or d
//   thread  M (the main loop) or W<i> (worker i)
//   event   what happened (below)
//   t_us    when (an interval: when it began), from the run's Init()
//   dur_us  an interval's length (0: an instant)
//   worker  the worker it concerns (-1: none)
//   bytes   the bytes it moved
//   x       per event (below)
//   note    per event (below)
//
// M, the main loop:
//   param   a parameter of the run: note its name, bytes its value
//   read    waited for its input (chkinp(): the driver reads; c: the splitter makes the next bytes);
//           bytes: the input then there
//   write   waited for its output to be taken (chkout(): the driver writes); bytes: that buffer
//   wait    waited for a worker (main_wait()): x 1 the parse (d) or the dealing (c) stopped by
//           worker's full input ring (d: dynbuff refused, the worker of the output being busy),
//           2 the input ended (the workers to finish), 0 other
//   blk     d: a block of the container parsed, for worker: x 1 put aside in dynbuff (its input
//           ring full), 0 into its ring. c: a block of worker's output written to the container
//   dyn     d: a block put aside handed to its worker
//   eof     d: the end of its input handed to worker
//   out     d: an output buffer of worker written
//   feed    c: an input buffer dealt to worker
//   round   d: the output moves on to worker; c: the dealing moves on to worker
//   end     the run's end (Quit()): bytes dynbuff's memory at most
//   cpu     dur: the main thread's CPU time during the run
// W<i>, worker i:
//   call      in its coroutine (coding): x what it returned (1 input wanted, 2 output buffer full,
//             4 a round (part) done, 0 done, 3 failed)
//   wait_in   waited for input (its input ring empty)
//   wait_out  waited for room for its output (its output ring full)
//   pop       an input buffer taken: x 1 its input's end
//   push      an output buffer handed over: x its flags (2 a round's last, 1 the end)
//   exit      its thread ends: x what its coroutine returned last, bytes the input it left
//   cpu       dur: the thread's CPU time

#ifdef _WIN32
static double mtdbg_tick_ns; // ns per QueryPerformanceCounter() tick

static void mtdbg_init( void ) {
  LARGE_INTEGER f;
  QueryPerformanceFrequency( &f );
  mtdbg_tick_ns = 1e9 / double(f.QuadPart);
}

static qword mtdbg_now( void ) { // ns
  LARGE_INTEGER c;
  QueryPerformanceCounter( &c );
  return qword( double(c.QuadPart)*mtdbg_tick_ns );
}

static qword mtdbg_cpu( void ) { // the calling thread's CPU time, ns
  FILETIME c, e, k, u;
  if( !GetThreadTimes( GetCurrentThread(), &c, &e, &k, &u ) ) return 0;
  return ( ((qword(k.dwHighDateTime)<<32) | k.dwLowDateTime) + ((qword(u.dwHighDateTime)<<32) | u.dwLowDateTime) )*100;
}
#else
static void mtdbg_init( void ) {}

static qword mtdbg_clock( clockid_t id ) {
  struct timespec t;
  clock_gettime( id, &t );
  return qword(t.tv_sec)*1000000000u + qword(t.tv_nsec);
}

static qword mtdbg_now( void ) { return mtdbg_clock( CLOCK_MONOTONIC ); }
static qword mtdbg_cpu( void ) { return mtdbg_clock( CLOCK_THREAD_CPUTIME_ID ); }
#endif

enum {
  MTDBG_PARAM, MTDBG_READ, MTDBG_WRITE, MTDBG_WAIT, MTDBG_BLK, MTDBG_DYN, MTDBG_EOF, MTDBG_OUT,
  MTDBG_FEED, MTDBG_ROUND, MTDBG_END, MTDBG_CALL, MTDBG_WAIT_IN, MTDBG_WAIT_OUT, MTDBG_POP,
  MTDBG_PUSH, MTDBG_EXIT, MTDBG_CPU, MTDBG_N
};
static const char* const mtdbg_name[MTDBG_N] = {
  "param", "read", "write", "wait", "blk", "dyn", "eof", "out",
  "feed", "round", "end", "call", "wait_in", "wait_out", "pop",
  "push", "exit", "cpu"
};

// the main loop's wait (MTDBG_WAIT's x)
enum { MTDBG_W_OTHER, MTDBG_W_RING, MTDBG_W_ENDED };

struct mtdbg_rec {
  qword t;          // ns, mtdbg_now()'s
  qword dur;        // ns
  qword n;          // bytes
  const char* note; // (a literal, or 0)
  uint ev;
  int w;
  uint x;
};

// a thread's events, in time order. Plain data (its owner never constructed): init() first
struct mtdbg_log {
  mtdbg_rec* r;
  size_t n, room;

  void init( void ) { r=0; n=room=0; }

  void quit( void ) { free(r); init(); }

  void add( uint ev, qword t, qword dur, int w, qword bytes, uint x, const char* note=0 ) {
    if( n==room ) {
      size_t m = room ? 2*room : 4096;
      mtdbg_rec* q = (mtdbg_rec*)realloc( r, m*sizeof(mtdbg_rec) );
      if( q==0 ) return; // (a statistic lost: the run goes on)
      r=q; room=m;
    }
    mtdbg_rec& e = r[n++];
    e.t=t; e.dur=dur; e.n=bytes; e.note=note; e.ev=ev; e.w=w; e.x=x;
  }

  // an instant, now
  void at( uint ev, int w, qword bytes, uint x ) { add( ev, mtdbg_now(), 0, w, bytes, x ); }

  // an interval, from t to now
  void since( uint ev, qword t, int w, qword bytes, uint x ) { add( ev, t, mtdbg_now()-t, w, bytes, x ); }
};

static uint mtdbg_runs; // the runs in dbg.txt so far

// a run's logs into dbg.txt, merged in time order: lg[0] the main loop's, lg[1+i] worker i's (of
// nl-1); t0 the run's start
static void mtdbg_dump( mtdbg_log* const* lg, uint nl, uint f_DEC, qword t0 ) {
  enum{ N_LOGS = 65 }; // the main loop's and 64 workers'
  size_t k[N_LOGS];
  uint i, b;
  FILE* f = fopen( "dbg.txt", mtdbg_runs ? "ab" : "wb" );
  if( f==0 ) return;
  if( nl>N_LOGS ) nl=N_LOGS;
  if( mtdbg_runs==0 ) fprintf( f, "run\tmode\tthread\tevent\tt_us\tdur_us\tworker\tbytes\tx\tnote\n" );
  for( i=0; i<nl; i++ ) k[i]=0;
  for(;;) {
    char th[8];
    for( b=nl, i=0; i<nl; i++ )
      if( k[i]<lg[i]->n && ( b==nl || lg[i]->r[k[i]].t<lg[b]->r[k[b]].t ) ) b=i;
    if( b==nl ) break;
    const mtdbg_rec& e = lg[b]->r[k[b]++];
    if( b==0 ) snprintf( th, sizeof th, "M" ); else snprintf( th, sizeof th, "W%u", b-1 );
    fprintf( f, "%u\t%c\t%s\t%s\t%.3f\t%.3f\t%d\t%.0f\t%u\t%s\n", mtdbg_runs, f_DEC ? 'd' : 'c', th,
             mtdbg_name[e.ev], e.t>=t0 ? double(e.t-t0)/1e3 : 0.0, double(e.dur)/1e3,
             b ? int(b-1) : e.w, double(e.n), e.x, e.note ? e.note : "" );
  }
  fclose( f );
  mtdbg_runs++;
}
