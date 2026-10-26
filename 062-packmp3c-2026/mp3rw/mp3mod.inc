
#include "mp3_model/model__mp3.inc"

template< int f_DEC >
struct mp3mod : mp3model<f_DEC> {
  using mp3model<f_DEC>::InitModel;
  using mp3model<f_DEC>::BProcess;
  using mp3model<f_DEC>::NProcess;
  using mp3model<f_DEC>::put;
  using mp3model<f_DEC>::ProcMode;
  using mp3model<f_DEC>::GrbLen;
  using mp3model<f_DEC>::Garbage;
  using mp3model<f_DEC>::f_quit;
  using mp3model<f_DEC>::inpptr;
  using mp3model<f_DEC>::outptr;
  // Names looked up in a dependent base need to be declared for compilers
  // that implement two-phase lookup properly (clang, gcc without
  // -fpermissive, icx).  gcc's -fpermissive used to paper over these.
  using mp3model<f_DEC>::get;
  using mp3model<f_DEC>::chkinp;
  using mp3model<f_DEC>::chkout;
  using mp3model<f_DEC>::getinplen;
  using mp3model<f_DEC>::getoutlen;
  using mp3model<f_DEC>::InitSlen;
  using mp3model<f_DEC>::StartEncode;
  using mp3model<f_DEC>::StartDecode;
  using mp3model<f_DEC>::FinishEncode;
  using mp3model<f_DEC>::Process;

  #include "frame_dump_io.inc"

  uint Init( uint _rsv = 0 ) {
    InitSlen();
    InitModel( 0, 0 );
    ProcMode = 1-f_DEC; // 1=ReadFrame,rc enc, 0=rc dec, WriteFrame
    init_frame_dump();
    return 0;
  }

  void Quit( void ) {
  }

  void do_process( void ) {
    qword len; int flag;
    uint c,i,j,l,f_EOF=0, p_grb32k=hSCALE;

    if( f_DEC==0 ) StartEncode(); else StartDecode();

    while( f_EOF==0 ) {

      for( flag=0; flag==0; ) {
        if( f_DEC==0 ) flag = read_frame_dump(len,f_EOF);

        BProcess( p_grb32k, flag ); p_grb32k=SCALE-2;
        if( flag ) {

          qword c=len*2+f_EOF+1,r=0; int d;
          for( uint sign=0,l=0; sign==0; l+=7 ) {
            d = byte(c>>l);
            d = (d&0x7F) | (((c>>(l+7))==0)<<7);
            NProcess( d, 8, 0, 256 );
            r |= qword(d&0x7F)<<l; sign=d&0x80;
          }
          len=r-1; f_EOF = len&1;

        } else {
          Process(); // compress/decompress a frame
        }

        if( f_DEC==1 ) write_frame_dump(len,flag);
      }

    }

    if( f_DEC==0 ) FinishEncode();

    yield(this,0);
  }

};
