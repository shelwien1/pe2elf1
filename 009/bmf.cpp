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
// Nine of them are one `template<int32_t f_DEC>` each, instantiated as the two
// names their callers use.  What decided which: how many lines the two bodies
// actually share, measured as a longest common subsequence over the pair.
//
//   rc_end                       5 + 5     CounterNode::code_symbol      53 + 49
//   alt_model_p2_d8             11 + 11     code_symbol_tree              85 + 86
//   BitCtr::code_context_bit    52 + 50     AltP2Block::alt_p2_d8_body   171 + 147
//   rc_begin                    91 + 89     alt_model_p1                 263 + 212
//   P2Freq::code_three_way      49 + 55
//
// Five pairs were measured and declined.  What they share is not the body --
// it is the declaration block and the loop scaffolding -- and folding them
// would put two unrelated algorithms behind one `if`.  `tools/pairshare.py`
// re-measures this table and reports any line of it that has drifted; the
// numbers below were all wrong by the time it was written, which is why it
// exists.
//
//   alt_model_p2_encode / alt_model_p2_decode         25 of 214 (12%)
//   code_pixel / decode_pixel                        108 of 724 (15%)
//   predict_med / unpredict_med                       10 of 122 (8%)
//   alt_model_p1_d8_encode / alt_model_p1_d8_decode     3 of 34 (9%)
//   model_plane / unmodel_plane                       15 of 198 (8%)
//
// Every one of the five has moved since it was measured, and all of them the
// same way: what the two halves shared was never the algorithm, so naming it
// and calling it from both leaves less behind, not more.  The p2 pair fell
// from 123 shared lines of 586 to 25 of 215 and the p1 pair from 8 of 60 to 3
// of 34.  The two `*_pixel` bodies have lost 40% of their length between them
// -- `load_neighbours`, `FreqRec::blend_from`, `ModelBlock::start_row`,
// `find_level`, `bump` -- and went from 1044 lines to 724.  The percentages
// barely move because both halves shrink together, which is the point: the
// shared part is scaffolding, and scaffolding is what comes out.
//
// Every merge was gated on its own -- the fifteen streams byte for byte, at
// both pointer widths -- and then all nine were instrumented and run over the
// corpus to check that both instantiations are actually entered.  A template
// whose decode half never runs is a gate passing about nothing.

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
#include "sym_list_decode.inc"
#include "model_workspace.inc"
#include "sym_reduce.inc"
#include "alt_p1.inc"
#include "alt_p1_code.inc"
#include "alt_p2.inc"
#include "alt_p2_context.inc"
#include "alt_p2_model.inc"
#include "alt_p2_encode.inc"
#include "alt_p2_decode.inc"
#include "plane_predict.inc"
#include "plane.inc"
#include "model_plane.inc"
#include "filter_cost.inc"
#include "plane_choose.inc"
#include "filter_search.inc"
#include "bmp_read.inc"
#include "bmp_write.inc"
#include "image_expand.inc"
#include "image_compress.inc"

void bmf_compress(const char* InName, const char* OutName) {
  int32_t i;
  BmfFile* Arc;
  FILE* fp = fopen(InName, "rb");
  if( !fp )
    bmf_fatal(bmf_no_open, InName);
  fclose(fp);
  BmfImage *p_i = read_bmp((char*)InName);
  if( !p_i )
    bmf_fatal(bmf_read_error);
  printf("File %16s, image %dx%dx%d, size - %d:", InName, p_i->width, p_i->height, p_i->depth&depth_bits, p_i->data_size);
  if( void* nb = bmf_new(sizeof(BmfFile)) )
    Arc = bmf_open_file((BmfFile*)nb, (char*)OutName, 0);
  else
    Arc = nullptr;
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
  bmf_close_file(arc, 1);
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
