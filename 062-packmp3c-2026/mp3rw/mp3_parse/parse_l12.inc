
 // Layer I/II frames. No bit reservoir here: a frame is header, CRC,
 // audio data and ancillary data up to the next header.

 // CRC of nbits bits from bit position pos down, in the order getbits reads them
 word CalcCRCbits( byte* p, int pos, int nbits ) {
   int crc = 0xFFFF, b;
   for( ; nbits>0; nbits-- ) {
     pos--; b = (p[pos>>3]>>(pos&7))&1;
     crc = ((crc<<1) ^ ((((crc>>15)^b)&1) ? 0x8005 : 0)) & 0xFFFF;
   }
   return crc;
 }

 int L12Same( header& x ) {
   return x.ID==h.ID && x.IDex==h.IDex && x.layer==h.layer && x.frequency==h.frequency;
 }

 // Audio data via procbits, so it both reads and writes. fbits is what the
 // frame has after header and CRC. Returns 1 if the data wouldn't fit or uses
 // an invalid code, ie it isn't a frame.
 int ProcL12( int fbits ) {
   int sb, ch, s, b, k, n, nb, v, q;
   int lay1 = (h.layer==3), nch = (h.mode!=3)+1;
   int tab = lay1 ? 0 : L2Table();
   int sblimit = lay1 ? 32 : l2_sblimit[tab];
   int bound = L12Bound( sblimit );

   for( n=0,sb=0; sb<sblimit; sb++ ) n += ((sb<bound)?nch:1)*L12Nbal( tab, sb );
   if( n>fbits ) return 1;
   for( sb=0; sb<sblimit; sb++ ) {
     nb = L12Nbal( tab, sb );
     for( ch=0; ch<((sb<bound)?nch:1); ch++ ) {
       procbits( L.alloc[ch][sb], nb );
       if( lay1 && L.alloc[ch][sb]==15 ) return 1;
     }
     if( sb>=bound ) L.alloc[1][sb] = L.alloc[0][sb];
   }

   if( !lay1 ) {
     for( sb=0; sb<sblimit; sb++ ) for( ch=0; ch<nch; ch++ ) if( L.alloc[ch][sb] ) n+=2;
     if( n>fbits ) return 1;
     for( sb=0; sb<sblimit; sb++ ) for( ch=0; ch<nch; ch++ ) if( L.alloc[ch][sb] ) procbits( L.scfsi[ch][sb], 2 );
   }
   l12crcbits = n; // CRC covers allocation and scfsi

   if( CalcL12Bits()-32-(h.protection?0:16) > fbits ) return 1;

   for( sb=0; sb<sblimit; sb++ ) for( ch=0; ch<nch; ch++ ) if( L.alloc[ch][sb] ) {
     byte* f = L.scf[ch][sb];
     k = lay1 ? 2 : L.scfsi[ch][sb]; // 0: 3 scalefactors, 1: f0 f2, 2: f0, 3: f0 f2
     procbits( f[0], 6 );
     if( k==0 ) procbits( f[1], 6 ); else if( k!=3 ) f[1]=f[0];
     if( k!=2 ) procbits( f[2], 6 ); else f[2]=f[0];
     if( k==3 ) f[1]=f[2];
   }

   for( s=0; s<12; s++ ) for( sb=0; sb<sblimit; sb++ ) for( ch=0; ch<((sb<bound)?nch:1); ch++ ) {
     if( (b=L.alloc[ch][sb])==0 ) continue;
     word* x = &L.smp[ch][sb][lay1?s:3*s];
     if( lay1 ) { nb=b+1; procbits( x[0], nb ); continue; }
     k = l2_rows[l2_row[tab][sb]][b]; nb = l2_qbits[k];
     if( nb>0 ) { procbits( x[0], nb ); procbits( x[1], nb ); procbits( x[2], nb ); continue; }
     q = l2_levels[k]; nb = -nb; // 3 samples in one code
     if( ProcMode ) {
       v = getbits( sinf, nb );
       if( v>=q*q*q ) return 1;
       x[0] = v%q; x[1] = v/q%q; x[2] = v/q/q;
     } else {
       v = x[0] + q*(x[1] + q*x[2]);
       putbits( v, nb );
     }
   }

   return 0;
 }

 // h is a layer I/II header, already taken from the buffer. Returns 0 for a
 // frame, 2 if it's not one, 3 if the layer III frames before it have to be
 // finished first (then it's read again).
 int ReadFrameL12( void ) {
   int flen, fend, crcpos, i, l;

   if( h.IDex==0 || h.bitrate==0 || h.bitrate==15 || h.frequency==3 || h.emphasis==2 ) return 2;
   flen = CalcFrameLenL12();

   // accept a frame only if it continues the previous layer I/II frame or the
   // next one follows it, otherwise random data would yield lots of them
   ReadLoEnd( flen );
   fend = BufPtr-(flen-4)*8; // end of the frame
   i = l12prev && L12Same( l12hdr );
   if( fend>=32 ) {
     header n; (uint&)n = (uint&)sinf[(fend-32)/8];
     if( n.syncword==2047 && n.bitrate!=0 && n.bitrate!=15 && L12Same(n) ) i=1;
   }
   if( i==0 ) return 2;

   if( GrbPtr>=0 ) return 3; // layer III data is still pending

   if( fend<0 ) { ReadLoEnd( flen-4, 0 ); fend = BufPtr-(flen-4)*8; } // cut off by EOF

   memset( &L, 0, sizeof(L) );
   if( h.protection==0 ) CRC = (word&)sinf[(BufPtr-=16)/8];
   crcpos = BufPtr;
   if( ProcL12( BufPtr-fend ) ) return 2;

   // a wrong CRC doesn't make it any less of a frame, just keep the CRC then
   L.crcok = 0;
   if( h.protection==0 ) {
     word w = (word&)sinf[crcpos/8];
     (word&)sinf[crcpos/8] = (word&)h; // header bits 16..31 go first, as for layer III
     L.crcok = (CalcCRCbits( sinf, crcpos+16, 16+l12crcbits )==CRC);
     (word&)sinf[crcpos/8] = w;
   }

   // the rest of the frame is ancillary data
   GrbLen = BufPtr-fend;
   for( i=0; BufPtr>fend; i++ ) { l = Min<int>( 8, BufPtr-fend ); Garbage[i] = getbits( sinf, l )<<(8-l); }

   BufLen = BufPtr; // the bytes after the frame stay in the buffer
   l12hdr = h;
   rframe1 = -1;    // a layer III frame after this one starts without a bit reservoir
   return 0;
 }

 int WriteFrameL12( void ) {
   int i, j, l, flen = CalcFrameLenL12(), SavPtr = OutPtr, crcpos;
   ProcMode = 0;

   for( i=(OutPtr-8)/8, j=__max(0,i-2048); j<=i; j++ ) Out[j]=0;

   (header&)Out[(OutPtr-=32)>>3]=h;
   if( h.protection==0 ) (word&)Out[(OutPtr-=16)>>3] = (word&)h; // for the CRC
   crcpos = OutPtr;
   ProcL12( flen*8-(SavPtr-OutPtr) );
   if( h.protection==0 ) (word&)Out[crcpos>>3] = L.crcok ? CalcCRCbits( Out, crcpos+16, 16+l12crcbits ) : CRC;
   for( i=0; i*8<GrbLen; i++ ) { l = Min<int>( 8, GrbLen-i*8 ); putbits( Garbage[i]>>(8-l), l ); }

   // layer III frames are all written out by now, see ReadFrameL12
   for( i=0; i<flen; i++ ) if( outpos<outlen ) put( Out[SavPtr/8-1-i] ), outpos++;

   OutPtr = SavPtr;
   return 0;
 }
