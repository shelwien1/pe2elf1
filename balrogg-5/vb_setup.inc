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

constexpr u32 VB_MAXMODE = 64;   /*  the mode count is 6 bits + 1  */

struct vb_setup {
  vb_book bk[VB_MAXBOOK];   u32 nbk;
  vb_floor fl[VB_MAXFLOOR]; u32 nfl;
  vb_res rs[VB_MAXRES];     u32 nrs;
  vb_map mp[VB_MAXMAP];     u32 nmp;
  u32 nmd;
  u8 blockflag[VB_MAXMODE], mdmap[VB_MAXMODE];

  /*  Validate books before the payload dereferences them.  */
  void check() const {
    u32 i, j;
    Fi(nfl, Fj(VB_MAXCLASS,
      u32 k;
      FATAL_UNLESS(fl[i].cbook[j] < (i32) nbk, "vorbis: floor master book %ld",
                   (long) fl[i].cbook[j]);
      Fk(8,
        FATAL_UNLESS(fl[i].csb[j][k] < (i32) nbk, "vorbis: floor subclass "
                     "book %ld", (long) fl[i].csb[j][k]))));
    Fi(nrs,
      u32 c, p;
      FATAL_UNLESS(rs[i].cbook < nbk, "vorbis: residue class book %lu",
                   (unsigned long) rs[i].cbook);
      FATAL_UNLESS(rs[i].ncl <= 16,
                   "vorbis: %lu residue classifications, limit 16",
                   (unsigned long) rs[i].ncl);
      for (c = 0; c < rs[i].ncl; c++)
        for (p = 0; p < 8; p++) {
          i32 b = rs[i].book[c][p];
          FATAL_UNLESS(b < (i32) nbk, "vorbis: residue book %ld", (long) b);
          FATAL_UNLESS(b < 0 || bk[b].look == 1, "vorbis: residue book %ld has "
                       "no lookup table", (long) b);
        });
    Fi(nmd, FATAL_UNLESS(mdmap[i] < nmp, "vorbis: mode %lu names mapping "
                            "%d", (unsigned long) i, mdmap[i]));
    Fi(nmp, Fj(mp[i].sub,
      FATAL_UNLESS(mp[i].fl[j] < nfl, "vorbis: submap floor %d", mp[i].fl[j]);
      FATAL_UNLESS(mp[i].rs[j] < nrs, "vorbis: submap residue %d", mp[i].rs[j])));
  }
};

/*  A link owns exactly one setup, so one table is enough.  */
static vb_setup vb_su;
