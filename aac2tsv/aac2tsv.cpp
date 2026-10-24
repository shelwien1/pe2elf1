/*
 * aac2tsv - lossless AAC <-> TSV converter
 *
 *   aac2tsv c [options] input.aac output.tsv    AAC bitstream -> TSV records
 *   aac2tsv d [options] input.tsv output.aac    TSV records -> identical AAC
 *
 * The AAC syntax (ADTS frames, raw_data_block elements, individual channel
 * streams with sections, scalefactors, TNS, pulse, gain control and the
 * Huffman-decoded spectral values, fill elements with SBR, PS and DRC) is
 * written as one record per line. Whatever does not parse (other data,
 * corrupt frames) is kept as hex, so any input converts back bit-exactly.
 *
 * Built on the syntax and tables of aacdec (a standalone C++ port of the
 * FFmpeg 8.1.3 AAC decoder). Licensed under the GNU Lesser General Public
 * License v2.1 or later.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <string>
#include <vector>
#include <deque>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#include "tables.inc"
#include "bitio.h"
#include "huff.h"
#include "sbrfreq.h"
#include "tsv.h"
#include "syntax.h"
#include "asc.h"

#define AAC2TSV_VERSION 1

static int verbose = 1;

/* ======================= ADTS ======================= */

/* frame length if p holds a usable ADTS header, else 0 */
static size_t adts_frame_len(const uint8_t *p, size_t avail)
{
    if (avail < 7 || p[0] != 0xff || (p[1] & 0xf6) != 0xf0)   /* syncword, layer 0 */
        return 0;
    if (((p[2] >> 2) & 15) > 12)
        return 0;
    size_t len = (size_t)(p[3] & 3) << 11 | (size_t)p[4] << 3 | p[5] >> 5;
    size_t min = 7 + ((p[1] & 1) ? 0 : 2 + 2 * (p[6] & 3));
    return len >= min ? len : 0;
}

static uint16_t adts_crc(const uint8_t *b, size_t hdr_start, size_t hdr_bits, const std::vector<CrcReg> *regs)
{
    uint16_t crc = 0xffff;
    crc = crc16_bits(crc, b, hdr_start, hdr_bits);
    if (regs)
        for (const CrcReg &r : *regs) {
            size_t len = r.end - r.start;
            if (r.max) {
                size_t n = len < (size_t)r.max ? len : (size_t)r.max;
                crc = crc16_bits(crc, b, r.start, n);
                crc = crc16_zeros(crc, r.max - n);
            } else {
                crc = crc16_bits(crc, b, r.start, len);
            }
        }
    return crc;
}

static void adts_config(Codec &c, const AdtsFixed &fx)
{
    c.st.er = 0;
    c.st.aot = fx.profile + 1;
    c.st.sf_index = fx.sf_index;
    c.st.frame960 = 0;
    c.st.sbr_rate = 2 * sample_rates[fx.sf_index];
}

/* c mode: ADTS frame at byte fs, flen bytes -> records */
static void adts_frame_r(Codec &c, size_t fs, size_t flen)
{
    BitReader &br = c.br;
    OutFrame &o = *c.out;
    const size_t fbeg = fs * 8, fend = (fs + flen) * 8;
    br = BitReader(c.buf, fbeg, fend);

    br.get(12);   /* syncword */
    AdtsFixed fx;
    fx.id = br.get1();
    fx.layer = br.get(2);
    fx.prot_absent = br.get1();
    fx.profile = br.get(2);
    fx.sf_index = br.get(4);
    fx.private_bit = br.get1();
    fx.chan_config = br.get(3);
    fx.original = br.get1();
    fx.home = br.get1();
    int cib = br.get1(), cis = br.get1();
    br.get(13);   /* aac_frame_length: derived */
    int fullness = br.get(11), nrdb = br.get(2);

    if (!c.st.have_fixed || memcmp(&fx, &c.st.fixed, sizeof(fx))) {
        OutRec &r = o.add(R_ADTSH);
        r.i(fx.id); r.i(fx.layer); r.i(fx.prot_absent); r.i(fx.profile); r.i(fx.sf_index);
        r.i(fx.private_bit); r.i(fx.chan_config); r.i(fx.original); r.i(fx.home);
        c.st.fixed = fx;
        c.st.have_fixed = 1;
    }
    {
        OutRec &r = o.add(R_ADTS);
        r.i(fullness);
        r.i(nrdb);
        if (cib || cis) {
            r.i(cib);
            r.i(cis);
        }
    }
    adts_config(c, fx);

    unsigned pos[3] = { 0, 0, 0 }, hcrc = 0, bcrc[4] = { 0, 0, 0, 0 };
    size_t posri = 0, crcri = 0, bcrcri[4] = { 0, 0, 0, 0 };
    if (!fx.prot_absent) {
        if (nrdb) {
            posri = o.n;
            o.add(R_RDBPOS).omit = true;
            for (int i = 0; i < nrdb; i++)
                pos[i] = br.get(16);
        }
        crcri = o.n;
        o.add(R_CRC).omit = true;
        hcrc = br.get(16);
    }

    size_t bstart[4];
    std::vector<CrcReg> regs[4];
    int nstarted = 0, nbcrc = 0;
    bool ok = true;
    for (int b = 0; b <= nrdb; b++) {
        bstart[b] = br.pos;
        nstarted++;
        ok = c.raw_data_block(fend, true);
        if (!ok)
            break;
        regs[b] = c.crcr;
        if (!fx.prot_absent && nrdb) {
            if (br.left() < 16) {
                c.rest_r(br.pos, fend);
                ok = false;
                break;
            }
            bcrcri[b] = o.n;
            o.add(R_CRC).omit = true;
            bcrc[b] = br.get(16);
            nbcrc++;
        }
    }
    if (ok && br.pos < fend) {
        OutRec &r = o.add(R_TAIL);
        r.hex(c.buf, br.pos, fend - br.pos);
        br.pos = fend;
    }

    /* CRCs and block positions are written only when they are not the
     * values computed from the frame */
    if (!fx.prot_absent) {
        if (nrdb) {
            bool match = true;
            for (int i = 1; i <= nrdb; i++)
                if (i >= nstarted || pos[i - 1] != (bstart[i] - bstart[0]) / 8)
                    match = false;
            if (!match) {
                o.recs[posri].omit = false;
                for (int i = 0; i < nrdb; i++)
                    o.recs[posri].i(pos[i]);
            }
            if (adts_crc(c.buf, fbeg, 56 + 16 * nrdb, nullptr) != hcrc) {
                o.recs[crcri].omit = false;
                o.recs[crcri].i(hcrc);
            }
            for (int b = 0; b < nbcrc; b++) {
                if (adts_crc(c.buf, 0, 0, &regs[b]) != bcrc[b]) {
                    o.recs[bcrcri[b]].omit = false;
                    o.recs[bcrcri[b]].i(bcrc[b]);
                }
            }
        } else if (!ok || adts_crc(c.buf, fbeg, 56, &regs[0]) != hcrc) {
            o.recs[crcri].omit = false;
            o.recs[crcri].i(hcrc);
        }
    }
}

/* d mode: rest record in place of a block CRC */
static bool rest_w(Codec &c)
{
    if (c.peek() != R_REST)
        return false;
    c.rb(R_REST);
    int64_t n = c.geti(0, (int64_t)1 << 40);
    c.put_nibbles(c.getnibbles(), (size_t)n);
    return true;
}

/* d mode: adts record (and what follows) -> c.fw */
static void adts_frame_w(Codec &c)
{
    c.rb(R_ADTS);
    int fullness = (int)c.geti(0, 2047), nrdb = (int)c.geti(0, 3);
    int cib = c.more() ? (int)c.geti(0, 1) : 0;
    int cis = c.more() ? (int)c.geti(0, 1) : 0;
    if (!c.st.have_fixed)
        c.err("adts record before the first adtsh record");
    const AdtsFixed fx = c.st.fixed;
    if (fx.sf_index > 12)
        c.err("adtsh: sampling frequency index > 12");
    c.fw.clear();
    c.w = &c.fw;
    c.wdepth = 0;
    BitWriter &w = c.fw;
    w.put(0xfff, 12);
    w.put(fx.id, 1);
    w.put(fx.layer, 2);
    w.put(fx.prot_absent, 1);
    w.put(fx.profile, 2);
    w.put(fx.sf_index, 4);
    w.put(fx.private_bit, 1);
    w.put(fx.chan_config, 3);
    w.put(fx.original, 1);
    w.put(fx.home, 1);
    w.put(cib, 1);
    w.put(cis, 1);
    size_t flpos = w.pos;
    w.put(0, 13);
    w.put(fullness, 11);
    w.put(nrdb, 2);
    adts_config(c, fx);

    int64_t xpos[3] = { -1, -1, -1 }, xhcrc = -1, xbcrc[4] = { -1, -1, -1, -1 };
    size_t pospos = 0, hcrcpos = 0, bcrcpos[4] = { 0, 0, 0, 0 };
    if (!fx.prot_absent) {
        if (nrdb) {
            if (c.peek() == R_RDBPOS) {
                c.rb(R_RDBPOS);
                for (int i = 0; i < nrdb; i++)
                    xpos[i] = c.geti(0, 65535);
            }
            pospos = w.pos;
            for (int i = 0; i < nrdb; i++)
                w.put(0, 16);
        }
        if (c.peek() == R_CRC) {
            c.rb(R_CRC);
            xhcrc = c.geti(0, 65535);
        }
        hcrcpos = w.pos;
        w.put(0, 16);
    }

    size_t bstart[4];
    std::vector<CrcReg> regs[4];
    int nstarted = 0, nbcrc = 0;
    bool ok = true;
    for (int b = 0; b <= nrdb; b++) {
        bstart[b] = w.pos;
        nstarted++;
        ok = c.raw_data_block(0, true);
        if (!ok)
            break;
        regs[b] = c.crcr;
        if (!fx.prot_absent && nrdb) {
            if (rest_w(c)) {
                ok = false;
                break;
            }
            if (c.peek() == R_CRC) {
                c.rb(R_CRC);
                xbcrc[b] = c.geti(0, 65535);
            }
            bcrcpos[b] = w.pos;
            w.put(0, 16);
            nbcrc++;
        }
    }
    if (ok && c.peek() == R_TAIL) {
        c.rb(R_TAIL);
        for (uint8_t v : c.getbytes())
            w.put(v, 8);
    }
    c.wend_check();
    if (w.pos % 8)
        c.err("ADTS frame is not a whole number of bytes");
    if (w.pos / 8 > 8191)
        c.err("ADTS frame longer than 8191 bytes");
    w.patch(flpos, (uint32_t)(w.pos / 8), 13);
    if (!fx.prot_absent) {
        if (nrdb) {
            for (int i = 1; i <= nrdb; i++) {
                int64_t v = xpos[0] >= 0 ? xpos[i - 1] : i < nstarted ? (int64_t)((bstart[i] - bstart[0]) / 8) : 0;
                w.patch(pospos + 16 * (i - 1), (uint32_t)v, 16);
            }
            w.patch(hcrcpos, xhcrc >= 0 ? (uint32_t)xhcrc : adts_crc(w.b.data(), 0, 56 + 16 * nrdb, nullptr), 16);
            for (int b = 0; b < nbcrc; b++)
                w.patch(bcrcpos[b], xbcrc[b] >= 0 ? (uint32_t)xbcrc[b] : adts_crc(w.b.data(), 0, 0, &regs[b]), 16);
        } else {
            uint16_t crc = xhcrc >= 0 ? (uint16_t)xhcrc : adts_crc(w.b.data(), 0, 56, ok ? &regs[0] : nullptr);
            w.patch(hcrcpos, crc, 16);
        }
    }
}

static void adtsh_w(Codec &c)
{
    c.rb(R_ADTSH);
    AdtsFixed fx;
    fx.id = (int)c.geti(0, 1);
    fx.layer = (int)c.geti(0, 3);
    fx.prot_absent = (int)c.geti(0, 1);
    fx.profile = (int)c.geti(0, 3);
    fx.sf_index = (int)c.geti(0, 15);
    fx.private_bit = (int)c.geti(0, 1);
    fx.chan_config = (int)c.geti(0, 7);
    fx.original = (int)c.geti(0, 1);
    fx.home = (int)c.geti(0, 1);
    c.st.fixed = fx;
    c.st.have_fixed = 1;
}

/* ======================= files ======================= */

/* ======================= LOAS / LATM ======================= */

static size_t loas_frame_len(const uint8_t *p, size_t avail)
{
    if (avail < 3 || p[0] != 0x56 || (p[1] & 0xe0) != 0xe0)
        return 0;
    return ((size_t)(p[1] & 0x1f) << 8 | p[2]) + 3;
}

/* LatmGetValue(): bytesForValue and value, two fields */
static int64_t latm_value(Codec &c)
{
    int nb = c.u(2);
    return c.u(8 * (nb + 1));
}

static void set_config(SynState &st, const AscInfo &a, bool blocks)
{
    st.aot = a.aot;
    st.sf_index = a.sf_index;
    st.frame960 = a.frame960;
    st.sbr_rate = a.sf_index <= 12 ? 2 * sample_rates[a.sf_index] : 0;
    st.blocks = blocks && a.raw_blocks;
    st.er = a.er;
    st.chan_config = a.chan_config;
}

/* AudioSpecificConfig of nbits as a hex field; for LATM version 0 its length
 * is found by parsing it (fixed = false) */
static void asc_field(Codec &c, int64_t nbits_fixed)
{
    AscInfo a;
    bool ok = true;
    if (c.R) {
        size_t a0 = c.br.pos;
        if (nbits_fixed < 0) {
            a = parse_asc(c.br, a0);   /* unparsable: the frame is stored raw */
            c.cr->i((int64_t)(c.br.pos - a0));
            c.cr->hex(c.buf, a0, c.br.pos - a0);
        } else {
            if ((size_t)nbits_fixed > c.br.left())
                parse_fail("AudioSpecificConfig overrun");
            try {
                BitReader b(c.buf, a0, a0 + nbits_fixed);
                a = parse_asc(b, a0);
            } catch (ParseError &) {
                ok = false;
            }
            c.cr->i(nbits_fixed);
            c.cr->hex(c.buf, a0, (size_t)nbits_fixed);
            c.br.pos = a0 + nbits_fixed;
        }
    } else {
        int64_t nbits = c.geti(0, 1 << 20);
        if (nbits_fixed >= 0 && nbits != nbits_fixed)
            c.err("AudioSpecificConfig length does not match ascLen");
        size_t a0 = c.w->pos;
        c.put_nibbles(c.getnibbles(), (size_t)nbits);
        try {
            BitReader b(c.w->b.data(), a0, a0 + nbits);
            a = parse_asc(b, a0);
            if (nbits_fixed < 0 && b.pos != b.end)
                c.err("AudioSpecificConfig length does not match its content");
        } catch (ParseError &) {
            if (nbits_fixed < 0)
                c.err("unparsable AudioSpecificConfig");
            ok = false;
        }
    }
    if (ok)
        set_config(c.st, a, true);
    else
        c.st.blocks = 0;
}

/* StreamMuxConfig(), the subset with one program and layer, all streams in
 * the same time framing, frameLengthType 0 (as aacdec) */
static void stream_mux_config(Codec &c)
{
    SynState &st = c.st;
    c.rb(R_SMC);
    int v = c.u(1);                 /* audioMuxVersion */
    if (v && c.u(1))                /* audioMuxVersionA */
        c.bad("audioMuxVersionA 1");
    if (v)
        latm_value(c);              /* taraBufferFullness */
    if (!c.u(1))                    /* allStreamsSameTimeFraming */
        c.bad("allStreamsSameTimeFraming 0");
    int nsub = c.u(6);              /* numSubFrames */
    if (c.u(4))                     /* numProgram */
        c.bad("several programs");
    if (c.u(3))                     /* numLayer */
        c.bad("several layers");
    c.rb(R_ASC);
    if (v) {
        /* ascLen as the trailing field: [nbits, hex, bytesForValue] */
        if (c.R) {
            int nb = c.br.get(2);
            int64_t len = c.br.get(8 * (nb + 1));
            asc_field(c, len);
            c.cr->i(nb);
        } else {
            InRec *r = c.ir;
            if (r->nf() != 3)
                c.err("asc record of LATM version 1 needs 3 fields");
            int64_t len = parse_int(*r, 0), nb = parse_int(*r, 2);
            if (nb < 0 || nb > 3 || len < 0 || (nb < 3 && len >> (8 * (nb + 1))))
                c.err("bad ascLen");
            c.w->put((uint32_t)nb, 2);
            c.w->put((uint32_t)len, 8 * ((int)nb + 1));
            asc_field(c, len);
            c.geti();
        }
    } else {
        asc_field(c, -1);
    }
    c.rb(R_SMCX);
    if (c.u(3))                     /* frameLengthType */
        c.bad("frameLengthType other than 0");
    c.u(8);                         /* latmBufferFullness */
    int odp = c.u(1);               /* otherDataPresent */
    int64_t obits = 0;
    if (odp) {
        if (v) {
            obits = latm_value(c);  /* otherDataLenBits */
        } else if (c.R) {
            int n = 0, esc;
            do {                    /* otherDataLenEsc, otherDataLenTmp */
                esc = c.br.get1();
                obits = obits << 8 | c.br.get(8);
                if (++n > 3)
                    parse_fail("otherDataLenBits too large");
            } while (esc);
            c.cr->i(n);
            c.cr->i(obits);
        } else {
            int n = (int)c.geti(1, 3);
            obits = c.geti(0, ((int64_t)1 << (8 * n)) - 1);
            for (int k = n - 1; k >= 0; k--) {
                c.w->put(k > 0, 1);
                c.w->put((uint32_t)(obits >> (8 * k)) & 255, 8);
            }
        }
        if (obits > 8 * 8192)
            c.bad("otherDataLenBits larger than a LOAS frame");
    }
    if (c.u(1))                     /* crcCheckPresent */
        c.u(8);                     /* crcCheckSum */
    st.latm_cfg = 1;
    st.mux_version = v;
    st.num_subframes = nsub;
    st.other_present = odp;
    st.other_bits = (int)obits;
}

/* c mode: AudioSyncStream frame -> records */
static void loas_frame_r(Codec &c, size_t fs, size_t flen)
{
    BitReader &br = c.br;
    OutFrame &o = *c.out;
    const size_t fend = (fs + flen) * 8;
    br = BitReader(c.buf, fs * 8, fend);
    br.get(11);   /* syncword */
    br.get(13);   /* audioMuxLengthBytes: derived */
    const size_t ame = br.pos;
    size_t li = o.n;
    c.rb(R_LOAS);
    if (!c.u(1))  /* useSameStreamMux */
        stream_mux_config(c);
    else if (!c.st.latm_cfg)
        parse_fail("no StreamMuxConfig yet");
    for (int k = 0; k <= c.st.num_subframes; k++) {
        size_t len = 0;
        int t;
        do {      /* MuxSlotLengthBytes */
            t = br.get(8);
            len += t;
        } while (t == 255);
        if (len * 8 > br.left())
            parse_fail("payload overrun");
        size_t ps = br.pos, pe = ps + 8 * len;
        if (c.st.blocks) {
            c.rb(R_PAY);
            br.end = pe;
            bool ok = c.st.er ? c.er_frame(pe) : c.raw_data_block(pe, false);
            br.end = fend;
            if (ok) {
                size_t pad = pe - br.pos;
                if (pad >= 8 || (pad && br.peek((int)pad))) {
                    c.rb(R_PAYPAD);
                    c.cr->i((int64_t)pad);
                    c.cr->hex(c.buf, br.pos, pad);
                }
            }
        } else {
            c.rb(R_PAYRAW);
            c.cr->hex(c.buf, ps, 8 * len);
        }
        br.pos = pe;
    }
    if (c.st.other_present) {
        if ((size_t)c.st.other_bits > br.left())
            parse_fail("other data overrun");
        c.rb(R_OTHER);
        c.cr->hex(c.buf, br.pos, (size_t)c.st.other_bits);
        br.skip((size_t)c.st.other_bits);
    }
    unsigned ab = br.get((int)((ame - br.pos) & 7));   /* byte_align() */
    if (ab)
        o.recs[li].i(ab);
    if (br.pos < fend) {
        c.rb(R_TAIL);
        c.cr->hex(c.buf, br.pos, fend - br.pos);
        br.pos = fend;
    }
}

static void loas_frame_w(Codec &c)
{
    BitWriter &w = c.fw;
    w.put(0x2b7, 11);
    size_t lpos = w.pos;
    w.put(0, 13);
    const size_t ame = w.pos;
    c.rb(R_LOAS);
    int same = (int)c.geti(0, 1);
    int64_t ab = c.more() ? c.geti(1, 127) : 0;
    w.put(same, 1);
    if (!same)
        stream_mux_config(c);
    else if (!c.st.latm_cfg)
        c.err("loas frame without a StreamMuxConfig");
    for (int k = 0; k <= c.st.num_subframes; k++) {
        c.push_sub();
        if (c.st.blocks) {
            c.rb(R_PAY);
            if (c.st.er ? c.er_frame(0) : c.raw_data_block(0, false)) {
                if (c.peek() == R_PAYPAD) {
                    c.rb(R_PAYPAD);
                    int64_t n = c.geti(0, 8 * 8192);
                    c.put_nibbles(c.getnibbles(), (size_t)n);
                } else {
                    c.w->put(0, (int)((8 - c.w->pos % 8) % 8));
                }
            }
        } else {
            c.rb(R_PAYRAW);
            for (uint8_t v : c.getbytes())
                c.w->put(v, 8);
        }
        if (c.w->pos % 8)
            c.err("payload is not a whole number of bytes");
        BitWriter &sub = c.pop_sub();
        size_t len = sub.pos / 8;
        for (; len >= 255; len -= 255)
            w.put(255, 8);
        w.put((uint32_t)len, 8);
        w.append(sub);
    }
    if (c.st.other_present) {
        c.rb(R_OTHER);
        c.put_nibbles(c.getnibbles(), (size_t)c.st.other_bits);
    }
    int nb = (int)((ame - w.pos) & 7);
    if (ab >> nb)
        c.err("alignment bits out of range");
    w.put((uint32_t)ab, nb);
    if (c.peek() == R_TAIL) {
        c.rb(R_TAIL);
        for (uint8_t v : c.getbytes())
            w.put(v, 8);
    }
    c.wend_check();
    if (w.pos / 8 - 3 > 8191)
        c.err("LOAS frame longer than 8191 bytes");
    w.patch(lpos, (uint32_t)(w.pos / 8 - 3), 13);
}

/* ======================= ADIF, raw access units, AUS1 ======================= */

/* adif_header(); hs is the bit position of "ADIF" (PCE byte alignment) */
static void adif_header(Codec &c, size_t hs)
{
    if (c.R) {
        if (c.br.get(32) != 0x41444946)
            parse_fail("no ADIF header");
    } else {
        c.w->put(0x41444946, 32);
    }
    c.rb(R_ADIF);
    if (c.u(1)) {    /* copyright_id_present */
        if (c.R) {
            c.br.skip(72);
            c.cr->hex(c.buf, c.br.pos - 72, 72);
        } else {
            c.put_nibbles(c.getnibbles(), 72);
        }
    }
    c.u(1);          /* original_copy */
    c.u(1);          /* home */
    int bt = c.u(1); /* bitstream_type */
    c.u(23);         /* bitrate */
    int npce = c.u(4);
    int ot = 1, sfi = 4;
    for (int i = 0; i <= npce; i++) {
        c.rb(R_ADIFPCE);
        if (!bt)
            c.u(20); /* adif_buffer_fullness */
        int o2, s2;
        c.pce_body(hs, &o2, &s2);
        if (!i) {
            ot = o2;
            sfi = s2;
        }
    }
    AscInfo a;
    a.aot = ot + 1;
    a.sf_index = sfi;
    a.chan_config = 0;
    a.frame960 = 0;
    a.er = 0;
    a.raw_blocks = sfi <= 12;
    set_config(c.st, a, true);
}

/* raw_data_block() at top level (ADIF, raw access units, AUS1) */
static void block_w(Codec &c)
{
    if (!c.st.blocks)
        c.err("raw_data_block without a configuration that allows parsing it");
    c.raw_data_block(0, true);
}

static void put_le(BitWriter &w, uint32_t v, int bytes)
{
    for (int k = 0; k < bytes; k++)
        w.put((v >> (8 * k)) & 255, 8);
}

/* rawasc / aus1 record: the AudioSpecificConfig of a raw access unit stream */
static void asc_config_w(Codec &c, int name)
{
    c.rb(name);
    std::vector<uint8_t> asc = c.getbytes();
    asc.resize(asc.size() + 16, 0);
    try {
        BitReader b(asc.data(), 0, (asc.size() - 16) * 8);
        set_config(c.st, parse_asc(b, 0), true);
        if (name == R_RAWASC && c.st.er)
            c.st.blocks = 0;   /* not self-delimiting */
    } catch (ParseError &) {
        c.st.blocks = 0;
    }
    asc.resize(asc.size() - 16);
    if (name == R_AUS1) {
        BitWriter &w = c.fw;
        w.put(0x41555331, 32);   /* "AUS1" */
        put_le(w, (uint32_t)asc.size(), 2);
        for (uint8_t v : asc)
            w.put(v, 8);
    }
}

/* AUS1 access unit: le32 length, raw_data_block() or ER frame and the bits
 * after it (paypad), or the whole unit as payraw */
static void au_w(Codec &c)
{
    BitWriter &w = c.fw;
    c.rb(R_AU);
    w.put(0, 32);
    if (c.peek() == R_PAYRAW) {
        c.rb(R_PAYRAW);
        for (uint8_t v : c.getbytes())
            w.put(v, 8);
    } else {
        if (!c.st.blocks)
            c.err("access unit records without a configuration that allows parsing them");
        bool ok = c.st.er ? c.er_frame(0) : c.raw_data_block(0, true);
        if (ok) {
            if (c.peek() == R_PAYPAD) {
                c.rb(R_PAYPAD);
                int64_t n = c.geti(0, (int64_t)1 << 40);
                c.put_nibbles(c.getnibbles(), (size_t)n);
            } else {
                w.put(0, (int)((8 - w.pos % 8) % 8));
            }
        }
    }
    c.wend_check();
    uint32_t n = (uint32_t)(w.pos / 8 - 4);
    for (int k = 0; k < 4; k++)
        w.patch(8 * k, (n >> (8 * k)) & 255, 8);
}

/* ======================= TSV -> bytes, one unit ======================= */

/* d mode: the records of one unit at the head of the stream; its bytes are
 * left in c.fw. Returns false at the eof record. */
static bool decode_unit(Codec &c)
{
    c.fw.clear();
    c.w = &c.fw;
    c.wdepth = 0;
    int pk = c.peek();
    switch (pk) {
    case R_RAW:
        c.rb(R_RAW);
        for (uint8_t v : c.getbytes())
            c.fw.put(v, 8);
        break;
    case R_ADTSH:
        adtsh_w(c);
        break;
    case R_ADTS:
        adts_frame_w(c);
        break;
    case R_LOAS:
        loas_frame_w(c);
        break;
    case R_ADIF:
        adif_header(c, 0);
        break;
    case R_RAWASC:
    case R_AUS1:
        asc_config_w(c, pk);
        break;
    case R_AU:
        au_w(c);
        break;
    case R_SBRINIT:
        c.sbrinit_w();
        break;
    case R_SCE: case R_CPE: case R_CCE: case R_LFE: case R_DSE: case R_PCE: case R_FIL: case R_END: case R_REST:
        block_w(c);
        break;
    case R_EOF:
        return false;
    default:
        c.wend_check();
        if (pk < 0)
            c.err("missing eof record");
        throw TsvError{ c.in->line() + 1, std::string("unexpected record '") + rec_names[pk] + "'" };
    }
    c.wend_check();
    if (c.fw.pos % 8)
        c.err("unit is not a whole number of bytes");
    return true;
}

/* ======================= files ======================= */

static bool read_file(const char *name, std::vector<uint8_t> &data)
{
    FILE *f = strcmp(name, "-") ? fopen(name, "rb") : stdin;
    if (!f) {
        fprintf(stderr, "cannot open %s: %s\n", name, strerror(errno));
        return false;
    }
#ifdef _WIN32
    if (f == stdin)
        _setmode(_fileno(stdin), _O_BINARY);
#endif
    data.clear();
    std::vector<uint8_t> chunk(1 << 20);
    size_t n;
    while ((n = fread(chunk.data(), 1, chunk.size(), f)) > 0)
        data.insert(data.end(), chunk.begin(), chunk.begin() + n);
    bool e = ferror(f) != 0;
    if (f != stdin)
        fclose(f);
    if (e) {
        fprintf(stderr, "read error on %s\n", name);
        return false;
    }
    return true;
}

static FILE *open_out(const char *name)
{
    if (!strcmp(name, "-")) {
#ifdef _WIN32
        _setmode(_fileno(stdout), _O_BINARY);
#endif
        return stdout;
    }
    FILE *f = fopen(name, "wb");
    if (!f)
        fprintf(stderr, "cannot create %s: %s\n", name, strerror(errno));
    return f;
}

static bool close_out(FILE *f)
{
    bool e = ferror(f) != 0;
    if (f != stdout ? fclose(f) != 0 : fflush(f) != 0)
        e = true;
    if (e)
        fprintf(stderr, "write error\n");
    return !e;
}

struct Options {
    bool verify = true;
    std::vector<uint8_t> asc;   /* -asc: raw access units */
    int format = 0;             /* forced container: 0 auto */
};

enum { F_AUTO, F_ADTS, F_LOAS, F_ADIF, F_RAW };

/* ======================= c: AAC -> TSV ======================= */

struct Encoder {
    const Options &opt;
    const uint8_t *data;
    size_t size;
    FILE *fo;
    std::string obuf;
    std::vector<uint8_t> pend;     /* raw bytes not yet written */
    Codec rc, vc;                  /* reader, verifier */
    OutFrame of;
    std::string ftxt;
    RecStream vrs;
    uint64_t units = 0, raw_units = 0, raw_bytes = 0, rest_units = 0, hex_payloads = 0;
    /* probe: a parse-only pass that finds the first SBR header of the
     * elements whose first SBR payloads come without one */
    bool probe = false, stop = false;
    uint64_t probe_units = 0;
    int assume[3][16] = {};
    SbrState assumed[3][16];

    Encoder(const Options &o, const uint8_t *d, size_t n, FILE *f) : opt(o), data(d), size(n), fo(f)
    {
        rc.R = true;
        rc.buf = d;
        rc.out = &of;
        rc.st.reset();
        vc.R = false;
        vc.in = &vrs;
        vc.st.reset();
    }

    void flush_out(bool force)
    {
        if (probe) {
            obuf.clear();
            return;
        }
        if (obuf.size() >= (1 << 20) || (force && !obuf.empty())) {
            fwrite(obuf.data(), 1, obuf.size(), fo);
            obuf.clear();
        }
    }

    void flush_raw()
    {
        const size_t line = 32;
        for (size_t k = 0; k < pend.size(); k += line) {
            size_t n = pend.size() - k < line ? pend.size() - k : line;
            obuf += rec_names[R_RAW];
            obuf += '\t';
            for (size_t j = 0; j < n; j++) {
                obuf += hexdig[pend[k + j] >> 4];
                obuf += hexdig[pend[k + j] & 15];
            }
            obuf += '\n';
        }
        raw_bytes += pend.size();
        pend.clear();
        flush_out(false);
    }

    /* the unit text must decode to the same bytes and the same state */
    bool verify(const SynState &before, size_t start, size_t len)
    {
        vc.st = before;
        vrs.src.open_mem(ftxt.data(), ftxt.size());
        vrs.have_look = vrs.at_end = false;
        vc.ir = nullptr;
        size_t pos = 0;
        try {
            while (vrs.peek() >= 0) {
                if (!decode_unit(vc))
                    return false;
                size_t n = vc.fw.pos / 8;
                if (pos + n > len || (n && memcmp(vc.fw.b.data(), data + start + pos, n)))
                    return false;
                pos += n;
            }
        } catch (TsvError &e) {
            if (verbose > 1)
                fprintf(stderr, "verify: line %zu: %s\n", e.line, e.msg.c_str());
            return false;
        }
        return pos == len && vc.st.same(rc.st);
    }

    /* walk one unit at byte start with fn (returns its end byte, throws
     * ParseError if it does not parse); emits its records if they decode back
     * to the same bytes, else the bytes as raw. Returns the end byte, or 0 if
     * the unit did not parse. */
    /* probe: stop when every element with header-less SBR payloads has
     * found a header, or after a while without such payloads */
    void probe_check()
    {
        bool need = false, open = false;
        for (int k = 0; k < 3; k++)
            for (int t = 0; t < 16; t++)
                if (rc.sbr_need[k][t]) {
                    need = true;
                    open |= !rc.sbr_found[k][t];
                }
        probe_units++;
        stop = need ? !open : probe_units > 1000;
        pend.clear();
    }

    template <class F> size_t unit(size_t start, size_t known_len, F fn)
    {
        if (probe) {
            size_t end = start + known_len;
            SynState before = rc.st;
            of.clear();
            try {
                end = fn();
            } catch (ParseError &) {
                rc.st = before;
                if (!known_len)
                    return 0;
            }
            probe_check();
            return end;
        }
        SynState before = rc.st;
        of.clear();
        size_t end;
        try {
            end = fn();
        } catch (ParseError &) {
            rc.st = before;
            if (!known_len)
                return 0;
            if (verbose > 1)
                fprintf(stderr, "unit at offset %zu does not parse, stored raw\n", start);
            raw_units++;
            pend.insert(pend.end(), data + start, data + start + known_len);
            return start + known_len;
        }
        ftxt.clear();
        of.format(ftxt);
        units++;
        if (opt.verify && !verify(before, start, end - start)) {
            if (verbose)
                fprintf(stderr, "unit at offset %zu does not round-trip, stored raw\n", start);
            rc.st = before;
            raw_units++;
            pend.insert(pend.end(), data + start, data + end);
            return end;
        }
        bool rest = false;
        for (size_t k = 0; k < of.n; k++) {
            if (of.recs[k].omit)
                continue;
            rest |= of.recs[k].name == R_REST;
            hex_payloads += of.recs[k].name == R_PAYRAW;
        }
        rest_units += rest;
        flush_raw();
        obuf += ftxt;
        flush_out(false);
        return end;
    }

    static size_t id3v2_len(const uint8_t *p, size_t avail)
    {
        if (avail < 10 || memcmp(p, "ID3", 3) || p[3] == 0xff || p[4] == 0xff ||
            ((p[6] | p[7] | p[8] | p[9]) & 0x80))
            return 0;
        size_t len = ((size_t)p[6] << 21 | (size_t)p[7] << 14 | (size_t)p[8] << 7 | p[9]) + 10;
        if (p[5] & 0x10)
            len += 10;
        return len <= avail ? len : 0;
    }

    size_t frame_len(int fmt, size_t pos) const
    {
        return fmt == F_ADTS ? adts_frame_len(data + pos, size - pos) : loas_frame_len(data + pos, size - pos);
    }

    /* ADTS and LOAS frames, anything between them kept raw */
    void scan_frames(size_t pos)
    {
        bool insync = false;
        while (pos < size && !stop) {
            size_t tl = id3v2_len(data + pos, size - pos);
            if (tl) {
                pend.insert(pend.end(), data + pos, data + pos + tl);
                pos += tl;
                insync = false;
                continue;
            }
            int fmt = 0;
            size_t len = 0;
            for (int f = F_ADTS; f <= F_LOAS && !fmt; f++) {
                if (opt.format && opt.format != f)
                    continue;
                len = frame_len(f, pos);
                if (!len || len > size - pos)
                    continue;
                /* out of sync: also require a frame header after this one */
                if (insync || pos + len == size || frame_len(f, pos + len))
                    fmt = f;
            }
            if (!fmt) {
                pend.push_back(data[pos++]);
                insync = false;
                continue;
            }
            if (fmt == F_ADTS)
                unit(pos, len, [&] { adts_frame_r(rc, pos, len); return pos + len; });
            else
                unit(pos, len, [&] { loas_frame_r(rc, pos, len); return pos + len; });
            pos += len;
            insync = true;
        }
    }

    /* back-to-back raw_data_block()s until the end */
    void scan_blocks(size_t pos)
    {
        while (pos < size && !stop)
            pos = unit(pos, 0, [&] {
                rc.br = BitReader(data, pos * 8, size * 8);
                rc.raw_data_block(size * 8, true);
                return rc.br.pos / 8;
            });
    }

    bool adif(size_t pos)
    {
        size_t end = unit(pos, 0, [&] {
            rc.br = BitReader(data, pos * 8, size * 8);
            adif_header(rc, pos * 8);
            return rc.br.pos / 8;
        });
        if (!end)
            return false;
        if (rc.st.blocks)
            scan_blocks(end);
        else
            pend.insert(pend.end(), data + end, data + size);
        return true;
    }

    bool aus1(size_t pos)
    {
        if (size - pos < 6 || memcmp(data + pos, "AUS1", 4))
            return false;
        size_t alen = data[pos + 4] | data[pos + 5] << 8;
        if (size - pos - 6 < alen)
            return false;
        flush_raw();
        {   /* header: config from the AudioSpecificConfig */
            OutRec r = OutRec();
            r.name = R_AUS1;
            r.hexbytes(data + pos + 6, alen);
            obuf += rec_names[R_AUS1];
            obuf += r.s;
            obuf += '\n';
            try {
                BitReader b(data, (pos + 6) * 8, (pos + 6 + alen) * 8);
                set_config(rc.st, parse_asc(b, (pos + 6) * 8), true);
            } catch (ParseError &) {
                rc.st.blocks = 0;
            }
        }
        pos += 6 + alen;
        while (pos < size && !stop) {
            if (size - pos < 4)
                break;
            size_t n = data[pos] | data[pos + 1] << 8 | data[pos + 2] << 16 | (size_t)data[pos + 3] << 24;
            if (n > size - pos - 4)
                break;
            size_t s = pos;
            pos = unit(pos, n + 4, [&] {
                size_t ab = (s + 4) * 8, ae = (s + 4 + n) * 8;
                rc.rb(R_AU);
                rc.br = BitReader(data, ab, ae);
                if (!rc.st.blocks) {
                    rc.rb(R_PAYRAW);
                    rc.cr->hex(data, ab, ae - ab);
                    return s + 4 + n;
                }
                if (rc.st.er ? rc.er_frame(ae) : rc.raw_data_block(ae, true)) {
                    size_t pad = ae - rc.br.pos;
                    if (pad >= 8 || (pad && rc.br.peek((int)pad))) {
                        rc.rb(R_PAYPAD);
                        rc.cr->i((int64_t)pad);
                        rc.cr->hex(data, rc.br.pos, pad);
                    }
                }
                return s + 4 + n;
            });
        }
        pend.insert(pend.end(), data + pos, data + size);
        return true;
    }

    void run()
    {
        obuf += rec_names[R_HDR];
        obuf += '\t';
        append_int(obuf, AAC2TSV_VERSION);
        obuf += "\n# aac2tsv: one AAC syntax structure per line, see README.md\n";
        for (int k = 0; k < 3; k++)
            for (int t = 0; t < 16; t++)
                if (assume[k][t]) {
                    const SbrState &a = assumed[k][t];
                    OutRec r = OutRec();
                    r.i(k); r.i(t); r.i(a.amp_res);
                    r.i(a.sp.start_freq); r.i(a.sp.stop_freq); r.i(a.sp.xover_band);
                    r.i(a.sp.freq_scale); r.i(a.sp.alter_scale); r.i(a.sp.noise_bands);
                    obuf += rec_names[R_SBRINIT];
                    obuf += r.s;
                    obuf += '\n';
                    Codec::sbr_assume(rc.st.sbr[k][t], a.amp_res, a.sp);
                }
        size_t pos = 0, tl;
        while ((tl = id3v2_len(data + pos, size - pos)) != 0) {
            pend.insert(pend.end(), data + pos, data + pos + tl);
            pos += tl;
        }
        if (!opt.asc.empty()) {
            obuf += rec_names[R_RAWASC];
            obuf += '\t';
            for (uint8_t v : opt.asc) {
                obuf += hexdig[v >> 4];
                obuf += hexdig[v & 15];
            }
            obuf += '\n';
            std::vector<uint8_t> a(opt.asc);
            a.resize(a.size() + 16, 0);
            BitReader b(a.data(), 0, opt.asc.size() * 8);
            set_config(rc.st, parse_asc(b, 0), true);
            scan_blocks(pos);
        } else if ((opt.format == F_AUTO || opt.format == F_ADIF) && size - pos >= 4 &&
                   !memcmp(data + pos, "ADIF", 4) && adif(pos)) {
        } else if (opt.format == F_AUTO && aus1(pos)) {
        } else {
            scan_frames(pos);
        }
        flush_raw();
        obuf += rec_names[R_EOF];
        obuf += '\t';
        append_int(obuf, (int64_t)size);
        obuf += '\t';
        uint32_t crc = crc32_update(0, data, size);
        for (int k = 28; k >= 0; k -= 4)
            obuf += hexdig[(crc >> k) & 15];
        obuf += '\n';
        flush_out(true);
        if (verbose > 1 && !probe)
            fprintf(stderr, "%llu units (%llu stored raw, %llu partly raw), %llu payloads not parsed, "
                            "%llu other bytes\n",
                    (unsigned long long)units, (unsigned long long)raw_units, (unsigned long long)rest_units,
                    (unsigned long long)hex_payloads, (unsigned long long)raw_bytes);
    }
};

static int cmd_encode(const Options &opt, const char *in, const char *outn)
{
    std::vector<uint8_t> data;
    if (!read_file(in, data))
        return 1;
    size_t size = data.size();
    data.resize(size + 16, 0);   /* the bit reader looks ahead */
    huff_init();
    if (!opt.asc.empty()) {
        std::vector<uint8_t> a(opt.asc);
        a.resize(a.size() + 16, 0);
        try {
            BitReader b(a.data(), 0, opt.asc.size() * 8);
            AscInfo ai = parse_asc(b, 0);
            if (!ai.raw_blocks)
                parse_fail("access units of this object type are not parsed");
            if (ai.er)
                parse_fail("ER access units do not end with END, they cannot be back to back");
        } catch (ParseError &e) {
            fprintf(stderr, "-asc: %s\n", e.msg);
            return 1;
        }
    }
    FILE *fo = open_out(outn);
    if (!fo)
        return 1;
    Encoder *p = new Encoder(opt, data.data(), size, nullptr);
    p->probe = true;
    int dl = debug_level;
    debug_level = 0;
    p->run();
    debug_level = dl;
    Encoder *e = new Encoder(opt, data.data(), size, fo);
    for (int k = 0; k < 3; k++)
        for (int t = 0; t < 16; t++)
            if (p->rc.sbr_need[k][t] && p->rc.sbr_found[k][t]) {
                e->assume[k][t] = 1;
                e->assumed[k][t] = p->rc.sbr_first[k][t];
            }
    delete p;
    e->run();
    delete e;
    return close_out(fo) ? 0 : 1;
}

/* ======================= d: TSV -> AAC ======================= */

static int cmd_decode(const Options &opt, const char *in, const char *outn)
{
    (void)opt;
    FILE *fi = strcmp(in, "-") ? fopen(in, "rb") : stdin;
    if (!fi) {
        fprintf(stderr, "cannot open %s: %s\n", in, strerror(errno));
        return 1;
    }
#ifdef _WIN32
    if (fi == stdin)
        _setmode(_fileno(stdin), _O_BINARY);
#endif
    FILE *fo = open_out(outn);
    if (!fo) {
        if (fi != stdin)
            fclose(fi);
        return 1;
    }
    huff_init();
    Codec *cp = new Codec;
    Codec &c = *cp;
    RecStream *rs = new RecStream;
    rs->src.open_file(fi);
    c.R = false;
    c.in = rs;
    c.st.reset();
    uint64_t written = 0;
    uint32_t crc = 0;
    int ret = 0;
    std::vector<uint8_t> obuf;
    try {
        if (rs->peek() != R_HDR)
            c.err("not an aac2tsv file");
        c.rb(R_HDR);
        int64_t ver = c.geti();
        if (ver != AAC2TSV_VERSION)
            c.err("unsupported aac2tsv version " + std::to_string(ver));
        while (decode_unit(c)) {
            size_t n = c.fw.pos / 8;
            obuf.insert(obuf.end(), c.fw.b.data(), c.fw.b.data() + n);
            crc = crc32_update(crc, c.fw.b.data(), n);
            written += n;
            if (obuf.size() >= (1 << 20)) {
                fwrite(obuf.data(), 1, obuf.size(), fo);
                obuf.clear();
            }
        }
        c.rb(R_EOF);
        int64_t sz = c.geti(0, INT64_MAX);
        std::string h = c.gets();
        uint32_t want = (uint32_t)strtoul(h.c_str(), nullptr, 16);
        c.wend_check();
        if (rs->peek() >= 0)
            c.err("records after eof");
        if ((uint64_t)sz != written || want != crc)
            fprintf(stderr, "warning: output differs from the original (%llu bytes, crc %08x; "
                            "original %lld bytes, crc %08x)\n",
                    (unsigned long long)written, crc, (long long)sz, want);
    } catch (TsvError &e) {
        fprintf(stderr, "%s: line %zu: %s\n", in, e.line, e.msg.c_str());
        ret = 1;
    }
    if (!obuf.empty())
        fwrite(obuf.data(), 1, obuf.size(), fo);
    if (fi != stdin)
        fclose(fi);
    delete rs;
    delete cp;
    if (!close_out(fo))
        ret = 1;
    return ret;
}

/* ======================= main ======================= */

static void usage()
{
    fprintf(stderr,
        "aac2tsv - lossless AAC <-> TSV converter\n"
        "usage: aac2tsv c|d [options] input output\n"
        "  c   AAC -> TSV: ADTS, LOAS/LATM, ADIF (autodetected), raw access units\n"
        "      with -asc; data that does not parse is kept as hex\n"
        "  d   TSV -> AAC, bit-exact\n"
        "  input/output '-' = stdin/stdout\n"
        "options (c):\n"
        "  -adts | -latm | -adif   only look for this container\n"
        "  -asc HEX    input is raw access units with this AudioSpecificConfig\n"
        "  -noverify   skip the check that each frame converts back (faster)\n"
        "  -q          quiet\n"
        "  -v          verbose (statistics); -vv also lists SBR payloads kept as hex\n");
}

static bool parse_hex(const char *s, std::vector<uint8_t> &out)
{
    out.clear();
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        s += 2;
    while (s[0]) {
        int hi = hexval((unsigned char)s[0]), lo = s[1] ? hexval((unsigned char)s[1]) : -1;
        if (hi < 0 || lo < 0)
            return false;
        out.push_back((uint8_t)(hi << 4 | lo));
        s += 2;
    }
    return !out.empty();
}

int main(int argc, char **argv)
{
    Options opt;
    const char *mode = nullptr, *in = nullptr, *out = nullptr;
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!mode) {
            mode = a;
        } else if (a[0] == '-' && a[1]) {
            if (!strcmp(a, "-noverify"))
                opt.verify = false;
            else if (!strcmp(a, "-q"))
                verbose = 0;
            else if (!strcmp(a, "-v"))
                verbose = 2;
            else if (!strcmp(a, "-vv")) {
                verbose = 2;
                debug_level = 1;
            }
            else if (!strcmp(a, "-adts"))
                opt.format = F_ADTS;
            else if (!strcmp(a, "-latm") || !strcmp(a, "-loas"))
                opt.format = F_LOAS;
            else if (!strcmp(a, "-adif"))
                opt.format = F_ADIF;
            else if (!strcmp(a, "-asc") && i + 1 < argc) {
                if (!parse_hex(argv[++i], opt.asc)) {
                    fprintf(stderr, "invalid -asc value (hex bytes expected)\n");
                    return 1;
                }
            } else {
                fprintf(stderr, "unknown option %s\n", a);
                usage();
                return 1;
            }
        } else if (!in) {
            in = a;
        } else if (!out) {
            out = a;
        } else {
            usage();
            return 1;
        }
    }
    if (!mode || !in || !out || strlen(mode) != 1 || (mode[0] != 'c' && mode[0] != 'd')) {
        usage();
        return 1;
    }
    return mode[0] == 'c' ? cmd_encode(opt, in, out) : cmd_decode(opt, in, out);
}
