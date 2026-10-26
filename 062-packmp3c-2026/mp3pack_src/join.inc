// The joiner of d: mp3rs (mp3rw/mp3rs.inc) as a coroutine whose input is
// the dump the model decodes, and which reads the outside data straight
// from its file, through a buffer, not through a second input of the
// coroutine (mp3rs's pin 2). Its output is the file restored.
//
// What a broken stream decodes to is checked before mp3rs writes it: a
// frame is written only as mp3rw's parser could have read it (frame_ok()),
// so that no value of a broken stream sends the writer outside its tables
// or buffers, and the outside data must hold what the dump asks of it, and
// end where the dump does. Else the run ends with an error (yield 3, its
// note saying why); the CRC of the input, at the stream's end, then
// catches whatever else is not as it was (drivers.inc).

namespace mp3pack {

// Bytes read straight from a file, through a buffer (the outside data)
struct FileIn {
  enum { kSize = 1 << 16 };
  FILE* f;
  uint8_t* buf;
  size_t n, pos;
  uint64_t total;  // the bytes taken
  bool eof;

  void Init(FILE* g) {
    f = g;
    buf = (uint8_t*)mem_alloc(kSize);
    n = pos = 0;
    total = 0;
    eof = false;
  }
  void Quit() {
    mem_free(buf);
    buf = nullptr;
  }
  // the next byte, -1 at the end
  int get() {
    if (pos == n) {
      if (eof) return -1;
      n = fread(buf, 1, kSize, f);
      pos = 0;
      if (!n) {
        eof = true;
        return -1;
      }
    }
    total++;
    return buf[pos++];
  }
};

struct Mp3Join : mp3parse {
#include "frame_dump_io.inc"

  FileIn* xi;  // the outside data
  uint64_t frames;
  const char* why;  // the error the run ended with

  void Init(FileIn* i) {
    coro_init();
    Output();
    init_frame_dump();
    xi = i;
    frames = 0;
    why = nullptr;
  }

  // the run ends with an error
  NOINLINE void fail(const char* w) {
    why = w;
    for (;;) yield(this, 3);
  }

  // the outside data, as mp3rs's het() and hetv() read it
  uint het() {
    int c = xi->get();
    if (c < 0) fail("the outside data ends before the stream does: a wrong or broken file");
    return (uint)c;
  }
  qword hetv() {
    qword r = 0;
    for (uint l = 0, c = 0; (c & 0x80) == 0; l += 7) {
      c = het();
      if (l > 63) fail("the outside data is broken: a number that does not end");
      r |= qword(c & 0x7F) << l;
    }
    return r;
  }
  void read_garbage() {
    qword g = hetv();
    if (g > 4096 * 8 && g != ~(qword)0) fail("the outside data is broken: the bits after a run of frames");
    GrbLen = (int)g;
    uint i, l = (GrbLen + 7) / 8;
    for (i = 0; i < l; i++) Garbage[i] = het();
  }

  // Whether the frame (the dump's record) is one mp3rw's parser could have
  // read: its header, side information, garbage and layer I/II data in
  // the ranges the parser takes, that the writer's tables are made for.
  bool frame_ok() {
    if (GrbLen < -1 || GrbLen >= 4096 * 8) return false;
    if (h.syncword != 2047 || h.layer == 0 || (h.ID == 1 && h.IDex == 0)) return false;
    if (h.bitrate == 0 || h.bitrate == 15 || h.frequency == 3) return false;
    if (h.layer != 1) return l12_ok();
    const int ngr = h.ID, nch = (h.mode != 3);
    if (si.main_data_begin < 0 || si.main_data_begin >= (ngr ? 512 : 256)) return false;
    if (si.backup_mdb < 0 || si.backup_mdb >= (ngr ? 512 : 256)) return false;
    if ((unsigned)si.p23cleared > 1) return false;
    for (int gr = 0; gr <= ngr; gr++)
      for (int ch = 0; ch <= nch; ch++) {
        const gr_info& g = si.gi[gr][ch];
        if ((unsigned)g.part2_3_length > 4095 || (unsigned)g.backup_p23 > 4095) return false;
        if ((unsigned)g.big_values > 576 / 2 || (unsigned)g.global_gain > 255) return false;
        if ((unsigned)g.scalefac_compress >= (ngr ? 16u : 512u)) return false;
        if ((unsigned)g.window_switching_flag > 1 || (unsigned)g.block_type > 3 || (unsigned)g.mixed_block_flag > 1) return false;
        for (int i = 0; i < 3; i++)
          if ((unsigned)g.table_select[i] > 31 || g.table_select[i] == 4 || g.table_select[i] == 14 ||
              (unsigned)g.subblock_gain[i] > 7)
            return false;
        if ((unsigned)g.region0_count > 15 || (unsigned)g.region1_count > 22) return false;
        if (!g.window_switching_flag && (g.region1_count > 7 || g.region0_count + g.region1_count + 2 > 22)) return false;
        if ((unsigned)g.preflag > 1 || (unsigned)g.scalefac_scale > 1 || (unsigned)g.count1table_select > 1) return false;
      }
    return true;
  }
  // a layer I/II frame (parse_l12.inc's ProcL12()): the allocation of
  // each subband in the bits of its table, and the codes of its samples
  bool l12_ok() {
    if (h.IDex == 0 || h.emphasis == 2) return false;
    const int lay1 = (h.layer == 3), nch = (h.mode != 3) + 1;
    const int tab = lay1 ? 0 : L2Table();
    const int sblimit = lay1 ? 32 : l2_sblimit[tab];
    for (int sb = 0; sb < sblimit; sb++) {
      const int nb = L12Nbal(tab, sb);
      for (int ch = 0; ch < nch; ch++) {
        const int b = L.alloc[ch][sb];
        if (b >= (1 << nb) || (lay1 && b == 15) || L.scfsi[ch][sb] > 3) return false;
        for (int k = 0; k < 3; k++)
          if (L.scf[ch][sb][k] > 63) return false;
        if (!b) continue;
        int bits, levels = 0;
        if (lay1) {
          bits = b + 1;
        } else {
          const int k = l2_rows[l2_row[tab][sb]][b];
          bits = l2_qbits[k];
          if (bits < 0) levels = l2_levels[k];
        }
        for (int s = 0; s < (lay1 ? 12 : 36); s++) {
          const int x = L.smp[ch][sb][s];
          if (levels ? x >= levels : x >= (1 << bits)) return false;
        }
      }
    }
    return true;
  }

  // whether GrbLen bits, written from bit pos of the writer's buffer up
  // (GarbageFlush()), stay in it. (putbits() ors each in through 4 bytes,
  // whose bytes past the bits are 0: a run of bits that ends at the
  // buffer's end goes past it by those, unchanged, as it always did.)
  bool garbage_fits(int pos) { return GrbLen <= 0 || (pos + GrbLen + 7) / 8 <= (int)sizeof(Out); }

  void do_process() {
    qword len;
    uint f_EOF = 0, f_skip = 1;
    while (f_EOF == 0) {
      while (read_frame_dump(len, f_EOF) == 0) {
        if (!frame_ok() || (h.layer == 1 && !garbage_fits(OutPtr + (si.p23cleared ? 0 : si.main_data_begin * 8))))
          fail("a broken stream: a frame that is not one");
        frames++;
        WriteFrame();
      }
      if (f_skip)
        f_skip = 0;
      else {
        read_garbage();
        if (Hdr1 != Hdr2 && !garbage_fits(OutPtr)) fail("the outside data is broken: the bits after a run of frames");
        WriteFlush();
      }
      outpos += len;
      while (len--) put(het());
      if (!f_EOF) outlen = outpos + hetv();  // size limit for WriteFrame/WriteFlush
    }
    if (xi->get() >= 0) fail("the outside data goes on past the stream's: a wrong or broken file");
    yield(this, 0);
  }
};

}  // namespace mp3pack
