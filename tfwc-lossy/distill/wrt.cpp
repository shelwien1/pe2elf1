// wrt: cmix dictionary transform + the fx2 byte permutation (book1wrt's form)
//   wrt english.dic in.txt out.wrt
#include <cstdio>
#include <cstdlib>
#include <string>
#include "dictionary.h"
static int perm(int c) {
  if (c >= '{' && c < 127) c += 'P' - '{';
  else if (c >= 'P' && c < 'T') c -= 'P' - '{';
  else if ((c >= ':' && c <= '?') || (c >= 'J' && c <= 'O')) c ^= 0x70;
  if (c == 'X' || c == '`') c ^= 'X' ^ '`';
  return c;
}
int main(int argc, char** argv) {
  FILE* d = fopen(argv[1], "rb"); FILE* in = fopen(argv[2], "rb"); FILE* tmp = tmpfile();
  fseek(in, 0, SEEK_END); int len = ftell(in); fseek(in, 0, SEEK_SET);
  preprocessor::Dictionary dict(d, true, false);
  dict.Encode(in, len, tmp);
  rewind(tmp);
  FILE* out = fopen(argv[3], "wb");
  int c;
  while ((c = getc(tmp)) != EOF) putc(perm(c), out);
  fclose(out);
  return 0;
}
