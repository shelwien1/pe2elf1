/*  A stand-in for the `void *` fields libvorbis keeps its per-object state in.

    C converts void* to any object pointer implicitly; C++ does not.  libvorbis
    reads `vi->codec_setup`, `v->backend_state`, `vb->internal`, and the floor /
    residue / mapping look and info arrays straight into typed pointers in a few
    hundred places:

        codec_setup_info * ci = vi->codec_setup;

    Giving those *fields* this type instead of `void *` restores exactly the C
    behaviour at the point of use, and not one of the several hundred reads has
    to be touched.  It is the same trick vshim.h already plays on the
    allocators, moved from the value to the field it lands in.

    g++ -fpermissive downgrades the conversion to a warning and so never needed
    this; clang's -fpermissive does not, and rejects every one of those reads.

    vb_voidp has the size, alignment and layout of a void*, and stays trivially
    copyable and trivially default-constructible, so the calloc and
    memset(..., 0, sizeof *x) zeroing libvorbis does over the enclosing structs
    remains valid.  In C the macros expand to the original declarations, which
    keeps psyfit/fit.c -- the one consumer that still compiles these headers as
    C -- building unchanged.  */
#ifndef VB_VOIDP_H
#define VB_VOIDP_H

#ifdef __cplusplus

struct vb_voidp {
  void * p;

  vb_voidp() = default;
  vb_voidp(decltype(nullptr)) : p(0) {}
  template <typename T> vb_voidp(T * q) : p((void *) q) {}

  /*  Templates, so the implicit copy constructor and copy assignment survive
      and the type stays trivial.  */
  template <typename T> vb_voidp & operator = (T * q) { p = (void *) q; return *this; }
  vb_voidp & operator = (decltype(nullptr)) { p = 0; return *this; }

  template <typename T> operator T * () const { return (T *) p; }

  /*  Without these the truth tests and null comparisons libvorbis writes on
      these fields are ambiguous: every T* the conversion could produce is an
      equally good match.  */
  explicit operator bool () const { return p != 0; }
  bool operator ! () const { return p == 0; }

  template <typename T> friend bool operator == (vb_voidp a, T * b) { return a.p == (void *) b; }
  template <typename T> friend bool operator != (vb_voidp a, T * b) { return a.p != (void *) b; }
  template <typename T> friend bool operator == (T * a, vb_voidp b) { return (void *) a == b.p; }
  template <typename T> friend bool operator != (T * a, vb_voidp b) { return (void *) a != b.p; }
  friend bool operator == (vb_voidp a, vb_voidp b) { return a.p == b.p; }
  friend bool operator != (vb_voidp a, vb_voidp b) { return a.p != b.p; }
  friend bool operator == (vb_voidp a, decltype(nullptr)) { return a.p == 0; }
  friend bool operator != (vb_voidp a, decltype(nullptr)) { return a.p != 0; }
  friend bool operator == (decltype(nullptr), vb_voidp b) { return b.p == 0; }
  friend bool operator != (decltype(nullptr), vb_voidp b) { return b.p != 0; }
};

struct vb_cvoidp {
  const void * p;

  vb_cvoidp() = default;
  vb_cvoidp(decltype(nullptr)) : p(0) {}
  template <typename T> vb_cvoidp(T * q) : p((const void *) q) {}

  template <typename T> vb_cvoidp & operator = (T * q) { p = (const void *) q; return *this; }
  vb_cvoidp & operator = (decltype(nullptr)) { p = 0; return *this; }

  template <typename T> operator T * () const { return (T *) p; }

  explicit operator bool () const { return p != 0; }
  bool operator ! () const { return p == 0; }

  template <typename T> friend bool operator == (vb_cvoidp a, T * b) { return a.p == (const void *) b; }
  template <typename T> friend bool operator != (vb_cvoidp a, T * b) { return a.p != (const void *) b; }
  friend bool operator == (vb_cvoidp a, decltype(nullptr)) { return a.p == 0; }
  friend bool operator != (vb_cvoidp a, decltype(nullptr)) { return a.p != 0; }
};

/*  The whole point is that these are void* in all but conversion: libvorbis
    callocs and memsets the structs holding them, and casts them across
    translation-unit boundaries that no longer exist but whose layout
    assumptions remain.  */
#if defined(__cplusplus) && __cplusplus >= 201103L
#include <type_traits>
static_assert(sizeof(vb_voidp) == sizeof(void *), "vb_voidp must be void*-sized");
static_assert(alignof(vb_voidp) == alignof(void *), "vb_voidp must be void*-aligned");
static_assert(std::is_trivially_copyable<vb_voidp>::value, "vb_voidp must survive memcpy");
static_assert(std::is_trivially_destructible<vb_voidp>::value, "vb_voidp must not need destruction");
static_assert(std::is_standard_layout<vb_voidp>::value, "vb_voidp must be standard layout");
static_assert(sizeof(vb_cvoidp) == sizeof(const void *), "vb_cvoidp must be void*-sized");
static_assert(std::is_trivially_copyable<vb_cvoidp>::value, "vb_cvoidp must survive memcpy");
#endif

#define VB_VOIDP   vb_voidp
#define VB_CVOIDP  vb_cvoidp

#else

#define VB_VOIDP   void *
#define VB_CVOIDP  const void *

#endif
#endif
