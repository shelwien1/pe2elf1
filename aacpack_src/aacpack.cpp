// aacpack: a lossless compressor of AAC streams (c, d) -- its command line.
// All the rest is aacpack.inc.
#include "aacpack.inc"

namespace aacpack {

void usage() {
  char lv[64];
  int k = 0;
  for (int n : kLevels) k += snprintf(lv + k, sizeof lv - k, " -%d", n);
  fprintf(stderr,
          "usage: aacpack c|d [options] input output\n"
          "  c  compress an AAC stream or file: ADTS, LOAS/LATM, ADIF, MP4 (M4A, MOV) or AUS1 (found\n"
          "     as they come), or raw access units (-asc); what is not AAC (tags, junk, video) is\n"
          "     kept as it is\n"
          "  d  decompress it, bit for bit\n"
          "options:\n"
          "  -v          print statistics, and what the stream is (and where it changes)\n"
          "  -p          what the stream is and the progress on stderr even when it is not a terminal\n"
          "              (where it is, always)\n"
          "  -s          strict (c): fail where a frame would be stored as its bytes or partly as bits\n"
          " %s         (c) the model's level\n"
          "  -adts | -latm | -adif   (c) look for this container alone\n"
          "  -asc HEX    (c) the input is back-to-back raw_data_blocks of this AudioSpecificConfig\n"
          "input and output can be - for stdin and stdout, and not the same file\n",
          lv);
}

// a string of hex digits (0x first, or not) as bytes: their count, 0 bad
size_t parse_hex(const char* s, uint8_t* out, size_t cap) {
  if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
  size_t n = 0;
  auto hexval = [](int c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1; };
  while (s[0]) {
    int hi = hexval((unsigned char)s[0]), lo = s[1] ? hexval((unsigned char)s[1]) : -1;
    if (hi < 0 || lo < 0 || n >= cap) return 0;
    out[n++] = (uint8_t)(hi << 4 | lo);
    s += 2;
  }
  return n;
}

}  // namespace aacpack

int main(int argc, char** argv) {
  using namespace aacpack;
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
  const char* files[2];
  int nfiles = 0;
  for (int i = 2; i < argc; i++) {
    const char* arg = argv[i];
    if (arg[0] == '-' && arg[1]) {
      if (!strcmp(arg, "-adts")) {
        opt.format = F_ADTS;
      } else if (!strcmp(arg, "-latm") || !strcmp(arg, "-loas")) {
        opt.format = F_LOAS;
      } else if (!strcmp(arg, "-adif")) {
        opt.format = F_ADIF;
      } else if (!strcmp(arg, "-asc")) {
        if (i + 1 >= argc || !(opt.asc_n = parse_hex(argv[++i], opt.asc, sizeof opt.asc))) {
          fprintf(stderr, "aacpack: -asc takes an AudioSpecificConfig as hex bytes\n");
          return 2;
        }
        AscInfo ai;
        uint8_t pad[sizeof opt.asc + 16];
        memset(pad, 0, sizeof pad);
        memcpy(pad, opt.asc, opt.asc_n);
        const char* why = !asc_parses(pad, 0, opt.asc_n * 8, false, ai) ? "it does not parse, or is of a kind not known"
                          : ai.er ? "ER access units have no END, so they cannot be told apart back to back (AUS1 can hold them)"
                          : !ai.raw_blocks ? "its access units are not parsed (object type, or sampling rate)"
                                           : nullptr;
        if (why) {
          fprintf(stderr, "aacpack: -asc %s: %s\n", argv[i], why);
          return 2;
        }
      } else {
        for (const char* p = arg + 1; *p; p++) {
          if (*p == 'v')
            opt.verbose = true;
          else if (*p == 's')
            opt.strict = true;
          else if (*p == 'p')
            opt.progress = true;
          else if (*p >= '1' && *p <= '9')
            opt.level = *p - '0';
          else {
            usage();
            return 2;
          }
        }
      }
    } else {
      if (nfiles < 2) files[nfiles] = arg;
      nfiles++;
    }
  }
  if (nfiles != 2) {
    usage();
    return 2;
  }
  return argv[1][0] == 'c' ? code(files[0], files[1], opt) : decode(files[0], files[1], opt);
}
