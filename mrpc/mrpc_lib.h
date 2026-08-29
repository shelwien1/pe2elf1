/* -------------------------------------------------------------
   mrpc_lib.h -- lossless image compression on MRP's scheme.

   The codec segments an image over a quadtree, fits a linear predictor
   per (class, colour component), picks a probability model per pixel by
   local activity, and optimises all of that against measured code
   length.  The encoder's optimisation loop runs on an OpenCL device
   where there is one; the decoder is serial and always runs on the host.

   Everything here is in memory: a raster goes in, a blob comes out, and
   the other way round.  No files, no paths, no stdio.  The one thing
   the library will touch on its own is <device name>.!cl in the working
   directory, and only when opts.cache_kernels says so.

   Lifetime is explicit.  mrpc_init hands back a context, mrpc_quit
   gives it back, and nothing the library allocates outlives the call
   that returned it except through mrpc_free.

   Not thread safe: one call on one context at a time.  Two contexts in
   two threads share the coder's scratch and will corrupt each other.
   ------------------------------------------------------------- */
#ifndef MRPC_LIB_H
#define MRPC_LIB_H

#include <stddef.h>

/* --- exports -------------------------------------------------------
   On ELF the attribute is unconditional: it costs nothing in a static
   build, and it is what survives -fvisibility=hidden, which is how a
   shared library ends up with no surface at all.

   Windows cannot do that, because the two sides are different keywords.
   Building the DLL wants MRPC_BUILD_DLL, using one wants MRPC_DLL, and
   with neither the header describes a static library, which is what the
   Makefile here builds by default.

       cl /LD  /DMRPC_BUILD_DLL mrpc_lib.cpp
       gcc -shared -DMRPC_BUILD_DLL ... -Wl,--out-implib,libmrpc.dll.a

   The calling convention is spelled out because a DLL outlives the
   compiler flags it was built with: on 32-bit Windows /Gz or /Gr would
   otherwise change it out from under the header.  -------------------- */
#if defined(_WIN32)||defined(__CYGWIN__)
#  if defined(MRPC_BUILD_DLL)
#    define MRPC_API __declspec(dllexport)
#  elif defined(MRPC_DLL)
#    define MRPC_API __declspec(dllimport)
#  else
#    define MRPC_API
#  endif
#  define MRPC_CALL __cdecl
#elif defined(__GNUC__)&&(__GNUC__>=4)
#  define MRPC_API __attribute__((visibility("default")))
#  define MRPC_CALL
#else
#  define MRPC_API
#  define MRPC_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MRPC_VERSION   3
#define MRPC_MAX_COMP  4  /* RGB and RGBA, in the order the raster has them */

/* An image as the codec sees it: one interleaved raster, `stride` bytes
   per row, `ncomp` bytes per pixel.  One component is a grey image or a
   field of palette indices -- the codec predicts whatever the bytes are
   and codes the residual, which is lossless either way, though indices
   into an arbitrary palette are labels rather than magnitudes and there
   is not much for a predictor to find in them.  Rows may be padded -- the padding
   is compressed too, so a raster with row padding round-trips exactly.
   `data` is stride*height bytes and belongs to whoever allocated it;
   mrpc_decompress allocates one and hands it over. */
typedef struct {
  unsigned       width, height;
  unsigned       ncomp;  /* 1 (grey or palette indices) to 4 (RGBA) */
  unsigned       stride; /* >= width*ncomp */
  unsigned char* data;
} mrpc_image;

/* A block the library allocated.  Release it with mrpc_free. */
typedef struct {
  unsigned char* data;
  size_t         size;
} mrpc_blob;

/* Which device does the encoder's arithmetic.  A zeroed mrpc_opts is
   valid and means: OpenCL on, first GPU or else first CPU device, no
   kernel cache, quiet. */
typedef struct {
  int use_opencl;    /* 0 = everything on the host */
  int platform;      /* index, or -1 for any */
  int device;        /* index as mrpc_device_get counts them, or -1 */
  int device_type;   /* MRPC_DEV_*, when device < 0 */
  int cache_kernels; /* keep the compiled kernels in the working directory */
  int verbose;       /* device build log and per-kernel timings on stderr */
  int progress;      /* 0 = silent, 1 = if the image is big, 2 = always */
  int num_class;     /* predictor classes: 0 = pick from the image, else 2..63.
                        More classes fit the image more closely and cost side
                        information and time; both searches are linear in it. */
  int trial_flip;    /* 1 = encode the image both ways round and keep the
                        smaller.  Worth -0.41% on the reference corpus and
                        never worse on any image; it costs two encodes and
                        nothing at all to decode. */
  int trial_vmap;    /* 1 = also try re-indexing each plane to the values it
                        actually uses.  Worth -6.4% on the reference corpus
                        and up to -32% on quantised imagery, but it warps
                        what a linear predictor sees and costs up to 14% on
                        two tiles, so it is a trial rather than a rule.
                        Another two encodes; four with trial_flip. */
} mrpc_opts;

enum { MRPC_DEV_ANY = 0, MRPC_DEV_CPU, MRPC_DEV_GPU, MRPC_DEV_ACC };

enum {
  MRPC_OK = 0,
  MRPC_ERR_ARG = -1,    /* a null pointer, or geometry the codec cannot take */
  MRPC_ERR_MEM = -2,    /* out of memory */
  MRPC_ERR_DATA = -3,   /* the blob is not one of ours, or is truncated */
  MRPC_ERR_VERSION = -4 /* it is ours, but from a later format */
};

typedef struct mrpc_ctx mrpc_ctx;

/* --- lifetime ------------------------------------------------------
   opts may be null for the defaults.  The context carries the device,
   the compiled kernels and every buffer the codec reuses, so compressing
   a hundred images through one context opens the device once. */
MRPC_API mrpc_ctx*   MRPC_CALL mrpc_init(const mrpc_opts* opts);
MRPC_API void        MRPC_CALL mrpc_quit(mrpc_ctx* c);

/* What the context actually opened -- "[GPU] NVIDIA GeForce RTX 3090
   (NVIDIA CUDA)" -- or "host" if it is not using a device. */
MRPC_API const char* MRPC_CALL mrpc_device_used(mrpc_ctx* c);

/* --- compression ---------------------------------------------------
   head and tail are carried through the stream with an order-1 model
   and handed back by mrpc_decompress: a file format's header and
   trailer ride along with the raster it wraps.  Either may be null.

   img may be null, which compresses head and tail alone -- what a
   frontend does with a file it could not parse as an image.

   *out is allocated here; free it with mrpc_free. */
MRPC_API int MRPC_CALL mrpc_compress(mrpc_ctx* c, const mrpc_image* img,
                                     const void* head, size_t headlen,
                                     const void* tail, size_t taillen,
                                     mrpc_blob* out);

/* img->data, head->data and tail->data are allocated here; free each
   with mrpc_free.  Any of the three outputs may be null if you do not
   want it.  An image the stream does not carry comes back with
   width == 0 and data == 0. */
MRPC_API int MRPC_CALL mrpc_decompress(mrpc_ctx* c, const void* data, size_t size,
                                       mrpc_image* img, mrpc_blob* head,
                                       mrpc_blob* tail);

/* Where the bits went.  Encodes img exactly as mrpc_compress would --
   same search, same model, same stream -- and instead of the stream
   hands back a raster of the same geometry whose every component byte is
   the code length of that component, in 4.4 fixed point: sixteenths of a
   bit, saturating at 15.9375.  Components come back in the order they
   went in, so the plot lines up with the image channel for channel.

   out->data is allocated here; free it with mrpc_free.  Unlike
   mrpc_compress this one wants an image: there is nothing to plot about
   a blob.

   bits, if given, receives the exact total before rounding.  It is worth
   asking for: a symbol that costs less than a thirty-second of a bit --
   and on a flat plane most of them do -- rounds to a zero byte, so the
   raster adds up to slightly less than the image really cost. */
MRPC_API int MRPC_CALL mrpc_plot(mrpc_ctx* c, const mrpc_image* img,
                                 mrpc_image* out, double* bits);

/* Everything the library allocated goes back through here, and only
   here: across a DLL boundary the caller's free() may not be the one
   that allocated it. */
MRPC_API void        MRPC_CALL mrpc_free(void* p);
MRPC_API const char* MRPC_CALL mrpc_error(int rc);

/* --- devices -------------------------------------------------------
   For a frontend that wants to show what is available.  Counting them
   does not need a context: it is what you call before you make one. */
typedef struct {
  char               name[256];
  char               platform[256];
  char               version[128];
  char               driver[128];
  int                type; /* MRPC_DEV_* */
  int                units;
  int                clock_mhz;
  unsigned long long global_mem;
  unsigned long long local_mem;
  size_t             max_work_group;
} mrpc_device;

MRPC_API int MRPC_CALL mrpc_device_count(void);
MRPC_API int MRPC_CALL mrpc_device_get(int index, mrpc_device* out); /* MRPC_OK or ERR_ARG */

#ifdef __cplusplus
}
#endif
#endif
