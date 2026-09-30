// dicrank2.cpp - version 2 of the dicrank side file: english.dic stored as
// predictions from the word statistics of enwik8 and enwik9, computed with
// integer arithmetic only, so every machine and compiler gets the same result.
//
// Like dicrank (v1), a compressor that has enwik9 can rebuild english.dic from
// the text plus this side file instead of storing the dictionary. v2 predicts
// the dictionary's order from how words are used: each word gets a vector of
// its left and right neighbours among the K most frequent words (weighted by
// positive pointwise mutual information, PPMI). The side file then holds
// mostly small numbers that cmix compresses well:
//
//  - The first S words (default 3920, the 1- and 2-byte codeword tiers of
//    cmix's word transform) are in a hand-made order by meaning. Stored: which
//    of the top ranks they are (bitmap), the first word, and for each next
//    word its position among the remaining ones sorted by similarity to the
//    previous two words.
//  - The other words form maximal alphabetical runs of one kind of word each.
//    Stored: a membership bitmap over the other candidates, with the bits
//    ordered by word length, document frequency, count and share of lowercase
//    uses; then, for each member from most to least frequent, the position of
//    its run among the runs started so far, sorted by similarity between the
//    word and each run's centroid (or "n<k>" for the k-th run not started yet).
//
// Word counts, document frequencies and case come from the first 10^8 bytes of
// the text (= enwik8); context statistics come from all of it (enwik9).
// Weights use an exact fixed-point log2, vectors are normalized with an exact
// integer square root, and ties are broken by index, so encoder and decoder
// always agree. The encoder decodes its own output and fails unless it
// reproduces the dictionary byte for byte.
//
// build:  g++ -O2 -std=c++17 -o dicrank2 dicrank2.cpp
// encode: dicrank2 e [-v] [-n BYTES] [-c BYTES] [-k K] [-s WORDS] TEXT DICTIONARY SIDEFILE
// decode: dicrank2 d [-v] TEXT SIDEFILE DICTIONARY
//   -n BYTES  text prefix for word counts (default 100000000)
//   -c BYTES  text prefix for context statistics (default: all of TEXT)
//   -k K      context words (default 4000)
//   -s WORDS  words kept in explicit order (default 3920, at most 10000: their
//             similarity matrix takes 4 x WORDS^2 bytes)
//   -v        print the time each stage takes

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

bool g_verbose = false;
const size_t kMaxExplicit = 10000;  // explicit words; their similarity matrix is 4 x S^2 bytes

[[noreturn]] void Fail(const char* msg, const std::string& arg = "") {
  fprintf(stderr, "dicrank2: %s%s\n", msg, arg.c_str());
  exit(1);
}

double Seconds() {
  using namespace std::chrono;
  return duration<double>(steady_clock::now().time_since_epoch()).count();
}

// With -v, prints how long each stage took.
double g_stage_start = Seconds();
void Stage(const char* name) {
  double now = Seconds();
  if (g_verbose) fprintf(stderr, "  %-26s %6.1fs\n", name, now - g_stage_start);
  g_stage_start = now;
}

std::string ReadFile(const char* path, uint64_t limit = UINT64_MAX) {
  FILE* f = fopen(path, "rb");
  if (!f) Fail("cannot open ", path);
  std::string data;
  std::vector<char> buf(1 << 20);
  while (data.size() < limit) {
    size_t want = (size_t)std::min<uint64_t>(buf.size(), limit - data.size());
    size_t got = fread(buf.data(), 1, want, f);
    if (got == 0) break;
    data.append(buf.data(), got);
  }
  fclose(f);
  return data;
}

uint64_t FileSize(const char* path) {
  FILE* f = fopen(path, "rb");
  if (!f) Fail("cannot open ", path);
  fseek(f, 0, SEEK_END);
  uint64_t n = (uint64_t)ftell(f);
  fclose(f);
  return n;
}

void WriteFile(const char* path, const std::string& data) {
  FILE* f = fopen(path, "wb");
  if (!f || fwrite(data.data(), 1, data.size(), f) != data.size()) Fail("cannot write ", path);
  fclose(f);
}

// ------------------------------------------------------------------ integers

// floor(log2(x) * 65536) for x >= 1, by repeated squaring of the mantissa.
int64_t Log2Q16(uint64_t x) {
  int e = 63 - __builtin_clzll(x);
  uint64_t m = e >= 62 ? x >> (e - 62) : x << (62 - e);  // [1, 2) as Q62
  int64_t r = (int64_t)e << 16;
  for (int bit = 15; bit >= 0; --bit) {
    m = (uint64_t)(((unsigned __int128)m * m) >> 62);  // [1, 4)
    if (m >> 63) {
      m >>= 1;
      r |= (int64_t)1 << bit;
    }
  }
  return r;
}

// floor(sqrt(x)).
uint64_t ISqrt(uint64_t x) {
  uint64_t r = 0, bit = (uint64_t)1 << 62;
  while (bit > x) bit >>= 2;
  while (bit) {
    if (x >= r + bit) {
      x -= r + bit;
      r = (r >> 1) + bit;
    } else {
      r >>= 1;
    }
    bit >>= 2;
  }
  return r;
}

int FloorLog2(uint64_t x) { return 63 - __builtin_clzll(x); }  // x >= 1

// --------------------------------------------------------------------- words

// Splits text into words like cmix's word transform: a lowercase letter after
// two or more capitals starts a new word ("JScript" -> "js" "cript"), a
// capital after lowercase letters starts a new word ("McDonalds" -> "mc"
// "donalds"), anything but A-Z/a-z ends a word. Words are lowercased.
struct Tokenizer {
  std::string word;
  bool lower_start = false;
  int upper = 0, lower = 0;

  template <class Emit>
  void Push(unsigned char c, Emit&& emit) {
    bool is_lower = c >= 'a' && c <= 'z', is_upper = c >= 'A' && c <= 'Z';
    if ((is_lower && upper > 1) || (is_upper && lower > 0) || (!is_lower && !is_upper)) {
      Flush(emit);
    }
    if (is_lower) {
      if (word.empty()) lower_start = true;
      ++lower;
      word += (char)c;
    } else if (is_upper) {
      if (word.empty()) lower_start = false;
      ++upper;
      word += (char)(c - 'A' + 'a');
    }
  }

  template <class Emit>
  void Flush(Emit&& emit) {
    if (!word.empty()) emit(word, lower_start);
    word.clear();
    upper = lower = 0;
  }
};

struct WordInfo {
  uint32_t off, len;       // spelling at arena[off, off + len)
  uint32_t count = 0;      // occurrences in the counted prefix
  uint32_t df = 0;         // <page>s it occurs in (text before the first <page> counts as one)
  uint32_t low = 0;        // occurrences written in lowercase
  uint32_t last_page = 0;
};

// Open-addressing hash table of words.
class WordTable {
 public:
  WordTable() : slots_(1 << 20, 0) {}

  size_t size() const { return words_.size(); }
  WordInfo& operator[](size_t i) { return words_[i]; }
  const WordInfo& operator[](size_t i) const { return words_[i]; }
  const char* Data(size_t i) const { return arena_.data() + words_[i].off; }
  std::string Spelling(size_t i) const { return std::string(Data(i), words_[i].len); }

  int64_t Find(const char* s, size_t len) const {
    for (size_t i = Hash(s, len) & (slots_.size() - 1);; i = (i + 1) & (slots_.size() - 1)) {
      uint32_t k = slots_[i];
      if (!k) return -1;
      const WordInfo& w = words_[k - 1];
      if (w.len == len && !memcmp(arena_.data() + w.off, s, len)) return k - 1;
    }
  }

  size_t Add(const char* s, size_t len) {
    int64_t found = Find(s, len);
    if (found >= 0) return (size_t)found;
    if (2 * (words_.size() + 1) > slots_.size()) Grow();
    WordInfo w;
    w.off = (uint32_t)arena_.size();
    w.len = (uint32_t)len;
    arena_.append(s, len);
    words_.push_back(w);
    Insert(words_.size() - 1);
    return words_.size() - 1;
  }

  // Alphabetical (byte) order of two words.
  bool Less(size_t a, size_t b) const {
    const WordInfo &x = words_[a], &y = words_[b];
    int c = memcmp(Data(a), Data(b), std::min(x.len, y.len));
    return c ? c < 0 : x.len < y.len;
  }

 private:
  static size_t Hash(const char* s, size_t len) {
    uint64_t h = 1469598103934665603ULL;
    for (size_t i = 0; i < len; ++i) h = (h ^ (unsigned char)s[i]) * 1099511628211ULL;
    return (size_t)(h ^ (h >> 29));
  }
  void Insert(size_t k) {
    size_t i = Hash(Data(k), words_[k].len) & (slots_.size() - 1);
    while (slots_[i]) i = (i + 1) & (slots_.size() - 1);
    slots_[i] = (uint32_t)k + 1;
  }
  void Grow() {
    slots_.assign(slots_.size() * 2, 0);
    for (size_t k = 0; k < words_.size(); ++k) Insert(k);
  }

  std::vector<uint32_t> slots_;
  std::vector<WordInfo> words_;
  std::string arena_;
};

// Candidate list: the extra words (dictionary words the text lacks) in the
// given order, then the words of the first scan8 bytes by descending count,
// ties alphabetical. Also counts document frequency and lowercase use.
struct Vocabulary {
  WordTable table;
  std::vector<uint32_t> cand;  // word index of each rank
  std::vector<int64_t> rank;   // rank of each word index, -1 if not a candidate
  size_t extras = 0;

  void Build(const char* text_path, uint64_t scan8, const std::vector<std::string>& extra) {
    std::string text = ReadFile(text_path, scan8);
    if (text.size() != scan8) Fail("text is shorter than the counted prefix");
    Tokenizer tok;
    uint32_t page = 0;
    auto emit = [&](const std::string& w, bool lower_start) {
      WordInfo& e = table[table.Add(w.data(), w.size())];
      ++e.count;
      if (lower_start) ++e.low;
      if (e.df == 0 || e.last_page != page) {
        ++e.df;
        e.last_page = page;
      }
    };
    for (size_t i = 0; i < text.size(); ++i) {
      unsigned char c = (unsigned char)text[i];
      tok.Push(c, emit);  // ends the previous word before a new page starts
      if (c == '<' && text.compare(i, 6, "<page>") == 0) ++page;
    }
    tok.Flush(emit);
    size_t counted = table.size();
    for (auto& w : extra) {
      size_t k = table.Add(w.data(), w.size());
      if (table[k].count) Fail("extra word occurs in the text: ", w);
      cand.push_back((uint32_t)k);
    }
    extras = extra.size();
    std::vector<uint32_t> words(counted);
    for (size_t k = 0; k < counted; ++k) words[k] = (uint32_t)k;
    std::sort(words.begin(), words.end(), [&](uint32_t a, uint32_t b) {
      if (table[a].count != table[b].count) return table[a].count > table[b].count;
      return table.Less(a, b);
    });
    cand.insert(cand.end(), words.begin(), words.end());
    rank.assign(table.size(), -1);
    for (size_t r = 0; r < cand.size(); ++r) rank[cand[r]] = (int64_t)r;
  }

  std::string Word(size_t r) const { return table.Spelling(cand[r]); }
  const WordInfo& Info(size_t r) const { return table[cand[r]]; }
  bool WordLess(size_t ra, size_t rb) const { return table.Less(cand[ra], cand[rb]); }
};

// Order in which the membership bits of the non-explicit candidates are
// listed: word length, document frequency, count, share of lowercase use.
struct BitmapKey {
  uint32_t length, df, count, lowercase, rank;
  bool operator<(const BitmapKey& o) const {
    if (length != o.length) return length < o.length;
    if (df != o.df) return df < o.df;
    if (count != o.count) return count < o.count;
    if (lowercase != o.lowercase) return lowercase < o.lowercase;
    return rank < o.rank;
  }
};

BitmapKey KeyOf(const Vocabulary& v, uint32_t r) {
  const WordInfo& w = v.Info(r);
  BitmapKey k;
  k.length = std::min<uint32_t>(w.len, 12) / 3;
  k.df = FloorLog2((uint64_t)w.df + 1);
  k.count = FloorLog2((uint64_t)w.count + 1);
  k.lowercase = w.count ? std::min<uint32_t>(4, (uint32_t)(5ULL * w.low / w.count)) : 0;
  k.rank = r;
  return k;
}

// ------------------------------------------------------------------- vectors

// Integer context vectors: entry f is PPMI (fixed-point log2, Q16) of the
// target word with feature f, where features 0..K-1 are "left neighbour is
// the (f)-th most frequent word" and K..2K-1 the same for the right
// neighbour, normalized to length ~2^15.
struct Vectors {
  std::vector<uint64_t> start;  // CSR row starts
  std::vector<uint16_t> feature;
  std::vector<int32_t> value;
  std::vector<int64_t> norm2;   // exact squared length of each row
  size_t Rows() const { return norm2.size(); }
};

// Counts neighbour features over the first scan9 bytes of the text for the
// targets (candidate ranks); groups[g] = [begin, end) row ranges that get
// their own PPMI statistics.
std::vector<Vectors> ContextVectors(const Vocabulary& v, const char* text_path, uint64_t scan9,
                                    uint32_t K, const std::vector<uint32_t>& targets,
                                    const std::vector<std::pair<size_t, size_t>>& groups) {
  const size_t F = 2 * (size_t)K;
  std::vector<int32_t> feature_of(v.cand.size(), -1), target_of(v.cand.size(), -1);
  for (size_t r = v.extras; r < v.cand.size() && r - v.extras < K; ++r) feature_of[r] = (int32_t)(r - v.extras);
  for (size_t t = 0; t < targets.size(); ++t) target_of[targets[t]] = (int32_t)t;
  std::vector<uint32_t> counts(targets.size() * F, 0);

  FILE* f = fopen(text_path, "rb");
  if (!f) Fail("cannot open ", text_path);
  Tokenizer tok;
  int64_t prev2 = -1, prev1 = -1;  // candidate ranks of the last two words, -1 if none
  uint64_t tokens = 0;
  auto emit = [&](const std::string& w, bool) {
    int64_t k = v.table.Find(w.data(), w.size());
    int64_t cur = k >= 0 ? v.rank[k] : -1;
    // prev1 is the middle word, with prev2 on its left and cur on its right
    // (the first word of the text is never a middle word).
    if (tokens >= 2 && prev1 >= 0 && target_of[prev1] >= 0) {
      uint32_t* row = &counts[(size_t)target_of[prev1] * F];
      if (prev2 >= 0 && feature_of[prev2] >= 0) ++row[feature_of[prev2]];
      if (cur >= 0 && feature_of[cur] >= 0) ++row[K + feature_of[cur]];
    }
    prev2 = prev1;
    prev1 = cur;
    ++tokens;
  };
  std::vector<char> buf(16 << 20);
  uint64_t left = scan9;
  while (left) {
    size_t got = fread(buf.data(), 1, (size_t)std::min<uint64_t>(buf.size(), left), f);
    if (!got) Fail("text is shorter than the context prefix");
    for (size_t i = 0; i < got; ++i) tok.Push((unsigned char)buf[i], emit);
    left -= got;
  }
  fclose(f);
  tok.Flush(emit);

  std::vector<Vectors> out;
  for (auto& g : groups) {
    uint64_t total = 0;
    std::vector<uint64_t> col(F, 0), row(g.second - g.first, 0);
    for (size_t t = g.first; t < g.second; ++t) {
      const uint32_t* c = &counts[t * F];
      for (size_t j = 0; j < F; ++j) {
        row[t - g.first] += c[j];
        col[j] += c[j];
      }
      total += row[t - g.first];
    }
    Vectors vec;
    vec.start.push_back(0);
    std::vector<std::pair<uint16_t, int64_t>> pmi;
    for (size_t t = g.first; t < g.second; ++t) {
      const uint32_t* c = &counts[t * F];
      pmi.clear();
      uint64_t sum2 = 0;
      for (size_t j = 0; j < F; ++j) {
        if (!c[j]) continue;
        int64_t p = Log2Q16((uint64_t)c[j] * total) - Log2Q16(row[t - g.first] * col[j]);
        if (p <= 0) continue;
        pmi.emplace_back((uint16_t)j, p);
        sum2 += (uint64_t)(p * p);
      }
      // Rounding the norm up keeps |u|^2 <= 2^30, so dot products of two
      // vectors fit in int32 (see Gram) and centroid sums stay small.
      uint64_t norm = ISqrt(sum2);
      if (norm * norm < sum2) ++norm;
      int64_t n2 = 0;
      for (auto& e : pmi) {
        int32_t u = (int32_t)((e.second << 15) / (int64_t)norm);
        if (!u) continue;
        vec.feature.push_back(e.first);
        vec.value.push_back(u);
        n2 += (int64_t)u * u;
      }
      vec.norm2.push_back(n2);
      vec.start.push_back(vec.feature.size());
    }
    out.push_back(std::move(vec));
  }
  return out;
}

// ------------------------------------------------------------ explicit words

// Gram matrix of the explicit words' vectors (cosine x ~2^30; fits int32).
std::vector<int32_t> Gram(const Vectors& vec, uint32_t F) {
  size_t n = vec.Rows();
  std::vector<int32_t> dense((size_t)F * n, 0), gram(n * n, 0);  // dense is feature-major
  for (size_t a = 0; a < n; ++a) {
    for (uint64_t i = vec.start[a]; i < vec.start[a + 1]; ++i) dense[(size_t)vec.feature[i] * n + a] = vec.value[i];
  }
  for (size_t a = 0; a < n; ++a) {  // upper triangle, then mirror
    int32_t* g = &gram[a * n];
    for (uint64_t i = vec.start[a]; i < vec.start[a + 1]; ++i) {
      const int32_t* d = &dense[(size_t)vec.feature[i] * n];
      int32_t x = vec.value[i];
      for (size_t c = a; c < n; ++c) g[c] += x * d[c];
    }
    for (size_t c = a + 1; c < n; ++c) gram[c * n + a] = g[c];
  }
  return gram;
}

// Score of candidate c after the words p1 (previous) and p2 (the one before,
// or -1): 2 cos(p1, c) + cos(p2, c).
inline int64_t HeadScore(const std::vector<int32_t>& gram, size_t n, int64_t p1, int64_t p2, size_t c) {
  int64_t s = 2 * (int64_t)gram[(size_t)p1 * n + c];
  if (p2 >= 0) s += gram[(size_t)p2 * n + c];
  return s;
}

// --------------------------------------------------------------------- runs

// Incremental run centroids; scores runs for a word by
// floor(dot(word, centroid) * 2^20 / |centroid|), all integer.
class Runs {
 public:
  Runs(size_t runs, uint32_t F) : n_(runs), centroid_((size_t)F * runs, 0), norm2_(runs, 0),
                                  started_(runs, false), acc_(runs, 0) {}

  // Fills scores for all runs (0 for runs not started) from the word's vector.
  const std::vector<int64_t>& Score(const Vectors& vec, size_t row) {
    std::fill(acc_.begin(), acc_.end(), 0);
    for (uint64_t i = vec.start[row]; i < vec.start[row + 1]; ++i) {
      const int64_t* c = &centroid_[(size_t)vec.feature[i] * n_];
      int64_t x = vec.value[i];
      for (size_t r = 0; r < n_; ++r) acc_[r] += x * c[r];
    }
    dots_ = acc_;
    for (size_t r = 0; r < n_; ++r) {
      uint64_t len = started_[r] ? ISqrt((uint64_t)norm2_[r]) : 0;
      acc_[r] = len ? (int64_t)(((__int128)dots_[r] << 20) / len) : 0;
    }
    return acc_;
  }

  // Adds the word (whose Score() was just computed) to run r.
  void Add(const Vectors& vec, size_t row, size_t r) {
    for (uint64_t i = vec.start[row]; i < vec.start[row + 1]; ++i) {
      centroid_[(size_t)vec.feature[i] * n_ + r] += vec.value[i];
    }
    norm2_[r] += 2 * dots_[r] + vec.norm2[row];
    started_[r] = true;
  }

  bool Started(size_t r) const { return started_[r]; }

 private:
  size_t n_;
  std::vector<int64_t> centroid_, norm2_;  // centroid is feature-major
  std::vector<bool> started_;
  std::vector<int64_t> acc_, dots_;
};

// Position of `target` among the items, ordered by descending score, ties by
// ascending index.
size_t PositionOf(const std::vector<size_t>& items, const std::vector<int64_t>& score, size_t target) {
  size_t pos = 0;
  for (size_t i : items) {
    if (score[i] > score[target] || (score[i] == score[target] && i < target)) ++pos;
  }
  return pos;
}

// Item at position k in that order.
size_t ItemAt(std::vector<size_t> items, const std::vector<int64_t>& score, size_t k) {
  if (k >= items.size()) Fail("position out of range in side file");
  std::nth_element(items.begin(), items.begin() + k, items.end(), [&](size_t a, size_t b) {
    return score[a] != score[b] ? score[a] > score[b] : a < b;
  });
  return items[k];
}

// ---------------------------------------------------------------- side file

// Layout (text):
//   dicrank 2
//   <counted bytes> <context bytes> <K> <explicit words S> <runs> <extra words>
//   <extra words, one per line>
//   <bitmap over ranks 0..: which are explicit words>
//   <position of the first explicit word among them (by rank)>
//   <S-1 lines: position of the next word by similarity to the previous two>
//   <number of other candidates covered by the next bitmap>
//   <bitmap over those candidates, in BitmapKey order: dictionary member?>
//   <one line per member, most frequent first: run position, or n<k> for a new run>
struct Side {
  uint64_t scan8 = 100000000, scan9 = 0;
  uint32_t K = 4000;
  size_t S = 0, runs = 0;
  std::vector<std::string> extra;
  std::string head_bitmap;
  size_t first = 0;
  std::vector<uint32_t> head_pos;
  size_t n_cand = 0;
  std::string tail_bitmap;
  std::vector<std::string> run_syms;
};

std::string Format(const Side& s) {
  std::string o = "dicrank 2\n";
  char line[160];
  snprintf(line, sizeof(line), "%llu %llu %u %zu %zu %zu\n", (unsigned long long)s.scan8,
           (unsigned long long)s.scan9, s.K, s.S, s.runs, s.extra.size());
  o += line;
  for (auto& w : s.extra) o += w + '\n';
  o += s.head_bitmap + '\n';
  if (s.S) {
    o += std::to_string(s.first) + '\n';
    for (uint32_t p : s.head_pos) o += std::to_string(p) + '\n';
  }
  o += std::to_string(s.n_cand) + '\n' + s.tail_bitmap + '\n';
  for (auto& r : s.run_syms) o += r + '\n';
  return o;
}

Side Parse(const std::string& data) {
  Side s;
  size_t pos = 0;
  auto line = [&]() {
    size_t end = data.find('\n', pos);
    if (end == std::string::npos) Fail("side file is truncated");
    std::string l = data.substr(pos, end - pos);
    pos = end + 1;
    return l;
  };
  auto number = [&](const std::string& l) {
    char* end;
    unsigned long long v = strtoull(l.c_str(), &end, 10);
    if (l.empty() || *end) Fail("bad number in side file: ", l);
    return (uint64_t)v;
  };
  if (line() != "dicrank 2") Fail("not a dicrank 2 side file");
  unsigned long long scan8, scan9;
  size_t extra;
  if (sscanf(line().c_str(), "%llu %llu %u %zu %zu %zu", &scan8, &scan9, &s.K, &s.S, &s.runs, &extra) != 6) {
    Fail("bad side file header");
  }
  s.scan8 = scan8;
  s.scan9 = scan9;
  for (size_t i = 0; i < extra; ++i) s.extra.push_back(line());
  s.head_bitmap = line();
  if ((size_t)std::count(s.head_bitmap.begin(), s.head_bitmap.end(), '1') != s.S) Fail("bad explicit-word bitmap");
  if (s.S > kMaxExplicit) Fail("too many explicit words in side file");
  if (s.S) {
    s.first = number(line());
    for (size_t i = 1; i < s.S; ++i) s.head_pos.push_back((uint32_t)number(line()));
  }
  s.n_cand = number(line());
  s.tail_bitmap = line();
  if (s.tail_bitmap.size() != s.n_cand) Fail("bad membership bitmap length");
  size_t members = std::count(s.tail_bitmap.begin(), s.tail_bitmap.end(), '1');
  for (size_t i = 0; i < members; ++i) s.run_syms.push_back(line());
  if (pos != data.size()) Fail("trailing data in side file");
  return s;
}

// ------------------------------------------------------------- the two ways

// Everything both directions compute from the text and the two word sets.
struct Model {
  Vocabulary voc;
  std::vector<uint32_t> head;  // explicit words' ranks, ascending
  std::vector<uint32_t> tail;  // other members' ranks, ascending (= most frequent first)
  Vectors head_vec, tail_vec;
  std::vector<int32_t> gram;

  void Vectorize(const char* text_path, const Side& s) {
    std::vector<uint32_t> targets(head);
    targets.insert(targets.end(), tail.begin(), tail.end());
    auto v = ContextVectors(voc, text_path, s.scan9, s.K, targets,
                            {{0, head.size()}, {head.size(), targets.size()}});
    head_vec = std::move(v[0]);
    tail_vec = std::move(v[1]);
    Stage("context vectors");
    gram = Gram(head_vec, 2 * s.K);
    Stage("explicit word similarity");
  }
};

// The candidates a membership bitmap covers, in bitmap order.
std::vector<uint32_t> BitmapOrder(const Vocabulary& voc, const std::vector<char>& is_head, size_t n_cand) {
  std::vector<uint32_t> pool;
  for (uint32_t r = 0; r < voc.cand.size() && pool.size() < n_cand; ++r) {
    if (!is_head[r]) pool.push_back(r);
  }
  if (pool.size() != n_cand) Fail("membership bitmap longer than the word list");
  std::vector<BitmapKey> keys;
  for (uint32_t r : pool) keys.push_back(KeyOf(voc, r));
  std::sort(keys.begin(), keys.end());
  for (size_t i = 0; i < keys.size(); ++i) pool[i] = keys[i].rank;
  return pool;
}

std::string Decode(const char* text_path, const Side& s) {
  Model m;
  m.voc.Build(text_path, s.scan8, s.extra);
  Stage("word counts");
  const Vocabulary& voc = m.voc;
  std::vector<char> is_head(voc.cand.size(), 0);
  if (s.head_bitmap.size() > voc.cand.size()) Fail("explicit-word bitmap longer than the word list");
  for (uint32_t r = 0; r < s.head_bitmap.size(); ++r) {
    if (s.head_bitmap[r] == '1') {
      is_head[r] = 1;
      m.head.push_back(r);
    }
  }
  std::vector<uint32_t> order = BitmapOrder(voc, is_head, s.n_cand);
  for (size_t i = 0; i < order.size(); ++i) {
    if (s.tail_bitmap[i] == '1') m.tail.push_back(order[i]);
  }
  std::sort(m.tail.begin(), m.tail.end());
  m.Vectorize(text_path, s);

  std::vector<std::string> out;
  if (s.S) {
    size_t n = m.head.size();
    std::vector<size_t> seq{s.first};
    if (s.first >= n) Fail("bad first explicit word");
    std::vector<char> used(n, 0);
    used[s.first] = 1;
    std::vector<int64_t> score(n);
    for (size_t i = 1; i < n; ++i) {
      std::vector<size_t> remaining;
      int64_t p2 = i >= 2 ? (int64_t)seq[i - 2] : -1;
      for (size_t c = 0; c < n; ++c) {
        if (used[c]) continue;
        remaining.push_back(c);
        score[c] = HeadScore(m.gram, n, (int64_t)seq[i - 1], p2, c);
      }
      size_t c = ItemAt(remaining, score, s.head_pos[i - 1]);
      seq.push_back(c);
      used[c] = 1;
    }
    for (size_t c : seq) out.push_back(voc.Word(m.head[c]));
  }
  Stage("explicit word order");

  Runs runs(s.runs, 2 * s.K);
  std::vector<std::vector<uint32_t>> members(s.runs);
  std::vector<size_t> unstarted(s.runs);
  for (size_t r = 0; r < s.runs; ++r) unstarted[r] = r;
  for (size_t j = 0; j < m.tail.size(); ++j) {
    const std::string& sym = s.run_syms[j];
    const std::vector<int64_t>& score = runs.Score(m.tail_vec, j);
    size_t run;
    if (!sym.empty() && sym[0] == 'n') {
      size_t k = strtoull(sym.c_str() + 1, nullptr, 10);
      if (k >= unstarted.size()) Fail("bad new-run index in side file");
      run = unstarted[k];
      unstarted.erase(unstarted.begin() + k);
    } else {
      std::vector<size_t> started;
      for (size_t r = 0; r < s.runs; ++r) {
        if (runs.Started(r)) started.push_back(r);
      }
      run = ItemAt(started, score, strtoull(sym.c_str(), nullptr, 10));
    }
    runs.Add(m.tail_vec, j, run);
    members[run].push_back(m.tail[j]);
  }
  Stage("run assignment");
  for (auto& run : members) {
    std::sort(run.begin(), run.end(), [&](uint32_t a, uint32_t b) { return voc.WordLess(a, b); });
    for (uint32_t r : run) out.push_back(voc.Word(r));
  }
  std::string text;
  for (auto& w : out) text += w + '\n';
  return text;
}

Side Encode(const char* text_path, const std::vector<std::string>& dict, uint64_t scan8,
            uint64_t scan9, uint32_t K, size_t S) {
  Side s;
  s.scan8 = scan8;
  s.scan9 = scan9;
  s.K = K;
  s.S = std::min(S, dict.size());
  if (s.S > kMaxExplicit) Fail("-s must be at most 10000");
  Model m;
  {  // extra words: dictionary words missing from the counted prefix
    Vocabulary probe;
    probe.Build(text_path, scan8, {});
    for (auto& w : dict) {
      int64_t k = probe.table.Find(w.data(), w.size());
      if (k < 0) s.extra.push_back(w);
    }
  }
  m.voc.Build(text_path, scan8, s.extra);
  Stage("word counts");
  const Vocabulary& voc = m.voc;
  std::vector<uint32_t> rank_of_word;
  for (auto& w : dict) rank_of_word.push_back((uint32_t)voc.rank[voc.table.Find(w.data(), w.size())]);

  // Explicit words: bitmap over ranks, first word, then similarity positions.
  std::vector<char> is_head(voc.cand.size(), 0);
  uint32_t max_head = 0;
  for (size_t i = 0; i < s.S; ++i) {
    is_head[rank_of_word[i]] = 1;
    max_head = std::max(max_head, rank_of_word[i]);
  }
  if (s.S) {
    s.head_bitmap.assign(max_head + 1, '0');
    for (uint32_t r = 0; r <= max_head; ++r) {
      if (is_head[r]) {
        s.head_bitmap[r] = '1';
        m.head.push_back(r);
      }
    }
  }
  // Other words: maximal ascending runs, membership bitmap.
  std::vector<uint32_t> run_of(voc.cand.size(), UINT32_MAX);
  uint32_t run = 0;
  for (size_t i = s.S; i < dict.size(); ++i) {
    if (i > s.S && dict[i] <= dict[i - 1]) ++run;
    run_of[rank_of_word[i]] = run;
    m.tail.push_back(rank_of_word[i]);
  }
  s.runs = dict.size() > s.S ? run + 1 : 0;
  std::sort(m.tail.begin(), m.tail.end());
  size_t n_cand = 0, seen = 0;
  for (uint32_t r = 0; r < voc.cand.size(); ++r) {
    if (is_head[r]) continue;
    ++seen;
    if (run_of[r] != UINT32_MAX) n_cand = seen;
  }
  s.n_cand = n_cand;
  for (uint32_t r : BitmapOrder(voc, is_head, n_cand)) s.tail_bitmap += run_of[r] != UINT32_MAX ? '1' : '0';

  m.Vectorize(text_path, s);
  if (s.S) {
    size_t n = m.head.size();
    std::vector<size_t> seq;  // positions in m.head, dictionary order
    for (size_t i = 0; i < s.S; ++i) {
      seq.push_back(std::lower_bound(m.head.begin(), m.head.end(), rank_of_word[i]) - m.head.begin());
    }
    s.first = seq[0];
    std::vector<char> used(n, 0);
    used[seq[0]] = 1;
    std::vector<int64_t> score(n);
    for (size_t i = 1; i < n; ++i) {
      std::vector<size_t> remaining;
      int64_t p2 = i >= 2 ? (int64_t)seq[i - 2] : -1;
      for (size_t c = 0; c < n; ++c) {
        if (used[c]) continue;
        remaining.push_back(c);
        score[c] = HeadScore(m.gram, n, (int64_t)seq[i - 1], p2, c);
      }
      s.head_pos.push_back((uint32_t)PositionOf(remaining, score, seq[i]));
      used[seq[i]] = 1;
    }
  }
  Stage("explicit word order");
  Runs runs(s.runs, 2 * K);
  std::vector<size_t> unstarted(s.runs);
  for (size_t r = 0; r < s.runs; ++r) unstarted[r] = r;
  for (size_t j = 0; j < m.tail.size(); ++j) {
    uint32_t r = run_of[m.tail[j]];
    const std::vector<int64_t>& score = runs.Score(m.tail_vec, j);
    if (!runs.Started(r)) {
      size_t k = std::find(unstarted.begin(), unstarted.end(), r) - unstarted.begin();
      s.run_syms.push_back("n" + std::to_string(k));
      unstarted.erase(unstarted.begin() + k);
    } else {
      std::vector<size_t> started;
      for (size_t q = 0; q < s.runs; ++q) {
        if (runs.Started(q)) started.push_back(q);
      }
      s.run_syms.push_back(std::to_string(PositionOf(started, score, r)));
    }
    runs.Add(m.tail_vec, j, r);
  }
  Stage("run assignment");
  return s;
}

std::vector<std::string> ParseDictionary(const std::string& data) {
  std::vector<std::string> words;
  std::string w;
  for (char c : data) {
    if (c == '\n') {
      if (w.empty()) Fail("empty line in dictionary");
      words.push_back(w);
      w.clear();
    } else if (c >= 'a' && c <= 'z') {
      w += c;
    } else {
      Fail("dictionary must contain only lowercase a-z words, one per line");
    }
  }
  if (!w.empty()) Fail("dictionary must end with a newline");
  std::vector<std::string> sorted(words);
  std::sort(sorted.begin(), sorted.end());
  if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end()) Fail("dictionary words must be unique");
  return words;
}

int Usage() {
  fprintf(stderr,
          "usage: dicrank2 e [-v] [-n BYTES] [-c BYTES] [-k K] [-s WORDS] TEXT DICTIONARY SIDEFILE\n"
          "       dicrank2 d [-v] TEXT SIDEFILE DICTIONARY\n");
  return 2;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) return Usage();
  std::string mode = argv[1];
  uint64_t scan8 = 100000000, scan9 = 0;
  uint32_t K = 4000;
  size_t S = 3920;
  std::vector<const char*> args;
  for (int i = 2; i < argc; ++i) {
    if (!strcmp(argv[i], "-n") && i + 1 < argc) scan8 = strtoull(argv[++i], nullptr, 10);
    else if (!strcmp(argv[i], "-c") && i + 1 < argc) scan9 = strtoull(argv[++i], nullptr, 10);
    else if (!strcmp(argv[i], "-k") && i + 1 < argc) K = (uint32_t)strtoul(argv[++i], nullptr, 10);
    else if (!strcmp(argv[i], "-s") && i + 1 < argc) S = strtoull(argv[++i], nullptr, 10);
    else if (!strcmp(argv[i], "-v")) g_verbose = true;
    else args.push_back(argv[i]);
  }
  if (args.size() != 3) return Usage();
  if (K == 0 || K > 32768) Fail("K must be 1..32768");
  double t0 = Seconds();
  if (mode == "e") {
    uint64_t size = FileSize(args[0]);
    scan8 = std::min(scan8, size);
    scan9 = scan9 ? std::min(scan9, size) : size;
    std::string dict_data = ReadFile(args[1]);
    std::vector<std::string> dict = ParseDictionary(dict_data);
    Side side = Encode(args[0], dict, scan8, scan9, K, S);
    std::string out = Format(side);
    double t1 = Seconds();
    if (Decode(args[0], Parse(out)) != dict_data) {
      Fail("internal error: decoding the side file does not reproduce the dictionary");
    }
    WriteFile(args[2], out);
    fprintf(stderr, "%zu words: %zu explicit, %zu in %zu runs, %zu not in the text; side file %zu bytes; "
            "encode %.1fs, check %.1fs\n", dict.size(), side.S, side.run_syms.size(), side.runs,
            side.extra.size(), out.size(), t1 - t0, Seconds() - t1);
    return 0;
  }
  if (mode == "d") {
    std::string out = Decode(args[0], Parse(ReadFile(args[1])));
    WriteFile(args[2], out);
    fprintf(stderr, "decode %.1fs\n", Seconds() - t0);
    return 0;
  }
  return Usage();
}
