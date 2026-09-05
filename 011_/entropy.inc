
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
uint e_pad;                // consecutive MCUs decoded from padding, not data
enum { E_PAD_LIMIT = 1<<20 };
uint e_bytes;              // entropy bytes consumed, for reporting
qword e_end;               // offset of the FF that ended the segment, for a
                           // caller that has to cut the scan out of the file

void bits_init( void ) { e_buf=0; e_bits=0; e_marker=0; e_eof=0; e_short=0; e_end=0; }

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
        qword ff_at = at()-1;             // where this run of FFs began
        do {
          c = get();
          if( c==uint(-1) ) { e_eof=1; break; }
          e_bytes++;
        } while( c==0xFF );
        if( e_eof ) break;
        if( c==0 ) c = 0xFF;
        else { e_marker = c; e_end = ff_at; break; }   // a real marker: no more bytes
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

// ---- coefficient capture ----------------------------------------------------
//
// pjpg reads the entropy data to describe it, and throws the values away.  A
// transcoder wants them, so when co_buf is set the decoders write each block's
// 64 coefficients into it, in zigzag order -- the order they are coded in, and
// so the order that makes re-encoding a straight walk.
//
// The grid is padded to whole MCUs, because that is what the bitstream codes:
// an image 17 pixels wide with 2x1 sampling still has a block column past the
// edge in every MCU row, and it carries coded data that has to come back.  So a
// component's row is co_bw blocks wide whatever its width_in_blocks says.
short* co_buf;
qword  co_nblk;                  // blocks co_buf holds, for bounds checking
qword  co_base[4];               // where each frame component's blocks start
uint   co_bw[4], co_bh[4];       // its padded block grid
uint   mcu_row, mcu_col;         // where decode_scan's loops are

uint co_want;                    // allocate co_buf at the first decodable scan
uint co_failed;                  // ... and could not

// The grid is only knowable once a frame header has been read, and the first
// scan is the first moment it is needed, so that is where it is built.  Padded
// to whole MCUs because that is what the bitstream codes.
uint co_alloc( void ) {
  uint c,mw,mh;
  qword n;
  co_free();
  if( (num_components<1) || (num_components>4) ) { co_failed=1; return 0; }
  if( (max_h_samp_factor<1) || (max_v_samp_factor<1) ) { co_failed=1; return 0; }
  mw = uint(jdiv_round_up( int(image_width),  max_h_samp_factor*8 ));
  mh = uint(jdiv_round_up( int(image_height), max_v_samp_factor*8 ));
  n = 0;
  for( c=0; c<uint(num_components); c++ ) {
    co_bw[c] = mw * uint(comp_info[c].h_samp_factor);
    co_bh[c] = mh * uint(comp_info[c].v_samp_factor);
    co_base[c] = n;
    n += qword(co_bw[c]) * co_bh[c];
  }
  // 64 shorts a block, so this is the memory the whole image costs.  A cap,
  // because the dimensions come from the file and 65535x65535 asks for 550 GB.
  if( (n==0) || (n > (qword(1)<<28)) ) { co_failed=1; return 0; }
  co_buf = (short*)calloc( size_t(n)*64, sizeof(short) );
  if( co_buf==0 ) { co_failed=1; return 0; }
  co_nblk = n;
  return 1;
}

void co_free( void ) {
  if( co_buf ) free( co_buf );
  co_buf = 0; co_nblk = 0;
}

// Storage for block blkn of the MCU being decoded, or 0 if there is none.
short* co_block( uint blkn ) {
  uint ci,cc,o,r,c,wib;
  qword idx;
  if( co_buf==0 ) return 0;
  if( blkn >= uint(DIM(MCU_membership)) ) return 0;
  ci = uint(MCU_membership[blkn]);
  if( ci >= uint(comps_in_scan) ) return 0;
  cc = cs_cmp[ci];
  if( (cc>=4) || (cc>=uint(num_components)) ) return 0;

  if( comps_in_scan==1 ) {
    // A single-component scan walks that component's own blocks, left to right
    // and top to bottom over its real size -- not the padded one, so the row
    // has to be recovered before it can be indexed into the padded grid.
    wib = comp_info[cc].width_in_blocks;
    if( wib==0 ) return 0;
    r = mcu_row; c = mcu_col;                    // == nz_pos / wib, nz_pos % wib
    if( c>=wib ) return 0;
  } else {
    o = blkn - uint(MCU_first[ci]);
    r = mcu_row * uint(cur_comp_info[ci]->MCU_height) + o / uint(cur_comp_info[ci]->MCU_width);
    c = mcu_col * uint(cur_comp_info[ci]->MCU_width)  + o % uint(cur_comp_info[ci]->MCU_width);
  }
  if( (r>=co_bh[cc]) || (c>=co_bw[cc]) ) return 0;
  idx = co_base[cc] + qword(r)*co_bw[cc] + c;
  if( idx >= co_nblk ) return 0;
  return co_buf + idx*64;
}
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
  int  s,k,r,v;
  short* b;
  for( blkn=0; blkn<uint(blocks_in_MCU); blkn++ ) {
    uint ci = uint(MCU_membership[blkn]);
    b = co_block( blkn );
    s = huff_decode( *dc_cur[blkn] );
    if( s ) { r = int(get_bits(s)); s = HUFF_EXTEND(r,s); }
    s += last_dc_val[ci];
    last_dc_val[ci] = s;
    if( b ) b[0] = short(s);
    for( k=1; k<64; k++ ) {
      s = huff_decode( *ac_cur[blkn] );
      r = s>>4; s &= 15;
      if( s ) {
        k += r;
        v = int(get_bits(s));
        if( b && (k<64) ) b[k] = short(HUFF_EXTEND(v,s));
      }
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
  short* b;
  for( blkn=0; blkn<uint(blocks_in_MCU); blkn++ ) {
    uint ci = uint(MCU_membership[blkn]);
    s = huff_decode( *dc_cur[blkn] );
    if( s ) { r = int(get_bits(s)); s = HUFF_EXTEND(r,s); }
    s += last_dc_val[ci];
    last_dc_val[ci] = s;
    b = co_block( blkn );
    // Scaled, not shifted: the value is signed and a left shift of a negative
    // is undefined before C++20.  Multiplying by the same power of two is the
    // same arithmetic and the same instruction.
    if( b ) b[0] = short(s * (1<<Al));  // stored already scaled, as it is coded
    e_blocks++;
  }
  return 1;
}

// libjpeg's decode_mcu_DC_refine: one bit per block, unconditionally
uint dec_mcu_dc_refine( void ) {
  uint blkn;
  short* b;
  for( blkn=0; blkn<uint(blocks_in_MCU); blkn++ ) {
    b = co_block( blkn );
    if( get_bits(1) ) { if( b ) b[0] = short(b[0] | (1<<Al)); }
    e_blocks++;
  }
  return 1;
}

// libjpeg's decode_mcu_AC_first.  Always one block per MCU.
uint dec_mcu_ac_first( void ) {
  int s,k,r,v;
  short* b;
  if( EOBRUN>0 ) { EOBRUN--; e_blocks++; return 1; }
  b = co_block( 0 );
  nz_load();
  for( k=int(Ss); k<=int(Se); k++ ) {
    s = huff_decode( *ac_cur[0] );
    r = s>>4; s &= 15;
    if( s ) {
      k += r;
      v = int(get_bits(s));
      if( (k>=0) && (k<64) ) {
        nz_cur |= qword(1)<<k;                        // now nonzero
        if( b ) b[k] = short(HUFF_EXTEND(v,s) * (1<<Al)); // stored as coded
      }
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
  int s,k,r,p1,m1,kk;
  short* b;
  b  = co_block( 0 );
  p1 = 1 << Al;                         // the bit this scan adds, positive
  m1 = -(1 << Al);                      // and negative
  nz_load();
  // With the coefficients in hand, "is this one already nonzero" is a fact
  // rather than a bitmap lookup, so the map is only consulted when there is
  // nowhere to store values.
  #define AC_NONZERO(i)  ( b ? (b[i]!=0) : (((nz_cur>>(i)) & 1)!=0) )
  k = int(Ss);
  if( EOBRUN==0 ) {
    for( ; k<=int(Se); k++ ) {
      s = huff_decode( *ac_cur[0] );
      r = s>>4; s &= 15;
      if( s ) {
        s = get_bits(1) ? p1 : m1;      // sign of the newly nonzero coefficient
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
        if( AC_NONZERO(k) ) {
          if( get_bits(1) ) {           // correction bit
            if( b && ((b[k] & p1)==0) ) b[k] = short( (b[k]>=0) ? (b[k]+p1) : (b[k]+m1) );
          }
        } else {
          if( r==0 ) break;
          r--;
        }
        k++;
      }
      // libjpeg lands an overrun on jpeg_natural_order[64..79], all of which
      // are 63; under this relabelling that is coefficient 63.
      kk = (k>=0 && k<64) ? k : 63;
      if( s ) { nz_cur |= qword(1) << kk; if( b ) b[kk] = short(s); }
    }
  }
  if( EOBRUN>0 ) {
    for( ; k<=int(Se); k++ ) {
      if( AC_NONZERO(k) ) {
        if( get_bits(1) ) {
          if( b && ((b[k] & p1)==0) ) b[k] = short( (b[k]>=0) ? (b[k]+p1) : (b[k]+m1) );
        }
      }
    }
    EOBRUN--;
  }
  #undef AC_NONZERO
  nz_store();
  e_blocks++;
  return 1;
}

// The QM-coder probability estimation table, verbatim from djpeg's
// arith_tab.inc (JPEG T.81 Table D.3): Qe, next-MPS, switch, next-LPS packed.
static constexpr int jaritab[114] = {
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
   (((int)0x59eb << 16) | ((int)111 << 8) | ((int)1 << 7) | 112),
// Index 113 is not part of Table D.3: it is the fixed 0.5 estimate ITU-T
// T.851 10.3 Table 5 recommends, and libjpeg appends it for the same reason.
// Both transitions point back at itself with no MPS switch, so a bin left in
// this state stays at Qe=0x5A1D with the MPS at 0 no matter what is coded --
// which is what "does not adapt" means.
   (((int)0x5a1d << 16) | ((int)113 << 8) | ((int)0 << 7) | 113)
};

// ---- arithmetic decoder (T.81 Annex D) --------------------------------------
// libjpeg's jdarith.c, with the byte fetch pulling from the coroutine.

int  ar_c, ar_a, ar_ct;
int  ar_last_dc[4];
int  ar_dc_context[4];
byte ar_dc_stats[16][64];
byte ar_ac_stats[16][256];
// libjpeg's fixed_bin: the non-adapting bin the sign and the DC refinement bits
// are coded against (T.851 10.3).  This used to borrow ar_ac_stats[tbl][245],
// zeroed before each use -- which codes the identical decision, state 0 having
// the same Qe, and 245 is genuinely free: the highest index a real context
// reaches is 244.  But that is an unwritten bound holding by one, in an array
// something else owns, and the encoder in jpgarith.inc mirrors jcarith.c, which
// keeps the bin apart.  So it is apart here too.
byte ar_fixed[4];
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
  { qword ff_at = at()-1;               // where this run of FFs began
    do {
      c = get();
      if( c==uint(-1) ) { e_eof=1; return 0; }
      e_bytes++;
    } while( c==0xFF );
    if( c==0 ) return 0xFF;
    // next_marker_now() returns immediately once e_marker is set, so if this
    // does not record where the segment ended nothing will -- and the caller
    // that wants to know which bytes were the scan's gets an empty range.
    e_marker = c;
    e_end = ff_at;
  }
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

void arith_reset_coder( void ) {
  ar_c=0; ar_a=0; ar_ct=-16; ar_dead=0;
  ar_fixed[0]=113; ar_fixed[1]=0; ar_fixed[2]=0; ar_fixed[3]=0;
}

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

// libjpeg's decode_mcu1 (sequential) and decode_mcu1_DC_first share this.
// b, when the transcoder asked for coefficients, is the block to store into;
// scale is 1<<Al for a progressive first scan and 1 for a sequential one --
// the DC value is stored as it was coded, exactly as the Huffman path does.
uint ar_decode_dc( uint ci, uint tbl, short* b, int scale ) {
  byte* st;
  int sign,v,m;
  st = ar_dc_stats[tbl] + ar_dc_context[ci];
  if( arith_decode(st)==0 ) {
    ar_dc_context[ci] = 0;
    if( b ) b[0] = short(ar_last_dc[ci] * scale);
    return 1;
  }
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
  if( b ) b[0] = short(ar_last_dc[ci] * scale);
  return 1;
}

// one AC coefficient magnitude, shared by the sequential and AC-first paths.
// As above, b is where to store it and scale is 1<<Al (1 when sequential).
uint ar_decode_ac_val( uint tbl, byte* st, int k, short* b, int scale ) {
  int sign,v,m;
  sign = arith_decode( ar_fixed );
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
  v += 1; if( sign ) v = -v;
  if( b && (k>=0) && (k<64) ) b[k] = short(v * scale);
  return 1;
}

uint ar_mcu_seq( void ) {
  uint blkn,ci,tbl; int k; byte* st; short* b;
  if( ar_dead ) return 1;
  for( blkn=0; blkn<uint(blocks_in_MCU); blkn++ ) {
    ci = uint(MCU_membership[blkn]);
    b = co_block( blkn );
    tbl = uint(comp_info[cs_cmp[ci]].dc_tbl_no) & 15;
    if( !ar_decode_dc(ci,tbl,b,1) ) return 1;
    tbl = uint(comp_info[cs_cmp[ci]].ac_tbl_no) & 15;
    for( k=1; k<64; k++ ) {
      st = ar_ac_stats[tbl] + 3*(k-1);
      if( arith_decode(st) ) break;
      while( arith_decode(st+1)==0 ) {
        st += 3; k++;
        if( k>=64 ) { ar_dead=1; return 1; }
      }
      if( !ar_decode_ac_val(tbl,st,k,b,1) ) return 1;
    }
    e_blocks++;
  }
  return 1;
}

uint ar_mcu_dc_first( void ) {
  uint blkn,ci,tbl; short* b;
  if( ar_dead ) return 1;
  for( blkn=0; blkn<uint(blocks_in_MCU); blkn++ ) {
    ci = uint(MCU_membership[blkn]);
    b = co_block( blkn );
    tbl = uint(comp_info[cs_cmp[ci]].dc_tbl_no) & 15;
    if( !ar_decode_dc(ci,tbl,b,1<<Al) ) return 1;
    e_blocks++;
  }
  return 1;
}

uint ar_mcu_dc_refine( void ) {
  uint blkn; short* b;
  for( blkn=0; blkn<uint(blocks_in_MCU); blkn++ ) {
    b = co_block( blkn );
    if( arith_decode( ar_fixed ) ) { if( b ) b[0] = short(b[0] | (1<<Al)); }
    e_blocks++;
  }
  return 1;
}

uint ar_mcu_ac_first( void ) {
  uint tbl; int k; byte* st; short* b;
  if( ar_dead ) { e_blocks++; return 1; }
  tbl = uint(comp_info[cs_cmp[0]].ac_tbl_no) & 15;
  b = co_block( 0 );
  nz_load();
  for( k=int(Ss); k<=int(Se); k++ ) {
    st = ar_ac_stats[tbl] + 3*(k-1);
    if( arith_decode(st) ) break;
    while( arith_decode(st+1)==0 ) {
      st += 3; k++;
      if( k > int(Se) ) { ar_dead=1; nz_store(); e_blocks++; return 1; }
    }
    if( !ar_decode_ac_val(tbl,st,k,b,1<<Al) ) { nz_store(); e_blocks++; return 1; }
    if( (k>=0) && (k<64) ) nz_cur |= qword(1)<<k;
  }
  nz_store(); e_blocks++;
  return 1;
}

uint ar_mcu_ac_refine( void ) {
  uint tbl; int k,kex,p1,m1; byte* st; short* b;
  if( ar_dead ) { e_blocks++; return 1; }
  tbl = uint(comp_info[cs_cmp[0]].ac_tbl_no) & 15;
  b  = co_block( 0 );
  p1 = 1 << Al;                         // the bit this scan adds, positive
  m1 = -(1 << Al);                      // and negative
  nz_load();
  // As on the Huffman side: with the coefficients in hand "is this one already
  // nonzero" is a fact, and the bitmap is only consulted when there is nowhere
  // to keep them.  libjpeg reads the coefficient, so this reads it too whenever
  // it exists -- the two cannot then disagree.
  #define AR_NONZERO(i)  ( b ? (b[i]!=0) : (((nz_cur>>(i)) & 1)!=0) )
  // libjpeg scans down for the highest nonzero coefficient
  for( kex=int(Se); kex>0; kex-- ) if( AR_NONZERO(kex) ) break;
  for( k=int(Ss); k<=int(Se); k++ ) {
    st = ar_ac_stats[tbl] + 3*(k-1);
    if( k>kex ) if( arith_decode(st) ) break;
    while(1) {
      if( AR_NONZERO(k) ) {
        if( arith_decode( st+2 ) )           // correction bit
          if( b ) b[k] = short( b[k] + ((b[k]<0) ? m1 : p1) );
        break;
      }
      if( arith_decode(st+1) ) {
        if( arith_decode( ar_fixed ) ) { if( b ) b[k] = short(m1); }
        else                           { if( b ) b[k] = short(p1); }
        nz_cur |= qword(1)<<k;                          // newly nonzero
        break;
      }
      st += 3; k++;
      if( k > int(Se) ) { ar_dead=1; nz_store(); e_blocks++; return 1; }
    }
  }
  #undef AR_NONZERO
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
    MCU_first[0] = 0;
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
      MCU_first[ci] = blocks_in_MCU;
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
    { qword ff_at = at()-1;               // where this run of FFs began
      do {
        c = get();
        if( c==uint(-1) ) { e_eof=1; return 0; }
        e_bytes++;
      } while( c==0xFF );
      if( c==0 ) { e_resync += 2; continue; }   // stuffed byte, still entropy data
      e_marker = c;
      e_end = ff_at;
      return c;
    }
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
uint n_scan_bad;       // scans that should have decoded and did not, whether
                       // they failed in the data or never got as far as it.
                       // e_scan_ok only describes the last scan, and a detector
                       // deciding whether a candidate really is a JPEG needs
                       // the whole image's verdict, not the final scan's.

// libjpeg's scan-parameter validation (jdphuff.c start_pass_phuff_decoder and
// jdhuff.c start_pass_huff_decoder).  These are exact rules, not heuristics: a
// scan that breaks them cannot be decoded by any conforming decoder.
uint scan_params_valid( void ) {
  uint bad = 0;
  if( progressive_mode ) {
    if( Ss==0 ) {
      if( Se!=0 ) bad = 1;                      // a DC scan covers only coef 0
    } else {
      if( (Ss>Se) || (Se>=64) ) bad = 1;
      if( comps_in_scan!=1 ) bad = 1;           // an AC scan is single-component
    }
    if( Ah!=0 ) { if( Al != Ah-1 ) bad = 1; }   // refinement must be Al = Ah-1
    if( Al>13 ) bad = 1;
  } else {
    if( (Ss!=0) || (Se!=63) ) bad = 1;
    if( (Ah!=0) || (Al!=0) ) bad = 1;
  }
  if( bad ) {
    pjpg_err( E_BADSOS, progressive_mode ? "invalid progressive scan parameters"
                                         : "invalid sequential scan parameters" );
    return 0;
  }
  return 1;
}

// Can this scan be entropy-decoded at all?
uint scan_decodable( void ) {
  uint i,c;
  if( saw_SOF==0 ) return 0;
  if( !setup_frame_blocks() ) return 0;
  if( sof_lossless || sof_hier ) return 0;       // not DCT-coded
  if( f_SOS==0 ) return 0;
  if( (comps_in_scan<1) || (comps_in_scan>4) ) return 0;
  if( !scan_params_valid() ) return 0;
  if( data_precision!=8 ) return 0;              // libjpeg's decoders are 8-bit
  if( Se>63 ) return 0;
  for( i=0; i<uint(comps_in_scan); i++ ) {
    if( cur_comp_info[i]==0 ) return 0;          // header never resolved it
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
  if( co_want && (co_buf==0) ) co_alloc();
  e_bytes = 0; e_blocks = 0; e_mcus = 0; n_rst_seen = 0; e_resync = 0;
  // ar_dead is a verdict on ONE scan, and it was only ever cleared by
  // arith_reset_coder() -- which decode_scan calls on the arithmetic branch
  // alone, while the report below reads it for every scan.  So one arithmetic
  // scan that died condemned every later scan in the same parser, Huffman
  // included.  Invisible to pjpg, which parses one file per process; fatal to
  // anything that reuses a parser, which is what a stream detector does.
  ar_dead = 0; e_pad = 0;
  EOBRUN = 0;
  for( i=0; i<4; i++ ) last_dc_val[i] = 0;
  restarts_to_go = restart_interval;
  e_scan_ok = 1;

  // Every way per_scan_setup() can fail is a hard error in libjpeg -- a zero
  // dimension, or an MCU carrying more than the 10 data units T.81 A.2.3
  // allows ("Sampling factors too large for interleaved scan").  The early
  // return jumps over the counters at the bottom of this function, so without
  // recording the verdict here the scan is detected and then forgotten.
  if( !per_scan_setup() ) {
    pjpg_warn( "scan geometry is unusable, skipping its data" );
    n_nonconf++; n_scan_bad++;
    e_scan_ok=0; return next_marker_now();
  }
  if( !scan_tables_ready() ) {
    pjpg_err( E_BADDHT, "a Huffman table this scan uses is missing or bogus" );
    n_scan_bad++;
    e_scan_ok=0; return next_marker_now();
  }
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
      mcu_row = row; mcu_col = col;       // where co_block() puts what it stores

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

      // Once the entropy data has run out, every further MCU is decoded
      // against the zero padding fill_bit_buffer() substitutes -- so it
      // consumes no input, and the number of them is whatever the frame header
      // claimed.  A 155-byte file declaring 65535x65535 buys 67 million, and
      // since nothing is read, nothing yields: neither f_quit nor any limit the
      // frontend keeps can interrupt it.  Chaining scan headers multiplies it,
      // so 395 bytes cost 34 seconds and 13 KB would cost hours.
      //
      // libjpeg pads to the end of the frame too, and matching it is worth
      // more than the microseconds saved by stopping the moment the data ends
      // -- a progressive refinement scan reads the nonzero map the padded MCUs
      // of the scan before it left behind.  So this is a ceiling, not a cut:
      // high enough that no file whose declared size bears any relation to its
      // data can reach it, low enough that one that does not costs a few
      // milliseconds.  restarts_to_go is left alone; process_restart() clears
      // e_short, so real data resuming at an RSTn resets the count with it.
      if( e_short ) {
        if( ++e_pad > E_PAD_LIMIT ) { e_scan_ok=0; goto done; }
      } else e_pad = 0;
    }
  }

done:
  if( e_short )  { pjpg_warn( "entropy data ran out before the scan was complete" ); e_scan_ok = 0; }
  if( ar_dead )  { pjpg_warn( "bad arithmetic code in the scan" ); e_scan_ok = 0; }

  n_scan_dec++;
  if( e_scan_ok==0 ) n_scan_bad++;
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
