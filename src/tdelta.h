// tdelta.h - delta coding of tables of fixed-size records inside data.
//
// Finds runs of N-byte records (N = 1..30) whose leading 16-bit value moves
// in small monotonic steps (sorted offsets, counters, sizes, fixed-point
// curves) and replaces every record but the first by its difference from the
// previous record: little-endian subtraction with the borrow carried across
// adjacent differenced bytes.  For N other than 2, 4 and 8, byte columns
// that rarely change are kept as they are and moved in front of the table.
// This is the table filter of Bulat Ziganshin's delta (FreeArc), with the
// same detection heuristics; the encoder decides, the decoder only applies
// the list of tables it is given.  The cost model uses integer arithmetic so
// that the choice does not depend on the floating-point library.
#pragma once
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <vector>

namespace tdelta {

struct Table {
  uint64_t off;   // start in the buffer
  uint32_t type;  // 1 << N | k-th bit set: column k is kept as is
  uint32_t rows;
};

enum { kMaxN = 30, kSmallN = 4, kCycles = 4, kLine = 32, kStep = 8, kPad = 8 * kMaxN + 64 };

static inline int typeN(uint32_t type) {
  int n = -1;
  while (type) { type >>= 1; n++; }
  return n;
}

static inline int s16(const uint8_t* p) { int16_t v; memcpy(&v, p, 2); return v; }
static inline uint32_t u32(const uint8_t* p) { uint32_t v; memcpy(&v, p, 4); return v; }

// difference of each record from the previous one, last record first
static void diffRows(uint8_t* t, int n, uint32_t rows, const bool* keep) {
  for (uint8_t* r = t + (size_t)n * (rows - 1); r > t; r -= n) {
    unsigned borrow = 0;
    for (int i = 0; i < n; i++) {
      if (keep[i]) { borrow = 0; continue; }
      unsigned sub = r[i - n] + borrow;
      borrow = r[i] < sub;
      r[i] = (uint8_t)(r[i] - sub);
    }
  }
}

static void undiffRows(uint8_t* t, int n, uint32_t rows, const bool* keep) {
  for (uint8_t* r = t + n; r < t + (size_t)n * rows; r += n) {
    unsigned carry = 0;
    for (int i = 0; i < n; i++) {
      if (keep[i]) { carry = 0; continue; }
      unsigned sum = r[i] + r[i - n] + carry;
      r[i] = (uint8_t)sum;
      carry = sum >= 256;
    }
  }
}

// kept columns first (row by row), then the differenced ones
static void gather(uint8_t* t, int n, uint32_t rows, const bool* keep, bool fwd) {
  int nk = 0;
  for (int i = 0; i < n; i++) nk += keep[i];
  if (nk == 0 || nk == n) return;
  size_t len = (size_t)n * rows;
  std::vector<uint8_t> tmp(t, t + len);
  uint8_t* a = fwd ? t : tmp.data();                 // kept part
  uint8_t* b = a + (size_t)nk * rows;                // differenced part
  uint8_t* q = fwd ? tmp.data() : t;                 // records
  for (uint32_t r = 0; r < rows; r++)
    for (int i = 0; i < n; i++, q++) {
      uint8_t*& d = keep[i] ? a : b;
      if (fwd) *d++ = *q; else *q = *d++;
    }
}

// fixed-point log2(x) * 256 for x >= 1
static int log2fp(uint64_t x) {
  int e = 0;
  while (x >> (e + 1)) e++;
  // mantissa in [1, 2) as 1.31 fixed point, squared 8 times for 8 fraction bits
  uint64_t m = e >= 31 ? x >> (e - 31) : x << (31 - e);
  int f = 0;
  for (int k = 0; k < 8; k++) {
    m = (m * m) >> 31;
    f <<= 1;
    if (m >> 32) { m >>= 1; f |= 1; }
  }
  return e * 256 + f;
}

// sqrt(N) * 256, N = 0..30
static const int kSqrt[kMaxN + 1] = {0,    256,  362,  443,  512,  572,  627,  677,  724,  768,  810,
                                     849,  887,  923,  958,  991,  1024, 1056, 1086, 1116, 1145, 1173,
                                     1201, 1228, 1254, 1280, 1305, 1330, 1355, 1379, 1402};

struct Finder {
  uint8_t* lo;   // start of the buffer (reads may go a little below, into padding)
  uint8_t* end;  // end of the data (padding follows)

  // one end of a table that contains t: the last point before the leading
  // 16-bit value stops moving monotonically for more than a couple of
  // records (n < 0: search backwards)
  uint8_t* bound(int n, uint8_t* t, const uint8_t* start, int& useless) const {
    int dir = s16(t + n) - s16(t) < 0 ? -1 : 1;
    int len = 0, omit = 0, uc = 0, bad = 0;
    useless = 0;
    uint8_t* last = t;
    bool first = true;
    for (t += n; start <= t + n && t + n + 2 <= end; t += n) {
      int diff = s16(t) - s16(t - n);
      unsigned ad = (unsigned)abs(diff), item = (unsigned)abs(s16(t));
      unsigned lim = item > 4095 ? item / 6 : item / 3;
      if (diff == 0) uc++;
      else if (dir * diff > 0) {
        if (ad < lim) len++, omit = 0;
        else uc++;
        omit++;
      } else {
        if (len >= 4 || first) {
          bad = 0, last = t - n * omit, useless = uc, first = false;
        } else if (++bad >= 2) {
          break;
        }
        dir = s16(t + n) - s16(t) < 0 ? -1 : 1;
        len = omit = 0;
        if (dir * diff > 0) t -= n;
      }
    }
    return last;
  }

  static bool quick(int n, const uint8_t* p) {
    return (unsigned)(p[1] - p[n + 1] + kStep) <= 2 * kStep && (unsigned)(p[n + 1] - p[2 * n + 1] + kStep) <= 2 * kStep &&
           (unsigned)(p[2 * n + 1] - p[3 * n + 1] + kStep) <= 2 * kStep &&
           s16(p) + s16(p + n) != s16(p + 2 * n) + s16(p + 3 * n);
  }

  // a table with records of n bytes around p, worth its descriptor?
  bool table(int n, uint8_t* p, const uint8_t* start, Table& tab) {
    int useless;
    uint8_t* ts = bound(-n, p, start, useless);
    uint8_t* te = bound(n, ts, start, useless);
    int64_t rows = (te - ts) / n;
    int64_t useful = rows - useless;
    int64_t skip = ts - start > 1 ? ts - start : 1;
    if (rows < 2 || useful * kSqrt[n] <= 30 * 256 + 4 * log2fp((uint64_t)skip)) return false;
    bool keep[kMaxN] = {};
    for (int k = 0; k < n; k++) {
      int64_t neq = 0;
      for (int64_t i = 1; i < rows; i++) neq += ts[i * n + k] != ts[(i - 1) * n + k];
      keep[k] = neq * 4 < rows && n != 2 && n != 4 && n != 8;
    }
    diffRows(ts, n, (uint32_t)rows, keep);
    gather(ts, n, (uint32_t)rows, keep, true);
    tab.type = 1u << n;
    for (int k = 0; k < n; k++) tab.type |= (uint32_t)keep[k] << k;
    tab.off = (uint64_t)(ts - lo);
    tab.rows = (uint32_t)rows;
    return true;
  }
};

// Finds and transforms the tables in b, appends them to T.
static void encode(std::vector<uint8_t>& b, std::vector<Table>& T) {
  size_t size = b.size();
  if (size < 2 * kLine + kMaxN || size > 0x7FFFFFFF) return;
  std::vector<uint8_t> buf(kPad + size + kPad, 0);
  memcpy(&buf[kPad], b.data(), size);
  Finder F;
  F.lo = &buf[kPad];
  F.end = F.lo + size;
  uint8_t* last = F.lo;  // end of the last table
  uint8_t* seen[16];     // last position of each high nibble
  uint8_t count[256] = {}, prev[256] = {};
  for (auto& s : seen) s = F.lo - 1;
  unsigned cycle = 0;
  for (uint8_t* ptr = F.lo + kMaxN; ptr + kLine < F.end; ptr += kLine) {
    if (u32(ptr) == u32(ptr + 3)) continue;  // runs of one value
    // distances between bytes with equal high nibbles; sizes up to kSmallN
    // are checked every line, the others every kCycles lines
    unsigned ns = cycle == 0 ? kMaxN : kSmallN;
    for (unsigned i = 1; i <= ns; i++) prev[i] = count[i], count[i] = 0;
    for (uint8_t* p = ptr; p < ptr + kLine; p++) {
      count[(p - seen[*p >> 4]) & 255]++;
      seen[*p >> 4] = p;
    }
    cycle = (cycle + 1) % kCycles;
    ns = cycle == 0 ? kMaxN : kSmallN;
    for (unsigned i = 0; i < ns; i++) {
      if (count[i + 1] + prev[i + 1] < (i >= kSmallN ? 24 : 12)) continue;
      int n = (int)i + 1;
      uint8_t* p = ptr - (i >= kSmallN ? (kCycles - 1) * kLine : 0);
      bool found = false;
      for (int j = 0; j < n && !found; j++, p++) {
        Table t;
        if (Finder::quick(n, p) && F.table(n, p, last, t)) {
          T.push_back(t);
          last = F.lo + t.off + (size_t)n * t.rows;
          ptr = last - kLine;
          cycle = 0;
          found = true;
        }
      }
      if (found) break;
    }
  }
  memcpy(b.data(), F.lo, size);
}

// Undoes the tables T (as found by encode) in b[0..size); false if they don't fit.
static bool decode(uint8_t* b, uint64_t size, const std::vector<Table>& T) {
  uint64_t pos = 0;
  for (const Table& t : T) {
    int n = typeN(t.type);
    if (n < 1 || n > kMaxN || t.off < pos || t.off > size || (uint64_t)n * t.rows > size - t.off) return false;
    bool keep[kMaxN];
    for (int k = 0; k < n; k++) keep[k] = t.type >> k & 1;
    if (t.rows) {
      gather(b + t.off, n, t.rows, keep, false);
      undiffRows(b + t.off, n, t.rows, keep);
    }
    pos = t.off + (uint64_t)n * t.rows;
  }
  return true;
}

}  // namespace tdelta
