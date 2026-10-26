// mp3pack: a lossless compressor of MP3 files (c, d) -- its command line,
// as aacpack's. All the rest is mp3pack_src/mp3pack.inc.
#include "mp3pack_src/mp3pack.inc"

namespace mp3pack {

void usage() {
  fprintf(stderr,
          "usage: mp3pack c [options] input output outside\n"
          "       mp3pack d [options] input outside output\n"
          "  c  compress an MP3 file (MPEG-1, 2 and 2.5, layers I to III): its frames, as mp3rw parses\n"
          "     them, by mp3c's model into output; what is not frames (tags, other data, the bits\n"
          "     after a run of frames) into outside, as it is (mp3rw's .out)\n"
          "  d  decompress: input and outside back into the file, bit for bit\n"
          "options:\n"
          "  -v          print statistics\n"
          "  -p          the progress on stderr even when it is not a terminal (where it is, always)\n"
          "  -t N        (c) compress on N threads, 1 to 64 (default 1): the frames in parts, a model\n"
          "              for each; d takes N from the stream\n"
          "  -c N        (c with -t) the size of a part of the input, at least: N MB (1 to 1024), or\n"
          "              N KB with a k (default 1)\n"
          "input, output and outside can be - for stdin or stdout (one of each at most), and no two\n"
          "the same file\n");
}

}  // namespace mp3pack

int main(int argc, char** argv) {
  using namespace mp3pack;
  if (argc < 2 || !argv[1][0] || argv[1][1] || !strchr("cd", argv[1][0])) {
    usage();
    return 2;
  }
#ifdef _WIN32
  _setmode(_fileno(stdin), _O_BINARY);
  _setmode(_fileno(stdout), _O_BINARY);
#endif
  Options opt;
  opt.Init();
  const char* files[3];
  int nfiles = 0;
  for (int i = 2; i < argc; i++) {
    const char* arg = argv[i];
    if (arg[0] == '-' && arg[1]) {
      for (const char* p = arg + 1; *p; p++) {
        if (*p == 'v')
          opt.verbose = true;
        else if (*p == 'p')
          opt.progress = true;
        else if ((*p == 't' || *p == 'c') && (p[1] || i + 1 < argc)) {
          // -t N, -c N (or -tN, -cN): the rest of the argument, or the next
          const char* v = p[1] ? p + 1 : argv[++i];
          char* e;
          unsigned long n = strtoul(v, &e, 10);
          if (*p == 't') {
            if (*e || e == v || n < 1 || n > 64) {
              fprintf(stderr, "mp3pack: -t takes 1 to 64 threads\n");
              return 2;
            }
            opt.threads = (int)n;
          } else {
            unsigned long unit = 1 << 20;
            if (*e == 'k' || *e == 'K') unit = 1 << 10, e++;
            if (*e || e == v || n < 1 || n > (1024ul << 20) / unit) {
              fprintf(stderr, "mp3pack: -c takes a part size of 1 to 1024 MB, or in KB with a k\n");
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
      if (nfiles < 3) files[nfiles] = arg;
      nfiles++;
    }
  }
  if (nfiles != 3) {
    usage();
    return 2;
  }
  return argv[1][0] == 'c' ? code(files[0], files[1], files[2], opt) : decode(files[0], files[1], files[2], opt);
}
