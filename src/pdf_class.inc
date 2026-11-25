// pdf_class.inc -- read-only ObjStm expansion, stream classification, signature table,
// raw-image description for BMP3.
//
// Classification is an encoder-only decision recorded in the stream descriptor, so nothing here
// needs exact re-serialization.

enum StreamClass : uint8_t {
  SC_UNKNOWN, SC_OBJSTM, SC_XREF, SC_CONTENT, SC_IMAGE, SC_FONT_T1, SC_FONT_TT, SC_FONT_CFF, SC_FONT_OT,
  SC_ICC, SC_CMAP, SC_XML, SC_JS, SC_ATTACH, SC_FUNC, SC_SHADING, SC_CIDSET, SC_CIDTOGID, SC_JB2G,
  SC_PALETTE, SC_HINT, SC_AIPRIV, SC_COUNT
};
static const char* const kClassName[SC_COUNT] = {
    "unknown", "objstm",  "xref", "content", "image",   "type1", "truetype",     "cff",     "opentype", "icc", "cmap",
    "xml",     "js",      "attachment", "function", "shading", "cidset", "cidtogidmap", "jbig2globals", "palette", "hint",
    "illustrator"};
static const char* const kClassExt[SC_COUNT] = {"bin", "bin", "bin", "cs",  "img", "pfa", "ttf", "cff", "otf", "icc", "cmap",
                                                "xml", "js",  "bin", "bin", "bin", "bin", "bin", "jb2g", "bin", "bin",
                                                "ai"};

// ---- signatures ------------------------------------------------------------------------------
static const char* sniffExt(const uint8_t* p, size_t n) {
  auto has = [&](size_t off, const char* sig, size_t l) { return n >= off + l && memcmp(p + off, sig, l) == 0; };
  if (has(0, "\xFF\xD8\xFF", 3)) return "jpg";
  if (has(0, "\x89PNG\r\n\x1A\n", 8)) return "png";
  if (has(0, "GIF87a", 6) || has(0, "GIF89a", 6)) return "gif";
  if (has(0, "II*\0", 4) || has(0, "MM\0*", 4)) return "tif";
  if (has(0, "\0\0\0\x0CjP  \r\n\x87\n", 12)) return "jp2";
  if (has(0, "\xFF\x4F\xFF\x51", 4)) return "j2k";
  if (has(0, "%PDF-", 5)) return "pdf";
  if (has(0, "PK\x03\x04", 4)) return "zip";
  if (has(0, "\0\x01\0\0", 4) || has(0, "true", 4) || has(0, "ttcf", 4)) return "ttf";
  if (has(0, "OTTO", 4)) return "otf";
  if (has(0, "%!PS-AdobeFont", 14) || has(0, "%!FontType1", 11)) return "pfa";
  if (has(0, "%!PS", 4)) return "ps";
  if (has(36, "acsp", 4)) return "icc";
  // text signatures, after an optional UTF-8 BOM and whitespace
  size_t i = 0;
  if (has(0, "\xEF\xBB\xBF", 3)) i = 3;
  while (i < n && i < 4096 && isWS(p[i])) i++;
  if (has(i, "<?xml", 5) || has(i, "<?xpacket", 9) || has(i, "<x:xmpmeta", 10)) return "xml";
  return nullptr;
}

// extension from an attachment file name: [a-z0-9]{1,8}
static std::string fileNameExt(const std::string& name0) {
  std::string name = name0;
  if (name.size() >= 2 && (uint8_t)name[0] == 0xFE && (uint8_t)name[1] == 0xFF) {
    std::string a;
    for (size_t i = 2; i + 1 < name.size(); i += 2) a += name[i] ? '?' : name[i + 1];
    name = a;
  }
  size_t dot = name.rfind('.');
  if (dot == std::string::npos) return "";
  std::string e;
  for (size_t i = dot + 1; i < name.size(); i++) {
    char c = (char)tolower((unsigned char)name[i]);
    if (!isalnum((unsigned char)c) || (unsigned char)name[i] >= 0x80) return "";
    e += c;
  }
  if (e.empty() || e.size() > 8) return "";
  return e;
}

// ---- object resolution -------------------------------------------------------------------------
struct ObjIndex {
  std::unordered_map<uint32_t, const Obj*> byNum;  // latest definition in file order
  const Obj* resolve(const Obj* o, int depth = 0) const {
    while (o && o->t == Obj::REF && depth++ < 32) {
      auto it = byNum.find((uint32_t)o->i);
      o = it == byNum.end() ? nullptr : it->second;
    }
    return o;
  }
  const Obj* get(const Obj* dict, const char* key) const {
    dict = resolve(dict);
    return dict ? resolve(dict->get(key)) : nullptr;
  }
};

// ObjStm members: (objnum, value)
static bool expandObjStm(const Obj& dict, const Bytes& data, std::vector<std::pair<uint32_t, Obj>>& out) {
  const Obj* N = dict.get("N");
  const Obj* F = dict.get("First");
  if (!N || !F || N->t != Obj::INT || F->t != Obj::INT || N->i < 0 || F->i < 0 || (size_t)F->i > data.size())
    return false;
  Lexer lx(data.data(), data.size(), 0);
  std::vector<std::pair<uint64_t, uint64_t>> hdr;
  for (int64_t k = 0; k < N->i; k++) {
    uint64_t num, off;
    if (!lx.parseUInt(num) || !lx.parseUInt(off)) break;
    hdr.push_back({num, off});
  }
  for (auto& h : hdr) {
    size_t at = (size_t)F->i + (size_t)h.second;
    if (at >= data.size() || h.first > 0xFFFFFFFFull) continue;
    Lexer ml(data.data(), data.size(), at);
    Obj o;
    if (ml.parse(o)) out.push_back({(uint32_t)h.first, std::move(o)});
  }
  return !out.empty();
}

// ---- classification ----------------------------------------------------------------------------
// Referrer rules assign classes to object numbers; own-dictionary rules come from the stream dict.

struct Classifier {
  const ObjIndex& idx;
  std::unordered_map<uint32_t, uint8_t> refClass;
  std::unordered_map<uint32_t, std::string> attachName;
  explicit Classifier(const ObjIndex& i) : idx(i) {}

  void assign(const Obj* v, uint8_t cls) {
    if (!v) return;
    if (v->t == Obj::REF) {
      auto it = refClass.find((uint32_t)v->i);
      if (it == refClass.end() || it->second == SC_UNKNOWN) refClass[(uint32_t)v->i] = cls;
    } else if (v->t == Obj::ARR) {
      for (auto& e : v->a)
        if (e.t == Obj::REF) assign(&e, cls);
    }
  }
  void assignValues(const Obj* d, uint8_t cls) {
    d = idx.resolve(d);
    if (!d || d->t != Obj::DICT) return;
    for (size_t k = 1; k < d->a.size(); k += 2) assign(&d->a[k], cls);
  }

  static bool isPage(const Obj& d) {
    const Obj* t = d.get("Type");
    if (t) return t->isName("Page");
    return d.get("Parent") && (d.get("MediaBox") || d.get("Resources"));
  }

  void walk(const Obj& o, int depth = 0) {
    if (depth > 64) return;
    if (o.t == Obj::ARR) {
      if (o.a.size() >= 2 && o.a[0].isName("ICCBased")) assign(&o.a[1], SC_ICC);
      if (o.a.size() >= 4 && (o.a[0].isName("Indexed") || o.a[0].isName("I"))) assign(&o.a[3], SC_PALETTE);
      for (auto& e : o.a) walk(e, depth + 1);
      return;
    }
    if (o.t != Obj::DICT) return;
    for (size_t k = 0; k + 1 < o.a.size(); k += 2) {
      const std::string& key = o.a[k].s;
      const Obj* v = &o.a[k + 1];
      if (key == "Contents" && isPage(o)) assign(v, SC_CONTENT);
      else if (key == "FontFile") assign(v, SC_FONT_T1);
      else if (key == "FontFile2") assign(v, SC_FONT_TT);
      else if (key == "FontFile3") assign(v, SC_FONT_CFF);
      else if (key == "ToUnicode") assign(v, SC_CMAP);
      else if (key == "Encoding" && v->t == Obj::REF) assign(v, SC_CMAP);  // CMap stream of a Type0 font
      else if (key == "UseCMap" && v->t == Obj::REF) assign(v, SC_CMAP);
      else if (key == "Metadata") assign(v, SC_XML);
      else if (key == "XFA") assign(v, SC_XML);
      else if (key == "JS") assign(v, SC_JS);
      else if (key == "CharProcs") assignValues(v, SC_CONTENT);
      else if (key == "AP") {
        const Obj* ap = idx.resolve(v);
        if (ap && ap->t == Obj::DICT)
          for (size_t j = 1; j < ap->a.size(); j += 2) {
            const Obj* s = &ap->a[j];
            if (s->t == Obj::REF) {
              const Obj* r = idx.resolve(s);
              if (r && r->t == Obj::DICT && !r->get("Length")) assignValues(r, SC_CONTENT);  // state dict
              else assign(s, SC_CONTENT);
            } else if (s->t == Obj::DICT) {
              assignValues(s, SC_CONTENT);
            }
          }
      } else if (key == "EF") {
        const Obj* ef = idx.resolve(v);
        std::string name;
        const Obj* uf = idx.resolve(o.get("UF"));
        const Obj* f = idx.resolve(o.get("F"));
        if (uf && uf->t == Obj::STR) name = uf->s;
        else if (f && f->t == Obj::STR) name = f->s;
        if (ef && ef->t == Obj::DICT)
          for (size_t j = 1; j < ef->a.size(); j += 2)
            if (ef->a[j].t == Obj::REF) {
              assign(&ef->a[j], SC_ATTACH);
              if (!name.empty()) attachName[(uint32_t)ef->a[j].i] = name;
            }
      } else if (key == "CIDSet") assign(v, SC_CIDSET);
      else if (key == "CIDToGIDMap") assign(v, SC_CIDTOGID);
      else if (key == "JBIG2Globals") assign(v, SC_JB2G);
      else if (key == "Thumb" || key == "SMask" || key == "Mask") assign(v, SC_IMAGE);
      else if (key.compare(0, 13, "AIPrivateData") == 0 || key.compare(0, 16, "AIPDFPrivateData") == 0 ||
               key == "AIMetaData") assign(v, SC_AIPRIV);
      else if (key == "Function" || key == "Functions" || key == "TR" || key == "TR2" || key == "BG" ||
               key == "BG2" || key == "UCR" || key == "UCR2" || key == "TransferFunction")
        assign(v, SC_FUNC);
      walk(*v, depth + 1);
    }
  }

  // class from the stream's own dictionary (SC_UNKNOWN if not decisive)
  uint8_t ownClass(const Obj& d) const {
    const Obj* t = idx.resolve(d.get("Type"));
    const Obj* st = idx.resolve(d.get("Subtype"));
    if (t && t->isName("ObjStm")) return SC_OBJSTM;
    if (t && t->isName("XRef")) return SC_XREF;
    if (st && st->isName("Image")) return SC_IMAGE;
    if (st && st->isName("Form")) return SC_CONTENT;
    if (d.get("PatternType")) {
      const Obj* pt = idx.resolve(d.get("PatternType"));
      return pt && pt->t == Obj::INT && pt->i == 1 ? SC_CONTENT : SC_SHADING;
    }
    if (d.get("ShadingType")) return SC_SHADING;
    if (d.get("FunctionType")) return SC_FUNC;
    if (t && t->isName("Metadata")) return SC_XML;
    if (st && st->isName("XML")) return SC_XML;
    if (t && t->isName("EmbeddedFile")) return SC_ATTACH;
    if (t && t->isName("CMap")) return SC_CMAP;
    if (st && (st->isName("Type1C") || st->isName("CIDFontType0C"))) return SC_FONT_CFF;
    if (st && st->isName("OpenType")) return SC_FONT_OT;
    return SC_UNKNOWN;
  }

  uint8_t classify(const Obj& d, uint32_t num) const {
    uint8_t own = ownClass(d);
    auto it = refClass.find(num);
    uint8_t ref = it == refClass.end() ? (uint8_t)SC_UNKNOWN : it->second;
    if (own == SC_FONT_OT) return own;
    if (own == SC_FONT_CFF && ref == SC_FONT_CFF) return own;
    if (own != SC_UNKNOWN) return own;
    if (ref == SC_UNKNOWN) {
      if (d.get("Length1") && d.get("Length2")) return SC_FONT_T1;
      if (d.get("Length1")) return SC_FONT_TT;
    }
    return ref;
  }
};

// ---- raw image description for BMP3 ------------------------------------------------------------
static bool imageBmpSpec(const ObjIndex& idx, const Obj& d, const std::function<const Bytes*(const Obj*)>& streamData,
                         BmpSpec& sp) {
  auto num = [&](const char* k, const char* ab) -> int64_t {
    const Obj* o = idx.resolve(d.get2(k, ab));
    return o && o->t == Obj::INT ? o->i : -1;
  };
  int64_t W = num("Width", "W"), H = num("Height", "H"), bpc = num("BitsPerComponent", "BPC");
  if (W <= 0 || H <= 0) return false;
  const Obj* im = idx.resolve(d.get2("ImageMask", "IM"));
  bool mask = im && im->t == Obj::BOOL && im->i;
  const Obj* dec = idx.resolve(d.get2("Decode", "D"));
  bool inverted = dec && dec->t == Obj::ARR && dec->a.size() >= 2 && dec->a[0].isNum() && dec->a[0].num() > dec->a[1].num();
  sp.width = (uint32_t)W;
  sp.height = (uint32_t)H;
  sp.palette.clear();
  auto ramp = [&](int bits, bool inv) {
    int nent = 1 << bits;
    for (int i = 0; i < nent; i++) {
      int v = i * 255 / (nent - 1);
      if (inv) v = 255 - v;
      sp.palette.push_back((uint32_t)(v * 0x010101));
    }
  };
  if (mask) {
    sp.bpp = 1;
    ramp(1, inverted);
    return true;
  }
  if (bpc != 1 && bpc != 4 && bpc != 8) return false;
  const Obj* cs = idx.resolve(d.get2("ColorSpace", "CS"));
  if (!cs) return false;
  int comps = 0;
  const Obj* indexedBase = nullptr;
  const Obj* lookup = nullptr;
  int64_t hival = -1;
  std::function<int(const Obj*, int)> compsOf = [&](const Obj* c, int depth) -> int {
    c = idx.resolve(c);
    if (!c || depth > 8) return 0;
    if (c->t == Obj::NAME) {
      if (c->s == "DeviceGray" || c->s == "G" || c->s == "CalGray") return 1;
      if (c->s == "DeviceRGB" || c->s == "RGB" || c->s == "CalRGB") return 3;
      if (c->s == "DeviceCMYK" || c->s == "CMYK") return 4;
      return 0;
    }
    if (c->t != Obj::ARR || c->a.empty() || c->a[0].t != Obj::NAME) return 0;
    const std::string& f = c->a[0].s;
    if (f == "CalGray") return 1;
    if (f == "CalRGB" || f == "Lab") return 3;
    if (f == "Separation") return 1;
    if (f == "DeviceN" && c->a.size() >= 2) {
      const Obj* names = idx.resolve(&c->a[1]);
      return names && names->t == Obj::ARR ? (int)names->a.size() : 0;
    }
    if (f == "ICCBased" && c->a.size() >= 2) {
      const Obj* s = idx.resolve(&c->a[1]);
      const Obj* n = s ? idx.resolve(s->get("N")) : nullptr;
      return n && n->t == Obj::INT ? (int)n->i : 0;
    }
    if ((f == "Indexed" || f == "I") && c->a.size() >= 4 && depth == 0) {
      indexedBase = &c->a[1];
      const Obj* hv = idx.resolve(&c->a[2]);
      hival = hv && hv->t == Obj::INT ? hv->i : -1;
      lookup = &c->a[3];
      return -1;
    }
    return 0;
  };
  comps = compsOf(cs, 0);
  if (comps == -1) {
    sp.bpp = (int)bpc;
    int bc = compsOf(indexedBase, 1);
    const Obj* lk = idx.resolve(lookup);
    const uint8_t* lp = nullptr;
    size_t ln = 0;
    if (lk && lk->t == Obj::STR) { lp = (const uint8_t*)lk->s.data(); ln = lk->s.size(); }
    else if (lookup && lookup->t == Obj::REF) {
      const Bytes* b = streamData(lookup);
      if (b) { lp = b->data(); ln = b->size(); }
    }
    int nent = 1 << sp.bpp;
    if ((bc == 3 || bc == 1) && lp && hival >= 0) {
      for (int i = 0; i < nent; i++) {
        uint32_t c = 0;
        if (i <= hival && (size_t)(i + 1) * bc <= ln) {
          if (bc == 3) c = ((uint32_t)lp[3 * i] << 16) | (lp[3 * i + 1] << 8) | lp[3 * i + 2];
          else c = lp[i] * 0x010101u;
        }
        sp.palette.push_back(c);
      }
    } else {
      ramp(sp.bpp, false);
    }
    return true;
  }
  if (comps == 3 && bpc == 8) { sp.bpp = 24; return true; }
  if (comps == 4 && bpc == 8) { sp.bpp = 32; return true; }
  if (comps == 1) {
    sp.bpp = (int)bpc;
    ramp(sp.bpp, inverted);
    return true;
  }
  return false;
}
