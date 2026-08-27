
#pragma push_macro("setjmp")
#pragma push_macro("longjmp")
#undef setjmp
#undef longjmp
#define _JMP_BUF_DEFINED
#define _INC_SETJMP
#define _INC_SETJMPEX
#define setjmp setjmp1
#define longjmp longjmp1

//#include <intrin.h>
#include <immintrin.h>
//#include "my_intrin.h"

#undef setjmp
#undef longjmp
#pragma pop_macro("setjmp")
#pragma pop_macro("longjmp")

