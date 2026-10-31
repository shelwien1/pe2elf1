
  c_goal = c_inp + qword(buf1_N)*InpBufsize;

//printf( "!c_goal=%I64X!\n", c_goal );

//  while( f_eof+1<(1<<th_N) ) {
  while( f_eof != f_eof_all ) {
    //if( CheckTimer(500) ) yield(this,8); // PrintInfo(j);

    f_busy=0;
    int dbg_stuck = -1; // (dbg) the worker whose full input ring stopped the dealing

    // a failed worker ends the job: stop reading input, let the others finish
    if( f_quit==0 ) for( i=0; i<th_N; i++ ) if( C1[i].f_fail ) { f_err=1; f_quit=1; break; }

    // worker j ended before getting EOF -> it failed (r=3); its input ring
    // would stay full forever, so skip it and end the input for the others
    if( C1[j].f_done ) {
      if( f_quit==0 ) { f_err=1; f_quit=1; }
      j = (j+1) % th_N; c_goal = c_inp; part_left = 0;
      f_busy=1; // the next worker may be done as well; no signal would come for it
    }

    if( p=que1[j].out() ) {
 
      if( f_quit ) {
        l = 0; 
      } else if( f_parts ) {

        // parts mode: frames [u64 L][L bytes]; a frame goes whole to worker j
        byte* q = p->buf; uint h, k=0, m;
        if( part_left==0 ) { // a new frame: read its header (it is passed on too)
          for( h=0; h<8; ) {
            dbg_chkinp(); l = Min<uint>( 8-h, getinplen() );
            if( l==0 ) break; // EOF
            memcpy( &phdr[h], (byte*)inpptr, l ); inpptr+=l; h+=l;
          }
          if( h==8 ) { memcpy( &part_left, phdr, 8 ); memcpy( q, phdr, 8 ); k=8; }
          else if( h>0 ) { f_err=1; f_quit=1; } // truncated frame header
        }
        m = k + uint( Min<qword>( qword(p->size-k), part_left ) );
        while( k<m ) {
          dbg_chkinp(); l = Min<uint>( m-k, getinplen() );
          if( l==0 ) { f_err=1; f_quit=1; break; } // truncated frame
          memcpy( &q[k], (byte*)inpptr, l ); inpptr+=l; k+=l;
          part_left -= l;
        }
        l = k;
        c_inp += l;

      } else {

        byte* q = p->buf; n = p->size; uint i;
        for( i=0; i<n; ) {
          dbg_chkinp(); l = Min<int>( n-i, getinplen() );
          memcpy( &q[i], (byte*)inpptr, l );
          inpptr+=l; i+=l;
          if( f_quit ) break;
        }
        l = i;

        c_inp += l;
      }

      if( l==0 ) { p->f_eof=1; c_goal=c_inp; }
      que1[j].push(l); // set size / allow access
      dlog.at( MTDBG_FEED, j, l, l==0 );
      f_busy=1;

      if( f_parts ) {
        if( (part_left==0) || f_quit ) { j = (j+1) % th_N; dlog.at( MTDBG_ROUND, j, c_inp, 0 ); } // frame complete (or EOF pushed): next worker
      } else
      if( c_inp>=c_goal ) {
        j = (j+1) % th_N; // j'th block to j'th thread
        dlog.at( MTDBG_ROUND, j, c_inp, 0 );
        c_goal = c_inp + qword(buf1_N)*InpBufsize;
      }
    } else if( f_quit==0 ) dbg_stuck = j;

    // first come - first serve
    for( i=0; i<th_N; i++ ) { 

      p = que2[i].inp();

      if( p==0 ) {

        // f_done first, then the queue once more: a worker pushes its last block
        // and only then sets f_done, so a queue seen empty BEFORE reading f_done=1
        // may have been filled in between -- that block used to be dropped
        if( C1[i].f_done && (que2[i].inp()==0) ) f_eof |= qword(1) << i; // no buffers and f_done

      } else if( (p->len<=1) && ((f_seen>>i)&1) ) {
        // an empty buffer (a worker that ends a round, parts mode, hands its
        // last one over empty when it ends): nothing to write. Written, a block
        // of the id alone would read as the worker's end (the decoder ends a
        // worker's input at the container's end, so it is not needed), and one
        // cut off would go unnoticed. A worker's first is written, empty or not:
        // the decoder wants a block of each.
        que2[i].pop();
        f_busy=1;
      } else {
        byte* q = p->buf; l = p->len; // includes the first id byte
        uint j=0, n; byte b1,b2;
        f_seen |= qword(1)<<i;

/*
  00xxxxxx: l=0x10000, tid=x
+ 11xxxxxx: l=0x0FFFF, tid=x
  10xxxxxx UU VV: l=0xUUVV, tid=x, +2 tail bytes
- 11xxxxxx UU VV WW: l=0xWWUUVV, tid=x, +3 tail bytes

*/

        // its internal mtwrap buffer, so we can modify it
        if( (l>=2) && (l<=OutBufsize-2) ) {

          // max l = OutBufsize-2; +=2 = OutBufsize, still fits

          q[0] = (i|0x80); // tail flag
          q[l+0] = q[2]; // [2+0]=[2]
          q[l+1] = q[1]; // [2+1]=[1]
          q[1] = byte(l>>8);
          q[2] = byte(l);
          l = Max<uint>(l+2,5); // l=0: impossible; l=1: tid only; l=2: one data byte

        } else {

          // handle l=0xFFFF and l=0x10000 and l=0x0001
          q[0] = l<OutBufsize ? (l>1) ? (i|0x40)/*64k-1*/ : (i|0xC0)/*1*/ : (i|0x00)/*64k*/; 

        }

//static FILE* dbg = fopen( "dbg_c.txt", "wb" );
//fprintf(dbg, "%5i %02X %02X %02X\n", l, q[0],q[1],q[2] ); fflush(dbg);

        c_out += l;

        for( j=0; j<l; ) {
          dbg_chkout(); n = Min<int>( l-j, getoutlen() );
          memcpy( (byte*)outptr, &q[j], n );
          outptr+=n; j+=n;
        }

        dlog.at( MTDBG_BLK, i, l, 0 );
        que2[i].pop();

        f_busy=1;
      }


    } // for

    // f_eof may have reached f_eof_all in this very iteration: waiting then would
    // never return
    if( (f_busy==0) && (f_eof!=f_eof_all) )
      dbg_wait( dbg_stuck>=0 ? MTDBG_W_RING : f_quit ? MTDBG_W_ENDED : MTDBG_W_OTHER, dbg_stuck );
  }

