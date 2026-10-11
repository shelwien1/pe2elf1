// sub_100048F0 @ 0x100048F0 -- hand-written (incdec.md §4.1): copy a1 dwords src -> dst.
// `rep movsd`: forward, a dword at a time.  Hex-Rays printed qmemcpy, a byte
// copy, which differs from this when the ranges overlap by less than 4 bytes.
PROBE_DECL(__sub_100048F0)
static __attribute__((stdcall)) VBN_ENTRY void __sub_100048F0(int a1, void *src, void *dst)
{
  PROBE_HIT(__sub_100048F0);
  uint32_t *d = (uint32_t *)dst;
  const uint32_t *s = (const uint32_t *)src;
  for (uint32_t n = (uint32_t)a1; n; n--)
    *d++ = *s++;
}
