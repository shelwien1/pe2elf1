// sub_10000369 @ 0x10000369 -- set file attributes (stdcall, ret 8):
//   DOS 4301h, CX=attributes (first argument), DX=name (second); carry -> -1, else 0
// As vbn's port served it, the attributes are not applied: the file is only
// made writable for its owner (dos_setattr).
static __attribute__((stdcall)) int __sub_10000369(int a2, int a3)
{
  (void)a2;
  return dos_setattr((const char *)(uintptr_t)a3) < 0 ? -1 : 0;
}
