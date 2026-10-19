// What the syntax walk exchanges the values of the syntax elements with.
//
// The walk (Syn for the headers, SliceCoder for the slice data) parses a
// bitstream or writes one. Parsing, it hands every value it reads to a
// channel; writing, it asks the channel for every value it writes. The
// channel is the TSV rows (tsv.inc), the value log of the compressor's dry
// run (log.inc) or the model of the compressor (model.inc).
//
// Each value method takes the value read when the walk parses (W false) and
// returns it; when the walk writes (W true), the argument is ignored and the
// method returns the value to write. A method is called exactly where the
// walk reaches the syntax element, in both directions, so the walk's state
// at the call is the same whether it parses or writes.

namespace h264p {

struct SliceCoder;

struct Chan {
  explicit Chan(bool w) : W(w) {}
  virtual ~Chan() {}
  bool W;  // the walk writes a bitstream: values come from the channel
  // (writing) the CABAC bins are not wanted, only the values: the walk
  // skips their arithmetic coding (the compressor's model pass at a level
  // that reads no CABAC states), and the bits it writes are not the
  // stream's
  bool nobins = false;

  // --- header syntax: groups of values under the name of a TSV row
  virtual void hbegin(const char* name) = 0;
  // the next value is a u(n): of n bits (before hval)
  virtual void hbits(int n) { (void)n; }
  virtual int64_t hval(int64_t v) = 0;
  // bytes; writing, b has the size wanted
  virtual void hbytes(std::vector<uint8_t>& b) = 0;
  virtual void hend() = 0;
  // whether more_rbsp_data(): parsing, m; writing, whether a row `next` follows
  virtual bool hmore(const char* next, bool m) = 0;

  // --- slice data
  virtual void slice_begin(SliceCoder& sc) { (void)sc; }
  virtual void slice_end() {}
  virtual int skip(int v) = 0;                 // the macroblock is skipped
  virtual int end(int v, bool certain) = 0;    // the slice ends after it (certain: at the end of the picture)
  virtual void mb_begin() = 0;                 // a coded macroblock
  virtual void mb_end() = 0;
  virtual int field(int v) = 0;                // mb_field_decoding_flag
  virtual int mb_type(int v) = 0;
  virtual int t8x8(int v) = 0;                 // transform_size_8x8_flag
  virtual int ipm(int blk, int v) = 0;         // -1 for prev_intra_pred_mode_flag, else rem_intra_pred_mode
  virtual int cpred(int v) = 0;                // intra_chroma_pred_mode
  virtual int sub(int i, int v) = 0;           // sub_mb_type
  virtual int ref(int list, int x, int y, int w, int h, int v) = 0;
  virtual int mvd(int list, int comp, int x, int y, int w, int h, int v) = 0;
  virtual int cbp(int v) = 0;
  virtual int qpd(int v) = 0;                  // mb_qp_delta
  // the coefficients of a block in scan order; coded: it is known to have a
  // nonzero one (CABAC 8x8 blocks without coded_block_flag)
  virtual void block(int cat, int idx, int* c, int n, bool coded) = 0;
  // I_PCM: the alignment bits (nb of them, XOR x264's padding bit) and the
  // samples; writing, a channel that knows nb gives it (the walk without
  // its bins cannot count them)
  virtual void pcm(int& align, int& nb, int* s, int n) = 0;

  // an inconsistency found while writing
  [[noreturn]] virtual void fail(const std::string& msg) = 0;
};

}  // namespace h264p
