/* xad_drive.inc -- part of xadpcm.cpp; not standalone.
 *
 * XadDrive<Carrier> -- the coroutine pump, the two carriers, and reporting.
 * The only part of the program that knows what a FILE* is. */

/* The coroutine pump.  Lib3's CoroFileProc services r==1 (refill pin0 from one
   FILE*) and r==2 (flush pin1 to one FILE*); xadpcm needs three things it does
   not have:
     - several inputs behind pin0, because `xadpcm c a.wav b.wav out` codes the
       concatenation of them as one stream;
     - an output that may be a file, a memory buffer (-t keeps the coded stream
       in memory so it can be verified before anything is written), or a
       SEQUENCE of files split by the decoded file table;
     - not setting f_quit until the LAST input is exhausted, or a multi-file
       encode would stop after the first.
   XadDrive is the only part of the program that knows what a FILE* is. */
template<class Carrier>
struct XadDrive : Carrier {
  enum { bufsize = 1<<16 };
  ALIGN(4096) byte inpbuf[bufsize];
  ALIGN(4096) byte outbuf[bufsize];

  // input: either a list of paths, opened in turn, or one memory image
  char** in_paths = nullptr;
  int in_n = 0, in_i = 0;
  FILE* in_f = nullptr;
  const u8* in_mem = nullptr;
  size_t in_memn = 0, in_mempos = 0;

  // output: at most one of these is set
  const char* out_path = nullptr;
  string out_dir;    // out_path plus a trailing separator, for the split sink
  vector<u8>* out_mem = nullptr;
  int out_split = 0; // out_path names a directory when the file table is non-empty
  FILE* out_f = nullptr;
  size_t sp_i = 0;
  u64 sp_left = 0;

  /* Next window of input.  Advances through the path list, so a zero-length
     input cannot make the pump hand back 0 bytes forever: the loop only
     returns 0 once there is no path left.  An empty file still contributes its
     (name, 0) pair, which is what today's ar.sizes.push_back(one.size())
     records for it. */
  uint feed_files() {
    for(;; ) {
      if( !in_f ) {
        if( in_i>=in_n )
          return 0;
        const char* p = in_paths[in_i++];
        in_f = fopen(p, "rb");
        if( !in_f ) {
          fprintf(stderr, "error: can't read '%s'\n", p);
          this->rc_err = 1;
          in_i = in_n;
          return 0;
        }
        if( in_n>1 ) {
          this->in_names.push_back(safe_name(string(p)));
          this->in_sizes.push_back(0);
        }
      }
      uint l = uint(fread(inpbuf, 1, bufsize, in_f));
      if( l ) {
        if( in_n>1 )
          this->in_sizes.back() += l;
        return l;
      }
      fclose(in_f);
      in_f = nullptr;
    }
  }

  void sink_split(const byte* p, size_t n) {
    while( n ) {
      if( !out_f ) {
        if( sp_i>=this->ar.names.size() )
          return; // the carrier already checked the table against the total
        string path = out_dir+safe_name(this->ar.names[sp_i]);
        out_f = fopen(path.c_str(), "wb");
        if( !out_f ) {
          fprintf(stderr, "error: can't write '%s'\n", path.c_str());
          this->rc_err = 1;
          return;
        }
        sp_left = this->ar.sizes[sp_i];
      }
      size_t k = u64(n)<sp_left ? n : size_t(sp_left);
      if( k )
        fwrite(p, 1, k, out_f);
      p += k;
      n -= k;
      sp_left -= k;
      if( !sp_left ) {
        fclose(out_f);
        out_f = nullptr;
        sp_i++;
      }
    }
  }

  /* Opened lazily, so a run that fails before producing an output byte (a bad
     magic, a crc mismatch) leaves no file behind -- the property the old
     "decode fully, then save_file" order had for free. */
  void sink(const byte* p, size_t n) {
    if( !n )
      return;
    if( out_mem ) {
      out_mem->insert(out_mem->end(), p, p+n);
      return;
    }
    if( out_split&&!this->ar.names.empty() ) {
      sink_split(p, n);
      return;
    }
    if( !out_f ) {
      out_f = fopen(out_path, "wb");
      if( !out_f ) {
        fprintf(stderr, "error: can't write '%s'\n", out_path);
        this->rc_err = 1;
        return;
      }
    }
    fwrite(p, 1, n, out_f);
  }

  uint run() {
    uint r = 0;
    this->coro_init();
    this->addout(outbuf, bufsize);
    for(;; ) {
      r = this->coro_call(this);
      if( r==1 ) {
        if( in_mem ) {
          size_t left = in_memn-in_mempos;
          uint k = left>size_t(bufsize) ? uint(bufsize) : uint(left);
          if( !k )
            this->f_quit = 1;
          this->addinp((byte*)in_mem+in_mempos, k);
          in_mempos += k;
        } else {
          uint l = feed_files();
          if( !l )
            this->f_quit = 1;
          this->addinp(inpbuf, l);
        }
      } else { // r==0 quit, r==2 pin1 full
        uint l = this->getoutsize();
        if( l )
          sink(outbuf, l);
        if( r!=2 )
          break;
        this->addout(outbuf, bufsize);
      }
    }
    if( in_f ) {
      fclose(in_f);
      in_f = nullptr;
    }
    if( out_f ) {
      fclose(out_f);
      out_f = nullptr;
    }
    return r;
  }
};

/* One carrier per direction, in BSS.  They are declared here, after the
   the generated MOD headers includes, so the mapping objects those headers instantiate are
   constructed first -- Codec's hj_bits / sgn_maxnb member initialisers read
   G0_* values that come from them. */
static XadDrive< Xad<0> > Menc;
static XadDrive< Xad<1> > Mdec;

// ------------------------------------------------------------------- reporting

template<int MD>
static void report(const Codec<MD> &cx, const Arc &ar, size_t insz, u64 outsz, u64 stk_hi) {
  size_t ncode = 0;
  for( size_t i = 0; i<ar.segs.size(); i++ ) {
    const Params &pm = ar.segs[i].pm;
    int spb = block_samples(pm);
    u32 nblk = u32((pm.data_size+size_t(pm.block_align)-1)/size_t(pm.block_align));
    if( spb>0 )
      ncode += size_t(nblk)*size_t(spb-(pm.format==FMT_MS ? 2 : 1))*size_t(pm.chans);
  }
  fprintf(stderr, " %zu -> %llu bytes, %.4f of original", insz, (unsigned long long)outsz, double(outsz)/double(insz));
  if( ncode )
    fprintf(stderr, ", %.3f bits/code", 8.0*double(outsz)/double(ncode));
  fprintf(stderr, "\n");
  if( verbose ) {
#if XAD_STATS
    static const char* cat[ST_NCAT] = {"codes  ", "headers", "literal", "flags  "};
    for( int i = 0; i<ST_NCAT; i++ )
      if( cx.stat[i] )
        fprintf(stderr, "  %s %9.1f bytes\n", cat[i], double(cx.stat[i])/(8.0*double(ST_SCALE)));
#else
    (void)cx;
    fprintf(stderr, "  per-stream accounting: rebuild with -DXAD_STATS=1\n");
#endif
    fprintf(stderr, "  coroutine stack high-water %llu of %u bytes\n",
            (unsigned long long)stk_hi, unsigned(Coroutine::STKPAD));
    fprintf(stderr, "  nlms kernels: %s\n", xad_isa());
    fprintf(stderr, "  model arenas: counters %.1f MB, match %.1f MB\n",
            double(cx.cta.used)/1048576.0, double(cx.mta.used)/1048576.0);
  }
}

// what a segment is, in one line
static void describe(const Params &pm) {
  fprintf(stderr, "   %s %d-bit, %d ch, %u Hz, block %d%s\n", pm.format==FMT_MS ? "MS-ADPCM " : "IMA-ADPCM", pm.bps, pm.chans, pm.rate, pm.block_align, pm.canonical ? "" : ", header stored");
}
