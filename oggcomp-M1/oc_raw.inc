//  Bytes the parser could not place: a tag in front of the first page, junk
//  between or after pages, the payload of a page that is not Vorbis, the
//  padding of a packet that has some, a file that is not Ogg at all.  Two
//  counters, order 0 and order 1 -- the bit tree through the current byte
//  picks the first, the previous byte on top of that picks the second --
//  mixed by a weight set the bit tree picks too, so the order-1 table only
//  gets a say once it has seen something.  That is what a compressor gets
//  on text before it starts trying, and honest on a JPEG: about eight bits
//  a byte, where the order-1 table alone would spend 4% learning 65536
//  contexts it never sees twice.  It is one model, not an IDX family, and
//  its rates are three numbers, because everything it codes is something
//  oggcomp would rather not be coding; the day that changes, this is where
//  a real model goes.
constexpr int RAW_RATE0 = 250;
constexpr int RAW_RATE1 = 60;
constexpr int RAW_LR = 24;
static cm_cnt raw_o0[256];
static cm_cnt raw_o1[256][256];
static i32 raw_w[256 * 3];
static u8 raw_wc[256];
static cm_mix<3> raw_mx;
static u32 raw_prev;
static void oc_raw_init(void) { raw_mx.init(raw_w, raw_wc, 256); }
static u32 oc_raw(u32 b) {
  cm_cnt *row = raw_o1[raw_prev];
  u32 node = 1;
  int i;
  tc_stage = STG_RAW;
  for(i = 7; i >= 0; i--) {
    cm_cnt c0 = raw_o0[node], c1 = row[node];
    int bit;
    raw_mx.set(0, cm_stretch(c0.P()));
    raw_mx.set(1, c1.t ? cm_stretch(c1.P()) : 0);
    raw_mx.set(2, 256);
    bit = tc_bit(raw_mx.mix(node), (int)(tc_enc && ((b >> i) & 1)));
    raw_mx.upd(bit, RAW_LR, 0);
    c0.upd(bit, RAW_RATE0, 0);
    c1.upd(bit, RAW_RATE1, 0);
    raw_o0[node] = c0;
    row[node] = c1;
    node = node * 2 + (u32)bit;
  }
  raw_prev = node & 255;
  if(tc_verbose)
    tc_syms[STG_RAW]++;
  return node & 255;
}
