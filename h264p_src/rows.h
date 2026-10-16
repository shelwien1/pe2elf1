// TSV rows: a name followed by tab-separated values, one row per line.
#ifndef H264P_ROWS_H
#define H264P_ROWS_H

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "bits.h"

namespace h264p {

class RowWriter {
 public:
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

 private:
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
  bool is(const char* n) const;
  bool absent(size_t i) const;     // the field is "-"
  int64_t num(size_t i) const;     // throws TsvError
  [[noreturn]] void fail(const std::string& msg) const;
};

class RowReader {
 public:
  explicit RowReader(FILE* f) : f_(f) {}
  RowReader(const char* text, size_t len) : f_(nullptr) { buf_.assign(text, len); }
  // the next row without consuming it, or nullptr at the end
  const Row* peek();
  // the next row; throws at the end
  const Row& take();
  bool at_end() { return peek() == nullptr; }
  long lineno() const { return lineno_; }

 private:
  bool fill();
  bool read_line(std::string& out);

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

}  // namespace h264p

#endif
