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
      /* The length came out of the archive and the range decoder keeps producing
         bytes after its input runs out, so a corrupted varint would otherwise
         write until the disk filled.  One window of slop past the end. */
      if( rc_starved() )
        return (void)fail("truncated archive");
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
      if( rc_starved() )
        return (void)fail("truncated archive");
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

  /* Before the rc stream starts these write raw bytes to the pin, which is what
     the five-byte preamble (magic, version, solid) needs.  Inside it they code
     through the literal model on its own stream, so put_seg/get_seg -- and the
     file table, and the crc -- emit exactly the bytes they always did without
     knowing where they ended up.  `dprev` is the order-1 context and is carried
     across records on purpose: descriptor bytes resemble each other far more
     than they resemble the payload. */
  int in_rc = 0;
  int dprev = 0;

  void hput(u8 c) {
    if( in_rc ) {
      cx.code_byte(LIT_DESC, dprev, c);
      dprev = c;
      return;
    }
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
    if( in_rc ) {
      int v = cx.code_byte(LIT_DESC, dprev, 0);
      dprev = v;
      return v;
    }
    uint c = pin[0].get();
    if( c>255 ) {
      hdr_eof = 1;
      return -1;
    }
    return int(c);
  }

  /* A truncated v5 archive has no length to run out of -- the range decoder
     happily produces bytes past the end of its input -- so the record loop needs
     its own stop.  This is it: the driver has said there is no more input and
     the window is drained. */
  bool rc_starved() const { return f_quit&&pin[0].ptr>=pin[0].end; }

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
    /* A failed run leaves nothing behind, and after the streaming rewrite that
       has to be said here rather than only at the crc check: a corrupted rc
       stream can decode a garbage record tag hundreds of megabytes in, by which
       point the output file exists and is wrong.  Every failure path now takes
       the same cleanup as a crc mismatch.  Harmless when nothing was written --
       discard_output removes a file that is not there and moves on. */
    out_bad = 1;
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

  /* ------------------------------------------------------- streaming encode
   *
   * The input arrives a window at a time and leaves as records.  Nothing is held
   * beyond the window: the scan looks for a RIFF/WAVE that also parses as ADPCM,
   * everything before one is a T_LIT run, a wav is a T_SEG followed by its head
   * and a chunk list, and T_END carries the file table and the crc that has been
   * rolling all along.
   *
   * What made this possible is the chunk list.  A segment's record used to have
   * to state a tail_size measured to the NEXT wav's start; now the data is a
   * sequence of (len, bytes) with a zero terminator, so the encoder commits to
   * nothing it has not already read, and a truncated final wav -- fewer bytes
   * present than the data chunk declares -- is just a short last chunk instead of
   * a special case.
   *
   * EWIN must hold a whole block AND the largest RIFF header the scan will parse.
   * A header longer than the window is treated as not-a-wav and rides along as
   * literal data: a ratio loss on a pathological file, never a correctness one. */
  static const size_t EWIN = 1<<20;
  static_assert(EWIN>=size_t(BA_MAX)*2, "the scan window must hold a block and a header");
  u8 ewin[EWIN];
  size_t ew_n = 0, ew_pos = 0;
  u64 in_total = 0;   // set by the driver when the inputs are seekable; 0 = unknown
  u64 inn = 0;        // input bytes consumed, which src.size() used to report

  /* Consume n bytes at the cursor: they are now the crc's, and they are input the
     report can count.  Every path that advances ew_pos goes through here, so the
     two cannot drift apart. */
  void ew_take(size_t n) {
    rcrc = crc32b_up(rcrc, ewin+ew_pos, n);
    ew_pos += n;
    inn += n;
  }

  /* Slide the window forward and top it up.  Returns what is readable at ew_pos.
     `keep` is what must remain addressable behind the cursor -- nothing here, the
     cursor only moves over bytes already coded. */
  size_t ew_fill() {
    if( ew_pos ) {
      memmove(ewin, ewin+ew_pos, ew_n-ew_pos);
      ew_n -= ew_pos;
      ew_pos = 0;
    }
    while( ew_n<EWIN ) {
      coro3_pin &I = pin[0];
      if( I.ptr<I.end ) {
        size_t k = size_t(I.end-I.ptr), room = EWIN-ew_n;
        if( k>room )
          k = room;
        memcpy(ewin+ew_n, I.ptr, k);
        I.ptr += k;
        ew_n += k;
        continue;
      }
      if( f_quit )
        break;
      this->stk_probe();
      chkinp();
    }
    return ew_n-ew_pos;
  }

  /* "Nothing more will arrive."  ew_fill() only returns short of a full window
     once the driver has said so, so this is exactly f_quit -- NOT
     `f_quit && window empty`, which was the first version and silently dropped
     the held-back tail: at end of input the window still holds those bytes, so
     the predicate read false, the 11 were kept back a second time, and the loop
     took its `nothing left` exit with them still unwritten. */
  bool in_done() const { return f_quit!=0; }

  // one T_LIT record of n bytes starting at ew_pos, consumed from the window
  void enc_lit(int stream, size_t n) {
    if( !n )
      return;
    hput(u8(T_LIT));
    hput_varint(u64(n));
    int prev = 0;
    cx.code_bytes(stream, ewin+ew_pos, n, prev);
    ew_take(n);
  }

  void do_encode() {
    if( ew_fill()==0 ) {
      if( rc_err ) // the driver could not open one of the inputs
        return;
      fprintf(stderr, "error: nothing to compress\n");
      rc_err = 1;
      return;
    }
    ar.solid = solid;

    for( int i = 0; i<4; i++ )
      hput(u8(MAGIC[i]));
    hput(VERSION);
    hput(u8(solid));
    /* The member table rides in the PREAMBLE, outside the rc stream.  It is the
       one piece of container metadata a reader needs before the first output
       byte -- the split sink cuts the decoded stream on it -- so it cannot live
       in the trailer with the crc.  It is known up front because the driver
       seeks every input before the coroutine starts. */
    ar.names.steal(in_names);
    ar.sizes.steal(in_sizes);
    hput_varint(u64(ar.names.size())); // 0 = one unnamed stream
    for( size_t i = 0; i<ar.names.size(); i++ ) {
      u32 ln = ar.names.len_at(i);
      const char* nm = ar.names.at(i);
      hput_varint(u64(ln));
      for( u32 j = 0; j<ln; j++ )
        hput(u8(nm[j]));
      hput_varint(ar.sizes[i]);
    }

    /* Sized at the format maximum, because a streaming encoder cannot know what
       geometries are still coming.  That costs nothing: the row indices use the
       SEGMENT's nsym and nib_rows, set in begin_seg, while these only size the
       arena -- forcing the maximum on the v4 encoder left all 24 archives
       byte-identical, measured before this was written. */
    Params p0;
    cx.init(p0, 16, 1, in_total);
    this->init_enc();
    in_rc = 1;
    rcrc = crc32b_init();

    size_t nseg = 0;
    Params prev_pm;
    for(;; ) {
      size_t avail = ew_fill();
      if( !avail )
        break;

      /* Find the next candidate.  A RIFF that straddles the end of a window
         cannot be judged yet, so unless this is the last window we stop short of
         it and let the next fill bring the rest -- the window slides, so the
         candidate is whole next time round. */
      size_t lit = 0, stop = avail;
      Params pm;
      Wav w;
      bool got = false;
      /* lit+12 <= avail, not lit < avail.  Judging a candidate reads bytes 0..3
         and 8..11 of it, so anything in the last eleven cannot be looked at --
         and the old bound let memchr find an 'R' there and then memcmp read past
         the valid bytes.  Inside a 1 MB BSS array, so it never faulted; it read
         indeterminate bytes and could in principle have invented a wav. */
      while( lit+12<=avail ) {
        const u8* base = ewin+ew_pos+lit;
        size_t left = avail-lit;
        const void* hit = memchr(base, 'R', left-11);
        if( !hit )
          break;
        lit = size_t((const u8*)hit-(ewin+ew_pos));
        base = ewin+ew_pos+lit;
        left = avail-lit;
        if( !memcmp(base, "RIFF", 4)&&!memcmp(base+8, "WAVE", 4) ) {
          Params t;
          Wav tw;
          /* extent = head + the DECLARED data size, so tail_size comes out zero
             and the bytes after the wav become their own T_LIT record.  That is
             the change that removes the lookahead. */
          if( parse_wav(base, left, tw)&&tw.data_off+tw.data_decl>=tw.data_off ) {
            size_t ext = tw.data_off+tw.data_decl;
            if( analyze(base, ext, "", t, tw, false)&&t.block_align<=BA_MAX
                &&t.head_size<=left ) {
              pm = t;
              w = tw;
              got = true;
              break;
            }
          }
          /* A RIFF/WAVE that did not parse, with the window ending behind it.
             It may be a real wav whose header simply runs past the end of what
             has been read -- so stop the literal run HERE and refill; the window
             slides, the candidate lands at offset 0, and next time round it gets
             the whole window to be judged in.  Without this the literal run ran
             straight over it and the wav was lost: measured, a wav placed 36
             bytes before the window edge was missed entirely and its archive
             came out 24% larger.  At lit == 0 a refill would change nothing, so
             that case is judged on what is there and moves on. */
          if( lit&&!in_done() ) {
            stop = lit;
            break;
          }
        }
        lit++;
      }

      if( !got ) {
        /* How much of this window is certainly literal.  Either up to a
           candidate that could not be judged yet, or all of it bar the last
           eleven bytes, which might be the front of a RIFF the next window
           completes.  Zero means neither -- wait for more input; ew_fill only
           returns short of a full window once the driver has said there is none,
           so this cannot spin. */
        size_t n = stop<avail ? stop : (in_done() ? avail : (avail>=11 ? avail-11 : 0));
        if( !n )
          continue;
        enc_lit(nseg ? LIT_TAIL : LIT_GAP, n);
        continue;
      }
      if( lit ) {
        enc_lit(nseg ? LIT_TAIL : LIT_GAP, lit);
        continue;
      }

      /* ---- a wav starts here.  The record goes out BEFORE reset_stats and
         begin_seg, and that order is load-bearing: the decoder cannot reset
         anything until it has read the record that tells it to, so an encoder
         that resets first codes the descriptor against fresh counters while the
         decoder reads it against the previous segment's.  It decodes the first
         segment perfectly and then falls over on the second, which is exactly
         how this was found. */
      hput(u8(T_SEG));
      put_seg(pm);
      bool wider = cx.grow_geom(pm);
      bool keep = nseg&&!wider&&(solid==SOLID_ALL||(solid==SOLID_SAME&&same_type(prev_pm, pm)));
      if( nseg&&!keep&&!wider )
        cx.reset_stats();
      cx.begin_seg(pm, !keep);
      if( !nseg )
        ar.lead = size_t(inn); // report only: bytes ahead of the first wav
      if( !pm.canonical )
        cx.code_bytes(LIT_HEAD, ewin+ew_pos, pm.head_size);
      ew_take(pm.head_size);

      /* The chunk list.  Whole blocks per chunk except the last, so the walk
         decomposes exactly as one call over the same bytes would.  Stops at the
         declared size, or at end of input if the file is short. */
      size_t want = w.data_decl;
      size_t chunk_cap = (EWIN/size_t(pm.block_align))*size_t(pm.block_align);
      size_t coded = 0;
      while( want ) {
        size_t have = ew_fill();
        if( !have )
          break;
        size_t k = want<have ? want : have;
        if( k>chunk_cap )
          k = chunk_cap;
        /* Only the final chunk may be short of a block; otherwise trim back so
           the block grid is preserved across the chunk boundary. */
        if( k<want&&k>size_t(pm.block_align) )
          k -= k%size_t(pm.block_align);
        hput_varint(u64(k));
        cx.code_data(ewin+ew_pos, k);
        ew_take(k);
        want -= k;
        coded += k;
      }
      hput_varint(0); // end of chunk list
      pm.data_size = coded;
      pm.tail_size = 0;
      pm.orig_size = pm.head_size+coded;
      prev_pm = pm;
      nseg++;
      Seg sg;
      sg.pm = pm;
      ar.segs.push_back(sg); // for the -v report only; one Params per wav
    }

    hput(u8(T_END));
    ar.crc = crc32b_fin(rcrc);
    u8 c4[4];
    wr32(c4, ar.crc);
    for( int i = 0; i<4; i++ )
      hput(c4[i]);
    this->flush();
  }

  // ------------------------------------------------------------------ decode

  /* ------------------------------------------------------- streaming decode
   *
   * The mirror of do_encode: read records, produce the stream a window at a time,
   * push as it goes.  Neither side ever holds more than a window.
   *
   * Because there is no table of contents any more, the geometry has to be sized
   * at the format maximum here too -- and for the same measured reason it costs
   * nothing.  It also means a truncated archive has no length to run out of: the
   * range decoder will produce bytes forever, so the record loop stops on
   * rc_starved() and on a record count that cannot be legitimate. */
  void do_decode() {
    u8 m[4];
    for( int i = 0; i<4; i++ ) {
      int c = hget();
      if( c<0 )
        return (void)fail("not an xadpcm file");
      m[i] = u8(c);
    }
    if( memcmp(m, MAGIC, 4) )
      return (void)fail("not an xadpcm file");
    int ver = hget();
    if( ver!=VERSION )
      return (void)fail(ver==4 ? "made by xadpcm rev 4, whose container is not streamable"
                               : "made by a different xadpcm version");
    int sl = hget();
    if( hdr_eof||sl<0||sl>SOLID_ALL )
      return (void)fail("truncated or implausible header");
    ar.solid = sl;
    u64 nfile = hget_varint();
    if( hdr_eof||nfile>(1u<<20) )
      return (void)fail("truncated or implausible header");
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
    if( hdr_eof )
      return (void)fail("truncated header");

    Params p0;
    cx.init(p0, 16, 1, in_total);
    this->init_dec();
    in_rc = 1;

    rcrc = crc32b_init();
    outn = 0;
    size_t nseg = 0;
    Params prev_pm;
    for(;; ) {
      if( rc_starved() )
        return (void)fail("truncated archive");
      int tag = hget();
      if( tag==T_END )
        break;
      if( tag==T_LIT ) {
        u64 n = hget_varint();
        dec_lit(nseg ? LIT_TAIL : LIT_GAP, n);
        continue;
      }
      if( tag!=T_SEG )
        return (void)fail("unknown container record");
      if( nseg>(1u<<24) )
        return (void)fail("implausible segment count");
      Params pm;
      if( !get_seg(pm) )
        return;
      bool wider = cx.grow_geom(pm);
      bool keep = nseg&&!wider&&(ar.solid==SOLID_ALL||(ar.solid==SOLID_SAME&&same_type(prev_pm, pm)));
      if( nseg&&!keep&&!wider )
        cx.reset_stats();
      cx.begin_seg(pm, !keep);
      if( pm.canonical ) {
        /* A regenerated header is a few dozen bytes -- CANON_HEAD, or
           canon_head_ms(ncoef) with ncoef <= MS_MAXCOEF -- so it fits the window
           with room to spare.  Checked rather than assumed, because head_size is
           set from the archive for the non-canonical case just below. */
        if( pm.head_size>OUTWIN )
          return (void)fail("canonical header too large");
        write_canon(owin, pm);
        emit(owin, pm.head_size);
      } else
        dec_lit(LIT_HEAD, pm.head_size);
      size_t coded = 0;
      for(;; ) {
        if( rc_starved() )
          return (void)fail("truncated archive");
        u64 k = hget_varint();
        if( !k )
          break;
        if( k>u64(pm.data_size)-u64(coded) )
          return (void)fail("chunk list overruns the declared data size");
        dec_data(k, pm.block_align);
        coded += size_t(k);
      }
      pm.data_size = coded;
      pm.orig_size = pm.head_size+coded;
      prev_pm = pm;
      nseg++;
      Seg sg;
      sg.pm = pm;
      ar.segs.push_back(sg); // report only
    }

    u8 c4[4];
    for( int i = 0; i<4; i++ )
      c4[i] = u8(hget());
    ar.crc = rd32(c4);

    /* The file table has to fit inside what was actually produced.  In v4 that
       was checkable before a byte was decoded; the sizes are still in the
       preamble, but `outn` is only final now, so the comparison happens here
       alongside the crc and takes the same failure path. */
    u64 named = 0;
    for( size_t i = 0; i<ar.sizes.size(); i++ )
      named += ar.sizes[i];
    if( named>outn ) {
      out_bad = 1;
      return (void)fail("file table overruns the stream");
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
