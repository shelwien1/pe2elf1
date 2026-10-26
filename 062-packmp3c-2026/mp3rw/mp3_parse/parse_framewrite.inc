
 int WriteFrame( void ) {

   if( h.layer!=1 ) return WriteFrameL12();

   int retval, i, j, c, stl, SavPtr, tmp, errflag = si.p23cleared;
   ProcMode = 0; // encoding

   for( i=(OutPtr-8)/8, j=__max(0,i-2048); j<=i; j++ ) Out[j]=0;

   SavPtr = OutPtr;

//   putbits( (uint&)h, 32 ); // store header  
   (header&)Out[(OutPtr-=32)>>3]=h;
   stl = 4; // header size

   // header low bits for CRC (if present)
   if( h.protection==0 ) procbits( ((word&)h), 16 ), stl+=2; 

   ProcSideInfo();
   int chgr = (h.mode!=3)*2 + h.ID;
   stl += l_si[chgr];

   if( h.protection==0 ) {
     i = OutPtr/8;
     (word&)Out[i+l_si[chgr]] = CalcCRC( &Out[i], l_si[chgr]+2 );
   }

   for( i=0; i<stl; i++ ) {
     Hdr[Hdr1].h[stl-1-i] = Out[OutPtr/8+i];
     Out[OutPtr/8+i] = 0; // for further writes via OR
   }
   Hdr[Hdr1].HSize = stl;
   // size of maindata frame part
   stl=(h.ID?1152:576)/8*bitrat[h.ID][h.bitrate]/spfreq[h.ID][h.IDex][h.frequency]+h.padding-stl;
   Hdr[Hdr1].MSize = stl;
   Hdr1++; // autowrap

   if( errflag ) {
     OutPtr = SavPtr;
     GarbageFlush();
   } else {
     OutPtr = SavPtr + si.main_data_begin*8;
     GarbageFlush();
     h.ID ? ProcScaleFactors1() : ProcScaleFactors2();     
   }

   OutPtr = SavPtr - stl*8;
//   while( (Hdr2!=Hdr1 && OutPtr/8+BufSiz<OutLen) || (Hdr1+1==Hdr2) ) FrameFlush();
//   while( (Hdr2!=Hdr1 && OutPtr/8+512+1024+Hdr[Hdr2].MSize<OutLen+1) || (Hdr1+1==Hdr2) ) FrameFlush();
   while( (Hdr2!=Hdr1 && OutPtr/8+8192<OutLen+1) || (Hdr1+1==Hdr2) ) FrameFlush();

   if( OutLen<BufSiz ) {
     for( i=0; i<BufSiz; i++ ) Out[i+BufSiz]=Out[i];
     OutPtr += 8*BufSiz;
     OutLen += BufSiz;
   }

   wframe1=0;

   return 0;
 }
 
 void GarbageFlush( void ) {
   int i, tmp = OutPtr;
   if( GrbLen>0 ) {
     OutPtr+=GrbLen;
     for( i=0; i*8<GrbLen-8; i++ ) putbits( Garbage[i], 8 );
     putbits( (Garbage[i]>>(8-GrbLen&7)), (GrbLen-i*8) );
   }
   OutPtr = tmp;
 }

 void FrameFlush( void ) {
   int i;
   FW++;
//   for( i=0; i<Hdr[Hdr2].HSize; i++ ) glen?(putc( Hdr[Hdr2].h[i], g ), glen--):0;
//   for( i=0; i<Hdr[Hdr2].MSize; i++ ) glen?(putc( Out[OutLen-i], g ),  glen--):0;

   uint framesize = Hdr[Hdr2].HSize + Hdr[Hdr2].MSize;

   if( outpos+framesize<=outlen ) {
     for( i=0; i<Hdr[Hdr2].HSize; i++ ) put(Hdr[Hdr2].h[i]);
     for( i=0; i<Hdr[Hdr2].MSize; i++ ) put(Out[OutLen-i]);
     outpos += framesize;
   } else {
     uint glen=outlen-outpos;
     for( i=0; i<Hdr[Hdr2].HSize; i++ ) if( glen>0 ) { put(Hdr[Hdr2].h[i]); glen--; }
     for( i=0; i<Hdr[Hdr2].MSize; i++ ) if( glen>0 ) { put(Out[OutLen-i]); glen--; }
     outpos = outlen;
   }

   OutLen -= Hdr[Hdr2].MSize;
   Hdr2++;
 }

 void WriteFlush( void ) {
   if( Hdr1!=Hdr2 ) GarbageFlush();
   while( Hdr1!=Hdr2 ) FrameFlush();
 }
