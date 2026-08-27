/*   rc_kernel.c -- the OpenCL C side, as functions. */
/*  */
/*   Named .c rather than .cl because that is what it is to an editor: C, which */
/*   is close enough to OpenCL C to highlight and index properly. It is the */
/*   source; rc_kernel.cl and then rc_kernel.inc are generated from it. */
/*   mk_kernel.sh runs the chain, build.sh runs mk_kernel.sh when this file is */
/*   newer: */
/*  */
/*     rc_kernel.c  --rc_macro.pl-->  functions turned into #define/#enddef */
/*                  --defines.pl -->  ... into real multi-line macros */
/*                  --txt2inc.pl -->  ... into C string literals */
/*  */
/*   The coder is macros for the same reason sh_v1xN_macro.inc's is: a lane's */
/*   state has to be plain private scalars in the kernel, with no pointer to an */
/*   aggregate anywhere for the vectoriser to give up on, and that costs about */
/*   6%. Writing them as functions here and generating the macros keeps that */
/*   without hand-maintained backslashes. */
/*  */
/*   So the coder functions below take only their real arguments and reach the */
/*   lane state -- low, range, rpre, ffnum, nout, o -- by name. They are macro */
/*   bodies, not functions, however much they look like functions; that is the */
/*   one thing to keep in mind when editing this file. */
/*  */
/*   Only the carryless coder is here. The carry-propagating twin is a serial */
/*   Cache/FF-run state machine with a data-dependent inner loop, and it only */
/*   runs on the blocks the fast path had to hand back, so it stays on the host. */
/*   So does the decoder. */
/*  */
/*   What the host and the device have to agree on comes over as -D, from */
/*   RcCL::Program: RCNUM, SCALElog, LOWBYTES, CODBYTES, RC_LOWSPLIT, the block */
/*   size and the output stride. Every one of them is a compile-time constant on */
/*   the host side too, so there is nothing to pass per launch except the block. */
/*  */
/*   The output must be byte-identical to the host coder -- it is the same */
/*   integer arithmetic in the same order -- and t.sh checks exactly that by */
/*   comparing a device encode against -C. */

/*  [[static const char RC_CL_SRC]] */

/*  OpenCL C has no <stdint.h>, but it does not need one: char, short, int and */
/*  long are exactly 8, 16, 32 and 64 bits wide by definition, not "at least". */
/*  These are a spelling, so the kernel reads like the host code next to it. */
typedef uchar  uint8_t;
typedef ushort uint16_t;
typedef uint   uint32_t;
typedef ulong  uint64_t;

#define LOWBITS (LOWBYTES*8)
#define SKIP    (LOWBYTES-CODBYTES)

/*  the row in dwords, for RC_CL_WORDOUT's capacity test */
#define OUTWCAP ((SKIP+OUTCAP)/4)

#define sTOP 0x01000000u
#define gTOP 0x00010000u

#if RC_RANGE64
 typedef uint64_t rangetype;
 #define RANGE_INIT (1UL<<32)
#else
 typedef uint32_t rangetype;
 #define RANGE_INIT 0xFFFFFFFFu
#endif

/*  --------------------------------------------------------------------------- */
/*   The low accumulator, LOWBITS wide. */
/*  */
/*   RC_LOWSPLIT holds it as two 32-bit halves instead of one 64-bit word -- */
/*   rc.inc's arrangement, and sh_v1xN.inc's. It is stream-neutral either way. */
/*   On this CPU device it measures a shade slower, same as on the host; it is */
/*   here because a device without native 64-bit integers has no such choice, */
/*   and that is most GPUs. */
/*  */
/*   Every variant is written out as a function under its own name and the #if */
/*   at the end of the section picks one. A coder function becomes a macro body */
/*   and a macro body cannot hold #if, so this is the only shape the choice can */
/*   take -- and it beats four hand-written blocks of backslashes. */
/*  */
/*   The unpicked variants are still defined, as macros nothing expands. That is */
/*   why the split ones may name lowl and lowh while the other pair names low: */
/*   only one pair is ever reached. */
/*  --------------------------------------------------------------------------- */

/*  Both masks unconditionally: they are constants, and the variants that use */
/*  them are compiled only when expanded. */
#if LOWBYTES==8
 #define LOW_MASK  0xFFFFFFFFFFFFFFFFUL
 #define LOWH_MASK 0xFFFFFFFFu
#else
 #define LOW_MASK  ((1UL<<LOWBITS)-1UL)
 #define LOWH_MASK ((1u<<(LOWBITS-32))-1u)   /* 0 at LOWBYTES==4 */
#endif

/*  low += rpre, with cv left holding the carry out of LOWBITS. cv is an out */
/*  parameter -- these are macros, so the type on it is only there to be parsed. */
/*  */
/*  The LOWBYTES==8 / <8 split is rc.inc's low_Add: at 8 there is no bit left */
/*  above the accumulator to hold the overflow, so the test has to be on the */
/*  addition itself. */

#define low_addc_split8(cv) {\
  uint32_t _cf = (lowl+rpre < lowl);\
  lowl += rpre;\
  (cv) = (lowh+_cf < lowh);\
  lowh += _cf;\
\
}

#define low_addc_splitn(cv) {\
  uint32_t _cf = (lowl+rpre < lowl);\
  lowl += rpre;\
  lowh += _cf;\
  (cv) = (lowh > LOWH_MASK);\
  lowh &= LOWH_MASK;\
\
}

#define low_addc_word8(cv) {\
  uint64_t _t = low + rpre;\
  (cv) = (_t < low);\
  low = _t;\
\
}

#define low_addc_wordn(cv) {\
  uint64_t _t = low + rpre;\
  (cv) = (_t > LOW_MASK);\
  low = _t & LOW_MASK;\
\
}

/*  sh is 0, 8 or 16. The select in the split version is not an optimisation: a */
/*  32-bit shift by 32-sh with sh==0 is a shift by 32, which OpenCL takes modulo */
/*  32 and turns into a shift by nothing at all. */

#define low_shl_split(sh) {\
  uint32_t _hi = ((sh)==0) ? 0u : (lowl >> (32-(sh)));\
  lowh = ((lowh<<(sh)) | _hi) & LOWH_MASK;\
  lowl <<= (sh);\
\
}

#define low_shl_word(sh) {\
  low = (low<<(sh)) & LOW_MASK;\
\
}

/*  Which variant. LOW_DECL and the three below it stay ordinary defines: a */
/*  declaration must land in the caller's scope rather than inside a block, and */
/*  the other three are expressions, which a generated macro -- a { } body -- */
/*  cannot be. */
#if RC_LOWSPLIT

 #define LOW_DECL uint32_t lowl = 0, lowh = 0
 #define LOW_SHL(sh) low_shl_split(sh)
 #if LOWBYTES==8
  #define LOW_ADDC(cv) low_addc_split8(cv)
 #else
  #define LOW_ADDC(cv) low_addc_splitn(cv)
 #endif

 /*  the top two bytes. Which half they live in depends on LOWBYTES: the top */
 /*  byte sits at LOWBITS-8, which is in lowh once LOWBITS-8 >= 32. */
 #if LOWBYTES>=5
  #define LOW_B0() (lowh>>(LOWBITS-8-32))
 #else
  #define LOW_B0() (lowl>>(LOWBITS-8))
 #endif
 #if LOWBYTES>=6
  #define LOW_B1() (lowh>>(LOWBITS-16-32))
 #else
  #define LOW_B1() (lowl>>(LOWBITS-16))
 #endif

 #define LOW_GET() (((uint64_t)lowh<<32) | lowl)

#else

 #define LOW_DECL uint64_t low = 0
 #define LOW_SHL(sh) low_shl_word(sh)
 #if LOWBYTES==8
  #define LOW_ADDC(cv) low_addc_word8(cv)
 #else
  #define LOW_ADDC(cv) low_addc_wordn(cv)
 #endif

 #define LOW_B0() ((uint32_t)(low>>(LOWBITS- 8)))
 #define LOW_B1() ((uint32_t)(low>>(LOWBITS-16)))
 #define LOW_GET() (low)

#endif

/*  --------------------------------------------------------------------------- */
/*   The coder. Everything from here to the kernel becomes a macro. */
/*  --------------------------------------------------------------------------- */

/*  --------------------------------------------------------------------------- */
/*   The output stage. */
/*  */
/*   The row is laid out as the host's: SKIP bytes of zero prefix, then the */
/*   payload. The prefix is stored rather than tested for -- same as RC_IO::put */
/*   -- and outlen reports the payload alone. Past the capacity the substream */
/*   did not fit, and nout keeps counting so the host can see by how much. */
/*  */
/*   RC_CL_WORDOUT picks how a lane's bytes reach its row. */
/*  */
/*   A byte store to a per-lane address does not vectorise, at all: there is no */
/*   byte scatter in AVX-512, and no other SIMD ISA has one either. So when the */
/*   runtime vectorises across work-items, each one becomes extract the address */
/*   out of the vector, scalar store, test the lane's mask bit, branch -- once */
/*   per lane, per byte. Disassembling the built kernel: 517 conditional */
/*   branches and 347 vector extracts feeding 76 scalar byte stores, for a coder */
/*   whose arithmetic is maybe forty instructions. It is why a wider work-group */
/*   measures slower rather than faster. */
/*  */
/*   vpscatterdd does exist. So the word path buffers each lane's bytes in a */
/*   64-bit accumulator and lets a whole dword go at once, to a dword-aligned */
/*   per-lane slot -- one masked instruction for every lane, no branch. What it */
/*   costs is the accumulator, a byte count, and flushing the last few bytes at */
/*   the end of the block. */
/*  --------------------------------------------------------------------------- */

/*  Append win (nsh bytes, low end first) to the lane's output. */
/*  */
/*  Everything here is 32-bit on purpose. A 64-bit value costs two zmm registers */
/*  at sixteen lanes, so the compiler splits every operation on it with */
/*  vextracti64x4 and widens whatever feeds it with vpmovzxdq -- and a 64-bit */
/*  *address* is worse still, because it turns the store into vpscatterqd, */
/*  scatter by 64-bit index, which cannot hold sixteen indices in one register */
/*  either. So the row is addressed as a uniform base and a 32-bit index rather */
/*  than a per-lane pointer, and the accumulator holds four bytes rather than */
/*  eight. */
/*  */
/*  The partial dword is stored every time and rewritten until it is full, which */
/*  is what lets four bytes be enough: there is never a fifth byte to hold, only */
/*  a spill to carry into the next slot. Bytes past nout in the last slot are */
/*  rubbish the host never reads, the same as the byte path's spare byte. */
#define rc_emit(win,nsh) {\
  uint32_t _sh = nacc*8;\
  uint32_t _hi = (_sh==0) ? 0u : ((win) >> (32-_sh));\
  acc |= (win) << _sh;\
\
  if( wpos < OUTWCAP ) ow[owbase+wpos] = acc;\
\
  nacc += (nsh);\
  nout += (nsh);\
\
  uint32_t _full = (nacc>=4);\
  wpos += _full;\
  acc   = _full ? _hi : acc;\
  nacc -= _full*4;\
\
}

#define rc_put_word(c) {\
  rc_emit( (c)&0xFF, 1 );\
\
}

#define rc_put_byte(c) {\
  if( nout < SKIP+OUTCAP ) ob[obase+nout] = (uint8_t)(c);\
  nout++;\
\
}

/*  nothing to do: the word path has already stored the partial dword */
#define rc_tail_word() {\
\
}

#define rc_tail_byte() {\
\
}

#if RC_CL_WORDOUT
 #define RC_OUT_DECL uint32_t acc = 0, nacc = 0, wpos = 0
 #define rc_put(c) rc_put_word(c)
 #define RC_TAIL() rc_tail_word()
#else
 #define RC_OUT_DECL uint32_t _unused_out = 0
 #define rc_put(c) rc_put_byte(c)
 #define RC_TAIL() rc_tail_byte()
#endif

/*  low += rpre, then nsh bytes out of the top. A carry out of the accumulator */
/*  only raises the flag: it has LOWBYTES-CODBYTES bytes of headroom to travel */
/*  through before it could reach an already-emitted byte, and if it ever gets */
/*  out the host re-codes the block with the carry-propagating twin. */
/*  */
/*  sh_v1xN.inc's ShiftLowN: store the window unconditionally and advance the */
/*  cursor by nsh, rather than loop nsh times. nsh is 0, 1 or 2 and differs */
/*  between lanes, so the loop was the one place the coder diverged -- every */
/*  lane paying for the widest. Both bytes go out every time; at nsh<2 the */
/*  cursor does not move past the second, so the next store overwrites it. The */
/*  last one can leave a byte of rubbish just past the substream, inside the */
/*  row's padding and past the length the host is told, which is why nothing */
/*  reads it. */
/*  */
/*  sh_v1xN.inc writes backwards, which is what lets it do this in one 16-bit */
/*  store; that layout would reverse the substream against the host coder and */
/*  the decoder, so here it is two byte stores going forwards. The branch is */
/*  what cost, not the store width -- a single vstore2 measured slower. */
#define rc_shiftlow_byte(nsh) {\
  uint32_t carry;\
  LOW_ADDC(carry);\
  uint32_t b0 = LOW_B0();\
  uint32_t b1 = LOW_B1();\
  ffnum += carry;\
  rpre = 0;\
  if( nout < SKIP+OUTCAP ) {\
    ob[obase+nout  ] = (uint8_t)b0;\
    ob[obase+nout+1] = (uint8_t)b1;\
  }\
  nout += (nsh);\
  LOW_SHL((nsh)*8);\
\
}

/*  The same window, appended to the accumulator instead of stored. Here the */
/*  spare byte has to be masked off rather than overwritten: nothing comes along */
/*  behind it to do that. */
#define rc_shiftlow_word(nsh) {\
  uint32_t carry;\
  LOW_ADDC(carry);\
  uint32_t b0 = LOW_B0();\
  uint32_t b1 = LOW_B1();\
  ffnum += carry;\
  rpre = 0;\
  uint32_t _w = (((b0)&0xFF) | (((b1)&0xFF)<<8)) & ((1u<<((nsh)*8))-1u);\
  rc_emit( _w, (nsh) );\
  LOW_SHL((nsh)*8);\
\
}

#if RC_CL_WORDOUT
 #define rc_shiftlow(nsh) rc_shiftlow_word(nsh)
#else
 #define rc_shiftlow(nsh) rc_shiftlow_byte(nsh)
#endif

/*  The optional tail loop, the same shape as the LOW_ variants: both written */
/*  out, the #if picks. It goes after rc_shiftlow because it calls it. */
#define rc_renorm_tail_on() {\
  while( range<sTOP ) { rc_shiftlow(1); range <<= 8; }\
\
}

#define rc_renorm_tail_off() {\
\
}

#if RC_RENORM_TAIL
 #define RC_RENORM_TAIL_LOOP() rc_renorm_tail_on()
#else
 #define RC_RENORM_TAIL_LOOP() rc_renorm_tail_off()
#endif

/*  The counted 0/1/2-byte shift is exact for a binary coder: the unit is */
/*  range>>SCALElog >= 2^9, so one step cannot take range below 2^8. */
#define rc_renorm() {\
  uint32_t _nsh = (range<sTOP) + (range<gTOP);\
  rc_shiftlow(_nsh);\
  range <<= _nsh*8;\
  RC_RENORM_TAIL_LOOP();\
\
}

#define rc_process(freq,bit) {\
  uint32_t _b = (bit);\
  rc_renorm();\
  rpre = (uint32_t)(range>>SCALElog);\
  rpre *= (freq);\
  range -= rpre;\
  range = _b ? range : (rangetype)rpre;\
  rpre &= (uint32_t)(-(int)_b);\
\
}

/*  The minimal flush: set as many low bytes as still leave low < high and drop */
/*  them, because the decoder reads 0xFF past the end of a substream and rc_Read */
/*  pads with exactly that. */
#define rc_quit() {\
  rc_renorm();\
  uint32_t _nn = LOWBYTES;\
  uint64_t _lo = LOW_GET();\
  uint64_t _hi = _lo + (uint64_t)range;\
  uint32_t _hc = (_hi<_lo);   /* _lo+range can leave the uint64_t at LOWBYTES==8 */\
  for( uint32_t _i=0; _i<LOWBYTES; _i++ ) {\
    uint64_t _m = ((1UL<<((_i+1)*4))<<((_i+1)*4))-1UL;\
    if( _hc || ((_lo|_m)<_hi) ) { _lo |= _m; _nn--; }\
  }\
  for( uint32_t _i=0; _i<_nn; _i++ ) rc_put( _lo>>(LOWBITS-8-_i*8) );\
\
}

/*  --------------------------------------------------------------------------- */
/*   One block, RCNUM work-items. Lane id codes, in this order: the block-length */
/*   flag if it is lane 0, then the length bits that fall to it, then every bit */
/*   of the block whose index is id mod RCNUM -- which is model0.inc's order, */
/*   bit for bit. */
/*  */
/*   Not turned into a macro: rc_macro.pl only converts `type name(args) {`, and */
/*   this signature spans lines. */
/*  --------------------------------------------------------------------------- */
__kernel void rc_encode(
    __global const uint16_t* pbit,   /*  nbits entries, packed (bit<<15)|p */
    const uint32_t nbits,
    const uint32_t blksize,
    __global uint8_t* out,           /*  RCNUM rows of OUTSTRIDE */
    __global uint32_t* outlen,
    __global uint32_t* outcarry )
{
  const uint32_t id = get_global_id(0);

  LOW_DECL;
  RC_OUT_DECL;
  rangetype range = RANGE_INIT;
  uint32_t rpre = 0, ffnum = 0, nout = 0;
  /*  uniform bases; the per-lane part is a 32-bit index, so the scatter is by */
  /*  32-bit index (vpscatterdd) and not by 64-bit pointer (vpscatterqd) */
  __global uint8_t*  ob = out;
  __global uint32_t* ow = (__global uint32_t*)out;
  const uint32_t obase  = id*OUTSTRIDE;
  const uint32_t owbase = id*(OUTSTRIDE/4);

  const uint32_t flag = (blksize!=BLKFULL);

  if( id==0 ) rc_process( hSCALE, flag );

  if( flag )
    for( uint32_t j=id; j<16; j+=RCNUM )
      rc_process( hSCALE, (blksize>>(15-j))&1 );

  for( uint32_t k=id; k<nbits; k+=RCNUM ) {
    uint32_t b = pbit[k];
    rc_process( b&0x7FFF, b>>15 );
  }

  rc_quit();
  RC_TAIL();

  /*  a lane that coded nothing can stop inside the prefix and owes nothing */
  outlen[id]   = nout>SKIP ? nout-SKIP : 0;
  outcarry[id] = ffnum;
}
