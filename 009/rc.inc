// rc.inc -- the arithmetic coder, and nothing else.  Its file ends are rc_io.inc.
//
// A carry-propagating range coder: `low` and `range` narrow as symbols arrive,
// and a byte leaves whenever `range` falls below `kTop`.  The four constants
// were four magic words in the decompilation -- 0x800000, 0x7F800000,
// 0x80000000, 0x7FFFFFFF -- and the fifth, `kMarker = 0x97`, is the byte the
// original seeds `cache` with, which is why a stream's first byte is not part
// of the coding.
//
// The private section is private for a reason: `p()` and `set_p()` wrap
// `stream.cur`, the one global this shares with the bit packer in rc_io.inc,
// and `dec_get` is the only bounds check between a truncated file and a read
// past the end of the buffer.
struct RangeCoder {
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
  uint8_t*p() {
    return stream.cur;
  }

  void set_p(uint8_t* q) {
    stream.cur = q;
  }

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

  // The cached byte with the carry folded into it, and then the run of pending
  // bytes that carry resolves.  Both writers open with these four lines, and
  // they answer the cursor rather than storing it because only one of them is
  // finished with it: `emit` moves on to the next cache byte, `flush` to the
  // length and the tail.
  uint8_t* emit_carry(uint32_t carry) {
    uint8_t* q = p();
    *q++ = (uint8_t)(cache+carry);
    for(; pending; --pending )
      *q++ = (uint8_t)(carry-1);
    return q;
  }

  void emit(uint32_t carry) {
    set_p(emit_carry(carry));
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
    uint32_t r = range/tot;
    uint32_t below = r*cum;
    uint32_t width = r*(high-cum);
    range = (high<tot) ? width : range-below;
    low += below;
    return width;
  }

  uint32_t encode_bit(uint32_t f0, uint32_t f1, int32_t bit) {
    enc_normalise();
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
    while( (uint32_t)(q-(uint8_t*)stream.buf)%4!=3 )
      *q++ = 0;
    *q++ = kMarker;
    set_p(q);
    stream.pk.free_bits = 0;
    stream.pk.acc = 0;
    stream.pk.word = (uint32_t*)stream.cur;
  }

  void dec_init() {
    uint8_t* q = p();
    if( dec_get(q)!=kMarker )
      bmf_fatal(bmf_read_error);
    cache = dec_get(q);
    set_p(q);
    range = 128;
    low = cache>>1;
  }

  void dec_normalise() {
    uint8_t* q = p();
    while( range<=kTop ) {
      uint32_t head = (uint32_t)(uint8_t)(cache<<7)|(low<<8);
      cache = dec_get(q);
      low = (cache>>1)|head;
      range <<= 8;
    }
    set_p(q);
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
    uint8_t* q = p();
    while( dec_get(q)!=kMarker ) {}
    set_p(q);
    stream.pk.word = (uint32_t*)stream.cur;
    stream.pk.free_bits = 0;
    stream.pk.acc = 0;
  }
};

// The one coder the program has.
static RangeCoder rc;
