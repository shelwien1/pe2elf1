
#define ProcMDCT(a,b,c) (ProcMode?Dec_MDCT(a,b,c):Enc_MDCT(a,b,c))

#define hgetbits(l) getbits(sinf,l)

int Dec_MDCT( short (&is)[576], gr_info& gi, int BufBorder ) {

  int i, t, error=0;
  int sfreq        = h.frequency + ((h.ID==0)+(h.IDex==0))*3;
  int bigvalues    = gi.big_values * 2;
  int region1Start = __min(bigvalues, bandInfo[sfreq][gi.region0_count+1]);
  int region2Start = __min(bigvalues, bandInfo[sfreq][__min(22,gi.region0_count+gi.region1_count+2)] );

  uint bbtemp = ((uint&)sinf[(BufBorder-24)>>3]);
  ( ((uint&)sinf[(BufBorder-24)>>3])|=(0xFFFFFF<<(BufBorder&7)) );

  memset( is, 0, sizeof(is) );
   
  t = gi.table_select[0]; if(t)
  for( i=0;            BufPtr>BufBorder && i<region1Start; i+=2 ) error |= dec_huffman(t, &is[i] );
  t = gi.table_select[1]; if(t) 
  for( i=region1Start; BufPtr>BufBorder && i<region2Start; i+=2 ) error |= dec_huffman(t, &is[i] );
  t = gi.table_select[2]; if(t)
  for( i=region2Start; BufPtr>BufBorder && i<bigvalues;    i+=2 ) error |= dec_huffman(t, &is[i] );

  t = 32 + gi.count1table_select;
  for( i=bigvalues; BufPtr>BufBorder && i<=576-4; i+=4 ) error |= dec_huffman32(t, &is[i] );

  while( BufPtr>BufBorder ) if( getbits(sinf,1)!=1 ) error=1;

  BufPtr=BufBorder; // there can be some padding %-|
  ((uint&)sinf[(BufBorder-24)>>3]) = bbtemp;

  return error;

}

int dec_huffman( int table, short* xx ) {

  int i, cc, ci, val, x, y;

  const hufproptab& h = htprop[table];
  const uint* hd      = huff_dtab[table];
  const byte* hl      = huff_hlen[table];
 
  if( h.xlen==0 ) { xx[0]=xx[1]=0; return 0; } // table 0 needs no bits

  for( i=0,cc=1;; i++ ) {
    ci = (hd[i]>>8);
    if( ci==0 ) return 1;
    while( cc<ci ) cc+=cc+hgetbits(1);
    if( cc==ci ) break;
  }

  val = hd[i]&255;
  x = val>>4;
  y = val&15;

// Process sign and escape encodings for dual tables.
  if( h.linbits ) if( x+1==h.xlen ) x+=hgetbits(h.linbits);
  if( x ) if( hgetbits(1)==1 ) x=-x;

  if( h.linbits ) if( y+1==h.ylen ) y+=hgetbits(h.linbits);
  if( y ) if( hgetbits(1)==1 ) y=-y;

  xx[0]=x; xx[1]=y;

  return 0;  
}

int dec_huffman32( int table, short* x ) {

  int i, cc, ci, val;

  if( table==32 ) {
    static byte tt[] = {2,1,4,8, 9,6,3,10,12, 11,15,13,14,7,5 };
    val = hgetbits(1) ? 0 : 
          (cc=hgetbits(3))>=4 ? tt[cc-4] :
          (cc+=cc+hgetbits(1))>=3 ? tt[cc-3+4] : 
          tt[cc+cc+hgetbits(1)+4+5];
  } else {
    val = 15-hgetbits(4);
  }

  x[0] = (val&8) ? hgetbits(1) ? -1 : 1 : 0;
  x[1] = (val&4) ? hgetbits(1) ? -1 : 1 : 0;
  x[2] = (val&2) ? hgetbits(1) ? -1 : 1 : 0;
  x[3] = (val&1) ? hgetbits(1) ? -1 : 1 : 0;

  return 0;  

}
