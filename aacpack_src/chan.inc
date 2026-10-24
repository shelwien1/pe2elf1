// What the syntax walk exchanges the values of the syntax elements with
// (as h264p's chan.inc).
//
// The walk (Walk, syntax.inc and units.inc) parses a bitstream or writes
// one. Parsing (R), it hands every value it reads to a channel; writing
// (W), it asks the channel for every value it writes. The channel is the
// value log of the compressor's parse (log.inc), or the model (model.inc),
// which codes the values the log has, writing (compressing), and decodes
// them, writing too (decompressing): so both see the walk in the same state
// at every value.
//
// Each value method takes the value read when the walk parses and returns
// it; when the walk writes, the argument is ignored and the method returns
// the value to write, which the walk then checks (it has its range, a
// corrupt stream may give anything) and writes.

namespace aacpack {

struct Huff;
struct Walk;

// (A channel is made where its walk goes, and Init() of its kind sets it
// up: the vtable aside, it is plain data, and holds no memory of its own.)
struct Chan {
  bool W;  // the walk writes: values come from the channel

  // a unit is walked (begin), with the walk whose state the model reads
  virtual void begin(Walk& w) { (void)w; }
  virtual void end() {}
  // a field e of n bits (n 0: a count or length the bitstream has
  // otherwise), its index i in a run of e (a band, an envelope, ...)
  virtual int64_t val(int e, int n, int64_t v, int i) = 0;
  // a value of the Huffman codebook h
  virtual int hv(int e, const Huff& h, int v, int i) = 0;
  // spectral values of codebook cb (1-11): c[0..dim) at k, k + 1, ... of
  // window win of the walk's channel (Walk::cur)
  virtual void spec(int cb, int win, int k, int* c, int dim) = 0;
  // a string of nbits bits, in bytes, the first bit the first byte's top
  // (the last byte's bits past nbits zero)
  virtual void bits(int e, uint8_t* b, size_t nbits) = 0;
  // what the walk is at, for a model that follows it (Walk::Note)
  virtual void note(int what) { (void)what; }

  // parsing: where the log is, and the log cut back to it (a part that does
  // not parse is taken again otherwise)
  virtual size_t mark() { return 0; }
  virtual void rollback(size_t m) { (void)m; }
};

}  // namespace aacpack
