/*  Shim that lets libvorbis's C compile inside a C++ translation unit.

    C converts void* to any object pointer implicitly; C++ does not, and
    libvorbis assigns the result of its allocators straight into typed
    pointers in a few hundred places.  Rather than edit those, the allocators
    return a value with a templated conversion, which restores exactly the C
    behaviour at the point of use.  alloca has to stay a macro so the storage
    lands in the caller's frame.  */
#ifndef VSHIM_H
#define VSHIM_H
#include <stdlib.h>
#include "vb_voidp.h"
#include "vorbis/codec.h"
#include "codec_internal.h"
#undef _ogg_malloc
#define _ogg_malloc(n)      (vb_voidp{ malloc(n) })
#undef _ogg_calloc
#define _ogg_calloc(n, s)   (vb_voidp{ calloc(n, s) })
#undef _ogg_realloc
#define _ogg_realloc(p, n)  (vb_voidp{ realloc(p, n) })
#undef _ogg_free
#define _ogg_free(p)        free(p)
#undef alloca
#define alloca(n)           (vb_voidp{ __builtin_alloca(n) })
/*  _vorbis_block_alloc lives in block.c and returns void* as well  */
#define VB_ALLOC(vb, n) (vb_voidp{ _vorbis_block_alloc(vb, n) })
#endif
