// tools/parallel.cpp -- eight codecs, one process, random directions, at once.
//
//   g++ -std=c++17 -O2 -I. -o /tmp/par tools/parallel.cpp -lpthread && /tmp/par
//   /tmp/par testfiles/other.bmp 12345      # another image, another seed
//
// This is the check the class exists for.  `BMFCodec` was made so that several
// of them could run at the same time without touching each other's state, and
// that claim is worth nothing unless something runs it: for as long as the
// codec was file-scope statics, a second instance would have shared the stream,
// the range coder, the plane descriptors, the deadzone and the model blocks.
//
// **What it does.**  Eight threads, each with its own `BMFCodec` on the heap
// (86 MB apiece, so they are `new`ed rather than stacked).  Each thread takes
// `kSteps` steps, and at every step its own generator picks one of the four
// images and one of the two directions -- so a codec is asked to compress after
// it expanded and the other way round, which a fixed job per thread never does.
// Every result is compared against what one codec produced alone, before any
// thread started.
//
// **Byte-for-byte is the test.**  A shared-state bug in this program does not
// crash, it moves a stream -- one counter updated by the wrong plane, one
// descriptor read while another codec was writing it.  So the pass condition is
// that the parallel bytes equal the serial bytes exactly, which is the same
// condition `test.sh` uses for everything else.
//
// The seed is fixed by default and printed on every line this writes, so a
// green run means the same thing twice and a red one can be repeated.
//
// It is not in `test.sh`'s 111 checks: it needs `-lpthread` and it is about the
// shape of the program rather than about the format.  `tools/sweep.sh` does not
// read it either.  Run it by hand after anything that touches `BMFState`,
// `BMFCodec` or the block pools -- and under `-fsanitize=thread`, which is what
// named the p2 tables when the byte comparison could only say something was
// wrong.
#define main bmf_main_unused
#include "bmf.cpp"
#undef main

#include <thread>
#include <vector>
#include <cstdio>
#include <cstring>
#include <memory>

#ifndef kRounds
static const int32_t kRounds = 3;
#endif
#ifndef kThreads
static const int32_t kThreads = 8;
#endif
#ifndef kSteps
static const int32_t kSteps = 6;
#endif
// A fixed default so a green run means the same thing twice, and an override so
// a red one can be repeated.  It is printed either way.
#ifndef kSeed
static const uint32_t kSeed = 20260821u;
#endif

// Numerical Recipes' LCG, per thread and by value: `rand` is one generator for
// the process and would be the shared state this program exists to look for.
static uint32_t next(uint32_t& s) {
  s = s*1664525u+1013904223u;
  return s>>16;                 // the high half; the low bits of an LCG cycle
}

// One image read off disk into memory, which is all a codec needs.  The file
// I/O stays outside the class -- `read_bmp` is the program's, not the codec's --
// and what crosses into a thread is the pixels.
struct Sample {
  const char* path;
  BmfImage* img;
  size_t pal;   // the palette bytes that follow the pixels, if any
};

// One image compressed once.  A fresh `BmfImage` copy every time, because the
// coder writes the plane descriptors back into the header it is handed, so a
// second call on the same buffer would not start where the first did.
static std::vector<uint8_t> compress_one(BMFCodec& cx, const Sample& s) {
  std::vector<uint8_t> out;
  const size_t bytes = sizeof(BmfImage)+s.img->data_size+s.pal;
  std::unique_ptr<uint8_t[]> copy(new uint8_t[bytes]);
  memcpy(copy.get(), s.img, bytes);
  size_t len = 0;
  uint8_t* coded = cx.compress_to_memory((BmfImage*)copy.get(), &len);
  if( !coded ) {
    fprintf(stderr, "%s: compress refused it\n", s.path);
    return out;
  }
  out.assign(coded, coded+len);
  free(coded);
  return out;
}

// And the other direction, once.
static std::vector<uint8_t> expand_one(BMFCodec& cx, const std::vector<uint8_t>& coded) {
  std::vector<uint8_t> out;
  BmfImage* img = cx.expand_from_memory(coded.data(), coded.size());
  if( !img ) {
    fprintf(stderr, "expand refused a stream this program wrote\n");
    return out;
  }
  out.assign(img->pixels, img->pixels+img->data_size);
  free(img);
  return out;
}

// `kRounds` of the same thing on one codec, which is what takes the reference.
// The repetition is the point rather than the average: a codec that has coded
// once is not in the state a new one is, and running the reference three times
// is what caught `reset()` being missing.
static std::vector<uint8_t> compress_once(BMFCodec& cx, const Sample& s) {
  std::vector<uint8_t> out;
  for( int32_t r = 0; r<kRounds; ++r ) {
    out = compress_one(cx, s);
    if( out.empty() )
      return out;
  }
  return out;
}

static std::vector<uint8_t> expand_once(BMFCodec& cx, const std::vector<uint8_t>& coded) {
  std::vector<uint8_t> out;
  for( int32_t r = 0; r<kRounds; ++r ) {
    out = expand_one(cx, coded);
    if( out.empty() )
      return out;
  }
  return out;
}

// **A codec that refused a stream is still a codec**, and that is only worth
// checking because the class made it so: before `expand_from_memory` existed, a
// stream this program would not take ended the process, so whatever the refusal
// path had not freed did not matter.  Now the same object is handed the next
// one.
//
// Truncation is the refusal this can use, and it is worth writing down why it
// is the only one.  Corrupting the payload instead was the first attempt, on
// the theory that a stream of the right length that decodes to the wrong number
// of bytes reaches furthest in.  It does -- and it never comes back: the
// arithmetic decoder calls `bmf_fatal(bmf_read_error)` on a stream that does
// not make sense, which is `noreturn` and takes the process with it.  That is
// the 1997 program's design and not something this round changes, but it does
// bound what an in-memory codec can promise: **`expand_from_memory` returns
// `nullptr` for a stream it can reject by inspection, and exits for one that
// only falls apart once the decoder is inside it.**
//
// So what this checks is the reachable half: a header whose `data_size` runs
// past the end of the buffer is refused by `expand_image` before anything is
// allocated, sixteen times over on one codec, and the codec still expands a
// good stream afterwards.
static bool refusals_are_clean(const std::vector<uint8_t>& coded) {
  std::unique_ptr<BMFCodec> cx(new BMFCodec());
  int32_t refused = 0;
  for( int32_t r = 0; r<16; ++r ) {
    BmfImage* img = cx->expand_from_memory(coded.data(), coded.size()/2);
    if( img )
      free(img);
    else
      ++refused;
  }
  // A probe that never provoked the path it is about proves nothing, so it says
  // so rather than passing quietly.
  if( refused!=16 ) {
    fprintf(stderr, "%d of 16 truncated streams were refused -- expected all\n",
            refused);
    return false;
  }
  // And the codec still works afterwards, which is the other half of the claim.
  BmfImage* img = cx->expand_from_memory(coded.data(), coded.size());
  if( !img ) {
    fprintf(stderr, "a codec that refused a stream would not take a good one\n");
    return false;
  }
  free(img);
  return true;
}

int main(int argc, char** argv) {
  // **No `bmf_set_denormal_mode()` here, deliberately.**  `main` in bmf.cpp
  // calls it and this program is not that main; the codec sets it in `reset()`,
  // on whatever thread is about to run, because MXCSR is per thread and a
  // worker starts at the default.  Leaving it out here is what makes this
  // program check that.
  const char* files[4] = { "testfiles/t24.bmp", "testfiles/t8g.bmp",
                           "testfiles/x_ai.bmp", "testfiles/med32.bmp" };
  if( argc>1 )
    files[0] = argv[1];
  const uint32_t seed = argc>2 ? (uint32_t)strtoul(argv[2], nullptr, 0) : kSeed;

  Sample samples[4];
  for( int32_t k = 0; k<4; ++k ) {
    samples[k].path = files[k];
    samples[k].img = read_bmp((char*)files[k]);
    samples[k].pal = samples[k].img && (samples[k].img->depth&depth_palette)
                   ? (size_t)3u<<(samples[k].img->depth&depth_bits) : 0;
    if( !samples[k].img ) {
      fprintf(stderr, "cannot read %s -- run this from the 009 directory\n", files[k]);
      return 2;
    }
  }

  // **The reference, taken one codec at a time.**  Everything below has to
  // match this; a shared-state bug shows up as a difference and not as a crash.
  std::vector<uint8_t> want_coded[4], want_pixels[4];
  {
    std::unique_ptr<BMFCodec> cx(new BMFCodec());
    for( int32_t k = 0; k<4; ++k ) {
      want_coded[k] = compress_once(*cx, samples[k]);
      if( want_coded[k].empty() )
        return 3;
      want_pixels[k] = expand_once(*cx, want_coded[k]);
      if( want_pixels[k].empty() )
        return 3;
    }
  }

  if( !refusals_are_clean(want_coded[0]) )
    return 4;

  // **Eight codecs, and each one picks its own direction every step.**  The
  // first version of this gave every thread a fixed job -- four compressing,
  // four expanding -- which puts both directions in the process at once but
  // never puts both through *one* codec, so the state a compress leaves behind
  // was never what an expand started from.  A thread here does whichever
  // direction its own generator names, on whichever of the four images it
  // names, for `kSteps` steps.
  //
  // Seeded and reported, so a failure is reproducible: the generator is an LCG
  // per thread, its seed is `kSeed + thread`, and `argv[2]` overrides `kSeed`.
  // Nothing here is shared and written: `samples` and the two `want_` arrays
  // are read-only once the threads start, and each thread touches only its own
  // slot of `bad_at`.
  int32_t bad_at[kThreads] = {};
  std::vector<std::thread> threads;
  for( int32_t t = 0; t<kThreads; ++t ) {
    threads.emplace_back([&, t] {
      std::unique_ptr<BMFCodec> cx(new BMFCodec());
      uint32_t s = seed+(uint32_t)t;
      for( int32_t step = 0; step<kSteps; ++step ) {
        const int32_t k = (int32_t)(next(s)%4u);
        if( next(s)&1u ) {
          if( compress_one(*cx, samples[k])!=want_coded[k] )
            ++bad_at[t];
        } else {
          if( expand_one(*cx, want_coded[k])!=want_pixels[k] )
            ++bad_at[t];
        }
      }
    });
  }
  for( std::thread& t : threads )
    t.join();

  // The samples were read before any codec existed and nothing else owns them.
  // Freed here rather than left to exit, because this program is run under
  // LeakSanitizer -- four unfreed images is 5 MB of report standing between the
  // reader and a leak the codec actually had.
  for( int32_t k = 0; k<4; ++k )
    free(samples[k].img);

  int32_t bad = 0;
  for( int32_t t = 0; t<kThreads; ++t ) {
    if( !bad_at[t] )
      continue;
    printf("thread %d: %d of %d results differ from the serial ones\n",
           t, bad_at[t], kSteps);
    bad += bad_at[t];
  }
  if( bad ) {
    printf("FAILED: %d of %d parallel results differ, at seed %u\n",
           bad, kThreads*kSteps, seed);
    return 1;
  }
  printf("PASS: %d codecs in %d threads, %d steps each in directions chosen at "
         "random, byte-identical to serial (seed %u)\n",
         kThreads, kThreads, kSteps, seed);
  return 0;
}
