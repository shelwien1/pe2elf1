// rc_io.inc -- the range coder's file ends -- what it writes before a stream and what
// it reads after one.  The coder itself is rc.inc.

uint32_t __rc_decode_flat(uint32_t tot) {
  uint32_t sym = rc.get_freq(tot);
  rc.decode(sym, sym+1, tot);
  return sym;
}

// The two ends differ in one call: the encoder flushes what it still holds and
// the decoder reads to the marker.  Everything after is the same tear-down.
template<int f_DEC>
void __rc_end() {
  if( f_DEC )
    rc.finish();
  else
    rc.flush();
  if( plane_alt_model )
    free(model_tables);
}

void __rc_end_decode() {
  __rc_end<1>();
}

void __rc_end_encode() {
  __rc_end<0>();
}

// The two heads of a stream.  Between the packer's last word and the coder's
// first byte the two directions do the same work: back the cursor off the
// partial word, and -- when the header names an alternate model -- lay out the
// level geometry and seed the 1024 frequency tables.  That middle is 60 lines
// and was written twice, once with `at`/`bits`/`tbl` and once with
// `cursor`/`bits_left`/`tbl0`, which is the decompiler's record of two register
// allocations rather than two intentions.
//
// Three things are genuinely one-sided and each is under `f_DEC`: the encoder
// flushes the packer's accumulator into the stream first, the encoder's counter
// runs forward and the decoder's back, and the coder is initialised for the
// direction it is about to run in.
//
// `tbl` is returned uninitialised when `plane_alt_model` is clear, which is
// what the encoder did before this and what its one caller relies on -- it
// reads the result only under the same flag.
template<int f_DEC>
uint16_t*__rc_begin() {
  uint16_t *tbl;
  uint32_t i;
  if( !f_DEC )
    *packer_word = packer_acc;
  uint8_t* at = out_cursor;
  if( out_cursor!=(uint8_t*)packer_word ) {
    int32_t bits = packer_free_bits-8;
    if( bits<0 ) {
      packer_free_bits = bits;
    } else {
      do {
        --at;
        bits -= 8;
      } while( bits>=0 );
      out_cursor = at;
      packer_free_bits = bits;
    }
  }
  if( plane_alt_model ) {
    if( plane_predictor==2 ) {
      alt_freq_init = 8;
      alt_freq_limit = 8;
    } else {
      alt_freq_limit = 16;
      alt_freq_init = 64;
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
    model_geometry[0] = 0;
    model_geometry[1] = 1;
    level_geom[2].half = 1;
    level_geom[2].first = 2;
    level_geom[2].tbl_base = 0;
    memset(&model_geometry[2], 2, 2);
    int32_t slot = 4;
    for( int32_t lvl = 3; lvl<8; ++lvl ) {
      level_geom[lvl].half = 1<<(lvl-2);
      level_geom[lvl].first = (uint8_t)slot;
      level_geom[lvl].tbl_base = (uint8_t)slot-lvl;
      memset(&model_geometry[slot], lvl, 2*level_geom[lvl].half);
      slot += 2*level_geom[lvl].half;
    }
    tbl = (uint16_t*)bmf_new(0x7F000u);
    if( tbl ) {
      int32_t k = 0;
      uint16_t *row = tbl;
      do {
        row[1] = 24*alt_freq_limit;
        row[2] = 205;
        row[6] = 48;
        row[3] = 124;
        row[7] = 16;
        row[4] = 147;
        row[5] = 83;
        row[8] = 8;
        row[9] = 4;
        *row = 635;
        FreqPair* seed = (FreqPair*)&row[10];
        for( i = 0; i<0x7A; ++i ) {
          seed[i].f[0] = 60;
          seed[i].f[1] = 36;
        }
        row += 254;
        ++k;
      } while( (uint32_t)k<0x400 );
    }
    model_tables = tbl;
  }
  if( f_DEC )
    rc.dec_init();
  else
    rc.enc_init();
  return tbl;
}

uint16_t*__rc_begin_encode() {
  return __rc_begin<0>();
}

// The flag has never been read.  It is the decompiler's name for an argument
// the donor pushed and the body ignored, and it is kept in the signature
// because the call sites pass it.
void __rc_begin_decode() {
  __rc_begin<1>();
}

static uint32_t __packer_word() {
  if( out_cursor+4>coded_buf+coded_size )
    bmf_fatal(bmf_read_error);
  uint32_t w = *(uint32_t*)out_cursor;
  out_cursor += 4;
  return w;
}

// Rewind the packer to the start of the coded buffer.  `search_filter` encodes
// the same tile once per candidate flag and measures the bytes each time, so it
// does this between every trial; seven sites wrote all five lines out, and the
// fifth -- `hist_scratch`, which is a *different* buffer's cursor that happens
// to live at the far end of this one -- is the one a reader has to remember is
// part of the reset.
static void packer_reset() {
  packer_free_bits = 0;
  packer_acc = 0;
  out_cursor = coded_buf;
  packer_word = (uint32_t*)coded_buf;
  hist_scratch = coded_buf+coded_size-4096;
}

// The header's bit stream: the per-plane descriptors, a few bits at a time,
// through a 32-bit accumulator that spills to `packer_word` when it fills.
//
// `compress_image` wrote this out six times and `expand_image` its mirror six
// times, at widths 4, 6 and 8.  What made the six copies look unlike each other
// was the count: it was kept in `bits_left`, `free_bits` or `packer_free_bits`
// depending on the site, and every spill branch read the guard variable in one
// place and `packer_free_bits` in another.  They agreed because the line before
// each block assigned one from the other; here there is one name and nothing to
// keep in step.
//
// The order below is the one thing to preserve when reading this against the
// original: the spill branch's `packer_acc` is computed from the count *before*
// the count moves on.  Writing the count first would shift the carry-out by the
// width.
static void pack_bits(uint32_t word, int32_t n) {
  if( packer_free_bits<n ) {
    *(uint32_t*)packer_word = packer_acc|(2*(word<<((31-packer_free_bits)&31)));
    packer_word = (uint32_t*)out_cursor;
    out_cursor += 4;
    packer_acc = word>>(packer_free_bits&31);
    packer_free_bits += 32-n;
  } else {
    packer_acc |= word<<(-packer_free_bits&31);
    packer_free_bits -= n;
  }
}

static uint32_t unpack_bits(int32_t n) {
  const uint32_t mask = (1u<<n)-1;
  uint32_t v;
  packer_free_bits -= n;
  if( packer_free_bits<0 ) {
    const uint32_t w = __packer_word();
    v = packer_acc|(w<<((packer_free_bits+n)&31))&mask;
    packer_acc = w>>(-packer_free_bits&31);
    packer_free_bits += 32;
  } else {
    v = packer_acc&mask;
    packer_acc = packer_acc>>n;
  }
  return v;
}
