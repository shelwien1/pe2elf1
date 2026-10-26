
// #define DISABLE_c0c1       
// #define DISABLE_bvflag     
// #define DISABLE_bvdump     
// #define DISABLE_xbigval0   
// #define DISABLE_xbigval1   
                              
// #define DISABLE_rg0cnt     
// #define DISABLE_rg1cnt     
// #define DISABLE_tblsel     
                              
// #define DISABLE_fdatlen    
// #define DISABLE_fgrblen    
// #define DISABLE_NGrbLen    
// #define DISABLE_xdatlen    
// #define DISABLE_grb_b1     
                              
// #define DISABLE_hdrflgs    
// #define DISABLE_glgain     
// #define DISABLE_sfcomp     
// #define DISABLE_scfscl     
// #define DISABLE_cn1tbl     
// #define DISABLE_bltyp1     
// #define DISABLE_bltyp2     
// #define DISABLE_mixblk     
// #define DISABLE_sbgain     
                              
// #define DISABLE_f_mdb      
// #define DISABLE_mdbmain    
                              
// #define DISABLE_p23clr     
// #define DISABLE_p23back    
// #define DISABLE_ypart23    
// #define DISABLE_spart23    
// #define DISABLE_zpart23    
                              
// #define DISABLE_s1mixed    
// #define DISABLE_s1small    
// #define DISABLE_s1scfsi    
// #define DISABLE_s1xlong    
                              
// #define DISABLE_s2ylong    






//Table( Number1ii<4>,  xlong, C_xlong_c_Volume );

// the scalefactors' mixing stage (Process_sf1xlong): the counters of its
// contexts ya..yd, a row holding the 32 nodes of a scalefactor's binary
// tree; the mixer's weight sets (yw and the node) and their update counts

// MPEG-1's short-block scalefactors' stage (Process_sf1short), the same
// with rows of 16 nodes

// MPEG-2's scalefactors' stage (Process_sf2), the same with rows of 64 nodes






  // (the types of Table()s count0, Scount0, count1 and Scount1 are model__mp3.inc's,
  // where struct C_T, which holds them, needs them)

//  SSEj<*(int*)& C_Sxlong_Q,*(int*)& C_Sxlong_d_Volume> Sxlong;
//  SSEj<*(int*)& C_Sglgain_Q,*(int*)& C_Sglgain_d_Volume> Sglgain;
//  SSEj<*(int*)& C_Ssfcomp_Q,*(int*)& C_Ssfcomp_d_Volume> Ssfcomp;
//  SSEj<*(int*)& C_Srg0cnt_Q,*(int*)& C_Srg0cnt_d_Volume> Srg0cnt;
//  SSEj<*(int*)& C_Srg1cnt_Q,*(int*)& C_Srg1cnt_d_Volume> Srg1cnt;

  int p_c1[8];
  int p_c0[8];

  // MPEG-1's long-block scalefactors of each channel's last granule, and
  // its global_gain, scalefac_scale and preflag (ProcessScale1 keeps them,
  // for the stage)
  byte ysf[2][22];
  int  ygg[2], yscs[2], ypre[2];
  // MPEG-1's short-block scalefactors of each channel's last short
  // granule (ProcessScale1 keeps them)
  byte ssf[2][12][3];
  // MPEG-2's scalefactors of each channel's last frame, for each kind of
  // block (0 long, 1 short, 2 mixed), and its global_gain and
  // scalefac_scale (ProcessScale2 keeps them)
  byte zsf[2][3][40];
  int  zgg[2], zscs[2];

//  Number1ii<4>    x_sfcomp[2];
//  Number1ii<4>    x_rg0cnt[2];
//  Number1ii<3>    x_rg1cnt[32];


// the tables' starting state, after C_Init() made them
void C_InitData( void ) {

  int i;

  for( i=0; i<C_mc0_e_Volume; i++ ) count0[i].Init_Number2i();
  for( i=0; i<C_Smc0_d_Volume; i++ ) Init_SSEii( Scount0[i] );
  for( i=0; i<C_mc00_e_Volume; i++ ) count00[i].Init_Node2i();

  for( i=0; i<C_mc1_e_Volume; i++ ) count1[i].Init_Number2i();
  for( i=0; i<C_Smc1_d_Volume; i++ ) Init_SSEii( Scount1[i] );
  for( i=0; i<C_mc10_e_Volume; i++ ) count10[i].Init_Node2i();

  memset( &p_c0, 0, sizeof(p_c0) );
  memset( &p_c1, 0, sizeof(p_c1) );

  for( i=0; i<C_xlong_c_Volume; i++ ) xlong[i].Init_Node2i();
  for( i=0; i<C_Sxlong_d_Volume; i++ ) Sxlong[i].Init_SSEi(C_Sxlong_wi,C_Sxlong_ti);
  for( i=0; i<C_pxlong_e_Volume; i++ ) pxlong[i]=0;
  for( i=0; i<C_pxlong_f_Volume; i++ ) qxlong[i]=0;

  // the scalefactors' stage: counters at 1/2, each weight set taking mp3c's
  // two predictions half and half
  for( i=0; i<C_ya_Volume*32; i++ ) yA[i].Init();
  for( i=0; i<C_yb_Volume*32; i++ ) yB[i].Init();
  for( i=0; i<C_yc_Volume*32; i++ ) yC[i].Init();
  for( i=0; i<C_yd_Volume*32; i++ ) yD[i].Init();
  for( i=0; i<C_yw_Volume*32; i++ ) {
    for( int k=0; k<XNIN; k++ ) yW[i*XNIN+k] = k==0 ? C_ywa : k==10 ? C_ywb : 0;
    yWN[i] = 0;
  }
  memset( ysf, 0, sizeof(ysf) );
  ygg[0] = ygg[1] = 0; yscs[0] = yscs[1] = 0; ypre[0] = ypre[1] = 0;

  for( i=0; i<C_za_Volume*64; i++ ) zA[i].Init();
  for( i=0; i<C_zb_Volume*64; i++ ) zB[i].Init();
  for( i=0; i<C_zc_Volume*64; i++ ) zC[i].Init();
  for( i=0; i<C_zd_Volume*64; i++ ) zD[i].Init();
  for( i=0; i<C_zw_Volume*64; i++ ) {
    for( int k=0; k<XNIN; k++ ) zW[i*XNIN+k] = k==0 ? C_ywa : k==10 ? C_ywb : 0;
    zWN[i] = 0;
  }
  memset( zsf, 0, sizeof(zsf) );
  zgg[0] = zgg[1] = 0; zscs[0] = zscs[1] = 0;

  for( i=0; i<C_sa_Volume*16; i++ ) sA[i].Init();
  for( i=0; i<C_sb_Volume*16; i++ ) sB[i].Init();
  for( i=0; i<C_sc_Volume*16; i++ ) sC[i].Init();
  for( i=0; i<C_sd_Volume*16; i++ ) sD[i].Init();
  for( i=0; i<C_sw_Volume*16; i++ ) {
    for( int k=0; k<XNIN; k++ ) sW[i*XNIN+k] = k==0 ? C_ywa : k==10 ? C_ywb : 0;
    sWN[i] = 0;
  }
  memset( ssf, 0, sizeof(ssf) );

//  Init_Number1ii( x_glgain );
  for( i=0; i<C_glgain_c_Volume; i++ ) glgain[i].Init_Node2i();
  for( i=0; i<C_Sglgain_d_Volume; i++ ) Sglgain[i].Init_SSEi(C_Sglgain_wi,C_Sglgain_ti);
  for( i=0; i<C_pglgain_f_Volume; i++ ) qglgain[i]=0;
  for( i=0; i<C_pglgain_e_Volume; i++ ) pglgain[i]=0;
  for( i=0; i<C_pglgain_e_Volume; i++ ) pglgaio[i]=0;

//  Init_Number1ii( x_sfcomp );
  for( i=0; i<C_sfcomp_c_Volume; i++ ) sfcomp[i].Init_Node2i();
  for( i=0; i<C_Ssfcomp_d_Volume; i++ ) Ssfcomp[i].Init_SSEi(C_Ssfcomp_wi,C_Ssfcomp_ti);
  for( i=0; i<C_psfcomp_f_Volume; i++ ) qsfcomp[i]=0;
  for( i=0; i<C_psfcomp_e_Volume; i++ ) psfcomp[i]=0;
  for( i=0; i<C_psfcomp_e_Volume; i++ ) psfcomq[i]=0;

//  Init_Number1ii( x_rg0cnt );
  for( i=0; i<C_rg0cnt_c_Volume; i++ ) rg0cnt[i].Init_Node2i();
  for( i=0; i<C_Srg0cnt_d_Volume; i++ ) Srg0cnt[i].Init_SSEi(C_Srg0cnt_wi,C_Srg0cnt_ti);
  for( i=0; i<C_prg0cnt_f_Volume; i++ ) qrg0cnt[i]=0;
  for( i=0; i<C_prg0cnt_e_Volume; i++ ) prg0cnt[i]=0;
  for( i=0; i<C_prg0cnt_e_Volume; i++ ) prg0cnu[i]=0;

//  Init_Number1ii( x_rg1cnt );
  for( i=0; i<C_rg1cnt_c_Volume; i++ ) rg1cnt[i].Init_Node2i();
  for( i=0; i<C_Srg1cnt_d_Volume; i++ ) Srg1cnt[i].Init_SSEi(C_Srg1cnt_wi,C_Srg1cnt_ti);
  for( i=0; i<C_prg1cnt_f_Volume; i++ ) qrg1cnt[i]=0;
  for( i=0; i<C_prg1cnt_e_Volume; i++ ) prg1cnt[i]=0;
  for( i=0; i<C_prg1cnt_e_Volume; i++ ) prg1cnu[i]=0;

}

void Process_rgxcnt( int& c, int& _c1, int ch ) {
  RangeCoder& rc = *this;

  //x_rg0cnt[ch].Process( rc, c, -1,16, C_x_rg0cnt_wr, C_x_rg0cnt_mw );
{
//int c0=c;

  int rg0cnt_c,Srg0cnt_d,prg0cnt_e,prg0cnt_f;

  prg0cnt_e = 0;
  byte& ctx1 = prg0cnu[prg0cnt_e];
  byte& ctx0 = prg0cnt[prg0cnt_e];
  char rec = ctx0 - ctx1;

//printf( "%i ", rec );


  prg0cnt_f = 0;
  prg0cnt_f = prg0cnt_f*2 + (((ch)>0+(1-1)));
  prg0cnt_f = prg0cnt_f*8 + C_prg0cnt_f_i2[__min(28,__max(0,(rec)-(-12-1)))];
  prg0cnt_f = (prg0cnt_f<<1) + ((((ctx0)>>3)&1)); // 1000
  prg0cnt_f = prg0cnt_f*(C_Jrg0cnt_g_Volume) + (0);

  rg0cnt_c = 0;
  rg0cnt_c = rg0cnt_c*(C_Jrg0cnt_f_Volume) + (0);

  Srg0cnt_d = 0;
  Srg0cnt_d = Srg0cnt_d*2 + (((ch)>0+(1-1)));
  Srg0cnt_d = Srg0cnt_d*2 + (((rec)>25+(-12-1)));
  Srg0cnt_d = Srg0cnt_d*(C_Jrg0cnt_e_Volume) + (0);

  {
    const word* rg0cnt_wr[] = { C_rg0cnt_wr4,C_rg0cnt_wr3,C_rg0cnt_wr2,C_rg0cnt_wr1,C_rg0cnt_wr0 };
    const word* rg0cnt_wq[] = { C_rg0cnt_wq4,C_rg0cnt_wq3,C_rg0cnt_wq2,C_rg0cnt_wq1,C_rg0cnt_wq0 };
    const uint  rg0cnt_mw[] = { C_rg0cnt_mw4,C_rg0cnt_mw3,C_rg0cnt_mw2,C_rg0cnt_mw1,C_rg0cnt_mw0 };

    int lo = 0, hi = 1<<4; enum{ len=4 };
    int i,j,k,z, Jrg0cnt_e,Jrg0cnt_f,Jrg0cnt_g, bas=C_rg0cnt_bas; 

//    c = (c-(ctx0-0))&15;  
    c += bas; lo += bas; hi += bas;
    for( i=len,j=1,z=0; i>=0; i-- ) {
      if( z+(1<<i)-1< lo ) k=1; 
  else if( z+(1<<i)  >=hi ) k=0; 
  else {
        k = (c>>i) & 1;

        Jrg0cnt_g = 0;
        Jrg0cnt_g = (Jrg0cnt_g*31) + C_Jrg0cnt_g_i1[j]; // 00001
        byte& old = qrg0cnt[prg0cnt_f+Jrg0cnt_g];

        Jrg0cnt_e = 0;
        Jrg0cnt_e = (Jrg0cnt_e*31) + C_Jrg0cnt_e_i1[j]; // 00001

        Jrg0cnt_f = 0;
        Jrg0cnt_f = (Jrg0cnt_f*32) + (j); // 00000
        Srg0cnt[Srg0cnt_d+Jrg0cnt_e].Process( rc, rg0cnt[rg0cnt_c+Jrg0cnt_f], rg0cnt_wr[i], rg0cnt_mw[i], k, rg0cnt_wq[i] );
        old = old*2 + k;
      }
      z |= (k<<i);
      j += j + k;
    }
    c = z;

    c -= bas; 
//    c = (c+(ctx0+0))&15;
  }

//if( c!=c0 ) printf( "%i %i\n", c, c0 );

  ctx1 = ctx0;
  ctx0 = c;
}

//  x_rg1cnt[ch+2*c].Process( rc, _c1,-1,8, C_x_rg1cnt_wr, C_x_rg1cnt_mw );
{ int rg0=c; int c=_c1;
//int c0=c;

  int rg1cnt_c,Srg1cnt_d,prg1cnt_e,prg1cnt_f;

  prg1cnt_e = 0;
  byte& ctx1 = prg1cnu[prg1cnt_e];
  byte& ctx0 = prg1cnt[prg1cnt_e];
  char rec = ctx0 - ctx1;

//printf( "%i ", rec );


  prg1cnt_f = 0;
  prg1cnt_f = prg1cnt_f*3 + (((rec)>8+(-12-1))+((rec)>19+(-12-1)));
  prg1cnt_f = (prg1cnt_f<<4) + (((rg0)&15)); // 1111
  prg1cnt_f = prg1cnt_f*(C_Jrg1cnt_g_Volume) + (0);

  rg1cnt_c = 0;
  rg1cnt_c = (rg1cnt_c<<4) + (((rg0)&15)); // 1111
  rg1cnt_c = rg1cnt_c*(C_Jrg1cnt_f_Volume) + (0);

  Srg1cnt_d = 0;
  Srg1cnt_d = Srg1cnt_d*2 + (((rec)>25+(-12-1)));
  Srg1cnt_d = Srg1cnt_d*7 + C_Srg1cnt_d_i6[__min(16,__max(0,(rg0)-(1-1)))];
  Srg1cnt_d = Srg1cnt_d*(C_Jrg1cnt_e_Volume) + (0);

  {
    const word* rg1cnt_wr[] = { C_rg1cnt_wr3,C_rg1cnt_wr2,C_rg1cnt_wr1,C_rg1cnt_wr0 };
    const word* rg1cnt_wq[] = { C_rg1cnt_wq3,C_rg1cnt_wq2,C_rg1cnt_wq1,C_rg1cnt_wq0 };
    const uint  rg1cnt_mw[] = { C_rg1cnt_mw3,C_rg1cnt_mw2,C_rg1cnt_mw1,C_rg1cnt_mw0 };

    int lo = 0, hi = 1<<3; enum{ len=3 };
    int i,j,k,z, Jrg1cnt_e,Jrg1cnt_f,Jrg1cnt_g, bas=C_rg1cnt_bas; 

//    c = (c-(ctx0-0))&7;  
    c += bas; lo += bas; hi += bas;
    for( i=len,j=1,z=0; i>=0; i-- ) {
      if( z+(1<<i)-1< lo ) k=1; 
  else if( z+(1<<i)  >=hi ) k=0; 
  else {
        k = (c>>i) & 1;

        Jrg1cnt_g = 0;
        Jrg1cnt_g = (Jrg1cnt_g*15) + C_Jrg1cnt_g_i1[j]; // 0001
        byte& old = qrg1cnt[prg1cnt_f+Jrg1cnt_g];

        Jrg1cnt_e = 0;
        Jrg1cnt_e = (Jrg1cnt_e<<1) + (((old)&1)); // 0001
        Jrg1cnt_e = (Jrg1cnt_e*11) + C_Jrg1cnt_e_i1[j]; // 0011

        Jrg1cnt_f = 0;
        Jrg1cnt_f = (Jrg1cnt_f*16) + (j); // 0000
        Srg1cnt[Srg1cnt_d+Jrg1cnt_e].Process( rc, rg1cnt[rg1cnt_c+Jrg1cnt_f], rg1cnt_wr[i], rg1cnt_mw[i], k, rg1cnt_wq[i] );
        old = old*2 + k;
      }
      z |= (k<<i);
      j += j + k;
    }
    c = z;

    c -= bas; 
//    c = (c+(ctx0+0))&7;
  }

//if( c!=c0 ) printf( "%i %i\n", c, c0 );

  ctx1 = ctx0;
  ctx0 = c;
  _c1 = c;
}

} // return


void Process_sfcomp( int& c, int ch ) {
  RangeCoder& rc = *this;
//  x_sfcomp[ch].Process( rc, c, -1, 16, C_x_sfcomp_wr, C_x_sfcomp_mw ); // 4 bits

//int c0=c;

  int sfcomp_c,Ssfcomp_d,psfcomp_e,psfcomp_f;

  psfcomp_e = 0;
  byte& ctx1 = psfcomq[psfcomp_e];
  byte& ctx0 = psfcomp[psfcomp_e];
  char rec = ctx0 - ctx1;

//printf( "%i ", rec );


  psfcomp_f = 0;
  psfcomp_f = psfcomp_f*6 + C_psfcomp_d_i2[__min(28,__max(0,(rec)-(-12-1)))];
  psfcomp_f = (psfcomp_f<<4) + (((ctx0)&15)); // 1111
  psfcomp_f = psfcomp_f*(C_Jsfcomp_g_Volume) + (0);

  sfcomp_c = 0;
  sfcomp_c = sfcomp_c*8 + C_sfcomp_c_i2[__min(28,__max(0,(rec)-(-12-1)))];
  sfcomp_c = (sfcomp_c<<3) + ((((ctx0)>>1)&7)); // 1110
  sfcomp_c = sfcomp_c*(C_Jsfcomp_f_Volume) + (0);

  Ssfcomp_d = 0;
  Ssfcomp_d = Ssfcomp_d*5 + C_Ssfcomp_d_i2[__min(28,__max(0,(rec)-(-12-1)))];
  Ssfcomp_d = (Ssfcomp_d<<3) + (((((ctx0)>>3)&1)<<2) + ((ctx0)&3)); // 1011
  Ssfcomp_d = Ssfcomp_d*(C_Jsfcomp_e_Volume) + (0);

  {
    const word* sfcomp_wr[] = { C_sfcomp_wr4,C_sfcomp_wr3,C_sfcomp_wr2,C_sfcomp_wr1,C_sfcomp_wr0 };
    const word* sfcomp_wq[] = { C_sfcomp_wq4,C_sfcomp_wq3,C_sfcomp_wq2,C_sfcomp_wq1,C_sfcomp_wq0 };
    const uint  sfcomp_mw[] = { C_sfcomp_mw4,C_sfcomp_mw3,C_sfcomp_mw2,C_sfcomp_mw1,C_sfcomp_mw0 };

    int lo = 0, hi = 1<<4; enum{ len=4 };
    int i,j,k,z, Jsfcomp_e,Jsfcomp_f,Jsfcomp_g, bas=C_sfcomp_bas; 

//    c = (c-(ctx0-0))&15;  
    c += bas; lo += bas; hi += bas;
    for( i=len,j=1,z=0; i>=0; i-- ) {
      if( z+(1<<i)-1< lo ) k=1; 
  else if( z+(1<<i)  >=hi ) k=0; 
  else {
        k = (c>>i) & 1;

        Jsfcomp_g = 0;
        Jsfcomp_g = (Jsfcomp_g*32) + (j); // 00000
        byte& old = qsfcomp[psfcomp_f+Jsfcomp_g];

        Jsfcomp_e = 0;
        Jsfcomp_e = (Jsfcomp_e*32) + (j); // 00000

        Jsfcomp_f = 0;
        Jsfcomp_f = (Jsfcomp_f*32) + (j); // 00000
        Ssfcomp[Ssfcomp_d+Jsfcomp_e].Process( rc, sfcomp[sfcomp_c+Jsfcomp_f], sfcomp_wr[i], sfcomp_mw[i], k, sfcomp_wq[i] );
        old = old*2 + k;
      }
      z |= (k<<i);
      j += j + k;
    }
    c = z;

    c -= bas; 
//    c = (c+(ctx0+0))&15;
  }

//if( c!=c0 ) printf( "%i %i\n", c, c0 );

  ctx1 = ctx0;
  ctx0 = c;
}


void Process_GlGain( int& c, int ch, int gr ) {
  RangeCoder& rc = *this;

//int c0=c;

//  x_glgain[0].Process( rc, c, -1, 256, C_x_glgain_0_wr, C_x_glgain_0_mw );
  int glgain_c,Sglgain_d,pglgain_e,pglgain_f;

  pglgain_e = 0;
  pglgain_e = pglgain_e*2 + (((ch)>0+(1-1)));
  byte& ctx1 = pglgaio[pglgain_e];
  byte& ctx0 = pglgain[pglgain_e];
  char rec = ctx0 - ctx1;

//printf( "%i ", rec );


  pglgain_f = 0;
  pglgain_f = pglgain_f*2 + (((gr)>0+(1-1)));
  pglgain_f = pglgain_f*2 + (((rec)>5+(-12-1)));
  pglgain_f = pglgain_f*(C_Jglgain_g_Volume) + (0);

  glgain_c = 0;
  glgain_c = (glgain_c<<7) + (((ctx0)&127)); // 01111111
  glgain_c = glgain_c*(C_Jglgain_f_Volume) + (0);

  Sglgain_d = 0;
  Sglgain_d = Sglgain_d*9 + C_Sglgain_d_i2[__min(32,__max(0,(rec)-(-12-1)))];
  Sglgain_d = Sglgain_d*(C_Jglgain_e_Volume) + (0);

  {
    const word* glgain_wr[] = { C_glgain_wr8,C_glgain_wr7,C_glgain_wr6,C_glgain_wr5,C_glgain_wr4,C_glgain_wr3,C_glgain_wr2,C_glgain_wr1,C_glgain_wr0 };
    const word* glgain_wq[] = { C_glgain_wq8,C_glgain_wq7,C_glgain_wq6,C_glgain_wq5,C_glgain_wq4,C_glgain_wq3,C_glgain_wq2,C_glgain_wq1,C_glgain_wq0 };
    const uint  glgain_mw[] = { C_glgain_mw8,C_glgain_mw7,C_glgain_mw6,C_glgain_mw5,C_glgain_mw4,C_glgain_mw3,C_glgain_mw2,C_glgain_mw1,C_glgain_mw0 };

    int lo = 0, hi = 1<<8; enum{ len=8 };
    int i,j,k,z, Jglgain_e,Jglgain_f,Jglgain_g, bas=C_glgain_bas; 

    c = byte(c-(ctx0-0)); c += bas; lo += bas; hi += bas;
    for( i=len,j=1,z=0; i>=0; i-- ) {
      if( z+(1<<i)-1< lo ) k=1; 
  else if( z+(1<<i)  >=hi ) k=0; 
  else {
        k = (c>>i) & 1;

        Jglgain_g = 0;
        Jglgain_g = (Jglgain_g*130) + C_Jglgain_g_i1[j]; // 101000000
        byte& old = qglgain[pglgain_f+Jglgain_g];

        Jglgain_e = 0;
        Jglgain_e = (Jglgain_e<<1) + (((old)&1)); // 00000001
        Jglgain_e = (Jglgain_e*41) + C_Jglgain_e_i1[j]; // 011101000

        Jglgain_f = 0;
        Jglgain_f = (Jglgain_f*75) + C_Jglgain_f_i1[j]; // 011001000
        Sglgain[Sglgain_d+Jglgain_e].Process( rc, glgain[glgain_c+Jglgain_f], glgain_wr[i], glgain_mw[i], k, glgain_wq[i] );
        old = old*2 + k;
      }
      z |= (k<<i);
      j += j + k;
    }
    c = z;

    c -= bas; c = byte(c+(ctx0+0));
  }

//if( c!=c0 ) printf( "%i %i\n", c, c0 );

  //rec = c;
  ctx1 = ctx0;
  ctx0 = c;
}


// a long-block scalefactor of MPEG-1: band i of granule gr, channel ch,
// the band below's value k, ll bits.  Its bits are coded by the
// scalefactors' mixing stage (freq_cm.inc: cm_code), which mixes mp3c's two
// predictions (the counter's and the SSE's) with four counters of rows
// made here, from:
//   yprev  the channel's last granule's value at the band
//   yoth   the other channel's: this granule's, for the second channel
//   ydg    how much the channel's global_gain grew since its last granule
//   ypred  the value that would keep the band's quantizer step of the
//          last granule (model_delta.inc: CalcSteps), 0..16
// by the weight set of the band (yw) and the node.
void Process_sf1xlong( word& _c, int ch, int k, int ll, int i, int gr ) {
  RangeCoder& rc = *this;

  int xlong_c,Sxlong_d,pxlong_e,pxlong_f, c = _c;

  pxlong_e = 0;
  pxlong_e = pxlong_e*2 + (((ch)>0+(1-1)));
  pxlong_e = pxlong_e*21 + __min(20,__max(0,(i)-(1-1)));
  pxlong_e = pxlong_e*3 + (((k)>10+(1-1))+((k)>11+(1-1)));
  pxlong_e = pxlong_e*2 + (((ll)>0+(1-1)));
  byte& rec = pxlong[pxlong_e];

  pxlong_f = 0;
  pxlong_f = pxlong_f*18 + C_pxlong_f_i1[__min(20,__max(0,(i)-(1-1)))];
  pxlong_f = pxlong_f*5 + C_pxlong_f_i2[__min(15,__max(0,(k)-(1-1)))];
  pxlong_f = pxlong_f*4 + C_pxlong_f_i4[__min(15,__max(0,(ll)-(1-1)))];
  pxlong_f = pxlong_f*(C_Jxlong_g_Volume) + (0);

  xlong_c = 0;
  xlong_c = xlong_c*20 + C_xlong_c_i1[__min(20,__max(0,(i)-(1-1)))];
  xlong_c = xlong_c*7 + C_xlong_c_i2[__min(15,__max(0,(k)-(1-1)))];
  xlong_c = xlong_c*3 + (((rec)>0+(1-1))+((rec)>7+(1-1)));
  xlong_c = (xlong_c<<2) + ((((ll)>>1)&3)); // 0110
  xlong_c = xlong_c*(C_Jxlong_f_Volume) + (0);

  Sxlong_d = 0;
  Sxlong_d = Sxlong_d*9 + C_Sxlong_d_i1[__min(20,__max(0,(i)-(1-1)))];
  Sxlong_d = Sxlong_d*3 + (((k)>3+(1-1))+((k)>5+(1-1)));
  Sxlong_d = Sxlong_d*2 + (((rec)>2+(1-1)));
  Sxlong_d = Sxlong_d*2 + (((ll)>2+(1-1)));
  Sxlong_d = Sxlong_d*(C_Jxlong_e_Volume) + (0);

  int yra, yrb, yrc, yrd, yrw;
  {
    uint yprev = ysf[ch][i], yoth = ch ? SF[gr][0].l[i] : ysf[1][i];
    const gr_info& g = si.gi[gr][ch];
    int ydg = g.global_gain - ygg[ch];
    // (the last step, less 210, is ygg - 2*(1+yscs)*(ysf + ypre*pretab);
    // q-64 is global_gain less it, clamped, d the step of one scalefactor)
    int q = ydg + 2*(1+yscs[ch])*(ysf[ch][i] + (ypre[ch] ? cm_pretab[i] : 0));
    q = q<-64 ? 0 : q>128 ? 192 : q+64;
    int d = 2*(1+g.scalefac_scale);
    int ypred = (q + d/2)/d - 64/d - (g.preflag ? cm_pretab[i] : 0);
    ypred = ypred<0 ? 0 : ypred>16 ? 16 : ypred;
    int ya, yb, yc, yd, yw;

    ya = 0;
    ya = ya*21 + __min(20,__max(0,(i)-(1-1)));
    ya = ya*16 + __min(15,__max(0,(yprev)-(1-1)));

    yb = 0;
    yb = yb*21 + __min(20,__max(0,(i)-(1-1)));
    yb = yb*16 + __min(15,__max(0,(yoth)-(1-1)));

    yc = 0;
    yc = yc*17 + __min(16,__max(0,(ypred)-(1-1)));
    yc = yc*16 + __min(15,__max(0,(k)-(1-1)));

    yd = 0;
    yd = yd*17 + __min(16,__max(0,(ypred)-(1-1)));
    yd = yd*16 + __min(15,__max(0,(ydg)-(-7-1)));

    yw = 0;
    yw = yw*21 + __min(20,__max(0,(i)-(1-1)));
    yra = ya*32; yrb = yb*32; yrc = yc*32; yrd = yd*32; yrw = yw*32;
  }

//  xlong[xlong_c].Process( rc, c, -1, 1<<ll, C_xlong_wr, C_xlong_mw, Sxlong, Sxlong_d, C_xlong_wq );
  {
    const word* xlong_wr[] = { C_xlong_wr4,C_xlong_wr3,C_xlong_wr2,C_xlong_wr1,C_xlong_wr0 };
    const word* xlong_wq[] = { C_xlong_wq4,C_xlong_wq3,C_xlong_wq2,C_xlong_wq1,C_xlong_wq0 };
    const uint  xlong_mw[] = { C_xlong_mw4,C_xlong_mw3,C_xlong_mw2,C_xlong_mw1,C_xlong_mw0 };

    int lo = 0, hi = 1<<ll; enum{ len=4 };
    int i,j,k,z, Jxlong_e,Jxlong_f,Jxlong_g, bas=C_xlong_bas; 

    c += bas; lo += bas; hi += bas;
    for( i=len,j=1,z=0; i>=0; i-- ) {
      if( z+(1<<i)-1< lo ) k=1; 
  else if( z+(1<<i)  >=hi ) k=0; 
  else {
        k = (c>>i) & 1;

        Jxlong_g = 0;
        Jxlong_g = (Jxlong_g*16) + C_Jxlong_g_i1[j]; // 10000
        byte& old = qxlong[pxlong_f+Jxlong_g];

        Jxlong_e = 0;
        Jxlong_e = (Jxlong_e<<1) + (((old)&1)); // 0001
        Jxlong_e = (Jxlong_e*16) + C_Jxlong_e_i1[j]; // 10000

        Jxlong_f = 0;
        Jxlong_f = (Jxlong_f*9) + C_Jxlong_f_i1[j]; // 11001
        auto& S = Sxlong[Sxlong_d+Jxlong_e];
        Node2ii& M = xlong[xlong_c+Jxlong_f];
        SSEii_updstr X;
        int ps = S.SSE_Pred( M.P, X );
        cm_code( rc, k, M.P, ps, yA[yra+j], yB[yrb+j], yC[yrc+j], yD[yrd+j],
                 &yW[(yrw+j)*XNIN], yWN[yrw+j], C_ylim, C_ymw, C_ysh, C_ylr, C_ymb );
        S.SSE_Update( k, xlong_wq[i], X );
        M.Update( k, xlong_wr[i], xlong_mw[i] );
        old = old*2 + k;
      }
      z |= (k<<i);
      j += j + k;
    }
    c = z;
    c -= bas;
  }

//if( c!=_c ) printf( "%i %i\n", c, _c );

  rec = c;

  _c = c;
}

// MPEG-2's scalefactor at position p (of SF[0][ch].l) of channel ch, num
// bits, block kind bk (0 long, 1 short, 2 mixed): the binary tree of
// Number1ii<7> Y (ProcessScale2's ylong) with its rates wr and mw, each
// bit coded by the stage (freq_cm.inc: cm_code) with Y's counter as mp3c's
// prediction and four counters of rows made here, from:
//   zprev  the channel's last frame's value at p, in a block of this kind
//   zoth   the other channel's: this frame's, for the second channel
//   zbelow this frame's value at p-1
//   zdg    how much the channel's global_gain grew since its last frame
//   zpred  the value that would keep the last frame's quantizer step at p
// by the weight set of p and bk (zw), and the node (r, 1 at the first bit
// coded).  (The parameters are the MPEG-1 stage's, ylim .. ymb.)
void Process_sf2( word& v, int ch, int p, int num, int bk, Node2ii* Y, const word* wr, int mw ) {
  RangeCoder& rc = *this;
  int zra, zrb, zrc, zrd, zrw;
  {
    const gr_info& g = si.gi[0][ch];
    uint zprev = zsf[ch][bk][p], zoth = ch ? SF[0][0].l[p] : zsf[1][bk][p], zbelow = p ? SF[0][ch].l[p-1] : 0;
    int zdg = g.global_gain - zgg[ch];
    int q = zdg + 2*(1+zscs[ch])*int(zprev);
    q = q<-64 ? 0 : q>128 ? 192 : q+64;
    int d = 2*(1+g.scalefac_scale);
    int zpred = (q + d/2)/d - 64/d;
    zpred = zpred<0 ? 0 : zpred>16 ? 16 : zpred;
    int za, zb, zc, zd, zw;

    za = 0;
    za = za*39 + __min(38,__max(0,(p)-(1-1)));
    za = za*16 + __min(15,__max(0,(zprev)-(1-1)));

    zb = 0;
    zb = zb*39 + __min(38,__max(0,(p)-(1-1)));
    zb = zb*16 + __min(15,__max(0,(zoth)-(1-1)));

    zc = 0;
    zc = zc*17 + __min(16,__max(0,(zpred)-(1-1)));
    zc = zc*16 + __min(15,__max(0,(zbelow)-(1-1)));

    zd = 0;
    zd = zd*17 + __min(16,__max(0,(zpred)-(1-1)));
    zd = zd*16 + __min(15,__max(0,(zdg)-(-7-1)));

    zw = 0;
    zw = zw*39 + __min(38,__max(0,(p)-(1-1)));
    zw = zw*3 + (((bk)>0+(1-1))+((bk)>1+(1-1)));
    zra = za*64; zrb = zb*64; zrc = zc*64; zrd = zd*64; zrw = zw*64;
  }
  int z = 0, n = v, top = 1<<num;
  for( int i=6; i>=0; i-- ) {
    int k = (n>>i)&1;
    if( z+(1<<i) >= top ) k = 0; else {
      Node2ii& P = Y[(128+z)>>(i+1)];
      int r = (top+z)>>(i+1); r = r>63 ? 63 : r;
      cm_code( rc, k, P.P, P.P, zA[zra+r], zB[zrb+r], zC[zrc+r], zD[zrd+r],
               &zW[(zrw+r)*XNIN], zWN[zrw+r], C_ylim, C_ymw, C_ysh, C_ylr, C_ymb );
      P.Update( k, wr, mw );
    }
    z |= (k<<i);
  }
  v = word(z);
}

// MPEG-1's short-block scalefactor of band i, window b, granule gr,
// channel ch, l bits: the binary tree of Number1ii<4> Y (ProcessScale1's
// small) with its rates wr and mw, each bit coded by the stage with Y's
// counter as mp3c's prediction and four counters of rows made here, from:
//   sprev  the channel's last short granule's value at the band and window
//   swin   the value of the window before, at the band (16 for the first)
//   sbelow this window's value at the band below
//   soth   the other channel's: this granule's, for the second channel
// by the weight set of the band and window (sw), and the node (r, 1 at
// the first bit coded).  (The parameters are the long blocks', ylim ..)
void Process_sf1short( word& v, int ch, int gr, int i, int b, int l, Node2ii* Y, const word* wr, int mw ) {
  RangeCoder& rc = *this;
  int sra, srb, src, srd, srw;
  {
    const ScaleFac& sf = SF[gr][ch];
    uint sprev = ssf[ch][i][b], swin = b ? sf.s[i][b-1] : 16, sbelow = i ? sf.s[i-1][b] : 0;
    uint soth = ch ? SF[gr][0].s[i][b] : ssf[1][i][b];
    int sa, sb, sc, sd, sw;

    sa = 0;
    sa = sa*12 + __min(11,__max(0,(i)-(1-1)));
    sa = sa*3 + (((b)>0+(1-1))+((b)>1+(1-1)));
    sa = sa*16 + __min(15,__max(0,(sprev)-(1-1)));

    sb = 0;
    sb = sb*12 + __min(11,__max(0,(i)-(1-1)));
    sb = sb*17 + __min(16,__max(0,(swin)-(1-1)));

    sc = 0;
    sc = sc*16 + __min(15,__max(0,(sbelow)-(1-1)));
    sc = sc*17 + __min(16,__max(0,(swin)-(1-1)));

    sd = 0;
    sd = sd*12 + __min(11,__max(0,(i)-(1-1)));
    sd = sd*16 + __min(15,__max(0,(soth)-(1-1)));

    sw = 0;
    sw = sw*12 + __min(11,__max(0,(i)-(1-1)));
    sw = sw*3 + (((b)>0+(1-1))+((b)>1+(1-1)));
    sra = sa*16; srb = sb*16; src = sc*16; srd = sd*16; srw = sw*16;
  }
  int z = 0, n = v, top = 1<<l;
  for( int k3=3; k3>=0; k3-- ) {
    int k = (n>>k3)&1;
    if( z+(1<<k3) >= top ) k = 0; else {
      Node2ii& P = Y[(16+z)>>(k3+1)];
      int r = (top+z)>>(k3+1);
      cm_code( rc, k, P.P, P.P, sA[sra+r], sB[srb+r], sC[src+r], sD[srd+r],
               &sW[(srw+r)*XNIN], sWN[srw+r], C_ylim, C_ymw, C_ysh, C_ylr, C_ymb );
      P.Update( k, wr, mw );
    }
    z |= (k<<k3);
  }
  v = word(z);
}

void ProcessCounts1( int& k, int& c1, int i, int gr, int ch ) {
  RangeCoder& rc = *this;

#ifndef DISABLE_c0c1

  int Band_smc0,Band_smc1, j, mc0_e,Smc0_d,mc1_e,Smc1_d, mc00_e,mc10_e,flag10,flag00,flag20,flag30, mc30_e,mc20_e;

  j = p_c1[i]; 

  Band_smc1 = 0;
  Band_smc1 = Band_smc1*101 + C_Band_smc1_i0[__min(576,__max(0,(j)-(1-1)))];

  flag10 = (c1==0);

  mc10_e = 0;
  count10[mc10_e].Process( rc, flag10, C_count10_wr, C_count10_mw );

  mc1_e = 0;
  mc1_e = mc1_e*2 + (((ch)>0+(1-1)));
  mc1_e = mc1_e*2 + (((i)>6+(1-1)));
  mc1_e = mc1_e*(C_Band_smc1_Volume) + (Band_smc1);

  Smc1_d = 0;
  Smc1_d = Smc1_d*3 + (((i)>0+(1-1))+((i)>4+(1-1)));
  if( flag10 ) c1=0; else {
    c1 += C_count1_bas;
    count1[mc1_e].Process( rc, c1, 0+C_count1_bas, 577+C_count1_bas, C_count1_wr0, C_count1_mw, Scount1[Smc1_d], C_count1_wr1 );
    c1 -= C_count1_bas;
  }

  j = c1;//p_c0[i];

  Band_smc0 = 0;
  Band_smc0 = Band_smc0*6 + C_Band_smc0_i0[__min(576,__max(0,(j)-(1-1)))];

  flag00 = (k==0);

  mc00_e = 0;
  count00[mc00_e].Process( rc, flag00, C_count00_wr, C_count00_mw );

  mc0_e = 0;
  mc0_e = mc0_e*4 + C_mc0_e_i2[__min(8,__max(0,(i)-(1-1)))];
  mc0_e = mc0_e*(C_Band_smc0_Volume) + (Band_smc0);

  Smc0_d = 0;
  Smc0_d = Smc0_d*2 + (((i)>0+(1-1)));
  if( flag00 ) k=0; else {
    k += C_count0_bas;
    count0[mc0_e].Process( rc, k, Max<int>(0,c1-1)+C_count0_bas, 577+C_count0_bas, C_count0_wr0, C_count0_mw, Scount0[Smc0_d], C_count0_wr1 );
    k -= C_count0_bas;
  }

  p_c0[i] = k;
  p_c1[i] = c1;

#endif

//printf( "%3i %3i\n", k, c1 ); fflush(stdout);

}
