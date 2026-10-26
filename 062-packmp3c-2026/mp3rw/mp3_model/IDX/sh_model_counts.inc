
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

def_Data

  typedef Number2ii<10,1023> Number2c1;
  typedef SSEii<5,5,1> SSE_c1[(1024+1023)/2+1];
  typedef Number2ii<10,1023> Number2c0;
  typedef SSEii<5,5,2> SSE_c0[(1024+1023)/2+1];

//  SSEj<*(int*)& %M%Sxlong_Q,*(int*)& %M%Sxlong_d_Volume> Sxlong;
//  SSEj<*(int*)& %M%Sglgain_Q,*(int*)& %M%Sglgain_d_Volume> Sglgain;
//  SSEj<*(int*)& %M%Ssfcomp_Q,*(int*)& %M%Ssfcomp_d_Volume> Ssfcomp;
//  SSEj<*(int*)& %M%Srg0cnt_Q,*(int*)& %M%Srg0cnt_d_Volume> Srg0cnt;
//  SSEj<*(int*)& %M%Srg1cnt_Q,*(int*)& %M%Srg1cnt_d_Volume> Srg1cnt;

  int p_c1[8];
  int p_c0[8];

//  Number1ii<4>    x_sfcomp[2];
//  Number1ii<4>    x_rg0cnt[2];
//  Number1ii<3>    x_rg1cnt[32];

end_Data

def_Init

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

end_Init

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


void Process_GlGain( int& c, int ch, int gr ) {
  RangeCoder& rc = *this;

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


void Process_sf1xlong( word& _c, int ch, int k, int ll, int i ) {
  RangeCoder& rc = *this;

  int xlong_c,Sxlong_d,pxlong_e,pxlong_f, c = _c;
  MakeIndex pxlong_e
  byte& rec = pxlong[pxlong_e];
  MakeIndex pxlong_f,xlong_c,Sxlong_d

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
        Sxlong[Sxlong_d+Jxlong_e].Process( rc, xlong[xlong_c+Jxlong_f], xlong_wr[i], xlong_mw[i], k, xlong_wq[i] );
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
    count1[mc1_e].Process( rc, c1, 0+%M%count1_bas, 577+%M%count1_bas, %M%count1_wr0, %M%count1_mw, Scount1[Smc1_d], %M%count1_wr1 );
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
    count0[mc0_e].Process( rc, k, Max<int>(0,c1-1)+%M%count0_bas, 577+%M%count0_bas, %M%count0_wr0, %M%count0_mw, Scount0[Smc0_d], %M%count0_wr1 );
    k -= %M%count0_bas;
  }

  p_c0[i] = k;
  p_c1[i] = c1;

#endif

//printf( "%3i %3i\n", k, c1 ); fflush(stdout);

}
