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

#ifdef __GNUC__
 #define INLINE   __attribute__((always_inline)) inline
 #define NOINLINE __attribute__((noinline))
 #define ALIGN(n) __attribute__((aligned(n)))
 #define restrict __restrict
#else
 #define INLINE   __forceinline
 #define NOINLINE __declspec(noinline)
 #define ALIGN(n) __declspec(align(n))
#endif

#define if_e0(x) if(__builtin_expect((x),0))
#define if_e1(x) if(__builtin_expect((x),1))

#include "bmf_util.inc"
#include "bmf_tables.inc"
#include "records.inc"
#include "rangecoder.inc"
#include "bmf_state.inc"
#include "sym_list.inc"
#include "counters.inc"
#include "alt_p1.inc"
#include "alt_p2.inc"
#include "model.inc"
#include "bmp.inc"
#include "planes.inc"
#include "codec.inc"

// Layout guards.  Several routines deliberately write past the end of one
// member into the next (the fold/fold_hi pair filled by alt_init_tables, the
// bias[] slots NbRow::predict reaches through p2_row, the ModelBlock grid
// memset that runs one record into its padding), and reset() clears exactly
// the BMFState subobject.  These assertions make any reordering that would
// silently break those a build failure instead.
static_assert(offsetof(AltP1Block, fold_hi)==offsetof(AltP1Block, fold)+256);
static_assert(offsetof(AltP1Block, unfold)==offsetof(AltP1Block, fold_hi)+256);
static_assert(offsetof(AltP2Block, fold_hi)==offsetof(AltP2Block, fold)+256);
static_assert(offsetof(AltP2Block, unfold)==offsetof(AltP2Block, fold_hi)+256);
static_assert(offsetof(AltP2Block, bias)==offsetof(AltP2Block, p2_row)+sizeof(float)*7*4);
static_assert(offsetof(ModelBlock, _pad1051664)==offsetof(ModelBlock, grid)+sizeof(FreqRec)*ModelBlock::kFreqGridCount);
static_assert(offsetof(ModelBlock, grid)+188*sizeof(FreqRec)+0x100000<=offsetof(ModelBlock, bit_root));
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
static_assert(offsetof(BMFCodec, p1_blocks)>=sizeof(BMFState));
#pragma clang diagnostic pop
static_assert(sizeof(BmfImage)==16&&offsetof(BmfImage, pixels)==16);
static_assert(offsetof(BmfImage, depth)==10&&offsetof(BmfImage, flags)==11&&offsetof(BmfImage, data_size)==12);
static_assert(sizeof(CodedTail)==8);
static_assert(sizeof(BmpHeader)==54);
static_assert(sizeof(SymEntry)==3);

BMFCodec bmf_codec;

void bmf_compress(const char* InName, const char* OutName) {
  int32_t i;
  BmfImage* p_i = read_bmp((char*)InName);
  if( !p_i )
    bmf_fatal(bmf_read_error);
  printf("File %16s, image %dx%dx%d, size - %d:", InName, p_i[0].width, p_i[0].height, p_i[0].depth&depth_bits, p_i[0].data_size);
  static BmfFile arc_store;
  BmfFile* Arc = bmf_open_file(&arc_store, (char*)OutName, 0);
  int32_t Depth = p_i[0].depth;
  if( Depth&depth_palette ) {
    if( Depth&depth_grey ) {
      p_i[0].depth = Depth^depth_palette;
    } else {
      int32_t Colours = 1<<(Depth&depth_bits);
      int32_t Step = 0x100u>>(Depth&depth_bits);
      const uint8_t* Palette = p_i[0].palette();
      int32_t Grey = 0;
      for( i = 0; i<Colours; ++i ) {
        if( Palette[3*i]!=Grey||Palette[3*i+1]!=Grey||Palette[3*i+2]!=Grey )
          break;
        Grey += Step;
      }
      if( i>=Colours )
        p_i[0].depth = (Depth|depth_grey)^depth_palette;
    }
  }
  int32_t coded_len = bmf_codec.compress_image(Arc, p_i, bmf_codec.coded_block);
  if( !coded_len )
    bmf_fatal(bmf_write_error, OutName);
  printf("%6.3f bpp\n", (double)coded_len*8.0/(double)(p_i[0].height*p_i[0].width));
  free(p_i);
}

void bmf_decompress(const char* InName, const char* OutName) {
  BmfFile* arc;
  if( void* nb = bmf_new(sizeof(BmfFile)) )
    arc = bmf_open_file((BmfFile*)nb, (char*)InName, 1);
  else
    arc = nullptr;
  BmfImage* p_i = bmf_codec.expand_image(arc, &bmf_codec.coded_block);
  if( !p_i )
    bmf_fatal(bmf_bad_file, InName);
  printf("File %16s, image %dx%dx%d, size - %d\n", InName, p_i[0].width, p_i[0].height, p_i[0].depth&depth_bits, p_i[0].data_size);
  int32_t Depth = p_i[0].depth&depth_bits;
  if( Depth==2||Depth==15||Depth==16 ) {
    printf("%s: %d bits per pixel is not a BMP depth\n", OutName, Depth);
    exit(5);
  }
  if( !write_bmp(p_i, (char*)OutName, 1) )
    bmf_fatal(bmf_write_error, OutName);
  free(bmf_codec.coded_block);
  bmf_codec.coded_block = nullptr;
  free(p_i);
  bmf_close_file(arc);
}

int32_t main(int32_t argc, char** argv) {
  const char*const* args = (const char*const*)argv;
  bmf_codec.Init();
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
