// The batched half of the split decoder -- see rc_splitdec.cpp for the
// contract. Two entry points measuring two integration granularities:
//   rc_batch  -- one 16-lane update+renorm+refill+threshold pass; the caller
//                keeps the model and calls this every 16 bits.
//   rc_block  -- the whole decode loop, model included: the model chain runs
//                as uniform (scalar) code, the lane state lives in varying
//                registers across the whole call. One call per block.
typedef unsigned int8  u8;
typedef unsigned int16 u16;
typedef unsigned int32 u32;
typedef unsigned int64 u64;

static inline void batch_step( u32& r, u32& c, u32& q, u32& u, u32& t,
                               uniform const u8 strm[], uniform u32 stride,
                               u32 pbv )
{
  u32 p   = pbv>>1;
  u32 bit = pbv&1;
  u32 rp  = u*p;
  r = bit ? r-rp : rp;
  c = bit ? c-rp : c;
  u32 n  = (r<0x01000000u ? 1u:0u) + (r<0x00010000u ? 1u:0u);
  u32 sh = n*8;
  u32 j  = programIndex;
  // one dword gather instead of two byte gathers (AVX-512 has no 16-bit
  // gather); the two stream bytes are the low two of the loaded dword
  u32 d  = *((const u32*)(&strm[j*stride+q]));
  u32 w  = ((d&0xFF)<<8)|((d>>8)&0xFF);
  c = (c<<sh) + (w>>(16-sh));
  r = r<<sh;
  q = (q+n)&(stride-1);
  u = r>>15;
  double fq = (double)c/(double)u;
  t = (u32)floor(fq);
  u32 dd = c - t*u;
  if( dd>=u ) t+=1;
}

export void rc_batch( uniform u32 range[], uniform u32 code[], uniform u32 pos[],
                      uniform u32 ru[], uniform u32 T[],
                      uniform const u8 strm[], uniform u32 stride,
                      uniform const u16 pb[] )
{
  u32 r=range[programIndex], c=code[programIndex], q=pos[programIndex];
  u32 u=ru[programIndex],    t=T[programIndex];
  u32 pbv = pb[programIndex];
  batch_step( r,c,q,u,t, strm,stride, pbv );
  range[programIndex]=r; code[programIndex]=c; pos[programIndex]=q;
  ru[programIndex]=u; T[programIndex]=t;
}

export void rc_block( uniform u32 range[], uniform u32 code[], uniform u32 pos[],
                      uniform u32 ru[], uniform u32 T[],
                      uniform const u8 strm[], uniform u32 stride,
                      uniform u16 cst[], uniform const u16 fsm2[],
                      uniform const u16 ppt[], uniform u32 ctx_io[],
                      uniform u32 ngroups )
{
  u32 r=range[programIndex], c=code[programIndex], q=pos[programIndex];
  u32 u=ru[programIndex],    t=T[programIndex];
  uniform u32 Ta[16];
  uniform u16 pbu[16];
  Ta[programIndex] = t;
  uniform u32 ctx = ctx_io[0];
  for( uniform u32 g=0; g<ngroups; g++ ) {
    // model chain + decision: uniform (scalar) code, exactly the fused shape
    for( uniform u32 j=0; j<16; j++ ) {
      if( (j&7)==0 ) ctx = 1;
      uniform u32 st = cst[ctx];
      uniform u32 p  = ppt[st];
      uniform u32 bit = (p<=Ta[j]) ? 1u : 0u;
      pbu[j] = (u16)((p<<1)|bit);
      cst[ctx] = fsm2[st*2+bit];
      ctx = ctx*2+bit;
    }
    u32 pbv = pbu[programIndex];
    batch_step( r,c,q,u,t, strm,stride, pbv );
    Ta[programIndex] = t;
  }
  range[programIndex]=r; code[programIndex]=c; pos[programIndex]=q;
  ru[programIndex]=u; T[programIndex]=t;
  ctx_io[0]=ctx;
}

export void rc_batch2( uniform u32 range[], uniform u32 code[], uniform u32 pos[],
                       uniform u32 ru[], uniform u32 T[],
                       uniform u32 look[], uniform u16 Tc[],
                       uniform const u8 strm[], uniform u32 stride,
                       uniform u64 pk0, uniform u64 pk1, uniform u64 pk2, uniform u64 pk3 )
{
  u32 r=range[programIndex], c=code[programIndex], q=pos[programIndex];
  u32 u=ru[programIndex];
  u64 pks = programIndex<4 ? pk0 : programIndex<8 ? pk1 : programIndex<12 ? pk2 : pk3;
  u32 pbv = (u32)((pks>>((programIndex&3)*16)) & 0xFFFF);
  u32 p   = pbv>>1;
  u32 bit = pbv&1;
  u32 rp  = u*p;
  r = bit ? r-rp : rp;
  c = bit ? c-rp : c;
  u32 n  = (r<0x01000000u ? 1u:0u) + (r<0x00010000u ? 1u:0u);
  u32 sh = n*8;
  u32 d  = look[programIndex];
  u32 w  = ((d&0xFF)<<8)|((d>>8)&0xFF);
  c = (c<<sh) + (w>>(16-sh));
  r = r<<sh;
  q = (q+n)&(stride-1);
  // refill the lookahead for the NEXT call -- the gather's latency overlaps
  // the caller's next scalar phase instead of this batch's chain
  look[programIndex] = *((const u32*)(&strm[programIndex*stride+q]));
  u = r>>15;
  double fq = (double)c/(double)u;
  u32 t = (u32)floor(fq);
  u32 dd = c - t*u;
  if( dd>=u ) t+=1;
  range[programIndex]=r; code[programIndex]=c; pos[programIndex]=q;
  ru[programIndex]=u; T[programIndex]=t;
  (void)Tc;
}
