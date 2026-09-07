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

constexpr u32 VB_MAXRES = 64;   /*  the residue count is 6 bits + 1  */
constexpr u32 VB_MAXRCL = 64;   /*  classifications are 6 bits + 1  */

struct vb_res {
  u32 type, beg, end, psz, ncl, cbook;
  u8 casc[VB_MAXRCL];
  i32 book[VB_MAXRCL][8];
};
