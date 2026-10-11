// sub_100002AC @ 0x100002AC -- hand-written (incdec.md §4.1): the size of an open file.
// Hex-Rays decompiled the .shim thunks' pushad frame into the body, so the C it
// printed builds frames in the wrong order.  What the code does (vbn.asm):
//   DOS 4202h, BX=handle, CX:DX=0     lseek to the end; carry -> return -1
//   size = DX<<16 | AX
//   DOS 4200h, BX=handle, CX:DX=0     back to the start; carry ignored
//   return size                        (stdcall, ret 4)
PROBE_DECL(__sub_100002AC)
static __attribute__((stdcall)) VBN_ENTRY int __sub_100002AC(unsigned int *a4)
{
  PROBE_HIT(__sub_100002AC);
  uint32_t handle = (uint32_t)(uintptr_t)a4;
  DosRegs r = {};
  r.eax = 0x4202; r.ebx = handle;
  if (vbn_dos(r))
    return -1;
  int size = (int)(r.eax | (r.edx << 16));
  DosRegs s = {};
  s.eax = 0x4200; s.ebx = handle;
  vbn_dos(s);
  return size;
}
