
 word CalcCRC( byte *p, int l )
 {
   int i,j,data;
   int crc = 0xFFFF;
   for( i=l-1; i>=0; i-- )
   {
     data = p[i]<<8;
     for( j=8; j; j-- ) {
       crc <<= 1;
       data<<= 1;
       if( (data^crc)&0x10000 ) crc^=0x8005;
     }
   }
   return crc&0xFFFF;
 }

 int ReadFrame( void ) {

   int BufPtrSaved = BufPtr;
   int retval, i, j, hsize, SavPtr;
   ProcMode = 1;

   // 1 = break on read error (by break)
   // 0 = finished frame decoding ("loop" end reached ok)   
   
   for( retval=1; retval>0;  ) {

     if( retval==2 ) { 
       BufPtr=BufPtrSaved;
       sinf+=BufLen/8; GarbageStore(0); sinf-=BufLen/8; 
       BufPtr = (BufLen-=8);
       retval = BufPtr>=0 ? -256 + sinf[BufPtr/8] : 1;
       break;
     };
     
     retval=2;
     if( ReadLoEnd(4) ) continue; 

     (uint&)h = (uint&)sinf[(BufPtr-=32)/8];

     if( h.syncword!=2047 ) continue;     
     if( h.ID==1 && h.IDex==0 ) continue; 
     if( h.layer!=1 ) continue;           
     if( h.bitrate==0 || h.bitrate==15 ) continue; 
     if( h.frequency==3 ) continue;       

//     pos = ftell(f) - (BufPtr+32)/8;

     int chgr = (h.mode!=3)*2 + h.ID;
     int stl  = 4;     

     if( h.protection==0 ) {
       if( ReadLoEnd(2) ) continue;
       CRC = (word&)sinf[(BufPtr-=16)/8];
       stl   += 2;
     }

     if( ReadLoEnd(l_si[chgr]) ) continue;
     stl += l_si[chgr]; 
     hsize = stl;

     if( h.protection==0 ) {
       (word&)sinf[BufPtr/8] = (word&)h;
       cCRC = CalcCRC( &sinf[BufPtr/8-l_si[chgr]], l_si[chgr]+2 );
       (word&)sinf[BufPtr/8] = CRC;
       
       if( CRC!=cCRC ) continue;
     }


     if( ProcSideInfo() ) continue;

     j = BufPtr>>3;

     
     stl=(h.ID?1152:576)/8*bitrat[h.ID][h.bitrate]/spfreq[h.ID][h.IDex][h.frequency]+h.padding-hsize;

     for( i=j-1; i>=0; i-- ) sinf[hsize+i]=sinf[i]; 
     sinf+=hsize;


     ReadLoEnd( stl, 0 ); 

     
     BufPtr += 8*si.main_data_begin;
     SavPtr = BufPtr; 


     memset( &MC,0,sizeof(MC) );

     int mdb_err_flag = (si.backup_mdb>0 && rframe1); 
     mdb_err_flag |= (si.p23sum>(stl+si.main_data_begin)*8); 
     mdb_err_flag |= (GrbPtr>=0 && (stl*8+GrbPtr)<BufPtr); 
     if( mdb_err_flag==0 ) mdb_err_flag = (h.ID ? ProcScaleFactors1() : ProcScaleFactors2());

     if( mdb_err_flag ) {
       BufPtr = SavPtr-8*si.main_data_begin;
       SavPtr = BufPtr;
       si.main_data_begin = 0;
       for( i=0; i<4; i++ ) {
         gr_info& gi = si.gi[0][i];
         gi.backup_p23=gi.part2_3_length;
         gi.part2_3_length = 0;
       }
       si.p23cleared = 1; 
       si.p23sum     = 0;
       BufPtr = SavPtr;
     }

     j=BufPtr;
     BufPtr=SavPtr; 
     GarbageStore(stl);
     BufPtr=j;    

     GrbPtr = BufPtr; 

     BufLen=BufPtr=0; 

     retval=0;
     rframe1=0;
   }

   if( retval<0 ) rframe1=-1;

   return retval;
 }

 void GarbageStore( int stl ) {
   int tmp,i;
   if( GrbPtr>=0 ) {
     tmp = BufPtr;     
     for( BufPtr=stl*8+GrbPtr,i=0; BufPtr>=tmp; i++ ) procbits( Garbage[i], 8 );
//     putbits( (Garbage[i]>>(8-GrbLen&7)), (GrbLen-i*8) );
     BufPtr = tmp;
     GrbLen = stl*8+GrbPtr-BufPtr;
   }
   GrbPtr=-1;
 }

 void ReadFlush( void ) {
   GarbageStore(0);
 }
