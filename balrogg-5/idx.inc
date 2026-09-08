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

/*  IDX: contexts declared rather than written.

    mp3c does not write its context arithmetic by hand.  A `.idx` file names,
    for each context index, a list of variables and a threshold pattern each,
    `idx2inc.pl` turns that into a mixed-radix computation, and an optimizer
    hill-climbs the patterns against a corpus.  IDX/IDX-FORMAT.md in that tree
    is the specification; this is a reader for the part of it a single-file
    program needs.

    The difference is that nothing is generated.  IDX has two shapes -- a
    tuning build where every threshold is a live object and a shipping build
    where the same thresholds are folded to literals -- and this is the tuning
    shape, kept, with the declaration parsed at startup instead of compiled
    in.  A balrogg stream costs milliseconds to walk and the parse is a few
    hundred microseconds once, so the folded shape would buy nothing that
    matters, and what it would cost is a perl dependency and a generated file
    that can go stale against its source.  What is kept is the property the
    format exists for: the contexts are data, in one place, and changing one
    is changing a string.

    The subset read here:

      Prefix <name>                 recorded, unused
      Debug <0|1>  Const <0|1>      accepted and ignored -- see above
      Index <name>                  begins an index
       <tag>: <var>, <base>!<pat>   threshold factor: a threshold at j+base-1
                                    for each '1' at position j of <pat>
       <tag>: <var>, &<pat>         bitmask factor: keep the '1' positions of
                                    <var>, MSB of the pattern first, packed
       ADD <mult>: <var>            dense factor: ctx = ctx*<mult> + <var>
      Number <name>, <mult>, <base>!<pat>
                                    a tunable integer: (pat as binary + base)
                                    * mult

    Not read: the `+` chunked-threshold form, `Rate`/`Rate1`, `!` and `-` line
    modifiers, `Table`, and the template markers -- all of which exist to
    serve the generator.  A `!` line would be a knob frozen against an
    optimizer that is not here; the chunked form builds a band map out of
    24-bit slices, and this program's band maps are computed in C++ and
    handed in already coarse, which is what mp3c does for `band_j` too.

    A variable is named, not positional: each index is declared in C++ with
    the vector of variable names its family supplies, and a factor line
    resolves its <var> against that.  A name the family does not supply is an
    error at startup rather than a silently wrong context.  */

constexpr int IDX_MAXTH   = 48;   /*  thresholds in one factor  */
constexpr int IDX_MAXFAC  = 16;   /*  factors in one index  */
constexpr int IDX_MAXIDX  = 32;   /*  indices in one declaration  */
constexpr int IDX_MAXNUM  = 32;   /*  Number parameters  */
constexpr int IDX_NAME    = 32;
constexpr u32 IDX_MAXVOL  = 1u << 22;   /*  rows one index may address  */

struct idx_fac {
  u8 kind;                    /*  0 threshold, 1 mask, 2 dense  */
  u8 slot;                    /*  which of the family's variables  */
  u8 nth;                     /*  thresholds, or kept bits  */
  u8 lutn;                    /*  values the lookup covers; 0 = no lookup  */
  i32 size;                   /*  buckets this factor contributes  */
  i32 th[IDX_MAXTH];
  u8 bit[32];
  u8 lut[256];

  INLINE i32 q(i32 x) const {
    if (x < 0) x = 0;
    if (lutn) return x < (i32) lutn ? (i32) lut[x] : (i32) nth;
    { i32 k = 0, j;
      for (j = 0; j < (i32) nth; j++) k += (x > th[j]);
      return k; }
  }
};

struct idx_index {
  char name[IDX_NAME];
  idx_fac f[IDX_MAXFAC];
  int n;
  u32 vol;                    /*  rows: the product of the factor sizes  */

  INLINE u32 build(const i32 * v) const {
    u32 c = 0;
    int i;
    for (i = 0; i < n; i++) {
      const idx_fac & a = f[i];
      i32 x = v[a.slot];
      if (a.kind == 0) c = c * (u32) a.size + (u32) a.q(x);
      else if (a.kind == 1) {
        u32 m = 0;
        int j;
        for (j = 0; j < (int) a.nth; j++) m = m * 2 + (((u32) x >> a.bit[j]) & 1);
        c = c * (u32) a.size + m;
      } else {
        if (x < 0) x = 0;
        if (x >= a.size) x = a.size - 1;
        c = c * (u32) a.size + (u32) x;
      }
    }
    return c;
  }
};

struct idx_num { char name[IDX_NAME];  i32 value; };

/*  One parsed declaration.  The variable vectors are registered before the
    parse, by family: an index called `dig_a` takes the vector registered for
    `dig`, so a family's four indices cannot disagree about what `q1` means.  */
struct idx_decl {
  idx_index ix[IDX_MAXIDX];
  int nix;
  idx_num nm[IDX_MAXNUM];
  int nnm;
  struct { char fam[IDX_NAME];  const char * const * vars; } fv[IDX_MAXIDX];
  int nfv;
  char prefix[IDX_NAME];

  void init() { nix = 0;  nnm = 0;  nfv = 0;  prefix[0] = 0; }

  void family(const char * fam, const char * const * vars) {
    FATAL_UNLESS(nfv < IDX_MAXIDX, "too many IDX families");
    snprintf(fv[nfv].fam, IDX_NAME, "%s", fam);
    fv[nfv].vars = vars;
    nfv++;
  }

  const idx_index * index(const char * nm_) const {
    int i;
    for (i = 0; i < nix; i++) if (!strcmp(ix[i].name, nm_)) return ix + i;
    FATAL("IDX declares no index '%s'", nm_);
    return nullptr;
  }
  i32 number(const char * nm_) const {
    int i;
    for (i = 0; i < nnm; i++) if (!strcmp(nm[i].name, nm_)) return nm[i].value;
    FATAL("IDX declares no number '%s'", nm_);
    return 0;
  }

  void parse(const char * text);

private:
  const char * const * varsfor(const char * ixname) const {
    char fam[IDX_NAME];
    const char * u = strchr(ixname, '_');
    int i;
    sz n = u ? (sz) (u - ixname) : strlen(ixname);
    FATAL_UNLESS(n < IDX_NAME, "IDX index name '%s' is too long", ixname);
    memcpy(fam, ixname, n);  fam[n] = 0;
    for (i = 0; i < nfv; i++) if (!strcmp(fv[i].fam, fam)) return fv[i].vars;
    FATAL("IDX index '%s' has no registered variables", ixname);
    return nullptr;
  }
};

/*  A line at a time, with the tail of each line already trimmed.  */
void idx_decl::parse(const char * text) {
  const char * p = text;
  idx_index * cur = nullptr;
  const char * const * vars = nullptr;
  u32 lineno = 0;

  while (*p) {
    char line[256], * s, * e;
    sz n = 0;
    while (*p && *p != '\n') { if (n + 1 < sizeof line) line[n++] = *p;  p++; }
    if (*p) p++;
    line[n] = 0;
    lineno++;
    { char * h = strchr(line, '#');  if (h) *h = 0; }
    s = line;
    while (*s == ' ' || *s == '\t' || *s == '\r') s++;
    e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r')) e--;
    *e = 0;
    if (!*s) continue;

    if (!strncmp(s, "Prefix ", 7)) {
      snprintf(prefix, IDX_NAME, "%s", s + 7);
      continue;
    }
    if (!strncmp(s, "Debug ", 6) || !strncmp(s, "Const ", 6)) continue;

    if (!strncmp(s, "Index ", 6)) {
      FATAL_UNLESS(nix < IDX_MAXIDX, "IDX line %" PRIu64 ": too many indices",
                   (u64) lineno);
      cur = ix + nix++;
      snprintf(cur->name, IDX_NAME, "%s", s + 6);
      cur->n = 0;  cur->vol = 1;
      vars = varsfor(cur->name);
      continue;
    }

    if (!strncmp(s, "Number ", 7)) {
      /*  Number <name>, <mult>, <base>!<pattern>  */
      char nmbuf[IDX_NAME];
      const char * c1 = strchr(s + 7, ',');
      const char * c2 = c1 ? strchr(c1 + 1, ',') : nullptr;
      const char * bang = c2 ? strchr(c2 + 1, '!') : nullptr;
      i64 mult, base, val = 0;
      const char * q;
      sz ln;
      FATAL_UNLESS(c1 && c2 && bang, "IDX line %" PRIu64 ": malformed Number",
                   (u64) lineno);
      ln = (sz) (c1 - (s + 7));
      while (ln && (s[7 + ln - 1] == ' ')) ln--;
      FATAL_UNLESS(ln < IDX_NAME, "IDX line %" PRIu64 ": Number name too long",
                   (u64) lineno);
      memcpy(nmbuf, s + 7, ln);  nmbuf[ln] = 0;
      mult = strtoll(c1 + 1, nullptr, 10);
      base = strtoll(c2 + 1, nullptr, 10);
      for (q = bang + 1; *q == '0' || *q == '1'; q++) val = val * 2 + (*q - '0');
      FATAL_UNLESS(nnm < IDX_MAXNUM, "IDX line %" PRIu64 ": too many Numbers",
                   (u64) lineno);
      snprintf(nm[nnm].name, IDX_NAME, "%s", nmbuf);
      nm[nnm].value = (i32) ((val + base) * mult);
      nnm++;
      continue;
    }

    FATAL_UNLESS(cur != nullptr, "IDX line %" PRIu64 ": factor before any Index",
                 (u64) lineno);
    FATAL_UNLESS(cur->n < IDX_MAXFAC, "IDX line %" PRIu64 ": too many factors",
                 (u64) lineno);

    { idx_fac & a = cur->f[cur->n];
      char vbuf[IDX_NAME];
      const char * vn;
      const char * arg;
      int i;

      memset(&a, 0, sizeof a);

      if (!strncmp(s, "ADD ", 4)) {
        /*  ADD <mult>: <var>  */
        const char * col = strchr(s, ':');
        FATAL_UNLESS(col != nullptr, "IDX line %" PRIu64 ": malformed ADD",
                     (u64) lineno);
        a.kind = 2;
        a.size = (i32) strtoll(s + 4, nullptr, 10);
        FATAL_UNLESS(a.size > 0, "IDX line %" PRIu64 ": ADD multiplier",
                     (u64) lineno);
        vn = col + 1;
        while (*vn == ' ') vn++;
        arg = nullptr;
      } else {
        /*  <tag>: <var>, <base>!<pattern>  or  <tag>: <var>, &<pattern>  */
        const char * col = strchr(s, ':');
        const char * com;
        sz ln;
        FATAL_UNLESS(col != nullptr, "IDX line %" PRIu64 ": '%s' is not a factor",
                     (u64) lineno, s);
        com = strchr(col + 1, ',');
        FATAL_UNLESS(com != nullptr, "IDX line %" PRIu64 ": factor has no pattern",
                     (u64) lineno);
        vn = col + 1;
        while (*vn == ' ') vn++;
        ln = (sz) (com - vn);
        while (ln && vn[ln - 1] == ' ') ln--;
        FATAL_UNLESS(ln && ln < IDX_NAME, "IDX line %" PRIu64 ": variable name",
                     (u64) lineno);
        memcpy(vbuf, vn, ln);  vbuf[ln] = 0;
        vn = vbuf;
        arg = com + 1;
        while (*arg == ' ') arg++;
      }

      /*  resolve the variable against the family's vector  */
      { int slot = -1;
        for (i = 0; vars[i]; i++) if (!strcmp(vars[i], vn)) { slot = i;  break; }
        FATAL_UNLESS(slot >= 0, "IDX line %" PRIu64 ": index '%s' has no "
                     "variable '%s'", (u64) lineno, cur->name, vn);
        a.slot = (u8) slot; }

      if (a.kind != 2) {
        if (*arg == '&') {
          const char * q = arg + 1;
          sz w = strlen(q), j;
          a.kind = 1;
          FATAL_UNLESS(w && w <= 32, "IDX line %" PRIu64 ": mask width",
                       (u64) lineno);
          for (j = 0; j < w; j++)
            if (q[j] == '1') a.bit[a.nth++] = (u8) (w - 1 - j);
            else FATAL_UNLESS(q[j] == '0', "IDX line %" PRIu64 ": mask digit",
                              (u64) lineno);
          a.size = 1 << a.nth;
        } else {
          const char * bang = strchr(arg, '!');
          i64 base;
          const char * q;
          FATAL_UNLESS(bang != nullptr, "IDX line %" PRIu64 ": factor has no '!'",
                       (u64) lineno);
          base = strtoll(arg, nullptr, 10);
          a.kind = 0;
          for (q = bang + 1; *q; q++) {
            if (*q == '1') {
              FATAL_UNLESS(a.nth < IDX_MAXTH, "IDX line %" PRIu64 ": too many "
                           "thresholds", (u64) lineno);
              a.th[a.nth++] = (i32) (base + (q - bang - 1) - 1);
            } else FATAL_UNLESS(*q == '0', "IDX line %" PRIu64 ": pattern digit",
                                (u64) lineno);
          }
          a.size = a.nth + 1;
          /*  Thresholds are written in ascending order by construction, so a
              small one can be answered from a table instead of a scan.  */
          if (a.nth && a.th[a.nth - 1] < 254 && a.th[0] >= 0) {
            i32 x, k = 0;
            a.lutn = (u8) (a.th[a.nth - 1] + 2);
            for (x = 0; x < (i32) a.lutn; x++) {
              while (k < (i32) a.nth && x > a.th[k]) k++;
              a.lut[x] = (u8) k;
            }
          }
        }
      }

      /*  A factor with one bucket says nothing; keeping it would cost a
          multiply by 1 on every value.  It stays in the declaration -- that
          a variable was tried and left at zero is worth reading -- and is
          dropped here.  */
      if (a.size > 1) {
        FATAL_UNLESS(cur->vol <= IDX_MAXVOL / (u32) a.size,
                     "IDX index '%s' addresses more than %" PRIu64 " rows",
                     cur->name, (u64) IDX_MAXVOL);
        cur->vol *= (u32) a.size;
        cur->n++;
      }
    }
  }
}
