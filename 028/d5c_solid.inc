/* d5c_solid.inc -- part of dxt5comp_core.inc; not standalone.
 *
 * texture_init, SolidBase and Solid<MD> -- the archive layer. */

//------------------------------------------------------------------ SOLID

/* Reset the cross-level (mip) prediction state between textures.  level_init()
 * decides "do I have a parent level?" from the previous level's iw/ih, so
 * without this the first level of texture N+1 would treat the last level of
 * texture N as its 2x-finer parent.  Everything else -- counters, mixers, APM,
 * hash tables -- is deliberately NOT reset: that carry-over is the solid gain. */
template<int MD>
static inline void texture_init(Codec<MD>& cx) { cx.iw = 0; cx.ih = 0; cx.hasP = false; }

/* Size the geometry slabs for this texture before coding it.  Called with the
 * cross-level state already dead (texture_init), which is the only point where
 * re-pointing the rotating views is free.  Grows only. */
template<int MD>
static inline void texture_reserve(Codec<MD>& cx, const dds_info& di) {
  /* FMT_RAW and FMT_MODE never reach level_init: code_raster and
     code_mode_blocks work out of the fixed-size sRaster / sModeRow rows.
     Their "block grid" is the pixel grid (bedge 1), so sizing the level slabs
     from it would ask for 16*w*h plane bytes for a texture that does not use a
     single one of them -- 1.2 GiB for a 2048x2048 raster. */
  if(di.fmt == FMT_RAW || di.fmt == FMT_MODE) return;
  u32 w, h, bw, bh; level_dims(di, 0, w, h, bw, bh);
  /* What level_init(bw,bh) actually touches, and level 0 is the largest level
     of the chain, so this covers the whole texture. */
  cx.reserve(size_t(bw) * bh * 16, size_t(bw) * bh, bw);
}

/* Solid archive layout:
 *
 *     "D5S1"            4 bytes
 *     varint rc_len     length of the range-coded stream
 *     rc stream         everything else
 *     stored blobs      verbatim, in order, for mode-1 entries
 *
 * There is no fixed-width directory. Per texture the coded stream carries a
 * continuation bit, then mode / length / crc32 as bytes through the tail
 * model, then the texture itself. The earlier layout put a 16-byte entry per
 * texture outside the model -- 1-bit mode padded to 4 bytes, and a u64 length
 * whose top 5 bytes are always zero -- so roughly 9 of every 16 bytes were
 * structural zeros that nothing could ever compress, growing linearly with the
 * texture count.
 *
 * The flags byte at the head of the coded stream carries bit 0 = "non-solid":
 * the model is fully re-initialised once per .dds FILE, so each file is coded
 * exactly as an independent .d5c would be, while still landing in a single
 * archive.  Note the granularity: the reset is per file, NOT per image inside
 * it -- every mip level of a .dds still shares one model, which is what makes
 * the cross-level (A1) prediction work at all.  Whether non-solid wins depends
 * entirely on the parameter set -- tuning on single files makes cross-file
 * carry-over a liability -- so it is a switch, not a decision baked into the
 * format.
 *
 * Stored blobs stay OUT of the coded stream on purpose: they are the textures
 * dxt5comp cannot model (BC7, cubemaps, volumes), and pushing that data
 * through the order-1 tail model would barely compress it while polluting the
 * statistics that the DDS headers and real tails depend on.
 */
/* Counters main() reads after processfile() returns, kept out of the template
 * so the frontend need not branch on the instantiation. */
struct SolidBase {
  u64  raw_total = 0;      // sum of entry lengths seen so far
  u32  nents = 0;
  size_t meta_coded = 0;   // bytes the metadata records cost
  size_t last_coded = 0;   // coded size of the most recent entry
  u8   last_mode = 0;      // mode of the entry enc_add just wrote (3 = a copy)
  u64  last_dup_pos = 0;   // ...and where the original was, for the report
};

template<int MD>
struct Solid : SolidBase {
  enum { enc = (MD == 0) };
  Codec<MD> cx;
  Solid(RC<MD>& r) : cx(r) {}
  coro3_pin* rcpin = 0;    // which pin carries the coded stream; 0 = default
  coro3_pin* rcpin_keep() const { return rcpin; }
  coro3_pin* rcp() { return rcpin ? rcpin : cx.rc.rc_defpin(); }
  struct Ent { u8 mode; u32 crc; u64 len; u32 slot; };
  /* Mode 3 -- a texture byte-identical to one the dedup table still holds.  It
     carries no pixel data at all, just the slot the copy comes from, so a
     repeat costs the record instead of a second full coding pass.  See
     d5c_dedup.inc for why both directions can agree on what that slot holds. */
  Dedup dd;
  // raw_total / nents / last_coded / meta_coded live in SolidBase
  Ctr cmore;               // continuation bit: "another texture follows"
  bool started = false;
  bool nonsolid = false;   // reset the model once per .dds file (not per level)
  bool first_entry = true; // the enc_begin/dec_begin init serves file #1

  static u32 crc_of(const u8* d, size_t n) { return crc32b(d, n); }

  static void put_varint(ByteBuf& v, u64 x) {
    do { u8 b = u8(x & 0x7F); x >>= 7; v.push(u8(b | (x ? 0x80 : 0))); } while(x);
  }
  static bool get_varint(const u8* p, size_t n, size_t& i, u64& out) {
    out = 0;
    for(int sh = 0; sh < 64; sh += 7) {
      if(i >= n) return false;
      u8 b = p[i++];
      out |= u64(b & 0x7F) << sh;
      if(!(b & 0x80)) return true;
    }
    return false;
  }

  /* One metadata record: mode, varint length, [mode 3: varint slot,] crc32 --
     through the tail model.  1 mode + at most 10 varint bytes + 4 crc = 15;
     mode 3 adds a second varint, so 25, and meta_dec caps at 26. */
  void meta_enc( const Ent& e ) {
    u8 m[26]; size_t k = 0;
    m[k++] = e.mode;
    u64 x = e.len;
    do { u8 b = u8(x & 0x7F); x >>= 7; m[k++] = u8(b | (x ? 0x80 : 0)); } while(x);
    if(e.mode == 3) { x = e.slot; do { u8 b = u8(x & 0x7F); x >>= 7; m[k++] = u8(b | (x ? 0x80 : 0)); } while(x); }
    for(int i = 0; i < 4; i++) m[k++] = u8(e.crc >> (8 * i));
    u8 n = u8(k);
    cx.code_tail(&n, 1);
    cx.code_tail(m, k);
  }
  bool meta_dec( Ent& e ) {
    u8 n = 0;
    cx.code_tail(&n, 1);
    if(n < 6 || n > 26) return false;
    u8 m[26] = {0};
    cx.code_tail(m, n);
    e.mode = m[0];
    size_t i = 1; u64 len = 0, slot = 0;
    if(!get_varint(m, n, i, len)) return false;
    if(e.mode == 3 && !get_varint(m, n, i, slot)) return false;
    if(i + 4 != n) return false;
    e.len = len;
    e.slot = u32(slot);
    if(slot != (u64)e.slot) return false;
    e.crc = 0;
    for(int k = 0; k < 4; k++) e.crc |= u32(m[i + k]) << (8 * k);
    return true;
  }

  inline void more_bit( int b ) {          // b = -1 to decode
    int p = cmore.pr();
    int r = cx.rc.bit(p, b);
    cmore.upd(r);
    if(b < 0) last_more = r;
  }
  int last_more = 0;

  // ---- encode --------------------------------------------------------
  /* Embedded mode: the container's non-texture bytes -- everything between
   * the textures -- ride in the coded stream too, as a "gap" before each
   * entry and one after the last, so the archive is the ONLY output file.
   * The gaps go through the tail model, which is the same order-1 model the
   * DDS headers use and exactly the right shape for container structure.
   * With gaps present the caller emits no markers: the interleaving itself
   * says where each texture sat. */
  bool embed = false;

  void enc_gap( const u8* d, size_t n ) {
    if( !embed ) return;
    u8 lb[8];
    for( int i=0; i<8; i++ ) lb[i] = u8((u64)n >> (8*i));
    cx.code_tail(lb, 8);
    if( n ) cx.code_tail(const_cast<u8*>(d), n);
  }
  /* Container gap bytes go straight to the frontend as they decode; only a
   * 4 KB window exists at a time.  code_tail carries `prev` across the chunks
   * so the order-1 model still sees one continuous sequence. */
  void (*gap_out)(void* ctx, const u8* d, size_t n) = 0;
  void*  gap_ctx = 0;
  u64    gap_len = 0;              // bytes the last dec_gap produced
  void dec_gap( void ) {
    gap_len = 0;
    if( !embed ) return;
    u8 lb[8] = {};
    cx.code_tail(lb, 8);
    u64 n = 0;
    for( int i=7; i>=0; i-- ) n = (n << 8) | lb[i];
    gap_len = n;
    u8 buf[4096]; int prev = 0;
    while( n ) {
      size_t k = n > sizeof buf ? sizeof buf : (size_t)n;
      cx.code_tail(buf, k, prev);
      if( gap_out ) gap_out(gap_ctx, buf, k);
      n -= k;
    }
  }

  /* Raw p=1/2 bits, LSB first -- the same trick the stream flags use, for the
     dedup geometry that has to survive into the decoder. */
  void raw_bits( u32& v, int nb ) {
    if constexpr( enc ) { for(int i = 0; i < nb; i++) cx.rc.bit(2048, (v >> i) & 1); }
    else { v = 0; for(int i = 0; i < nb; i++) v |= u32(cx.rc.bit(2048, -1) != 0) << i; }
  }

  void enc_begin( bool nonsolid_ = false, bool embed_ = false, u32 dedup_hb = 0, u32 dedup_mb = 0 ) {
    nonsolid = nonsolid_;
    embed = embed_;
    init_tables();
    cx.rc.init_enc(rcpin);
    /* D5S5: D5S4 plus the dedup stream flag.  The flag has to be at stream
       level rather than implied by the first mode-3 record, because the decoder
       must start RETAINING entries before it ever sees a reference to one --
       and a decoder that retained unconditionally would pay the memory budget
       on every archive, deduped or not.  Bumping the magic buys a clean "or an
       older revision" from dec_begin instead of a confusing "corrupt entry
       record" the first time an old build meets a mode-3 entry. */
    { static const u8 m[4] = {'D','5','S','5'}; cx.rc.raw_put(m, 4); }
    // Stream flag, bit 0 = non-solid.  Coded as a RAW range-coder bit at p=1/2,
    // not through the tail model: that way it needs no tables, so the one init
    // below leaves a pristine model that serves as file #1's own reset and
    // there is exactly one init per .dds file rather than one extra.
    cx.rc.bit(2048, nonsolid ? 1 : 0);
    cx.rc.bit(2048, embed ? 1 : 0);   // second stream flag, same raw-bit trick
    cx.rc.bit(2048, dedup_hb ? 1 : 0);
    if( dedup_hb ) {
      /* Geometry in the stream, not as a build constant: the retained set is
         shared state, so a decoder built with a different default would drift
         from the encoder on the first eviction.  init() first, and write back
         what it settled on rather than what was asked for, so the stream can
         never carry a value the encoder did not actually use. */
      dd.init(dedup_hb, dedup_mb);
      u32 a = dd.hb, b = dd.mb;
      raw_bits(a, 6); raw_bits(b, 6);
    }
    cx.init(FMT_DXT5);            // allocates every table; fmt is per file
    first_entry = true;
    started = true;
  }

  // add one texture; returns true if it was modelled, false if stored verbatim.
  // `pos` is where it sat in the container -- report only, never coded.
  bool enc_add( const u8* d, size_t n, ByteBuf* ep = 0, u64 pos = 0 ) {
    u64 rc0 = cx.rc.emitted;
    Ent e; e.mode = 1; e.crc = crc_of(d, n); e.len = n; e.slot = 0;

    /* Seen this exact texture recently?  The lookup verifies with a memcmp, so
       a hash collision costs a dedup, not correctness -- and because a miss and
       a rejected hit both leave the table untouched, the decoder (which sees
       neither) stays in step. */
    Hash128 hh;
    const bool dedup = dd.on;
    if( dedup ) {
      hh = hash128(d, n);
      const u32 s = dd.find(hh, d, n);
      if( s != DD_NIL ) {
        e.mode = 3; e.slot = s;
        u64 m1 = cx.rc.emitted;
        more_bit(1);
        meta_enc(e);
        meta_coded += size_t(cx.rc.emitted - m1);
        raw_total += e.len; nents++;
        dd.touch(s);                    // mirrored by dec_body's resolve
        dd.hits++; dd.saved += n;
        last_mode = 3; last_dup_pos = dd.tab[s].pos;
        last_coded = size_t(cx.rc.emitted - rc0);
        first_entry = false;
        return true;
      }
    }

    dds_info di; const char* err = "";
    bool modelled = (n >= 128 && parse_dds(d, n, di, &err));
    e.mode = modelled ? (ep ? 2 : 0) : 1;   // 2 = coded with external endpoints
    last_mode = e.mode;

    u64 m0 = cx.rc.emitted;
    more_bit(1);
    meta_enc(e);
    meta_coded += size_t(cx.rc.emitted - m0);
    raw_total += e.len; nents++;
    /* Retain BEFORE coding: the decoder inserts the entry the moment it has the
       bytes, which is after decoding, and neither side may reference an entry
       from within its own record -- so the two orders agree on the table state
       every reference is resolved against. */
    if( dedup ) dd.insert(hh, d, n, pos, 0);

    if( modelled ) {
      // independent model per .dds FILE (once here, not per mip level below).
      // file #1 skips it: enc_begin's init already left the model pristine.
      if( nonsolid && !first_entry ) cx.init(FMT_DXT5);
      cx.fmt = di.fmt;
      texture_init(cx); texture_reserve(cx, di);
      cx.epx = ep; cx.epx_pos = 0; cx.epx_fail = false;
      // code_tail only reads its buffer when encoding, so the source bytes go
      // straight through -- no copy needed
      cx.code_tail(const_cast<u8*>(d), di.data_off);   // the DDS header itself
      size_t off = di.data_off;
      for( u32 fc2=0; fc2<di.faces; fc2++ ) {
      texture_init(cx);          // each cube face is its own mipchain
      for( u32 l=0; l<di.levels; l++ ) {
        u32 w,h,bw,bh; level_dims(di, l, w, h, bw, bh);
        size_t bytes = size_t(bw)*bh*di.bsize;
        for( u32 z=0; z<level_depth(di, l); z++ ) {
          /* Truncated entry: code whole BLOCKS of the level that does not fit,
           * then stop.  The level keeps its true bw/bh so every context still
           * sees the real geometry; blk_lim only ends the scan early.  The
           * partial block and everything after go to code_tail below. */
          u32 nblk_ = bw*bh;
          if( di.truncated ) {
            const size_t left = off < n ? n - off : 0;
            if( bytes > left ) {
              nblk_ = u32(left / di.bsize);
              if( !nblk_ ) { l = di.levels; fc2 = di.faces; break; }
            }
          }
          cx.blk_lim = (nblk_ == bw*bh) ? 0 : nblk_;
          if( di.fmt==FMT_RAW ) cx.code_raster(const_cast<u8*>(d) + off, w, h, di.bsize);
          else if( di.fmt==FMT_MODE ) cx.code_mode_blocks(const_cast<u8*>(d) + off, bw, bh, di.bsize);
          else { cx.level_init(bw, bh); cx.code_level(const_cast<u8*>(d) + off); }
          cx.blk_lim = 0;
          off += size_t(nblk_)*di.bsize;
          if( nblk_ != bw*bh ) { l = di.levels; fc2 = di.faces; break; }
        }
      }
      }
      if( off < n ) cx.code_tail(const_cast<u8*>(d) + off, n - off);
      cx.epx = 0;
      first_entry = false;
      last_coded = size_t(cx.rc.emitted - rc0);
      return true;
    }
    /* Mode 1: the texture could not be modelled, so its bytes go into the
     * stream verbatim, bracketed by a flush and a fresh coder.  ~10 bytes of
     * bracket, and the blob never enters any model -- the property the old
     * trailing "stored" section existed to guarantee.  Only the coder's
     * low/range are reset; every counter, mixer and hash carries on, so the
     * neighbouring entries compress exactly as before. */
    first_entry = false;
    cx.rc.flush();
    cx.rc.raw_put(d, n);
    cx.rc.init_enc(rcpin_keep());
    last_coded = n;
    return false;
  }

  void enc_end( void ) { if( started ) { more_bit(0); cx.rc.flush(); } }

  /* The invariant D5S4 rests on: over a complete init..flush session the
   * encoder emits exactly as many bytes as the decoder consumes, so a
   * sequential decoder stops precisely at stream end and the stored length is
   * redundant.  Asserted rather than assumed -- set D5C_RCTRACE=1. */
  void rc_trace( const char* what ) const {
    if( getenv("D5C_RCTRACE") )
      fprintf(stderr, "d5c: rc %s emitted=%llu consumed=%llu\n", what,
              (unsigned long long)cx.rc.emitted, (unsigned long long)cx.rc.consumed);
  }


  // ---- decode --------------------------------------------------------
  u32 ndec = 0;

  /* Primes the range coder, so it can pull from the pin and must run INSIDE
   * the coroutine. */
  bool dec_begin( const char** err ) {
    // read the magic BEFORE init_dec, which primes with 5 coded bytes
    { coro3_pin* q = rcp(); u8 m[4];
      for(int i = 0; i < 4; i++) { uint c = q->get(); if(c > 255) return *err = "d5s truncated", false; m[i] = u8(c); }
      if( memcmp(m, "D5S5", 4) ) return *err = "not a d5s archive (or an older revision)", false; }
    init_tables();
    cx.rc.init_dec(rcpin);
    nonsolid = cx.rc.bit(2048, -1) != 0;   // raw bits, mirroring enc_begin
    embed    = cx.rc.bit(2048, -1) != 0;
    if( cx.rc.bit(2048, -1) != 0 ) {       // dedup: geometry follows
      u32 hb = 0, mb = 0;
      raw_bits(hb, 6); raw_bits(mb, 6);
      dd.init(hb, mb);
    }
    cx.init(FMT_DXT5);                     // ...and, as there, serves file #1
    first_entry = true;
    ndec = 0; raw_total = 0; nents = 0; pending = false;
    return true;
  }

  u32 count( void ) const { return ndec; }

  /* Decoding is two steps because the entry's mode lives inside the coded
   * stream: dec_peek() pulls the continuation bit and the record, so the
   * caller learns whether this texture needs an external endpoint file before
   * it has to hand one over; dec_body() then decodes the texture itself. */
  Ent pend;                       // entry from the last dec_peek
  bool pending = false;
  u8   pend_hdr[148];             // the entry's DDS header (128, or 148 with DX10)
  u32  pend_hdr_n = 0;
  dds_info pend_di;               // ...parsed, so the caller can size things
  u64 pend_nblk = 0;

  // 2 = next texture needs external endpoints, 1 = self-contained,
  // 0 = end of archive, -1 = error.  For coded entries this already decodes
  // and parses the DDS header, so peek_geometry()/peek_blocks() are usable --
  // the caller needs the block count to read an endpoint image back.
  int dec_peek( const char** err ) {
    if( pending ) return pend.mode==2 ? 2 : 1;
    dec_gap();                    // the container bytes that preceded this entry
    more_bit(-1);
    if( !last_more ) return 0;
    if( !meta_dec(pend) ) return *err = "corrupt entry record", -1;
    /* No archive image to size-check against any more.  A corrupt length is
     * caught by the crc32 of the decoded entry; this only stops an absurd
     * allocation before that happens. */
    if( pend.len > (u64)1 << 40 ) return *err = "implausible entry length", -1;
    pend_nblk = 0;
    /* Mode 3 is a reference: no header in the stream, nothing to model, and
       nothing for the caller to supply -- so it announces itself as an ordinary
       self-contained entry and dec_body serves it from the table. */
    if( pend.mode == 3 ) {
      if( !dd.valid(pend.slot, (size_t)pend.len) ) return *err = "dedup reference to an entry the table no longer holds", -1;
      pending = true;
      return 1;
    }
    if( pend.mode != 1 ) {
      if( pend.len < 128 ) return *err = "coded entry too small", -1;
      // must mirror enc_add: the reset happens after the entry record and
      // before the DDS header, so both sides code the header with the same
      // model state.  fmt is set later, from the header we are about to read.
      if( nonsolid && !first_entry ) cx.init(FMT_DXT5);
      // the header is 128 bytes, or 148 when a DX10 block follows the fourcc;
      // decode the fixed part first, then the extension it announces
      memset(pend_hdr, 0, sizeof pend_hdr);
      pend_hdr_n = 128;
      cx.code_tail(pend_hdr, 128);
      if( !memcmp(pend_hdr + 84, "DX10", 4) ) {
        if( pend.len < 148 ) return *err = "coded entry too small for its DX10 header", -1;
        pend_hdr_n = 148;
        cx.code_tail(pend_hdr + 128, 20);
      }
      if( !parse_dds(pend_hdr, (size_t)pend.len, pend_di, err) ) return -1;
      for( u32 l=0; l<pend_di.levels; l++ ) {
        u32 w,h,bw,bh; level_dims(pend_di, l, w, h, bw, bh);
        pend_nblk += u64(bw)*bh*level_depth(pend_di, l);
      }
      pend_nblk *= pend_di.faces;
    }
    pending = true;
    return pend.mode==2 ? 2 : 1;
  }
  bool peek_geometry( dds_info& di ) const { if(pend.mode==1) return false; di = pend_di; return true; }
  u64  peek_blocks( void ) const { return pend_nblk; }

  // decode the texture dec_peek() announced; ep required iff it returned 2
  int dec_body( ByteBuf& dst, const char** err, const ByteBuf* ep = 0 ) {
    if( !pending ) return *err = "dec_body without dec_peek", -1;
    pending = false;
    const Ent& e = pend;
    if( e.mode==3 ) {
      const Dedup::Slot& t = dd.tab[e.slot];
      dst.assign_copy(t.data.data(), t.data.size());
      dd.touch(e.slot);                    // mirrors enc_add's hit
      dd.hits++; dd.saved += (size_t)e.len;
      if( crc_of(dst.data(), dst.size()) != e.crc ) return *err = "crc mismatch on a dedup reference", -1;
      first_entry = false;
      raw_total += e.len; nents++; ndec++;
      return 1;
    }
    dst.assign_zero((size_t)e.len);
    if( e.mode==1 ) {
      cx.rc.raw_get(dst.data(), (size_t)e.len);
      cx.rc.init_dec(rcpin_keep());
      dd_retain(dst);
      ndec++; raw_total += e.len; nents++;
      return 1;
    }
    {
      if( (e.mode==2) != (ep!=0) ) return *err = "external endpoint file mismatch", -1;
      memcpy(dst.data(), pend_hdr, pend_hdr_n);            // recovered by dec_peek
      const dds_info& di = pend_di;
      cx.epx = const_cast<ByteBuf*>(ep); cx.epx_pos = 0; cx.epx_fail = false;
      cx.fmt = di.fmt;
      texture_init(cx); texture_reserve(cx, di);
      size_t off = di.data_off;
      for( u32 fc2=0; fc2<di.faces; fc2++ ) {
      texture_init(cx);
      for( u32 l=0; l<di.levels; l++ ) {
        u32 w,h,bw,bh; level_dims(di, l, w, h, bw, bh);
        size_t bytes = size_t(bw)*bh*di.bsize;
        if( !di.truncated && off + bytes*level_depth(di, l) > (size_t)e.len )
          { cx.epx = 0; return *err = "level past end", -1; }
        for( u32 z=0; z<level_depth(di, l); z++ ) {
          /* Truncated entry: code whole BLOCKS of the level that does not fit,
           * then stop.  The level keeps its true bw/bh so every context still
           * sees the real geometry; blk_lim only ends the scan early.  The
           * partial block and everything after go to code_tail below. */
          u32 nblk_ = bw*bh;
          if( di.truncated ) {
            const size_t left = off < (size_t)e.len ? (size_t)e.len - off : 0;
            if( bytes > left ) {
              nblk_ = u32(left / di.bsize);
              if( !nblk_ ) { l = di.levels; fc2 = di.faces; break; }
            }
          }
          cx.blk_lim = (nblk_ == bw*bh) ? 0 : nblk_;
          if( di.fmt==FMT_RAW ) cx.code_raster(dst.data() + off, w, h, di.bsize);
          else if( di.fmt==FMT_MODE ) cx.code_mode_blocks(dst.data() + off, bw, bh, di.bsize);
          else { cx.level_init(bw, bh); cx.code_level(dst.data() + off); }
          cx.blk_lim = 0;
          off += size_t(nblk_)*di.bsize;
          if( nblk_ != bw*bh ) { l = di.levels; fc2 = di.faces; break; }
        }
      }
      }
      if( off < (size_t)e.len ) cx.code_tail(dst.data() + off, (size_t)e.len - off);
      bool bad = cx.epx_fail; cx.epx = 0;
      if( bad ) return *err = "external endpoint file too short", -1;
    }
    if( crc_of(dst.data(), dst.size()) != e.crc ) return *err = "crc mismatch", -1;
    dd_retain(dst);
    first_entry = false;
    raw_total += e.len; nents++;
    ndec++;
    return 1;
  }

  /* The decoder's half of the dedup contract: every entry that was CODED gets
     remembered, so a later reference has something to point at.  enc_add
     inserts at the same point in the entry sequence -- before its body rather
     than after, which is the only place it can, and which nothing in between
     observes.  The slot's `pos` stays 0 here: it is only ever printed, never
     compared, so the two sides are free to disagree about it. */
  void dd_retain( const ByteBuf& b ) {
    if( !dd.on ) return;
    dd.insert(hash128(b.data(), b.size()), b.data(), b.size(), 0, 0);
  }

  // convenience for callers that never use external endpoints
  int dec_next( ByteBuf& dst, const char** err ) {
    int r = dec_peek(err);
    if( r<=0 ) return r;
    return dec_body(dst, err, 0);
  }
};
