// sub_100002FA @ 0x100002FA -- open a file (stdcall, ret 8):
//   DOS 3Dh, AL=mode (first argument), DX=name (second); carry -> -1, else the handle
static __attribute__((stdcall)) int __sub_100002FA(char a3, int a4)
{
  return dos_open((const char *)(uintptr_t)a4, 0, (unsigned char)a3);
}
