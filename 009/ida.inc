// ida.inc -- what is left of the Hex-Rays vocabulary.
//
// The decompiler writes a byte or a word inside a wider value as a macro over
// a pointer cast -- `BYTE4(x)` for `*((uint8_t *)&x + 4)`, `LOWORD(x) = y` for
// a sixteen-bit store into the low half of a register.  Every one of those is
// gone from this tree now, and what replaced each is at the site: an absolute
// value written as `abs32`, a signed comparison written as a comparison, and a
// partial store written as the assignment it amounts to -- in every case the
// half it preserved turned out to be dead, which the types say and the streams
// agree with.  The macros were how MSVC's register allocation looked from the
// outside; none of them was a decision the program made.
//
// `__OFSUB__` went the same way.  It is the x86 overflow flag after a compare,
// and Hex-Rays emits it wherever the original tested a signed inequality --
// signed less-than *is* `SF != OF`, so `(x-y<0) ^ __OFSUB__(x,y)` is `x < y`
// and nothing more.  Its one site said `e_top > deadzone_hi`, which is what
// the five sibling lines around it already said.
//
// Two are kept, because both say something the code means rather than
// something the compiler did:

// A 64-bit value assembled from two 32-bit halves.  The original works in
// halves here -- this is a value the program builds, not a register split.
#define __PAIR64__(high, low) (((uint64_t)(high)<<32)|(uint32_t)(low))

// Four bytes of a recovered struct that exist only at the wider pointer width:
// a member that is a pointer in one build and four bytes of padding in the
// other.  The struct's layout is the program's, and this is how it stays the
// same size at both.
#define BMF_SPILL_PAD(n) uint8_t _spill_pad ## n[sizeof(void*)-4]

// The two helpers the decompiler calls by name.  `abs32` is a function and not
// a macro, and it is what the branchless absolute values above became: the
// original computed `(m ^ x) - m` with `m` the sign broadcast into a byte or a
// word, which is `|x|` in as many bits as the destination keeps.
inline uint32_t abs32(int32_t x) {
  return x>=0 ? (uint32_t)x : (uint32_t)-x;
}
