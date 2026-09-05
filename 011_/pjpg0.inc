
struct pjpg0 : cinfo {

  void* state;
  uint A,B,C,D;
  uint coro2_c;
  uint coro2_hl;

  uint tag_id;
  uint l_tag;
  uint f_SOS;  

  uint c_tag[256];

  void pjpg0_init( void ) {
    bzero(c_tag);
  }

  void coro2_init( void ) {
    state=0;
    f_SOS = 0;
  }

  int coro2_process( void ) {
    do_process();
    return state!=0;
  }


  #define coro2_get2E(f_LE,pos) ( coro2_get2(), pos+=2, (f_LE?(B<<8)|A:(A<<8)|B) )
  #define coro2_get4E(f_LE,pos) ( coro2_get4(), pos+=4, (f_LE?(D<<24)|(C<<16)|(B<<8)|A:(A<<24)|(B<<16)|(C<<8)|D) )

  #define coro2_getH() (byte(coro2_hl=coro2_get())>>4)
  #define coro2_getL() (byte(coro2_hl&0xF))

  void do_process( void ) {
//printf("!tag=%02X!\n", tag_id );
    switch( tag_id ) {
    case 0:      /*proc_MCU();*/ break;
    case M_DRI:  pa_DRI(); break;
    case M_DHT:  pa_DHT(); break;
    case M_DQT:  pa_DQT(); break;
    case M_SOS:  pa_SOS(); break;
    case M_APP0: pa_APP0(); break;
    case M_APP1: pa_APP1(); break;
    case M_SOF0: 
    case M_SOF1: pa_SOF(0,0); break; 
    case M_SOF2: pa_SOF(1,0); break; 
    case M_SOF9: pa_SOF(0,1); break; 
    case M_SOF10: pa_SOF(1,1); break; 
    }

    c_tag[tag_id] += (state==0);
    //yield(this,0);
    //state=0;
  }

  #include "tag_APP0.inc"
  #include "tag_APP1.inc"
  #include "tag_DHT.inc"
  #include "tag_SOF.inc"
  #include "tag_DQT.inc"
  #include "tag_SOS.inc"
  #include "tag_DRI.inc"

};
