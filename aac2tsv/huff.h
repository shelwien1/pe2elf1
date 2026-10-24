/*
 * aac2tsv - Huffman codebooks (encode and decode) built from the decoder's
 * code tables: spectral (1-11), scalefactor, SBR (10), PS (10).
 *
 * Licensed under the GNU Lesser General Public License v2.1 or later.
 */

struct Huff {
    enum { L1 = 10 };
    struct E {
        int32_t sym;     /* symbol (leaf) or subtable offset */
        uint8_t len;     /* code length (leaf), 0 if not a leaf */
        uint8_t sub;     /* subtable bits (first level only) */
    };
    int n = 0;                   /* number of symbols */
    int offset = 0;              /* value = symbol + offset */
    std::vector<uint32_t> code;  /* right aligned codeword per symbol */
    std::vector<uint8_t> len;    /* 0: symbol has no codeword */
    std::vector<E> tab;          /* [0, 1 << L1): first level, then subtables */

    void build()
    {
        tab.assign(1u << L1, E{ 0, 0, 0 });
        /* first level leaves */
        for (int s = 0; s < n; s++) {
            int l = len[s];
            if (!l || l > L1)
                continue;
            uint32_t first = code[s] << (L1 - l);
            for (uint32_t k = 0; k < (1u << (L1 - l)); k++)
                tab[first + k] = E{ s, (uint8_t)l, 0 };
        }
        /* subtables for the long codes, per L1-bit prefix */
        std::vector<int> maxsub(1u << L1, 0);
        for (int s = 0; s < n; s++)
            if (len[s] > L1) {
                uint32_t pre = code[s] >> (len[s] - L1);
                if (len[s] - L1 > maxsub[pre])
                    maxsub[pre] = len[s] - L1;
            }
        for (uint32_t pre = 0; pre < (1u << L1); pre++) {
            if (!maxsub[pre])
                continue;
            if (tab[pre].len)
                abort(); /* not a prefix code */
            int sb = maxsub[pre];
            tab[pre].sym = (int32_t)tab.size();
            tab[pre].sub = (uint8_t)sb;
            tab.resize(tab.size() + (1u << sb), E{ 0, 0, 0 });
        }
        for (int s = 0; s < n; s++) {
            int l = len[s];
            if (l <= L1)
                continue;
            uint32_t pre = code[s] >> (l - L1);
            int sb = tab[pre].sub;
            uint32_t rest = code[s] & ((1u << (l - L1)) - 1);
            uint32_t first = rest << (sb - (l - L1));
            for (uint32_t k = 0; k < (1u << (sb - (l - L1))); k++)
                tab[tab[pre].sym + first + k] = E{ s, (uint8_t)(l - L1), 0 };
        }
    }

    /* codes given explicitly per symbol */
    void init_codes(int nsym, const uint32_t *codes, const uint8_t *bits, int off)
    {
        n = nsym;
        offset = off;
        code.assign(codes, codes + nsym);
        len.assign(bits, bits + nsym);
        build();
    }
    void init_codes16(int nsym, const uint16_t *codes, const uint8_t *bits, int off)
    {
        std::vector<uint32_t> c(codes, codes + nsym);
        init_codes(nsym, c.data(), bits, off);
    }
    /* (symbol, length) pairs in code order, codes assigned consecutively
     * (as the decoder's vlc_init_tables_from_lengths() does) */
    void init_lengths(int nsym, const uint8_t (*tab_)[2], int off)
    {
        n = nsym;
        offset = off;
        code.assign(nsym, 0);
        len.assign(nsym, 0);
        uint64_t c = 0;
        for (int i = 0; i < nsym; i++) {
            int s = tab_[i][0], l = tab_[i][1];
            if (s >= nsym)
                abort();
            code[s] = (uint32_t)(c >> (32 - l));
            len[s] = (uint8_t)l;
            c += 1ull << (32 - l);
        }
        build();
    }

    /* returns the symbol (not offset) */
    int decode(BitReader &br) const
    {
        uint32_t x = br.peek(32);
        const E &e = tab[x >> (32 - L1)];
        if (e.len) {
            br.skip(e.len);
            return e.sym;
        }
        if (!e.sub)
            parse_fail("invalid Huffman code");
        const E &e2 = tab[e.sym + ((x << L1) >> (32 - e.sub))];
        if (!e2.len)
            parse_fail("invalid Huffman code");
        br.skip(L1 + e2.len);
        return e2.sym;
    }
    int decode_value(BitReader &br) const { return decode(br) + offset; }

    bool can_encode_value(int v) const
    {
        int s = v - offset;
        return s >= 0 && s < n && len[s];
    }
    void encode_value(BitWriter &bw, int v) const
    {
        int s = v - offset;
        bw.put(code[s], len[s]);
    }
};

/* spectral codebook properties, index cb-1 */
struct SpecCb {
    int dim;       /* 4 or 2 values per codeword */
    int range;     /* values per dimension in the index */
    int off;       /* index digit = value + off (signed books), |value| otherwise */
    bool is_signed;
    int maxval;    /* largest |value| without escape */
};
static const SpecCb spec_cb[11] = {
    { 4, 3, 1, true, 1 },   { 4, 3, 1, true, 1 },
    { 4, 3, 0, false, 2 },  { 4, 3, 0, false, 2 },
    { 2, 9, 4, true, 4 },   { 2, 9, 4, true, 4 },
    { 2, 8, 0, false, 7 },  { 2, 8, 0, false, 7 },
    { 2, 13, 0, false, 12 }, { 2, 13, 0, false, 12 },
    { 2, 17, 0, false, 16 },
};

enum {
    /* PS tables, order of aacps_huff_tabs */
    PS_IID_DF1, PS_IID_DT1, PS_IID_DF0, PS_IID_DT0,
    PS_ICC_DF, PS_ICC_DT, PS_IPD_DF, PS_IPD_DT, PS_OPD_DF, PS_OPD_DT,
};
enum {
    /* SBR tables, order of sbr_huffman_tab */
    T_HUFFMAN_ENV_1_5DB, F_HUFFMAN_ENV_1_5DB,
    T_HUFFMAN_ENV_BAL_1_5DB, F_HUFFMAN_ENV_BAL_1_5DB,
    T_HUFFMAN_ENV_3_0DB, F_HUFFMAN_ENV_3_0DB,
    T_HUFFMAN_ENV_BAL_3_0DB, F_HUFFMAN_ENV_BAL_3_0DB,
    T_HUFFMAN_NOISE_3_0DB, T_HUFFMAN_NOISE_BAL_3_0DB,
};

static Huff huff_spec[11], huff_sf, huff_sbr[10], huff_ps[10];

static void huff_init()
{
    static bool done;
    if (done)
        return;
    for (int i = 0; i < 11; i++)
        huff_spec[i].init_codes16(ff_aac_spectral_sizes[i], ff_aac_spectral_codes[i],
                                  ff_aac_spectral_bits[i], 0);
    huff_sf.init_codes(121, ff_aac_scalefactor_code, ff_aac_scalefactor_bits, -60);
    const uint8_t (*t)[2] = sbr_huffman_tab;
    for (int i = 0; i < 10; i++) {
        huff_sbr[i].init_lengths(sbr_huffman_nb_codes[i], t, sbr_vlc_offsets[i]);
        t += sbr_huffman_nb_codes[i];
    }
    t = aacps_huff_tabs;
    for (int i = 0; i < 10; i++) {
        huff_ps[i].init_lengths(huff_sizes[i], t, huff_offset[i]);
        t += huff_sizes[i];
    }
    done = true;
}
