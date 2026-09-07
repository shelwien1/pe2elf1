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

/*  Predicting the floor with libvorbis's own analysis.

    The floor posts are not in the PCM in any recoverable sense -- they are a
    fit to the psychoacoustic mask of the signal *before* quantization, and
    that mask is a hundred-odd tuning constants the format never transmits.
    But libvorbis holds those constants, keyed by quality, and the decoded PCM
    differs from what the encoder saw only by the codec's own quantization.
    So rather than infer the floor, this runs the encoder's analysis again on
    the decoded audio.

    The blocks are ours, not the encoder's.  Left to itself libvorbis picks its
    own block sizes -- 415 where one test stream had 877 -- and then no two
    floors describe the same span.  Driving the internals directly with the
    block size, window flags and floor configuration already known from the
    record stream makes every prediction line up by construction.

    What remains is the quality, which mode c finds by sweeping and writes to
    the meta as one index.  On a stream whose encoder is reproducible the sweep
    has a sharp maximum at the setting actually used.  */

constexpr u32 VF_MAXN = 4096;

struct vfloor {
  vorbis_info vi;
  vorbis_dsp_state vd;
  vorbis_block vb;
  int ready, quality;
  float gain;                             /*  dB added before the psy model  */
  i32 curve[VF_MAXN];
  float pcmw[2 * VF_MAXN], mdctb[2 * VF_MAXN], fftb[2 * VF_MAXN];
  float logfft[VF_MAXN], logmdct[VF_MAXN];
  float noise[VF_MAXN], tone[VF_MAXN], logmask[VF_MAXN];
  float gampmax;

  vfloor() : ready(0), quality(-1), gampmax(-9999.f) {}

  void close() {
    if (!ready) return;
    vorbis_block_clear(&vb);  vorbis_dsp_clear(&vd);  vorbis_info_clear(&vi);
    ready = 0;
  }

  /*  Stand up an encoder at one quality.  Returns 0 if the setting cannot
      describe this stream at all.  */
  int open(u32 ch, u32 rate, int q10) {
    close();
    vorbis_info_init(&vi);
    if (vorbis_encode_init_vbr(&vi, (long) ch, (long) rate, q10 * 0.1f)) {
      vorbis_info_clear(&vi);  return 0;
    }
    vorbis_analysis_init(&vd, &vi);
    vorbis_block_init(&vd, &vb);
    ready = 1;  quality = q10;  gampmax = -9999.f;  gain = 0.f;
    return 1;
  }

  void reset_amp() { gampmax = -9999.f; }

  /*  Render a fitted post set into a curve of dB-table indices, the way the
      decoder would.  Needed when libvorbis's post positions are not the
      stream's: the curve can still be sampled wherever the stream put its
      posts, which is what lets a stream from a different encoder be
      predicted at all.  */
  void render(const int * out, const vorbis_info_floor1 * inf,
              const vorbis_look_floor1 * fl, u32 n, i32 * curve) {
    int lx = 0, ly = (out[0] & 0x7fff) * inf->mult, hx = 0, hy;
    u32 j;
    for (j = 1; j < (u32) fl->posts; j++) {
      int cur = fl->forward_index[j];
      if (out[cur] & 0x8000) continue;    /*  the fit did not use this post  */
      hy = (out[cur] & 0x7fff) * inf->mult;
      hx = inf->postlist[cur];
      { int dy = hy - ly, adx = hx - lx, ady = dy < 0 ? -dy : dy;
        int base = adx ? dy / adx : 0, sy = dy < 0 ? base - 1 : base + 1;
        int x = lx, y = ly, err = 0;
        ady -= (base < 0 ? -base : base) * adx;
        if (x < (int) n) curve[x] = y;
        while (++x < hx && x < (int) n) {
          err += ady;
          if (err >= adx) { err -= adx;  y += sy; } else y += base;
          curve[x] = y;
        }
      }
      lx = hx;  ly = hy;
    }
    for (j = (u32) hx; j < n; j++) curve[j] = ly;
  }

  /*  The floor libvorbis would have fitted to this block.  `x` is the post
      list and `posts` its length; `out` receives the posts in the stream's
      own units.  Returns 0 when this encoder setting has no floor or block
      size matching the stream's.  */
  int fit(const i16 * seg, u32 stride, u32 c, u32 n, int W,
          const i64 * win, const u32 * x, u32 posts, u32 mult, i32 * out) {
    codec_setup_info * ci = (codec_setup_info *) vi.codec_setup;
    private_state * b = (private_state *) vd.backend_state;
    vorbis_look_psy * pl;
    vorbis_look_floor1 * fl = nullptr;
    int fno = -1, i, j, direct;
    u32 k;
    if (!ready || n > VF_MAXN) return 0;
    if (ci->blocksizes[W] != (long) (2 * n)) return 0;
    /*  prefer a floor whose post list is the stream's; otherwise take one
        that spans the same block and sample its curve  */
    for (i = 0; i < ci->floors; i++) {
      vorbis_info_floor1 * inf = (vorbis_info_floor1 *) ci->floor_param[i];
      if (ci->floor_type[i] != 1) continue;
      if ((u32) inf->postlist[1] != x[1]) continue;
      if ((u32) inf->mult == mult) { fno = i;  break; }
      if (fno < 0) fno = i;
    }
    if (fno < 0) return 0;
    fl = (vorbis_look_floor1 *) b->flr[fno];
    {
      vorbis_info_floor1 * inf = (vorbis_info_floor1 *) ci->floor_param[fno];
      direct = ((u32) fl->posts == posts && (u32) inf->mult == mult);
      if (direct)
        for (k = 0; k < posts; k++)
          if ((u32) inf->postlist[k] != x[k]) { direct = 0;  break; }
    }

    pl = b->psy + (W ? 2 : 0);
    for (k = 0; k < 2 * n; k++)
      pcmw[k] = (float) ((double) seg[k * stride + c] / 32767.0 *
                         ((double) win[k] / (double) ((i64) 1 << F)));
    memcpy(mdctb, pcmw, 2 * n * sizeof *mdctb);
    mdct_forward((mdct_lookup *) b->transform[W][0], mdctb, mdctb);
    memcpy(fftb, pcmw, 2 * n * sizeof *fftb);
    drft_forward(&b->fft_look[W], fftb);
    {
      float scale = 4.f / (2 * n);
      float scale_dB = todB(&scale) + .345f;
      float amp;
      logfft[0] = scale_dB + todB(fftb) + .345f;
      amp = logfft[0];
      for (j = 1; j < (int) (2 * n) - 1; j += 2) {
        float t = fftb[j] * fftb[j] + fftb[j + 1] * fftb[j + 1];
        t = logfft[(j + 1) >> 1] = scale_dB + .5f * todB(&t) + .345f;
        if (t > amp) amp = t;
      }
      if (amp > 0.f) amp = 0.f;
      /*  global_ampmax carries across the stream, attenuated per block; the
          tone mask reads both it and the block's own peak  */
      gampmax += (float) n / (float) vi.rate *
                 ci->psy_g_param.ampmax_att_per_sec;
      if (gampmax < -9999.f) gampmax = -9999.f;
      if (amp > gampmax) gampmax = amp;
      /*  The decoded PCM is not what the encoder analysed -- it has been
          through the codec's own quantization -- and the difference shows up
          mostly as a level shift in the mask.  One fitted decibel offset,
          carried in the meta, takes most of it out.  */
      for (k = 0; k < n; k++) logmdct[k] = todB(mdctb + k) + .345f + gain;
      _vp_noisemask(pl, logmdct, noise);
      _vp_tonemask(pl, logfft, tone, gampmax, amp);
      _vp_offset_and_mix(pl, noise, tone, 1, logmask, mdctb, logmdct);
    }
    {
      int * r;
      _vorbis_block_ripcord(&vb);
      vb.W = W;  vb.pcmend = (long) (2 * n);  vb.vd = &vd;  vb.mode = 0;
      r = floor1_fit(&vb, fl, logmdct, logmask);
      if (!r) return 0;
      if (direct) {
        for (k = 0; k < posts; k++) {
          int v = r[k] & 0x7fff;
          /*  libvorbis truncates here; matching that matters, and rounding
              instead costs ten points of exact agreement  */
          switch (mult) {
            case 1: v >>= 2;  break;
            case 2: v >>= 3;  break;
            case 3: v /= 12;  break;
            default: v >>= 4;  break;
          }
          out[k] = v;
        }
      } else {
        vorbis_info_floor1 * inf = (vorbis_info_floor1 *) ci->floor_param[fno];
        int q[VIF_POSIT + 2];
        for (k = 0; k < (u32) fl->posts; k++) {
          int v = r[k] & 0x7fff;
          switch (inf->mult) {
            case 1: v >>= 2;  break;
            case 2: v >>= 3;  break;
            case 3: v /= 12;  break;
            default: v >>= 4;  break;
          }
          q[k] = v | (r[k] & 0x8000);
        }
        render(q, inf, fl, n, curve);
        /*  sample where the stream put its posts, in the stream's units  */
        for (k = 0; k < posts; k++) {
          i32 c = curve[x[k] < n ? x[k] : n - 1];
          out[k] = (c + (i32) mult / 2) / (i32) mult;
          if (out[k] < 0) out[k] = 0;
        }
      }
    }
    return 1;
  }
};
