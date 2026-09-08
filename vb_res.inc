
constexpr u32 VB_MAXRES = 64;   /*  the residue count is 6 bits + 1  */
constexpr u32 VB_MAXRCL = 64;   /*  classifications are 6 bits + 1  */

struct vb_res {
  u32 type, beg, end, psz, ncl, cbook;
  u8 casc[VB_MAXRCL];
  i32 book[VB_MAXRCL][8];
};
