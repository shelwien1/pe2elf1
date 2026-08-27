/*   rc_kernel.c -- the device-side coder, as functions. */
/*  */
/*   Named .c rather than .ispc because that is what it is to an editor: C, */
/*   which is close enough to ISPC's language to highlight and index properly. */
/*   It is the source; rc_kernel.ispc is generated from it and compiled by ispc */
/*   at build time straight into the executable. mk_kernel.sh runs the chain, */
/*   build.sh runs mk_kernel.sh when this file is newer: */
/*  */
/*     rc_kernel.c  --rc_macro.pl-->  functions turned into #define/#enddef */
/*                  --defines.pl -->  ... into real multi-line macros */
/*                                    ... and that is rc_kernel.ispc */
/*  */
/*   The coder is macros for the same reason sh_v1xN_macro.inc's is: a lane's */
/*   state has to be plain private scalars, with no pointer to an aggregate */
/*   anywhere for the vectoriser to give up on. Writing them as functions here */
/*   and generating the macros keeps that without hand-maintained backslashes. */
/*  */
/*   So the coder functions below take only their real arguments and reach the */
/*   lane state -- low, range, rpre, ffnum, nout, o -- by name. They are macro */
/*   bodies, not functions, however much they look like functions; that is the */
/*   one thing to keep in mind when editing this file. */
/*  */
/*   Two entry points: rc_encode, the carryless coding pass over a block's */
/*   cached {p;bit} pairs, and rc_decode_batch, the decoder split's per-group */
/*   lane update (see rc_split_ispc_v1.md). The carry-propagating twin is a */
/*   serial Cache/FF-run state machine and stays on the host; so does the */
/*   decoder's model, always. */
/*  */
/*   What the host and the kernel have to agree on comes over as -D from */
/*   build.sh, which maps the RC_* configuration to these names: RCNUM, */
/*   SCALElog, hSCALE, LOWBYTES, CODBYTES, RC_LOWSPLIT, BLKFULL, RC_DEV_WORDOUT, */
/*   RC_RANGE64, RC_RENORM_TAIL. OUTSTRIDE and OUTCAP arrive as arguments. */
/*  */
/*   The output must be byte-identical to the host coder -- it is the same */
/*   integer arithmetic in the same order -- and t.sh checks exactly that by */
/*   comparing a device encode against -C. */

/*  ISPC has no <stdint.h>; these are a spelling, so the kernel reads like the */
/*  host code next to it. One thing to know when editing: an ISPC integer */
/*  literal suffixed UL is still 32 bits, so every 64-bit constant below is */
/*  built with an explicit (uint64_t) cast. */
typedef unsigned int8  uint8_t;
typedef unsigned int16 uint16_t;
typedef unsigned int32 uint32_t;
typedef unsigned int64 uint64_t;

#define LOWBITS (LOWBYTES*8)
#define SKIP    (LOWBYTES-CODBYTES)

/*  the row in dwords, for RC_DEV_WORDOUT's capacity test */
#define OUTWCAP ((SKIP+OUTCAP)/4)

#define sTOP 0x01000000u
#define gTOP 0x00010000u

#if RC_RANGE64
 typedef uint64_t rangetype;
 #define RANGE_INIT ((uint64_t)1<<32)
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
 #define LOW_MASK  ((uint64_t)0-1)
 #define LOWH_MASK 0xFFFFFFFFu
#else
 #define LOW_MASK  (((uint64_t)1<<LOWBITS)-1)
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
/*  32-bit shift by 32-sh with sh==0 is a shift by 32, which shift semantics */
/*  take modulo 32 and turn into a shift by nothing at all. */

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
/*   RC_DEV_WORDOUT picks how a lane's bytes reach its row. */
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

#if RC_DEV_WORDOUT
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

#if RC_DEV_WORDOUT
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
  /* ternaries because an ISPC comparison is a bool, and bool+bool\
     does not parse */\
  uint32_t _nsh = (range<sTOP ? 1u:0u) + (range<gTOP ? 1u:0u);\
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
  uint64_t _qh = _lo + (uint64_t)range;\
  uint32_t _hc = (_qh<_lo);   /* _lo+range can leave the uint64_t at LOWBYTES==8 */\
  for( uint32_t _i=0; _i<LOWBYTES; _i++ ) {\
    uint64_t _m = (((uint64_t)1<<((_i+1)*4))<<((_i+1)*4))-1;\
    if( _hc || ((_lo|_m)<_qh) ) { _lo |= _m; _nn--; }\
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
/*  One block, RCNUM lanes: `foreach` walks them through the gang a */
/*  gang-width at a time, so the same code covers RCNUM above and below the */
/*  gang size. Lane id codes, in this order: the block-length flag if it is */
/*  lane 0, then the length bits that fall to it, then every bit of the block */
/*  whose index is id mod RCNUM -- which is model0.inc's order, bit for bit. */
/*  */
/*  Inside foreach the induction variable is known linear, so `pbit[k]` with */
/*  k = id + m*RCNUM is a unit-stride access and ispc emits one contiguous */
/*  vector load, not a gather. (The retired OpenCL backend needed the Intel */
/*  sub-group block-read extension for the same effect.) */
/*  */
/*  OUTSTRIDE and OUTCAP arrive as arguments rather than -D: they come from */
/*  RCio's template geometry, which the C++ side computes, and a uniform */
/*  argument costs the kernel nothing while sparing the build a second copy of */
/*  that arithmetic. */
export void rc_encode(
    const uniform uint16_t pbit[],   /*  nbits entries, packed (bit<<15)|p */
    uniform uint32_t nbits,
    uniform uint32_t blksize,
    uniform uint8_t  out[],          /*  RCNUM rows of OUTSTRIDE */
    uniform uint32_t outlen[],
    uniform uint32_t outcarry[],
    uniform uint32_t OUTSTRIDE,
    uniform uint32_t OUTCAP )
{
  uniform uint8_t*  uniform ob = out;
  uniform uint32_t* uniform ow = (uniform uint32_t* uniform)out;

  foreach( id = 0 ... RCNUM ) {

    LOW_DECL;
    RC_OUT_DECL;
    rangetype range = RANGE_INIT;
    uint32_t rpre = 0, ffnum = 0, nout = 0;
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

    outlen[id]   = nout>SKIP ? nout-SKIP : 0;
    outcarry[id] = ffnum;
  }
}

/*  --------------------------------------------------------------------------- */
/*   The decoder split's batch: one 16-lane pass of everything the per-bit */
/*   scalar loop deferred. The scalar side (rc_vec.inc) decides each bit with */
/*   one compare, p <= T[lane] -- exact, because for integers */
/*   code >= (range>>SCALElog)*p is p <= floor(code/(range>>SCALElog)) -- and */
/*   logs {p;bit}; every 16 bits this consumes the log and brings the 16 lanes */
/*   of that group current: the range/code update, the counted renorm, the */
/*   refill, and the next T. See rc_split_ispc_v1.md for the design and the */
/*   measured cost model. */
/*  */
/*   The {p;bit} pairs arrive packed in four u64 arguments -- by value, so that */
/*   direction of the scalar/vector handoff never touches memory. pos[] holds */
/*   absolute 32-bit offsets into base, which is the whole slot's row block, so */
/*   no per-lane pointer exists anywhere. look[] is each lane's next four */
/*   stream bytes, loaded at the END of the previous batch: the refill consumes */
/*   it before the gather that refreshes it, which moves the gather's latency */
/*   off this batch's chain and under the caller's next scalar phase. Reading a */
/*   few bytes past a substream's end lands in rc_Read's FF padding, same as */
/*   the fused decoder's own overreads. */
/*  */
/*   The division is exact: code and ru are exactly representable in a double, */
/*   the quotient is below 2^23, and the true quotient is at least 2^-17 away */
/*   from the next integer while the rounding error is below 2^-30 -- so */
/*   floor() can only be one short, and the single conditional increment */
/*   repairs it. The state fingerprint checks in misc/bench/rc_splitdec.cpp */
/*   verify the whole arrangement bit for bit against the fused coder. */
/*  */
/*   Not turned into a macro: the signature spans lines, same as rc_encode. */
export void rc_decode_batch(
    uniform uint32_t range[], uniform uint32_t code[], uniform uint32_t pos[],
    uniform uint32_t ru[],    uniform uint32_t T[],    uniform uint32_t look[],
    const uniform uint8_t base[],
    uniform uint64_t pk0, uniform uint64_t pk1,
    uniform uint64_t pk2, uniform uint64_t pk3 )
{
  uint32_t r = range[programIndex];
  uint32_t c = code[programIndex];
  uint32_t q = pos[programIndex];
  uint32_t u = ru[programIndex];
  uint32_t d = look[programIndex];

  uint64_t pks = programIndex<4 ? pk0 : programIndex<8 ? pk1
               : programIndex<12 ? pk2 : pk3;
  uint32_t pbv = (uint32_t)((pks>>((programIndex&3)*16)) & 0xFFFF);
  uint32_t p   = pbv>>1;
  uint32_t bit = pbv&1;

  uint32_t rp = u*p;
  r = bit ? r-rp : rp;
  c = bit ? c-rp : c;

  uint32_t n  = (r<sTOP ? 1u:0u) + (r<gTOP ? 1u:0u);
  uint32_t sh = n*8;
  uint32_t w  = ((d&0xFF)<<8) | ((d>>8)&0xFF);
  c = (c<<sh) + (w>>(16-sh));
  r = r<<sh;
  q = q+n;

  look[programIndex] = *((const uint32_t*)(&base[q]));

  u = r>>15;
  double fq  = (double)c/(double)u;
  uint32_t t = (uint32_t)floor(fq);
  uint32_t dd = c - t*u;
  if( dd>=u ) t += 1;

  range[programIndex]=r; code[programIndex]=c; pos[programIndex]=q;
  ru[programIndex]=u; T[programIndex]=t;
}
