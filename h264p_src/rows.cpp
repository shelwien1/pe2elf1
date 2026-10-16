#include "rows.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>

namespace h264p {

bool Row::is(const char* n) const { return strcmp(name(), n) == 0; }

bool Row::absent(size_t i) const {
  if (i >= size()) fail("missing field " + std::to_string(i));
  const char* f = field(i);
  return f[0] == '-' && f[1] == 0;
}

int64_t Row::num(size_t i) const {
  if (i >= size()) fail("missing field " + std::to_string(i));
  const char* f = field(i);
  char* end;
  errno = 0;
  long long v = strtoll(f, &end, 10);
  if (end == f || *end || errno) fail(std::string("not a number: '") + f + "'");
  return v;
}

void Row::fail(const std::string& msg) const {
  throw TsvError("line " + std::to_string(lineno) + " (" + name() + "): " + msg);
}

bool RowReader::fill() {
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

bool RowReader::read_line(std::string& out) {
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

const Row* RowReader::peek() {
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
    have_next_ = true;
  }
  return &next_;
}

const Row& RowReader::take() {
  if (!peek()) throw TsvError("unexpected end of file");
  std::swap(cur_, next_);
  have_next_ = false;
  return cur_;
}

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
