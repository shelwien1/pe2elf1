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

/*  A 16-bit PCM RIFF/WAVE sink.  Sizes are only known once the stream ends,
    so the header is written twice: a placeholder first, the real one on
    close.  */

constexpr sz WAV_OBUF = 1 << 16;

static float wav_bias = 0.5f;             /*  0.5 is round to nearest  */

/*  RIFF states both sizes in 32-bit fields, so a WAVE file cannot describe
    more than 4 GiB.  Past that the header would silently wrap and every
    reader would see a truncated file, so the writer stops instead.  RF64 or
    WAVE64 would lift the limit; neither is worth the complexity until a
    stream needs it.  */
constexpr unsigned long long WAV_MAXBYTES = 0xFFFFFFFFULL - 36;

/*  Vorbis orders channels its own way; RIFF expects the Microsoft order, and
    both libvorbis's oggdec and ffmpeg map between the two on the way out.
    ord[i] is the Vorbis channel that becomes WAV channel i.  */
static const u8 WAV_ORDER[9][8] = {
  { 0 },
  { 0 },                              /*  mono  */
  { 0, 1 },                           /*  L R  */
  { 0, 2, 1 },                        /*  L C R  */
  { 0, 1, 2, 3 },                     /*  FL FR RL RR  */
  { 0, 2, 1, 3, 4 },                  /*  FL C FR RL RR  */
  { 0, 2, 1, 5, 3, 4 },               /*  ... LFE  */
  { 0, 2, 1, 6, 5, 3, 4 },            /*  ... RC LFE  */
  { 0, 2, 1, 7, 5, 6, 3, 4 }          /*  ... RL RR LFE  */
};

/*  Fill ord[] for a channel count: ord[i] is the Vorbis channel that becomes
    WAV channel i.  Both the sink and the reader need it, so it is not a
    member of either.  */
static void wav_order(u32 ch, u8 * ord) {
  u32 i;
  for (i = 0; i < ch; i++) ord[i] = (u8) (ch <= 8 ? WAV_ORDER[ch][i] : i);
}

struct wav {
  FILE * f;
  const char * path;
  u32 rate, ch;
  unsigned long long frames;
  u8 ob[WAV_OBUF];  sz on;
  u8 ord[VD_MAXCH];
  unsigned long long maxframes;

  static void w32(u8 * p, u32 v) {
    p[0] = (u8) v;  p[1] = (u8) (v >> 8);
    p[2] = (u8) (v >> 16);  p[3] = (u8) (v >> 24);
  }
  static void w16(u8 * p, u32 v) { p[0] = (u8) v;  p[1] = (u8) (v >> 8); }

  void flush() {
    if (!on) return;
    if (fwrite(ob, 1, on, f) != on)
      FATAL_CODE(BLR_EXIT_IO, "write error on %s", path);
    on = 0;
  }

  void header() {
    u8 h[44];
    u32 bytes = (u32) (frames * ch * 2), align = ch * 2;
    memcpy(h, "RIFF", 4);       w32(h + 4, 36 + bytes);
    memcpy(h + 8, "WAVEfmt ", 8);
    w32(h + 16, 16);            w16(h + 20, 1);
    w16(h + 22, ch);            w32(h + 24, rate);
    w32(h + 28, rate * align);  w16(h + 32, align);
    w16(h + 34, 16);
    memcpy(h + 36, "data", 4);  w32(h + 40, bytes);
    if (fwrite(h, 1, sizeof h, f) != sizeof h)
      FATAL_CODE(BLR_EXIT_IO, "write error on %s", path);
  }

  void create(const char * pth, u32 r, u32 c) {
    f = fopen(pth, "wb");
    if (!f) FATAL_CODE(BLR_EXIT_IO, "cannot create %s", pth);
    path = pth;  rate = r;  ch = c;  frames = 0;  on = 0;
    wav_order(ch, ord);
    maxframes = WAV_MAXBYTES / (ch * 2);
    header();
  }

  /*  One frame of `ch` samples, rounded and clipped the way libvorbis's own
      example decoder does it.  */
  void frame(const float * s) {
    u32 i;
    FATAL_UNLESS(frames < maxframes,
                 "%s: a WAVE file cannot hold more than %llu frames at %lu "
                 "channels", path, maxframes, (unsigned long) ch);
    if (on + 2 * ch > WAV_OBUF) flush();
    for (i = 0; i < ch; i++) {
      /*  Where the sample lands between two integers is the one genuinely
          lossy step in the whole chain, and the tool owns it: the WAV is its
          own output.  Nudging the split point moves which samples round up,
          which moves the recovered residue, which moves how many digits have
          to be corrected.  Mode c tries a few and keeps the cheapest; mode d
          only reads the result, so nothing has to be carried.  */
      int v = (int) floor(s[ord[i]] * 32767.0f + wav_bias);
      if (v > 32767) v = 32767;
      if (v < -32768) v = -32768;
      w16(ob + on, (u32) (v & 0xFFFF));  on += 2;
    }
    frames++;
  }

  void close() {
    flush();
    if (fseek(f, 0, SEEK_SET) == 0) header();
    if (fclose(f)) FATAL_CODE(BLR_EXIT_IO, "write error on %s", path);
    f = nullptr;
  }
};
