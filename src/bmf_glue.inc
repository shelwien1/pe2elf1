// bmf_glue.inc -- the handful of symbols bmp.inc expects from its host
//
// bmp.inc is a fragment of the BMF codec: it is included into a translation unit
// that has already defined the allocator, the fatal-error path, and a few tuning
// constants.  This coder wants nothing else from BMF, so rather than drag the rest
// of that codec in, the fragment's environment is reproduced here.

#include <stddef.h>

enum : int32_t {
  bmf_no_open = 1,
  bmf_write_error = 2,
  bmf_no_memory = 3,
};

[[noreturn]] void bmf_fatal(int32_t code, const char* what = nullptr) {
  switch( code ) {
    case bmf_no_open:
      fprintf(stderr, "error: cannot open %s\n", what ? what : "(null)");
      break;
    case bmf_write_error:
      fprintf(stderr, "error: write failed\n");
      break;
    default:
      fprintf(stderr, "error: out of memory\n");
      break;
  }
  exit(2);
}

void*bmf_new(size_t n) {
  void* p = calloc(n ? n : 1, 1);
  if( !p )
    bmf_fatal(bmf_no_memory);
  return p;
}

int32_t min32(int32_t a, int32_t b) {
  return a<b ? a : b;
}

int32_t max32(int32_t a, int32_t b) {
  return a>b ? a : b;
}

// Thresholds for BMF's region-adaptive strip trial.  Nothing here calls
// strips_worth_trying, but the fragment has to compile.
enum : int32_t {
  CD_strip_min_h = 64,
  CD_strip_small = 16,
  CD_strip_ratio = 4,
  CD_strip_offset = 0,
};

// The tail a BMF member header carries after the fixed 16 bytes.  Only
// write_member_head touches it, and nothing here calls that either.
struct CodedTail {
  uint32_t len;
};
