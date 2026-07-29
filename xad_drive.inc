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
  const char* in_path = nullptr; // the one in_f came from, so it can be closed right
  FILE* in_f = nullptr;
  const u8* in_mem = nullptr;
  size_t in_memn = 0, in_mempos = 0;

  // output: at most one of these is set
  const char* out_path = nullptr;
  char out_dir[PATH_CAP] = {}; // out_path plus a trailing separator, for the split sink
  char sp_path[PATH_CAP];      // out_dir + the current member name
  Buf<u8>* out_mem = nullptr;
  int out_split = 0; // out_path names a directory when the file table is non-empty
  FILE* out_f = nullptr;
  size_t sp_i = 0;
  u64 sp_left = 0;

  /* The member table, built BEFORE the coroutine starts.  It used to accumulate
     as the pump ran, which was fine when it was written into a trailer -- but the
     split sink needs it before the first output byte, so it has to be known
     before the first input byte.  Every name is argv and every size is a seek, so
     it is; a pipe reports 0 and multi-input from pipes is not a thing anyway,
     since several pipes cannot be told apart.
     Also totals the inputs, which is what decides whether pre-faulting the match
     arena pays -- see Codec::PREFAULT_MIN.  0 means "unknown", i.e. don't. */
  void scan_inputs() {
    for( int i = 0; i<in_n; i++ ) {
      u64 sz = 0;
      if( !is_std(in_paths[i]) ) {
        FILE* f = fopen(in_paths[i], "rb");
        if( f ) {
          if( !fseek(f, 0, SEEK_END) ) {
            long long t = ftell(f);
            if( t>0 )
              sz = u64(t);
          }
          fclose(f);
        }
      }
      this->in_total += sz;
      if( in_n>1 ) {
        this->in_names.push_back(safe_name(in_paths[i]));
        this->in_sizes.push_back(sz);
      }
    }
  }

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
        in_path = p;
        in_f = xfopen(p, false);
        if( !in_f ) {
          fprintf(stderr, "error: can't read '%s'\n", p);
          this->rc_err = 1;
          in_i = in_n;
          return 0;
        }

      }
      uint l = uint(fread(inpbuf, 1, bufsize, in_f));
      if( l )
        return l;
      xfclose(in_path, in_f);
      in_f = nullptr;
    }
  }

  void sink_split(cu8P p, size_t n) {
    while( n ) {
      if( !out_f ) {
        if( sp_i>=this->ar.names.size() )
          return; // the carrier already checked the table against the total
        /* Truncation is reported, not absorbed: a member silently written to a
           shortened path is a wrong file, not a smaller one. */
        if( !path_join(sp_path, sizeof(sp_path), out_dir, safe_name(this->ar.names.at(sp_i))) ) {
          fprintf(stderr, "error: output path for member %zu is too long\n", sp_i);
          this->rc_err = 1;
          return;
        }
        out_f = fopen(sp_path, "wb");
        if( !out_f ) {
          fprintf(stderr, "error: can't write '%s'\n", sp_path);
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

  /* Everything this run has written, removed.  The decoder verifies the crc
     after the last byte rather than before the first, so "no bad output
     survives" is now kept by taking the output back instead of by withholding
     it.  Split output removes every member opened this run, not just the one in
     progress -- an archive that fails at member three has already closed one and
     two, and leaving them behind would be worse than leaving nothing.
     A pipe cannot be taken back.  That is the one case where the property is
     genuinely lost, the usage text says so, and -b is the way to keep it. */
  void discard_output() {
    if( out_f ) {
      xfclose(out_path, out_f);
      out_f = nullptr;
    }
    /* Nothing on disk to take back: a memory sink is the caller's to discard,
       and bytes already down a pipe are gone.  The exit status is the only
       signal in the second case, which is what -b exists for. */
    if( out_mem||!out_path||is_std(out_path) )
      return;
    if( out_split&&!this->ar.names.empty() ) {
      size_t last = sp_i<this->ar.names.size() ? sp_i : this->ar.names.size()-1;
      for( size_t i = 0; i<=last; i++ )
        if( path_join(sp_path, sizeof(sp_path), out_dir, safe_name(this->ar.names.at(i))) )
          remove(sp_path);
    } else
      remove(out_path);
  }

  /* Opened lazily, so a run that fails before producing an output byte (a bad
     magic, a truncated header) leaves no file behind. */
  void sink(cu8P p, size_t n) {
    if( !n )
      return;
    if( out_mem ) {
      out_mem->append(p, n);
      return;
    }
    if( out_split&&!this->ar.names.empty() ) {
      sink_split(p, n);
      return;
    }
    if( !out_f ) {
      out_f = xfopen(out_path, true);
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
      xfclose(in_path, in_f);
      in_f = nullptr;
    }
    if( this->out_bad )
      discard_output();
    if( out_f ) {
      xfclose(out_path, out_f);
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
