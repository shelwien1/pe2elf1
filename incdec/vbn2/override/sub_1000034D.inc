// sub_1000034D @ 0x1000034D -- close a file (stdcall, ret 4):
//   DOS 3Eh, BX=handle; 0 (closing a handle that is not open is no error)
static __attribute__((stdcall)) int __sub_1000034D(int a3)
{
  return dos_close(a3);
}
