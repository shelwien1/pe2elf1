/* xad_arc.inc -- part of xadpcm.cpp; not standalone.
 *
 * the container: the segment table, the scan that finds every wav in a
 * stream, and the geometry the model tables are sized from. */
static const char MAGIC[4] = {'X', 'A', 'C', '1'};
static const u8 VERSION = 5;

/* Container records, coded INSIDE the rc stream rather than written ahead of it.
   Version 4 put a table of contents first -- nseg, every segment's geometry, the
   file table, the crc -- so the encoder could not emit a byte until it had
   scanned the whole input.  The blocker was not the count or the crc but
   tail_size: a segment's record spanned up to the NEXT wav's start, so it could
   not be written until the next wav had been found, which is an unbounded
   lookahead.  Making the bytes between two wavs their own T_LIT record removes
   it, and the count, the file table and the crc then have somewhere natural to
   go -- the end, where they are known.
   Everything else is unchanged: put_seg/get_seg emit the same bytes in the same
   order, they just go through the literal model now instead of raw. */
enum {
  T_END = 0, // varint nfile, the file table, u32 crc32 -- then the stream ends
  T_LIT = 1, // varint n, then n literal bytes
  T_SEG = 2, // Params, the head, then a chunk list: varint len (0 ends), bytes
};


struct Seg {
  Params pm;
  size_t off = 0; // where this wav's RIFF sits in the stream
};

struct Arc {
  int solid = SOLID_OFF;
  Buf<Seg> segs;
  size_t lead = 0; // bytes ahead of the first wav
  size_t total = 0;
  u32 crc = 0;
  NameTab names; // empty unless several inputs were given
  Buf<u64> sizes;
};

/* Find every wav in the stream.  A candidate is a RIFF/WAVE header that also
   parses as IMA or MS ADPCM with a usable geometry, which is a strong enough
   filter that audio data almost never trips it -- and when it does, the cost is
   only ratio: the decoder never repeats this scan, it reads the segment table
   out of the container, and each segment's head+data+tail covers its extent
   exactly, so a wav split down the middle still restores byte for byte. */
/* "Same type" is codec AND code width, not codec alone.  A 4-bit and a 2-bit
   IMA file share a format tag but not an alphabet: their tree nodes and context
   rows index differently, so statistics carried between them would describe the
   wrong thing.  Channel count and block size are deliberately left out -- those
   change what the contexts contain, not what they mean. */
static bool same_type(const Params &a, const Params &b) {
  return a.format==b.format&&a.bps==b.bps;
}

static void scan_segments(const Buf<u8> &buf, Arc &ar) {
  cu8P d = buf.data();
  size_t n = buf.size();
  Buf<size_t> cand;
  for( size_t i = 0; i+12<=n; ) {
    const void* hit = memchr(d+i, 'R', n-11-i);
    if( !hit )
      break;
    i = size_t((const u8*)hit-d);
    if( !memcmp(d+i, "RIFF", 4)&&!memcmp(d+i+8, "WAVE", 4) ) {
      Params pm;
      Wav w;
      if( analyze(d+i, n-i, "", pm, w, false) ) {
        cand.push_back(i);
        // skip past this wav's data; a nested RIFF inside coded audio is noise
        i += w.data_off+w.data_size;
        continue;
      }
    }
    i++;
  }
  ar.total = n;
  ar.lead = cand.empty() ? n : cand[0];
  for( size_t k = 0; k<cand.size(); k++ ) {
    Seg sg;
    sg.off = cand[k];
    size_t extent = (k+1<cand.size() ? cand[k+1] : n)-cand[k];
    Wav w;
    if( !analyze(d+sg.off, extent, "", sg.pm, w, false) )
      continue; // cannot happen: the same call succeeded above
    ar.segs.push_back(sg);
  }
}

/* put_seg / get_seg moved into Xad<MD> below: they are now pull/push over the
   coroutine pins rather than index arithmetic over a fully-loaded buffer.  The
   field order and encoding are unchanged, so the header bytes are identical. */

static void write_canon(u8P dst, const Params &pm) {
  int spb = block_samples(pm);
  u32 nblk = u32((pm.data_size+size_t(pm.block_align)-1)/size_t(pm.block_align));
  u32 fs = u32(i64(nblk)*i64(spb)-pm.factdiff);
  if( pm.format==FMT_MS )
    build_canon_head_ms(dst, pm.chans, pm.rate, pm.block_align, spb, fs, u32(pm.data_size), pm.coef, pm.ncoef);
  else
    build_canon_head(dst, pm.chans, pm.bps, pm.rate, pm.block_align, spb, fs, u32(pm.data_size));
}

// widest geometry any segment needs; the tables are allocated once from this
static void arc_geometry(const Arc &ar, int &maxsym, int &maxnib) {
  maxsym = 16;
  maxnib = 1;
  for( size_t i = 0; i<ar.segs.size(); i++ ) {
    const Params &p = ar.segs[i].pm;
    int ns = 1<<p.bps, nb = p.format==FMT_MS ? M0_Md_Volume : I0_Ib_Volume;
    if( ns>maxsym )
      maxsym = ns;
    if( nb>maxnib )
      maxnib = nb;
  }
  if( ar.segs.size()==1 )
    maxsym = 1<<ar.segs[0].pm.bps;   // a lone wav is sized exactly as it always was
}
