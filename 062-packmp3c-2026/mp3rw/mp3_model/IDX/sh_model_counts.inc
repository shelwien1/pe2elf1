
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




Table( Number2c0, count0, %M%mc0_e_Volume );
Table( SSE_c0,   Scount0, %M%Smc0_d_Volume );
Table( Node2ii, count00, %M%mc00_e_Volume );

Table( Number2c1, count1, %M%mc1_e_Volume );
Table( SSE_c1,   Scount1, %M%Smc1_d_Volume );
Table( Node2ii, count10, %M%mc10_e_Volume );

//Table( Number1ii<4>,  xlong, %M%xlong_c_Volume );
Table( Node2ii,   xlong, %M%xlong_c_Volume );
Table( SSEii<5>, Sxlong, %M%Sxlong_d_Volume );
Table( byte,     pxlong, %M%pxlong_e_Volume );
Table( byte,     qxlong, %M%pxlong_f_Volume );

// the scalefactors' mixing stage (Process_sf1xlong): the counters of its
// contexts ya..yd, a row holding the 32 nodes of a scalefactor's binary
// tree; the mixer's weight sets (yw and the node) and their update counts
Table( cm_cnt2, yA, %M%ya_Volume * 32 );
Table( cm_cnt2, yB, %M%yb_Volume * 32 );
Table( cm_cnt2, yC, %M%yc_Volume * 32 );
Table( cm_cnt2, yD, %M%yd_Volume * 32 );
Table( int,     yW, %M%yw_Volume * 32 * XNIN );
Table( byte,   yWN, %M%yw_Volume * 32 );

// MPEG-1's short-block scalefactors' stage (Process_sf1short), the same
// with rows of 16 nodes
Table( cm_cnt2, sA, %M%sa_Volume * 16 );
Table( cm_cnt2, sB, %M%sb_Volume * 16 );
Table( cm_cnt2, sC, %M%sc_Volume * 16 );
Table( cm_cnt2, sD, %M%sd_Volume * 16 );
Table( int,     sW, %M%sw_Volume * 16 * XNIN );
Table( byte,   sWN, %M%sw_Volume * 16 );

// MPEG-2's scalefactors' stage (Process_sf2), the same with rows of 64 nodes
Table( cm_cnt2, zA, %M%za_Volume * 64 );
Table( cm_cnt2, zB, %M%zb_Volume * 64 );
Table( cm_cnt2, zC, %M%zc_Volume * 64 );
Table( cm_cnt2, zD, %M%zd_Volume * 64 );
Table( int,     zW, %M%zw_Volume * 64 * XNIN );
Table( byte,   zWN, %M%zw_Volume * 64 );

// the counts' stage (Process_cnt), the same with rows of the 1024 nodes of
// the count trees, the weight sets by cw and the depth
Table( cm_cnt2, cA, %M%ca_Volume * 1024 );
Table( cm_cnt2, cB, %M%cb_Volume * 1024 );
Table( cm_cnt2, cC, %M%cc_Volume * 1024 );
Table( cm_cnt2, cD, %M%cd_Volume * 1024 );
Table( int,     cW, %M%cw_Volume * 10 * XNIN );
Table( byte,   cWN, %M%cw_Volume * 10 );

// global_gain's stage (Process_GlGain), the same with rows of the 512 nodes
// of its tree, the weight sets by gw and the depth
Table( cm_cnt2, gA, %M%ga_Volume * 512 );
Table( cm_cnt2, gB, %M%gb_Volume * 512 );
Table( cm_cnt2, gC, %M%gc_Volume * 512 );
Table( cm_cnt2, gD, %M%gd_Volume * 512 );
Table( int,     gW, %M%gw_Volume * 9 * XNIN );
Table( byte,   gWN, %M%gw_Volume * 9 );

Table( Node2ii,   glgain, %M%glgain_c_Volume );
Table( SSEii<5>, Sglgain, %M%Sglgain_d_Volume );
Table( byte,     qglgain, %M%pglgain_f_Volume );
Table( byte,     pglgain, %M%pglgain_e_Volume );
Table( byte,     pglgaio, %M%pglgain_e_Volume );

Table( Node2ii,   sfcomp, %M%sfcomp_c_Volume );
Table( SSEii<5>, Ssfcomp, %M%Ssfcomp_d_Volume );
Table( byte,     qsfcomp, %M%psfcomp_f_Volume );
Table( byte,     psfcomp, %M%psfcomp_e_Volume );
Table( byte,     psfcomq, %M%psfcomp_e_Volume );

Table( Node2ii,   rg0cnt, %M%rg0cnt_c_Volume );
Table( SSEii<5>, Srg0cnt, %M%Srg0cnt_d_Volume );
Table( byte,     qrg0cnt, %M%prg0cnt_f_Volume );
Table( byte,     prg0cnt, %M%prg0cnt_e_Volume );
Table( byte,     prg0cnu, %M%prg0cnt_e_Volume );

Table( Node2ii,   rg1cnt, %M%rg1cnt_c_Volume );
Table( SSEii<5>, Srg1cnt, %M%Srg1cnt_d_Volume );
Table( byte,     qrg1cnt, %M%prg1cnt_f_Volume );
Table( byte,     prg1cnt, %M%prg1cnt_e_Volume );
Table( byte,     prg1cnu, %M%prg1cnt_e_Volume );


  // (the types of Table()s count0, Scount0, count1 and Scount1 are model__mp3.inc's,
  // where struct C_T, which holds them, needs them)

//  SSEj<*(int*)& %M%Sxlong_Q,*(int*)& %M%Sxlong_d_Volume> Sxlong;
//  SSEj<*(int*)& %M%Sglgain_Q,*(int*)& %M%Sglgain_d_Volume> Sglgain;
//  SSEj<*(int*)& %M%Ssfcomp_Q,*(int*)& %M%Ssfcomp_d_Volume> Ssfcomp;
//  SSEj<*(int*)& %M%Srg0cnt_Q,*(int*)& %M%Srg0cnt_d_Volume> Srg0cnt;
//  SSEj<*(int*)& %M%Srg1cnt_Q,*(int*)& %M%Srg1cnt_d_Volume> Srg1cnt;

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
  // each channel's last global_gain and the one before (Process_GlGain)
  int  gcur[2], gprv[2];
  // each channel's last c1 and k (ProcessCounts1)
  int  ck[2][2];

//  Number1ii<4>    x_sfcomp[2];
//  Number1ii<4>    x_rg0cnt[2];
//  Number1ii<3>    x_rg1cnt[32];


// the tables' starting state, after C_Init() made them
void %M%InitData( void ) {

  int i;

  for( i=0; i<%M%mc0_e_Volume; i++ ) count0[i].Init_Number2i();
  for( i=0; i<%M%Smc0_d_Volume; i++ ) Init_SSEii( Scount0[i] );
  for( i=0; i<%M%mc00_e_Volume; i++ ) count00[i].Init_Node2i();

  for( i=0; i<%M%mc1_e_Volume; i++ ) count1[i].Init_Number2i();
  for( i=0; i<%M%Smc1_d_Volume; i++ ) Init_SSEii( Scount1[i] );
  for( i=0; i<%M%mc10_e_Volume; i++ ) count10[i].Init_Node2i();

  memset( &p_c0, 0, sizeof(p_c0) );
  memset( &p_c1, 0, sizeof(p_c1) );

  for( i=0; i<%M%xlong_c_Volume; i++ ) xlong[i].Init_Node2i();
  for( i=0; i<%M%Sxlong_d_Volume; i++ ) Sxlong[i].Init_SSEi(%M%Sxlong_wi,%M%Sxlong_ti);
  for( i=0; i<%M%pxlong_e_Volume; i++ ) pxlong[i]=0;
  for( i=0; i<%M%pxlong_f_Volume; i++ ) qxlong[i]=0;

  // the scalefactors' stage: counters at 1/2, each weight set taking mp3c's
  // two predictions half and half
  for( i=0; i<%M%ya_Volume*32; i++ ) yA[i].Init();
  for( i=0; i<%M%yb_Volume*32; i++ ) yB[i].Init();
  for( i=0; i<%M%yc_Volume*32; i++ ) yC[i].Init();
  for( i=0; i<%M%yd_Volume*32; i++ ) yD[i].Init();
  for( i=0; i<%M%yw_Volume*32; i++ ) {
    for( int k=0; k<XNIN; k++ ) yW[i*XNIN+k] = k==0 ? %M%ywa : k==10 ? %M%ywb : 0;
    yWN[i] = 0;
  }
  memset( ysf, 0, sizeof(ysf) );
  ygg[0] = ygg[1] = 0; yscs[0] = yscs[1] = 0; ypre[0] = ypre[1] = 0;

  for( i=0; i<%M%za_Volume*64; i++ ) zA[i].Init();
  for( i=0; i<%M%zb_Volume*64; i++ ) zB[i].Init();
  for( i=0; i<%M%zc_Volume*64; i++ ) zC[i].Init();
  for( i=0; i<%M%zd_Volume*64; i++ ) zD[i].Init();
  for( i=0; i<%M%zw_Volume*64; i++ ) {
    for( int k=0; k<XNIN; k++ ) zW[i*XNIN+k] = k==0 ? %M%ywa : k==10 ? %M%ywb : 0;
    zWN[i] = 0;
  }
  memset( zsf, 0, sizeof(zsf) );
  zgg[0] = zgg[1] = 0; zscs[0] = zscs[1] = 0;

  for( i=0; i<%M%sa_Volume*16; i++ ) sA[i].Init();
  for( i=0; i<%M%sb_Volume*16; i++ ) sB[i].Init();
  for( i=0; i<%M%sc_Volume*16; i++ ) sC[i].Init();
  for( i=0; i<%M%sd_Volume*16; i++ ) sD[i].Init();
  for( i=0; i<%M%sw_Volume*16; i++ ) {
    for( int k=0; k<XNIN; k++ ) sW[i*XNIN+k] = k==0 ? %M%ywa : k==10 ? %M%ywb : 0;
    sWN[i] = 0;
  }
  memset( ssf, 0, sizeof(ssf) );

  for( i=0; i<%M%ga_Volume*512; i++ ) gA[i].Init();
  for( i=0; i<%M%gb_Volume*512; i++ ) gB[i].Init();
  for( i=0; i<%M%gc_Volume*512; i++ ) gC[i].Init();
  for( i=0; i<%M%gd_Volume*512; i++ ) gD[i].Init();
  for( i=0; i<%M%gw_Volume*9; i++ ) {
    for( int k=0; k<XNIN; k++ ) gW[i*XNIN+k] = k==0 ? %M%gwa : k==10 ? %M%gwb : 0;
    gWN[i] = 0;
  }
  gcur[0] = gcur[1] = gprv[0] = gprv[1] = 0;

  for( i=0; i<%M%ca_Volume*1024; i++ ) cA[i].Init();
  for( i=0; i<%M%cb_Volume*1024; i++ ) cB[i].Init();
  for( i=0; i<%M%cc_Volume*1024; i++ ) cC[i].Init();
  for( i=0; i<%M%cd_Volume*1024; i++ ) cD[i].Init();
  for( i=0; i<%M%cw_Volume*10; i++ ) {
    for( int k=0; k<XNIN; k++ ) cW[i*XNIN+k] = k==0 ? %M%cwa : k==10 ? %M%cwb : 0;
    cWN[i] = 0;
  }
  memset( ck, 0, sizeof(ck) );

//  Init_Number1ii( x_glgain );
  for( i=0; i<%M%glgain_c_Volume; i++ ) glgain[i].Init_Node2i();
  for( i=0; i<%M%Sglgain_d_Volume; i++ ) Sglgain[i].Init_SSEi(%M%Sglgain_wi,%M%Sglgain_ti);
  for( i=0; i<%M%pglgain_f_Volume; i++ ) qglgain[i]=0;
  for( i=0; i<%M%pglgain_e_Volume; i++ ) pglgain[i]=0;
  for( i=0; i<%M%pglgain_e_Volume; i++ ) pglgaio[i]=0;

//  Init_Number1ii( x_sfcomp );
  for( i=0; i<%M%sfcomp_c_Volume; i++ ) sfcomp[i].Init_Node2i();
  for( i=0; i<%M%Ssfcomp_d_Volume; i++ ) Ssfcomp[i].Init_SSEi(%M%Ssfcomp_wi,%M%Ssfcomp_ti);
  for( i=0; i<%M%psfcomp_f_Volume; i++ ) qsfcomp[i]=0;
  for( i=0; i<%M%psfcomp_e_Volume; i++ ) psfcomp[i]=0;
  for( i=0; i<%M%psfcomp_e_Volume; i++ ) psfcomq[i]=0;

//  Init_Number1ii( x_rg0cnt );
  for( i=0; i<%M%rg0cnt_c_Volume; i++ ) rg0cnt[i].Init_Node2i();
  for( i=0; i<%M%Srg0cnt_d_Volume; i++ ) Srg0cnt[i].Init_SSEi(%M%Srg0cnt_wi,%M%Srg0cnt_ti);
  for( i=0; i<%M%prg0cnt_f_Volume; i++ ) qrg0cnt[i]=0;
  for( i=0; i<%M%prg0cnt_e_Volume; i++ ) prg0cnt[i]=0;
  for( i=0; i<%M%prg0cnt_e_Volume; i++ ) prg0cnu[i]=0;

//  Init_Number1ii( x_rg1cnt );
  for( i=0; i<%M%rg1cnt_c_Volume; i++ ) rg1cnt[i].Init_Node2i();
  for( i=0; i<%M%Srg1cnt_d_Volume; i++ ) Srg1cnt[i].Init_SSEi(%M%Srg1cnt_wi,%M%Srg1cnt_ti);
  for( i=0; i<%M%prg1cnt_f_Volume; i++ ) qrg1cnt[i]=0;
  for( i=0; i<%M%prg1cnt_e_Volume; i++ ) prg1cnt[i]=0;
  for( i=0; i<%M%prg1cnt_e_Volume; i++ ) prg1cnu[i]=0;

}

void Process_rgxcnt( int& c, int& _c1, int ch ) {
  RangeCoder& rc = *this;

  //x_rg0cnt[ch].Process( rc, c, -1,16, %M%x_rg0cnt_wr, %M%x_rg0cnt_mw );
{
//int c0=c;

  int rg0cnt_c,Srg0cnt_d,prg0cnt_e,prg0cnt_f;
  MakeIndex prg0cnt_e
  byte& ctx1 = prg0cnu[prg0cnt_e];
  byte& ctx0 = prg0cnt[prg0cnt_e];
  char rec = ctx0 - ctx1;

//printf( "%i ", rec );

  MakeIndex prg0cnt_f,rg0cnt_c,Srg0cnt_d

  {
    const word* rg0cnt_wr[] = { %M%rg0cnt_wr4,%M%rg0cnt_wr3,%M%rg0cnt_wr2,%M%rg0cnt_wr1,%M%rg0cnt_wr0 };
    const word* rg0cnt_wq[] = { %M%rg0cnt_wq4,%M%rg0cnt_wq3,%M%rg0cnt_wq2,%M%rg0cnt_wq1,%M%rg0cnt_wq0 };
    const uint  rg0cnt_mw[] = { %M%rg0cnt_mw4,%M%rg0cnt_mw3,%M%rg0cnt_mw2,%M%rg0cnt_mw1,%M%rg0cnt_mw0 };

    int lo = 0, hi = 1<<4; enum{ len=4 };
    int i,j,k,z, Jrg0cnt_e,Jrg0cnt_f,Jrg0cnt_g, bas=%M%rg0cnt_bas; 

//    c = (c-(ctx0-0))&15;  
    c += bas; lo += bas; hi += bas;
    for( i=len,j=1,z=0; i>=0; i-- ) {
      if( z+(1<<i)-1< lo ) k=1; 
  else if( z+(1<<i)  >=hi ) k=0; 
  else {
        k = (c>>i) & 1;
        MakeIndex Jrg0cnt_g
        byte& old = qrg0cnt[prg0cnt_f+Jrg0cnt_g];
        MakeIndex Jrg0cnt_e,Jrg0cnt_f
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

//  x_rg1cnt[ch+2*c].Process( rc, _c1,-1,8, %M%x_rg1cnt_wr, %M%x_rg1cnt_mw );
{ int rg0=c; int c=_c1;
//int c0=c;

  int rg1cnt_c,Srg1cnt_d,prg1cnt_e,prg1cnt_f;
  MakeIndex prg1cnt_e
  byte& ctx1 = prg1cnu[prg1cnt_e];
  byte& ctx0 = prg1cnt[prg1cnt_e];
  char rec = ctx0 - ctx1;

//printf( "%i ", rec );

  MakeIndex prg1cnt_f,rg1cnt_c,Srg1cnt_d

  {
    const word* rg1cnt_wr[] = { %M%rg1cnt_wr3,%M%rg1cnt_wr2,%M%rg1cnt_wr1,%M%rg1cnt_wr0 };
    const word* rg1cnt_wq[] = { %M%rg1cnt_wq3,%M%rg1cnt_wq2,%M%rg1cnt_wq1,%M%rg1cnt_wq0 };
    const uint  rg1cnt_mw[] = { %M%rg1cnt_mw3,%M%rg1cnt_mw2,%M%rg1cnt_mw1,%M%rg1cnt_mw0 };

    int lo = 0, hi = 1<<3; enum{ len=3 };
    int i,j,k,z, Jrg1cnt_e,Jrg1cnt_f,Jrg1cnt_g, bas=%M%rg1cnt_bas; 

//    c = (c-(ctx0-0))&7;  
    c += bas; lo += bas; hi += bas;
    for( i=len,j=1,z=0; i>=0; i-- ) {
      if( z+(1<<i)-1< lo ) k=1; 
  else if( z+(1<<i)  >=hi ) k=0; 
  else {
        k = (c>>i) & 1;
        MakeIndex Jrg1cnt_g
        byte& old = qrg1cnt[prg1cnt_f+Jrg1cnt_g];
        MakeIndex Jrg1cnt_e,Jrg1cnt_f
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
//  x_sfcomp[ch].Process( rc, c, -1, 16, %M%x_sfcomp_wr, %M%x_sfcomp_mw ); // 4 bits

//int c0=c;

  int sfcomp_c,Ssfcomp_d,psfcomp_e,psfcomp_f;
  MakeIndex psfcomp_e
  byte& ctx1 = psfcomq[psfcomp_e];
  byte& ctx0 = psfcomp[psfcomp_e];
  char rec = ctx0 - ctx1;

//printf( "%i ", rec );

  MakeIndex psfcomp_f,sfcomp_c,Ssfcomp_d

  {
    const word* sfcomp_wr[] = { %M%sfcomp_wr4,%M%sfcomp_wr3,%M%sfcomp_wr2,%M%sfcomp_wr1,%M%sfcomp_wr0 };
    const word* sfcomp_wq[] = { %M%sfcomp_wq4,%M%sfcomp_wq3,%M%sfcomp_wq2,%M%sfcomp_wq1,%M%sfcomp_wq0 };
    const uint  sfcomp_mw[] = { %M%sfcomp_mw4,%M%sfcomp_mw3,%M%sfcomp_mw2,%M%sfcomp_mw1,%M%sfcomp_mw0 };

    int lo = 0, hi = 1<<4; enum{ len=4 };
    int i,j,k,z, Jsfcomp_e,Jsfcomp_f,Jsfcomp_g, bas=%M%sfcomp_bas; 

//    c = (c-(ctx0-0))&15;  
    c += bas; lo += bas; hi += bas;
    for( i=len,j=1,z=0; i>=0; i-- ) {
      if( z+(1<<i)-1< lo ) k=1; 
  else if( z+(1<<i)  >=hi ) k=0; 
  else {
        k = (c>>i) & 1;
        MakeIndex Jsfcomp_g
        byte& old = qsfcomp[psfcomp_f+Jsfcomp_g];
        MakeIndex Jsfcomp_e,Jsfcomp_f
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


// global_gain of granule gr, channel ch: its change from the channel's
// last value, a binary tree of 9 levels, each bit coded by a mixing stage
// (freq_cm.inc: cm_code) with mp3c's two predictions and four counters of
// rows made here, from:
//   gld   the channel's last change
//   god   the other channel's latest change (this granule's, for the
//         second channel)
//   glast the channel's last value
//   gx    the other channel's latest value less this channel's last
// by the weight set of gw and the depth; the rows hold the tree's nodes.
void Process_GlGain( int& c, int ch, int gr ) {
  RangeCoder& rc = *this;

  int gra, grb, grc, grd, grw;
  {
    int o = ch^1;
    int gld = gcur[ch] - gprv[ch], god = gcur[o] - gprv[o];
    uint glast = gcur[ch];
    int gx = gcur[o] - gcur[ch];
    int ga, gb, gc, gd, gw;
    MakeIndex ga,gb,gc,gd,gw
    gra = ga*512; grb = gb*512; grc = gc*512; grd = gd*512; grw = gw*9;
  }

//int c0=c;

//  x_glgain[0].Process( rc, c, -1, 256, C_x_glgain_0_wr, C_x_glgain_0_mw );
  int glgain_c,Sglgain_d,pglgain_e,pglgain_f;
  MakeIndex pglgain_e
  byte& ctx1 = pglgaio[pglgain_e];
  byte& ctx0 = pglgain[pglgain_e];
  char rec = ctx0 - ctx1;

//printf( "%i ", rec );

  MakeIndex pglgain_f,glgain_c,Sglgain_d

  {
    const word* glgain_wr[] = { %M%glgain_wr8,%M%glgain_wr7,%M%glgain_wr6,%M%glgain_wr5,%M%glgain_wr4,%M%glgain_wr3,%M%glgain_wr2,%M%glgain_wr1,%M%glgain_wr0 };
    const word* glgain_wq[] = { %M%glgain_wq8,%M%glgain_wq7,%M%glgain_wq6,%M%glgain_wq5,%M%glgain_wq4,%M%glgain_wq3,%M%glgain_wq2,%M%glgain_wq1,%M%glgain_wq0 };
    const uint  glgain_mw[] = { %M%glgain_mw8,%M%glgain_mw7,%M%glgain_mw6,%M%glgain_mw5,%M%glgain_mw4,%M%glgain_mw3,%M%glgain_mw2,%M%glgain_mw1,%M%glgain_mw0 };

    int lo = 0, hi = 1<<8; enum{ len=8 };
    int i,j,k,z, Jglgain_e,Jglgain_f,Jglgain_g, bas=%M%glgain_bas; 

    c = byte(c-(ctx0-0)); c += bas; lo += bas; hi += bas;
    for( i=len,j=1,z=0; i>=0; i-- ) {
      if( z+(1<<i)-1< lo ) k=1; 
  else if( z+(1<<i)  >=hi ) k=0; 
  else {
        k = (c>>i) & 1;
        MakeIndex Jglgain_g
        byte& old = qglgain[pglgain_f+Jglgain_g];
        MakeIndex Jglgain_e,Jglgain_f
        auto& S = Sglgain[Sglgain_d+Jglgain_e];
        Node2ii& M = glgain[glgain_c+Jglgain_f];
        SSEii_updstr X;
        int ps = S.SSE_Pred( M.P, X );
        cm_code( rc, k, M.P, ps, gA[gra+j], gB[grb+j], gC[grc+j], gD[grd+j],
                 &gW[(grw+8-i)*XNIN], gWN[grw+8-i], %M%glim, %M%gmw, %M%gsh, %M%glr, %M%gmb );
        S.SSE_Update( k, glgain_wq[i], X );
        M.Update( k, glgain_wr[i], glgain_mw[i] );
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
  gprv[ch] = gcur[ch]; gcur[ch] = c;
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
  MakeIndex pxlong_e
  byte& rec = pxlong[pxlong_e];
  MakeIndex pxlong_f,xlong_c,Sxlong_d

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
    MakeIndex ya,yb,yc,yd,yw
    yra = ya*32; yrb = yb*32; yrc = yc*32; yrd = yd*32; yrw = yw*32;
  }

//  xlong[xlong_c].Process( rc, c, -1, 1<<ll, %M%xlong_wr, %M%xlong_mw, Sxlong, Sxlong_d, %M%xlong_wq );
  {
    const word* xlong_wr[] = { %M%xlong_wr4,%M%xlong_wr3,%M%xlong_wr2,%M%xlong_wr1,%M%xlong_wr0 };
    const word* xlong_wq[] = { %M%xlong_wq4,%M%xlong_wq3,%M%xlong_wq2,%M%xlong_wq1,%M%xlong_wq0 };
    const uint  xlong_mw[] = { %M%xlong_mw4,%M%xlong_mw3,%M%xlong_mw2,%M%xlong_mw1,%M%xlong_mw0 };

    int lo = 0, hi = 1<<ll; enum{ len=4 };
    int i,j,k,z, Jxlong_e,Jxlong_f,Jxlong_g, bas=%M%xlong_bas; 

    c += bas; lo += bas; hi += bas;
    for( i=len,j=1,z=0; i>=0; i-- ) {
      if( z+(1<<i)-1< lo ) k=1; 
  else if( z+(1<<i)  >=hi ) k=0; 
  else {
        k = (c>>i) & 1;
        MakeIndex Jxlong_g
        byte& old = qxlong[pxlong_f+Jxlong_g];
        MakeIndex Jxlong_e,Jxlong_f
        auto& S = Sxlong[Sxlong_d+Jxlong_e];
        Node2ii& M = xlong[xlong_c+Jxlong_f];
        SSEii_updstr X;
        int ps = S.SSE_Pred( M.P, X );
        cm_code( rc, k, M.P, ps, yA[yra+j], yB[yrb+j], yC[yrc+j], yD[yrd+j],
                 &yW[(yrw+j)*XNIN], yWN[yrw+j], %M%ylim, %M%ymw, %M%ysh, %M%ylr, %M%ymb );
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
    MakeIndex za,zb,zc,zd,zw
    zra = za*64; zrb = zb*64; zrc = zc*64; zrd = zd*64; zrw = zw*64;
  }
  int z = 0, n = v, top = 1<<num;
  for( int i=6; i>=0; i-- ) {
    int k = (n>>i)&1;
    if( z+(1<<i) >= top ) k = 0; else {
      Node2ii& P = Y[(128+z)>>(i+1)];
      int r = (top+z)>>(i+1); r = r>63 ? 63 : r;
      cm_code( rc, k, P.P, P.P, zA[zra+r], zB[zrb+r], zC[zrc+r], zD[zrd+r],
               &zW[(zrw+r)*XNIN], zWN[zrw+r], %M%ylim, %M%ymw, %M%ysh, %M%ylr, %M%ymb );
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
    MakeIndex sa,sb,sc,sd,sw
    sra = sa*16; srb = sb*16; src = sc*16; srd = sd*16; srw = sw*16;
  }
  int z = 0, n = v, top = 1<<l;
  for( int k3=3; k3>=0; k3-- ) {
    int k = (n>>k3)&1;
    if( z+(1<<k3) >= top ) k = 0; else {
      Node2ii& P = Y[(16+z)>>(k3+1)];
      int r = (top+z)>>(k3+1);
      cm_code( rc, k, P.P, P.P, sA[sra+r], sB[srb+r], sC[src+r], sD[srd+r],
               &sW[(srw+r)*XNIN], sWN[srw+r], %M%ylim, %M%ymw, %M%ysh, %M%ylr, %M%ymb );
      P.Update( k, wr, mw );
    }
    z |= (k<<k3);
  }
  v = word(z);
}

// a count of granule gr, channel ch: n in [lo,hi) (Number2ii's sense of
// them) MSB first over the tree T of 10 levels, with T's counters and the
// SSEs S of its nodes (and their rates wr, mw, Swr) as mp3c's two
// predictions, each bit coded by the stage (freq_cm.inc: cm_code) with
// four counters of rows made here, from:
//   cwh    which count: 0 c1 (the count1 region's start), 1 k (the end of
//          the nonzero values)
//   clast  the channel's last value of this count
//   coth   the other channel's latest (this granule's, for the second)
//   cdg    how much the channel's global_gain grew (Process_GlGain's)
//   cbt    block_type, and cc1 the c1 coded just before k (0 for c1)
// by the weight set of cw and the depth; the rows hold the tree's nodes.
template< class NT, class SS >
void Process_cnt( int& n, int lo, int hi, NT& T, SS* S, const word* wr, int mw, const word* Swr, int cwh, int ch, int gr, int c1 ) {
  RangeCoder& rc = *this;
  int cra, crb, crc, crd, crw;
  {
    uint clast = ck[ch][cwh], coth = ck[ch^1][cwh];
    int cdg = gcur[ch] - gprv[ch];
    uint cbt = si.gi[gr][ch].block_type, cc1 = cwh ? c1 : 0;
    int ca, cb, cc, cd, cw;
    MakeIndex ca,cb,cc,cd,cw
    cra = ca*1024; crb = cb*1024; crc = cc*1024; crd = cd*1024; crw = cw*10;
  }
  int z = 0;
  for( int i=9; i>=0; i-- ) {
    int j = (1024+z)>>(i+1);
    int k = (n>>i)&1;
    if( z+(1<<i)-1<=lo ) k=1; else if( z+(1<<i) >= hi ) k=0; else {
      Node2ii& P = T.P[j];
      SSEii_updstr X;
      int ps = S[j].SSE_Pred( P.P, X );
      cm_code( rc, k, P.P, ps, cA[cra+j], cB[crb+j], cC[crc+j], cD[crd+j],
               &cW[(crw+9-i)*XNIN], cWN[crw+9-i], %M%clim, %M%cmw, %M%csh, %M%clr, %M%cmb );
      S[j].SSE_Update( k, Swr, X );
      P.Update( k, wr, mw );
    }
    z |= (k<<i);
  }
  n = z;
}

void ProcessCounts1( int& k, int& c1, int i, int gr, int ch ) {
  RangeCoder& rc = *this;

#ifndef DISABLE_c0c1

  int Band_smc0,Band_smc1, j, mc0_e,Smc0_d,mc1_e,Smc1_d, mc00_e,mc10_e,flag10,flag00,flag20,flag30, mc30_e,mc20_e;

  j = p_c1[i]; 
  MakeIndex Band_smc1

  flag10 = (c1==0);
  MakeIndex mc10_e
  count10[mc10_e].Process( rc, flag10, %M%count10_wr, %M%count10_mw );
  MakeIndex mc1_e,Smc1_d
  if( flag10 ) c1=0; else {
    c1 += %M%count1_bas;
    Process_cnt( c1, 0+%M%count1_bas, 577+%M%count1_bas, count1[mc1_e], Scount1[Smc1_d], %M%count1_wr0, %M%count1_mw, %M%count1_wr1, 0, ch, gr, 0 );
    c1 -= %M%count1_bas;
  }

  j = c1;//p_c0[i];
  MakeIndex Band_smc0

  flag00 = (k==0);
  MakeIndex mc00_e
  count00[mc00_e].Process( rc, flag00, %M%count00_wr, %M%count00_mw );
  MakeIndex mc0_e,Smc0_d
  if( flag00 ) k=0; else {
    k += %M%count0_bas;
    Process_cnt( k, Max<int>(0,c1-1)+%M%count0_bas, 577+%M%count0_bas, count0[mc0_e], Scount0[Smc0_d], %M%count0_wr0, %M%count0_mw, %M%count0_wr1, 1, ch, gr, c1 );
    k -= %M%count0_bas;
  }

  p_c0[i] = k;
  p_c1[i] = c1;
  ck[ch][0] = c1; ck[ch][1] = k;

#endif

//printf( "%3i %3i\n", k, c1 ); fflush(stdout);

}
