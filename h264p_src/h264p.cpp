// h264p: a lossless compressor of H.264 Annex B streams (c, d) -- its
// command line. All the rest is h264p.inc, which a program of its own can
// include instead, for the coroutines of coro.inc.
#include "h264p.inc"

namespace h264p {

void usage() {
  std::string lv;
  for (int n : kLevels) lv += " -" + std::to_string(n);
  fprintf(stderr,
          "usage: h264p c|d [options] input output\n"
          "  c  compress an H.264 Annex B stream\n"
          "  d  decompress it\n"
          "options:\n"
          "  -v    print statistics\n"
          "  -s    strict (c): fail if a NAL unit of a supported type has to be stored verbatim\n"
          "  -p    progress on stderr even when it is not a terminal (where it is, always)\n"
          "  %s  (c) the model's level, from the strongest (the default) to the fastest\n"
          "  -t N  (c) compress on N threads, 1 to 64 (default 1): the stream in parts that start\n"
          "        at IDR or I access units (at any picture past 4 times the part size), a model\n"
          "        for each; d takes N from the stream\n"
          "  -c N  (c with -t) the size of a part, at least: N MB (1 to 1024), or N KB with a k\n"
          "        (default 4)\n"
          "input and output can be - for stdin and stdout, and not the same file\n",
          lv.c_str() + 1);
}

}  // namespace h264p

int main(int argc, char** argv) {
  using namespace h264p;
  if (argc < 2 || !argv[1][0] || argv[1][1] || !strchr("cd", argv[1][0])) {
    usage();
    return 2;
  }
#ifdef _WIN32
  _setmode(_fileno(stdin), _O_BINARY);
  _setmode(_fileno(stdout), _O_BINARY);
#endif
  Options opt;
  std::vector<const char*> files;
  for (int i = 2; i < argc; i++) {
    if (argv[i][0] == '-' && argv[i][1]) {
      for (const char* p = argv[i] + 1; *p; p++) {
        if (*p == 'v')
          opt.verbose = true;
        else if (*p == 's')
          opt.strict = true;
        else if (*p == 'p')
          opt.progress = true;
        else if (*p >= '1' && *p <= '9')
          opt.level = *p - '0';
        else if ((*p == 't' || *p == 'c') && (p[1] || i + 1 < argc)) {
          // -t N, -c N (or -tN, -cN): the rest of the argument, or the next
          const char* v = p[1] ? p + 1 : argv[++i];
          char* e;
          unsigned long n = strtoul(v, &e, 10);
          if (*p == 't') {
            if (*e || e == v || n < 1 || n > 64) {
              fprintf(stderr, "h264p: -t takes 1 to 64 threads\n");
              return 2;
            }
            opt.threads = (int)n;
          } else {
            unsigned long unit = 1 << 20;
            if (*e == 'k' || *e == 'K') unit = 1 << 10, e++;
            if (*e || e == v || n < 1 || n > (1024ul << 20) / unit) {
              fprintf(stderr, "h264p: -c takes a part size of 1 to 1024 MB, or in KB with a k\n");
              return 2;
            }
            opt.part = (uint32_t)(n * unit);
          }
          break;
        } else {
          usage();
          return 2;
        }
      }
    } else {
      files.push_back(argv[i]);
    }
  }
  if (files.size() != 2) {
    usage();
    return 2;
  }
  try {
    return argv[1][0] == 'c' ? code(files[0], files[1], opt) : decode(files[0], files[1], opt);
  } catch (const std::exception& e) {  // no memory
    fprintf(stderr, "h264p: %s\n", e.what());
    return 1;
  }
}
