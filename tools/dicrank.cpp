// dicrank.cpp - store english.dic (the word dictionary of fx2-cmix / cmix-lex)
// as a small side file that is expanded again with the help of enwik8/enwik9.
//
// The compressor has to carry english.dic (100,088 bytes after cmix) because
// it needs the dictionary to compress enwik9, but it also has enwik9 itself.
// Every dictionary word occurs in the first 10^8 bytes of enwik9 (= enwik8)
// when the text is split into words exactly like cmix's word transform does,
// so the compressor only needs to know which of those words to take and in
// what order. english.dic is laid out like this:
//
//  - The first 3920 words fill the 1- and 2-byte codeword tiers of the word
//    transform and are in a hand-made order. They are stored as frequency
//    ranks.
//  - The remaining 40595 words consist of 510 maximal runs that are sorted
//    alphabetically (groups of words of one kind: adverbs, verb forms,
//    plant names, medieval names, ...). They are stored as a membership
//    bitmap over the frequency-sorted word list, plus a run number per word.
//    The run numbers are listed in order of the words' reversed spelling, so
//    words with the same ending, and usually the same kind of word, are next
//    to each other.
//
// With fx2-cmix's own `cmix -c`, the side file for english.dic and enwik8 is
// 45,569 bytes, against 100,088 for english.dic itself. Measured on prototype
// files, listing the run numbers alphabetically costs 3.4 KB more, in
// frequency order 5.9 KB more, and move-to-front coding them 2.2 KB more.
// Plain frequency ranks for all words (no runs) come to 82,448 bytes.
//
// Any word list of unique lowercase a-z words, one per line, can be encoded;
// words that do not occur in the text are stored literally. The side file is
// plain text meant to be compressed with the entry's own `cmix -c`, like the
// other side files. archive9 still needs the full dictionary (it has no enwik9
// yet), so this only makes the compressor smaller.
//
// build:  g++ -O2 -std=c++17 -o dicrank dicrank.cpp
// encode: dicrank e [-n BYTES] [-s WORDS] TEXT DICTIONARY SIDEFILE
// decode: dicrank d TEXT SIDEFILE DICTIONARY
//   TEXT      enwik8 or enwik9 (only the first BYTES are read, default 10^8,
//             so both give the same result)
//   -s WORDS  words kept in explicit order (default 3920)
// The encoder decodes its own output and fails unless it reproduces the
// dictionary byte for byte.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

[[noreturn]] void Fail(const char* msg, const std::string& arg = "") {
  fprintf(stderr, "dicrank: %s%s\n", msg, arg.c_str());
  exit(1);
}

std::string ReadFile(const char* path, uint64_t limit = UINT64_MAX) {
  FILE* f = fopen(path, "rb");
  if (!f) Fail("cannot open ", path);
  std::string data;
  char buf[1 << 16];
  while (data.size() < limit) {
    size_t want = (size_t)std::min<uint64_t>(sizeof(buf), limit - data.size());
    size_t got = fread(buf, 1, want, f);
    if (got == 0) break;
    data.append(buf, got);
  }
  fclose(f);
  return data;
}

void WriteFile(const char* path, const std::string& data) {
  FILE* f = fopen(path, "wb");
  if (!f || fwrite(data.data(), 1, data.size(), f) != data.size()) {
    Fail("cannot write ", path);
  }
  fclose(f);
}

// Word frequencies of the text, tokenized like preprocessor::Dictionary::Encode
// in cmix: a lowercase letter after two or more capitals starts a new word
// ("JScript" -> "js" "cript"), a capital after lowercase letters starts a new
// word ("McDonalds" -> "mc" "donalds"), anything but A-Z/a-z ends a word.
// Words are lowercased. (The transform also cuts words longer than the longest
// dictionary word; such words are never in the dictionary, so that is ignored.)
std::unordered_map<std::string, uint32_t> CountWords(const std::string& text) {
  std::unordered_map<std::string, uint32_t> count;
  count.reserve(1 << 20);
  std::string word;
  int num_upper = 0, num_lower = 0;
  auto flush = [&]() {
    if (!word.empty()) ++count[word];
    word.clear();
    num_upper = num_lower = 0;
  };
  for (unsigned char c : text) {
    if (c >= 'a' && c <= 'z') {
      if (num_upper > 1) flush();
      ++num_lower;
      word += (char)c;
    } else if (c >= 'A' && c <= 'Z') {
      if (num_lower > 0) flush();
      ++num_upper;
      word += (char)(c - 'A' + 'a');
    } else {
      flush();
    }
  }
  flush();
  return count;
}

// The extra words (dictionary words the text lacks) in the given order,
// followed by all words of the text by descending frequency, ties alphabetical.
// Putting the extras first keeps the membership bitmap short.
std::vector<std::string> Candidates(const std::string& text,
                                    const std::vector<std::string>& extra) {
  auto count = CountWords(text);
  std::vector<std::pair<uint32_t, std::string>> v;
  v.reserve(count.size());
  for (auto& kv : count) v.emplace_back(kv.second, kv.first);
  std::sort(v.begin(), v.end(), [](const auto& a, const auto& b) {
    return a.first != b.first ? a.first > b.first : a.second < b.second;
  });
  std::vector<std::string> words(extra);
  words.reserve(v.size() + extra.size());
  for (auto& p : v) words.push_back(std::move(p.second));
  return words;
}

bool ReversedLess(const std::string& a, const std::string& b) {
  return std::lexicographical_compare(a.rbegin(), a.rend(), b.rbegin(), b.rend());
}

std::string Join(const std::vector<std::string>& words) {
  std::string out;
  for (auto& w : words) out += w + '\n';
  return out;
}

// Side file layout (text):
//   dicrank 1
//   <scanned bytes> <explicit words S> <runs> <bitmap length> <extra words>
//   <extra words, one per line>
//   <S lines: rank of each explicitly ordered word>
//   <bitmap: one '0'/'1' per candidate, frequency order, explicit words skipped>
//   <one line per remaining word, in order of reversed spelling: run number>
struct Side {
  uint64_t scanned = 0;
  size_t explicit_words = 0, runs = 0;
  std::vector<std::string> extra;
  std::vector<uint32_t> head_ranks;
  std::string bitmap;
  std::vector<uint32_t> run_ids;
};

std::string FormatSide(const Side& s) {
  std::string out = "dicrank 1\n";
  char line[128];
  snprintf(line, sizeof(line), "%llu %zu %zu %zu %zu\n",
           (unsigned long long)s.scanned, s.explicit_words, s.runs,
           s.bitmap.size(), s.extra.size());
  out += line;
  for (auto& w : s.extra) out += w + '\n';
  for (uint32_t r : s.head_ranks) out += std::to_string(r) + '\n';
  out += s.bitmap + '\n';
  for (uint32_t r : s.run_ids) out += std::to_string(r) + '\n';
  return out;
}

Side ParseSide(const std::string& data) {
  Side s;
  size_t pos = 0;
  auto next_line = [&]() {
    size_t end = data.find('\n', pos);
    if (end == std::string::npos) Fail("side file is truncated");
    std::string l = data.substr(pos, end - pos);
    pos = end + 1;
    return l;
  };
  auto number = [&]() {
    std::string l = next_line();
    char* end;
    unsigned long v = strtoul(l.c_str(), &end, 10);
    if (l.empty() || *end) Fail("bad number in side file: ", l);
    return (uint32_t)v;
  };
  if (next_line() != "dicrank 1") Fail("not a dicrank side file");
  unsigned long long scanned;
  size_t bitmap_len, extra;
  if (sscanf(next_line().c_str(), "%llu %zu %zu %zu %zu", &scanned,
             &s.explicit_words, &s.runs, &bitmap_len, &extra) != 5) {
    Fail("bad side file header");
  }
  s.scanned = scanned;
  for (size_t i = 0; i < extra; ++i) s.extra.push_back(next_line());
  for (size_t i = 0; i < s.explicit_words; ++i) s.head_ranks.push_back(number());
  s.bitmap = next_line();
  if (s.bitmap.size() != bitmap_len) Fail("bad bitmap length");
  size_t ones = std::count(s.bitmap.begin(), s.bitmap.end(), '1');
  for (size_t i = 0; i < ones; ++i) s.run_ids.push_back(number());
  if (pos != data.size()) Fail("trailing data in side file");
  return s;
}

std::vector<std::string> Decode(const Side& s, const std::string& text) {
  std::vector<std::string> cand = Candidates(text, s.extra);
  std::vector<std::string> out;
  std::unordered_set<std::string> head;
  for (uint32_t r : s.head_ranks) {
    if (r >= cand.size()) Fail("rank out of range");
    out.push_back(cand[r]);
    head.insert(cand[r]);
  }
  std::vector<std::string> tail;
  size_t k = 0;
  for (auto& w : cand) {
    if (k == s.bitmap.size()) break;
    if (head.count(w)) continue;
    if (s.bitmap[k++] == '1') tail.push_back(w);
  }
  if (k != s.bitmap.size()) Fail("bitmap longer than the word list");
  std::sort(tail.begin(), tail.end(), ReversedLess);
  std::vector<std::vector<std::string>> runs(s.runs);
  for (size_t i = 0; i < tail.size(); ++i) {
    if (s.run_ids[i] >= s.runs) Fail("run number out of range");
    runs[s.run_ids[i]].push_back(tail[i]);
  }
  for (auto& r : runs) {
    std::sort(r.begin(), r.end());
    out.insert(out.end(), r.begin(), r.end());
  }
  return out;
}

Side Encode(const std::vector<std::string>& dict, const std::string& text,
            size_t explicit_words) {
  Side s;
  s.scanned = text.size();
  s.explicit_words = std::min(explicit_words, dict.size());
  {
    auto count = CountWords(text);
    for (auto& w : dict) {
      if (!count.count(w)) s.extra.push_back(w);
    }
  }
  std::vector<std::string> cand = Candidates(text, s.extra);
  std::unordered_map<std::string, uint32_t> rank;
  rank.reserve(cand.size());
  for (size_t i = 0; i < cand.size(); ++i) rank.emplace(cand[i], (uint32_t)i);

  std::unordered_set<std::string> head;
  for (size_t i = 0; i < s.explicit_words; ++i) {
    s.head_ranks.push_back(rank.at(dict[i]));
    head.insert(dict[i]);
  }
  // Remaining words: maximal strictly ascending runs.
  std::unordered_map<std::string, uint32_t> run_of;
  uint32_t run = 0;
  for (size_t i = s.explicit_words; i < dict.size(); ++i) {
    if (i > s.explicit_words && dict[i] <= dict[i - 1]) ++run;
    run_of.emplace(dict[i], run);
  }
  s.runs = dict.size() > s.explicit_words ? run + 1 : 0;
  // Membership bitmap over the candidates in frequency order, without the
  // explicitly ordered words, up to the last member.
  std::string bitmap;
  size_t last = 0;
  for (auto& w : cand) {
    if (head.count(w)) continue;
    bool member = run_of.count(w) > 0;
    bitmap += member ? '1' : '0';
    if (member) last = bitmap.size();
  }
  bitmap.resize(last);
  s.bitmap = bitmap;
  // Run numbers in order of reversed spelling.
  std::vector<std::string> tail;
  for (auto& kv : run_of) tail.push_back(kv.first);
  std::sort(tail.begin(), tail.end(), ReversedLess);
  for (auto& w : tail) s.run_ids.push_back(run_of[w]);
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
  std::unordered_set<std::string> seen(words.begin(), words.end());
  if (seen.size() != words.size()) Fail("dictionary words must be unique");
  return words;
}

int Usage() {
  fprintf(stderr,
          "usage: dicrank e [-n BYTES] [-s WORDS] TEXT DICTIONARY SIDEFILE\n"
          "       dicrank d TEXT SIDEFILE DICTIONARY\n");
  return 2;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) return Usage();
  std::string mode = argv[1];
  uint64_t scan = 100000000;
  size_t explicit_words = 3920;
  std::vector<const char*> args;
  for (int i = 2; i < argc; ++i) {
    if (!strcmp(argv[i], "-n") && i + 1 < argc) scan = strtoull(argv[++i], nullptr, 10);
    else if (!strcmp(argv[i], "-s") && i + 1 < argc) explicit_words = strtoull(argv[++i], nullptr, 10);
    else args.push_back(argv[i]);
  }
  if (args.size() != 3) return Usage();

  if (mode == "e") {
    std::string text = ReadFile(args[0], scan);
    std::string dict_data = ReadFile(args[1]);
    std::vector<std::string> dict = ParseDictionary(dict_data);
    Side side = Encode(dict, text, explicit_words);
    std::string out = FormatSide(side);
    if (Join(Decode(ParseSide(out), text)) != dict_data) {
      Fail("internal error: decoding the side file does not reproduce the dictionary");
    }
    WriteFile(args[2], out);
    fprintf(stderr,
            "%zu words: %zu in explicit order, %zu in %zu alphabetical runs, "
            "%zu not in the text; bitmap %zu; side file %zu bytes\n",
            dict.size(), side.explicit_words, side.run_ids.size(), side.runs,
            side.extra.size(), side.bitmap.size(), out.size());
    return 0;
  }
  if (mode == "d") {
    Side side = ParseSide(ReadFile(args[1]));
    std::string text = ReadFile(args[0], side.scanned);
    if (text.size() != side.scanned) Fail("text is shorter than when encoding");
    WriteFile(args[2], Join(Decode(side, text)));
    return 0;
  }
  return Usage();
}
