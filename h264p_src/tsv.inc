// The TSV rows as a channel (chan.inc): dumping, the values the walk parses
// become rows; restoring, the rows give the values the walk writes.

namespace h264p {

const int kAbsent = INT_MIN;

// One coded block of coefficients (TSV row "res"), in MbRec::coef.
struct Block {
  int cat, idx;
  size_t off, len;
};

// The syntax elements of one coded macroblock, in the form of the TSV rows
// "mb", "fld", "pcm", "ipm", "sub", "ref0", "ref1", "mvd0", "mvd1" and "res".
struct MbRec {
  int type = 0;
  int t8x8 = -1, cpred = -1, cbp = -1, qpd = kAbsent;
  int field = -1;  // mb_field_decoding_flag when coded (row "fld")
  bool has_pcm = false;
  int pcm_align = 0;
  std::vector<int> pcm, ipm, sub, ref[2], mvd[2];
  std::vector<Block> res;
  std::vector<int> coef;
  // restoring: read positions and the line of the "mb" row
  size_t ipm_i = 0, sub_i = 0, ref_i[2] = {0, 0}, mvd_i[2] = {0, 0}, res_i = 0;
  unsigned used = 0;  // fields used
  long line = 0;

  // keeps the allocated memory
  void clear() {
    type = 0;
    t8x8 = cpred = cbp = field = -1;
    qpd = kAbsent;
    has_pcm = false;
    pcm_align = 0;
    for (std::vector<int>* v : {&pcm, &ipm, &sub, &ref[0], &ref[1], &mvd[0], &mvd[1], &coef}) v->clear();
    res.clear();
    ipm_i = sub_i = ref_i[0] = ref_i[1] = mvd_i[0] = mvd_i[1] = res_i = 0;
    used = 0;
    line = 0;
  }
};

enum { kUseT8x8 = 1, kUseCpred = 2, kUseCbp = 4, kUseQpd = 8, kUseField = 16, kUsePcm = 32 };

static void put_list(RowWriter& o, const char* name, const std::vector<int>& v) {
  if (v.empty()) return;
  o.begin(name);
  for (int x : v) o.add(x);
  o.end();
}

static int to_int(const Row& row, size_t i) {
  int64_t v = row.num(i);
  if (v < INT_MIN + 1 || v > INT_MAX) row.fail("value out of range");
  return (int)v;
}

static void get_list(const Row& row, std::vector<int>& v, size_t from = 1) {
  if (!v.empty()) row.fail("duplicate row");
  for (size_t i = from; i < row.size(); i++) v.push_back(to_int(row, i));
  if (v.empty()) row.fail("no values");
}

struct TsvChan : Chan {
  explicit TsvChan(RowWriter* out) : Chan(false), out_(out), in_(nullptr) {}
  explicit TsvChan(RowReader* in) : Chan(true), out_(nullptr), in_(in) {}
  // (writing) whether the rows are all taken
  bool at_end() { return in_->at_end(); }

  // --- header syntax
  void hbegin(const char* name) override {
    if (!W) {
      out_->begin(name);
      return;
    }
    const Row* r = in_->peek();
    if (!r) throw TsvError(std::string("unexpected end of file, expected ") + name);
    if (!r->is(name)) r->fail(std::string("expected ") + name);
    row_ = &in_->take();
    field_ = 1;
  }
  int64_t hval(int64_t v) override {
    if (!W) {
      out_->add(v);
      return v;
    }
    return row_->num(field_++);
  }
  void hbytes(std::vector<uint8_t>& b) override {
    if (!W) {
      out_->add(to_hex(b.data(), b.size()));
      return;
    }
    if (field_ >= row_->size()) {
      if (!b.empty()) row_->fail("missing data");
      return;
    }
    b = from_hex(*row_, field_++);
  }
  void hend() override {
    if (!W) {
      out_->end();
      return;
    }
    if (field_ != row_->size()) row_->fail("too many values");
  }
  bool hmore(const char* next, bool m) override {
    if (!W) return m;
    const Row* r = in_->peek();
    return r && r->is(next);
  }

  // --- slice data
  template <class SC>
  void begin(SC& sc) {
    intra_ = sc.stype_ == kI || sc.stype_ == kSI;
    size_ = sc.size_;
    row_ = nullptr;
  }
  void slice_begin(SliceCoderT<0>& sc) override { begin(sc); }
  void slice_begin(SliceCoderT<1>& sc) override { begin(sc); }
  void slice_end() override {
    if (!W) flush_skip();
  }
  int skip(int v) override {
    if (!W) {
      skipped_ += v;
      return v;
    }
    if (!skip_left_ && peek_is(kSkipRow)) skip_left_ = take_skip();
    bool s = skip_left_ > 0;
    if (!s && !peek_is(kMbRow)) fail("expected a macroblock");
    skip_left_ -= s;
    return s;
  }
  int end(int v, bool certain) override {
    (void)certain;
    if (!W) return v;
    return !skip_left_ && !peek_is(kMbRow) && !peek_is(kSkipRow);
  }
  void mb_begin() override {
    MbRec& r = r_;
    r.clear();
    if (!W) {
      flush_skip();
    } else {
      if (!peek_is(kMbRow)) fail("expected a macroblock");
      read_mb_rows(r);
    }
    in_mb_ = true;
  }
  void mb_end() override {
    MbRec& r = r_;
    in_mb_ = false;
    if (!W) {
      write_mb_rows(r);
      return;
    }
    if (r.field >= 0 && !(r.used & kUseField)) fail("fld row for a macroblock without mb_field_decoding_flag");
    if (r.has_pcm && !(r.used & kUsePcm)) fail("pcm row for a macroblock that is not I_PCM");
    if ((r.used & kUsePcm) && (r.ipm.size() || r.sub.size() || r.ref[0].size() || r.ref[1].size() ||
                               r.mvd[0].size() || r.mvd[1].size() || r.res.size() || r.t8x8 >= 0 || r.cpred >= 0 ||
                               r.cbp >= 0 || r.qpd != kAbsent))
      fail("values for syntax elements that are not coded in I_PCM");
    if (r.ipm_i != r.ipm.size() || r.sub_i != r.sub.size() || r.ref_i[0] != r.ref[0].size() ||
        r.ref_i[1] != r.ref[1].size() || r.mvd_i[0] != r.mvd[0].size() || r.mvd_i[1] != r.mvd[1].size())
      fail("too many values for this macroblock");
    if (r.res_i != r.res.size()) fail("res row for a block that is not coded");
    if ((r.t8x8 >= 0 && !(r.used & kUseT8x8)) || (r.cpred >= 0 && !(r.used & kUseCpred)) ||
        (r.cbp >= 0 && !(r.used & kUseCbp)) || (r.qpd != kAbsent && !(r.used & kUseQpd)))
      fail("value for a syntax element that is not coded");
  }
  int field(int v) override {
    if (!W) return r_.field = v;
    if (r_.field < 0 || r_.field > 1) fail("missing or bad mb_field_decoding_flag (fld row)");
    r_.used |= kUseField;
    return r_.field;
  }
  int mb_type(int v) override {
    if (!W) return r_.type = v;
    return r_.type;
  }
  int t8x8(int v) override {
    if (!W) return r_.t8x8 = v;
    return get_field(r_.t8x8, kUseT8x8, "transform_size_8x8_flag");
  }
  int ipm(int blk, int v) override {
    (void)blk;
    return list(r_.ipm, r_.ipm_i, "ipm", v);
  }
  int cpred(int v) override {
    if (!W) return r_.cpred = v;
    return get_field(r_.cpred, kUseCpred, "intra_chroma_pred_mode");
  }
  int sub(int i, int v) override {
    (void)i;
    return list(r_.sub, r_.sub_i, "sub", v);
  }
  int ref(int l, int x, int y, int w, int h, int v) override {
    (void)x, (void)y, (void)w, (void)h;
    return list(r_.ref[l], r_.ref_i[l], l ? "ref1" : "ref0", v);
  }
  int mvd(int l, int comp, int x, int y, int w, int h, int v) override {
    (void)comp, (void)x, (void)y, (void)w, (void)h;
    return list(r_.mvd[l], r_.mvd_i[l], l ? "mvd1" : "mvd0", v);
  }
  int cbp(int v) override {
    if (!W) return r_.cbp = v;
    return get_field(r_.cbp, kUseCbp, "coded_block_pattern");
  }
  int qpd(int v) override {
    if (!W) return r_.qpd = v;
    return get_field(r_.qpd, kUseQpd, "mb_qp_delta");
  }
  void block(int cat, int idx, int* c, int n, bool coded) override {
    (void)coded;
    MbRec& r = r_;
    if (!W) {  // stored when not all zero, up to the last nonzero coefficient
      int last = n - 1;
      while (last >= 0 && !c[last]) last--;
      if (last < 0) return;
      r.res.push_back(Block{cat, idx, r.coef.size(), (size_t)last + 1});
      r.coef.insert(r.coef.end(), c, c + last + 1);
      return;
    }
    for (int i = 0; i < n; i++) c[i] = 0;
    if (r.res_i < r.res.size() && r.res[r.res_i].cat == cat && r.res[r.res_i].idx == idx) {
      const Block& b = r.res[r.res_i++];
      if ((int)b.len > n) fail("too many coefficients in a res row");
      for (size_t i = 0; i < b.len; i++) c[i] = r.coef[b.off + i];
    }
  }
  void pcm(int& align, int& nb, int* s, int n) override {
    (void)nb;
    MbRec& r = r_;
    if (!W) {
      r.has_pcm = true;
      r.pcm_align = align;
      r.pcm.assign(s, s + n);
      return;
    }
    if (!r.has_pcm) fail("missing pcm row");
    if ((int)r.pcm.size() != n) fail("wrong number of PCM samples");
    r.used |= kUsePcm;
    align = r.pcm_align;
    for (int i = 0; i < n; i++) s[i] = r.pcm[i];
  }

  [[noreturn]] void fail(const std::string& msg) override {
    if (!in_mb_ && row_) row_->fail(msg);
    throw TsvError("line " + std::to_string(in_mb_ ? r_.line : in_->lineno()) + ": " + msg);
  }

  // --- rows of the slice data
  void flush_skip() {
    if (skipped_) out_->row("skip", skipped_);
    skipped_ = 0;
  }
  bool peek_is(RowKind k) {
    const Row* r = in_->peek();
    return r && r->kind == k;
  }
  int take_skip() {
    const Row& r = in_->take();
    if (intra_) r.fail("skipped macroblocks in an intra slice");
    if (r.size() != 2) r.fail("expected 1 value");
    int64_t n = r.num(1);
    if (n < 1 || n > size_) r.fail("bad skip count");
    return (int)n;
  }
  int list(std::vector<int>& q, size_t& i, const char* what, int v) {
    if (!W) {
      q.push_back(v);
      return v;
    }
    if (i >= q.size()) fail(std::string("missing ") + what + " value");
    return q[i++];
  }
  int get_field(int f, unsigned use, const char* what) {
    if (f == kAbsent || (use != kUseQpd && f < 0)) fail(std::string("missing ") + what);
    r_.used |= use;
    return f;
  }

  void read_mb_rows(MbRec& r) {
    const Row& m = in_->take();
    if (m.size() != 6) m.fail("expected 5 values");
    r.line = m.lineno;
    r.type = to_int(m, 1);
    r.t8x8 = m.absent(2) ? -1 : to_int(m, 2);
    r.cpred = m.absent(3) ? -1 : to_int(m, 3);
    r.cbp = m.absent(4) ? -1 : to_int(m, 4);
    r.qpd = m.absent(5) ? kAbsent : to_int(m, 5);
    if (r.t8x8 < -1 || r.cpred < -1 || r.cbp < -1) m.fail("negative value");
    while (const Row* p = in_->peek()) {
      switch (p->kind) {
        case kFldRow: {
          const Row& row = in_->take();
          if (r.field >= 0) row.fail("duplicate row");
          if (row.size() != 2) row.fail("expected 1 value");
          r.field = to_int(row, 1);
          if (r.field < 0) row.fail("bad value");
          break;
        }
        case kPcmRow: {
          const Row& row = in_->take();
          if (r.has_pcm) row.fail("duplicate row");
          r.has_pcm = true;
          r.pcm_align = to_int(row, 1);
          for (size_t i = 2; i < row.size(); i++) r.pcm.push_back(to_int(row, i));
          break;
        }
        case kIpmRow:
          get_list(in_->take(), r.ipm);
          break;
        case kSubRow:
          get_list(in_->take(), r.sub);
          break;
        case kRef0Row:
          get_list(in_->take(), r.ref[0]);
          break;
        case kRef1Row:
          get_list(in_->take(), r.ref[1]);
          break;
        case kMvd0Row:
          get_list(in_->take(), r.mvd[0]);
          break;
        case kMvd1Row:
          get_list(in_->take(), r.mvd[1]);
          break;
        case kResRow: {
          const Row& row = in_->take();
          Block b;
          b.cat = to_int(row, 1);
          b.idx = to_int(row, 2);
          b.off = r.coef.size();
          for (size_t i = 3; i < row.size(); i++) r.coef.push_back(to_int(row, i));
          b.len = r.coef.size() - b.off;
          if (!b.len || !r.coef.back()) row.fail("coefficients must end with a nonzero one");
          r.res.push_back(b);
          break;
        }
        default:
          return;
      }
    }
  }

  void write_mb_rows(const MbRec& r) {
    RowWriter& o = *out_;
    o.begin("mb");
    o.add(r.type);
    for (int f : {r.t8x8, r.cpred, r.cbp})
      if (f < 0)
        o.add_absent();
      else
        o.add(f);
    if (r.qpd == kAbsent)
      o.add_absent();
    else
      o.add(r.qpd);
    o.end();
    if (r.field >= 0) o.row("fld", r.field);
    if (r.has_pcm) {
      o.begin("pcm");
      o.add(r.pcm_align);
      for (int x : r.pcm) o.add(x);
      o.end();
    }
    put_list(o, "ipm", r.ipm);
    put_list(o, "sub", r.sub);
    put_list(o, "ref0", r.ref[0]);
    put_list(o, "ref1", r.ref[1]);
    put_list(o, "mvd0", r.mvd[0]);
    put_list(o, "mvd1", r.mvd[1]);
    for (const Block& b : r.res) {
      o.begin("res");
      o.add(b.cat);
      o.add(b.idx);
      for (size_t i = 0; i < b.len; i++) o.add(r.coef[b.off + i]);
      o.end();
    }
  }

  RowWriter* out_;
  RowReader* in_;
  const Row* row_ = nullptr;  // header row being read
  size_t field_ = 0;
  MbRec r_;
  bool in_mb_ = false;
  bool intra_ = false;
  int size_ = 0;
  int skipped_ = 0;    // dumping: skipped macroblocks not written yet
  int skip_left_ = 0;  // restoring: macroblocks left of the skip row
};

}  // namespace h264p
