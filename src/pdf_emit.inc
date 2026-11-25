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
  std::string ext;          // ST_FILE
  uint32_t ordinal = 0;     // ST_FILE (assigned at emit) / ST_DUP (target)
  uint8_t cipher = CI_NONE;
  uint32_t num = 0, gen = 0;
  uint8_t iv[16] = {};
  std::vector<Layer> layers;  // undo order: outermost filter first
  bool deferred = false;      // restored after the last item (forward-dependent XRef stream)
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
  void get(ChanReader& r, uint8_t type, Bytes& out) {
    uint64_t v = r.var(CH_N);
    if (v == 0) {
      r.lenBytes(CH_N, CH_R, out);
      blobs.push_back(out);
      last[type] = (int64_t)blobs.size() - 1;
      return;
    }
    int64_t k = v == 1 ? last[type] : (int64_t)(v - 2);
    if (k < 0 || k >= (int64_t)blobs.size()) fail("bad recon blob reference");
    out = blobs[(size_t)k];
    last[type] = k;
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
  w.u8(CH_S, sd.deferred ? 1 : 0);
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
    case ST_FILE: sd.ordinal = w.ownAdd(sd.ext, std::move(stored)); break;
    case ST_DUP: break;
  }
}

static void readStreamDesc(ChanReader& r, ReadState& st, StreamDesc& sd, Bytes& stored,
                           std::vector<std::string>& ordExt) {
  sd.cls = r.u8(CH_S);
  sd.storage = r.u8(CH_S);
  size_t len = 0;
  switch (sd.storage) {
    case ST_FILE: {
      size_t n = r.u8(CH_S);
      const uint8_t* p = r.get(CH_S, n);
      sd.ext.assign((const char*)p, n);
      for (char c : sd.ext)
        if (!isalnum((unsigned char)c)) fail("bad extension in stream descriptor");
      break;
    }
    case ST_DUP: sd.ordinal = (uint32_t)r.var(CH_N); break;
    case ST_CLASS:
      sd.classFile = r.u8(CH_S);
      if (sd.classFile >= CF_COUNT) fail("bad class file id");
      len = r.len(CH_N);
      break;
    case ST_BIN:
    case ST_TXT: len = r.len(CH_N); break;
    default: fail("bad stream storage %d", sd.storage);
  }
  sd.cipher = r.u8(CH_S);
  if (sd.cipher > CI_AES256) fail("bad cipher id");
  if (sd.cipher == CI_RC4 || sd.cipher == CI_AES128) {
    sd.num = (uint32_t)((int64_t)st.prevCipherNum + r.svar(CH_N));
    sd.gen = (uint32_t)r.var(CH_N);
    st.prevCipherNum = sd.num;
  }
  if (sd.cipher == CI_AES128 || sd.cipher == CI_AES256) memcpy(sd.iv, r.get(CH_R, 16), 16);
  uint8_t flags = r.u8(CH_S);
  if (flags > 1) fail("bad stream flags");
  sd.deferred = flags & 1;
  if (sd.deferred) sd.payloadLen = r.len(CH_N);
  size_t nl = r.u8(CH_S);
  sd.layers.resize(nl);
  for (auto& l : sd.layers) {
    l.type = r.u8(CH_S);
    if (l.type == L_NONE || l.type >= L_COUNT) fail("bad layer type");
    st.recon.get(r, l.type, l.recon);
  }
  switch (sd.storage) {
    case ST_BIN: { const uint8_t* p = r.get(CH_B, len); stored.assign(p, p + len); break; }
    case ST_TXT: { const uint8_t* p = r.get(CH_T, len); stored.assign(p, p + len); break; }
    case ST_CLASS: r.classGet(sd.classFile, len, stored); break;
    case ST_FILE:
      sd.ordinal = st.nextOrdinal++;
      ordExt.push_back(sd.ext);
      stored = r.own(sd.ordinal, sd.ext);
      break;
    case ST_DUP:
      if (sd.ordinal >= ordExt.size()) fail("bad duplicate reference");
      stored = r.own(sd.ordinal, ordExt[sd.ordinal]);
      break;
  }
}

// stored bytes -> PDF payload bytes: filter layers innermost first, then the cipher
static void restorePayload(const CodecCtx& ctx, const Bytes& fileKey, const StreamDesc& sd, Bytes& data) {
  for (size_t k = sd.layers.size(); k-- > 0;) {
    Bytes out;
    if (!layerEncode(ctx, sd.layers[k].type, sd.layers[k].recon, data, out))
      fail("%s layer failed to re-encode", kLayerName[sd.layers[k].type]);
    data.swap(out);
  }
  if (sd.cipher != CI_NONE) {
    if (fileKey.empty()) fail("encrypted stream but no file key in dump");
    Bytes out;
    cipherEncrypt(sd.cipher, fileKey, sd.num, sd.gen, sd.iv, data, out);
    data.swap(out);
  }
}

// Optional shortcut for the encoder's own verify: given the k-th stream's descriptor and stored
// bytes as read back from the dump, supply the payload if they are identical to what the
// encoder already verified layer by layer (so re-running the layer encoders cannot differ).
typedef std::function<bool(size_t, const StreamDesc&, const Bytes&, Bytes&)> FastRestore;

static Bytes decodeDump(const DumpFiles& d, size_t meta0, ChanReader::OwnLoader loader,
                        const FastRestore* fast = nullptr) {
  CodecCtx ctx;
  ctx.zlibHdr[0] = d.hdr.zlibHdr[0];
  ctx.zlibHdr[1] = d.hdr.zlibHdr[1];
  // undo whole-file transforms first
  DumpFiles tmp;
  const DumpFiles* use = &d;
  if (d.hdr.flags & (kFlagLiftT | kFlagLiftCS | kFlagCmap)) {
    tmp.hdr = d.hdr;
    tmp.chan = d.chan;
    for (int c = 0; c < CF_COUNT; c++) tmp.classFile[c] = d.classFile[c];
    if (d.hdr.flags & kFlagLiftT) {
      int f = d.hdr.map.fileOf[CH_T];
      if (f == 0) fail("bad lift flag");
      if (!liftInverse(LIFT_OBJ, d.chan[f], d.aux[AX_TXN], Bytes(), tmp.chan[f])) fail("text number lift: bad data");
    }
    if (d.hdr.flags & kFlagLiftCS)
      if (!liftInverse(LIFT_CS, d.classFile[CF_CS], d.aux[AX_CSN], d.aux[AX_CSH], tmp.classFile[CF_CS], &ctx,
                       &d.aux[AX_CSI]))
        fail("content number lift: bad data");
    if (d.hdr.flags & kFlagCmap)
      if (!cmapInverse(d.classFile[CF_CMAP], d.aux[AX_CMB], tmp.classFile[CF_CMAP])) fail("cmap transform: bad data");
    use = &tmp;
  }
  ChanReader r(*use, meta0, std::move(loader), &d.ownData);
  ReadState st;
  std::vector<std::string> ordExt;
  Bytes out;
  ObjPositions objPos;
  ctx.objPos = &objPos;
  DeriveState ds;
  StreamFacts noFacts;
  Bytes txt;
  std::vector<uint64_t> itemStart;
  std::vector<FwdPatch> fwd, local;
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
  out.reserve((size_t)std::min<uint64_t>(d.hdr.origSize, (uint64_t)1 << 32));
  for (;;) {
    uint8_t tag = r.u8(CH_S);
    if (tag == I_END) break;
    itemStart.push_back(out.size());
    switch (tag) {
      case I_TEXT:
      case I_OBJ: {
        size_t n = r.len(CH_N);
        const uint8_t* p = r.get(CH_T, n);
        uint32_t num = 0, gen;
        bool hdr = tag == I_OBJ && objHeaderOf(p, n, num, gen);
        if (hdr) objPos.add(num, gen, out.size());
        DerivCrypt dc{&d.hdr.fileKey, num, hdr ? gen : 0};
        if (!derivDecode(p, n, noFacts, ds, num, txt, &dc, &local)) fail("bad derived field in item text");
        rebase(out.size());
        out.insert(out.end(), txt.begin(), txt.end());
        break;
      }
      case I_XREF: {
        ds.xrefPos.push_back(out.size());
        std::vector<std::pair<uint64_t, int64_t>> xf;
        readXref(r, objPos, out, &ds.lastXrefEnd, &xf);
        for (auto& x : xf) fwd.push_back({x.first, 10, 2, (uint64_t)x.second + 1, 0});
        break;
      }
      case I_RAW: {
        size_t n = r.len(CH_N);
        const uint8_t* p = r.get(CH_B, n);
        out.insert(out.end(), p, p + n);
        break;
      }
      case I_STREAM: {
        StreamDesc sd;
        Bytes data;
        readStreamDesc(r, st, sd, data, ordExt);
        size_t n = r.len(CH_N);
        const uint8_t* p = r.get(CH_T, n);
        uint64_t start = out.size();
        uint32_t num = 0, gen = 0;
        if (objHeaderOf(p, n, num, gen)) objPos.add(num, gen, start);
        int need = derivNeeds(p, n);
        StreamFacts f;
        streamFacts(ctx, sd, data, (need & 1) != 0, (need & 2) != 0, f);
        Bytes payload;
        bool defer = sd.deferred;
        if (defer) {
          payload.assign((size_t)sd.payloadLen, 0);
        } else if (!(fast && (*fast)(streamNo, sd, data, payload))) {
          payload.swap(data);
          restorePayload(ctx, d.hdr.fileKey, sd, payload);
        }
        f.payloadKnown = true;
        f.payloadLen = payload.size();
        DerivCrypt dc{&d.hdr.fileKey, num, gen};
        if (!derivDecode(p, n, f, ds, num, txt, &dc, &local)) fail("bad derived field in stream dictionary");
        rebase(out.size());
        out.insert(out.end(), txt.begin(), txt.end());
        if (defer) deferred.push_back({out.size(), sd, std::move(data), streamNo});
        out.insert(out.end(), payload.begin(), payload.end());
        uint32_t lr;
        if (lengthRefOf(txt.data(), txt.size(), lr)) ds.lenRef[lr] = payload.size();
        if (sd.cls == SC_XREF) {
          ds.xrefPos.push_back(start);
          ds.lastXRefStm = (int64_t)start;
        }
        streamNo++;
        n = r.len(CH_N);
        p = r.get(CH_T, n);
        if (!derivDecode(p, n, noFacts, ds, 0, txt, nullptr, &local)) fail("bad derived field in item text");
        rebase(out.size());
        out.insert(out.end(), txt.begin(), txt.end());
        break;
      }
      default: fail("bad item tag %d", tag);
    }
  }
  if (!r.allConsumed()) fail("dump has unconsumed data");
  // forward-dependent streams: every object position is known now
  for (auto& df : deferred) {
    Bytes payload;
    if (!(fast && (*fast)(df.streamNo, df.sd, df.data, payload))) {
      payload.swap(df.data);
      restorePayload(ctx, d.hdr.fileKey, df.sd, payload);
    }
    if (payload.size() != df.sd.payloadLen || df.pos + payload.size() > out.size()) fail("deferred stream size mismatch");
    memcpy(out.data() + df.pos, payload.data(), payload.size());
  }
  if (!fwdApply(out, itemStart, fwd)) fail("bad forward slot");
  return out;
}
