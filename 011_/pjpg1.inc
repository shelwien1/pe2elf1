
struct pjpg : Coroutine, pjpg0 {

  byte f_lentag[256];
  byte f_ptag[256];

  void init_lentag( void ) {
    uint i;
    bzero( f_lentag ); 
    for( i=0; len2[i]; i++ ) f_lentag[len2[i]]=1;

    bzero( f_ptag );
    for( i=0; parsedtags[i]; i++ ) f_ptag[parsedtags[i]]=1;
  }

  void do_process( void ) {
    uint c,pc,i,j,r,pst,f_MCU;

    init_lentag();

    tag_id=0; l_tag=0;

    pc = 0; // 
    pst = 0; // parser state: 0=default, 1=marker exp, 2,3=len exp, 4=len rec
    f_MCU = 0; // entropy-coded data processing flag
    f_SOS = 0;

    coro2_init();

    while(1) {

      c = get(); if( c==-1 ) break;

//fprintf(stderr, "%05X: %02X %i pst=%i f_MCU=%i f_SOS=%i\n", i, c, l_tag, pst, f_MCU, f_SOS );

      l_tag -= (l_tag>0);

      if( (pst==4) && f_ptag[tag_id] ) {
        // tag structure byte for parsing
        coro2_c = c; 
        r = coro2_process(); if( r==0 ) tag_id=0;
      } else {
        // tag or unknown byte
      }

      (pc<<=8) |= byte(c);

      switch( pst ) {

      case 0:
        if( c==0xFF ) { pst=1; break; } // tag_id prefix
      parse_code: 
        break;

      case 1:
        if( c==0xFF ) break; // FFFF=FF, keep state
        if( c==0 ) { pst=0; c=0xFF; goto parse_code; }; // FF00=FF byte

        tag_id = c;
        f_MCU = 0;
        pst = f_lentag[c] ? 2 : 0; // continue decoding len if the tag has a len field
        if( (c&0xF8)==M_RST0 ) { pst=0; break; } // don't parse RSTx as a new tag

printf("!tag=%02X!\n", tag_id );

        break;

      case 2: 
        pst = 3; 
        break; // len hi

      case 3:
        l_tag = word(pc); // len
        pst = (l_tag<=2) ? (l_tag=0,tag_id=0,0) : (l_tag-=2,4);
        if( (pst==4) && f_ptag[tag_id] ) {
          coro2_init();
          coro2_process(); // run parser until first input
        }
        break; 

      case 4:
        // SOS finished and properly parsed? -> parse as MCU
        if( l_tag==0 ) { pst = 0; f_MCU = f_SOS; }
        break;
      }

    }

    yield(this,0);
  }

};
