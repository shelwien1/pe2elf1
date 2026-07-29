/* xad_wav.inc -- part of xadpcm.cpp; not standalone.
 *
 *  the RIFF layer and what a segment is: wav parsing, the canonical header
 * ADPCM-XQ writes, Params, and analyze(). */

static const int MAX_CHANS = 8;
static const int CANON_HEAD = 60; // riff 12 + fmt 8+20 + fact 8+4 + data 8

// the MS fmt chunk is 18 + wSamplesPerBlock + wNumCoef + the coefficient pairs
static INLINE int canon_head_ms(int ncoef) {
  return 62+4*ncoef;     // 90 for the usual seven
}

struct Wav {
  const u8* fmt = nullptr, * fact = nullptr;
  u32 fmt_size = 0, fact_size = 0;
  size_t data_off = 0, data_size = 0;
};

static bool parse_wav(cu8P f, size_t n, Wav &w) {
  if( n<12||memcmp(f, "RIFF", 4)||memcmp(f+8, "WAVE", 4) )
    return false;
  for( size_t pos = 12; pos+8<=n; ) {
    const u8* id = f+pos;
    u32 size = rd32(f+pos+4);
    size_t body = pos+8;
    u32 avail = u32(body+size<=n ? size : n-body);
    if( !memcmp(id, "fmt ", 4) ) {
      w.fmt = f+body;
      w.fmt_size = avail;
    } else if( !memcmp(id, "fact", 4) ) {
      w.fact = f+body;
      w.fact_size = avail;
    } else if( !memcmp(id, "data", 4) ) {
      w.data_off = body;
      w.data_size = avail;
      return true;
    }
    pos = body+size+(size&1);
  }
  return false;
}

// the header ADPCM-XQ writes; when it matches, it costs a flag instead of 60 bytes
static void build_canon_head(u8P h, int chans, int bps, u32 rate, int ba, int spb, u32 nsamples, u32 dsize) {
  memcpy(h, "RIFF", 4);
  wr32(h+4, 52+dsize);
  memcpy(h+8, "WAVE", 4);
  memcpy(h+12, "fmt ", 4);
  wr32(h+16, 20);
  wr16(h+20, 0x11);
  wr16(h+22, u32(chans));
  wr32(h+24, rate);
  wr32(h+28, rate*u32(ba)/u32(spb));
  wr16(h+32, u32(ba));
  wr16(h+34, u32(bps));
  wr16(h+36, 2);
  wr16(h+38, u32(spb));
  memcpy(h+40, "fact", 4);
  wr32(h+44, 4);
  wr32(h+48, nsamples);
  memcpy(h+52, "data", 4);
  wr32(h+56, dsize);
}

// the same for MS: fixed layout, only the coefficient table varies in length
static void build_canon_head_ms(u8P h, int chans, u32 rate, int ba, int spb, u32 nsamples, u32 dsize, const i32 (*coef)[2], int ncoef) {
  u32 fmtsz = 22+4*u32(ncoef);
  int at = 40;
  memcpy(h, "RIFF", 4);
  wr32(h+4, u32(canon_head_ms(ncoef)-8)+dsize);
  memcpy(h+8, "WAVE", 4);
  memcpy(h+12, "fmt ", 4);
  wr32(h+16, fmtsz);
  wr16(h+20, 0x02);
  wr16(h+22, u32(chans));
  wr32(h+24, rate);
  wr32(h+28, spb>0 ? rate*u32(ba)/u32(spb) : 0);
  wr16(h+32, u32(ba));
  wr16(h+34, 4);
  wr16(h+36, fmtsz-18);     // cbSize
  wr16(h+38, u32(spb));
  wr16(h+at, u32(ncoef));
  at += 2;
  for( int i = 0; i<ncoef; i++, at += 4 ) {
    wr16(h+at, u32(coef[i][0])&0xFFFF);
    wr16(h+at+2, u32(coef[i][1])&0xFFFF);
  }
  memcpy(h+at, "fact", 4);
  wr32(h+at+4, 4);
  wr32(h+at+8, nsamples);
  memcpy(h+at+12, "data", 4);
  wr32(h+at+16, dsize);
}

enum { FMT_IMA = 0, FMT_MS = 1 };

/* How much a wav inherits from the one before it.
   OFF is the default because carrying statistics is only a win when the next
   file resembles the last: an adaptive model primed on unrelated material is
   worse than one primed on nothing, and there is no way to tell from inside
   which case a given archive is.  SAME is the safe middle -- share only along a
   run of wavs of the same codec and code width, where a context row means the
   same thing on both sides of the boundary.  ALL shares regardless and is for
   when the caller knows the material is homogeneous. */
enum { SOLID_OFF = 0, SOLID_SAME = 1, SOLID_ALL = 2 };

struct Params {
  int format = FMT_IMA;
  int chans = 1, bps = 4, block_align = 0, canonical = 0;
  int ncoef = 0, spb = 0; // MS only
  i32 coef[MS_MAXCOEF][2] = {};
  u32 rate = 0, crc = 0;
  i64 factdiff = 0; // nblk*spb - the fact chunk's count
  size_t data_size = 0, head_size = 0, tail_size = 0, orig_size = 0;

  int hdr_bytes() const {
    return format==FMT_MS ? 7*chans : 4*chans;
  }
};

// samples a full block of the given geometry decodes to
static int block_samples(const Params &pm) {
  if( pm.format==FMT_MS )
    return (pm.block_align-7*pm.chans)*2/pm.chans+2;
  return adpcm_block_size_to_sample_count(pm.block_align, pm.chans, pm.bps);
}

/* Analyze one wav living at base[0 .. extent).  `extent` is where the NEXT wav
   starts (or the end of the stream), so head+data+tail always covers it
   exactly and nothing can fall between two segments. */
static bool analyze(const u8* base, size_t extent, const char* name, Params &pm, Wav &w, bool loud) {
#define AFAIL(...)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
        do {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
          if( loud )                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
          fprintf(stderr, __VA_ARGS__);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
          return false;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
        } while( 0 )
  if( !parse_wav(base, extent, w) )
    AFAIL("error: '%s' is not a RIFF/WAVE file with a data chunk\n", name);
  if( !w.fmt||w.fmt_size<16 )
    AFAIL("error: '%s': missing or short fmt chunk\n", name);
  u32 tag = rd16(w.fmt);
  if( tag==0x11 )
    pm.format = FMT_IMA;
  else if( tag==0x02 )
    pm.format = FMT_MS;
  else
    AFAIL("error: '%s' is neither IMA-ADPCM (0x11) nor MS-ADPCM (0x02)\n", name);

  pm.chans = int(rd16(w.fmt+2));
  pm.rate = rd32(w.fmt+4);
  pm.block_align = int(rd16(w.fmt+12));
  pm.bps = int(rd16(w.fmt+14));
  pm.spb = int(w.fmt_size>=20 ? rd16(w.fmt+18) : 0);
  pm.data_size = w.data_size;
  pm.head_size = w.data_off;
  pm.tail_size = extent-(w.data_off+w.data_size);
  pm.orig_size = extent;

  if( pm.format==FMT_MS ) {
    if( pm.chans<1||pm.chans>2 )
      AFAIL("error: MS-ADPCM is mono or stereo only, got %d channels\n", pm.chans);
    if( pm.bps!=4 )
      AFAIL("error: unsupported MS-ADPCM code width %d (need 4 bits)\n", pm.bps);
    if( pm.block_align<7*pm.chans )
      AFAIL("error: block size %d smaller than the block header\n", pm.block_align);
    // the coefficient table; a file without a usable one still codes fine, the
    // standard set just drives the prediction instead
    pm.ncoef = int(w.fmt_size>=22 ? rd16(w.fmt+20) : 0);
    if( pm.ncoef<1||pm.ncoef>MS_MAXCOEF||w.fmt_size<22+4*u32(pm.ncoef) )
      pm.ncoef = 0;
    if( pm.ncoef ) {
      for( int i = 0; i<pm.ncoef; i++ ) {
        pm.coef[i][0] = int16_t(rd16(w.fmt+22+4*i));
        pm.coef[i][1] = int16_t(rd16(w.fmt+24+4*i));
      }
    } else {
      pm.ncoef = 7;
      for( int i = 0; i<7; i++ ) {
        pm.coef[i][0] = ms_std_coef[i][0];
        pm.coef[i][1] = ms_std_coef[i][1];
      }
    }
  } else {
    if( pm.chans<1||pm.chans>MAX_CHANS )
      AFAIL("error: unsupported channel count %d\n", pm.chans);
    if( pm.bps<2||pm.bps>5 )
      AFAIL("error: unsupported code width %d (need 2-5 bits)\n", pm.bps);
    if( pm.block_align<pm.chans*4 )
      AFAIL("error: block size %d smaller than the block header\n", pm.block_align);
  }
#undef AFAIL

  // can the RIFF header be regenerated instead of stored?
  int dspb = block_samples(pm);
  u32 nblk = u32((pm.data_size+size_t(pm.block_align)-1)/size_t(pm.block_align));
  u32 fs = (w.fact&&w.fact_size>=4) ? rd32(w.fact) : 0;
  pm.canonical = 0;
  if( w.fact&&w.fact_size==4&&pm.spb>0&&pm.spb==dspb ) {
    u8 probe[64+4*MS_MAXCOEF];
    size_t cs;
    if( pm.format==FMT_MS ) {
      cs = size_t(canon_head_ms(pm.ncoef));
      build_canon_head_ms(probe, pm.chans, pm.rate, pm.block_align, pm.spb, fs, u32(pm.data_size), pm.coef, pm.ncoef);
    } else {
      cs = CANON_HEAD;
      build_canon_head(probe, pm.chans, pm.bps, pm.rate, pm.block_align, pm.spb, fs, u32(pm.data_size));
    }
    if( pm.head_size==cs&&!memcmp(probe, base, cs) ) {
      pm.canonical = 1;
      pm.factdiff = i64(nblk)*i64(dspb)-i64(fs);
    }
  }
  return true;
}
