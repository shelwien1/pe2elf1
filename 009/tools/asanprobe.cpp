// tools/asanprobe.cpp -- what AddressSanitizer can and cannot see in this tree.
//
// Built and run by `tools/asan.sh --selftest`, and by a full `tools/asan.sh`
// before it looks at a single image.  It is not built by `build.sh` and nothing
// links it into the program.
//
// **Why a program that deliberately writes out of bounds is checked in.**
// `asan.sh` answers "no AddressSanitizer report in 44 runs over 19 images", and
// that sentence is only worth what the sanitizer can still see.  It got quietly
// cheaper once: the three model blocks used to come from `bmf_page_alloc`, so a
// write past the end of one landed in a `malloc` redzone and was reported; when
// they became slots of a static array the same write became a legal store to
// the live object next door, and forty-four clean runs went on saying the same
// words about less ground.  Nobody would have noticed from the output.
//
// So the three cases below are run every time, and each one is a claim `asan.sh`
// makes:
//
//   A  one byte past a *taken* pool slot, with free slots beside it.  MUST
//      report -- this is what `BlockPool`'s poisoning of unheld slots buys, and
//      it is the case an overflow in a coder would actually hit, because at
//      most four planes are ever live.
//   B  one byte past a `malloc`.  MUST report.  The control: without it, a
//      build where the sanitizer was silently not linked would pass case C and
//      look like a clean result rather than an absent one.
//   C  one byte past the only slot of a one-slot pool.  MUST NOT report -- there
//      is no neighbour to poison, so this is a known gap, and it is asserted so
//      that it stays a *known* one.  If this ever starts reporting, the gap has
//      closed and the note in `asan.sh` should say so.
//
// C is the odd one and worth being plain about: a test that requires silence is
// normally a bad test.  It earns its place here because the thing being measured
// is the instrument, not the program -- the point is that the reach of the gate
// is written down in something that runs, in both directions, rather than in a
// comment that ages.
#define main bmf_main_unused
#include "bmf.cpp"
#undef main

#include <cstdio>
#include <cstdlib>

// File scope on purpose: a `BMFCodec` is 86 MB and will not sit on a stack.
static BMFCodec cx;

int main(int argc, char** argv) {
  const char which = argc>1 ? argv[1][0] : 'A';
  if( which=='A' ) {
    // `take(false)` and not `take(true)`: the zeroing would be the first thing
    // to touch the slot, and what is being probed is the byte *after* it.
    AltP1Block* blk = cx.p1_blocks.take(false);
    volatile uint8_t* p = (volatile uint8_t*)blk;
    fprintf(stderr, "A: 1 byte past a taken %zu-byte p1 slot, 3 free beside it\n",
            sizeof(AltP1Block));
    p[sizeof(AltP1Block)] = 0x5A;
    fprintf(stderr, "A: no report\n");
  } else if( which=='B' ) {
    volatile uint8_t* q = (volatile uint8_t*)malloc(64);
    fprintf(stderr, "B: 1 byte past a 64-byte malloc\n");
    q[64] = 0x5A;
    fprintf(stderr, "B: no report\n");
  } else {
    ModelBlock* blk = cx.model_blocks.take(true);
    volatile uint8_t* p = (volatile uint8_t*)blk;
    fprintf(stderr, "C: 1 byte past the only slot of a 1-slot pool\n");
    p[sizeof(ModelBlock)] = 0x5A;
    fprintf(stderr, "C: no report\n");
  }
  return 0;
}
