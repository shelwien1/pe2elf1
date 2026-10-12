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
// The algorithm is an LZP-type predictive coder: the match source is
// predicted from the longest earlier match of the current context, found
// among the history's sorted contexts, and only match lengths and literals
// are coded, with PPM-style blended probabilities.  VBN 1.001 is ACB 2.00c,
// George Buyanovsky's Associative Coder of Buyanovsky (1997), renamed.  See
// ALGORITHM.md.
//
// The functions are VBN's own, decompiled from vbn.exe: incdec/ moved the ones
// plain `vbn u` and `vbn r` run out of the program one at a time and verified
// each (incdec/README.md), and tools/mkvbn2.py there made them a standalone
// program.  This file started as that program; it has been cut down to the
// coder and written out by hand since, each function headed by the vbn.exe
// function it is.  Nothing is global: the state is in three classes --
// ArithCoder, the arithmetic coder; Codec, the coder around it (the window,
// the context tree and its search, and the models), whose members are vbn's
// globals; and Files, the files they read and write -- and main runs one
// Codec.

#define _FILE_OFFSET_BITS 64        // stat() with -m32 fails on 64-bit inode numbers otherwise
#include <cerrno>
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
// ... and flip it if `bit` (0 or 1) is set, without a branch on it.
static inline void xor_bit(void *buf, uint32_t n, uint32_t bit)
{
  ((uint8_t *)buf)[n >> 3] ^= bit << (n & 7);
}

// ===========================================================================
// Files: the input and the output.  The coder reads its input -- the data
// when compressing, the code (after the stream's size) when decompressing --
// and writes its output, the code or the data, through them.
// ===========================================================================
class Files
{
public:
  const char *const in_name, *const out_name;
  FILE *in = nullptr, *out = nullptr;
  uint32_t data_left = 0;                         // decompressing: the bytes of data still to write

  Files(const char *in_name, const char *out_name) : in_name(in_name), out_name(out_name) {}

  // Say what failed (and errno, if set), remove the output, and exit.
  [[noreturn]] void fail(const char *what, const char *name)
  {
    fprintf(stderr, "vbn2: %s %s%s%s\n", what, name, errno ? ": " : "", errno ? strerror(errno) : "");
    if (out) {
      fclose(out);
      remove(out_name);
    }
    exit(1);
  }

  // read (vbn.exe 0x10000D0E): read the coder's input.  vbn's reader read the
  // archive's title and then each file, or the archive's volumes.  A short
  // read leaves the rest of the buffer as it was, as vbn's did at the end of
  // its data.
  void read(int8_t *to, uint32_t count)
  {
    if (fread(to, 1, count, in) < count && ferror(in))
      fail("cannot read", in_name);
  }

  // write_code, write_data (vbn.exe 0x100012A8): write the coder's output,
  // the code or the data.  vbn's writer split the code into archive volumes,
  // or the data into the title and the files, and stopped at the end of the
  // last file: the data written never runs past the stream's size.
  void write_code(const int8_t *from, uint32_t count) { write(from, count); }
  void write_data(const int8_t *from, uint32_t count)
  {
    if (count > data_left)
      count = data_left;
    data_left -= count;
    write(from, count);
  }

private:
  void write(const int8_t *from, uint32_t count)
  {
    if (fwrite(from, 1, count, out) != count)
      fail("cannot write", out_name);
  }
};

// ===========================================================================
// ArithCoder: VBN's arithmetic coder.  A symbol is coded with a frequency
// list, {count, symbol} pairs: the interval [low, high] narrows to the
// symbol's share of it, and the leading bits low and high come to agree on
// are the code.  The code goes through an 8 KB buffer, which Files writes out
// (compressing) or fills (decompressing).
// ===========================================================================
class ArithCoder
{
public:
  ArithCoder(Files &files, bool encoding) : files(files), encoding(encoding) {}

  // Start: the whole interval, at the start of an empty buffer --
  // decompressing, filled with up to `count` bytes of code.
  void start(uint32_t count)
  {
    low = 0;
    high = 0xFFFFFFFF;
    pending = 0;
    value = 0;
    pos = 0;
    memset(buf, 0, sizeof buf);
    if (!encoding)
      files.read(buf, count < sizeof buf ? count : sizeof buf);
  }

  // Compressing: code `symbol` with the frequency list `model`, whose counts
  // sum to `total`.  Decompressing: the symbol the code holds.
  void encode(uint32_t total, int32_t symbol, const uint32_t *model) { code(total, symbol, model); }
  int32_t decode(uint32_t total, const uint32_t *model) { return code(total, -1, model); }

  // The bit position in the buffer the code has reached.
  uint32_t bits() const { return pos; }

  // next_block (vbn.exe 0x10004924): once 8 KB of code are done --
  // compressing, write them out and move the 16 bytes after them to the start;
  // decompressing, move those 16 bytes to the start and read the next 8 KB
  // after them -- and the bit position goes 8 KB back.
  void next_block()
  {
    if (pos < 8 * BLOCK)
      return;
    if (encoding) {
      files.write_code(buf, BLOCK);
      memcpy(buf, buf + BLOCK, 16);
      memset(buf + 16, 0, BLOCK);
    } else {
      memcpy(buf, buf + BLOCK, 16);
      files.read(buf + 16, BLOCK);
    }
    pos -= 8 * BLOCK;
  }

  // The end: end the code and, compressing, write what is left of it.
  void finish()
  {
    flush();
    if (encoding)
      files.write_code(buf, (pos + 7) >> 3);
  }

private:
  static constexpr uint32_t BLOCK = 0x2000;       // the code goes out and comes in 8 KB at a time

  Files &files;
  const bool encoding;
  uint32_t low, high;                             // 0x1001640C, 0x10016410 the interval
  int32_t pending;                                // 0x10016414 bits of the interval not settled yet
  uint32_t value;                                 // 0x10016460 decompressing: the code read so far
  uint32_t pos;                                   // the bit position in buf the code has reached
  int8_t buf[BLOCK + 16];                         // 0x1000A110 8 KB of code, and 16 bytes more (vbn reused its banner)

  // flush (vbn.exe 0x10004993): end the code.  Writes into buf, from bit
  // pos on, the shortest tail -- a 0 and then 1s, or a 1 and then 0s -- that
  // keeps every continuation inside the interval, and resets the interval.
  // low's top bit is 0 and high's is 1: a 0 and then 1s stays above low once
  // its 1s outlast the 1s after low's top bit, and a 1 and then 0s stays below
  // high once its 0s outlast the 0s after high's.  Neither run is counted past
  // `pending` bits.
  void flush()
  {
    uint32_t lo = low << 1, hi = high << 1;
    uint32_t n_low = 1, n_high = 1;
    for (int32_t k = pending; k > 0; k--) {
      n_low++;
      if ((int32_t)lo >= 0)
        break;
      lo <<= 1;
    }
    for (int32_t k = pending; k > 0; k--) {
      n_high++;
      if ((int32_t)hi < 0)
        break;
      hi <<= 1;
    }
    uint32_t tail = 0x7FFFFFFF, n = n_low;
    if (n_low > n_high) {
      tail = 0x80000000;
      n = n_high;
    }
    for (; n; n--) {
      xor_bit(buf, pos, tail >> 31);
      pos++;
      tail <<= 1;
      pending--;
    }
    low = 0;
    high = 0xFFFFFFFF;
  }

  // code (vbn.exe 0x100061F4): code one symbol with a frequency list of
  // {count, symbol} pairs whose counts sum to `total`.  Compressing (symbol >=
  // 0), the interval narrows to the symbol's share and the bits it settles go
  // to buf; decompressing (symbol < 0), the symbol is the one whose share
  // holds the code read so far.  Returns the symbol.
  //
  // The interval's size, high - low + 1, is 2^32 after flush(), which
  // vbn.exe's 32-bit arithmetic treats as a case of its own and a 64-bit size
  // does not need to.  Decompressing, value holds the 32 bits of code that
  // line up with low; the ones past `pending` arrive with the next symbol.
  int32_t code(uint32_t total, int32_t symbol, const uint32_t *model)
  {
    uint32_t range = high - low;
    if (2 * total > range) {
      flush();
      range = 0xFFFFFFFF;
    }
    uint64_t size = (uint64_t)range + 1;
    const uint32_t *entry;                        // the symbol's pair
    uint32_t below = 0;                           // the counts before it
    if (symbol >= 0) {
      for (entry = model; entry[1] != (uint32_t)symbol; entry += 2)
        below += entry[0];
    } else {
      uint32_t v = value, at = pos + pending;
      for (int32_t n = 32 - pending; n > 0; n--)
        v = 2 * v + get_bit(buf, at++);
      value = v;
      pending = 0;
      // The count the code points at: (value - low + 1) * total - 1, over the
      // size.  vbn subtracts the 1 with `sbb eax,1` straight after the `mul`,
      // so the mul's carry -- the product needing 64 bits -- comes off too
      // (and with a product of total << 32, the borrow of value - low).
      uint32_t offset = v - low;
      uint64_t t;
      if (offset + 1 == 0)
        t = ((uint64_t)total << 32) - 1 - (v < low);
      else {
        t = (uint64_t)total * (offset + 1);
        t -= 1 + (t >> 32 != 0);
      }
      uint32_t target = (uint32_t)(t / size);
      for (entry = model; below + entry[0] <= target; entry += 2)
        below += entry[0];
    }
    uint32_t above = below + entry[0];
    high = low + (uint32_t)(above * size / total) - 1;
    low = low + (uint32_t)(below * size / total);

    // Shift out the leading bits low and high agree on, into buf (a 1 is
    // written by toggling a 0, with no branch on the bit: the code's bits are
    // as good as random).  vbn toggles them when decompressing too, which
    // does no harm: they lie before the 32 bits value holds when it is next
    // used.  flush()'s tail is the same.
    uint32_t lo = low, hi = high, at = pos;
    int32_t left = 32;
    for (uint32_t agree = lo ^ hi; (int32_t)agree >= 0; agree <<= 1) {
      xor_bit(buf, at, lo >> 31);
      left--;
      at++;
      lo <<= 1;
      hi = hi << 1 | 1;
    }
    low = lo;
    high = hi;
    pending = left;
    pos = at;
    return entry[1];
  }
};

// ===========================================================================
// Codec: VBN's coder, which compresses or decompresses one stream.  It codes
// the data a window at a time.  At each position it searches the context tree
// for earlier positions with the same context, the candidates, and codes how
// many bytes match the best one's -- and after no match, a literal -- each
// with its model blended with the candidates' predictions, through an
// ArithCoder.  Its members are vbn's globals, with the address vbn.exe kept
// each at after its name; sizes are what the code reaches.
// ===========================================================================
class Codec
{
public:
  static constexpr uint32_t MAX_SIZE = 0x1FFFFFFF; // the most data: the coder counts it in bits, in 32 bits

  Codec(Files &files, bool encoding) noexcept : files(files), encoding(encoding), ac(files, encoding) {}
  Codec(const Codec &) = delete;                  // it points into itself
  Codec &operator=(const Codec &) = delete;

  // A Codec takes about 15 MB, from the heap: new gets it from malloc, and
  // gives null if there is not that much.  (The global operator new would
  // bring in libstdc++, which vbn2 does not otherwise use.)
  static void *operator new(size_t size) noexcept { return malloc(size); }
  static void operator delete(void *p) noexcept { free(p); }

  void run(uint32_t size);

private:
  // A walk over a subtree's leaves (walk_cands).
  struct Walk {
    int32_t *out;                                 // where the next pair goes
    int32_t left;                                 // how many pairs may still be written
    int32_t weight;                               // the weight the leaves get
    bool excluding;                               // whether the leaves that predict excluded_byte are skipped
  };

  Files &files;

  // The direction, and the mode.
  const bool encoding;                            // 0x1000D4C4 compressing, or decompressing
  static constexpr int32_t max_cands = 95;        // 0x1000D4B0 how many candidates a search returns: 95 (MAX)

  ArithCoder ac;

  // The data: a window of WINDOW bytes that slides by SLIDE.  Positions are
  // bit positions, 8 per byte.
  static constexpr uint32_t WINDOW = 0x198000;
  static constexpr uint32_t SLIDE = 0x66000;
  static constexpr uint32_t KEEP = WINDOW - SLIDE; // what a slide keeps
  int8_t window[WINDOW];                          // 0x100166F0
  uint32_t window_pos;                            // 0x1001646C where coding is, in bits
  int32_t window_bits;                            // 0x10016444 where the data in the window ends, in bits (as an address: 0x1001643C)
  uint32_t total_bits;                            // 0x10016468 the data coded so far, in bits
  int32_t prev_byte;                              // 0x100164B0 the byte before the one being coded
  int32_t prev_byte2;                             // 0x100164B4 ... and the one before that
  int8_t next_byte[256];                          // 0x1001620C per byte: the byte that followed it last
  int8_t next_byte2[256];                         // 0x1001630C per byte: the byte that came two after it last

  // The context tree: a binary tree over the data read backwards from each
  // position, one bit per level.  Its nodes live in an arena, numbered in the
  // order they were made: node n is the two slots tree[2n] and tree[2n+1], and
  // node 0 is the root.  A slot holding a value > 0 points to the node of that
  // number; a value <= 0 is a leaf, minus the position it registers.  A window
  // registers at most WINDOW positions, each making at most one node, and the
  // tree starts over when the window slides: the arena holds WINDOW + 1 nodes.
  // (vbn.exe kept nodes' addresses in the slots, and the next node's address
  // at 0x10016430, always node tree_nodes.)
  int32_t tree[2 * (1 + WINDOW)];                 // 0x101AE6F0 the arena, 12.75 MiB
  int32_t tree_nodes;                             // 0x1000CA4C nodes in use, the root too: the next one made
  int32_t tree_updates;                           // 0x10016434 counts changes to the tree
  int32_t registered[WINDOW / 32];                // 0x10E6E6F0 a bit per position in the window: in the tree

  // The search: from the position being coded, down the tree, collecting
  // earlier positions with the same context -- the candidates -- and a
  // weight for each.
  int32_t cands[2 * max_cands];                   // 0x1000D558 {weight, -position} pairs
  int32_t n_cands;                                // 0x10016450 how many
  int32_t search_gain;                            // 0x1001648C how far the last search's depth beat the average
  int32_t cand_floor;                             // 0x100164C4 a quarter of the weakest candidate's weight
  int32_t excluded_byte;                          // 0x10016488 the byte a failed match predicted, or -1
  int32_t found_at;                               // 0x1001647C where the last descent stopped: the bit position,
  int32_t *found_slot;                            // 0x10016480 ... the slot,
  int32_t found_leaf;                             // 0x10016484 ... and what the slot held

  // Candidates grouped by the value they predict: a match length in
  // code_match, a byte in code_literal.
  uint32_t *group_slot[256];                      // 0x1000EC60 per value: its {weight, value} pair in groups, or null
  uint32_t groups[2 * 256];                       // 0x1000F060

  // The match length model: a count for each length 0..255 (0: no match),
  // following the last 4096 lengths coded.
  uint32_t len_model[2 * 256];                    // 0x1000F860 {count, length} pairs, by length
  uint32_t len_total;                             // 0x100164C0 the sum of the counts
  int32_t len_ring[4096];                         // 0x1001220C the last 4096 lengths
  int32_t len_ring_pos;                           // 0x10016420
  int32_t len_avg;                                // 0x100164A8 the average length coded, x128

  // The literal model: a count for each byte, kept sorted by count, largest
  // first; the last 363 literals add 16 to theirs.
  uint32_t lit_model[2 * 256];                    // 0x10010460 {count, byte} pairs
  uint32_t *lit_slot[256];                        // 0x10010060 per byte: its pair in lit_model
  uint32_t lit_total;                             // 0x100164BC the sum of the counts
  int32_t lit_inc;                                // 0x10016490 what every count gets added (1..1024, by lit_cost8)
  int32_t lit_resort;                             // 0x10016494 literals until the model is sorted again
  int32_t lit_ring1[363];                         // 0x10010C60 the last 363 literals
  int32_t lit_ring1_pos;                          // 0x10016418
  int32_t lit_ring2[1024];                        // 0x1001120C the last 1024 literals, with the byte before each
  int32_t lit_ring2_pos;                          // 0x10016428
  int8_t ctx_count[0x10000];                      // 0x10EB96F0 per byte pair: its count in lit_ring2
  int32_t ctx16;                                  // 0x100164C8 the byte before and the byte coded, as a pair
  int32_t literal_run;                            // 0x100164AC literals in a row whose pair was known
  int32_t lit_threshold;                          // 0x100164B8 set by code_literal, weighs code_match's model
  int32_t lit_cost8;                              // 0x100164A4 the bits a literal costs, x2^8, over about 2^8 literals;
  int32_t lit_cost9;                              // 0x100164A0 ... x2^9 over 2^9,
  int32_t lit_cost10;                             // 0x1001649C ... x2^10 over 2^10,
  int32_t lit_cost11;                             // 0x10016498 ... and x2^11 over 2^11

  // vbn's functions, the ones plain `u` and `r` run, callees first; each is
  // described where it is defined, below the class.
  void len_model_add(int32_t len);
  int32_t *descend(uint32_t pos, int32_t *at);
  void tree_add(uint32_t pos, int32_t *slot, int32_t leaf, uint32_t at, bool flip);
  void tree_insert(uint32_t count, uint32_t bitpos);
  bool walk_emit(int32_t leaf, Walk &w);
  bool walk_cands(int32_t n, Walk &w);
  static uint32_t pair_bonus(uint32_t w, uint32_t seen, bool gained);
  void lit_swap(uint32_t *a, uint32_t *b);
  int32_t code_literal(int32_t excluded, uint32_t bitpos);
  int32_t code_match(int32_t *excluded, uint32_t bitpos);
  void search(uint32_t pos, bool excluding);
  uint32_t code_window(uint32_t bitpos);
  void tree_rebuild();

  // add_low16: add d to the low 16 bits of x only (`add word ptr`).
  static void add_low16(uint32_t &x, int32_t d) { x = (x & 0xFFFF0000) | (uint16_t)(x + d); }

  // ring_put: put v in a ring at pos, and return the value it replaces --
  // the one put as many values ago as the ring holds, or -1.
  template <int32_t N>
  static int32_t ring_put(int32_t (&ring)[N], int32_t &pos, int32_t v)
  {
    int32_t old = ring[pos];
    ring[pos] = v;
    pos = pos + 1 == N ? 0 : pos + 1;
    return old;
  }

  // other_slot: the other slot of the node `slot` is in.  (vbn.exe flips bit
  // 2 of the slot's address.)
  int32_t *other_slot(int32_t *slot) { return tree + ((slot - tree) ^ 1); }

  // Whether the byte at the position of `leaf`, the byte it predicts, is
  // excluded_byte.
  bool predicts_excluded(int32_t leaf) const { return (uint8_t)window[(uint32_t)-leaf >> 3] == excluded_byte; }
};

// len_model_add (vbn.exe 0x100050B0): count a length in the length model.
// Its count goes up by 5 and the count of the length coded 4096 lengths ago
// (from len_ring) down by 5; until the ring is full, len_total grows by 5
// instead.  The counts are added to and taken from in their low 16 bits only
// (`add word ptr`).
void Codec::len_model_add(int32_t len)
{
  add_low16(len_model[2 * len], 5);
  int32_t old = ring_put(len_ring, len_ring_pos, len);
  if (old >= 0)
    add_low16(len_model[2 * old], -5);
  else
    len_total += 5;
}

// descend: from the root, down the context tree along the data before bit
// position pos (> 0), read backwards a bit at a time -- a 1 bit takes slot 0,
// a 0 bit slot 1 -- to the first slot that holds a leaf, or, when the data
// runs out, to slot 0 of the node reached.  Returns the slot, and in *at the
// bit position the descent stopped at.  (tree_insert's and tree_rebuild's;
// search records its path as well.)
int32_t *Codec::descend(uint32_t pos, int32_t *at)
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
void Codec::tree_add(uint32_t pos, int32_t *slot, int32_t leaf, uint32_t at, bool flip)
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
void Codec::tree_insert(uint32_t count, uint32_t bitpos)
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
// written; ECX and EDI come back); here the caller passes a Walk.  Returns
// false once no more pairs may be written: the search is over.
//
// The walk's tests, node or leaf, follow the tree's shape and are the coder's
// worst-predicted branches, but they stay.  A walk without them -- a stack
// pushed and popped by arithmetic -- waits at every step for the node before
// it, while the branching walk guesses ahead and fetches the next nodes in
// the meantime: it ran a third to a half slower.  A jump through a table of
// the four shapes a node can have mispredicts as often as the tests, and was
// no faster.  What helps is to fetch the right child, wanted only after the
// left subtree, while that is walked.
bool Codec::walk_emit(int32_t leaf, Walk &w)
{
  w.out[1] = leaf;
  if (w.excluding && predicts_excluded(leaf))
    return true;
  w.out[0] = w.weight;
  w.out += 2;
  return --w.left != 0;
}
bool Codec::walk_cands(int32_t n, Walk &w)
{
  const int32_t *node = &tree[2 * n];
  __builtin_prefetch(&tree[2 * (node[1] & -(node[1] > 0))]); // a leaf fetches the root, at hand
  if (!(node[0] > 0 ? walk_cands(node[0], w) : walk_emit(node[0], w)))
    return false;
  if (node[1] > 0)
    return walk_cands(node[1], w);
  if (node[0] > n)                                // cmp eax,[eax]; jle
    return walk_emit(node[1], w);
  return true;
}

// pair_bonus: what the first candidate for a byte, of weight w, gets added,
// by how often the pair (byte before, byte) came up in the last 1024
// literals (`seen`, at most 255): bonus * w / 16, from the second row once
// the search has `gained` 0x13 or more.  The rows are vbn's multiples -- w/8,
// 2w, 3w, 4w, 5w (seen 4..6), 8w (7 on); w/16, w/2, w, 3w/2, 2w, 5w/2 (5 on)
// -- and round as vbn's do: w + (w >> 1) is (24 * w) >> 4.  vbn picks them
// with a chain of compares.  A weight is below 2^15 (a depth of at most 1284,
// shifted by at most 4), so the product fits in 32 bits.
uint32_t Codec::pair_bonus(uint32_t w, uint32_t seen, bool gained)
{
  static constexpr uint8_t bonus[2][8] = {
    { 2, 32, 48, 64, 80, 80, 80, 128 },           // search_gain < 0x13
    { 1, 8, 16, 24, 32, 40, 40, 40 },             // search_gain >= 0x13
  };
  return (bonus[gained][seen < 7 ? seen : 7] * w) >> 4;
}

// lit_swap: swap two pairs of the literal model, and the slots that find them.
void Codec::lit_swap(uint32_t *a, uint32_t *b)
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
int32_t Codec::code_literal(int32_t excluded, uint32_t bitpos)
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
  static constexpr uint32_t steps[7] = { 0x2EC, 0x37F, 0x489, 0x56F, 0x63F, 0x70F, 0x75C };
  static constexpr int32_t incs[8] = { 1, 2, 4, 8, 16, 32, 128, 1024 };
  uint32_t c8 = lit_cost8, k = 0;
  for (uint32_t i = 0; i < 7; i++)
    k += c8 >= steps[i];                          // the steps rise: k is the first one above c8
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
  //
  // The loop does not branch on the candidates, whose bytes it cannot
  // predict: each one adds its weight to its byte's group or, if it is the
  // byte's first, to a new group at `group`, which only then moves on; every
  // one computes a pair_bonus, which only a first one keeps; an excluded one
  // adds 0 and leaves its byte without a group.  (vbn sets ctx16's low byte
  // to each new group's byte; nothing reads it before it is set again.)
  uint32_t n_groups = 0, sum = 0, *group = groups;
  if (n_cands) {
    const int32_t *end = cands + 2 * (uint8_t)n_cands;
    uint32_t floor = cand_floor, before = ctx16 & ~0xFF, skipped = 0;
    bool gained = (uint32_t)search_gain >= 0x13;
    for (const int32_t *cand = cands; cand < end; cand += 2) {
      uint32_t b = (uint8_t)cand[1], skip = b == (uint32_t)excluded;
      uint32_t *g = group_slot[b], *t = g ? g : group;
      uint32_t fresh = !g & !skip;
      uint32_t w = (uint32_t)cand[0] - floor + (pair_bonus(cand[0], (uint8_t)ctx_count[before | b], gained) & -fresh);
      w &= skip - 1;
      t[0] = (t[0] & -(uint32_t)(g != nullptr)) + w; // read either way: `g ? t[0] : 0` branches
      t[1] = b;
      group_slot[b] = skip ? g : t;
      group += 2 * fresh;
      sum += w;
      skipped |= skip;
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
    static constexpr int32_t mode_shift[3] = { 10, 6, 4 }; // 0x1000CA54 by mode, max_cands >> 5
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

  uint32_t start = ac.bits();
  int32_t byte;
  if (encoding) {
    byte = (uint8_t)window[bitpos >> 3];
    ac.encode(lit_total, byte, lit_model);
  } else
    byte = ac.decode(lit_total, lit_model);
  uint32_t bits = ac.bits() - start;
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
  int32_t old = ring_put(lit_ring2, lit_ring2_pos, pair);
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
  old = ring_put(lit_ring1, lit_ring1_pos, byte);
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
int32_t Codec::code_match(int32_t *excluded, uint32_t bitpos)
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
  int8_t *cur = &window[at], *match = &window[-cands[1] >> 3], *end = &window[(uint32_t)window_bits >> 3];
  uint32_t max_len = 255;
  if ((uint32_t)(cur - match) < 0xFF)
    max_len = cur - match;
  if ((uint32_t)(end - cur) < max_len)
    max_len = end - cur;
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
      ctx16 = (ctx16 & ~0xFF) | cands[1];
      // Its byte never came up lately: w gains w/512.  Once, twice, 3..5
      // times, 6 on: w loses w/32, 5w/32, 3w/16, w/4.  Each step is
      // (mul * (w >> pre)) >> 2, rounding as vbn's chain of compares does:
      // its 5w/32 is (w >> 3) + (w >> 5), which is (5 * (w >> 3)) >> 2.  The
      // product fits in 32 bits for any w.
      static constexpr uint8_t mul[8] = { 1, 1, 5, 6, 6, 6, 1, 1 }, pre[8] = { 7, 3, 3, 3, 3, 3, 0, 0 };
      uint32_t seen = (uint8_t)ctx_count[cands[1]], s = seen < 7 ? seen : 7;
      uint32_t step = (mul[s] * (w >> pre[s])) >> 2;
      *g0 = seen ? w - step : w + step;
    }
    saved_total = len_total;
    // The model weighs 4 times its total, 6 once literals cost less than
    // 0x44C / 2^8 (about 4.3 bits), 9 below 0x384 / 2^8 (about 3.5).
    static constexpr uint8_t times[3] = { 4, 6, 9 };
    uint32_t weight = times[((uint32_t)lit_cost8 < 0x44C) + ((uint32_t)lit_cost8 < 0x384)] * len_total;
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
    ac.encode(total, len, len_model);
  } else {
    len = ac.decode(total, len_model);
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
// find the candidates for position pos, the earlier positions with the same
// context.  The descent goes down the context tree from its root along the
// data before pos, read backwards a bit at a time (a 1 bit takes slot 0, a 0
// bit slot 1), recording the slots in search_path, to the first slot that
// holds a leaf -- or, when the data runs out first, to slot 0 of the node
// reached -- and keeps where it stopped in found_at, found_slot and
// found_leaf.  Hex-Rays printed it unrolled, eight bits to a byte, as carry
// and overflow arithmetic (see tree_rebuild).  Then the search walks back up:
// at each level above search_avg, the other slot's leaves become candidates
// (walk_cands), weighted by how far the level is above it.  max_cands
// candidates end the search at once: vbn.exe returns from however deep in the
// walk (0x1000703B records where the search returns to, and 0x10006E22
// returns there); here the walks return false.  search_tree gives up after
// the descent when lit_cost9 is above 0xFFE; search_excluding (`excluding`:
// after a failed match) skips the candidates that predict excluded_byte.
// (vbn.exe kept the search's locals in globals: pos at 0x1001644C,
// search_path at 0x1000D850, search_depth at 0x10016454, search_avg at
// 0x10016438 and weight_shift at 0x10016448.)
void Codec::search(uint32_t pos, bool excluding)
{
  int32_t *search_path[1284];                     // the slots the descent went through
  int32_t search_avg = (276 * top_bit(tree_updates + 4)) >> 9; // the depth a candidate has to beat
  int32_t weight_shift = 4;                       // the next candidate's weight shift (4 down to 0)
  n_cands = 0;

  // pos >= 0.  search_excluding tests pos < 0 for no data rather than == 0;
  // it only runs after a match, at a position > 0.
  int32_t **path = search_path, *node = tree, *slot, leaf;
  int32_t bitpos = pos;
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
  int32_t search_depth = depth;                   // the depth the walk back up has reached
  search_gain = 0;
  if (!excluding && (uint32_t)lit_cost9 > 0xFFE)
    return;
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
      bool more = walk_cands(other, w);
      n_cands = (w.out - cands) / 2;
      if (!more)
        return;
    } else if (!(excluding && predicts_excluded(other))) {
      cands[2 * n_cands] = (search_depth - search_avg) << weight_shift;
      cands[2 * n_cands + 1] = other;
      if (++n_cands == max_cands)
        return;
    }
    search_depth--;
  }
}

// code_window (vbn.exe 0x100056C4): code the window from bitpos to its end.  At
// each position: search for candidates (excluding the byte a failed match
// predicted) and code a match length; on a match, register some of its
// positions in the tree and skip it, else code a literal and register the
// position.  Every 8 KB of code goes in or out (ArithCoder::next_block).
// Returns the bit position reached.
uint32_t Codec::code_window(uint32_t bitpos)
{
  uint32_t start = bitpos;
  int32_t excluded = -1;
  while (bitpos < (uint32_t)window_bits) {
    excluded_byte = excluded;
    search(bitpos, excluded >= 0);
    cand_floor = n_cands > 1 ? (uint32_t)cands[2 * n_cands - 2] >> 2 : 0;
    // Of the candidates with the top weight, the latest goes first.
    for (int32_t *cand = cands + 2, n = n_cands - 1; n > 0 && cand[0] == cands[0]; cand += 2, n--)
      if (cands[1] >= cand[1])
        std::swap(cands[1], cand[1]);

    uint32_t len = code_match(&excluded, bitpos);
    if (len) {
      next_byte[prev_byte] = window[bitpos >> 3];
      next_byte2[prev_byte2] = window[bitpos >> 3];
      if (excluded >= 0 || (uint32_t)search_gain < 0x2A)
        tree_insert(len, bitpos);
      bitpos += 8 * len;
    } else {
      int8_t byte = code_literal(excluded, bitpos);
      next_byte[prev_byte] = byte;
      next_byte2[prev_byte2] = byte;
      tree_insert(1, bitpos);                     // vbn has this one inline
      excluded = -1;
      bitpos += 8;
    }
    int32_t at = bitpos >> 3;
    prev_byte = (uint8_t)window[at - 1];
    prev_byte2 = at == 1 ? prev_byte : (uint8_t)window[at - 2];
    ac.next_block();
  }
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
void Codec::tree_rebuild()
{
  for (uint32_t pos = 8; pos < window_pos; pos += 8)
    if (get_bit(registered, pos >> 3)) {
      int32_t at;
      int32_t *slot = descend(pos, &at);
      tree_add(pos, slot, *slot, at, false);
    }
}

// run (vbn.exe 0x10004A5F): compress or decompress `size` bytes.
// It sets up the models, codes the window, and slides it by SLIDE bytes --
// re-registering the positions it keeps -- until all the data is coded; then
// it ends the code.  The data and the code come from Files and go to it.
void Codec::run(uint32_t size)
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
  static constexpr int32_t len_init_counts[23] = { // 0x1000C97C the counts lengths 0..22 start with
    2662, 633, 321, 185, 77, 40, 24, 22, 18, 15, 13, 10, 7, 5, 3, 3, 2, 2, 2, 1, 1, 1, 1
  };
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

  // The first window, the coder and the tree.
  total_bits = 0;
  uint32_t n = size < WINDOW ? size : WINDOW;
  window_bits = 8 * n;
  if (encoding)
    files.read(window, n);
  ac.start(2 * n);
  memset(registered, 0, sizeof registered);
  tree[0] = tree[1] = 0;
  tree_nodes = 1;
  tree_updates = 1;
  window_pos = 0;

  for (;;) {
    if (window_pos != (uint32_t)window_bits)
      window_pos = code_window(window_pos);
    if (total_bits >> 3 >= size)
      break;
    // Slide the window by SLIDE bytes, writing them out when decompressing.
    if (!encoding)
      files.write_data(window, SLIDE);
    memmove(window, window + SLIDE, KEEP);
    memmove(registered, (int8_t *)registered + SLIDE / 8, KEEP / 8);
    memset((int8_t *)registered + KEEP / 8, 0, SLIDE / 8);
    tree[0] = tree[1] = 0;
    tree_nodes = 1;
    window_pos = 8 * KEEP;
    tree_rebuild();
    if (encoding)
      files.read(window + KEEP, SLIDE);
    uint32_t left = size - (total_bits >> 3);
    if (left < SLIDE)
      window_bits = 8 * (KEEP + left);
  }
  ac.finish();
  if (!encoding)
    files.write_data(window, window_pos >> 3);
}

// ===========================================================================
// main: vbn2 c|d input output.  A compressed stream is the input's size (4
// bytes, little-endian) and then vbn's code for it, in vbn's MAX mode (`u`):
// the archive around it -- header, title, volumes -- is gone.  The coder is
// vbn's, run once, as vbn ran it for an archive.
// ===========================================================================
int main(int argc, char **argv)
{
  if (argc != 4 || (strcmp(argv[1], "c") && strcmp(argv[1], "d"))) {
    fprintf(stderr, "vbn2: VBN 1.001's compressor (MAX mode) on one file\n"
                    "Usage: vbn2 c input output   compress\n"
                    "       vbn2 d input output   decompress\n");
    return 1;
  }
  bool encoding = argv[1][0] == 'c';             // compressing, or decompressing
  Files files(argv[2], argv[3]);
  struct stat in_st, out_st;
  files.in = fopen(files.in_name, "rb");
  if (!files.in || fstat(fileno(files.in), &in_st) < 0)
    files.fail("cannot open", files.in_name);
  if (stat(files.out_name, &out_st) == 0 && out_st.st_dev == in_st.st_dev && out_st.st_ino == in_st.st_ino) {
    errno = 0;
    files.fail("input and output are the same file:", files.in_name);
  }

  uint8_t header[4];
  uint32_t size;
  if (encoding) {
    if (!S_ISREG(in_st.st_mode) || in_st.st_size > (int64_t)Codec::MAX_SIZE) {
      errno = 0;
      files.fail(S_ISREG(in_st.st_mode) ? "input is too large (512 MB at most):" : "not a regular file:", files.in_name);
    }
    size = (uint32_t)in_st.st_size;
    for (int32_t i = 0; i < 4; i++)
      header[i] = (uint8_t)(size >> 8 * i);
  } else {
    errno = 0;
    if (fread(header, 1, 4, files.in) != 4)
      files.fail(ferror(files.in) ? "cannot read" : "not a vbn2 stream (too short):", files.in_name);
    size = header[0] | header[1] << 8 | header[2] << 16 | (uint32_t)header[3] << 24;
    if (size > Codec::MAX_SIZE)
      files.fail("not a vbn2 stream (bad size):", files.in_name);
    files.data_left = size;
  }
  files.out = fopen(files.out_name, "wb");
  if (!files.out)
    files.fail("cannot create", files.out_name);
  if (encoding && fwrite(header, 1, 4, files.out) != 4)
    files.fail("cannot write", files.out_name);

  Codec *codec = new Codec(files, encoding);
  if (!codec) {
    errno = 0;
    files.fail("cannot allocate", "memory");
  }
  codec->run(size);
  delete codec;

  errno = 0;
  if (!encoding && files.data_left)
    files.fail("not a vbn2 stream (data missing):", files.in_name);
  if (fclose(files.out) != 0) {
    files.out = nullptr;                          // closed, if not written
    remove(files.out_name);
    files.fail("cannot write", files.out_name);
  }
  fclose(files.in);
  return 0;
}
