/*  Copyright (C) 2026 Kamila Szewczyk

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.  */

/*  An ordered stream of (tag, value) records held as a two-column TSV file.
    A run of records that share a tag becomes one row, the tag written once
    and the values separated by spaces:

        flr.y<TAB>26 13 11

    The writer appends; the reader replays in the same order.  Encoding and
    decoding walk one shared traversal, so the reader also checks that every
    tag is the one the caller expects and any desync is reported at the record
    where it happens rather than as corrupt output.  */

constexpr char TSV_MAGIC[] = "tag\tvalues";

/*  The meta is read by tsv2wav and by nothing else, so its tags do not have
    to be the ones balrogg chose -- and a tag is written once per row, which
    over a meta that changes tag every few values is most of what the tag
    costs.  Every tag that can reach a meta has a short name here, given out
    shortest-first by how many rows the tag takes, and the stream translates
    on the way out and back.  Nothing at the call sites changes, and the tee
    keeps handing over balrogg's names.

    Measured over 39 files this is 538,143 bytes, 3.55% of the meta.  */
struct tsv_short { const char * lng, * sht; };
static const tsv_short TSV_SHORT[] = {
  { "pk", "a" },
  { "kn", "b" },
  { "flr.d", "c" },
  { "kd.v", "d" },
  { "kc.v", "e" },
  { "flr.y", "f" },
  { "kc.i", "g" },
  { "kd.i", "h" },
  { "page.gran", "i" },
  { "page.npkt", "k" },
  { "page.plen", "l" },
  { "page.tail", "m" },
  { "page.spill", "n" },
  { "flr.cdim", "o" },
  { "flr.csub", "p" },
  { "flr.sbook", "q" },
  { "flr.cbook", "r" },
  { "link.more", "s" },
  { "mode.blockflag", "t" },
  { "mode.mapping", "u" },
  { "flr.type", "v" },
  { "flr.parts", "w" },
  { "flr.pclass", "x" },
  { "flr.mult", "y" },
  { "flr.rangebits", "z" },
  { "flr.x", "A" },
  { "res.type", "B" },
  { "res.begin", "C" },
  { "res.end", "D" },
  { "res.partsize", "E" },
  { "res.nclass", "F" },
  { "res.classbook", "G" },
  { "res.cascade", "H" },
  { "res.book", "I" },
  { "map.submaps", "J" },
  { "map.steps", "K" },
  { "map.smtime", "L" },
  { "map.smfloor", "M" },
  { "map.smres", "N" },
  { "cm.mode", "O" },
  { "link.frames", "P" },
  { "link.serial", "Q" },
  { "id.channels", "R" },
  { "id.rate", "S" },
  { "id.brmax", "T" },
  { "id.brnom", "U" },
  { "id.brmin", "V" },
  { "id.blocksize", "W" },
  { "id.framing", "X" },
  { "cmt.byte", "Y" },
  { "cb.count", "Z" },
  { "cb.i", "0" },
  { "flr.ntime", "1" },
  { "flr.count", "2" },
  { "res.count", "3" },
  { "map.count", "4" },
  { "mode.count", "5" },
  { "pg.pred", "aj" },
  { "cg.on", "ai" },
  { "dg.on", "6" },
  { "vf.q", "7" },
  { "page.eos", "8" },
  { "cm.a", "9" },
  { "cm.b", "aa" },
  { "map.mag", "ab" },
  { "map.ang", "ac" },
  { "aud.wpfix", "ad" },
  { "cm.n", "ae" },
  { "cm.f", "af" },
  { "cm.g", "ag" },
  { "cm.c", "ah" },
};
constexpr int TSV_NSHORT = (int) (sizeof TSV_SHORT / sizeof *TSV_SHORT);
constexpr sz   TSV_OBUF    = 1 << 20;
constexpr sz   TSV_IBUF    = 1 << 16;
constexpr sz   TSV_TAGMAX  = 31;

/*  Close a row once its values reach this many bytes and open another under
    the same tag.  No real stream comes near it; it only keeps a pathological
    run from becoming one enormous line.  */
constexpr sz TSV_ROWMAX = 8192;

/*  Reading is a byte tokenizer rather than a line reader, so a row may hold
    as many values as the writer cared to put on it.  */
static char tsv_ib[TSV_IBUF];
static char tsv_ob[TSV_OBUF];

struct tsv {
  FILE * f;
  const char * path;
  int enc;
  u64 row;                    /*  records written or read so far  */

  sz on;                      /*  bytes held in the write buffer  */
  sz rl;                      /*  value bytes on the row being written  */
  const char * wtag;          /*  its tag, or null between rows  */

  tsv * tee;                  /*  every record read is echoed here  */
  int terse;                  /*  the stream uses the short tags above  */
  /*  Records arrive in runs of one tag, so remembering the last one turns
      the lookup into a pointer compare nearly every time.  */
  const char * clng, * csht;

  const char * shorten(const char * tag) {
    int i;
    if (!terse) return tag;
    if (tag == clng) return csht;
    for (i = 0; i < TSV_NSHORT; i++)
      if (!strcmp(TSV_SHORT[i].lng, tag))
        { clng = tag;  csht = TSV_SHORT[i].sht;  return csht; }
    return tag;               /*  not a meta tag; it goes out as it came  */
  }
  static const char * lengthen(const char * sht) {
    int i;
    for (i = 0; i < TSV_NSHORT; i++)
      if (!strcmp(TSV_SHORT[i].sht, sht)) return TSV_SHORT[i].lng;
    return sht;
  }
  char * mem;                 /*  when set, output goes here, not to a file  */
  sz memcap, memlen;
  sz bytes;                   /*  what has been written, for trial runs  */
  sz ip, in;                  /*  cursor and fill of the read buffer  */
  char rtag[TSV_TAGMAX + 1];  /*  tag of the row being read  */
  int left;                   /*  that row still has a value to hand out  */

  void create(const char * pth) {
    f = fopen(pth, "wb");
    if (!f) FATAL_CODE(BLR_EXIT_IO, "cannot create %s", pth);
    path = pth;  enc = 1;  row = 0;  on = 0;  rl = 0;  wtag = nullptr;  bytes = 0;
    mem = nullptr;  memcap = memlen = 0;
    terse = 0;  clng = csht = nullptr;
    emit(TSV_MAGIC, sizeof TSV_MAGIC - 1);  emit("\n", 1);
  }

  /*  Write into memory instead of a file, so a section can be captured and
      compared against one generated from elsewhere.  */
  void create_mem(char * buf, sz cap) {
    f = nullptr;  path = "(memory)";  enc = 1;  row = 0;  on = 0;  rl = 0;
    wtag = nullptr;  bytes = 0;  mem = buf;  memcap = cap;  memlen = 0;
    terse = 0;  clng = csht = nullptr;
  }

  void emit(const char * s, sz n) {
    bytes += n;                 /*  so a trial run can be sized without a file  */
    if (mem) {
      FATAL_UNLESS(memlen + n <= memcap, "captured section is too large");
      memcpy(mem + memlen, s, n);  memlen += n;
      return;
    }
    if (on + n > TSV_OBUF) flush();
    /*  One write can be larger than the whole buffer: su_books hands over a
        captured codebook section, which is bounded by CB_CAP and not by this.
        The flush above emptied the buffer but did not make room for that, so
        anything oversized goes straight out.  */
    if (n > TSV_OBUF) {
      if (fwrite(s, 1, n, f) != n)
        FATAL_CODE(BLR_EXIT_IO, "write error on %s", path);
      return;
    }
    memcpy(tsv_ob + on, s, n);  on += n;
  }

  void open(const char * pth) {
    const char * m = TSV_MAGIC;
    int c;
    f = fopen(pth, "rb");
    if (!f) FATAL_CODE(BLR_EXIT_IO, "cannot open %s", pth);
    path = pth;  enc = 0;  row = 0;  ip = in = 0;  left = 0;  rtag[0] = 0;
    tee = nullptr;  terse = 0;  clng = csht = nullptr;
    while (*m) {
      FATAL_UNLESS(rd() == (u8) *m, "%s: not a balrogg TSV stream", path);
      m++;
    }
    c = rd();
    if (c == '\r') c = rd();
    FATAL_UNLESS(c == '\n', "%s: not a balrogg TSV stream", path);
  }

  /*  End the row in progress without closing, so a captured section is
      complete and comparable.  */
  void flush_row() { if (enc && wtag) { emit("\n", 1);  wtag = nullptr;  rl = 0; } }

  void close() {
    if (enc) {
      if (wtag) emit("\n", 1);
      flush();
    }
    if (f) {
      int bad = ferror(f);
      if (fclose(f)) bad = 1;
      if (bad) FATAL_CODE(BLR_EXIT_IO, "%s on %s",
                          enc ? "write error" : "read error", path);
    }
    f = nullptr;
  }

  /*  Decimal without the printf machinery: this runs once per coded symbol.
      A record joins the row above it when the tags match; the literals at the
      call sites usually make that a pointer compare.  */
  void put(const char * tag, i64 v) {
    char n[24];
    int k = (int) sizeof n;
    u64 a = v < 0 ? (u64) 0 - (u64) v : (u64) v;
    tag = shorten(tag);
    if (wtag && rl < TSV_ROWMAX && (tag == wtag || !strcmp(tag, wtag)))
      emit(" ", 1);
    else {
      sz tn = strlen(tag);
      FATAL_UNLESS(tn <= TSV_TAGMAX, "tag '%s' is too long", tag);
      if (wtag) emit("\n", 1);
      emit(tag, tn);  emit("\t", 1);
      wtag = tag;  rl = 0;
    }
    do { n[--k] = (char) ('0' + (a % 10));  a /= 10; } while (a);
    if (v < 0) n[--k] = '-';
    emit(n + k, sizeof n - (sz) k);
    rl += sizeof n - (sz) k + 1;
    row++;
  }

  i64 get(const char * tag) {
    u64 a = 0;
    int c, neg = 0, nd = 0;
    row++;
    if (!left) head(tag);
    { const char * want = shorten(tag);
      FATAL_UNLESS(!strcmp(rtag, want),
                   "%s: record %" PRIu64 " is '%s', expected '%s'", path, row,
                   terse ? lengthen(rtag) : rtag, tag); }
    c = rd();
    if (c == '-') { neg = 1;  c = rd(); }
    while (c >= '0' && c <= '9') {
      FATAL_UNLESS(nd < 19, "%s: record %" PRIu64 " has a malformed value", path, row);
      a = a * 10 + (u32) (c - '0');
      nd++;  c = rd();
    }
    FATAL_UNLESS(nd && a <= (u64) INT64_MAX,
                 "%s: record %" PRIu64 " has a malformed value", path, row);
    if (c == ' ') left = 1;
    else {
      if (c == '\r') c = rd();
      FATAL_UNLESS(c == '\n' || c < 0, "%s: record %" PRIu64 " has a malformed value",
                   path, row);
      left = 0;
    }
    {
      i64 r = neg ? -(i64) a : (i64) a;
      if (tee) tee->put(tag, r);
      return r;
    }
  }

  /*  Fetch and range-check in one step; `hi` is exclusive.  */

  /*  The tag of the next record without consuming it, or "" at end of
      stream.  Lets an optional record cost nothing when it is absent.  */
  const char * peek() {
    if (!left && !head_soft()) return "";
    return terse ? lengthen(rtag) : rtag;
  }

  u32 get_u(const char * tag, u64 hi) {
    i64 v = get(tag);
    FATAL_UNLESS(v >= 0 && (u64) v < hi,
                 "%s: record %" PRIu64 ": %s = %" PRId64 " is out of range", path, row,
                 tag, v);
    return (u32) v;
  }

private:

  void flush() {
    if (!on) return;
    if (fwrite(tsv_ob, 1, on, f) != on)
      FATAL_CODE(BLR_EXIT_IO, "write error on %s", path);
    on = 0;
  }

  /*  Next input byte, or -1 at end of file.  */
  INLINE int rd() {
    if (ip == in) {
      in = fread(tsv_ib, 1, TSV_IBUF, f);
      ip = 0;
      if (!in) {
        if (ferror(f)) FATAL_CODE(BLR_EXIT_IO, "read error on %s", path);
        return -1;
      }
    }
    return (u8) tsv_ib[ip++];
  }

  /*  Read a row's tag and leave the cursor on its first value.  Returns 0
      at end of stream so peek() can report it without failing.  */
  int head_soft() {
    int c = rd();
    sz n = 0;
    if (c < 0) return 0;
    while (c >= 0 && c != '\t' && c != '\n') {
      FATAL_UNLESS(n < TSV_TAGMAX, "%s: record %" PRIu64 " has an overlong tag", path, row);
      rtag[n++] = (char) c;
      c = rd();
    }
    FATAL_UNLESS(c == '\t', "%s: record %" PRIu64 " has no value column", path, row);
    rtag[n] = 0;
    left = 1;
    return 1;
  }

  void head(const char * tag) {
    FATAL_UNLESS(head_soft(), "%s: stream ends before record %" PRIu64 " (%s)",
                 path, row, tag);
  }

};
