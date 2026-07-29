/* xad_front.inc -- part of xadpcm.cpp; not standalone.
 *
 * Xad<MD> -- the frontend coroutine.  RC<MD> is its base, so this object IS
 * the coroutine and the range coder is a layer of it. */

/* Xad<MD> IS the coroutine: it derives from RC<MD>, which derives from
   Coroutine, so there is one object, one stack, and the range coder is a layer
   of the chain rather than a second instance.  pin[0] carries the input window
   and pin[1] the output window; the RC's rcio defaults to whichever of the two
   the direction wants, so the coded stream never touches a staging buffer.

   The base is dependent on MD, so unqualified lookup does not see it -- hence
   the using-declarations rather than `this->` on every use, as in ddsdet. */
template<int MD>
struct Xad : RC<MD> {
  typedef RC<MD> RCB;
  using RCB::pin;
  using RCB::f_quit;
  using RCB::chkinp;
  using RCB::addinp;
  using RCB::addout;
  using RCB::coro_init;
  using RCB::coro_call;
  using RCB::getoutsize;
  using RCB::stkptrH;
  using RCB::emitted;
  using RCB::stk_hi;
  using RCB::stk_probe;

  Codec<MD> cx;
  Xad() : cx(*this) {}

  Arc ar;
  /* ENCODING, this is the container as it arrived, and it still has to be whole:
     nseg, the segment geometry and the crc32 all go in the header ahead of the
     first coded byte, and they come from a full scan.  That is a property of the
     format, not of the codec -- see PLAN-streaming.md §5.
     DECODING it is unused unless -b is given.  The decoder produces its output a
     window at a time through owin and pushes as it goes; -b restores the old
     order, where the whole image is rebuilt, verified, and only then written. */
  Buf<u8> src;
  NameTab in_names; // encode: one per input file, when there is more than one
  Buf<u64> in_sizes;
  /* The file whose name goes in a diagnostic.  A borrowed pointer, never owned:
     every assignment is an argv element, which outlives the whole run.  Starts
     at "" rather than null so a diagnostic before it is set prints nothing
     instead of stepping into printf's undefined behaviour. */
  const char* name = "";
  /* Scratch for one member name while the header is parsed.  A MEMBER, not a
     local: parse_header runs inside the coroutine, whose stack is 64 KB and is
     copied whole on every yield, and NAME_CAP is 4 KB of it.  Menc/Mdec are in
     BSS, so this costs address space and no page until a split archive uses it. */
  char nmbuf[NAME_CAP];

  /* ---------------------------------------------------------- output window
     Decoding used to materialise the whole output image in src, verify the crc
     over it, and only then push it.  It now produces the image a window at a
     time and pushes as it goes, so peak memory is flat in the size of the file
     rather than proportional to it.
     A MEMBER, like nmbuf and for the same reason: do_decode runs inside the
     coroutine, whose 64 KB stack is copied whole on every yield().  OUTWIN must
     be >= BA_MAX, because the payload is chunked in whole blocks (see dec_data)
     and one block can be BA_MAX bytes. */
  static const size_t OUTWIN = 1<<20;
  static_assert(OUTWIN>=size_t(BA_MAX), "the payload window must hold a whole block");
  u8 owin[OUTWIN];
  u32 rcrc = 0;   // rolling crc register, pre/post complement not applied
  u64 outn = 0;   // bytes produced, which src.size() used to report
  int buffered = 0; // -b: rebuild the whole image first, as before
  int solid = SOLID_OFF;
  int rc_err = 0;
  int out_bad = 0;   // decode failed after bytes were already pushed
  int hdr_eof = 0;
  u64 hdr_bytes = 0;       // container header, i.e. everything ahead of the rc stream

  /* How much coded audio this run will actually push through the model.  Only
     used to decide whether pre-faulting the match arena pays -- see
     Codec::PREFAULT_MIN. */
  size_t total_payload() const {
    size_t n = 0;
    for( size_t i = 0; i<ar.segs.size(); i++ )
      n += ar.segs[i].pm.data_size;
    return n;
  }

  // ------------------------------------------------------------- pin helpers

  /* Block write straight into the output window.  chkout() after each chunk
     keeps put()'s "ptr < end on entry" invariant, yielding r==2 to the driver
     exactly when the window fills. */
  void putn(cu8P p, size_t n) {
    for(;; ) {
      size_t room = size_t(pin[1].end-pin[1].ptr);
      size_t k = n<room ? n : room;
      if( k ) {
        memcpy(pin[1].ptr, p, k);
        pin[1].ptr += k;
        p += k;
        n -= k;
      }
      this->stk_probe();
      pin[1].chkout();
      if( !n )
        return;
    }
  }

  /* One window of decoded output: fold it into the rolling crc and push it.
     Under -b it goes into src instead and the old order -- whole image, verify,
     then write -- is restored exactly. */
  void emit(cu8P p, size_t n) {
    outn += n;
    if( buffered ) {
      src.append(p, n);
      return;
    }
    rcrc = crc32b_up(rcrc, p, n);
    putn(p, n);
  }

  /* A literal run of n bytes, produced a window at a time.  `prev` is carried
     across the windows: the order-1 context is the byte just coded, so letting
     it reset at a window boundary would decode a different stream from the one
     the encoder's single call produced. */
  void dec_lit(int stream, u64 n) {
    int prev = 0;
    while( n ) {
      size_t k = n<u64(OUTWIN) ? size_t(n) : size_t(OUTWIN);
      cx.code_bytes(stream, owin, k, prev);
      emit(owin, k);
      n -= k;
    }
  }

  /* n bytes of payload, in WHOLE BLOCKS.  code_data decomposes its argument as
     blocks of block_align with a possibly short last one, so chunking at a
     multiple of ba gives byte-for-byte the same decomposition as one call: every
     chunk but the last contains only full blocks, and the last chunk's remainder
     is the same remainder.  Everything the walk carries between blocks -- the
     ADPCM state, both NLMS cascades, the channel history -- lives in Codec and
     survives the call boundary. */
  void dec_data(u64 n, int ba) {
    size_t chunk = (size_t(OUTWIN)/size_t(ba))*size_t(ba);
    while( n ) {
      size_t k = n<u64(chunk) ? size_t(n) : chunk;
      cx.code_data(owin, k);
      emit(owin, k);
      n -= k;
    }
  }

  /* The whole container in, a window at a time.  f_quit is the driver's "no
     more input files"; chkinp() yields r==1 for the next window. */
  void slurp() {
    for(;; ) {
      coro3_pin &I = pin[0];
      if( I.ptr<I.end ) {
        src.append(I.ptr, size_t(I.end-I.ptr));
        I.ptr = I.end;
        continue;
      }
      if( f_quit )
        return;
      this->stk_probe();
      chkinp();
    }
  }

  void hput(u8 c) {
    pin[1].put(c);
    hdr_bytes++;
  }

  void hput_varint(u64 x) {
    while( x>=0x80 ) {
      hput(u8(u8(x&0x7F)|0x80));
      x >>= 7;
    }
    hput(u8(x));
  }

  int hget() {
    uint c = pin[0].get();
    if( c>255 ) {
      hdr_eof = 1;
      return -1;
    }
    return int(c);
  }

  u64 hget_varint() {
    u64 v = 0;
    int sh = 0;
    for(;; ) {
      int c = hget();
      if( c<0||sh>63 ) {
        hdr_eof = 1;
        return 0;
      }
      v |= u64(u32(c)&0x7F)<<sh;
      if( !(c&0x80) )
        return v;
      sh += 7;
    }
  }

  int fail(const char* what) {
    fprintf(stderr, "error: '%s': %s\n", name, what);
    rc_err = 1;
    return 0;
  }

  // ------------------------------------------------------- segment table i/o

  void put_seg(const Params &pm) {
    hput(u8(pm.canonical));
    hput(u8(pm.format));
    hput(u8(pm.chans));
    hput(u8(pm.bps));
    hput_varint(u64(pm.block_align));
    hput_varint(u64(pm.data_size));
    hput_varint(u64(pm.tail_size));
    if( pm.canonical ) {
      hput_varint(pm.rate);
      hput_varint(zig(pm.factdiff));
    } else
      hput_varint(u64(pm.head_size));
    if( pm.format==FMT_MS ) {
      // the usual seven costs a zero byte; anything else is spelled out, since
      // both sides have to predict with the same table
      bool std = pm.ncoef==7;
      for( int i = 0; std&&i<7; i++ )
        std = pm.coef[i][0]==ms_std_coef[i][0]&&pm.coef[i][1]==ms_std_coef[i][1];
      hput(u8(std ? 0 : pm.ncoef));
      if( !std )
        for( int i = 0; i<pm.ncoef; i++ ) {
          hput_varint(zig(pm.coef[i][0]));
          hput_varint(zig(pm.coef[i][1]));
        }
    }
  }

  bool get_seg(Params &pm) {
    pm.canonical = hget();
    pm.format = hget();
    pm.chans = hget();
    pm.bps = hget();
    if( hdr_eof )
      return fail("truncated segment table"), false;
    pm.block_align = int(hget_varint());
    pm.data_size = size_t(hget_varint());
    pm.tail_size = size_t(hget_varint());
    if( pm.canonical ) {
      pm.rate = u32(hget_varint());
      pm.factdiff = unzig(hget_varint());
    } else
      pm.head_size = size_t(hget_varint());
    if( pm.format==FMT_MS ) {
      int nc = hget();
      if( hdr_eof )
        return fail("truncated segment table"), false;
      pm.ncoef = nc;
      if( !pm.ncoef ) {
        pm.ncoef = 7;
        for( int i = 0; i<7; i++ ) {
          pm.coef[i][0] = ms_std_coef[i][0];
          pm.coef[i][1] = ms_std_coef[i][1];
        }
      } else if( pm.ncoef>MS_MAXCOEF )
        return fail("bad coefficient count"), false;
      else
        for( int i = 0; i<pm.ncoef; i++ ) {
          pm.coef[i][0] = i32(unzig(hget_varint()));
          pm.coef[i][1] = i32(unzig(hget_varint()));
        }
    }
    if( hdr_eof )
      return fail("truncated segment table"), false;
    if( pm.format!=FMT_IMA&&pm.format!=FMT_MS )
      return fail("unknown payload format"), false;
    if( pm.format==FMT_MS ? (pm.chans<1||pm.chans>2||pm.bps!=4||pm.block_align<7*pm.chans) : (pm.chans<1||pm.chans>MAX_CHANS||pm.bps<2||pm.bps>5||pm.block_align<pm.chans*4) )
      return fail("bad parameters"), false;
    /* Upper bound on block_align, and it is load-bearing rather than tidy.  It
       arrives as a varint from the archive while the payload walks size their
       scratch at BA_MAX, so a header claiming block_align = 1000000 memsets a
       megabyte into a 64 KB slice.  Verified reachable: an 88-byte hand-built
       archive tripped _FORTIFY_SOURCE, and without fortify it is a silent BSS
       overwrite.  BA_MAX is not a policy limit -- an encoder cannot produce a
       larger one, because block_align comes out of the fmt chunk's u16 -- so
       nothing legal is being rejected here.
       This became reachable when the walks moved from vectors sized at `ba` to
       fixed slabs; before that an absurd block_align was merely an absurd
       allocation.  A bound the format always implied and never checked. */
    if( pm.block_align>BA_MAX )
      return fail("block size out of range"), false;

    int spb = block_samples(pm);
    if( pm.canonical ) {
      if( spb<=0 )
        return fail("bad block size"), false;
      pm.spb = spb;
      pm.head_size = pm.format==FMT_MS ? size_t(canon_head_ms(pm.ncoef)) : size_t(CANON_HEAD);
    }
    pm.orig_size = pm.head_size+pm.data_size+pm.tail_size;
    return true;
  }

  // ------------------------------------------------------------------ encode

  void do_encode() {
    slurp();
    if( rc_err ) // the driver could not open one of the inputs
      return;
    if( src.empty() ) {
      fprintf(stderr, "error: nothing to compress\n");
      rc_err = 1;
      return;
    }
    ar.solid = solid;
    /* Moved, not copied: the driver filled these while pumping and has no use
       for them afterwards, and Buf is non-copyable so the intent has to be
       spelled out rather than happening by accident. */
    ar.names.steal(in_names);
    ar.sizes.steal(in_sizes);
    scan_segments(src, ar);
    if( ar.segs.empty() ) {
      // say why, using the first input's own diagnostics
      Params pm;
      Wav w;
      analyze(src.data(), src.size(), name, pm, w, true);
      rc_err = 1;
      return;
    }
    ar.crc = crc32b(src.data(), src.size());

    for( int i = 0; i<4; i++ )
      hput(u8(MAGIC[i]));
    hput(VERSION);
    hput(u8(ar.solid));
    hput_varint(u64(ar.segs.size()));
    hput_varint(u64(ar.lead));
    for( size_t i = 0; i<ar.segs.size(); i++ )
      put_seg(ar.segs[i].pm);
    hput_varint(u64(ar.names.size())); // 0 = one unnamed stream
    for( size_t i = 0; i<ar.names.size(); i++ ) {
      u32 ln = ar.names.len_at(i);
      const char* nm = ar.names.at(i);
      hput_varint(u64(ln));
      for( u32 j = 0; j<ln; j++ )
        hput(u8(nm[j]));
      hput_varint(ar.sizes[i]);
    }
    u8 c4[4];
    wr32(c4, ar.crc);
    for( int i = 0; i<4; i++ )
      hput(c4[i]);

    u8* d = src.data();
    int maxsym, maxnib;
    arc_geometry(ar, maxsym, maxnib);
    Params p0;
    cx.init(ar.segs.empty() ? p0 : ar.segs[0].pm, maxsym, maxnib, u64(total_payload()));
    this->init_enc();
    if( ar.lead )
      cx.code_bytes(LIT_GAP, d, ar.lead);
    for( size_t i = 0; i<ar.segs.size(); i++ ) {
      const Params &pm = ar.segs[i].pm;
      u8* q = d+ar.segs[i].off;
      bool keep = i&&(ar.solid==SOLID_ALL||(ar.solid==SOLID_SAME&&same_type(ar.segs[i-1].pm, pm)));
      if( i&&!keep )
        cx.reset_stats();
      cx.begin_seg(pm, !keep);
      if( !pm.canonical )
        cx.code_bytes(LIT_HEAD, q, pm.head_size);
      cx.code_data(q+pm.head_size, pm.data_size);
      cx.code_bytes(LIT_TAIL, q+pm.head_size+pm.data_size, pm.tail_size);
    }
    this->flush();
  }

  // ------------------------------------------------------------------ decode

  void do_decode() {
    u8 m[4];
    for( int i = 0; i<4; i++ ) {
      int c = hget();
      if( c<0 ) {
        fprintf(stderr, "error: '%s' is not an xadpcm file\n", name);
        rc_err = 1;
        return;
      }
      m[i] = u8(c);
    }
    if( memcmp(m, MAGIC, 4) ) {
      fprintf(stderr, "error: '%s' is not an xadpcm file\n", name);
      rc_err = 1;
      return;
    }
    if( hget()!=VERSION )
      return (void)fail("made by a different xadpcm version");
    int sl = hget();
    if( sl<0||sl>SOLID_ALL )
      return (void)fail("unknown solid mode");
    ar.solid = sl;
    u64 nseg = hget_varint();
    ar.lead = size_t(hget_varint());
    if( hdr_eof||nseg>(1u<<24) )
      return (void)fail("truncated or implausible header");
    /* One at a time rather than resize(nseg) then fill.  nseg is a varint out of
       the header and the cap above only says 2^24, so the old shape asked for
       5.4 GB (2^24 * sizeof(Seg)) before reading a single segment, from a header
       forty bytes long -- and then failed on the first get_seg for want of input.
       Growing as they parse costs one 1.5x realloc chain and bounds the request
       by what the archive actually contains.  On a well-formed archive the two
       are identical, including that a failure leaves rc_err set and every caller
       returning before it looks at ar.segs. */
    for( u64 i = 0; i<nseg; i++ ) {
      Seg sg;
      if( !get_seg(sg.pm) )
        return;
      ar.segs.push_back(sg);
    }
    u64 nfile = hget_varint();
    if( hdr_eof||nfile>(1u<<20) )
      return (void)fail("truncated header");
    for( u64 i = 0; i<nfile; i++ ) {
      u64 ln = hget_varint();
      if( hdr_eof||ln>NAME_CAP )
        return (void)fail("bad file table");
      for( u64 j = 0; j<ln; j++ ) {
        int c = hget();
        if( c<0 )
          return (void)fail("bad file table");
        nmbuf[size_t(j)] = char(c);
      }
      ar.names.push_back(nmbuf, size_t(ln));
      ar.sizes.push_back(hget_varint());
    }
    u8 c4[4];
    for( int i = 0; i<4; i++ ) {
      int c = hget();
      if( c<0 )
        return (void)fail("truncated header");
      c4[i] = u8(c);
    }
    ar.crc = rd32(c4);

    size_t total = ar.lead;
    for( size_t i = 0; i<ar.segs.size(); i++ )
      total += ar.segs[i].pm.orig_size;
    ar.total = total;
    /* The file table has to fit inside what the segment table says the stream
       is.  Checked here, before a byte is decoded, rather than while writing
       members out -- the driver's split sink is downstream of this. */
    u64 named = 0;
    for( size_t i = 0; i<ar.sizes.size(); i++ )
      named += ar.sizes[i];
    if( named>u64(total) )
      return (void)fail("file table overruns the stream");
    int maxsym, maxnib;
    arc_geometry(ar, maxsym, maxnib);
    Params p0;
    cx.init(ar.segs.empty() ? p0 : ar.segs[0].pm, maxsym, maxnib, u64(total_payload()));
    this->init_dec();

    rcrc = crc32b_init();
    outn = 0;
    if( ar.lead )
      dec_lit(LIT_GAP, ar.lead);
    size_t at = ar.lead;
    for( size_t i = 0; i<ar.segs.size(); i++ ) {
      Params &pm = ar.segs[i].pm;
      ar.segs[i].off = at;
      bool keep = i&&(ar.solid==SOLID_ALL||(ar.solid==SOLID_SAME&&same_type(ar.segs[i-1].pm, pm)));
      if( i&&!keep )
        cx.reset_stats();
      cx.begin_seg(pm, !keep);
      if( pm.canonical ) {
        /* A regenerated header is a few dozen bytes -- CANON_HEAD, or
           canon_head_ms(ncoef) with ncoef <= MS_MAXCOEF -- so it fits the window
           with room to spare.  Checked rather than assumed, because head_size is
           set from the archive for the non-canonical case just below. */
        if( pm.head_size>size_t(OUTWIN) )
          return (void)fail("canonical header too large");
        write_canon(owin, pm);
        emit(owin, pm.head_size);
      } else
        dec_lit(LIT_HEAD, pm.head_size);
      dec_data(pm.data_size, pm.block_align);
      dec_lit(LIT_TAIL, pm.tail_size);
      at += pm.orig_size;
    }

    /* Under -b nothing has been written yet, so the old order holds: verify,
       then push.  Streaming, the bytes are already downstream, so a mismatch
       sets out_bad and the driver removes what it wrote -- see
       XadDrive::discard_output().  For a pipe there is nothing to remove and the
       non-zero exit is all the caller gets; the usage text says so. */
    if( buffered ) {
      if( crc32b(src.data(), src.size())!=ar.crc )
        return (void)fail("crc mismatch after decode");
      putn(src.data(), src.size());
      return;
    }
    if( crc32b_fin(rcrc)!=ar.crc ) {
      out_bad = 1;
      return (void)fail("crc mismatch after decode");
    }
  }

  void do_process() {
    if constexpr( MD==0 )
      do_encode();
    else
      do_decode();
    yield(this, 0);
  }
};
