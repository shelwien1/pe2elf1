
struct dyn_buff {
  enum{ bufsize=65536 };
  enum{ N_th = 64 }; // mtwrap container doesn't support more anyway

  typedef byte* ptr;

  //---------------
  template< class T >
  T* VAlloc( qword s ) {
    s *= sizeof(T);
    s = (s+0xFFF) & (~qword(0xFFF));
    void* r = VirtualAlloc(0, s, MEM_COMMIT, PAGE_READWRITE );
    return (T*)r;
  }

  void VFree( void* p ) {
    VirtualFree( p, 0, MEM_RELEASE );
  }
  //---------------

  struct ptrlist {
    ptr p; // buffer ptr
    ptrlist* next; // next in list
    uint len;
    //uint pad1;
  };

  enum{ blksize = (bufsize)/(sizeof(ptrlist)) };
  struct ptrlistblk {
    ptrlistblk* next;
    ptrlist p[blksize];
  };

  ptr next_free;

  ptrlist* next_ptrlist;
  ptrlistblk* next_ptrlistblk;

  ptrlist* head[N_th]; 
  ptrlist* tail[N_th]; 
  ptrlist* prev[N_th];

  ptrlistblk blk0;

  uint c_alloc0;

  ptr alloc0( void ) {
    ptr p = VAlloc<byte>(bufsize); //new byte[bufsize];
    if( p==0 ) __debugbreak();
    (ptr&)p[0] = 0; 
    c_alloc0++;
    return p;
  }

  void free0( ptr p ) {
    //delete[] p;
    VFree(p);
  }

  ptr alloc( void ) {
    if( next_free==0 ) next_free=alloc0();
    ptr p = next_free; 
    next_free = (ptr&)p[0];
    return p;
  }

  void free( ptr p ) {
    (ptr&)p[0] = next_free;
    next_free = p;
  }

  void pl_add( ptrlistblk* pblk0 ) {
    uint i;
    ptrlistblk& blk0 = pblk0[0];
    blk0.next = 0;
    for( i=0; i<blksize; i++ ) {
      ptrlist&q = blk0.p[i];
      q.p = 0;
      q.next = (i+1<blksize) ? &blk0.p[i+1] : next_ptrlist;
    }
    next_ptrlist = &blk0.p[0];
  }

  ptrlist* pl_alloc( void ) {
    if( next_ptrlist==0 ) {
      ptrlistblk* p = (ptrlistblk*)alloc();
      pl_add( p );
      p->next = next_ptrlistblk;
      next_ptrlistblk = p;
    }
    ptrlist* p = next_ptrlist;
    next_ptrlist = next_ptrlist->next;
    return p;
  }

  void pl_free( ptrlist* p ) {
    p->p = 0;
    p->len = 0;
    p->next = next_ptrlist;
    next_ptrlist = p;
  }


  void Init( void ) {
    uint i;
    next_free = 0;
    next_ptrlist = 0; pl_add( &blk0 );
    next_ptrlistblk = 0;
    for( i=0; i<N_th; i++ ) { head[i]=tail[i]=pl_alloc(); prev[i]=0; }
    c_alloc0 = 0;
  }

  void Quit( void ) {
    uint i, n;

    for( i=0; i<N_th; i++ ) while( head[i]!=tail[i] ) Drop(i);

    for( n=0; next_free; n++ ) free0( alloc() ); 

    while( next_ptrlistblk ) {
      ptrlistblk* next = next_ptrlistblk->next;
      free0( (ptr)next_ptrlistblk ); n++;
      next_ptrlistblk = next;
    }
    //printf( "Quit: n=%i c_alloc0=%i\n", n, c_alloc0 );
    Init();
  }

  // save a buffer
  byte* Save( uint tid, byte* p, uint psize ) {
    ptrlist* pl = tail[tid];
    prev[tid] = pl;
    pl->p = alloc();
    pl->len = psize;
    pl->next = tail[tid] = pl_alloc();
    memcpy( pl->p, p, psize );
    return pl->p;
  }

  byte* Load( uint tid ) {
    ptrlist* pl = head[tid];
    return pl->p;
  }

  uint LoadSize( uint tid ) {
    ptrlist* pl = head[tid];
    return pl->len;
  }

  uint CopyDrop( uint tid, byte* buf ) {
    uint l = 0;
    ptrlist* pl = head[tid];
    if( pl && (pl->p) ) {
      l = pl->len;
      memcpy( buf, pl->p, l );
    }
    Drop(tid);
    return l;
  }

  void Drop( uint tid ) {
    ptrlist* pl = head[tid];
    if( pl->p ) {
      head[tid] = pl->next; // remove list node from thread's list
      free( pl->p ); // free buffer
      pl_free( pl ); // free list node
    } else prev[tid]=0;
  }

};


