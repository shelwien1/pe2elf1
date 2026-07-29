/* xad_util.inc -- part of xadpcm.cpp; not standalone.
 *
 * crc32b, byte order, varint zigzag, bit packing, and the one global flag.
 * Nothing here knows anything about the model. */

/* No load_file / save_file any more.  Both directions read and write through
   coroutine pins (Xad::slurp, Xad::putn and the RC's own pin), and the only
   stdio left in the program is in XadDrive, which is the one place allowed to
   know what a FILE* is. */

/* Split into init/update/final so the streaming decoder can fold each window in
   as it is produced instead of being handed a whole image.  crc32b() is the same
   function it always was, spelled in terms of the three; the register form is
   what the container's u32 is NOT -- the pre/post complement lives in the
   wrappers, so an intermediate value never escapes as if it were a checksum. */
static u32 CRC_T[256];
static bool crc_ready = false;
static void crc32_tab() {
  if( crc_ready )
    return;
  for( u32 i = 0; i<256; i++ ) {
    u32 c = i;
    for( int k = 0; k<8; k++ )
      c = c&1 ? 0xEDB88320u^(c>>1) : c>>1;
    CRC_T[i] = c;
  }
  crc_ready = true;
}

static INLINE u32 crc32b_init() { crc32_tab(); return 0xFFFFFFFFu; }
static INLINE u32 crc32b_fin(u32 c) { return c^0xFFFFFFFFu; }
static u32 crc32b_up(u32 c, cu8P d, size_t n) {
  for( size_t i = 0; i<n; i++ )
    c = CRC_T[(c^d[i])&0xFF]^(c>>8);
  return c;
}

static u32 crc32b(cu8P d, size_t n) {
  return crc32b_fin(crc32b_up(crc32b_init(), d, n));
}

static INLINE u32 rd16(cu8P p) {
  return u32(p[0])|(u32(p[1])<<8);
}

static INLINE u32 rd32(cu8P p) {
  return rd16(p)|(rd16(p+2)<<16);
}

static INLINE void wr16(u8P p, u32 v) {
  p[0] = u8(v);
  p[1] = u8(v>>8);
}

static INLINE void wr32(u8P p, u32 v) {
  wr16(p, v&0xFFFF);
  wr16(p+2, v>>16);
}

static INLINE u64 zig(i64 v) {
  return (u64(v)<<1)^u64(v>>63);
}

static INLINE i64 unzig(u64 u) {
  return i64(u>>1)^-i64(u&1);
}

/* ---------------------------------------------------------------- Buf<T>
 *
 * What std::vector was actually being asked for in this program: append, index,
 * size, and a one-shot fill.  Nothing here sorts, erases from the middle, or
 * holds a type with a destructor.
 *
 * Growth is 1.5x, so N appends stay amortized O(N).  There are no exceptions
 * (the builds use -fno-exceptions), so an allocation failure aborts -- which is
 * what a vector does too, only via std::terminate and without saying what it
 * was trying to allocate.  Being told "8388608 bytes for the file table" beats
 * "terminate called".
 *
 * NON-COPYABLE on purpose.  Arc holds three of these and is passed around by
 * const reference everywhere; making a copy compile-time impossible is the
 * cheapest way to be sure a by-value parameter never appears and silently
 * duplicates a whole input image.  The one place that genuinely moves a Buf
 * (in_names -> ar.names) says so with steal().
 *
 * T must be trivially copyable: growth is realloc(), which moves bytes and does
 * not run a move constructor.  The static_assert below is the guard -- every T
 * used here is a POD or an aggregate of them, and one that stops being so would
 * otherwise be an unnoticed lifetime bug. */
template<class T>
struct Buf {
  T* p = nullptr;
  size_t n = 0, c = 0;

  static_assert(__is_trivially_copyable(T), "Buf<T> grows with realloc()");

  Buf() {}
  ~Buf() { free(p); }
  Buf(const Buf&) = delete;
  Buf& operator=(const Buf&) = delete;

  size_t size() const { return n; }
  bool empty() const { return n==0; }
  T* data() { return p; }
  const T* data() const { return p; }
  T &operator[](size_t i) { return p[i]; }
  const T &operator[](size_t i) const { return p[i]; }
  T &back() { return p[n-1]; }

  void reserve(size_t k) {
    if( k<=c )
      return;
    /* The byte count is computed in size_t, so a large enough element count
       would wrap and ask for a small block that the caller then writes past.
       Checked rather than assumed: two of these are sized from header fields. */
    if( k>(size_t(0)-1)/sizeof(T) ) {
      fprintf(stderr, "xadpcm: buffer of %zu elements is too large\n", k);
      abort();
    }
    T* q = (T*)realloc(p, k*sizeof(T));
    if( !q ) {
      fprintf(stderr, "xadpcm: out of memory (%zu bytes)\n", k*sizeof(T));
      abort();
    }
    p = q;
    c = k;
  }

  void push_back(const T &v) {
    if( n==c )
      reserve(c+c/2+16);
    p[n++] = v;
  }

  /* Value-initialises the new tail, exactly as vector::resize does -- which for
     Seg means running Params' default member initialisers (chans = 1, bps = 4),
     NOT zeroing.  memset here would be a different function. */
  void resize(size_t k) {
    reserve(k);
    for( size_t i = n; i<k; i++ )
      p[i] = T();
    n = k;
  }

  void assign(size_t k, const T &v) {
    reserve(k);
    for( size_t i = 0; i<k; i++ )
      p[i] = v;
    n = k;
  }

  void append(const T* s, size_t k) {
    if( n+k>c )
      reserve(n+k>c+c/2 ? n+k : c+c/2+16);
    memcpy(p+n, s, k*sizeof(T));
    n += k;
  }

  // hand the storage over and leave the source empty; no copy, no allocation
  void steal(Buf &o) {
    free(p);
    p = o.p;
    n = o.n;
    c = o.c;
    o.p = nullptr;
    o.n = o.c = 0;
  }
};

/* The member-name table.  One blob of bytes plus (offset, length) pairs rather
   than a vector<string>: the names arrive from the archive header, where the
   count is only bounded by a varint, and one allocation per name at that count
   is a shape worth not having.  Each entry is stored with its exact length AND a
   terminating zero -- the length so the encoder writes back the same bytes it
   read, the zero so at() is a C string for fopen().  A name containing a zero
   byte therefore reaches fopen() truncated, which is precisely what
   string::c_str() did here before. */
struct NameTab {
  Buf<char> blob;
  Buf<u32> off, len;

  size_t size() const { return off.size(); }
  bool empty() const { return off.empty(); }
  const char* at(size_t i) const { return blob.data()+off[i]; }
  u32 len_at(size_t i) const { return len[i]; }

  void push_back(const char* s, size_t k) {
    off.push_back(u32(blob.size()));
    len.push_back(u32(k));
    blob.append(s, k);
    blob.push_back(0);
  }
  void push_back(const char* s) { push_back(s, strlen(s)); }
  void steal(NameTab &o) { blob.steal(o.blob); off.steal(o.off); len.steal(o.len); }
};

/* Strip any directory part from a stored name.  Archives name their members,
   and a member called ../../etc/x must not be able to say where it lands.
   Returns a pointer INTO s -- the basename is a suffix, so there is nothing to
   copy -- or the literal, so the result lives as long as the argument. */
static const char* safe_name(const char* s) {
  const char* b = s;
  for( const char* q = s; *q; q++ )
    if( *q=='/'||*q=='\\' )
      b = q+1;
  if( !b[0]||!strcmp(b, ".")||!strcmp(b, "..") )
    b = "member";
  return b;
}

/* "-" means stdin/stdout, which is what makes the streaming decoder worth
   having: `xadpcm d big.xac - | player`.  The returned handle is NOT always
   ours to close, hence is_std() -- fclose(stdout) in the middle of a run would
   take the diagnostics with it. */
static bool is_std(const char* p) { return p&&p[0]=='-'&&!p[1]; }
static FILE* xfopen(const char* p, bool write) {
  if( is_std(p) )
    return write ? stdout : stdin;
  return fopen(p, write ? "wb" : "rb");
}
static void xfclose(const char* p, FILE* f) {
  if( !f )
    return;
  if( is_std(p) )
    fflush(f);
  else
    fclose(f);
}

/* Join a directory prefix and a member name into dst, and say so if it did not
   fit.  Silent truncation here would write a member to the wrong path. */
static bool path_join(char* dst, size_t cap, const char* dir, const char* name) {
  int k = snprintf(dst, cap, "%s%s", dir, name);
  return k>=0&&size_t(k)<cap;
}


static int verbose = 0;
static INLINE int16_t i16_of(cu8P p) {
  return int16_t(u16(p[0])|u16(u16(p[1])<<8));
}

static INLINE u32 get_bits(cu8P b, size_t bitpos, int nbits) {
  u32 v = u32(b[bitpos>>3])>>(bitpos&7);
  int have = 8-int(bitpos&7);
  size_t i = (bitpos>>3)+1;
  while( have<nbits ) {
    v |= u32(b[i++])<<have;
    have += 8;
  }
  return v&((1u<<nbits)-1);
}

static INLINE void put_bits(u8P b, size_t bitpos, int nbits, u32 v) {
  b[bitpos>>3] |= u8(v<<(bitpos&7));
  int have = 8-int(bitpos&7);
  size_t i = (bitpos>>3)+1;
  while( have<nbits ) {
    b[i++] |= u8(v>>have);
    have += 8;
  }
}
