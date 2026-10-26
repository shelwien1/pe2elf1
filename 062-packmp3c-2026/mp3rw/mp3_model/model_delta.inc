
byte band[576];

byte InitBand( byte* band ) {
  static const int x[] = {4,8,12,16, 20,24,30,36, 44,52,62,74, 90,110,134,162, 196,238,288,342, 576};
  int i,j;
  for( i=0,j=0; i<21; i++ ) for( ;j<x[i]; j++ ) band[j]=i;
  return band[0];
}

// The long-block scalefactor band of each place, in each of bandInfo's
// tables (sfreq: as model_delta_regions.inc has it).  band[] above is the
// 44.1 kHz table at every rate: its bands still pick the model's contexts,
// but the scalefactor of a place (curband) and the mixing stage's band and
// quantizer steps come from the frame's own table.
byte sfbOf[9][576];
// the quantizer step of each place in this granule and channel (the mixing
// stage's rescaled reference, IDX/sh_model.inc), in units of 1.5 dB
short xStep[576];

void InitSfb( void ) {
  for( int s=0; s<9; s++ ) for( int b=0; b<22; b++ )
    for( int j=bandInfo[s][b]; j<bandInfo[s][b+1]; j++ ) sfbOf[s][j] = b;
}

//  |x| = |q|^(4/3) * 2^(step/4): global_gain less the band's scalefactor,
//  with MPEG-1's pretab where preflag is set (in MPEG-2 preflag comes from
//  scalefac_compress and is not part of what is coded here, so it is
//  left out).  Short blocks get global_gain alone.
void CalcSteps( const gr_info& gi, const ScaleFac& sf, const byte* sfb ) {
  int lng = (gi.block_type!=2), pre = (h.ID && gi.preflag);
  for( int j=0; j<576; j++ ) {
    int b = sfb[j], s = (lng && b<21) ? sf.l[b] + (pre ? cm_pretab[b] : 0) : 0;
    xStep[j] = short( gi.global_gain - 210 - 2*(1+gi.scalefac_scale)*s );
  }
}

int CalcTableSelect( short* MDCT, int x, int y ) {

static const byte TablesToCheck[][3] =
  {
   {  0, 0, 0 },   //  0  
   {  1, 0, 0 },   //  1
   {  2, 3, 0 },   //  2
   {  5, 6, 0 },   //  3  
   {  7, 8, 9 },   //  4  
   {  7, 8, 9 },   //  5  
   { 10,11,12 },   //  6  
   { 10,11,12 },   //  7  
   { 13,15, 0 },   //  8  
   { 13,15, 0 },   //  9
   { 13,15, 0 },   // 10
   { 13,15, 0 },   // 11
   { 13,15, 0 },   // 12  
   { 13,15, 0 },   // 13  
   { 13,15, 0 },   // 14  
   { 13,15, 0 },   // 15  
   { 16,24, 0 },   // 16  
   { 17,24, 0 },   // 17  
   { 18,24, 0 },   // 18  
   { 19,24, 0 },   // 19  
   { 20,25, 0 },   // 20  
   { 20,26, 0 },   // 21
//   { 21,28, 0 },   // 22
   { 21,28, 0 },   // 23  
   { 22,29, 0 },   // 24  512
   { 22,30, 0 },   // 25  1024
   { 23,30, 0 },   // 26  2048
   { 23,31, 0 },   // 27  4096
   { 23,31, 0 },   // 28  8192
   { 23,31, 0 }    // 28  8192
  };

  int i,j,k,l, e,d,c;

  for( j=x,d=0; j<y; j++ ) d=__max(d,l_abs(MDCT[j]));
  d = (d<16) ? d : 16+log2i(l_abs(d-15))+1-1;
  d = Min<int>( d, 28 ); // (more only in a broken stream: |x| is 8206 at most)
//  d = (d<16) ? d : 16+ilog2(d-15)-1;

  c = 0; e=99999;
  for( j=0; j<3; j++ ) {   
    i = TablesToCheck[d][j];
    if( i==0 ) break;
    for( k=x,l=0; k<y; k+=2 ) l+=len_huffman( i,MDCT[k],MDCT[k+1] );
    if( l<e ) e=l, c=i;
  }

  return c;
}


void Init_Delta( void ) {
  InitBand(band);
  InitSfb();
  Init_Counts();
  Init_Regions();
  for( int ch=0; ch<8; ch++ ) m_rDelta[ch].Init_RangeDelta(ch);
}

void ProcessDelta( void ) {

  int gr,ch;
  int ngr = h.ID;
  int stereo = (h.mode!=3)+1;
  RangeCoder& rc = *this;

  int st_mod = 0;
  if( h.mode==1 ) { //MPG_MD_JOINT_STEREO
    if( h.mode_ext!=0 ) st_mod = 2; // intensity или перемешанные блоки
    if( h.mode_ext==2 ) st_mod = 1; // только ms stereo
  }

  int ii[2][2];
  for( gr=0; gr<=ngr; gr++ ) {
    for( ch=0; ch<stereo; ch++ ) {
      gr_info& gi = si.gi[gr][ch];
//      st_mod==0  -> bt
//      st_mod==1  -> 6*ch + bt
//      st_mod==2  -> 2*bt + ch + 2
      int      bt = ( gi.block_type==2 );
      int i = ii[gr][ch] = (st_mod!=2) ? (st_mod==1 && ch==1) ? 6+bt : bt : 2*bt+ch+2; 
//      if( m_rDelta[i]==0 ) m_rDelta[i] = new RangeDelta(i);
    }
  }

  for( gr=0; gr<=ngr; gr++ ) for( ch=0; ch<stereo; ch++ ) ProcessCounts( ii[gr][ch], gr, ch );

  int ctext[2][2] = {0,0,0,0};
  int  ctx1[2][2] = {0,0,0,0};
  int  ctx2[2][2] = {0,0,0,0};
  int  ctx3[2][2] = {0,0,0,0};
  int  dtx1[2][2] = {0,0,0,0};

  int j;
  const byte* sfb = sfbOf[ h.frequency + ((h.ID==0)+(h.IDex==0))*3 ];

  // rate-control disabled
  for( gr=0; gr<=ngr; gr++ ) {
    for( ch=0; ch<stereo; ch++ ) {
      int i = ii[gr][ch], j;
      CalcSteps( si.gi[gr][ch], SF[gr][ch], sfb );
      m_rDelta[i].xstp = xStep; m_rDelta[i].xsfb = sfb;
      for( j=0; j<576; j++ ) {
        if( j<cc0[gr][ch] ) {
          int curband = ((i==0) || (i==2) || (i==3) || (i==6) ) ? (sfb[j]<21 ? SF[gr][ch].l[ sfb[j] ] : 0) : 0;
          m_rDelta[i].ModelMDCT( rc, MC[gr][ch][j], si.gi[gr][ch], curband, band[j], cc0[gr][ch], cc1[gr][ch], ch, j,
                                  ctext[gr][ch], ctx1[gr][ch], ctx2[gr][ch], ctx3[gr][ch], dtx1[gr][ch] );
        } else {
          m_rDelta[i].ShiftMDCT(j,0);
        }
      }
    }
  }

  for( gr=0; gr<=ngr; gr++ ) for( ch=0; ch<stereo; ch++ ) ProcessRegions( gr, ch );

}




