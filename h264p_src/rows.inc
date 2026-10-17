// TSV rows: a name followed by tab-separated values, one row per line.

namespace h264p {

struct RowWriter {
  void begin(const char* name) { s_ += name; }
  void add(int64_t v) {
    char tmp[24];
    char* p = tmp + sizeof tmp;
    uint64_t u = v < 0 ? 0 - (uint64_t)v : (uint64_t)v;
    do *--p = (char)('0' + u % 10); while (u /= 10);
    if (v < 0) *--p = '-';
    s_ += '\t';
    s_.append(p, tmp + sizeof tmp - p);
  }
  void add(const std::string& v) {
    s_ += '\t';
    s_ += v;
  }
  void add_absent() { s_ += "\t-"; }
  void end() { s_ += '\n'; }
  void row(const char* name, int64_t v) {
    begin(name);
    add(v);
    end();
  }
  std::string& text() { return s_; }
  size_t size() const { return s_.size(); }
  void truncate(size_t n) { s_.resize(n); }

  std::string s_;
};

// Names of the rows of the slice data, classified when a row is read.
enum RowKind { kOtherRow, kMbRow, kSkipRow, kResRow, kIpmRow, kSubRow, kRef0Row, kRef1Row, kMvd0Row,
               kMvd1Row, kFldRow, kPcmRow };

struct Row {
  std::string line;            // the line with tabs replaced by NULs
  std::vector<uint32_t> off;   // start of each field
  long lineno = 0;
  RowKind kind = kOtherRow;

  size_t size() const { return off.size(); }
  const char* field(size_t i) const { return line.c_str() + off[i]; }
  const char* name() const { return field(0); }
  bool is(const char* n) const { return strcmp(name(), n) == 0; }

  // the field is "-"
  bool absent(size_t i) const {
    if (i >= size()) fail("missing field " + std::to_string(i));
    const char* f = field(i);
    return f[0] == '-' && f[1] == 0;
  }

  // throws TsvError
  int64_t num(size_t i) const {
    if (i >= size()) fail("missing field " + std::to_string(i));
    const char* f = field(i);
    // short decimal numbers directly, the rest with strtoll
    const char* p = f + (*f == '-');
    int64_t v = 0;
    int n = 0;
    while (*p >= '0' && *p <= '9' && n < 18) v = v * 10 + (*p++ - '0'), n++;
    if (n && !*p) return *f == '-' ? -v : v;
    char* end;
    errno = 0;
    long long x = strtoll(f, &end, 10);
    if (end == f || *end || errno) fail(std::string("not a number: '") + f + "'");
    return x;
  }

  [[noreturn]] void fail(const std::string& msg) const {
    throw TsvError("line " + std::to_string(lineno) + " (" + name() + "): " + msg);
  }
};

static RowKind classify(const char* n) {
  switch (n[0]) {
    case 'm':
      if (!strcmp(n, "mb")) return kMbRow;
      if (!strcmp(n, "mvd0")) return kMvd0Row;
      if (!strcmp(n, "mvd1")) return kMvd1Row;
      break;
    case 'r':
      if (!strcmp(n, "res")) return kResRow;
      if (!strcmp(n, "ref0")) return kRef0Row;
      if (!strcmp(n, "ref1")) return kRef1Row;
      break;
    case 's':
      if (!strcmp(n, "skip")) return kSkipRow;
      if (!strcmp(n, "sub")) return kSubRow;
      break;
    case 'i':
      if (!strcmp(n, "ipm")) return kIpmRow;
      break;
    case 'f':
      if (!strcmp(n, "fld")) return kFldRow;
      break;
    case 'p':
      if (!strcmp(n, "pcm")) return kPcmRow;
      break;
  }
  return kOtherRow;
}

struct RowReader {
  explicit RowReader(FILE* f) : f_(f) {}
  RowReader(const char* text, size_t len) : f_(nullptr) { buf_.assign(text, len); }

  // the next row without consuming it, or nullptr at the end
  const Row* peek() {
    while (!have_next_) {
      if (!read_line(next_.line)) return nullptr;
      lineno_++;
      std::string& l = next_.line;
      if (!l.empty() && l.back() == '\r') l.pop_back();
      if (l.empty() || l[0] == '#') continue;  // blank lines and comments
      next_.off.clear();
      next_.off.push_back(0);
      for (size_t i = 0; i < l.size(); i++)
        if (l[i] == '\t') {
          l[i] = 0;
          next_.off.push_back((uint32_t)i + 1);
        }
      next_.lineno = lineno_;
      next_.kind = classify(l.c_str());
      have_next_ = true;
    }
    return &next_;
  }

  // the next row; throws at the end
  const Row& take() {
    if (!peek()) throw TsvError("unexpected end of file");
    std::swap(cur_, next_);
    have_next_ = false;
    return cur_;
  }

  bool at_end() { return peek() == nullptr; }
  long lineno() const { return lineno_; }

  bool fill() {
    if (!f_ || eof_) return false;
    if (bpos_ > 0) {
      buf_.erase(0, bpos_);
      bpos_ = 0;
    }
    size_t old = buf_.size();
    buf_.resize(old + (1 << 20));
    size_t n = fread(&buf_[old], 1, 1 << 20, f_);
    buf_.resize(old + n);
    if (n == 0) eof_ = true;
    return n > 0;
  }

  bool read_line(std::string& out) {
    for (;;) {
      size_t nl = buf_.find('\n', bpos_);
      if (nl != std::string::npos) {
        out.assign(buf_, bpos_, nl - bpos_);
        bpos_ = nl + 1;
        return true;
      }
      if (!fill()) {
        if (bpos_ < buf_.size()) {  // last line without a newline
          out.assign(buf_, bpos_, std::string::npos);
          bpos_ = buf_.size();
          return true;
        }
        return false;
      }
    }
  }

  FILE* f_;
  std::string buf_;
  size_t bpos_ = 0;
  bool eof_ = false;
  long lineno_ = 0;
  Row next_, cur_;
  bool have_next_ = false;
};

std::string to_hex(const uint8_t* p, size_t n);
// throws TsvError
std::vector<uint8_t> from_hex(const Row& r, size_t field);

std::string to_hex(const uint8_t* p, size_t n) {
  static const char d[] = "0123456789abcdef";
  std::string s(2 * n, '0');
  for (size_t i = 0; i < n; i++) {
    s[2 * i] = d[p[i] >> 4];
    s[2 * i + 1] = d[p[i] & 15];
  }
  return s;
}

static int hexval(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

std::vector<uint8_t> from_hex(const Row& r, size_t field) {
  std::vector<uint8_t> v;
  if (field >= r.size()) return v;  // empty data
  const char* s = r.field(field);
  size_t n = strlen(s);
  if (n % 2) r.fail("odd number of hex digits");
  v.resize(n / 2);
  for (size_t i = 0; i < n / 2; i++) {
    int a = hexval(s[2 * i]), b = hexval(s[2 * i + 1]);
    if (a < 0 || b < 0) r.fail("bad hex digit");
    v[i] = (uint8_t)(a << 4 | b);
  }
  return v;
}

}  // namespace h264p
