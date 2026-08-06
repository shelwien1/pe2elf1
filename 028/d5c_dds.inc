/* d5c_dds.inc -- part of dxt5comp_core.inc; not standalone.
 *
 * dds container parsing: dds_info, the fmt_* predicates, parse_dds,
 * level_dims / level_depth / chain_size.  No model state. */

//------------------------------------------------------------------- dds parsing (per dxt5.cpp)

enum dxt_fmt { FMT_DXT1 = 1, FMT_DXT3 = 3, FMT_DXT5 = 5, FMT_BC4 = 6, FMT_BC5 = 7,
               FMT_BTC = 9, FMT_RAW = 10, FMT_MODE = 11 };
struct dds_info {
  dxt_fmt fmt; u32 w, h, levels, bsize; u64 nblk; size_t data_off;
  bool floor_grid;  // grid is w/4 x h/4 rather than the usual ceil
  bool truncated;   // payload is short: code what fits, rest to the tail
  u32 bedge;        // texels per block edge: 4, or 1 for an uncompressed raster
  u32 faces;        // 1, or 6 for a cubemap (x arraySize for a DX10 cube array);
                    // each face carries the full mipchain
  u32 depth;        // 1, or the slice count of a volume texture (halves per level)
};

// 565 colour half present (two RGB565 endpoints + 16 2-bit indices)
static constexpr bool fmt_color(dxt_fmt f) { return f == FMT_DXT1 || f == FMT_DXT3 || f == FMT_DXT5; }
// BC4-style channels (8+8-bit endpoints + 16 3-bit indices): DXT5's alpha half,
// BC4's single channel, BC5's two. Each is coded by the alpha models, and for
// BC5 the second channel gets its own causal state so its contexts never see
// the first channel's pixels.
static constexpr int fmt_ach(dxt_fmt f) {
  return f == FMT_BC5 ? 2 : (f == FMT_DXT5 || f == FMT_BC4 || f == FMT_BTC) ? 1 : 0;
}
// bits per texel index in those channels: 3, or 1 for BTC's selector
static constexpr int fmt_ibits(dxt_fmt f) { return f == FMT_BTC ? 1 : 3; }
// palette entries the index selects between: 8, or 2 for BTC
static constexpr int fmt_npal(dxt_fmt f) { return f == FMT_BTC ? 2 : 8; }
// bytes per channel in the block: endpoint pair + the packed indices
static constexpr int fmt_chsize(dxt_fmt f) { return f == FMT_BTC ? 4 : 8; }
static inline const char* fmt_name(dxt_fmt f) {
  return f == FMT_DXT1 ? "DXT1" : f == FMT_DXT3 ? "DXT3" : f == FMT_DXT5 ? "DXT5"
       : f == FMT_BC4 ? "BC4/ATI1" : f == FMT_BC5 ? "BC5/ATI2" : f == FMT_BTC ? "BTC"
       : f == FMT_RAW ? "uncompressed" : f == FMT_MODE ? "BC6H/BC7" : "?";
}

static void level_dims(const dds_info& di, u32 l, u32& w, u32& h, u32& bw, u32& bh) {
  w = di.w >> l ? di.w >> l : 1;
  h = di.h >> l ? di.h >> l : 1;
  const u32 e = di.bedge ? di.bedge : 4;
  if(di.floor_grid) { bw = w / e ? w / e : 1; bh = h / e ? h / e : 1; }
  else              { bw = (w + e - 1) / e;   bh = (h + e - 1) / e; }
}

// slices of a volume level: depth halves alongside width and height
static inline u32 level_depth(const dds_info& di, u32 l) {
  u32 d = (di.depth ? di.depth : 1) >> l;
  return d ? d : 1;
}

// blocks and bytes for ONE face's mipchain under one rounding rule
static void chain_size(const dds_info& di, bool fl, u64& blocks, size_t& bytes) {
  dds_info t = di; t.floor_grid = fl;
  blocks = 0; bytes = 0;
  for(u32 l = 0; l < di.levels; l++) {
    u32 w, h, bw, bh; level_dims(t, l, w, h, bw, bh);
    const u64 sl = level_depth(di, l);
    blocks += u64(bw) * bh * sl; bytes += size_t(bw) * bh * di.bsize * sl;
  }
}

static bool parse_dds(const u8* dds, size_t size, dds_info& di, const char** err) {
  u32 fc, flags, mips, caps2, pfflags;
  di.faces = 0; di.depth = 1;
  if(size < 128 || memcmp(dds, "DDS ", 4)) return *err = "not a dds", false;
  memcpy(&di.h, dds + 12, 4); memcpy(&di.w, dds + 16, 4);
  memcpy(&flags, dds + 8, 4); memcpy(&mips, dds + 28, 4);
  memcpy(&pfflags, dds + 80, 4); memcpy(&fc, dds + 84, 4);
  memcpy(&caps2, dds + 112, 4);  // dwCaps2 (116 is dwCaps3 -- was reading the wrong field)
  di.data_off = 128;
  di.bedge = 4;
  // A recognised tag identifies the format even when DDPF_FOURCC is clear: the
  // BTC writers leave dwSize/dwFlags/dwCaps and the whole ddspf zero.
  const bool known_fc = !memcmp(&fc, "DXT", 3) || !memcmp(&fc, "RXGB", 4) ||
                        !memcmp(&fc, "ATI", 3) || !memcmp(&fc, "BC4", 3) ||
                        !memcmp(&fc, "BC5", 3) || !memcmp(&fc, "DX10", 4) ||
                        !memcmp(&fc, "BTC ", 4);
  if(!(pfflags & 0x4) && !known_fc) {
    // Bitmap-like: DDPF_RGB / LUMINANCE / ALPHA.  There is no block structure
    // at all -- every pixel is simply an endpoint -- so the raster is coded by
    // the endpoint literal models directly, one 1x1 "block" per pixel.
    if(!(pfflags & (0x40 | 0x20000 | 0x2))) return *err = "not a fourcc/compressed dds", false;
    u32 bits; memcpy(&bits, dds + 88, 4);
    if(bits < 8 || bits > 128 || (bits & 7)) return *err = "raster bit depth is not a whole number of bytes", false;
    di.fmt = FMT_RAW; di.bedge = 1; di.bsize = bits / 8;
  } else
  if(!memcmp(&fc, "BTC ", 4)) di.fmt = FMT_BTC;
  else if(!memcmp(&fc, "DXT1", 4)) di.fmt = FMT_DXT1;
  else if(!memcmp(&fc, "DXT2", 4) || !memcmp(&fc, "DXT3", 4)) di.fmt = FMT_DXT3;
  else if(!memcmp(&fc, "DXT4", 4) || !memcmp(&fc, "DXT5", 4) ||
          !memcmp(&fc, "RXGB", 4)) di.fmt = FMT_DXT5;  // RXGB: DXT5 layout, swizzled semantics
  else if(!memcmp(&fc, "ATI1", 4) || !memcmp(&fc, "BC4U", 4) ||
          !memcmp(&fc, "BC4S", 4)) di.fmt = FMT_BC4;
  else if(!memcmp(&fc, "ATI2", 4) || !memcmp(&fc, "BC5U", 4) ||
          !memcmp(&fc, "BC5S", 4)) di.fmt = FMT_BC5;
  else if(!memcmp(&fc, "DX10", 4)) {
    if(size < 148) return *err = "truncated DX10 header", false;
    u32 dxgi, rd, misc, asz;
    memcpy(&dxgi, dds + 128, 4); memcpy(&rd, dds + 132, 4);
    memcpy(&misc, dds + 136, 4); memcpy(&asz, dds + 140, 4);
    if(dxgi >= 70 && dxgi <= 72) di.fmt = FMT_DXT1;
    else if(dxgi >= 73 && dxgi <= 75) di.fmt = FMT_DXT3;
    else if(dxgi >= 76 && dxgi <= 78) di.fmt = FMT_DXT5;
    else if(dxgi >= 79 && dxgi <= 81) di.fmt = FMT_BC4;
    else if(dxgi >= 82 && dxgi <= 84) di.fmt = FMT_BC5;
    else if(dxgi >= 94 && dxgi <= 99) di.fmt = FMT_MODE;  // BC6H, BC7
    else return *err = "DX10 dxgiFormat is not BC1-BC7", false;
    if(rd == 4) { u32 dep; memcpy(&dep, dds + 24, 4); di.depth = dep ? dep : 1; }
    else if(rd != 3) return *err = "DX10 resourceDimension is not TEXTURE2D/3D", false;
    u32 a = asz ? asz : 1;
    if(a > 4096) return *err = "DX10 arraySize out of range", false;
    di.faces = (misc & 0x4) ? a * 6 : a;   // cube array, array, or plain 2d
    di.data_off = 148;
  }
  else if(fc < 256) {
    // a D3DFORMAT enum in the fourcc slot: the wide-channel rasters, which no
    // mask description can express.  Each byte position becomes its own plane.
    u32 bpp = fc == 36 || fc == 110 || fc == 113 || fc == 115 ? 8
            : fc == 111 || fc == 117 ? 2 : fc == 112 || fc == 114 ? 4 : fc == 116 ? 16 : 0;
    if(!bpp) return *err = "unknown D3DFORMAT code in the fourcc slot", false;
    di.fmt = FMT_RAW; di.bedge = 1; di.bsize = bpp;
  }
  else return *err = "fourcc is not DXT1-5/RXGB/ATI1/ATI2/BC4x/BC5x/BTC/DX10", false;
  if(caps2 & 0x200000) {              // DDSCAPS2_VOLUME: dwDepth slices per level
    u32 dep; memcpy(&dep, dds + 24, 4);
    di.depth = dep ? dep : 1;
    if(di.depth > 65536) return *err = "volume depth out of range", false;
  }
  if(di.faces == 0) {                 // not already set by the DX10 branch
    di.faces = 1;
    if(caps2 & 0x200) {               // DDSCAPS2_CUBEMAP: one mipchain per face
      u32 n = 0;
      for(int i = 0; i < 6; i++) if(caps2 & (0x400u << i)) n++;
      di.faces = n ? n : 1;
    }
  }
  if(!di.w || !di.h) return *err = "bad dimensions", false;
  /* The per-level buffers are sized for MAX_DIM (see above), so anything
   * bigger cannot be modelled.  Rejecting here is the right place: the
   * encoder's caller already treats a parse failure as "store this texture
   * verbatim", which stays lossless, and the decoder refuses the entry before
   * any buffer is indexed. */
  if(di.w > MAX_DIM || di.h > MAX_DIM) return *err = "dimensions exceed the model's MAX_DIM", false;
  if(di.fmt != FMT_RAW)
    di.bsize = di.fmt == FMT_BTC ? 4 : (di.fmt == FMT_DXT1 || di.fmt == FMT_BC4) ? 8 : 16;
  di.levels = (flags & 0x20000) && mips ? mips : 1;
  /* dwWidth is not always a multiple of 4 and writers disagree about the
   * partial edge column: most round up, some drop it.  Guessing wrong shifts
   * every block row, so pick the grid the payload actually fits.  size is the
   * whole file on both sides (encoder: n, decoder: the coded entry length), so
   * the choice replays identically.  Whatever is left over after the chosen
   * chain is trailing data and goes to the tail model. */
  const size_t avail = size > di.data_off ? size - di.data_off : 0;
  u64 cb, fb; size_t cn, fn;
  di.floor_grid = false; chain_size(di, false, cb, cn);
  chain_size(di, true, fb, fn);
  // a lax writer may set the cubemap caps on a file holding a single face;
  // trust the file size over the flags
  if(di.faces > 1 && cn * di.faces > avail && cn <= avail) di.faces = 1;
  di.truncated = false;
  if(cn * di.faces <= avail) { di.nblk = cb * di.faces; return true; }
  if(fn && fn * di.faces <= avail) { di.floor_grid = true; di.nblk = fb * di.faces; return true; }

  /* Truncated: the declared mipchain does not fit.  Refusing here sent the whole
   * entry to a verbatim store, which is worse than leaving it in the gap; code
   * what IS there instead.  Whole levels while they fit, then whole BLOCKS of
   * the level that does not -- the last partial row is still modelled, and only
   * the partial block falls to the tail model along with everything after it.
   *
   * size is the whole entry on both sides, so the decoder derives the identical
   * stopping point and nothing has to be recorded in the stream. */
  di.truncated = true;
  di.nblk = 0;
  size_t left = avail;
  for(u32 f = 0; f < di.faces; f++)
    for(u32 l = 0; l < di.levels; l++) {
      u32 lw, lh, bw, bh; level_dims(di, l, lw, lh, bw, bh);
      for(u32 z = 0; z < level_depth(di, l); z++) {
        const size_t need = size_t(bw) * bh * di.bsize;
        if(need <= left) { di.nblk += u64(bw) * bh; left -= need; continue; }
        di.nblk += left / di.bsize;                      // whole blocks only
        return true;
      }
    }
  return true;
}
