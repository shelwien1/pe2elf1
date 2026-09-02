// tfwc - unpack a FX2TFWC2 weights file into uncompressed BMP images plus a
// text file with what the images cannot hold, and pack them back losslessly.
//
//   tfwc c input.tfwc2 output_prefix     ->  output_prefix.txt, output_prefix_*.bmp
//   tfwc d input_prefix output.tfwc2     <-  input_prefix.txt,  input_prefix_*.bmp
//
// One tensor is one image, except where MODEL.md's parameter budget groups
// them: the four 192x192 projections of an attention block (q, k, v, o, KDA or
// sliding-window alike) become one 32-bit RGBA image (R=q, G=k, B=v, A=o), and
// their four per-row scales a 192x2 RGBA image; a KDA block's two rank-64 gate
// matrices are stacked into one 8-bit image per direction.  Everything with 32
// elements or fewer - the config, the coefficients, the decay rates, the
// activation scales, rope.inv_freq - goes into the text file as hex, and
// rope.sin/rope.cos go nowhere: the file format itself recomputes them.
//
// Pixel values.  int4 weights are stored as q+7 (0..14), the raw symbol, so the
// image data is the pure weight stream for whatever is done with it next;
// 8-bit images carry a palette that spreads those 15 levels over the grey
// scale for viewing (32-bit images have no palette and look dark - stretch the
// levels by 18 to see them).  bf16 scales are two rows, high byte then low.
// fp32 tensors are one float per 32-bit pixel, bytes verbatim.
//
// BMP version 3: the 40-byte BITMAPINFOHEADER, BI_RGB, rows bottom-up and
// padded to four bytes, 8 or 32 bits per pixel.  Nothing else.
//
// The text file also records the order the tensors had in the input, because
// the range coder's adaptive models make the output bytes depend on it: with
// the order kept, `d` reproduces the input file byte for byte.

#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <initializer_list>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "tf/weights_io.h"
#include "tf/weights_io.inc"
#include "tf/weights_io_compressed.inc"
#include "tf/weights_write.inc"

using fx2::WTensor;
using fx2::WeightsFile;

namespace {

[[noreturn]] void fail(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "tfwc: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

// ---------------------------------------------------------------------------
// BMP v3
// ---------------------------------------------------------------------------

struct Bmp {
  int w = 0, h = 0, bpp = 0;      // bpp 8 or 32
  std::vector<uint8_t> px;        // top-down, w*bpp/8 bytes per row, unpadded
  bool int4_palette = false;      // 8bpp: spread 0..14 over grey, else identity

  size_t stride() const { return size_t(w) * bpp / 8; }
  uint8_t* row(int y) { return px.data() + size_t(y) * stride(); }
  const uint8_t* row(int y) const { return px.data() + size_t(y) * stride(); }
  void alloc(int W, int H, int B) {
    w = W;
    h = H;
    bpp = B;
    px.assign(stride() * H, 0);
  }
};

void put_u16(std::vector<uint8_t>& o, uint32_t v) {
  o.push_back(uint8_t(v));
  o.push_back(uint8_t(v >> 8));
}
void put_u32(std::vector<uint8_t>& o, uint32_t v) {
  for (int i = 0; i < 4; i++) o.push_back(uint8_t(v >> (8 * i)));
}
uint32_t get_u16(const uint8_t* p) { return p[0] | (p[1] << 8); }
uint32_t get_u32(const uint8_t* p) {
  return p[0] | (p[1] << 8) | (p[2] << 16) | (uint32_t(p[3]) << 24);
}

void write_bmp(const std::string& path, const Bmp& b) {
  const size_t padded = (b.stride() + 3) & ~size_t(3);
  const uint32_t pal_bytes = b.bpp == 8 ? 256 * 4 : 0;
  const uint32_t offset = 14 + 40 + pal_bytes;
  const uint32_t image = uint32_t(padded * b.h);
  std::vector<uint8_t> o;
  o.reserve(offset + image);
  // BITMAPFILEHEADER
  o.push_back('B');
  o.push_back('M');
  put_u32(o, offset + image);
  put_u16(o, 0);
  put_u16(o, 0);
  put_u32(o, offset);
  // BITMAPINFOHEADER (v3)
  put_u32(o, 40);
  put_u32(o, uint32_t(b.w));
  put_u32(o, uint32_t(b.h));  // positive: bottom-up
  put_u16(o, 1);
  put_u16(o, uint32_t(b.bpp));
  put_u32(o, 0);  // BI_RGB
  put_u32(o, image);
  put_u32(o, 2835);
  put_u32(o, 2835);
  put_u32(o, b.bpp == 8 ? 256 : 0);
  put_u32(o, 0);
  if (b.bpp == 8) {
    for (int i = 0; i < 256; i++) {
      const uint8_t g = b.int4_palette ? uint8_t(i <= 14 ? i * 18 : 0) : uint8_t(i);
      o.push_back(g);
      o.push_back(g);
      o.push_back(g);
      o.push_back(0);
    }
  }
  for (int y = b.h - 1; y >= 0; y--) {
    o.insert(o.end(), b.row(y), b.row(y) + b.stride());
    for (size_t k = b.stride(); k < padded; k++) o.push_back(0);
  }
  FILE* f = fopen(path.c_str(), "wb");
  if (!f) fail("cannot write %s", path.c_str());
  if (fwrite(o.data(), 1, o.size(), f) != o.size()) fail("short write on %s", path.c_str());
  fclose(f);
}

Bmp read_bmp(const std::string& path) {
  FILE* f = fopen(path.c_str(), "rb");
  if (!f) fail("cannot open %s", path.c_str());
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  std::vector<uint8_t> d(size_t(n > 0 ? n : 0));
  if (n < 54 || fread(d.data(), 1, d.size(), f) != d.size()) fail("cannot read %s", path.c_str());
  fclose(f);
  if (d[0] != 'B' || d[1] != 'M') fail("%s: not a BMP", path.c_str());
  const uint32_t offset = get_u32(&d[10]);
  const uint32_t hsize = get_u32(&d[14]);
  if (hsize < 40) fail("%s: needs a v3 (40-byte) info header", path.c_str());
  const int32_t w = int32_t(get_u32(&d[18]));
  int32_t h = int32_t(get_u32(&d[22]));
  const uint32_t bpp = get_u16(&d[28]);
  const uint32_t comp = get_u32(&d[30]);
  if (comp != 0 || (bpp != 8 && bpp != 32)) fail("%s: only 8/32 bpp BI_RGB is understood", path.c_str());
  const bool topdown = h < 0;
  if (topdown) h = -h;
  Bmp b;
  b.alloc(w, h, int(bpp));
  const size_t padded = (b.stride() + 3) & ~size_t(3);
  if (offset + padded * size_t(h) > d.size()) fail("%s: truncated", path.c_str());
  for (int y = 0; y < h; y++) {
    const int src = topdown ? y : h - 1 - y;
    memcpy(b.row(y), &d[offset + padded * size_t(src)], b.stride());
  }
  return b;
}

// ---------------------------------------------------------------------------
// tensor <-> image
// ---------------------------------------------------------------------------

const char* dtype_name(uint8_t d) {
  switch (d) {
    case fx2::DT_I8: return "i8";
    case fx2::DT_BF16: return "bf16";
    case fx2::DT_F32: return "f32";
    case fx2::DT_I32: return "i32";
  }
  fail("unknown dtype %u", unsigned(d));
}
uint8_t dtype_code(const std::string& s) {
  if (s == "i8") return fx2::DT_I8;
  if (s == "bf16") return fx2::DT_BF16;
  if (s == "f32") return fx2::DT_F32;
  if (s == "i32") return fx2::DT_I32;
  fail("unknown dtype '%s'", s.c_str());
}
size_t esize(uint8_t d) { return d == fx2::DT_I8 ? 1 : (d == fx2::DT_BF16 ? 2 : 4); }

// rows x cols view of a tensor: [a] is 1 x a, [a,b] is a x b
void dims2(const WTensor& t, int* rows, int* cols) {
  if (t.shape.size() == 1) {
    *rows = 1;
    *cols = int(t.shape[0]);
  } else if (t.shape.size() == 2) {
    *rows = int(t.shape[0]);
    *cols = int(t.shape[1]);
  } else {
    fail("tensor of %zu dims cannot be an image", t.shape.size());
  }
}

// --- int4 [rows,cols] as q+7 ---
void put_i8(Bmp& b, const WTensor& t, int ch /*-1 = 8bpp*/, int row0) {
  int r, c;
  dims2(t, &r, &c);
  const int8_t* q = t.i8();
  for (int y = 0; y < r; y++)
    for (int x = 0; x < c; x++) {
      const int v = int(q[size_t(y) * c + x]) + 7;
      if (v < 0 || v > 14) fail("int4 weight out of range");
      if (ch < 0)
        b.row(row0 + y)[x] = uint8_t(v);
      else
        b.row(row0 + y)[size_t(x) * 4 + ch] = uint8_t(v);
    }
}
void get_i8(const Bmp& b, WTensor& t, int ch, int row0) {
  int r, c;
  dims2(t, &r, &c);
  if ((ch < 0 && b.bpp != 8) || (ch >= 0 && b.bpp != 32)) fail("image depth does not match");
  if (b.w != c || row0 + r > b.h) fail("image size does not match tensor");
  int8_t* q = reinterpret_cast<int8_t*>(t.data.data());
  for (int y = 0; y < r; y++)
    for (int x = 0; x < c; x++) {
      const uint8_t v = ch < 0 ? b.row(row0 + y)[x] : b.row(row0 + y)[size_t(x) * 4 + ch];
      if (v > 14) fail("pixel %u is not an int4 symbol", unsigned(v));
      q[size_t(y) * c + x] = int8_t(int(v) - 7);
    }
}

// --- bf16 [n] as two rows: high byte, low byte ---
void put_bf16(Bmp& b, const WTensor& t, int ch) {
  const uint16_t* s = t.bf16_bits();
  for (size_t i = 0; i < t.numel; i++) {
    const uint8_t hi = uint8_t(s[i] >> 8), lo = uint8_t(s[i]);
    if (ch < 0) {
      b.row(0)[i] = hi;
      b.row(1)[i] = lo;
    } else {
      b.row(0)[i * 4 + ch] = hi;
      b.row(1)[i * 4 + ch] = lo;
    }
  }
}
void get_bf16(const Bmp& b, WTensor& t, int ch) {
  if ((ch < 0 && b.bpp != 8) || (ch >= 0 && b.bpp != 32)) fail("image depth does not match");
  if (size_t(b.w) != t.numel || b.h != 2) fail("image size does not match scale tensor");
  uint16_t* s = reinterpret_cast<uint16_t*>(t.data.data());
  for (size_t i = 0; i < t.numel; i++) {
    const uint8_t hi = ch < 0 ? b.row(0)[i] : b.row(0)[i * 4 + ch];
    const uint8_t lo = ch < 0 ? b.row(1)[i] : b.row(1)[i * 4 + ch];
    s[i] = uint16_t((hi << 8) | lo);
  }
}

// --- f32 / i32 [rows,cols], one element per 32-bit pixel, bytes verbatim ---
void put_32(Bmp& b, const WTensor& t) {
  int r, c;
  dims2(t, &r, &c);
  for (int y = 0; y < r; y++) memcpy(b.row(y), t.data.data() + size_t(y) * c * 4, size_t(c) * 4);
}
void get_32(const Bmp& b, WTensor& t) {
  int r, c;
  dims2(t, &r, &c);
  if (b.bpp != 32 || b.w != c || b.h != r) fail("image size does not match tensor");
  for (int y = 0; y < r; y++) memcpy(t.data.data() + size_t(y) * c * 4, b.row(y), size_t(c) * 4);
}

std::string hex_of(const WTensor& t) {
  std::string s;
  char buf[16];
  const size_t es = esize(t.dtype);
  for (size_t i = 0; i < t.numel; i++) {
    uint32_t v = 0;
    for (size_t k = 0; k < es; k++) v |= uint32_t(t.data[i * es + k]) << (8 * k);
    snprintf(buf, sizeof buf, es == 2 ? " %04x" : " %08x", v);
    s += buf;
  }
  return s;
}

// short image name for a tensor: blocks.3.attention.query_projection.weight.q
// -> L03_attn_query_projection
std::string short_name(const std::string& name, const WeightsFile& wf) {
  std::string s = name;
  auto strip = [&](const char* suf, const char* rep) {
    const size_t n = strlen(suf);
    if (s.size() >= n && s.compare(s.size() - n, n, suf) == 0) s = s.substr(0, s.size() - n) + rep;
  };
  strip(".weight.q", "");
  strip(".weight.scale", "_scale");
  strip(".weight", "");
  strip(".value", "");
  if (s.rfind("blocks.", 0) == 0) {
    const size_t dot = s.find('.', 7);
    const int l = atoi(s.substr(7, dot - 7).c_str());
    std::string rest = s.substr(dot + 1);
    const bool kda = wf.has("blocks." + std::to_string(l) + ".attention.dt_bias");
    if (rest.rfind("attention.", 0) == 0) rest = std::string(kda ? "kda_" : "attn_") + rest.substr(10);
    else if (rest.rfind("mlp.", 0) == 0) rest = "mlp_" + rest.substr(4);
    char lb[8];
    snprintf(lb, sizeof lb, "L%02d_", l);
    s = lb + rest;
  }
  for (char& ch : s)
    if (ch == '.') ch = '_';
  return s;
}

// ---------------------------------------------------------------------------
// c: weights -> images + text
// ---------------------------------------------------------------------------

struct Entry {  // one line of the text file
  std::string name;
  std::string storage;
};

void unpack(const char* in, const std::string& prefix) {
  WeightsFile wf = WeightsFile::load_compressed(in);
  if (wf.order.size() != wf.tensors.size()) fail("loader did not record the tensor order");

  std::vector<Entry> entries;
  std::unordered_set<std::string> done;
  int n_images = 0;
  auto emit = [&](const std::string& file, const Bmp& b) {
    write_bmp(prefix + "_" + file, b);
    n_images++;
  };
  auto shape_str = [&](const WTensor& t) {
    std::string s = dtype_name(t.dtype);
    s += " " + std::to_string(t.shape.size());
    for (uint32_t d : t.shape) s += " " + std::to_string(d);
    return s;
  };
  auto add = [&](const std::string& name, const std::string& storage) {
    entries.push_back({name, shape_str(wf.get(name)) + " " + storage});
    done.insert(name);
  };

  for (const std::string& name : wf.order) {
    if (done.count(name)) continue;
    const WTensor& t = wf.get(name);

    if (name == "rope.sin" || name == "rope.cos") {
      add(name, "rope");
      continue;
    }
    if (t.dtype != fx2::DT_I8 && t.numel <= 32) {
      add(name, "hex" + hex_of(t));
      continue;
    }

    // the four projections of an attention block -> one RGBA image, and their
    // scales -> another (R=q, G=k, B=v, A=o; BMP byte order is B,G,R,A)
    const std::string proj = ".attention.";
    size_t p = name.find(proj);
    if (p != std::string::npos && name.rfind("_projection.weight.q") != std::string::npos &&
        name.find("gate_projection") == std::string::npos) {
      const std::string base = name.substr(0, p + proj.size());
      const char* kinds[4] = {"query", "key", "value", "output"};
      const int chan[4] = {2, 1, 0, 3};
      std::string q[4], sc[4];
      bool all = true;
      for (int i = 0; i < 4; i++) {
        q[i] = base + kinds[i] + "_projection.weight.q";
        sc[i] = base + kinds[i] + "_projection.weight.scale";
        all = all && wf.has(q[i]) && wf.has(sc[i]);
      }
      if (all) {
        const WTensor& t0 = wf.get(q[0]);
        int r, c;
        dims2(t0, &r, &c);
        const std::string img = short_name(base + "qkvo", wf) + ".bmp";
        Bmp b;
        b.alloc(c, r, 32);
        for (int i = 0; i < 4; i++) {
          put_i8(b, wf.get(q[i]), chan[i], 0);
          add(q[i], "bmp32ch " + img + " " + std::to_string(chan[i]));
        }
        emit(img, b);
        const std::string simg = short_name(base + "qkvo_scale", wf) + ".bmp";
        Bmp s;
        s.alloc(r, 2, 32);
        for (int i = 0; i < 4; i++) {
          put_bf16(s, wf.get(sc[i]), chan[i]);
          add(sc[i], "bmp32ch " + simg + " " + std::to_string(chan[i]));
        }
        emit(simg, s);
        continue;
      }
    }

    // a KDA block's two gate matrices, per direction, stacked in one image
    p = name.find("_gate_projection.");
    if (p != std::string::npos && name.rfind(".weight.q") != std::string::npos) {
      const std::string base = name.substr(0, name.find(".attention.") + 11);
      const std::string dir = name.substr(p + 17);  // "up.weight.q" / "down.weight.q"
      const std::string dirname = dir.substr(0, dir.find('.'));
      const std::string a = base + "forget_gate_projection." + dirname + ".weight.q";
      const std::string o = base + "output_gate_projection." + dirname + ".weight.q";
      if (wf.has(a) && wf.has(o)) {
        int r, c, r2, c2;
        dims2(wf.get(a), &r, &c);
        dims2(wf.get(o), &r2, &c2);
        if (c == c2) {
          const std::string img = short_name(base + "gates_" + dirname, wf) + ".bmp";
          Bmp b;
          b.alloc(c, r + r2, 8);
          b.int4_palette = true;
          put_i8(b, wf.get(a), -1, 0);
          put_i8(b, wf.get(o), -1, r);
          add(a, "bmp8rows " + img + " 0");
          add(o, "bmp8rows " + img + " " + std::to_string(r));
          emit(img, b);
          continue;
        }
      }
    }

    // everything else: one tensor, one image
    const std::string img = short_name(name, wf) + ".bmp";
    Bmp b;
    if (t.dtype == fx2::DT_I8) {
      int r, c;
      dims2(t, &r, &c);
      b.alloc(c, r, 8);
      b.int4_palette = true;
      put_i8(b, t, -1, 0);
      add(name, "bmp8 " + img);
    } else if (t.dtype == fx2::DT_BF16) {
      b.alloc(int(t.numel), 2, 8);
      put_bf16(b, t, -1);
      add(name, "bmp8 " + img);
    } else {
      int r, c;
      dims2(t, &r, &c);
      b.alloc(c, r, 32);
      put_32(b, t);
      add(name, "bmp32 " + img);
    }
    emit(img, b);
  }

  // the text file, in the input's tensor order
  const std::string meta = prefix + ".txt";
  FILE* f = fopen(meta.c_str(), "wb");
  if (!f) fail("cannot write %s", meta.c_str());
  fprintf(f, "tfwc 1\n");
  fprintf(f, "# name dtype ndim dims... storage\n");
  fprintf(f, "# storage: bmp8 FILE | bmp8rows FILE ROW0 | bmp32 FILE | bmp32ch FILE CH | hex WORDS | rope\n");
  fprintf(f, "# int4 pixels are q+7; bf16 images are two rows, high byte over low; 32-bit\n");
  fprintf(f, "# pixels are the element's four bytes, CH indexes them (B=0 G=1 R=2 A=3);\n");
  fprintf(f, "# every FILE is relative to the prefix, as prefix_FILE\n");
  fprintf(f, "tensors %zu\n", entries.size());
  std::unordered_map<std::string, const Entry*> by_name;
  for (const Entry& e : entries) by_name[e.name] = &e;
  for (const std::string& name : wf.order) fprintf(f, "%s %s\n", name.c_str(), by_name[name]->storage.c_str());
  fclose(f);
  printf("%s: %zu tensors -> %d images and %s\n", in, entries.size(), n_images, meta.c_str());
}

// ---------------------------------------------------------------------------
// d: images + text -> weights
// ---------------------------------------------------------------------------

std::vector<std::string> split(const std::string& line) {
  std::vector<std::string> v;
  size_t i = 0;
  while (i < line.size()) {
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r')) i++;
    size_t j = i;
    while (j < line.size() && line[j] != ' ' && line[j] != '\t' && line[j] != '\r') j++;
    if (j > i) v.push_back(line.substr(i, j - i));
    i = j;
  }
  return v;
}

void pack(const std::string& prefix, const char* out) {
  const std::string meta = prefix + ".txt";
  FILE* f = fopen(meta.c_str(), "rb");
  if (!f) fail("cannot open %s", meta.c_str());
  std::string text;
  char buf[4096];
  size_t n;
  while ((n = fread(buf, 1, sizeof buf, f)) > 0) text.append(buf, n);
  fclose(f);

  WeightsFile wf;
  std::unordered_map<std::string, Bmp> images;
  auto image = [&](const std::string& file) -> const Bmp& {
    auto it = images.find(file);
    if (it == images.end()) it = images.emplace(file, read_bmp(prefix + "_" + file)).first;
    return it->second;
  };

  size_t pos = 0;
  bool header = false;
  while (pos < text.size()) {
    size_t nl = text.find('\n', pos);
    if (nl == std::string::npos) nl = text.size();
    const std::string line = text.substr(pos, nl - pos);
    pos = nl + 1;
    std::vector<std::string> w = split(line);
    if (w.empty() || w[0][0] == '#') continue;
    if (!header) {
      if (w.size() < 2 || w[0] != "tfwc" || w[1] != "1") fail("%s: not a tfwc 1 file", meta.c_str());
      header = true;
      continue;
    }
    if (w[0] == "tensors") continue;

    // name dtype ndim dims... storage...
    if (w.size() < 4) fail("bad line: %s", line.c_str());
    WTensor t;
    t.dtype = dtype_code(w[1]);
    const int ndim = atoi(w[2].c_str());
    if (ndim < 1 || ndim > 2 || int(w.size()) < 4 + ndim) fail("bad line: %s", line.c_str());
    t.numel = 1;
    for (int i = 0; i < ndim; i++) {
      t.shape.push_back(uint32_t(strtoul(w[3 + i].c_str(), nullptr, 10)));
      t.numel *= t.shape.back();
    }
    const std::string& kind = w[3 + ndim];
    const size_t a0 = 4 + ndim;  // first storage argument
    if (kind == "rope") {
      // no payload: the encoder writes it as recomputed
    } else {
      t.data.assign(t.numel * esize(t.dtype), 0);
      if (kind == "hex") {
        if (w.size() - a0 != t.numel) fail("%s: hex word count", w[0].c_str());
        const size_t es = esize(t.dtype);
        for (size_t i = 0; i < t.numel; i++) {
          const uint32_t v = uint32_t(strtoul(w[a0 + i].c_str(), nullptr, 16));
          for (size_t k = 0; k < es; k++) t.data[i * es + k] = uint8_t(v >> (8 * k));
        }
      } else if (kind == "bmp8" || kind == "bmp8rows") {
        const Bmp& b = image(w[a0]);
        const int row0 = kind == "bmp8rows" ? atoi(w[a0 + 1].c_str()) : 0;
        if (t.dtype == fx2::DT_I8) get_i8(b, t, -1, row0);
        else if (t.dtype == fx2::DT_BF16) get_bf16(b, t, -1);
        else fail("%s: bmp8 holds int4 or bf16", w[0].c_str());
      } else if (kind == "bmp32ch") {
        const Bmp& b = image(w[a0]);
        const int ch = atoi(w[a0 + 1].c_str());
        if (t.dtype == fx2::DT_I8) get_i8(b, t, ch, 0);
        else if (t.dtype == fx2::DT_BF16) get_bf16(b, t, ch);
        else fail("%s: bmp32ch holds int4 or bf16", w[0].c_str());
      } else if (kind == "bmp32") {
        get_32(image(w[a0]), t);
      } else {
        fail("%s: unknown storage '%s'", w[0].c_str(), kind.c_str());
      }
    }
    wf.order.push_back(w[0]);
    wf.tensors[w[0]] = std::move(t);
  }
  if (!header) fail("%s: empty", meta.c_str());
  if (!fx2::write_weights_v2(out, wf, wf.order)) fail("writing %s failed", out);
  printf("%s: %zu tensors from %zu images -> %s\n", meta.c_str(), wf.order.size(), images.size(), out);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 4 || (argv[1][0] != 'c' && argv[1][0] != 'd')) {
    fprintf(stderr,
            "usage: tfwc c input.tfwc2 output_prefix    unpack to prefix.txt + prefix_*.bmp\n"
            "       tfwc d input_prefix output.tfwc2    pack them back, losslessly\n");
    return 1;
  }
  if (argv[1][0] == 'c')
    unpack(argv[2], argv[3]);
  else
    pack(argv[2], argv[3]);
  return 0;
}
