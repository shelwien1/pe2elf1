
// Layer I/II frames, see mp3_parse/parse_l12.inc for what the record holds.
// Everything the frame writer uses is coded; GrbLen (ancillary bits) follows
// from the frame size and the bit allocation. Channel 1 is coded with
// channel 0 as context: broadcast streams are often mono sent as stereo.

Node2ii      l12_frame;              // layer III frame or layer I/II frame

Node2ii      l12_lay[2];             // by previous header's value
Node2ii      l12_hbit[5][2];         // ID, protection, private, copyright, original
Number1ii<4> l12_br[16];
Number1ii<2> l12_fs[4];
Number1ii<2> l12_mode[4];
Number1ii<2> l12_ext[4][4];          // [mode][previous]
Number1ii<2> l12_emph[4];
Node2ii      l12_pad[256];           // by the last 8 padding bits
Node2ii      l12_crcok;

Number1ii<4> l12_alloc[2][2][32][16][8]; // [layer I][ch][sb][previous frame's][below / other channel]
Number1ii<2> l12_scfsi[2][5][8];     // [ch][previous frame's, 4=none][sb/4]
Node2ii      l12_seq[2][2][3];       // channel 1 scalefactor = channel 0's: [layer I][last one][which]
Number1ii<6> l12_scf[2][65][3];      // [layer I][predicted, 64=none][which]
Node2ii      l12_eq[2][3][2][2];     // channel 1 sample = channel 0's: [layer I][previous one in the subband, 2=none][last one][class>4]
Number1ii<5> l12_e[2][17][18][9][5]; // sample size: [layer I][class][previous one in the subband, 17=none]
                                     //   [channel 1: channel 0's here, else the subband below; 8=none][channel 1: subband below/2, 4=n/a]
Node2ii      l12_m[2][2][17][17];    // the 2 bits below the top one: [which][layer I][class][size]
Node2ii      l12_sgn[2][3][4];       // sign: [layer I][previous one in the subband, 0=zero][channel 0's here, 3=n/a]
Node2ii      l12_anc[256][256];      // ancillary bits: [this byte so far][previous byte]

uint l12_ph;                         // previous layer I/II header
uint l12_padh;
int  l12_lseq, l12_leq;
byte l12_pa[2][32], l12_ps[2][32], l12_pf[2][32]; // previous frame's allocation, scfsi, last scalefactor
int  l12_ready;                      // Init_L12Tables has run

// l12_frame codes whether a frame is layer III; the rest is made when the
// first layer I/II frame comes (both sides know it then), as most streams
// have none
void Init_L12( void ) {
  Init_Node2ii( l12_frame );
  l12_ready = 0;
}

void Init_L12Tables( void ) {
  l12_ready = 1;
  Init_Node2ii( l12_lay );
  Init_Node2ii( l12_hbit );
  Init_Number1ii( l12_br );
  Init_Number1ii( l12_fs );
  Init_Number1ii( l12_mode );
  Init_Number1ii( l12_ext );
  Init_Number1ii( l12_emph );
  Init_Node2ii( l12_pad );
  Init_Node2ii( l12_crcok );
  Init_Number1ii( l12_alloc );
  Init_Number1ii( l12_scfsi );
  Init_Node2ii( l12_seq );
  Init_Number1ii( l12_scf );
  Init_Node2ii( l12_eq );
  Init_Number1ii( l12_e );
  Init_Node2ii( l12_m );
  Init_Node2ii( l12_sgn );
  Init_Node2ii( l12_anc );
  l12_ph = l12_padh = 0;
  l12_lseq = 1; l12_leq = 0;
  memset( l12_pa, 0, sizeof(l12_pa) );
  memset( l12_ps, 4, sizeof(l12_ps) );
  memset( l12_pf, 64, sizeof(l12_pf) );
}

static int BitLen( uint x ) { int n=0; for( ; x; x>>=1 ) n++; return n; }

void ProcessFrame( void ) {
  RangeCoder& rc = *this;
  int t = (h.layer!=1);
  l12_frame.Process( rc, t, TTable[24], 16 );
  if( t ) { if( !l12_ready ) Init_L12Tables(); ProcessL12(); } else Process();
}

void ProcessHeaderL12( void ) {
  RangeCoder& rc = *this;
  const word* W = TTable[24]; const int MW = 16;
  header& p = (header&)l12_ph;
  int t = (h.layer==3);

  l12_lay[p.layer==3].Process( rc, t, W, MW );
  h.layer = t ? 3 : 2;
  MProcess1( l12_hbit[0][p.ID], h.ID, W, MW );
  MProcess1( l12_hbit[1][p.protection], h.protection, W, MW );
  MProcess2( l12_br[p.bitrate], h.bitrate, 0, 15, W, MW );
  MProcess2( l12_fs[p.frequency], h.frequency, -1, 3, W, MW );
  MProcess1( l12_pad[l12_padh&255], h.padding, W, MW );
  MProcess1( l12_hbit[2][p.privatex], h.privatex, W, MW );
  MProcess2( l12_mode[p.mode], h.mode, -1, 4, W, MW );
  MProcess2( l12_ext[h.mode][p.mode_ext], h.mode_ext, -1, 4, W, MW );
  MProcess1( l12_hbit[3][p.copyright], h.copyright, W, MW );
  MProcess1( l12_hbit[4][p.original], h.original, W, MW );
  MProcess2( l12_emph[p.emphasis], h.emphasis, -1, 4, W, MW );
  h.syncword = 0x7FF; h.IDex = 1;

  l12_padh += l12_padh + h.padding;
  l12_ph = (uint&)h;
}

// scalefactor k of channel ch, predicted by pr (64 = nothing to go by);
// for channel 1 first a flag for "same as channel 0's"
void ProcessScfL12( int ch, int sb, int k, int pr ) {
  RangeCoder& rc = *this;
  int t, v, lay1 = (h.layer==3);
  byte& f = L.scf[ch][sb][k];
  if( ch==1 && L.alloc[0][sb] ) {
    t = (f==L.scf[0][sb][k]);
    l12_seq[lay1][l12_lseq][k].Process( rc, t, TTable[14], 32 );
    l12_lseq = t;
    if( t ) { f = L.scf[0][sb][k]; return; }
  }
  v = f; l12_scf[lay1][pr][k].Process( rc, v, -1, 64, TTable[14], 32 ); f = v;
}

void ProcessL12( void ) {
  RangeCoder& rc = *this;
  int sb, ch, s, i, b, k, t, v, m, e, n, q, mid, emax, cls, en, ec;
  byte pe[2][32], psg[2][32], peq[32]; // previous sample's size, sign; previous channel 1 = channel 0 flag
  byte e0[3], s0[3];                   // channel 0's size and sign for the samples of this granule

  if( rc.rc_ProcMode==1 ) {
    byte* p = (byte*)&h;
    memset( p, 0, (byte*)&Garbage-p );
  }

  ProcessHeaderL12();
  if( !HeaderOk() || h.emphasis==2 ) BadData();

  if( h.protection==0 ) {
    t = L.crcok;
    l12_crcok.Process( rc, t, TTable[24], 16 );
    L.crcok = t;
    if( t==0 ) { v = CRC; rc.NProcess( v, 16, 0, 1<<16 ); CRC = v; }
  }

  int lay1 = (h.layer==3), nch = (h.mode!=3)+1;
  int tab = lay1 ? 0 : L2Table();
  int sblimit = lay1 ? 32 : l2_sblimit[tab];
  int bound = L12Bound( sblimit );

  // bit allocation
  for( sb=0; sb<sblimit; sb++ ) {
    int hi = lay1 ? 15 : 1<<L12Nbal( tab, sb );
    for( ch=0; ch<((sb<bound)?nch:1); ch++ ) {
      int c2 = Min<int>( 7, ch ? L.alloc[0][sb] : sb ? L.alloc[0][sb-1] : 0 );
      v = L.alloc[ch][sb];
      l12_alloc[lay1][ch][sb][l12_pa[ch][sb]][c2].Process( rc, v, -1, hi, TTable[14], 32 );
      L.alloc[ch][sb] = l12_pa[ch][sb] = v;
    }
    if( sb>=bound ) L.alloc[1][sb] = L.alloc[0][sb];
  }

  // scalefactor select info
  if( !lay1 ) for( sb=0; sb<sblimit; sb++ ) for( ch=0; ch<nch; ch++ ) if( L.alloc[ch][sb] ) {
    v = L.scfsi[ch][sb];
    l12_scfsi[ch][l12_ps[ch][sb]][sb>>2].Process( rc, v, -1, 4, TTable[17], 32 );
    L.scfsi[ch][sb] = l12_ps[ch][sb] = v;
  }

  // scalefactors, each predicted by the one before it in the subband
  // (channel 1's first one by channel 0's)
  for( sb=0; sb<sblimit; sb++ ) for( ch=0; ch<nch; ch++ ) if( L.alloc[ch][sb] ) {
    byte* f = L.scf[ch][sb];
    k = lay1 ? 2 : L.scfsi[ch][sb]; // which of them are sent, as in ProcL12
    ProcessScfL12( ch, sb, 0, (ch && L.alloc[0][sb]) ? L.scf[0][sb][0] : l12_pf[ch][sb] );
    if( k==0 ) ProcessScfL12( ch, sb, 1, f[0] ); else if( k!=3 ) f[1] = f[0];
    if( k!=2 ) ProcessScfL12( ch, sb, 2, f[k==0] ); else f[2] = f[0];
    if( k==3 ) f[1] = f[2];
    l12_pf[ch][sb] = f[2];
  }

  // samples: offset from the middle code, as size, mantissa and sign
  memset( pe, 17, sizeof(pe) );
  memset( psg, 0, sizeof(psg) );
  memset( peq, 2, sizeof(peq) );
  for( s=0; s<12; s++ ) for( sb=0; sb<sblimit; sb++ ) for( ch=0; ch<((sb<bound)?nch:1); ch++ ) {
    if( (b=L.alloc[ch][sb])==0 ) continue;
    if( lay1 ) { k = b+1; cls = (k>3) ? k : 2*(k-2); q = (1<<k)-1; n = 1; } // class with the same number of levels
    else { cls = l2_rows[l2_row[tab][sb]][b]; q = l2_levels[cls]; n = 3; }
    mid  = (q-1)>>1;
    emax = BitLen( mid+1 ); // the all-ones code is mid+1
    int ref = (ch==1 && L.alloc[0][sb]); // channel 0 has samples here
    word* x  = &L.smp[ch][sb][lay1?s:3*s];
    word* x0 = &L.smp[0][sb][lay1?s:3*s];
    for( i=0; i<n; i++ ) {
      if( ref && L.alloc[0][sb]==b ) {
        t = (x[i]==x0[i]);
        l12_eq[lay1][peq[sb]][l12_leq][cls>4].Process( rc, t, TTable[14], 32 );
        peq[sb] = l12_leq = t;
        if( t ) {
          x[i] = x0[i]; v = x[i]-mid; m = (v<0) ? -v : v;
          pe[ch][sb] = BitLen( m ); psg[ch][sb] = m ? 1+(v<0) : 0;
          continue;
        }
      }
      v = x[i]-mid; m = (v<0) ? -v : v;
      e = BitLen( m );
      en = (sb && L.alloc[ch][sb-1]) ? Min<int>( 7, pe[ch][sb-1] ) : 8;
      ec = 4;
      if( ref ) ec = Min<int>( 3, en>>1 ), en = Min<int>( 7, e0[i] );
      l12_e[lay1][cls][pe[ch][sb]][en][ec].Process( rc, e, -1, emax+1, TTable[10], 16 );
      if( e>=2 ) {
        int mm = 1;
        for( k=e-2; k>=0; k-- ) {
          t = (m>>k)&1;
          if( k>=e-3 ) l12_m[e-2-k][lay1][cls][e].Process( rc, t, TTable[10], 16 ); else rc.BProcess( hSCALE, t );
          mm += mm+t;
        }
        m = mm;
      } else m = e;
      if( m ) {
        t = (v<0);
        l12_sgn[lay1][psg[ch][sb]][ref ? s0[i] : 3].Process( rc, t, TTable[14], 32 );
        v = t ? -m : m; psg[ch][sb] = 1+t;
      } else { v = 0; psg[ch][sb] = 0; }
      x[i] = v+mid;
      pe[ch][sb] = e;
      if( ch==0 ) e0[i] = e, s0[i] = psg[0][sb];
    }
  }

  // ancillary data: whatever the frame has left
  GrbLen = CalcFrameLenL12()*8 - CalcL12Bits();
  int c = 1, pb = 0;
  for( i=0; i<GrbLen; i++ ) {
    t = (Garbage[i>>3]>>(7-(i&7)))&1;
    l12_anc[c][pb].Process( rc, t, TTable[20], 16 );
    c += c+t;
    if( c>=256 ) { Garbage[i>>3] = pb = c&255; c = 1; }
  }
  if( c>1 ) Garbage[GrbLen>>3] = (c&((1<<(GrbLen&7))-1)) << (8-(GrbLen&7));
}
