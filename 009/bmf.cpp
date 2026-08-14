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
// ## the encode/decode pairs
//
// Nine of them are one `template<int f_DEC>` each, instantiated as the two
// names their callers use.  What decided which: how many lines the two bodies
// actually share, measured as a longest common subsequence over the pair.
//
//   __rc_end                    5 + 5      __alt_p1_code_symbol   53 + 49
//   __alt_model_p2_d8          11 + 11     __code_symbol_tree     85 + 86
//   BitCtr::code_context_bit   52 + 50     __alt_p2_d8_body      171 + 147
//   __rc_begin                 91 + 89     __alt_model_p1        263 + 212
//   P2Freq::code_symbol        49 + 55
//
// Five pairs were measured and declined.  What they share is not the body --
// it is the declaration block and the loop scaffolding -- and folding them
// would put two unrelated algorithms behind one `if`:
//
//   __alt_model_p2_encode/decode     130 of 646 lines shared (20%)
//   code_pixel/decode_pixel          179 of 1229 (14%)
//   __predict_med/__unpredict_med     24 of 152 (15%)
//   __alt_model_p1_d8_encode/decode    6 of 98 (6%)
//   __model_plane/__unmodel_plane     10 of 271 (3%)
//
// Every merge was gated on its own -- the fifteen streams byte for byte, at
// both pointer widths -- and then all nine were instrumented and run over the
// corpus to check that both instantiations are actually entered.  A template
// whose decode half never runs is a gate passing about nothing.

#include <cctype>
#include <cmath>
#include <cstdarg>
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
#include "symlist.inc"
#include "bitctr.inc"
#include "altp1.inc"
#include "altp2.inc"
#include "model.inc"
#include "bmp.inc"
#include "arc.inc"
#include "rcio.inc"
#include "symcode.inc"
#include "decode_symbol_list.inc"
#include "workspace.inc"
#include "reduce_alphabet.inc"
#include "p1.inc"
#include "alt_model_p1.inc"
#include "p2.inc"
#include "alt_p2_context.inc"
#include "alt_p2_model.inc"
#include "alt_model_p2_encode.inc"
#include "alt_model_p2_decode.inc"
#include "predict.inc"
#include "plane.inc"
#include "model_plane.inc"
#include "cost_candidate.inc"
#include "choose_plane_coding.inc"
#include "search_filter.inc"
#include "read_bmp.inc"
#include "write_bmp.inc"
#include "expand_image.inc"
#include "compress_image.inc"

void __bmf_compress(const char* InName, const char* OutName) {
  int32_t i;
  BmfArc* Arc;
  FILE* fp = fopen(InName, "rb");
  if( !fp )
    __exit_402E40(6, InName);
  fclose(fp);
  int32_t *p_i = __read_bmp((char*)InName);
  if( !p_i )
    __exit_402E40(4);
  BmfImage*const p_i_img = (BmfImage*)p_i;
  printf("File %16s, image %dx%dx%d, size - %d:", InName, p_i_img->width, p_i_img->height, p_i_img->depth&0x3F, p_i_img->data_size);
  if( void* __nb = bmf_new(sizeof(BmfArc)) )
    Arc = __bmf_open_archive((BmfArc*)__nb, (char*)OutName, 0);
  else
    Arc = nullptr;
  int32_t Flags = p_i_img->depth;
  if( Flags&0x80 ) {
    if( Flags&0x40 ) {
      p_i_img->depth = Flags^0x80;
    } else {
      int32_t Colours = 1<<(Flags&31);
      int32_t Step = 0x100u>>(Flags&31);
      const uint8_t* Palette = (const uint8_t*)p_i+p_i_img->data_size+16;
      int32_t Grey = 0;
      for( i = 0; i<Colours; ++i ) {
        if( Palette[3*i]!=Grey||Palette[3*i+1]!=Grey||Palette[3*i+2]!=Grey )
          break;
        Grey += Step;
      }
      if( i>=Colours )
        p_i_img->depth = (Flags|0x40)^0x80;
    }
  }
  int32_t coded_len = __compress_image(Arc, (BmfImage*)p_i, (void*)coded_block);
  if( !coded_len )
    __exit_402E40(5, OutName);
  printf("%6.3f bpp\n", (double)coded_len*8.0/(double)(p_i_img->height*p_i_img->width));
  free(p_i);
}

void __bmf_decompress(const char* InName, const char* OutName) {
  BmfArc* arc;
  if( void* __nb = bmf_new(sizeof(BmfArc)) )
    arc = __bmf_open_archive((BmfArc*)__nb, (char*)InName, 1);
  else
    arc = nullptr;
  uint32_t* p_i = (uint32_t*)__expand_image(arc, &coded_block);
  // One image in a file: nothing to parse is not the end of a list of members,
  // it is a file that is not one of ours.
  if( !p_i )
    __exit_402E40(3, InName);
  BmfImage*const p_i_img = (BmfImage*)p_i;
  printf("File %16s, image %dx%dx%d, size - %d\n", InName, p_i_img->width, p_i_img->height, p_i_img->depth&0x3F, p_i_img->data_size);
  int32_t Depth = p_i_img->depth&0x3F;
  if( Depth==2||Depth==15||Depth==16 ) {
    printf("%s: %d bits per pixel is not a BMP depth\n", OutName, Depth);
    exit(5);
  }
  if( !__write_bmp((BmfImage*)p_i, (char*)OutName, 1) )
    __exit_402E40(5, OutName);
  free(coded_block);
  coded_block = nullptr;
  free(p_i);
  __bmf_destroy_archive(arc, 1);
}


int32_t __main(int32_t argc, const char** argv) {
  bmf_set_denormal_mode();
  __set_new_handler(__out_of_memory_handler);
  printf("BMF lossless image compressor, v.2.01 (C) 1998-1999, 2009 by Dmitry Shkarin\n");
  int32_t mode = argc==4&&!argv[1][1] ? toupper(argv[1][0]) : 0;
  if( mode!='C'&&mode!='D' ) {
    printf("e-mail: <dmitry.shkarin@mtu-net.ru>;  web: http://compression.graphicon.ru/ds/\n"
           "Usage: bmf c input.bmp output     compress, always with -S -Q9\n"
           "       bmf d input output.bmp     expand\n");
    return 1;
  }
  if( mode=='C' )
    __bmf_compress(argv[2], argv[3]);
  else
    __bmf_decompress(argv[2], argv[3]);
  return 0;
}

int32_t main(int32_t argc, char** argv) {
  return __main(argc, (const char**)argv);
}
