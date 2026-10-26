
#define MProcess( x,y )  \
{                        \
  int temp = y;          \
  x.Process(rc, temp );     \
  y = temp;              \
}

#define MProcess1( x,y,z,t )  \
{                            \
  int temp = y;              \
  x.Process(rc, temp,z,t );     \
  y = temp;                  \
}

#define MProcess2( x,y,z,t,u,v )  \
{                            \
  int temp = y;              \
  x.Process(rc, temp,z,t,u,v );     \
  y = temp;                  \
}

Number1ii<2>    x_emph;
Node2ii         x_org;
Node2ii         x_cop;
Number1ii<2>    x_modext;
Number1ii<2>    x_mod;
Node2ii         x_ext;
Periodic1ii<32> x_padding;
Number1ii<2>    x_smp;
Number1ii<4>    x_bitrat;
Node2ii         x_crc;
Node2ii         x_ID;
Node2ii         x_IDex;
Number1ii<5>    x_privatebits;

Number1ii<9>    x_sfcomp2[2]; // MPEG2 version
Node2ii         x_winswf[4];
Node2ii         x_preflg[32];
Node2ii         x_scfscl[64];
byte            p_scfscl[2][2];
Node2ii         x_cn1tbl[64];
Periodic1ii<32> x_bltyp1;
Number1ii<2>    x_bltyp2;
byte            p_bltype;
Node2ii         x_mixblk;         // rarely used
Number1ii<3>    x_sbgain[3];

Node2ii         x_scfsi[8][4];
byte            p_scfsi[2][4];

void Init_Header() {

  memset( p_scfscl,0, sizeof(p_scfscl) );
  p_bltype = 0;

  Init_Periodic1ii( x_padding );
  Init_Periodic1ii( x_bltyp1 );

  Init_Number1ii( x_emph );
  Init_Number1ii( x_modext );
  Init_Number1ii( x_mod );
  Init_Number1ii( x_smp );
  Init_Number1ii( x_bitrat );
  Init_Number1ii( x_privatebits );
  Init_Number1ii( x_sfcomp2 );
  Init_Number1ii( x_bltyp2 );
  Init_Number1ii( x_sbgain );

  Init_Node2ii( x_org );
  Init_Node2ii( x_cop );
  Init_Node2ii( x_ext );
  Init_Node2ii( x_crc );
  Init_Node2ii( x_ID );
  Init_Node2ii( x_IDex );
  Init_Node2ii( x_winswf );           
  Init_Node2ii( x_preflg );
  Init_Node2ii( x_scfscl );
  Init_Node2ii( x_cn1tbl );
  Init_Node2ii( x_mixblk );

  memset( p_scfsi, 0, sizeof(p_scfsi)  );
  Init_Node2ii( x_scfsi );
}

void ProcessHeader( void ) {
  int gr, ch, b, i, j;
  RangeCoder& rc = *this;

  h.layer=1; // layer3
  h.syncword=0x7FF;

#ifndef DISABLE_hdrflgs
  // Process the header
  MProcess2( x_emph,   h.emphasis, -1,4, M_x_emph_wr, M_x_emph_mw );
  MProcess2( x_modext, h.mode_ext, -1,4, M_x_modext_wr, M_x_modext_mw );
  MProcess2( x_mod,        h.mode, -1,4, M_x_mod_wr, M_x_mod_mw );
  MProcess1( x_padding, h.padding, M_x_padding_wr, M_x_padding_mw );
  MProcess2( x_smp,   h.frequency, -1,4, M_x_smp_wr, M_x_smp_mw );
  MProcess1( x_crc,  h.protection, M_x_crc_wr, M_x_crc_mw );
  MProcess1( x_ID,           h.ID, M_x_ID_wr, M_x_ID_mw );
  if( h.ID==0 ) MProcess1( x_IDex, h.IDex, M_x_IDex_wr, M_x_IDex_mw );
  MProcess2( x_bitrat,  h.bitrate, -1,16, M_x_bitrat_wr, M_x_bitrat_mw );
  MProcess1( x_org,    h.original, M_x_org_wr, M_x_org_mw );
  MProcess1( x_cop,   h.copyright, M_x_cop_wr, M_x_cop_mw );
  MProcess1( x_ext,    h.privatex, M_x_ext_wr, M_x_ext_mw );
  x_privatebits.Process( rc, si.private_bits, -1,32, M_x_privatebits_wr, M_x_privatebits_mw );
#endif
 
  if( h.ID!=0 ) h.IDex=1;
  int ngr = h.ID;
  int stereo = (h.mode!=3)+1;

  // !!! MPEG1 only 
  if( ngr ) for( ch=0; ch<stereo; ch++ ) {
    for( i=0,j=0; i<4; i++ ) {
#ifndef DISABLE_s1scfsi
      x_scfsi[ch+2*p_scfsi[ch][i]+4*j][i].Process( rc, si.scfsi[ch][i], M_x_scfsi_wr, M_x_scfsi_mw );
#endif
      p_scfsi[ch][i] = (j = si.scfsi[ch][i]);
    }
  }

  for( gr=0; gr<=ngr; gr++ ) {
    for( ch=0; ch<stereo; ch++ ) {

      gr_info& gi = si.gi[gr][ch];
#ifndef DISABLE_glgain
      Process_GlGain( gi.global_gain, ch, gr );
//      x_glgain[0].Process( rc, gi.global_gain, -1, 256, C_x_glgain_0_wr, C_x_glgain_0_mw );
#endif

#ifndef DISABLE_sfcomp
      if( ngr ) {
        Process_sfcomp( gi.scalefac_compress, ch );
      } else {
        x_sfcomp2[ch].Process( rc, gi.scalefac_compress,-1,512, M_x_sfcomp2_wr, M_x_sfcomp2_mw ); // 9 bits
      }
#endif

#ifndef DISABLE_scfscl
      x_winswf[ch+2*(p_bltype>0)].Process( rc, gi.window_switching_flag, M_x_winswf_wr, M_x_winswf_mw );
      if( ngr ) x_preflg[ch+2*(gi.scalefac_compress&15)].Process( rc, gi.preflag, M_x_preflg_wr, M_x_preflg_mw );
      x_scfscl[ch+2*(p_scfscl[gr][ch]&1)+4*(gi.scalefac_compress&15)].Process( rc, gi.scalefac_scale, M_x_scfscl_wr, M_x_scfscl_mw );
      (p_scfscl[gr][ch]<<=1) += gi.scalefac_scale;
#endif

#ifndef DISABLE_cn1tbl
      x_cn1tbl[ch+2*(gi.scalefac_scale)].Process( rc, gi.count1table_select, M_x_cn1tbl_wr, M_x_cn1tbl_mw );
#endif

      if( gi.window_switching_flag ) {

        if( ch==0 && p_bltype<2 ) p_bltype++; // 0->1

        j = (gi.block_type!=p_bltype);
#ifndef DISABLE_bltyp1
        x_bltyp1.Process( rc, j, M_x_bltyp1_wr, M_x_bltyp1_mw );
#endif
        if( j ) {
          j = gi.block_type;
#ifndef DISABLE_bltyp2
          x_bltyp2.Process( rc, j, -1,4, M_x_bltyp2_wr, M_x_bltyp2_mw );
#endif
          if( j ) {
            gi.block_type = j;
            p_bltype = 0;
          } else {
            gi.block_type = j;
            p_bltype++;
          }
        } else {
          gi.block_type=p_bltype;
        }

        if( ch==1 && p_bltype==3 ) p_bltype=0;

#ifndef DISABLE_mixblk
        x_mixblk.Process( rc, gi.mixed_block_flag, M_x_mixblk_wr, M_x_mixblk_mw );
#endif

#ifndef DISABLE_sbgain
        x_sbgain[0].Process( rc, gi.subblock_gain[0], -1,8, M_x_sbgain_0_wr, M_x_sbgain_0_mw );
        x_sbgain[1].Process( rc, gi.subblock_gain[1], -1,8, M_x_sbgain_1_wr, M_x_sbgain_1_mw );
        x_sbgain[2].Process( rc, gi.subblock_gain[2], -1,8, M_x_sbgain_2_wr, M_x_sbgain_2_mw );
#endif

      } // if( gi.window_switching_flag )

    } // for ch
  } // for gr

}

