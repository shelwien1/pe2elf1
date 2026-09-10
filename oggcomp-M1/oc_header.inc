constexpr sz OC_TAGMAX = 31;
static char tc_tagbuf[TC_MAXTAG * (OC_TAGMAX + 1)];
static const char *tc_tag[TC_MAXTAG];
static u32 tc_ntag;
static int tc_lasttag = -1;
static i64 tc_tlast[TC_MAXTAG], tc_tlast2[TC_MAXTAG];
static i64 tc_runpos;
static int tc_tagid(const char *tag) {
  u32 i;
  int id = -1;
  for(i = 0; i < tc_ntag; i++)
    if(!strcmp(tc_tag[i], tag)) {
      id = (int)i;
      break;
    }
  if(id < 0) {
    char *dst = tc_tagbuf + (sz)tc_ntag * (OC_TAGMAX + 1);
    FATAL_UNLESS(tc_ntag < TC_MAXTAG, "the reader asks for more than %d tags", TC_MAXTAG);
    snprintf(dst, OC_TAGMAX + 1, "%s", tag);
    tc_tag[tc_ntag] = dst;
    id = (int)tc_ntag++;
  }
  tc_runpos = (id == tc_lasttag) ? tc_runpos + 1 : 0;
  tc_lasttag = id;
  return id;
}
static i64 tc_hdrval(int id, i64 x) {
  tcx v;
  i64 pr = tc_tlast[id];
  tc_make_hdr(id, tc_qlog(tc_tlast[id]), tc_qlog(tc_tlast2[id]), tc_qlog(tc_runpos), (int)(tc_tlast[id] & 255), v);
  x = fam_hdr.codes(v, tc_enc ? x - pr : 0, (u32)id) + pr;
  tc_tlast2[id] = tc_tlast[id];
  tc_tlast[id] = x;
  return x;
}
static u32 oc_hdr(const char *tag, u32 x, u64 hi) {
  i64 v;
  tc_stage = STG_HDR;
  v = tc_hdrval(tc_tagid(tag), tc_enc ? (i64)x : 0);
  if(!tc_enc)
    FATAL_UNLESS(v >= 0 && (u64)v < hi, "coded stream: %s = %" PRId64 " is out of range", tag, v);
  return (u32)v;
}
