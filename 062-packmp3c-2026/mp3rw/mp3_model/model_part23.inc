
Node2ii       p23clrflag;
Node2ii       y_part23[2][2]; // part23 coincidence
Node2ii       s_part23[2][2]; // delta's sign
Number1ii<12> z_part23[2][2][2]; // delta to actual value

int last_p23sum;
int prev_p23sum;

void Init_Part23( void ) {
  Init_Number1ii(z_part23);

  Init_Node2ii( p23clrflag );
  Init_Node2ii( y_part23 );
  Init_Node2ii( s_part23 );
}

//layer *head, short mdctPF[2][2][576], SideInfo_t *sideinf, ScaleFac_t *scf, any_data *left
void ProcessPart23( void ) {

  int j,k,gr,ch,flag;
  int ngr = h.ID;
  int stereo = (h.mode!=3)+1;
  int p23sum = 0;
  RangeCoder& rc = *this;

#ifndef DISABLE_p23clr
  p23clrflag.Process( rc, si.p23cleared, M_p23clrflag_wr, M_p23clrflag_mw );
#endif
  if( si.p23cleared ) {

    for( gr=0; gr<=ngr; gr++ ) {
      for( ch=0; ch<stereo; ch++ ) {
        gr_info& gi = si.gi[gr][ch];
#ifndef DISABLE_p23back
        rc.NProcess( gi.backup_p23, 12, 0, 4096 );
#endif
        gi.part2_3_length = 0;
      }
    }

  } else {

    for( gr=0; gr<=ngr; gr++ ) {
      for( ch=0; ch<stereo; ch++ ) {

        gr_info& gi = si.gi[gr][ch];

        k = Len_MDCT( MC[gr][ch], gi );
        int l = k + gi.p23len, l1=l; // predicted length

        l = gi.part2_3_length - l1;
        flag = (l!=0);
#ifndef DISABLE_ypart23
        y_part23[gr][ch].Process( rc, flag, M_y_part23_wr, M_y_part23_mw );
#endif
        if( flag ) {
          if( l>0 ) l--; // remove zero
          int s = (l<0); if(s) l=-l;
#ifndef DISABLE_spart23
          s_part23[gr][ch].Process( rc, s, M_s_part23_wr, M_s_part23_mw );
#endif
#ifndef DISABLE_zpart23
          z_part23[gr][ch][s].Process( rc, l, -1, 4096, M_z_part23_wr, M_z_part23_mw );
#endif
          if(s) l=-l;
          if( l>=0 ) l++;
        } else l=0;

        gi.part2_3_length = l1 + l;
        p23sum += gi.part2_3_length;

      }
    }
  }

  last_p23sum = p23sum;

}


