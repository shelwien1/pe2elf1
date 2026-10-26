// The splitter of c: mp3rw's parse (mp3rw/mp3rw.inc) as a coroutine whose
// output is the dump of the frames (frame_dump_io.inc's records, not
// padded: FRAME_DUMP_ALIGN 0), which mp3c's model codes. What is not
// frames (the bytes between runs of frames, each run's length, the bits
// after its last frame) is the outside data, mp3rw's .out byte for byte:
// written straight to its file, through a buffer, not through a second
// output of the coroutine (mp3rw's pin 2).
//
// With -t (parts), the records go out in parts, for MTwrap's parts mode
// (MTwrap.md section 11): a part ends before the first record at least -c's
// part size of the input after its start, or at most kDumpCap times that of
// records (a record is about 5.5 KB a frame, 4 to 80 times the frame), and
// goes out as a frame of its own:
//
//   u64 L       the bytes after it: 24 and the part's records
//   u64 at      where in the input the part starts
//   u64 raw     the input it stands for, to the next part's start
//   u64 frames  its frames
//
// A part's records start a model of their own (mt.inc), which needs none of
// the records before them: a cut can be between any two.

namespace mp3pack {

// Bytes written straight to a file, through a buffer (the outside data)
struct FileOut {
  enum { kSize = 1 << 16 };
  FILE* f;
  uint8_t* buf;
  size_t n;
  uint64_t total;  // the bytes put
  bool err;        // a write failed

  void Init(FILE* g) {
    f = g;
    buf = (uint8_t*)mem_alloc(kSize);
    n = 0;
    total = 0;
    err = false;
  }
  void Quit() {
    mem_free(buf);
    buf = nullptr;
  }
  void put(uint8_t c) {
    if (n == kSize) flush();
    buf[n++] = c;
    total++;
  }
  void flush() {
    if (n && fwrite(buf, 1, n, f) != n) err = true;
    n = 0;
  }
};

// A part's records at most, in times the part size of input
const uint64_t kDumpCap = 16;

struct Mp3Split : mp3parse {
  FileOut* xo;      // the outside data
  bool parts;       // -t: the records in parts
  uint64_t chunk;   // the part size (of the input)
  Vec<uint8_t> part;  // the part under way: its frame's header room, then its records
  uint64_t part_at, part_frames;  // where it starts in the input, its frames
  uint64_t frames, runs;          // the frames and their runs so far
  uint64_t nparts;
  uint8_t rec[sizeof(mp3frame0) + 32];  // a record

  void Init(FileOut* o, bool _parts, uint64_t _chunk) {
    coro_init();
    Input();
    xo = o;
    parts = _parts;
    chunk = _chunk;
    part.Init();
    part.resize(32);
    part_at = part_frames = 0;
    frames = runs = nparts = 0;
  }
  void Quit() { part.Quit(); }

  // the outside data, as mp3rw's qut() and qutv() write it
  void qut(uint c) { xo->put((uint8_t)c); }
  void qutv(qword c) {
    for (uint flag = 0; flag == 0; c >>= 7) qut((c & 0x7F) | (flag = (c < 128) << 7));
  }
  void write_garbage() {
    qutv(GrbLen);
    uint i, l = (GrbLen + 7) / 8;
    for (i = 0; i < l; i++) qut(Garbage[i]);
  }

  // a number of a record, as frame_dump_io.inc's putv(): c+1, 7 bits a
  // byte, the last with its top bit set
  static size_t rec_v(uint8_t* p, uint64_t c) {
    size_t k = 0;
    c += 1;
    for (uint flag = 0; flag == 0; c >>= 7) p[k++] = (uint8_t)((c & 0x7F) | (flag = (c < 128) << 7));
    return k;
  }
  // the frame's record: GrbLen, then the frame from h to its garbage
  size_t frame_rec() {
    const byte* p = (const byte*)&GrbLen;
    size_t k = rec_v(rec, (uint64_t)(int64_t)GrbLen);
    size_t n = (size_t)(((const byte*)&Garbage - p) + (GrbLen + 7) / 8);
    memcpy(rec + k, p + 4, n - 4);
    return k + n - 4;
  }
  // the record of the bytes before a run of frames (binlen*2), or of the
  // end (binlen*2+1)
  size_t mark_rec(uint64_t v) {
    size_t k = rec_v(rec, 4096 * 8);
    return k + rec_v(rec + k, v);
  }
  // a record to the output, or to the part under way
  void put_rec(size_t n) {
    if (parts) {
      part.append(rec, n);
      return;
    }
    for (size_t i = 0; i < n;) {
      chkout();
      size_t l = Min<size_t>(n - i, getoutleft());
      memcpy(outptr, rec + i, l);
      outptr += l, i += l;
    }
  }
  // the part under way out, as a frame, its input ending at end
  void emit(uint64_t end) {
    uint8_t* h = part.data();
    put_le(h, part.size() - 8, 8);
    put_le(h + 8, part_at, 8);
    put_le(h + 16, end - part_at, 8);
    put_le(h + 24, part_frames, 8);
    for (size_t i = 0, n = part.size(); i < n;) {
      chkout();
      size_t l = Min<size_t>(n - i, getoutleft());
      memcpy(outptr, part.data() + i, l);
      outptr += l, i += l;
    }
    part.resize(32);
    part_at = end;
    part_frames = 0;
    nparts++;
  }
  // before a record at pos of the input: the part's end, if it is due
  void cut(uint64_t pos) {
    if (parts && part.size() > 32 && (pos - part_at >= chunk || part.size() - 32 >= kDumpCap * chunk)) emit(pos);
  }

  void do_process() {
    uint f_mp3 = 0;
    qword mp3len = 0, binlen = 0;
    for (;;) {
      qword r_pos = inppos - eofcount - (BufPtr / 8);
      int r = ReadFrame();
      if (r == 0) {
        cut(r_pos);
        // the first frame of a run: the record of the bytes before it
        if (f_mp3 == 0) {
          put_rec(mark_rec(binlen * 2 + 0));
          mp3len = r_pos;
          runs++;
        }
        put_rec(frame_rec());
        f_mp3 = 1;
        frames++, part_frames++;
      } else {
        if (f_mp3) {
          ReadFlush();
          qutv(r_pos - mp3len);
          f_mp3 = 0;
          binlen = 0;
          write_garbage();
          GrbLen = 0;  // stored now; the next run's first frame would repeat it
        }
        if (r == 1) {
          put_rec(mark_rec(binlen * 2 + 1));
          break;
        }
        if (r == 3) continue;  // layer III -> I/II: that frame starts the next run
        qut(r);
        binlen++;
      }
    }
    xo->flush();
    if (parts) emit(inppos - eofcount);
    yield(this, 0);
  }
};

}  // namespace mp3pack
