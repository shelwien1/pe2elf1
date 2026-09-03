
// ---- self tests -------------------------------------------------------------
// sh_bitc.cpp's suite, rescaled for CDM.  The driver replicates
// CoroFileProc::processfile's protocol in memory, so the very same coroutines
// are exercised; g.ovf catches a runaway decode of an archive no encoder made.
//
// What this coder promises, and what it does not:
//
//   [1][4] FORWARD, decode(encode(m))==m for every m -- asserted.  This is the
//          property a compressor must have, and it holds exhaustively.
//
//   [0]    Bijective TERMINATION -- asserted.  There is no EOF symbol and no
//          rangecoder flush: the empty file is the empty archive, and the
//          archive length is the code length up to byte rounding.  That is
//          what porting sh_bit's reserved inverse-FO endings buys.
//
//   [2][3] BACKWARD, encode(decode(a))==a for every a -- REPORTED, NOT
//          asserted, because CDM cannot have it.  sh_bitc can: its order-0
//          model codes one symbol per byte with no choices, so an archive
//          determines the encoder's every move.  CDM's encoder runs an optimal
//          parser -- it picks block lengths and one of three coding methods per
//          block by price -- so an arbitrary archive decodes to a message whose
//          implied parse is generally not the parse the encoder would choose,
//          and re-encoding produces a different (usually cheaper) archive.  A
//          second, independent cause: the decoder reads an implied tail of 1s
//          past the end of the archive, and with blocks of up to 256 bytes it
//          can consume much more codestream than the archive pays for, so the
//          re-encoding can also come out longer.  Both are properties of a
//          parsing compressor, not of the coder.  What [3] DOES assert is
//          stability: whatever message an arbitrary archive decodes to, that
//          message survives its own encode/decode round trip.
struct memhandle {
  byte* buf; uint len, pos, ovf;
  memhandle( const void* p, uint n ) : buf((byte*)p), len(n), pos(0), ovf(0) {}
  uint sread( void* p, uint n ) { if( n>len-pos ) n=len-pos; memcpy(p,buf+pos,n); pos+=n; return n; }
  uint writ( void* p, uint n ) { if( n>len-pos ) n=len-pos, ovf=1; memcpy(buf+pos,p,n); pos+=n; return n; }
};

template< class P >
int memproc( P& X, memhandle& f, memhandle& g ) {
  uint l,r;
  X.coro_init();
  X.addout( X.outbuf, X.outbufsize );
  while(1) {
    r = X.coro_call(&X);
    if( r==1 ) {
      l = f.sread( X.inpbuf, X.inpbufsize );
      if( l==0 ) X.f_quit=1;
      X.addinp( X.inpbuf, l );
    } else {
      l = X.getoutsize();
      if( l>0 ) g.writ( (byte*)X.outbeg, l );
      if( g.ovf ) return -1; // runaway decode: abandon it (coro_init restarts)
      if( r!=2 ) break;
      X.addout( X.outbuf, X.outbufsize );
    }
  }
  return g.pos;
}

int mem_encode( const byte* m, uint mlen, byte* out, uint ocap ) { memhandle f(m,mlen), g(out,ocap); return memproc( C, f,g ); }
int mem_decode( const byte* a, uint alen, byte* out, uint ocap ) { memhandle f(a,alen), g(out,ocap); return memproc( D, f,g ); }

static uint rnd_state = 20240524;
static uint rnd( void ) { rnd_state = rnd_state*1103515245u+12345u; return rnd_state>>8; }


#ifndef TEST_MAXLEN
#define TEST_MAXLEN 2       // exhaustive tests cover every file/archive of <= this many bytes
#endif
#ifndef TEST_BWD
#define TEST_BWD 2000       // random archives in test [3]
#endif
#ifndef TEST_FWD
#define TEST_FWD 300        // random files in test [4]
#endif

int run_tests( void ) {
  g_trace = 0;                // the suite drives millions of blocks
  C.M1.Init(); D.M2.Init();   // price tables: once, not once per case

  static byte m[1<<17], a[1<<17], d[1<<17];
  int rc = 0;
  uint len,v,i;

  { // [0] bijective termination: no EOF symbol, no flush
    int al = mem_encode( m,0, a,sizeof(a) );
    int dl = mem_decode( a,0, d,sizeof(d) );
    int ok = (al==0) && (dl==0);
    printf( "[0] TERMINATION empty file <-> empty archive        : enc=%d dec=%d %s\n", al,dl, ok?"PASS":"FAIL" );
    rc |= !ok;
  }
  { // [1] FORWARD: every file of <=2 bytes
    long ok=0,fail=0,maxa=0;
    for( len=0; len<=TEST_MAXLEN; len++ ) for( v=0; v<(1u<<(8*len)); v++ ) {
      for( i=0; i<len; i++ ) m[i] = v>>(8*(len-1-i));
      int al = mem_encode( m,len, a,sizeof(a) ); if( al>maxa ) maxa=al;
      int dl = mem_decode( a,al, d,sizeof(d) );
      if( (dl==int(len)) && (memcmp(m,d,len)==0) ) ok++; else fail++;
    }
    printf( "[1] FORWARD  decode(encode(m))==m, |m|<=%i bytes : ok=%ld fail=%ld maxarc=%ld %s\n", TEST_MAXLEN, ok,fail,maxa, fail?"FAIL":"PASS" );
    rc |= (fail!=0);
  }
  { // [2] BACKWARD: every archive of <=2 bytes
    long ok=0,fail=0,run=0,maxm=0;
    for( len=0; len<=TEST_MAXLEN; len++ ) for( v=0; v<(1u<<(8*len)); v++ ) {
      for( i=0; i<len; i++ ) a[i] = v>>(8*(len-1-i));
      int dl = mem_decode( a,len, d,sizeof(d) ); if( dl<0 ) { run++; continue; } if( dl>maxm ) maxm=dl;
      int al = mem_encode( d,dl, m,sizeof(m) );
      if( (al==int(len)) && (memcmp(a,m,len)==0) ) ok++; else fail++;
    }
    printf( "[2] BACKWARD encode(decode(a))==a, |a|<=%i bytes : ok=%ld differ=%ld (%.1f%%) runaway=%ld maxmsg=%ld %s\n",
            TEST_MAXLEN, ok,fail, 100.0*ok/(ok+fail), run, maxm, run?"FAIL":"reported" );
    rc |= (run!=0);   // a runaway would be a real defect; a differing re-encode is the parser
  }
  { // [3] STABILITY on arbitrary archives (+ backward rate, reported)
    long ok=0,unstable=0,run=0,exact=0;
    static byte d2[1<<17];
    for( uint it=0; it<TEST_BWD; it++ ) {
      len = 3+rnd()%62; uint mode=rnd()%4;
      for( i=0; i<len; i++ ) a[i] = (mode==0) ? rnd() : (mode==1) ? (rnd()%16==0 ? rnd() : 0) : (mode==2) ? (rnd()%16==0 ? rnd() : 0xFF) : (rnd()&1 ? 0x80 : 0x00);
      int dl = mem_decode( a,len, d,sizeof(d) ); if( dl<0 ) { run++; continue; }
      int al = mem_encode( d,dl, m,sizeof(m) );
      if( (al==int(len)) && (memcmp(a,m,len)==0) ) exact++;
      int dl2 = mem_decode( m,al, d2,sizeof(d2) );
      if( (dl2==dl) && (memcmp(d,d2,dl)==0) ) ok++; else unstable++;
    }
    printf( "[3] STABILITY %i random archives, 3<=|a|<=64 : ok=%ld unstable=%ld runaway=%ld (exact backward %ld) %s\n",
            TEST_BWD, ok,unstable,run,exact, (unstable||run)?"FAIL":"PASS" );
    rc |= (unstable!=0)||(run!=0);
  }
  { // [4] FUZZ forward: random files, skewed
    long ok=0,fail=0; qword sm=0,sa=0;
    for( uint it=0; it<TEST_FWD; it++ ) {
      len = rnd()%4000; uint mode=rnd()%4, cur=rnd(), r=0;
      for( i=0; i<len; i++ ) {
        if( mode==0 ) m[i]=rnd(); else if( mode==1 ) m[i]=rnd()%3; else if( mode==2 ) m[i]='a'+rnd()%6;
        else { if( r--==0 ) cur=rnd(), r=rnd()%200; m[i]=cur; }
      }
      int al = mem_encode( m,len, a,sizeof(a) ); sm+=len; sa+=al;
      int dl = mem_decode( a,al, d,sizeof(d) );
      if( (dl==int(len)) && (memcmp(m,d,len)==0) ) ok++; else fail++;
    }
    printf( "[4] FUZZ     %i random files, |m|<4000        : ok=%ld fail=%ld (%llu -> %llu bytes) %s\n", TEST_FWD, ok,fail, sm,sa, fail?"FAIL":"PASS" );
    rc |= (fail!=0);
  }
  printf( "%s (RC_BIJ=%i)\n", rc ? "*** TESTS FAILED ***" : "ALL TESTS PASSED", RC_BIJ );
  return rc;
}
