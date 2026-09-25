// tsv.h - minimal TSV reader + float formatting shared by predict.cpp/metric.cpp
#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cmath>
#include <string>
#include <vector>

inline void die(const char* fmt, ...) {
  va_list ap; va_start(ap, fmt);
  fprintf(stderr, "error: "); vfprintf(stderr, fmt, ap); fprintf(stderr, "\n");
  va_end(ap); exit(1);
}

// Whole file is loaded into memory; fields are NUL-terminated in place.
// First line is the header with column names.
struct TSV {
  std::string fname;
  std::vector<std::string> names;
  std::vector<char> buf;
  std::vector<char*> cell; // nrows*ncols
  size_t ncols = 0, nrows = 0;

  void load(const char* fn) {
    fname = fn;
    FILE* f = fopen(fn, "rb"); if (!f) die("can't open %s", fn);
    fseek(f, 0, SEEK_END); long len = ftell(f); fseek(f, 0, SEEK_SET);
    buf.resize(size_t(len) + 1);
    if (len > 0 && fread(&buf[0], 1, len, f) != size_t(len)) die("can't read %s", fn);
    fclose(f);
    buf[len] = 0;
    char* p = &buf[0]; char* end = p + len;
    size_t line = 0;
    std::vector<char*> row;
    while (p < end) {
      char* e = (char*)memchr(p, '\n', end - p); if (!e) e = end;
      char* le = e; if (le > p && le[-1] == '\r') --le;
      *le = 0; line++;
      if (le > p) { // skip empty lines
        row.clear(); row.push_back(p);
        for (char* q = p; q < le; q++) if (*q == '\t') { *q = 0; row.push_back(q + 1); }
        if (names.empty()) {
          for (size_t i = 0; i < row.size(); i++) names.push_back(row[i]);
          ncols = names.size();
        } else {
          if (row.size() != ncols) die("%s:%zu: %zu fields, header has %zu", fn, line, row.size(), ncols);
          cell.insert(cell.end(), row.begin(), row.end());
          nrows++;
        }
      }
      p = e + 1;
    }
    if (names.empty()) die("%s: empty file", fn);
  }
  int col(const char* name) const {
    for (size_t i = 0; i < ncols; i++) if (names[i] == name) return int(i);
    return -1;
  }
  int need(const char* name) const {
    int c = col(name); if (c < 0) die("%s: no column '%s'", fname.c_str(), name);
    return c;
  }
  const char* at(size_t r, int c) const { return cell[r * ncols + c]; }
  float f(size_t r, int c) const {
    const char* s = at(r, c); char* e;
    float v = strtof(s, &e);
    if (e == s || *e) die("%s: row %zu col %s: bad number '%s'", fname.c_str(), r + 1, names[c].c_str(), s);
    return v;
  }
  long long i(size_t r, int c) const {
    const char* s = at(r, c); char* e;
    long long v = strtoll(s, &e, 10);
    if (e == s || *e) die("%s: row %zu col %s: bad integer '%s'", fname.c_str(), r + 1, names[c].c_str(), s);
    return v;
  }
  bool b(size_t r, int c) const { // 0/1 or True/False (pandas)
    const char* s = at(r, c);
    if ((s[0] == '0' || s[0] == '1') && !s[1]) return s[0] == '1';
    if (!strcmp(s, "True") || !strcmp(s, "true")) return true;
    if (!strcmp(s, "False") || !strcmp(s, "false")) return false;
    return i(r, c) != 0;
  }
};

// Shortest "%g" text that reads back (strtof) as exactly the same float.
inline int fmt_float(char* out, float v) {
  for (int p = 1; p < 9; p++) {
    int n = sprintf(out, "%.*g", p, v);
    if (strtof(out, 0) == v) return n;
  }
  return sprintf(out, "%.9g", v);
}

// Feature column names in the official order (utils.FEATURE_COLUMNS)
inline std::vector<std::string> feature_names() {
  std::vector<std::string> r;
  const char* g[4] = {"p", "v", "dp", "dv"}; const int n[4] = {22, 22, 4, 4};
  char s[32];
  for (int i = 0; i < 2; i++)
    for (int k = 0; k < 4; k++)
      for (int j = 0; j < n[k]; j++) { sprintf(s, "i%d_%s%d", i, g[k], j); r.push_back(s); }
  for (int i = 0; i < 8; i++) { sprintf(s, "a%d", i); r.push_back(s); }
  return r;
}
enum { N_FEATURES = 112 };
