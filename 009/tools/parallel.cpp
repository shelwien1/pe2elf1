// tools/parallel.cpp -- two codecs, one process, opposite directions, at once.
//
//   g++ -std=c++17 -O2 -I. -o /tmp/par tools/parallel.cpp -lpthread && /tmp/par
//
// This is the check the class exists for.  `BMFCodec` was made so that two of
// them could run at the same time without touching each other's state, and that
// claim is worth nothing unless something runs it: for as long as the codec was
// file-scope statics, a second instance would have shared the stream, the range
// coder, the plane descriptors, the deadzone and the model blocks.
//
// **What it does.**  Four threads, each with its own `BMFCodec` on the heap
// (86 MB apiece, so they are `new`ed rather than stacked).  Two compress and
// two expand, interleaved, and every thread runs its image `kRounds` times.
// Then each thread's results are compared against what the same codec produced
// alone, single-threaded, before any thread started.
//
// **Byte-for-byte is the test.**  A shared-state bug in this program does not
// crash, it moves a stream -- one counter updated by the wrong plane, one
// descriptor read while another codec was writing it.  So the pass condition is
// that the parallel bytes equal the serial bytes exactly, which is the same
// condition `test.sh` uses for everything else.
//
// It is not in `test.sh`'s 111 checks: it needs `-lpthread` and it is about the
// shape of the program rather than about the format.  `tools/sweep.sh` does not
// read it either.  Run it by hand after anything that touches `BMFState`,
// `BMFCodec` or the block pools.
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

// One image read off disk into memory, which is all a codec needs.  The file
// I/O stays outside the class -- `read_bmp` is the program's, not the codec's --
// and what crosses into a thread is the pixels.
struct Sample {
  const char* path;
  BmfImage* img;
  size_t pal;   // the palette bytes that follow the pixels, if any
};

// One codec's worth of work: compress `img` `kRounds` times and answer the
// bytes of the last round.  A fresh `BMFImage` copy each round, because the
// coder writes the plane descriptors back into the header it was given.
static std::vector<uint8_t> compress_once(BMFCodec& cx, const Sample& s) {
  std::vector<uint8_t> out;
  for( int32_t r = 0; r<kRounds; ++r ) {
    // A fresh copy each round: the coder writes the plane descriptors back into
    // the header it is handed, so the second round would not start where the
    // first did.
    const size_t bytes = sizeof(BmfImage)+s.img->data_size+s.pal;
    std::unique_ptr<uint8_t[]> copy(new uint8_t[bytes]);
    memcpy(copy.get(), s.img, bytes);
    size_t len = 0;
    uint8_t* coded = cx.compress_to_memory((BmfImage*)copy.get(), &len);
    if( !coded ) {
      fprintf(stderr, "%s: compress refused it\n", s.path);
      out.clear();
      return out;
    }
    out.assign(coded, coded+len);
    free(coded);
  }
  return out;
}

// And the other direction: expand a coded stream `kRounds` times and answer the
// pixels of the last round.
static std::vector<uint8_t> expand_once(BMFCodec& cx, const std::vector<uint8_t>& coded) {
  std::vector<uint8_t> out;
  for( int32_t r = 0; r<kRounds; ++r ) {
    BmfImage* img = cx.expand_from_memory(coded.data(), coded.size());
    if( !img ) {
      fprintf(stderr, "expand refused a stream this program wrote\n");
      out.clear();
      return out;
    }
    out.assign(img->pixels, img->pixels+img->data_size);
    free(img);
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
  bmf_set_denormal_mode();
  const char* files[4] = { "testfiles/t24.bmp", "testfiles/t8g.bmp",
                           "testfiles/x_ai.bmp", "testfiles/med32.bmp" };
  if( argc>1 )
    files[0] = argv[1];

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

  // Eight threads, each with a codec of its own: four compressing, four
  // expanding, and the two directions interleaved so that a compressor and an
  // expander are always live at the same time.
  std::vector<uint8_t> got_coded[4], got_pixels[4];
  std::vector<std::thread> threads;
  for( int32_t k = 0; k<4; ++k ) {
    threads.emplace_back([&, k] {
      std::unique_ptr<BMFCodec> cx(new BMFCodec());
      got_coded[k] = compress_once(*cx, samples[k]);
    });
    threads.emplace_back([&, k] {
      std::unique_ptr<BMFCodec> cx(new BMFCodec());
      got_pixels[k] = expand_once(*cx, want_coded[k]);
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
  for( int32_t k = 0; k<4; ++k ) {
    if( got_coded[k]!=want_coded[k] ) {
      printf("%-22s compressed stream differs in parallel (%zu vs %zu bytes)\n",
             samples[k].path, got_coded[k].size(), want_coded[k].size());
      ++bad;
    }
    if( got_pixels[k]!=want_pixels[k] ) {
      printf("%-22s expanded pixels differ in parallel (%zu vs %zu bytes)\n",
             samples[k].path, got_pixels[k].size(), want_pixels[k].size());
      ++bad;
    }
  }
  if( bad ) {
    printf("FAILED: %d of 8 parallel results differ from the serial ones\n", bad);
    return 1;
  }
  printf("PASS: 8 codecs in 8 threads, %d rounds each, both directions, "
         "byte-identical to serial\n", kRounds);
  return 0;
}
