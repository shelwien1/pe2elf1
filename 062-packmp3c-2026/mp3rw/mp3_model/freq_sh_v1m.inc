
enum {
  SCALElog = 15,
  SCALE    = 1<<SCALElog
};

template< int _rc_ProcMode >
struct RangeCoder_v1m : Coroutine {
  enum{ rc_ProcMode=_rc_ProcMode };

  //uint  rc_ProcMode; // 0=encode, 1=decode;

  #define NUM   4
  #define sTOP  uint(1<<24)
  #define Thres uint(255*sTOP)
  #define DO(n) for (int _=0; _<n; _++)

  uint  low;  
  uint  code; 
  uint  FFNum;
  uint  Carry;
  uint  Cache;
  uint  range;

  uint muldivR( uint a, uint b ) { return (qword(a)<<SCALElog)/range; }
  uint mulRdiv( uint a, uint c ) { return (qword(a)*range)>>SCALElog; }

  void BProcess( uint freq, int& bit ) { 

//if( (int(freq)<1) || (int(freq)>32767) ) printf( "freq=%i\n", freq );

    uint rnew;
    if( range>=16*sTOP ) 
      rnew = freq*(range>>SCALElog);
    else
      rnew = freq*(range>>(SCALElog-4))>>4;

    if( rc_ProcMode ) {
      bit = (code>=rnew);
      if( bit ) {
        range -= rnew;
        code  -= rnew;
      } else {
        range  = rnew;
      }
      while( range<sTOP ) range<<=8, (code<<=8)+=byte(get());
    } else {
      if( bit ) {
        range -= rnew;
        Carry += (low+rnew<low);
        low   += rnew;
      } else {
        range  = rnew;
      }
      while( range<sTOP ) range<<=8, ShiftLow();
    }

  }

 // unlike Number's low is included
/*
   void NProcess( int &n, int len, int lo, int hi ) {
     int i,j,k,z=0;
     for( i=len-1; i>=0; i-- ) {
       k = (n>>i)&1;
       if( z+(1<<i)-1< lo ) k=1; 
  else if( z+(1<<i)  >=hi ) k=0; 
  else BProcess( SCALE/2, k );
       z |= (k<<i);
     }
     n = z;
   }
*/

  void NProcess( int &n, int len, int lo, int hi ) {
    int i,l_half,h_half,p,bit;
    n-=lo; hi-=lo; if( hi<=0 ) return;
    for( i=len-1; i>=0; i-- ) {
      l_half = (1<<i); if( l_half>=hi ) continue;
      h_half = hi - l_half;

      p = (SCALE<<i)/(l_half+h_half); // SCALE*l_half/hi
      bit = (n>=l_half);
      //BProcess( p+(p==0), bit );
      BProcess( p+1, bit );

      lo += (bit<<i); // bit*l_half
      n -= (bit<<i);
      hi = bit ? h_half : l_half;
    }
    n = lo;
  }

  void StartEncode( void ) { 
    //CodeFile = f;
    //rc_ProcMode=0;
    range = 0xFFFFFFFF;
    low   = 0;
    FFNum = 0;
    Carry = 0;    
    Cache = -1;
  }

 // void ShiftLow( void );

  void StartDecode( void ) {  
    StartEncode(); //rc_ProcMode=1;    
    DO(NUM+0) (code<<=8)+=byte(get());
  }

  void FinishEncode( void ) { 
//    DO(NUM+1) ShiftLow(); 
    uint i, n = NUM;

//printf( "Cache=%X FFNum=%i Carry=%X low=%08X\n", Cache, FFNum, Carry, low );

    // cache .. FF x FFNum .. low
    qword llow=(qword(Carry)<<32)+low;
    qword high=llow+range;

    if( qword(llow|0xFF) < high ) llow|=0xFF,n--;
    if( qword(llow|0xFFFF) < high ) llow|=0xFFFF,n--;
    if( qword(llow|0xFFFFFF) < high ) llow|=0xFFFFFF,n--;
    if( qword(llow|0xFFFFFFFF) < high ) llow|=0xFFFFFFFF,n--;
    low = uint(llow);
    if( low==0xFFFFFFFF ) FFNum=0; // they're also FFs

    if( Cache!=-1 ) put( Cache+Carry );
    for( i=0; i<FFNum; i++ ) put( 0xFF+Carry );
    for( i=0; i<n; i++ ) put( low>>24 ), low<<=8;
  }

  void FinishDecode( void ) {}

  void ShiftLow( void ) {
    if( low<Thres || Carry ) {
      if( Cache!=-1 ) put( Cache+Carry );
      for(; FFNum!=0; FFNum-- ) put( Carry-1 );
      Cache = low>>24;
      Carry = 0;
    } else FFNum++;
    low<<=8;
  }

};


#undef DO
#undef NUM  
#undef sTOP 
#undef Thres
