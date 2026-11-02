// oggsplit: splits an Ogg Vorbis file into parts that are valid Ogg Vorbis files by themselves, for
// processing them in parallel (with ogg2tsv, for one), and joins them back.
//
//   oggsplit N [options] input.ogg output_prefix
//   oggsplit r [options] input_prefix output.ogg
//
//   -v  print statistics
//
// The file is read as a sequence of Ogg pages, found as libogg finds them: "OggS", version 0, the
// header and body there in full, the CRC right; where no page starts, the next one is looked for
// from the next byte on. Bytes that aren't part of a page (junk) stay where they are. A link (of
// a chained file) starts with the first BOS page, or with a BOS page after the EOS pages of all
// the streams of the last link; the BOS pages right after it start its other streams, if it has
// more than one. The cuts go before pages:
// - before a BOS page that starts a link, but the first (what comes before it goes with it):
//   nothing changes there;
// - within a link that is a single Vorbis stream, its three header packets on pages of their
//   own, before a page that starts with a new packet and has a granule position, after an audio
//   page that ends with a complete packet, and before the EOS page (a decoder can only cut the
//   last packet down to the EOS page's granule position if a page before it had one). The part
//   before the cut gets the EOS flag on its last page (and the CRC that goes with it). The part
//   after it gets the stream's header packets inserted at its start: the identification header
//   alone on a BOS page, then a comment header that keeps the vendor string but no comments
//   (comments can hold pictures of hundreds of KB, and nothing that decodes or parses the audio
//   reads them), and the setup header, as libvorbis lays them out, numbered so that the sequence
//   numbers lead without a gap into the page at the cut.
//   ogg2tsv takes all the pages of a link for its stream, while a decoder takes those with the
//   stream's serial number, and a BOS page with another one for a new link: a page of another
//   stream, or a BOS page that doesn't start a link, ends the cut points of its link.
// Among the cut points, the ones that make the largest part as small as possible are chosen, a
// part costing kByteCost per byte plus 1 per sample per channel of its Vorbis audio. There are
// fewer than N parts when there are not enough cut points. The parts are <prefix>.000.ogg,
// <prefix>.001.ogg, ...
//
// <prefix>.meta holds only what restoring cannot find out from the parts: for each part after the
// first, the number of pages inserted at its start, as a LEB128 varint (one byte up to 127); 0 for
// a cut before a link. When it is not 0, the part before it had the EOS flag set on its last page.
// Restoring removes the inserted pages, takes for that last page the last place where a page
// starts that ends at the end of the part (the split only cuts after pages where no other place
// would be taken), clears the flag and puts the CRC back.
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

[[noreturn]] void fail(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fputs("oggsplit: ", stderr);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
  va_end(ap);
  exit(1);
}

FILE* open_in(const std::string& name) {
  FILE* f = fopen(name.c_str(), "rb");
  if (!f) fail("cannot open %s: %s", name.c_str(), strerror(errno));
  return f;
}

FILE* open_out(const std::string& name) {
  FILE* f = fopen(name.c_str(), "wb");
  if (!f) fail("cannot create %s: %s", name.c_str(), strerror(errno));
  return f;
}

void close_out(FILE* f, const std::string& name) {
  if (ferror(f) | fclose(f)) fail("write error on %s", name.c_str());
}

typedef unsigned long long ull;

const char* plural(uint64_t n) { return n == 1 ? "" : "s"; }

// ---------------------------------------------------------------------------
// Ogg pages (RFC 3533): "OggS", version, flags, granule position (8 bytes), serial number,
// sequence number, CRC (4 bytes each, little-endian), the number of segments, their lacing values,
// the body

const size_t kHdr = 27;                         // the header up to the lacing values
const size_t kMaxPage = kHdr + 255 + 255 * 255;  // the longest page
const size_t kNone = SIZE_MAX;
enum : int { kCont = 1, kBos = 2, kEos = 4 };  // the flags: continued packet, first page, last page

uint32_t crc_tab[8][256];

void crc_init() {
  for (uint32_t i = 0; i < 256; i++) {
    uint32_t c = i << 24;
    for (int k = 0; k < 8; k++) c = c & 0x80000000u ? c << 1 ^ 0x04C11DB7u : c << 1;
    crc_tab[0][i] = c;
  }
  for (int t = 1; t < 8; t++)
    for (int i = 0; i < 256; i++) crc_tab[t][i] = crc_tab[t - 1][i] << 8 ^ crc_tab[0][crc_tab[t - 1][i] >> 24];
}

uint32_t be32(const uint8_t* p) { return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3]; }
uint32_t rd32(const uint8_t* p) { return (uint32_t)p[3] << 24 | (uint32_t)p[2] << 16 | (uint32_t)p[1] << 8 | p[0]; }
uint64_t rd64(const uint8_t* p) { return (uint64_t)rd32(p + 4) << 32 | rd32(p); }
void wr32(uint8_t* p, uint32_t v) {
  for (int i = 0; i < 4; i++) p[i] = (uint8_t)(v >> 8 * i);
}

// Ogg's CRC (polynomial 04C11DB7, MSB first, no inversions), 8 bytes at a time
uint32_t crc_update(uint32_t c, const uint8_t* p, size_t n) {
  for (; n >= 8; p += 8, n -= 8) {
    uint32_t a = c ^ be32(p), b = be32(p + 4);
    c = crc_tab[7][a >> 24] ^ crc_tab[6][a >> 16 & 255] ^ crc_tab[5][a >> 8 & 255] ^ crc_tab[4][a & 255] ^
        crc_tab[3][b >> 24] ^ crc_tab[2][b >> 16 & 255] ^ crc_tab[1][b >> 8 & 255] ^ crc_tab[0][b & 255];
  }
  for (; n; p++, n--) c = c << 8 ^ crc_tab[0][(c >> 24 ^ *p) & 255];
  return c;
}

// the CRC of the page p (n bytes), its CRC field taken as 0
uint32_t page_crc(const uint8_t* p, size_t n) {
  static const uint8_t zero[4] = {0, 0, 0, 0};
  return crc_update(crc_update(crc_update(0, p, 22), zero, 4), p + 26, n - 26);
}

void set_crc(uint8_t* p, size_t n) { wr32(p + 22, page_crc(p, n)); }

// The length of the page at p (avail bytes from p on), or 0 if no page starts there.
size_t page_len(const uint8_t* p, size_t avail) {
  if (avail < kHdr || memcmp(p, "OggS", 4) || p[4]) return 0;
  size_t n = kHdr + p[26];
  if (avail < n) return 0;
  for (size_t i = kHdr; i < kHdr + p[26]; i++) n += p[i];
  if (avail < n || page_crc(p, n) != rd32(p + 22)) return 0;
  return n;
}

// Where the last page of d (n bytes) starts: the last place where a page starts that ends at n,
// or kNone. This is how restoring finds the page that got the EOS flag at the end of a part.
size_t last_page(const uint8_t* d, size_t n) {
  for (size_t p = n < kHdr ? 0 : n - kHdr + 1; p-- > 0;)
    if (d[p] == 'O' && page_len(d + p, n - p) == n - p) return p;
  return kNone;
}

// Whether last_page() finds the page p (n bytes) where it starts once it has the EOS flag: that
// there is no other page in it that would end with it.
bool eos_findable(const uint8_t* p, size_t n) {
  static std::vector<uint8_t> t;
  t.assign(p, p + n);
  t[5] |= kEos;
  set_crc(t.data(), n);
  return last_page(t.data(), n) == 0;
}

// A chunk of the file: a page, or the bytes up to the next page that are not part of one (junk).
struct Chunk {
  uint64_t pos = 0, len = 0;
  const uint8_t* page = nullptr;  // a page's bytes (valid during the call only); null for junk
};

// Calls on_chunk for each chunk of the file; returns its size.
template <class F>
uint64_t scan_pages(FILE* f, F&& on_chunk) {
  std::vector<uint8_t> buf((1 << 20) + kMaxPage);
  uint8_t* b = buf.data();
  size_t have = 0, at = 0;  // the bytes in buf, the place in them
  uint64_t off = 0;         // the offset of buf[0]
  uint64_t junk = UINT64_MAX;  // where the junk under way started
  bool eof = false;
  auto end_junk = [&](uint64_t pos) {
    if (junk == UINT64_MAX) return;
    Chunk j;
    j.pos = junk;
    j.len = pos - junk;
    on_chunk(j);
    junk = UINT64_MAX;
  };
  for (;;) {
    if (have - at < kMaxPage && !eof) {  // a page from at on is in buf if it is in the file
      memmove(b, b + at, have - at);
      off += at, have -= at, at = 0;
      while (have < buf.size()) {
        size_t n = fread(b + have, 1, buf.size() - have, f);
        if (!n) {
          eof = true;
          break;
        }
        have += n;
      }
      if (ferror(f)) fail("read error");
    }
    if (at == have) break;
    size_t n = page_len(b + at, have - at);
    if (n) {
      end_junk(off + at);
      Chunk c;
      c.pos = off + at;
      c.len = n;
      c.page = b + at;
      on_chunk(c);
      at += n;
      continue;
    }
    if (junk == UINT64_MAX) junk = off + at;
    // on to the next "OggS" (or to the end of buf, where one may start whose bytes aren't all in)
    size_t i = at + 1;
    for (;;) {
      const uint8_t* q = (const uint8_t*)memchr(b + i, 'O', have - i);
      if (!q) {
        i = have;
        break;
      }
      i = (size_t)(q - b);
      if (have - i < 4 || !memcmp(q, "OggS", 4)) break;
      i++;
    }
    at = i;
  }
  end_junk(off + have);
  return off + have;
}

// ---------------------------------------------------------------------------
// Finding the cut points

const size_t kMaxHeader = 1u << 20;  // the longest identification and setup headers copied
const size_t kMaxVendor = 1u << 16;  // the longest vendor string copied

// The cost of processing a part, for balancing: kByteCost per byte plus 1 per sample per channel
// (of a Vorbis stream, from the granule positions). Over the pages of 12 streams (8 to 96 kHz, 1
// to 6 channels, 48 kbps to q10), ogg2tsv c wrote 2.46 values per byte plus 0.42 per sample per
// channel, taking 20 to 31 ns per value: a byte holds more values at low bit rates, and a sample
// more at high ones. Within a stream the two go together, but a chain of a q10, a q0 and an
// 8 kHz mono stream cut into 8 parts by size alone had ogg2tsv take 1.41 times the mean time on
// the largest part, and 1.12 times with this weight (2 to 20 did about as well).
const uint64_t kByteCost = 6;

// A place to cut: before the page at pos.
struct Cut {
  uint64_t pos;    // the page's offset
  uint64_t cost;   // the cost of what comes before it
  uint64_t pages;  // the pages before it
  int link;        // -1: the page starts a link; else the link whose headers the part gets
  uint32_t seq;    // the page's sequence number
  uint32_t last;   // the length of the page before it, which gets the EOS flag
};

// A link of a chained file: the pages from a BOS page that starts it (and those of its other
// streams, if it has more than one) to before the next.
struct Link {
  uint32_t serial = 0;  // its (first) stream's
  int streams = 1;      // its BOS pages
  int eos = 0;          // its EOS pages (after one, ogg2tsv takes a link to have ended)
  bool bos_run = true;  // only BOS pages so far
  // Its first stream, while it is the only one: hdr is the number of header packets it has had
  // (Vorbis, on pages of their own) up to 3, or -1 if it is something else.
  int hdr = 0;
  bool open = false;  // its last page ended inside a packet
  bool cuts = false;  // it has cut points (and keeps its header packets for them)
  int64_t gran = 0;   // the last granule position
  int channels = 0;
  uint32_t bs1 = 0;  // the long block size
  std::vector<uint8_t> pkt;  // the header packet under way (up to cap(), of pkt_len bytes)
  size_t pkt_len = 0;
  std::vector<uint8_t> ident, vendor, setup;
};

struct Analyzer {
  std::vector<Link> links;
  std::vector<Cut> cuts;
  uint64_t cost = 0;   // of the chunks so far
  uint64_t pages = 0, junk = 0, junk_bytes = 0, samples = 0, vorbis = 0, big = 0, link_cuts = 0;
  // whether the page before the chunk under way is one a cut may come after, and its length
  bool after = false;
  uint32_t after_len = 0;

  size_t cap(const Link& l) const { return l.hdr == 1 ? 11 + kMaxVendor : kMaxHeader + 1; }

  // The header packet hdr of l has come in full.
  void header(Link& l) {
    const std::vector<uint8_t>& d = l.pkt;
    static const uint8_t kSig[3][7] = {{1, 'v', 'o', 'r', 'b', 'i', 's'}, {3, 'v', 'o', 'r', 'b', 'i', 's'}, {5, 'v', 'o', 'r', 'b', 'i', 's'}};
    if (d.size() < 7 || memcmp(d.data(), kSig[l.hdr], 7)) {
      l.hdr = -1;
    } else if (l.hdr == 0) {  // the identification header, as ogg2tsv takes it
      int b0 = l.pkt_len == 30 ? d[28] & 15 : 0, b1 = l.pkt_len == 30 ? d[28] >> 4 : 0;
      if (l.pkt_len != 30 || rd32(&d[7]) || !d[11] || !rd32(&d[12]) || b0 < 6 || b1 < b0 || b1 > 13) {
        l.hdr = -1;
      } else {
        l.ident = d;
        l.channels = d[11];
        l.bs1 = 1u << b1;
        vorbis++;
      }
    } else if (l.hdr == 1) {  // the comment header: its vendor string, if it is all there
      if (d.size() >= 11 && rd32(&d[7]) <= d.size() - 11) l.vendor.assign(d.begin() + 11, d.begin() + 11 + rd32(&d[7]));
    } else if (l.pkt_len > kMaxHeader) {  // the setup header
      big++;
      l.hdr = -1;
    } else {
      l.setup = d;
    }
    if (l.hdr >= 0) l.hdr++;
    l.pkt.clear();
    l.pkt_len = 0;
  }

  // A page of the stream of l, its only one; returns its samples.
  uint64_t stream_page(Link& l, const uint8_t* p) {
    const int nseg = p[26];
    const uint8_t* lace = p + kHdr;
    const uint8_t* body = lace + nseg;
    if (l.hdr >= 0 && l.hdr < 3) {
      if (!(p[5] & kCont) != !l.open) l.hdr = -1;  // a packet cut off, or a continuation of none
      for (int s = 0, at = 0; s < nseg && l.hdr >= 0 && l.hdr < 3; at += lace[s++]) {
        size_t k = std::min((size_t)lace[s], cap(l) - std::min(cap(l), l.pkt.size()));
        l.pkt.insert(l.pkt.end(), body + at, body + at + k);
        l.pkt_len += lace[s];
        if (lace[s] < 255) {
          header(l);
          if (l.hdr == 3 && s != nseg - 1) l.hdr = -1;  // the setup header doesn't end its page
        }
      }
    }
    if (nseg) l.open = lace[nseg - 1] == 255;
    if (!l.channels) return 0;  // (not a Vorbis stream)
    // the samples: what the granule position went up by, if the packets ended on the page can
    // return that many (each at most half a long block)
    int64_t g = (int64_t)rd64(p + 6);
    if (g == -1) return 0;
    uint64_t ended = 0, n = 0;
    for (int s = 0; s < nseg; s++) ended += lace[s] < 255;
    if (g > l.gran && (uint64_t)g - (uint64_t)l.gran <= ended * (l.bs1 / 2)) n = (uint64_t)g - (uint64_t)l.gran;
    l.gran = g;
    return n;
  }

  void add_cut(uint64_t pos, int link, uint32_t seq, uint32_t last) {
    Cut c = {pos, cost, pages, link, seq, last};
    cuts.push_back(c);
    if (link >= 0) links[link].cuts = true;
    else link_cuts++;
  }

  void new_link(uint32_t serial) {
    if (!links.empty()) {  // done with the last: what its cuts don't need
      Link& l = links.back();
      std::vector<uint8_t>().swap(l.pkt);
      if (!l.cuts) {
        std::vector<uint8_t>().swap(l.ident);
        std::vector<uint8_t>().swap(l.vendor);
        std::vector<uint8_t>().swap(l.setup);
      }
    }
    links.push_back(Link());
    links.back().serial = serial;
  }

  void chunk(const Chunk& c) {
    const bool ok = after;  // a cut may come before this chunk, if it is a page of the stream
    after = false;
    if (!c.page) {
      junk++;
      junk_bytes += c.len;
      cost += kByteCost * c.len;
      return;
    }
    const uint8_t* p = c.page;
    const int fl = p[5];
    const uint32_t serial = rd32(p + 14);
    bool stray = false;  // a BOS page that starts neither a link nor a stream of one
    if (fl & kBos) {
      // A link starts at a BOS page that comes first or after the EOS pages of all the streams of
      // the last link (with one stream, after an EOS page, as ogg2tsv has it). A BOS page among
      // those that start a link starts another stream of it.
      Link* l = links.empty() ? nullptr : &links.back();
      if (!l || l->eos >= l->streams) {
        if (l) add_cut(c.pos, -1, 0, 0);  // (what comes before the first link goes with it)
        new_link(serial);
      } else if (l->bos_run) {
        l->streams++;
      } else {
        stray = true;
      }
    } else if (!links.empty()) {
      links.back().bos_run = false;
    }
    if (!links.empty() && (fl & kEos)) links.back().eos++;
    uint64_t smp = 0;
    if (!links.empty() && links.back().streams == 1) {
      Link& l = links.back();
      // ogg2tsv takes all the pages of the link for its stream, and a decoder only those with its
      // serial number, and a stray BOS page for a new link if it has another: where they could
      // differ, the link has no cut points from there on (nor its header packets, if they are
      // not all in)
      if (serial != l.serial || stray) l.hdr = -1;
      if (serial == l.serial) {
        const bool audio = l.hdr == 3;  // all the header packets came before this page
        // (not before the EOS page, nor before a page no packet ends on: a decoder can only cut
        // the last packet down to the EOS page's granule position if a page before it had one)
        if (ok && audio && !l.eos && !(fl & (kBos | kCont)) && rd64(p + 6) != ~(uint64_t)0)
          add_cut(c.pos, (int)links.size() - 1, rd32(p + 18), after_len);
        smp = stream_page(l, p);
        after = audio && !l.eos && !(fl & kBos) && p[26] && p[kHdr + p[26] - 1] < 255 &&
                eos_findable(p, c.len);
        after_len = (uint32_t)c.len;
        samples += smp;
        smp *= l.channels;
      }
    }
    pages++;
    cost += kByteCost * c.len + smp;
  }

  void end() { new_link(0); links.pop_back(); }
};

// Chooses among the candidate cut points x[1..C] (x[0] = 0, x[C+1] = total),
// x being the cost of what precedes them, the ones splitting into
// K = min(N, C+1) parts with the smallest largest cost, then within that
// bound the ones nearest to an even split of what is left. Returns the
// indices of the chosen points. (h264split's.)
std::vector<size_t> choose_cuts(const std::vector<uint64_t>& x, size_t N) {
  size_t C = x.size() - 2;
  size_t K = std::min(N, C + 1);
  std::vector<size_t> r;
  if (K <= 1) return r;
  if (K == C + 1) {
    for (size_t i = 1; i <= C; i++) r.push_back(i);
    return r;
  }
  uint64_t T = x[C + 1], lo = 0, hi = T;
  for (size_t i = 0; i <= C; i++) lo = std::max(lo, x[i + 1] - x[i]);
  std::vector<size_t> far(C + 1);
  std::vector<size_t> mp(C + 1);  // fewest parts from point j with parts <= L
  auto compute = [&](uint64_t L) {
    size_t k = 0;
    for (size_t j = 0; j <= C; j++) {
      if (k < j) k = j;
      while (k + 1 <= C && x[k + 1] - x[j] <= L) k++;
      far[j] = k;
    }
    for (size_t j = C + 1; j-- > 0;) mp[j] = T - x[j] <= L ? 1 : 1 + mp[far[j]];
  };
  while (lo < hi) {  // the smallest feasible largest part
    uint64_t mid = lo + (hi - lo) / 2;
    compute(mid);
    if (mp[0] <= K)
      hi = mid;
    else
      lo = mid + 1;
  }
  compute(lo);
  size_t s = 0;
  for (size_t left = K; left > 1; left--) {
    double target = (double)x[s] + (double)(T - x[s]) / (double)left;
    size_t end = std::min(far[s], C + 2 - left), best = 0;
    double bd = 0;
    for (size_t j = s + 1; j <= end; j++) {
      if (mp[j] > left - 1) continue;
      double d = (double)x[j] - target;
      if (d < 0) d = -d;
      if (!best || d < bd) best = j, bd = d;
    }
    if (!best) fail("internal error: no cut point");
    r.push_back(best);
    s = best;
  }
  return r;
}

// The header pages a part gets that starts at a page of the link l with the sequence number seq:
// the identification header alone on a BOS page, the comment header (the vendor string, no
// comments) and the setup header on the next ones, as many as they take, the setup header ending
// the last; granule position 0 (-1 on a page no packet ends on); numbered up to seq - 1. Sets
// count to the number of pages.
std::vector<uint8_t> header_pages(const Link& l, uint32_t seq, uint32_t& count) {
  std::vector<uint8_t> comment = {3, 'v', 'o', 'r', 'b', 'i', 's', 0, 0, 0, 0};
  wr32(&comment[7], (uint32_t)l.vendor.size());
  comment.insert(comment.end(), l.vendor.begin(), l.vendor.end());
  comment.insert(comment.end(), {0, 0, 0, 0, 1});  // no comments; the framing bit
  const std::vector<uint8_t>* group[2][2] = {{&l.ident, nullptr}, {&comment, &l.setup}};
  struct Page {
    std::vector<uint8_t> lace, body;
    int flags;
    bool ends;  // a packet ends on it
  };
  std::vector<Page> pages;
  for (int g = 0; g < 2; g++) {  // the packets of a group start a page and end one
    bool mid = false;  // the page under way starts inside a packet
    pages.push_back(Page{{}, {}, g ? 0 : kBos, false});
    for (const std::vector<uint8_t>* d : group[g]) {
      if (!d) continue;
      for (size_t at = 0;;) {
        if (pages.back().lace.size() == 255) pages.push_back(Page{{}, {}, mid ? kCont : 0, false});
        size_t k = std::min<size_t>(255, d->size() - at);
        pages.back().lace.push_back((uint8_t)k);
        pages.back().body.insert(pages.back().body.end(), d->begin() + at, d->begin() + at + k);
        at += k;
        mid = k == 255;
        if (!mid) {
          pages.back().ends = true;
          break;
        }
      }
    }
  }
  count = (uint32_t)pages.size();
  std::vector<uint8_t> out;
  for (size_t i = 0; i < pages.size(); i++) {
    const Page& pg = pages[i];
    size_t at = out.size();
    out.resize(at + kHdr);
    uint8_t* h = &out[at];
    memcpy(h, "OggS", 4);
    h[4] = 0;
    h[5] = (uint8_t)pg.flags;
    uint64_t gran = pg.ends ? 0 : ~(uint64_t)0;
    wr32(h + 6, (uint32_t)gran);
    wr32(h + 10, (uint32_t)(gran >> 32));
    wr32(h + 14, l.serial);
    wr32(h + 18, seq - count + (uint32_t)i);
    h[26] = (uint8_t)pg.lace.size();
    out.insert(out.end(), pg.lace.begin(), pg.lace.end());
    out.insert(out.end(), pg.body.begin(), pg.body.end());
    set_crc(&out[at], out.size() - at);
  }
  return out;
}

std::string part_name(const std::string& prefix, size_t i) {
  char b[32];
  snprintf(b, sizeof b, ".%03u.ogg", (unsigned)i);
  return prefix + b;
}

// copies n bytes from fi to fo
void copy_bytes(FILE* fi, FILE* fo, uint64_t n, const char* what) {
  static std::vector<uint8_t> buf(1 << 20);
  while (n) {
    size_t k = (size_t)std::min<uint64_t>(n, buf.size());
    size_t got = fread(buf.data(), 1, k, fi);
    if (got != k) fail("%s: unexpected end of file or read error", what);
    fwrite(buf.data(), 1, k, fo);
    n -= k;
  }
}

void put_varint(FILE* f, uint64_t v) {
  while (v > 127) fputc((int)(v & 127) | 128, f), v >>= 7;
  fputc((int)v, f);
}

int split(size_t N, bool verbose, const std::string& in_name, const std::string& prefix) {
  FILE* fi = open_in(in_name);
  Analyzer an;
  uint64_t total = scan_pages(fi, [&](const Chunk& c) { an.chunk(c); });
  an.end();
  fclose(fi);

  std::vector<uint64_t> x{0};
  for (const Cut& c : an.cuts) x.push_back(c.cost);
  x.push_back(an.cost);
  std::vector<size_t> chosen = choose_cuts(x, N);

  struct Part {
    uint64_t begin, end, pages;
    const Cut* cut = nullptr;  // the cut it starts at
    uint32_t last = 0;         // the length of its last page, which gets the EOS flag
    uint32_t inserted = 0;     // pages inserted at its start
    size_t extra = 0;          // their bytes
  };
  std::vector<Part> parts;
  uint64_t begin = 0, before = 0;
  for (size_t k = 0; k <= chosen.size(); k++) {
    const Cut* next = k < chosen.size() ? &an.cuts[chosen[k] - 1] : nullptr;
    Part p;
    p.begin = begin;
    p.end = next ? next->pos : total;
    p.pages = (next ? next->pages : an.pages) - before;
    p.cut = k ? &an.cuts[chosen[k - 1] - 1] : nullptr;
    if (next && next->link >= 0) p.last = next->last;
    parts.push_back(p);
    begin = p.end;
    before = next ? next->pages : 0;
  }

  fi = open_in(in_name);
  std::vector<uint8_t> page(kMaxPage);
  for (size_t k = 0; k < parts.size(); k++) {
    Part& p = parts[k];
    std::string name = part_name(prefix, k);
    FILE* fo = open_out(name);
    if (p.cut && p.cut->link >= 0) {
      std::vector<uint8_t> extra = header_pages(an.links[p.cut->link], p.cut->seq, p.inserted);
      fwrite(extra.data(), 1, extra.size(), fo);
      p.extra = extra.size();
    }
    copy_bytes(fi, fo, p.end - p.begin - p.last, in_name.c_str());
    if (p.last) {
      if (fread(page.data(), 1, p.last, fi) != p.last || page_len(page.data(), p.last) != p.last)
        fail("%s: changed while splitting", in_name.c_str());
      page[5] |= kEos;
      set_crc(page.data(), p.last);
      fwrite(page.data(), 1, p.last, fo);
    }
    close_out(fo, name);
  }
  if (fgetc(fi) != EOF) fail("%s: changed while splitting", in_name.c_str());
  fclose(fi);

  std::string meta_name = prefix + ".meta";
  FILE* fm = open_out(meta_name);
  for (size_t k = 1; k < parts.size(); k++) put_varint(fm, parts[k].inserted);
  close_out(fm, meta_name);

  if (an.big)
    fprintf(stderr, "oggsplit: warning: %llu Vorbis streams with setup headers over %u bytes are not cut\n",
            (ull)an.big, (unsigned)kMaxHeader);
  if (parts.size() < N)
    fprintf(stderr, "oggsplit: only %llu part%s possible\n", (ull)parts.size(), parts.size() == 1 ? " is" : "s are");
  if (verbose) {
    fprintf(stderr,
            "%llu page%s, %llu bytes of junk in %llu run%s, %llu link%s (%llu Vorbis), %llu samples, %llu cut "
            "point%s (%llu where links start)\n",
            (ull)an.pages, plural(an.pages), (ull)an.junk_bytes, (ull)an.junk, plural(an.junk),
            (ull)an.links.size(), plural(an.links.size()), (ull)an.vorbis, (ull)an.samples, (ull)an.cuts.size(),
            plural(an.cuts.size()), (ull)an.link_cuts);
    for (size_t k = 0; k < parts.size(); k++)
      fprintf(stderr, "%s: %llu bytes at %llu, %llu page%s, +%u bytes in %u page%s%s\n", part_name(prefix, k).c_str(),
              (ull)(parts[k].end - parts[k].begin), (ull)parts[k].begin, (ull)parts[k].pages,
              plural(parts[k].pages), (unsigned)parts[k].extra, (unsigned)parts[k].inserted,
              plural(parts[k].inserted), parts[k].last ? ", EOS set" : "");
  }
  return 0;
}

// reads a page from f into pg; false at the end of the file or if no page is there
bool read_page(FILE* f, std::vector<uint8_t>& pg) {
  pg.resize(kHdr);
  if (fread(pg.data(), 1, kHdr, f) != kHdr) return false;
  const size_t h = kHdr + pg[26];
  pg.resize(h);
  if (fread(pg.data() + kHdr, 1, h - kHdr, f) != h - kHdr) return false;
  size_t n = h;
  for (size_t i = kHdr; i < h; i++) n += pg[i];
  pg.resize(n);
  if (fread(pg.data() + h, 1, n - h, f) != n - h) return false;
  return page_len(pg.data(), n) == n;
}

int restore(bool verbose, const std::string& prefix, const std::string& out_name) {
  std::string meta = prefix + ".meta";
  FILE* fm = open_in(meta);
  std::vector<uint64_t> counts{0};  // pages inserted into each part
  for (int c; (c = fgetc(fm)) != EOF;) {
    uint64_t v = 0;
    for (int shift = 0;; shift += 7) {
      if (c == EOF || shift > 28) fail("%s: bad varint", meta.c_str());
      v |= (uint64_t)(c & 127) << shift;
      if (!(c & 128)) break;
      c = fgetc(fm);
    }
    counts.push_back(v);
  }
  if (ferror(fm)) fail("%s: read error", meta.c_str());
  fclose(fm);

  FILE* fo = open_out(out_name);
  std::vector<uint8_t> pg, buf(kMaxPage + (1 << 20));
  uint64_t removed = 0;
  for (size_t k = 0; k < counts.size(); k++) {
    std::string name = part_name(prefix, k);
    FILE* fi = open_in(name);
    uint64_t at = 0;
    for (uint64_t n = 0; n < counts[k]; n++) {  // the inserted pages
      if (!read_page(fi, pg) || (!n && !(pg[5] & kBos))) {
        if (ferror(fi)) fail("%s: read error", name.c_str());
        fail("%s: no inserted header page at offset %llu", name.c_str(), (ull)at);
      }
      at += pg.size();
    }
    removed += at;
    // the rest, but for the last kMaxPage bytes if the last page has to be found
    const size_t keep = k + 1 < counts.size() && counts[k + 1] ? kMaxPage : 0;
    size_t have = 0;
    for (;;) {
      size_t n = fread(buf.data() + have, 1, buf.size() - have, fi);
      have += n;
      if (have > keep) {
        fwrite(buf.data(), 1, have - keep, fo);
        memmove(buf.data(), buf.data() + (have - keep), keep);
        have = keep;
      }
      if (!n) break;
    }
    if (ferror(fi)) fail("%s: read error", name.c_str());
    fclose(fi);
    if (keep) {
      size_t p = last_page(buf.data(), have);
      if (p == kNone || !(buf[p + 5] & kEos)) fail("%s: does not end with a page with the EOS flag", name.c_str());
      buf[p + 5] &= (uint8_t)~kEos;
      set_crc(&buf[p], have - p);
      fwrite(buf.data(), 1, have, fo);
    }
  }
  close_out(fo, out_name);
  if (verbose)
    fprintf(stderr, "%llu part%s, %llu bytes of inserted header pages removed\n", (ull)counts.size(),
            plural(counts.size()), (ull)removed);
  return 0;
}

void usage() {
  fprintf(stderr,
          "usage: oggsplit N [options] input.ogg output_prefix\n"
          "       oggsplit r [options] input_prefix output.ogg\n"
          "  N   split an Ogg Vorbis file into at most N parts that are valid Ogg\n"
          "      Vorbis files, cutting where links start and between the audio pages\n"
          "      of a Vorbis stream; the parts are <prefix>.NNN.ogg, the information\n"
          "      to join them is <prefix>.meta\n"
          "  r   join the parts back into the original file\n"
          "options:\n"
          "  -v  print statistics\n");
}

}  // namespace

int main(int argc, char** argv) {
  bool verbose = false;
  std::vector<std::string> files;
  for (int i = 2; i < argc; i++) {
    if (argv[i][0] == '-' && argv[i][1]) {
      for (const char* p = argv[i] + 1; *p; p++) {
        if (*p == 'v') {
          verbose = true;
        } else {
          usage();
          return 2;
        }
      }
    } else {
      files.push_back(argv[i]);
    }
  }
  if (argc < 2 || files.size() != 2) {
    usage();
    return 2;
  }
  crc_init();
  if (!strcmp(argv[1], "r")) return restore(verbose, files[0], files[1]);
  char* e;
  unsigned long n = strtoul(argv[1], &e, 10);
  if (*e || !n || n > 100000 || !isdigit((unsigned char)argv[1][0])) {
    usage();
    return 2;
  }
  return split(n, verbose, files[0], files[1]);
}
