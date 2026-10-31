// ctx.inc -- syntax.inc's Ctx: the stream's state the syntax depends on, and the parsers of the
// headers that set it.

// The stream's state the syntax depends on (the VOL, the encoder's build in the user data, the GOP's
// time), and the parsers of the headers that set it
struct Ctx {
  int have_vol;
  const char* unsupported;   // why VOPs of the current VOL are kept raw
  int dp;                    // data partitioning: only B-VOPs are supported
  int width, height, mb_w, mb_h, mb_num, mb_num_bits;
  int interlaced, sprite_usage, warp_points, quant_precision, time_inc_bits;
  int cplx_i, cplx_p, cplx_b;
  int divx_version, divx_build, xvid_build;
  int time_res;              // vop_time_increment_resolution
  int fixed_inc;             // fixed_vop_time_increment (0: none, the VOPs' times vary)
  int qpel;                  // quarter_sample
  // what the reconstruction (recon.inc) needs beyond the syntax: the VOL's quantiser matrices
  // (natural order) and warping accuracy, the libavcodec build of the user data (its bugs), how many
  // VOL and GOP headers came (a VOL resets the field times, a GOP the time base: gop_time, seconds)
  int vo_type, mpeg_quant, warp_acc, lavc_build, vol_count, gop_count, gop_time;
  U16 intra_matrix[BLK_COEFS], inter_matrix[BLK_COEFS];
  // c: skip flags of the last anchor VOP and of the one being parsed (a B-VOP macroblock whose
  // co-located macroblock in the backward reference was skipped has no bits), skip_cap entries
  U8 *skip_ref, *skip_cur;
  size_t skip_cap;
  U8 hbuf[HDR_PEEK + GB_PAD];  // a header chunk being parsed

  void init() {  // (the skip flags stay: quit() frees them)
    U8 *r = skip_ref, *u = skip_cur;
    const size_t cap = skip_cap;
    memset(this, 0, sizeof *this);
    skip_ref = r;
    skip_cur = u;
    skip_cap = cap;
    divx_version = divx_build = xvid_build = lavc_build = -1;
    time_inc_bits = TIB_DEFAULT;
    quant_precision = QUANT_PREC_DEFAULT;
  }
  void quit() {
    free(skip_ref);
    free(skip_cur);
    skip_ref = skip_cur = 0;
    skip_cap = 0;
  }
  // room for the skip flags of n macroblocks and one more (0: no memory)
  int skip_room(int n) {
    size_t r = skip_cap, u = skip_cap;
    if ((size_t)++n <= skip_cap) return 1;
    if (!grow(&skip_ref, &r, n) || !grow(&skip_cur, &u, n)) return 0;
    memset(skip_ref + skip_cap, 0, r - skip_cap);
    memset(skip_cur + skip_cap, 0, u - skip_cap);
    skip_cap = r;
    return 1;
  }

#include "parse_vol.inc"  // parse_vol(): the VOL header

  // user data: encoder identification (m4vdec's decode_user_data, the parts the syntax depends on)
  void parse_user_data(GB& gb) {
    char buf[USER_DATA_MAX + 1], last;
    int i, e, ver = 0, build = 0;
    for (i = 0; i < USER_DATA_MAX && gb.pos < gb.size; i++) {
      if (gb.show(SC_ZEROS) == 0) break;
      buf[i] = (char)gb.get(BYTE_BITS);
    }
    buf[i] = 0;
    e = sscanf(buf, "DivX%dBuild%d%c", &ver, &build, &last);
    if (e < 2) e = sscanf(buf, "DivX%db%d%c", &ver, &build, &last);
    if (e >= 2) { divx_version = ver; divx_build = build; }
    {  // libavcodec's build (m4vdec's decode_user_data)
      int v2 = 0, v3 = 0, lb = 0;
      constexpr int ALL = 4;  // (the fields of the longest format, matched: sscanf()'s count, made up to it)
      e = sscanf(buf, "FFmpe%*[^b]b%d", &lb) + ALL - 1;
      if (e != ALL) e = sscanf(buf, "FFmpeg v%d.%d.%d / libavcodec build: %d", &ver, &v2, &v3, &lb);
      if (e != ALL) {
        e = sscanf(buf, "Lavc%d.%d.%d", &ver, &v2, &v3) + 1;
        if (e > 1) lb = lavc_version(ver, v2, v3);
      }
      if (e != ALL) { if (strcmp(buf, "ffmpeg") == 0) lavc_build = LAVC_BUILD_FFMPEG; }
      if (e == ALL) lavc_build = lb;
    }
    if (sscanf(buf, "XviD%d", &build) == 1) xvid_build = build;
  }

  // GOP header: its time code resets the time base (m4vdec's decode_gop_header)
  void parse_gop(GB& gb) {
    int hours, minutes, seconds;
    if (!gb.show(SC_ZEROS)) return;
    hours = (int)gb.get(HOURS_BITS);
    minutes = (int)gb.get(MINUTES_BITS);
    gb.skip(1);
    seconds = (int)gb.get(SECONDS_BITS);
    gop_time = seconds + MINUTE_SECS * (minutes + HOUR_MINUTES * hours);
    gop_count++;
  }

  // a chunk (the first 32 KiB of it) that isn't a VOP: VOL headers and user data matter
  void parse_header_chunk(pcU8 p, size_t n) {
    GB gb;
    int code;
    if (n < SC_LEN || (code = sc_code(p)) < 0) return;
    if (n > HDR_PEEK) n = HDR_PEEK;
    memcpy(hbuf, p, n);
    memset(hbuf + n, 0, GB_PAD);
    gb.init(hbuf + SC_LEN, n - SC_LEN);
    if (sc_is_vol(code)) parse_vol(gb);
    else if (code == SC_USER_DATA) parse_user_data(gb);
    else if (code == SC_GOV) parse_gop(gb);
  }

  int divx413() const { return divx_version == DIVX413_VERSION && divx_build == DIVX413_BUILD && xvid_build < 0; }

  // Which fields a VOP of a type has, for the walks below and the model alike: global motion
  // compensation (an S-VOP of a GMC sprite), vop_rounding_type (P-VOPs, GMC S-VOPs), the complexity
  // estimates' bits (as the VOL asks); and whether a macroblock has a dct_type (interlaced: intra, or
  // with coded blocks)
  int vop_gmc(int type) const { return type == PT_S && sprite_usage == GMC_SPRITE; }
  int vop_has_rnd(int type) const { return type == PT_P || vop_gmc(type); }
  int vop_ncplx(int type) const { return cplx_i + (type != PT_I ? cplx_p : 0) + (type == PT_B ? cplx_b : 0); }
  int mb_has_dt(int intra, int cbp) const { return interlaced && (intra || cbp); }

  // a sprite trajectory (GMC) both ways (the walks below say how): each warping point's du, dv as a
  // length's code, the value (FFmpeg's xbits) and a marker (DivX 4.13 leaves out a point's first one)
  template <class IO>
  int traj_walk(IO& io, pInt w) const {
    int i;
    for (i = 0; i < 2 * warp_points; i++) {
      int len = IO::W ? bitlen(w[i]) : 0;
      if (!io.vlc(&len, vlc_sprite_traj)) return 0;
      if (!len) w[i] = 0;
      else if (!io.xbits(&w[i], len)) return 0;
      if (((i & 1) || !divx413()) && !io.marker()) return 0;
    }
    return 1;
  }
};
