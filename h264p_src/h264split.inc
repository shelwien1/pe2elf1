
// h264split.inc -- h264split's stream analysis as a streaming splitter
// coroutine for MTwrap's parts mode (rzwrap -DRZW_H264SPLIT).
//
// The analysis (NAL scanning, SPS/PPS/slice-header parsing, finding access
// units of IDR pictures, and which parameter sets to copy into a part) is
// h264split.cpp's (h264split-1), unchanged; only the file-based scan_nals()
// became the incremental Scanner below, and choose_cuts() (which needs the
// whole stream) became a greedy rule: a part ends at the first cut point
// (an IDR access unit, or with allow_i an I one) at least `chunksize` bytes
// after its start, or, with a cap, at the first picture at least `cap`
// bytes after it (h264p: a stream with few IDR access units still comes in
// parts of a bounded size, at the cost of a part that starts with no
// reference pictures), or where no picture came within half again of the
// cap, at any NAL unit, or where none came within twice the cap, there.
//
// Output: one frame per part, for MTwrap's parts mode:
//   u64 L, u64 ins, u64 xlen, u64 nins, u64 nal0, part[L-32]
// part = the stream bytes [begin,end) with the parameter sets its slices
// refer to, as defined before it (their PPSs and the SPSs of those: nins
// NAL units, each with a 4-byte start code, xlen bytes in all), at ins:
// after its access unit delimiter, if it starts with one
// (h264p::aud_end()), else first; nal0 the index in the stream of its
// first NAL unit (for messages). Every part but the first starts at an access unit (or after
// a cut at the cap's half again, at a NAL unit, or at twice the cap,
// anywhere), with the parameter sets it needs.

// (h264p: the system headers it needs come first, from h264p.inc; and
// h264split's own copy of unescape() is h264p's, annexb.inc. Its parser of
// the headers stays its own, apart from h264p's walk, which needs a whole
// context: it tells pictures apart by 7.4.1.2.4, where h264p's x264
// padding counts slices with first_mb_in_slice 0 -- where they differ, a
// part is cut elsewhere, and parts are coded losslessly whatever they hold.
// Its Scanner finds start codes in the buffers it is fed, across their
// ends, which annexb.inc's reader does in a buffer of its own.)

namespace h264s {

using h264p::high_profile;
using h264p::unescape;

struct Bits {
  const uint8_t* p;
  size_t n, pos;  // n and pos in bits
  bool err;
  void Init(const h264p::Buf<uint8_t>& v) {
    p = v.data();
    n = v.size() * 8;
    pos = 0;
    err = false;
  }
  uint32_t u(int k) {
    uint32_t v = 0;
    while (k-- > 0) {
      int b = 0;
      if (pos < n)
        b = h264p::bit_at(p, pos), pos++;
      else
        err = true;
      v = v << 1 | b;
    }
    return v;
  }
  uint32_t ue() {
    int z = 0;
    while (!u(1)) {
      if (err || ++z > 31) {
        err = true;
        return 0;
      }
    }
    return z ? (1u << z) - 1 + u(z) : 0;
  }
  int32_t se() {
    uint32_t k = ue();
    return k & 1 ? (int32_t)((k >> 1) + 1) : -(int32_t)(k >> 1);
  }
};

struct Sps {
  int id, separate_colour_plane, log2_max_frame_num, poc_type, log2_max_poc_lsb, delta_pic_order_always_zero,
      frame_mbs_only;
  void Init() {
    id = separate_colour_plane = 0, log2_max_frame_num = 4, poc_type = 0, log2_max_poc_lsb = 4;
    delta_pic_order_always_zero = 0, frame_mbs_only = 1;
  }
};

struct Pps {
  int id, sps_id, bottom_field_pic_order_in_frame_present, redundant_pic_cnt_present;
  void Init() { id = sps_id = bottom_field_pic_order_in_frame_present = redundant_pic_cnt_present = 0; }
};

// seq_parameter_set_data() up to frame_mbs_only_flag; false if it is bad
bool parse_sps(const h264p::Buf<uint8_t>& rbsp, Sps& s) {
  Bits b;
  b.Init(rbsp);
  int profile = b.u(8);
  b.u(16);
  s.id = b.ue();
  if (s.id > 31) return false;
  if (high_profile(profile)) {
    uint32_t cf = b.ue();
    if (cf > 3) return false;
    if (cf == 3) s.separate_colour_plane = b.u(1);
    b.ue();
    b.ue();
    b.u(1);
    if (b.u(1))
      for (int i = 0; i < (cf != 3 ? 8 : 12) && !b.err; i++)
        if (b.u(1)) {
          int last = 8, next = 8;
          for (int j = 0; j < (i < 6 ? 16 : 64) && !b.err; j++) {
            if (next) next = (last + b.se() + 256) & 255;
            if (next) last = next;
          }
        }
  }
  uint32_t v = b.ue();
  if (v > 12) return false;
  s.log2_max_frame_num = v + 4;
  s.poc_type = b.ue();
  if (s.poc_type == 0) {
    v = b.ue();
    if (v > 12) return false;
    s.log2_max_poc_lsb = v + 4;
  } else if (s.poc_type == 1) {
    s.delta_pic_order_always_zero = b.u(1);
    b.se();
    b.se();
    v = b.ue();
    if (v > 255) return false;
    for (uint32_t i = 0; i < v && !b.err; i++) b.se();
  } else if (s.poc_type != 2) {
    return false;
  }
  b.ue();
  b.u(1);
  b.ue();
  b.ue();
  s.frame_mbs_only = b.u(1);
  return !b.err;
}

// pic_parameter_set_rbsp() up to redundant_pic_cnt_present_flag
bool parse_pps(const h264p::Buf<uint8_t>& rbsp, Pps& s) {
  Bits b;
  b.Init(rbsp);
  s.id = b.ue();
  s.sps_id = b.ue();
  if (s.id > 255 || s.sps_id > 31) return false;
  b.u(1);
  s.bottom_field_pic_order_in_frame_present = b.u(1);
  uint32_t groups = b.ue() + 1;
  if (groups > 8) return false;
  if (groups > 1) {
    uint32_t type = b.ue();
    if (type == 0) {
      for (uint32_t i = 0; i < groups; i++) b.ue();
    } else if (type == 2) {
      for (uint32_t i = 0; i + 1 < groups; i++) b.ue(), b.ue();
    } else if (type >= 3 && type <= 5) {
      b.u(1);
      b.ue();
    } else if (type == 6) {
      uint32_t units = b.ue() + 1;
      int bits = 0;
      while ((1u << bits) < groups) bits++;
      if ((uint64_t)units * bits > b.n) return false;
      for (uint32_t i = 0; i < units; i++) b.u(bits);
    } else if (type != 1) {
      return false;
    }
  }
  b.ue();
  b.ue();
  b.u(3);
  b.se();
  b.se();
  b.se();
  b.u(2);
  s.redundant_pic_cnt_present = b.u(1);
  return !b.err;
}

struct SliceInfo {
  int level;  // 0: nothing parsed, 1: up to pic_parameter_set_id, 2: all
  uint32_t first_mb, type, pps_id;
  int ref_idc, idr;
  uint32_t frame_num, idr_pic_id, poc_lsb, redundant;
  int field, bottom, poc_type;
  int32_t dpoc_bottom, dpoc0, dpoc1;
  void Init() { memset((void*)this, 0, sizeof *this); }  // (all 0)
};

// whether b, which follows a, is the first slice of a new primary coded picture
bool new_picture(const SliceInfo& a, const SliceInfo& b) {
  if (a.level < 2 || b.level < 2) return b.first_mb == 0;
  if (a.frame_num != b.frame_num || a.pps_id != b.pps_id || a.field != b.field) return true;
  if (a.field && a.bottom != b.bottom) return true;
  if ((a.ref_idc == 0) != (b.ref_idc == 0)) return true;
  if (a.poc_type == 0 && b.poc_type == 0 && (a.poc_lsb != b.poc_lsb || a.dpoc_bottom != b.dpoc_bottom))
    return true;
  if (a.poc_type == 1 && b.poc_type == 1 && (a.dpoc0 != b.dpoc0 || a.dpoc1 != b.dpoc1)) return true;
  if (a.idr != b.idr) return true;
  if (a.idr && a.idr_pic_id != b.idr_pic_id) return true;
  return false;
}

bool is_intra(uint32_t slice_type) { return slice_type % 5 == 2 || slice_type % 5 == 4; }

// ---------------------------------------------------------------------------
// Annex B scanning

struct Nal {
  uint64_t cut;  // start of the zero bytes before its start code
  uint64_t pos;  // offset of the NAL unit header byte
  uint64_t len;  // length up to its last nonzero byte
  h264p::Buf<uint8_t> head;  // its first bytes; all of them for a parameter set
  void Init() {
    cut = pos = len = 0;
    head.Init();
  }
  void Quit() { head.Quit(); }
  int type() const { return head.empty() ? -1 : head[0] & 31; }
};

bool is_pset(int type) { return type == 7 || type == 8 || type == 13 || type == 15; }

const size_t kHeadBytes = 256;      // enough for the start of a slice header
const size_t kMaxPset = 1u << 16;   // larger "parameter sets" are not tracked (the largest PPS, a slice
                                    // group map of 139,264 units of 3 bits, takes 52 KB)

// ---------------------------------------------------------------------------
// Finding the cut points

uint32_t pset_key(int type, int id) { return (uint32_t)type << 8 | (uint32_t)id; }
// the place of a parameter set's key in the tables by key (kSlots of them,
// in the order of the keys: SPS 7, PPS 8, SPS extension 13, subset SPS 15)
const int kSlots = 4 * 256;
int pset_slot(uint32_t key) {
  int t = (int)(key >> 8);
  return (t == 7 ? 0 : t == 8 ? 1 : t == 13 ? 2 : 3) * 256 + (int)(key & 255);
}

// a set of parameter sets' keys (by slot: fixed, however many a run of
// them holds)
struct KeySet {
  uint64_t w[kSlots / 64];
  void clear() { memset(w, 0, sizeof w); }
  void add(uint32_t key) {
    int i = pset_slot(key);
    w[i >> 6] |= uint64_t(1) << (i & 63);
  }
  bool has(int slot) const { return w[slot >> 6] >> (slot & 63) & 1; }
};

// A new version of a parameter set, at the NAL unit starting at `cut`.
struct PsetEvent {
  uint64_t cut;
  uint32_t key;
  size_t off, len;  // the NAL unit (Analyzer::ev_bytes)
};

struct Candidate {
  bool sync;      // an IDR access unit (or an I one, with allow_i)
  uint64_t cut;   // start of the access unit
  uint64_t nal;   // its first NAL unit's index in the stream
  KeySet keys;     // parameter sets defined before its first slice
  void Init() {
    sync = false;
    cut = nal = 0;
    keys.clear();
  }
};
// A slice that refers to a PPS (its id), at the NAL unit starting at cut,
// and to that PPS's SPS (its id, as the PPS was then; -1 not known)
struct PpsUse {
  uint64_t cut;
  uint32_t pps;
  int sps;
};

struct Analyzer {
  bool allow_i;
  Sps sps[32];
  Pps pps[256];
  bool has_sps[32], has_pps[256];
  size_t latest[kSlots];  // by key: the index in events + 1 of its last version (0 none)
  h264p::Buf<PsetEvent> events;
  h264p::Buf<uint8_t> ev_bytes;  // their NAL units
  // the cut point the last picture ended (cand, where has_cand: one at a
  // time, as end_picture() makes one and the splitter looks at it, which
  // it does after each NAL unit)
  Candidate cand;
  bool has_cand;
  uint64_t pictures;
  h264p::Buf<PpsUse> uses;  // the slices since the last cut (H264Split::emit())
  uint64_t nals;            // the NAL units so far (the one nal() has: nals - 1)

  // the non-VCL NAL units since the last VCL NAL unit that may start an
  // access unit
  bool run, force_new;
  Candidate run_au;
  // the current picture
  Candidate pic;
  bool pic_ok, pic_idr, pic_intra;
  SliceInfo prev;
  bool have_prev;
  h264p::Buf<uint8_t> rbsp;  // a NAL unit's

  void Init(bool allow) {
    allow_i = allow;
    memset(has_sps, 0, sizeof has_sps);
    memset(has_pps, 0, sizeof has_pps);
    memset(latest, 0, sizeof latest);
    events.Init();
    ev_bytes.Init();
    cand.Init();
    has_cand = false;
    pictures = 0;
    nals = 0;
    uses.Init();
    run = force_new = false;
    run_au.Init();
    pic.Init();
    pic_ok = pic_idr = pic_intra = false;
    prev.Init();
    have_prev = false;
    rbsp.Init();
  }
  void Quit() {
    events.Quit();
    ev_bytes.Quit();
    uses.Quit();
    rbsp.Quit();
  }

  void begin_run(const Nal& n, bool aud) {
    run = true;
    run_au.cut = n.cut;
    run_au.nal = nals - 1;
    run_au.keys.clear();
    if (aud) force_new = true;
  }

  void pset(const Nal& n) {
    int t = n.type();
    if (n.len > kMaxPset) return;
    unescape(n.head.data() + 1, n.head.size() - 1, rbsp);
    int id = -1;
    if (t == 7) {
      Sps s;
      s.Init();
      if (parse_sps(rbsp, s)) sps[s.id] = s, has_sps[s.id] = true, id = s.id;
    } else if (t == 8) {
      Pps s;
      s.Init();
      if (parse_pps(rbsp, s)) pps[s.id] = s, has_pps[s.id] = true, id = s.id;
    } else {
      Bits b;
      b.Init(rbsp);
      if (t == 15) b.u(24);
      uint32_t v = b.ue();
      if (!b.err && v <= 31) id = (int)v;
    }
    if (id < 0) return;
    uint32_t key = pset_key(t, id);
    run_au.keys.add(key);
    size_t& last = latest[pset_slot(key)];
    if (last) {
      const PsetEvent& e = events[last - 1];
      if (e.len == n.head.size() && !memcmp(ev_bytes.data() + e.off, n.head.data(), e.len)) return;
    }
    last = events.size() + 1;
    events.push_back(PsetEvent{n.cut, key, ev_bytes.size(), n.head.size()});
    ev_bytes.append(n.head.data(), n.head.size());
  }

  void parse_slice(const Nal& n, SliceInfo& s) {
    unescape(n.head.data() + 1, n.head.size() - 1, rbsp);
    Bits b;
    b.Init(rbsp);
    s.ref_idc = n.head[0] >> 5 & 3;
    s.idr = n.type() == 5;
    s.first_mb = b.ue();
    s.type = b.ue();
    s.pps_id = b.ue();
    if (b.err || s.type > 9) return;
    s.level = 1;
    if (s.pps_id > 255 || !has_pps[s.pps_id] || !has_sps[pps[s.pps_id].sps_id]) return;
    const Pps& p = pps[s.pps_id];
    const Sps& q = sps[p.sps_id];
    if (q.separate_colour_plane) b.u(2);
    s.frame_num = b.u(q.log2_max_frame_num);
    if (!q.frame_mbs_only) {
      s.field = b.u(1);
      if (s.field) s.bottom = b.u(1);
    }
    if (s.idr) s.idr_pic_id = b.ue();
    s.poc_type = q.poc_type;
    if (q.poc_type == 0) {
      s.poc_lsb = b.u(q.log2_max_poc_lsb);
      if (p.bottom_field_pic_order_in_frame_present && !s.field) s.dpoc_bottom = b.se();
    }
    if (q.poc_type == 1 && !q.delta_pic_order_always_zero) {
      s.dpoc0 = b.se();
      if (p.bottom_field_pic_order_in_frame_present && !s.field) s.dpoc1 = b.se();
    }
    if (p.redundant_pic_cnt_present) s.redundant = b.ue();
    if (!b.err) s.level = 2;
  }

  void end_picture() {
    if (!pictures) return;
    if (pictures > 1 && pic_ok) {
      pic.sync = pic_idr || (allow_i && pic_intra);
      cand = pic;
      has_cand = true;
    }
  }

  void slice(const Nal& n) {
    SliceInfo s;
    s.Init();
    parse_slice(n, s);
    if (s.level >= 1 && s.pps_id <= 255) uses.push_back(PpsUse{n.cut, s.pps_id, has_pps[s.pps_id] ? pps[s.pps_id].sps_id : -1});
    if (s.redundant) {  // a slice of a redundant coded picture
      run = false;
      return;
    }
    bool newpic = !have_prev || force_new || new_picture(prev, s);
    if (newpic) {
      end_picture();
      pictures++;
      if (run) {
        pic = run_au;
      } else {
        pic.cut = n.cut;
        pic.nal = nals - 1;
        pic.keys.clear();
      }
      pic_ok = s.level == 2;
      pic_idr = s.idr;
      pic_intra = s.level >= 1 && is_intra(s.type);
    } else {
      pic_idr &= s.idr;
      pic_intra &= s.level >= 1 && is_intra(s.type);
    }
    prev = s;
    have_prev = true;
    run = force_new = false;
  }

  void nal(const Nal& n) {
    int t = n.type();
    nals++;
    switch (t) {
      case 9:
        begin_run(n, true);
        break;
      case 7:
      case 8:
      case 13:
      case 15:
        if (!run) begin_run(n, false);
        pset(n);
        break;
      case 6:
      case 14:
      case 16:
      case 17:
      case 18:
        if (!run) begin_run(n, false);
        break;
      case 1:
      case 2:
      case 5:
        slice(n);
        break;
      default:  // slice data partitions B and C, extension slices, end of
                // sequence/stream, filler data, unspecified and reserved
        run = false;
        break;
    }
  }
};


// scan_nals() of h264split.cpp, fed one buffer at a time
struct Scanner {
  uint64_t off;   // stream offset of the next byte fed
  uint64_t zrun;  // zero bytes at the end of the data so far
  bool in_nal;
  Nal cur;

  void Init() {
    off = zrun = 0;
    in_nal = false;
    cur.Init();
  }
  void Quit() { cur.Quit(); }

  void append(const uint8_t* p, size_t n) {
    if (!in_nal) return;
    while (n) {
      size_t cap = cur.head.empty() ? 1 : is_pset(cur.head[0] & 31) ? kMaxPset + 1 : kHeadBytes;
      if (cur.head.size() >= cap) return;
      size_t k = n < cap - cur.head.size() ? n : cap - cur.head.size();
      cur.head.append(p, k);
      p += k, n -= k;
    }
  }
  template <class F>
  void finish(uint64_t end, F& on_nal) {
    cur.len = end - cur.pos;
    if (cur.head.size() > cur.len) cur.head.resize((size_t)cur.len);
    on_nal(cur);
  }
  template <class F>
  void feed(const uint8_t* b, size_t n, F& on_nal) {
    size_t i = 0;
    while (i < n) {
      const uint8_t* q = (const uint8_t*)memchr(b + i, 1, n - i);
      size_t p = q ? (size_t)(q - b) : n;
      if (p > i) {
        append(b + i, p - i);
        size_t t = p;
        while (t > i && !b[t - 1]) t--;
        zrun = t == i ? zrun + (p - i) : p - t;
      }
      if (p == n) break;
      if (zrun >= 2) {  // start code
        uint64_t cut = off + p - zrun;
        if (in_nal) finish(cut, on_nal);
        in_nal = true;
        cur.cut = cut;
        cur.pos = off + p + 1;
        cur.head.clear();
      } else {
        append(b + p, 1);
      }
      zrun = 0;
      i = p + 1;
    }
    off += n;
  }
  template <class F>
  void end(F& on_nal) {
    if (in_nal) finish(off - zrun, on_nal);
    in_nal = false;
  }
};

}  // namespace h264s

struct H264Split : Coroutine {

  // The state lives on the heap, behind a pointer, Init() and Quit() its
  // (the object may sit in memory that is never constructed: rzwrap's
  // static union; h264p's coro_new() maps it, on zeroed pages)
  struct State {
    h264s::Scanner sc;
    h264s::Analyzer an;
    h264p::Buf<uint8_t> buf;       // stream bytes from `begin` on
    size_t state[h264s::kSlots];   // by key: the parameter set before the last cut (an.events' index + 1; 0 none)
    size_t ev;
    uint64_t begin;                // the current part
    uint64_t nal0;                 // its first NAL unit's index in the stream
    h264s::KeySet keys;            // the parameter sets its first access unit defines
    h264p::Buf<uint8_t> extra;     // (emit()'s)
    uint64_t parts, total;
    void Init( bool allow_i ) {
      sc.Init();
      an.Init( allow_i );
      buf.Init();
      memset( state, 0, sizeof state );
      ev = 0;
      begin = nal0 = 0;
      keys.clear();
      extra.Init();
      parts = total = 0;
    }
    void Quit( void ) {
      sc.Quit();
      an.Quit();
      buf.Quit();
      extra.Quit();
    }
  };
  State* st;
  qword chunksize;
  qword cap; // 0: none

  uint Init( qword _chunksize, uint allow_i, qword _cap=0 ) {
    coro_init();
    chunksize = _chunksize;
    cap = _cap;
    st = (State*)h264p::mem_alloc( sizeof(State) );
    st->Init( allow_i!=0 );
    return 0;
  }

  void Quit( void ) {
    if( st ) st->Quit(), h264p::mem_free( st );
    st = 0;
  }

  void put_bytes( const byte* p, size_t n ) {
    while( n ) {
      chkout(); uint l = uint( Min<size_t>( n, getoutleft() ) );
      memcpy( (byte*)outptr, p, l ); outptr+=l; p+=l; n-=l;
    }
  }

  // The current part, up to stream offset `end`, as a frame, with the
  // parameter sets it needs copied in: the PPSs its slices refer to and
  // their SPSs (each id's SPS, SPS extension and subset SPS), as defined
  // before it, where its first access unit does not define them; by type
  // (SPS, SPS extension, subset SPS, PPS), each by id, where aud_end() puts
  // them (a part with no slice gets none)
  void emit( uint64_t end ) {
    using namespace h264s;
    State& s = *st;
    Analyzer& an = s.an;
    uint64_t n = end - s.begin;
    bool used[256] = {}, used_sps[32] = {}, use[kSlots] = {};
    size_t k = 0;
    for( ; k<an.uses.size() && an.uses[k].cut<end; k++ ) {
      used[an.uses[k].pps] = true;
      if( an.uses[k].sps>=0 && an.uses[k].sps<32 ) used_sps[an.uses[k].sps] = true;
    }
    an.uses.erase_front( k );
    for( int i=0; i<kSlots; i++ )  // (slots 256..511: the PPSs; the others' ids are SPS ids, below 32)
      if( s.state[i] && !s.keys.has(i) ) use[i] = i/256==1 ? used[i%256] : i%256<32 && used_sps[i%256];
    s.extra.clear();
    uint32_t nins = 0;
    static const int order[4] = {7, 13, 15, 8};
    for( int t : order )
      for( int id=0; id<256; id++ ) {
        int i = pset_slot( pset_key(t, id) );
        if( !use[i] ) continue;
        const PsetEvent& e = an.events[s.state[i]-1];
        s.extra.append( h264p::kStartCode4, 4 );
        s.extra.append( an.ev_bytes.data()+e.off, e.len );
        nins++;
      }
    size_t ins = nins ? h264p::aud_end( s.buf.data(), size_t(n) ) : 0;
    qword L = 32 + n + s.extra.size(), h[4] = {ins, s.extra.size(), nins, s.nal0};
    put_bytes( (const byte*)&L, 8 );
    put_bytes( (const byte*)h, 32 );
    put_bytes( s.buf.data(), ins );
    put_bytes( s.extra.data(), s.extra.size() );
    put_bytes( s.buf.data()+ins, size_t(n-ins) );
    s.buf.erase_front( size_t(n) );
    s.begin = end;
    s.parts++;
  }

  // a new part at cut point c: the parameter sets defined before it, and
  // in its first access unit
  void start_part( const h264s::Candidate& c ) {
    using namespace h264s;
    State& s = *st;
    Analyzer& an = s.an;
    while( s.ev<an.events.size() && an.events[s.ev].cut<c.cut ) s.state[pset_slot(an.events[s.ev].key)] = s.ev+1, s.ev++;
    s.keys = c.keys;
    s.nal0 = c.nal;
    trim_events();
  }

  // The versions of parameter sets no part can copy any more (before the
  // last cut, each key's but its last) go, once they are most of what
  // an.events holds: what stays is a version of each key (s.state's) and
  // those of the part under way (of its own bytes), however long the stream
  // and however many versions it brings
  void trim_events( void ) {
    using namespace h264s;
    State& s = *st;
    Analyzer& an = s.an;
    const size_t w = sizeof(PsetEvent);  // (an event's weight: its bytes and this)
    size_t live = (an.events.size()-s.ev)*w + (s.ev<an.events.size() ? an.ev_bytes.size()-an.events[s.ev].off : 0);
    for( int i=0; i<kSlots; i++ ) if( s.state[i] ) live += w+an.events[s.state[i]-1].len;
    if( an.events.size()*w+an.ev_bytes.size() < 2*live+(1<<16) ) return;
    size_t j = 0, off = 0, ev = 0;
    for( size_t i=0; i<an.events.size(); i++ ) {  // (in place: what stays moves to the front, in order)
      PsetEvent e = an.events[i];
      int k = pset_slot( e.key );
      if( i<s.ev && s.state[k]!=i+1 ) continue;
      memmove( an.ev_bytes.data()+off, an.ev_bytes.data()+e.off, e.len );
      e.off = off, off += e.len;
      // (renumbered to an index no larger: no later event of the key is taken for it)
      if( s.state[k]==i+1 ) s.state[k] = j+1;
      if( an.latest[k]==i+1 ) an.latest[k] = j+1;
      an.events[j++] = e;
      if( i<s.ev ) ev = j;
    }
    an.events.resize( j );
    an.ev_bytes.resize( off );
    s.ev = ev;
  }
  void cut( const h264s::Candidate& c ) { emit( c.cut ); start_part( c ); }

  // after the NAL unit n: a cut at the cut point of the picture that ended,
  // if any (none is looked at again, nor one behind the last cut); or, with
  // a cap, where none came within half again of it (pictures whose headers
  // the splitter cannot parse, or as large), at the access unit of the
  // picture under way, or within one that started before the part, at n
  void check_cands( const h264s::Nal& n ) {
    State& s = *st;
    h264s::Analyzer& an = s.an;
    if( an.has_cand ) {
      const h264s::Candidate& c = an.cand;
      an.has_cand = false;
      if( c.cut>s.begin && ((c.sync && c.cut-s.begin>=chunksize) || (cap && c.cut-s.begin>=cap)) ) { cut( c ); return; }
    }
    if( !cap || n.cut<=s.begin || n.cut-s.begin<cap+cap/2 ) return;
    if( an.pictures && an.pic.cut>s.begin ) { cut( an.pic ); return; }
    h264s::Candidate c;
    c.Init();
    c.cut = n.cut;
    c.nal = an.nals - 1;
    cut( c );
  }

  void do_process( void ) {
    State& s = *st;
    auto on_nal = [this]( const h264s::Nal& n ) { st->an.nal(n); check_cands(n); };

    while(1) {
      chkinp(); uint l = getinplen();
      if( l==0 ) break; // EOF
      const byte* p = (const byte*)inpptr;
      s.buf.append( p, l );
      s.sc.feed( p, l, on_nal ); // may emit parts (and yield 2); p stays valid: no input is requested meanwhile
      inpptr += l;
      // (a stretch that check_cands() could not cut, with no start code or
      // one NAL unit past the cap, is cut at twice the cap: a part may then
      // start inside a NAL unit, whose bytes its model codes as they are)
      while( cap && s.sc.off-s.begin>=2*cap ) {
        h264s::Candidate c;
        c.Init();
        c.cut = s.begin+2*cap;
        c.nal = s.an.nals+s.sc.in_nal;  // (the part's first whole NAL unit)
        cut( c );
      }
    }
    s.sc.end( on_nal );
    s.an.end_picture();
    h264s::Nal none;
    none.Init();
    check_cands( none );
    none.Quit();
    s.total = s.sc.off;
    if( s.total>s.begin ) emit( s.total );

    yield(this,0);
  }

};
