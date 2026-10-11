// vbn2.cpp: VBN 1.001's compressor, standalone, on one file.
//
//   g++ -m32 -O2 -o vbn2 vbn2.cpp
//   ./vbn2 c input output      compress
//   ./vbn2 d input output      decompress
//
// A compressed stream is the input's size (4 bytes, little-endian) followed by
// the code VBN writes for it in its MAX mode (`vbn u`): VBN's archive around it
// -- header, file table, volumes -- is gone.  The code is VBN's, bit for bit:
// for the data VBN puts in an archive (its file table, then the files), vbn2 c
// writes exactly the code that follows the archive's 3-byte header.
//
// The functions are VBN's own, decompiled from vbn.exe: incdec/ moved the ones
// plain `vbn u` and `vbn r` run out of the program one at a time and verified
// each (incdec/README.md), and tools/mkvbn2.py there made them a standalone
// program.  This file started as that program and has been cut down to the
// coder since.  vbn2 is i386 code: the decompiled bodies keep pointers in
// 32-bit integers and reach VBN's data at its own addresses, which vbn2 maps
// where vbn.exe had it (0x10000000).

#define _FILE_OFFSET_BITS 64        // stat() on i386 fails on 64-bit inode numbers otherwise
#include <cerrno>
#include <csetjmp>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sys/mman.h>
#include <sys/stat.h>
#include <ucontext.h>
#include <unistd.h>

#if !defined(__i386__)
#error "vbn2.cpp is i386 code: build it with g++ -m32"
#endif

// ===========================================================================
// Hex-Rays' helpers, with the meaning its defs.h gives them.
// ===========================================================================
// Parts of a variable, in place (little-endian).  The pointers may alias the
// variable: taking a 64-bit local's halves through a plain uint32_t * breaks
// C++'s aliasing rules, and g++ -O2 may then reorder the accesses.
typedef uint16_t __attribute__((may_alias)) alias_uint16;
typedef uint32_t __attribute__((may_alias)) alias_uint32;
#define BYTEn(x, n)   (*((uint8_t *)&(x) + (n)))
#define WORDn(x, n)   (*((alias_uint16 *)&(x) + (n)))
#define DWORDn(x, n)  (*((alias_uint32 *)&(x) + (n)))
#define LOBYTE(x)     BYTEn(x, 0)
#define BYTE1(x)      BYTEn(x, 1)
#define HIBYTE(x)     BYTEn(x, sizeof(x) - 1)
#define LOWORD(x)     WORDn(x, 0)
#define LODWORD(x)    DWORDn(x, 0)
#define HIDWORD(x)    DWORDn(x, sizeof(x) / 4 - 1)

// A forward, byte-at-a-time copy (rep movsb), overlapping or not.
static inline void *qmemcpy(void *dst, const void *src, uint32_t n)
{
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;
  while (n--)
    *d++ = *s++;
  return dst;
}

// count * size does not overflow T (unsigned)
template <class T> static inline bool is_mul_ok(T count, T size)
{
  return count == 0 || size == 0 || count <= T(-1) / size;
}

// The flags of x + y, computed at the wider operand's size, as x86 sets them.
template <class T> static inline int8_t sign_of(T x)
{
  if (sizeof(T) == 1) return (int8_t)x < 0;
  if (sizeof(T) == 2) return (int16_t)x < 0;
  if (sizeof(T) == 4) return (int32_t)x < 0;
  return (int64_t)x < 0;
}
template <class T, class U> static inline int8_t __CFADD__(T x, U y)
{
  uint32_t n = sizeof(T) > sizeof(U) ? sizeof(T) : sizeof(U);
  if (n == 1) return (uint8_t)x > (uint8_t)(x + y);
  if (n == 2) return (uint16_t)x > (uint16_t)(x + y);
  if (n == 4) return (uint32_t)x > (uint32_t)(x + y);
  return (uint64_t)x > (uint64_t)(x + y);
}
template <class T, class U> static inline int8_t __OFADD__(T x, U y)
{
  // overflow: both operands have one sign and the sum the other
  if (sizeof(T) < sizeof(U)) {
    U x2 = x;
    int8_t sx = sign_of(x2);
    return ((1 ^ sx) ^ sign_of(y)) & (sx ^ sign_of(U(x2 + y)));
  }
  T y2 = y;
  int8_t sx = sign_of(x);
  return ((1 ^ sx) ^ sign_of(y2)) & (sx ^ sign_of(T(x + y2)));
}

// Intrinsics, with the x86 semantics the decompiler assumed.  bsr leaves its
// destination unchanged for a zero source, as the CPU does.
static inline uint8_t _BitScanReverse(uint32_t *idx, uint32_t v)
{ if (!v) return 0; *idx = 31 - __builtin_clz(v); return 1; }
static inline uint8_t _BitScanReverse(int32_t *idx, uint32_t v)
{ return _BitScanReverse((uint32_t *)idx, v); }
// bt/btc with a memory operand address a bit string: the offset is signed and
// may run past the first dword.
static inline uint8_t _bittest(const void *base, int32_t bit)
{ return (((const uint8_t *)base)[bit >> 3] >> (bit & 7)) & 1; }
static inline uint8_t _bittestandcomplement(void *base, int32_t bit)
{ uint8_t *p = (uint8_t *)base + (bit >> 3), m = 1 << (bit & 7), o = *p & m; *p ^= m; return o != 0; }
template <class T, class V> static inline T _InterlockedExchange(T *p, V v)
{ T o = *p; *p = (T)v; return o; }

// ===========================================================================
// vbn's memory.  vbn.exe is one flat 64 MB image at 0x10000000: code, data,
// BSS, and (in the original) its heap and stack.  The decompiled bodies reach
// every global at its address, store addresses in 32-bit integers, and tell a
// tree node (an address) from a leaf (a position, negated) by its sign, so the
// image is mapped where it was.  Its initialized data is embedded below; the
// rest starts out zero, as vbn's startup code left it.
// ===========================================================================
static const uint32_t IMAGE_BASE = 0x10000000, IMAGE_SIZE = 0x4000000;
static const uint32_t DATA_START = 0x1000A08C;         // .vbn's initialized data, to 0x1000D400
extern const uint8_t image_data[];                // filled in at the end of the file
extern const uint32_t image_data_size;

#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000
#endif
static void image_map()
{
  void *p = mmap((void *)IMAGE_BASE, IMAGE_SIZE, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
  if (p != (void *)IMAGE_BASE) {
    fprintf(stderr, "vbn2: cannot map vbn's memory at %#x: %s\n", IMAGE_BASE, strerror(errno));
    exit(127);
  }
  memcpy((void *)DATA_START, image_data, image_data_size);
}

// ===========================================================================
// vbn's globals: each at its address in vbn's image, under the name IDA gave
// it (Hex-Rays names many after a string it found there, which says nothing
// about the object).  An array is declared to the end of the image: that is
// how far the decompiled code may index it.
// ===========================================================================
static int8_t (&aVbnVer1001)[67067632] = *(int8_t (*)[67067632])0x1000A110;
static int32_t (&src_3)[16764860] = *(int32_t (*)[16764860])0x1000C110;
static int32_t (&Data_size____10u_n)[16764321] = *(int32_t (*)[16764321])0x1000C97C;
static int32_t &dword_1000CA4C = *(int32_t *)0x1000CA4C;
static int32_t (&dword_1000CA54)[16764267] = *(int32_t (*)[16764267])0x1000CA54;
static int32_t &n55 = *(int32_t *)0x1000D4B0;
static int32_t &dword_1000D4C4 = *(int32_t *)0x1000D4C4;
static int32_t (&dword_1000D558)[16763562] = *(int32_t (*)[16763562])0x1000D558;
static int32_t (&dword_1000D55C)[16763561] = *(int32_t (*)[16763561])0x1000D55C;
static int32_t (&dword_1000D560)[16763560] = *(int32_t (*)[16763560])0x1000D560;
static int32_t (&dword_1000D850)[16763372] = *(int32_t (*)[16763372])0x1000D850;
static int32_t (&buf__0)[16762088] = *(int32_t (*)[16762088])0x1000EC60;
static int32_t (&dword_1000F058)[16761834] = *(int32_t (*)[16761834])0x1000F058;
static int32_t (&dword_1000F060)[16761832] = *(int32_t (*)[16761832])0x1000F060;
static int32_t (&i_5)[16761320] = *(int32_t (*)[16761320])0x1000F860;
static int32_t (&dword_1000F864)[16761319] = *(int32_t (*)[16761319])0x1000F864;
static int32_t (&dword_10010060)[16760808] = *(int32_t (*)[16760808])0x10010060;
static uint32_t &i_ = *(uint32_t *)0x10010460;
static int32_t (&dword_10010464)[16760551] = *(int32_t (*)[16760551])0x10010464;
static int32_t (&i_4)[16760040] = *(int32_t (*)[16760040])0x10010C60;
static int32_t (&buf_6)[16759677] = *(int32_t (*)[16759677])0x1001120C;
static int32_t (&buf_7)[16758653] = *(int32_t (*)[16758653])0x1001220C;
static int8_t (&buf_3)[67018228] = *(int8_t (*)[67018228])0x1001620C;
static int8_t (&buf_4)[67017972] = *(int8_t (*)[67017972])0x1001630C;
static int32_t &n0x7FFFFFFF = *(int32_t *)0x1001640C;
static int32_t &dword_10016410 = *(int32_t *)0x10016410;
static int32_t &n0x7FFFFFFF_1 = *(int32_t *)0x10016414;
static int32_t &n362 = *(int32_t *)0x10016418;
static int8_t *&buf = *(int8_t **)0x1001641C;
static int32_t &n362_0 = *(int32_t *)0x10016420;
static int8_t *&buf_1 = *(int8_t **)0x10016424;
static int32_t &n362_1 = *(int32_t *)0x10016428;
static int8_t *&buf_0 = *(int8_t **)0x1001642C;
static int32_t &dword_10016430 = *(int32_t *)0x10016430;
static int32_t &dword_10016434 = *(int32_t *)0x10016434;
static int32_t &dword_10016438 = *(int32_t *)0x10016438;
static int32_t &dst_4 = *(int32_t *)0x1001643C;
static int8_t *&src_4 = *(int8_t **)0x10016440;
static int32_t &n13369344 = *(int32_t *)0x10016444;
static int32_t &n4_0 = *(int32_t *)0x10016448;
static int32_t &n8_2 = *(int32_t *)0x1001644C;
static int32_t &n2_1 = *(int32_t *)0x10016450;
static int32_t &dword_10016454 = *(int32_t *)0x10016454;
static int32_t &dword_1001645C = *(int32_t *)0x1001645C;
static int32_t &n0x7FFFFFFF_2 = *(int32_t *)0x10016460;
static int32_t &n8_1 = *(int32_t *)0x10016468;
static uint32_t &n8 = *(uint32_t *)0x1001646C;
static int32_t &n8_3 = *(int32_t *)0x10016470;
static int32_t &n8_0 = *(int32_t *)0x1001647C;
static int32_t &dst_0 = *(int32_t *)0x10016480;
static int32_t &dst_1 = *(int32_t *)0x10016484;
static int32_t &dword_10016488 = *(int32_t *)0x10016488;
static int32_t &n4 = *(int32_t *)0x1001648C;
static int32_t &n2_0 = *(int32_t *)0x10016490;
static int32_t &dword_10016494 = *(int32_t *)0x10016494;
static int32_t &n0x4000 = *(int32_t *)0x10016498;
static int32_t &n0x2000 = *(int32_t *)0x1001649C;
static int32_t &n0xFFE = *(int32_t *)0x100164A0;
static int32_t &n0x384 = *(int32_t *)0x100164A4;
static int32_t &dword_100164A8 = *(int32_t *)0x100164A8;
static int32_t &dword_100164AC = *(int32_t *)0x100164AC;
static int32_t &dword_100164B0 = *(int32_t *)0x100164B0;
static int32_t &dword_100164B4 = *(int32_t *)0x100164B4;
static int32_t &dword_100164B8 = *(int32_t *)0x100164B8;
static uint32_t &n2048 = *(uint32_t *)0x100164BC;
static uint32_t &n256 = *(uint32_t *)0x100164C0;
static int32_t &dword_100164C4 = *(int32_t *)0x100164C4;
static int32_t &dword_100164C8 = *(int32_t *)0x100164C8;
static int8_t (&byte_100166EF)[67016977] = *(int8_t (*)[67016977])0x100166EF;
static int8_t (&dst_3)[67016976] = *(int8_t (*)[67016976])0x100166F0;
static int32_t (&dword_100167F0)[16754180] = *(int32_t (*)[16754180])0x100167F0;
static int32_t (&src_5)[16649796] = *(int32_t (*)[16649796])0x1007C6F0;
static int8_t (&dst_5)[65763600] = *(int8_t (*)[65763600])0x101486F0;
static int32_t (&dst)[16336452] = *(int32_t (*)[16336452])0x101AE6F0;
static int32_t &dword_101AE6F4 = *(int32_t *)0x101AE6F4;
static int32_t (&dword_101AE6F8)[16336450] = *(int32_t (*)[16336450])0x101AE6F8;
static int32_t &buf_ = *(int32_t *)0x10E6E6F0;
static int32_t (&src_6)[12981060] = *(int32_t (*)[12981060])0x10E7B2F0;
static int32_t (&buf_5)[12954948] = *(int32_t (*)[12954948])0x10E94AF0;
static int8_t (&buf_2)[51669264] = *(int8_t (*)[51669264])0x10EB96F0;

// ===========================================================================
// vbn's C runtime: the one function the coder still calls.
// ===========================================================================
#define __PE_DECL___sub_100091DC
static int8_t *__sub_100091DC(void *buf, uint8_t c, uint32_t n)   // memset
{
  memset(buf, c, n);
  return (int8_t *)buf;
}

// ===========================================================================
// The compressor's early exit.  sub_1000703B records where the next call
// returns to, and sub_10006E22 returns from that call at once, from however
// deep in the search: setjmp and longjmp.  vbn_restartable(fn, arg) is the
// pair of calls sub_100056C4 makes; the tree walkers end the search through
// vbn_walk_exit when their output is full (n2_1 = n55, then the exit).
// ===========================================================================
static jmp_buf early_exit;
template <class F> static __attribute__((noinline)) void vbn_restartable(F fn, int32_t arg)
{
  if (!setjmp(early_exit))
    fn((int32_t *)(uintptr_t)arg);
}
#define __PE_DECL___pe_sub_10006E22
[[noreturn]] static void __pe_sub_10006E22(void) { longjmp(early_exit, 1); }
[[noreturn]] static void vbn_walk_exit(void)
{
  n2_1 = n55;
  longjmp(early_exit, 1);
}
struct VbnWalk { uint8_t *out; int32_t left; int32_t aux; int32_t value; };   // ECX EDI EDX ESI


// ===========================================================================
// vbn's functions: the ones plain `u` and `r` run, callees first.
// ===========================================================================

// sub_100048F0 @ 0x100048F0 -- hand-written (incdec.md §4.1): copy a1 dwords src -> dst.
// `rep movsd`: forward, a dword at a time.  Hex-Rays printed qmemcpy, a byte
// copy, which differs from this when the ranges overlap by less than 4 bytes.
static __attribute__((stdcall)) void __sub_100048F0(int32_t a1, void *src, void *dst)
{
  uint32_t *d = (uint32_t *)dst;
  const uint32_t *s = (const uint32_t *)src;
  for (uint32_t n = (uint32_t)a1; n; n--)
    *d++ = *s++;
}

// sub_1000490B @ 0x1000490B -- generated by tools/extract.py from vbn.exe.c lines 5088..5092
// Hex-Rays: int __stdcall sub_1000490B(int a1, void *buf)
// real convention: stdcall, ret 8

static __attribute__((stdcall)) int32_t __sub_1000490B(int32_t a1, void *buf)
{
  memset(buf, 0, 4 * a1);
  return 0;
}

// sub_10004993 @ 0x10004993 -- generated by tools/extract.py from vbn.exe.c lines 5130..5176
// Hex-Rays: unsigned int __stdcall sub_10004993(unsigned int a1)
// real convention: stdcall, ret 4

static __attribute__((stdcall)) uint32_t __sub_10004993(uint32_t a1)
{
  uint32_t v5; // [esp+0h] [ebp-Ch]
  uint32_t v6; // [esp+4h] [ebp-8h]
  int32_t v7; // [esp+8h] [ebp-4h]
  int32_t v8; // [esp+8h] [ebp-4h]

  v7 = n0x7FFFFFFF_1;
  v6 = 1;
  v5 = 1;
  n0x7FFFFFFF = (int32_t)((uint32_t)n0x7FFFFFFF << 1);   // shl: wraps
  dword_10016410 = (int32_t)((uint32_t)dword_10016410 << 1);   // shl: wraps
  while ( v7-- )
  {
    ++v5;
    if ( n0x7FFFFFFF >= 0 )
      break;
    n0x7FFFFFFF = (int32_t)((uint32_t)n0x7FFFFFFF << 1);   // shl: wraps
  }
  v8 = n0x7FFFFFFF_1;
  while ( v8-- )
  {
    ++v6;
    if ( dword_10016410 < 0 )
      break;
    dword_10016410 = (int32_t)((uint32_t)dword_10016410 << 1);   // shl: wraps
  }
  n0x7FFFFFFF = 0x7FFFFFFF;
  if ( v5 > v6 )
  {
    v5 = v6;
    n0x7FFFFFFF = 0x80000000;
  }
  while ( v5-- )
  {
    if ( n0x7FFFFFFF < 0 )
      _bittestandcomplement(
        (int32_t *)aVbnVer1001,          // "                                                \nvbn ver.1...."
        a1);
    ++a1;
    n0x7FFFFFFF = (int32_t)((uint32_t)n0x7FFFFFFF << 1);   // shl: wraps
    --n0x7FFFFFFF_1;
  }
  n0x7FFFFFFF = 0;
  dword_10016410 = -1;
  return a1;
}

// sub_100050B0 @ 0x100050B0 -- generated by tools/extract.py from vbn.exe.c lines 5478..5495
// Hex-Rays: int __stdcall sub_100050B0(int j)
// real convention: stdcall, ret 4

static __attribute__((stdcall)) int32_t __sub_100050B0(int32_t j)
{
  int32_t n362; // edx
  int32_t *v2; // edx
  int32_t result; // eax

  LOWORD(i_5[2 * j]) += 5;
  n362 = n362_0;
  n362_0 = ((uint16_t)n362_0 + 1) & 0xFFF;
  v2 = (int32_t *)&buf_1[4 * n362];
  result = *v2;
  *v2 = j;
  if ( result >= 0 )
    LOWORD(i_5[2 * result]) -= 5;
  else
    n256 += 5;
  return result;
}

// sub_10005251 @ 0x10005251 -- hand-written (incdec.md §4.1): update the context tree.
// Assembly that keeps the bit position in EBP after `mov ebp,esi`; Hex-Rays
// still read EBP as the frame pointer (&savedregs).  What it does (vbn.asm):
//
// For iters = min(bsr(a1 + a1/2 + 1) + 1, a1) positions pos = n8, n8+8, ...:
//  - find a slot: the first time, the one the previous search left in the
//    globals n8_0/dst_0/dst_1 (bit position, slot address, slot value); after
//    that, descend from the root `dst` reading the data bits backwards from
//    byte pos/8-1 (bit 1 -> child [0], bit 0 -> child [1]) to the first slot
//    holding a value <= 0, or to node[0] of the last node when the bits run out;
//  - depth = pos - (bit position where the descent stopped); then
//      depth <= 64*bsr(nodes+32):  an empty leaf becomes a new node from the
//          allocator at dword_10016430, both children -8*(pos/8); the sibling
//          slot being a node bumps dword_10016434;
//      deeper: a leaf older than 0x800 is refreshed to -8*(pos/8);
//    either way bit pos/8 of the bitmap buf_ is toggled.
// Returns 0 (EAX after the count runs out); stdcall, ret 8.

static __attribute__((stdcall)) uint32_t __sub_10005251(uint32_t a1, int32_t n8)
{
  uint32_t t = a1 + (a1 >> 1) + 1, iters = t ? 31 - __builtin_clz(t) : 0;
  iters += 1;
  if (iters > a1)
    iters = a1;
  uint32_t pos = (uint32_t)n8;                         // EBP
  uint32_t at = (uint32_t)n8_0;                  // ESI
  int32_t *slot = (int32_t *)(uintptr_t)dst_0;   // EBX
  int32_t leaf = dst_1;                          // EDX
  for (;;) {
    uint32_t depth = pos - at;
    uint32_t thr = (uint32_t)(31 - __builtin_clz((uint32_t)dword_1000CA4C + 32)) << 6;
    if (depth > thr) {
      if (leaf <= 0 && pos + (uint32_t)leaf >= 0x800) {
        _bittestandcomplement(((void *)&buf_), (int32_t)(pos >> 3));
        *slot = -(int32_t)((pos >> 3) << 3);
        dword_10016434++;
      }
    } else if (leaf <= 0) {
      _bittestandcomplement(((void *)&buf_), (int32_t)(pos >> 3));
      int32_t v = -(int32_t)((pos >> 3) << 3);
      int32_t *node = (int32_t *)(uintptr_t)dword_10016430;
      node[0] = v;
      node[1] = v;
      *slot = (int32_t)(uintptr_t)node;
      dword_10016430 += 8;
      dword_1000CA4C++;
      slot = (int32_t *)((uintptr_t)slot ^ 4);
      if (*slot > 0)
        dword_10016434++;
    }
    if (--iters == 0)
      return 0;
    pos += 8;
    // descend from the root
    int32_t *node = ((int32_t *)dst);
    at = pos;
    slot = node;
    if (at == 0)
      continue;                                        // or esi,esi; je (leaf unchanged)
    const uint8_t *p = ((const uint8_t *)dst_3) + (pos >> 3) - 1;
    uint8_t al = *p;
    for (;;) {
      int32_t k, found = 0;
      for (k = 1; k <= 8; k++) {
        int32_t bit = (al >> 7) & 1;
        al = (uint8_t)(al << 1);
        int32_t *s = node + (bit ? 0 : 1);
        int32_t child = *s;
        if (child <= 0) {
          slot = s;
          leaf = child;
          at -= k;
          found = 1;
          break;
        }
        node = (int32_t *)(uintptr_t)child;
      }
      if (found)
        break;
      al = *--p;
      at -= 8;
      if ((int32_t)at <= 0) {
        leaf = node[0];
        slot = node;
        break;
      }
    }
  }
}

// sub_10006E22 @ 0x10006E22 -- the longjmp half of the compressor's early exit
// (mov esp,[10016474]; mov ebp,[10016478]; ret 4): __pe_sub_10006E22, above.

// sub_10006CB0 @ 0x10006CB0 -- in-order walk of a context subtree, emitting its
// leaves.  In vbn.exe it is assembly with a register interface (EBX node, EDX
// offset of the right child, ESI value, ECX output cursor, EDI how many pairs
// may still be written; ECX and EDI come back); here its two callers pass a
// VbnWalk.  A field <= 0 is a leaf, > 0 a child.  emit(v) writes {value, v}
// (v at +aux) and advances by 8; when the count runs out the search ends
// through vbn_walk_exit.
static inline void walk1_emit(int32_t v, VbnWalk *w)
{
  *(int32_t *)w->out = w->value;
  *(int32_t *)(w->out + w->aux) = v;
  w->out += 8;
  if (--w->left == 0)
    vbn_walk_exit();
}
static void walk1(int32_t *node, VbnWalk *w)
{
  int32_t l = node[0];
  if (l > 0)
    walk1((int32_t *)(uintptr_t)l, w);
  else
    walk1_emit(l, w);
  int32_t r = *(int32_t *)((uint8_t *)node + w->aux);
  if (r > 0)
    walk1((int32_t *)(uintptr_t)r, w);
  else if (node[0] > (int32_t)(uintptr_t)node)    // cmp eax,[eax]; jle
    walk1_emit(r, w);
}
static void vbn_walk1_entry(int32_t *node, VbnWalk *w)
{
  walk1(node, w);
}

// sub_10006E31 @ 0x10006E31 -- the same walk as sub_10006CB0 with a different
// emit, the right child always at +4, and `aux` a byte table instead of an
// offset:
//   emit(v): [out+4] = v; if table[(-v) >> 3] != dword_10016488 then
//            [out] = value, advance by 8, and end the search when the count runs out.

static inline void walk2_emit(int32_t v, VbnWalk *w)
{
  *(int32_t *)(w->out + 4) = v;
  uint32_t t = (0u - (uint32_t)v) >> 3;                   // neg ebx; shr ebx,3
  if ((uint32_t)((uint8_t *)(uintptr_t)w->aux)[t] != (uint32_t)dword_10016488) {
    *(int32_t *)w->out = w->value;
    w->out += 8;
    if (--w->left == 0)
      vbn_walk_exit();
  }
}
static void walk2(int32_t *node, VbnWalk *w)
{
  int32_t l = node[0];
  if (l > 0)
    walk2((int32_t *)(uintptr_t)l, w);
  else
    walk2_emit(l, w);
  int32_t r = node[1];
  if (r > 0)
    walk2((int32_t *)(uintptr_t)r, w);
  else if (node[0] > (int32_t)(uintptr_t)node)    // cmp eax,[eax]; jle
    walk2_emit(r, w);
}
static void vbn_walk2_entry(int32_t *node, VbnWalk *w)
{
  walk2(node, w);
}

// sub_100061F4 @ 0x100061F4 -- generated by tools/extract.py from vbn.exe.c lines 7101..7428
// Hex-Rays: int __stdcall sub_100061F4(unsigned int a1, unsigned int *a2, int a3, _DWORD *i_1)
// real convention: stdcall, ret 16


static __attribute__((stdcall)) int32_t __sub_100061F4(uint32_t a1, uint32_t *a2, int32_t a3, uint32_t *i_1)
{
  uint32_t v4; // edx
  uint32_t *i; // edi
  uint32_t v6; // ebx
  uint64_t v7; // rax
  uint32_t v8; // eax
  int32_t n0x7FFFFFFF; // ecx
  uint32_t n0x7FFFFFFF_1; // edx
  uint32_t v11; // eax
  uint32_t v12; // eax
  uint32_t v13; // eax
  uint32_t v14; // eax
  uint32_t v15; // eax
  uint32_t v16; // eax
  uint32_t v17; // eax
  bool v18; // cf
  uint32_t v19; // edx
  uint32_t v20; // eax
  uint64_t v21; // rax
  uint32_t v22; // ett
  uint32_t *i_3; // esi
  int32_t v24; // ebx
  uint32_t v25; // ebx
  int32_t n32; // ebx
  int32_t v27; // edi
  int32_t v28; // edx
  uint32_t v29; // eax
  int32_t n0x7FFFFFFF_2; // esi
  bool v31; // sf
  uint32_t *i_2; // [esp+Ch] [ebp-Ch]
  uint32_t v34; // [esp+10h] [ebp-8h]
  uint32_t v35; // [esp+14h] [ebp-4h]  high - low, which wraps

  v34 = *a2;
  v35 = (uint32_t)dword_10016410 - (uint32_t)::n0x7FFFFFFF;
  if ( 2 * a1 > (uint32_t)dword_10016410 - (uint32_t)::n0x7FFFFFFF )
  {
    v34 = __sub_10004993(*a2);
    v35 = -1;
  }
  if ( a3 >= 0 )
  {
    v4 = 0;
    for ( i = i_1; a3 != i[1]; i += 16 )
    {
      v4 += *i;
      if ( a3 == i[3] )
      {
        i += 2;
        break;
      }
      v4 += i[2];
      if ( a3 == i[5] )
      {
        i += 4;
        break;
      }
      v4 += i[4];
      if ( a3 == i[7] )
      {
        i += 6;
        break;
      }
      v4 += i[6];
      if ( a3 == i[9] )
      {
        i += 8;
        break;
      }
      v4 += i[8];
      if ( a3 == i[11] )
      {
        i += 10;
        break;
      }
      v4 += i[10];
      if ( a3 == i[13] )
      {
        i += 12;
        break;
      }
      v4 += i[12];
      if ( a3 == i[15] )
      {
        i += 14;
        break;
      }
      v4 += i[14];
    }
    v6 = v4;
    i_2 = i;
    HIDWORD(v7) = *i + v4;
    LODWORD(v7) = v35 + 1;
    if ( v35 == -1 )
    {
      if ( HIDWORD(v7) == a1 )
        goto LABEL_25;
    }
    else
    {
      v7 = HIDWORD(v7) * (uint64_t)(uint32_t)v7;
    }
    LODWORD(v7) = v7 / a1;
LABEL_25:
    dword_10016410 = ::n0x7FFFFFFF + v7 - 1;
    LODWORD(v7) = v35 + 1;
    if ( v35 == -1 )
    {
      HIDWORD(v7) = v6;
      if ( v6 == a1 )
      {
LABEL_29:
        ::n0x7FFFFFFF += v7;
        goto LABEL_76;
      }
    }
    else
    {
      v7 = v6 * (uint64_t)(uint32_t)v7;
    }
    LODWORD(v7) = v7 / a1;
    goto LABEL_29;
  }
  v8 = ::n0x7FFFFFFF_1 + v34;
  n0x7FFFFFFF = 33 - ::n0x7FFFFFFF_1;
  n0x7FFFFFFF_1 = ::n0x7FFFFFFF_2;
  while ( --n0x7FFFFFFF )
  {
    n0x7FFFFFFF_1 += _bittest(
                       (int32_t *)aVbnVer1001,// "                                                \nvbn ver.1...."
                       v8) + n0x7FFFFFFF_1;
    v11 = v8 + 1;
    if ( !--n0x7FFFFFFF )
      break;
    n0x7FFFFFFF_1 += _bittest(
                       (int32_t *)aVbnVer1001,// "                                                \nvbn ver.1...."
                       v11) + n0x7FFFFFFF_1;
    v12 = v11 + 1;
    if ( !--n0x7FFFFFFF )
      break;
    n0x7FFFFFFF_1 += _bittest(
                       (int32_t *)aVbnVer1001,// "                                                \nvbn ver.1...."
                       v12) + n0x7FFFFFFF_1;
    v13 = v12 + 1;
    if ( !--n0x7FFFFFFF )
      break;
    n0x7FFFFFFF_1 += _bittest(
                       (int32_t *)aVbnVer1001,// "                                                \nvbn ver.1...."
                       v13) + n0x7FFFFFFF_1;
    v14 = v13 + 1;
    if ( !--n0x7FFFFFFF )
      break;
    n0x7FFFFFFF_1 += _bittest(
                       (int32_t *)aVbnVer1001,// "                                                \nvbn ver.1...."
                       v14) + n0x7FFFFFFF_1;
    v15 = v14 + 1;
    if ( !--n0x7FFFFFFF )
      break;
    n0x7FFFFFFF_1 += _bittest(
                       (int32_t *)aVbnVer1001,// "                                                \nvbn ver.1...."
                       v15) + n0x7FFFFFFF_1;
    v16 = v15 + 1;
    if ( !--n0x7FFFFFFF )
      break;
    n0x7FFFFFFF_1 += _bittest(
                       (int32_t *)aVbnVer1001,// "                                                \nvbn ver.1...."
                       v16) + n0x7FFFFFFF_1;
    v17 = v16 + 1;
    if ( !--n0x7FFFFFFF )
      break;
    n0x7FFFFFFF_1 += _bittest(
                       (int32_t *)aVbnVer1001,// "                                                \nvbn ver.1...."
                       v17) + n0x7FFFFFFF_1;
    v8 = v17 + 1;
  }
  ::n0x7FFFFFFF_2 = n0x7FFFFFFF_1;
  ::n0x7FFFFFFF_1 = n0x7FFFFFFF;
  v18 = n0x7FFFFFFF_1 < ::n0x7FFFFFFF;
  v19 = n0x7FFFFFFF_1 - ::n0x7FFFFFFF;
  v20 = v19 + 1;
  if ( v19 == -1 )
  {
    HIDWORD(v21) = a1;
    v22 = v18 + 1;
    v18 = v20 < v22;
    LODWORD(v21) = v20 - v22;
    if ( !v18 )
    {
      if ( v35 != -1 )
      {
        LODWORD(v21) = v21 / (uint32_t)(v35 + 1);
        goto LABEL_50;
      }
      goto LABEL_44;
    }
LABEL_45:
    --HIDWORD(v21);
    if ( v35 != -1 )
    {
      LODWORD(v21) = v21 / (uint32_t)(v35 + 1);
      goto LABEL_50;
    }
    goto LABEL_44;
  }
  v21 = a1 * (uint64_t)v20;
  v18 = (uint32_t)v21 < (uint32_t)(HIDWORD(v21) != 0) + 1;
  LODWORD(v21) = v21 - ((HIDWORD(v21) != 0) + 1);
  if ( v18 )
    goto LABEL_45;
  if ( v35 == -1 )
  {
LABEL_44:
    LODWORD(v21) = HIDWORD(v21);
    goto LABEL_50;
  }
  LODWORD(v21) = v21 / (uint32_t)(v35 + 1);
LABEL_50:
  i_3 = i_1;
  HIDWORD(v21) = 0;
  while ( 1 )
  {
    HIDWORD(v21) += *i_3;
    if ( HIDWORD(v21) > (uint32_t)v21 )
      break;
    HIDWORD(v21) += i_3[2];
    if ( HIDWORD(v21) > (uint32_t)v21 )
    {
      i_3 += 2;
      break;
    }
    HIDWORD(v21) += i_3[4];
    if ( HIDWORD(v21) > (uint32_t)v21 )
    {
      i_3 += 4;
      break;
    }
    HIDWORD(v21) += i_3[6];
    if ( HIDWORD(v21) > (uint32_t)v21 )
    {
      i_3 += 6;
      break;
    }
    HIDWORD(v21) += i_3[8];
    if ( HIDWORD(v21) > (uint32_t)v21 )
    {
      i_3 += 8;
      break;
    }
    HIDWORD(v21) += i_3[10];
    if ( HIDWORD(v21) > (uint32_t)v21 )
    {
      i_3 += 10;
      break;
    }
    HIDWORD(v21) += i_3[12];
    if ( HIDWORD(v21) > (uint32_t)v21 )
    {
      i_3 += 12;
      break;
    }
    HIDWORD(v21) += i_3[14];
    if ( HIDWORD(v21) > (uint32_t)v21 )
    {
      i_3 += 14;
      break;
    }
    i_3 += 16;
  }
  i_2 = i_3;
  v24 = HIDWORD(v21);
  LODWORD(v21) = v35 + 1;
  if ( v35 == -1 )
  {
    if ( HIDWORD(v21) == a1 )
      goto LABEL_71;
  }
  else
  {
    v21 = HIDWORD(v21) * (uint64_t)(uint32_t)v21;
  }
  LODWORD(v21) = v21 / a1;
LABEL_71:
  dword_10016410 = ::n0x7FFFFFFF + v21 - 1;
  v25 = v24 - *i_3;
  LODWORD(v21) = v35 + 1;
  if ( v35 != -1 )
  {
    v21 = v25 * (uint64_t)(uint32_t)v21;
LABEL_74:
    LODWORD(v21) = v21 / a1;
    goto LABEL_75;
  }
  HIDWORD(v21) = v25;
  if ( v25 != a1 )
    goto LABEL_74;
LABEL_75:
  ::n0x7FFFFFFF += v21;
LABEL_76:
  n32 = 32;
  v27 = dword_10016410;
  v28 = ::n0x7FFFFFFF ^ dword_10016410;
  if ( (::n0x7FFFFFFF ^ dword_10016410) >= 0 )
  {
    v29 = v34;
    n0x7FFFFFFF_2 = ::n0x7FFFFFFF;
    do
    {
      if ( n0x7FFFFFFF_2 < 0 )
        _bittestandcomplement(
          (int32_t *)aVbnVer1001,        // "                                                \nvbn ver.1...."
          v29);
      --n32;
      ++v29;
      n0x7FFFFFFF_2 = (int32_t)((uint32_t)n0x7FFFFFFF_2 << 1);   // shl/rcl: wrap
      v27 = (int32_t)((uint32_t)v27 << 1 | 1);
      v31 = (v28 & 0x40000000) != 0;
      v28 = (int32_t)((uint32_t)v28 << 1);
    }
    while ( !v31 );
    v34 = v29;
    ::n0x7FFFFFFF = n0x7FFFFFFF_2;
    dword_10016410 = v27;
  }
  ::n0x7FFFFFFF_1 = n32;
  *a2 = v34;
  return i_2[1];
}

// sub_100064C0 @ 0x100064C0 -- generated by tools/extract.py from vbn.exe.c lines 7435..7978
// Hex-Rays: int __stdcall sub_100064C0(int a1, unsigned int *p_n0x10000, unsigned int n8)
// real convention: stdcall, ret 12


static __attribute__((stdcall)) int32_t __sub_100064C0(int32_t a1, uint32_t *p_n0x10000, uint32_t n8)
{
  uint32_t v3; // edi
  int32_t n2; // eax
  int32_t v5; // eax
  int32_t n256; // ecx
  uint32_t *i; // esi
  uint32_t n2048; // edx
  int32_t *v9; // edi
  int32_t v10; // ebx
  int32_t *v11; // edi
  int32_t v12; // ebx
  uint32_t v13; // edx
  int32_t v14; // ebx
  int32_t *v15; // edx
  int32_t *v16; // edi
  int16_t n2_1; // cx
  int32_t v18; // eax
  uint32_t *v19; // esi
  int32_t v20; // eax
  int32_t v21; // eax
  int32_t v22; // eax
  uint32_t v23; // ebx
  uint64_t v24; // rax
  uint32_t v25; // eax
  uint32_t v26; // edx
  uint32_t v27; // eax
  int32_t v28; // ecx
  uint32_t v29; // edx
  uint32_t v30; // eax
  uint32_t v31; // edx
  uint32_t v32; // ecx
  int8_t v33; // cl
  int32_t *v34; // eax
  int32_t v35; // ebx
  int32_t *v36; // eax
  int32_t v37; // ebx
  int8_t v38; // ch
  uint32_t *v39; // esi
  int32_t n2_2; // ebx
  int32_t v41; // eax
  int32_t v42; // edx
  int32_t *v43; // edi
  int32_t v44; // edx
  int32_t v45; // eax
  int32_t v46; // edx
  uint32_t *v47; // edi
  uint32_t v48; // edx
  int32_t v49; // eax
  int32_t v50; // eax
  int32_t v51; // edx
  int32_t v52; // eax
  int32_t v53; // edx
  int32_t *v54; // edi
  int32_t v55; // edx
  int32_t v56; // eax
  int32_t v57; // edx
  uint32_t v58; // eax
  uint32_t v59; // ecx
  int32_t *v60; // esi
  int32_t v61; // eax
  int32_t n362; // edx
  int8_t *v63; // edx
  int32_t v64; // ecx
  uint32_t *i_1; // esi
  uint32_t *v66; // edi
  uint32_t v67; // eax
  uint32_t v68; // edx
  uint32_t v69; // eax
  uint32_t v70; // edx
  int32_t n362_3; // edx
  int32_t v72; // eax
  int32_t n363; // edx
  int32_t *v74; // esi
  int32_t *i_2; // edi
  int32_t v76; // eax
  uint32_t v77; // edx
  int32_t v78; // eax
  int32_t v79; // edx
  int32_t result; // eax
  int32_t n362_1; // edx
  int32_t n362_2; // ecx
  int8_t *v83; // edx
  int32_t v84; // ecx
  int32_t v85; // [esp+Ch] [ebp-1Ch]
  uint32_t v86; // [esp+10h] [ebp-18h]
  uint32_t n0x10000; // [esp+14h] [ebp-14h]
  uint32_t n2048_1; // [esp+18h] [ebp-10h]
  int32_t v89; // [esp+1Ch] [ebp-Ch]
  int32_t v90; // [esp+20h] [ebp-8h]
  int32_t v91; // [esp+24h] [ebp-4h]

  dword_100164C4 = (uint32_t)dword_100164C4 >> 1;
  _BitScanReverse(&v3, ((uint32_t)n0x384 >> 3) + 1);
  dword_100164B8 = ((uint32_t)n4 >> 4)
                 + 54
                 - (((uint32_t)n0x4000 >> 11)
                  + 1)
                 - (((uint32_t)n0x2000 >> 10)
                  + 1)
                 - (((uint32_t)n0xFFE >> 9)
                  + 1)
                 - (((uint32_t)n0x384 >> 8)
                  + 1)
                 - v3
                 - ((((uint32_t)n0x4000 >> 11) + 1) >> 1)
                 - ((((uint32_t)n0x2000 >> 10) + 1) >> 1)
                 - ((((uint32_t)n0xFFE >> 9) + 1) >> 1)
                 - ((((uint32_t)n0x384 >> 8) + 1) >> 1);
  if ( dword_100164B8 <= 0 )
  {
    dword_100164B8 = 0;
    if ( n4 <= 4 )
    {
      ::n2_1 = 0;
      n0xFFE = 4096;
      n0x2000 = 0x2000;
      n0x4000 = 0x4000;
    }
  }
  n2 = 1;
  if ( (uint32_t)n0x384 >= 0x2EC )
  {
    n2 = 2;
    if ( (uint32_t)n0x384 >= 0x37F )
    {
      n2 = 4;
      if ( (uint32_t)n0x384 >= 0x489 )
      {
        n2 = 8;
        if ( (uint32_t)n0x384 >= 0x56F )
        {
          n2 = 16;
          if ( (uint32_t)n0x384 >= 0x63F )
          {
            n2 = 32;
            if ( (uint32_t)n0x384 >= 0x70F )
            {
              n2 = 128;
              n0xFFE = 4096;
              n0x2000 = 0x2000;
              n0x4000 = 0x4000;
              if ( (uint32_t)n0x384 >= 0x75C )
                n2 = 1024;
            }
          }
        }
      }
    }
  }
  if ( n2_0 != n2 )
  {
    v5 = n2 - n2_0;
    n2_0 += v5;
    n256 = 256;
    i = &i_;
    n2048 = ::n2048;
    do
    {
      n2048 += v5;
      *i += v5;
      i += 2;
      --n256;
    }
    while ( n256 );
    ::n2048 = n2048;
  }
  if ( a1 < 0 || (v9 = (int32_t *)dword_10010060[a1], v10 = *v9, *v9 = 0, v89 = v10, ::n2048 -= v10, a1 != dword_10016488) )
  {
    if ( dword_10016488 >= 0 )
    {
      v11 = (int32_t *)dword_10010060[dword_10016488];
      v12 = *v11;
      *v11 = 0;
      v90 = v12;
      ::n2048 -= v12;
    }
  }
  v13 = 0;
  v86 = 0;
  if ( ::n2_1 )
  {
    v14 = 0;
    v15 = dword_1000F060;
    v16 = dword_1000D558;
    n2_1 = ::n2_1;
    do
    {
      while ( 1 )
      {
        while ( 1 )
        {
          v18 = *((uint8_t *)v16 + 4);
          if ( v18 != a1 )
            break;
          v16 += 2;
          ++HIBYTE(n2_1);
          LOBYTE(n2_1) = n2_1 - 1;
          if ( !(uint8_t)n2_1 )
            goto LABEL_52;
        }
        v19 = (uint32_t *)buf__0[v18];
        if ( !v19 )
          break;
        v20 = *v16 - dword_100164C4;
        *v19 += v20;
        v14 += v20;
        v16 += 2;
        LOBYTE(n2_1) = n2_1 - 1;
        if ( !(uint8_t)n2_1 )
          goto LABEL_52;
      }
      buf__0[v18] = (int32_t)v15;
      LOBYTE(dword_100164C8) = v18;
      v15[1] = v18;
      if ( (uint32_t)n4 < 0x13 )
      {
        if ( buf_2[dword_100164C8] )
        {
          if ( (uint8_t)buf_2[dword_100164C8] > 1u )
          {
            if ( (uint8_t)buf_2[dword_100164C8] > 2u )
            {
              if ( (uint8_t)buf_2[dword_100164C8] > 3u )
              {
                if ( (uint8_t)buf_2[dword_100164C8] > 6u )
                  v21 = 8 * *v16;
                else
                  v21 = 5 * *v16;
              }
              else
              {
                v21 = 4 * *v16;
              }
            }
            else
            {
              v21 = 3 * *v16;
            }
          }
          else
          {
            v21 = 2 * *v16;
          }
        }
        else
        {
          v21 = (uint32_t)*v16 >> 3;
        }
      }
      else if ( buf_2[dword_100164C8] )
      {
        if ( (uint8_t)buf_2[dword_100164C8] > 1u )
        {
          if ( (uint8_t)buf_2[dword_100164C8] > 2u )
          {
            if ( (uint8_t)buf_2[dword_100164C8] > 3u )
            {
              if ( (uint8_t)buf_2[dword_100164C8] > 4u )
                v21 = *v16 + *v16 + ((uint32_t)*v16 >> 1);
              else
                v21 = 2 * *v16;
            }
            else
            {
              v21 = *v16 + ((uint32_t)*v16 >> 1);
            }
          }
          else
          {
            v21 = *v16;
          }
        }
        else
        {
          v21 = (uint32_t)*v16 >> 1;
        }
      }
      else
      {
        v21 = (uint32_t)*v16 >> 4;
      }
      v22 = *v16 + v21 - dword_100164C4;
      *v15 = v22;
      v14 += v22;
      v15 += 2;
      v16 += 2;
      LOBYTE(n2_1) = n2_1 - 1;
    }
    while ( (uint8_t)n2_1 );
LABEL_52:
    v85 = v14;
    v13 = (uint32_t)((int8_t *)v15 - (int8_t *)dword_1000F060) >> 3;
    v86 = v13;
    if ( HIBYTE(n2_1) )
      --::n2_1;
  }
  if ( v13 )
  {
    v23 = ::n2_1 * v85;
    v24 = ::n2048
        * (uint64_t)((::n2_1 - v86) * ((3 * dword_100164B8 + 5 * (((uint32_t)n4 >> 2) + n4)) >> 3));
    if ( is_mul_ok(::n2048, (::n2_1 - v86) * ((3 * dword_100164B8 + 5 * (((uint32_t)n4 >> 2) + n4)) >> 3)) )
      v25 = (uint32_t)v24 >> 3;
    else
      v25 = v24 >> 3;
    n2048_1 = ::n2048;
    v26 = v25 >> (dword_1000CA54[(uint32_t)n55 >> 5] & 31);
    v27 = v25 - v26;
    v28 = ((uint32_t)(48 - dword_100164B8) >> 3) - 4;
    if ( v28 < 0 )
      v29 = v26 << ((4 - ((uint32_t)(48 - dword_100164B8) >> 3)) & 31);
    else
      v29 = v26 >> (v28 & 31);
    v30 = v29 + v27;
    v31 = v29;   // bsr edx,eax: EDX holds v29, and bsr leaves it there when v30 == 0
    _BitScanReverse(&v31, v30);
    _BitScanReverse(&v32, v23);
    v33 = 31 - v31 + v32;
    ::n2_1 = ((uint64_t)v30 << ((v33 & 0x1F) & 31)) / v23;
    if ( buf__0[(uint8_t)buf_3[dword_100164B0]] )
    {
      v34 = (int32_t *)buf__0[(uint8_t)buf_3[dword_100164B0]];
      v35 = *v34;
      *v34 *= 8;
      *v34 -= v35;
      *v34 = (uint32_t)*v34 >> 2;
    }
    if ( buf__0[(uint8_t)buf_4[dword_100164B4]] )
    {
      v36 = (int32_t *)buf__0[(uint8_t)buf_4[dword_100164B4]];
      v37 = *v36;
      *v36 *= 2;
      *v36 += v37;
      *v36 = (uint32_t)*v36 >> 1;
    }
    v38 = v86;
    v39 = (uint32_t *)dword_1000F058;
    n2_2 = ::n2_1;
    if ( (uint32_t)n0x384 < 0x6C9 )
    {
      if ( dword_100164AC )
      {
        do
        {
          v39 += 2;
          v52 = ((uint32_t)n2_2 * (uint64_t)*v39) >> (v33 & 31);
          ::n2048 += v52;
          v53 = v39[1];
          LOBYTE(dword_100164C8) = v53;
          v54 = (int32_t *)dword_10010060[v53];
          buf__0[v53] -= (int32_t)v39;
          v55 = *v54;
          *v39 = *v54;
          v56 = v55 + v52;
          v57 = (uint8_t)buf_2[dword_100164C8] << 6;
          ::n2048 += v57;
          *v54 = v57 + v56;
          --v38;
        }
        while ( v38 );
      }
      else
      {
        do
        {
          v39 += 2;
          v45 = ((uint32_t)n2_2 * (uint64_t)*v39) >> (v33 & 31);
          ::n2048 += v45;
          v46 = v39[1];
          LOBYTE(dword_100164C8) = v46;
          v47 = (uint32_t *)dword_10010060[v46];
          buf__0[v46] -= (int32_t)v39;
          v48 = *v47;
          *v39 = *v47;
          v49 = v48 + v45;
          v48 >>= 2;
          v50 = v48 + v49;
          ::n2048 += v48;
          v51 = (uint8_t)buf_2[dword_100164C8] << 6;
          ::n2048 += v51;
          *v47 = v51 + v50;
          --v38;
        }
        while ( v38 );
      }
    }
    else
    {
      do
      {
        v39 += 2;
        v41 = ((uint32_t)n2_2 * (uint64_t)*v39) >> (v33 & 31);
        ::n2048 += v41;
        v42 = v39[1];
        v43 = (int32_t *)dword_10010060[v42];
        buf__0[v42] -= (int32_t)v39;
        v44 = *v43;
        *v39 = *v43;
        *v43 = v44 + v41;
        --v38;
      }
      while ( v38 );
    }
  }
  n0x10000 = *p_n0x10000;
  if ( dword_1000D4C4 )
    v91 = __sub_100061F4(::n2048, (uint32_t *)p_n0x10000, (uint8_t)dst_3[n8 >> 3], (uint32_t *)&i_);
  else
    v91 = __sub_100061F4(::n2048, (uint32_t *)p_n0x10000, -1, (uint32_t *)&i_);
  v58 = (*p_n0x10000 - n0x10000) << 8;
  n0x384 = (v58 + (n0x384 << 8) - n0x384) >> 8;
  v58 *= 2;
  n0xFFE = (v58 + (n0xFFE << 9) - n0xFFE) >> 9;
  v58 *= 2;
  n0x2000 = (v58 + (n0x2000 << 10) - n0x2000) >> 10;
  n0x4000 = (2 * v58 + (n0x4000 << 11) - n0x4000) >> 11;
  v59 = v86;
  if ( v86 )
  {
    v60 = dword_1000F060;
    ::n2048 = n2048_1;
    do
    {
      *(uint32_t *)dword_10010060[v60[1]] = *v60;
      v60 += 2;
      --v59;
    }
    while ( v59 );
  }
  if ( a1 < 0 || (*(uint32_t *)dword_10010060[a1] = v89, ::n2048 += v89, a1 != dword_10016488) )
  {
    if ( dword_10016488 >= 0 )
    {
      *(uint32_t *)dword_10010060[dword_10016488] = v90;
      ::n2048 += v90;
    }
  }
  v61 = dword_100164C8;
  LOBYTE(v61) = v91;
  dst_3[n8 >> 3] = v91;
  if ( buf_2[v61] != -1 )
  {
    ++dword_100164AC;
    if ( !buf_2[v61] )
      dword_100164AC = 0;
    ++buf_2[v61];
  }
  n362 = ::n362_1;
  ::n362_1 = ((uint16_t)::n362_1 + 1) & 0x3FF;
  v63 = &buf_0[4 * n362];
  v64 = *(uint32_t *)v63;
  *(uint32_t *)v63 = v61;
  if ( v64 >= 0 && buf_2[v64] )
    --buf_2[v64];
  if ( --dword_10016494 )
  {
    result = v91;
    *(uint32_t *)dword_10010060[v91] += 16;
    n362_1 = ::n362;
    n362_2 = ::n362 + 1;
    if ( ::n362 == 362 )
      n362_2 = 0;
    ::n362 = n362_2;
    v83 = &buf[4 * n362_1];
    v84 = *(uint32_t *)v83;
    *(uint32_t *)v83 = v91;
    if ( v84 < 0 )
      ::n2048 += 16;
    else
      *(uint32_t *)dword_10010060[v84] -= 16;
  }
  else
  {
    dword_10016494 = (uint8_t)(BYTE1(n0x384) + 1);
    i_1 = (uint32_t *)dword_10010060[v91];
    *i_1 += 16;
    v66 = i_1 - 2;
    v67 = *i_1;
    v68 = *(i_1 - 2);
    if ( *i_1 > v68 )
    {
      do
      {
        if ( i_1 <= &i_ )
          break;
        *i_1 = v68;
        *v66 = v67;
        v69 = i_1[1];
        v70 = v66[1];
        i_1[1] = v70;
        v66[1] = v69;
        dword_10010060[v70] = (int32_t)i_1;
        dword_10010060[v69] = (int32_t)v66;
        i_1 -= 2;
        v66 -= 2;
        v67 = *i_1;
        v68 = *v66;
      }
      while ( *i_1 > *v66 );
    }
    n362_3 = ::n362;
    v72 = *(uint32_t *)&buf[4 * ::n362];
    *(uint32_t *)&buf[4 * ::n362] = v91;
    n363 = n362_3 + 1;
    if ( n363 == 363 )
      n363 = 0;
    ::n362 = n363;
    if ( v72 < 0 )
    {
      ::n2048 += 16;
    }
    else
    {
      v74 = (int32_t *)dword_10010060[v72];
      *v74 -= 16;
      i_2 = v74 + 2;
      v76 = *v74;
      v77 = v74[2];
      if ( *v74 < v77 )
      {
        do
        {
          if ( i_2 >= i_4 )
            break;
          *v74 = v77;
          *i_2 = v76;
          v78 = v74[1];
          v79 = i_2[1];
          v74[1] = v79;
          i_2[1] = v78;
          dword_10010060[v79] = (int32_t)v74;
          dword_10010060[v78] = (int32_t)i_2;
          v74 += 2;
          i_2 += 2;
          v76 = *v74;
          v77 = *i_2;
        }
        while ( *v74 < (uint32_t)*i_2 );
      }
    }
    return v91;
  }
  return result;
}

// sub_10005C50 @ 0x10005C50 -- generated by tools/extract.py from vbn.exe.c lines 6645..7077
// Hex-Rays: int __stdcall sub_10005C50(unsigned int *p_n0x10000, _DWORD *a2, unsigned int n8)
// real convention: stdcall, ret 12


static __attribute__((stdcall)) int32_t __sub_10005C50(uint32_t *p_n0x10000, uint32_t *a2, uint32_t n8)
{
  int32_t v3; // eax
  int32_t result; // eax
  uint32_t v5; // edx
  int8_t *v6; // ecx
  int32_t v7; // eax
  uint32_t v8; // ebx
  int32_t *v9; // edx
  int32_t *v10; // edi
  int8_t *v11; // eax
  int8_t *v12; // esi
  bool v13; // zf
  uint32_t n255_1; // eax
  int8_t *v15; // edi
  int8_t n255_2; // cl
  uint32_t *v17; // esi
  int32_t v18; // eax
  int32_t v19; // eax
  uint32_t *v20; // eax
  uint32_t v21; // ecx
  uint32_t v22; // esi
  uint32_t v23; // edx
  uint32_t v24; // ecx
  uint32_t v25; // edx
  uint32_t v26; // edx
  uint32_t v27; // edx
  uint32_t v28; // edx
  uint32_t v29; // edx
  uint32_t v30; // eax
  uint32_t v31; // ecx
  int8_t v32; // cl
  uint32_t v33; // ebx
  int8_t v34; // ch
  int32_t *v35; // esi
  int32_t v36; // eax
  int32_t v37; // edx
  uint32_t *v38; // edi
  int32_t v39; // edx
  uint32_t n256_1; // edx
  uint32_t *v41; // esi
  uint32_t v42; // ecx
  uint8_t *v43; // edx
  int8_t *v44; // edi
  uint32_t n255_6; // eax
  int32_t v46; // ecx
  int32_t n255_7; // eax
  int32_t v48; // eax
  int8_t *v49; // edi
  uint32_t n255_8; // ebx
  uint8_t *v51; // edx
  int32_t v52; // ecx
  uint8_t *v53; // edx
  int8_t *v54; // edi
  uint32_t n255_3; // eax
  int32_t v56; // ecx
  int32_t n255_4; // eax
  int32_t v58; // eax
  int8_t *v59; // edi
  uint32_t n255_5; // ebx
  uint8_t *v61; // edx
  int32_t v62; // ecx
  uint32_t v63; // ecx
  int32_t *v64; // esi
  int32_t n362; // edx
  int8_t *v66; // edx
  int32_t v67; // ecx
  int8_t *v68; // [esp-8h] [ebp-38h]
  int32_t *v69; // [esp-4h] [ebp-34h]
  uint32_t v70; // [esp+Ch] [ebp-24h]
  uint32_t n255; // [esp+10h] [ebp-20h]
  uint32_t n256; // [esp+14h] [ebp-1Ch]
  int32_t v73; // [esp+18h] [ebp-18h]
  int32_t n2; // [esp+1Ch] [ebp-14h]
  int8_t *v75; // [esp+20h] [ebp-10h]
  uint32_t n256_2; // [esp+28h] [ebp-8h]
  int32_t v77; // [esp+2Ch] [ebp-4h]

  v3 = 0;
  if ( n8 >> 3 )
    BYTE1(v3) = dst_3[(n8 >> 3) - 1];
  dword_100164C8 = v3;
  if ( n2_1 )
  {
    v73 = i_5[0];
    ::n256 -= i_5[0];
    v5 = 9 * dword_100164AC + n4 + ((uint32_t)dword_100164B8 >> 2) + 1;
    n2 = 0;
    i_5[0] = 5 * i_5[0] / (v5 + (v5 >> 4));
    ::n256 += i_5[0];
    n255 = 255;
    v6 = &dst_3[-dword_1000D55C[0] >> 3];
    v75 = v6;
    dword_1001645C = (int32_t)&dst_3[n8 >> 3];
    if ( (uint32_t)(dword_1001645C - (uint32_t)v6) < 0xFF )
      n255 = dword_1001645C - (uint32_t)v6;
    if ( dst_4 - dword_1001645C < n255 )
      n255 = dst_4 - dword_1001645C;
    v7 = (uint8_t)*v6;
    dword_1000D55C[0] = v7;
    if ( (uint8_t)v7 == (uint8_t)(buf_3[dword_100164B0]))
    {
      n2 = 1;
      if ( (uint8_t)v7 == (uint8_t)(buf_3[dword_100164B4]))
        n2 = 2;
    }
    v8 = 0;
    v9 = dword_1000F060;
    v10 = dword_1000D560;
    BYTE1(v6) = n2_1;
    LOBYTE(v6) = 1;
    if ( (uint8_t)n2_1 > 1u )
    {
      do
      {
        while ( 1 )
        {
          v11 = v75;
          v12 = &dst_3[(uint32_t)-v10[1] >> 3];
          v13 = *v12 == *v75;
          LOBYTE(v11) = *v12;
          v10[1] = (int32_t)v11;
          n255_1 = 0;
          if ( v13 )
          {
            v69 = v10;
            v68 = v6;
            n255_1 = n255;
            v15 = v75;
            if ( v75 <= v12 && n255 > dword_1001645C - (int32_t)v12 )
              n255_1 = dword_1001645C - (uint32_t)v12;
            n255_2 = n255_1;
            do
            {
              if ( !--n255_2 )
                break;
              if ( v15[1] != v12[1] )
                break;
              if ( !--n255_2 )
                break;
              if ( v15[2] != v12[2] )
                break;
              if ( !--n255_2 )
                break;
              if ( v15[3] != v12[3] )
                break;
              if ( !--n255_2 )
                break;
              if ( v15[4] != v12[4] )
                break;
              if ( !--n255_2 )
                break;
              v12 += 5;
              v15 += 5;
            }
            while ( *v15 == *v12 );
            LOBYTE(n255_1) = n255_1 - n255_2;
            v6 = v68;
            v10 = v69;
          }
          v17 = (uint32_t *)buf__0[n255_1];
          if ( !v17 )
            break;
          v18 = *v10 - dword_100164C4;
          *v17 += v18;
          v8 += v18;
          v10 += 2;
          LOBYTE(v6) = (uint8_t)(uintptr_t)v6 + 1;
          if ( (uint8_t)(uintptr_t)v6 >= BYTE1(v6) )
            goto LABEL_33;
        }
        buf__0[n255_1] = (int32_t)v9;
        v9[1] = n255_1;
        LOBYTE(n255_1) = BYTE1(v6) - (uint8_t)(uintptr_t)v6;
        v19 = *v10 + (n255_1 >> 1) - dword_100164C4;
        *v9 = v19;
        v8 += v19;
        v9 += 2;
        v10 += 2;
        LOBYTE(v6) = (uint8_t)(uintptr_t)v6 + 1;
      }
      while ( (uint8_t)(uintptr_t)v6 < BYTE1(v6) );
    }
LABEL_33:
    v70 = (uint32_t)((int8_t *)v9 - (int8_t *)dword_1000F060) >> 3;
    if ( v70 )
    {
      v20 = (uint32_t *)buf__0[0];
      if ( buf__0[0] )
      {
        _BitScanReverse(&v21, ((uint32_t)(48 - dword_100164B8) >> 3) + 1);
        v22 = (uint32_t)dword_100164A8 >> (v21 & 31);
        v23 = *(uint32_t *)buf__0[0];
        if ( n2 )
        {
          v22 = dword_100164A8;
          v23 = ((3 * v23) >> 2) + 1;
          if ( n2 != 1 )
            v23 = ((7 * v23) >> 3) + 1;
        }
        _BitScanReverse(&v24, v22 + 1);
        v25 = (v23 >> (v24 & 31)) + v23;
        *(uint32_t *)buf__0[0] = v25;
        LOBYTE(dword_100164C8) = dword_1000D55C[0];
        if ( buf_2[dword_1000D55C[0]] )
        {
          if ( (uint8_t)buf_2[dword_1000D55C[0]] > 1u )
          {
            if ( (uint8_t)buf_2[dword_1000D55C[0]] > 2u )
            {
              if ( (uint8_t)buf_2[dword_1000D55C[0]] > 3u )
              {
                if ( (uint8_t)buf_2[dword_1000D55C[0]] > 5u )
                {
                  v29 = v25 >> 2;
                }
                else
                {
                  v28 = v25 >> 3;
                  *v20 -= v28;
                  v29 = v28 >> 1;
                }
                *v20 -= v29;
              }
              else
              {
                v27 = v25 >> 3;
                *v20 -= v27;
                *v20 -= v27 >> 1;
              }
            }
            else
            {
              v26 = v25 >> 3;
              *v20 -= v26;
              *v20 -= v26 >> 2;
            }
          }
          else
          {
            *v20 -= v25 >> 5;
          }
        }
        else
        {
          *v20 += v25 >> 9;
        }
      }
      n256 = ::n256;
      v30 = 4 * ::n256;
      if ( (uint32_t)n0x384 < 0x44C )
      {
        v30 = 6 * ::n256;
        if ( (uint32_t)n0x384 < 0x384 )
          v30 = 9 * ::n256;
      }
      _BitScanReverse(&v31, v8);
      v32 = v31 + 6;
      v33 = ((uint64_t)(uint32_t)((n2_1 + 4 * n4 + ((uint32_t)n0x384 >> 6))
                                            * (uint64_t)v30
                                            / (((uint32_t)(dword_100164A8 + 1935 - n0x384) >> 3) + 1)) << ((v32 & 0x1F) & 31))
          / v8;
      v34 = v70;
      v35 = dword_1000F058;
      do
      {
        v35 += 2;
        v36 = n4 + ((v33 * (uint64_t)(uint32_t)*v35) >> (v32 & 31));
        ::n256 += v36;
        v37 = v35[1];
        v38 = (uint32_t *)(8 * v37 + 268499040);
        buf__0[v37] -= (int32_t)v35;
        v39 = i_5[2 * v37];
        *v35 = v39;
        *v38 = v39 + v36;
        --v34;
      }
      while ( v34 );
    }
    if ( n255 >= 0xFF )
    {
      if ( dword_1000D4C4 )
      {
        v53 = (uint8_t *)dword_1001645C;
        v54 = v75;
        n255_3 = n255;
        v56 = 0;
        while ((uint8_t)(*v54) == (uint8_t)(*v53))
        {
          ++v53;
          ++v54;
          if ( !--n255_3 )
          {
            *a2 = -1;
            n255_4 = n255;
            goto LABEL_76;
          }
        }
        LOBYTE(v56) = *v54;
        *a2 = v56;
        n255_4 = n255 - n255_3;
LABEL_76:
        v77 = __sub_100061F4(::n256, (uint32_t *)p_n0x10000, n255_4, (uint32_t *)i_5);
      }
      else
      {
        v58 = __sub_100061F4(::n256, (uint32_t *)p_n0x10000, -1, (uint32_t *)i_5);
        v77 = v58;
        v59 = v75;
        n255_5 = n255;
        v61 = (uint8_t *)dword_1001645C;
        v62 = 0;
        while ( 1 )
        {
          LOBYTE(v62) = *v59;
          if ( --v58 < 0 )
            break;
          *v61++ = v62;
          ++v59;
          if ( !--n255_5 )
          {
            *a2 = -1;
            goto LABEL_82;
          }
        }
        *a2 = v62;
      }
    }
    else
    {
      n256_1 = ::n256;
      v41 = (uint32_t *)(8 * n255 + 268499048);
      v42 = 255 - n255;
      do
      {
        n256_1 -= *v41;
        v41 += 2;
        --v42;
      }
      while ( v42 );
      n256_2 = n256_1;
      if ( dword_1000D4C4 )
      {
        v43 = (uint8_t *)dword_1001645C;
        v44 = v75;
        n255_6 = n255;
        v46 = 0;
        while ((uint8_t)(*v44) == (uint8_t)(*v43))
        {
          ++v43;
          ++v44;
          if ( !--n255_6 )
          {
            *a2 = -1;
            n255_7 = n255;
            goto LABEL_64;
          }
        }
        LOBYTE(v46) = *v44;
        *a2 = v46;
        n255_7 = n255 - n255_6;
LABEL_64:
        v77 = __sub_100061F4(n256_2, (uint32_t *)p_n0x10000, n255_7, (uint32_t *)i_5);
      }
      else
      {
        v48 = __sub_100061F4(n256_1, (uint32_t *)p_n0x10000, -1, (uint32_t *)i_5);
        v77 = v48;
        v49 = v75;
        n255_8 = n255;
        v51 = (uint8_t *)dword_1001645C;
        v52 = 0;
        while ( 1 )
        {
          LOBYTE(v52) = *v49;
          if ( --v48 < 0 )
            break;
          *v51++ = v52;
          ++v49;
          if ( !--n255_8 )
          {
            *a2 = -1;
            goto LABEL_82;
          }
        }
        *a2 = v52;
      }
    }
LABEL_82:
    dword_100164A8 = (uint32_t)((v77 << 7) + 63 * dword_100164A8) >> 6;
    v63 = v70;
    if ( v70 )
    {
      v64 = dword_1000F060;
      ::n256 = n256;
      do
      {
        i_5[2 * v64[1]] = *v64;
        v64 += 2;
        --v63;
      }
      while ( v63 );
    }
    ::n256 -= i_5[0];
    ::n256 += v73;
    i_5[0] = v73;
    result = v77;
    LOWORD(i_5[2 * v77]) += 5;
    n362 = n362_0;
    n362_0 = ((uint16_t)n362_0 + 1) & 0xFFF;
    v66 = &buf_1[4 * n362];
    v67 = *(uint32_t *)v66;
    *(uint32_t *)v66 = v77;
    if ( v67 < 0 )
    {
      ::n256 += 5;
      return v77;
    }
    else
    {
      LOWORD(i_5[2 * v67]) -= 5;
    }
  }
  else
  {
    *a2 = -1;
    n4 = 0;
    dword_100164AC = 0;
    dword_100164A8 = 0;
    return 0;
  }
  return result;
}

// sub_100053E2 @ 0x100053E2 -- generated by tools/extract.py from vbn.exe.c lines 6034..6235
// Hex-Rays: int __stdcall sub_100053E2(int *dst_2)
// real convention: stdcall, ret 4

static __attribute__((stdcall)) int32_t __sub_100053E2(int32_t *dst_2)
{
  int32_t *v2; // esi
  int32_t n8; // edi
  int8_t v4; // al
  bool v5; // cf
  int8_t v6; // al
  int32_t dst; // edx
  int32_t dst_1; // ecx
  int8_t v9; // al
  int32_t v10; // ebx
  int8_t v11; // al
  int8_t v12; // al
  int32_t v13; // ebx
  int8_t v14; // al
  int8_t v15; // al
  int32_t v16; // ebx
  int8_t v17; // al
  bool v18; // cc
  uint32_t v19; // esi
  uint32_t v20; // ebx
  int32_t v21; // eax
  int32_t result; // eax
  int32_t v23; // edx
  int32_t v24; // ecx
  int32_t v25; // esi
  uint32_t v26; // ecx
  int32_t *v29; // [esp+Ch] [ebp-4h]

  v2 = dword_1000D850;
  n8 = n8_2;
  n8_0 = (int32_t)&dst_3[(uint32_t)n8_2 >> 3];
  v4 = *(uint8_t *)--n8_0;
  if ( n8_2 < 0 )
  {
LABEL_20:
    dst_1 = *dst_2;
    dst = (int32_t)dst_2;
    *v2 = (int32_t)dst_2;
  }
  else
  {
    while ( 1 )
    {
      v5 = __CFADD__(v4, v4);
      v6 = 2 * v4;
      dst = (int32_t)&dst_2[-v5 + 1];
      *v2 = dst;
      dst_1 = *(uint32_t *)dst;
      if ( (dst_1 < 0) ^ __OFADD__(v5, -v5) | (dst_1 == 0) )
      {
        --n8;
        goto LABEL_21;
      }
      ++v2;
      v5 = __CFADD__(v6, v6);
      v9 = 2 * v6;
      dst = dst_1 - 4 * v5 + 4;
      *v2 = dst;
      v10 = *(uint32_t *)dst;
      if ( (v10 < 0) ^ __OFADD__(v5, -v5) | (v10 == 0) )
      {
        n8 -= 2;
        dst_1 = *(uint32_t *)dst;
        goto LABEL_21;
      }
      ++v2;
      v5 = __CFADD__(v9, v9);
      v11 = 2 * v9;
      dst = v10 - 4 * v5 + 4;
      *v2 = dst;
      dst_1 = *(uint32_t *)dst;
      if ( (dst_1 < 0) ^ __OFADD__(v5, -v5) | (dst_1 == 0) )
      {
        n8 -= 3;
        goto LABEL_21;
      }
      ++v2;
      v5 = __CFADD__(v11, v11);
      v12 = 2 * v11;
      dst = dst_1 - 4 * v5 + 4;
      *v2 = dst;
      v13 = *(uint32_t *)dst;
      if ( (v13 < 0) ^ __OFADD__(v5, -v5) | (v13 == 0) )
      {
        n8 -= 4;
        dst_1 = *(uint32_t *)dst;
        goto LABEL_21;
      }
      ++v2;
      v5 = __CFADD__(v12, v12);
      v14 = 2 * v12;
      dst = v13 - 4 * v5 + 4;
      *v2 = dst;
      dst_1 = *(uint32_t *)dst;
      if ( (dst_1 < 0) ^ __OFADD__(v5, -v5) | (dst_1 == 0) )
      {
        n8 -= 5;
        goto LABEL_21;
      }
      ++v2;
      v5 = __CFADD__(v14, v14);
      v15 = 2 * v14;
      dst = dst_1 - 4 * v5 + 4;
      *v2 = dst;
      v16 = *(uint32_t *)dst;
      if ( (v16 < 0) ^ __OFADD__(v5, -v5) | (v16 == 0) )
      {
        n8 -= 6;
        dst_1 = *(uint32_t *)dst;
        goto LABEL_21;
      }
      ++v2;
      v5 = __CFADD__(v15, v15);
      v17 = 2 * v15;
      dst = v16 - 4 * v5 + 4;
      *v2 = dst;
      dst_1 = *(uint32_t *)dst;
      if ( (dst_1 < 0) ^ __OFADD__(v5, -v5) | (dst_1 == 0) )
      {
        n8 -= 7;
        goto LABEL_21;
      }
      ++v2;
      dst = dst_1 - 4 * __CFADD__(v17, v17) + 4;
      *v2 = dst;
      dst_2 = *(int32_t **)dst;
      if ( ((int32_t)dst_2 < 0) ^ __OFADD__(__CFADD__(v17, v17), -__CFADD__(v17, v17)) | (dst_2 == nullptr) )
        break;
      ++v2;
      v4 = *(uint8_t *)--n8_0;
      v18 = n8 <= 8;
      n8 -= 8;
      if ( v18 )
        goto LABEL_20;
    }
    n8 -= 8;
    dst_1 = *(uint32_t *)dst;
  }
LABEL_21:
  dst_0 = dst;
  if ( dst_1 == *(uint32_t *)(dst ^ 4) && v2 != dword_1000D850 )
    --v2;
  v29 = v2;
  v19 = ((uint32_t)((int8_t *)v2 - (int8_t *)dword_1000D850) >> 2) + 1;
  dword_10016454 = v19;
  n8_0 = n8;
  ::dst_1 = dst_1;
  n4 = 0;
  if ( (int32_t)v19 > dword_10016438 )
  {
    _BitScanReverse(&v20, dword_1000CA4C + 7 * dword_10016434);
    n4 = 16 * (v19 - dword_10016438) / v20;
    dword_10016438 = ((v19 >> 3) + v19 + 2 * dword_10016438) >> 2;
    dst_1 = ::dst_1;
  }
  if ( dst_1 <= 0
    && dword_10016438 < (uint32_t)dword_10016454
    && (uint8_t)dst_3[(uint32_t)-dst_1 >> 3] != dword_10016488 )
  {
    dword_1000D55C[0] = dst_1;
    v21 = (dword_10016454 - dword_10016438) << (n4_0-- & 31);
    ++n2_1;
    dword_1000D558[0] = v21;
  }
  while ( 1 )
  {
    result = dword_10016454;
    if ( dword_10016454 <= (uint32_t)dword_10016438 )
      break;
    v23 = *(uint32_t *)(*v29-- ^ 4);
    if ( v23 > 0 )
    {
      v25 = dword_10016454 - dword_10016438;
      v26 = n4_0;
      if ( n4_0 )
      {
        --n4_0;
        v25 <<= (v26 & 31) >> 1;
      }
      {   // the walk returns its cursor in ECX, which Hex-Rays could not see (v27)
        VbnWalk w = { (uint8_t *)(8 * n2_1 + 268490072), n55 - n2_1, (int32_t)(uintptr_t)dst_3, (int32_t)v25 };
        vbn_walk2_entry((int32_t *)(uintptr_t)v23, &w);
        n2_1 = (uint32_t)((uintptr_t)w.out - (uintptr_t)dword_1000D558) >> 3;
      }
      --dword_10016454;
    }
    else
    {
      if ( (uint8_t)dst_3[(uint32_t)-v23 >> 3] != (uint8_t)(dword_10016488))
      {
        v24 = n2_1;
        dword_1000D558[2 * n2_1] = (dword_10016454 - dword_10016438) << (n4_0 & 31);
        dword_1000D55C[2 * v24] = v23;
        n2_1 = v24 + 1;
        if ( v24 + 1 == n55 )
          __pe_sub_10006E22();
      }
      --dword_10016454;
    }
  }
  return result;
}

// sub_10005989 @ 0x10005989 -- generated by tools/extract.py from vbn.exe.c lines 6423..6622
// Hex-Rays: int __stdcall sub_10005989(int *dst_2)
// real convention: stdcall, ret 4

static __attribute__((stdcall)) int32_t __sub_10005989(int32_t *dst_2)
{
  int32_t *v2; // esi
  int32_t n8; // edi
  int8_t v4; // al
  bool v5; // cf
  int8_t v6; // al
  int32_t dst; // edx
  int32_t dst_1; // ecx
  int8_t v9; // al
  int32_t v10; // ebx
  int8_t v11; // al
  int8_t v12; // al
  int32_t v13; // ebx
  int8_t v14; // al
  int8_t v15; // al
  int32_t v16; // ebx
  int8_t v17; // al
  bool v18; // cc
  uint32_t v19; // esi
  uint32_t v20; // ebx
  int32_t v21; // eax
  int32_t result; // eax
  uint32_t **v23; // edx
  int32_t v24; // ecx
  int32_t v25; // esi
  uint32_t v26; // ecx
  int32_t *v30; // [esp+Ch] [ebp-4h]

  v2 = dword_1000D850;
  n8 = n8_2;
  n8_0 = (int32_t)&dst_3[(uint32_t)n8_2 >> 3];
  v4 = *(uint8_t *)--n8_0;
  if ( n8_2 )
  {
    while ( 1 )
    {
      v5 = __CFADD__(v4, v4);
      v6 = 2 * v4;
      dst = (int32_t)&dst_2[-v5 + 1];
      *v2 = dst;
      dst_1 = *(uint32_t *)dst;
      if ( (dst_1 < 0) ^ __OFADD__(v5, -v5) | (dst_1 == 0) )
      {
        --n8;
        goto LABEL_20;
      }
      ++v2;
      v5 = __CFADD__(v6, v6);
      v9 = 2 * v6;
      dst = dst_1 - 4 * v5 + 4;
      *v2 = dst;
      v10 = *(uint32_t *)dst;
      if ( (v10 < 0) ^ __OFADD__(v5, -v5) | (v10 == 0) )
      {
        n8 -= 2;
        dst_1 = *(uint32_t *)dst;
        goto LABEL_20;
      }
      ++v2;
      v5 = __CFADD__(v9, v9);
      v11 = 2 * v9;
      dst = v10 - 4 * v5 + 4;
      *v2 = dst;
      dst_1 = *(uint32_t *)dst;
      if ( (dst_1 < 0) ^ __OFADD__(v5, -v5) | (dst_1 == 0) )
      {
        n8 -= 3;
        goto LABEL_20;
      }
      ++v2;
      v5 = __CFADD__(v11, v11);
      v12 = 2 * v11;
      dst = dst_1 - 4 * v5 + 4;
      *v2 = dst;
      v13 = *(uint32_t *)dst;
      if ( (v13 < 0) ^ __OFADD__(v5, -v5) | (v13 == 0) )
      {
        n8 -= 4;
        dst_1 = *(uint32_t *)dst;
        goto LABEL_20;
      }
      ++v2;
      v5 = __CFADD__(v12, v12);
      v14 = 2 * v12;
      dst = v13 - 4 * v5 + 4;
      *v2 = dst;
      dst_1 = *(uint32_t *)dst;
      if ( (dst_1 < 0) ^ __OFADD__(v5, -v5) | (dst_1 == 0) )
      {
        n8 -= 5;
        goto LABEL_20;
      }
      ++v2;
      v5 = __CFADD__(v14, v14);
      v15 = 2 * v14;
      dst = dst_1 - 4 * v5 + 4;
      *v2 = dst;
      v16 = *(uint32_t *)dst;
      if ( (v16 < 0) ^ __OFADD__(v5, -v5) | (v16 == 0) )
      {
        n8 -= 6;
        dst_1 = *(uint32_t *)dst;
        goto LABEL_20;
      }
      ++v2;
      v5 = __CFADD__(v15, v15);
      v17 = 2 * v15;
      dst = v16 - 4 * v5 + 4;
      *v2 = dst;
      dst_1 = *(uint32_t *)dst;
      if ( (dst_1 < 0) ^ __OFADD__(v5, -v5) | (dst_1 == 0) )
      {
        n8 -= 7;
        goto LABEL_20;
      }
      ++v2;
      dst = dst_1 - 4 * __CFADD__(v17, v17) + 4;
      *v2 = dst;
      dst_2 = *(int32_t **)dst;
      if ( ((int32_t)dst_2 < 0) ^ __OFADD__(__CFADD__(v17, v17), -__CFADD__(v17, v17)) | (dst_2 == nullptr) )
        break;
      ++v2;
      v4 = *(uint8_t *)--n8_0;
      v18 = n8 <= 8;
      n8 -= 8;
      if ( v18 )
        goto LABEL_11;
    }
    n8 -= 8;
    dst_1 = *(uint32_t *)dst;
  }
  else
  {
LABEL_11:
    dst_1 = *dst_2;
    dst = (int32_t)dst_2;
    *v2 = (int32_t)dst_2;
  }
LABEL_20:
  dst_0 = dst;
  if ( dst_1 == *(uint32_t *)(dst ^ 4) && v2 != dword_1000D850 )
    --v2;
  v30 = v2;
  v19 = ((uint32_t)((int8_t *)v2 - (int8_t *)dword_1000D850) >> 2) + 1;
  dword_10016454 = v19;
  n8_0 = n8;
  ::dst_1 = dst_1;
  n4 = 0;
  if ( (uint32_t)n0xFFE > 0xFFE )
    __pe_sub_10006E22();
  if ( (int32_t)v19 > dword_10016438 )
  {
    _BitScanReverse(&v20, dword_1000CA4C + 7 * dword_10016434);
    n4 = 16 * (v19 - dword_10016438) / v20;
    dword_10016438 = ((v19 >> 3) + v19 + 2 * dword_10016438) >> 2;
    dst_1 = ::dst_1;
  }
  if ( dst_1 <= 0 && dword_10016438 < (uint32_t)dword_10016454 )
  {
    dword_1000D55C[0] = dst_1;
    v21 = (dword_10016454 - dword_10016438) << (n4_0-- & 31);
    ++n2_1;
    dword_1000D558[0] = v21;
  }
  while ( 1 )
  {
    result = dword_10016454;
    if ( dword_10016454 <= (uint32_t)dword_10016438 )
      break;
    v23 = *(uint32_t ***)(*v30-- ^ 4);
    if ( (int32_t)v23 > 0 )
    {
      v25 = dword_10016454 - dword_10016438;
      v26 = n4_0;
      if ( n4_0 )
      {
        --n4_0;
        v25 <<= (v26 & 31) >> 1;
      }
      {   // the walk returns its cursor in ECX, which Hex-Rays could not see (v27)
        VbnWalk w = { (uint8_t *)(8 * n2_1 + 268490072), n55 - n2_1, 4, (int32_t)v25 };
        vbn_walk1_entry((int32_t *)(uintptr_t)v23, &w);
        n2_1 = (uint32_t)((uintptr_t)w.out - (uintptr_t)dword_1000D558) >> 3;
      }
      --dword_10016454;
    }
    else
    {
      v24 = n2_1;
      dword_1000D558[2 * n2_1] = (dword_10016454 - dword_10016438) << (n4_0 & 31);
      dword_1000D55C[2 * v24] = (int32_t)v23;
      n2_1 = v24 + 1;
      if ( v24 + 1 == n55 )
        __pe_sub_10006E22();
      --dword_10016454;
    }
  }
  return result;
}

// sub_10000D0E @ 0x10000D0E -- read the coder's input.  In vbn this was the
// input reader: the data (the archive's title, then each file) when
// compressing, the archive's volumes when decompressing.  vbn2 reads one file:
// the data when compressing, the code after the stream's size when
// decompressing.  A short read leaves the rest of the buffer as it was, as
// vbn's did at the end of its data.
static FILE *vbn_in, *vbn_out;
static const char *vbn_in_name, *vbn_out_name;
static int32_t vbn_compressing;
static uint32_t vbn_out_left;           // decompressing: the bytes of data still to write
[[noreturn]] static void vbn_fail(const char *what, const char *name);

static uint32_t __sub_10000D0E(int8_t *dst, uint32_t count)
{
  uint32_t n = fread(dst, 1, count, vbn_in);
  if (n < count && ferror(vbn_in))
    vbn_fail("cannot read", vbn_in_name);
  return (uint32_t)n;
}

// sub_100012A8 @ 0x100012A8 -- write the coder's output: the code when
// compressing, the data when decompressing.  vbn's writer split it into
// archive volumes, or into the title and the files; it stopped at the end of
// the last file, so the data written never runs past the stream's size.
static int8_t __sub_100012A8(int8_t *src, uint32_t count)
{
  if (!vbn_compressing) {
    if (count > vbn_out_left)
      count = vbn_out_left;
    vbn_out_left -= count;
  }
  if (fwrite(src, 1, count, vbn_out) != count)
    vbn_fail("cannot write", vbn_out_name);
  return 0;
}

// sub_10004924 @ 0x10004924 -- generated by tools/extract.py from vbn.exe.c lines 5095..5123
// Hex-Rays: int __usercall sub_10004924@<eax>( int a1@<edx>, int count@<ebx>, char *p_j@<edi>, char *_Data_size____10u_n@<esi>, int n0x10000)
// real convention: cdecl; register parameters dropped: int a1, int count, char *p_j, char *_Data_size____10u_n


static int32_t __sub_10004924(int32_t n0x10000)
{

  if ( dword_1000D4C4 )
  {
    __sub_100012A8((int8_t *)aVbnVer1001, // "                                                \nvbn ver.1...."
      0x2000u);
    __sub_100048F0(4, (void *)src_3, (void *)aVbnVer1001);        // "                                                \nvbn ver.1...."
    __sub_1000490B(2048, (void *)&aVbnVer1001[16]);       // "                                                \nvbn ver.1...."
  }
  else
  {
    __sub_100048F0(4, (void *)src_3, (void *)aVbnVer1001);        // "                                                \nvbn ver.1...."
    __sub_10000D0E((int8_t *)&aVbnVer1001[16], 0x2000u);// "                                                \nvbn ver.1...."
  }
  return n0x10000 - 0x10000;
}

// sub_100056C4 @ 0x100056C4 -- generated by tools/extract.py from vbn.exe.c lines 6256..6393
// Hex-Rays: unsigned int __userpurge sub_100056C4@<eax>( int dst@<ebx>, char *p_j@<edi>, int *_Data_size____10u_n@<esi>, unsigned int *a4, int n0x198000, unsigned int n8)
// real convention: stdcall, ret 12; register parameters dropped: int dst, char *p_j, int *_Data_size____10u_n


static __attribute__((stdcall)) uint32_t __sub_100056C4(uint32_t *a4, int32_t n0x198000, uint32_t n8)
{
  int32_t dst = 0;   // @<ebx> in Hex-Rays; regflow shows no caller sets it
  int32_t * _Data_size____10u_n = 0;   // @<esi> in Hex-Rays; regflow shows no caller sets it
  alignas(16) uint8_t __frame[36];   // the original frame, ebp-0x14..ebp+0x0
  uint32_t &p_n0x10000 = *(uint32_t *)(__frame + 0);
  uint32_t &v18 = *(uint32_t *)(__frame + 4);
  int32_t &v19 = *(int32_t *)(__frame + 8);
  uint32_t &n8_1 = *(uint32_t *)(__frame + 12);
  int32_t &v21 = *(int32_t *)(__frame + 16);
  uint32_t v7; // eax
  int32_t v8; // ecx
  int32_t v9; // eax
  int8_t v10; // al
  uint32_t v11; // ecx
  uint32_t v12; // eax
  int32_t v13; // ecx
  int8_t v14; // al
  int32_t v15; // edx

  n8_1 = n8;
  p_n0x10000 = *a4;
  v19 = 0;
  v21 = -1;
  while ( n8 < n13369344 )
  {
    dword_10016488 = v21;
    n8_2 = n8;
    dword_10016454 = 0;
    n2_1 = 0;
    dword_100164C4 = 0;
    n4_0 = 4;
    _BitScanReverse(&v7, dword_10016434 + 4);
    dword_10016438 = (276 * v7) >> 9;
    if ( v21 < 0 )
    {
      vbn_restartable(__sub_10005989, (int32_t)(uintptr_t)::dst);   // sub_1000703B(0); sub_10005989(dst): see vbn_restartable
    }
    else
    {
      vbn_restartable(__sub_100053E2, (int32_t)(uintptr_t)::dst);   // sub_1000703B(0); sub_100053E2(dst): see vbn_restartable
    }
    if ( n2_1 > 1 )
      dword_100164C4 = (uint32_t)dword_1000D558[2 * n2_1 - 2] >> 2;
    if ( (uint32_t)n2_1 >= 2 )
    {
      _Data_size____10u_n = dword_1000D560;
      v8 = n2_1 - 1;
      v9 = dword_1000D558[0];
      do
      {
        if ( v9 != *_Data_size____10u_n )
          break;
        dst = dword_1000D55C[0];
        if ( dword_1000D55C[0] >= _Data_size____10u_n[1] )
        {
          dst = _InterlockedExchange(_Data_size____10u_n + 1, dword_1000D55C[0]);
          dword_1000D55C[0] = dst;
        }
        _Data_size____10u_n += 2;
        --v8;
      }
      while ( v8 );
    }
    v18 = __sub_10005C50((uint32_t *)&p_n0x10000, (uint32_t *)&v21, n8);
    if ( v18 )
    {
      v14 = dst_3[n8 >> 3];
      buf_3[dword_100164B0] = v14;
      buf_4[dword_100164B4] = v14;
      if ( v21 >= 0 || (uint32_t)n4 < 0x2A )
        __sub_10005251(v18, n8);
      v19 += v18;
      n8 += 8 * v18;
    }
    else
    {
      v10 = __sub_100064C0(v21, (uint32_t *)&p_n0x10000, n8);
      buf_3[dword_100164B0] = v10;
      buf_4[dword_100164B4] = v10;
      dst = dst_0;
      _BitScanReverse(&v11, dword_1000CA4C + 32);
      if ( n8 - n8_0 > v11 << 6 )
      {
        if ( dst_1 <= 0 && dst_1 + n8 >= 0x800 )
        {
          _bittestandcomplement(&buf_, n8 >> 3);
          *(uint32_t *)dst_0 = -8 * (n8 >> 3);
          ++dword_10016434;
        }
      }
      else if ( dst_1 <= 0 )
      {
        _bittestandcomplement(&buf_, n8 >> 3);
        v12 = -8 * (n8 >> 3);
        v13 = dword_10016430;
        *(uint32_t *)dword_10016430 = v12;
        *(uint32_t *)(v13 + 4) = v12;
        *(uint32_t *)dst = v13;
        dword_10016430 += 8;
        ++dword_1000CA4C;
        dst ^= 4u;
        if ( *(int32_t *)dst > 0 )
          ++dword_10016434;
      }
      v21 = -1;
      if ( n8 < n13369344 )
      {
        n8 += 8;
        ++v19;
      }
    }
    v15 = (uint8_t)byte_100166EF[n8 >> 3];
    dword_100164B0 = v15;
    if ( n8 >> 3 != 1 )
      v15 = (uint8_t)byte_100166EF[(n8 >> 3) - 1];
    dword_100164B4 = v15;
    if ( p_n0x10000 >= 0x10000 )
    {
      p_n0x10000 = __sub_10004924(p_n0x10000);
    }
  }
  *a4 = p_n0x10000;
  ::n8_1 += 8 * v19;
  return n8;
}

// sub_100050F8 @ 0x100050F8 -- hand-written (incdec.md §4.1): re-register the
// context tree's positions after the input window slides (sub_10004A5F).
// Hex-Rays prints the eight-way unrolled descent as carry/overflow arithmetic:
// per bit, `add cl,cl; sbb r,r; adc r,[node+r*4+4]; jle`.  sbb leaves CF (the
// bit) alone, so adc loads [node] for a 1 bit and [node+4] for a 0 bit, carries
// the bit back out and never overflows: jle is a plain `child <= 0`.  It is the
// descent of sub_10005251:
//
// For every position pos = 8, 16, ... below n8 whose bit pos/8 is set in the
// bitmap buf_:
//  - descend from the root `dst` reading the data bits backwards from byte
//    pos/8-1 (bit 1 -> child [0], bit 0 -> child [1]) to the first slot
//    holding a value <= 0, or to node[0] of the last node when the bits run out;
//  - depth = pos - (bit position where the descent stopped); then
//      depth <= 64*bsr(nodes+32):  an empty leaf becomes a new node from the
//          allocator at dword_10016430, both children -pos; the sibling slot
//          being a node bumps dword_10016434;
//      deeper: a leaf older than 0x800 is refreshed to -pos, and bumps it too.
// Returns the first position >= n8 (EAX); cdecl, no arguments.

static uint32_t __sub_100050F8(void)
{
  uint32_t pos = 0;                                     // EAX
  for (;;) {
    pos += 8;
    if (pos >= n8)
      return pos;
    if (!_bittest(((const void *)&buf_), (int32_t)(pos >> 3)))
      continue;
    const uint8_t *p = ((const uint8_t *)dst_3) + (pos >> 3) - 1;     // EDI
    int32_t at = (int32_t)pos;                          // ESI
    int32_t *node = ((int32_t *)dst), *slot;                  // EBX/EDX alternate in the asm
    int32_t leaf;
    uint8_t cl = *p;
    for (;;) {
      int32_t k, found = 0;
      for (k = 1; k <= 8; k++) {
        int32_t bit = (cl >> 7) & 1;
        cl = (uint8_t)(cl << 1);
        int32_t *s = node + (bit ? 0 : 1);
        int32_t child = *s;
        if (child <= 0) {
          slot = s;
          leaf = child;
          at -= k;
          found = 1;
          break;
        }
        node = (int32_t *)(uintptr_t)child;
      }
      if (found)
        break;
      cl = *--p;
      at -= 8;
      if (at <= 0) {
        leaf = node[0];
        slot = node;
        break;
      }
    }
    uint32_t depth = pos - (uint32_t)at;
    uint32_t thr = (uint32_t)(31 - __builtin_clz((uint32_t)dword_1000CA4C + 32)) << 6;
    if (depth > thr) {
      if (leaf <= 0 && pos + (uint32_t)leaf >= 0x800) {
        *slot = -(int32_t)pos;
        dword_10016434++;
      }
    } else if (leaf <= 0) {
      int32_t *n = (int32_t *)(uintptr_t)dword_10016430;
      n[0] = -(int32_t)pos;
      n[1] = -(int32_t)pos;
      *slot = (int32_t)(uintptr_t)n;
      dword_10016430 += 8;
      dword_1000CA4C++;
      slot = (int32_t *)((uintptr_t)slot ^ 4);
      if (*slot > 0)
        dword_10016434++;
    }
  }
}

// sub_10004A5F @ 0x10004A5F -- generated by tools/extract.py from vbn.exe.c lines 5182..5421
// Hex-Rays: unsigned int __usercall sub_10004A5F@<eax>( int k@<ecx>, int dst@<ebx>, char *p_j@<edi>, int *_Data_size____10u_n@<esi>, unsigned int n0x198000)
// real convention: cdecl; register parameters dropped: int k, int dst, char *p_j, int *_Data_size____10u_n


static void __sub_10004A5F(uint32_t n0x198000)
{
  alignas(16) uint8_t __frame[148];   // the original frame, ebp-0x84..ebp+0x0
  int32_t &_Code_size___u__Ratio____u___or__u__05d_bpc___1 = *(int32_t *)(__frame + 16);
  uint32_t &i = *(uint32_t *)(__frame + 20);
  uint32_t (&p_ja)[23] = *(uint32_t (*)[23])(__frame + 28);
  int32_t &j = *(int32_t *)(__frame + 120);
  uint32_t v12; // ecx

  buf = (int8_t *)i_4;
  buf_0 = (int8_t *)buf_6;
  buf_1 = (int8_t *)buf_7;
  {
    n2048 = 2048;
    for ( i = 0; i < 0x100; ++i )
    {
      *(&i_ + 2 * i) = 8;
      dword_10010464[2 * i] = i;
      dword_10010060[i] = 8 * i + 268502112;
    }
    n256 = 256;
    for ( i = 0; i < 0x100; ++i )
    {
      i_5[2 * i] = 1;
      dword_1000F864[2 * i] = i;
    }
    __sub_100091DC(buf, 0xFFu, 0x5ACu);
    __sub_100091DC(buf_0, 0xFFu, 0x1000u);
    __sub_100091DC(buf_1, 0xFFu, 0x4000u);
    n362_0 = __sub_1000490B(0x4000, (void *)buf_2);
    n362_1 = n362_0;
    n362 = n362_0;
    qmemcpy(
      p_ja,
      Data_size____10u_n,                       // "f\n"
      sizeof(p_ja));                            // " Data size= %-10u\n"
    for ( j = 0; j < 23; ++j )
    {
      for ( i = p_ja[j]; i; --i )
        __sub_100050B0(j);
    }
    dword_10016410 = -1;
    n0x7FFFFFFF = __sub_1000490B(256, (void *)buf__0);
    n0x7FFFFFFF_1 = n0x7FFFFFFF;
    n8_1 = n0x7FFFFFFF;
    n0x384 = 1280;
    n0xFFE = 3072;
    n0x2000 = 7168;
    n0x4000 = 0x4000;
    dword_100164A8 = 0;
    if ( n0x198000 <= 0x198000 )
    {
      if ( dword_1000D4C4 || dword_1000CA4C )
      {
        n13369344 = 8 * n0x198000;
        dst_4 = (int32_t)&::dst_3[n0x198000];
      }
      else
      {
        n13369344 = 8 * (n0x198000 + 256);
        dst_4 = (int32_t)dword_100167F0 + n0x198000;
      }
    }
    else
    {
      n13369344 = 13369344;
      dst_4 = (int32_t)::dst;
    }
    if ( dword_1000D4C4 )
    {
      __sub_10000D0E((int8_t *)// " Data size= %-10u\n"
        ::dst_3, (uint32_t)n13369344 >> 3);
      __sub_1000490B(2052, (void *)aVbnVer1001);          // "                                                \nvbn ver.1...."
    }
    else if ( (uint32_t)n13369344 >> 2 >= 0x2010 )
    {
      __sub_10000D0E((int8_t *)// " Data size= %-10u\n"
        aVbnVer1001, // "                                                \nvbn ver.1...."
        0x2010u);
    }
    else
    {
      __sub_10000D0E((int8_t *)// " Data size= %-10u\n"
        aVbnVer1001, // "                                                \nvbn ver.1...."
        (uint32_t)n13369344 >> 2);
    }
    __sub_1000490B(52224, (void *)&buf_);
    dword_101AE6F4 = 0;
    ::dst[0] = 0;
    n8 = 0;
    dword_10016430 = (int32_t)dword_101AE6F8;
    dword_1000CA4C = 1;
    dword_10016494 = 1;
    dword_10016434 = 1;
    n2_0 = 8;
    dword_100164B0 = __sub_1000490B(64, (void *)buf_3);
    dword_100164B4 = dword_100164B0;
    dword_100164AC = dword_100164B0;
    dword_100164B8 = dword_100164B0;
    __sub_1000490B(64, (void *)buf_4);
  }
  n8_3 = n8;
  src_4 = &::dst_3[n8 >> 3];
  _Code_size___u__Ratio____u___or__u__05d_bpc___1 = 0;
  while ( 1 )
  {
    if ( n8 != n13369344 )
      n8 = __sub_100056C4((uint32_t *)(uint32_t *)&_Code_size___u__Ratio____u___or__u__05d_bpc___1, n0x198000, n8);
    if ( (uint32_t)n8_1 >> 3 >= n0x198000 )
      break;
    if ( !dword_1000D4C4 )
    {
      if ( src_4 == ::dst_3 )
      {
        __sub_100012A8((int8_t *)src_4, 0x66000u);
      }
      else if ( src_4 < (int8_t *)src_5 )
      {
        __sub_100012A8((int8_t *)src_4, (int8_t *)src_5 - src_4);
      }
    }
    src_4 -= 417792;
    if ( src_4 < ::dst_3 || (int32_t)src_4 < 0 )
      src_4 = ::dst_3;
    __sub_100048F0(313344, (void *)src_5, (void *)::dst_3);
    __sub_100048F0(39168, (void *)src_6, (void *)&buf_);
    __sub_1000490B(13056, (void *)buf_5);
    dword_101AE6F4 = 0;
    ::dst[0] = 0;
    dword_10016430 = (int32_t)dword_101AE6F8;
    dword_1000CA4C = 1;
    n8 = 10027008;
    __sub_100050F8();
    if ( dword_1000D4C4 )
      __sub_10000D0E((int8_t *)dst_5, 0x66000u);
    v12 = (uint32_t)n8_1 >> 3;
    if ( n0x198000 - ((uint32_t)n8_1 >> 3) < 0x66000 )
    {
      n13369344 = n0x198000 + 1253376 - v12;
      dst_4 = (int32_t)&::dst_3[n0x198000 + 1253376 - v12];
      n13369344 *= 8;
    }
  }
  _Code_size___u__Ratio____u___or__u__05d_bpc___1 = __sub_10004993(_Code_size___u__Ratio____u___or__u__05d_bpc___1);
  if ( dword_1000D4C4 )
    __sub_100012A8((int8_t *)aVbnVer1001, // "                                                \nvbn ver.1...."
      ((_Code_size___u__Ratio____u___or__u__05d_bpc___1 & 7) != 0)
    + ((uint32_t)_Code_size___u__Ratio____u___or__u__05d_bpc___1 >> 3));
  else
    __sub_100012A8((int8_t *)src_4, &::dst_3[n8 >> 3] - src_4);
}

// ===========================================================================
// main: vbn2 c|d input output.  A compressed stream is the input's size (4
// bytes, little-endian) and then vbn's code for it, in vbn's MAX mode (`u`):
// the archive around it -- header, title, volumes -- is gone.  The coder is
// vbn's, run once, as vbn ran it for an archive; it runs on a stack inside
// vbn's memory, as in the port, so every address it handles is an image
// address.
// ===========================================================================
static const uint32_t STACK_TOP = 0x13D00000, STACK_SIZE = 0x500000;   // down to the heap's end
static const uint32_t MAX_SIZE = 0x1FFFFFFF;    // the coder counts the data in bits, in 32 bits

static uint32_t vbn_size;
static void vbn_main()
{
  __sub_10004A5F(vbn_size);
}

static void vbn_fail(const char *what, const char *name)
{
  fprintf(stderr, "vbn2: %s %s%s%s\n", what, name, errno ? ": " : "", errno ? strerror(errno) : "");
  if (vbn_out) {
    fclose(vbn_out);
    remove(vbn_out_name);
  }
  exit(1);
}

int main(int argc, char **argv)
{
  if (argc != 4 || (strcmp(argv[1], "c") && strcmp(argv[1], "d"))) {
    fprintf(stderr, "vbn2: VBN 1.001's compressor (MAX mode) on one file\n"
                    "Usage: vbn2 c input output   compress\n"
                    "       vbn2 d input output   decompress\n");
    return 1;
  }
  vbn_compressing = argv[1][0] == 'c';
  vbn_in_name = argv[2];
  vbn_out_name = argv[3];
  struct stat in_st, out_st;
  vbn_in = fopen(vbn_in_name, "rb");
  if (!vbn_in || fstat(fileno(vbn_in), &in_st) < 0)
    vbn_fail("cannot open", vbn_in_name);
  if (stat(vbn_out_name, &out_st) == 0 && out_st.st_dev == in_st.st_dev && out_st.st_ino == in_st.st_ino) {
    errno = 0;
    vbn_fail("input and output are the same file:", vbn_in_name);
  }

  uint8_t header[4];
  if (vbn_compressing) {
    if (!S_ISREG(in_st.st_mode) || in_st.st_size > (int64_t)MAX_SIZE) {
      errno = 0;
      vbn_fail(S_ISREG(in_st.st_mode) ? "input is too large (512 MB at most):" : "not a regular file:", vbn_in_name);
    }
    vbn_size = (uint32_t)in_st.st_size;
    for (int32_t i = 0; i < 4; i++)
      header[i] = (uint8_t)(vbn_size >> 8 * i);
  } else {
    errno = 0;
    if (fread(header, 1, 4, vbn_in) != 4)
      vbn_fail(ferror(vbn_in) ? "cannot read" : "not a vbn2 stream (too short):", vbn_in_name);
    vbn_size = header[0] | header[1] << 8 | header[2] << 16 | (uint32_t)header[3] << 24;
    if (vbn_size > MAX_SIZE)
      vbn_fail("not a vbn2 stream (bad size):", vbn_in_name);
    vbn_out_left = vbn_size;
  }
  vbn_out = fopen(vbn_out_name, "wb");
  if (!vbn_out)
    vbn_fail("cannot create", vbn_out_name);
  if (vbn_compressing && fwrite(header, 1, 4, vbn_out) != 4)
    vbn_fail("cannot write", vbn_out_name);

  image_map();
  dword_1000D4C4 = vbn_compressing;              // the direction
  n55 = 95;                                      // MAX mode (`u`; `b` is 55, `B` 31)
  if (!vbn_compressing)
    dword_1000CA4C = 1;                          // as in vbn's second run (see sub_10004A5F)

  ucontext_t back, run;
  getcontext(&run);
  run.uc_stack.ss_sp = (void *)(uintptr_t)(STACK_TOP - STACK_SIZE);
  run.uc_stack.ss_size = STACK_SIZE;
  run.uc_link = &back;
  makecontext(&run, vbn_main, 0);
  if (swapcontext(&back, &run) < 0)
    vbn_fail("cannot run", "the coder");

  errno = 0;
  if (!vbn_compressing && vbn_out_left)
    vbn_fail("not a vbn2 stream (data missing):", vbn_in_name);
  if (fclose(vbn_out) != 0) {
    vbn_out = NULL;                               // closed, if not written
    remove(vbn_out_name);
    vbn_fail("cannot write", vbn_out_name);
  }
  fclose(vbn_in);
  return 0;
}

// vbn's initialized data, 0x1000A08C..0x1000D400 (vbn.exe's .vbn section, from file
// offset 0xA88C): its messages and tables, and the C runtime's variables.
const uint8_t image_data[] =
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\0004\000"
  "\000\000\000\360\000\000\020\003\020\000\015\012NULL code pointer called\015\012$\015\012Not eno"
  "ugh memory\015\012$\015\012Bad stack size parameter\015\012$\015\012Stack Overflow\015\012$\000"
  "\000\000                                                \012vbn ver.1.001 (freeeware)           "
  "                                       \012Usage: vbn <command> <[path\\]archive_name>[\"context"
  "_name] [<[path\\]mask>] [...]\012Command:\012   b       -Backup files to archive with recurse su"
  "bdirectories\012Note: command \"b\" has 3 modifications : \"B\" FAST, \"b\" NORMAL, \"u\" MAX.\012"
  "   bf      -Backup only files to archive without recurse subdirectories\012bN | bfN   -Where N i"
  "s size (Kb) of Volumes (enable multiple Volumes)\012   r[PSW]  -Restore files from archive\012  "
  " l[PSW]  -List archive contents    Note: PSW -password (len<17) can be absent\012   p[PSW]  -Err"
  "or Protection  *.vbn->*.acp\012   c[PSW]  -Error Correction\012   d[PSW]  -Deprotection of archi"
  "ve  *.acp->*.vbn\012EXAMPLES:\012vbn b win c:\\win  -packing of the catalogue WIN with subdirect"
  "ories\012vbn B360 d:\\awin c:\\win\\*.* -FAST packing in multivolume archive (360 Kb.)\012vbn r "
  "d:\\awin e:\\win3    -in catalogue WIN3 copy c:\\win\\*.* with subdirectories\012vbn uf53pKUK wr"
  "c c:\\win\\*.rc -MAX.packing of only files *.rc from catalogue WIN\012in multivolume archive (53"
  "Kb) with setting of protection mode with password=KUK\012vbn lKUK  wrc -contents of wrc.\?\?p   "
  " Note: After r-operation wrc.\?\?p->wrc.\?\?b\012vbn rKUK  wrc -restore all data from wrc.\?\?p "
  "in the current catalogue\012vbn pALmA wrc -setting of protection mode with password=ALmA  wrc.\?"
  "\?b->wrc.\?\?p\012vbn cALmA wrc -testing and correcting of archive wrc.\?\?p\012vbn dALmA wrc -d"
  "eprotected wrc.\?\?p->wrc.\?\?b\012                   Press any key to continue ......\000"
  "\012                        \"TRAINABLE CHANNEL\" - MODE:\012\"TRAINABLE CHANNEL\" - compression"
  " of another portion of information taking into\012account all earlier compressed information -co"
  "ntext. For example: you transmit\012by modem a corrected version of the text transmitted yesterd"
  "ay. In this case\012it's possible to expect 100-time compression, in other words, only the lates"
  "t\012information was transmitted. Besides, there's a possibility to maximize\012compression of t"
  "he data of a certain type. For example, create a context from\012the files *.txt and use this tx"
  "t-context to compress txt-files. In addition,\012such an approach ensures additional data protec"
  "tion against non-authorised\012access -it's impossible in principle without knowledge of the who"
  "le background\012data being decoded. In unpacking the context should be the same as in packing.\012"
  "The context file is renewed each time it is used. For example: you packed the\012text of an agre"
  "ement using the context, the context file was renewed and can't\012be used to unpack the receive"
  "d archive but the receiving party (in our example\012\"office\") has a yesterday's context ident"
  "ical to yours up to the latest\012packing, that is why the transmitted archive in the office wil"
  "l be unpacked\012and the context file will be renewed and identical to yours. If you want to\012"
  "ban renewal of the context file set it in Read_Only.\012                        EXAMPLE:\012vbn "
  "b john\"office.jon john.txt - John transmitted to the office a renewed text\012vbn r john\"john."
  "ofs            - the office unpacked the transmitted text.\012In the given example the name of t"
  "he context file is convenient to be\012interpreted as address, for John it has the name \"office"
  ".jon\" and in the\012office the name \"john.ofs\".\012                   Press any key to contin"
  "ue ......\000\012                        AGREEMENTS:\012vbn - does not have a possibility to add"
  "/take out one or several files\012      in/out of the archive - only all the data.\012- files wi"
  "th attributes: hidden, system, read_only are placed to archive too\012- Commands: l,p,c,d can be"
  " used in the \"TRAINABLE CHANNEL\"-mode\012- sizes of the compressed data are not limited, but t"
  "he maximum separate file\012  size is 64 Mb.\012- maximum number of files in the archive is 4096"
  ".\012- maximum size of the title is 256 Kb (PATH(s)\\NAME(s).EXT(s)\\ATTRIBUTE(s)).\012- If ther"
  "e is not enough disk space multivolume archive is created\012  automatically!  (1 Kb == 1024 byt"
  "es)\012- name of the first volume *.00b...second *.01b...eleventh *.0ab...last *.vbn\012- unpack"
  "ing begins with *.00b\012- for EP-mode (Errors Protection) names *.\?\?b are renamed *.\?\?p\012"
  "- for EP-mode always \"multivolume archive\" mode is set (by default size of\012  volume is 13 M"
  "b, if nothing else or more than 13 Mb is set)\012                        TECHNICAL REQUIREMENTS:"
  "\012For work of vbn the following technical means are necessary:\012recommended - Pentium-133 RA"
  "M 16 Mb;  minimum - i486SX-33 RAM 16 Mb;\012Operational systems are: MS-DOS 6.+,DOS session in W"
  "IN_95 & OS/2 & WIN_NT-4.0\012  - minimum size of swapping 8 Mb;    maximum not less than 16 Mb;\012"
  "  - size DPMI must be 16 Mb (sometimes it is sufficient to have 15 Mb)\012Note: Usage of archive"
  "s in the background mode under WINDOWS is effective.\012      It can be recommended for computer"
  "s with 20 Mb RAM and more.\012                   Press any key to continue ......\000"
  "\015For the vbn-algorithm it's necessary to have 1550 Kbytes of FREE RAM (Cnv+Ext),\012or if fre"
  "e memory is less, the program can be executed under WINDOWS_95, OS/2\012WIN_NT-4.0, as the progr"
  "am uses its swapping, that results in dramatic slower\012packing/unpacking in big sizes of data "
  "(with 3Mb free RAM, slowing down takes\012place with 100 Kb (or more) of nonpacked data).\012\012"
  "===================================================================\012\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000123456789012345\000\000\000\000\000"
  "\000\000\000\000'\000\000\000\000\000\000\000\001\000"
  "\000\000\000\000\000\000\001\000\000\000Too many files. Wait the next version please...\000"
  "Not enough memory\000\012Too many volumes.\000Invalid path\000"
  "\\\000Too many files.\012Compress only 4096 files.\000"
  "..\000.\000File %s is too large\012\000ERROR: CAN'T CHANGE DIR\000"
  "*.*\000Can't create directory\000\000\000\000\000"
  "Invalid associative model (context error) !!!\000"
  ".vbn\000%s\000\012Archive volume is not found, input path: \000"
  "Error archive file\000For decompressing use vbn 1.%02d.\012\000"
  "Read error\000Process aborted\000CAN'T CREATE FILE\000"
  "Disk full!\000\012Disk full, input new path: \000"
  "\\\000Title more than 256 Kb\012\000Write error\000"
  "\000Nothing do\000.00b\000.00p\000.acp\000Required associative model (context) of data...\000"
  "This archive was created with using associative model (context).\000"
  "\012Archive size is too big. vbn Protect can not be made.\000"
  ".vbn\000Archive size is too big. vbn Protect can not be made.\000"
  "You don't need compress.\012\000\012The file(s) is (are) too small.\012\000"
  "%c\000Compress \000( FAST_mode ) :\000( NORMAL_mode ) :\000"
  "( MAX_mode ) :\000%s \000\\\000Invalid associative model (context error) !!!\000"
  "\012This archive was created without using associative model (context).\012For creating new asso"
  "ciative model point out name of creating context.\012\000"
  "Decompress \000  %s\012\000These is error in volume %s or in previous volume\012\000"
  "Error archive file\000This archive is protected.\000"
  ".00p\000.acp\000%s\000\012Archive volume is not found, input path: \000"
  "Data can not be restore\000Required associative model (context) of data...\000"
  "This archive was created with using associative model (context).\000"
  "For decompressing use vbn 1.%02d.\012\000bytes     mm:dd:yy  hh:mm  ahrs\012\000"
  "Size        Date    Time   Attr  Name\000Contents of archive  %s.vbn:\012\000"
  "  %02d:%02d  \000%02d:%02d:%02d\000%-8d  \000a\000"
  ".\000h\000r\000s  \000.  \000%s\\\000%s\012\000\012Total ... %d bytes ( %d file(s) )\012\000"
  "vbn Protect running...\000.vbn\000%s\000\012Archive volume is not found, input path: \000"
  "\\\000Error archive file\000\012Archive size is too big. vbn Protect can not be made.\000"
  "Protect error\000Write error\000vbn Deprotect running...\000"
  ".acp\000Data can not be restore\000vbn Correct running...\000"
  "Context is saved.\000\000\000Maximal permissible size of block error ......... = %-9u bytes.\012"
  "\000Maximal permissible quantity of stochastic errors = %-9u bits.\012\000"
  "Invalid password !!\000Found Error !!!  %u bits corrected !!!\012\000"
  "Error not found.\000\000\000f\012\000\000y\002\000"
  "\000A\001\000\000\271\000\000\000M\000\000\000(\000"
  "\000\000\030\000\000\000\026\000\000\000\022\000"
  "\000\000\017\000\000\000\015\000\000\000\012\000"
  "\000\000\007\000\000\000\005\000\000\000\003\000"
  "\000\000\003\000\000\000\002\000\000\000\002\000"
  "\000\000\002\000\000\000\001\000\000\000\001\000"
  "\000\000\001\000\000\000\001\000\000\000 Data size= %-10u\012\000"
  "vbn.EXE CRC Error !!!\000Code size= %u  Ratio = %u%% or %u.%05d bpc \000"
  " Time = %d sec.            \012\000\000\000\000\000"
  "\000\000\000\000\000\000\012\000\000\000\006\000"
  "\000\000\004\000\000\000Passed... %-10u bytes\015\000"
  "\000$\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\001\000"
  "\000\?\023\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\?\023\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\320p\000"
  "\000+\000\000\000\322p\000\000+\000\000\000\324p\000"
  "\000+\000\000\000\326p\000\000+\000\000\000\330p\000"
  "\000+\000\000\000\332p\000\000+\000\000\000\334p\000"
  "\000+\000\000\000\336p\000\000+\000\000\000\340p\000"
  "\000+\000\000\000q\002\000\000p\000\033\000>&\000"
  "\000x\000\000\000\000\000\000\000\000\200\000\000"
  "\000\000\000\000\000\000\000\000Floating point not loaded\000"
  "\000\000i\000\000\000b\000\000\000o\000\000\000d\000"
  "\000\000u\000\000\000x\000\000\000p\000\000\000X\000"
  "\000\000s\000\000\000c\000\000\000e\000\000\000E\000"
  "\000\000f\000\000\000g\000\000\000G\000\000\000[\000"
  "\000\000n\000\000\000%\000\000\000\000\000\000\000"
  "\336t\000\020wu\000\020\245u\000\020\323u\000\020\367u\000"
  "\020\001v\000\020\001v\000\020\001v\000\020<v\000"
  "\020\253v\000\020-w\000\020-w\000\020-w\000\020-w\000"
  "\020-w\000\020Aw\000\020rw\000\020Xx\000\020fx\000"
  "\020\000(null)\0000\0000x\0000X\000Floating point not loaded\000"
  "\000\000-\000\000\000+\000\000\000 \000\000\000#\000"
  "\000\0000\000\000\000*\000\000\000\000\000\000\000"
  "`{\000\020\203{\000\020\212{\000\020\221{\000\020\230{\000"
  "\020\237{\000\020\363\200\000\020l\000\000\000h\000"
  "\000\000L\000\000\000N\000\000\000F\000\000\000i|\000"
  "\020p|\000\020w|\000\020~|\000\020\205|\000\020s\000"
  "\000\000%\000\000\000c\000\000\000d\000\000\000i\000"
  "\000\000b\000\000\000o\000\000\000u\000\000\000p\000"
  "\000\000X\000\000\000x\000\000\000f\000\000\000e\000"
  "\000\000E\000\000\000g\000\000\000G\000\000\000n\000"
  "\000\000\265|\000\020\022}\000\020\030}\000\020=}\000"
  "\020=}\000\020\177}\000\020\210}\000\020\240}\000"
  "\020\276}\000\020\356}\000\020\363}\000\020H\177\000"
  "\020H\177\000\020H\177\000\020H\177\000\020H\177\000"
  "\020\215\177\000\020 \000+\000-\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000@\030\030\030\010\010"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000         (((((                  H\020\020\020\020\020\020\020"
  "\020\020\020\020\020\020\020\020\204\204\204\204\204\204\204\204\204\204\020\020\020\020\020\020"
  "\020\201\201\201\201\201\201\001\001\001\001\001\001\001\001\001\001\001\001\001\001\001\001\001"
  "\001\001\001\020\020\020\020\020\020\202\202\202\202\202\202\002\002\002\002\002\002\002\002\002"
  "\002\002\002\002\002\002\002\002\002\002\002\020\020\020\020 \000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\007\000\000\000\000"
  ":\\\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000PATH\000 \\\000\\\000:\000.\000"
  "..\000\000\000\000\000\000\037\000;\000Z\000x\000"
  "\227\000\265\000\324\000\363\000\021\0010\001N\001m\001\000"
  "\000$\317\000\020\000\000\000\000$\317\000\020\012Heap is corrupted\012a\222\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\001\001\000"
  "\000\000\000\000\000\001\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\002\001\000"
  "\000\001\000\000\000\001\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\006\001\000"
  "\000\002\000\000\000\001\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\200\001\000"
  "\000\003\000\000\000\001\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\002\001\000"
  "\000\004\000\000\000\001\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000P\224\000"
  "\020\002\000\000\000\202\000\000\000\200\000\000"
  "\000\001\000\000\000\201\000\000\000\342\224\000"
  "\020\356\224\000\020\356\224\000\020\373\224\000"
  "\020R\225\000\020\015\012Stack Overflow\015\012$\000"
  "\035\237\000\020L\237\000\020v\237\000\020r\224\000"
  "\020\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
  "\000\000\000\000\000\000\000\000\000\000\000\000\000";
const uint32_t image_data_size = sizeof image_data - 1;     // 13172 bytes
