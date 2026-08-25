////////////////////////////////////////////////////////////////////////////
//                            **** MTF2IMA ****                           //
//    Lossless structural repack between MT Framework ADPCM and IMA WAV   //
//                    (companion tool for ADPCM-XQ)                       //
////////////////////////////////////////////////////////////////////////////

/* Capcom's MT Framework ADPCM (ffmpeg's adpcm_ima_mtf, carried in .fwse
 * files) and IMA-ADPCM spend one 4-bit code per sample and share the same
 * 89-entry step table, so one can be repacked into the other with no loss
 * and almost no side data.  What travels is the code stream, not the audio:
 * code i of channel k in the source becomes code i of channel k in the
 * target, through the bijection that preserves the order of the
 * reconstruction levels.  MTF codes are already in level order -- its delta
 * is step*(2n-15), rising monotonically from n = 0 to n = 15 -- while IMA
 * codes are sign-magnitude, so the map is
 *
 *     MTF n  ->  IMA c = (n >= 8) ? n - 8 : (8 | (7 - n))
 *
 * (the same rank ordering ima2ms uses, with rank = n).  This pair of formats
 * lines up far better than IMA and MS do.  Write the MTF delta for n >= 8 as
 * step*(2m+1) with m = n-8, and the IMA delta for magnitude m is
 * step*(2m+1)/8 exactly (that is what the shift-and-add ladder computes, up
 * to its truncation): the two ladders are the same rungs a factor of 8
 * apart.  Better still, the step-index tables agree rung for rung under the
 * map above -- MTF's {8,6,4,2,-1..-1,2,4,6,8} indexed by n is IMA's
 * {-1,-1,-1,-1,2,4,6,8} indexed by magnitude -- and both clamp to 0..88.  So
 * the repacked stream does not merely carry the codes, it reproduces the
 * source's step-index trajectory sample for sample.
 *
 * What does not survive is the recursion.  IMA integrates, MTF leaks:
 *
 *     IMA:  p  =  clip16 (p + delta)
 *     MTF:  p  =  clip16 ((p + 8*delta) >> 4)
 *
 * MTF is very nearly a memoryless adaptive quantiser -- the old predictor
 * comes back attenuated by 16 -- so the two reconstructions diverge from the
 * first sample no matter how the scales are matched, and no fixed code map
 * can prevent that.  The output is a conformant IMA-ADPCM WAV -- correct fmt
 * chunk, valid block geometry, accepted by ordinary decoders -- but it does
 * not carry the source audio.  This is the fake-PCM trick one level up: the
 * file is a container for the code stream, and restoring the audio means
 * restoring the source file, which is what "d" does.
 *
 * Block geometry lines up byte for byte because both formats pack two codes
 * of one channel per byte: the payload of an IMA block holds exactly as many
 * MTF bytes as it has payload bytes, and each block costs 4 bytes per
 * channel of header on top.  Only the packing order differs -- MTF puts the
 * earlier code in the high nibble and interleaves channels every byte, IMA
 * puts it in the low nibble and interleaves every 4 bytes.
 *
 *   MTF -> IMA   the block header has no counterpart in the source, so it is
 *                synthesised from the MTF decoder state at the block
 *                boundary: the step index is exactly right (the trajectories
 *                agree), and the predictor is the MTF one.  Being a function
 *                of the codes it costs nothing in the metainfo -- the
 *                restore ignores block headers entirely.
 *
 *   IMA -> MTF   the block header has nowhere to go, so it goes to the
 *                metainfo: the predictor as a residual against the previous
 *                block's, the step index likewise, and the "reserved" byte
 *                as an exception list.
 *
 * The source container header (the 1 KiB FWSE header, or the RIFF header) is
 * stored run-length coded, with the fields that are functions of the payload
 * -- FWSE file size and duration -- blanked out and rebuilt.  The target's
 * header is generated.  A CRC-32 of the source is verified after restoring;
 * a CRC-32 of the target sample stream detects a mismatched wav/metainfo
 * pair.  The forward direction runs its own inverse in memory before writing
 * anything.
 *
 * The metainfo does not have to be a separate file.  Both containers have room
 * for it: RIFF is a chunk list, so it goes in an "M2I1" chunk after the data
 * chunk, where the payload keeps its canonical offset and every decoder skips
 * what it does not know; FWSE has a 1 KiB header of which only the first 0x1c
 * bytes are read and the rest is 0xCC filler, so it goes in there at 0x2c, for
 * free until it outgrows the filler -- past that the header grows and
 * start_offset, which the demuxer honours, grows with it.  Naming the metainfo
 * file is what asks for it separately.
 *
 * Usage:  mtf2ima c|d [-q] [-bN] input output [metainfo.bin]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MI_MAGIC        "M2I1"
#define MI_VERSION      1
#define MI_FLAG_RLE     0x1     // the stored container header is run-length coded

#define SRC_MTF         1       // FWSE / adpcm_ima_mtf
#define SRC_IMA         2       // RIFF WAVE / adpcm_ima_wav

#define FMT_IMA         0x11    // WAVE_FORMAT_IMA_ADPCM

#define MAX_CHANS       2       // the fwse demuxer accepts 1 or 2
#define CANON_IMA       60      // riff 12 + fmt 8+20 + fact 8+4 + data 8
#define FWSE_HEAD       0x400   // start_offset of every known .fwse
#define FWSE_META_OFF   0x2c    // first byte of the header no known .fwse uses
#define DEF_ALIGN       2048    // default IMA block size, exact for 1 and 2 ch

static const char *sign_on =
    "\n MTF2IMA  lossless MT Framework ADPCM <-> IMA-ADPCM structural repack  version 1.0\n\n";

static const char *usage =
    " Usage:    mtf2ima c|d [-q] [-bN] input output [metainfo.bin]\n\n"
    "           c = repack into the other format (.fwse <-> IMA wav, detected\n"
    "               from the input's magic)\n"
    "           d = rebuild the original file from output + metainfo.bin\n\n"
    "           without metainfo.bin the metainfo travels inside the output\n"
    "           itself, in an M2I1 chunk of the wav or in the FWSE header\n\n"
    "           the code stream is carried across unchanged, so the output is a\n"
    "           conformant file of the other format but not the same audio\n\n"
    " Options:  -q  = quiet\n"
    "           -bN = IMA block size in bytes for c (default 2048), rounded down\n"
    "                 to 4*channels*(groups+1)\n\n";

static int quiet = 0;
static int opt_align = DEF_ALIGN;

/*------------------------------ little endian ------------------------------*/

static inline uint32_t rd16 (const uint8_t *p) { return (uint32_t) p [0] | ((uint32_t) p [1] << 8); }
static inline uint32_t rd32 (const uint8_t *p) { return rd16 (p) | (rd16 (p + 2) << 16); }
static inline void wr16 (uint8_t *p, uint32_t v) { p [0] = (uint8_t) v; p [1] = (uint8_t)(v >> 8); }
static inline void wr32 (uint8_t *p, uint32_t v) { wr16 (p, v & 0xffff); wr16 (p + 2, v >> 16); }

/*--------------------------------- helpers ---------------------------------*/

static void die (const char *msg, const char *arg = NULL)
{
    if (arg)
        fprintf (stderr, "mtf2ima: %s \"%s\"\n", msg, arg);
    else
        fprintf (stderr, "mtf2ima: %s\n", msg);

    exit (1);
}

static inline int32_t clamp16 (int32_t v) { return v < -32768 ? -32768 : (v > 32767 ? 32767 : v); }
static inline int clampidx (int v) { return v < 0 ? 0 : (v > 88 ? 88 : v); }

struct Buf {
    uint8_t *d;
    size_t n, cap;

    Buf () : d (NULL), n (0), cap (0) { }
    ~Buf () { free (d); }

    void reserve (size_t k)
    {
        if (n + k > cap) {
            cap = (n + k) * 2 + 65536;
            if (!(d = (uint8_t *) realloc (d, cap)))
                die ("out of memory");
        }
    }

    void put (int v) { reserve (1); d [n++] = (uint8_t) v; }
    void put (const void *s, size_t k) { if (k) { reserve (k); memcpy (d + n, s, k); n += k; } }
};

// sequential reader with bounds checking, used for every metainfo sub-stream
struct Cursor {
    const uint8_t *d;
    size_t n, p;
    const char *name;

    Cursor () : d (NULL), n (0), p (0), name ("metainfo") { }
    void set (const uint8_t *data, size_t size, const char *who) { d = data; n = size; p = 0; name = who; }
    int left () const { return p < n; }

    int byte ()
    {
        if (p >= n) die ("corrupt or truncated metainfo from", name);
        return d [p++];
    }
};

static void put_varint (Buf &b, uint64_t v)
{
    while (v >= 0x80) { b.put ((int) (v & 0x7f) | 0x80); v >>= 7; }
    b.put ((int) v);
}

static uint64_t get_varint (Cursor &c)
{
    uint64_t v = 0;
    int shift = 0;

    for (;;) {
        int byte = c.byte ();
        v |= (uint64_t) (byte & 0x7f) << shift;
        if (!(byte & 0x80)) return v;
        if ((shift += 7) > 63) die ("corrupt metainfo from", c.name);
    }
}

static inline uint64_t zig (int64_t v) { return ((uint64_t) v << 1) ^ (uint64_t) (v >> 63); }
static inline int64_t unzig (uint64_t v) { return (int64_t) (v >> 1) ^ -(int64_t) (v & 1); }

static uint32_t crc32_buf (const uint8_t *p, size_t n)
{
    static uint32_t table [256];
    static int inited;

    if (!inited) {
        for (int i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xEDB88320u & (uint32_t) -(int32_t) (c & 1));
            table [i] = c;
        }
        inited = 1;
    }

    uint32_t c = 0xFFFFFFFFu;
    while (n--) c = table [(c ^ *p++) & 0xff] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

static uint8_t *load_file (const char *name, size_t *size)
{
    FILE *f = fopen (name, "rb");
    if (!f) die ("can't open", name);

    fseek (f, 0, SEEK_END);
    long len = ftell (f);
    fseek (f, 0, SEEK_SET);
    if (len < 0) die ("can't size", name);

    uint8_t *d = (uint8_t *) malloc ((size_t) len + 1);
    if (!d) die ("out of memory");

    if (fread (d, 1, (size_t) len, f) != (size_t) len) die ("can't read", name);
    fclose (f);
    *size = (size_t) len;
    return d;
}

static void save_file (const char *name, const void *d, size_t n)
{
    FILE *f = fopen (name, "wb");
    if (!f) die ("can't create", name);
    if (fwrite (d, 1, n, f) != n) die ("can't write", name);
    fclose (f);
}

/*----------------------------------- RLE ------------------------------------*/

/* The FWSE header is 1 KiB of which all but ~170 bytes is 0xCC filler, so it
 * is worth collapsing before it goes into the metainfo.  Control byte: 0..127
 * = that many plus one literal bytes follow, 128..255 = the next byte repeats
 * (c - 128) + 3 times. */

static void rle_pack (Buf &out, const uint8_t *p, size_t n)
{
    size_t i = 0;

    while (i < n) {
        size_t run = 1;

        while (i + run < n && p [i + run] == p [i] && run < 130) ++run;

        if (run >= 3) {
            out.put ((int) (128 + (run - 3)));
            out.put (p [i]);
            i += run;
            continue;
        }

        size_t lit = 0;                                 // gather until a run of 3 starts

        while (i + lit < n && lit < 128) {
            if (i + lit + 2 < n && p [i + lit] == p [i + lit + 1] && p [i + lit] == p [i + lit + 2])
                break;
            ++lit;
        }

        if (!lit) lit = 1;
        out.put ((int) (lit - 1));
        out.put (p + i, lit);
        i += lit;
    }
}

static void rle_unpack (Buf &out, Cursor &c, size_t want)
{
    size_t done = 0;

    while (done < want) {
        int ctl = c.byte ();

        if (ctl < 128) {
            size_t k = (size_t) ctl + 1;

            if (done + k > want) die ("corrupt metainfo from", c.name);
            for (size_t i = 0; i < k; ++i) out.put (c.byte ());
            done += k;
        }
        else {
            size_t k = (size_t) (ctl - 128) + 3;
            int v = c.byte ();

            if (done + k > want) die ("corrupt metainfo from", c.name);
            for (size_t i = 0; i < k; ++i) out.put (v);
            done += k;
        }
    }
}

/*------------------------------- MTF decoder --------------------------------*/

static const int16_t step_table [89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
    253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
    1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
    3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

// ffmpeg's mtf_index_table, indexed by the code itself rather than by magnitude
static const int8_t mtf_index_table [16] = {
     8,  6,  4,  2, -1, -1, -1, -1,
    -1, -1, -1, -1,  2,  4,  6,  8
};

struct MtfState {
    int32_t predictor;
    int step_index;
};

// exactly adpcm_ima_mtf_expand_nibble(), so the synthesised IMA headers agree
// with what an MT Framework decoder holds at that point in the stream
static inline void mtf_advance (MtfState &s, int nibble)
{
    int32_t p = s.predictor + (int32_t) step_table [s.step_index] * (2 * nibble - 15);

    s.step_index = clampidx (s.step_index + mtf_index_table [nibble]);
    s.predictor = clamp16 (p >> 4);
}

/*------------------------------ the code bijection --------------------------*/

// rank 0 is the most negative reconstruction level, rank 15 the most positive;
// an MTF code is its own rank, an IMA code is sign-magnitude
static inline int ima_to_rank (int c) { return (c & 8) ? 7 - (c & 7) : 8 + (c & 7); }
static inline int rank_to_ima (int r) { return (r >= 8) ? r - 8 : (8 | (7 - r)); }

static inline int mtf_to_ima (int n) { return rank_to_ima (n); }
static inline int ima_to_mtf (int c) { return ima_to_rank (c); }

/*------------------------------ code addressing -----------------------------*/

// MTF interleaves one byte per channel; the earlier code is the high nibble
static inline int mtf_get (const uint8_t *body, int nch, int k, size_t i)
{
    return (body [(i >> 1) * (size_t) nch + (size_t) k] >> ((i & 1) ? 0 : 4)) & 15;
}

static inline void mtf_put (uint8_t *body, int nch, int k, size_t i, int v)
{
    body [(i >> 1) * (size_t) nch + (size_t) k] |= (uint8_t) ((v & 15) << ((i & 1) ? 0 : 4));
}

// IMA packs a channel's codes in 4-byte groups, interleaved by channel, and
// the earlier code is the low nibble
static inline size_t ima_byte (int nch, int k, size_t jj)
{
    return (jj & ~(size_t) 3) * (size_t) nch + (size_t) k * 4 + (jj & 3);
}

static inline int ima_get (const uint8_t *body, int nch, int k, size_t i)
{
    return (body [ima_byte (nch, k, i >> 1)] >> ((i & 1) * 4)) & 15;
}

static inline void ima_put (uint8_t *body, int nch, int k, size_t i, int v)
{
    body [ima_byte (nch, k, i >> 1)] |= (uint8_t) ((v & 15) << ((i & 1) * 4));
}

/*------------------------------ block geometry ------------------------------*/

struct Ctx {
    int src_format, chans;
    int align;              // the IMA block size, in whichever direction
};

/* One step of the walk, derived identically in both directions from the
 * length of the source chunk alone.  In both directions a chunk of the MTF
 * stream and the payload of one IMA block hold the same codes in the same
 * number of bytes; only the IMA header and the odd bytes at the very end of
 * the stream have to be accounted for. */

struct Geom {
    size_t src_len;     // bytes taken from the source
    size_t dst_len;     // bytes the matching target chunk occupies
    size_t codes;       // codes per channel carried across
    size_t rest;        // source bytes that the target's grouping cannot hold
};

// MTF -> IMA: src_len is the MTF chunk, at most one IMA block's payload
static Geom geom_mtf (const Ctx &c, size_t left)
{
    const size_t nch = (size_t) c.chans, hdr = 4 * nch;
    size_t body = (size_t) c.align - hdr;
    Geom g;

    memset (&g, 0, sizeof (g));
    g.src_len = left < body ? left : body;

    size_t frames = g.src_len / nch;                    // whole bytes per channel
    size_t groups = (frames + 3) / 4;                   // IMA rounds up to 4-byte groups

    g.codes = frames * 2;
    g.rest = g.src_len - frames * nch;                  // partial trailing frame
    g.dst_len = hdr + groups * 4 * nch;

    return g;
}

// IMA -> MTF: src_len is one IMA block
static Geom geom_ima (const Ctx &c, size_t left)
{
    const size_t nch = (size_t) c.chans, hdr = 4 * nch;
    Geom g;

    memset (&g, 0, sizeof (g));
    g.src_len = left < (size_t) c.align ? left : (size_t) c.align;

    size_t body = g.src_len - hdr;
    size_t groups = body / (4 * nch);

    g.codes = groups * 8;
    g.rest = body - groups * 4 * nch;                   // no whole group left
    g.dst_len = groups * 4 * nch;

    return g;
}

/*--------------------------------- streams ----------------------------------*/

struct Streams {
    Buf head;       // source container header, run-length coded
    Buf side;       // odd bytes at the end of a chunk, runt tails
    Buf pred;       // IMA -> MTF: block predictor residual against the previous block
    Buf sidx;       // IMA -> MTF: block step index residual against the previous block
    Buf rsvx;       // IMA -> MTF: exceptions, nonzero IMA "reserved" bytes
};

struct Joined {
    Cursor head, side, pred, sidx, rsvx;
};

/*------------------------------- MTF -> IMA ---------------------------------*/

static void mtf_to_ima_fwd (const Ctx &c, const uint8_t *src, size_t n, Buf &dst,
    Streams &s, uint64_t *samples)
{
    const int nch = c.chans, hdr = 4 * nch;
    MtfState st [MAX_CHANS];
    uint8_t *blk = (uint8_t *) malloc ((size_t) c.align + 16);

    if (!blk) die ("out of memory");
    memset (st, 0, sizeof (st));

    for (size_t pos = 0; pos < n;) {
        Geom g = geom_mtf (c, n - pos);
        const uint8_t *in = src + pos;

        memset (blk, 0, g.dst_len);

        for (int k = 0; k < nch; ++k) {                             // the state is the header
            wr16 (blk + k * 4, (uint32_t) st [k].predictor);
            blk [k * 4 + 2] = (uint8_t) st [k].step_index;
            blk [k * 4 + 3] = 0;
        }

        for (size_t i = 0; i < g.codes; ++i)
            for (int k = 0; k < nch; ++k) {
                int code = mtf_get (in, nch, k, i);

                ima_put (blk + hdr, nch, k, i, mtf_to_ima (code));
                mtf_advance (st [k], code);
            }

        if (g.rest)
            s.side.put (in + g.src_len - g.rest, g.rest);

        dst.put (blk, g.dst_len);
        *samples += g.codes + 1;                                    // plus the header sample
        pos += g.src_len;
    }

    free (blk);
}

static void mtf_to_ima_inv (const Ctx &c, const uint8_t *dsrc, size_t dn, Buf &out,
    size_t n, Joined &in)
{
    const int nch = c.chans, hdr = 4 * nch;
    size_t dpos = 0;
    uint8_t *blk = (uint8_t *) malloc ((size_t) c.align + 16);

    if (!blk) die ("out of memory");

    for (size_t pos = 0; pos < n;) {
        Geom g = geom_mtf (c, n - pos);

        if (dpos + g.dst_len > dn) die ("converted file is shorter than the metainfo expects");

        const uint8_t *d = dsrc + dpos;

        memset (blk, 0, g.src_len);

        for (size_t i = 0; i < g.codes; ++i)
            for (int k = 0; k < nch; ++k)
                mtf_put (blk, nch, k, i, ima_to_mtf (ima_get (d + hdr, nch, k, i)));

        for (size_t i = 0; i < g.rest; ++i)
            blk [g.src_len - g.rest + i] = (uint8_t) in.side.byte ();

        out.put (blk, g.src_len);
        dpos += g.dst_len;
        pos += g.src_len;
    }

    free (blk);
}

/*------------------------------- IMA -> MTF ---------------------------------*/

static void ima_to_mtf_fwd (const Ctx &c, const uint8_t *src, size_t n, Buf &dst,
    Streams &s, uint64_t *samples)
{
    const int nch = c.chans, hdr = 4 * nch;
    int32_t pprev [MAX_CHANS], iprev [MAX_CHANS];
    uint64_t ordinal = 0;
    uint8_t *blk = (uint8_t *) malloc ((size_t) c.align + 16);

    if (!blk) die ("out of memory");
    memset (pprev, 0, sizeof (pprev));
    memset (iprev, 0, sizeof (iprev));

    for (size_t pos = 0; pos < n;) {
        size_t left = n - pos;
        const uint8_t *in = src + pos;

        if (left < (size_t) hdr) { s.side.put (in, left); break; }      // runt tail

        Geom g = geom_ima (c, left);

        memset (blk, 0, g.dst_len);

        for (int k = 0; k < nch; ++k, ++ordinal) {
            int32_t pred = (int16_t) rd16 (in + k * 4);
            int index = in [k * 4 + 2];

            put_varint (s.pred, zig (pred - pprev [k]));
            put_varint (s.sidx, zig (index - iprev [k]));
            pprev [k] = pred;
            iprev [k] = index;

            if (in [k * 4 + 3]) {                                       // expected zero
                put_varint (s.rsvx, ordinal);
                s.rsvx.put (in [k * 4 + 3]);
            }
        }

        for (size_t i = 0; i < g.codes; ++i)
            for (int k = 0; k < nch; ++k)
                mtf_put (blk, nch, k, i, ima_to_mtf (ima_get (in + hdr, nch, k, i)));

        if (g.rest)
            s.side.put (in + g.src_len - g.rest, g.rest);

        dst.put (blk, g.dst_len);
        *samples += g.codes;
        pos += g.src_len;
    }

    free (blk);
}

static void ima_to_mtf_inv (const Ctx &c, const uint8_t *dsrc, size_t dn, Buf &out,
    size_t n, Joined &in)
{
    const int nch = c.chans, hdr = 4 * nch;
    int32_t pprev [MAX_CHANS], iprev [MAX_CHANS];
    uint64_t ordinal = 0, exc_at = 0;
    int exc_valid = 0;
    size_t dpos = 0;
    uint8_t *blk = (uint8_t *) malloc ((size_t) c.align + 16);

    if (!blk) die ("out of memory");
    memset (pprev, 0, sizeof (pprev));
    memset (iprev, 0, sizeof (iprev));

    if (in.rsvx.left ()) { exc_at = get_varint (in.rsvx); exc_valid = 1; }

    for (size_t pos = 0; pos < n;) {
        size_t left = n - pos;

        if (left < (size_t) hdr) {
            for (size_t i = 0; i < left; ++i) out.put (in.side.byte ());
            break;
        }

        Geom g = geom_ima (c, left);

        if (dpos + g.dst_len > dn) die ("converted file is shorter than the metainfo expects");

        const uint8_t *d = dsrc + dpos;

        memset (blk, 0, g.src_len);

        for (int k = 0; k < nch; ++k, ++ordinal) {
            int32_t pred = (int32_t) (pprev [k] + unzig (get_varint (in.pred)));
            int index = (int) (iprev [k] + unzig (get_varint (in.sidx)));

            wr16 (blk + k * 4, (uint32_t) pred);
            blk [k * 4 + 2] = (uint8_t) index;
            blk [k * 4 + 3] = 0;
            pprev [k] = pred;
            iprev [k] = index;

            if (exc_valid && exc_at == ordinal) {
                blk [k * 4 + 3] = (uint8_t) in.rsvx.byte ();
                exc_valid = in.rsvx.left ();
                if (exc_valid) exc_at = get_varint (in.rsvx);
            }
        }

        for (size_t i = 0; i < g.codes; ++i)
            for (int k = 0; k < nch; ++k)
                ima_put (blk + hdr, nch, k, i, mtf_to_ima (mtf_get (d, nch, k, i)));

        for (size_t i = 0; i < g.rest; ++i)
            blk [g.src_len - g.rest + i] = (uint8_t) in.side.byte ();

        out.put (blk, g.src_len);
        dpos += g.dst_len;
        pos += g.src_len;
    }

    free (blk);
}

static void repack_fwd (const Ctx &c, const uint8_t *src, size_t n, Buf &dst,
    Streams &s, uint64_t *samples)
{
    if (c.src_format == SRC_MTF) mtf_to_ima_fwd (c, src, n, dst, s, samples);
    else ima_to_mtf_fwd (c, src, n, dst, s, samples);
}

static void repack_inv (const Ctx &c, const uint8_t *dsrc, size_t dn, Buf &out,
    size_t n, Joined &in)
{
    if (c.src_format == SRC_MTF) mtf_to_ima_inv (c, dsrc, dn, out, n, in);
    else ima_to_mtf_inv (c, dsrc, dn, out, n, in);
}

/*------------------------------ container headers ---------------------------*/

struct Src {
    size_t head_size, data_off, data_size, tail_size;
    int format, chans;
    uint32_t rate, duration;
};

static void parse_fwse (const uint8_t *f, size_t fsize, Src *w, const char *name)
{
    memset (w, 0, sizeof (*w));

    if (fsize < 0x1c || memcmp (f, "FWSE", 4)) die ("not an FWSE file:", name);

    uint32_t version = rd32 (f + 4), start = rd32 (f + 12);

    if (version != 2 && version != 3) die ("unsupported FWSE version in", name);
    if (start < 0x1c || start > fsize) die ("bad FWSE start offset in", name);

    w->format = SRC_MTF;
    w->chans = (int) rd32 (f + 16);
    w->duration = rd32 (f + 20);
    w->rate = rd32 (f + 24);
    w->head_size = start;
    w->data_off = start;
    w->data_size = fsize - start;
    w->tail_size = 0;
}

static void parse_wav (const uint8_t *f, size_t fsize, Src *w, const char *name,
    int *align, int *spb)
{
    const uint8_t *fmt = NULL;
    uint32_t fmt_size = 0;

    memset (w, 0, sizeof (*w));

    if (fsize < 12 || memcmp (f, "RIFF", 4) || memcmp (f + 8, "WAVE", 4))
        die ("not a RIFF/WAVE file:", name);

    for (size_t pos = 12; pos + 8 <= fsize;) {
        const uint8_t *id = f + pos;
        uint32_t size = rd32 (f + pos + 4);
        size_t body = pos + 8;
        uint32_t avail = (uint32_t) (body + size <= fsize ? size : fsize - body);

        if (!memcmp (id, "fmt ", 4)) { fmt = f + body; fmt_size = avail; }
        else if (!memcmp (id, "data", 4)) {
            if (!fmt || fmt_size < 16) die ("missing or short fmt chunk in", name);

            w->format = SRC_IMA;
            w->chans = (int) rd16 (fmt + 2);
            w->rate = rd32 (fmt + 4);
            *align = (int) rd16 (fmt + 12);
            *spb = (fmt_size >= 20) ? (int) rd16 (fmt + 18) : 0;

            if ((int) rd16 (fmt) != FMT_IMA) die ("not an IMA-ADPCM (0x11) wav file:", name);
            if ((int) rd16 (fmt + 14) != 4) die ("only 4-bit ADPCM can be repacked");

            w->head_size = body;                        // sizes in the wild lie; clamp
            w->data_off = body;
            w->data_size = avail;
            w->tail_size = fsize - (body + avail);
            w->duration = 0;
            return;
        }

        pos = body + size + (size & 1);
    }

    die ("no data chunk in", name);
}

// samples a full IMA block of the given geometry decodes to
static int ima_block_samples (int chans, int align) { return (align - 4 * chans) * 2 / chans + 1; }

static void build_ima_head (Buf &out, int chans, uint32_t rate, int align,
    uint32_t num_samples, uint32_t data_size, uint32_t extra)
{
    uint8_t h [CANON_IMA];
    int spb = ima_block_samples (chans, align);

    memcpy (h, "RIFF", 4);
    wr32 (h + 4, (uint32_t) (CANON_IMA - 8) + data_size + (data_size & 1) + extra);
    memcpy (h + 8, "WAVE", 4);

    memcpy (h + 12, "fmt ", 4);
    wr32 (h + 16, 20);
    wr16 (h + 20, FMT_IMA);
    wr16 (h + 22, (uint32_t) chans);
    wr32 (h + 24, rate);
    wr32 (h + 28, spb > 0 ? rate * (uint32_t) align / (uint32_t) spb : 0);
    wr16 (h + 32, (uint32_t) align);
    wr16 (h + 34, 4);
    wr16 (h + 36, 2);                                           // cbSize
    wr16 (h + 38, (uint32_t) spb);

    memcpy (h + 40, "fact", 4);
    wr32 (h + 44, 4);
    wr32 (h + 48, num_samples);

    memcpy (h + 52, "data", 4);
    wr32 (h + 56, data_size);

    out.put (h, CANON_IMA);
}

/* A generated FWSE header.  Only the fields the demuxer reads are meaningful;
 * the rest follows what the samples look like, 0xCC filler over an opaque
 * middle that no encoder outside MT Framework can fill in. */

static size_t fwse_head_size (size_t msize)
{
    size_t need = msize ? FWSE_META_OFF + 8 + msize : 0;

    return need <= FWSE_HEAD ? FWSE_HEAD : ((need + 15) & ~(size_t) 15);
}

static void build_fwse_head (Buf &out, int chans, uint32_t rate, uint32_t data_size,
    const uint8_t *meta, size_t msize)
{
    size_t hs = fwse_head_size (msize);
    uint8_t *h = (uint8_t *) malloc (hs);

    if (!h) die ("out of memory");
    memset (h, 0xcc, hs);
    memcpy (h, "FWSE", 4);
    wr32 (h + 4, 2);                                            // version
    wr32 (h + 8, (uint32_t) hs + data_size);                    // file size
    wr32 (h + 12, (uint32_t) hs);                               // start offset
    wr32 (h + 16, (uint32_t) chans);
    wr32 (h + 20, chans ? data_size * 2 / (uint32_t) chans : 0);
    wr32 (h + 24, rate);
    wr32 (h + 28, 16);
    wr32 (h + 32, 0xffffffffu);
    wr32 (h + 36, 0xffffffffu);
    wr32 (h + 40, 0);

    if (msize) {                                                // into the filler
        memcpy (h + FWSE_META_OFF, MI_MAGIC, 4);
        wr32 (h + FWSE_META_OFF + 4, (uint32_t) msize);
        memcpy (h + FWSE_META_OFF + 8, meta, msize);
    }

    out.put (h, hs);
    free (h);
}

/*--------------------------- the embedded metainfo ---------------------------*/

// an M2I1 chunk anywhere in the RIFF chunk list
static const uint8_t *find_wav_meta (const uint8_t *f, size_t fsize, size_t *msize)
{
    for (size_t pos = 12; pos + 8 <= fsize;) {
        uint32_t size = rd32 (f + pos + 4);
        size_t body = pos + 8;
        uint32_t avail = (uint32_t) (body + size <= fsize ? size : fsize - body);

        if (!memcmp (f + pos, MI_MAGIC, 4)) { *msize = avail; return f + body; }

        pos = body + size + (size & 1);
    }

    return NULL;
}

// the fixed spot in the FWSE header filler, inside whatever start_offset says
static const uint8_t *find_fwse_meta (const uint8_t *f, size_t start, size_t *msize)
{
    if (start < FWSE_META_OFF + 8 || memcmp (f + FWSE_META_OFF, MI_MAGIC, 4)) return NULL;

    uint32_t size = rd32 (f + FWSE_META_OFF + 4);

    if (size > start - (FWSE_META_OFF + 8)) return NULL;

    *msize = size;
    return f + FWSE_META_OFF + 8;
}

/* Fields of the stored source header that are functions of what is stored
 * elsewhere get blanked before the RLE and rebuilt after it, so that a
 * varying file size and duration do not each cost their own literal run. */

static void head_blank (uint8_t *h, size_t n, int format)
{
    if (format == SRC_MTF && n >= 24) { wr32 (h + 8, 0); wr32 (h + 20, 0); }
}

static void head_rebuild (uint8_t *h, size_t n, int format, size_t data_size, uint32_t duration)
{
    if (format == SRC_MTF && n >= 24) {
        wr32 (h + 8, (uint32_t) (n + data_size));
        wr32 (h + 20, duration);
    }
}

/*--------------------------------- convert ----------------------------------*/

static void convert (const char *infile, const char *outfile, const char *metafile)
{
    size_t fsize;
    uint8_t *file = load_file (infile, &fsize);
    Src w;
    Ctx c;
    int src_align = 0, src_spb = 0;

    if (fsize >= 4 && !memcmp (file, "FWSE", 4)) parse_fwse (file, fsize, &w, infile);
    else parse_wav (file, fsize, &w, infile, &src_align, &src_spb);

    c.src_format = w.format;
    c.chans = w.chans;

    if (c.chans < 1 || c.chans > MAX_CHANS) die ("unsupported channel count");

    if (c.src_format == SRC_MTF) {
        int unit = 4 * c.chans;

        c.align = opt_align / unit * unit;                      // 4*nch*(groups+1)
        if (c.align < 2 * unit) c.align = 2 * unit;
        if (c.align > 65535 / unit * unit) c.align = 65535 / unit * unit;
    }
    else {
        c.align = src_align;
        if (c.align < 4 * c.chans) die ("block size smaller than the block header");
    }

    const uint8_t *src = file + w.data_off;
    size_t n = w.data_size;

    if (!quiet)
    {
        fprintf (stderr, " input:   %s, %s, %d ch, %lu Hz, %lu data bytes",
            infile, c.src_format == SRC_MTF ? "MT Framework ADPCM" : "IMA-ADPCM",
            c.chans, (unsigned long) w.rate, (unsigned long) n);

        if (c.src_format == SRC_MTF)
            fprintf (stderr, "\n");
        else
            fprintf (stderr, ", block %d, %lu block(s)\n",
                c.align, (unsigned long) ((n + c.align - 1) / c.align));
    }

    Buf dst;
    Streams s;
    uint64_t samples = 0;

    repack_fwd (c, src, n, dst, s, &samples);

    // verify the inverse right here: this pair is only useful if it round-trips
    {
        Buf test;
        Joined in;

        in.side.set (s.side.d, s.side.n, infile);
        in.pred.set (s.pred.d, s.pred.n, infile);
        in.sidx.set (s.sidx.d, s.sidx.n, infile);
        in.rsvx.set (s.rsvx.d, s.rsvx.n, infile);
        repack_inv (c, dst.d, dst.n, test, n, in);

        if (test.n != n || memcmp (test.d, src, n))
            die ("internal error: round-trip mismatch, nothing written");
    }

    // the source container header, with its derivable fields blanked
    {
        Buf h;

        h.put (file, w.head_size);
        head_blank (h.d, h.n, c.src_format);
        rle_pack (s.head, h.d, h.n);
        if (s.head.n >= h.n) { s.head.n = 0; s.head.put (h.d, h.n); }
    }

    int flags = (s.head.n < w.head_size) ? MI_FLAG_RLE : 0;

    Buf meta;
    uint8_t crcs [8];

    meta.put (MI_MAGIC, 4);
    meta.put (MI_VERSION);
    meta.put (c.src_format);
    meta.put (c.chans);
    meta.put (flags);
    wr32 (crcs, crc32_buf (file, fsize));                       // guards the restored file
    wr32 (crcs + 4, crc32_buf (dst.d, dst.n));                  // pairs metainfo with its wav
    meta.put (crcs, 8);
    put_varint (meta, (uint64_t) c.align);
    put_varint (meta, (uint64_t) n);
    put_varint (meta, (uint64_t) w.head_size);
    put_varint (meta, (uint64_t) w.tail_size);
    put_varint (meta, (uint64_t) w.duration);
    put_varint (meta, (uint64_t) s.head.n);
    put_varint (meta, (uint64_t) s.side.n);
    put_varint (meta, (uint64_t) s.pred.n);
    put_varint (meta, (uint64_t) s.sidx.n);
    put_varint (meta, (uint64_t) s.rsvx.n);

    size_t fixed = meta.n;

    meta.put (s.head.d, s.head.n);
    meta.put (file + w.data_off + n, w.tail_size);
    meta.put (s.side.d, s.side.n);
    meta.put (s.pred.d, s.pred.n);
    meta.put (s.sidx.d, s.sidx.n);
    meta.put (s.rsvx.d, s.rsvx.n);

    Buf out;
    int embed = !metafile;

    if (c.src_format == SRC_MTF) {
        uint32_t extra = embed ? (uint32_t) (8 + meta.n + (meta.n & 1)) : 0;

        build_ima_head (out, c.chans, w.rate, c.align, (uint32_t) samples, (uint32_t) dst.n, extra);
        out.put (dst.d, dst.n);
        if (dst.n & 1) out.put (0);                             // RIFF chunks are padded

        if (embed) {
            uint8_t ch [8];

            memcpy (ch, MI_MAGIC, 4);
            wr32 (ch + 4, (uint32_t) meta.n);
            out.put (ch, 8);
            out.put (meta.d, meta.n);
            if (meta.n & 1) out.put (0);
        }
    }
    else {
        build_fwse_head (out, c.chans, w.rate, (uint32_t) dst.n,
            embed ? meta.d : NULL, embed ? meta.n : 0);
        out.put (dst.d, dst.n);
    }

    if (!embed) save_file (metafile, meta.d, meta.n);
    save_file (outfile, out.d, out.n);

    if (!quiet) {
        fprintf (stderr, " output:  %s, %s, %lu data bytes, %lu samples/ch",
            outfile, c.src_format == SRC_MTF ? "IMA-ADPCM" : "MT Framework ADPCM",
            (unsigned long) dst.n, (unsigned long) samples);

        if (c.src_format == SRC_MTF)
            fprintf (stderr, ", block %d", c.align);

        fprintf (stderr, "\n meta:    %s, %lu bytes = %lu hdr + %lu riff%s + %lu odd + %lu %s\n",
            embed ? (c.src_format == SRC_MTF ? "in the wav's M2I1 chunk" : "in the FWSE header")
                  : metafile,
            (unsigned long) meta.n, (unsigned long) fixed,
            (unsigned long) s.head.n, (flags & MI_FLAG_RLE) ? " (rle)" : "",
            (unsigned long) (s.side.n + w.tail_size),
            (unsigned long) (s.pred.n + s.sidx.n + s.rsvx.n),
            c.src_format == SRC_MTF ? "block state (derived)" : "pred+index+exc");
        fprintf (stderr, " codes:   %lu carried, order-preserving map, step index preserved,"
                         " verified exact\n",
            (unsigned long) ((c.src_format == SRC_MTF ? n : dst.n) * 2 / (unsigned) c.chans));
    }

    free (file);
}

/*--------------------------------- restore ----------------------------------*/

static void restore (const char *infile, const char *outfile, const char *metafile)
{
    size_t fsize, msize = 0;
    uint8_t *file = load_file (infile, &fsize);
    uint8_t *mfile = NULL;
    const uint8_t *mdata = NULL;
    const char *mname = metafile;
    Src w;
    Ctx c;
    int dst_align = 0, dst_spb = 0;

    if (metafile)
        mdata = mfile = load_file (metafile, &msize);
    else {
        mname = infile;

        if (fsize >= 4 && !memcmp (file, "FWSE", 4))
            mdata = (fsize >= 16) ? find_fwse_meta (file, rd32 (file + 12) < fsize
                ? rd32 (file + 12) : fsize, &msize) : NULL;
        else if (fsize >= 12 && !memcmp (file, "RIFF", 4))
            mdata = find_wav_meta (file, fsize, &msize);

        if (!mdata)
            die ("no metainfo inside, and none named on the command line:", infile);
    }

    if (msize < 12 || memcmp (mdata, MI_MAGIC, 4))
        die ("not an mtf2ima metainfo file:", mname);

    Cursor m;
    m.set (mdata + 4, msize - 4, mname);

    if (m.byte () != MI_VERSION) die ("metainfo written by a different mtf2ima version:", mname);

    c.src_format = m.byte ();
    c.chans = m.byte ();

    int flags = m.byte ();
    uint32_t crc_src = 0, crc_dst = 0;
    {
        uint8_t crcs [8];
        for (int i = 0; i < 8; ++i) crcs [i] = (uint8_t) m.byte ();
        crc_src = rd32 (crcs);
        crc_dst = rd32 (crcs + 4);
    }

    c.align = (int) get_varint (m);

    size_t n = (size_t) get_varint (m), head_size = (size_t) get_varint (m);
    size_t tail_size = (size_t) get_varint (m);
    uint32_t duration = (uint32_t) get_varint (m);
    size_t head_bytes = (size_t) get_varint (m), side_size = (size_t) get_varint (m);
    size_t pred_size = (size_t) get_varint (m), sidx_size = (size_t) get_varint (m);
    size_t rsvx_size = (size_t) get_varint (m);

    if ((c.src_format != SRC_MTF && c.src_format != SRC_IMA) ||
        c.chans < 1 || c.chans > MAX_CHANS || c.align < 4 * c.chans)
            die ("corrupt metainfo from", mname);

    // the converted file: an IMA wav if the source was FWSE, an FWSE if it was a wav
    if (c.src_format == SRC_MTF) {
        parse_wav (file, fsize, &w, infile, &dst_align, &dst_spb);
        if (dst_align != c.align) die ("block size does not match the metainfo:", infile);
    }
    else
        parse_fwse (file, fsize, &w, infile);

    if (w.chans != c.chans) die ("channel count does not match the metainfo:", infile);

    size_t stored = head_bytes + tail_size + side_size + pred_size + sidx_size + rsvx_size;

    if (m.p + stored > m.n) die ("truncated metainfo from", mname);

    const uint8_t *q = mdata + 4 + m.p;
    Cursor hc;
    hc.set (q, head_bytes, mname); q += head_bytes;
    const uint8_t *tail = q; q += tail_size;

    Joined in;
    in.side.set (q, side_size, mname); q += side_size;
    in.pred.set (q, pred_size, mname); q += pred_size;
    in.sidx.set (q, sidx_size, mname); q += sidx_size;
    in.rsvx.set (q, rsvx_size, mname);

    if (crc32_buf (file + w.data_off, w.data_size) != crc_dst)
        die ("sample stream does not match this metainfo (mismatched pair?):", infile);

    if (!quiet)
        fprintf (stderr, " input:   %s, %s, %lu data bytes\n meta:    %s, %lu bytes\n",
            infile, c.src_format == SRC_MTF ? "IMA-ADPCM" : "MT Framework ADPCM",
            (unsigned long) w.data_size,
            metafile ? metafile : (c.src_format == SRC_MTF ? "in the wav's M2I1 chunk"
                                                           : "in the FWSE header"),
            (unsigned long) msize);

    Buf out;

    if (flags & MI_FLAG_RLE) rle_unpack (out, hc, head_size);
    else for (size_t i = 0; i < head_size; ++i) out.put (hc.byte ());

    head_rebuild (out.d, head_size, c.src_format, n, duration);

    repack_inv (c, file + w.data_off, w.data_size, out, n, in);
    out.put (tail, tail_size);

    if (crc32_buf (out.d, out.n) != crc_src)
        die ("restored file does not match the recorded CRC-32, output not written");

    save_file (outfile, out.d, out.n);

    if (!quiet)
        fprintf (stderr, " output:  %s, %s, %lu bytes, crc32 %08lx verified\n",
            outfile, c.src_format == SRC_MTF ? "MT Framework ADPCM" : "IMA-ADPCM",
            (unsigned long) out.n, (unsigned long) crc_src);

    free (mfile);
    free (file);
}

/*----------------------------------- main -----------------------------------*/

int main (int argc, char **argv)
{
    const char *infile = NULL, *outfile = NULL, *metafile = NULL;
    int mode = 0;

    while (--argc) {
        char *arg = *++argv;

#if defined (_WIN32)
        if ((*arg == '-' || *arg == '/') && arg [1]) {
#else
        if (*arg == '-' && arg [1]) {
#endif
            while (*++arg)
                switch (*arg) {
                    case 'Q': case 'q': quiet = 1; break;
                    case 'B': case 'b':
                        opt_align = (int) strtol (arg + 1, &arg, 10);
                        --arg;
                        if (opt_align < 8 || opt_align > 65535)
                            die ("block size must be between 8 and 65535");
                        break;
                    default:
                        fprintf (stderr, "mtf2ima: unknown option '%c'\n", *arg);
                        return 1;
                }
        }
        else if (!mode && (!strcmp (arg, "c") || !strcmp (arg, "d")))
            mode = *arg;
        else if (!infile)
            infile = arg;
        else if (!outfile)
            outfile = arg;
        else if (!metafile)
            metafile = arg;
        else {
            fprintf (stderr, "mtf2ima: extra argument \"%s\"\n", arg);
            return 1;
        }
    }

    if (!mode || !infile || !outfile) {
        fprintf (stderr, "%s%s", sign_on, usage);
        return 1;
    }

    if (!strcmp (infile, outfile) ||
        (metafile && (!strcmp (metafile, infile) || !strcmp (metafile, outfile))))
            die ("input, output and metainfo files must all differ");

    if (mode == 'c')
        convert (infile, outfile, metafile);
    else
        restore (infile, outfile, metafile);

    return 0;
}
