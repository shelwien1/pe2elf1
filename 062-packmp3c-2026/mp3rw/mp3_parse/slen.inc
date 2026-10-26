
uint n_slen2[512]; /* MPEG 2.0 slen for 'normal' mode */
uint i_slen2[256]; /* MPEG 2.0 slen for intensity stereo */

void InitSlen( void ) {

  int i,j,k,l;

  for( i=0; i<5; i++ ) {
    for( j=0; j<6; j++ ) {
      for( k=0; k<6; k++ ) {
        int n = k + j * 6 + i * 36;
        i_slen2[n] = i|(j<<3)|(k<<6)|(3<<12);
      }
    }
  }

  for( i=0; i<4; i++ ) {
    for( j=0; j<4; j++ ) {
      for( k=0; k<4; k++ ) {
        int n = k + j * 4 + i * 16;
        i_slen2[n+180] = i|(j<<3)|(k<<6)|(4<<12);
      }
    }
  }

  for( i=0; i<4; i++ ) {
    for( j=0; j<3; j++ ) {
      int n = j + i * 3;
      i_slen2[n+244] = i|(j<<3) | (5<<12);
      n_slen2[n+500] = i|(j<<3) | (2<<12) | (1<<15);
    }
  }

  for( i=0; i<5; i++ ) {
    for( j=0; j<5; j++ ) {
      for( k=0; k<4; k++ ) {
        for( l=0; l<4;l++) {
          int n = l + k * 4 + j * 16 + i * 80;
          n_slen2[n] = i|(j<<3)|(k<<6)|(l<<9)|(0<<12);
        }
      }
    }
  }

  for( i=0; i<5; i++ ) {
    for( j=0; j<5; j++ ) {
      for( k=0; k<4; k++ ) {
        int n = k + j * 4 + i * 20;
        n_slen2[n+400] = i|(j<<3)|(k<<6)|(1<<12);
      }
    }
  }

}

