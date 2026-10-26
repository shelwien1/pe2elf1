
//typedef unsigned long long int qword;
//typedef unsigned long  uint;
//typedef unsigned short word;
//typedef unsigned char  byte;
//#pragma pack(1)

struct header {
  uint emphasis        :2 ;  
  uint original        :1 ;  
  uint copyright       :1 ;  
  uint mode_ext        :2 ;  
  uint mode            :2 ;  
  uint privatex        :1 ;  
  uint padding         :1 ;  
  uint frequency       :2 ;  
  uint bitrate         :4 ;  
  uint protection      :1 ;  
  uint layer           :2 ;  
  uint ID              :1 ;  
  uint IDex            :1 ;  
  uint syncword        :11;  
};

struct gr_info
{
int p23len; // used by model
int backup_p23;
  int  part2_3_length;
  int  big_values;
  int  global_gain;
  int  scalefac_compress;
  int  window_switching_flag;
  int  block_type;
  int  mixed_block_flag;
  int  table_select[3];
  int  subblock_gain[3];
  int  region0_count;
  int  region1_count;
  int  preflag;
  int  scalefac_scale;
  int  count1table_select;
};

struct SideInfo
{
int backup_mdb;
  int  main_data_begin;
  int  private_bits;
  int  scfsi[2][4];
int p23sum;
int p23cleared; // invalid frame flag
  gr_info gi[2][2];
};

struct ScaleFac {
  word l[39];   
  word s[13][3];
};

// layer I/II frame contents, stored in place of si/SF/MC;
// subbands from the joint stereo bound up use alloc[0] and smp[0] for both channels
struct l12data {
  byte crcok;         // CRC was there and matched (so it's recomputed on write)
  byte alloc[2][32];  // bit allocation; layer II: index into the allocation table
  byte scfsi[2][32];  // layer II scalefactor select info
  byte scf[2][32][3]; // scalefactors, including the ones scfsi repeats; layer I: [0]
  word smp[2][32][36];// sample codes, grouped ones split up; layer I: [0..11]
};

struct mp3frame0 {
  int GrbLen;

  header   h;
  word     CRC;

  union {
    struct {
      SideInfo si;

      ScaleFac SF[2][2];

      short    MC[2][2][576];
    };
    l12data L;
  };

  byte Garbage[4096];
};

struct mp3frame : mp3frame0 {

  int GrbPtr, GrbPrev;
  int rframe1, wframe1;
  int ProcMode; 
  int fstframe() { return ProcMode?rframe1:wframe1; }
  int FW;

  #include "slen.inc"
  #include "mdct_len.inc"
  #include "calc.inc"
};

//#pragma pack()

