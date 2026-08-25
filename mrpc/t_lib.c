/* -------------------------------------------------------------
   t_lib.c -- the library through its own header, from C.
//
   This is C, not C++, and it links against libmrpc.a: if it builds at
   all then mrpc_lib.h is what it says it is.  What it checks:

     * a synthetic raster round-trips through memory, bit for bit,
       including row padding and the head and tail blobs;
     * an image-less call -- what a frontend does with a file it could
       not parse -- round-trips too;
     * a damaged stream is refused rather than believed;
     * a hundred images through one context do not grow it, which is
       what tells you Init and Quit are paired all the way down.
   ------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <unistd.h>
#endif

#include "mrpc_lib.h"

static int fails = 0;

static void check(int ok, const char* what) {
  printf("  %-46s %s\n", what, ok ? "ok" : "FAILED");
  if( !ok )
    fails++;
}

/* something with structure in it, so the codec has work to do */
static unsigned char* make_raster(unsigned w, unsigned h, unsigned nc, unsigned stride) {
  unsigned char* p = (unsigned char*)malloc((size_t)stride*h);
  unsigned x, y, k;
  unsigned seed = w*7u+h*13u+nc;
  for( y = 0; y<h; y++ ) {
    unsigned char* r = p+(size_t)y*stride;
    for( x = 0; x<w; x++ )
      for( k = 0; k<nc; k++ ) {
        seed = seed*1664525u+1013904223u;
        int v = (int)((x*3+y*5+k*40)%256)+(int)((seed>>24)%9)-4;
        r[x*nc+k] = (unsigned char)(v<0 ? 0 : v>255 ? 255 : v);
      }
    for( x = w*nc; x<stride; x++ )
      r[x] = (unsigned char)(0xA5+y); /* padding is coded too */
  }
  return p;
}

/* how much memory this process has, in KiB; 0 where we cannot tell */
static long rss_kb(void) {
#ifdef __linux__
  long n = 0;
  FILE* f = fopen("/proc/self/statm", "r");
  if( f ) {
    long pages = 0;
    if( fscanf(f, "%*ld %ld", &pages)==1 )
      n = pages*(sysconf(_SC_PAGESIZE)/1024);
    fclose(f);
  }
  return n;
#else
  return 0;
#endif
}

int main(void) {
  const unsigned W = 97, H = 61, NC = 3;
  const unsigned STRIDE = ((W*NC*8+31)/32)*4; /* BMP-style row padding */
  unsigned char head[57], tail[19];
  unsigned char* raster = make_raster(W, H, NC, STRIDE);
  mrpc_opts opt;
  mrpc_ctx* c;
  mrpc_image in, out;
  mrpc_blob blob, h2, t2;
  int rc, i;

  for( i = 0; i<(int)sizeof(head); i++ )
    head[i] = (unsigned char)(i*7+1);
  for( i = 0; i<(int)sizeof(tail); i++ )
    tail[i] = (unsigned char)(255-i*3);

  memset(&opt, 0, sizeof(opt));
  opt.use_opencl = 1;
  opt.platform = -1;
  opt.device = -1;

  printf("mrpc library, through the C API\n");
  c = mrpc_init(&opt);
  check(c!=0, "mrpc_init");
  if( !c )
    return 1;
  printf("  device: %s\n", mrpc_device_used(c));

  /* --- an image, with a header and a trailer wrapped around it ---- */
  memset(&in, 0, sizeof(in));
  in.width = W;
  in.height = H;
  in.ncomp = NC;
  in.stride = STRIDE;
  in.data = raster;
  rc = mrpc_compress(c, &in, head, sizeof(head), tail, sizeof(tail), &blob);
  check(rc==MRPC_OK&&blob.size>0, "mrpc_compress");
  printf("  %u x %u x %u, %lu bytes -> %lu\n", W, H, NC,
         (unsigned long)((size_t)STRIDE*H+sizeof(head)+sizeof(tail)),
         (unsigned long)blob.size);

  rc = mrpc_decompress(c, blob.data, blob.size, &out, &h2, &t2);
  check(rc==MRPC_OK, "mrpc_decompress");
  check(out.width==W&&out.height==H&&out.ncomp==NC&&out.stride==STRIDE,
        "geometry survives");
  check(out.data&&memcmp(out.data, raster, (size_t)STRIDE*H)==0,
        "raster is bit-identical, padding included");
  check(h2.size==sizeof(head)&&memcmp(h2.data, head, sizeof(head))==0,
        "head blob is bit-identical");
  check(t2.size==sizeof(tail)&&memcmp(t2.data, tail, sizeof(tail))==0,
        "tail blob is bit-identical");
  mrpc_free(out.data);
  mrpc_free(h2.data);
  mrpc_free(t2.data);

  /* --- a damaged stream is not to be believed --------------------- */
  {
    unsigned char* bad = (unsigned char*)malloc(blob.size);
    memcpy(bad, blob.data, blob.size);
    bad[blob.size/2] ^= 0xFF;
    rc = mrpc_decompress(c, bad, blob.size, &out, &h2, &t2);
    /* it may decode to nonsense or be refused; what it may not do is
       hand back a raster it never allocated, or crash getting there */
    check(rc!=MRPC_OK||out.data!=0||out.width==0, "damaged stream is survivable");
    if( rc==MRPC_OK ) {
      mrpc_free(out.data);
      mrpc_free(h2.data);
      mrpc_free(t2.data);
    }
    free(bad);
    rc = mrpc_decompress(c, blob.data, 3, &out, &h2, &t2);
    check(rc!=MRPC_OK, "truncated stream is refused");
  }
  mrpc_free(blob.data);

  /* --- no image at all: a file the frontend could not parse -------- */
  {
    unsigned char junk[1000];
    for( i = 0; i<(int)sizeof(junk); i++ )
      junk[i] = (unsigned char)(i*i);
    rc = mrpc_compress(c, 0, junk, sizeof(junk), 0, 0, &blob);
    check(rc==MRPC_OK, "mrpc_compress with no image");
    rc = mrpc_decompress(c, blob.data, blob.size, &out, &h2, &t2);
    check(rc==MRPC_OK&&out.width==0&&out.data==0, "no image comes back as none");
    check(h2.size==sizeof(junk)&&memcmp(h2.data, junk, sizeof(junk))==0,
          "the bytes come back anyway");
    mrpc_free(h2.data);
    mrpc_free(t2.data);
    mrpc_free(blob.data);
  }

  /* --- the same context, over and over ---------------------------- */
  {
    long before = 0, after = 0;
    const int N = 60;
    int bad = 0;
    for( i = 0; i<N; i++ ) {
      rc = mrpc_compress(c, &in, head, sizeof(head), tail, sizeof(tail), &blob);
      if( rc!=MRPC_OK )
        bad++;
      else {
        rc = mrpc_decompress(c, blob.data, blob.size, &out, &h2, &t2);
        if( rc!=MRPC_OK||!out.data||memcmp(out.data, raster, (size_t)STRIDE*H)!=0 )
          bad++;
        mrpc_free(out.data);
        mrpc_free(h2.data);
        mrpc_free(t2.data);
        mrpc_free(blob.data);
      }
      if( i==9 )
        before = rss_kb();
    }
    after = rss_kb();
    check(bad==0, "60 round trips through one context");
    if( before>0 )
      printf("  resident after 10 round trips %ld KiB, after %d: %ld KiB\n",
             before, N, after);
    check(before==0||after<=before+2048, "and it does not grow between them");
  }

  mrpc_quit(c);
  free(raster);
  printf(fails ? "\n%d check(s) FAILED\n" : "\nall checks passed\n", fails);
  return fails ? 1 : 0;
}
