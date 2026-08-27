
// -------------------------------------------------------------
//  rc_kernel.inc -- the OpenCL C side, as a string.
//
//  This is rc.inc's carryless coder, one work-item per lane. Nothing about
//  it is new: the encoder's lanes were already independent -- bit i of a
//  block goes to lane i%RCNUM and no lane reads another's state -- which is
//  what sh_v1xN.inc's SIMD version was built on and what the perl macro pass
//  existed to inline. Here the device compiler does that job instead: RCNUM
//  scalar work-items, vectorised across work-items by the runtime.
//
//  Only the carryless coder is here. The carry-propagating twin is a serial
//  Cache/FF-run state machine with a data-dependent inner loop, and it only
//  runs on the blocks the fast path had to hand back, so it stays on the host.
//  So does the decoder.
//
//  What the host and the device have to agree on comes over as -D, from
//  RcCL::Program: RCNUM, SCALElog, LOWBYTES, CODBYTES, the block size and the
//  output stride. They are the strides the device compiler folds into
//  addresses, and every one of them is a compile-time constant on the host
//  side too, so there is nothing to pass per launch except the block.
//
//  The output must be byte-identical to the host coder -- it is the same
//  integer arithmetic in the same order -- and t.sh checks exactly that by
//  comparing a device encode against -C.
// -------------------------------------------------------------

static const char* RC_CL_SRC = R"CLC(

#define LOWBITS (LOWBYTES*8)

#if LOWBYTES==8
 #define LOW_MASK 0xFFFFFFFFFFFFFFFFUL
#else
 #define LOW_MASK ((1UL<<LOWBITS)-1UL)
#endif

#define sTOP 0x01000000u
#define gTOP 0x00010000u

#if RC_RANGE64
 typedef ulong rangetype;
 #define RANGE_INIT (1UL<<32)
#else
 typedef uint  rangetype;
 #define RANGE_INIT 0xFFFFFFFFu
#endif

typedef struct {
  ulong low;                // low accumulator, LOWBITS wide
  rangetype range;
  uint  rpre;               // the unit, scaled to the low delta by rc_process
  uint  ffnum;              // carries that escaped -- the flag, not an FF run
  uint  skip;               // leading zero bytes still to drop
  uint  n;                  // bytes handed to put, dropped ones included
  __global uchar* o;
} rc_t;

// The first LOWBYTES-CODBYTES bytes are the zero prefix and are not part of
// the stream; past OUTCAP the substream did not fit, and n keeps counting so
// the host can see by how much.
inline void rc_put( rc_t* s, uchar c ) {
  if( s->skip ) { s->skip--; return; }
  if( s->n<OUTCAP ) s->o[s->n] = c;
  s->n++;
}

// low += rpre, then n bytes out of the top. A carry out of the accumulator
// only raises the flag: it has LOWBYTES-CODBYTES bytes of headroom to travel
// through before it could reach an already-emitted byte, and if it ever gets
// out the host re-codes the block with the carry-propagating twin.
inline void rc_shiftlow( rc_t* s, uint nsh ) {
  ulong l = s->low + s->rpre;
#if LOWBYTES==8
  uint c = (l<s->low);
#else
  uint c = (l>LOW_MASK);
#endif
  s->low  = l & LOW_MASK;
  s->rpre = 0;
  s->ffnum += c;

  for( ; nsh!=0; nsh-- ) {
    rc_put( s, (uchar)(s->low>>(LOWBITS-8)) );
    s->low = (s->low<<8) & LOW_MASK;
  }
}

// The counted 0/1/2-byte shift is exact for a binary coder: the unit is
// range>>SCALElog >= 2^9, so one step cannot take range below 2^8.
inline void rc_renorm( rc_t* s ) {
  uint nsh = (s->range<sTOP) + (s->range<gTOP);
  rc_shiftlow( s, nsh );
  s->range <<= nsh*8;
#if RC_RENORM_TAIL
  while( s->range<sTOP ) { rc_shiftlow( s, 1 ); s->range <<= 8; }
#endif
}

inline void rc_process( rc_t* s, uint freq, uint bit ) {
  rc_renorm( s );
  s->rpre = (uint)(s->range>>SCALElog);

  s->rpre *= freq;

  s->range -= s->rpre;

  s->range = bit ? s->range : (rangetype)s->rpre;
  s->rpre &= (uint)(-(int)bit);
}

// The minimal flush: set as many low bytes as still leave low < high and drop
// them, because the decoder reads 0xFF past the end of a substream and rc_Read
// pads with exactly that.
inline void rc_quit( rc_t* s ) {
  rc_renorm( s );

  uint nn = LOWBYTES;
  ulong llow = s->low;
  ulong high = llow + (ulong)s->range;
  uint  hic  = (high<llow);          // llow+range can leave the ulong at LOWBYTES==8

  for( uint i=0; i<LOWBYTES; i++ ) {
    ulong mask = ((1UL<<((i+1)*4))<<((i+1)*4))-1UL;
    if( hic || ((llow|mask)<high) ) { llow |= mask; nn--; }
  }

  for( uint i=0; i<nn; i++ ) rc_put( s, (uchar)(llow>>(LOWBITS-8-i*8)) );
}

// One block, RCNUM work-items. Lane id codes, in this order: the block-length
// flag if it is lane 0, then the length bits that fall to it, then every bit
// of the block whose index is id mod RCNUM -- which is model0.inc's order,
// bit for bit.
__kernel void rc_encode(
    __global const ushort* pbit,     // nbits entries, packed (bit<<15)|p
    const uint nbits,
    const uint blksize,
    __global uchar* out,             // RCNUM rows of OUTSTRIDE
    __global uint*  outlen,
    __global uint*  outcarry )
{
  const uint id = get_global_id(0);

  rc_t s;
  s.low   = 0;
  s.range = RANGE_INIT;
  s.rpre  = 0;
  s.ffnum = 0;
  s.skip  = LOWBYTES-CODBYTES;
  s.n     = 0;
  s.o     = out + (size_t)id*OUTSTRIDE;

  const uint flag = (blksize!=BLKFULL);

  if( id==0 ) rc_process( &s, hSCALE, flag );

  if( flag )
    for( uint j=id; j<16; j+=RCNUM )
      rc_process( &s, hSCALE, (blksize>>(15-j))&1 );

  for( uint k=id; k<nbits; k+=RCNUM ) {
    uint b = pbit[k];
    rc_process( &s, b&0x7FFF, b>>15 );
  }

  rc_quit( &s );

  outlen[id]   = s.n;
  outcarry[id] = s.ffnum;
}

)CLC";
