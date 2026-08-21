// rc_io.inc -- the range coder's file ends -- what it writes before a stream and what
// it reads after one.  The coder itself is rc.inc.

uint32_t BMFState::rc_decode_flat(uint32_t tot) {
  uint32_t sym = rc.get_freq(tot);
  rc.decode(sym, sym+1, tot);
  return sym;
}

// Give back what `begin_plane_stream` took.  This is the model layer's, not
// the coder's -- `rc_end` was doing both jobs and only one of them is range
// coding.
// Nothing to give back: the table is `model_table_store`, and what this used to
// do was `free(model_tables)`.  The function stays because the two ends of a
// plane stream being named is what the comment above is about, and because
// `begin_plane_stream` still has work on the other side of the flag.
static void end_plane_stream() {
}

// The two ends differ in one call: the encoder flushes what it still holds and
// the decoder reads to the marker.  Everything after is the same tear-down.
template<int32_t f_DEC>
void rc_end() {
  if constexpr( f_DEC )
    bmf_cx.rc.finish();
  else
    bmf_cx.rc.flush();
  end_plane_stream();
}

void rc_end_decode() {
  rc_end<1>();
}

void rc_end_encode() {
  rc_end<0>();
}

// Back the output cursor off the packer's partial word.
//
// The packer writes 32 bits at a time and the coder starts at a byte, so
// whatever whole bytes the last word did not need are given back here, one at
// a time, and `stream.pk.free_bits` is left holding what is still owed.  It goes
// transiently negative on the way, which is the thing to know about it.
static void packer_rewind() {
  if( bmf_cx.stream.cur==(uint8_t*)bmf_cx.stream.pk.word )
    return;
  int32_t bits = bmf_cx.stream.pk.free_bits-8;
  if( bits<0 ) {
    bmf_cx.stream.pk.free_bits = bits;
    return;
  }
  // The guard above makes the first turn certain, so the `do` and the `while`
  // are the same walk; this one says its bound before it takes a step.
  uint8_t* at = bmf_cx.stream.cur;
  while( bits>=0 ) {
    --at;
    bits -= 8;
  }
  bmf_cx.stream.cur = at;
  bmf_cx.stream.pk.free_bits = bits;
}

// Everything an alternate model needs before its first symbol: the residual
// alphabet's two limits, the symbol tree's level geometry, and 1024 seeded
// frequency strips.  None of it is range coding -- `rc_begin` was doing this
// and the coder's own init in one function, which is why it was 90 lines.
static uint16_t* begin_plane_stream() {
  if( bmf_cx.plane_predictor==pred_p2 ) {
    bmf_cx.alt_freq_init = 8;
    bmf_cx.alt_freq_limit = 8;
  } else {
    bmf_cx.alt_freq_limit = 16;
    bmf_cx.alt_freq_init = 64;
  }
  // Six levels of a binary symbol tree, laid out end to end.  Level `lvl`
  // holds `2^(lvl-2)` pairs, starts at `first`, and its frequency table
  // starts at `first - lvl` -- except level 2, whose base is 0 rather than
  // -2, and levels 2 and 3, whose `first` the original wrote as constants
  // because the recurrence has not started yet.  `model_geometry[i]` is the
  // level that owns slot `i`.
  //
  // The decompilation wrote all six out, with `at4`..`at7` naming the running
  // offset at four of them and the other two folded into constants.
  bmf_cx.model_geometry[0] = 0;
  bmf_cx.model_geometry[1] = 1;
  bmf_cx.level_geom[2].half = 1;
  bmf_cx.level_geom[2].first = 2;
  bmf_cx.level_geom[2].tbl_base = 0;
  memset(&bmf_cx.model_geometry[2], 2, 2);
  int32_t slot = 4;
  for( int32_t lvl = 3; lvl<8; ++lvl ) {
    bmf_cx.level_geom[lvl].half = 1<<(lvl-2);
    bmf_cx.level_geom[lvl].first = (uint8_t)slot;
    bmf_cx.level_geom[lvl].tbl_base = (uint8_t)slot-lvl;
    memset(&bmf_cx.model_geometry[slot], lvl, 2*bmf_cx.level_geom[lvl].half);
    slot += 2*bmf_cx.level_geom[lvl].half;
  }
  // The model's frequency tables, one allocation.  `0x7F000` is 520192 bytes
  // and the shape behind it is `model_geometry`'s: it is a byte count in the
  // decompilation and stays one, because nothing here divides it by an
  // element size.
  uint16_t* tbl = bmf_cx.model_table_store;
  if( tbl ) {
    int32_t k = 0;
    uint16_t *row = tbl;
    // The ten numbers every row starts from.  They were ten stores in
    // scheduler order with `row[0]` last and spelled `*row`, one row at a
    // time, a thousand and twenty-four times; the values are unrelated to each
    // other, so this is a table and not a loop, and index order is the only
    // thing that lets it be read against the model it seeds.
    const uint16_t seed_row[10] = {
      635, (uint16_t)(24*bmf_cx.alt_freq_limit), 205, 124, 147, 83, 48, 16, 8, 4,
    };
    for( k = 0; k<0x400; ++k, row += 254 ) {
      memcpy(row, seed_row, sizeof seed_row);
      FreqPair* seed = (FreqPair*)&row[10];
      for( uint32_t i = 0; i<0x7A; ++i ) {
        seed[i].f[0] = 60;
        seed[i].f[1] = 36;
      }
    }
  }
  bmf_cx.model_tables = tbl;
  return tbl;
}

// The two heads of a stream, and the three steps are three functions now: give
// the packer's spare bytes back, set the model up if the header named one, and
// start the coder.
//
// Three things are genuinely one-sided and each is under `f_DEC`: the encoder
// flushes the packer's accumulator into the stream first, the encoder's counter
// runs forward and the decoder's back, and the coder is initialised for the
// direction it is about to run in.
//
// It used to return `tbl` *uninitialised* when `plane_alt_model` was clear,
// with a comment that the one caller reads it only under the same flag.  It
// returns null there instead: that is what the caller behaves as if it got,
// and it is not undefined.
template<int32_t f_DEC>
uint16_t*rc_begin() {
  if constexpr( !f_DEC )
    *bmf_cx.stream.pk.word = bmf_cx.stream.pk.acc;
  packer_rewind();
  uint16_t* tbl = bmf_cx.plane_alt_model ? begin_plane_stream() : nullptr;
  if constexpr( f_DEC )
    bmf_cx.rc.dec_init();
  else
    bmf_cx.rc.enc_init();
  return tbl;
}

uint16_t*rc_begin_encode() {
  return rc_begin<0>();
}

void rc_begin_decode() {
  rc_begin<1>();
}

static uint32_t packer_word() {
  if( bmf_cx.stream.cur+4>bmf_cx.stream.buf+bmf_cx.stream.size )
    bmf_fatal(bmf_read_error);
  uint32_t w = *(uint32_t*)bmf_cx.stream.cur;
  bmf_cx.stream.cur += 4;
  return w;
}

// Rewind the packer to the start of the coded buffer.  `search_filter` encodes
// the same tile once per candidate flag and measures the bytes each time, so it
// does this between every trial; seven sites wrote all five lines out, and the
// fifth -- `hist_scratch`, which is a *different* buffer's cursor that happens
// to live at the far end of this one -- is the one a reader has to remember is
// part of the reset.
static void packer_reset() {
  bmf_cx.stream.pk.free_bits = 0;
  bmf_cx.stream.pk.acc = 0;
  bmf_cx.stream.cur = bmf_cx.stream.buf;
  bmf_cx.stream.pk.word = (uint32_t*)bmf_cx.stream.buf;
  bmf_cx.hist_scratch = bmf_cx.stream.buf+bmf_cx.stream.size-4096;
}

// A coded buffer for an image of `data_size` bytes, with the packer rewound
// onto it.  128K of headroom past the pixels, which is what the three encoding
// entry points allocate: the size is written into `stream.size` first because
// `packer_reset` reads it back to place `hist_scratch` at the far end.
static void stream_open(uint32_t data_size) {
  bmf_cx.stream.size = data_size+0x20000;
  bmf_cx.stream.buf = (uint8_t*)bmf_new(bmf_cx.stream.size);
  packer_reset();
}

// The other half of `packer_reset`: put the accumulator's last, partial word
// where the next spill would have put it, so that `stream.cur - stream.buf` is
// the byte count of everything coded.  Eight sites wrote it out as
// `*(uint32_t*)stream.pk.word = stream.pk.acc;` -- seven of them in
// `search_filter`, which measures a trial encode and flushes after each -- and
// the cast is to the type `pk.word` already has, so it said nothing at all.
static void packer_flush() {
  *bmf_cx.stream.pk.word = bmf_cx.stream.pk.acc;
}

// The header's bit stream: the per-plane descriptors, a few bits at a time,
// through a 32-bit accumulator that spills to `stream.pk.word` when it fills.
//
// `compress_image` wrote this out six times and `expand_image` its mirror six
// times, at widths 4, 6 and 8.  What made the six copies look unlike each other
// was the count: it was kept in `bits_left`, `free_bits` or `stream.pk.free_bits`
// depending on the site, and every spill branch read the guard variable in one
// place and `stream.pk.free_bits` in another.  They agreed because the line before
// each block assigned one from the other; here there is one name and nothing to
// keep in step.
//
// The order below is the one thing to preserve when reading this against the
// original: the spill branch's `stream.pk.acc` is computed from the count *before*
// the count moves on.  Writing the count first would shift the carry-out by the
// width.
static void pack_bits(uint32_t word, int32_t n) {
  if( bmf_cx.stream.pk.free_bits<n ) {
    *bmf_cx.stream.pk.word = bmf_cx.stream.pk.acc|(2*(word<<((31-bmf_cx.stream.pk.free_bits)&31)));
    bmf_cx.stream.pk.word = (uint32_t*)bmf_cx.stream.cur;
    bmf_cx.stream.cur += 4;
    bmf_cx.stream.pk.acc = word>>(bmf_cx.stream.pk.free_bits&31);
    bmf_cx.stream.pk.free_bits += 32-n;
  } else {
    bmf_cx.stream.pk.acc |= word<<(-bmf_cx.stream.pk.free_bits&31);
    bmf_cx.stream.pk.free_bits -= n;
  }
}

static uint32_t unpack_bits(int32_t n) {
  const uint32_t mask = (1u<<n)-1;
  uint32_t v;
  bmf_cx.stream.pk.free_bits -= n;
  if( bmf_cx.stream.pk.free_bits<0 ) {
    const uint32_t w = packer_word();
    v = bmf_cx.stream.pk.acc|((w<<((bmf_cx.stream.pk.free_bits+n)&31))&mask);
    bmf_cx.stream.pk.acc = w>>(-bmf_cx.stream.pk.free_bits&31);
    bmf_cx.stream.pk.free_bits += 32;
  } else {
    v = bmf_cx.stream.pk.acc&mask;
    bmf_cx.stream.pk.acc = bmf_cx.stream.pk.acc>>n;
  }
  return v;
}
