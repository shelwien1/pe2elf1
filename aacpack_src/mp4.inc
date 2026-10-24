// MP4 files (ISO base media, and QuickTime's), for the compressor: where
// their AAC tracks' access units are, and each track's
// AudioSpecificConfig, from the moov box (Mp4Index::parse()) and, in a
// fragmented file, the moof boxes (Mp4Index::moof()). The scanner
// (cmstream.inc) codes the file's boxes as bytes and each access unit in
// it as a unit of its own (U_SAMPLE), under its track's config (U_ASC
// where that changes); the decompressor needs none of this. The moov box
// is parsed where it comes, if it comes before the mdat, or read ahead by
// the compressor's driver from a file (mp4_moov_ahead()).

namespace aacpack {

static const int kMp4Tracks = 16;                       // AAC tracks, at most
static const size_t kMp4Samples = (size_t)1 << 22;      // access units, at most (the rest are bytes)
static const size_t kMp4AuMax = (size_t)1 << 20;        // an access unit, at most (cmstream.inc's kAuMax)
static const uint64_t kMoovMax = (uint64_t)64 << 20;    // a moov box parsed, at most
static const size_t kMp4AscMax = 64;                    // an AudioSpecificConfig, at most
static const uint64_t kMp4Work = (uint64_t)1 << 26;     // samples gone through in the boxes' tables, at most

static inline uint32_t be32(const uint8_t* p) {
  return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}
static inline uint64_t be64(const uint8_t* p) { return (uint64_t)be32(p) << 32 | be32(p + 4); }
static constexpr uint32_t fourcc(const char* s) {
  return (uint32_t)(uint8_t)s[0] << 24 | (uint32_t)(uint8_t)s[1] << 16 | (uint32_t)(uint8_t)s[2] << 8 | (uint8_t)s[3];
}

// The header of the box at p (avail bytes there): its length n (0: to the
// end of the file), its type t and the header's length h; false if there
// is none
static bool mp4_head(const uint8_t* p, size_t avail, uint64_t& n, uint32_t& t, size_t& h) {
  if (avail < 8) return false;
  n = be32(p);
  t = be32(p + 4);
  h = 8;
  if (n == 1) {
    if (avail < 16) return false;
    n = be64(p + 8);
    h = 16;
  }
  return n == 0 || n >= h;
}
// whether a file starts with the box at p: one of the types a file starts with
static bool mp4_start(const uint8_t* p, size_t avail) {
  uint64_t n;
  uint32_t t;
  size_t h;
  if (!mp4_head(p, avail, n, t, h) || !n) return false;
  static const uint32_t kFirst[] = {fourcc("ftyp"), fourcc("styp"), fourcc("moov"), fourcc("mdat"),
                                    fourcc("free"), fourcc("skip"), fourcc("wide")};
  for (uint32_t f : kFirst)
    if (t == f) return true;
  return false;
}

// A box inside a parsed one: the payload [p, e) of the next box in [q, end)
struct Mp4Box {
  const uint8_t *p, *e;
  uint32_t t;
};
static bool mp4_next(const uint8_t*& q, const uint8_t* end, Mp4Box& b) {
  uint64_t n;
  size_t h;
  if (!mp4_head(q, (size_t)(end - q), n, b.t, h)) return false;
  if (!n) n = (uint64_t)(end - q);
  if (n > (uint64_t)(end - q)) return false;
  b.p = q + h;
  b.e = q + n;
  q = b.e;
  return true;
}
// the first box of the type t in [p, e), its payload in b
static bool mp4_find(const uint8_t* p, const uint8_t* e, uint32_t t, Mp4Box& b) {
  while (mp4_next(p, e, b))
    if (b.t == t) return true;
  return false;
}

// an MPEG-4 descriptor at p (before e): its tag, its payload [*dp, *de)
static bool mp4_descr(const uint8_t* p, const uint8_t* e, int& tag, const uint8_t*& dp, const uint8_t*& de) {
  if (p >= e) return false;
  tag = *p++;
  uint64_t n = 0;
  for (int i = 0; i < 4; i++) {
    if (p >= e) return false;
    int b = *p++;
    n = n << 7 | (b & 0x7f);
    if (!(b & 0x80)) break;
  }
  if (n > (uint64_t)(e - p)) return false;
  dp = p;
  de = p + n;
  return true;
}
// the DecoderSpecificInfo of an esds box's payload [p, e): the AudioSpecificConfig
static bool mp4_esds(const uint8_t* p, const uint8_t* e, const uint8_t*& ap, const uint8_t*& ae) {
  int tag;
  const uint8_t *q, *qe;
  if (e - p < 4 || !mp4_descr(p + 4, e, tag, q, qe) || tag != 3 || qe - q < 3) return false;
  int fl = q[2];
  size_t k = 3;
  if (fl & 0x80) k += 2;                                         // dependsOn_ES_ID
  if ((fl & 0x40) && k < (size_t)(qe - q)) k += 1 + (size_t)q[k];  // URL
  if (fl & 0x20) k += 2;                                         // OCR_ES_Id
  if (k >= (size_t)(qe - q)) return false;
  q += k;
  while (q < qe) {
    const uint8_t *dp, *de;
    if (!mp4_descr(q, qe, tag, dp, de)) return false;
    if (tag == 4 && de - dp >= 13) {  // DecoderConfigDescriptor
      const uint8_t* r = dp + 13;
      while (r < de) {
        int t2;
        const uint8_t *sp, *se;
        if (!mp4_descr(r, de, t2, sp, se)) return false;
        if (t2 == 5) {
          ap = sp, ae = se;
          return true;
        }
        r = se;
      }
    }
    q = de;
  }
  return false;
}

struct Mp4Sample {
  uint64_t at;     // its place in the file
  uint32_t n;      // its length
  uint32_t track;  // its track (Mp4Index::asc)
};

// What the compressor knows of an MP4 file's AAC tracks
struct Mp4Index {
  Buf<Mp4Sample> s;  // the access units, by place
  int ntracks;
  uint8_t asc[kMp4Tracks][kMp4AscMax + 16];  // each track's AudioSpecificConfig (16 zeros after it)
  size_t asc_n[kMp4Tracks];
  uint32_t id[kMp4Tracks], sdi[kMp4Tracks];  // its track_ID, the index of its sample entry (1 on)
  uint32_t dsize[kMp4Tracks];                // the default sample size of its fragments (trex)
  uint64_t work;      // samples gone through in the tables (kMp4Work)
  bool have;          // a moov box parsed
  uint64_t moof_end;  // where the last moof box parsed ends
  size_t next;        // the scan's next access unit
  int cur;            // and the track whose config the walk has (-1 none)

  void Init() {
    s.Init();
    ntracks = 0;
    memset(asc, 0, sizeof asc);
    memset(asc_n, 0, sizeof asc_n);
    memset(id, 0, sizeof id);
    memset(sdi, 0, sizeof sdi);
    memset(dsize, 0, sizeof dsize);
    work = 0;
    have = false;
    moof_end = 0;
    next = 0;
    cur = -1;
  }
  void Quit() { s.Quit(); }

  // the moov box's payload [p, p + n): its AAC tracks, and their defaults
  // for fragments (mvex)
  void parse(const uint8_t* p, size_t n) {
    have = true;
    const uint8_t* e = p + n;
    Mp4Box b, tx;
    for (const uint8_t* q = p; mp4_next(q, e, b);)
      if (b.t == fourcc("trak") && ntracks < kMp4Tracks) track(b.p, b.e);
    if (mp4_find(p, e, fourcc("mvex"), b))
      for (const uint8_t* q = b.p; mp4_next(q, b.e, tx);) {
        if (tx.t != fourcc("trex") || tx.e - tx.p < 24) continue;
        int t = track_of(be32(tx.p + 4));
        if (t >= 0) dsize[t] = be32(tx.p + 16);
      }
    sort(0);
  }
  int track_of(uint32_t track_id) const {
    for (int t = 0; t < ntracks; t++)
      if (id[t] == track_id) return t;
    return -1;
  }

  // a moof box of a fragmented file, at place at, its payload [p, e): the
  // access units of its AAC tracks' fragments (once each: where it is
  // after the last one parsed)
  void moof(uint64_t at, uint64_t end, const uint8_t* p, const uint8_t* e) {
    if (at < moof_end) return;
    moof_end = end;
    const size_t s0 = s.size();
    uint64_t data_end = at;  // (where the last track fragment's data ended)
    Mp4Box tf, b;
    for (const uint8_t* q = p; mp4_next(q, e, tf);) {
      if (tf.t != fourcc("traf") || !mp4_find(tf.p, tf.e, fourcc("tfhd"), b) || b.e - b.p < 8) continue;
      const uint32_t fl = be32(b.p) & 0xffffff;
      const int t = track_of(be32(b.p + 4));
      const uint8_t* r = b.p + 8;
      uint64_t base = data_end;
      uint32_t d = t >= 0 ? sdi[t] : 0, dsz = t >= 0 ? dsize[t] : 0;
      // (the fields there are: what is not there reads as 0)
      auto field = [&](int bytes) -> uint64_t {
        uint64_t v = r + bytes <= b.e ? (bytes == 8 ? be64(r) : be32(r)) : 0;
        r = r + bytes <= b.e ? r + bytes : b.e;
        return v;
      };
      if (fl & 0x1)
        base = field(8);
      else if ((fl & 0x20000) || data_end == at)
        base = at;  // (default-base-is-moof; the first fragment's)
      if (fl & 0x2) d = (uint32_t)field(4);
      if (fl & 0x8) field(4);
      if (fl & 0x10) dsz = (uint32_t)field(4);
      uint64_t pos = base;
      Mp4Box tr;
      for (const uint8_t* q2 = tf.p; mp4_next(q2, tf.e, tr);) {
        if (tr.t != fourcc("trun") || tr.e - tr.p < 8) continue;
        const uint32_t f2 = be32(tr.p) & 0xffffff, n = be32(tr.p + 4);
        const uint8_t* x = tr.p + 8;
        if (f2 & 0x1) {
          if (x + 4 > tr.e) break;
          pos = base + (uint64_t)(int64_t)(int32_t)be32(x);
          x += 4;
        }
        if (f2 & 0x4) x = x + 4 <= tr.e ? x + 4 : tr.e;
        const size_t per = 4 * (size_t)(((f2 >> 8) & 1) + ((f2 >> 9) & 1) + ((f2 >> 10) & 1) + ((f2 >> 11) & 1));
        if ((uint64_t)n * per > (uint64_t)(tr.e - x)) break;
        for (uint32_t k = 0; k < n && ++work <= kMp4Work; k++, x += per) {
          uint32_t sz = dsz;
          if (f2 & 0x200) sz = be32(x + ((f2 & 0x100) ? 4 : 0));
          if (t >= 0 && d == sdi[t] && sz && sz <= kMp4AuMax && s.size() < kMp4Samples) {
            Mp4Sample u;
            u.at = pos;
            u.n = sz;
            u.track = (uint32_t)t;
            s.push_back(u);
          }
          pos += sz;
        }
      }
      data_end = pos;
    }
    sort(s0);
  }

  // a trak box's payload: its access units, if it is AAC (else nothing)
  void track(const uint8_t* p, const uint8_t* e) {
    Mp4Box mdia, hdlr, minf, stbl, stsd, sz, co, sc;
    if (!mp4_find(p, e, fourcc("mdia"), mdia)) return;
    if (!mp4_find(mdia.p, mdia.e, fourcc("hdlr"), hdlr) || hdlr.e - hdlr.p < 12 || be32(hdlr.p + 8) != fourcc("soun"))
      return;
    if (!mp4_find(mdia.p, mdia.e, fourcc("minf"), minf) || !mp4_find(minf.p, minf.e, fourcc("stbl"), stbl)) return;
    if (!mp4_find(stbl.p, stbl.e, fourcc("stsd"), stsd) || stsd.e - stsd.p < 8) return;
    // the first mp4a sample entry with an AudioSpecificConfig: its index (1 on) and the config
    uint32_t entry = 0;
    const uint8_t *ap = nullptr, *ae = nullptr;
    {
      const uint8_t* q = stsd.p + 8;
      Mp4Box en;
      for (uint32_t i = 1; !entry && mp4_next(q, stsd.e, en); i++) {
        if (en.t != fourcc("mp4a") || en.e - en.p < 28) continue;
        int ver = en.p[8] << 8 | en.p[9];  // (QuickTime's sound description version)
        const uint8_t* c = en.p + (ver == 1 ? 44 : ver == 2 ? 64 : 28);
        if (c > en.e) continue;
        Mp4Box es, wave;
        bool ok = mp4_find(c, en.e, fourcc("esds"), es);
        if (!ok && mp4_find(c, en.e, fourcc("wave"), wave)) ok = mp4_find(wave.p, wave.e, fourcc("esds"), es);
        if (ok && mp4_esds(es.p, es.e, ap, ae) && ae - ap >= 2 && (size_t)(ae - ap) <= kMp4AscMax) entry = i;
      }
    }
    if (!entry) return;
    AscInfo ai;
    const int t = ntracks;
    memset(asc[t], 0, sizeof asc[t]);
    memcpy(asc[t], ap, (size_t)(ae - ap));
    if (!asc_parses(asc[t], 0, (size_t)(ae - ap) * 8, false, ai) || !ai.raw_blocks) return;
    asc_n[t] = (size_t)(ae - ap);
    Mp4Box tkhd;
    id[t] = mp4_find(p, e, fourcc("tkhd"), tkhd) && tkhd.e - tkhd.p >= 24 ? be32(tkhd.p + (tkhd.p[0] == 1 ? 20 : 12)) : 0;
    sdi[t] = entry;
    dsize[t] = 0;
    // the sample table: sizes, chunk offsets, samples per chunk
    uint32_t fixed = 0, count = 0;
    const uint8_t* szs = nullptr;
    int field = 32;
    ntracks++;  // (a track of no samples: a fragmented file's)
    if (mp4_find(stbl.p, stbl.e, fourcc("stsz"), sz) && sz.e - sz.p >= 12) {
      fixed = be32(sz.p + 4);
      count = be32(sz.p + 8);
      szs = sz.p + 12;
      if (!fixed && (uint64_t)count * 4 > (uint64_t)(sz.e - szs)) return;
    } else if (mp4_find(stbl.p, stbl.e, fourcc("stz2"), sz) && sz.e - sz.p >= 12) {
      field = sz.p[7];
      count = be32(sz.p + 8);
      szs = sz.p + 12;
      if ((field != 4 && field != 8 && field != 16) || ((uint64_t)count * field + 7) / 8 > (uint64_t)(sz.e - szs)) return;
    } else {
      return;
    }
    bool wide = false;
    if (mp4_find(stbl.p, stbl.e, fourcc("stco"), co))
      ;
    else if (mp4_find(stbl.p, stbl.e, fourcc("co64"), co))
      wide = true;
    else
      return;
    if (co.e - co.p < 8 || !mp4_find(stbl.p, stbl.e, fourcc("stsc"), sc) || sc.e - sc.p < 8) return;
    const uint32_t nch = be32(co.p + 4), nsc = be32(sc.p + 4);
    if ((uint64_t)nch * (wide ? 8 : 4) > (uint64_t)(co.e - co.p - 8) || (uint64_t)nsc * 12 > (uint64_t)(sc.e - sc.p - 8))
      return;
    auto size_of = [&](uint32_t k) -> uint32_t {
      if (fixed) return fixed;
      if (field == 32) return be32(szs + 4 * (size_t)k);
      if (field == 16) return (uint32_t)szs[2 * (size_t)k] << 8 | szs[2 * (size_t)k + 1];
      if (field == 8) return szs[k];
      return k & 1 ? szs[k / 2] & 15 : szs[k / 2] >> 4;
    };
    uint32_t k = 0;  // the sample
    uint32_t run = 0;
    if (!nsc || be32(sc.p + 8) != 1) return;  // (the first run is the first chunk's)
    for (uint32_t ch = 1; ch <= nch && k < count && ++work <= kMp4Work; ch++) {
      while (run + 1 < nsc && be32(sc.p + 8 + 12 * (size_t)(run + 1)) <= ch) run++;
      const uint8_t* r = sc.p + 8 + 12 * (size_t)run;
      const uint32_t per = be32(r + 4), d = be32(r + 8);
      uint64_t at = wide ? be64(co.p + 8 + 8 * (size_t)(ch - 1)) : be32(co.p + 8 + 4 * (size_t)(ch - 1));
      for (uint32_t j = 0; j < per && k < count && ++work <= kMp4Work; j++, k++) {
        const uint32_t n = size_of(k);
        if (d == entry && n && n <= kMp4AuMax && s.size() < kMp4Samples) {
          Mp4Sample x;
          x.at = at;
          x.n = n;
          x.track = (uint32_t)t;
          s.push_back(x);
        }
        at += n;
      }
    }
  }

  // the access units from the i0-th on by place (the tracks' chunks
  // interleave); a merge sort, where they are not in order already
  void sort(size_t i0) {
    const size_t n = s.size() - i0;
    bool in_order = true;
    for (size_t i = i0 + 1; i < s.size() && in_order; i++) in_order = s[i - 1].at <= s[i].at;
    if (in_order) return;
    Buf<Mp4Sample> t;
    t.Init();
    t.resize(n);
    Mp4Sample *a = s.data() + i0, *b = t.data();
    for (size_t w = 1; w < n; w *= 2) {
      for (size_t lo = 0; lo < n; lo += 2 * w) {
        size_t mid = Min(lo + w, n), hi = Min(lo + 2 * w, n), i = lo, j = mid, o = lo;
        while (i < mid && j < hi) b[o++] = a[j].at < a[i].at ? a[j++] : a[i++];
        while (i < mid) b[o++] = a[i++];
        while (j < hi) b[o++] = a[j++];
      }
      Mp4Sample* x = a;
      a = b, b = x;
    }
    if (a != s.data() + i0) memcpy(s.data() + i0, a, n * sizeof *a);
    t.Quit();
  }
};

// The compressor's driver, before the run: the moov box of the MP4 file f,
// read ahead where it comes after an mdat box (and f is a file that can be
// read twice: not a pipe), into moov (empty: none); f is back at its start
#define MP4_TELL ftello
#define MP4_SEEK fseeko
#ifdef _WIN32
#undef MP4_TELL
#undef MP4_SEEK
#define MP4_TELL _ftelli64
#define MP4_SEEK _fseeki64
#endif
static void mp4_moov_ahead(FILE* f, uint64_t size, Buf<uint8_t>& moov) {
  if (!size || MP4_TELL(f) != 0) return;
  uint8_t h[16];
  uint64_t at = 0;
  bool mdat = false;
  for (int boxes = 0; at + 8 <= size && boxes < 1 << 16; boxes++) {
    if (MP4_SEEK(f, (int64_t)at, SEEK_SET) != 0) break;
    size_t got = fread(h, 1, sizeof h, f);
    uint64_t n;
    uint32_t t;
    size_t hl;
    if (!mp4_head(h, got, n, t, hl) || (!at && !mp4_start(h, got))) break;
    if (!n) n = size - at;
    if (n > size - at) break;
    if (t == fourcc("mdat")) mdat = true;
    if (t == fourcc("moov")) {
      // (one that comes first is parsed where it comes)
      if (mdat && n - hl <= kMoovMax && MP4_SEEK(f, (int64_t)(at + hl), SEEK_SET) == 0) {
        moov.resize((size_t)(n - hl));
        if (fread(moov.data(), 1, moov.size(), f) != moov.size()) moov.clear();
      }
      break;
    }
    at += n;
  }
  MP4_SEEK(f, 0, SEEK_SET);
}
#undef MP4_TELL
#undef MP4_SEEK

}  // namespace aacpack
