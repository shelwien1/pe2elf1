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
static int32_t &s5251_n8_0 = *(int32_t *)0x1001647C;   // n8_0: bit position
static int32_t &s5251_dst_0 = *(int32_t *)0x10016480;  // dst_0: slot address
static int32_t &s5251_dst_1 = *(int32_t *)0x10016484;  // dst_1: slot value
static int32_t &s5251_nodes = *(int32_t *)0x1000CA4C;  // dword_1000CA4C
static int32_t &s5251_alloc = *(int32_t *)0x10016430;  // dword_10016430
static int32_t &s5251_count = *(int32_t *)0x10016434;  // dword_10016434
#define S5251_BITMAP ((void *)0x10E6E6F0)              // buf_
#define S5251_DATA ((const uint8_t *)0x100166F0)       // dst_3
#define S5251_ROOT ((int32_t *)0x101AE6F0)             // dst

PROBE_DECL(__sub_10005251)
static __attribute__((stdcall)) VBN_ENTRY unsigned int __sub_10005251(unsigned int a1, int n8)
{
  PROBE_HIT(__sub_10005251);
  uint32_t t = a1 + (a1 >> 1) + 1, iters = t ? 31 - __builtin_clz(t) : 0;
  iters += 1;
  if (iters > a1)
    iters = a1;
  uint32_t pos = (uint32_t)n8;                         // EBP
  uint32_t at = (uint32_t)s5251_n8_0;                  // ESI
  int32_t *slot = (int32_t *)(uintptr_t)s5251_dst_0;   // EBX
  int32_t leaf = s5251_dst_1;                          // EDX
  for (;;) {
    uint32_t depth = pos - at;
    uint32_t thr = (uint32_t)(31 - __builtin_clz((uint32_t)s5251_nodes + 32)) << 6;
    if (depth > thr) {
      if (leaf <= 0 && pos + (uint32_t)leaf >= 0x800) {
        _bittestandcomplement(S5251_BITMAP, (int)(pos >> 3));
        *slot = -(int32_t)((pos >> 3) << 3);
        s5251_count++;
      }
    } else if (leaf <= 0) {
      _bittestandcomplement(S5251_BITMAP, (int)(pos >> 3));
      int32_t v = -(int32_t)((pos >> 3) << 3);
      int32_t *node = (int32_t *)(uintptr_t)s5251_alloc;
      node[0] = v;
      node[1] = v;
      *slot = (int32_t)(uintptr_t)node;
      s5251_alloc += 8;
      s5251_nodes++;
      slot = (int32_t *)((uintptr_t)slot ^ 4);
      if (*slot > 0)
        s5251_count++;
    }
    if (--iters == 0)
      return 0;
    pos += 8;
    // descend from the root
    int32_t *node = S5251_ROOT;
    at = pos;
    slot = node;
    if (at == 0)
      continue;                                        // or esi,esi; je (leaf unchanged)
    const uint8_t *p = S5251_DATA + (pos >> 3) - 1;
    uint8_t al = *p;
    for (;;) {
      int k, found = 0;
      for (k = 1; k <= 8; k++) {
        int bit = (al >> 7) & 1;
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
