/* ddsdet_carve.inc -- the carver/restorer coroutine; part of ddsdet.cpp.
 *
 * ddsdet<mode> derives from d5c::RC<mode>, which derives from Coroutine, so
 * this object IS the coroutine and the range coder is a layer of it.  Its own
 * body is further assembled from dds_header.inc and coro_getput.inc. */

/* Templated on direction so the codec's `enc` folds: mode 0 = carve/encode
 * (ops c, cs), mode 1 = restore/decode (ops d, ds).  Member functions of a
 * class template are instantiated only when used, so ddsdet<1> carries no
 * encoder and ddsdet<0> no decoder -- the split needs no #ifdefs. */
template<int mode>
struct ddsdet : d5c::RC<mode> {
  /* RC<mode> is a Coroutine, so this IS the coroutine -- one object, one
   * stack, with the range coder as a layer in the chain rather than a second
   * instance.  The carver's container I/O is the inherited get()/put() on
   * pins 0/1 exactly as before; the coded stream gets its own pin where a
   * second stream exists (cs: pin2 out, two-file ds: pin3 in).
   *
   * The base is dependent on `mode`, so unqualified lookup does not see it --
   * hence the using-declarations rather than `this->` on every use. */
  typedef d5c::RC<mode> RCB;
  using RCB::pin; using RCB::f_quit; using RCB::state;
  using RCB::inpptr; using RCB::inpbeg; using RCB::inpend;
  using RCB::outptr; using RCB::outbeg; using RCB::outend;
  using RCB::get; using RCB::put; using RCB::get0; using RCB::put0;
  using RCB::chkinp; using RCB::chkout;
  using RCB::addinp; using RCB::addout;
  using RCB::getinplen; using RCB::getoutlen; using RCB::getinpsize; using RCB::getoutsize;
  using RCB::coro_init; using RCB::coro_call;

  #include "dds_header.inc"

  #include "coro_getput.inc"

  DDSHEADER hdr;

  d5c::Solid<mode>* sol;   // non-null in solid modes
  uint why;           // --why: report why each "DDS " candidate was rejected
  uint bmp;           // --bmp: endpoints to numbered .bmp instead of the coder
  const char* bprefix;
  const char* whyrej; // reason from the most recent failed scan_dds()
  const char* solpath;
  uint op;            // 0 = carve, 1 = restore, 2 = carve solid, 3 = restore solid
  uint zero;          // zero-pad placeholders to original length
  char prefix[4096];
  uint zindex;        // running carve index / restore count
  qword src_pos;      // distinct source bytes pulled from the coroutine
  uint co_eof;        // 1 once the underlying coroutine is truly exhausted
  int  rc;            // nonzero -> fatal error

  // pushback (LIFO) on top of the coroutine get()
  byte* pbuf; uint pbn, pbcap;
  // per-texture capture buffer (raw consumed source bytes)
  byte* cbuf; size_t cn, ccap; uint cap_on;

  void grow_pb() { pbcap = pbcap ? pbcap*2 : 4096; pbuf = (byte*)realloc(pbuf, pbcap);
                   if( !pbuf ) { fprintf(stderr, "ddsdet: oom\n"); exit(4); } }
  void grow_cb() { ccap = ccap ? ccap*2 : (1u<<16); cbuf = (byte*)realloc(cbuf, ccap);
                   if( !cbuf ) { fprintf(stderr, "ddsdet: oom\n"); exit(4); } }

  // unget(): pushed-back data means the model is no longer at EOF, so clear
  // f_quit (the flag the bundled parser polls). co_eof still records that the
  // underlying coroutine is dry, so once the pushback drains f_quit re-asserts.
  void unget( uint c )    { if( pbn>=pbcap ) grow_pb(); pbuf[pbn++] = (byte)c; f_quit = 0; }
  void cap_push( byte b ) { if( cn>=ccap ) grow_cb(); cbuf[cn++] = b; }

  // get(): honor pushback, count distinct source bytes, tee into cbuf while
  // capturing. Returns uint(-1) only at true model EOF (never teed / counted).
  // f_quit means "no more bytes available from the model" = pushback empty AND
  // coroutine exhausted; it is NOT the raw coroutine EOF (which would poison
  // every rescan of pushed-back data after the first over-read hits EOF).
  uint get( void ) {
    uint c;
    if( pbn ) {
      c = pbuf[--pbn];
      if( cap_on ) cap_push((byte)c);
      if( pbn==0 && co_eof ) f_quit = 1;   // last buffered byte; nothing follows
      return c;
    }
    if( co_eof ) { f_quit = 1; return (uint)-1; }   // don't re-poll a dead coroutine
    c = Coroutine::get();          // -1 iff EOF (real bytes are 0..255)
    if( c==(uint)-1 ) { co_eof = 1; f_quit = 1; return c; }
    src_pos++;
    if( cap_on ) cap_push((byte)c);
    return c;
  }

  // put(): suppressed while capturing (the texture becomes a sidecar file).
  d5c::vector<byte> gapbuf;   // with --all: the container bytes between textures
  uint embed = 0;

  void put( uint c ) {
    if( cap_on ) return;
    if( embed ) { gapbuf.push_back((byte)c); return; }
    Coroutine::put(c);
  }

  // ------------------------------------------------ bulk pin-buffer layer --
  // putn(): block write straight into the output window pin[1]; chkout()
  // after each chunk keeps put()'s "ptr<end on entry" invariant, yielding
  // r==2 to the pump exactly when the window fills (flush-only: the input
  // window is untouched, so in-flight memcpy sources stay valid).
  void putn( const byte* p, qword n ) {
    if( embed && !cap_on ) { for( qword i=0; i<n; i++ ) gapbuf.push_back(p[i]); return; }
    while( n ) {
      uint room = (uint)(pin[1].end - pin[1].ptr);
      uint k = n<(qword)room ? (uint)n : room;
      memcpy( pin[1].ptr, p, k ); pin[1].ptr += k;
      pin[1].chkout();
      p += k; n -= k;
    }
  }

  // skipn(): consume n bytes; window-sized memcpy chunks (teed into cbuf when
  // capturing) while the pushback is empty, get() otherwise.  false on EOF.
  bool skipn( qword n ) {
    while( n ) {
      if( pbn==0 && !co_eof ) {
        coro3_pin& I = pin[0];
        uint avail = (uint)(I.end - I.ptr);
        if( avail ) {
          uint k = n<(qword)avail ? (uint)n : avail;
          if( cap_on ) { while( cn+k>ccap ) grow_cb(); memcpy(cbuf+cn, I.ptr, k); cn += k; }
          I.ptr += k; src_pos += k; n -= k;
          continue;
        }
        if( f_quit ) { co_eof = 1; return false; }
        chkinp();                              // yield r==1 for a refill
        continue;
      }
      uint c = get();
      if( c==(uint)-1 ) return false;
      n--;
    }
    return true;
  }

  /* skipn_scan(): consume up to n payload bytes, stopping early if a "DDS "
   * signature appears inside them.
   *
   * A truncated texture declares a mipchain longer than the bytes that follow,
   * and plain skipn() swallows the NEXT texture whole as payload -- it gets
   * coded as this one's pixel data and the real header is never found.
   * Stopping at the signature carves a short candidate instead: parse_dds
   * rejects it with "truncated pixel data", enc_add stores it verbatim so the
   * bytes survive exactly, and the scan resumes on the header that was found.
   *
   * `got` receives the bytes consumed; the signature itself is left unconsumed
   * and un-teed.  The window loop does the bulk with memchr; the byte loop
   * carries a 3-byte rolling history so a signature straddling a window
   * boundary is still caught. */
  /* Is this "DDS " hit plausibly a real header, or four bytes of pixel data?
   *
   * The signature alone is 32 bits of evidence, which is NOT enough to overrule
   * a header that has already passed a full check().  Texture payloads carry it
   * by chance: in compressed blocks at roughly the random rate, and far more
   * often in uncompressed rasters, where 44 44 53 20 is an ordinary mid-grey
   * pixel with low alpha.  The file that exposed this -- a 4096x2048 DXT3 --
   * has two of them in its pixel data, and rejecting on the bare signature
   * threw the whole texture away.
   *
   * dwSize and ddspf.dwSize are exact values, so those two alone are 64 bits of
   * constraint; the dimension sanity check adds more.  Deliberately NOT testing
   * dwFlags: lax writers omit bits there, and the exact-size fields already
   * take the false-positive rate to nothing. */
  static bool looks_like_header( const byte* p, size_t n ) {
    /* Offsets from the "DDS " magic.  DDS_HEADER starts at +4 and ddspf sits
     * 72 bytes into it, so ddspf.dwSize is at +76 -- NOT +80, which is
     * ddspf.dwFlags and made this reject every real header. */
    enum { PF_OFF = 76 };
    if( n < PF_OFF + 4 ) return false;              // cannot judge -> not a header
    #define U32AT(o) (uint(p[o]) | uint(p[(o)+1])<<8 | uint(p[(o)+2])<<16 | uint(p[(o)+3])<<24)
    if( U32AT(4) != DDSHEADER::HDR_SIZE ) return false;
    /* {32, 0}, matching check(): lax writers zero ddspf.dwSize, and one of the
     * files here does.  Requiring 32 made such a header invisible to the scan,
     * so a truncated predecessor swallowed it. */
    { const uint pfs = U32AT(PF_OFF);
      if( pfs != DDSHEADER::PF_SIZE && pfs != 0 ) return false; }
    const uint h = U32AT(12), w = U32AT(16);        // dwHeight, dwWidth
    #undef U32AT
    return h && w && h <= 65536 && w <= 65536;
  }

  bool skipn_scan( qword n, qword& got ) {
    got = 0;
    while( n ) {
      if( pbn==0 && !co_eof ) {
        coro3_pin& I = pin[0];
        uint avail = (uint)(I.end - I.ptr);
        if( avail >= 4 ) {
          const byte* base = I.ptr; const byte* e = base + avail;
          const byte* q = base; const byte* hit = 0;
          while( q + 4 <= e ) {
            const byte* d = (const byte*)memchr(q, 'D', (size_t)(e - q - 3));
            if( !d ) break;
            if( d[1]=='D' && d[2]=='S' && d[3]==' '
                && looks_like_header(d, (size_t)(e - d)) ) { hit = d; break; }
            q = d + 1;                 // signature but not a header: pixel data
          }
          const qword hoff = hit ? (qword)(hit - base) : ~0ull;
          const bool stop = hit && hoff < n;      // a header inside our payload
          // never consume the last 3 bytes on a no-match window: a signature
          // could straddle into the next one, and the byte loop below needs
          // them to see it
          qword want = stop ? hoff : n;
          uint cap = stop ? avail : (avail - 3);
          uint k = want < (qword)cap ? (uint)want : cap;
          if( k ) {
            if( cap_on ) { while( cn+k>ccap ) grow_cb(); memcpy(cbuf+cn, I.ptr, k); cn += k; }
            I.ptr += k; src_pos += k; n -= k; got += k;
          }
          if( stop && k == (uint)hoff ) return true;
          if( k ) continue;
        }
        if( f_quit && avail == 0 ) { co_eof = 1; return false; }
      }
      // byte path: rolling 4-byte match across window and pushback boundaries
      uint c0 = get(); if( c0==(uint)-1 ) return false;
      if( c0 == 'D' ) {
        /* Peek a whole header's worth so the same validation applies here as in
         * the window path -- otherwise a signature straddling a window boundary
         * would be accepted on 32 bits while one inside a window needs 64+. */
        enum { PEEK = 4 + 80 };
        byte pk[PEEK]; pk[0] = byte(c0); uint m = 1;
        for( ; m<PEEK; m++ ) { uint v = get(); if( v==(uint)-1 ) break; pk[m] = byte(v); }
        for( uint k = m; --k; ) unget(pk[k]);
        cn -= (m - 1);                            // untee the peek
        if( m >= 4 && pk[1]=='D' && pk[2]=='S' && pk[3]==' '
            && looks_like_header(pk, m) ) { unget(c0); cn--; return true; }
      }
      n--; got++;
    }
    return true;
  }

  // fast_skip(): the fast signature search.  Runs while no pushback exists:
  // memchr for 'D' over the input window, bulk-copy everything before it to
  // the output, memcmp the full "DDS " in place.  Returns with the window
  // positioned at a 4-byte signature match, at a 'D' within 3 bytes of the
  // window end (buffer boundary -- the byte-wise path resolves it), or at
  // EOF / nonempty pushback (byte-wise path handles those too).
  void fast_skip( void ) {
    if( pbn || co_eof ) return;
    coro3_pin& I = pin[0];
    while(1) {
      uint avail = (uint)(I.end - I.ptr);
      if( avail==0 ) {
        if( f_quit ) return;                   // dry: byte-wise get() reports EOF
        chkinp();                              // yield r==1 for a refill
        continue;
      }
      byte* hit = (byte*)memchr( I.ptr, 'D', avail );
      if( !hit ) {
        putn( I.ptr, avail ); src_pos += avail; I.ptr = I.end;
        continue;
      }
      uint pre = (uint)(hit - I.ptr);
      if( pre ) { putn( I.ptr, pre ); src_pos += pre; I.ptr = hit; }
      if( avail-pre < 4 ) return;              // sig may straddle the window
      if( memcmp( I.ptr, DDS_SIG, 4 )==0 ) return;
      putn( I.ptr, 1 ); src_pos++; I.ptr++;    // lone 'D'
    }
  }

  // Walk one texture from "DDS " through its computed extent. Consume-only:
  // every byte teed into cbuf. Returns true iff the header validates and the
  // payload length is exactly derivable -- in which case exactly total_len
  // bytes were consumed.
  bool scan_dds( void ) {
    whyrej = "";
    hdr.Parse(this);
    if( f_quit || hdr.parse_err ) { whyrej = "header truncated"; return false; }
    if( !hdr.check() ) { whyrej = hdr.why; return false; }
    /* Ambiguous extent (see dds_header.inc: block grid, chain length).  Walk
       the shorter candidates in order, and after each look at the four bytes
       that follow.  A "DDS " there -- or the end of the input -- means that
       candidate was where the texture really ended, so take it.  Anything else
       and we go on, ending at the default extent.
       Every segment goes through skipn_scan, so a VALIDATED header inside the
       payload still ends the entry.  That matters more than it used to: the
       default extent can now be LONGER than dwMipMapCount declares, and
       without the scan an over-long extent would swallow its successor whole
       -- the same failure plain skipn() had on a truncated texture. */
    qword at = 0;
    for( uint i=0; i<=hdr.alts; i++ ) {
      const qword want = (i<hdr.alts) ? hdr.alt_len[i] : hdr.data_len;
      qword got = 0;
      const bool ok = skipn_scan( want - at, got );
      at += got;
      if( at < want ) {
        /* Ran into a validated header, or into the end of the input, before
         * this candidate's extent -- so the entry is truncated and `at` is its
         * real length.  parse_dds accepts a short chain and the walk codes
         * whole block rows of whatever fits, with the remainder going to the
         * tail model.  Earlier this rejected instead, because back when
         * parse_dds refused a short chain the entry was stored verbatim, and
         * raw storage lost to leaving the bytes in the gap (30040 B against
         * 28029 B).  Modelling them beats both. */
        if( !ok && at == 0 ) { whyrej = "payload runs past end of input"; return false; }
        hdr.data_len = at; hdr.total_len = hdr.hdr_len + at;
        return true;
      }
      if( i==hdr.alts ) break;                     // the default extent: done
      uint p[4]; uint n = 0;
      for( ; n<4; n++ ) { p[n] = get(); if( p[n]==(uint)-1 ) break; }
      bool eof = n<4;
      bool sig = n==4 && p[0]=='D' && p[1]=='D' && p[2]=='S' && p[3]==' ';
      for( uint k=n; k--; ) unget(p[k]);
      cn -= n;                                     // untee the peek
      if( sig || eof ) {
        hdr.data_len = at; hdr.total_len = hdr.hdr_len + at;
        return true;
      }
    }
    return true;
  }

  // -------------------------------------------------------------- carve ----
  void carve( void ) {
    if constexpr( mode == 0 ) {
      if( op==2 && sol ) sol->enc_begin(cfg_nonsolid, cfg_embed);
    }
    while(1) {
      fast_skip();                              // bulk-skip to the next "DDS "
      qword cand_off = src_pos - pbn;           // logical position of next byte
      uint b0 = get();
      if( b0==(uint)-1 ) break;                 // EOF
      if( b0!='D' ) { put(b0); continue; }      // pushback drain / boundary

      uint b1 = get();
      if( b1==(uint)-1 ) { put(b0); break; }
      uint b2 = get();
      if( b2==(uint)-1 ) { put(b0); unget(b1); continue; }
      uint b3 = get();
      if( b3==(uint)-1 ) { put(b0); unget(b2); unget(b1); continue; }

      if( !(b1=='D' && b2=='S' && b3==' ') ) {  // not "DDS "
        put(b0); unget(b3); unget(b2); unget(b1);
        continue;
      }

      // candidate texture: replay the signature into the parser, capture all.
      unget(b3); unget(b2); unget(b1); unget(b0);
      cn = 0; cap_on = 1;
      bool ok = scan_dds();
      cap_on = 0;

      if( ok ) {
        char name[4096];
        if( sol ) {
          if( embed ) { sol->enc_gap(gapbuf.data(), gapbuf.size()); gapbuf.clear(); }
          d5c::ByteBuf ep;
          /* The epx record is written by the COLOUR endpoint plane, which does
           * not run for BC4/BC5/BTC -- those have no 565 half.  The alpha plane
           * only stages the pair, so on an alpha-only format the stream stays
           * empty and --bmp used to emit an all-black bitmap and then decode
           * garbage.  Fall back to coding those textures normally: the entry
           * mode is recorded in the archive, so the decoder simply does not
           * look for a bitmap. */
          d5c::dds_info di; const char* e2 = "";
          /* Every block format now externalises endpoints: colour formats write
           * RGBA (RGB from the 565 pair, A from the alpha pair), alpha-only
           * formats write A with RGB left at 0.  FMT_RAW / FMT_MODE have no
           * endpoint pair at all, so they still code normally. */
          bool haveep = bmp && d5c::parse_dds(cbuf, cn, di, &e2)
                            && (d5c::fmt_color(di.fmt) || d5c::fmt_ach(di.fmt) > 0);
          bool modelled = sol->enc_add(cbuf, cn, haveep ? &ep : 0);
          if( modelled && haveep ) {
            d5c::vector<byte> px; uint bw2 = 0, bh2 = 0;
            char bn[4096];
            const uint nsurf = di.faces;             // one bitmap per face; BC5's
            for( uint fc = 0; fc < nsurf; fc++ ) {   // two channels share RGBA
              ep_to_bmp(di, ep, fc, px, bw2, bh2);
              ep_name(bn, sizeof bn, bprefix, zindex, fc, nsurf);
              if( !wbmp32(bn, bw2, bh2, px.data()) ) { rc = 1; return; }
            }
            snprintf(name, sizeof name, "%s#%u [%zu B] + %s (%ux%u%s)",
                     solpath, zindex, sol->last_coded, bn, bw2, bh2,
                     nsurf > 1 ? ", per face" : "");
          } else
            snprintf(name, sizeof name, "%s#%u [%zu B%s]", solpath, zindex, sol->last_coded, modelled ? "" : " stored");
        } else {
          dds_name(name, sizeof name, prefix, zindex);
          if( !write_file(name, cbuf, cn) ) { rc = 1; return; }
        }

        qword orig_len    = cn;
        qword placeholder = (zero && orig_len>=MK_SIZE) ? orig_len : (qword)MK_SIZE;
        if( !embed ) {                       // with --all the interleaving locates
          byte mk[MK_SIZE]; mk_build(mk, zindex, orig_len, placeholder, cand_off);
          putn(mk, MK_SIZE);                 // each texture, so no marker is needed
          for( qword p=placeholder-MK_SIZE; p; ) { uint k = p>sizeof(zpad)?(uint)sizeof(zpad):(uint)p; putn(zpad,k); p-=k; }
        }

        char sf[32]; hdr.subfmt(sf, sizeof sf);
        fprintf(stderr, "  [%04X] %08llX %9llu B  %ux%u mips=%u %s %s -> %s%s\n",
                zindex, (unsigned long long)cand_off, (unsigned long long)orig_len,
                hdr.dwWidth, hdr.dwHeight, hdr.mips, hdr.kindname(), sf,
                name, zero ? " [zeroed]" : "");
        zindex++;
      } else {
        if( why )
          fprintf(stderr, "  [skip] off=%llu: %s\n",
                  (unsigned long long)cand_off, whyrej && *whyrej ? whyrej : "rejected");
        // false positive / truncated: emit first byte, rescan the rest.
        if( cn>0 ) {
          put(cbuf[0]);
          for( size_t i=cn; i>1; i-- ) unget(cbuf[i-1]);
        } else {
          // scan consumed nothing (candidate still buffered): force progress by
          // emitting one byte, so the loop can never spin in place.
          uint c = get();
          if( c!=(uint)-1 ) put(c);
        }
      }
    }
  }

  // ------------------------------------------------------------- restore ---
  void restore( void ) {
    // ds (two-file): prime the coder here, inside the coroutine, because it
    // pulls its first bytes through a pin and may yield.
    if constexpr( mode == 1 ) {
      if( op==3 && sol ) {
        const char* e = "";
        if( !sol->dec_begin(&e) ) { fprintf(stderr, "ddsdet: %s\n", e); rc = 1; return; }
      }
    }
    while(1) {
      fast_skip();                              // markers begin with "DDS " too
      uint b0 = get();
      if( b0==(uint)-1 ) break;
      if( b0!='D' ) { put(b0); continue; }

      byte m[MK_SIZE]; m[0]=(byte)b0; uint got=1;
      for( ; got<MK_SIZE; got++ ) { uint c=get(); if( c==(uint)-1 ) break; m[got]=(byte)c; }

      uint index; qword orig_len, placeholder, orig_off;
      if( got==MK_SIZE && mk_parse(m, &index, &orig_len, &placeholder, &orig_off) ) {
        if( sol ) {
          if( index!=zindex ) {
            fprintf(stderr, "ddsdet: marker %u out of order (expected %u); a solid "
                            "archive decodes strictly in carve order\n", index, zindex);
            rc = 1; return;
          }
          const char* e = "";
          static d5c::ByteBuf tex;
          int r = sol->dec_peek(&e);
          if( r==2 ) {
            char bn[4096];
            d5c::vector<byte> px; uint bw2, bh2;
            d5c::ByteBuf ep;
            d5c::dds_info di;
            // the entry's own header tells us the block count and geometry
            if( !sol->peek_geometry(di) ) {
              fprintf(stderr, "ddsdet: can't size endpoints for texture %u\n", index); rc = 1; return;
            }
            const uint nch = (d5c::fmt_ach(di.fmt) == 2) ? 2u : 1u;
            const uint nsurf = di.faces;   // BC5 shares one bitmap
            ep.assign_zero((size_t)sol->peek_blocks() * nch * 6);
            for( uint fc = 0; fc < nsurf; fc++ ) {
              ep_name(bn, sizeof bn, bprefix, index, fc, nsurf);
              if( !rbmp32(bn, bw2, bh2, px) ) { rc = 1; return; }
              if( !bmp_to_ep(di, px, bw2, bh2, fc, ep) ) { rc = 1; return; }
            }
            r = sol->dec_body(tex, &e, &ep);
          } else if( r==1 ) {
            r = sol->dec_body(tex, &e, 0);
          }
          if( r<=0 ) {
            fprintf(stderr, "ddsdet: solid decode %s at texture %u%s%s\n",
                    r==0 ? "ran out of textures" : "failed", index,
                    r==0 ? "" : ": ", r==0 ? "" : e);
            rc = 1; return;
          }
          if( tex.size()!=orig_len ) {
            fprintf(stderr, "ddsdet: size mismatch for texture %u: have %zu, expected %llu\n",
                    index, tex.size(), (unsigned long long)orig_len);
            rc = 1; return;
          }
          putn(tex.data(), tex.size());
        } else {
          char name[4096]; dds_name(name, sizeof name, prefix, index);
          size_t zn=0; byte* z = read_file(name, &zn);
          if( !z ) { rc = 1; return; }
          if( zn!=orig_len ) {
            fprintf(stderr, "ddsdet: size mismatch for '%s': have %zu, expected %llu\n",
                    name, zn, (unsigned long long)orig_len);
            free(z); rc = 1; return;
          }
          putn(z, zn);
          free(z);
        }
        skipn( placeholder-MK_SIZE );           // EOF here is tolerated
        zindex++;
      } else {
        put(b0);
        for( uint i=got; i>1; i-- ) unget(m[i-1]);
      }
    }
  }

  /* op 4 = ds --all.  This used to be a loop in main(), the one coding path
   * that ran on the process stack instead of the coroutine's.  Moving it in
   * here makes the archive an ordinary pin stream like every other input, so
   * the range coder can pull from it directly, and lets the decoded gap bytes
   * go straight out through put() instead of being staged in pend_gap. */
  void restore_all( void ) {
    if constexpr( mode == 1 ) {
      const char* e = ""; int r; qword tot = 0;
      // gap bytes are container bytes: stream them out as they decode
      sol->gap_ctx = this;
      sol->gap_out = []( void* c, const d5c::u8* d, size_t n ) {
        ((ddsdet*)c)->putn((const byte*)d, n);
      };
      if( !sol->dec_begin(&e) ) { fprintf(stderr, "ddsdet: %s\n", e); rc = 1; return; }
      for(;;) {
        r = sol->dec_peek(&e);
        if( r < 0 ) { fprintf(stderr, "ddsdet: solid decode %s\n", e); rc = 1; return; }
        tot += sol->gap_len;          // the sink already wrote them
        if( r == 0 ) break;
        d5c::ByteBuf tex;
        if( sol->dec_body(tex, &e, 0) < 0 ) { fprintf(stderr, "ddsdet: solid decode %s\n", e); rc = 1; return; }
        putn(tex.data(), tex.size()); tot += tex.size(); zindex++;
      }
      allbytes = tot;
      sol->rc_trace("decoded(all)");
    }
  }

  qword allbytes = 0;   // op 4: total container bytes written
  uint cfg_nonsolid = 0, cfg_embed = 0;   // enc_begin args, applied inside do_process
  const byte* arcp = 0; size_t arcn = 0;   // archive image (ds); header already parsed

  void do_process( void ) {
    src_pos = 0; zindex = 0; rc = 0; co_eof = 0; whyrej = "";
    pbn = 0; cn = 0; cap_on = 0;

    if( op==0 || op==2 ) carve(); else if( op==4 ) restore_all(); else restore();

    /* Close the archive here, not in main(): enc_gap/enc_end write through
     * the rc pin and can yield, so they have to run on the coroutine stack
     * like every other coding step. */
    if constexpr( mode == 0 ) {
      if( op==2 && sol && rc==0 ) {
        if( embed ) sol->enc_gap(gapbuf.data(), gapbuf.size());
        sol->enc_end();
      }
    }

    yield(this, 0);
  }

};
