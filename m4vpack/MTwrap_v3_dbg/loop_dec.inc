
  c_goal = f_parts ? ~qword(0) : c_out + qword(buf1_N)*OutBufsize; // parts mode: only round-end flags (or a finished worker) switch

  while( f_eof != f_eof_all ) {
    //if( CheckTimer(500) ) yield(this,8);

    f_busy=0;
    int dbg_stuck = -1; // (dbg) the worker whose full input ring stopped the parse

    // a failed worker ends the job: stop parsing, EOF to the others
    if( f_err==0 ) for( i=0; i<th_N; i++ ) if( C1[i].f_fail ) { f_err=1; f_quit=1; InpBuf_l=0; break; }

    // blocks put aside (dynbuff) go to their workers as soon as these have
    // room, before any more is read: else the worker whose output is next
    // could wait on its own blocks, put aside, while the reading went on,
    // putting aside the others' (more and more of a stream, on one whose
    // parts were not in step)
    for( i=0; i<th_N; i++ )
      while( B.Load(i) && (p=que1[i].out()) ) {
        l = B.CopyDrop(i,p->buf);
        p->f_eof = (l==0);
        que1[i].push(l);
        dlog.at( MTDBG_DYN, i, l, l==0 );
        f_busy = 1;
      }

    byte* InpBug = InpBuf;

    if( InpBuf_l<InpBufsize ) {

      if( f_quit ) l=0; else {

        if( f_quit==0 ) dbg_chkinp(); 
        l=getinplen();

        if( (InpBuf_l==0) && (l==InpBufsize) ) {

          InpBug = (byte*)inpptr; // use outside buffer directly, if its a full buffer
          inpptr += l;

        } else {

          byte* q = &InpBug[InpBuf_l]; uint i;
          uint n = InpBufsize-InpBuf_l; 
          for( i=0; i<n; ) {
            if( i>0 ) dbg_chkinp(); 
            l = Min<int>( (InpBufsize+n)-i, getinplen() );
            memcpy( &q[i], (byte*)inpptr, l );
            inpptr+=l; i+=l;
            if( f_quit ) break;
          }
          l = i;
        }

        c_inp += l;
        InpBuf_l += l;
      }

    } // if InpBuf_l

    if( InpBuf_l==0 ) {  

      InpBug = InpBuf;

      for( i=0; i<th_N; i++ ) {
        if( p=que1[i].out() ) {
          if( B.Load(i) ) goto DoLoad;
          p->f_eof=1;
          que1[i].push(0);
          dlog.at( MTDBG_EOF, i, 0, 0 );
        }
      }

    } else {

      uint f_dynbuff;
      byte a0,a1,a2;

      i = InpBug[0]; // thread id for this block
      if( (i&0x3F)>=th_N ) { f_err=1; f_quit=1; InpBuf_l=0; continue; } // bad/truncated input: stop parsing, send EOF to all workers
      // a block for a worker after its empty (EOF) block, or after the worker has
      // stopped, is not valid (and would otherwise pile up in dynbuff unbounded)
      if( ((f_eofblk>>(i&0x3F))&1) || C1[i&0x3F].f_done ) { f_err=1; f_quit=1; InpBuf_l=0; continue; }
      p = que1[i&0x3F].out();
      f_dynbuff = 0;

      if( p==0 ) {

        if( InpBug!=InpBuf ) {
          memcpy( &InpBuf[0], &InpBug[0], InpBuf_l );
          InpBug = InpBuf;
        }

        // only use dynbuff if the thread has nothing to do: inp=empty, out=empty
        // (the que1[j] check was missing: blocks went to dynbuff whenever que2[j]
        // was momentarily empty, so a valid archive ended up mostly in RAM)
        if( (que2[j].inp()==0) && (que1[j].inp()==0) ) {
          f_dynbuff=1; p=&tmp;
          goto ParseAnyway; // Save input in dynbuff
        }
        dbg_stuck = i&0x3F;

      } else {

        if( B.Load(i&63) ) {

DoLoad:
          if( InpBug!=InpBuf ) {
            memcpy( &InpBuf[0], &InpBug[0], InpBuf_l );
            InpBug = InpBuf;
          }

          i &= 0x3F;
          l = B.CopyDrop(i,p->buf);
          p->f_eof = (l==0);
          que1[i].push(l); // set size / allow access
          dlog.at( MTDBG_DYN, i, l, l==0 );
          f_busy = 1;
          goto QueueInput;
        }

ParseAnyway:

        if( (i&0xC0)==0x80 ) { // tail mark

          l = (uint(InpBug[1])<<8) + InpBug[2]; 
          n = Max<uint>(l+2,5);

          // assert
          // L>=2: the encoder never writes a 0-byte tail; one would end the worker early
          if( (l<2) || (n>InpBuf_l) || (n>InpBufsize) ) { f_err=1; f_quit=1; InpBuf_l=0; continue; } // bad/truncated input: stop parsing, send EOF to all workers
          p->f_eof = 0; // the slot is reused: don't inherit a stale flag

          // swapped, because at datasize=1 (l=2) they would overlap with len field
          a0 = InpBug[l+1];
          a1 = InpBug[l+0];
          // l here is the tail's length for i'th thread

        } else {

          uint N = ((i&0xC0)!=0xC0) ? InpBufsize - ((i&0xC0)==0x40) : 1;
          p->f_eof = (N<=1);

          if( InpBuf_l<N ) { f_err=1; f_quit=1; InpBuf_l=0; continue; } // bad/truncated input: stop parsing, send EOF to all workers

          n = l = N;
        }

        if( f_dynbuff==0 ) {
          // copy normally to i'th thread's input queue
          memcpy( &p->buf[0], &InpBug[1], l-1 );
        } else {
          p->init( B.Save(i&63,&InpBug[1],l-1), l-1 );
          //p->buf = B.Save(i&63,&InpBug[1],l-1);
        }

        if( (i&0xC0)==0x80 ) { // tail mark
          p->buf[0] = a0;
          p->buf[1] = a1;
        }

        InpBuf_l -= n;
        //l_memcpy( &InpBuf[0], &InpBug[n], InpBuf_l );
        memmove( &InpBuf[0], &InpBug[n], InpBuf_l );
        InpBug = InpBuf;

        if( (i&0xC0)==0xC0 ) f_eofblk |= qword(1)<<(i&0x3F);
        f_seen |= qword(1)<<(i&0x3F);

        i &= 0x3F; 
        if( f_dynbuff==0 ) {
          // add buffer to thread's input queue
          que1[i].push(l-1); // set size / allow access
        }
        f_busy = 1; // a block was consumed (also when it went to dynbuff: no worker may signal, but there is more to parse)
        dlog.at( MTDBG_BLK, i, l-1, f_dynbuff );
      }

    }

QueueInput:
    // j'th block from j'th thread
    if( p=que2[j].inp() ) {

      l = p->len;
      c_out += l;
      if( f_parts && (p->flags & 2) ) c_goal = c_out; // worker j ended its round with this buffer

      if( l>0 ) {
        byte* q = p->buf; uint i,n=l; 
        for( i=0; i<n; ) {
          dbg_chkout(); l = Min<int>( n-i, getoutlen() );
          memcpy( (byte*)outptr, &q[i], l );
          outptr+=l; i+=l;
        }
        l = i;
      }

      dlog.at( MTDBG_OUT, j, l, p->flags );
      que2[j].pop();
      f_busy=1;

    }

    // f_done and Q2.eof -> no more data from this thread, set EoF
    // needs inp() check again because
    if( C1[j].f_done && (que2[j].inp()==0) ) f_eof|=qword(1)<<j;

    // if EOF is set for this thread, there's no way for it to reach the goal by increasing c_out
    if( (f_eof>>j)&1 ) c_goal=c_out; //{ j=(j+1) % th_N; f_busy=1; }

    // if goal is reached, switch to next thread
    if( c_out>=c_goal ) {
      j = (j+1) % th_N; // j'th block to j'th thread
      dlog.at( MTDBG_ROUND, j, c_out, 0 );
      c_goal = f_parts ? ~qword(0) : c_goal + qword(buf1_N)*OutBufsize;
      f_busy = 1;
    }

    // f_eof may have reached f_eof_all in this very iteration: waiting then would
    // never return
    if( (f_busy==0) && (f_eof!=f_eof_all) )
      dbg_wait( dbg_stuck>=0 ? MTDBG_W_RING : InpBuf_l==0 ? MTDBG_W_ENDED : MTDBG_W_OTHER, dbg_stuck );
  }

//printf( "!out of loop!\n" ); fflush(stdout);

  // every worker has stopped: check that the container was used up exactly
  if( f_err==0 ) {
    for( i=0; i<th_N; i++ ) {
      if( ((f_seen>>i)&1)==0 ) f_err=1;  // the encoder writes at least one block per worker
      if( C1[i].inp_left ) f_err=1;      // stopped with input left over (e.g. records after its last chunk)
      if( que1[i].pending() ) f_err=1;   // ... or with unread buffers in its queue
      if( B.Load(i) ) f_err=1;           // ... or with blocks still in dynbuff
    }
  }


