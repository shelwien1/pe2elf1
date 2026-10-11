// sub_100002E0 @ 0x100002E0 -- hand-written (incdec.md §4.1): create a file.
//   DOS 3Ch, DX=name, CX=20h (archive attribute)
//   carry -> -1, else the handle in EAX            (stdcall, ret 4)
PROBE_DECL(__sub_100002E0)
static __attribute__((stdcall)) VBN_ENTRY int __sub_100002E0(int srca)
{
  PROBE_HIT(__sub_100002E0);
  DosRegs r = {};
  r.eax = 0x3C00; r.edx = (uint32_t)srca; r.ecx = 0x20;
  return vbn_dos(r) ? -1 : (int)r.eax;
}
