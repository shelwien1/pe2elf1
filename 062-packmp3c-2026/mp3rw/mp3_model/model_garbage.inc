
#define bit1l 128
#define bit1x 8
#define bit1y 256

Node2ii bit1[bit1l][bit1x][bit1y]; 
int ctx1;
int ctx3; // len sum

// max frame length is about 1440 bytes, so ancillary len needs at least 11 bits
Number1ii<11> x_datlen;

Node2ii f_datlen;

void Init_Ancillary( void ) {
  Init_Number1ii(x_datlen);

  Init_Node2ii( bit1 );
  Init_Node2ii( f_datlen );

  prev_p23sum = last_p23sum = -1;
  ctx3=0;
}

void ProcessAncillary( void ) {
  int i,j,c,d;
  int ctx1=0; // row context
  RangeCoder& rc = *this;

  int p_mdb = GrbPrev;//(rc.ProcMode?GrbPtrW:GrbPtrR);
  int flag = (GrbLen==p_mdb); // && (fstframe()==0);
#ifndef DISABLE_fdatlen
  f_datlen.Process( rc, flag, M_f_datlen_wr, M_f_datlen_mw );
#endif
  if( flag ) {
    GrbLen = p_mdb;
  } else {

    flag = (GrbLen != (GrbLen/8*8 + ((8-prev_p23sum)&7)));
#ifndef DISABLE_fgrblen
    rc.BProcess( SCALE-2, flag );
#endif
    if( flag ) {
#ifndef DISABLE_NGrbLen
      rc.NProcess( GrbLen, 16, 0, 1<<16 );
#endif
    } else {
      j = GrbLen/8;
#ifndef DISABLE_xdatlen
      x_datlen.Process( rc, j, -1, 1442+512, M_x_datlen_wr, M_x_datlen_mw );
#endif
      j = j*8 + ((8-prev_p23sum)&7);
      GrbLen = j;
    }

  }

  prev_p23sum = last_p23sum;
  if( GrbLen>=4096*8 ) BadData(); // (a broken stream: Garbage holds no more)

  if( GrbLen>0 ) {

    int jlast = (GrbLen+7)/8;

    for( j=0; j<jlast; j++ ) {
      
      int ilast = (j*8+8>GrbLen) ? (GrbLen-j*8) : 8;
      int k     = (j<bit1l) ? j : bit1l-1;
      c = Garbage[j];
      int cc=0;
     
      for( i=0; i<ilast; i++ ) {

        d = (c>>(7-i))&1;

        Node2ii& grb_b1 = bit1[k][i][ (ctx1&(bit1y/2-1)) + (ctx3&1)*bit1y/2 ];
        if( (grb_b1.T==1) && (j>0) ) grb_b1 = (&grb_b1)[-bit1x*bit1y];

#ifndef DISABLE_grb_b1
        grb_b1.Process( rc, d, M_grb_b1_wr, M_grb_b1_mw );
#endif
        cc |= (d<<(7-i));

        ctx1+=ctx1 + d;
      }

      Garbage[j]=cc;
    }

    ctx3 += GrbLen;
  } // if

}

#undef bit1l
#undef bit1x
#undef bit1y
