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

struct vb_ctx {
  vb_info i;
  vb_setup * cur;             /*  the setup this link's audio runs on  */

  void init() {
    memset(&i, 0, sizeof i);
    cur = nullptr;
    vb_used = 0;
  }

  /*  Each link carries its own three header packets, so the previous link's
      setup is never referenced again.  */
  void link() { cur = nullptr;  vb_used = 0; }
};
