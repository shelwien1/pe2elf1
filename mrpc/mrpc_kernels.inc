// -------------------------------------------------------------
// mrpc_kernels.inc -- the OpenCL C side of mrpc, as a string.
//
// Kept in the binary rather than in a .cl file next to it: the program is
// built at run time anyway, and a codec that cannot find its kernels is a
// codec that cannot compress.  What the kernels have to agree with the
// host about comes over in two ways, and which way is the difference
// between a program that is built once and one that is built per image:
//
//   -D on the build line, from MrpcCL::Build, for what is a property of
//   the codec -- tap counts, table strides, the padding of the bordered
//   raster, the block size the cube is cut into.  These are the same for
//   every image mrpc will ever be handed, and they are the ones worth
//   having as constants: they are the strides the device compiler folds
//   into addresses, and passing them as arguments instead measured 4x
//   slower, because it can no longer prove that neighbouring work-items
//   touch neighbouring memory.
//
//   kernel arguments for what is a property of the image -- the
//   component count, the row stride of the bordered raster, the size of
//   a plane.  None of those is a stride the vectorizer cares about: they
//   multiply row and plane numbers, which are uniform across the
//   vectorized dimension, and the address still moves one element per
//   work-item.
//
// The tap loop runs to NTMAX rather than to the image's actual tap count
// so that it stays a constant: coefficients and tap offsets past the end
// are zero, so the extra terms are zero, and 24 percent more prediction
// work on a 3-component image buys a program that a 4-component one can
// use too.
//
// The kernels mirror the host functions they replace:
//
//   predict_cube  SetPrdBuf, for every class at once, into a tile that
//                 carries the activity halo with it
//   pixel_cost    the body of CalcCost, per (class, pixel)
//   batch_block   the same two together, for the first optimisation
//                 loop's small blocks -- a chunk of blocks per launch
//                 instead of one, and the tile kept private
//   coef_scan     the 11x3 candidate sweep of OptimizeCoef, for every
//                 class at once -- classes touch disjoint pixels and
//                 disjoint coefficients, so nothing is reordered by it
//   coef_apply    the prd/err fixup that follows a coefficient change
//   group_hist    the histogram OptimizeGroup runs its DP on
//   group_dp      the threshold search it runs on that histogram
//   pmodel_cost   the shape choice OptimizeGroup makes after it
// -------------------------------------------------------------

static const char* MRPC_CL_SRC = R"CLC(

// ---- helpers ------------------------------------------------------
inline int cl_clip(int v) {
  return (v<0) ? 0 : ((v>MAXPRD) ? MAXPRD : v);
}

// |2*org - prd_half| with MRP's econv twist
inline int cl_econv(int org_, int prdc) {
  int j = prdc>>(COEF_PREC-1);
  int t = (org_<<1)-j;
  return (t>0) ? t-1 : -t;
}

// base and subposition of the pmf window for a clamped prediction
inline int cl_qprd(int prdc) {
  return (MAXPRD-prdc+(1<<(COEF_PREC-PM_ACC-1)))>>(COEF_PREC-PM_ACC);
}

// ---- the class search --------------------------------------------
//
// One work-item per (tile cell, group of CLGRP classes).  Cells inside
// the block are predicted under each candidate class; cells outside it --
// the activity halo, up to UPEL_DIST away -- are copied from the
// committed error plane the host uploaded.  After this the cost kernel
// reads one tile per class with no bounds tests and no idea where the
// block ends.
//
// The classes are batched because they share their input: a tap value is
// one load feeding CLGRP multiply-accumulates, where a class-per-item
// kernel reloads it CLGRP times.  Prediction here is a small dense
// matrix product -- classes by taps, against taps by pixels -- and this
// is the register tiling that a matrix product wants.
#define CLGRP 8

__kernel void predict_cube(__global const uchar* planes,
                           __global const int*   coef,
                           __global const int*   tapoff,
                           __global const short* halo,
                           __global short*       prdcube,
                           __global short*       errcube,
                           const int tly, const int tlx,
                           const int bh,  const int bw,
                           const int ncls,
                           const int nc,  const int bsw, const int bn)
{
  const int tx = get_global_id(0);
  const int ty = get_global_id(1);
  const int c0 = get_global_id(2)*CLGRP;

  const int y = ty-HT_T; // block-relative
  const int x = tx-HT_L;

  if( !((y>=0)&(y<bh)&(x>=0)&(x<bw)) ) {
    for( int k = 0; k<nc; k++ ) {
      const short h = halo[(k*TH+ty)*TW+tx];
      for( int c = 0; c<CLGRP; c++ )
        if( c0+c<ncls )
          errcube[(((c0+c)*nc+k)*TH+ty)*TW+tx] = h;
    }
    return;
  }

  const int o = (tly+y+PADT)*bsw+(tlx+x+PADL);
  for( int k = 0; k<nc; k++ ) {
    int acc[CLGRP];
#pragma unroll
    for( int c = 0; c<CLGRP; c++ )
      acc[c] = 0;
    __global const int* cf = coef+(c0*MAXC+k)*NTMAX;
    __global const int* to = tapoff+k*NTMAX;
    for( int i = 0; i<NTMAX; i++ ) {
      const int v = (int)planes[o+to[i]];
#pragma unroll
      for( int c = 0; c<CLGRP; c++ )
        acc[c] += v*cf[c*MAXC*NTMAX+i];
    }
    const int org_ = (int)planes[k*bn+o];
    for( int c = 0; c<CLGRP; c++ ) {
      if( c0+c>=ncls )
        break;
      const int p = cl_clip(acc[c]);
      prdcube[(((c0+c)*nc+k)*BSZ+y)*BSZ+x] = (short)p;
      errcube[(((c0+c)*nc+k)*TH+ty)*TW+tx] = (short)cl_econv(org_, p);
    }
  }
}

// One work-item per (class, pixel): the body of CalcCost.  The tile is
// always MAX_BSIZE square, even where the image ends inside it, so that
// every stride here is a constant the device compiler can fold: with the
// block size in an argument it cannot prove that neighbouring work-items
// touch neighbouring memory, and every load becomes a gather.  bh and bw
// are still passed, but only to bound the work -- a pixel past the edge
// of a clipped block has no prediction in the cube, because predict_cube
// only wrote the ones inside it, and reading one anyway lands an
// arbitrary short in cl_qprd and indexes off the end of a cost table.
// The prefix sum above never reaches these pixels; the device does not
// know that.
//
// The result is left per-pixel rather than summed, because the quadtree
// wants the cost of 341 different rectangles of it and a prefix sum
// answers all of them.
__kernel void pixel_cost(__global const uchar* planes,
                         __global const short* prdcube,
                         __global const short* errcube,
                         __global const uchar* uq,
                         __global const float* costtab,
                         __global const int*   pmoff,
                         __global const int*   udy,
                         __global const int*   udx,
                         __global const int*   uw,
                         __global float*       pixcost,
                         const int tly, const int tlx,
                         const int bh,  const int bw,
                         const int nc,  const int bsw, const int bn)
{
  const int x = get_global_id(0);
  const int y = get_global_id(1);
  const int cl = get_global_id(2);
  if( (y>=bh)|(x>=bw) ) {
    pixcost[(cl*BSZ+y)*BSZ+x] = 0.0f;
    return;
  }
  const int ty = y+HT_T, tx = x+HT_L;
  const int o = (tly+y+PADT)*bsw+(tlx+x+PADL);
  float c = 0.0f;

  for( int k = 0; k<nc; k++ ) {
    __global const short* e = errcube+((cl*nc+k)*TH+ty)*TW+tx;
    int u = 0;
#pragma unroll
    for( int i = 0; i<NUM_UPELS; i++ )
      u += (int)e[udy[i]*TW+udx[i]]*uw[i];
#if XUPEL
    // this pixel's earlier components: if red just missed by 30, green is
    // about to as well
    for( int j = 0; j<k; j++ )
      u += (int)errcube[((cl*nc+j)*TH+ty)*TW+tx]*64;
#endif
    u >>= 6;
    u = (u>MAX_UPARA) ? MAX_UPARA : u;

    const int gr = (int)uq[(cl*MAXC+k)*(MAX_UPARA+1)+u];
    const int prdc = (int)prdcube[((cl*nc+k)*BSZ+y)*BSZ+x];
    const int q = cl_qprd(prdc);
    const int base = q>>PM_ACC;
    __global const float* cq = costtab+pmoff[k*MRP_GROUP+gr]+(q&(NUM_SUBPM-1))*CSTRIDE;
    c += cq[base+(int)planes[k*bn+o]]+cq[PMSIZE+base];
  }
  pixcost[(cl*BSZ+y)*BSZ+x] = c;
}

// ---- the first loop's class search, in bulk -----------------------
//
// The second loop's blocks are MAX_BSIZE and carry a quadtree, so they
// are decided one at a time and each one's activity halo is the last
// one's committed errors.  The first loop's are BASE_BSIZE with no
// quadtree under them -- one argmin over classes per block -- and there
// are sixteen times as many.  One launch per block never paid for
// itself, so they were left on the host, where they were the single
// largest item in the encode: 28 GMAC an iteration on one core.
//
// Here they go in chunks instead: every block in the chunk, every class,
// one launch.  What that costs is the halo -- a block's neighbours are
// no longer committed by the time it is measured, so every block sees
// the error planes as the pass started.  The decisions still happen on
// the host, in raster order, so the move-to-front state the class costs
// come from is exactly what it was.
//
// One work-item is one (block, class), and the tile it needs -- 8x8
// predictions and a 13x11 error field with the activity halo around it
// -- lives in private memory for the length of that work-item.  Cutting
// it in two, a predict pass and a cost pass over global cubes, costs
// more than it saves twice over: the halo cells have no prediction to
// make but sit in the same vector as the ones that do, and the cubes
// themselves are gigabytes of traffic an iteration.  Here nothing
// leaves the work-item but one float.
__kernel void batch_block(__global const uchar* planes,
                          __global const int*   coef,
                          __global const int*   tapoff,
                          __global const short* errB,
                          __global const uchar* uq,
                          __global const float* costtab,
                          __global const int*   pmoff,
                          __global const int*   udy,
                          __global const int*   udx,
                          __global const int*   uw,
                          __global float*       blockcost,
                          const int nbx, const int b0, const int nblk,
                          const int ncls, const int nc,
                          const int bsw, const int bn,
                          const int W, const int H)
{
  const int cl = get_global_id(0);
  const int bi = get_global_id(1);
  if( cl>=ncls||bi>=nblk )
    return;
  const int blk = b0+bi;
  const int tly = (blk/nbx)*B8, tlx = (blk%nbx)*B8;
  const int bh = (tly+B8<H) ? B8 : H-tly;
  const int bw = (tlx+B8<W) ? B8 : W-tlx;

  // The activity measure only ever looks up and to the left, so one
  // raster pass can predict a pixel and charge for it at once: nothing
  // it reads is anything this pass has not already written.  Only the
  // error field has to be kept, and only the rows the halo reaches --
  // four of them, rolling.  That is 208 shorts of private memory, which
  // is what makes this fit in a work-item at all.
#define ER (HT_T+1)
#define EW (B8+HT_L+HT_R)
#define EAT(k, yy, xx) et[(((yy)&(ER-1))*MAXC+(k))*EW+(xx)+HT_L]
#if (ER&(ER-1))
#error "the rolling row index masks, so HT_T+1 has to be a power of two"
#endif
  short et[ER*MAXC*EW];

  // the halo above: committed errors, as the pass found them
  for( int y = -HT_T; y<0; y++ ) {
    const int o = ((tly+y+PADT)*bsw+(tlx+PADL))*nc;
    for( int x = -HT_L; x<B8+HT_R; x++ )
      for( int k = 0; k<nc; k++ )
        EAT(k, y, x) = errB[o+x*nc+k];
  }

  float c = 0.0f;
  for( int y = 0; y<bh; y++ ) {
    // the halo to the left and to the right of this row, likewise
    const int oh = ((tly+y+PADT)*bsw+(tlx+PADL))*nc;
    for( int x = -HT_L; x<0; x++ )
      for( int k = 0; k<nc; k++ )
        EAT(k, y, x) = errB[oh+x*nc+k];
    for( int x = bw; x<B8+HT_R; x++ )
      for( int k = 0; k<nc; k++ )
        EAT(k, y, x) = errB[oh+x*nc+k];

    for( int x = 0; x<bw; x++ ) {
      const int o = (tly+y+PADT)*bsw+(tlx+x+PADL);
      for( int k = 0; k<nc; k++ ) {
        __global const int* cf = coef+(cl*MAXC+k)*NTMAX;
        __global const int* to = tapoff+k*NTMAX;
        int v = 0;
        for( int i = 0; i<NTMAX; i++ )
          v += (int)planes[o+to[i]]*cf[i];
        v = cl_clip(v);
        EAT(k, y, x) = (short)cl_econv((int)planes[k*bn+o], v);

        int u = 0;
#pragma unroll
        for( int i = 0; i<NUM_UPELS; i++ )
          u += (int)EAT(k, y+udy[i], x+udx[i])*uw[i];
#if XUPEL
        for( int j = 0; j<k; j++ )
          u += (int)EAT(j, y, x)*64;
#endif
        u >>= 6;
        u = (u>MAX_UPARA) ? MAX_UPARA : u;
        const int gr = (int)uq[(cl*MAXC+k)*(MAX_UPARA+1)+u];
        const int q = cl_qprd(v);
        const int base = q>>PM_ACC;
        __global const float* cq = costtab+pmoff[k*MRP_GROUP+gr]+(q&(NUM_SUBPM-1))*CSTRIDE;
        c += cq[base+(int)planes[k*bn+o]]+cq[PMSIZE+base];
      }
    }
  }
#undef EAT
#undef EW
#undef ER
  blockcost[(size_t)bi*ncls+cl] = c;
}

// ---- the coefficient search ---------------------------------------
//
// OptimizeCoef moves weight between two taps and keeps the best of an
// 11x3 grid of shifts.  The 33 candidates are the reduction; the pixels
// are the parallel part.  Every class is swept in the same launch: a
// class only ever reads its own pixels and writes its own coefficients,
// so the sweeps of two classes cannot see each other, and the sequence
// of shifts any one class goes through is the one the host would have
// made.
//
// Each work-item keeps 33 accumulators over a strided slice and writes
// them out; the host adds up the slices, which is where the side cost --
// absolute bits, not sampled -- goes in.  No barriers: on a CPU device a
// work-group barrier is a fibre switch per work-item, and there is not
// enough left of a sweep to pay for 33 floats' worth of them.
__kernel void coef_scan(__global const uchar* org,
                        __global const short* prd,
                        __global const char*  grp,
                        __global const uint*  cpo,
                        __global const uint*  cpq,
                        __global const float* costtab,
                        __global const int*   pmoff,
                        __global const int*   cA,
                        __global const int*   cB,
                        __global const uint*  cbeg,
                        const int k, const int o1, const int o2,
                        const int workers,
                        __global float* partials)
{
  const int w = get_global_id(0);
  const int cl = get_global_id(1);
  const uint b = cbeg[cl], e = cbeg[cl+1];
  const uint cnt = e-b;
  const uint st = (COEF_MAX&&cnt>COEF_MAX) ? cnt/COEF_MAX : 1;

  float acc[NCAND];
#pragma unroll
  for( int t = 0; t<NCAND; t++ )
    acc[t] = 0.0f;

  for( uint ci = b+(uint)w*st; ci<e; ci += (uint)workers*st ) {
    const uint po = cpo[ci];
    const uint pq = cpq[ci];
    const int d1 = (int)org[po+o1], d2 = (int)org[po+o2];
    const int pf0 = (int)prd[pq+k]-(d1-d2)*(SR>>1)+d2*(SSR>>1);
    const int v0 = (int)org[po+k];
    __global const float* cb0 = costtab+pmoff[k*MRP_GROUP+(int)grp[pq+k]];
#pragma unroll 4
    for( int t = 0; t<NCAND; t++ ) {
      const int pf = pf0+cA[t]*(d1-d2)-cB[t]*d2;
      const int q = cl_qprd(cl_clip(pf));
      const int base = q>>PM_ACC;
      __global const float* cq = cb0+(q&(NUM_SUBPM-1))*CSTRIDE;
      acc[t] += cq[base+v0]+cq[PMSIZE+base];
    }
  }

  __global float* p = partials+(size_t)(cl*workers+w)*NCAND;
  for( int t = 0; t<NCAND; t++ )
    p[t] = acc[t];
}

// The other half of OptimizeCoef: once the shift is chosen, every pixel
// of the class needs its prediction and error moved by it.  prd and errB
// stay on the device between the thousands of sweeps a pass makes; the
// host rebuilds them from the coefficients when the pass ends.
__kernel void coef_apply(__global const uchar* org,
                         __global short*       prd,
                         __global short*       errB,
                         __global const uint*  cpo,
                         __global const uint*  cpq,
                         const uint cbeg, const uint cend,
                         const int  k,    const int  o1, const int o2,
                         const int  di,   const int  dj)
{
  const uint i = cbeg+(uint)get_global_id(0);
  if( i>=cend )
    return;
  const uint po = cpo[i], pq = cpq[i];
  const int v = cl_clip((int)prd[pq+k]+(int)org[po+o1]*di+(int)org[po+o2]*dj);
  prd[pq+k] = (short)v;
  errB[po+k] = (short)cl_econv((int)org[po+k], v);
}

// ---- the group quantiser ------------------------------------------
//
// OptimizeGroup needs, per (class, component, group), the code length of
// every pixel of that class binned by its activity -- MRP_GROUP tables of
// MAX_UPARA+2 bins.  One work-item per (class, component, group) owns a
// whole table, so the accumulation needs no atomics and no local memory;
// the price is that the pixel's index data is read MRP_GROUP times, which
// is cheap next to the table lookups it saves.
//
// The bins are doubles because the host's are: they are sums of tens of
// thousands of code lengths, and the DP downstream decides thresholds on
// differences far below what float keeps at that magnitude -- measured
// at 0.7% of the output on a small image.  Every bin here sums the same
// pixels in the same order the host would, so with doubles the histogram
// is bit-identical and so is everything after it.  Devices without
// cl_khr_fp64 keep the whole group search on the host.
#ifdef cl_khr_fp64
__kernel void group_hist(__global const uchar* org,
                         __global const short* prd,
                         __global const short* errB,
                         __global const uint*  cpo,
                         __global const uint*  cpq,
                         __global const float* costtab,
                         __global const int*   pmoff,
                         __global const int*   udy,
                         __global const int*   udx,
                         __global const int*   uw,
                         __global const uint*  cbeg,
                         __global double*      hist,
                         const int nc, const int bsw)
{
  const int gr = get_global_id(0);
  const int k = get_global_id(1);
  const int cl = get_global_id(2);
  if( k>=nc )
    return;

  __global double* h = hist+((size_t)(cl*nc+k)*MRP_GROUP+gr)*(MAX_UPARA+2);
  for( int i = 0; i<MAX_UPARA+2; i++ )
    h[i] = 0.0;

  __global const float* cb = costtab+pmoff[k*MRP_GROUP+gr];
  const uint ce = cbeg[cl+1];
  for( uint ci = cbeg[cl]; ci<ce; ci++ ) {
    const uint po = cpo[ci], pq = cpq[ci];
    __global const short* e = errB+po;
    int u = 0;
#pragma unroll
    for( int i = 0; i<NUM_UPELS; i++ )
      u += (int)e[(udy[i]*bsw+udx[i])*nc+k]*uw[i];
#if XUPEL
    for( int j = 0; j<k; j++ )
      u += (int)e[j]*64;
#endif
    u >>= 6;
    u = (u>MAX_UPARA) ? MAX_UPARA : u;

    const int q = cl_qprd((int)prd[pq+k]);
    const int base = q>>PM_ACC;
    __global const float* cq = cb+(q&(NUM_SUBPM-1))*CSTRIDE;
    h[u+1] += cq[base+(int)org[po+k]]+cq[PMSIZE+base];
  }
}
#endif

// The other half of OptimizeGroup: with the groups fixed, which of the
// NUM_PMODEL generalized-Gaussian shapes each one should use.  One
// work-item per (component, shape) walks the image and bins by the group
// the pixel ended up in, so the sixteen shapes are sixteen independent
// passes rather than sixteen table lookups inside one.
#ifdef cl_khr_fp64
__kernel void pmodel_cost(__global const uchar* org,
                          __global const short* prd,
                          __global const char*  grp,
                          __global const uint*  cpo,
                          __global const uint*  cpq,
                          __global const float* costtab,
                          const int npix, const int num_pm,
                          __global double* out)
{
  const int i = get_global_id(0);  // shape
  const int k = get_global_id(1);  // component
  const int ch = get_global_id(2); // chunk of the image
  const int nch = get_global_size(2);

  double acc[MRP_GROUP];
#pragma unroll
  for( int g = 0; g<MRP_GROUP; g++ )
    acc[g] = 0.0;

  // the class-sorted pixel list covers the image exactly once, and
  // carries both the bordered and the plain offset of every pixel
  const int lo = (int)((long)npix*ch/nch), hi = (int)((long)npix*(ch+1)/nch);
  for( int ci = lo; ci<hi; ci++ ) {
    const uint po = cpo[ci], pq = cpq[ci];
    const int gr = (int)grp[pq+k];
    const int q = cl_qprd((int)prd[pq+k]);
    const int base = q>>PM_ACC;
    __global const float* cq = costtab+(size_t)((gr*num_pm+i)*NUM_SUBPM+(q&(NUM_SUBPM-1)))*CSTRIDE;
    acc[gr] += cq[base+(int)org[po+k]]+cq[PMSIZE+base];
  }
  for( int g = 0; g<MRP_GROUP; g++ )
    out[((size_t)(k*MRP_GROUP+g)*num_pm+i)*nch+ch] = acc[g];
}
#endif

// The dynamic program over the activity thresholds.  MRP chooses the
// MRP_GROUP-1 thresholds that split the activity range into groups by
// exact code length, which is a 514-state, 16-stage shortest path -- and
// there is one of them per (class, component), all independent.  They are
// small and serial, so this is parallel in the only way that matters:
// 252 of them at once.
//
// It needs doubles.  The states are cumulative code lengths over the
// whole class, tens of thousands of bits, and the differences that decide
// a threshold are fractions of one; in float the DP picks different
// thresholds.  Devices without cl_khr_fp64 keep the host's DP.
#ifdef cl_khr_fp64
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#define NU (MAX_UPARA+2)

// The DP, in three kernels rather than one work-item per problem.
//
// One work-item per (class, component) is 60 of them on a small image --
// four cores' worth at best, each grinding a 16-stage, 514-state
// shortest path serially, and it was the largest single item on the
// device.  It does not have to be serial: within a stage, every th1
// reads dp[] as the previous stage left it -- the writes go to dp[th1]
// and the reads to dp[t < th1], which the descending loop has not
// reached yet -- so all 514 of them are independent.  Staging it as 16
// launches over (th1, component, class) turns 60 work-items into thirty
// thousand, with no barriers and no local memory: the launch boundary is
// the barrier.
//
// The tie-break has to be kept exactly.  The serial loop starts from
// th0 = th1 and takes a t only on a strict improvement, scanning
// upwards, so the winner is the lowest t that beats it; each work-item
// here reproduces that scan for its own th1.

__kernel void dp_prefix(__global const double* hist,
                        __global double*       cum,
                        __global double*       dp0,
                        __global short*        trebuf,
                        const int nc, const int ncls)
{
  const int gr = get_global_id(0);
  const int k = get_global_id(1);
  const int cl = get_global_id(2);
  if( k>=nc||cl>=ncls )
    return;
  const size_t w = (size_t)(cl*nc+k);
  __global const double* h = hist+(w*MRP_GROUP+gr)*NU;
  __global double* c = cum+(w*MRP_GROUP+gr)*NU;
  double s = 0.0;
  for( int u = 0; u<NU; u++ ) {
    s += h[u];
    c[u] = s;
  }
  if( gr==0 ) // the first stage's dp is the first group's own cost,
    for( int u = 0; u<NU; u++ ) { // and it came from nowhere
      dp0[w*NU+u] = c[u];
      trebuf[w*MRP_GROUP*NU+u] = 0;
    }
}

__kernel void dp_stage(__global const double* cum,
                       __global const double* thcost,
                       __global const double* dpin,
                       __global double*       dpout,
                       __global short*        trebuf,
                       const int gr, const int nc, const int ncls)
{
  const int th1 = get_global_id(0);
  const int k = get_global_id(1);
  const int cl = get_global_id(2);
  if( th1>=NU||k>=nc||cl>=ncls )
    return;
  const size_t w = (size_t)(cl*nc+k);
  __global const double* c = cum+(w*MRP_GROUP+gr)*NU;
  __global const double* dpi = dpin+w*NU;
  __global const short* tp = trebuf+(w*MRP_GROUP+(gr-1))*NU;
  __global short* tc = trebuf+(w*MRP_GROUP+gr)*NU;

  int th0 = th1;
  double mc = dpi[th1]-c[th1]+thcost[0]+thcost[th0-tp[th0]];
  for( int t = 0; t<th1; t++ ) {
    double cc = dpi[t]-c[t]+thcost[th1-t]+thcost[t-tp[t]];
    if( cc<mc ) {
      mc = cc;
      th0 = t;
    }
  }
  dpout[w*NU+th1] = mc+c[th1];
  tc[th1] = (short)th0;
}

// stage 0's back-pointers are all zero, and the walk back is 16 steps
__kernel void dp_back(__global short* trebuf,
                      __global int*   thout,
                      const int nc, const int ncls)
{
  const int k = get_global_id(0);
  const int cl = get_global_id(1);
  if( k>=nc||cl>=ncls )
    return;
  const size_t w = (size_t)(cl*nc+k);
  __global short* tre = trebuf+w*MRP_GROUP*NU;
  for( int u = 0; u<NU; u++ )
    tre[u] = 0;
  int t = NU-1;
  for( int gr = MRP_GROUP-1; gr>0; gr-- ) {
    t = (int)tre[gr*NU+t];
    thout[(cl*MAXC+k)*MRP_GROUP+gr-1] = t;
  }
  thout[(cl*MAXC+k)*MRP_GROUP+MRP_GROUP-1] = MAX_UPARA+1;
}

#endif

)CLC";
