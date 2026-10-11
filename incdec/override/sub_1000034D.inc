// sub_1000034D @ 0x1000034D -- hand-written (incdec.md §4.1): close a file.
//   DOS 3Eh, BX=handle;  carry -> -1, else 0       (stdcall, ret 4)
PROBE_DECL(__sub_1000034D)
static __attribute__((stdcall)) VBN_ENTRY int __sub_1000034D(int a3)
{
  PROBE_HIT(__sub_1000034D);
  DosRegs r = {};
  r.eax = 0x3E00; r.ebx = (uint32_t)a3;
  return vbn_dos(r) ? -1 : 0;
}
