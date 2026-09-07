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

constexpr u32 VB_MAXFLOOR = 64;   /*  the floor count is 6 bits + 1  */
constexpr u32 VB_MAXCLASS = 16;   /*  floor 1 class numbers are 4 bits  */
constexpr u32 VB_MAXPART  = 32;   /*  floor 1 partition count is 5 bits  */
constexpr u32 VB_MAXPOST  = 256;  /*  at most 2 + 31*8 = 250, rounded up  */

struct vb_floor {
  u32 parts, mult, quant, posts, rng;
  u8 pcls[VB_MAXPART];
  u8 cdim[VB_MAXCLASS], csub[VB_MAXCLASS];
  i32 cbook[VB_MAXCLASS], csb[VB_MAXCLASS][8];
  u32 x[VB_MAXPOST];
  u8 srt[VB_MAXPOST];         /*  post numbers in ascending X order  */

  /*  Sort Floor 1 posts by X.  */
  void sort() {
    u32 i, j, t;
    Fi(posts,
      srt[i] = (u8) i;
      for (j = i; j > 0 && x[srt[j - 1]] > x[srt[j]]; j--) {
        t = srt[j];  srt[j] = srt[j - 1];  srt[j - 1] = (u8) t;
      });
    Fi(posts, if (i) FATAL_UNLESS(x[srt[i]] != x[srt[i - 1]],
                                     "vorbis: floor X list repeats a value"));
  }
};
