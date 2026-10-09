// hello_eh - test target for del_eh: C++ with try blocks, destructors and
// library calls that need unwind tables, but no exception is thrown in a
// normal run.  An empty argument takes the throwing path:
//   hello_eh          prints "hello world 10" and exits with 0
//   hello_eh a bc     uses the arguments instead ("hello world 3")
//   hello_eh ""       throws std::invalid_argument; with the unwind tables
//                     intact it is caught (exit 2), without them the
//                     program ends in std::terminate
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

struct Guard {
  const char* name;
  int& depth;
  Guard(const char* n, int& d) : name(n), depth(d) { depth++; }
  ~Guard() { depth--; }
};

static int depth = 0;

static int measure(const std::string& s) {
  Guard g("measure", depth);
  if (s.empty()) throw std::invalid_argument("empty word");
  return (int)s.size();
}

static int total(const std::vector<std::string>& words) {
  Guard g("total", depth);
  int t = 0;
  for (const std::string& w : words) {
    try {
      t += measure(w);
    } catch (const std::length_error&) {  // never thrown here either
      t = -1;
    }
  }
  return t;
}

int main(int argc, char** argv) {
  std::vector<std::string> words;
  if (argc > 1) words.assign(argv + 1, argv + argc);
  else words = {"hello", "world"};
  try {
    int t = total(words);
    std::printf("hello world %d\n", t);
  } catch (const std::exception& e) {
    std::printf("caught: %s (depth %d)\n", e.what(), depth);
    return 2;
  }
  return depth;
}
