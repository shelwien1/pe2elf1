
struct log2LUT {

  enum {
    LUTsize = 32768+1, //8192, 
    Precision = 16
  };

  uint LUT[LUTsize];

  const uint operator() ( uint i ) const {
//if( i>=LUTsize ) printf( "LUT bug!\n" );
    return LUT[i];
  }

  uint Calc( uint i ) const {
    uint k=0;
    qword w = i;
    for( uint j=0; j<Precision; j++ ) {
      w = w * w;
      k = k + k;
      while( w>=(1ULL<<32) ) w=(w+1)>>1, k++;
    }
    return k;
  }

  void Init( void ) {
    for( uint i=0; i<LUTsize; i++ ) LUT[i] = Calc(i);
  }

};

log2LUT LOG2;
