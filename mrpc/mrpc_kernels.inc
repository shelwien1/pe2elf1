// -------------------------------------------------------------
// mrpc_kernels.inc -- the OpenCL C side of mrpc, as a string.
//
// Kept in the binary rather than in a .cl file next to it: the program is
// built at run time anyway, and a codec that cannot find its kernels is a
// codec that cannot compress.  Everything the kernels need to agree with
// the host about (tap counts, table strides, the padding of the bordered
// raster) is passed as -D on the build line from MrpcCL::Build, so the
// two cannot drift.
//
// The kernels mirror the host functions they replace:
//
//   predict_cube  SetPrdBuf, for every class at once, into a tile that
//                 carries the activity halo with it
//   pixel_cost    the body of CalcCost, per (class, pixel)
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
                           const int ncls)
{
  const int tx = get_global_id(0);
  const int ty = get_global_id(1);
  const int c0 = get_global_id(2)*CLGRP;

  const int y = ty-HT_T; // block-relative
  const int x = tx-HT_L;

  if( !((y>=0)&(y<bh)&(x>=0)&(x<bw)) ) {
    for( int k = 0; k<NC; k++ ) {
      const short h = halo[(k*TH+ty)*TW+tx];
      for( int c = 0; c<CLGRP; c++ )
        if( c0+c<ncls )
          errcube[(((c0+c)*NC+k)*TH+ty)*TW+tx] = h;
    }
    return;
  }

  const int o = (tly+y+PADT)*BSW+(tlx+x+PADL);
  for( int k = 0; k<NC; k++ ) {
    int acc[CLGRP];
#pragma unroll
    for( int c = 0; c<CLGRP; c++ )
      acc[c] = 0;
    __global const int* cf = coef+(c0*MAXC+k)*NTMAX;
    __global const int* to = tapoff+k*NTPAD;
    for( int i = 0; i<NTPAD; i++ ) {
      const int v = (int)planes[o+to[i]];
#pragma unroll
      for( int c = 0; c<CLGRP; c++ )
        acc[c] += v*cf[c*MAXC*NTMAX+i];
    }
    const int org_ = (int)planes[k*BN+o];
    for( int c = 0; c<CLGRP; c++ ) {
      if( c0+c>=ncls )
        break;
      const int p = cl_clip(acc[c]);
      prdcube[(((c0+c)*NC+k)*BSZ+y)*BSZ+x] = (short)p;
      errcube[(((c0+c)*NC+k)*TH+ty)*TW+tx] = (short)cl_econv(org_, p);
    }
  }
}

// One work-item per (class, pixel): the body of CalcCost.  The tile is
// always MAX_BSIZE square, even where the image ends inside it, so that
// every stride here is a constant the device compiler can fold: with the
// block size passed in as an argument it cannot prove that neighbouring
// work-items touch neighbouring memory, and every load becomes a gather.
// The pixels past the edge are computed and dropped -- the prefix sum
// above never reaches them.
//
// The result is left per-pixel rather than summed, because the quadtree above wants the
// cost of 341 different rectangles of it and a prefix sum answers all of
// them.
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
                         const int tly, const int tlx)
{
  const int x = get_global_id(0);
  const int y = get_global_id(1);
  const int cl = get_global_id(2);
  const int ty = y+HT_T, tx = x+HT_L;
  const int o = (tly+y+PADT)*BSW+(tlx+x+PADL);
  float c = 0.0f;

  for( int k = 0; k<NC; k++ ) {
    __global const short* e = errcube+((cl*NC+k)*TH+ty)*TW+tx;
    int u = 0;
#pragma unroll
    for( int i = 0; i<NUM_UPELS; i++ )
      u += (int)e[udy[i]*TW+udx[i]]*uw[i];
#if XUPEL
    // this pixel's earlier components: if red just missed by 30, green is
    // about to as well
    for( int j = 0; j<k; j++ )
      u += (int)errcube[((cl*NC+j)*TH+ty)*TW+tx]*64;
#endif
    u >>= 6;
    u = (u>MAX_UPARA) ? MAX_UPARA : u;

    const int gr = (int)uq[(cl*MAXC+k)*(MAX_UPARA+1)+u];
    const int prdc = (int)prdcube[((cl*NC+k)*BSZ+y)*BSZ+x];
    const int q = cl_qprd(prdc);
    const int base = q>>PM_ACC;
    __global const float* cq = costtab+pmoff[k*MRP_GROUP+gr]+(q&(NUM_SUBPM-1))*CSTRIDE;
    c += cq[base+(int)planes[k*BN+o]]+cq[PMSIZE+base];
  }
  pixcost[(cl*BSZ+y)*BSZ+x] = c;
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
                         __global double*      hist)
{
  const int gr = get_global_id(0);
  const int k = get_global_id(1);
  const int cl = get_global_id(2);
  if( k>=NC )
    return;

  __global double* h = hist+((size_t)(cl*NC+k)*MRP_GROUP+gr)*(MAX_UPARA+2);
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
      u += (int)e[(udy[i]*BSW+udx[i])*NC+k]*uw[i];
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

__kernel void group_dp(__global const double* hist,
                       __global double*       cum,
                       __global const double* thcost,
                       __global double*       dpbuf,
                       __global short*        trebuf,
                       __global int*          thout,
                       const int ncls)
{
  const int cl = get_global_id(0);
  const int k = get_global_id(1);
  if( cl>=ncls||k>=NC )
    return;
  const size_t w = (size_t)(cl*NC+k);

  __global const double* h0 = hist+w*MRP_GROUP*NU;
  __global double* c0 = cum+w*MRP_GROUP*NU;
  for( int gr = 0; gr<MRP_GROUP; gr++ ) {
    __global const double* h = h0+gr*NU;
    __global double* c = c0+gr*NU;
    double s = 0.0;
    for( int u = 0; u<NU; u++ ) {
      s += h[u];
      c[u] = s;
    }
  }

  __global double* dp = dpbuf+w*NU;
  __global short* tre = trebuf+w*MRP_GROUP*NU;
  for( int u = 0; u<NU; u++ ) {
    dp[u] = c0[u];
    tre[u] = 0;
  }
  for( int gr = 1; gr<MRP_GROUP; gr++ ) {
    __global const double* c = c0+gr*NU;
    __global const short* tp = tre+(gr-1)*NU;
    __global short* tc = tre+gr*NU;
    for( int th1 = NU-1; th1>=0; th1-- ) {
      int th0 = th1;
      double mc = dp[th1]-c[th1]+thcost[0]+thcost[th0-tp[th0]];
      for( int t = 0; t<th1; t++ ) {
        double cc = dp[t]-c[t]+thcost[th1-t]+thcost[t-tp[t]];
        if( cc<mc ) {
          mc = cc;
          th0 = t;
        }
      }
      dp[th1] = mc+c[th1];
      tc[th1] = (short)th0;
      if( gr==MRP_GROUP-1 )
        break;
    }
  }

  int t = NU-1;
  for( int gr = MRP_GROUP-1; gr>0; gr-- ) {
    t = (int)tre[gr*NU+t];
    thout[(cl*MAXC+k)*MRP_GROUP+gr-1] = t;
  }
  thout[(cl*MAXC+k)*MRP_GROUP+MRP_GROUP-1] = MAX_UPARA+1;
}
#endif

)CLC";
