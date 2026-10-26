
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
