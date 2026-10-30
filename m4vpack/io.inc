// io.inc -- the coroutines' input and output as runs of bytes (the chunk reader on them: reader.inc).

//--- streams: coroutines -----------------------------------------------------------------------

// m4vpack's compressor and decompressor are coroutines of Lib3 (Lib3/coro3b.inc), in its buffer
// protocol: the input comes in the buffers addinp() gives, a 0-length one with f_quit set at its
// end, and yield Y_IN asks for the next; the output goes into the buffer addout() gave, yield
// Y_OUT handing it over full; Y_END ends a run (its last output in the buffer), Y_FAIL after an
// error (the run's note says it). Y_PART is MTwrap's: a worker's part is done (threads, below).
// Y_VIDEO tells the driver what the video is (the run's video: the progress line's, progress.inc),
// after which it goes on.
enum { Y_END = 0, Y_IN = 1, Y_OUT = 2, Y_FAIL = 3, Y_PART = 4, Y_VIDEO = 5 };

constexpr int NOTE_MAX = 256;  // an error's note (a line)
constexpr U64 OVER_MAX = 16;   // bytes the range decoder may ask for past the input's end; more: damaged data

#include "coio.inc"  // CoIo: a coroutine's input and output
