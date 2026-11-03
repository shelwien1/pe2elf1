//  oc_mt.inc -- `oggcomp c -t N`, and `d` of what it writes: the input in
//  parts, coded on N threads by MTwrap (MTwrap_v3/, m4vpack's).
//
//  OcSplit, a coroutine on the main thread, cuts the input into parts
//  where oggsplit (../oggsplit) cuts, so that each part is a stream the
//  model codes as it would code it in the whole file:
//    - before a link (of a chained file) but the first, where nothing
//      changes;
//    - inside a link that is one Vorbis stream, its three header packets on
//      pages of their own, before a page that starts with a new packet,
//      after an audio page that ends with a complete packet.  The part gets
//      the stream's header packets inserted at its start, on pages of their
//      own (oggsplit's: the comment header keeps the vendor string but not
//      the comments), numbered to lead into the page at the cut, so the
//      model can parse the audio after them.
//  A part ends at the first such place at least the part size (-c) after
//  its start.  Where none comes -- a stream that is not Vorbis, several
//  streams in one link, bytes that are not pages -- it ends anyway at
//  MT_HARD part sizes: before the page there (with the header packets, if
//  it is inside a Vorbis stream whose headers are in), or in the run of
//  junk there.  The model codes whatever it is given, so any cut is
//  lossless; these are only the cuts it codes as well as the whole.
//
//  MTwrap deals the parts to the workers in turn, as frames
//  [u64 L][u64 xlen][the inserted pages, xlen bytes][the part], L counting
//  what follows it.  A worker (OcPart) codes its parts one after the other
//  with one model -- solid, OC_F_SOLID after its first part, as `oggdet -S`
//  codes its segments -- so a part starts with what the worker's earlier
//  parts taught the model, and its inserted headers, seen before, cost a
//  symbol each.  Its stream holds, for each part:
//    u64 plen   the part's size as coded (with the inserted pages; not 0)
//    u64 xlen   the inserted pages' bytes, at its start; removed after d
//    u64 part   its place in the input, from 0: worker w's k-th is w + kN
//    u32 crc    the CRC (Ogg's) of the 24 bytes before it; u32 0
//    then the part as coded -- the API's stream, what `oggcomp c` writes
//    after its header, which ends in the CRC of the part -- in chunks of a
//    varint length and that many bytes, and a varint 0 after the last: the
//    decoder reads ahead of where the coder stopped (rc.inc's flush leaves
//    off the 0xFF bytes it would read past the end), so the part has to
//    end where its stream ends, not where the next record starts.
//  and at its end a record of plen 0 (xlen 0), whose part is the place its
//  next part would have had.  So d finds a part lost, added or moved, or a
//  worker's stream cut short.  Each worker writes its stream as it codes,
//  64 KB at a time, and MTwrap's container holds the N streams interleaved
//  as they were made, after the header:
//    "oggm\x1a", the stream version, the model, the threads, the part size
//    in KB (4 bytes, little-endian)
//  d needs as many threads as c had, and takes the number from the header;
//  each part ends a round of MTwrap's decoder, which takes the parts from
//  the workers in turn, as they were dealt.

#define AlignUp(x, r) ((x) + ((r)-1)) / (r) * (r)  // (MTwrap's)
//  Where dyn_buff, the decoder's store of blocks a worker cannot take yet,
//  runs out of memory -- a container whose interleaving parks a worker's
//  whole stream, decoded under a tight limit -- it calls this, and without
//  it __debugbreak(): SIGILL, where the promise is exit 3.
#define MTWRAP_NOMEM() FATAL_CODE(OGC_EXIT_IO, "out of memory for the blocks of a run on threads")
//  MTwrap as m4vpack has it, its warnings under -Wall -Wextra left to it
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-label"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wuninitialized"
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#endif

#include "MTwrap_v3/MTwrap.inc"
#include "Lib3/coro2pair.inc"
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

constexpr char OC_MT_MAGIC[] = "oggm\x1a";
enum { MT_AT_VERSION = 5, MT_AT_MODEL = 6, MT_AT_THREADS = 7, MT_AT_PART = 8, MT_HEAD = 12 };
constexpr int OC_MT_MAX = 64;         // threads: MTwrap's container holds 64 workers' streams
constexpr u32 MT_RING = 1u << 20;     // MTwrap's buffers to and from each worker
constexpr u64 MT_HARD = 8;            // where no cut comes, a part ends at this many part sizes
constexpr u64 MT_PART_DEFAULT = (u64)1 << 20;  // -c's default where the input's size is not known
constexpr u64 MT_PART_MIN = (u64)64 << 10;     // the least the default goes to
constexpr u64 MT_PART_MAX = (u64)4 << 20;      // and the most
constexpr u64 MT_ROUNDS = 4;                   // the parts the default gives each thread
enum { MT_R_PLEN = 0, MT_R_XLEN = 8, MT_R_PART = 16, MT_R_CRC = 24, MT_R_ZERO = 28, MT_REC = 32 };
enum { MT_F_LEN = 0, MT_F_XLEN = 8, MT_FRAME = 16 };
//  MTwrap's yields, as its thread_pipe takes them from a worker
enum { Y_END = 0, Y_IN = 1, Y_OUT = 2, Y_FAIL = 3, Y_PART = 4 };

static void mt_put_le(u8 *p, u64 v, int n) {
  int i;
  for(i = 0; i < n; i++)
    p[i] = (u8)(v >> (8 * i));
}
static u64 mt_get_le(const u8 *p, int n) {
  u64 v = 0;
  int i;
  for(i = n - 1; i >= 0; i--)
    v = (v << 8) | p[i];
  return v;
}
static u32 mt_rec_crc(const u8 *r) {
  crcinit();
  return crcrun(0, r, MT_R_CRC);
}
static void mt_rec(u8 *r, u64 plen, u64 xlen, u64 part) {
  mt_put_le(r + MT_R_PLEN, plen, 8);
  mt_put_le(r + MT_R_XLEN, xlen, 8);
  mt_put_le(r + MT_R_PART, part, 8);
  mt_put_le(r + MT_R_CRC, mt_rec_crc(r), 4);
  mt_put_le(r + MT_R_ZERO, 0, 4);
}
//  The most bytes the header pages of a part can take: the identification
//  header, the comment header's vendor string (64 KB at most, below), the
//  setup header (1 MB at most), and their pages' headers.
constexpr u64 MT_INS_MAX = (u64)2 << 20;
constexpr sz MT_MAXPAGE = 27 + 255 + 255 * 255;  // the longest page
//  The most a part can be, as the splitter cuts it: MT_HARD part sizes, a
//  page past them, and its header pages.  d takes a record of a larger part
//  to be broken, which bounds the memory a part's output takes.
static u64 mt_part_max(u64 part) { return MT_HARD * part + MT_MAXPAGE + MT_INS_MAX; }
//  -c's default for an input of total bytes on n threads (0: not known):
//  MT_ROUNDS parts for each thread, so that one whose parts code slower
//  keeps the others waiting little at the end -- MTwrap deals the parts in
//  turn, and the threads' last round is as full as the parts are even --
//  of 64 KB to 4 MB.  (On a 5.7 MB file on 4 threads, parts of a part per
//  thread and a half took 7.0 s, of four rounds 5.5 to 5.8 s.)
static u64 mt_part_default(u64 total, int n) {
  u64 p;
  if(!total)
    return MT_PART_DEFAULT;
  p = (total + MT_ROUNDS * (u64)n - 1) / (MT_ROUNDS * (u64)n);
  p = (p + 1023) & ~(u64)1023;
  return p < MT_PART_MIN ? MT_PART_MIN : p > MT_PART_MAX ? MT_PART_MAX : p;
}


//  ---------------------------------------------------------------------------
//  The splitter.  oggsplit's analysis of the pages, one at a time as they
//  come (oggsplit/oggsplit.cpp's Analyzer, but for the current link only:
//  a cut is decided where its page comes, so nothing else has to be kept).

constexpr sz MT_MAXHDR = (sz)1 << 20;   // the longest identification and setup headers copied
constexpr sz MT_MAXVENDOR = (sz)1 << 16;  // the longest vendor string copied
static int mt_page_len(const u8 *p, sz avail) {  // 0 when no page is at p
  sz n, i;
  if(avail < 27 || memcmp(p, "OggS", 4) || p[4])
    return 0;
  n = 27 + (sz)p[26];
  if(avail < n)
    return 0;
  for(i = 27; i < 27 + (sz)p[26]; i++)
    n += p[i];
  if(avail < n || !ogg_page::crc_ok(p, n))
    return 0;
  return (int)n;
}
//  A growable buffer of bytes, plain data.
struct mt_bytes {
  u8 *p;
  sz n, room;
  void init() {
    p = nullptr;
    n = room = 0;
  }
  void quit() {
    free(p);
    init();
  }
  int reserve(sz want) {
    if(want <= room)
      return 1;
    sz r = room ? room : 4096;
    while(r < want)
      r *= 2;
    u8 *q = (u8 *)realloc(p, r);
    if(!q)
      return 0;
    p = q;
    room = r;
    return 1;
  }
  int add(const void *b, sz k) {
    if(!reserve(n + k))
      return 0;
    memcpy(p + n, b, k);
    n += k;
    return 1;
  }
};
enum { MT_CUT_NONE, MT_CUT_LINK, MT_CUT_MID };
struct mt_link {  // the link under way
  u32 serial;
  int streams, eos, bos_run;
  int hdr;  // header packets of its stream seen, up to 3; -1: not a stream cut inside
  int open;
  mt_bytes pkt;  // the header packet under way
  sz pkt_len;
  mt_bytes ident, vendor, setup;
};
struct mt_analyzer {
  int have;  // a link has started
  mt_link l;
  int after;  // the page before the chunk under way is one a cut may come after
  void init() {
    have = 0;
    memset(&l, 0, sizeof l);
    l.pkt.init();
    l.ident.init();
    l.vendor.init();
    l.setup.init();
    after = 0;
  }
  void quit() {
    l.pkt.quit();
    l.ident.quit();
    l.vendor.quit();
    l.setup.quit();
  }
  void new_link(u32 serial) {
    mt_bytes pkt = l.pkt, ident = l.ident, vendor = l.vendor, setup = l.setup;
    memset(&l, 0, sizeof l);
    l.pkt = pkt;
    l.ident = ident;
    l.vendor = vendor;
    l.setup = setup;
    l.pkt.n = l.ident.n = l.vendor.n = l.setup.n = 0;
    l.serial = serial;
    l.streams = 1;
    l.bos_run = 1;
    have = 1;
  }
  sz cap() const { return l.hdr == 1 ? 11 + MT_MAXVENDOR : MT_MAXHDR + 1; }
  //  The header packet l.hdr has come in full.
  void header() {
    const mt_bytes &d = l.pkt;
    static const u8 sig[3] = {1, 3, 5};
    if(d.n < 7 || d.p[0] != sig[l.hdr] || memcmp(d.p + 1, "vorbis", 6)) {
      l.hdr = -1;
    } else if(l.hdr == 0) {  // the identification header, as ogg2tsv takes it
      int b0 = l.pkt_len == 30 ? d.p[28] & 15 : 0, b1 = l.pkt_len == 30 ? d.p[28] >> 4 : 0;
      if(l.pkt_len != 30 || rd32(d.p + 7) || !d.p[11] || !rd32(d.p + 12) || b0 < 6 || b1 < b0 || b1 > 13 || !l.ident.add(d.p, d.n))
        l.hdr = -1;
    } else if(l.hdr == 1) {  // the comment header: its vendor string, if it is all there
      if(d.n >= 11 && rd32(d.p + 7) <= d.n - 11 && !l.vendor.add(d.p + 11, rd32(d.p + 7)))
        l.hdr = -1;
    } else if(l.pkt_len > MT_MAXHDR || !l.setup.add(d.p, d.n)) {  // the setup header
      l.hdr = -1;
    }
    if(l.hdr >= 0)
      l.hdr++;
    l.pkt.n = 0;
    l.pkt_len = 0;
  }
  //  A page of the link's stream, its only one.
  void stream_page(const u8 *p) {
    const int nseg = p[26];
    const u8 *lace = p + 27, *body = lace + nseg;
    if(l.hdr >= 0 && l.hdr < 3) {
      if(!(p[5] & 1) != !l.open)
        l.hdr = -1;  // a packet cut off, or a continuation of none
      for(int s = 0, at = 0; s < nseg && l.hdr >= 0 && l.hdr < 3; at += lace[s++]) {
        sz k = lace[s], c = cap();
        if(l.pkt.n + k > c)
          k = l.pkt.n < c ? c - l.pkt.n : 0;
        if(k && !l.pkt.add(body + at, k)) {
          l.hdr = -1;
          break;
        }
        l.pkt_len += lace[s];
        if(lace[s] < 255) {
          header();
          if(l.hdr == 3 && s != nseg - 1)
            l.hdr = -1;  // the setup header doesn't end its page
        }
      }
    }
    if(nseg)
      l.open = lace[nseg - 1] == 255;
  }
  //  Junk: no cut right after it.
  void junk() { after = 0; }
  //  A page: whether a cut may go before it, and how; then the page.
  int page(const u8 *p) {
    const int ok = after, fl = p[5];
    const u32 serial = rd32(p + 14);
    int stray = 0, cut = MT_CUT_NONE;
    after = 0;
    if(fl & 2) {
      //  A link starts at a BOS page that comes first or after the EOS
      //  pages of all the streams of the last link; the BOS pages right
      //  after it start its other streams.
      if(!have || l.eos >= l.streams) {
        if(have)
          cut = MT_CUT_LINK;  // (what comes before the first link goes with it)
        new_link(serial);
      } else if(l.bos_run) {
        l.streams++;
      } else {
        stray = 1;
      }
    } else if(have) {
      l.bos_run = 0;
    }
    if(have && (fl & 4))
      l.eos++;
    if(have && l.streams == 1) {
      //  where ogg2tsv and a decoder could take the pages differently --
      //  another stream's page, a BOS page that starts nothing -- the link
      //  has no cut points from there on
      if(serial != l.serial || stray)
        l.hdr = -1;
      if(serial == l.serial) {
        const int audio = l.hdr == 3;
        if(ok && audio && !l.eos && !(fl & 3))
          cut = MT_CUT_MID;
        stream_page(p);
        after = audio && !l.eos && !(fl & 2) && p[26] && p[27 + p[26] - 1] < 255;
      }
    }
    return cut;
  }
  //  Whether a part that starts at the page p, cut where no cut point was,
  //  is inside the link's Vorbis stream with its headers in: then it gets
  //  them all the same, and the model parses what follows the first page.
  int headers_known(const u8 *p) const { return have && l.streams == 1 && l.hdr == 3 && !l.eos && rd32(p + 14) == l.serial && !(p[5] & 2); }
};

//  Pages of packets, as libogg lays them out: up to 255 segments a page,
//  the continued flag on a page that starts inside a packet, the granule
//  position 0 on a page a packet ends on and -1 on one none does (header
//  pages); numbered from seq.
struct mt_pager {
  mt_bytes *out;
  u32 serial, seq;
  u8 lace[255];
  sz nseg;
  mt_bytes body;
  int flags, ends, mid, ok;
  void init(mt_bytes *o, u32 ser, u32 first) {
    out = o;
    serial = ser;
    seq = first;
    nseg = 0;
    body.init();
    flags = 2;  // the first page: BOS
    ends = mid = 0;
    ok = 1;
  }
  void flush() {
    u8 h[27];
    const u64 g = ends ? 0 : ~(u64)0;
    sz at = out->n;
    memcpy(h, "OggS", 4);
    h[4] = 0;
    h[5] = (u8)flags;
    wr32(h + 6, (u32)g);
    wr32(h + 10, (u32)(g >> 32));
    wr32(h + 14, serial);
    wr32(h + 18, seq++);
    wr32(h + 22, 0);
    h[26] = (u8)nseg;
    if(!out->add(h, 27) || !out->add(lace, nseg) || !out->add(body.p, body.n))
      ok = 0;
    else
      ogg_page::crc_set(out->p + at, out->n - at);
    nseg = 0;
    body.n = 0;
    ends = 0;
    flags = mid ? 1 : 0;
  }
  void packet(const u8 *p, sz n) {
    sz at = 0;
    for(;;) {
      const sz k = n - at < 255 ? n - at : 255;
      if(nseg == 255)
        flush();
      lace[nseg++] = (u8)k;
      if(!body.add(p + at, k))
        ok = 0;
      at += k;
      mid = k == 255;
      if(!mid) {
        ends = 1;
        return;
      }
    }
  }
  //  pages for the segments of packets of n bytes
  static sz segs(sz n) { return n / 255 + 1; }
};
//  The header pages a part gets that starts at a page with the sequence
//  number seq inside the link l (oggsplit's header_pages): the
//  identification header alone on a BOS page, the comment header (the
//  vendor string, no comments) and the setup header on the next ones, the
//  setup header ending the last; numbered up to seq - 1.  Into out.
static int mt_header_pages(const mt_link &l, u32 seq, mt_bytes &out) {
  mt_bytes cm;
  mt_pager pg;
  u8 h[11];
  const u8 tail[5] = {0, 0, 0, 0, 1};  // no comments; the framing bit
  sz s;
  u32 count;
  int ok;
  cm.init();
  memcpy(h, "\3vorbis", 7);
  wr32(h + 7, (u32)l.vendor.n);
  ok = cm.add(h, 11) && cm.add(l.vendor.p, l.vendor.n) && cm.add(tail, 5);
  s = mt_pager::segs(cm.n) + mt_pager::segs(l.setup.n);
  count = 1 + (u32)((s + 254) / 255);
  pg.init(&out, l.serial, seq - count);
  pg.packet(l.ident.p, l.ident.n);
  pg.flush();
  pg.packet(cm.p, cm.n);
  pg.packet(l.setup.p, l.setup.n);
  pg.flush();
  ok = ok && pg.ok;
  pg.body.quit();
  cm.quit();
  return ok;
}

//  The splitter: the input in, frames out, the parts dealt by MTwrap.
//  Plain data (a member of the CoroutinePair): Init() sets it up, Quit()
//  frees what it took.
struct OcSplit : Coroutine {
  u64 part, hard;  // the part size; where a part ends without a cut point
  mt_bytes buf;    // the input from begin on
  u64 begin;       // the part under way's start (an offset in the input)
  u64 scan;        // where the next chunk starts
  u64 junk;        // where the junk under way started; ~0: none
  u64 total, parts;
  int eof, fail;  // fail: out of memory
  mt_bytes ins, ins_next;  // the header pages of the part under way, and of the next
  mt_analyzer an;

  void Init(u64 part_size) {
    coro_init();
    part = part_size;
    hard = MT_HARD * part_size;
    buf.init();
    ins.init();
    ins_next.init();
    an.init();
    begin = scan = total = parts = 0;
    junk = ~(u64)0;
    eof = fail = 0;
  }
  void Quit() {
    buf.quit();
    ins.quit();
    ins_next.quit();
    an.quit();
  }
  void put(const void *p, sz n) {
    const u8 *b = (const u8 *)p;
    while(n) {
      sz k;
      chkout();
      k = (sz)(outend - outptr) < n ? (sz)(outend - outptr) : n;
      memcpy(outptr, b, k);
      outptr += k;
      b += k;
      n -= k;
    }
  }
  //  The part under way, up to the offset x, as a frame; the next starts
  //  at x with the header pages in ins_next.
  void emit(u64 x) {
    const sz n = (sz)(x - begin);
    u8 h[MT_FRAME];
    mt_put_le(h + MT_F_LEN, (MT_FRAME - MT_F_XLEN) + (u64)ins.n + n, 8);
    mt_put_le(h + MT_F_XLEN, ins.n, 8);
    put(h, MT_FRAME);
    put(ins.p, ins.n);
    put(buf.p, n);
    memmove(buf.p, buf.p + n, buf.n - n);
    buf.n -= n;
    begin = x;
    parts++;
    //  the next part's header pages are the ones under way now
    mt_bytes t = ins;
    ins = ins_next;
    ins_next = t;
    ins_next.n = 0;
  }
  //  A cut before the page at x (in buf): a part ends there if it is long
  //  enough, or where it has to; with the link's header pages for the next
  //  if the cut is inside it.
  void page_cut(u64 x, int how, const u8 *p) {
    const u64 have = x - begin;
    int with = 0;
    if(x == begin)
      return;
    if(how != MT_CUT_NONE && have >= part)
      with = how == MT_CUT_MID;
    else if(have >= hard)
      with = an.headers_known(p);
    else
      return;
    ins_next.n = 0;
    if(with && !mt_header_pages(an.l, rd32(p + 18), ins_next))
      fail = 1;
    emit(x);
  }
  void do_process() {
    for(;;) {
      sz at, avail;
      //  enough of the input that a page at scan is all there, unless it
      //  has ended
      while(!eof && begin + buf.n - scan < MT_MAXPAGE) {
        sz l;
        chkinp();
        l = (sz)(inpend - inpptr);
        if(!l) {
          eof = 1;
          break;
        }
        if(!buf.add(inpptr, l)) {
          fail = 1;
          eof = 1;
          break;
        }
        inpptr += l;
        total += l;
      }
      if(fail)
        break;
      at = (sz)(scan - begin);
      avail = buf.n - at;
      if(!avail)
        break;
      {
        const u8 *p = buf.p + at;
        int n = mt_page_len(p, avail);
        if(n) {
          int how;
          if(junk != ~(u64)0) {
            an.junk();
            junk = ~(u64)0;
          }
          how = an.page(p);
          page_cut(scan, how, p);
          scan += (u64)n;
          continue;
        }
      }
      //  junk: on to the next "OggS", or where one may start whose bytes
      //  are not all in yet; and a cut in it where a part has to end
      if(junk == ~(u64)0)
        junk = scan;
      {
        sz i = at + 1;
        for(;;) {
          const u8 *q = i < buf.n ? (const u8 *)memchr(buf.p + i, 'O', buf.n - i) : nullptr;
          if(!q) {
            i = buf.n;
            break;
          }
          i = (sz)(q - buf.p);
          if(buf.n - i < 4 || !memcmp(q, "OggS", 4))
            break;
          i++;
        }
        scan = begin + i;
        while(scan - begin >= hard) {
          ins_next.n = 0;
          emit(begin + hard);
        }
      }
    }
    if(!fail && buf.n)
      emit(begin + buf.n);
    yield(this, fail ? Y_FAIL : Y_END);
  }
};

//  ---------------------------------------------------------------------------
//  The workers.

//  The order the workers' failures came in.  One worker failing ends the
//  input of all, and a worker that was in the middle of a part then fails
//  too -- "a part cut short" -- so the failure to report is the first, not
//  the one of the lowest-numbered worker: that made an out-of-memory exit 4
//  as an internal error, and an intact stream decoded short of memory
//  "damaged data".
static u64 mt_fail_clock;

//  What a run on threads gives its workers.
struct oc_mt_args {
  int threads, flags;
  u64 part_max;      // d: the largest part a record can have (mt_part_max())
  const oc_api *api;  // the model, as a table of pointers (oc_load.inc)
  oc_progress *meter;  // the run's progress line (0: none)
  const char *name;    // the input's, for messages
};

//  MTwrap's Enc_class: a worker, coding the parts it is dealt one after the
//  other with the model instance of its thread.  c: each part in memory
//  first, as MTwrap deals it whole -- so the main thread can deal the next
//  to the next worker -- and its output written as it comes; d: its input
//  as it comes, its output in memory until the part is done, the parts
//  going out in turn.  Plain data, never constructed (MTwrap maps its
//  workers): Init() sets it up, do_process() frees what it took as it ends.
template <int f_DEC> struct OcPart : Coroutine {
  oc_mt_args args;
  u32 worker;  // its place among the workers, dealt the parts in turn
  u64 parts;   // the parts done
  void *inst;  // the model instance of the worker's thread
  int code;    // the exit status its error stands for
  u64 failed;  // when it failed, on mt_fail_clock; 0: it has not
  char note[1100];
  mt_bytes part;  // c: the part under way; d: its output
  u8 io[1 << 16];  // the model's output (c), or input (d), 64 KB at a time
  //  its counts on the progress line: done_* its parts' done, prog_* with
  //  the part under way (read from any thread)
  u64 done_raw, done_packed, prog_raw, prog_packed, prog_parts;
  //  -v: where the bits went, its parts' added up, and its tables' size
  int nstage;
  u64 tables;
  double bits[16];
  u64 values[16];
  char stage[16][16];

  uint Init(const oc_mt_args *a, uint w) {
    coro_init();
    args = *a;
    worker = w;
    parts = 0;
    inst = nullptr;
    code = 0;
    failed = 0;
    note[0] = 0;
    part.init();
    done_raw = done_packed = 0;
    prog_set(0, 0);
    nstage = 0;
    tables = 0;
    return 0;
  }
  void Quit() { part.quit(); }
  void prog_set(u64 raw, u64 packed) {
    __atomic_store_n(&prog_raw, raw, __ATOMIC_RELAXED);
    __atomic_store_n(&prog_packed, packed, __ATOMIC_RELAXED);
    __atomic_store_n(&prog_parts, parts, __ATOMIC_RELAXED);
  }
  void prog_add(oc_counts &c) const {
    c.raw += __atomic_load_n(&prog_raw, __ATOMIC_RELAXED);
    c.packed += __atomic_load_n(&prog_packed, __ATOMIC_RELAXED);
    c.parts += __atomic_load_n(&prog_parts, __ATOMIC_RELAXED);
  }
  int fail(int c, const char *fmt, ...) OGC_PRINTF(3, 4) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(note, sizeof note, fmt, ap);
    va_end(ap);
    code = c;
    failed = __atomic_add_fetch(&mt_fail_clock, 1, __ATOMIC_SEQ_CST);
    return c;
  }
  //  up to n bytes of the worker's input, fewer only at its end
  sz read(u8 *p, sz n) {
    sz k = 0;
    while(k < n) {
      sz l;
      if(inpptr >= inpend) {
        if(f_quit)
          break;
        chkinp();
        if(inpptr >= inpend)
          break;  // a 0-length buffer: the end
      }
      l = (sz)(inpend - inpptr) < n - k ? (sz)(inpend - inpptr) : n - k;
      memcpy(p + k, inpptr, l);
      inpptr += l;
      k += l;
    }
    return k;
  }
  //  all n bytes into the worker's output
  void write(const void *pp, sz n) {
    const u8 *p = (const u8 *)pp;
    while(n) {
      sz l;
      if(outptr >= outend)
        chkout();
      l = (sz)(outend - outptr) < n ? (sz)(outend - outptr) : n;
      memcpy(outptr, p, l);
      outptr += l;
      p += l;
      n -= l;
    }
  }
  void put_varint(u64 v) {
    u8 b[10];
    int n = 0;
    while(v > 127) {
      b[n++] = (u8)(v & 127) | 128;
      v >>= 7;
    }
    b[n++] = (u8)v;
    write(b, (sz)n);
  }
  //  a varint of the worker's input: 1, or 0 at its end or when broken
  int get_varint(u64 &v) {
    u8 c;
    int shift;
    v = 0;
    for(shift = 0; shift <= 63; shift += 7) {
      if(read(&c, 1) != 1)
        return 0;
      v |= (u64)(c & 127) << shift;
      if(!(c & 128))
        return 1;
    }
    return 0;
  }
  //  the place of the next part it has (worker w's k-th: w + kN)
  u64 next_part() const { return worker + parts * (u64)args.threads; }
  //  the model's error, as its exit status and note
  int model_failed(int r) {
    const char *e = args.api->Error(inst);
    return fail(r >= 5 && r <= 7 ? r - 4 : OGC_EXIT_INTERNAL, "%s", e && *e ? e : "the model failed");
  }
  void tell() {
    if(args.meter)
      args.meter->poll();
  }
  //  -v: the model's bits, added to the worker's
  void add_stages() {
    int i;
    if(!(args.flags & OC_F_VERBOSE) || !inst)
      return;
    tables = args.api->TableBytes();
    if(!args.api->Stage)
      return;
    for(i = 0; i < 16; i++) {
      double b = 0;
      unsigned long long v = 0;
      const char *s = args.api->Stage(inst, i, &b, &v);
      if(!s)
        break;
      snprintf(stage[i], sizeof stage[i], "%s", s);
      bits[i] = b;  // (the model's counts run on over a worker's parts)
      values[i] = v;
      nstage = i + 1;
    }
  }

  void do_process() {
    int r;
    inst = args.api->Alloc();
    if(!inst)
      r = fail(OGC_EXIT_IO, "no memory for a model on thread %u", worker + 1);
    else {
      r = f_DEC ? run_d() : run_c();
      add_stages();
      args.api->Free(inst);
      inst = nullptr;
    }
    Quit();
    for(;;)
      yield(this, r ? Y_FAIL : Y_END);
  }

  //  c: each frame (u64 L, u64 xlen, then L - 8 bytes: the part) into its
  //  record and the part coded; as the frames end, the worker's end
  int run_c() {
    for(;;) {
      u8 h[MT_FRAME], rec[MT_REC];
      const sz n = read(h, MT_FRAME);
      u64 L, xlen, plen, pos = 0;
      int r;
      if(!n) {
        mt_rec(rec, 0, 0, next_part());
        write(rec, MT_REC);
        return 0;
      }
      L = mt_get_le(h + MT_F_LEN, 8);
      xlen = mt_get_le(h + MT_F_XLEN, 8);
      plen = L - (MT_FRAME - MT_F_XLEN);
      if(n < MT_FRAME || L <= MT_FRAME - MT_F_XLEN || xlen > plen || plen != (sz)plen)
        return fail(OGC_EXIT_INTERNAL, "internal error: a part framed wrong");
      part.n = 0;
      if(!part.reserve((sz)plen))
        return fail(OGC_EXIT_IO, "no memory for a part of %" PRIu64 " bytes", plen);
      if(read(part.p, (sz)plen) != plen)
        return fail(OGC_EXIT_INTERNAL, "internal error: a part cut short");
      mt_rec(rec, plen, xlen, next_part());
      write(rec, MT_REC);
      done_packed += MT_REC;
      r = args.api->Init(inst, OC_ENCODE, args.flags | (parts ? OC_F_SOLID : 0), args.name);
      if(r)
        return model_failed(r);
      args.api->addout(inst, io, (int)sizeof io);
      for(;;) {
        r = args.api->Loop(inst);
        if(r == OC_NEED_INPUT) {
          const u64 k = plen - pos < ((u64)1 << 20) ? plen - pos : (u64)1 << 20;
          args.api->addinp(inst, k ? part.p + pos : nullptr, (int)k);
          pos += k;
          continue;
        }
        if(r == OC_NEED_OUTPUT || r == OC_DONE) {
          const int l = args.api->getoutlen(inst);
          u64 d;
          if(l > 0) {
            put_varint((u64)l);
            write(io, (sz)l);
            done_packed += (u64)l + 3;
          }
          d = args.api->Done ? args.api->Done(inst) : pos;
          prog_set(done_raw + (d > xlen ? d - xlen : 0), done_packed);
          tell();
          if(r == OC_DONE)
            break;
          args.api->addout(inst, io, (int)sizeof io);
          continue;
        }
        return model_failed(r);
      }
      put_varint(0);
      parts++;
      done_raw += plen - xlen;
      done_packed += 1;
      prog_set(done_raw, done_packed);
      tell();
      //  the part's last buffer handed over now, as far as it is filled (a
      //  round's end): the part's end is in the container as it is done
      yield(this, Y_PART);
    }
  }

  //  d: each record into its part, the inserted pages removed, after which
  //  MTwrap takes the next part from the next worker; to the record of the
  //  worker's end, after which nothing
  int run_d() {
    for(;;) {
      u8 h[MT_REC];
      u64 plen, xlen, left = 0;
      int r, ended = 0;
      if(read(h, MT_REC) < MT_REC)
        return fail(OGC_EXIT_REFUSED, "damaged data: a worker's records cut short");
      plen = mt_get_le(h + MT_R_PLEN, 8);
      xlen = mt_get_le(h + MT_R_XLEN, 8);
      if(mt_get_le(h + MT_R_CRC, 4) != mt_rec_crc(h) || mt_get_le(h + MT_R_ZERO, 4) || mt_get_le(h + MT_R_PART, 8) != next_part())
        return fail(OGC_EXIT_REFUSED, "damaged data: a part's record broken, or a part lost, added or moved");
      done_packed += MT_REC;
      if(!plen) {  // the worker's end
        u8 x;
        if(xlen)
          return fail(OGC_EXIT_REFUSED, "damaged data: a part's record broken");
        if(read(&x, 1))
          return fail(OGC_EXIT_REFUSED, "damaged data after a worker's end");
        return 0;
      }
      if(plen > args.part_max || xlen > plen)
        return fail(OGC_EXIT_REFUSED, "damaged data: a part's record broken");
      part.n = 0;
      if(!part.reserve((sz)plen))
        return fail(OGC_EXIT_IO, "no memory for a part of %" PRIu64 " bytes", plen);
      r = args.api->Init(inst, OC_DECODE, args.flags | (parts ? OC_F_SOLID : 0), args.name);
      if(r)
        return model_failed(r);
      args.api->addout(inst, part.p, (int)(plen < ((u64)1 << 30) ? plen : (u64)1 << 30));
      for(;;) {
        r = args.api->Loop(inst);
        if(r == OC_NEED_INPUT) {
          sz k = 0;
          if(!ended && !left) {
            if(!get_varint(left))
              return fail(OGC_EXIT_REFUSED, "damaged data: a part's stream cut short");
            done_packed += 1;
            ended = !left;
          }
          if(!ended) {
            k = left < sizeof io ? (sz)left : sizeof io;
            if(read(io, k) != k)
              return fail(OGC_EXIT_REFUSED, "damaged data: a part's stream cut short");
            left -= k;
            done_packed += k;
          }
          args.api->addinp(inst, k ? io : nullptr, (int)k);
          continue;
        }
        if(r == OC_NEED_OUTPUT || r == OC_DONE) {
          part.n += (sz)args.api->getoutlen(inst);
          prog_set(done_raw + (part.n > xlen ? part.n - xlen : 0), done_packed);
          tell();
          if(r == OC_DONE)
            break;
          if(part.n >= plen)  // (more than its record says)
            return fail(OGC_EXIT_REFUSED, "damaged data: a part comes out longer than its record says");
          args.api->addout(inst, part.p + part.n, (int)(plen - part.n < ((u64)1 << 30) ? plen - part.n : (u64)1 << 30));
          continue;
        }
        return model_failed(r);
      }
      if(part.n != plen)
        return fail(OGC_EXIT_REFUSED, "damaged data: a part comes out shorter than its record says");
      //  what of the part's stream the decoder did not read: the chunks to the end
      while(!ended) {
        if(left) {
          sz k = left < sizeof io ? (sz)left : sizeof io;
          if(read(io, k) != k)
            return fail(OGC_EXIT_REFUSED, "damaged data: a part's stream cut short");
          left -= k;
          done_packed += k;
          continue;
        }
        if(!get_varint(left))
          return fail(OGC_EXIT_REFUSED, "damaged data: a part's stream cut short");
        done_packed += 1;
        ended = !left;
      }
      write(part.p + xlen, (sz)(plen - xlen));
      parts++;
      done_raw += plen - xlen;
      prog_set(done_raw, done_packed);
      tell();
      yield(this, Y_PART);
    }
  }
};

//  c: the splitter into MTwrap's coder; d: MTwrap's decoder
typedef MTwrap<0, OcPart<0>, const oc_mt_args *> OcMtWorkers;
typedef CoroutinePair<OcSplit, OcMtWorkers> OcMtEnc;
typedef MTwrap<1, OcPart<1>, const oc_mt_args *> OcMtDec;

//  Whether a worker failed: its exit status, its note copied (the first
//  failed worker's; the workers go with MTwrap's Quit()).
template <class M> static int mt_failed(M &m, char *note, sz n) {
  uint i, first = m.th_N;
  for(i = 0; i < m.th_N; i++)
    if(m.C1[i].f_fail && (first == m.th_N || (m.C1[i].failed && (!m.C1[first].failed || m.C1[i].failed < m.C1[first].failed))))
      first = i;
  if(first == m.th_N)
    return 0;
  snprintf(note, n, "%s", m.C1[first].note);
  return m.C1[first].code ? m.C1[first].code : OGC_EXIT_INTERNAL;
}
//  Whether d's workers had the parts as c deals them, in turn: of P in all,
//  worker i the i-th of each round of N, one more where P % N is past it.
static int mt_dealt(OcMtDec &d) {
  u64 P = 0, N = d.th_N;
  uint i;
  for(i = 0; i < d.th_N; i++)
    P += d.C1[i].parts;
  for(i = 0; i < d.th_N; i++)
    if(d.C1[i].parts != P / N + (i < P % N))
      return 0;
  return 1;
}
//  A run on threads' counts: its workers', added up, after the header.
template <class M> static void mt_counts(void *ctx, oc_counts &c) {
  const M &m = *(const M *)ctx;
  uint i;
  c.raw = c.parts = 0;
  c.packed = MT_HEAD;
  for(i = 0; i < m.th_N; i++)
    m.C1[i].prog_add(c);
}
