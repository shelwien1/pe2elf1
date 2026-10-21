
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
// reference pictures).
//
// Output: one frame per part, for MTwrap's parts mode:
//   u64 L, u32 nins, part[L-4]
// part = the stream bytes [begin,end) with the parameter sets h264split
// would insert (nins NAL units, each with a 4-byte start code) at the same
// place (after the access unit delimiter, if any). Every part but the first
// starts at an access unit, with the parameter sets it needs.

// (h264p: the system headers it needs come first, from h264p.inc; and
// h264split's own copy of unescape() is h264p's, annexb.inc. Its parser of
// the headers stays its own, apart from h264p's walk, which needs a whole
// context: it tells pictures apart by 7.4.1.2.4, where h264p's x264
// padding counts slices with first_mb_in_slice 0 -- where they differ, a
// part is cut elsewhere, and parts are coded losslessly whatever they hold.
// Its Scanner finds start codes in the buffers it is fed, across their
// ends, which annexb.inc's reader does in a buffer of its own.)

namespace h264s {

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
        b = p[pos >> 3] >> (7 - (pos & 7)) & 1, pos++;
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
  uint64_t mbs;  // macroblocks in a frame
  void Init() {
    id = separate_colour_plane = 0, log2_max_frame_num = 4, poc_type = 0, log2_max_poc_lsb = 4;
    delta_pic_order_always_zero = 0, frame_mbs_only = 1;
    mbs = 0;
  }
};

struct Pps {
  int id, sps_id, bottom_field_pic_order_in_frame_present, redundant_pic_cnt_present;
  void Init() { id = sps_id = bottom_field_pic_order_in_frame_present = redundant_pic_cnt_present = 0; }
};

bool high_profile(int p) {
  static const int list[] = {100, 110, 122, 244, 44, 83, 86, 118, 128, 138, 139, 134, 135};
  for (int x : list)
    if (p == x) return true;
  return false;
}

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
  uint64_t w = (uint64_t)b.ue() + 1, h = (uint64_t)b.ue() + 1;
  s.frame_mbs_only = b.u(1);
  s.mbs = w * h * (2 - s.frame_mbs_only);
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
  uint64_t mbs;  // macroblocks in the picture
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
const size_t kMaxPset = 1u << 20;   // larger "parameter sets" are not tracked

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

// A new version of a parameter set, at the NAL unit starting at `cut`.
struct PsetEvent {
  uint64_t cut;
  uint32_t key;
  int ref_sps;      // the SPS a PPS refers to, else -1
  size_t off, len;  // the NAL unit (Analyzer::ev_bytes)
};

struct Candidate {
  bool sync;      // an IDR access unit (or an I one, with allow_i)
  uint64_t cut;   // start of the access unit
  uint64_t mbs;   // macroblocks of the pictures before it
  uint64_t pics;  // pictures before it
  uint64_t ins;   // where to insert parameter sets, relative to cut
  h264p::Buf<uint32_t> keys;  // parameter sets defined before its first slice
  void Init() {
    sync = false;
    cut = mbs = pics = ins = 0;
    keys.Init();
  }
  void Quit() { keys.Quit(); }
  void copy(const Candidate& o) {
    sync = o.sync, cut = o.cut, mbs = o.mbs, pics = o.pics, ins = o.ins;
    keys.copy(o.keys);
  }
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
  long nals, pictures, idr, intra, untracked;
  uint64_t mbs;  // macroblocks of the pictures so far

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
    nals = pictures = idr = intra = untracked = 0;
    mbs = 0;
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
    cand.Quit();
    run_au.Quit();
    pic.Quit();
    rbsp.Quit();
  }

  void begin_run(const Nal& n, bool aud) {
    run = true;
    run_au.cut = n.cut;
    run_au.ins = aud ? n.pos + n.len - n.cut : 0;
    run_au.keys.clear();
    if (aud) force_new = true;
  }

  void pset(const Nal& n) {
    int t = n.type();
    if (n.len > kMaxPset) {
      untracked++;
      return;
    }
    unescape(n.head.data() + 1, n.head.size() - 1, rbsp);
    int id = -1, ref = -1;
    if (t == 7) {
      Sps s;
      s.Init();
      if (parse_sps(rbsp, s)) sps[s.id] = s, has_sps[s.id] = true, id = s.id;
    } else if (t == 8) {
      Pps s;
      s.Init();
      if (parse_pps(rbsp, s)) pps[s.id] = s, has_pps[s.id] = true, id = s.id, ref = s.sps_id;
    } else {
      Bits b;
      b.Init(rbsp);
      if (t == 15) b.u(24);
      uint32_t v = b.ue();
      if (!b.err && v <= 31) id = (int)v;
    }
    if (id < 0) {
      untracked++;
      return;
    }
    uint32_t key = pset_key(t, id);
    run_au.keys.push_back(key);
    size_t& last = latest[pset_slot(key)];
    if (last) {
      const PsetEvent& e = events[last - 1];
      if (e.len == n.head.size() && !memcmp(ev_bytes.data() + e.off, n.head.data(), e.len)) return;
    }
    last = events.size() + 1;
    events.push_back(PsetEvent{n.cut, key, ref, ev_bytes.size(), n.head.size()});
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
    s.mbs = s.field ? q.mbs / 2 : q.mbs;
  }

  void end_picture() {
    if (!pictures) return;
    idr += pic_idr;
    intra += pic_intra && !pic_idr;
    if (pictures > 1 && pic_ok) {
      pic.sync = pic_idr || (allow_i && pic_intra);
      cand.copy(pic);
      has_cand = true;
    }
  }

  void slice(const Nal& n) {
    SliceInfo s;
    s.Init();
    parse_slice(n, s);
    if (s.redundant) {  // a slice of a redundant coded picture
      run = false;
      return;
    }
    bool newpic = !have_prev || force_new || new_picture(prev, s);
    if (newpic) {
      end_picture();
      pictures++;
      if (run) {
        pic.copy(run_au);
      } else {
        pic.cut = n.cut;
        pic.ins = 0;
        pic.keys.clear();
      }
      pic.mbs = mbs;
      pic.pics = pictures - 1;
      mbs += s.mbs;
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
    nals++;
    int t = n.type();
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
    uint64_t ins;
    uint32_t nins;
    h264p::Buf<uint8_t> extra;
    uint64_t parts, total;
    void Init( bool allow_i ) {
      sc.Init();
      an.Init( allow_i );
      buf.Init();
      memset( state, 0, sizeof state );
      ev = 0;
      begin = ins = 0;
      nins = 0;
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

  // the current part, up to stream offset `end`, as a frame
  void emit( uint64_t end ) {
    State& s = *st;
    uint64_t n = end - s.begin;
    qword L = 4 + n + s.extra.size();
    put_bytes( (const byte*)&L, 8 );
    put_bytes( (const byte*)&s.nins, 4 );
    put_bytes( s.buf.data(), size_t(s.ins) );
    put_bytes( s.extra.data(), s.extra.size() );
    put_bytes( s.buf.data()+s.ins, size_t(n-s.ins) );
    s.buf.erase_front( size_t(n) );
    s.begin = end;
    s.parts++;
  }

  // a new part at cut point c: h264split's choice of parameter sets to
  // copy: those defined before it but not in its first access unit, and
  // the SPSs of the PPSs among them; by type (SPS, SPS extension, subset
  // SPS, PPS), each by id
  void start_part( const h264s::Candidate& c ) {
    using namespace h264s;
    State& s = *st;
    Analyzer& an = s.an;
    while( s.ev<an.events.size() && an.events[s.ev].cut<c.cut ) s.state[pset_slot(an.events[s.ev].key)] = s.ev+1, s.ev++;
    bool defined[kSlots] = {}, use[kSlots] = {}, use0[kSlots];
    for( uint32_t key : c.keys ) defined[pset_slot(key)] = true;
    for( int k=0; k<kSlots; k++ )
      if( s.state[k] && !defined[k] ) use[k] = true;
    memcpy( use0, use, sizeof use );
    static const int sps_types[2] = {7, 15};
    for( int k=0; k<kSlots; k++ ) { // the SPS of the PPS copied
      if( !use0[k] ) continue;
      int ref = an.events[s.state[k]-1].ref_sps;
      if( ref<0 ) continue;
      for( int t : sps_types )
        if( s.state[pset_slot(pset_key(t, ref))] ) use[pset_slot(pset_key(t, ref))] = true;
    }
    s.extra.clear(); s.nins = 0;
    static const int order[4] = {7, 13, 15, 8};
    for( int t : order )
      for( int id=0; id<256; id++ ) {
        int k = pset_slot( pset_key(t, id) );
        if( !use[k] ) continue;
        static const uint8_t sc4[4] = {0, 0, 0, 1};
        const PsetEvent& e = an.events[s.state[k]-1];
        s.extra.append( sc4, 4 );
        s.extra.append( an.ev_bytes.data()+e.off, e.len );
        s.nins++;
      }
    s.ins = s.extra.empty() ? 0 : c.ins;
  }

  // (the cut point of the picture that ended, if any: none is looked at again)
  void check_cands( void ) {
    State& s = *st;
    if( !s.an.has_cand ) return;
    const h264s::Candidate& c = s.an.cand;
    if( (c.sync && c.cut-s.begin>=chunksize) || (cap && c.cut-s.begin>=cap) ) { emit( c.cut ); start_part( c ); }
    s.an.has_cand = false;
  }

  void do_process( void ) {
    State& s = *st;
    auto on_nal = [this]( const h264s::Nal& n ) { st->an.nal(n); check_cands(); };

    while(1) {
      chkinp(); uint l = getinplen();
      if( l==0 ) break; // EOF
      const byte* p = (const byte*)inpptr;
      s.buf.append( p, l );
      s.sc.feed( p, l, on_nal ); // may emit parts (and yield 2); p stays valid: no input is requested meanwhile
      inpptr += l;
    }
    s.sc.end( on_nal );
    s.an.end_picture();
    check_cands();
    s.total = s.sc.off;
    if( s.total>s.begin ) emit( s.total );

    yield(this,0);
  }

};
