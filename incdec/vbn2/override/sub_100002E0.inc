// sub_100002E0 @ 0x100002E0 -- create a file (stdcall, ret 4):
//   DOS 3Ch, DX=name, CX=20h (archive attribute); carry -> -1, else the handle
static __attribute__((stdcall)) int __sub_100002E0(int srca)
{
  return dos_open((const char *)(uintptr_t)srca, 1, 0);
}
