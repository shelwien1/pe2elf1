// case_rerank.cpp - dicrank2's run numbers with case agreement folded into the
// ranking (docs/english_dic_reconstruction.md §9, "What might pay").
//
// Reads a dump written by "dicrank3 e -D DUMP ..." (runs.bin, words.txt) and
// writes the run numbers as dicrank2's side file lists them: "n<k>" for a new
// run, else the position of the word's run among the started runs. The
// runs are ranked by
//
//   score + beta * 2^35 * ln((m + 0.5) / (n + 1))
//
// where score is dicrank2's centroid score (cosine x 2^35), n the run's size
// and m the number of its members in the word's case class (the BitmapKey
// lowercase-share bucket). beta = 0 reproduces dicrank2's run section. It
// prints the sum of log2(position + 1), the proxy used to pick beta; compress
// OUT with cmix for the real size.
//
// build: g++ -O2 -o case_rerank case_rerank.cpp
// usage: case_rerank DUMP BETA [OUT]      (the doc uses BETA = 0.01)

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

static void Read(FILE* f, void* p, size_t n) {
  if (fread(p, 1, n, f) != n) { fprintf(stderr, "case_rerank: truncated runs.bin\n"); exit(1); }
}

int main(int argc, char** argv) {
  if (argc < 3) { fprintf(stderr, "usage: case_rerank DUMP BETA [OUT]\n"); return 2; }
  std::string dir = argv[1];
  double beta = atof(argv[2]);
  FILE* f = fopen((dir + "/runs.bin").c_str(), "rb");
  if (!f) { fprintf(stderr, "case_rerank: cannot open runs.bin\n"); return 1; }
  uint32_t T, R;
  Read(f, &T, 4); Read(f, &R, 4);
  std::vector<uint32_t> rank(T), truth(T);
  Read(f, rank.data(), 4 * (size_t)T); Read(f, truth.data(), 4 * (size_t)T);
  std::vector<int64_t> sc((size_t)T * R);
  Read(f, sc.data(), 8 * sc.size());
  fclose(f);
  std::vector<int> cls;  // case class of each rank: min(4, 5 * lowercase / count)
  FILE* g = fopen((dir + "/words.txt").c_str(), "r");
  if (!g) { fprintf(stderr, "case_rerank: cannot open words.txt\n"); return 1; }
  char w[256]; unsigned c, d, l;
  while (fscanf(g, "%255s %u %u %u", w, &c, &d, &l) == 4) cls.push_back(c ? (int)std::min(4u, 5 * l / c) : 0);
  fclose(g);

  std::vector<int> n(R, 0), m(R * 5, 0);
  std::vector<char> started(R, 0);
  std::vector<uint32_t> unstarted(R);
  for (uint32_t r = 0; r < R; r++) unstarted[r] = r;
  std::vector<double> s(R);
  std::string out;
  double proxy = 0;
  for (uint32_t j = 0; j < T; j++) {
    const int64_t* sj = &sc[(size_t)j * R];
    int k = cls[rank[j]];
    uint32_t t = truth[j];
    if (!started[t]) {
      size_t i = std::find(unstarted.begin(), unstarted.end(), t) - unstarted.begin();
      out += "n" + std::to_string(i) + "\n";
      unstarted.erase(unstarted.begin() + i);
      started[t] = 1;
    } else {
      for (uint32_t r = 0; r < R; r++) {
        if (started[r]) s[r] = (double)sj[r] + beta * 34359738368.0 * std::log((m[r * 5 + k] + 0.5) / (n[r] + 1.0));
      }
      size_t pos = 0;
      for (uint32_t r = 0; r < R; r++) {
        if (started[r] && r != t && (s[r] > s[t] || (s[r] == s[t] && r < t))) pos++;
      }
      out += std::to_string(pos) + "\n";
      proxy += std::log2(pos + 1.0);
    }
    n[t]++;
    m[t * 5 + k]++;
  }
  printf("beta %g: sum log2(position + 1) = %.0f bits\n", beta, proxy);
  if (argc > 3) {
    FILE* o = fopen(argv[3], "w");
    if (!o || fwrite(out.data(), 1, out.size(), o) != out.size()) { fprintf(stderr, "case_rerank: cannot write\n"); return 1; }
    fclose(o);
  }
  return 0;
}
