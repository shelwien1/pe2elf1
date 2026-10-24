/*
 * aac2tsv - AudioSpecificConfig (14496-3 1.6.2.1), parsed for its length and
 * for the configuration the access units need. It is kept as hex in the
 * TSV. Follows aacdec ff_mpeg4audio_get_config_gb(),
 * decode_ga_specific_config(), decode_eld_specific_config().
 *
 * Licensed under the GNU Lesser General Public License v2.1 or later.
 */

struct AscInfo {
    int aot;           /* core object type (after explicit SBR/PS signaling) */
    int sf_index;      /* core sampling frequency index (15: explicit rate) */
    int chan_config;
    int frame960;      /* frameLengthFlag */
    int er;            /* 1 ER AAC LC/LD, 2 ER AAC ELD */
    bool raw_blocks;   /* aac2tsv parses the access units */
};

static int asc_object_type(BitReader &b)
{
    int aot = b.get(5);
    if (aot == 31)
        aot = 32 + b.get(6);
    return aot;
}

static int asc_sample_rate(BitReader &b)
{
    int idx = b.get(4);
    if (idx == 15)
        b.get(24);
    return idx;
}

/* program_config_element() after element_instance_tag (decode_pce()) */
static void asc_skip_pce(BitReader &b, size_t align_ref)
{
    b.get(2);   /* object_type */
    b.get(4);   /* sampling_frequency_index */
    int nf = b.get(4), ns = b.get(4), nb = b.get(4), nl = b.get(2), na = b.get(3), nc = b.get(4);
    if (b.get1())
        b.get(4);
    if (b.get1())
        b.get(4);
    if (b.get1())
        b.get(3);
    b.skip(5 * (size_t)(nf + ns + nb + nc) + 4 * (size_t)(nl + na));
    b.skip((align_ref - b.pos) & 7);
    b.skip(8 * (size_t)b.get(8));   /* comment_field_data */
}

/* parse an AudioSpecificConfig at b.pos; ParseError if unsupported. The
 * backward compatible sync extension is not parsed: in LATM version 1 the
 * bits after the core config up to ascLen are kept as they are. */
static AscInfo parse_asc(BitReader &b, size_t align_ref)
{
    AscInfo a;
    a.aot = asc_object_type(b);
    a.sf_index = asc_sample_rate(b);
    a.chan_config = b.get(4);
    a.frame960 = 0;
    a.er = 0;
    bool ldsbr = false;
    if (a.chan_config > 14)
        parse_fail("invalid channel configuration");
    if (a.aot == 5 || (a.aot == 29 && !(b.peek(3) & 0x03 && !(b.peek(9) & 0x3f)))) {
        asc_sample_rate(b);              /* extensionSamplingFrequencyIndex */
        a.aot = asc_object_type(b);
        if (a.aot == 22)
            b.get(4);                    /* extensionChannelConfiguration */
    }
    switch (a.aot) {
    case 1: case 2: case 3: case 4: case 17: case 23: {   /* GASpecificConfig */
        a.frame960 = b.get1();
        if (b.get1())       /* dependsOnCoreCoder */
            b.get(14);      /* coreCoderDelay */
        int ext = b.get1(); /* extensionFlag */
        if (a.chan_config == 0) {
            b.get(4);       /* element_instance_tag */
            asc_skip_pce(b, align_ref);
        }
        if (ext) {
            if (a.aot == 17 || a.aot == 23)
                if (b.get(3))   /* aacSection/Scalefactor/SpectralDataResilienceFlag */
                    parse_fail("error resilience tools");
            b.get1();           /* extensionFlag3 */
        }
        if (a.aot == 17 || a.aot == 23)
            if (b.get(2))       /* epConfig */
                parse_fail("epConfig");
        break;
    }
    case 39: {   /* ELDSpecificConfig */
        a.frame960 = b.get1();
        if (b.get(3))
            parse_fail("error resilience tools");
        if (b.get1()) {   /* ldSbrPresentFlag: aacdec does not decode it */
            ldsbr = true;
            b.get(2);     /* ldSbrSamplingRate, ldSbrCrcFlag */
            static const int nhdr[16] = { 0, 1, 1, 2, 3, 3, 3, 4, 0, 0, 0, 4, 4, 0, 4, 0 };   /* as fdk-aac */
            for (int k = 0; k < nhdr[a.chan_config]; k++) {   /* ld_sbr_header() */
                b.get(14);                   /* bs_amp_res .. bs_reserved */
                int x1 = b.get1(), x2 = b.get1();
                if (x1)
                    b.get(5);                /* bs_freq_scale, bs_alter_scale, bs_noise_bands */
                if (x2)
                    b.get(6);                /* bs_limiter_bands .. bs_smoothing_mode */
            }
        }
        while (b.get(4)) {   /* eldExtType != ELDEXT_TERM */
            int len = b.get(4);
            if (len == 15)
                len += b.get(8);
            if (len == 15 + 255)
                len += b.get(16);
            b.skip(8 * (size_t)len);
        }
        if (b.get(2))
            parse_fail("epConfig");
        break;
    }
    default:
        parse_fail("unsupported audio object type");
    }
    a.er = a.aot == 17 || a.aot == 23 ? 1 : a.aot == 39 ? 2 : 0;
    if (!a.er) {
        a.raw_blocks = a.aot >= 1 && a.aot <= 4 && a.sf_index <= 12;
    } else {
        bool ld = a.aot != 17;
        a.raw_blocks = a.sf_index <= 12 && er_layout[a.chan_config] && !ldsbr &&
                       (!ld || (a.frame960 ? ff_aac_num_swb_480 : ff_aac_num_swb_512)[a.sf_index]);
    }
    return a;
}
