// The SBR frequency band tables (14496-3 4.6.18.3), as far as the
// bitstream syntax needs them: the number of envelope bands at low and
// high frequency resolution and of noise floor bands, and the checks that
// make a decoder turn SBR off. aac2tsv's sbrfreq.h, ported from aacdec
// aacsbr.inc (FFmpeg aacsbr_template.c; LGPL 2.1 or later).
//
// The decoder computes in float; here it is done in double, which gives the
// same tables for every possible header and sample rate (checked
// exhaustively by aac2tsv), with enough margin to the rounding decisions
// that compilers, libms and -ffast-math cannot change them.

namespace aacpack {

struct SbrSpectrum {
    int start_freq, stop_freq, xover_band, freq_scale, alter_scale, noise_bands;
    bool operator==(const SbrSpectrum &o) const
    {
        return start_freq == o.start_freq && stop_freq == o.stop_freq && xover_band == o.xover_band &&
               freq_scale == o.freq_scale && alter_scale == o.alter_scale && noise_bands == o.noise_bands;
    }
    bool operator!=(const SbrSpectrum &o) const { return !(*this == o); }
};

struct SbrBands {
    int n[2];   /* envelope bands, low / high frequency resolution */
    int n_q;    /* noise floor bands */
};

static int sbr_cmp_int16(const void *a, const void *b)
{
    return *(const int16_t *)a - *(const int16_t *)b;
}

static void sbr_make_bands(int16_t *bands, int start, int stop, int num_bands)
{
    int k, previous, present;
    double base, prod;

    base = pow((double)stop / start, 1.0 / num_bands);
    prod = start;
    previous = start;

    for (k = 0; k < num_bands - 1; k++) {
        prod *= base;
        present = (int)lrint(prod);
        bands[k] = (int16_t)(present - previous);
        previous = present;
    }
    bands[num_bands - 1] = (int16_t)(stop - previous);
}

/* returns false where the decoder's sbr_reset() fails (SBR turned off) */
static bool sbr_compute_bands(int sample_rate, const SbrSpectrum &sp, SbrBands &out)
{
    const int8_t *offs;
    int k0, k1, k2, n_master;
    int16_t f_master[64 + 1];

    switch (sample_rate) {
    case 16000: offs = sbr_offset[0]; break;
    case 22050: offs = sbr_offset[1]; break;
    case 24000: offs = sbr_offset[2]; break;
    case 32000: offs = sbr_offset[3]; break;
    case 44100: case 48000: case 64000: offs = sbr_offset[4]; break;
    case 88200: case 96000: case 128000: case 176400: case 192000: offs = sbr_offset[5]; break;
    default: return false;
    }

    /* sbr_make_f_master() */
    unsigned temp = sample_rate < 32000 ? 3000 : sample_rate < 64000 ? 4000 : 5000;
    unsigned start_min = ((temp << 7) + (sample_rate >> 1)) / sample_rate;
    unsigned stop_min  = ((temp << 8) + (sample_rate >> 1)) / sample_rate;

    k0 = start_min + offs[sp.start_freq];
    if (sp.stop_freq < 14) {
        int16_t stop_dk[13];
        k2 = stop_min;
        sbr_make_bands(stop_dk, stop_min, 64, 13);
        qsort(stop_dk, 13, sizeof(int16_t), sbr_cmp_int16);
        for (int i = 0; i < sp.stop_freq; i++)
            k2 += stop_dk[i];
    } else if (sp.stop_freq == 14) {
        k2 = 2 * k0;
    } else {
        k2 = 3 * k0;
    }
    if (k2 > 64)
        k2 = 64;

    int max_qmf_subbands = sample_rate <= 32000 ? 48 : sample_rate == 44100 ? 35 : 32;
    if (k2 - k0 > max_qmf_subbands)
        return false;

    if (!sp.freq_scale) {
        int dk = sp.alter_scale + 1, k2diff;
        n_master = ((k2 - k0 + (dk & 2)) >> dk) << 1;
        if (n_master <= 0 || sp.xover_band >= n_master)
            return false;
        for (int i = 1; i <= n_master; i++)
            f_master[i] = (int16_t)dk;
        k2diff = k2 - k0 - n_master * dk;
        if (k2diff < 0) {
            f_master[1]--;
            f_master[2] -= (k2diff < -1);
        } else if (k2diff) {
            f_master[n_master]++;
        }
        f_master[0] = (int16_t)k0;
        for (int i = 1; i <= n_master; i++)
            f_master[i] += f_master[i - 1];
    } else {
        int half_bands = 7 - sp.freq_scale;
        int two_regions, num_bands_0, vdk0_max;
        int16_t vk0[49];

        if (49 * k2 > 110 * k0) {
            two_regions = 1;
            k1 = 2 * k0;
        } else {
            two_regions = 0;
            k1 = k2;
        }
        num_bands_0 = (int)lrint(half_bands * log2(k1 / (double)k0)) * 2;
        if (num_bands_0 <= 0 || num_bands_0 > 48)
            return false;
        vk0[0] = 0;
        sbr_make_bands(vk0 + 1, k0, k1, num_bands_0);
        qsort(vk0 + 1, num_bands_0, sizeof(int16_t), sbr_cmp_int16);
        vdk0_max = vk0[num_bands_0];
        vk0[0] = (int16_t)k0;
        for (int i = 1; i <= num_bands_0; i++) {
            if (vk0[i] <= 0)
                return false;
            vk0[i] += vk0[i - 1];
        }
        if (two_regions) {
            int16_t vk1[49];
            double invwarp = sp.alter_scale ? 0.76923076923076923077 : 1.0;
            int num_bands_1 = (int)lrint(half_bands * invwarp * log2(k2 / (double)k1)) * 2;
            if (num_bands_1 <= 0 || num_bands_1 > 48)
                return false;
            sbr_make_bands(vk1 + 1, k1, k2, num_bands_1);
            int vdk1_min = vk1[1];
            for (int i = 2; i <= num_bands_1; i++)
                if (vk1[i] < vdk1_min)
                    vdk1_min = vk1[i];
            if (vdk1_min < vdk0_max) {
                int change;
                qsort(vk1 + 1, num_bands_1, sizeof(int16_t), sbr_cmp_int16);
                change = vdk0_max - vk1[1];
                if ((vk1[num_bands_1] - vk1[1]) >> 1 < change)
                    change = (vk1[num_bands_1] - vk1[1]) >> 1;
                vk1[1] += change;
                vk1[num_bands_1] -= change;
            }
            qsort(vk1 + 1, num_bands_1, sizeof(int16_t), sbr_cmp_int16);
            vk1[0] = (int16_t)k1;
            for (int i = 1; i <= num_bands_1; i++) {
                if (vk1[i] <= 0)
                    return false;
                vk1[i] += vk1[i - 1];
            }
            n_master = num_bands_0 + num_bands_1;
            if (n_master > 64 || sp.xover_band >= n_master)
                return false;
            memcpy(&f_master[0], vk0, (num_bands_0 + 1) * sizeof(f_master[0]));
            memcpy(&f_master[num_bands_0 + 1], vk1 + 1, num_bands_1 * sizeof(f_master[0]));
        } else {
            n_master = num_bands_0;
            if (sp.xover_band >= n_master)
                return false;
            memcpy(f_master, vk0, (num_bands_0 + 1) * sizeof(f_master[0]));
        }
    }

    /* sbr_make_f_derived() */
    int16_t f_tablehigh[64 + 1];
    int n1 = n_master - sp.xover_band;
    int n0 = (n1 + 1) >> 1;
    memcpy(f_tablehigh, &f_master[sp.xover_band], (n1 + 1) * sizeof(f_master[0]));
    int m = f_tablehigh[n1] - f_tablehigh[0];
    int kx = f_tablehigh[0];
    if (kx + m > 64 || kx > 32)
        return false;
    int n_q = (int)lrint(sp.noise_bands * log2(k2 / (double)kx));
    if (n_q < 1)
        n_q = 1;
    if (n_q > 5)
        return false;

    /* sbr_hf_calc_npatches(): patch construction must terminate */
    {
        int i, k, last_k = -1, last_msb = -1, sb = 0, num_patches = 0;
        int msb = k0, usb = kx;
        int goal_sb = ((1000 << 11) + (sample_rate >> 1)) / sample_rate;
        if (goal_sb < kx + m) {
            for (k = 0; f_master[k] < goal_sb; k++)
                ;
        } else
            k = n_master;
        do {
            int odd = 0;
            if (k == last_k && msb == last_msb)
                return false;
            last_k = k;
            last_msb = msb;
            for (i = k; i == k || sb > (k0 - 1 + msb - odd); i--) {
                if (i < 0)
                    return false;
                sb = f_master[i];
                odd = (sb + k0) & 1;
            }
            if (num_patches > 5)
                return false;
            int nsub = sb - usb > 0 ? sb - usb : 0;
            if (nsub > 0) {
                usb = sb;
                msb = sb;
                num_patches++;
            } else
                msb = kx;
            if (f_master[k] - sb < 3)
                k = n_master;
        } while (sb != kx + m);
    }

    out.n[0] = n0;
    out.n[1] = n1;
    out.n_q = n_q;
    return true;
}

}  // namespace aacpack
