// pdf_xmp.inc -- the JPEG thumbnail of an XMP packet as the stream's stored form (layer L_XMPJPG).
//
// <xmpGImg:image> (or xapGImg) holds a base64 JPEG in lines of L characters joined by a separator
// (&#xA; in every file seen). The stream's stored form becomes the JPEG itself (.jpg, for a JPEG
// recompressor); the recon keeps the XMP text around it and the wrapping. A packet with several
// thumbnails gives up its largest one; the rest stay text.
// Recon: var line length, var separator length + bytes, u8 flags (bit0 '=' padding, bit1
// separator after the last line), var prefix length + bytes, var suffix length + bytes.

static const char* const kXmpSeps[] = {"&#xA;", "&#10;", "&#xD;&#xA;", "&#13;&#10;", "\r\n", "\n"};

static void xmpWrap(const Bytes& jpeg, size_t L, const Bytes& sep, uint8_t flags, Bytes& out) {
  Bytes b64;
  b64Encode(jpeg.data(), jpeg.size(), (flags & 1) != 0, b64);
  for (size_t i = 0; i < b64.size(); i += L) {
    if (i) out.insert(out.end(), sep.begin(), sep.end());
    out.insert(out.end(), b64.begin() + i, b64.begin() + lib::min(b64.size(), i + L));
  }
  if (flags & 2) out.insert(out.end(), sep.begin(), sep.end());
}

// XMP text -> JPEG + recon; false if there is no thumbnail in exactly this form
static bool xmpThumbSplit(const Bytes& in, Bytes& jpeg, Bytes& recon) {
  static const char tag[] = "GImg:image>";
  const size_t tl = sizeof(tag) - 1;
  size_t bestA = 0, bestB = 0;
  for (size_t p = 0; (p = lib::search(in.begin() + p, in.end(), tag, tag + tl) - in.begin()) < in.size(); p += tl) {
    size_t s = p;  // start of the element name
    while (s > 0 && (isalnum(in[s - 1]) || in[s - 1] == '_')) s--;
    if (s == 0 || in[s - 1] != '<') continue;  // a closing tag or something else
    lib::string close = "</" + lib::string(in.begin() + s, in.begin() + p + tl);
    size_t a = p + tl;
    auto e = lib::search(in.begin() + a, in.end(), close.begin(), close.end());
    if (e == in.end()) continue;
    size_t b = e - in.begin();
    if (b - a > bestB - bestA) { bestA = a; bestB = b; }
  }
  if (bestB - bestA < 64) return false;
  const uint8_t* c = in.data() + bestA;
  size_t n = bestB - bestA;
  // first line and separator
  size_t L = 0;
  while (L < n && (b64Val(c[L]) >= 0 || c[L] == '=')) L++;
  Bytes sep;
  if (L < n) {
    for (const char* s : kXmpSeps) {
      size_t sl = strlen(s);
      if (n - L >= sl && memcmp(c + L, s, sl) == 0) { sep.assign(s, s + sl); break; }
    }
    if (sep.empty()) return false;
  }
  if (L == 0) return false;
  // all lines: L characters except the last; collect the base64 text
  Bytes text;
  size_t i = 0;
  uint8_t flags = 0;
  while (true) {
    size_t j = i;
    while (j < n && (b64Val(c[j]) >= 0 || c[j] == '=')) j++;
    size_t len = j - i;
    text.insert(text.end(), c + i, c + j);
    if (j == n) {
      if (len > L) return false;
      break;
    }
    if (len != L || sep.empty() || n - j < sep.size() || memcmp(c + j, sep.data(), sep.size()) != 0) return false;
    i = j + sep.size();
    if (i == n) { flags |= 2; break; }
  }
  size_t eq = 0;
  while (eq < text.size() && eq < 2 && text[text.size() - 1 - eq] == '=') eq++;
  for (size_t k = 0; k + eq < text.size(); k++)
    if (text[k] == '=') return false;
  if (eq) flags |= 1;
  if (!b64Decode(text.data(), text.size() - eq, jpeg)) return false;
  if (jpeg.size() < 4 || jpeg[0] != 0xFF || jpeg[1] != 0xD8) return false;
  if (!eq && (text.size() % 4) == 0) flags |= 1;  // no padding needed: either choice re-encodes alike
  recon.clear();
  putVarTo(recon, L);
  putLenBytesTo(recon, sep.data(), sep.size());
  recon.push_back(flags);
  putLenBytesTo(recon, in.data(), bestA);
  putLenBytesTo(recon, in.data() + bestB, in.size() - bestB);
  return true;
}

// JPEG + recon -> XMP text (layerEncode for L_XMPJPG)
static bool xmpEncodeLayer(const Bytes& recon, const Bytes& in, Bytes& out) {
  RBuf r(recon);
  size_t L = (size_t)r.var();
  if (L == 0 || L > ((size_t)1 << 20)) return false;
  Bytes sep, prefix;
  r.bytes(r.len(), sep);
  uint8_t flags = r.u8();
  r.bytes(r.len(), prefix);
  out.swap(prefix);
  xmpWrap(in, L, sep, flags, out);
  r.appendTo(r.len(), out);
  return r.ok() && r.p == recon.size();
}
