
// #define DISABLE_Sm0A     
// #define DISABLE_Sm0B     
// #define DISABLE_Sm1      
// #define DISABLE_Sm2      
// #define DISABLE_Sm3      
// #define DISABLE_Sm4      
// #define DISABLE_Smx      
// #define DISABLE_bignum   
// #define DISABLE_Sz512    
// #define DISABLE_Sz256    
// #define DISABLE_NProc    
// #define DISABLE_Smant    
// #define DISABLE_Smbnt    
// #define DISABLE_Sms      
// #define DISABLE_Sms1     













// the mixing stage (XC): the counters of its contexts xa..xd, XND nodes a
// row; the mixer's weight sets and their update counts, one set for each
// of the mixer contexts of mp3c's magnitude decisions (mm0A_g .. mmx_g,
// whose mixers the stage replaced), node after node


#define SC8(x,y) ((x*y)>>8)
/*	
  SSEj<*(int*)& M_Sm0A_Q,*(int*)& M_Sm0A_d_Volume> Sm0A;
  SSEj<*(int*)& M_Sm0B_Q,*(int*)& M_Sm0B_d_Volume> Sm0B;

  SSEj<*(int*)& M_Sm1_Q,*(int*)& M_Sm1_d_Volume> Sm1;
  SSEj<*(int*)& M_Sm2_Q,*(int*)& M_Sm2_d_Volume> Sm2;
  SSEj<*(int*)& M_Sm3_Q,*(int*)& M_Sm3_d_Volume> Sm3;
  SSEj<*(int*)& M_Sm4_Q,*(int*)& M_Sm4_d_Volume> Sm4;
  SSEj<*(int*)& M_Smx_Q,*(int*)& M_Smx_d_Volume> Smx;

  SSEj<*(int*)& M_Sz256_Q,*(int*)& M_Sz256_d_Volume> Sz256;
  SSEj<*(int*)& M_Sz512_Q,*(int*)& M_Sz512_d_Volume> Sz512;
  SSEj<*(int*)& M_Smant_Q,*(int*)& M_Smant_e_Volume> Smant;
  SSEj<*(int*)& M_Smbnt_Q,*(int*)& M_Smbnt_e_Volume> Smbnt;

  SSEj<*(int*)& M_Sms_Q,*(int*)& M_Sms_d_Volume> Sms;
  SSEj<*(int*)& M_Sms1_Q,*(int*)& M_Sms1_d_Volume> Sms1;
*/



  uint  signs[576];
  uint  zeros[576];
//  uint  z512s[576];
//  uint  plus1[576];
//  uint  m1nus[576];

  int cMDCT[576];
  int dMDCT[576];
  int eMDCT[576];

  int av_MDCT[576];

  int si; // stats instance index

  // j_band's thresholds of a place (ModelMDCT adds j>=c1), made by InitData
  byte jband[576];

  // the quantizer step each place's last value (cMDCT) was coded at, and
  // the granule's: its steps and scalefactor bands (ProcessDelta sets them)
  short cStep[576];
  const short* xstp;
  const byte*  xsfb;

// the tables' starting state, after M_Init() made them
void M_InitData( void ) {
  int i;
//    si = instance;

  for( i=0; i<576; i++ ) av_MDCT[i]=eMDCT[i]=dMDCT[i]=cMDCT[i]=signs[i]=zeros[i]=0; //plus1[i]=m1nus[i]=0;z512s[i]=

  for( i=0; i<M_pm0A_f_Volume; i++ ) pm0A[i]=0;
  for( i=0; i<M_m0A_e_Volume; i++ )  m0A[i].Init_Node2i();
  for( i=0; i<M_Sm0A_d_Volume; i++ ) Sm0A[i].Init_SSEi(M_Sm0A_wi,M_Sm0A_ti);
  for( i=0; i<M_n0A_e_Volume; i++ )  n0A[i].Init_Node2i();

  for( i=0; i<M_pm0B_f_Volume; i++ ) pm0B[i]=0;
  for( i=0; i<M_m0B_e_Volume; i++ )  m0B[i].Init_Node2i();
  for( i=0; i<M_Sm0B_d_Volume; i++ ) Sm0B[i].Init_SSEi(M_Sm0B_wi,M_Sm0B_ti);
  for( i=0; i<M_n0B_e_Volume; i++ )  n0B[i].Init_Node2i();

  for( i=0; i<M_pm1_f_Volume; i++ ) pm1[i]=0;
  for( i=0; i<M_m1_e_Volume; i++ )  m1[i].Init_Node2i();
  for( i=0; i<M_Sm1_d_Volume; i++ ) Sm1[i].Init_SSEi(M_Sm1_wi,M_Sm1_ti);
  for( i=0; i<M_n1_e_Volume; i++ )  n1[i].Init_Node2i();

  for( i=0; i<M_pm2_f_Volume; i++ ) pm2[i]=0;
  for( i=0; i<M_m2_e_Volume; i++ )  m2[i].Init_Node2i();
  for( i=0; i<M_Sm2_d_Volume; i++ ) Sm2[i].Init_SSEi(M_Sm2_wi,M_Sm2_ti);
  for( i=0; i<M_n2_e_Volume; i++ )  n2[i].Init_Node2i();

  for( i=0; i<M_pm3_f_Volume; i++ ) pm3[i]=0;
  for( i=0; i<M_m3_e_Volume; i++ )  m3[i].Init_Node2i();
  for( i=0; i<M_Sm3_d_Volume; i++ ) Sm3[i].Init_SSEi(M_Sm3_wi,M_Sm3_ti);
  for( i=0; i<M_n3_e_Volume; i++ )  n3[i].Init_Node2i();

  for( i=0; i<M_pm4_f_Volume; i++ ) pm4[i]=0;
  for( i=0; i<M_m4_e_Volume; i++ )  m4[i].Init_Node2i();
  for( i=0; i<M_Sm4_d_Volume; i++ ) Sm4[i].Init_SSEi(M_Sm4_wi,M_Sm4_ti);
  for( i=0; i<M_n4_e_Volume; i++ )  n4[i].Init_Node2i();

  for( i=0; i<M_mx_e_Volume; i++ )  mx[i].Init_Node2i();
  for( i=0; i<M_Smx_d_Volume; i++ ) Smx[i].Init_SSEi();
  for( i=0; i<M_nx_e_Volume; i++ )  nx[i].Init_Node2i();

  for( i=0; i<M_pms1_f_Volume; i++ ) pms1[i]=0;
  for( i=0; i<M_ms1_e_Volume; i++ )  ms1[i].Init_Node2i();
  for( i=0; i<M_Sms1_d_Volume; i++ ) Sms1[i].Init_SSEi(M_Sms1_wi,M_Sms1_ti);
  InitMixers( mms1, M_mms1_g_Volume, M_mms1_W0 );
  for( i=0; i<M_ns1_e_Volume; i++ )  ns1[i].Init_Node2i();

  for( i=0; i<M_pms_f_Volume; i++ ) pms[i]=0;
  for( i=0; i<M_ms_e_Volume; i++ )   ms[i].Init_Node2i();
  for( i=0; i<M_Sms_d_Volume; i++ )  Sms[i].Init_SSEi(M_Sms_wi,M_Sms_ti);
  InitMixers( mms, M_mms_g_Volume, M_mms_W0 );
  for( i=0; i<M_ns_e_Volume; i++ )  ns[i].Init_Node2i();

  for( i=0; i<M_z512_e_Volume; i++ ) z512[i].Init_Node2i();
  for( i=0; i<M_Sz512_d_Volume; i++ ) Sz512[i].Init_SSEi(M_Sz512_wi,M_Sz512_ti);

  for( i=0; i<M_mant_f_Volume; i++ ) mant[i].Init_Node2i();
  for( i=0; i<M_Smant_e_Volume; i++ ) Smant[i].Init_SSEi(M_Smant_wi,M_Smant_ti);

//  for( i=0; i<M_mbnt_f_Volume; i++ ) mbnt[i].Init_Number2i();
//  for( i=0; i<M_Smbnt_e_Volume; i++ ) Smbnt[i].Init_SSEi(M_Smbnt_wi,M_Smbnt_ti);

  for( i=0; i<M_z256_e_Volume; i++ ) z256[i].Init_Node2i();
  for( i=0; i<M_Sz256_d_Volume; i++ ) Sz256[i].Init_SSEi(M_Sz256_wi,M_Sz256_ti);

  // the mixing stage: counters at 1/2, each weight set taking mp3c's two
  // predictions half and half (M_xwa, M_xwb)
  for( i=0; i<M_xa_Volume*XND; i++ ) xA[i].Init();
  for( i=0; i<M_xb_Volume*XND; i++ ) xB[i].Init();
  for( i=0; i<M_xc_Volume*XND; i++ ) xC[i].Init();
  for( i=0; i<M_xd_Volume*XND; i++ ) xD[i].Init();
  {
    long long v[XND] = { M_mm0A_g_Volume, M_mm0B_g_Volume, M_mm1_g_Volume, M_mm2_g_Volume,
                         M_mm3_g_Volume, M_mm4_g_Volume, M_mmx_g_Volume };
    for( int n=0; n<XND; n++ ) xWo[n] = n ? xWo[n-1] + v[n-1] : 0;
  }
  for( i=0; i<xWo[XND-1]+M_mmx_g_Volume; i++ ) {
    for( int k=0; k<XNIN; k++ ) xW[i*XNIN+k] = k==0 ? M_xwa : k==10 ? M_xwb : 0;
    xWN[i] = 0;
  }
  for( i=0; i<576; i++ ) cStep[i]=0;
  for( i=0; i<576; i++ ) jband[i] = (i>M_Sms1_j1) + (i>M_Sms1_j2) + (i>M_Sms1_j3) + (i>M_Sms1_j4) + (i>M_Sms1_j5) + (i>M_Sms1_j6) + (i>M_Sms1_j7) + (i>M_Sms1_j8);
  xstp = 0; xsfb = 0;
}

// The mixing stage (freq_cm.inc: cm_code): codes the decision bit at node
// n -- 0 and 1 the zero flags of the big_values and the count1 region,
// 2..5 |v|==1 to |v|==4, 6 the |v|==x of the loop after them -- in place
// of mp3c's own two-input mixer there: mp3c's two predictions (of a 0, 15
// bits), D's and the SSE's, are mixed with the four counters of this
// place's rows by the weight set g of the node (mp3c's mixer context).
// The rows are ModelMDCT's, made once a place.
int xrA, xrB, xrC, xrD;
long long xWo[XND]; // the nodes' first weight sets

void XC( RangeCoder& rc, int n, int pd, int ps, int& bit, int g ) {
  cm_code( rc, bit, pd, ps, xA[xrA+n], xB[xrB+n], xC[xrC+n], xD[xrD+n],
           &xW[(xWo[n]+g)*XNIN], xWN[xWo[n]+g], M_xlim, M_xmw, M_xsh, M_xlr, M_xmb );
}

void Init_RangeDelta( int instance ) {
  M_Init();
  M_InitData();
  si = instance;
}

void ModelMDCT( 
  RangeCoder& rc, short& MDCT_j, gr_info& gi, int curband, int band_j, 
  int k, int c1, int ch, int j,
  int& ctext, int& ctx1, int& ctx2, int& ctx3, int& dtx1 
) {

  int i, a,c,d,e,f,l,ll,s,x,flag,branch;
  int aext, exA,exB, av_total;

  c = MDCT_j;
  int cMDCTjm1 = dtx1; //dMDCT[j-1];
  int cMDCTj   = cMDCT[j];
  int dMDCTj   = dMDCT[j];
  int eMDCTj   = eMDCT[j];
  uint caMDCTj  = l_abs(cMDCT[j]);
  uint daMDCTj  = l_abs(dMDCT[j]);
  uint eaMDCTj  = l_abs(eMDCT[j]);
  uint caMDCTjm1 = j>0 ? l_abs(cMDCTjm1) : 0;
  uint ctx1a    = l_abs(ctx1);
  uint ctx2a    = l_abs(ctx2);
  uint ctx3a    = l_abs(ctx3);
  uint zrun = ctx1==0 ? ctx2==0 ? ctx3==0 ? 3 : 2 : 1 : 0;

  uint signs_j = signs[j];
  uint zeros_j = zeros[j];
//  uint plus1_j = plus1[j];
//  uint m1nus_j = m1nus[j];

  uint j_lt_c1 = j<c1;
  uint j_band = jband[j] + (j>=c1);

  // the mixing stage's rows of this place (XC), from:
  //   rrl  the bit length of 16 times the last granule's |value| at j, as
  //        this granule's quantizer step would have it (cm_tab.inc)
  //   dS   how much coarser its step was, in 1.5 dB units (clamped to 48)
  //   sfb  the scalefactor band of j, in the frame's own table
  //   dc1, dk  the bit lengths of the places left before the count1
  //        region (0 within it) and before the zeros
  //   w5   the last granule's |values| from j-2 to j+2, and w2 those two
  //        granules back from j-1 to j+1 (the places before j have been
  //        shifted already: their last granule is in dMDCT, eMDCT)
  {
    // (unsigned, the variables that cannot be negative)
    uint cp1 = j<575 ? l_abs(cMDCT[j+1]) : 0, cp2 = j<574 ? l_abs(cMDCT[j+2]) : 0;
    uint w5 = (j>1 ? l_abs(dMDCT[j-2]) : 0) + caMDCTjm1 + caMDCTj + cp1 + cp2;
    uint w2 = (j>0 ? l_abs(eMDCT[j-1]) : 0) + daMDCTj + (j<575 ? l_abs(dMDCT[j+1]) : 0);
    int dS = cStep[j] - xstp[j]; dS = dS<-48 ? -48 : dS>48 ? 48 : dS;
    uint rrl = cm_blen( caMDCTj*cm_resc[dS+48] );
    uint dc1 = j<c1 ? cm_blen( c1-j ) : 0;
    uint dk = cm_blen( k-j );
    uint sfb = xsfb[j];
    int xa, xb, xc, xd;

    xa = 0;
    xa = xa*17 + __min(16,__max(0,(rrl)-(1-1)));
    xa = xa*7 + M_xa_i1[__min(32,__max(0,(dS)-(-15-1)))];
    xa = xa*4 + __min(3,__max(0,(ctx1a)-(1-1)));
    xa = xa*22 + __min(21,__max(0,(sfb)-(1-1)));

    xb = 0;
    xb = xb*11 + __min(10,__max(0,(dc1)-(1-1)));
    xb = xb*11 + __min(10,__max(0,(dk)-(1-1)));
    xb = xb*4 + __min(3,__max(0,(caMDCTj)-(1-1)));
    xb = xb*3 + (((ctx1a)>0+(1-1))+((ctx1a)>1+(1-1)));

    xc = 0;
    xc = xc*576 + __min(575,__max(0,(j)-(1-1)));
    xc = xc*8 + __min(7,__max(0,(caMDCTj)-(1-1)));

    xd = 0;
    xd = xd*32 + __min(31,__max(0,(w5)-(1-1)));
    xd = xd*16 + __min(15,__max(0,(w2)-(1-1)));
    xd = xd*3 + (((ctx1a)>0+(1-1))+((ctx1a)>1+(1-1)));
    xrA = xa*XND; xrB = xb*XND; xrC = xc*XND; xrD = xd*XND;
  }

  if( (j!=k-1) && (j!=c1-1) ) {
    int zero = (c==0);
    d = 0; e = 0; f=0;

    if( j<c1 ) {
#ifndef DISABLE_Sm0A
      int band_Sm0A,band_Sn0A,band_Smm0A;

      band_Sm0A = 0;
      band_Sm0A = band_Sm0A*198 + M_band_Sm0A_i0[__min(576,__max(0,(j)-(1-1)))];

      band_Sn0A = 0;
      band_Sn0A = band_Sn0A*22 + M_band_Sn0A_i0[__min(576,__max(0,(j)-(1-1)))];

      band_Smm0A = 0;
      band_Smm0A = band_Smm0A*5 + M_band_Smm0A_i0[__min(576,__max(0,(j)-(1-1)))];

      uint flag_jm1 = caMDCTj + 1-((j>0)&&(cMDCTjm1==0)); //((j>0)&&(cMDCTjm1==0)) && (caMDCTj==0);

      int pm0A_f, Sm0A_d, m0A_e, n0A_e, mm0A_g;

      pm0A_f = 0;
      pm0A_f = pm0A_f*2 + (((ch)>0+(1-1)));
      pm0A_f = pm0A_f*2 + (((band_j)>0+(1-1)));
      pm0A_f = pm0A_f*2 + (((curband)>1+(1-1)));
      pm0A_f = pm0A_f*2 + (((caMDCTj)>0+(1-1)));
      byte& rec = pm0A[pm0A_f];
      uint recm = cm_trail1[rec&31];

      m0A_e = 0;
      m0A_e = m0A_e*2 + (((band_j)>9+(1-1)));
      m0A_e = m0A_e*4 + M_m0A_e_i3[__min(21,__max(0,(curband)-(1-1)))];
      m0A_e = m0A_e*4 + M_m0A_e_i6[__min(16,__max(0,(daMDCTj)-(1-1)))];
      m0A_e = m0A_e*(M_band_Sm0A_Volume) + (band_Sm0A);

      n0A_e = 0;
      n0A_e = n0A_e*4 + M_n0A_e_i3[__min(21,__max(0,(curband)-(1-1)))];
      n0A_e = n0A_e*2 + (((caMDCTj)>3+(1-1)));
      n0A_e = n0A_e*3 + (((daMDCTj)>0+(1-1))+((daMDCTj)>2+(1-1)));
      n0A_e = n0A_e*2 + (((eaMDCTj)>1+(1-1)));
      n0A_e = n0A_e*4 + M_n0A_e_i9[__min(16,__max(0,(ctx1a)-(1-1)))];
      n0A_e = n0A_e*2 + (((ctx2a)>1+(1-1)));
      n0A_e = n0A_e*2 + (((ctx3a)>1+(1-1)));
      n0A_e = n0A_e*(M_band_Sn0A_Volume) + (band_Sn0A);

      mm0A_g = 0;
      mm0A_g = mm0A_g*2 + (((ch)>0+(1-1)));
      mm0A_g = mm0A_g*2 + (((band_j)>16+(1-1)));
      mm0A_g = mm0A_g*2 + (((curband)>0+(1-1)));
      mm0A_g = mm0A_g*3 + (((caMDCTj)>2+(1-1))+((caMDCTj)>16+(1-1)));
      mm0A_g = mm0A_g*4 + M_mm0A_g_i7[__min(20,__max(0,(daMDCTj)-(1-1)))];
      mm0A_g = mm0A_g*3 + (((eaMDCTj)>5+(1-1))+((eaMDCTj)>10+(1-1)));
      mm0A_g = mm0A_g*2 + (((ctx1a)>9+(1-1)));
      mm0A_g = mm0A_g*2 + (((zrun)>2+(1-1)));
      mm0A_g = mm0A_g*(M_band_Smm0A_Volume) + (band_Smm0A);

      Sm0A_d = 0;
      Sm0A_d = Sm0A_d*2 + (((ch)>0+(1-1)));
      Sm0A_d = Sm0A_d*2 + (((band_j)>12+(1-1)));
      Sm0A_d = Sm0A_d*2 + (((j_band)>6+(1-1)));
      Sm0A_d = Sm0A_d*4 + M_Sm0A_d_i5[__min(16,__max(0,(caMDCTj)-(1-1)))];
      Sm0A_d = Sm0A_d*3 + (((caMDCTjm1)>1+(1-1))+((caMDCTjm1)>3+(1-1)));
      Sm0A_d = Sm0A_d*3 + (((ctx1a)>1+(1-1))+((ctx1a)>3+(1-1)));
      Sm0A_d = Sm0A_d*2 + (((zrun)>2+(1-1)));
      Sm0A_d = Sm0A_d*2 + (((recm)>4+(1-1)));

      const word* M_Sm0A_wr = j>M_Sm0A_jj ? M_Sm0A_wr0 : M_Sm0A_wq0;
      const word* M_Sm0A_wq = j>M_Sm0A_jj ? M_Sm0A_wr1 : M_Sm0A_wq1;
      const uint  M_Sm0A_mw = j>M_Sm0A_jj ? M_Sm0A_mw0 : M_Sm0A_mw1;
      const word* M_Sn0A_wr = j>M_Sn0A_j1 ? M_Sn0A_wr0 : M_Sn0A_wq0;
      const uint  M_Sn0A_mw = j>M_Sn0A_j1 ? M_Sn0A_mw0 : M_Sn0A_mw1;

      SSEii_updstr X;
      Node2ii& C = m0A[m0A_e];
      Node2ii& D = n0A[n0A_e];
      auto& S = Sm0A[Sm0A_d];
      int p_sse = S.SSE_Pred( C.P, X );

      XC( rc, 0, D.P, p_sse, zero, mm0A_g );
      S.SSE_Update( zero, M_Sm0A_wq, X );
      C.Update( zero, M_Sm0A_wr, M_Sm0A_mw );
      D.Update( zero, M_Sn0A_wr, M_Sn0A_mw );

      rec=rec*2+zero;
#endif
    } else {
#ifndef DISABLE_Sm0B
      int pm0B_f, Sm0B_d, m0B_e, n0B_e,mm0B_g, band_Sm0B,band_Sn0B,band_Smm0B;
      uint flag_jm1 = ((j>1)&&(ctx2==0)) && ((j>0)&&(ctx1==0));


      band_Sm0B = 0;
      band_Sm0B = band_Sm0B*99 + M_band_Sm0B_i0[__min(576,__max(0,(j)-(1-1)))];

      band_Sn0B = 0;
      band_Sn0B = band_Sn0B*18 + M_band_Sn0B_i0[__min(576,__max(0,(j)-(1-1)))];

      band_Smm0B = 0;
      band_Smm0B = band_Smm0B*48 + M_band_Smm0B_i0[__min(576,__max(0,(j)-(1-1)))];

      pm0B_f = 0;
      pm0B_f = pm0B_f*2 + (((gi.count1table_select)>0+(1-1)));
      pm0B_f = pm0B_f*4 + M_pm0B_f_i3[__min(24,__max(0,(curband)-(1-1)))];
      pm0B_f = pm0B_f*3 + (((flag_jm1)>0+(1-1))+((flag_jm1)>3+(1-1)));
      pm0B_f = pm0B_f*5 + M_pm0B_f_i5[__min(16,__max(0,(caMDCTj)-(1-1)))];
      pm0B_f = pm0B_f*5 + M_pm0B_f_i6[__min(16,__max(0,(daMDCTj)-(1-1)))];
      pm0B_f = pm0B_f*2 + (((eaMDCTj)>8+(1-1)));
      pm0B_f = pm0B_f*2 + (((ctx1a)>5+(1-1)));
      byte& rec = pm0B[pm0B_f];
      uint recm = cm_trail1[rec&31];

      m0B_e = 0;
      m0B_e = m0B_e*3 + (((curband)>0+(1-1))+((curband)>2+(1-1)));
      m0B_e = m0B_e*2 + (((daMDCTj)>1+(1-1)));
      m0B_e = m0B_e*2 + (((zrun)>2+(1-1)));
      m0B_e = m0B_e*(M_band_Sm0B_Volume) + (band_Sm0B);

      n0B_e = 0;
      n0B_e = n0B_e*2 + (((gi.count1table_select)>0+(1-1)));
      n0B_e = n0B_e*2 + (((ch)>0+(1-1)));
      n0B_e = n0B_e*2 + (((curband)>1+(1-1)));
      n0B_e = n0B_e*2 + (((caMDCTj)>0+(1-1)));
      n0B_e = n0B_e*2 + (((ctx1a)>0+(1-1)));
      n0B_e = n0B_e*3 + (((ctx2a)>3+(1-1))+((ctx2a)>10+(1-1)));
      n0B_e = n0B_e*(M_band_Sn0B_Volume) + (band_Sn0B);

      mm0B_g = 0;
      mm0B_g = mm0B_g*3 + (((band_j)>13+(1-1))+((band_j)>17+(1-1)));
      mm0B_g = mm0B_g*3 + (((caMDCTj)>1+(1-1))+((caMDCTj)>3+(1-1)));
      mm0B_g = mm0B_g*2 + (((daMDCTj)>0+(1-1)));
      mm0B_g = mm0B_g*2 + (((caMDCTjm1)>13+(1-1)));
      mm0B_g = mm0B_g*2 + (((zrun)>0+(1-1)));
      mm0B_g = mm0B_g*2 + (((recm)>4+(1-1)));
      mm0B_g = mm0B_g*(M_band_Smm0B_Volume) + (band_Smm0B);

      Sm0B_d = 0;
      Sm0B_d = Sm0B_d*2 + (((gi.count1table_select)>0+(1-1)));
      Sm0B_d = Sm0B_d*2 + (((ch)>0+(1-1)));
      Sm0B_d = Sm0B_d*3 + (((j_band)>7+(1-1))+((j_band)>8+(1-1)));
      Sm0B_d = Sm0B_d*2 + (((curband)>0+(1-1)));
      Sm0B_d = Sm0B_d*3 + (((caMDCTj)>0+(1-1))+((caMDCTj)>1+(1-1)));
      Sm0B_d = Sm0B_d*2 + (((caMDCTjm1)>1+(1-1)));
      Sm0B_d = Sm0B_d*2 + (((ctx1a)>1+(1-1)));

      const word* M_Sm0B_wr = j>M_Sm0B_jj ? M_Sm0B_wr0 : M_Sm0B_wq0;
      const word* M_Sm0B_wq = j>M_Sm0B_jj ? M_Sm0B_wr1 : M_Sm0B_wq1;
      const uint  M_Sm0B_mw = j>M_Sm0B_jj ? M_Sm0B_mw0 : M_Sm0B_mw1;
      //Sm0B[Sm0B_d].Process( rc, m0B[m0B_e], M_Sm0B_wr, M_Sm0B_mw, zero, M_Sm0B_wq );
      const word* M_Sn0B_wr = j>M_Sn0B_j1 ? M_Sn0B_wr0 : M_Sn0B_wq0;
      const uint  M_Sn0B_mw = j>M_Sn0B_j1 ? M_Sn0B_mw0 : M_Sn0B_mw1;

      SSEii_updstr X;
      Node2ii& C = m0B[m0B_e];
      Node2ii& D = n0B[n0B_e];
      auto& S = Sm0B[Sm0B_d];
      int p_sse = S.SSE_Pred( C.P, X );

      XC( rc, 1, D.P, p_sse, zero, mm0B_g );
      S.SSE_Update( zero, M_Sm0B_wq, X );
      C.Update( zero, M_Sm0B_wr, M_Sm0B_mw );
      D.Update( zero, M_Sn0B_wr, M_Sn0B_mw );

      rec=rec*2+zero;
#endif
    }

    if( zero ) { c=0; goto Processed; }
  }

//  MakeIndex band_x

  av_total = 0;
  av_total = av_total*(0) + (M_av_C1);
  av_total = av_total*(1) + (M_av_C2);
  av_total = av_total*(1) + (M_av_C3);
  av_total = av_total*(1) + (M_av_C4);
  av_total = av_total*(1) + (M_av_C5);

  s = (c<0); c=l_abs(c); l=0;

  if( j!=c1-1 ) {
    int abs1 = (c==1);
#ifndef DISABLE_Sm1
    if( j>=c1 ) abs1=1; else {
      int j_gt_j0 = j>M_Sm1_j0;
      int m1_e,Sm1_d,pm1_f, mm1_g,n1_e, band_Sm1, band_Sn1;

      band_Sm1 = 0;
      band_Sm1 = band_Sm1*254 + M_band_Sm1_i0[__min(576,__max(0,(j)-(1-1)))];

      band_Sn1 = 0;
      band_Sn1 = band_Sn1*10 + M_band_Sn1_i0[__min(576,__max(0,(j)-(1-1)))];

      pm1_f = 0;
      pm1_f = pm1_f*2 + (((ch)>0+(1-1)));
      pm1_f = pm1_f*7 + M_pm1_f_i4[__min(21,__max(0,(band_j)-(1-1)))];
      pm1_f = pm1_f*3 + (((curband)>7+(1-1))+((curband)>13+(1-1)));
      pm1_f = pm1_f*11 + M_pm1_f_i9[__min(26,__max(0,(caMDCTj)-(1-1)))];
      pm1_f = pm1_f*3 + (((daMDCTj)>13+(1-1))+((daMDCTj)>25+(1-1)));
      byte &rec = pm1[pm1_f];

      Sm1_d = 0;
      Sm1_d = Sm1_d*2 + (((ch)>0+(1-1)));
      Sm1_d = Sm1_d*2 + (((j_band)>7+(1-1)));
      Sm1_d = Sm1_d*2 + (((j_gt_j0)>0+(1-1)));
      Sm1_d = Sm1_d*4 + M_Sm1_d_i7[__min(16,__max(0,(ctx1a)-(1-1)))];
      Sm1_d = Sm1_d*6 + M_Sm1_d_ia[__min(25,__max(0,(caMDCTj)-(1-1)))];
      Sm1_d = Sm1_d*3 + (((caMDCTjm1)>2+(1-1))+((caMDCTjm1)>6+(1-1)));
      Sm1_d = (Sm1_d<<2) + (((rec)&3)); // 00000011

      m1_e = 0;
      m1_e = m1_e*4 + M_m1_e_i5[__min(21,__max(0,(curband)-(1-1)))];
      m1_e = m1_e*2 + (((ctx2a)>14+(1-1)));
      m1_e = m1_e*(M_band_Sm1_Volume) + (band_Sm1);

      mm1_g = 0;
      mm1_g = mm1_g*2 + (((ch)>0+(1-1)));
      mm1_g = mm1_g*3 + (((j_band)>2+(1-1))+((j_band)>6+(1-1)));
      mm1_g = mm1_g*3 + (((band_j)>9+(1-1))+((band_j)>16+(1-1)));
      mm1_g = mm1_g*2 + (((curband)>0+(1-1)));
      mm1_g = mm1_g*2 + (((ctx1a)>11+(1-1)));
      mm1_g = mm1_g*3 + (((caMDCTj)>1+(1-1))+((caMDCTj)>9+(1-1)));
      mm1_g = mm1_g*4 + M_mm1_g_ib[__min(20,__max(0,(daMDCTj)-(1-1)))];
      mm1_g = mm1_g*2 + (((eaMDCTj)>9+(1-1)));
      mm1_g = (mm1_g<<1) + ((((rec)>>2)&1)); // 00000100
      mm1_g = mm1_g*2 + (((zrun)>2+(1-1)));

      n1_e = 0;
      n1_e = n1_e*2 + (((ch)>0+(1-1)));
      n1_e = n1_e*2 + (((curband)>2+(1-1)));
      n1_e = n1_e*2 + (((ctx1a)>3+(1-1)));
      n1_e = n1_e*2 + (((ctx2a)>3+(1-1)));
      n1_e = n1_e*2 + (((ctx3a)>3+(1-1)));
      n1_e = n1_e*2 + (((caMDCTj)>3+(1-1)));
      n1_e = n1_e*4 + M_n1_e_ib[__min(16,__max(0,(daMDCTj)-(1-1)))];
      n1_e = n1_e*2 + (((eaMDCTj)>3+(1-1)));
      n1_e = n1_e*(M_band_Sn1_Volume) + (band_Sn1);
      const word* M_Sm1_wr = j>M_Sm1_j1 ? M_Sm1_wr0 : M_Sm1_wq0;
      const word* M_Sm1_wq = j>M_Sm1_j1 ? M_Sm1_wr1 : M_Sm1_wq1;
      const uint  M_Sm1_mw = j>M_Sm1_j1 ? M_Sm1_mw0 : M_Sm1_mw1;
      const word* M_Sn1_wr = j>M_Sn1_j1 ? M_Sn1_wr0 : M_Sn1_wq0;
      const uint  M_Sn1_mw = j>M_Sn1_j1 ? M_Sn1_mw0 : M_Sn1_mw1;

      SSEii_updstr X;
      Node2ii& C = m1[m1_e];
      Node2ii& D = n1[n1_e];
      auto& S = Sm1[Sm1_d];
      int p_sse = S.SSE_Pred( C.P, X );

      XC( rc, 2, D.P, p_sse, abs1, mm1_g );
      S.SSE_Update( abs1, M_Sm1_wq, X );
      C.Update( abs1, M_Sm1_wr, M_Sm1_mw );
      D.Update( abs1, M_Sn1_wr, M_Sn1_mw );

      rec = rec*2 + abs1;
    }
#endif
    if( abs1 ) { c=1; goto SignToProcess; }
  }


  exA = av_MDCT[j]*M_av_C1 + daMDCTj*M_av_C2 + caMDCTj*M_av_C3 + eaMDCTj*M_av_C4 + ctx1a*M_av_C5;
  aext = exA/av_total;
  a = Max<int>(1, (aext*M_av_C0)/256 ) - 1;

  ll = (a>0) ? log2i(a)+1 : 9; 
  l = Min<int>(Max<int>(ll,2),9)-2;

  {
    int abs2 = (c==2);
#ifndef DISABLE_Sm2
    int j_gt_j0 = j>M_Sm2_j0;
    int m2_e,Sm2_d,pm2_f, mm2_g,n2_e;
    int band_Sm2,band_Sn2;

    band_Sm2 = 0;
    band_Sm2 = band_Sm2*108 + M_band_Sm2_i0[__min(576,__max(0,(j)-(1-1)))];

    band_Sn2 = 0;
    band_Sn2 = band_Sn2*16 + M_band_Sn2_i0[__min(576,__max(0,(j)-(1-1)))];

    pm2_f = 0;
    pm2_f = pm2_f*2 + (((ch)>0+(1-1)));
    pm2_f = pm2_f*5 + M_pm2_f_i1[__min(21,__max(0,(band_j)-(1-1)))];
    pm2_f = pm2_f*2 + (((curband)>6+(1-1)));
    pm2_f = pm2_f*3 + (((l)>2+(1-1))+((l)>4+(1-1)));
    pm2_f = pm2_f*4 + M_pm2_f_i5[__min(25,__max(0,(ctx1a)-(1-1)))];
    pm2_f = pm2_f*6 + M_pm2_f_i8[__min(16,__max(0,(caMDCTj)-(1-1)))];
    pm2_f = pm2_f*2 + (((caMDCTjm1)>17+(1-1)));
    pm2_f = pm2_f*4 + M_pm2_f_ia[__min(23,__max(0,(daMDCTj)-(1-1)))];
    pm2_f = pm2_f*2 + (((eaMDCTj)>14+(1-1)));
    byte &rec = pm2[pm2_f];

    Sm2_d = 0;
    Sm2_d = Sm2_d*2 + (((ch)>0+(1-1)));
    Sm2_d = Sm2_d*2 + (((j_gt_j0)>0+(1-1)));
    Sm2_d = Sm2_d*4 + M_Sm2_d_i5[__min(16,__max(0,(l)-(1-1)))];
    Sm2_d = Sm2_d*3 + (((ctx1a)>1+(1-1))+((ctx1a)>4+(1-1)));
    Sm2_d = Sm2_d*4 + M_Sm2_d_i9[__min(16,__max(0,(caMDCTj)-(1-1)))];
    Sm2_d = Sm2_d*3 + (((caMDCTjm1)>1+(1-1))+((caMDCTjm1)>4+(1-1)));
    Sm2_d = (Sm2_d<<2) + (((rec)&3)); // 00000011

    m2_e = 0;
    m2_e = m2_e*3 + (((curband)>1+(1-1))+((curband)>4+(1-1)));
    m2_e = m2_e*3 + (((l)>0+(1-1))+((l)>6+(1-1)));
    m2_e = m2_e*2 + (((ctx2a)>3+(1-1)));
    m2_e = m2_e*2 + (((caMDCTj)>7+(1-1)));
    m2_e = m2_e*2 + (((daMDCTj)>4+(1-1)));
    m2_e = m2_e*(M_band_Sm2_Volume) + (band_Sm2);

    mm2_g = 0;
    mm2_g = mm2_g*2 + (((ch)>0+(1-1)));
    mm2_g = mm2_g*4 + M_mm2_g_i1[__min(21,__max(0,(band_j)-(1-1)))];
    mm2_g = mm2_g*2 + (((j_band)>7+(1-1)));
    mm2_g = mm2_g*2 + (((curband)>2+(1-1)));
    mm2_g = mm2_g*3 + (((l)>0+(1-1))+((l)>5+(1-1)));
    mm2_g = mm2_g*2 + (((ctx2a)>7+(1-1)));
    mm2_g = mm2_g*4 + M_mm2_g_i8[__min(21,__max(0,(caMDCTj)-(1-1)))];
    mm2_g = mm2_g*4 + M_mm2_g_ia[__min(21,__max(0,(daMDCTj)-(1-1)))];
    mm2_g = mm2_g*2 + (((eaMDCTj)>7+(1-1)));

    n2_e = 0;
    n2_e = n2_e*2 + (((ch)>0+(1-1)));
    n2_e = n2_e*4 + M_n2_e_i2[__min(21,__max(0,(curband)-(1-1)))];
    n2_e = n2_e*2 + (((l)>1+(1-1)));
    n2_e = n2_e*2 + (((ctx1a)>8+(1-1)));
    n2_e = n2_e*3 + (((caMDCTj)>2+(1-1))+((caMDCTj)>4+(1-1)));
    n2_e = n2_e*(M_band_Sn2_Volume) + (band_Sn2);
    const word* M_Sm2_wr = j>M_Sm2_j1 ? M_Sm2_wr0 : M_Sm2_wq0;
    const word* M_Sm2_wq = j>M_Sm2_j1 ? M_Sm2_wr1 : M_Sm2_wq1;
    const uint  M_Sm2_mw = j>M_Sm2_j1 ? M_Sm2_mw0 : M_Sm2_mw1;
    const word* M_Sn2_wr = j>M_Sn2_j1 ? M_Sn2_wr0 : M_Sn2_wq0;
    const uint  M_Sn2_mw = j>M_Sn2_j1 ? M_Sn2_mw0 : M_Sn2_mw1;

    SSEii_updstr X;
    Node2ii& C = m2[m2_e];
    Node2ii& D = n2[n2_e];
    auto& S = Sm2[Sm2_d];
    int p_sse = S.SSE_Pred( C.P, X );

    XC( rc, 3, D.P, p_sse, abs2, mm2_g );
    S.SSE_Update( abs2, M_Sm2_wq, X );
    C.Update( abs2, M_Sm2_wr, M_Sm2_mw );
    D.Update( abs2, M_Sn2_wr, M_Sn2_mw );

    rec = rec*2 + abs2;
#endif
    if( abs2 ) { c=2; goto SignToProcess; }
  }

  {
    int abs3 = (c==3);
#ifndef DISABLE_Sm3
    int j_gt_j0 = j>M_Sm3_j0;
    int m3_e,Sm3_d,pm3_f, mm3_g,n3_e;
    int band_Sm3,band_Sn3;

    band_Sm3 = 0;
    band_Sm3 = band_Sm3*81 + M_band_Sm3_i0[__min(576,__max(0,(j)-(1-1)))];

    band_Sn3 = 0;
    band_Sn3 = band_Sn3*9 + M_band_Sn3_i0[__min(576,__max(0,(j)-(1-1)))];

    pm3_f = 0;
    pm3_f = pm3_f*2 + (((ch)>0+(1-1)));
    pm3_f = pm3_f*3 + (((j_band)>0+(1-1))+((j_band)>1+(1-1)));
    pm3_f = pm3_f*3 + (((curband)>0+(1-1))+((curband)>1+(1-1)));
    pm3_f = pm3_f*2 + (((l)>2+(1-1)));
    pm3_f = pm3_f*2 + (((ctx1a)>16+(1-1)));
    pm3_f = pm3_f*8 + M_pm3_f_i8[__min(21,__max(0,(caMDCTj)-(1-1)))];
    pm3_f = pm3_f*3 + (((caMDCTjm1)>21+(1-1))+((caMDCTjm1)>23+(1-1)));
    pm3_f = pm3_f*3 + (((daMDCTj)>18+(1-1))+((daMDCTj)>19+(1-1)));
    pm3_f = pm3_f*2 + (((eaMDCTj)>18+(1-1)));
    byte &rec = pm3[pm3_f];

    Sm3_d = 0;
    Sm3_d = Sm3_d*2 + (((ch)>0+(1-1)));
    Sm3_d = Sm3_d*3 + (((l)>2+(1-1))+((l)>3+(1-1)));
    Sm3_d = Sm3_d*4 + M_Sm3_d_i6[__min(16,__max(0,(ctx1a)-(1-1)))];
    Sm3_d = Sm3_d*4 + M_Sm3_d_i9[__min(16,__max(0,(caMDCTj)-(1-1)))];
    Sm3_d = Sm3_d*4 + M_Sm3_d_ia[__min(16,__max(0,(caMDCTjm1)-(1-1)))];
    Sm3_d = (Sm3_d<<2) + (((rec)&3)); // 00000011

    m3_e = 0;
    m3_e = m3_e*2 + (((curband)>4+(1-1)));
    m3_e = m3_e*2 + (((ctx2a)>4+(1-1)));
    m3_e = m3_e*2 + (((ctx3a)>4+(1-1)));
    m3_e = m3_e*2 + (((caMDCTj)>8+(1-1)));
    m3_e = m3_e*2 + (((daMDCTj)>5+(1-1)));
    m3_e = m3_e*(M_band_Sm3_Volume) + (band_Sm3);

    mm3_g = 0;
    mm3_g = mm3_g*2 + (((ch)>0+(1-1)));
    mm3_g = mm3_g*3 + (((band_j)>9+(1-1))+((band_j)>16+(1-1)));
    mm3_g = mm3_g*2 + (((j_band)>7+(1-1)));
    mm3_g = mm3_g*2 + (((curband)>1+(1-1)));
    mm3_g = mm3_g*4 + M_mm3_g_i4[__min(16,__max(0,(l)-(1-1)))];
    mm3_g = mm3_g*2 + (((ctx2a)>8+(1-1)));
    mm3_g = mm3_g*4 + M_mm3_g_i8[__min(21,__max(0,(caMDCTj)-(1-1)))];
    mm3_g = mm3_g*2 + (((daMDCTj)>12+(1-1)));

    n3_e = 0;
    n3_e = n3_e*2 + (((ch)>0+(1-1)));
    n3_e = n3_e*3 + (((curband)>0+(1-1))+((curband)>7+(1-1)));
    n3_e = n3_e*5 + M_n3_e_i3[__min(16,__max(0,(l)-(1-1)))];
    n3_e = n3_e*2 + (((ctx1a)>9+(1-1)));
    n3_e = n3_e*3 + (((caMDCTj)>3+(1-1))+((caMDCTj)>4+(1-1)));
    n3_e = n3_e*2 + (((zrun)>2+(1-1)));
    n3_e = n3_e*(M_band_Sn3_Volume) + (band_Sn3);
    const word* M_Sm3_wr = j>M_Sm3_j1 ? M_Sm3_wr0 : M_Sm3_wq0;
    const word* M_Sm3_wq = j>M_Sm3_j1 ? M_Sm3_wr1 : M_Sm3_wq1;
    const uint  M_Sm3_mw = j>M_Sm3_j1 ? M_Sm3_mw0 : M_Sm3_mw1;
    const word* M_Sn3_wr = j>M_Sn3_j1 ? M_Sn3_wr0 : M_Sn3_wq0;
    const uint  M_Sn3_mw = j>M_Sn3_j1 ? M_Sn3_mw0 : M_Sn3_mw1;

    SSEii_updstr X;
    Node2ii& C = m3[m3_e];
    Node2ii& D = n3[n3_e];
    auto& S = Sm3[Sm3_d];
    int p_sse = S.SSE_Pred( C.P, X );

    XC( rc, 4, D.P, p_sse, abs3, mm3_g );
    S.SSE_Update( abs3, M_Sm3_wq, X );
    C.Update( abs3, M_Sm3_wr, M_Sm3_mw );
    D.Update( abs3, M_Sn3_wr, M_Sn3_mw );

    rec = rec*2 + abs3;
#endif
    if( abs3 ) { c=3; goto SignToProcess; }
  }

  {
    int abs4 = (c==4);
#ifndef DISABLE_Sm4
    int j_gt_j0 = j>M_Sm4_j0;
    int m4_e,Sm4_d,pm4_f, mm4_g,n4_e;
    int band_Sm4,band_Sn4;

    band_Sm4 = 0;

    band_Sn4 = 0;
    band_Sn4 = band_Sn4*84 + M_band_Sn4_i0[__min(576,__max(0,(j)-(1-1)))];

    pm4_f = 0;
    pm4_f = pm4_f*2 + (((ch)>0+(1-1)));
    pm4_f = pm4_f*5 + M_pm4_f_i1[__min(21,__max(0,(band_j)-(1-1)))];
    pm4_f = pm4_f*2 + (((j_band)>7+(1-1)));
    pm4_f = pm4_f*2 + (((curband)>0+(1-1)));
    pm4_f = pm4_f*4 + M_pm4_f_i4[__min(16,__max(0,(l)-(1-1)))];
    pm4_f = pm4_f*2 + (((ctx1a)>16+(1-1)));
    pm4_f = pm4_f*11 + M_pm4_f_i8[__min(27,__max(0,(caMDCTj)-(1-1)))];
    pm4_f = pm4_f*2 + (((caMDCTjm1)>21+(1-1)));
    byte &rec = pm4[pm4_f];

    Sm4_d = 0;
    Sm4_d = Sm4_d*2 + (((ch)>0+(1-1)));
    Sm4_d = Sm4_d*2 + (((j_gt_j0)>0+(1-1)));
    Sm4_d = Sm4_d*3 + (((curband)>1+(1-1))+((curband)>8+(1-1)));
    Sm4_d = Sm4_d*6 + M_Sm4_d_i5[__min(16,__max(0,(l)-(1-1)))];
    Sm4_d = Sm4_d*2 + (((caMDCTj)>4+(1-1)));
    Sm4_d = (Sm4_d<<1) + (((rec)&1)); // 00000001

    m4_e = 0;
    m4_e = m4_e*2 + (((ch)>0+(1-1)));
    m4_e = m4_e*4 + M_m4_e_i4[__min(16,__max(0,(ctx1a)-(1-1)))];
    m4_e = m4_e*2 + (((ctx2a)>3+(1-1)));
    m4_e = m4_e*2 + (((ctx3a)>3+(1-1)));
    m4_e = m4_e*6 + M_m4_e_i7[__min(16,__max(0,(caMDCTj)-(1-1)))];
    m4_e = m4_e*4 + M_m4_e_i8[__min(16,__max(0,(caMDCTjm1)-(1-1)))];
    m4_e = (m4_e<<2) + ((((rec)>>1)&3)); // 00000110
    m4_e = m4_e*(M_band_Sm4_Volume) + (band_Sm4);

    mm4_g = 0;
    mm4_g = mm4_g*2 + (((ch)>0+(1-1)));
    mm4_g = mm4_g*2 + (((band_j)>12+(1-1)));
    mm4_g = mm4_g*3 + (((j_band)>0+(1-1))+((j_band)>6+(1-1)));
    mm4_g = mm4_g*3 + (((l)>4+(1-1))+((l)>13+(1-1)));
    mm4_g = mm4_g*2 + (((ctx1a)>11+(1-1)));
    mm4_g = mm4_g*2 + (((ctx3a)>10+(1-1)));
    mm4_g = mm4_g*2 + (((caMDCTj)>17+(1-1)));
    mm4_g = mm4_g*3 + (((daMDCTj)>10+(1-1))+((daMDCTj)>14+(1-1)));

    n4_e = 0;
    n4_e = n4_e*2 + (((ch)>0+(1-1)));
    n4_e = n4_e*3 + (((band_j)>6+(1-1))+((band_j)>12+(1-1)));
    n4_e = n4_e*2 + (((ctx1a)>7+(1-1)));
    n4_e = n4_e*2 + (((ctx2a)>7+(1-1)));
    n4_e = n4_e*2 + (((caMDCTj)>7+(1-1)));
    n4_e = n4_e*(M_band_Sn4_Volume) + (band_Sn4);
    const word* M_Sm4_wr = j>M_Sm4_j1 ? M_Sm4_wr0 : M_Sm4_wq0;
    const word* M_Sm4_wq = j>M_Sm4_j1 ? M_Sm4_wr1 : M_Sm4_wq1;
    const uint  M_Sm4_mw = j>M_Sm4_j1 ? M_Sm4_mw0 : M_Sm4_mw1;
    const word* M_Sn4_wr = j>M_Sn4_j1 ? M_Sn4_wr0 : M_Sn4_wq0;
    const uint  M_Sn4_mw = j>M_Sn4_j1 ? M_Sn4_mw0 : M_Sn4_mw1;

    SSEii_updstr X;
    Node2ii& C = m4[m4_e];
    Node2ii& D = n4[n4_e];
    auto& S = Sm4[Sm4_d];
    int p_sse = S.SSE_Pred( C.P, X );

    XC( rc, 5, D.P, p_sse, abs4, mm4_g );
    S.SSE_Update( abs4, M_Sm4_wq, X );
    C.Update( abs4, M_Sm4_wr, M_Sm4_mw );
    D.Update( abs4, M_Sn4_wr, M_Sn4_mw );

    rec = rec*2 + abs4;
#endif
    if( abs4 ) { c=4; goto SignToProcess; }
  }

  {
    int mx_e,Smx_d,nx_e,mmx_g,band_Smx,band_Snx;

    band_Smx = 0;
    band_Smx = band_Smx*90 + M_band_Smx_i0[__min(576,__max(0,(j)-(1-1)))];

    band_Snx = 0;

    mx_e = 0;
    mx_e = mx_e*2 + (((curband)>2+(1-1)));
    mx_e = mx_e*2 + (((ctx1a)>15+(1-1)));
    mx_e = mx_e*4 + M_mx_e_ib[__min(16,__max(0,(l)-(1-1)))];
    mx_e = mx_e*(M_band_Smx_Volume) + (band_Smx);
    mx_e = mx_e*(M_Anx_ex_Volume) + (0);

    Smx_d = 0;
    Smx_d = Smx_d*2 + (((ch)>0+(1-1)));
    Smx_d = Smx_d*2 + (((j_band)>6+(1-1)));
    Smx_d = Smx_d*3 + (((band_j)>6+(1-1))+((band_j)>14+(1-1)));
    Smx_d = Smx_d*2 + (((curband)>0+(1-1)));
    Smx_d = Smx_d*7 + M_Smx_d_i7[__min(34,__max(0,(caMDCTj)-(1-1)))];
    Smx_d = Smx_d*2 + (((l)>3+(1-1)));
    Smx_d = Smx_d*(M_ASmx_dx_Volume) + (0);

    nx_e = 0;
    nx_e = nx_e*2 + (((ch)>0+(1-1)));
    nx_e = nx_e*2 + (((band_j)>17+(1-1)));
    nx_e = nx_e*3 + (((ctx1a)>5+(1-1))+((ctx1a)>8+(1-1)));
    nx_e = nx_e*2 + (((ctx3a)>6+(1-1)));
    nx_e = nx_e*7 + M_nx_e_i7[__min(23,__max(0,(caMDCTj)-(1-1)))];
    nx_e = nx_e*3 + (((caMDCTjm1)>5+(1-1))+((caMDCTjm1)>8+(1-1)));
    nx_e = nx_e*3 + (((daMDCTj)>6+(1-1))+((daMDCTj)>10+(1-1)));
    nx_e = nx_e*2 + (((eaMDCTj)>7+(1-1)));
    nx_e = nx_e*2 + (((l)>0+(1-1)));
    nx_e = nx_e*(M_band_Snx_Volume) + (band_Snx);
    nx_e = nx_e*(M_Anx_ex_Volume) + (0);

    mmx_g = 0;
    mmx_g = mmx_g*2 + (((ch)>0+(1-1)));
    mmx_g = mmx_g*2 + (((j_band)>7+(1-1)));
    mmx_g = mmx_g*4 + M_mmx_g_i2[__min(21,__max(0,(band_j)-(1-1)))];
    mmx_g = mmx_g*2 + (((ctx2a)>9+(1-1)));
    mmx_g = mmx_g*2 + (((ctx3a)>17+(1-1)));
    mmx_g = mmx_g*2 + (((caMDCTj)>16+(1-1)));
    mmx_g = mmx_g*3 + (((daMDCTj)>17+(1-1))+((daMDCTj)>27+(1-1)));
    mmx_g = mmx_g*2 + (((eaMDCTj)>21+(1-1)));
    mmx_g = mmx_g*4 + M_mmx_g_ib[__min(16,__max(0,(l)-(1-1)))];
    mmx_g = mmx_g*(M_Ammx_gx_Volume) + (0);
    int Ammx_gx,Anx_ex,Amx_ex,ASmx_dx;

    x=5; // c=4..
    do {
      int abs4 = (c==x);
#ifndef DISABLE_Smx

      Ammx_gx = 0;

      Anx_ex = 0;
      Anx_ex = Anx_ex*3 + ((((x-4))>1+(1-1))+(((x-4))>4+(1-1)));

      Amx_ex = 0;

      ASmx_dx = 0;
      ASmx_dx = ASmx_dx*4 + M_ASmx_dx_i0[__min(8,__max(0,((x-4))-(1-1)))];
      const word* M_Smx_wr = j>M_Smx_j1 ? M_Smx_wr0 : M_Smx_wq0;
      const word* M_Smx_wq = j>M_Smx_j1 ? M_Smx_wr1 : M_Smx_wq1;
      const uint  M_Smx_mw = j>M_Smx_j1 ? M_Smx_mw0 : M_Smx_mw1;
      const word* M_Snx_wr = j>M_Snx_j1 ? M_Snx_wr0 : M_Snx_wq0;
      const uint  M_Snx_mw = j>M_Snx_j1 ? M_Snx_mw0 : M_Snx_mw1;

      SSEii_updstr X;
      Node2ii& C = mx[mx_e+Amx_ex];
      Node2ii& D = nx[nx_e+Anx_ex];
      auto& S = Smx[Smx_d+ASmx_dx];
      int p_sse = S.SSE_Pred( C.P, X );

      XC( rc, 6, D.P, p_sse, abs4, mmx_g+Ammx_gx );
      S.SSE_Update( abs4, M_Smx_wq, X );
      C.Update( abs4, M_Smx_wr, M_Smx_mw );
      D.Update( abs4, M_Snx_wr, M_Snx_mw );
#endif
      if( abs4 ) { c=x; goto SignToProcess; }
      x++;
    } while( x<M_mntDepth );
  }


#ifdef DISABLE_bignum
if(0)
#endif
  {

//int _x = c, _t=0;

    int d = M_mntDepth; //x;
    int h = M_mntLimit512;
//    int ctx1al=log2i(ctx1a+1),ctx2al=log2i(ctx2a+1),calMDCTj=log2i(caMDCTj+1);
    int ctx1bl=log2i(ctx1a*ctx1a+1),ctx2bl=log2i(ctx2a*ctx2a+1),cblMDCTj=log2i(caMDCTj*caMDCTj+1),dblMDCTj=log2i(daMDCTj*daMDCTj+1);

    {
      flag = (c>=M_mntLimit512);
#ifndef DISABLE_Sz512
      int Sz512_d,z512_e,Band_z512_e,Band_z512_d;

      Band_z512_d = 0;
      Band_z512_d = Band_z512_d*27 + M_Band_z512_d_i0[__min(576,__max(0,(j)-(1-1)))];

      Band_z512_e = 0;

      Sz512_d = 0;
      Sz512_d = Sz512_d*2 + (((ch)>0+(1-1)));
      Sz512_d = Sz512_d*2 + (((cblMDCTj)>17+(1-1)));
      Sz512_d = Sz512_d*2 + (((ctx1bl)>17+(1-1)));
      Sz512_d = Sz512_d*(M_Band_z512_d_Volume) + (Band_z512_d);

      z512_e = 0;
      z512_e = z512_e*2 + (((l)>6+(1-1)));
      z512_e = z512_e*5 + M_z512_e_i5[__min(27,__max(0,(cblMDCTj)-(1-1)))];
      z512_e = z512_e*4 + M_z512_e_i6[__min(25,__max(0,(dblMDCTj)-(1-1)))];
      z512_e = z512_e*(M_Band_z512_e_Volume) + (Band_z512_e);
      const word* M_Sz512_wr = j>M_Sz512_j1 ? M_Sz512_wr0 : M_Sz512_wq0;
      const word* M_Sz512_wq = j>M_Sz512_j1 ? M_Sz512_wr1 : M_Sz512_wq1;
      const uint  M_Sz512_mw = j>M_Sz512_j1 ? M_Sz512_mw0 : M_Sz512_mw1;
      Sz512[Sz512_d].Process( rc, z512[z512_e], M_Sz512_wr, M_Sz512_mw, flag, M_Sz512_wq );
#endif
    }

    if( flag ) {
#ifndef DISABLE_NProc
      d=h; h=8207;
      c-=d; h-=d;
      //rc.NProcess( c, 14, M_mntLimit512, 8207 ); // c @ [512..8206)
      rc.NProcess( c, 8+(h>256)+(h>512)+(h>1024)+(h>2048)+(h>4096), 0, h );
      c+=d; h+=d;
//_t = 1;
#endif
    } else {

      {
        flag = (c>=M_mntLimit256);
#ifndef DISABLE_Sz256
        int Sz256_d,z256_e,Band_z256_e,Band_z256_d;

        Band_z256_d = 0;
        Band_z256_d = Band_z256_d*8 + M_Band_z256_d_i0[__min(576,__max(0,(j)-(1-1)))];

        Band_z256_e = 0;
        Band_z256_e = Band_z256_e*2 + (((j)>574+(1-1)));

        Sz256_d = 0;
        Sz256_d = Sz256_d*2 + (((ch)>0+(1-1)));
        Sz256_d = Sz256_d*2 + (((l)>6+(1-1)));
        Sz256_d = Sz256_d*2 + (((cblMDCTj)>13+(1-1)));
        Sz256_d = Sz256_d*2 + (((ctx2bl)>17+(1-1)));
        Sz256_d = Sz256_d*2 + (((zrun)>1+(1-1)));
        Sz256_d = Sz256_d*(M_Band_z256_d_Volume) + (Band_z256_d);

        z256_e = 0;
        z256_e = z256_e*4 + M_z256_e_i3[__min(16,__max(0,(l)-(1-1)))];
        z256_e = z256_e*2 + (((curband)>3+(1-1)));
        z256_e = z256_e*7 + M_z256_e_i5[__min(25,__max(0,(cblMDCTj)-(1-1)))];
        z256_e = z256_e*7 + M_z256_e_i6[__min(25,__max(0,(dblMDCTj)-(1-1)))];
        z256_e = z256_e*2 + (((ctx1bl)>19+(1-1)));
        z256_e = z256_e*(M_Band_z256_e_Volume) + (Band_z256_e);
        const word* M_Sz256_wr = j>M_Sz256_j1 ? M_Sz256_wr0 : M_Sz256_wq0;
        const word* M_Sz256_wq = j>M_Sz256_j1 ? M_Sz256_wr1 : M_Sz256_wq1;
        const uint  M_Sz256_mw = j>M_Sz256_j1 ? M_Sz256_mw0 : M_Sz256_mw1;
        Sz256[Sz256_d].Process( rc, z256[z256_e], M_Sz256_wr, M_Sz256_mw, flag, M_Sz256_wq );
#endif
        d = flag ? M_mntLimit256 : d;
        h = flag ? h : M_mntLimit256;
      }

      if( flag==0 ) {
        // c = 4+MntDepth .. 255
#ifndef DISABLE_Smant
//        int bit0 = c&1; c>>=1;
//        rc.BProcess( hSCALE, bit0 );

        const uint j_band = (j>M_Smant1_j1) + (j>M_Smant1_j2) + (j>M_Smant1_j3) + (j>M_Smant1_j4) + (j>M_Smant1_j5) + (j>M_Smant1_j6) + (j>M_Smant1_j7) + (j>M_Smant1_j8) + (j>=c1);
        int Smant_e,mant_f;

        Smant_e = 0;
        Smant_e = Smant_e*2 + (((ch)>0+(1-1)));
        Smant_e = Smant_e*2 + (((j_band)>0+(1-1)));
        Smant_e = Smant_e*9 + M_Smant_e_i4[__min(21,__max(0,(cblMDCTj)-(1-1)))];
        Smant_e = Smant_e*2 + (((ctx1bl)>7+(1-1)));
        Smant_e = Smant_e*2 + (((zrun)>2+(1-1)));
        Smant_e = Smant_e*(M_Jmant_e_Volume) + (0);

        mant_f = 0;
        mant_f = mant_f*3 + (((j_band)>1+(1-1))+((j_band)>6+(1-1)));
        mant_f = mant_f*2 + (((curband)>1+(1-1)));
        mant_f = mant_f*7 + M_mant_f_i2[__min(16,__max(0,(l)-(1-1)))];
        mant_f = mant_f*4 + M_mant_f_i5[__min(21,__max(0,(dblMDCTj)-(1-1)))];
        mant_f = mant_f*2 + (((ctx2bl)>11+(1-1)));
        mant_f = mant_f*(M_Jmant_f_Volume) + (0);

        const word* M_mant_wr[] = { M_mant_wr8,M_mant_wr7,M_mant_wr6,M_mant_wr5,M_mant_wr4,M_mant_wr3,M_mant_wr2,M_mant_wr1,M_mant_wr0 };
        const word* M_mant_wq[] = { M_mant_wq8,M_mant_wq7,M_mant_wq6,M_mant_wq5,M_mant_wq4,M_mant_wq3,M_mant_wq2,M_mant_wq1,M_mant_wq0 };
        const uint  M_mant_mw[] = { M_mant_mw8,M_mant_mw7,M_mant_mw6,M_mant_mw5,M_mant_mw4,M_mant_mw3,M_mant_mw2,M_mant_mw1,M_mant_mw0 };

        int lo = d, hi = h; enum{ len=8 };
        int i,j,k,z, Jmant_e,Jmant_f, bas=M_Jmant_bas; 

        c += bas; lo += bas; hi += bas;
        for( i=len,j=1,z=0; i>=0; i-- ) {
          if( z+(1<<i)-1< lo ) k=1; 
     else if( z+(1<<i)  >=hi ) k=0; 
     else {
            k = (c>>i) & 1;

            Jmant_e = 0;
            Jmant_e = (Jmant_e*51) + M_Jmant_e_i1[j]; // 001011111

            Jmant_f = 0;
            Jmant_f = (Jmant_f*48) + M_Jmant_f_i1[j]; // 100011110
            Smant[Smant_e+Jmant_e].Process( rc, mant[mant_f+Jmant_f], M_mant_wr[i], M_mant_mw[i], k, M_mant_wq[i] );
          }
          z |= (k<<i);
          j += j + k;
        }
        c = z;
        c -= bas;

//        c = c*2 + bit0;
//_t = 2;
#endif

      } else {

#ifndef DISABLE_Smbnt
        c-=d; h-=d;
        rc.NProcess( c, 8+(h>256)+(h>512)+(h>1024)+(h>2048)+(h>4096), 0, h ); // c @ [M_mntLimit256..M_mntLimit512)
        c+=d; h+=d;
//_t = 3;
#endif
      }


    } // <512

//if( c!=_x ) printf( "%i %i  %i %i %i  %i\n", c, _x, M_mntDepth, M_mntLimit256, M_mntLimit512, _t );
  }


SignToProcess:
  if( c==1 ) {
#ifndef DISABLE_Sms1
    int pms1_f, Sms1_d, ms1_e, mms1_g,ns1_e;

    pms1_f = 0;
    pms1_f = pms1_f*2 + (((band_j)>0+(1-1)));
    pms1_f = pms1_f*2 + (((curband)>7+(1-1)));
    pms1_f = pms1_f*15 + M_pms1_f_i4[__min(40,__max(0,(cMDCTj)-(-16-1)))];
    pms1_f = (pms1_f<<1) + (((byte(signs_j))&1)); // 00000001
    pms1_f = (pms1_f<<1) + (((byte(zeros_j))&1)); // 00000001
    byte& rec = pms1[pms1_f];

    ms1_e = 0;
    ms1_e = ms1_e*2 + (((ch)>0+(1-1)));
    ms1_e = ms1_e*2 + (((cMDCTj)>9+(-8-1)));
    ms1_e = ms1_e*4 + M_ms1_e_i4[__min(20,__max(0,(dMDCTj)-(-8-1)))];
    ms1_e = ms1_e*3 + (((eMDCTj)>6+(-8-1))+((eMDCTj)>10+(-8-1)));
    ms1_e = (ms1_e<<3) + (((byte(signs_j))&7)); // 00000111
    ms1_e = (ms1_e<<1) + ((((byte(zeros_j))>>2)&1)); // 00000100
    ms1_e = (ms1_e<<5) + (((rec)&31)); // 00011111

    Sms1_d = 0;
    Sms1_d = Sms1_d*2 + (((ch)>0+(1-1)));
    Sms1_d = Sms1_d*5 + M_Sms1_d_i1[__min(9,__max(0,(j_band)-(1-1)))];
    Sms1_d = Sms1_d*4 + M_Sms1_d_id[__min(21,__max(0,(band_j)-(1-1)))];
    Sms1_d = Sms1_d*6 + M_Sms1_d_i3[__min(25,__max(0,(cMDCTj)-(-12-1)))];

    mms1_g = 0;
    mms1_g = mms1_g*2 + (((ch)>0+(1-1)));
    mms1_g = mms1_g*2 + (((j_band)>5+(1-1)));
    mms1_g = mms1_g*5 + M_mms1_g_i2[__min(21,__max(0,(band_j)-(1-1)))];
    mms1_g = mms1_g*2 + (((curband)>9+(1-1)));
    mms1_g = mms1_g*4 + M_mms1_g_i4[__min(16,__max(0,(cMDCTj)-(-8-1)))];
    mms1_g = mms1_g*3 + (((cMDCTjm1)>2+(-8-1))+((cMDCTjm1)>14+(-8-1)));
    mms1_g = mms1_g*3 + (((ctx1)>2+(-8-1))+((ctx1)>14+(-8-1)));
    mms1_g = (mms1_g<<1) + (((byte(zeros_j))&1)); // 00000001

    ns1_e = 0;
    ns1_e = ns1_e*2 + (((ch)>0+(1-1)));
    ns1_e = ns1_e*3 + (((cMDCTj)>8+(-8-1))+((cMDCTj)>9+(-8-1)));
    ns1_e = ns1_e*3 + (((dMDCTj)>7+(-8-1))+((dMDCTj)>10+(-8-1)));
    ns1_e = ns1_e*2 + (((eMDCTj)>8+(-8-1)));
    ns1_e = ns1_e*5 + M_ns1_e_i7[__min(16,__max(0,(cMDCTjm1)-(-8-1)))];
    ns1_e = ns1_e*6 + M_ns1_e_i8[__min(17,__max(0,(ctx1)-(-8-1)))];
    ns1_e = (ns1_e<<1) + ((((byte(signs_j))>>1)&1)); // 00000010
    ns1_e = ns1_e*2 + (((zrun)>0+(1-1)));

    const word* M_Sms1_wr = j>M_Sms1_jj ? M_Sms1_wr0 : M_Sms1_wq0;
    const word* M_Sms1_wq = j>M_Sms1_jj ? M_Sms1_wr1 : M_Sms1_wq1;
    const uint  M_Sms1_mw = j>M_Sms1_jj ? M_Sms1_mw0 : M_Sms1_mw1;
    const word* M_Sns1_wr = j>M_Sns1_jj ? M_Sns1_wr0 : M_Sns1_wq0;
    const uint  M_Sns1_mw = j>M_Sns1_jj ? M_Sns1_mw0 : M_Sns1_mw1;

    SSEii_updstr X;
    iMixer& M = mms1[mms1_g];
    Node2ii& C = ms1[ms1_e];
    Node2ii& D = ns1[ns1_e];
    auto& S = Sms1[Sms1_d];
    int p_sse = S.SSE_Pred( C.P, X );

    int p1 = M.st( D.P );
    int p2 = M.st( p_sse );
    int p3 = M.Mixup( p1, p2, M_mms1_wr );
    int p3sq = Max<int>(1,Min<int>(SCALE-1, M.sq(p3) ));

    rc.BProcess( p3sq, s );

    M.Update( s, p1, p2, M_mms1_wq, p3sq );
    S.SSE_Update( s, M_Sms1_wq, X );
    C.Update( s, M_Sms1_wr, M_Sms1_mw );
    D.Update( s, M_Sns1_wr, M_Sns1_mw );

    rec = rec*2 + s;
#endif
  } else {
#ifndef DISABLE_Sms
    int pms_f, Sms_d, ms_e, mms_g,ns_e, band_Sms, band_Sns;

    pms_f = 0;
    pms_f = pms_f*2 + (((ch)>0+(1-1)));
    pms_f = pms_f*16 + __min(15,__max(0,(band_j)-(1-1)));
    pms_f = pms_f*4 + M_pms_f_i2[__min(9,__max(0,(j_band)-(1-1)))];
    pms_f = pms_f*6 + M_pms_f_i4[__min(41,__max(0,(cMDCTj)-(-12-1)))];
    pms_f = pms_f*2 + (((dMDCTj)>5+(-12-1)));
    pms_f = pms_f*2 + (((c)>19+(-8-1)));
    pms_f = (pms_f<<1) + ((((byte(signs_j))>>2)&1)); // 00000100

    band_Sms = 0;

    band_Sns = 0;
    band_Sns = band_Sns*123 + M_band_Sns_i0[__min(576,__max(0,(j)-(1-1)))];
    byte& rec = pms[pms_f];

    ms_e = 0;
    ms_e = ms_e*2 + (((ch)>0+(1-1)));
    ms_e = ms_e*5 + M_ms_e_i4[__min(32,__max(0,(cMDCTj)-(-12-1)))];
    ms_e = ms_e*9 + M_ms_e_i5[__min(32,__max(0,(dMDCTj)-(-12-1)))];
    ms_e = ms_e*5 + M_ms_e_i6[__min(32,__max(0,(eMDCTj)-(-12-1)))];
    ms_e = ms_e*2 + (((c)>14+(-8-1)));
    ms_e = (ms_e<<5) + ((((byte(signs_j))>>2)&31)); // 01111100
    ms_e = (ms_e<<1) + ((((byte(zeros_j))>>2)&1)); // 00000100
    ms_e = ms_e*(M_band_Sms_Volume) + (band_Sms);

    Sms_d = 0;
    Sms_d = Sms_d*2 + (((ch)>0+(1-1)));
    Sms_d = Sms_d*4 + M_Sms_d_i1[__min(9,__max(0,(j_band)-(1-1)))];
    Sms_d = Sms_d*7 + M_Sms_d_i2[__min(21,__max(0,(band_j)-(1-1)))];
    Sms_d = Sms_d*3 + (((cMDCTj)>12+(-12-1))+((cMDCTj)>13+(-12-1)));
    Sms_d = Sms_d*2 + (((c)>20+(-8-1)));
    Sms_d = (Sms_d<<1) + (((rec)&1)); // 00000001

    mms_g = 0;
    mms_g = mms_g*2 + (((ch)>0+(1-1)));
    mms_g = mms_g*5 + M_mms_g_i1[__min(21,__max(0,(band_j)-(1-1)))];
    mms_g = mms_g*3 + (((j_band)>1+(1-1))+((j_band)>3+(1-1)));
    mms_g = mms_g*2 + (((curband)>9+(1-1)));
    mms_g = mms_g*9 + M_mms_g_i4[__min(26,__max(0,(cMDCTj)-(-12-1)))];
    mms_g = mms_g*4 + M_mms_g_i8[__min(25,__max(0,(c)-(-8-1)))];
    mms_g = (mms_g<<1) + (((byte(zeros_j))&1)); // 00000001

    ns_e = 0;
    ns_e = ns_e*12 + __min(11,__max(0,(band_j)-(1-1)));
    ns_e = ns_e*4 + M_ns_e_i5[__min(21,__max(0,(dMDCTj)-(-8-1)))];
    ns_e = (ns_e<<2) + (((((byte(signs_j))>>5)&1)<<1) + (((byte(signs_j))>>3)&1)); // 00101000
    ns_e = ns_e*(M_band_Sns_Volume) + (band_Sns);
    const word* M_Sms_wr = j>M_Sms_jj ? M_Sms_wr0 : M_Sms_wq0;
    const word* M_Sms_wq = j>M_Sms_jj ? M_Sms_wr1 : M_Sms_wq1;
    const uint  M_Sms_mw = j>M_Sms_jj ? M_Sms_mw0 : M_Sms_mw1;
    const word* M_Sns_wr = j>M_Sns_jj ? M_Sns_wr0 : M_Sns_wq0;
    const uint  M_Sns_mw = j>M_Sns_jj ? M_Sns_mw0 : M_Sns_mw1;

    SSEii_updstr X;
    iMixer& M = mms[mms_g];
    Node2ii& C = ms[ms_e];
    Node2ii& D = ns[ns_e];
    auto& S = Sms[Sms_d];
    int p_sse = S.SSE_Pred( C.P, X );

    int p1 = M.st( D.P );
    int p2 = M.st( p_sse );
    int p3 = M.Mixup( p1, p2, M_mms_wr );
    int p3sq = Max<int>(1,Min<int>(SCALE-1, M.sq(p3) ));

    rc.BProcess( p3sq, s );

    M.Update( s, p1, p2, M_mms_wq, p3sq );
    S.SSE_Update( s, M_Sms_wq, X );
    C.Update( s, M_Sms_wr, M_Sms_mw );
    D.Update( s, M_Sns_wr, M_Sns_mw );

    rec = rec*2 + s;
#endif
    av_MDCT[j] = (av_MDCT[j]*M_av_D0)/256 + l_abs(c);
    ctext = aext;            
  }
  c = s?-c:c;

Processed:

  ctx3 = ctx2;
  ctx2 = ctx1;
  ctx1 = c;

  (signs[j]<<=1) += (c<0);
  (zeros[j]<<=1) += (c==0);
//  (plus1[j]<<=1) += (c==1);
//  (m1nus[j]<<=1) += (c==-1);

  MDCT_j = c;

  dtx1 = cMDCT[j];

  ShiftMDCT(j,c);
}

void ShiftMDCT( int j, int c ) {
  cStep[j]=xstp[j];
  eMDCT[j]=dMDCT[j];
  dMDCT[j]=cMDCT[j]; cMDCT[j]=c;
}


#undef SC8
