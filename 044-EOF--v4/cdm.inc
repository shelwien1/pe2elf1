
#include <windows.h>


#define TRACE_ON

enum { SCALElog=15, SCALE=1<<SCALElog, mSCALE=SCALE-1, hSCALE=SCALE/2, eSCALE=16*SCALE };

#include "log2lut.inc"
#include "pop2lut.inc"
#include "grc_lut.inc"

#include "sh_v1m.inc"

ALIGN(32) log2LUT LOG2;

ALIGN(32) pop2LUT POP2;

ALIGN(32) GRC_LUT GRAY;

uint div16( uint x ) { return (x+15)>>4; }

#include "sh_mapping.inc"
#include "MOD/sh_model_h.inc"

enum {
//  minblk = 16,
  maxblk = 256, maxblklog = 8, mb_prob_log=12, mb_prob_scale=1<<mb_prob_log,
//  maxblk = 2048, maxblklog = 11,
  optblk = (1<<20)-(maxblk+2), 
  optblk2 = optblk+(maxblk+2),
  tokblk = optblk+(maxblk+2),
  goaldiff = maxblk*32,
//  mid_div1 = 28, mid_div2 = 10, mid_nmin = (mid_div1+2+mid_div2)*2
};

uint p_maxblk = Max<int>(1,Min<int>(mSCALE,M_maxblk));
//uint p_compflag = Max<int>(1,Min<int>(mSCALE,M_compflag));
uint p_EOF =       Max<int>(1,Min<int>(mSCALE,M_EOF));
uint p_freq_half = Max<int>(1,Min<int>(mSCALE,M_freq_half));
uint p_freq_sign = Max<int>(1,Min<int>(mSCALE,M_freq_sign));
//uint p_freq_mid = Max<int>(1,Min<int>(mSCALE,M_freq_mid));
uint mid_div1 = M_mid_div1;
uint mid_div2 = M_mid_div2;
uint mid_nmin = (mid_div1+2+mid_div2)*2;

// precalc for block price estimation
uint f_maxblk_0;
uint f_maxblk_1;
uint f_compflag_0;
uint f_compflag_1;
uint f_compflag_2;
uint f_freq_half_0;
uint f_freq_half_1;
uint f_freq_sign_0;
uint f_freq_sign_1;
//uint f_freq_mid_0;
//uint f_freq_mid_1;


int p_lsb_0[maxblklog+1] = { M_lsb0,M_lsb1,M_lsb2,M_lsb3,M_lsb4,M_lsb5,M_lsb6,M_lsb7,0 };

int p_mid_0[4] = { M_mid0,M_mid1,M_mid2, 0 };

int p_cfl_0[4] = { M_cfl0,M_cfl1,M_cfl2, 0 };

//uint p_maxblk = SCALE-16;
//uint p_EOF = SCALE-16;
//uint p_freq_half = SCALE-16;
//uint p_freq_sign = SCALE/2;
//uint p_freq_mid = 4096;
//uint mid_div1 = 28, mid_div2 = 10, mid_nmin = (mid_div1+2+mid_div2)*2;

#pragma pack()

#include "opt_calc.inc"

#include "opt_tok.inc"

template< int f_DEC >
struct CDM: Rangecoder_SH1m<f_DEC> {

  using Rangecoder_SH1m<f_DEC>::FFNum;
  using Rangecoder_SH1m<f_DEC>::inpptr;
#ifdef TRACE_ON
  using Rangecoder_SH1m<f_DEC>::rc_outbytes;
  using Rangecoder_SH1m<f_DEC>::range;
#endif

  enum { inpbufsize = 1<<16, inpbuftotal=inpbufsize*32, inpbufhalf=inpbuftotal/2 };
  ALIGN(4096)
  byte inpbuf[inpbuftotal];

  qword inppos;
  qword tokendpos;

  qword goalpos, s_goalpos; 
  uint goalptr;
  uint _pad1;

  byte* inpbegptr;
  byte* inpendptr;

  uint p_lsb[maxblklog+1];
  uint p_mid[3+1];
  uint p_cfl[3+1];

  ALIGN(32)
  token tok_array[tokblk];

  ALIGN(32)
  byte pop_array[tokblk][2];

  ALIGN(32)
  uint e0_price[maxblk+1];
  uint e1_price[4*maxblk*maxblk+5*maxblk];
//  uint e2_price[4*maxblk*maxblk+5*maxblk];

  uint mb_price[mb_prob_scale][2][3];

  word len_array[tokblk]; // len array from parser opt

  void init_prices( void ) {
    f_maxblk_0    = (LOG2(SCALE)-LOG2(p_maxblk));
    f_maxblk_1    = (LOG2(SCALE)-LOG2(SCALE-p_maxblk));                

    f_compflag_0  = LOG2(p_cfl[3])-LOG2(p_cfl[0+1]-p_cfl[0]);
    f_compflag_1  = LOG2(p_cfl[3])-LOG2(p_cfl[1+1]-p_cfl[1]);
    f_compflag_2  = LOG2(p_cfl[3])-LOG2(p_cfl[2+1]-p_cfl[2]);

    f_freq_half_0 = (LOG2(SCALE)-LOG2(p_freq_half));                   
    f_freq_half_1 = (LOG2(SCALE)-LOG2(SCALE-p_freq_half));             
    f_freq_sign_0 = (LOG2(SCALE)-LOG2(p_freq_sign));                   
    f_freq_sign_1 = (LOG2(SCALE)-LOG2(SCALE-p_freq_sign));             
  }

  uint Init( void ) {
    uint i, cumfreq;

    coro_init();
    LOG2.Init();
    POP2.Init();
    GRAY.Init();

    tok_array[0].Init(tokblk);

    for( i=0,cumfreq=0; i<=maxblklog; i++ ) { p_lsb[i]=cumfreq; cumfreq+=p_lsb_0[i]; }

    for( i=0,cumfreq=0; i<=3; i++ ) { p_mid[i]=cumfreq; cumfreq+=p_mid_0[i]; }

    for( i=0,cumfreq=0; i<=3; i++ ) { p_cfl[i]=cumfreq; cumfreq+=p_cfl_0[i]; }

    init_prices();

    {
      uint j, blklen,  r, c0,c1, e0,e1, O_price_i, mode, log2maxblk;
      uint P,Pu, e2, g_c0, g_c1;

//printf( "e0_price=%i\n", sizeof(e0_price) );
//printf( "e1_price=%i\n", sizeof(e1_price) );
//printf( "e2_price=%i\n", sizeof(e2_price) );

      for( i=0; i<mb_prob_scale; i++ ) {
        P = i<<(SCALElog-mb_prob_log); 
        P = Max<int>(1,Min<int>(mSCALE,P));

        Pu = LinearUpdate( P, 0, M_mb_wr, M_mb_mw );
        Pu = Max<int>(1,Min<int>(mSCALE,Pu));
        mb_price[i][0][0] = (LOG2(SCALE)-LOG2(Pu));
        mb_price[i][0][1] = (LOG2(SCALE)-LOG2(SCALE-Pu));
        mb_price[i][0][2] = Pu;

        Pu = LinearUpdate( P, 1, M_mb_wr, M_mb_mw );
        Pu = Max<int>(1,Min<int>(mSCALE,Pu));
        mb_price[i][1][0] = (LOG2(SCALE)-LOG2(Pu));
        mb_price[i][1][1] = (LOG2(SCALE)-LOG2(SCALE-Pu));
        mb_price[i][1][2] = Pu;
//      f_maxblk_0 = (LOG2(SCALE)-LOG2(P));
//      f_maxblk_1 = (LOG2(SCALE)-LOG2(SCALE-P));                
      }


      // blklen loop
      for( i=1; i<=maxblk; i++ ) {
        blklen = i*8;
        e0 = CalcPrice0(*this,i,log2maxblk);
        e0_price[i-1] = e0;

        j = 4*(i-1)*(i-1)+5*(i-1);
        for( c0=0; c0<=blklen; c0++ ) {
          c1 = blklen-c0;

          e1 = CalcPrice1(*this,i,c0,c1,log2maxblk, f_compflag_2);
          e1_price[j+c0] = e1;
//          e2 = CalcPrice1(*this,i,c0,c1,log2maxblk, f_compflag_2);
//          e2_price[j+c0] = e2;
        }
      }
    }

    inppos = 0;
    return 0;
  }

  void do_process( void ) {
    rc_Init();
    if( f_DEC==0 ) do_encode(); else do_decode();
    rc_Quit(); 
    yield(this,0);
  }

#include "codec.inc"

  void do_encode( void ) {
    uint c,l,r,n,i,j,bit,p,blklen=0;
    qword curpos;

    inpbegptr = &inpbuf[0];
    inpendptr = &inpbuf[0];

    tokendpos = 0; // tok_array should be filled from inppos to tokendpos
    goalpos = 0;  
    s_goalpos = -1ULL; // goalpos for which len_array is filled atm
    goalptr = 0; // len_array pointer

    uint l_quit = 0;
    uint n_kills;

    while(1) {

      // shift window to buf start
      if( inpbegptr>=inpbuf+inpbufhalf ) {
        l = inpendptr-inpbegptr;
        if( l>0 ) memcpy( inpbuf, (byte*)inpbegptr, l );
        inpbegptr = inpbuf+0;
        inpendptr = inpbuf+l;
      }

      // if there's place in inpbuf, read more from external input
      while( (l_quit==0) && ((n=inpbuf+inpbuftotal-inpendptr)>0) ) {
        chkinp(); l=getinplen(); if( l==0 ) { l_quit=1; break; }
        l = Min<uint>( l, n );
        memcpy( (void*)inpendptr, (void*)inpptr, l ); inpptr+=l; inpendptr += l;
      }
      // if there's no more data, just quit
      if( l_quit && (inpbegptr>=inpendptr) ) { r=0; break; }

//if( !(inpbegptr+tokendpos-inppos + (l_quit?0:inpbufsize) > inpendptr) ) {
//printf( "{ %i\n", GetTickCount() );
//printf( "%I64i, ", tokendpos );

      // fill tok_array
      while(1) {
        if( inpbegptr+tokendpos-inppos + (l_quit?0:0) > inpendptr ) break;
        if( tokendpos-inppos >= optblk+1 ) break;

        c = (inpbegptr-1)[tokendpos-inppos];
        byte (&p)[2] = pop_array[(tokendpos+tokblk-1)%tokblk];
        p[0] = 8-POP2( c );
        p[1] = 8-POP2( GRAY.LUT[c] );
//printf( "curpos=%I64i p0=%02X\n", tokendpos-1, p[0] );

        tok_array[ tokendpos%tokblk ].Compute( inpbegptr, inpendptr, tokendpos, *this );
        tokendpos++;
      }

//printf( "%I64i\n", tokendpos );
//printf( "} %i\n", GetTickCount() );
//}

      if( (goalpos+1<tokendpos) && (goalpos<inppos+goaldiff) ) goalpos=0;
      if( (goalptr==0) || (s_goalpos!=goalpos) ) {
        uint len;

Reparse:
        n_kills = 0;
        goalptr = 0;

//printf( "Reparse: inppos=%I64i/%I64i goalpos=%I64i s_goalpos=%I64i blklen=%i goalptr=%i pos=%.3lf\n", inppos,tokendpos,goalpos,s_goalpos,blklen,goalptr, double(tok_array[inppos%tokblk].price)/4096/8 ); fflush(stdout);

        if( goalpos<inppos+goaldiff ) goalpos=0;

        curpos = (goalpos==0) ? tokendpos-1 : goalpos;
        while(1) {
          token& T = tok_array[curpos%tokblk]; len=T.blklen;
          len_array[goalptr++] = len;
//printf( "inppos=%I64i curpos=%I64i len=%i\n", inppos, curpos, len );
          if( sqword(curpos-len)<=sqword(inppos) ) break;
          curpos -= len;
          if( goalpos==0 ) goalpos = curpos; // skip tail (potentially incomplete)
        }
        if( (goalpos==0) || (goalpos<inppos+goaldiff) ) goalpos = tokendpos-1;

        // Update tok_array if the path changed due to goalpos updates or something
        if( sqword(curpos-len)<sqword(inppos) ) {
          // check tok_array; kill tokens that lead below inppos
          for( curpos=inppos+1; curpos<goalpos; curpos++ ) {
            token& T1 = tok_array[curpos%tokblk];
            token& T0 = tok_array[(curpos-T1.blklen)%tokblk];
            if( (T1.pos!=curpos) || (T0.pos<inppos) || (T0.pos!=curpos-T1.blklen) ) T1.pos=-1ULL, n_kills++;
          }
          // recompute the killed tokens
          for( curpos=inppos+1; curpos<goalpos; curpos++ ) {
            token& T1 = tok_array[curpos%tokblk];
            if( T1.pos==token::pos_empty ) tok_array[curpos%tokblk].Compute( inpbegptr, inpendptr, curpos, *this );
          }
          goalpos=0;
//getc(stdin);
          goto Reparse;
        }

//        len = curpos-inppos;
//        if( len>maxblk ) printf( "!!! backtrack error: len=%i !!!\n", len );
//        blklen = len;
        s_goalpos = goalpos;
      }

      blklen = len_array[--goalptr];

//printf( "inppos=%I64i goalpos=%I64i s_goalpos=%I64i blklen=%i goalptr=%i\n", inppos,goalpos,s_goalpos,blklen,goalptr); fflush(stdout);

      process_EOF(0);

//printf( "%.3lf\n", double(tok_array[(inppos+blklen)%tokblk].price)/4096/8 );
//printf( "blklen=%i\n", blklen );

      encode_block( inpbegptr, blklen, tok_array[(inppos+blklen)%tokblk].f_compflag ); 

//printf( "%X %X; ", p_maxblk, tok_array[(inppos+0)%tokblk].p_maxlen );

      p_maxblk = LinearUpdate( p_maxblk, (blklen!=maxblk), M_mb_wr, M_mb_mw );

//printf( "%X %X\n", p_maxblk, tok_array[(inppos+blklen)%tokblk].p_maxlen );

      inpbegptr+=blklen; inppos+=blklen; 


    } // mainloop

    // encode EOF
    process_EOF(1);

  } // do_encode


  void do_decode( void ) {
    uint l_quit = 0, blklen=0;
    while(1) {
      if( process_EOF() ) break;
      blklen = decode_block(); //if( blklen==0 ) break;
      p_maxblk = LinearUpdate( p_maxblk, (blklen!=maxblk), M_mb_wr, M_mb_mw );
    }
  } // do_decode

};
