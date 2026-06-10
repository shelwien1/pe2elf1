
void typehex( void* buf, uint l ) {
  uint i;
  byte* p = (byte*)buf;
  for( i=0; i<l; i++ ) printf( "%02X ", p[i] );
  printf( "\n" );
}

void typestr( void* buf, uint l ) {
  uint i;
  byte* p = (byte*)buf;
  printf( "\"" );
  for( i=0; i<l; i++ ) printf( p[i]<32?"\\x%02X":"%c", p[i] );
  printf( "\"" );
  printf( "\n" );
}

struct zipdump : Coroutine {

  template<int a,int b,int c,int d> struct B { enum{ n=(d<<24)+(c<<16)+(b<<8)+a }; };
  template<int a,int b,int c,int d> struct L { enum{ n=(a<<24)+(b<<16)+(c<<8)+d }; };

  #include "zip_filerec.inc"
  #include "zip_direntry.inc"
  #include "zip_endloc.inc"
  #include "zip_end64.inc"
  #include "zip_descr.inc"

  #include "zip_getput.inc"

  #include "zipcrypt.inc"

  uint k0,k1,k2, pad0,pad1,pad2;
  uint mode; // 0=enc 1=dec
  uint last_zip64; // ZIP64 status of the most recent local file header
  zipcrypt zcr;
  char password[256];

  union {
    ZIPFILERECORD hdr;
    ZIPDIRENTRY idx;
    ZIPENDLOCATOR end;
    ZIP64ENDLOCATOR end64;
    ZIP64LOCATOR loc64;
    ZIPDATADESCR dsc;
  };

  // Large enough for archives with up to ~1M members (ZIP64's main use case
  // beyond 4 GiB is >65535 files). Writes are bounds-checked below so an even
  // bigger archive degrades gracefully instead of corrupting memory.
  enum { max_cells = 1<<20 };

  struct cell {
    uint Offs0; // source offset
    uint Offs1; // output offset
    uint cSize1; // output cSize (32-bit slot value as stored in the local header)
    uint flags1; // output flags
    int  dSize;  // signed delta added to the real cSize by encryption (+12 in c mode, -12 in d mode)
    void Init( uint x, uint y, uint z, uint t, int d ) { Offs0=x; Offs1=y; cSize1=z; flags1=t; dSize=d; }
  };

  cell ofstbl[max_cells]; // table for offset recalc after recompression

  void p_bits( uint x, uint l=8 ) {
    uint i;
//    for( i=l-1; i!=-1; i-- ) printf( "%i", (x>>i)&1 );
    for( i=0; i<l; i++ ) printf( "%i", (x>>i)&1 );
  }

  // --- streaming (bit-3 / data-descriptor) support -------------------------
  // A small lookahead ring so a streamed entry's data descriptor can be found
  // by its 0x08074b50 signature + a compressed-size field equal to the bytes
  // consumed, without over-reading past it (the coroutine has no unget).
  byte la[64]; uint la_head, la_cnt;
  uint la_peek( uint i ) { return la[(la_head+i)&63]; }
  void la_push( uint b ) { la[(la_head+la_cnt)&63]=byte(b); la_cnt++; }
  uint la_pop( void ) { uint b=la[la_head]; la_head=(la_head+1)&63; la_cnt--; return b; }
  qword la_rd( uint off, uint n ) { qword v=0; for( uint k=0; k<n; k++ ) v|=qword(la_peek(off+k))<<(8*k); return v; }

  // Process a streaming entry's [file data][data descriptor].
  //   f_cr : 0=copy, 1=encrypt, 2=decrypt    zip64 : 8-byte descriptor sizes
  // Returns input bytes consumed / output bytes emitted (data + descriptor).
  void do_stream_entry( uint f_cr, uint zip64, qword& in_bytes, qword& out_bytes ) {
    uint N    = zip64 ? 8 : 4;
    uint want = 8 + N;     // sig(4)+crc(4)+csize(N): enough to recognise the descriptor
    uint ddlen= 8 + 2*N;   // full signed descriptor
    qword raw_cnt = 0;     // raw input data bytes == the input descriptor's cSize
    in_bytes = 0; out_bytes = 0;
    la_head = la_cnt = 0;

    if( f_cr ) { zcr.init_keys( password ); zcr.k0=k0; zcr.k1=k1; zcr.k2=k2; }

    if( f_cr==2 ) {                                    // decrypt: 12-byte header is input
      for( uint i=0; i<12; i++ ) { zcr.pad[i]=get(); in_bytes++; raw_cnt++; }
      zcr.crypt( zcr.pad, 12, mode );
      fwrite( zcr.pad, 1,12, PF );
    } else if( f_cr==1 ) {                             // encrypt: emit a 12-byte header
      fread( zcr.pad, 1,12, PF );
      zcr.crypt( zcr.pad, 12, mode );
      for( uint i=0; i<12; i++ ) { put( zcr.pad[i] ); out_bytes++; }
    }

    while(1) {
      while( la_cnt<want && !f_quit ) { uint b=get(); if( f_quit ) break; la_push(b); in_bytes++; }
      if( la_cnt>=want
          && la_peek(0)==0x50 && la_peek(1)==0x4b && la_peek(2)==7 && la_peek(3)==8
          && la_rd(8,N)==raw_cnt ) break;             // data descriptor reached
      if( la_cnt==0 ) break;                          // EOF safety
      uint in_b = la_pop();
      raw_cnt++;
      uint out_b = (f_cr==0) ? in_b : (in_b ^ zcr.decrypt_byte());
      if( f_cr ) zcr.update_keys( (f_cr==2) ? out_b : in_b );
      put( out_b ); out_bytes++;
    }

    while( la_cnt<ddlen && !f_quit ) { uint b=get(); if( f_quit ) break; la_push(b); in_bytes++; }
    byte dd[24];
    for( uint i=0; i<ddlen; i++ ) dd[i] = byte( la_cnt ? la_pop() : 0 );
    qword csz = 0; for( uint k=0; k<N; k++ ) csz |= qword(dd[8+k]) << (8*k);
    csz += (f_cr==1) ? +12 : (f_cr==2) ? -12 : 0;     // rewrite compressed size
    for( uint i=0; i<8; i++ )  { put( dd[i] );             out_bytes++; } // sig + crc
    for( uint k=0; k<N; k++ )  { put( byte(csz>>(8*k)) );  out_bytes++; } // cSize'
    for( uint k=0; k<N; k++ )  { put( dd[8+N+k] );         out_bytes++; } // uSize
  }

  void do_process( void ) {

    uint inp_ofs=0;
    uint out_ofs=0;
    uint n_cells=0;

    zcr.InitCRC();

    uint tag,c,d,i,j,l,f_cr;

    while(1) {
      tag = get_uint(); 
//printf( "tag=%08X ofs=%08X %08X %08X\n", tag, inpptr-inpbeg-4, inp_ofs, out_ofs );
      if( f_quit ) break;

      if( tag==L<'P','K',3,4>::n ) {

        uint hdrlen = hdr.Parse(this,0);
        uint f_dd = (hdr.flags>>3)&1;       // bit 3: a data descriptor trails the data
        last_zip64 = hdr.has_zip64;         // for a standalone PK\7\8 fallback
        l = hdrlen + hdr.cSize64;

        f_cr=0;

        // "c" mode (encrypt/restore)
        if( mode==0 ) {
          // don't enable encryption with 0x80 flag
          if( (hdr.flags&0x81)==0 ) f_cr=1, hdr.cSize64+=12, hdr.flags^=1;
          hdr.flags &= ~0x80;
        }

        // "d" mode (decrypt)
        if( mode==1 ) {
          hdr.flags &= ~0x80;
          if( (hdr.flags&0x01)==1 ) f_cr=2, hdr.cSize64-=12, hdr.flags^=1;
          else hdr.flags|=0x80; // decrypt mode, but wasn't encrypted
        }

        // For a streamed entry the local-header sizes are placeholders (the
        // real size is in the trailing descriptor), so don't touch them.
        if( !f_dd ) hdr.sync_zip64();

//printf( "mode=%i flags=%i f_cr=%i cSize=%llu\n", mode, hdr.flags, f_cr, (unsigned long long)hdr.cSize64 );

        hdr.Store(this);

        int dSize = (f_cr==1) ? +12 : (f_cr==2) ? -12 : 0;

        if( f_dd ) {

          // Streamed entry: scan for the data descriptor instead of trusting
          // the (zero) local-header size.
          qword sin, sout;
          do_stream_entry( f_cr, hdr.has_zip64, sin, sout );
          if( n_cells<max_cells ) ofstbl[n_cells].Init( inp_ofs, out_ofs, hdr.cSize, hdr.flags, dSize );
          n_cells++;
          inp_ofs += hdrlen + sin;
          out_ofs += hdrlen + sout;

        } else {

        if( f_cr ) {

          zcr.init_keys( password );

          if( f_cr==2 ) get_string( zcr.pad,12,12 ); else {
            //memcpy( zcr.pad,(byte*)"\x43\x19\xE5\x9E\x19\x4F\xD8\x92\x8E\xE5\xED\x97",12 );
            //memcpy( zcr.pad,(byte*)"\x9F\x1E\x5C\x49\xC0\xDE\xD0\x5D\xF1\xF8\x45\x36",12 );
            //memcpy( zcr.pad,&pad0,12 );
             fread( zcr.pad, 1,12, PF );
          }

          //zcr.init_keys( password );
          zcr.k0=k0;zcr.k1=k1;zcr.k2=k2;
          zcr.crypt( zcr.pad, 12, mode );

// 1+2+5+5+4+3*19 = 74 = 9.25*8

//for( i=0; i<12; i++ ) printf( "%02X", zcr.pad[i] );
//printf( "\n" );
//printf( "%08X %08X %08X !%08X %8i %8i !", (uint&)zcr.pad[0], (uint&)zcr.pad[4], (uint&)zcr.pad[8], hdr.crc32, hdr.uSize, hdr.cSize );
printf( "%08X %08X %08X %8llu %8llu !", (uint&)zcr.pad[8], hdr.crc32, (uint(hdr.FileDate)<<16)+hdr.FileTime, (unsigned long long)hdr.uSize64, (unsigned long long)hdr.cSize64 );
          if( mode==1 ) fwrite( zcr.pad, 1,12, PF );

          if( f_cr==1 ) put_string( zcr.pad,12,12 );

          qword jj = (f_cr==2) ? hdr.cSize64 : hdr.cSize64-12;
          for( qword ii=0; ii<jj; ii++ ) {
            uint c = get();
            d = c ^ zcr.decrypt_byte();
            zcr.update_keys( mode ? d : c );
            put( d );
if( ii<12 ) p_bits(d);
          }
printf( "\n" );

        } else {

          for( qword ii=0; ii<hdr.cSize64; ii++ ) put( get() );

        }

        if( n_cells<max_cells ) ofstbl[n_cells].Init( inp_ofs, out_ofs, hdr.cSize, hdr.flags, dSize );
        n_cells++;

        inp_ofs+=l;
        out_ofs+=l;
        if( f_cr==2 ) out_ofs -= 12; else
        if( f_cr==1 ) out_ofs += 12;

        }

      } else if( tag==L<'P','K',1,2>::n ) {

        l=idx.Parse(this,0);

        // Look up by the real (possibly ZIP64) local-header offset.
        uint key = (idx.realOffs() <= 0xFFFFFFFFu) ? uint(idx.realOffs()) : 0xFFFFFFFFu;
        uint nc = (n_cells<max_cells) ? n_cells : max_cells; // clamp to stored entries
        for( i=0,d=(1<<log2i(nc)); d>0; d>>=1 )
          if( (i+d<nc) && (key>=ofstbl[i+d].Offs0) ) i+=d;

        if( nc==0 || ofstbl[i].Offs0!=key ) {
//printf( "!!! %2i %2i %08X %08X\n", i, n_cells, ofstbl[i].Offs0, key );
        } else {
          // Patch offset, encryption-induced cSize delta, and flags in place,
          // writing to the 32-bit slot or the ZIP64 extra field as appropriate.
          idx.setOffs( ofstbl[i].Offs1 );
          if( ofstbl[i].dSize ) idx.addSize( ofstbl[i].dSize );
          idx.flags = ofstbl[i].flags1;
        }

        idx.Store(this);
        inp_ofs += l;
        out_ofs += l;

      } else if( tag==L<'P','K',5,5>::n ) {

        // Central directory digital signature record: copy through unchanged.
        put_Luint( B<'P','K',5,5>::n );
        uint sz = get_Lword(); put_Lword( sz );
        for( uint q=0; q<sz; q++ ) put( get() );
        l = 4+2+sz;
        inp_ofs += l; out_ofs += l;

      } else if( tag==L<'P','K',6,8>::n ) {

        // Archive extra data record (precedes the central directory): copy through.
        put_Luint( B<'P','K',6,8>::n );
        uint xl = get_Luint(); put_Luint( xl );
        for( uint q=0; q<xl; q++ ) put( get() );
        l = 4+4+xl;
        inp_ofs += l; out_ofs += l;

      } else if( tag==L<'P','K',6,6>::n ) {

        // ZIP64 end of central directory record. Its CD offset shifts by the
        // net size change accumulated in the local-header region.
        int delta = (int)(out_ofs - inp_ofs);
        l=end64.Parse(this,0);
        end64.DirOffs += delta;
        end64.Store(this);
        inp_ofs += l;
        out_ofs += l;

      } else if( tag==L<'P','K',6,7>::n ) {

        // ZIP64 end of central directory locator: its record offset shifts too.
        int delta = (int)(out_ofs - inp_ofs);
        l=loc64.Parse(this,0);
        loc64.Z64EOCDOffs += delta;
        loc64.Store(this);
        inp_ofs += l;
        out_ofs += l;

      } else if( tag==L<'P','K',5,6>::n ) {

        int delta = (int)(out_ofs - inp_ofs);
        l=end.Parse(this,0);
        // Leave the 0xFFFFFFFF marker in place for true ZIP64 archives (real
        // offset lives in the PK\6\6 record); otherwise shift by the net delta.
        if( end.DirOffs!=0xFFFFFFFFu ) end.DirOffs += delta;
        end.Store(this);
        inp_ofs += l;
        out_ofs += l;

      } else if( tag==L<'P','K',7,8>::n ) {

        // Standalone data descriptor (entry that wasn't routed through the
        // streaming path); copy through, honoring 8-byte ZIP64 sizes.
        l=dsc.Parse(this,0,last_zip64);
          dsc.Dump();
          dsc.Store(this);
          out_ofs += l;
        inp_ofs += l;

      } else break;
    }

    yield(this,0);
  }

};
