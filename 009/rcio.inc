// rcio.inc -- the range coder's file ends -- what it writes before a stream and what
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
  uint16_t* tbl, * row;
  FreqPair* seed;
  int32_t at4, at5, k;
  int32_t bits;
  uint8_t* at;
  int32_t at6, at7;
  uint32_t i;
  if( !f_DEC )
    *packer_word = packer_acc;
  at = out_cursor;
  if( out_cursor!=(uint8_t*)packer_word ) {
    bits = packer_free_bits-8;
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
    level_geom[2].first = 2;
    model_geometry[0] = 0;
    model_geometry[1] = 1;
    model_geometry[2] = 2;
    model_geometry[3] = 2;
    level_geom[2].tbl_base = 0;
    level_geom[2].half = 1;
    level_geom[3].half = 2;
    level_geom[3].first = 4;
    level_geom[3].tbl_base = 1;
    memset(&model_geometry[4], 3, 4);
    level_geom[4].half = 4;
    at4 = 2*level_geom[3].half+4;
    level_geom[4].first = 2*level_geom[3].half+4;
    level_geom[4].tbl_base = 2*level_geom[3].half;
    memset(&model_geometry[at4], 4, 8);
    level_geom[5].half = 8;
    at5 = at4+2*level_geom[4].half;
    level_geom[5].first = at5;
    level_geom[5].tbl_base = at5-5;
    memset(&model_geometry[at5], 0x05, 16);
    level_geom[6].half = 16;
    at6 = at5+2*level_geom[5].half;
    level_geom[6].first = (uint8_t)at6;
    level_geom[6].tbl_base = (uint8_t)at6-6;
    memset(&model_geometry[at6], 0x06, 32);
    level_geom[7].half = 32;
    at7 = at6+2*level_geom[6].half;
    level_geom[7].first = (uint8_t)at7;
    level_geom[7].tbl_base = (uint8_t)at7-7;
    memset(&model_geometry[at7], 0x07, 64);
    tbl = (uint16_t*)bmf_new(0x7F000u);
    if( tbl ) {
      k = 0;
      row = tbl;
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
        seed = (FreqPair*)&row[10];
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
void __rc_begin_decode(int8_t unread_flag) {
  __rc_begin<1>();
}

static uint32_t __packer_word() {
  if( out_cursor+4>coded_buf+coded_size )
    __exit_402E40(4);
  uint32_t w = *(uint32_t*)out_cursor;
  out_cursor += 4;
  return w;
}
