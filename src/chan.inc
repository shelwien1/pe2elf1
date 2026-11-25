// chan.inc -- channel writer/reader, channel->file map, dump header, varints.
//
// The writer API is put(channel, bytes). The decoder reads in exactly the encoder's write order,
// so any set of channels can share one physical file with no framing. The channel->file map is
// stored in the header, which sits at the start of <P>.meta.

typedef std::vector<uint8_t> Bytes;

struct Error : std::runtime_error {
  explicit Error(const std::string& s) : std::runtime_error(s) {}
};

[[noreturn]] static void fail(const char* fmt, ...) {
  char buf[1024];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  throw Error(buf);
}

enum Chan { CH_S, CH_N, CH_T, CH_X, CH_O, CH_F, CH_R, CH_B, CH_COUNT };
static const char kChanLetters[CH_COUNT + 1] = "SNTXOFRB";
static const char* const kChanSuffix[CH_COUNT] = {"meta", "num", "txt", "str", "ops", "fmt", "rec", "bin"};
static const char* const kDefaultChanMap = "SN,TX,O,F,R,B";

static const uint8_t kMagic[8] = {'P', 'D', 'F', 'D', 'U', 'M', 'P', 0x1A};
static const uint32_t kFormatVersion = 6;

// Class files: all streams of a class concatenated, regardless of size.
enum ClassFile { CF_CS, CF_CMAP, CF_COUNT };
static const char* const kClassFileSuffix[CF_COUNT] = {"cs", "cmap"};

// Aux files: outputs of whole-file transforms (number lifting, pdf_lift.inc), signature blobs,
// base64 blobs of object text (data, recon).
enum AuxFile { AX_TXN, AX_CSN, AX_CSH, AX_CMB, AX_CSI, AX_P7S, AX_B64, AX_B64R, AX_CSS, AX_COUNT };
static const char* const kAuxSuffix[AX_COUNT] = {"txn", "csn", "csh", "cmb", "csi", "p7s", "b64", "b64r", "css"};

// header flags
static const uint64_t kFlagStored = 1, kFlagLiftT = 2, kFlagLiftCS = 4, kFlagCmap = 8;

struct ChanMap {
  int fileOf[CH_COUNT];
  std::vector<std::string> suffix;  // per physical file; file 0 is always "meta" (holds S)

  // spec: comma-separated groups of channel letters, every channel exactly once, e.g. "SN,TX,O,F,R,B".
  bool parse(const char* spec, std::string& err) {
    for (int c = 0; c < CH_COUNT; c++) fileOf[c] = -1;
    std::vector<std::string> groups;
    std::string cur;
    for (const char* p = spec;; p++) {
      if (*p == ',' || *p == 0) {
        if (cur.empty()) { err = "empty channel group"; return false; }
        groups.push_back(cur);
        cur.clear();
        if (*p == 0) break;
      } else {
        cur += (char)toupper((unsigned char)*p);
      }
    }
    // the group holding S becomes file 0
    std::stable_sort(groups.begin(), groups.end(), [](const std::string& a, const std::string& b) {
      return (a.find('S') != std::string::npos) > (b.find('S') != std::string::npos);
    });
    suffix.clear();
    for (size_t g = 0; g < groups.size(); g++) {
      int first = CH_COUNT;
      for (char ch : groups[g]) {
        const char* q = strchr(kChanLetters, ch);
        if (!q || !ch) { err = std::string("unknown channel letter '") + ch + "'"; return false; }
        int c = (int)(q - kChanLetters);
        if (fileOf[c] >= 0) { err = std::string("channel '") + ch + "' given twice"; return false; }
        fileOf[c] = (int)g;
        first = std::min(first, c);
      }
      suffix.push_back(kChanSuffix[first]);
    }
    for (int c = 0; c < CH_COUNT; c++)
      if (fileOf[c] < 0) { err = std::string("channel '") + kChanLetters[c] + "' missing"; return false; }
    if (fileOf[CH_S] != 0) { err = "internal: S not in file 0"; return false; }
    return true;
  }
  void setDefault() {
    std::string err;
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
  Bytes fileKey;               // recovered file encryption key (empty: none)
  uint8_t zlibHdr[2] = {0x78, 0x9C};  // most common zlib header in the file
  uint8_t syntaxLevel = 1, filterLevel = 1;
  uint64_t offsetBias = 0;  // file offsets in xref data are counted from here (junk before %PDF-)
  uint64_t present = 0;  // which files exist: bit f = channel file f, 8 + c = class file, 16 + a = aux
  ChanMap map;
};
static const int kPresentClass = 8, kPresentAux = 16;

// In-memory image of a complete dump.
struct DumpFiles {
  DumpHeader hdr;
  std::vector<Bytes> chan;           // physical channel files (channel data only, header excluded)
  Bytes classFile[CF_COUNT];
  std::vector<std::string> ownExt;   // own files by ordinal
  std::vector<Bytes> ownData;
  Bytes aux[AX_COUNT];
  std::vector<std::string> blobExt;  // large base64 blobs of object text, own files <P>b%07X.<ext>
  std::vector<Bytes> blobData;
};

static void serializeHeader(const DumpHeader& h, Bytes& out) {
  Bytes b;
  putVarTo(b, h.version);
  putVarTo(b, h.flags);
  putVarTo(b, h.origSize);
  for (int i = 0; i < 4; i++) b.push_back((uint8_t)(h.origCrc >> (8 * i)));
  b.push_back((uint8_t)h.map.suffix.size());
  for (int c = 0; c < CH_COUNT; c++) b.push_back((uint8_t)h.map.fileOf[c]);
  for (auto& s : h.map.suffix) { b.push_back((uint8_t)s.size()); b.insert(b.end(), s.begin(), s.end()); }
  b.push_back((uint8_t)h.fileKey.size());
  b.insert(b.end(), h.fileKey.begin(), h.fileKey.end());
  b.push_back(h.zlibHdr[0]);
  b.push_back(h.zlibHdr[1]);
  b.push_back(h.syntaxLevel);
  b.push_back(h.filterLevel);
  putVarTo(b, h.present);
  putVarTo(b, h.offsetBias);
  out.insert(out.end(), kMagic, kMagic + 8);
  putVarTo(out, b.size());
  out.insert(out.end(), b.begin(), b.end());
}

// Parses the header at the start of the meta file; returns the offset of channel data.
static size_t parseHeader(const Bytes& meta, DumpHeader& h) {
  size_t p = 0;
  auto need = [&](size_t n) { if (p + n > meta.size()) fail("dump header truncated"); };
  auto var = [&]() -> uint64_t {
    uint64_t v = 0;
    for (int sh = 0;; sh += 7) {
      need(1);
      uint8_t c = meta[p++];
      if (sh > 63) fail("bad varint in header");
      v |= (uint64_t)(c & 0x7F) << sh;
      if (!(c & 0x80)) return v;
    }
  };
  need(8);
  if (memcmp(meta.data(), kMagic, 8) != 0) fail("not a pdfdump dump (bad magic)");
  p = 8;
  uint64_t len = var();
  size_t end = p + (size_t)len;
  if (end > meta.size()) fail("dump header truncated");
  h.version = (uint32_t)var();
  if (h.version != kFormatVersion) fail("dump format version %u not supported (this build: %u)", h.version, kFormatVersion);
  h.flags = var();
  h.origSize = var();
  need(4);
  h.origCrc = meta[p] | (meta[p + 1] << 8) | (meta[p + 2] << 16) | ((uint32_t)meta[p + 3] << 24);
  p += 4;
  need(1 + CH_COUNT);
  size_t nfiles = meta[p++];
  if (nfiles < 1 || nfiles > CH_COUNT) fail("bad channel map");
  for (int c = 0; c < CH_COUNT; c++) {
    h.map.fileOf[c] = meta[p++];
    if (h.map.fileOf[c] >= (int)nfiles) fail("bad channel map");
  }
  h.map.suffix.clear();
  for (size_t i = 0; i < nfiles; i++) {
    need(1);
    size_t n = meta[p++];
    need(n);
    std::string s((const char*)&meta[p], n);
    for (char ch : s)
      if (!isalnum((unsigned char)ch)) fail("bad channel file suffix");
    h.map.suffix.push_back(s);
    p += n;
  }
  need(1);
  size_t kl = meta[p++];
  need(kl);
  h.fileKey.assign(meta.begin() + p, meta.begin() + p + kl);
  p += kl;
  need(4);
  h.zlibHdr[0] = meta[p++];
  h.zlibHdr[1] = meta[p++];
  h.syntaxLevel = meta[p++];
  h.filterLevel = meta[p++];
  h.present = var();
  h.offsetBias = var();
  if (p > end) fail("dump header overrun");
  return end;
}

class ChanWriter {
 public:
  explicit ChanWriter(DumpFiles& d) : d_(d) { d_.chan.assign(d_.hdr.map.suffix.size(), Bytes()); }
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
  uint32_t ownAdd(const std::string& ext, Bytes&& data) {
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
  typedef std::function<Bytes(uint32_t, const std::string&)> OwnLoader;
  ChanReader(const DumpFiles& d, size_t meta0, OwnLoader loader, const std::vector<Bytes>* ownData = nullptr)
      : d_(d), loader_(std::move(loader)), ownData_(ownData ? ownData : &d.ownData) {
    pos_.assign(d.chan.size(), 0);
    if (!pos_.empty()) pos_[0] = meta0;
    for (int i = 0; i < CF_COUNT; i++) cfPos_[i] = 0;
  }
  const uint8_t* get(int ch, size_t n) {
    int f = d_.hdr.map.fileOf[ch];
    const Bytes& b = d_.chan[f];
    if (n > b.size() - pos_[f]) fail("dump truncated: channel %c overrun", kChanLetters[ch]);
    const uint8_t* r = b.data() + pos_[f];
    pos_[f] += n;
    return r;
  }
  uint8_t u8(int ch) { return *get(ch, 1); }
  uint64_t var(int ch) {
    uint64_t v = 0;
    for (int sh = 0;; sh += 7) {
      uint8_t c = u8(ch);
      if (sh > 63) fail("bad varint in channel %c", kChanLetters[ch]);
      v |= (uint64_t)(c & 0x7F) << sh;
      if (!(c & 0x80)) return v;
    }
  }
  int64_t svar(int ch) { return unzigzag(var(ch)); }
  size_t len(int ch, size_t limit = (size_t)1 << 40) {
    uint64_t v = var(ch);
    if (v > limit) fail("implausible length in channel %c", kChanLetters[ch]);
    return (size_t)v;
  }
  void lenBytes(int lenCh, int dataCh, Bytes& out) {
    size_t n = len(lenCh);
    const uint8_t* p = get(dataCh, n);
    out.assign(p, p + n);
  }
  void classGet(int cf, size_t n, Bytes& out) {
    const Bytes& b = d_.classFile[cf];
    if (n > b.size() - cfPos_[cf]) fail("dump truncated: class file .%s overrun", kClassFileSuffix[cf]);
    out.assign(b.begin() + cfPos_[cf], b.begin() + cfPos_[cf] + n);
    cfPos_[cf] += n;
  }
  Bytes own(uint32_t ordinal, const std::string& ext) {
    if (ordinal < ownData_->size()) return (*ownData_)[ordinal];
    if (!loader_) fail("own file %08X missing", ordinal);
    return loader_(ordinal, ext);
  }
  bool allConsumed() const {
    for (size_t f = 0; f < pos_.size(); f++)
      if (pos_[f] != d_.chan[f].size()) return false;
    for (int i = 0; i < CF_COUNT; i++)
      if (cfPos_[i] != d_.classFile[i].size()) return false;
    return true;
  }

 private:
  const DumpFiles& d_;
  OwnLoader loader_;
  const std::vector<Bytes>* ownData_;
  std::vector<size_t> pos_;
  size_t cfPos_[CF_COUNT];
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

static bool readFile(const std::string& name, Bytes& out) {
  FILE* f = fopen(name.c_str(), "rb");
  if (!f) return false;
  out.clear();
  uint8_t buf[1 << 16];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), f)) > 0) out.insert(out.end(), buf, buf + n);
  bool ok = !ferror(f);
  fclose(f);
  return ok;
}

static void writeFile(const std::string& name, const uint8_t* p, size_t n) {
  FILE* f = fopen(name.c_str(), "wb");
  if (!f) fail("cannot create %s: %s", name.c_str(), strerror(errno));
  if (n && fwrite(p, 1, n, f) != n) { fclose(f); fail("write error on %s", name.c_str()); }
  if (fclose(f) != 0) fail("write error on %s", name.c_str());
}

static const uint32_t kBlobFileBit = 0x80000000u;  // OwnLoader: blob file k is ordinal kBlobFileBit | k

static std::string ownFileName(const std::string& prefix, uint32_t ordinal, const std::string& ext) {
  if (ordinal & kBlobFileBit) {
    char buf[16];
    snprintf(buf, sizeof(buf), "b%07X", ordinal & ~kBlobFileBit);
    return prefix + buf + "." + ext;
  }
  char buf[16];
  snprintf(buf, sizeof(buf), "%08X", ordinal);
  return prefix + buf + "." + ext;
}
