/* ddsdet_drive.inc -- the 4-pin coroutine driver; part of ddsdet.cpp.
 *
 * Lib3's CoroFileProc services r=1 (refill pin0) and r=2 (flush pin1) only.
 * cs/ds need two more: r=3 writes pin2's window to the archive file, r=4
 * reads pin3 from it. */

/* One carrier per direction.  o0rc puts the two instantiations in a union to
 * share the stack/window storage; ddsdet cannot, because it holds a
 * std::vector (gapbuf) and a union member with a non-trivial constructor
 * would need a user-provided ctor/dtor for the union.  Two carriers cost
 * ~400 KB of BSS, which is demand-zero and irrelevant next to the codec. */
/* Driver.  Lib3's CoroFileProc handles r=1 (refill pin0) and r=2 (flush pin1)
 * and breaks on anything else, so the two extra pins the coded stream uses
 * need their own cases:
 *   r=3  pin2 full  -- the encoder's archive window
 *   r=4  pin3 dry   -- the decoder's archive window
 * S1 keeps the archive itself in memory (rcbuf on the way out, the loaded
 * image on the way in) so the D5S3 container is produced and consumed exactly
 * as before; S3 swaps these two cases for direct file I/O.  Independent pins
 * have no ordering constraints between them, so restfile and archive bytes
 * interleaving is a non-issue. */
template<class Carver>
struct CoroDrive : Carver {
  enum { bufsize = 1<<16 };
  ALIGN(4096) byte inpbuf[bufsize];
  ALIGN(4096) byte outbuf[bufsize];
  ALIGN(4096) byte rcobuf[bufsize];   // pin2: coded stream out
  ALIGN(4096) byte rcibuf[bufsize];   // pin3: coded stream in

  FILE* rcf = 0;                      // the archive when READING
  LazyOut rcout;                 // the archive when WRITING: opened on first byte

  void rc_drain( void ) {             // pin2's window straight out to the archive
    size_t l = (size_t)(this->pin[2].ptr - this->pin[2].beg);
    if( l ) { FILE* o = rcout.w(); if( o ) fwrite(rcobuf, 1, l, o); }
    this->pin[2].addbuf(rcobuf, bufsize);
  }

  LazyOut out;                   // pin1's destination, likewise deferred

  uint processfile( FILE* f, FILE* g ) {
    (void)g;                          // kept for signature compatibility
    uint l, r = 0;
    this->coro_init();
    this->addout(outbuf, bufsize);
    this->pin[2].addbuf(rcobuf, bufsize);
    this->pin[3].addbuf(rcibuf, 0);   // empty: first get() yields for a fill
    while(1) {
      r = this->coro_call(this);
      if( r==1 ) {                    // pin0 dry: more container input
        l = (uint)fread(inpbuf, 1, bufsize, f);
        if( l==0 ) this->f_quit = 1;
        this->addinp(inpbuf, l);
      } else if( r==3 ) {             // pin2 full: archive window ready
        rc_drain();
      } else if( r==4 ) {             // pin3 dry: pull more of the archive
        size_t k = rcf ? fread(rcibuf, 1, bufsize, rcf) : 0;
        this->pin[3].addbuf(rcibuf, (uint)k);
        this->pin[3].f_EOF = (k==0);
      } else {                        // r=0 quit, r=2 pin1 full, r=3.. error
        l = this->getoutsize();
        if( l>0 ) { FILE* o = out.w(); if( o ) fwrite(outbuf, 1, l, o); }
        if( r!=2 ) break;
        this->addout(outbuf, bufsize);
      }
    }
    rc_drain();                       // whatever the final flush left in pin2
    return r;
  }
};

/* Built on first use: a run is either a carve or a restore, never both, and
 * each driver carries a 64 KB coroutine stack plus the carver's state.  As
 * statics both were constructed at load whichever mode was asked for. */
static CoroDrive< ddsdet<0> >* Menc_ = nullptr;
static CoroDrive< ddsdet<1> >* Mdec_ = nullptr;
CoroDrive< ddsdet<0> >& Menc() { if(!Menc_) Menc_ = d5c::onew< CoroDrive< ddsdet<0> > >("carve driver"); return *Menc_; }
CoroDrive< ddsdet<1> >& Mdec() { if(!Mdec_) Mdec_ = d5c::onew< CoroDrive< ddsdet<1> > >("restore driver"); return *Mdec_; }
