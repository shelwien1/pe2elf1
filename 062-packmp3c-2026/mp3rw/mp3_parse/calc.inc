
int CalcHeaderLen( void ) {
  int l = 4+(h.protection?0:2)+l_si[(h.mode!=3)*2+h.ID];
  return l;
}

int CalcFrameLen( void ) {
  int l = (h.ID?1152:576)/8*bitrat[h.ID][h.bitrate]/spfreq[h.ID][h.IDex][h.frequency]+h.padding;
  return l;
}

int CalcPart23Total( void ) {
  int l = 0;
  int ch,gr;
  int ngr = h.ID;
  int nch = (h.mode!=3);
  for( gr=0; gr<=ngr; gr++ ) for( ch=0; ch<=nch; ch++ )
    l += si.gi[gr][ch].part2_3_length;
  return l;    
}

int CalcGarbageLen( void ) {
  int l;
  l = (CalcFrameLen()-CalcHeaderLen()); 
  l+= si.main_data_begin;               
  l = (l*8 - CalcPart23Total());        
  return l;
}

// layer I/II

int CalcFrameLenL12( void ) {
  int br = l12_bitrat[h.ID][h.layer==2][h.bitrate]*1000;
  int fs = spfreq[h.ID][h.IDex][h.frequency];
  return (h.layer==3) ? (12*br/fs+h.padding)*4 : 144*br/fs+h.padding;
}

// layer II allocation table, chosen as in the ISO reference code
int L2Table( void ) {
  if( h.ID==0 ) return 4;
  int br = l12_bitrat[1][1][h.bitrate]/((h.mode!=3)+1); // per channel
  int fs = spfreq[1][1][h.frequency];
  if( (fs==48000 && br>=56) || (br>=56 && br<=80) ) return 0;
  if( fs!=48000 && br>=96 ) return 1;
  if( fs!=32000 && br<=48 ) return 2;
  return 3;
}

int L12Bound( int sblimit ) {
  return Min<int>( (h.mode==1) ? (h.mode_ext+1)*4 : 32, sblimit );
}

// bits of the allocation field of subband sb
int L12Nbal( int tab, int sb ) {
  return (h.layer==3) ? 4 : l2_rows[l2_row[tab][sb]][0];
}

// bits of all samples of one subband and channel with allocation b
int L12SmpBits( int tab, int sb, int b ) {
  if( b==0 ) return 0;
  if( h.layer==3 ) return 12*(b+1);
  int q = l2_qbits[ l2_rows[l2_row[tab][sb]][b] ];
  return 12*( (q<0) ? -q : 3*q );
}

// header, CRC and audio data bits, from the allocation and scfsi in L
int CalcL12Bits( void ) {
  int tab = (h.layer==3) ? 0 : L2Table();
  int sblimit = (h.layer==3) ? 32 : l2_sblimit[tab];
  int bound = L12Bound( sblimit ), nch = (h.mode!=3)+1;
  int n = 32 + (h.protection?0:16), sb, ch, m;
  for( sb=0; sb<sblimit; sb++ ) {
    m = (sb<bound) ? nch : 1;
    n += m*L12Nbal( tab, sb );
    for( ch=0; ch<nch; ch++ ) if( L.alloc[ch][sb] ) n += (h.layer==3) ? 6 : 2+6*l2_nscf[L.scfsi[ch][sb]];
    for( ch=0; ch<m; ch++ ) n += L12SmpBits( tab, sb, L.alloc[ch][sb] );
  }
  return n;
}
