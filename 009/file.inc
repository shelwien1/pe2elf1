// file.inc -- the file at each end: one image out, one image in.
//
// This was an archive.  The donor put several images in one file -- its command
// line took a list and appended each to the archive named by `-o` -- and the
// reader walked the members until one failed to parse.  `bmf c in.bmp out`
// writes one image and `bmf d in out.bmp` reads one, so the walk, the append
// point it existed to find, and the member count are all gone.  What is left is
// the handle and the two ways of opening it.

// Each member of a BMF file opens with a four-byte tag: two signature bytes,
// then the version as two ASCII digits.  Two signatures exist -- one introduces
// an image and the other introduces a member `expand_image` skips by the length
// in its eight-byte header -- and this build reads and writes 2.0 only.
//
// The version arithmetic was `((major<<8)-12288)|(minor-48)` tested against
// 512, which is three constants for one idea: 12288 is `'0'<<8`, 48 is `'0'`,
// and 512 is `2<<8`.  It reads the two digits and packs them into a byte each.
//
// The tag is handled as a word at both ends -- the reader `fread`s four bytes
// into a `uint32_t` and masks, and `bmf_tag` builds the same word for the
// writer -- so the signature is the low half on a little-endian host and this
// says nothing useful on any other.  That is the whole program's assumption,
// not this pair's.
enum : uint16_t {
  bmf_sig_image = 0x8A81,
  bmf_sig_other = 0x9081,
};

// `2<<8`, the only version this build accepts.
static constexpr int32_t bmf_version_20 = 2<<8;

static inline int32_t bmf_tag_version(uint32_t tag) {
  return (((uint8_t)(tag>>16)-'0')<<8)|((uint8_t)(tag>>24)-'0');
}

static inline uint32_t bmf_tag(uint16_t sig, char major, char minor) {
  return sig|((uint32_t)(uint8_t)major<<16)|((uint32_t)(uint8_t)minor<<24);
}

// **One stream, over a file or over memory, and the codec cannot tell which.**
//
// The `.bmf` side of this program reads and writes through six operations --
// read, write, seek, tell, end-of-input, close -- and for a long time the thing
// underneath them was a `FILE*`.  That was fine until the codec grew in-memory
// entry points, at which point the obvious move was `open_memstream` and
// `fmemopen`: a `FILE*` over a buffer, so nothing above had to change.
//
// It was the wrong move.  Those two are POSIX-only, so Windows needed a shim,
// and a `FILE*` over memory cannot be built on Windows at all -- there is no
// `fopencookie` -- so the shim was a *temporary file*, which dragged in
// `<windows.h>`, a delete-on-close flag, a hand-built unique name for eight
// threads, and two implementations chosen by `__has_include`.  All of it to
// avoid naming the six operations the program actually uses.
//
// Naming them is smaller than any of that.  There is no `platform.inc` now, no
// SDK header, no temp file, nothing to clean up if the process dies, and the
// in-memory path does not touch a disk it never wanted to touch.  The one thing
// the shim bought that this does not is that `read_bmp` and `write_bmp` still
// take a `FILE*` -- they are the BMP side, which is `main`'s file I/O and not
// the codec's, and a BMP always comes from a path.
struct BmfStream {
  // Exactly one of these two is set.  `fp` is a file; `buf` is memory.
  FILE* fp = nullptr;
  uint8_t* buf = nullptr;
  size_t len = 0;      // bytes of `buf` that hold data
  size_t cap = 0;      // bytes allocated in `buf`; 0 when the buffer is not ours
  size_t pos = 0;      // the cursor, for the memory case
  bool ran_off = false;  // a read asked for more than was there

  static BmfStream over_file(FILE* f) {
    BmfStream s;
    s.fp = f;
    return s;
  }

  // Read-only over a buffer the caller owns.  The `const` is cast away here and
  // nowhere else: nothing writes through a stream made this way, because the
  // only writer is `write`, and `write` refuses when `cap` is zero.
  static BmfStream over_memory(const void* data, size_t n) {
    BmfStream s;
    s.buf = (uint8_t*)data;
    s.len = n;
    return s;
  }

  // A buffer that grows to fit what is written to it, which is what
  // `open_memstream` was for.  `take` hands it to the caller at the end.
  static BmfStream in_memory() {
    BmfStream s;
    s.cap = 1;                     // non-zero: this is the writable kind
    s.buf = (uint8_t*)bmf_new(1);
    return s;
  }

  size_t read(void* dst, size_t n) {
    if( fp )
      return fread(dst, 1u, n, fp);
    const size_t got = pos>=len ? 0 : (n<len-pos ? n : len-pos);
    memcpy(dst, buf+pos, got);
    pos += got;
    if( got<n )
      ran_off = true;
    return got;
  }

  size_t write(const void* src, size_t n) {
    if( fp )
      return fwrite(src, 1u, n, fp);
    if( !cap )
      return 0;                    // a read-only memory stream
    if( pos+n>cap ) {
      // Doubling, and from a one-byte start, so a coded stream of any size
      // costs a logarithmic number of copies.  `bmf_new` is fatal on failure,
      // which is the same answer every other allocation in this program gives.
      size_t want = cap;
      while( want<pos+n )
        want *= 2;
      uint8_t* wider = (uint8_t*)bmf_new(want);
      memcpy(wider, buf, len);
      free(buf);
      buf = wider;
      cap = want;
    }
    memcpy(buf+pos, src, n);
    pos += n;
    if( pos>len )
      len = pos;
    return n;
  }

  // One record in or out, or the answer is no.
  //
  // Six `fread`/`fwrite` results went unread before these existed.  A codec
  // reading untrusted input cannot do that: a short read leaves the destination
  // holding whatever was there before, and the four in `read_bmp` were filling
  // a palette, an RLE absolute run and a 32-bit mask from stale scratch.
  // `(void)` on the result would compile and would be worse than the warning.
  bool read_exact(void* dst, size_t n) { return n==0||read(dst, n)==n; }
  bool write_exact(const void* src, size_t n) { return n==0||write(src, n)==n; }

  // Where the caller has no failure path -- `compress_image`'s palette write is
  // the last thing it does -- a partial archive is not something to return
  // success for.
  void write_all(const void* src, size_t n) {
    if( !write_exact(src, n) )
      bmf_fatal(bmf_write_error);
  }

  bool seek(long off, int32_t whence) {
    if( fp )
      return fseek(fp, off, whence)==0;
    // `size_t` arithmetic on a signed offset, so the comparison is done in
    // `long` first: a negative `SEEK_CUR` past the start has to fail rather
    // than wrap into a huge `pos`.
    long at = whence==SEEK_SET ? 0 : whence==SEEK_END ? (long)len : (long)pos;
    if( at+off<0||(size_t)(at+off)>len )
      return false;
    pos = (size_t)(at+off);
    return true;
  }

  // The memory case cannot distinguish "at the end" from "read past the end"
  // the way `feof` does, so it records the read that ran off rather than
  // inferring it from the position -- which is the same thing `feof` reports.
  bool eof() const { return fp ? feof(fp)!=0 : ran_off; }

  void flush() { if( fp ) fflush(fp); }

  // What is left between here and the end.  Used to refuse a member header that
  // claims more bytes than the stream holds, which is the check that stops a
  // crafted length from being believed.
  uint32_t bytes_left() {
    if( !fp )
      return (uint32_t)(len-pos);
    const long here = ftell(fp);
    if( here<0||fseek(fp, 0, SEEK_END)!=0 )
      return 0;
    const long end = ftell(fp);
    if( end<0||fseek(fp, here, SEEK_SET)!=0 )
      return 0;
    return (uint32_t)(end-here);
  }

  // The buffer, handed to the caller, who frees it.  The stream keeps nothing:
  // calling this twice answers null the second time rather than a pointer two
  // people now believe they own.
  uint8_t* take(size_t* out_len) {
    uint8_t* out = buf;
    *out_len = len;
    buf = nullptr;
    cap = len = pos = 0;
    return out;
  }

  void close() {
    if( fp ) {
      fclose(fp);
      fp = nullptr;
    } else if( cap ) {
      free(buf);          // ours only when it grows; `over_memory`'s is not
      buf = nullptr;
      cap = len = pos = 0;
    }
  }
};

struct BmfFile {
  BmfStream io;
  // Both returns were the same constant -- `nullptr` on the branch that
  // closes, and `f` on the branch where `f` is already null -- and the one
  // caller discarded it.  It also left `fp` pointing at a closed `FILE`, which
  // was harmless only because that caller frees the struct immediately.
  // Close the input and give up.  Eleven sites in `image_expand` wrote
  // `fclose(arc->fp); arc->fp = 0; return nullptr;` out, which is eleven places
  // a reader had to check the file was actually closed.
  BmfImage* fail() {
    close();
    return nullptr;
  }

  // Stage-2 11.5.  The `fseek(fp, 0, SEEK_END)` that stood here answered no
  // format requirement -- one member per file, written front to back, and the
  // position after `fclose` is not a thing anyone can read.
  void close() {
    io.close();
  }
};

// Same shape as `AltP2Block::alt_p2_free`: it returned `arc` after `free(arc)`
// and tested a `do_free` its one caller always passes as 1.
void bmf_close_file(BmfFile* arc) {
  arc->close();
  free(arc);
}

// The BMP reader reads a BMP, which always comes from a path, so it keeps a
// `FILE*` and keeps this.  It is the same one-record-or-no it always was; what
// moved into `BmfStream` is the codec's copy of it.
static inline bool read_exact(void* dst, size_t n, FILE* fp) {
  return n==0||fread(dst, n, 1u, fp)==1;
}

// `read_exact`, `write_all` and `bytes_left` were free functions taking a
// `FILE*`.  They are `BmfStream` methods now, beside the reads and writes they
// are shorthand for -- which is what `methodise.py` says about a first
// parameter that is only ever a receiver, and what the stream being a type
// rather than a typedef makes possible.
//
// `read_bmp` and `read_rle8`/`read_rle4` still take a `FILE*`: they read a BMP,
// which always comes from a path, and pulling them through the stream would put
// the codec's abstraction on `main`'s file I/O for no gain.  Their own
// `fread`-result checks stayed where they were.

BmfFile*bmf_open_file(BmfFile* out, char* path, int32_t read_only) {
  // `wb` to write and `rb` to read.  The donor opened `a+b` -- read *and*
  // append -- because it had members already there to walk past; there is one
  // image in a file now, so writing never reads and reading never writes.
  FILE* fp = fopen(path, read_only ? "rb" : "wb");
  if( !fp )
    bmf_fatal(bmf_no_open, path);
  out->io = BmfStream::over_file(fp);
  return out;
}
