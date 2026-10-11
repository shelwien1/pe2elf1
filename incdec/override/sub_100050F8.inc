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
static uint32_t &s50F8_n8 = *(uint32_t *)0x1001646C;    // n8
static int32_t &s50F8_nodes = *(int32_t *)0x1000CA4C;   // dword_1000CA4C
static int32_t &s50F8_alloc = *(int32_t *)0x10016430;   // dword_10016430
static int32_t &s50F8_count = *(int32_t *)0x10016434;   // dword_10016434
#define S50F8_BITMAP ((const void *)0x10E6E6F0)          // buf_
#define S50F8_DATA ((const uint8_t *)0x100166F0)         // dst_3
#define S50F8_ROOT ((int32_t *)0x101AE6F0)               // dst

PROBE_DECL(__sub_100050F8)
static VBN_ENTRY unsigned int __sub_100050F8(void)
{
  PROBE_HIT(__sub_100050F8);
  uint32_t pos = 0;                                     // EAX
  for (;;) {
    pos += 8;
    if (pos >= s50F8_n8)
      return pos;
    if (!_bittest(S50F8_BITMAP, (int)(pos >> 3)))
      continue;
    const uint8_t *p = S50F8_DATA + (pos >> 3) - 1;     // EDI
    int32_t at = (int32_t)pos;                          // ESI
    int32_t *node = S50F8_ROOT, *slot;                  // EBX/EDX alternate in the asm
    int32_t leaf;
    uint8_t cl = *p;
    for (;;) {
      int k, found = 0;
      for (k = 1; k <= 8; k++) {
        int bit = (cl >> 7) & 1;
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
    uint32_t thr = (uint32_t)(31 - __builtin_clz((uint32_t)s50F8_nodes + 32)) << 6;
    if (depth > thr) {
      if (leaf <= 0 && pos + (uint32_t)leaf >= 0x800) {
        *slot = -(int32_t)pos;
        s50F8_count++;
      }
    } else if (leaf <= 0) {
      int32_t *n = (int32_t *)(uintptr_t)s50F8_alloc;
      n[0] = -(int32_t)pos;
      n[1] = -(int32_t)pos;
      *slot = (int32_t)(uintptr_t)n;
      s50F8_alloc += 8;
      s50F8_nodes++;
      slot = (int32_t *)((uintptr_t)slot ^ 4);
      if (*slot > 0)
        s50F8_count++;
    }
  }
}
