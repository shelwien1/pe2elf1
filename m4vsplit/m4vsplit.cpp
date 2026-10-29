// m4vsplit: splits an MPEG-4 Part 2 video elementary stream (.m4v: XviD, DivX, libavcodec mpeg4,
// ...) into parts that are valid streams by themselves, for processing them in parallel, and
// joins them back.
//
//   m4vsplit N [options] input.m4v output_prefix
//   m4vsplit r [options] input_prefix output.m4v
//
//   -i  also cut before I-VOPs that B-VOPs follow (those reference the previous part)
//   -v  print statistics
//
// The stream is cut into chunks at start codes (00 00 01 xx): a chunk runs from its start code to
// the next. The cuts are made before I-VOPs that start a closed group, where the next VOP isn't a
// B-VOP (a B-VOP that follows an I-VOP is displayed before it, and references the VOP before it
// too) -- with -i, before any I-VOP -- or rather before the run of headers right before such an
// I-VOP (VOS, VO, video object, VOL, user data, GOV), choosing among those the cuts that make the
// largest part as small as possible. There are fewer than N parts when there are not enough cut
// points. The parts are <prefix>.000.m4v, <prefix>.001.m4v, ...
//
// A part gets a copy of the headers that are known at its start and that its first run of
// headers does not define itself, inserted at its start: the last VOS, VO, video object and VOL
// headers (each start code value by itself), and the last user data of each kind (by its first 4
// bytes: "DivX", "XviD", "Lavc", ...), which decoders read to work around encoder bugs; in that
// order, the user data in the order its kinds first came. Restoring removes them and checks that
// they are headers.
//
// <prefix>.meta holds only what restoring cannot find out from the parts: for each part after
// the first, the number of chunks inserted into it, as a LEB128 varint (one byte up to 127). The
// number of parts is one more than the number of values; the first part never gets any. The
// count cannot be told from the part, since its first run of headers may define the same headers
// again.
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

[[noreturn]] void fail(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fputs("m4vsplit: ", stderr);
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

// ---------------------------------------------------------------------------
// Start codes

enum : int {
  kVos = 0xB0,       // visual_object_sequence_start_code
  kVosEnd = 0xB1,    // visual_object_sequence_end_code
  kUserData = 0xB2,  // user_data_start_code
  kGov = 0xB3,       // group_of_vop_start_code
  kVo = 0xB5,        // visual_object_start_code
  kVop = 0xB6,       // vop_start_code
};

// whether a chunk with the start code c is a header that may come before a VOP: VOS, VO, video
// object (00..1F), VOL (20..2F), user data, GOV
bool is_header(int c) { return c == kVos || c == kVo || c == kUserData || c == kGov || (c >= 0 && c < 0x30); }
// whether it is one a part gets a copy of (not a GOV, which goes with its group)
bool is_copied(int c) { return is_header(c) && c != kGov; }

struct Chunk {
  uint64_t pos = 0;  // offset of its start code (0 for the bytes before the first one)
  uint64_t len = 0;  // up to the next start code
  int code = -1;     // the start code's last byte; -1 for the bytes before the first start code
  std::vector<uint8_t> head;  // its first bytes; all of them for a header
};

const size_t kHeadBytes = 32;     // enough for a VOP's type and whether it is coded
const size_t kMaxHeader = 1u << 20;  // larger "headers" are not tracked

// Calls on_chunk for each chunk of the stream; returns its size.
template <class F>
uint64_t scan_chunks(FILE* f, F&& on_chunk) {
  std::vector<uint8_t> buf(1 << 20);
  uint64_t off = 0;  // offset of buf[0]
  int zeros = 0;     // zero bytes at the end of the data so far (up to 2)
  Chunk cur;         // the chunk under way
  size_t cap = kMaxHeader + 1;  // its bytes kept: kMaxHeader + 1 until its code is known
  auto append = [&](const uint8_t* p, size_t n) {
    if (cur.head.size() >= cap) return;
    size_t k = std::min(n, cap - cur.head.size());
    cur.head.insert(cur.head.end(), p, p + k);
    if (cur.code < 0 && cur.pos == 0 && cur.head.size() < 4) return;
    if (cur.head.size() >= 4 && cur.head[0] == 0 && cur.head[1] == 0 && cur.head[2] == 1) {
      cur.code = cur.head[3];
      cap = is_header(cur.code) ? kMaxHeader + 1 : kHeadBytes;
    }
  };
  bool first = true;
  for (;;) {
    size_t n = fread(buf.data(), 1, buf.size(), f);
    if (!n) break;
    const uint8_t* b = buf.data();
    size_t i = 0;
    while (i < n) {
      // a start code ends at a 01 after two zeros (counting those before this buffer)
      const uint8_t* q = (const uint8_t*)memchr(b + i, 1, n - i);
      size_t p = q ? (size_t)(q - b) : n;
      if (p > i) {
        size_t t = p;
        while (t > i && !b[t - 1]) t--;
        zeros = t == i ? std::min(zeros + (int)(p - i), 2) : (int)std::min<size_t>(p - t, 2);
      }
      if (p == n) {
        append(b + i, n - i);
        break;
      }
      if (zeros >= 2) {  // a start code at off + p - 2
        uint64_t sc = off + p - 2;
        // the chunk under way ends at sc: its bytes up to there (those before this buffer are in it)
        if (p - i >= 2) append(b + i, p - 2 - i);
        else cur.head.resize(cur.head.size() - std::min(cur.head.size(), (size_t)(2 - (p - i))));
        if (!first || sc > 0) {
          cur.len = sc - cur.pos;
          if (cur.head.size() > cur.len) cur.head.resize((size_t)cur.len);
          on_chunk(cur);
        }
        first = false;
        cur = Chunk();
        cur.pos = sc;
        cap = kMaxHeader + 1;
        static const uint8_t kSc[3] = {0, 0, 1};
        append(kSc, 3);
      } else {
        append(b + i, p + 1 - i);
      }
      zeros = 0;
      i = p + 1;
    }
    off += n;
  }
  if (ferror(f)) fail("read error");
  if (off > cur.pos || !first) {
    cur.len = off - cur.pos;
    if (cur.head.size() > cur.len) cur.head.resize((size_t)cur.len);
    on_chunk(cur);
  }
  return off;
}

// ---------------------------------------------------------------------------
// Finding the cut points

// What a copied header is: its start code, or for user data, its first 4 bytes after it (its kind)
uint64_t header_key(const Chunk& c) {
  if (c.code != kUserData) return (uint64_t)c.code << 32;
  uint64_t k = (uint64_t)kUserData << 32 | 1u << 31;
  for (size_t i = 4; i < 8; i++) k |= (uint64_t)(i < c.head.size() ? c.head[i] : 0) << (8 * (7 - i));
  return k;
}
// the order headers are inserted in: VOS, VO, video object, VOL, user data (by first appearance)
int key_rank(uint64_t key) {
  int c = (int)(key >> 32);
  return c == kVos ? 0 : c == kVo ? 1 : c < 0x20 ? 2 : c < 0x30 ? 3 : 4;
}

// A new version of a header, in the chunk at `cut`.
struct HeaderEvent {
  uint64_t cut;
  uint64_t key;
  std::vector<uint8_t> data;  // the chunk
};

struct Candidate {
  uint64_t cut;   // start of the run of headers before the I-VOP, or of the I-VOP
  uint64_t mbs;   // macroblocks of the VOPs before it
  uint64_t vops;  // VOPs before it
  bool closed;    // the next VOP isn't a B-VOP
  std::vector<uint64_t> keys;  // headers defined before its I-VOP
};

// What the VOP syntax depends on of a VOL (ISO/IEC 14496-2 video_object_layer(), up to
// sprite_enable): the frame size, the length of vop_time_increment, whether sprites are GMC.
// ok: whether it parsed (a rectangular VOL; the time increment's length even if not).
struct VolInfo {
  int w = 0, h = 0, tib = 0, gmc = 0;
  bool ok = false;
};

// reads bits from a chunk, its start code skipped
struct Bits {
  const std::vector<uint8_t>& d;
  size_t pos = 32;
  bool err = false;
  explicit Bits(const std::vector<uint8_t>& v) : d(v) {}
  uint32_t u(int k) {
    uint32_t v = 0;
    while (k-- > 0) {
      int b = 0;
      if (pos < d.size() * 8)
        b = d[pos >> 3] >> (7 - (pos & 7)) & 1, pos++;
      else
        err = true;
      v = v << 1 | b;
    }
    return v;
  }
  uint32_t show(int k) {
    size_t p = pos;
    bool e = err;
    uint32_t v = u(k);
    pos = p, err = e;
    return v;
  }
};

VolInfo vol_info(const std::vector<uint8_t>& d) {
  VolInfo r;
  Bits b(d);
  b.u(1);  // random_accessible_vol
  b.u(8);  // video_object_type_indication
  int ver = 1;
  if (b.u(1)) ver = b.u(4), b.u(3);  // is_object_layer_identifier: verid, priority
  if (b.u(4) == 15) b.u(16);         // aspect_ratio_info, par_width, par_height
  if (b.u(1)) {                      // vol_control_parameters
    b.u(3);                          // chroma_format, low_delay
    if (b.u(1)) b.u(79);             // vbv_parameters
  }
  int shape = b.u(2);
  if (shape == 3 && ver != 1) b.u(4);
  b.u(1);
  uint32_t res = b.u(16);
  b.u(1);
  r.tib = 1;  // (as FFmpeg: the bits of res - 1, at least 1)
  while (res > 1 && (1u << r.tib) < res) r.tib++;
  if (b.err || !res) return r;
  if (b.u(1)) b.u(r.tib);  // fixed_vop_rate: fixed_vop_time_increment
  if (shape != 0) return r;
  b.u(1);
  r.w = (int)b.u(13);
  b.u(1);
  r.h = (int)b.u(13);
  b.u(1);
  b.u(2);  // interlaced, obmc_disable
  r.gmc = (ver == 1 ? b.u(1) : b.u(2)) == 2;
  r.ok = !b.err && r.w > 0 && r.h > 0;
  return r;
}

// Whether a VOP (of type t) is coded (vop_coded), the time increment taking tib bits, unless it
// does not fit there, where tib is guessed again as FFmpeg does; -1 if its header is cut short
int vop_coded(const std::vector<uint8_t>& d, int t, int& tib, int gmc) {
  Bits b(d);
  b.u(2);
  for (int n = 0; b.u(1); n++)  // modulo_time_base
    if (b.err || n > 1000) return -1;
  b.u(1);
  if (!(b.show(tib + 1) & 1)) {
    for (tib = 1; tib < 16; tib++) {
      if (t == 1 || (t == 3 && gmc)) {
        if ((b.show(tib + 6) & 0x37) == 0x30) break;
      } else if ((b.show(tib + 5) & 0x1F) == 0x18) {
        break;
      }
    }
  }
  b.u(tib + 1);
  int coded = b.u(1);
  return b.err ? -1 : coded;
}

struct Analyzer {
  bool allow_i = false;
  std::map<uint64_t, size_t> latest;  // key -> index in events
  std::vector<HeaderEvent> events;
  std::vector<Candidate> cands;
  long chunks = 0, vops = 0, by_type[4] = {0, 0, 0, 0}, not_coded = 0, open = 0, untracked = 0;
  uint64_t mbs = 0;  // macroblocks of the VOPs so far
  uint64_t mb_num = 0;  // of the last VOL
  int tib = 0, gmc = 0;  // the length of vop_time_increment (0 before a VOL); GMC sprites

  // the run of headers since the last chunk that is not one
  bool run = false;
  uint64_t run_cut = 0;
  std::vector<uint64_t> run_keys;
  // an I-VOP waiting for the next VOP's type
  bool pending = false;
  Candidate pend;

  void header(const Chunk& c) {
    if (!run) {
      run = true;
      run_cut = c.pos;
      run_keys.clear();
    }
    if (c.code >= 0x20 && c.code < 0x30 && c.len <= kMaxHeader) {
      VolInfo v = vol_info(c.head);
      tib = v.tib;
      gmc = v.gmc;
      if (v.ok) mb_num = (uint64_t)((v.w + 15) / 16) * ((v.h + 15) / 16);
    }
    if (!is_copied(c.code)) return;
    if (c.len > kMaxHeader) {
      untracked++;
      return;
    }
    uint64_t key = header_key(c);
    run_keys.push_back(key);
    auto it = latest.find(key);
    if (it != latest.end() && events[it->second].data == c.head) return;
    latest[key] = events.size();
    events.push_back(HeaderEvent{c.pos, key, c.head});
  }

  void vop(const Chunk& c) {
    int t = c.head.size() > 4 ? c.head[4] >> 6 : -1;  // I, P, B, S
    int coded = t >= 0 && tib ? vop_coded(c.head, t, tib, gmc) : -1;  // (-1: taken to be coded)
    if (!coded) {  // a placeholder, as packed B-VOPs leave (it changes no reference VOP)
      not_coded++;
      vops++;
      run = false;
      return;
    }
    if (pending) {
      pend.closed = t != 2;
      if (!pend.closed) open++;
      if (pend.closed || allow_i) cands.push_back(pend);
      pending = false;
    }
    if (t == 0 && coded == 1) {
      Candidate k;
      k.cut = run ? run_cut : c.pos;
      k.mbs = mbs;
      k.vops = vops;
      k.closed = true;
      if (run) k.keys = run_keys;
      if (k.cut > 0) pending = true, pend = k;
    }
    if (t >= 0) by_type[t]++;
    vops++;
    mbs += mb_num;
    run = false;
  }

  void chunk(const Chunk& c) {
    chunks++;
    if (c.code == kVop)
      vop(c);
    else if (is_header(c.code))
      header(c);
    else
      run = false;  // the bytes before the first start code, a VOS end, or anything else
  }

  void end() {
    if (pending) cands.push_back(pend);  // (no VOP after it)
    pending = false;
  }
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

// The cost of processing a part, for balancing: kByteCost per byte plus 1 per macroblock (of the
// coded VOPs, at the size of the last VOL). m4vpack c takes 0.76 us per byte plus 0.16 us per
// macroblock (fitted over 12 streams of 90 KB to 5.9 MB), so a macroblock costs about a fifth of a
// byte, as in h264split: a part of many small pictures takes longer than one of the same size
// with few large ones.
const uint64_t kByteCost = 5;

std::string part_name(const std::string& prefix, size_t i) {
  char b[32];
  snprintf(b, sizeof b, ".%03u.m4v", (unsigned)i);
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

// copies the rest of fi to fo
void copy_rest(FILE* fi, FILE* fo, const char* what) {
  static std::vector<uint8_t> buf(1 << 20);
  size_t n;
  while ((n = fread(buf.data(), 1, buf.size(), fi)) > 0) fwrite(buf.data(), 1, n, fo);
  if (ferror(fi)) fail("%s: read error", what);
}

void put_varint(FILE* f, uint64_t v) {
  while (v > 127) fputc((int)(v & 127) | 128, f), v >>= 7;
  fputc((int)v, f);
}

int split(size_t N, bool allow_i, bool verbose, const std::string& in_name, const std::string& prefix) {
  FILE* fi = open_in(in_name);
  Analyzer an;
  an.allow_i = allow_i;
  uint64_t total = scan_chunks(fi, [&](const Chunk& c) { an.chunk(c); });
  an.end();
  fclose(fi);

  std::vector<uint64_t> x{0};
  for (const Candidate& c : an.cands) x.push_back(c.cut * kByteCost + c.mbs);
  x.push_back(total * kByteCost + an.mbs);
  std::vector<size_t> chosen = choose_cuts(x, N);

  struct Part {
    uint64_t begin, end, vops, chunks = 0;  // chunks: inserted
    std::vector<uint8_t> extra;
  };
  std::vector<Part> parts;
  std::map<uint64_t, size_t> state;  // headers before the cut
  std::map<uint64_t, size_t> first;  // the event of a key's first version (the order of user data)
  for (size_t e = 0; e < an.events.size(); e++) first.insert(std::make_pair(an.events[e].key, e));
  size_t ev = 0;
  uint64_t begin = 0;
  for (size_t k = 0; k <= chosen.size(); k++) {
    Part p;
    p.begin = begin;
    p.end = k < chosen.size() ? an.cands[chosen[k] - 1].cut : total;
    p.vops = (k < chosen.size() ? an.cands[chosen[k] - 1].vops : an.vops) - (k ? an.cands[chosen[k - 1] - 1].vops : 0);
    if (k) {
      const Candidate& c = an.cands[chosen[k - 1] - 1];
      while (ev < an.events.size() && an.events[ev].cut < c.cut) state[an.events[ev].key] = ev, ev++;
      std::set<uint64_t> defined(c.keys.begin(), c.keys.end());
      std::vector<std::pair<std::pair<int, size_t>, size_t> > use;  // (rank, first event), the event to copy
      for (auto& s : state)
        if (!defined.count(s.first)) use.push_back(std::make_pair(std::make_pair(key_rank(s.first), first[s.first]), s.second));
      std::sort(use.begin(), use.end());
      for (auto& u : use) {
        const std::vector<uint8_t>& d = an.events[u.second].data;
        p.extra.insert(p.extra.end(), d.begin(), d.end());
        p.chunks++;
      }
    }
    parts.push_back(p);
    begin = p.end;
  }

  fi = open_in(in_name);
  for (size_t k = 0; k < parts.size(); k++) {
    const Part& p = parts[k];
    std::string name = part_name(prefix, k);
    FILE* fo = open_out(name);
    if (!p.extra.empty()) fwrite(p.extra.data(), 1, p.extra.size(), fo);
    copy_bytes(fi, fo, p.end - p.begin, in_name.c_str());
    close_out(fo, name);
  }
  if (fgetc(fi) != EOF) fail("%s: changed while splitting", in_name.c_str());
  fclose(fi);

  std::string meta_name = prefix + ".meta";
  FILE* fm = open_out(meta_name);
  for (size_t k = 1; k < parts.size(); k++) put_varint(fm, parts[k].chunks);
  close_out(fm, meta_name);

  if (an.untracked) fprintf(stderr, "m4vsplit: warning: %ld headers longer than %u bytes are not copied\n", an.untracked, (unsigned)kMaxHeader);
  if (parts.size() < N) fprintf(stderr, "m4vsplit: only %u parts are possible\n", (unsigned)parts.size());
  if (verbose) {
    fprintf(stderr,
            "%ld chunks, %ld VOPs (I %ld, P %ld, B %ld, S %ld, not coded %ld), %llu macroblocks, %u cut points (%ld "
            "I-VOPs that B-VOPs follow%s)\n",
            an.chunks, an.vops, an.by_type[0], an.by_type[1], an.by_type[2], an.by_type[3], an.not_coded, (ull)an.mbs,
            (unsigned)an.cands.size(), an.open, allow_i ? "" : ", not used");
    for (size_t k = 0; k < parts.size(); k++)
      fprintf(stderr, "%s: %llu bytes at %llu, %llu VOPs, +%u bytes in %u headers\n", part_name(prefix, k).c_str(),
              (ull)(parts[k].end - parts[k].begin), (ull)parts[k].begin, (ull)parts[k].vops,
              (unsigned)parts[k].extra.size(), (unsigned)parts[k].chunks);
  }
  return 0;
}

// The first bytes of a part, read as far as needed.
struct PartHead {
  FILE* f;
  const char* name;
  std::vector<uint8_t> buf;
  bool eof = false;

  bool more() {
    if (eof) return false;
    size_t old = buf.size();
    buf.resize(old + (1 << 16));
    size_t n = fread(&buf[old], 1, 1 << 16, f);
    buf.resize(old + n);
    if (!n) {
      if (ferror(f)) fail("%s: read error", name);
      eof = true;
    }
    return n > 0;
  }
  bool have(size_t n) {
    while (buf.size() < n)
      if (!more()) return false;
    return true;
  }
  // the first start code at or after i (the index of its first 0), or npos
  size_t start_code(size_t i) {
    for (;;) {
      for (; i + 2 < buf.size(); i++)
        if (!buf[i] && !buf[i + 1] && buf[i + 2] == 1) return i;
      if (!more()) return std::string::npos;
    }
  }
};

int restore(bool verbose, const std::string& prefix, const std::string& out_name) {
  std::string meta = prefix + ".meta";
  FILE* fm = open_in(meta);
  std::vector<uint64_t> counts{0};  // chunks inserted into each part
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
  uint64_t removed = 0;
  for (size_t k = 0; k < counts.size(); k++) {
    std::string name = part_name(prefix, k);
    FILE* fi = open_in(name);
    if (counts[k]) {
      PartHead h{fi, name.c_str(), {}};
      size_t pos = 0;  // the inserted chunks: from the start, each up to the next start code
      for (uint64_t n = 0; n < counts[k]; n++) {
        if (!h.have(pos + 4) || h.buf[pos] || h.buf[pos + 1] || h.buf[pos + 2] != 1 || !is_copied(h.buf[pos + 3]))
          fail("%s: no inserted header at offset %llu", name.c_str(), (ull)pos);
        size_t j = h.start_code(pos + 3);
        if (j == std::string::npos) fail("%s: ends after the inserted headers", name.c_str());
        pos = j;
      }
      fwrite(h.buf.data() + pos, 1, h.buf.size() - pos, fo);
      removed += pos;
    }
    copy_rest(fi, fo, name.c_str());
    fclose(fi);
  }
  close_out(fo, out_name);
  if (verbose)
    fprintf(stderr, "%u parts, %llu bytes of inserted headers removed\n", (unsigned)counts.size(), (ull)removed);
  return 0;
}

void usage() {
  fprintf(stderr,
          "usage: m4vsplit N [options] input.m4v output_prefix\n"
          "       m4vsplit r [options] input_prefix output.m4v\n"
          "  N   split an MPEG-4 Part 2 video stream into at most N parts that are\n"
          "      valid streams, cutting before I-VOPs that no B-VOP follows (or the\n"
          "      headers right before them); the parts are <prefix>.NNN.m4v, the\n"
          "      information to join them is <prefix>.meta\n"
          "  r   join the parts back into the original stream\n"
          "options:\n"
          "  -i  also cut before I-VOPs that B-VOPs follow (those reference the\n"
          "      previous part)\n"
          "  -v  print statistics\n");
}

}  // namespace

int main(int argc, char** argv) {
  bool allow_i = false, verbose = false;
  std::vector<std::string> files;
  for (int i = 2; i < argc; i++) {
    if (argv[i][0] == '-' && argv[i][1]) {
      for (const char* p = argv[i] + 1; *p; p++) {
        if (*p == 'i')
          allow_i = true;
        else if (*p == 'v')
          verbose = true;
        else {
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
  if (!strcmp(argv[1], "r")) return restore(verbose, files[0], files[1]);
  char* e;
  unsigned long n = strtoul(argv[1], &e, 10);
  if (*e || !n || n > 100000 || !isdigit((unsigned char)argv[1][0])) {
    usage();
    return 2;
  }
  return split(n, allow_i, verbose, files[0], files[1]);
}
