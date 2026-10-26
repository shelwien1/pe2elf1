
int Enc_MDCT( short is[576], gr_info& gi, int OutBorder ) {

  int i, t;
  int sfreq        = h.frequency + ((h.ID==0)+(h.IDex==0))*3;
  int bigvalues    = gi.big_values * 2;
  int region1Start = __min(bigvalues, bandInfo[sfreq][gi.region0_count+1]);
  int region2Start = __min(bigvalues, bandInfo[sfreq][__min(22,gi.region0_count+gi.region1_count+2)] );

  t = gi.table_select[0]; if(t)
  for( i=0;            OutPtr>OutBorder && i<region1Start; i+=2 ) enc_huffman( t, is[i], is[i+1] );
  t = gi.table_select[1]; if(t) 
  for( i=region1Start; OutPtr>OutBorder && i<region2Start; i+=2 ) enc_huffman( t, is[i], is[i+1] );
  t = gi.table_select[2]; if(t)
  for( i=region2Start; OutPtr>OutBorder && i<bigvalues;    i+=2 ) enc_huffman( t, is[i], is[i+1] );

  t = 32 + gi.count1table_select;
  for( i=bigvalues;    OutPtr>OutBorder && i<=576-4; i+=4 ) {   
    int p = c1tab[t-32][is[i]&3][is[i+1]&3][is[i+2]&3][is[i+3]&3];
    putbits( p&1023, p>>10 );
  }

  while( OutPtr>OutBorder ) putbits( 1, 1 );

  OutPtr=OutBorder;

  ( ((uint&)Out[(OutPtr-24)>>3])&=~(0xFFFFFF<<(OutPtr&7)) );

  return 0;
}


void enc_huffman( int table, int x, int y )
{
  const hufproptab& h = htprop[table];
  const uint* ht   = huff_ctab[table];
  const byte* hl   = huff_hlen[table];

  uint signx = (x<=0) ? (x=-x, 1) : 0;
  uint signy = (y<=0) ? (y=-y, 1) : 0;
  uint idx   = __min(15,x) * h.ylen + __min(15,y);
  putbits( ht[idx], hl[idx] );

  if( x>=15 ) putbits( x-15, h.linbits );
  if( x ) putbits( signx, 1 );
  if( y>=15 ) putbits( y-15, h.linbits );
  if( y ) putbits( signy, 1 );   
}


