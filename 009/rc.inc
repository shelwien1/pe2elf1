
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
    return out_cursor;
  }

  void set_p(uint8_t* q) {
    out_cursor = q;
  }

  uint8_t dec_get(uint8_t*&q) {
    if( q>=coded_buf+coded_size )
      __exit_402E40(4);
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

  void emit(uint32_t carry) {
    uint8_t* q = p();
    *q++ = (uint8_t)(cache+carry);
    for(; pending; --pending )
      *q++ = (uint8_t)(carry-1);
    set_p(q);
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
    uint32_t carry = (rounded>0xFF);
    uint8_t* q = p();
    *q++ = (uint8_t)(cache+carry);
    for(; pending; --pending )
      *q++ = (uint8_t)(carry-1);
    *q++ = (uint8_t)rounded;
    *q++ = (uint8_t)(len>>16);
    *q++ = (uint8_t)(len>>8);
    *q++ = (uint8_t)len;
    while( (uint32_t)(q-(uint8_t*)coded_buf)%4!=3 )
      *q++ = 0;
    *q++ = kMarker;
    set_p(q);
    packer_free_bits = 0;
    packer_acc = 0;
    packer_word = (uint32_t*)out_cursor;
  }

  void dec_init() {
    uint8_t* q = p();
    if( dec_get(q)!=kMarker )
      __exit_402E40(4);
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
    return (count>=tot) ? tot-1 : count;
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
    packer_word = (uint32_t*)out_cursor;
    packer_free_bits = 0;
    packer_acc = 0;
  }
};

// The one coder the program has.
static RangeCoder rc;
