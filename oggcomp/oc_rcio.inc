//  Past the end of the input the pin hands back -1, which as a byte is
//  the 0xFF the encoder's flush left off; a few of those are the tail of
//  every stream.  Many more is a stream cut short, and the decoder would
//  otherwise go on turning them into output for as long as they came --
//  a cut in the middle of a run of bytes has no parser check to stop it.
static const char *oc_in;
static u32 rc_over;
constexpr u32 RC_OVER_MAX = 48;
template <class RC> struct rc_pin_io {
  Coroutine *co;
  byte get() {
    uint c = co->pin[0].get();
    if(UNLIKELY(c == uint(-1))) {
      FATAL_UNLESS(++rc_over <= RC_OVER_MAX, "%s: cut short -- the stream ends before its last symbol", oc_in);
      return 0xFF;
    }
    return (byte)c;
  }
  void put(byte c) { co->pin[1].put(c); }
};
#define RC_IO_BASE rc_pin_io
