/*  Drive libvorbis's own psychoacoustic analysis and floor fit over blocks
    whose sizes we impose, rather than letting the encoder pick them.  Reads
    the dump tsv2wav writes: per block, the block size, the floor's post list,
    the coded posts, and the PCM window the block spans.  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vorbis/codec.h"
#include "vorbis/vorbisenc.h"
#include "codec_internal.h"
#include "psy.h"
#include "mdct.h"
#include "smallft.h"
#include "registry.h"
#include "misc.h"
#include "os.h"
#include "scales.h"

#define MAXN 8192
static float pcmw[MAXN], mdctb[MAXN], fftb[MAXN], logfft[MAXN], logmdct[MAXN];
static float noise[MAXN], tone[MAXN], logmask[MAXN];

int main(int argc, char ** argv) {
  vorbis_info vi;  vorbis_dsp_state vd;  vorbis_block vb;
  int q = atoi(argv[1]), rate = atoi(argv[2]), ch = atoi(argv[3]);
  int bt = argc > 5 ? atoi(argv[5]) : 0;
  FILE * f = fopen(argv[4], "r");
  codec_setup_info * ci;  private_state * b;
  char line[64];
  long exact = 0, tot = 0, within1 = 0;  double sumabs = 0, sumsig = 0;
  float gampmax = -9999.f;                /*  tracked across the stream, as
                                              mapping0_forward does  */
  static int dv[2000000];
  int dbg=0, blk=0;
  FILE * dumpf = getenv("FITDUMP") ? fopen(getenv("FITDUMP"), "w") : NULL;

  vorbis_info_init(&vi);
  if (vorbis_encode_init_vbr(&vi, ch, rate, q * 0.1f)) { puts("init failed"); return 1; }
  vorbis_analysis_init(&vd, &vi);
  vorbis_block_init(&vd, &vb);
  ci = vi.codec_setup;  b = vd.backend_state;

  while (fgets(line, sizeof line, f)) {
    int n, posts, mult, W, i, j;
    static int px[256], py[256];
    if (line[0] != 'B') continue;
    blk++;
    sscanf(line, "B %d %d %d %d", &n, &posts, &mult, &W);
    for (i = 0; i < posts; i++) { fgets(line, sizeof line, f);
                                  sscanf(line, "P %d %d", px + i, py + i); }
    for (i = 0; i < 2 * n; i++) { fgets(line, sizeof line, f);
                                  sscanf(line, "W %f", pcmw + i); }
    /*  which of libvorbis's two block sizes matches ours  */
    if (ci->blocksizes[W] != 2 * n) continue;
    {
      vorbis_look_psy * pl = b->psy + bt + (W ? 2 : 0);
      vorbis_look_floor1 * fl;
      int fno = -1;
      /*  the floor whose post list matches the stream's  */
      for (i = 0; i < ci->floors; i++) {
        vorbis_info_floor1 * inf = ci->floor_param[i];
        if (ci->floor_type[i] != 1 || inf->postlist[1] != px[1]) continue;
        fno = i;  break;
      }
      if (fno < 0) continue;
      fl = b->flr[fno];
      if (fl->posts != posts) continue;

      memcpy(mdctb, pcmw, 2 * n * sizeof(float));
      mdct_forward((mdct_lookup *) b->transform[W][0], mdctb, mdctb);
      memcpy(fftb, pcmw, 2 * n * sizeof(float));
      drft_forward(&b->fft_look[W], fftb);
      {
        /*  exactly as mapping0_forward computes it  */
        float scale = 4.f / (2 * n);
        float scale_dB = todB(&scale) + .345f;
        float amp;
        logfft[0] = scale_dB + todB(fftb) + .345f;
        amp = logfft[0];
        for (j = 1; j < 2 * n - 1; j += 2) {
          float temp = fftb[j] * fftb[j] + fftb[j + 1] * fftb[j + 1];
          temp = logfft[(j + 1) >> 1] = scale_dB + .5f * todB(&temp) + .345f;
          if (temp > amp) amp = temp;
        }
        for (i = 0; i < n; i++) logmdct[i] = todB(mdctb + i) + .345f;
        if (!dbg) { double s1=0,s2=0,s3=0; int c;
          dbg=1;
          for (c=0;c<n;c++) s1+=logmdct[c];
          for (c=0;c<n;c++) s2+=logfft[c];
          fprintf(stderr,"  n=%d  mean logmdct %.1f dB  mean logfft %.1f dB  amp %.1f\n",
                  n, s1/n, s2/n, amp);
          (void)s3; }
        /*  local_ampmax is clamped at 0; global_ampmax carries across blocks
            with the per-second attenuation applied each time  */
        if (amp > 0.f) amp = 0.f;
        { codec_setup_info * cs = vi.codec_setup;
          float secs = (float) n / vi.rate;
          gampmax += secs * cs->psy_g_param.ampmax_att_per_sec;
          if (gampmax < -9999.f) gampmax = -9999.f;
          if (amp > gampmax) gampmax = amp; }
        _vp_noisemask(pl, logmdct, noise);
        _vp_tonemask(pl, logfft, tone, gampmax, amp);
        _vp_offset_and_mix(pl, noise, tone, 1, logmask, mdctb, logmdct);
        if (dbg==1) { double s1=0,s2=0,s4=0; int c; dbg=2;
          for (c=0;c<n;c++){s1+=noise[c];s2+=tone[c];s4+=logmask[c];}
          fprintf(stderr,"  mean noise %.1f  mean tone %.1f  mean logmask %.1f dB\n",
                  s1/n,s2/n,s4/n); }
        {
          int * out;
          _vorbis_block_ripcord(&vb);
          vb.W = W;  vb.pcmend = 2 * n;  vb.vd = &vd;  vb.mode = 0;
          out = floor1_fit(&vb, fl, logmdct, logmask);
          if (!out) continue;
          for (i = 0; i < posts; i++) {
            int v = out[i] & 0x7fff, d;
            switch (((vorbis_info_floor1 *) ci->floor_param[fno])->mult) {
              case 1: v >>= 2; break;  case 2: v >>= 3; break;
              case 3: v /= 12; break;  default: v >>= 4; break;
            }
            d = py[i] - v;
            if (dumpf) fprintf(dumpf, "D %d %d %d %d %d %d %d\n",
                               blk, i, px[i], py[i], v, posts, out[i] & 0x7fff);
            if (tot < 2000000) { dv[tot] = d; }
            tot++;  sumabs += fabs((double) d);  sumsig += d;
            if (!d) exact++;
            if (d >= -1 && d <= 1) within1++;
          }
        }
      }
    }
  }
  if (tot) {
    long i2, off = (long) (sumsig / tot + (sumsig < 0 ? -0.5 : 0.5));
    long ex = 0, w1 = 0, w2 = 0;  double sa = 0;
    printf("q=%.1f  posts %ld  exact %.1f%%  within1 %.1f%%  mean|dY| %.2f  mean signed %+.2f\n",
           q * 0.1, tot, 100.0 * exact / tot, 100.0 * within1 / tot,
           sumabs / tot, sumsig / tot);
    for (i2 = 0; i2 < tot && i2 < 2000000; i2++) {
      int e = dv[i2] - (int) off;
      if (!e) ex++;
      if (e >= -1 && e <= 1) w1++;
      if (e >= -2 && e <= 2) w2++;
      sa += fabs((double) e);
    }
    { /*  entropy of the correction that would be stored  */
      static long h[4096];  double H = 0;  long i3;
      memset(h, 0, sizeof h);
      for (i3 = 0; i3 < tot && i3 < 2000000; i3++) {
        int e = dv[i3] + 2048;
        if (e >= 0 && e < 4096) h[e]++;
      }
      for (i3 = 0; i3 < 4096; i3++) if (h[i3]) {
        double p = (double) h[i3] / tot;  H -= p * log(p) / log(2.0);
      }
      printf("        H(dY) = %.3f bits   (cost basis, coded flr.y: 3.37)\n", H);
    }
    (void) off; (void) ex; (void) w1; (void) w2; (void) sa;
  } else printf("q=%.1f  no usable blocks\n", q * 0.1);
  if (dumpf) fclose(dumpf);
  return 0;
}
