/* ddsdet - DDS detector / extractor ("carver") with lossless restore.
 *
 *   ddsdet c [--z|--zero] inputfile outputfile [dds_prefix]
 *     CARVE: forward-scan inputfile for embedded DDS textures. Each texture is
 *     written out to  sprintf("%s%08X.dds", dds_prefix, index)  and replaced in
 *     outputfile by a self-describing 44-byte marker.
 *       default     : DDS bytes are removed (outputfile shrinks).
 *       --z/--zero  : marker is padded with zero bytes back to the texture's
 *                     original length, so every byte offset after it in
 *                     outputfile is preserved.
 *     The marker begins with a real DDS signature ("DDS ") followed by
 *     metadata, so restoration needs no separate sidecar file.
 *
 *   ddsdet cs [--z|--zero] inputfile restfile ddsfile
 *     CARVE SOLID: as above, but instead of one sidecar per texture, every
 *     texture is coded into a single .d5s archive by the dxt5comp model, with
 *     one codec instance spanning the whole archive (counters, mixer weights,
 *     APM tables and match hash tables all carry over between .dds files).
 *     --ns turns that off: the model is reset once per .dds file -- per FILE,
 *     not per mip level, so cross-level prediction inside a file is kept. The
 *     container minus its textures -- the part this tool cannot model -- is
 *     written to restfile uncompressed.
 *
 *   ddsdet ds restfile ddsfile outputfile
 *     RESTORE SOLID: reverse of cs.
 *
 *   ddsdet cs --bmp inputfile restfile prefix   /   ddsdet ds --bmp restfile prefix outputfile
 *     As cs/ds, but the endpoint fields of every modelled texture are written
 *     to prefix%08X_ep.bmp (32bpp, dxt5.cpp "cv" layout) instead of being
 *     range-coded, so an external image codec can attack the plane that
 *     dominates the archive.  The index planes stay in the model and lose
 *     nothing: the decoder reads the image back before decoding indices, so
 *     the palette, p^ and every endpoint-derived context still see the real
 *     values.  Textures dxt5comp cannot model are stored verbatim as usual and
 *     get no image.
 *
 *   ddsdet d inputfile outputfile [dds_prefix]
 *     RESTORE: reverse a prior carve. Scan inputfile for markers, splice the
 *     matching %08X.dds files back in, reproducing the original byte-for-byte.
 *
 * I/O goes through CoroFileProc (chunked, stream-like, no whole-file load).
 * Like cabdet, the signature search runs directly on the coroutine input
 * window: memchr for 'D' over pin[0].ptr..end, memcmp for the full "DDS ", and
 * bulk memcpy of non-candidate bytes into the output window (fast_skip/putn
 * below), falling back to get()/unget() only at buffer boundaries and while
 * draining pushback.  A candidate that fails to validate is flushed back
 * verbatim and rescanned -- carving is always lossless.
 *
 * Texture extent: this is where DDS differs from CAB.  A cabinet declares its
 * own size in CFHEADER.cbCabinet; a .dds does not declare anything, so the
 * length has to be *computed* from the header -- format, dimensions, mip
 * count, depth, cube faces, array size (dds_header.inc, shareable with a
 * future ddsrec).  Consequences:
 *   - the header is validated hard before we trust the computed length
 *     (dwSize==124, ddspf.dwSize==32, sane dims, mip count consistent with the
 *     dimensions, recognised format, dwPitchOrLinearSize agreeing with the
 *     computed level-0 pitch/linear size), and
 *   - an unrecognised FourCC or DXGI format is *rejected* rather than sized by
 *     guesswork, so such a texture is passed through untouched.
 * Even so, carve/restore is lossless regardless of the sizing: the sidecar
 * holds exactly the bytes consumed and restore splices exactly those back, so
 * a mis-sized texture would round-trip correctly, just with a short tail left
 * in the container (or a trailing tail folded into the .dds).
 *
 * Note the marker is itself immune to re-detection: bytes [4..8) hold "DDSD",
 * so a DDS parse of a marker reads dwSize==0x44534444 and fails immediately.
 *
 * C++17, C stdio only. Drops into the 012 tree; build like cabdet.cpp.
 */

#define INC_LOG2I
#include "common.inc"
#include "coro3b.inc"
#include "coro_fp.inc"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>

/* the dxt5comp codec + the SOLID multi-texture container (namespace d5c).
 * needs the dxt5comp tree on the include path so sh_mapping.inc and MOD/*.inc
 * resolve; see D5C in g.bat. system headers above are its prerequisites. */
#include "dxt5comp_core.inc"

#define DDSDET_VERSION "r28"

/* ------------------------------------------------------------------ CRC-32 */
/* Standard reflected CRC (poly 0xEDB88320) used only for marker integrity.  */
static uint g_crc[256];

static void crc_init() {
  for( uint n = 0; n<256; n++ ) {
    uint c = n;
    for( int k = 0; k<8; k++ )
      c = (c&1) ? (0xEDB88320u^(c>>1)) : (c>>1);
    g_crc[n] = c;
  }
}

static uint crc32b(const byte* p, size_t n) {
  uint c = 0xFFFFFFFFu;
  for( size_t i = 0; i<n; i++ )
    c = g_crc[(c^p[i])&0xFF]^(c>>8);
  return c^0xFFFFFFFFu;
}

/* --------------------------------------------------------- byte helpers BE */
static uint get_be32(const byte* p) {
  return (uint)p[0]<<24|(uint)p[1]<<16|(uint)p[2]<<8|(uint)p[3];
}
static qword get_be64(const byte* p) {
  return (qword)get_be32(p)<<32|get_be32(p+4);
}
static void put_be32(byte* p, uint v) {
  p[0] = v>>24; p[1] = v>>16; p[2] = v>>8; p[3] = v;
}
static void put_be64(byte* p, qword v) {
  put_be32(p, (uint)(v>>32)); put_be32(p+4, (uint)v);
}

/* ----------------------------------------------------------------- markers */
static const byte DDS_SIG[4] = {'D', 'D', 'S', ' '};
static const byte MK_TAG[8]  = {'D', 'D', 'S', 'D', 'E', 'T', 'v', '1'};

/* Marker layout (44 bytes, all multi-byte fields big-endian):
 *   [ 0]  4  dds signature  "DDS "  (kept in place; "looks" like a texture)
 *   [ 4]  8  tag "DDSDETv1"         (lands on dwSize+dwFlags, so dwSize reads
 *                                    0x44534444 and a real DDS parse fails)
 *   [12]  4  index                  (-> %08X.dds filename)
 *   [16]  8  orig_len               (original texture byte length == file size)
 *   [24]  8  placeholder_len        (bytes this placeholder occupies)
 *   [32]  8  orig_off               (original offset in source; diagnostics)
 *   [40]  4  crc32 over bytes [0..40)
 */
enum { MK_SIZE = 44 };

static void mk_build(byte b[MK_SIZE], uint index, qword orig_len, qword placeholder_len, qword orig_off) {
  memcpy(b+0, DDS_SIG, 4);
  memcpy(b+4, MK_TAG, 8);
  put_be32(b+12, index);
  put_be64(b+16, orig_len);
  put_be64(b+24, placeholder_len);
  put_be64(b+32, orig_off);
  put_be32(b+40, crc32b(b, 40));
}

static bool mk_parse(const byte* b, uint* index, qword* orig_len, qword* placeholder_len, qword* orig_off) {
  if( memcmp(b, DDS_SIG, 4)!=0 ) return false;
  if( memcmp(b+4, MK_TAG, 8)!=0 ) return false;
  if( get_be32(b+40)!=crc32b(b, 40) ) return false;
  *index = get_be32(b+12);
  *orig_len = get_be64(b+16);
  *placeholder_len = get_be64(b+24);
  *orig_off = get_be64(b+32);
  return true;
}

static const byte zpad[4096] = {0};

struct vector_bytes { byte* p = 0; size_t n = 0; ~vector_bytes() { free(p); } };

/* ------------------------------------------------------------------- I/O */
static byte* read_file(const char* path, size_t* out_n) {
  FILE* f = fopen(path, "rb");
  if( !f ) { fprintf(stderr, "ddsdet: cannot open '%s'\n", path); return 0; }
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  rewind(f);
  if( sz<0 ) { fclose(f); return 0; }
  byte* buf = (byte*)malloc((size_t)sz ? (size_t)sz : 1);
  if( !buf ) { fclose(f); fprintf(stderr, "ddsdet: out of memory\n"); return 0; }
  if( sz && fread(buf, 1, (size_t)sz, f)!=(size_t)sz ) {
    fclose(f); free(buf); fprintf(stderr, "ddsdet: short read on '%s'\n", path); return 0;
  }
  fclose(f);
  *out_n = (size_t)sz;
  return buf;
}

static bool write_file(const char* path, const byte* data, size_t n) {
  FILE* f = fopen(path, "wb");
  if( !f ) { fprintf(stderr, "ddsdet: cannot create '%s'\n", path); return false; }
  if( n && fwrite(data, 1, n, f)!=n ) { fclose(f); fprintf(stderr, "ddsdet: write failed on '%s'\n", path); return false; }
  fclose(f);
  return true;
}


/* ------------------------------------------------- endpoint bmp (dxt5 "cv")

   --bmp writes each modelled texture's endpoint fields to a numbered 32bpp
   BMP instead of range-coding them, so an external image codec can have a go
   at the plane that dominates a .d5s (endpoints are ~24% of a coded DXT5).
   The index planes stay in the archive and lose nothing: the decoder reads
   the BMP back before decoding indices, so the palette, p^ and every
   endpoint-derived context still see the real values.

   Geometry follows dxt5.cpp's cv mode: blocks of every mip level are laid out
   continuously at width plane_bw = ceil(w/4); with vertical pairs the image is
   plane_bw wide and 2*plane_bh tall, endpoint 0 on row 2*by and endpoint 1 on
   row 2*by+1. Pixels are BGRA with the raw 565 fields unshifted (B=b 0..31,
   G=g 0..63, R=r 0..31) and A = the DXT5 alpha endpoint, 255 otherwise --
   the same convention dxt5.cpp uses, so these files are interchangeable. */
#pragma pack(push,1)
struct bmp_fh { word type; uint size; word r1, r2; uint offbits; };
struct bmp_ih { uint size; int w, h; word planes, bpp; uint comp, simg; int xppm, yppm; uint clrused, clrimp; };
#pragma pack(pop)

static bool wbmp32(const char* path, uint w, uint h, const byte* src) {
  bmp_fh fh = {}; bmp_ih ih = {};
  ih.size = sizeof(ih); ih.w = int(w); ih.h = int(h); ih.planes = 1; ih.bpp = 32;
  ih.simg = w * 4 * h;
  fh.type = 0x4D42; fh.offbits = sizeof(fh) + sizeof(ih); fh.size = fh.offbits + ih.simg;
  FILE* f = fopen(path, "wb");
  if( !f ) { fprintf(stderr, "ddsdet: can't create '%s'\n", path); return false; }
  bool ok = fwrite(&fh, sizeof fh, 1, f)==1 && fwrite(&ih, sizeof ih, 1, f)==1;
  for( uint y = h; ok && y--; ) ok = fwrite(src + size_t(w)*4*y, size_t(w)*4, 1, f)==1;
  fclose(f);
  return ok;
}

static bool rbmp32(const char* path, uint& w, uint& h, d5c::vector<byte>& px) {
  size_t n = 0; byte* b = read_file(path, &n);
  if( !b ) return false;
  if( n < sizeof(bmp_fh)+sizeof(bmp_ih) ) { free(b); fprintf(stderr, "ddsdet: '%s' truncated\n", path); return false; }
  bmp_fh fh; bmp_ih ih;
  memcpy(&fh, b, sizeof fh); memcpy(&ih, b+sizeof fh, sizeof ih);
  if( fh.type!=0x4D42 || ih.size<40 || ih.planes!=1 || ih.bpp!=32 || (ih.comp!=0 && ih.comp!=3) ) {
    free(b); fprintf(stderr, "ddsdet: '%s': need a 32bpp BI_RGB bmp\n", path); return false;
  }
  bool flip = ih.h > 0;
  w = uint(ih.w); h = uint(flip ? ih.h : -ih.h);
  size_t pitch = size_t(w)*4;
  if( !w || !h || (qword)fh.offbits + (qword)pitch*h > (qword)n ) {
    free(b); fprintf(stderr, "ddsdet: '%s' truncated\n", path); return false;
  }
  px.assign(size_t(w)*h*4, 0);
  for( uint y = 0; y < h; y++ )
    memcpy(px.data() + pitch*y, b + fh.offbits + pitch*(flip ? h-1-y : y), pitch);
  free(b);
  return true;
}

/* Endpoint-bitmap layout.
 *
 * One bitmap per FACE (cubemap face / DX10 array element), because those are
 * separate images and packing them into one plane gave an image codec a seam
 * every face boundary.
 *
 * Within a face, level 0 sits at the top and levels 1.. lie SIDE BY SIDE in a
 * strip beneath it.  Because sum(bw_l, l>=1) ~ bw_0 and the tallest strip
 * member is level 1, the strip is about W x H/2 -- the classic mip-strip.  The
 * previous layout packed every block of every level linearly and wrapped at
 * level-0 width, which re-wrapped each mip level at the wrong pitch and
 * destroyed its 2D structure; that is exactly what an image codec needs to see.
 *
 * Depth slices stack vertically within their level, so a volume texture is
 * well-defined too.  Each block is one column and two rows: row 0 endpoint 0,
 * row 1 endpoint 1.
 *
 * THIS CHANGES THE .bmp INTERCHANGE FORMAT -- endpoint bitmaps written by an
 * older build will not load. */
enum { EP_MAXLEV = 32 };
struct EpLayout {
  uint w, h;                      // pixels
  uint sx[EP_MAXLEV];             // left column of the level
  uint sy[EP_MAXLEV];             // top pixel row of its slice 0
  uint dz[EP_MAXLEV];             // pixel rows per depth slice
  qword fblocks;                  // epx records in one face (channels included)
  uint  nch;                      // BC5 writes one record per block PER CHANNEL
};
static void ep_layout(const d5c::dds_info& di, EpLayout& L) {
  uint w0,h0,bw0,bh0; d5c::level_dims(di, 0, w0, h0, bw0, bh0);
  const uint band0 = bh0 * 2 * d5c::level_depth(di, 0);
  uint stripx = 0, striph = 0;
  L.fblocks = 0;
  L.nch = (d5c::fmt_ach(di.fmt) == 2) ? 2u : 1u;
  for( uint l = 0; l < di.levels && l < EP_MAXLEV; l++ ) {
    uint w,h,bw,bh; d5c::level_dims(di, l, w, h, bw, bh);
    const uint d = d5c::level_depth(di, l);
    L.dz[l] = bh * 2;
    L.fblocks += qword(bw) * bh * d * L.nch;
    if( l == 0 ) { L.sx[0] = 0; L.sy[0] = 0; }
    else {
      L.sx[l] = stripx; L.sy[l] = band0; stripx += bw;
      if( bh * 2 * d > striph ) striph = bh * 2 * d;
    }
  }
  L.w = bw0 > stripx ? bw0 : stripx;
  L.h = band0 + striph;
}

/* Solid's epx stream (6 B/block: a0,a1,c0,c1) <-> one face's bitmap */
/* One bitmap per FACE.  Pixel layout, two rows per block (endpoint 0, endpoint 1):
 *
 *   colour formats   B,G,R = the 565 endpoint's fields;  A = alpha endpoint
 *   BC4 / BTC        A = alpha endpoint;                 RGB = 0
 *   BC5              A = channel 0's endpoint, R = channel 1's;  G,B = 0
 *
 * BC5's two channels share one bitmap: four endpoint values per block against
 * eight bytes of RGBA, so a second file would have been pure waste -- and
 * co-locating them lets an image codec see the cross-channel correlation
 * instead of having it split across files.  Channel 0 keeps the A slot so the
 * "alpha lives in A" rule holds for every alpha-bearing format. */
static void ep_to_bmp(const d5c::dds_info& di, const d5c::ByteBuf& ep, uint face,
                      d5c::vector<byte>& px, uint& w, uint& h) {
  EpLayout L; ep_layout(di, L);
  w = L.w; h = L.h;
  px.assign(size_t(w) * h * 4, 0);
  const qword nb = ep.size() / 6;
  qword i = L.fblocks * face;
  for( uint l = 0; l < di.levels && l < EP_MAXLEV; l++ ) {
    uint lw,lh,bw,bh; d5c::level_dims(di, l, lw, lh, bw, bh);
    const qword nblk = qword(bw) * bh;
    for( uint z = 0; z < d5c::level_depth(di, l); z++, i += nblk * L.nch ) {
      for( uint by = 0, b = 0; by < bh; by++ )
        for( uint bx = 0; bx < bw; bx++, b++ ) {
          if( i + b >= nb ) return;
          const byte* e = ep.data() + size_t(i + b)*6;
          const byte* e2 = (L.nch == 2 && i + nblk + b < nb)
                         ? ep.data() + size_t(i + nblk + b)*6 : nullptr;
          const uint cx = L.sx[l] + bx, cy = L.sy[l] + z * L.dz[l] + by * 2;
          for( int k = 0; k < 2; k++ ) {
            uint c = e[2+k*2] | uint(e[3+k*2]) << 8;
            byte* d = px.data() + (size_t(cy + k) * w + cx) * 4;
            d[0] = byte(c & 31); d[1] = byte(c >> 5 & 63); d[2] = byte(c >> 11); d[3] = e[k];
            if( e2 ) d[2] = e2[k];             // BC5 channel 1 -> R
          }
        }
    }
  }
}

static bool bmp_to_ep(const d5c::dds_info& di, const d5c::vector<byte>& px, uint w, uint h,
                      uint face, d5c::ByteBuf& ep) {
  EpLayout L; ep_layout(di, L);
  if( w != L.w || h != L.h ) {
    fprintf(stderr, "ddsdet: endpoint bmp is %ux%u, expected %ux%u\n", w, h, L.w, L.h);
    return false;
  }
  const qword nb = ep.size() / 6;
  qword i = L.fblocks * face;
  for( uint l = 0; l < di.levels && l < EP_MAXLEV; l++ ) {
    uint lw,lh,bw,bh; d5c::level_dims(di, l, lw, lh, bw, bh);
    const qword nblk = qword(bw) * bh;
    for( uint z = 0; z < d5c::level_depth(di, l); z++, i += nblk * L.nch ) {
      for( uint by = 0, b = 0; by < bh; by++ )
        for( uint bx = 0; bx < bw; bx++, b++ ) {
          if( i + b >= nb ) return true;
          byte* e = ep.data() + size_t(i + b)*6;
          byte* e2 = (L.nch == 2 && i + nblk + b < nb)
                   ? ep.data() + size_t(i + nblk + b)*6 : nullptr;
          const uint cx = L.sx[l] + bx, cy = L.sy[l] + z * L.dz[l] + by * 2;
          for( int k = 0; k < 2; k++ ) {
            const byte* d = px.data() + (size_t(cy + k) * w + cx) * 4;
            uint c = uint(d[2] & 31) << 11 | uint(d[1] & 63) << 5 | uint(d[0] & 31);
            e[k] = d[3];
            if( e2 ) { e2[k] = d[2]; e2[2+k*2] = 0; e2[3+k*2] = 0; c = 0; }
            e[2+k*2] = byte(c); e[3+k*2] = byte(c >> 8);
          }
        }
    }
  }
  return true;
}

/* One file per face; the single-face case keeps the original name. */
static void ep_name(char* dst, size_t cap, const char* prefix, uint index, uint surf, uint nsurf) {
  if( nsurf <= 1 ) snprintf(dst, cap, "%s%08X_ep.bmp", prefix, index);
  else             snprintf(dst, cap, "%s%08X_s%02u_ep.bmp", prefix, index, surf);
}

static void dds_name(char* dst, size_t cap, const char* prefix, uint index) {
  snprintf(dst, cap, "%s%08X.dds", prefix, index);
}

/* =========================================================== ddsdet model */

#include "ddsdet_carve.inc"
#include "ddsdet_drive.inc"


/* Settings main() parses once and hands to whichever direction runs. */
struct RunCfg {
  const char* outpath = 0;         // opened lazily by the driver
  uint op, why, bmp, zero, embed, nonsolid;
  uint dedup_hb, dedup_mb;         // dedup table log2 sizes; hb 0 = off
  const char *bprefix, *solpath, *prefix;
  const char *arcout;            // cs: where the archive goes
};

/* Configure the carrier, run it, and do the direction-specific tail.
 * `if constexpr` keeps this one function: the encoder-only archive write is
 * not even compiled into the decode instantiation. */
template<int mode, class Carrier, class Sol>
static int run_dir( Carrier& M, Sol* sol, const RunCfg& c, FILE* f, FILE* g ) {
  M.op = c.op; M.sol = sol; M.why = c.why; M.bmp = c.bmp;
  M.bprefix = c.bprefix; M.solpath = c.solpath;
  M.zero = c.zero; M.embed = c.embed;
  M.cfg_dedup_hb = c.dedup_hb; M.cfg_dedup_mb = c.dedup_mb;
  strncpy(M.prefix, c.prefix, sizeof(M.prefix)-1);
  M.prefix[sizeof(M.prefix)-1] = 0;

  M.out.path = c.outpath;            // deferred: created on the first byte written
  M.processfile(f, g);
  fclose(f);
  bool wok = M.out.close() && M.rcout.close();
  if( M.rcf ) fclose(M.rcf);
  if( !wok && M.rc==0 ) M.rc = 1;

  if constexpr( mode == 0 ) {
    if( M.rc==0 && M.op==2 ) {                  // the archive is already on disk
      qword raw = sol->raw_total;
      qword arcsz = sol->cx.rc.emitted;          // raw_put counted the magic too
      fprintf(stderr, "ddsdet: %u dds, %llu -> %llu bytes %s (%.3f bpb), directory %zu B\n",
              M.zindex, (unsigned long long)raw, (unsigned long long)arcsz,
              c.nonsolid ? "non-solid" : "solid",
              raw ? 8.0*(double)arcsz/(double)raw : 0.0, sol->meta_coded);
    }
  }
  if( M.rc==0 ) {
    /* Dedup lives in Solid for the archive modes and in the carrier for the
       sidecar ones, so the report has to look in whichever ran. */
    const d5c::Dedup* dd = (sol && sol->dd.on) ? &sol->dd : (M.dd.on ? &M.dd : 0);
    if( dd && dd->hits )
      fprintf(stderr, "ddsdet: dedup: %llu duplicate DDS, %llu bytes not %s\n",
              (unsigned long long)dd->hits, (unsigned long long)dd->saved,
              mode==0 ? "coded" : "decoded");
    if( M.op==4 )
      fprintf(stderr, "ddsdet: restored %u DDS(es), %llu bytes\n",
              M.zindex, (unsigned long long)M.allbytes);
    else
      fprintf(stderr, "ddsdet: %s %u DDS(es)\n", (M.op==0||M.op==2) ? "extracted" : "restored", M.zindex);
  }
  return M.rc;
}

/* ------------------------------------------------------------------ main */
static int usage() {
  fprintf(stderr,
    "usage: ddsdet c|d  [--z|--zero] inputfile outputfile [dds_prefix]\n"
    "       ddsdet cs   [--z|--zero] inputfile restfile ddsfile\n"
    "       ddsdet ds                restfile  ddsfile  outputfile\n"
    "  shortcuts: append a=--all, b=--bmp, n=--ns, p=--dedup to c/d/cs/ds, in\n"
    "             any order and combination -- cp, csa, csap, csabn, dsa ...\n"
    "  c  carve : extract embedded DDS to <prefix>%%08X.dds, replace with markers\n"
    "     --z/--zero : zero-pad markers to original DDS size (preserve offsets)\n"
    "  d  decode: restore original file from a carved file + its .dds parts\n"
    "  cs carve solid : all textures -> one dxt5comp-coded .d5s archive,\n"
    "                   everything else -> restfile, uncompressed\n"
    "  ds decode solid: rebuild the original from restfile + the .d5s archive\n"
    "  --all : (cs/ds) fold the non-texture bytes into the coded stream too, so\n"
    "          the archive is the only output: cs --all infile archive,\n"
    "          ds --all archive outfile. No restfile and no markers -- the\n"
    "          gaps between textures are coded by the same model\n"
    "  --why : report why each \"DDS \" candidate that was NOT carved got rejected\n"
    "  --ns  : (cs) non-solid -- reset the model once per .dds file, so each file\n"
    "          is coded independently while still landing in one archive. the mip\n"
    "          levels inside a file still share a model. whether this wins depends\n"
    "          on the parameter set -- measure both\n"
    "  --bmp : (cs/ds) endpoints go to <prefix>%%08X_ep.bmp instead of the coder,\n"
    "          dxt5 \"cv\" layout; with it the third file is a PREFIX, and the\n"
    "          archive itself is written to that prefix verbatim\n"
    "  --dedup : detect textures byte-identical to one already seen and write a\n"
    "          reference instead of coding them again. In c mode the repeat\n"
    "          writes no .dds and its marker names the first copy's file; in cs\n"
    "          mode the archive entry is a back-reference. Matches are keyed by\n"
    "          SHA-256 and confirmed byte for byte, so a hash collision costs a\n"
    "          dedup, never correctness. Decoding needs no flag -- the archive\n"
    "          says so -- but the decoder does hold the same table, hence the\n"
    "          memory bound below\n"
    "  --dedup-mem N  bytes of texture the table keeps live -- the window a\n"
    "          repeat has to fall inside to be caught. K/M/G suffixes, rounded\n"
    "          up to a power of two, default 256M. BOTH directions hold this\n"
    "          much, so the value rides in the archive and the decoder needs no\n"
    "          matching command line\n"
    "  --dedup-slots N  table slots, rounded up to a power of two; default 64K.\n"
    "          One slot per bucket, so a collision costs a missed duplicate\n"
    "  dds_prefix defaults to \"dds_\"\n"
    "ddsdet " DDSDET_VERSION "\n");
  return 2;
}

int main( int argc, char** argv ) {
  crc_init();
  fprintf(stderr, "ddsdet " DDSDET_VERSION "\n");
  if( argc<4 ) return usage();

  // flags may appear anywhere, including before the mode
  bool zero = false, why = false, bmpx = false, nonsolid = false, embed = false;
  bool dedup = false;
  qword dedup_mem = (qword)1<<28;         // 256 MiB of retained textures
  qword dedup_slots = 1<<16;
  const char* mode = 0;
  const char* pos[3] = {0,0,0};
  int np = 0;
  /* Round a byte/count argument up to a power of two: the table masks with it
     and the archive stores its log2, so nothing else is representable. */
  auto p2 = []( const char* s, qword lo, qword hi ) {
    char* e = 0;
    qword v = strtoull(s, &e, 0);
    if( e ) switch( *e ) { case 'k': case 'K': v <<= 10; break;    // 256M reads
                           case 'm': case 'M': v <<= 20; break;    // better than
                           case 'g': case 'G': v <<= 30; break; }  // 268435456
    if( v<lo ) v = lo;
    if( v>hi ) v = hi;
    qword r = lo; while( r<v ) r <<= 1;
    return r;
  };
  for( int a=1; a<argc; a++ ) {
    if( strcmp(argv[a],"--z")==0 || strcmp(argv[a],"--zero")==0 ) zero = true;
    else if( strcmp(argv[a],"--why")==0 ) why = true;
    else if( strcmp(argv[a],"--bmp")==0 ) bmpx = true;
    else if( strcmp(argv[a],"--ns")==0 || strcmp(argv[a],"--nonsolid")==0 ) nonsolid = true;
    else if( strcmp(argv[a],"--all")==0 ) embed = true;
    else if( strcmp(argv[a],"--dedup")==0 || strcmp(argv[a],"--p")==0 ) dedup = true;
    else if( strcmp(argv[a],"--dedup-mem")==0 && a+1<argc )
      { dedup_mem = p2(argv[++a], 1u<<20, (qword)1<<46); dedup = true; }
    else if( strcmp(argv[a],"--dedup-slots")==0 && a+1<argc )
      { dedup_slots = p2(argv[++a], 1u<<8, 1u<<26); dedup = true; }
    else if( !mode ) mode = argv[a];
    else if( np<3 ) pos[np++] = argv[a];
    else { fprintf(stderr, "ddsdet: too many arguments\n"); return usage(); }
  }
  if( !mode || np<2 ) return usage();

  /* Shortcut modes: cs/ds followed by any combination of the flag letters, in
   * any order -- csa == "cs --all", csab == csba == "cs --all --bmp", and so on
   * up to csabn.  Expanded here, before anything looks at `mode`, so the rest
   * of main() still sees a plain "cs"/"ds" and every strcmp below is unchanged.
   *
   * A suffix is only consumed if EVERY letter is a known flag; "csx" is left
   * alone and falls through to the usual unknown-mode error rather than being
   * silently read as "cs". */
  char modebuf[8];
  if( mode[0]=='c' || mode[0]=='d' ) {
    /* "cs"/"ds" take every letter; plain "c"/"d" take only p, because a/b/n
     * describe the archive and there is no archive here -- so "cn" stays the
     * unknown-mode error it always was rather than silently doing nothing. */
    const int sfx = (mode[1]=='s') ? 2 : 1;
    const char* legal = (sfx==2) ? "abnp" : "p";
    if( mode[sfx] ) {
      bool ok = true;
      for( const char* q = mode+sfx; *q; q++ )
        if( !strchr(legal, *q) ) { ok = false; break; }
      if( ok ) {
        for( const char* q = mode+sfx; *q; q++ )
          switch( *q ) { case 'a': embed = true; break;
                         case 'b': bmpx  = true; break;
                         case 'n': nonsolid = true; break;
                         case 'p': dedup = true; break; }
        modebuf[0] = mode[0]; modebuf[1] = 's'; modebuf[sfx] = 0;
        mode = modebuf;
      }
    }
  }

  const char* infile  = pos[0];
  const char* outfile = pos[1];
  const char* prefix  = (np>=3) ? pos[2] : "dds_";

  bool solid = (strcmp(mode,"cs")==0) || (strcmp(mode,"ds")==0);
  if( strcmp(mode,"c")!=0 && strcmp(mode,"d")!=0 && !solid ) return usage();
  if( solid && !embed && np<3 ) { fprintf(stderr, "ddsdet: solid modes need three files\n"); return usage(); }
  if( solid && embed && np<2 ) { fprintf(stderr, "ddsdet: cs/ds --all need two files\n"); return usage(); }
  if( (strcmp(mode,"d")==0 || strcmp(mode,"ds")==0) && zero )
    fprintf(stderr, "ddsdet: note: --zero is ignored when decoding\n");

  // direction: 0 = carve/encode (c, cs), 1 = restore/decode (d, ds)
  const bool dec_dir = (strcmp(mode,"d")==0) || (strcmp(mode,"ds")==0);
  d5c::Solid<0>* sole = 0;
  d5c::Solid<1>* sold = 0;
  vector_bytes archive;                       // ds: the whole .d5s, loaded once
  if( solid ) {
    // default-init, NOT new Solid(): value-initialization would zero the whole
    // object, dirtying every page of the static geometry slabs and model
    // tables that Solid/Codec now carry.  cx.init() fills what it uses.
    // Only the direction actually running is allocated.
    if( strcmp(mode,"cs")==0 ) {
      // the codec's RC IS the carver's base subobject: one coroutine, one
      // coder, and the archive rides pin2
      // the codec's RC IS the carver's base subobject: one coroutine, one
      // coder, and the archive rides pin2 straight to its file -- no staging
      // buffer, and the D5S4 magic is the head of the coded stream itself
      const char* ap = embed ? pos[1] : pos[2];
      Menc().rcout.path = ap;          // opened on the first coded byte
      sole = d5c::onew< d5c::Solid<0> >("solid encoder", Menc());
      sole->rcpin = &Menc().pin[2];
      Menc().cfg_nonsolid = nonsolid ? 1 : 0;
      Menc().cfg_embed    = embed ? 1 : 0;
    } else {
      // ds: pos[0]=restfile, pos[1]=ddsfile, pos[2]=outputfile
      //     ds --all: pos[0]=archive, pos[1]=outputfile
      const char* ap = embed ? pos[0] : pos[1];
      Mdec().rcf = fopen(ap, "rb");
      if( !Mdec().rcf ) { fprintf(stderr, "ddsdet: cannot open '%s'\n", ap); return 1; }
      sold = d5c::onew< d5c::Solid<1> >("solid decoder", Mdec());
      sold->rcpin = &Mdec().pin[3];
    }
  }

  if( solid && embed && strcmp(mode,"ds")==0 ) { outfile = pos[1]; infile = "/dev/null"; }
  FILE* f = fopen(infile, "rb");
  if( !f ) { fprintf(stderr, "ddsdet: cannot open '%s'\n", infile); return 1; }
  if( solid && !embed && strcmp(mode,"ds")==0 ) outfile = pos[2];
  FILE* g = 0;                       // the drivers open `outfile` on first write

  RunCfg c;
  c.op   = (strcmp(mode,"c")==0) ? 0 : (strcmp(mode,"d")==0) ? 1 : (strcmp(mode,"cs")==0) ? 2 : 3;
  // ds --all: archive in, container out -- op 4, driven from do_process
  if( c.op==3 && embed ) c.op = 4;
  c.outpath = outfile;
  c.why  = why ? 1 : 0;
  c.bmp  = bmpx ? 1 : 0;
  c.bprefix = solid ? ((strcmp(mode,"cs")==0) ? (embed ? pos[1] : pos[2]) : pos[1]) : "";
  c.solpath = c.bprefix;
  c.zero = zero ? 1 : 0;
  c.embed = (embed && c.op==2) ? 1 : 0;
  c.nonsolid = nonsolid ? 1 : 0;
  /* Only the carve directions decide about dedup.  ds reads the geometry out
     of the archive, and d needs no table at all -- a duplicate's marker just
     names a sidecar that is already on disk. */
  c.dedup_hb = c.dedup_mb = 0;
  if( dedup && !dec_dir ) {
    uint hb = 0, mb = 0;
    while( ((qword)1<<(hb+1)) <= dedup_slots ) hb++;
    while( ((qword)1<<(mb+1)) <= dedup_mem  ) mb++;
    c.dedup_hb = hb; c.dedup_mb = mb;
  } else if( dedup )
    fprintf(stderr, "ddsdet: note: --dedup is ignored when decoding (the archive carries it)\n");
  c.prefix = prefix;
  c.arcout = (solid && strcmp(mode,"cs")==0) ? (embed ? pos[1] : pos[2]) : "";

  int r_ = dec_dir ? run_dir<1>(Mdec(), sold, c, f, g)
                   : run_dir<0>(Menc(), sole, c, f, g);
  return r_;
}
