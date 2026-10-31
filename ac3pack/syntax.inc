// syntax.inc -- the syntax of AC-3 and E-AC-3, as ac3dec and ac3tsv have it: the tables, the bits,
// the elements of a syncframe (AC3_FIELDS), the frame record, the bit allocation, and the walk of a
// frame (Walk), which reads the elements from the bits (BitIn), writes them (BitOut), or at record
// level hands them to an IO of the record's values: ac3tsv's text, ac3pack's model (model.inc).
// Then the CRCs of a frame and its size. This section is ac3tsv.cpp's (from ac3dec.cpp: derived
// from FFmpeg's AC-3 and E-AC-3 decoder) as it is; ac3pack never sets g_ffmpeg, so its walks follow
// A/52 where FFmpeg differs.

// ===================================================================================================
// Tables (A/52; as ffmpeg has them)
// ===================================================================================================

// frame sizes in 16-bit words, by frmsizecod and fscod (Table 5.18)
static const U16 frame_size_tab[38][3] = {
  {64, 69, 96},     {64, 70, 96},     {80, 87, 120},    {80, 88, 120},    {96, 104, 144},
  {96, 105, 144},   {112, 121, 168},  {112, 122, 168},  {128, 139, 192},  {128, 140, 192},
  {160, 174, 240},  {160, 175, 240},  {192, 208, 288},  {192, 209, 288},  {224, 243, 336},
  {224, 244, 336},  {256, 278, 384},  {256, 279, 384},  {320, 348, 480},  {320, 349, 480},
  {384, 417, 576},  {384, 418, 576},  {448, 487, 672},  {448, 488, 672},  {512, 557, 768},
  {512, 558, 768},  {640, 696, 960},  {640, 697, 960},  {768, 835, 1152}, {768, 836, 1152},
  {896, 975, 1344}, {896, 976, 1344}, {1024, 1114, 1536}, {1024, 1115, 1536},
  {1152, 1253, 1728}, {1152, 1254, 1728}, {1280, 1393, 1920}, {1280, 1394, 1920},
};
static const int sample_rate_tab[3] = {48000, 44100, 32000};
static const U16 bitrate_tab[19] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640};
// full bandwidth channels by acmod
static const U8 nfchans_tab[8] = {2, 1, 2, 3, 3, 4, 4, 5};
static const U8 eac3_blocks[4] = {1, 2, 3, 6};

// bit allocation (A/52 section 7.2)
static const U8 band_start_tab[51] = {
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,
  26, 27, 28, 31, 34, 37, 40, 43, 46, 49, 55, 61, 67, 73, 79, 85, 97, 109, 121, 133, 157, 181, 205, 229, 253};
static U8 bin_to_band_tab[253];  // (made from band_start_tab)
static const U8 log_add_tab[260] = {
  0x40, 0x3f, 0x3e, 0x3d, 0x3c, 0x3b, 0x3a, 0x39, 0x38, 0x37, 0x36, 0x35, 0x34, 0x34, 0x33, 0x32, 0x31, 0x30, 0x2f, 0x2f,
  0x2e, 0x2d, 0x2c, 0x2c, 0x2b, 0x2a, 0x29, 0x29, 0x28, 0x27, 0x26, 0x26, 0x25, 0x24, 0x24, 0x23, 0x23, 0x22, 0x21, 0x21,
  0x20, 0x20, 0x1f, 0x1e, 0x1e, 0x1d, 0x1d, 0x1c, 0x1c, 0x1b, 0x1b, 0x1a, 0x1a, 0x19, 0x19, 0x18, 0x18, 0x17, 0x17, 0x16,
  0x16, 0x15, 0x15, 0x15, 0x14, 0x14, 0x13, 0x13, 0x13, 0x12, 0x12, 0x12, 0x11, 0x11, 0x11, 0x10, 0x10, 0x10, 0x0f, 0x0f,
  0x0f, 0x0e, 0x0e, 0x0e, 0x0d, 0x0d, 0x0d, 0x0d, 0x0c, 0x0c, 0x0c, 0x0c, 0x0b, 0x0b, 0x0b, 0x0b, 0x0a, 0x0a, 0x0a, 0x0a,
  0x0a, 0x09, 0x09, 0x09, 0x09, 0x09, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x06, 0x06,
  0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
  0x04, 0x04, 0x04, 0x04, 0x04, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x02,
  0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const U16 hearing_threshold_tab[50][3] = {
  {0x04d0, 0x04f0, 0x0580}, {0x04d0, 0x04f0, 0x0580}, {0x0440, 0x0460, 0x04b0}, {0x0400, 0x0410, 0x0450},
  {0x03e0, 0x03e0, 0x0420}, {0x03c0, 0x03d0, 0x03f0}, {0x03b0, 0x03c0, 0x03e0}, {0x03b0, 0x03b0, 0x03d0},
  {0x03a0, 0x03b0, 0x03c0}, {0x03a0, 0x03a0, 0x03b0}, {0x03a0, 0x03a0, 0x03b0}, {0x03a0, 0x03a0, 0x03b0},
  {0x03a0, 0x03a0, 0x03a0}, {0x0390, 0x03a0, 0x03a0}, {0x0390, 0x0390, 0x03a0}, {0x0390, 0x0390, 0x03a0},
  {0x0380, 0x0390, 0x03a0}, {0x0380, 0x0380, 0x03a0}, {0x0370, 0x0380, 0x03a0}, {0x0370, 0x0380, 0x03a0},
  {0x0360, 0x0370, 0x0390}, {0x0360, 0x0370, 0x0390}, {0x0350, 0x0360, 0x0390}, {0x0350, 0x0360, 0x0390},
  {0x0340, 0x0350, 0x0380}, {0x0340, 0x0350, 0x0380}, {0x0330, 0x0340, 0x0380}, {0x0320, 0x0340, 0x0370},
  {0x0310, 0x0320, 0x0360}, {0x0300, 0x0310, 0x0350}, {0x02f0, 0x0300, 0x0340}, {0x02f0, 0x02f0, 0x0330},
  {0x02f0, 0x02f0, 0x0320}, {0x02f0, 0x02f0, 0x0310}, {0x0300, 0x02f0, 0x0300}, {0x0310, 0x0300, 0x02f0},
  {0x0340, 0x0320, 0x02f0}, {0x0390, 0x0350, 0x02f0}, {0x03e0, 0x0390, 0x0300}, {0x0420, 0x03e0, 0x0310},
  {0x0460, 0x0420, 0x0330}, {0x0490, 0x0450, 0x0350}, {0x04a0, 0x04a0, 0x03c0}, {0x0460, 0x0490, 0x0410},
  {0x0440, 0x0460, 0x0470}, {0x0440, 0x0440, 0x04a0}, {0x0520, 0x0480, 0x0460}, {0x0800, 0x0630, 0x0440},
  {0x0840, 0x0840, 0x0450}, {0x0840, 0x0840, 0x04e0},
};
static const U8 bap_tab[64] = {
  0,  1,  1,  1,  1,  1,  2,  2,  3,  3,  3,  4,  4,  5,  5,  6,  6,  6,  6,  7,  7,  7,
  7,  8,  8,  8,  8,  9,  9,  9,  9,  10, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 12, 13,
  13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15};
static const U8 hebap_tab[64] = {
  0,  1,  2,  3,  4,  5,  6,  7,  8,  8,  8,  8,  9,  9,  9,  10, 10, 10, 10, 11, 11, 11,
  11, 12, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17,
  17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19};
static const U8 slow_decay_tab[4] = {0x0f, 0x11, 0x13, 0x15};
static const U8 fast_decay_tab[4] = {0x3f, 0x53, 0x67, 0x7b};
static const U16 slow_gain_tab[4] = {0x540, 0x4d8, 0x478, 0x410};
static const U16 db_per_bit_tab[4] = {0x000, 0x700, 0x900, 0xb00};
static const S16 floor_tab[8] = {0x2f0, 0x2b0, 0x270, 0x230, 0x1f0, 0x170, 0x0f0, (S16)0xf800};
static const U16 fast_gain_tab[8] = {0x080, 0x100, 0x180, 0x200, 0x280, 0x300, 0x380, 0x400};
// mantissa bits by bap (6-15), and the levels of the symmetric quantisers (1-5)
static const U8 quant_bits[16] = {0, 3, 5, 7, 11, 15, 5, 6, 7, 8, 9, 10, 11, 12, 14, 16};

// rematrixing bands; the default coupling and SPX banding structures (Tables E2.12, E2.11)
static const U8 rematrix_band_tab[5] = {13, 25, 37, 61, 253};
static const U8 default_cpl_band_struct[18] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1};
static const U8 default_spx_band_struct[17] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1};

// E-AC-3 frame exponent strategies (Table E2.10): frmchexpstr -> each block's
enum { EXP_REUSE = 0, EXP_D15, EXP_D25, EXP_D45 };
static const U8 frm_expstr[32][6] = {
  {1, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 3}, {1, 0, 0, 0, 2, 0}, {1, 0, 0, 0, 3, 3}, {2, 0, 0, 2, 0, 0}, {2, 0, 0, 2, 0, 3},
  {2, 0, 0, 3, 2, 0}, {2, 0, 0, 3, 3, 3}, {2, 0, 1, 0, 0, 0}, {2, 0, 2, 0, 0, 3}, {2, 0, 2, 0, 2, 0}, {2, 0, 2, 0, 3, 3},
  {2, 0, 3, 2, 0, 0}, {2, 0, 3, 2, 0, 3}, {2, 0, 3, 3, 2, 0}, {2, 0, 3, 3, 3, 3}, {3, 1, 0, 0, 0, 0}, {3, 1, 0, 0, 0, 3},
  {3, 2, 0, 0, 2, 0}, {3, 2, 0, 0, 3, 3}, {3, 2, 0, 2, 0, 0}, {3, 2, 0, 2, 0, 3}, {3, 2, 0, 3, 2, 0}, {3, 2, 0, 3, 3, 3},
  {3, 3, 1, 0, 0, 0}, {3, 3, 2, 0, 0, 3}, {3, 3, 2, 0, 2, 0}, {3, 3, 2, 0, 3, 3}, {3, 3, 3, 2, 0, 0}, {3, 3, 3, 2, 0, 3},
  {3, 3, 3, 3, 2, 0}, {3, 3, 3, 3, 3, 3},
};

// AHT (A/52 Annex E section 3.4): mantissa bits by hebap, GAQ's remapping (Table E3.6)
static const U8 bits_vs_hebap[20] = {0, 2, 3, 4, 5, 7, 8, 9, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 16};
static const S16 gaq_remap_1[12] = {4681, 2185, 1057, 520, 258, 129, 64, 32, 16, 8, 2, 0};
static const S16 gaq_remap_2_4_a[9][2] = {{-10923, -4681}, {-14043, -6554}, {-15292, -7399},
                                          {-15855, -7802}, {-16124, -7998}, {-16255, -8096},
                                          {-16320, -8144}, {-16352, -8168}, {-16368, -8180}};
static const S16 gaq_remap_2_4_b[9][2] = {{-5461, -1170}, {-11703, -4915}, {-14199, -6606},
                                          {-15327, -7412}, {-15864, -7805}, {-16126, -7999},
                                          {-16255, -8096}, {-16320, -8144}, {-16352, -8168}};
// SPX attenuation (Table E.25): pow(2, (bin + 1) * (code + 1) / -15)
static const float spx_atten_tab[32][3] = {
  {0.954841603910416503f, 0.911722488558216804f, 0.870550563296124125f}, {0.911722488558216804f, 0.831237896142787758f, 0.757858283255198995f},
  {0.870550563296124125f, 0.757858283255198995f, 0.659753955386447100f}, {0.831237896142787758f, 0.690956439983888004f, 0.574349177498517438f},
  {0.793700525984099792f, 0.629960524947436595f, 0.500000000000000000f}, {0.757858283255198995f, 0.574349177498517438f, 0.435275281648062062f},
  {0.723634618720189082f, 0.523647061410313364f, 0.378929141627599553f}, {0.690956439983888004f, 0.477420801955208307f, 0.329876977693223550f},
  {0.659753955386447100f, 0.435275281648062062f, 0.287174588749258719f}, {0.629960524947436595f, 0.396850262992049896f, 0.250000000000000000f},
  {0.601512518041058319f, 0.361817309360094541f, 0.217637640824031003f}, {0.574349177498517438f, 0.329876977693223550f, 0.189464570813799776f},
  {0.548412489847312945f, 0.300756259020529160f, 0.164938488846611775f}, {0.523647061410313364f, 0.274206244923656473f, 0.143587294374629387f},
  {0.500000000000000000f, 0.250000000000000000f, 0.125000000000000000f}, {0.477420801955208307f, 0.227930622139554201f, 0.108818820412015502f},
  {0.455861244279108402f, 0.207809474035696939f, 0.094732285406899888f}, {0.435275281648062062f, 0.189464570813799776f, 0.082469244423305887f},
  {0.415618948071393879f, 0.172739109995972029f, 0.071793647187314694f}, {0.396850262992049896f, 0.157490131236859149f, 0.062500000000000000f},
  {0.378929141627599553f, 0.143587294374629387f, 0.054409410206007751f}, {0.361817309360094541f, 0.130911765352578369f, 0.047366142703449930f},
  {0.345478219991944002f, 0.119355200488802049f, 0.041234622211652958f}, {0.329876977693223550f, 0.108818820412015502f, 0.035896823593657347f},
  {0.314980262473718298f, 0.099212565748012460f, 0.031250000000000000f}, {0.300756259020529160f, 0.090454327340023621f, 0.027204705103003875f},
  {0.287174588749258719f, 0.082469244423305887f, 0.023683071351724965f}, {0.274206244923656473f, 0.075189064755132290f, 0.020617311105826479f},
  {0.261823530705156682f, 0.068551561230914118f, 0.017948411796828673f}, {0.250000000000000000f, 0.062500000000000000f, 0.015625000000000000f},
  {0.238710400977604098f, 0.056982655534888536f, 0.013602352551501938f}, {0.227930622139554201f, 0.051952368508924235f, 0.011841535675862483f},
};


// ffmpeg's ways where they differ from A/52 (-x; see the top); and with them its banding
// structures, which it keeps from frame to frame
static int g_ffmpeg = 0;
static int g_ff_cplbndstrc[18], g_ff_spxbndstrc[17];

// ===================================================================================================
// Bits
// ===================================================================================================

constexpr int PAD = 16;  // zero bytes after a buffer the bit reader may read

// a frame's bits, read MSB first
struct BitIn {
  enum { BITS = 1, READ = 1 };
  const U8* p;
  int pos, end;  // bits
  void init(const U8* buf, int nbits) {
    p = buf;
    pos = 0;
    end = nbits;
  }
  U32 peek(int n) const {  // n <= 32, PAD bytes after the buffer
    const U8* q = p + (pos >> 3);
    U64 w = ((U64)q[0] << 56) | ((U64)q[1] << 48) | ((U64)q[2] << 40) | ((U64)q[3] << 32) | ((U64)q[4] << 24) |
            ((U64)q[5] << 16) | ((U64)q[6] << 8) | (U64)q[7];
    return n ? (U32)((w << (pos & 7)) >> (64 - n)) : 0;
  }
  bool u(int, int n, int& v) {
    if (pos + n > end) return false;
    v = (int)peek(n);
    pos += n;
    return true;
  }
  bool s(int fid, int n, int& v) {
    if (!u(fid, n, v)) return false;
    if (n && (v >> (n - 1))) v -= 1 << n;
    return true;
  }
  // n bits into b (MSB first; the last byte's unused bits 0)
  bool raw(int, int n, U8* b) {
    if (pos + n > end) return false;
    for (int i = 0; i < n; i += 8) {
      const int k = imin(8, n - i);
      b[i >> 3] = (U8)(peek(k) << (8 - k));
      pos += k;
    }
    return true;
  }
  bool r(int, int, int, int&) { return false; }  // (record level only)
  int peek_bsid() const { return p[5] >> 3; }    // (the bsid, bits 40-44 of the frame)
  void put_at(int, int, int) {}                   // (BitOut's)
  void open(int, int) {}                          // (the text's lines: ac3tsv's)
  void baps(const U8*, int, int) {}
};

// CRC-16 of A/52 7.10.1: x^16 + x^15 + x^2 + 1, MSB first, the register from 0
static U16 crc_tab[256];
static void crc_init(void) {
  for (int i = 0; i < 256; i++) {
    U32 c = (U32)i << 8;
    for (int k = 0; k < 8; k++) c = (c << 1) ^ ((c & 0x8000) ? 0x8005 : 0);
    crc_tab[i] = (U16)c;
  }
}
static U16 crc16(U16 c, const U8* p, int n) {
  while (n-- > 0) c = (U16)((c << 8) ^ crc_tab[((c >> 8) ^ *p++) & 0xFF]);
  return c;
}
// The word crc1 must be for the CRC of [crc1, the rest of the first 5/8) to be 0, rest the CRC of
// the rest (register from 0), nbits its length: crc1 x^(nbits + 16) = rest x^16, so crc1 = rest
// x^-(nbits + 16) mod the polynomial (whose x^0 term makes x invertible)
static inline U16 crc1_for(U16 rest, int nbits) {
  U32 c = rest;
  for (int i = 0; i < nbits + 16; i++) c = (c & 1) ? (c ^ 0x18005) >> 1 : c >> 1;
  return (U16)c;
}

// ===================================================================================================
// The syntax: the elements of a syncframe (A/52 sections 5.3-5.4, Annex E section 2), as a record
// ===================================================================================================

// The elements, by their names in A/52, each with the line of ac3tsv's text it goes to (its kind:
// H the header, A audfrm, B an audio block, E a channel's exponents, M its mantissas, Q its AHT
// data, T the trailer). A walk of a frame (Walk, below) reads each from the bits, or writes it, or
// (record level: ac3tsv's text, ac3pack's model) reads or writes the record's values as they are,
// through its IO; F_* tells the IO which element it has. (fcplstre, fcplinu: audfrm's cplstre and
// cplinu; exps, mgrp*, mant, gaqgrp: the bits' groups, whose values the record has as dexp, q,
// qfill, gain and gainfill.)
enum { K_H, K_A, K_B, K_E, K_M, K_Q, K_T, K_N };
#define AC3_FIELDS(X) \
  X(crc1, H) X(crc1ok, H) X(fscod, H) X(frmsizecod, H) X(bsid, H) X(bsmod, H) X(acmod, H) X(cmixlev, H) \
  X(surmixlev, H) X(dsurmod, H) X(lfeon, H) X(dialnorm, H) X(compre, H) X(compr, H) X(langcode, H) \
  X(langcod, H) X(audprodie, H) X(mixlevel, H) X(roomtyp, H) X(copyrightb, H) X(origbs, H) X(timecod1e, H) \
  X(timecod1, H) X(timecod2e, H) X(timecod2, H) X(xbsi1e, H) X(dmixmod, H) X(ltrtcmixlev, H) \
  X(ltrtsurmixlev, H) X(lorocmixlev, H) X(lorosurmixlev, H) X(xbsi2e, H) X(dsurexmod, H) X(dheadphonmod, H) \
  X(adconvtyp, H) X(xbsi2, H) X(encinfo, H) X(addbsie, H) X(addbsil, H) X(addbsi, H) \
  X(strmtyp, H) X(substreamid, H) X(frmsiz, H) X(fscod2, H) X(numblkscod, H) X(chanmape, H) X(chanmap, H) \
  X(mixmdate, H) X(lfemixlevcode, H) X(lfemixlevcod, H) X(pgmscle, H) X(pgmscl, H) X(extpgmscle, H) \
  X(extpgmscl, H) X(mixdef, H) X(premixcmpsel, H) X(drcsrc, H) X(premixcmpscl, H) X(mixdata12, H) \
  X(mixdeflen, H) X(mixdata, H) X(paninfoe, H) X(panmean, H) X(paninfo, H) X(frmmixcfginfoe, H) \
  X(blkmixcfginfoe, H) X(blkmixcfginfo, H) X(infomdate, H) X(sourcefscod, H) X(convsync, H) X(blkid, H) \
  X(convfrmsizecod, H) \
  X(expstre, A) X(ahte, A) X(snroffststr, A) X(transproce, A) X(blkswe, A) X(dithflage, A) X(bamode, A) \
  X(frmfgaincode, A) X(dbaflde, A) X(skipflde, A) X(spxattene, A) X(fcplstre, A) X(fcplinu, A) \
  X(cplexpstr, A) X(chexpstr, A) X(frmcplexpstr, A) X(frmchexpstr, A) X(lfeexpstr, A) X(convexpstre, A) \
  X(convexpstr, A) X(cplahtinu, A) X(chahtinu, A) X(lfeahtinu, A) X(frmcsnroffst, A) X(frmfsnroffst, A) \
  X(chintransproc, A) X(transprocloc, A) X(transproclen, A) X(chinspxatten, A) X(spxattencod, A) \
  X(blkstrtinfoe, A) X(blkstrtinfo, A) \
  X(blksw, B) X(dithflag, B) X(dynrnge, B) X(dynrng, B) X(spxstre, B) X(spxinu, B) X(chinspx, B) \
  X(spxstrtf, B) X(spxbegf, B) X(spxendf, B) X(spxbndstrce, B) X(spxbndstrc, B) X(spxcoe, B) X(spxblnd, B) \
  X(mstrspxco, B) X(spxcoexp, B) X(spxcomant, B) X(cplstre, B) X(cplinu, B) X(ecplinu, B) X(chincpl, B) \
  X(phsflginu, B) X(cplbegf, B) X(cplendf, B) X(cplbndstrce, B) X(cplbndstrc, B) X(cplcoe, B) \
  X(mstrcplco, B) X(cplcoexp, B) X(cplcomant, B) X(phsflg, B) X(rematstr, B) X(rematflg, B) X(expstr, B) \
  X(chbwcod, B) X(baie, B) X(sdcycod, B) X(fdcycod, B) X(sgaincod, B) X(dbpbcod, B) X(floorcod, B) \
  X(snroffste, B) X(csnroffst, B) X(fsnroffst, B) X(fgaincode, B) X(fgaincod, B) X(convsnroffste, B) \
  X(convsnroffst, B) X(cplleake, B) X(cplfleak, B) X(cplsleak, B) X(deltbaie, B) X(deltbae, B) \
  X(deltnseg, B) X(deltoffst, B) X(deltlen, B) X(deltba, B) X(skiple, B) X(skipl, B) X(skipfld, B) \
  X(qfill, B) \
  X(absexp, E) X(exps, E) X(dexp, E) X(gainrng, E) \
  X(mgrp1, M) X(mgrp2, M) X(mgrp4, M) X(mant, M) X(q, M) \
  X(gaqmod, Q) X(gaqgain, Q) X(gaqgrp, Q) X(gain, Q) X(gainfill, Q) X(vq, Q) X(gaqmant, Q) X(gaqbig, Q) \
  X(tail, T) X(auxdatae, T) X(crcrsv, T) X(crc2, T) X(crc2ok, T)
#define AC3_FID(n, k) F_##n,
enum { AC3_FIELDS(AC3_FID) F_N };
#undef AC3_FID
#define AC3_FNAME(n, k) #n,
static const char* const field_name[F_N] = {AC3_FIELDS(AC3_FNAME)};
#undef AC3_FNAME
#define AC3_FKIND(n, k) K_##k,
static const U8 field_kind[F_N] = {AC3_FIELDS(AC3_FKIND)};
#undef AC3_FKIND

// channel slots: 0 the coupling channel, 1..nfch the full bandwidth ones (in A/52's order: L C R
// Ls Rs as the mode has them), nfch + 1 the LFE
enum { NFCH = 5, NCH = 7, NBLK = 6, NBIN = 256, NCPLSB = 18, NSPXSB = 17, NSEG = 8, CPL = 0 };
constexpr int MAX_FRAME = 4096;  // bytes: E-AC-3's frmsiz is 11 bits of words

// the header: syncinfo and bsi
struct Bsi {
  int crc1, crc1ok;
  int fscod, frmsizecod, bsid, bsmod, acmod, cmixlev, surmixlev, dsurmod, lfeon;
  int dialnorm[2], compre[2], compr[2], langcode[2], langcod[2], audprodie[2], mixlevel[2], roomtyp[2];
  int copyrightb, origbs, timecod1e, timecod1, timecod2e, timecod2;
  int xbsi1e, dmixmod, ltrtcmixlev, ltrtsurmixlev, lorocmixlev, lorosurmixlev;
  int xbsi2e, dsurexmod, dheadphonmod, adconvtyp[2], xbsi2, encinfo;
  int addbsie, addbsil;
  U8 addbsi[64];
  // E-AC-3
  int strmtyp, substreamid, frmsiz, fscod2, numblkscod, chanmape, chanmap;
  int mixmdate, lfemixlevcode, lfemixlevcod, pgmscle[2], pgmscl[2], extpgmscle, extpgmscl, mixdef;
  int premixcmpsel, drcsrc, premixcmpscl, mixdata12, mixdeflen;
  U8 mixdata[40];
  int paninfoe[2], panmean[2], paninfo[2], frmmixcfginfoe, blkmixcfginfoe[NBLK], blkmixcfginfo[NBLK];
  int infomdate, sourcefscod, convsync, blkid, convfrmsizecod;
};
// E-AC-3's audfrm
struct Audfrm {
  int expstre, ahte, snroffststr, transproce, blkswe, dithflage, bamode, frmfgaincode, dbaflde, skipflde,
      spxattene;
  int cplstre[NBLK], cplinu[NBLK];
  int frmcplexpstr, frmchexpstr[NCH];
  int convexpstre, convexpstr[NCH];
  int cplahtinu, chahtinu[NCH], lfeahtinu;
  int frmcsnroffst, frmfsnroffst;
  int chintransproc[NCH], transprocloc[NCH], transproclen[NCH];
  int chinspxatten[NCH], spxattencod[NCH];
  int blkstrtinfoe, nblkstrtbits;
  U8 blkstrtinfo[16];
};
// an audio block, as sent
struct Blk {
  int blksw[NCH], dithflag[NCH], dynrnge[2], dynrng[2];
  int spxstre, spxinu, chinspx[NCH], spxstrtf, spxbegf, spxendf, spxbndstrce, spxbndstrc[NSPXSB];
  int spxcoe[NCH], spxblnd[NCH], mstrspxco[NCH], spxcoexp[NCH][NSPXSB], spxcomant[NCH][NSPXSB];
  int cplstre, cplinu, ecplinu, chincpl[NCH], phsflginu, cplbegf, cplendf, cplbndstrce, cplbndstrc[NCPLSB];
  int cplcoe[NCH], mstrcplco[NCH], cplcoexp[NCH][NCPLSB], cplcomant[NCH][NCPLSB], phsflg[NCPLSB];
  int rematstr, rematflg[4];
  int chbwcod[NCH], absexp[NCH], gainrng[NCH];
  U8 dexp[NCH][NBIN];  // exponent deltas + 2 (0..4), 3 per group; a group's first 5 for codes 125-127
  int baie, sdcycod, fdcycod, sgaincod, dbpbcod, floorcod;
  int snroffste, csnroffst, fsnroffst[NCH], fgaincode, fgaincod[NCH], convsnroffste, convsnroffst;
  int cplleake, cplfleak, cplsleak;
  int deltbaie, deltbae[NCH], deltnseg[NCH], deltoffst[NCH][NSEG], deltlen[NCH][NSEG], deltba[NCH][NSEG];
  int skiple, skipl;
  U8 skipfld[512];
  S16 q[NCH][NBIN];  // the mantissas, as levels: code - (levels - 1) / 2 for bap 1-5, signed for 6-15
  int qfill[3][2];   // the values of a block's last groups (bap 1, 2, 4) past its last mantissas
};
// a channel's AHT data (in block 0 for the frame)
struct Aht {
  int gaqmod;
  S8 gain[NBIN];     // GAQ's gain codes by bin: 0/1 (gaqmod 1-2), 0..2 (3; a group's first 3 for codes 27-31)
  int gainfill[2];   // gaqmod 3: the values of the last group past the last gain
  S16 vq[NBIN];      // hebap 1-7: the vector's index
  S16 m[NBIN][NBLK]; // hebap 8+: the mantissas, and where one is the escape, the large one after it
  S16 big[NBIN][NBLK];
};

// The state a walk keeps from block to block within a frame (reset as a frame starts): what the
// elements sent so far make of the blocks to come
struct Run {
  int cplinu, chincpl[NCH], phsflginu, cplbndstrc[NCPLSB], cpl_banded;
  int cplstrt, cplend, ncplbnd, cplbndsz[NCPLSB];
  int firstcplcos[NCH], firstcplleak;
  int spxinu, chinspx[NCH], spxbndstrc[NSPXSB], spx_banded, firstspxcos[NCH];
  int spx_begin, spx_end, spx_dst_start, spx_src_start, spx_dst_end, nspxbnd, spxbndsz[NSPXSB];
  int nrematbd, rematflg[4];
  int strtmant[NCH], endmant[NCH], nexpgrps[NCH];
  S8 exps[NCH][NBIN];
  U8 bap[NCH][NBIN];
  int sdecay, fdecay, sgain, dbknee, floor;  // bit allocation parameters, as the model takes them
  int snroffst[NCH], fgain[NCH], cplfleak, cplsleak;
  int deltbae[NCH], deltnseg[NCH], deltoffst[NCH][NSEG], deltlen[NCH][NSEG], deltba[NCH][NSEG];
  int cplahtinu, chahtinu[NCH];  // AHT in use by slot
};

// A syncframe: its elements as sent (b, a, k, aht, the tail and the CRCs) and what follows from
// them (eac3, fbytes ...). Plain data.
struct Frame {
  int eac3;            // bsid 11-16
  int fbytes;          // its size in bytes
  int nblk, nfch, lfe, nch, lfech;  // blocks; full bandwidth channels, LFE on, channels (nfch + lfe), its slot
  int sr_code, sr_shift, srate;
  Bsi b;
  Audfrm a;
  int expstr[NBLK][NCH];  // exponent strategies (E-AC-3: from audfrm)
  Blk k[NBLK];
  Aht aht[NCH];
  int tailbits;        // the bits between the last block and auxdatae
  U8 tail[MAX_FRAME];
  int auxdatae, crcrsv, crc2, crc2ok;
  int odd;             // values no encoder writes: group codes past the levels (decoded as ffmpeg does)
  const char* err;     // why a walk failed
  int err_blk;         // and in which block (-1 the header)
};

// --- bit allocation (A/52 section 7.2, as ffmpeg's ac3.c and ac3dsp.c) ----------------------------

static void bit_alloc_init(void) {
  for (int b = 0; b < 50; b++)
    for (int i = band_start_tab[b]; i < band_start_tab[b + 1]; i++) bin_to_band_tab[i] = (U8)b;
}
static inline int calc_lowcomp1(int a, int b0, int b1, int c) {
  if ((b0 + 256) == b1) a = c;
  else if (b0 > b1) a = imax(a - 64, 0);
  return a;
}
static inline int calc_lowcomp(int a, int b0, int b1, int bin) {
  if (bin < 7) return calc_lowcomp1(a, b0, b1, 384);
  if (bin < 20) return calc_lowcomp1(a, b0, b1, 320);
  return imax(a - 128, 0);
}
// A channel's bit allocation pointers from its exponents (slot ch of r, bins start..end): 0, or
// -1 where the delta bit allocation goes past the bands (damaged data)
static int bit_alloc(Run& r, int ch, int start, int end, int is_lfe, int sr_shift, int sr_code, const U8* tab) {
  S16 psd[NBIN], band_psd[50], mask[50], excite[50];
  int bin, band;
  if (end <= start) return 0;
  if (r.snroffst[ch] == -960) {  // (ffmpeg: all zero)
    memset(r.bap[ch], 0, NBIN);
    return 0;
  }
  for (bin = start; bin < end; bin++) psd[bin] = (S16)(3072 - (r.exps[ch][bin] << 7));
  bin = start;
  band = bin_to_band_tab[start];
  do {
    int v = psd[bin++];
    const int band_end = imin(band_start_tab[band + 1], end);
    for (; bin < band_end; bin++) {
      const int max = imax(v, psd[bin]);
      const int adr = imin(max - ((v + psd[bin] + 1) >> 1), 255);
      v = max + log_add_tab[adr];
    }
    band_psd[band++] = (S16)v;
  } while (end > band_start_tab[band]);
  // the excitation, the masking curve
  const int band_start = bin_to_band_tab[start], band_end = bin_to_band_tab[end - 1] + 1;
  const int fast_gain = r.fgain[ch];
  int lowcomp, fastleak = 0, slowleak = 0, begin, end1;
  if (band_start == 0) {
    lowcomp = 0;
    lowcomp = calc_lowcomp1(lowcomp, band_psd[0], band_psd[1], 384);
    excite[0] = (S16)(band_psd[0] - fast_gain - lowcomp);
    lowcomp = calc_lowcomp1(lowcomp, band_psd[1], band_psd[2], 384);
    excite[1] = (S16)(band_psd[1] - fast_gain - lowcomp);
    begin = 7;
    for (band = 2; band < 7; band++) {
      if (!(is_lfe && band == 6)) lowcomp = calc_lowcomp1(lowcomp, band_psd[band], band_psd[band + 1], 384);
      fastleak = band_psd[band] - fast_gain;
      slowleak = band_psd[band] - r.sgain;
      excite[band] = (S16)(fastleak - lowcomp);
      if (!(is_lfe && band == 6)) {
        if (band_psd[band] <= band_psd[band + 1]) {
          begin = band + 1;
          break;
        }
      }
    }
    end1 = imin(band_end, 22);
    for (band = begin; band < end1; band++) {
      if (!(is_lfe && band == 6)) lowcomp = calc_lowcomp(lowcomp, band_psd[band], band_psd[band + 1], band);
      fastleak = imax(fastleak - r.fdecay, band_psd[band] - fast_gain);
      slowleak = imax(slowleak - r.sdecay, band_psd[band] - r.sgain);
      excite[band] = (S16)imax(fastleak - lowcomp, slowleak);
    }
    begin = 22;
  } else {  // the coupling channel
    begin = band_start;
    fastleak = (r.cplfleak << 8) + 768;
    slowleak = (r.cplsleak << 8) + 768;
  }
  for (band = begin; band < band_end; band++) {
    fastleak = imax(fastleak - r.fdecay, band_psd[band] - fast_gain);
    slowleak = imax(slowleak - r.sdecay, band_psd[band] - r.sgain);
    excite[band] = (S16)imax(fastleak, slowleak);
  }
  for (band = band_start; band < band_end; band++) {
    const int tmp = r.dbknee - band_psd[band];
    if (tmp > 0) excite[band] = (S16)(excite[band] + (tmp >> 2));
    mask[band] = (S16)imax(hearing_threshold_tab[band >> sr_shift][sr_code], excite[band]);
  }
  // the delta bit allocation
  if (r.deltbae[ch] == 0 || r.deltbae[ch] == 1) {
    band = band_start;
    for (int seg = 0; seg < r.deltnseg[ch]; seg++) {
      band += r.deltoffst[ch][seg];
      if (band >= 50 || r.deltlen[ch][seg] > 50 - band) return -1;
      const int delta = r.deltba[ch][seg] >= 4 ? (r.deltba[ch][seg] - 3) * 128 : (r.deltba[ch][seg] - 4) * 128;
      for (int i = 0; i < r.deltlen[ch][seg]; i++) {
        mask[band] = (S16)(mask[band] + delta);
        band++;
      }
    }
  }
  // the pointers
  bin = start;
  band = bin_to_band_tab[start];
  int band_end2;
  do {
    const int m = (imax(mask[band] - r.snroffst[ch] - r.floor, 0) & 0x1FE0) + r.floor;
    band_end2 = imin(band_start_tab[++band], end);
    for (; bin < band_end2; bin++) {
      int address = (psd[bin] - m) >> 5;
      address = address < 0 ? 0 : address > 63 ? 63 : address;
      r.bap[ch][bin] = tab[address];
    }
  } while (end > band_end2);
  return 0;
}

// --- the walk ----------------------------------------------------------------------------------------

// No callback (the walk alone)
struct NoBlockCb {
  void block(const Frame&, const Run&, int) {}
};

// A walk of one syncframe through its IO: reads the elements from the bits into the frame record
// (BitIn), or writes them (ac3tsv's BitOut), or at record level reads or writes the record's
// values as they are (ac3tsv's text, ac3pack's model): one walk, so that the directions can't
// disagree. As it goes it makes what the elements imply (the channels, the bands, the exponents,
// the bit allocation: Run), and after each block it calls cb.block() with them. IO::BITS: the
// elements as the bits have them (mantissa and exponent groups, the CRC words), else as the record
// has them (the values in the groups, a flag for each CRC that holds). pos counts the bits the
// elements take, at either level.
template <class IO, class CB = NoBlockCb>
struct Walk {
  IO& io;
  Frame& f;
  CB& cb;
  Run r;
  int pos;
  int gpos[3], gn[3], gv[3][3];  // the open mantissa groups of the block (bap 1, 2, 4): where (BitOut), how full, the values
  // where the walk is, for a model's contexts: the block (-1 the frame's header), the channel,
  // the bin and its bap; an exponent delta's strategy and the exponent before it
  int cur_blk, cur_ch, cur_bin, cur_bap, cur_es, cur_exp;

  Walk(IO& i, Frame& fr, CB& c) : io(i), f(fr), cb(c) {}

  bool bad(const char* why) {
    if (!f.err) f.err = why;
    return false;
  }
  bool u(int fid, int n, int& v) {
    if (!io.u(fid, n, v)) return bad(field_name[fid]);
    pos += n;
    return true;
  }
  bool s(int fid, int n, int& v) {
    if (!io.s(fid, n, v)) return bad(field_name[fid]);
    pos += n;
    return true;
  }
  // a record level value in lo..hi, which takes n bits in the bits
  bool rv(int fid, int lo, int hi, int n, int& v) {
    if (!io.r(fid, lo, hi, v)) return bad(field_name[fid]);
    pos += n;
    return true;
  }
  bool raw(int fid, int n, U8* b) {
    if (!io.raw(fid, n, b)) return bad(field_name[fid]);
    pos += n;
    return true;
  }
#define U(fid, n, v)                \
  do {                              \
    if (!u(F_##fid, n, v)) return false; \
  } while (0)
#define S(fid, n, v)                \
  do {                              \
    if (!s(F_##fid, n, v)) return false; \
  } while (0)

  // the whole frame: false where it can't be (f.err says why, f.err_blk where: -1 the header,
  // NBLK the trailer). Or in steps: begin(), header(), body().
  bool frame() {
    begin();
    return header() && body();
  }
  void begin() {
    memset(&r, 0, sizeof r);
    cur_blk = -1, cur_ch = cur_bin = cur_bap = cur_es = cur_exp = 0;
    if (g_ffmpeg) {
      memcpy(r.cplbndstrc, g_ff_cplbndstrc, sizeof r.cplbndstrc);
      memcpy(r.spxbndstrc, g_ff_spxbndstrc, sizeof r.spxbndstrc);
    }
    f.err = nullptr;
    f.err_blk = -1;
    f.odd = 0;
    pos = 16;  // (the sync word)
  }
  bool body() {
    for (int blk = 0; blk < f.nblk; blk++) {
      f.err_blk = blk;
      if (!block(blk)) return false;
      cb.block(f, r, blk);
    }
    f.err_blk = NBLK;
    return trailer();
  }

  // --- syncinfo, bsi -------------------------------------------------------------------------------
  bool header() {
    Bsi& b = f.b;
    io.open(K_H, 0);
    // the bsid tells AC-3 (0-10) from E-AC-3 (11-16): in the bits at bit 40 in either; at record
    // level first
    if (!IO::BITS) {
      U(bsid, 5, b.bsid);
    } else if (IO::READ) {
      if (io.end < 64) return bad("short");
      b.bsid = io.peek_bsid();
    }
    if (b.bsid > 16) return bad("bsid");
    f.eac3 = b.bsid > 10;
    if (!(f.eac3 ? bsi_eac3() : bsi_ac3())) return false;
    f.nfch = nfchans_tab[b.acmod];
    f.lfe = b.lfeon;
    f.nch = f.nfch + f.lfe;
    f.lfech = f.lfe ? f.nfch + 1 : -1;
    // the state of the walk as the frame starts
    for (int ch = 0; ch < NCH; ch++) r.firstcplcos[ch] = r.firstspxcos[ch] = 1;
    r.firstcplleak = f.eac3;
    for (int ch = 0; ch < NCH; ch++) r.deltbae[ch] = 2;  // (none)
    if (f.lfe) {
      r.strtmant[f.lfech] = 0;
      r.endmant[f.lfech] = 7;
      r.nexpgrps[f.lfech] = 2;
    }
    if (f.eac3) {
      if (!audfrm()) return false;
    } else {
      memset(&f.a, 0, sizeof f.a);
      for (int ch = 0; ch < NCH; ch++) r.chahtinu[ch] = 0;
    }
    return true;
  }

  bool bsi_ac3() {
    Bsi& b = f.b;
    if (IO::BITS) {
      U(crc1, 16, b.crc1);
    } else {  // (the flag isn't in the bits; the word, or its 16 bits, is)
      if (!rv(F_crc1ok, 0, 1, 0, b.crc1ok)) return false;
      if (!b.crc1ok) U(crc1, 16, b.crc1);
      else pos += 16;
    }
    U(fscod, 2, b.fscod);
    if (b.fscod == 3) return bad("fscod 3");
    U(frmsizecod, 6, b.frmsizecod);
    if (b.frmsizecod > 37) return bad("frmsizecod");
    if (IO::BITS) U(bsid, 5, b.bsid);  // (at record level, read first: header())
    U(bsmod, 3, b.bsmod);
    U(acmod, 3, b.acmod);
    if ((b.acmod & 1) && b.acmod != 1) U(cmixlev, 2, b.cmixlev);
    if (b.acmod & 4) U(surmixlev, 2, b.surmixlev);
    if (b.acmod == 2) U(dsurmod, 2, b.dsurmod);
    U(lfeon, 1, b.lfeon);
    for (int i = 0; i <= (b.acmod == 0); i++) {
      U(dialnorm, 5, b.dialnorm[i]);
      U(compre, 1, b.compre[i]);
      if (b.compre[i]) U(compr, 8, b.compr[i]);
      U(langcode, 1, b.langcode[i]);
      if (b.langcode[i]) U(langcod, 8, b.langcod[i]);
      U(audprodie, 1, b.audprodie[i]);
      if (b.audprodie[i]) {
        U(mixlevel, 5, b.mixlevel[i]);
        U(roomtyp, 2, b.roomtyp[i]);
      }
    }
    U(copyrightb, 1, b.copyrightb);
    U(origbs, 1, b.origbs);
    if (b.bsid == 6) {  // the alternate bit stream syntax (Annex D)
      U(xbsi1e, 1, b.xbsi1e);
      if (b.xbsi1e) {
        U(dmixmod, 2, b.dmixmod);
        U(ltrtcmixlev, 3, b.ltrtcmixlev);
        U(ltrtsurmixlev, 3, b.ltrtsurmixlev);
        U(lorocmixlev, 3, b.lorocmixlev);
        U(lorosurmixlev, 3, b.lorosurmixlev);
      }
      U(xbsi2e, 1, b.xbsi2e);
      if (b.xbsi2e) {
        U(dsurexmod, 2, b.dsurexmod);
        U(dheadphonmod, 2, b.dheadphonmod);
        U(adconvtyp, 1, b.adconvtyp[0]);
        U(xbsi2, 8, b.xbsi2);
        U(encinfo, 1, b.encinfo);
      }
    } else {
      U(timecod1e, 1, b.timecod1e);
      if (b.timecod1e) U(timecod1, 14, b.timecod1);
      U(timecod2e, 1, b.timecod2e);
      if (b.timecod2e) U(timecod2, 14, b.timecod2);
    }
    if (!addbsi()) return false;
    f.sr_code = b.fscod;
    f.sr_shift = imax(b.bsid, 8) - 8;
    f.srate = sample_rate_tab[b.fscod] >> f.sr_shift;
    f.nblk = 6;
    f.fbytes = frame_size_tab[b.frmsizecod][b.fscod] * 2;
    return true;
  }

  bool addbsi() {
    Bsi& b = f.b;
    U(addbsie, 1, b.addbsie);
    if (b.addbsie) {
      U(addbsil, 6, b.addbsil);
      if (!raw(F_addbsi, 8 * (b.addbsil + 1), b.addbsi)) return false;
    }
    return true;
  }

  bool bsi_eac3() {
    Bsi& b = f.b;
    U(strmtyp, 2, b.strmtyp);
    if (b.strmtyp == 3) return bad("strmtyp 3");
    U(substreamid, 3, b.substreamid);
    U(frmsiz, 11, b.frmsiz);
    U(fscod, 2, b.fscod);
    if (b.fscod == 3) {
      U(fscod2, 2, b.fscod2);
      if (b.fscod2 == 3) return bad("fscod2 3");
      b.numblkscod = 3;
    } else {
      U(numblkscod, 2, b.numblkscod);
    }
    U(acmod, 3, b.acmod);
    U(lfeon, 1, b.lfeon);
    if (IO::BITS) U(bsid, 5, b.bsid);  // (at record level, read first: header())
    f.nblk = eac3_blocks[b.numblkscod];
    f.fbytes = (b.frmsiz + 1) * 2;
    if (b.fscod == 3) {
      f.sr_code = b.fscod2;
      f.sr_shift = 1;
      f.srate = sample_rate_tab[b.fscod2] / 2;
    } else {
      f.sr_code = b.fscod;
      f.sr_shift = 0;
      f.srate = sample_rate_tab[b.fscod];
    }
    for (int i = 0; i <= (b.acmod == 0); i++) {
      U(dialnorm, 5, b.dialnorm[i]);
      U(compre, 1, b.compre[i]);
      if (b.compre[i]) U(compr, 8, b.compr[i]);
    }
    if (b.strmtyp == 1) {
      U(chanmape, 1, b.chanmape);
      if (b.chanmape) U(chanmap, 16, b.chanmap);
    }
    U(mixmdate, 1, b.mixmdate);
    if (b.mixmdate) {
      if (b.acmod > 2) U(dmixmod, 2, b.dmixmod);
      if ((b.acmod & 1) && b.acmod > 2) {
        U(ltrtcmixlev, 3, b.ltrtcmixlev);
        U(lorocmixlev, 3, b.lorocmixlev);
      }
      if (b.acmod & 4) {
        U(ltrtsurmixlev, 3, b.ltrtsurmixlev);
        U(lorosurmixlev, 3, b.lorosurmixlev);
      }
      if (b.lfeon) {
        U(lfemixlevcode, 1, b.lfemixlevcode);
        if (b.lfemixlevcode) U(lfemixlevcod, 5, b.lfemixlevcod);
      }
      if (b.strmtyp == 0) {
        for (int i = 0; i <= (b.acmod == 0); i++) {
          U(pgmscle, 1, b.pgmscle[i]);
          if (b.pgmscle[i]) U(pgmscl, 6, b.pgmscl[i]);
        }
        U(extpgmscle, 1, b.extpgmscle);
        if (b.extpgmscle) U(extpgmscl, 6, b.extpgmscl);
        U(mixdef, 2, b.mixdef);
        if (b.mixdef == 1) {
          U(premixcmpsel, 1, b.premixcmpsel);
          U(drcsrc, 1, b.drcsrc);
          U(premixcmpscl, 3, b.premixcmpscl);
        } else if (b.mixdef == 2) {
          U(mixdata12, 12, b.mixdata12);
        } else if (b.mixdef == 3) {
          U(mixdeflen, 5, b.mixdeflen);
          if (!raw(F_mixdata, 8 * (b.mixdeflen + 2), b.mixdata)) return false;
        }
        if (b.acmod < 2) {
          for (int i = 0; i <= (b.acmod == 0); i++) {
            U(paninfoe, 1, b.paninfoe[i]);
            if (b.paninfoe[i]) {
              U(panmean, 8, b.panmean[i]);
              U(paninfo, 6, b.paninfo[i]);
            }
          }
        }
        U(frmmixcfginfoe, 1, b.frmmixcfginfoe);
        if (b.frmmixcfginfoe) {
          if (b.numblkscod == 0) {
            U(blkmixcfginfo, 5, b.blkmixcfginfo[0]);
          } else {
            for (int blk = 0; blk < f.nblk; blk++) {
              U(blkmixcfginfoe, 1, b.blkmixcfginfoe[blk]);
              if (b.blkmixcfginfoe[blk]) U(blkmixcfginfo, 5, b.blkmixcfginfo[blk]);
            }
          }
        }
      }
    }
    U(infomdate, 1, b.infomdate);
    if (b.infomdate) {
      U(bsmod, 3, b.bsmod);
      U(copyrightb, 1, b.copyrightb);
      U(origbs, 1, b.origbs);
      if (b.acmod == 2) {
        U(dsurmod, 2, b.dsurmod);
        U(dheadphonmod, 2, b.dheadphonmod);
      }
      if (b.acmod >= 6) U(dsurexmod, 2, b.dsurexmod);
      for (int i = 0; i <= (b.acmod == 0); i++) {
        U(audprodie, 1, b.audprodie[i]);
        if (b.audprodie[i]) {
          U(mixlevel, 5, b.mixlevel[i]);
          U(roomtyp, 2, b.roomtyp[i]);
          U(adconvtyp, 1, b.adconvtyp[i]);
        }
      }
      if (b.fscod < 3) U(sourcefscod, 1, b.sourcefscod);
    }
    if (b.strmtyp == 0 && b.numblkscod != 3) U(convsync, 1, b.convsync);
    if (b.strmtyp == 2) {
      if (b.numblkscod == 3) b.blkid = 1;
      else U(blkid, 1, b.blkid);
      if (b.blkid) U(convfrmsizecod, 6, b.convfrmsizecod);
    }
    return addbsi();
  }

  // --- E-AC-3's audfrm -----------------------------------------------------------------------------
  bool audfrm() {
    Audfrm& a = f.a;
    const Bsi& b = f.b;
    const int nblk = f.nblk, nfch = f.nfch;
    io.open(K_A, 0);
    if (b.numblkscod == 3) {
      U(expstre, 1, a.expstre);
      U(ahte, 1, a.ahte);
    } else {
      a.expstre = 1;
      a.ahte = 0;
    }
    U(snroffststr, 2, a.snroffststr);
    U(transproce, 1, a.transproce);
    U(blkswe, 1, a.blkswe);
    U(dithflage, 1, a.dithflage);
    U(bamode, 1, a.bamode);
    U(frmfgaincode, 1, a.frmfgaincode);
    U(dbaflde, 1, a.dbaflde);
    U(skipflde, 1, a.skipflde);
    U(spxattene, 1, a.spxattene);
    int ncplblks = 0;
    if (b.acmod > 1) {
      a.cplstre[0] = 1;
      U(fcplinu, 1, a.cplinu[0]);
      for (int blk = 1; blk < nblk; blk++) {
        U(fcplstre, 1, a.cplstre[blk]);
        if (a.cplstre[blk]) U(fcplinu, 1, a.cplinu[blk]);
        else a.cplinu[blk] = a.cplinu[blk - 1];
      }
    } else {
      for (int blk = 0; blk < nblk; blk++) a.cplstre[blk] = a.cplinu[blk] = 0;
    }
    for (int blk = 0; blk < nblk; blk++) ncplblks += a.cplinu[blk];
    if (a.expstre) {
      for (int blk = 0; blk < nblk; blk++) {
        if (a.cplinu[blk]) U(cplexpstr, 2, f.expstr[blk][CPL]);
        else f.expstr[blk][CPL] = 0;
        for (int ch = 1; ch <= nfch; ch++) U(chexpstr, 2, f.expstr[blk][ch]);
      }
    } else {
      if (b.acmod > 1 && ncplblks > 0) {
        U(frmcplexpstr, 5, a.frmcplexpstr);
        for (int blk = 0; blk < 6; blk++) f.expstr[blk][CPL] = frm_expstr[a.frmcplexpstr][blk];
      } else {
        for (int blk = 0; blk < 6; blk++) f.expstr[blk][CPL] = 0;
      }
      for (int ch = 1; ch <= nfch; ch++) {
        U(frmchexpstr, 5, a.frmchexpstr[ch]);
        for (int blk = 0; blk < 6; blk++) f.expstr[blk][ch] = frm_expstr[a.frmchexpstr[ch]][blk];
      }
    }
    if (f.lfe)
      for (int blk = 0; blk < nblk; blk++) U(lfeexpstr, 1, f.expstr[blk][f.lfech]);
    if (b.strmtyp == 0) {
      if (b.numblkscod != 3) U(convexpstre, 1, a.convexpstre);
      else a.convexpstre = 1;
      if (a.convexpstre)
        for (int ch = 1; ch <= nfch; ch++) U(convexpstr, 5, a.convexpstr[ch]);
    }
    // (the defaults of what isn't sent are set where it isn't: so that a walk that writes the
    // record writes its values, not the defaults)
    for (int ch = 0; ch < NCH; ch++) r.chahtinu[ch] = 0;
    if (!a.ahte) {
      a.cplahtinu = a.lfeahtinu = 0;
      for (int ch = 1; ch <= nfch; ch++) a.chahtinu[ch] = 0;
    } else {  // (6 blocks)
      // the regions: how many times the exponents (and for coupling the strategy) are sent
      int ncplregs = 0, nlferegs = 0;
      for (int blk = 0; blk < 6; blk++) ncplregs += a.cplstre[blk] || (a.cplinu[blk] && f.expstr[blk][CPL]);
      if (g_ffmpeg) {  // (ffmpeg: blocks 1-5 reuse)
        ncplregs = 1;
        for (int blk = 1; blk < 6; blk++)
          if (f.expstr[blk][CPL] || a.cplstre[blk]) ncplregs = 2;
      }
      if (ncplblks == 6 && ncplregs == 1) U(cplahtinu, 1, a.cplahtinu);
      else a.cplahtinu = 0;
      for (int ch = 1; ch <= nfch; ch++) {
        int nchregs = 0;
        for (int blk = 0; blk < 6; blk++) nchregs += f.expstr[blk][ch] != EXP_REUSE;
        if (g_ffmpeg) {
          nchregs = 1;
          for (int blk = 1; blk < 6; blk++)
            if (f.expstr[blk][ch]) nchregs = 2;
        }
        if (nchregs == 1) U(chahtinu, 1, a.chahtinu[ch]);
        else a.chahtinu[ch] = 0;
      }
      if (f.lfe) {
        for (int blk = 0; blk < 6; blk++) nlferegs += f.expstr[blk][f.lfech] != EXP_REUSE;
        if (g_ffmpeg) {
          nlferegs = 1;
          for (int blk = 1; blk < 6; blk++)
            if (f.expstr[blk][f.lfech]) nlferegs = 2;
        }
        if (nlferegs == 1) U(lfeahtinu, 1, a.lfeahtinu);
        else a.lfeahtinu = 0;
      } else {
        a.lfeahtinu = 0;
      }
      r.chahtinu[CPL] = a.cplahtinu;
      for (int ch = 1; ch <= nfch; ch++) r.chahtinu[ch] = a.chahtinu[ch];
      if (f.lfe) r.chahtinu[f.lfech] = a.lfeahtinu;
    }
    if (a.snroffststr == 0) {
      U(frmcsnroffst, 6, a.frmcsnroffst);
      U(frmfsnroffst, 4, a.frmfsnroffst);
      const int snr = ((a.frmcsnroffst - 15) * 16 + a.frmfsnroffst) * 4;
      for (int ch = 0; ch < NCH; ch++) r.snroffst[ch] = snr;
    }
    if (a.transproce) {
      for (int ch = 1; ch <= nfch; ch++) {
        U(chintransproc, 1, a.chintransproc[ch]);
        if (a.chintransproc[ch]) {
          U(transprocloc, 10, a.transprocloc[ch]);
          U(transproclen, 8, a.transproclen[ch]);
        }
      }
    }
    if (a.spxattene) {
      for (int ch = 1; ch <= nfch; ch++) {
        U(chinspxatten, 1, a.chinspxatten[ch]);
        if (a.chinspxatten[ch]) U(spxattencod, 5, a.spxattencod[ch]);
      }
    } else {
      for (int ch = 1; ch <= nfch; ch++) a.chinspxatten[ch] = 0;
    }
    if (b.numblkscod != 0) U(blkstrtinfoe, 1, a.blkstrtinfoe);
    else a.blkstrtinfoe = 0;
    if (a.blkstrtinfoe) {
      // (nblkstrtbits = (blocks - 1) * (4 + ceil(log2(words))), as ffmpeg has it)
      int lg = 0;
      for (int v = f.fbytes - 2; v > 1; v >>= 1) lg++;
      a.nblkstrtbits = (nblk - 1) * (4 + lg);
      if (!raw(F_blkstrtinfo, a.nblkstrtbits, a.blkstrtinfo)) return false;
    }
    // the bit allocation's defaults without bamode, and the fast gains without frmfgaincode
    if (!a.bamode) set_ba(2, 1, 1, 2, 7);
    for (int ch = 0; ch < NCH; ch++) r.fgain[ch] = fast_gain_tab[4];
    return true;
  }

  void set_ba(int sdcycod, int fdcycod, int sgaincod, int dbpbcod, int floorcod) {
    r.sdecay = slow_decay_tab[sdcycod] >> f.sr_shift;
    r.fdecay = fast_decay_tab[fdcycod] >> f.sr_shift;
    r.sgain = slow_gain_tab[sgaincod];
    r.dbknee = db_per_bit_tab[dbpbcod];
    r.floor = floor_tab[floorcod];
  }

  // a banding structure: the subbands begin..end (12 bins each), struc[sb] 1 where subband sb goes
  // with the one before; the number of bands and their sizes
  static int bands(const int* struc, int begin, int end, int* sz) {
    int n = 0;
    sz[0] = 12;
    for (int sb = begin + 1; sb < end; sb++) {
      if (struc[sb]) sz[n] += 12;
      else sz[++n] = 12;
    }
    return n + 1;
  }

  // --- an audio block ------------------------------------------------------------------------------
  bool block(int blk) {
    Blk& k = f.k[blk];
    const Audfrm& a = f.a;
    const Bsi& b = f.b;
    const int nfch = f.nfch, eac3 = f.eac3, acmod = b.acmod;
    int ch;
    cur_blk = blk;
    io.open(K_B, blk);
    for (ch = 1; ch <= nfch; ch++) {
      if (!eac3 || a.blkswe) U(blksw, 1, k.blksw[ch]);
      else k.blksw[ch] = 0;
    }
    for (ch = 1; ch <= nfch; ch++) {
      if (!eac3 || a.dithflage) U(dithflag, 1, k.dithflag[ch]);
      else k.dithflag[ch] = 1;
    }
    for (int i = 0; i <= (acmod == 0); i++) {
      U(dynrnge, 1, k.dynrnge[i]);
      if (k.dynrnge[i]) U(dynrng, 8, k.dynrng[i]);
    }
    // spectral extension: its strategy, its coordinates
    if (eac3) {
      if (blk == 0) k.spxstre = 1;
      else U(spxstre, 1, k.spxstre);
      if (k.spxstre) {
        U(spxinu, 1, k.spxinu);
        r.spxinu = k.spxinu;
        if (k.spxinu) {
          if (acmod == 1) r.chinspx[1] = 1;
          else
            for (ch = 1; ch <= nfch; ch++) {
              U(chinspx, 1, k.chinspx[ch]);
              r.chinspx[ch] = k.chinspx[ch];
            }
          U(spxstrtf, 2, k.spxstrtf);
          U(spxbegf, 3, k.spxbegf);
          U(spxendf, 3, k.spxendf);
          const int begin = k.spxbegf < 6 ? k.spxbegf + 2 : 2 * k.spxbegf - 3;
          const int end = k.spxendf < 3 ? k.spxendf + 5 : 2 * k.spxendf + 3;
          r.spx_begin = begin;
          r.spx_end = end;
          r.spx_dst_start = k.spxstrtf * 12 + 25;
          r.spx_src_start = begin * 12 + 25;
          r.spx_dst_end = end * 12 + 25;
          if (begin >= end) return bad("spx range");
          if (r.spx_dst_start >= r.spx_src_start) return bad("spx copy start");
          U(spxbndstrce, 1, k.spxbndstrce);
          if (g_ffmpeg ? blk == 0 : !r.spx_banded)
            for (int sb = 0; sb < NSPXSB; sb++) r.spxbndstrc[sb] = default_spx_band_struct[sb];
          r.spx_banded = 1;
          if (k.spxbndstrce)
            for (int sb = begin + 1; sb < end; sb++) {
              U(spxbndstrc, 1, k.spxbndstrc[sb]);
              r.spxbndstrc[sb] = k.spxbndstrc[sb];
            }
          r.nspxbnd = bands(r.spxbndstrc, begin, end, r.spxbndsz);
          if (g_ffmpeg) memcpy(g_ff_spxbndstrc, r.spxbndstrc, sizeof r.spxbndstrc);
        } else {
          for (ch = 1; ch <= nfch; ch++) r.chinspx[ch] = 0, r.firstspxcos[ch] = 1;
        }
      }
    }
    if (!eac3 || !r.spxinu) {
      r.spxinu = 0;
      for (ch = 1; ch <= nfch; ch++) r.chinspx[ch] = 0, r.firstspxcos[ch] = 1;
    }
    if (r.spxinu) {
      for (ch = 1; ch <= nfch; ch++) {
        if (!r.chinspx[ch]) {
          k.spxcoe[ch] = 0;
          r.firstspxcos[ch] = 1;
          continue;
        }
        if (r.firstspxcos[ch]) k.spxcoe[ch] = 1;
        else U(spxcoe, 1, k.spxcoe[ch]);
        r.firstspxcos[ch] = 0;
        if (k.spxcoe[ch]) {
          U(spxblnd, 5, k.spxblnd[ch]);
          U(mstrspxco, 2, k.mstrspxco[ch]);
          for (int bnd = 0; bnd < r.nspxbnd; bnd++) {
            U(spxcoexp, 4, k.spxcoexp[ch][bnd]);
            U(spxcomant, 2, k.spxcomant[ch][bnd]);
          }
        }
      }
    }
    // coupling: its strategy, its coordinates
    int cplstre;
    if (eac3) cplstre = a.cplstre[blk];
    else {
      U(cplstre, 1, k.cplstre);
      cplstre = k.cplstre;
    }
    if (cplstre) {
      if (eac3) r.cplinu = a.cplinu[blk];
      else {
        U(cplinu, 1, k.cplinu);
        r.cplinu = k.cplinu;
      }
      if (r.cplinu) {
        if (acmod < 2) return bad("coupling in mono or dual mono");
        if (eac3) {
          U(ecplinu, 1, k.ecplinu);
          if (k.ecplinu) return bad("enhanced coupling");
        }
        if (eac3 && acmod == 2) r.chincpl[1] = r.chincpl[2] = 1;
        else
          for (ch = 1; ch <= nfch; ch++) {
            U(chincpl, 1, k.chincpl[ch]);
            r.chincpl[ch] = k.chincpl[ch];
          }
        if (acmod == 2) {
          U(phsflginu, 1, k.phsflginu);
          r.phsflginu = k.phsflginu;
        }
        U(cplbegf, 4, k.cplbegf);
        int end;
        if (eac3 && r.spxinu) end = r.spx_begin - 1;  // (cplendf = spx_begin_subbnd - 4: may be negative)
        else {
          U(cplendf, 4, k.cplendf);
          end = k.cplendf + 3;
        }
        if (k.cplbegf >= end) return bad("coupling range");
        r.cplstrt = k.cplbegf * 12 + 37;
        r.cplend = end * 12 + 37;
        if (g_ffmpeg ? blk == 0 : !r.cpl_banded)
          for (int sb = 0; sb < NCPLSB; sb++) r.cplbndstrc[sb] = default_cpl_band_struct[sb];
        r.cpl_banded = 1;
        int sent = 1;
        if (eac3) {
          U(cplbndstrce, 1, k.cplbndstrce);
          sent = k.cplbndstrce;
        }
        if (sent)
          for (int sb = k.cplbegf + 1; sb < end; sb++) {
            U(cplbndstrc, 1, k.cplbndstrc[sb]);
            r.cplbndstrc[sb] = k.cplbndstrc[sb];
          }
        r.ncplbnd = bands(r.cplbndstrc, k.cplbegf, end, r.cplbndsz);
        if (g_ffmpeg) memcpy(g_ff_cplbndstrc, r.cplbndstrc, sizeof r.cplbndstrc);
      } else {
        for (ch = 1; ch <= nfch; ch++) r.chincpl[ch] = 0, r.firstcplcos[ch] = 1;
        r.firstcplleak = eac3;
        r.phsflginu = 0;
      }
    } else if (!eac3) {
      if (blk == 0) return bad("no coupling strategy in block 0");
    } else {
      r.cplinu = a.cplinu[blk];
    }
    if (r.cplinu) {
      int any = 0;
      for (ch = 1; ch <= nfch; ch++) {
        if (!r.chincpl[ch]) {
          k.cplcoe[ch] = 0;
          r.firstcplcos[ch] = 1;
          continue;
        }
        if (eac3 && r.firstcplcos[ch]) k.cplcoe[ch] = 1;
        else U(cplcoe, 1, k.cplcoe[ch]);
        if (k.cplcoe[ch]) {
          r.firstcplcos[ch] = 0;
          any = 1;
          U(mstrcplco, 2, k.mstrcplco[ch]);
          for (int bnd = 0; bnd < r.ncplbnd; bnd++) {
            U(cplcoexp, 4, k.cplcoexp[ch][bnd]);
            U(cplcomant, 4, k.cplcomant[ch][bnd]);
          }
        } else if (blk == 0) {
          return bad("no coupling coordinates in block 0");
        }
      }
      if (acmod == 2 && any) {
        for (int bnd = 0; bnd < r.ncplbnd; bnd++) {
          if (r.phsflginu) U(phsflg, 1, k.phsflg[bnd]);
          else k.phsflg[bnd] = 0;
        }
      }
    }
    // rematrixing
    if (acmod == 2) {
      if (eac3 && blk == 0) k.rematstr = 1;
      else U(rematstr, 1, k.rematstr);
      if (k.rematstr) {
        r.nrematbd = 4;
        if (r.cplinu && r.cplstrt <= 61) r.nrematbd -= 1 + (r.cplstrt == 37);
        else if (r.spxinu && r.spx_src_start <= 61) r.nrematbd--;
        for (int bnd = 0; bnd < r.nrematbd; bnd++) {
          U(rematflg, 1, k.rematflg[bnd]);
          r.rematflg[bnd] = k.rematflg[bnd];
        }
      } else if (blk == 0) {
        r.nrematbd = 0;
      }
    }
    // exponent strategies (AC-3), bandwidths
    const int first = !r.cplinu;
    if (!eac3)
      for (ch = first; ch <= f.nch; ch++) U(expstr, ch == f.lfech ? 1 : 2, f.expstr[blk][ch]);
    for (ch = 1; ch <= nfch; ch++) {
      r.strtmant[ch] = 0;
      if (f.expstr[blk][ch] != EXP_REUSE) {
        int end;
        if (r.chincpl[ch]) end = r.cplstrt;
        else if (r.chinspx[ch]) end = r.spx_src_start;
        else {
          U(chbwcod, 6, k.chbwcod[ch]);
          if (k.chbwcod[ch] > 60) return bad("chbwcod");
          end = k.chbwcod[ch] * 3 + 73;
        }
        r.endmant[ch] = end;
        const int gs = 3 << (f.expstr[blk][ch] - 1);
        r.nexpgrps[ch] = (end + gs - 4) / gs;
      }
    }
    if (r.cplinu) {
      r.strtmant[CPL] = r.cplstrt;
      r.endmant[CPL] = r.cplend;
      if (f.expstr[blk][CPL] != EXP_REUSE) r.nexpgrps[CPL] = (r.cplend - r.cplstrt) / (3 << (f.expstr[blk][CPL] - 1));
    }
    // exponents
    for (ch = first; ch <= f.nch; ch++) {
      if (f.expstr[blk][ch] == EXP_REUSE) continue;
      if (!exponents(k, blk, ch)) return false;
      if (ch != CPL && ch != f.lfech) U(gainrng, 2, k.gainrng[ch]);
    }
    // bit allocation parameters
    if (!eac3 || a.bamode) {
      U(baie, 1, k.baie);
      if (k.baie) {
        U(sdcycod, 2, k.sdcycod);
        U(fdcycod, 2, k.fdcycod);
        U(sgaincod, 2, k.sgaincod);
        U(dbpbcod, 2, k.dbpbcod);
        U(floorcod, 3, k.floorcod);
        set_ba(k.sdcycod, k.fdcycod, k.sgaincod, k.dbpbcod, k.floorcod);
      } else if (blk == 0) {
        return bad("no bit allocation parameters in block 0");
      }
    }
    if (!snr_offsets(k, blk)) return false;
    if (eac3 && b.strmtyp == 0) {
      U(convsnroffste, 1, k.convsnroffste);
      if (k.convsnroffste) U(convsnroffst, 10, k.convsnroffst);
    }
    if (r.cplinu) {
      if (r.firstcplleak) k.cplleake = 1;
      else U(cplleake, 1, k.cplleake);
      r.firstcplleak = 0;
      if (k.cplleake) {
        U(cplfleak, 3, k.cplfleak);
        U(cplsleak, 3, k.cplsleak);
        r.cplfleak = k.cplfleak;
        r.cplsleak = k.cplsleak;
      } else if (!eac3 && blk == 0) {
        return bad("no coupling leak in block 0");
      }
    }
    // delta bit allocation
    if (!eac3 || a.dbaflde) U(deltbaie, 1, k.deltbaie);
    else k.deltbaie = 0;
    if (k.deltbaie) {
      for (ch = first; ch <= nfch; ch++) {
        U(deltbae, 2, k.deltbae[ch]);
        if (k.deltbae[ch] == 3) return bad("deltbae 3");
        r.deltbae[ch] = k.deltbae[ch];
      }
      for (ch = first; ch <= nfch; ch++) {
        if (k.deltbae[ch] != 1) continue;
        U(deltnseg, 3, k.deltnseg[ch]);
        r.deltnseg[ch] = k.deltnseg[ch] + 1;
        for (int seg = 0; seg <= k.deltnseg[ch]; seg++) {
          U(deltoffst, 5, k.deltoffst[ch][seg]);
          U(deltlen, 4, k.deltlen[ch][seg]);
          U(deltba, 3, k.deltba[ch][seg]);
          r.deltoffst[ch][seg] = k.deltoffst[ch][seg];
          r.deltlen[ch][seg] = k.deltlen[ch][seg];
          r.deltba[ch][seg] = k.deltba[ch][seg];
        }
      }
    } else if (blk == 0) {
      for (ch = 0; ch < NCH; ch++) r.deltbae[ch] = 2;
    }
    // unused dummy data
    if (!eac3 || a.skipflde) U(skiple, 1, k.skiple);
    else k.skiple = 0;
    if (k.skiple) {
      U(skipl, 9, k.skipl);
      if (!raw(F_skipfld, 8 * k.skipl, k.skipfld)) return false;
    }
    // the bit allocation, then the mantissas
    for (ch = first; ch <= f.nch; ch++) {
      const U8* tab = r.chahtinu[ch] ? hebap_tab : bap_tab;
      if (bit_alloc(r, ch, r.strtmant[ch], r.endmant[ch], ch == f.lfech, f.sr_shift, f.sr_code, tab) < 0)
        return bad("delta bit allocation");
    }
    return mantissas(k, blk);
  }

  // a channel's exponents (A/52 section 7.1.3)
  bool exponents(Blk& k, int blk, int ch) {
    const int es = f.expstr[blk][ch], ng = r.nexpgrps[ch], gsz = 1 << (es - 1);
    io.open(K_E, ch);
    U(absexp, 4, k.absexp[ch]);
    int prev = k.absexp[ch] << (ch == CPL);
    int j = r.strtmant[ch];
    if (ch != CPL) r.exps[ch][j++] = (S8)prev;
    if (3 * ng > NBIN) return bad("exponent groups");
    for (int g = 0; g < ng; g++) {
      U8* d = &k.dexp[ch][3 * g];
      if (IO::BITS) {
        int c;
        if (IO::READ) {
          U(exps, 7, c);
          d[0] = (U8)(c / 25), d[1] = (U8)(c % 25 / 5), d[2] = (U8)(c % 5);
        } else {
          if (d[0] > 4 || d[1] > 4 || d[2] > 4) return bad("exps");
          c = d[0] * 25 + d[1] * 5 + d[2];
          U(exps, 7, c);
        }
      } else {
        cur_ch = ch, cur_es = es, cur_exp = prev;
        for (int i = 0; i < 3; i++) {  // (the group's 7 bits counted with its first)
          int v = d[i];
          cur_bin = j + i * gsz;
          if (!rv(F_dexp, 0, 4, i ? 0 : 7, v)) return false;
          d[i] = (U8)v;
          cur_exp += v - 2;
        }
      }
      if (d[0] * 25 + d[1] * 5 + d[2] >= 125) return bad("exponent group 125-127");
      for (int i = 0; i < 3; i++) {
        prev += d[i] - 2;
        if ((unsigned)prev > 24) return bad("exponent out of range");
        for (int t = 0; t < gsz && j < NBIN; t++) r.exps[ch][j++] = (S8)prev;
      }
    }
    return true;
  }

  // SNR offsets and fast gains
  bool snr_offsets(Blk& k, int blk) {
    const Audfrm& a = f.a;
    const int first = !r.cplinu, eac3 = f.eac3;
    int ch;
    if (!eac3) {
      U(snroffste, 1, k.snroffste);
      if (k.snroffste) {
        U(csnroffst, 6, k.csnroffst);
        for (ch = first; ch <= f.nch; ch++) {
          U(fsnroffst, 4, k.fsnroffst[ch]);
          U(fgaincod, 3, k.fgaincod[ch]);
          r.snroffst[ch] = ((k.csnroffst - 15) * 16 + k.fsnroffst[ch]) * 4;
          r.fgain[ch] = fast_gain_tab[k.fgaincod[ch]];
        }
      } else if (blk == 0) {
        return bad("no SNR offsets in block 0");
      }
      return true;
    }
    if (a.snroffststr) {
      if (g_ffmpeg) {  // (ffmpeg: snroffste sent in block 0, and in no other)
        if (blk == 0) U(snroffste, 1, k.snroffste);
        else k.snroffste = 0;
      } else if (blk == 0) {
        k.snroffste = 1;
      } else {
        U(snroffste, 1, k.snroffste);
      }
      if (k.snroffste) {
        U(csnroffst, 6, k.csnroffst);
        for (ch = first; ch <= f.nch; ch++) {
          if (ch == first || a.snroffststr == 2) U(fsnroffst, 4, k.fsnroffst[ch]);
          else k.fsnroffst[ch] = k.fsnroffst[first];
          r.snroffst[ch] = ((k.csnroffst - 15) * 16 + k.fsnroffst[ch]) * 4;
        }
      }
    }
    if (a.frmfgaincode) U(fgaincode, 1, k.fgaincode);
    else k.fgaincode = 0;
    if (k.fgaincode) {
      for (ch = first; ch <= f.nch; ch++) {
        U(fgaincod, 3, k.fgaincod[ch]);
        r.fgain[ch] = fast_gain_tab[k.fgaincod[ch]];
      }
    }
    return true;
  }

  // --- mantissas -----------------------------------------------------------------------------------

  // The bits of a group of mantissas (bap 1, 2, 4: g 0, 1, 2) as its first is met, and its values
  // as all are: read, they are its code split (the first can be past the levels, codes 27-31,
  // 125-127, 121-127: f.odd); written, the code's place is kept and the code put there as the
  // group is full or the block ends
  static int gsize(int g) { return g == 2 ? 2 : 3; }
  static int glevels(int g) { return g == 0 ? 3 : g == 1 ? 5 : 11; }
  static int gbits(int g) { return g == 0 ? 5 : 7; }
  // a group's first value, at most: the largest code's (31 / 9, 127 / 25, 127 / 11)
  static int gfirst_max(int g) { return ((1 << gbits(g)) - 1) / pw(glevels(g), gsize(g) - 1); }
  bool group_put(int g) {  // (BitOut: the full group's code into its place)
    const int L = glevels(g);
    int c = 0;
    for (int i = 0; i < gsize(g); i++) c = c * L + gv[g][i];
    if (c >= 1 << gbits(g)) return bad("mantissa group");
    io.put_at(gpos[g], gbits(g), c);
    return true;
  }
  bool mant(int ch, int bin, int bap) {
    S16& q = f.k[cur_blk].q[ch][bin];
    if (bap == 0) return true;
    cur_ch = ch, cur_bin = bin, cur_bap = bap;
    if (bap == 1 || bap == 2 || bap == 4) {
      const int g = bap == 1 ? 0 : bap == 2 ? 1 : 2, L = glevels(g), h = (L - 1) / 2, n = gsize(g);
      if (!IO::BITS) {
        int v = q;
        if (!rv(F_q, -h, gn[g] == 0 ? gfirst_max(g) - h : h, gn[g] == 0 ? gbits(g) : 0, v)) return false;
        q = (S16)v;
        if (gn[g] == 0 && v > h) f.odd = 1;
        gn[g] = (gn[g] + 1) % n;
        return true;
      }
      if (IO::READ) {
        if (gn[g] == 0) {
          int c;
          if (!u(g == 0 ? F_mgrp1 : g == 1 ? F_mgrp2 : F_mgrp4, gbits(g), c)) return false;
          for (int i = n - 1; i >= 0; i--) gv[g][i] = c % L, c /= L;
          gv[g][0] += c * L;  // (codes past L^n: the first value past the levels)
          if (gv[g][0] >= L) f.odd = 1;
        }
        q = (S16)(gv[g][gn[g]] - h);
        gn[g] = (gn[g] + 1) % n;
        return true;
      }
      if (gn[g] == 0) {
        gpos[g] = io.pos;
        int c = 0;
        if (!u(g == 0 ? F_mgrp1 : g == 1 ? F_mgrp2 : F_mgrp4, gbits(g), c)) return false;  // (its place)
      }
      const int v = q + h;
      if (v < 0 || v > (gn[g] == 0 ? gfirst_max(g) : L - 1)) return bad("mantissa");
      gv[g][gn[g]] = v;
      if (++gn[g] == n) {
        gn[g] = 0;
        if (!group_put(g)) return false;
      }
      return true;
    }
    int v = q;
    if (bap == 3 || bap == 5) {  // 7 and 15 levels in 3 and 4 bits (codes 7, 15: f.odd)
      const int h = bap == 3 ? 3 : 7, nb = bap == 3 ? 3 : 4;
      if (!IO::BITS) {
        if (!rv(F_q, -h, h + 1, nb, v)) return false;
      } else {
        v += h;
        if (!u(F_mant, nb, v)) return false;
        v -= h;
      }
      if (v > h) f.odd = 1;
    } else {  // 6-15: two's complement
      const int nb = quant_bits[bap];
      if (!IO::BITS) {
        if (!rv(F_q, -(1 << (nb - 1)), (1 << (nb - 1)) - 1, nb, v)) return false;
      } else if (!s(F_mant, nb, v)) {
        return false;
      }
    }
    q = (S16)v;
    return true;
  }
  static int pw(int b, int e) {
    int v = 1;
    while (e-- > 0) v *= b;
    return v;
  }
  // the block's last groups: the values past its last mantissas
  bool groups_end(Blk& k) {
    for (int g = 0; g < 3; g++) {
      const int L = glevels(g);
      for (int i = 0; gn[g] && gn[g] < gsize(g); i++) {
        int v = k.qfill[g][i];
        if (!IO::BITS) {
          if (!rv(F_qfill, 0, L - 1, 0, v)) return false;
          k.qfill[g][i] = v;
        } else if (IO::READ) {
          k.qfill[g][i] = gv[g][gn[g]];
        } else {
          if (v < 0 || v >= L) return bad("qfill");
          gv[g][gn[g]] = v;
        }
        if (++gn[g] == gsize(g)) {
          gn[g] = 0;
          if (IO::BITS && !IO::READ && !group_put(g)) return false;
        }
      }
    }
    return true;
  }
  bool mantissas(Blk& k, int blk) {
    cur_blk = blk;
    gn[0] = gn[1] = gn[2] = 0;
    int got_cpl = 0;
    for (int ch = 1; ch <= f.nch; ch++) {
      if (r.chahtinu[ch]) {
        if (blk == 0 && !aht(ch)) return false;
      } else {
        io.open(K_M, ch);
        io.baps(r.bap[ch], r.strtmant[ch], r.endmant[ch]);
        for (int bin = r.strtmant[ch]; bin < r.endmant[ch]; bin++)
          if (!mant(ch, bin, r.bap[ch][bin])) return false;
      }
      if (r.cplinu && r.chincpl[ch] && !got_cpl) {
        got_cpl = 1;
        if (r.chahtinu[CPL]) {
          if (blk == 0 && !aht(CPL)) return false;
        } else {
          io.open(K_M, CPL);
          io.baps(r.bap[CPL], r.cplstrt, r.cplend);
          for (int bin = r.cplstrt; bin < r.cplend; bin++)
            if (!mant(CPL, bin, r.bap[CPL][bin])) return false;
        }
      }
    }
    return groups_end(k);
  }

  // a gain of GAQ's mode 3, the n-th of its group (vals; gp where its code is, BitOut)
  bool gain3(S8& g, int n, int& gp, int* vals, int nb, int fid) {
    if (!IO::BITS) {
      int v = g;
      if (!rv(fid, 0, 2, nb, v)) return false;
      g = (S8)v;
      return true;
    }
    if (IO::READ) {
      if (n == 0) {
        int c;
        if (!u(F_gaqgrp, 5, c)) return false;
        if (c > 26) return bad("gaqgrp");
        vals[0] = c / 9, vals[1] = c % 9 / 3, vals[2] = c % 3;
      }
      g = (S8)vals[n];
      return true;
    }
    if (g < 0 || g > 2) return bad("gain");
    if (n == 0) {
      gp = io.pos;
      int c = 0;
      if (!u(F_gaqgrp, 5, c)) return false;
    }
    vals[n] = g;
    if (n == 2) io.put_at(gp, 5, vals[0] * 9 + vals[1] * 3 + vals[2]);
    return true;
  }

  // A channel's AHT data, for the six blocks (A/52 Annex E section 3.4)
  bool aht(int ch) {
    Aht& h = f.aht[ch];
    cur_ch = ch;
    io.open(K_Q, ch);
    io.baps(r.bap[ch], r.strtmant[ch], r.endmant[ch]);
    U(gaqmod, 2, h.gaqmod);
    const int endbap = h.gaqmod < 2 ? 12 : 17;
    const int start = r.strtmant[ch], end = r.endmant[ch];
    int bin;
    if (h.gaqmod == 1 || h.gaqmod == 2) {
      for (bin = start; bin < end; bin++)
        if (r.bap[ch][bin] > 7 && r.bap[ch][bin] < endbap) {
          int v = h.gain[bin];
          cur_bin = bin, cur_bap = r.bap[ch][bin];
          U(gaqgain, 1, v);
          h.gain[bin] = (S8)v;
        }
    } else if (h.gaqmod == 3) {  // three gains in 5 bits (codes 27-31: damaged data)
      int n = 0, gp = 0, vals[3] = {0, 0, 0};
      for (bin = start; bin < end; bin++) {
        if (!(r.bap[ch][bin] > 7 && r.bap[ch][bin] < 17)) continue;
        cur_bin = bin, cur_bap = r.bap[ch][bin];
        if (!gain3(h.gain[bin], n, gp, vals, n == 0 ? 5 : 0, F_gain)) return false;
        n = (n + 1) % 3;
      }
      for (int i = 0; n && n + i < 3; i++) {
        S8 v = (S8)h.gainfill[i];
        if (!gain3(v, n + i, gp, vals, 0, F_gainfill)) return false;
        h.gainfill[i] = v;
      }
    }
    for (bin = start; bin < end; bin++) {
      const int hebap = r.bap[ch][bin], bits = bits_vs_hebap[hebap];
      if (hebap == 0) continue;
      cur_bin = bin, cur_bap = hebap;
      if (hebap < 8) {
        int v = h.vq[bin];
        U(vq, bits, v);
        h.vq[bin] = (S16)v;
        continue;
      }
      int log_gain = 0;
      if (h.gaqmod && hebap < endbap) log_gain = h.gaqmod == 3 ? h.gain[bin] : h.gain[bin] << (h.gaqmod - 1);
      const int gb = bits - log_gain;
      cur_es = log_gain;  // (AHT: the gain, for a model)
      for (int blk = 0; blk < 6; blk++) {
        int v = h.m[bin][blk];
        cur_exp = blk;  // (and the block)
        S(gaqmant, gb, v);
        h.m[bin][blk] = (S16)v;
        if (log_gain && v == -(1 << (gb - 1))) {
          int w = h.big[bin][blk];
          S(gaqbig, bits - (2 - log_gain), w);
          h.big[bin][blk] = (S16)w;
        }
      }
    }
    return true;
  }

  // --- the trailer: what is after the last block, auxdatae, crcrsv (E-AC-3: encinfo), crc2 -------
  bool trailer() {
    const int fbits = f.fbytes * 8;
    io.open(K_T, 0);
    f.tailbits = fbits - 18 - pos;
    if (f.tailbits < 0) return bad("blocks past the frame");
    if (!raw(F_tail, f.tailbits, f.tail)) return false;
    U(auxdatae, 1, f.auxdatae);
    U(crcrsv, 1, f.crcrsv);
    if (IO::BITS) {
      U(crc2, 16, f.crc2);
    } else {
      if (!rv(F_crc2ok, 0, 1, 0, f.crc2ok)) return false;
      if (!f.crc2ok) U(crc2, 16, f.crc2);
      else pos += 16;
    }
    return true;
  }
#undef U
#undef S
};

// ===================================================================================================
// The bits written: BitOut, the walk's IO that writes the record's elements as bits
// ===================================================================================================

struct BitOut {
  enum { BITS = 1, READ = 0 };
  U8* p;
  int pos, end;  // bits
  // a frame of nbits into buf (zeroed, its sync word in place), the walk to start after the sync
  void init(U8* buf, int nbits) {
    p = buf;
    memset(p, 0, (size_t)(nbits + 7) / 8);
    p[0] = 0x0B;
    p[1] = 0x77;
    pos = 16;
    end = nbits;
  }
  void put(int at, int n, U32 v) {
    for (int i = n - 1; i >= 0; i--, at++) {
      U8& b = p[at >> 3];
      const int m = 0x80 >> (at & 7);
      b = (U8)(((v >> i) & 1) ? (b | m) : (b & ~m));
    }
  }
  bool u(int, int n, int& v) {
    if (pos + n > end || v < 0 || (n < 31 && v >= (1 << n))) return false;
    put(pos, n, (U32)v);
    pos += n;
    return true;
  }
  bool s(int, int n, int& v) {
    if (pos + n > end || n < 1 || v < -(1 << (n - 1)) || v >= (1 << (n - 1))) return false;
    put(pos, n, (U32)v & ((1u << n) - 1));
    pos += n;
    return true;
  }
  bool raw(int, int n, U8* b) {
    if (pos + n > end) return false;
    for (int i = 0; i < n; i++) put(pos + i, 1, (b[i >> 3] >> (7 - (i & 7))) & 1);
    pos += n;
    return true;
  }
  bool r(int, int, int, int&) { return false; }
  int peek_bsid() const { return 0; }
  void put_at(int at, int n, int v) { put(at, n, (U32)v); }
  void open(int, int) {}
  void baps(const U8*, int, int) {}
};

// The CRC words of a frame (n bytes at p) where the record has them hold, written in; and from a
// frame's bits whether they hold
static void put_crcs(U8* p, int n, const Frame& f) {
  if (!f.eac3 && f.b.crc1ok) {
    const int words = n / 2, f58 = 2 * ((words >> 1) + (words >> 3));
    const U16 c = crc1_for(crc16(0, p + 4, f58 - 4), (f58 - 4) * 8);
    p[2] = (U8)(c >> 8), p[3] = (U8)c;
  }
  if (f.crc2ok) {
    const int words = n / 2, from = f.eac3 ? 2 : 2 * ((words >> 1) + (words >> 3));
    const U16 c = crc16(0, p + from, n - 2 - from);
    p[n - 2] = (U8)(c >> 8), p[n - 1] = (U8)c;
  }
}
static void get_crcs(const U8* p, int n, Frame& f) {
  const int words = n / 2, f58 = 2 * ((words >> 1) + (words >> 3));
  if (!f.eac3) {
    f.b.crc1ok = crc16(0, p + 2, f58 - 2) == 0;
    f.crc2ok = crc16(0, p + f58, n - f58) == 0;
  } else {
    f.b.crc1ok = 0;
    f.crc2ok = crc16(0, p + 2, n - 2) == 0;
  }
}


// the size of the frame at p (n bytes there): 0 where it isn't one
static int frame_bytes(const U8* p, size_t n) {
  if (n < 6 || p[0] != 0x0B || p[1] != 0x77) return 0;
  const int bsid = p[5] >> 3;
  if (bsid <= 10) {
    const int fscod = p[4] >> 6, frmsizecod = p[4] & 63;
    if (fscod == 3 || frmsizecod > 37) return 0;
    return frame_size_tab[frmsizecod][fscod] * 2;
  }
  if (bsid > 16 || (p[2] >> 6) == 3) return 0;
  const int fs = ((((p[2] & 7) << 8) | p[3]) + 1) * 2;
  return fs < 8 ? 0 : fs;
}
static int frame_crc_ok(const U8* p, int n) {
  const int words = n / 2, f58 = 2 * ((words >> 1) + (words >> 3));
  if ((p[5] >> 3) > 10) return crc16(0, p + 2, n - 2) == 0;
  return crc16(0, p + 2, f58 - 2) == 0 && crc16(0, p + f58, n - f58) == 0;
}

