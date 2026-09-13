template <int f_DEC> int oc_model<f_DEC>::tagid(const char *tag) {
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
template <int f_DEC> i64 oc_model<f_DEC>::hdrval(int id, i64 x) {
  tcx v;
  i64 pr = tc_tlast[id];
  tc_make_hdr(id, tc_qlog(tc_tlast[id]), tc_qlog(tc_tlast2[id]), tc_qlog(tc_runpos), (int)(tc_tlast[id] & 255), v);
  x = fam_hdr.codes(v, ENC ? x - pr : 0, (u32)id) + pr;
  tc_tlast2[id] = tc_tlast[id];
  tc_tlast[id] = x;
  return x;
}
template <int f_DEC> u32 oc_model<f_DEC>::hdr(const char *tag, u32 x, u64 hi) {
  i64 v;
  tc_stage = STG_HDR;
  v = hdrval(tagid(tag), ENC ? (i64)x : 0);
  if(!ENC)
    FATAL_UNLESS(v >= 0 && (u64)v < hi, "coded stream: %s = %" PRId64 " is out of range", tag, v);
  return (u32)v;
}
