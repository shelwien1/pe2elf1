
// -------------------------------------------------------------
//  rc_kernel.cl -- the OpenCL C side.
//
//  txt2inc.pl turns this into rc_kernel.inc, the string rc_cl.cpp compiles;
//  build.sh regenerates it whenever this file is newer. Everything above the
//  marker line below is preamble: a human reads it here, and it is not shipped
//  to the device compiler on every run.
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

// [[static const char RC_CL_SRC]]

#define LOWBITS (LOWBYTES*8)
#define SKIP    (LOWBYTES-CODBYTES)

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

// The coder's state, as plain private scalars rather than a struct. A struct
// ought to be scalarised before the runtime vectorises across work-items, and
// on this runtime it is -- but leaving that to the compiler is a bet, and
// these are the same six values sh_v1xN.inc keeps as separate ALIGN(VECSIZE)
// arrays for exactly this reason. The bodies below are macros for the same
// reason sh_v1xN_macro.inc's are: so there is no pointer to a private
// aggregate anywhere for the vectoriser to give up on.
#define RC_DECL                                                              \
  ulong low = 0;              /* low accumulator, LOWBITS wide            */ \
  rangetype range = RANGE_INIT;                                              \
  uint rpre = 0;              /* the unit, scaled to the low delta        */ \
  uint ffnum = 0;             /* carries that escaped -- a flag, not a run */ \
  uint nout = 0;              /* bytes stored, the zero prefix included   */ \
  __global uchar* o = out + (size_t)get_global_id(0)*OUTSTRIDE

// The row is laid out as the host's: SKIP bytes of zero prefix, then the
// payload. The prefix is stored rather than tested for -- same as RC_IO::put --
// and outlen reports the payload alone. Past the capacity the substream did not
// fit, and nout keeps counting so the host can see by how much.
#define rc_put(c)                                                            \
  do { if( nout<SKIP+OUTCAP ) o[nout] = (uchar)(c); nout++; } while(0)

#if LOWBYTES==8
 #define RC_CARRYOUT(l) ((uint)((l)<low))
#else
 #define RC_CARRYOUT(l) ((uint)((l)>LOW_MASK))
#endif

// low += rpre, then nsh bytes out of the top. A carry out of the accumulator
// only raises the flag: it has LOWBYTES-CODBYTES bytes of headroom to travel
// through before it could reach an already-emitted byte, and if it ever gets
// out the host re-codes the block with the carry-propagating twin.
//
// sh_v1xN.inc's ShiftLowN: store the window unconditionally and advance the
// cursor by nsh, rather than loop nsh times. nsh is 0, 1 or 2 and differs
// between lanes, so the loop was the one place a 16-way vectorised kernel had
// to diverge -- every lane paying for the widest. Both bytes go out every
// time; at nsh<2 the cursor does not move past the second, so the next store
// overwrites it. The last one can leave a byte of rubbish just past the
// substream, inside the row's padding and past the length the host is told,
// which is why nothing reads it.
//
// sh_v1xN.inc writes backwards, which is what lets it do this in one 16-bit
// store; that layout would reverse the substream against the host coder and
// the decoder, so here it is two byte stores going forwards. The branch is
// what cost, not the store width -- a single vstore2 measured slower.
#define rc_shiftlow(nsh)                                                     \
  do {                                                                       \
    ulong _l = low + rpre;                                                   \
    ffnum += RC_CARRYOUT(_l);                                                \
    rpre = 0;                                                                \
    if( nout < SKIP+OUTCAP ) {                                               \
      o[nout  ] = (uchar)(_l>>(LOWBITS- 8));                                 \
      o[nout+1] = (uchar)(_l>>(LOWBITS-16));                                 \
    }                                                                        \
    nout += (nsh);                                                           \
    low = (_l<<((nsh)*8)) & LOW_MASK;                                        \
  } while(0)

// The counted 0/1/2-byte shift is exact for a binary coder: the unit is
// range>>SCALElog >= 2^9, so one step cannot take range below 2^8.
#if RC_RENORM_TAIL
 #define RC_RENORM_TAIL_LOOP() \
   while( range<sTOP ) { rc_shiftlow(1); range <<= 8; }
#else
 #define RC_RENORM_TAIL_LOOP() do {} while(0)
#endif

#define rc_renorm()                                                          \
  do {                                                                       \
    uint _nsh = (range<sTOP) + (range<gTOP);                                 \
    rc_shiftlow(_nsh);                                                       \
    range <<= _nsh*8;                                                        \
    RC_RENORM_TAIL_LOOP();                                                   \
  } while(0)

#define rc_process(freq,bit)                                                 \
  do {                                                                       \
    uint _b = (bit);                                                         \
    rc_renorm();                                                             \
    rpre = (uint)(range>>SCALElog);                                          \
    rpre *= (freq);                                                          \
    range -= rpre;                                                           \
    range = _b ? range : (rangetype)rpre;                                    \
    rpre &= (uint)(-(int)_b);                                                \
  } while(0)

// The minimal flush: set as many low bytes as still leave low < high and drop
// them, because the decoder reads 0xFF past the end of a substream and rc_Read
// pads with exactly that.
#define rc_quit()                                                            \
  do {                                                                       \
    rc_renorm();                                                             \
    uint  _nn = LOWBYTES;                                                    \
    ulong _lo = low;                                                         \
    ulong _hi = _lo + (ulong)range;                                          \
    uint  _hc = (_hi<_lo);   /* _lo+range can leave the ulong at LOWBYTES==8 */\
    for( uint _i=0; _i<LOWBYTES; _i++ ) {                                    \
      ulong _m = ((1UL<<((_i+1)*4))<<((_i+1)*4))-1UL;                        \
      if( _hc || ((_lo|_m)<_hi) ) { _lo |= _m; _nn--; }                      \
    }                                                                        \
    for( uint _i=0; _i<_nn; _i++ ) rc_put( _lo>>(LOWBITS-8-_i*8) );          \
  } while(0)

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

  RC_DECL;

  const uint flag = (blksize!=BLKFULL);

  if( id==0 ) rc_process( hSCALE, flag );

  if( flag )
    for( uint j=id; j<16; j+=RCNUM )
      rc_process( hSCALE, (blksize>>(15-j))&1 );

  for( uint k=id; k<nbits; k+=RCNUM ) {
    uint b = pbit[k];
    rc_process( b&0x7FFF, b>>15 );
  }

  rc_quit();

  // a lane that coded nothing can stop inside the prefix and owes nothing
  outlen[id]   = nout>SKIP ? nout-SKIP : 0;
  outcarry[id] = ffnum;
}

