// BMF lossless image compressor, v.2.01 (C) 1998-1999, 2009 by Dmitry Shkarin.
// A decompilation of BMF.exe; see tools/README.md for how it is kept honest.
//
// This file is the include list and the entry points.  Everything else is one
// unit per file below: a class with its methods defined in its own body, or a
// function large enough to be read on its own.
//
// The order is a reading order, not a dependency order -- `decls.inc` declares
// every function before any of them is defined, so a unit sits with what it
// belongs to rather than with what happens to call it.  What the order does
// still say is that the four files before `decls.inc` are the ones everything
// is written in terms of: the decompiler's macros, the allocator, the constant
// tables and the program's state.
//
// The *names* group where that order does not.  Sorted, the directory reads as
// nine runs, each one subsystem: `alt_p1_*` and `alt_p2_*` are the two
// alternate models -- each with its block, its parts and its coding bodies --
// then `bmp_*` the file format, `filter_*` the filter search, `image_*` one
// image each way, `model_*` the main model, `plane_*` one plane, `rc_*` the
// range coder, and `sym_*` the alphabet.  The noun comes first and the verb
// second, so `read_bmp.inc` is `bmp_read.inc` and its partner is beside it
// rather than fifteen entries away.
//
// The eight files that join no run -- `bitctr`, `ctxidx`, `decls`, `file`,
// `globals`, `ida`, `memory`, `tables` -- have no run to join.  A prefix
// invented for one file groups nothing, and renaming to a scheme rather than
// to a neighbour is how a naming convention stops meaning anything.
//
// ## the encode/decode pairs
//
// Fourteen of them are one `template<int32_t f_DEC>` each, instantiated as the
// two names their callers use.  What decided which: how many lines the two
// bodies actually share, measured as a longest common subsequence over the
// pair.
//
//   rc_end                       5 + 5     CounterNode::code_symbol      53 + 49
//   alt_model_p2_d8             11 + 11     code_symbol_tree              85 + 86
//   BitCtr::code_context_bit    52 + 50     AltP2Block::alt_p2_d8_body   171 + 147
//   rc_begin                    91 + 89     alt_model_p1                 263 + 212
//   P2Freq::code_three_way      49 + 55     alt_model_p2                  3 + 3
//   AltP1Block::d8_body         30 + 26     code_plane                    1 + 1
//   code_colour_plane           40 + 43     ModelBlock::code_run_length   26 + 24
//
// The last of those is inside the one pair that stays two functions.  A
// declined pair is a decision about two *bodies*, not a bar on naming what
// they share: `run_scan` came out of these two an earlier round and this is
// the walk beside it, one bit per bucket level with the bit read on one side
// and taken from the length on the other.  The pair is 13 shared lines of 211
// now, from 27 of 277 at the start of the round -- the share falls because
// what they shared keeps becoming something with a name.
//
// `code_colour_plane` was never on the declined table, which is the point of
// it: `interleave_plane` and `colour_transform` are an encode/decode pair that
// nothing had *named* as one, so no share was ever measured and no decision
// was ever taken.  A comment above them said the nineteen lines that read the
// descriptor "are the whole of what the two share -- the loops under them are
// inverses and have nothing else in common", and that was a sentence about
// spelling: the two blends were written four different ways between them, one
// casting to `uint32_t` mid-expression, one summing into an `int32_t` and
// casting the total, two splitting the sum across an extra local.  All four
// are the same thirty-two bits.  Written unsigned throughout, each loop is a
// blend and a sign.
//
// The last three came off the declined table below, and two of them for a
// reason worth writing down: **a small share can mean the two bodies differ,
// or it can mean one of them is not where the tool is looking, and the
// measurement cannot tell those apart.**
//
// `alt_model_p2` was the first line of that table at 34 shared of 153, and
// there the share really was about the bodies -- it was just wrong about what
// it meant.  Outside the four coding calls the two are the same body; what
// separated them was eight spellings (a bias loop counting up in one and down
// in the other over four slots it sets to zero, a `for` against a `do`/`while`,
// two register spills of the row index) plus one deliberate near-miss the
// merge keeps as the only `f_DEC` in it that is not about direction.
//
// `AltP1Block::d8_body` was the *smallest* line of the table at 3 of 34, and
// that number was not about the coders at all.  The encode half was a method
// and the decode half was written out inside `alt_model_p1_d8_decode`, so what
// the tool compared was a four-line wrapper against a twenty-six-line one and
// the twenty-two lines they really share were on the wrong side of a call.
//
// `code_plane` left in two steps.  First the sixteen-line dispatch over the
// descriptor's model and depth came out as `alt_model_plane<f_DEC>`, taking
// the pair from 14 shared lines of 170 to 4 of 146.  What was left read as
// "the main model, which the two do differently" -- and it was not: the
// decoder's main model was a call to `unmodel_plane_slow` and the encoder's
// was 130 lines of `blk->` written out in `model_plane`.  Those two bodies
// really are different and stay two methods; naming the encoder's the way the
// decoder's was already named left two five-line wrappers with one line
// between them.
//
// Two pairs are measured and declined, and after the three above came off this
// table the reason for each is written out rather than left as a share.
// `tools/pairshare.py` re-measures both and reports any line that has drifted.
//
//   code_pixel / decode_pixel                         13 of 211 (6%)
//   predict_med / unpredict_med                        7 of 89 (8%)
//
// **`predict_med` walks backwards and `unpredict_med` forwards**, and that is
// structural rather than a spelling.  The forward transform has to read each
// pixel's *original* north and west, so it starts at the bottom-right corner
// and steps back; the inverse has to read the *reconstructed* ones, so it
// starts at the top-left and steps forward.  The first row and column follow
// the walk -- taken last by one and first by the other -- and that is most of
// what the two do outside the shared middle.  A template over a direction flag
// would have to reverse three loop headers and two edge cases, which is
// writing both bodies out again with an `if` around each line.
//
// `unpredict_med` got nine lines shorter anyway, and not by sharing anything
// with its partner.  Its first pixel of a row sat inside a `while( 1 )` that
// broke out of itself for a wide image and looped for a narrow one -- one loop
// doing two jobs, with the narrow case's `return` three levels in.  The narrow
// case is its own loop now, and it is worth knowing that **no gate reaches
// it**: `search_filter` gives up below `4 x 3` and leaves every descriptor at
// mode 0, so the encoder never picks MED for an image one pixel wide, and only
// a stream that claims one can get there.  `tools/narrow.sh` says so where a
// reader will look for it.
//
// **`code_pixel` and `decode_pixel` diverge at the top and meet at the
// bottom.**  The encoder knows the symbol and asks where it ranks; the decoder
// asks the coder for a target and finds which rank holds it.  That is
// `Neighbours::rank` against `FreqRec::find_level`, and it decides the shape
// of everything above it -- the encoder can test candidates in rank order and
// stop, the decoder cannot.  What they genuinely share is the context, and
// that has been coming out by name for several rounds: `load_neighbours`,
// `FreqRec::blend_from`, `ModelBlock::start_row`, `find_level`, `bump`,
// `cum_below`, `code_against`/`decode_against`, and now the whole opening as
// `open_pixel` -- neighbours, match context, pair key, context state, symbol
// pair and context id, twenty-eight lines that were written twice -- and
// `seed_after_run`, the four gradient seeds and two match flags a coded run
// leaves behind, which each body had wrapped in a spill of an unrelated local,
// and `fill_run`, the records the run covers -- one a turn in the decoder and
// unrolled by two in the encoder, whose second copy stored through a second
// name for a cursor the first had not moved.
//
// Both have moved since they were first measured, and the same way: what the
// two halves shared was never the algorithm, so naming it and calling it from
// both leaves less behind, not more.  The two `*_pixel` bodies went from 1044
// lines to 277, and the share with them -- 179 lines to 27.  The percentage
// barely moves because both halves shrink together, which is the point: the
// shared part is scaffolding, and scaffolding is what comes out.
//
// What kept the opening from being seen as one thing for nine rounds was four
// values in two arbitrary sets of stack slots -- `nb_sym[8]`, `[10]`, `[6]`,
// `[5]` in one body and `[0]`, `[1]`, `[3]`, `[2]` in the other.  Nothing was
// wrong with either; they simply could not be compared.  Naming them
// `Neighbours` made twenty-eight lines identical that had always been the same
// lines.
//
// Every merge was gated on its own -- the seventeen streams byte for byte, at
// both pointer widths -- and then instrumented and run over the corpus to
// check that both instantiations are actually entered.  A template whose
// decode half never runs is a gate passing about nothing.

#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pmmintrin.h>
#include <xmmintrin.h>

#include "ida.inc"
#include "ctxidx.inc"
#include "memory.inc"
#include "tables.inc"
#include "globals.inc"
#include "rc.inc"
#include "decls.inc"
#include "sym_list.inc"
#include "bitctr.inc"
#include "alt_p1_block.inc"
#include "alt_p2_block.inc"
#include "model.inc"
#include "bmp.inc"
#include "file.inc"
#include "rc_io.inc"
#include "sym_code.inc"
#include "model_workspace.inc"
#include "sym_reduce.inc"
#include "alt_p1.inc"
#include "alt_p1_code.inc"
#include "alt_p2.inc"
#include "alt_p2_context.inc"
#include "alt_p2_model.inc"
#include "alt_p2_code.inc"
#include "plane_predict.inc"
#include "plane.inc"
#include "filter_cost.inc"
#include "plane_choose.inc"
#include "filter_search.inc"
#include "bmp_read.inc"
#include "bmp_write.inc"
#include "image_expand.inc"
#include "image_compress.inc"

void bmf_compress(const char* InName, const char* OutName) {
  int32_t i;
  BmfImage *p_i = read_bmp((char*)InName);
  if( !p_i )
    bmf_fatal(bmf_read_error);
  printf("File %16s, image %dx%dx%d, size - %d:", InName, p_i->width, p_i->height, p_i->depth&depth_bits, p_i->data_size);
  // Eight bytes, one per run.  It was a `bmf_new` whose result was handed
  // straight to `bmf_open_file` and never freed.
  static BmfFile arc_store;
  BmfFile* Arc = bmf_open_file(&arc_store, (char*)OutName, 0);
  // `Flags`, in a program whose header has a `flags` byte that is not this
  // one: it is the depth.
  int32_t Depth = p_i->depth;
  if( Depth&depth_palette ) {
    if( Depth&depth_grey ) {
      p_i->depth = Depth^depth_palette;
    } else {
      int32_t Colours = 1<<(Depth&depth_bits);
      int32_t Step = 0x100u>>(Depth&depth_bits);
      const uint8_t* Palette = p_i->palette();
      int32_t Grey = 0;
      for( i = 0; i<Colours; ++i ) {
        if( Palette[3*i]!=Grey||Palette[3*i+1]!=Grey||Palette[3*i+2]!=Grey )
          break;
        Grey += Step;
      }
      if( i>=Colours )
        p_i->depth = (Depth|depth_grey)^depth_palette;
    }
  }
  int32_t coded_len = compress_image(Arc, p_i, coded_block);
  if( !coded_len )
    bmf_fatal(bmf_write_error, OutName);
  printf("%6.3f bpp\n", (double)coded_len*8.0/(double)(p_i->height*p_i->width));
  free(p_i);
}

void bmf_decompress(const char* InName, const char* OutName) {
  BmfFile* arc;
  if( void* nb = bmf_new(sizeof(BmfFile)) )
    arc = bmf_open_file((BmfFile*)nb, (char*)InName, 1);
  else
    arc = nullptr;
  BmfImage* p_i = expand_image(arc, &coded_block);
  // One image in a file: nothing to parse is not the end of a list of members,
  // it is a file that is not one of ours.
  if( !p_i )
    bmf_fatal(bmf_bad_file, InName);
  printf("File %16s, image %dx%dx%d, size - %d\n", InName, p_i->width, p_i->height, p_i->depth&depth_bits, p_i->data_size);
  int32_t Depth = p_i->depth&depth_bits;
  if( Depth==2||Depth==15||Depth==16 ) {
    printf("%s: %d bits per pixel is not a BMP depth\n", OutName, Depth);
    exit(5);
  }
  if( !write_bmp(p_i, (char*)OutName, 1) )
    bmf_fatal(bmf_write_error, OutName);
  free(coded_block);
  coded_block = nullptr;
  free(p_i);
  bmf_close_file(arc);
}


// `argv` is `char**` because that is main's signature and `const char**` inside
// because that is what everything downstream takes.  The donor had two bodies
// here -- one of them four lines that cast and forwarded -- and the cast is all
// that was ever between them.
int32_t main(int32_t argc, char** argv) {
  const char*const* args = (const char*const*)argv;
  bmf_set_denormal_mode();
  printf("BMF lossless image compressor, v.2.01 (C) 1998-1999, 2009 by Dmitry Shkarin\n");
  int32_t mode = argc==4&&!args[1][1] ? toupper(args[1][0]) : 0;
  if( mode!='C'&&mode!='D' ) {
    printf("e-mail: <dmitry.shkarin@mtu-net.ru>;  web: http://compression.graphicon.ru/ds/\n"
           "Usage: bmf c input.bmp output     compress, always with -S -Q9\n"
           "       bmf d input output.bmp     expand\n");
    return 1;
  }
  if( mode=='C' )
    bmf_compress(args[2], args[3]);
  else
    bmf_decompress(args[2], args[3]);
  return 0;
}
