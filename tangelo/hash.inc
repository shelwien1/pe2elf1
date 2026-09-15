// Hash
static uint hash(uint a, uint b, uint c = 0xffffffff) {
  uint h = a * 200002979u + b * 30005491u + c * 50004239u + 4114959990u;
  return h ^ h >> 9 ^ a >> 2 ^ b >> 3 ^ c >> 4 ^ 0x4000000;
}
