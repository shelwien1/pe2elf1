/*  Lib3's coroutine and file API, and what this tree has to say to use them.

    Lib3/ is the framework oggcomp streams through: coro3b.inc's Coroutine,
    a stackful coroutine on setjmp/longjmp with an input pin and an output
    pin, and coro_fhp2.inc's CoroFileProc, which drives one over a pair of
    files -- 64 KB read into the input pin whenever the coroutine has
    emptied it, 64 KB written out whenever it has filled the output pin.
    The framework's files are as supplied, but for coro3b.inc, where the
    coroutine's stack size can be set from outside and yield refuses a
    call chain deeper than it can save.

    Lib3 spells its types and its hints as its own common.inc does; this
    tree has a common.inc of its own, so the names Lib3 uses are defined
    here over ours rather than including both.  The typedefs repeat
    oc_model.inc's word for word, which C++ allows.  */
typedef u8  byte;
typedef u16 word;
typedef u32 uint;
typedef u64 qword;

#ifndef ALIGN
#if defined(__GNUC__)
#define ALIGN(n) __attribute__((aligned(n)))
#else
#define ALIGN(n) __declspec(align(n))
#endif
#endif
#define if_e0(x) if (__builtin_expect((x), 0))
#define if_e1(x) if (__builtin_expect((x), 1))
#ifndef _MSC_VER
#define __assume(x) do { if (!(x)) __builtin_unreachable(); } while (0)
#endif
template <class T, int N> constexpr uint DIM(T (&)[N]) { return N; }
#if defined(__x86_64) || defined(_M_X64)
#define X64
#endif

/*  Lib3's stdio file API on every platform: main looks at ferror() through
    it, and the raw Win32 one it would otherwise pick has nothing to look
    at.  */
#define FILE_API_STD 1
#include "Lib3/file_api.inc"
#include "Lib3/coro3b.inc"
/*  CoroFileProc counts the input bytes it has handed over here, for a
    progress meter psrc has and this program does not.  */
static qword g_prog_in;
#include "Lib3/coro_fhp2.inc"

/*  The range coder's bytes -- rc.inc's RC_IO_BASE -- on the coroutine's
    pins: got from the input pin, put on the output pin.  Lib3's get()
    returns uint(-1) once the driver has said the input is done; as a byte
    that is 0xFF, forever, which is what the coder's flush counts on.  */
template <class RC>
struct rc_pin_io {
  Coroutine * co;
  byte get() { return (byte) co->pin[0].get(); }
  void put(byte c) { co->pin[1].put(c); }
};
