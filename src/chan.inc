// chan.inc -- channel writer/reader, channel->file map, dump header, varints.
//
// The writer API is put(channel, bytes). The decoder reads in exactly the encoder's write order,
// so any set of channels can share one physical file with no framing. The channel->file map is
// stored in the header, which sits at the start of <P>.meta.

typedef std::vector<uint8_t> Bytes;

// A variable-size array with its storage at the maximum size, in place (no heap allocation).
// push_back and resize return false when the array would outgrow N; callers that can meet more
// than N items check it.
template <class T, size_t N>
struct FixedVec {
  T a[N];
  size_t n = 0;
  size_t size() const { return n; }
  bool empty() const { return n == 0; }
  bool full() const { return n == N; }
  static constexpr size_t capacity() { return N; }
  T& operator[](size_t i) { return a[i]; }
  const T& operator[](size_t i) const { return a[i]; }
  T& back() { return a[n - 1]; }
  const T& back() const { return a[n - 1]; }
  T* begin() { return a; }
  T* end() { return a + n; }
  const T* begin() const { return a; }
  const T* end() const { return a + n; }
  bool push_back(const T& v) {
    if (n == N) return false;
    a[n++] = v;
    return true;
  }
  bool push_back(T&& v) {
    if (n == N) return false;
    a[n++] = std::move(v);
    return true;
  }
  bool resize(size_t m) {
    if (m > N) return false;
    for (size_t i = m; i < n; i++) a[i] = T();
    n = m;
    return true;
  }
  void clear() { resize(0); }
};

// Errors. pdfdump builds with -fno-exceptions -fno-rtti: fail() records the first error of the
// calling thread and returns false, and every function that can fail returns bool (or leaves a
// sticky error in its reader) for its caller to pass on. The readers (ChanReader, RBuf) return
// zeros after an error, so code between a failing read and its check runs on harmless values.
// die() is for the command line only: message and exit.
static thread_local bool tFailed = false;
static thread_local char tFailMsg[1024];

static bool fail(const char* fmt, ...) {
  if (!tFailed) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(tFailMsg, sizeof(tFailMsg), fmt, ap);
    va_end(ap);
    tFailed = true;
  }
  return false;
}
static bool failed() { return tFailed; }
static const char* failMessage() { return tFailed ? tFailMsg : ""; }
static void failClear() {
  tFailed = false;
  tFailMsg[0] = 0;
}

// f() as an attempt that may fail (a layer that stays encoded, a lift that is skipped): false if
// it returns false or records an error; the caller's own error state is left as it was
template <class F>
static bool softly(F&& f) {
  bool was = tFailed;
  char msg[sizeof(tFailMsg)];
  memcpy(msg, tFailMsg, sizeof(msg));
  tFailed = false;
  bool ok = f() && !tFailed;
  tFailed = was;
  if (was) memcpy(tFailMsg, msg, sizeof(msg));
  else tFailMsg[0] = 0;
  return ok;
}

[[noreturn]] static void die(const char* fmt, ...) {
  char buf[1024];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  fprintf(stderr, "pdfdump: %s\n", buf);
  exit(1);
}

enum Chan { CH_S, CH_N, CH_T, CH_X, CH_O, CH_F, CH_R, CH_B, CH_COUNT };
static const char kChanLetters[CH_COUNT + 1] = "SNTXOFRB";
static const char* const kChanSuffix[CH_COUNT] = {"meta", "num", "txt", "str", "ops", "fmt", "rec", "bin"};
static const char* const kDefaultChanMap = "SN,TX,O,F,R,B";

static const uint8_t kMagic[8] = {'P', 'D', 'F', 'D', 'U', 'M', 'P', 0x1A};
static const uint32_t kFormatVersion = 9;

// Class files: all streams of a class concatenated, regardless of size.
enum ClassFile { CF_CS, CF_CMAP, CF_COUNT };
static const char* const kClassFileSuffix[CF_COUNT] = {"cs", "cmap"};

// Aux files: outputs of whole-file transforms (number lifting, pdf_lift.inc), signature blobs,
// base64 blobs of object text (data, recon).
enum AuxFile { AX_TXN, AX_CSN, AX_CSH, AX_CMB, AX_CSI, AX_P7S, AX_B64, AX_B64R, AX_CSS, AX_ILN, AX_COUNT };
static const char* const kAuxSuffix[AX_COUNT] = {"txn", "csn", "csh", "cmb", "csi", "p7s", "b64", "b64r", "css", "iln"};
// aux files up to this size are kept at the end of the meta file (xz of the 120-file set: 0 KB
// 7810.5 KB, 4 KB 7803.8 KB, 16 KB 7801.1 KB, 64 KB 7799.1 KB, all 7803.6 KB)
static const size_t kAuxInlineMax = 65536;

// header flags
static const uint64_t kFlagStored = 1, kFlagLiftT = 2, kFlagLiftCS = 4, kFlagCmap = 8;

// A string of at most N bytes kept in place, NUL-terminated (names, suffixes, extensions).
// set() and append() fail, leaving the string as it was, when the result would not fit.
template <size_t N>
struct FixedStr {
  char s[N + 1] = {};
  uint32_t n = 0;
  FixedStr() {}
  explicit FixedStr(const char* p) { set(p); }
  bool set(const char* p, size_t len) {
    if (len > N) return false;
    if (len) memcpy(s, p, len);  // p may be null when len is 0
    s[len] = 0;
    n = (uint32_t)len;
    return true;
  }
  bool set(const char* p) { return set(p, strlen(p)); }
  bool append(const char* p, size_t len) {
    if (len > N - n) return false;
    if (len) memcpy(s + n, p, len);
    n += (uint32_t)len;
    s[n] = 0;
    return true;
  }
  void clear() { n = 0; s[0] = 0; }
  // the first N bytes of a longer input
  void setCut(const char* p, size_t len) { set(p, std::min(len, N)); }
  // printf into the string, cut at N bytes; false if it was cut
  bool format(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(s, N + 1, fmt, ap);
    va_end(ap);
    if (r < 0) { clear(); return false; }
    n = (uint32_t)std::min<size_t>((size_t)r, N);
    return (size_t)r <= N;
  }
  const char* c_str() const { return s; }
  const char* data() const { return s; }
  const char* begin() const { return s; }
  const char* end() const { return s + n; }
  size_t size() const { return n; }
  bool empty() const { return n == 0; }
  bool eq(const char* p, size_t len) const { return n == len && memcmp(s, p, len) == 0; }
  bool operator==(const char* p) const { return eq(p, strlen(p)); }
  bool operator!=(const char* p) const { return !(*this == p); }
  bool operator==(const FixedStr& o) const { return eq(o.s, o.n); }
  bool operator!=(const FixedStr& o) const { return !(*this == o); }
};
typedef FixedStr<15> ShortName;
// PDF names: at most 127 bytes (ISO 32000-1 Annex C)
typedef FixedStr<127> PdfName;
// messages: an error (fail(), at most 1023 bytes) with a short prefix
typedef FixedStr<1279> Message;
// short values in text: a decimal uint64 (20), an MD5 in hex (32), "N G R" (23)
typedef FixedStr<39> ValStr;
// file names: at most 4095 bytes (PATH_MAX)
typedef FixedStr<4095> Path;
// passwords: the handlers read at most the first 127 bytes (R6; R2-R4 read 32)
typedef FixedStr<127> Password;

// Counters keyed by a short string (report tables): at most N keys. Once N - 1 keys are in, any
// other key counts under "(other)".
template <class Key, class V, size_t N>
struct NameTable {
  struct E { Key k; V v; };
  FixedVec<E, N> e;
  V& at(const char* p, size_t len) {
    for (auto& x : e)
      if (x.k.eq(p, len)) return x.v;
    E x{};
    if (e.size() + 1 < N) x.k.setCut(p, len);
    else {
      for (auto& y : e)
        if (y.k == "(other)") return y.v;
      x.k.set("(other)");
    }
    e.push_back(x);
    return e.back().v;
  }
  V& at(const char* p) { return at(p, strlen(p)); }
  bool empty() const { return e.empty(); }
  // entries in byte order of their keys
  void sorted(const E** out) const {
    size_t m = e.size();
    for (size_t i = 0; i < m; i++) out[i] = &e[i];
    std::sort(out, out + m, [](const E* a, const E* b) { return strcmp(a->k.c_str(), b->k.c_str()) < 0; });
  }
};

// Key material (hashes, file and object keys, /O /U /OE /UE): at most 64 bytes, kept in place.
// assign() keeps the first 64 bytes of a longer input (the handlers read at most 48 of any).
struct KeyBuf {
  uint8_t b[64];
  size_t n = 0;
  KeyBuf() {}
  KeyBuf(const uint8_t* p, size_t len) { assign(p, len); }
  void assign(const uint8_t* p, size_t len) {
    n = std::min(len, sizeof(b));
    if (n) memcpy(b, p, n);
  }
  void resize(size_t m) {
    if (m > sizeof(b)) m = sizeof(b);
    if (m > n) memset(b + n, 0, m - n);
    n = m;
  }
  void clear() { n = 0; }
  const uint8_t* data() const { return b; }
  uint8_t* data() { return b; }
  size_t size() const { return n; }
  bool empty() const { return n == 0; }
  uint8_t& operator[](size_t i) { return b[i]; }
  uint8_t operator[](size_t i) const { return b[i]; }
  uint8_t back() const { return b[n - 1]; }
  const uint8_t* begin() const { return b; }
  const uint8_t* end() const { return b + n; }
  bool operator==(const KeyBuf& o) const { return n == o.n && memcmp(b, o.b, n) == 0; }
  bool operator!=(const KeyBuf& o) const { return !(*this == o); }
};

struct ChanMap {
  int fileOf[CH_COUNT];
  ShortName suffix[CH_COUNT];  // per physical file; file 0 is always "meta" (holds S)
  size_t nfiles = 0;

  // spec: comma-separated groups of channel letters, every channel exactly once, e.g. "SN,TX,O,F,R,B".
  // Every group holds at least one of the CH_COUNT channels, so there are at most CH_COUNT.
  // spec: groups of channel letters separated by commas; the groups holding S come first (S is
  // in file 0), each part in spec order. Read in place.
  bool parse(const char* spec, Message& err) {
    for (int c = 0; c < CH_COUNT; c++) fileOf[c] = -1;
    for (const char *p = spec, *g = spec;; p++)
      if (*p == ',' || *p == 0) {
        if (p == g) { err.set("empty channel group"); return false; }
        if (*p == 0) break;
        g = p + 1;
      }
    nfiles = 0;
    for (int pass = 0; pass < 2; pass++)
      for (const char* g = spec;;) {
        const char* e = g;
        bool hasS = false;
        for (; *e && *e != ','; e++) hasS |= toupper((unsigned char)*e) == 'S';
        if (hasS == (pass == 0)) {
          int first = CH_COUNT;
          for (const char* q0 = g; q0 < e; q0++) {
            char ch = (char)toupper((unsigned char)*q0);
            const char* q = strchr(kChanLetters, ch);
            if (!q) { err.format("unknown channel letter '%c'", ch); return false; }
            int c = (int)(q - kChanLetters);
            if (fileOf[c] >= 0) { err.format("channel '%c' given twice", ch); return false; }
            fileOf[c] = (int)nfiles;
            first = std::min(first, c);
          }
          suffix[nfiles++].set(kChanSuffix[first]);  // each group a new channel: at most CH_COUNT
        }
        if (!*e) break;
        g = e + 1;
      }
    for (int c = 0; c < CH_COUNT; c++)
      if (fileOf[c] < 0) { err.format("channel '%c' missing", kChanLetters[c]); return false; }
    if (fileOf[CH_S] != 0) { err.set("internal: S not in file 0"); return false; }
    return true;
  }
  void setDefault() {
    Message err;
    parse(kDefaultChanMap, err);
  }
};

static inline void putVarTo(Bytes& b, uint64_t v) {
  while (v >= 0x80) { b.push_back((uint8_t)(v | 0x80)); v >>= 7; }
  b.push_back((uint8_t)v);
}
static inline uint64_t zigzag(int64_t v) { return ((uint64_t)v << 1) ^ (uint64_t)(v >> 63); }
static inline int64_t unzigzag(uint64_t v) { return (int64_t)(v >> 1) ^ -(int64_t)(v & 1); }

// Values that live in the dump header (decoder needs them; d takes no modelling options).
struct DumpHeader {
  uint32_t version = kFormatVersion;
  uint64_t flags = 0;          // bit0: stored mode
  uint64_t origSize = 0;
  uint32_t origCrc = 0;
  KeyBuf fileKey;              // recovered file encryption key (empty: none)
  uint8_t zlibHdr[2] = {0x78, 0x9C};  // most common zlib header in the file
  uint8_t syntaxLevel = 1, filterLevel = 1;
  uint64_t offsetBias = 0;  // file offsets in xref data are counted from here (junk before %PDF-)
  uint64_t present = 0;  // which files exist: bit f = channel file f, 8 + c = class file, 16 + a = aux
  uint64_t inlineAux = 0;  // aux files kept at the end of the meta file instead (bit a), with their lengths
  uint64_t inlineLen[AX_COUNT] = {};
  ChanMap map;
};
static const int kPresentClass = 8, kPresentAux = 16;

// In-memory image of a complete dump.
struct DumpFiles {
  DumpHeader hdr;
  Bytes chan[CH_COUNT];              // physical channel files (hdr.map.nfiles; data only, no header)
  Bytes classFile[CF_COUNT];
  std::vector<ShortName> ownExt;     // own files by ordinal
  std::vector<Bytes> ownData;
  Bytes aux[AX_COUNT];
  std::vector<ShortName> blobExt;    // large base64 blobs of object text, own files <P>b%07X.<ext>
  std::vector<Bytes> blobData;
};

static void serializeHeader(const DumpHeader& h, Bytes& out) {
  Bytes b;
  putVarTo(b, h.version);
  putVarTo(b, h.flags);
  putVarTo(b, h.origSize);
  for (int i = 0; i < 4; i++) b.push_back((uint8_t)(h.origCrc >> (8 * i)));
  b.push_back((uint8_t)h.map.nfiles);
  for (int c = 0; c < CH_COUNT; c++) b.push_back((uint8_t)h.map.fileOf[c]);
  for (size_t f = 0; f < h.map.nfiles; f++) {
    const ShortName& s = h.map.suffix[f];
    b.push_back((uint8_t)s.size());
    b.insert(b.end(), s.s, s.s + s.n);
  }
  b.push_back((uint8_t)h.fileKey.size());
  b.insert(b.end(), h.fileKey.begin(), h.fileKey.end());
  b.push_back(h.zlibHdr[0]);
  b.push_back(h.zlibHdr[1]);
  b.push_back(h.syntaxLevel);
  b.push_back(h.filterLevel);
  putVarTo(b, h.present);
  putVarTo(b, h.offsetBias);
  putVarTo(b, h.inlineAux);
  for (int a = 0; a < AX_COUNT; a++)
    if (h.inlineAux >> a & 1) putVarTo(b, h.inlineLen[a]);
  out.insert(out.end(), kMagic, kMagic + 8);
  putVarTo(out, b.size());
  out.insert(out.end(), b.begin(), b.end());
}

// Parses the header at the start of the meta file; meta0 = the offset of channel data.
static bool parseHeader(const Bytes& meta, DumpHeader& h, size_t& meta0) {
  size_t p = 0;
  bool ok = true;
  auto need = [&](size_t n) {
    if (ok && n > meta.size() - std::min(p, meta.size())) ok = fail("dump header truncated");
    return ok;
  };
  auto var = [&]() -> uint64_t {
    uint64_t v = 0;
    for (int sh = 0; need(1); sh += 7) {
      uint8_t c = meta[p++];
      if (sh > 63) { ok = fail("bad varint in header"); return 0; }
      v |= (uint64_t)(c & 0x7F) << sh;
      if (!(c & 0x80)) return v;
    }
    return 0;
  };
  if (!need(8)) return false;
  if (memcmp(meta.data(), kMagic, 8) != 0) return fail("not a pdfdump dump (bad magic)");
  p = 8;
  uint64_t len = var();
  if (!ok) return false;
  if (len > meta.size() - p) return fail("dump header truncated");
  size_t end = p + (size_t)len;
  h.version = (uint32_t)var();
  if (!ok) return false;
  if (h.version != kFormatVersion)
    return fail("dump format version %u not supported (this build: %u)", h.version, kFormatVersion);
  h.flags = var();
  h.origSize = var();
  if (!need(4)) return false;
  h.origCrc = meta[p] | (meta[p + 1] << 8) | (meta[p + 2] << 16) | ((uint32_t)meta[p + 3] << 24);
  p += 4;
  if (!need(1 + CH_COUNT)) return false;
  size_t nfiles = meta[p++];
  if (nfiles < 1 || nfiles > CH_COUNT) return fail("bad channel map");
  for (int c = 0; c < CH_COUNT; c++) {
    h.map.fileOf[c] = meta[p++];
    if (h.map.fileOf[c] >= (int)nfiles) return fail("bad channel map");
  }
  h.map.nfiles = nfiles;
  for (size_t i = 0; i < nfiles; i++) {
    if (!need(1)) return false;
    size_t n = meta[p++];
    if (!need(n)) return false;
    for (size_t k = 0; k < n; k++)
      if (!isalnum(meta[p + k])) return fail("bad channel file suffix");
    if (!h.map.suffix[i].set((const char*)&meta[p], n)) return fail("channel file suffix too long");
    p += n;
  }
  if (!need(1)) return false;
  size_t kl = meta[p++];
  if (!need(kl)) return false;
  if (kl > sizeof(h.fileKey.b)) return fail("file key too long");
  h.fileKey.assign(&meta[p], kl);
  p += kl;
  if (!need(4)) return false;
  h.zlibHdr[0] = meta[p++];
  h.zlibHdr[1] = meta[p++];
  h.syntaxLevel = meta[p++];
  h.filterLevel = meta[p++];
  h.present = var();
  h.offsetBias = var();
  h.inlineAux = var();
  if (!ok) return false;
  if (h.inlineAux >> AX_COUNT) return fail("bad inline file set");
  for (int a = 0; a < AX_COUNT; a++)
    if (h.inlineAux >> a & 1) h.inlineLen[a] = var();
  if (!ok) return false;
  if (p > end) return fail("dump header overrun");
  meta0 = end;
  return true;
}

class ChanWriter {
 public:
  explicit ChanWriter(DumpFiles& d) : d_(d) {
    for (auto& c : d_.chan) c.clear();
  }
  void put(int ch, const void* p, size_t n) {
    Bytes& b = d_.chan[d_.hdr.map.fileOf[ch]];
    b.insert(b.end(), (const uint8_t*)p, (const uint8_t*)p + n);
    stat[ch] += n;
  }
  void put(int ch, const Bytes& v) { put(ch, v.data(), v.size()); }
  void u8(int ch, uint8_t v) { put(ch, &v, 1); }
  void var(int ch, uint64_t v) {
    uint8_t tmp[10];
    int n = 0;
    while (v >= 0x80) { tmp[n++] = (uint8_t)(v | 0x80); v >>= 7; }
    tmp[n++] = (uint8_t)v;
    put(ch, tmp, n);
  }
  void svar(int ch, int64_t v) { var(ch, zigzag(v)); }
  // length to lenCh, bytes to dataCh
  void lenBytes(int lenCh, int dataCh, const uint8_t* p, size_t n) { var(lenCh, n); put(dataCh, p, n); }
  void lenBytes(int lenCh, int dataCh, const Bytes& v) { lenBytes(lenCh, dataCh, v.data(), v.size()); }
  void classPut(int cf, const uint8_t* p, size_t n) { d_.classFile[cf].insert(d_.classFile[cf].end(), p, p + n); }
  uint32_t ownAdd(const ShortName& ext, Bytes&& data) {
    d_.ownExt.push_back(ext);
    d_.ownData.push_back(std::move(data));
    return (uint32_t)(d_.ownExt.size() - 1);
  }
  uint64_t stat[CH_COUNT] = {};

 private:
  DumpFiles& d_;
};

class ChanReader {
 public:
  // ownLoader: fetches own file by (ordinal, ext); used when the dump comes from disk.
  typedef std::function<Bytes(uint32_t, const char*)> OwnLoader;  // ordinal, extension
  ChanReader(const DumpFiles& d, size_t meta0, OwnLoader loader, const std::vector<Bytes>* ownData = nullptr)
      : d_(d), loader_(std::move(loader)), ownData_(ownData ? ownData : &d.ownData) {
    for (auto& p : pos_) p = 0;
    pos_[0] = meta0;
    for (int i = 0; i < CF_COUNT; i++) cfPos_[i] = 0;
  }
  // n bytes of channel ch; nullptr (and the reader stays failed) if there are fewer
  const uint8_t* get(int ch, size_t n) {
    if (bad_) return nullptr;
    int f = d_.hdr.map.fileOf[ch];
    const Bytes& b = d_.chan[f];
    if (n > b.size() - pos_[f]) {
      bad_ = true;
      fail("dump truncated: channel %c overrun", kChanLetters[ch]);
      return nullptr;
    }
    static const uint8_t kNone[1] = {0};  // a 0-byte read of an empty file is not an error
    const uint8_t* r = b.empty() ? kNone : b.data() + pos_[f];
    pos_[f] += n;
    return r;
  }
  uint8_t u8(int ch) {
    const uint8_t* p = get(ch, 1);
    return p ? *p : 0;
  }
  uint64_t var(int ch) {
    uint64_t v = 0;
    for (int sh = 0;; sh += 7) {
      uint8_t c = u8(ch);
      if (sh > 63) {
        bad_ = true;
        fail("bad varint in channel %c", kChanLetters[ch]);
        return 0;
      }
      v |= (uint64_t)(c & 0x7F) << sh;
      if (!(c & 0x80)) return v;
    }
  }
  int64_t svar(int ch) { return unzigzag(var(ch)); }
  size_t pos(int ch) const { return pos_[d_.hdr.map.fileOf[ch]]; }
  size_t remaining(int ch) const {
    int f = d_.hdr.map.fileOf[ch];
    return d_.chan[f].size() - pos_[f];
  }
  size_t len(int ch, size_t limit = (size_t)1 << 40) {
    uint64_t v = var(ch);
    if (v > limit) {
      bad_ = true;
      fail("implausible length in channel %c", kChanLetters[ch]);
      return 0;
    }
    return (size_t)v;
  }
  void lenBytes(int lenCh, int dataCh, Bytes& out) {
    size_t n = len(lenCh);
    const uint8_t* p = get(dataCh, n);
    if (p) out.assign(p, p + n);
    else out.clear();
  }
  void classGet(int cf, size_t n, Bytes& out) {
    out.clear();
    if (bad_) return;
    const Bytes& b = d_.classFile[cf];
    if (n > b.size() - cfPos_[cf]) {
      bad_ = true;
      fail("dump truncated: class file .%s overrun", kClassFileSuffix[cf]);
      return;
    }
    out.assign(b.begin() + cfPos_[cf], b.begin() + cfPos_[cf] + n);
    cfPos_[cf] += n;
  }
  // an own file; empty, with the error recorded, if it cannot be had
  Bytes own(uint32_t ordinal, const char* ext) {
    if (ordinal < ownData_->size()) return (*ownData_)[ordinal];
    if (!loader_) {
      fail("own file %08X missing", ordinal);
      return Bytes();
    }
    return loader_(ordinal, ext);
  }
  bool ok() const { return !bad_; }
  bool allConsumed() const {
    if (bad_) return false;
    for (size_t f = 0; f < d_.hdr.map.nfiles; f++)
      if (pos_[f] != d_.chan[f].size()) return false;
    for (int i = 0; i < CF_COUNT; i++)
      if (cfPos_[i] != d_.classFile[i].size()) return false;
    return true;
  }

 private:
  const DumpFiles& d_;
  OwnLoader loader_;
  const std::vector<Bytes>* ownData_;
  size_t pos_[CH_COUNT];
  size_t cfPos_[CF_COUNT];
  bool bad_ = false;
};

// ---------------------------------------------------------------------------------------------
// misc helpers

static uint32_t crc32_update(uint32_t crc, const uint8_t* p, size_t n) {
  static uint32_t table[256];
  static bool init = false;
  if (!init) {
    for (uint32_t i = 0; i < 256; i++) {
      uint32_t c = i;
      for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
      table[i] = c;
    }
    init = true;
  }
  crc = ~crc;
  for (size_t i = 0; i < n; i++) crc = table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
  return ~crc;
}

// encoder-side lookup hash (dedup, verify shortcut); not part of the dump format
static uint64_t hash64(const uint8_t* p, size_t n) {
  uint64_t h = 0x9E3779B97F4A7C15ull ^ (uint64_t)n;
  for (; n >= 8; p += 8, n -= 8) {
    uint64_t w;
    memcpy(&w, p, 8);
    h = (h ^ w) * 0xFF51AFD7ED558CCDull;
    h ^= h >> 29;
  }
  for (; n; p++, n--) h = (h ^ *p) * 0x100000001B3ull;
  h ^= h >> 33;
  h *= 0xC4CEB9FE1A85EC53ull;
  return h ^ (h >> 33);
}

static bool readFile(const char* name, Bytes& out) {
  FILE* f = fopen(name, "rb");
  if (!f) return false;
  out.clear();
  uint8_t buf[1 << 16];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), f)) > 0) out.insert(out.end(), buf, buf + n);
  bool ok = !ferror(f);
  fclose(f);
  return ok;
}

static void writeFile(const char* name, const uint8_t* p, size_t n) {
  FILE* f = fopen(name, "wb");
  if (!f) die("cannot create %s: %s", name, strerror(errno));
  if (n && fwrite(p, 1, n, f) != n) { fclose(f); die("write error on %s", name); }
  if (fclose(f) != 0) die("write error on %s", name);
}

// prefix + "." + suffix
static Path dumpFileName(const char* prefix, const char* suffix) {
  Path r;
  if (!r.format("%s.%s", prefix, suffix)) die("file name too long: %s.%s", prefix, suffix);
  return r;
}

static const uint32_t kBlobFileBit = 0x80000000u;  // OwnLoader: blob file k is ordinal kBlobFileBit | k

static Path ownFileName(const char* prefix, uint32_t ordinal, const char* ext) {
  Path r;
  bool ok = ordinal & kBlobFileBit ? r.format("%sb%07X.%s", prefix, ordinal & ~kBlobFileBit, ext)
                                   : r.format("%s%08X.%s", prefix, ordinal, ext);
  if (!ok) die("file name too long: %s...", prefix);
  return r;
}
