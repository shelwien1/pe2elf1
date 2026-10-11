// sub_10000313 @ 0x10000313 -- hand-written (incdec.md §4.1): read from a file.
//   DOS 3Fh, CX=count (first argument), DX=buffer (second), BX=handle (third)
//   carry -> -1, else the byte count in EAX        (stdcall, ret 12)
PROBE_DECL(__sub_10000313)
static __attribute__((stdcall)) VBN_ENTRY int __sub_10000313(int a2, int a3, int a4)
{
  PROBE_HIT(__sub_10000313);
  DosRegs r = {};
  r.eax = 0x3F00; r.ecx = (uint32_t)a2; r.edx = (uint32_t)a3; r.ebx = (uint32_t)a4;
  return vbn_dos(r) ? -1 : (int)r.eax;
}
