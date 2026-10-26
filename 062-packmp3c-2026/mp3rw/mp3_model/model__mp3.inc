
#include "sh_mapping.inc"

#include "freq_sh_v1m.inc"

#include "MOD/sh_model_freq_h.inc"
#include "freq_tbl.inc"

#include "MOD/sh_modelu_h.inc"
#include "MOD/sh_modelw_h.inc"
#include "freq_paqmixer.inc"

#include "freq_node2.inc"
#include "freq_SSE1.inc"
#include "freq_SSE1_dbg.inc"

//#pragma pack(push)
//#pragma pack(8)

static const int m1_e_j_max = 576/2+256/2; //M_Sm1_j1/2

#include "MOD/sh_model_h.inc"
#include "MOD/sh_model_counts_h.inc"

int l_abs( int x ) {
  return (x>=0) ? x : -x;
}

template< int rc_ProcMode >
struct mp3model: RangeCoder_v1m<rc_ProcMode>, mp3frame, C_T {
  typedef RangeCoder_v1m<rc_ProcMode> RangeCoder;

  #include "MOD/sh_model_counts_p.inc"

  #include "model_header.inc"
  #include "model_scale1.inc"
  #include "model_scale2.inc"
  #include "model_part23.inc"
  #include "model_garbage.inc"

  #include "model_delta_counts.inc"
  #include "model_delta_regions.inc"
  #include "model_delta_range.inc"
  #include "model_delta.inc"

  #include "model_mdb.inc"

  RangeDelta m_rDelta[8];

  void InitModel( int lossy=0, int bitrate=0 ) {

    GrbPtr=GrbLen=GrbPrev=-1;
    rframe1=0; wframe1=0;

    InitSlen(); // scalefactor tables for mpeg2, already done at Input() or Output()

    Init_Header();
    Init_Scale1();
    Init_Scale2();
    Init_Part23();
    Init_Ancillary();
    Init_MDB();
    Init_Delta();
  }

  void Process( void ) {
    RangeCoder& rc = *this;

    if( rc.rc_ProcMode==1 ) {
/*
      memset( &h, 0, sizeof(h) );
      memset( &si,0, sizeof(si));
      memset( &SF,0, sizeof(SF));
      memset( &MC,0, sizeof(MC));
*/
      byte* p = (byte*)&h;
      byte* q = (byte*)&Garbage;
      memset( p, 0, q-p );
    }

    ProcessHeader();

    uint FrameTime = (h.ID?1152:576)*44100/spfreq[h.ID][h.IDex][h.frequency];

    h.ID ? ProcessScale1() : ProcessScale2();

    ProcessDelta(); // sets new CodeStart
    ProcessPart23();

    ProcessAncillary(); // prev frame's
    ProcessMDB();

    GrbPrev = CalcGarbageLen();
  }
	
  void FlushModel( void ) {
    ProcessAncillary();
    Init_Ancillary();
//    last_p23sum = -1;
  }

};

//#pragma pack(pop)

