// sub_10000369 @ 0x10000369 -- hand-written (incdec.md §4.1): set file attributes.
//   DOS 4301h, CX=attributes (low byte of the first argument), DX=name (second)
//   carry -> -1, else 0                            (stdcall, ret 8)
PROBE_DECL(__sub_10000369)
static __attribute__((stdcall)) VBN_ENTRY int __sub_10000369(int a2, int a3)
{
  PROBE_HIT(__sub_10000369);
  DosRegs r = {};
  r.eax = 0x4301; r.ecx = (unsigned char)a2; r.edx = (uint32_t)a3;
  return vbn_dos(r) ? -1 : 0;
}
