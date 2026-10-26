
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
  prg0cnt_f = prg0cnt_f*2 + ((ch>0+(1-1)));
  prg0cnt_f = prg0cnt_f*8 + ((rec>3+(-12-1))+(rec>8+(-12-1))+(rec>10+(-12-1))+(rec>13+(-12-1))+(rec>15+(-12-1))+(rec>17+(-12-1))+(rec>21+(-12-1)));
  prg0cnt_f = (prg0cnt_f<<1) + (((ctx0)>>3)&1); // 1000
  prg0cnt_f = prg0cnt_f*(C_Jrg0cnt_g_Volume) + (0);

  rg0cnt_c = 0;
  rg0cnt_c = rg0cnt_c*(C_Jrg0cnt_f_Volume) + (0);

  Srg0cnt_d = 0;
  Srg0cnt_d = Srg0cnt_d*2 + ((ch>0+(1-1)));
  Srg0cnt_d = Srg0cnt_d*2 + ((rec>25+(-12-1)));
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
  prg1cnt_f = prg1cnt_f*3 + ((rec>8+(-12-1))+(rec>19+(-12-1)));
  prg1cnt_f = (prg1cnt_f<<4) + ((rg0)&15); // 1111
  prg1cnt_f = prg1cnt_f*(C_Jrg1cnt_g_Volume) + (0);

  rg1cnt_c = 0;
  rg1cnt_c = (rg1cnt_c<<4) + ((rg0)&15); // 1111
  rg1cnt_c = rg1cnt_c*(C_Jrg1cnt_f_Volume) + (0);

  Srg1cnt_d = 0;
  Srg1cnt_d = Srg1cnt_d*2 + ((rec>25+(-12-1)));
  Srg1cnt_d = Srg1cnt_d*7 + ((rg0>0+(1-1))+(rg0>5+(1-1))+(rg0>7+(1-1))+(rg0>9+(1-1))+(rg0>11+(1-1))+(rg0>13+(1-1)));
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
        Jrg1cnt_e = (Jrg1cnt_e<<1) + ((old)&1); // 0001
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
  psfcomp_f = psfcomp_f*6 + ((rec>1+(-12-1))+(rec>2+(-12-1))+(rec>10+(-12-1))+(rec>11+(-12-1))+(rec>12+(-12-1)));
  psfcomp_f = (psfcomp_f<<4) + ((ctx0)&15); // 1111
  psfcomp_f = psfcomp_f*(C_Jsfcomp_g_Volume) + (0);

  sfcomp_c = 0;
  sfcomp_c = sfcomp_c*8 + ((rec>13+(-12-1))+(rec>14+(-12-1))+(rec>20+(-12-1))+(rec>21+(-12-1))+(rec>22+(-12-1))+(rec>23+(-12-1))+(rec>24+(-12-1)));
  sfcomp_c = (sfcomp_c<<3) + (((ctx0)>>1)&7); // 1110
  sfcomp_c = sfcomp_c*(C_Jsfcomp_f_Volume) + (0);

  Ssfcomp_d = 0;
  Ssfcomp_d = Ssfcomp_d*5 + ((rec>3+(-12-1))+(rec>10+(-12-1))+(rec>11+(-12-1))+(rec>12+(-12-1)));
  Ssfcomp_d = (Ssfcomp_d<<3) + (((ctx0)>>1)&~3) + ((ctx0)&3); // 1011
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
  pglgain_e = pglgain_e*2 + ((ch>0+(1-1)));
  byte& ctx1 = pglgaio[pglgain_e];
  byte& ctx0 = pglgain[pglgain_e];
  char rec = ctx0 - ctx1;

//printf( "%i ", rec );


  pglgain_f = 0;
  pglgain_f = pglgain_f*2 + ((gr>0+(1-1)));
  pglgain_f = pglgain_f*2 + ((rec>5+(-12-1)));
  pglgain_f = pglgain_f*(C_Jglgain_g_Volume) + (0);

  glgain_c = 0;
  glgain_c = (glgain_c<<7) + ((ctx0)&127); // 01111111
  glgain_c = glgain_c*(C_Jglgain_f_Volume) + (0);

  Sglgain_d = 0;
  Sglgain_d = Sglgain_d*9 + C_Sglgain_d_i2[__min(32,__max(0,rec-(-12-1)))];
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
        Jglgain_e = (Jglgain_e<<1) + ((old)&1); // 00000001
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


void Process_sf1xlong( word& _c, int ch, int k, int ll, int i ) {
  RangeCoder& rc = *this;

  int xlong_c,Sxlong_d,pxlong_e,pxlong_f, c = _c;

  pxlong_e = 0;
  pxlong_e = pxlong_e*2 + ((ch>0+(1-1)));
  pxlong_e = pxlong_e*21 + C_pxlong_e_i1[__min(20,__max(0,i-(1-1)))];
  pxlong_e = pxlong_e*3 + ((k>10+(1-1))+(k>11+(1-1)));
  pxlong_e = pxlong_e*2 + ((ll>0+(1-1)));
  byte& rec = pxlong[pxlong_e];

  pxlong_f = 0;
  pxlong_f = pxlong_f*18 + C_pxlong_f_i1[__min(20,__max(0,i-(1-1)))];
  pxlong_f = pxlong_f*5 + ((k>3+(1-1))+(k>5+(1-1))+(k>8+(1-1))+(k>12+(1-1)));
  pxlong_f = pxlong_f*4 + ((ll>1+(1-1))+(ll>2+(1-1))+(ll>3+(1-1)));
  pxlong_f = pxlong_f*(C_Jxlong_g_Volume) + (0);

  xlong_c = 0;
  xlong_c = xlong_c*20 + C_xlong_c_i1[__min(20,__max(0,i-(1-1)))];
  xlong_c = xlong_c*7 + ((k>0+(1-1))+(k>1+(1-1))+(k>2+(1-1))+(k>4+(1-1))+(k>6+(1-1))+(k>7+(1-1)));
  xlong_c = xlong_c*3 + ((rec>0+(1-1))+(rec>7+(1-1)));
  xlong_c = (xlong_c<<2) + (((ll)>>1)&3); // 0110
  xlong_c = xlong_c*(C_Jxlong_f_Volume) + (0);

  Sxlong_d = 0;
  Sxlong_d = Sxlong_d*9 + C_Sxlong_d_i1[__min(20,__max(0,i-(1-1)))];
  Sxlong_d = Sxlong_d*3 + ((k>3+(1-1))+(k>5+(1-1)));
  Sxlong_d = Sxlong_d*2 + ((rec>2+(1-1)));
  Sxlong_d = Sxlong_d*2 + ((ll>2+(1-1)));
  Sxlong_d = Sxlong_d*(C_Jxlong_e_Volume) + (0);

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
        Jxlong_e = (Jxlong_e<<1) + ((old)&1); // 0001
        Jxlong_e = (Jxlong_e*16) + C_Jxlong_e_i1[j]; // 10000

        Jxlong_f = 0;
        Jxlong_f = (Jxlong_f*9) + C_Jxlong_f_i1[j]; // 11001
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

  Band_smc1 = 0;
  Band_smc1 = Band_smc1*1 + ((j>12+(25-1)));
  Band_smc1 = Band_smc1*1 + ((j>16+(73-1)));
  Band_smc1 = Band_smc1*1 + ((j>1+(97-1))+(j>17+(97-1))+(j>18+(97-1))+(j>21+(97-1)));
  Band_smc1 = Band_smc1*1 + ((j>2+(121-1))+(j>4+(121-1))+(j>11+(121-1)));
  Band_smc1 = Band_smc1*1 + ((j>1+(145-1))+(j>6+(145-1))+(j>9+(145-1))+(j>13+(145-1)));
  Band_smc1 = Band_smc1*1 + ((j>9+(169-1))+(j>10+(169-1)));
  Band_smc1 = Band_smc1*1 + ((j>8+(193-1)));
  Band_smc1 = Band_smc1*1 + ((j>23+(265-1)));
  Band_smc1 = Band_smc1*1 + C_Band_smc1_ic[__min(24,__max(0,j-(289-1)))];
  Band_smc1 = Band_smc1*1 + C_Band_smc1_id[__min(24,__max(0,j-(313-1)))];
  Band_smc1 = Band_smc1*1 + C_Band_smc1_ie[__min(24,__max(0,j-(337-1)))];
  Band_smc1 = Band_smc1*1 + C_Band_smc1_if[__min(24,__max(0,j-(361-1)))];
  Band_smc1 = Band_smc1*1 + C_Band_smc1_ig[__min(24,__max(0,j-(385-1)))];
  Band_smc1 = Band_smc1*1 + C_Band_smc1_ih[__min(24,__max(0,j-(409-1)))];
  Band_smc1 = Band_smc1*1 + C_Band_smc1_ii[__min(24,__max(0,j-(433-1)))];
  Band_smc1 = Band_smc1*1 + ((j>16+(457-1))+(j>21+(457-1)));
  Band_smc1 = Band_smc1*1 + ((j>2+(505-1))+(j>8+(505-1)));
  Band_smc1 = Band_smc1*1 + ((j>1+(553-1))+(j>5+(553-1))+(j>8+(553-1))+(j>11+(553-1))+(j>15+(553-1)));

  flag10 = (c1==0);

  mc10_e = 0;
  count10[mc10_e].Process( rc, flag10, C_count10_wr, C_count10_mw );

  mc1_e = 0;
  mc1_e = mc1_e*2 + ((ch>0+(1-1)));
  mc1_e = mc1_e*2 + ((i>6+(1-1)));
  mc1_e = mc1_e*(C_Band_smc1_Volume) + (Band_smc1);

  Smc1_d = 0;
  Smc1_d = Smc1_d*3 + ((i>0+(1-1))+(i>4+(1-1)));
  if( flag10 ) c1=0; else {
    c1 += C_count1_bas;
    count1[mc1_e].Process( rc, c1, 0+C_count1_bas, 577+C_count1_bas, C_count1_wr0, C_count1_mw, Scount1[Smc1_d], C_count1_wr1 );
    c1 -= C_count1_bas;
  }

  j = c1;//p_c0[i];

  Band_smc0 = 0;
  Band_smc0 = Band_smc0*1 + ((j>23+(265-1)));
  Band_smc0 = Band_smc0*1 + ((j>18+(337-1)));
  Band_smc0 = Band_smc0*1 + ((j>3+(409-1)));
  Band_smc0 = Band_smc0*1 + ((j>15+(457-1)));
  Band_smc0 = Band_smc0*1 + ((j>8+(553-1)));

  flag00 = (k==0);

  mc00_e = 0;
  count00[mc00_e].Process( rc, flag00, C_count00_wr, C_count00_mw );

  mc0_e = 0;
  mc0_e = mc0_e*4 + ((i>0+(1-1))+(i>4+(1-1))+(i>6+(1-1)));
  mc0_e = mc0_e*(C_Band_smc0_Volume) + (Band_smc0);

  Smc0_d = 0;
  Smc0_d = Smc0_d*2 + ((i>0+(1-1)));
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
