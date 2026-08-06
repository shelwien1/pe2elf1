/* d5c_bytebuf.inc -- part of dxt5comp_core.inc; not standalone.
 *
 * ByteBuf -- the core's own growable byte buffer. */

struct ByteBuf {
  u8* p = nullptr; size_t n = 0, cap = 0;
  ByteBuf() {}
  ~ByteBuf() { free(p); }
  ByteBuf(const ByteBuf&) = delete;
  ByteBuf& operator=(const ByteBuf&) = delete;
  void reserve(size_t c) {
    if(c <= cap) return;
    size_t nc = cap ? cap : 64;
    while(nc < c) nc += nc / 2 + 64;
    u8* q = (u8*)realloc(p, nc);
    if(!q) d5c_oom("ByteBuf", nc, 1);   // was a bare abort(): no message at all
    p = q; cap = nc;
  }
  void clear() { n = 0; }
  /* Give the storage back, not just the length.  The dedup table holds one of
     these per retained texture and its whole point is a byte budget, so an
     evicted slot has to stop costing anything. */
  void free_all() { ::free(p); p = nullptr; n = cap = 0; }
  /* Exactly k bytes, allocated to fit: reserve()'s 1.5x growth is right for a
     buffer that is appended to, and wrong for one that is filled once and then
     counted against a budget. */
  void assign_copy(cu8P d, size_t k) {
    if(k != cap) { u8* q = (u8*)realloc(p, k ? k : 1); if(!q) d5c_oom("ByteBuf", k, 1); p = q; cap = k; }
    if(k) memcpy(p, d, k);
    n = k;
  }
  void resize(size_t k) { reserve(k); if(k > n) memset(p + n, 0, k - n); n = k; }
  void assign_zero(size_t k) { n = 0; resize(k); }
  inline void push(u8 b) { if(n == cap) reserve(n + 1); p[n++] = b; }
  void append(cu8P d, size_t k) { if(!k) return; reserve(n + k); memcpy(p + n, d, k); n += k; }
  u8* data() { return p; }
  const u8* data() const { return p; }
  size_t size() const { return n; }
  u8& operator[](size_t i) { return p[i]; }
  u8 operator[](size_t i) const { return p[i]; }
};
