// dxt5comp.cpp -- standalone field-aware context-mixing compressor for
// DXT1/DXT3/DXT5 (BC1/2/3), BC4/ATI1, BC5/ATI2, BTC and uncompressed
// (bitmap-like) .dds files, with legacy, D3DFORMAT-in-fourcc or DX10 headers,
// implementing dxt5_cm_model.md:
//
//   * raster block order; per block a0,a1,c0,c1,ai[0..15],ci[0..15] --
//     endpoints first so every index conditions on the known palette
//   * pair-match models for (a0,a1) and (c0,c1): W / N / hash-of-(W,N)
//     candidates, one bit each, coded only when the candidate is distinct
//   * endpoint literals as MSB-first binary trees, mixed contexts per
//     5/6/5-bit channel: same-channel W, N, NE (N for e1), 2D gradient
//     clamp(W+N-NW), joint (W,N) (joint (e0.ch,W) for e1), cross-channel
//     r->g->b within an endpoint, pair-hash candidate channel
//   * word-match models for the 48-bit ai and 32-bit ci index words: four
//     candidates -- W, N, hash keyed by this block's endpoint pair, hash
//     keyed by (W,N) words -- coded only while distinct; mismatched
//     candidates are still salvaged per texel as predicted-symbol contexts
//   * per-texel index models through the palette-map predictor
//     p^ = argmin_k dist(decoded neighbor pixel, current palette[k]),
//     tie -> lower k, computed on the causally decoded pixel planes so it
//     crosses block borders and follows gradients through palette changes.
//     ci contexts: (p^L,p^U,Wsame), (p^L,p^U,p^UL), (p^L,p^U,p^UL,p^UR)
//     (+xWsame, + slow-rate twin), (p^UL,p^UR), raw-index (L,U) and
//     (L,U,UL), scan-prev x texel position, W/N same-position x p^, both
//     hash-candidate texels; ai contexts analogous with mode + |a0-a1|
//     bucket folded in. Logistic mixing (sets selected on p^ agreement,
//     mode/d-bucket or palette distinct-count, tree node) + SSE/APM stages
//   * DXT1 = the color half verbatim, with the meaningful c0<=c1 mode bit
//     as index-model context (3-color palette geometry); DXT3 alpha = plain
//     4-bit gradient plane (W/N/2D contexts), not an index plane
//   * BC4/ATI1 = the DXT5 alpha half on its own (8-byte block, no 565 half);
//     BC5/ATI2 = two such channels, coded as two full passes over the level so
//     each has its own causal neighbourhood, while the adaptive tables stay
//     shared -- the channels have the same kind of statistics and a shared
//     model warms up twice as fast
//   * BTC ('BTC ' fourcc, 4-byte block) = the same channel with 1-bit
//     selectors over a 2-entry palette: the index coder is parameterised on
//     symbol width (fmt_ibits) and palette size (fmt_npal), so the whole
//     endpoint path, the word-match models and every p^ context carry over
//   * a bitmap-like dds is nothing but endpoints -- no indices, no palette --
//     so the raster goes straight through the endpoint literal machinery:
//     same tree(), same mixer, same 256-row tables, with the spatial contexts
//     an endpoint plane already uses (W, N, 2D clamp(W+N-NW), MED) plus the
//     previous channel of the same pixel, which is the cross-channel r->g->b
//     link the colour endpoints use. One byte plane per channel, so a 16-bit
//     channel is two planes and the low byte's noise never pollutes the high
//     byte's model
//   * cubemaps and DX10 arrays: faces/slices each carry the full mipchain, so
//     the coder walks face-major and resets the mip-parent state per face --
//     otherwise face N's level 0 would take face N-1's smallest level as its
//     2x-finer parent. A lax writer that sets the cubemap caps on a file
//     holding one face is caught by checking the caps against the file size
//   * BC6H/BC7 have no fixed field layout -- a mode field at the bottom of
//     byte 0 selects one of 8/14 layouts with different partition, endpoint
//     and index widths -- so they are coded byte-wise with (mode, position in
//     block, W/N block's byte at that position, previous byte, 2D gradient)
//     as context. Contexts in IDX/dxt5-P0.idx; a per-mode field decomposition
//     is the next step, and this is the shape it builds on
//   * volume textures: dwDepth slices per level, halving alongside width and
//     height, each slice coded as its own 2D image
//   * container header is 128 bytes, or 148 when the fourcc announces DX10
//   * a block grid whose width/height is not a multiple of 4 is resolved
//     against the file size (ceil grid, else floor); leftovers go to the tail
//   * mipchains: each level is its own 2D block grid, model state carried
//     across levels; trailing bytes after the last level: order-1
//
// container: "D5C1" u32origsize u32crc32 raw dds header (128 B, 148 with
// DX10), rc stream
// decode verifies crc32 of the reconstructed file.
//
// on 0000006F.dds (2820x1600 DXT5, 4 512 128 B): 1 027 480 B lossless,
// ~1.7 s each direction -- vs xz -9e 1 461 848, webp-lossless cvpk split
// 1 300 628, best transform pipeline (cpk) 1 033 874. Plane accounting:
// cep 334.5K (webp ep 389.5K), ci+match 535.0K (webp 521.6K), ai+match
// 133.7K (webp 122.7K), aep 23.9K.
//
// every context index and every arbitrary constant is declared in IDX/*.idx
// and reached only through the generated MOD/*_h.inc / MOD/*_p.inc; nothing in
// this file hardcodes a context layout or a tuning value:
//
//   G0  counter rate schedule, mixer learning rates / weight init / clamp,
//       mixer bias, APM blend weights and rate
//   M0  match-model history contexts + hash table widths
//   A0  alpha endpoint literal contexts
//   B0  alpha index-plane contexts, mixer selection, both SSE stages, slow twin
//   C0  color index-plane contexts, mixer selection, both SSE stages, slow twin
//   E0  color endpoint literal contexts (per-channel and cross-channel)
//   X0  DXT3 nibble-plane contexts + trailing-byte order-1
//   P0  BC6H/BC7 byte-wise contexts
//
// The core allocates nothing per texture: every context table is a generated
// IDX Table() and every per-level buffer is a fixed slab sized by MAX_DIM
// (D5C_MAX_DIM, default 4096).  A texture larger than that cannot be modelled
// and parse_dds refuses it -- here that is a hard error, since a standalone
// .d5c has no verbatim-store path the way a solid archive does.  Rebuild with
// -DD5C_MAX_DIM=8192 for larger textures, or =2048 to shrink the codec object.
//
// build:  ./mk.sh            debug/optimizable (patterns live in the binary)
//         ./mk.sh release    const (patterns folded at preprocess time)
//         g.bat [release]    same under Windows/MinGW
// tune:   perl IDX/opt.pl [corpus.lst]   then fold export.!!! back with
//         cd IDX && for f in *.idx; do perl import.pl $f ../export.!!! >t && mv t $f; done
// every generated value passes through pclamp/PSHIFT/PHBITS/PBLEND before use,
// so no bit pattern the optimizer can produce makes the codec crash or
// allocate unbounded memory.
//
// usage: dxt5comp c|d input output

// the core's RC is a Coroutine layer now, so the Lib3 chain comes first
#include "common.inc"
#include "coro3b.inc"
#include "coro_fp.inc"

#include <stdint.h>
#include <math.h>

#include <utility>

#include "dxt5comp_core.inc"
using namespace d5c;

// Informational output. Build the optimizer binary with -DQUIET to silence it.
// Do NOT use fclose(stdout) for this: every later printf then operates on a
// closed stream -- glibc quietly fails, MSVC/UCRT frees the FILE and crashes.
// Errors always go to stderr and stay visible.
#ifdef QUIET
  #define MSG(...) ((void)0)
#else
  #define MSG(...) printf(__VA_ARGS__)
#endif

//------------------------------------------------------------------- file io

static bool load_file(const char* p, ByteBuf& v) {
  // chunked read: ftell()'s long is 32-bit on Windows, so a size-then-read
  // load silently truncates past 2 GB. no size query, no 64-bit ifdefs.
  FILE* f = fopen(p, "rb"); if(!f) return false;
  v.clear(); v.reserve(1 << 20);
  static u8 buf[1 << 16];
  for(size_t n; (n = fread(buf, 1, sizeof buf, f)) > 0; ) v.append(buf, n);
  bool ok = !ferror(f);
  fclose(f); return ok;
}
static bool save_file(const char* p, const u8* d, size_t n) {
  FILE* f = fopen(p, "wb"); if(!f) return false;
  bool ok = !n || fwrite(d, 1, n, f) == n;
  return fclose(f) == 0 && ok;
}

//------------------------------------------------------------------- top level

static const char MAGIC[4] = {'D', '5', 'C', '1'};

/* The Codec carries every context table and every MAX_DIM geometry slab as a
 * member, so in the Const build it is a few hundred MiB.  That is fine as one
 * demand-zero heap allocation, but a `static Codec` in each of compress() and
 * decompress() would put two of them in BSS, and a local would overflow the
 * stack outright.  One object, made on first use, serves whichever direction
 * runs. */
/* The coder runs inside a coroutine so the compressed stream moves through a
 * pin instead of a buffer.  d5cM<MD> derives from RC<MD>, which derives from
 * Coroutine, so this object IS the coroutine and the range coder is a layer of
 * it rather than a second instance.  rcio is left at its default: the .d5c is
 * this tool's only stream, so it is exactly the inherited put()/get() on
 * pin1/pin0 and no extra pin is needed.
 *
 * The .dds side stays in memory in both directions, and not out of laziness:
 * the container's crc32 covers the whole file and is written into the header
 * before any coded byte, and on decode it must verify before a single byte is
 * emitted.  The compressed side is what streams. */
template<int MD>
struct d5cM : RC<MD> {
  typedef RC<MD> RCB;
  using RCB::pin; using RCB::f_quit;
  using RCB::get; using RCB::put;
  using RCB::addinp; using RCB::addout;
  using RCB::coro_init; using RCB::coro_call; using RCB::getoutsize;
  using RCB::outbeg;

  const char* inp = 0;
  int rcode = 0;                // nonzero -> fatal
  ByteBuf dds;                  // the raw texture, both directions
  dds_info di;
  u64 coded = 0;                // .d5c size, for the summary line

  void putn(const u8* p, size_t n) { for(size_t i = 0; i < n; i++) put(p[i]); }
  // -1 on EOF; every header read checks, so a truncated .d5c is caught here
  bool getn(u8* p, size_t n) {
    for(size_t i = 0; i < n; i++) { uint c = get(); if(c > 255) return false; p[i] = u8(c); }
    return true;
  }

  /* One Codec per direction, made on first use and bound to this coroutine.
   * There is exactly one d5cM<MD> object in the process, so the reference it
   * hands the Codec stays valid for the program's life -- no rebinding. */
  Codec<MD>& codec() { static Codec<MD>* p = d5c::onew< Codec<MD> >("codec", *this); return *p; }

  /* `lim` is the entry's real length.  A truncated file declares a mipchain
   * longer than the bytes present -- dxt1a.dds is 16 bytes short of its own
   * chain -- and walking the declared geometry read straight off the end of the
   * buffer.  Same rule as the solid path: whole levels while they fit, then
   * whole blocks of the one that does not. */
  void walk( Codec<MD>& cx, size_t off, size_t lim ) {
    double lvlprev = 0;
    for(u32 f = 0; f < di.faces; f++) {
      texture_init(cx);   // each face is its own mipchain: no cross-face parent
      for(u32 l = 0; l < di.levels; l++) {
        u32 w, h, bw, bh; level_dims(di, l, w, h, bw, bh);
        for(u32 z = 0; z < level_depth(di, l); z++) {
          const size_t bytes = size_t(bw) * bh * di.bsize;
          u32 nblk_ = bw * bh;
          if(di.truncated) {
            const size_t left = off < lim ? lim - off : 0;
            if(bytes > left) {
              nblk_ = u32(left / di.bsize);
              if(!nblk_) { l = di.levels; f = di.faces; break; }
            }
          }
          cx.blk_lim = (nblk_ == bw * bh) ? 0 : nblk_;
          if(di.fmt == FMT_RAW)       cx.code_raster(dds.data() + off, w, h, di.bsize);
          else if(di.fmt == FMT_MODE) cx.code_mode_blocks(dds.data() + off, bw, bh, di.bsize);
          else { cx.level_init(bw, bh); cx.code_level(dds.data() + off); }
          cx.blk_lim = 0;
          off += size_t(nblk_) * di.bsize;
          if(nblk_ != bw * bh) { l = di.levels; f = di.faces; break; }
        }
        if(MD == 0 && getenv("D5C_LVLSTAT")) {
          double tot = 0; for(int i = 0; i < 8; i++) tot += cx.stat[i];
          double cur = (tot - lvlprev) / 2048.0; lvlprev = tot;
          fprintf(stderr, "  L%-2u %4ux%-4u %7llu blk %9.0f B  %6.3f b/blk\n", l, w, h,
                  (unsigned long long)(size_t(bw) * bh), cur, 8.0 * cur / (double(bw) * bh));
        }
      }
    }
    cx.code_tail(dds.data() + off, dds.size() - off);
  }

  void do_encode( void ) {
    if(!load_file(inp, dds)) { fprintf(stderr, "error: can't read '%s'\n", inp); rcode = 1; return; }
    const char* err = "";
    if(!parse_dds(dds.data(), dds.size(), di, &err)) {
      fprintf(stderr, "error: '%s': %s\n", inp, err); rcode = 1; return;
    }
    u32 osz = u32(dds.size()), crc = crc32b(dds.data(), dds.size());
    putn((const u8*)MAGIC, 4);
    putn((const u8*)&osz, 4);
    putn((const u8*)&crc, 4);
    putn(dds.data(), di.data_off);          // the dds header, verbatim

    Codec<0>& cx = codec();
    cx.init(di.fmt);
    d5c::texture_reserve(cx, di);   // size the geometry slabs for this texture
    cx.rc.init_enc(0);                      // 0 = the default pin, i.e. put()
    walk(cx, di.data_off, dds.size());
    cx.rc.flush();
    coded = 12 + di.data_off + cx.rc.emitted;
  }

  void do_decode( void ) {
    u8 h[12]; u32 osz = 0, crc = 0;
    if(!getn(h, 12) || memcmp(h, MAGIC, 4)) {
      fprintf(stderr, "error: '%s' is not a dxt5comp file\n", inp); rcode = 1; return;
    }
    memcpy(&osz, h + 4, 4); memcpy(&crc, h + 8, 4);
    if(osz < 128) { fprintf(stderr, "error: bad container\n"); rcode = 1; return; }
    dds.assign_zero(osz);
    if(!getn(dds.data(), 128)) { fprintf(stderr, "error: bad container\n"); rcode = 1; return; }
    // 128 bytes, or 148 when the fourcc announces a DX10 block
    if(!memcmp(dds.data() + 84, "DX10", 4)) {
      if(osz < 148 || !getn(dds.data() + 128, 20)) {
        fprintf(stderr, "error: bad container\n"); rcode = 1; return;
      }
    }
    const char* err = "";
    if(!parse_dds(dds.data(), osz, di, &err)) {
      fprintf(stderr, "error: bad stored header: %s\n", err); rcode = 1; return;
    }
    // the coded region must fit what the header describes
    for(u32 l = 0, off = 0; l < di.levels; l++) {
      u32 w, h2, bw, bh; level_dims(di, l, w, h2, bw, bh);
      (void)w; (void)h2; (void)off;
    }
    Codec<1>& cx = codec();
    cx.init(di.fmt);
    d5c::texture_reserve(cx, di);   // size the geometry slabs for this texture
    cx.rc.init_dec(0);                      // 0 = the default pin, i.e. get()
    size_t need = di.data_off;
    for(u32 f = 0; f < di.faces; f++)
      for(u32 l = 0; l < di.levels; l++) {
        u32 w, h2, bw, bh; level_dims(di, l, w, h2, bw, bh);
        need += size_t(bw) * bh * di.bsize * level_depth(di, l);
      }
    /* A truncated entry legitimately declares more than it has; parse_dds has
     * already reduced di.nblk to what fits, and walk() stops there. */
    if(!di.truncated && need > dds.size()) { fprintf(stderr, "error: container size mismatch\n"); rcode = 1; return; }
    walk(cx, di.data_off, dds.size());
    if(crc32b(dds.data(), dds.size()) != crc) {
      fprintf(stderr, "error: crc mismatch after decode\n"); rcode = 1; return;
    }
    putn(dds.data(), dds.size());           // only now, with the crc verified
  }

  void do_process( void ) {
    if constexpr( MD == 0 ) do_encode(); else do_decode();
    yield(this, 0);
  }
};

/* Built on first use, so a run constructs only the direction it needs.  As
 * statics both were constructed at load however the tool was invoked, and each
 * carrier holds a 64 KB coroutine stack plus the codec's carrier state. */
static CoroFileProc< d5cM<0> >* Enc_ = nullptr;
static CoroFileProc< d5cM<1> >* Dec_ = nullptr;
static CoroFileProc< d5cM<0> >& Enc() { if(!Enc_) Enc_ = d5c::onew< CoroFileProc< d5cM<0> > >("encoder carrier"); return *Enc_; }
static CoroFileProc< d5cM<1> >& Dec() { if(!Dec_) Dec_ = d5c::onew< CoroFileProc< d5cM<1> > >("decoder carrier"); return *Dec_; }

static int compress(const char* inp, const char* outp) {
  FILE* f = fopen(inp, "rb");          // the coroutine loads the dds itself
  Enc().inp = inp;
  Enc().out.path = outp;               // created on the first byte written
  Enc().processfile(f, 0);
  if(f) fclose(f);
  bool wok = Enc().out.close();
  if(Enc().rcode) return Enc().rcode;
  if(!wok) return fprintf(stderr, "error: can't write '%s'\n", outp), 1;
  const dds_info& di = Enc().di;
  /* FMT_RAW counts samples, not 4x4 blocks -- calling them "blocks" made a
   * 1280x720 raster look like it had 921600 of them. */
  MSG("%s -> %s: %zu -> %llu bytes (%.3f bpb), %s %ux%u %u level(s), %llu %s\n",
      inp, outp, Enc().dds.size(), (unsigned long long)Enc().coded,
      8.0 * (double)Enc().coded / Enc().dds.size(),
      fmt_name(di.fmt), di.w, di.h, di.levels, (unsigned long long)di.nblk,
      di.fmt == FMT_RAW ? "samples" : "blocks");
  static const char* cat[8] = {"aep", "cep", "aiw", "ai ", "ciw", "ci ", "x4 ", "tail"};
  Codec<0>& cx = Enc().codec();
  for(int i = 0; i < 8; i++)
    if(cx.stat[i]) MSG("  %s %9.0f bytes\n", cat[i], cx.stat[i] / 2048.0);
  return 0;
}

static int decompress(const char* inp, const char* outp) {
  FILE* f = fopen(inp, "rb");
  if(!f) return fprintf(stderr, "error: can't read '%s'\n", inp), 1;
  Dec().inp = inp;
  Dec().out.path = outp;               // created on the first byte written
  Dec().processfile(f, 0);
  fclose(f);
  bool wok = Dec().out.close();
  if(Dec().rcode) return Dec().rcode;
  if(!wok) return fprintf(stderr, "error: can't write '%s'\n", outp), 1;
  MSG("%s -> %s: %zu bytes, crc ok\n", inp, outp, Dec().dds.size());
  return 0;
}

int main(int argc, char** argv) {
  init_tables();
  if(argc == 4 && !strcmp(argv[1], "c")) { int r = compress(argv[2], argv[3]);   return r; }
  if(argc == 4 && !strcmp(argv[1], "d")) { int r = decompress(argv[2], argv[3]); return r; }
  fprintf(stderr,
          "dxt5comp - field-aware context-mixing compressor for DXT1/3/5 dds\n"
          "usage: dxt5comp c input.dds output.d5c\n"
          "       dxt5comp d input.d5c output.dds\n");
  return 1;
}

