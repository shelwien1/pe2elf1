// dicrank2_dec.cpp - small decoder for dicrank2 (v2) side files, for linking
// into a compressor. It produces the same dictionary as `dicrank2 d` with the
// same integer arithmetic, but avoids the standard containers: the decoder in
// dicrank2.cpp adds 14.4 KB of packed code to fx2-cmix, and the program is
// stored twice (in the compressor and in archive9). Only C stdio, malloc and
// libc's qsort are used.
//
//   int Dicrank2Decode(const char* text_path, const char* side_path,
//                      const char* out_path);   // 0 on success
//
// Test build as a command-line tool:
//   g++ -O2 -DDICRANK2_DEC_MAIN -o dicrank2_dec dicrank2_dec.cpp
//   dicrank2_dec TEXT SIDEFILE DICTIONARY

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

struct Word {
  uint32_t off, len, count, df, low, last_page;
};

// Word table: open addressing over an arena of spellings.
char* g_arena;
uint64_t g_arena_len, g_arena_cap;
Word* g_words;
uint32_t g_nwords, g_words_cap;
uint32_t* g_slots;
uint32_t g_slot_mask;

uint32_t HashOf(const char* s, uint32_t len) {
  uint64_t h = 1469598103934665603ULL;
  for (uint32_t i = 0; i < len; ++i) h = (h ^ (unsigned char)s[i]) * 1099511628211ULL;
  return (uint32_t)(h ^ (h >> 29));
}

int64_t FindWord(const char* s, uint32_t len) {
  for (uint32_t i = HashOf(s, len) & g_slot_mask;; i = (i + 1) & g_slot_mask) {
    uint32_t k = g_slots[i];
    if (!k) return -1;
    const Word& w = g_words[k - 1];
    if (w.len == len && !memcmp(g_arena + w.off, s, len)) return k - 1;
  }
}

void InsertSlot(uint32_t k) {
  uint32_t i = HashOf(g_arena + g_words[k].off, g_words[k].len) & g_slot_mask;
  while (g_slots[i]) i = (i + 1) & g_slot_mask;
  g_slots[i] = k + 1;
}

uint32_t AddWord(const char* s, uint32_t len) {
  int64_t k = FindWord(s, len);
  if (k >= 0) return (uint32_t)k;
  if (2 * (g_nwords + 1) > g_slot_mask + 1) {  // grow the slot array
    free(g_slots);
    g_slot_mask = 2 * g_slot_mask + 1;
    g_slots = (uint32_t*)calloc((size_t)g_slot_mask + 1, sizeof(uint32_t));
    for (uint32_t j = 0; j < g_nwords; ++j) InsertSlot(j);
  }
  if (g_nwords == g_words_cap) g_words = (Word*)realloc(g_words, (g_words_cap *= 2) * sizeof(Word));
  while (g_arena_len + len > g_arena_cap) g_arena = (char*)realloc(g_arena, g_arena_cap *= 2);
  memcpy(g_arena + g_arena_len, s, len);
  g_words[g_nwords] = Word{(uint32_t)g_arena_len, len, 0, 0, 0, 0};
  g_arena_len += len;
  InsertSlot(g_nwords);
  return g_nwords++;
}

int CompareSpelling(uint32_t a, uint32_t b) {
  const Word &x = g_words[a], &y = g_words[b];
  int c = memcmp(g_arena + x.off, g_arena + y.off, x.len < y.len ? x.len : y.len);
  return c ? c : (x.len < y.len ? -1 : x.len > y.len);
}

// Splits text into words like cmix's word transform (see dicrank2.cpp).
struct Tokenizer {
  char* word;
  uint32_t len, cap;
  int upper, lower;
  bool lower_start;
};

// Returns true when a word ended before c; its bytes stay in t.word[0, len)
// until the caller appends the next letter.
inline bool Push(Tokenizer& t, unsigned char c, uint32_t* ended_len, bool* ended_lower) {
  bool is_lower = c >= 'a' && c <= 'z', is_upper = c >= 'A' && c <= 'Z';
  bool ended = false;
  if ((is_lower && t.upper > 1) || (is_upper && t.lower > 0) || (!is_lower && !is_upper)) {
    if (t.len) {
      ended = true;
      *ended_len = t.len;
      *ended_lower = t.lower_start;
    }
    t.len = 0;
    t.upper = t.lower = 0;
  }
  if (is_lower || is_upper) {
    if (t.len + 1 >= t.cap) t.word = (char*)realloc(t.word, t.cap *= 2);
    if (!t.len) t.lower_start = is_lower;
    if (is_lower) ++t.lower; else ++t.upper;
  }
  return ended;
}

// Appends the letter c (lowercased) to the current word.
inline void Append(Tokenizer& t, unsigned char c) {
  if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) t.word[t.len++] = (char)(c <= 'Z' ? c - 'A' + 'a' : c);
}

int64_t Log2Q16(uint64_t x) {
  int e = 63 - __builtin_clzll(x);
  uint64_t m = e >= 62 ? x >> (e - 62) : x << (62 - e);
  int64_t r = (int64_t)e << 16;
  for (int bit = 15; bit >= 0; --bit) {
    m = (uint64_t)(((unsigned __int128)m * m) >> 62);
    if (m >> 63) {
      m >>= 1;
      r |= (int64_t)1 << bit;
    }
  }
  return r;
}

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

int FloorLog2(uint64_t x) { return 63 - __builtin_clzll(x); }

// qsort comparators; their context lives in globals.
uint32_t* g_cand;          // word index of each rank
const int64_t* g_score;    // scores for the selection comparator

int ByCount(const void* x, const void* y) {
  uint32_t a = *(const uint32_t*)x, b = *(const uint32_t*)y;
  if (g_words[a].count != g_words[b].count) return g_words[a].count > g_words[b].count ? -1 : 1;
  return CompareSpelling(a, b);
}

uint64_t BitmapKey(uint32_t r) {  // length, df, count, lowercase share; rank breaks ties
  const Word& w = g_words[g_cand[r]];
  uint64_t length = (w.len < 12 ? w.len : 12) / 3, df = FloorLog2((uint64_t)w.df + 1);
  uint64_t count = FloorLog2((uint64_t)w.count + 1), share = 0;
  if (w.count) {
    share = 5ULL * w.low / w.count;
    if (share > 4) share = 4;
  }
  return (length << 60) | (df << 54) | (count << 48) | (share << 44) | r;
}

int ByKey(const void* x, const void* y) {
  uint64_t a = *(const uint64_t*)x, b = *(const uint64_t*)y;
  return a < b ? -1 : a > b;
}

int ByRank(const void* x, const void* y) {
  uint32_t a = *(const uint32_t*)x, b = *(const uint32_t*)y;
  return a < b ? -1 : a > b;
}

int ByScore(const void* x, const void* y) {  // descending score, ties by index
  uint32_t a = *(const uint32_t*)x, b = *(const uint32_t*)y;
  if (g_score[a] != g_score[b]) return g_score[a] > g_score[b] ? -1 : 1;
  return a < b ? -1 : a > b;
}

int ByWord(const void* x, const void* y) {  // alphabetical, for ranks
  return CompareSpelling(g_cand[*(const uint32_t*)x], g_cand[*(const uint32_t*)y]);
}

// The item at position k when items are ordered by ByScore.
uint32_t Select(uint32_t* items, uint32_t n, const int64_t* score, uint32_t k) {
  g_score = score;
  qsort(items, n, sizeof(uint32_t), ByScore);
  return items[k];
}

char* ReadAll(const char* path, uint64_t limit, uint64_t* size) {
  FILE* f = fopen(path, "rb");
  if (!f) return nullptr;
  fseek(f, 0, SEEK_END);
  uint64_t n = (uint64_t)ftell(f);
  fseek(f, 0, SEEK_SET);
  if (n > limit) n = limit;
  char* buf = (char*)malloc(n + 1);
  if (!buf || fread(buf, 1, n, f) != n) {
    fclose(f);
    free(buf);
    return nullptr;
  }
  fclose(f);
  buf[n] = 0;
  *size = n;
  return buf;
}

}  // namespace

int Dicrank2Decode(const char* text_path, const char* side_path, const char* out_path) {
  uint64_t side_size;
  char* side = ReadAll(side_path, UINT64_MAX, &side_size);
  if (!side) return 1;
  char* p = side;
  auto next_line = [&]() -> char* {
    char* line = p;
    char* nl = strchr(p, '\n');
    if (!nl) return nullptr;
    *nl = 0;
    p = nl + 1;
    return line;
  };
  char* line = next_line();
  if (!line || strcmp(line, "dicrank 2")) return 2;
  unsigned long long scan8, scan9;
  unsigned K;
  size_t S, R, extra;
  line = next_line();
  if (!line || sscanf(line, "%llu %llu %u %zu %zu %zu", &scan8, &scan9, &K, &S, &R, &extra) != 6) return 2;
  if (!K || K > 32768 || S > 10000) return 2;
  const uint32_t F = 2 * K;

  // Word counts, document frequency and lowercase use of the first scan8 bytes.
  uint64_t n8;
  char* text = ReadAll(text_path, scan8, &n8);
  if (!text || n8 != scan8) return 3;
  g_slot_mask = (1u << 20) - 1;
  g_slots = (uint32_t*)calloc((size_t)g_slot_mask + 1, sizeof(uint32_t));
  g_arena_cap = 1 << 22;
  g_arena = (char*)malloc(g_arena_cap);
  g_words_cap = 1 << 18;
  g_words = (Word*)malloc(g_words_cap * sizeof(Word));
  g_arena_len = g_nwords = 0;
  Tokenizer tok{(char*)malloc(64), 0, 64, 0, 0, false};
  uint32_t page = 0, wl;
  bool wlow;
  auto count_word = [&](uint32_t len, bool lower_start) {
    uint32_t k = AddWord(tok.word, len);  // may move g_words
    Word& e = g_words[k];
    ++e.count;
    if (lower_start) ++e.low;
    if (e.df == 0 || e.last_page != page) {
      ++e.df;
      e.last_page = page;
    }
  };
  for (uint64_t i = 0; i <= n8; ++i) {  // the final 0 ends the last word
    unsigned char c = i < n8 ? (unsigned char)text[i] : 0;
    if (Push(tok, c, &wl, &wlow)) count_word(wl, wlow);  // before a new page starts
    Append(tok, c);
    if (c == '<' && i + 6 <= n8 && !memcmp(text + i, "<page>", 6)) ++page;
  }
  free(text);

  // Candidates: extra words as listed, then counted words by count.
  uint32_t counted = g_nwords;
  uint32_t ncand = counted + (uint32_t)extra;
  g_cand = (uint32_t*)malloc(((size_t)ncand + 1) * sizeof(uint32_t));
  for (uint32_t i = 0; i < counted; ++i) g_cand[extra + i] = i;
  qsort(g_cand + extra, counted, sizeof(uint32_t), ByCount);
  for (size_t i = 0; i < extra; ++i) {
    line = next_line();
    if (!line) return 2;
    uint32_t before = g_nwords;
    AddWord(line, (uint32_t)strlen(line));
    if (g_nwords == before) return 2;  // an extra word must not occur in the text
    g_cand[i] = g_nwords - 1;
  }
  int64_t* rank_of = (int64_t*)malloc((size_t)g_nwords * sizeof(int64_t));
  for (uint32_t r = 0; r < ncand; ++r) rank_of[g_cand[r]] = r;

  // Explicit words (bitmap over ranks), then the other members.
  line = next_line();
  if (!line) return 2;
  size_t head_len = strlen(line);
  if (head_len > ncand) return 2;
  char* is_head = (char*)calloc(ncand, 1);
  uint32_t* targets = (uint32_t*)malloc(((size_t)ncand + 1) * sizeof(uint32_t));
  uint32_t nh = 0;
  for (uint32_t r = 0; r < head_len; ++r) {
    if (line[r] == '1') {
      is_head[r] = 1;
      targets[nh++] = r;
    }
  }
  if (nh != S) return 2;
  uint32_t* head_pos = (uint32_t*)malloc((S + 1) * sizeof(uint32_t));  // [0] = first word
  for (size_t i = 0; i < S; ++i) {
    line = next_line();
    if (!line) return 2;
    head_pos[i] = (uint32_t)strtoul(line, nullptr, 10);
  }
  line = next_line();
  if (!line) return 2;
  size_t n_cand = strtoull(line, nullptr, 10);
  char* bitmap = next_line();
  if (!bitmap || strlen(bitmap) != n_cand) return 2;
  uint64_t* keys = (uint64_t*)malloc((n_cand + 1) * sizeof(uint64_t));
  size_t k = 0;
  for (uint32_t r = 0; r < ncand && k < n_cand; ++r) {
    if (!is_head[r]) keys[k++] = BitmapKey(r);
  }
  if (k != n_cand) return 2;
  qsort(keys, n_cand, sizeof(uint64_t), ByKey);
  uint32_t nt = 0;
  for (size_t i = 0; i < n_cand; ++i) {
    if (bitmap[i] == '1') targets[nh + nt++] = (uint32_t)(keys[i] & 0xFFFFFFFFFFFULL);
  }
  free(keys);
  qsort(targets + nh, nt, sizeof(uint32_t), ByRank);  // most frequent first

  // Neighbour counts over the first scan9 bytes: features 0..K-1 = left
  // neighbour is the f-th most frequent word, K..2K-1 = right neighbour.
  uint32_t ntarg = nh + nt;
  int32_t* feature_of = (int32_t*)malloc((size_t)ncand * sizeof(int32_t));
  int32_t* target_of = (int32_t*)malloc((size_t)ncand * sizeof(int32_t));
  for (uint32_t r = 0; r < ncand; ++r) {
    feature_of[r] = r >= extra && r - extra < K ? (int32_t)(r - extra) : -1;
    target_of[r] = -1;
  }
  for (uint32_t t = 0; t < ntarg; ++t) target_of[targets[t]] = (int32_t)t;
  uint32_t* counts = (uint32_t*)calloc((size_t)ntarg * F, sizeof(uint32_t));
  FILE* f = fopen(text_path, "rb");
  if (!f || !counts) return 3;
  const size_t kBuf = 16 << 20;
  unsigned char* buf = (unsigned char*)malloc(kBuf);
  int64_t prev2 = -1, prev1 = -1;
  uint64_t tokens = 0, left = scan9;
  tok.len = tok.upper = tok.lower = 0;
  auto context_word = [&](uint32_t len) {
    int64_t w = FindWord(tok.word, len);
    int64_t cur = w >= 0 ? rank_of[w] : -1;
    if (tokens >= 2 && prev1 >= 0 && target_of[prev1] >= 0) {
      uint32_t* row = counts + (size_t)target_of[prev1] * F;
      if (prev2 >= 0 && feature_of[prev2] >= 0) ++row[feature_of[prev2]];
      if (cur >= 0 && feature_of[cur] >= 0) ++row[K + feature_of[cur]];
    }
    prev2 = prev1;
    prev1 = cur;
    ++tokens;
  };
  while (true) {
    size_t got = left ? fread(buf, 1, left < kBuf ? left : kBuf, f) : 0;
    if (left && !got) return 3;
    bool last = got == left;
    for (size_t i = 0; i < got + (last ? 1 : 0); ++i) {  // a final 0 ends the last word
      unsigned char c = i < got ? buf[i] : 0;
      if (Push(tok, c, &wl, &wlow)) context_word(wl);
      Append(tok, c);
    }
    if (last) break;
    left -= got;
  }
  fclose(f);
  free(buf);

  // PPMI (fixed-point log2), normalized to length <= 2^15, per group.
  uint64_t* start = (uint64_t*)malloc(((size_t)ntarg + 1) * sizeof(uint64_t));
  uint64_t cap = 1 << 22, nnz = 0;
  uint16_t* feat = (uint16_t*)malloc(cap * sizeof(uint16_t));
  int32_t* val = (int32_t*)malloc(cap * sizeof(int32_t));
  int64_t* norm2 = (int64_t*)malloc(((size_t)ntarg + 1) * sizeof(int64_t));
  uint64_t* col = (uint64_t*)malloc(F * sizeof(uint64_t));
  uint64_t* rowsum = (uint64_t*)malloc(((size_t)ntarg + 1) * sizeof(uint64_t));
  int64_t* pmi = (int64_t*)malloc(F * sizeof(int64_t));
  start[0] = 0;
  for (int g = 0; g < 2; ++g) {
    uint32_t lo = g ? nh : 0, hi = g ? ntarg : nh;
    uint64_t total = 0;
    memset(col, 0, F * sizeof(uint64_t));
    for (uint32_t t = lo; t < hi; ++t) {
      const uint32_t* c = counts + (size_t)t * F;
      uint64_t s = 0;
      for (uint32_t j = 0; j < F; ++j) {
        s += c[j];
        col[j] += c[j];
      }
      rowsum[t] = s;
      total += s;
    }
    for (uint32_t t = lo; t < hi; ++t) {
      const uint32_t* c = counts + (size_t)t * F;
      uint64_t sum2 = 0;
      for (uint32_t j = 0; j < F; ++j) {
        pmi[j] = 0;
        if (!c[j]) continue;
        int64_t v = Log2Q16((uint64_t)c[j] * total) - Log2Q16(rowsum[t] * col[j]);
        if (v <= 0) continue;
        pmi[j] = v;
        sum2 += (uint64_t)(v * v);
      }
      uint64_t norm = ISqrt(sum2);
      if (norm * norm < sum2) ++norm;
      int64_t n2 = 0;
      for (uint32_t j = 0; j < F; ++j) {
        if (!pmi[j]) continue;
        int32_t u = (int32_t)((pmi[j] << 15) / (int64_t)norm);
        if (!u) continue;
        if (nnz == cap) {
          cap *= 2;
          feat = (uint16_t*)realloc(feat, cap * sizeof(uint16_t));
          val = (int32_t*)realloc(val, cap * sizeof(int32_t));
        }
        feat[nnz] = (uint16_t)j;
        val[nnz++] = u;
        n2 += (int64_t)u * u;
      }
      norm2[t] = n2;
      start[t + 1] = nnz;
    }
  }
  free(counts);
  free(col);
  free(rowsum);
  free(pmi);

  // Order of the explicit words: similarity to the previous two words.
  uint32_t* out = (uint32_t*)malloc(((size_t)ntarg + 1) * sizeof(uint32_t));  // ranks, output order
  uint32_t nout = 0;
  if (nh) {
    int32_t* dense = (int32_t*)calloc((size_t)F * nh, sizeof(int32_t));  // feature-major
    int32_t* gram = (int32_t*)calloc((size_t)nh * nh, sizeof(int32_t));
    for (uint32_t a = 0; a < nh; ++a) {
      for (uint64_t i = start[a]; i < start[a + 1]; ++i) dense[(size_t)feat[i] * nh + a] = val[i];
    }
    for (uint32_t a = 0; a < nh; ++a) {
      int32_t* g = gram + (size_t)a * nh;
      for (uint64_t i = start[a]; i < start[a + 1]; ++i) {
        const int32_t* d = dense + (size_t)feat[i] * nh;
        int32_t x = val[i];
        for (uint32_t c = a; c < nh; ++c) g[c] += x * d[c];
      }
      for (uint32_t c = a + 1; c < nh; ++c) gram[(size_t)c * nh + a] = g[c];
    }
    free(dense);
    char* used = (char*)calloc(nh, 1);
    uint32_t* remaining = (uint32_t*)malloc(nh * sizeof(uint32_t));
    int64_t* score = (int64_t*)malloc(nh * sizeof(int64_t));
    uint32_t* seq = (uint32_t*)malloc(nh * sizeof(uint32_t));
    if (head_pos[0] >= nh) return 2;
    seq[0] = head_pos[0];
    used[seq[0]] = 1;
    for (uint32_t i = 1; i < nh; ++i) {
      uint32_t n = 0;
      for (uint32_t c = 0; c < nh; ++c) {
        if (used[c]) continue;
        remaining[n++] = c;
        int64_t s = 2 * (int64_t)gram[(size_t)seq[i - 1] * nh + c];
        if (i >= 2) s += gram[(size_t)seq[i - 2] * nh + c];
        score[c] = s;
      }
      if (head_pos[i] >= n) return 2;
      seq[i] = Select(remaining, n, score, head_pos[i]);
      used[seq[i]] = 1;
    }
    for (uint32_t i = 0; i < nh; ++i) out[nout++] = targets[seq[i]];
    free(gram); free(used); free(remaining); free(score); free(seq);
  }

  // Runs: each member (most frequent first) joins the run at the given
  // position among started runs, ordered by similarity to their centroids.
  int64_t* centroid = (int64_t*)calloc((size_t)F * R + 1, sizeof(int64_t));  // feature-major
  int64_t* rnorm2 = (int64_t*)calloc(R + 1, sizeof(int64_t));
  int64_t* dot = (int64_t*)malloc((R + 1) * sizeof(int64_t));
  int64_t* score = (int64_t*)malloc((R + 1) * sizeof(int64_t));
  char* started = (char*)calloc(R + 1, 1);
  uint32_t* list = (uint32_t*)malloc((R + 1) * sizeof(uint32_t));
  uint32_t* run_of = (uint32_t*)malloc(((size_t)nt + 1) * sizeof(uint32_t));
  uint32_t* run_size = (uint32_t*)calloc(R + 1, sizeof(uint32_t));
  for (uint32_t j = 0; j < nt; ++j) {
    uint32_t t = nh + j;
    line = next_line();
    if (!line) return 2;
    memset(dot, 0, R * sizeof(int64_t));
    for (uint64_t i = start[t]; i < start[t + 1]; ++i) {
      const int64_t* c = centroid + (size_t)feat[i] * R;
      int64_t x = val[i];
      for (size_t r = 0; r < R; ++r) dot[r] += x * c[r];
    }
    uint32_t run;
    if (line[0] == 'n') {
      unsigned long kth = strtoul(line + 1, nullptr, 10);
      run = UINT32_MAX;
      for (uint32_t r = 0, seen = 0; r < R; ++r) {
        if (started[r]) continue;
        if (seen++ == kth) {
          run = r;
          break;
        }
      }
      if (run == UINT32_MAX) return 2;
    } else {
      uint32_t n = 0;
      for (uint32_t r = 0; r < R; ++r) {
        if (!started[r]) continue;
        list[n++] = r;
        uint64_t len = ISqrt((uint64_t)rnorm2[r]);
        score[r] = len ? (int64_t)(((__int128)dot[r] << 20) / len) : 0;
      }
      unsigned long kth = strtoul(line, nullptr, 10);
      if (kth >= n) return 2;
      run = Select(list, n, score, (uint32_t)kth);
    }
    for (uint64_t i = start[t]; i < start[t + 1]; ++i) centroid[(size_t)feat[i] * R + run] += val[i];
    rnorm2[run] += 2 * dot[run] + norm2[t];
    started[run] = 1;
    run_of[j] = run;
    ++run_size[run];
  }
  // Each run's members alphabetically, runs in order.
  uint32_t* first = (uint32_t*)calloc(R + 1, sizeof(uint32_t));
  for (uint32_t r = 0; r < R; ++r) first[r + 1] = first[r] + run_size[r];
  uint32_t* grouped = (uint32_t*)malloc(((size_t)nt + 1) * sizeof(uint32_t));
  uint32_t* fill = (uint32_t*)malloc((R + 1) * sizeof(uint32_t));
  memcpy(fill, first, (R + 1) * sizeof(uint32_t));
  for (uint32_t j = 0; j < nt; ++j) grouped[fill[run_of[j]]++] = targets[nh + j];
  for (uint32_t r = 0; r < R; ++r) qsort(grouped + first[r], run_size[r], sizeof(uint32_t), ByWord);
  for (uint32_t j = 0; j < nt; ++j) out[nout++] = grouped[j];

  FILE* o = fopen(out_path, "wb");
  if (!o) return 4;
  for (uint32_t i = 0; i < nout; ++i) {
    const Word& w = g_words[g_cand[out[i]]];
    fwrite(g_arena + w.off, 1, w.len, o);
    fputc('\n', o);
  }
  fclose(o);
  free(side); free(g_slots); free(g_arena); free(g_words); free(tok.word); free(g_cand);
  free(rank_of); free(is_head); free(targets); free(head_pos); free(feature_of); free(target_of);
  free(start); free(feat); free(val); free(norm2); free(out); free(centroid); free(rnorm2);
  free(dot); free(score); free(started); free(list); free(run_of); free(run_size); free(first);
  free(grouped); free(fill);
  return 0;
}

#ifdef DICRANK2_DEC_MAIN
int main(int argc, char** argv) {
  if (argc != 4) {
    fprintf(stderr, "usage: dicrank2_dec TEXT SIDEFILE DICTIONARY\n");
    return 2;
  }
  int err = Dicrank2Decode(argv[1], argv[2], argv[3]);
  if (err) fprintf(stderr, "dicrank2_dec: error %d\n", err);
  return err;
}
#endif
