// videoinfo.inc -- VideoInfo: what the video is, as a run has it, for the progress line
// (progress.inc); and video_words(), that in words.

// What the video is, as a run has it (the VOL, the encoder's user data): width 0, not known yet
struct VideoInfo {
  int width, height, interlaced, qpel, gmc, mpeg_quant, data_part;
  int time_res, fixed_inc;  // vop_time_increment_resolution, fixed_vop_time_increment (0: none)
  int divx_version, divx_build, xvid_build, lavc_build;  // (-1: none)

  void set(const Ctx& c) {
    width = c.width;
    height = c.height;
    interlaced = c.interlaced;
    qpel = c.qpel;
    gmc = c.sprite_usage == GMC_SPRITE ? c.warp_points : -1;
    mpeg_quant = c.mpeg_quant;
    data_part = c.dp;
    time_res = c.time_res;
    fixed_inc = c.fixed_inc;
    divx_version = c.divx_version;
    divx_build = c.divx_build;
    xvid_build = c.xvid_build;
    lavc_build = c.lavc_build;
  }
  bool same(const VideoInfo& v) const { return !memcmp(this, &v, sizeof v); }
};

// What the video is, in words, into b (n bytes): its size, how it is scanned, the tools its VOL turns
// on, the frame rate where it is fixed, the encoder
static void video_words(const VideoInfo& v, char* b, size_t n) {
  constexpr int LAVC_MAJOR = 2 * BYTE_BITS;  // (lavc_version()'s fields)
  size_t k = 0;
  put_text(b, n, k, "%dx%d, %s", v.width, v.height, v.interlaced ? "interlaced" : "progressive");
  if (v.qpel) put_text(b, n, k, ", quarter-sample motion");
  if (v.gmc >= 0) put_text(b, n, k, ", GMC with %d point%s", v.gmc, v.gmc == 1 ? "" : "s");
  if (v.mpeg_quant) put_text(b, n, k, ", MPEG quantisation");
  if (v.data_part) put_text(b, n, k, ", data partitioning");
  if (v.fixed_inc > 0) put_text(b, n, k, ", %.4g fps", (double)v.time_res / v.fixed_inc);
  if (v.xvid_build >= 0) put_text(b, n, k, ", XviD build %d", v.xvid_build);
  else if (v.divx_version >= 0) put_text(b, n, k, ", DivX %d build %d", v.divx_version, v.divx_build);
  if (v.lavc_build >= 0)
    put_text(b, n, k, ", libavcodec %d.%d.%d", v.lavc_build >> LAVC_MAJOR, (v.lavc_build >> BYTE_BITS) & BYTE_MASK,
             v.lavc_build & BYTE_MASK);
}
