// ccsds -- a lossless BMP coder built on CCSDS 123.0-R-1
//
//   ccsds c [options] input.bmp output.ccsds
//   ccsds d [options] input.ccsds output.bmp
//
// The image goes in through bmp.inc's reader, is presented to the CCSDS 123
// predictor as a small hyperspectral cube (one band per colour plane), and the
// mapped residuals go out through the CCSDS entropy coder.  Decoding runs the
// same machinery backwards and hands the samples back to bmp.inc's writer.
//
// Nothing here is lossy: the predictor is the standard's lossless one, the
// residual mapping is a bijection, and the container carries the palette and
// the sub-byte row padding so the pixels come back exactly as they went in.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "bmf_glue.inc"
#include "bmp.inc"

#include "ccsds123/utils.h"
#include "ccsds123/predictor.h"
#include "ccsds123/entropy_encoder.h"
#include "ccsds123/decoder.h"
#include "ccsds123/unpredict.h"

// ---------------------------------------------------------------------------
// Container
//
// The CCSDS header already carries the cube's geometry, its dynamic range and
// every predictor and coder parameter, so this only has to say how the BMP was
// turned into a cube in the first place.

enum : uint32_t {
  cc_magic = 0x31424343,  // "CCB1", little endian
};

enum : uint8_t {
  cc_unpacked = 0x01,   // a 1 or 4 bpp source, one sample per pixel
  cc_palette = 0x02,
  cc_row_pad = 0x04,    // an explicit table of the rows' padding bits follows
  cc_rct = 0x08,        // the colour planes were run through the reversible transform
};

struct CcHeader {
  uint8_t bpp;
  uint8_t flags;
  uint16_t width;
  uint16_t height;
  uint16_t pal_entries;
};

constexpr int32_t cc_head_bytes = 12;

// ---------------------------------------------------------------------------
// Options

// Which entropy coder settings the compressor should consider.
enum : int32_t {
  coder_auto = 0,   // try them all and keep the smallest
  coder_sample = 1,
  coder_block = 2,
};

struct Options {
  // Entropy coder
  int32_t coder;
  int32_t block_size;  // 0 means "try every block size"
  int32_t k;              // -1 selects the default
  int32_t u_max;
  int32_t y_star;
  int32_t y_0;
  int32_t ref_interval;
  int32_t restricted;
  // Predictor
  int32_t pred_bands;     // -1 selects the default
  int32_t reduced;
  int32_t column_sum;
  int32_t reg_size;
  int32_t w_resolution;
  int32_t w_interval;
  int32_t w_initial;
  int32_t w_final;
  // Container
  int32_t rct;
  int32_t verbose;
};

void options_defaults(Options* o) {
  o[0].coder = coder_auto;
  o[0].block_size = 0;
  o[0].k = -1;
  o[0].u_max = 16;
  o[0].y_star = 6;
  o[0].y_0 = 1;
  o[0].ref_interval = 256;
  o[0].restricted = 0;
  o[0].pred_bands = -1;
  o[0].reduced = 0;
  o[0].column_sum = 0;
  o[0].reg_size = 32;
  o[0].w_resolution = 14;
  o[0].w_interval = 128;
  o[0].w_initial = -2;
  o[0].w_final = 5;
  o[0].rct = 0;
  o[0].verbose = 0;
}

const char usage_text[] =
  "ccsds -- lossless BMP coder (CCSDS 123.0-R-1)\n"
  "\n"
  "usage: ccsds c|d [options] input output\n"
  "\n"
  "  c   compress a BMP (1, 4, 8, 24 or 32 bpp) into a coded stream\n"
  "  d   decompress a coded stream back into a BMP\n"
  "\n"
  "general options:\n"
  "  -v, --verbose            report the geometry, the coder settings and the rate\n"
  "  -h, --help               this text\n"
  "\n"
  "entropy coder (compression only):\n"
  "      --auto               code with every setting below and keep the smallest\n"
  "                           result (default).  Prediction runs once either way, so\n"
  "                           this costs a fraction of the time and never loses\n"
  "      --sample             sample-adaptive coder only\n"
  "      --block              block-adaptive coder only\n"
  "  -k, --k N                accumulator initialisation constant, 0..D-2 (default 3)\n"
  "      --u-max N            unary length limit, 8..32 (default 16)\n"
  "      --y-star N           rescaling counter size, 4..9 (default 6)\n"
  "      --y0 N               initial count exponent, 1..8 (default 1)\n"
  "  -B, --block-size N       block size 8, 16, 32 or 64 (default: try each)\n"
  "      --ref-interval N     reference sample interval, 1..4096 (default 256)\n"
  "      --restricted         restricted code set, block coder with D<=4 only\n"
  "\n"
  "predictor (compression only):\n"
  "  -p, --pred-bands N       spectral bands used for prediction (default: all but one)\n"
  "      --reduced            reduced prediction mode (default: full)\n"
  "      --column-sum         column oriented local sum (default: neighbour oriented)\n"
  "      --reg-size N         register size, 4..64 (default 32)\n"
  "      --w-resolution N     weight resolution, 4..19 (default 14)\n"
  "      --w-interval N       weight update interval, a power of 2 in 16..2048 (default 128)\n"
  "      --w-initial N        weight update scaling exponent, initial, -6..9 (default -2)\n"
  "      --w-final N          weight update scaling exponent, final, -6..9 (default 5)\n"
  "\n"
  "colour (compression only):\n"
  "      --rct                decorrelate 24/32 bpp planes with the reversible\n"
  "                           YCoCg-R transform before prediction.  Off by default:\n"
  "                           CCSDS 123 learns the inter-band weights itself, and on\n"
  "                           every image measured the transform came out larger\n"
  "      --no-rct             code the colour planes as they are (default)\n";

void usage(FILE* to) {
  fputs(usage_text, to);
}

// Reads the argument of an option that takes one, whether it was written as
// --opt=N or as --opt N.
bool opt_value(int32_t argc, char** argv, int32_t* i, const char* inline_at, int32_t* out) {
  const char* text = inline_at;
  if( !text ) {
    if( i[0]+1>=argc )
      return false;
    text = argv[++i[0]];
  }
  char* end = nullptr;
  const long v = strtol(text, &end, 10);
  if( end==text||end[0] )
    return false;
  out[0] = (int32_t)v;
  return true;
}

// ---------------------------------------------------------------------------
// Sub-byte rows
//
// bmp.inc keeps 1 and 4 bpp images packed, with the tail of each row's last
// byte left over when the width does not divide evenly.  Those bits are not
// pixels, but they are part of the file, so the coder carries them beside the
// samples rather than guessing they are zero.

int32_t row_pad_bits(const BmfImage* img, int32_t bits) {
  return (int32_t)img[0].stride*8-img[0].width*bits;
}

// The low `pad` bits of each row's last byte, one row per output byte.
void collect_row_pad(const BmfImage* img, int32_t pad, uint8_t* out) {
  const uint32_t mask = (1u<<pad)-1;
  for( int32_t y = 0; y<img[0].height; ++y )
    out[y] = (uint8_t)(img[0].pixels[(uint32_t)y*img[0].stride+img[0].stride-1]&mask);
}

// One byte per pixel back into packed rows.  bmp.inc's pack_rows would do this,
// but only for widths that fill their last byte; this one also puts the padding
// bits back.
void pack_rows_padded(BmfImage* img, int32_t bits, const uint8_t* src, const uint8_t* pad) {
  const int32_t per = 8/bits;
  const uint32_t mask = (1u<<bits)-1;
  const int32_t w = img[0].width, h = img[0].height;
  const int32_t pad_bits = row_pad_bits(img, bits);
  for( int32_t y = 0; y<h; ++y ) {
    uint8_t*const row = img[0].pixels+(uint32_t)y*img[0].stride;
    const uint8_t*const in = src+(size_t)y*w;
    memset(row, 0, img[0].stride);
    for( int32_t x = 0; x<w; ++x )
      row[x/per] |= (uint8_t)((in[x]&mask)<<(8-bits*(x%per+1)));
    if( pad_bits>0&&pad )
      row[img[0].stride-1] |= (uint8_t)(pad[y]&((1u<<pad_bits)-1));
  }
}

// ---------------------------------------------------------------------------
// Colour transform
//
// YCoCg-R: reversible, and it needs one bit more than the source for the two
// chroma planes, which is why a transformed cube is coded at D+1 bits.  That
// extra bit is most of why it does not pay here -- see cube_shape.

void rct_forward(int32_t r, int32_t g, int32_t b, int32_t* y, int32_t* co, int32_t* cg) {
  const int32_t co_v = r-b;
  const int32_t t = b+(co_v>>1);
  const int32_t cg_v = g-t;
  y[0] = t+(cg_v>>1);
  co[0] = co_v;
  cg[0] = cg_v;
}

void rct_inverse(int32_t y, int32_t co, int32_t cg, int32_t* r, int32_t* g, int32_t* b) {
  const int32_t t = y-(cg>>1);
  g[0] = cg+t;
  b[0] = t-(co>>1);
  r[0] = co+b[0];
}

// The bias that carries the two signed chroma planes into the unsigned range
// the coder works in.  They span -(2^8 - 1) .. 2^8 - 1, so a D+1 = 9 bit sample
// holds them with the bias applied.
constexpr int32_t rct_bias = 1<<8;

// ---------------------------------------------------------------------------
// Parameter blocks

// Fills in the CCSDS structures from the options and the cube's shape.  Returns
// false, having said why, when the combination is one the standard's header
// cannot express or the coder would refuse.
bool build_params(const Options* o, int32_t x_size, int32_t y_size, int32_t z_size, int32_t dyn_range,
                  input_feature_t* in, predictor_config_t* pred, encoder_config_t* enc) {
  memset(in, 0, sizeof(input_feature_t));
  memset(pred, 0, sizeof(predictor_config_t));
  memset(enc, 0, sizeof(encoder_config_t));

  in[0].signed_samples = 0;
  in[0].dyn_range = (uint8_t)dyn_range;
  in[0].x_size = (uint32_t)x_size;
  in[0].y_size = (uint32_t)y_size;
  in[0].z_size = (uint32_t)z_size;
  in[0].in_interleaving = BSQ;
  in[0].byte_ordering = LITTLE;
  in[0].regular_input = 1;

  int32_t bands = o[0].pred_bands<0 ? z_size-1 : o[0].pred_bands;
  // Both sides of the codec clamp a too-large band count, but to different
  // values, so it is clamped here instead and they never have to.
  if( bands>z_size )
    bands = z_size;
  if( bands>15 )
    bands = 15;
  if( bands<0 )
    bands = 0;
  pred[0].pred_bands = (uint8_t)bands;
  pred[0].user_input_pred_bands = (uint8_t)bands;
  pred[0].full = o[0].reduced ? 0 : 1;
  pred[0].neighbour_sum = o[0].column_sum ? 0 : 1;
  pred[0].register_size = (uint8_t)o[0].reg_size;
  pred[0].weight_resolution = (uint8_t)o[0].w_resolution;
  pred[0].weight_interval = o[0].w_interval;
  pred[0].weight_initial = (char)o[0].w_initial;
  pred[0].weight_final = (char)o[0].w_final;
  pred[0].weight_init_resolution = 0;
  pred[0].weight_init_table = nullptr;

  enc[0].out_interleaving = BSQ;
  enc[0].out_interleaving_depth = 0;
  enc[0].out_wordsize = 1;
  // The coder and the block size are filled in per candidate; everything else is
  // the same whichever one wins.  u_max is only written to the header by the
  // sample-adaptive coder, but it also sizes the coder's worst-case output
  // buffer, so it is always set.
  enc[0].encoding_method = SAMPLE;
  enc[0].u_max = (uint32_t)o[0].u_max;
  enc[0].y_star = (uint32_t)o[0].y_star;
  enc[0].y_0 = (uint32_t)o[0].y_0;
  enc[0].block_size = 16;
  enc[0].restricted = (uint8_t)(o[0].restricted&&dyn_range<=4 ? 1 : 0);
  enc[0].ref_interval = (uint32_t)o[0].ref_interval;

  int32_t k = o[0].k;
  if( k<0 )
    k = dyn_range-2<3 ? dyn_range-2 : 3;
  if( k>dyn_range-2 ) {
    fprintf(stderr, "error: k must be at most D-2 = %d for %d bit samples\n", dyn_range-2, dyn_range);
    return false;
  }
  enc[0].k = (uint32_t)k;
  enc[0].k_init = (uint32_t*)bmf_new(sizeof(uint32_t)*(size_t)z_size);
  for( int32_t z = 0; z<z_size; ++z )
    enc[0].k_init[z] = (uint32_t)k;

  if( o[0].reg_size<4||o[0].reg_size>64 ) {
    fprintf(stderr, "error: the register size must be in [4, 64]\n");
    return false;
  }
  if( o[0].w_resolution<4||o[0].w_resolution>19 ) {
    fprintf(stderr, "error: the weight resolution must be in [4, 19]\n");
    return false;
  }
  if( o[0].w_interval<16||o[0].w_interval>2048||(o[0].w_interval&(o[0].w_interval-1)) ) {
    fprintf(stderr, "error: the weight update interval must be a power of 2 in [16, 2048]\n");
    return false;
  }
  if( o[0].w_initial<-6||o[0].w_initial>9||o[0].w_final<-6||o[0].w_final>9 ) {
    fprintf(stderr, "error: the weight update scaling exponents must be in [-6, 9]\n");
    return false;
  }
  if( o[0].w_initial>o[0].w_final ) {
    fprintf(stderr, "error: the initial weight scaling exponent must not exceed the final one\n");
    return false;
  }
  // Both coders' parameters are checked whatever was asked for: either may be
  // tried, and a value out of range is a mistake worth reporting even when the
  // coder it belongs to is not the one that ends up running.
  if( o[0].u_max<8||o[0].u_max>32 ) {
    fprintf(stderr, "error: the unary length limit must be in [8, 32]\n");
    return false;
  }
  if( o[0].y_0<1||o[0].y_0>8 ) {
    fprintf(stderr, "error: the initial count exponent must be in [1, 8]\n");
    return false;
  }
  if( o[0].y_star<4||o[0].y_star>9||o[0].y_star<o[0].y_0+1 ) {
    fprintf(stderr, "error: the rescaling counter size must be in [max(4, y0+1), 9]\n");
    return false;
  }
  if( o[0].block_size!=0&&o[0].block_size!=8&&o[0].block_size!=16
      &&o[0].block_size!=32&&o[0].block_size!=64 ) {
    fprintf(stderr, "error: the block size must be 8, 16, 32 or 64\n");
    return false;
  }
  if( o[0].ref_interval<1||o[0].ref_interval>4096 ) {
    fprintf(stderr, "error: the reference sample interval must be in [1, 4096]\n");
    return false;
  }
  return true;
}

// The entropy coder settings to try.  Prediction has already run by the time
// these are used, and coding is the cheap half, so trying all five and keeping
// the smallest costs little -- and no single one of them wins on every image:
// the block coder takes the colour photographs by a wide margin, the sample
// coder takes the smooth greyscales, and the small blocks take the palette art.
int32_t coder_candidates(const Options* o, encoder_t* method, uint8_t* block_size) {
  static const uint8_t sizes[4] = {8, 16, 32, 64};
  int32_t n = 0;
  if( o[0].coder!=coder_block ) {
    method[n] = SAMPLE;
    block_size[n] = 16;
    ++n;
  }
  if( o[0].coder!=coder_sample ) {
    for( int32_t i = 0; i<4; ++i ) {
      if( o[0].block_size&&sizes[i]!=(uint8_t)o[0].block_size )
        continue;
      method[n] = BLOCK;
      block_size[n] = sizes[i];
      ++n;
    }
  }
  return n;
}

void describe(const input_feature_t* in, const predictor_config_t* pred, const encoder_config_t* enc) {
  fprintf(stderr, "  cube        %u x %u x %u, %u bit samples\n",
          in[0].x_size, in[0].y_size, in[0].z_size, in[0].dyn_range);
  fprintf(stderr, "  predictor   %s mode, %s local sum, %d prediction band%s\n",
          pred[0].full ? "full" : "reduced", pred[0].neighbour_sum ? "neighbour oriented" : "column oriented",
          pred[0].pred_bands, pred[0].pred_bands==1 ? "" : "s");
  fprintf(stderr, "  weights     resolution %d, interval %d, exponent %d..%d, register %d\n",
          pred[0].weight_resolution, pred[0].weight_interval,
          pred[0].weight_initial, pred[0].weight_final, pred[0].register_size);
  if( !enc )
    return;
  if( enc[0].encoding_method==SAMPLE )
    fprintf(stderr, "  coder       sample adaptive, u_max %u, y* %u, y0 %u, k %u\n",
            enc[0].u_max, enc[0].y_star, enc[0].y_0, enc[0].k);
  else
    fprintf(stderr, "  coder       block adaptive, block %u, reference interval %u%s\n",
            enc[0].block_size, enc[0].ref_interval, enc[0].restricted ? ", restricted" : "");
}

// ---------------------------------------------------------------------------
// Cube geometry
//
// How a BMP of a given depth is presented to the predictor.

struct CubeShape {
  int32_t x_size;
  int32_t y_size;
  int32_t z_size;
  int32_t dyn_range;
  int32_t bits;       // source bits per pixel
  int32_t planes;     // bytes per pixel, for the byte-per-plane depths
  bool unpacked;      // a sub-byte source, one sample per pixel
  bool rct;           // the colour planes carry Y, Co, Cg rather than B, G, R
};

// A colour BMP becomes one band per byte of the pixel, in the order the bytes
// sit in the file: blue, green, red and, at 32 bpp, alpha.  Permuting them to
// put green first was measured and is not worth a format flag: CCSDS 123 learns
// the inter-band weights either way.  So, on every image measured, is a fixed
// colour transform ahead of it -- --rct came out 2 to 3 percent larger, which is
// why it is not the default.
bool cube_shape(int32_t bpp, int32_t width, int32_t height, bool want_rct, CubeShape* s) {
  memset(s, 0, sizeof(CubeShape));
  s[0].x_size = width;
  s[0].y_size = height;
  s[0].bits = bpp;
  switch( bpp ) {
    case 1:
    case 4:
      s[0].z_size = 1;
      s[0].dyn_range = bpp<2 ? 2 : bpp;  // the standard's D starts at 2
      s[0].planes = 1;
      s[0].unpacked = true;
      break;
    case 8:
      s[0].z_size = 1;
      s[0].dyn_range = 8;
      s[0].planes = 1;
      break;
    case 24:
    case 32:
      s[0].planes = bpp/8;
      s[0].z_size = s[0].planes;
      s[0].rct = want_rct;
      // The transform's chroma planes need one bit more than the source.
      s[0].dyn_range = want_rct ? 9 : 8;
      break;
    default:
      fprintf(stderr, "error: %d bits per pixel is not supported\n", bpp);
      return false;
  }
  return true;
}

// The CCSDS structures index samples with 32 bit arithmetic, so a cube has to
// stay well inside that even before the memory it would take is considered.
bool cube_fits(const CubeShape* s) {
  const uint64_t n = (uint64_t)s[0].x_size*(uint64_t)s[0].y_size*(uint64_t)s[0].z_size;
  if( n==0||n>(uint64_t)1<<28 ) {
    fprintf(stderr, "error: %llu samples is out of range for this coder\n", (unsigned long long)n);
    return false;
  }
  return true;
}

// ---------------------------------------------------------------------------
// BMP <-> samples

void samples_from_image(const BmfImage* img, const CubeShape* s, uint16_t* samples) {
  const int32_t w = s[0].x_size, h = s[0].y_size;
  const size_t band = (size_t)w*h;
  if( s[0].unpacked ) {
    uint8_t* flat = (uint8_t*)bmf_new(band);
    unpack_rows(img, flat);
    for( size_t i = 0; i<band; ++i )
      samples[i] = flat[i];
    free(flat);
    return;
  }
  if( s[0].z_size==1 ) {
    for( int32_t y = 0; y<h; ++y ) {
      const uint8_t*const row = img[0].pixels+(uint32_t)y*img[0].stride;
      for( int32_t x = 0; x<w; ++x )
        samples[(size_t)y*w+x] = row[x];
    }
    return;
  }
  const int32_t planes = s[0].planes;
  for( int32_t y = 0; y<h; ++y ) {
    const uint8_t*const row = img[0].pixels+(uint32_t)y*img[0].stride;
    for( int32_t x = 0; x<w; ++x ) {
      const uint8_t*const px = row+(size_t)x*planes;
      const size_t at = (size_t)y*w+x;
      if( s[0].rct ) {
        int32_t yy, co, cg;
        rct_forward(px[2], px[1], px[0], &yy, &co, &cg);
        samples[at] = (uint16_t)yy;
        samples[band+at] = (uint16_t)(co+rct_bias);
        samples[2*band+at] = (uint16_t)(cg+rct_bias);
        if( planes>3 )
          samples[3*band+at] = px[3];
      } else {
        for( int32_t z = 0; z<planes; ++z )
          samples[(size_t)z*band+at] = px[z];
      }
    }
  }
}

void image_from_samples(BmfImage* img, const CubeShape* s, const uint16_t* samples, const uint8_t* row_pad) {
  const int32_t w = s[0].x_size, h = s[0].y_size;
  const size_t band = (size_t)w*h;
  if( s[0].unpacked ) {
    uint8_t* flat = (uint8_t*)bmf_new(band);
    for( size_t i = 0; i<band; ++i )
      flat[i] = (uint8_t)samples[i];
    pack_rows_padded(img, s[0].bits, flat, row_pad);
    free(flat);
    return;
  }
  if( s[0].z_size==1 ) {
    for( int32_t y = 0; y<h; ++y ) {
      uint8_t*const row = img[0].pixels+(uint32_t)y*img[0].stride;
      for( int32_t x = 0; x<w; ++x )
        row[x] = (uint8_t)samples[(size_t)y*w+x];
    }
    return;
  }
  const int32_t planes = s[0].planes;
  for( int32_t y = 0; y<h; ++y ) {
    uint8_t*const row = img[0].pixels+(uint32_t)y*img[0].stride;
    for( int32_t x = 0; x<w; ++x ) {
      uint8_t*const px = row+(size_t)x*planes;
      const size_t at = (size_t)y*w+x;
      if( s[0].rct ) {
        int32_t r, g, b;
        rct_inverse((int32_t)samples[at], (int32_t)samples[band+at]-rct_bias,
                    (int32_t)samples[2*band+at]-rct_bias, &r, &g, &b);
        px[0] = (uint8_t)b;
        px[1] = (uint8_t)g;
        px[2] = (uint8_t)r;
        if( planes>3 )
          px[3] = (uint8_t)samples[3*band+at];
      } else {
        for( int32_t z = 0; z<planes; ++z )
          px[z] = (uint8_t)samples[(size_t)z*band+at];
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Container header

bool write_container(FILE* out, const CcHeader* h, const uint8_t* palette, const uint8_t* row_pad) {
  uint8_t head[cc_head_bytes];
  put_u32(head, cc_magic);
  head[4] = h[0].bpp;
  head[5] = h[0].flags;
  head[6] = (uint8_t)h[0].width;
  head[7] = (uint8_t)(h[0].width>>8);
  head[8] = (uint8_t)h[0].height;
  head[9] = (uint8_t)(h[0].height>>8);
  head[10] = (uint8_t)h[0].pal_entries;
  head[11] = (uint8_t)(h[0].pal_entries>>8);
  if( fwrite(head, 1, sizeof head, out)!=sizeof head )
    return false;
  if( h[0].pal_entries ) {
    const size_t n = (size_t)3*h[0].pal_entries;
    if( fwrite(palette, 1, n, out)!=n )
      return false;
  }
  if( h[0].flags&cc_row_pad ) {
    if( fwrite(row_pad, 1, h[0].height, out)!=h[0].height )
      return false;
  }
  return true;
}

// Reads the container header, allocating the palette and the row padding table
// when they are there.  The caller frees whatever it is handed.
bool read_container(FILE* in, CcHeader* h, uint8_t** palette, uint8_t** row_pad) {
  uint8_t head[cc_head_bytes];
  palette[0] = nullptr;
  row_pad[0] = nullptr;
  if( fread(head, 1, sizeof head, in)!=sizeof head ) {
    fprintf(stderr, "error: the input is too short to be a coded stream\n");
    return false;
  }
  uint32_t magic;
  memcpy(&magic, head, sizeof magic);
  if( magic!=cc_magic ) {
    fprintf(stderr, "error: not a ccsds coded stream\n");
    return false;
  }
  h[0].bpp = head[4];
  h[0].flags = head[5];
  h[0].width = (uint16_t)(head[6]|head[7]<<8);
  h[0].height = (uint16_t)(head[8]|head[9]<<8);
  h[0].pal_entries = (uint16_t)(head[10]|head[11]<<8);
  if( !h[0].width||!h[0].height ) {
    fprintf(stderr, "error: the coded stream describes an empty image\n");
    return false;
  }
  if( h[0].pal_entries>256||(h[0].pal_entries&&h[0].bpp>8)
      ||(h[0].pal_entries&&h[0].pal_entries!=(1u<<(h[0].bpp&31))) ) {
    fprintf(stderr, "error: the coded stream has an inconsistent palette\n");
    return false;
  }
  if( h[0].pal_entries ) {
    const size_t n = (size_t)3*h[0].pal_entries;
    palette[0] = (uint8_t*)bmf_new(n);
    if( fread(palette[0], 1, n, in)!=n ) {
      fprintf(stderr, "error: the coded stream ends inside its palette\n");
      return false;
    }
  }
  if( h[0].flags&cc_row_pad ) {
    row_pad[0] = (uint8_t*)bmf_new(h[0].height);
    if( fread(row_pad[0], 1, h[0].height, in)!=h[0].height ) {
      fprintf(stderr, "error: the coded stream ends inside its row padding\n");
      return false;
    }
  }
  return true;
}

// A decoded header may have brought a weight initialisation table with it,
// sized by the cube it describes.  Nothing else in the predictor config owns
// memory, so this is the whole of its clean-up.
void free_predictor(predictor_config_t* pred, uint32_t z_size) {
  if( !pred[0].weight_init_table )
    return;
  for( uint32_t z = 0; z<z_size; ++z )
    free(pred[0].weight_init_table[z]);
  free(pred[0].weight_init_table);
  pred[0].weight_init_table = nullptr;
}

// ---------------------------------------------------------------------------

int32_t compress(const char* in_path, const char* out_path, const Options* o) {
  BmfImage* img = read_bmp((char*)in_path);
  if( !img ) {
    fprintf(stderr, "error: %s is not a BMP this coder can read\n", in_path);
    fprintf(stderr, "       (uncompressed or RLE, 1, 4, 8, 24 or 32 bpp, with a 40 byte info header)\n");
    return 1;
  }

  const int32_t bpp = img[0].depth&depth_bits;
  CubeShape shape;
  if( !cube_shape(bpp, img[0].width, img[0].height, o[0].rct!=0, &shape) ) {
    free(img);
    return 1;
  }
  if( !cube_fits(&shape) ) {
    free(img);
    return 1;
  }

  CcHeader head;
  head.bpp = (uint8_t)bpp;
  head.flags = 0;
  head.width = img[0].width;
  head.height = img[0].height;
  head.pal_entries = 0;
  if( shape.unpacked )
    head.flags |= cc_unpacked;
  if( shape.rct )
    head.flags |= cc_rct;
  if( img[0].depth&depth_palette ) {
    head.flags |= cc_palette;
    head.pal_entries = (uint16_t)(1u<<(bpp&31));
  }

  // A sub-byte row whose width does not fill its last byte leaves bits over.
  // They are almost always zero, so the table only rides along when it has to.
  uint8_t* row_pad = nullptr;
  if( shape.unpacked ) {
    const int32_t pad = row_pad_bits(img, bpp);
    if( pad>0 ) {
      row_pad = (uint8_t*)bmf_new(img[0].height);
      collect_row_pad(img, pad, row_pad);
      for( int32_t y = 0; y<img[0].height; ++y )
        if( row_pad[y] ) {
          head.flags |= cc_row_pad;
          break;
        }
    }
  }

  input_feature_t in_params;
  predictor_config_t pred_params;
  encoder_config_t enc_params;
  if( !build_params(o, shape.x_size, shape.y_size, shape.z_size, shape.dyn_range,
                    &in_params, &pred_params, &enc_params) ) {
    free(row_pad);
    free(img);
    return 1;
  }

  const size_t n = (size_t)shape.x_size*shape.y_size*shape.z_size;
  uint16_t* samples = (uint16_t*)bmf_new(n*sizeof(uint16_t));
  uint16_t* residuals = (uint16_t*)bmf_new(n*sizeof(uint16_t));
  samples_from_image(img, &shape, samples);

  if( predict_samples(in_params, pred_params, samples, residuals)!=0 ) {
    fprintf(stderr, "error: prediction failed\n");
    free(samples);
    free(residuals);
    free(row_pad);
    free(enc_params.k_init);
    free(img);
    return 1;
  }
  free(samples);

  FILE* out = fopen(out_path, "wb");
  if( !out ) {
    fprintf(stderr, "error: cannot create %s\n", out_path);
    free(residuals);
    free(row_pad);
    free(enc_params.k_init);
    free(img);
    return 1;
  }
  const uint8_t* palette = head.pal_entries ? img[0].palette() : nullptr;
  if( !write_container(out, &head, palette, row_pad) ) {
    fprintf(stderr, "error: cannot write %s\n", out_path);
    fclose(out);
    free(residuals);
    free(row_pad);
    free(enc_params.k_init);
    free(img);
    return 1;
  }
  // Code the residuals under each candidate and keep the smallest.
  encoder_t methods[5];
  uint8_t block_sizes[5];
  const int32_t n_cand = coder_candidates(o, methods, block_sizes);
  uint8_t* best = nullptr;
  int32_t best_len = -1;
  encoder_t best_method = SAMPLE;
  uint8_t best_block = 16;
  for( int32_t c = 0; c<n_cand; ++c ) {
    enc_params.encoding_method = methods[c];
    enc_params.block_size = block_sizes[c];
    uint8_t* coded_stream = nullptr;
    const int32_t len = encode_to_buffer(in_params, enc_params, pred_params, residuals, &coded_stream);
    if( len<0 ) {
      free(coded_stream);
      continue;
    }
    if( best_len<0||len<best_len ) {
      free(best);
      best = coded_stream;
      best_len = len;
      best_method = methods[c];
      best_block = block_sizes[c];
    } else {
      free(coded_stream);
    }
  }
  // Put the winning settings back so the report below describes what was written.
  enc_params.encoding_method = best_method;
  enc_params.block_size = best_block;

  if( best_len<0 ) {
    fprintf(stderr, "error: entropy coding failed\n");
    fclose(out);
    free(residuals);
    free(row_pad);
    free(enc_params.k_init);
    free(img);
    return 1;
  }
  const bool wrote = fwrite(best, 1, (size_t)best_len, out)==(size_t)best_len;
  const long total = ftell(out);
  fclose(out);
  free(best);
  free(residuals);
  free(row_pad);
  free(enc_params.k_init);

  if( !wrote ) {
    fprintf(stderr, "error: cannot write %s\n", out_path);
    free(img);
    return 1;
  }

  if( o[0].verbose ) {
    fprintf(stderr, "%s -> %s\n", in_path, out_path);
    fprintf(stderr, "  bmp         %u x %u, %d bpp%s%s\n", img[0].width, img[0].height, bpp,
            (img[0].depth&depth_palette) ? ", palette" : "", shape.rct ? ", colour transform" : "");
    describe(&in_params, &pred_params, &enc_params);
    const double pixels = (double)img[0].width*img[0].height;
    fprintf(stderr, "  stream      %ld bytes, %.4f bits/pixel, %.4f bits/sample\n",
            total, total*8.0/pixels, total*8.0/(double)n);
  }
  free(img);
  return 0;
}

int32_t decompress(const char* in_path, const char* out_path, const Options* o) {
  FILE* in = fopen(in_path, "rb");
  if( !in ) {
    fprintf(stderr, "error: cannot open %s\n", in_path);
    return 1;
  }
  CcHeader head;
  uint8_t* palette = nullptr;
  uint8_t* row_pad = nullptr;
  if( !read_container(in, &head, &palette, &row_pad) ) {
    fclose(in);
    free(palette);
    free(row_pad);
    return 1;
  }

  CubeShape shape;
  if( !cube_shape(head.bpp, head.width, head.height, (head.flags&cc_rct)!=0, &shape)
      ||!cube_fits(&shape) ) {
    fclose(in);
    free(palette);
    free(row_pad);
    return 1;
  }
  if( ((head.flags&cc_unpacked)!=0)!=shape.unpacked||((head.flags&cc_rct)!=0)!=shape.rct ) {
    fprintf(stderr, "error: the coded stream's flags do not match its depth\n");
    fclose(in);
    free(palette);
    free(row_pad);
    return 1;
  }

  const size_t n = (size_t)shape.x_size*shape.y_size*shape.z_size;
  input_feature_t in_params;
  predictor_config_t pred_params;
  uint16_t* residuals = nullptr;
  memset(&in_params, 0, sizeof in_params);
  memset(&pred_params, 0, sizeof pred_params);
  if( decode_from_stream(in, &in_params, &pred_params, &residuals, (uint32_t)n)!=0 ) {
    fprintf(stderr, "error: decoding failed\n");
    fclose(in);
    free_predictor(&pred_params, in_params.z_size);
    free(residuals);
    free(palette);
    free(row_pad);
    return 1;
  }
  fclose(in);

  // The CCSDS header is written by this coder, but it is the part of the file a
  // truncation or an edit shows up in first, so it is checked against what the
  // container said the image was rather than trusted.
  if( in_params.x_size!=(uint32_t)shape.x_size||in_params.y_size!=(uint32_t)shape.y_size
      ||in_params.z_size!=(uint32_t)shape.z_size||in_params.dyn_range!=(uint8_t)shape.dyn_range ) {
    fprintf(stderr, "error: the coded stream's geometry (%u x %u x %u, %u bit) does not match "
                    "its container (%d x %d x %d, %d bit)\n",
            in_params.x_size, in_params.y_size, in_params.z_size, in_params.dyn_range,
            shape.x_size, shape.y_size, shape.z_size, shape.dyn_range);
    free_predictor(&pred_params, in_params.z_size);
    free(residuals);
    free(palette);
    free(row_pad);
    return 1;
  }

  uint16_t* samples = (uint16_t*)bmf_new(n*sizeof(uint16_t));
  if( unpredict_samples(in_params, pred_params, residuals, samples)!=0 ) {
    fprintf(stderr, "error: reconstruction failed\n");
    free_predictor(&pred_params, in_params.z_size);
    free(samples);
    free(residuals);
    free(palette);
    free(row_pad);
    return 1;
  }
  free(residuals);

  BmfImage* img = alloc_image(head.width, head.height, head.bpp, head.bpp<=8, 1);
  if( !img ) {
    fprintf(stderr, "error: cannot allocate the output image\n");
    free_predictor(&pred_params, in_params.z_size);
    free(samples);
    free(palette);
    free(row_pad);
    return 1;
  }
  image_from_samples(img, &shape, samples, row_pad);
  if( head.pal_entries&&(img[0].depth&depth_palette) )
    memcpy(img[0].palette(), palette, (size_t)3*head.pal_entries);
  free(samples);

  const int32_t ok = write_bmp(img, (char*)out_path, 0);
  if( !ok )
    fprintf(stderr, "error: cannot write %s\n", out_path);

  if( ok&&o[0].verbose ) {
    fprintf(stderr, "%s -> %s\n", in_path, out_path);
    fprintf(stderr, "  bmp         %u x %u, %d bpp%s%s\n", img[0].width, img[0].height, head.bpp,
            head.pal_entries ? ", palette" : "", shape.rct ? ", colour transform" : "");
    describe(&in_params, &pred_params, nullptr);
  }

  free_predictor(&pred_params, in_params.z_size);
  free(img);
  free(palette);
  free(row_pad);
  return ok ? 0 : 1;
}

int main(int argc, char** argv) {
  if( argc<2 ) {
    usage(stderr);
    return 1;
  }
  const char* mode = argv[1];
  if( !strcmp(mode, "-h")||!strcmp(mode, "--help") ) {
    usage(stdout);
    return 0;
  }
  const bool encoding = !strcmp(mode, "c");
  if( !encoding&&strcmp(mode, "d") ) {
    fprintf(stderr, "error: the first argument must be c (compress) or d (decompress)\n\n");
    usage(stderr);
    return 1;
  }

  Options o;
  options_defaults(&o);
  const char* paths[2] = {nullptr, nullptr};
  int32_t n_paths = 0;

  for( int32_t i = 2; i<argc; ++i ) {
    char* arg = argv[i];
    if( arg[0]!='-'||!arg[1] ) {
      if( n_paths>=2 ) {
        fprintf(stderr, "error: too many file names\n");
        return 1;
      }
      paths[n_paths++] = arg;
      continue;
    }
    // --opt=value is accepted alongside --opt value.
    char* eq = strchr(arg, '=');
    char* inline_at = nullptr;
    if( eq ) {
      eq[0] = 0;
      inline_at = eq+1;
    }
    bool ok = true;
    if( !strcmp(arg, "-h")||!strcmp(arg, "--help") ) {
      usage(stdout);
      return 0;
    } else if( !strcmp(arg, "-v")||!strcmp(arg, "--verbose") ) {
      o.verbose = 1;
    } else if( !strcmp(arg, "--auto") ) {
      o.coder = coder_auto;
    } else if( !strcmp(arg, "--sample") ) {
      o.coder = coder_sample;
    } else if( !strcmp(arg, "--block") ) {
      o.coder = coder_block;
    } else if( !strcmp(arg, "--restricted") ) {
      o.restricted = 1;
    } else if( !strcmp(arg, "--reduced") ) {
      o.reduced = 1;
    } else if( !strcmp(arg, "--column-sum") ) {
      o.column_sum = 1;
    } else if( !strcmp(arg, "--rct") ) {
      o.rct = 1;
    } else if( !strcmp(arg, "--no-rct") ) {
      o.rct = 0;
    } else if( !strcmp(arg, "-k")||!strcmp(arg, "--k") ) {
      ok = opt_value(argc, argv, &i, inline_at, &o.k);
    } else if( !strcmp(arg, "--u-max") ) {
      ok = opt_value(argc, argv, &i, inline_at, &o.u_max);
    } else if( !strcmp(arg, "--y-star") ) {
      ok = opt_value(argc, argv, &i, inline_at, &o.y_star);
    } else if( !strcmp(arg, "--y0") ) {
      ok = opt_value(argc, argv, &i, inline_at, &o.y_0);
    } else if( !strcmp(arg, "-B")||!strcmp(arg, "--block-size") ) {
      ok = opt_value(argc, argv, &i, inline_at, &o.block_size);
      o.coder = coder_block;
    } else if( !strcmp(arg, "--ref-interval") ) {
      ok = opt_value(argc, argv, &i, inline_at, &o.ref_interval);
    } else if( !strcmp(arg, "-p")||!strcmp(arg, "--pred-bands") ) {
      ok = opt_value(argc, argv, &i, inline_at, &o.pred_bands);
    } else if( !strcmp(arg, "--reg-size") ) {
      ok = opt_value(argc, argv, &i, inline_at, &o.reg_size);
    } else if( !strcmp(arg, "--w-resolution") ) {
      ok = opt_value(argc, argv, &i, inline_at, &o.w_resolution);
    } else if( !strcmp(arg, "--w-interval") ) {
      ok = opt_value(argc, argv, &i, inline_at, &o.w_interval);
    } else if( !strcmp(arg, "--w-initial") ) {
      ok = opt_value(argc, argv, &i, inline_at, &o.w_initial);
    } else if( !strcmp(arg, "--w-final") ) {
      ok = opt_value(argc, argv, &i, inline_at, &o.w_final);
    } else {
      fprintf(stderr, "error: unknown option %s\n\n", arg);
      usage(stderr);
      return 1;
    }
    if( !ok ) {
      fprintf(stderr, "error: %s wants a number\n", arg);
      return 1;
    }
  }

  if( n_paths!=2 ) {
    fprintf(stderr, "error: expected an input and an output file name\n\n");
    usage(stderr);
    return 1;
  }
  return encoding ? compress(paths[0], paths[1], &o) : decompress(paths[0], paths[1], &o);
}
