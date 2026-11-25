// pdfdump -- lossless PDF dump/restore.
//
//   pdfdump c [options] input.pdf outprefix      dump
//   pdfdump d [options] inprefix restored.pdf    restore, byte-exact
//   pdfdump [options] input.pdf                  info only: items, streams, coverage
//
// Single translation unit: this file plus the .inc modules it includes. System headers come
// first; preflate.inc (from ziprec, unmodified) brings its own includes and global typedefs, so
// everything of ours lives in namespace pdd.

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "preflate.inc"

namespace pdd {

#include "chan.inc"
#include "pdf_lex.inc"
#include "pdf_xref.inc"
#include "pdf_seg.inc"
#include "pdf_crypt.inc"
#include "pdf_filt.inc"
#include "pdf_bmp.inc"
#include "pdf_t1.inc"
#include "pdf_objstm.inc"
#include "pdf_class.inc"
#include "pdf_lift.inc"
#include "pdf_emit.inc"

static const int kSyntaxLevelMax = 1;  // 1 = items + raw object text (M1..M4)

struct Options {
  std::string password;
  int filterLevel = 1;
  int syntaxLevel = kSyntaxLevelMax;
  size_t smallThreshold = 1024;
  bool dedup = true;
  std::string chanMap = kDefaultChanMap;
  bool noVerify = false;
  int verbose = 0;
  bool quiet = false;
};

enum Remain : uint8_t { R_DONE, R_ENCODED, R_TERMINAL, R_ENCRYPTED };
static const char* const kRemainName[] = {"decoded", "encoded", "terminal", "encrypted"};

struct StreamRec {
  int item = -1;
  StreamDesc desc;
  Bytes data;                // stored form
  uint8_t remain = R_DONE;
  std::string stop;          // filter at which undoing stopped
  std::vector<std::string> chain;
  size_t payload = 0;        // bytes in the PDF
  size_t decoded = 0;        // bytes after filters (before BMP wrap)
  uint8_t wantCipher = CI_NONE;
  bool bmp = false;
  bool t1 = false;           // Type 1 encryption removed
  size_t storedSize = 0;     // stored form size (data moves into the dump at emit)
  uint64_t storedHash = 0;   // of the stored form, for the verify shortcut
};

struct FilterRef {
  std::string name;
  const Obj* parms = nullptr;
};

static std::string normFilter(const std::string& f) {
  static const char* const ab[][2] = {{"AHx", "ASCIIHexDecode"}, {"A85", "ASCII85Decode"}, {"LZW", "LZWDecode"},
                                      {"Fl", "FlateDecode"},     {"RL", "RunLengthDecode"}, {"CCF", "CCITTFaxDecode"},
                                      {"DCT", "DCTDecode"}};
  for (auto& a : ab)
    if (f == a[0]) return a[1];
  return f;
}
static bool isTerminalFilter(const std::string& f) {
  return f == "DCTDecode" || f == "JPXDecode" || f == "JBIG2Decode" || f == "CCITTFaxDecode";
}
static uint8_t layerOfFilter(const std::string& f) {
  if (f == "FlateDecode") return L_FLATE;
  if (f == "LZWDecode") return L_LZW;
  if (f == "ASCIIHexDecode") return L_AHX;
  if (f == "ASCII85Decode") return L_A85;
  if (f == "RunLengthDecode") return L_RL;
  return L_NONE;
}

struct Stats {
  uint64_t itemCount[Item::KIND_COUNT] = {}, itemBytes[Item::KIND_COUNT] = {};
  struct F { uint64_t present = 0, undone = 0, terminal = 0, bytes = 0; };
  std::map<std::string, F> filters;
  struct C { uint64_t count = 0, payload = 0, stored = 0; };
  C cls[SC_COUNT];
  uint64_t storage[ST_COUNT] = {}, storageBytes[ST_COUNT] = {};
  uint64_t cipherCount[4] = {}, cipherKept = 0;
  uint64_t extents[5] = {};
  std::map<std::string, std::pair<uint64_t, uint64_t>> exts;  // own files by extension: count, bytes
  uint64_t reconBlobs = 0, reconRefs = 0;
  uint64_t typedBytes = 0, payloadBytes = 0, layeredPayload = 0;
  uint64_t bmpCount = 0;
};

class Encoder {
 public:
  Encoder(const Bytes& input, const Options& o) : in_(input), opt_(o) {}

  DumpFiles dump;
  Stats st;
  Segmentation seg;
  Security sec;
  std::vector<StreamRec> streams;
  bool stored = false;
  std::string storedReason;
  double tEncode = 0, tVerify = 0;

  void run() {
    auto t0 = std::chrono::steady_clock::now();
    if (!dump.hdr.map.parse(opt_.chanMap.c_str(), err_)) fail("bad channel map: %s", err_.c_str());
    dump.hdr.origSize = in_.size();
    dump.hdr.origCrc = crc32_update(0, in_.data(), in_.size());
    dump.hdr.filterLevel = (uint8_t)opt_.filterLevel;
    dump.hdr.syntaxLevel = (uint8_t)opt_.syntaxLevel;
    bool modelled = false;
    if (opt_.syntaxLevel > 0) {
      Segmenter(in_.data(), in_.size(), seg).run();
      if (seg.isPdf) {
        modelled = true;
        try {
          analyze();
          emit();
          applyLifts();
        } catch (const Error& e) {
          modelled = false;
          storedReason = std::string("encoder error: ") + e.what();
        }
      } else {
        storedReason = "no %PDF- header in the first 1024 bytes";
      }
    } else {
      storedReason = "syntax level 0";
    }
    tEncode = secondsSince(t0);
    if (modelled && !opt_.noVerify) {
      auto t1 = std::chrono::steady_clock::now();
      std::string why;
      try {
        FastRestore fast = [&](size_t k, const StreamDesc& sd, const Bytes& stored, Bytes& payload) {
          if (k >= streams.size()) return false;
          const StreamRec& s = streams[k];
          const StreamDesc& e = s.desc;
          if (sd.cipher != e.cipher || sd.num != e.num || sd.gen != e.gen || memcmp(sd.iv, e.iv, 16) != 0 ||
              sd.layers.size() != e.layers.size() || stored.size() != s.storedSize ||
              fnv1a64(stored.data(), stored.size()) != s.storedHash)
            return false;
          for (size_t i = 0; i < sd.layers.size(); i++)
            if (sd.layers[i].type != e.layers[i].type || sd.layers[i].recon != e.layers[i].recon) return false;
          const Item& it = seg.items[s.item];
          payload.assign(in_.begin() + it.dataStart, in_.begin() + it.dataEnd);
          return true;
        };
        Bytes re = decodeDump(dump, 0, nullptr, &fast);
        if (re != in_) why = "restored file differs from input";
      } catch (const Error& e) {
        why = e.what();
      }
      tVerify = secondsSince(t1);
      if (!why.empty()) {
        modelled = false;
        storedReason = "verify failed: " + why;
        fprintf(stderr, "pdfdump: internal error, %s; writing stored mode\n", storedReason.c_str());
      }
    }
    if (!modelled) emitStored();
  }

  void writeFiles(const std::string& prefix) {
    Bytes meta;
    serializeHeader(dump.hdr, meta);
    meta.insert(meta.end(), dump.chan[0].begin(), dump.chan[0].end());
    writeFile(prefix + "." + dump.hdr.map.suffix[0], meta.data(), meta.size());
    for (size_t f = 1; f < dump.chan.size(); f++)
      if (!dump.chan[f].empty()) writeFile(prefix + "." + dump.hdr.map.suffix[f], dump.chan[f].data(), dump.chan[f].size());
    for (int c = 0; c < CF_COUNT; c++)
      if (!dump.classFile[c].empty())
        writeFile(prefix + "." + kClassFileSuffix[c], dump.classFile[c].data(), dump.classFile[c].size());
    for (int a = 0; a < AX_COUNT; a++)
      if (!dump.aux[a].empty()) writeFile(prefix + "." + kAuxSuffix[a], dump.aux[a].data(), dump.aux[a].size());
    for (size_t i = 0; i < dump.ownData.size(); i++) {
      const Bytes& b = dump.ownData[i];
      writeFile(ownFileName(prefix, (uint32_t)i, dump.ownExt[i]), b.data(), b.size());
    }
  }

  uint64_t chanFileSize(size_t f) const {
    if (f == 0) {
      Bytes meta;
      serializeHeader(dump.hdr, meta);
      return meta.size() + dump.chan[0].size();
    }
    return dump.chan[f].size();
  }

 private:
  const Bytes& in_;
  Options opt_;
  std::string err_;
  ObjIndex idx_;
  CodecCtx ctx_;
  std::deque<std::vector<std::pair<uint32_t, Obj>>> members_;
  std::unordered_map<int, int> streamOfItem_;
  std::unordered_map<uint32_t, std::vector<int>> streamsByNum_;

  // mostly printable ASCII / UTF-8 text (XMP, PostScript, JavaScript, ...)
  static bool isText(const Bytes& b) {
    if (b.empty()) return false;
    size_t bad = 0;
    for (uint8_t c : b)
      if (!((c >= 0x20 && c < 0x7F) || c == '\r' || c == '\n' || c == '\t' || c >= 0xC0 || (c >= 0x80 && c < 0xC0)))
        bad++;
    size_t hi = 0;
    for (uint8_t c : b) hi += c >= 0x80;
    return bad * 50 <= b.size() && hi * 10 <= b.size();
  }

  static double secondsSince(std::chrono::steady_clock::time_point t) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t).count();
  }

  void applyLifts() {
    Bytes sk, nu, hx;
    int ft = dump.hdr.map.fileOf[CH_T];
    if (ft != 0 && !dump.chan[ft].empty() && liftApply(LIFT_OBJ, dump.chan[ft], sk, nu, hx) && hx.empty()) {
      dump.chan[ft].swap(sk);
      dump.aux[AX_TXN].swap(nu);
      dump.hdr.flags |= kFlagLiftT;
    }
    if (!dump.classFile[CF_CS].empty() && liftApply(LIFT_CS, dump.classFile[CF_CS], sk, nu, hx)) {
      dump.classFile[CF_CS].swap(sk);
      dump.aux[AX_CSN].swap(nu);
      dump.aux[AX_CSH].swap(hx);
      dump.hdr.flags |= kFlagLiftCS;
    }
  }

  void emitStored() {
    stored = true;
    dump.hdr.flags = kFlagStored;
    for (auto& a : dump.aux) a.clear();
    dump.chan.assign(dump.hdr.map.suffix.size(), Bytes());
    for (auto& c : dump.classFile) c.clear();
    dump.ownData.clear();
    dump.ownExt.clear();
    dump.hdr.fileKey.clear();
    ChanWriter w(dump);
    w.u8(CH_S, I_RAW);
    w.lenBytes(CH_N, CH_B, in_);
    w.u8(CH_S, I_END);
  }

  std::vector<FilterRef> getFilters(const Obj& d) const {
    std::vector<FilterRef> r;
    const Obj* f = idx_.resolve(d.get("Filter"));
    const Obj* p = idx_.resolve(d.get("DecodeParms"));
    if (!f) return r;
    if (f->t == Obj::NAME) {
      r.push_back({normFilter(f->s), p && p->t == Obj::DICT ? p : (p && p->t == Obj::ARR && !p->a.empty() ? idx_.resolve(&p->a[0]) : nullptr)});
    } else if (f->t == Obj::ARR) {
      for (size_t i = 0; i < f->a.size(); i++) {
        const Obj* fn = idx_.resolve(&f->a[i]);
        if (!fn || fn->t != Obj::NAME) { r.push_back({"?", nullptr}); continue; }
        const Obj* pp = nullptr;
        if (p && p->t == Obj::ARR && i < p->a.size()) pp = idx_.resolve(&p->a[i]);
        else if (p && p->t == Obj::DICT && f->a.size() == 1) pp = p;
        r.push_back({normFilter(fn->s), pp && pp->t == Obj::DICT ? pp : nullptr});
      }
    }
    return r;
  }

  FilterParams params(const Obj* p) const {
    FilterParams fp;
    if (!p) return fp;
    auto geti = [&](const char* k, int& v) {
      const Obj* o = idx_.resolve(p->get(k));
      if (o && o->t == Obj::INT && o->i >= 0 && o->i < (1 << 30)) v = (int)o->i;
    };
    geti("Predictor", fp.predictor);
    geti("Colors", fp.colors);
    geti("BitsPerComponent", fp.bpc);
    geti("Columns", fp.columns);
    geti("EarlyChange", fp.earlyChange);
    return fp;
  }

  uint8_t streamCipher(const Obj& d, uint32_t num, const std::vector<FilterRef>& fl) const {
    if (!sec.present) return CI_NONE;
    if (sec.encNum && num == sec.encNum) return CI_NONE;
    const Obj* t = idx_.resolve(d.get("Type"));
    if (t && t->isName("XRef")) return CI_NONE;
    if (sec.V >= 4) {
      for (auto& f : fl)
        if (f.name == "Crypt") {
          const Obj* n = f.parms ? idx_.resolve(f.parms->get("Name")) : nullptr;
          return sec.filterCipher(n && n->t == Obj::NAME ? n->s : "Identity");
        }
      if (t && t->isName("EmbeddedFile")) return sec.embeddedCipher();
      if (t && t->isName("Metadata") && !sec.encryptMetadata) return CI_NONE;
      return sec.streamCipher();
    }
    return CI_RC4;
  }

  void setupEncryption() {
    // trailers: classic ones and XRef stream dictionaries, in file order
    const Obj* encRef = nullptr;
    const Obj* id = nullptr;
    std::vector<const Obj*> trailers;
    for (size_t i = 0; i < seg.items.size(); i++) {
      const Item& it = seg.items[i];
      if (it.kind == Item::STREAM) {
        const Obj* t = seg.objs[it.obj].get("Type");
        if (t && t->isName("XRef")) trailers.push_back(&seg.objs[it.obj]);
      }
    }
    for (auto& t : seg.trailers) trailers.push_back(&t);
    for (const Obj* t : trailers) {
      if (const Obj* e = t->get("Encrypt")) encRef = e;
      if (const Obj* i = t->get("ID")) if (!id) id = i;
    }
    if (!encRef) return;
    // prefer the /ID of a trailer that carries /Encrypt
    for (const Obj* t : trailers)
      if (t->get("Encrypt") && t->get("ID")) id = t->get("ID");
    const Obj* enc = idx_.resolve(encRef);
    if (!enc || enc->t != Obj::DICT) return;
    if (encRef->t == Obj::REF) sec.encNum = (uint32_t)encRef->i;
    sec.load(*enc, idx_.resolve(id));
    if (sec.recover(opt_.password)) {
      dump.hdr.fileKey = sec.key;
    } else if (!opt_.quiet) {
      fprintf(stderr, "pdfdump: encrypted file (%s handler, V%d R%d): %s; strings and payloads stay ciphertext\n",
              sec.standard ? "standard" : "non-standard", sec.V, sec.R,
              sec.standard ? (opt_.password.empty() ? "password required, use -P" : "password does not match")
                           : "handler not supported");
    }
  }

  // most common zlib header among Flate streams (plaintext)
  void chooseZlibHeader() {
    std::map<uint16_t, size_t> cnt;
    for (auto& it : seg.items) {
      if (it.kind != Item::STREAM || it.dataEnd - it.dataStart < 2) continue;
      const Obj& d = seg.objs[it.obj];
      auto fl = getFilters(d);
      size_t k = 0;
      while (k < fl.size() && fl[k].name == "Crypt") k++;
      if (k >= fl.size() || fl[k].name != "FlateDecode") continue;
      const uint8_t* p = in_.data() + it.dataStart;
      size_t n = it.dataEnd - it.dataStart;
      uint8_t h[2];
      uint8_t ci = streamCipher(d, it.num, fl);
      if (ci == CI_NONE) {
        h[0] = p[0]; h[1] = p[1];
      } else if (sec.key.empty()) {
        continue;
      } else if (ci == CI_RC4) {
        Bytes k2 = objectKey(sec.key, it.num, it.gen, false);
        rc4(k2.data(), k2.size(), p, 2, h);
      } else {
        if (n < 32) continue;
        Bytes k2 = ci == CI_AES128 ? objectKey(sec.key, it.num, it.gen, true) : sec.key;
        if (k2.size() != (ci == CI_AES128 ? 16u : 32u)) continue;
        uint8_t blk[16];
        aesCbcDecrypt(k2.data(), k2.size(), p, p + 16, 16, blk);
        h[0] = blk[0]; h[1] = blk[1];
      }
      if (zlibHeaderValid(h[0], h[1])) cnt[(uint16_t)(h[0] << 8 | h[1])]++;
    }
    size_t best = 0;
    for (auto& c : cnt)
      if (c.second > best) {
        best = c.second;
        dump.hdr.zlibHdr[0] = (uint8_t)(c.first >> 8);
        dump.hdr.zlibHdr[1] = (uint8_t)c.first;
      }
    ctx_.zlibHdr[0] = dump.hdr.zlibHdr[0];
    ctx_.zlibHdr[1] = dump.hdr.zlibHdr[1];
  }

  void processStream(StreamRec& s) {
    const Item& it = seg.items[s.item];
    const Obj& d = seg.objs[it.obj];
    s.payload = it.dataEnd - it.dataStart;
    Bytes cur(in_.begin() + it.dataStart, in_.begin() + it.dataEnd);
    std::vector<FilterRef> fl = getFilters(d);
    for (auto& f : fl) s.chain.push_back(f.name);
    uint8_t ci = streamCipher(d, it.num, fl);
    s.wantCipher = ci;
    if (ci != CI_NONE) {
      Bytes pt;
      if (sec.key.empty() || !cipherDecrypt(ci, sec.key, it.num, it.gen, cur.data(), cur.size(), pt, s.desc.iv)) {
        s.remain = R_ENCRYPTED;
        s.data.swap(cur);
        s.decoded = s.data.size();
        return;
      }
      Bytes re;
      cipherEncrypt(ci, sec.key, it.num, it.gen, s.desc.iv, pt, re);
      if (re != cur) {
        s.remain = R_ENCRYPTED;
        s.data.swap(cur);
        s.decoded = s.data.size();
        return;
      }
      s.desc.cipher = ci;
      s.desc.num = it.num;
      s.desc.gen = it.gen;
      cur.swap(pt);
    }
    if (d.get("F")) {
      s.stop = "external";  // data lives in an external file; in-file bytes are not filtered
    } else if (opt_.filterLevel == 0) {
      if (!fl.empty()) { s.remain = R_ENCODED; s.stop = fl[0].name; }
    } else {
      for (auto& f : fl) {
        if (f.name == "Crypt") continue;
        if (isTerminalFilter(f.name)) { s.remain = R_TERMINAL; s.stop = f.name; break; }
        uint8_t lt = layerOfFilter(f.name);
        if (!lt) { s.remain = R_ENCODED; s.stop = f.name; break; }
        FilterParams fp = params(f.parms);
        Bytes out, recon;
        if (!layerDecode(ctx_, lt, fp, cur, out, recon)) { s.remain = R_ENCODED; s.stop = f.name; break; }
        s.desc.layers.push_back({lt, std::move(recon)});
        cur.swap(out);
        if ((lt == L_FLATE || lt == L_LZW) && fp.predictor > 1) {
          uint8_t pl = fp.predictor == 2 ? L_TIFF : fp.predictor >= 10 ? L_PNG : L_NONE;
          if (!pl || !layerDecode(ctx_, pl, fp, cur, out, recon)) { s.remain = R_ENCODED; s.stop = "Predictor"; break; }
          s.desc.layers.push_back({pl, std::move(recon)});
          cur.swap(out);
        }
      }
    }
    s.data.swap(cur);
    s.decoded = s.data.size();
  }

  void analyze() {
    // index of top-level objects (latest definition wins)
    for (auto& it : seg.items)
      if (it.kind == Item::OBJ || it.kind == Item::STREAM) idx_.byNum[it.num] = &seg.objs[it.obj];
    setupEncryption();
    chooseZlibHeader();
    for (size_t i = 0; i < seg.items.size(); i++) {
      if (seg.items[i].kind != Item::STREAM) continue;
      StreamRec s;
      s.item = (int)i;
      streamOfItem_[(int)i] = (int)streams.size();
      streamsByNum_[seg.items[i].num].push_back((int)streams.size());
      streams.push_back(std::move(s));
    }
    for (auto& s : streams) processStream(s);
    // read-only ObjStm expansion; rebuild the index in file order with members
    Classifier cl(idx_);
    std::vector<int> objstmOf(seg.items.size(), -1);
    for (auto& s : streams) {
      const Obj& d = seg.objs[seg.items[s.item].obj];
      if (cl.ownClass(d) != SC_OBJSTM || s.remain != R_DONE) continue;
      members_.emplace_back();
      if (expandObjStm(d, s.data, members_.back())) objstmOf[s.item] = (int)members_.size() - 1;
      else members_.pop_back();
    }
    idx_.byNum.clear();
    for (size_t i = 0; i < seg.items.size(); i++) {
      const Item& it = seg.items[i];
      if (it.kind != Item::OBJ && it.kind != Item::STREAM) continue;
      idx_.byNum[it.num] = &seg.objs[it.obj];
      if (objstmOf[i] >= 0)
        for (auto& m : members_[objstmOf[i]]) idx_.byNum[m.first] = &m.second;
    }
    // classification
    for (auto& it : seg.items)
      if (it.kind == Item::OBJ || it.kind == Item::STREAM) cl.walk(seg.objs[it.obj]);
    for (auto& ms : members_)
      for (auto& m : ms) cl.walk(m.second);
    size_t hintOff = SIZE_MAX;
    for (auto& it : seg.items) {
      if (it.kind != Item::OBJ) continue;
      const Obj& o = seg.objs[it.obj];
      if (o.get("Linearized")) {
        const Obj* h = o.get("H");
        if (h && h->t == Obj::ARR && !h->a.empty() && h->a[0].t == Obj::INT) hintOff = (size_t)h->a[0].i;
        break;
      }
    }
    for (auto& s : streams) {
      const Item& it = seg.items[s.item];
      const Obj& d = seg.objs[it.obj];
      s.desc.cls = cl.classify(d, it.num);
      if (s.desc.cls == SC_UNKNOWN && hintOff != SIZE_MAX && (it.start == hintOff || it.start == hintOff + seg.hdrPos))
        s.desc.cls = SC_HINT;
    }
    // BMP3 for fully decoded raw images
    auto streamData = [&](const Obj* ref) -> const Bytes* {
      if (!ref || ref->t != Obj::REF) return nullptr;
      auto f = streamsByNum_.find((uint32_t)ref->i);
      if (f == streamsByNum_.end()) return nullptr;
      const StreamRec& t = streams[f->second.back()];
      return t.remain == R_DONE && !t.bmp ? &t.data : nullptr;
    };
    for (auto& s : streams) {
      if (s.desc.cls != SC_IMAGE || s.remain != R_DONE) continue;
      BmpSpec sp;
      if (!imageBmpSpec(idx_, seg.objs[seg.items[s.item].obj], streamData, sp)) continue;
      Bytes bmp, recon, back;
      if (!bmpWrap(sp, s.data, bmp, recon)) continue;
      if (!layerEncode(ctx_, L_BMP, recon, bmp, back) || back != s.data) continue;
      s.desc.layers.push_back({L_BMP, std::move(recon)});
      s.data.swap(bmp);
      s.bmp = true;
    }
    // Type 1 fonts: remove eexec and charstring encryption
    for (auto& s : streams) {
      if (s.remain != R_DONE) continue;
      const char* sig = sniffExt(s.data.data(), s.data.size());
      if (s.desc.cls != SC_FONT_T1 && !(sig && !strcmp(sig, "pfa"))) continue;
      Bytes out, recon;
      FilterParams fp;
      bool ok = layerDecode(ctx_, L_T1, fp, s.data, out, recon);
      if (!ok) {
        fp.t1Charstrings = false;
        ok = layerDecode(ctx_, L_T1, fp, s.data, out, recon);
      }
      if (!ok) continue;
      s.desc.layers.push_back({L_T1, std::move(recon)});
      s.data.swap(out);
      s.t1 = true;
    }
    // object streams: header table derived from the members
    for (auto& s : streams) {
      if (s.desc.cls != SC_OBJSTM || s.remain != R_DONE) continue;
      const Obj& d = seg.objs[seg.items[s.item].obj];
      const Obj* N = idx_.resolve(d.get("N"));
      const Obj* F = idx_.resolve(d.get("First"));
      if (!N || !F || N->t != Obj::INT || F->t != Obj::INT) continue;
      FilterParams fp;
      fp.objstmN = N->i;
      fp.objstmFirst = F->i;
      Bytes out, recon;
      if (!layerDecode(ctx_, L_OBJSTM, fp, s.data, out, recon)) continue;
      s.desc.layers.push_back({L_OBJSTM, std::move(recon)});
      s.data.swap(out);
    }
    // extension and storage
    for (auto& s : streams) {
      uint8_t cls = s.desc.cls;
      std::string ext;
      if (s.remain == R_ENCRYPTED) {
        ext = "enc";
      } else if (s.remain == R_TERMINAL) {
        if (s.stop == "DCTDecode") ext = "jpg";
        else if (s.stop == "JPXDecode") ext = sniffExt(s.data.data(), s.data.size()) && std::string(sniffExt(s.data.data(), s.data.size())) == "j2k" ? "j2k" : "jp2";
        else if (s.stop == "JBIG2Decode") ext = "jb2";
        else ext = "g4";
      } else if (s.bmp) {
        ext = "bmp";
      } else if (s.t1) {
        ext = "t1d";
      } else if (const char* sig = sniffExt(s.data.data(), s.data.size())) {
        ext = sig;
      } else {
        uint32_t num = seg.items[s.item].num;
        auto an = cl.attachName.find(num);
        if (cls == SC_ATTACH && an != cl.attachName.end()) ext = fileNameExt(an->second);
        if (ext.empty()) ext = s.remain == R_ENCODED ? "z" : kClassExt[cls];
        if (cls == SC_CONTENT || cls == SC_CMAP) ext = s.remain == R_ENCODED ? "z" : "bin";
      }
      s.desc.ext = ext;
      if (cls == SC_OBJSTM && s.remain == R_DONE) s.desc.storage = ST_TXT;
      else if (cls == SC_XREF && s.remain == R_DONE) s.desc.storage = ST_BIN;
      else if ((cls == SC_CONTENT || cls == SC_CMAP) && s.remain == R_DONE) {
        s.desc.storage = ST_CLASS;
        s.desc.classFile = cls == SC_CONTENT ? CF_CS : CF_CMAP;
      } else if (s.data.size() < opt_.smallThreshold) {
        s.desc.storage = isText(s.data) ? ST_TXT : ST_BIN;  // small text streams sit with the object text
      }
      else s.desc.storage = ST_FILE;
    }
  }

  void emit() {
    ChanWriter w(dump);
    EmitState es;
    std::unordered_map<uint64_t, std::vector<uint32_t>> dedup;
    ObjPositions objPos;
    for (size_t i = 0; i < seg.items.size(); i++) {
      const Item& it = seg.items[i];
      size_t len = it.end - it.start;
      if (it.kind == Item::OBJ || it.kind == Item::STREAM) {
        uint32_t num, gen;
        size_t hl = (it.kind == Item::STREAM ? it.dataStart : it.end) - it.start;
        if (objHeaderOf(in_.data() + it.start, hl, num, gen)) objPos.add(num, gen, it.start);
      }
      st.itemCount[it.kind]++;
      st.itemBytes[it.kind] += len;
      switch (it.kind) {
        case Item::TEXT:
          w.u8(CH_S, I_TEXT);
          w.lenBytes(CH_N, CH_T, in_.data() + it.start, len);
          break;
        case Item::RAW:
          w.u8(CH_S, I_RAW);
          w.lenBytes(CH_N, CH_B, in_.data() + it.start, len);
          break;
        case Item::OBJ:
          w.u8(CH_S, I_OBJ);
          w.lenBytes(CH_N, CH_T, in_.data() + it.start, len);
          break;
        case Item::XREF:
          w.u8(CH_S, I_XREF);
          writeXref(w, seg.xrefs[it.xref], objPos);
          break;
        case Item::STREAM: {
          StreamRec& s = streams[streamOfItem_[(int)i]];
          w.u8(CH_S, I_STREAM);
          uint64_t h = 0;
          if (s.desc.storage == ST_FILE && opt_.dedup) {
            h = fnv1a64(s.data.data(), s.data.size()) ^ ((uint64_t)s.data.size() * 0x9E3779B97F4A7C15ull);
            auto f = dedup.find(h);
            if (f != dedup.end())
              for (uint32_t k : f->second)
                if (dump.ownData[k] == s.data) {
                  s.desc.storage = ST_DUP;
                  s.desc.ordinal = k;
                  break;
                }
          }
          accountStream(s);
          writeStreamDesc(w, es, s.desc, s.data);
          if (s.desc.storage == ST_FILE && opt_.dedup) dedup[h].push_back(s.desc.ordinal);
          w.lenBytes(CH_N, CH_T, in_.data() + it.start, it.dataStart - it.start);
          w.lenBytes(CH_N, CH_T, in_.data() + it.dataEnd, it.end - it.dataEnd);
          break;
        }
      }
    }
    w.u8(CH_S, I_END);
    st.reconBlobs = es.recon.count;
    st.reconRefs = es.recon.refs;
  }

  void accountStream(StreamRec& s) {
    const Item& it = seg.items[s.item];
    s.storedSize = s.data.size();
    s.storedHash = fnv1a64(s.data.data(), s.data.size());
    st.extents[it.extent]++;
    auto& c = st.cls[s.desc.cls];
    c.count++;
    c.payload += s.payload;
    c.stored += s.data.size();
    st.storage[s.desc.storage]++;
    st.storageBytes[s.desc.storage] += s.data.size();
    st.payloadBytes += s.payload;
    if (!s.desc.layers.empty()) st.layeredPayload += s.payload;
    if (s.wantCipher != CI_NONE) {
      if (s.desc.cipher != CI_NONE) st.cipherCount[s.desc.cipher]++;
      else st.cipherKept++;
    }
    bool sawStop = false;
    for (auto& f : s.chain) {
      auto& fs = st.filters[f];
      fs.present++;
      fs.bytes += s.payload;
      if (f == "Crypt") continue;
      if (sawStop || s.remain == R_ENCRYPTED) continue;
      if (f == s.stop) {
        sawStop = true;
        if (s.remain == R_TERMINAL) fs.terminal++;
        continue;
      }
      fs.undone++;
    }
    if (s.bmp) st.bmpCount++;
    if (s.desc.storage == ST_FILE || s.desc.storage == ST_DUP) {
      auto& e = st.exts[s.desc.ext];
      if (s.desc.storage == ST_FILE) { e.first++; e.second += s.data.size(); }
    }
    bool typed = s.desc.storage == ST_CLASS ||
                 ((s.desc.storage == ST_FILE || s.desc.storage == ST_DUP) && s.desc.ext != "bin" && s.desc.ext != "z" &&
                  s.desc.ext != "enc");
    if (typed) st.typedBytes += s.payload;
  }
};

// ---------------------------------------------------------------------------------------------

static std::string human(uint64_t v) {
  char b[32];
  if (v < 10000) snprintf(b, sizeof(b), "%llu", (unsigned long long)v);
  else if (v < 10000000) snprintf(b, sizeof(b), "%.1fK", v / 1024.0);
  else snprintf(b, sizeof(b), "%.1fM", v / 1048576.0);
  return b;
}
static double pct(uint64_t a, uint64_t b) { return b ? 100.0 * a / b : 0.0; }

static void printInfo(const char* name, const Bytes& in, Encoder& E, const Options& opt) {
  const Stats& st = E.st;
  printf("%s: %llu bytes", name, (unsigned long long)in.size());
  if (E.seg.isPdf) {
    const uint8_t* h = in.data() + E.seg.hdrPos;
    size_t k = 0;
    while (E.seg.hdrPos + k < in.size() && k < 12 && !isWS(h[k])) k++;
    printf(", %.*s", (int)k, (const char*)h);
    if (E.seg.hdrPos) printf(" at offset %llu", (unsigned long long)E.seg.hdrPos);
  }
  printf("\n");
  if (E.stored) {
    printf("stored mode: %s\n", E.storedReason.c_str());
    return;
  }
  static const char* kn[] = {"text", "raw", "obj", "stream", "xref"};
  printf("items:");
  for (int k = 0; k < Item::KIND_COUNT; k++)
    printf("  %s %llu (%s)", kn[k], (unsigned long long)st.itemCount[k], human(st.itemBytes[k]).c_str());
  printf("\n");
  if (E.sec.present) {
    printf("encryption: %s handler V%d R%d, %s\n", E.sec.standard ? "standard" : "non-standard", E.sec.V, E.sec.R,
           E.sec.key.empty() ? "no key (payloads kept as ciphertext)"
                             : ("key recovered from " + E.sec.keySource + ", " + std::to_string(E.sec.key.size()) +
                                " bytes").c_str());
    printf("  streams decrypted: RC4 %llu, AES-128 %llu, AES-256 %llu, kept encrypted %llu\n",
           (unsigned long long)st.cipherCount[CI_RC4], (unsigned long long)st.cipherCount[CI_AES128],
           (unsigned long long)st.cipherCount[CI_AES256], (unsigned long long)st.cipherKept);
  }
  printf("stream extents:");
  for (int k = 0; k < 5; k++)
    if (st.extents[k]) printf("  %s %llu", kExtentName[k], (unsigned long long)st.extents[k]);
  printf("\n");
  printf("filters (streams present / undone / terminal, payload bytes):\n");
  for (auto& f : st.filters)
    printf("  %-16s %6llu %6llu %6llu  %s\n", f.first.c_str(), (unsigned long long)f.second.present,
           (unsigned long long)f.second.undone, (unsigned long long)f.second.terminal, human(f.second.bytes).c_str());
  printf("classes (count, payload -> stored):\n");
  for (int c = 0; c < SC_COUNT; c++)
    if (st.cls[c].count)
      printf("  %-12s %6llu  %9s -> %9s\n", kClassName[c], (unsigned long long)st.cls[c].count,
             human(st.cls[c].payload).c_str(), human(st.cls[c].stored).c_str());
  printf("storage:");
  for (int k = 0; k < ST_COUNT; k++)
    if (st.storage[k])
      printf("  %s %llu (%s)", kStorageName[k], (unsigned long long)st.storage[k], human(st.storageBytes[k]).c_str());
  printf("\n");
  if (!st.exts.empty()) {
    printf("files:");
    for (auto& e : st.exts)
      if (e.second.first) printf("  .%s %llu (%s)", e.first.c_str(), (unsigned long long)e.second.first, human(e.second.second).c_str());
    printf("\n");
  }
  if (st.bmpCount) printf("bmp3 images: %llu\n", (unsigned long long)st.bmpCount);
  printf("dump:");
  uint64_t total = 0;
  for (size_t f = 0; f < E.dump.chan.size(); f++) {
    uint64_t sz = E.chanFileSize(f);
    total += sz;
    printf("  .%s %s", E.dump.hdr.map.suffix[f].c_str(), human(sz).c_str());
  }
  for (int c = 0; c < CF_COUNT; c++)
    if (!E.dump.classFile[c].empty()) {
      total += E.dump.classFile[c].size();
      printf("  .%s %s", kClassFileSuffix[c], human(E.dump.classFile[c].size()).c_str());
    }
  for (int a = 0; a < AX_COUNT; a++)
    if (!E.dump.aux[a].empty()) {
      total += E.dump.aux[a].size();
      printf("  .%s %s", kAuxSuffix[a], human(E.dump.aux[a].size()).c_str());
    }
  uint64_t own = 0;
  for (auto& b : E.dump.ownData) own += b.size();
  total += own;
  printf("  own files %zu (%s)  total %s\n", E.dump.ownData.size(), human(own).c_str(), human(total).c_str());
  uint64_t rawb = st.itemBytes[Item::RAW];
  uint64_t recon = 0;  // channel R bytes (exact only when R has a file of its own)
  int rf = E.dump.hdr.map.fileOf[CH_R];
  bool rAlone = true;
  for (int c = 0; c < CH_COUNT; c++)
    if (c != CH_R && E.dump.hdr.map.fileOf[c] == rf) rAlone = false;
  if (rAlone) recon = E.dump.chan[rf].size();
  printf("coverage: raw share %.2f%%, typed share %.1f%%, recon share %s%.2f%% (%s recon bytes, %llu blobs for %llu layers)\n",
         pct(rawb, in.size()), pct(st.typedBytes, st.payloadBytes), rAlone ? "" : "~",
         pct(recon, st.layeredPayload), human(recon).c_str(), (unsigned long long)st.reconBlobs,
         (unsigned long long)st.reconRefs);
  printf("time: encode %.2fs (preflate: %llu streams, %s unpacked, analyze %.2fs, re-encode %.2fs), verify %.2fs\n",
         E.tEncode, (unsigned long long)gFlateProf.streams, human(gFlateProf.unpacked).c_str(), gFlateProf.dec,
         gFlateProf.enc, E.tVerify);
  if (opt.verbose) {
    printf("streams:\n  %-6s %-9s %-12s %-28s %-9s %9s %9s %-6s %s\n", "item", "obj", "class", "filters", "state",
           "payload", "stored", "store", "ext/layers");
    for (auto& s : E.streams) {
      const Item& it = E.seg.items[s.item];
      std::string fl;
      for (auto& f : s.chain) fl += (fl.empty() ? "" : ",") + f;
      if (fl.empty()) fl = "-";
      std::string ly;
      for (auto& l : s.desc.layers) ly += std::string(ly.empty() ? "" : "+") + kLayerName[l.type];
      char obj[32];
      snprintf(obj, sizeof(obj), "%u %u", it.num, it.gen);
      printf("  %-6d %-9s %-12s %-28s %-9s %9zu %9zu %-6s .%s %s%s%s\n", s.item, obj, kClassName[s.desc.cls],
             fl.c_str(), kRemainName[s.remain], s.payload, s.storedSize, kStorageName[s.desc.storage],
             s.desc.ext.c_str(), ly.c_str(), s.desc.cipher ? " +" : "", s.desc.cipher ? kCipherName[s.desc.cipher] : "");
    }
  }
}

// ---------------------------------------------------------------------------------------------

static int selfTest() {
  int bad = 0;
  auto hex = [](const Bytes& b) {
    std::string s;
    char t[3];
    for (uint8_t c : b) { snprintf(t, 3, "%02x", c); s += t; }
    return s;
  };
  auto check = [&](const char* what, const std::string& got, const char* want) {
    bool ok = got == want;
    printf("%-28s %s\n", what, ok ? "ok" : ("FAIL got " + got).c_str());
    if (!ok) bad++;
  };
  uint8_t m[16];
  md5((const uint8_t*)"abc", 3, m);
  check("md5(abc)", hex(Bytes(m, m + 16)), "900150983cd24fb0d6963f7d28e17f72");
  check("sha256(abc)", hex(sha256((const uint8_t*)"abc", 3)),
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
  check("sha384(abc)", hex(sha512x((const uint8_t*)"abc", 3, true)),
        "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7");
  check("sha512(abc)", hex(sha512x((const uint8_t*)"abc", 3, false)),
        "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f");
  {
    Bytes key(16), pt(16), ct(16);
    for (int i = 0; i < 16; i++) { key[i] = (uint8_t)i; pt[i] = (uint8_t)(i * 0x11); }
    Aes a(key.data(), 16);
    a.encBlock(pt.data(), ct.data());
    check("aes128 encrypt", hex(ct), "69c4e0d86a7b0430d8cdb78070b4c55a");
    Bytes back(16);
    a.decBlock(ct.data(), back.data());
    check("aes128 decrypt", hex(back), hex(pt).c_str());
    Bytes key2(32);
    for (int i = 0; i < 32; i++) key2[i] = (uint8_t)i;
    Aes b(key2.data(), 32);
    b.encBlock(pt.data(), ct.data());
    check("aes256 encrypt", hex(ct), "8ea2b7ca516745bfeafc49904b496089");
    b.decBlock(ct.data(), back.data());
    check("aes256 decrypt", hex(back), hex(pt).c_str());
  }
  {
    uint8_t out[9];
    rc4((const uint8_t*)"Key", 3, (const uint8_t*)"Plaintext", 9, out);
    check("rc4(Key,Plaintext)", hex(Bytes(out, out + 9)), "bbf316e8d940af0ad3");
  }
  // filter round trips
  Bytes sample;
  for (int i = 0; i < 20000; i++) sample.push_back((uint8_t)((i * 7) % 13 == 0 ? 0 : "abcabcabdx"[i % 10] + (i / 997) % 3));
  for (int early = 0; early <= 1; early++) {
    Bytes enc, dec, recon, re;
    bool ok = lzwTestEncode(sample, enc, early);
    FilterParams fp;
    fp.earlyChange = early;
    ok = ok && lzwDecode(fp, enc, dec, recon) && dec == sample && lzwEncode(recon, dec, re) && re == enc;
    check(early ? "lzw early=1 round trip" : "lzw early=0 round trip", ok ? "ok" : "mismatch", "ok");
  }
  {
    const char* hx = "48656c6c6f2c20776f726c6421\n0a>";
    Bytes in((const uint8_t*)hx, (const uint8_t*)hx + strlen(hx)), out, recon, re;
    bool ok = ahxDecode(in, out, recon) && ahxEncode(recon, out, re) && re == in;
    check("asciihex round trip", ok ? "ok" : "mismatch", "ok");
    Bytes a85in;
    a85Write(sample, false, true, 75, Bytes{'\n'}, false, true, a85in);
    ok = a85Decode(a85in, out, recon) && out == sample && a85Encode(recon, out, re) && re == a85in;
    check("ascii85 round trip", ok ? "ok" : "mismatch", "ok");
    Bytes hdrs, rl;
    rlGreedyHeaders(sample, 2, hdrs);
    Bytes rr;
    rr.push_back(2);
    rr.push_back(1);
    putVarTo(rr, 0);
    ok = rlEncode(rr, sample, rl) && rlDecode(rl, out, recon) && out == sample && rlEncode(recon, out, re) && re == rl;
    check("runlength round trip", ok ? "ok" : "mismatch", "ok");
  }
  printf("%s\n", bad ? "SELFTEST FAILED" : "all ok");
  return bad ? 1 : 0;
}

// ---------------------------------------------------------------------------------------------

static void usage() {
  fprintf(stderr,
          "pdfdump: lossless PDF dump/restore\n"
          "  pdfdump c [options] input.pdf outprefix   dump\n"
          "  pdfdump d inprefix restored.pdf           restore, byte-exact\n"
          "  pdfdump [options] input.pdf               info only\n"
          "options (c / info):\n"
          "  -P PW    password for encrypted input (user or owner); empty user password is tried automatically\n"
          "  -f N     0 keep streams encoded, 1 undo filters (default 1)\n"
          "  -s N     syntax depth: 0 stored, 1 items + raw object text (default %d)\n"
          "  -p N     small-stream threshold: streams below N bytes share <P>.bin (default 1024)\n"
          "  -D       disable dedup of extracted streams\n"
          "  -c MAP   channel->file map, e.g. S,N,T,X,O,F,R,B (default %s)\n"
          "  -n       skip the final in-memory verify\n"
          "  -v / -q  per-stream report / quiet\n",
          kSyntaxLevelMax, kDefaultChanMap);
}

static int realMain(int argc, char** argv) {
  if (argc < 2) { usage(); return 2; }
  std::string mode = argv[1];
  int first = 2;
  if (mode == "selftest") return selfTest();
  if (mode != "c" && mode != "d") { mode = "i"; first = 1; }
  Options opt;
  std::vector<std::string> pos;
  for (int i = first; i < argc; i++) {
    std::string a = argv[i];
    if (a.size() >= 2 && a[0] == '-') {
      char o = a[1];
      auto val = [&]() -> std::string {
        if (a.size() > 2) return a.substr(2);
        if (i + 1 >= argc) fail("option -%c needs a value", o);
        return argv[++i];
      };
      auto num = [&]() -> long long {
        std::string v = val();
        char* e;
        long long r = strtoll(v.c_str(), &e, 10);
        if (*e || r < 0) fail("bad value for -%c: %s", o, v.c_str());
        return r;
      };
      switch (o) {
        case 'P': opt.password = val(); break;
        case 'f': opt.filterLevel = (int)std::min(1LL, num()); break;
        case 's': {
          long long s = num();
          if (s > kSyntaxLevelMax) {
            fprintf(stderr, "pdfdump: syntax depth %lld not built yet, using %d\n", s, kSyntaxLevelMax);
            s = kSyntaxLevelMax;
          }
          opt.syntaxLevel = (int)s;
          break;
        }
        case 'p': opt.smallThreshold = (size_t)num(); break;
        case 'D': opt.dedup = false; break;
        case 'c': opt.chanMap = val(); break;
        case 'n': opt.noVerify = true; break;
        case 'v': opt.verbose++; break;
        case 'q': opt.quiet = true; break;
        default: fail("unknown option %s", a.c_str());
      }
    } else {
      pos.push_back(a);
    }
  }
  if (mode == "i") {
    if (pos.size() != 1) { usage(); return 2; }
    Bytes in;
    if (!readFile(pos[0], in)) fail("cannot read %s", pos[0].c_str());
    Encoder E(in, opt);
    E.run();
    printInfo(pos[0].c_str(), in, E, opt);
    return 0;
  }
  if (pos.size() != 2) { usage(); return 2; }
  if (mode == "c") {
    Bytes in;
    if (!readFile(pos[0], in)) fail("cannot read %s", pos[0].c_str());
    Encoder E(in, opt);
    E.run();
    E.writeFiles(pos[1]);
    if (opt.verbose) printInfo(pos[0].c_str(), in, E, opt);
    else if (!opt.quiet) {
      uint64_t total = 0;
      for (size_t f = 0; f < E.dump.chan.size(); f++) total += E.chanFileSize(f);
      for (auto& c : E.dump.classFile) total += c.size();
      for (auto& a : E.dump.aux) total += a.size();
      for (auto& b : E.dump.ownData) total += b.size();
      fprintf(stderr, "%s: %llu -> %llu bytes in %zu files%s\n", pos[0].c_str(), (unsigned long long)in.size(),
              (unsigned long long)total, E.dump.ownData.size() + E.dump.chan.size(),
              E.stored ? (" (stored: " + E.storedReason + ")").c_str() : "");
    }
    return 0;
  }
  // d
  const std::string& prefix = pos[0];
  DumpFiles d;
  Bytes meta;
  if (!readFile(prefix + ".meta", meta)) fail("cannot read %s.meta", prefix.c_str());
  size_t meta0 = parseHeader(meta, d.hdr);
  d.chan.resize(d.hdr.map.suffix.size());
  d.chan[0].swap(meta);
  for (size_t f = 1; f < d.chan.size(); f++) readFile(prefix + "." + d.hdr.map.suffix[f], d.chan[f]);
  for (int c = 0; c < CF_COUNT; c++) readFile(prefix + "." + kClassFileSuffix[c], d.classFile[c]);
  for (int a = 0; a < AX_COUNT; a++) readFile(prefix + "." + kAuxSuffix[a], d.aux[a]);
  Bytes out = decodeDump(d, meta0, [&](uint32_t ord, const std::string& ext) {
    Bytes b;
    std::string name = ownFileName(prefix, ord, ext);
    if (!readFile(name, b)) fail("cannot read %s", name.c_str());
    return b;
  });
  if (out.size() != d.hdr.origSize || crc32_update(0, out.data(), out.size()) != d.hdr.origCrc)
    fail("restored file does not match the original size/CRC recorded in the dump");
  writeFile(pos[1], out.data(), out.size());
  return 0;
}

}  // namespace pdd

int main(int argc, char** argv) {
  try {
    return pdd::realMain(argc, argv);
  } catch (const std::exception& e) {
    fprintf(stderr, "pdfdump: %s\n", e.what());
    return 1;
  }
}
