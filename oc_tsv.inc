/*  The record-stream sink: every value goes out as the (tag, value) record
    balrogg has always written, and comes back from one.  The tags and their
    order are exactly what io.inc wrote before it had a sink, so the TSV a
    stream produces is byte for byte what it was -- which is what makes this
    the oracle oc_model is checked against (OGGCOMP-PLAN.md sec.6).  */
/*  The page header fields as records, as ogg_page::put and get wrote them.  */

static void oc_page_put(const ogg_page & p, tsv & t) {
  int i;
  t.put("page.type", p.type);
  t.put("page.granlo", p.glo);
  t.put("page.granhi", p.ghi);
  t.put("page.serial", p.serial);
  t.put("page.seq", p.seq);
  t.put("page.npkt", p.np);
  Fi(p.np, t.put("page.plen", p.plen[i]));
  if (p.np && !(p.plen[p.np - 1] % OGG_MAXSEG)) t.put("page.tail", p.tail);
}

static void oc_page_get(ogg_page & p, tsv & t) {
  int i;
  p.type = (u8) t.get_u("page.type", 0x100);
  p.glo = t.get_u("page.granlo", (u64) 0x100000000);
  p.ghi = t.get_u("page.granhi", (u64) 0x100000000);
  p.serial = t.get_u("page.serial", (u64) 0x100000000);
  p.seq = t.get_u("page.seq", (u64) 0x100000000);
  p.np = (int) t.get_u("page.npkt", OGG_MAXSEG + 1);
  Fi(p.np, p.plen[i] = t.get_u("page.plen",
                           (u64) OGG_MAXSEG * OGG_MAXSEG + 1));
  p.tail = 0;
  if (p.np && !(p.plen[p.np - 1] % OGG_MAXSEG)) p.tail = (int) t.get_u("page.tail", 2);
  p.pack();
}


struct oc_tsv {
  tsv t;
  int enc;

  void create(const char * path) { t.create(path);  enc = 1; }
  void open(const char * path) { t.open(path);  enc = 0; }
  void close() { t.close(); }

  u32 link(u32 more) {
    if (enc) { t.put("link.more", more);  return more; }
    return t.get_u("link.more", 2);
  }
  void link_begin() {}

  void page(ogg_page & p, int cont) {
    (void) cont;
    if (enc) oc_page_put(p, t);  else oc_page_get(p, t);
  }

  u32 spill(u32 n) {
    if (enc) { t.put("page.spill", (i64) n);  return n; }
    return t.get_u("page.spill", (u64) MAXPAY * OGG_MAXSEG + 1);
  }

  u32 hdr(const char * tag, u32 x, u64 hi) {
    if (enc) { t.put(tag, x);  return x; }
    return t.get_u(tag, hi);
  }
  void setup_done(vb_ctx & v) { (void) v; }

  u32 mode(u32 md, u32 nmd) {
    if (enc) { t.put("aud.mode", md);  return md; }
    return t.get_u("aud.mode", nmd);
  }
  u32 wprev(u32 w) {
    if (enc) { t.put("aud.wprev", w);  return w; }
    return t.get_u("aud.wprev", 2);
  }
  u32 wnext(u32 w) {
    if (enc) { t.put("aud.wnext", w);  return w; }
    return t.get_u("aud.wnext", 2);
  }

  /*  The posts go out in ascending X, the order the packet holds them in;
      `y` is indexed by post number, as the packet is.  */
  void floor(u32 k, u32 fno, vb_floor * f, u32 * y, int & used) {
    u32 i;
    (void) k;  (void) fno;
    if (enc) t.put("flr.used", used);
    else used = (int) t.get_u("flr.used", 2);
    if (!used) return;
    Fi(f->posts, u32 p = f->srt[i];
                 if (enc) t.put("flr.y", y[p]);
                 else y[p] = t.get_u("flr.y", (u64) 0x100000000));
  }

  void residue_begin(u32 rno, vb_res * r, const u8 * nz, u32 nch, u32 n) {
    (void) rno;  (void) r;  (void) nz;  (void) nch;  (void) n;
  }
  u32 cls(u32 rno, u32 j, u32 pc, u32 k, u32 c) {
    (void) rno;  (void) j;  (void) pc;  (void) k;
    if (enc) { t.put("res.class", c);  return c; }
    return t.get_u("res.class", 16);
  }
  void part_begin(u32 rno, u32 pass, u32 j, u32 pc, vb_res * r, vb_book * bk,
                  u32 bn, u32 c) {
    (void) rno;  (void) pass;  (void) j;  (void) pc;  (void) r;  (void) bk;
    (void) bn;  (void) c;
  }
  void vec_begin() {}
  i64 digit(i64 d) {
    if (enc) { t.put("res.digit", d);  return d; }
    return t.get("res.digit");
  }
};
