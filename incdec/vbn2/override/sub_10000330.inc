// sub_10000330 @ 0x10000330 -- write to a file (stdcall, ret 12):
//   DOS 40h, CX=count (first argument), DX=buffer (second), BX=handle (third)
//   carry -> -1, else the byte count; a count of 0 truncates the file there
static __attribute__((stdcall)) int __sub_10000330(int a2, int a3, int a4)
{
  return dos_write(a4, (const void *)(uintptr_t)a3, (uint32_t)a2);
}
