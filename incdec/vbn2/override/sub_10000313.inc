// sub_10000313 @ 0x10000313 -- read from a file (stdcall, ret 12):
//   DOS 3Fh, CX=count (first argument), DX=buffer (second), BX=handle (third)
//   carry -> -1, else the byte count
static __attribute__((stdcall)) int __sub_10000313(int a2, int a3, int a4)
{
  return dos_read(a4, (void *)(uintptr_t)a3, (uint32_t)a2);
}
