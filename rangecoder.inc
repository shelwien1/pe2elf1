// rangecoder.inc -- the binary range coder: interval arithmetic,
// renormalisation and carry handling, plus the coded stream it writes into.
// Every model feeds this; BMFState derives from it, so any model holding a
// BMFState* calls these directly.
// Included from bmf.cpp; not a standalone translation unit.

struct RangeCoder {
  alignas(16) CodedStream stream;
  // Plot mode ('p', f_DEC==2): the cost in bits of everything coded since the
  // last sample boundary.  The models charge it to a component and clear it.
  // plot_on is false on every normal encode, so the two tests below are a
  // predictable not-taken branch and nothing else.
  double plot_bits;
  int32_t plot_on;
  static const uint32_t kTop = 0x00800000;
  static const uint32_t kPend = 0x7F800000;
  static const uint32_t kCarry = 0x80000000;
  static const uint32_t kMask = 0x7FFFFFFF;
  static const uint8_t kMarker = 0x97;

private:
  uint32_t range;
  uint32_t low;
  uint8_t cache;
  uint32_t pending;
  uint32_t rdiv;
  uint32_t bytes;
  uint8_t dec_get(uint8_t*&q) {
    if( q>=stream.buf+stream.size )
      bmf_fatal(bmf_read_error);
    return *q++;
  }

public:
  void enc_init() {
    range = 0x80000000;
    low = 0;
    pending = 0;
    bytes = 0;
    cache = kMarker;
  }

  // The one place coded bytes reach memory, and so the one place to check that
  // they still fit.  stream_open sizes the buffer at the raw data plus 128 KB,
  // which holds for any probability a working model produces -- but a swept
  // parameter need not produce one.  AP_p_ceil at its low extreme pins every
  // probability into [kProbFloor, kProbFloor+1], about 32/65536, so each coded
  // bit costs eleven and the stream runs an order of magnitude past the buffer;
  // IDX/sweep.py found this as a bus error on t24.  Bailing here rather than
  // clamping the probability wider covers every parameter that can inflate the
  // stream, not just this one, and bmf_fatal exits non-zero, which is what the
  // sweep reads as a refusal rather than a crash.
  uint8_t*emit_carry(uint32_t carry) {
    uint8_t* q = stream.cur;
    if( q+1+(size_t)pending>stream.buf+stream.size )
      bmf_fatal(bmf_write_error, "<coded stream: model expands past the buffer>");
    *q++ = (uint8_t)(cache+carry);
    for(; pending; --pending )
      *q++ = (uint8_t)(carry-1);
    return q;
  }

  void emit(uint32_t carry) {
    stream.cur = emit_carry(carry);
    cache = (uint8_t)(low>>23);
  }

  void enc_normalise() {
    while( range<=kTop ) {
      ++bytes;
      if( low<kPend )
        emit(0);
      else if( low&kCarry )
        emit(1);
      else
        ++pending;
      range <<= 8;
      low = (low<<8)&kMask;
    }
  }

  uint32_t encode(uint32_t cum, uint32_t high, uint32_t tot) {
    enc_normalise();
    if_e0( plot_on )
      plot_bits += log2((double)tot/(double)(high-cum));
    uint32_t r = range/tot;
    uint32_t below = r*cum;
    uint32_t width = r*(high-cum);
    range = (high<tot) ? width : range-below;
    low += below;
    return width;
  }

  uint32_t encode_bit(uint32_t f0, uint32_t f1, int32_t bit) {
    enc_normalise();
    if_e0( plot_on )
      plot_bits += log2((double)(f0+f1)/(double)(bit ? f1 : f0));
    uint32_t rt = f0*(range/(f0+f1));
    if( bit ) {
      low += rt;
      range -= rt;
    } else {
      range = rt;
    }
    return range;
  }

  void flush() {
    enc_normalise();
    bytes += 5;
    uint32_t len = bytes;
    uint32_t rounded = ((low&0x7FFFFF)>=((len&0xFFFFFF)>>1))+(low>>23);
    uint32_t carry = rounded>0xFF;
    uint8_t* q = emit_carry(carry);
    *q++ = (uint8_t)rounded;
    *q++ = (uint8_t)(len>>16);
    *q++ = (uint8_t)(len>>8);
    *q++ = (uint8_t)len;
    while( (uint32_t)(q-stream.buf)%4!=3 )
      *q++ = 0;
    *q++ = kMarker;
    stream.cur = q;
    stream.packer_reset();
  }

  void dec_init() {
    uint8_t* q = stream.cur;
    if( dec_get(q)!=kMarker )
      bmf_fatal(bmf_read_error);
    cache = dec_get(q);
    stream.cur = q;
    range = 128;
    low = cache>>1;
  }

  void dec_normalise() {
    uint8_t* q = stream.cur;
    while( range<=kTop ) {
      uint32_t head = (uint32_t)(uint8_t)(cache<<7)|(low<<8);
      cache = dec_get(q);
      low = (cache>>1)|head;
      range <<= 8;
    }
    stream.cur = q;
  }

  uint32_t get_freq(uint32_t tot) {
    dec_normalise();
    rdiv = range/tot;
    uint32_t count = low/rdiv;
    return umin32(count, tot-1);
  }

  void decode(uint32_t cum, uint32_t high, uint32_t tot) {
    uint32_t below = cum*rdiv;
    low -= below;
    range = (high<tot) ? (high-cum)*rdiv : range-below;
  }

  int32_t decode_bit(uint32_t f0, uint32_t f1) {
    dec_normalise();
    uint32_t rt = f0*(range/(f0+f1));
    if( low>=rt ) {
      range -= rt;
      low -= rt;
      return 1;
    }
    range = rt;
    return 0;
  }

  void finish() {
    dec_normalise();
    uint8_t* q = stream.cur;
    while( dec_get(q)!=kMarker ) {
    }
    stream.cur = q;
    stream.packer_reset();
  }

  // Uniform coding over [0, tot): every symbol one count wide.
  uint32_t decode_flat(uint32_t tot) {
    uint32_t sym = get_freq(tot);
    decode(sym, sym+1, tot);
    return sym;
  }

  template <int32_t f_DEC> uint32_t code_alphabet_size(uint32_t n, uint32_t cap) {
    if constexpr( f_DEC==1 ) return decode_flat(cap)+1;
    encode(n-1, n, cap);
    return n;
  }
};
