
//  RC_THREADS -- the encoder's model pass on a second thread.
//
//  The model pass and the coding sweep already talk through a buffer: that is
//  what pbit[] is, and RC_CHUNK made it small enough to keep in L1. So the two
//  are a producer and a consumer, and putting the producer on its own thread
//  overlaps them. The pass is a little over half of encode, so the ceiling is
//  about 1/0.57 = 1.75x -- and only for encode; the decoder cannot be split at
//  all, its model and its coder are interleaved per bit by construction.
//
//  Blocks stay strictly ordered. The model carries cty[] and inpptr across
//  them, and nothing here reorders anything: it overlaps chunk k+1's model
//  pass with chunk k's sweep INSIDE one block, which is exactly the dependency
//  the model/sweep split already respects.
//
//  Model<0> is a reinterpreted byte buffer (see misc/model_api.inc) and never
//  runs a constructor, so the worker and its control block live out here.

#if RC_THREADS

#include <atomic>
#include <thread>

struct t_thpipe {
  // prod and cons are monotonic chunk counts, never reset: the buffer a chunk
  // lands in is prod%RC_NBUF, and the ring is full when prod-cons==RC_NBUF.
  std::atomic<uint>  prod, cons, job, quit;
  void*  obj;
  uint   blk;
  uint   done;      // worker-private: the last job it ran
  uint   mine;      // main-private: the chunk it is waiting for
};

// C++17 inline variables: one instance across the model TUs, and no
// constructor to run inside Model<0>, which is a reinterpreted byte buffer.
inline t_thpipe    g_th{};
inline std::thread g_thread;

// Spin, then hand the core back. A chunk is RC_CHUNK input bytes, tens of
// microseconds of work, so a handoff that takes a few hundred cycles is noise
// -- but a spin that never yields is not, on a machine with other work.
INLINE void th_spin( uint& n ) {
  if( ++n < 128 ) _mm_pause(); else { n = 0; std::this_thread::yield(); }
}

#endif
