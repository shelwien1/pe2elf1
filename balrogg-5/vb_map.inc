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

constexpr u32 VB_MAXMAP = 64;   /*  the mapping count is 6 bits + 1  */
constexpr u32 VB_MAXSUB = 16;   /*  submaps per mapping are 4 bits + 1  */
constexpr u32 VB_MAXCH  = 256;  /*  the channel count is 8 bits  */

struct vb_map {
  u32 sub, nstep;
  u8 mag[VB_MAXCH], ang[VB_MAXCH];
  u8 mux[VB_MAXCH], fl[VB_MAXSUB], rs[VB_MAXSUB];
};
