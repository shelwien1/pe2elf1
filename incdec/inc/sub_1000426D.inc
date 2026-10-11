// sub_1000426D @ 0x1000426D -- hand-written (incdec.md §4.1): a bitwise CRC over a buffer.
// The routine is assembly that saves EBP and then uses it as a zero register
// (`push ebp; mov ebp,eax` with EAX=0, then `adc ebx,ebp`); Hex-Rays took EBP
// for the frame pointer and computed with &savedregs.  Per bit, MSB first:
//   add dl,dl; sbb ebx,ebx    ebx = -(data bit)
//   add eax,eax; adc ebx,ebp  ebx += (top bit of the crc) + 0
//   je; xor eax,0FFFFFFF7h    xor when the two bits differ
// The count is tested after the first byte (dec ecx; jnz), as in the original.
PROBE_DECL(__sub_1000426D)
static __attribute__((stdcall)) VBN_ENTRY int __sub_1000426D(int a1, _BYTE *a2)
{
  PROBE_HIT(__sub_1000426D);
  uint32_t crc = 0;
  do {
    unsigned d = *a2++;
    for (int k = 0; k < 8; k++) {
      unsigned t = ((d >> 7) ^ (crc >> 31)) & 1;
      d <<= 1;
      crc <<= 1;
      if (t)
        crc ^= 0xFFFFFFF7u;
    }
  } while (--a1);
  return (int)crc;
}
