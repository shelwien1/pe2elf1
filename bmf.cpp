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
 #define PRINTFLIKE(f, a) __attribute__((format(printf, f, a)))
#else
 #define INLINE   __forceinline
 #define NOINLINE __declspec(noinline)
 #define ALIGN(n) __declspec(align(n))
 #define PRINTFLIKE(f, a)
#endif

#define if_e0(x) if(__builtin_expect((x),0))
#define if_e1(x) if(__builtin_expect((x),1))

#include "bmf_util.inc"
#include "bmf_tables.inc"
#include "apm.inc"
#include "records.inc"
#include "rangecoder.inc"
#include "bmf_state.inc"
#include "sym_list.inc"
#include "counters.inc"
#include "alt_p1.inc"
#include "alt_p2.inc"
#include "ctx_model.inc"
#include "model.inc"
#include "bmp.inc"
#include "planes.inc"
#include "palette.inc"
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

// Region adaptivity, in its simplest useful form: the image is offered as one
// member or as a stack of horizontal strips, each searched and coded on its own,
// and the smaller wins.  What makes this worth more than it used to be is that
// the codec now has a mode -- the small-alphabet context model of section 7 --
// that some regions want and others do not; before it, both halves of a mixed
// page chose the same model anyway and a split was worth 0.3%.
//
// The strip count goes in the first member's second spare header byte, and the
// decoder concatenates that many members.  Splitting is only attempted when the
// halves differ enough in alphabet size to plausibly want different modes, so on
// a homogeneous image it costs nothing.
const int32_t kMaxStrips = 4;

int32_t bmf_compress_body(BmfFile* arc, BmfImage* p_i) {
  const int32_t height = p_i[0].height;
  // compress_to_memory re-Inits the codec, and the tail pointer lives in the
  // state it clears, so hold it here.
  CodedTail*const tail = bmf_codec.coded_block;
  if( !strips_worth_trying(p_i) )
    return bmf_codec.compress_image(arc, p_i, tail);
  uint8_t* whole_buf = nullptr;
  size_t whole_len = 0;
  {
    BmfImage* copy = strip_image(p_i, 0, height);
    whole_buf = bmf_codec.compress_to_memory(copy, &whole_len, tail);
    free(copy);
  }
  uint8_t* best_buf[kMaxStrips] = {nullptr, nullptr, nullptr, nullptr};
  size_t best_len[kMaxStrips] = {0, 0, 0, 0};
  int32_t best_n = 1;
  size_t best_total = whole_len;
  for( int32_t n = 2; n<=kMaxStrips; n *= 2 ) {
    uint8_t* buf[kMaxStrips] = {nullptr, nullptr, nullptr, nullptr};
    size_t len[kMaxStrips] = {0, 0, 0, 0};
    size_t total = 0;
    bool ok = height/n>=8;
    for( int32_t k = 0; ok&&k<n; ++k ) {
      const int32_t y0 = (int32_t)((int64_t)height*k/n), y1 = (int32_t)((int64_t)height*(k+1)/n);
      BmfImage* part = strip_image(p_i, y0, y1);
      part[0]._pad8[1] = (uint8_t)(k ? 0 : n);
      buf[k] = bmf_codec.compress_to_memory(part, &len[k], k ? nullptr : tail);
      free(part);
      ok = buf[k]!=nullptr;
      total += len[k];
    }
    bmf_log("  strips: %d x %d rows costs %llu bytes against %llu whole\n", n, height/n,
            (unsigned long long)total, (unsigned long long)whole_len);
    if( ok&&total<best_total ) {
      for( int32_t k = 0; k<best_n; ++k )
        free(best_buf[k]);
      best_total = total;
      best_n = n;
      for( int32_t k = 0; k<n; ++k ) {
        best_buf[k] = buf[k];
        best_len[k] = len[k];
      }
    } else {
      for( int32_t k = 0; k<n; ++k )
        free(buf[k]);
      break;
    }
  }
  int32_t written = 0;
  if( best_n==1 ) {
    written = arc[0].io.write_exact(whole_buf, whole_len) ? (int32_t)whole_len : 0;
  } else {
    written = 1;
    for( int32_t k = 0; k<best_n; ++k ) {
      written &= arc[0].io.write_exact(best_buf[k], best_len[k]) ? 1 : 0;
      free(best_buf[k]);
    }
    written = written ? (int32_t)best_total : 0;
  }
  arc[0].io.flush();
  free(whole_buf);
  return written;
}

void bmf_compress(const char* InName, const char* OutName) {
  int32_t i;
  BmfImage* p_i = read_bmp((char*)InName);
  if( !p_i )
    bmf_fatal(bmf_read_error);
  printf("File %16s, image %dx%dx%d, size - %d:", InName, p_i[0].width, p_i[0].height, p_i[0].depth&depth_bits, p_i[0].data_size);
  bmf_log("\n");
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
  int32_t coded_len = bmf_compress_body(Arc, p_i);
  if( !coded_len )
    bmf_fatal(bmf_write_error, OutName);
  bmf_log("  actual coded size: ");
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
  if( p_i[0]._pad8[1]>1 ) {
    // A stack of strips: the rest of them follow as their own members.
    const int32_t n = p_i[0]._pad8[1];
    if( n>kMaxStrips )
      bmf_fatal(bmf_bad_file, InName);
    BmfImage* part[kMaxStrips];
    part[0] = p_i;
    int32_t rows = p_i[0].height;
    for( int32_t k = 1; k<n; ++k ) {
      part[k] = bmf_codec.expand_image(arc, nullptr);
      if( !part[k]||part[k][0].width!=p_i[0].width||part[k][0].depth!=p_i[0].depth )
        bmf_fatal(bmf_bad_file, InName);
      rows += part[k][0].height;
    }
    BmfImage* full = alloc_image(p_i[0].width, rows, p_i[0].depth&depth_bits,
                                 (p_i[0].depth&depth_palette)!=0, (p_i[0].flags&flags_packed)!=0);
    full[0].depth = p_i[0].depth;
    full[0].flags = p_i[0].flags&(uint8_t)~flags_transposed;
    int32_t at = 0;
    for( int32_t k = 0; k<n; ++k ) {
      for( int32_t y = 0; y<part[k][0].height; ++y, ++at )
        memcpy(full[0].pixels+(uint32_t)at*full[0].stride,
               part[k][0].pixels+(uint32_t)y*part[k][0].stride, full[0].stride);
      if( k )
        free(part[k]);
    }
    if( p_i[0].depth&depth_palette )
      memcpy(full[0].palette(), p_i[0].palette(), (size_t)3u<<(p_i[0].depth&depth_bits));
    free(p_i);
    p_i = full;
  }
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
  int32_t at = 2;
  if( argc==5&&args[2][0]=='-'&&toupper(args[2][1])=='V'&&!args[2][2] ) {
    bmf_verbose = 1;
    at = 3;
  }
  const int32_t want_argc = at+2;
  int32_t mode = argc==want_argc&&!args[1][1] ? toupper(args[1][0]) : 0;
  if( mode!='C'&&mode!='D' ) {
    printf("e-mail: <dmitry.shkarin@mtu-net.ru>;  web: http://compression.graphicon.ru/ds/\n"
           "Usage: bmf c [-v] input.bmp output     compress, always with -S -Q9\n"
           "       bmf d [-v] input output.bmp     expand\n"
           "       -v   report the coding-method trials and the choices made\n");
    return 1;
  }
  if( mode=='C' )
    bmf_compress(args[at], args[at+1]);
  else
    bmf_decompress(args[at], args[at+1]);
  return 0;
}
