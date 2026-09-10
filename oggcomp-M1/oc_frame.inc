template <int f_DEC> u32 oc_model<f_DEC>::mode(u32 md, u32 nmd) {
  tc_stage = STG_AUD;
  md = (u32)auxc(F_MODE, (i32)tc_prevmode, ENC ? (i64)md : 0, 0);
  FATAL_UNLESS(md < nmd, "coded stream: audio names mode %" PRIu64, (u64)md);
  tc_prevmode = md;
  hist.tc_blk = vb->cur->blockflag[md];
  oc_nextW = hist.tc_blk;
  if(!hist.tc_blk)
    tc_prevW = 0;
  return md;
}
template <int f_DEC> u32 oc_model<f_DEC>::wprev(u32 w) {
  w = (u32)auxc(F_WPREV, (i32)tc_prevW, ENC ? (i64)w : 0, 0);
  FATAL_UNLESS(w < 2, "coded stream: aud.wprev is %u", w);
  oc_wp = w;
  return w;
}
template <int f_DEC> u32 oc_model<f_DEC>::wnext(u32 w) {
  w = (u32)auxc(F_WNEXT, (i32)oc_wp, ENC ? (i64)w : 0, 0);
  FATAL_UNLESS(w < 2, "coded stream: aud.wnext is %u", w);
  tc_prevW = oc_nextW;
  return w;
}
//  `bos` is what the caller expects of the type's beginning-of-stream bit:
//  true while every page of the group so far has carried it, which is the
//  first page, and after it the further streams of a multiplexed file.
template <int f_DEC> void oc_model<f_DEC>::page(ogg_page &p, int cont, int bos) {
  u32 i;
  i64 g;
  u32 seq = oc_seq++;
  tc_stage = STG_PAGE;
  {
    u32 want = (u32)((bos ? 2 : 0) | (cont ? 1 : 0));
    i64 d = auxc(F_PGTYPE, (i32)(want * 4 + (pg_prevtype & 3)), ENC ? (i64)p.type - (i64)want : 0, 1);
    if(!ENC) {
      FATAL_UNLESS(d + (i64)want >= 0 && d + (i64)want < 0x100, "coded stream: page type %" PRId64, d + (i64)want);
      p.type = (u8)(d + (i64)want);
    }
    pg_prevtype = p.type;
  }
  g = ENC ? (i64)(((u64)p.ghi << 32) | p.glo) : 0;
  g = auxc(F_GRAN, tc_qlog(tc_last2[F_GRAN]), ENC ? g - pg_prev : 0, 1) + pg_prev;
  pg_prev = g;
  if(!ENC) {
    p.glo = (u32)((u64)g & 0xFFFFFFFFu);
    p.ghi = (u32)((u64)g >> 32);
  }
  {
    i64 d = auxc(F_SERIAL, bos, ENC ? (i64)p.serial - (i64)pg_prevser : 0, 1);
    if(!ENC)
      p.serial = (u32)((i64)pg_prevser + d);
    pg_prevser = p.serial;
  }
  {
    i64 d = auxc(F_SEQ, 0, ENC ? (i64)p.seq - (i64)seq : 0, 1);
    if(!ENC)
      p.seq = (u32)((i64)seq + d);
  }
  {
    i64 np = aux(F_NPKT, ENC ? (i64)p.np : 0);
    FATAL_UNLESS(np >= 0 && np <= OGG_MAXSEG, "coded stream: page holds %" PRId64 " packets", np);
    p.np = (int)np;
  }
  {
    i64 prev = 0;
    for(i = 0; i < (u32)p.np; i++) {
      i64 v = auxc(F_PLEN, tc_qlog(prev), ENC ? (i64)p.plen[i] - prev : 0, 1) + prev;
      FATAL_UNLESS(v >= 0 && v <= (i64)OGG_MAXSEG * OGG_MAXSEG, "coded stream: packet length %" PRId64, v);
      p.plen[i] = (u32)v;
      prev = v;
    }
  }
  if(p.np && !(p.plen[p.np - 1] % OGG_MAXSEG)) {
    i64 v = aux(F_TAIL, ENC ? (i64)p.tail : 0);
    FATAL_UNLESS(v >= 0 && v < 2, "coded stream: page tail %" PRId64, v);
    p.tail = (int)v;
  } else
    p.tail = 0;
  if(!ENC)
    p.pack();
}
template <int f_DEC> u32 oc_model<f_DEC>::link(u32 more) {
  i64 m;
  tc_stage = STG_PAGE;
  m = aux(F_MORE, ENC ? (i64)more : 0);
  FATAL_UNLESS(m >= 0 && m < 2, "coded stream: link.more is %" PRId64, m);
  return (u32)m;
}
template <int f_DEC> void oc_model<f_DEC>::link_begin() {
  pg_prev = 0;
  tc_prevW = 0;
  tc_prevmode = 0;
  pg_prevtype = 0;
  pg_prevser = 0;
  oc_seq = 0;
  memset(tc_used, 0, sizeof tc_used);
}
//  The v3 structure: a run of bytes that are not a page, coded in chunks
//  of at most MAXPAY and ended by a zero; whether the group has another
//  page, in the context of the last page's end-of-stream bit; whether a
//  page's payload is bytes rather than packets, in the context of the last
//  page's answer; whether its stored CRC is the one its content gives; and
//  whether a packet has bits past its last field.  All but the run length
//  are one binary symbol with a nearly certain answer.
template <int f_DEC> u32 oc_model<f_DEC>::rawlen(u32 n) {
  i64 v;
  tc_stage = STG_RAW;
  v = aux(F_RAWLEN, ENC ? (i64)n : 0);
  FATAL_UNLESS(v >= 0 && v <= (i64)MAXPAY, "coded stream: a raw run of %" PRId64, v);
  return (u32)v;
}
template <int f_DEC> u32 oc_model<f_DEC>::more(u32 m, int eos) {
  i64 v;
  tc_stage = STG_PAGE;
  v = auxc(F_PGMORE, eos ? 1 : 0, ENC ? (i64)m : 0, 0);
  FATAL_UNLESS(v >= 0 && v < 2, "coded stream: page.more is %" PRId64, v);
  return (u32)v;
}
template <int f_DEC> u32 oc_model<f_DEC>::pgraw(u32 r, int lastraw) {
  i64 v;
  tc_stage = STG_PAGE;
  v = auxc(F_PGRAW, lastraw ? 1 : 0, ENC ? (i64)r : 0, 0);
  FATAL_UNLESS(v >= 0 && v < 2, "coded stream: page.raw is %" PRId64, v);
  return (u32)v;
}
template <int f_DEC> u32 oc_model<f_DEC>::crcbad(u32 c) {
  i64 v;
  tc_stage = STG_PAGE;
  v = aux(F_CRCBAD, ENC ? (i64)c : 0);
  FATAL_UNLESS(v >= 0 && v < 2, "coded stream: page.crcbad is %" PRId64, v);
  return (u32)v;
}
//  The last thing in the stream: Ogg's CRC-32 over the whole input, 32
//  flat bits.  A stream cut short or with a byte turned decodes to
//  something, and the parser's own checks catch most of that on Vorbis
//  and none of it on bytes; this catches it on either, at four bytes a
//  file.  Flat, because it is one value per file and there is nothing to
//  learn it from.
template <int f_DEC> u32 oc_model<f_DEC>::check(u32 c) {
  u32 v = 0;
  int i;
  tc_stage = STG_PAGE;
  for(i = 31; i >= 0; i--)
    v = v * 2 + (u32)tc_bit<f_DEC>(CM_PHALF, (int)(ENC && ((c >> i) & 1)));
  return v;
}
template <int f_DEC> u32 oc_model<f_DEC>::padnz(u32 z) {
  i64 v;
  tc_stage = STG_AUD;
  v = aux(F_PADNZ, ENC ? (i64)z : 0);
  FATAL_UNLESS(v >= 0 && v < 2, "coded stream: packet.padding is %" PRId64, v);
  return (u32)v;
}
template <int f_DEC> u32 oc_model<f_DEC>::spill(u32 n) {
  i64 ex;
  tc_stage = STG_PAGE;
  ex = aux(F_SPILL, ENC ? (i64)n : 0);
  FATAL_UNLESS(ex >= 0 && ex <= (i64)MAXPAY * OGG_MAXSEG, "coded stream: page spill %" PRId64, ex);
  return (u32)ex;
}
