// sub_100002AC @ 0x100002AC -- the size of an open file (stdcall, ret 4):
//   DOS 4202h, BX=handle, CX:DX=0     lseek to the end; carry -> return -1
//   DOS 4200h, BX=handle, CX:DX=0     back to the start; carry ignored
// The size is DX:AX of the first call: its low 32 bits.
static __attribute__((stdcall)) int __sub_100002AC(unsigned int *a4)
{
  int handle = (int)(uintptr_t)a4;
  int64_t size = dos_seek(handle, 0, SEEK_END);
  if (size < 0)
    return -1;
  dos_seek(handle, 0, SEEK_SET);
  return (int)(uint32_t)size;
}
