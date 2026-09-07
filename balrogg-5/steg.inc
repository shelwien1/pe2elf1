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

/*  The payload, and where it lives.

    tsv2wav's one lossy step is where a sample lands between two integers.
    META.md calls it the quantization gap and settles it with a rule -- floor,
    nearest, ceiling -- or with the error feedback that shapes the residue.
    Whichever rule it uses, the gap is real information the WAV has room for
    and the format does not spend: mode d reads back whatever was written, so
    nothing downstream cares which of the two integers is there.

    tsvsteg spends it.  The upper integer is chosen with probability equal to
    the fractional part, which is the same first moment round-to-nearest has,
    so the samples are still unbiased -- and the choice is drawn by *decoding*
    the payload under that model.  Because the model is the payload's own
    coding distribution, the bits come out at the gap's entropy, and because
    the coder is exact, encoding the choices back returns the payload.

    Recovery therefore needs the ideal sample x, not just the WAV.  x comes
    from the synthesis, and the synthesis comes from the record stream, which
    mode d has just rebuilt: so extraction is a second synthesis pass over the
    restored TSV, reading the WAV rather than writing it.  That is why the
    payload cannot be read out of the WAV alone -- it takes the meta too.  */

/*  The error feedback is fixed off.  x depends on it, both sides have to
    compute the same x, and the coefficient is not carried in the meta --
    tsv2wav can sweep it precisely because nothing has to agree about it
    afterwards.  Carrying it would be one record a file; until then, saying so
    here is better than a silent dependency on a compile-time constant.  */
constexpr double STEG_SHAPE = 0.0;

/*  The payload goes in behind its own length, so extraction knows where the
    real bytes stop and the coder's flush begins.  */
constexpr sz STEG_HDR = 8;

static rc_buf steg_src;                   /*  embed: [len][payload]  */
static rc_buf steg_dst;                   /*  what encoding the choices returns  */
static Rangecoder steg_dec;               /*  embed: payload -> choices  */
static Rangecoder steg_enc;               /*  choices -> payload  */

static double steg_cap;                   /*  what the gaps held, in bits  */
static unsigned long long steg_nsym;      /*  gaps that could carry a choice  */
static unsigned long long steg_nsamp;     /*  samples seen  */

/*  The WAV being read back, for extraction.  pcmwin walks the RIFF chunks and
    checks fmt against the stream, which is exactly the check wanted here, so
    the reader is borrowed rather than written again.  */
static pcmwin steg_pw;
static i16 steg_rfrm[VD_MAXCH];
static i64 steg_rat;

static void steg_ref_open(const char * path, u32 ch, u32 rate) {
  steg_pw.open(path, ch, rate);
  steg_pw.seek_to(steg_pw.data0);
  steg_rat = 0;
}

static const i16 * steg_ref_frame(u32 ch) {
  u8 t[2 * VD_MAXCH];  u32 c;
  FATAL_UNLESS(steg_rat < steg_pw.frames,
               "%s: the stream is longer than the WAV -- %lld frames",
               steg_pw.path, (long long) steg_pw.frames);
  if (fread(t, 2, ch, steg_pw.f) != ch)
    FATAL_CODE(BLR_EXIT_IO, "short read on %s", steg_pw.path);
  for (c = 0; c < ch; c++)
    steg_rfrm[c] = (i16) (u16) ((u32) t[2 * c] | ((u32) t[2 * c + 1] << 8));
  steg_rat++;
  return steg_rfrm;
}

/*  P(the upper integer) as a count out of SCALE.  */
static uint steg_freq(double f) {
  long q = (long) (f * (double) SCALE + 0.5);
  if (q < 0) q = 0;
  if (q > SCALE) q = SCALE;
  return (uint) q;
}

static double steg_H(double p) {
  if (p <= 0.0 || p >= 1.0) return 0.0;
  return -(p * log(p) + (1.0 - p) * log(1.0 - p)) / log(2.0);
}

/*  One sample.  `obs` is what the WAV has, and is read only when extracting.

    A gap carries a choice only when both integers are inside the 16-bit range
    and the fractional part is not so close to an end that the model would
    give one of them the whole interval.  Both sides decide that from x alone,
    so they agree without anything being carried.  */
static int steg_choose(double x, int obs) {
  int xi, lo, hi, v;
  uint q, freq, bit;
  steg_nsamp++;
  if (x > 32767.0) x = 32767.0;
  if (x < -32768.0) x = -32768.0;
  xi = (int) floor(x);
  lo = xi;  hi = xi + 1;
  q = (lo < -32768 || hi > 32767) ? 0 : steg_freq(x - (double) xi);
  if (q == 0 || q == (uint) SCALE) {
    /*  no room: the sample is forced, and says nothing  */
    v = (q == 0) ? lo : hi;
    if (v > 32767) v = 32767;
    if (v < -32768) v = -32768;
    if (steg_mode == STEG_EXTRACT)
      FATAL_UNLESS(obs == v, "%s: sample %llu is %d where the stream forces "
                   "%d -- the WAV does not go with this meta",
                   steg_pw.path, (unsigned long long) steg_nsamp, obs, v);
    return v;
  }
  steg_nsym++;
  steg_cap += steg_H((double) q / (double) SCALE);
  freq = (uint) SCALE - q;                /*  P(bit=0) = P(the lower one)  */
  if (steg_mode == STEG_EMBED) {
    bit = steg_dec.rc_BProcess(freq, 0);
    steg_enc.rc_BProcess(freq, bit);      /*  and straight back, to check  */
  } else {
    FATAL_UNLESS(obs == lo || obs == hi,
                 "%s: sample %llu is %d, not %d or %d -- the WAV does not go "
                 "with this meta", steg_pw.path,
                 (unsigned long long) steg_nsamp, obs, lo, hi);
    bit = (obs == hi);
    steg_enc.rc_BProcess(freq, bit);
  }
  return bit ? hi : lo;
}

/*  A synthesis pass may be run more than once; the one that counts is the
    last, so both coders start again with it.  */
static void steg_pass_reset(void) {
  if (!steg_mode) return;
  steg_dst.free_();
  steg_cap = 0.0;  steg_nsym = steg_nsamp = 0;
  if (steg_mode == STEG_EMBED) { steg_src.at = 0;  steg_dec.StartDecode(&steg_src); }
  steg_enc.StartEncode(&steg_dst);
}

static void steg_read_file(const char * path, rc_buf * b) {
  FILE * f = fopen(path, "rb");
  sz got;
  if (!f) FATAL_CODE(BLR_EXIT_IO, "cannot open %s", path);
  for (;;) {
    u8 t[1 << 16];
    got = fread(t, 1, sizeof t, f);
    { sz i;  for (i = 0; i < got; i++) b->put(t[i]); }
    if (got != sizeof t) break;
  }
  if (ferror(f)) { fclose(f);  FATAL_CODE(BLR_EXIT_IO, "read error on %s", path); }
  fclose(f);
}

/*  [length][payload], ready to be decoded from.  */
static void steg_embed_begin(const char * path) {
  unsigned long long len;
  int i;
  steg_src.init();  steg_dst.init();
  for (i = 0; i < (int) STEG_HDR; i++) steg_src.put(0);
  steg_read_file(path, &steg_src);
  len = (unsigned long long) (steg_src.n - STEG_HDR);
  for (i = 0; i < (int) STEG_HDR; i++) steg_src.p[i] = (u8) (len >> (8 * i));
  steg_mode = STEG_EMBED;
}

/*  The entropy of the gaps is a ceiling, not a promise: a range coder spends
    a little over it, the flush costs a few bytes, and a random payload's own
    code length wanders around its mean by O(sqrt n).  Measured on 00.ogg the
    shortfall was 21 bytes in 21,565, so the figure quoted to the caller backs
    off by a fixed 32 bytes and a further tenth of a percent, and is then a
    length that will actually go in rather than one that nearly will.  */
static double steg_bytes_held(void) {
  return steg_cap / 8.0 - (double) STEG_HDR - 32.0 - steg_cap / 8192.0;
}

/*  The verifying encoder has been running alongside; if what it gives back is
    the buffer that went in, the payload is recoverable.  */
static void steg_embed_end(const char * path) {
  sz want = steg_src.n;
  steg_enc.FinishEncode();
  if (steg_dst.n < want || memcmp(steg_dst.p, steg_src.p, want) != 0) {
    double cap = steg_bytes_held();
    FATAL_CODE(BLR_EXIT_REFUSED,
               "%s: this stream holds about %.0f payload bytes, and the "
               "payload is %llu", path, cap < 0.0 ? 0.0 : cap,
               (unsigned long long) (steg_src.n - STEG_HDR));
  }
}

static void steg_extract_begin(void) {
  steg_src.init();  steg_dst.init();
  steg_mode = STEG_EXTRACT;
}

static void steg_extract_end(const char * path) {
  unsigned long long len = 0;
  int i;
  FILE * f;
  steg_enc.FinishEncode();
  FATAL_UNLESS(steg_dst.n >= STEG_HDR,
               "%s: nothing came back out of the WAV", steg_pw.path);
  for (i = (int) STEG_HDR - 1; i >= 0; i--) len = (len << 8) | steg_dst.p[i];
  FATAL_UNLESS(len <= (unsigned long long) (steg_dst.n - STEG_HDR),
               "%s: the recovered length is %llu bytes but only %llu came "
               "back -- the WAV, the meta or the payload has been altered",
               steg_pw.path, len,
               (unsigned long long) (steg_dst.n - STEG_HDR));
  f = fopen(path, "wb");
  if (!f) FATAL_CODE(BLR_EXIT_IO, "cannot create %s", path);
  if (len && fwrite(steg_dst.p + STEG_HDR, 1, (sz) len, f) != (sz) len) {
    fclose(f);  FATAL_CODE(BLR_EXIT_IO, "write error on %s", path);
  }
  if (fclose(f)) FATAL_CODE(BLR_EXIT_IO, "write error on %s", path);
}

/*  Silent on success like the rest of the tree, since a payload that did not
    fit is an error rather than a smaller result.  The numbers are worth
    having when sizing one, so an environment variable asks for them.  */
static void steg_report(const char * what) {
  if (!getenv("TSVSTEG_VERBOSE")) return;
  fprintf(stderr, "%s: %llu of %llu samples had a gap to fill, holding "
          "%.0f bits = %.0f bytes; %.2f bits a sample\n", what,
          steg_nsym, steg_nsamp, steg_cap, steg_cap / 8.0,
          steg_nsamp ? steg_cap / (double) steg_nsamp : 0.0);
}
