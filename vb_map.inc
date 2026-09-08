
constexpr u32 VB_MAXMAP = 64;   /*  the mapping count is 6 bits + 1  */
constexpr u32 VB_MAXSUB = 16;   /*  submaps per mapping are 4 bits + 1  */
constexpr u32 VB_MAXCH  = 256;  /*  the channel count is 8 bits  */

struct vb_map {
  u32 sub, nstep;
  u8 mag[VB_MAXCH], ang[VB_MAXCH];
  u8 mux[VB_MAXCH], fl[VB_MAXSUB], rs[VB_MAXSUB];
};
