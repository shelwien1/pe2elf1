// vbn2.cpp: VBN 1.001's compressor, standalone, on one file.
//
//   g++ -O2 -o vbn2 vbn2.cpp   (or -m32)
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
// program.  This file started as that program; it has been cut down to the
// coder and written out by hand since, each function headed by the vbn.exe
// function it is.

#define _FILE_OFFSET_BITS 64        // stat() with -m32 fails on 64-bit inode numbers otherwise
#include <cerrno>
#include <csetjmp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <utility>

// ===========================================================================
// Bits.
// ===========================================================================
// bsr: the index of v's highest set bit.  v is never 0 where vbn2 asks.
static inline uint32_t top_bit(uint32_t v) { return 31 - __builtin_clz(v); }
// bt and btc on a bit string: bit n of buf.
static inline uint32_t get_bit(const void *buf, uint32_t n)
{
  return ((const uint8_t *)buf)[n >> 3] >> (n & 7) & 1;
}
static inline void flip_bit(void *buf, uint32_t n)
{
  ((uint8_t *)buf)[n >> 3] ^= 1 << (n & 7);
}

// ===========================================================================
// vbn's globals, each with storage of its own.  The comment after a name gives
// the address vbn.exe kept it at.  Sizes are what the code reaches.
// ===========================================================================
// The direction, and the mode.
static int32_t encoding;                          // 0x1000D4C4 1 compressing, 0 decompressing
static int32_t max_cands;                         // 0x1000D4B0 how many candidates a search returns: 95 (MAX)
static const int32_t mode_shift[3] = { 10, 6, 4 }; // 0x1000CA54 by mode, max_cands >> 5

// The data: a window of 0x198000 bytes that slides by 0x66000.  Positions
// are bit positions, 8 per byte.
static int8_t window[0x198000];                   // 0x100166F0
static uint32_t window_pos;                       // 0x1001646C where coding is, in bits
static int32_t window_bits;                       // 0x10016444 where the data in the window ends, in bits
static int8_t *data_end;                          // 0x1001643C ... and as an address
static uint32_t total_bits;                       // 0x10016468 the data coded before the window, in bits
static int32_t prev_byte;                         // 0x100164B0 the byte before the one being coded
static int32_t prev_byte2;                        // 0x100164B4 ... and the one before that
static int8_t next_byte[256];                     // 0x1001620C per byte: the byte that followed it last
static int8_t next_byte2[256];                    // 0x1001630C per byte: the byte that came two after it last

// The context tree: a binary tree over the data read backwards from each
// position, one bit per level.  Its nodes live in an arena, numbered in the
// order they were made: node n is the two slots tree[2n] and tree[2n+1], and
// node 0 is the root.  A slot holding a value > 0 points to the node of that
// number; a value <= 0 is a leaf, minus the position it registers.  A window
// registers at most 0x198000 positions, each making at most one node, and the
// tree starts over when the window slides: the arena holds 0x198001 nodes.
// (vbn.exe kept nodes' addresses in the slots, and the next node's address
// at 0x10016430, always node tree_nodes.)
static int32_t tree[2 * (1 + 0x198000)];          // 0x101AE6F0 the arena, 12.75 MiB
static int32_t tree_nodes;                        // 0x1000CA4C nodes in use, the root too: the next one made
static int32_t tree_updates;                      // 0x10016434 counts changes to the tree
static int32_t registered[0x33000 / 4];           // 0x10E6E6F0 a bit per position in the window: in the tree

// The search: from the position being coded, down the tree, collecting
// earlier positions with the same context -- the candidates -- and a
// weight for each.
static int32_t cands[2 * 95];                     // 0x1000D558 {weight, -position} pairs
static int32_t n_cands;                           // 0x10016450 how many
static int32_t search_pos;                        // 0x1001644C where the search starts, in bits
static int32_t *search_path[1284];                // 0x1000D850 the slots the descent went through
static int32_t search_depth;                      // 0x10016454 the depth the walk back up has reached
static int32_t search_avg;                        // 0x10016438 the depth a candidate has to beat
static int32_t search_gain;                       // 0x1001648C how far the last search beat it
static int32_t weight_shift;                      // 0x10016448 the next candidate's weight shift (4 down to 0)
static int32_t cand_floor;                        // 0x100164C4 a quarter of the weakest candidate's weight
static int32_t excluded_byte;                     // 0x10016488 the byte a failed match predicted, or -1
static int32_t found_at;                          // 0x1001647C where the last descent stopped: the bit position,
static int32_t *found_slot;                       // 0x10016480 ... the slot,
static int32_t found_leaf;                        // 0x10016484 ... and what the slot held

// Candidates grouped by the value they predict: a match length in
// code_match, a byte in code_literal.
static uint32_t *group_slot[256];                 // 0x1000EC60 per value: its {weight, value} pair in groups, or null
static uint32_t groups[2 * 256];                  // 0x1000F05C

// The match length model: a count for each length 0..255 (0: no match),
// following the last 4096 lengths coded.
static uint32_t len_model[2 * 256];               // 0x1000F860 {count, length} pairs, by length
static uint32_t len_total;                        // 0x100164C0 the sum of the counts
static const int32_t len_init_counts[23] = {     // 0x1000C97C the counts lengths 0..22 start with
  2662, 633, 321, 185, 77, 40, 24, 22, 18, 15, 13, 10, 7, 5, 3, 3, 2, 2, 2, 1, 1, 1, 1
};
static int32_t len_ring[4096];                    // 0x1001220C the last 4096 lengths
static int32_t len_ring_pos;                      // 0x10016420
static int32_t len_avg;                           // 0x100164A8 the average length coded, x128

// The literal model: a count for each byte, kept sorted by count, largest
// first; the last 363 literals add 16 to theirs.
static uint32_t lit_model[2 * 256];               // 0x1000F860 + 0x800 {count, byte} pairs
static uint32_t *lit_slot[256];                   // 0x10010060 per byte: its pair in lit_model
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
static int32_t lit_cost8;                         // 0x100164A4 the bits a literal costs, x2^8, over about 2^8 literals;
static int32_t lit_cost9;                         // 0x100164A0 ... x2^9 over 2^9,
static int32_t lit_cost10;                        // 0x1001649C ... x2^10 over 2^10,
static int32_t lit_cost11;                        // 0x10016498 ... and x2^11 over 2^11

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
// bit `pos` on, the shortest tail -- a 0 and then 1s, or a 1 and then 0s --
// that keeps every continuation inside the interval, resets the interval, and
// returns the bit position after the tail.  low's top bit is 0 and high's is
// 1: a 0 and then 1s stays above low once its 1s outlast the 1s after low's
// top bit, and a 1 and then 0s stays below high once its 0s outlast the 0s
// after high's.  Neither run is counted past code_pending bits.
static uint32_t code_flush(uint32_t pos)
{
  uint32_t low = (uint32_t)code_low << 1, high = (uint32_t)code_high << 1;
  uint32_t n_low = 1, n_high = 1;
  for (int32_t k = code_pending; k > 0; k--) {
    n_low++;
    if ((int32_t)low >= 0)
      break;
    low <<= 1;
  }
  for (int32_t k = code_pending; k > 0; k--) {
    n_high++;
    if ((int32_t)high < 0)
      break;
    high <<= 1;
  }
  uint32_t tail = 0x7FFFFFFF, n = n_low;
  if (n_low > n_high) {
    tail = 0x80000000;
    n = n_high;
  }
  for (; n; n--) {
    if ((int32_t)tail < 0)
      flip_bit(code_buf, pos);
    pos++;
    tail <<= 1;
    code_pending--;
  }
  code_low = 0;
  code_high = -1;
  return pos;
}

// ring_put: put v in a ring of n values at *pos, and return the value it
// replaces -- the one put n values ago, or -1.
static int32_t ring_put(int32_t *ring, int32_t *pos, int32_t n, int32_t v)
{
  int32_t old = ring[*pos];
  ring[*pos] = v;
  *pos = *pos + 1 == n ? 0 : *pos + 1;
  return old;
}

// len_model_add (vbn.exe 0x100050B0): count a length in the length model.
// Its count goes up by 5 and the count of the length coded 4096 lengths ago
// (from len_ring) down by 5; until the ring is full, len_total grows by 5
// instead.  The counts are added to and taken from in their low 16 bits only
// (`add word ptr`).  Returns that old length, or -1.
static void add_low16(uint32_t *x, int32_t d) { *x = (*x & 0xFFFF0000) | (uint16_t)(*x + d); }
static int32_t len_model_add(int32_t len)
{
  add_low16(&len_model[2 * len], 5);
  int32_t old = ring_put(len_ring, &len_ring_pos, 4096, len);
  if (old >= 0)
    add_low16(&len_model[2 * old], -5);
  else
    len_total += 5;
  return old;
}

// other_slot: the other slot of the node `slot` is in.  (vbn.exe flips bit 2
// of the slot's address.)
static inline int32_t *other_slot(int32_t *slot)
{
  return tree + ((slot - tree) ^ 1);
}

// descend: from the root, down the context tree along the data before bit
// position pos (> 0), read backwards a bit at a time -- a 1 bit takes slot 0,
// a 0 bit slot 1 -- to the first slot that holds a leaf, or, when the data
// runs out, to slot 0 of the node reached.  Returns the slot, and in *at the
// bit position the descent stopped at.  (tree_insert's and tree_rebuild's;
// search records its path as well.)
static int32_t *descend(uint32_t pos, int32_t *at)
{
  int32_t *node = tree, left = pos;
  const uint8_t *p = (const uint8_t *)window + (pos >> 3);
  for (; left > 0; left -= 8) {
    uint8_t byte = *--p;
    for (int32_t k = 1; k <= 8; k++, byte <<= 1) {
      int32_t *slot = &node[1 - (byte >> 7)];
      if (*slot <= 0) {
        *at = left - k;
        return slot;
      }
      node = &tree[2 * *slot];
    }
  }
  *at = left;
  return node;
}

// tree_add: register position pos (a bit position, whole bytes) at the slot
// a descent stopped at, `at` holding the bit position it stopped at and
// `leaf` what the slot held.  Within 64*bsr(tree_nodes + 32) bits of the
// root, an empty leaf becomes a node of two leaves for pos, and a node in the
// other slot counts as a change; deeper, only a leaf more than 0x800 bits
// old is renewed to pos, which counts as a change too.  tree_insert also
// flips the position's bit in `registered`.
static void tree_add(uint32_t pos, int32_t *slot, int32_t leaf, uint32_t at, bool flip)
{
  if (pos - at > top_bit(tree_nodes + 32) << 6) {
    if (leaf <= 0 && pos + (uint32_t)leaf >= 0x800) {
      if (flip)
        flip_bit(registered, pos >> 3);
      *slot = -(int32_t)pos;
      tree_updates++;
    }
  } else if (leaf <= 0) {
    if (flip)
      flip_bit(registered, pos >> 3);
    int32_t n = tree_nodes++;
    tree[2 * n] = tree[2 * n + 1] = -(int32_t)pos;
    *slot = n;
    if (*other_slot(slot) > 0)
      tree_updates++;
  }
}

// tree_insert (vbn.exe 0x10005251): register the positions of a match, from
// bitpos on: as many as bsr(count + count/2 + 1) + 1, and no more than the
// count.  The first goes where the search's descent stopped (found_at,
// found_slot, found_leaf); the others descend for themselves.  In vbn.exe it
// is assembly that keeps the bit position in EBP, which Hex-Rays read as the
// frame pointer.
static void tree_insert(uint32_t count, uint32_t bitpos)
{
  uint32_t n = top_bit(count + (count >> 1) + 1) + 1;
  if (n > count)
    n = count;
  uint32_t pos = bitpos, at = found_at;
  int32_t *slot = found_slot, leaf = found_leaf;
  for (;;) {
    tree_add(pos, slot, leaf, at, true);
    if (--n == 0)
      return;
    pos += 8;
    int32_t stop;
    slot = descend(pos, &stop);
    leaf = *slot;
    at = stop;
  }
}

// walk_cands (vbn.exe 0x10006CB0; 0x10006E31 when excluding): add the leaves
// of node n's subtree to the candidates, left to right, as {weight, leaf}
// pairs.  A node's right leaf counts only when its left child is a node (vbn
// compares that slot with the node's address, here with its number: a node's
// children are made after it).  Excluding, a leaf that predicts excluded_byte
// is skipped.  In vbn.exe the walks are assembly with a register interface
// (EBX node, ESI weight, ECX output cursor, EDI how many pairs may still be
// written; ECX and EDI come back); here the caller passes a Walk.  When no
// more pairs may be written, the search ends (walk_exit).
struct Walk {
  int32_t *out;
  int32_t left;
  int32_t weight;
  bool excluding;
};

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
static void walk_cands(int32_t n, Walk *w)
{
  int32_t *node = &tree[2 * n];
  if (node[0] > 0)
    walk_cands(node[0], w);
  else
    walk_emit(node[0], w);
  if (node[1] > 0)
    walk_cands(node[1], w);
  else if (node[0] > n)                           // cmp eax,[eax]; jle
    walk_emit(node[1], w);
}

// code_symbol (vbn.exe 0x100061F4): code one symbol with a frequency list of
// {count, symbol} pairs whose counts sum to `total`.  Compressing (symbol >= 0),
// the interval narrows to the symbol's share and the bits it settles go to
// code_buf at *code_pos; decompressing (symbol < 0), the symbol is the one
// whose share holds the code read so far.  Returns the symbol, and moves
// *code_pos past the bits.
//
// The interval is [code_low, code_high]; its size, high - low + 1, is 2^32
// after code_flush, which vbn.exe's 32-bit arithmetic treats as a case of its
// own and a 64-bit size does not need to.  Decompressing, code_value holds
// the 32 bits of code that line up with code_low; the ones past code_pending
// arrive with the next symbol.
static int32_t code_symbol(uint32_t total, uint32_t *code_pos, int32_t symbol, uint32_t *model)
{
  uint32_t pos = *code_pos;
  uint32_t range = (uint32_t)code_high - (uint32_t)code_low;
  if (2 * total > range) {
    pos = code_flush(pos);
    range = 0xFFFFFFFF;
  }
  uint64_t size = (uint64_t)range + 1;
  uint32_t *entry, below = 0;                     // the symbol's pair, the counts before it
  if (symbol >= 0) {
    for (entry = model; entry[1] != (uint32_t)symbol; entry += 2)
      below += entry[0];
  } else {
    uint32_t value = code_value, at = pos + code_pending;
    for (int32_t n = 32 - code_pending; n > 0; n--)
      value = 2 * value + get_bit(code_buf, at++);
    code_value = value;
    code_pending = 0;
    // The count the code points at: (value - low + 1) * total - 1, over the
    // size.  vbn subtracts the 1 with `sbb eax,1` straight after the `mul`,
    // so the mul's carry -- the product needing 64 bits -- comes off too
    // (and with a product of total << 32, the borrow of value - low).
    uint32_t offset = value - (uint32_t)code_low;
    uint64_t t;
    if (offset + 1 == 0)
      t = ((uint64_t)total << 32) - 1 - (value < (uint32_t)code_low);
    else {
      t = (uint64_t)total * (offset + 1);
      t -= 1 + (t >> 32 != 0);
    }
    uint32_t target = (uint32_t)(t / size);
    for (entry = model; below + entry[0] <= target; entry += 2)
      below += entry[0];
  }
  uint32_t above = below + entry[0];
  code_high = (uint32_t)code_low + (uint32_t)(above * size / total) - 1;
  code_low = (uint32_t)code_low + (uint32_t)(below * size / total);

  // Shift out the leading bits low and high agree on, into code_buf (a 1 is
  // written by toggling a 0).  vbn toggles them when decompressing too, which
  // does no harm: they lie before the 32 bits code_value holds when it is
  // next used.  code_flush's tail is the same.
  int32_t pending = 32;
  uint32_t low = code_low, high = code_high, agree = low ^ high;
  if ((int32_t)agree >= 0) {
    do {
      if ((int32_t)low < 0)
        flip_bit(code_buf, pos);
      pending--;
      pos++;
      low <<= 1;
      high = high << 1 | 1;
      agree <<= 1;
    } while ((int32_t)agree >= 0);
    code_low = low;
    code_high = high;
  }
  code_pending = pending;
  *code_pos = pos;
  return entry[1];
}

// pair_bonus: what the first candidate for a byte, of weight w, gets added,
// by how often the pair (byte before, byte) came up in the last 1024
// literals (`seen`, at most 255).
static uint32_t pair_bonus(uint32_t w, uint32_t seen)
{
  if ((uint32_t)search_gain < 0x13)
    switch (seen) {
      case 0: return w >> 3;
      case 1: return 2 * w;
      case 2: return 3 * w;
      case 3: return 4 * w;
      case 4:
      case 5:
      case 6: return 5 * w;
      default: return 8 * w;
    }
  switch (seen) {
    case 0: return w >> 4;
    case 1: return w >> 1;
    case 2: return w;
    case 3: return w + (w >> 1);
    case 4: return 2 * w;
    default: return 2 * w + (w >> 1);
  }
}

// lit_swap: swap two pairs of the literal model, and the slots that find them.
static void lit_swap(uint32_t *a, uint32_t *b)
{
  std::swap(a[0], b[0]);
  std::swap(a[1], b[1]);
  lit_slot[a[1]] = a;
  lit_slot[b[1]] = b;
}

// code_literal (vbn.exe 0x100064C0): code the byte at bitpos as a literal.  The
// literal model's counts are mixed with the candidates' predictions (grouped
// by the byte they predict); `excluded` is a byte a failed match already ruled
// out, or -1.  Afterwards the literal model, the rings and the cost averages
// adapt.  Returns the byte (decompressing: also stores it in the window).
static int32_t code_literal(int32_t excluded, uint32_t *code_pos, uint32_t bitpos)
{
  cand_floor = (uint32_t)cand_floor >> 1;

  // lit_threshold, how much the candidates count for: less the more bits
  // literals have been costing, by each average (1.5 a bit) and by the log
  // of lit_cost8.  At 0, with a search that gained little, the candidates are
  // dropped and the long averages start over.
  auto cost = [](int32_t c, int32_t k) { uint32_t b = ((uint32_t)c >> k) + 1; return b + (b >> 1); };
  lit_threshold = ((uint32_t)search_gain >> 4) + 54 - top_bit(((uint32_t)lit_cost8 >> 3) + 1)
                  - cost(lit_cost8, 8) - cost(lit_cost9, 9) - cost(lit_cost10, 10) - cost(lit_cost11, 11);
  if (lit_threshold <= 0) {
    lit_threshold = 0;
    if (search_gain <= 4) {
      n_cands = 0;
      lit_cost9 = 4096;
      lit_cost10 = 0x2000;
      lit_cost11 = 0x4000;
    }
  }

  // lit_inc, what every count in the literal model gets added: the more
  // literals cost, the flatter the model.  From about 7 bits on, the long
  // averages start over.
  static const uint32_t steps[7] = { 0x2EC, 0x37F, 0x489, 0x56F, 0x63F, 0x70F, 0x75C };
  static const int32_t incs[8] = { 1, 2, 4, 8, 16, 32, 128, 1024 };
  uint32_t c8 = lit_cost8, k = 0;
  while (k < 7 && c8 >= steps[k])
    k++;
  int32_t inc = incs[k];
  if (c8 >= 0x70F) {
    lit_cost9 = 4096;
    lit_cost10 = 0x2000;
    lit_cost11 = 0x4000;
  }
  if (lit_inc != inc) {
    for (int32_t b = 0; b < 256; b++)
      lit_model[2 * b] += inc - lit_inc;
    lit_total += 256 * (inc - lit_inc);
    lit_inc = inc;
  }

  // The bytes ruled out -- `excluded`, and excluded_byte, which the search
  // left out -- leave the model while this one is coded.
  uint32_t kept = 0, kept_byte = 0;
  bool exclude_byte = excluded_byte >= 0 && excluded_byte != excluded;
  if (excluded >= 0) {
    kept = *lit_slot[excluded];
    *lit_slot[excluded] = 0;
    lit_total -= kept;
  }
  if (exclude_byte) {
    kept_byte = *lit_slot[excluded_byte];
    *lit_slot[excluded_byte] = 0;
    lit_total -= kept_byte;
  }

  // The candidates, grouped by the byte they predict, each group a pair
  // {weight, byte}; none for `excluded` (and then n_cands counts one less,
  // however many there were).  A group weighs its candidates' weights over
  // cand_floor, and its first one's pair_bonus.
  uint32_t n_groups = 0, sum = 0, *group = groups;
  if (n_cands) {
    bool skipped = false;
    for (int32_t *cand = cands; cand < cands + 2 * (uint8_t)n_cands; cand += 2) {
      int32_t b = (uint8_t)cand[1];
      if (b == excluded) {
        skipped = true;
        continue;
      }
      uint32_t w = (uint32_t)cand[0] - cand_floor;
      if (group_slot[b])
        *group_slot[b] += w;
      else {
        group_slot[b] = group;
        ctx16 = (ctx16 & ~0xFF) | b;
        w += pair_bonus(cand[0], (uint8_t)ctx_count[ctx16]);
        group[0] = w;
        group[1] = b;
        group += 2;
      }
      sum += w;
    }
    n_groups = (group - groups) / 2;
    if (skipped)
      n_cands--;
  }

  // The groups join the literal model for now, their weights scaled so that
  // together they make about `share` counts: more with lit_total, with the
  // candidates that agree with another, with lit_threshold and with the
  // search's gain.  The byte next_byte predicts weighs 7/4 more, next_byte2's
  // 3/2.  While literals cost less than about 6.8 bits, a byte's count also
  // gets 64 for each time its pair came up lately, and while no pair is known
  // (literal_run 0), a quarter of itself.  The groups keep the counts they
  // replace.
  uint32_t saved_total = 0;
  if (n_groups) {
    uint32_t all = n_cands * sum;
    uint32_t x = (n_cands - n_groups) * ((3 * lit_threshold + 5 * (((uint32_t)search_gain >> 2) + search_gain)) >> 3);
    uint32_t base = ((uint64_t)lit_total * x) >> 3;
    saved_total = lit_total;
    uint32_t part = base >> mode_shift[(uint32_t)max_cands >> 5];
    uint32_t t = (uint32_t)(48 - lit_threshold) >> 3;
    int32_t up = t - 4;
    uint32_t moved = up < 0 ? part << ((4 - t) & 31) : part >> (up & 31);
    uint32_t share = moved + (base - part);
    uint32_t top = share ? top_bit(share) : moved;   // bsr edx,eax leaves EDX, `moved`, when share == 0
    uint32_t shift = 31 - top + top_bit(all);         // shld/shrd: the count is mod 32
    uint32_t scale = ((uint64_t)share << (shift & 31)) / all;
    if (uint32_t *g = group_slot[(uint8_t)next_byte[prev_byte]])
      *g = (7 * *g) >> 2;
    if (uint32_t *g = group_slot[(uint8_t)next_byte2[prev_byte2]])
      *g = (3 * *g) >> 1;
    for (uint32_t *g = groups; g < group; g += 2) {
      uint32_t add = ((uint64_t)scale * g[0]) >> (shift & 31);
      uint32_t *count = lit_slot[g[1]], old = *count;
      group_slot[g[1]] = nullptr;
      g[0] = old;
      if ((uint32_t)lit_cost8 < 0x6C9) {
        ctx16 = (ctx16 & ~0xFF) | g[1];
        if (!literal_run)
          add += old >> 2;
        add += (uint8_t)ctx_count[ctx16] << 6;
      }
      *count = old + add;
      lit_total += add;
    }
  }

  uint32_t start = *code_pos;
  int32_t byte = code_symbol(lit_total, code_pos, encoding ? (uint8_t)window[bitpos >> 3] : -1, lit_model);
  uint32_t bits = *code_pos - start;
  lit_cost8 = ((bits << 8) + ((uint32_t)lit_cost8 << 8) - lit_cost8) >> 8;
  lit_cost9 = ((bits << 9) + ((uint32_t)lit_cost9 << 9) - lit_cost9) >> 9;
  lit_cost10 = ((bits << 10) + ((uint32_t)lit_cost10 << 10) - lit_cost10) >> 10;
  lit_cost11 = ((bits << 11) + ((uint32_t)lit_cost11 << 11) - lit_cost11) >> 11;

  // The model as it was.
  if (n_groups) {
    lit_total = saved_total;
    for (uint32_t *g = groups; g < group; g += 2)
      *lit_slot[g[1]] = g[0];
  }
  if (excluded >= 0) {
    *lit_slot[excluded] = kept;
    lit_total += kept;
  }
  if (exclude_byte) {
    *lit_slot[excluded_byte] = kept_byte;
    lit_total += kept_byte;
  }

  // The pair (byte before, byte) counts once more while it is in lit_ring2,
  // up to 255; literal_run counts the literals in a row whose pair had come
  // up before.
  int32_t pair = (ctx16 & ~0xFF) | byte;
  window[bitpos >> 3] = byte;
  if (ctx_count[pair] != -1) {
    literal_run = ctx_count[pair] ? literal_run + 1 : 0;
    ctx_count[pair]++;
  }
  int32_t old = ring_put(lit_ring2, &lit_ring2_pos, 1024, pair);
  if (old >= 0 && ctx_count[old])
    ctx_count[old]--;

  // The byte counts 16 more while it is in lit_ring1.  Every lit_resort
  // literals, the two counts that changed move to their places in the order.
  *lit_slot[byte] += 16;
  bool resort = --lit_resort == 0;
  if (resort) {
    lit_resort = (uint8_t)(((uint32_t)lit_cost8 >> 8) + 1);
    for (uint32_t *p = lit_slot[byte]; p > lit_model && p[0] > p[-2]; p -= 2)
      lit_swap(p, p - 2);
  }
  old = ring_put(lit_ring1, &lit_ring1_pos, 363, byte);
  if (old < 0)
    lit_total += 16;
  else {
    *lit_slot[old] -= 16;
    if (resort)
      for (uint32_t *p = lit_slot[old]; p + 2 < lit_model + 2 * 256 && p[0] < p[2]; p += 2)
        lit_swap(p, p + 2);
  }
  return byte;
}

// code_match (vbn.exe 0x10005C50): code how many bytes at bitpos match the best
// candidate -- 0..255, 0 being no match -- with the length model, boosted by
// the candidates grouped by their own match lengths.  On a mismatch,
// *excluded gets the byte the candidate predicted (else -1).  Returns the
// length (decompressing: also copies the matched bytes into the window).
static int32_t code_match(uint32_t *code_pos, int32_t *excluded, uint32_t bitpos)
{
  uint32_t at = bitpos >> 3;
  ctx16 = at ? (uint8_t)window[at - 1] << 8 : 0;
  if (!n_cands) {
    *excluded = -1;
    search_gain = 0;
    literal_run = 0;
    len_avg = 0;
    return 0;
  }

  // Length 0, no match, counts for less while literals do well.
  uint32_t count0 = len_model[0];
  len_total -= count0;
  uint32_t k = 9 * literal_run + search_gain + ((uint32_t)lit_threshold >> 2) + 1;
  len_model[0] = 5 * count0 / (k + (k >> 4));
  len_total += len_model[0];

  // The best candidate's match: at most 255 bytes, its distance, and what is
  // left of the data.  From here on, a candidate holds the byte it predicts.
  int8_t *cur = &window[at], *match = &window[-cands[1] >> 3];
  uint32_t max_len = 255;
  if ((uint32_t)(cur - match) < 0xFF)
    max_len = cur - match;
  if ((uint32_t)(data_end - cur) < max_len)
    max_len = data_end - cur;
  cands[1] = (uint8_t)*match;
  int32_t agree = 0;                              // it is next_byte's byte (1), next_byte's twice (2)
  if ((uint8_t)*match == (uint8_t)next_byte[prev_byte])
    agree = (uint8_t)*match == (uint8_t)next_byte[prev_byte2] ? 2 : 1;

  // The other candidates, grouped by how many bytes they agree with the best
  // one's on (0: not the first), each group a pair {weight, length}.  A group
  // weighs its candidates' weights over cand_floor, and a bonus for its first
  // one's rank.
  uint32_t sum = 0, *group = groups, n = (uint8_t)n_cands;
  for (uint32_t i = 1; i < n; i++) {
    int32_t *cand = &cands[2 * i];
    int8_t *data = &window[(uint32_t)-cand[1] >> 3];
    cand[1] = (uint8_t)*data;
    uint32_t len = 0;
    if (*data == *match) {
      uint32_t most = max_len;
      if (match <= data && max_len > (uint32_t)(cur - data))
        most = cur - data;
      for (len = 1; len < most && data[len] == match[len]; len++)
        ;
    }
    uint32_t w = (uint32_t)cand[0] - cand_floor;
    if (group_slot[len])
      *group_slot[len] += w;
    else {
      group_slot[len] = group;
      w += (n - i) >> 1;
      group[0] = w;
      group[1] = len;
      group += 2;
    }
    sum += w;
  }

  // The groups join the length model for now, scaled to it; the groups keep
  // the counts they replace.  The no-match group is weighed first: by
  // len_avg against lit_threshold, less when next_byte agrees with the best
  // candidate, and by how often its byte came up lately -- ctx_count indexed
  // by the byte alone, as vbn does.
  uint32_t saved_total = 0;
  if (group != groups) {
    if (uint32_t *g0 = group_slot[0]) {
      uint32_t avg = (uint32_t)len_avg >> top_bit(((uint32_t)(48 - lit_threshold) >> 3) + 1);
      uint32_t w = *g0;
      if (agree) {
        avg = len_avg;
        w = ((3 * w) >> 2) + 1;
        if (agree != 1)
          w = ((7 * w) >> 3) + 1;
      }
      w += w >> top_bit(avg + 1);
      *g0 = w;
      ctx16 = (ctx16 & ~0xFF) | cands[1];
      uint8_t seen = ctx_count[cands[1]];
      if (seen == 0)
        *g0 += w >> 9;
      else if (seen == 1)
        *g0 -= w >> 5;
      else if (seen == 2)
        *g0 -= (w >> 3) + (w >> 3 >> 2);
      else if (seen <= 5)
        *g0 -= (w >> 3) + (w >> 3 >> 1);
      else
        *g0 -= w >> 2;
    }
    saved_total = len_total;
    uint32_t weight = 4 * len_total;
    if ((uint32_t)lit_cost8 < 0x44C)
      weight = (uint32_t)lit_cost8 < 0x384 ? 9 * len_total : 6 * len_total;
    uint32_t shift = top_bit(sum) + 6;               // shld/shrd: the count is mod 32
    uint32_t share = (uint64_t)(n_cands + 4 * search_gain + ((uint32_t)lit_cost8 >> 6)) * weight
                     / (((uint32_t)(len_avg + 1935 - lit_cost8) >> 3) + 1);
    uint32_t scale = ((uint64_t)share << (shift & 31)) / sum;
    for (uint32_t *g = groups; g < group; g += 2) {
      uint32_t add = search_gain + ((scale * (uint64_t)g[0]) >> (shift & 31));
      len_total += add;
      group_slot[g[1]] = nullptr;
      uint32_t old = len_model[2 * g[1]];
      g[0] = old;
      len_model[2 * g[1]] = old + add;
    }
  }

  // The length: up to max_len, so the longer ones leave the total.
  uint32_t total = len_total;
  for (uint32_t l = max_len + 1; l < 256; l++)
    total -= len_model[2 * l];
  int32_t len;
  if (encoding) {
    for (len = 0; (uint32_t)len < max_len && cur[len] == match[len]; len++)
      ;
    code_symbol(total, code_pos, len, len_model);
  } else {
    len = code_symbol(total, code_pos, -1, len_model);
    for (int32_t j = 0; j < len; j++)
      cur[j] = match[j];
  }
  *excluded = (uint32_t)len < max_len ? (uint8_t)match[len] : -1;
  len_avg = (uint32_t)((len << 7) + 63 * len_avg) >> 6;

  // The model as it was, and the length counted.
  if (group != groups) {
    len_total = saved_total;
    for (uint32_t *g = groups; g < group; g += 2)
      len_model[2 * g[1]] = g[0];
  }
  len_total = len_total - len_model[0] + count0;
  len_model[0] = count0;
  len_model_add(len);
  return len;
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
static __attribute__((noinline)) void search(bool excluding)
{
  // search_pos >= 0.  search_excluding tests search_pos < 0 for no data
  // rather than == 0; it only runs after a match, at a position > 0.
  int32_t **path = search_path, *node = tree, *slot, leaf;
  int32_t bitpos = search_pos;
  const uint8_t *p = (const uint8_t *)&window[(uint32_t)bitpos >> 3];
  for (; bitpos > 0; bitpos -= 8) {
    uint8_t byte = *--p;
    for (int32_t k = 1; k <= 8; k++, byte <<= 1) {
      slot = &node[1 - (byte >> 7)];
      *path = slot;
      leaf = *slot;
      if (leaf <= 0) {
        bitpos -= k;
        goto found;
      }
      path++;
      node = &tree[2 * *slot];
    }
  }
  slot = node;                                    // out of data
  leaf = node[0];
  *path = node;
found:
  found_at = bitpos;
  found_slot = slot;
  found_leaf = leaf;
  // When the other slot holds the same leaf, as a new node's two do, the walk
  // starts a level higher.
  if (leaf == *other_slot(slot) && path != search_path)
    path--;

  uint32_t depth = path - search_path + 1;
  search_depth = depth;
  search_gain = 0;
  if (!excluding && (uint32_t)lit_cost9 > 0xFFE)
    search_exit();
  if ((int32_t)depth > search_avg) {
    search_gain = 16 * (depth - search_avg) / top_bit(tree_nodes + 7 * tree_updates);
    search_avg = ((depth >> 3) + depth + 2 * search_avg) >> 2;
  }
  if (leaf <= 0
      && (uint32_t)search_avg < (uint32_t)search_depth
      && !(excluding && predicts_excluded(leaf))) {
    cands[0] = (search_depth - search_avg) << weight_shift--;
    cands[1] = leaf;
    n_cands++;
  }
  while ((uint32_t)search_depth > (uint32_t)search_avg) {
    int32_t other = *other_slot(*path--);
    if (other > 0) {
      int32_t weight = search_depth - search_avg;
      if (weight_shift)
        weight <<= weight_shift-- >> 1;
      Walk w = { &cands[2 * n_cands], max_cands - n_cands, weight, excluding };
      walk_cands(other, &w);
      n_cands = (w.out - cands) / 2;
    } else if (!(excluding && predicts_excluded(other))) {
      cands[2 * n_cands] = (search_depth - search_avg) << weight_shift;
      cands[2 * n_cands + 1] = other;
      if (++n_cands == max_cands)
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
  if (encoding) {
    write_output(code_buf, 0x2000);
    memcpy(code_buf, code_buf + 0x2000, 16);
    memset(code_buf + 16, 0, 0x2000);
  } else {
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
  while (bitpos < (uint32_t)window_bits) {
    excluded_byte = excluded;
    search_pos = bitpos;
    search_depth = 0;
    n_cands = 0;
    cand_floor = 0;
    weight_shift = 4;
    search_avg = (276 * top_bit(tree_updates + 4)) >> 9;
    run_search(excluded >= 0);
    if (n_cands > 1)
      cand_floor = (uint32_t)cands[2 * n_cands - 2] >> 2;
    // Of the candidates with the top weight, the latest goes first.
    for (int32_t *cand = cands + 2, n = n_cands - 1; n > 0 && cand[0] == cands[0]; cand += 2, n--)
      if (cands[1] >= cand[1])
        std::swap(cands[1], cand[1]);

    uint32_t len = code_match(&code_pos, &excluded, bitpos);
    if (len) {
      next_byte[prev_byte] = window[bitpos >> 3];
      next_byte2[prev_byte2] = window[bitpos >> 3];
      if (excluded >= 0 || (uint32_t)search_gain < 0x2A)
        tree_insert(len, bitpos);
      bitpos += 8 * len;
    } else {
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
    if (code_pos >= 0x10000)
      code_pos = code_buffer_io(code_pos);
  }
  *code_pos_p = code_pos;
  total_bits += bitpos - start;
  return bitpos;
}

// tree_rebuild (vbn.exe 0x100050F8): register the window's positions in an
// empty tree again, after the window slid: every pos = 8, 16, ... below
// window_pos whose bit pos/8 is set in `registered`.  Hex-Rays printed its
// eight-way unrolled descent as carry/overflow arithmetic: per bit, `add
// cl,cl; sbb r,r; adc r,[node+r*4+4]; jle`.  sbb leaves CF (the bit) alone, so
// adc loads [node] for a 1 bit and [node+4] for a 0 bit, carries the bit back
// out and never overflows: jle is a plain `child <= 0`.
static void tree_rebuild(void)
{
  for (uint32_t pos = 8; pos < window_pos; pos += 8)
    if (get_bit(registered, pos >> 3)) {
      int32_t at;
      int32_t *slot = descend(pos, &at);
      tree_add(pos, slot, *slot, at, false);
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
  for (uint32_t i = 0; i < 256; i++) {
    lit_model[2 * i] = 8;
    lit_model[2 * i + 1] = i;
    lit_slot[i] = &lit_model[2 * i];
  }
  len_total = 256;
  for (uint32_t i = 0; i < 256; i++) {
    len_model[2 * i] = 1;
    len_model[2 * i + 1] = i;
  }
  memset(lit_ring1, 0xFF, sizeof lit_ring1);
  memset(lit_ring2, 0xFF, sizeof lit_ring2);
  memset(len_ring, 0xFF, sizeof len_ring);
  memset(ctx_count, 0, sizeof ctx_count);
  lit_ring1_pos = lit_ring2_pos = len_ring_pos = 0;
  for (int32_t len = 0; len < 23; len++)
    for (int32_t n = len_init_counts[len]; n; n--)
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
  data_end = &window[n];
  if (encoding) {
    read_input(window, n);
    memset(code_buf, 0, sizeof code_buf);
  } else
    read_input(code_buf, 2 * n < 0x2010 ? 2 * n : 0x2010);
  memset(registered, 0, sizeof registered);
  tree[0] = tree[1] = 0;
  tree_nodes = 1;
  tree_updates = 1;
  window_pos = 0;

  uint32_t code_pos = 0;
  for (;;) {
    if (window_pos != (uint32_t)window_bits)
      window_pos = code_window(&code_pos, window_pos);
    if (total_bits >> 3 >= size)
      break;
    // Slide the window by 0x66000 bytes, writing them out when decompressing.
    if (!encoding)
      write_output(window, 0x66000);
    memmove(window, window + 0x66000, 0x132000);
    memmove(registered, (int8_t *)registered + 0xCC00, 0x26400);
    memset((int8_t *)registered + 0x26400, 0, 0xCC00);
    tree[0] = tree[1] = 0;
    tree_nodes = 1;
    window_pos = 8 * 0x132000;
    tree_rebuild();
    if (encoding)
      read_input(window + 0x132000, 0x66000);
    uint32_t left = size - (total_bits >> 3);
    if (left < 0x66000) {
      window_bits = 8 * (0x132000 + left);
      data_end = &window[0x132000 + left];
    }
  }
  code_pos = code_flush(code_pos);
  if (encoding)
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
