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
// 32-bit integers.

#define _FILE_OFFSET_BITS 64        // stat() on i386 fails on 64-bit inode numbers otherwise
#include <cerrno>
#include <csetjmp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <utility>

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

// count * size does not overflow T (unsigned)
template <class T> static inline bool is_mul_ok(T count, T size)
{
  return count == 0 || size == 0 || count <= T(-1) / size;
}

// Intrinsics, with the x86 semantics the decompiler assumed.  bsr leaves its
// destination unchanged for a zero source, as the CPU does.
static inline uint8_t _BitScanReverse(uint32_t *idx, uint32_t v)
{ if (!v) return 0; *idx = 31 - __builtin_clz(v); return 1; }
// bt/btc with a memory operand address a bit string: the offset is signed and
// may run past the first dword.
static inline uint8_t _bittest(const void *base, int32_t bit)
{ return (((const uint8_t *)base)[bit >> 3] >> (bit & 7)) & 1; }
static inline uint8_t _bittestandcomplement(void *base, int32_t bit)
{ uint8_t *p = (uint8_t *)base + (bit >> 3), m = 1 << (bit & 7), o = *p & m; *p ^= m; return o != 0; }

// ===========================================================================
// vbn's globals, each with storage of its own.  The comment after a name gives
// the address vbn.exe kept it at.  Sizes are what the code reaches.  The tree
// code tells a node (an address) from a leaf (a position, negated) by its
// sign, so the tree has to sit below 0x80000000, which main() checks.
// ===========================================================================
// The direction, and the mode.
static int32_t encoding;                          // 0x1000D4C4 1 compressing, 0 decompressing
static int32_t max_cands;                         // 0x1000D4B0 how many candidates a search returns: 95 (MAX)
static const int32_t mode_shift[3] = { 10, 6, 4 }; // 0x1000CA54 by mode, max_cands >> 5

// The data: a window of 0x198000 bytes (at 0x100166F0) that slides by
// 0x66000.  The code reads up to two bytes before its start, where vbn had
// zeros.  Positions are bit positions, 8 per byte.
static int8_t window_space[16 + 0x198000];
static int8_t *const window = window_space + 16;
static uint32_t window_pos;                       // 0x1001646C where coding is, in bits
static int32_t window_bits;                       // 0x10016444 where the data in the window ends, in bits
static int32_t data_end;                          // 0x1001643C ... and as an address
static int32_t cur_ptr;                           // 0x1001645C the byte being coded
static uint32_t total_bits;                       // 0x10016468 the data coded before the window, in bits
static int32_t prev_byte;                         // 0x100164B0 the byte before the one being coded
static int32_t prev_byte2;                        // 0x100164B4 ... and the one before that
static int8_t next_byte[256];                     // 0x1001620C per byte: the byte that followed it last
static int8_t next_byte2[256];                    // 0x1001630C per byte: the byte that came two after it last

// The context tree: a binary tree over the data read backwards from each
// position, one bit per level.  A slot holding a value > 0 points to a node
// (two slots); a value <= 0 is a leaf, minus the position it registers.  A
// node's two slots differ in bit 2 of their address: the code finds one's
// sibling with `^ 4`.
alignas(8) static int32_t tree[2 + 2 * 0x198000]; // 0x101AE6F0 the root, then the nodes
static int32_t node_next;                         // 0x10016430 the next free node (an address)
static int32_t tree_nodes;                        // 0x1000CA4C nodes in use
static int32_t tree_updates;                      // 0x10016434 counts changes to the tree
static int32_t registered[0x33000 / 4];           // 0x10E6E6F0 a bit per position in the window: in the tree

// The search: from the position being coded, down the tree, collecting
// earlier positions with the same context -- the candidates -- and a
// weight for each.
static int32_t cands[2 * 95];                     // 0x1000D558 {weight, -position} pairs
static int32_t n_cands;                           // 0x10016450 how many
static int32_t search_pos;                        // 0x1001644C where the search starts, in bits
static int32_t search_path[1284];                 // 0x1000D850 the nodes the descent went through
static int32_t search_depth;                      // 0x10016454 the depth the walk back up has reached
static int32_t search_avg;                        // 0x10016438 the depth a candidate has to beat
static int32_t search_gain;                       // 0x1001648C how far the last search beat it
static int32_t weight_shift;                      // 0x10016448 the next candidate's weight shift (4 down to 0)
static int32_t cand_floor;                        // 0x100164C4 a quarter of the weakest candidate's weight
static int32_t excluded_byte;                     // 0x10016488 the byte a failed match predicted, or -1
static int32_t found_at;                          // 0x1001647C where the last descent stopped: the bit position,
static int32_t found_slot;                        // 0x10016480 ... the slot,
static int32_t found_leaf;                        // 0x10016484 ... and what the slot held

// Candidates grouped by the value they predict: a match length in
// code_match, a byte in code_literal.  Loops over the groups start one pair
// before them (and step before they read), so that pair is storage too.
static int32_t group_slot[256];                   // 0x1000EC60 per value: its {weight, value} pair in groups, or 0
static int32_t groups_space[2 + 2 * 256];
static int32_t *const groups = groups_space + 2;

// The match length model: a count for each length 0..255 (0: no match),
// following the last 4096 lengths coded.
static int32_t len_model[2 * 256];                // 0x1000F860 {count, length} pairs
static uint32_t len_total;                        // 0x100164C0 the sum of the counts
static const int32_t len_init_counts[23] = {     // 0x1000C97C the counts lengths 0..22 start with
  2662, 633, 321, 185, 77, 40, 24, 22, 18, 15, 13, 10, 7, 5, 3, 3, 2, 2, 2, 1, 1, 1, 1 };
static int32_t len_ring[4096];                    // 0x1001220C the last 4096 lengths
static int32_t len_ring_pos;                      // 0x10016420
static int32_t len_avg;                           // 0x100164A8 the average length coded, x128

// The literal model: a count for each byte, kept sorted by count; the last
// 363 literals add 16 to theirs.  Its re-sorting steps read one pair past
// either end before their bound check stops them, so those pairs are
// storage too (vbn had other tables there).
static uint32_t lit_model_space[2 + 2 * 256 + 2];
static uint32_t *const lit_model = lit_model_space + 2;
static int32_t lit_slot[256];                     // 0x10010060 per byte: its pair in lit_model
static uint32_t lit_total;                        // 0x100164BC the sum of the counts
static int32_t lit_inc;                           // 0x10016490 what every count gets added (1..1024, by lit_cost8)
static int32_t lit_resort;                        // 0x10016494 literals until the model is sorted again
static int32_t lit_ring1[363];                    // 0x10010C60 the last 363 literals
static int32_t lit_ring1_pos;                     // 0x10016418
static int32_t lit_ring2[1024];                   // 0x1001120C the last 1024 literals, with the byte before each
static int32_t lit_ring2_pos;                     // 0x10016428
static int8_t ctx_count[0x10000];                 // 0x10EB96F0 per byte pair: its count in lit_ring2
static int32_t ctx16;                             // 0x100164C8 the byte before and the byte coded, as a pair
static int32_t literal_run;                       // 0x100164AC literals in a row whose pair was known
static int32_t lit_threshold;                     // 0x100164B8 set by code_literal, weighs code_match's model
static int32_t lit_cost8;                         // 0x100164A4 the bits a literal costs, x256, averaged over
static int32_t lit_cost9;                         // 0x100164A0 ... 2^8, 2^9, 2^10 and 2^11 literals
static int32_t lit_cost10;                        // 0x1001649C
static int32_t lit_cost11;                        // 0x10016498

// The arithmetic coder.
static int8_t code_buf[0x2010];                   // 0x1000A110 8 KB of code, and 16 bytes more (vbn reused its banner)
static int32_t code_low;                          // 0x1001640C the interval
static int32_t code_high;                         // 0x10016410
static int32_t code_pending;                      // 0x10016414 bits of the interval not settled yet
static int32_t code_value;                        // 0x10016460 decompressing: the code read so far

// ===========================================================================
// The search's early exit.  vbn.exe's 0x1000703B records where the next call
// returns to, and its 0x10006E22 returns from that call at once, from however
// deep in the search: setjmp and longjmp, run_search() and search_exit().
// The walks end the search through walk_exit when the candidates are full.
// ===========================================================================
static jmp_buf search_jmp;
[[noreturn]] static void search_exit(void) { longjmp(search_jmp, 1); }
[[noreturn]] static void walk_exit(void)
{
  n_cands = max_cands;
  longjmp(search_jmp, 1);
}

// ===========================================================================
// vbn's functions: the ones plain `u` and `r` run, callees first.
// ===========================================================================

// code_flush (vbn.exe 0x10004993): end the code.  Writes into code_buf, from
// bit `a1` on, the shortest tail -- a 0 and then 1s, or a 1 and then 0s --
// that keeps every continuation inside the interval, resets the interval, and
// returns the bit position after the tail.
static uint32_t code_flush(uint32_t a1)
{
  int32_t v7, v8;
  uint32_t v5, v6;

  v7 = code_pending;
  v6 = 1;
  v5 = 1;
  code_low = (int32_t)((uint32_t)code_low << 1);   // shl: wraps
  code_high = (int32_t)((uint32_t)code_high << 1);   // shl: wraps
  while ( v7-- )
  {
    ++v5;
    if ( code_low >= 0 )
      break;
    code_low = (int32_t)((uint32_t)code_low << 1);   // shl: wraps
  }
  v8 = code_pending;
  while ( v8-- )
  {
    ++v6;
    if ( code_high < 0 )
      break;
    code_high = (int32_t)((uint32_t)code_high << 1);   // shl: wraps
  }
  code_low = 0x7FFFFFFF;
  if ( v5 > v6 )
  {
    v5 = v6;
    code_low = 0x80000000;
  }
  while ( v5-- )
  {
    if ( code_low < 0 )
      _bittestandcomplement(
        (int32_t *)code_buf,
        a1);
    ++a1;
    code_low = (int32_t)((uint32_t)code_low << 1);   // shl: wraps
    --code_pending;
  }
  code_low = 0;
  code_high = -1;
  return a1;
}

// len_model_add (vbn.exe 0x100050B0): count a length in the length model.
// Its count goes up by 5 and the count of the length coded 4096 lengths ago
// (from len_ring) down by 5; until the ring is full, len_total grows by 5
// instead.  Returns that old length, or -1.
static int32_t len_model_add(int32_t len)
{
  int32_t pos, *v2, result;

  LOWORD(len_model[2 * len]) += 5;
  pos = len_ring_pos;
  len_ring_pos = ((uint16_t)len_ring_pos + 1) & 0xFFF;
  v2 = &len_ring[pos];
  result = *v2;
  *v2 = len;
  if ( result >= 0 )
    LOWORD(len_model[2 * result]) -= 5;
  else
    len_total += 5;
  return result;
}

// tree_insert (vbn.exe 0x10005251): register positions in the context tree.
// In vbn.exe it is assembly that keeps the bit position in EBP, which
// Hex-Rays read as the frame pointer (&savedregs); this is written from it.
//
// For iters = min(bsr(count + count/2 + 1) + 1, count) positions pos = bitpos,
// bitpos+8, ...:
//  - find a slot: the first time, the one the last search left in found_at,
//    found_slot and found_leaf (bit position, slot address, slot value); after
//    that, descend from the root reading the data bits backwards from byte
//    pos/8-1 (bit 1 -> child [0], bit 0 -> child [1]) to the first slot
//    holding a value <= 0, or to node[0] of the last node when the bits run out;
//  - depth = pos - (bit position where the descent stopped); then
//      depth <= 64*bsr(tree_nodes+32): an empty leaf becomes a new node from
//          node_next, both children -8*(pos/8); the sibling slot being a node
//          bumps tree_updates;
//      deeper: a leaf older than 0x800 is refreshed to -8*(pos/8);
//    either way bit pos/8 of `registered` is toggled.
static void tree_insert(uint32_t count, int32_t bitpos)
{
  uint32_t t = count + (count >> 1) + 1, iters = t ? 31 - __builtin_clz(t) : 0;
  iters += 1;
  if (iters > count)
    iters = count;
  uint32_t pos = (uint32_t)bitpos;                         // EBP
  uint32_t at = (uint32_t)found_at;                  // ESI
  int32_t *slot = (int32_t *)(uintptr_t)found_slot;   // EBX
  int32_t leaf = found_leaf;                          // EDX
  for (;;) {
    uint32_t depth = pos - at;
    uint32_t thr = (uint32_t)(31 - __builtin_clz((uint32_t)tree_nodes + 32)) << 6;
    if (depth > thr) {
      if (leaf <= 0 && pos + (uint32_t)leaf >= 0x800) {
        _bittestandcomplement(((void *)::registered), (int32_t)(pos >> 3));
        *slot = -(int32_t)((pos >> 3) << 3);
        tree_updates++;
      }
    } else if (leaf <= 0) {
      _bittestandcomplement(((void *)::registered), (int32_t)(pos >> 3));
      int32_t v = -(int32_t)((pos >> 3) << 3);
      int32_t *node = (int32_t *)(uintptr_t)node_next;
      node[0] = v;
      node[1] = v;
      *slot = (int32_t)(uintptr_t)node;
      node_next += 8;
      tree_nodes++;
      slot = (int32_t *)((uintptr_t)slot ^ 4);
      if (*slot > 0)
        tree_updates++;
    }
    if (--iters == 0)
      return;
    pos += 8;
    // descend from the root
    int32_t *node = ((int32_t *)tree);
    at = pos;
    slot = node;
    if (at == 0)
      continue;                                        // or esi,esi; je (leaf unchanged)
    const uint8_t *p = ((const uint8_t *)window) + (pos >> 3) - 1;
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

// walk_cands (vbn.exe 0x10006CB0; 0x10006E31 when excluding): add the leaves
// of a subtree to the candidates, left to right, as {weight, leaf} pairs.  A
// node's right leaf counts only when its left child is a node (vbn compares
// that slot with the node's address: a node's children are made after it, at
// higher addresses).  Excluding, a leaf that predicts excluded_byte is
// skipped.  In vbn.exe the walks are assembly with a register interface (EBX
// node, ESI weight, ECX output cursor, EDI how many pairs may still be
// written; ECX and EDI come back); here the caller passes a Walk.  When no
// more pairs may be written, the search ends (walk_exit).
struct Walk { int32_t *out; int32_t left; int32_t weight; bool excluding; };

// Whether the byte at the position of `leaf`, the byte it predicts, is
// excluded_byte.
static inline bool predicts_excluded(int32_t leaf)
{
  return (uint8_t)window[(uint32_t)-leaf >> 3] == excluded_byte;
}
static void walk_emit(int32_t leaf, Walk *w)
{
  w->out[1] = leaf;
  if (w->excluding && predicts_excluded(leaf))
    return;
  w->out[0] = w->weight;
  w->out += 2;
  if (--w->left == 0)
    walk_exit();
}
static void walk_cands(int32_t *node, Walk *w)
{
  if (node[0] > 0)
    walk_cands((int32_t *)(uintptr_t)node[0], w);
  else
    walk_emit(node[0], w);
  if (node[1] > 0)
    walk_cands((int32_t *)(uintptr_t)node[1], w);
  else if (node[0] > (int32_t)(uintptr_t)node)    // cmp eax,[eax]; jle
    walk_emit(node[1], w);
}

// code_symbol (vbn.exe 0x100061F4): code one symbol with a frequency list of
// {count, symbol} pairs whose counts sum to `total`.  Compressing (symbol >= 0),
// the interval narrows to the symbol's share and the bits it settles go to
// code_buf at *code_pos; decompressing (symbol < 0), the symbol is the one
// whose share holds the code read so far.  Returns the symbol, and moves
// *code_pos past the bits.
static int32_t code_symbol(uint32_t total, uint32_t *code_pos, int32_t symbol, uint32_t *model)
{
  bool v18, v31;
  int32_t nbits, v24, n32, v27, v28, low;
  uint32_t v4, *i, v6, v8, value, v11, v12, v13, v14, v15, v16, v17, v19, v20, v22, *i_3,
           v25, v29, *i_2, v34;
  uint64_t v7, v21;
  uint32_t v35; // [esp+14h] [ebp-4h]  high - low, which wraps

  v34 = *code_pos;
  v35 = (uint32_t)code_high - (uint32_t)::code_low;
  if ( 2 * total > (uint32_t)code_high - (uint32_t)::code_low )
  {
    v34 = code_flush(*code_pos);
    v35 = -1;
  }
  if ( symbol >= 0 )
  {
    v4 = 0;
    for ( i = model; (uint32_t)symbol != i[1]; i += 16 )
    {
      v4 += *i;
      if ( (uint32_t)symbol == i[3] )
      {
        i += 2;
        break;
      }
      v4 += i[2];
      if ( (uint32_t)symbol == i[5] )
      {
        i += 4;
        break;
      }
      v4 += i[4];
      if ( (uint32_t)symbol == i[7] )
      {
        i += 6;
        break;
      }
      v4 += i[6];
      if ( (uint32_t)symbol == i[9] )
      {
        i += 8;
        break;
      }
      v4 += i[8];
      if ( (uint32_t)symbol == i[11] )
      {
        i += 10;
        break;
      }
      v4 += i[10];
      if ( (uint32_t)symbol == i[13] )
      {
        i += 12;
        break;
      }
      v4 += i[12];
      if ( (uint32_t)symbol == i[15] )
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
    if ( v35 == (uint32_t)(-1) )
    {
      if ( HIDWORD(v7) == total )
        goto LABEL_25;
    }
    else
    {
      v7 = HIDWORD(v7) * (uint64_t)(uint32_t)v7;
    }
    LODWORD(v7) = v7 / total;
LABEL_25:
    code_high = ::code_low + v7 - 1;
    LODWORD(v7) = v35 + 1;
    if ( v35 == (uint32_t)(-1) )
    {
      HIDWORD(v7) = v6;
      if ( v6 == total )
      {
LABEL_29:
        ::code_low += v7;
        goto LABEL_76;
      }
    }
    else
    {
      v7 = v6 * (uint64_t)(uint32_t)v7;
    }
    LODWORD(v7) = v7 / total;
    goto LABEL_29;
  }
  v8 = ::code_pending + v34;
  nbits = 33 - ::code_pending;
  value = ::code_value;
  while ( --nbits )
  {
    value += _bittest(
                       (int32_t *)code_buf,
                       v8) + value;
    v11 = v8 + 1;
    if ( !--nbits )
      break;
    value += _bittest(
                       (int32_t *)code_buf,
                       v11) + value;
    v12 = v11 + 1;
    if ( !--nbits )
      break;
    value += _bittest(
                       (int32_t *)code_buf,
                       v12) + value;
    v13 = v12 + 1;
    if ( !--nbits )
      break;
    value += _bittest(
                       (int32_t *)code_buf,
                       v13) + value;
    v14 = v13 + 1;
    if ( !--nbits )
      break;
    value += _bittest(
                       (int32_t *)code_buf,
                       v14) + value;
    v15 = v14 + 1;
    if ( !--nbits )
      break;
    value += _bittest(
                       (int32_t *)code_buf,
                       v15) + value;
    v16 = v15 + 1;
    if ( !--nbits )
      break;
    value += _bittest(
                       (int32_t *)code_buf,
                       v16) + value;
    v17 = v16 + 1;
    if ( !--nbits )
      break;
    value += _bittest(
                       (int32_t *)code_buf,
                       v17) + value;
    v8 = v17 + 1;
  }
  ::code_value = value;
  ::code_pending = nbits;
  v18 = value < (uint32_t)::code_low;
  v19 = value - ::code_low;
  v20 = v19 + 1;
  if ( v19 == (uint32_t)(-1) )
  {
    HIDWORD(v21) = total;
    v22 = v18 + 1;
    v18 = v20 < v22;
    LODWORD(v21) = v20 - v22;
    if ( !v18 )
    {
      if ( v35 != (uint32_t)(-1) )
      {
        LODWORD(v21) = v21 / (v35 + 1);
        goto LABEL_50;
      }
      goto LABEL_44;
    }
LABEL_45:
    --HIDWORD(v21);
    if ( v35 != (uint32_t)(-1) )
    {
      LODWORD(v21) = v21 / (v35 + 1);
      goto LABEL_50;
    }
    goto LABEL_44;
  }
  v21 = total * (uint64_t)v20;
  v18 = (uint32_t)v21 < (uint32_t)(HIDWORD(v21) != 0) + 1;
  LODWORD(v21) = v21 - ((HIDWORD(v21) != 0) + 1);
  if ( v18 )
    goto LABEL_45;
  if ( v35 == (uint32_t)(-1) )
  {
LABEL_44:
    LODWORD(v21) = HIDWORD(v21);
    goto LABEL_50;
  }
  LODWORD(v21) = v21 / (v35 + 1);
LABEL_50:
  i_3 = model;
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
  if ( v35 == (uint32_t)(-1) )
  {
    if ( HIDWORD(v21) == total )
      goto LABEL_71;
  }
  else
  {
    v21 = HIDWORD(v21) * (uint64_t)(uint32_t)v21;
  }
  LODWORD(v21) = v21 / total;
LABEL_71:
  code_high = ::code_low + v21 - 1;
  v25 = v24 - *i_3;
  LODWORD(v21) = v35 + 1;
  if ( v35 != (uint32_t)(-1) )
  {
    v21 = v25 * (uint64_t)(uint32_t)v21;
LABEL_74:
    LODWORD(v21) = v21 / total;
  }
  else
  {
    HIDWORD(v21) = v25;
    if ( v25 != total )
      goto LABEL_74;
  }
  ::code_low += v21;
LABEL_76:
  n32 = 32;
  v27 = code_high;
  v28 = ::code_low ^ code_high;
  if ( (::code_low ^ code_high) >= 0 )
  {
    v29 = v34;
    low = ::code_low;
    do
    {
      if ( low < 0 )
        _bittestandcomplement(
          (int32_t *)code_buf,
          v29);
      --n32;
      ++v29;
      low = (int32_t)((uint32_t)low << 1);   // shl/rcl: wrap
      v27 = (int32_t)((uint32_t)v27 << 1 | 1);
      v31 = (v28 & 0x40000000) != 0;
      v28 = (int32_t)((uint32_t)v28 << 1);
    }
    while ( !v31 );
    v34 = v29;
    ::code_low = low;
    code_high = v27;
  }
  ::code_pending = n32;
  *code_pos = v34;
  return i_2[1];
}

// code_literal (vbn.exe 0x100064C0): code the byte at bitpos as a literal.  The
// literal model's counts are mixed with the candidates' predictions (grouped
// by the byte they predict); `excluded` is a byte a failed match already ruled
// out, or -1.  Afterwards the literal model, the rings and the cost averages
// adapt.  Returns the byte (decompressing: also stores it in the window).
static int32_t code_literal(int32_t excluded, uint32_t *code_pos, uint32_t bitpos)
{
  int16_t cnt;
  int32_t n2, v5, k256, *v9, v10, *v11, v12, v14, *v15, *v16, v18, v20, v21, v22, v28, *v34, v35,
          *v36, v37, n2_2, v41, v42, *v43, v44, v45, v46, v49, v50, v51, v52, v53, *v54, v55,
          v56, v57, *v60, v61, rpos, v64, n362_3, v72, n363, *v74, *i_2, v76, v78, v79, result,
          rpos1, n362_2, v84, v85, v89, v90, v91;
  int8_t v33, v38, *v63, *v83;
  uint32_t v3, *i, lit_sum, v13, *v19, v23, v25, v26, v27, v29, v30, v31, v32, *v39, *v47, v48,
           v58, v59, *i_1, *v66, v67, v68, v69, v70, v77, v86, n0x10000, n2048_1;
  uint64_t v24;

  cand_floor = (uint32_t)cand_floor >> 1;
  _BitScanReverse(&v3, ((uint32_t)lit_cost8 >> 3) + 1);
  lit_threshold = ((uint32_t)search_gain >> 4)
                 + 54
                 - (((uint32_t)lit_cost11 >> 11)
                  + 1)
                 - (((uint32_t)lit_cost10 >> 10)
                  + 1)
                 - (((uint32_t)lit_cost9 >> 9)
                  + 1)
                 - (((uint32_t)lit_cost8 >> 8)
                  + 1)
                 - v3
                 - ((((uint32_t)lit_cost11 >> 11) + 1) >> 1)
                 - ((((uint32_t)lit_cost10 >> 10) + 1) >> 1)
                 - ((((uint32_t)lit_cost9 >> 9) + 1) >> 1)
                 - ((((uint32_t)lit_cost8 >> 8) + 1) >> 1);
  if ( lit_threshold <= 0 )
  {
    lit_threshold = 0;
    if ( search_gain <= 4 )
    {
      ::n_cands = 0;
      lit_cost9 = 4096;
      lit_cost10 = 0x2000;
      lit_cost11 = 0x4000;
    }
  }
  n2 = 1;
  if ( (uint32_t)lit_cost8 >= 0x2EC )
  {
    n2 = 2;
    if ( (uint32_t)lit_cost8 >= 0x37F )
    {
      n2 = 4;
      if ( (uint32_t)lit_cost8 >= 0x489 )
      {
        n2 = 8;
        if ( (uint32_t)lit_cost8 >= 0x56F )
        {
          n2 = 16;
          if ( (uint32_t)lit_cost8 >= 0x63F )
          {
            n2 = 32;
            if ( (uint32_t)lit_cost8 >= 0x70F )
            {
              n2 = 128;
              lit_cost9 = 4096;
              lit_cost10 = 0x2000;
              lit_cost11 = 0x4000;
              if ( (uint32_t)lit_cost8 >= 0x75C )
                n2 = 1024;
            }
          }
        }
      }
    }
  }
  if ( lit_inc != n2 )
  {
    v5 = n2 - lit_inc;
    lit_inc += v5;
    k256 = 256;
    i = ::lit_model;
    lit_sum = ::lit_total;
    do
    {
      lit_sum += v5;
      *i += v5;
      i += 2;
      --k256;
    }
    while ( k256 );
    ::lit_total = lit_sum;
  }
  if ( excluded < 0 || (v9 = (int32_t *)lit_slot[excluded], v10 = *v9, *v9 = 0, v89 = v10, ::lit_total -= v10, excluded != excluded_byte) )
  {
    if ( excluded_byte >= 0 )
    {
      v11 = (int32_t *)lit_slot[excluded_byte];
      v12 = *v11;
      *v11 = 0;
      v90 = v12;
      ::lit_total -= v12;
    }
  }
  v13 = 0;
  v86 = 0;
  if ( ::n_cands )
  {
    v14 = 0;
    v15 = groups;
    v16 = cands;
    cnt = ::n_cands;
    do
    {
      while ( 1 )
      {
        while ( 1 )
        {
          v18 = *((uint8_t *)v16 + 4);
          if ( v18 != excluded )
            break;
          v16 += 2;
          ++HIBYTE(cnt);
          LOBYTE(cnt) = cnt - 1;
          if ( !(uint8_t)cnt )
            goto LABEL_52;
        }
        v19 = (uint32_t *)group_slot[v18];
        if ( !v19 )
          break;
        v20 = *v16 - cand_floor;
        *v19 += v20;
        v14 += v20;
        v16 += 2;
        LOBYTE(cnt) = cnt - 1;
        if ( !(uint8_t)cnt )
          goto LABEL_52;
      }
      group_slot[v18] = (int32_t)v15;
      LOBYTE(ctx16) = v18;
      v15[1] = v18;
      if ( (uint32_t)search_gain < 0x13 )
      {
        if ( ctx_count[ctx16] )
        {
          if ( (uint8_t)ctx_count[ctx16] > 1u )
          {
            if ( (uint8_t)ctx_count[ctx16] > 2u )
            {
              if ( (uint8_t)ctx_count[ctx16] > 3u )
              {
                if ( (uint8_t)ctx_count[ctx16] > 6u )
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
      else if ( ctx_count[ctx16] )
      {
        if ( (uint8_t)ctx_count[ctx16] > 1u )
        {
          if ( (uint8_t)ctx_count[ctx16] > 2u )
          {
            if ( (uint8_t)ctx_count[ctx16] > 3u )
            {
              if ( (uint8_t)ctx_count[ctx16] > 4u )
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
      v22 = *v16 + v21 - cand_floor;
      *v15 = v22;
      v14 += v22;
      v15 += 2;
      v16 += 2;
      LOBYTE(cnt) = cnt - 1;
    }
    while ( (uint8_t)cnt );
LABEL_52:
    v85 = v14;
    v13 = (uint32_t)((int8_t *)v15 - (int8_t *)groups) >> 3;
    v86 = v13;
    if ( HIBYTE(cnt) )
      --::n_cands;
  }
  if ( v13 )
  {
    v23 = ::n_cands * v85;
    v24 = ::lit_total
        * (uint64_t)((::n_cands - v86) * ((3 * lit_threshold + 5 * (((uint32_t)search_gain >> 2) + search_gain)) >> 3));
    if ( is_mul_ok(::lit_total, (::n_cands - v86) * ((3 * lit_threshold + 5 * (((uint32_t)search_gain >> 2) + search_gain)) >> 3)) )
      v25 = (uint32_t)v24 >> 3;
    else
      v25 = v24 >> 3;
    n2048_1 = ::lit_total;
    v26 = v25 >> (mode_shift[(uint32_t)max_cands >> 5] & 31);
    v27 = v25 - v26;
    v28 = ((uint32_t)(48 - lit_threshold) >> 3) - 4;
    if ( v28 < 0 )
      v29 = v26 << ((4 - ((uint32_t)(48 - lit_threshold) >> 3)) & 31);
    else
      v29 = v26 >> (v28 & 31);
    v30 = v29 + v27;
    v31 = v29;   // bsr edx,eax: EDX holds v29, and bsr leaves it there when v30 == 0
    _BitScanReverse(&v31, v30);
    _BitScanReverse(&v32, v23);
    v33 = 31 - v31 + v32;
    ::n_cands = ((uint64_t)v30 << ((v33 & 0x1F) & 31)) / v23;
    if ( group_slot[(uint8_t)next_byte[prev_byte]] )
    {
      v34 = (int32_t *)group_slot[(uint8_t)next_byte[prev_byte]];
      v35 = *v34;
      *v34 *= 8;
      *v34 -= v35;
      *v34 = (uint32_t)*v34 >> 2;
    }
    if ( group_slot[(uint8_t)next_byte2[prev_byte2]] )
    {
      v36 = (int32_t *)group_slot[(uint8_t)next_byte2[prev_byte2]];
      v37 = *v36;
      *v36 *= 2;
      *v36 += v37;
      *v36 = (uint32_t)*v36 >> 1;
    }
    v38 = v86;
    v39 = (uint32_t *)(::groups - 2);
    n2_2 = ::n_cands;
    if ( (uint32_t)lit_cost8 < 0x6C9 )
    {
      if ( literal_run )
      {
        do
        {
          v39 += 2;
          v52 = ((uint32_t)n2_2 * (uint64_t)*v39) >> (v33 & 31);
          ::lit_total += v52;
          v53 = v39[1];
          LOBYTE(ctx16) = v53;
          v54 = (int32_t *)lit_slot[v53];
          group_slot[v53] -= (int32_t)v39;
          v55 = *v54;
          *v39 = *v54;
          v56 = v55 + v52;
          v57 = (uint8_t)ctx_count[ctx16] << 6;
          ::lit_total += v57;
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
          ::lit_total += v45;
          v46 = v39[1];
          LOBYTE(ctx16) = v46;
          v47 = (uint32_t *)lit_slot[v46];
          group_slot[v46] -= (int32_t)v39;
          v48 = *v47;
          *v39 = *v47;
          v49 = v48 + v45;
          v48 >>= 2;
          v50 = v48 + v49;
          ::lit_total += v48;
          v51 = (uint8_t)ctx_count[ctx16] << 6;
          ::lit_total += v51;
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
        ::lit_total += v41;
        v42 = v39[1];
        v43 = (int32_t *)lit_slot[v42];
        group_slot[v42] -= (int32_t)v39;
        v44 = *v43;
        *v39 = *v43;
        *v43 = v44 + v41;
        --v38;
      }
      while ( v38 );
    }
  }
  n0x10000 = *code_pos;
  if ( encoding )
    v91 = code_symbol(::lit_total, (uint32_t *)code_pos, (uint8_t)window[bitpos >> 3], ::lit_model);
  else
    v91 = code_symbol(::lit_total, (uint32_t *)code_pos, -1, ::lit_model);
  v58 = (*code_pos - n0x10000) << 8;
  lit_cost8 = (v58 + (lit_cost8 << 8) - lit_cost8) >> 8;
  v58 *= 2;
  lit_cost9 = (v58 + (lit_cost9 << 9) - lit_cost9) >> 9;
  v58 *= 2;
  lit_cost10 = (v58 + (lit_cost10 << 10) - lit_cost10) >> 10;
  lit_cost11 = (2 * v58 + (lit_cost11 << 11) - lit_cost11) >> 11;
  v59 = v86;
  if ( v86 )
  {
    v60 = groups;
    ::lit_total = n2048_1;
    do
    {
      *(uint32_t *)lit_slot[v60[1]] = *v60;
      v60 += 2;
      --v59;
    }
    while ( v59 );
  }
  if ( excluded < 0 || (*(uint32_t *)lit_slot[excluded] = v89, ::lit_total += v89, excluded != excluded_byte) )
  {
    if ( excluded_byte >= 0 )
    {
      *(uint32_t *)lit_slot[excluded_byte] = v90;
      ::lit_total += v90;
    }
  }
  v61 = ctx16;
  LOBYTE(v61) = v91;
  window[bitpos >> 3] = v91;
  if ( ctx_count[v61] != -1 )
  {
    ++literal_run;
    if ( !ctx_count[v61] )
      literal_run = 0;
    ++ctx_count[v61];
  }
  rpos = ::lit_ring2_pos;
  ::lit_ring2_pos = ((uint16_t)::lit_ring2_pos + 1) & 0x3FF;
  v63 = (int8_t *)&lit_ring2[rpos];
  v64 = *(uint32_t *)v63;
  *(uint32_t *)v63 = v61;
  if ( v64 >= 0 && ctx_count[v64] )
    --ctx_count[v64];
  if ( --lit_resort )
  {
    result = v91;
    *(uint32_t *)lit_slot[v91] += 16;
    rpos1 = ::lit_ring1_pos;
    n362_2 = ::lit_ring1_pos + 1;
    if ( ::lit_ring1_pos == 362 )
      n362_2 = 0;
    ::lit_ring1_pos = n362_2;
    v83 = (int8_t *)&lit_ring1[rpos1];
    v84 = *(uint32_t *)v83;
    *(uint32_t *)v83 = v91;
    if ( v84 < 0 )
      ::lit_total += 16;
    else
      *(uint32_t *)lit_slot[v84] -= 16;
  }
  else
  {
    lit_resort = (uint8_t)(BYTE1(lit_cost8) + 1);
    i_1 = (uint32_t *)lit_slot[v91];
    *i_1 += 16;
    v66 = i_1 - 2;
    v67 = *i_1;
    v68 = *(i_1 - 2);
    if ( *i_1 > v68 )
    {
      do
      {
        if ( i_1 <= ::lit_model )
          break;
        *i_1 = v68;
        *v66 = v67;
        v69 = i_1[1];
        v70 = v66[1];
        i_1[1] = v70;
        v66[1] = v69;
        lit_slot[v70] = (int32_t)i_1;
        lit_slot[v69] = (int32_t)v66;
        i_1 -= 2;
        v66 -= 2;
        v67 = *i_1;
        v68 = *v66;
      }
      while ( *i_1 > *v66 );
    }
    n362_3 = ::lit_ring1_pos;
    v72 = *(uint32_t *)&lit_ring1[::lit_ring1_pos];
    *(uint32_t *)&lit_ring1[::lit_ring1_pos] = v91;
    n363 = n362_3 + 1;
    if ( n363 == 363 )
      n363 = 0;
    ::lit_ring1_pos = n363;
    if ( v72 < 0 )
    {
      ::lit_total += 16;
    }
    else
    {
      v74 = (int32_t *)lit_slot[v72];
      *v74 -= 16;
      i_2 = v74 + 2;
      v76 = *v74;
      v77 = v74[2];
      if ( (uint32_t)(*v74) < v77 )
      {
        do
        {
          if ( i_2 >= (int32_t *)::lit_model + 2 * 256 )
            break;
          *v74 = v77;
          *i_2 = v76;
          v78 = v74[1];
          v79 = i_2[1];
          v74[1] = v79;
          i_2[1] = v78;
          lit_slot[v79] = (int32_t)v74;
          lit_slot[v78] = (int32_t)i_2;
          v74 += 2;
          i_2 += 2;
          v76 = *v74;
          v77 = *i_2;
        }
        while ( (uint32_t)(*v74) < (uint32_t)*i_2 );
      }
    }
    return v91;
  }
  return result;
}

// code_match (vbn.exe 0x10005C50): code how many bytes at bitpos match the best
// candidate -- 0..255, 0 being no match -- with the length model, boosted by
// the candidates grouped by their own match lengths.  On a mismatch,
// *excluded gets the byte the candidate predicted (else -1).  Returns the
// length (decompressing: also copies the matched bytes into the window).
static int32_t code_match(uint32_t *code_pos, int32_t *excluded, uint32_t bitpos)
{
  bool v13;
  int32_t v3, result, v7, *v9, *v10, v18, v19, *v35, v36, v37, v39, v46, n255_7, v48, v52, v56,
          n255_4, v58, v62, *v64, rpos, v67, *v69, v73, n2, v77;
  int8_t *v6, *v11, *v12, *v15, n255_2, v32, v34, *v44, *v49, *v54, *v59, *v66, *v68, *v75;
  uint32_t v5, v8, n255_1, *v17, *v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31,
           v33, *v38, n256_1, *v41, v42, n255_6, n255_8, n255_3, n255_5, v63, v70, n255, len_sum0,
           n256_2;
  uint8_t *v43, *v51, *v53, *v61;

  v3 = 0;
  if ( bitpos >> 3 )
    BYTE1(v3) = window[(bitpos >> 3) - 1];
  ctx16 = v3;
  if ( n_cands )
  {
    v73 = len_model[0];
    ::len_total -= len_model[0];
    v5 = 9 * literal_run + search_gain + ((uint32_t)lit_threshold >> 2) + 1;
    n2 = 0;
    len_model[0] = 5 * len_model[0] / (v5 + (v5 >> 4));
    ::len_total += len_model[0];
    n255 = 255;
    v6 = &window[-(::cands + 1)[0] >> 3];
    v75 = v6;
    cur_ptr = (int32_t)&window[bitpos >> 3];
    if ( (cur_ptr - (uint32_t)v6) < 0xFF )
      n255 = cur_ptr - (uint32_t)v6;
    if ( (uint32_t)(data_end - cur_ptr) < n255 )
      n255 = data_end - cur_ptr;
    v7 = (uint8_t)*v6;
    (::cands + 1)[0] = v7;
    if ( (uint8_t)v7 == (uint8_t)(next_byte[prev_byte]))
    {
      n2 = 1;
      if ( (uint8_t)v7 == (uint8_t)(next_byte[prev_byte2]))
        n2 = 2;
    }
    v8 = 0;
    v9 = groups;
    v10 = (::cands + 2);
    BYTE1(v6) = n_cands;
    LOBYTE(v6) = 1;
    if ( (uint8_t)n_cands > 1u )
    {
      do
      {
        while ( 1 )
        {
          v11 = v75;
          v12 = &window[(uint32_t)-v10[1] >> 3];
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
            if ( v75 <= v12 && n255 > (uint32_t)(cur_ptr - (int32_t)v12) )
              n255_1 = cur_ptr - (uint32_t)v12;
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
          v17 = (uint32_t *)group_slot[n255_1];
          if ( !v17 )
            break;
          v18 = *v10 - cand_floor;
          *v17 += v18;
          v8 += v18;
          v10 += 2;
          LOBYTE(v6) = (uint8_t)(uintptr_t)v6 + 1;
          if ( (uint8_t)(uintptr_t)v6 >= BYTE1(v6) )
            goto LABEL_33;
        }
        group_slot[n255_1] = (int32_t)v9;
        v9[1] = n255_1;
        LOBYTE(n255_1) = BYTE1(v6) - (uint8_t)(uintptr_t)v6;
        v19 = *v10 + (n255_1 >> 1) - cand_floor;
        *v9 = v19;
        v8 += v19;
        v9 += 2;
        v10 += 2;
        LOBYTE(v6) = (uint8_t)(uintptr_t)v6 + 1;
      }
      while ( (uint8_t)(uintptr_t)v6 < BYTE1(v6) );
    }
LABEL_33:
    v70 = (uint32_t)((int8_t *)v9 - (int8_t *)groups) >> 3;
    if ( v70 )
    {
      v20 = (uint32_t *)group_slot[0];
      if ( group_slot[0] )
      {
        _BitScanReverse(&v21, ((uint32_t)(48 - lit_threshold) >> 3) + 1);
        v22 = (uint32_t)len_avg >> (v21 & 31);
        v23 = *(uint32_t *)group_slot[0];
        if ( n2 )
        {
          v22 = len_avg;
          v23 = ((3 * v23) >> 2) + 1;
          if ( n2 != 1 )
            v23 = ((7 * v23) >> 3) + 1;
        }
        _BitScanReverse(&v24, v22 + 1);
        v25 = (v23 >> (v24 & 31)) + v23;
        *(uint32_t *)group_slot[0] = v25;
        LOBYTE(ctx16) = (::cands + 1)[0];
        if ( ctx_count[(::cands + 1)[0]] )
        {
          if ( (uint8_t)ctx_count[(::cands + 1)[0]] > 1u )
          {
            if ( (uint8_t)ctx_count[(::cands + 1)[0]] > 2u )
            {
              if ( (uint8_t)ctx_count[(::cands + 1)[0]] > 3u )
              {
                if ( (uint8_t)ctx_count[(::cands + 1)[0]] > 5u )
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
      len_sum0 = ::len_total;
      v30 = 4 * ::len_total;
      if ( (uint32_t)lit_cost8 < 0x44C )
      {
        v30 = 6 * ::len_total;
        if ( (uint32_t)lit_cost8 < 0x384 )
          v30 = 9 * ::len_total;
      }
      _BitScanReverse(&v31, v8);
      v32 = v31 + 6;
      v33 = ((uint64_t)(uint32_t)((n_cands + 4 * search_gain + ((uint32_t)lit_cost8 >> 6))
                                            * (uint64_t)v30
                                            / (((uint32_t)(len_avg + 1935 - lit_cost8) >> 3) + 1)) << ((v32 & 0x1F) & 31))
          / v8;
      v34 = v70;
      v35 = (::groups - 2);
      do
      {
        v35 += 2;
        v36 = search_gain + ((v33 * (uint64_t)(uint32_t)*v35) >> (v32 & 31));
        ::len_total += v36;
        v37 = v35[1];
        v38 = (uint32_t *)&::len_model[2 * v37];
        group_slot[v37] -= (int32_t)v35;
        v39 = len_model[2 * v37];
        *v35 = v39;
        *v38 = v39 + v36;
        --v34;
      }
      while ( v34 );
    }
    if ( n255 >= 0xFF )
    {
      if ( encoding )
      {
        v53 = (uint8_t *)cur_ptr;
        v54 = v75;
        n255_3 = n255;
        v56 = 0;
        while ((uint8_t)(*v54) == (uint8_t)(*v53))
        {
          ++v53;
          ++v54;
          if ( !--n255_3 )
          {
            *excluded = -1;
            n255_4 = n255;
            goto LABEL_76;
          }
        }
        LOBYTE(v56) = *v54;
        *excluded = v56;
        n255_4 = n255 - n255_3;
LABEL_76:
        v77 = code_symbol(::len_total, (uint32_t *)code_pos, n255_4, (uint32_t *)len_model);
      }
      else
      {
        v58 = code_symbol(::len_total, (uint32_t *)code_pos, -1, (uint32_t *)len_model);
        v77 = v58;
        v59 = v75;
        n255_5 = n255;
        v61 = (uint8_t *)cur_ptr;
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
            *excluded = -1;
            goto LABEL_82;
          }
        }
        *excluded = v62;
      }
    }
    else
    {
      n256_1 = ::len_total;
      v41 = (uint32_t *)&::len_model[2 * n255 + 2];
      v42 = 255 - n255;
      do
      {
        n256_1 -= *v41;
        v41 += 2;
        --v42;
      }
      while ( v42 );
      n256_2 = n256_1;
      if ( encoding )
      {
        v43 = (uint8_t *)cur_ptr;
        v44 = v75;
        n255_6 = n255;
        v46 = 0;
        while ((uint8_t)(*v44) == (uint8_t)(*v43))
        {
          ++v43;
          ++v44;
          if ( !--n255_6 )
          {
            *excluded = -1;
            n255_7 = n255;
            goto LABEL_64;
          }
        }
        LOBYTE(v46) = *v44;
        *excluded = v46;
        n255_7 = n255 - n255_6;
LABEL_64:
        v77 = code_symbol(n256_2, (uint32_t *)code_pos, n255_7, (uint32_t *)len_model);
      }
      else
      {
        v48 = code_symbol(n256_1, (uint32_t *)code_pos, -1, (uint32_t *)len_model);
        v77 = v48;
        v49 = v75;
        n255_8 = n255;
        v51 = (uint8_t *)cur_ptr;
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
            *excluded = -1;
            goto LABEL_82;
          }
        }
        *excluded = v52;
      }
    }
LABEL_82:
    len_avg = (uint32_t)((v77 << 7) + 63 * len_avg) >> 6;
    v63 = v70;
    if ( v70 )
    {
      v64 = groups;
      ::len_total = len_sum0;
      do
      {
        len_model[2 * v64[1]] = *v64;
        v64 += 2;
        --v63;
      }
      while ( v63 );
    }
    ::len_total -= len_model[0];
    ::len_total += v73;
    len_model[0] = v73;
    result = v77;
    LOWORD(len_model[2 * v77]) += 5;
    rpos = len_ring_pos;
    len_ring_pos = ((uint16_t)len_ring_pos + 1) & 0xFFF;
    v66 = (int8_t *)&len_ring[rpos];
    v67 = *(uint32_t *)v66;
    *(uint32_t *)v66 = v77;
    if ( v67 < 0 )
    {
      ::len_total += 5;
      return v77;
    }
    else
    {
      LOWORD(len_model[2 * v67]) -= 5;
    }
  }
  else
  {
    *excluded = -1;
    search_gain = 0;
    literal_run = 0;
    len_avg = 0;
    return 0;
  }
  return result;
}

// search (vbn.exe 0x10005989, search_tree; 0x100053E2, search_excluding):
// find the candidates for search_pos, the earlier positions with the same
// context.  The descent goes down the context tree from its root along the
// data before search_pos, read backwards a bit at a time (a 1 bit takes slot
// 0, a 0 bit slot 1), recording the slots in search_path, to the first slot
// that holds a leaf -- or, when the data runs out first, to slot 0 of the
// node reached -- and keeps where it stopped in found_at, found_slot and
// found_leaf.  Hex-Rays printed it unrolled, eight bits to a byte, as carry
// and overflow arithmetic (see tree_rebuild).  Then the search walks back up:
// at each level above search_avg, the other slot's leaves become candidates
// (walk_cands), weighted by how far the level is above it.  max_cands
// candidates end the search at once (search_exit, walk_exit).  search_tree
// gives up after the descent when lit_cost9 is above 0xFFE; search_excluding
// (`excluding`: after a failed match) skips the candidates that predict
// excluded_byte.
static void search(bool excluding)
{
  // search_pos >= 0.  search_excluding tests search_pos < 0 for no data
  // rather than == 0; it only runs after a match, at a position > 0.
  int32_t *path = search_path, *node = tree, *slot, leaf;
  int32_t bitpos = search_pos;
  const uint8_t *p = (const uint8_t *)&window[(uint32_t)bitpos >> 3];
  for (; bitpos > 0; bitpos -= 8)
  {
    uint8_t byte = *--p;
    for (int32_t k = 1; k <= 8; k++, byte <<= 1)
    {
      slot = &node[1 - (byte >> 7)];
      *path = (int32_t)(uintptr_t)slot;
      leaf = *slot;
      if ( leaf <= 0 )
      {
        bitpos -= k;
        goto found;
      }
      path++;
      node = (int32_t *)(uintptr_t)leaf;
    }
  }
  slot = node;                                    // out of data
  leaf = node[0];
  *path = (int32_t)(uintptr_t)node;
found:
  found_at = bitpos;
  found_slot = (int32_t)(uintptr_t)slot;
  found_leaf = leaf;
  // When the other slot holds the same leaf, as a new node's two do, the walk
  // starts a level higher.
  if ( leaf == *(int32_t *)((uintptr_t)slot ^ 4) && path != search_path )
    path--;

  uint32_t depth = path - search_path + 1;
  search_depth = depth;
  search_gain = 0;
  if ( !excluding && (uint32_t)lit_cost9 > 0xFFE )
    search_exit();
  if ( (int32_t)depth > search_avg )
  {
    uint32_t nbits;
    _BitScanReverse(&nbits, tree_nodes + 7 * tree_updates);
    search_gain = 16 * (depth - search_avg) / nbits;
    search_avg = ((depth >> 3) + depth + 2 * search_avg) >> 2;
  }
  if ( leaf <= 0
    && (uint32_t)search_avg < (uint32_t)search_depth
    && !(excluding && predicts_excluded(leaf)) )
  {
    cands[0] = (search_depth - search_avg) << weight_shift--;
    cands[1] = leaf;
    n_cands++;
  }
  while ( (uint32_t)search_depth > (uint32_t)search_avg )
  {
    int32_t other = *(int32_t *)(uintptr_t)(*path-- ^ 4);
    if ( other > 0 )
    {
      int32_t weight = search_depth - search_avg;
      if ( weight_shift )
        weight <<= weight_shift-- >> 1;
      Walk w = { &cands[2 * n_cands], max_cands - n_cands, weight, excluding };
      walk_cands((int32_t *)(uintptr_t)other, &w);
      n_cands = (w.out - cands) / 2;
    }
    else if ( !(excluding && predicts_excluded(other)) )
    {
      cands[2 * n_cands] = (search_depth - search_avg) << weight_shift;
      cands[2 * n_cands + 1] = other;
      if ( ++n_cands == max_cands )
        search_exit();
    }
    search_depth--;
  }
}

// run_search: the search, however it ends.  Not inlined, so that code_window's
// locals stay out of what longjmp returns to.
static __attribute__((noinline)) void run_search(bool excluding)
{
  if (!setjmp(search_jmp))
    search(excluding);
}

// read_input (vbn.exe 0x10000D0E): read the coder's input -- the data when
// compressing, the code (after the stream's size) when decompressing.  vbn's
// reader read the archive's title and then each file, or the archive's
// volumes.  A short read leaves the rest of the buffer as it was, as vbn's did
// at the end of its data.
static FILE *vbn_in, *vbn_out;
static const char *vbn_in_name, *vbn_out_name;
static uint32_t vbn_out_left;           // decompressing: the bytes of data still to write
[[noreturn]] static void vbn_fail(const char *what, const char *name);

static void read_input(int8_t *to, uint32_t count)
{
  if (fread(to, 1, count, vbn_in) < count && ferror(vbn_in))
    vbn_fail("cannot read", vbn_in_name);
}

// write_output (vbn.exe 0x100012A8): write the coder's output -- the code when
// compressing, the data when decompressing.  vbn's writer split it into
// archive volumes, or into the title and the files, and stopped at the end of
// the last file: the data written never runs past the stream's size.
static void write_output(int8_t *src, uint32_t count)
{
  if (!encoding) {
    if (count > vbn_out_left)
      count = vbn_out_left;
    vbn_out_left -= count;
  }
  if (fwrite(src, 1, count, vbn_out) != count)
    vbn_fail("cannot write", vbn_out_name);
}

// code_buffer_io (vbn.exe 0x10004924): 8 KB of code are done.  Compressing,
// write them out and move the 16 bytes after them to the start; decompressing,
// move those 16 bytes to the start and read the next 8 KB after them.
// Returns the bit position, 8 KB back.
static uint32_t code_buffer_io(uint32_t code_pos)
{
  if ( encoding )
  {
    write_output(code_buf, 0x2000);
    memcpy(code_buf, code_buf + 0x2000, 16);
    memset(code_buf + 16, 0, 0x2000);
  }
  else
  {
    memcpy(code_buf, code_buf + 0x2000, 16);
    read_input(code_buf + 16, 0x2000);
  }
  return code_pos - 0x10000;
}

// code_window (vbn.exe 0x100056C4): code the window from bitpos to its end.  At
// each position: search for candidates (excluding the byte a failed match
// predicted) and code a match length; on a match, register some of its
// positions in the tree and skip it, else code a literal and register the
// position.  Every 8 KB of code goes through code_buffer_io.  Returns the bit
// position reached.
static uint32_t code_window(uint32_t *code_pos_p, uint32_t bitpos)
{
  uint32_t code_pos = *code_pos_p, start = bitpos;
  int32_t excluded = -1;
  while ( bitpos < (uint32_t)window_bits )
  {
    excluded_byte = excluded;
    search_pos = bitpos;
    search_depth = 0;
    n_cands = 0;
    cand_floor = 0;
    weight_shift = 4;
    uint32_t nbits;
    _BitScanReverse(&nbits, tree_updates + 4);
    search_avg = (276 * nbits) >> 9;
    run_search(excluded >= 0);
    if ( n_cands > 1 )
      cand_floor = (uint32_t)cands[2 * n_cands - 2] >> 2;
    // Of the candidates with the top weight, the latest goes first.
    for ( int32_t *cand = cands + 2, n = n_cands - 1; n > 0 && cand[0] == cands[0]; cand += 2, n-- )
      if ( cands[1] >= cand[1] )
        std::swap(cands[1], cand[1]);

    uint32_t len = code_match(&code_pos, &excluded, bitpos);
    if ( len )
    {
      next_byte[prev_byte] = window[bitpos >> 3];
      next_byte2[prev_byte2] = window[bitpos >> 3];
      if ( excluded >= 0 || (uint32_t)search_gain < 0x2A )
        tree_insert(len, bitpos);
      bitpos += 8 * len;
    }
    else
    {
      int8_t byte = code_literal(excluded, &code_pos, bitpos);
      next_byte[prev_byte] = byte;
      next_byte2[prev_byte2] = byte;
      tree_insert(1, bitpos);                     // vbn has this one inline
      excluded = -1;
      bitpos += 8;
    }
    int32_t at = bitpos >> 3;
    prev_byte = (uint8_t)window[at - 1];
    prev_byte2 = at == 1 ? prev_byte : (uint8_t)window[at - 2];
    if ( code_pos >= 0x10000 )
      code_pos = code_buffer_io(code_pos);
  }
  *code_pos_p = code_pos;
  total_bits += bitpos - start;
  return bitpos;
}

// tree_rebuild (vbn.exe 0x100050F8): register the window's positions in an
// empty tree again, after the window slid.  Hex-Rays printed its eight-way
// unrolled descent as carry/overflow arithmetic: per bit, `add cl,cl; sbb r,r;
// adc r,[node+r*4+4]; jle`.  sbb leaves CF (the bit) alone, so adc loads
// [node] for a 1 bit and [node+4] for a 0 bit, carries the bit back out and
// never overflows: jle is a plain `child <= 0`.  It is tree_insert's descent:
//
// For every position pos = 8, 16, ... below window_pos whose bit pos/8 is set
// in `registered`:
//  - descend from the root reading the data bits backwards from byte pos/8-1
//    (bit 1 -> child [0], bit 0 -> child [1]) to the first slot holding a
//    value <= 0, or to node[0] of the last node when the bits run out;
//  - depth = pos - (bit position where the descent stopped); then
//      depth <= 64*bsr(tree_nodes+32): an empty leaf becomes a new node from
//          node_next, both children -pos; the sibling slot being a node bumps
//          tree_updates;
//      deeper: a leaf older than 0x800 is refreshed to -pos, and bumps it too.
// Returns the first position >= window_pos.
static uint32_t tree_rebuild(void)
{
  uint32_t pos = 0;                                     // EAX
  for (;;) {
    pos += 8;
    if (pos >= window_pos)
      return pos;
    if (!_bittest(((const void *)::registered), (int32_t)(pos >> 3)))
      continue;
    const uint8_t *p = ((const uint8_t *)window) + (pos >> 3) - 1;     // EDI
    int32_t at = (int32_t)pos;                          // ESI
    int32_t *node = ((int32_t *)tree), *slot;                  // EBX/EDX alternate in the asm
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
    uint32_t thr = (uint32_t)(31 - __builtin_clz((uint32_t)tree_nodes + 32)) << 6;
    if (depth > thr) {
      if (leaf <= 0 && pos + (uint32_t)leaf >= 0x800) {
        *slot = -(int32_t)pos;
        tree_updates++;
      }
    } else if (leaf <= 0) {
      int32_t *n = (int32_t *)(uintptr_t)node_next;
      n[0] = -(int32_t)pos;
      n[1] = -(int32_t)pos;
      *slot = (int32_t)(uintptr_t)n;
      node_next += 8;
      tree_nodes++;
      slot = (int32_t *)((uintptr_t)slot ^ 4);
      if (*slot > 0)
        tree_updates++;
    }
  }
}

// coder_run (vbn.exe 0x10004A5F): compress or decompress `size` bytes, as
// `encoding` says.  It sets up the models, codes the window, and slides it by
// 0x66000 bytes -- re-registering the positions it keeps -- until all the data
// is coded; then it ends the code.  The data comes from read_input and goes to
// write_output.
static void coder_run(uint32_t size)
{
  // The models.
  lit_total = 2048;
  for ( uint32_t i = 0; i < 256; i++ )
  {
    lit_model[2 * i] = 8;
    lit_model[2 * i + 1] = i;
    lit_slot[i] = (int32_t)(uintptr_t)&lit_model[2 * i];
  }
  len_total = 256;
  for ( uint32_t i = 0; i < 256; i++ )
  {
    len_model[2 * i] = 1;
    len_model[2 * i + 1] = i;
  }
  memset(lit_ring1, 0xFF, sizeof lit_ring1);
  memset(lit_ring2, 0xFF, sizeof lit_ring2);
  memset(len_ring, 0xFF, sizeof len_ring);
  memset(ctx_count, 0, sizeof ctx_count);
  lit_ring1_pos = lit_ring2_pos = len_ring_pos = 0;
  for ( int32_t len = 0; len < 23; len++ )
    for ( int32_t n = len_init_counts[len]; n; n-- )
      len_model_add(len);
  memset(group_slot, 0, sizeof group_slot);
  lit_cost8 = 1280;
  lit_cost9 = 3072;
  lit_cost10 = 7168;
  lit_cost11 = 0x4000;
  len_avg = 0;
  lit_resort = 1;
  lit_inc = 8;
  memset(next_byte, 0, sizeof next_byte);
  memset(next_byte2, 0, sizeof next_byte2);
  prev_byte = prev_byte2 = literal_run = lit_threshold = 0;

  // The coder, the first window and the tree.
  code_low = code_pending = 0;
  code_high = -1;
  total_bits = 0;
  uint32_t n = size < 0x198000 ? size : 0x198000;
  window_bits = 8 * n;
  data_end = (int32_t)(uintptr_t)&window[n];
  if ( encoding )
  {
    read_input(window, n);
    memset(code_buf, 0, sizeof code_buf);
  }
  else
    read_input(code_buf, 2 * n < 0x2010 ? 2 * n : 0x2010);
  memset(registered, 0, sizeof registered);
  tree[0] = tree[1] = 0;
  node_next = (int32_t)(uintptr_t)&tree[2];
  tree_nodes = 1;
  tree_updates = 1;
  window_pos = 0;

  uint32_t code_pos = 0;
  for ( ;; )
  {
    if ( window_pos != (uint32_t)window_bits )
      window_pos = code_window(&code_pos, window_pos);
    if ( total_bits >> 3 >= size )
      break;
    // Slide the window by 0x66000 bytes, writing them out when decompressing.
    if ( !encoding )
      write_output(window, 0x66000);
    memmove(window, window + 0x66000, 0x132000);
    memmove(registered, (int8_t *)registered + 0xCC00, 0x26400);
    memset((int8_t *)registered + 0x26400, 0, 0xCC00);
    tree[0] = tree[1] = 0;
    node_next = (int32_t)(uintptr_t)&tree[2];
    tree_nodes = 1;
    window_pos = 8 * 0x132000;
    tree_rebuild();
    if ( encoding )
      read_input(window + 0x132000, 0x66000);
    uint32_t left = size - (total_bits >> 3);
    if ( left < 0x66000 )
    {
      window_bits = 8 * (0x132000 + left);
      data_end = (int32_t)(uintptr_t)&window[0x132000 + left];
    }
  }
  code_pos = code_flush(code_pos);
  if ( encoding )
    write_output(code_buf, (code_pos + 7) >> 3);
  else
    write_output(window, window_pos >> 3);
}

// ===========================================================================
// main: vbn2 c|d input output.  A compressed stream is the input's size (4
// bytes, little-endian) and then vbn's code for it, in vbn's MAX mode (`u`):
// the archive around it -- header, title, volumes -- is gone.  The coder is
// vbn's, run once, as vbn ran it for an archive.
// ===========================================================================
static const uint32_t MAX_SIZE = 0x1FFFFFFF;    // the coder counts the data in bits, in 32 bits

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
  encoding = argv[1][0] == 'c';                 // 1 compressing, 0 decompressing
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
  uint32_t vbn_size;
  if (encoding) {
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
  if (encoding && fwrite(header, 1, 4, vbn_out) != 4)
    vbn_fail("cannot write", vbn_out_name);

  max_cands = 95;                               // MAX mode (`vbn u`; `b` is 55, `B` 31)

  if ((int32_t)(uintptr_t)(tree + sizeof tree / sizeof *tree) < 0) {   // see vbn's globals
    errno = 0;
    vbn_fail("cannot run:", "vbn's tree lies above 0x80000000");
  }
  coder_run(vbn_size);

  errno = 0;
  if (!encoding && vbn_out_left)
    vbn_fail("not a vbn2 stream (data missing):", vbn_in_name);
  if (fclose(vbn_out) != 0) {
    vbn_out = NULL;                               // closed, if not written
    remove(vbn_out_name);
    vbn_fail("cannot write", vbn_out_name);
  }
  fclose(vbn_in);
  return 0;
}

