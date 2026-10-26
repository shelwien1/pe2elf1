
int len_huffman( int table, int x, int y ) {
  uint cbits=0;
  if( (table!=4) && (table!=14) ) {
    hufproptab& h = (hufproptab&)htprop[table];
    if( x<=0 ) x=-x;
    if( y<=0 ) y=-y;
    if( x>=15 ) cbits+=h.linbits, x=15;
    if( y>=15 ) cbits+=h.linbits, y=15;
    cbits += huff_hlen[table][x*h.ylen + y] + (x>0) + (y>0);
  }
  return cbits;
}

int Len_MDCT( short (&is)[576], gr_info& gi ) {

  int i, k, t;
  int bits=0;

  for( k=576; k>0 && (is[k-1]==0); k-- ); // count rzero
  int rzero = k;

  int sfreq        = h.frequency + ((h.ID==0)+(h.IDex==0))*3;
  int bigvalues    = gi.big_values * 2;
  int region1Start = Min<int>(bigvalues, bandInfo[sfreq][gi.region0_count+1]);
  int region2Start = Min<int>(bigvalues, bandInfo[sfreq][Min<int>(22,gi.region0_count+gi.region1_count+2)] );

  t = gi.table_select[0]; if(t) for( i=0;            i<region1Start; i+=2 ) bits+=len_huffman(t,is[i],is[i+1]);
  t = gi.table_select[1]; if(t) for( i=region1Start; i<region2Start; i+=2 ) bits+=len_huffman(t,is[i],is[i+1]);
  t = gi.table_select[2]; if(t) for( i=region2Start; i<bigvalues;    i+=2 ) bits+=len_huffman(t,is[i],is[i+1]);

  t = 32 + gi.count1table_select;
  // (a quadruple goes past 576 only in a broken stream: those lines are 0)
  for( i=bigvalues; i<rzero; i+=4 ) bits += c1tab[t-32][is[i]&3][(i+1<576?is[i+1]:0)&3][(i+2<576?is[i+2]:0)&3][(i+3<576?is[i+3]:0)&3]>>10;

  return bits;
}
