

struct mp3parse : Coroutine, mp3frame {

  enum{ BufSiz=16384 };

  qword inppos, outpos, outlen; 
  uint eofcount;
  
  word cCRC;

  // The pointer has to be decremented before (BufPtr&7)/(OutPtr&7) is read,
  // so do it as a separate comma operand. putbits used to decrement OutPtr in
  // the left operand of |= and read it in the right one: unsequenced before
  // C++17, right operand first since. clang always took the right operand
  // first, gcc did for procbits(*scf++,num) in parse_scale2.inc (MPEG-2/2.5),
  // and the decoder wrote those bits at the wrong offset.
  #define getbits( buf, l )   ( BufPtr-=l, (((uint&)buf[BufPtr>>3])>>(BufPtr&7)) & ((1<<l)-1) )
  #define putbits( c,l )      ( OutPtr-=l, ((uint&)Out[OutPtr>>3])|=((c)<<(OutPtr&7)) )
  #define procbits( c,l )     (ProcMode?c=getbits(sinf,l):putbits(c,l))

  byte Buf[2*BufSiz];
  byte* sinf; 
  int BufPtr, BufLen;

  byte Out[2*BufSiz];
  int OutPtr,OutLen;

  struct {
    byte h[4+2+32];
    int  HSize;  
    int  MSize;  
  } Hdr[512];
  uint Hdr1:9;
  uint Hdr2:9;

  int    l12prev;    // last ReadFrame returned a layer I/II frame
  header l12hdr;     // its header
  int    l12crcbits; // CRC protected bits after the CRC, set by ProcL12

  int ReadLoEnd( int l, int flag=1 ) {
    int i,c,rv=0;
    while( BufPtr<l*8 ) {
      c = get(); inppos++;
      if( c==-1 ) {
        eofcount++;
        if(flag) { rv=1; break; } else c=0;
      }
      if( sinf<=Buf ) {
        for( i=0; i<BufSiz; i++ ) Buf[BufSiz+i]=Buf[i];
        sinf=&Buf[BufSiz];
      }
      (--sinf)[0] = c;
      BufPtr += 8;
      BufLen += 8;
    }
    return rv;
  }

   
  void Input( void ) { 
    int i;
    
//    fptr=0;
//    tagflag=0;
    
    for( i=0; i<sizeof(Buf); i++ ) Buf[i]=0;
    BufPtr=BufLen=0;
    sinf = &Buf[sizeof(Buf)];

    GrbPtr=GrbLen=GrbPrev=-1;

    
    InitSlen();

    rframe1=1; l12prev=0;
    ProcMode=1; eofcount=0; inppos=0;
  }

   
  void Output( void ) { 
    int i;            
 
//    g=G; 
//    glen = flen;
    FW=0;

    for( i=0; i<sizeof(Out); i++ ) Out[i]=0; 
    OutPtr = (sizeof(Out)-1024)*8;
    OutLen = (sizeof(Out)-1024)-1; 

    memset( Hdr, 0, sizeof(Hdr) );
    Hdr1=Hdr2=0;

  
    GrbPrev=-1;

    
    InitSlen();

    wframe1=1;
    ProcMode=0; outpos=0; outlen=-1LL;
  }

  #include "parse_frameread.inc"
  #include "parse_framewrite.inc"

  #include "parse_huf_dec.inc"
  #include "parse_huf_enc.inc"
  #include "parse_scale1.inc"
  #include "parse_scale2.inc"

  #include "parse_sideinfo.inc"

  #include "parse_l12.inc"

};
