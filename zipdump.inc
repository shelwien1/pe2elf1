
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

  void do_process( void ) {

    uint inp_ofs=0;
    uint out_ofs=0;
    uint dir_ofs=-1;
    uint n_cells=0;
    qword z64_ofs=-1; // output offset of the ZIP64 EOCD record (for the locator)

    zcr.InitCRC();

    uint tag,c,d,i,j,l,f_cr;

    while(1) {
      tag = get_uint(); 
//printf( "tag=%08X ofs=%08X %08X %08X\n", tag, inpptr-inpbeg-4, inp_ofs, out_ofs );
      if( f_quit ) break;

      if( tag==L<'P','K',3,4>::n ) {

        l=hdr.Parse(this,0); l+=hdr.cSize64;

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

        // Push the patched size back where it actually lives: the 32-bit cSize
        // slot for plain entries, or the ZIP64 extra field for force_zip64
        // entries (whose cSize slot stays 0xFFFFFFFF).
        hdr.sync_zip64();

//printf( "mode=%i flags=%i f_cr=%i cSize=%llu\n", mode, hdr.flags, f_cr, (unsigned long long)hdr.cSize64 );

        hdr.Store(this);

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

        {
          int dSize = (f_cr==1) ? +12 : (f_cr==2) ? -12 : 0;
          if( n_cells<max_cells ) ofstbl[n_cells].Init( inp_ofs, out_ofs, hdr.cSize, hdr.flags, dSize );
          n_cells++;
        }

        inp_ofs+=l;
        out_ofs+=l;
        if( f_cr==2 ) out_ofs -= 12; else
        if( f_cr==1 ) out_ofs += 12;

      } else if( tag==L<'P','K',1,2>::n ) {

        if( dir_ofs==-1 ) dir_ofs = out_ofs;

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

      } else if( tag==L<'P','K',6,6>::n ) {

        // ZIP64 end of central directory record
        l=end64.Parse(this,0);

        z64_ofs = out_ofs; // remember where it lands for the locator
        // Only patch the offset when it isn't the (rare) self-referential
        // 0xFFFFFFFF marker; the real CD start in the output is dir_ofs.
        if( dir_ofs!=uint(-1) ) end64.DirOffs = dir_ofs;

        end64.Store(this);
        inp_ofs += l;
        out_ofs += l;

      } else if( tag==L<'P','K',6,7>::n ) {

        // ZIP64 end of central directory locator
        l=loc64.Parse(this,0);

        if( z64_ofs!=qword(-1) ) loc64.Z64EOCDOffs = z64_ofs;

        loc64.Store(this);
        inp_ofs += l;
        out_ofs += l;

      } else if( tag==L<'P','K',5,6>::n ) {

        l=end.Parse(this,0);

        // Leave the 0xFFFFFFFF marker in place for true ZIP64 archives (real
        // offset lives in the PK\6\6 record); otherwise point at the output CD.
        if( end.DirOffs!=0xFFFFFFFFu && dir_ofs!=uint(-1) ) end.DirOffs = dir_ofs;

        end.Store(this);
        inp_ofs += l;
        out_ofs += l;

      } else if( tag==L<'P','K',7,8>::n ) {

        l=dsc.Parse(this); 
//        if( mode==0 ) {
          dsc.Dump();
          dsc.Store(this);
          out_ofs += l;
//        }
        inp_ofs += l;

      } else break;
    }

    yield(this,0);
  }

};
