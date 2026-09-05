
#include "jpeg_marker.inc"

// Markers pjpg has a parser for.  Everything else that carries a length is
// still recognised and skipped correctly -- see init_lentag() in pjpg1.inc,
// which derives "has a length field" from the T.81 rule rather than a list
// (the old len2[] table omitted 11 length-bearing markers, so their payloads
// were scanned as data and any FF xx inside became a fabricated marker).
byte parsedtags[] = {
  M_SOF0, M_SOF1, M_SOF2, M_SOF3, M_SOF5, M_SOF6, M_SOF7,
  M_SOF9, M_SOF10,M_SOF11,M_SOF13,M_SOF14,M_SOF15,
  M_DHP,  M_EXP,  M_DNL,  M_DAC,
  M_DHT,  M_DQT,  M_SOS,  M_DRI,  M_COM,
  M_APP0, M_APP1, M_APP2, M_APP3, M_APP4,
  M_APP5, M_APP6, M_APP7, M_APP8, M_APP9,
  M_APP10,M_APP11,M_APP12,M_APP13,M_APP14,
  M_APP15,
0 };

// Fatal parse errors.  A parser records one and exits; pjpg1.inc then ends the
// coroutine with a PJPG_ERR yield instead of QUIT, and main() exits non-zero.
enum JPEG_ERR {
  E_OK = 0,
  E_TRUNCATED,   // input ended inside a segment
  E_BADLEN,      // segment length too small for the marker it claims to be
  E_BADSOF,      // bogus frame header
  E_BADSOS,      // bogus scan header
  E_BADDHT,      // bogus Huffman table definition
  E_BADDQT,      // bogus quantisation table definition
  E_BADDAC,      // bogus arithmetic conditioning
  E_BADEXIF,     // malformed Exif/TIFF structure
};

static const char* jpeg_err_name( uint e ) {
  switch( e ) {
  case E_OK:        return "ok";
  case E_TRUNCATED: return "truncated";
  case E_BADLEN:    return "bad segment length";
  case E_BADSOF:    return "bad frame header";
  case E_BADSOS:    return "bad scan header";
  case E_BADDHT:    return "bad Huffman table";
  case E_BADDQT:    return "bad quantisation table";
  case E_BADDAC:    return "bad arithmetic conditioning";
  case E_BADEXIF:   return "bad Exif structure";
  }
  return "unknown";
}

//---------------------------------------------------------

int jdiv_round_up( int a, int b ) { return (a+b-1) / b; }

int jround_up( int a, int b ) { a+=b-1; return a-(a % b); }

template <typename T> T jmax( T a, T b ) { return a>b ? a : b; }

template <typename T> T jmin( T a, T b ) { return a<b ? a : b; }

//---------------------------------------------------------

typedef short JBLOCK[64];
typedef JBLOCK  *JBLOCKROW;

struct JQUANT_TBL {
  word quantval[64];
  int sent_table; 
  int f_undef;

  JQUANT_TBL( void ) : f_undef(1) {}
};

struct JHUFF_TBL {
  byte bits[17];
  byte huffval[256];
  int sent_table;
};

enum J_COLOR_SPACE {
  JCS_UNKNOWN,
  JCS_GRAYSCALE,
  JCS_RGB,
  JCS_YCbCr,
  JCS_CMYK,
  JCS_YCCK
};


struct jpeg_component_info {

  int component_id;
  int component_index;
  int h_samp_factor;
  int v_samp_factor;
  int quant_tbl_no;

  int dc_tbl_no;
  int ac_tbl_no;

  uint width_in_blocks;
  uint height_in_blocks;

  int DCT_h_scaled_size;
  int DCT_v_scaled_size;

  uint downsampled_width;
  uint downsampled_height;

  int component_needed;

  int MCU_width;
  int MCU_height;
  int MCU_blocks;
  int MCU_sample_width;
  int last_col_width;
  int last_row_height;

  JQUANT_TBL quant_table;

  void * dct_table;
};

//#include "HUF/huffman.inc"

struct cinfo {

  uint image_width;
  uint image_height;
  int num_components;

  J_COLOR_SPACE jpeg_color_space;
  J_COLOR_SPACE out_color_space;

  int do_block_smoothing;

  uint output_width;
  uint output_height;
  int out_color_components;
  int output_components;

  int actual_number_of_colors;

  uint output_scanline;

  int input_scan_number;
  uint input_iMCU_row;

  int output_scan_number;
  uint output_iMCU_row;

//  int (*coef_bits)[64];
  int coef_bits[4][64];

  JQUANT_TBL quant_tbl[4];

  JHUFF_TBL dc_huff_tbl_ptrs[4];
  JHUFF_TBL ac_huff_tbl_ptrs[4];

  int data_precision;

  jpeg_component_info comp_info[256];

  int progressive_mode;
  int arith_code;

  byte arith_dc_L[16];
  byte arith_dc_U[16];
  byte arith_ac_K[16];

  uint restart_interval;

  int saw_JFIF_marker;

  byte JFIF_major_version;
  byte JFIF_minor_version;
  byte density_unit;
  word X_density;
  word Y_density;
  int saw_Adobe_marker;
  byte Adobe_transform;

  int CCIR601_sampling;

  int max_h_samp_factor;
  int max_v_samp_factor;

  int min_DCT_h_scaled_size;
  int min_DCT_v_scaled_size;

  uint total_iMCU_rows;

  byte sample_range_limit0[ (255+1) ];
  byte sample_range_limit[ 4*(255+1)+128 ];

  int comps_in_scan;
  jpeg_component_info* cur_comp_info[4];

  uint MCUs_per_row;
  uint MCU_rows_in_scan;

  int blocks_in_MCU;
  int MCU_membership[10];

  int Ss, Se, Ah, Al;

  int unread_marker;

  int has_multiple_scans;
  int eoi_reached;
  int inheaders;

  int saw_SOI;
  int saw_SOF;
  int next_restart_num;
  uint discarded_bytes;
  uint bytes_read;

//  huff_entropy_decoder huf;
};

const uint jpeg_zigzag_order[8][8] = {
  {  0,  1,  5,  6, 14, 15, 27, 28 },
  {  2,  4,  7, 13, 16, 26, 29, 42 },
  {  3,  8, 12, 17, 25, 30, 41, 43 },
  {  9, 11, 18, 24, 31, 40, 44, 53 },
  { 10, 19, 23, 32, 39, 45, 52, 54 },
  { 20, 22, 33, 38, 46, 51, 55, 60 },
  { 21, 34, 37, 47, 50, 56, 59, 61 },
  { 35, 36, 48, 49, 57, 58, 62, 63 }
};

const uint jpeg_natural_order[64+16] = {
  0,  1,  8, 16,  9,  2,  3, 10,
 17, 24, 32, 25, 18, 11,  4,  5,
 12, 19, 26, 33, 40, 48, 41, 34,
 27, 20, 13,  6,  7, 14, 21, 28,
 35, 42, 49, 56, 57, 50, 43, 36,
 29, 22, 15, 23, 30, 37, 44, 51,
 58, 59, 52, 45, 38, 31, 39, 46,
 53, 60, 61, 54, 47, 55, 62, 63,
 63, 63, 63, 63, 63, 63, 63, 63,
 63, 63, 63, 63, 63, 63, 63, 63
};

