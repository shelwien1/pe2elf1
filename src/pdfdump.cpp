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
#include <atomic>
#include <chrono>
#include <deque>
#include <new>
#include <unistd.h>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <thread>
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
#include "pdf_tiff.inc"
#include "pdf_icc.inc"
#include "pdf_ccitt.inc"
#include "pdf_t1.inc"
#include "pdf_font.inc"
#include "pdf_objstm.inc"
#include "pdf_class.inc"
#include "pdf_lift.inc"
#include "pdf_cmap.inc"
#include "pdf_emit.inc"
#include "pdf_xmp.inc"  // includes pdf_derive.inc

// syntax depth: 1 = items + raw object text; 2 = + object-level structure (xref sections and streams,
// ObjStm headers, derived fields, encrypted strings, forward slots, object text number lifting);
// 3 = + content streams and CMaps (number/hex lifting, inline image filters, CMap blocks)
static const int kSyntaxLevelMax = 3;

struct Options {
  std::string password;
  int filterLevel = 1;
  int syntaxLevel = kSyntaxLevelMax;
  size_t smallThreshold = 1024;
  bool dedup = true;
  std::string chanMap = kDefaultChanMap;
  bool noVerify = false;
  bool derive = true;  // derived fields (-d disables)
  bool ccitt = false;  // decode CCITT fax images to bitmaps (-g)
  unsigned threads = 0;  // streams decoded in parallel (-j); 0 = one per CPU
  bool rgba = false;     // RGB image + 8-bit soft mask as one 32 bpp BMP (-a)
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
  bool tif = false;          // stored as a TIFF file
  bool t1 = false;           // Type 1 encryption removed
  const Obj* stopParms = nullptr;  // /DecodeParms of the filter at which undoing stopped
  bool stopLast = false;           // ... and that filter is the last one
  size_t storedSize = 0;     // stored form size (data moves into the dump at emit)
  uint64_t storedHash = 0;   // of the stored form, for the verify shortcut
};

struct FilterRef {
  std::string name;
  const Obj* parms = nullptr;
};

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
  uint64_t derived = 0, stringsDecrypted = 0, sigBlobs = 0, rgbaMerged = 0, b64Blobs = 0, b64Bytes = 0, xmpThumbs = 0,
           iccRefs = 0, itemLenPredicted = 0, widthArrays = 0, widthsDerived = 0, structParents = 0, membersDerived = 0,
           parentTreeRefs = 0, widthsByKind[8] = {};
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
  double tEncode = 0, tVerify = 0, tLift = 0;

  void run() {
    auto t0 = std::chrono::steady_clock::now();
    if (!dump.hdr.map.parse(opt_.chanMap.c_str(), err_)) die("bad channel map: %s", err_.c_str());
    dump.hdr.origSize = in_.size();
    dump.hdr.origCrc = crc32_update(0, in_.data(), in_.size());
    dump.hdr.filterLevel = (uint8_t)opt_.filterLevel;
    dump.hdr.syntaxLevel = (uint8_t)opt_.syntaxLevel;
    bool modelled = false;
    if (opt_.syntaxLevel < 2) opt_.derive = false;
    if (opt_.syntaxLevel > 0) {
      Segmenter(in_.data(), in_.size(), seg, opt_.syntaxLevel >= 2).run();
      if (seg.isPdf) {
        modelled = true;
        // an error in any phase (a worker thread's included) falls back to stored mode
        failClear();
        analyze();
        if (!failed()) emit();
        if (!failed()) itemLengths();
        auto tl = std::chrono::steady_clock::now();
        if (!failed()) applyLifts(opt_.noVerify);
        tLift = secondsSince(tl);
        if (failed()) {
          modelled = false;
          storedReason = "encoder error: " + failMessage();
          failClear();
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
      std::string why = verifyDump();
      if (!why.empty() && liftsUnchecked_) {
        // a lift that its own check would have refused: redo the lifts with the checks
        undoLifts();
        applyLifts(true);
        why = failed() ? failMessage() : verifyDump();
        failClear();
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
    // record which files exist, so that d ignores stale files of an earlier dump with this prefix
    dump.hdr.present = 1;
    for (size_t f = 1; f < dump.hdr.map.nfiles; f++)
      if (!dump.chan[f].empty()) dump.hdr.present |= (uint64_t)1 << f;
    for (int c = 0; c < CF_COUNT; c++)
      if (!dump.classFile[c].empty()) dump.hdr.present |= (uint64_t)1 << (kPresentClass + c);
    // small aux files ride at the end of the meta file: a file of its own costs a tar header
    dump.hdr.inlineAux = 0;
    for (int a = 0; a < AX_COUNT; a++) {
      dump.hdr.inlineLen[a] = 0;
      if (dump.aux[a].empty()) continue;
      if (dump.aux[a].size() <= kAuxInlineMax) {
        dump.hdr.inlineAux |= (uint64_t)1 << a;
        dump.hdr.inlineLen[a] = dump.aux[a].size();
      } else {
        dump.hdr.present |= (uint64_t)1 << (kPresentAux + a);
      }
    }
    Bytes meta;
    serializeHeader(dump.hdr, meta);
    meta.insert(meta.end(), dump.chan[0].begin(), dump.chan[0].end());
    for (int a = 0; a < AX_COUNT; a++)
      if (dump.hdr.inlineAux >> a & 1) meta.insert(meta.end(), dump.aux[a].begin(), dump.aux[a].end());
    writeFile(prefix + "." + dump.hdr.map.suffix[0].c_str(), meta.data(), meta.size());
    for (size_t f = 1; f < dump.hdr.map.nfiles; f++)
      if (!dump.chan[f].empty())
        writeFile(prefix + "." + dump.hdr.map.suffix[f].c_str(), dump.chan[f].data(), dump.chan[f].size());
    for (int c = 0; c < CF_COUNT; c++)
      if (!dump.classFile[c].empty())
        writeFile(prefix + "." + kClassFileSuffix[c], dump.classFile[c].data(), dump.classFile[c].size());
    for (int a = 0; a < AX_COUNT; a++)
      if (dump.hdr.present >> (kPresentAux + a) & 1)
        writeFile(prefix + "." + kAuxSuffix[a], dump.aux[a].data(), dump.aux[a].size());
    for (size_t i = 0; i < dump.ownData.size(); i++) {
      const Bytes& b = dump.ownData[i];
      writeFile(ownFileName(prefix, (uint32_t)i, dump.ownExt[i]), b.data(), b.size());
    }
    for (size_t i = 0; i < dump.blobData.size(); i++)
      writeFile(ownFileName(prefix, kBlobFileBit | (uint32_t)i, dump.blobExt[i]), dump.blobData[i].data(),
                dump.blobData[i].size());
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
  std::vector<uint32_t> ownOrd_;                     // stream -> own-file ordinal (ST_FILE)
  std::unordered_map<uint32_t, size_t> streamByNum_;  // object number -> stream
  FontCache fontCache_;                               // metrics of recent font programs
  std::vector<std::string> ordExt_;                      // own-file ordinal -> extension
  std::unordered_map<uint32_t, const Obj*> widthArrays_;  // array object -> font dictionary naming it
  std::unordered_map<uint32_t, uint32_t> numsArrays_;     // array object -> its /Nums key
  std::unordered_map<uint32_t, size_t> inlineSeen_;       // inline-stored streams written so far
  StructMap sm_;                                      // structure parents, in item order
  Bytes preT_, preCS_, preCmap_;                      // texts before unchecked lifts
  bool liftsUnchecked_ = false;
  std::unordered_map<uint32_t, std::vector<int>> streamsByNum_;
  std::vector<Bytes> b64Orig_;  // original zlib bytes per base64 blob (verify shortcut)
  std::unordered_map<uint64_t, std::vector<int>> cache_;
  uint64_t bias_ = 0;  // offset bias (plan 4, step 1)

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

  // object and stream item texts: bytes to T, length to .iln after the whole T channel is known
  struct ItemText {
    size_t pos, len;
    int kind;
  };
  std::vector<ItemText> itemTexts_;
  void itemText(ChanWriter& w, int kind, const Bytes& t) {
    itemTexts_.push_back({dump.chan[dump.hdr.map.fileOf[CH_T]].size(), t.size(), kind});
    w.put(CH_T, t);
  }
  void itemLengths() {
    const Bytes& t = dump.chan[dump.hdr.map.fileOf[CH_T]];
    ItemLenPredictor pr(t.data(), t.size());
    Bytes& o = dump.aux[AX_ILN];
    for (auto& i : itemTexts_) {
      size_t p = pr.predict(i.pos, i.kind);
      putVarTo(o, p == i.len ? 0 : zigzag((int64_t)i.len - (int64_t)p));
      st.itemLenPredicted += p == i.len;
    }
    itemTexts_.clear();
  }

  // the in-memory restore of the whole dump; "" if it reproduces the input
  std::string verifyDump() {
    std::string why;
    {
      FastRestore fast = [&](size_t k, const StreamDesc& sd, const Bytes& stored, Bytes& payload) {
        if (k >= streams.size()) return false;
        const StreamRec& s = streams[k];
        const StreamDesc& e = s.desc;
        if (sd.cipher != e.cipher || sd.num != e.num || sd.gen != e.gen || memcmp(sd.iv, e.iv, 16) != 0 ||
            sd.layers.size() != e.layers.size() || stored.size() != s.storedSize ||
            hash64(stored.data(), stored.size()) != s.storedHash)
          return false;
        for (size_t i = 0; i < sd.layers.size(); i++)
          if (sd.layers[i].type != e.layers[i].type || sd.layers[i].recon != e.layers[i].recon) return false;
        const Item& it = seg.items[s.item];
        payload.assign(in_.begin() + it.dataStart, in_.begin() + it.dataEnd);
        return true;
      };
      Bytes re;
      if (!decodeDump(dump, 0, nullptr, re, &fast, &b64Orig_)) why = failMessage();
      else if (re != in_) why = "restored file differs from input";
      failClear();
    }
    return why;
  }

  // back to the text before applyLifts(false)
  void undoLifts() {
    int ft = dump.hdr.map.fileOf[CH_T];
    if (dump.hdr.flags & kFlagLiftT) {
      dump.chan[ft].swap(preT_);
      dump.aux[AX_TXN].clear();
    }
    if (dump.hdr.flags & kFlagLiftCS) {
      dump.classFile[CF_CS].swap(preCS_);
      for (int a : {AX_CSN, AX_CSH, AX_CSS, AX_CSI}) dump.aux[a].clear();
    }
    if (dump.hdr.flags & kFlagCmap) {
      dump.classFile[CF_CMAP].swap(preCmap_);
      dump.aux[AX_CMB].clear();
    }
    dump.hdr.flags &= ~(kFlagLiftT | kFlagLiftCS | kFlagCmap);
    preT_.clear();
    preCS_.clear();
    preCmap_.clear();
    liftsUnchecked_ = false;
  }

  // The three transforms are independent: with more than one thread, object text is lifted
  // next to content streams and CMaps (it is a serial step after the parallel stream decoding).
  // check = false: the number lifts skip their own inverse (the final verify covers them) and
  // the original texts are kept for undoLifts.
  void applyLifts(bool check) {
    if (opt_.syntaxLevel < 2) return;
    int ft = dump.hdr.map.fileOf[CH_T];
    Bytes oSk, oNu, oHx, oSs, cSk, cNu, cHx, cSs, cInl, mSk, mNu;
    bool objOk = false, csOk = false, cmOk = false;
    auto runObj = [&] {
      objOk = ft != 0 && !dump.chan[ft].empty() &&
              liftApply(LIFT_OBJ, dump.chan[ft], oSk, oNu, oHx, oSs, nullptr, nullptr, check) && oHx.empty() &&
              oSs.empty();
    };
    auto runCs = [&] {
      if (opt_.syntaxLevel < 3) return;
      csOk = !dump.classFile[CF_CS].empty() &&
             liftApply(LIFT_CS, dump.classFile[CF_CS], cSk, cNu, cHx, cSs, &ctx_, &cInl, check);
      cmOk = !dump.classFile[CF_CMAP].empty() && cmapApply(dump.classFile[CF_CMAP], mSk, mNu);
    };
    unsigned nt = opt_.threads ? opt_.threads : std::max(1u, std::thread::hardware_concurrency());
    if (nt > 1) {
      std::string e1;  // an error the other thread recorded (thread-local), passed on here
      std::thread th([&] {
        runObj();
        if (failed()) e1 = failMessage();
      });
      runCs();
      th.join();
      if (!e1.empty()) fail("%s", e1.c_str());
    } else {
      runObj();
      runCs();
    }
    liftsUnchecked_ = !check;
    if (objOk) {
      dump.chan[ft].swap(oSk);
      if (!check) preT_.swap(oSk);
      dump.aux[AX_TXN].swap(oNu);
      dump.hdr.flags |= kFlagLiftT;
    }
    if (csOk) {
      dump.classFile[CF_CS].swap(cSk);
      if (!check) preCS_.swap(cSk);
      dump.aux[AX_CSN].swap(cNu);
      dump.aux[AX_CSH].swap(cHx);
      dump.aux[AX_CSS].swap(cSs);
      dump.aux[AX_CSI].swap(cInl);
      dump.hdr.flags |= kFlagLiftCS;
    }
    if (cmOk) {
      dump.classFile[CF_CMAP].swap(mSk);
      if (!check) preCmap_.swap(mSk);
      dump.aux[AX_CMB].swap(mNu);
      dump.hdr.flags |= kFlagCmap;
    }
  }

  void emitStored() {
    stored = true;
    dump.hdr.flags = kFlagStored;
    for (auto& a : dump.aux) a.clear();
    dump.blobData.clear();
    dump.blobExt.clear();
    for (auto& c : dump.chan) c.clear();
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
        KeyBuf k2 = objectKey(sec.key, it.num, it.gen, false);
        rc4(k2.data(), k2.size(), p, 2, h);
      } else {
        if (n < 32) continue;
        KeyBuf k2 = ci == CI_AES128 ? objectKey(sec.key, it.num, it.gen, true) : sec.key;
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
    ctx_.verify = !opt_.noVerify;
  }

  // filter chain + parameters as a cache key
  std::string chainKey(const std::vector<FilterRef>& fl) const {
    std::string k;
    for (auto& f : fl) {
      FilterParams fp = params(f.parms);
      char b[96];
      snprintf(b, sizeof(b), "%s/%d/%d/%d/%d/%d;", f.name.c_str(), fp.predictor, fp.colors, fp.bpc, fp.columns,
               fp.earlyChange);
      k += b;
      if (f.name == "CCITTFaxDecode") {
        FilterParams cp = ccittParams(f.parms);
        snprintf(b, sizeof(b), "K%d/%d/%d/%d%d%d%d;", cp.ccittK, cp.ccittCols, cp.ccittRows, cp.ccittEol, cp.ccittAlign,
                 cp.ccittEob, cp.ccittBlack1);
        k += b;
      }
    }
    return k;
  }

  // payload size, filter chain and cipher of a stream, before any decoding
  void streamPrep(StreamRec& s) {
    const Item& it = seg.items[s.item];
    const Obj& d = seg.objs[it.obj];
    s.payload = it.dataEnd - it.dataStart;
    std::vector<FilterRef> fl = getFilters(d);
    for (auto& f : fl) s.chain.push_back(f.name);
    s.wantCipher = streamCipher(d, it.num, fl);
  }

  // identical unencrypted payload + identical chain -> identical result: index of the earlier
  // stream to copy, or -1 (and s is registered for later duplicates)
  int cacheLookup(StreamRec& s) {
    const Item& it = seg.items[s.item];
    const Obj& d = seg.objs[it.obj];
    if (s.wantCipher != CI_NONE || d.get("F") || it.dataEnd - it.dataStart < 64) return -1;
    std::string key = chainKey(getFilters(d));
    uint64_t h = hash64(in_.data() + it.dataStart, it.dataEnd - it.dataStart) ^ hash64((const uint8_t*)key.data(), key.size());
    auto& v = cache_[h];
    for (int k : v) {
      const StreamRec& o = streams[k];
      const Item& oi = seg.items[o.item];
      if (oi.dataEnd - oi.dataStart == it.dataEnd - it.dataStart &&
          memcmp(in_.data() + oi.dataStart, in_.data() + it.dataStart, it.dataEnd - it.dataStart) == 0 &&
          chainKey(getFilters(seg.objs[oi.obj])) == key)
        return k;
    }
    v.push_back((int)(&s - streams.data()));
    return -1;
  }

  void copyResult(StreamRec& s, const StreamRec& o) {
    s.desc.layers = o.desc.layers;
    s.data = o.data;
    s.remain = o.remain;
    s.stop = o.stop;
    s.decoded = o.decoded;
    // the stop filter's parameters from this stream's own chain (same chain key, same position)
    std::vector<FilterRef> fl = getFilters(seg.objs[seg.items[s.item].obj]);
    std::vector<FilterRef> ofl = getFilters(seg.objs[seg.items[o.item].obj]);
    for (size_t k = 0; k < ofl.size() && k < fl.size(); k++)
      if (o.stopParms == ofl[k].parms) s.stopParms = fl[k].parms;
    s.stopLast = o.stopLast;
  }

  // every stream: prep and dedup in file order, the filter work on a thread pool (results do not
  // depend on the thread count), duplicates copied afterwards
  void processAll() {
    std::vector<int> dupOf(streams.size(), -1);
    std::vector<size_t> work;
    for (size_t i = 0; i < streams.size(); i++) {
      streamPrep(streams[i]);
      dupOf[i] = cacheLookup(streams[i]);
      if (dupOf[i] < 0) work.push_back(i);
    }
    // largest first, so one big stream does not finish last on its own
    std::stable_sort(work.begin(), work.end(), [&](size_t a, size_t b) { return streams[a].payload > streams[b].payload; });
    // an error a worker records stops nothing else: it is passed on afterwards, first by stream
    std::vector<std::string> err(streams.size());
    std::atomic<size_t> next{0};
    auto worker = [&]() {
      for (size_t k; (k = next++) < work.size();) {
        failClear();
        processStream(streams[work[k]]);
        if (failed()) err[work[k]] = failMessage();
        failClear();
      }
    };
    unsigned nt = opt_.threads ? opt_.threads : std::max(1u, std::thread::hardware_concurrency());
    nt = (unsigned)std::min<size_t>(nt, work.size());
    // tables that preflate and the AES code build lazily on first use: build them before the pool
    PreflateBlockTrees::staticLitLenTreeDecoder();
    PreflateBlockTrees::staticDistTreeDecoder();
    PreflateBlockTrees::staticLitLenTreeEncoder();
    PreflateBlockTrees::staticDistTreeEncoder();
    Aes::init();
    if (nt <= 1) {
      worker();
    } else {
      std::vector<std::thread> pool;
      for (unsigned t = 0; t < nt; t++) pool.emplace_back(worker);
      for (auto& t : pool) t.join();
    }
    for (auto& e : err)
      if (!e.empty()) {
        fail("%s", e.c_str());
        return;
      }
    for (size_t i = 0; i < streams.size(); i++)
      if (dupOf[i] >= 0) copyResult(streams[i], streams[(size_t)dupOf[i]]);
  }

  // filters (and cipher) of one stream undone as far as they verify; streamPrep has run
  void processStream(StreamRec& s) {
    const Item& it = seg.items[s.item];
    const Obj& d = seg.objs[it.obj];
    Bytes cur(in_.begin() + it.dataStart, in_.begin() + it.dataEnd);
    std::vector<FilterRef> fl = getFilters(d);
    uint8_t ci = s.wantCipher;
    if (ci != CI_NONE) {
      Bytes pt;
      if (sec.key.empty() || !cipherDecrypt(ci, sec.key, it.num, it.gen, cur.data(), cur.size(), pt, s.desc.iv)) {
        s.remain = R_ENCRYPTED;
        s.data.swap(cur);
        s.decoded = s.data.size();
        return;
      }
      Bytes re;
      if (!cipherEncrypt(ci, sec.key, it.num, it.gen, s.desc.iv, pt, re) || re != cur) {
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
      if (!fl.empty()) {
        s.remain = R_ENCODED;
        s.stop = fl[0].name;
        s.stopParms = fl[0].parms;
        s.stopLast = fl.size() == 1;
      }
    } else {
      for (auto& f : fl) {
        if (f.name == "Crypt") continue;
        s.stopParms = f.parms;
        s.stopLast = &f == &fl.back();
        if (f.name == "CCITTFaxDecode" && opt_.ccitt) {
          FilterParams fp = ccittParams(f.parms);
          Bytes out, recon;
          if (layerDecode(ctx_, L_CCITT, fp, cur, out, recon) && s.desc.layers.push_back({L_CCITT, std::move(recon)})) {
            cur.swap(out);
            continue;
          }
        }
        if (isTerminalFilter(f.name)) { s.remain = R_TERMINAL; s.stop = f.name; break; }
        uint8_t lt = layerOfFilter(f.name);
        if (!lt) { s.remain = R_ENCODED; s.stop = f.name; break; }
        FilterParams fp = params(f.parms);
        Bytes out, recon;
        if (s.desc.layers.size() + 4 > kMaxLayers ||  // room for a predictor and pseudo layers
            !layerDecode(ctx_, lt, fp, cur, out, recon) || !s.desc.layers.push_back({lt, std::move(recon)})) {
          s.remain = R_ENCODED;
          s.stop = f.name;
          break;
        }
        cur.swap(out);
        if ((lt == L_FLATE || lt == L_LZW) && fp.predictor > 1) {
          uint8_t pl = fp.predictor == 2 ? L_TIFF : fp.predictor >= 10 ? L_PNG : L_NONE;
          if (!pl || !layerDecode(ctx_, pl, fp, cur, out, recon) || !s.desc.layers.push_back({pl, std::move(recon)})) {
            s.remain = R_ENCODED;
            s.stop = "Predictor";
            break;
          }
          cur.swap(out);
        }
      }
    }
    s.data.swap(cur);
    s.decoded = s.data.size();
  }

  FilterParams ccittParams(const Obj* parms) const {
    FilterParams fp;
    if (!parms) return fp;
    auto geti = [&](const char* k, int& v) {
      const Obj* o = idx_.resolve(parms->get(k));
      if (o && o->t == Obj::INT) v = (int)std::max<int64_t>(-1000000, std::min<int64_t>(1000000, o->i));
    };
    auto getb = [&](const char* k, bool& v) {
      const Obj* o = idx_.resolve(parms->get(k));
      if (o && o->t == Obj::BOOL) v = o->i != 0;
    };
    geti("K", fp.ccittK);
    geti("Columns", fp.ccittCols);
    geti("Rows", fp.ccittRows);
    getb("EndOfLine", fp.ccittEol);
    getb("EncodedByteAlign", fp.ccittAlign);
    getb("EndOfBlock", fp.ccittEob);
    getb("BlackIs1", fp.ccittBlack1);
    return fp;
  }

  // offset bias: with junk before %PDF-, some files count offsets from the header. Pick the bias
  // under which more classic xref entries / XRef-stream-free objects land on object starts.
  void chooseBias() {
    bias_ = 0;
    if (seg.hdrPos == 0) return;
    std::unordered_set<uint64_t> starts;
    for (auto& it : seg.items)
      if (it.kind == Item::OBJ || it.kind == Item::STREAM) starts.insert(it.start);
    size_t hit0 = 0, hitB = 0;
    for (auto& x : seg.xrefs)
      for (auto& e : x.entries)
        if (e.type == 'n') {
          hit0 += starts.count(e.off);
          hitB += starts.count(e.off + seg.hdrPos);
        }
    if (hitB > hit0) bias_ = seg.hdrPos;
  }

  void analyze() {
    chooseBias();
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
    processAll();
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
      return t.remain == R_DONE && !t.bmp && !t.tif ? &t.data : nullptr;
    };
    for (auto& s : streams) {
      if (s.desc.cls != SC_IMAGE || s.remain != R_DONE) continue;
      BmpSpec sp;
      if (!imageBmpSpec(idx_, seg.objs[seg.items[s.item].obj], streamData, sp)) continue;
      Bytes bmp, recon, back;
      if (!bmpWrap(sp, s.data, bmp, recon)) continue;
      if (!layerEncode(ctx_, L_BMP, recon, bmp, back) || back != s.data) continue;
      if (!s.desc.layers.push_back({L_BMP, std::move(recon)})) continue;
      s.data.swap(bmp);
      s.bmp = true;
    }
    // -a: RGB image + 8-bit soft mask of the same size as one BGRA BMP; the mask is restored from it
    if (opt_.rgba) {
      std::vector<char> merged(streams.size(), 0);
      for (size_t i = 0; i < streams.size(); i++) {
        StreamRec& s = streams[i];
        if (!s.bmp || merged[i] || s.desc.layers.empty() || s.desc.layers.back().type != L_BMP) continue;
        const Obj* sm = seg.objs[seg.items[s.item].obj].get("SMask");
        if (!sm || sm->t != Obj::REF) continue;
        auto f = streamsByNum_.find((uint32_t)sm->i);
        if (f == streamsByNum_.end()) continue;
        size_t j = (size_t)f->second.back();
        StreamRec& t = streams[j];
        if (j == i || merged[j] || !t.bmp || t.desc.layers.empty() || t.desc.layers.back().type != L_BMP) continue;
        Bytes rgba, rawS, rawT, chk, arecon;
        if (!rgbaMerge(s.data, t.data, rgba)) continue;
        if (!layerEncode(ctx_, L_BMP, s.desc.layers.back().recon, s.data, rawS) ||
            !layerEncode(ctx_, L_BMP, t.desc.layers.back().recon, t.data, rawT))
          continue;
        if (!rgbaEncodeLayer(s.desc.layers.back().recon, rgba, chk) || chk != rawS) continue;
        putVarTo(arecon, i);
        arecon.insert(arecon.end(), t.desc.layers.back().recon.begin(), t.desc.layers.back().recon.end());
        CodecCtx c2 = ctx_;
        c2.alphaSource = [&](uint64_t k) -> const Bytes* { return k == i ? &rgba : nullptr; };
        if (!alphaEncodeLayer(c2, arecon, chk) || chk != rawT) continue;
        s.desc.layers.back().type = L_RGBA;
        s.data.swap(rgba);
        t.desc.layers.back() = {L_ALPHA, std::move(arecon)};
        t.data.clear();
        t.desc.deferred = true;
        t.desc.payloadLen = t.payload;
        merged[i] = merged[j] = 1;
        st.rgbaMerged++;
      }
    }
    // streams matching a TIFF subformat (own files only) go into a TIFF container
    for (auto& s : streams) {
      if (s.bmp || s.data.size() < opt_.smallThreshold) continue;
      const Obj& d = seg.objs[seg.items[s.item].obj];
      TiffSpec t;
      if (s.remain == R_TERMINAL && s.stop == "CCITTFaxDecode" && s.stopLast) {
        FilterParams fp = ccittParams(s.stopParms);
        const Obj* H = idx_.resolve(d.get2("Height", "H"));
        int64_t rows = fp.ccittRows > 0 ? fp.ccittRows : H && H->t == Obj::INT ? H->i : 0;
        if (rows <= 0 || rows > 0x7FFFFFFF || fp.ccittCols <= 0) continue;
        if (fp.ccittK < 0 && !fp.ccittAlign && !fp.ccittEol) t.compression = 4;
        else if (fp.ccittK >= 0 && fp.ccittEol) {
          t.compression = 3;
          t.t4 = (fp.ccittK > 0 ? 1 : 0) | (fp.ccittAlign ? 4 : 0);
        } else if (fp.ccittK == 0 && fp.ccittAlign) t.compression = 2;
        else continue;
        t.width = (uint32_t)fp.ccittCols;
        t.height = (uint32_t)rows;
        // fax decoders yield 1 = black whatever the photometric says: WhiteIsZero, unless the
        // PDF shows the image the other way round (/BlackIs1 or a high-to-low /Decode)
        ImgColor g;
        bool inv = fp.ccittBlack1 ^ (imageColor(idx_, d, streamData, g) && g.inverted);
        t.photometric = inv ? 1 : 0;
      } else if (s.desc.cls == SC_IMAGE && (s.remain == R_ENCODED || s.remain == R_DONE)) {
        if (!imageTiffSpec(idx_, d, streamData, t)) continue;
        if (s.remain == R_DONE) {
          if (s.data.size() / t.height < ((size_t)t.width * t.spp * t.bps + 7) / 8) continue;
        } else {
          if (!s.stopLast) continue;
          FilterParams fp = params(s.stopParms);
          if (s.stop == "FlateDecode") t.compression = 8;
          else if (s.stop == "LZWDecode" && fp.earlyChange == 1) t.compression = 5;
          else if (s.stop == "RunLengthDecode") t.compression = 32773;
          else continue;
          if (s.stop != "RunLengthDecode" && fp.predictor == 2) {
            if (fp.colors != t.spp || fp.bpc != t.bps || fp.columns != (int)t.width || (t.bps != 8 && t.bps != 16)) continue;
            t.predictor = 2;
          } else if (s.stop != "RunLengthDecode" && fp.predictor > 1) {
            continue;
          }
        }
      } else {
        continue;
      }
      Bytes tif, back;
      if (!tiffWrap(t, s.data, tif)) continue;
      if (!layerEncode(ctx_, L_TIFW, Bytes(), tif, back) || back != s.data) continue;
      if (!s.desc.layers.push_back({L_TIFW, Bytes()})) continue;
      s.data.swap(tif);
      s.tif = true;
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
      if (!s.desc.layers.push_back({L_T1, std::move(recon)})) continue;
      s.data.swap(out);
      s.t1 = true;
    }
    // XMP packets with a base64 JPEG thumbnail: the JPEG becomes the stored form (.jpg)
    for (auto& s : streams) {
      if (s.remain != R_DONE || s.bmp || s.tif || s.t1 || s.data.size() < 256) continue;
      // not for streams that go to a class file or get a structural layer below
      uint8_t c = s.desc.cls;
      if (c == SC_CONTENT || c == SC_CMAP || c == SC_OBJSTM || c == SC_XREF) continue;
      static const char kTag[] = "GImg:image>";
      if (std::search(s.data.begin(), s.data.end(), kTag, kTag + sizeof(kTag) - 1) == s.data.end()) continue;
      Bytes jpeg, recon, back;
      if (!xmpThumbSplit(s.data, jpeg, recon)) continue;
      if (!layerEncode(ctx_, L_XMPJPG, recon, jpeg, back) || back != s.data) continue;
      if (!s.desc.layers.push_back({L_XMPJPG, std::move(recon)})) continue;
      s.data.swap(jpeg);
      st.xmpThumbs++;
    }
    // well-known ICC profiles: a table index instead of the data
    for (auto& s : streams) {
      if (s.remain != R_DONE || s.bmp || s.tif || s.data.size() < 128) continue;
      int k = iccLookup(s.data);
      if (k < 0) continue;
      Bytes recon{(uint8_t)k}, back;
      if (!layerEncode(ctx_, L_ICCREF, recon, Bytes(), back) || back != s.data) continue;
      if (!s.desc.layers.push_back({L_ICCREF, std::move(recon)})) continue;
      s.data.clear();
      st.iccRefs++;
    }
    // object streams: header table derived from the members
    for (auto& s : streams) {
      if (s.desc.cls != SC_OBJSTM || s.remain != R_DONE || opt_.syntaxLevel < 2) continue;
      const Obj& d = seg.objs[seg.items[s.item].obj];
      const Obj* N = idx_.resolve(d.get("N"));
      const Obj* F = idx_.resolve(d.get("First"));
      if (!N || !F || N->t != Obj::INT || F->t != Obj::INT) continue;
      FilterParams fp;
      fp.objstmN = N->i;
      fp.objstmFirst = F->i;
      Bytes out, recon;
      if (!layerDecode(ctx_, L_OBJSTM, fp, s.data, out, recon)) continue;
      if (!s.desc.layers.push_back({L_OBJSTM, std::move(recon)})) continue;
      s.data.swap(out);
    }
    // XRef streams: rows predicted from object positions known to the decoder at that point
    for (auto& s : streams) {
      if (s.desc.cls != SC_XREF || s.remain != R_DONE || opt_.syntaxLevel < 2) continue;
      const Obj& d = seg.objs[seg.items[s.item].obj];
      const Obj* W = idx_.resolve(d.get("W"));
      const Obj* Ix = idx_.resolve(d.get("Index"));
      const Obj* Sz = idx_.resolve(d.get("Size"));
      if (!W || W->t != Obj::ARR || W->a.size() != 3) continue;
      FilterParams fp;
      bool ok = true;
      for (auto& w : W->a) {
        if (w.t != Obj::INT || w.i < 0 || w.i > 8) ok = false;
        fp.xrefW.push_back(w.t == Obj::INT ? (uint64_t)w.i : 0);
      }
      if (Ix && Ix->t == Obj::ARR && Ix->a.size() % 2 == 0) {
        for (size_t k = 0; k + 1 < Ix->a.size(); k += 2) {
          if (Ix->a[k].t != Obj::INT || Ix->a[k + 1].t != Obj::INT || Ix->a[k].i < 0 || Ix->a[k + 1].i < 0) ok = false;
          else fp.xrefIndex.push_back({(uint64_t)Ix->a[k].i, (uint64_t)Ix->a[k + 1].i});
        }
      } else if (!Ix && Sz && Sz->t == Obj::INT && Sz->i > 0) {
        fp.xrefIndex.push_back({0, (uint64_t)Sz->i});
      } else {
        ok = false;
      }
      if (!ok) continue;
      // positions known when the stream is written, and all positions (deferred restore)
      ObjPositions snap, all;
      for (int i = 0; i < (int)seg.items.size(); i++) {
        const Item& it = seg.items[i];
        if (it.kind != Item::OBJ && it.kind != Item::STREAM) continue;
        uint32_t num, gen;
        size_t hl = (it.kind == Item::STREAM ? it.dataStart : it.end) - it.start;
        if (!objHeaderOf(in_.data() + it.start, hl, num, gen)) continue;
        if (i <= s.item) snap.add(num, gen, it.start - bias_);
        all.add(num, gen, it.start - bias_);
      }
      Bytes out, recon, out2, recon2;
      ctx_.objPos = &snap;
      bool done = layerDecode(ctx_, L_XREFROWS, fp, s.data, out, recon);
      ctx_.objPos = &all;
      bool done2 = opt_.derive && layerDecode(ctx_, L_XREFROWS, fp, s.data, out2, recon2);
      ctx_.objPos = nullptr;
      auto nonzero = [](const Bytes& b) {
        size_t z = 0;
        for (uint8_t c : b) z += c != 0;
        return z;
      };
      if (done2 && (!done || nonzero(out2) + 8 < nonzero(out))) {
        // the descriptor must carry the payload size; /DL-style facts would differ, so only
        // when nothing in the prefix needs the decoded data
        std::vector<DerivTok> toks;
        const Item& it = seg.items[s.item];
        derivWalk(in_.data() + it.start, it.dataStart - it.start, toks);
        bool needDecoded = false;
        for (auto& t : toks)
          if (t.path == "/DL" || t.path == "/Length1" || t.path == "/Length3" || t.path == "/First" ||
              t.path == "/Params/Size" || t.path == "/Params/CheckSum")
            needDecoded = true;
        if (!needDecoded) {
          s.desc.deferred = true;
          s.desc.payloadLen = s.payload;
          done = true;
          out.swap(out2);
          recon.swap(recon2);
        }
      }
      if (!done) continue;
      if (!s.desc.layers.push_back({L_XREFROWS, std::move(recon)})) continue;
      s.data.swap(out);
    }
    // extension and storage
    for (auto& s : streams) {
      uint8_t cls = s.desc.cls;
      std::string ext;
      if (s.tif) {
        ext = "tif";
      } else if (s.remain == R_ENCRYPTED) {
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

  // own-file ordinals in file order, and duplicates, decided before any item is written: a font
  // dictionary can name its font program's file before the program's own item comes
  void assignOrdinals() {
    ownOrd_.assign(streams.size(), UINT32_MAX);
    ordExt_.clear();
    streamByNum_.clear();
    widthArrays_.clear();
    numsArrays_.clear();
    for (auto& e : idx_.byNum) {  // width arrays and /Nums values that are objects of their own
      const Obj* o = e.second;
      if (!o || o->t != Obj::DICT) continue;
      const Obj* nums = o->get("Nums");
      if (nums && nums->t == Obj::ARR)
        for (size_t k = 0; k + 1 < nums->a.size(); k += 2)
          if (nums->a[k].t == Obj::INT && nums->a[k].i >= 0 && nums->a[k].i <= 0xFFFFFFFFll && nums->a[k + 1].t == Obj::REF &&
              nums->a[k + 1].i >= 0 && nums->a[k + 1].i <= 0xFFFFFFFFll)
            numsArrays_[(uint32_t)nums->a[k + 1].i] = (uint32_t)nums->a[k].i;
      for (const char* k : {"Widths", "W"}) {
        const Obj* w = o->get(k);
        if (w && w->t == Obj::REF && w->i >= 0 && w->i <= 0xFFFFFFFFll) widthArrays_[(uint32_t)w->i] = o;
      }
    }
    std::unordered_map<uint64_t, std::vector<size_t>> seen;
    uint32_t next = 0;
    for (size_t i = 0; i < seg.items.size(); i++) {
      if (seg.items[i].kind != Item::STREAM) continue;
      size_t si = (size_t)streamOfItem_[(int)i];
      StreamRec& s = streams[si];
      streamByNum_[seg.items[i].num] = si;
      if (s.desc.storage != ST_FILE) continue;
      if (opt_.dedup) {
        uint64_t h = hash64(s.data.data(), s.data.size()) ^ ((uint64_t)s.data.size() * 0x9E3779B97F4A7C15ull);
        bool dup = false;
        for (size_t k : seen[h])
          if (streams[k].data == s.data) {
            s.desc.storage = ST_DUP;
            s.desc.ordinal = ownOrd_[k];
            dup = true;
            break;
          }
        if (dup) continue;
        seen[h].push_back(si);
      }
      ownOrd_[si] = next++;
      ordExt_.push_back(s.desc.ext);
    }
  }

  // a stream as a width source: its own file ("o<ord>.<ext>") or, stored inline in an item
  // already written, "s<num>"; data = its stored form
  bool streamSource(uint32_t num, std::string& src, const Bytes*& data, const StreamRec*& rec) {
    auto sf = streamByNum_.find(num);
    if (sf == streamByNum_.end()) return false;
    const StreamRec& s = streams[sf->second];
    if (s.remain != R_DONE) return false;
    rec = &s;
    if (s.desc.storage == ST_FILE || s.desc.storage == ST_DUP) {
      uint32_t ord = s.desc.storage == ST_DUP ? s.desc.ordinal : ownOrd_[sf->second];
      if (ord >= ordExt_.size()) return false;
      src = "o" + decStr(ord) + "." + ordExt_[ord];
      data = !s.data.empty() ? &s.data : ord < dump.ownData.size() ? &dump.ownData[ord] : nullptr;
      return data != nullptr;
    }
    auto f = inlineSeen_.find(num);
    if (!inlineSourceOk(s.desc, s.data.size()) || f == inlineSeen_.end() || f->second != sf->second) return false;
    src = "s" + decStr(num);
    data = &s.data;
    return true;
  }

  // kind, header fields and metrics for the widths of font dictionary o
  struct WidthFont {
    int kind;
    uint32_t first = 0;
    std::string fields;
    const FontMetrics* m = nullptr;
  };
  bool widthFont(const Obj& o, WidthFont& wf) {
    const Obj* sub = o.get("Subtype");
    if (!sub || sub->t != Obj::NAME) return false;
    const Obj* fd = idx_.resolve(o.get("FontDescriptor"));
    if (!fd || fd->t != Obj::DICT) return false;
    const char* ffKey;
    if (sub->s == "TrueType") { wf.kind = WK_TT; ffKey = "FontFile2"; }
    else if (sub->s == "Type1" || sub->s == "MMType1") {
      wf.kind = fd->get("FontFile3") ? WK_CFF : WK_T1;
      ffKey = wf.kind == WK_CFF ? "FontFile3" : "FontFile";
    } else if (sub->s == "CIDFontType2") { wf.kind = WK_CID2; ffKey = "FontFile2"; }
    else if (sub->s == "CIDFontType0") { wf.kind = WK_CID0; ffKey = "FontFile3"; }
    else return false;
    const Obj* ff = fd->get(ffKey);
    if (!ff || ff->t != Obj::REF) return false;
    std::string src, mapSrc, t1hex;
    const Bytes* data;
    const Bytes* mapData = nullptr;
    const StreamRec* rec;
    if (!streamSource((uint32_t)ff->i, src, data, rec)) return false;
    if (wf.kind == WK_CID2) {
      const Obj* m = o.get("CIDToGIDMap");
      if (m && m->t == Obj::REF) {
        const StreamRec* mr;
        if (!streamSource((uint32_t)m->i, mapSrc, mapData, mr)) return false;
        wf.kind = WK_CID2M;
      } else if (m && !(m->t == Obj::NAME && m->s == "Identity")) {
        return false;
      }
    }
    Bytes recon;
    if (wf.kind == WK_T1) {
      if (rec->desc.layers.empty() || rec->desc.layers.back().type != L_T1) return false;
      recon = rec->desc.layers.back().recon;
      static const char* hx = "0123456789abcdef";
      t1hex = "h";
      for (uint8_t c : recon) { t1hex += hx[c >> 4]; t1hex += hx[c & 15]; }
    }
    if (wf.kind != WK_CID2 && wf.kind != WK_CID2M && wf.kind != WK_CID0) {
      const Obj* fc = idx_.resolve(o.get("FirstChar"));
      if (!fc || fc->t != Obj::INT || fc->i < 0 || fc->i > 0xFFFF) return false;
      wf.first = (uint32_t)fc->i;
    }
    std::string key = std::string(1, (char)('a' + wf.kind)) + " " + src + " " + t1hex + " " + mapSrc;
    bool found;
    FontMetrics* fm = fontCache_.get(key, found);  // valid until the next get
    if (!found && parseFontOf(wf.kind, *data, recon, *fm) && wf.kind == WK_CID2M)
      fm->setCidMap(mapData->data(), mapData->size());
    if (!fm->ok) return false;
    wf.m = fm;
    wf.fields = " " + src + " " + decStr(wf.first) + " ";
    if (wf.kind == WK_T1) wf.fields += t1hex + " ";
    if (wf.kind == WK_CID2M) wf.fields += mapSrc + " ";
    return true;
  }

  // /Widths and /W from the font programs (plan2 P8): in a font dictionary (direct array) or in
  // an array object a font dictionary names. p: the object's value (no header); edits at base +.
  // top: a top-level object's value, right after "obj": a placeholder there must not touch the
  // keyword (the decoder reads "N G obj" before it resolves any placeholder)
  void widthsDerive(const uint8_t* p, size_t n, size_t base, uint32_t num, bool top, std::vector<DerivEdit>& edits) {
    size_t at = SIZE_MAX;
    const Obj* fontDict = nullptr;
    Obj o;
    size_t i0 = 0;
    while (i0 < n && isWS(p[i0])) i0++;
    bool after = false;
    if (i0 < n && p[i0] == '[') {
      after = top && i0 == 0;
      auto f = widthArrays_.find(num);
      if (f == widthArrays_.end()) return;
      fontDict = f->second;
      at = i0;
    } else {
      if (!memfind(p, n, "/Widths", 7) && !memfind(p, n, "/W", 2)) return;
      Lexer lx(p, n, 0);
      if (!lx.parse(o) || o.t != Obj::DICT) return;
      const Obj* sub = o.get("Subtype");
      bool cid = sub && sub->t == Obj::NAME && (sub->s == "CIDFontType2" || sub->s == "CIDFontType0");
      const Obj* arr = o.get(cid ? "W" : "Widths");
      if (!arr || arr->t != Obj::ARR) return;
      at = topKeyValue(p, n, cid ? "/W" : "/Widths");
      if (at == SIZE_MAX || p[at] != '[') return;
      fontDict = &o;
    }
    WidthFont wf;
    if (!widthFont(*fontDict, wf)) return;
    std::vector<DerivEdit> we;
    size_t got = widthsChoose(p, n, at, wf.kind, wf.fields, wf.first, *wf.m, we, after);
    for (auto& e : we) edits.push_back({e.a + base, e.b + base, e.rep});
    if (got) {
      st.widthArrays++;
      st.widthsDerived += got;
      st.widthsByKind[wf.kind] += got;
    }
  }

  // /ParentTree leaves: refs predicted from page /StructParents and the structure elements' MCIDs
  // member: ObjStm member index, -1 for a top-level object
  void numsDerive(const uint8_t* p, size_t n, size_t base, uint32_t num, long member, std::vector<DerivEdit>& edits) {
    size_t i0 = 0;
    while (i0 < n && isWS(p[i0])) i0++;
    if (i0 < n && p[i0] == '[') {  // an array object that is a /Nums value
      auto f = numsArrays_.find(num);
      if (f != numsArrays_.end())  // "obj[": header after the bracket (see widthsDerive)
        st.parentTreeRefs += numsChoose(p, n, i0, base, sm_, edits, (int64_t)f->second, member, member < 0 && i0 == 0);
      return;
    }
    if (!memfind(p, n, "/Nums", 5)) return;
    size_t at = topKeyValue(p, n, "/Nums");
    if (at == SIZE_MAX || p[at] != '[') return;
    st.parentTreeRefs += numsChoose(p, n, at, base, sm_, edits, -1, member);
  }

  // ObjStm members: /P and widths placeholders inside the member text; every member's /K goes into
  // the structure map in member order. False if nothing was replaced (the text stays as it is).
  bool membersDerive(const StreamRec& s, Bytes& out) {
    ObjstmMembers mem;
    if (!mem.init(s.desc)) return false;
    const Bytes& M = s.data;
    std::vector<DerivEdit> ed;
    size_t N = mem.nums.size();
    for (size_t k = 0; k < N; k++) {
      uint64_t a, b = M.size();
      if (!mem.start(M.data(), M.size(), k, a) || (k + 1 < N && !mem.start(M.data(), M.size(), k + 1, b))) break;
      const uint8_t* v = M.data() + a;
      size_t vn = (size_t)(b - a);
      uint32_t num = (uint32_t)mem.nums[k];
      if (structChoose(v, vn, (size_t)a, num, (long)k, sm_, ed)) st.structParents++;
      widthsDerive(v, vn, (size_t)a, num, false, ed);
      numsDerive(v, vn, (size_t)a, num, (long)k, ed);
      structAddKids(v, vn, num, 0, sm_);
    }
    if (ed.empty()) return false;
    derivEncode(M.data(), M.size(), ed, out);
    st.membersDerived++;
    return true;
  }

  void emit() {
    ChanWriter w(dump);
    EmitState es;
    assignOrdinals();
    ObjPositions objPos;
    DeriveState ds;
    StreamFacts noFacts;
    Bytes txt;
    std::vector<DerivEdit> edits;
    ctx_.objPos = &objPos;
    FwdAnchors fa;
    for (auto& it : seg.items) fa.starts.push_back(it.start);
    fa.fileSize = in_.size();
    fa.bias = bias_;
    dump.hdr.offsetBias = bias_;
    std::function<int64_t(uint64_t)> itemAt = [&](uint64_t off) -> int64_t {
      off += bias_;
      auto f = std::lower_bound(fa.starts.begin(), fa.starts.end(), off);
      return f != fa.starts.end() && *f == off ? (int64_t)(f - fa.starts.begin()) : -1;
    };
    // item text with derived fields replaced (kind: 0 text, 1 object, 2 stream prefix, 3 plain)
    auto derived = [&](size_t a, size_t b, int kind, uint8_t cls, const StreamFacts& f, uint32_t num) {
      edits.clear();
      if (kind < 3 && opt_.derive) {
        derivChoose(in_.data() + a, b - a, kind, cls, f, ds, num, edits);
        fwdChoose(in_.data() + a, b - a, kind, fa, edits);
      }
      // signature blobs: DER to .p7s
      if ((kind == 1 || kind == 2) && opt_.derive) {
        size_t before = edits.size();
        sigStrings(in_.data() + a, b - a, edits, dump.aux[AX_P7S]);
        st.sigBlobs += edits.size() - before;
        before = edits.size();
        b64Strings(ctx_, in_.data() + a, b - a, edits, dump.aux[AX_B64], dump.aux[AX_B64R], &b64Orig_, st.b64Bytes,
                   dump.blobData, dump.blobExt);
        st.b64Blobs += edits.size() - before;
      }
      // strings of top-level objects of an encrypted file: stored decrypted
      if ((kind == 1 || kind == 2) && opt_.derive && !sec.key.empty() && num != sec.encNum && cls != SC_XREF) {
        uint32_t n2, g2;
        if (objHeaderOf(in_.data() + a, b - a, n2, g2)) {
          DerivCrypt dc{&sec.key, n2, g2};
          uint8_t ci = sec.V >= 4 ? sec.filterCipher(sec.strF) : CI_RC4;
          size_t before = edits.size();
          if (ci != CI_NONE) cryptStrings(in_.data() + a, b - a, ci, dc, edits);
          st.stringsDecrypted += edits.size() - before;
        }
      }
      size_t body = SIZE_MAX;
      uint32_t hn = 0, hg = 0;
      if (kind == 1 && opt_.derive && objHeaderOf(in_.data() + a, b - a, hn, hg)) {
        body = objBodyStart(in_.data() + a, b - a);
        if (body != SIZE_MAX) {
          if (structChoose(in_.data() + a + body, b - a - body, body, hn, -1, sm_, edits)) st.structParents++;
          widthsDerive(in_.data() + a + body, b - a - body, body, hn, true, edits);
          numsDerive(in_.data() + a + body, b - a - body, body, hn, -1, edits);
        }
      }
      derivEncode(in_.data() + a, b - a, edits, txt);
      st.derived += edits.size();
      if (body != SIZE_MAX) structAddKids(in_.data() + a + body, b - a - body, hn, hg, sm_);
    };
    for (size_t i = 0; i < seg.items.size(); i++) {
      const Item& it = seg.items[i];
      size_t len = it.end - it.start;
      if (it.kind == Item::OBJ || it.kind == Item::STREAM) {
        uint32_t num, gen;
        size_t hl = (it.kind == Item::STREAM ? it.dataStart : it.end) - it.start;
        if (objHeaderOf(in_.data() + it.start, hl, num, gen)) objPos.add(num, gen, it.start - bias_);
      }
      st.itemCount[it.kind]++;
      st.itemBytes[it.kind] += len;
      switch (it.kind) {
        case Item::TEXT:
          w.u8(CH_S, I_TEXT);
          derived(it.start, it.end, 0, 0, noFacts, 0);
          w.lenBytes(CH_N, CH_T, txt);
          break;
        case Item::RAW:
          w.u8(CH_S, I_RAW);
          w.lenBytes(CH_N, CH_B, in_.data() + it.start, len);
          break;
        case Item::OBJ: {
          w.u8(CH_S, I_OBJ);
          uint32_t num = 0, gen;
          objHeaderOf(in_.data() + it.start, len, num, gen);
          derived(it.start, it.end, 1, 0, noFacts, num);
          itemText(w, IL_OBJ, txt);
          break;
        }
        case Item::XREF:
          w.u8(CH_S, I_XREF);
          ds.xrefPos.push_back(it.start - bias_);
          ds.lastXrefEnd = xrefEnd(seg.xrefs[it.xref].subs);
          writeXref(w, seg.xrefs[it.xref], objPos, opt_.derive ? &itemAt : nullptr);
          break;
        case Item::STREAM: {
          size_t si = (size_t)streamOfItem_[(int)i];
          StreamRec& s = streams[si];
          w.u8(CH_S, I_STREAM);
          accountStream(s);
          Bytes members;
          if (opt_.derive && objstmMembersOf(s.desc) && membersDerive(s, members)) s.desc.membersDerived = true;
          // facts for derived fields come from the stored form, exactly as the decoder sees it
          StreamFacts f;
          f.payloadKnown = true;
          f.payloadLen = s.payload;
          {
            std::vector<DerivTok> toks;
            derivWalk(in_.data() + it.start, it.dataStart - it.start, toks);
            bool wantDec = false, wantMd5 = false;
            for (auto& t : toks) {
              if (t.path == "/DL" || t.path == "/Length1" || t.path == "/Length3" || t.path == "/First" ||
                  t.path == "/Params/Size")
                wantDec = true;
              if (t.path == "/Params/CheckSum") wantMd5 = true;
            }
            if (opt_.derive) streamFacts(ctx_, s.desc, s.data, wantDec, wantMd5, f);
          }
          uint32_t num = 0, gen;
          objHeaderOf(in_.data() + it.start, it.dataStart - it.start, num, gen);
          derived(it.start, it.dataStart, 2, s.desc.cls, f, num);
          Bytes prefix;
          prefix.swap(txt);
          writeStreamDesc(w, es, s.desc, s.desc.membersDerived ? members : s.data);
          {
            uint32_t pn, pg;
            if (inlineSourceOk(s.desc, s.data.size()) &&
                objHeaderOf(in_.data() + it.start, it.dataStart - it.start, pn, pg))
              inlineSeen_[pn] = si;
          }
          if (s.desc.storage == ST_FILE && s.desc.ordinal != ownOrd_[si]) fail("own file ordinal out of order");
          itemText(w, IL_PRE, prefix);
          uint32_t lr;
          if (lengthRefOf(in_.data() + it.start, it.dataStart - it.start, lr)) ds.lenRef[lr] = s.payload;
          if (s.desc.cls == SC_XREF) {
            ds.xrefPos.push_back(it.start - bias_);
            ds.lastXRefStm = (int64_t)(it.start - bias_);
          }
          derived(it.dataEnd, it.end, 3, 0, noFacts, 0);
          itemText(w, IL_SUF, txt);
          break;
        }
      }
    }
    w.u8(CH_S, I_END);
    ctx_.objPos = nullptr;
    st.reconBlobs = es.recon.count;
    st.reconRefs = es.recon.refs;
  }

  void accountStream(StreamRec& s) {
    const Item& it = seg.items[s.item];
    s.storedSize = s.data.size();
    s.storedHash = hash64(s.data.data(), s.data.size());
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
  if (st.rgbaMerged) printf("RGB + soft mask merged (-a): %llu\n", (unsigned long long)st.rgbaMerged);
  if (st.xmpThumbs) printf("XMP thumbnails as JPEG: %llu\n", (unsigned long long)st.xmpThumbs);
  if (st.iccRefs) printf("ICC profiles from the built-in table: %llu\n", (unsigned long long)st.iccRefs);
  if (st.itemLenPredicted) printf("item lengths predicted from the text: %llu\n", (unsigned long long)st.itemLenPredicted);
  if (st.structParents) printf("structure parents (/P) from /K: %llu\n", (unsigned long long)st.structParents);
  if (st.membersDerived) printf("ObjStm streams with derived member fields: %llu\n", (unsigned long long)st.membersDerived);
  if (st.parentTreeRefs) printf("structure parent tree entries from /K: %llu\n", (unsigned long long)st.parentTreeRefs);
  if (st.widthArrays) {
    printf("widths from the embedded fonts: %llu in %llu arrays (", (unsigned long long)st.widthsDerived,
           (unsigned long long)st.widthArrays);
    static const char* const kn[WK_COUNT] = {"TrueType", "Type1C", "CIDFontType2", "Type 1", "CIDFontType0", "CIDFontType2 map"};
    bool sp = false;
    for (int k = 0; k < WK_COUNT; k++)
      if (st.widthsByKind[k]) { printf("%s%s %llu", sp ? ", " : "", kn[k], (unsigned long long)st.widthsByKind[k]); sp = true; }
    printf(")\n");
  }
  if (st.derived)
    printf("derived fields: %llu\n", (unsigned long long)(st.derived - st.stringsDecrypted - st.sigBlobs - st.b64Blobs));
  if (st.b64Blobs)
    printf("base64 blobs in object text: %llu (%llu bytes decoded to .b64)\n", (unsigned long long)st.b64Blobs,
           (unsigned long long)st.b64Bytes);
  if (st.sigBlobs)
    printf("signature blobs: %llu (%llu bytes of DER in .p7s)\n", (unsigned long long)st.sigBlobs,
           (unsigned long long)E.dump.aux[AX_P7S].size());
  if (st.stringsDecrypted) printf("strings stored decrypted: %llu\n", (unsigned long long)st.stringsDecrypted);
  printf("dump:");
  uint64_t total = 0;
  for (size_t f = 0; f < E.dump.hdr.map.nfiles; f++) {
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
  if (gObjstmExact + gObjstmInexact)
    printf("objstm headers: %llu offsets exact, %llu with corrections, %llu with sequential numbers\n",
           (unsigned long long)gObjstmExact, (unsigned long long)gObjstmInexact, (unsigned long long)gObjstmNumsSeq);
  printf("time: encode %.2fs (preflate: %llu streams, %s unpacked, analyze %.2fs, re-encode %.2fs; number lifting %.2fs), verify %.2fs\n",
         E.tEncode, (unsigned long long)gFlateProf.streams, human(gFlateProf.unpacked).c_str(), gFlateProf.dec,
         gFlateProf.enc, E.tLift, E.tVerify);
  if (opt.verbose) {
    printf("streams:\n  %-6s %-9s %-12s %-28s %-9s %9s %9s %7s %-6s %s\n", "item", "obj", "class", "filters", "state",
           "payload", "stored", "recon", "store", "ext/layers");
    for (auto& s : E.streams) {
      const Item& it = E.seg.items[s.item];
      std::string fl;
      for (auto& f : s.chain) fl += (fl.empty() ? "" : ",") + f;
      if (fl.empty()) fl = "-";
      std::string ly;
      for (auto& l : s.desc.layers) ly += std::string(ly.empty() ? "" : "+") + kLayerName[l.type];
      char obj[32];
      snprintf(obj, sizeof(obj), "%u %u", it.num, it.gen);
      size_t rc = 0;
      for (auto& l : s.desc.layers) rc += l.recon.size();
      printf("  %-6d %-9s %-12s %-28s %-9s %9zu %9zu %7zu %-6s .%s %s%s%s\n", s.item, obj, kClassName[s.desc.cls],
             fl.c_str(), kRemainName[s.remain], s.payload, s.storedSize, rc, kStorageName[s.desc.storage],
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
  auto keyBytes = [](const KeyBuf& k) { return Bytes(k.begin(), k.end()); };
  check("md5(abc)", hex(Bytes(m, m + 16)), "900150983cd24fb0d6963f7d28e17f72");
  check("sha256(abc)", hex(keyBytes(sha256((const uint8_t*)"abc", 3))),
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
  check("sha384(abc)", hex(keyBytes(sha512x((const uint8_t*)"abc", 3, true))),
        "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7");
  check("sha512(abc)", hex(keyBytes(sha512x((const uint8_t*)"abc", 3, false))),
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
  {
    Bytes strip, tif, back;
    for (int i = 0; i < 777; i++) strip.push_back((uint8_t)(i * 37));
    TiffSpec t;
    t.width = 7;
    t.height = 3;
    t.bps = 2;
    t.photometric = 3;
    grayRamp(t.palette, 2, false);
    uint32_t w = 0, h = 0, cm = 0;
    bool ok = tiffWrap(t, strip, tif) && tifwEncodeLayer(Bytes(), tif, back) && back == strip &&
              tiffTag(tif, 256, w) && tiffTag(tif, 257, h) && w == 7 && h == 3 && !tiffTag(tif, 320, cm);
    t = TiffSpec();
    t.width = 5;
    t.height = 9;
    t.bps = 16;
    t.spp = 4;
    t.photometric = 5;
    t.compression = 8;
    t.predictor = 2;
    ok = ok && tiffWrap(t, strip, tif) && tifwEncodeLayer(Bytes(), tif, back) && back == strip;
    Bytes cut(tif.begin(), tif.end() - 1);
    ok = ok && !tifwEncodeLayer(Bytes(), cut, back) && !tifwEncodeLayer(Bytes{1}, tif, back);
    check("tiff wrap round trip", ok ? "ok" : "mismatch", "ok");
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
          "  -s N     syntax depth: 0 stored, 1 items + raw object text, 2 + object structure and\n"
          "           derived fields, 3 + content streams and CMaps (default %d)\n"
          "  -p N     small-stream threshold: streams below N bytes share <P>.bin (default 1024)\n"
          "  -D       disable dedup of extracted streams\n"
          "  -d       do not derive computable fields (/Length, xref offsets, ...)\n"
          "  -g       decode CCITT fax images (G3 1D, G4) to bitmaps (BMP); off by default: xz\n"
          "           compresses the bitmaps worse than the fax data, a bilevel recompressor may not.\n"
          "           Without -g, fax data whose parameters match a TIFF subformat is stored as .tif\n"
          "  -a       RGB images with an 8-bit /SMask of the same size: both in one 32 bpp BMP (BGRA);\n"
          "           xz -3%% on those images, off by default for BMP recompressors without alpha\n"
          "  -c MAP   channel->file map, e.g. S,N,T,X,O,F,R,B (default %s)\n"
          "  -n       no verification: skip the final in-memory restore and the per-stream Flate re-encode\n"
          "  -j N     decode streams on N threads (default: one per CPU); the dump does not depend on N\n"
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
        if (i + 1 >= argc) die("option -%c needs a value", o);
        return argv[++i];
      };
      auto num = [&]() -> long long {
        std::string v = val();
        char* e;
        long long r = strtoll(v.c_str(), &e, 10);
        if (*e || r < 0) die("bad value for -%c: %s", o, v.c_str());
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
        case 'd': opt.derive = false; break;
        case 'g': opt.ccitt = true; break;
        case 'j': opt.threads = (unsigned)std::min(256LL, num()); break;
        case 'a': opt.rgba = true; break;
        case 'c': opt.chanMap = val(); break;
        case 'n': opt.noVerify = true; break;
        case 'v': opt.verbose++; break;
        case 'q': opt.quiet = true; break;
        default: die("unknown option %s", a.c_str());
      }
    } else {
      pos.push_back(a);
    }
  }
  if (mode == "i") {
    if (pos.size() != 1) { usage(); return 2; }
    Bytes in;
    if (!readFile(pos[0], in)) die("cannot read %s", pos[0].c_str());
    Encoder E(in, opt);
    E.run();
    printInfo(pos[0].c_str(), in, E, opt);
    return 0;
  }
  if (pos.size() != 2) { usage(); return 2; }
  if (mode == "c") {
    Bytes in;
    if (!readFile(pos[0], in)) die("cannot read %s", pos[0].c_str());
    Encoder E(in, opt);
    E.run();
    E.writeFiles(pos[1]);
    if (opt.verbose) printInfo(pos[0].c_str(), in, E, opt);
    else if (!opt.quiet) {
      uint64_t total = 0;
      for (size_t f = 0; f < E.dump.hdr.map.nfiles; f++) total += E.chanFileSize(f);
      for (auto& c : E.dump.classFile) total += c.size();
      for (auto& a : E.dump.aux) total += a.size();
      for (auto& b : E.dump.ownData) total += b.size();
      for (auto& b : E.dump.blobData) total += b.size();
      fprintf(stderr, "%s: %llu -> %llu bytes in %zu files%s\n", pos[0].c_str(), (unsigned long long)in.size(),
              (unsigned long long)total, E.dump.ownData.size() + E.dump.blobData.size() + E.dump.hdr.map.nfiles,
              E.stored ? (" (stored: " + E.storedReason + ")").c_str() : "");
    }
    return 0;
  }
  // d
  const std::string& prefix = pos[0];
  DumpFiles d;
  Bytes meta;
  if (!readFile(prefix + ".meta", meta)) die("cannot read %s.meta", prefix.c_str());
  size_t meta0 = 0;
  if (!parseHeader(meta, d.hdr, meta0)) die("%s", failMessage().c_str());
  {
    uint64_t tot = 0;
    for (int a = 0; a < AX_COUNT; a++) {
      if (d.hdr.inlineLen[a] > meta.size()) die("inline file overrun");
      tot += d.hdr.inlineLen[a];
    }
    if (tot > meta.size() - meta0) die("inline file overrun");
    size_t q = meta.size() - (size_t)tot;
    for (int a = 0; a < AX_COUNT; a++) {
      d.aux[a].assign(meta.begin() + q, meta.begin() + q + (size_t)d.hdr.inlineLen[a]);
      q += (size_t)d.hdr.inlineLen[a];
    }
    meta.resize(meta.size() - (size_t)tot);
  }
  d.chan[0].swap(meta);
  auto need = [&](const std::string& name, int bit, Bytes& out) {
    if (!(d.hdr.present >> bit & 1)) return;
    if (!readFile(name, out)) die("cannot read %s", name.c_str());
  };
  for (size_t f = 1; f < d.hdr.map.nfiles; f++) need(prefix + "." + d.hdr.map.suffix[f].c_str(), (int)f, d.chan[f]);
  for (int c = 0; c < CF_COUNT; c++) need(prefix + "." + kClassFileSuffix[c], kPresentClass + c, d.classFile[c]);
  for (int a = 0; a < AX_COUNT; a++) need(prefix + "." + kAuxSuffix[a], kPresentAux + a, d.aux[a]);
  Bytes out;
  bool ok = decodeDump(d, meta0, [&](uint32_t ord, const std::string& ext) {
    Bytes b;
    std::string name = ownFileName(prefix, ord, ext);
    if (!readFile(name, b)) fail("cannot read %s", name.c_str());  // the decoder stops on it
    return b;
  }, out);
  if (!ok) die("%s", failMessage().c_str());
  if (out.size() != d.hdr.origSize || crc32_update(0, out.data(), out.size()) != d.hdr.origCrc)
    die("restored file does not match the original size/CRC recorded in the dump");
  writeFile(pos[1], out.data(), out.size());
  return 0;
}

}  // namespace pdd

int main(int argc, char** argv) {
  // no exceptions: an allocation that cannot be met (a size from a corrupt dump that the checks
  // let through) ends the program with a message instead of an abort
  std::set_new_handler([] {
    fputs("pdfdump: out of memory\n", stderr);
    _exit(1);
  });
  return pdd::realMain(argc, argv);
}
