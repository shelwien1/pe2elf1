// The same system headers coder0.cpp puts at the top of the program, so a
// tool can pull in tf/tf_all.inc the way coder0 does.  The transformer's own
// sources carry no #include <...> of their own.
#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <initializer_list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <immintrin.h>
#if defined(__linux__)
#include <sys/mman.h>
#endif
