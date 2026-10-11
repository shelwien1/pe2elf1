// sub_100002FA @ 0x100002FA -- hand-written (incdec.md §4.1): open a file.
//   DOS 3Dh, AL=mode (first argument), DX=name (second)
//   carry -> -1, else the handle in EAX            (stdcall, ret 8)
PROBE_DECL(__sub_100002FA)
static __attribute__((stdcall)) VBN_ENTRY int __sub_100002FA(char a3, int a4)
{
  PROBE_HIT(__sub_100002FA);
  DosRegs r = {};
  r.eax = 0x3D00 | (unsigned char)a3; r.edx = (uint32_t)a4;
  return vbn_dos(r) ? -1 : (int)r.eax;
}
