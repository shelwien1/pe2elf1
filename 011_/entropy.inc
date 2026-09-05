
//=============================================================================
// Entropy-coded scan parsing: Huffman and arithmetic.
//
// Ported from the libjpeg-derived 002_djpeg tree -- huff_bit_buffer.inc,
// huff_macro.inc, huff_decode.inc, huff_make_d_derived_tbl.inc,
// huff_start_pass.inc, huff_decode_mcu{,_DC,_DCr,_AC,_ACr}.inc, arith.inc,
// arith_tab.inc, decodeMCU_ari.inc and per_scan_setup.inc.
//
// Two deliberate differences, both because pjpg parses rather than decodes:
//
//  * djpeg reads the whole file into memory and the bit reader walks
//    next_input_byte.  pjpg is a single forward pass, so fill_bit_buffer() and
//    the arithmetic decoder's byte fetch pull from the coroutine input instead,
//    doing the FF00 unstuffing and marker detection inline.
//
//  * There is no coefficient array.  Nothing is reconstructed, so coefficient
//    values are discarded -- except that a progressive refinement scan's
//    bitstream shape depends on WHICH coefficients are already nonzero.  That
//    one bit per coefficient is all this keeps: 8 bytes per block instead of
//    the 128 a JBLOCK costs.  See nz_get/nz_set.
//=============================================================================

enum { HUFF_LOOKAHEAD = 8 };

static constexpr int e_bmask[17] = {
  0x0000,0x0001,0x0003,0x0007,0x000F,0x001F,0x003F,0x007F,
  0x00FF,0x01FF,0x03FF,0x07FF,0x0FFF,0x1FFF,0x3FFF,0x7FFF,0xFFFF
};

// libjpeg's d_derived_tbl
struct d_tbl {
  int  maxcode[18];
  int  valoffset[17];
  JHUFF_TBL* pub;
  byte look_nbits[1<<HUFF_LOOKAHEAD];
  byte look_sym[1<<HUFF_LOOKAHEAD];
};

d_tbl dtbl[2][4];          // [0]=DC, [1]=AC
byte  dtbl_ok[2][4];

// ---- bit reader -------------------------------------------------------------

uint e_buf;                // bit buffer, bits held in the low e_bits
int  e_bits;
uint e_marker;             // marker that ended the entropy segment (0 = none)
uint e_eof;
uint e_short;              // ran out of data inside the segment
uint e_bytes;              // entropy bytes consumed, for reporting

void bits_init( void ) { e_buf=0; e_bits=0; e_marker=0; e_eof=0; e_short=0; }

// libjpeg's jpeg_fill_bit_buffer, reading through the coroutine rather than a
// memory buffer.  FF 00 is a stuffed FF, a run of FF is fill, and FF followed
// by anything else is the marker that ends this entropy segment.
uint fill_bit_buffer( int nbits ) {
  uint c;
  if( e_marker==0 && e_eof==0 ) {
    while( e_bits < (32-7) ) {
      c = get();
      if( c==uint(-1) ) { e_eof=1; break; }
      e_bytes++;
      if( c==0xFF ) {
        do {
          c = get();
          if( c==uint(-1) ) { e_eof=1; break; }
          e_bytes++;
        } while( c==0xFF );
        if( e_eof ) break;
        if( c==0 ) c = 0xFF;
        else { e_marker = c; break; }     // a real marker: no more bytes
      }
      e_buf = (e_buf<<8) | c;
      e_bits += 8;
    }
  }
  if( (e_marker || e_eof) && (nbits > e_bits) ) {
    // libjpeg pads with zeroes and carries on so the rest of the scan still
    // walks; pjpg records it so the scan can be reported as short.
    e_short = 1;
    e_buf <<= (32-7) - e_bits;
    e_bits = 32-7;
  }
  return 1;
}

uint get_bits( int n ) {
  if( (n<=0) || (n>16) ) return 0;          // no valid JPEG field is wider
  if( e_bits < n ) fill_bit_buffer( n );
  e_bits -= n;
  return (e_buf >> e_bits) & uint(e_bmask[n]);
}
uint peek_bits( int n ) {
  if( (n<=0) || (n>16) ) return 0;
  if( e_bits < n ) fill_bit_buffer( n );
  return (e_buf >> (e_bits-n)) & uint(e_bmask[n]);
}
void drop_bits( int n ) { e_bits -= n; }

// libjpeg's jpeg_huff_decode: the slow path for codes longer than the lookahead
int huff_decode_slow( d_tbl& t, int min_bits ) {
  int l,code;
  l = min_bits;
  code = int( get_bits(l) );
  while( code > t.maxcode[l] ) {
    code <<= 1;
    code |= int( get_bits(1) );
    l++;
    if( l>17 ) break;
  }
  if( l>16 ) return 0;                 // corrupt: libjpeg fakes a zero
  return t.pub->huffval[ code + t.valoffset[l] ];
}

// libjpeg's HUFF_DECODE macro
int huff_decode( d_tbl& t ) {
  int nb,look;
  if( e_bits < HUFF_LOOKAHEAD ) {
    fill_bit_buffer( 0 );
    if( e_bits < HUFF_LOOKAHEAD ) return huff_decode_slow( t, 1 );
  }
  look = int( peek_bits(HUFF_LOOKAHEAD) );
  nb = t.look_nbits[look];
  if( nb!=0 ) { drop_bits(nb); return t.look_sym[look]; }
  return huff_decode_slow( t, HUFF_LOOKAHEAD+1 );
}

#define HUFF_EXTEND(x,s) ( ((x) <= e_bmask[(s)-1]) ? ((x) - e_bmask[s]) : (x) )

// ---- derived tables ---------------------------------------------------------

// libjpeg's jpeg_make_d_derived_tbl.  Returns 0 on a table pjpg has already
// rejected or that is internally inconsistent, rather than exit()ing.
uint make_derived( uint isDC, uint tblno ) {
  JHUFF_TBL* htbl;
  d_tbl* t;
  int p,i,l,si,lookbits,ctr;
  uint code;
  char  huffsize[257];
  uint  huffcode[257];

  if( tblno>=4 ) return 0;
  htbl = isDC ? &dc_huff_tbl_ptrs[tblno] : &ac_huff_tbl_ptrs[tblno];
  if( htbl->sent_table==0 ) return 0;          // never defined by a DHT
  t = &dtbl[isDC?0:1][tblno];
  t->pub = htbl;

  for( l=1,p=0; l<=16; l++ ) {
    i = htbl->bits[l];
    if( (i<0) || (p+i>256) ) return 0;
    while( i-- ) huffsize[p++] = char(l);
  }
  huffsize[p] = 0;

  code = 0; si = huffsize[0]; p = 0;
  while( huffsize[p] ) {
    while( huffsize[p]==si ) { huffcode[p++] = code; code++; }
    if( code >= (1u<<si) ) return 0;
    code <<= 1; si++;
  }

  p = 0;
  for( l=1; l<=16; l++ ) {
    if( htbl->bits[l] ) {
      t->valoffset[l] = int(p) - int(huffcode[p]);
      p += htbl->bits[l];
      t->maxcode[l] = int(huffcode[p-1]);
    } else t->maxcode[l] = -1;
  }
  t->maxcode[17] = 0xFFFFF;

  bzero( t->look_nbits );
  p = 0;
  for( l=1; l<=HUFF_LOOKAHEAD; l++ ) {
    for( i=1; i<=htbl->bits[l]; i++,p++ ) {
      lookbits = int( huffcode[p] << (HUFF_LOOKAHEAD-l) );
      for( ctr=1<<(HUFF_LOOKAHEAD-l); ctr>0; ctr-- ) {
        t->look_nbits[lookbits] = byte(l);
        t->look_sym[lookbits] = htbl->huffval[p];
        lookbits++;
      }
    }
  }
  // libjpeg's final check: a DC table may only carry symbols 0..15, since the
  // symbol IS the magnitude category.  Without this a corrupt table yields
  // get_bits(200) and the bit counter leaves any sane range.
  if( isDC ) {
    int k, nsym = 0;
    for( l=1; l<=16; l++ ) nsym += htbl->bits[l];
    for( k=0; k<nsym; k++ ) if( htbl->huffval[k] > 15 ) return 0;
  }

  dtbl_ok[isDC?0:1][tblno] = 1;
  return 1;
}

// ---- per-coefficient nonzero bitmap ----------------------------------------
// One qword per block, bit k set once the coefficient at zigzag index k is
// nonzero.  This is the whole of what a progressive refinement scan needs to
// know about the scans before it.  Indexing by the zigzag index k rather than
// jpeg_natural_order[k] is a consistent relabelling: every access goes through
// the same permutation.

qword* nzmap[4];
uint   nz_blocks[4];
uint   nz_on;

void nz_free( void ) {
  uint c;
  for( c=0; c<4; c++ ) { if( nzmap[c] ) free(nzmap[c]); nzmap[c]=0; nz_blocks[c]=0; }
  nz_on = 0;
}

enum { NZ_MAX_BYTES = 1u<<28 };     // 256 MB ceiling for the whole image

uint nz_alloc( void ) {
  uint c; qword need;
  nz_free();
  need = 0;
  for( c=0; c<uint(num_components); c++ ) need += qword(comp_info[c].blocks_total)*8;
  if( (need==0) || (need>NZ_MAX_BYTES) ) return 0;
  for( c=0; c<uint(num_components); c++ ) {
    nz_blocks[c] = comp_info[c].blocks_total;
    nzmap[c] = (qword*)calloc( nz_blocks[c]?nz_blocks[c]:1, 8 );
    if( nzmap[c]==0 ) { nz_free(); return 0; }
  }
  nz_on = 1;
  return 1;
}

uint  nz_ci;        // component index (into comp_info) of the current block
uint  nz_pos;       // block index within that component
qword nz_cur;       // its bits, loaded before a refinement block

void nz_load( void ) {
  nz_cur = ( nz_on && (nz_ci<4) && nzmap[nz_ci] && (nz_pos<nz_blocks[nz_ci]) )
           ? nzmap[nz_ci][nz_pos] : 0;
}
void nz_store( void ) {
  if( nz_on && (nz_ci<4) && nzmap[nz_ci] && (nz_pos<nz_blocks[nz_ci]) )
    nzmap[nz_ci][nz_pos] = nz_cur;
}

// ---- scan state -------------------------------------------------------------

int  last_dc_val[4];
uint EOBRUN;
uint restarts_to_go;
uint e_blocks;              // blocks decoded in this scan
uint e_mcus;                // MCUs decoded in this scan
uint n_rst_seen;

d_tbl* dc_cur[10];          // per MCU block, as libjpeg's dc_cur_tbls
d_tbl* ac_cur[10];

// ---- Huffman MCU decoders ---------------------------------------------------
// Each returns 0 only when the scan cannot continue.

// libjpeg's decode_mcu (baseline / extended sequential)
uint dec_mcu_seq( void ) {
  uint blkn;
  int  s,k,r;
  for( blkn=0; blkn<uint(blocks_in_MCU); blkn++ ) {
    uint ci = uint(MCU_membership[blkn]);
    s = huff_decode( *dc_cur[blkn] );
    if( s ) { r = int(get_bits(s)); s = HUFF_EXTEND(r,s); }
    s += last_dc_val[ci];
    last_dc_val[ci] = s;
    for( k=1; k<64; k++ ) {
      s = huff_decode( *ac_cur[blkn] );
      r = s>>4; s &= 15;
      if( s ) { k += r; (void)get_bits(s); }
      else { if( r!=15 ) break; k += 15; }
    }
    e_blocks++;
  }
  return 1;
}

// libjpeg's decode_mcu_DC_first
uint dec_mcu_dc_first( void ) {
  uint blkn;
  int  s,r;
  for( blkn=0; blkn<uint(blocks_in_MCU); blkn++ ) {
    uint ci = uint(MCU_membership[blkn]);
    s = huff_decode( *dc_cur[blkn] );
    if( s ) { r = int(get_bits(s)); s = HUFF_EXTEND(r,s); }
    s += last_dc_val[ci];
    last_dc_val[ci] = s;
    e_blocks++;
  }
  return 1;
}

// libjpeg's decode_mcu_DC_refine: one bit per block, unconditionally
uint dec_mcu_dc_refine( void ) {
  uint blkn;
  for( blkn=0; blkn<uint(blocks_in_MCU); blkn++ ) { (void)get_bits(1); e_blocks++; }
  return 1;
}

// libjpeg's decode_mcu_AC_first.  Always one block per MCU.
uint dec_mcu_ac_first( void ) {
  int s,k,r;
  if( EOBRUN>0 ) { EOBRUN--; e_blocks++; return 1; }
  nz_load();
  for( k=int(Ss); k<=int(Se); k++ ) {
    s = huff_decode( *ac_cur[0] );
    r = s>>4; s &= 15;
    if( s ) {
      k += r;
      (void)get_bits(s);
      if( (k>=0) && (k<64) ) nz_cur |= qword(1)<<k;   // now nonzero
    } else {
      if( r==15 ) { k += 15; }
      else {
        EOBRUN = 1u<<r;
        if( r ) EOBRUN += get_bits(r);
        EOBRUN--;
        break;
      }
    }
  }
  nz_store();
  e_blocks++;
  return 1;
}

// libjpeg's decode_mcu_AC_refine.  The bitstream shape depends only on which
// coefficients are already nonzero, which is exactly what nz_cur holds.
uint dec_mcu_ac_refine( void ) {
  int s,k,r;
  nz_load();
  k = int(Ss);
  if( EOBRUN==0 ) {
    for( ; k<=int(Se); k++ ) {
      s = huff_decode( *ac_cur[0] );
      r = s>>4; s &= 15;
      if( s ) {
        (void)get_bits(1);              // sign of the newly nonzero coefficient
        s = 1;
      } else {
        if( r!=15 ) {
          EOBRUN = 1u<<r;
          if( r ) EOBRUN += get_bits(r);
          break;                        // the EOB logic below finishes the block
        }
      }
      // advance over already-nonzero coefficients (each takes a correction
      // bit) and r still-zero ones
      while( k<=int(Se) ) {
        if( (nz_cur>>k) & 1 ) {
          (void)get_bits(1);            // correction bit
        } else {
          if( r==0 ) break;
          r--;
        }
        k++;
      }
      // libjpeg lands an overrun on jpeg_natural_order[64..79], all of which
      // are 63; under this relabelling that is coefficient 63.
      if( s ) nz_cur |= qword(1) << ((k>=0 && k<64) ? k : 63);
    }
  }
  if( EOBRUN>0 ) {
    for( ; k<=int(Se); k++ ) if( (nz_cur>>k)&1 ) (void)get_bits(1);
    EOBRUN--;
  }
  nz_store();
  e_blocks++;
  return 1;
}

// The QM-coder probability estimation table, verbatim from djpeg's
// arith_tab.inc (JPEG T.81 Table D.3): Qe, next-MPS, switch, next-LPS packed.
static constexpr int jaritab[113] = {
   (((int)0x5a1d << 16) | ((int)1 << 8) | ((int)1 << 7) | 1),
   (((int)0x2586 << 16) | ((int)2 << 8) | ((int)0 << 7) | 14),
   (((int)0x1114 << 16) | ((int)3 << 8) | ((int)0 << 7) | 16),
   (((int)0x080b << 16) | ((int)4 << 8) | ((int)0 << 7) | 18),
   (((int)0x03d8 << 16) | ((int)5 << 8) | ((int)0 << 7) | 20),
   (((int)0x01da << 16) | ((int)6 << 8) | ((int)0 << 7) | 23),
   (((int)0x00e5 << 16) | ((int)7 << 8) | ((int)0 << 7) | 25),
   (((int)0x006f << 16) | ((int)8 << 8) | ((int)0 << 7) | 28),
   (((int)0x0036 << 16) | ((int)9 << 8) | ((int)0 << 7) | 30),
   (((int)0x001a << 16) | ((int)10 << 8) | ((int)0 << 7) | 33),
   (((int)0x000d << 16) | ((int)11 << 8) | ((int)0 << 7) | 35),
   (((int)0x0006 << 16) | ((int)12 << 8) | ((int)0 << 7) | 9),
   (((int)0x0003 << 16) | ((int)13 << 8) | ((int)0 << 7) | 10),
   (((int)0x0001 << 16) | ((int)13 << 8) | ((int)0 << 7) | 12),
   (((int)0x5a7f << 16) | ((int)15 << 8) | ((int)1 << 7) | 15),
   (((int)0x3f25 << 16) | ((int)16 << 8) | ((int)0 << 7) | 36),
   (((int)0x2cf2 << 16) | ((int)17 << 8) | ((int)0 << 7) | 38),
   (((int)0x207c << 16) | ((int)18 << 8) | ((int)0 << 7) | 39),
   (((int)0x17b9 << 16) | ((int)19 << 8) | ((int)0 << 7) | 40),
   (((int)0x1182 << 16) | ((int)20 << 8) | ((int)0 << 7) | 42),
   (((int)0x0cef << 16) | ((int)21 << 8) | ((int)0 << 7) | 43),
   (((int)0x09a1 << 16) | ((int)22 << 8) | ((int)0 << 7) | 45),
   (((int)0x072f << 16) | ((int)23 << 8) | ((int)0 << 7) | 46),
   (((int)0x055c << 16) | ((int)24 << 8) | ((int)0 << 7) | 48),
   (((int)0x0406 << 16) | ((int)25 << 8) | ((int)0 << 7) | 49),
   (((int)0x0303 << 16) | ((int)26 << 8) | ((int)0 << 7) | 51),
   (((int)0x0240 << 16) | ((int)27 << 8) | ((int)0 << 7) | 52),
   (((int)0x01b1 << 16) | ((int)28 << 8) | ((int)0 << 7) | 54),
   (((int)0x0144 << 16) | ((int)29 << 8) | ((int)0 << 7) | 56),
   (((int)0x00f5 << 16) | ((int)30 << 8) | ((int)0 << 7) | 57),
   (((int)0x00b7 << 16) | ((int)31 << 8) | ((int)0 << 7) | 59),
   (((int)0x008a << 16) | ((int)32 << 8) | ((int)0 << 7) | 60),
   (((int)0x0068 << 16) | ((int)33 << 8) | ((int)0 << 7) | 62),
   (((int)0x004e << 16) | ((int)34 << 8) | ((int)0 << 7) | 63),
   (((int)0x003b << 16) | ((int)35 << 8) | ((int)0 << 7) | 32),
   (((int)0x002c << 16) | ((int)9 << 8) | ((int)0 << 7) | 33),
   (((int)0x5ae1 << 16) | ((int)37 << 8) | ((int)1 << 7) | 37),
   (((int)0x484c << 16) | ((int)38 << 8) | ((int)0 << 7) | 64),
   (((int)0x3a0d << 16) | ((int)39 << 8) | ((int)0 << 7) | 65),
   (((int)0x2ef1 << 16) | ((int)40 << 8) | ((int)0 << 7) | 67),
   (((int)0x261f << 16) | ((int)41 << 8) | ((int)0 << 7) | 68),
   (((int)0x1f33 << 16) | ((int)42 << 8) | ((int)0 << 7) | 69),
   (((int)0x19a8 << 16) | ((int)43 << 8) | ((int)0 << 7) | 70),
   (((int)0x1518 << 16) | ((int)44 << 8) | ((int)0 << 7) | 72),
   (((int)0x1177 << 16) | ((int)45 << 8) | ((int)0 << 7) | 73),
   (((int)0x0e74 << 16) | ((int)46 << 8) | ((int)0 << 7) | 74),
   (((int)0x0bfb << 16) | ((int)47 << 8) | ((int)0 << 7) | 75),
   (((int)0x09f8 << 16) | ((int)48 << 8) | ((int)0 << 7) | 77),
   (((int)0x0861 << 16) | ((int)49 << 8) | ((int)0 << 7) | 78),
   (((int)0x0706 << 16) | ((int)50 << 8) | ((int)0 << 7) | 79),
   (((int)0x05cd << 16) | ((int)51 << 8) | ((int)0 << 7) | 48),
   (((int)0x04de << 16) | ((int)52 << 8) | ((int)0 << 7) | 50),
   (((int)0x040f << 16) | ((int)53 << 8) | ((int)0 << 7) | 50),
   (((int)0x0363 << 16) | ((int)54 << 8) | ((int)0 << 7) | 51),
   (((int)0x02d4 << 16) | ((int)55 << 8) | ((int)0 << 7) | 52),
   (((int)0x025c << 16) | ((int)56 << 8) | ((int)0 << 7) | 53),
   (((int)0x01f8 << 16) | ((int)57 << 8) | ((int)0 << 7) | 54),
   (((int)0x01a4 << 16) | ((int)58 << 8) | ((int)0 << 7) | 55),
   (((int)0x0160 << 16) | ((int)59 << 8) | ((int)0 << 7) | 56),
   (((int)0x0125 << 16) | ((int)60 << 8) | ((int)0 << 7) | 57),
   (((int)0x00f6 << 16) | ((int)61 << 8) | ((int)0 << 7) | 58),
   (((int)0x00cb << 16) | ((int)62 << 8) | ((int)0 << 7) | 59),
   (((int)0x00ab << 16) | ((int)63 << 8) | ((int)0 << 7) | 61),
   (((int)0x008f << 16) | ((int)32 << 8) | ((int)0 << 7) | 61),
   (((int)0x5b12 << 16) | ((int)65 << 8) | ((int)1 << 7) | 65),
   (((int)0x4d04 << 16) | ((int)66 << 8) | ((int)0 << 7) | 80),
   (((int)0x412c << 16) | ((int)67 << 8) | ((int)0 << 7) | 81),
   (((int)0x37d8 << 16) | ((int)68 << 8) | ((int)0 << 7) | 82),
   (((int)0x2fe8 << 16) | ((int)69 << 8) | ((int)0 << 7) | 83),
   (((int)0x293c << 16) | ((int)70 << 8) | ((int)0 << 7) | 84),
   (((int)0x2379 << 16) | ((int)71 << 8) | ((int)0 << 7) | 86),
   (((int)0x1edf << 16) | ((int)72 << 8) | ((int)0 << 7) | 87),
   (((int)0x1aa9 << 16) | ((int)73 << 8) | ((int)0 << 7) | 87),
   (((int)0x174e << 16) | ((int)74 << 8) | ((int)0 << 7) | 72),
   (((int)0x1424 << 16) | ((int)75 << 8) | ((int)0 << 7) | 72),
   (((int)0x119c << 16) | ((int)76 << 8) | ((int)0 << 7) | 74),
   (((int)0x0f6b << 16) | ((int)77 << 8) | ((int)0 << 7) | 74),
   (((int)0x0d51 << 16) | ((int)78 << 8) | ((int)0 << 7) | 75),
   (((int)0x0bb6 << 16) | ((int)79 << 8) | ((int)0 << 7) | 77),
   (((int)0x0a40 << 16) | ((int)48 << 8) | ((int)0 << 7) | 77),
   (((int)0x5832 << 16) | ((int)81 << 8) | ((int)1 << 7) | 80),
   (((int)0x4d1c << 16) | ((int)82 << 8) | ((int)0 << 7) | 88),
   (((int)0x438e << 16) | ((int)83 << 8) | ((int)0 << 7) | 89),
   (((int)0x3bdd << 16) | ((int)84 << 8) | ((int)0 << 7) | 90),
   (((int)0x34ee << 16) | ((int)85 << 8) | ((int)0 << 7) | 91),
   (((int)0x2eae << 16) | ((int)86 << 8) | ((int)0 << 7) | 92),
   (((int)0x299a << 16) | ((int)87 << 8) | ((int)0 << 7) | 93),
   (((int)0x2516 << 16) | ((int)71 << 8) | ((int)0 << 7) | 86),
   (((int)0x5570 << 16) | ((int)89 << 8) | ((int)1 << 7) | 88),
   (((int)0x4ca9 << 16) | ((int)90 << 8) | ((int)0 << 7) | 95),
   (((int)0x44d9 << 16) | ((int)91 << 8) | ((int)0 << 7) | 96),
   (((int)0x3e22 << 16) | ((int)92 << 8) | ((int)0 << 7) | 97),
   (((int)0x3824 << 16) | ((int)93 << 8) | ((int)0 << 7) | 99),
   (((int)0x32b4 << 16) | ((int)94 << 8) | ((int)0 << 7) | 99),
   (((int)0x2e17 << 16) | ((int)86 << 8) | ((int)0 << 7) | 93),
   (((int)0x56a8 << 16) | ((int)96 << 8) | ((int)1 << 7) | 95),
   (((int)0x4f46 << 16) | ((int)97 << 8) | ((int)0 << 7) | 101),
   (((int)0x47e5 << 16) | ((int)98 << 8) | ((int)0 << 7) | 102),
   (((int)0x41cf << 16) | ((int)99 << 8) | ((int)0 << 7) | 103),
   (((int)0x3c3d << 16) | ((int)100 << 8) | ((int)0 << 7) | 104),
   (((int)0x375e << 16) | ((int)93 << 8) | ((int)0 << 7) | 99),
   (((int)0x5231 << 16) | ((int)102 << 8) | ((int)0 << 7) | 105),
   (((int)0x4c0f << 16) | ((int)103 << 8) | ((int)0 << 7) | 106),
   (((int)0x4639 << 16) | ((int)104 << 8) | ((int)0 << 7) | 107),
   (((int)0x415e << 16) | ((int)99 << 8) | ((int)0 << 7) | 103),
   (((int)0x5627 << 16) | ((int)106 << 8) | ((int)1 << 7) | 105),
   (((int)0x50e7 << 16) | ((int)107 << 8) | ((int)0 << 7) | 108),
   (((int)0x4b85 << 16) | ((int)103 << 8) | ((int)0 << 7) | 109),
   (((int)0x5597 << 16) | ((int)109 << 8) | ((int)0 << 7) | 110),
   (((int)0x504f << 16) | ((int)107 << 8) | ((int)0 << 7) | 111),
   (((int)0x5a10 << 16) | ((int)111 << 8) | ((int)1 << 7) | 110),
   (((int)0x5522 << 16) | ((int)109 << 8) | ((int)0 << 7) | 112),
   (((int)0x59eb << 16) | ((int)111 << 8) | ((int)1 << 7) | 112)
};

// ---- arithmetic decoder (T.81 Annex D) --------------------------------------
// libjpeg's jdarith.c, with the byte fetch pulling from the coroutine.

int  ar_c, ar_a, ar_ct;
int  ar_last_dc[4];
int  ar_dc_context[4];
byte ar_dc_stats[16][64];
byte ar_ac_stats[16][256];
uint ar_dead;                 // set once the coder hits a bad code

// The arithmetic coder's own byte fetch.  Same FF00 / marker rules as the
// Huffman bit reader; once a marker is seen it feeds zeroes, as libjpeg does.
uint ar_byte( void ) {
  uint c;
  if( e_marker || e_eof ) return 0;
  c = get();
  if( c==uint(-1) ) { e_eof=1; return 0; }
  e_bytes++;
  if( c!=0xFF ) return c;
  do {
    c = get();
    if( c==uint(-1) ) { e_eof=1; return 0; }
    e_bytes++;
  } while( c==0xFF );
  if( c==0 ) return 0xFF;
  e_marker = c;
  return 0;
}

int arith_decode( byte* st ) {
  int qe,temp,sv,data;
  byte nl,nm;
  while( ar_a < 0x8000 ) {
    if( --ar_ct < 0 ) {
      data = int( ar_byte() );
      ar_c = (ar_c<<8) | data;
      if( (ar_ct += 8) < 0 ) if( ++ar_ct == 0 ) ar_a = 0x8000;
    }
    ar_a <<= 1;
  }
  sv = *st;
  qe = jaritab[ sv & 0x7F ];
  nl = byte(qe & 0xFF); qe >>= 8;
  nm = byte(qe & 0xFF); qe >>= 8;
  temp = ar_a - qe;
  ar_a = temp;
  temp <<= ar_ct;
  if( ar_c >= temp ) {
    ar_c -= temp;
    if( ar_a < qe ) { ar_a = qe; *st = byte((sv & 0x80) ^ nm); }
    else            { ar_a = qe; *st = byte((sv & 0x80) ^ nl); sv ^= 0x80; }
  } else if( ar_a < 0x8000 ) {
    if( ar_a < qe ) { *st = byte((sv & 0x80) ^ nl); sv ^= 0x80; }
    else            { *st = byte((sv & 0x80) ^ nm); }
  }
  return sv >> 7;
}

void arith_reset_coder( void ) { ar_c=0; ar_a=0; ar_ct=-16; ar_dead=0; }

void arith_reset_stats( void ) {
  uint i;
  for( i=0; i<uint(comps_in_scan); i++ ) {
    uint c = cs_cmp[i];
    if( (progressive_mode==0) || ((Ss==0) && (Ah==0)) ) {
      bzero( ar_dc_stats[ comp_info[c].dc_tbl_no & 15 ] );
      ar_last_dc[i] = 0; ar_dc_context[i] = 0;
    }
    if( (progressive_mode==0) || Ss ) bzero( ar_ac_stats[ comp_info[c].ac_tbl_no & 15 ] );
  }
}

// libjpeg's decode_mcu1 (sequential) and decode_mcu1_DC_first share this
uint ar_decode_dc( uint ci, uint tbl ) {
  byte* st;
  int sign,v,m;
  st = ar_dc_stats[tbl] + ar_dc_context[ci];
  if( arith_decode(st)==0 ) { ar_dc_context[ci] = 0; return 1; }
  sign = arith_decode( st+1 );
  st += 2; st += sign;
  if( (m = arith_decode(st)) != 0 ) {
    st = ar_dc_stats[tbl] + 20;
    while( arith_decode(st) ) {
      if( (m <<= 1) == 0x8000 ) { ar_dead=1; return 0; }
      st += 1;
    }
  }
  if( m < ((1 << arith_dc_L[tbl]) >> 1) )      ar_dc_context[ci] = 0;
  else if( m > ((1 << arith_dc_U[tbl]) >> 1) ) ar_dc_context[ci] = 12 + (sign*4);
  else                                         ar_dc_context[ci] = 4 + (sign*4);
  v = m;
  st += 14;
  while( m >>= 1 ) if( arith_decode(st) ) v |= m;
  v += 1; if( sign ) v = -v;
  ar_last_dc[ci] += v;
  return 1;
}

// one AC coefficient magnitude, shared by the sequential and AC-first paths
uint ar_decode_ac_val( uint tbl, byte* st, int k ) {
  int sign,v,m;
  ar_ac_stats[tbl][245] = 0;
  sign = arith_decode( ar_ac_stats[tbl] + 245 );
  st += 2;
  if( (m = arith_decode(st)) != 0 ) {
    if( arith_decode(st) ) {
      m <<= 1;
      st = ar_ac_stats[tbl] + ( (k <= int(arith_ac_K[tbl])) ? 189 : 217 );
      while( arith_decode(st) ) {
        if( (m <<= 1) == 0x8000 ) { ar_dead=1; return 0; }
        st += 1;
      }
    }
  }
  v = m;
  st += 14;
  while( m >>= 1 ) if( arith_decode(st) ) v |= m;
  (void)v; (void)sign;
  return 1;
}

uint ar_mcu_seq( void ) {
  uint blkn,ci,tbl; int k; byte* st;
  if( ar_dead ) return 1;
  for( blkn=0; blkn<uint(blocks_in_MCU); blkn++ ) {
    ci = uint(MCU_membership[blkn]);
    tbl = uint(comp_info[cs_cmp[ci]].dc_tbl_no) & 15;
    if( !ar_decode_dc(ci,tbl) ) return 1;
    tbl = uint(comp_info[cs_cmp[ci]].ac_tbl_no) & 15;
    for( k=1; k<64; k++ ) {
      st = ar_ac_stats[tbl] + 3*(k-1);
      if( arith_decode(st) ) break;
      while( arith_decode(st+1)==0 ) {
        st += 3; k++;
        if( k>=64 ) { ar_dead=1; return 1; }
      }
      if( !ar_decode_ac_val(tbl,st,k) ) return 1;
    }
    e_blocks++;
  }
  return 1;
}

uint ar_mcu_dc_first( void ) {
  uint blkn,ci,tbl;
  if( ar_dead ) return 1;
  for( blkn=0; blkn<uint(blocks_in_MCU); blkn++ ) {
    ci = uint(MCU_membership[blkn]);
    tbl = uint(comp_info[cs_cmp[ci]].dc_tbl_no) & 15;
    if( !ar_decode_dc(ci,tbl) ) return 1;
    e_blocks++;
  }
  return 1;
}

uint ar_mcu_dc_refine( void ) {
  uint blkn; byte st[4];
  for( blkn=0; blkn<uint(blocks_in_MCU); blkn++ ) {
    st[0] = 0;
    (void)arith_decode( st );
    e_blocks++;
  }
  return 1;
}

uint ar_mcu_ac_first( void ) {
  uint tbl; int k; byte* st;
  if( ar_dead ) { e_blocks++; return 1; }
  tbl = uint(comp_info[cs_cmp[0]].ac_tbl_no) & 15;
  nz_load();
  for( k=int(Ss); k<=int(Se); k++ ) {
    st = ar_ac_stats[tbl] + 3*(k-1);
    if( arith_decode(st) ) break;
    while( arith_decode(st+1)==0 ) {
      st += 3; k++;
      if( k > int(Se) ) { ar_dead=1; nz_store(); e_blocks++; return 1; }
    }
    if( !ar_decode_ac_val(tbl,st,k) ) { nz_store(); e_blocks++; return 1; }
    if( (k>=0) && (k<64) ) nz_cur |= qword(1)<<k;
  }
  nz_store(); e_blocks++;
  return 1;
}

uint ar_mcu_ac_refine( void ) {
  uint tbl; int k,kex; byte* st;
  if( ar_dead ) { e_blocks++; return 1; }
  tbl = uint(comp_info[cs_cmp[0]].ac_tbl_no) & 15;
  nz_load();
  // libjpeg scans down for the highest nonzero coefficient; the bitmap has it
  for( kex=int(Se)+1; kex>1; kex-- ) if( (nz_cur>>(kex-1)) & 1 ) break;
  for( k=int(Ss); k<=int(Se); k++ ) {
    st = ar_ac_stats[tbl] + 3*(k-1);
    if( k>=kex ) if( arith_decode(st) ) break;
    while(1) {
      if( (nz_cur>>k) & 1 ) {
        (void)arith_decode( st+2 );          // correction bit
        break;
      }
      if( arith_decode(st+1) ) {
        ar_ac_stats[tbl][245] = 0;
        (void)arith_decode( ar_ac_stats[tbl] + 245 );   // sign
        nz_cur |= qword(1)<<k;                          // newly nonzero
        break;
      }
      st += 3; k++;
      if( k > int(Se) ) { ar_dead=1; nz_store(); e_blocks++; return 1; }
    }
  }
  nz_store(); e_blocks++;
  return 1;
}

// ---- per-scan setup ---------------------------------------------------------
// libjpeg's per_scan_setup, filling the MCU geometry pjpg's cinfo already has
// fields for.

uint per_scan_setup( void ) {
  uint ci,mcublks;
  if( comps_in_scan==1 ) {
    jpeg_component_info& c = *cur_comp_info[0];
    MCUs_per_row = c.width_in_blocks;
    MCU_rows_in_scan = c.height_in_blocks;
    c.MCU_width = 1; c.MCU_height = 1; c.MCU_blocks = 1;
    blocks_in_MCU = 1;
    MCU_membership[0] = 0;
  } else {
    if( (comps_in_scan<=0) || (comps_in_scan>4) ) return 0;
    MCUs_per_row     = jdiv_round_up( int(image_width),  max_h_samp_factor*8 );
    MCU_rows_in_scan = jdiv_round_up( int(image_height), max_v_samp_factor*8 );
    blocks_in_MCU = 0;
    for( ci=0; ci<uint(comps_in_scan); ci++ ) {
      jpeg_component_info& c = *cur_comp_info[ci];
      c.MCU_width = c.h_samp_factor;
      c.MCU_height = c.v_samp_factor;
      c.MCU_blocks = c.MCU_width * c.MCU_height;
      mcublks = uint(c.MCU_blocks);
      if( uint(blocks_in_MCU)+mcublks > 10 ) return 0;   // sampling factors too large
      while( mcublks-- > 0 ) MCU_membership[blocks_in_MCU++] = int(ci);
    }
  }
  if( (MCUs_per_row==0) || (MCU_rows_in_scan==0) ) return 0;
  return 1;
}

// Block geometry for the frame, libjpeg's initial_setup.  Called once per SOF.
uint setup_frame_blocks( void ) {
  uint c;
  if( (image_width==0) || (image_height==0) ) return 0;
  if( (max_h_samp_factor<1) || (max_v_samp_factor<1) ) return 0;
  for( c=0; c<uint(num_components); c++ ) {
    comp_info[c].width_in_blocks  =
      uint(jdiv_round_up( int(image_width) *comp_info[c].h_samp_factor, max_h_samp_factor*8 ));
    comp_info[c].height_in_blocks =
      uint(jdiv_round_up( int(image_height)*comp_info[c].v_samp_factor, max_v_samp_factor*8 ));
    comp_info[c].blocks_total = comp_info[c].width_in_blocks * comp_info[c].height_in_blocks;
    if( comp_info[c].blocks_total==0 ) return 0;
  }
  return 1;
}

// ---- restart ----------------------------------------------------------------

// Advance to the next marker in the input, whatever it is.  Used to find the
// RSTn between restart intervals and the marker that ends the scan.
uint e_resync;      // bytes next_marker_now() had to skip to find the marker

uint next_marker_now( void ) {
  uint c;
  if( e_marker ) return e_marker;
  while(1) {
    c = get();
    if( c==uint(-1) ) { e_eof=1; return 0; }
    e_bytes++;
    if( c!=0xFF ) { e_resync++; continue; }
    do {
      c = get();
      if( c==uint(-1) ) { e_eof=1; return 0; }
      e_bytes++;
    } while( c==0xFF );
    if( c==0 ) { e_resync += 2; continue; }   // stuffed byte, still entropy data
    e_marker = c;
    return c;
  }
}

// libjpeg's process_restart: drop the pad bits, consume the RSTn, reset the
// predictors.  0 if what follows is not a restart marker, which ends the scan.
uint process_restart( void ) {
  uint i,m;
  e_bits = 0; e_buf = 0;
  m = next_marker_now();
  if( (m==0) || ((m & 0xF8) != M_RST0) ) return 0;
  e_marker = 0;                          // consumed; entropy data resumes
  n_rst_seen++;
  for( i=0; i<4; i++ ) last_dc_val[i] = 0;
  EOBRUN = 0;
  e_short = 0;
  restarts_to_go = restart_interval;
  if( arith_code ) { arith_reset_stats(); arith_reset_coder(); }
  return 1;
}

// ---- the scan driver --------------------------------------------------------

uint e_scan_ok;        // 1 if the last scan decoded without complaint
uint n_scan_dec;       // scans actually decoded

// Can this scan be entropy-decoded at all?
uint scan_decodable( void ) {
  uint i,c;
  if( saw_SOF==0 ) return 0;
  if( !setup_frame_blocks() ) return 0;
  if( sof_lossless || sof_hier ) return 0;       // not DCT-coded
  if( f_SOS==0 ) return 0;
  if( (comps_in_scan<1) || (comps_in_scan>4) ) return 0;
  if( data_precision!=8 ) return 0;              // libjpeg's decoders are 8-bit
  if( Se>63 ) return 0;
  for( i=0; i<uint(comps_in_scan); i++ ) {
    c = cs_cmp[i];
    if( c>=uint(num_components) ) return 0;
    if( (comp_info[c].h_samp_factor<1) || (comp_info[c].v_samp_factor<1) ) return 0;
  }
  return 1;
}

// Build the Huffman tables this scan needs.  0 if one was never defined.
uint scan_tables_ready( void ) {
  uint i,c;
  if( arith_code ) return 1;
  for( i=0; i<uint(comps_in_scan); i++ ) {
    c = cs_cmp[i];
    if( progressive_mode ) {
      if( Ss==0 ) { if( Ah==0 ) if( !make_derived(1,uint(comp_info[c].dc_tbl_no)) ) return 0; }
      else        { if( !make_derived(0,uint(comp_info[c].ac_tbl_no)) ) return 0; }
    } else {
      if( !make_derived(1,uint(comp_info[c].dc_tbl_no)) ) return 0;
      if( !make_derived(0,uint(comp_info[c].ac_tbl_no)) ) return 0;
    }
  }
  return 1;
}

// Point the per-MCU-block table pointers at the right derived tables.
void scan_bind_tables( void ) {
  uint blkn,ci,c;
  for( blkn=0; blkn<uint(blocks_in_MCU); blkn++ ) {
    ci = uint(MCU_membership[blkn]);
    c = cs_cmp[ci];
    dc_cur[blkn] = &dtbl[0][ uint(comp_info[c].dc_tbl_no) & 3 ];
    ac_cur[blkn] = &dtbl[1][ uint(comp_info[c].ac_tbl_no) & 3 ];
  }
}

// Decode the entropy-coded segment that follows the SOS just parsed.
// Returns the marker that ended it, or 0 at end of input.
uint decode_scan( void ) {
  uint row,col,i,mcu_ok;

  bits_init();
  e_bytes = 0; e_blocks = 0; e_mcus = 0; n_rst_seen = 0; e_resync = 0;
  EOBRUN = 0;
  for( i=0; i<4; i++ ) last_dc_val[i] = 0;
  restarts_to_go = restart_interval;
  e_scan_ok = 1;

  if( !per_scan_setup() ) { pjpg_warn( "scan geometry is unusable, skipping its data" ); e_scan_ok=0; return next_marker_now(); }
  if( !scan_tables_ready() ) { pjpg_warn( "a Huffman table this scan uses was never defined" ); e_scan_ok=0; return next_marker_now(); }
  if( !arith_code ) scan_bind_tables();
  else { arith_reset_stats(); arith_reset_coder(); }

  // A refinement scan needs to know which coefficients earlier scans made
  // nonzero; allocate that the first time a progressive frame needs it.
  if( progressive_mode && (nz_on==0) ) {
    if( !nz_alloc() ) pjpg_warn( "image too large for the coefficient map, refinement scans will be approximate" );
  }
  nz_ci = cs_cmp[0];

  for( row=0; row<MCU_rows_in_scan; row++ ) {
    for( col=0; col<MCUs_per_row; col++ ) {

      if( restart_interval ) {
        if( restarts_to_go==0 ) {
          if( !process_restart() ) {
            pjpg_warn( "entropy data ended before all MCUs were decoded" );
            e_scan_ok = 0;
            goto done;
          }
        }
        restarts_to_go--;
      }

      nz_pos = row*MCUs_per_row + col;    // only meaningful when non-interleaved

      if( arith_code ) {
        if( progressive_mode==0 )   mcu_ok = ar_mcu_seq();
        else if( Ss==0 )            mcu_ok = (Ah==0) ? ar_mcu_dc_first() : ar_mcu_dc_refine();
        else                        mcu_ok = (Ah==0) ? ar_mcu_ac_first() : ar_mcu_ac_refine();
      } else {
        if( progressive_mode==0 )   mcu_ok = dec_mcu_seq();
        else if( Ss==0 )            mcu_ok = (Ah==0) ? dec_mcu_dc_first() : dec_mcu_dc_refine();
        else                        mcu_ok = (Ah==0) ? dec_mcu_ac_first() : dec_mcu_ac_refine();
      }
      e_mcus++;
      if( mcu_ok==0 ) { e_scan_ok=0; goto done; }
      if( e_eof ) { e_scan_ok=0; goto done; }
    }
  }

done:
  if( e_short )  { pjpg_warn( "entropy data ran out before the scan was complete" ); e_scan_ok = 0; }
  if( ar_dead )  { pjpg_warn( "bad arithmetic code in the scan" ); e_scan_ok = 0; }

  n_scan_dec++;
  pf( "  scan %u: %s%s, %u MCUs, %u blocks, %u entropy bytes",
      n_scan_dec,
      arith_code ? "arithmetic" : "Huffman",
      progressive_mode ? ((Ss==0) ? ((Ah==0)?" progressive DC":" progressive DC refine")
                                  : ((Ah==0)?" progressive AC":" progressive AC refine"))
                       : " sequential",
      e_mcus, e_blocks, e_bytes );
  if( n_rst_seen ) pf( ", %u restarts", n_rst_seen );
  if( e_resync )   pf( ", resync +%u", e_resync );
  pf( "%s\n", e_scan_ok ? "" : "  [INCOMPLETE]" );

  return next_marker_now();
}
