// Codebook selection: the section data the encoders would choose for a
// group of a channel, from its quantized spectral values and the bands'
// kinds (zero, coded, noise, intensity). Model's order f_CS codes the
// section data after the spectra, with these as contexts. Each is the
// encoder's own algorithm over the bits each codebook would take:
//  - fdk-aac (libAACenc dyn_bits.cpp): each band its cheapest codebook,
//    neighbours of the same one joined, then the two neighbouring sections
//    whose merging saves the most bits merged, and again, while one saves
//  - ffmpeg (aaccoder_trellis.h, codebook_trellis_rate()): the cheapest path
//    through the bands' codebooks, from each band's smallest codebook up,
//    with the section's length bits (its trellis keeps one path per
//    codebook, as ffmpeg's does)
//  - faac (huff2.c): each band the smaller of the two codebooks of the
//    smallest pair that holds its largest magnitude (the lower on a tie),
//    neighbours of the same codebook one section
// They come out as the stream's in every band of every fdk-aac and faac
// stream tried, and in 99% of ffmpeg's bands (ffmpeg's smallest codebook
// is of its own estimate of the band's largest magnitude).

namespace aacpack {

enum { CS_FDK, CS_FF, CS_FAAC, CS_N };  // the encoders
const int kCsInf = 1 << 28;              // a codebook that cannot hold the band

struct CbSel {
  int nb;              // the group's bands (max_sfb)
  bool shortw;         // eight short windows
  uint8_t kind[64];    // each band's: 0 none, 1 a spectral codebook, 13-15 noise or intensity
  int bits[64][12];    // what each codebook (0 the zero one) takes of the band's values
  uint8_t minb[64];    // the smallest codebook that holds the band's (0 all zero)
  uint8_t cb[CS_N][64];  // each encoder's codebook of each band
  uint8_t st[CS_N][64];  // and whether a section starts there

  // a tuple's bits in codebook c (1-11), kCsInf if it does not hold it
  static int tuple_bits(int c, const int* v) {
    const SpecCb& p = spec_cb[c - 1];
    int idx = 0, b = 0;
    for (int j = 0; j < p.dim; j++) {
      const int x = v[j], a = x < 0 ? -x : x;
      if (c < 11 && a > p.maxval) return kCsInf;
      idx = idx * p.range + (p.is_signed ? x + p.off : a < 16 ? a : 16);
      if (!p.is_signed && a) b++;
      if (c == 11 && a >= 16) {
        int n = 0;
        while ((a >> (n + 5)) > 0) n++;
        b += 2 * n + 5;
      }
    }
    return b + huff_spec[c - 1].len[idx];
  }

  // the group g's bands of the channel's values (window w's at w * 128):
  // each codebook's bits
  void count(const Ics& ii, int g, const int* coef, const uint8_t* kinds) {
    nb = ii.max_sfb;
    shortw = ii.num_windows == 8;
    int w0 = 0;
    for (int i = 0; i < g; i++) w0 += ii.group_len[i];
    const int gl = shortw ? ii.group_len[g] : 1;
    for (int b = 0; b < nb; b++) {
      kind[b] = kinds[b];
      int* bc = bits[b];
      for (int c = 0; c < 12; c++) bc[c] = kCsInf;
      minb[b] = 0;
      if (kind[b] >= 13) continue;
      const int a = ii.swb[b], e = ii.swb[b + 1];  // (a band of no codebook too: its zeros, as an encoder counts them)
      int mx = 0;
      for (int w = w0; w < w0 + gl; w++)
        for (int k = a; k < e; k++) {
          const int x = coef[w * 128 + k];
          mx = Max(mx, x < 0 ? -x : x);
        }
      const int lo = mx == 0 ? 1 : mx <= 1 ? 1 : mx <= 2 ? 3 : mx <= 4 ? 5 : mx <= 7 ? 7 : mx <= 12 ? 9 : 11;
      minb[b] = (uint8_t)(mx == 0 ? 0 : lo);
      if (!mx) bc[0] = 0;
      for (int c = lo; c <= 11; c++) {
        const int dim = spec_cb[c - 1].dim;
        int s = 0;
        for (int w = w0; w < w0 + gl && s < kCsInf; w++)
          for (int k = a; k < e; k += dim) {
            const int t = tuple_bits(c, coef + w * 128 + k);
            if (t >= kCsInf) {
              s = kCsInf;
              break;
            }
            s += t;
          }
        bc[c] = s;
      }
    }
  }

  // the codebook (1-11) that takes the fewest bits for the bands [lo, hi)
  // (the lower on a tie)
  int cheapest(int lo, int hi) const {
    int best = 11, bv = kCsInf;
    for (int c = 1; c <= 11; c++) {
      int t = 0;
      for (int b = lo; b < hi && t < kCsInf; b++) t = bits[b][c] >= kCsInf ? kCsInf : t + bits[b][c];
      if (t < bv) bv = t, best = c;
    }
    return best;
  }

  // section data bits of a section of cnt bands
  int side(int cnt) const { return shortw ? 4 + 3 * (cnt / 7 + 1) : 4 + 5 * (cnt / 31 + 1); }

  void fdk() {
    struct Sec {
      int cnt, start, bits, book;
    } s[64];
    int lk[64][12];
    // the codebook that takes the fewest bits of the bands merged at i (book
    // where none holds them), and its bits
    auto best_of = [&](int i, int book, int& mb) {
      mb = kCsInf;
      for (int j = 0; j < 12; j++)
        if (lk[i][j] < mb) mb = lk[i][j], book = j;
      return book;
    };
    for (int i = 0; i < nb; i++) {
      for (int j = 0; j < 12; j++) lk[i][j] = bits[i][j];
      s[i].cnt = 1, s[i].start = i;
      if (kind[i] >= 13)
        s[i].book = kind[i], s[i].bits = 0;
      else
        s[i].book = best_of(i, 0, s[i].bits);
    }
    auto merge_bits = [&](int a, int b) {
      for (int j = 0; j < 12; j++) lk[a][j] = Min(lk[a][j] + lk[b][j], kCsInf);
    };
    for (int ms = 0; ms < nb;) {  // neighbours of the same codebook joined
      int me = ms + 1;
      for (; me < nb && s[ms].book == s[me].book; me++) {
        s[ms].cnt++;
        s[ms].bits += s[me].bits;
        merge_bits(ms, me);
      }
      s[ms].bits += side(s[ms].cnt);
      s[me - 1].start = s[ms].start;
      ms = me;
    }
    int gain[64];
    auto calc = [&](int a, int b) {
      if (s[a].book >= 13 || s[b].book >= 13) return -1;
      int mm = kCsInf;
      for (int j = 0; j < 12; j++) mm = Min(mm, lk[a][j] + lk[b][j]);
      return s[a].bits + s[b].bits - (side(s[a].cnt + s[b].cnt) + mm);
    };
    for (int i = 0; i + s[i].cnt < nb; i += s[i].cnt) gain[i] = calc(i, i + s[i].cnt);
    for (;;) {  // the merge that saves the most, while one saves
      int mg = 0, mx = 0;
      for (int i = 0; i + s[i].cnt < nb; i += s[i].cnt)
        if (gain[i] > mg) mg = gain[i], mx = i;
      if (mg <= 0) break;
      int nx = mx + s[mx].cnt;
      s[mx].cnt += s[nx].cnt;
      s[mx].bits += s[nx].bits - mg;
      merge_bits(mx, nx);
      if (mx) {
        const int last = s[mx - 1].start;
        gain[last] = calc(last, mx);
      }
      nx = mx + s[mx].cnt;
      s[nx - 1].start = s[mx].start;
      if (nx < nb) gain[mx] = calc(mx, nx);
    }
    for (int i = 0; i < nb; i += s[i].cnt) {
      int book = s[i].book, mb;
      if (book < 13) book = best_of(i, book, mb);
      for (int k = i; k < i + s[i].cnt; k++) cb[CS_FDK][k] = (uint8_t)book, st[CS_FDK][k] = k == i;
    }
  }

  void ff() {
    static const uint8_t kOut[15] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14, 15};
    static const uint8_t kRunL[64] = {5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5, 5,
                                      5,  5,  5,  5,  5,  5,  5,  5,  5,  10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
                                      10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 15};
    static const uint8_t kRunS[16] = {3, 3, 3, 3, 3, 3, 3, 6, 6, 6, 6, 6, 6, 6, 6, 9};
    const int run_bits = shortw ? 3 : 5;
    auto rvb = [&](int run) { return shortw ? kRunS[Min(run, 15)] : kRunL[Min(run, 63)]; };
    struct P {
      int cost, prev, run;
    } path[65][15];
    for (int c = 0; c < 15; c++) path[0][c] = P{run_bits + 4, -1, 0};
    int next_minbits = kCsInf, next_mincb = 0;
    for (int b = 0; b < nb; b++) {
      if (kind[b] == 0) {  // (a band the quantizer zeroed)
        int stay = path[b][0].cost;
        const int get = next_minbits + run_bits + 4;
        if (rvb(path[b][0].run) != rvb(path[b][0].run + 1)) stay += run_bits;
        if (get < stay)
          path[b + 1][0] = P{get, next_mincb, 1};
        else
          path[b + 1][0] = P{stay, 0, path[b][0].run + 1};
        next_minbits = path[b + 1][0].cost, next_mincb = 0;
        for (int c = 1; c < 15; c++) path[b + 1][c] = P{61450, -1, 0};
        continue;
      }
      const int minbits = next_minbits, mincb = next_mincb;
      const int startcb = kind[b] >= 13 ? kind[b] - 1 : minb[b];
      next_minbits = kCsInf, next_mincb = 0;
      for (int c = 0; c < 15; c++) {
        const int bits_c = c < startcb || (c >= 12 && kind[b] != kOut[c]) || (c < 12 && kind[b] >= 13) ? kCsInf
                           : c >= 12                                                                ? 0
                                                                                                    : bits[b][c];
        if (bits_c >= kCsInf) {
          path[b + 1][c] = P{61450, -1, 0};
          continue;
        }
        int stay = path[b][c].cost + bits_c;
        const int get = minbits + bits_c + run_bits + 4;
        if (rvb(path[b][c].run) != rvb(path[b][c].run + 1)) stay += run_bits;
        if (get < stay)
          path[b + 1][c] = P{get, mincb, 1};
        else
          path[b + 1][c] = P{stay, c, path[b][c].run + 1};
        if (path[b + 1][c].cost < next_minbits) next_minbits = path[b + 1][c].cost, next_mincb = c;
      }
    }
    int idx = 0;
    for (int c = 1; c < 15; c++)
      if (path[nb][c].cost < path[nb][idx].cost) idx = c;
    for (int pos = nb; pos > 0;) {
      const int c = idx, run = path[pos][c].run;
      if (run <= 0 || run > pos) {  // (no path: every band its smallest)
        for (int k = 0; k < pos; k++) cb[CS_FF][k] = kind[k] == 1 ? minb[k] : kind[k], st[CS_FF][k] = 1;
        break;
      }
      idx = path[pos - run + 1][c].prev;
      for (int k = pos - run; k < pos; k++) cb[CS_FF][k] = kOut[c], st[CS_FF][k] = k == pos - run;
      pos -= run;
      if (idx < 0 && pos > 0) idx = 0;
    }
  }

  void faac() {
    for (int b = 0; b < nb; b++) {
      int c;
      if (kind[b] != 1)
        c = kind[b];
      else if (minb[b] == 0 || minb[b] == 11)
        c = minb[b] ? 11 : 1;  // (a coded band of zeros: the smallest book)
      else
        c = bits[b][minb[b] + 1] < bits[b][minb[b]] ? minb[b] + 1 : minb[b];
      cb[CS_FAAC][b] = (uint8_t)c;
      st[CS_FAAC][b] = b == 0 || c != cb[CS_FAAC][b - 1];
    }
  }

  void run_all() {
    fdk();
    ff();
    faac();
  }
};

}  // namespace aacpack
