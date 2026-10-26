// What mp3pack's front end takes from aacpack's base (err.inc, mem.inc,
// bits.inc): memory that ends the program when there is none, a growable
// array of plain data, CRC-32 and little-endian numbers.
//
// The front end has no traps of its own: a call checks what can fail and
// says it (cli.inc, drivers.inc). Out of memory, the program ends at once,
// its temporary files removed (cli.inc's quit_outputs(), at exit).

namespace mp3pack {

[[noreturn]] void out_of_memory() {
  fprintf(stderr, "mp3pack: out of memory\n");
  exit(1);
}

void* mem_alloc(size_t n) {
  void* p = malloc(n ? n : 1);
  if (!p) out_of_memory();
  return p;
}
void* mem_realloc(void* p, size_t n) {
  void* q = realloc(p, n ? n : 1);
  if (!q) out_of_memory();
  return q;
}
void mem_free(void* p) { free(p); }

// Pages of their own, zeroed, for what is too large for the heap or must
// start zeroed as a static would (mp3c's model): win_shim.inc's
// VirtualAlloc, an anonymous mapping on POSIX
void* page_alloc(size_t n) {
  void* p = VirtualAlloc(0, n, MEM_COMMIT, PAGE_READWRITE);
  if (!p) out_of_memory();
  return p;
}
void page_free(void* p) {
  if (p) VirtualFree(p, 0, MEM_RELEASE);
}

// a type for each int (a function's overload for each value of a template
// argument)
template <int N>
struct IntTag {};

// A growable array of plain data: Init() sets it up empty, Quit() frees it
// (no constructor or destructor: whatever holds one may sit on pages that
// are mapped, not constructed)
template <class T>
struct Vec {
  T* p;
  size_t n, cap;  // the elements, and the room for them

  void Init() {
    p = nullptr;
    n = cap = 0;
  }
  void Quit() {
    mem_free(p);
    Init();
  }
  size_t size() const { return n; }
  T* data() { return p; }
  const T* data() const { return p; }
  void clear() { n = 0; }
  // room for m elements, twice as much as before at least
  void room(size_t m) {
    if (m <= cap) return;
    size_t c = m > 2 * cap ? m : 2 * cap;
    if (c > SIZE_MAX / sizeof(T)) out_of_memory();
    p = (T*)mem_realloc(p, c * sizeof(T));
    cap = c;
  }
  // m elements, the new ones zero
  void resize(size_t m) {
    room(m);
    if (m > n) memset((void*)(p + n), 0, (m - n) * sizeof(T));
    n = m;
  }
  void append(const T* q, size_t k) {
    room(n + k);
    memcpy((void*)(p + n), q, k * sizeof(T));
    n += k;
  }
};

// CRC-32 (IEEE 802.3, as zip's): the value of the bytes so far
struct Crc32 {
  uint32_t v;
  static const uint32_t* table() {
    static uint32_t t[256];
    static int made = 0;
    if (!__atomic_load_n(&made, __ATOMIC_ACQUIRE)) {
      for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        t[i] = c;
      }
      __atomic_store_n(&made, 1, __ATOMIC_RELEASE);
    }
    return t;
  }
  void Init() { v = 0xFFFFFFFFu; }
  void add(const void* p, size_t n) {
    const uint32_t* t = table();
    const uint8_t* q = (const uint8_t*)p;
    uint32_t c = v;
    for (size_t i = 0; i < n; i++) c = t[(c ^ q[i]) & 0xFF] ^ (c >> 8);
    v = c;
  }
  uint32_t value() const { return v ^ 0xFFFFFFFFu; }
};
const bool crc32_made = Crc32::table() != nullptr;  // (made as the program starts, before any thread)

// n bytes of v, little-endian, and back
void put_le(uint8_t* p, uint64_t v, int n) {
  for (int i = 0; i < n; i++) p[i] = (uint8_t)(v >> 8 * i);
}
uint64_t get_le(const uint8_t* p, int n) {
  uint64_t v = 0;
  for (int i = 0; i < n; i++) v |= (uint64_t)p[i] << 8 * i;
  return v;
}

}  // namespace mp3pack
