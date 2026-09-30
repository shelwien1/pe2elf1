// dicrank_dec.cpp - small decoder for dicrank side files, for linking into a
// compressor. It produces the same dictionary as `dicrank d` but avoids the
// standard containers. Linked into fx2-cmix (compiled with -Os), it adds 4,144
// bytes to the program before packing and 2,004 after UPX --ultra-brute; the
// decoder from dicrank.cpp adds 12,116. The program is stored twice (in the
// compressor and in archive9), so this matters. It uses only C stdio, malloc
// and libc's qsort.
//
//   int DicrankDecode(const char* text_path, const char* side_path,
//                     const char* out_path);   // 0 on success
//
// Test build as a command-line tool:
//   g++ -O2 -DDICRANK_DEC_MAIN -o dicrank_dec dicrank_dec.cpp
//   dicrank_dec TEXT SIDEFILE DICTIONARY

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

struct Word {
  uint32_t off, len, count;  // lowercased spelling at arena[off .. off+len)
};

char* g_arena;
Word* g_words;

int CompareBytes(const char* a, uint32_t la, const char* b, uint32_t lb) {
  int c = memcmp(a, b, la < lb ? la : lb);
  return c ? c : (la < lb ? -1 : la > lb);
}

// Descending count, then alphabetical (the order dicrank uses for ranks).
int ByFrequency(const void* x, const void* y) {
  const Word& a = g_words[*(const uint32_t*)x];
  const Word& b = g_words[*(const uint32_t*)y];
  if (a.count != b.count) return a.count > b.count ? -1 : 1;
  return CompareBytes(g_arena + a.off, a.len, g_arena + b.off, b.len);
}

int Alphabetical(const void* x, const void* y) {
  const Word& a = g_words[*(const uint32_t*)x];
  const Word& b = g_words[*(const uint32_t*)y];
  return CompareBytes(g_arena + a.off, a.len, g_arena + b.off, b.len);
}

// Compare the spellings read backwards.
int ByReversedSpelling(const void* x, const void* y) {
  const Word& a = g_words[*(const uint32_t*)x];
  const Word& b = g_words[*(const uint32_t*)y];
  const char* pa = g_arena + a.off + a.len;
  const char* pb = g_arena + b.off + b.len;
  uint32_t n = a.len < b.len ? a.len : b.len;
  for (uint32_t i = 1; i <= n; ++i) {
    if (pa[-(int64_t)i] != pb[-(int64_t)i]) return pa[-(int64_t)i] < pb[-(int64_t)i] ? -1 : 1;
  }
  return a.len < b.len ? -1 : a.len > b.len;
}

char* ReadAll(const char* path, uint64_t limit, uint64_t* size) {
  FILE* f = fopen(path, "rb");
  if (!f) return nullptr;
  fseek(f, 0, SEEK_END);
  uint64_t n = (uint64_t)ftell(f);
  fseek(f, 0, SEEK_SET);
  if (n > limit) n = limit;
  char* buf = (char*)malloc(n + 1);
  if (!buf || fread(buf, 1, n, f) != n) { fclose(f); free(buf); return nullptr; }
  fclose(f);
  buf[n] = 0;
  *size = n;
  return buf;
}

}  // namespace

int DicrankDecode(const char* text_path, const char* side_path,
                  const char* out_path) {
  uint64_t side_size, text_size;
  char* side = ReadAll(side_path, UINT64_MAX, &side_size);
  if (!side) return 1;
  char* p = side;
  auto next_line = [&]() -> char* {  // returns the line, 0-terminated
    char* line = p;
    char* nl = strchr(p, '\n');
    if (!nl) return nullptr;
    *nl = 0;
    p = nl + 1;
    return line;
  };
  char* line = next_line();
  if (!line || strcmp(line, "dicrank 1")) return 2;
  unsigned long long scanned;
  size_t explicit_words, runs, bitmap_len, extra;
  line = next_line();
  if (!line || sscanf(line, "%llu %zu %zu %zu %zu", &scanned, &explicit_words,
                      &runs, &bitmap_len, &extra) != 5) return 2;

  // Word counts of the text, split like cmix's word transform.
  char* text = ReadAll(text_path, scanned, &text_size);
  if (!text || text_size != scanned) return 3;
  const uint32_t kSlots = 1u << 22;  // plenty for 10^8 bytes (~282k words)
  uint32_t* slot = (uint32_t*)calloc(kSlots, sizeof(uint32_t));  // word index + 1
  uint64_t arena_cap = 1 << 22, arena_len = 0;
  uint32_t words_cap = 1 << 18, nwords = 0;
  g_arena = (char*)malloc(arena_cap);
  g_words = (Word*)malloc(words_cap * sizeof(Word));
  auto add = [&](const char* s, uint32_t len, uint32_t inc) {
    uint32_t h = 2166136261u;
    for (uint32_t i = 0; i < len; ++i) h = (h ^ (unsigned char)s[i]) * 16777619u;
    for (uint32_t i = h & (kSlots - 1);; i = (i + 1) & (kSlots - 1)) {
      if (!slot[i]) {
        if (nwords == words_cap) g_words = (Word*)realloc(g_words, (words_cap *= 2) * sizeof(Word));
        while (arena_len + len > arena_cap) g_arena = (char*)realloc(g_arena, arena_cap *= 2);
        memcpy(g_arena + arena_len, s, len);
        g_words[nwords] = Word{(uint32_t)arena_len, len, inc};
        arena_len += len;
        slot[i] = ++nwords;
        return;
      }
      Word& e = g_words[slot[i] - 1];
      if (e.len == len && !memcmp(g_arena + e.off, s, len)) { e.count += inc; return; }
    }
  };
  // A word is a contiguous stretch of the text; it is lowercased in place.
  uint64_t start_of_word = 0;
  uint32_t wl = 0;
  int upper = 0, lower = 0;
  for (uint64_t i = 0; i <= text_size; ++i) {
    unsigned char c = i < text_size ? (unsigned char)text[i] : 0;
    bool is_lower = c >= 'a' && c <= 'z', is_upper = c >= 'A' && c <= 'Z';
    if ((is_lower && upper > 1) || (is_upper && lower > 0) || (!is_lower && !is_upper)) {
      if (wl) {
        char* s = text + start_of_word;
        for (uint32_t j = 0; j < wl; ++j) {
          if (s[j] <= 'Z') s[j] = (char)(s[j] - 'A' + 'a');
        }
        add(s, wl, 1);
      }
      wl = 0;
      upper = lower = 0;
    }
    if (is_lower || is_upper) {
      if (!wl) start_of_word = i;
      ++wl;
      if (is_lower) ++lower; else ++upper;
    }
  }
  free(text);

  // Candidate list: the extra words as listed, then the counted words by
  // frequency.
  uint32_t counted = nwords;
  uint32_t* cand = (uint32_t*)malloc((counted + extra) * sizeof(uint32_t));
  for (uint32_t i = 0; i < counted; ++i) cand[extra + i] = i;
  qsort(cand + extra, counted, sizeof(uint32_t), ByFrequency);
  for (size_t i = 0; i < extra; ++i) {
    line = next_line();
    if (!line) return 2;
    uint32_t before = nwords;
    add(line, (uint32_t)strlen(line), 0);
    if (nwords == before) return 2;  // an extra word must not occur in the text
    cand[i] = nwords - 1;
  }
  uint32_t ncand = counted + (uint32_t)extra;

  // Explicitly ordered words, given as ranks.
  uint32_t* out = (uint32_t*)malloc(ncand * sizeof(uint32_t));
  size_t nout = 0;
  char* is_head = (char*)calloc(ncand, 1);
  for (size_t i = 0; i < explicit_words; ++i) {
    line = next_line();
    if (!line) return 2;
    unsigned long r = strtoul(line, nullptr, 10);
    if (r >= ncand) return 2;
    out[nout++] = cand[r];
    is_head[r] = 1;
  }
  // Membership bitmap over the remaining candidates.
  line = next_line();
  if (!line || strlen(line) != bitmap_len) return 2;
  uint32_t* tail = (uint32_t*)malloc(ncand * sizeof(uint32_t));
  size_t ntail = 0, k = 0;
  for (uint32_t r = 0; r < ncand && k < bitmap_len; ++r) {
    if (is_head[r]) continue;
    if (line[k++] == '1') tail[ntail++] = cand[r];
  }
  if (k != bitmap_len) return 2;
  // Run numbers in order of reversed spelling; then each run alphabetically.
  qsort(tail, ntail, sizeof(uint32_t), ByReversedSpelling);
  uint32_t* run_of = (uint32_t*)malloc((ntail + 1) * sizeof(uint32_t));
  uint32_t* start = (uint32_t*)calloc(runs + 1, sizeof(uint32_t));
  for (size_t i = 0; i < ntail; ++i) {
    line = next_line();
    if (!line) return 2;
    unsigned long r = strtoul(line, nullptr, 10);
    if (r >= runs) return 2;
    run_of[i] = (uint32_t)r;
    ++start[r + 1];
  }
  for (size_t r = 0; r < runs; ++r) start[r + 1] += start[r];
  uint32_t* grouped = (uint32_t*)malloc((ntail + 1) * sizeof(uint32_t));
  uint32_t* fill = (uint32_t*)malloc((runs + 1) * sizeof(uint32_t));
  memcpy(fill, start, (runs + 1) * sizeof(uint32_t));
  for (size_t i = 0; i < ntail; ++i) grouped[fill[run_of[i]]++] = tail[i];
  for (size_t r = 0; r < runs; ++r) {
    qsort(grouped + start[r], start[r + 1] - start[r], sizeof(uint32_t), Alphabetical);
  }
  for (size_t i = 0; i < ntail; ++i) out[nout++] = grouped[i];

  FILE* f = fopen(out_path, "wb");
  if (!f) return 4;
  for (size_t i = 0; i < nout; ++i) {
    fwrite(g_arena + g_words[out[i]].off, 1, g_words[out[i]].len, f);
    fputc('\n', f);
  }
  fclose(f);
  free(side); free(slot); free(g_arena); free(g_words); free(cand); free(out);
  free(is_head); free(tail); free(run_of); free(start); free(grouped); free(fill);
  return 0;
}

#ifdef DICRANK_DEC_MAIN
int main(int argc, char** argv) {
  if (argc != 4) {
    fprintf(stderr, "usage: dicrank_dec TEXT SIDEFILE DICTIONARY\n");
    return 2;
  }
  int err = DicrankDecode(argv[1], argv[2], argv[3]);
  if (err) fprintf(stderr, "dicrank_dec: error %d\n", err);
  return err;
}
#endif
