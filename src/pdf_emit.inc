// pdf_emit.inc -- shared serializer: item records, stream descriptors, payload restore, decoder.
//
// The routines that produce PDF bytes are shared by c and d. c runs the whole decoder on the
// in-memory dump before committing any output file.

enum ItemTag : uint8_t { I_END = 0, I_TEXT = 1, I_RAW = 2, I_OBJ = 3, I_STREAM = 4, I_XREF = 5 };
enum Storage : uint8_t { ST_BIN, ST_TXT, ST_CLASS, ST_FILE, ST_DUP, ST_COUNT };
static const char* const kStorageName[ST_COUNT] = {"bin", "txt", "class", "file", "dup"};

struct StreamDesc {
  uint8_t cls = 0;
  uint8_t storage = ST_BIN;
  uint8_t classFile = 0;
  ShortName ext;            // ST_FILE: [a-z0-9]+
  uint32_t ordinal = 0;     // ST_FILE (assigned at emit) / ST_DUP (target)
  uint8_t cipher = CI_NONE;
  uint32_t num = 0, gen = 0;
  uint8_t iv[16] = {};
  Layers layers;              // undo order: outermost filter first
  bool deferred = false;      // restored after the last item (forward-dependent XRef stream)
  bool membersDerived = false;  // ObjStm: member text stored with derived-field placeholders
  uint64_t payloadLen = 0;    // deferred: payload size (space reserved when the item is written)
};

#include "pdf_derive.inc"

// Recon blobs go through a per-file dictionary of distinct blobs. Reference coding (channel N):
// 0 = new blob (length N, bytes R), 1 = same blob as the previous layer of this type, k+2 = blob k.
struct ReconDictW {
  std::unordered_map<std::string, uint32_t> map;
  uint32_t count = 0;
  int64_t last[L_COUNT];
  uint64_t newBytes = 0, refs = 0, hits = 0;
  ReconDictW() { for (auto& l : last) l = -1; }
  void put(ChanWriter& w, uint8_t type, const Bytes& blob) {
    std::string key((const char*)blob.data(), blob.size());
    key.push_back((char)type);
    refs++;
    auto it = map.find(key);
    if (it != map.end()) {
      hits++;
      if (last[type] == (int64_t)it->second) w.var(CH_N, 1);
      else w.var(CH_N, (uint64_t)it->second + 2);
      last[type] = it->second;
      return;
    }
    w.var(CH_N, 0);
    w.lenBytes(CH_N, CH_R, blob);
    newBytes += blob.size();
    map[key] = count;
    last[type] = count++;
  }
};
struct ReconDictR {
  std::vector<Bytes> blobs;
  int64_t last[L_COUNT];
  ReconDictR() { for (auto& l : last) l = -1; }
  bool get(ChanReader& r, uint8_t type, Bytes& out) {
    uint64_t v = r.var(CH_N);
    if (!r.ok()) return false;
    if (v == 0) {
      r.lenBytes(CH_N, CH_R, out);
      blobs.push_back(out);
      last[type] = (int64_t)blobs.size() - 1;
      return r.ok();
    }
    int64_t k = v == 1 ? last[type] : v - 2 > (uint64_t)INT64_MAX ? -1 : (int64_t)(v - 2);
    if (k < 0 || k >= (int64_t)blobs.size()) return fail("bad recon blob reference");
    out = blobs[(size_t)k];
    last[type] = k;
    return true;
  }
};

struct EmitState {
  ReconDictW recon;
  uint32_t prevCipherNum = 0;
};
struct ReadState {
  ReconDictR recon;
  uint32_t prevCipherNum = 0;
  uint32_t nextOrdinal = 0;
};

static void writeStreamDesc(ChanWriter& w, EmitState& st, StreamDesc& sd, Bytes& stored) {
  w.u8(CH_S, sd.cls);
  w.u8(CH_S, sd.storage);
  switch (sd.storage) {
    case ST_FILE:
      w.u8(CH_S, (uint8_t)sd.ext.size());
      w.put(CH_S, sd.ext.data(), sd.ext.size());
      break;
    case ST_DUP:
      w.var(CH_N, sd.ordinal);
      break;
    case ST_CLASS:
      w.u8(CH_S, sd.classFile);
      w.var(CH_N, stored.size());
      break;
    default:
      w.var(CH_N, stored.size());
  }
  w.u8(CH_S, sd.cipher);
  if (sd.cipher == CI_RC4 || sd.cipher == CI_AES128) {
    w.svar(CH_N, (int64_t)sd.num - (int64_t)st.prevCipherNum);
    w.var(CH_N, sd.gen);
    st.prevCipherNum = sd.num;
  }
  if (sd.cipher == CI_AES128 || sd.cipher == CI_AES256) w.put(CH_R, sd.iv, 16);
  w.u8(CH_S, (sd.deferred ? 1 : 0) | (sd.membersDerived ? 2 : 0));
  if (sd.deferred) w.var(CH_N, sd.payloadLen);
  w.u8(CH_S, (uint8_t)sd.layers.size());
  for (auto& l : sd.layers) {
    w.u8(CH_S, l.type);
    st.recon.put(w, l.type, l.recon);
  }
  // stored data
  switch (sd.storage) {
    case ST_BIN: w.put(CH_B, stored); break;
    case ST_TXT: w.put(CH_T, stored); break;
    case ST_CLASS: w.classPut(sd.classFile, stored.data(), stored.size()); break;
    case ST_FILE: sd.ordinal = w.ownAdd(sd.ext, lib::move(stored)); break;
    case ST_DUP: break;
  }
}

static bool readStreamDesc(ChanReader& r, ReadState& st, StreamDesc& sd, Bytes& stored,
                           std::vector<ShortName>& ordExt) {
  sd.cls = r.u8(CH_S);
  sd.storage = r.u8(CH_S);
  size_t len = 0;
  switch (sd.storage) {
    case ST_FILE: {
      size_t n = r.u8(CH_S);
      const uint8_t* p = r.get(CH_S, n);
      if (!p) return false;
      if (!sd.ext.set((const char*)p, n)) return fail("bad extension in stream descriptor");
      for (size_t k = 0; k < n; k++)
        if (!isalnum(p[k])) return fail("bad extension in stream descriptor");
      break;
    }
    case ST_DUP: sd.ordinal = (uint32_t)r.var(CH_N); break;
    case ST_CLASS:
      sd.classFile = r.u8(CH_S);
      if (sd.classFile >= CF_COUNT) return fail("bad class file id");
      len = r.len(CH_N);
      break;
    case ST_BIN:
    case ST_TXT: len = r.len(CH_N); break;
    default: return fail("bad stream storage %d", sd.storage);
  }
  sd.cipher = r.u8(CH_S);
  if (sd.cipher > CI_AES256) return fail("bad cipher id");
  if (sd.cipher == CI_RC4 || sd.cipher == CI_AES128) {
    sd.num = (uint32_t)((int64_t)st.prevCipherNum + r.svar(CH_N));
    sd.gen = (uint32_t)r.var(CH_N);
    st.prevCipherNum = sd.num;
  }
  if (sd.cipher == CI_AES128 || sd.cipher == CI_AES256) {
    const uint8_t* iv = r.get(CH_R, 16);
    if (!iv) return false;
    memcpy(sd.iv, iv, 16);
  }
  uint8_t flags = r.u8(CH_S);
  if (flags > 3) return fail("bad stream flags");
  sd.deferred = flags & 1;
  sd.membersDerived = (flags & 2) != 0;
  if (sd.deferred) sd.payloadLen = r.len(CH_N);
  size_t nl = r.u8(CH_S);
  if (!sd.layers.resize(nl)) return fail("too many layers in stream descriptor");
  for (auto& l : sd.layers) {
    l.type = r.u8(CH_S);
    if (!r.ok()) return false;
    if (l.type == L_NONE || l.type >= L_COUNT) return fail("bad layer type");
    if (!st.recon.get(r, l.type, l.recon)) return false;
  }
  switch (sd.storage) {
    case ST_BIN: {
      const uint8_t* p = r.get(CH_B, len);
      if (!p) return false;
      stored.assign(p, p + len);
      break;
    }
    case ST_TXT: {
      const uint8_t* p = r.get(CH_T, len);
      if (!p) return false;
      stored.assign(p, p + len);
      break;
    }
    case ST_CLASS: r.classGet(sd.classFile, len, stored); break;
    case ST_FILE:
      sd.ordinal = st.nextOrdinal++;
      ordExt.push_back(sd.ext);
      stored = r.own(sd.ordinal, sd.ext.c_str());
      break;
    case ST_DUP:
      if (sd.ordinal >= ordExt.size()) return fail("bad duplicate reference");
      stored = r.own(sd.ordinal, ordExt[sd.ordinal].c_str());
      break;
  }
  return r.ok() && !failed();
}

// stored bytes -> PDF payload bytes: filter layers innermost first, then the cipher
static bool restorePayload(const CodecCtx& ctx, const KeyBuf& fileKey, const StreamDesc& sd, Bytes& data) {
  for (size_t k = sd.layers.size(); k-- > 0;) {
    Bytes out;
    if (!layerEncode(ctx, sd.layers[k].type, sd.layers[k].recon, data, out) || failed())
      return fail("%s layer failed to re-encode", kLayerName[sd.layers[k].type]);
    data.swap(out);
  }
  if (sd.cipher != CI_NONE) {
    if (fileKey.empty()) return fail("encrypted stream but no file key in dump");
    Bytes out;
    if (!cipherEncrypt(sd.cipher, fileKey, sd.num, sd.gen, sd.iv, data, out))
      return fail("bad file key length for %s", kCipherName[sd.cipher]);
    data.swap(out);
  }
  return true;
}

// Optional shortcut for the encoder's own verify: given the k-th stream's descriptor and stored
// bytes as read back from the dump, supply the payload if they are identical to what the
// encoder already verified layer by layer (so re-running the layer encoders cannot differ).
typedef lib::function<bool(size_t, const StreamDesc&, const Bytes&, Bytes&)> FastRestore;

// The whole decoder: false, with the error recorded (failMessage()), on bad dump data.
static bool decodeDumpImpl(const DumpFiles& d, size_t meta0, ChanReader::OwnLoader loader, const FastRestore* fast,
                           const std::vector<Bytes>* blobFast, Bytes& out);
static bool decodeDump(const DumpFiles& d, size_t meta0, ChanReader::OwnLoader loader, Bytes& out,
                       const FastRestore* fast = nullptr, const std::vector<Bytes>* blobFast = nullptr) {
  failClear();
  out.clear();
  bool ok = decodeDumpImpl(d, meta0, lib::move(loader), fast, blobFast, out) && !failed();
  if (!ok && !failed()) fail("bad dump data");
  return ok;
}

static bool decodeDumpImpl(const DumpFiles& d, size_t meta0, ChanReader::OwnLoader loader, const FastRestore* fast,
                           const std::vector<Bytes>* blobFast, Bytes& out) {
  CodecCtx ctx;
  ctx.zlibHdr[0] = d.hdr.zlibHdr[0];
  ctx.zlibHdr[1] = d.hdr.zlibHdr[1];
  // undo whole-file transforms first
  DumpFiles tmp;
  const DumpFiles* use = &d;
  if (d.hdr.flags & (kFlagLiftT | kFlagLiftCS | kFlagCmap)) {
    tmp.hdr = d.hdr;
    for (int f = 0; f < CH_COUNT; f++) tmp.chan[f] = d.chan[f];
    for (int c = 0; c < CF_COUNT; c++) tmp.classFile[c] = d.classFile[c];
    if (d.hdr.flags & kFlagLiftT) {
      int f = d.hdr.map.fileOf[CH_T];
      if (f == 0) return fail("bad lift flag");
      if (!liftInverse(LIFT_OBJ, d.chan[f], d.aux[AX_TXN], Bytes(), Bytes(), tmp.chan[f]) || failed())
        return fail("text number lift: bad data");
    }
    if (d.hdr.flags & kFlagLiftCS)
      if (!liftInverse(LIFT_CS, d.classFile[CF_CS], d.aux[AX_CSN], d.aux[AX_CSH], d.aux[AX_CSS], tmp.classFile[CF_CS],
                       &ctx, &d.aux[AX_CSI]) || failed())
        return fail("content number lift: bad data");
    if (d.hdr.flags & kFlagCmap)
      if (!cmapInverse(d.classFile[CF_CMAP], d.aux[AX_CMB], tmp.classFile[CF_CMAP]) || failed())
        return fail("cmap transform: bad data");
    use = &tmp;
  }
  ChanReader::OwnLoader blobLoader = loader;
  ChanReader r(*use, meta0, lib::move(loader), &d.ownData);
  DerivFonts fonts;
  fonts.load = [&](uint32_t ord, const char* ext) { return r.own(ord, ext); };
  StructMap sm;
  DerivExtra ex;
  ex.fonts = &fonts;
  ex.sm = &sm;
  const Bytes& tchan = use->chan[use->hdr.map.fileOf[CH_T]];
  ItemLenPredictor ilp(tchan.data(), tchan.size());
  const Bytes& iln = d.aux[AX_ILN];
  size_t ilnPos = 0;
  // an item's length; false on bad data
  auto itemLen = [&](int kind, size_t& len) -> bool {
    uint64_t v = 0;
    for (int sh = 0;; sh += 7) {
      if (ilnPos >= iln.size() || sh > 63) return fail("dump truncated: item lengths");
      uint8_t c = iln[ilnPos++];
      v |= (uint64_t)(c & 0x7F) << sh;
      if (!(c & 0x80)) break;
    }
    if (v > ((uint64_t)1 << 42)) return fail("implausible item length");
    int64_t n = (int64_t)ilp.predict(r.pos(CH_T), kind) + (v ? unzigzag(v) : 0);
    if (n < 0) return fail("bad item length");
    len = (size_t)n;
    return true;
  };
  ReadState st;
  std::vector<ShortName> ordExt;
  ObjPositions objPos;
  ctx.objPos = &objPos;
  std::unordered_map<uint64_t, Bytes> rgbaSrc;  // stored BGRA BMPs of L_RGBA streams, for L_ALPHA
  ctx.alphaSource = [&](uint64_t k) -> const Bytes* {
    auto f = rgbaSrc.find(k);
    return f == rgbaSrc.end() ? nullptr : &f->second;
  };
  const uint64_t bias = d.hdr.offsetBias;
  DeriveState ds;
  StreamFacts noFacts;
  Bytes txt;
  std::vector<uint64_t> itemStart;
  std::vector<FwdPatch> fwd, local;
  SigBlobs sig{&d.aux[AX_P7S], 0};
  B64Blobs b64;
  b64.data = &d.aux[AX_B64];
  b64.rec = &d.aux[AX_B64R];
  b64.fastZ = blobFast;
  b64.ctx = &ctx;
  b64.file = [&](uint32_t k, const char* ext) -> Bytes {
    if (k < d.blobData.size()) return d.blobData[k];
    if (!blobLoader) {
      fail("blob file %07X missing", k);
      return Bytes();
    }
    return blobLoader(kBlobFileBit | k, ext);
  };
  struct Deferred { uint64_t pos; StreamDesc sd; Bytes data; size_t streamNo; };
  std::vector<Deferred> deferred;
  auto rebase = [&](uint64_t base) {
    for (auto& f : local) {
      f.pos += base;
      fwd.push_back(f);
    }
    local.clear();
  };
  size_t streamNo = 0;
  // (origSize is the dump's own claim: reserve no more than the dump could plausibly hold)
  out.reserve((size_t)lib::min<uint64_t>(d.hdr.origSize, (uint64_t)1 << 28));
  for (;;) {
    uint8_t tag = r.u8(CH_S);
    if (!r.ok() || failed()) return false;
    if (tag == I_END) break;
    itemStart.push_back(out.size());
    switch (tag) {
      case I_TEXT:
      case I_OBJ: {
        size_t n = 0;
        if (tag == I_OBJ ? !itemLen(IL_OBJ, n) : (n = r.len(CH_N), !r.ok())) return false;
        const uint8_t* p = r.get(CH_T, n);
        if (!p) return false;
        uint32_t num = 0, gen;
        bool hdr = tag == I_OBJ && objHeaderOf(p, n, num, gen);
        if (hdr) objPos.add(num, gen, out.size() - bias);
        DerivCrypt dc{&d.hdr.fileKey, num, hdr ? gen : 0};
        if (!derivDecode(p, n, noFacts, ds, num, txt, &dc, &local, &sig, &b64, &ex) || failed())
          return fail("bad derived field in item text");
        if (hdr) {
          size_t b0 = objBodyStart(txt.data(), txt.size());
          if (b0 != SIZE_MAX) structAddKids(txt.data() + b0, txt.size() - b0, num, gen, sm);
        }
        rebase(out.size());
        out.insert(out.end(), txt.begin(), txt.end());
        break;
      }
      case I_XREF: {
        ds.xrefPos.push_back(out.size() - bias);
        std::vector<lib::pair<uint64_t, int64_t>> xf;
        if (!readXref(r, objPos, out, &ds.lastXrefEnd, &xf)) return false;
        for (auto& x : xf) fwd.push_back({x.first, 10, 2, (uint64_t)x.second + 1, 0});
        break;
      }
      case I_RAW: {
        size_t n = r.len(CH_N);
        const uint8_t* p = r.get(CH_B, n);
        if (!p) return false;
        out.insert(out.end(), p, p + n);
        break;
      }
      case I_STREAM: {
        StreamDesc sd;
        Bytes data;
        if (!readStreamDesc(r, st, sd, data, ordExt)) return false;
        if (!sd.layers.empty() && sd.layers.back().type == L_RGBA) rgbaSrc[streamNo] = data;
        if (objstmMembersOf(sd)) {
          ObjstmMembers mem;
          bool ok = mem.init(sd);
          if (failed()) return false;
          if (sd.membersDerived) {
            if (!ok) return fail("bad derived ObjStm members");
            Bytes real;
            ex.members = &mem;
            bool good = derivDecode(data.data(), data.size(), noFacts, ds, 0, real, nullptr, nullptr, nullptr, nullptr, &ex);
            ex.members = nullptr;
            if (!good || failed()) return fail("bad derived field in ObjStm members");
            data.swap(real);
          }
          if (ok) mem.advance(data.data(), data.size(), mem.nums.size(), sm);
        } else if (sd.membersDerived) {
          return fail("bad stream flags");
        }
        size_t n = 0;
        if (!itemLen(IL_PRE, n)) return false;
        const uint8_t* p = r.get(CH_T, n);
        if (!p) return false;
        uint64_t start = out.size();
        uint32_t num = 0, gen = 0;
        if (objHeaderOf(p, n, num, gen)) {
          objPos.add(num, gen, start - bias);
          if (inlineSourceOk(sd, data.size())) fonts.inl[num] = data;  // a later width source
        }
        int need = derivNeeds(p, n);
        StreamFacts f;
        streamFacts(ctx, sd, data, (need & 1) != 0, (need & 2) != 0, f);
        Bytes payload;
        bool defer = sd.deferred;
        if (defer) {
          if (sd.payloadLen > d.hdr.origSize) return fail("deferred stream larger than the file");
          payload.assign((size_t)sd.payloadLen, 0);
        } else if (!(fast && (*fast)(streamNo, sd, data, payload))) {
          payload.swap(data);
          if (!restorePayload(ctx, d.hdr.fileKey, sd, payload)) return false;
        }
        if (failed()) return false;
        f.payloadKnown = true;
        f.payloadLen = payload.size();
        DerivCrypt dc{&d.hdr.fileKey, num, gen};
        if (!derivDecode(p, n, f, ds, num, txt, &dc, &local, &sig, &b64) || failed())
          return fail("bad derived field in stream dictionary");
        rebase(out.size());
        out.insert(out.end(), txt.begin(), txt.end());
        if (defer) deferred.push_back({out.size(), sd, lib::move(data), streamNo});
        out.insert(out.end(), payload.begin(), payload.end());
        uint32_t lr;
        if (lengthRefOf(txt.data(), txt.size(), lr)) ds.lenRef[lr] = payload.size();
        if (sd.cls == SC_XREF) {
          ds.xrefPos.push_back(start - bias);
          ds.lastXRefStm = (int64_t)(start - bias);
        }
        streamNo++;
        if (!itemLen(IL_SUF, n)) return false;
        p = r.get(CH_T, n);
        if (!p) return false;
        if (!derivDecode(p, n, noFacts, ds, 0, txt, nullptr, &local) || failed())
          return fail("bad derived field in item text");
        rebase(out.size());
        out.insert(out.end(), txt.begin(), txt.end());
        break;
      }
      default: return fail("bad item tag %d", tag);
    }
    if (out.size() > d.hdr.origSize) return fail("restored file larger than recorded");
  }
  if (!r.allConsumed() || ilnPos != iln.size() || sig.pos != sig.data->size() || b64.dpos != b64.data->size() || b64.rpos != b64.rec->size() ||
      (!d.blobData.empty() && b64.nextFile != d.blobData.size()))
    return fail("dump has unconsumed data");
  // forward-dependent streams: every object position is known now
  for (auto& df : deferred) {
    Bytes payload;
    if (!(fast && (*fast)(df.streamNo, df.sd, df.data, payload))) {
      payload.swap(df.data);
      if (!restorePayload(ctx, d.hdr.fileKey, df.sd, payload)) return false;
    }
    if (payload.size() != df.sd.payloadLen || df.pos + payload.size() > out.size())
      return fail("deferred stream size mismatch");
    memcpy(out.data() + df.pos, payload.data(), payload.size());
  }
  if (!fwdApply(out, itemStart, fwd, bias)) return fail("bad forward slot");
  return !failed();
}
