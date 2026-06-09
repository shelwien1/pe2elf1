// zipline_axi.h — field accessors for the flattened struct ports shared by the
// Project-Zipline XP10 engines (decoder cr_xp10_decomp, Huffman encoder
// cr_huf_comp, ...). All driven over Verilator packed-vector ports.
//
//   axi4s_dp_bus_t (83b):  [63:0] tdata | [71:64] tuser | [79:72] tstrb
//                          | [80] tid | [81] tlast | [82] tvalid
//   axi4s_dp_rdy_t (1b):   tready
//   rbus_ring_t   (88b):   [1] ack | [0] err_ack | [33:2] rd_data | [34] rd_strb
//                          | [66:35] wr_data | [67] wr_strb | [87:68] addr
#ifndef ZIPLINE_AXI_H
#define ZIPLINE_AXI_H
#include <cstdint>
typedef uint64_t qword;
typedef uint32_t uint;

// packed-vector bit helpers (LSB = bit 0 of word[0])
static inline qword vget(const uint* w, int hi, int lo) {
  qword v = 0;
  for (int b = hi; b >= lo; --b) v = (v << 1) | ((w[b >> 5] >> (b & 31)) & 1u);
  return v;
}
static inline void vset(uint* w, int hi, int lo, qword val) {
  for (int b = lo; b <= hi; ++b) {
    uint bit = (uint)((val >> (b - lo)) & 1u);
    if (bit) w[b >> 5] |=  (1u << (b & 31));
    else     w[b >> 5] &= ~(1u << (b & 31));
  }
}

struct DpBus { // axi4s_dp_bus_t
  static void set(uint* w, qword tdata, uint tuser, uint tstrb,
                  uint tid, uint tlast, uint tvalid) {
    vset(w,63,0,tdata); vset(w,71,64,tuser); vset(w,79,72,tstrb);
    vset(w,80,80,tid);  vset(w,81,81,tlast); vset(w,82,82,tvalid);
  }
  static qword tdata (const uint* w){ return vget(w,63,0); }
  static uint  tuser (const uint* w){ return (uint)vget(w,71,64); }
  static uint  tstrb (const uint* w){ return (uint)vget(w,79,72); }
  static uint  tid   (const uint* w){ return (uint)vget(w,80,80); }
  static uint  tlast (const uint* w){ return (uint)vget(w,81,81); }
  static uint  tvalid(const uint* w){ return (uint)vget(w,82,82); }
};

struct RbusRing { // rbus_ring_t
  static void set(uint* w, uint addr, uint wr_strb, uint wr_data,
                  uint rd_strb, uint rd_data, uint ack, uint err_ack) {
    vset(w,87,68,addr); vset(w,67,67,wr_strb); vset(w,66,35,wr_data);
    vset(w,34,34,rd_strb); vset(w,33,2,rd_data); vset(w,1,1,ack); vset(w,0,0,err_ack);
  }
  static uint addr   (const uint* w){ return (uint)vget(w,87,68); }
  static uint wr_strb(const uint* w){ return (uint)vget(w,67,67); }
  static uint wr_data(const uint* w){ return (uint)vget(w,66,35); }
  static uint rd_strb(const uint* w){ return (uint)vget(w,34,34); }
  static uint rd_data(const uint* w){ return (uint)vget(w,33,2); }
  static uint ack    (const uint* w){ return (uint)vget(w,1,1); }
  static uint err_ack(const uint* w){ return (uint)vget(w,0,0); }
};
#endif
