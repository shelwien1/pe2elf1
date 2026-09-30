// dicrank3.cpp - version 3 of the dicrank side file: dicrank2's predictions,
// coded by a built-in context-mixing coder instead of being written as text
// for cmix.
//
// The transform is dicrank2's, unchanged and integer-only (see dicrank2.cpp):
// word statistics from the first 10^8 bytes of the text, context vectors
// (PPMI of left/right neighbours among the K most frequent words) from all
// of it, the explicit words ordered by similarity to the previous two, and
// the other words' runs predicted from run centroids. dicrank2 writes the
// resulting positions and bitmaps as text and leaves them to cmix; dicrank3
// codes them itself, with contexts cmix cannot see: word statistics,
// candidate scores and run statistics.
// The coder (dr3_model.inc) is built from the components of the tsvcomp
// coder and its IDX parameter framework (IDX/, MOD/); dr3_tune.cpp codes
// dumped streams for tuning with IDX/opt.pl.
//
// The model uses floats, so encoder and decoder must be builds that evaluate
// floats identically: no -ffast-math, no FMA contraction (dr3_model.inc
// checks both), SSE2 floats. The encoder decodes its own output and fails
// unless it reproduces the dictionary byte for byte.
//
// build:  g++ -O2 -std=c++17 -ffp-contract=off -o dicrank3 dicrank3.cpp   (in this folder;
//         ./build.sh regenerates MOD/ from IDX/ first)
// encode: dicrank3 e [-v] [-n BYTES] [-c BYTES] [-k K] [-s WORDS] [-D DIR] TEXT DICTIONARY SIDEFILE
// decode: dicrank3 d [-v] TEXT SIDEFILE DICTIONARY
//   -n BYTES  text prefix for word counts (default 100000000)
//   -c BYTES  text prefix for context statistics (default: all of TEXT)
//   -k K      context words (default 4000)
//   -s WORDS  words kept in explicit order (default 3920, at most 10000)
//   -D DIR    also write the coder's inputs to DIR, for dr3_tune
//   -v        print the time each stage takes and the bytes of each stream
//
// Side file: two text lines, then the extra words, then the coded streams:
//   dicrank 3
//   <counted bytes> <context bytes> <K> <explicit words S> <runs> <extra words> <explicit-word bitmap length> <membership bitmap length>
//   <extra words, one per line>
//   <range coder stream: explicit-word set, membership bitmap, explicit-word order, runs>

#include <chrono>
#include <string>

#include "dr3_model.inc"

namespace {

bool g_verbose = false;
const size_t kMaxExplicit = 10000;  // explicit words; their similarity matrix is 4 x S^2 bytes

[[noreturn]] void Fail(const char* msg, const std::string& arg = "") {
  fprintf(stderr, "dicrank3: %s%s\n", msg, arg.c_str());
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


// ------------------------------------------------------------ the two ways

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

// -------------------------------------------------------------------- coding

WordF Feat(const Vocabulary& v, uint32_t r) {
  const WordInfo& w = v.Info(r);
  return MakeWordF(v.table.Data(v.cand[r]), (int)w.len, w.count, w.df, w.low, r);
}

// Features of the given ranks, and pointers to them, as the coder takes them.
struct Feats {
  std::vector<WordF> f;
  std::vector<const WordF*> p;
  Feats(const Vocabulary& v, const std::vector<uint32_t>& ranks) {
    for (uint32_t r : ranks) f.push_back(Feat(v, r));
    for (const WordF& x : f) p.push_back(&x);
  }
};

// dicrank2's run centroids as the run coder's score provider. With a dump
// string, it also records every word's scores for dr3_tune.
struct CentroidScores {
  Runs runs;
  const Vectors& vec;
  std::string* dump;
  CentroidScores(size_t R, uint32_t F, const Vectors& v, std::string* d) : runs(R, F), vec(v), dump(d) {}
  const int64_t* Score(uint32_t j) {
    const std::vector<int64_t>& s = runs.Score(vec, j);
    if (dump) dump->append((const char*)s.data(), s.size() * sizeof(int64_t));
    return s.data();
  }
  void Add(uint32_t j, uint32_t r) { runs.Add(vec, j, r); }
};

// Everything both directions compute from the text and the two word sets.
struct Model {
  Vocabulary voc;
  std::vector<uint32_t> head;  // explicit words' ranks, ascending
  std::vector<uint32_t> tail;  // other members' ranks, ascending (= most frequent first)
  Vectors head_vec, tail_vec;
  std::vector<int32_t> gram;

  void Vectorize(const char* text_path, uint64_t scan9, uint32_t K) {
    std::vector<uint32_t> targets(head);
    targets.insert(targets.end(), tail.begin(), tail.end());
    auto v = ContextVectors(voc, text_path, scan9, K, targets, {{0, head.size()}, {head.size(), targets.size()}});
    head_vec = std::move(v[0]);
    tail_vec = std::move(v[1]);
    Stage("context vectors");
    gram = Gram(head_vec, 2 * K);
    Stage("explicit word similarity");
  }
};

// ----------------------------------------------------------------- side file

struct Header {
  uint64_t scan8 = 100000000, scan9 = 0;
  uint32_t K = 4000;
  size_t S = 0, runs = 0;
  size_t hset = 0;    // length of the explicit-word bitmap (ranks 0..hset-1)
  size_t n_cand = 0;  // length of the membership bitmap
  std::vector<std::string> extra;
};

void WriteHeader(FILE* f, const Header& h) {
  fprintf(f, "dicrank 3\n%llu %llu %u %zu %zu %zu %zu %zu\n", (unsigned long long)h.scan8,
          (unsigned long long)h.scan9, h.K, h.S, h.runs, h.extra.size(), h.hset, h.n_cand);
  for (auto& w : h.extra) fprintf(f, "%s\n", w.c_str());
}

Header ReadHeader(FILE* f) {
  char buf[1024];
  auto line = [&]() {
    if (!fgets(buf, sizeof(buf), f)) Fail("side file is truncated");
    size_t n = strlen(buf);
    if (!n || buf[n - 1] != '\n') Fail("bad line in side file");
    return std::string(buf, n - 1);
  };
  if (line() != "dicrank 3") Fail("not a dicrank 3 side file");
  Header h;
  unsigned long long scan8, scan9;
  size_t extra;
  std::string l = line();
  if (sscanf(l.c_str(), "%llu %llu %u %zu %zu %zu %zu %zu", &scan8, &scan9, &h.K, &h.S, &h.runs, &extra,
             &h.hset, &h.n_cand) != 8) {
    Fail("bad side file header");
  }
  h.scan8 = scan8;
  h.scan9 = scan9;
  if (h.K == 0 || h.K > 32768) Fail("bad K in side file");
  if (h.S > kMaxExplicit) Fail("too many explicit words in side file");
  if (h.runs > (1u << 24) || extra > (1u << 24)) Fail("bad side file header");
  for (size_t i = 0; i < extra; ++i) {
    h.extra.push_back(line());
    for (char c : h.extra.back()) {
      if (c < 'a' || c > 'z') Fail("bad extra word in side file");
    }
    if (h.extra.back().empty()) Fail("bad extra word in side file");
  }
  return h;
}

void PrintStreams() {
  if (!g_verbose) return;
  static const char* name[4] = {"runs", "explicit-word order", "membership bitmap", "explicit-word set"};
  for (int s : {kHeadSet, kBitmap, kHead, kRuns}) {
    fprintf(stderr, "  %-26s %9.1f bytes, %llu decisions\n", name[s], g_m->bits[s] / 8, g_m->decisions[s]);
  }
}

// The coder's inputs, for dr3_tune (see its header for the format).
void WriteDumps(const std::string& dir, const Vocabulary& voc, const std::vector<uint32_t>& hset_ranks,
                const std::vector<uint8_t>& hset_bits, const std::vector<uint32_t>& border,
                const std::vector<uint8_t>& bbits, const Model& m, const std::vector<uint32_t>& seq,
                const std::vector<uint32_t>& runs_j, size_t R, const std::string& scores) {
  auto put = [](std::string& o, const void* p, size_t n) { o.append((const char*)p, n); };
  auto put32 = [&](std::string& o, uint32_t x) { put(o, &x, 4); };
  uint32_t top = (uint32_t)hset_ranks.size();
  for (uint32_t r : border) top = std::max(top, r + 1);
  std::string w;
  for (uint32_t r = 0; r < top; ++r) {
    const WordInfo& i = voc.Info(r);
    w += voc.Word(r) + " " + std::to_string(i.count) + " " + std::to_string(i.df) + " " + std::to_string(i.low) + "\n";
  }
  WriteFile((dir + "/words.txt").c_str(), w);
  std::string b;
  put32(b, (uint32_t)hset_bits.size());
  put(b, hset_bits.data(), hset_bits.size());
  put32(b, (uint32_t)border.size());
  put(b, border.data(), border.size() * 4);
  put(b, bbits.data(), bbits.size());
  WriteFile((dir + "/bitmap.bin").c_str(), b);
  std::string h;
  put32(h, (uint32_t)m.head.size());
  put(h, m.head.data(), m.head.size() * 4);
  put(h, seq.data(), seq.size() * 4);
  put(h, m.gram.data(), m.gram.size() * 4);
  WriteFile((dir + "/head.bin").c_str(), h);
  std::string r;
  put32(r, (uint32_t)m.tail.size());
  put32(r, (uint32_t)R);
  put(r, m.tail.data(), m.tail.size() * 4);
  put(r, runs_j.data(), runs_j.size() * 4);
  r += scores;
  WriteFile((dir + "/runs.bin").c_str(), r);
}

// Writes the side file for the dictionary to f.
void Encode(const char* text_path, const std::vector<std::string>& dict, Header& h, FILE* f,
            const char* dump_dir) {
  h.S = std::min(h.S, dict.size());
  if (h.S > kMaxExplicit) Fail("-s must be at most 10000");
  Model m;
  {  // extra words: dictionary words missing from the counted prefix
    Vocabulary probe;
    probe.Build(text_path, h.scan8, {});
    for (auto& w : dict) {
      if (probe.table.Find(w.data(), w.size()) < 0) h.extra.push_back(w);
    }
  }
  m.voc.Build(text_path, h.scan8, h.extra);
  Stage("word counts");
  const Vocabulary& voc = m.voc;
  std::vector<uint32_t> rank_of_word;
  for (auto& w : dict) rank_of_word.push_back((uint32_t)voc.rank[voc.table.Find(w.data(), w.size())]);

  // Explicit words: a bitmap over ranks 0..the last of them.
  std::vector<char> is_head(voc.cand.size(), 0);
  uint32_t max_head = 0;
  for (size_t i = 0; i < h.S; ++i) {
    is_head[rank_of_word[i]] = 1;
    max_head = std::max(max_head, rank_of_word[i]);
  }
  std::vector<uint32_t> hset_ranks;
  std::vector<uint8_t> hset_bits;
  for (uint32_t r = 0; h.S && r <= max_head; ++r) {
    hset_ranks.push_back(r);
    hset_bits.push_back((uint8_t)is_head[r]);
    if (is_head[r]) m.head.push_back(r);
  }
  h.hset = hset_ranks.size();
  // Other words: maximal ascending runs, membership bitmap.
  std::vector<uint32_t> run_of(voc.cand.size(), UINT32_MAX);
  uint32_t run = 0;
  for (size_t i = h.S; i < dict.size(); ++i) {
    if (i > h.S && dict[i] <= dict[i - 1]) ++run;
    run_of[rank_of_word[i]] = run;
    m.tail.push_back(rank_of_word[i]);
  }
  h.runs = dict.size() > h.S ? run + 1 : 0;
  std::sort(m.tail.begin(), m.tail.end());
  size_t seen = 0;
  for (uint32_t r = 0; r < voc.cand.size(); ++r) {
    if (is_head[r]) continue;
    ++seen;
    if (run_of[r] != UINT32_MAX) h.n_cand = seen;
  }
  std::vector<uint32_t> border = BitmapOrder(voc, is_head, h.n_cand);
  std::vector<uint8_t> bbits;
  for (uint32_t r : border) bbits.push_back(run_of[r] != UINT32_MAX);

  m.Vectorize(text_path, h.scan9, h.K);
  std::vector<uint32_t> seq;  // explicit words in dictionary order, as indices into m.head
  for (size_t i = 0; i < h.S; ++i) {
    seq.push_back((uint32_t)(std::lower_bound(m.head.begin(), m.head.end(), rank_of_word[i]) - m.head.begin()));
  }
  std::vector<uint32_t> runs_j;  // run of each tail word
  for (uint32_t r : m.tail) runs_j.push_back(run_of[r]);

  WriteHeader(f, h);
  Dr3Fpu fpu;
  ModelInit();
  g_m->rc.StartEncode(f);
  Feats fs(voc, hset_ranks), fb(voc, border), fh(voc, m.head), ft(voc, m.tail);
  CodeBitmap(0, kHeadSet, hset_ranks.size(), fs.p.data(), hset_bits.data());
  CodeBitmap(0, kBitmap, border.size(), fb.p.data(), bbits.data());
  CodeHead(0, (uint)m.head.size(), fh.p.data(), m.gram.data(), seq.data());
  std::string scores;
  CentroidScores prov(h.runs, 2 * h.K, m.tail_vec, dump_dir ? &scores : nullptr);
  CodeRuns(0, (uint)m.tail.size(), (uint)h.runs, ft.p.data(), runs_j.data(), prov);
  g_m->rc.FinishEncode();
  Stage("coding");
  PrintStreams();
  ModelFree();
  if (dump_dir) {
    WriteDumps(dump_dir, voc, hset_ranks, hset_bits, border, bbits, m, seq, runs_j, h.runs, scores);
    Stage("dump");
  }
}

// Reads a side file from f and rebuilds the dictionary.
std::string Decode(const char* text_path, FILE* f) {
  Header h = ReadHeader(f);
  Model m;
  m.voc.Build(text_path, h.scan8, h.extra);
  Stage("word counts");
  const Vocabulary& voc = m.voc;
  if (h.hset > voc.cand.size()) Fail("explicit-word bitmap longer than the word list");
  Dr3Fpu fpu;
  ModelInit();
  g_m->rc.StartDecode(f);
  std::vector<uint32_t> hset_ranks(h.hset);
  for (size_t r = 0; r < h.hset; ++r) hset_ranks[r] = (uint32_t)r;
  std::vector<uint8_t> hbits(h.hset);
  Feats fs(voc, hset_ranks);
  CodeBitmap(1, kHeadSet, h.hset, fs.p.data(), hbits.data());
  std::vector<char> is_head(voc.cand.size(), 0);
  for (uint32_t r = 0; r < h.hset; ++r) {
    if (hbits[r]) {
      is_head[r] = 1;
      m.head.push_back(r);
    }
  }
  if (m.head.size() != h.S) Fail("bad explicit-word bitmap");
  std::vector<uint32_t> border = BitmapOrder(voc, is_head, h.n_cand);
  std::vector<uint8_t> bbits(border.size());
  Feats fb(voc, border);
  CodeBitmap(1, kBitmap, border.size(), fb.p.data(), bbits.data());
  for (size_t i = 0; i < border.size(); ++i) {
    if (bbits[i]) m.tail.push_back(border[i]);
  }
  std::sort(m.tail.begin(), m.tail.end());
  Stage("bitmaps");
  m.Vectorize(text_path, h.scan9, h.K);
  std::vector<uint32_t> seq(h.S);
  Feats fh(voc, m.head);
  CodeHead(1, (uint)h.S, fh.p.data(), m.gram.data(), seq.data());
  Stage("explicit word order");
  std::vector<uint32_t> runs_j(m.tail.size());
  Feats ft(voc, m.tail);
  CentroidScores prov(h.runs, 2 * h.K, m.tail_vec, nullptr);
  CodeRuns(1, (uint)m.tail.size(), (uint)h.runs, ft.p.data(), runs_j.data(), prov);
  Stage("run assignment");
  ModelFree();

  std::string text;
  for (uint32_t c : seq) text += voc.Word(m.head[c]) + '\n';
  std::vector<std::vector<uint32_t>> members(h.runs);
  for (size_t j = 0; j < m.tail.size(); ++j) members[runs_j[j]].push_back(m.tail[j]);
  for (auto& run : members) {
    std::sort(run.begin(), run.end(), [&](uint32_t a, uint32_t b) { return voc.WordLess(a, b); });
    for (uint32_t r : run) text += voc.Word(r) + '\n';
  }
  return text;
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
          "usage: dicrank3 e [-v] [-n BYTES] [-c BYTES] [-k K] [-s WORDS] [-D DIR] TEXT DICTIONARY SIDEFILE\n"
          "       dicrank3 d [-v] TEXT SIDEFILE DICTIONARY\n");
  return 2;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) return Usage();
  std::string mode = argv[1];
  Header h;
  h.S = 3920;
  const char* dump_dir = nullptr;
  std::vector<const char*> args;
  for (int i = 2; i < argc; ++i) {
    if (!strcmp(argv[i], "-n") && i + 1 < argc) h.scan8 = strtoull(argv[++i], nullptr, 10);
    else if (!strcmp(argv[i], "-c") && i + 1 < argc) h.scan9 = strtoull(argv[++i], nullptr, 10);
    else if (!strcmp(argv[i], "-k") && i + 1 < argc) h.K = (uint32_t)strtoul(argv[++i], nullptr, 10);
    else if (!strcmp(argv[i], "-s") && i + 1 < argc) h.S = strtoull(argv[++i], nullptr, 10);
    else if (!strcmp(argv[i], "-D") && i + 1 < argc) dump_dir = argv[++i];
    else if (!strcmp(argv[i], "-v")) g_verbose = true;
    else args.push_back(argv[i]);
  }
  if (args.size() != 3) return Usage();
  if (h.K == 0 || h.K > 32768) Fail("K must be 1..32768");
  double t0 = Seconds();
  if (mode == "e") {
    uint64_t size = FileSize(args[0]);
    h.scan8 = std::min(h.scan8, size);
    h.scan9 = h.scan9 ? std::min(h.scan9, size) : size;
    std::string dict_data = ReadFile(args[1]);
    std::vector<std::string> dict = ParseDictionary(dict_data);
    FILE* tmp = tmpfile();
    if (!tmp) Fail("cannot create a temporary file");
    Encode(args[0], dict, h, tmp, dump_dir);
    fflush(tmp);
    long bytes = ftell(tmp);
    double t1 = Seconds();
    rewind(tmp);
    if (Decode(args[0], tmp) != dict_data) {
      Fail("internal error: decoding the side file does not reproduce the dictionary");
    }
    std::string out((size_t)bytes, '\0');
    rewind(tmp);
    if (fread(&out[0], 1, out.size(), tmp) != out.size()) Fail("cannot read the temporary file");
    fclose(tmp);
    WriteFile(args[2], out);
    fprintf(stderr, "%zu words: %zu explicit, %zu in %zu runs, %zu not in the text; side file %ld bytes; "
            "encode %.1fs, check %.1fs\n", dict.size(), h.S, dict.size() - h.S, h.runs, h.extra.size(), bytes,
            t1 - t0, Seconds() - t1);
    return 0;
  }
  if (mode == "d") {
    FILE* f = fopen(args[1], "rb");
    if (!f) Fail("cannot open ", args[1]);
    std::string out = Decode(args[0], f);
    fclose(f);
    WriteFile(args[2], out);
    fprintf(stderr, "decode %.1fs\n", Seconds() - t0);
    return 0;
  }
  return Usage();
}
