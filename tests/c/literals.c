/* Literals, strings, characters, numeric edge cases */
#include <limits.h>
#include <stdio.h>
#include <wchar.h>

static const char text[] = "tab\there\nquote\" backslash\\ "
                           "concatenated "
                           "across lines";
static const wchar_t wide[] = L"wide é";
static const char bytes[] = {'\x41', '\102', '\0', 'c', '\'', '\\'};

int main(void) {
  unsigned long long u = 18446744073709551615ULL;
  long long neg = -9223372036854775807LL - 1;
  int oct = 0755, hex = 0xDeadBeef & 0xFF, bin = 0b1011;
  double hf = 0x1.8p3, e = 1.5e-3, d = 6.02E+23;
  float f = 1.0f / 3;
  long double ld = 3.14159265358979323846L;
  char c = 'A' + 2;
  printf("%s\n", text);
  printf("%zu %d\n", sizeof wide / sizeof wide[0], (int)wide[5]);
  printf("%d %d %d %d %d %d\n", bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5]);
  printf("%llu %lld\n", u, neg);
  printf("%d %d %d\n", oct, hex, bin);
  printf("%g %g %g %.6f %.10Lf\n", hf, e, d, f, ld);
  printf("%c %d %d\n", c, INT_MAX, INT_MIN);
  printf("%d %d\n", -1 >> 1, (unsigned)-1 > 0);
  return 0;
}
