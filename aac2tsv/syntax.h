/*
 * aac2tsv - AAC bitstream syntax, walked in both directions by the same
 * code: in R mode (c) syntax elements are read from the bitstream and
 * emitted as TSV records, in W mode (d) they are taken from TSV records and
 * written to the bitstream. Follows aacdec (FFmpeg) aacdec.inc,
 * aacdec_float.inc, aacsbr.inc, aacps.inc.
 *
 * Licensed under the GNU Lesser General Public License v2.1 or later.
 */

enum { TYPE_SCE, TYPE_CPE, TYPE_CCE, TYPE_LFE, TYPE_DSE, TYPE_PCE, TYPE_FIL, TYPE_END };
enum { EIGHT_SHORT_SEQUENCE = 2 };

static const int sample_rates[16] = {
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0
};

/* PS header values that persist between frames */
struct PsHdr {
    int enable_iid, iid_quant, nr_iid_par, nr_ipdopd_par, enable_icc, nr_icc_par, enable_ext;
};

/* SBR header state of one channel element (all int: compared with memcmp) */
struct SbrState {
    int have_hdr;   /* a header was received */
    int ok;         /* band tables valid for (sp, rate) */
    int rate;       /* SBR sample rate the tables were computed for */
    int amp_res;    /* bs_amp_res of the header */
    SbrSpectrum sp;
    SbrBands bands;
    PsHdr ps;
};

struct AdtsFixed {
    int id, layer, prot_absent, profile, sf_index, private_bit, chan_config, original, home;
};

/* syntax state carried from frame to frame */
struct SynState {
    int aot, sf_index, frame960, sbr_rate;   /* configuration of the stream */
    int blocks;                              /* access units are parsed raw_data_block()s */
    int have_fixed;                          /* ADTS: fixed header of the last adtsh */
    AdtsFixed fixed;
    int latm_cfg;                            /* LATM: StreamMuxConfig known */
    int mux_version, num_subframes, other_present, other_bits;
    SbrState sbr[3][16];   /* [SCE, CPE, CCE][element_instance_tag] */

    void reset() { memset(this, 0, sizeof(*this)); }
    bool same(const SynState &o) const { return !memcmp(this, &o, sizeof(*this)); }
};

/* ADTS CRC region: [start, end) bits, max bits (0 = all) */
struct CrcReg {
    size_t start, end;
    int max;
};

struct Ics {
    int ws;            /* window_sequence */
    int max_sfb;
    int num_windows, num_groups, group_len[8];
    const uint16_t *swb;
    int num_swb;
};

struct SbrCh {
    int num_env, num_noise, amp_res;
    uint8_t freq_res[8], df_env[8], df_noise[2];
};

static const int8_t sbr_ceil_log2[] = { 0, 1, 2, 2, 3, 3 };

struct Codec {
    bool R = true;

    /* R mode */
    const uint8_t *buf = nullptr;   /* whole input, padded */
    BitReader br;
    OutFrame *out = nullptr;
    OutRec *cr = nullptr;

    /* W mode */
    BitWriter fw;                    /* frame being written */
    std::vector<BitWriter *> wstack; /* sub-writers (counted payloads) */
    size_t wdepth = 0;
    BitWriter *w = &fw;
    RecStream *in = nullptr;
    InRec *ir = nullptr;
    size_t fi = 0;

    SynState st;

    /* per raw_data_block */
    int prev_che_type = -1, prev_che_tag = 0;
    std::vector<CrcReg> crcr;

    ~Codec()
    {
        for (BitWriter *b : wstack)
            delete b;
    }

    /* ---------- errors ---------- */

    [[noreturn]] void err(const std::string &msg)
    {
        throw TsvError{ in ? in->line() : 0, msg };
    }
    /* a constraint of the syntax is violated: bad bitstream (R) or bad TSV (W) */
    [[noreturn]] void bad(const char *msg)
    {
        if (R)
            parse_fail(msg);
        err(msg);
    }

    /* ---------- records ---------- */

    void wend_check()
    {
        if (ir && fi < ir->nf())
            err(std::string("extra field in '") + rec_names[ir->name] + "' record");
    }
    void rb(int name)
    {
        if (R) {
            cr = &out->add(name);
            return;
        }
        wend_check();
        InRec *r = in->next();
        if (!r)
            err(std::string("unexpected end of input, expected '") + rec_names[name] + "'");
        if (r->name != name)
            throw TsvError{ r->line, std::string("expected '") + rec_names[name] + "', found '" +
                                     rec_names[r->name] + "'" };
        ir = r;
        fi = 0;
    }
    int peek() { return in->peek(); }
    bool more() const { return fi < ir->nf(); }
    int64_t geti()
    {
        if (fi >= ir->nf())
            err(std::string("missing field in '") + rec_names[ir->name] + "' record");
        return parse_int(*ir, fi++);
    }
    int64_t geti(int64_t lo, int64_t hi)
    {
        int64_t v = geti();
        if (v < lo || v > hi)
            err(std::string("field ") + std::to_string(fi) + " of '" + rec_names[ir->name] +
                "' out of range: " + std::to_string(v));
        return v;
    }
    /* string field; "-" is the empty string */
    std::string gets()
    {
        if (fi >= ir->nf())
            err(std::string("missing field in '") + rec_names[ir->name] + "' record");
        std::string s = ir->text.substr(ir->fs[fi], ir->fe[fi] - ir->fs[fi]);
        fi++;
        if (s == "-")
            s.clear();
        return s;
    }
    /* hex string field -> nibbles */
    std::vector<uint8_t> getnibbles()
    {
        std::string s = gets();
        std::vector<uint8_t> v(s.size());
        for (size_t k = 0; k < s.size(); k++) {
            int d = hexval((unsigned char)s[k]);
            if (d < 0)
                err("bad hex digit");
            v[k] = (uint8_t)d;
        }
        return v;
    }
    std::vector<uint8_t> getbytes()
    {
        std::vector<uint8_t> n = getnibbles();
        if (n.size() & 1)
            err("odd number of hex digits");
        std::vector<uint8_t> b(n.size() / 2);
        for (size_t k = 0; k < b.size(); k++)
            b[k] = (uint8_t)(n[2 * k] << 4 | n[2 * k + 1]);
        return b;
    }
    /* write the first nbits of a nibble string */
    void put_nibbles(const std::vector<uint8_t> &n, size_t nbits)
    {
        if (n.size() != (nbits + 3) / 4)
            err("hex string length does not match its bit count");
        for (size_t k = 0; k < n.size(); k++) {
            int take = nbits - 4 * k >= 4 ? 4 : (int)(nbits - 4 * k);
            if ((n[k] & ((1 << (4 - take)) - 1)))
                err("nonzero bits past the bit count");
            w->put(n[k] >> (4 - take), take);
        }
    }

    /* ---------- sub-writers ---------- */

    void push_sub()
    {
        if (wdepth == wstack.size())
            wstack.push_back(new BitWriter);
        BitWriter *b = wstack[wdepth++];
        b->clear();
        w = b;
    }
    BitWriter &pop_sub()
    {
        BitWriter *b = wstack[--wdepth];
        w = wdepth ? wstack[wdepth - 1] : &fw;
        return *b;
    }

    size_t bitpos() const { return R ? br.pos : w->pos; }

    /* ---------- fields ---------- */

    /* fixed width unsigned field */
    unsigned u(int n)
    {
        if (R) {
            unsigned v = br.get(n);
            cr->i(v);
            return v;
        }
        int64_t v = geti(0, ((int64_t)1 << n) - 1);
        w->put((uint32_t)v, n);
        return (unsigned)v;
    }
    /* Huffman coded value */
    int hv(const Huff &h)
    {
        if (R) {
            int v = h.decode_value(br);
            cr->i(v);
            return v;
        }
        int64_t v = geti(-100000, 100000);
        if (!h.can_encode_value((int)v))
            err("value " + std::to_string(v) + " has no codeword");
        h.encode_value(*w, (int)v);
        return (int)v;
    }
    /* bit vector of n bits, as a binary string field */
    void bv(int n, uint8_t *dst = nullptr)
    {
        uint8_t tmp[1024];
        if (n > 1024)
            bad("bit vector too long");
        if (R) {
            for (int k = 0; k < n; k++)
                tmp[k] = (uint8_t)br.get1();
            cr->bits(tmp, n);
        } else {
            std::string s = gets();
            if ((int)s.size() != n)
                err("bit string of " + std::to_string(s.size()) + " bits, " + std::to_string(n) + " expected");
            for (int k = 0; k < n; k++) {
                if (s[k] != '0' && s[k] != '1')
                    err("bad bit string");
                tmp[k] = (uint8_t)(s[k] - '0');
                w->put(tmp[k], 1);
            }
        }
        if (dst)
            memcpy(dst, tmp, n);
    }
    /* presence bit of an optional record */
    bool flag(int name)
    {
        if (R)
            return br.get1() != 0;
        bool f = peek() == name;
        w->put(f, 1);
        return f;
    }

    /* ---------- individual_channel_stream ---------- */

    void ltp_data(int max_sfb)
    {
        if (u(1)) {   /* ltp_data_present */
            u(11);    /* ltp_lag */
            u(3);     /* ltp_coef */
            bv(max_sfb < 40 ? max_sfb : 40);   /* ltp_long_used */
        }
    }

    void ics_info(Ics &ii, bool common_window)
    {
        u(1);              /* ics_reserved_bit */
        ii.ws = u(2);      /* window_sequence */
        u(1);              /* window_shape */
        ii.num_groups = 1;
        ii.group_len[0] = 1;
        if (ii.ws == EIGHT_SHORT_SEQUENCE) {
            ii.max_sfb = u(4);
            unsigned grouping = u(7);
            for (int i = 6; i >= 0; i--) {
                if (grouping >> i & 1) {
                    ii.group_len[ii.num_groups - 1]++;
                } else {
                    ii.num_groups++;
                    ii.group_len[ii.num_groups - 1] = 1;
                }
            }
            ii.num_windows = 8;
            ii.swb = st.frame960 ? ff_swb_offset_120[st.sf_index] : ff_swb_offset_128[st.sf_index];
            ii.num_swb = ff_aac_num_swb_128[st.sf_index];
        } else {
            ii.max_sfb = u(6);
            ii.num_windows = 1;
            ii.swb = st.frame960 ? ff_swb_offset_960[st.sf_index] : ff_swb_offset_1024[st.sf_index];
            ii.num_swb = st.frame960 ? ff_aac_num_swb_960[st.sf_index] : ff_aac_num_swb_1024[st.sf_index];
            if (u(1)) {   /* predictor_data_present */
                if (st.aot == 1) {
                    if (u(1))   /* predictor_reset */
                        u(5);   /* predictor_reset_group_number */
                    int n = ii.max_sfb < ff_aac_pred_sfb_max[st.sf_index] ? ii.max_sfb : ff_aac_pred_sfb_max[st.sf_index];
                    bv(n);      /* prediction_used */
                } else {
                    ltp_data(ii.max_sfb);
                    if (common_window)
                        ltp_data(ii.max_sfb);
                }
            }
        }
        if (ii.max_sfb > ii.num_swb)
            bad("max_sfb exceeds the number of scalefactor bands");
    }

    void section_data(const Ics &ii, uint8_t *bt)
    {
        if (!ii.max_sfb)
            return;
        const int bits = ii.ws == EIGHT_SHORT_SEQUENCE ? 3 : 5, esc = (1 << bits) - 1;
        for (int g = 0; g < ii.num_groups; g++) {
            rb(R_SEC);
            int k = 0;
            while (k < ii.max_sfb) {
                int cb, len;
                if (R) {
                    int incr;
                    cb = br.get(4);
                    if (cb == 12)
                        parse_fail("reserved codebook 12");
                    len = 0;
                    do {
                        incr = br.get(bits);
                        len += incr;
                    } while (incr == esc);
                    if (k + len > ii.max_sfb)
                        parse_fail("section beyond max_sfb");
                    cr->i(cb);
                    cr->i(len);
                } else {
                    cb = (int)geti(0, 15);
                    if (cb == 12)
                        err("reserved codebook 12");
                    len = (int)geti(0, ii.max_sfb - k);
                    w->put(cb, 4);
                    int l = len;
                    while (l >= esc) {
                        w->put(esc, bits);
                        l -= esc;
                    }
                    w->put(l, bits);
                }
                for (int s = k; s < k + len; s++)
                    bt[g * ii.max_sfb + s] = (uint8_t)cb;
                k += len;
            }
        }
    }

    void scale_factor_data(const Ics &ii, const uint8_t *bt)
    {
        if (!ii.max_sfb)
            return;
        bool noise_pcm = true;
        for (int g = 0; g < ii.num_groups; g++) {
            bool any = false;
            for (int sfb = 0; sfb < ii.max_sfb; sfb++)
                any |= bt[g * ii.max_sfb + sfb] != 0;
            if (!any)
                continue;
            rb(R_SF);
            for (int sfb = 0; sfb < ii.max_sfb; sfb++) {
                int cb = bt[g * ii.max_sfb + sfb];
                if (!cb)
                    continue;
                if (cb == 13 && noise_pcm) {
                    noise_pcm = false;
                    u(9);              /* dpcm_noise_nrg of the first noise band */
                } else {
                    hv(huff_sf);       /* scalefactor / intensity / noise delta */
                }
            }
        }
    }

    void pulse_data()
    {
        rb(R_PULSE);
        int np = u(2);   /* number_pulse */
        u(6);            /* pulse_start_sfb */
        for (int i = 0; i <= np; i++) {
            u(5);        /* pulse_offset */
            u(4);        /* pulse_amp */
        }
    }

    void tns_data(const Ics &ii)
    {
        const bool is8 = ii.ws == EIGHT_SHORT_SEQUENCE;
        for (int wi = 0; wi < ii.num_windows; wi++) {
            rb(R_TNS);
            int nf = u(is8 ? 1 : 2);
            if (!nf)
                continue;
            int coef_res = u(1);
            for (int f = 0; f < nf; f++) {
                u(is8 ? 4 : 6);              /* length */
                int order = u(is8 ? 3 : 5);
                if (!order)
                    continue;
                u(1);                        /* direction */
                int compress = u(1);
                int clen = coef_res + 3 - compress;
                for (int i = 0; i < order; i++)
                    u(clen);
            }
        }
    }

    void gain_control_data(const Ics &ii)
    {
        static const uint8_t gain_mode[4][3] = { { 1, 0, 5 }, { 2, 1, 2 }, { 8, 0, 2 }, { 2, 1, 5 } };
        rb(R_GC);
        const uint8_t *m = gain_mode[ii.ws];
        int max_band = u(2);
        for (int bd = 0; bd < max_band; bd++)
            for (int wd = 0; wd < m[0]; wd++) {
                int an = u(3);
                for (int ad = 0; ad < an; ad++) {
                    u(4);                                   /* alevcode */
                    u((wd == 0 && m[1]) ? 4 : m[2]);        /* aloccode */
                }
            }
    }

    void dec_tuple(int cb, int *c)
    {
        const SpecCb &p = spec_cb[cb - 1];
        int idx = huff_spec[cb - 1].decode(br);
        int v[4];
        if (p.dim == 4) {
            v[0] = idx / 27;
            v[1] = idx / 9 % 3;
            v[2] = idx / 3 % 3;
            v[3] = idx % 3;
        } else {
            v[0] = idx / p.range;
            v[1] = idx % p.range;
        }
        if (p.is_signed) {
            for (int j = 0; j < p.dim; j++)
                c[j] = v[j] - p.off;
            return;
        }
        int sgn[4];
        for (int j = 0; j < p.dim; j++)
            sgn[j] = v[j] ? (int)br.get1() : 0;
        if (cb == 11)
            for (int j = 0; j < 2; j++)
                if (v[j] == 16) {
                    int n = 0;
                    while (br.get1())
                        if (++n > 8)
                            parse_fail("escape sequence too long");
                    v[j] = (1 << (n + 4)) + (int)br.get(n + 4);
                }
        for (int j = 0; j < p.dim; j++)
            c[j] = sgn[j] ? -v[j] : v[j];
    }

    void enc_tuple(int cb, const int *c)
    {
        const SpecCb &p = spec_cb[cb - 1];
        int idx = 0;
        for (int j = 0; j < p.dim; j++) {
            int x = c[j], m = x < 0 ? -x : x, a;
            if (p.is_signed) {
                if (m > p.maxval)
                    err("spectral value " + std::to_string(x) + " out of range of codebook " + std::to_string(cb));
                a = x + p.off;
            } else if (cb == 11) {
                if (m > 8191)
                    err("spectral value " + std::to_string(x) + " out of range");
                a = m >= 16 ? 16 : m;
            } else {
                if (m > p.maxval)
                    err("spectral value " + std::to_string(x) + " out of range of codebook " + std::to_string(cb));
                a = m;
            }
            idx = idx * p.range + a;
        }
        huff_spec[cb - 1].encode_value(*w, idx);
        if (p.is_signed)
            return;
        for (int j = 0; j < p.dim; j++)
            if (c[j])
                w->put(c[j] < 0, 1);
        if (cb == 11)
            for (int j = 0; j < 2; j++) {
                int m = c[j] < 0 ? -c[j] : c[j];
                if (m < 16)
                    continue;
                int n = 0;
                while (m >= (1 << (n + 5)))
                    n++;
                w->put((1u << (n + 1)) - 2, n + 1);   /* n ones and a zero */
                w->put(m - (1 << (n + 4)), n + 4);
            }
    }

    void spectral_data(const Ics &ii, const uint8_t *bt)
    {
        bool any = false;
        for (int k = 0; k < ii.num_groups * ii.max_sfb; k++)
            any |= bt[k] && bt[k] < 13;
        if (!any)
            return;
        static thread_local int coef[1024];
        const int width = ii.swb[ii.max_sfb];
        if (R) {
            memset(coef, 0, sizeof(coef));
        } else {
            for (int wi = 0; wi < ii.num_windows; wi++) {
                rb(R_SPEC);
                for (int k = 0; k < width; k++)
                    coef[wi * 128 + k] = (int)geti(-8191, 8191);
            }
        }
        int win = 0;
        for (int g = 0; g < ii.num_groups; g++) {
            for (int sfb = 0; sfb < ii.max_sfb; sfb++) {
                int cb = bt[g * ii.max_sfb + sfb];
                int a = ii.swb[sfb], b = ii.swb[sfb + 1];
                if (!cb || cb >= 13) {
                    if (!R)
                        for (int wi = 0; wi < ii.group_len[g]; wi++)
                            for (int k = a; k < b; k++)
                                if (coef[(win + wi) * 128 + k])
                                    err("nonzero spectral value in a band without spectral data");
                    continue;
                }
                const int dim = spec_cb[cb - 1].dim;
                for (int wi = 0; wi < ii.group_len[g]; wi++) {
                    int *c = coef + (win + wi) * 128;
                    for (int k = a; k < b; k += dim) {
                        if (R)
                            dec_tuple(cb, c + k);
                        else
                            enc_tuple(cb, c + k);
                    }
                }
            }
            win += ii.group_len[g];
        }
        if (R)
            for (int wi = 0; wi < ii.num_windows; wi++) {
                rb(R_SPEC);
                for (int k = 0; k < width; k++)
                    cr->i(coef[wi * 128 + k]);
            }
    }

    /* individual_channel_stream(common_window, 0) */
    void ics(Ics &ii, bool common_window, uint8_t *bt)
    {
        rb(R_ICS);
        u(8);   /* global_gain */
        if (!common_window)
            ics_info(ii, false);
        section_data(ii, bt);
        scale_factor_data(ii, bt);
        if (flag(R_PULSE))
            pulse_data();
        if (flag(R_TNS))
            tns_data(ii);
        if (flag(R_GC))
            gain_control_data(ii);
        spectral_data(ii, bt);
    }

    /* ---------- channel elements ---------- */

    void sce_lfe(int name)
    {
        uint8_t bt[8 * 64];
        Ics ii;
        rb(name);
        size_t s = bitpos();
        int tag = u(4);
        ics(ii, false, bt);
        crcr.push_back({ s, bitpos(), 192 });
        prev_che_type = name == R_SCE ? TYPE_SCE : TYPE_LFE;
        prev_che_tag = tag;
    }

    void cpe()
    {
        uint8_t bt0[8 * 64], bt1[8 * 64];
        Ics ii0, ii1;
        rb(R_CPE);
        size_t s = bitpos();
        int tag = u(4);
        int cw = u(1);   /* common_window */
        if (cw) {
            ics_info(ii0, true);
            rb(R_MS);
            int msp = u(2);   /* ms_mask_present */
            if (msp == 1)
                bv(ii0.num_groups * ii0.max_sfb);
        }
        ics(ii0, cw, bt0);
        size_t s2 = bitpos();
        if (cw)
            ics(ii0, true, bt1);
        else
            ics(ii1, false, bt1);
        size_t e = bitpos();
        crcr.push_back({ s, e, 192 });
        crcr.push_back({ s2, e, 128 });
        prev_che_type = TYPE_CPE;
        prev_che_tag = tag;
    }

    void cce()
    {
        uint8_t bt[8 * 64];
        Ics ii;
        rb(R_CCE);
        size_t s = bitpos();
        int tag = u(4);
        int ind_sw = u(1);   /* ind_sw_cce_flag */
        int nc = u(3);       /* num_coupled_elements */
        int num_gain = 0;
        for (int c = 0; c <= nc; c++) {
            num_gain++;
            int is_cpe = u(1);   /* cc_target_is_cpe */
            u(4);                /* cc_target_tag_select */
            if (is_cpe && u(2) == 3)   /* cc_l, cc_r */
                num_gain++;
        }
        u(1);   /* cc_domain */
        u(1);   /* gain_element_sign */
        u(2);   /* gain_element_scale */
        ics(ii, false, bt);
        for (int c = 1; c < num_gain; c++) {
            rb(R_CGAIN);
            int cge = ind_sw ? 1 : u(1);   /* common_gain_element_present */
            if (cge) {
                hv(huff_sf);
            } else {
                for (int g = 0; g < ii.num_groups; g++)
                    for (int sfb = 0; sfb < ii.max_sfb; sfb++)
                        if (bt[g * ii.max_sfb + sfb])
                            hv(huff_sf);
            }
        }
        crcr.push_back({ s, bitpos(), 192 });
        prev_che_type = TYPE_CCE;
        prev_che_tag = tag;
    }

    /* ---------- DSE, PCE ---------- */

    void dse(size_t blk_start)
    {
        rb(R_DSE);
        size_t s = bitpos();
        u(4);   /* element_instance_tag */
        if (R) {
            int al = br.get1();
            int cnt = br.get(8);
            if (cnt == 255)
                cnt += br.get(8);
            unsigned ab = 0;
            if (al)
                ab = br.get((int)((blk_start - br.pos) & 7));
            if ((size_t)cnt * 8 > br.left())
                parse_fail("data stream element overrun");
            cr->i(al);
            cr->hex(buf, br.pos, (size_t)cnt * 8);
            br.skip((size_t)cnt * 8);
            if (ab)
                cr->i(ab);
        } else {
            int al = (int)geti(0, 1);
            std::vector<uint8_t> d = getbytes();
            int64_t ab = more() ? geti(0, 127) : 0;
            size_t cnt = d.size();
            if (cnt > 510)
                err("data stream element longer than 510 bytes");
            w->put(al, 1);
            if (cnt < 255) {
                w->put((uint32_t)cnt, 8);
            } else {
                w->put(255, 8);
                w->put((uint32_t)(cnt - 255), 8);
            }
            int nb = al ? (int)((blk_start - w->pos) & 7) : 0;
            if (ab >> nb)
                err("alignment bits out of range");
            w->put((uint32_t)ab, nb);
            for (uint8_t c : d)
                w->put(c, 8);
        }
        crcr.push_back({ s, bitpos(), 0 });
    }

    /* program_config_element() fields into the current record; byte_alignment()
     * is relative to align_ref; returns the number of channels it declares */
    int pce_body(size_t align_ref, int *object_type = nullptr, int *sf_index = nullptr)
    {
        int ch = 0;
        u(4);                 /* element_instance_tag */
        int ot = u(2);        /* object_type */
        int sfi = u(4);       /* sampling_frequency_index */
        if (object_type)
            *object_type = ot;
        if (sf_index)
            *sf_index = sfi;
        int nf = u(4), ns = u(4), nb = u(4), nl = u(2), na = u(3), nc = u(4);
        if (u(1))   /* mono_mixdown_present */
            u(4);
        if (u(1))   /* stereo_mixdown_present */
            u(4);
        if (u(1)) { /* matrix_mixdown_idx_present */
            u(2);   /* matrix_mixdown_idx */
            u(1);   /* pseudo_surround_enable */
        }
        for (int i = 0; i < nf + ns + nb; i++) {
            ch += 1 + u(1);   /* *_element_is_cpe */
            u(4);             /* *_element_tag_select */
        }
        ch += nl;
        for (int i = 0; i < nl + na; i++)
            u(4);   /* lfe / assoc_data element tag_select */
        for (int i = 0; i < nc; i++) {
            u(1);   /* cc_element_is_ind_sw */
            u(4);   /* valid_cc_element_tag_select */
        }
        if (R) {
            unsigned ab = br.get((int)((align_ref - br.pos) & 7));
            int clen = br.get(8);
            if ((size_t)clen * 8 > br.left())
                parse_fail("program config element overrun");
            cr->hex(buf, br.pos, (size_t)clen * 8);
            br.skip((size_t)clen * 8);
            if (ab)
                cr->i(ab);
        } else {
            std::vector<uint8_t> d = getbytes();
            int64_t ab = more() ? geti(0, 127) : 0;
            int nbits = (int)((align_ref - w->pos) & 7);
            if (ab >> nbits)
                err("alignment bits out of range");
            if (d.size() > 255)
                err("comment longer than 255 bytes");
            w->put((uint32_t)ab, nbits);
            w->put((uint32_t)d.size(), 8);
            for (uint8_t c : d)
                w->put(c, 8);
        }
        return ch;
    }

    void pce(size_t blk_start)
    {
        rb(R_PCE);
        size_t s = bitpos();
        pce_body(blk_start);
        crcr.push_back({ s, bitpos(), 0 });
    }

    /* ---------- FIL: extension payloads ---------- */

    static bool is_payload(int name)
    {
        return name == R_FILL || name == R_EXT || name == R_DRC || name == R_SBR || name == R_SBRC;
    }

    /* dynamic_range_info() after the extension type; returns the byte count
     * the decoder subtracts from the fill element */
    int drc()
    {
        rb(R_DRC);
        int n = 1, nb = 1;
        if (u(1)) {        /* pce_tag_present */
            u(4);          /* pce_instance_tag */
            u(4);          /* drc_tag_reserved_bits */
            n++;
        }
        if (u(1)) {        /* excluded_chns_present */
            int ne = 0;
            for (;;) {
                u(7);      /* exclude_mask */
                ne += 7;
                if (ne >= 57 || !u(1))   /* additional_excluded_chns (as aacdec) */
                    break;
            }
            n += ne / 7;
        }
        if (u(1)) {        /* drc_bands_present */
            int incr = u(4);   /* drc_band_incr */
            u(4);              /* drc_interpolation_scheme */
            n++;
            nb += incr;
            for (int i = 0; i < nb; i++) {
                u(8);          /* drc_band_top */
                n++;
            }
        }
        if (u(1)) {        /* prog_ref_level_present */
            u(7);          /* prog_ref_level */
            u(1);          /* prog_ref_level_reserved_bits */
            n++;
        }
        for (int i = 0; i < nb; i++) {
            u(1);          /* dyn_rng_sgn */
            u(7);          /* dyn_rng_ctl */
            n++;
        }
        return n;
    }

    /* R mode: raw extension payload of type t: the remaining 8*cnt-4 bits */
    void ext_raw(int t, size_t start, size_t pend)
    {
        rb(R_EXT);
        cr->i(t);
        cr->hex(buf, start, pend - start);
        br.pos = pend;
    }

    /* R mode: one extension_payload of a fill element with cnt bytes left
     * (aacdec decode_extension_payload()); returns the bytes consumed */
    int payload_r(int cnt)
    {
        size_t ps = br.pos, pend = ps + 8 * (size_t)cnt;
        size_t save_end = br.end;
        int t = br.get(4);
        size_t start = br.pos;
        if (t == 11) {
            size_t mark = out->n;
            br.end = pend;
            try {
                int n = drc();
                br.end = save_end;
                if (n > cnt)
                    parse_fail("dynamic_range_info longer than its fill element");
                return n;
            } catch (ParseError &) {
                out->n = mark;
                br.pos = start;
                br.end = save_end;
                ext_raw(t, start, pend);
                return cnt;
            }
        }
        if (t == 13 || t == 14) {
            sbr_payload_r(t, start, pend);
            return cnt;
        }
        /* padding: a zero nibble and cnt-1 equal bytes, as EXT_FILL_DATA
         * (fill_nibble 0000, fill_byte 10100101) or zeros */
        bool uniform = br.peek(4) == 0;
        unsigned byte = t == 1 || cnt == 1 ? 0xa5 : (BitReader(buf, start + 4, pend).get(8));
        for (size_t p = start + 4; uniform && p < pend; p += 8)
            uniform = BitReader(buf, p, pend).get(8) == byte;
        if (uniform) {
            rb(R_FILL);
            cr->i(cnt);
            if (t != 1 || byte != 0xa5) {
                cr->i(t);
                cr->i(byte);
            }
            br.pos = pend;
            return cnt;
        }
        ext_raw(t, start, pend);
        return cnt;
    }

    /* W mode: one extension payload; returns its byte count, *all is set
     * when the payload takes the rest of the fill element */
    int payload_w(bool *all)
    {
        int pk = peek();
        *all = true;
        switch (pk) {
        case R_DRC: {
            w->put(11, 4);
            *all = false;
            return drc();
        }
        case R_SBR:
        case R_SBRC:
            return sbr_payload_w(pk);
        case R_FILL: {
            rb(R_FILL);
            int n = (int)geti(1, 269), t = 1, byte = 0xa5;
            if (more()) {
                t = (int)geti(0, 15);
                byte = (int)geti(0, 255);
                if (t == 11 || t == 13 || t == 14 || (t == 1 && byte == 0xa5) || (n == 1 && byte != 0xa5))
                    err("non-canonical fill record");
            }
            w->put(t, 4);
            w->put(0, 4);
            for (int k = 1; k < n; k++)
                w->put(byte, 8);
            return n;
        }
        default: { /* R_EXT */
            rb(R_EXT);
            int t = (int)geti(0, 15);
            std::vector<uint8_t> nib = getnibbles();
            if (!(nib.size() & 1))
                err("extension payload must have an odd number of hex digits");
            w->put(t, 4);
            put_nibbles(nib, nib.size() * 4);
            return (int)(nib.size() + 1) / 2;
        }
        }
    }

    void fil()
    {
        rb(R_FIL);
        if (R) {
            int count = br.get(4), cnt = count;
            if (count == 15) {
                cnt = 15 + br.get(8) - 1;
                if (cnt < 15)
                    cr->i(1);   /* 14 written with the escape */
            }
            if ((size_t)cnt * 8 > br.left())
                parse_fail("fill element overrun");
            while (cnt > 0)
                cnt -= payload_r(cnt);
        } else {
            int escf = more() ? (int)geti(1, 1) : 0;
            push_sub();
            int total = 0;
            bool all = false;
            while (!all && is_payload(peek()))
                total += payload_w(&all);
            BitWriter &sub = pop_sub();
            if (escf) {
                if (total != 14)
                    err("escape flag of a fill element is only valid for 14 bytes");
                w->put(15, 4);
                w->put(0, 8);
            } else if (total < 15) {
                w->put(total, 4);
            } else {
                if (total > 269)
                    err("fill element longer than 269 bytes");
                w->put(15, 4);
                w->put(total - 14, 8);
            }
            w->append(sub);
        }
    }

    /* ---------- SBR ---------- */

    SbrState *sbr_state()
    {
        int k = prev_che_type == TYPE_SCE ? 0 : prev_che_type == TYPE_CPE ? 1 : prev_che_type == TYPE_CCE ? 2 : -1;
        return k < 0 ? nullptr : &st.sbr[k][prev_che_tag];
    }

    void sbr_header(SbrState &s, SbrSpectrum &sp)
    {
        rb(R_SBRH);
        s.amp_res = u(1);       /* bs_amp_res */
        sp.start_freq = u(4);
        sp.stop_freq = u(4);
        sp.xover_band = u(3);
        u(2);                   /* bs_reserved */
        int e1 = u(1), e2 = u(1);   /* bs_header_extra_1, _2 */
        if (e1) {
            sp.freq_scale = u(2);
            sp.alter_scale = u(1);
            sp.noise_bands = u(2);
        } else {
            sp.freq_scale = 2;
            sp.alter_scale = 1;
            sp.noise_bands = 2;
        }
        if (e2) {
            u(2);   /* bs_limiter_bands */
            u(2);   /* bs_limiter_gains */
            u(1);   /* bs_interpol_freq */
            u(1);   /* bs_smoothing_mode */
        }
    }

    /* band tables after an optional header (aacdec read_sbr_header() /
     * sbr_reset(): recomputed when the spectrum parameters or the rate change) */
    void sbr_tables(SbrState &s, bool hdr, const SbrSpectrum &sp)
    {
        if (hdr) {
            if (!s.have_hdr || sp != s.sp || s.rate != st.sbr_rate) {
                s.sp = sp;
                s.rate = st.sbr_rate;
                s.ok = sbr_compute_bands(st.sbr_rate, sp, s.bands);
            }
            s.have_hdr = 1;
        } else if (s.have_hdr && s.rate != st.sbr_rate) {
            s.rate = st.sbr_rate;
            s.ok = sbr_compute_bands(st.sbr_rate, s.sp, s.bands);
        }
        if (!s.have_hdr || !s.ok)
            bad("SBR data without a valid SBR header");
    }

    void sbr_grid(SbrState &s, SbrCh &c)
    {
        rb(R_SGRID);
        int fc = u(2);   /* bs_frame_class */
        int ne = 1;
        c.amp_res = s.amp_res;
        switch (fc) {
        case 0: { /* FIXFIX */
            ne = 1 << u(2);
            if (ne > 5)
                bad("too many SBR envelopes");
            int fr = u(1);
            for (int e = 0; e < ne; e++)
                c.freq_res[e] = (uint8_t)fr;
            if (ne == 1)
                c.amp_res = 0;
            break;
        }
        case 1: { /* FIXVAR */
            uint8_t fr[8];
            u(2);                    /* bs_abs_bord */
            int nrt = u(2);          /* bs_num_rel */
            ne = nrt + 1;
            for (int i = 0; i < nrt; i++)
                u(2);                /* bs_rel_bord */
            u(sbr_ceil_log2[ne]);    /* bs_pointer */
            bv(ne, fr);              /* bs_freq_res, last envelope first */
            for (int i = 0; i < ne; i++)
                c.freq_res[ne - 1 - i] = fr[i];
            break;
        }
        case 2: { /* VARFIX */
            u(2);
            int nrl = u(2);
            ne = nrl + 1;
            for (int i = 0; i < nrl; i++)
                u(2);
            u(sbr_ceil_log2[ne]);
            bv(ne, c.freq_res);
            break;
        }
        default: { /* VARVAR */
            u(2);                    /* bs_abs_bord_0 */
            u(2);                    /* bs_abs_bord_1 */
            int nrl = u(2), nrt = u(2);
            ne = nrl + nrt + 1;
            if (ne > 5)
                bad("too many SBR envelopes");
            for (int i = 0; i < nrl + nrt; i++)
                u(2);                /* bs_rel_bord_0, _1 */
            u(sbr_ceil_log2[ne]);
            bv(ne, c.freq_res);
            break;
        }
        }
        c.num_env = ne;
        c.num_noise = ne > 1 ? 2 : 1;
    }

    void sbr_dtdf(SbrCh &c)
    {
        rb(R_SDTDF);
        bv(c.num_env, c.df_env);
        bv(c.num_noise, c.df_noise);
    }

    void sbr_invf(const SbrState &s)
    {
        rb(R_SINVF);
        for (int i = 0; i < s.bands.n_q; i++)
            u(2);   /* bs_invf_mode */
    }

    void sbr_envelope(const SbrState &s, const SbrCh &c, bool bal)
    {
        const Huff *th, *fh;
        int startbits;
        if (bal) {
            th = &huff_sbr[c.amp_res ? T_HUFFMAN_ENV_BAL_3_0DB : T_HUFFMAN_ENV_BAL_1_5DB];
            fh = &huff_sbr[c.amp_res ? F_HUFFMAN_ENV_BAL_3_0DB : F_HUFFMAN_ENV_BAL_1_5DB];
            startbits = c.amp_res ? 5 : 6;
        } else {
            th = &huff_sbr[c.amp_res ? T_HUFFMAN_ENV_3_0DB : T_HUFFMAN_ENV_1_5DB];
            fh = &huff_sbr[c.amp_res ? F_HUFFMAN_ENV_3_0DB : F_HUFFMAN_ENV_1_5DB];
            startbits = c.amp_res ? 6 : 7;
        }
        for (int e = 0; e < c.num_env; e++) {
            rb(R_SENV);
            int nb = s.bands.n[c.freq_res[e]];
            if (c.df_env[e]) {
                for (int j = 0; j < nb; j++)
                    hv(*th);
            } else {
                u(startbits);   /* bs_env_start_value */
                for (int j = 1; j < nb; j++)
                    hv(*fh);
            }
        }
    }

    void sbr_noise(const SbrState &s, const SbrCh &c, bool bal)
    {
        const Huff &th = huff_sbr[bal ? T_HUFFMAN_NOISE_BAL_3_0DB : T_HUFFMAN_NOISE_3_0DB];
        const Huff &fh = huff_sbr[bal ? F_HUFFMAN_ENV_BAL_3_0DB : F_HUFFMAN_ENV_3_0DB];
        for (int i = 0; i < c.num_noise; i++) {
            rb(R_SNOISE);
            if (c.df_noise[i]) {
                for (int j = 0; j < s.bands.n_q; j++)
                    hv(th);
            } else {
                u(5);   /* bs_noise_start_value */
                for (int j = 1; j < s.bands.n_q; j++)
                    hv(fh);
            }
        }
    }

    void sbr_harmonic(const SbrState &s)
    {
        rb(R_SHARM);
        if (u(1))   /* bs_add_harmonic_flag */
            bv(s.bands.n[1]);
    }

    /* extension size field + payload written to a sub-writer; R mode reads
     * the size and returns it in bits */
    void write_ext_size(int cnt)
    {
        if (cnt < 15) {
            w->put(cnt, 4);
        } else {
            if (cnt > 270)
                err("extension longer than 270 bytes");
            w->put(15, 4);
            w->put(cnt - 15, 8);
        }
    }
    int64_t read_ext_size()
    {
        int cnt = br.get(4);
        if (cnt == 15)
            cnt += br.get(8);
        return 8 * (int64_t)cnt;
    }

    void ps_ipdopd(PsHdr &h, int ne)
    {
        if (R) {
            size_t ri = out->n;
            rb(R_PSIPD);
            int en = u(1);   /* enable_ipdopd */
            if (en)
                for (int e = 0; e < ne; e++) {
                    rb(R_PSPD);
                    int dt = u(1);
                    for (int b = 0; b < h.nr_ipdopd_par; b++)
                        hv(huff_ps[dt ? PS_IPD_DT : PS_IPD_DF]);
                    dt = u(1);
                    for (int b = 0; b < h.nr_ipdopd_par; b++)
                        hv(huff_ps[dt ? PS_OPD_DT : PS_OPD_DF]);
                }
            out->recs[ri].i(br.get1());   /* reserved_ps */
        } else {
            rb(R_PSIPD);
            int en = (int)geti(0, 1), res = (int)geti(0, 1);
            w->put(en, 1);
            if (en)
                for (int e = 0; e < ne; e++) {
                    rb(R_PSPD);
                    int dt = u(1);
                    for (int b = 0; b < h.nr_ipdopd_par; b++)
                        hv(huff_ps[dt ? PS_IPD_DT : PS_IPD_DF]);
                    dt = u(1);
                    for (int b = 0; b < h.nr_ipdopd_par; b++)
                        hv(huff_ps[dt ? PS_OPD_DT : PS_OPD_DF]);
                }
            w->put(res, 1);
        }
    }

    /* ps_data() (aacdec ff_ps_read_data()) */
    void ps_data(PsHdr &h)
    {
        rb(R_PS);
        if (u(1)) {   /* enable_ps_header */
            h.enable_iid = u(1);
            if (h.enable_iid) {
                int m = u(3);   /* iid_mode */
                if (m > 5)
                    bad("reserved iid_mode");
                h.nr_iid_par = nr_iidicc_par_tab[m];
                h.iid_quant = m > 2;
                h.nr_ipdopd_par = nr_iidopd_par_tab[m];
            }
            h.enable_icc = u(1);
            if (h.enable_icc) {
                int m = u(3);   /* icc_mode */
                if (m > 5)
                    bad("reserved icc_mode");
                h.nr_icc_par = nr_iidicc_par_tab[m];
            }
            h.enable_ext = u(1);
        }
        int fc = u(1);                       /* frame_class */
        int ne = num_env_tab[fc][u(2)];      /* num_env_idx */
        if (fc)
            for (int e = 0; e < ne; e++)
                u(5);                        /* border_position */
        if (h.enable_iid)
            for (int e = 0; e < ne; e++) {
                rb(R_PSIID);
                int dt = u(1);
                const Huff &t = huff_ps[dt ? (h.iid_quant ? PS_IID_DT1 : PS_IID_DT0)
                                           : (h.iid_quant ? PS_IID_DF1 : PS_IID_DF0)];
                for (int b = 0; b < h.nr_iid_par; b++)
                    hv(t);
            }
        if (h.enable_icc)
            for (int e = 0; e < ne; e++) {
                rb(R_PSICC);
                int dt = u(1);
                for (int b = 0; b < h.nr_icc_par; b++)
                    hv(huff_ps[dt ? PS_ICC_DT : PS_ICC_DF]);
            }
        if (!h.enable_ext)
            return;
        if (R) {
            size_t xi = out->n;
            rb(R_PSX);
            int64_t left = read_ext_size();
            if ((size_t)left > br.left())
                parse_fail("PS extension overrun");
            while (left > 7) {
                int id = br.get(2);   /* ps_extension_id */
                left -= 2;
                if (id == 0) {
                    size_t p0 = br.pos;
                    ps_ipdopd(h, ne);
                    left -= (int64_t)(br.pos - p0);
                } else {
                    rb(R_PSXE);
                    cr->i(id);
                }
            }
            if (left < 0)
                parse_fail("PS extension overflow");
            unsigned fb = br.get((int)left);
            if (fb)
                out->recs[xi].i(fb);
        } else {
            rb(R_PSX);
            int64_t fb = more() ? geti(0, 127) : 0;
            push_sub();
            for (;;) {
                int pk = peek();
                if (pk == R_PSIPD) {
                    w->put(0, 2);
                    ps_ipdopd(h, ne);
                } else if (pk == R_PSXE) {
                    rb(R_PSXE);
                    w->put((uint32_t)geti(1, 3), 2);
                } else {
                    break;
                }
            }
            int nb = (int)((8 - w->pos % 8) % 8);
            if (fb >> nb)
                err("PS extension fill bits out of range");
            w->put((uint32_t)fb, nb);
            BitWriter &sub = pop_sub();
            write_ext_size((int)(sub.pos / 8));
            w->append(sub);
        }
    }

    void sbr_extension(SbrState &s)
    {
        if (R) {
            size_t xi = out->n;
            rb(R_SBRX);
            int64_t left = read_ext_size();
            if ((size_t)left > br.left())
                parse_fail("SBR extension overrun");
            while (left > 7) {
                int id = br.get(2);   /* bs_extension_id */
                left -= 2;
                if (id == 2) {        /* EXTENSION_ID_PS */
                    size_t p0 = br.pos, save_end = br.end;
                    br.end = br.pos + left;
                    ps_data(s.ps);
                    br.end = save_end;
                    left -= (int64_t)(br.pos - p0);
                } else {
                    rb(R_SBRXE);
                    cr->i(id);
                    cr->i(left);
                    cr->hex(buf, br.pos, (size_t)left);
                    br.skip((size_t)left);
                    left = 0;
                }
            }
            unsigned fb = br.get((int)left);
            if (fb)
                out->recs[xi].i(fb);
        } else {
            rb(R_SBRX);
            int64_t fb = more() ? geti(0, 127) : 0;
            push_sub();
            for (;;) {
                int pk = peek();
                if (pk == R_PS) {
                    w->put(2, 2);
                    ps_data(s.ps);
                } else if (pk == R_SBRXE) {
                    rb(R_SBRXE);
                    int id = (int)geti(0, 3);
                    if (id == 2)
                        err("sbrxe record with the PS extension id");
                    int64_t n = geti(0, 8 * 270);
                    std::vector<uint8_t> nib = getnibbles();
                    w->put(id, 2);
                    put_nibbles(nib, (size_t)n);
                    if (w->pos % 8)
                        err("sbrxe payload does not end the extension on a byte boundary");
                    break;
                } else {
                    break;
                }
            }
            int nb = (int)((8 - w->pos % 8) % 8);
            if (fb >> nb)
                err("SBR extension fill bits out of range");
            w->put((uint32_t)fb, nb);
            BitWriter &sub = pop_sub();
            write_ext_size((int)(sub.pos / 8));
            w->append(sub);
        }
    }

    /* sbr_data() for the element type of the preceding channel element */
    void sbr_data(SbrState &s)
    {
        SbrCh c0, c1;
        rb(R_SBRD);
        if (prev_che_type != TYPE_CPE) {
            if (u(1))   /* bs_data_extra */
                u(4);   /* bs_reserved */
            sbr_grid(s, c0);
            sbr_dtdf(c0);
            sbr_invf(s);
            sbr_envelope(s, c0, false);
            sbr_noise(s, c0, false);
            sbr_harmonic(s);
        } else {
            if (u(1))   /* bs_data_extra */
                u(8);   /* bs_reserved x2 */
            int coupling = u(1);   /* bs_coupling */
            if (coupling) {
                sbr_grid(s, c0);
                c1 = c0;
                sbr_dtdf(c0);
                sbr_dtdf(c1);
                sbr_invf(s);
                sbr_envelope(s, c0, false);
                sbr_noise(s, c0, false);
                sbr_envelope(s, c1, true);
                sbr_noise(s, c1, true);
            } else {
                sbr_grid(s, c0);
                sbr_grid(s, c1);
                sbr_dtdf(c0);
                sbr_dtdf(c1);
                sbr_invf(s);
                sbr_invf(s);
                sbr_envelope(s, c0, false);
                sbr_envelope(s, c1, false);
                sbr_noise(s, c0, false);
                sbr_noise(s, c1, false);
            }
            sbr_harmonic(s);
            sbr_harmonic(s);
        }
        if (flag(R_SBRX))   /* bs_extended_data */
            sbr_extension(s);
    }

    /* R mode: sbr_extension_data() in [start, pend) after the extension type;
     * stored raw when it does not parse */
    void sbr_payload_r(int t, size_t start, size_t pend)
    {
        SbrState *ps = sbr_state();
        if (!ps) {   /* no preceding SCE/CPE/CCE: not decodable */
            ext_raw(t, start, pend);
            return;
        }
        SbrState s = *ps;
        size_t mark = out->n, save_end = br.end;
        br.end = pend;
        try {
            size_t ri = out->n;
            rb(t == 14 ? R_SBRC : R_SBR);
            unsigned crc = t == 14 ? br.get(10) : 0;   /* bs_sbr_crc_bits */
            SbrSpectrum sp = {};
            bool hdr = br.get1();   /* bs_header_flag */
            if (hdr)
                sbr_header(s, sp);
            sbr_tables(s, hdr, sp);
            sbr_data(s);
            size_t pad = pend - br.pos;
            if (pad >= 8 || (pad && br.peek((int)pad))) {
                rb(R_SBRPAD);
                cr->i((int64_t)pad);
                cr->hex(buf, br.pos, pad);
            }
            br.pos = pend;
            if (t == 14 && crc10_bits(buf, start + 10, pend - start - 10) != crc)
                out->recs[ri].i(crc);
            br.end = save_end;
            *ps = s;
        } catch (ParseError &) {
            out->n = mark;
            br.end = save_end;
            ext_raw(t, start, pend);
        }
    }

    /* W mode: SBR payload including the extension type; returns its bytes */
    int sbr_payload_w(int name)
    {
        rb(name);
        int t = name == R_SBRC ? 14 : 13;
        int64_t xcrc = t == 14 && more() ? geti(0, 1023) : -1;
        SbrState *ps = sbr_state();
        if (!ps)
            err("SBR data not preceded by an SCE, CPE or CCE");
        SbrState s = *ps;
        push_sub();
        w->put(t, 4);
        if (t == 14)
            w->put(0, 10);
        SbrSpectrum sp = {};
        bool hdr = peek() == R_SBRH;
        w->put(hdr, 1);
        if (hdr)
            sbr_header(s, sp);
        sbr_tables(s, hdr, sp);
        sbr_data(s);
        if (peek() == R_SBRPAD) {
            rb(R_SBRPAD);
            int64_t n = geti(0, 8 * 270);
            put_nibbles(getnibbles(), (size_t)n);
        } else {
            w->put(0, (int)((8 - w->pos % 8) % 8));
        }
        if (w->pos % 8)
            err("SBR payload does not end on a byte boundary");
        if (t == 14) {
            unsigned crc = xcrc >= 0 ? (unsigned)xcrc : crc10_bits(w->b.data(), 14, w->pos - 14);
            w->patch(4, crc, 10);
        }
        BitWriter &sub = pop_sub();
        w->append(sub);
        *ps = s;
        return (int)(sub.pos / 8);
    }

    /* ---------- raw_data_block ---------- */

    /* returns true when the element was END */
    bool element(int t, size_t blk_start, bool align_end)
    {
        switch (t) {
        case TYPE_SCE: sce_lfe(R_SCE); break;
        case TYPE_LFE: sce_lfe(R_LFE); break;
        case TYPE_CPE: cpe(); break;
        case TYPE_CCE: cce(); break;
        case TYPE_DSE: dse(blk_start); break;
        case TYPE_PCE: pce(blk_start); break;
        case TYPE_FIL: fil(); break;
        default:
            rb(R_END);
            if (align_end) {   /* byte_alignment() */
                int nb = (int)((blk_start - bitpos()) & 7);
                if (R) {
                    unsigned v = br.get(nb);
                    if (v)
                        cr->i(v);
                } else {
                    int64_t v = more() ? geti(1, 127) : 0;
                    if (v >> nb)
                        err("alignment bits out of range");
                    w->put((uint32_t)v, nb);
                }
            }
            return true;
        }
        return false;
    }

    /* R mode: the rest of the frame from bit p, stored as is */
    void rest_r(size_t p, size_t end)
    {
        rb(R_REST);
        cr->i((int64_t)(end - p));
        cr->hex(buf, p, end - p);
        br.pos = end;
    }

    /* returns false when the frame was finished by a rest record */
    bool raw_data_block(size_t frame_end, bool align_end)
    {
        size_t blk_start = bitpos();
        prev_che_type = -1;
        prev_che_tag = 0;
        crcr.clear();
        for (;;) {
            if (R) {
                size_t es = br.pos, mark = out->n, cmark = crcr.size();
                int pt = prev_che_type, pg = prev_che_tag;
                try {
                    int t = br.get(3);
                    if (element(t, blk_start, align_end))
                        return true;
                } catch (ParseError &) {
                    out->n = mark;
                    crcr.resize(cmark);
                    prev_che_type = pt;
                    prev_che_tag = pg;
                    br.end = frame_end;
                    rest_r(es, frame_end);
                    return false;
                }
            } else {
                int pk = peek();
                if (pk == R_REST) {
                    rb(R_REST);
                    int64_t n = geti(0, (int64_t)1 << 40);
                    put_nibbles(getnibbles(), (size_t)n);
                    return false;
                }
                int t;
                switch (pk) {
                case R_SCE: t = TYPE_SCE; break;
                case R_CPE: t = TYPE_CPE; break;
                case R_CCE: t = TYPE_CCE; break;
                case R_LFE: t = TYPE_LFE; break;
                case R_DSE: t = TYPE_DSE; break;
                case R_PCE: t = TYPE_PCE; break;
                case R_FIL: t = TYPE_FIL; break;
                case R_END: t = TYPE_END; break;
                default:
                    wend_check();
                    err(pk < 0 ? std::string("unexpected end of input in a raw_data_block")
                               : std::string("expected a syntax element, found '") + rec_names[pk] + "'");
                }
                w->put(t, 3);
                if (element(t, blk_start, align_end))
                    return true;
            }
        }
    }
};
