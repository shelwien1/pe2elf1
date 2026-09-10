static u32 tc_prevW;
static u32 tc_prevmode;
static u32 oc_nextW, oc_wp;
static u32 oc_mode(u32 md, u32 nmd) {
  tc_stage = STG_AUD;
  md = (u32)tc_auxc(F_MODE, (i32)tc_prevmode, tc_enc ? (i64)md : 0, 0);
  FATAL_UNLESS(md < nmd, "coded stream: audio names mode %" PRIu64, (u64)md);
  tc_prevmode = md;
  tc_blk = oc_v->cur->blockflag[md];
  oc_nextW = tc_blk;
  if(!tc_blk)
    tc_prevW = 0;
  return md;
}
static u32 oc_wprev(u32 w) {
  w = (u32)tc_auxc(F_WPREV, (i32)tc_prevW, tc_enc ? (i64)w : 0, 0);
  FATAL_UNLESS(w < 2, "coded stream: aud.wprev is %u", w);
  oc_wp = w;
  return w;
}
static u32 oc_wnext(u32 w) {
  w = (u32)tc_auxc(F_WNEXT, (i32)oc_wp, tc_enc ? (i64)w : 0, 0);
  FATAL_UNLESS(w < 2, "coded stream: aud.wnext is %u", w);
  tc_prevW = oc_nextW;
  return w;
}
static i64 pg_prev;
static u32 pg_prevtype, pg_prevser;
static u32 oc_seq;
static void oc_page(ogg_page &p, int cont) {
  u32 i;
  i64 g;
  u32 seq = oc_seq++;
  int bos = seq == 0;
  tc_stage = STG_PAGE;
  {
    u32 want = (u32)((bos ? 2 : 0) | (cont ? 1 : 0));
    i64 d = tc_auxc(F_PGTYPE, (i32)(want * 4 + (pg_prevtype & 3)), tc_enc ? (i64)p.type - (i64)want : 0, 1);
    if(!tc_enc) {
      FATAL_UNLESS(d + (i64)want >= 0 && d + (i64)want < 0x100, "coded stream: page type %" PRId64, d + (i64)want);
      p.type = (u8)(d + (i64)want);
    }
    pg_prevtype = p.type;
  }
  g = tc_enc ? (i64)(((u64)p.ghi << 32) | p.glo) : 0;
  g = tc_auxc(F_GRAN, tc_qlog(tc_last2[F_GRAN]), tc_enc ? g - pg_prev : 0, 1) + pg_prev;
  pg_prev = g;
  if(!tc_enc) {
    p.glo = (u32)((u64)g & 0xFFFFFFFFu);
    p.ghi = (u32)((u64)g >> 32);
  }
  {
    i64 d = tc_auxc(F_SERIAL, bos, tc_enc ? (i64)p.serial - (i64)pg_prevser : 0, 1);
    if(!tc_enc)
      p.serial = (u32)((i64)pg_prevser + d);
    pg_prevser = p.serial;
  }
  {
    i64 d = tc_auxc(F_SEQ, 0, tc_enc ? (i64)p.seq - (i64)seq : 0, 1);
    if(!tc_enc)
      p.seq = (u32)((i64)seq + d);
  }
  {
    i64 np = tc_aux(F_NPKT, tc_enc ? (i64)p.np : 0);
    FATAL_UNLESS(np >= 0 && np <= OGG_MAXSEG, "coded stream: page holds %" PRId64 " packets", np);
    p.np = (int)np;
  }
  {
    i64 prev = 0;
    for(i = 0; i < (u32)p.np; i++) {
      i64 v = tc_auxc(F_PLEN, tc_qlog(prev), tc_enc ? (i64)p.plen[i] - prev : 0, 1) + prev;
      FATAL_UNLESS(v >= 0 && v <= (i64)OGG_MAXSEG * OGG_MAXSEG, "coded stream: packet length %" PRId64, v);
      p.plen[i] = (u32)v;
      prev = v;
    }
  }
  if(p.np && !(p.plen[p.np - 1] % OGG_MAXSEG)) {
    i64 v = tc_aux(F_TAIL, tc_enc ? (i64)p.tail : 0);
    FATAL_UNLESS(v >= 0 && v < 2, "coded stream: page tail %" PRId64, v);
    p.tail = (int)v;
  } else
    p.tail = 0;
  if(!tc_enc)
    p.pack();
}
static u32 oc_link(u32 more) {
  i64 m;
  tc_stage = STG_PAGE;
  m = tc_aux(F_MORE, tc_enc ? (i64)more : 0);
  FATAL_UNLESS(m >= 0 && m < 2, "coded stream: link.more is %" PRId64, m);
  return (u32)m;
}
static void oc_link_begin(void) {
  pg_prev = 0;
  tc_prevW = 0;
  tc_prevmode = 0;
  pg_prevtype = 0;
  pg_prevser = 0;
  oc_seq = 0;
  memset(tc_used, 0, sizeof tc_used);
}
static u32 oc_spill(u32 n) {
  i64 ex;
  tc_stage = STG_PAGE;
  ex = tc_aux(F_SPILL, tc_enc ? (i64)n : 0);
  FATAL_UNLESS(ex >= 0 && ex <= (i64)MAXPAY * OGG_MAXSEG, "coded stream: page spill %" PRId64, ex);
  return (u32)ex;
}
