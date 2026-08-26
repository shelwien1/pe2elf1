// -------------------------------------------------------------
// mrpc -- the command line around the library in mrpc_lib.h.
//
// Everything here is BMP and stdio: the file goes in, the header and the
// trailer are split off the raster, and the three go to mrpc_compress,
// which knows nothing about any of it.  Decoding puts the file back
// together from what mrpc_decompress hands back.
//
// Nothing in this file includes a codec header, and it is plain C++
// against the C API -- which is the point of the split: a frontend that
// can only reach the library the way anything else would.
// -------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mrpc_lib.h"

typedef unsigned char byte;
typedef unsigned int uint;

// -------------------------------------------------------------
// BMP.  What the codec wants is a raster and its geometry; what it does
// not want is to know how a file said so.  An 8bpp file is one component
// per pixel and its palette is just more header -- bfOffBits already
// points past it, so it rides along in the head blob and comes back
// bit-identical without the codec ever seeing it.  Anything that is not
// an 8, 24 or 32bpp uncompressed raster gets compressed as one
// undifferentiated blob, which is what an image-less call does.
// -------------------------------------------------------------
static uint get16(const byte* p) {
  return p[0]|(p[1]<<8);
}

static uint get32(const byte* p) {
  return p[0]|(p[1]<<8)|(p[2]<<16)|(uint(p[3])<<24);
}

static const uint HDR0 = 54; // BITMAPFILEHEADER + BITMAPINFOHEADER

struct BmpInfo {
  uint ok, off, W, H, nc, stride, pixbytes;

  void Init() {
    memset(this, 0, sizeof(*this));
  }

  void Quit() {
  }

  void Parse(const byte* d, size_t flen) {
    Init();
    if( flen<HDR0 )
      return;
    if( d[0]!='B'||d[1]!='M' )
      return;
    uint o = get32(d+10);
    uint isz = get32(d+14);
    int w = int(get32(d+18));
    int h = int(get32(d+22));
    uint pl = get16(d+26);
    uint bpp = get16(d+28);
    uint comp = get32(d+30);
    if( isz<40||pl!=1||comp!=0 )
      return;
    if( bpp!=8&&bpp!=24&&bpp!=32 )
      return; // 8bpp indices, RGB or RGBA
    if( w<=0||h==0 )
      return;
    uint H_ = uint(h<0 ? -h : h), W_ = uint(w);
    if( o<HDR0||o>flen )
      return;
    if( o<14+isz )
      return;
    uint st = ((W_*bpp+31)/32)*4;
    if( st==0||H_>(0xFFFFFFFFu/st) )
      return;
    uint pix = st*H_;
    if( pix>flen-o )
      return;
    ok = 1;
    off = o;
    W = W_;
    H = H_;
    nc = bpp/8;
    stride = st;
    pixbytes = pix;
  }
};

// -------------------------------------------------------------
// Files, whole.  The library is an in-memory API and this is what that
// costs: the input and the output are both resident.
// -------------------------------------------------------------
struct File {
  byte* data;
  size_t size;

  void Init() {
    data = 0;
    size = 0;
  }

  void Quit() {
    free(data);
    data = 0;
    size = 0;
  }

  int Read(const char* name) {
    Init();
    FILE* f = fopen(name, "rb");
    if( !f )
      return 0;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if( n<0 ) {
      fclose(f);
      return 0;
    }
    size = size_t(n);
    data = (byte*)malloc(size ? size : 1);
    if( !data||(size&&fread(data, 1, size, f)!=size) ) {
      fclose(f);
      Quit();
      return 0;
    }
    fclose(f);
    return 1;
  }

  static int Write(const char* name, const void* p1, size_t n1,
                   const void* p2, size_t n2, const void* p3, size_t n3) {
    FILE* f = fopen(name, "wb");
    if( !f )
      return 0;
    int ok = (!n1||fwrite(p1, 1, n1, f)==n1)&&
             (!n2||fwrite(p2, 1, n2, f)==n2)&&
             (!n3||fwrite(p3, 1, n3, f)==n3);
    fclose(f);
    return ok;
  }
};

// -------------------------------------------------------------
static void ListDevices(FILE* f) {
  int n = mrpc_device_count();
  if( n<=0 ) {
    fprintf(f, "no OpenCL devices found\n"
#if defined(_WIN32)||defined(_MSC_VER)
               "  (OpenCL.dll and a runtime have to be installed; a GPU driver\n"
               "   brings both, and Intel's CPU runtime adds a device that does\n"
               "   not need one)\n"
#else
               "  (an ICD loader and a runtime have to be installed; on Debian or\n"
               "   Ubuntu that is ocl-icd-libopencl1 plus a driver --\n"
               "   intel-oneapi-runtime-opencl for the CPU, mesa-opencl-icd or the\n"
               "   vendor package for a GPU)\n"
#endif
    );
    return;
  }
  static const char* kind[4] = {"?", "CPU", "GPU", "ACC"};
  fprintf(f, "OpenCL devices (-d takes the index):\n\n");
  for( int i = 0; i<n; i++ ) {
    mrpc_device d;
    if( mrpc_device_get(i, &d)!=MRPC_OK )
      continue;
    fprintf(f, "  -d %-2d [%s] %s\n", i, kind[d.type&3], d.name);
    fprintf(f, "        %s\n", d.platform);
    fprintf(f, "        %d units @ %d MHz, %.1f GiB global, %llu KiB local, max wg %u\n",
            d.units, d.clock_mhz, double(d.global_mem)/(1024.0*1024.0*1024.0),
            d.local_mem/1024, unsigned(d.max_work_group));
    fprintf(f, "        %s / %s\n", d.version, d.driver);
  }
}

// The options are all about which device does the arithmetic; the two
// positional arguments and the mode letter are what they always were, so
// an existing command line keeps working.
static int ArgNum(int argc, char** argv, int &i, const char* opt) {
  const char* a = argv[i];
  size_t n = strlen(opt);
  if( a[n] )
    return atoi(a+n); // -d0
  if( i+1<argc ) {    // -d 0
    i++;
    return atoi(argv[i]);
  }
  return -1;
}

static void Usage(const char* argv0) {
  printf("mrpc - lossless RGB/RGBA BMP compressor on MRP's scheme\n"
         "\n"
         "Usage: %s [options] <mode> <input> <output>\n"
         "       %s -l\n"
         "\n"
         "  <mode>    'c' compress, 'd' decompress, 'p' plot\n"
         "\n"
         "Options (the encoder's optimisation loop runs on an OpenCL device;\n"
         "the decoder is serial and always runs on the host):\n"
         "\n"
         "  -l        list the OpenCL platforms and devices, and exit\n"
         "  -d <n>    use device <n>, numbered as -l prints it\n"
         "  -p <n>    only look at platform <n> (and number -d within it)\n"
         "  -T <t>    pick by type instead: cpu, gpu, acc\n"
         "  -C        do not use OpenCL at all -- the reference code path\n"
         "  -k        cache the compiled kernels in the working directory\n"
         "  -n <n>    use <n> predictor classes (2..63) instead of the number\n"
         "            the image size suggests\n"
         "  -t        encode the image both ways round and keep the smaller.\n"
         "            Two encodes, nothing at the decoder; worth 0.41%% on the\n"
         "            reference corpus and never worse on any image of it\n"
         "  -V        report what the device compiler had to say\n"
         "\n"
         "'p' encodes the image exactly as 'c' does, throws the stream away, and\n"
         "writes a BMP of the same geometry instead: every component byte is\n"
         "the code length of that component in 4.4 fixed point -- sixteenths\n"
         "of a bit, saturating at 15.9375 -- so the picture is a map of where\n"
         "the file's bits went, channel for channel.  A one-component plot\n"
         "gets a grey ramp for a palette so that it can be looked at.\n"
         "\n"
         "With no -d or -T, the first GPU is used, or the first CPU device\n"
         "if there is no GPU.  Anything that goes wrong with the device is\n"
         "reported and the encode continues on the host.\n"
         "\n"
         "-k caches the compiled kernels in the working directory as\n"
         "<device name>.!cl, and rebuilds them if the device, the driver or\n"
         "mrpc itself changes.  The kernels do not depend on the image, so\n"
         "one file serves every image on that device.\n"
         "\n"
         "More classes fit the image more closely, and cost their\n"
         "coefficients, their share of the class map, and time -- both\n"
         "searches are linear in the count.  The default grows with the log\n"
         "of the pixel count and caps at 63.\n"
         "\n"
         "The codec segments the image into classes over a quadtree, fits one\n"
         "linear predictor per (class, colour component), and codes the\n"
         "residual with a generalized-Gaussian model picked by the local\n"
         "activity.  Class map, coefficients and quantiser are optimized\n"
         "against measured code length and transmitted.  8bpp files -- grey\n"
         "or paletted -- are coded as one component, with the palette carried\n"
         "through as part of the header.  Anything that is not an 8, 24 or\n"
         "32bpp uncompressed raster falls back to order 1.\n",
         argv0, argv0);
}

int main(int argc, char** argv) {
  const char* pos[3] = {0, 0, 0};
  int npos = 0, list = 0;
  mrpc_opts opt;
  memset(&opt, 0, sizeof(opt));
  opt.use_opencl = 1;
  opt.platform = -1;
  opt.device = -1;
  opt.progress = 1; // report when the image is big enough to take a while

  for( int i = 1; i<argc; i++ ) {
    const char* a = argv[i];
    if( a[0]!='-'||a[1]==0 ) {
      if( npos<3 )
        pos[npos] = a;
      npos++;
      continue;
    }
    switch( a[1] ) {
      case 'l':
        list = 1;
        break;
      case 'C':
        opt.use_opencl = 0;
        break;
      case 'V':
        opt.verbose = 1;
        break;
      case 'k':
        opt.cache_kernels = 1;
        break;
      case 'n':
        opt.num_class = ArgNum(argc, argv, i, "-n");
        break;
      case 't':
        opt.trial_flip = 1;
        break;
      case 'd':
        opt.device = ArgNum(argc, argv, i, "-d");
        break;
      case 'p':
        opt.platform = ArgNum(argc, argv, i, "-p");
        break;
      case 'T': {
        const char* t = a[2] ? a+2 : ((i+1<argc) ? argv[++i] : "");
        opt.device_type = (t[0]=='c') ? MRPC_DEV_CPU : (t[0]=='g') ? MRPC_DEV_GPU :
                          (t[0]=='a') ? MRPC_DEV_ACC : MRPC_DEV_ANY;
        break;
      }
      default:
        fprintf(stderr, "mrpc: unknown option %s\n", a);
        return 1;
    }
  }

  if( list ) {
    ListDevices(stdout);
    if( npos<3 )
      return 0;
  }
  if( npos<3 ) {
    Usage(argv[0]);
    return 1;
  }

  const char mode = pos[0][0];
  if( (mode!='c'&&mode!='d'&&mode!='p')||pos[0][1] ) {
    fprintf(stderr, "mrpc: mode is 'c', 'd' or 'p', not \"%s\"\n", pos[0]);
    return 1;
  }
  const int dec = (mode=='d');

  File in;
  in.Init();
  if( !in.Read(pos[1]) ) {
    fprintf(stderr, "mrpc: cannot read %s\n", pos[1]);
    return 2;
  }

  mrpc_ctx* ctx = mrpc_init(&opt);
  if( !ctx ) {
    // either out of memory, or a device was named that is not there --
    // the library has already said which on stderr
    in.Quit();
    fprintf(stderr, "mrpc: cannot start\n");
    return 8;
  }
  if( opt.verbose )
    fprintf(stderr, "mrpc: device: %s\n", mrpc_device_used(ctx));

  int rc = MRPC_OK, ret = 0;
  if( mode=='p' ) {
    // --- plot: the same encode, and a picture of what it cost
    BmpInfo b;
    b.Parse(in.data, in.size);
    if( !b.ok ) {
      fprintf(stderr, "mrpc: %s is not a raster this can plot\n", pos[1]);
      mrpc_quit(ctx);
      in.Quit();
      return 3;
    }
    mrpc_image im, pl;
    double exact = 0.0;
    memset(&im, 0, sizeof(im));
    im.width = b.W;
    im.height = b.H;
    im.ncomp = b.nc;
    im.stride = b.stride;
    im.data = in.data+b.off;
    rc = mrpc_plot(ctx, &im, &pl, &exact);
    if( rc==MRPC_OK ) {
      // the input's own header, so the plot is the same shape of file as
      // the image it describes -- except that a palette maps indices to
      // colours and a code length is not an index, so a one-component
      // plot gets a grey ramp instead and can be looked at
      byte* hdr = (byte*)malloc(b.off ? b.off : 1);
      if( hdr ) {
        memcpy(hdr, in.data, b.off);
        if( b.nc==1&&b.off>=HDR0+1024 )
          for( uint i = 0; i<256; i++ ) {
            byte* e = hdr+b.off-1024+i*4;
            e[0] = e[1] = e[2] = byte(i);
            e[3] = 0;
          }
        if( !File::Write(pos[2], hdr, b.off, pl.data, size_t(pl.stride)*pl.height, 0, 0) ) {
          fprintf(stderr, "mrpc: cannot write %s\n", pos[2]);
          ret = 4;
        }
        free(hdr);
      } else {
        rc = MRPC_ERR_MEM;
      }
      // what the raster adds up to, against what the encode actually
      // cost -- they differ by the symbols too cheap to round to a
      // sixteenth of a bit, and on a flat plane that is most of them
      double bits = 0;
      size_t sat = 0;
      for( uint y = 0; y<pl.height; y++ ) {
        const byte* r = pl.data+size_t(y)*pl.stride;
        for( uint i = 0; i<pl.width*pl.ncomp; i++ ) {
          bits += double(r[i]);
          if( r[i]==255 )
            sat++;
        }
      }
      bits /= 16.0;
      const double n = double(pl.width)*pl.height*pl.ncomp;
      fprintf(stderr, "mrpc: %.0f symbols, %.0f bits = %.0f B raster,"
                      " %.0f B exact, mean %.3f b/symbol",
              n, bits, bits/8.0, exact/8.0, exact/n);
      if( sat )
        fprintf(stderr, ", %llu saturated", (unsigned long long)sat);
      fprintf(stderr, "\n");
      mrpc_free(pl.data);
    }
    b.Quit();
  } else if( !dec ) {
    // --- compress: split the file into header, raster and trailer
    BmpInfo b;
    b.Parse(in.data, in.size);
    mrpc_image im;
    memset(&im, 0, sizeof(im));
    if( b.ok ) {
      im.width = b.W;
      im.height = b.H;
      im.ncomp = b.nc;
      im.stride = b.stride;
      im.data = in.data+b.off;
    }
    const size_t tailoff = b.ok ? b.off+b.pixbytes : in.size;
    mrpc_blob out;
    rc = mrpc_compress(ctx, b.ok ? &im : 0,
                       in.data, b.ok ? b.off : in.size,
                       in.data+tailoff, in.size-tailoff, &out);
    if( rc==MRPC_OK ) {
      if( !File::Write(pos[2], out.data, out.size, 0, 0, 0, 0) ) {
        fprintf(stderr, "mrpc: cannot write %s\n", pos[2]);
        ret = 4;
      }
      mrpc_free(out.data);
    }
    b.Quit();
  } else {
    // --- decompress: and put the file back together in that order
    mrpc_image im;
    mrpc_blob head, tail;
    rc = mrpc_decompress(ctx, in.data, in.size, &im, &head, &tail);
    if( rc==MRPC_OK ) {
      if( !File::Write(pos[2], head.data, head.size,
                       im.data, im.data ? size_t(im.stride)*im.height : 0,
                       tail.data, tail.size) ) {
        fprintf(stderr, "mrpc: cannot write %s\n", pos[2]);
        ret = 4;
      }
      mrpc_free(head.data);
      mrpc_free(tail.data);
      mrpc_free(im.data);
    }
  }

  if( rc!=MRPC_OK ) {
    fprintf(stderr, "mrpc: %s\n", mrpc_error(rc));
    ret = 5;
  }

  mrpc_quit(ctx);
  in.Quit();
  return ret;
}
